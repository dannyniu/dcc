/* DannyNiu/NJF, 2026-01-09. Public Domain. */

#include "cxing-interp.h"
#include "../langlex/langlex-cxing.h"

#define NS_RULES ns_rules_cxing
#define var_lex_elems LexElems

bool CXParserInit()
{
    int subret = 0, i;
    for(i=0; var_lex_elems[i].pattern; i++)
    {
        subret = libregcomp(
            &var_lex_elems[i].preg,
            var_lex_elems[i].pattern,
            var_lex_elems[i].cflags);
        if( subret != 0 ) goto cleanup;
    }
    
    NS_RULES = strvec_create();
    if( !NS_RULES )
        goto cleanup;

    return true;

cleanup:
    if( NS_RULES ) s2obj_release(NS_RULES->pobj);

    for(i=0; var_lex_elems[i].pattern; i++)
    {
        libregfree(&var_lex_elems[i].preg);
        // 2026-01-09:
        // `libregfree` hasn't been able to set `re_atoms` to
        // NULL, thus further attempts at re-initialization
        // can and would cause finalization to UB.
        // TODO: to inform librematch of the necessary fix.
    }

    return false;
}

void CXParserFinal()
{
    int i;

    for(i=0; var_lex_elems[i].pattern; i++)
    {
        libregfree(&var_lex_elems[i].preg);
    }
    
    lalr_parse_accel_cache_clear();
    s2obj_release(NS_RULES->pobj);
}
