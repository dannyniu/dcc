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
    CxingModuleDump(module);

    func = CXSym(module, "main");
    printf("args: %p, func: %p, module: %p.\n", &funcarg, func, module);

    funcret = func(0, &funcarg);
    printf("Execution of function `main` returned: %lld.\n",
           funcret.proper.l);

    s2obj_release(module->pobj);

    CxingRuntimeFinal();
    CXParserFinalCommon();

    subret = EXIT_SUCCESS;

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
