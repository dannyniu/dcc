/* DannyNiu/NJF, 2026-01-24. Public Domain. */

#ifndef dcc_cxing_stdlib_h
#define dcc_cxing_stdlib_h 1

#include "langsem.h"
#include "runtime.h"

struct value_nativeobj CxingStdlibFunc_Print(
    int argn, struct value_nativeobj args[]);

struct value_nativeobj CxingStdlibFunc_Input(
    int argn, struct value_nativeobj args[]);

extern const type_nativeobj_struct_p9 type_nativeobj_RegFile;
extern const type_nativeobj_struct_p8 type_nativeobj_Pipe;
extern const type_nativeobj_struct_p3 type_nativeobj_PipeEnds;
extern const type_nativeobj_struct_p4 type_nativeobj_DIR;

extern const type_nativeobj_struct_p6 type_nativeobj_Regex;

extern cxing_builtin_def_t CxingStdlibStructBuiltins[];
extern cxing_builtin_def_t CxingStdlibIoBuiltins[];
extern cxing_builtin_def_t CxingStdlibMathBuiltins[];
extern cxing_builtin_def_t CxingStdlibRegexBuiltins[];
int CxingInitialization_DefineStandardLibrary();

#endif // dcc_cxing_stdlib_h
