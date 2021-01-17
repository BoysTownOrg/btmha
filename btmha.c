// btmha.c - main file for Boys Town Master Hearing Aid program

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#include <sigpro.h>
#include "btmha.h"
#include "eval.h"
#include "stream.h"
#include "var_list.h"
#include "version.h"

#define MAX_INC         10
#define MAX_STA         1

#ifdef WIN32
#include <io.h>
#include <direct.h>
#ifndef R_OK
#define R_OK            4
#endif
#define INTERACTIVE     1
#else
#include <unistd.h>
#define _access         access
#define _chdir          chdir
#define _getcwd         getcwd
#define _strdup         strdup
#define INTERACTIVE     0
#endif

char* skipwhite(char*);
char* stream_version();
char* plugin_version();
int process_config(VAR *, char *, int);
int is_processor(char *, char *, int *);
int optimize_config(VAR *, char *);
void optimize_plugin(I_O *, STA *, VAR *, int, double, char *);
void plugin_config(VAR *, char *, char *, int);
void plugin_show(char, char *, char *, int);
void process_prepare(STA *, VAR *, MHA *, char *, int *, char *);
void process_cleanup();
void process_init();
void process_show(VAR *, char *, char *);

//===========================================================

static void (*processor_report)(void **, char *);

static char *banner = "==== Boys Town Master Hearing Aid ====";
static char* path_dirs[8] = { "", ""};
static char quit = 0;
static char msg[MAX_MSG];
static int prepared = 0;
static int npath = 2;
static int processed = 0;
static int line_number = 0;
static int interactive_mode = INTERACTIVE;
static int inclev = 0;
static FILE *lfp_lst[MAX_INC];
static VAR *lvl = NULL;
static VAR *ovl = NULL;
static STA state[MAX_STA];
static MHA def_mha = {
    24000,
    32, 0, 1, 1, 0, 40, 1, 0,
    "mixer", "capture", "playback",
    1.1219e-6, 65
};
static MHA mha = {0};
static I_O io = {0};

//===========================================================

static void
usage()
{
    printf("usage: btmha [-options] [list_file] ...\n");
    printf("options\n");
    printf("-h    display usage info\n");
    printf("-i    interactive mode\n");
    printf("-q    quiet mode\n");
    printf("-v    display version\n");
    exit(0);
}

static void
version()
{
    printf("%s\n", VER);
    printf("%s\n", NOTICE);
    exit(0);
}

//===========================================================

static int
is_non_numeric(char *line)
{
    char *s, *e;
    int i, n, nn = 0;
    static char *nnp[] = {     // non-numeric parameters
        "input", "output", "process", "datfile",
        "plugin", "mixer", "chain", "bank"};
    static int np = sizeof(nnp) / sizeof(char *);

    // check whether LHS appears on list of non-numeric parameters
    s = skipwhite(line);
    for (i = 0; i < np; i++) {
        n = strlen(nnp[i]);
        if (strncmp(s, nnp[i], n) == 0) {
            e = skipwhite(s + n);
            if (e[0] == '=') {
                nn++;
                break;
            }
            e = skipwhite(s + n + 1);
            if (isdigit(s[n]) && (e[0] == '=')) {
                nn++;
                break;
            }
        }
    }
    return (nn);
}

static void
process_replace(char *line, VAR *vl)
{
    char *s, name[80];
    int i, j, n;
    static char *nnp[] = {     // processors
        "plugin", "mixer", "chain", "bank"};
    static int np = sizeof(nnp) / sizeof(char *);

    // check if LHS appears in list of processors
    s = skipwhite(line);
    for (i = 0; i < np; i++) {
        n = strlen(nnp[i]);
        if (strncmp(s, nnp[i], n) == 0) {
            if (s[n] == '0') {
                j = var_list_index(lvl, nnp[i]);
                if (j >= 0) {
                    var_list_delete(lvl, nnp[i]);
                    printf("%s0 replaces %s\n", nnp[i], nnp[i]);
                }
            } else if (!isdigit(s[n])) {
                strcpy(name, nnp[i]);
                strcat(name, "0");
                j = var_list_index(lvl, name);
                if (j >= 0) {
                    var_list_delete(lvl, name);
                    printf("%s replaces %s0\n", nnp[i], nnp[i]);
                }
            }
        }
    }
}

//===========================================================

static int
read_list(char *fn, char quiet)
{
    char line[256];
    FILE *lfp;

    if (inclev >= MAX_INC) {
        return (0);
    }
    lfp = fopen(fn, "rt");
    if (lfp == NULL) {
        fprintf(stderr, "Can't open list file (%s).\n", fn);
        return (0);
    }
    lfp_lst[inclev++] = lfp;
    while(inclev) {
        lfp = lfp_lst[inclev - 1];
        if (fgets(line, 256, lfp) == NULL) {
            fclose(lfp);
            inclev--;
            continue;
        }
        trim_line(line);
        if (strlen(line) > 0) {
            parse_line(line, quiet);
        }
        if (quit) break;
    }

    return (1);
}

static int 
find_file(char *line, char quiet)
{
    char s[256];
    int d, k, r = 0;
    static char* ext = ".lst";

    d = (strchr(line, '.') != NULL);
    for (k = 0; k < npath; k++) {
        if (k == 0) {
            s[0] = 0;
        } else {
            strcpy(s, path_dirs[k]);
        }
        strcat(s, line);
        if (!d) strcat(s, ext);
        if (_access(s, R_OK) == 0) {
            r = read_list(s, quiet);
            break;
        }
    }
    return (r);
}

//===========================================================

static void
write_wave(I_O *io)
{
    float r[1], *w;
    int   c, n, nbits = 16;
    static VAR *vl;

    printf("%12s: %s\n", "WAV output", io->ofn);
    remove(io->ofn);
    c = io->noch;
    n = io->nwav;
    w = io->owav;
    r[0] = (float) io->rate;
    vl = sp_var_alloc(8);
    sp_var_add(vl, "rate",        r,       1, 1, "f4");
    sp_var_add(vl, "wave",        w,       n, c, "f4");
    vl[1].dtyp = SP_DTYP_F4; /* workaround sigpro bug */
    sp_wav_write(io->ofn, vl + 1, r, nbits);
    sp_var_clear(vl);
}

static void
reset_mha(char quiet)
{
    mha = def_mha;
    stream_init(&mha); // initialize stream variables
    if (!quiet) puts(banner);
}

//===========================================================

static void
do_plugin(char *line)
{
    char plug[20];
    int n;

    strcpy(plug, "plugin");
    if (line && strcmp(line, plug) == 0) {
        process_show(lvl, line, plug);
    } else if (is_processor(line, plug, &n)) {     // configure plugin ?
        plugin_config(lvl, line, plug, n);
        plugin_show(1, line, plug, n);
    }
}

static void
do_help()
{
    int i;
    static char* cmd[] = {
        "bank       Displays definitions of bank processors",
        "chain      Displays definitions of chain processors",
        "clear      Deletes all list variables",
        "device     Displays available audio devices and indicates selected device",
        "help       Displays list of BTMHA commands",
        "intersect  Displays intersections of list variables with plugin variables",
        "stream     Displays input / output variables",
        "list       Displays all list variables",
        "mixer      Displays definitions of mixer processors",
        "optimize   Optimizes AFC parameters for example in tst_gha.lst",
        "play       Streams file to audio device [default=input; @=output]",
        "plugin     Displays names of plugin processors",
        "prepare    Initializes state for primary processor",
        "quit       Exits the program",
        "sndchk     Outputs sound check",
        "start      Initiates processing from input to output",
        "ver        Reports version numbers of BTMHA software components"
    };
    static int ncmd = sizeof(cmd) / sizeof(char*);

    printf("List of commands:\n");
    for (i = 0; i < ncmd; i++) {
        printf("  %s\n", cmd[i]);
    }
}

static void
do_intersect()
{
    char plug[20], line[40];
    int n;

    strcpy(plug, "plugin");
    for (n = 0; n < 10; n++) {
        sprintf(line, "%s%d", plug, n);
        plugin_config(lvl, line, plug, n);
        plugin_show(2, line, plug, n);
    }
}

static void
do_device()
{
    mha.io_dev = stream_device(mha.io_dev);
}

static void
do_assign(char *line)
{
    int nn = is_non_numeric(line);
    if (var_list_assign(lvl, line, nn, interactive_mode)) {
        process_replace(line, lvl);
        stream_replace(&mha, lvl);
        prepared = processed = 0;
    }
}

static void
do_list(char *line)
{
    if (var_list_length(lvl) > 0) {
        printf("list:\n");
        var_list_show(lvl, line);
    } else {
        printf("List is empty.\n");
    }
}

static void
do_stream(char *line)
{
    printf("stream:\n");
    stream_show(line);
}

static void
do_prepare(char quiet)
{
    char *p;
    int nr, nc, ni, no;
    int32_t rc[2];

    prepared = processed = 0;
    // configure processor
    p = mha.process;            // processor name
    if (p == NULL) {
        printf("process not specified\n");
        return;
    }
    if (process_config(lvl, p, quiet)) {
        printf("configure failed\n");
        return;
    }
    if (!quiet) {
        printf("    %-16s = %s\n", "process", p);
    }
    // prepare I/O
    if (stream_prepare(&io, &mha, msg + strlen(msg))) {
        printf("I/O stream prepare failed\n");
        return;
    }
    // prepare processor state
    process_prepare(state, lvl, &mha, p, rc, msg);
    if (!quiet && *msg) {
        printf("%s", msg);
        *msg = 0;
    }
    if (state->proc == NULL) return;
    // check for channel number compatibility
    nr = rc[0];
    nc = rc[1];
    ni = mha.nich;
    no = mha.noch;
    if (nr != ni) {
        printf("ERROR: %s has wrong number of ", p);
        printf("input channels (%d != %d)\n", nr, ni);
        state->proc = NULL;
    }
    if (nc != no) {
        printf("ERROR: %s has wrong number of ", p);
        printf("output channels (%d != %d)\n", nc, no);
        state->proc = NULL;
    }
    if (state->proc) prepared++;
}

static void
do_process(char quiet)
{
    do_prepare(1);
    if (!prepared) {
        if (*msg) {
            printf("ERROR: processor prepare failed\n");
            *msg = 0;
        }
        return;
    }
    if (!quiet && *msg) {
        printf("%s", msg);
        *msg = 0;
    }
    stream_start(&io, state);
    if (state->rprt) {
        processor_report = state->rprt;
        processor_report(state->cp, msg);
    }
    if (io.ofn) write_wave(&io);
    if (!quiet && *msg) {
        printf("%s", msg);
        *msg = 0;
    }
    processed++;
}

static void
do_optimize(char *line)
{
    double tqm;
    int i, n = 0, nov;

    // initialize parameters to optimize
    plugin_config(lvl, "plugin", "plugin", n);
    var_list_replace(ovl, lvl);
    // perform optimization
    nov = optimize_config(ovl, line);
    printf("%12s:", "optimize");
    for (i = 0; i < nov; i++) {
        printf(" %s", ovl[i].name);
    }
    printf("\n");
    do_prepare(0);
    if (!prepared) {
        if (*msg) {
            printf("%s", msg);
            *msg = 0;
        }
        return;
    }
    tqm = var_list_f8n(lvl, "tqm");
    optimize_plugin(&io, state, ovl, nov, tqm, msg);
    if (*msg) {
        printf("%s", msg);
        *msg = 0;
    }
    processed++;
}

static void
do_mixer(char *line)
{
    process_show(lvl, line, "mixer");
}

static void
do_cd(char* line)
{
    char s[256];

    line = skipwhite(skipalphnum(line));
    if (*line == '~') {
        _chdir(path_dirs[0]);
    } else if (*line == '@') {
        _chdir(path_dirs[1]);
    } else if (strlen(line)) {
        _chdir(line);
    }
    _getcwd(s, 256);
    printf("cd = '%s'\n", s);
}

static void
do_chain(char* line)
{
    process_show(lvl, line, "chain");
}

static void
do_bank(char *line)
{
    process_show(lvl, line, "bank");
}

static void
do_sndchk()
{
    sound_check(mha.sr, mha.io_dev - 1);
    processed++;
}

static void
do_play(char *line)
{
    char* arg;
    int out;

    if (!prepared) do_prepare(1);
    io.iod = mha.io_dev - 1;
    arg = skipwhite(skipalphnum(line));
    out = (*arg == '@');
    play_wave(&io, out);
    processed++;
}

static void
do_clear(char quiet)
{
    var_list_clear(lvl);
}

static void
do_reset(char quiet, char *line)
{
    line = skipwhite(skipalphnum(line));
    if (*line == '~') {
        _chdir(path_dirs[0]);
    } else if (*line == '@') {
        _chdir(path_dirs[1]);
    }
    var_list_clear(lvl);
    reset_mha(quiet);
}

static void
do_start(char quiet)
{
    if (!quiet) printf("start...\n");
    do_process(quiet);
}

static void
do_ver()
{
    printf("%s\n", VER);
    printf("%s\n", sp_version());
    printf("%s\n", plugin_version());
    printf("%s\n", stream_version());
}

//===========================================================

static int
cmd_prefx(char* line, char* cmd)
{
    int n;
    n = strlen(cmd);
    return (strncmp(line, cmd, n) == 0);
}

static int
cmd_match(char* line, char* cmd)
{
    char* e;
    int n;

    n = strlen(cmd);
    e = skipalphnum(line);
    return ((strncmp(line, cmd, n) == 0) && (*e == 0 || isspace(*e)));
}

void
parse_line(char *line, char quiet)
{
    line_number++;
    line = _strdup(line);                       // create local copy
    trim_line(line);
    if (*line == 0) {                           // empty line
        ;
    } else if (strchr(line, '=')) {             // assignment
        do_assign(line);
    } else if (cmd_prefx(line, "bank")) {       // bank command
        do_bank(line);
    } else if (cmd_match(line, "cd")) {         // cd command
        do_cd(line);
    } else if (cmd_prefx(line, "chain")) {      // chain command
        do_chain(line);
    } else if (cmd_match(line, "clear")) {      // clear command
        do_clear(0);
    } else if (cmd_prefx(line, "device")) {     // device command
        do_device();
    } else if (cmd_match(line, "help")) {       // help command
        do_help();
    } else if (cmd_match(line, "intersect")) {  // intersect command
        do_intersect();
    } else if (cmd_match(line, "list")) {       // list command
        do_list(line);
    } else if (cmd_match(line, "stream")) {     // stream command
        do_stream(line);
    } else if (cmd_prefx(line, "mixer")) {      // mixer command
        do_mixer(line);
    } else if (cmd_match(line, "optimize")) {   // optimize command
        do_optimize(line);
    } else if (cmd_match(line, "play")) {       // play command
        do_play(line);
    } else if (cmd_prefx(line, "plugin")) {     // plugin command
        do_plugin(line);
    } else if (cmd_match(line, "prepare")) {    // prepare command
        do_prepare(0);
    } else if (cmd_match(line, "reset")) {      // reset command
        do_reset(quiet, line);
    } else if (cmd_match(line, "sndchk")) {     // device command
        do_sndchk();
    } else if (cmd_match(line, "start")) {      // start command
        do_start(0);
    } else if (cmd_match(line, "quit")) {       // quit command
        processed++;
        quit++;
    } else if (cmd_match(line, "ver")) {        // ver command
        do_ver();
    } else if (find_file(line, quiet)) {
        ;
    } else if (!quiet) {
        printf("Don't understand %s.\n", line);
    }
    free(line);                           // release local copy
}

static void
parse_args(int ac, char *av[], char *quiet)
{
    while (ac > 1) {
        if (quit) break;
        if (av[1][0] == '-') {
            if (av[1][1] == 'h') {
                usage();
            } else if (av[1][1] == 'i') {
                interactive_mode = 1;
                interactive(0, &quit);
                interactive_mode = 0;
                return;
            } else if (av[1][1] == 'q') {
                *quiet = 1;
            } else if (av[1][1] == 'v') {
                version();
            }
        } else {
           parse_line(av[1], 1);
        }
        ac--;
        av++;
    }
}

static void
mha_init(char quiet)
{
    if (lvl == NULL) {
        lvl = sp_var_alloc(MAX_VAR);
        ovl = (VAR *) calloc(MAX_VAR, sizeof(VAR));
    }
    process_init();    // initialize process functions
    reset_mha(quiet);
}

static void
state_cleanup()
{
    int i;

    for (i = 0; i < MAX_STA; i++) {
        free_null(state[i].cp);
        free_null(state[i].data);
    }
}

static void
mha_cleanup()
{
    sp_var_clear_all();
    stream_cleanup();
    if (io.ifn) free(io.ifn);
    if (io.ofn) free(io.ofn);
    if (io.iwav) free(io.iwav);
    if (io.owav) free(io.owav);
    process_cleanup();
    state_cleanup();
}

//===========================================================

static void 
save_dir(char** av)
{
    char* s;

    path_dirs[0] = _getcwd(NULL, 0);
    s = _strdup(av[0]);
    path_dirs[1] = s;
    s += strlen(s);
    while (s > path_dirs[1]) {
        if ((s[-1] == '/') || (s[-1] == '\\')) {
            *s = 0;
            break;
        }
        s--;
    }
}

int
main(int ac, char **av)
{
    static char quiet = 0;

    save_dir(av);
    mha_init(quiet);
    if (ac > 1) {
        parse_args(ac, av, &quiet);
        if (!processed) {
            do_start(0);
        }
    } else {
         interactive_mode++;
         interactive(0, &quit);
    }
    mha_cleanup();
    if (!quiet) puts("done...");
    return (0);
}
