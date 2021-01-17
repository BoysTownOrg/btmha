// stream.c - stream functions for BTMHA program

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <arsclib.h>
#include <time.h>
#include "var_list.h"
#include "btmha.h"
#include "stream.h"

#ifdef WIN32
#include <io.h>
#include <Windows.h>
#endif

#define MAX_MSG         1024

static int nsv = 0;                 // number of stream variables
static void (*process_chunk)(void **, float *, float *, int);
static VAR *svl = NULL;

/***********************************************************/

static void
msleep(uint32_t msec)
{
#ifdef WIN32
	Sleep(msec);
#else
    struct timespec delay = {0};
    uint32_t sec = msec / 1000;
    msec -= sec * 1000;
    delay.tv_sec  = sec;
    delay.tv_nsec = msec * 1000000; // convert msec to nsec
    nanosleep(&delay, &delay);
#endif
}

static int
device_init(int io_dev)
{
    int i;

    for (i = 0; (i < 8) && (!io_dev); i++) {
	io_dev = ar_find_dev(ARSC_PREF_SYNC) + 1; // find preferred audio device
	msleep(100);
    }
    var_list_set(svl, "device", &io_dev, SP_DTYP_I4, 1);
    return (io_dev);
}

/***********************************************************/

static int
get_aud(I_O *io)
{
    io->oseg = ar_io_cur_seg(io->iod);
    return (io->oseg < io->nseg);
}

static void
put_aud(I_O *io, STA *st)
{
    int cs, od, iw, ow, nd, ns;

    cs = io->cs;
    if ((io->oseg + io->mseg) == io->pseg) {
        od = io->pseg * cs;
        nd = io->nrep * io->nwav - od;
        ow = (io->pseg % io->mseg) * cs;
        iw = od % io->nwav;
        ns = (cs > (io->nwav - iw)) ? (io->nwav - iw) : cs;
        if (nd >= cs) {
            if (ns == cs) {
                fcopy(io->owav + ow, io->iwav + iw, cs);
            } else {
                fcopy(io->owav + ow, io->iwav + iw, ns);
                fcopy(io->owav + ow, io->iwav, cs - ns);
            }
        } else if (nd > 0) {
            if (ns == cs) {
                fcopy(io->owav + ow, io->iwav + iw, nd);
                fzero(io->owav + ow + nd, 2 * cs - nd);
            } else {
                fcopy(io->owav + ow, io->iwav + iw, nd);
                fcopy(io->owav + ow + nd, io->iwav + iw, ns - nd);
                fzero(io->owav + ow + ns, 2 * cs - ns);
            } 
        } else {
            fzero(io->owav, 2 * cs);
        }
        io->pseg++;
        process_chunk(st->cp, io->owav + ow, io->owav + ow, cs);
    }
}

//===========================================================

static void
set_spl(float *x, int n, double rms_lev, double spl_ref)
{
    float scl;
    double xx, rms, smsq, lev;
    int i;

    smsq = 0;
    for (i = 0; i < n; i++) {
        xx = x[i];
        smsq += xx * xx;
    }
    rms = sqrt(smsq / n);
    lev = 20 * log10(rms / spl_ref);
    scl = (float) pow(10,(rms_lev - lev) / 20);
    for (i = 0; i < n; i++) {
        x[i] *= scl;
    }
}

static int
init_wav(I_O *io, char *msg)
{
    float fs, f, p;
    int i;
    VAR *vl;

    free_null(io->iwav);
    free_null(io->owav);
    if (io->ifn && (io->ifn[0] == '~')) { // use built-in stimlus
        if (strcmp(io->ifn, "~impulse") == 0) {
            io->nwav = round(io->rate);
            io->iwav = (float *) calloc(io->nwav, sizeof(float));
            io->iwav[0] = 1;
        } else if (strcmp(io->ifn, "~tone") == 0) {
            io->nwav = round(io->rate);
            io->iwav = (float *) calloc(io->nwav, sizeof(float));
	    f = 1000;
            p = (float) ((2 * M_PI * f) / io->rate);
            for (i = 0; i < io->nwav; i++) {
                io->iwav[i] = (float) sin(i * p);
            }
        }
    } else if (io->ifn) {
        // get WAV file info
        vl = sp_wav_read(io->ifn, 0, 0, &fs);
        if (vl == NULL) {
            fprintf(stderr, "can't open %s\n", io->ifn);
            return (1);
        }
        if (io->rate != fs) {
            fprintf(stderr, "WARNING: %s rate mismatch: ", io->ifn);
            fprintf(stderr, "%.0f != %.0f\n", fs, io->rate);
            io->rate = fs;
        }
        if (msg) sprintf(msg, "%12s: %s repeat=%d\n", "WAV input ", io->ifn, io->nrep);
        io->nwav = vl[0].rows * vl[0].cols;
        io->iwav = (float *) calloc(io->nwav, sizeof(float));
        fcopy(io->iwav, vl[0].data, io->nwav);
        sp_var_clear(vl);
    } else {    /* ADC input */
        io->nwav = 0;
        io->iwav = (float *) calloc(io->cs * 2, sizeof(float));
    }
    if (io->ofn) {
        io->nsmp = io->nwav;
        io->mseg = 1;
        io->nseg = 1;
        io->owav = (float *) calloc(io->nsmp * io->noch, sizeof(float));
    } else {    /* DAC output */
        io->cs = round((io->rate * io->wait * 4) / 1000); // chunk size
        io->mseg = 2;
        io->nseg = io->nrep * io->nwav  / io->cs;
        io->owav = (float *) calloc(io->cs * (io->mseg + 1), sizeof(float));
	io->nsmp = io->nwav * io->nrep;
    } 
    io->pseg = io->mseg;
    return (0);
}

/***********************************************************/

static void
init_aud(I_O *io, char *msg)
{
    char name[80];
    int i, j, err;
    static int nchn = 2;        // number of channels
    static int nswp = 0;        // number of sweeps (0=continuous)
    static int32_t fmt[2] = {ARSC_DATA_F4, 0};

    err = ar_out_open(io->iod, io->rate, nchn);
    if (err) {
        ar_err_msg(err, msg, MAX_MSG);
        fprintf(stderr, "ERROR: %s\n", msg);
        return;
    }
    ar_dev_name(io->iod, name, 80);
    ar_set_fmt(io->iod, fmt);
    io->siz = (int32_t *) calloc(io->mseg, sizeof(int32_t));
    io->out = (void **) calloc(io->mseg * nchn, sizeof(void *));
    for (i = 0; i < io->mseg; i++) {
        io->siz[i] = io->cs;
        io->out[i * nchn] = io->owav + io->cs * i;
        for (j = 1; j < nchn; j++) {
            io->out[i * nchn + j] = NULL;
        }
    }
    ar_out_prepare(io->iod, io->out, (int32_t *)io->siz, io->mseg, nswp);
    printf("%12s: %s\n", "audio output", name);
    ar_io_start(io->iod);
}

/***********************************************************/

int
stream_prepare(I_O *io, MHA *mha, char *msg)
{
    static char *aud_in  = "capture";
    static char *aud_out = "playback";

    free_null(io->ifn);
    free_null(io->ofn);
    // initialize I/O
    io->rate = var_list_f8n(svl, "srate"  );
    io->cs   = var_list_i4n(svl, "chunk"  );
    io->ifn  = var_list_str(svl, "input"  );
    io->ofn  = var_list_str(svl, "output" );
    io->iod  = var_list_i4n(svl, "device" ) - 1;
    io->wait = var_list_i4n(svl, "io_wait");
    io->nich = var_list_i4n(svl, "nich"   );
    io->noch = var_list_i4n(svl, "noch"   );
    io->nrep = var_list_i4n(svl, "nreps"  );
    if (io->nrep < 1) io->nrep = 1;;
    if (io->ifn && strcmp(io->ifn, aud_in) == 0) {
        free_null(io->ifn);
    }
    if (io->ofn && strcmp(io->ofn, aud_out) == 0) {
        free_null(io->ofn);
    }
    if (init_wav(io, msg)) {
        return (1);
    }
    if (mha->spl_ref > 0) {
        set_spl(io->iwav, io->nwav, mha->rms_lev, mha->spl_ref);
    }
    // prepare i/o
    if (!io->ofn) {
        init_aud(io, msg + strlen(msg));
    }
    var_list_set(svl, "srate" , &io->rate, SP_DTYP_F8, 1);
    var_list_set(svl, "chunk" , &io->cs  , SP_DTYP_I4, 1);
    var_list_set(svl, "nsamp" , &io->nsmp, SP_DTYP_I4, 1);
    mha->sr = io->rate;
    mha->cs = io->cs;
    mha->nsamp = io->nsmp;
    sprintf(msg + strlen(msg),
        "  prepare_io: sr=%.0f cs=%d ns=%d\n", 
        io->rate, io->cs, io->nsmp);
    return (0);
}

void
stream_start(I_O *io, STA *st)
{
    float *x, *y;
    int i, j, m, n, cs, nk, w;
    double t1, t2;

    process_chunk = st->proc;
    if (io->ofn) {
        sp_tic();
        // initialize i/o pointers
        x = io->iwav;
        y = io->owav;
        if (!x || !y) return;
        n = io->nwav;
        m = io->nrep;
        cs = st->cs;        // chunk size
        nk = n / cs;        // number of chunks
        for (j = 0; j < m; j++) {
            for (i = 0; i < nk; i++) {
                process_chunk(st->cp, x + i * cs, y + i * cs, cs);
            }
        }
        t1 = sp_toc();
        t2 = (io->nwav / io->rate) * io->nrep;
        if (t1 > 0) {
            printf("%12s: (wave_time/wall_time) = ", "speed_ratio");
            printf("(%.3g/%.3g) = %.3g\n", t2, t1, t2 / t1);
        }
    } else {
        w = io->wait;       // wait time
        while (get_aud(io)) {
            put_aud(io, st);
            msleep(w);
        }
    }
}

//===========================================================

void
stream_init(MHA *mha)
{
    if (svl == NULL) {
        svl = (VAR *) calloc(MAX_VAR, sizeof(VAR));
    }
    // initialize stream parameters
    var_list_add(svl, "srate"  , &mha->sr     , 1, SP_DTYP_F8, &nsv);
    var_list_add(svl, "chunk"  , &mha->cs     , 1, SP_DTYP_I4, &nsv);
    var_list_add(svl, "device" , &mha->io_dev , 1, SP_DTYP_I4, &nsv);
    var_list_add(svl, "io_wait", &mha->io_wait, 1, SP_DTYP_I4, &nsv);
    var_list_add(svl, "repeat" , &mha->nreps  , 1, SP_DTYP_I4, &nsv);
    var_list_add(svl, "nich"   , &mha->nich   , 1, SP_DTYP_I4, &nsv);
    var_list_add(svl, "noch"   , &mha->noch   , 1, SP_DTYP_I4, &nsv);
    var_list_add(svl, "process", mha->process , 1, SP_DTYP_U1, &nsv);
    var_list_add(svl, "input"  , mha->input   , 1, SP_DTYP_U1, &nsv);
    var_list_add(svl, "output" , mha->output  , 1, SP_DTYP_U1, &nsv);
    var_list_add(svl, "spl_ref", &mha->spl_ref, 1, SP_DTYP_F8, &nsv);
    var_list_add(svl, "rms_lev", &mha->rms_lev, 1, SP_DTYP_F8, &nsv);
    // initialize I/O
    mha->io_dev = device_init(0);
}

int
stream_device(int io_dev)
{
    char name[ARSC_NAMLEN];
    int n, i;

    io_dev = device_init(io_dev);
    n = ar_num_devs();
    for (i = 0; i < n; i++) {
        if (ar_dev_name(i, name, ARSC_NAMLEN)) {
            break;
        }
        printf("%s %d = %s\n", (i == (io_dev - 1)) ? ">" : " ", i + 1, name);
    }
    return (io_dev);
}

void
stream_replace(MHA *mha, VAR *lvl)
{
    int i;

    var_list_replace(svl, lvl);
    var_list_get(svl, "srate"  ,  &mha->sr     , SP_DTYP_F8, 1);
    var_list_get(svl, "chunk"  ,  &mha->cs     , SP_DTYP_I4, 1);
    var_list_get(svl, "nich"   ,  &mha->nich   , SP_DTYP_I4, 1);
    var_list_get(svl, "noch"   ,  &mha->noch   , SP_DTYP_I4, 1);
    var_list_get(svl, "device" ,  &mha->io_dev , SP_DTYP_I4, 1);
    var_list_get(svl, "io_wait",  &mha->io_wait, SP_DTYP_I4, 1);
    var_list_get(svl, "repeat" ,  &mha->nreps  , SP_DTYP_I4, 1);
    var_list_get(svl, "spl_ref",  &mha->spl_ref, SP_DTYP_F8, 1);
    var_list_get(svl, "rms_ref",  &mha->rms_lev, SP_DTYP_F8, 1);
    i = var_list_index(svl, "process");
    mha->process = svl[i].data;            // processor name
}

void
stream_cleanup()
{
    var_list_cleanup(svl);
}

void
stream_show(char* line)
{
    var_list_show(svl, line);
}

char *
stream_version()
{
    return ar_version();
}

//===========================================================

static void
tone(float *s, int n, double a, double f, double rate)
{
    double p;
    int i;

    p = 2 * M_PI * f / rate;
    for (i = 0; i < n; i++) {
	s[i] = (float) (a * sin(p * i));
    }
}

static void
ramp(float *b, int n, double nr)
{
    double  s, a, p;
    int     i, j, m;

    p = M_PI / (2 * nr) ;
    m = (int) ceil(nr);
    for (i = 0; i < m; i++) {
	s = sin(p * i);
	a = s * s;
	j = n - 1 - i;
	b[i] *= (float) a;
	b[j] *= (float) a;
    }
}

void
sound_check(double rate, int io_dev)
{
    char msg[MAX_MSG];
    double vfs;
    float *s, *z;
    int i, j, np, cseg, err;
    int32_t siz[4];
    void *out[8];
    static double ft = 1000;
    static double rd = 0.010;
    static double td = 0.500;
    static double vo = 1.00;	// volts out
    static int nchn = 2;
    static int nseg = 4;
    static int nswp = 1;
    static int lseg = 1;
    static int32_t fmt[2] = {ARSC_DATA_F4, 0};

    vfs = 2.666;
    np = round((td + rd) * rate);
    s = (float *) calloc(np, sizeof(float));
    z = (float *) calloc(np, sizeof(float));
    tone(s, np, vo / vfs, ft, rate);
    ramp(s, np, rd * rate);
    err = ar_out_open(io_dev, rate, nchn);
    if (err) {
	ar_err_msg(err, msg, MAX_MSG);
        fprintf(stderr, "ERROR: %s, io_dev=%d\n", msg, io_dev);
	free(s);
	free(z);
	return;
    }
    ar_set_fmt(io_dev, fmt);
    printf("output: ");
    fflush(stdout);
    for (i = 0; i < nseg; i++) {
        siz[i] = np;
	for (j = 0; j < nchn; j++) {
            out[i * nchn + j] = ((j * 2) == i) ? s : z;
	}
    }
    ar_out_prepare(io_dev, out, siz, nseg, nswp);
    ar_io_start(io_dev);
    cseg = lseg = -1;
    while(cseg < nseg) {
	if (cseg == lseg) {
	    msleep(10);
	} else {
	    if ((cseg % 2) == 0) {
		printf("%d___", cseg / 2 + 1);
            } else {
		printf("    ");
	    }
            fflush(stdout);
	    lseg = cseg;
	}
	cseg = ar_io_cur_seg(io_dev);
    }
    printf(".\n");
    fflush(stdout);
    ar_io_close(io_dev);
    free(s);
    free(z);
    strcpy(msg, ".\n");
}

void
play_wave(I_O *io, int o)
{
    char msg[MAX_MSG];
    int j, cseg, err;
    int32_t siz[4];
    void *out[8];
    static int nchn = 2;
    static int nseg = 1;
    static int nswp = 1;
    static int lseg = 1;
    static int32_t fmt[2] = {ARSC_DATA_F4, 0};

    if (o ? !io->ofn : !io->ifn) {
        fprintf(stderr, "ERROR: undefined %sput file\n", o ? "out" : "in");
	return;
    }
    err = ar_out_open(io->iod, io->rate, nchn);
    if (err) {
	ar_err_msg(err, msg, MAX_MSG);
        fprintf(stderr, "ERROR: %s\n", msg);
	return;
    }
    ar_set_fmt(io->iod, fmt);
    printf("play: %s", o ? io->ofn : io->ifn);
    fflush(stdout);
    siz[0] = io->nwav;
    for (j = 0; j < nchn; j++) {
        out[j] = o ? io->owav : io->iwav;
    }
    ar_out_prepare(io->iod, out, siz, nseg, nswp);
    ar_io_start(io->iod);
    cseg = lseg = -1;
    while(cseg < nseg) {
	if (cseg == lseg) {
	    msleep(80);
	}
	cseg = ar_io_cur_seg(io->iod);
    }
    printf("\n");
    fflush(stdout);
    ar_io_close(io->iod);
    strcpy(msg, ".\n");
}

/***********************************************************/

