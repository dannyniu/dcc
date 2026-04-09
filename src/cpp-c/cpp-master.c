/* DannyNiu, 2026-03-29. Public Domain. */

#include "cpp-c.h"
#include <s2ref.h>

void cpptu_macdef_free(cpptu_macdef_t *x)
{
    if( x->mname ) s2obj_release(x->mname->pobj);
    if( x->macdef ) s2obj_release(x->macdef->pobj);
}

static bool areTokensIdentical(lex_token_t *a, lex_token_t *b)
{
    if( strcmp(s2data_weakmap(a->str), s2data_weakmap(b->str)) != 0 )
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
            if( 0 != strcmp(
                    s2data_weakmap( ((lex_token_t *)lpa->value)->str ),
                    s2data_weakmap( ((lex_token_t *)lpb->value)->str )) )
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

        if( strcmp(s2data_weakmap(macro_name->str),
                   s2data_weakmap(md->mname->str)) != 0 )
            continue;

        return md->macdef;
    }

    return NULL;
}
