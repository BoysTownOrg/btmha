// var_list.c - support for VAR lists

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "eval.h"
#include "var_list.h"

#define CMNT_CHAR       '#'
#define MAX_VAR         256
#define MAX_MSG         1024
#define free_null(x)    if(x){free(x);x=NULL;}

static int rv = 0;
static size_t dtyp_sz[10] = {0, 1, 1, 2, 2, 4, 4, 0, 4, 8};

//===========================================================

static int
eol(char *line)
{
    return ((*line == 0) || (*line == CMNT_CHAR));
}

static int
name_match(char *args, char *name)
{
    char *s;
    int n;

    n = strlen(name);
    s = skipwhite(args);
    while (*s) {
        if (strncmp(s, name, n) == 0) {
            return (1);
        }
        s = skipalphnum(s);
        s = skipwhite(s);
    }
    return (0);
}

int
trim_line(char *line)
{
    char *s;
    int i, j, n;

    s = line;
    n = strlen(s);
    // remove trailing space
    for(i = n; (i > 0) && (s[i - 1] <= ' '); i--) {
        s[i - 1] = 0;
    }
    // find start of text
    for(j = 0; (j < n) && (s[j] <= ' '); j++) {
    }
    // remove leading space
    if (j > 0) {
        for(i = 0; i <= (n - j); i++) {
            s[i] = s[i + j];
        }
    }
    // remove comment
    s = strchr(line, CMNT_CHAR);
    if (s) *s = 0;

    return (strlen(line));
}

//===========================================================

int
var_list_length(VAR *vl)
{
    int i, n = 0;

    // find end of var list
    for (i = 0; i < MAX_VAR; i++) {
        if (vl[i].name) n = i + 1;
    }
    return (n);
}

int
var_list_index(VAR *vl, char *name)
{
    int i;

    // find end of var list
    for (i = 0; i < MAX_VAR; i++) {
        if (vl[i].name && (strcmp(vl[i].name, name) == 0)) {
            return (i);
        }
    }
    return (-1);
}

int
var_list_get(VAR *vl, char *name, void *data, int dtyp, int n)
{
    double *f8a, *f8b;
    int i, nn, d, e = 0;
    int32_t *i4a, *i4b;

    d = var_list_index(vl, name);
    if (d < 0) {
        e = -1;
    } 
    nn = vl[d].rows * vl[d].cols;
    if (nn > n) nn = n;
    if (vl[d].dtyp == SP_DTYP_F8) {
        f8a = (double *)vl[d].data;
        if (dtyp == SP_DTYP_F8) {
            f8b = (double *)data;
            for (i = 0; i < nn; i++) f8b[i] = f8a[i];
        } else if (dtyp == SP_DTYP_I4) {
            i4b = (int32_t *)data;
	    for (i = 0; i < nn; i++) i4b[i] = (int32_t)f8a[i];
        }
    } else if (vl[d].dtyp == SP_DTYP_I4) {
        i4a = (int32_t *)vl[d].data;
        if (dtyp == SP_DTYP_F8) {
            f8b = (double *)data;
            for (i = 0; i < nn; i++) f8b[i] = (double)i4a[i];
        } else if (dtyp == SP_DTYP_I4) {
            i4b = (int32_t *)data;
            for (i = 0; i < nn; i++) i4b[i] = i4a[i];
        }
    } else {
       e = -2;
    }
    return (e);
}

int
var_list_set(VAR *vl, char *name, void *data, int dtyp, int n)
{
    double *f8a, *f8b;
    int i, nn, d, e = 0;
    int32_t *i4a, *i4b;

    d = var_list_index(vl, name);
    if (d < 0) {
        e = -1;
    } 
    nn = vl[d].rows * vl[d].cols;
    if (nn > n) nn = n;
    if (vl[d].dtyp == SP_DTYP_F8) {
        f8a = (double *)vl[d].data;
        if (dtyp == SP_DTYP_F8) {
            f8b = (double *)data;
            for (i = 0; i < nn; i++) f8a[i] = f8b[i];
        } else if (dtyp == SP_DTYP_I4) {
            i4b = (int32_t *)data;
	    for (i = 0; i < nn; i++) f8a[i] = (double)i4b[i];
        }
    } else if (vl[d].dtyp == SP_DTYP_I4) {
        i4a = (int32_t *)vl[d].data;
        if (dtyp == SP_DTYP_F8) {
            f8b = (double *)data;
            for (i = 0; i < nn; i++) i4a[i] = (int32_t)f8b[i];
        } else if (dtyp == SP_DTYP_I4) {
            i4b = (int32_t *)data;
            for (i = 0; i < nn; i++) i4a[i] = i4b[i];
        }
    } else {
       e = -2;
    }
    return (e);
}

double
var_list_f8n(VAR *vl, char *name)
{
    double *dvar = NULL;
    int i;
    int32_t *ivar;

    // find matching var name
    for (i = 0; i < MAX_VAR; i++) {
        if (vl[i].name && (strcmp(vl[i].name, name) == 0)) {
            if (vl[i].dtyp == SP_DTYP_F8) {
	        dvar = (double *)vl[i].data;
                return (dvar[0]);
            } else if (vl[i].dtyp == SP_DTYP_I4) {
	        ivar = (int32_t *)vl[i].data;
                return ((double)ivar[0]);
            }
        }
    }
    return(0);
}

int
var_list_i4n(VAR *vl, char *name)
{
    double *dvar = NULL;
    int i;
    int32_t *ivar;

    // find matching var name
    for (i = 0; i < MAX_VAR; i++) {
        if (vl[i].name && (strcmp(vl[i].name, name) == 0)) {
            if (vl[i].dtyp == SP_DTYP_F8) {
	        dvar = (double *)vl[i].data;
                return ((int32_t)dvar[0]);
            } else if (vl[i].dtyp == SP_DTYP_I4) {
	        ivar = (int32_t *)vl[i].data;
                return (ivar[0]);
            }
        }
    }
    return(0);
}

char *
var_list_str(VAR *vl, char *name)
{
    char *s = NULL;
    int i, r;

    // find matching var name
    for (i = 0; i < MAX_VAR; i++) {
        if ((vl[i].name) && (strcmp(vl[i].name, name) == 0)) {
            r = vl[i].rows;
            s = (char *)malloc(r + 1);
	    memcpy(s, vl[i].data, r);
	    s[r] = 0;
	    break;
        }
    }
    return (s);
}

int
var_list_intersect(VAR *vl0, VAR *vl1, VAR *vl2, char *prfx)
{
    char *n, prfxname[80];
    int i, j, n0, n1, n2, r, c, t, s, cmp1, cmp2;
    void *d;

    // transfer values of v2 variables matching v1 to v0
    n0 = 0;
    n1 = var_list_length(vl1);
    n2 = var_list_length(vl2);
    for (i = 0; i < n2; i++) {
        for (j = 0; j < n1; j++) {
            if (vl1[j].name && vl2[i].name) {
                sprintf(prfxname, "%s.%s", prfx, vl1[j].name);
                cmp1 = strcmp(vl1[j].name, vl2[i].name); // compare name w/o prefix
                cmp2 = strcmp(   prfxname, vl2[i].name); // compare name with prefix
                if ((cmp1 == 0) || (cmp2 == 0)) {        // match either way
		    free_null(vl0[n0].name);
		    free_null(vl0[n0].data);
                    n = vl1[j].name;                     // use name being replaced
                    d = vl2[i].data;
                    r = vl2[i].rows;
                    c = vl2[i].cols;
                    t = vl2[i].dtyp;
		    s = dtyp_sz[t] * r * c;
                    vl0[n0].name = _strdup(n);
                    vl0[n0].data = malloc(s);
                    vl0[n0].rows = r;
                    vl0[n0].cols = c;
                    vl0[n0].dtyp = t;
                    memcpy(vl0[n0].data, d, s);
                    n0++;
                }
                if (n0 == MAX_VAR) break;
            }
        }
    }
    for (i = n0; i < MAX_VAR; i++) {
        if (vl0[i].name) free(vl0[i].name);
        if (vl0[i].data) free(vl0[i].data);
        vl0[i].name = NULL;
        vl0[i].data = NULL;
    }
    return (n0);
}

void
var_list_replace(VAR *vl0, VAR *vl1)
{
    char *n;
    int i, j, n1, n2, r, c, t, s;
    void *d;

    // copy matching vl1 to vl0
    n1 = var_list_length(vl0);
    n2 = var_list_length(vl1);
    for (i = 0; i < n1; i++) {
        for (j = 0; j < n2; j++) {
            if (vl1[j].name && vl0[i].name) {
                if (strcmp(vl1[j].name, vl0[i].name) == 0) {
		    free_null(vl0[i].name);
		    free_null(vl0[i].data);
                    n = vl1[j].name;
                    d = vl1[j].data;
                    r = vl1[j].rows;
                    c = vl1[j].cols;
                    t = vl1[j].dtyp;
		    s = dtyp_sz[t] * r * c;
                    vl0[i].name = _strdup(n);
                    vl0[i].data = malloc(s);
                    vl0[i].rows = r;
                    vl0[i].cols = c;
                    vl0[i].dtyp = t;
                    memcpy(vl0[i].data, d, s);
                }
            }
        }
    }
}

void
var_list_answer(VAR *vl, int i, char *ans)
{
    char *del, *tok;
    double *f8a;
    int j, nr, nc, nn, nm;
    int32_t *i4a;

    if ((i < 0) || (i > MAX_VAR)) {
	return;
    }
    nr = vl[i].rows;
    nc = vl[i].cols;
    nn = nr * nc;
    nm = nn - 1;
    *ans = 0;
    if (vl[i].dtyp == 0) {
        ;
    } else if (vl[i].dtyp == SP_DTYP_F8) {
        f8a = (double *)vl[i].data; 
        if (nn > 1) {
            sprintf(ans + strlen(ans), "[");
            for (j = 0; j < nn; j++) {
                del = (j < nm) ? ((j % nc) == (nc - 1) ? ";" : " ") : "]";
                sprintf(ans + strlen(ans), "%.5g%s", f8a[j], del);
            }
        } else {
            sprintf(ans + strlen(ans), "%.5g", f8a[0]);
        }
    } else if (vl[i].dtyp == SP_DTYP_I4) {
        i4a = (int32_t *)vl[i].data;
        if (nc > 1) {
            sprintf(ans + strlen(ans), "[");
            for (j = 0; j < nn; j++) {
                del = (j < nm) ? ((j % nc) == (nc - 1) ? ";" : " ") : "]";
                sprintf(ans + strlen(ans), "%d%s", i4a[j], del);
            }
        } else {
            sprintf(ans + strlen(ans), "%d", i4a[0]);
        }
    } else if (vl[i].dtyp == SP_DTYP_U1) {
        if (nr > 255) nr = 255;
        if (nc > 1) {
            sprintf(ans + strlen(ans), "[");
            for (j = 0; j < nc; j++) {
                tok = (char *)vl[i].data + j * nr;
                del = (j < (nc - 1)) ? " " : "]";
                sprintf(ans + strlen(ans), "%s%s", tok, del);
            }
        } else {
            tok = (char *)vl[i].data;
            sprintf(ans + strlen(ans), "%s", tok);
        }
    }
}

void
var_list_show_one(VAR *vl, int i)
{
    char *nam, *opt, ans[256];

    if ((i < 0) || (i > MAX_VAR)) {
	return;
    }
    nam = vl[i].name;
    opt = vl[i].last ? "." : " ";
    var_list_answer(vl, i, ans);
    printf("%s   %-16s = %s\n", opt, nam, ans);
}

void
var_list_show(VAR *vl, char *line)
{
    char *args;
    int i, all = 1;

    if (line) {
        args = skipwhite(line);
        args = skipalphnum(args);
        args = skipwhite(args);
        all = eol(args);
    }
    if (all) {                      // display all variables
        for (i = 0; i < MAX_VAR; i++) {
            if (vl[i].name) {
                var_list_show_one(vl, i);
            }
        }
    } else {                              // display only requested variables
        for (i = 0; i < MAX_VAR; i++) {
            if (vl[i].name && name_match(args, vl[i].name)) {
                var_list_show_one(vl, i);
            }
        }
    }
}

void
var_list_delete(VAR *vl, char *name)
{
    int i;

    // delete variables with matching name
    for (i = 0; i < MAX_VAR; i++) {
        if (vl[i].name && (strcmp(vl[i].name, name) == 0)) {
            free(vl[i].name);
            free(vl[i].data);
            vl[i].name = NULL;
            vl[i].data = NULL;
        }
    }
}

void
var_list_clear(VAR *vl)
{
    int i;

    // delete all variables
    for (i = 0; i < MAX_VAR; i++) {
        if (vl[i].name) {
            free(vl[i].name);
            free(vl[i].data);
            vl[i].name = NULL;
            vl[i].data = NULL;
        }
    }
}

void
var_list_eval(VAR *vl, char *name)
{
    char *r;
    double ary[256], *f8a;
    int i, sz, nr, nc;

    // delete variables with matching name
    for (i = 0; i < MAX_VAR; i++) {
        if (vl[i].name && (strcmp(vl[i].name, name) == 0)) {
            if (vl[i].dtyp == SP_DTYP_U1) {
                r = vl[i].data;
                if (eval_expr(r, ary, &nr, &nc, ";", 256) > 0) { 
                    free(vl[i].data);
                    sz = nr * nc * sizeof(double);
                    f8a = malloc(sz);
                    memcpy(f8a, ary, sz);
                    vl[i].rows = nr;
                    vl[i].cols = nc;
                    vl[i].data = f8a;
                    vl[i].dtyp = SP_DTYP_F8;
                }
            }
        }
    }
}

int
var_list_assign(VAR *vl, char *line, int nn, int im)
{
    char *e, *l, *r, str[MAX_MSG];
    double ary[256];
    int b = 0, i, nr, nc;

    line  = _strdup(line); // create local copy
    l = line;
    e = strchr(line, '=');
    r = e + 1;
    *e = 0;
    trim_line(l);
    nr = trim_line(r);
    var_list_delete(vl, l);
    if (nr) {
        if ((*r == '[') && isalpha(*skipwhite(r + 1))) {
            get_strings(r, str, &nr, &nc, MAX_MSG); 
            sp_var_add(vl, l, str, nr, nc, "u1");        // add strings to VAR list
        } else if (nn) {                                 // non-numeric assignment
            sp_var_add(vl, l, r, nr + 1, 1, "u1");
        } else if ((b = eval_expr(r, ary, &nr, &nc, ";", 256)) > 0) { 
            sp_var_add(vl, l, ary, nr, nc, "f8"); // add array to VAR list
            usr_var_set(l, r);
        } else {
            sp_var_add(vl, l, r, nr + 1, 1, "u1");
        }
        nr = strlen(r);
        if (r[nr - 1] == ';') im = 0;
        if (im || (b < 0)) {
            i = var_list_index(vl, l);
            var_list_answer(vl, i, str);
            printf("%s = %s\n", l, str);
            nc = strlen(l) + 3;
            if (get_expr_err(str, MAX_MSG)) {
                printf("%*s%s", nc, "", str);
            }
        }
    }
    rv = (*l && *r);        // return value
    free(line);             // release local copy
    return (rv);
}

void
var_list_add(VAR *vl, char *n, void *d, int c, int t, int *nv)
{
    int i, r, s;

    r = (t == SP_DTYP_U1)  ? strlen(d) + 1 : 1;
    s = dtyp_sz[t] * r * c;
    i = *nv;
    free_null(vl[i].name);
    free_null(vl[i].data);
    vl[i].name = _strdup(n);
    vl[i].data = malloc(s);
    vl[i].rows = r;
    vl[i].cols = c;
    vl[i].dtyp = t;
    vl[i].last = 0;
    memcpy(vl[i].data, d, s);
    *nv = i + 1;
}

void
var_list_cleanup(VAR *vl)
{
    int i;

    if (vl) {
        for (i = 0; i < MAX_VAR; i++) {
            free_null(vl[i].name);
            free_null(vl[i].data);
	}
	free(vl);
    }
}

