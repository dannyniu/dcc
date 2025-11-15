/* DannyNiu/NJF, 2025-11-15. Public Domain. */

#ifndef cxing_expr_h
#define cxing_expr_h

#include "langsem.h"

struct value_nativeobj NullCoalesceExpr(
    struct value_nativeobj testexpr,
    struct value_nativeobj replexpr);

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

// Unary expressions so far.

#endif /* cxing_expr_h */
