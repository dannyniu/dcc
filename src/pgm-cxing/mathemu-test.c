/* DannyNiu/NJF, 2026-05-05. Public Domain. */

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

double roundeven_emu(double);
#define roundeven(...) roundeven_emu(__VA_ARGS__)

int test_roundeven()
{
    if( roundeven(2.5) != 2 ) return EXIT_FAILURE;
    if( roundeven(3.5) != 4 ) return EXIT_FAILURE;
    if( roundeven(-5.5) != -6 ) return EXIT_FAILURE;
    if( roundeven(-8.5) != -8 ) return EXIT_FAILURE;
    if( roundeven(11.5) != 12 ) return EXIT_FAILURE;
    if( roundeven(12.5) != 12 ) return EXIT_FAILURE;
    if( roundeven(123456789123456789.5) != 123456789123456790.0 )
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

int fpdelta(double a, double b)
{
    if( fabs(a - b) > 0.001 )
        return true;
    else return false;
}
#define fpassert(a, b) if( fpdelta(a,b) ) {             \
        printf("Failure @ line %d.\n", __LINE__);       \
        return EXIT_FAILURE; }

double rootn(double, long long);
int test_rootn()
{
    fpassert( rootn(-8, 3) , -2 );
    fpassert( rootn(-9, 2) , NAN );
    fpassert( rootn(-10, 1), -10 );
    fpassert( rootn(-8, -3), -0.5);
    fpassert( rootn(9, -2), 1.0/3.0);
    fpassert( rootn(32, -5), 0.5);
    fpassert( rootn(144, 2), 12);
    fpassert( rootn(125, 3), 5);
    return EXIT_SUCCESS;
}

typedef int (*testfunc)();
testfunc tests[] = {
    test_roundeven,
    test_rootn,
    NULL,
}, *testp = tests;

int main()
{
    int ret = EXIT_SUCCESS;
    while( *testp && ret == EXIT_SUCCESS )
        ret = (*testp++)();
    return ret;
}
