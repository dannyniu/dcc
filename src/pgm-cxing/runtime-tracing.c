/* DannyNiu/NJF, 2026-03-14. Public Domain. */

#include <stdarg.h>
#include "runtime.h"

void CxingDebug(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    fprintf(stderr, "[CxingDebug]: ");
    vfprintf(stderr, msg, ap);
    va_end(ap);
}

void CxingDiagnose(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    fprintf(stderr, "[CxingDiagnose]: ");
    vfprintf(stderr, msg, ap);
    va_end(ap);
    exit(1); // TODO (2026-01-01): try to fail more gracefully.
}

void CxingWarning(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    fprintf(stderr, "[CxingWarning]: ");
    vfprintf(stderr, msg, ap);
    va_end(ap);
}

void CxingFatal(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    fprintf(stderr, "[CxingFatal]: ");
    vfprintf(stderr, msg, ap);
    va_end(ap);
    abort(); // TODO (2025-12-31): I might be able to fail more gracefully.
}

#ifdef CXING_RUN_PENDBG

#include <SafeTypes2.h>
extern bool trace;
void *PtrCapture_Keep(s2obj_t *x)
{
    if( trace )
    {
        printf("av+: %04x %p %d\n", x->type, x, x->keptcnt);
        printStackTrace();
    }
    return (s2obj_keep)(x);
}

void PtrCapture_Leave(s2obj_t *x)
{
    if( trace )
    {
        printf("av-: %04x %p %d\n", x->type, x, x->keptcnt);
        printStackTrace();
    }
    return (s2obj_leave)(x);
}

#include <execinfo.h>
void printStackTrace() {
    void *returnAddresses[512];
    int depth = backtrace(returnAddresses, sizeof returnAddresses / sizeof *returnAddresses);
    printf("\tstack depth = %d\n", depth);
    char **symbols = backtrace_symbols(returnAddresses, depth);
    for (int i = 0; i < depth; ++i) {
        printf("\t%s\n", symbols[i]);
    }
    (free)(symbols);
}

#endif /* CXING_RUN_PENDBG */
