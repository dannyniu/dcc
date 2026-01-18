#!/bin/sh

optimize=debug
testfunc()
{
    #lldb \
       time $exec ../tests/for-loop-test-01.cxing
}

cd "$(dirname "$0")"
unitest_sh=../unitest.sh
. $unitest_sh

src_common="\
lex-common/lex.c
lex-common/rope.c
./../contrib/SafeTypes2/src/s2data.c
./../contrib/SafeTypes2/src/s2obj.c
./../contrib/librematch/src/librematch.c
./../contrib/librematch/src/regcomp-brackets.c
./../contrib/librematch/src/regcomp-bre.c
./../contrib/librematch/src/regcomp-ere.c
./../contrib/librematch/src/regcomp-interval.c
"

cflags_common="\
-D SAFETYPES2_BUILD_WITHOUT_GC
-I ./../src/../contrib/SafeTypes2/src
-I ./../src/../contrib/librematch/src
"

arch_family=defaults

srcset="Plain C for CXING"
src="\
langlex-cxing-check.c
langlex-cxing.c
"

tests_run
