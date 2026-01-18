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

#endif /* dcc_langlex_cxing_h */
