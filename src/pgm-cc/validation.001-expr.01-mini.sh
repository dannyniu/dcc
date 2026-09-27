#!/bin/sh

#CC='clang-mp-21'
#cflags_common='-fprofile-instr-generate'
#ldflags_common='-fprofile-instr-generate'

optimize=debug
testfunc()
{
    echo parsing starts.
    #export LLVM_PROFILE_FILE="$HOME/deleteme.instrprof"
    #lldb \
        #leaks -atExit -- \
              $exec ../tests/cc-text-scalar-types/001-expr-ret.c
}

cd "$(dirname "$0")"
unitest_sh=../unitest.sh
. $unitest_sh

. ./cc-src-common.inc
src="\
validation.001-expr.01-mini.c
"

cflags_common="\
-D SAFETYPES2_BUILD_WITH_GC
-I ./../src/../contrib/SafeTypes2/src
-I ./../src/../contrib/librematch/src
"

arch_family=defaults
srcset="Plain C"
cflags="-D INTERCEPT_MEM_CALLS $sanitizers"
ldflags="$sanitizers"

if [ $EXPAND_SRC = yes ] ; then
cc -E -DNDEBUG $cflags_common c-semantics-xhale.c ; else
tests_run ; fi
