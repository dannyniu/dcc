#!/bin/sh
# <CXING-check007(2026-03-14)> #

optimize=optimize
testfunc()
{
    echo Test Start.
    #lldb -- \
        $exec ../tests/cxing/methods-test-02.cxing &&
        $exec ../tests/cxing/methods-test-01.cxing &&
        $exec ../tests/cxing/for-loop-test-01.cxing
}

cd "$(dirname "$0")"
unitest_sh=../unitest.sh
. $unitest_sh

. ./cxing-src-common.inc
src="\
validation.util-hello-world.c
"

cflags_common="\
-U SAFETYPES2_BUILD_WITHOUT_GC
-I ./../src/../contrib/SafeTypes2/src
-I ./../src/../contrib/librematch/src
$memintercept
"

arch_family=defaults
srcset="Plain C"
cflags="$sanitizers" #' -D DCC_LALR_LOGGING'
ldflags="-g $sanitizers"

tests_run
