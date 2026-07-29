/* DannyNiu/NJF, 2025-01-01. Public Domain. */

#include "lalr-01misc.bits.h"

// returns -1 on host error and less on logic errors.
// returns the number of rules matched on success.
int32_t lalr_stack_matcher(
    // result output.
    // ====
    //
    // non-negative if there's a 'FULL' matching rule.
    int32_t *restrict out_ri_uniq,
    //
    // the expectation for the next symbol.
    // freed by caller.
    lalr_rule_symbol_t *restrict *restrict out_expected,

    // Unowned, immutable.
    lalr_stack_t *restrict ps, // parsing stack.
    lalr_term_t *restrict te, // base term (usually anchored).
    lalr_rule_t rules[restrict], // contextual info,
    void *restrict ctx, // contextual info,
    strvec_t *restrict ns_rules, // contextual info,

    // borrowed, mutable.
    uint32_t *candidates_bitmap, // working ctx var,
    uint32_t ruleset_cardinality) // working ctx var.
{
    lalr_rule_symbol_t *expect, *expect_chain;
    int32_t unique_rule = -1;
    int32_t candidate_rules_count;
    int32_t ri; // rule index.

    dump_parsing_stack("\n........\n", ps->bottom, te, ns_rules, "........\n");

    // Enumerate rules that have matches with the parsing stack.
    Candidates_Clear();
    candidate_rules_count = 0;
    unique_rule = -1;

    if( out_expected )
    {
        if( !(expect = calloc(1, sizeof(lalr_rule_symbol_t))) )
        {
            return -1; // [host error].
        }
        expect_chain = expect;
    }

    for(ri=0; rules[ri]; ri++)
    {
        lalr_rule_symbol_t const *mt;

        // 2025-01-17:
        // a separate `mt` so that `mr` won't be mistakenly overwritten
        // in a case of `unique_rule`.
        if( !(mt = lalr_rule_match(rules, ri, te, ctx, ns_rules)) )
            continue;

        eprintf("  Rule becomes candidate: %d.\n", ri);
        /*if( te->expecting && !lalr_rule_expect(
          rules, ri, te, ctx, ns_rules) )
          // continue // 2026-07-25: delegated by the below block.
          ; // */

        if( te->anchored )
        {
            // 2026-07-25, to see if this works:
            // Additionally, for the first term on the stack,
            // exclude rules that doesn't lead to the goal symbol.

            lalr_rule_symbol_t *goalsyms = te->expecting;
            int32_t production = (int32_t)(intptr_t)rules[ri](
                lalr_rule_inspect_lhs, NULL, ri, NULL, rules, ns_rules);
            lalr_rule_symbol_t expect_symbol = {};
            const char *lhs = strvec_i2str(ns_rules, production);

            expect_symbol.type = lalr_symtype_prod;
            expect_symbol.value = lhs;

            if( !goalsyms )
            {
                goalsyms = rules[0](
                    lalr_rule_inspect_symseq,
                    NULL, 0, NULL, rules, ns_rules);
            }

            while( goalsyms && !begins_with_expected(
                       goalsyms, &expect_symbol,
                       NULL, rules, ns_rules) )
            {
                goalsyms = goalsyms->next;
            }

            if( !goalsyms && (te != ps->bottom || ri != 0) )
            {
                eprintf("  Rule excluded for goal: %d.\n", ri);
                continue;
            }
        }

        while( mt->type && mt->optional )
        {
            // 2025-01-19:
            // this block is present so that trailing optional terms
            // won't disrupt potential complete matches.

            if( out_expected )
            {
                // 2026-07-16:
                // make the expectation mechanism additionally aware
                // of the potential optional terms.
                *expect_chain = *mt;
                if( !(expect_chain->next =
                      calloc(1, sizeof(lalr_rule_symbol_t))) )
                {
                    lalr_rule_symbol_free(expect);
                    return -1; // [host error].
                }
                expect_chain = expect_chain->next;
            }

            mt++;
        }

        if( mt->type )
        {
            eprintf("  Rule Prefix match: %d.\n", ri);
            candidate_rules_count++;
        }

        if( !mt->type )
        {
            eprintf("  Rule  FULL  match: %d.\n", ri);
            candidate_rules_count++;

            // 2025-01-19:
            // see "docs/Parser Pseudo-Code.txt" for rationale.
            //
            // 2026-07-29:
            // The caller (i.e. `lalr_parse`) can now distinguish
            // between FULL-match rules as long as the grammar
            // is well-formed.
            //- assert( unique_rule == -1 );

            unique_rule = ri;
        }

        if( out_expected )
        {
            // 2025-01-27:
            // Currently, 'expectation' mechanism
            // doesn't handle optional terms.
            // 2026-07-16 T 11:25 UTC+8, Moment of truth...

            *expect_chain = *mt;
            if( !(expect_chain->next =
                  calloc(1, sizeof(lalr_rule_symbol_t))) )
            {
                lalr_rule_symbol_free(expect);
                return -1; // [host error].
            }
            expect_chain = expect_chain->next;
        }

        Candidates_Set(ri);
    }

    if( out_ri_uniq )
        *out_ri_uniq = unique_rule;

    if( out_expected )
        *out_expected = expect;

    return candidate_rules_count;
}

int32_t lalr_stack_sieve(
    // Unowned, immutable.
    lalr_stack_t *restrict ps, // parsing stack.
    lalr_term_t *restrict te, // base term (usually anchored).
    lalr_rule_t rules[restrict], // contextual info,
    void *restrict ctx, // contextual info,
    strvec_t *restrict ns_rules, // contextual info,

    // borrowed, mutable.
    uint32_t *candidates_bitmap, // working ctx var,
    uint32_t ruleset_cardinality) // working ctx var.
{
    int32_t lookahead_rules_count = 0;
    int32_t ri; // rule index.

    (void)ps;
    (void)ruleset_cardinality;
    
    for(ri=0; rules[ri]; ri++)
    {
        if( !Candidate_Test(ri) )
            continue;

        if( !lalr_rule_match(rules, ri, te, ctx, ns_rules) )
        {
            Candidate_Drop(ri);
            continue;
        }

        lookahead_rules_count++;
    }

    return lookahead_rules_count;
}
