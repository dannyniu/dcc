#!/bin/sh

optimize=debug
testfunc()
{
    #leaks --atExit -- \
        $exec -f ../tests/dcc-preproc/stdtorture.c &&
            : $exec -f ../tests/dcc-preproc/rescan.c &&
            : $exec -f ../tests/dcc-preproc/stdexam.c
}

cd "$(dirname "$0")"
unitest_sh=../unitest.sh
. $unitest_sh

. ./cpp-c-src-common.inc
src="\
validation.001-macros-postulates-check.c
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
