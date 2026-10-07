/* DannyNiu/NJF, 2026-08-23. Public Domain. */

// 2026-08-30:
// Handles the definition of 1 function.
// Although it's possible to extend it to the entirety of all
// external declarations, it makes determining the boundary of
// forward traversal more difficult for the register allocator.
//
// 2026-10-05:
// The above 2026-08-30 note might be a slight bit out of date.

#include "c-semantics-expr.bits.h"
#include "c-semantics-decl.bits.h"
