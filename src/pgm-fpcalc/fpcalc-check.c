/* DannyNiu/NJF, 2025-02-14. Public Domain. */

#include "fpcalc.h"
#include "../langlex/langlex-c.h"

void print_token(lex_token_t *tn, int indentlevel);
void print_prod(lalr_prod_t *prod, int indentlevel, strvec_t *ns);

const char *quadratic_func = "f(x) = x*x";
const char *derivative = "g(x,h,d) = (h(x+d)-h(x))/d";

int logger(void *ctx, const char *msg)
{
    (void)ctx;
    fprintf(stderr, "%s\n", msg);
    return 0;
}

int main()
{
    //shifter_ctx_t lexer;
    source_rope_t *rope;
    RegexLexContext lexer;

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

    /*shifter_init_from_expr(&lexer, NULL);
     *    lexer.fsm = langlex_fsm;
     *    lexer.puncts = langlex_puncts;
     *    lexer.matched_fallback = langlex_punct;*/
    for(i=0; CLexElems[i].pattern; i++)
    {
        subret = libregcomp(
            &CLexElems[i].preg, CLexElems[i].pattern, CLexElems[i].cflags);
    }
    lexer.regices = CLexElems;
    lexer.logger_base = (struct logging_ctxbase){
        .logger = (logger_func)logger,
    };

    ns_rules_fpcalc = strvec_create();
    globaldefs = s2dict_create();

    rope = CreateRopeFromLineData(
        s2data_from_str(quadratic_func), s2_setter_gave);
    RegexLexFromRope_Init(&lexer, rope);
    subret = lalr_parse(
        &ps,
        fpcalc_grammar_rules,
        NULL, ns_rules_fpcalc,
        (token_shifter_t)RegexLexFromRope_Shift,
        &lexer);
    printf("%d is returned from parsing the quadratic function.\n", subret);

    subret = remember_definition(ps->bottom->production, s2_setter_kept);
    printf("%d is returned from remembering the definition.\n", subret);
    s2obj_release(ps->pobj);
    s2obj_release(rope->pobj);

    rope = CreateRopeFromLineData(
        s2data_from_str(derivative), s2_setter_gave);
    RegexLexFromRope_Init(&lexer, rope);
    subret = lalr_parse(
        &ps,
        fpcalc_grammar_rules,
        NULL, ns_rules_fpcalc,
        (token_shifter_t)RegexLexFromRope_Shift,
        &lexer);
    printf("%d is returned from parsing the derivative function.\n", subret);

    subret = remember_definition(ps->bottom->production, s2_setter_kept);
    printf("%d is returned from remembering the definition.\n", subret);
    s2obj_release(ps->pobj);
    s2obj_release(rope->pobj);

    for(i=0; i<10; i++)
    {
        int subret;
        double result;
        char expreval[] = "g( , f, 0.25)";
        expreval[2] = i + '0';

        rope = CreateRopeFromLineData(
            s2data_from_str(expreval), s2_setter_gave);
        RegexLexFromRope_Init(&lexer, rope);
        subret = lalr_parse(
            &ps,
            fpcalc_grammar_rules,
            NULL, ns_rules_fpcalc,
            (token_shifter_t)RegexLexFromRope_Shift,
            &lexer);

        fpcalc_eval_start(&result, ps->bottom->production, NULL, NULL);
        printf("derivative at %d is %f (subret: %d)\n", i, result, subret);
        s2obj_release(ps->pobj);
        s2obj_release(rope->pobj);
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
