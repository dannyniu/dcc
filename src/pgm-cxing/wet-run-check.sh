#!/bin/sh

optimize=debug
testfunc()
{
    #lldb -- \
    time $exec -f ../tests/loop-breaking-test-01.cxing &&
    time $exec -f ../tests/conditionals-test-01.cxing &&
    time $exec -f ../tests/for-loop-test-01.cxing
}

cd "$(dirname "$0")"
unitest_sh=../unitest.sh
. $unitest_sh

. ./cxing-src-common.inc
src="\
cxing-exec-01-check.c
"
#src="cxing-grammar-check.c"

cflags_common="\
-D SAFETYPES2_BUILD_WITHOUT_GC
-I ./../src/../contrib/SafeTypes2/src
-I ./../src/../contrib/librematch/src
"

arch_family=defaults
srcset="Plain C"
cflags="-D INTERCEPT_MEM_CALLS -fsanitize=address"
ldflags="-fsanitize=address"

tests_run
