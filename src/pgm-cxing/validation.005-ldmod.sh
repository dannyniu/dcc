#!/bin/sh
# <CXING-check006(2026-02-28)> #

optimize=debug
testfunc()
{
    #lldb -- \
      #$exec ../tests/cxing/auto-res-man-test-01.cxing
      $exec ../tests/cxing/xlate-unit-test-01.cxing
      $exec ../tests/cxing/xlate-unit-test-03.cxing
}

cd "$(dirname "$0")"
unitest_sh=../unitest.sh
. $unitest_sh

. ./cxing-src-common.inc
src="\
validation.util-ldmod.c
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
