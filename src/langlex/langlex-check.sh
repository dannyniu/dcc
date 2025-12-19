#!/bin/sh

optimize=debug
testfunc()
{
    #lldb \
       time $exec ../tests/langlex-test-src.${plang}
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

export plang=c
srcset="Plain C for C"
src="\
langlex-c-check.c
langlex-c.c
"

tests_run

export plang=cxing
srcset="Plain C for CXING"
src="\
langlex-cxing-check.c
langlex-cxing.c
"

tests_run
