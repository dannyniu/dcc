/* DannyNiu/NJF, 2025-08-09. Public Domain. */

#include "rope.h"
#include <s2obj.h>

static void rope_final(source_rope_t *rope)
{
    s2obj_release(rope->sourcecode->pobj);
    s2obj_release(rope->linedelims->pobj);
}

source_rope_t *CreateRopeFromGetc(lex_getc_base_t *getcctx, int flags)
{
    source_rope_t *ret;
    s2data_t *src, *lin;
    size_t p;

    ret = (source_rope_t *)s2gc_obj_alloc(
        S2_OBJ_TYPE_SRCROPE, sizeof(source_rope_t));
    src = s2data_create(0);
    lin = s2data_create(0);

    if( !ret || !src || !lin )
    {
        if( src ) s2obj_release(src->pobj);
        if( lin ) s2obj_release(lin->pobj);
        if( ret ) s2gc_obj_dealloc(ret->pobj);
        return NULL;
    }

    ret->sourcecode = src;
    ret->linedelims = lin;
    ret->base.finalf = (s2func_final_t)rope_final;

    // 2025-08-17: shifting the responsibility of handling errors.

    while( true )
    {
        int c = getcctx->getc(getcctx);
        if( c < 0 ) break;

        if( c == '\\' && (flags & RopeCreatFlag_LineConti) )
        {
            c = getcctx->getc(getcctx);
            if( c != '\n' )
            {
                s2data_putc(src, '\\');
                if( c >= 0 )
                    s2data_putc(src, c);
                else break;
            }

            else
            {
                s2data_putfin(src);
                p = s2data_len(src);
                s2data_puts(lin, &p, sizeof(p));
            }
        }
        else
        {
            s2data_putc(src, c);
            if( c == '\n' )
            {
                s2data_putfin(src);
                p = s2data_len(src);
                s2data_puts(lin, &p, sizeof(p));
            }
        }
    }

    s2data_putfin(src);
    s2data_putfin(lin);

    return ret;
}

source_rope_t *CreateRopeFromLineData(s2data_t *src, int semantic)
{
    source_rope_t *ret;
    s2data_t *lin;

    ret = (source_rope_t *)s2gc_obj_alloc(
        S2_OBJ_TYPE_SRCROPE, sizeof(source_rope_t));
    lin = s2data_create(0);

    if( !ret || !src || !lin )
    {
        if( lin ) s2obj_release(lin->pobj);
        if( ret ) s2gc_obj_dealloc(ret->pobj);
        return NULL;
    }

    if( semantic == s2_setter_kept )
        s2obj_retain(src->pobj);

    ret->sourcecode = src;
    ret->linedelims = lin;
    ret->base.finalf = (s2func_final_t)rope_final;

    return ret;
}

void RegexLexFromRope_Init(RegexLexContext *ctx, source_rope_t *rope)
{
    ctx->offsub = 0;
    ctx->srcchars = s2data_len(rope->sourcecode);
    ctx->lineind = 0;
    ctx->linecnt = s2data_len(rope->linedelims) / sizeof(ptrdiff_t);
    ctx->rope = rope;
}

lex_token_t *RegexLexFromRope_Shift(RegexLexContext *ctx)
{
    libre_match_t matched;
    ptrdiff_t *lin = s2data_weakmap(ctx->rope->linedelims);
    char *src = s2data_weakmap(ctx->rope->sourcecode);
    lex_token_t *ret = NULL;

    while( true )
    {
        if( (size_t)ctx->offsub >= ctx->srcchars )
        {
            /*
            ctx->logger_base.logger(
                &ctx->logger_base,
                "[Regex Lexing Shifter]: Source code string exhausted.");//*/
            break; // EOF.
        }

        if( !isspace(src[ctx->offsub]) )
        {
            int i, subret;
            int record = -1;
            ptrdiff_t best = -1;

            for(i=0; ctx->regices[i].pattern; i++)
            {
                subret = libregexec(
                    &ctx->regices[i].preg,
                    src+ctx->offsub,
                    1, &matched, ctx->regices[i].eflags);
                if( subret == 0 && matched.rm_so == 0 )
                {
                    if( matched.rm_eo > best )
                    {
                        best = matched.rm_eo;
                        record = i;
                    }
                }
            }

            // Lex elements earlier in the list have
            // higher precedence, so don't use `>=`.
            if( record > 0 )
            {
                i = record;
                matched.rm_so = 0;
                matched.rm_eo = best;
            }

            if( !ctx->regices[i].pattern )
            {
                ctx->logger_base.logger(
                    &ctx->logger_base,
                    "[Regex Lexing Shifter]: Unrecognized Token.");
                return NULL;
            }

            if( !(ret = lex_token_create()) )
            {
                ctx->logger_base.logger(
                    &ctx->logger_base,
                    "[Regex Lexing Shifter]: Unable to Create Object.");
                return NULL;
            }

            // 2025-08-17: shifting the responsibility of handling errors.
            s2data_puts(ret->str, src + ctx->offsub,
                        matched.rm_eo - matched.rm_so);
            s2data_putfin(ret->str);
            //- fprintf(stderr, "%s\n", s2data_weakmap(ret->str));

            ret->completion = ctx->regices[i].completion;
            ret->lineno = ctx->lineind + 1;
            ret->column =
                ctx->offsub  + 1 -
                (ctx->lineind > 0 ? lin[ctx->lineind - 1] : 0);
            ret->identity = ret->classification = 0;
            ctx->offsub += matched.rm_eo - matched.rm_so;
        }
        else ctx->offsub ++;

        while( ctx->lineind < ctx->linecnt )
        {
            if( lin[ctx->lineind] > ctx->offsub )
                break;
            ctx->lineind ++;
        }
        if( ret ) break;
    }
    return ret;
}
