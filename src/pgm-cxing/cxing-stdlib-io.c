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

    if( args[0].type != (const void *)&type_nativeobj_s2impl_str )
    {
        CxingDiagnose("Unrecognized implementation of the string type. "
                      "Does this object come from another runtime?");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    x = args[0].proper.p;
    ret = fwrite(s2data_weakmap(x), 1, s2data_len(x), stdout);
    ret += putchar('\n') != EOF;

    return (struct value_nativeobj){
        .proper.u = ret,
        .type = (const void *)&type_nativeobj_ulong };
}
