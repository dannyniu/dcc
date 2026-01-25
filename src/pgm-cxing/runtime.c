/* DannyNiu/NJF, 2025-10-02. Public Domain. */

#define _CRT_RAND_S // To use the `rand_s` Win32 API.
#include "langsem.h"
#include "runtime.h"
#include <SafeTypes2.h>
#include <stdarg.h>

struct value_nativeobj CxingPropName_copy;
struct value_nativeobj CxingPropName_final;
struct value_nativeobj CxingPropName_equals;
struct value_nativeobj CxingPropName_cmpwith;
struct value_nativeobj CxingPropName_InitSet;
struct value_nativeobj CxingPropName_Proto;

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
    {0},
};

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
            CxingWarning("IO error when reading CSPRNG for seeding SipHash.");
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
        CxingWarning("IO error when opening CSPRNG for seeding SipHash.");
#endif // _WIN32
    }

    //
    // Initialize value native objects for string constants.

    for(i=0; CxingPropNames[i].istr; i++)
    {
        CxingPropNames[i].pdat = s2data_from_str(CxingPropNames[i].istr);
        if( !CxingPropNames[i].istr )
        {
            CxingFatal("String constant '%s' failed to allocate!",
                       CxingPropNames[i].istr);
            ret = false;
            goto strings_dealloc;
        }

        CxingPropNames[i].pvar->proper.p = CxingPropNames[i].pdat;
        CxingPropNames[i].pvar->type =
            (const void *)&type_nativeobj_s2impl_str;
    }

    return ret;

strings_dealloc:
    while( i --> 0 )
    {
        s2obj_release(CxingPropNames[i].pdat->pobj);
    }

    return ret;
}

void CxingRuntimeFinal()
{
    int i;
    for(i=0; CxingPropNames[i].istr; i++)
    {
        if( CxingPropNames[i].pdat )
        {
            s2obj_release(CxingPropNames[i].pdat->pobj);
            CxingPropNames[i].pdat = NULL;
        }
    }

}

void CxingDebug(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    fprintf(stderr, "[CxingDebug]: ");
    vfprintf(stderr, msg, ap);
    va_end(ap);
}

void CxingDiagnose(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    fprintf(stderr, "[CxingDiagnose]: ");
    vfprintf(stderr, msg, ap);
    va_end(ap);
    exit(1); // TODO (2026-01-01): try to fail more gracefully.
}

void CxingWarning(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    fprintf(stderr, "[CxingWarning]: ");
    vfprintf(stderr, msg, ap);
    va_end(ap);
}

void CxingFatal(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    fprintf(stderr, "[CxingFatal]: ");
    vfprintf(stderr, msg, ap);
    va_end(ap);
    abort(); // TODO (2025-12-31): I might be able to fail more gracefully.
}

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

Cxing_s2Type_MethodImpl(s2Data, Copy);
Cxing_s2Type_MethodImpl(s2Data, Final);

const type_nativeobj_struct_p6 type_nativeobj_s2impl_dict = {
    .typeid = valtyp_obj,
    .n_entries = 5, // Will be 7 (or bigger) during release engineering,
    .static_members = {
        { .name = "__get__", .member = &CxingValue_s2Dict_Get },
        { .name = "__set__", .member = &CxingValue_s2Dict_Set },
        { .name = "__copy__", .member = &CxingValue_s2Dict_Copy },
        { .name = "__final__", .member = &CxingValue_s2Dict_Final },
        { .name = "__unset__", .member = &CxingValue_s2Dict_Unset },
    },
};

const type_nativeobj_struct_p8 type_nativeobj_s2impl_str = {
    .typeid = valtyp_obj,
    .n_entries = 2, // Will be 8 (or bigger) during release engineering,
    .static_members = {
        { .name = "__copy__", .member = &CxingValue_s2Data_Copy },
        { .name = "__final__", .member = &CxingValue_s2Data_Final },
        // 2025-12-13: TODO.
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
        CxingFatal("Unable to allocate runtime resource for value.");
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

struct value_nativeobj CxingImpl_s2Dict_Get(
    int argn, struct value_nativeobj args[])
{
    int x;
    s2cxing_value_t *ret_wrapped;

    if( argn < 2 ) return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };

    if( args[0].type != (const void *)&type_nativeobj_s2impl_dict )
    {
        CxingDiagnose("Unrecognized implementation of the dict type. "
                      "Does this object come from another runtime?");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

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
        CxingDiagnose("Cxing Runtime Internal Error: "
                      "SafeTypes2 Dictionary Getter Failure!");
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

    if( argn < 3 ) return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };

    if( args[0].type != (const void *)&type_nativeobj_s2impl_dict )
    {
        CxingDiagnose("Unrecognized implementation of the dict type. "
                      "Does this object come from another runtime?");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    if( args[1].type != (const void *)&type_nativeobj_s2impl_str )
    {
        CxingDiagnose("Unrecognized implementation of the string type. "
                      "Does this object come from another runtime?");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    ret_wrapped = s2cxing_value_create(
        // 2025-12-31 (TODO: come back later):
        // This copy is per spec section "Automatic Resource Management".
        ValueCopy(args[2]));

    if( !ret_wrapped )
    {
        CxingFatal("Unable to create SafeTypes2 binding for the CXING value.");
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
    else if( x == s2_access_nullval )
    {
        CxingDiagnose("Unexpected return value from runtime implementation.");
        s2obj_release(ret_wrapped->pobj);
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }
    else
    {
        CxingDiagnose("Cxing Runtime Internal Error: "
                      "SafeTypes2 Dictionary Setter Failure!");
        s2obj_release(ret_wrapped->pobj);
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }
}

struct value_nativeobj CxingImpl_s2Dict_Copy(
    int argn, struct value_nativeobj args[])
{
    if( argn < 1 ) return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };

    if( args[0].type != (const void *)&type_nativeobj_s2impl_dict )
    {
        CxingDiagnose("Unrecognized implementation of the dict type. "
                      "Does this object come from another runtime?");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    s2obj_keep(args[0].proper.p);

    return args[0];
}

struct value_nativeobj CxingImpl_s2Dict_Final(
    int argn, struct value_nativeobj args[])
{
    if( argn < 1 ) return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };

    if( args[0].type != (const void *)&type_nativeobj_s2impl_dict )
    {
        CxingDiagnose("Unrecognized implementation of the dict type. "
                      "Does this object come from another runtime?");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    s2obj_leave(args[0].proper.p);

    return (struct value_nativeobj){
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth };
}

struct value_nativeobj CxingImpl_s2Dict_Unset(
    int argn, struct value_nativeobj args[])
{
    int x;

    if( argn < 2 ) return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };

    if( args[0].type != (const void *)&type_nativeobj_s2impl_dict )
    {
        CxingDiagnose("Unrecognized implementation of the dict type. "
                      "Does this object come from another runtime?");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    x = s2dict_unset(
        args[0].proper.p, args[1].proper.p);

    if( x != s2_access_success )
    {
        CxingDiagnose("Cxing Runtime Internal Error: "
                      "SafeTypes2 Dictionary Getter Failure!");
    }

    return (struct value_nativeobj){
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth };
}

struct value_nativeobj CxingImpl_s2Dict_Create(
    int argn, struct value_nativeobj args[])
{
    s2dict_t *dimpl = s2dict_create();

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

    return (struct value_nativeobj){
        .proper.p = dimpl,
        .type = (const void *)&type_nativeobj_s2impl_dict };
}

// 2026-01-02 TODO: the `__initset__` and the `__keys__` methods.

struct value_nativeobj CxingImpl_s2Data_Copy(
    int argn, struct value_nativeobj args[])
{
    if( argn < 1 ) return (struct value_nativeobj){
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

    s2obj_keep(args[0].proper.p);

    return args[0];
}

struct value_nativeobj CxingImpl_s2Data_Final(
    int argn, struct value_nativeobj args[])
{
    if( argn < 1 ) return (struct value_nativeobj){
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

    s2obj_leave(args[0].proper.p);

    return (struct value_nativeobj){
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth };
}
