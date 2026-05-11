/* DannyNiu/NJF, 2026-05-09. Public Domain. */

#include "cxing-stdlib.h"
/*
void *t;
#define s2gc_obj_alloc(...) (t = (s2gc_obj_alloc)(__VA_ARGS__), printf("a.%s,%d: %p.\n", __func__, __LINE__, t), t)
#define s2dict_create() (t = (s2dict_create)(), printf("c.%s,%d: %p.\n", __func__, __LINE__, t), t)
#define s2obj_retain(x) (t = (s2obj_retain)(x), printf("+.%s,%d: %p.\n", __func__, __LINE__, t), t)
#define s2obj_release(x) (printf("-.%s,%d: %p.\n", __func__, __LINE__, x), (s2obj_release)(x))
#define s2obj_keep(x) (t = (s2obj_keep)(x), printf("^.%s,%d: %p.\n", __func__, __LINE__, t), t)
#define s2obj_leave(x) (printf("v.%s,%d: %p.\n", __func__, __LINE__, x), (s2obj_leave)(x))
//*/
extern struct value_nativeobj CxingPropName_Size;
extern struct value_nativeobj CxingPropName_Align;

//
// MARK: Data Array.

#define S2_OBJ_TYPE_CXING_DATA_ARRAY 0x2115
typedef struct {
    s2obj_base;
    struct value_nativeobj elemtype;
    size_t elem_sz, elem_ag, len, arr_sz;
} cxing_data_array_t;

struct value_nativeobj CxingDataArray(
    struct value_nativeobj elemtype, size_t arraylen);

// 2026-05-09: Finalizer.
void cxing_data_array_final(cxing_data_array_t *x)
{
    ValueDestroy(x->elemtype);
}

struct value_nativeobj CxingImpl_DataArrayType_Get(
    int argn, struct value_nativeobj args[])
{
    const char *k;
    uint64_t *ulk;
    AssertArgN(2);
    AssertArgImpl(0, data_array_type_obj, "data array type object");
    AssertArgImpl(1, s2impl_str, "string");

    k = s2data_weakmap(args[1].proper.p);
    ulk = (uint64_t *)k;

    if( s2data_len(args[1].proper.p) == sizeof(uint64_t) &&
        *ulk <= CXING_INT_INDEX_MAX )
    {
        return CxingDataArray(args[0], *ulk);
    }

    if( 0 == strcmp(k, "size") )
        return (struct value_nativeobj){
            .proper.u = ((cxing_data_array_t *)args[0].proper.p)->arr_sz,
            .type = (const void *)&type_nativeobj_ulong };

    if( 0 == strcmp(k, "align") )
        return (struct value_nativeobj){
            .proper.u = ((cxing_data_array_t *)args[0].proper.p)->elem_ag,
            .type = (const void *)&type_nativeobj_ulong };

    return (struct value_nativeobj){
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth };
}

struct value_nativeobj CxingValue_DataArrayType_Get = {
    .proper.p = CxingImpl_DataArrayType_Get,
    .type = (const void *)&type_nativeobj_method };

const type_nativeobj_struct_p3 type_nativeobj_data_array_type_obj = {
    .typeid = valtyp_obj,
    .n_entries = 3,
    .static_members = {
        { .name = "__get__", .member = &CxingValue_DataArrayType_Get },
        { .name = "__copy__", .member = &CxingValue_s2Obj_Copy },
        { .name = "__final__", .member = &CxingValue_s2Obj_Final },
    },
};

struct value_nativeobj CxingDataArray(
    struct value_nativeobj elemtype, size_t arraylen)
{
    cxing_data_array_t *ret;
    struct value_nativeobj prop_sz, prop_ag;
    size_t elem_sz, elem_ag;

    ret = (cxing_data_array_t *)s2gc_obj_alloc(
        S2_OBJ_TYPE_CXING_DATA_ARRAY, sizeof *ret);

    if( !ret )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    s2obj_keep(ret->pobj);
    s2obj_release(ret->pobj);

    prop_sz = GetValProperty(elemtype, CxingPropName_Size).value;
    prop_ag = GetValProperty(elemtype, CxingPropName_Align).value;

    if( !IsInteger(prop_sz) || !IsInteger(prop_ag) )
    {
        s2obj_leave(ret->pobj);
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    ret->base.finalf = (s2func_final_t)cxing_data_array_final;
    ret->elemtype = ValueCopy(elemtype);

    ret->elem_sz = elem_sz = prop_sz.proper.u;
    ret->elem_ag = elem_ag = prop_ag.proper.u;

    ret->len = arraylen;
    elem_sz += elem_ag - 1;
    elem_sz -= elem_sz % elem_ag;
    ret->arr_sz = elem_sz * arraylen;

    return (struct value_nativeobj){
        .proper.p = ret,
        .type = (const void *)&type_nativeobj_data_array_type_obj };
}

//
// MARK: Primitive Types.

struct value_nativeobj CxingImpl_PrimitiveType_Get(
    int argn, struct value_nativeobj args[])
{
    const char *k;
    uint64_t *ulk;

    (void)argn;

    k = s2data_weakmap(args[1].proper.p);
    ulk = (uint64_t *)k;

    if( s2data_len(args[1].proper.p) == sizeof(uint64_t) &&
        *ulk <= CXING_INT_INDEX_MAX )
    {
        return CxingDataArray(args[0], *ulk);
    }

    if( 0 == strcmp(k, "size") ||
        0 == strcmp(k, "align") )
        return (struct value_nativeobj){
            .proper.u = args[0].proper.u,
            .type = (const void *)&type_nativeobj_ulong };

    return (struct value_nativeobj){
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth };
}

struct value_nativeobj CxingImpl_SignedType_Get(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(2);
    AssertArgImpl(0, signed_type_obj, "signed integer type object");
    AssertArgImpl(1, s2impl_str, "string");
    return CxingImpl_PrimitiveType_Get(argn, args);
}

struct value_nativeobj CxingImpl_UnsignedType_Get(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(2);
    AssertArgImpl(0, unsigned_type_obj, "unsigned integer type object");
    AssertArgImpl(1, s2impl_str, "string");
    return CxingImpl_PrimitiveType_Get(argn, args);
}

struct value_nativeobj CxingImpl_FloatingPointType_Get(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(2);
    AssertArgImpl(0, floatingpoint_type_obj, "floating point type object");
    AssertArgImpl(1, s2impl_str, "string");
    return CxingImpl_PrimitiveType_Get(argn, args);
}

struct value_nativeobj CxingValue_SignedType_Get = {
    .proper.p = CxingImpl_SignedType_Get,
    .type = (const void *)&type_nativeobj_method };

struct value_nativeobj CxingValue_UnsignedType_Get = {
    .proper.p = CxingImpl_UnsignedType_Get,
    .type = (const void *)&type_nativeobj_method };

struct value_nativeobj CxingValue_FloatingPointType_Get = {
    .proper.p = CxingImpl_FloatingPointType_Get,
    .type = (const void *)&type_nativeobj_method };

const type_nativeobj_struct_p1 type_nativeobj_signed_type_obj = {
    .typeid = valtyp_obj,
    .n_entries = 1,
    .static_members = {
        { .name = "__get__", .member = &CxingValue_SignedType_Get },
    },
};

const type_nativeobj_struct_p1 type_nativeobj_unsigned_type_obj = {
    .typeid = valtyp_obj,
    .n_entries = 1,
    .static_members = {
        { .name = "__get__", .member = &CxingValue_UnsignedType_Get },
    },
};

const type_nativeobj_struct_p1 type_nativeobj_floatingpoint_type_obj = {
    .typeid = valtyp_obj,
    .n_entries = 1,
    .static_members = {
        { .name = "__get__", .member = &CxingValue_FloatingPointType_Get },
    },
};

//
// MARK: Aggregates.

#define S2_OBJ_TYPE_AGGREGATE_MEMBER 0x2116
typedef struct {
    s2obj_base;
    struct value_nativeobj member_type;
    size_t member_offset;
} cxing_aggregate_member_t;

#define S2_OBJ_TYPE_AGGREGATE_PACK 0x2117
#define S2_OBJ_TYPE_AGGREGATE_UNION 0x2118
#define S2_OBJ_TYPE_AGGREGATE_STRUCT 0x2119
typedef struct {
    s2obj_base;
    s2dict_t *members;
    size_t next_offset;
    size_t total_size;
    size_t alignment;
} cxing_aggregate_t,
    cxing_aggregate_pack_t,
    cxing_aggregate_union_t,
    cxing_aggregate_struct_t;

void cxing_aggregate_member_final(cxing_aggregate_member_t *x)
{
    ValueDestroy(x->member_type);
}

void cxing_aggregate_final(cxing_aggregate_t *x)
{
    s2obj_release(x->members->pobj);
}

struct value_nativeobj CxingImpl_AggrPack_InitSet0(
    int argn, struct value_nativeobj args[])
{
    struct value_nativeobj prop_sz;
    size_t elem_sz;
    cxing_aggregate_pack_t *aggr;
    cxing_aggregate_member_t *entry;

    (void)argn;

    prop_sz = GetValProperty(args[2], CxingPropName_Size).value;
    if( !IsInteger(prop_sz) )
    {
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }
    elem_sz = prop_sz.proper.u;

    entry = (cxing_aggregate_member_t *)s2gc_obj_alloc(
        S2_OBJ_TYPE_AGGREGATE_MEMBER, sizeof *entry);

    if( !entry )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    entry->base.finalf = (s2func_final_t)cxing_aggregate_member_final;
    entry->member_type = ValueCopy(args[2]);
    aggr = args[0].proper.p;

    if( s2_access_success != s2dict_set(
            aggr->members, args[1].proper.p, entry->pobj, s2_setter_gave) )
    {
        s2obj_release(entry->pobj);
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    entry->member_offset = aggr->next_offset;
    aggr->next_offset += elem_sz;
    aggr->total_size += elem_sz;
    aggr->alignment = 1;

    return args[1];
}

struct value_nativeobj CxingImpl_AggrUnion_InitSet0(
    int argn, struct value_nativeobj args[])
{
    struct value_nativeobj prop_sz, prop_ag;
    size_t elem_sz, elem_ag;
    cxing_aggregate_pack_t *aggr;
    cxing_aggregate_member_t *entry;

    (void)argn;

    prop_sz = GetValProperty(args[2], CxingPropName_Size).value;
    prop_ag = GetValProperty(args[2], CxingPropName_Align).value;

    if( !IsInteger(prop_sz) || !IsInteger(prop_ag) )
    {
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    elem_sz = prop_sz.proper.u;
    elem_ag = prop_ag.proper.u;

    entry = (cxing_aggregate_member_t *)s2gc_obj_alloc(
        S2_OBJ_TYPE_AGGREGATE_MEMBER, sizeof *entry);

    if( !entry )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    entry->base.finalf = (s2func_final_t)cxing_aggregate_member_final;
    entry->member_type = ValueCopy(args[2]);
    aggr = args[0].proper.p;

    if( s2_access_success != s2dict_set(
            aggr->members, args[1].proper.p, entry->pobj, s2_setter_gave) )
    {
        s2obj_release(entry->pobj);
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    entry->member_offset = 0;
    aggr->next_offset = 0;
    if( aggr->total_size < elem_sz )
        aggr->total_size = elem_sz;
    if( aggr->alignment < elem_ag )
        aggr->alignment = elem_ag;

    return args[1];
}

struct value_nativeobj CxingImpl_AggrStruct_InitSet0(
    int argn, struct value_nativeobj args[])
{
    struct value_nativeobj prop_sz, prop_ag;
    size_t elem_sz, elem_ag, elem_offset;
    cxing_aggregate_pack_t *aggr;
    cxing_aggregate_member_t *entry;

    (void)argn;

    prop_sz = GetValProperty(args[2], CxingPropName_Size).value;
    prop_ag = GetValProperty(args[2], CxingPropName_Align).value;

    if( !IsInteger(prop_sz) || !IsInteger(prop_ag) )
    {
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    elem_sz = prop_sz.proper.u;
    elem_ag = prop_ag.proper.u;

    entry = (cxing_aggregate_member_t *)s2gc_obj_alloc(
        S2_OBJ_TYPE_AGGREGATE_MEMBER, sizeof *entry);

    if( !entry )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    entry->base.finalf = (s2func_final_t)cxing_aggregate_member_final;
    entry->member_type = ValueCopy(args[2]);
    aggr = args[0].proper.p;

    if( s2_access_success != s2dict_set(
            aggr->members, args[1].proper.p, entry->pobj, s2_setter_gave) )
    {
        s2obj_release(entry->pobj);
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    elem_offset = aggr->next_offset;
    elem_offset += elem_ag - 1;
    elem_offset -= elem_offset % elem_ag;

    entry->member_offset = elem_offset;
    aggr->total_size = aggr->next_offset = elem_offset + elem_sz;
    if( aggr->alignment < elem_ag )
        aggr->alignment = elem_ag;

    return args[1];
}

struct value_nativeobj CxingImpl_AggrPack_InitSet(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(3);
    AssertArgImpl(0, aggr_pack_type_obj, "pack aggregate");
    AssertArgImpl(1, s2impl_str, "string");
    AssertArgImpls(
        2,
        AcceptArgImpl(2, data_array_type_obj)
        AcceptArgImpl(2, aggr_pack_type_obj)
        AcceptArgImpl(2, aggr_union_type_obj)
        AcceptArgImpl(2, aggr_struct_type_obj)
        AcceptArgImpl(2, signed_type_obj)
        AcceptArgImpl(2, unsigned_type_obj)
        AcceptArgImpl(2, floatingpoint_type_obj),
        "type obj");

    if( 0 == strcmp("__proto__", s2data_weakmap(args[1].proper.p)) )
        return args[2];

    return CxingImpl_AggrPack_InitSet0(argn, args);
}

struct value_nativeobj CxingImpl_AggrUnion_InitSet(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(3);
    AssertArgImpl(0, aggr_union_type_obj, "union aggregate");
    AssertArgImpl(1, s2impl_str, "string");
    AssertArgImpls(
        2,
        AcceptArgImpl(2, data_array_type_obj)
        AcceptArgImpl(2, aggr_pack_type_obj)
        AcceptArgImpl(2, aggr_union_type_obj)
        AcceptArgImpl(2, aggr_struct_type_obj)
        AcceptArgImpl(2, signed_type_obj)
        AcceptArgImpl(2, unsigned_type_obj)
        AcceptArgImpl(2, floatingpoint_type_obj),
        "type obj");

    if( 0 == strcmp("__proto__", s2data_weakmap(args[1].proper.p)) )
        return args[2];

    return CxingImpl_AggrUnion_InitSet0(argn, args);
}

struct value_nativeobj CxingImpl_AggrStruct_InitSet(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(3);
    AssertArgImpl(0, aggr_struct_type_obj, "struct aggregate");
    AssertArgImpl(1, s2impl_str, "string");
    AssertArgImpls(
        2,
        AcceptArgImpl(2, data_array_type_obj)
        AcceptArgImpl(2, aggr_pack_type_obj)
        AcceptArgImpl(2, aggr_union_type_obj)
        AcceptArgImpl(2, aggr_struct_type_obj)
        AcceptArgImpl(2, signed_type_obj)
        AcceptArgImpl(2, unsigned_type_obj)
        AcceptArgImpl(2, floatingpoint_type_obj),
        "type obj");

    if( 0 == strcmp("__proto__", s2data_weakmap(args[1].proper.p)) )
        return args[2];

    return CxingImpl_AggrStruct_InitSet0(argn, args);
}

struct value_nativeobj CxingImpl_AggrTypes_Get(
    int argn, struct value_nativeobj args[])
{
    const char *k;
    uint64_t *ulk;

    (void)argn;

    k = s2data_weakmap(args[1].proper.p);
    ulk = (uint64_t *)k;

    if( s2data_len(args[1].proper.p) == sizeof(uint64_t) &&
        *ulk <= CXING_INT_INDEX_MAX )
    {
        return CxingDataArray(args[0], *ulk);
    }

    if( 0 == strcmp(k, "size") )
    {
        return (struct value_nativeobj){
            .proper.u = ((cxing_aggregate_t *)args[0].proper.p)->total_size,
            .type = (const void *)&type_nativeobj_ulong };
    }

    if( 0 == strcmp(k, "align") )
    {
        return (struct value_nativeobj){
            .proper.u = ((cxing_aggregate_t *)args[0].proper.p)->alignment,
            .type = (const void *)&type_nativeobj_ulong };
    }

    return (struct value_nativeobj){
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth };
}

struct value_nativeobj CxingValue_AggrPack_InitSet = {
    .proper.p = CxingImpl_AggrPack_InitSet,
    .type = (const void *)&type_nativeobj_method };

struct value_nativeobj CxingValue_AggrUnion_InitSet = {
    .proper.p = CxingImpl_AggrUnion_InitSet,
    .type = (const void *)&type_nativeobj_method };

struct value_nativeobj CxingValue_AggrStruct_InitSet = {
    .proper.p = CxingImpl_AggrStruct_InitSet,
    .type = (const void *)&type_nativeobj_method };

struct value_nativeobj CxingValue_AggrTypes_Get = {
    .proper.p = CxingImpl_AggrTypes_Get,
    .type = (const void *)&type_nativeobj_method };

const type_nativeobj_struct_p4 type_nativeobj_aggr_pack_type_obj = {
    .typeid = valtyp_obj,
    .n_entries = 4,
    .static_members = {
        { .name = "__get__", .member = &CxingValue_AggrTypes_Get },
        { .name = "__initset__", .member = &CxingValue_AggrPack_InitSet },
        { .name = "__copy__", .member = &CxingValue_s2Obj_Copy },
        { .name = "__final__", .member = &CxingValue_s2Obj_Final },
    },
};

const type_nativeobj_struct_p4 type_nativeobj_aggr_union_type_obj = {
    .typeid = valtyp_obj,
    .n_entries = 4,
    .static_members = {
        { .name = "__get__", .member = &CxingValue_AggrTypes_Get },
        { .name = "__initset__", .member = &CxingValue_AggrUnion_InitSet },
        { .name = "__copy__", .member = &CxingValue_s2Obj_Copy },
        { .name = "__final__", .member = &CxingValue_s2Obj_Final },
    },
};

const type_nativeobj_struct_p4 type_nativeobj_aggr_struct_type_obj = {
    .typeid = valtyp_obj,
    .n_entries = 4,
    .static_members = {
        { .name = "__get__", .member = &CxingValue_AggrTypes_Get },
        { .name = "__initset__", .member = &CxingValue_AggrStruct_InitSet },
        { .name = "__copy__", .member = &CxingValue_s2Obj_Copy },
        { .name = "__final__", .member = &CxingValue_s2Obj_Final },
    },
};

struct value_nativeobj CxingStdlibFunc_Packed(
    int argn, struct value_nativeobj args[])
{
    cxing_aggregate_pack_t *ret;
    (void)argn;
    (void)args;

    if( !(ret = (cxing_aggregate_pack_t *)s2gc_obj_alloc(
              S2_OBJ_TYPE_AGGREGATE_PACK, sizeof *ret)) )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    s2obj_keep(ret->pobj);
    s2obj_release(ret->pobj);

    if( !(ret->members = s2dict_create()) )
    {
        s2obj_leave(ret->pobj);
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    ret->base.finalf = (s2func_final_t)cxing_aggregate_final;
    return (struct value_nativeobj){
        .proper.p = ret,
        .type = (const void *)&type_nativeobj_aggr_pack_type_obj };
}

struct value_nativeobj CxingStdlibFunc_Union(
    int argn, struct value_nativeobj args[])
{
    cxing_aggregate_union_t *ret;
    (void)argn;
    (void)args;

    if( !(ret = (cxing_aggregate_union_t *)s2gc_obj_alloc(
              S2_OBJ_TYPE_AGGREGATE_UNION, sizeof *ret)) )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    s2obj_keep(ret->pobj);
    s2obj_release(ret->pobj);

    if( !(ret->members = s2dict_create()) )
    {
        s2obj_leave(ret->pobj);
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    ret->base.finalf = (s2func_final_t)cxing_aggregate_final;
    return (struct value_nativeobj){
        .proper.p = ret,
        .type = (const void *)&type_nativeobj_aggr_union_type_obj };
}

struct value_nativeobj CxingStdlibFunc_Struct(
    int argn, struct value_nativeobj args[])
{
    cxing_aggregate_struct_t *ret;
    (void)argn;
    (void)args;

    if( !(ret = (cxing_aggregate_struct_t *)s2gc_obj_alloc(
              S2_OBJ_TYPE_AGGREGATE_STRUCT, sizeof *ret)) )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    s2obj_keep(ret->pobj);
    s2obj_release(ret->pobj);

    if( !(ret->members = s2dict_create()) )
    {
        s2obj_leave(ret->pobj);
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    ret->base.finalf = (s2func_final_t)cxing_aggregate_final;
    return (struct value_nativeobj){
        .proper.p = ret,
        .type = (const void *)&type_nativeobj_aggr_struct_type_obj };
}

cxing_builtin_def_t CxingStdlibStructBuiltins[] = {
    { "char", (struct value_nativeobj){
            .proper.u = 1,
            .type = (const void *)&type_nativeobj_signed_type_obj } },
    { "short", (struct value_nativeobj){
            .proper.u = 2,
            .type = (const void *)&type_nativeobj_signed_type_obj } },
    { "int", (struct value_nativeobj){
            .proper.u = 4,
            .type = (const void *)&type_nativeobj_signed_type_obj } },
    { "long", (struct value_nativeobj){
            .proper.u = 8,
            .type = (const void *)&type_nativeobj_signed_type_obj } },

    { "byte", (struct value_nativeobj){
            .proper.u = 1,
            .type = (const void *)&type_nativeobj_unsigned_type_obj } },
    { "ushort", (struct value_nativeobj){
            .proper.u = 2,
            .type = (const void *)&type_nativeobj_unsigned_type_obj } },
    { "uint", (struct value_nativeobj){
            .proper.u = 4,
            .type = (const void *)&type_nativeobj_unsigned_type_obj } },
    { "ulong", (struct value_nativeobj){
            .proper.u = 8,
            .type = (const void *)&type_nativeobj_unsigned_type_obj } },

    { "float", (struct value_nativeobj){
            .proper.u = 4,
            .type = (const void *)&type_nativeobj_floatingpoint_type_obj } },
    { "double", (struct value_nativeobj){
            .proper.u = 8,
            .type = (const void *)&type_nativeobj_floatingpoint_type_obj } },

    { "struct", (struct value_nativeobj){
            .proper.p = CxingStdlibFunc_Struct,
            .type = (const void *)&type_nativeobj_subr } },

    { "packed", (struct value_nativeobj){
            .proper.p = CxingStdlibFunc_Packed,
            .type = (const void *)&type_nativeobj_subr } },

    { "union", (struct value_nativeobj){
            .proper.p = CxingStdlibFunc_Union,
            .type = (const void *)&type_nativeobj_subr } },

    {},
};

//
// MARK: Mapping bindings between data and structures.

#define S2_OBJ_TYPE_DATA_STRUCT_MAP 0x211A
typedef struct {
    s2obj_base;
    s2data_t *src;
    size_t offset;
    union {
        cxing_aggregate_t *aggr;
        cxing_data_array_t *arr;
    } bound_type;
    s2dict_t *bindings;
} cxing_data_struct_map_t;

void cxing_data_struct_map_final(cxing_data_struct_map_t *x)
{
    s2data_unmap(x->src);
    s2obj_release(x->src->pobj);
    s2obj_release(x->bound_type.aggr->pobj);
    s2obj_release(x->bindings->pobj);
}

struct value_nativeobj CxingImpl_DataStructMap_AccessUnwrap(
    s2data_t *src, size_t offset_base, struct value_nativeobj type_obj)
{
    if( type_obj.type == (const void *)&type_nativeobj_floatingpoint_type_obj )
    {
        double ret;
        if( type_obj.proper.u == 4 )
            ret = *(float *)s2data_map(src, offset_base, 4);
        else if( type_obj.proper.u == 8 )
            ret = *(double *)s2data_map(src, offset_base, 8);
        else assert( 0 );

        s2data_unmap(src);
        return (struct value_nativeobj){
            .proper.f = ret,
            .type = (const void *)&type_nativeobj_double };
    }
    else if( type_obj.type == (const void *)&type_nativeobj_signed_type_obj )
    {
        int64_t ret;
        if( type_obj.proper.u == 1 )
            ret = *(int8_t *)s2data_map(src, offset_base, 1);
        else if( type_obj.proper.u == 2 )
            ret = *(int16_t *)s2data_map(src, offset_base, 2);
        else if( type_obj.proper.u == 4 )
            ret = *(int32_t *)s2data_map(src, offset_base, 4);
        else if( type_obj.proper.u == 8 )
            ret = *(int64_t *)s2data_map(src, offset_base, 8);
        else assert( 0 );

        s2data_unmap(src);
        return (struct value_nativeobj){
            .proper.l = ret,
            .type = (const void *)&type_nativeobj_long };
    }
    else if( type_obj.type == (const void *)&type_nativeobj_unsigned_type_obj )
    {
        uint64_t ret;
        if( type_obj.proper.u == 1 )
            ret = *(uint8_t *)s2data_map(src, offset_base, 1);
        else if( type_obj.proper.u == 2 )
            ret = *(uint16_t *)s2data_map(src, offset_base, 2);
        else if( type_obj.proper.u == 4 )
            ret = *(uint32_t *)s2data_map(src, offset_base, 4);
        else if( type_obj.proper.u == 8 )
            ret = *(uint64_t *)s2data_map(src, offset_base, 8);
        else assert( 0 );

        s2data_unmap(src);
        return (struct value_nativeobj){
            .proper.u = ret,
            .type = (const void *)&type_nativeobj_ulong };
    }
    else if( type_obj.type == (const void *)&type_nativeobj_aggr_pack_type_obj ||
             type_obj.type == (const void *)&type_nativeobj_aggr_union_type_obj ||
             type_obj.type == (const void *)&type_nativeobj_aggr_struct_type_obj ||
             type_obj.type == (const void *)&type_nativeobj_data_array_type_obj )
    {
        cxing_data_struct_map_t *map;
        size_t mapsz = GetValProperty(
            type_obj, CxingPropName_Size).value.proper.u;

        map = (cxing_data_struct_map_t *)s2gc_obj_alloc(
            S2_OBJ_TYPE_DATA_STRUCT_MAP, sizeof *map);

        if( !map )
        {
            return (struct value_nativeobj){
                .proper.p = NULL,
                .type = (const void *)&type_nativeobj_morgoth };
        }

        s2obj_keep(map->pobj);
        s2obj_release(map->pobj);

        map->bindings = s2dict_create();
        if( !map->bindings )
        {
            s2obj_leave(map->pobj);
            return (struct value_nativeobj){
                .proper.l = errno,
                .type = (const void *)&type_nativeobj_null };
        }

        s2data_map(src, offset_base, mapsz);
        map->base.finalf = (s2func_final_t)cxing_data_struct_map_final;
        map->src = (s2data_t *)s2obj_retain(src->pobj);
        map->offset = offset_base;
        map->bound_type.aggr = (cxing_aggregate_t *)s2obj_retain(type_obj.proper.p);
        return (struct value_nativeobj){
            .proper.p = map,
            .type = (const void *)&type_nativeobj_data_struct_map_obj };
    }
    else
    {
        printf("%llx %p\n", type_obj.proper.u, type_obj.type);
        assert( 0 ); // ought to fail more gracefully in cxing.
    }
}

struct value_nativeobj CxingImpl_DataStructMap_MemberAccess(
    int argn, struct value_nativeobj args[])
{
    cxing_data_struct_map_t *self;
    struct value_nativeobj type_obj, map_obj;

    AssertArgN(2);
    AssertArgImpl(0, data_struct_map_obj, "data structure mapping");
    AssertArgImpl(1, s2impl_str, "string");

    self = args[0].proper.p;

    if( self->bound_type.arr->base.type == S2_OBJ_TYPE_CXING_DATA_ARRAY )
    {
        uint64_t *ulk = (uint64_t *)s2data_weakmap(args[1].proper.p);
        int subret;

        if( s2data_len(args[1].proper.p) != sizeof(uint64_t) ||
            *ulk > CXING_INT_INDEX_MAX )
            return (struct value_nativeobj){
                .proper.p = NULL,
                .type = (const void *)&type_nativeobj_morgoth };

        if( *ulk >= self->bound_type.arr->len )
            // out-of-bound access, it can happen.
            return (struct value_nativeobj){
                .proper.p = NULL,
                .type = (const void *)&type_nativeobj_morgoth };

        subret = s2dict_get_T(void)( // `cxing_data_struct_map_t` actually.
            self->bindings, args[1].proper.p, &map_obj.proper.p);

        if( subret == s2_access_success )
        {
            map_obj.type = (const void *)&type_nativeobj_data_struct_map_obj;
            return map_obj;
        }
        else if( subret == s2_access_error )
        {
            return (struct value_nativeobj){
                .proper.p = NULL,
                .type = (const void *)&type_nativeobj_morgoth };
        }
        else assert( subret == s2_access_nullval );

        type_obj = self->bound_type.arr->elemtype;
        map_obj = CxingImpl_DataStructMap_AccessUnwrap(
            self->src, self->offset +
            *ulk * self->bound_type.arr->elem_sz, type_obj);
        
        if( map_obj.type == (const void *)&type_nativeobj_data_struct_map_obj )
        {
            subret = s2dict_set(
                self->bindings, args[1].proper.p,
                map_obj.proper.p, s2_setter_kept); // would've been `gave`, ...

            if( !subret )
            {
                subret = errno;
                ValueDestroy(map_obj);
                return (struct value_nativeobj){
                    .proper.l = subret,
                    .type = (const void *)&type_nativeobj_null };
            }

            // ... but in CXING runtime, it had been created
            // with an initial `keptcnt`, so this here is a `leave.
            s2obj_leave(map_obj.proper.p);
        }

        return map_obj;
    }
    else if( self->bound_type.aggr->base.type == S2_OBJ_TYPE_AGGREGATE_PACK ||
             self->bound_type.aggr->base.type == S2_OBJ_TYPE_AGGREGATE_UNION ||
             self->bound_type.aggr->base.type == S2_OBJ_TYPE_AGGREGATE_STRUCT )
    {
        cxing_aggregate_member_t *member_info;
        int subret;

        subret = s2dict_get_T(void)( // `cxing_data_struct_map_t` actually.
            self->bindings, args[1].proper.p, &map_obj.proper.p);

        if( subret == s2_access_success )
        {
            map_obj.type = (const void *)&type_nativeobj_data_struct_map_obj;
            return map_obj;
        }
        else if( subret == s2_access_error )
        {
            return (struct value_nativeobj){
                .proper.p = NULL,
                .type = (const void *)&type_nativeobj_morgoth };
        }
        else assert( subret == s2_access_nullval );

        subret = s2dict_get_T(cxing_aggregate_member_t)(
            self->bound_type.aggr->members, args[1].proper.p, &member_info);

        if( subret != s2_access_success )
        {
            return (struct value_nativeobj){
                .proper.p = NULL,
                .type = (const void *)&type_nativeobj_morgoth };
        }

        type_obj = member_info->member_type;
        map_obj = CxingImpl_DataStructMap_AccessUnwrap(
            self->src, self->offset +
            member_info->member_offset, type_obj);

        if( map_obj.type == (const void *)&type_nativeobj_data_struct_map_obj )
        {
            subret = s2dict_set(
                self->bindings, args[1].proper.p,
                map_obj.proper.p, s2_setter_kept); // would've been `gave`, ...

            if( !subret )
            {
                subret = errno;
                ValueDestroy(map_obj);
                return (struct value_nativeobj){
                    .proper.l = subret,
                    .type = (const void *)&type_nativeobj_null };
            }

            // ... but in CXING runtime, it had been created
            // with an initial `keptcnt`, so this here is a `leave`.
            s2obj_leave(map_obj.proper.p);
        }

        return map_obj;
    }
    else assert( 0 );
}

struct value_nativeobj CxingImpl_DataStructMap_AccessWrite(
    struct value_nativeobj loperand,
    s2data_t *src, size_t offset_base,
    struct value_nativeobj type_obj,
    struct value_nativeobj rvalue)
{
    if( type_obj.type == (const void *)&type_nativeobj_floatingpoint_type_obj )
    {
        double ret = ConvertToDouble(rvalue).proper.f;
        if( type_obj.proper.u == 4 )
            *(float *)s2data_map(src, offset_base, 4) = ret;
        else if( type_obj.proper.u == 8 )
            *(double *)s2data_map(src, offset_base, 8) = ret;
        else assert( 0 );

        s2data_unmap(src);
        return (struct value_nativeobj){
            .proper.f = ret,
            .type = (const void *)&type_nativeobj_double };
    }
    else if( type_obj.type == (const void *)&type_nativeobj_signed_type_obj )
    {
        int64_t ret = ConvertToLong(rvalue).proper.l;
        if( type_obj.proper.u == 1 )
            *(int8_t *)s2data_map(src, offset_base, 1) = ret;
        else if( type_obj.proper.u == 2 )
            *(int16_t *)s2data_map(src, offset_base, 2) = ret;
        else if( type_obj.proper.u == 4 )
            *(int32_t *)s2data_map(src, offset_base, 4) = ret;
        else if( type_obj.proper.u == 8 )
            *(int64_t *)s2data_map(src, offset_base, 8) = ret;
        else assert( 0 );

        s2data_unmap(src);
        return (struct value_nativeobj){
            .proper.l = ret,
            .type = (const void *)&type_nativeobj_long };
    }
    else if( type_obj.type == (const void *)&type_nativeobj_unsigned_type_obj )
    {
        uint64_t ret = ConvertToUlong(rvalue).proper.u;
        if( type_obj.proper.u == 1 )
            *(uint8_t *)s2data_map(src, offset_base, 1) = ret;
        else if( type_obj.proper.u == 2 )
            *(uint16_t *)s2data_map(src, offset_base, 2) = ret;
        else if( type_obj.proper.u == 4 )
            *(uint32_t *)s2data_map(src, offset_base, 4) = ret;
        else if( type_obj.proper.u == 8 )
            *(uint64_t *)s2data_map(src, offset_base, 8) = ret;
        else assert( 0 );

        s2data_unmap(src);
        return (struct value_nativeobj){
            .proper.u = ret,
            .type = (const void *)&type_nativeobj_ulong };
    }
    else if( type_obj.type == (const void *)&type_nativeobj_aggr_pack_type_obj ||
             type_obj.type == (const void *)&type_nativeobj_aggr_union_type_obj ||
             type_obj.type == (const void *)&type_nativeobj_aggr_struct_type_obj ||
             type_obj.type == (const void *)&type_nativeobj_data_array_type_obj )
    {
        cxing_data_struct_map_t *roperand;
        cxing_aggregate_t *typeof_left, *typeof_right;
        size_t objsz;

        if( rvalue.type != (const void *)&type_nativeobj_data_struct_map_obj )
        {
            return (struct value_nativeobj){
                .proper.p = NULL,
                .type = (const void *)&type_nativeobj_morgoth };
        }

        typeof_left = type_obj.proper.p;
        roperand = rvalue.proper.p;
        typeof_right = roperand->bound_type.aggr;

        if( typeof_right != typeof_left )
        {
            return (struct value_nativeobj){
                .proper.p = NULL,
                .type = (const void *)&type_nativeobj_morgoth };
        }

        objsz = GetValProperty(type_obj, CxingPropName_Size).value.proper.u;

        memmove(s2data_map(src, offset_base, objsz),
                s2data_map(roperand->src, roperand->offset, objsz), objsz);
        s2data_unmap(src);
        s2data_unmap(roperand->src);

        return ValueCopy(loperand);
    }
    else assert( 0 ); // ought to fail more gracefully in cxing.
}

struct value_nativeobj CxingImpl_DataStructMap_MemberModify(
    int argn, struct value_nativeobj args[])
{
    cxing_data_struct_map_t *self;
    struct value_nativeobj type_obj;

    AssertArgN(3);
    AssertArgImpl(0, data_struct_map_obj, "data structure mapping");
    AssertArgImpl(1, s2impl_str, "string");

    self = args[0].proper.p;

    if( self->bound_type.arr->base.type == S2_OBJ_TYPE_CXING_DATA_ARRAY )
    {
        // len may be less than 8, but since we're
        // not (yet) dereferencing the pointer,
        // it'll be safe for now.
        uint64_t *ulk = (uint64_t *)s2data_weakmap(args[1].proper.p);

        if( s2data_len(args[1].proper.p) != sizeof(uint64_t) ||
            *ulk > CXING_INT_INDEX_MAX )
            return (struct value_nativeobj){
                .proper.p = NULL,
                .type = (const void *)&type_nativeobj_morgoth };

        if( *ulk >= self->bound_type.arr->len )
            // out-of-bound access, it can happen.
            return (struct value_nativeobj){
                .proper.p = NULL,
                .type = (const void *)&type_nativeobj_morgoth };

        type_obj = self->bound_type.arr->elemtype;
        return CxingImpl_DataStructMap_AccessWrite(
            args[0], self->src, self->offset +
            *ulk * self->bound_type.arr->elem_sz,
            type_obj, args[2]);
    }
    else if( self->bound_type.aggr->base.type == S2_OBJ_TYPE_AGGREGATE_PACK ||
             self->bound_type.aggr->base.type == S2_OBJ_TYPE_AGGREGATE_UNION ||
             self->bound_type.aggr->base.type == S2_OBJ_TYPE_AGGREGATE_STRUCT )
    {
        cxing_aggregate_member_t *member_info;
        int subret;

        subret = s2dict_get_T(cxing_aggregate_member_t)(
            self->bound_type.aggr->members, args[1].proper.p, &member_info);

        if( subret != s2_access_success )
        {
            return (struct value_nativeobj){
                .proper.p = NULL,
                .type = (const void *)&type_nativeobj_morgoth };
        }

        type_obj = member_info->member_type;
        return CxingImpl_DataStructMap_AccessWrite(
            args[0], self->src, self->offset +
            member_info->member_offset,
            type_obj, args[2]);
    }
    else assert( 0 );
}

struct value_nativeobj CxingValue_DataStructMap_Get = {
    .proper.p = CxingImpl_DataStructMap_MemberAccess,
    .type = (const void *)&type_nativeobj_method };

struct value_nativeobj CxingValue_DataStructMap_Set = {
    .proper.p = CxingImpl_DataStructMap_MemberModify,
    .type = (const void *)&type_nativeobj_method };

const type_nativeobj_struct_p4 type_nativeobj_data_struct_map_obj = {
    .typeid = valtyp_obj,
    .n_entries = 4,
    .static_members = {
        { .name = "__get__", .member = (const void *)&CxingValue_DataStructMap_Get },
        { .name = "__set__", .member = (const void *)&CxingValue_DataStructMap_Set },
        { .name = "__copy__", .member = (const void *)&CxingValue_s2Obj_Copy },
        { .name = "__final__", .member = (const void *)&CxingValue_s2Obj_Final },
    },
};

struct value_nativeobj CxingImpl_s2Data_Map(
    int argn, struct value_nativeobj args[])
{
    cxing_data_struct_map_t *map;
    size_t mapsz;

    AssertArgN(2);
    AssertArgImpl(0, s2impl_str, "string");
    AssertArgImpls(
        1,
        AcceptArgImpl(1, data_array_type_obj)
        AcceptArgImpl(1, aggr_pack_type_obj)
        AcceptArgImpl(1, aggr_union_type_obj)
        AcceptArgImpl(1, aggr_struct_type_obj),
        "type obj");

    mapsz = GetValProperty(args[1], CxingPropName_Size).value.proper.u;
    if( mapsz > s2data_len(args[0].proper.p) )
    {
        CxingDebug("The string object is too short "
                   "for mapping with the type object.\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    map = (cxing_data_struct_map_t *)s2gc_obj_alloc(
        S2_OBJ_TYPE_DATA_STRUCT_MAP, sizeof *map);

    s2obj_keep(map->pobj);
    s2obj_release(map->pobj);

    if( !map )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    map->bindings = s2dict_create();
    if( !map->bindings )
    {
        s2obj_leave(map->pobj);
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    s2data_map(args[0].proper.p, 0, mapsz);
    map->base.finalf = (s2func_final_t)cxing_data_struct_map_final;
    map->src = (s2data_t *)s2obj_retain(args[0].proper.p);
    map->offset = 0;
    map->bound_type.aggr = (cxing_aggregate_t *)s2obj_retain(args[1].proper.p);
    return (struct value_nativeobj){
        .proper.p = map,
        .type = (const void *)&type_nativeobj_data_struct_map_obj };
}
