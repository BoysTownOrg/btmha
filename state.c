// state.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "btmha.h"

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#define free_null(x)    if(x){free(x);x=NULL;}

//===========================================================

static void
state_make(STA *state, int32_t *cpsiz, void **cp, int ptsiz, int arsiz)
{
    char *data;
    double *dvar;
    int32_t *ivar;
    int i;
    void *mkdata;
    void **mkcp;

    // allocate memory
    mkcp = (void **)calloc(ptsiz, sizeof(void *));
    mkdata = calloc(arsiz, 1);
    // copy data
    data = (char *)mkdata;
    for (i = 0; i < ptsiz; i++) {
        if (cp[i]) {
            memcpy(data, cp[i], cpsiz[i]);
            mkcp[i] = data;
            data += cpsiz[i];
        }
    }
    // return state variables
    ivar = (int32_t *) cp[_ivar];
    dvar =  (double *) cp[_dvar];
    state->ptsiz = ptsiz;
    state->arsiz = arsiz;
    state->cp    = mkcp;
    state->data  = mkdata;
    state->sr    = dvar[_sr] * 1000;
    state->cs    = ivar[_cs];
}

void
state_copy(STA *new_state, STA *old_state)
{
    int ptsiz, arsiz;
    int32_t *cpsiz;
    void **cp;

    cp = old_state->cp;
    cpsiz = (int32_t *)cp[_size];
    ptsiz = old_state->ptsiz;
    arsiz = old_state->arsiz;
    state_make(new_state, cpsiz, cp, ptsiz, arsiz);
}

void
state_free(STA *state)
{
    free_null(state->cp);
    free_null(state->data);
}
