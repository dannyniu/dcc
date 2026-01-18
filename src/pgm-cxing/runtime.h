/* DannyNiu/NJF, 2025-10-02. Public Domain. */

#ifndef cxing2c_runtime_h
#define cxing2c_runtime_h 1

// A few stuff specific to the reference implementation.

#include "value-nativobj.h"

/// @fn
/// @param msg The debug message.
/// @details
/// These messages are from the runtime to
/// assist in debugging of CXING programs.
void CxingDebug(const char *msg, ...);

/// @fn
/// @param msg The diagnostic message.
/// @details
/// Diagnosis are for severe things such as violations of runtime assumptions.
void CxingDiagnose(const char *msg, ...);

/// @fn
/// @param msg The warning message.
/// @details
/// Warnings indicates current or potential future degradation of service.
/// These types of degradation shouldn't be functionally affecting.
void CxingWarning(const char *msg, ...);

/// @fn
/// @param msg The error message.
/// @details
/// Fatal errors prevents the program from correctly functioning.
void CxingFatal(const char *msg, ...);

extern struct value_nativeobj CxingPropName_copy;
extern struct value_nativeobj CxingPropName_final;
extern struct value_nativeobj CxingPropName_equals;
extern struct value_nativeobj CxingPropName_cmpwith;

bool CxingRuntimeInit();
void CxingRuntimeFinal();

#define S2_OBJ_TYPE_CXING_VALUE 0x2112
#define s2_is_cxing_value(obj)                          \
    (((s2obj_t *)obj)->type == S2_OBJ_TYPE_CXING_VALUE)

typedef struct {
    s2obj_base;
    struct value_nativeobj cxing_value;
} s2cxing_value_t;

typedef struct s2cxing_value_iter s2cxing_value_iter_t;

s2cxing_value_t *s2cxing_value_create(struct value_nativeobj val);

typedef struct TYPE_NATIVEOBJ_STRUCT(1) type_nativeobj_struct_p0;
typedef struct TYPE_NATIVEOBJ_STRUCT(7) type_nativeobj_struct_p6;
typedef struct TYPE_NATIVEOBJ_STRUCT(8) type_nativeobj_struct_p7;
typedef struct TYPE_NATIVEOBJ_STRUCT(9) type_nativeobj_struct_p8;

// The "Morgoth" null.
extern const type_nativeobj_struct_p0 type_nativeobj_morgoth;

// The "blessed" null.
extern const type_nativeobj_struct_p0 type_nativeobj_null;

// The arithmetic types.
extern const type_nativeobj_struct_p0 type_nativeobj_long;
extern const type_nativeobj_struct_p0 type_nativeobj_ulong;
extern const type_nativeobj_struct_p0 type_nativeobj_double;

extern const type_nativeobj_struct_p0 type_nativeobj_subr;
extern const type_nativeobj_struct_p0 type_nativeobj_method;

extern const type_nativeobj_struct_p8 type_nativeobj_s2impl_str;
extern const type_nativeobj_struct_p6 type_nativeobj_s2impl_dict;

struct value_nativeobj CxingImpl_s2Dict_Create(
    int argn, struct value_nativeobj args[]);

#endif /* cxing2c_runtime_h */
