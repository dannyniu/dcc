#!/bin/sh

#CC='clang-mp-21'
#cflags_common='-fprofile-instr-generate'
#ldflags_common='-fprofile-instr-generate'

optimize=true
testfunc()
{
    echo parsing starts.
    #export LLVM_PROFILE_FILE="$HOME/deleteme.instrprof"
    #lldb -- \
    #time $exec -f ../tests/langlex-test-src.cxing
    time $exec -f ../tests/phrase-experiment.cxing
}

cd "$(dirname "$0")"
unitest_sh=../unitest.sh
. $unitest_sh

. ./cxing-src-common.inc
src="\
cxing-grammar-check.c
"

cflags_common="\
-D SAFETYPES2_BUILD_WITHOUT_GC
-I ./../src/../contrib/SafeTypes2/src
-I ./../src/../contrib/librematch/src
"

arch_family=defaults
srcset="Plain C"
#cflags="-D INTERCEPT_MEM_CALLS"

tests_run
