#!/bin/sh

optimize=debug
testfunc()
{
    $exec -f "$(realpath ../tests/fpcalc-grammar-test-expr.txt)"
    #$exec -f "$(realpath ../tests/fpcalc-grammar-test-funcs.txt)"
    #$exec -f "$(realpath ../tests/fpcalc-grammar-test-partial.txt)"
}

cd "$(dirname "$0")"
unitest_sh=../unitest.sh
. $unitest_sh

. ./fpcalc-src-common.inc
src="\
fpcalc-grammar-check.c
fpcalc-grammar-simple.c
"

cflags_common="\
-D SAFETYPES2_BUILD_WITHOUT_GC
"

arch_family=defaults
srcset="Plain C"
cflags="-D INTERCEPT_MEM_CALLS"

tests_run
