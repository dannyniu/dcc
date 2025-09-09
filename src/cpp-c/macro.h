/* DannyNiu/NJF, 2025-05-18. Public Domain. */

#ifndef dcc_macro_h
#define dcc_macro_h 1

#include "../lex-common/lex.h"

#define S2_OBJ_TYPE_PPTOKEN 0x2041

typedef struct pptoken pptoken_t;

struct pptoken {
    s2obj_base;
    lex_token_t *super;

    // New tokens entering the macro expansion process have this
    // depth start with 0. For each macro expansion, the last token
    // of the replaced sequence have this value increased by 1.
    // This implements the limiting mechanism in the rescanning
    // process that prevents already expanded macros from being
    // expanded recursively.
    int32_t rescan_depth;

    // If non-NULL, it's a (retained, not kept) reference to the token
    // which led to the expansion of this token.
    pptoken_t *abbrev;

    // If non-NULL, it's the reference to the argument token in an
    // invocation of function-like macro that led to the expansion
    // of this token.
    pptoken_t *abbrev_arg;
};

#define S2_OBJ_TYPE_OBJMACRO 0x2042

typedef struct objmacro objmacro_t;

struct objmacro {
    s2obj_base;
    s2list_t *tokseq;
};

#define S2_OBJ_TYPE_FUNCMACRO 0x2043

typedef struct funcmacro funcmacro_t;

struct funcmacro {
    s2obj_base;
    s2list_t *tokseq;
    s2list_t *paramseq;
    int is_variadic;
};

typedef struct {
    s2dict_t *macros;
    s2list_t *norescan;
} MacroReplacementCtx_t;

#endif /* dcc_macro_h */
