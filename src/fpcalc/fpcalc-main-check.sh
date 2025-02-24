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

src="\
fpcalc-main.c
fpcalc.c
lalr/lalr.c
lex/langlex.c
lex/lex.c
infra/strvec.c
./../contrib/SafeTypes2/src/s2dict.c
./../contrib/SafeTypes2/src/s2data.c
./../contrib/SafeTypes2/src/s2obj.c
./../contrib/SafeTypes2/src/siphash.c
./../contrib/SafeTypes2/src/mem-intercept.c
"

cflags_common="\
-D SAFETYPES2_BUILD_WITHOUT_GC
-I ./../src/../contrib/SafeTypes2/src
"

arch_family=defaults
srcset="Plain C"
cflags="-D INTERCEPT_MEM_CALLS"

tests_run
