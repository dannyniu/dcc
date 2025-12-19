/* DannyNiu/NJF, 2025-11-15. Public Domain. */

#include "expr.h"
#include "runtime.h"
#include <math.h>

#define DefineArithOp(Identifier, AssignOp)                     \
    struct value_nativeobj Identifier(                          \
        struct value_nativeobj a, struct value_nativeobj b)     \
    {                                                           \
        int newtype = DetermineValueTypeForArithContext(        \
            NormalizeTypeForArithContext(a),                    \
            NormalizeTypeForArithContext(b));                   \
                                                                \
        if( newtype == valtyp_long )                            \
        {                                                       \
            a = ConvertToLong(a);                               \
            b = ConvertToLong(b);                               \
            a.proper.l AssignOp b.proper.l;                     \
        }                                                       \
                                                                \
        if( newtype == valtyp_ulong )                           \
        {                                                       \
            a = ConvertToUlong(a);                              \
            b = ConvertToUlong(b);                              \
            a.proper.u AssignOp b.proper.u;                     \
        }                                                       \
                                                                \
        if( newtype == valtyp_double )                          \
        {                                                       \
            a = ConvertToDouble(a);                             \
            b = ConvertToDouble(b);                             \
            a.proper.f AssignOp b.proper.f;                     \
        }                                                       \
                                                                \
        return a;                                               \
    }

DefineArithOp(ArithAddExpr, +=)
DefineArithOp(ArithSubExpr, -=)
DefineArithOp(ArithMulExpr, *=)

struct value_nativeobj ArithDivExpr(
    struct value_nativeobj a, struct value_nativeobj b)
{
    struct value_nativeobj d;
    int newtype = DetermineValueTypeForArithContext(
        NormalizeTypeForArithContext(a),
        NormalizeTypeForArithContext(b));

    if( newtype == valtyp_long )
    {
        d = ConvertToLong(b);
        if( d.proper.l == 0 )
            newtype = valtyp_double;
    }

    if( newtype == valtyp_ulong )
    {
        d = ConvertToUlong(b);
        if( d.proper.u == 0 )
            newtype = valtyp_double;
    }

    if( newtype == valtyp_long )
    {
        a = ConvertToLong(a);
        a.proper.l /= d.proper.l;
    }

    if( newtype == valtyp_ulong )
    {
        a = ConvertToUlong(a);
        a.proper.u /= d.proper.u;
    }

    if( newtype == valtyp_double )
    {
        a = ConvertToDouble(a);
        b = ConvertToDouble(b);
        a.proper.f /= b.proper.f;
    }

    return a;
}

struct value_nativeobj ArithModExpr(
    struct value_nativeobj a, struct value_nativeobj b)
{
    struct value_nativeobj d;
    int newtype = DetermineValueTypeForArithContext(
        NormalizeTypeForArithContext(a),
        NormalizeTypeForArithContext(b));

    if( newtype == valtyp_long )
    {
        d = ConvertToLong(b);
        if( d.proper.l == 0 )
            newtype = valtyp_double;
    }

    if( newtype == valtyp_ulong )
    {
        d = ConvertToUlong(b);
        if( d.proper.u == 0 )
            newtype = valtyp_double;
    }

    if( newtype == valtyp_long )
    {
        a = ConvertToLong(a);
        a.proper.l %= d.proper.l;
    }

    if( newtype == valtyp_ulong )
    {
        a = ConvertToUlong(a);
        a.proper.u %= d.proper.u;
    }

    if( newtype == valtyp_double )
    {
        a = ConvertToDouble(a);
        b = ConvertToDouble(b);
        a.proper.f = fmod(a.proper.f, b.proper.f);
    }

    return a;
}

static int *OrderingOfArithVal(
    struct value_nativeobj a,
    struct value_nativeobj b,
    int *order)
{
    int newtype = DetermineOrderingTypeForArithContext(
        NormalizeTypeForArithContext(a),
        NormalizeTypeForArithContext(b));
    int le, eq, ge;

    if( newtype == valtyp_long )
    {
        a = ConvertToLong(a);
        b = ConvertToLong(b);
        le = a.proper.l <= b.proper.l;
        eq = a.proper.l == b.proper.l;
        ge = a.proper.l >= b.proper.l;
    }

    if( newtype == valtyp_ulong )
    {
        a = ConvertToUlong(a);
        b = ConvertToUlong(b);
        le = a.proper.u <= b.proper.u;
        eq = a.proper.u == b.proper.u;
        ge = a.proper.u >= b.proper.u;
    }

    if( newtype == valtyp_double )
    {
        a = ConvertToDouble(a);
        b = ConvertToDouble(b);
        le = a.proper.f <= b.proper.f;
        eq = a.proper.f == b.proper.f;
        ge = a.proper.f >= b.proper.f;
    }

    if( eq ) { *order = 0; return order; }
    else if( ge ) { *order = 1; return order; }
    else if( le ) { *order = -1; return order; }
    else return NULL; // Possible with NaNs.
}

// Compare a with b using the `cmpwith()` method property of `a`,
// placing the result in the location pointed to by `order`,
// which is then returned.
// If the method is not found, or doesn't return an integer,
// the location pointed to by `order` is not modified, and
// a NULL pointer is returned.
static int *OrderWithMethodProperty(
    struct value_nativeobj a,
    struct value_nativeobj b,
    int *order)
{
    struct value_nativeobj CmpWithMethod;
    struct value_nativeobj retval;
    struct value_nativeobj args[2];

    CmpWithMethod = GetValProperty(
        a, CxingPropName_cmpwith).value;

    if( CmpWithMethod.type->typeid != valtyp_method )
        return NULL;

    args[0] = a;
    args[1] = b;
    retval = ((cxing_call_proto)CmpWithMethod.proper.p)(2, args);

    if( !IsInteger(retval) ) return NULL;

    *order = retval.proper.l;
    return order;
}

// Same as above, except that it also checks the method property on `b`.
static int *OrderWithMethodProperties(
    struct value_nativeobj a,
    struct value_nativeobj b,
    int *order)
{
    int ordering;
    int *ret;

    if( !(ret = OrderWithMethodProperty(a, b, &ordering)) )
    {
        ret = OrderWithMethodProperty(b, a, &ordering);
        ordering = -ordering;
    }

    if( ret ) *order = ordering;
    return order;
}

// Same as above, but does a function compatibility check.
static int *OrderingOfObjects(
    struct value_nativeobj a,
    struct value_nativeobj b,
    int *order)
{
    struct value_nativeobj meta, metb;

    meta = GetValProperty(a, CxingPropName_cmpwith).value;
    metb = GetValProperty(b, CxingPropName_cmpwith).value;
    if( !IsFunction(meta) || !IsFunction(metb) )
        return NULL;

    if( meta.proper.p != metb.proper.p )
        return OrderWithMethodProperties(a, b, order);

    else return OrderWithMethodProperty(a, b, order);
}

#define DefineOrderingRelForValues(Identifier, CmpOp)           \
    struct value_nativeobj Identifier(                          \
        struct value_nativeobj a,                               \
        struct value_nativeobj b)                               \
    {                                                           \
        int order;                                              \
                                                                \
        if( IsFunction(a) || IsFunction(b) )                    \
            return Logic2ValueNativeObj(false);                 \
                                                                \
        if( a.type->typeid == valtyp_obj &&                     \
            b.type->typeid == valtyp_obj )                      \
        {                                                       \
            if( !OrderingOfObjects(a, b, &order) )              \
                return Logic2ValueNativeObj(false);             \
            else return Logic2ValueNativeObj(order CmpOp 0);    \
        }                                                       \
                                                                \
        else /* assume arithmetic values. */                    \
        {                                                       \
            if( !OrderingOfArithVal(a, b, &order) )             \
                return Logic2ValueNativeObj(false);             \
            else return Logic2ValueNativeObj(order CmpOp 0);    \
        }                                                       \
    }

DefineOrderingRelForValues(LessThanExpr, <);
DefineOrderingRelForValues(LessEqaulExpr, <=);
DefineOrderingRelForValues(GreaterThanExpr, >);
DefineOrderingRelForValues(GreaterEqaulExpr, >=);

static int *EqualityWithMethodProperty(
    struct value_nativeobj a,
    struct value_nativeobj b,
    int *equals)
{
    struct value_nativeobj EqualsMethod;
    struct value_nativeobj retval;
    struct value_nativeobj args[2];

    EqualsMethod = GetValProperty(
        a, CxingPropName_equals).value;

    if( EqualsMethod.type->typeid != valtyp_method )
        return NULL;

    args[0] = a;
    args[1] = b;
    retval = ((cxing_call_proto)EqualsMethod.proper.p)(2, args);

    if( !IsInteger(retval) ) return NULL;

    *equals = retval.proper.l;
    return equals;
}

static int *EqualityWithMethodProperties(
    struct value_nativeobj a,
    struct value_nativeobj b,
    int *equals)
{
    int equality;
    int *ret;

    if( !(ret = EqualityWithMethodProperty(a, b, &equality)) )
    {
        ret = EqualityWithMethodProperty(b, a, &equality);
    }

    if( ret ) *equals = equality;
    return equals;
}

static int EqualityOfObjects(
    struct value_nativeobj a,
    struct value_nativeobj b)
{
    int subret;
    if( EqualityWithMethodProperties(a, b, &subret) )
    {
        return subret;
    }
    else if( OrderWithMethodProperties(a, b, &subret) )
    {
        return subret == 0;
    }
    else return false;
}

static int EqualityOfArithVal(
    struct value_nativeobj a,
    struct value_nativeobj b)
{
    int newtype = DetermineOrderingTypeForArithContext(
        NormalizeTypeForArithContext(a),
        NormalizeTypeForArithContext(b));
    // int eq;

    if( newtype == valtyp_long )
    {
        a = ConvertToLong(a);
        b = ConvertToLong(b);
        return a.proper.l == b.proper.l;
    }

    if( newtype == valtyp_ulong )
    {
        a = ConvertToUlong(a);
        b = ConvertToUlong(b);
        return a.proper.u == b.proper.u;
    }

    if( newtype == valtyp_double )
    {
        a = ConvertToDouble(a);
        b = ConvertToDouble(b);
        return a.proper.f == b.proper.f;
    }

    // One is null, the other is not, so false.
    // If both are nulls, then caller would've returned.
    return false;
}

bool LogicalLooseEquality(
    struct value_nativeobj a,
    struct value_nativeobj b)
{
    if( IsNull(a) && IsNull(b) )
        return true;

    if( IsFunction(a) && IsFunction(b) )
        return a.proper.p == b.proper.p;

    if( a.type->typeid == valtyp_obj &&
        b.type->typeid == valtyp_obj )
        return EqualityOfObjects(a, b);

    return EqualityOfArithVal(a, b);
}

bool LogicalStrictEquality(
    struct value_nativeobj a,
    struct value_nativeobj b)
{
    if( IsNull(a) && IsNull(b) )
        return true;

    if( IsFunction(a) && IsFunction(b) )
        return a.proper.p == b.proper.p;

    if( a.type->typeid == valtyp_obj &&
        b.type->typeid == valtyp_obj )
        return a.proper.p == b.proper.p;

    return EqualityOfArithVal(a, b);
}

struct value_nativeobj LooseEqualityExpr(
    struct value_nativeobj a,
    struct value_nativeobj b)
{
    return Logic2ValueNativeObj(LogicalLooseEquality(a, b));
}

struct value_nativeobj StrictEqualityExpr(
    struct value_nativeobj a,
    struct value_nativeobj b)
{
    return Logic2ValueNativeObj(LogicalStrictEquality(a, b));
}
