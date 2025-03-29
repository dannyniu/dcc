/* DannyNiu/NJF, 2025-03-29. Public Domain. */

// #include "fpcalc-grammar.h" // 2025-03-29: included by parent source.
// 2025-03-29: expects `GRAMMAR_RULES` and `NS_RULES`.

#include "../lex/langlex.h"
#include "../lex/shifter.h"
#include <ctype.h>

#include "../lalr/print-prod.c.h"

int main(int argc, char *argv[])
{
    shifter_ctx_t shctx;
    lalr_stack_t *parsed;
    lalr_term_t *te;
    int indentlevel = 0;
    int subret = 0, i;

#if INTERCEPT_MEM_CALLS
    long acq_before = 0;
    long rel_before = 0;
    long acq_after = 0;
    long rel_after = 0;
#endif /* INTERCEPT_MEM_CALLS */

    if( strcmp(argv[1], "-f") == 0 )
    {
        shifter_init_from_fp(&shctx, fopen(argv[2], "r"));
    }
    else if( strcmp(argv[1], "-e") == 0 )
    {
        shifter_init_from_expr(&shctx, argv[2]);
    }
    shctx.fsm = langlex_fsm;
    shctx.puncts = langlex_puncts;
    shctx.matched_fallback = langlex_punct;
    (void)argc;

#if INTERCEPT_MEM_CALLS
    acq_before = allocs;
    rel_before = frees;
#endif /* INTERCEPT_MEM_CALLS */

    NS_RULES = strvec_create();

    i = lalr_parse(&parsed, GRAMMAR_RULES, NULL, NS_RULES,
               (token_shifter_t)shifter, (void *)&shctx);
    printf("parsing returned: %d, stack:\n", i);

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
    s2obj_release(NS_RULES->pobj);
    subret = EXIT_SUCCESS;

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
