/* DannyNiu/NJF, 2026-08-23. Public Domain. */

// 2026-08-30:
// Handles the definition of 1 function.
// Although it's possible to extend it to the entirety of all
// external declarations, it makes determining the boundary of
// forward traversal more difficult for the register allocator.

#ifndef SemaX
#define SemaX
#include "c-grammar.h"
#include "mental-comp.h"

#define ErrChk(...) __VA_ARGS__

#define OpTerm(n) (void)n; break; case n:;

#define X(prodrule, deps, children,                                     \
          phase1, actions1,                                             \
          phase2, actions2,                                             \
          phase3, actions3,                                             \
          phase4, actions4, ...)                                        \
    void foo##prodrule(                                                 \
        lalr_prod_t *node, dccMeta_t *ctx,                              \
        ptrdiff_t *operand_index){                                      \
        assert( prodrule );                                             \
        #deps; switch( *operand_index ){ default: ErrChk children };    \
        #phase1; { ErrChk actions1; }                                   \
        #phase2; { ErrChk actions2; }                                   \
        #phase3; { ErrChk actions3; }                                   \
        #phase4; { ErrChk actions4; } }

#endif // SemaX
