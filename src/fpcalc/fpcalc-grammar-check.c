/* DannyNiu/NJF, 2025-01-03. Public Domain. */

#include "fpcalc-grammar.h"

#include "../lex/langlex.h"
#include <ctype.h>

typedef struct {
    lex_getc_base_t base;
    FILE *fp;
    int32_t lineno, column;
    int c, i;
} shifter_ctx_t;

static int lex_getc(shifter_ctx_t *ctx)
{
    return fgetc(ctx->fp);
}

static int lex_ungetc(int c, shifter_ctx_t *ctx)
{
    return ungetc(c, ctx->fp);
}

lex_token_t *shifter(shifter_ctx_t *shctx)
{
    lex_token_t *token;
    int c;

    while( !feof(shctx->fp) )
    {
        if( (c = getc(shctx->fp)) == EOF ) break;
        if( c == '\n' )
        {
            shctx->lineno ++;
            shctx->column = 1;
        }
        else if( isspace(c) )
        {
            shctx->column ++;
        }

        if( !isspace(c) )
        {
            ungetc(c, shctx->fp);
            token = lex_token_parse(langlex_fsm, &shctx->base);

            if( !token )
                token = lex_token_match(
                    langlex_puncts, langlex_punct, &shctx->base);

            if( !token )
            {
                fprintf(stderr, "Encountered malformed token!\n");
                return NULL;
            }

            token->lineno = shctx->lineno;
            token->column = shctx->column;
            shctx->column += s2data_len(token->str);

            return token;
        }
    }

    return NULL;
}

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

    shctx.base.getc = (lex_getc_func_t)lex_getc;
    shctx.base.ungetc = (lex_ungetc_func_t)lex_ungetc;
    shctx.fp = fopen(argv[1], "r");
    shctx.lineno = 1;
    shctx.column = 0;
    (void)argc;

#if INTERCEPT_MEM_CALLS
    acq_before = allocs;
    rel_before = frees;
#endif /* INTERCEPT_MEM_CALLS */

    ns_rules_fpcalc = strvec_create();

    i = lalr_parse(&parsed, fpcalc_grammar_rules, NULL, ns_rules_fpcalc,
               (token_shifter_t)shifter, (void *)&shctx);
    printf("parsing returned: %d, stack:\n", i);

    te = parsed->bottom;
    while( te )
    {
        printf("%p\t: ", te);

        if( s2_is_prod(te->production) )
        {
            print_prod(te->production, indentlevel, ns_rules_fpcalc);
        }
        else print_token(te->terminal, indentlevel);

        te = te->up;
    }

    s2obj_release(parsed->pobj);
    s2obj_release(ns_rules_fpcalc->pobj);
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
