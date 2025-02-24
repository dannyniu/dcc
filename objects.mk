# DannyNiu/NJF, 2023-03-17. Public Domain.

OBJ_SRC_FPCALC = \
    src/infra/strvec.o \
    src/lex/lex.o \
    src/lex/langlex.o \
    src/lalr/lalr.o \
    src/fpcalc/fpcalc.o \
    src/fpcalc/fpcalc-main.o

OBJ_CONTRIB_SAFETYPE2 = \
    contrib/SafeTypes2/src/s2obj.o \
    contrib/SafeTypes2/src/s2data.o \
    contrib/SafeTypes2/src/s2dict.o \
    contrib/SafeTypes2/src/s2list.o \
    contrib/SafeTypes2/src/s2ref.o \
    contrib/SafeTypes2/src/siphash.o

OBJS_GROUP_ALL = ${OBJ_SRC_FPCALC} ${OBJ_CONTRIB_SAFETYPE2}

OBJ_SRC_UNUSED = \
    src/fpcalc/fpcalc-grammar.o \
    src/fpcalc/fpcalc-grammar-simple.o
