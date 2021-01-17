// mha_gha.c - GHA plugin for BTMHA

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <chapro.h>
#include "btmha.h"

//===========================================================

static char *plugin_name = "mha_gha";
static void *cp[NPTR] = {0};
static CHA_AFC afc = {0};
static CHA_DSL dsl = {0};
static CHA_WDRC agc = {0};

//===========================================================

int
process(CHA_STA *state, char *msg)
{
    if (msg) sprintf(msg, "%s: process\n", plugin_name);
    return (1);
}

//===========================================================

// prepare IIR filterbank

static void
prepare_filterbank(CHA_PTR cp, double sr, int cs)
{
    double td, *cf;
    int nc, nz;
    // zeros, poles, gains, & delays
    float   z[64], p[64], g[8];
    int     d[8];

    // prepare IIRFB
    nc = dsl.nchannel;
    cf = dsl.cross_freq;
    nz = agc.nz;
    td = agc.td;
    cha_iirfb_design(z, p, g, d, cf, nc, nz, sr, td);
    cha_iirfb_prepare(cp, z, p, g, d, nc, nz, sr, cs);
}

// prepare AGC compressor

static void
prepare_compressor(CHA_PTR cp)
{
    // prepare AGC
    cha_agc_prepare(cp, &dsl, &agc);
}

// prepare feedback

static void
prepare_feedback(CHA_PTR cp, int n)
{
    afc.nqm = n;          // initialize quality metric length
    // prepare AFC
    cha_afc_prepare(cp, &afc);
}

int
prepare(CHA_STA *state, void *v, MHA *mha, char *msg)
{
    prepare_filterbank(cp, mha->sr, mha->cs);
    prepare_compressor(cp);
    prepare_feedback(cp, mha->nsamp);
    cha_state(cp, state);
    sprintf(msg, "%s: prepare\n", plugin_name);

    return (1);
}

//===========================================================

static void
configure_compressor()
{
    // DSL prescription example
    static CHA_DSL dsl_ex = {5, 50, 119, 0, 8,
        {317.1666,502.9734,797.6319,1264.9,2005.9,3181.1,5044.7},
        {-13.5942,-16.5909,-3.7978,6.6176,11.3050,23.7183,35.8586,37.3885},
        {0.7,0.9,1,1.1,1.2,1.4,1.6,1.7},
        {32.2,26.5,26.7,26.7,29.8,33.6,34.3,32.7},
        {78.7667,88.2,90.7,92.8333,98.2,103.3,101.9,99.8}
    };
    static CHA_WDRC agc_ex = {1, 50, 24000, 119, 0, 105, 10, 105};
    static int    nz = 4;
    static double td = 2.5;

    memcpy(&dsl, &dsl_ex, sizeof(CHA_DSL));
    memcpy(&agc, &agc_ex, sizeof(CHA_WDRC));
    agc.nz = nz;
    agc.td = td;
}

static void
configure_feedback()
{
    // AFC parameters
    afc.rho  = 0.0011907; // forgetting factor
    afc.eps  = 0.0010123; // power threshold
    afc.mu   = 0.0001504; // step size
    afc.afl  = 100;       // adaptive filter length
    afc.wfl  = 0;         // whitening-filter length
    afc.pfl  = 0;         // persistent-filter length
    afc.hdel = 0;         // output/input hardware delay
    afc.sqm  = 1;         // save quality metric ?
    afc.fbg  = 1;         // simulated-feedback gain 
}

int
configure(void *v, int *nv)
{
    if (!quiet) fprintf(stderr, "%s: configure\n", plugin_name);
    // initialize local variables
    configure_compressor();
    configure_feedback();
    // specify configurable parameters
    var_list_init(v);
    put_f8n("afc_rho"      , afc.rho);
    put_f8n("afc_mu"       , afc.mu );
    put_f8n("afc_eps"      , afc.eps);
    put_i4n("afc_sqm"      , afc.sqm);
    var_list_return(nv);

    return (1);
}

