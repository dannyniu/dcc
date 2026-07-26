/* DannyNiu/NJF, 2026-03-21. Public Domain. */

// 2026-04-12:
// this is a work-in-progress header that may likely
// have some of its declarations moved elsewhere.
//
// 2026-07-05:
// TODOs:
// - include guards.

/// @brief
/// A translation unit at its pre-processing stage.
#define S2_OBJ_TYPE_CPPTU 0x2042
typedef struct cpptu cpptu_t;

#include "../c-misc/diagnose.h"
#include "macro.h"

// Initialize global resources used by the C pre-processor.
bool ccPreprocInit();

// Finalize the global resources used by the C pre-processor.
void ccPreprocFin();

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
    lex_getc_fp_t getcx;
    source_rope_t *rope;
    RegexLexContext ctx_shifter;
    token_shifter_t shifter;

    // output buffer to the parser.
    s2list_t *pushlist; // of lexical tokens.

    // looks out for `#` at the beginning of lines.
    struct cppBufferedShifter lash; // look-ahead shifter.
    struct cppMacroExpandShifter rescan_stackbase;

    // Conditional inclusion state keeping.
    // Maximum number of nesting of conditional inclusion
    // as required by the standard is 63. If implemented using
    // fixed-size array / stack for keeping the `state`,
    // should cover majority of use-cases.
    uint8_t condinc_level;

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
    uint8_t condinc_state[63];

    // The name of the file being processed by the current working context.
    // Substitutes `__FILE__` in special expansions.
    s2data_t *HotFile;

    // Include file search directories.
    // 2026-07-05:
    // think about how to support different kind of include directories,
    // such as `-I`, `-iquote` (if plan commits), `-isystem` (desired),
    // preferably with a set of flag bits in the `ctxinfo` field.
    s2list_t *IncPaths;

    // Added 2026-07-19.
    cpptu_t *Includee;

    // Added 2026-07-24.
    // This is the (partial) parse tree used for the lexer hack.
    s2obj_t *misc;

    long count_errors, count_warnings;
};

// Creates a `cpptu_t` object from `sourcefile`.
// Macro definitions (and `#undef`s) are shared with `parent`.
cpptu_t *cpptu_create(char *sourcefile, cpptu_t *parent);

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

// translation phase 3, but specifically for control lines.
void CtrlLine_ArgCollect(
    s2list_t *atoks,
    cpptu_t *restrict ctx_tu,
    void *restrict ctx_shifter,
    token_shifter_t shifter);

// conforms to `token_shifter_t`.
lex_token_t *shift_from_s2list(s2list_t *toklist);

// parses one, and saves it into `ctx_tu` using `cppDefine1Macro`.
int cppProcessDefineDirective(
    cpptu_t *restrict ctx_tu,
    void *restrict ctx_shifter,
    token_shifter_t shifter);

// Includes a source code file.
int cppProcessIncludeDirective(
    cpptu_t *restrict ctx_tu,
    void *restrict ctx_shifter,
    token_shifter_t shifter);

// A helper function.
char *ResolveImportationFilename(
    s2list_t *atoks, s2list_t *incpaths,
    struct cppMacroExpandShifter *ctx);

// Evaluates the control expression on the `if` and `elif` line.
// Although operands are evaluated in `(u?)intmax_t`,
// the result is returned in true, false,
// or -1, the last of which indicates failure.
int cppEvaluateCtrlExpr(
    cpptu_t *restrict ctx_tu,
    void *restrict ctx_shifter,
    token_shifter_t shifter);

// conforms to `token_shifter_t`.
lex_token_t *cppDirectivesDispatch(cpptu_t *ctx_tu);
lex_token_t *cppMainProgramCoroutine(cpptu_t *ctx_tu);

#define eprintf(...) // fprintf(stderr, __VA_ARGS__)
#define rprintf(file, line, ...) eprintf(file":" #line " " __VA_ARGS__)
#define rprintf1(file, line, ...) rprintf(file, line, __VA_ARGS__)
#define Reached(...) rprintf1(__FILE__, __LINE__, __VA_ARGS__);
