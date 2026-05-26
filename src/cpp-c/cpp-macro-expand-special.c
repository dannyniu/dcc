/* DannyNiu/NJF, 2026-05-16. Public Domain. */

#include "cpp-c.h"

const char *special_funclike_macros[] = {
    "defined", "__has_include",
    NULL,
};

const char *special_objlike_macros[] = {
    "__STDC__",
    "__STDC_VERSION__",
    NULL,
};

bool SpecialEval_Defined(lex_token_t *identifier)
{
    const char *tokstr  = s2data_weakmap(identifier->str);
    int i;

    for(i=0; special_funclike_macros[i]; i++)
    {
        if( 0 == strcmp(special_funclike_macros[i], tokstr) )
        {
            return true;
        }
    }

    for(i=0; special_objlike_macros[i]; i++)
    {
        if( 0 == strcmp(special_objlike_macros[i], tokstr) )
        {
            return true;
        }
    }

    return false;
}

s2list_t *ExpandSpecial(
    struct cppMacroExpandShifter *ctx,
    lex_token_t *macname,
    s2list_t *args) // a list of lists of tokens.
{
    s2list_t *ret = s2list_create();
    lex_token_t *cur;
    const char *mname = s2data_weakmap(macname->str);

    if( 0 == strcmp(mname, "defined") )
    {
        s2list_t *atoks;
        lex_token_t *da;

        if( s2list_len(args) != 1 )
        {
            // 2026-05-16: or too few.
            ccDiagnoseError(ctx->ctx_tu, "[%s]: Too many arguments "
                            "for the `defined` operator "
                            "at line %d, column %d.\n",
                            __func__, macname->lineno, macname->column);
        }

        s2list_seek(args, 0, S2_LIST_SEEK_SET);
        s2list_get_T(s2list_t)(args, &atoks);

        if( s2list_len(atoks) != 1 )
        {
            // 2026-05-16: or too few.
            ccDiagnoseError(ctx->ctx_tu, "[%s]: Too many tokens "
                            "in the arguments for the `defined` operator "
                            "at line %d, column %d.\n",
                            __func__, macname->lineno, macname->column);
        }

        s2list_seek(args, 0, S2_LIST_SEEK_SET);
        s2list_get_T(lex_token_t)(atoks, &da);

        if( SpecialEval_Defined(da) )
        {
            cur = lex_token_create();
            s2data_putc(cur->str, '1');
            s2data_putfin(cur->str);
            cur->completion = langlex_declit;
            cur->lineno = macname->lineno;
            cur->column = macname->lineno;
        }
        else
        {
            cur = lex_token_create();
            s2data_putc(cur->str, '0');
            s2data_putfin(cur->str);
            cur->completion = langlex_octlit;
            cur->lineno = macname->lineno;
            cur->column = macname->lineno;
        }

        s2list_push(ret, cur->pobj, s2_setter_gave);
    }
    else
    {
        ccDiagnoseError(ctx->ctx_tu, "[%s]: Unrecognized special macro "
                        "at line %d, column %d. Perhaps the plan to "
                        "implement them is underway?\n",
                        __func__, macname->lineno, macname->column);
    }

    if( args ) s2obj_release(args->pobj);
    return ret;
}
