# DannyNiu/NJF, 2025-12-25. Public Domain.

OBJ_CONTRIB_LIBREMATCH = \
    contrib/librematch/src/regcomp-brackets.o \
    contrib/librematch/src/regcomp-interval.o \
    contrib/librematch/src/regcomp-bre.o \
    contrib/librematch/src/regcomp-ere.o \
    contrib/librematch/src/librematch.o

OBJ_CONTRIB_SAFETYPES2 = \
    contrib/SafeTypes2/src/s2obj.o \
    contrib/SafeTypes2/src/s2data.o \
    contrib/SafeTypes2/src/s2dict.o \
    contrib/SafeTypes2/src/s2list.o \
    contrib/SafeTypes2/src/s2ref.o \
    contrib/SafeTypes2/src/siphash.o

OBJ_SRC_MISC = \
    src/infra/ArcBase.o \
    src/infra/kvtab.o \
    src/infra/rfdict.o \
    src/infra/s2bools.o \
    src/infra/strvec.o \
    src/pathutils/pathutils.o

OBJ_SRC_PARSER_COMMON = \
    src/lex-common/lex.o \
    src/lex-common/rope.o \
    src/lalr-common/lalr.o \

OBJ_SRC_LANGLEX_C = \
    src/langlex/langlex-c.o

OBJ_SRC_PGM_CXING = \
    src/langlex/langlex-cxing.o \
    src/pgm-cxing/cxing-callstub.o \
    src/pgm-cxing/cxing-grammar.o \
    src/pgm-cxing/cxing-interp-func-exec.o \
    src/pgm-cxing/cxing-interp-misc.o \
    src/pgm-cxing/cxing-interp-module.o \
    src/pgm-cxing/cxing-stdlib-fpmath.o \
    src/pgm-cxing/cxing-stdlib-io.o \
    src/pgm-cxing/cxing-stdlib-misc.o \
    src/pgm-cxing/cxing-stdlib-proc.o \
    src/pgm-cxing/cxing-stdlib-regex.o \
    src/pgm-cxing/cxing-stdlib-struct.o \
    src/pgm-cxing/cxing-stdlib-thrd.o \
    src/pgm-cxing/expr-arith.o \
    src/pgm-cxing/expr-bits.o \
    src/pgm-cxing/expr-lex.o \
    src/pgm-cxing/expr-unary.o \
    src/pgm-cxing/langsem.o \
    src/pgm-cxing/mathemu-stubs.o \
    src/pgm-cxing/runtime-array.o \
    src/pgm-cxing/runtime-dict.o \
    src/pgm-cxing/runtime-str.o \
    src/pgm-cxing/runtime-tracing.o \
    src/pgm-cxing/runtime-typeobjs.o \
    src/pgm-cxing/runtime.o \
    src/pgm-cxing/cxing.o

OBJ_SRC_PGM_FPCALC = \
    src/pgm-fpcalc/fpcalc-grammar.o \
    src/pgm-fpcalc/fpcalc-main.o \
    src/pgm-fpcalc/fpcalc.o

OBJ_SRC_UNUSED = \
    src/lalr-common/reduce-reduce-conflict.o \
    src/pgm-fpcalc/fpcalc-grammar-simple.o

OBJ_GROUP_CXING = \
    ${OBJ_CONTRIB_LIBREMATCH} ${OBJ_CONTRIB_SAFETYPES2} \
    ${OBJ_SRC_MISC} ${OBJ_SRC_PARSER_COMMON} ${OBJ_SRC_PGM_CXING}
