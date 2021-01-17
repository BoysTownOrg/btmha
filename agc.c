// agc.c - CHAPRO/GHA plugin example for BTMHA

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <chapro.h>
#include "plugin_helper.h"

//===========================================================

static int icf = 0;
static int32_t nchan[2] = {1,1};
static CHA_DSL dsl = {0};
static CHA_WDRC agc = {0};

//===========================================================

static void
process0(void **cp, float *x, float *y, int cs)
{
    cha_agc_input(cp, x, y, cs);
}

static void
process1(void **cp, float *x, float *y, int cs)
{
    cha_agc_channel(cp, x, y, cs);
}

static void
process2(void **cp, float *x, float *y, int cs)
{
    cha_agc_output(cp, x, y, cs);
}

//===========================================================

// prepare compressor

static void
prepare_compressor(CHA_PTR cp, int cs)
{
    cha_chunk_size(cp, cs);
    cha_agc_prepare(cp, &dsl, &agc);
}

static void
prepare(CHA_STA *state, void *v, MHA *mha, int entry)
{
    int nc;
    void **cp;

    var_transfer(v);
    cp = calloc(NPTR, sizeof(void *));
    cha_prepare(cp);
    prepare_compressor(cp, mha->cs);
    cha_state_save(cp, state);
    cha_cleanup(cp);
    nc = pvl[icf].cols + 1;
    if (entry == 0) {
        state->proc = process0;
	nchan[0] = 1;
        nchan[1] = 1;
    } else if (entry == 1) {
        state->proc = process1;
        nchan[0] = nc;
	nchan[1] = nc;
    } else if (entry == 2) {
        state->proc = process2;
        nchan[0] = 1;
	nchan[1] = 1;
    } else {
        state->proc = NULL;
	nchan[0] = 0;
        nchan[1] = 0;
    }
    state->type = 0;       // state type is "plugin"
    state->entry = entry;  // index of entry point
    state->rprt = NULL;
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
}

static void
configure(void *v, int *nv)
{
    int nc;

    // initialize local variables
    configure_compressor();
    // specify configurable parameters
    nc = dsl.nchannel;
    var_list_init(v);
    put_list_i4a("nchan"        ,          nchan,    2, 0); // read only
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
    var_list_return(nv);
}

void
agc_bind(PLUG* plugin)
{
    plugin->configure = configure;
    plugin->prepare = prepare;
}


