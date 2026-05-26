/* DannyNiu/NJF, 2026-03-21. Public Domain. */

#include "cpp-c.h"

extern const char *special_funclike_macros[];
extern const char *special_objlike_macros[];

// * {ScanningShifter}:
// - shifting tokens from the token stream,
// - invoke {Expand} on macro names,
// Actual implementation is a scanning-'recursion', which
// pushes the expansion onto a 'list'.
//
// * {Expand}:
// - expands the macro,
// - places the expanded sequence on a hotlist,
// - rescan using {ScanningShifter}.

#define PRINT_STACK_DEPTH() /* do {                                     \
                            printf("- %td -\n",                         \
                            (char *)ctx - (char *)&ctx->ctx_tu->rescan_stackbase); \
                            } while( false ) */

static lex_token_t *ChainFallbackShifter(struct cppMacroExpandShifter *ctx)
{
    lex_token_t *ret;
    PRINT_STACK_DEPTH();

    if( s2list_len(ctx->hotlist) > 0 )
    {
        s2list_shift_T(lex_token_t)(ctx->hotlist, &ret);
        Reached("HotShift: `%s`\n", ret ? (char *)s2data_weakmap(ret->str) : NULL);
        return ret;
    }
    else
    {
        ret = ctx->coldlist_shifter(ctx->coldlist);
        Reached("HotShift: `%s`\n", ret ? (char *)s2data_weakmap(ret->str) : NULL);
        return ret;
    }
}

struct MacroArgPointer
{
    s2list_t *args_found; // Not owned (or thus released).
    struct s2ctx_list_element *argp; // weak ref.
};

static lex_token_t *ArgTokSeqShifter(struct MacroArgPointer *arg)
{
    lex_token_t *ret;
    if( arg->argp == &arg->args_found->anch_tail )
        return NULL;

    // 2026-04-13: there's still a copy in `arg->args_found`.
    ret = (lex_token_t *)s2obj_retain(arg->argp->value);
    arg->argp = arg->argp->next;
    return ret;
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
        else s2data_putc(out, ts[i]);
    }
}

static s2data_t *QuoteTokens(
    s2data_t *ret,
    s2list_t *tseq) // no guarantee about its current position.
{
    lex_token_t *telem;
    int sep = '\"';

    s2list_seek(tseq, 0, S2_LIST_SEEK_SET);
    while( s2list_pos(tseq) < s2list_len(tseq) )
    {
        s2list_get_T(lex_token_t)(tseq, &telem);
        s2data_putc(ret, sep);
        Quote1Token(ret, telem);
        sep = ' ';
        s2list_seek(tseq, 1, S2_LIST_SEEK_CUR);
    }

    s2data_putc(ret, '\"');
    s2data_putfin(ret);
    return ret;
}

static struct MacroArgPointer *FindArg(
    struct MacroArgPointer *out,
    lex_token_t *argname,
    cppmacro_t *macdef,
    s2list_t *args)
{
    s2list_seek(macdef->params, 0, S2_LIST_SEEK_SET);
    while( s2list_pos(macdef->params) < s2list_len(macdef->params) )
    {
        lex_token_t *pn;
        s2list_get_T(lex_token_t)(macdef->params, &pn);

        if( s2data_cmp(argname->str, pn->str) == 0 )
        {
            s2list_t *ret;
            s2list_seek(args, s2list_pos(macdef->params), S2_LIST_SEEK_SET);
            s2list_get_T(s2list_t)(args, &ret);

            out->args_found = ret;
            out->argp = ret->anch_head.next;
            return out;
        }

        s2list_seek(macdef->params, 1, S2_LIST_SEEK_CUR);
    }
    out->args_found = NULL;
    out->argp = NULL;
    return NULL;
}

static void ScanningRecursion(struct cppMacroExpandShifter *ctx);

bool SpecialEval_Defined(lex_token_t *identifier);

s2list_t *ExpandSpecial(
    struct cppMacroExpandShifter *ctx,
    lex_token_t *macname,
    s2list_t *args); // a list of lists of tokens.

static s2list_t *ExpandMacro(
    struct cppMacroExpandShifter *ctx,
    lex_token_t *macname,
    cppmacro_t *macdef,
    s2list_t *args) // a list of lists of tokens.
{
    s2list_t *ret = NULL;
    lex_token_t *cur;
    struct s2ctx_list_element *lptr;

    PRINT_STACK_DEPTH();

    if( ctx->flags == MACEXP_FLAG_EVALCTX_CTRLLINE )
    {
        // This callee doesn't release `args`.
        ret = ExpandSpecial(ctx, macname, args);
        if( ret )
        {
            if( args ) s2obj_release(args->pobj);
            return ret;
        }
    }

    ret = s2list_create();

    // Would assume single-threaded compilation.
    lptr = macdef->repllist->anch_head.next;

    while( lptr != &macdef->repllist->anch_tail )
    {
        cur = (lex_token_t *)lptr->value;
        Reached("cur: `%s`/%d.\n",
                (char *)s2data_weakmap(cur->str),
                cur->classification);

        if( cur->classification == PPTOK_CLS_ORDINARY ||
            cur->classification == PPTOK_CLS_OPERAND )
        {
            Reached("MacExp.Push: `%s`,\n", (char *)s2data_weakmap(cur->str));
            s2list_push(ret, cur->pobj, s2_setter_kept);
            lptr = lptr->next;
            continue;
        }

        else if( cur->classification == PPTOK_CLS_PARAMETER )
        {
            // Needed another shifter on collected arguments,
            // that doesn't release any of its contained descendent elements.

            struct cppMacroExpandShifter argeval = *ctx;
            struct MacroArgPointer argptr;
            argeval.pushlist = s2list_create();
            argeval.hotlist = NULL;

            assert( args ); // object-like macros cannot have parameters.

            // 2026-03-28: TODO variadic macros.
            argeval.coldlist = FindArg(&argptr, cur, macdef, args);
            argeval.coldlist_shifter = (token_shifter_t)ArgTokSeqShifter;
            Reached("MacExp.Para: `%s`,\n", (char *)s2data_weakmap(cur->str));

            while( argptr.argp != &argptr.args_found->anch_tail )
            {
                Reached("Calling ScanningRecursion from MacExp.\n");
                ScanningRecursion(&argeval);
                Reached("ScanningRecursion Retruned to MacExp.\n");
            }

            s2list_seek(argeval.pushlist, 0, S2_LIST_SEEK_SET);
            while( s2list_len(argeval.pushlist) > 0 )
            {
                lex_token_t *tx;
                s2list_shift_T(lex_token_t)(argeval.pushlist, &tx);
                Reached("MacExp.Push: `%s`,\n", (char *)s2data_weakmap(tx->str));
                s2list_push(ret, tx->pobj, s2_setter_gave);
            }

            s2obj_release(argeval.pushlist->pobj);
            lptr = lptr->next;
            continue;
        }

        else if( cur->classification ==
                 (PPTOK_CLS_PARAMETER |
                  PPTOK_CLS_OPERAND) )
        {
            // 2026-04-14: Arguments taken literally (i.e. not evaluated).
            // The comments in this block dated earlier are carried over.

            struct MacroArgPointer argptr;

            assert( args ); // object-like macros cannot have parameters.

            // 2026-03-28: TODO variadic macros.
            FindArg(&argptr, cur, macdef, args);
            Reached("MacExp.Para: `%s`,\n", (char *)s2data_weakmap(cur->str));

            s2list_seek(argptr.args_found, 0, S2_LIST_SEEK_SET);
            while( s2list_pos(argptr.args_found) <
                   s2list_len(argptr.args_found) )
            {
                lex_token_t *tx;
                s2list_get_T(lex_token_t)(argptr.args_found, &tx);
                s2list_seek(argptr.args_found, 1, S2_LIST_SEEK_CUR);
                //- s2list_shift_T(lex_token_t)(argptr.args_found, &tx);
                Reached("MacExp.Push: `%s`,\n", (char *)s2data_weakmap(tx->str));
                s2list_push(ret, tx->pobj, s2_setter_kept);
            }

            lptr = lptr->next;
            continue;
        }

        Reached("CurCls: %d.\n", cur->classification);

        // else, apply concatenation and/or stringification operator(s).

        if( cur->classification & PPTOK_CLS_STRINGIFY )
        {
            struct MacroArgPointer arg;
            lex_token_t *re;

            FindArg(&arg, cur, macdef, args);
            re = lex_token_create();
            QuoteTokens(re->str, arg.args_found);
            re->completion = langlex_strlit;
            re->lineno = macname->lineno;
            re->column = macname->column;
            Reached("MacExp.Push: `%s`,\n", (char *)s2data_weakmap(re->str));
            s2list_push(ret, re->pobj, s2_setter_gave);
        }

        if( cur->classification & PPTOK_CLS_CATENATE )
        {
            lex_token_t *prev, *first;
            if( args ) // Function-like Macro.
            {
                struct MacroArgPointer arg;
                FindArg(&arg, cur, macdef, args);

                // concatenate the first token of the argument,
                // with the last previous token.
                s2list_pop_T(lex_token_t)(ret, &prev); // retrieve, then ...
                first = lex_token_create();
                s2data_puts(
                    first->str,
                    s2data_weakmap(prev->str),
                    s2data_len(prev->str));
                first->completion = prev->completion;
                first->lineno = prev->lineno;
                first->column = prev->column;
                s2obj_release(prev->pobj);
                prev = first;

                s2list_seek(arg.args_found, 0, S2_LIST_SEEK_SET);
                s2list_get_T(lex_token_t)(arg.args_found, &first);
                s2list_seek(arg.args_found, 1, S2_LIST_SEEK_CUR);

                s2data_puts(
                    prev->str,
                    s2data_weakmap(first->str),
                    s2data_len(first->str));
                s2data_putfin(prev->str);
                Reached("MacExp.Push: `%s`,\n", (char *)s2data_weakmap(prev->str));
                s2list_push(ret, prev->pobj, s2_setter_gave); // ... put back.

                // then the rest of the argument.
                while( s2list_pos(arg.args_found) < s2list_len(arg.args_found) )
                {
                    lex_token_t *tx;
                    s2list_get_T(lex_token_t)(arg.args_found, &tx);
                    s2list_seek(arg.args_found, 1, S2_LIST_SEEK_CUR);
                    Reached("MacExp.Push: `%s`,\n", (char *)s2data_weakmap(tx->str));
                    s2list_push(ret, tx->pobj, s2_setter_kept);
                }
            }
            else // Object-like Macro.
            {
                // concatenate the first token of `cur`,
                // with the last previous token.
                Reached(" len(ret): %td.\n", s2list_len(ret));
                s2list_pop_T(lex_token_t)(ret, &prev); // retrieve, then ...
                s2data_puts(
                    prev->str,
                    s2data_weakmap(cur->str),
                    s2data_len(cur->str));
                s2data_putfin(prev->str);
                Reached("MacExp.Push: `%s`,\n", (char *)s2data_weakmap(prev->str));
                s2list_push(ret, prev->pobj, s2_setter_gave);
            }
        }

        Reached("MacExp.ReplList.Shift.\n");
        lptr = lptr->next;
    }

    if( args ) s2obj_release(args->pobj);
    return ret;
}

static s2list_t *ArgCollect(struct cppMacroExpandShifter *ctx)
{
    // starts from one after the left-parenthesis token,
    // stops at _the_ unmatched right-parenthesis.

    s2list_t *ret = s2list_create();
    s2list_t *argelem = s2list_create();
    int level = 0;

    PRINT_STACK_DEPTH();

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

        eprintf("%s ", (char *)s2data_weakmap(px->str));

        if( strcmp("(", sv) == 0 )
        {
            level ++;
        }

        else if( strcmp(")", sv) == 0 )
        {
            if( level == 0 )
            {
                eprintf("@( 1 last arg )\n");
                s2list_push(ret, argelem->pobj, s2_setter_gave);
                s2obj_release(px->pobj);
                return ret;
            }
            else level --;
        }

        else if( strcmp(",", sv) == 0 && level == 0 )
        {
            eprintf("@( 1 more arg )\n");
            s2list_push(ret, argelem->pobj, s2_setter_gave);
            argelem = s2list_create();
            s2obj_release(px->pobj);
            continue;
        }

        s2list_push(argelem, px->pobj, s2_setter_gave);
        continue;
    }
}

static int findHotname(struct cppMacroExpandShifter *ctx, lex_token_t *name)
{
    if( !name ) return false;

    while( true )
    {
        if( !ctx->hotname ) return false;

        if( s2data_cmp(name->str, ctx->hotname->str) == 0 )
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

    // 0: not special,
    // 1: special func-like,
    // 2: special obj-like.
    int macdef_special;

    PRINT_STACK_DEPTH();

    tx = ctx->coldlist_shifter(ctx->coldlist);

check_start:
    if( !tx ) return;

    if( findHotname(ctx, tx) )
    {
        Reached("hot-token: `%s`.\n", (char *)s2data_weakmap(tx->str));
        s2list_push(ctx->pushlist, tx->pobj, s2_setter_gave); // should've been gave, 2026-05-14: but was kept.
        return;
    }

    macdef = NULL;
    macdef_special = false;
    if( tx->completion == langlex_identifier )
    {
        // 2026-05-16 TODO:
        // Handle special macros: defined(), __has_include(), and other __has_*().
        // Doing some right now.
        // 2026-05-16:
        // Additionally, there's a form of `defined` without parenthesis.
        // Think about how to handle that.
        macdef = cppLookup1Macro(ctx->ctx_tu, tx);

        if( ctx->flags == MACEXP_FLAG_EVALCTX_CTRLLINE )
        {
            int i;
            const char *tokstr = s2data_weakmap(tx->str);

            for(i=0; special_funclike_macros[i]; i++)
            {
                if( 0 == strcmp(special_funclike_macros[i], tokstr) )
                {
                    macdef_special = 1;
                    break;
                }
            }

            for(i=0; special_objlike_macros[i]; i++)
            {
                if( 0 == strcmp(special_objlike_macros[i], tokstr) )
                {
                    macdef_special = 2;
                    break;
                }
            }
        }
    }
    if( !macdef && !macdef_special )
    {
        Reached("lang-token: `%s`.\n", (char *)s2data_weakmap(tx->str));
        eprintf(" ScnRecr pllp: %td, %td.\n", s2list_pos(ctx->pushlist), s2list_len(ctx->pushlist));
        s2list_push(ctx->pushlist, tx->pobj, s2_setter_gave);
        return;
    }

    la = NULL;
    if( macdef->params || macdef_special == 1 )
    {
        la = ctx->coldlist_shifter(ctx->coldlist);
        if( !la || strcmp("(", s2data_weakmap(la->str)) != 0 )
        {
            if( 0 == strcmp("defined", s2data_weakmap(tx->str)) &&
                la && la->completion == langlex_identifier )
            {
                // Added 2026-05-16:
                // This is a special case for the `defined <identifier>` form.
                s2obj_release(tx->pobj);

                tx = lex_token_create();
                if( SpecialEval_Defined(la) )
                {
                    s2data_putc(tx->str, '1');
                    s2data_putfin(tx->str);
                    tx->completion = langlex_declit;
                    tx->lineno = la->lineno;
                    tx->column = la->lineno;
                }
                else
                {
                    s2data_putc(tx->str, '0');
                    s2data_putfin(tx->str);
                    tx->completion = langlex_octlit;
                    tx->lineno = la->lineno;
                    tx->column = la->lineno;
                }

                s2list_push(ctx->pushlist, tx->pobj, s2_setter_gave);
                s2obj_release(la->pobj);
                return;
            }
            else
            {
                Reached("Function-like macro `%s` not invoked as a function.\n",
                        (char *)s2data_weakmap(tx->str));
                s2list_push(ctx->pushlist, tx->pobj, s2_setter_gave);
                tx = la;
                goto check_start;

                // 2026-05-16:
                // Unlike the preceeding clause, `la` could be the introducer
                // of the next one. Therefore we need to goto the start to
                // check for sure.
            }
        }
    }

    eprintf(" Expanding Macro%s.\n", la ? " with Collected Arguments" : "");
    ctx->hotlist = ExpandMacro(ctx, tx, macdef, la ? ArgCollect(ctx) : NULL);
    eprintf(" Got a replacement list with %td elements.\n", s2list_len(ctx->hotlist));
    s2list_seek(ctx->hotlist, 0, S2_LIST_SEEK_SET);

    if( la ) { s2obj_release(la->pobj); la = NULL; }

    ctx_passdown = *ctx;
    ctx_passdown.hotlist = NULL;
    ctx_passdown.hotname = tx;
    ctx_passdown.coldlist = ctx_passdown.upstack = ctx;
    ctx_passdown.coldlist_shifter = (token_shifter_t)ChainFallbackShifter;

    while( s2list_len(ctx->hotlist) > 0 )
    {
        Reached("Calling ScanningRecursion Directly.\n");
        ScanningRecursion(&ctx_passdown);
        Reached("ScanningRecursion Retruned Directly.\n");
    }

    s2obj_release(tx->pobj);
    s2obj_release(ctx->hotlist->pobj);
}

lex_token_t *cppTokenJet(struct cppMacroExpandShifter *ctx)
{
    lex_token_t *ret = NULL;

    eprintf("TokenJet pllp: %td, %td.\n",
            s2list_pos(ctx->pushlist),
            s2list_len(ctx->pushlist));

    if( s2list_len(ctx->pushlist) <= 0 )
        ScanningRecursion(ctx);

    s2list_seek(ctx->pushlist, 0, S2_LIST_SEEK_SET);
    if( s2list_len(ctx->pushlist) > 0 )
    {
        // Recall that it's always positioned at tail.
        s2list_shift_T(lex_token_t)(ctx->pushlist, &ret);
        return ret;
    }
    // Emptying the `pushlist` ensures it's positioned at the end.

    return NULL;
}
