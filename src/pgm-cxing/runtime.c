/* DannyNiu/NJF, 2025-10-02. Public Domain. */

#define _CRT_RAND_S // To use the `rand_s` Win32 API.
#include "langsem.h"
#include "runtime.h"
#include <SafeTypes2.h>

struct value_nativeobj CxingPropName_copy;
struct value_nativeobj CxingPropName_final;
struct value_nativeobj CxingPropName_equals;
struct value_nativeobj CxingPropName_cmpwith;
struct value_nativeobj CxingPropName_InitSet;
struct value_nativeobj CxingPropName_Proto;

struct value_nativeobj CxingPropName_Len;
struct value_nativeobj CxingPropName_Trunc;
struct value_nativeobj CxingPropName_Putc;
struct value_nativeobj CxingPropName_Puts;
struct value_nativeobj CxingPropName_Putfin;
struct value_nativeobj CxingPropName_Map;

struct {
    const char *istr;
    s2data_t *pdat;
    struct value_nativeobj *pvar;
} CxingPropNames[] = {
    { .istr = "__copy__", .pvar = &CxingPropName_copy },
    { .istr = "__final__", .pvar = &CxingPropName_final },
    { .istr = "equals", .pvar = &CxingPropName_equals },
    { .istr = "cmpwith", .pvar = &CxingPropName_cmpwith },
    { .istr = "__initset__", .pvar = &CxingPropName_InitSet },
    { .istr = "__proto__", .pvar = &CxingPropName_Proto },

    { .istr = "len", .pvar = &CxingPropName_Len },
    { .istr = "trunc", .pvar = &CxingPropName_Trunc },
    { .istr = "putc", .pvar = &CxingPropName_Putc },
    { .istr = "puts", .pvar = &CxingPropName_Puts },
    { .istr = "putfin", .pvar = &CxingPropName_Putfin },
    { .istr = "map", .pvar = &CxingPropName_Map },

    {0},
};

// The "Morgoth" null.
const type_nativeobj_struct_p0 type_nativeobj_morgoth = {
    .typeid = valtyp_obj, .n_entries = 0, .static_members[0] = {} };

// The "blessed" null.
const type_nativeobj_struct_p0 type_nativeobj_null = {
    .typeid = valtyp_null, .n_entries = 0, .static_members[0] = {} };

// The arithmetic types.
const type_nativeobj_struct_p0 type_nativeobj_long = {
    .typeid = valtyp_long, .n_entries = 0, .static_members[0] = {} };

const type_nativeobj_struct_p0 type_nativeobj_ulong = {
    .typeid = valtyp_ulong, .n_entries = 0, .static_members[0] = {} };

const type_nativeobj_struct_p0 type_nativeobj_double = {
    .typeid = valtyp_double, .n_entries = 0, .static_members[0] = {} };

const type_nativeobj_struct_p0 type_nativeobj_subr = {
    .typeid = valtyp_subr, .n_entries = 0, .static_members[0] = {} };

const type_nativeobj_struct_p0 type_nativeobj_method = {
    .typeid = valtyp_method, .n_entries = 0, .static_members[0] = {} };

#define Cxing_s2Type_MethodImpl(s2type, name)                   \
    struct value_nativeobj CxingImpl_##s2type##_##name(         \
        int argn, struct value_nativeobj args[]);               \
    struct value_nativeobj CxingValue_##s2type##_##name =       \
        (struct value_nativeobj){                               \
        .proper.p = CxingImpl_##s2type##_##name,                \
        .type = (const void *)&type_nativeobj_method };

Cxing_s2Type_MethodImpl(s2Dict, Get);
Cxing_s2Type_MethodImpl(s2Dict, Set);
Cxing_s2Type_MethodImpl(s2Dict, Copy);
Cxing_s2Type_MethodImpl(s2Dict, Final);
Cxing_s2Type_MethodImpl(s2Dict, Unset);
Cxing_s2Type_MethodImpl(s2Dict, NextKey);
Cxing_s2Type_MethodImpl(s2Dict, FirstKey);

Cxing_s2Type_MethodImpl(s2Data, Copy);
Cxing_s2Type_MethodImpl(s2Data, Final);
Cxing_s2Type_MethodImpl(s2Data, Len);
Cxing_s2Type_MethodImpl(s2Data, Trunc);
Cxing_s2Type_MethodImpl(s2Data, Putc);
Cxing_s2Type_MethodImpl(s2Data, Puts);
Cxing_s2Type_MethodImpl(s2Data, Putfin);
Cxing_s2Type_MethodImpl(s2Data, Map);
Cxing_s2Type_MethodImpl(s2Data, CmpWith);
Cxing_s2Type_MethodImpl(s2Data, Equals);

const type_nativeobj_struct_p8 type_nativeobj_s2impl_dict = {
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

struct s2cxing_value_iter {
    struct s2ctx_iter base;
    struct value_nativeobj val;
    bool done;
};

static int s2cxing_value_iter_step(s2cxing_value_iter_t *restrict iter)
{
    iter->base.key = NULL;
    if( !iter->done )
    {
        iter->base.value = iter->val.proper.p;
        iter->done = true;
        return 1;
    }
    else return 0;
}

static s2cxing_value_iter_t *s2cxing_value_iter_create(
    s2cxing_value_t *restrict dict)
{
    s2cxing_value_iter_t *iter = NULL;

    iter = calloc(1, sizeof(s2cxing_value_t));
    if( !iter ) return NULL;

    iter->base.final = (s2iter_final_func_t)free;
    iter->base.next = (s2iter_stepfunc_t)s2cxing_value_iter_step;
    iter->val = dict->cxing_value;
    iter->done = false;

    return iter;
}

static void s2cxing_value_final(s2cxing_value_t *s2v)
{
    ValueDestroy(s2v->cxing_value);
}

s2cxing_value_t *s2cxing_value_create(struct value_nativeobj val)
{
    s2cxing_value_t *ret;

    ret = (s2cxing_value_t *)s2gc_obj_alloc(
        S2_OBJ_TYPE_CXING_VALUE, sizeof(s2cxing_value_t));

    if( !ret )
    {
        CxingFatal("Unable to allocate runtime resource for value.\n");
        return NULL;
    }

    ret->cxing_value = val;
    ret->base.finalf = (s2func_final_t)s2cxing_value_final;

    if( val.type == (const void *)&type_nativeobj_s2impl_dict )
    {
        // A recognized implementation of dict.
        ret->base.itercreatf = (s2func_iter_create_t)s2cxing_value_iter_create;
    }

    return ret;
}

#define AssertArgN(n)                                           \
    if( argn < n ) return (struct value_nativeobj){             \
            .proper.p = NULL,                                   \
            .type = (const void *)&type_nativeobj_morgoth };

#define AssertArgImpl(n, typ, hr)                                       \
    if( args[n].type != (const void *)&type_nativeobj_##typ )           \
    {                                                                   \
        CxingDebug("Encountered unrecognized implementation of "        \
                   "the "hr" type in arg%d in function `%s`. "          \
                   "Does this object come from another runtime?\n",     \
                   n, __func__);                                        \
        return (struct value_nativeobj){                                \
            .proper.p = NULL,                                           \
            .type = (const void *)&type_nativeobj_morgoth };            \
    }

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
        CxingFatal("Cxing Runtime Internal Error: "
                   "SafeTypes2 Dictionary Getter Failure!\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
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
        CxingFatal("Unable to create SafeTypes2 binding "
                   "for the CXING value.\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
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
        CxingFatal("Cxing Runtime Internal Error: "
                   "SafeTypes2 Dictionary Setter Failure!\n");
        s2obj_release(ret_wrapped->pobj);
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
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
    s2obj_keep(args[0].proper.p);

    return args[0];
}

struct value_nativeobj CxingImpl_s2Dict_Final(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, s2impl_dict, "dict");
    s2obj_leave(args[0].proper.p);

    return (struct value_nativeobj){
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth };
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
            .type = (void *)&type_nativeobj_morgoth };
    }

    key = s2data_create(klen = s2data_len(iter->base.key));
    if( !key )
    {
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (void *)&type_nativeobj_morgoth };
    }

    s2obj_keep(key->pobj);
    s2obj_release(key->pobj);

    memcpy(s2data_weakmap(key), s2data_weakmap(iter->base.key), klen);
    return (struct value_nativeobj){
        .proper.p = key,
        .type = (void *)&type_nativeobj_s2impl_str };
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
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    s2obj_keep(dimpl->pobj);
    s2obj_release(dimpl->pobj);

    ret_wrapped = s2cxing_value_create(
        (struct value_nativeobj){
            .proper.p = CxingImpl_s2Dict_InitSet,
            .type = (const void *)&type_nativeobj_method });

    if( !ret_wrapped )
    {
        CxingFatal("Unable to create SafeTypes2 binding "
                   "for the CXING value.\n");
        s2obj_leave(dimpl->pobj);
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
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
        CxingFatal("Failed to create dict object.\n");
        s2obj_leave(dimpl->pobj);
        s2obj_release(ret_wrapped->pobj);
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }
}

struct value_nativeobj CxingImpl_s2Data_Copy(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, s2impl_str, "string");
    s2obj_keep(args[0].proper.p);

    return args[0];
}

struct value_nativeobj CxingImpl_s2Data_Final(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, s2impl_str, "string");
    s2obj_leave(args[0].proper.p);

    return (struct value_nativeobj){
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth };
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
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
}

struct value_nativeobj CxingImpl_s2Data_Putc(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(2);
    AssertArgImpl(0, s2impl_str, "string");

    if( s2data_putc(args[0].proper.p, ConvertToUlong(args[1]).proper.u) == 0 )
        return ValueCopy(args[0]);
    else return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
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
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
}

struct value_nativeobj CxingImpl_s2Data_Putfin(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, s2impl_str, "string");

    if( s2data_putfin(args[0].proper.p) == 0 )
        return ValueCopy(args[0]);
    else return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
}

struct value_nativeobj CxingImpl_s2Data_Map(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(2);
    AssertArgImpl(0, s2impl_str, "string");
    AssertArgImpl(1, s2impl_str, "string");

    CxingFatal("The `map` method for strings had not been implemented yet\n");
    return (struct value_nativeobj){
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth };
}

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
    AssertArgN(2);
    AssertArgImpl(0, s2impl_str, "string");
    AssertArgImpl(1, s2impl_str, "string");

    // TODO (2026-03-12).
    CxingFatal("The `equals` method for strings had not been _correctly_ implemented yet\n");

    return (struct value_nativeobj){
        .proper.l = s2data_cmp(args[0].proper.p, args[1].proper.p) == 0,
        .type = (const void *)&type_nativeobj_long };
}

cxing_builtin_def_t CxingRuntimeBuiltins[] = {
    { "dict", (struct value_nativeobj){
            .proper.p = CxingImpl_s2Dict_Create,
            .type = (const void *)&type_nativeobj_subr } },
    { 0 },
};

s2dict_t *CxingBuiltins = NULL;

bool CxingRuntimeInit()
{
    FILE *csprng;
    uint8_t seed[16] = {0};
    bool ret = true;
    int i;

    //
    // Initialize seed for SipHash.

    csprng = fopen("/dev/urandom", "rb");
    if( csprng )
    {
        fread(seed, 1, sizeof(seed), csprng);
        fclose(csprng);
        if( ferror(csprng) )
        {
            CxingWarning("IO error when reading CSPRNG for seeding SipHash.\n");
        }
        siphash_setkey(seed, sizeof(seed));
    }
    else
    {
#ifdef _WIN32
        unsigned int buf; // Eventhough we know the size of int on windows.
        size_t t = 0;
        while( t < sizeof(seed) )
        {
            rand_s(&buf);
            seed[t++] = buf;
        }
#else // Not Win32 platform.
        CxingWarning("IO error when opening CSPRNG for seeding SipHash.\n");
#endif // _WIN32
    }

    //
    // Initialize value native objects for string constants.

    for(i=0; CxingPropNames[i].istr; i++)
    {
        CxingPropNames[i].pdat = s2data_from_str(CxingPropNames[i].istr);
        if( !CxingPropNames[i].istr )
        {
            CxingFatal("String constant '%s' failed to allocate!\n",
                       CxingPropNames[i].istr);
            ret = false;
            goto strings_dealloc;
        }

        CxingPropNames[i].pvar->proper.p = CxingPropNames[i].pdat;
        CxingPropNames[i].pvar->type =
            (const void *)&type_nativeobj_s2impl_str;
    }

    CxingBuiltins = s2dict_create();
    if( !CxingBuiltins )
    {
        CxingFatal("Cannot create hash table for built-ins.\n");
        ret = false;
        goto builtin_dict_dealloc;
    }

    for(i=0; CxingRuntimeBuiltins[i].name; i++)
    {
        s2data_t *key;
        s2cxing_value_t *value;

        if( !(key = s2data_from_str(CxingRuntimeBuiltins[i].name)) )
        {
            CxingFatal("Built-in name constant '%s' failed to allocate!\n",
                       CxingRuntimeBuiltins[i].name);
            ret = false;
            goto builtin_dict_dealloc;
        }
        if( !(value = s2cxing_value_create(CxingRuntimeBuiltins[i].val)) )
        {
            // Not using `ValueCopy` as outlined in comments for
            // the `val` member of the `cxing_builtin_def_t` structure type.
            // Enjoyed the benefit of not having to check for copying failures.
            CxingFatal("Unable to create SafeTypes2 binding for built-in: '%s'\n",
                       CxingRuntimeBuiltins[i].name);
            ret = false;
            goto builtin_dict_dealloc;
        }
        if( s2dict_set(CxingBuiltins, key, value->pobj, s2_setter_gave)
            != s2_access_success )
        {
            CxingFatal("Runtime Initialization Error: "
                       "SafeTypes2 Dictionary Setter Failure!\n");
            ret = false;
            goto builtin_dict_dealloc;
        }
        s2obj_release(key->pobj);
    }

    return ret;

builtin_dict_dealloc:
    if( CxingBuiltins )
    {
        s2obj_release(CxingBuiltins->pobj);
        CxingBuiltins = NULL;
    }

strings_dealloc:
    while( i --> 0 )
    {
        s2obj_release(CxingPropNames[i].pdat->pobj);
        CxingPropNames[i].pdat = NULL;
    }

    return ret;
}

void CxingRuntimeFinal()
{
    int i;

    if( CxingBuiltins )
    {
        s2obj_release(CxingBuiltins->pobj);
        CxingBuiltins = NULL;
    }

    for(i=0; CxingPropNames[i].istr; i++)
    {
        if( CxingPropNames[i].pdat )
        {
            s2obj_release(CxingPropNames[i].pdat->pobj);
            CxingPropNames[i].pdat = NULL;
        }
    }
}
