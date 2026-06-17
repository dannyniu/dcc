/* DannyNiu/NJF, 2026-01-24. Public Domain. */

#ifndef dcc_cxing_stdlib_h
#define dcc_cxing_stdlib_h 1

#include "langsem.h"
#include "runtime.h"

struct value_nativeobj CxingStdlibFunc_Print(
    int argn, struct value_nativeobj args[]);

struct value_nativeobj CxingStdlibFunc_Input(
    int argn, struct value_nativeobj args[]);

extern const type_nativeobj_struct_p10 type_nativeobj_RegFile;
extern const type_nativeobj_struct_p9 type_nativeobj_Pipe;
extern const type_nativeobj_struct_p3 type_nativeobj_PipeEnds;
extern const type_nativeobj_struct_p4 type_nativeobj_DIR;

extern const type_nativeobj_struct_p6 type_nativeobj_Regex;

extern const type_nativeobj_struct_p13 type_nativeobj_CmdInterp;
extern const type_nativeobj_struct_p8 type_nativeobj_ProcHndl;

extern cxing_builtin_def_t CxingStdlibStructBuiltins[];
extern cxing_builtin_def_t CxingStdlibIoBuiltins[];
extern cxing_builtin_def_t CxingStdlibMathBuiltins[];
extern cxing_builtin_def_t CxingStdlibRegexBuiltins[];
extern cxing_builtin_def_t CxingStdlibProcBuiltins[];
int CxingInitialization_DefineStandardLibrary();

// Error Number Name Space.
// ====
// 
// For error numbers other than `errno`, the identifier parameter
// named by the `ident` argument evaluates to the error number,
// which is casted into a `null`. If provided, the variadic arguments
// provide necessary argument to call `ident` as a function.
#define CXErrNS(ident, ...) ident __VA_ARGS__ + CXEnsCookie(#ident)

// Equals `(int64_t)(0x7fffffff & (CRC32(s) | 0x40000000)) << 32`.
// where CRC32 is as defined in ISO/IEC 8802-3:1996.
// Note the POSIX "cksum" utility computes the same function,
// and the web version of the standard contains a free-to-access
// demo code in C that demonstrates the algorithm.
int64_t CXEnsCookie(const char *s);

#endif // dcc_cxing_stdlib_h
