/* DannyNiu/NJF, 2025-02-20. Public Domain. */

#ifndef dcc_cxing_interp_h
#define dcc_cxing_interp_h 1

#include "cxing-grammar.h"
#include <s2dict.h>

#define S2_OBJ_TYPE_CXING_MODULE 0x2111
#define s2_is_cxing_module (((s2obj_t *)obj)->type == S2_OBJ_TYPE_CXING_MODULE)

typedef struct cxing_module {
    s2obj_base;
    // ... TODO (2025-12-13).
} cxing_module_t;

cxing_module_t *CXOpen(const char *restrict CxingModulePath);
void *CXSym(cxing_module_t *module, const char *restrict sym);

#endif /* dcc_cxing_interp_h */
