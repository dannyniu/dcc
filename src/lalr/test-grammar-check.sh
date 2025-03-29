#!/bin/sh

# 2025-03-29:
# this check verifies whether the current parser can handle
# reduce-reduce conflict, and the result shows that it can't.

optimize=debug
testfunc()
{
    $exec -e 'f(x), g(x,(a,b),y) + 3*4'
}

cd "$(dirname "$0")"
unitest_sh=../unitest.sh
. $unitest_sh

src="\
test-grammar-check.c
test-grammar.c
lalr/lalr.c
lex/shifter.c
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
