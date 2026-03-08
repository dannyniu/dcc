/* DannyNiu/NJF, 2025-02-20. Public Domain. */

#ifndef dcc_cxing_interp_h
#define dcc_cxing_interp_h 1

#include "cxing-grammar.h"
#include "langsem.h"
#include <s2dict.h>

/// @fn
/// @details
/// 1. compile regices for lexer,
/// 2. creates string indicies table for grammar rules.
bool CXParserInitCommon();

/// @fn
/// @details
/// Undoes `CXParserInitCommon()`.
void CXParserFinalCommon();

#define S2_OBJ_TYPE_CXING_MODULE 0x2111
#define s2_is_cxing_module(obj)                                 \
    (((s2obj_t *)obj)->type == S2_OBJ_TYPE_CXING_MODULE)

/// @class
/// @details
/// Corresponds to what's known as 'Translation Unit' in the CXING spec.
/// Keeps track of entity definitions and declarations, as well as
/// injected definitions.
typedef struct cxing_module {
    s2obj_base;

    // The original filename.
    char *filename;

    // Files that had been `_Include`'d once.
    s2dict_t *oncehad;

    // The ones defined in the source code.
    s2dict_t *entities;

    // The ones exposed from foreign environment.
    s2dict_t *linked;

    size_t func_defs_cnt;
    uint8_t *CallStubs;
    s2data_t **SymTab;

    // introduced 2026-02-06.
    long error_count;

    // ... TODO (2025-12-13, 2026-01-25).
} cxing_module_t;

#define S2_OBJ_TYPE_SYM_INFO 0x2113
#define s2_is_cxing_syminfo(obj)                        \
    (((s2obj_t *)obj)->type == S2_OBJ_TYPE_SYM_INFO)

/// @typedef
/// @brief
/// Metadata attached to entity declarations.
typedef struct cxing_syminfo {
    s2obj_base;

    enum { // bitmasks actually.
        module_syminfo_extern = 1,
    } flags;
} cxing_syminfo_t;

/// @fn
/// @param CxingModulePath
/// @details
/// Opens a CXING source code file, then
/// 1. parse to produce an AST tree,
/// 2. allocate spaces for a module,
/// 3. process the AST tree as a 'translation unit':
///    - process inclusions,
///    - load dynamic libraries (a.k.a. shared objects)
///      (TODO 2026-03-01: decide how to limit scope).
///    - extract constant definitions,
///    - extract function definitions,
///    - and export 'extern' functions as entry points.
/// 4. Create call stubs for functions so that
///    they appear as 1st-class citizens and
///    can be invoked from pointers
cxing_module_t *CXOpen(const char *restrict CxingModulePath);

/// @fn
/// @param module
/// @param msg
/// @brief
/// Used by the parser to report syntax and semantic errors.
void CxingSyntaxErr(cxing_module_t *restrict module, const char *msg, ...);

/// @fn
/// @param module
/// @param msg
/// @brief
/// Used by the interpreter to report syntax and semantic errors.
void CxingSemanticErr(cxing_module_t *restrict module, const char *msg, ...);

/// @fn
/// @param node_body
/// @details
/// A reusable function for parsing constants for use
/// in module construction and executing function codes.
struct value_nativeobj CXConstDefParse(lalr_prod_t *node_body);

/// @fn
/// @param module
/// @param dumper
/// @details
/// Checks the semantic correctness of the module.
/// If `dumper` is non-NULL, then check is not done,
/// and module definition is dumped to the file
/// specified by `dumper`.
bool CxingModuleInspectDefinitions(
    cxing_module_t *restrict module, FILE *dumper);

/// @fn
/// @param module
/// @brief
/// Dumps all of the module and any injected definitions.
void CxingModuleDump(cxing_module_t *restrict module);

/// @fn
/// @param module
/// @param name
/// @param referent
/// @details
/// Exposes (injects) `referent` as the definition of an entity
/// under `name` into `module`.
bool CXExpose(
    cxing_module_t *restrict module,
    const char *restrict name,
    struct value_nativeobj referent);

// 2026-02-06 TODO:
// Add a function that does a dry run after establishing module.

/// @fn
/// @param module where the definition resides.
/// @param sym name of the definition to retrieve.
/// @details
/// Retrieves the definition for a function (i.e. subroutine or method)
/// and return a pointer that may be used as a C function pointer
/// for calling the function. The call must be made according to the
/// calling convention of CXING as described in "runtime semantic".
///
/// @note
/// This form of functions requires creating assembly-written
/// function stubs to invoke `CxingExecuteFunction` (which is
/// now a macro expanding to `CxingFuncEval`) over the
/// parse-compiled function body. These stubs exist in form of
/// `cxing_callxfer_bridge` which are objects of type
/// `cxing_platform_abi_bridge_t`, and as the name suggests,
/// these call-transfer bridges are platform-specific.
void *CXSym(cxing_module_t *restrict module, const char *restrict sym);

typedef enum {
    // multi-fold purpose semantic verification.
    // 1. verify that all referenced identifiers are defined or declared.
    // 2. verify that assignments receive lvalues as left-hand side operands.
    cxing_func_eval_mode_dryrun = 0,

    // actual execution.
    cxing_func_eval_mode_execute = 1,
} cxing_func_eval_mode_t;

struct value_nativeobj CxingFuncEval(
    cxing_module_t *restrict module,
    lalr_prod_t *restrict textsegment,
    lalr_prod_t *restrict params,
    int argn, struct value_nativeobj args[],
    cxing_func_eval_mode_t evalmode);

// 2025-12-30: subroutines first, methods next.
/*- struct value_nativeobj CxingExecuteFunction(
    cxing_module_t *restrict module,
    lalr_prod_t *restrict textsegment,
    lalr_prod_t *restrict params,
    int argn, struct value_nativeobj args[]); -*/
#define CxingExecuteFunction(module, textsegment, params, argn, args) \
    CxingFuncEval(module, textsegment, params, argn, args, \
                  cxing_func_eval_mode_execute)

/// @class
/// @brief data protocol definition for platform-specific call-transfer bridge.
typedef struct {
    // A short stub to be called which jumps to the
    // callxfer implementation that executes the function.
    uint8_t *callstub;

    // The size (in bytes) of the stub.
    size_t sz_callstub;

    // The field (at the beginning) for relocation data.
    // (after which is the CPU instructions).
    size_t sz_relocdat;

    // Link-time relocation.
    void (*relocator)(
        uint8_t *stubptr,
        cxing_module_t *module);

    // Allocates a piece of writable memory,
    void *(*prepare_memory_for_stubs)(size_t cnt, size_t sz);

    // Then makes it executable.
    bool (*make_stub_memory_executable)(void *p, size_t cnt, size_t sz);

    // Frees that piece of memory.
    void (*free_callstub_memory)(void *p, size_t cnt, size_t sz);
} cxing_platform_abi_bridge_t;

/// @var
/// @brief object instance of call-transfer bridge defined by the platform.
extern cxing_platform_abi_bridge_t cxing_callxfer_bridge;

#endif /* dcc_cxing_interp_h */
