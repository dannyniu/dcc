/* DannyNiu/NJF, 2024-12-26. Public Domain. */

#ifndef dcc_langlex_h
#define dcc_langlex_h 1

#include "lex.h"

// This is the language lexer that comes after the pre-processing stage.

#define LEX_ENUM(enumerant) enumerant,
enum {
    langlex_complete = lex_token_complete,
    langlex_start = lex_token_start,
#include "langlex-def.inc"
};
#undef LEX_ENUM

extern const struct lex_enum_strtab langlex_token_strtab[];

extern const struct lex_fsm_trans langlex_fsm[];
extern const char *langlex_puncts[];

#endif /* dcc_langlex_h */
