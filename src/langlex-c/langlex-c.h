/* DannyNiu/NJF, 2024-12-26. Public Domain. */

#ifndef dcc_langlex_c_h
#define dcc_langlex_c_h 1

#include "../lex-common/rope.h"

// This is the language lexer that comes after the pre-processing stage.

#define LEX_ENUM(enumerant) enumerant,
enum {
    langlex_complete = lex_token_complete,
    langlex_start = lex_token_start,
#include "langlex-c-def.inc"
};
#undef LEX_ENUM

extern const struct lex_enum_strtab langlex_token_strtab[];

extern const struct lex_fsm_trans langlex_fsm[];
extern const char *langlex_puncts[];

// 2025-04-30:
// Initially, there was only langlex and pplex, and there's no distinction
// between general lexer implementation and lexing FSM for specific languages.
// Now the general lexer is separated to its own directory, and file names for
// source codes for for langlex (and pplex if applicable) shall be suffixed
// by the name of the language (before the filename extension).
// The identifier names for lexing rules' data structure can be
// simply named 'langlex' or 'pplex' for brevity.

extern lex_elem_t CLexElems[];

#endif /* dcc_langlex_c_h */
