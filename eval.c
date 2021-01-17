/* eval.c */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include "eval.h"

#ifndef WIN32
#include <unistd.h>
#define _strdup         strdup
#endif

#define MAXNOPS	    8
#define MAXNNBS	    8
#define MAXNARG	    8
#define MAX_DAT	    256
#define MAX_ERR	    100
#define MAX_MSG	    256
#define EQ          128
#define EBE         128

#define MAXNUV 99
#define MAXNSZ 99

#define free_null(x)            if(x){free(x);x=NULL;}
#define round(x)                ((int)floor((x)+0.5))
#define stack_scalar(i,v)       {vs[i].data[0]=v;vs[i].rows=1;vs[i].cols=1;i++;}
#define stack_array(i,a,m,n)    {memcpy(vs[i].data,a,m*n*sizeof(double));vs[i].rows=m;vs[i].cols=n;i++;}

typedef struct {char *name; double data[MAX_DAT]; int32_t rows, cols;} VAR;

static char err_msg[MAX_MSG] = {0};
static int nuv = 0;
static VAR vl[MAXNUV] = {{0}};

/**********************************************************************/

char *
skipwhite(char *s)
{
    while (*s == ' ' || *s == '\t' || *s == '\n')
	s++;
    return (s);
}

char *
skipalphnum(char *s)
{
    while ((*s >= 'A' && *s <= 'Z') || (*s >= 'a' && *s <= 'z')
        || (*s >= '0' && *s <= '9') || (*s == '_') || (*s == '.'))
	s++;
    return (s);
}

int
inset(int c, char *s)
{
    char *p = s;

    while (*p != '\0')
	if (*p++ == c)
	    return (p - s);
    return (0);
}

/**********************************************************************/

static int
usr_var_nam(char *s, char *nam)
{
    int i, n;

    s = skipwhite(s);
    n = (int) (skipalphnum(s) - s);
    if (n >= MAXNSZ)
	n = MAXNSZ - 1;
    for (i = 0; i < n; i++)
	nam[i] = s[i];
    nam[n] = '\0';
    return (n);
}

static int
usr_var_get(char *s, double *v, char *msg)
{
    char nam[MAXNSZ];
    int i;

    if (!nuv || !usr_var_nam(s, nam)) {
	return (0);
    }
    for (i = 0; i < nuv; i++) {
	if (vl[i].name 
            && vl[i].name[0]
            && (strcmp(vl[i].name, nam) == 0))
	    break;
    }
    if (i >= nuv) {
	return (0);
    }
    if (v) {
        *v = vl[i].data[0];
    }
    if (msg)  {
        sprintf(msg, "%s = %g", nam, *v);
    }
    return (i + 1);
}

static int
usr_var_chk(char *ps, double *pn, int *nr, int *nc, int mx)
{
    char *s;
    double val;
    int n, nn;

    s = skipwhite(ps);
    if (!isalpha(s[0])) return (0);
    n = usr_var_get(s, &val, NULL);
    if (n <= 0) return (0);
    n--;
    *nr = vl[n].rows;
    *nc = vl[n].cols;
    nn = vl[n].rows * vl[n].cols;
    if (nn < mx) {
        memcpy(pn, vl[n].data, nn * sizeof(double));
    } else {
        *nr = 1;
        *nc = mx;
        memcpy(pn, vl[n].data, mx * sizeof(double));
    }
    return (strlen(vl[n].name));
}

int
usr_var_set(char *lhs, char *rhs)
{
    char nam[MAXNSZ];
    double ary[MAX_DAT], val = 0;
    int n, nr, nc, sz, b;

    if (!usr_var_nam(lhs, nam)) {
	return (0);
    }
    n = usr_var_get(nam, &val, NULL);
    n = n ? n - 1 : nuv++;
    b = eval_expr(rhs, ary, &nr, &nc, ";", MAX_DAT);
    if (b > 0) { 
	free_null(vl[n].name);
	vl[n].name = _strdup(nam);
	vl[n].rows = nr;
	vl[n].cols = nc;
        sz = ((nr * nc) < MAX_DAT) ? nr * nc : MAX_DAT;
	memcpy(vl[n].data, ary, sz * sizeof(double));
    }
    return (1);
}

/**********************************************************************/

int
get_array(char *ps, double *pn, int *nr, int *nc, int mx)
{
    char *s;
    int i, m, r, c, b;

    s = skipwhite(ps);
    if (*s != '[') {
        *nr = 1;
        *nc = 1;
        pn[0] = 0;
        return (0);
    }
    r = 1;
    m = 0;
    s = skipwhite(s + 1);
    for(i = 0; i < mx; i++) {
        b = eval_expr(s, pn + i, nr, nc, " ,];", 1);
        if (b <= 0) break;
        if (s[b] == ']') {
            s = skipwhite(s + b + 1);
            i++;
            break;
        } else if (s[b] == ';') {
            r++;
            m = i + 1;
        }
        s = skipwhite(s + b + 1);
    }
    c = i - m;
    if (b <= 0) {
        *nr = 1;
        *nc = 1;
        pn[0] = 0;
	return (b);
    } else if (i == (r * c)) {
        *nr = r;
        *nc = c;
    } else {
        *nr = 1;
        *nc = i;
    }
    b  = (s - ps);
    *err_msg = 0;
    return (b);
}

int
get_strings(char *ps, char *pn, int *nr, int *nc, int mx)
{
    char *s, *t, *u, *v, *w;
    int r, c;

    s = skipwhite(ps + 1);
    r = 0;
    for(c = 0; c < mx; c++) {
        if ((*s == ']') || (*s == 0)) break;
        t = skipalphnum(s);
        if (r < (t - s)) r = t - s;
        s = skipwhite(t);
        if (*s == ',') s = skipwhite(s + 1);
    }
    r++;
    s = skipwhite(ps + 1);
    w = pn;
    for(c = 0; c < mx; c++) {
        if ((*s == ']') || (*s == 0)) break;
        u = s;
        t = skipalphnum(s);
        s = skipwhite(t);
        if (*s == ',') s = skipwhite(s + 1);
        v = w + r;
        while (u < t) *w++ = *u++;
        while (w < v) *w++ = 0;
    }
    *nr = r;
    *nc = c;
    return (s - ps);
}

/**********************************************************************/

static int
op_prec(int c)
{
    c &= 255;
    if (c == '~' || c == '\'')
	return (8);
    if (c == '^')
	return (7);
    if (c == '*' || c == '/' || c == '%')
	return (6);
    if (c == '+' || c == '-')
	return (5);
    if (c == '<' || c == ('<' + EQ) || c == '>' ||  c == ('>' + EQ))
	return (4);
    if (c == ('=' + EQ) ||  c == ('~' + EQ))
	return (3);
    if (c == '&')
	return (2);
    if (c == '|')
	return (1);
    return (0);
}

static int
do_op_add(VAR *vs, int m)
{
    double *p1, *p2;
    int i, nr, nc, nn;

    p1 = vs[m].data;
    p2 = vs[m + 1].data;
    nr = vs[m].rows;        
    nc = vs[m].cols;        
    nn = nr * nc;
    for (i = 0; i < nn; i++) {
        p1[i] += p2[i];
    }
    return (0);
}

static int
do_op_sub(VAR *vs, int m)
{
    double *p1, *p2;
    int i, nr, nc, nn;

    p1 = vs[m].data;
    p2 = vs[m + 1].data;
    nr = vs[m].rows;        
    nc = vs[m].cols;        
    nn = nr * nc;
    for (i = 0; i < nn; i++) {
        p1[i] -= p2[i];
    }
    return (0);
}

static int
do_op_mul(VAR *vs, int m)
{
    double dv, *p0, *p1, *p2;
    int i, j, k, n, n0, n1, n2, nn;

    n = m + 1;
    n1 = vs[m].rows * vs[m].cols; 
    n2 = vs[n].rows * vs[n].cols; 
    p1 = vs[m].data;
    p2 = vs[n].data;
    if ((n1 == 1) && (n2 == 1)) {
        vs[m].rows = 1;        
        vs[m].cols = 1;        
        p1[0] *= p2[0];        
    } else if (n1 == 1) {
        vs[m].rows = vs[n].rows;
        vs[m].cols = vs[n].cols;
        dv = p1[0];
        for (i = 0; i < n2; i++) {
            p1[i] = dv * p2[i];
        }
    } else if (n2 == 1) {
        for (i = 0; i < n1; i++) {
            p1[i] *= p2[0];
        }
    } else if (vs[m].cols == vs[n].rows) {
        n0 = vs[m].rows; 
        n1 = vs[m].cols; 
        n2 = vs[n].cols; 
        nn = n0 * n2;
        if (nn > MAX_DAT) nn = MAX_DAT;
        p0 = calloc(nn, sizeof(double));
        for (i = 0; i < n0; i++) {
            for (j = 0; j < n2; j++) {
                dv = 0;
                for (k = 0; k < n1; k++) {
                    dv += p1[i + k * n0] * p2[k + j * n1];
                }
                p0[i + j * n0] = dv;
            }
        }
        vs[m].cols = n2; 
        memcpy(p1, p0, nn * sizeof(double));
    } else {
        return (-1);
    }
    return (0);
}

static int
do_op_mul_ebe(VAR *vs, int m)
{
    double dv, *p1, *p2;
    int i, n, n1, n2, nn;

    n = m + 1;
    n1 = vs[m].rows * vs[m].cols; 
    n2 = vs[n].rows * vs[n].cols; 
    p1 = vs[m].data;
    p2 = vs[n].data;
    if ((n1 == 1) && (n2 == 1)) {
        vs[m].rows = 1;        
        vs[m].cols = 1;        
        p1[0] *= p2[0];        
    } else if (n1 == 1) {
        vs[m].rows = vs[n].rows;
        vs[m].cols = vs[n].cols;
        dv = p1[0];
        for (i = 0; i < n2; i++) {
            p1[i] = dv * p2[i];
        }
    } else if (n2 == 1) {
        for (i = 0; i < n1; i++) {
            p1[i] *= p2[0];
        }
    } else if (n1 == n2) {
        nn = n1 * n2;
        for (i = 0; i < nn; i++) {
            p1[i] *= p2[i];
        }
    } else {
        return (-1);
    }
    return (0);
}

static int
do_op_div(VAR *vs, int m)
{
    double dv, *p1, *p2;
    int i, n, n1, n2;

    n = m + 1;
    n1 = vs[m].rows * vs[m].cols; 
    n2 = vs[n].rows * vs[n].cols; 
    p1 = vs[m].data;
    p2 = vs[n].data;
    if ((n1 == 1) && (n2 == 1)) {
        vs[m].rows = 1;        
        vs[m].cols = 1;        
        p1[0] /= p2[0];        
    } else if (n1 == 1) {
        vs[m].rows = vs[n].rows;
        vs[m].cols = vs[n].cols;
        dv = p1[0];
        for (i = 0; i < n2; i++) {
            if (p2[i] == 0) return (-1);
            p1[i] = dv / p2[i];
        }
    } else if (n2 == 1) {
        if (p2[0] == 0) return (-1);
        for (i = 0; i < n1; i++) {
            p1[i] /= p2[0];
        }
    } else {
        return (-1);
    }
    return (0);
}

static int
do_op_div_ebe(VAR *vs, int m)
{
    double dv, *p1, *p2;
    int i, n, n1, n2, nn;

    n = m + 1;
    n1 = vs[m].rows * vs[m].cols; 
    n2 = vs[n].rows * vs[n].cols; 
    p1 = vs[m].data;
    p2 = vs[n].data;
    if ((n1 == 1) && (n2 == 1)) {
        vs[m].rows = 1;        
        vs[m].cols = 1;        
        p1[0] /= p2[0];        
    } else if (n1 == 1) {
        vs[m].rows = vs[n].rows;
        vs[m].cols = vs[n].cols;
        dv = p1[0];
        for (i = 0; i < n2; i++) {
            if (p2[i] == 0) return (-1);
            p1[i] = dv / p2[i];
        }
    } else if (n2 == 1) {
        if (p2[0] == 0) return (-1);
        for (i = 0; i < n1; i++) {
            p1[i] /= p2[0];
        }
    } else if (n1 == n2) {
        nn = n1 * n2;
        for (i = 0; i < nn; i++) {
            if (p2[i] == 0) return (-1);
            p1[i] /= p2[i];
        }
    } else {
        return (-1);
    }
    return (0);
}

static int
do_op_pow(VAR *vs, int m)
{
    double dv, *p1, *p2;
    int i, n, n1, n2, nn;

    n = m + 1;
    n1 = vs[m].rows * vs[m].cols; 
    n2 = vs[n].rows * vs[n].cols; 
    nn = n1 * n2;
    p1 = vs[m].data;
    p2 = vs[n].data;
    if (nn == 1 && (n2 == 1)) {
        vs[m].rows = 1;        
        vs[m].cols = 1;        
        p1[0] = pow(p1[0], p2[0]);        
    } else if (n1 == 1) {
        vs[m].rows = vs[n].rows;        
        vs[m].cols = vs[n].cols;        
        dv = p1[0];
        for (i = 0; i < n2; i++) {
            p1[i] = pow(dv, p2[i]);        
        }
    } else if (n2 == 1) {
        for (i = 0; i < n1; i++) {
            p1[i] = pow(p1[i], p2[0]);        
        }
    } else {
        return (-1);
    }
    return (0);
}

static int
do_op_pow_ebe(VAR *vs, int m)
{
    double dv, *p1, *p2;
    int i, n, n1, n2, nn;

    n = m + 1;
    n1 = vs[m].rows * vs[m].cols; 
    n2 = vs[n].rows * vs[n].cols; 
    nn = n1 * n2;
    p1 = vs[m].data;
    p2 = vs[n].data;
    if (nn == 1 && (n2 == 1)) {
        vs[m].rows = 1;        
        vs[m].cols = 1;        
        p1[0] = pow(p1[0], p2[0]);        
    } else if (n1 == 1) {
        vs[m].rows = vs[n].rows;        
        vs[m].cols = vs[n].cols;        
        dv = p1[0];
        for (i = 0; i < n2; i++) {
            p1[i] = pow(dv, p2[i]);        
        }
    } else if (n2 == 1) {
        for (i = 0; i < n1; i++) {
            p1[i] = pow(p1[i], p2[0]);        
        }
    } else if (n1 == n2) {
        for (i = 0; i < n1; i++) {
            p1[i] = pow(p1[i], p2[i]);        
        }
    } else {
        return (-1);
    }
    return (0);
}

static int
do_op_transpose(VAR *vs, int m)
{
    double *p1, *p2;
    int i, j, nr, nc, nn;

    nr = vs[m].rows;
    nc = vs[m].cols; 
    nn = nr * nc;
    if (nn == 1) return (0);
    p1 = vs[m].data;
    p2 = calloc(nn, sizeof(double));
    for (i = 0; i < nr; i++) {
        for (j = 0; j < nc; j++) {
            p2[i + j * nr] = p1[j + i * nc];
        }
    }
    vs[m].rows = nc;
    vs[m].cols = nr; 
    memcpy(p1, p2, nn * sizeof(double));
    free(p2);
    return (0);
}

static int
do_op(VAR *vs, int *opst, int *pnnbs, int *pnops, int *pi)
{
    int     i, e = 0;

    i = (*pi) - 1;
    switch (opst[i] & 255) {
    case '+':
        do_op_add(vs, i);
	break;
    case '-':
        do_op_sub(vs, i);
	break;
    case '*':
        e = do_op_mul(vs, i);
	break;
    case '/':
        e = do_op_div(vs, i);
	break;
    case '^':
        e = do_op_pow(vs, i);
	break;
    case '*' + EBE:
        e = do_op_mul_ebe(vs, i);
	break;
    case '/' + EBE:
        e = do_op_div_ebe(vs, i);
	break;
    case '^' + EBE:
        e = do_op_pow_ebe(vs, i);
	break;
    case '%':
	vs[i].data[0] = fmod(vs[i].data[0],vs[i + 1].data[0]);
	break;
    case '<':
	vs[i].data[0] = (vs[i].data[0] < vs[i + 1].data[0]);
	break;
    case '>':
	vs[i].data[0] = (vs[i].data[0] > vs[i + 1].data[0]);
	break;
    case '&':
 	vs[i].data[0] = (vs[i].data[0] && vs[i + 1].data[0]);
	break;
    case '|':
	vs[i].data[0] = (vs[i].data[0] || vs[i + 1].data[0]);
	break;
    case '~':
	vs[i].data[0] = !vs[i + 1].data[0];
	break;
    case ('<' + EQ):
	vs[i].data[0] = (vs[i].data[0] <= vs[i + 1].data[0]);
	break;
    case ('>' + EQ):
	vs[i].data[0] = (vs[i].data[0] >= vs[i + 1].data[0]);
	break;
    case ('=' + EQ):
	vs[i].data[0] = (vs[i].data[0] == vs[i + 1].data[0]);
	break;
    case ('~' + EQ):
	vs[i].data[0] = (vs[i].data[0] != vs[i + 1].data[0]);
	break;
    }
    (*pnnbs)--;
    for (i = *pi; i < *pnnbs; i++) {
        vs[i].data[0] = vs[i + 1].data[0];
    }
    (*pnops)--;
    (*pi)--;
    for (i = *pi; i < *pnops; i++)
	opst[i] = opst[i + 1];
    if (*pnnbs < 0 || *pnops < 0 || *pi < 0)
	e = -1;
    return (e);
}

static int
ctof(s, v)
char   *s;
double *v;
{
    char   *b, n;
    double  d, p;

    b = s;
    d = 0;
    while ('0' <= *s && *s <= '9') {
	d *= 10;
	d += *s - '0';
	s++;
    }
    if (*s == '.') {
	s++;
	p = 1;
	while ('0' <= *s && *s <= '9') {
	    p /= 10;
	    d += (*s - '0') * p;
	    s++;
	}
    }
    if (*s == 'e' || *s == 'E') {
	s++;
	n = 0;
	if (*s == '+') {
	    s++;
	} else if (*s == '-') {
	    n = 1;
	    s++;
	}
	p = 0;
	while ('0' <= *s && *s <= '9') {
	    p *= 10;
	    p += *s - '0';
	    s++;
	}
	if (p > 0)
	    d *= pow(10.0, n ? -p : p);
    }
    *v = d;
    return (s - b);
}

/**********************************************************************/

#define FN_SIN  0
#define FN_COS  1
#define FN_TAN  2
#define FN_LOG  3
#define FN_LN   4
#define FN_MIN  5
#define FN_MAX  6
#define FN_ABS  7
#define FN_LMT  8
#define FN_IFE  9
#define FN_ATN  10
#define FN_SQR  11
#define FN_FLR  12
#define FN_CEL  13
#define FN_EXP  14
#define FN_TNH  15
#define FN_ATH  16
#define FN_SEL  17

int
lookup_func(s)
char   *s;
{
    int     i;
    static char *fn[] = {
	"sin",
	"cos",
	"tan",
	"log",
	"ln",
	"min",
	"max",
	"abs",
	"limit",
	"ifelse",
	"atan",
	"sqrt",
	"floor",
	"ceil",
	"exp",
	"tanh",
	"atanh",
	"select",
    };
    static int nfn = sizeof(fn) / sizeof(fn[0]);

    for (i = 0; i < nfn; i++)
	if (strncmp(s, fn[i], strlen(fn[i])) == 0) {
	    return (i);
        }
    return (-1);
}

double
inv_tanh(double x)
{
    return (log(fabs((1 + x)/(1 - x))) / 2);
}

int
eval_func(int i, int n, double *a, double *v)
{
    if (n <= 0)
	return (8);
    switch (i) {
    case FN_SIN:
	*v = sin(*a);
	break;
    case FN_COS:
	*v = cos(*a);
	break;
    case FN_TAN:
	*v = tan(*a);
	break;
    case FN_LOG:
	if (*a <= 0)
	    return (9);
	*v = log10(*a);
	break;
    case FN_LN:
	if (*a <= 0)
	    return (9);
	*v = log(*a);
	break;
    case FN_MIN:
	*v = a[--n];
	while (--n >= 0)
	    if (*v > a[n])
		*v = a[n];
	break;
    case FN_MAX:
	*v = a[--n];
	while (--n >= 0)
	    if (*v < a[n])
		*v = a[n];
	break;
    case FN_ABS:
	*v = (a[0] < 0) ? -a[0] : a[0];
	break;
    case FN_LMT:
	*v = (a[1] < a[0]) ? a[0] : (a[1] > a[2]) ? a[2] : a[1];
	break;
    case FN_IFE:
	*v = a[0] ? a[1] : a[2];
	break;
    case FN_ATN:
	*v = (n < 2) ? atan(a[0]) : atan2(a[0], a[1]);
	break;
    case FN_SQR:
	*v = (a[0] > 0) ? sqrt(a[0]) : 0;
	break;
    case FN_FLR:
	*v = floor(a[0]);
	break;
    case FN_CEL:
	*v = ceil(a[0]);
	break;
    case FN_EXP:
	*v = exp(a[0]);
	break;
    case FN_TNH:
	*v = tanh(a[0]);
	break;
    case FN_ATH:
	*v = inv_tanh(a[0]);
	break;
    case FN_SEL:
        i = round(a[0]);
	if ((i < 1) || (i > n)) {
            return (15);
        }
	*v = a[i];
	break;
    }
    return (0);
}

int
eval_func_var(int fn, int na, VAR *a, double *v, int *pnr, int *pnc)
{
    double fa[MAXNARG];
    int j, k, sz, e = 0, mxsz = 0;

    for (k = 0; k < na; k++) {
        sz = a[k].rows * a[k].cols;
        if (mxsz < sz) {
            mxsz = sz;
        }
    }
    sz = a[0].rows * a[0].cols;
    if ((fn == FN_SEL) && (sz == 1)) {
        k = round(a[0].data[0]);
        if ((k < 1) || (k > na)) {
            return (15);
        }
        sz = a[k].rows * a[k].cols;
        memcpy(v, a[k].data, sz * sizeof(double));
        *pnr = a[k].rows;
        *pnc = a[k].cols;
        return (0);
    }
    for (k = 0; k < na; k++) {
        sz = a[k].rows * a[k].cols;
        if ((sz != mxsz) && (sz != 1)) {
            return (14);
        }
        if (sz == mxsz) {
            *pnr = a[k].rows;
            *pnc = a[k].cols;
        }
    }
    for (j = 0; j < mxsz; j++) {   
        for (k = 0; k < na; k++) {
            sz = a[k].rows * a[k].cols;
            fa[k] = (sz == 1) ? a[k].data[0] : a[k].data[j];
        }
        e = eval_func(fn, na, fa, v + j);
    }
    return (e);
}

int
parse_args(char *s, VAR *va, int *n)
{
    double  *data;
    int     b, c = 0, i, nr, nc;
    char   *t;

    t = s;
    if (*t == '(') {
	for (i = 0; i < MAXNARG; i++) {
            data = va[i].data;
            b = eval_expr(++t, data, &nr, &nc, ",)", MAXNARG);
            va[i].rows = nr;
            va[i].cols = nc;
	    if (b <= 0)
		return (-1);
	    t += b;
	    c++;
	    if (*t == ')') break;
	}
	t++;
    }
    *n = c;
    return (t - s);
}

/**********************************************************************/

static void
expr_err(s, e, c)
char   *s, *e;
int     c;
{
    char *msg;
    int     i, m, n;
    static int errcnt = 0;

    if (errcnt++ > MAX_ERR)
	return;
    n = (int) (e - s);
    switch (c) {
    case 1:
	e = "operators with no numbers";
	break;
    case 2:
	e = "can't do operation";
	break;
    case 3:
	e = "numbers with no operator";
	break;
    case 4:
	e = "unbalanced parentheses";
	break;
    case 5:
	e = "unknown internal variable";
	break;
    case 6:
	e = "unknown function or variable";
	break;
    case 7:
	e = "unexpected character";
	break;
    case 8:
	e = "function has no arguments";
	break;
    case 9:
	e = "log argument is zero or negative";
	break;
    case 10:
	e = "unable to evaluate macro";
	break;
    case 11:
	e = "invalid binary operator";
	break;
    case 12:
	e = "operation parsing error";
	break;
    case 13:
	e = "";
	break;
    case 14:
	e = "function argument dimension mismatch";
	break;
    case 15:
	e = "select range error";
	break;
    case 16:
	e = "transpose error";
	break;
    }
    m = MAX_MSG - strlen(e) - 1;
    if (n > m) n = m;
    msg = err_msg;
    for (i = 0; i < n; i++) *msg++ = ' ';
    *msg++ = '^';
    sprintf(msg, " %s\n", e);
}

int
get_expr_err(char *msg, int mx)
{
    if (*msg == 0) return (0);
    strncpy(msg, err_msg, mx);
    *err_msg = 0;
    return (strlen(msg));
}

/**********************************************************************/

int
eval_expr(char *ps, double *pn, int *pr, int *pc, char *dlm, int mx)
{
    char   *s, *op, *pop;
    double  v[MAX_DAT];
    int     opst[MAXNOPS];
    int     b, e, i, na, nr, nc, nn, nnbs = 0, nops = 0;
    VAR     vs[MAXNNBS], va[MAXNNBS];

    pop = ps;
    for (s = ps; !(*s == 0 || inset(*s, dlm));) {
	s = skipwhite(s);
	if (*s == 0 || inset(*s, dlm)) {
	    break;
	} else if (*s == '\'') {              /* transpose */
            if (nnbs <= nops) {
		expr_err(ps, s, 16);
		return (-1);
	    }
            do_op_transpose(vs, nnbs - 1);
            s++;
	} else if (inset(*s, "+-*/%^<>&|=~.")) {
	    if ((nops >= MAXNOPS)) {
		expr_err(ps, s, 1);
		return (-1);
	    }
            op = s;
	    if (nops < nnbs) {                /* binary op */
                if (inset(*s, "<>=~") && (s[1] == '=')) {
    		    opst[nops++] = *s++ + EQ;
                } else if ((*s == '.') && inset(s[1], "*/^")) {
    		    opst[nops++] = *++s + EBE;
		} else if (inset(*s, "=~") || (s[1] == '=')) {
		    expr_err(ps, s, 11);
		    return (-1);
		} else {
		    opst[nops++] = *s;
		}
	    } else if (*s == '-') {           /* unary minus */
                stack_scalar(nnbs, -1);
		opst[nops++] = '*';
	    } else if (*s == '~') {           /* unary negate */
                stack_scalar(nnbs, 0);
		opst[nops++] = *s;
	    } else if (*s == '\'') {	      /* unary transpose */
                stack_scalar(nnbs, 0);
		opst[nops++] = *s;
	    } else if (*s != '+') {	      /* unary plus */
		expr_err(ps, s, 1);
		return (-1);
	    }
	    for (i = 1; i < nops; i++) {
		if (op_prec(opst[i - 1]) >= op_prec(opst[i])) {
		    if (do_op(vs, opst, &nnbs, &nops, &i) < 0) {
			expr_err(ps, pop, 2);
			return (-1);
		    }
		}
	    }
            pop = op;
	    s++;
	} else if (*s == '(') {
	    if (nnbs >= MAXNNBS || nnbs > nops) {
		expr_err(ps, s, 3);
		return (-1);
	    }
	    s++;
	    b = eval_expr(s, vs[nnbs].data, &nr, &nc, ")", MAX_DAT);
            vs[nnbs].rows = nr;
            vs[nnbs].cols = nc;
            nnbs++;
	    if (b <= 0) {
		return (-1);
            }
	    s = s + b;
	    if (*s != ')') {
		expr_err(ps, s, 4);
		return (-1);
	    }
	    s++;
	} else if (*s == ')') {
	    expr_err(ps, s, 4);
	    return (-1);
	} else if (*s == '[') {
	    if (nnbs >= MAXNNBS || nnbs > nops) {
		expr_err(ps, s, 3);
		return (-1);
	    }
	    b = get_array(s, v, &nr, &nc, MAX_DAT);
	    if (b <= 0) {
		return (-1);
            }
            stack_array(nnbs,v,nr,nc);
	    s = s + b;
	} else if (*s == ']') {
	    expr_err(ps, s, 4);
	    return (-1);
	} else if (*s == '.' || (*s >= '0' && *s <= '9')) {
	    if (nnbs >= MAXNNBS || nnbs > nops) {
		expr_err(ps, s, 3);
		return (-1);
	    }
	    s += ctof(s, v);
	    stack_scalar(nnbs, v[0]);
	} else if ((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z')) {
	    if (nnbs >= MAXNNBS || nnbs > nops) {
		expr_err(ps, s, 3);
		return (-1);
	    }
	    if ((i = lookup_func(s)) >= 0) {
                s = skipalphnum(s);
		if ((b = parse_args(s, va, &na)) < 0) {
		    return (-1);
                }
		e = eval_func_var(i, na, va, v, &nr, &nc);
		if (e != 0) {
		    expr_err(ps, s, e);
		    return (-1);
                }
                stack_array(nnbs,v,nr,nc);
		s += b;
            } else if ((b = usr_var_chk(s, v, &nr, &nc, MAX_DAT))) {
                stack_array(nnbs,v,nr,nc);
		s = s + b;
	    } else {
		expr_err(ps, s, 6);
		return (-1);
	    }
	} else {
	    expr_err(ps, s, 7);
	    return (-1);
	}
	if (nnbs > (nops + 1) || nnbs < (nops - 1)) {
	    expr_err(ps, s, 12);
	    return (-1);
        }
    }
    for (i = nops; i > 0;) {
	if (do_op(vs, opst, &nnbs, &nops, &i) < 0) {
	    expr_err(ps, pop, 2);
	    return (-1);
        }
    }
    if (nops > 0 || s <= ps) {
	expr_err(ps, s, 12);
	return (-1);
    }
    if (nnbs == 1) {
	nn = nr * nc;
        if (nn > MAX_DAT) nn = MAX_DAT;
	if (nn > mx) {
	    nr = 1;
	    nc = mx;
	} else {
	    nr = vs[0].rows;
	    nc = vs[0].cols;
        }
        nr = vs[0].rows;
        nc = vs[0].cols;
	nn = nr * nc;
	*pr = nr;
	*pc = nc;
	memcpy(pn, vs[0].data, nn * sizeof(double));
    }
    b = (int)(s - ps);
    *err_msg = 0;
    return (b);
}
