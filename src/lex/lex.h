/* DannyNiu/NJF, 2024-12-27. Public Domain */

#ifndef dcc_lex_h
#define dcc_lex_h 1

#include "../common.h"
#include <s2obj.h>
#include <s2data.h>

#if defined(LEX_ENUM) || defined(LEX_ENUM_INIT)
#error Conflicting definitions for internally used macros LEX_ENUM{,_INIT}.
// 2025-01-16 note:
// LEX_ENUM_INIT is currently unused, though it may in the future.
#endif /* LEX_ENUM{,_INIT} */

typedef int lexer_state_t;
enum {
    lex_exceptional = -1,
    lex_token_complete = 0,
    lex_token_start = 1,
};

struct lex_fsm_trans {
    char *expect;
    lexer_state_t now;
    lexer_state_t next;
    enum {
        lex_expect_set = 0, // accept a set of chars.
        lex_expect_compl, // accept the complement of a set of chars.
    } flags;
};

struct lex_enum_strtab {
    lexer_state_t enumerant;
    const char *str;
};

#define S2_OBJ_TYPE_LEXTOKEN 0x2011
#define s2_is_token(obj) (((s2obj_t *)obj)->type == S2_OBJ_TYPE_LEXTOKEN)

typedef struct {
    s2obj_t base;
    s2data_t *str;
    lexer_state_t completion;
    int32_t identity;
    int32_t classification;
    int32_t lineno, column; // both are 1-based.
} lex_token_t;

lex_token_t *lex_token_create();

// Lexing is deterministic as does regular languages,
// as such, there should be no ambiguity as to
// what the next state is. Therefore we can be confident
// that the current state is not any other's prefix.

typedef struct lex_getc_base lex_getc_base_t;
typedef int (*lex_getc_func_t)(lex_getc_base_t *ctx);
typedef int (*lex_ungetc_func_t)(int c, lex_getc_base_t *ctx);

struct lex_getc_base {
    lex_getc_func_t getc;
    lex_ungetc_func_t ungetc;
};

// scan based on a DFA specified in the `fsm` parameter.
lex_token_t *lex_token_parse(
    const struct lex_fsm_trans *fsm,
    lex_getc_base_t *fc);

// scan from a pre-defined set of tokens specified in `token_set`.
lex_token_t *lex_token_match(
    const char *token_set[],
    lexer_state_t fallback_type,
    lex_getc_base_t *fc);

#endif /* dcc_lex_h */
