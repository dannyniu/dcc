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

/// @def
/// @brief non-fatal assertion that number of arguments are sufficient.
#define AssertArgN(n)                                           \
    if( argn < n ) return (struct value_nativeobj){             \
            .proper.p = NULL,                                   \
            .type = (const void *)&type_nativeobj_morgoth };

/// @def
/// @brief non-fatal assertion that implementation is as expected.
#define AssertArgImpl(n, typ, hr)                                       \
    if( args[n].type != (const void *)&type_nativeobj_##typ )           \
    {                                                                   \
        CxingDebug("Encountered unrecognized implementation of "        \
                   "the "hr" type in arg%d in function `%s`. "          \
                   "Does this object come from another runtime?\n",     \
                   n, __func__);                                        \
        return (struct value_nativeobj){                                \
            .proper.p = NULL,                                           \
            .type = (const void *)&type_nativeobj_morgoth };            \
    }

/// @def
/// @brief non-fatal assertion that implementation is of expected.
#define AssertArgImpls(n, acceptances, hr) do {                         \
    acceptances;                                                        \
    CxingDebug("Encountered unexpected implementation of "              \
               "an "hr" instance in arg%d in function `%s`.\n",         \
               n, __func__);                                            \
    return (struct value_nativeobj){                                    \
        .proper.p = NULL,                                               \
        .type = (const void *)&type_nativeobj_morgoth };                \
    } while( false )

/// @def
/// @brief one of the `acceptances` clauses.
#define AcceptArgImpl(n, typ)                                           \
    if( args[n].type == (const void *)&type_nativeobj_##typ ) break;

// A binding of SafeTypes2 objects for the CXING language.
struct value_nativeobj CxingImpl_s2Obj_Copy(
    int argn, struct value_nativeobj args[]);
struct value_nativeobj CxingImpl_s2Obj_Final(
    int argn, struct value_nativeobj args[]);

extern struct value_nativeobj CxingPropName_copy;
extern struct value_nativeobj CxingPropName_final;
extern struct value_nativeobj CxingPropName_equals;
extern struct value_nativeobj CxingPropName_cmpwith;
extern struct value_nativeobj CxingPropName_InitSet;
extern struct value_nativeobj CxingPropName_Proto;

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
typedef struct TYPE_NATIVEOBJ_STRUCT(4) type_nativeobj_struct_p3;
typedef struct TYPE_NATIVEOBJ_STRUCT(7) type_nativeobj_struct_p6;
typedef struct TYPE_NATIVEOBJ_STRUCT(8) type_nativeobj_struct_p7;
typedef struct TYPE_NATIVEOBJ_STRUCT(9) type_nativeobj_struct_p8;
typedef struct TYPE_NATIVEOBJ_STRUCT(10) type_nativeobj_struct_p9;
typedef struct TYPE_NATIVEOBJ_STRUCT(11) type_nativeobj_struct_p10;

// The "Morgoth" null.
extern const type_nativeobj_struct_p0 type_nativeobj_morgoth;

// The "blessed" null.
extern const type_nativeobj_struct_p0 type_nativeobj_null;

// The arithmetic types.
extern const type_nativeobj_struct_p0 type_nativeobj_long;
extern const type_nativeobj_struct_p0 type_nativeobj_ulong;
extern const type_nativeobj_struct_p0 type_nativeobj_double;

// Functions.
extern const type_nativeobj_struct_p0 type_nativeobj_subr;
extern const type_nativeobj_struct_p0 type_nativeobj_method;

// Built-in objects.
extern const type_nativeobj_struct_p9 type_nativeobj_s2impl_str;
extern const type_nativeobj_struct_p8 type_nativeobj_s2impl_dict;

struct value_nativeobj CxingImpl_s2Dict_Create(
    int argn, struct value_nativeobj args[]);

// Table entry type for runtime and 3rd party libraries to
// inject built-in definitions into the language environment,
// by iterating over the table, and setting the definition
// into a 'global' hash table.
typedef struct {
    const char *name;

    // Among the 3 alternatives for 'global' built-ins:
    // 1. use actual **atomic** reference counts,
    // 2. not having the `__copy__` and the `__final__` methods,
    // 3. do actual copies (over immutable internal data structures),
    // Option 2 is preferred.
    struct value_nativeobj val;
} cxing_builtin_def_t;

// Used by module loader.
extern s2dict_t *CxingBuiltins;

/// @fn
/// @param name the identifier to inject the definition as,
/// @param value the value of the definition to inject.
/// @returns 0 on success, and -1 on error.
/// @detils
/// Entities injected using this function are exposed globally in all modules.
/// Calling this function while some module is executing risks unpredictable
/// behavior, especially in multi-threaded applications.
int CxingBuiltinsExtend(const char *name, struct value_nativeobj value);

#endif /* cxing2c_runtime_h */
