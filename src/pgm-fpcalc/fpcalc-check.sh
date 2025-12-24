#!/bin/sh

optimize=debug
testfunc()
{
    #lldb \
        $exec
}

cd "$(dirname "$0")"
unitest_sh=../unitest.sh
. $unitest_sh

. ./fpcalc-src-common.inc
src="\
fpcalc-check.c
fpcalc.c
fpcalc-grammar.c
"

cflags_common="\
-D SAFETYPES2_BUILD_WITHOUT_GC
"

arch_family=defaults
srcset="Plain C"
cflags="-D INTERCEPT_MEM_CALLS"

tests_run
