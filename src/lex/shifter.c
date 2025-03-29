/* DannyNiu/NJF, 2025-03-29. Public Domain. */

#include "shifter.h"
#include <ctype.h>

int fp_getc(shifter_ctx_t *ctx)
{
    return fgetc(ctx->fp);
}

int fp_ungetc(int c, shifter_ctx_t *ctx)
{
    return ungetc(c, ctx->fp);
}

int expr_getc(shifter_ctx_t *ctx)
{
    int ret = *ctx->expr;
    if( !ret ) return EOF;
    ctx->expr++;
    return ret;
}

int expr_ungetc(int c, shifter_ctx_t *ctx)
{
    if( c >= 0 ) --ctx->expr;
    return c;
}

shifter_ctx_t *shifter_init_from_fp(shifter_ctx_t *shctx, FILE *fp)
{
    shctx->base.getc = (lex_getc_func_t)fp_getc;
    shctx->base.ungetc = (lex_ungetc_func_t)fp_ungetc;
    shctx->fp = fp;

    shctx->lineno = 1;
    shctx->column = 0;
    return shctx;
}

shifter_ctx_t *shifter_init_from_expr(shifter_ctx_t *shctx, const char *expr)
{
    shctx->base.getc = (lex_getc_func_t)expr_getc;
    shctx->base.ungetc = (lex_ungetc_func_t)expr_ungetc;
    shctx->expr = expr;

    shctx->lineno = 1;
    shctx->column = 0;
    return shctx;
}

lex_token_t *shifter(shifter_ctx_t *shctx)
{
    lex_getc_base_t *bx = &shctx->base;

    lex_token_t *token;
    int c;

    while( shctx->base.getc == (lex_getc_func_t)fp_getc ?
           !feof(shctx->fp) : *shctx->expr )
    {
        if( (c = bx->getc(bx)) < 0 ) break;
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
            bx->ungetc(c, bx);
            token = lex_token_parse(shctx->fsm, bx);

            if( !token )
                token = lex_token_match(
                    shctx->puncts, shctx->matched_fallback, bx);

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

