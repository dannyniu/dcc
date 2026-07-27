/* DannyNiu/NJF, 2025-01-01. Public Domain. */

#include "lalr.h"
#include "../infra/s2bools.h"

#if DCC_LALR_LOGGING == 1

#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#define Reached(...) (eprintf("% 5d ", __LINE__), eprintf(__VA_ARGS__))

static void symbol_print_expect_chain(lalr_rule_symbol_t *chain)
{
    while( chain )
    {
        if( chain->type == lalr_symtype_vtoken )
        {
            eprintf("vtoken(%ld), ", (long)chain->vtype);
        }
        else if( chain->type == lalr_symtype_stoken )
        {
            eprintf("\"%s\", ", chain->value);
        }
        else if( chain->type == lalr_symtype_prod )
        {
            eprintf("%s, ", chain->value);
        }
        else eprintf("!%d!, ", chain->type);

        chain = chain->next;
    }
    eprintf("\n");
}

static void dump_parsing_stack(
    const char *h,
    lalr_term_t const *bt,
    lalr_term_t const *te,
    strvec_t *ns_rules,
    const char *t)
{
#define STRBUF_SIZE 58
    char strbuf[STRBUF_SIZE];
    char *strp;
    eprintf("%s", h);
    for(; bt; bt = bt->up)
    {
        memset(strbuf, 0, sizeof(strbuf));
        memset(strbuf, ' ', 2);
        strp = strbuf + 2;
        if( bt == te ) strbuf[0] = '^';
        if( bt->anchored ) strbuf[1] = '!';
        if( s2_is_prod(bt->production->pobj) )
            snprintf(strp, STRBUF_SIZE-2, "%s", strvec_i2str(
                         ns_rules, bt->production->production));
        else snprintf(strp, STRBUF_SIZE-2, "\"%s\"",
                      (char *)s2data_weakmap(bt->terminal->str));
        eprintf("%-28s: ", strbuf);
        symbol_print_expect_chain(bt->expecting);
    }
    eprintf("\n");
    eprintf("%s", t);
}

#else /* Not Logging. */

#define eprintf(...)
#define Reached(...)
#define symbol_print_expect_chain(...)
#define dump_parsing_stack(...)

#endif /* DCC_LALR_LOGGING */

void fprint_token(FILE *fp, lex_token_t *tn, int indentlevel)
{
    (void)indentlevel;
    fprintf(fp, "%s\n", (char *)s2data_weakmap(tn->str));
}

void fprint_prod(FILE *fp, lalr_prod_t *prod, int indentlevel, strvec_t *ns)
{
    size_t t;
    fprintf(fp, "%d:%s<%d:%s>\n",
            prod->rule,
            strvec_i2str(ns, prod->production),
            prod->semantic_rule,
            strvec_i2str(ns, prod->semantic_production));

    for(t=0; t<prod->terms_count; t++)
    {
        if( !prod->terms[t].production ) // `|| !prod->terms[t].terminal`.
        {
            fprintf(fp, "%*s ./. \n", indentlevel * 2 + 3, "");
        }
        else if( s2_is_prod(prod->terms[t].production) )
        {
            fprintf(fp, "%*s [%zi] ", indentlevel * 2 + 3, "", t);
            fprint_prod(fp, prod->terms[t].production, indentlevel + 1, ns);
        }
        else
        {
            lex_token_t *tn = prod->terms[t].terminal;
            assert( s2_is_token(tn) );
            fprintf(fp, "%*s _%zi_ ", indentlevel * 2 + 3, "", t);
            fprint_token(fp, tn, indentlevel);
        }
    }
}

static void lalr_prod_final(lalr_prod_t *ctx)
{
    size_t t;

    if( ctx->value ) s2obj_release(ctx->value);

    for(t=0; t<ctx->terms_count; t++)
    {

        if( ctx->terms[t].terminal ) // similarly `... production->pobj`.
            s2obj_release(ctx->terms[t].terminal->pobj);
    }

    free(ctx->terms);
}

lalr_prod_t *lalr_prod_create(size_t init_terms_cnt)
{
    lalr_prod_t *ret = NULL;
    void *terms = NULL;

    if( !(terms = calloc(init_terms_cnt, sizeof(void *))) )
        return NULL;

    ret = (lalr_prod_t *)s2gc_obj_alloc(
        S2_OBJ_TYPE_PRODUCTION, sizeof(lalr_prod_t));

    if( !ret )
    {
        free(terms);
        return NULL;
    }

    ret->base.itercreatf = NULL;
    ret->base.finalf = (s2func_final_t)lalr_prod_final;

    ret->production = 0;
    ret->rule = 0;
    ret->value = NULL;

    ret->terms_count = init_terms_cnt;
    ret->terms = terms;
    return ret;
}

void lalr_term_free(lalr_term_t *term)
{
    lalr_rule_symbol_free(term->expecting);

    // 2025-06-01:
    // refer to `lalr_rule_reduce` function definition
    // for why production/terminal is not freed here.
    free(term);
}

static bool lalr_symbol_matches_term(
    lalr_rule_symbol_t const *symbol,
    lalr_term_t *term,
    strvec_t *ns_rules)
{
    // 2025-01-27:
    // predicate: `term` matches `symbol`.
    // gives no regard to `optional` (which is handled elsewhere).

    // Assertion added 2026-07-25:
    // This function handles only non-NULL operands.
    assert( term );

    if( symbol->type == lalr_symtype_stoken )
    {
        if( !s2_is_token(term->terminal) )
            return false;

        if( 0 != strcmp(
                symbol->value,
                s2data_weakmap(term->terminal->str)) )
            return false;

        // 2026-06-03:
        // A fault was discovered, that the string literal `"true"` was
        // mistakenly recognized as such 'stoken', causing an assertion
        // in the parsing code to fail.
        // The token is now augmented with a field that indicates whether
        // or not it had underwent dequoting, as the CXING string literal
        // concatenator would have done so while lexing.
        if( term->terminal->identity != TOKIDENT_PRISTINE )
            return false;

        return true;
    }
    else if( symbol->type == lalr_symtype_vtoken )
    {
        if( !s2_is_token(term->terminal) )
            return false;

        return symbol->vtype == term->terminal->completion;
    }
    else if( symbol->type == lalr_symtype_prod )
    {
        const char *strptr = NULL;

        if( !s2_is_prod(term->production) )
            return false;

        strptr = strvec_i2str(ns_rules, term->production->production);
        assert( strptr );

        return strcmp(strptr, symbol->value) == 0;
    }
    else assert( 0 );
}

static lalr_rule_symbol_t const *(lalr_rule_match)(
    lalr_rule_symbol_t const *symbolseq,
    lalr_term_t *anchterm,
    strvec_t *ns_rules)
{
    lalr_term_t *term = anchterm;

    if( symbolseq[0].type == lalr_symtype_symset )
    {
        // This block is added 2026-07-17, for handling `symset` - symbol sets.

        int i = 1;

        while( true )
        {
            if( !symbolseq[i].type ) // equals 0, i.e. `lalr_symtype_invalid`.
                break;

            if( lalr_symbol_matches_term(symbolseq, term, ns_rules) )
                break;

            i ++;
            // 2026-07-17: Not iterating over terms - top-of-stack is assumed.
        }

        if( !symbolseq[i].type == !symbolseq->vtype )
            // 2026-07-17:
            // 1. term matches 1 symbol and it's an accepting set.
            // 2. term matches no symbol and it's an exclusion set.
            return symbolseq;
    }

    while( true )
    {
        if( !symbolseq->type && term )
            return NULL;

        if( !term )
        {
            return symbolseq;
        }

        if( lalr_symbol_matches_term(symbolseq, term, ns_rules) )
        {
            term = term->up;
            symbolseq++;
            continue;
        }
        else if( symbolseq->optional )
        {
            symbolseq++;
            continue;
        }
        else return NULL;
    }
}

static lalr_prod_t *(lalr_rule_reduce)(
    lalr_rule_symbol_t const *symbolseq,
    int32_t production,
    int32_t ri,
    lalr_term_t *anchterm,
    strvec_t *ns_rules)
{
    // less checks are made, lenient on undefined behaviors.

    int32_t terms_count = 0, i;
    lalr_term_t *terms = anchterm, *temp;
    lalr_prod_t *newprod = NULL;

    for(terms_count = 0; symbolseq[terms_count].type; terms_count++) ((void)0);

    eprintf("prod: %s; ", strvec_i2str(ns_rules, production));

    if( symbolseq[0].type == lalr_symtype_prod &&
        s2_is_prod(terms[0].production) &&
        !symbolseq[0].optional && terms_count == 1 )
    {
        // 2025-12-17: optimization for the degenerate case.
        anchterm->production->production = production;
        anchterm->production->rule = ri;
        return anchterm->production;
    }

    if( symbolseq[0].type == lalr_symtype_symset )
    {
        // This block is added 2026-07-17, for handling `symset` - symbol sets.
        assert( !newprod ); // prevents my future self from making mistakes.
        newprod = lalr_prod_create(1);
        if( !newprod ) return NULL;

        if( s2_is_prod(terms->production) )
        {
            eprintf("%s, ", strvec_i2str(
                        ns_rules, terms->production->production));

            terms->production->parent = newprod;
        }
        else
        {
            eprintf("\"%s\", ", (char *)s2data_weakmap(
                        terms->terminal->str));
        }

        // could also be `... terms->terminal`.
        newprod->terms[0].production = terms->production;

        // We're done creating and initializing `newprod`,
        // set this to 0 to skip the normal path.
        terms_count = 0;
    }

    if( terms_count > 0 )
    {
        newprod = lalr_prod_create(terms_count);
        if( !newprod ) return NULL;
    }

    for(i=0; i<terms_count; i++)
    {
        // commented-out on 2026-07-25 for its
        // inability to handle optional terms.
        //- assert( terms );

        if( terms && lalr_symbol_matches_term(symbolseq+i, terms, ns_rules) )
        {
            if( s2_is_prod(terms->production) )
            {
                eprintf("%s, ", strvec_i2str(
                            ns_rules, terms->production->production));

                terms->production->parent = newprod;
            }
            else
            {
                eprintf("\"%s\", ", (char *)s2data_weakmap(
                            terms->terminal->str));
            }

            // could also be `... terms->terminal`.
            newprod->terms[i].production = terms->production;

            // 2025-06-01:
            // Because we're assigning the production/terminal
            // to the newly created production as part of the
            // reduction operation, when we free the term using
            // `lalr_term_free`, we must retain the
            // production/terminal originally contained therein.

            if( terms != anchterm )
            {
                temp = terms->up;
                lalr_term_free(terms);
                terms = temp;
            }
            else terms = terms->up;
        }
        else
        {
            eprintf("REDUC.te: %p.\n", terms);
            assert( symbolseq[i].optional );

            // could also be `... terms->terminal`.
            newprod->terms[i].production = NULL;
        }
    }

    anchterm->up = terms;
    anchterm->production = newprod;

    newprod->semantic_production = newprod->production = production;
    newprod->semantic_rule = newprod->rule = ri;
    // newprod->rule // Assigned from a calling routine.

    return newprod;
}

struct traversed_rules {
    lalr_rule_t r;
    struct traversed_rules *up;
};

static bool find_rule_in_traversed(
    lalr_rule_t rule, struct traversed_rules *tup)
{
    while( tup )
    {
        if( rule == tup->r ) return true;
        else tup = tup->up;
    }
    return false;
}

s2dict_t *lalr_parse_accel_cache = NULL;

void lalr_parse_accel_cache_clear()
{
    if( lalr_parse_accel_cache ) s2obj_release(lalr_parse_accel_cache->pobj);
    lalr_parse_accel_cache = NULL;
}

// returns one of the `s2_access_retvals` enumeration.
static int lalr_parse_accel_cache_insert(
    lalr_rule_symbol_t const *restrict symbolseq,
    lalr_rule_symbol_t const *restrict expected_sym,
    int query_result)
{
    // All errors in this function may be safely ignored by
    // the caller, which then resort to a full search.
    s2data_t *cache_key = NULL;
    int ret;

    if( !lalr_parse_accel_cache )
        lalr_parse_accel_cache = s2dict_create();

    if( !lalr_parse_accel_cache )
    {
        lalr_parse_accel_cache_clear();
        return s2_access_error;
    }

    assert( expected_sym->type == lalr_symtype_prod );//return s2_access_error;

    if( !(cache_key = s2data_create(0)) )
        return s2_access_error;

    // First element of the key tuple is the pointer to
    // the static-qualified function-scope symbol sequence.
    s2data_puts(cache_key, (const void *)&symbolseq, sizeof(const void *));

    // Second element of the key tuple is the string
    // representing the (human-readable) production
    // of the expected symbol.
    s2data_puts(cache_key, expected_sym->value, strlen(expected_sym->value));

    ret = s2dict_set(
        lalr_parse_accel_cache, cache_key,
        query_result ? s2_true : s2_false,
        s2_setter_kept);
    s2obj_release(cache_key->pobj);
    return ret;
}

// Returns one of true, false, and -1.
static int lalr_parse_accel_cache_query(
    lalr_rule_symbol_t const *restrict symbolseq,
    lalr_rule_symbol_t const *restrict expected_sym)
{
    s2data_t *query_result;
    s2data_t *cache_key;
    int ret = -1;

    if( !lalr_parse_accel_cache ) return -1;

    assert( expected_sym->type == lalr_symtype_prod );

    if( !(cache_key = s2data_create(0)) ) return -1;
    s2data_puts(cache_key, (const void *)&symbolseq, sizeof(const void *));
    s2data_puts(cache_key, expected_sym->value, strlen(expected_sym->value));

    ret = s2dict_get_T(s2data_t)(
        lalr_parse_accel_cache, cache_key, &query_result);
    s2obj_release(cache_key->pobj);

    if( ret != s2_access_success )
    {
        return -1;
    }
    else if( *(char *)s2data_weakmap(query_result) )
    {
        return true;
    }
    else return false;
}

static bool begins_with_expected(
    lalr_rule_symbol_t const *restrict symbolseq, // from the compiled grammar.
    lalr_rule_symbol_t const *restrict expected_sym,
    struct traversed_rules *tup, // prevents infinite loop.
    lalr_rule_t grammar_rules[restrict],
    strvec_t *restrict ns_rules);

static bool begins_with_expected(
    lalr_rule_symbol_t const *restrict symbolseq, // from the compiled grammar.
    lalr_rule_symbol_t const *restrict expected_sym,
    struct traversed_rules *tup, // prevents infinite loop.
    lalr_rule_t grammar_rules[restrict],
    strvec_t *restrict ns_rules)
{
    lalr_rule_symbol_t const *rchain; // r = rule/reduction.
    lalr_rule_t *subsrule = grammar_rules; // subs = substitution,
    int ssi = 0; // symbolseq index;

    // 2026-07-25 TODO: Not able to handle `lalr_symtype_symset` yet.
    //- eprintf("%s.symbolseq: ", __func__); symbol_print_expect_chain(symbolseq);
#define BEW_TRACE (void)0 //eprintf("bew.Reached-%d: %d %d %d. %td %p,\n", __LINE__, ssi, !!tup, !symbolseq[ssi].optional, subsrule - grammar_rules, tup)
    for(ssi=0; ; ssi++)
    {
        if( symbolseq[ssi].type == lalr_symtype_stoken ||
            symbolseq[ssi].type == lalr_symtype_vtoken )
        {
            // If the current state expect a terminal (in the symbol sequence),
            // and the term is one, then return true.

            if( expected_sym->type == lalr_symtype_prod )
            {
                BEW_TRACE;
                if( !tup || // 2026-07-25: `symbolseq` is actually an `expect_chain`.
                    !symbolseq[ssi].optional )
                    // 2025-01-20:
                    // Because expectation is a non-terminal, and the beginning symbol(s)
                    // in the current rule isn't one, donot apply the current rule.
                    return false;
                else continue;
            }

            if( expected_sym->type != symbolseq[ssi].type )
            {
                BEW_TRACE;
                if( !tup || // 2026-07-25: `symbolseq` is actually an `expect_chain`.
                    !symbolseq[ssi].optional )
                    return false;
                else continue;
            }

            BEW_TRACE;
            if( expected_sym->type == lalr_symtype_stoken )
                if( strcmp(expected_sym->value, symbolseq[ssi].value) == 0 )
                    return true;

            BEW_TRACE;
            if( expected_sym->type == lalr_symtype_vtoken )
                if( expected_sym->vtype == symbolseq[ssi].vtype )
                    return true;
            BEW_TRACE;
        }

        if( expected_sym->type != symbolseq[ssi].type )
        {
            BEW_TRACE;
            if( !tup || // 2026-07-25: `symbolseq` is actually an `expect_chain`.
                !symbolseq[ssi].optional )
                return false;
            else continue;
        }

        if( strcmp(expected_sym->value, symbolseq[ssi].value) == 0 )
        {
            BEW_TRACE;
            lalr_parse_accel_cache_insert(symbolseq, expected_sym, true);
            return true;
        }

        for(subsrule = grammar_rules; *subsrule; subsrule++)
        {
            struct traversed_rules trav = { .r = *subsrule, .up = tup };
            int32_t lhs;
            int query_result;
            const char *strptr;

            rchain = (*subsrule)(lalr_rule_inspect_symseq,
                                 NULL, -1, NULL,
                                 grammar_rules, ns_rules);

            query_result = lalr_parse_accel_cache_query(
                rchain, expected_sym);
            if( query_result == false ) continue;

            lhs = (int32_t)(ptrdiff_t)(*subsrule)(
                lalr_rule_inspect_lhs,
                NULL, -1, NULL,
                grammar_rules, ns_rules);
            strptr = strvec_i2str(ns_rules, lhs);

            if( strcmp(symbolseq[ssi].value, strptr) != 0 )
                // the lhs of subsrule doesn't match the 1st symbol of symbolseq.
                continue;

            if( rchain->type == lalr_symtype_prod )
                if( strcmp(rchain[0].value, strptr) == 0 )
                    // The rule's 1st symbol equals its left-hand-side,
                    // avoid its infinite loop.
                    continue;

            if( find_rule_in_traversed(*subsrule, tup) )
                // catches loop-in-alternation rule pairs and groups.
                continue;

            if( query_result == true || begins_with_expected(
                    rchain, expected_sym, &trav, grammar_rules, ns_rules) )
            {
                BEW_TRACE;
                lalr_parse_accel_cache_insert(
                    symbolseq, expected_sym, true);
                return true;
            }
        }

        if( !tup || // 2026-07-25: `symbolseq` is actually an `expect_chain`.
            !symbolseq[ssi].optional )
        {
            BEW_TRACE;
            lalr_parse_accel_cache_insert(symbolseq, expected_sym, false);
            return false;
        }
        else continue;
    }
}

static bool (lalr_rule_expect)(
    int32_t production,
    lalr_term_t const *term_expectation,
    lalr_rule_t grammar_rules[],
    strvec_t *ns_rules)
{
    lalr_rule_symbol_t *expect_chain;
    lalr_rule_symbol_t expect_symbol = {};
    const char *lhs = strvec_i2str(ns_rules, production);

    //- eprintf("lrx: "); symbol_print_expect_chain(term_expectation->expecting);

    expect_symbol.type = lalr_symtype_prod;
    expect_symbol.value = lhs;

    for(expect_chain = term_expectation->expecting;
        expect_chain; expect_chain = expect_chain->next)
    {
        if( begins_with_expected(
                expect_chain, &expect_symbol,
                NULL, grammar_rules, ns_rules) )
            break;
    }

    if( !expect_chain )
        return false;

    return true;
}

void *lalr_rule_actions_generic(
    lalr_rule_symbol_t *restrict symbolseq,
    int32_t production,
    int32_t ri,
    lalr_rule_action_t action,
    lalr_term_t *restrict terms,
    // `ctx` is used by rules themselves, to e.g. build semantics.
    lalr_rule_t rules[restrict],
    strvec_t *restrict ns_rules)
{
    switch( action )
    {
    case lalr_rule_action_match:
        return (void *)(lalr_rule_match)(
            symbolseq, terms, ns_rules);

    case lalr_rule_action_reduce:
        return (lalr_rule_reduce)(
            symbolseq, production, ri, terms, ns_rules);

    case lalr_rule_action_expect:
        return (void *)(intptr_t)(lalr_rule_expect)(
            production, terms, rules, ns_rules);

    case lalr_rule_inspect_lhs:
        return (void *)(ptrdiff_t)production;

    case lalr_rule_inspect_symseq:
        return symbolseq;

    default:
        return NULL;
    }
}

#define lalr_rule_match(rules, ri, terms, ctx, strtab)                  \
    (((lalr_rule_symbol_t const *(*)(lalr_rule_params))rules[ri])       \
     (lalr_rule_action_match, terms, ri, ctx, rules, strtab))

#define lalr_rule_reduce(rules, ri, terms, ctx, strtab)         \
    (((lalr_prod_t *(*)(lalr_rule_params))rules[ri])            \
     (lalr_rule_action_reduce, terms, ri, ctx, rules, strtab))

#define lalr_rule_expect(rules, ri, terms, ctx, strtab)         \
    ((bool)((void *(*)(lalr_rule_params))rules[ri])             \
     (lalr_rule_action_expect, terms, ri, ctx, rules, strtab))

static void lalr_stack_final(lalr_stack_t *ctx)
{
    lalr_term_t *t = ctx->bottom, *s;

    while( t )
    {
        s = t->up;

        // similarly `... production->pobj`.
        s2obj_release(t->terminal->pobj);
        lalr_term_free(t);

        t = s;
    }
}

lalr_stack_t *lalr_stack_create()
{
    lalr_stack_t *ret = NULL;

    ret = (lalr_stack_t *)s2gc_obj_alloc(
        S2_OBJ_TYPE_STACK, sizeof(lalr_stack_t));

    if( !ret ) return NULL;

    ret->base.itercreatf = NULL;
    ret->base.finalf = (s2func_final_t)lalr_stack_final;

    return ret;
}

static lalr_term_t *terms_drop_1anch(lalr_term_t *te)
{
    while( !te->anchored )
    {
        if( !te->dn ) break;
        te = te->dn;
    }
    te->anchored = false;

    if( !te->dn )
        return NULL;

    while( !te->anchored )
    {
        if( !te->dn ) break;
        te = te->dn;
    }

    return te;
}

void lalr_rule_symbol_free(lalr_rule_symbol_t *chain)
{
    lalr_rule_symbol_t *next;

    while( chain )
    {
        next = chain->next;
        free(chain);
        chain = next;
    }
}

#define Candidates_Set(ri)                      \
    ( candidates_bitmap ?                       \
      candidates_bitmap[ri / 32] |=             \
      ((uint32_t)1 << (ri % 32)) : 1 )

#define Candidate_Test(ri)                      \
    ( candidates_bitmap ?                       \
      candidates_bitmap[ri / 32] &              \
      ((uint32_t)1 << (ri % 32)) : 1 )

#define Candidate_Drop(ri)                      \
    ( candidates_bitmap ?                       \
      candidates_bitmap[ri / 32] ^=             \
      ((uint32_t)1 << (ri % 32)) : 0 )

#define Candidates_Clear()                              \
    if( candidates_bitmap ){                            \
        uint32_t ci;                                    \
        for(ci=0; ci<ruleset_cardinality; ci+=32)       \
            candidates_bitmap[ci / 32] = 0;             \
    }
