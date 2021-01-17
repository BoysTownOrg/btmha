// afc.c - CHAPRO/GHA plugin example for BTMHA

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <chapro.h>
#include <sigpro.h>
#include "plugin_helper.h"

//===========================================================

static char *plugin_name = "afc";
static char ifn[MAX_NAM] = "";
static char ofn[MAX_NAM] = "";
static char dfn[MAX_NAM] = "";
static int configured = 0;
static int prepared = 0;
static int processed = 0;
static int afc_manage = 1;
static int fir_filter = 0;
static int32_t nchan[2] = {1,1};
static CHA_AFC afc = {0};

//===========================================================

static void
process0(void **cp, float *x, float *y, int cs)
{
    cha_afc_filters(cp, &afc); // update pointers to AFC filters
    cha_afc_input(cp, x, y, cs);
    processed++;
}

void
process1(void **cp, float *x, float *y, int cs)
{
    cha_afc_output(cp, y, cs);
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
        sprintf(msg, "%12s: %s=%d\n", plugin_name, "processed", processed);
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
    prepare_feedback(cp, mha->nsamp);
    cha_state_save(cp, state);
    cha_cleanup(cp);
    if (entry == 0) {
        state->proc = process0;
        state->rprt = report;  // pointer to report function
    } else {
        state->proc = process1;
        state->rprt = NULL;
    }
    state->type = 0;       // state type is "plugin"
    state->entry = entry;  // index of entry point
}

//===========================================================

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
    // initialize local variables
    configure_feedback();
    // specify configurable parameters
    *ifn = 0;
    *ofn = 0;
    *dfn = 0;
    var_list_init(v);
    put_list_i4a("nchan"        ,          nchan,    2, 0); // read only
    put_list_u1a("input"        ,            ifn);
    put_list_u1a("output"       ,            ofn);
    put_list_u1a("datfile"      ,            dfn);
    put_list_i4n("afc_afl"      ,       &afc.afl);
    put_list_i4n("afc_sqm"      ,       &afc.sqm);
    put_list_f8o("afc_rho"      ,       &afc.rho, _rho);
    put_list_f8o("afc_eps"      ,       &afc.eps, _eps);
    put_list_f8o("afc_mu"       ,        &afc.mu, _mu );
    put_list_f8n("afc_fbg"      ,       &afc.fbg);
    put_list_f8n("afc_hdel"     ,      &afc.hdel);
    var_list_return(nv);
    processed = prepared = 0;
    configured++;
}

void
afc_bind(PLUG *plugin)
{
    plugin->configure = configure;
    plugin->prepare   = prepare;
}
