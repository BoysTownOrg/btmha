// var_list.h - header for VAR lists

#ifndef VAR_LIST_H
#define VAR_LIST_H

#include <sigpro.h>

#if !defined(_MSC_VER) || (_MSC_VER > 1500)
#include <stdint.h>
#else
typedef short int16_t;
typedef long int32_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;
#endif

#ifndef WIN32
#define _strdup         strdup
#endif

//===========================================================

char *var_list_str(VAR *, char *);
double var_list_f8n(VAR *, char *);
int trim_line(char *);
int var_list_assign(VAR *, char *, int, int);
int var_list_index(VAR *, char *);
int var_list_intersect(VAR *, VAR *, VAR *, char *);
int var_list_length(VAR *);
int var_list_i4n(VAR *, char *);
int var_list_get(VAR *, char *, void *, int, int);
int var_list_set(VAR *, char *, void *, int, int);
void var_list_add(VAR *, char *, void *, int, int, int *);
void var_list_answer(VAR *, int, char *);
void var_list_cleanup(VAR *);
void var_list_clear(VAR *);
void var_list_delete(VAR *, char *);
void var_list_eval(VAR *, char *);
void var_list_replace(VAR *, VAR *);
void var_list_show(VAR *, char *);
void var_list_show_one(VAR *, int);

//===========================================================

#endif // VAR_LIST_H
