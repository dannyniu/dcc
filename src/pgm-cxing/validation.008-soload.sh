#!/bin/sh

optimize=optimize
testfunc()
{
    echo Building Stub DLL.
    ${CC:-$target_cc} -c $cflags_proj -fPIC ../src/pgm-cxing/dll-stub.c
    
    case "$(uname -s)" in
        Darwin)
            cc -o dll-stub.so -Xlinker -dylib dll-stub.o runtime-typeobjs.o
            ;;
        
        Linux)
            ${LD:-$target_ld} -o dll-stub.so -shared -s dll-stub.o runtime-typeobjs.o
            ;;

        CYGWIN_NT-*|MINGW*_NT-*)
            cc -o dll-stub.so -mdll dll-stub.o runtime-typeobjs.o
            ;;

        *)
            echo Unrecognized operating system, not sure how to build DLL.
            return 1
            ;;
    esac

    echo Test Start.
    #lldb \
        $exec ../tests/cxing/soload-01.cxing
}

cd "$(dirname "$0")"
unitest_sh=../unitest.sh
. $unitest_sh

. ./cxing-src-common.inc
src="\
validation.util-main-exec.c
"

cflags_common="\
-U SAFETYPES2_BUILD_WITHOUT_GC
-I ./../src/../contrib/SafeTypes2/src
-I ./../src/../contrib/librematch/src
$memintercept
"

arch_family=defaults
srcset="Plain C"
cflags="$sanitizers" #' -D DCC_LALR_LOGGING'
ldflags="-g $sanitizers"

tests_run
