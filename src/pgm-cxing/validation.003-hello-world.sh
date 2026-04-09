#!/bin/sh
# <CXING-check004(2026-01-24)> #

optimize=debug
testfunc()
{
    #lldb -- \
    $exec ../tests/cxing/hello-world-01.cxing &&
    $exec ../tests/cxing/hello-world-02.cxing
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
ldflags="-g $sanitizers -lreadline"

tests_run
