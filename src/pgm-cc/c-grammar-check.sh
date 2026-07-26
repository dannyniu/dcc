#!/bin/sh
# <c-check001(2026-07-19)> #

#CC='clang-mp-21'
#cflags_common='-fprofile-instr-generate'
#ldflags_common='-fprofile-instr-generate'

optimize=debug
testfunc()
{
    echo parsing starts.
    #export LLVM_PROFILE_FILE="$HOME/deleteme.instrprof"
    #lldb \-
        $exec ../tests/cc-text-scalar-types/001-expr-ret.c
}

cd "$(dirname "$0")"
unitest_sh=../unitest.sh
. $unitest_sh

. ./cc-src-common.inc
src="\
c-grammar-check.c
"

cflags_common="\
-D SAFETYPES2_BUILD_WITH_GC
-I ./../src/../contrib/SafeTypes2/src
-I ./../src/../contrib/librematch/src
"

arch_family=defaults
srcset="Plain C"
cflags="-D INTERCEPT_MEM_CALLS"

tests_run
