/* DannyNiu/NJF, 2026-08-29. Public Domain. */

#ifndef SemaX_Incs
#define SemaX_Incs
#include "mental-comp.h"
#include "c-grammar.h"

typedef struct eval_stack_chained_ctx eval_stack_chained_t;

struct eval_stack_chained_ctx {
    eval_stack_chained_t *ret;
    ptrdiff_t operand_index;
    lalr_prod_t *body;
};
#endif

#define SemaX

#define OpTerm(n)                                               \
    node = sp->body->terms[sp->operand_index = n].production;   \
    rfp = c_grammar_rules[node->semantic_rule];                 \
    bp = calloc(1, sizeof(eval_stack_chained_t));               \
    assert( bp );                                               \
    bp->ret = sp;                                               \
    bp->operand_index = -1;                                     \
    bp->body = node;                                            \
    sp = bp;                                                    \
    goto start_evaluation;                                      \
    break; case n:;
#define Dependencies(...) __VA_ARGS__

#define X(rule, deps, children,                         \
          phase1, actions1,                             \
          phase2, actions2,                             \
          phase3, actions3, ...)                        \
    if( rfp == rule ) { switch( sp->operand_index ) {   \
        default: deps children; node = sp->body;        \
            phase1 actions1;                            \
            phase2 actions2;                            \
            phase3 actions3;                            \
            goto finish_eval_1term;                     \
        } } else

#ifndef NodeColoring
#define NodeColoring(...)
#endif

#ifndef OperandDiscount
#define OperandDiscount(...)
#endif

#ifndef CollectReuseStats
#define CollectReuseStats(...)
#endif

int ProbeTravIdent(lalr_prod_t *code, dccMeta_t *ctx)
{
    eval_stack_chained_t rs_anch = {
        .ret = NULL,
        .operand_index = -1,
        .body = code,
    };
    eval_stack_chained_t *sp = &rs_anch, *bp;

    lalr_prod_t *node;
    lalr_rule_t rfp = c_grammar_rules[sp->body->semantic_rule];
    // int subret;
    int ret = 0;

start_evaluation:
    //- fprintf(stderr, "rule id: %d, rfp: %p [%p]. sp.ret: %p.\n", sp->body->semantic_rule, rfp, const_declit, sp->ret);
#include "c-semantics.bits.h"
    { fprintf(stderr, "unrecognized rule: %d. implementation underway?\n", sp->body->semantic_rule); }
    ; // This semicolon here is because of the `else` clause in the macro `X`, so DO NOT REMOVE!

finish_eval_1term:
    if( sp == &rs_anch )
        return ret;

    bp = sp;
    sp = sp->ret;
    free(bp);
    rfp = c_grammar_rules[sp->body->semantic_rule];
    goto start_evaluation;
}

#undef NodeColoring
#undef OperandDiscount
#undef CollectReuseStats
#undef ProbeTravIdent
