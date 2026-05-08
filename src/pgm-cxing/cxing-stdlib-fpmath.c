/* DannyNiu/NJF, 2026-05-05. Public Domain. */

#include "cxing-stdlib.h"
#include <math.h>


// rounding modes. (2026-05-08 TODO).
// ====


// helper boilderplates.
// ====

#define MathBinding_FuncProto(funcname)                 \
    struct value_nativeobj CxingMath_##funcname(        \
        int argn, struct value_nativeobj args[])

#define UnaryFormatOfOperation(funcname)                        \
    MathBinding_FuncProto(funcname)                             \
    {                                                           \
        AssertArgN(1);                                          \
        args[0] = ConvertToDouble(args[0]);                     \
        return (struct value_nativeobj){                        \
            .proper.f = funcname(args[0].proper.f),             \
            .type = (const void *)&type_nativeobj_double };     \
    }

#define BinaryFormatOfOperation(funcname)                       \
    MathBinding_FuncProto(funcname)                             \
    {                                                           \
        AssertArgN(2);                                          \
        args[0] = ConvertToDouble(args[0]);                     \
        args[1] = ConvertToDouble(args[1]);                     \
        return (struct value_nativeobj){                        \
            .proper.f = funcname(                               \
                args[0].proper.f,                               \
                args[1].proper.f),                              \
            .type = (const void *)&type_nativeobj_double };     \
    }

#define UnaryPredicateOperation(funcname)                       \
    MathBinding_FuncProto(funcname)                             \
    {                                                           \
        AssertArgN(1);                                          \
        args[0] = ConvertToDouble(args[0]);                     \
        return (struct value_nativeobj){                        \
            .proper.l = funcname(args[0].proper.f),             \
            .type = (const void *)&type_nativeobj_long };       \
    }


// mandatory operations not expressed as operators
// ====

UnaryFormatOfOperation(sqrt)

MathBinding_FuncProto(fma)
{
    AssertArgN(3);
    args[0] = ConvertToDouble(args[0]);
    args[1] = ConvertToDouble(args[1]);
    args[2] = ConvertToDouble(args[2]);
    return (struct value_nativeobj){
        .proper.f = fma(
            args[0].proper.f,
            args[1].proper.f,
            args[2].proper.f),
        .type = (const void *)&type_nativeobj_double };
}

UnaryFormatOfOperation(fabs)
    BinaryFormatOfOperation(copysign)

    double fmax_emu(double a, double b);
double fmin_emu(double a, double b);
double fpmax_emu(double a, double b);
double fpmin_emu(double a, double b);

MathBinding_FuncProto(fmax)
{
    AssertArgN(2);
    double x = ConvertToDouble(args[0]).proper.f;
    double y = ConvertToDouble(args[1]).proper.f;
    return (struct value_nativeobj){
        .proper.f = fmax_emu(x, y),
        .type = (const void *)&type_nativeobj_double };
}

MathBinding_FuncProto(fmin)
{
    AssertArgN(2);
    double x = ConvertToDouble(args[0]).proper.f;
    double y = ConvertToDouble(args[1]).proper.f;
    return (struct value_nativeobj){
        .proper.f = fmin_emu(x, y),
        .type = (const void *)&type_nativeobj_double };
}

MathBinding_FuncProto(fpmax)
{
    AssertArgN(2);
    double x = ConvertToDouble(args[0]).proper.f;
    double y = ConvertToDouble(args[1]).proper.f;
    return (struct value_nativeobj){
        .proper.f = fpmax_emu(x, y),
        .type = (const void *)&type_nativeobj_double };
}

MathBinding_FuncProto(fpmin)
{
    AssertArgN(2);
    double x = ConvertToDouble(args[0]).proper.f;
    double y = ConvertToDouble(args[1]).proper.f;
    return (struct value_nativeobj){
        .proper.f = fpmin_emu(x, y),
        .type = (const void *)&type_nativeobj_double };
}

UnaryPredicateOperation(fpclassify)
    UnaryPredicateOperation(isfinite)
    UnaryPredicateOperation(isinf)
    UnaryPredicateOperation(isnan)
    UnaryPredicateOperation(isnormal)
    UnaryPredicateOperation(signbit)

// Eventhough this isn't a predicate.
    UnaryPredicateOperation(ilogb)

    MathBinding_FuncProto(scalbn)
{
    AssertArgN(2);
    double x = ConvertToDouble(args[0]).proper.f;
    long y = ConvertToLong(args[1]).proper.l;
    return (struct value_nativeobj){
        .proper.f = scalbln(x, y),
        .type = (const void *)&type_nativeobj_double };
}

UnaryFormatOfOperation(ceil)
    UnaryFormatOfOperation(floor)
    UnaryFormatOfOperation(trunc)
    UnaryFormatOfOperation(round)

    double roundeven_emu(double);
MathBinding_FuncProto(roundeven)
{
    AssertArgN(1);
    double x = ConvertToDouble(args[0]).proper.f;
    return (struct value_nativeobj){
        .proper.f = roundeven_emu(x),
        .type = (const void *)&type_nativeobj_double };
}

UnaryFormatOfOperation(rint)

// recommended operations provided to applications on best-effort basis
// ====

#if __STDC_VERSION__ < 202311L

    double sinpi(double);
double cospi(double);
double tanpi(double);

double asinpi(double);
double acospi(double);
double atanpi(double);
double atan2pi(double);

double exp10(double);
double exp2m1(double);
double exp10m1(double);
double log2p1(double);
double log10p1(double);

double compoundn(double x, long long n);
double pown(double x, long long n);
double rootn(double x, long long n);
double rsqrt(double x);

#endif /* Implementation of an older C standard. */

UnaryFormatOfOperation(sin)
    UnaryFormatOfOperation(cos)
    UnaryFormatOfOperation(tan)
    UnaryFormatOfOperation(sinpi)
    UnaryFormatOfOperation(cospi)
    UnaryFormatOfOperation(tanpi)

    UnaryFormatOfOperation(asin)
    UnaryFormatOfOperation(acos)
    UnaryFormatOfOperation(atan)
    BinaryFormatOfOperation(atan2)
    UnaryFormatOfOperation(asinpi)
    UnaryFormatOfOperation(acospi)
    UnaryFormatOfOperation(atanpi)
    UnaryFormatOfOperation(atan2pi)

    UnaryFormatOfOperation(sinh)
UnaryFormatOfOperation(cosh)
UnaryFormatOfOperation(tanh)
UnaryFormatOfOperation(asinh)
UnaryFormatOfOperation(acosh)
UnaryFormatOfOperation(atanh)

UnaryFormatOfOperation(exp)
UnaryFormatOfOperation(exp2)
UnaryFormatOfOperation(exp10)
UnaryFormatOfOperation(expm1)
UnaryFormatOfOperation(exp2m1)
UnaryFormatOfOperation(exp10m1)

UnaryFormatOfOperation(log)
UnaryFormatOfOperation(log2)
UnaryFormatOfOperation(log10)
UnaryFormatOfOperation(log1p)
MathBinding_FuncProto(logp1) { return CxingMath_log1p(argn, args); }
UnaryFormatOfOperation(log2p1)
    UnaryFormatOfOperation(log10p1)

    UnaryFormatOfOperation(cbrt)

    MathBinding_FuncProto(compoundn)
{
    AssertArgN(2);
    double x = ConvertToDouble(args[0]).proper.f;
    double y;
    long long n;

    if( IsInteger(args[1]) )
    {
        n = args[1].proper.l;
        x = compoundn(x, n);
    }
    else
    {
        y = ConvertToDouble(args[1]).proper.f;
        x = exp(log1p(x) * y);
    }

    return (struct value_nativeobj){
        .proper.f = x,
        .type = (const void *)&type_nativeobj_double };
}

BinaryFormatOfOperation(hypot)
    BinaryFormatOfOperation(pow)

    MathBinding_FuncProto(pown)
{
    AssertArgN(2);
    double x = ConvertToDouble(args[0]).proper.f;
    double y;
    long long n;

    if( IsInteger(args[1]) )
    {
        n = args[1].proper.l;
        x = pown(x, n);
    }
    else
    {
        y = ConvertToDouble(args[1]).proper.f;
        x = pow(x, y);
    }

    return (struct value_nativeobj){
        .proper.f = x,
        .type = (const void *)&type_nativeobj_double };
}

#define powr(x,y) pow(x,y)
BinaryFormatOfOperation(powr)

    MathBinding_FuncProto(rootn)
{
    AssertArgN(2);
    double x = ConvertToDouble(args[0]).proper.f;
    double y;
    long long n;

    if( IsInteger(args[1]) )
    {
        n = args[1].proper.l;
        x = rootn(x, n);
    }
    else
    {
        y = ConvertToDouble(args[1]).proper.f;
        x = pow(x, 1/y);
    }

    return (struct value_nativeobj){
        .proper.f = x,
        .type = (const void *)&type_nativeobj_double };
}

UnaryFormatOfOperation(rsqrt)

    cxing_builtin_def_t CxingStdlibMathBuiltins[] = {
#define CxingMathBinding(funcname)                              \
    { #funcname, (struct value_nativeobj){                      \
            .proper.p = CxingMath_##funcname,                   \
            .type = (const void *)&type_nativeobj_subr } },
#define CxingMathConstants(constname)                           \
    { #constname, (struct value_nativeobj){                     \
            .proper.f = constname,                              \
            .type = (const void *)&type_nativeobj_double } },
#define CxingMathEnums(enumname)                                \
    { #enumname, (struct value_nativeobj){                      \
            .proper.l = enumname,                               \
            .type = (const void *)&type_nativeobj_long } },

#include "cxing-math-bindings.inc"

#undef CxingMathBinding
#undef CxingMathConstants
#undef CxingMathEnums

    { 0 },
};
