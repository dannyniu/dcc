/* DannyNiu/NJF, 2026-03-21. Public Domain. */

// 2026-04-12:
// this is a work-in-progress header that may likely
// have some of its declarations moved elsewhere.

/// @brief
/// A translation unit at its pre-processing stage.
#define S2_OBJ_TYPE_CPPTU 0x2042
typedef struct cpptu cpptu_t;

#include "macro.h"

// declaration to be moved elsewhere (2026-03-21 Maybe TODO).
void ccDiagnoseError(cpptu_t *restrict ctx_tu, const char *fmt, ...);
void ccDiagnoseWarning(cpptu_t *restrict ctx_tu, const char *fmt, ...);

struct cppBufferedShifter {
    lex_token_t *sv;
    void *ctx_shifter;
    token_shifter_t shifter;
};

lex_token_t *cppBufferedShifterCoroutine(struct cppBufferedShifter *ctx);

struct cpptu {
    s2obj_base;
    s2list_t *macros;

    // input interface to the source code file.
    RegexLexContext *ctx_shifter;
    token_shifter_t shifter;

    // output buffer to the parser.
    s2list_t *pushlist; // of lexical tokens.

    struct cppBufferedShifter lash; // look-ahead shifter.
    struct cppMacroExpandShifter rescan_stackbase;
};

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

// parses one, and saves it into `ctx_tu` using `cppDefine1Macro`.
int cppProcessDefineDirective(
    cpptu_t *restrict ctx_tu,
    void *restrict ctx_shifter,
    token_shifter_t shifter);

// conforms to `token_shifter_t`.
lex_token_t *cppMainProgramCoroutine(cpptu_t *ctx_tu);

#define eprintf(...) // fprintf(stderr, __VA_ARGS__)
#define rprintf(file, line, ...) eprintf(file":" #line " " __VA_ARGS__)
#define rprintf1(file, line, ...) rprintf(file, line, __VA_ARGS__)
#define Reached(...) rprintf1(__FILE__, __LINE__, __VA_ARGS__);
