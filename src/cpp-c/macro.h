/* DannyNiu/NJF, 2025-05-18. Public Domain. */

#ifndef dcc_macro_h
#define dcc_macro_h 1

#include "../langlex/langlex-c.h"
#include <s2list.h>
#include <s2dict.h>

/// @brief
/// Represents a C pre-processing macro.
#define S2_OBJ_TYPE_CPPMACRO 0x2041
typedef struct cppmacro cppmacro_t;

struct cppmacro {
    s2obj_base;
    s2list_t *repllist;
    s2list_t *params; // non-NULL if function-like, otherwise object-like.
    int is_variadic;
};

/// @brief
/// Classification flags for pre-processing tokens.
#define PPTOK_CLS_ORDINARY 0 // not a param,
#define PPTOK_CLS_PARAMETER 1 // param, no eval unless `operand` flag is clear,
#define PPTOK_CLS_STRINGIFY 2 // applies to the following param.
#define PPTOK_CLS_CATENATE 4 // set on the following param, affects 2 tokens.
#define PPTOK_CLS_OPERAND 8 // unevaluated parameter.

struct cppMacroExpandShifter {
    // Always positioned at the tail.
    s2list_t *pushlist;

    // Always positioned at the head.
    s2list_t *hotlist;

    // a name of macro to be excluded from expansion.
    // only when coldlist == upstack (i.e. in a pass-down context).
    lex_token_t *hotname;

    // coldlist == upstack if in a pass-down context.
    // coldlist is external and upstack is nullptr if the top-most context.
    void *coldlist;
    token_shifter_t coldlist_shifter;
    struct cppMacroExpandShifter *upstack;

    // context pointer to the translation unit.
    struct cpptu *ctx_tu;
};

// conforms to `token_shifter_t`.
lex_token_t *cppTokenJet(struct cppMacroExpandShifter *ctx);

#endif /* dcc_macro_h */
