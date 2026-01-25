#!/bin/sh

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
cxing-strlit-check.c
"

cflags_common="\
-D SAFETYPES2_BUILD_WITHOUT_GC
-I ./../src/../contrib/SafeTypes2/src
-I ./../src/../contrib/librematch/src
"

arch_family=defaults
srcset="Plain C"
cflags="-D INTERCEPT_MEM_CALLS -fsanitize=address"
ldflags="-fsanitize=address -lreadline"

tests_run
