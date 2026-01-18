/* DannyNiu/NJF, 2025-08-09. Public Domain. */

#ifndef dcc_rope_h
#define dcc_rope_h 1

/// @file
/// Combined token shifter implementation for use with parser.

#include "lex.h"
#include <s2containers.h>

#define S2_OBJ_TYPE_SRCROPE 0x2012
#define s2_is_srcrope(obj) (((s2obj_t *)obj)->type == S2_OBJ_TYPE_SRCROPE)

/// @typedef
/// So called 'rope' is a data structure that many text editors use to
/// represent the content of open files, enabling quick line-based indexing.
/// This is a SafeTypes2 type.
typedef struct {
    s2obj_base;
    s2data_t *sourcecode; // the source code loaded in entirity.
    s2data_t *linedelims; // array of `ptrdiff_t`s.
} source_rope_t;

// Blackslack characters at the end of line continues the line.
#define RopeCreatFlag_LineConti (1 << 0)

/// @fn
/// @brief
/// create a rope from any (even invented) source using the 'getc' abstraction.
source_rope_t *CreateRopeFromGetc(lex_getc_base_t *getcctx, int flags);

/// @fn
/// @brief
/// if it's already an `s2data_t` object, then use it directly.
/// the `semantic` parameter should be one of the s2_setter_* values. 
source_rope_t *CreateRopeFromLineData(s2data_t *src, int semantic);

#include <librematch.h>

/// @typedef
/// a lexical _element_ is defined by language specificaitons
/// to recognize lexical _tokens_ - this type represents the
/// implementation of a lexical element that uses regular expressions
/// to recognize the lexical token.
typedef struct {
    libregexp_t preg;
    const char *pattern;
    int cflags, eflags;
    lexer_state_t completion;
} lex_elem_t;

/// @typedef
/// This is a working context that combines a set of lexical elements,
/// with a source code represented as a rope, that forms an instance
/// of a 'token shifter' to be used by a ('the' actually) parser.
typedef struct RegexLexContext RegexLexContext;
struct RegexLexContext {
    struct logging_ctxbase logger_base;

    source_rope_t *rope; // source code input.
    lex_elem_t *regices; // set of known lexical elements.
    const char **keywords; // 2026-01-09: found to be unused.
    void *misc; // For the lexer hack to be used with C.

    // current position in the rope-represented source code text. zero-based.
    ptrdiff_t offsub;

    // total number of bytes in source code text.
    size_t srcchars;

    // 'lineind' differentiates from 'lineno' in that it's zero-based.
    size_t lineind;

    // total number of recorded lines.
    size_t linecnt;
};

// 2025-08-17:
// Because `RegexLexContext` is not a SafeType2 type,
// it doesn't make sense to discuss the reference
// count of objects on it. As such, `rope` is not retained.
void RegexLexFromRope_Init(RegexLexContext *ctx, source_rope_t *rope);

lex_token_t *RegexLexFromRope_Shift(RegexLexContext *ctx);

#endif /* dcc_rope_h */
