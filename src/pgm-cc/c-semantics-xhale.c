/* DannyNiu/NJF, 2026-08-28. Public Domain. */

#include "mental-comp.h"

#define OpEval(n) *operand_index = n; break; case n:;

static void MiniInstr_PushPre(mInstr_t *dish, dccMeta_t *ctx)
{
    if( dish->pre )
    {
        MiniInstr_PushPre(dish->pre, ctx);
    }

    dish->seqno = s2list_len(ctx->mini_stream) + 1;
    s2list_push(ctx->mini_stream, dish->pobj, s2_setter_kept);
}

void BinaryOperation_Exhalation(
    lalr_prod_t *node, dccMeta_t *ctx,
    ptrdiff_t *operand_index)
{
    switch( *operand_index )
    {
    default:
        OpEval(0) OpEval(2) *operand_index = -1;

        // emitting the instruction.
        //
        // 2026-08-30:
        // For binary arithmetic instructions, the pre-sequence
        // may convert the type of the operands.
        MiniInstr_PushPre((mInstr_t *)node->value, ctx);

        break;
    }
}

void TerminalLeaf_Exhalation(
    lalr_prod_t *node, dccMeta_t *ctx,
    ptrdiff_t *operand_index)
{
    MiniInstr_PushPre((mInstr_t *)node->value, ctx);
    *operand_index = -1;
}

#define XhaleTravIdent XhaleTrav_MiniStream
#include "mental-trav-xhale.bits.h"
