/* DannyNiu/NJF, 2026-03-21. Public Domain. */

#include "cpp-c.h"

// * {ScanningShifter}:
// - shifting tokens from the token stream,
// - invoke {Expand} on macro names,
//
// * {Expand}:
// - expands the macro,
// - places the expanded sequence on a hotlist,
// - rescan using {ScanningShifter}.

struct cppMacroExpandShifter {
    // Always positioned at the tail.
    s2list_t *pushlist;

    // Always positioned at the head.
    s2list_t *hotlist;

    // a name of macro to be excluded from expansion.
    // only when coldlist == upstack (i.e. in a pass-down context).
    lex_token_t *hotname;

    // coldlist == upstack if in a pass-down context.
    // coldlist is external and upstack is nullptr if the top-most context.
    void *coldlist;
    token_shifter_t coldlist_shifter;
    struct cppMacroExpandShifter *upstack;

    // context pointer to the translation unit.
    void *ctx_tu;
};

static lex_token_t *ChainFallbackShifter(struct cppMacroExpandShifter *ctx)
{
    lex_token_t *ret;
    if( s2list_len(ctx->hotlist) > 0 )
    {
        s2list_shift_T(lex_token_t)(ctx->hotlist, &ret);
        return ret;
    }
    else return ctx->coldlist_shifter(ctx->coldlist);
}

static lex_token_t *ArgTokSeqShifter(s2list_t *arg)
{
    if( s2list_pos(arg) < s2list_len(arg) )
    {
        lex_token_t *ret;
        s2list_get_T(lex_token_t)(arg, &ret);
        return ret;
    }
    else return NULL;
}

static inline int i2hex(int v)
{
    v &= 15;
    if( v < 10 ) return v + '0';
    else return (v - 10) + 'A';
}

static void Quote1Token(s2data_t *out, lex_token_t *telem)
{
    const char *ts = s2data_weakmap(telem->str);
    size_t tl = s2data_len(telem->str);
    size_t i;

    for(i=0; i<tl; i++)
    {
        if( ts[i] == '\'' || ts[i] == '\"' || ts[i] == '\\' )
        {
            s2data_putc(out, '\\');
            s2data_putc(out, ts[i]);
        }
        else if( ts[i] < ' ' ) // assumes ASCII-based charset.
        {
            char hexseq[4] = { '\\', 'x', 'f', 'f' };
            hexseq[2] = i2hex(ts[i] >> 4);
            hexseq[3] = i2hex(ts[i]);
            s2data_puts(out, hexseq, 4);
        }
    }
}

static s2data_t *QuoteTokens(
    s2list_t *tseq) // no guarantee about its current position.
{
    s2data_t *ret = s2data_create(0);
    lex_token_t *telem;
    int sep = '\"';

    s2list_seek(tseq, 0, S2_LIST_SEEK_SET);
    while( s2list_pos(tseq) < s2list_len(tseq) )
    {
        s2list_get_T(lex_token_t)(tseq, &telem);
        s2data_putc(ret, sep);
        Quote1Token(ret, telem);
        sep = ' ';
    }

    s2data_putc(ret, '\"');
    s2data_putfin(ret);
    return ret;
}

static s2list_t *FindArg(
    lex_token_t *argname, cppmacro_t *macdef, s2list_t *args)
{
    s2list_seek(macdef->params, 0, S2_LIST_SEEK_SET);
    while( s2list_pos(macdef->params) < s2list_len(macdef->params) )
    {
        lex_token_t *pn;
        s2list_get_T(lex_token_t)(macdef->params, &pn);

        if( strcmp(s2data_weakmap(argname->str),
                   s2data_weakmap(pn->str)) == 0 )
        {
            s2list_t *ret;
            s2list_seek(args, s2list_pos(macdef->params), S2_LIST_SEEK_SET);
            s2list_get_T(s2list_t)(args, &ret);
            return ret;
        }
    }
    return NULL;
}

static void ScanningRecursion(struct cppMacroExpandShifter *ctx);

static s2list_t *ExpandMacro(
    struct cppMacroExpandShifter *ctx,
    lex_token_t *macname,
    cppmacro_t *macdef,
    s2list_t *args) // a list of lists of tokens.
{
    s2list_t *ret = s2list_create();
    lex_token_t *cur;
    struct s2ctx_list_element *lptr;

    // Would assume single-threaded compilation.
    lptr = macdef->repllist->anch_head.next;

    while( lptr != &macdef->repllist->anch_tail )
    {
        cur = (lex_token_t *)lptr->value;
        if( cur->classification == PPTOK_CLS_ORDINARY )
        {
            s2list_push(ret, cur->pobj, s2_setter_kept);
            continue;
        }

        else if( cur->classification == PPTOK_CLS_PARAMETER )
        {
            // Needed another shifter on collected arguments,
            // that doesn't release any of its contained descendent elements.

            struct cppMacroExpandShifter argeval = *ctx;
            argeval.pushlist = s2list_create();
            argeval.hotlist = NULL;

            // 2026-03-28: TODO variadic macros.
            argeval.coldlist = FindArg(cur, macdef, args);
            argeval.coldlist_shifter = (token_shifter_t)ArgTokSeqShifter;

            while( s2list_pos(argeval.coldlist) <
                   s2list_len(argeval.coldlist) )
            {
                ScanningRecursion(&argeval);
            }

            while( s2list_len(argeval.pushlist) > 0 )
            {
                lex_token_t *tx;
                s2list_shift_T(lex_token_t)(argeval.pushlist, &tx);
                s2list_push(ret, tx->pobj, s2_setter_gave);
            }

            s2obj_release(argeval.pushlist->pobj);
            continue;
        }

        // else, apply concatenation and/or stringification operator(s).

        if( cur->classification & PPTOK_CLS_STRINGIFY )
        {
            s2list_t *arg = FindArg(cur, macdef, args);
            s2data_t *str = QuoteTokens(arg);
            lex_token_t *re = lex_token_create();
            re->str = str;
            re->completion = langlex_strlit;
            // 2026-03-28, for now not relevant:
            // re->lineno = macname->lineno;
            // re->column = macname->column;
            s2list_push(ret, re->pobj, s2_setter_gave);
        }

        if( cur->classification & PPTOK_CLS_CATENATE )
        {
            s2list_t *arg = FindArg(cur, macdef, args);
            lex_token_t *prev, *first;

            // concatenate the first token of the argument,
            // with the last previous token.
            s2list_pop_T(lex_token_t)(ret, &prev); // retrieve, then ...
            s2list_shift_T(lex_token_t)(arg, &first);
            s2data_puts(
                prev->str,
                s2data_weakmap(first->str),
                s2data_len(first->str));
            s2data_putfin(prev->str);
            s2list_push(ret, prev->pobj, s2_setter_gave); // ... put back.

            // then the rest of the argument.
            while( s2list_len(arg) > 0 )
            {
                lex_token_t *tx;
                s2list_shift_T(lex_token_t)(arg, &tx);
                s2list_push(ret, tx->pobj, s2_setter_gave);
            }
            s2obj_release(arg->pobj);
        }
        lptr = lptr->next;
    }

    s2obj_release(args->pobj);
    return ret;
}

static s2list_t *ArgCollect(struct cppMacroExpandShifter *ctx)
{
    // starts from one after the left-parenthesis token,
    // stops at _the_ unmatched right-parenthesis.

    s2list_t *ret = s2list_create();
    s2list_t *argelem = s2list_create();
    int level = 0;

    while( true )
    {
        lex_token_t *px = ctx->coldlist_shifter(ctx->coldlist);
        const char *sv = s2data_weakmap(px->str);
        if( !px )
        {
            ccDiagnoseError(
                ctx->ctx_tu,
                "[%s]: Tokens exhausted during preprocessing!\n",
                __func__);
            s2obj_release(ret->pobj);
            return NULL;
        }

        if( strcmp("(", sv) == 0 )
        {
            level ++;
        }

        else if( strcmp(")", sv) == 0 )
        {
            if( level == 0 )
            {
                s2list_push(ret, argelem->pobj, s2_setter_gave);
                return ret;
            }
            else level --;
        }

        else if( strcmp(",", sv) == 0 && level == 0 )
        {
            s2list_push(ret, argelem->pobj, s2_setter_gave);
            argelem = s2list_create();
        }

        s2list_push(argelem, px->pobj, s2_setter_gave);
        continue;
    }
}

static int findHotname(struct cppMacroExpandShifter *ctx, lex_token_t *name)
{
    while( true )
    {
        if( strcmp(s2data_weakmap(name->str),
                   s2data_weakmap(ctx->hotname->str)) == 0 )
            return true;

        ctx = ctx->upstack;
        if( !ctx ) return false;
    }
}

static void ScanningRecursion(struct cppMacroExpandShifter *ctx)
{
    lex_token_t *tx, *la;
    cppmacro_t *macdef;
    struct cppMacroExpandShifter ctx_passdown;

    tx = ctx->coldlist_shifter(ctx->coldlist);

check_start:
    if( !tx ) return;

    if( findHotname(ctx, tx) )
    {
        s2list_push(ctx->pushlist, tx->pobj, s2_setter_gave);
        return;
    }

    macdef = NULL;
    if( tx->completion == langlex_identifier )
        macdef = cppLookup1Macro(ctx->ctx_tu, tx);

    if( !macdef )
    {
        s2list_push(ctx->pushlist, tx->pobj, s2_setter_gave);
        return;
    }

    la = NULL;
    if( macdef->params )
    {
        la = ctx->coldlist_shifter(ctx->coldlist);
        if( strcmp("(", s2data_weakmap(la->str)) != 0 )
        {
            // Function-like macro not invoked as a function.

            s2list_push(ctx->pushlist, tx->pobj, s2_setter_gave);
            tx = la;
            goto check_start;
            return;
        }
    }

    ctx->hotlist = ExpandMacro(ctx, tx, macdef, la ? ArgCollect(ctx) : NULL);
    s2list_seek(ctx->hotlist, 0, S2_LIST_SEEK_SET);
    s2obj_release(la->pobj);

    ctx_passdown = *ctx;
    ctx_passdown.hotlist = NULL;
    ctx_passdown.hotname = tx;
    ctx_passdown.coldlist = ctx_passdown.upstack = ctx;
    ctx_passdown.coldlist_shifter = (token_shifter_t)ChainFallbackShifter;

    while( s2list_len(ctx->hotlist) > 0 )
    {
        ScanningRecursion(&ctx_passdown);
    }

    s2obj_release(ctx->hotlist->pobj);
}
