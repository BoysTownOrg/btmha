// plugin_gha.c - GHA plugin for BTMHA

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <chapro.h>
#include "btmha.h"

//===========================================================

static char *plugin_name = "plugin_gha";
static void *cp[NPTR] = {0};
static int configured = 0;
static int prepared = 0;
static int processed = 0;
static CHA_AFC afc = {0};
static CHA_DSL dsl = {0};
static CHA_WDRC agc = {0};

//===========================================================

void
process(CHA_STA *state, float *x, float *y)
{
    processed++;
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
prepare_feedback(CHA_PTR cp)
{
    // prepare AFC
    cha_afc_prepare(cp, &afc);
}

void
prepare(CHA_STA *state, void *v, MHA *mha)
{
    var_transfer(v);
    prepare_filterbank(cp, mha->sr, mha->cs);
    prepare_compressor(cp);
    if (afc.sqm) afc.nqm = mha->nsamp;
    prepare_feedback(cp);
    cha_state(cp, state);
    processed = 0;
    prepared++;
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
    afc.rho  = 0.0014388; // forgetting factor
    afc.eps  = 0.0010148; // power threshold
    afc.mu   = 0.0001507; // step size
    afc.afl  = 100;     // adaptive filter length
    afc.wfl  = 0;       // whitening-filter length
    afc.pfl  = 0;       // persistent-filter length
    afc.hdel = 0;       // output/input hardware delay
    afc.sqm  = 0;       // save quality metric ?
    afc.fbg = 0;        // simulated-feedback gain 
    afc.nqm = 0;        // initialize quality-metric length
}

void
configure(void *v, int *nv)
{
    // initialize local variables
    configure_compressor();
    configure_feedback();
    // specify configurable parameters
    var_list_init(v);
    put_list_f8n("afc_rho"      , afc.rho);
    put_list_f8n("afc_mu"       , afc.mu );
    put_list_f8n("afc_eps"      , afc.eps);
    put_list_i4n("afc_sqm"      , afc.sqm);
    put_list_f8a("cross_freq"   , dsl.cross_freq, dsl.nchannel-1, DSL_MXCH);
    var_list_return(nv);
    prepared = processed = 0;
    configured++;
}

//===========================================================

void
report(char *msg)
{
    if (!configured) {
        strcpy(msg, "");
    } else if (processed) {
        sprintf(msg, "%s: %s\n", plugin_name, "processed");
    } else if (prepared) {
        sprintf(msg, "%s: %s\n", plugin_name, "prepared");
    } else if (configured) {
        sprintf(msg, "%s: %s\n", plugin_name, "configured");
    } else if (configured) {
        sprintf(msg, "%s: %s\n", plugin_name, "unknown state");
    }
}

