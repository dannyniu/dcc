/* DannyNiu/NJF, 2025-09-14. Public Domain. */

#include "expr.h"

struct value_nativeobj NullCoalesceExpr(
    struct value_nativeobj testexpr,
    struct value_nativeobj replexpr)
{
    if( IsNullish(testexpr) )
        return replexpr;
    else return testexpr;
}

struct value_nativeobj IncrementExpr(
    struct lvalue_nativeobj lvalue, int preop)
{
    struct value_nativeobj ovalue, svalue;
    int newtype = DetermineTypeForArithContext(
        NormalizeTypeForArithContext(lvalue.value), -1);

    if( newtype == valtyp_long ) ovalue = ConvertToLong(lvalue.value);
    if( newtype == valtyp_ulong ) ovalue = ConvertToUlong(lvalue.value);
    if( newtype == valtyp_double ) ovalue = ConvertToDouble(lvalue.value);

    if( newtype == valtyp_long ) ovalue.proper.l ++;
    if( newtype == valtyp_ulong ) ovalue.proper.u ++;
    if( newtype == valtyp_double ) ovalue.proper.f += 1;

    if( preop ) svalue = ValueCopy(lvalue.value);
    SetValProperty(lvalue.scope, (struct value_nativeobj){
            .proper.p = lvalue.key,
            .type = type_nativeobj_s2data_str
        }, ovalue);

    if( preop )
    {
        // The operator is at the left side of the operand.
        return svalue;
    }
    else
    {
        return ovalue;
    }
}

struct value_nativeobj DecrementExpr(
    struct lvalue_nativeobj lvalue, int preop)
{
    struct value_nativeobj ovalue, svalue;
    int newtype = DetermineTypeForArithContext(
        NormalizeTypeForArithContext(lvalue.value), -1);

    if( newtype == valtyp_long ) ovalue = ConvertToLong(lvalue.value);
    if( newtype == valtyp_ulong ) ovalue = ConvertToUlong(lvalue.value);
    if( newtype == valtyp_double ) ovalue = ConvertToDouble(lvalue.value);

    if( newtype == valtyp_long ) ovalue.proper.l --;
    if( newtype == valtyp_ulong ) ovalue.proper.u --;
    if( newtype == valtyp_double ) ovalue.proper.f -= 1;

    if( preop ) svalue = ValueCopy(lvalue.value);
    SetValProperty(lvalue.scope, (struct value_nativeobj){
            .proper.p = lvalue.key,
            .type = type_nativeobj_s2data_str
        }, ovalue);

    if( preop )
    {
        // The operator is at the left side of the operand.
        return svalue;
    }
    else
    {
        return ovalue;
    }
}

struct value_nativeobj PositiveExpr(
    struct value_nativeobj ivalue)
{
    struct value_nativeobj ovalue;
    int newtype = DetermineTypeForArithContext(
        NormalizeTypeForArithContext(ivalue), -1);

    if( newtype == valtyp_long ) ovalue = ConvertToLong(ivalue);
    if( newtype == valtyp_ulong ) ovalue = ConvertToUlong(ivalue);
    if( newtype == valtyp_double ) ovalue = ConvertToDouble(ivalue);

    return ovalue;
}

struct value_nativeobj NegativeExpr(
    struct value_nativeobj ivalue)
{
    struct value_nativeobj ovalue;
    int newtype = DetermineTypeForArithContext(
        NormalizeTypeForArithContext(ivalue), -1);

    if( newtype == valtyp_long )
    {
        ovalue = ConvertToLong(ivalue);
        ovalue.proper.l = -ovalue.proper.l;
    }

    if( newtype == valtyp_ulong )
    {
        ovalue = ConvertToUlong(ivalue);
        ovalue.proper.u = -ovalue.proper.u;
    }

    if( newtype == valtyp_double )
    {
        ovalue = ConvertToDouble(ivalue);
        ovalue.proper.f = -ovalue.proper.f;
    }

    return ovalue;
}

struct value_nativeobj BitComplExpr(
    struct value_nativeobj ivalue)
{
    if( ivalue.type->typeid == valtyp_ulong )
    {
        ivalue.proper.u = ~ivalue.proper.u;
    }
    else
    {
        ivalue = ConvertToLong(ivalue);
        ivalue.proper.l = ~ivalue.proper.l;
    }
    return ivalue;
}

bool LogicallyTrue(
    struct value_nativeobj ivalue)
{
    if( IsNull(ivalue) ) return false;
    else if( ivalue.type->typeid == valtyp_long )
        return ivalue.proper.l != 0;
    else if( ivalue.type->typeid == valtyp_ulong )
        return ivalue.proper.u != 0;
    else if( ivalue.type->typeid == valtyp_double )
        return ivalue.proper.f != 0;
    else return true; // non-null object.
}

struct value_nativeobj LogicNotExpr(
    struct value_nativeobj ivalue)
{
    return (struct value_nativeobj){
        .proper.l = LogicallyTrue(ivalue) ? 1 : 0,
        .type = type_nativeobj_long };
}
