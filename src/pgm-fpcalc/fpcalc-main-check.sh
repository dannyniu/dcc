#!/bin/sh

optimize=debug
testfunc()
{
    #lldb \
        $exec <<EOF
f(x) = 2*x
g(x) = 3*f(x)
g(1)
EOF
}

cd "$(dirname "$0")"
unitest_sh=../unitest.sh
. $unitest_sh

. ./fpcalc-src-common.inc
src="\
fpcalc-main.c
fpcalc.c
fpcalc-grammar.c
./../contrib/SafeTypes2/src/s2dict.c
"

cflags_common="\
-D SAFETYPES2_BUILD_WITHOUT_GC
-I ./../src/../contrib/SafeTypes2/src
-I ./../src/../contrib/librematch/src
"
ldflags_common="-lreadline"

arch_family=defaults
srcset="Plain C"
cflags="-D INTERCEPT_MEM_CALLS"

tests_run
