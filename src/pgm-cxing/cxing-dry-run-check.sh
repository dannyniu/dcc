#!/bin/sh
# <CXING-check005(2026-02-07)> #

optimize=debug
testfunc()
{
    ! $exec ../tests/dry-run-test-01.cxing
}

cd "$(dirname "$0")"
unitest_sh=../unitest.sh
. $unitest_sh

. ./cxing-src-common.inc
src="\
cxing-ldmod-check.c
"

cflags_common="\
-D SAFETYPES2_BUILD_WITHOUT_GC
-I ./../src/../contrib/SafeTypes2/src
-I ./../src/../contrib/librematch/src
"

arch_family=defaults
srcset="Plain C"
cflags="-D INTERCEPT_MEM_CALLS $sanitizers"
ldflags="$sanitizers"

tests_run
