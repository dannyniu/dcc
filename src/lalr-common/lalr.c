/* DannyNiu/NJF, 2025-01-01. Public Domain. */

#include "lalr-02matcher.bits.h"

int lalr_parse(
    lalr_stack_t *restrict *restrict out,
    lalr_rule_t rules[restrict],
    void *restrict ctx,
    strvec_t *restrict ns_rules,
    token_shifter_t shifter,
    void *restrict shifter_ctx)
{
    // The sketch of this function was given in "docs/Parser Pseudo-Code.txt".
    //
    // 2026-07-27: an updated one is provided here.
    //
    // The main loop of my (@dannyniu) parsing algorithm, consist of a
    // shift-match-lookahead loop.
    //
    // ```text/plain
    // 'start:
    // ^Assert that all matches obey expectation.
    // {Shift}
    // {r_cand := Match}
    // {Shift}
    // {r_peek := Match}
    // [#r_peek > 0] -> goto 'start;
    // [#r_cand == 0] -> {DropAnchor}, then goto 'start;
    // goto 'anchrd
    // ```
    //
    // The expectation mechanism is explained in
    // the comment for `lalr_rule_action_expect`.
    // It eliminates spurious candidate rules.
    //
    // ```text/plain
    // 'anchrd:
    // -- The @HaveReductionCandidates and @HaveWinningReduction predicates
    // -- checks respectively whether there are rules in the set matches
    // -- the term sequence, and there is exactly one rule matches exactly
    // -- the term sequence respectively
    // @HaveReductionCandidates(rules, termseq) := #rules > 1 || rules_0.@matchesPartially(termseq)
    // @HaveWinningReduction(rules, termseq) := #rules == 1 && rules_0.@matchesFully(termseq)
    //
    // -- one winning rule overrides everything else.
    // [@HaveWinningReduction(r_cand, ASEQ)] -> goto 'reduce;
    //
    // -- the look-ahead is probably starting a new ASEQ (anchored sequence of terms).
    // {Anchor}
    // {r_tmp1 := Match}
    // [@HaveWinningReduction(r_cand, ASEQ)] -> goto 'reduce;
    // [@HaveReductionCandidates(r_cand, ASEQ)] -> goto 'start;
    //
    // -- unshifts the last term (thus clearing its anchor).
    // {UnShift}
    //
    // -- the next-most-recent ASEQ is consulted:
    // -- The set of matched rules gives the new set of expected symbols
    // ^Reinstate Expectations from ASEQ.prevASEQ (i.e. the next-most-recent ASEQ)
    // {Shift}
    // {Anchor}
    // {r_tmp2 := Match}
    // [@HaveReductionCandidates(r_tmp2, ASEQ)] -> goto 'start;
    // ```
    //
    // The `@HaveWinningReduction` predicate is the unequivocal condition
    // for a reduce action. And that unequivocalness stems from the rigor
    // of this procedure, and the well-formedness of the grammar it parses.
    //
    // ```text/plain
    // 'reduce:
    // {UnShift}
    // {Reduce}
    // goto 'start;
    // ```
    //

    lalr_stack_t *ps = NULL; // parsing stack.
    lalr_term_t *te, *sv; // term & saved.
    lalr_prod_t *rd; // reduced production.
    lex_token_t *tn; // token.

    lalr_rule_symbol_t *expect;
    int32_t last_resort_rule = -1, unique_rule = -1;
    int32_t candidate_rules_count, lookahead_rules_count;
    int32_t ri; // rule index.

    uint32_t ruleset_cardinality;
    uint32_t *candidates_bitmap = NULL;

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

    for(ri=0; rules[ri]; ri++);
    ruleset_cardinality = ri;
    candidates_bitmap = calloc((ri + 31)/32, sizeof(uint32_t));

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

        candidate_rules_count = lalr_stack_matcher(
            &unique_rule, &expect,
            ps, te, rules, ctx, ns_rules,
            candidates_bitmap,
            ruleset_cardinality);

        if( candidate_rules_count < 0 )
        {
            if( candidates_bitmap )
                free(candidates_bitmap);
            return -1; // host error.
        }

        assert( candidate_rules_count >= 0 );
        Reached(" found: %d, uniq: %d, last-resort: %d.\n",
                candidate_rules_count, unique_rule, last_resort_rule);

        // All applicable rules are supposedly excluded by expectation (see
        // relevant note in "lalr.h" `lalr_rule_action_expect`).
        if( candidate_rules_count == 0 )
        {
            lalr_rule_symbol_free(expect);
            Reached(" Reached clear anchor.\n");

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

                Reached("sv: %p, ", sv);
                if( sv )
                {
                    if( sv->terminal ) { // equivalently `->production`.
                        eprintf("%x.", sv->terminal->base.type);
                        s2obj_release(sv->terminal->pobj);
                    }
                    lalr_term_free(sv);
                }
                eprintf("\n");

                if( candidates_bitmap ) free(candidates_bitmap);
                return -3;
            }
            continue;
        }

        Reached(" Reached matches.\n");
        last_resort_rule = unique_rule; // 2026-07-25 TODO: I was dead here.

        if( candidate_rules_count == 1 && last_resort_rule >= 0 )
        {
            // This block is added 2026-07-29.
            // The only rule and it's a FULL match,
            // reduce without consulting look-ahead.
            // The C lexer depends on scopes being closed on leave
            // to ensure unshadowing of declarations of identifiers
            // in outter scopes.
            lalr_rule_symbol_free(expect);
            goto reduction;
        }

        // shift 1 look-ahead token.
        if( sv )
        {
            Reached(" Shifting Saved Look-Ahead.\n");
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
            Reached(" Shifting New Look-Ahead.\n");
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
                        if( candidates_bitmap ) free(candidates_bitmap);
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

            Reached(" Shifted: %p \"%s\".\n", tn,
                    (char *)(tn ? s2data_weakmap(tn->str) : NULL));

            if( !(te = calloc(1, sizeof(lalr_term_t))) )
            {
                s2obj_release(tn->pobj);
                s2obj_release(ps->pobj);
                *out = NULL;
                if( candidates_bitmap ) free(candidates_bitmap);
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

        lookahead_rules_count = lalr_stack_sieve(
            ps, te, rules, ctx, ns_rules,
            candidates_bitmap,
            ruleset_cardinality);

        if( lookahead_rules_count < 0 )
        {
            if( candidates_bitmap )
                free(candidates_bitmap);
            return -1; // host error.
        }

        // was a partial match.
        if( lookahead_rules_count >= 1 )
        {
            Reached(" Reached further match with: %d.\n",
                    (int)lookahead_rules_count);
            continue;
        }

        // 2025-06-01:
        // @dannyniu checked that there's no code - conditional or not, that
        // altered `sv` between the shifting of saved or new token and
        // this newly added assertion. Barring new changes in the future
        // of course,
        assert( !sv );

        // 2026-07-27:
        // The `anchrd` label.
        if( last_resort_rule == -1 ||
            // Added 2026-07-26:
            // If the unique rule wasn't the only rule,
            // don't consider it as the last-resort rule either.
            (unique_rule > 0 && candidate_rules_count > 1) )
        {
            int32_t rc, ru;
            Reached(" Reached new *potential* sub grammar tree.\n");
            ps->top->anchored = true;
            te = ps->top;

            symbol_print_expect_chain(te->expecting);

            rc = lalr_stack_matcher(
                &ru, NULL,
                ps, te, rules, ctx, ns_rules,
                candidates_bitmap,
                ruleset_cardinality);
            
            if( rc < 0 )
            {
                if( candidates_bitmap )
                    free(candidates_bitmap);
                return -1; // host error.
            }

            // 2026-07-27:
            // either there's no 'FULL'-match unique rule, or too many rules.
            if( (rc == 1 && ru < 0) || rc > 1 )
            {
                // 2025-01-27:
                // The beginning of a new term. This won't conflict
                // with the previous contents of the parsing stack,
                // as any such rule would be excluded by
                // the 'expectation' mechanism.
                continue;
            }
            else if( ru >= 0 ) // @HaveWinningReduction.
            {
                last_resort_rule = ru;
                goto reduction;
            }

            Reached(" Unshift the *Offending* Look-Ahead,\n");
            if( sv )
            {
                // 2025-06-01:
                // See note above `assert( !sv )` added by @dannyniu.
                if( candidates_bitmap ) free(candidates_bitmap);
                return -5;
            }
            sv = ps->top;
            ps->top = sv->dn;
            ps->top->up = NULL;

            // 2026-07-27.
            // To reinstate the expectations from the next-most-recent ASEQ.
            // There **won't ever** be need for a 2nd-next-most-recent ASEQ,
            // as the last one isn't completed yet.
            Reached(" try look back across an anchor first.\n");
            te = ps->top;
            while( !te->anchored )
            {
                if( !te->dn ) break;
                te = te->dn;
            }
            if( te ) te = te->dn;
            while( te && !te->anchored )
            {
                if( !te->dn ) break;
                te = te->dn;
            }
            if( !te )
            {
                Reached(" too few anchors, go straight to reduce.\n");
                goto reduction;
            }

            lookahead_rules_count = lalr_stack_matcher(
                NULL, &expect,
                ps, te, rules, ctx, ns_rules,
                candidates_bitmap,
                ruleset_cardinality);

            if( lookahead_rules_count < 0 )
            {
                if( candidates_bitmap )
                    free(candidates_bitmap);
                return -1; // host error.
            }

            if( lookahead_rules_count == 0 )
            {
                Reached(" it needs reducing to fit.\n");
                lalr_rule_symbol_free(expect);
                goto reduction;
            }

            assert( sv );
            if( sv->expecting )
            {
                lalr_rule_symbol_free(sv->expecting);
            }
            sv->expecting = expect;
            sv->dn = ps->top;
            sv->anchored = true;
            ps->top->up = sv;
            te = ps->top = sv;
            sv = NULL;

            lookahead_rules_count = lalr_stack_matcher(
                NULL, NULL,
                ps, te, rules, ctx, ns_rules,
                candidates_bitmap,
                ruleset_cardinality);

            if( lookahead_rules_count < 0 )
            {
                if( candidates_bitmap )
                    free(candidates_bitmap);
                return -1; // host error.
            }

            if( lookahead_rules_count >= 1 )
                continue;

            if( last_resort_rule == -1 )
            {
                // 2026-07-27:
                // The parent block considers
                // `not @HaveWinningReduction`,
                // whereas the clearing of
                // recent-most anchor require
                // `not @HaveReductionCandidates`.

                sv = ps->top;
                ps->top = sv->dn;
                ps->top->up = NULL;
                te = ps->top;

                Reached(" and clear 1 recent-most anchor.\n");
                if( !(te = terms_drop_1anch(te)) )
                {
                    // [parse error] - encountered offending token.
                    // All available rules had been tried on all
                    // anchored sequences. This is the same reason
                    // as -3, but whose origin warrants distinction.
                    // (2025-06-01).
                    if( candidates_bitmap ) free(candidates_bitmap);
                    return -6;
                }
                continue;
            }
        }

        Reached(" Unshift the Look-Ahead.\n");
        if( sv )
        {
            // 2025-06-01:
            // See note above `assert( !sv )` added by @dannyniu.
            if( candidates_bitmap ) free(candidates_bitmap);
            return -7;
        }
        sv = ps->top;
        ps->top = sv->dn;
        ps->top->up = NULL;

    reduction:
        Reached(" Applying rule.\n");

        // set `te` to the recent-most anchored term.
        te = ps->top;
        while( !te->anchored )
        {
            if( !te->dn ) break;
            te = te->dn;
        }

        // invoke rule.
        assert( last_resort_rule >= 0 );
        rd = lalr_rule_reduce(rules, last_resort_rule, te, ctx, ns_rules);
        if( !rd )
        {
            s2obj_release(ps->pobj);
            *out = NULL;
            if( candidates_bitmap ) free(candidates_bitmap);
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

    if( candidates_bitmap ) free(candidates_bitmap);
    return 0;
}
