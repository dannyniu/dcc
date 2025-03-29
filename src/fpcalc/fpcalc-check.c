/* DannyNiu/NJF, 2025-02-14. Public Domain. */

#include "fpcalc.h"
#include "../lex/langlex.h"
#include "../lex/shifter.h"
#include <ctype.h>

#include "../lalr/print-prod.c.h"

const char *quadratic_func = "f(x) = x*x";
const char *derivative = "g(x,h,d) = (h(x+d)-h(x))/d";

int main()
{
    shifter_ctx_t lexer;

    lalr_stack_t *ps = NULL;
    int i, subret;

#if INTERCEPT_MEM_CALLS
    long acq_before = 0;
    long rel_before = 0;
    long acq_after = 0;
    long rel_after = 0;

    acq_before = allocs;
    rel_before = frees;
#endif /* INTERCEPT_MEM_CALLS */

    shifter_init_from_expr(&lexer, NULL);
    lexer.fsm = langlex_fsm;
    lexer.puncts = langlex_puncts;
    lexer.matched_fallback = langlex_punct;

    ns_rules_fpcalc = strvec_create();
    globaldefs = s2dict_create();

    lexer.expr = quadratic_func;
    subret = lalr_parse(
        &ps,
        fpcalc_grammar_rules,
        NULL, ns_rules_fpcalc,
        (token_shifter_t)shifter,
        &lexer);
    printf("%d is returned from parsing the quadratic function.\n", subret);

    subret = remember_definition(ps->bottom->production, s2_setter_kept);
    s2obj_release(ps->pobj);

    lexer.expr = derivative;
    subret = lalr_parse(
        &ps,
        fpcalc_grammar_rules,
        NULL, ns_rules_fpcalc,
        (token_shifter_t)shifter,
        &lexer);
    printf("%d is returned from parsing the derivative function.\n", subret);

    subret = remember_definition(ps->bottom->production, s2_setter_kept);
    s2obj_release(ps->pobj);

    for(i=0; i<10; i++)
    {
        int subret;
        double result;
        char expreval[] = "g( , f, 0.25)";
        expreval[2] = i + '0';

        lexer.expr = expreval;
        subret = lalr_parse(
            &ps,
            fpcalc_grammar_rules,
            NULL, ns_rules_fpcalc,
            (token_shifter_t)shifter,
            &lexer);

        fpcalc_eval_start(&result, ps->bottom->production, NULL, NULL);
        printf("derivative at %d is %f (subret: %d)\n", i, result, subret);
        s2obj_release(ps->pobj);
    }

    s2obj_release(ns_rules_fpcalc->pobj);
    s2obj_release(globaldefs->pobj);
    subret=EXIT_SUCCESS;

#if INTERCEPT_MEM_CALLS
    acq_before = allocs;
    rel_before = frees;
    printf("acq-before: %ld, acq-after: %ld.\n", acq_before, acq_after);
    printf("rel-before: %ld, rel-after: %ld.\n", rel_before, rel_after);
    printf("mem-acquire: %ld, mem-release: %ld.\n", allocs, frees);
    for(i=0; i<4; i++)
    {
        if( mh[i] ) subret = EXIT_FAILURE;
        printf("%08lx%c", (long)mh[i], i==3 ? '\n' : ' ');
    }
#endif /* INTERCEPT_MEM_CALLS */
    return subret;
}
