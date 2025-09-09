# DannyNiu/NJF, 2024-07-27. Public Domain.

LibraryName = None
ProductName = fpcalc
MajorVer = 0
MinorVer = 1
ProductVer = ${MajorVer}.${MinorVer}
ProductRev = ${ProductVer}.0

FILE_EXT_ELF = so.${ProductVer}
FILE_EXT_MACHO = ${ProductVer}.dylib

FILE_EXT = ${FILE_EXT_MACHO}

ccOpts = -Wall -Wextra -fPIC # If I guessed wrong, specify on command line.
LD = ${CC} # 2024-03-09: direct linker invocation lacks some default flags.

# ``-G'' is the System V and XPG-8/SUSv5 option for producing
# dynamic-linking library. Will need adaptation for pre-existing linkers.
DLLFLAGS = -G

OBJS_GROUP_WITH_ADDITION =
CFLAGS_GROUP_WITH =

INPUT_OBJECTS = ${OBJS_GROUP_ALL} ${OBJS_GROUP_WITH_ADDITION}
CFLAGS = ${ccOpts} ${CFLAGS_GROUP_WITH} `cat compile_flags.txt`

prefix = /usr/local
exec_prefix = ${prefix}
bindir = ${exec_prefix}/bin

# 2022-12-30:
# Each product consist of:
# - Name - obviously,
# - Versioning - semver, *.so.{ver} on ELF, *.{ver}.dylib on Mach-O
