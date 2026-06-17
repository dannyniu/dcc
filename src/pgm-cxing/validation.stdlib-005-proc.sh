#!/bin/sh

optimize=debug
: ${inst:=}
testfunc()
{
    echo Test Start. &&
        $exec ../tests/cxing-stdlib/proc-01.cxing &&
        $exec ../tests/cxing-stdlib/proc-02.cxing &&
        $exec ../tests/cxing-stdlib/proc-03a.cxing &&
        $exec ../tests/cxing-stdlib/proc-03b.cxing || echo $?
}

cd "$(dirname "$0")"
unitest_sh=../unitest.sh
. $unitest_sh

. ./cxing-src-common.inc
src="\
validation.util-main-exec.c
"

cflags_common="\
-U SAFETYPES2_BUILD_WITHOUT_GC -D ${VerboseCompile:=QuietCompile}
-I ./../src/../contrib/SafeTypes2/src
-I ./../src/../contrib/librematch/src
$memintercept
"

arch_family=defaults
srcset="Plain C"
cflags="$sanitizers" #' -D DCC_LALR_LOGGING'
ldflags="-g $sanitizers"

tests_run
