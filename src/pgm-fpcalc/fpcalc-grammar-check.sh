#!/bin/sh

optimize=debug
testfunc()
{
    #lldb -- \
        $exec -f ../tests/fpcalc-grammar-test-"${variant}".txt
}

cd "$(dirname "$0")"
unitest_sh=../unitest.sh
. $unitest_sh

. ./fpcalc-src-common.inc
src="\
fpcalc-grammar-check.c
fpcalc-grammar.c
"

cflags_common="\
-D SAFETYPES2_BUILD_WITHOUT_GC
-I ./../src/../contrib/SafeTypes2/src
-I ./../src/../contrib/librematch/src
"

arch_family=defaults
srcset="Plain C"
cflags="-D INTERCEPT_MEM_CALLS"

variant=expr
tests_run

variant=funcs
tests_run

variant=partial
tests_run
