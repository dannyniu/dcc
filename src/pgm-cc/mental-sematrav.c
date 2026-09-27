/* DannyNiu/NJF, 2026-08-13. Public Domain. */

// Semantical Traversal.

#define NodeColoring(...) __VA_ARGS__
#define ProbeTravIdent SemaTrav_NodeColoring
#include "mental-trav-probe.bits.h"

#define OperandDiscount(...) __VA_ARGS__
#define ProbeTravIdent SemaTrav_OperandDiscount
#include "mental-trav-probe.bits.h"

#define CollectReuseStats(...) __VA_ARGS__
#define ProbeTravIdent SemaTrav_CollectReuseStats
#include "mental-trav-probe.bits.h"
