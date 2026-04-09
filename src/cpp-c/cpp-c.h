/* DannyNiu/NJF, 2026-03-21. Public Domain. */

#include "macro.h"

/// @brief
/// A translation unit at its pre-processing stage.
#define S2_OBJ_TYPE_CPPTU 0x2042
typedef struct cpptu cpptu_t;

struct cpptu {
    s2obj_base;
    s2list_t *macros;
};

// declaration to be moved elsewhere (2026-03-21).
void ccDiagnoseError(void *restrict ctx_tu, const char *fmt, ...);
void ccDiagnoseWarning(void *restrict ctx_tu, const char *fmt, ...);

// basically just a 'pair', and is a type external to SafeTypes2.
typedef struct {
    // name of the macro, with all contextual information.
    lex_token_t *mname;

    // a NULL entry `#undef`'s a macro.
    cppmacro_t *macdef;
} cpptu_macdef_t;

void cpptu_macdef_free(cpptu_macdef_t *x);

// declaration might or might not be moved elsewhere (2026-03-27).
// 0 on success, -1 on error.
int cppDefine1Macro(
    cpptu_t *restrict ctx_tu,
    lex_token_t *restrict macro_name, // owned by `ctx_tu` upon return.
    cppmacro_t *restrict macrodef); // owned by `ctx_tu` upon return.

int cppUndef1Macro(
    cpptu_t *restrict ctx_tu,
    lex_token_t *restrict macro_name); // owned by `ctx_tu` upon return.

cppmacro_t *cppLookup1Macro(
    cpptu_t *restrict ctx_tu,
    lex_token_t *restrict macro_name);
