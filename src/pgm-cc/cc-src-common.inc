src_common_cpp="$(. ../cpp-c/cpp-c-src-common.inc ; echo "$src_common")"

src_common="\
c-grammar.c
${src_common_cpp}
"

memintercept='-DINTERCEPT_MEM_CALLS'
#memintercept=''

sanitizers='-fsanitize=address'
#sanitizers=''

libs_common=''
libs=''

case "$(uname -s)" in
    Linux)
        libs_common=-lm
        ;;

    CYGWIN_NT-*|MINGW*_NT-*)
        libs_common="-lpthread -lws2_32"
        ;;
esac

LD_sv="$LD"
LD=ld_add_libs

ld_add_libs() {
    ${LD_sv:-cc} "$@" $libs_common $libs
}