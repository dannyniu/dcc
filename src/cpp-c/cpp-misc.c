/* DannyNiu/NJF, 2026-07-19. Public Domain. */

#include "cpp-c.h"
#include "../langlex/langlex-c.h"
#include "../infra/strvec.h"

static void cpptu_final(cpptu_t *tu)
{
    s2obj_release(tu->macros->pobj);
    s2obj_release(tu->pushlist->pobj);
    s2obj_release(tu->HotFile->pobj);
    s2obj_release(tu->IncPaths->pobj);
    s2obj_release(tu->rope->pobj);
    if( tu->misc ) // for when testing stand-alone pre-processor.
        s2obj_release(tu->misc);
}

cpptu_t *cpptu_create(char *sourcefile, cpptu_t *parent)
{
    cpptu_t *ret = (cpptu_t *)s2gc_obj_alloc(
        S2_OBJ_TYPE_CPPTU, sizeof(cpptu_t));
    FILE *sfp = fopen(sourcefile, "r");
    assert( ret && sfp );

    ret->base.finalf = (s2func_final_t)cpptu_final;

    lex_getc_init_from_fp(&ret->getcx, sfp);
    ret->rope = CreateRopeFromGetc(&ret->getcx.base, RopeCreatFlag_LineConti);
    assert( ret->rope );

    RegexLexFromRope_Init(&ret->ctx_shifter, ret->rope);
    ret->ctx_shifter.regices = CLexElems;
    ret->shifter = (token_shifter_t)RegexLexFromRope_Shift;

    ret->pushlist = s2list_create();
    assert( ret->pushlist );

    ret->lash.sv = NULL;
    ret->lash.ctx_shifter = &ret->ctx_shifter;
    ret->lash.shifter = ret->shifter;

    ret->rescan_stackbase.flags = MACEXP_FLAG_EVALCTX_SOURCE;
    ret->rescan_stackbase.coldlist = &ret->lash;
    ret->rescan_stackbase.coldlist_shifter =
        (token_shifter_t)cppBufferedShifterCoroutine;

    ret->rescan_stackbase.ctx_tu = ret;
    ret->rescan_stackbase.pushlist = ret->pushlist;

    ret->condinc_level = 0;
    memset(ret->condinc_state, 0, sizeof(ret->condinc_state));

    ret->HotFile = s2data_from_str(sourcefile);
    assert( ret->HotFile );

    if( parent )
    {
        parent->Includee = ret;
        ret->IncPaths = (s2list_t *)s2obj_retain(parent->IncPaths->pobj);
        ret->macros = (s2list_t *)s2obj_retain(parent->macros->pobj);
        if( parent->misc ) // for when testing stand-alone pre-processor.
            ret->misc = s2obj_retain(parent->misc);
    }
    else
    {
        ret->IncPaths = s2list_create();
        ret->macros = s2list_create();
    }
    assert( ret->IncPaths && ret->macros );

    return ret;
}

extern strvec_t *ns_rules_ppexpr, *ns_rules_c;

bool ccPreprocInit()
{
    int subret = 0, i;
    for(i=0; CLexElems[i].pattern; i++)
    {
        subret = libregcomp(
            &CLexElems[i].preg,
            CLexElems[i].pattern,
            CLexElems[i].cflags);
        if( subret != 0 ) goto cleanup;
    }

    for(i=0; CNumLexElems[i].pattern; i++)
    {
        subret = libregcomp(
            &CNumLexElems[i].preg,
            CNumLexElems[i].pattern,
            CNumLexElems[i].cflags);
        if( subret != 0 ) goto cleanup;
    }

    ns_rules_ppexpr = strvec_create();
    ns_rules_c = strvec_create();
    if( !ns_rules_ppexpr ) goto cleanup;
    if( !ns_rules_c ) goto cleanup;

    return true;

cleanup:
    if( ns_rules_ppexpr ) s2obj_release(ns_rules_ppexpr->pobj);
    if( ns_rules_c ) s2obj_release(ns_rules_c->pobj);

    for(i=0; CNumLexElems[i].pattern; i++)
    {
        libregfree(&CNumLexElems[i].preg);
    }

    for(i=0; CLexElems[i].pattern; i++)
    {
        libregfree(&CLexElems[i].preg);
    }

    return false;
}

void ccPreprocFin()
{
    int i;

    if( ns_rules_ppexpr ) s2obj_release(ns_rules_ppexpr->pobj);
    if( ns_rules_c ) s2obj_release(ns_rules_c->pobj);

    for(i=0; CNumLexElems[i].pattern; i++)
    {
        libregfree(&CNumLexElems[i].preg);
    }

    for(i=0; CLexElems[i].pattern; i++)
    {
        libregfree(&CLexElems[i].preg);
    }
}
