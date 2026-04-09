/* DannyNiu/NJF, 2026-01-24. Public Domain. */

#include "cxing-stdlib.h"

struct value_nativeobj CxingStdlibFunc_Print(
    int argn, struct value_nativeobj args[])
{
    s2data_t *x;
    size_t ret;

    if( argn < 1 )
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };

    if( args[0].type->typeid == valtyp_long )
    {
        ret = printf("%lld\n", (long long)args[0].proper.l);
    }

    else if( args[0].type->typeid == valtyp_ulong )
    {
        ret = printf("%llu\n", (unsigned long long)args[0].proper.u);
    }

    else if( args[0].type->typeid == valtyp_double )
    {
        ret = printf("%f\n", args[0].proper.f);
    }

    else if( args[0].type == (const void *)&type_nativeobj_s2impl_str )
    {
        x = args[0].proper.p;
        ret = fwrite(s2data_weakmap(x), 1, s2data_len(x), stdout);
        ret += putchar('\n') != EOF;
    }

    else if( IsNull(args[0]) )
    {
        ret = printf("<null/%llu/%llu>\n",
                     args[0].type->typeid,
                     args[0].proper.u);
    }

    else
    {
        ret = printf("<%p/%s>\n",
                     args[0].proper.p,
                     args[0].type->typeid == valtyp_obj ? "obj" :
                     args[0].type->typeid == valtyp_subr ? "subr" :
                     args[0].type->typeid == valtyp_method ? "method" :
                     "-unknown-");
    }

    return (struct value_nativeobj){
        .proper.u = ret,
        .type = (const void *)&type_nativeobj_ulong };
}
