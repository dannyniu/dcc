/* DannyNiu/NJF, 2025-11-22. Public Domain. */

#ifndef dcc_langlex_cxing_h
#define dcc_langlex_cxing_h 1

#include "../lex-common/rope.h"

#define LEX_ENUM(enumerant) enumerant,
enum {
    langlex_complete = lex_token_complete,
    langlex_start = lex_token_start,
#include "langlex-cxing-def.inc"
};
#undef LEX_ENUM

extern const struct lex_enum_strtab langlex_token_strtab[];
extern lex_elem_t LexElems[];

// 2026-01-25:
// Languages may have a pre-processing stage, where the
// output of lexer is processed before entering the parser.

// strips comments.
typedef struct {
    RegexLexContext lexer;
} pp_strip_comments_ctx;

lex_token_t *PP_StripComments_Cxing(pp_strip_comments_ctx *ctx);

// concatenate adjacent string literals.
typedef struct {
    pp_strip_comments_ctx cmtstrip;
    lex_token_t *buffer;
} pp_strlit_concat_ctx, cxing_tokenizer;

lex_token_t *PP_StrLitConcat_Cxing(pp_strlit_concat_ctx *ctx);
#define CxingTokenizer_Shifter PP_StrLitConcat_Cxing

// Initializes Cxing pre-processor with a source code 'rope'.
// (rope is a string with metadata such as line and column number.)
void CxingTokenizerInit(cxing_tokenizer *shifter, source_rope_t *rope);

#endif /* dcc_langlex_cxing_h */
