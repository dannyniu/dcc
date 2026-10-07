/* DannyNiu/NJF, 2026-08-23. Public Domain. */

#include "c-semantics.errchk.bits.h"

X( addexpr_add,

   Dependencies, ( OpTerm(0) OpTerm(2) ),
   NodeColoring, (
       mInstr_t *dish = MetaInstrADD(node, ctx->abi_oracle, ctx->ctx_tu);
       node->value = dish->pobj;
       if( ++ *CookieTable_pEntry(&ctx->consumer_stats, &dish->cookie) > 1 ) dish->reuseCandidate = true; ),

   OperandDiscount, (OperandDiscount_UnsequencedExpr(node, ctx)),
   CollectReuseStats, (CollectReuseStats_UnsequencedExpr(node, ctx)),

   Exhalation, (BinaryOperation_Exhalation(node, ctx, operand_index)),
    )

X( addexpr_subtract,

   Dependencies, ( OpTerm(0) OpTerm(2) ),
   NodeColoring, (
       mInstr_t *dish = MetaInstrSUB(node, ctx->abi_oracle, ctx->ctx_tu);
       node->value = dish->pobj;
       if( ++ *CookieTable_pEntry(&ctx->consumer_stats, &dish->cookie) > 1 ) dish->reuseCandidate = true; ),

   OperandDiscount, (OperandDiscount_UnsequencedExpr(node, ctx)),
   CollectReuseStats, (CollectReuseStats_UnsequencedExpr(node, ctx)),

   Exhalation, (BinaryOperation_Exhalation(node, ctx, operand_index)),
    )

X( mulexpr_multiply,

   Dependencies, ( OpTerm(0) OpTerm(2) ),
   NodeColoring, (
       mInstr_t *dish = MetaInstrMUL(node, ctx->abi_oracle, ctx->ctx_tu);
       node->value = dish->pobj;
       if( ++ *CookieTable_pEntry(&ctx->consumer_stats, &dish->cookie) > 1 ) dish->reuseCandidate = true; ),

   OperandDiscount, (OperandDiscount_UnsequencedExpr(node, ctx)),
   CollectReuseStats, (CollectReuseStats_UnsequencedExpr(node, ctx)),

   Exhalation, (BinaryOperation_Exhalation(node, ctx, operand_index)),
    )

X( mulexpr_divide,

   Dependencies, ( OpTerm(0) OpTerm(2) ),
   NodeColoring, (
       mInstr_t *dish = MetaInstrDIV(node, ctx->abi_oracle, ctx->ctx_tu);
       node->value = dish->pobj;
       if( ++ *CookieTable_pEntry(&ctx->consumer_stats, &dish->cookie) > 1 ) dish->reuseCandidate = true; ),

   OperandDiscount, (OperandDiscount_UnsequencedExpr(node, ctx)),
   CollectReuseStats, (CollectReuseStats_UnsequencedExpr(node, ctx)),

   Exhalation, (BinaryOperation_Exhalation(node, ctx, operand_index)),
    )

X( mulexpr_remainder,

   Dependencies, ( OpTerm(0) OpTerm(2) ),
   NodeColoring, (
       mInstr_t *dish = MetaInstrREM(node, ctx->abi_oracle, ctx->ctx_tu);
       node->value = dish->pobj;
       if( ++ *CookieTable_pEntry(&ctx->consumer_stats, &dish->cookie) > 1 ) dish->reuseCandidate = true; ),

   OperandDiscount, (OperandDiscount_UnsequencedExpr(node, ctx)),
   CollectReuseStats, (CollectReuseStats_UnsequencedExpr(node, ctx)),

   Exhalation, (BinaryOperation_Exhalation(node, ctx, operand_index)),
    )

X( const_binlit,

   Dependencies, (;),
   NodeColoring, (
       mInstr_t *dish = MetaInstrIMM(node->terms[0].terminal, ctx->abi_oracle, ctx->ctx_tu);
       node->value = dish->pobj;
       if( ++ *CookieTable_pEntry(&ctx->consumer_stats, &dish->cookie) > 1 ) dish->reuseCandidate = true; ),

   OperandDiscount, (OperandDiscount_UnsequencedExpr(node, ctx)),
   CollectReuseStats, (CollectReuseStats_UnsequencedExpr(node, ctx)),

   Exhalation, (TerminalLeaf_Exhalation(node, ctx, operand_index)),
    )

X( const_octlit,

   Dependencies, (;),
   NodeColoring, (
       mInstr_t *dish = MetaInstrIMM(node->terms[0].terminal, ctx->abi_oracle, ctx->ctx_tu);
       node->value = dish->pobj;
       if( ++ *CookieTable_pEntry(&ctx->consumer_stats, &dish->cookie) > 1 ) dish->reuseCandidate = true; ),

   OperandDiscount, (OperandDiscount_UnsequencedExpr(node, ctx)),
   CollectReuseStats, (CollectReuseStats_UnsequencedExpr(node, ctx)),

   Exhalation, (TerminalLeaf_Exhalation(node, ctx, operand_index)),
    )

X( const_declit,

   Dependencies, (;),
   NodeColoring, (
       mInstr_t *dish = MetaInstrIMM(node->terms[0].terminal, ctx->abi_oracle, ctx->ctx_tu);
       node->value = dish->pobj;
       if( ++ *CookieTable_pEntry(&ctx->consumer_stats, &dish->cookie) > 1 ) dish->reuseCandidate = true; ),

   OperandDiscount, (OperandDiscount_UnsequencedExpr(node, ctx)),
   CollectReuseStats, (CollectReuseStats_UnsequencedExpr(node, ctx)),

   Exhalation, (TerminalLeaf_Exhalation(node, ctx, operand_index)),
    )

X( const_hexlit,

   Dependencies, (;),
   NodeColoring, (
       mInstr_t *dish = MetaInstrIMM(node->terms[0].terminal, ctx->abi_oracle, ctx->ctx_tu);
       node->value = dish->pobj;
       if( ++ *CookieTable_pEntry(&ctx->consumer_stats, &dish->cookie) > 1 ) dish->reuseCandidate = true; ),

   OperandDiscount, (OperandDiscount_UnsequencedExpr(node, ctx)),
   CollectReuseStats, (CollectReuseStats_UnsequencedExpr(node, ctx)),

   Exhalation, (TerminalLeaf_Exhalation(node, ctx, operand_index)),
    )
