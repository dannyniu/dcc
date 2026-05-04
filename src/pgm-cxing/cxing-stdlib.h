/* DannyNiu/NJF, 2026-01-24. Public Domain. */

#include "langsem.h"
#include "runtime.h"

struct value_nativeobj CxingStdlibFunc_Print(
    int argn, struct value_nativeobj args[]);

struct value_nativeobj CxingStdlibFunc_Input(
    int argn, struct value_nativeobj args[]);

extern const type_nativeobj_struct_p10 type_nativeobj_RegFile;

struct value_nativeobj CxingImpl_RegFile_Open(
    int argn, struct value_nativeobj args[]);
