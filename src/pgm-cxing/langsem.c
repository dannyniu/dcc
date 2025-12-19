/* DannyNiu/NJF, 2025-10-02. Public Domain. */

#include "langsem.h"
#include "runtime.h"

// The "Morgoth" null.
const struct TYPE_NATIVEOBJ_STRUCT(1) type_nativeobj_morgoth_def = {
    .typeid = valtyp_obj, .n_entries = 0, .static_members[0] = {} };

// The "blessed" null.
const struct TYPE_NATIVEOBJ_STRUCT(1) type_nativeobj_null_def = {
    .typeid = valtyp_null, .n_entries = 0, .static_members[0] = {} };

// The arithmetic types.
const struct TYPE_NATIVEOBJ_STRUCT(1) type_nativeobj_long_def = {
    .typeid = valtyp_long, .n_entries = 0, .static_members[0] = {} };

const struct TYPE_NATIVEOBJ_STRUCT(1) type_nativeobj_ulong_def = {
    .typeid = valtyp_ulong, .n_entries = 0, .static_members[0] = {} };

const struct TYPE_NATIVEOBJ_STRUCT(1) type_nativeobj_double_def = {
    .typeid = valtyp_double, .n_entries = 0, .static_members[0] = {} };

const struct TYPE_NATIVEOBJ_STRUCT(9) type_nativeobj_s2data_str_def = {
    .typeid = valtyp_obj,
    .n_entries = 8,
    .static_members = {
        // 2025-12-13: TODO.
    },
};

// The "Morgoth" null.
const struct type_nativeobj *type_nativeobj_morgoth =
    (const void *)&type_nativeobj_morgoth_def;

// The "blessed" null.
const struct type_nativeobj *type_nativeobj_null =
    (const void *)&type_nativeobj_null_def;

// The arithmetic types.
const struct type_nativeobj *type_nativeobj_long =
    (const void *)&type_nativeobj_long_def;

const struct type_nativeobj *type_nativeobj_ulong =
    (const void *)&type_nativeobj_ulong_def;

const struct type_nativeobj *type_nativeobj_double =
    (const void *)&type_nativeobj_double_def;

const struct type_nativeobj *type_nativeobj_s2data_str =
    (const void *)&type_nativeobj_s2data_str_def;

int NormalizeTypeForArithContext(struct value_nativeobj val)
{
    if( IsNull(val) ) return valtyp_null;
    else if( val.type->typeid == valtyp_ulong ||
             val.type->typeid == valtyp_double )
        return val.type->typeid;
    else return valtyp_long;
}

static int DetermineNumericTypeForArithContext(int type1, int type2)
{
    if( type1 == valtyp_double ||
        type2 == valtyp_double )
        return valtyp_double;

    if( type1 == valtyp_ulong ||
        type2 == valtyp_ulong )
        return valtyp_ulong;

    // type 1 and 2 can't both be absent, so assumed.
    return valtyp_long;
}

int DetermineValueTypeForArithContext(int type1, int type2)
{
    if( type1 == valtyp_null ||
        type2 == valtyp_null )
    {
        // -1 means operand absent.
        // The only null operand remain,
        // and it's arithmetic context.
        if( type1 == -1 ||
            type2 == -1 )
            return valtyp_long;
    }
    // still null.

    return DetermineNumericTypeForArithContext(type1, type2);
}

int DetermineOrderingTypeForArithContext(int type1, int type2)
{
    if( type1 == valtyp_null ||
        type2 == valtyp_null )
        return valtyp_null;
    else return DetermineNumericTypeForArithContext(type1, type2);
}

int DetermineTypeForIntegerContext(int type1, int type2)
{
    if( type1 == valtyp_ulong ||
        type2 == valtyp_ulong )
        return valtyp_ulong;
    else return valtyp_long;
}

struct value_nativeobj ConvertToDouble(struct value_nativeobj val)
{
    struct value_nativeobj ret;
    ret.type = type_nativeobj_double;

    if( val.type->typeid == valtyp_null )
        ret.proper.l = -1; // all-bit-one representation of NaN.

    else if( val.type->typeid == valtyp_obj ||
             val.type->typeid == valtyp_subr ||
             val.type->typeid == valtyp_method ||
             val.type->typeid == valtyp_ffisubr ||
             val.type->typeid == valtyp_ffimethod )
    {
        if( val.proper.p )
            ret.proper.f = 1.0;
        else ret.proper.l = -1; // all-bit-one representation of NaN.
    }

    else if( val.type->typeid == valtyp_long )
    {
        ret.proper.f = val.proper.l;
    }

    else if( val.type->typeid == valtyp_ulong )
    {
        ret.proper.f = val.proper.u;
    }

    else if( val.type->typeid == valtyp_double )
    {
        ret.proper.f = val.proper.f;
    }

    else assert( 0 );

    return ret;
}

struct value_nativeobj ConvertToUlong(struct value_nativeobj val)
{
    struct value_nativeobj ret;
    ret.type = type_nativeobj_ulong;

    if( val.type->typeid == valtyp_null )
        ret.proper.u = 0;

    else if( val.type->typeid == valtyp_obj ||
             val.type->typeid == valtyp_subr ||
             val.type->typeid == valtyp_method ||
             val.type->typeid == valtyp_ffisubr ||
             val.type->typeid == valtyp_ffimethod )
    {
        if( val.proper.p )
            ret.proper.u = 1;
        else ret.proper.u = 0;
    }

    else if( val.type->typeid == valtyp_long )
    {
        ret.proper.u = val.proper.l;
    }

    else if( val.type->typeid == valtyp_ulong )
    {
        ret.proper.u = val.proper.u;
    }

    else if( val.type->typeid == valtyp_double )
    {
        ret.proper.u = val.proper.f;
    }

    else assert( 0 );

    return ret;
}

struct value_nativeobj ConvertToLong(struct value_nativeobj val)
{
    struct value_nativeobj ret;
    ret.type = type_nativeobj_ulong;

    if( val.type->typeid == valtyp_null )
        ret.proper.l = 0;

    else if( val.type->typeid == valtyp_obj ||
             val.type->typeid == valtyp_subr ||
             val.type->typeid == valtyp_method ||
             val.type->typeid == valtyp_ffisubr ||
             val.type->typeid == valtyp_ffimethod )
    {
        if( val.proper.p )
            ret.proper.l = 1;
        else ret.proper.l = 0;
    }

    else if( val.type->typeid == valtyp_long )
    {
        ret.proper.l = val.proper.l;
    }

    else if( val.type->typeid == valtyp_ulong )
    {
        ret.proper.l = val.proper.u;
    }

    else if( val.type->typeid == valtyp_double )
    {
        ret.proper.l = val.proper.f;
    }

    else assert( 0 );

    return ret;
}

struct value_nativeobj Logic2ValueNativeObj(int logic)
{
    // 2025-11-21:
    // Otherwise it'll be verbose.
    return (struct value_nativeobj){
        .proper.l = logic ? 1 : 0,
        .type = type_nativeobj_long };
}

bool IsInteger(struct value_nativeobj val)
{
    if( val.type->typeid == valtyp_long ||
        val.type->typeid == valtyp_ulong )
        return true;
    else return false;
}

bool IsFunction(struct value_nativeobj val)
{
    if( val.type->typeid == valtyp_subr ||
        val.type->typeid == valtyp_method ||
        val.type->typeid == valtyp_ffisubr ||
        val.type->typeid == valtyp_ffimethod )
    {
        return true;
    }
    else return false;
}

bool IsNull(struct value_nativeobj val)
{
    if( val.type->typeid == valtyp_null )
    {
        return true;
    }
    else if( val.type->typeid == valtyp_obj ||
             val.type->typeid == valtyp_subr ||
             val.type->typeid == valtyp_method ||
             val.type->typeid == valtyp_ffisubr ||
             val.type->typeid == valtyp_ffimethod )
    {
        if( val.proper.p )
            return false;
        else return true;
    }
    else
    {
        assert( 0 );

        // 2025-10-02: this ought to be considered an internal error.
        return true;
    }
}

bool IsNullish(struct value_nativeobj val)
{
    if( val.type->typeid == valtyp_double )
    {
        if( val.proper.f == val.proper.f )
            return false;
        else return true;
    }
    else return IsNull(val);
}

struct lvalue_nativeobj GetValProperty(
    struct value_nativeobj obj,
    struct value_nativeobj key)
{
    // Gets a property from an object
    const struct type_nativeobj *ty = obj.type;
    uint64_t i;

    const char *name;

    if( key.type != type_nativeobj_s2data_str )
    {
        CxingDiagnose("Unrecognized implementation of the string type.\n"
                      "Does this object come from another runtime?");
        return (struct lvalue_nativeobj){
            .value = {
                .proper.p = NULL,
                .type = type_nativeobj_morgoth },
            .scope = obj,
            .key = (s2data_t *)NULL,
        };
    }

    // This implementation depends on the SafeTypes2 library.
    name = s2data_weakmap(key.proper.p);

    for(i=0; i<ty->n_entries; i++)
    {
        // The spelling of member name of type-associated properties
        // are guaranteed to be valid identifiers. Therefore we
        // don't need to use `memcmp`.
        if( strcmp(name, ty->static_members[i].name) == 0 )
            return (struct lvalue_nativeobj){
                .value = *ty->static_members[i].member,
                .scope = obj,
                .key = (s2data_t *)key.proper.p,
            };
    }

    // not one of the type-associated property,
    // try to fetch it using `__get__`.

    for(i=0; i<ty->n_entries; i++)
    {
        if( strcmp("__get__", ty->static_members[i].name) == 0 )
            break;
    }

    if( i >= ty->n_entries )
    {
        // This object doesn't have a '__get__' property.
        return (struct lvalue_nativeobj){
            .value = {
                .proper.p = NULL,
                .type = type_nativeobj_morgoth },
            .scope = obj,
            .key = (s2data_t *)key.proper.p,
        };
    }

    // Check that '__get__' is indeed a method.

    if( ty->static_members[i].member->type->typeid != valtyp_method )
    {
        CxingDiagnose("The '__get__' property is not a method!");
        return (struct lvalue_nativeobj){
            .value = {
                .proper.p = NULL,
                .type = type_nativeobj_morgoth },
            .scope = obj,
            .key = (s2data_t *)key.proper.p,
        };
    }
    else
    {
        struct value_nativeobj args[2] = { [0] = obj, [1] = key };
        return (struct lvalue_nativeobj){
            .value = ((cxing_call_proto)
                      ty->static_members[i].
                      member->proper.p)(2, args),
            .scope = obj,
            .key = (s2data_t *)key.proper.p,
        };
    }
}

struct value_nativeobj SetValProperty(
    struct value_nativeobj obj,
    struct value_nativeobj key,
    struct value_nativeobj val)
{
    const struct type_nativeobj *ty = obj.type;
    struct value_nativeobj setter = {};
    struct value_nativeobj args[3] = { [0] = obj, [1] = key, [2] = val };
    uint64_t i;

    // The '__set__' property is required to be
    // type-associated (as of 2025-11-15).
    for(i=0; i<ty->n_entries; i++)
    {
        // The spelling of member name of type-associated properties
        // are guaranteed to be valid identifiers. Therefore we
        // don't need to use `memcmp`.
        if( strcmp("__set__", ty->static_members[i].name) == 0 )
            setter = *ty->static_members[i].member;
    }

    // no setter.
    if( !setter.proper.p || setter.type->typeid != valtyp_method )
        return (struct value_nativeobj){
            .proper.p = NULL, .type = type_nativeobj_null };

    return ((cxing_call_proto)setter.proper.p)(3, args);
}

struct value_nativeobj ValueCopy(struct value_nativeobj val)
{
    struct value_nativeobj mprop =
        GetValProperty(val, CxingPropName_final).value;

    // Check that '__copy__' is indeed a method.

    // The null returned
    if( IsNull(mprop) )
        return val; // no non-trivial '__copy__'.

    if( mprop.type->typeid == valtyp_method )
    {
        return ((cxing_call_proto)mprop.proper.p)(1, &val);
    }
    else if( mprop.type->typeid == valtyp_ffimethod )
    {
        // 2025-11-01:
        // FFI methods proved to be an implementation impossibility
        // for unable to deduce the prototype of functions from the
        // objects from which they're called on.
        CxingFatal("FFI Methods are an Implementation Impossibility!");
        return (struct value_nativeobj){
            .proper.p = NULL, .type = type_nativeobj_morgoth };
    }
    else
    {
        CxingDiagnose("The '__copy__' property is not a method!");
        // This assumes that if typeid is valtyp_obj, then
        // this property must be the other representation of null
        // prescribed by the spec that runtime implementations must
        // recognize. Otherwise, it's not something that can be
        // correctly invoked as a function.
        return (struct value_nativeobj){
            .proper.p = NULL, .type = type_nativeobj_morgoth };
    }
}

void ValueDestroy(struct value_nativeobj val)
{
    // looks up the '__final__' property,
    // and invoke it on the value.

    struct value_nativeobj mprop =
        GetValProperty(val, CxingPropName_final).value;

    // Check that '__final__' is indeed a method.

    if( IsNull(mprop) )
        return; // No finalizer.

    if( mprop.type->typeid == valtyp_method )
    {
        ((cxing_call_proto)mprop.proper.p)(1, &val);
    }
    else if( mprop.type->typeid == valtyp_ffimethod )
    {
        // 2025-11-01:
        // FFI methods proved to be an implementation impossibility
        // for unable to deduce the prototype of functions from the
        // objects from which they're called on.
        CxingFatal("FFI Methods are an Implementation Impossibility!");
    }
    else
    {
        CxingDiagnose("The '__final__' property is not a method!");
        // This assumes that if typeid is valtyp_obj, then
        // this property must be the other representation of null
        // prescribed by the spec that runtime implementations must
        // recognize. Otherwise, it's not something that can be
        // correctly invoked as a function.
        return;
    }
}
