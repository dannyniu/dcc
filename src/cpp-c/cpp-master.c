/* DannyNiu, 2026-03-29. Public Domain. */

#include "cpp-c.h"
#include "../pgm-cc/c-grammar.h"
#include <s2ref.h>
#include <stdarg.h>

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
        ccDiagnoseWarn(ctx_tu, "Macro redefined", spelling_and_site(macro_name));
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

bool look_ahead_for_genuine_newline(RegexLexContext *ctx)
{
    const char *src = s2data_weakmap(ctx->rope->sourcecode);
    ptrdiff_t len = s2data_len(ctx->rope->sourcecode);
    ptrdiff_t t = ctx->offsub;

    while( true )
    {
        if( t >= len ) break;

        if( src[t] == '\n' )
            return true;

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
        else return false;
    }

    return true;
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

lex_token_t *cppDirectivesDispatch(cpptu_t *ctx_tu)
{
    lex_token_t *tok = NULL;
    bool space_delimit_next_token = false;

    static long cntr = 0; (void)cntr;
    Reached("CPP Cnd-Inc Counter: %ld.\n", ++cntr);

    if( s2list_len(ctx_tu->pushlist) > 0 )
    {
        s2list_seek(ctx_tu->pushlist, 0, S2_LIST_SEEK_SET);
        s2list_shift_T(lex_token_t)(ctx_tu->pushlist, &tok);

        if( PPTokGraduate(tok) != 0 )
        {
            ccDiagnoseError(ctx_tu, "Invalid Token", spelling_and_site(tok));
        }
        return tok; // Emptying the `pushlist` positions it to the tail.
    }

    while( true )
    {
        int pred;
        int first_of_a_line;
        const char *tokstr;

        if( ctx_tu->Includee )
        {
            // 2026-07-19:
            // now we're inside an include file.
            tok = cppDirectivesDispatch(ctx_tu->Includee);
            if( !tok )
            {
                ctx_tu->count_errors += ctx_tu->Includee->count_errors;
                ctx_tu->count_warnings += ctx_tu->Includee->count_warnings;
                s2obj_release(ctx_tu->Includee->pobj);
                ctx_tu->Includee = NULL;
            }
            else return tok;
        }

        first_of_a_line = is_first_of_the_line(&ctx_tu->ctx_shifter);
        Reached("FoL: %c\n", first_of_a_line ? '+' : '-');

        if( !(tok = ctx_tu->lash.shifter(ctx_tu->lash.ctx_shifter)) )
        {
            if( 0 == s2list_len(ctx_tu->pushlist) )
                break;

            // else
            s2list_shift_T(lex_token_t)(ctx_tu->pushlist, &tok);

            if( ctx_tu->condinc_state[ctx_tu->condinc_level] == CONDINC_INITIAL )
            {
                // This line group is not skipped.
                if( PPTokGraduate(tok) != 0 )
                {
                    ccDiagnoseError(ctx_tu, "Invalid Token", spelling_and_site(tok));
                }
                return tok;
            }
            else
            {
                // This is a skipped conditional inclusion line group.
                s2obj_release(tok->pobj);
                continue;
            }
        }

        tokstr = s2data_weakmap(tok->str);
        Reached("%s\n", tokstr);

        // 2026-07-21:
        // At this point, this token is certain to take effect.

        if( tok->completion == langlex_comment )
        {
            // 2026-07-21:
            // An essential mid-processing, as comments are
            // considered whitespaces before such significance
            // is lost during syntax parsing phase.
            space_delimit_next_token = true;
            if( strncmp(tokstr, "//", 2) == 0 )
                ctx_tu->ctx_shifter.offsub -= 1; // save 1 'genuine' newline.
            s2obj_release(tok->pobj);
            continue;
        }
        else if( space_delimit_next_token )
        {
            // 2026-07-21:
            // This assertion is so that in case other non-flag attrs
            // are added in the future.
            assert( tok->attrs == 0 || tok->attrs == 1 );

            tok->attrs |= TOKATTR_BLANKDELIM;
            space_delimit_next_token = false;
        }
        // In effect, the above clauses implements comment stripping.

        if( !first_of_a_line || 0 != strcmp("#", tokstr) )
        {
            // Not a pre-processing directive,
            // and ought to be handled by macro-expand.
            ctx_tu->lash.sv = tok;
            tok = cppTokenJet(&ctx_tu->rescan_stackbase);

            if( ctx_tu->condinc_state[ctx_tu->condinc_level] == CONDINC_INITIAL )
            {
                // This line group is not skipped.
                if( PPTokGraduate(tok) != 0 )
                {
                    ccDiagnoseError(ctx_tu, "Invalid Token", spelling_and_site(tok));
                }
                return tok;
            }
            else
            {
                // This is a skipped conditional inclusion line group.
                s2obj_release(tok->pobj);
                continue;
            }
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

            if( ctx_tu->condinc_state[ctx_tu->condinc_level] == CONDINC_INITIAL )
            {
                s2list_t *mdef = s2list_create();
                CtrlLine_ArgCollect(
                    mdef, ctx_tu, &ctx_tu->ctx_shifter, ctx_tu->shifter);
                cppProcessDefineDirective(
                    ctx_tu, mdef, (token_shifter_t)shift_from_s2list);
                s2obj_release(mdef->pobj);
            }
        }
        else if( 0 == strcmp("include", tokstr) )
        {
            if( ctx_tu->condinc_state[ctx_tu->condinc_level] == CONDINC_INITIAL )
            {
                s2list_t *mdef = s2list_create();
                CtrlLine_ArgCollect(
                    mdef, ctx_tu, &ctx_tu->ctx_shifter, ctx_tu->shifter);
                cppProcessIncludeDirective(
                    ctx_tu, mdef, (token_shifter_t)shift_from_s2list);
                s2obj_release(mdef->pobj);
            }
        }
        else if( 0 == strcmp("undef", tokstr) )
        {
            Reached(" _reached 2_\n");

            if( ctx_tu->condinc_state[ctx_tu->condinc_level] == CONDINC_INITIAL )
            {
                s2list_t *mdef = s2list_create();
                lex_token_t *tx;
                CtrlLine_ArgCollect(
                    mdef, ctx_tu, &ctx_tu->ctx_shifter, ctx_tu->shifter);
                s2list_seek(mdef, 0, S2_LIST_SEEK_SET);
                s2list_get_T(lex_token_t)(mdef, &tx);

                if( s2list_len(mdef) != 1 ||
                    tx->completion != langlex_identifier )
                {
                    ccDiagnoseError(ctx_tu, "Expected one identifier", spelling_and_site(tx));
                }

                cppUndef1Macro(ctx_tu, (lex_token_t *)s2obj_retain(tx->pobj));
                s2obj_release(mdef->pobj);
            }
        }

        else if( 0 == strcmp("error", tokstr) )
        {
            if( ctx_tu->condinc_state[ctx_tu->condinc_level] == CONDINC_INITIAL )
            {
                s2list_t *mdef = s2list_create();
                lex_token_t *tx;
                CtrlLine_ArgCollect(
                    mdef, ctx_tu, &ctx_tu->ctx_shifter, ctx_tu->shifter);
                s2list_seek(mdef, 0, S2_LIST_SEEK_SET);

                ccDiagnoseError(ctx_tu, "Error directive:", spelling_and_site(tok));
                while( s2list_shift_T(lex_token_t)(mdef, &tx) == s2_access_success )
                {
                    ccDiagnose(elog_raw, " %s", s2data_weakmap(tx->str));
                    s2obj_release(tx->pobj);
                }
                ccDiagnose(elog_raw, "\n");

                s2obj_release(mdef->pobj);
                ctx_tu->count_errors ++;
            }
        }

        else if( 0 == strcmp("warning", tokstr) )
        {
            if( ctx_tu->condinc_state[ctx_tu->condinc_level] == CONDINC_INITIAL )
            {
                s2list_t *mdef = s2list_create();
                lex_token_t *tx;
                CtrlLine_ArgCollect(
                    mdef, ctx_tu, &ctx_tu->ctx_shifter, ctx_tu->shifter);
                s2list_seek(mdef, 0, S2_LIST_SEEK_SET);

                ccDiagnoseWarn(ctx_tu, "Warning directive:", spelling_and_site(tok));
                while( s2list_shift_T(lex_token_t)(mdef, &tx) == s2_access_success )
                {
                    ccDiagnose(elog_raw, " %s", s2data_weakmap(tx->str));
                    s2obj_release(tx->pobj);
                }
                ccDiagnose(elog_raw, "\n");

                s2obj_release(mdef->pobj);
                ctx_tu->count_warnings ++;
            }
        }

        // 2026-05-14 TODO:
        // Remaining as of 2026-07-19:
        // - embed (next 1st recent).
        // - pragma (to be planned).
        else if( 0 == strcmp("if", tokstr) )
        {
            if( ctx_tu->condinc_state[ctx_tu->condinc_level] == CONDINC_INITIAL )
            {
                s2list_t *mdef = s2list_create();
                CtrlLine_ArgCollect(
                    mdef, ctx_tu, &ctx_tu->ctx_shifter, ctx_tu->shifter);
                pred = cppEvaluateCtrlExpr(
                    ctx_tu, mdef, (token_shifter_t)shift_from_s2list);
                s2obj_release(mdef->pobj);

                // Should be `_Countof`, but (assert?) it's a single byte anyway.
                assert( ctx_tu->condinc_level < sizeof ctx_tu->condinc_state );

                // 2026-05-16:
                //
                // Q: specific implementation behavior / actions:
                // A: see "cpp-c.h", notes below the `CONDINC_*` macros.

                if( pred )
                {
                    ctx_tu->condinc_state[ctx_tu->condinc_level] = CONDINC_INCLUDED;
                    ctx_tu->condinc_level ++;
                    ctx_tu->condinc_state[ctx_tu->condinc_level] = CONDINC_INITIAL;
                }
                else
                {
                    ctx_tu->condinc_state[ctx_tu->condinc_level] = CONDINC_TRYNEXT;
                    ctx_tu->condinc_level ++;
                    ctx_tu->condinc_state[ctx_tu->condinc_level] = CONDINC_SUPPRESS;
                }
            }
            else if( ctx_tu->condinc_state[ctx_tu->condinc_level] == CONDINC_SUPPRESS )
            {
                ctx_tu->condinc_level ++;
                ctx_tu->condinc_state[ctx_tu->condinc_level] = CONDINC_SUPPRESS;
            }
            else assert( 0 );
        }

        else if( 0 == strcmp("ifdef", tokstr) ||
                 0 == strcmp("ifndef", tokstr) )
        {
            assert( 0 );
            // TODO 2026-07-24: This block is new, being worked on!
            if( ctx_tu->condinc_state[ctx_tu->condinc_level] == CONDINC_INITIAL )
            {
                s2list_t *mdef = s2list_create();
                lex_token_t *tx;
                CtrlLine_ArgCollect(
                    mdef, ctx_tu, &ctx_tu->ctx_shifter, ctx_tu->shifter);
                s2list_seek(mdef, 0, S2_LIST_SEEK_SET);
                s2list_get_T(lex_token_t)(mdef, &tx);

                if( s2list_len(mdef) != 1 ||
                    tx->completion != langlex_identifier )
                {
                    ccDiagnoseError(ctx_tu, "Expected one identifier", spelling_and_site(tx));
                }

                pred = !cppLookup1Macro(
                    ctx_tu, (lex_token_t *)s2obj_retain(tx->pobj));
                if( 0 == strcmp("ifdef", tokstr) ) pred = !pred;

                s2obj_release(mdef->pobj);

                // Should be `_Countof`, but (assert?) it's a single byte anyway.
                assert( ctx_tu->condinc_level < sizeof ctx_tu->condinc_state );

                // 2026-05-16:
                //
                // Q: specific implementation behavior / actions:
                // A: see "cpp-c.h", notes below the `CONDINC_*` macros.

                if( pred )
                {
                    ctx_tu->condinc_state[ctx_tu->condinc_level] = CONDINC_INCLUDED;
                    ctx_tu->condinc_level ++;
                    ctx_tu->condinc_state[ctx_tu->condinc_level] = CONDINC_INITIAL;
                }
                else
                {
                    ctx_tu->condinc_state[ctx_tu->condinc_level] = CONDINC_TRYNEXT;
                    ctx_tu->condinc_level ++;
                    ctx_tu->condinc_state[ctx_tu->condinc_level] = CONDINC_SUPPRESS;
                }
            }
            else if( ctx_tu->condinc_state[ctx_tu->condinc_level] == CONDINC_SUPPRESS )
            {
                ctx_tu->condinc_level ++;
                ctx_tu->condinc_state[ctx_tu->condinc_level] = CONDINC_SUPPRESS;
            }
            else assert( 0 );
        }

        else if( 0 == strcmp("elifdef", tokstr) ||
                 0 == strcmp("elifndef", tokstr) )
        {
            ctx_tu->condinc_level --;
            if( ctx_tu->condinc_state[ctx_tu->condinc_level] == CONDINC_TRYNEXT )
            {
                s2list_t *mdef = s2list_create();
                lex_token_t *tx;
                CtrlLine_ArgCollect(
                    mdef, ctx_tu, &ctx_tu->ctx_shifter, ctx_tu->shifter);
                s2list_seek(mdef, 0, S2_LIST_SEEK_SET);
                s2list_get_T(lex_token_t)(mdef, &tx);

                if( s2list_len(mdef) != 1 ||
                    tx->completion != langlex_identifier )
                {
                    ccDiagnoseError(ctx_tu, "Expected one identifier", spelling_and_site(tx));
                }

                pred = !cppLookup1Macro(
                    ctx_tu, (lex_token_t *)s2obj_retain(tx->pobj));
                if( 0 == strcmp("elifdef", tokstr) ) pred = !pred;

                s2obj_release(mdef->pobj);

                // Should be `_Countof`, but (assert?) it's a single byte anyway.
                assert( ctx_tu->condinc_level < sizeof ctx_tu->condinc_state );

                // 2026-05-16:
                //
                // Q: specific implementation behavior / actions:
                // A: see "cpp-c.h", notes below the `CONDINC_*` macros.

                if( pred )
                {
                    ctx_tu->condinc_state[ctx_tu->condinc_level] = CONDINC_INCLUDED;
                    ctx_tu->condinc_level ++;
                    ctx_tu->condinc_state[ctx_tu->condinc_level] = CONDINC_INITIAL;
                }
                else
                {
                    ctx_tu->condinc_state[ctx_tu->condinc_level] = CONDINC_TRYNEXT;
                    ctx_tu->condinc_level ++;
                    ctx_tu->condinc_state[ctx_tu->condinc_level] = CONDINC_SUPPRESS;
                }
            }
            else if( ctx_tu->condinc_state[ctx_tu->condinc_level] == CONDINC_INCLUDED )
            {
                ctx_tu->condinc_level ++;
                ctx_tu->condinc_state[ctx_tu->condinc_level] = CONDINC_SUPPRESS;
            }
            else if( ctx_tu->condinc_state[ctx_tu->condinc_level] == CONDINC_SUPPRESS )
            {
                ctx_tu->condinc_level ++;
                ctx_tu->condinc_state[ctx_tu->condinc_level] = CONDINC_SUPPRESS;
            }
            else
            {
                ccDiagnoseError(ctx_tu, "Unpaired preprocessing group", spelling_and_site(tok));
            }
        }

        else if( 0 == strcmp("elif", tokstr) )
        {
            ctx_tu->condinc_level --;
            if( ctx_tu->condinc_state[ctx_tu->condinc_level] == CONDINC_TRYNEXT )
            {
                s2list_t *mdef = s2list_create();
                CtrlLine_ArgCollect(
                    mdef, ctx_tu, &ctx_tu->ctx_shifter, ctx_tu->shifter);
                pred = cppEvaluateCtrlExpr(
                    ctx_tu, mdef, (token_shifter_t)shift_from_s2list);
                s2obj_release(mdef->pobj);

                // Should be `_Countof`, but (assert?) it's a single byte anyway.
                assert( ctx_tu->condinc_level < sizeof ctx_tu->condinc_state );

                // 2026-05-16:
                //
                // Q: specific implementation behavior / actions:
                // A: see "cpp-c.h", notes below the `CONDINC_*` macros.

                if( pred )
                {
                    ctx_tu->condinc_state[ctx_tu->condinc_level] = CONDINC_INCLUDED;
                    ctx_tu->condinc_level ++;
                    ctx_tu->condinc_state[ctx_tu->condinc_level] = CONDINC_INITIAL;
                }
                else
                {
                    ctx_tu->condinc_state[ctx_tu->condinc_level] = CONDINC_TRYNEXT;
                    ctx_tu->condinc_level ++;
                    ctx_tu->condinc_state[ctx_tu->condinc_level] = CONDINC_SUPPRESS;
                }
            }
            else if( ctx_tu->condinc_state[ctx_tu->condinc_level] == CONDINC_INCLUDED )
            {
                ctx_tu->condinc_level ++;
                ctx_tu->condinc_state[ctx_tu->condinc_level] = CONDINC_SUPPRESS;
            }
            else if( ctx_tu->condinc_state[ctx_tu->condinc_level] == CONDINC_SUPPRESS )
            {
                ctx_tu->condinc_level ++;
                ctx_tu->condinc_state[ctx_tu->condinc_level] = CONDINC_SUPPRESS;
            }
            else
            {
                ccDiagnoseError(ctx_tu, "Unpaired preprocessing group", spelling_and_site(tok));
            }
        }

        else if( 0 == strcmp("else", tokstr) )
        {
            ctx_tu->condinc_level --;
            if( ctx_tu->condinc_state[ctx_tu->condinc_level] == CONDINC_TRYNEXT )
            {
                // Should be `_Countof`, but (assert?) it's a single byte anyway.
                assert( ctx_tu->condinc_level < sizeof ctx_tu->condinc_state );

                ctx_tu->condinc_state[ctx_tu->condinc_level] = CONDINC_INCLUDED;
                ctx_tu->condinc_level ++;
                ctx_tu->condinc_state[ctx_tu->condinc_level] = CONDINC_INITIAL;
            }
            else if( ctx_tu->condinc_state[ctx_tu->condinc_level] == CONDINC_INCLUDED )
            {
                ctx_tu->condinc_level ++;
                ctx_tu->condinc_state[ctx_tu->condinc_level] = CONDINC_SUPPRESS;
            }
            else if( ctx_tu->condinc_state[ctx_tu->condinc_level] == CONDINC_SUPPRESS )
            {
                ctx_tu->condinc_level ++;
                ctx_tu->condinc_state[ctx_tu->condinc_level] = CONDINC_SUPPRESS;
            }
            else
            {
                ccDiagnoseError(ctx_tu, "Unpaired preprocessing group", spelling_and_site(tok));
            }
        }

        else if( 0 == strcmp("endif", tokstr) )
        {
            ctx_tu->condinc_level --;
            if( ctx_tu->condinc_state[ctx_tu->condinc_level] == CONDINC_INITIAL )
            {
                ccDiagnoseError(ctx_tu, "Unpaired preprocessing group", spelling_and_site(tok));
            }
            ctx_tu->condinc_state[ctx_tu->condinc_level] = CONDINC_INITIAL;
            assert( ctx_tu->condinc_level >= 0 );
        }

        s2obj_release(tok->pobj);
    }

    return NULL;
}

// 2026-07-24:
// The token, if of the `langlex_strlit` completion, is:
#define STRLIT_CLS_ORD 4 // ordinary
#define STRLIT_CLS_WIDE 5 // wide
#define STRLIT_CLS_UTF16 6 // is UTF-16 encoded
#define STRLIT_CLS_UTF32 7 // is UTF-32 encoded
#define STRLIT_CLS_UTF8 8 // is UTF-8 encoded

// Added 2026-07-29.
#define theRule c_grammar_rules[prod->semantic_rule]

// Added 2026-07-29.
static bool find_typedef_spec(lalr_prod_t *prod)
{
    while( prod )
    {
        if( theRule == declspecs_genrule )
        {
            if( find_typedef_spec(prod->terms[0].production) )
                return true;

            prod = prod->terms[1].production;
            continue;
        }
        else if( theRule == stor_cls_spec_typedef )
        {
            return true;
        }
        else return false;
    }

    return false;
}

// Added 2026-07-29.
static bool find_typedef_name(lalr_prod_t *prod, lex_token_t *ident)
{
    while( prod )
    {
        if( theRule == blk_item_list_genrule )
        {
            if( find_typedef_name(prod->terms[1].production, ident) )
                return true;

            prod = prod->terms[0].production;
            continue;
        }

        if( theRule == TU_genrule )
        {
            if( find_typedef_name(prod->terms[1].production, ident) )
                return true;

            prod = prod->terms[0].production;
            continue;
        }

        if( theRule == selstmt_header_declexpr )
        {
            return find_typedef_name(prod->terms[0].production, ident);
        }

        if( theRule == decl_decl )
        {
            // 1st, look for `typedef` specifier.
            if( !find_typedef_spec(prod->terms[0].production) )
                return false;

            // then traverse the declarators list.
            prod = prod->terms[1].production;
            continue;
        }

        if( theRule == decl_with_attr )
        {
            // 1st, look for `typedef` specifier.
            if( !find_typedef_spec(prod->terms[1].production) )
                return false;

            // then traverse the declarators list.
            prod = prod->terms[2].production;
            continue;
        }

        if( theRule == initdecls_genrule )
        {
            if( find_typedef_name(prod->terms[1].production, ident) )
                return true;

            prod = prod->terms[0].production;
            continue;
        }

        if( theRule == direct_declarator_ident )
        {
            if( s2data_cmp(
                    prod->terms[0].production->terms[0].terminal->str,
                    ident->str) == 0 )
                return true;
        }

        if( theRule == direct_declarator_paren ||
            theRule == declarator_pointer )
        {
            prod = prod->terms[1].production;
            continue;
        }

        if( theRule == array_declarator_classic ||
                 theRule == array_declarator_static ||
                 theRule == array_declarator_qual ||
                 theRule == array_declarator_asterisk ||
                 theRule == function_declarator_funcdecl )
        {
            prod = prod->terms[0].production;
            continue;
        }

        return false;
    }

    return false;
}

lex_token_t *cppMainProgramCoroutine(cpptu_t *ctx_tu)
{
    lex_token_t *ret;
    lex_token_t *cooked = NULL;

    ret = cppDirectivesDispatch(ctx_tu);
    if( !ret ) return NULL;

    if( ret->completion == langlex_identifier )
    {
        // The disambiguation between typedef-name and other
        // non-tag identifiers - colloquially known as the
        // lexer hack, had been implemented on 2026-07-29.

        lalr_stack_t *ps = (lalr_stack_t *)*ctx_tu->misc;
        lalr_term_t *te = ps->top;

        while( te && !s2_is_prod(te->production) ) te = te->dn;
            
        while( te )
        {
            if( te && find_typedef_name(te->production, ret) )
            {
                ret->completion = lexer_hack_typedef_name;
                break;
            }

            do { te = te->dn; } while( te && !s2_is_prod(te->production) );
        }
    }

    while( ret->completion == langlex_strlit )
    {
        int rcls;
        char *rstr;
        size_t rlen;

        if( !cooked )
        {
            cooked = lex_token_create(); // TODO (still) 2026-07-21: handle error.
            cooked->lineno = ret->lineno;
            cooked->column = ret->column;
            cooked->identity = TOKIDENT_DEQUOTED; // Concat actually.
            cooked->completion = langlex_str_cooked;
            cooked->classification = STRLIT_CLS_ORD;
        }

        rstr = s2data_weakmap(ret->str);
        rlen = s2data_len(ret->str);
        rcls = -1;
        if( strncmp(rstr, "\"", 1) == 0 ) rcls = STRLIT_CLS_ORD;
        if( strncmp(rstr, "L\"", 2) == 0 ) rcls = STRLIT_CLS_WIDE;
        if( strncmp(rstr, "u\"", 2) == 0 ) rcls = STRLIT_CLS_UTF16;
        if( strncmp(rstr, "U\"", 2) == 0 ) rcls = STRLIT_CLS_UTF32;
        if( strncmp(rstr, "u8\"", 3) == 0 ) rcls = STRLIT_CLS_UTF8;

        if( cooked->classification == STRLIT_CLS_ORD )
        {
            cooked->classification = rcls;
        }
        else
        {
            ccDiagnoseError(ctx_tu, "Inconsistent kinds of string literals", spelling_and_site(ret));
        }

        if( rcls == STRLIT_CLS_UTF8 )
            s2data_puts(cooked->str, rstr+2, rlen-2);
        else if( rcls == STRLIT_CLS_ORD )
            s2data_puts(cooked->str, rstr, rlen);
        else s2data_puts(cooked->str, rstr+1, rlen-1);

        ret = cppDirectivesDispatch(ctx_tu);
        if( !ret ) break;
    }

    if( cooked )
    {
        s2list_seek(ctx_tu->pushlist, 0, S2_LIST_SEEK_SET);
        s2list_insert(ctx_tu->pushlist, ret->pobj, s2_setter_gave);
        return cooked;
    }
    else return ret;
}
