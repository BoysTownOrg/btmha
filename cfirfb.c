// cfirfb.c - CHAPRO/CFIRFB plugin example for BTMHA

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
//static void *cp[NPTR] = {0};

//===========================================================

static void
process0(void **cp, float *x, float *y, int cs)
{
    cha_cfirfb_analyze(cp, x, y, cs);
}

static void
process1(void **cp, float *x, float *y, int cs)
{
    cha_cfirfb_synthesize(cp, x, y, cs);
}

//===========================================================

// prepare CFIR filterbank

static void
prepare_filterbank_cfir(CHA_PTR cp, double sr, int cs)
{
    double *cf;
    int nc, nw, wt;

    nc = pvl[icf].cols + 1;
    cf = cross_freq;
    nw = 256;
    wt = 0;
    cha_cfirfb_prepare(cp, cf, nc, sr, nw, wt, cs);
}

static void
prepare(CHA_STA *state, void *v, MHA *mha, int entry)
{
    void **cp;

    var_transfer(v);
    cp = calloc(NPTR, sizeof(void *));
    cha_prepare(cp);
    nchannel = pvl[icf].cols + 1;
    prepare_filterbank_cfir(cp, mha->sr, mha->cs);
    cha_state_save(cp, state);
    cha_cleanup(cp);
    state->type = 0; // state type is "plugin"
    state->rprt = NULL;
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
cfirfb_bind(PLUG* plugin)
{
    plugin->configure = configure;
    plugin->prepare = prepare;
}

