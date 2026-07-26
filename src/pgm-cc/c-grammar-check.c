/* DannyNiu/NJF, 2026-07-21. Public Domain. */

#include "../cpp-c/cpp-c.h"
#include "c-grammar.h"
#include <s2obj.h>

#define GRAMMAR_RULES c_grammar_rules
#define NS_RULES ns_rules_c
#define var_lex_elems CLexElems
#include "../lalr-common/lalr.h"

#include <time.h>

int logger(void *ctx, const char *msg)
{
    (void)ctx;
    fprintf(stderr, "%s\n", msg);
    return 0;
}

int main(int argc, char *argv[])
{
    cpptu_t *cpptu;
    lalr_stack_t *parsed;
    lalr_term_t *te;
    int indentlevel = 0;
    int subret = 0, i;

    clock_t perfcounter;

#if INTERCEPT_MEM_CALLS
    long acq_before = 0;
    long rel_before = 0;
    long acq_after = 0;
    long rel_after = 0;
#endif /* INTERCEPT_MEM_CALLS */

    ccPreprocInit();
    perfcounter = clock();

    assert( argc > 1 );

    cpptu = cpptu_create(argv[1], NULL);
    s2list_insert(cpptu->IncPaths, s2data_from_str(
                      "../tests/dcc-preproc")->pobj, s2_setter_gave);
    cpptu->ctx_shifter.logger_base = (struct logging_ctxbase){
        .logger = (logger_func)logger,
    };

#if INTERCEPT_MEM_CALLS
    acq_before = allocs;
    rel_before = frees;
#endif /* INTERCEPT_MEM_CALLS */

    perfcounter = clock();
    i = lalr_parse(&parsed, GRAMMAR_RULES, NULL, NS_RULES,
                   (token_shifter_t)cppDirectivesDispatch, (void *)cpptu);
    printf("parsing returned: %d after %ld clock cycles, stack:\n",
           i, clock() - perfcounter);

    te = parsed->bottom;
    while( te )
    {
        printf("%p\t: ", te);

        if( s2_is_prod(te->production) )
        {
            print_prod(te->production, indentlevel, NS_RULES);
        }
        else print_token(te->terminal, indentlevel);

        te = te->up;
    }

    s2obj_release(parsed->pobj);
    s2obj_release(cpptu->pobj);

    perfcounter = clock() - perfcounter;
    lalr_parse_accel_cache_clear();
    ccPreprocFin();

#ifndef SAFETYPES2_BUILD_WITHOUT_GC
    s2obj_t *gctail = s2gc_obj_alloc(0x6543, 128);
    s2obj_t *gcsave = gctail;
    i=0;
    for(i=0; gctail; i++)
    {
        if( !gcsave->gc_prev ) break;
        printf("%d: (%p) %x %d+%d.\n", i, gctail, gctail->type, gctail->refcnt, gctail->keptcnt);
        if( s2_is_token(gctail) )
        {
            lex_token_t *tok = (void *)gctail;
            printf("tok<%d>: `%s`\n", tok->completion, (char *)s2data_weakmap(tok->str));
        }
        if( s2_is_data(gctail) )
        {
            printf("%s (%p, %zd bytes)\n",
                   (const char *)s2data_weakmap((s2data_t *)gctail),
                   gctail, s2data_len((s2data_t *)gctail));
        }

        gctail = gctail->gc_prev;
    }
    s2obj_release(gcsave);
#endif

#if INTERCEPT_MEM_CALLS
    acq_after = allocs;
    rel_after = frees;
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
