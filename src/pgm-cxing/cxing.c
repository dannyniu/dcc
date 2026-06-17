/* DannyNiu/NJF, 2026-06-15. Public Domain. */

#include "cxing-interp.h"
#include "runtime.h"

extern struct value_nativeobj CxingImpl_Array_Set(
    int argn, struct value_nativeobj args[]);

extern struct value_nativeobj CxingImpl_Array_Trunc(
    int argn, struct value_nativeobj args[]);

int main(int argc, char *argv[])
{
    int subret = 0, i;
    s2data_t *idat, *arge;
    cxing_module_t *module;
    cxing_call_proto func = {};
    struct value_nativeobj funcarg[3];
    struct value_nativeobj progret;

    if( argc < 2 )
    {
        fprintf(stderr, "Must specify a file for the interpreter to run!\n"
                "usage: cxing <cxing-program-source-file> arguments...\n\n");
        return EXIT_FAILURE;
    }

    CXParserInitCommon();
    CxingRuntimeInit();

    module = CXOpen(argv[1]);
    func = CXSym(module, "main");

    idat = s2data_create(sizeof(int64_t));
    funcarg[0] = CxingImpl_Array_Create(0, funcarg);
    funcarg[1].proper.l = argc-1;
    funcarg[1].type = (const void *)&type_nativeobj_long;

    CxingImpl_Array_Trunc(2, funcarg);
    funcarg[1].proper.p = idat;
    funcarg[1].type = (const void *)&type_nativeobj_s2impl_str;
    funcarg[2].type = (const void *)&type_nativeobj_s2impl_str;
    
    for(i=1; i<argc; i++)
    {
        *(int64_t *)s2data_weakmap(idat) = i-1;
        arge = s2data_from_str(argv[i]);
        s2obj_keep(arge->pobj);
        s2obj_release(arge->pobj);
        funcarg[2].proper.p = arge;
        CxingImpl_Array_Set(3, funcarg);
    }

    funcarg[1] = funcarg[0];
    funcarg[0].proper.l = argc - 1;
    funcarg[0].type = (const void *)&type_nativeobj_long;
    
    progret = func(2, funcarg);

    s2obj_release(module->pobj);
    CxingRuntimeFinal();
    CXParserFinalCommon();

    subret = progret.proper.l;
    return subret;
}
