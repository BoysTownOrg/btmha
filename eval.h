// eval.h - header for eval 

#ifndef EVAL_H
#define EVAL_H

char *skipwhite(char *);
char *skipalphnum(char *);
int eval_expr(char *, double *, int *, int *, char *, int);
int get_expr_err(char *, int);
int get_strings(char *, char *, int *, int *, int);
int usr_var_set(char *, char *);

#endif // EVAL_H
