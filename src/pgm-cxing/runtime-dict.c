/* DannyNiu/NJF, 2026-05-25. Public Domain. */

#include "langsem.h"
#include "runtime.h"
#include <SafeTypes2.h>

CxingMethodValueWithImpl(s2Dict, Get);
CxingMethodValueWithImpl(s2Dict, Set);
CxingMethodValueWithImpl(s2Dict, Copy);
CxingMethodValueWithImpl(s2Dict, Final);
CxingMethodValueWithImpl(s2Dict, Unset);
CxingMethodValueWithImpl(s2Dict, NextKey);
CxingMethodValueWithImpl(s2Dict, FirstKey);

const type_nativeobj_struct_p7 type_nativeobj_s2impl_dict = {
    .typeid = valtyp_obj,
    .n_entries = 7,
    .static_members = {
        { .name = "__get__", .member = &CxingValue_s2Dict_Get },
        { .name = "__set__", .member = &CxingValue_s2Dict_Set },
        { .name = "__copy__", .member = &CxingValue_s2Dict_Copy },
        { .name = "__final__", .member = &CxingValue_s2Dict_Final },
        { .name = "__unset__", .member = &CxingValue_s2Dict_Unset },
        { .name = "nextkey", .member = &CxingValue_s2Dict_NextKey },
        { .name = "firstkey", .member = &CxingValue_s2Dict_FirstKey },
    },
};

struct value_nativeobj CxingImpl_s2Dict_Get(
    int argn, struct value_nativeobj args[])
{
    int x;
    s2cxing_value_t *ret_wrapped;

    AssertArgN(2);
    AssertArgImpl(0, s2impl_dict, "dict");
    AssertArgImpl(1, s2impl_str, "string");

    x = s2dict_get_T(s2cxing_value_t)(
        args[0].proper.p, args[1].proper.p, &ret_wrapped);

    if( x == s2_access_success )
    {
        return ret_wrapped->cxing_value;
    }
    else if( x == s2_access_nullval )
    {
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }
    else
    {
        x = errno;
        CxingFatal("Cxing Runtime Internal Error: "
                   "SafeTypes2 Dictionary Getter Failure!\n");
        return (struct value_nativeobj){
            .proper.l = x,
            .type = (const void *)&type_nativeobj_null };
    }
}

struct value_nativeobj CxingImpl_s2Dict_Set(
    int argn, struct value_nativeobj args[])
{
    int x;
    s2cxing_value_t *ret_wrapped;

    AssertArgN(3);
    AssertArgImpl(0, s2impl_dict, "dict");
    AssertArgImpl(1, s2impl_str, "string");

    ret_wrapped = s2cxing_value_create(
        // This copy is per spec section "Automatic Resource Management".
        ValueCopy(args[2]));

    if( !ret_wrapped )
    {
        x = errno;
        CxingFatal("Unable to create SafeTypes2 binding "
                   "for the CXING value.\n");
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    x = s2dict_set(
        args[0].proper.p, args[1].proper.p,
        ret_wrapped->pobj, s2_setter_gave);

    if( x == s2_access_success )
    {
        return ret_wrapped->cxing_value;
    }
    else if( x == s2_access_error )
    {
        x = errno;
        CxingFatal("Cxing Runtime Internal Error: "
                   "SafeTypes2 Dictionary Setter Failure!\n");
        s2obj_release(ret_wrapped->pobj);
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }
    else
    {
        CxingFatal("Unexpected return value from runtime implementation.\n");
        s2obj_release(ret_wrapped->pobj);
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }
}

struct value_nativeobj CxingImpl_s2Dict_Copy(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, s2impl_dict, "dict");
    return CxingImpl_s2Obj_Copy(argn, args);
}

struct value_nativeobj CxingImpl_s2Dict_Final(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, s2impl_dict, "dict");
    return CxingImpl_s2Obj_Final(argn, args);
}

struct value_nativeobj CxingImpl_s2Dict_Unset(
    int argn, struct value_nativeobj args[])
{
    int x;

    AssertArgN(2);
    AssertArgImpl(0, s2impl_dict, "dict");

    x = s2dict_unset(
        args[0].proper.p, args[1].proper.p);

    if( x != s2_access_success )
    {
        CxingFatal("Cxing Runtime Internal Error: "
                   "SafeTypes2 Dictionary Getter Failure!\n");
    }

    return (struct value_nativeobj){
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth };
}

struct value_nativeobj CxingImpl_s2Dict_InitSet(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(3);
    AssertArgImpl(0, s2impl_dict, "dict");
    AssertArgImpl(1, s2impl_str, "string");

    if( strcmp(s2data_weakmap(args[1].proper.p), "__proto__") == 0 )
    {
        s2dict_unset(args[0].proper.p, CxingPropName_InitSet.proper.p);
        return args[0];
    }
    else
    {
        return CxingImpl_s2Dict_Set(argn, args);
    }
}

struct value_nativeobj CxingImpl_s2Dict_Step_Checked(s2dict_iter_t *iter)
{
    size_t klen;
    s2data_t *key;
    int x = s2dict_iter_step(iter);
    if( x <= 0 )
    {
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    key = s2data_create(klen = s2data_len(iter->base.key));
    if( !key )
    {
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    s2obj_keep(key->pobj);
    s2obj_release(key->pobj);

    memcpy(s2data_weakmap(key), s2data_weakmap(iter->base.key), klen);
    return (struct value_nativeobj){
        .proper.p = key,
        .type = (const void *)&type_nativeobj_s2impl_str };
}

static_assert( S2_DICT_HASH_MAX == 16,
               "Code changed too radically, cannot compile!");

struct value_nativeobj CxingImpl_s2Dict_FirstKey(
    int argn, struct value_nativeobj args[])
{
    s2dict_iter_t di;

    AssertArgN(1);
    AssertArgImpl(0, s2impl_dict, "dict");

    s2dict_iter_init(&di, args[0].proper.p);
    return CxingImpl_s2Dict_Step_Checked(&di);
}

struct value_nativeobj CxingImpl_s2Dict_NextKey(
    int argn, struct value_nativeobj args[])
{
    int i;
    int level = 0;
    siphash_t hx;
    uint8_t hash[S2_DICT_HASH_MAX];

    struct s2ctx_dict_table *V;
    s2dict_iter_t di;
    s2data_t *key;
    size_t klen;

    AssertArgN(2);
    AssertArgImpl(0, s2impl_dict, "dict");
    AssertArgImpl(1, s2impl_str, "string");

    s2dict_iter_init(&di, args[0].proper.p);

    key = args[1].proper.p;
    klen = s2data_len(key);

    // Hash the key.
    SipHash_o128_Init(&hx, s2dict_siphash_key, S2_DICT_HASH_MAX);
    SipHash_c2_Update(&hx, s2data_weakmap(key), klen);
    SipHash_c2d4o128_Final(&hx, hash, S2_DICT_HASH_MAX);
    for(i=0; i<S2_DICT_HASH_MAX; i++)
        di.iterpos[i] = hash[i];

    // Reconstruct iterator position from the hash.
    V = &di.dict->root;
    level = 0;
    while( true )
    {
        // Shouldn't happen when reading from the dict.
        assert( level < S2_DICT_HASH_MAX );

        if( V->members[di.iterpos[level]].flags != s2_dict_member_collision )
            break;

        V = V->members[di.iterpos[level]].nested;
        level ++;
    }
    di.iterpos[level] ++;
    di.iterlevel = level;

    return CxingImpl_s2Dict_Step_Checked(&di);
}

struct value_nativeobj CxingImpl_s2Dict_Create(
    int argn, struct value_nativeobj args[])
{
    s2dict_t *dimpl = s2dict_create();
    s2cxing_value_t *ret_wrapped;
    int x;

    (void)argn;
    (void)args;

    if( !dimpl )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    s2obj_keep(dimpl->pobj);
    s2obj_release(dimpl->pobj);

    ret_wrapped = s2cxing_value_create(
        (struct value_nativeobj){
            .proper.p = CxingImpl_s2Dict_InitSet,
            .type = (const void *)&type_nativeobj_method });

    if( !ret_wrapped )
    {
        x = errno;
        CxingFatal("Unable to create SafeTypes2 binding "
                   "for the CXING value.\n");
        s2obj_leave(dimpl->pobj);
        return (struct value_nativeobj){
            .proper.l = x,
            .type = (const void *)&type_nativeobj_null };
    }

    x = s2dict_set(dimpl, CxingPropName_InitSet.proper.p,
                   ret_wrapped->pobj, s2_setter_gave);

    if( x == s2_access_success )
    {
        return (struct value_nativeobj){
            .proper.p = dimpl,
            .type = (const void *)&type_nativeobj_s2impl_dict };
    }
    else
    {
        x = errno;
        CxingFatal("Failed to create dict object.\n");
        s2obj_leave(dimpl->pobj);
        s2obj_release(ret_wrapped->pobj);
        return (struct value_nativeobj){
            .proper.l = x,
            .type = (const void *)&type_nativeobj_null };
    }
}
