/* DannyNiu/NJF, 2025-08-09. Public Domain. */

#ifndef dcc_rope_h
#define dcc_rope_h 1

#include "lex.h"
#include <s2containers.h>

#define S2_OBJ_TYPE_SRCROPE 0x2012
#define s2_is_srcrope(obj) (((s2obj_t *)obj)->type == S2_OBJ_TYPE_SRCROPE)

typedef struct {
    s2obj_base;
    s2data_t *sourcecode;
    s2data_t *linedelims; // array of `ptrdiff_t`s.
} source_rope_t;

// Blackslack characters at the end of line continues the line.
#define RopeCreatFlag_LineConti (1 << 0)

source_rope_t *CreateRopeFromGetc(lex_getc_base_t *getcctx, int flags);
source_rope_t *CreateRopeFromLineData(s2data_t *src, int semantic);

#include <librematch.h>

typedef struct {
    libregexp_t preg;
    const char *pattern;
    int cflags, eflags;
    lexer_state_t completion;
} lex_elem_t;

typedef struct RegexLexContext RegexLexContext;
struct RegexLexContext {
    struct logging_ctxbase logger_base;

    source_rope_t *rope; // source code input.
    lex_elem_t *regices; // set of known lexical elements.
    const char **keywords;
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
