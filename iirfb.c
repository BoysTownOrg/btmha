// iirfb.c - CHAPRO/IIRFB plugin example for BTMHA

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <chapro.h>
#include "plugin_helper.h"

//===========================================================

static double cross_freq[MAX_CHN] = {
    317.1666,502.9734,797.6319,1264.9,2005.9,3181.1,5044.7};
static int nchannel = 8;
static int icf = 0;
static int32_t nchan[2] = {1,1};

//===========================================================

static void
process0(void **cp, float *x, float *y, int cs)
{
    cha_iirfb_analyze(cp, x, y, cs);
}

static void
process1(void **cp, float *x, float *y, int cs)
{
    cha_iirfb_synthesize(cp, x, y, cs);
}

//===========================================================

// prepare IIR filterbank

static void
prepare_filterbank_iir(CHA_PTR cp, double sr, int cs)
{
    double td, *cf;
    int nc, nz;
    // zeros, poles, gains, & delays
    float   z[64], p[64], g[8];
    int     d[8];

    nc = pvl[icf].cols + 1;
    cf = cross_freq;
    nz = 4;
    td = 2.5;
    cha_iirfb_design(z, p, g, d, cf, nc, nz, sr, td);
    cha_iirfb_prepare(cp, z, p, g, d, nc, nz, sr, cs);
}

static void
prepare(CHA_STA *state, void *v, MHA *mha, int entry)
{
    void **cp;

    var_transfer(v);
    cp = calloc(NPTR, sizeof(void *));
    cha_prepare(cp);
    prepare_filterbank_iir(cp, mha->sr, mha->cs);
    cha_state_save(cp, state);
    cha_cleanup(cp);
    nchannel = pvl[icf].cols + 1;
    if (entry == 0) {
        state->proc = process0;
	nchan[0] = 1;
        nchan[1] = nchannel;
    } else if (entry == 1) {
        state->proc = process1;
        nchan[0] = nchannel;
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
    ncf = nchannel - 1; // number of crossover frequencies
    var_list_init(v);
    put_list_i4a("nchan"        ,        nchan,   2, 0); // read only
    put_list_f8a("cross_freq"   ,   cross_freq, ncf, MAX_CHN);
    icf = npv - 1; // index to cross_freq
    var_list_return(nv);
}
void
iirfb_bind(PLUG* plugin)
{
    plugin->configure = configure;
    plugin->prepare = prepare;
}

