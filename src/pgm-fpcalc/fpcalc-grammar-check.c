/* DannyNiu/NJF, 2025-01-03. Public Domain. */

#include "../langlex/langlex-c.h"
#include "fpcalc-grammar.h"
#define GRAMMAR_RULES fpcalc_grammar_rules
#define NS_RULES ns_rules_fpcalc
#define var_lex_elems CLexElems

#include "../lalr-common/grammar-check.c"
