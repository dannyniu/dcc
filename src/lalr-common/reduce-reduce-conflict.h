/* DannyNiu/NJF, 2025-03-29. Public Domain. */

#ifndef dcc_lalr_test_grammar_h
#define dcc_lalr_test_grammar_h 1

#include "lalr.h"

extern strvec_t *ns_rules_test;

#define hRule(s)   strvec_str2i(ns_rules_test, s)

extern lalr_rule_t test_grammar_rules[];

#endif /* dcc_lalr_test_grammar_h */
