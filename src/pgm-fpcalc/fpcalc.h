/* DannyNiu/NJF, 2025-02-20. Public Domain. */

#ifndef dcc_fpcalc_h
#define dcc_fpcalc_h 1

#include "fpcalc-grammar.h"
#include <s2dict.h>

extern s2dict_t *globaldefs;
int remember_definition(lalr_prod_t *expr, int semantic);

bool fpcalc_eval_start(
    double *ret,
    lalr_prod_t *expr,
    lalr_prod_t *params,
    lalr_prod_t *args);

#endif /* dcc_fpcalc_h */
