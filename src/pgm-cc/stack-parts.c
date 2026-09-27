/* DannyNiu/NJF, 2026-08-12. Public Domain. */

#include "stack-parts.h"

s2dict_t *ScopedDecls_Push1Scope(s2list_t *sd)
{
    s2dict_t *head = s2dict_create();
    assert( head );

    s2list_seek(sd, 0, S2_LIST_SEEK_SET);
    s2list_insert(sd, head->pobj, s2_setter_gave);
    return head;
}

void ScopedDecls_Pop1Scope(s2list_t *sd)
{
    s2obj_t *head;

    s2list_seek(sd, 0, S2_LIST_SEEK_SET);
    s2list_shift(sd, &head);

    assert( head );
    s2obj_release(head);
}

int ScopedDecls_Lookup(s2list_t *sd, s2data_t *ident, dcc_decl_info_t **out)
{
    s2dict_t *tab;
    s2ref_t *ent;
    int subret;

    *out = NULL;
    s2list_seek(sd, 0, S2_LIST_SEEK_SET);

    while( true )
    {
        s2list_get_T(s2dict_t)(sd, &tab);
        subret = s2dict_get_T(s2ref_t)(tab, ident, &ent);

        if( subret == s2_access_success )
        {
            *out = s2ref_unwrap(ent);
            return subret;
        }
        else if( subret == s2_access_error )
        {
            return subret;
        }

        if( s2list_pos(sd) + 1 == s2list_len(sd) )
        {
            return s2_access_nullval;
        }
        s2list_seek(sd, 1, S2_LIST_SEEK_CUR);
    }
}

#define theRule(p) (c_grammar_rules[(p)->semantic_rule])

lalr_prod_t *find_direct_declaring_identifier(lalr_prod_t *dx)
{
    while( true )
    {
        lalr_rule_t rule = theRule(dx);

        if( rule == initdecl_noinit ||
            rule == initdecl_init )
        {
            dx = dx->terms[0].production;
            continue;
        }

        else if( rule == declarator_pointer )
        {
            dx = dx->terms[1].production;
            continue;
        }

        else if( rule == direct_declarator_ident )
        {
            return dx;
        }

        else if( rule == direct_declarator_paren )
        {
            dx = dx->terms[1].production;
            continue;
        }

        else if( rule == direct_declarator_array ||
                 rule == direct_declarator_func )
        {
            dx = dx->terms[0].production;
            continue;
        }

        else if( rule == array_declarator_classic ||
                 rule == array_declarator_static ||
                 rule == array_declarator_qual ||
                 rule == array_declarator_asterisk ||
                 rule == function_declarator_funcdecl )
        {
            dx = dx->terms[0].production;
            continue;
        }

        else
        {
            // 2026-08-12: Shouldn't happen unless inconsistent with grammar.
            fprintf(stderr, "[%s:%s:%d]: rule=%d, sem_rule=%d.\n",
                    __FILE__, __func__, __LINE__,
                    dx->rule, dx->semantic_rule);
            assert( 0 );
            abort();
        }
    }
}

bool find_in_specifiers(lalr_prod_t *declspecs, const char *spec_keyword)
{
    while( true )
    {
        lalr_rule_t rule = theRule(declspecs);

        if( rule == declspecs_base )
        {
            lex_token_t *subj = declspecs->
                terms[0].production[0].
                terms[0].terminal;

            if( !s2_is_token(subj) )
            {
                // not a keyword.
                return false;
            }

            if( strcmp(s2data_weakmap(subj->str), spec_keyword) == 0 )
                return true;
        }

        else if( rule == declspecs_genrule )
        {
            declspecs = declspecs->terms[1].production;
            continue;
        }
    }
}

int ScopedDecls_Process1Decl(s2list_t *sd, lalr_prod_t *declaration, cpptu_t *ctx_tu)
{
    s2dict_t *tab;
    s2ref_t *ent;

    lalr_prod_t *declarators;
    int skipattrs = 0;
    int subret;

    // This needs to be ensured by the caller.
    assert( theRule(declaration) == decl_decl ||
            theRule(declaration) == decl_with_attr );

    if( theRule(declaration) == decl_with_attr )
        skipattrs = 1;

    s2list_seek(sd, 0, S2_LIST_SEEK_SET);
    s2list_get_T(s2dict_t)(sd, &tab);

    assert( tab );

    declarators = declaration->terms[skipattrs + 1].production;
    while( declarators )
    {
        dcc_decl_info_t *di = calloc(1, sizeof *di);

        assert( di );
        di->declspecs = declaration->terms[skipattrs + 0].production;

        if( theRule(declarators) == initdecls_genrule )
        {
            // Unowned unretained.
            di->declarator = find_direct_declaring_identifier(
                declarators->terms[2].production);

            // also unowned unretained.
            di->ident = di->declarator->
                terms[0].production->
                terms[0].terminal;

            di->is_volatile = find_in_specifiers(
                declaration->terms[skipattrs + 0].production, "volatile");

            di->is_external_decl = s2list_len(sd) == 1;

            subret = s2dict_get_T(s2ref_t)(tab, di->ident->str, &ent);
            assert( subret != s2_access_error );
            if( subret == s2_access_success )
            {
                if( s2list_len(sd) != 1 && ctx_tu ) // i.e. not external declaration.
                {
                    ccDiagnoseError(ctx_tu, "Identifier redeclared", spelling_and_site(di->declarator->terms[0].production->terms[0].terminal));
                }

                // 2026-08-12:
                // could probably tolerate one or two leaks for a one-shot program.
                free(di);
                declarators = declarators->terms[0].production;
                continue; // try to process the remaining declarators.
            }

            ent = s2ref_create(di, free);
            assert( ent );
            subret = s2dict_set(tab, di->ident->str, ent->pobj, s2_setter_gave);
            assert( subret == s2_access_success );

            declarators = declarators->terms[0].production;
            continue;
        }

        else
        {
            // Unowned unretained.
            di->declarator = find_direct_declaring_identifier(declarators);

            // also unowned unretained.
            di->ident = di->declarator->
                terms[0].production->
                terms[0].terminal;

            di->is_volatile = find_in_specifiers(
                declaration->terms[skipattrs + 0].production, "volatile");

            di->is_external_decl = s2list_len(sd) == 1;

            ent = s2ref_create(di, free);
            assert( ent );
            return s2dict_set(tab, di->ident->str, ent->pobj, s2_setter_gave);
        }
    }

    assert( 0 );
}
