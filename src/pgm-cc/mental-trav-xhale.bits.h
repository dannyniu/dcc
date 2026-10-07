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

#define Exhalation(...) __VA_ARGS__

#define X(rule, deps, children,                                         \
          phase1, actions1,                                             \
          phase2, actions2,                                             \
          phase3, actions3,                                             \
          phasex, exhalation, ...)                                      \
    if( rfp == rule ) {                                                 \
        phasex exhalation;                                              \
        if( *operand_index < 0 ) {                                      \
            assert( *operand_index == -1 );                             \
            goto finish_eval_1term;                                     \
        } else {                                                        \
            node = sp->body->terms[sp->operand_index].production;       \
            rfp = c_grammar_rules[node->semantic_rule];                 \
            bp = calloc(1, sizeof(eval_stack_chained_t));               \
            assert( bp );                                               \
            bp->ret = sp;                                               \
            bp->operand_index = -1;                                     \
            bp->body = node;                                            \
            sp = bp;                                                    \
            operand_index = &sp->operand_index;                         \
            goto start_evaluation;                                      \
        } } else

int XhaleTravIdent(lalr_prod_t *code, dccMeta_t *ctx)
{
    eval_stack_chained_t rs_anch = {
        .ret = NULL,
        .operand_index = -1,
        .body = code,
    };
    eval_stack_chained_t *sp = &rs_anch, *bp;

    lalr_prod_t *node;
    lalr_rule_t rfp = c_grammar_rules[sp->body->semantic_rule];
    ptrdiff_t *operand_index = &sp->operand_index;
    // int subret;
    int ret = 0;

start_evaluation:
    node = sp->body;
    //- fprintf(stderr, "rule id: %d, rfp: %p [%p]. sp.ret: %p.\n", sp->body->semantic_rule, rfp, const_declit, sp->ret);
#include "c-semantics.bits.h"
    //- { fprintf(stderr, "unrecognized rule: %d. implementation underway?\n", sp->body->semantic_rule); }
    ; // This semicolon here is because of the `else` clause in the macro `X`, so DO NOT REMOVE!

finish_eval_1term:
    if( sp == &rs_anch )
        return ret;

    bp = sp;
    sp = sp->ret;
    free(bp);
    rfp = c_grammar_rules[sp->body->semantic_rule];
    operand_index = &sp->operand_index;
    if( *operand_index >= 0 )
        goto start_evaluation;
    else goto finish_eval_1term;
}

#undef XhaleTravIdent
