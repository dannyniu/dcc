/* DannyNiu/NJF, 2026-05-25. Public Domain. */

#include "langsem.h"
#include "runtime.h"
#include <SafeTypes2.h>

#define S2_OBJ_TYPE_CXING_ARRAY 0x211C
#define s2_is_cxing_array(obj)                          \
    (((s2obj_t *)obj)->type == S2_OBJ_TYPE_CXING_ARRAY)

typedef struct {
    s2obj_base;
    size_t capacity, length;
    struct value_nativeobj *ap;
} s2cxing_array_t;

typedef struct s2cxing_array_iter s2cxing_array_iter_t;

CxingMethodValueWithImpl(Array, Get);
CxingMethodValueWithImpl(Array, Set);
CxingMethodValueWithImpl(Array, Copy);
CxingMethodValueWithImpl(Array, Final);
CxingMethodValueWithImpl(Array, InitSet);
CxingMethodValueWithImpl(Array, Len);
CxingMethodValueWithImpl(Array, Trunc);
CxingMethodValueWithImpl(Array, Swap);
CxingMethodValueWithImpl(Array, Move2Head);
CxingMethodValueWithImpl(Array, Move2Tail);

const type_nativeobj_struct_p10 type_nativeobj_array = {
    .typeid = valtyp_obj,
    .n_entries = 10,
    .static_members = {
        { .name = "__get__", .member = &CxingValue_Array_Get },
        { .name = "__set__", .member = &CxingValue_Array_Set },
        { .name = "__copy__", .member = &CxingValue_Array_Copy },
        { .name = "__final__", .member = &CxingValue_Array_Final },
        { .name = "__initset__", .member = &CxingValue_Array_InitSet },
        { .name = "len", .member = &CxingValue_Array_Len },
        { .name = "trunc", .member = &CxingValue_Array_Trunc },
        { .name = "swap", .member = &CxingValue_Array_Swap },
        { .name = "move2head", .member = &CxingValue_Array_Move2Head },
        { .name = "move2tail", .member = &CxingValue_Array_Move2Tail },
    },
};

struct value_nativeobj CxingImpl_Array_Get(
    int argn, struct value_nativeobj args[])
{
    uint64_t ind;
    s2cxing_array_t *arr;

    AssertArgN(2);
    AssertArgImpl(0, array, "array");
    AssertArgImpl(1, s2impl_str, "string");

    if( s2data_len(args[1].proper.p) != sizeof ind )
    {
        CxingDebug("The array index doesn't look like an integer - it's not 64-bit.\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    arr = args[0].proper.p;

    ind = *(uint64_t *)s2data_weakmap(args[1].proper.p);
    if( ind > arr->length )
    {
        CxingTrace("Out-of-bound access to array element!\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    return arr->ap[ind];
}

struct value_nativeobj CxingImpl_Array_Set(
    int argn, struct value_nativeobj args[])
{
    uint64_t ind;
    s2cxing_array_t *arr;
    struct value_nativeobj tmp;

    AssertArgN(3);
    AssertArgImpl(0, array, "array");
    AssertArgImpl(1, s2impl_str, "string");

    if( s2data_len(args[1].proper.p) != sizeof ind )
    {
        CxingDebug("The array index doesn't look like an integer - it's not 64-bit.\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    arr = args[0].proper.p;

    ind = *(uint64_t *)s2data_weakmap(args[1].proper.p);
    if( ind > arr->length )
    {
        CxingTrace("Out-of-bound write to array element!\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    tmp = ValueCopy(args[2]);
    ValueDestroy(arr->ap[ind]);
    arr->ap[ind] = tmp;

    return arr->ap[ind];
}

struct value_nativeobj CxingImpl_Array_Copy(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, array, "array");
    return CxingImpl_s2Obj_Copy(argn, args);
}

struct value_nativeobj CxingImpl_Array_Final(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, array, "array");
    return CxingImpl_s2Obj_Final(argn, args);
}

#define ARRAY_CAPACITY_INCREMENTATION 8

struct value_nativeobj CxingImpl_Array_InitSet(
    int argn, struct value_nativeobj args[])
{
    s2cxing_array_t *arr;

    AssertArgN(3);
    AssertArgImpl(0, array, "array");

    if( args[1].type == (const void *)&type_nativeobj_s2impl_str &&
        strcmp(s2data_weakmap(args[1].proper.p), "__proto__") == 0 )
    {
        return args[0];
    }

    arr = args[0].proper.p;

    assert( arr->capacity >= arr->length );

    if( ++ arr->length > arr->capacity )
    {
        void *T = realloc(
            arr->ap, (arr->capacity + ARRAY_CAPACITY_INCREMENTATION) *
            sizeof( *arr->ap ));

        if( !T )
        {
            return (struct value_nativeobj){
                .proper.l = errno,
                .type = (const void *)&type_nativeobj_null };
        }

        arr->capacity += ARRAY_CAPACITY_INCREMENTATION;
        arr->ap = T;
    }

    return arr->ap[arr->length - 1] = ValueCopy(args[2]);
}

struct value_nativeobj CxingImpl_Array_Len(
    int argn, struct value_nativeobj args[])
{
    s2cxing_array_t *arr;

    AssertArgN(1);
    AssertArgImpl(0, array, "array");

    arr = args[0].proper.p;

    return (struct value_nativeobj){
        .proper.u = arr->length,
        .type = (const void *)&type_nativeobj_ulong };
}

struct value_nativeobj CxingImpl_Array_Trunc(
    int argn, struct value_nativeobj args[])
{
    s2cxing_array_t *arr;
    size_t oldlen, newlen, newcap, t;
    void *T;

    AssertArgN(2);
    AssertArgImpl(0, array, "array");
    if( !IsInteger(args[1]) )
    {
        CxingDebug("The first argument to `trunc` method of an array must be an integer!\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    arr = args[0].proper.p;
    oldlen = arr->length;
    newlen = args[1].proper.u;
    newcap = newlen + ARRAY_CAPACITY_INCREMENTATION - 1;
    newcap -= newcap % (ARRAY_CAPACITY_INCREMENTATION / 2);
    assert( newcap >= newlen );

    if( newlen < oldlen )
    {
        for(t=newlen; t<oldlen; t++)
        {
            ValueDestroy(arr->ap[t]);
            arr->ap[t] = (struct value_nativeobj){
                .proper.p = NULL,
                .type = (const void *)&type_nativeobj_morgoth };
        }
    }

    T = realloc(arr->ap, newcap * sizeof( *arr->ap ));
    if( !T )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    arr->ap = T;
    arr->length = newlen;

    if( newlen > oldlen )
    {
        for(t=oldlen; t<newlen; t++)
        {
            arr->ap[t] = (struct value_nativeobj){
                .proper.p = NULL,
                .type = (const void *)&type_nativeobj_morgoth };
        }
    }

    return ValueCopy(args[0]);
}

struct value_nativeobj CxingImpl_Array_Swap(
    int argn, struct value_nativeobj args[])
{
    s2cxing_array_t *arr;
    size_t a, b;
    struct value_nativeobj tmp;

    AssertArgN(3);
    AssertArgImpl(0, array, "array");
    if( !IsInteger(args[1]) || !IsInteger(args[2]) )
    {
        CxingDebug("The `swap` method of an array object expect integer arguments!\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    arr = args[0].proper.p;
    a = args[1].proper.u;
    b = args[2].proper.u;

    if( a >= arr->length || b >= arr->length )
    {
        CxingTrace("Out-of-bound access to array element!\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    tmp = arr->ap[a];
    arr->ap[a] = arr->ap[b];
    arr->ap[b] = tmp;

    return ValueCopy(args[0]);
}

struct value_nativeobj CxingImpl_Array_Move2Head(
    int argn, struct value_nativeobj args[])
{
    s2cxing_array_t *arr;
    size_t u, v;
    struct value_nativeobj tmp;

    AssertArgN(2);
    AssertArgImpl(0, array, "array");
    if( !IsInteger(args[1]) )
    {
        CxingDebug("Integer argument expected by the `move2head`!\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    arr = args[0].proper.p;
    u = args[1].proper.u;

    if( u >= arr->length )
    {
        CxingTrace("Out-of-bound access to array element!\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    tmp = arr->ap[u];
    for(v=u; v-->0; )
    {
        arr->ap[v+1] = arr->ap[v];
    }
    arr->ap[0] = tmp;

    return ValueCopy(args[0]);
}

struct value_nativeobj CxingImpl_Array_Move2Tail(
    int argn, struct value_nativeobj args[])
{
    s2cxing_array_t *arr;
    size_t u, v;
    struct value_nativeobj tmp;

    AssertArgN(2);
    AssertArgImpl(0, array, "array");
    if( !IsInteger(args[1]) )
    {
        CxingDebug("Integer argument expected by the `move2head`!\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    arr = args[0].proper.p;
    u = args[1].proper.u;

    if( u >= arr->length )
    {
        CxingTrace("Out-of-bound access to array element!\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    tmp = arr->ap[u];
    for(v=u; ++v<arr->length; )
    {
        arr->ap[v-1] = arr->ap[v];
    }
    arr->ap[arr->length-1] = tmp;

    return ValueCopy(args[0]);
}

struct s2cxing_array_iter {
    struct s2ctx_iter base;
    s2cxing_array_t *arr;
};

static int s2cxing_array_iter_step(s2cxing_array_iter_t *restrict iter)
{
    // The purpose of this iterator is not to enumerate _all_ array elements,
    // rather it's to enumerate all values that're implemented as SafeTypes2
    // bindings, so that they can be collected by the GC.
    uintptr_t pos = (uintptr_t)iter->base.key;

    if( pos >= iter->arr->length )
    {
        iter->base.value = NULL;
        return 0;
    }

    if( !iter->base.value && ValueIsBindingForSafeTypes2(iter->arr->ap[0]) )
    {
        iter->base.key = 0;
        iter->base.value = iter->arr->ap[0].proper.p;
        return 1;
    }

    while( !ValueIsBindingForSafeTypes2(iter->arr->ap[pos]) )
    {
        if( ++ pos >= iter->arr->length )
        {
            iter->base.key = (void *)pos;
            iter->base.value = NULL;
            return 0;
        }
    }

    iter->base.key = (void *)pos;
    iter->base.value = iter->arr->ap[pos].proper.p;
    return 1;
}

static s2cxing_array_iter_t *s2cxing_array_iter_create(
    s2cxing_array_t *restrict arr)
{
    s2cxing_array_iter_t *iter = NULL;

    iter = (calloc)(1, sizeof(s2cxing_array_iter_t));
    if( !iter ) return NULL;

    iter->base.final = (s2iter_final_func_t)free;
    iter->base.next = (s2iter_stepfunc_t)s2cxing_array_iter_step;
    iter->arr = arr;

    return iter;
}

static void s2cxing_array_final(s2cxing_array_t *arr)
{
    size_t t;

    if( arr->ap )
    {
        for(t=0; t<arr->length; t++)
            ValueDestroy(arr->ap[t]);

        free(arr->ap);
    }
}

struct value_nativeobj CxingImpl_Array_Create(
    int argn, struct value_nativeobj args[])
{
    s2cxing_array_t *aimpl = (void *)s2gc_obj_alloc(
        S2_OBJ_TYPE_CXING_ARRAY, sizeof(s2cxing_array_t));

    (void)argn;
    (void)args;

    if( !aimpl )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

#if INTERCEPT_MEM_CALLS
    // A pending `realloc` will receive a NULL pointer,
    // which is what we've omitted here.
    // Putting this here to pass the memory balance interceptor.
    allocs ++;
    mem_intercept(NULL);
#endif /* INTERCEPT_MEM_CALLS */

    aimpl->base.itercreatf = (s2func_iter_create_t)s2cxing_array_iter_create;
    aimpl->base.finalf = (s2func_final_t)s2cxing_array_final;

    s2obj_keep(aimpl->pobj);
    s2obj_release(aimpl->pobj);

    return (struct value_nativeobj){
        .proper.p = aimpl,
        .type = (const void *)&type_nativeobj_array };
}
