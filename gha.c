// gha.c - CHAPRO/GHA plugin example for BTMHA

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <chapro.h>
#include <sigpro.h>
#include "plugin_helper.h"

//===========================================================

static char *plugin_name = "gha";
static char ifn[MAX_NAM] = "";
static char ofn[MAX_NAM] = "";
static char dfn[MAX_NAM] = "";
static int configured = 0;
static int prepared = 0;
static int processed = 0;
static int afc_manage = 1;
static int fir_filter = 0;
static int icf = 0;
static int32_t nchan[2] = {1,1};
static CHA_AFC afc = {0};
static CHA_DSL dsl = {0};
static CHA_WDRC agc = {0};

//===========================================================

static void
process(void **cp, float *x, float *y, int cs)
{
    float *z = (float *) cp[_cc];
    cha_afc_filters(cp, &afc); // update pointers to AFC filters
    // process IIR+AGC+AFC
    cha_afc_input(cp, x, x, cs);
    cha_agc_input(cp, x, x, cs);
    if (fir_filter) {
        cha_firfb_analyze(cp, x, z, cs);
        cha_agc_channel(cp, z, z, cs);
        cha_firfb_synthesize(cp, z, y, cs);
    } else {
        cha_iirfb_analyze(cp, x, z, cs);
        cha_agc_channel(cp, z, z, cs);
        cha_iirfb_synthesize(cp, z, y, cs);
    }
    cha_agc_output(cp, y, y, cs);
    cha_afc_output(cp, y, cs);
    processed++;
}

//===========================================================

static void
write_qm(char *msg)
{
    float *meer;
    static VAR *vl;

    if (*dfn) {
        sprintf(msg, "%12s: %s\n", "MAT output", dfn);
        meer = afc.qm ? afc.qm : (float *) calloc(sizeof(float), afc.nqm);
        vl = sp_var_alloc(8);
        sp_var_add(vl, "merr",     meer, afc.nqm, 1, "f4");
        sp_var_add(vl, "sfbp", afc.sfbp, afc.fbl, 1, "f4");
        sp_var_add(vl, "efbp", afc.efbp, afc.afl, 1, "f4");
        sp_var_add(vl, "wfrp", afc.wfrp, afc.wfl, 1, "f4");
        sp_var_add(vl, "ffrp", afc.ffrp, afc.pfl, 1, "f4");
        sp_var_add(vl, "ifn" ,      ifn,       1, 1, "f4str");
        sp_var_add(vl, "ofn" ,      ofn,       1, 1, "f4str");
        remove(dfn);
        sp_mat_save(dfn, vl);
        sp_var_clear(vl);
        if (!afc.qm && meer) free(meer);
    }
}

static void
report_qm(char *msg)
{
    int iqm, msz;
    double fme;

    if (!afc_manage) {
	msg[0] = 0;
	return;
    }
    iqm = afc.iqmp ? afc.iqmp[0] : 0;
    sprintf(msg, "report_qm: sqm=%d nqm=%d iqm=%d\n", afc.sqm, afc.nqm, iqm);
    if (iqm) {
        write_qm(msg);
        if (afc.qm[iqm - 1] > 0) {
	    fme = 10 * log10(afc.qm[iqm - 1]);
	    msz = strlen(msg);
	    sprintf(msg + msz, "final misalignment error = %.2f dB\n", fme);
        }
    }
}

static void
report(void **cp, char *msg)
{
    if (!configured) {
        strcpy(msg, "");
    } else if (processed) {
        if (afc.sqm) report_qm(msg);
    } else if (prepared) {
        sprintf(msg, "%12s: %s+AGC%s\n", plugin_name, fir_filter ? "FIR" : "IIR", 
            afc.afl ? "+AFC" : "");
    } else if (configured) {
        sprintf(msg, "%12s: %s\n", plugin_name, "configured");
    } else if (configured) {
        sprintf(msg, "%12s: %s\n", plugin_name, "unknown state");
    }
}

//===========================================================

// prepare FIR filterbank

static void
prepare_filterbank_fir(CHA_PTR cp, double sr, int cs)
{
    double *cf;
    int nc, nw, wt;

    if (agc.nw == 0) {    // FIR
        agc.nw = 256;     // window size
    }
    // prepare FIRFB
    nc = pvl[icf].cols + 1;
    nc = dsl.nchannel;
    cf = dsl.cross_freq;
    nw = agc.nw;
    wt = agc.wt;
    cha_firfb_prepare(cp, cf, nc, sr, nw, wt, cs);
}

// prepare IIR filterbank

static void
prepare_filterbank_iir(CHA_PTR cp, double sr, int cs)
{
    double td, *cf;
    int nc, nz;
    // zeros, poles, gains, & delays
    float   z[64], p[64], g[8];
    int     d[8];

    // prepare IIRFB
    nc = pvl[icf].cols + 1;
    cf = dsl.cross_freq;
    nz = agc.nz;
    td = agc.td;
    cha_iirfb_design(z, p, g, d, cf, nc, nz, sr, td);
    cha_iirfb_prepare(cp, z, p, g, d, nc, nz, sr, cs);
}

// prepare compressor

static void
prepare_compressor(CHA_PTR cp, int cs)
{
    cha_chunk_size(cp, cs);
    cha_agc_prepare(cp, &dsl, &agc);
}

// prepare feedback

static void
prepare_feedback(CHA_PTR cp, int ns)
{
    if (!afc_manage) {
	afc.fbg = 0;
	afc.afl = 0;
	afc.nqm = 0;
    } else if (afc.fbg == 0) {
	afc.sqm = 0;
    } else if (afc.sqm) {
	afc.nqm = ns;
    }
    cha_afc_prepare(cp, &afc);
}

static void
prepare(CHA_STA *state, void *v, MHA *mha, int entry)
{
    void **cp;

    var_transfer(v);
    cp = calloc(NPTR, sizeof(void *));
    cha_prepare(cp);
    if (fir_filter) {
        prepare_filterbank_fir(cp, mha->sr, mha->cs);
    } else {
        prepare_filterbank_iir(cp, mha->sr, mha->cs);
    }
    prepare_compressor(cp, mha->cs);
    prepare_feedback(cp, mha->nsamp);
    cha_state_save(cp, state);
    cha_cleanup(cp);
    state->type = 0;       // state type is "plugin"
    state->proc = process; // pointer to process function
    state->rprt = report;  // pointer to report function
    state->entry = entry;  // index of entry point
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

    memcpy(&dsl, &dsl_ex, sizeof(CHA_DSL));
    memcpy(&agc, &agc_ex, sizeof(CHA_WDRC));
    // IIR filterbank
    agc.nz = 4;
    agc.td = 2.5;
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
    afc.fbg = 1;        // simulated-feedback gain 
    afc.nqm = 0;        // initialize quality-metric length
}

static void
configure(void *v, int *nv)
{
    int nc;

    // initialize local variables
    configure_compressor();
    configure_feedback();
    // specify configurable parameters
    *ifn = 0;
    *ofn = 0;
    *dfn = 0;
    nc = dsl.nchannel;
    var_list_init(v);
    put_list_i4a("nchan"        ,          nchan,    2, 0); // read only
    put_list_i4n("afc"          ,    &afc_manage);
    put_list_i4n("fir"          ,    &fir_filter);
    put_list_i4n("afc_afl"      ,       &afc.afl);
    put_list_i4n("afc_sqm"      ,       &afc.sqm);
    put_list_f8o("afc_rho"      ,       &afc.rho, _rho);
    put_list_f8o("afc_eps"      ,       &afc.eps, _eps);
    put_list_f8o("afc_mu"       ,        &afc.mu, _mu );
    put_list_f8n("afc_fbg"      ,       &afc.fbg);
    put_list_f8n("afc_hdel"     ,      &afc.hdel);
    put_list_f8n("agc_attack"   ,    &agc.attack);
    put_list_f8n("agc_release"  ,   &agc.release);
    put_list_f8n("agc_maxdB"    ,     &agc.maxdB);
    put_list_f8n("agc_tkgain"   ,    &agc.tkgain);
    put_list_f8n("agc_tk"       ,        &agc.tk);
    put_list_f8n("agc_cr"       ,        &agc.cr);
    put_list_f8n("agc_bolt"     ,      &agc.bolt);
    put_list_i4n("dsl_ear"      ,       &dsl.ear);
    put_list_f8n("dsl_attack"   ,    &dsl.attack);
    put_list_f8n("dsl_release"  ,   &dsl.release);
    put_list_f8n("dsl_maxdB"    ,     &dsl.maxdB);
    put_list_f8a("dsl_tkgain"   ,     dsl.tkgain,   nc, DSL_MXCH);
    put_list_f8a("dsl_tk"       ,         dsl.tk,   nc, DSL_MXCH);
    put_list_f8a("dsl_cr"       ,         dsl.cr,   nc, DSL_MXCH);
    put_list_f8a("dsl_bolt"     ,       dsl.bolt,   nc, DSL_MXCH);
    put_list_f8a("cross_freq"   , dsl.cross_freq, nc-1, DSL_MXCH);
    icf = npv - 1; // index to cross_freq
    put_list_u1a("input"        ,            ifn);
    put_list_u1a("output"       ,            ofn);
    put_list_u1a("datfile"      ,            dfn);
    var_list_return(nv);
    processed = prepared = 0;
    configured++;
}

void
gha_bind(PLUG *plugin)
{
    plugin->configure = configure;
    plugin->prepare   = prepare;
}
