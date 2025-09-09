/* DannyNiu/NJF, 2024-12-27. Public Domain. */

#include "lex.h"

static void lex_token_final(lex_token_t *ctx)
{
    s2obj_release(ctx->str->pobj);
}

lex_token_t *lex_token_create()
{
    lex_token_t *ret;
    s2data_t *str;

    if( !(str = s2data_create(0)) )
        return NULL;

    ret = (lex_token_t *)s2gc_obj_alloc(
        S2_OBJ_TYPE_LEXTOKEN, sizeof(lex_token_t));

    if( !ret )
    {
        s2obj_release(str->pobj);
        return NULL;
    }

    ret->str = str;

    // Newly created tokens are initialized to the invalid state.
    ret->completion = lex_exceptional;
    ret->identity = 0;
    ret->classification = 0;
    ret->lineno = ret->column = 0;

    ret->base.itercreatf = NULL;
    ret->base.finalf = (s2func_final_t)lex_token_final;

    return ret;
}

int fp_getc(lex_getc_fp_t *ctx)
{
    return fgetc(ctx->fp);
}

int fp_ungetc(int c, lex_getc_fp_t *ctx)
{
    return ungetc(c, ctx->fp);
}

int expr_getc(lex_getc_str_t *ctx)
{
    int ret = *ctx->expr;
    if( !ret ) return EOF;
    ctx->expr++;
    return ret;
}

int expr_ungetc(int c, lex_getc_str_t *ctx)
{
    if( c >= 0 ) --ctx->expr;
    return c;
}

lex_getc_base_t *lex_getc_init_from_str(lex_getc_str_t *ctx, const char *str)
{
    ctx->base.getc = (lex_getc_func_t)expr_getc;
    ctx->base.ungetc = (lex_ungetc_func_t)expr_ungetc;
    ctx->expr = str;
    return &ctx->base;
}

lex_getc_base_t *lex_getc_init_from_fp(lex_getc_fp_t *ctx, FILE *fp)
{
    ctx->base.getc = (lex_getc_func_t)fp_getc;
    ctx->base.ungetc = (lex_ungetc_func_t)fp_ungetc;
    ctx->fp = fp;
    return &ctx->base;
}
