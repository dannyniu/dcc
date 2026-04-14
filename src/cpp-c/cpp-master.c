/* DannyNiu, 2026-03-29. Public Domain. */

#include "cpp-c.h"
#include <s2ref.h>
#include <stdarg.h>

void ccDiagnoseError(cpptu_t *restrict ctx_tu, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[Error]: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    (void)ctx_tu;
}

void ccDiagnoseWarning(cpptu_t *restrict ctx_tu, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "[Warning]: ");
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    (void)ctx_tu;
}

void cpptu_macdef_free(cpptu_macdef_t *x)
{
    if( x->mname ) s2obj_release(x->mname->pobj);
    if( x->macdef ) s2obj_release(x->macdef->pobj);
    free(x);
}

static bool areTokensIdentical(lex_token_t *a, lex_token_t *b)
{
    if( s2data_cmp(a->str, b->str) != 0 )
        return false;

    if( a->classification != b->classification )
        return false;

    return true; // for the purpose of preprocessing, these are sufficient.
}

static bool areMacrosIdentical(cppmacro_t *a, cppmacro_t *b)
{
    struct s2ctx_list_element *lpa, *lpb;

    if( s2list_len(a->repllist) != s2list_len(b->repllist) ) return false;

    lpa = a->repllist->anch_head.next;
    lpb = b->repllist->anch_head.next;

    while( lpa != &a->repllist->anch_tail )
    {
        if( !areTokensIdentical(
                (lex_token_t *)lpa->value,
                (lex_token_t *)lpb->value) )
            return false;

        lpa = lpa->next;
        lpb = lpb->next;
    }

    if( a->params && !a->params == !b->params )
    {
        lpa = a->params->anch_head.next;
        lpb = b->params->anch_head.next;

        while( lpa != &a->repllist->anch_tail )
        {
            if( 0 != s2data_cmp(
                    ((lex_token_t *)lpa->value)->str ,
                    ((lex_token_t *)lpb->value)->str ) )
                return false;

            lpa = lpa->next;
            lpb = lpb->next;
        }

        return a->is_variadic == b->is_variadic;
    }

    return true;
}

int cppDefine1Macro(
    cpptu_t *restrict ctx_tu,
    lex_token_t *restrict macro_name,
    cppmacro_t *restrict macrodef)
{
    cppmacro_t *olddef;
    cpptu_macdef_t *macdef = calloc(1, sizeof(cpptu_macdef_t));

    olddef = cppLookup1Macro(ctx_tu, macro_name);
    if( olddef && !areMacrosIdentical(olddef, macrodef) )
    {
        ccDiagnoseWarning(
            ctx_tu, "[%s]: The macro `%s` is being redefined at line %d.\n",
            __func__,
            (const char *)s2data_weakmap(macro_name->str),
            macro_name->lineno);
    }

    macdef->mname = macro_name;
    macdef->macdef = macrodef;
    return s2list_insert(
        ctx_tu->macros,
        s2ref_create(
            macdef,
            (s2ref_final_func_t)
            cpptu_macdef_free)->pobj,
        s2_setter_gave);
}

int cppUndef1Macro(
    cpptu_t *restrict ctx_tu,
    lex_token_t *restrict macro_name)
{
    cpptu_macdef_t *macdef = calloc(1, sizeof(cpptu_macdef_t));

    macdef->mname = macro_name;
    macdef->macdef = NULL;
    return s2list_insert(
        ctx_tu->macros,
        s2ref_create(
            macdef,
            (s2ref_final_func_t)
            cpptu_macdef_free)->pobj,
        s2_setter_gave);
}

cppmacro_t *cppLookup1Macro(
    cpptu_t *restrict ctx_tu,
    lex_token_t *restrict macro_name)
{
    struct s2ctx_list_element *lptr;

    lptr = ctx_tu->macros->anch_head.next;

    while( lptr != &ctx_tu->macros->anch_tail )
    {
        cpptu_macdef_t *md = s2ref_unwrap((s2ref_t *)lptr->value);

        if( s2data_cmp(macro_name->str,
                       md->mname->str) != 0 )
        {
            lptr = lptr->next;
            continue;
        }

        return md->macdef;
    }

    return NULL;
}

static bool is_first_of_the_line(RegexLexContext *ctx)
{
    // look for the first non-blank character, and:
    // - determine whether there's a genuine newline, or
    // - if it's the beginning of the source code.

    char *src = s2data_weakmap(ctx->rope->sourcecode);
    ptrdiff_t len = s2data_len(ctx->rope->sourcecode);
    ptrdiff_t t = ctx->offsub;
    bool ret = false;

    while( true )
    {
        if( t >= len ) break;

        if( src[t] == '\n' || t == 0 )
            ret = true;

        if( src[t] == '\\' )
        {
            if( t + 1 < len &&
                src[t + 1] == '\n' )
            {
                t += 2;
                continue;
            }

            if( t + 2 < len &&
                src[t + 1] == '\r' &&
                src[t + 2] == '\n' )
            {
                t += 3;
                continue;
            }
        }

        if( isspace(src[t]) )
        {
            t++;
            continue;
        }
        return ret;
    }

    return ret;
}

lex_token_t *cppBufferedShifterCoroutine(struct cppBufferedShifter *ctx)
{
    lex_token_t *ret = ctx->sv;
    if( ret )
    {
        ctx->sv = NULL;
        return ret;
    }
    else return ctx->shifter(ctx->ctx_shifter);
}

lex_token_t *cppMainProgramCoroutine(cpptu_t *ctx_tu)
{
    lex_token_t *tok = NULL;

    static long cntr = 0;
    Reached("CPP Main Counter: %ld.\n", ++cntr);

    if( s2list_len(ctx_tu->pushlist) > 0 )
    {
        s2list_seek(ctx_tu->pushlist, 0, S2_LIST_SEEK_SET);
        s2list_shift_T(lex_token_t)(ctx_tu->pushlist, &tok);
        return tok; // Emptying the `pushlist` positions it to the tail.
    }

    while( true )
    {
        int first_of_a_line = is_first_of_the_line(ctx_tu->ctx_shifter);
        const char *tokstr;

        Reached("FoL: %c\n", first_of_a_line ? '+' : '-');

        if( !(tok = ctx_tu->lash.shifter(ctx_tu->lash.ctx_shifter)) )
        {
            if( 0 == s2list_len(ctx_tu->pushlist) ) break; else
            {
                s2list_shift_T(lex_token_t)(ctx_tu->pushlist, &tok);
                return tok;
            }
        }

        tokstr = s2data_weakmap(tok->str);
        Reached("%s\n", tokstr);

        if( !first_of_a_line || 0 != strcmp("#", tokstr) )
        {
            // Not a pre-processing directive,
            // and ought to be handled by macro-expand.
            ctx_tu->lash.sv = tok;
            return cppTokenJet(&ctx_tu->rescan_stackbase);
        }

        // At this point, it'll be certain that
        // it's a pre-processing control line.

        s2obj_release(tok->pobj);
        tok = ctx_tu->lash.shifter(ctx_tu->lash.ctx_shifter);
        tokstr = s2data_weakmap(tok->str);
        Reached("%s\n", tokstr);

        if( 0 == strcmp("define", tokstr) )
        {
            Reached(" _reached 1_\n");
            cppProcessDefineDirective(
                ctx_tu, ctx_tu->ctx_shifter, ctx_tu->shifter);
        }
        else if( 0 == strcmp("undef", tokstr) )
        {
            assert( 0 ); // not testing this yet.
            Reached(" _reached 2_\n");
            cppUndef1Macro(ctx_tu, tok);
        }

        s2obj_release(tok->pobj);
    }

    return NULL;
}
