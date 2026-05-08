/* DannyNiu/NJF, 2026-05-05. Public Domain. */

#define _USE_MATH_DEFINES
#include <math.h>

double roundeven_emu(double x)
{
    register double y = round(x);
    register double d = y-x;
    if( fabs(d) == 0.5 && fabs(fmod(y, 2)) == 1 )
    {
        y += (x-y) * 2;
    }
    return y;
}

double fmax_emu(double a, double b)
{
    if( a > b || b != b ) return a;
    else return b;
}

double fmin_emu(double a, double b)
{
    if( a < b || b != b ) return a;
    else return b;
}

double fpmax_emu(double a, double b)
{
    if( a > b || a != a ) return a;
    else return b;
}

double fpmin_emu(double a, double b)
{
    if( a < b || a != a ) return a;
    else return b;
}

#if __STDC_VERSION__ < 202311L

double sinpi(double x) { return sin(x * M_PI); }
double cospi(double x) { return cos(x * M_PI); }
double tanpi(double x) { return tan(x * M_PI); }

double asinpi(double x) { return asin(x) / M_PI; }
double acospi(double x) { return acos(x) / M_PI; }
double atanpi(double x) { return atan(x) / M_PI; }
double atan2pi(double u, double v) { return atan2(u, v) / M_PI; }

double exp10(double x) { return exp(x*M_LN10); }
double exp2m1(double x) { return expm1(x*M_LN2); }
double exp10m1(double x) { return expm1(x*M_LN10); }

double log2p1(double x) { return log1p(x)/M_LN2; }
double log10p1(double x) { return log1p(x)/M_LN10; }

double compoundn(double x, long long n) { return exp(log1p(x) * n); }

double pown(double x, long long n)
{
    double ret = pow(fabs(x), n);
    if( n&1 ) ret = copysign(ret, x);
    return ret;
}

double rootn(double x, long long n)
{
    if( n&1 )
    {
        return copysign(pow(fabs(x), 1.0/n), x);
    }
    else
    {
        if( x < 0 )
            return sqrt(-1); // generates the invalid operation exception
        else
            return fabs(pow(x, 1.0/n));
    }
}

double rsqrt(double x)
{
    return 1/sqrt(x); // newer CPUs implements specialized instructions.
}

#endif /* Implementation of an older C standard. */
