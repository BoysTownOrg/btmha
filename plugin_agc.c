// plugin_agc.c - AGC plugin for BTMHA

#include <stdio.h>

//===========================================================

static int proc_num;
static char *plugin_name = "plugin_agc";

//===========================================================

static int
prepare1(int arg)
{
    printf("Hello from %s:prepare 1! arg=%d\n", plugin_name, arg);
    return 0;
}

static int
prepare2(int arg)
{
    printf("Hello from %s:prepare 2! arg=%d\n", plugin_name, arg);
    return 0;
}

//===========================================================

static int
process1(int arg)
{
    printf("Greetings from %s:process 1! arg=%d\n", plugin_name, arg);
    return 0;
}

static int
process2(int arg)
{
    printf("Greetings from %s:process 2! arg=%d\n", plugin_name, arg);
    return 0;
}

//===========================================================

int
process(int arg)
{
    switch (proc_num) {
        case 1: return process1(arg);
        case 2: return process2(arg);
    }
    return 1;
}

int
prepare(int arg)
{
    switch (proc_num) {
        case 1: return prepare1(arg);
        case 2: return prepare2(arg);
    }
    return 1;
}

int
configure(void *v, void *c, int *nv, int *nc)
{
    proc_num = 1;
    return 1;
}

