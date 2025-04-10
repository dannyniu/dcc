/* DannyNiu/NJF, 2025-02-23. Public Domain. */

#include "fpcalc.h"
#include "../lex/langlex.h"
#include "../lex/shifter.h"
#include <ctype.h>

s2data_t *s2data_stdio_getline(FILE *fp)
{
    s2data_t *ret = s2data_create(0);
    int c;

    if( !ret ) return NULL;

    while( (c = getc(fp)) >= 0 )
    {
        if( s2data_putc(ret, c) == -1 )
        {
            s2obj_release(ret->pobj);
            return NULL;
        }
        if( c == '\n' ) break;
    }

    if( s2data_putfin(ret) == -1 )
    {
        s2obj_release(ret->pobj);
        return NULL;
    }
    return ret;
}

// for debugging:
//- #include "../lalr/print-prod.c.h"

int main()
{
    shifter_ctx_t lexer;

    lalr_stack_t *ps = NULL;
    s2data_t *line_data;
    const char *line;
    int i, subret;
    double result;

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

    while( !feof(stdin) )
    {
        fprintf(stderr, "input> ");
        fflush(stderr);

        line_data = s2data_stdio_getline(stdin);
        if( !line_data || s2data_len(line_data) == 0 )
        {
            fprintf(stderr, "Bye!\n");
            s2obj_release(line_data->pobj);
            break;
        }

        line = s2data_map(line_data, 0, 0);
        lexer.expr = line;
        subret = lalr_parse(
            &ps,
            fpcalc_grammar_rules,
            NULL, ns_rules_fpcalc,
            (token_shifter_t)shifter,
            &lexer);

        s2data_unmap(line_data);
        s2obj_release(line_data->pobj);

        if( subret != 0 )
        {
            fprintf(stderr, "`lalr_parse` returned error: %d!\n", subret);
            if( ps ) s2obj_release(ps->pobj);
            continue;
        }

        // for debugging:
        //- print_prod(ps->bottom->production, 0, ns_rules_fpcalc);

        subret = remember_definition(ps->bottom->production, s2_setter_kept);
        if( subret == s2_access_success )
        {
            s2obj_release(ps->pobj);
            continue;
        }

        if( subret != s2_access_nullval )
            fprintf(stderr, "`remember_definition` returned error!\n");

        subret = fpcalc_eval_start(
            &result, ps->bottom->production, NULL, NULL);

        if( !subret )
        {
            fprintf(stderr, "Expression evaluation encountered error!\n");
        }
        else printf("%f\n", result);

        s2obj_release(ps->pobj);
    }

    s2obj_release(ns_rules_fpcalc->pobj);
    s2obj_release(globaldefs->pobj);
    subret = EXIT_SUCCESS;

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
