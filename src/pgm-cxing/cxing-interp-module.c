/* DannyNiu/NJF, 2026-01-02. Public Domain. */

#include "../langlex/langlex-cxing.h"
#include "cxing-grammar.h"
#define GRAMMAR_RULES cxing_grammar_rules
#define NS_RULES ns_rules_cxing
#define var_lex_elems LexElems

#include "../lex-common/rope.h"
#include "../infra/strvec.h"
#include "../lalr-common/lalr.h"

#include "runtime.h"
#include "cxing-interp.h"
#include <s2list.h>
#include <stdio.h>

static void cxing_module_final(cxing_module_t *module)
{
    if( module->entities ) s2obj_release(module->entities->pobj);
    if( module->linked ) s2obj_release(module->linked->pobj);

    if( module->CallStubs )
    {
        cxing_callxfer_bridge.free_callstub_memory(
            module->CallStubs,
            module->func_defs_cnt,
            cxing_callxfer_bridge.sz_callstub);
    }

    if( module->SymTab )
    {
        size_t t;
        for(t=0; t<module->func_defs_cnt; t++)
        {
            s2data_unmap(module->SymTab[t]);
            s2obj_release(module->SymTab[t]->pobj);
        }
        free(module->SymTab);
    }
}

static cxing_module_t *cxing_module_create()
{
    cxing_module_t *ret;

    ret = (cxing_module_t *)s2gc_obj_alloc(
        S2_OBJ_TYPE_CXING_MODULE,
        sizeof(cxing_module_t));

    if( !ret ) return NULL;

    ret->entities = s2dict_create();
    ret->linked = s2dict_create();

    if( !ret->entities || !ret->linked )
    {
        cxing_module_final(ret);
        return NULL;
    }

    ret->base.finalf = (s2func_final_t)cxing_module_final;
    return ret;
}

#define entRule(x) (cxing_grammar_rules[(x)->semantic_rule])

// A return of `true` means OK, no conflict,
// whereas `false` means some error(s) exist(s).
static bool CheckFuncDefConflict(
    lex_token_t *restrict ss, // symbol name string,
    lalr_prod_t *restrict de, // in-coming definition,
    lalr_prod_t *restrict se) // stored existing definition if any.
{
    // 1. Check that they're not both definitions,
    // 2. Check that they're both subroutines or both methods,
    // 3. Check that the definition, if any, has a well-formed function body.
    if( entRule(se->terms[3].production) == stmt_brace )
    {
        if( entRule(de->terms[3].production) == stmt_brace )
        {
            CxingDebug("[CheckFuncDefConflict]: Redefinition of "
                       "function '%s' at line %d column %d\n",
                       ss->str, ss->lineno, ss->column);
            return false;
        }
    }

    if( s2data_cmp(de->terms[0].terminal->str,
                   se->terms[0].terminal->str) != 0 )
    {
        CxingDebug("[CheckFuncDefConflict]: A function cannot be "
                   "both subroutine and method at the same time: "
                   "'%s' at line %d column %d\n",
                   ss->str, ss->lineno, ss->column);
        return false;
    }

    if( entRule(de->terms[3].production) != stmt_brace &&
        entRule(de->terms[3].production) != stmt_emptystmt )
    {
        CxingDebug("[CheckFuncDefConflict]: Malformed function body: "
                   "'%s' at line %d column %d\n",
                   ss->str, ss->lineno, ss->column);
        return false;
    }

    if( entRule(se->terms[3].production) != stmt_brace &&
        entRule(se->terms[3].production) != stmt_emptystmt )
    {
        CxingDebug("[CheckFuncDefConflict]: A previously undiagnosed "
                   "malformed function body: "
                   "'%s' at line %d column %d\n",
                   ss->str, ss->lineno, ss->column);
        return false;
    }

    return true;
}

static bool ProcessTU(
    cxing_module_t *restrict module,
    lalr_stack_t *restrict parsed,
    long *func_defs_cnt)
{
    lalr_prod_t *de; // the definition.
    lalr_prod_t *fe; // subject to release.
    lalr_prod_t *se; // the whatever already stored entity info.
    lex_token_t *ss; // symbol string,
    s2list_t *tu = s2list_create();
    int subret, cont = true;
    bool ret = true;

    de = parsed->bottom->production; // repurposed a bit.
    while( true )
    {
        if( cxing_grammar_rules[de->semantic_rule] == TU_genrule )
        {
            subret = s2list_insert(
                tu, de->terms[1].production->pobj, s2_setter_kept);
            de = de->terms[0].production;
            if( subret != s2_access_success )
            {
                CxingFatal("[ProcessTU]: Unable to insert "
                           "entity declaration(s) "
                           "onto the buffer list.");

                s2obj_release(tu->pobj);
                return -1;
            }
        }
        else
        {
            subret = s2list_insert(tu, de->pobj, s2_setter_kept);
            if( subret != s2_access_success )
            {
                CxingFatal("[ProcessTU]: Unable to insert "
                           "entity declaration(s) "
                           "onto the buffer list.");

                s2obj_release(tu->pobj);
                return -1;
            }
            break;
        }
    }

    for( ; cont; fe && (s2obj_release(fe->pobj), 0) )
    {
        subret = s2list_shift_T(lalr_prod_t)(tu, &de);
        fe = de;
        if( subret == s2_access_nullval )
        {
            // end-of-list.
            cont = false;
            continue;
        }

        if( subret == s2_access_error )
        {
            CxingFatal("[ProcessTU]: Unable to shift entity declaration(s) "
                       "from the buffer list.");
            ret = cont = false;
            continue;
        }

        if( entRule(de) == funcdecl_subr ||
            entRule(de) == funcdecl_method )
        {
            // no linkage specifier.

            ss = de->terms[1].production->terms[0].terminal;
            subret = s2dict_get_T(lalr_prod_t)(module->entities, ss->str, &se);

            if( subret == s2_access_error )
            {
                CxingFatal(
                    "[ProcessTU]: Module object internal error 01!\n");
                ret = cont = false;
                continue;
            }

            else if( subret == s2_access_nullval )
            {
                // new declaration/definition.

                ++ *func_defs_cnt;
                subret = s2dict_set(module->entities, ss->str,
                                    de->pobj, s2_setter_kept);
                if( subret != s2_access_success )
                {
                    CxingFatal(
                        "[ProcessTU]: Unable to remember entity 01!\n");
                    ret = cont = false;
                    continue;
                }

                continue;
            }

            assert( subret == s2_access_success );

            if( !CheckFuncDefConflict(ss, de, se) )
            {
                // Found error while checking for definition conflict.
                ret = cont = false;
                continue;
            }

            if( entRule(de->terms[3].production) == stmt_brace )
            {
                // `de` is a definition,

                // Move the entity metadata over.
                assert( de != se );
                de->value = se->value;
                se->value = NULL;

                // remembering the definition.
                ++ *func_defs_cnt;
                subret = s2dict_set(module->entities, ss->str,
                                    de->pobj, s2_setter_kept);
                if( subret != s2_access_success )
                {
                    CxingFatal(
                        "[ProcessTU]: Unable to remember entity 02!\n");
                    ret = cont = false;
                    continue;
                }
            }

            continue;
        }

        else if( entRule(de) == entdecl_extern )
        {
            // `extern` linkage specifier.

            assert( entRule(de->terms[1].production) == funcdecl_subr ||
                    entRule(de->terms[1].production) == funcdecl_method );

            // decapsulate the marked entity.
            de = de->terms[1].production;

            ss = de->terms[1].production->terms[0].terminal;
            subret = s2dict_get_T(lalr_prod_t)(module->entities, ss->str, &se);

            if( subret == s2_access_error )
            {
                CxingFatal(
                    "[ProcessTU]: Module object internal error 02!\n");
                ret = cont = false;
                continue;
            }

            else if( subret == s2_access_nullval )
            {
                // new declaration/definition.

                de->value = s2gc_obj_alloc(
                    S2_OBJ_TYPE_SYM_INFO, sizeof(cxing_syminfo_t));
                if( !de->value )
                {
                    CxingFatal("[ProcessTU]: Unable to allocate metadata "
                               "for entity definition 01.\n");
                    ret = cont = false;
                    continue;
                }

                ++ *func_defs_cnt;
                subret = s2dict_set(module->entities, ss->str,
                                    de->pobj, s2_setter_kept);
                if( subret != s2_access_success )
                {
                    CxingFatal(
                        "[ProcessTU]: Unable to remember entity 03!\n");
                    ret = cont = false;
                    continue;
                }

                continue;
            }

            assert( subret == s2_access_success );

            if( !CheckFuncDefConflict(ss, de, se) )
            {
                // Found error while checking for definition conflict.
                ret = cont = false;
                continue;
            }

            if( entRule(de->terms[3].production) == stmt_brace )
            {
                // `de` is a definition,

                // Move the entity metadata over.
                de->value = se->value;
                se->value = NULL;

                if( !de->value )
                    de->value = s2gc_obj_alloc(
                        S2_OBJ_TYPE_SYM_INFO, sizeof(cxing_syminfo_t));
                if( !de->value )
                {
                    CxingFatal("[ProcessTU]: Unable to allocate metadata "
                               "for entity definition 02.\n");
                    ret = cont = false;
                    continue;
                }
                ((cxing_syminfo_t *)de->value)->flags |=
                module_syminfo_extern;

                // remembering the definition.
                ++ *func_defs_cnt;
                subret = s2dict_set(module->entities, ss->str,
                                    de->pobj, s2_setter_kept);
                if( subret != s2_access_success )
                {
                    CxingFatal(
                        "[ProcessTU]: Unable to remember entity 04!\n");
                    ret = cont = false;
                    continue;
                }
            }

            else
            {
                // A non-conflicting definition already exists.
                assert( entRule(se->terms[3].production) == stmt_brace );

                if( !se->value )
                    se->value = s2gc_obj_alloc(
                        S2_OBJ_TYPE_SYM_INFO, sizeof(cxing_syminfo_t));
                if( !se->value )
                {
                    CxingFatal("[ProcessTU]: Unable to allocate metadata "
                               "for entity definition 03.\n");
                    ret = cont = false;
                    continue;
                }
                ((cxing_syminfo_t *)se->value)->flags |=
                module_syminfo_extern;
            }

            continue;
        }

        else assert( 0 );

        // 2026-01-10 TODO: '_Include' and others.
    }

    s2obj_release(tu->pobj);
    return ret;
}

static bool CreateCallStubs(cxing_module_t *module, long func_defs_cnt)
{
    int i;
    size_t t;
    s2iter_t *ei = s2obj_iter_create(module->entities->pobj);

    module->CallStubs = cxing_callxfer_bridge.prepare_memory_for_stubs(
        func_defs_cnt, cxing_callxfer_bridge.sz_callstub);

    module->SymTab = calloc(
        func_defs_cnt, sizeof(s2data_t *));

    if( !ei || !module->CallStubs || !module->SymTab )
    {
        if( ei )
        {
#if INTERCEPT_MEM_CALLS
            // See notes below in `CxingModuleDump`.
            free(ei);
#else
            ei->final(ei);
#endif /* INTERCEPT_MEM_CALLS */
        }

        if( module->CallStubs )
        {
            cxing_callxfer_bridge.free_callstub_memory(
                module->CallStubs,
                module->func_defs_cnt,
                cxing_callxfer_bridge.sz_callstub);
        }

        if( module->SymTab )
            free(module->SymTab);

        module->CallStubs = NULL;
        module->SymTab = NULL;
        return false;
    }

    t = 0;
    for(i=ei->next(ei); i>0; i=ei->next(ei))
    {
        if( entRule(((lalr_prod_t *)ei->value)) != funcdecl_subr &&
            entRule(((lalr_prod_t *)ei->value)) != funcdecl_method )
            continue;

        memcpy(cxing_callxfer_bridge.sz_callstub * t + module->CallStubs,
               cxing_callxfer_bridge.callstub,
               cxing_callxfer_bridge.sz_callstub);

        module->SymTab[t] = (s2data_t *)s2obj_retain(ei->key);
        s2data_map(module->SymTab[t], 0, 0); // prevent it from being resized.

        cxing_callxfer_bridge.relocator(
            cxing_callxfer_bridge.sz_callstub * t + module->CallStubs,
            module);
        t ++;
    }

#if INTERCEPT_MEM_CALLS
    // See notes below in `CxingModuleDump`.
    free(ei);
#else
    ei->final(ei);
#endif /* INTERCEPT_MEM_CALLS */

    return cxing_callxfer_bridge.make_stub_memory_executable(
        module->CallStubs, func_defs_cnt, cxing_callxfer_bridge.sz_callstub);
}

cxing_module_t *CXOpen(const char *restrict CxingModulePath)
{
    int subret = 0;
    long func_defs_cnt = 0;

    cxing_module_t *ret = NULL;
    lex_getc_fp_t getcx;
    source_rope_t *rope = NULL;
    RegexLexContext lexer;

    lalr_stack_t *parsed = NULL;

    lex_getc_init_from_fp(&getcx, fopen(CxingModulePath, "r"));
    if( !getcx.fp ) goto cleanup;

    rope = CreateRopeFromGetc(&getcx.base, 0);
    if( !rope ) goto cleanup;

    lexer.regices = var_lex_elems;
    RegexLexFromRope_Init(&lexer, rope);

    subret = lalr_parse(
        &parsed, GRAMMAR_RULES, NULL, NS_RULES,
        (token_shifter_t)RegexLexFromRope_Shift, (void *)&lexer);

    if( subret != 0 )
    {
        CxingFatal("[CXOpen]: Parsing "
                   "of source code file %s "
                   "encountered error, "
                   "return value was: %d.\n",
                   CxingModulePath, subret);
        goto cleanup;
    }

    if( !(ret = cxing_module_create()) )
    {
        CxingFatal("[CXOpen]: Unable to create module "
                   "for translation unit from "
                   "source code file %s.\n", CxingModulePath);
        goto cleanup;
    }

    if( !ProcessTU(ret, parsed, &func_defs_cnt) )
    {
        CxingFatal("[CXOpen]: Processing of "
                   "source code file: %s failed.\n",
                   CxingModulePath);
        goto cleanup;
    }

    ret->func_defs_cnt = func_defs_cnt;

    if( !CreateCallStubs(ret, func_defs_cnt) )
    {
        CxingFatal("[CXOpen]: Failed to create function call stubs "
                   "for source code file %s.\n", CxingModulePath);
        goto cleanup;
    }

    s2obj_release(parsed->pobj);
    s2obj_release(rope->pobj);
    fclose(getcx.fp);
    return ret;

cleanup:
    lalr_parse_accel_cache_clear();

    if( parsed ) s2obj_release(parsed->pobj);

    CxingRuntimeFinal();
    CXParserFinal();

    if( rope ) s2obj_release(rope->pobj);
    if( getcx.fp ) fclose(getcx.fp);

    return ret;
}

void CxingModuleDump(cxing_module_t *restrict module)
{
    int i;
    s2iter_t *ei;

    ei = s2obj_iter_create(module->entities->pobj);
    assert( ei );

    for(i=ei->next(ei); i>0; i=ei->next(ei))
    {
        lalr_prod_t *ent = (lalr_prod_t *)ei->value;
        s2data_t *ss = ei->key;

        printf("ent: %p, ss: %p.\n", ent, ss);
        printf("Defintion for '%s' ",
               (const char *)s2data_weakmap(ss));

        if( ent->value )
        {
            printf("(flags: %d)\n",
                   ((cxing_syminfo_t *)ent->value)->flags);
        }
        else printf("(flags: -)\n");

        print_prod(ent, 0, NS_RULES);
    }

#if INTERCEPT_MEM_CALLS
    // s2dict assigned the non-macro implementation of `free`
    // to the iterator finalizer, which didn't intercept
    // memory allocations.
    free(ei);
#else
    ei->final(ei);
#endif /* INTERCEPT_MEM_CALLS */

    ei = s2obj_iter_create(module->linked->pobj);
    assert( ei );

    for(i=ei->next(ei); i>0; i=ei->next(ei))
    {
        s2cxing_value_t *ext = (s2cxing_value_t *)ei->value;
        s2data_t *ss = ei->key;

        printf("ext: %p, ss: %p.\n", ext, ss);
        printf("Defintion for '%s' %d\n",
               (const char *)s2data_weakmap(ss),
               (int)ext->cxing_value.type->typeid);
    }

    #if INTERCEPT_MEM_CALLS
    // s2dict assigned the non-macro implementation of `free`
    // to the iterator finalizer, which didn't intercept
    // memory allocations.
    free(ei);
    #else
    ei->final(ei);
    #endif /* INTERCEPT_MEM_CALLS */

}

void *CXSym(cxing_module_t *restrict module, const char *restrict sym)
{
    size_t t;

    for(t=0; t<module->func_defs_cnt; t++)
    {
        if( strcmp(s2data_weakmap(module->SymTab[t]), sym) == 0 )
            break;
    }

    if( t >= module->func_defs_cnt )
    {
        CxingDebug("Symbol Definition for %s Not Found.\n", sym);
        return NULL;
    }

    return module->CallStubs +
        cxing_callxfer_bridge.sz_callstub * t +
        cxing_callxfer_bridge.sz_relocdat;
}

// 2026-01-17 TODO: resolve identifiers in the module for linked defs.

bool CXExpose(
    cxing_module_t *restrict module,
    const char *restrict name,
    struct value_nativeobj linkee)
{
    int subret;
    s2data_t *k;
    s2cxing_value_t *v;

    k = s2data_from_str(name);
    if( !k )
    {
        CxingDiagnose("[CXExpose]: Unable to allocate runtime data "
                      "for identifier key: %s\n", name);
        return false;
    }

    // 2026-01-17 (TODO: come back later):
    // This copy is per spec section "Automatic Resource Management".
    v = s2cxing_value_create(ValueCopy(linkee));
    if( !v )
    {
        s2obj_release(k->pobj);
        CxingDiagnose("[CXExpose]: Unable to create "
                      "runtime module binding for exposed value "
                      "whose name was: %s\n", name);
        return false;
    }

    subret = s2dict_set(module->linked, k, v->pobj, s2_setter_gave);
    s2obj_release(k->pobj);

    if( subret != s2_access_success )
    {
        s2obj_release(v->pobj);
        CxingDiagnose("[CXExpose]: Unable to remember "
                      "definition exposed value "
                      "whose name was: %s\n", name);
        return false;
    }

    return true;
}
