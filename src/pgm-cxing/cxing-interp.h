/* DannyNiu/NJF, 2025-02-20. Public Domain. */

#ifndef dcc_cxing_interp_h
#define dcc_cxing_interp_h 1

#include "cxing-grammar.h"
#include "langsem.h"
#include <s2dict.h>

bool CXParserInit();
void CXParserFinal();

#define S2_OBJ_TYPE_CXING_MODULE 0x2111
#define s2_is_cxing_module(obj)                                 \
    (((s2obj_t *)obj)->type == S2_OBJ_TYPE_CXING_MODULE)

typedef struct cxing_module {
    s2obj_base;

    // The ones defined in the source code.
    s2dict_t *entities;

    // The ones exposed from foreign environment.
    s2dict_t *linked;

    size_t func_defs_cnt;
    uint8_t *CallStubs;
    s2data_t **SymTab;
    
    // ... TODO (2025-12-13).
} cxing_module_t;

#define S2_OBJ_TYPE_SYM_INFO 0x2112
#define s2_is_cxing_syminfo(obj)                        \
    (((s2obj_t *)obj)->type == S2_OBJ_TYPE_SYM_INFO)

typedef struct cxing_syminfo {
    s2obj_base;

    enum { // bitmasks actually.
        module_syminfo_extern = 1,
    } flags;
} cxing_syminfo_t;

cxing_module_t *CXOpen(const char *restrict CxingModulePath);

// primarily for debugging.
void CxingModuleDump(cxing_module_t *restrict module);

// 2026-01-02:
// This would require creating assembly-written function stub to invoke
// `CxingExecuteFunction` over the parse-compiled function body. 
void *CXSym(cxing_module_t *restrict module, const char *restrict sym);

bool CXExpose(
    cxing_module_t *restrict module,
    const char *restrict name,
    struct value_nativeobj linkee);

// 2025-12-30: subroutines first, methods next.
struct value_nativeobj CxingExecuteFunction(
    cxing_module_t *restrict module,
    lalr_prod_t *restrict textsegment,
    lalr_prod_t *restrict params,
    int argn, struct value_nativeobj args[]);

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

extern cxing_platform_abi_bridge_t cxing_callxfer_bridge;

#endif /* dcc_cxing_interp_h */
