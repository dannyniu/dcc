# DannyNiu/NJF, 2025-12-25. Public Domain.

## basic information. ##

ProgramName = fpcalc
ProductName = ${ProgramName}
MajorVer = 0
MinorVer = 1
ProductVer = ${MajorVer}.${MinorVer}
ProductRev = ${ProductVer}.0

## target product. ##

FILE_EXT_ELF = ""
FILE_EXT_MACHO = ""
FILE_EXT_PECOFF = "exe"

FILE_EXT = ${FILE_EXT_MACHO}

ccOpts = -Wall -Wextra # If I guessed wrong, specify on command line.
LD = ${CC} # 2024-03-09: direct linker invocation lacks some default flags.

## object files. ##

include objects.mk

OBJS_GROUP_WITH_ADDITION =
CFLAGS_GROUP_WITH = `cat compile_flags.txt`

INPUT_OBJECTS = \
    ${OBJ_CONTRIB_LIBREMATCH} \
    ${OBJ_CONTRIB_SAFETYPES2} \
    ${OBJ_SRC_PARSER_COMMON} \
    ${OBJ_SRC_LANGLEX_C} \
    ${OBJ_SRC_PGM_FPCALC} \
    ${OBJS_GROUP_WITH_ADDITION}

CFLAGS = ${ccOpts} ${CFLAGS_GROUP_WITH}
LDFLAGS = -lreadline
