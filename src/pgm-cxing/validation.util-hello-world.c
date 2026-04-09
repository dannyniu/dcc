/* DannyNiu/NJF, 2026-01-10. Public Domain. */

#include "cxing-interp.h"
#include "runtime.h"
#include "cxing-stdlib.h"

extern bool trace;

int main(int argc, char *argv[])
{
    int subret = 0, i;
    cxing_module_t *module;
    cxing_call_proto func = {};
    struct value_nativeobj funcarg;
    struct value_nativeobj funcret;

#if INTERCEPT_MEM_CALLS
    long acq_before = 0;
    long rel_before = 0;
    long acq_after = 0;
    long rel_after = 0;
#endif /* INTERCEPT_MEM_CALLS */

    if( argc < 2 )
    {
        fprintf(stderr, "Must specify a file!\n");
        return EXIT_FAILURE;
    }

    CXParserInitCommon();
    CxingRuntimeInit();

    module = CXOpen(argv[1]);
    CXExpose(module, "print", (struct value_nativeobj){
            .proper.p = CxingStdlibFunc_Print,
            .type = (const void *)&type_nativeobj_subr,
        });
    //CxingModuleDump(module);

    func = CXSym(module, "main");
    printf("args: %p, func: %p, module: %p.\n", &funcarg, func, module);

    trace = 0;

    funcret = func(0, &funcarg);
    printf("Execution of function `main` returned: %lld.\n",
           funcret.proper.l);
    //CxingModuleDump(module);

    trace = 0;

    s2obj_release(module->pobj);

    CxingRuntimeFinal();
    CXParserFinalCommon();

    subret = EXIT_SUCCESS;

    s2obj_t *gctail = s2gc_obj_alloc(0x6543, 128);
    s2obj_t *gcsave = gctail;
    i=0;
    for(i=0; gctail->gc_prev; i++)
    {
        printf("%d: (%p) %x %d+%d.\n", i, gctail, gctail->type, gctail->refcnt, gctail->keptcnt);
        if( s2_is_token(gctail) )
        {
            lex_token_t *tok = (void *)gctail;
            print_token(tok, 0);
        }
        if( s2_is_data(gctail) )
        {
            printf("%s (%p)\n", (const char *)s2data_weakmap((s2data_t *)gctail), gctail);
        }
        gctail = gctail->gc_prev;
    }
    s2obj_release(gcsave);

#if INTERCEPT_MEM_CALLS
    acq_after = allocs;
    rel_after = frees;
    printf("acq-before: %ld, acq-after: %ld.\n", acq_before, acq_after);
    printf("rel-before: %ld, rel-after: %ld.\n", rel_before, rel_after);
    printf("mem-acquire: %ld, mem-release: %ld.\n", allocs, frees);
    for(i=0; i<4; i++)
    {
        if( mh[i] ) subret = EXIT_FAILURE;
        printf("%08lx%c", (long)mh[i], i==3 ? '\n' : ' ');
    }
#endif /* INTERCEPT_MEM_CALLS */
    return subret;
}
