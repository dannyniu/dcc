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

    // Conditional inclusion state keeping.
    // Maximum number of nesting of conditional inclusion
    // as required by the standard is 63; maximum number
    // of nested `#include`s is 15. If implemented using
    // fixed-size array / stack for keeping the `state`,
    // should cover majority of use-cases.
    uint16_t condinc_level;

#define CONDINC_INITIAL 0 // do not participate in cond-inc state transition,
#define CONDINC_TRYNEXT 1 // control expression evaluated to false.
#define CONDINC_INCLUDED 2 // includes this line group, and not the rest.
#define CONDINC_SUPPRESS 3 // this can happen for nested cond-inc.
    // - A TU starts with {Initial}:
    //   - On `#if true`, transition to {Included},
    //   - On `#if false`, transition to {TryNext},
    // - At {TryNext}:
    //   - On `#elif true`, transition to {Included},
    // - At {Included}:
    //   - On `#endif`, transition to {Initial}.
    // - On transition from {Initial}, increment {Level}.
    // - On transition to {Initial}, decrement {Level}.
    // - Invariant: a line group at any {Level} is always entered with {Initial}.
    uint8_t condinc_state[1022];
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

// translation phase 2.
bool look_ahead_for_genuine_newline(RegexLexContext *ctx);

// parses one, and saves it into `ctx_tu` using `cppDefine1Macro`.
int cppProcessDefineDirective(
    cpptu_t *restrict ctx_tu,
    void *restrict ctx_shifter,
    token_shifter_t shifter);

// Evaluates the control expression on the `if` and `elif` line.
// Although operands are evaluated in `(u?)intmax_t`,
// the result is returned in true, false,
// or -1, the last of which indicates failure.
int cppEvaluateCtrlExpr(
    cpptu_t *restrict ctx_tu,
    void *restrict ctx_shifter,
    token_shifter_t shifter);

// conforms to `token_shifter_t`.
lex_token_t *cppMainProgramCoroutine(cpptu_t *ctx_tu);

#define eprintf(...) // fprintf(stderr, __VA_ARGS__)
#define rprintf(file, line, ...) eprintf(file":" #line " " __VA_ARGS__)
#define rprintf1(file, line, ...) rprintf(file, line, __VA_ARGS__)
#define Reached(...) rprintf1(__FILE__, __LINE__, __VA_ARGS__);
