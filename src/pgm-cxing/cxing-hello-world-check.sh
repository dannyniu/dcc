#!/bin/sh

optimize=debug
testfunc()
{
    #lldb -- \
    $exec ../tests/hello-world-01.cxing &&
    $exec ../tests/hello-world-02.cxing
}

cd "$(dirname "$0")"
unitest_sh=../unitest.sh
. $unitest_sh

. ./cxing-src-common.inc
src="\
cxing-hello-world-check.c
"

cflags_common="\
-D SAFETYPES2_BUILD_WITHOUT_GC
-I ./../src/../contrib/SafeTypes2/src
-I ./../src/../contrib/librematch/src
"

arch_family=defaults
srcset="Plain C"
cflags="-D INTERCEPT_MEM_CALLS -fsanitize=address"
ldflags="-fsanitize=address -lreadline"

tests_run
