/* DannyNiu/NJF, 2025-11-29. Public Domain. */

#include "../langlex/langlex-cxing.h"
#define LexerDecl cxing_tokenizer tokenizer;
#define LexerInit CxingTokenizerInit(\
        &tokenizer, rope)
#define lexer tokenizer.cmtstrip.lexer
#define RegexLexFromRope_Shift CxingTokenizer_Shifter

#include "cxing-grammar.h"
#define GRAMMAR_RULES cxing_grammar_rules
#define NS_RULES ns_rules_cxing
#define var_lex_elems LexElems

#include "../lalr-common/grammar-check.c"
