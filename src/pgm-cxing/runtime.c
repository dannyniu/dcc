/* DannyNiu/NJF, 2025-10-02. Public Domain. */

#define _CRT_RAND_S // To use the `rand_s` Win32 API.
#include "langsem.h"
#include "runtime.h"
#include "cxing-stdlib.h"
#include <SafeTypes2.h>

#if _WIN32
#include <windows.h>
#else // Assume POSIX.
#include <dlfcn.h>
#endif // _WIN32

struct value_nativeobj CxingImpl_s2Obj_Copy(
    int argn, struct value_nativeobj args[])
{
    (void)argn;
    s2obj_keep(args[0].proper.p);
    return args[0];
}

struct value_nativeobj CxingImpl_s2Obj_Final(
    int argn, struct value_nativeobj args[])
{
    (void)argn;
    s2obj_leave(args[0].proper.p);
    return (struct value_nativeobj){
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth };
}

struct value_nativeobj CxingValue_s2Obj_Copy =
    (struct value_nativeobj){
    .proper.p = CxingImpl_s2Obj_Copy,
    .type = (const void *)&type_nativeobj_method };

struct value_nativeobj CxingValue_s2Obj_Final =
    (struct value_nativeobj){
    .proper.p = CxingImpl_s2Obj_Final,
    .type = (const void *)&type_nativeobj_method };

// Used by language semantic.
struct value_nativeobj CxingPropName_copy;
struct value_nativeobj CxingPropName_final;
struct value_nativeobj CxingPropName_equals;
struct value_nativeobj CxingPropName_cmpwith;
struct value_nativeobj CxingPropName_InitSet;
struct value_nativeobj CxingPropName_Proto;

// Used by the structures standard library.
struct value_nativeobj CxingPropName_Size;
struct value_nativeobj CxingPropName_Align;

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

    { .istr = "size", .pvar = &CxingPropName_Size },
    { .istr = "align", .pvar = &CxingPropName_Align },

    {0},
};

// The "Morgoth" null.
const type_nativeobj_struct_p0 type_nativeobj_morgoth = {
    .typeid = valtyp_obj, .n_entries = 0, .static_members[0] = {} };

// The "blessed" null.
const type_nativeobj_struct_p0 type_nativeobj_null = {
    .typeid = valtyp_null, .n_entries = 0, .static_members[0] = {} };

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
        if( !ValueIsBindingForSafeTypes2(iter->val) )
        {
            // Not managed by SafeTypes2 GC.
            iter->done = true;
            return 0;
        }
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

    iter = (calloc)(1, sizeof(s2cxing_value_t));
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

struct value_nativeobj Cxing_NullUncast(
    int argn, struct value_nativeobj args[])
{
    (void)argn;
    if( args[0].type->typeid == valtyp_null )
    {
        return (struct value_nativeobj){
            .proper.l = args[0].proper.l,
            .type = (const void *)&type_nativeobj_long };
    }
    else if( IsNull(args[0]) )
    {
        // The Morgoth.
        return args[0];
    }
    else
    {
        // Not a null, see spec for rationale.
        return (struct value_nativeobj){
            .proper.l = 0,
            .type = (const void *)&type_nativeobj_long };
    }
}

struct value_nativeobj Cxing_IsNull(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);

    return (struct value_nativeobj){
        .proper.l = IsNull(args[0]),
        .type = (const void *)&type_nativeobj_long };
}

struct value_nativeobj Cxing_IsLong(
    int argn, struct value_nativeobj args[])
{
    int ret = false;
    AssertArgN(1);

    if( args[0].type->typeid == valtyp_long )
        ret = true;
    else ret = false;

    return (struct value_nativeobj){
        .proper.l = ret,
        .type = (const void *)&type_nativeobj_long };
}

struct value_nativeobj Cxing_IsULong(
    int argn, struct value_nativeobj args[])
{
    int ret = false;
    AssertArgN(1);

    if( args[0].type->typeid == valtyp_ulong )
        ret = true;
    else ret = false;

    return (struct value_nativeobj){
        .proper.l = ret,
        .type = (const void *)&type_nativeobj_long };
}

struct value_nativeobj Cxing_IsDouble(
    int argn, struct value_nativeobj args[])
{
    int ret = false;
    AssertArgN(1);

    if( args[0].type->typeid == valtyp_double )
        ret = true;
    else ret = false;

    return (struct value_nativeobj){
        .proper.l = ret,
        .type = (const void *)&type_nativeobj_long };
}

struct value_nativeobj cxing_gc(
    int argn, struct value_nativeobj args[])
{
    (void)argn;
    (void)args;

    s2gc_collect();

    return (struct value_nativeobj){
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth };
}

cxing_builtin_def_t CxingRuntimeBuiltins[] = {
    { "_Uncast", (struct value_nativeobj){
            .proper.p = Cxing_NullUncast,
            .type = (const void *)&type_nativeobj_subr } },

    { "dict", (struct value_nativeobj){
            .proper.p = CxingImpl_s2Dict_Create,
            .type = (const void *)&type_nativeobj_subr } },

    { "array", (struct value_nativeobj){
            .proper.p = CxingImpl_Array_Create,
            .type = (const void *)&type_nativeobj_subr } },

    { "print", (struct value_nativeobj){
            .proper.p = CxingStdlibFunc_Print,
            .type = (const void *)&type_nativeobj_subr } },

    { "input", (struct value_nativeobj){
            .proper.p = CxingStdlibFunc_Input,
            .type = (const void *)&type_nativeobj_subr } },

    { "isnull", (struct value_nativeobj){
            .proper.p = Cxing_IsNull,
            .type = (const void *)&type_nativeobj_subr } },

    { "islong", (struct value_nativeobj){
            .proper.p = Cxing_IsLong,
            .type = (const void *)&type_nativeobj_subr } },

    { "isulong", (struct value_nativeobj){
            .proper.p = Cxing_IsULong,
            .type = (const void *)&type_nativeobj_subr } },

    { "isdouble", (struct value_nativeobj){
            .proper.p = Cxing_IsDouble,
            .type = (const void *)&type_nativeobj_subr } },

    { "cxing_gc", (struct value_nativeobj){
            .proper.p = cxing_gc,
            .type = (const void *)&type_nativeobj_subr } },

    { 0 },
};

s2dict_t *CxingBuiltins = NULL;
void *CxingInterpLoadedDyn = NULL;

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

    if( CxingInitialization_DefineStandardLibrary() != 0 )
    {
        CxingFatal("Unable to load the standard libraries!\n");
        ret = false;
        goto builtin_dict_dealloc;
    }

#if _WIN32
    if( !(CxingInterpLoadedDyn = GetModuleHandle(NULL)) )
    {
        CxingFatal("Unable to open the handle to the global symbols table.\n");
        ret = false;
        goto builtin_dict_dealloc;
    }
#else // Assume POSIX.
    if( !(CxingInterpLoadedDyn = dlopen(NULL, RTLD_NOW|RTLD_GLOBAL)) )
    {
        CxingFatal("Unable to open the handle to the global symbols table.\n");
        ret = false;
        goto builtin_dict_dealloc;
    }
#endif // _WIN32

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

int CxingBuiltinsExtend(const char *name, struct value_nativeobj value)
{
    s2data_t *key;
    s2cxing_value_t *val;

    // 2026-05-08 TODO: Not strictly built-in.

    if( !(key = s2data_from_str(name)) )
    {
        CxingFatal("Standard name constant '%s' failed to allocate!\n", name);
        return -1;
    }
    if( !(val = s2cxing_value_create(value)) )
    {
        // Not using `ValueCopy` as outlined in comments for
        // the `val` member of the `cxing_builtin_def_t` structure type.
        // Enjoyed the benefit of not having to check for copying failures.
        CxingFatal("Unable to create SafeTypes2 binding "
                   "for standard entity: '%s'\n", name);
        return -1;
    }
    if( s2dict_set(CxingBuiltins, key, val->pobj, s2_setter_gave)
        != s2_access_success )
    {
        CxingFatal("Runtime Initialization Error: "
                   "SafeTypes2 Dictionary Setter Failure!\n");
        return -1;
    }
    s2obj_release(key->pobj);
    return 0;
}
