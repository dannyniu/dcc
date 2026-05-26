/* DannyNiu/NJF, 2026-04-12. Public Domain. */

#include "cpp-c.h"
#include "ppexpr-grammar.h"
#include <s2obj.h>

#define GRAMMAR_RULES ppexpr_grammar_rules
#define NS_RULES ns_rules_ppexpr
#define var_lex_elems CLexElems
#include "../lalr-common/lalr.h"

#define NextHeader "../cpp-c/validation.util-hello-world.bits.h"
#include "../lex-common/lexer-check-boilerplates.bits.h"
