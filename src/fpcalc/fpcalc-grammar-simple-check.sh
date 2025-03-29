#!/bin/sh

optimize=debug
testfunc()
{
    $exec -f ../tests/fpcalc-grammar-test-expr.txt
    #$exec -f ../tests/fpcalc-grammar-test-funcs.txt
    #$exec -f ../tests/fpcalc-grammar-test-partial.txt
}

cd "$(dirname "$0")"
unitest_sh=../unitest.sh
. $unitest_sh

src="\
fpcalc-grammar-check.c
fpcalc-grammar-simple.c
lalr/lalr.c
lex/shifter.c
lex/langlex.c
lex/lex.c
infra/strvec.c
./../contrib/SafeTypes2/src/s2data.c
./../contrib/SafeTypes2/src/s2obj.c
"

cflags_common="\
-D SAFETYPES2_BUILD_WITHOUT_GC
-I ./../src/../contrib/SafeTypes2/src
"

arch_family=defaults
srcset="Plain C"

tests_run
