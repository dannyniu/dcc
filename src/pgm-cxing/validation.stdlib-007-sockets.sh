#!/bin/sh

optimize=debug
: ${inst:=}
testfunc()
{
    delay() { { sleep 5 ; "$@" ; } & }
    echo Test Start.
    $exec ../tests/cxing-stdlib/sockets-03.cxing ; return
        $exec ../tests/cxing-stdlib/sockets-02.cxing &&
        { delay curl http://localhost:8080
          $exec ../tests/cxing-stdlib/sockets-01.cxing ; }
}

cd "$(dirname "$0")"
unitest_sh=../unitest.sh
. $unitest_sh

. ./cxing-src-common.inc
src="\
validation.util-main-exec.c
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
