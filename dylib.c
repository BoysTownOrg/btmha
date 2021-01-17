// dylib.c - dynamically load shared library

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <dlfcn.h>

//===========================================================

typedef struct {int32_t arsiz, ptsiz; void **cp; void *data; } STA;
typedef struct { double sr; int32_t cs, nsamp; } MHA;

void (*configure_plugin)(void *, int *);
void (*prepare_plugin)(STA *, void *, MHA *);
void (*process_plugin)(STA *, float *, float *);
void (*report_plugin)(char *);

static void *ph;

//===========================================================

int
mha_dylib(char *plugin_name, char verbose)
{
    char *err_msg, path_name[256];
    static char *libdir = "/usr/local/lib";

    sprintf(path_name, "%s/%s.so", libdir, plugin_name);
    if (ph) dlclose(ph);
    ph = dlopen(path_name, RTLD_LAZY);
    if (!ph) {
        if (verbose) puts(dlerror());
        return(1);
    }
    configure_plugin = dlsym(ph, "configure");
    err_msg = dlerror();
    if (err_msg != NULL) {
        if (verbose) puts(err_msg);
        return(2);
    }
    prepare_plugin = dlsym(ph, "prepare");
    err_msg = dlerror();
    if (err_msg != NULL) {
        if (verbose) puts(err_msg);
        return(2);
    }
    process_plugin = dlsym(ph, "process");
    if (err_msg != NULL) {
        if (verbose) puts(err_msg);
        return(3);
    }
    report_plugin = dlsym(ph, "report");
    if (err_msg != NULL) {
        if (verbose) puts(err_msg);
        return(3);
    }

    return (0);
}
