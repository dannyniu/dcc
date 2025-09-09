/* DannyNiu/NJF, 2025-02-23. Public Domain. */

#include "fpcalc.h"
#include "../langlex-c/langlex-c.h"

#if __has_include(<readline/readline.h>)
#include <readline/readline.h>

s2data_t *s2data_stdio_getline(FILE *fp)
{
    assert( fp == stdin );
    char *line = readline("input> ");
    if( !line ) return NULL;
    s2data_t *ret = s2data_from_str(line);
    (free)(line);
    return ret;
}

#else // There's no 'readline.h'
s2data_t *s2data_stdio_getline(FILE *fp)
{
    s2data_t *ret;
    int c;

    fprintf(stderr, "input> ");
    fflush(stderr);

    ret = s2data_create(0);
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

#endif // __has_include(<readline/readline.h>)

int logger(void *ctx, const char *msg)
{
    (void)ctx;
    fprintf(stderr, "%s\n", msg);
    return 0;
}

int main()
{
    source_rope_t *rope;
    RegexLexContext lexer;

    lalr_stack_t *ps = NULL;
    s2data_t *line_data;
    //const char *line;
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

    while( !feof(stdin) )
    {
        line_data = s2data_stdio_getline(stdin);
        if( !line_data || s2data_len(line_data) == 0 )
        {
            fprintf(stderr, "Bye!\n");
            if( line_data )
                s2obj_release(line_data->pobj);
            break;
        }
        rope = CreateRopeFromLineData(line_data, s2_setter_gave);
        RegexLexFromRope_Init(&lexer, rope);

        subret = lalr_parse(
            &ps,
            fpcalc_grammar_rules,
            NULL, ns_rules_fpcalc,
            (token_shifter_t)RegexLexFromRope_Shift,
            &lexer);

        s2obj_release(rope->pobj);

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

    for(i=0; CLexElems[i].pattern; i++)
    {
        libregfree(&CLexElems[i].preg);
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
