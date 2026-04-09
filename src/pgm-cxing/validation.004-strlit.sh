#!/bin/sh
# <CXING-check005(2026-01-24)> #

optimize=debug
testfunc()
{
    #lldb -- \
      $exec <<\EOF
"Hello World!\t"
"This is a \"\e[42;33mGreen Text\e[0m\"\n"
"This is another piece of \033[42;33mGreen\033[0m Text.\x0a - End Of Output. -\n"
EOF
}

cd "$(dirname "$0")"
unitest_sh=../unitest.sh
. $unitest_sh

. ./cxing-src-common.inc
src="\
validation.004-strlit.c
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
