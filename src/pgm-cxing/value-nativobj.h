/* DannyNiu/NJF, 2025-10-02. Public Domain. */

#ifndef cxing_value_nativobj_h
#define cxing_value_nativobj_h 1

#include "../common.h"
#include <SafeTypes2.h>

enum types_enum {
    valtyp_null = 0,
    valtyp_long,
    valtyp_ulong,
    valtyp_double,

    // the opaque object type.
    valtyp_obj,

    // `porper.p` points to a `struct value_nativeobj`.
    //
    // 2025-10-02 implementation note:
    // Currently, only function arguments may receive pointers to value
    // native objects, variables are not having this type at the moment.
    //
    valtyp_ref,

    // FFI and non-FFI subroutines and methods.
    valtyp_subr = 6,
    valtyp_method,
    valtyp_ffisubr,
    valtyp_ffimethod,

    // 10 types so far.
};

struct value_nativeobj;
struct type_nativeobj;

struct value_nativeobj {
    union { double f; int64_t l; uint64_t u; void *p; } proper;
    union {
        const struct type_nativeobj *type;
        uint64_t pad; // zero-extend the type pointer to 64-bit on ILP32 ABIs.
    };
};

struct lvalue_nativeobj {
    struct value_nativeobj value;

    // The following fields are for lvalues:

    // 2026-01-01:
    // because different kind of scopes needs different accessors,
    // a mere pointer to the scope is not enough - it needs
    // accessor properties, therefore this is changed to 
    // a value native object.
    struct value_nativeobj scope;
    
    // 2026-01-01:
    // the reference implementation uses `s2data_t` from the SafeTypes2
    // library, other implementations may have a different choice,
    // barring binary compatibility and interoperability issues.
    void *key; // if null, then it's not an lvalue irrespective of scope.
};

// There are `n + 1` elements in `static_members`, last of which `type` being
// the only `NULL` entry in the array.
#define TYPE_NATIVEOBJ_STRUCT(...) {                    \
        uint64_t typeid;                                \
        uint64_t n_entries;                             \
        struct {                                        \
            const char *name;                           \
            const struct value_nativeobj *member;       \
        } static_members[__VA_ARGS__];                  \
    }

struct type_nativeobj TYPE_NATIVEOBJ_STRUCT();

#endif /* cxing_value_nativobj_h */
