/* DannyNiu/NJF, 2024-12-30. Public Domain. */

#ifndef dcc_fpcalc_grammar_h
#define dcc_fpcalc_grammar_h 1

#include "../lalr-common/lalr.h"

extern strvec_t *ns_rules_fpcalc;

#define hRule(s)   strvec_str2i(ns_rules_fpcalc, s)

void *fpcalc_goal(lalr_rule_params);

void *assignexpr_degenerate(lalr_rule_params);
void *assignexpr_assignment(lalr_rule_params);

void *addexpr_degenerate(lalr_rule_params);
void *addexpr_addition(lalr_rule_params);
void *addexpr_subtraction(lalr_rule_params);

void *mulexpr_degenerate(lalr_rule_params);
void *mulexpr_multiplication(lalr_rule_params);
void *mulexpr_division(lalr_rule_params);

void *unaryexpr_degenerate(lalr_rule_params);
void *unaryexpr_positive(lalr_rule_params);
void *unaryexpr_negative(lalr_rule_params);

void *primary_identexpr(lalr_rule_params);
void *primary_number(lalr_rule_params);
void *primary_paren(lalr_rule_params);

void *addexprlist_base(lalr_rule_params);
void *addexprlist_recursion(lalr_rule_params);
void *identexpr_label(lalr_rule_params);
void *identexpr_function_noparam(lalr_rule_params);
void *identexpr_function_1param(lalr_rule_params);
void *identexpr_function_multiparam(lalr_rule_params);

extern lalr_rule_t fpcalc_grammar_rules[];

#endif /* dcc_fpcalc_grammar_h */
