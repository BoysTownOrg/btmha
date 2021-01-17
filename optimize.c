// optimize.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <sigpro.h>
#include "btmha.h"

#ifdef WIN32
#include <io.h>
#ifndef R_OK
#define R_OK            4
#endif
#else
#include <unistd.h>
#define _access         access
#define _strdup         strdup
#endif

// CHAPRO offset definitions
#define _offset   4
#define _qm       _offset+36
#define _iqmp     _offset+37
#define _nqm      9
#define _mu       10
#define _rho      11
#define _eps      12

//===========================================================

static double *dopt[4];
static int oopt[4];
static int nopt = 0;

void
opt_add(STA *st, int o)
{
    double *dvar;
    void **cp;

    if (o > 0) {
        cp = st->cp;
        dvar =  (double *) cp[_dvar];
        dopt[nopt] = dvar + o;
        oopt[nopt] = o;
        nopt++;
    }
}

//===========================================================

static void (*process_chunk)(void **, float *x, float *y, int);

static char dbg = 0;
static char prn = 0;
static double tqm = 8; // seconds
static double fme = 0;
static float *iwav, *owav, *qm;
static int32_t iqm = 0;
static int32_t jqm = 0;
static int32_t nqm = 0;
static I_O *io;

// process state
static void
proc_sta(I_O *io, STA *st)
{
    float *x, *y;
    int i, j, m, n, cs, nk;

    if (io->ofn) {
        // initialize i/o pointers
        x = iwav;
        y = owav;
        n = io->nwav;
        m = io->nrep;
        cs = st->cs;        // chunk size
        nk = n / cs;        // number of chunks
        for (j = 0; j < m; j++) {
            for (i = 0; i < nk; i++) {
                process_chunk(st->cp, x + i * cs, y + i * cs, cs);
            }
        }
    }
}

double
afc_error(float *par, void *v)
{
    double mxqm, err, sr, *dvar;
    int i;
    int32_t *ivar;
    void **cp;
    STA sta;

    // check parameter limits
    for (i = 0; i < nopt; i++) {
        if (par[i] <= 0) return (1e9);
    }
    // create local copy of state
    state_copy(&sta, (STA *)v);
    cp = sta.cp;
    sr = sta.sr;
    memcpy(iwav, io->iwav, io->nwav * sizeof(float));
    memcpy(owav, io->owav, io->nwav * sizeof(float));
    // modify AFC parameters
    dvar =  (double *) cp[_dvar];
    for (i = 0; i < nopt; i++) {
        if (oopt[i] > 0) dvar[oopt[i]] = par[i];
    }
    ivar = (int32_t *) cp[_ivar];
    nqm = ivar[_nqm];
    proc_sta(io, &sta); // process state
    qm = (float *) cp[_qm];
    ivar = (int32_t *) cp[_iqmp];
    if (!qm || !ivar) return (1e9);
    // report error
    iqm = ivar[0];
    jqm = round(sr * tqm);
    mxqm = 0;
    for (i = jqm; i < iqm; i++) {
        if (mxqm < qm[i]) {
            mxqm = qm[i];
        }
    }
    err = (mxqm < 1e-9) ? -900 : 10 * log10(mxqm);
    if (iqm > 1) fme = 10 * log10(qm[iqm - 1]);
    if (dbg) {
        printf("afc: %8.6f %8.6f %8.6f %6.2f\n", par[0], par[1], par[2], err);
    } else if (prn) {
        printf( "maximum error after %.3g sec = %.2f dB\n", tqm, err);
    } else {
        fputc('.', stderr);
    }
    state_free(&sta);
    return (err);
}

void
print_par(float *par, VAR *vl, int nv, char *lab)
{
    int i;

    fprintf(stdout, "    # %s parameters\n", lab);
    for (i = 0; i < nv; i++) {
        fprintf(stdout, "    %-8s  = %9.7f\n", vl[i].name, par[i]);
    }
}

void
optimize_afc(I_O *iop, STA *stp, VAR *vl, int nv, double t, char *msg)
{
    float par0[3], par[3];
    int i;

    if (stp->type != STYP_PLG) {
        printf("ERROR: optimization requires direct plugin connection.\n");
        return;
    }
    io = iop;
    if (t > 0) tqm = t;
    process_chunk = stp->proc;
    iwav = (float *) calloc(io->nwav, sizeof(float));
    owav = (float *) calloc(io->nwav, sizeof(float));
    // specify target variables
    nopt = 0;
    for (i = 0; i < nv; i++) {
        opt_add(stp, vl[i].last);
    }
    for (i = 0; i < nopt; i++) {
        par0[i] = par[i] = (float)(*dopt[i]);
    }
    // initial report
    print_par(par0, vl, nopt, "initial");
    afc_error(par0, stp);
    // optimize
    prn = 0;
    sp_fminsearch(par,  nopt, &afc_error, NULL, stp);
    fputc('\n', stderr);
    prn = 1;
    // final report
    if (dbg) { // repeat initial parameters in dbug mode
        print_par(par0, vl, nopt, "initial");
        afc_error(par0, stp);
    }
    print_par(par, vl, nopt, "optimized");
    afc_error(par, stp);
    free(iwav);
    free(owav);
    sprintf(msg, " final misalignment error = %.2f dB\n", fme);
}

