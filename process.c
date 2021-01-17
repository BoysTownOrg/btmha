// process.c - processor functions for BTMHA program

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "var_list.h"
#include "btmha.h"

#define MAX_DEPTH 64

char *skipalphnum(char *);
char *skipwhite(char *);
int plugin_load(PLUG *, int, char);
void process_prepare(STA *, VAR *, MHA *, char *, int *, char *);

static int depth = 0;            // recursion depth
static int npv = 0;              // number of plugin variables
static PLUG plugin[MAX_PRO];
static VAR *ivl = NULL;
static VAR *pvl = NULL;

/***********************************************************/

int
is_processor(char *line, char *proc, int *pn)
{
    int nl, np, ip = 0;

    nl = strlen(line);
    np = strlen(proc);
    *pn = 0;
    if (strncmp(line, proc, np) == 0) { // full processor name
        if (nl == np) {
            ip++;
        } else if ((nl == (np + 2)) && (line[np] == '.')) {
            if (isdigit(line[np + 1])) {
                ip++;
            }
        } else if ((nl > np) && isdigit(line[np])) {
            *pn = line[np] - '0';
            if (nl == (np + 1)) {
                ip++;
            } else if ((nl == (np + 3)) &&
                 (line[np + 1] == '.') &&
                 isdigit(line[np + 2])) {
                ip++;
            }
        }
    } else if (line[0] == proc[0]) {    // first-letter abbreviation
        if (nl == 1) {
            ip++;
        } else if ((nl == 3) && (line[1] == '.')) {
            if (isdigit(line[2])) {
                ip++;
            }
        } else if ((nl > 1) && isdigit(line[1])) {
            *pn = line[1] - '0';
            if (nl == 2) {
                ip++;
            } else if ((nl == 4) &&
                 (line[2] == '.') &&
                 isdigit(line[3])) {
                ip++;
            }
        }
    }
    return (ip);
}

void
process_show(VAR *vl, char *line, char *dpn)
{
    char pn[8];
    int i, ii, n;

    if ((*line == 0) || (strcmp(line, dpn) == 0)) { // show all
        ii = var_list_index(vl, dpn);
        var_list_show_one(vl, ii);
        for (i = 0; i < MAX_PRO; i++) {
            sprintf(pn, "%s%d", dpn, i);
            ii = var_list_index(vl, pn);
            var_list_show_one(vl, ii);
        }
    } else if (is_processor(line, dpn, &n)) {   // show one
        ii = var_list_index(vl, line);
        var_list_show_one(vl, ii);
    }
}

/***********************************************************/

static void
process_mixer(void **cp, float *x, float *y, int cs)
{
    float a, *m, *z, *mm, *xx, *yy;
    int32_t *v;
    int i, j, k, r, c;

    // retrieve mixer info
    v = cp[1]; // pointer to integer variables
    z = cp[2]; // pointer to output chunk buffer
    m = cp[3]; // pointer to mixer matrix
    r = v[0];  // rows
    c = v[1];  // cols
    // multiply input chunk by mixer matrix
    yy = z;
    for (k = 0; k < cs; k++) {
        mm = m;
        for (j = 0; j < c; j++) {
            xx = x + k * r;
            a = 0;
            for (i = 0; i < r; i++) {
                a  += (*xx++) * (*mm++);
            }
            *yy++ = a;
        }
    }
    // copy chunk buffer to output
    memcpy(y, z, c * cs * sizeof(float));
}

static void
prepare_mixer(STA *st, VAR *lvl, MHA *mha, char *p, int *rc)
{
    char *data;
    double *f8a;
    float  *f4a;
    int cs, i, ii, nr, nc, nintv;
    int32_t ptsiz, arsiz, size[4], *i4a;
    void **cp;

    st->type  = STYP_MIX; // state type is "mixer"
    st->proc = NULL;
    ii = var_list_index(lvl, p);
    if ((ii < 0) && (strcmp(p, "mixer") == 0)) {
        ii = var_list_index(lvl, "mixer0");
    }
    if (ii < 0) {
        printf("%s not specified\n", p);
        return;
    }
    var_list_eval(lvl, p);
    cs = mha->cs;
    nr = lvl[ii].rows;
    nc = lvl[ii].cols;
    ptsiz = 4; // number of pointers
    nintv = 2; // number of integer variables
    size[0] = ptsiz * sizeof(int32_t);
    size[1] = nintv * sizeof(int32_t);
    size[2] = cs * nc * sizeof(float);
    size[3] = nr * nc * sizeof(float);
    arsiz = size[0] + size[1] + size[2] + size[3];
    cp = (void **)calloc(ptsiz, sizeof(void *));
    data = calloc(arsiz, 1);
    cp[0] = (void *)data;
    cp[1] = (char *)cp[0] + size[0];
    cp[2] = (char *)cp[1] + size[1];
    cp[3] = (char *)cp[2] + size[2];
    i4a = cp[0];
    i4a[0] = size[0];
    i4a[1] = size[1];
    i4a[2] = size[2];
    i4a[3] = size[3];
    i4a = cp[1];
    i4a[0] = nr;
    i4a[1] = nc;
    f4a = cp[3];
    f8a = lvl[ii].data;
    for (i = 0; i < nr * nc; i++) {
        f4a[i] = (float) f8a[i];
    }
    st->sr    = mha->sr;
    st->cs    = mha->cs;
    st->ptsiz = ptsiz;
    st->arsiz = arsiz;
    st->cp    = cp;
    st->data  = data;
    st->proc  = process_mixer; 
    st->rprt  = NULL; 
    st->entry = 0; 
    rc[0]     = nr;
    rc[1]     = nc;
}

/***********************************************************/

static void
state_ptr(STA *st, void **cp, void *data)
{
    int i;
    int32_t ptsiz, ptrsz, type, stasz, *size;
    void **spcp, *spdat;
    STA *spsta;

    // retrive state info
    ptsiz = st->ptsiz;
    type = st->type;
    size = (int32_t *)data;
    // update list of pointers
    cp[0] = data;
    for (i = 1; i < ptsiz; i++) {
        cp[i] = (char *)cp[i - 1] + size[i - 1];
    }
    // update input state
    st->cp = cp;
    st->data = data;
    // update states of any composite processors
    if ((type == STYP_CHN) || (type == STYP_BNK)) {
        stasz = sizeof(STA);
        for (i = 5; i < ptsiz; i++) {
            spsta = (STA *)cp[i]; // pointer to subprocessor state
            ptrsz = spsta->ptsiz * sizeof(void *);
            spcp = (void **)((char *)cp[i] + stasz);
            spdat = (void *)((char *)cp[i] + stasz + ptrsz);
            state_ptr(spsta, spcp, spdat);
        }
    }
}

/***********************************************************/

static void
process_chain(void **cp, float *x, float *y, int cs)
{
    float *xx, *yy;
    int i, np, no;
    int32_t *iv;
    void (**pp)(void **, float *, float *, int);
    STA **ps;

    // retrieve chain info
    iv = cp[1];          // pointer to integer variables
    yy = cp[2];          // pointer to output chunk buffer
    pp = cp[3];          // pointer to list of process functions
    ps = (STA **)&cp[5]; // pointer to list of processor states
    no = iv[1];          // number of output channels
    np = iv[2];          // number of processors
    // process input chunk by chain of processors
    xx = x;
    for (i = 0; i < np; i++) {
        pp[i](ps[i]->cp, xx, yy, cs);
        xx = yy;
    }
    // copy chunk buffer to output
    memcpy(y, yy, cs * no * sizeof(float)); 
}

static void
report_chain(void **cp, char *msg)
{
    int i, np;
    int32_t *iv;
    void (**pr)(void **, char *);
    STA **ps;

    // retrieve chain info
    iv = cp[1];          // pointer to integer variables
    pr = cp[4];          // pointer to list of report functions
    ps = (STA **)&cp[5]; // pointer to list of processor states
    np = iv[2];          // number of processors
    // retrieve report from chain of processors
    for (i = 0; i < np; i++) {
        if (pr[i]) {
            pr[i](ps[i]->cp, msg + strlen(msg));
        }
    }
}

static int
initial_entry(char *pn, int ii, int nr, int nc)
{
    char *s, *name, root[20];
    int i, n;

    name = pn + ii * nr;
    strcpy(root, name);
    for(s = root; *s && (*s != '.'); s++);
    s[0] = 0;
    n = (int)(s - root);
    for (i = 0; i < ii; i++) {
        if (strncmp(root, pn + i * nr, n) == 0) break;
    }
    return (i);
}

static void
prepare_chain(STA *st, VAR *lvl, MHA *mha, char *p, int *rc)
{
    char *pn, *spnm, *data, *spdat;
    int cs, i, j, ii, ie, mx, nr, ni, no, nc, io[2];
    int32_t ptsiz, arsiz, nintv, datsz, stasz, ptrsz;
    int32_t stptsz, starsz, *sz, size[64];
    void **cp, **spcp, **proc, **rprt;
    STA spst[64], *spsta;

    st->type  = STYP_CHN; // state type is "chain"
    st->proc = NULL;
    ii = var_list_index(lvl, p);
    if ((ii < 0) && (strcmp(p, "chain") == 0)) {
        ii = var_list_index(lvl, "chain0");
    }
    if (ii < 0) {
        printf("%s not specified\n", p);
        return;
    }
    cs = mha->cs;
    nr = lvl[ii].rows;
    nc = lvl[ii].cols;
    pn = lvl[ii].data;
    stasz = sizeof(STA);
    datsz = 0;
    // collect subprocessor information
    mx = no  = ni = 0;
    for (i = 0; i < nc; i++) {
        // prepare subprocessor
        spnm = pn + i * nr;
        process_prepare(&spst[i], lvl, mha, spnm, io, NULL);
        if (spst[i].proc == NULL) {
            printf("ERROR: %s prepare_chain failed\n", spnm);
            rc[0] = ni;
            rc[1] = io[1];
            return;
        }
        ie = initial_entry(pn, i, nr, nc);
        if (i > ie) {
            size[i + 5] = 0;
        } else {
            // calculate data size
            ptsiz = spst[i].ptsiz;
            arsiz = spst[i].arsiz;
            ptrsz = ptsiz * sizeof(void *);
            datsz += stasz + ptrsz + arsiz;
            size[i + 5] = stasz + ptrsz + arsiz;
        }
        // check channel number compatibility
        if (i == 0) {
            ni = io[0];
        } else {
            if (io[0] != no) {
                printf("ERROR: %s.%s ", p, spnm);
                printf("has wrong number of input channels ");
                printf("(%d!=%d)\n",io[0], no);
                rc[0] = ni;
                rc[1] = io[1];
                return;
            }
        }
        no = io[1];
        if (mx < no) mx = no;
    }
    ptsiz = 5 + nc; // number of pointers
    nintv = 3;      // number of integer variables
    size[0] = ptsiz * sizeof(int32_t);
    size[1] = nintv * sizeof(int32_t);
    size[2] = cs * mx * sizeof(float);
    size[3] = nc * sizeof(void *);
    size[4] = nc * sizeof(void *);
    stptsz = ptsiz;
    starsz = datsz;
    for (i = 0; i < 5; i++) {
        starsz += size[i];
    }
    ptrsz = ptsiz * sizeof(void *);
    cp = (void **)calloc(stptsz, ptrsz);
    data = calloc(starsz, 1);
    // initialize chain pointers
    cp[0] = data;
    for (i = 1; i < ptsiz; i++) {
        cp[i] = (char *)cp[i - 1] + size[i - 1];
    }
    // initialize sizes
    sz = cp[0];
    for (i = 0; i < ptsiz; i++) {
        sz[i] = size[i];
    }
    sz = cp[1];
    sz[0] = ni;
    sz[1] = no;
    sz[2] = nc;
    // build composite state
    proc = (void **)cp[3];
    rprt = (void **)cp[4];
    for (i = 0; i < nc; i++) {
        // prepare subprocessor
        ie = initial_entry(pn, i, nr, nc);
        if (i > ie) {
            cp[i + 5] = cp[ie + 5]; // reuse initial-entry state
        } else {
            // set data size
            ptsiz = spst[i].ptsiz;
            arsiz = spst[i].arsiz;
            ptrsz = ptsiz * sizeof(void *);
            // recalculate subprocessor pointers
            j = i + 5;
            spsta = (STA *)cp[j]; // pointer to subprocessor state
            spcp = (void **)((char *)cp[j] + stasz);
            spdat = (void *)((char *)cp[j] + stasz + ptrsz);
            // copy subprocessor state & data
            memcpy(spsta, &spst[i], stasz);
            memcpy(spdat, spst[i].data, arsiz);
            state_ptr(spsta, spcp, spdat);
        }
        // copy pointer to process function
        proc[i] = spst[i].proc;
        rprt[i] = spst[i].rprt;
        // free subprocessor
        free_null(spst[i].cp);
        free_null(spst[i].data);
    }
    st->sr    = mha->sr;
    st->cs    = mha->cs;
    st->ptsiz = stptsz;
    st->arsiz = starsz;
    st->cp    = cp;
    st->data  = data;
    st->proc  = process_chain; 
    st->rprt  = report_chain; 
    st->entry = 0; 
    rc[0]     = ni;
    rc[1]     = no;
}

/***********************************************************/

static void
process_bank(void **cp, float *x, float *y, int cs)
{
    float *xx, *yy;
    int i, j, ii, nn, np, ni, no;
    int32_t *sz, *spni, *spno;
    void (**pp)(void **, float *, float *, int);
    STA **ps;

    // retrieve bank info
    sz = cp[1];          // pointer to integer variables
    yy = cp[2];          // pointer to output chunk buffer
    pp = cp[3];          // pointer to list of process functions
    ps = (STA **)&cp[5]; // pointer to list of processor states
    ni = sz[0];          // total number of input channels
    no = sz[1];          // total number of output channels
    np = sz[2];          // number of processors
    spni = sz + 3;       // number of subprocessor input channels
    spno = sz + 3 + np;  // number of subprocessor output channels
    // transpose input to input chunk buffer
    xx = (float *)((char *)cp[2] + cs * no); // pointer to input chunk buffer
    ii = 0;
    for (i = 0; i < np; i++) {
	nn = spni[i];
        for (j = 0; j < cs; j++) {
	    memcpy(xx, x + ii + j * ni, nn * sizeof(float)); 
            xx += nn;
        }
	ii += nn;
    }
    // process input chunk by bank of processors
    xx = (float *)((char *)cp[2] + cs * no); // pointer to input chunk buffer
    yy = (float *)cp[2];                     // pointer to output chunk buffer
    for (i = 0; i < np; i++) {
        pp[i](ps[i]->cp, xx, yy, cs);
        xx += spni[i] * cs;
        yy += spno[i] * cs;
    }
    // transpose ouput chunk buffer to output
    yy = cp[2];           // pointer to output chunk buffer
    ii = 0;
    for (i = 0; i < np; i++) {
	nn = spno[i];
        for (j = 0; j < cs; j++) {
	    memcpy(y + ii + j * no, yy, nn * sizeof(float)); 
            yy += nn;
        }
	ii += nn;
    }
}

static void
report_bank(void **cp, char *msg)
{
    int i, np;
    int32_t *iv;
    void (**pr)(void **, char *);
    STA **ps;

    // retrieve bank info
    iv = cp[1];          // pointer to integer variables
    pr = cp[4];          // pointer to list of report functions
    ps = (STA **)&cp[5]; // pointer to list of processor states
    np = iv[2];          // number of processors
    // retrieve report from bank of processors
    for (i = 0; i < np; i++) {
        if (pr[i]) {
            pr[i](ps[i]->cp, msg + strlen(msg));
        }
    }
}

static void
prepare_bank(STA *st, VAR *lvl, MHA *mha, char *p, int *rc)
{
    char *pn, *spnm, *data, *spdat;
    int cs, i, j, ii, nr, ni, no, nc, io[2];
    int32_t ptsiz, arsiz, nintv, datsz, stasz, ptrsz;
    int32_t stptsz, starsz, *sz, spni[64], spno[64], size[64];
    void **cp, **spcp, **proc, **rprt;
    STA spst[64], *spsta;

    st->type  = STYP_BNK; // state type is "bank"
    st->proc = NULL;
    ii = var_list_index(lvl, p);
    if ((ii < 0) && (strcmp(p, "bank") == 0)) {
        ii = var_list_index(lvl, "bank0");
    }
    if (ii < 0) {
        printf("%s not specified\n", p);
        return;
    }
    cs = mha->cs;
    nr = lvl[ii].rows;
    nc = lvl[ii].cols;
    pn = lvl[ii].data;
    stasz = sizeof(STA);
    datsz = 0;
    // collect subprocessor information
    ni = no = 0;
    for (i = 0; i < nc; i++) {
        // prepare subprocessor
        spnm = pn + i * nr;
        process_prepare(&spst[i], lvl, mha, spnm, io, NULL);
        if (spst[i].proc == NULL) {
            printf("ERROR: prepare_bank failed for %s\n", spnm);
            rc[0] = ni;
            rc[1] = io[1];
            return;
        }
        // calculate data size
        ptsiz = spst[i].ptsiz;
        arsiz = spst[i].arsiz;
        ptrsz = ptsiz * sizeof(void *);
        datsz += stasz + ptrsz + arsiz;
        size[i + 5] = stasz + ptrsz + arsiz;
	// save number of subprocessor input & output channels
        spni[i] = io[0];
        spno[i] = io[1];
	// total number of input & output channels
        ni += io[0];
        no += io[1];
    }
    ptsiz = 5 + nc;     // number of pointers
    nintv = 3 + nc * 2; // number of integer variables
    size[0] = ptsiz * sizeof(int32_t);
    size[1] = nintv * sizeof(int32_t);
    size[2] = cs * (ni + no) * sizeof(float);
    size[3] = nc * sizeof(void *);
    size[4] = nc * sizeof(void *);
    stptsz = ptsiz;
    starsz = datsz;
    for (i = 0; i < 5; i++) {
        starsz += size[i];
    }
    ptrsz = ptsiz * sizeof(void *);
    cp = (void **)calloc(stptsz, ptrsz);
    data = calloc(starsz, 1);
    // initialize bank pointers
    cp[0] = data;
    for (i = 1; i < ptsiz; i++) {
        cp[i] = (float *)((char *)cp[i - 1] + size[i - 1]);
    }
    // initialize sizes
    sz = cp[0];
    for (i = 0; i < ptsiz; i++) {
        sz[i] = size[i];
    }
    sz = cp[1];
    sz[0] = ni;
    sz[1] = no;
    sz[2] = nc;
    for (i = 0; i < nc; i++) {
        sz[i + 3] = spni[i];
        sz[i + 3 + nc] = spno[i];
    }
    // build composite state
    proc = (void **)cp[3];
    rprt = (void **)cp[4];
    for (i = 0; i < nc; i++) {
        // calculate data size
        ptsiz = spst[i].ptsiz;
        arsiz = spst[i].arsiz;
        ptrsz = ptsiz * sizeof(void *);
        // recalculate subprocessor pointers
        j = i + 5;
        spsta = (STA *)cp[j]; // pointer to subprocessor state
        spcp = (void **)((char *)cp[j] + stasz);
        spdat = (void *)((char *)cp[j] + stasz + ptrsz);
        // copy subprocessor state & data
        memcpy(spsta, &spst[i], stasz);
        memcpy(spdat, spst[i].data, arsiz);
        state_ptr(spsta, spcp, spdat);
        // copy pointer to process & report functions
        proc[i] = spst[i].proc;
        rprt[i] = spst[i].rprt;
        // free subprocessor
        free_null(spst[i].cp);
        free_null(spst[i].data);
    }
    st->sr    = mha->sr;
    st->cs    = mha->cs;
    st->ptsiz = stptsz;
    st->arsiz = starsz;
    st->cp    = cp;
    st->data  = data;
    st->proc  = process_bank; 
    st->rprt  = report_bank; 
    st->entry = 0; 
    rc[0]     = ni;
    rc[1]     = no;
}

//===========================================================

static int
plugin_entry(char *p)
{
    while (*p && (*p != '.')) p++;
    if (*p && isdigit(p[1])) {
        return (p[1] - '0');
    }
    return (0);
}

static void
prepare_plugin(STA *st, MHA *mha, char *msg, char *p, int *rc, int n)
{
    int pe;
    void (*report)(void **, char *);

    st->type = STYP_PLG; // state type is "plugin"
    st->proc = NULL;
    if (plugin[n].name == NULL) {
        if (msg) sprintf(msg, "%s not specified\n", p);
        return;
    }
    if (plugin[n].prepare == NULL) {
        if (msg) sprintf(msg, "can't prepare plugin %s", plugin[n].name);
        return;
    }
    pe = plugin_entry(p);
    plugin[n].prepare(st, ivl, mha, pe);
    var_list_get(pvl, "nchan", rc, SP_DTYP_I4, 2);
    // fetch report
    report = st->rprt;
    if (report && msg) {
        *msg = 0;
        report(st->cp, msg);
    }
}

//===========================================================

void
plugin_show(char mesg_type, char *line, char *p, int n)
{
    char *name;

    name = plugin[n].name;
    if (name == NULL) {
        return;
    }
    // show plugin variable list
    switch(mesg_type) {
    case 1:
        printf("%s%d.%s:\n", p, n, name);
        var_list_show(pvl, line);
        break;
    case 2:
        printf("%s%d.%s^list:\n", p, n, name);
        var_list_show(ivl, line);
        break;
    default:
        printf("%12s: %s.%s\n", "process", p, name);
    }
}

void
plugin_config(VAR *lvl, char *p, char *plug, int n)
{
    char *name, str[20];

    // plugin name
    name = NULL;
    if (n == 0) {
        name = var_list_str(lvl, plug);
    }
    if (name == NULL) {
        sprintf(str, "plugin%d", n);
        name = var_list_str(lvl, str);
    }   
    plugin[n].name = name;
    if (name == NULL) {
        return;
    }
    if (plugin_load(plugin, n, 0)) {
        printf("%s.%s not loaded\n", p, name);
        return;
    }
    if (plugin[n].configure == NULL) {
        printf("cannot configure %s.%s\n", p, name);
        return;
    }
    // extract prefix
    strcpy(str, p);
    for (name = str; *name && (*name != '.'); name++);
    name[0] = 0;
    // update intersection variable list
    npv = MAX_VAR;
    plugin[n].configure(pvl, &npv); 
    var_list_intersect(ivl, pvl, lvl, str);
    var_list_delete(ivl, "nchan"); // exclude nchan from intersect
}

/***********************************************************/

void
process_init()
{
    pvl = (VAR *) calloc(MAX_VAR, sizeof(VAR));
    ivl = (VAR *) calloc(MAX_VAR, sizeof(VAR));
}

int
process_config(VAR *lvl, char *p, int quiet)
{
    int m, n, e = 0;

    if (is_processor(p, "plugin", &n)) {    
        plugin_config(lvl, p, "plugin", n);    // configure plugin
        if (plugin[n].name == NULL) {
            e = 1;
        } else {
            m = quiet ? 0 : 2;
            plugin_show(m, 0, "plugin", n);
        }
    } else if (is_processor(p, "mixer", &n)) {   // show mixer 
        if (!quiet) process_show(lvl, p, "mixer");
    } else if (is_processor(p, "chain", &n)) {   // show chain
        if (!quiet) process_show(lvl, p, "chain");
    } else if (is_processor(p, "bank", &n)) {    // show bank
        if (!quiet) process_show(lvl, p, "bank");
    } else {                                     // unknown process
        e = 1;
    }
    depth = 0;
    return (e);
}

void
process_prepare(STA *st, VAR *lvl, MHA *mha, char *p, int *rc, char *msg)
{
    int n;

    // check recursion depth
    if (depth > MAX_DEPTH) {
        fprintf(stderr, "ERROR: recursion depth exceeded\n");
        exit (1);
    }
    depth++;
    // prepare processor state
    if (is_processor(p, "plugin", &n)) {       // prepare plugin
        plugin_config(lvl, p, "plugin", n);    // configure plugin
        prepare_plugin(st, mha, msg, p, rc, n);
    } else if (is_processor(p, "mixer", &n)) { // prepare mixer
        prepare_mixer(st, lvl, mha, p, rc);
    } else if (is_processor(p, "chain", &n)) { // prepare chain
        prepare_chain(st, lvl, mha, p, rc);
    } else if (is_processor(p, "bank", &n)) {  // prepare bank
        prepare_bank(st, lvl, mha, p, rc);
    } else {
        if (msg && p) {
            sprintf(msg, "unknown processor %s\n", p);
        }
        st->proc = NULL;
	return;
    }
    depth--;
}

void
process_cleanup()
{
    int i;

    for (i = 0; i < MAX_PRO; i++) {
        if (plugin[i].name) {
            //if (plugin[i].cleanup) {
		//plugin[i].cleanup();
            //}
            free_null(plugin[i].name);
	}
    }
    free_null(pvl);
    var_list_cleanup(ivl);
}

/***********************************************************/

static int
str_find(char *s, char *t)
{
    char *b, *e;
    int i, n;

    n = strlen(t);
    b = s;
    while (*b++) {
        e = t;
        for (i = 0; i < n; i++) {
            if (b[i] != e[i]) break;
        }
        if (i == n) break;
    }
    return (i == n);
}

int
optimize_config(VAR *ovl, char *line)
{
    char *s, *name;
    double parval[8] = {0};
    int i, nov;
    static char *parnam[] = {"afc_rho", "afc_eps", "afc_mu"};
    static char npar = sizeof(parnam) / sizeof(char *);

    // parse line
    s = skipwhite(skipalphnum(line));
    // initialize parameters
    nov = 0;
    var_list_clear(ovl);
    for (i = 0; i < npar; i++) {
        name = parnam[i];
        parval[i] = var_list_f8n(pvl, name);
        if ((*s == 0) || str_find(s, name)) {
            var_list_add(ovl, name, &parval[i], 1, SP_DTYP_F8, &nov);
        }
    }
    return (nov);
}

void
optimize_plugin(I_O *iop, STA *stp, VAR *ovl, int nov, double t, char *msg)
{
    int i, j;

    for (i = 0; i < nov; i++) {
        j = var_list_index(pvl, ovl[i].name);
        if (j > 0) ovl[i].last = pvl[j].last;
    }
    optimize_afc(iop, stp, ovl, nov, t, msg);
}

/***********************************************************/

