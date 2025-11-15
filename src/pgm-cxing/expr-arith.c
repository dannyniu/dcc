/* DannyNiu/NJF, 2025-11-15. Public Domain. */

#include "expr.h"
#include <math.h>

#define DefineArithOp(Identifier, AssignOp)                     \
    struct value_nativeobj Identifier(                          \
        struct value_nativeobj a, struct value_nativeobj b)     \
    {                                                           \
        int newtype = DetermineTypeForArithContext(             \
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
    int newtype = DetermineTypeForArithContext(
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
    int newtype = DetermineTypeForArithContext(
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
