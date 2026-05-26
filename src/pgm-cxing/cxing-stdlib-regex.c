/* DannyNiu/NJF, 2026-05-24. Public Domain. */

#include "cxing-stdlib.h"
#include <librematch.h>

#define S2_OBJ_TYPE_REGEXP 0x211B
#define s2_is_regexp(obj) (((s2obj_t *)obj)->type == S2_OBJ_TYPE_REGEXP)

typedef struct cxing_regex_ctx {
    s2obj_base;

    libregexp_t preg;
} cxing_regex_t;

static int regcomp_map_flags(int64_t x)
{
    int ret = 0;
    if( x == 0 ) return 0;

    while( x )
    {
        int f = x & 63;

        if( f == 34 ) // lower-case 'i'.
        {
            ret |= LIBREG_ICASE;
        }
        else if( f == 39 )
        {
            ret |= LIBREG_NEWLINE;
        }

        x >>= 6;
    }
    return x;
}

static void CxingRegexFinal(cxing_regex_t *x)
{
    libregfree(&x->preg);
}

static struct value_nativeobj CxingRegex_Compile(
    int argn, struct value_nativeobj args[], int cflags_additional)
{
    cxing_regex_t *ret;
    int subret;

    AssertArgN(2);
    AssertArgImpl(0, s2impl_str, "string");
    if( !IsInteger(args[1]) )
    {
        CxingDebug("Regex compilation flags need to be an integer!\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    ret = (void *)s2gc_obj_alloc(S2_OBJ_TYPE_REGEXP, sizeof(cxing_regex_t));
    if( !ret )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    ret->base.finalf = (s2func_final_t)CxingRegexFinal;
    s2obj_keep(ret->pobj);
    s2obj_release(ret->pobj);

    subret = libregcomp(
        &ret->preg, s2data_weakmap(args[0].proper.p),
        regcomp_map_flags(args[1].proper.l) | cflags_additional);
    if( subret != 0 )
    {
        s2obj_leave(ret->pobj);

        // For the reference implementation, the return value from failing
        // regex compulations are negative, which are guaranteed to be
        // separate from `errno` values.
        return (struct value_nativeobj){
            .proper.l = subret,
            .type = (const void *)&type_nativeobj_null };
    }

    return (struct value_nativeobj){
        .proper.p = ret,
        .type = (const void *)&type_nativeobj_Regex };
}

struct value_nativeobj CxingRegex_EREComp(
    int argn, struct value_nativeobj args[])
{
    return CxingRegex_Compile(argn, args, LIBREG_EXTENDED);
}

struct value_nativeobj CxingRegex_BREComp(
    int argn, struct value_nativeobj args[])
{
    return CxingRegex_Compile(argn, args, 0);
}

cxing_builtin_def_t CxingStdlibStructBuiltins[] = {
    { "ere_comp", (struct value_nativeobj){
            .proper.p = CxingRegex_EREComp,
            .type = (const void *)&type_nativeobj_subr } },

    { "bre_comp", (struct value_nativeobj){
            .proper.p = CxingRegex_BREComp,
            .type = (const void *)&type_nativeobj_subr } },

    {}
};
