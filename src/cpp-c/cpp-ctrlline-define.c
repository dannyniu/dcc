/* DannyNiu/NJF, 2026-03-21. Public Domain. */

// Implements the `#define ...` directive.

#include "cpp-c.h"

static bool isTokenParam(s2list_t *params, lex_token_t *tok)
{
    bool ret = false;
    if( !params ) return false;

    s2list_seek(params, 0, S2_LIST_SEEK_SET);
    while( s2list_pos(params) < s2list_len(params) )
    {
        lex_token_t *p;
        s2list_get_T(lex_token_t)(params, &p);

        if( s2data_cmp(p->str, tok->str) == 0 )
        {
            ret = true;
        }

        s2list_seek(params, 1, S2_LIST_SEEK_CUR);
    }
    return ret;
}

static bool look_ahead_for_genuine_newline(RegexLexContext *ctx)
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

static void cppMacroFinal(cppmacro_t *ctx)
{
    if( ctx->repllist ) s2obj_release(ctx->repllist->pobj);
    if( ctx->params ) s2obj_release(ctx->params->pobj);
}

int cppProcessDefineDirective(
    cpptu_t *restrict ctx_tu,
    void *restrict ctx_shifter,
    token_shifter_t shifter)
{
    // Directive was `define`, handle the rest of the line.

    cppmacro_t *macrodef;
    lex_token_t *macro_name;
    lex_token_t *tok, *tok1;
    s2list_t *funcmacro_params = NULL;
    s2list_t *repllist;
    int is_variadic = false;
    int next_cls = 0;

    RegexLexContext *shifter_rope = ctx_shifter;
    assert( shifter == (token_shifter_t)RegexLexFromRope_Shift );

    macro_name = shifter(ctx_shifter);
    if( macro_name->completion != langlex_identifier )
    {
        ccDiagnoseError(ctx_tu,
                        "[%s]: Token `%s` at line %d byte %d "
                        "is not a valid identifier for a macro name!\n",
                        __func__,
                        (const char *)s2data_weakmap(macro_name->str),
                        macro_name->lineno, macro_name->column);
    }

    eprintf("Start define 1 macro `%s`: ",
            (char *)s2data_weakmap(macro_name->str));

    tok = shifter(ctx_shifter);

    if( strcmp("(", s2data_weakmap(tok->str)) == 0 &&
        macro_name->lineno == tok->lineno &&
        macro_name->column + s2data_len(macro_name->str) == (size_t)tok->column )
    {
        // It's a lparen - i.e. with no adjacent whitespace.
        // So it's a function-like macro.

        funcmacro_params = s2list_create();
        s2obj_release(tok->pobj);

        while( true )
        {
            // remembering parameter identifiers.
            tok = shifter(ctx_shifter);

            if( tok->completion == langlex_identifier )
            {
                // 2026-03-23 TODO: Check for duplicates.
                s2list_push(funcmacro_params, tok->pobj, s2_setter_gave);
            }
            else if( strcmp("...", s2data_weakmap(tok->str)) == 0 )
            {
                is_variadic = true;
            }
            else if( strcmp(")", s2data_weakmap(tok->str)) == 0 )
            {
                s2obj_release(tok->pobj);
                break;
            }
            else
            {
                ccDiagnoseError(ctx_tu,
                                "[%s]: Token `%s` at line %d byte %d "
                                "is not a valid identifier "
                                "for a macro parameter!\n",
                                __func__,
                                (const char *)s2data_weakmap(macro_name->str),
                                macro_name->lineno, macro_name->column);
                // 2026-03-21 TODO: more graceful error handling?
            }

            // looking out for comma and right-parenthesis delimiters.
            tok1 = shifter(ctx_shifter);

            if( strcmp(")", s2data_weakmap(tok1->str)) == 0 )
            {
                s2obj_release(tok1->pobj);
                break;
            }
            else if( strcmp(",", s2data_weakmap(tok1->str)) == 0 )
            {
                s2obj_release(tok1->pobj);
                continue;
            }
            else
            {
                ccDiagnoseError(
                    ctx_tu,"[%s]: Expected `)` or `,`.\n", __func__);
                // 2026-03-21 TODO: more graceful error handling?
            }
        }

        tok = shifter(ctx_shifter);
    }

    // start building (transcribing - i.e. shallow compile) replacement list.
    repllist = s2list_create();

    if( strcmp("##", s2data_weakmap(tok->str)) == 0 )
    {
        ccDiagnoseError(ctx_tu,
                        "[%s]: The replacement list cannot begin with `##`. "
                        "The offending macro definition was `%s` "
                        "at line %d byte %d.\n",
                        __func__,
                        (const char *)s2data_weakmap(macro_name->str),
                        macro_name->lineno, macro_name->column);
    }
    
    for( ; tok; tok = look_ahead_for_genuine_newline(shifter_rope) ?
             NULL : // encountered a newline, the repllist is terminated.
             shifter(ctx_shifter) )
    {
        // 2026-03-28: This loop is definitely in error, rewriting it!
        // 
        // invariants:
        // - `tok` is the current token,
        // - `tok1` is the previous one, already in `repllist`.
        // - `next_cls` applies to the next token,
        //   it's set on encountering operator tokens,
        //   and cleared by the end of the loop body.

        eprintf("%s ", (char *)s2data_weakmap(tok->str));

        if( strcmp("##", s2data_weakmap(tok->str)) == 0 )
        {
            next_cls |= PPTOK_CLS_CATENATE;
            assert( tok1 ); // there must exist a first operand to the `##`-op.
            tok1->classification |= PPTOK_CLS_OPERAND;
            s2obj_release(tok->pobj);
            continue;
        }
        else if( strcmp("#", s2data_weakmap(tok->str)) == 0 &&
                 funcmacro_params )
        {
            next_cls |= PPTOK_CLS_STRINGIFY;
            s2obj_release(tok->pobj);
            continue;
        }

        if( isTokenParam(funcmacro_params, tok) )
        {
            tok->classification |= PPTOK_CLS_PARAMETER;
        }
        else if( next_cls & PPTOK_CLS_STRINGIFY )
        {
            ccDiagnoseError(ctx_tu,
                            "[%s]: The `#` operator expects a parameter "
                            "but `%s` is not a parameter.\n",
                            __func__,
                            s2data_weakmap(tok->str));
        }

        tok1 = tok;
        tok->classification |= next_cls;
        s2list_push(repllist, tok->pobj, s2_setter_gave);
        next_cls = 0;
    }

    if( next_cls )
    {
        ccDiagnoseError(ctx_tu,
                        "[%s]: The replacement list ended with "
                        "a dangling operator.\n", __func__);
    }

    macrodef = (cppmacro_t *)s2gc_obj_alloc(
        S2_OBJ_TYPE_CPPMACRO, sizeof(cppmacro_t));

    macrodef->base.finalf = (s2func_final_t)cppMacroFinal;

    macrodef->repllist = repllist;
    macrodef->params = funcmacro_params;
    macrodef->is_variadic = is_variadic;

    eprintf("\nmacro `%s` is now defined.\n",
            (char *)s2data_weakmap(macro_name->str));

    return cppDefine1Macro(ctx_tu, macro_name, macrodef);
}
