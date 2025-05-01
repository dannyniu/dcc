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

lex_token_t *lex_token_parse(
    const struct lex_fsm_trans *fsm,
    lex_getc_base_t *fc)
{
    lex_token_t *ret = NULL;
    lexer_state_t state = lex_token_start;
    lexer_state_t next;
    int c;
    int i;

    if( !(ret = lex_token_create()) ) return NULL;

    while( true )
    {
        next = lex_exceptional;
        c = fc->getc(fc);

        for(i=0; fsm[i].now; i++)
        {
            if( fsm[i].now != state )
                continue;

            if( fsm[i].flags == lex_expect_set ||
                fsm[i].flags == lex_expect_compl )
            {
                if( (bool)strchr(fsm[i].expect, c) ==
                    (fsm[i].flags == lex_expect_set) )
                {
                    s2data_putc(ret->str, c);
                    next = fsm[i].next;
                    break; // break to fetch the next character.
                }
            }
        }

        if( next == lex_token_complete )
        {
            break;
        }
        else if( next == lex_exceptional )
        {
            fc->ungetc(c, fc);
            break;
        }
        else
        {
            state = next;
            continue;
        }
    }

    s2data_putfin(ret->str);

    if( s2data_len(ret->str) > 0 )
    {
        ret->completion = state;
        return ret;
    }
    else
    {
        s2obj_release(ret->pobj);
        return NULL;
    }
}

lex_token_t *lex_token_match(
    const char *token_set[],
    lexer_state_t fallback_type,
    lex_getc_base_t *fc)
{
    lex_token_t *ret = NULL;
    size_t t = 0, m;
    int c;
    int i;

    if( !(ret = lex_token_create()) ) return NULL;
    ret->completion = fallback_type;

    while( true )
    {
        c = fc->getc(fc);

        for(i=0, m=0; token_set[i]; i++)
        {
            if( t > 0 )
                if( memcmp(token_set[i], s2data_weakmap(ret->str), t) != 0 )
                    continue;

            if( t < strlen(token_set[i]) && token_set[i][t] == c )
            {
                m++; t++;
                s2data_putc(ret->str, c);
                break;
            }
        }

        if( m == 0 )
            break;
        else continue;
    }

    fc->ungetc(c, fc);
    s2data_putfin(ret->str);
    if( s2data_len(ret->str) == 0 )
    {
        s2obj_release(ret->pobj);
        ret = NULL;
    }

    return ret;
}
