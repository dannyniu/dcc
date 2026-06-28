/* DannyNiu/NJF, 2026-05-25. Public Domain. */

#include "langsem.h"
#include "runtime.h"
#include <SafeTypes2.h>

CxingMethodValueWithImpl(s2Data, Copy);
CxingMethodValueWithImpl(s2Data, Final);
CxingMethodValueWithImpl(s2Data, Len);
CxingMethodValueWithImpl(s2Data, Trunc);
CxingMethodValueWithImpl(s2Data, Putc);
CxingMethodValueWithImpl(s2Data, Puts);
CxingMethodValueWithImpl(s2Data, Putfin);

extern struct value_nativeobj CxingImpl_s2Data_Map(
    int argn, struct value_nativeobj args[]);
static struct value_nativeobj CxingValue_s2Data_Map =
    (struct value_nativeobj){
    .proper.p = CxingImpl_s2Data_Map,
    .type = (const void *)&type_nativeobj_method };
// CxingMethodValueWithImpl(s2Data, Map); // Defined in another TU.

CxingMethodValueWithImpl(s2Data, CmpWith);
CxingMethodValueWithImpl(s2Data, Equals);

const type_nativeobj_struct_p9 type_nativeobj_s2impl_str = {
    .typeid = valtyp_obj,
    .n_entries = 9,
    .static_members = {
        { .name = "__copy__", .member = &CxingValue_s2Data_Copy },
        { .name = "__final__", .member = &CxingValue_s2Data_Final },
        { .name = "len", .member = &CxingValue_s2Data_Len },
        { .name = "trunc", .member = &CxingValue_s2Data_Trunc },
        { .name = "putc", .member = &CxingValue_s2Data_Putc },
        { .name = "puts", .member = &CxingValue_s2Data_Puts },
        { .name = "putfin", .member = &CxingValue_s2Data_Putfin },
        { .name = "map", .member = &CxingValue_s2Data_Map },
        { .name = "equals", .member = &CxingValue_s2Data_Equals },
        { .name = "cmpwith", .member = &CxingValue_s2Data_CmpWith },
        // 2025-12-13: TODO (2026-04-06: not done yet).
    },
};

struct value_nativeobj CxingImpl_s2Data_Copy(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, s2impl_str, "string");
    return CxingImpl_s2Obj_Copy(argn, args);
}

struct value_nativeobj CxingImpl_s2Data_Final(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, s2impl_str, "string");
    return CxingImpl_s2Obj_Final(argn, args);
}

struct value_nativeobj CxingImpl_s2Data_Len(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, s2impl_str, "string");

    return (struct value_nativeobj){
        .proper.u = s2data_len(args[0].proper.p),
        .type = (const void *)&type_nativeobj_ulong };
}

struct value_nativeobj CxingImpl_s2Data_Trunc(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(2);
    AssertArgImpl(0, s2impl_str, "string");

    // 2026-03-05 TODO:
    // What if I want to know whether the failure was caused by
    // reallocation failure or lingering mapping?

    if( s2data_trunc(args[0].proper.p, ConvertToUlong(args[1]).proper.u) == 0 )
        return ValueCopy(args[0]);
    else return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
}

struct value_nativeobj CxingImpl_s2Data_Putc(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(2);
    AssertArgImpl(0, s2impl_str, "string");

    if( s2data_putc(args[0].proper.p, ConvertToUlong(args[1]).proper.u) == 0 )
        return ValueCopy(args[0]);
    else return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
}

struct value_nativeobj CxingImpl_s2Data_Puts(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(2);
    AssertArgImpl(0, s2impl_str, "string");
    AssertArgImpl(1, s2impl_str, "string");

    if( s2data_puts(args[0].proper.p,
                    s2data_weakmap(args[1].proper.p),
                    s2data_len(args[1].proper.p)) == 0 )
        return ValueCopy(args[0]);
    else return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
}

struct value_nativeobj CxingImpl_s2Data_Putfin(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, s2impl_str, "string");

    if( s2data_putfin(args[0].proper.p) == 0 )
        return ValueCopy(args[0]);
    else return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
}

//- struct value_nativeobj CxingImpl_s2Data_Map(
//-     int argn, struct value_nativeobj args[]);
// Defined in another TU.

struct value_nativeobj CxingImpl_s2Data_CmpWith(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(2);
    AssertArgImpl(0, s2impl_str, "string");
    AssertArgImpl(1, s2impl_str, "string");

    return (struct value_nativeobj){
        .proper.l = s2data_cmp(args[0].proper.p, args[1].proper.p),
        .type = (const void *)&type_nativeobj_long };
}

struct value_nativeobj CxingImpl_s2Data_Equals(
    int argn, struct value_nativeobj args[])
{
    size_t t;
    uint8_t d = 0, *a, *b;
    AssertArgN(2);
    AssertArgImpl(0, s2impl_str, "string");
    AssertArgImpl(1, s2impl_str, "string");

    // TODO (2026-03-12).
    //CxingFatal("The `equals` method for strings had not been _correctly_ implemented yet\n");

    if( (t = s2data_len(args[0].proper.p)) != s2data_len(args[1].proper.p) )
    {
        return (struct value_nativeobj){
            .proper.l = false,
            .type = (const void *)&type_nativeobj_long };
    }

    a = s2data_weakmap(args[0].proper.p);
    b = s2data_weakmap(args[1].proper.p);

    while( t --> 0 )
    {
        d |= a[t] ^ b[t];
    }

    d |= d >> 4;
    d |= d >> 2;
    d |= d >> 1;
    d = (d ^ 1) & 1;

    return (struct value_nativeobj){
        .proper.l = d,
        .type = (const void *)&type_nativeobj_long };
}
