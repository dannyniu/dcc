#!/bin/sh

optimize=debug
testfunc()
{
    $exec ../tests/fpcalc-grammar-test-"${variant}".txt
}

cd "$(dirname "$0")"
unitest_sh=../unitest.sh
. $unitest_sh

src="\
fpcalc-grammar-check.c
fpcalc-grammar.c
lalr/lalr.c
lex/langlex.c
lex/lex.c
infra/strvec.c
./../contrib/SafeTypes2/src/s2data.c
./../contrib/SafeTypes2/src/s2obj.c
./../contrib/SafeTypes2/src/mem-intercept.c
./../contrib/SafeTypes2/src/siphash.c
"

cflags_common="\
-D SAFETYPES2_BUILD_WITHOUT_GC
-I ./../src/../contrib/SafeTypes2/src
"

arch_family=defaults
srcset="Plain C"
cflags="-D INTERCEPT_MEM_CALLS"

variant=expr
tests_run

variant=funcs
tests_run

variant=partial
tests_run
