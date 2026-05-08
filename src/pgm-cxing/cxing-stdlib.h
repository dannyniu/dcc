/* DannyNiu/NJF, 2026-01-24. Public Domain. */

#include "langsem.h"
#include "runtime.h"

struct value_nativeobj CxingStdlibFunc_Print(
    int argn, struct value_nativeobj args[]);

struct value_nativeobj CxingStdlibFunc_Input(
    int argn, struct value_nativeobj args[]);

extern const type_nativeobj_struct_p9 type_nativeobj_RegFile;
extern const type_nativeobj_struct_p8 type_nativeobj_Pipe;
const type_nativeobj_struct_p3 type_nativeobj_PipeEnds;

struct value_nativeobj CxingImpl_RegFile_Open(
    int argn, struct value_nativeobj args[]);

struct value_nativeobj CxingImpl_Pipe_Create(
    int argn, struct value_nativeobj args[]);

#define CxingMathBinding(funcname)                      \
    struct value_nativeobj CxingMath_##funcname(        \
        int argn, struct value_nativeobj args[]);

// Already declared in C,
// They're needed to expose to CXING.
#define CxingMathConstants(constname)
#define CxingMathEnums(enumname)

#include "cxing-math-bindings.inc"
#undef CxingMathBinding
#undef CxingMathConstants
#undef CxingMathEnums

extern cxing_builtin_def_t CxingStdlibIoBuiltins[];
extern cxing_builtin_def_t CxingStdlibMathBuiltins[];
int CxingInitialization_DefineStandardLibrary();
