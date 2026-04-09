#!/bin/sh
# <CXING-check002(2025-12-30)> #

optimize=debug
testfunc()
{
    #lldb -- \
    time $exec -f ../tests/cxing/loop-breaking-test-01.cxing &&
    time $exec -f ../tests/cxing/conditionals-test-01.cxing &&
    time $exec -f ../tests/cxing/for-loop-test-01.cxing
}

cd "$(dirname "$0")"
unitest_sh=../unitest.sh
. $unitest_sh

. ./cxing-src-common.inc
src="\
validation.001-wet-run.c
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
ldflags="-g $sanitizers -lreadline"

tests_run
