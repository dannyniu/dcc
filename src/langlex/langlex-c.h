/* DannyNiu/NJF, 2024-12-26. Public Domain. */

#ifndef dcc_langlex_c_h
#define dcc_langlex_c_h 1

#include "../lex-common/rope.h"

#define LEX_ENUM(enumerant) enumerant,
enum {
    langlex_complete = lex_token_complete,
    langlex_start = lex_token_start,
#include "langlex-c-def.inc"
};
#undef LEX_ENUM

extern const struct lex_enum_strtab langlex_token_strtab[];
extern const char *langlex_puncts[];
extern lex_elem_t CLexElems[];
extern lex_elem_t CNumLexElems[];

#endif /* dcc_langlex_c_h */
