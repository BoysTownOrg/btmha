// ciirfb.c - CHAPRO/CIIRFB plugin example for BTMHA

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <chapro.h>
#include "plugin_helper.h"

//===========================================================

static double cross_freq[MAX_CHN] = {
  0.25, 0.42, 0.58, 0.75, 0.92, 1.12, 1.41, 1.78,
  2.24, 2.83, 3.56, 4.49, 5.66, 7.13, 8.98};
static int nchannel = 16;
static int icf = 0;
static int32_t nchan[2] = {1,1};

//===========================================================

static void
process0(void **cp, float *x, float *y, int cs)
{
    cha_ciirfb_analyze(cp, x, y, cs);
}

static void
process1(void **cp, float *x, float *y, int cs)
{
    cha_ciirfb_synthesize(cp, x, y, cs);
}

//===========================================================

// determine filterbank center frequencies and bandwidths

static double
cgtfb_init(double *fc, double *bw, double *cf, int nc, double sr)
{
    double lfbw, smbw, fmax, fmid = 1000;
    int i;

    fmax = sr / 2;
    for (i = 1; i < (nc - 1); i++) {
        fc[i] = 1000 * (cf[i] + cf[i - 1]) / 2;
        bw[i] = 1000 * (cf[i] - cf[i - 1]);
    }
    fc[nc - 1] = (fmax + 1000 * cf[nc - 2]) / 2;
    bw[nc - 1] = fmax - 1000 * cf[nc - 2];
    smbw = 0;
    for (i = 1; i < nc; i++) {
	smbw += bw[i];
	if (fc[i] >= fmid) break;
    }
    lfbw = smbw / i;
    fc[0] = bw[0] = lfbw;

    return (400 / lfbw);
}

//===========================================================

// prepare CIIR filterbank

static void
prepare_filterbank_ciir(CHA_PTR cp, double sr, int cs)
{
    double gd, bw[MAX_CHN], fc[MAX_CHN], *cf;
    float z[256], p[256], g[64]; 
    int nc, d[32];
    static int no =  4;      // gammatone filter order

    nc = pvl[icf].cols + 1;
    cf = cross_freq;
    gd = cgtfb_init(fc, bw, cf, nc, sr);
    cha_ciirfb_design(z, p, g, d, nc, fc, bw, sr, gd);
    cha_ciirfb_prepare(cp, z, p, g, d, nc, no, sr, cs);
}

static void
prepare(CHA_STA *state, void *v, MHA *mha, int entry)
{
    void **cp;

    var_transfer(v);
    cp = calloc(NPTR, sizeof(void *));
    cha_prepare(cp);
    prepare_filterbank_ciir(cp, mha->sr, mha->cs);
    cha_state_save(cp, state);
    cha_cleanup(cp);
    nchannel = pvl[icf].cols + 1;
    if (entry == 0) {
        state->proc = process0;
	nchan[0] = 1;
        nchan[1] = nchannel * 2;
    } else if (entry == 1) {
        state->proc = process1;
        nchan[0] = nchannel * 2;
	nchan[1] = 1;
    } else {
        state->proc = NULL;
	nchan[0] = 0;
        nchan[1] = 0;
    }
    state->type = 0;       // state type is "plugin"
    state->rprt = NULL;
    state->entry = entry;  // index of entry point
}

//===========================================================

static void
configure(void *v, int *nv)
{
    int ncf;

    // specify configurable parameters
    ncf = nchannel - 1;
    var_list_init(v);
    put_list_i4a("nchan"        ,        nchan,   2, 0); // read only
    put_list_f8a("cross_freq"   ,   cross_freq, ncf, MAX_CHN);
    icf = npv - 1; // index to cross_freq
    var_list_return(nv);
}

void
ciirfb_bind(PLUG* plugin)
{
    plugin->configure = configure;
    plugin->prepare = prepare;
}

