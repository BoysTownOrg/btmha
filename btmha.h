// btmha.h - header file for BTMHA
#ifndef BTMHA_H
#define BTMHA_H

#include <math.h>
#include <sigpro.h>

#if !defined(_MSC_VER) || (_MSC_VER > 1500)
#include <stdint.h>
#else
typedef short int16_t;
typedef long int32_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
#endif

// BTMHA definitions
#define MAX_MSG         1024
#define MAX_PRO         10
#define MAX_VAR         256
#define STYP_PLG        0
#define STYP_MIX        1
#define STYP_CHN        2
#define STYP_BNK        3
#define fcopy(x,y,n)    memcpy(x,y,(n)*sizeof(float))
#define fzero(x,n)      memset(x,0,(n)*sizeof(float))
#define free_null(x)    if(x){free(x);x=NULL;}
#define round(x)        ((int)floor((x)+0.5))

// CHAPRO offset definitions
#define _size     0
#define _ivar     1
#define _dvar     2
#define _cs       0
#define _sr       0

//===========================================================

typedef struct {
    int32_t arsiz;               // total size of data (bytes)
    int32_t ptsiz;               // number of data pointers
    void **cp;                   // array of data pointers
    void *data;                  // pointer to data
    double sr;                   // sampling rate (kHz)
    int32_t cs;                  // chunk size
    int32_t type;                // type of state
    int32_t entry;               // entry point index
    void *rprt;                  // pointer to report function
    void *proc;                  // pointer to process function
} STA; 
typedef struct {
    double sr;
    int32_t cs, nsamp, nich, noch;
    int32_t io_dev, io_wait, nreps, rsrv;
    char *process, *input, *output;
    double spl_ref, rms_lev;
} MHA; 
typedef struct {
    char *ifn, *ofn, *dfn, mat, nrep;
    double rate;
    float *iwav, *owav;
    int32_t cs;
    int32_t *siz;
    int32_t iod, nwav, nsmp, wait;
    int32_t mseg, nseg, oseg, pseg;
    int32_t nich, noch;
    void **out;
} I_O;
typedef struct {
    char *name;
    void (*configure)(void *, int *);
    void (*prepare)(STA *, void *, MHA *, int);
    void *pp;
} PLUG; 

int interactive(char, char *);
int trim_line(char *);
void optimize_afc(I_O *, STA *, VAR *, int, double, char *);
void parse_line(char *, char);
void state_copy(STA *, STA *);
void state_free(STA *);

#endif // BTMHA_H
