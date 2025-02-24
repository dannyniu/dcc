/* DannyNiu/NJF, 2025-01-01. Public Domain. */

#include "lalr.h"

//#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#define eprintf(...) ((void)0)

static void lalr_prod_final(lalr_prod_t *ctx)
{
    size_t t;

    if( ctx->value ) s2obj_release(ctx->value);

    for(t=0; t<ctx->terms_count; t++)
    {
        // could also be `... production->base`.
        s2obj_release(&ctx->terms[t].terminal->base);
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
    free(term);
}

static bool lalr_symbol_matches_term(
    lalr_rule_symbol_t const *symbol,
    lalr_term_t *term, strvec_t *ns_rules)
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
    lalr_term_t *anchterm, strvec_t *ns_rules)
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
    lalr_rule_symbol_t const *symbolseq, int32_t production,
    lalr_term_t *anchterm, strvec_t *ns_rules)
{
    // less checks are made, lenient on undefined behaviors.

    size_t terms_count = 0, i;
    lalr_term_t *terms = anchterm, *temp;
    lalr_prod_t *newprod = NULL;

    for(terms_count = 0; symbolseq[terms_count].type; terms_count++);

    newprod = lalr_prod_create(terms_count);
    if( !newprod ) return NULL;

    eprintf("prod: %s; ", strvec_i2str(ns_rules, production));

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

    newprod->production = production;
    // newprod->rule // Assigned from a calling routine.

    eprintf("\n");
    return newprod;
}

static bool (lalr_rule_expect)(
    lalr_rule_symbol_t const *symbolseq, int32_t production,
    lalr_term_t const *term_expectation, strvec_t *ns_rules)
{
    lalr_rule_symbol_t *expect_chain;
    const char *lhs;

    if( symbolseq->type == lalr_symtype_stoken ||
        symbolseq->type == lalr_symtype_vtoken )
    {
        // If the current state expect a terminal (in the symbol sequence),
        // and the term is one, then return true.

        for(expect_chain = term_expectation->expecting;
            expect_chain; expect_chain = expect_chain->next)
        {
            if( expect_chain->type == lalr_symtype_prod )
            {
                // 2025-01-20:
                // Because expectation is a non-terminal, and the 1st symbol
                // in the current rule isn't one, donot apply the current rule.
                return true;
            }

            if( expect_chain->type != symbolseq->type )
                continue;

            if( expect_chain->type == lalr_symtype_stoken )
                if( strcmp(expect_chain->value, symbolseq->value) == 0 )
                    break;

            if( expect_chain->type == lalr_symtype_vtoken )
                if( expect_chain->vtype == symbolseq->vtype )
                    break;
        }

        if( expect_chain )
            return true;
        else return false;
    }

    assert( symbolseq->type == lalr_symtype_prod );

    for(expect_chain = term_expectation->expecting;
        expect_chain; expect_chain = expect_chain->next)
    {
        if( expect_chain->type != symbolseq->type )
            continue;

        if( strcmp(expect_chain->value, symbolseq->value) == 0 )
            break;
    }

    if( !expect_chain )
    {
        // 2025-01-20:
        // The 1st symbol of the rule is none that's inside the expected set.
        return true;
    }

    lhs = strvec_i2str(ns_rules, production);
    for(expect_chain = term_expectation->expecting;
        expect_chain; expect_chain = expect_chain->next)
    {
        if( expect_chain->type != symbolseq->type )
            continue;

        if( strcmp(expect_chain->value, lhs) == 0 )
            break;
    }

    if( !expect_chain )
    {
        // 2025-01-20:
        // the rule led to an LHS that's not expected
        // by a previous one on the parsing stack.
        return false;
    }

    return true;
}

void *lalr_rule_actions_generic(
    lalr_rule_symbol_t *restrict symbolseq, int32_t production,
    lalr_rule_action_t action,
    lalr_term_t *restrict terms,
    // `ctx` is used by rules themselves, to e.g. build semantics.
    strvec_t *restrict ns_rules)
{
    switch( action )
    {
    case lalr_rule_action_match:
        return (void *)(lalr_rule_match)(symbolseq, terms, ns_rules);

    case lalr_rule_action_reduce:
        return (lalr_rule_reduce)(symbolseq, production, terms, ns_rules);

    case lalr_rule_action_expect:
        return (void *)(intptr_t)(lalr_rule_expect)(
            symbolseq, production, terms, ns_rules);

    default:
        return NULL;
    }
}

#define lalr_rule_match(rule, terms, ctx, strtab)               \
    (((lalr_rule_symbol_t const *(*)(lalr_rule_params))rule)    \
     (lalr_rule_action_match, terms, ctx, strtab))

#define lalr_rule_reduce(rule, terms, ctx, strtab)      \
    (((lalr_prod_t *(*)(lalr_rule_params))rule)         \
     (lalr_rule_action_reduce, terms, ctx, strtab))

#define lalr_rule_expect(rule, terms, ctx, strtab)      \
    ((bool)((intptr_t (*)(lalr_rule_params))rule)       \
     (lalr_rule_action_expect, terms, ctx, strtab))

static void lalr_stack_final(lalr_stack_t *ctx)
{
    lalr_term_t *t = ctx->bottom, *s;

    while( t )
    {
        s = t->up;

        // could also be `... terminal->base`.
        s2obj_release(&t->production->base);
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
        else eprintf("!!, ");

        chain = chain->next;
    }
    eprintf("\n");
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

    if( !(ps = lalr_stack_create()) ) return -1;

    if( !(tn = shifter(shifter_ctx)) )
    {
        *out = NULL;
        s2obj_release(&ps->base);
        return -2;
    }

    sv = NULL;

    if( !(te = calloc(1, sizeof(lalr_term_t))) )
    {
        s2obj_release(&tn->base);
        s2obj_release(&ps->base);
        *out = NULL;
        return -1;
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

        eprintf("\n========\n");
        {
            lalr_term_t *bt;
            for(bt = ps->bottom; bt; bt = bt->up)
            {
                if( bt == te ) eprintf("^");
                if( bt->anchored ) eprintf("!");
                if( s2_is_prod(&bt->production->base) )
                    eprintf("%s, ", strvec_i2str(
                                ns_rules, bt->production->production));
                else eprintf("\"%s\", ", (char *)s2data_weakmap(
                                 bt->terminal->str));
            }
            eprintf("\n");
        }
        eprintf("--------\n");

        // Enumerate rules that have matches with the parsing stack.
        candidate_rules_count = 0;
        unique_rule = -1;
        mr = NULL;
        if( !(expect = calloc(1, sizeof(lalr_rule_symbol_t))) )
        {
            return -1;
        }
        expect_chain = expect;
        for(ri=0; rules[ri]; ri++)
        {
            lalr_rule_symbol_t const *mt;

            // 2025-01-17:
            // a separate `mt` so that `mr` won't be mistakenly overwritten
            // in a case of `unique_rule`.
            if( !(mt = lalr_rule_match(rules[ri], te, ctx, ns_rules)) )
                continue;

            if( te->expecting && !lalr_rule_expect(
                    rules[ri], te, ctx, ns_rules) )
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
                    return -1;
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
                        return -4;
                    }
                    continue;
                }
            }

            eprintf(" Shifted: %p \"%s\".\n", tn,
                    (char *)(tn ? s2data_weakmap(tn->str) : NULL));

            if( !(te = calloc(1, sizeof(lalr_term_t))) )
            {
                s2obj_release(&tn->base);
                s2obj_release(&ps->base);
                *out = NULL;
                return -1;
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
        eprintf(" == expect: %p.\n", expect);
        if( expect ) symbol_print_expect_chain(expect);

        eprintf("-- -- --\n");
        {
            lalr_term_t *bt;
            for(bt = ps->bottom; bt; bt = bt->up)
            {
                if( bt == te ) eprintf("^");
                if( bt->anchored ) eprintf("!");
                if( s2_is_prod(&bt->production->base) )
                    eprintf("%s, ", strvec_i2str(
                                ns_rules, bt->production->production));
                else eprintf("\"%s\", ", (char *)s2data_weakmap(
                                 bt->terminal->str));
            }
            eprintf("\n");
        }
        eprintf("-- -- --\n");

        lookahead_rules_count = 0;
        for(ri=0; rules[ri]; ri++)
        {
            if( !lalr_rule_match(rules[ri], te, ctx, ns_rules) )
                continue;

            if( te->expecting && !lalr_rule_expect(
                    rules[ri], te, ctx, ns_rules) )
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
                if( !(mt = lalr_rule_match(rules[ri], te, ctx, ns_rules)) )
                    continue;

                if( te->expecting && !lalr_rule_expect(
                        rules[ri], te, ctx, ns_rules) )
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
                return -5;
            }
            sv = ps->top;
            ps->top = sv->dn;
            ps->top->up = NULL;

            eprintf(" and clear 1 recent-most anchor.\n");
            te = ps->top;
            if( !(te = terms_drop_1anch(te)) )
            {
                return -6;
            }
            continue;
        }

        eprintf(" Unshift the Look-Ahead.\n");
        if( sv )
        {
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
        rd = lalr_rule_reduce(rules[last_resort_rule], te, ctx, ns_rules);
        if( !rd )
        {
            s2obj_release(&ps->base);
            *out = NULL;
            return -1;
        }

        // `lalr_rule_reduce` didn't know what value to assign.
        rd->rule = last_resort_rule;

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
