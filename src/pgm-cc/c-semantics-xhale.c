/* DannyNiu/NJF, 2026-08-28. Public Domain. */

#include "mental-comp.h"

#define OpEval(n) *operand_index = n; break; case n:;

static void MiniInstr_PushPre(mInstr_t *dish, dccMeta_t *ctx)
{
    if( dish->pre )
    {
        MiniInstr_PushPre(dish->pre, ctx);
    }

    s2list_push(ctx->mini_stream, dish->pobj, s2_setter_kept);
}

void BinaryOperation_Exhalation(
    lalr_prod_t *node, dccMeta_t *ctx,
    ptrdiff_t *operand_index)
{
    switch( *operand_index )
    {
    default:
        if( *CookieTable_pEntry(
                &ctx->usage_stats,
                &((mInstr_t *)node->value)->cookie) == 0 ||
            // the following line was added 2026-08-28, TODO: check if it conflicts with the 2026-08-24 codes on cookie table entry below.
            !((mInstr_t *)node->value)->reuseCandidate )
        {
            // First occurence, emit computation sequence.
            //
            // 2026-08-24 Another possibility to consider:
            // Weight too small to warrant saving on partition-II of the stack.
            OpEval(0) OpEval(2) *operand_index = -1;

            // emitting the instruction.
            //
            // 2026-08-30:
            // For binary arithmetic instructions, the pre-sequence
            // may convert the type of the operands.
            MiniInstr_PushPre((mInstr_t *)node->value, ctx);

            if( ((mInstr_t *)node->value)->reuseCandidate )
            {
                // 2026-08-24:
                // Incrementing here allows inhibiting reuse of low-weight operands.
                ++ *CookieTable_pEntry(
                    &ctx->usage_stats,
                    &((mInstr_t *)node->value)->cookie);

                // 2026-08-24 TODO: Emit operand save sequence.
            }
        }
        else
        {
            // Emit operand reuse sequence.
        }
        break;
    }
}

void TerminalLeaf_Exhalation(
    lalr_prod_t *node, dccMeta_t *ctx,
    ptrdiff_t *operand_index)
{
    // 2026-09-26 TODO: Currently a stub, to be extensively implemented.
    MiniInstr_PushPre((mInstr_t *)node->value, ctx);
    *operand_index = -1;
}

#define XhaleTravIdent XhaleTrav_MiniStream
#include "mental-trav-xhale.bits.h"
