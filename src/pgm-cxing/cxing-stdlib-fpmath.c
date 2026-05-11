/* DannyNiu/NJF, 2026-05-05. Public Domain. */

#include "cxing-stdlib.h"
#include <fenv.h>
#include <math.h>

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


// rounding modes.
// ====

const struct rounding_mode_constants {
    int cxing, cfenv;
} rounding_mode_constants[] = {
    { 0, FE_TONEAREST }, // 0\AA
    { 3*64+15, FE_UPWARD }, // 0\DP
    { 3*64+13, FE_DOWNWARD }, // 0\DN
    { 3*64+25, FE_TOWARDZERO }, // 0\DZ
#ifdef FE_TONEARESTFROMZERO
    { 8, FE_TONEARESTFROMZERO }, // 0\AI
    // To add: round-to-odd. // 0\DO
#endif // FE_TONEARESTFROMZERO
    // Other roundings aren't even standardized yet.
    { -1, -1 },
};

MathBinding_FuncProto(feRndMode)
{
    int i;
    int rnd = fegetround();

    (void)argn;
    (void)args;

    if( rnd < 0 )
    {
        return (struct value_nativeobj){
            // @dannyniu is not sure what errno value is appropriate.
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    for(i=0; rounding_mode_constants[i].cfenv >= 0; i++)
    {
        if( rnd == rounding_mode_constants[i].cfenv )
        {
            return (struct value_nativeobj){
                .proper.l = rounding_mode_constants[i].cxing,
                .type = (const void *)&type_nativeobj_long };
        }
    }

    return (struct value_nativeobj){
        // @dannyniu is not sure what errno value is appropriate.
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth };
}

MathBinding_FuncProto(feRndMode_manual)
{
    int i;
    int rnd;

    AssertArgN(1);
    if( !IsInteger(args[0]) )
    {
        CxingDebug("The rounding mode need to be encoded as an integer.\n");
        return (struct value_nativeobj){
            // @dannyniu is not sure what errno value is appropriate.
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    rnd = args[0].proper.l;

    for(i=0; rounding_mode_constants[i].cfenv >= 0; i++)
    {
        if( rnd == rounding_mode_constants[i].cxing )
        {
            fesetround(rounding_mode_constants[i].cfenv);
            return (struct value_nativeobj){
                .proper.l = rounding_mode_constants[i].cxing,
                .type = (const void *)&type_nativeobj_long };
        }
    }

    return (struct value_nativeobj){
        // @dannyniu is not sure what errno value is appropriate.
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth };
}


#define S2_OBJ_TYPE_CXING_STDFP_AUTOMODE 0x2114
typedef struct {
    s2obj_base;
    int oldmode;
} cxing_stdfp_automode_t;

void cxing_stdfp_automode_restoration(cxing_stdfp_automode_t *modesv)
{
    fesetround(modesv->oldmode);
}

const type_nativeobj_struct_p2 type_nativeobj_stdfp_automode = {
    .typeid = valtyp_obj,
    .n_entries = 2,
    .static_members = {
        { .name = "__copy__", .member = &CxingValue_s2Obj_Copy },
        { .name = "__final__", .member = &CxingValue_s2Obj_Final },
    },
};

MathBinding_FuncProto(feRndMode_auto)
{
    int i, rnd;
    cxing_stdfp_automode_t *ret;

    AssertArgN(1);
    if( !IsInteger(args[0]) )
    {
        CxingDebug("The rounding mode need to be encoded as an integer.\n");
        return (struct value_nativeobj){
            // @dannyniu is not sure what errno value is appropriate.
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    ret = (cxing_stdfp_automode_t *)s2gc_obj_alloc(
        S2_OBJ_TYPE_CXING_STDFP_AUTOMODE, sizeof *ret);

    if( !ret )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    s2obj_keep(ret->pobj);
    s2obj_release(ret->pobj);
    ret->base.finalf = (s2func_final_t)cxing_stdfp_automode_restoration;
    rnd = args[0].proper.l;
    ret->oldmode = fegetround();

    for(i=0; rounding_mode_constants[i].cfenv >= 0; i++)
    {
        if( rnd == rounding_mode_constants[i].cxing )
        {
            fesetround(rounding_mode_constants[i].cfenv);
            return (struct value_nativeobj){
                .proper.p = ret,
                .type = (const void *)&type_nativeobj_stdfp_automode };
        }
    }

    s2obj_leave(ret->pobj);
    return (struct value_nativeobj){
        // @dannyniu is not sure what errno value is appropriate.
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth };
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

MathBinding_FuncProto(dtoa)
{
    int t;
    s2data_t *x;
    AssertArgN(1);

    t = snprintf(NULL, 0, "%g", ConvertToDouble(args[0]).proper.f);
    if( t < 0 )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    if( !(x = s2data_create(t+1)) )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    t = snprintf(s2data_weakmap(x), t+1, "%g",
                 ConvertToDouble(args[0]).proper.f);
    s2data_trunc(x, t); // should always succeed.

    return (struct value_nativeobj){
        .proper.p = x,
        .type = (const void *)&type_nativeobj_s2impl_str };
}

MathBinding_FuncProto(atod)
{
    AssertArgN(1);
    AssertArgImpl(0, s2impl_str, "string");

    return (struct value_nativeobj){
        .proper.f = atof(s2data_weakmap(args[0].proper.p)),
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
        CxingDebug("Non-integer argument to a **math** function expecting one, "
                   "calculation result may degrade.\n");
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
        CxingDebug("Non-integer argument to a **math** function expecting one, "
                   "calculation result may degrade.\n");
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
        CxingDebug("Non-integer argument to a **math** function expecting one, "
                   "calculation result may degrade.\n");
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
