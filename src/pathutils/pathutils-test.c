/* DannyNiu/NJF, 2026-03-08. Public Domain. */

#include "pathutils.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
    assert( argc > 2 );
    printf("%s\n", PathReplaceBasename(argv[1], argv[2]));
    return EXIT_SUCCESS;
}
