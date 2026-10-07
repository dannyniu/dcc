/* DannyNiu/NJF, 2026-08-12. Public Domain. */

#ifndef dcc_stack_parts_h
#define dcc_stack_parts_h 1

#include "c-grammar.h"
#include "../c-misc/cookie-table.h"
#include "../cpp-c/cpp-c.h"
#include <SafeTypes2.h>

//
// Partition-I of the stack: declared variables.

// Wrapped in an `s2ref_t`.
// Info pertaining to the current state of a declared identifier
// used for determining:
// - whether the object variable needs a reload.
typedef struct {
    // Unowned pointers fields.
    lex_token_t *ident;
    lalr_prod_t *declspecs;
    lalr_prod_t *declarator;

    // 2026-08-12:
    // See note below for the `monotone_global` variable.
    // This mechanism is for guaranteeing the consistent loading
    // of values in objects from _external declarations_,
    // local stack variables don't consult this field.
    size_t monotone;

    // for resetting the monotone counter
    // across different phases of compilation.
    int era;

    int is_volatile; // in which case, no load/store will be omitted.
    int is_address_taken; // in which case, re-generate cookie on monotone change.
    int is_external_decl; // in which case, loading will be through symbols.

    // Currently held value, represented as a cookie data structure.
    cookie_t cookie;
} dcc_decl_info_t;

// 2026-08-12:
// A piece of pseudo-code may explain it better.
// note however, this isn't the only condition coming into play.
// ```
// bool needs_reload(vardecl) {
//   if( vardecl.monotone < monotone_global ) {
//     vardecl.monotone = monotone_global;
//     return true;
//   }
//   return false;
// }
// ```
//
// `monotone_global` is incremented whenever:
// - entering a function body,
// - encountering a jump label,
// - on a call to a function without the 'unsequenced' attribute,
// - on leave from the body of a conditional or loop statement.
//
// This includes basically any situation where a force-load is needed,
// plus anything else that's essentially opaque.
//
// A retroactive observation is that all these cases involve
// branching to a label at assembly level.
extern size_t monotone_global;
extern int era_global;

// When entering a `{}`.
// The returned dict is 'given' to (i.e. owned by) the list.
s2dict_t *ScopedDecls_Push1Scope(s2list_t *sd);

// When leaving a `{}`.
void ScopedDecls_Pop1Scope(s2list_t *sd);

// Returns one of the `s2_access_*` enumerations.
// The compiler may modify the declaration info (in particular, its cookie).
int ScopedDecls_Lookup(s2list_t *sd, s2data_t *ident, dcc_decl_info_t **out);

// Process 1 Declaration,
// And Report Errors.
// Returns one of the `s2_access_*` enumerations barring reported errors.
//
// 2026-08-12:
// This may be refuted some time later but,
// the processing of declarations here is
// separate from the coloring of the graph.
//
// 2026-08-12:
// This function reports redeclared identifiers,
// but errors in initializer expressions are
// handled elsewhere in the compiler.
//
int ScopedDecls_Process1Decl(s2list_t *sd, lalr_prod_t *declaration, cpptu_t *ctx_tu);

//
// Partition-II of the stack: reused operands - we did away with these as of 2026-10-07.
//
// Partition-III of the stack: ephemeral temporaries.

// Embedded onto `s2data_t` objects.
typedef struct {
    // The conceptual value of this stack slot according to the mental picture.
    cookie_t cookie;

    // the address of the stack slot, relative to the beginning of the ephemerals partition.
    int32_t addrend;
} dcc_stack_entry_info_t;

#endif // dcc_stack_parts_h
