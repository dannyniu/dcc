src_common="\
cxing-callstub.c
cxing-interp-func-exec.c
cxing-interp-module.c
cxing-interp-misc.c
cxing-grammar.c
expr-arith.c
expr-bits.c
expr-lex.c
expr-unary.c
cxing-stdlib-io.c
langsem.c
runtime.c
runtime-tracing.c
lalr-common/lalr.c
langlex/langlex-cxing.c
lex-common/lex.c
lex-common/rope.c
infra/s2bools.c
infra/strvec.c
infra/rfdict.c
pathutils/pathutils.c
./../contrib/SafeTypes2/src/s2data.c
./../contrib/SafeTypes2/src/s2dict.c
./../contrib/SafeTypes2/src/s2list.c
./../contrib/SafeTypes2/src/s2obj.c
./../contrib/SafeTypes2/src/mem-intercept.c
./../contrib/SafeTypes2/src/siphash.c
./../contrib/librematch/src/librematch.c
./../contrib/librematch/src/regcomp-brackets.c
./../contrib/librematch/src/regcomp-bre.c
./../contrib/librematch/src/regcomp-ere.c
./../contrib/librematch/src/regcomp-interval.c
"

memintercept='-DINTERCEPT_MEM_CALLS'
#memintercept=''

sanitizers='-fsanitize=address'
sanitizers=''

libs_common=''
libs=''

case "$(uname -s)" in
    Linux)
        libs_common=-lm
        ;;

    CYGWIN_NT-*|MINGW*_NT-*)
        libs_common=-lpthread
        ;;
esac

LD_sv="$LD"
LD=ld_add_libs

ld_add_libs() {
    ${LD_sv:-cc} "$@" $libs_common $libs
}