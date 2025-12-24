/* DannyNiu/NJF, 2025-01-01. Public Domain. */

#include "lalr.h"

#if DCC_LALR_LOGGING == 1

#error Logging?
#define eprintf(...) fprintf(stderr, __VA_ARGS__)

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
#define symbol_print_expect_chain(...)
#define dump_parsing_stack(...)

#endif /* DCC_LALR_LOGGING */

static void lalr_prod_final(lalr_prod_t *ctx)
{
    int32_t t;

    if( ctx->value ) s2obj_release(ctx->value);

    for(t=0; t<ctx->terms_count; t++)
    {
        // similarly `... production->pobj`.
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

    if( symbol->type == lalr_symtype_stoken )
    {
        if( !s2_is_token(term->terminal) )
            return false;

        return strcmp(
            symbol->value,
            s2data_weakmap(term->terminal->str)) == 0;
    }
    else if( symbol->type == lalr_symtype_vtoken )
    {
        if( !s2_is_token(term->terminal) )
            return false;

        return symbol->vtype == term->terminal->completion;
    }
    else
    {
        const char *strptr = NULL;

        assert( symbol->type == lalr_symtype_prod );

        if( !s2_is_prod(term->production) )
            return false;

        strptr = strvec_i2str(ns_rules, term->production->production);
        assert( strptr );

        return strcmp(strptr, symbol->value) == 0;
    }
}

static lalr_rule_symbol_t const *(lalr_rule_match)(
    lalr_rule_symbol_t const *symbolseq,
    lalr_term_t *anchterm,
    strvec_t *ns_rules)
{
    lalr_term_t *term = anchterm;

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

    newprod = lalr_prod_create(terms_count);
    if( !newprod ) return NULL;

    for(i=0; i<terms_count; i++)
    {
        assert( terms );

        if( lalr_symbol_matches_term(symbolseq+i, terms, ns_rules) )
        {
            if( s2_is_prod(terms->production) )
            {
                eprintf("%s, ", strvec_i2str(
                            ns_rules, terms->production->production));
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
static s2data_t *s2_true, *s2_false;

void lalr_parse_accel_cache_clear()
{
    if( lalr_parse_accel_cache ) s2obj_release(lalr_parse_accel_cache->pobj);
    if( s2_true ) s2obj_release(s2_true->pobj);
    if( s2_false ) s2obj_release(s2_false->pobj);
    lalr_parse_accel_cache = NULL;
    s2_true = s2_false = NULL;
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

    if( !s2_true ) s2_true = s2data_from_str("\x01");
    if( !s2_false ) s2_false = s2data_from_str("\x00");

    if( !lalr_parse_accel_cache || !s2_true || !s2_false )
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
        query_result ? s2_true->pobj : s2_false->pobj,
        s2_setter_kept);
    s2obj_release(cache_key->pobj);
    return ret;
}

// Returns one of true, false, and -1.
static int lalr_parse_accel_cache_query(
    lalr_rule_symbol_t const *restrict symbolseq,
    lalr_rule_symbol_t const *restrict expected_sym)
{//return -1;
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
    strvec_t *restrict ns_rules)
{
    lalr_rule_symbol_t const *rchain; // r = rule/reduction.
    lalr_rule_t *subsrule; // subs = substitution,

    if( symbolseq[0].type == lalr_symtype_stoken ||
        symbolseq[0].type == lalr_symtype_vtoken )
    {
        // If the current state expect a terminal (in the symbol sequence),
        // and the term is one, then return true.

        if( expected_sym->type == lalr_symtype_prod )
        {
            // 2025-01-20:
            // Because expectation is a non-terminal, and the 1st symbol
            // in the current rule isn't one, donot apply the current rule.
            return false;
        }

        if( expected_sym->type != symbolseq[0].type )
            return false;

        if( expected_sym->type == lalr_symtype_stoken )
            if( strcmp(expected_sym->value, symbolseq[0].value) == 0 )
                return true;

        if( expected_sym->type == lalr_symtype_vtoken )
            if( expected_sym->vtype == symbolseq[0].vtype )
                return true;
    }

    if( expected_sym->type != symbolseq[0].type )
        return false;

    if( strcmp(expected_sym->value, symbolseq[0].value) == 0 )
    {
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
        //if( tmp == true ) return tmp; else
        if( query_result == false ) continue;

        lhs = (int32_t)(long)(*subsrule)(
            lalr_rule_inspect_lhs,
            NULL, -1, NULL,
            grammar_rules, ns_rules);
        strptr = strvec_i2str(ns_rules, lhs);

        if( strcmp(symbolseq[0].value, strptr) != 0 )
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
            lalr_parse_accel_cache_insert(
                symbolseq, expected_sym, true);
            return true;
        }
    }

    lalr_parse_accel_cache_insert(symbolseq, expected_sym, false);
    return false;
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
        return (void *)(long)production;

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

int lalr_parse(
    lalr_stack_t *restrict *restrict out,
    lalr_rule_t rules[restrict],
    void *restrict ctx,
    strvec_t *restrict ns_rules,
    token_shifter_t shifter,
    void *restrict shifter_ctx)
{
    // The sketch of this function is given in "docs/Parser Pseudo-Code.txt".

    lalr_stack_t *ps = NULL; // parsing stack.
    lalr_term_t *te, *sv; // term & saved.
    lalr_prod_t *rd; // reduced production.
    lex_token_t *tn; // token.

    lalr_rule_symbol_t const *mr; // match result.
    lalr_rule_symbol_t *expect, *expect_chain;
    int32_t last_resort_rule = -1, unique_rule = -1;
    int32_t candidate_rules_count, lookahead_rules_count;
    int32_t ri; // rule index.

    if( !(ps = lalr_stack_create()) ) return -1; // [host error].

    if( !(tn = shifter(shifter_ctx)) )
    {
        *out = NULL;
        s2obj_release(ps->pobj);

        // [parse error] - empty token sequence (2025-06-01).
        return -2;
    }

    sv = NULL;

    if( !(te = calloc(1, sizeof(lalr_term_t))) )
    {
        s2obj_release(tn->pobj);
        s2obj_release(ps->pobj);
        *out = NULL;
        return -1; // [host error].
    }
    te->container = ps;
    te->terminal = tn;
    // `te->production` is in union with `te->terminal`
    te->dn = te->up = NULL;
    te->anchored = true;
    te->expecting = NULL;

    ps->bottom = te;
    ps->top = te;

    // 2025-01-27:
    // Was scattered below. Considering info about the parsing stack is
    // needed by the "lexer hack", it's brought up here.
    *out = ps;

    while( true )
    {
        if( !tn && !sv )
        {
            if( ps->top == ps->bottom )
            {
                if( s2_is_prod(ps->bottom->production) )
                {
                    if( ps->bottom->production->rule == 0 )
                        break; // the goal symbol is reached.
                }
            }
        }

        dump_parsing_stack("\n========\n", ps->bottom, te, ns_rules, "--------\n");

        // Enumerate rules that have matches with the parsing stack.
        candidate_rules_count = 0;
        unique_rule = -1;
        mr = NULL;
        if( !(expect = calloc(1, sizeof(lalr_rule_symbol_t))) )
        {
            return -1; // [host error].
        }
        expect_chain = expect;
        for(ri=0; rules[ri]; ri++)
        {
            lalr_rule_symbol_t const *mt;

            // 2025-01-17:
            // a separate `mt` so that `mr` won't be mistakenly overwritten
            // in a case of `unique_rule`.
            if( !(mt = lalr_rule_match(rules, ri, te, ctx, ns_rules)) )
                continue;

            eprintf("  rule becomes candidate: %d.\n", ri);
            if( te->expecting && !lalr_rule_expect(
                    rules, ri, te, ctx, ns_rules) )
                continue;

            mr = mt;

            while( mt->type && mt->optional )
            {
                // 2025-01-19:
                // this block is present so that trailing optional terms
                // won't disrupt potential complete matches.
                mt++;
            }

            if( mt->type )
            {
                eprintf("  rule Prefix match: %d.\n", ri);
                candidate_rules_count++;
            }

            if( !mt->type )
            {
                eprintf("  rule  FULL  match: %d.\n", ri);
                candidate_rules_count++;

                // 2025-01-19:
                // see "docs/Parser Pseudo-Code.txt" for rationale.
                assert( unique_rule == -1 );
                unique_rule = ri;
            }

            if( mt == mr )
            {
                // 2025-01-27:
                // Currently, 'expectation' mechanism
                // doesn't handle optional terms.

                *expect_chain = *mr;
                if( !(expect_chain->next =
                      calloc(1, sizeof(lalr_rule_symbol_t))) )
                {
                    lalr_rule_symbol_free(expect);
                    return -1; // [host error].
                }
                expect_chain = expect_chain->next;
            }
        }

        assert( candidate_rules_count >= 0 );
        eprintf(" found: %d, uniq: %d, last-resort: %d.\n",
                candidate_rules_count, unique_rule, last_resort_rule);

        // All applicable rules are supposedly excluded by expectation (see
        // relevant note in "lalr.h" `lalr_rule_action_expect`).
        if( candidate_rules_count == 0 )
        {
            lalr_rule_symbol_free(expect);
            eprintf(" Reached clear anchor.\n");

            // Pop 1 anchor from the parsing stack, and
            // continue to parse previous terms.
            te = ps->top;
            if( !(te = terms_drop_1anch(te)) )
            {
                // [parse error] - encountered offending token.
                // All available rules had been tried on all
                // anchored sequences. (2025-06-01).

                // 2025-06-01:
                // Because this return procedurally precedes the shifting
                // of next look-ahead token, there might be a saved one
                // that needs to be released.

                eprintf("sv: %p, ", sv);
                if( sv )
                {
                    if( sv->terminal ){ // equivalently `->production`.
                        eprintf("%x.", sv->terminal->base.type);
                        s2obj_release(sv->terminal->pobj);
                    }
                    lalr_term_free(sv);
                }
                eprintf("\n");

                return -3;
            }
            continue;
        }

        eprintf(" Reached matches.\n");
        last_resort_rule = unique_rule;

        // shift 1 look-ahead token.
        if( sv )
        {
            eprintf(" Shifting Saved Look-Ahead.\n");
            sv->dn = ps->top;
            sv->anchored = false;
            ps->top->up = sv;
            ps->top = sv;
            sv = NULL;
            if( last_resort_rule == 0 )
                last_resort_rule = -1;
        }
        else
        {
            eprintf(" Shifting New Look-Ahead.\n");
            if( !(tn && (tn = shifter(shifter_ctx))) )
            {
                // There will not be further token to assign expectations to.
                lalr_rule_symbol_free(expect);

                if( last_resort_rule >= 0 ) goto reduction; else
                {
                    if( !(te = terms_drop_1anch(te)) )
                    {
                        // [parse error] - parsing ended without reaching
                        // the goal symbol (2025-06-01).
                        return -4;
                    }
                    continue;
                }
            }
            else
            {
                if( last_resort_rule == 0 )
                    last_resort_rule = -1;
            }

            eprintf(" Shifted: %p \"%s\".\n", tn,
                    (char *)(tn ? s2data_weakmap(tn->str) : NULL));

            if( !(te = calloc(1, sizeof(lalr_term_t))) )
            {
                s2obj_release(tn->pobj);
                s2obj_release(ps->pobj);
                *out = NULL;
                return -1; // [host error].
            }
            te->container = ps;
            te->terminal = tn;
            te->up = NULL;
            te->dn = ps->top;
            te->anchored = false;

            ps->top->up = te;
            ps->top = te;

            while( !te->anchored )
            {
                if( !te->dn ) break;
                te = te->dn;
            }
        }

        if( ps->top->expecting )
        {
            // 2025-01-27:
            // release any pre-existing expectation,
            // so as to be replaced by new one(s).
            lalr_rule_symbol_free(ps->top->expecting);
        }
        ps->top->expecting = expect;

        dump_parsing_stack("-- -- --\n", ps->bottom, te, ns_rules, "-- -- --\n");

        lookahead_rules_count = 0;
        for(ri=0; rules[ri]; ri++)
        {
            if( !lalr_rule_match(rules, ri, te, ctx, ns_rules) )
                continue;

            if( te->expecting && !lalr_rule_expect(
                    rules, ri, te, ctx, ns_rules) )
                continue;

            lookahead_rules_count++;
        }

        // was a partial match.
        if( lookahead_rules_count >= 1 )
        {
            eprintf(" Reached further match with: %d.\n",
                    (int)lookahead_rules_count);
            continue;
        }

        // 2025-06-01:
        // @dannyniu checked that there's no code - conditional or not, that
        // altered `sv` between the shifting of saved or new token and
        // this newly added assertion. Barring new changes in the future
        // of course,
        assert( !sv );

        // ought to be expecting a LHS. as such, anchor the top of stack
        // so that it can be reduced (possibly grown with further tokens)
        // into what's expected.
        //
        // 2025-01-19:
        // it's also possible that it's a symbol not found in the start of
        // any production, in which case unshift it.
        //
        // 2025-01-27:
        // the 'expectation' mechanism additionally ensures if the symbol
        // is found in the start of any production, and that production
        // cannot follow the previous contents of the parsing stack, the
        // rule would be excluded.
        if( last_resort_rule == -1 )
        {
            eprintf(" Reached new *potential* sub grammar tree.\n");
            ps->top->anchored = true;
            te = ps->top;
            symbol_print_expect_chain(te->expecting);

            lookahead_rules_count = 0;
            for(ri=0; rules[ri]; ri++)
            {
                lalr_rule_symbol_t const *mt;
                if( !(mt = lalr_rule_match(rules, ri, te, ctx, ns_rules)) )
                    continue;

                eprintf("  rule becomes candidate: %d.\n", ri);
                if( te->expecting && !lalr_rule_expect(
                        rules, ri, te, ctx, ns_rules) )
                    continue;

                if( mt->type )
                {
                    eprintf("  rule Prefix match: %d.\n", ri);
                }

                if( !mt->type )
                {
                    eprintf("  rule  FULL  match: %d.\n", ri);
                }

                lookahead_rules_count++;
            }

            if( lookahead_rules_count >= 1 )
            {
                // 2025-01-27:
                // The beginning of a new term. This won't conflict
                // with the previous contents of the parsing stack,
                // as any such rule would be excluded by
                // the 'expectation' mechanism.
                continue;
            }

            eprintf(" Unshift the *Offending* Look-Ahead,\n");
            if( sv )
            {
                // 2025-06-01:
                // See note above `assert( !sv )` added by @dannyniu.
                return -5;
            }
            sv = ps->top;
            ps->top = sv->dn;
            ps->top->up = NULL;

            eprintf(" and clear 1 recent-most anchor.\n");
            te = ps->top;
            if( !(te = terms_drop_1anch(te)) )
            {
                // [parse error] - encountered offending token.
                // All available rules had been tried on all
                // anchored sequences. This is the same reason
                // as -3, but whose origin warrants distinction.
                // (2025-06-01).
                return -6;
            }
            continue;
        }

        eprintf(" Unshift the Look-Ahead.\n");
        if( sv )
        {
            // 2025-06-01:
            // See note above `assert( !sv )` added by @dannyniu.
            return -7;
        }
        sv = ps->top;
        ps->top = sv->dn;
        ps->top->up = NULL;

    reduction:
        eprintf(" Applying rule.\n");

        // set `te` to the recent-most anchored term.
        te = ps->top;
        while( !te->anchored )
        {
            if( !te->dn ) break;
            te = te->dn;
        }

        // invoke rule.
        rd = lalr_rule_reduce(rules, last_resort_rule, te, ctx, ns_rules);
        if( !rd )
        {
            s2obj_release(ps->pobj);
            *out = NULL;
            return -1; // [host error].
        }

        ps->top = te;

        if( !tn )
        {
            // 2025-01-27:
            // No more tokens. Codes around `goto reduction; else` will
            // try to clear anchors and attempt to reduce to the goal symbol.
        }
        else //*/
        {
            // 2025-01-19:
            // sanity check 01: `te` is the top of the stack.
            assert( !te->up );

            // 2025-01-19:
            // sanity-check 02: `te` is anchored as it has been.
            assert( te->anchored );
        }

        // forget the 'last-resort rule'.
        last_resort_rule = -1;

        // reposition `te` to the recent-most anchored term and loop on.
        while( !te->anchored )
        {
            if( !te->dn ) break;
            te = te->dn;
        }
        continue;
    }

    return 0;
}
