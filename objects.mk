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

OBJ_SRC_PARSER_COMMON = \
    src/lex-common/lex.o \
    src/lex-common/rope.o \
    src/lalr-common/lalr.o \
    src/infra/strvec.o

OBJ_SRC_LANGLEX_C = \
    src/langlex/langlex-c.o

OBJ_SRC_PGM_CXING = \
    src/langlex/langlex-cxing.o \
    src/pgm-cxing/cxing-grammar.o \
    src/pgm-cxing/cxing-interp.o \
    src/pgm-cxing/expr-arith.o \
    src/pgm-cxing/expr-bits.o \
    src/pgm-cxing/expr-unary.o \
    src/pgm-cxing/langsem.o \
    src/pgm-cxing/runtime.o

OBJ_SRC_PGM_FPCALC = \
    src/pgm-fpcalc/fpcalc-grammar.o \
    src/pgm-fpcalc/fpcalc-main.o \
    src/pgm-fpcalc/fpcalc.o

OBJ_SRC_UNUSED = \
    src/lalr-common/reduce-reduce-conflict.o \
    src/pgm-fpcalc/fpcalc-grammar-simple.o

OBJ_GROUP_ALL = \
    OBJ_CONTRIB_SAFETYPES2
