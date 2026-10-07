/* DannyNiu/NJF, 2026-08-13. Public Domain. */

// Semantical Traversal.

#define NodeColoring(...) __VA_ARGS__
#define ProbeTravIdent SemaTrav_NodeColoring
#include "mental-trav-probe.bits.h"

//
// 2026-10-05:
// The ways reused operands are handled had been changed.
// See section '2026-10-05' in "notes.md".

#define OperandDiscount(...) __VA_ARGS__
#define ProbeTravIdent SemaTrav_OperandDiscount
#include "mental-trav-probe.bits.h"

#define CollectReuseStats(...) __VA_ARGS__
#define ProbeTravIdent SemaTrav_CollectReuseStats
#include "mental-trav-probe.bits.h"
