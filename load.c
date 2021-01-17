// load.c - load shared library dynamically as a plugin

#include <stdio.h>
#include <stdlib.h>
#include "btmha.h"

#ifdef WIN32
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#define RTLD_GLOBAL 0x100 /* don't hide entries          */
#define RTLD_LOCAL  0x000 /* hide entries in this module */
#define RTLD_LAZY   0x000 /* accept unresolved externs   */
#define RTLD_NOW    0x001 /* abort if unresolved externs */
#else
#include <dlfcn.h>
#endif

//===========================================================
#ifdef WIN32

static struct {
    long last_err;
    const char* fun_name;
} errmsg = { 0, NULL};

void* dlopen(const char* filnam, int flgs)
{
    HINSTANCE h;

    h = LoadLibrary((LPCWSTR)filnam);
    if (h == NULL) {
        errmsg.last_err = GetLastError();
        errmsg.fun_name = "dlopen";
    }
    return h;
}

int dlclose(void* handle)
{
    BOOL ok;
    int rc = 0;

    ok = FreeLibrary((HINSTANCE)handle);
    if (!ok) {
        errmsg.last_err = GetLastError();
        errmsg.fun_name = "dlclose";
        rc = -1;
}
    return rc;
}

void* dlsym(void* handle, const char* name)
{
    FARPROC fp;

    fp = GetProcAddress((HINSTANCE)handle, name);
    if (!fp) {
        errmsg.last_err = GetLastError();
        errmsg.fun_name = "dlsym";
    }
    return (void*)(intptr_t)fp;
}

const char* dlerror(void)
{
    static char errstr[88];

    if (errmsg.last_err) {
        sprintf(errstr, "%s error #%ld", errmsg.fun_name, errmsg.last_err);
        return errstr;
    } else {
        return NULL;
    }
}
#endif

int
load_plugin(PLUG *plugin, int pn, char verbose)
{
    char path_name[256];
    char *plugin_name;
    int i;
    void *pp = NULL;
    static char *libdir[] = {".", "/usr/local/lib"}; // search path
    static char nld = sizeof(libdir) / sizeof(char *);

    plugin_name = plugin[pn].name;
    pp = plugin[pn].pp;
    if (pp) dlclose(pp);
    dlerror();    /* Clear any existing error */
    for (i = 0; i < nld; i++) {
        sprintf(path_name, "%s/%s.so", libdir[i], plugin_name);
        pp = dlopen(path_name, RTLD_LAZY);
	if (pp) break;
    }
    plugin[pn].pp = pp;
    if (!pp) {
        if (verbose) puts(dlerror());
        return(1);
    }
    plugin[pn].configure = dlsym(pp, "configure");
    plugin[pn].prepare   = dlsym(pp, "prepare");
    return (0);
}
