/* DannyNiu/NJF, 2026-08-10. Public Domain. */

// OMEGA - The ABI Oracle.

#ifndef dcc_omega_h
#define dcc_omega_h 1

#include "../lalr-common/lalr.h"
#include "mini.h"

// Can be embedded onto the data buffer of an `s2data_t` object.
typedef struct {
    // the actual size of this structure.
    size_t desc_size;

    // the size and alignment (in bytes) of
    // the type described by this structure.
    long size, align;

    // Other ABI-defined info.
} omega_type_desc_t;

// information regarding function call arguments and return result.
typedef struct {
    // The actual argument value is in RAM,
    // and is pointed to by this substituted pointer.
    int pointer_substituted;

    // 0 means it's not passed in register.
    regid_t regid;

    // (positive?) offset from the stack pointer at the time of the call.
    int32_t sp_offset;
} omega_operand_info_t;

// used by the mental picture of the compiler
// to determine the placement of arguments.
//
// ought to be saved on the AST/DAG node of the function call expression,
// perhaps wrapped in an `s2ref_t`.
typedef struct omega_func_args_placer omega_func_args_placer_t;

struct omega_func_args_placer
{
    int (*supply_prototype)(
        omega_func_args_placer_t *ctx,
        lalr_prod_t *declarator);

    int (*query_placement_for_result)(
        omega_func_args_placer_t *ctx,
        omega_operand_info_t *opinfo);

    int (*query_placement_for_nextarg)(
        omega_func_args_placer_t *ctx,
        omega_operand_info_t *opinfo);

    // Other members defined by ABI.
};

// 2026-08-14:
// Operations on register file that helps register allocation.
// When a reload is determined as necessary elsewhere (e.g. according to
// the declaration info in the scoped declarations list), these interfaces
// are consulted for where to load the datum into.
typedef struct omega_register_allocator omega_register_allocator_t;

typedef enum {
    // These are bitmask flags.
    // ABI-specific set(s) start from the 6th bit - i.e. decimal value 64.
    omega_regset_all = -1,
    omega_regset_default = 0,

    // By default, they're neither cleared by `clearallmarks`,
    // thus nor touched by while generating machine code.
    // This is changed between runs of code-gen to determine
    // whether it's better to save those registers to amortize
    // the cost of spilling.  As such, specifying any of the
    // following values, enables the respective sets of registers
    // to be used, and their save/restore sequence to be generated.
    omega_regset_callee_saved_GPR = 1,
    omega_regset_callee_saved_FPR = 2, // Recognized also as SIMD for many ABIs.

    // The sets that could be invalidated across function calls, unless a
    // function has no argument, or a function with only integer arguments
    // calls a function with only floating-point arguments (or vice versa).
    omega_regset_args_GPR = 4,
    omega_regset_args_FPR = 8,
} omega_regset_t;

typedef void (*omega_initregalloc_t)(
    omega_register_allocator_t *ctx, omega_regset_t regset);

// returns 0 if not found in the register file.
typedef regid_t (*omega_cookie2regid_t)(
    omega_register_allocator_t *ctx, mInstr_t *dish);

// returns all-zero cookie if not found in the register file.
typedef cookie_t (*omega_getregcookie_t)(
    omega_register_allocator_t *ctx, regid_t regid);

// returns one of `s2_access_{success,nullval,error}`.
typedef int (*omega_setregcookie_t)(
    omega_register_allocator_t *ctx, regid_t regid, mInstr_t *dish);

// marks the register as unavailable.
typedef void (*omega_markregister_t)(
    omega_register_allocator_t *ctx, regid_t regid);

// retrieves an available (i.e. unmarked) register.
typedef regid_t (*omega_getavailable_t)(
    omega_register_allocator_t *ctx, mInstr_t *recipient);

// clear registers' marks:
// If `regset` is 0, then only temporary (i.e. caller-saved) registers
// are cleared. To clear all sets of registers, specify all-ones (i.e. -1).
typedef void (*omega_clearallmarks_t)(
    omega_register_allocator_t *ctx, omega_regset_t regset);

// Computes the intersection of 2 register sets: Corresponding registers
// with the same cookie value are preserved, others have their cookie
// value cleared to all-bits-zero. Does not alter markings.
typedef int (*omega_regalloc_intersect_t)(
    omega_register_allocator_t *ctx,
    omega_register_allocator_t const *other);

// Invalidates the specified set of registers by clearing their
// cookie values to all-bits-zero. Does not alter markings.
typedef int (*omega_regalloc_invalidate_t)(
    omega_register_allocator_t *ctx, omega_regset_t regset);

struct omega_register_allocator
{
    // the size of this structure,
    // consulted when copying it.
    size_t struct_sz;

    omega_initregalloc_t initregalloc;

    omega_cookie2regid_t cookie2regid;
    omega_getregcookie_t getregcookie;
    omega_setregcookie_t setregcookie;

    omega_markregister_t markregister;
    omega_getavailable_t getavailable;
    omega_clearallmarks_t clearallmarks;

    omega_regalloc_intersect_t intersect;
    omega_regalloc_invalidate_t invalidate;
};

typedef struct {
    regid_t regid;
    int mark;
    cookie_t value_current;
} register_file_entry_t;

typedef struct dcc_omega dccOmega_t;

typedef omega_register_allocator_t *(*create_register_allocator_ctx_func)();

struct dcc_omega {
    bool char_is_signed;

    short stack_align;

    // Indexed with `mini_scalar_types` enumeration constants.
    omega_type_desc_t **types_descs;

    omega_func_args_placer_t (*create_func_args_placer_ctx)();

    create_register_allocator_ctx_func create_register_allocator_ctx;
};

#endif // dcc_omega_h
