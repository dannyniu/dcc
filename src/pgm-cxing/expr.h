/* DannyNiu/NJF, 2025-11-15. Public Domain. */

#ifndef cxing_expr_h
#define cxing_expr_h

#include "langsem.h"

struct value_nativeobj IncrementExpr(
    struct lvalue_nativeobj lvalue, int preop);

struct value_nativeobj DecrementExpr(
    struct lvalue_nativeobj lvalue, int preop);

struct value_nativeobj PositiveExpr(
    struct value_nativeobj ivalue);

struct value_nativeobj NegativeExpr(
    struct value_nativeobj ivalue);

struct value_nativeobj BitComplExpr(
    struct value_nativeobj ivalue);

bool LogicallyTrue(
    struct value_nativeobj ivalue);

struct value_nativeobj LogicNotExpr(
    struct value_nativeobj ivalue);

// -- unary expressions so far. -- //

struct value_nativeobj ArithMulExpr(
    struct value_nativeobj a,
    struct value_nativeobj b);

struct value_nativeobj ArithDivExpr(
    struct value_nativeobj a,
    struct value_nativeobj b);

struct value_nativeobj ArithModExpr(
    struct value_nativeobj a,
    struct value_nativeobj b);

struct value_nativeobj ArithAddExpr(
    struct value_nativeobj a,
    struct value_nativeobj b);

struct value_nativeobj ArithSubExpr(
    struct value_nativeobj a,
    struct value_nativeobj b);

// -- arithmetic computation expressions so far. -- //

struct value_nativeobj BitLShiftExpr(
    struct value_nativeobj a,
    struct value_nativeobj b);

struct value_nativeobj BitRShiftExpr(
    struct value_nativeobj a,
    struct value_nativeobj b);

struct value_nativeobj BitARShiftExpr(
    struct value_nativeobj a,
    struct value_nativeobj b);

// -- bit shifting expressions so far.

struct value_nativeobj LessThanExpr(
    struct value_nativeobj a,
    struct value_nativeobj b);

struct value_nativeobj LessEqaulExpr(
    struct value_nativeobj a,
    struct value_nativeobj b);

struct value_nativeobj GreaterThanExpr(
    struct value_nativeobj a,
    struct value_nativeobj b);

struct value_nativeobj GreaterEqaulExpr(
    struct value_nativeobj a,
    struct value_nativeobj b);

struct value_nativeobj LooseEqualityExpr(
    struct value_nativeobj a,
    struct value_nativeobj b);

struct value_nativeobj StrictEqualityExpr(
    struct value_nativeobj a,
    struct value_nativeobj b);

// -- comparison predicates so far. -- //

struct value_nativeobj BitAndExpr(
    struct value_nativeobj a,
    struct value_nativeobj b);

struct value_nativeobj BitOrExpr(
    struct value_nativeobj a,
    struct value_nativeobj b);

struct value_nativeobj BitXorExpr(
    struct value_nativeobj a,
    struct value_nativeobj b);

// 2025-11-22:
// The nullish coalescing and logic conjugations and disjunctions
// are short-circuit, as such they're not suitable to be implemented
// at here as functions that take already-evaluated operands.

#endif /* cxing_expr_h */
