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

/// @fn
/// @param tok the token to 'graduate'.
///
/// @brief
/// 'Graduates' a pre-processing token into a language token.
/// The completion of certain tokens - specifically pp-numbers,
/// would change at this stage.
///
/// @returns
/// 0 on success, and -1 if the spelling of the token has become invalid.
int PPTokGraduate(lex_token_t *tok);

#endif /* dcc_langlex_c_h */
