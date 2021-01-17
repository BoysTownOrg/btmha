// btmha.h - header file for BTMHA plugin

#ifndef BTMHA_H
#define BTMHA_H 
#include <stdint.h>

//===========================================================

#define MAX_CHN         32
#define MAX_NAM         256
#define MAX_VAR         256
#define MAX_MSG         1024

#define DTYP_I1      1
#define DTYP_U1      2
#define DTYP_I2      3
#define DTYP_U2      4
#define DTYP_I4      5
#define DTYP_U4      6
#define DTYP_F4      8
#define DTYP_F8      9

#define put_list_f8n(n,d)       put_list(n,d,1,1,DTYP_F8,0,1)
#define put_list_f8o(n,d,o)     put_list(n,d,1,1,DTYP_F8,o,1)
#define put_list_f8a(n,d,c,m)   put_list(n,d,1,c,DTYP_F8,0,m)
#define put_list_i4n(n,d)       put_list(n,d,1,1,DTYP_I4,0,1) 
#define put_list_i4a(n,d,c,m)   put_list(n,d,1,c,DTYP_I4,0,m)
#define put_list_u1a(n,d)       put_list(n,d,strlen(d)+1,1,DTYP_U1, 0, MAX_NAM)
#define var_list_init(v) {                    \
                     mx_npv = *nv;            \
                     pvl = (VAR_ *) v;        \
                     npv = 0;                 \
                     }
#define var_list_return(nv) {                 \
                     *nv = npv;               \
                     }

typedef struct {
    char *name;
    void *data;
    int32_t rows, cols;
    char dtyp, cmpx, text, last;
} VAR_; 
typedef struct {
    double sr;
    int32_t cs, nsamp, nich, noch;
    int32_t io_dev, io_wait, nreps, entry;
    char *process, *input, *output;
    double spl_ref, rms_lev;
} MHA;
typedef struct {
    char *name;
    void (*configure)(void*,int*);
    void (*prepare)(CHA_STA *,void*,MHA*,int);
    void *pp;
} PLUG; 

static int mx_npv = 0;
static int pvmx[MAX_VAR] = {0};
static int npv = 0;
static VAR_ *pvl = NULL;

//===========================================================

static void
put_list(char *n, void *d, int r, int c, int t, int o, int m) {
     pvl[npv].name = n;
     pvl[npv].data = d;
     pvl[npv].rows = r;
     pvl[npv].cols = c;
     pvl[npv].dtyp = t;
     pvl[npv].last = o;
     pvmx[npv] = m;
     npv++;
}

static void
var_transfer(void *v)
{
    char *u1a;
    double *f8a;
    int i, j, k, nv, sz;
    int32_t *i4a;
    VAR_ *ivl;

    ivl = (VAR_ *)v;
    for (i = 0; i < MAX_VAR; i++) {
        if (ivl[i].name == NULL) continue;
        if (ivl[i].name[0] == 0) continue;
        for (j = 0; j < MAX_VAR; j++) {
            if (pvmx[j] == 0) continue;
            if (pvl[j].name == NULL) continue;
            if (pvl[j].name[0] == 0) continue;
            if (strcmp(ivl[i].name, pvl[j].name) == 0) {
                if (pvl[j].dtyp == DTYP_I4) {            // integer
                    f8a = (double *)(ivl[i].data);
                    i4a = (int32_t *)(pvl[j].data);
                    sz = pvmx[j];
                    nv = ivl[i].rows * ivl[i].cols;
                    if (sz > nv) sz = nv;
                    for (k = 0; k < sz; k++) {
                        i4a[k] = (int32_t) f8a[k];
                    }
                    pvl[j].rows = 1;
                    pvl[j].cols = sz;
                } else if (pvl[j].dtyp == DTYP_U1) {     // string
                    sz = pvmx[j] - 1;
                    nv = ivl[i].rows * ivl[i].cols;
                    if (sz > nv) sz = nv;
                    memcpy(pvl[j].data, ivl[i].data, sz);
                    pvl[j].rows = 1;
                    pvl[j].cols = sz;
                    u1a = (char *)(pvl[j].data);
                    u1a[sz] = 0;
                } else if (pvl[j].dtyp == DTYP_F8) {     // double
                    sz = pvmx[j];
                    nv = ivl[i].rows * ivl[i].cols;
                    if (sz > nv) sz = nv;
                    memcpy(pvl[j].data, ivl[i].data, sz * sizeof(double));
                    pvl[j].rows = 1;
                    pvl[j].cols = sz;
                } else {
		    fprintf(stderr, "var_transfer: unknown data type; "); 
		    fprintf(stderr, "name=%s\n", pvl[j].name);
                }
            }
        }
    }
}

//===========================================================

#endif // BTMHA_H
