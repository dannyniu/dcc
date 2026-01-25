/* DannyNiu/NJF, 2024-12-27. Public Domain */

#ifndef dcc_lex_h
#define dcc_lex_h 1

/// @file
/// Provides data structure definitions for lexical tokens and source code
/// character-based reading support.

#include "../common.h"
#include <s2obj.h>
#include <s2data.h>

#if defined(LEX_ENUM) || defined(LEX_ENUM_INIT)
#error Conflicting definitions for internally used macros LEX_ENUM{,_INIT}.
// 2025-01-16 note:
// LEX_ENUM_INIT is currently unused, though it may in the future.
#endif /* LEX_ENUM{,_INIT} */

// The end state of an FSM, which represents the type of the token.
// The initial versions of DCC used manually-constructed FSM table,
// although that's no longer the case owing to the use of an
// actual regex library, traditional identifier names are preserved.
typedef int lexer_state_t;
enum {
    lex_exceptional = -1,
    lex_token_complete = 0,
    lex_token_start = 1,
};

// Maps FSM states to legible strings.
struct lex_enum_strtab {
    lexer_state_t enumerant;
    const char *str;
};

#define S2_OBJ_TYPE_LEXTOKEN 0x2011
#define s2_is_token(obj) (((s2obj_t *)obj)->type == S2_OBJ_TYPE_LEXTOKEN)

/// @typedef
/// @details
/// This struct represents all information for a token.
/// It's a SafeTypes2 type.
typedef struct {
    s2obj_base;
    s2data_t *str;
    lexer_state_t completion;
    int32_t identity; // reserved as of 2025-08-09.
    int32_t classification; // reserved as of 2025-08-09.
    int32_t lineno, column; // both are 1-based.
} lex_token_t;

lex_token_t *lex_token_create();

// a 'base class' for retrieving individual characters (bytes actually)
// from a piece of source code, be it a file or a nul-terminated string,
// or something completely invented.
typedef struct lex_getc_base lex_getc_base_t;

// gets 1 character, or EOF.
typedef int (*lex_getc_func_t)(lex_getc_base_t *ctx);

// pushes 1 character back to the front of the stream.
typedef int (*lex_ungetc_func_t)(int c, lex_getc_base_t *ctx);

struct lex_getc_base {
    lex_getc_func_t getc;
    lex_ungetc_func_t ungetc;
};

// lex getc context over a string expression.
typedef struct {
    lex_getc_base_t base;
    const char *expr;
} lex_getc_str_t;

// lex getc context over a file handle.
typedef struct {
    lex_getc_base_t base;
    FILE *fp;
} lex_getc_fp_t;

// For use with file handles,
int fp_getc(lex_getc_fp_t *ctx);
int fp_ungetc(int c, lex_getc_fp_t *ctx);

// For use with string expressions.
int expr_getc(lex_getc_str_t *ctx);
int expr_ungetc(int c, lex_getc_str_t *ctx);

// initializes a lex getc context from a string or a file handle respectively.
lex_getc_base_t *lex_getc_init_from_str(lex_getc_str_t *ctx, const char *str);
lex_getc_base_t *lex_getc_init_from_fp(lex_getc_fp_t *ctx, FILE *fp);

// To be implemented externally.
typedef lex_token_t *(*token_shifter_t)(void *);

// Added 2026-01-24: pre-processor chain.
typedef struct {
    token_shifter_t pp_shifter;
    void *pp_context;
} PP_Chain;

#endif /* dcc_lex_h */
