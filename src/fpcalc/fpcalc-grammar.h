/* DannyNiu/NJF, 2024-12-30. Public Domain. */

#ifndef dcc_fpcalc_grammar_h
#define dcc_fpcalc_grammar_h 1

#include "../lalr/lalr.h"

extern strvec_t *ns_rules_fpcalc;

#define hRule(s)   strvec_str2i(ns_rules_fpcalc, s)

extern lalr_rule_t fpcalc_grammar_rules[];

#endif /* dcc_fpcalc_grammar_h */
