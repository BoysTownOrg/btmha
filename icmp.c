// icmp.c - CHAPRO/ICMP plugin example for BTMHA

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <chapro.h>
#include "plugin_helper.h"

//===========================================================

static int icf = 0;
static int32_t nchan[2] = {1,1};
static CHA_CLS cls = {0};
static CHA_ICMP icmp = {0};

//===========================================================

static void
process(void **cp, float *x, float *y, int cs)
{
    cha_icmp_process(cp, x, y, cs);
}

/***********************************************************/

// specify filterbank crossover frequencies

static int
cross_freq(double *cf, double sr)
{
    int i, nh, nc, nm = 5;
    double fmin = 250, fmid = 1000, bpo = 3;

    nh = (int) floor(log2((float)sr / 2000) * bpo);
    nc = nh + nm;
    for (i = 0; i < nm; i++) {
        cf[i] = fmin + i * (fmid - fmin)  / (nm - 0.5);
    }
    for (i = 0; i < nh; i++) {
        cf[i + nm] = fmid * pow(2.0, (i + 0.5) / bpo);
    }

    return (nc + 1); // return number of channels = crossovers + 1
}

// CSL prescription

static void
compressor_init(CHA_CLS *cls, double *cf, double sr, double gn, int nc)
{
    double f1, f2;
    int k, n;

    // set compression mode
    cls->cm = 1;
    // loop over filterbank channel
    cls->nc = nc;
    n = nc - 1;
    for (k = 0; k < nc; k++) {
        cls->Lcs[k] = 0;
        cls->Lcm[k] = 50;
        cls->Lce[k] = 100;
        cls->Lmx[k] = 120;
        cls->Gcs[k] = (float) gn;
        cls->Gcm[k] = (float) gn / 2;
        cls->Gce[k] = 0;
        cls->Gmx[k] = 90;
        f1 = (k > 0) ? cf[k - 1] : 0;
        f2 = (k < n) ? cf[k] : sr / 2;
        cls->bw[k] = f2 - f1;
    }
}

//===========================================================

static void
prepare_compressor(CHA_PTR cp, double sr)
{
    static double lr = 2e-5;    // signal-level reference (Pa)
    static int    ds = 24;      // downsample factor

    icmp.sr = sr;      // sampling rate (Hz)
    cls.nc = cross_freq(cls.fc, icmp.sr);
    compressor_init(&cls, cls.fc, icmp.sr, icmp.gn, cls.nc);
    cha_icmp_prepare(cp, &cls, sr, lr, ds);
}

static void
prepare(CHA_STA *state, void *v, MHA *mha, int entry)
{
    void **cp;

    var_transfer(v);
    cp = calloc(NPTR, sizeof(void *));
    cha_prepare(cp);
    prepare_compressor(cp, mha->sr);
    cha_state_save(cp, state);
    cha_cleanup(cp);
    nchan[0] = cls.nc * 2;
    nchan[1] = cls.nc * 2;
    state->type = 0;       // state type is "plugin"
    state->proc = process; // pointer to process function
    state->rprt = NULL  ;  // pointer to report function
    state->entry = entry;  // index of entry point
}

//===========================================================

static void
configure_compressor()
{
    icmp.gn = 20;      // flat compressor gain (dB)
    icmp.nw = 256;     // window size
    icmp.wt = 0;       // window type: 0=Hamming, 1=Blackman
}

static void
configure(void *v, int *nv)
{
    // initialize local variables
    configure_compressor();
    // specify configurable parameters
    var_list_init(v);
    put_list_i4a("nchan"        ,   nchan,      2,        0); // read only
    put_list_f8a("cross_freq"   , &cls.fc, cls.nc, CLS_MXCH);
    icf = npv - 1; // index to cross_freq
    var_list_return(nv);
}

void
icmp_bind(PLUG* plugin)
{
    plugin->configure = configure;
    plugin->prepare = prepare;
}
