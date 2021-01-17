// interactive.c - implements interactive mode of Boys Town Master Hearing Aid

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "btmha.h"

#ifdef WIN32
#define TERMIO          0
#else
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include "fk.h"
#define _strdup         strdup
#define TERMIO          1
#endif

#if TERMIO

#define TRUE    1
#define FALSE   0
#define NHIST   64

/*
 * keyboard stuff 
 */
static struct termios kbd_setup;
static struct termios kbd_reset;
static int    kbd_initted = FALSE;
static int    kbd_isatty;
static int    kbd_lastchr;
static int    kbd_filedsc;
static int    opt_act = 0;
static enum { normal, test_key, wait_key, unknown } kbd_mode = unknown;

static char *hist[NHIST] = {NULL};
static int head = 0, tail = 0, curl = 0;

static void
kbd_restore(void)
{
        if(kbd_initted && kbd_isatty && (kbd_mode != normal)) {
            tcsetattr(kbd_filedsc, opt_act, &kbd_reset);
            kbd_mode = normal;
        }
}

static void
kbd_init(void)
{
        if(!kbd_initted) {
            kbd_initted = TRUE;
            kbd_lastchr = EOF;
            kbd_filedsc = fileno(stdin);
            kbd_isatty  = isatty(kbd_filedsc);	
            if(kbd_isatty) {
        	tcgetattr(kbd_filedsc, &kbd_setup);
        	tcgetattr(kbd_filedsc, &kbd_reset);
        	kbd_setup.c_lflag &= ~(ICANON | ECHO );
        	kbd_setup.c_iflag &= ~(INLCR  | ICRNL);
        	atexit(kbd_restore);
        	kbd_mode = normal;
            }
        }
}

int 
get_ch(void)
{
        unsigned char buf[4];
        if(!kbd_initted) {
            kbd_init();
        }
        if(!kbd_isatty) {
            return(getc(stdin));
        }
        if(kbd_lastchr != EOF) {
            buf[0] = kbd_lastchr;
            kbd_lastchr = EOF;
            return(buf[0]);
        }
        if(kbd_mode != wait_key) {
            kbd_setup.c_cc[VMIN]  = 1;
            kbd_setup.c_cc[VTIME] = 0;
            if(tcsetattr(kbd_filedsc, opt_act, &kbd_setup) == EOF) return(EOF);
            kbd_mode = wait_key;
        }

        if(read(kbd_filedsc,buf,1) > 0) {
            return(buf[0]);
        }
        return(EOF);
}


static int
get_str(char *prompt, char *line, int len)
{
    char   *s;
    int     c, i, j, k, n, p;
    int     esflg = 0, vtflg = 0;

    s = line;
    n = strlen(s);     	    /* number of characters in string */
    i = n;      	    /* index of cursor within string */
    p = -1;		    /* previous cursor position */
    k = 0;      	    /* number of characters printed */
    fputc('\r', stdout);
    fputs(prompt, stdout);
    fflush(stdout);
    while ((s - line) < len) {
        if (k != n) {
            fputc('\r', stdout);
            fputs(prompt, stdout);
            for (j = 0; j < k; j++) fputc(' ', stdout);
            fputc('\r', stdout);
            fputs(prompt, stdout);
            fputs(s, stdout);
            fputc('\r', stdout);
            fputs(prompt, stdout);
            for (j = 0; j < i; j++) printf ( "\033[C");
            fflush(stdout);
        } else if (p != i) {
            p = -1;
        }
        if (i < len) {
            p = i;
        }
        k = strlen(s);
        c = get_ch();
        fflush(stdout);
        if (c == CTRL_C) {
            s[0] = CTRL_C;
            s[1] = 0;
            return (0);
        } else if (esflg && c == ESC) {
            s[0] = ESC;
            s[1] = 0;
        } else if (c == LEFT_CLICK) {
            s[0] = CTRL_O;
            s[1] = 0;
            return (0);
        } else if (esflg) {
            c |= ES;
            esflg = 0;
        } else if (vtflg) {
            c |= VT;
            vtflg = 0;
        }
        switch (c) {
        case CTRL_A:	    /* CNTL-A */
        case (FN | 71):	    /* HOME */
            i = 0;
            fputc('\r', stdout);
            fputs(prompt, stdout);
            fflush(stdout);
            break;
        case CTRL_B:	    /* CNTL-B */
        case (FN | 75):	    /* Left-Arrow */
        case (VT | 'D'):    /* Escape,[,D */
            if (i > 0)
        	i--;
            fputc('\r', stdout);
            fputs(prompt, stdout);
            for (j = 0; j < i; j++) printf ( "\033[C");
            fflush(stdout);
            break;
        case CTRL_F:	    /* CNTL-F */
        case (FN | 77):	    /* Right-Arrow */
        case (VT | 'C'):    /* Escape,[,C */
            if (i < n)
        	i++;
            fputc('\r', stdout);
            fputs(prompt, stdout);
            for (j = 0; j < i; j++) printf ( "\033[C");
            fflush(stdout);
            break;
        case 11:	    /* CNTL-K */
            n = i;
            s[n] = '\0';
            break;
        case CTRL_N:	    /* CNTL-N */
        case (FN | 80):	    /* Down-Arrow */
        case (VT | 'B'):    /* Escape,[,B */
            s[n] = CTRL_N;
            s[n + 1] = '\0';
            return (-1);
        case CTRL_P:	    /* CNTL-P */
        case (FN | 72):	    /* Up-Arrow */
        case (VT | 'A'):    /* Escape,[,A */
            s[n] = CTRL_P;
            s[n + 1] = '\0';
            return (-1);
        case 21:	    /* CNTL-U */
            n -= i;
            for (j = 0; j < n; j++)
        	s[j] = s[j + i];
            i = 0;
            s[n] = '\0';
            break;
        case CTRL_E:	    /* CNTL-E */
        case (FN | 79):	    /* END */
            i = n;
            fputc('\r', stdout);
            fputs(prompt, stdout);
            for (j = 0; j < i; j++) printf ( "\033[C");
            fflush(stdout);
            break;
        case CTRL_D:	    /* CNTL-D */
            //s[n] = EOF;
            //s[n + 1] = '\0';
            s[0] = EOF;
            s[1] = '\0';
            return (0);
        case (FN | 83):	    /* DEL */
        case 127:	    /* DEL */
            if (i < n)
        	i++;	/* fall through to next case */
#ifdef NEVER
            else
        	break;
#endif
        case 8:		    /* CNTL-H */
            if (i > 0 && i <= n) {
        	for (j = i; j < n; j++)
        	    s[j - 1] = s[j];
        	n--;
        	s[n] = 0;
                i--;
            }
            break;
        case '\r':
        case '\n':
            s[n++] = '\n';
            s[n] = 0;
            fputc('\n', stdout);
            fflush(stdout);
            return (n);
        case (ES | '['):     /* Escape,[ */
            vtflg++;
            break;
        case ESC:            /* Escape */
            esflg++;
            break;
        default:
            if ((c & 0x7F) < ' ') {
                break;
            }
            if (n < len && c != (FN | 77) && c != 6) {
        	for (j = n; j > i; j--)
        	    s[j] = s[j - 1];
        	s[i] = c;
        	n++;
        	s[n] = 0;
            }
            if (i < n) {
        	i++;
            }
        }
    }
    return (0);
}

static int
get_line(char *p, char *s, int n)
{
    int c, sl, j, l;

    curl = tail;
    *s = '\0';
    while (get_str(p, s, n) < 0) {
        sl = strlen(s);
        fputc('\r', stdout);
        fputs(p, stdout);
        for (j = 0; j < sl; j++) fputc(' ', stdout);
        fputc('\r', stdout);
        fflush(stdout);
        c = s[sl - 1];
        if (c == CTRL_N) {		/* Ctrl-N */
            l = (curl + 1) % NHIST;
            if (l == tail) {
                curl = l;
               *s = '\0';
            } else if (l != tail && hist[l]) {
                curl = l;
                strcpy(s, hist[l]);
            }
        } else if (c == CTRL_P) {	/* Ctrl-P */
            l = (curl - 1 + NHIST) % NHIST;
            if (curl != head && hist[l]) {
                curl = l;
                strcpy(s, hist[l]);
            }
        }
        sl = strlen(s);
        while (sl > 0 && s[sl - 1] <= ' ')
            s[--sl] = '\0';
    }
    if (*s == CTRL_C || *s == CTRL_O)
        return (0);
    if (*s == EOF)
        return (-1);
    sl = strlen(s);
    while ((sl > 0) && (s[sl - 1] <= ' ')) {
        s[--sl] = '\0';
    }
    if (sl > 0) {
        if (hist[tail]) {
            free(hist[tail]);
        }
        hist[tail++] = _strdup(s);
        tail %= NHIST;
        if (head == tail) {
       	head++;
    	head %= NHIST;
        }
    }
    return (sl);
}

static char *
read_line(char *prompt, char *line)
{
    int nc;

    nc = get_line(prompt, line, MAX_MSG);
    if (nc < 0) return (NULL);
    return (line);
}

#else // TERMIO

static void kbd_init(void) {}
static void kbd_restore(void) {}

static char *
read_line(char *prompt, char *line)
{
    char *rv;

    fputs(prompt, stdout);
    rv = fgets(line, MAX_MSG, stdin);
    return (rv);
}

#endif // TERMIO

int
interactive(char quiet, char *quit)
{
    char *rv, *prompt = ">> ";
    int done = 0;
    static char line[MAX_MSG];

    kbd_init();
    while (!done) {
        rv = read_line(prompt, line);
        if (!rv) {
            done++;
            continue;
        }
        trim_line(line);
        if (strlen(line) > 0) {
            parse_line(line, quiet);
        }
        if (*quit) done++;
    }
    kbd_restore();
    return (0);
}

