// plugin.c - load plugin module

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "btmha.h"

//===========================================================

void afc_bind(PLUG* plugin);
void agc_bind(PLUG* plugin);
void cfirfb_bind(PLUG* plugin);
void ciirfb_bind(PLUG* plugin);
void firfb_bind(PLUG* plugin);
void gha_bind(PLUG *plugin);
void icmp_bind(PLUG* plugin);
void iirfb_bind(PLUG* plugin);

typedef struct {void (*bind)(PLUG *); char *name; } plug_id;

static plug_id plug_ls[] = {
    {afc_bind,    "afc"   },
    {agc_bind,    "agc"   },
    {cfirfb_bind, "cfirfb"},
    {ciirfb_bind, "ciirfb"},
    {firfb_bind,  "firfb" },
    {gha_bind,    "gha"   },
    {icmp_bind,   "icmp"  },
    {iirfb_bind,  "iirfb" }
};
static int nplug = sizeof(plug_ls) / sizeof(plug_id);

//===========================================================

int
plugin_load(PLUG *plugin, int pn, char verbose)
{
    int k;
    void (*bind)(PLUG *plugin) = NULL;

    for (k = 0; k < nplug; k++) {
        if (strcmp(plugin[pn].name, plug_ls[k].name) == 0) {
            bind = plug_ls[k].bind;
            if (bind) {
                bind(plugin + pn);
            }
            break;
        }
    }

    return (!bind);
}
