/* DannyNiu/NJF, 2025-11-21. Public Domain. */

#include "expr.h"

struct value_nativeobj BitLShiftExpr(
    struct value_nativeobj a, struct value_nativeobj b)
{
    int newtype = DetermineTypeForIntegerContext(
        NormalizeTypeForArithContext(a),
        NormalizeTypeForArithContext(b));

    if( newtype == valtyp_long )
    {
        a = ConvertToLong(a);
        b = ConvertToLong(b);
    }

    if( newtype == valtyp_ulong )
    {
        a = ConvertToUlong(a);
        b = ConvertToUlong(b);
    }

    a.proper.u <<= b.proper.u;
    return a;
}

struct value_nativeobj BitRShiftExpr(
    struct value_nativeobj a, struct value_nativeobj b)
{
    int newtype = DetermineTypeForIntegerContext(
        NormalizeTypeForArithContext(a),
        NormalizeTypeForArithContext(b));

    if( newtype == valtyp_long )
    {
        a = ConvertToLong(a);
        b = ConvertToLong(b);
    }

    if( newtype == valtyp_ulong )
    {
        a = ConvertToUlong(a);
        b = ConvertToUlong(b);
    }

    a.proper.u >>= b.proper.u;

    return a;
}

struct value_nativeobj BitARShiftExpr(
    struct value_nativeobj a, struct value_nativeobj b)
{
    int newtype = DetermineTypeForIntegerContext(
        NormalizeTypeForArithContext(a),
        NormalizeTypeForArithContext(b));

    if( newtype == valtyp_long )
    {
        a = ConvertToLong(a);
        b = ConvertToLong(b);
    }

    if( newtype == valtyp_ulong )
    {
        a = ConvertToUlong(a);
        b = ConvertToUlong(b);
    }

    a.proper.l >>= b.proper.l;

    return a;
}

struct value_nativeobj BitAndExpr(
    struct value_nativeobj a, struct value_nativeobj b)
{
    int newtype = DetermineTypeForIntegerContext(
        NormalizeTypeForArithContext(a),
        NormalizeTypeForArithContext(b));

    if( newtype == valtyp_long )
    {
        a = ConvertToLong(a);
        b = ConvertToLong(b);
    }

    if( newtype == valtyp_ulong )
    {
        a = ConvertToUlong(a);
        b = ConvertToUlong(b);
    }

    a.proper.u &= b.proper.u;

    return a;
}

struct value_nativeobj BitOrExpr(
    struct value_nativeobj a, struct value_nativeobj b)
{
    int newtype = DetermineTypeForIntegerContext(
        NormalizeTypeForArithContext(a),
        NormalizeTypeForArithContext(b));

    if( newtype == valtyp_long )
    {
        a = ConvertToLong(a);
        b = ConvertToLong(b);
    }

    if( newtype == valtyp_ulong )
    {
        a = ConvertToUlong(a);
        b = ConvertToUlong(b);
    }

    a.proper.u |= b.proper.u;

    return a;
}

struct value_nativeobj BitXorExpr(
    struct value_nativeobj a, struct value_nativeobj b)
{
    int newtype = DetermineTypeForIntegerContext(
        NormalizeTypeForArithContext(a),
        NormalizeTypeForArithContext(b));

    if( newtype == valtyp_long )
    {
        a = ConvertToLong(a);
        b = ConvertToLong(b);
    }

    if( newtype == valtyp_ulong )
    {
        a = ConvertToUlong(a);
        b = ConvertToUlong(b);
    }

    a.proper.u ^= b.proper.u;

    return a;
}
