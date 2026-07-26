/* DannyNiu/NJF, 2026-03-21. Public Domain. */

// Implements control expression for `#{if,elif}` directives.

#include "cpp-c.h"
#include "ppexpr-grammar.h"

#define GRAMMAR_RULES ppexpr_grammar_rules
#define NS_RULES ns_rules_ppexpr

lex_token_t *shift_from_s2list(s2list_t *toklist)
{
    lex_token_t *tok = NULL;

    if( s2list_len(toklist) > 0 )
    {
        s2list_seek(toklist, 0, S2_LIST_SEEK_SET);
        s2list_shift_T(lex_token_t)(toklist, &tok);
    }

    return tok;
}

static inline lalr_rule_t rules(int32_t r)
{
    return ppexpr_grammar_rules[r];
}

#define theRule rules(sp->body->semantic_rule)

typedef struct eval_stack_chained_ctx eval_stack_chained_t;

struct eval_stack_chained_ctx {
    eval_stack_chained_t *ret;
    size_t operand_index;
    lalr_prod_t *body;
};

#define create_value(lval, type)                                \
    lval = lval ? lval : (s2obj_t *)s2data_create(sizeof(type))

#define intmax_eval_operand(n)                                  \
    *(intmax_t *)s2data_weakmap(                                \
        (s2data_t *)sp->body->terms[n].production->value)

#define uintmax_eval_operand(n)                                 \
    *(uintmax_t *)s2data_weakmap(                               \
        (s2data_t *)sp->body->terms[n].production->value)

#define signedness_of_operand(n)                                \
    ((s2data_t *)sp->body->terms[n].production->value)->ctxinfo

// returns:
// - 0 if successful,
// - 1 if the result of evaluation was signed,
// - -1 if there were errors.
int intmax_eval_start(
    uintmax_t *ret,
    lalr_prod_t *expr)
{
    eval_stack_chained_t rs_anch = {
        .ret = NULL,
        .body = expr,
        .operand_index = 0,
    };
    eval_stack_chained_t *sp = &rs_anch, *bp;
    int signedness; // 1: signed, 0: unsigned.
    uintmax_t *valreg;

start_eval_1term:

    while( sp->operand_index < sp->body->terms_count )
    {
        if( !s2_is_prod(sp->body->terms[sp->operand_index].production) )
        {
            sp->operand_index ++;
            continue;
        }

        bp = calloc(1, sizeof(eval_stack_chained_t));
        assert( bp );

        bp->ret = sp;
        bp->operand_index = 0;
        bp->body = sp->body->terms[sp->operand_index].production;

        sp = bp;
        goto start_eval_1term;
    }

    create_value(sp->body->value, uintmax_t);
    valreg = s2data_map((s2data_t *)sp->body->value, 0, sizeof(uintmax_t));

    if( theRule == pp_goal_pp_const_expr )
    {
        *valreg = uintmax_eval_operand(0);
    }
    else if( theRule == pp_ident_ident )
    {
        assert( 0 ); // All identifiers should've been macro-replaced.
    }
    else if( theRule == pp_const_true )
    {
        *valreg = 1;
        signedness = 1;
    }
    else if( theRule == pp_const_false )
    {
        *valreg = 0;
        signedness = 1;
    }
    else if( theRule == pp_const_null )
    {
        // 2026-05-16:
        // This never happens.
        assert( 0 );
    }
    else if( theRule == pp_const_binlit )
    {
        const char *p = s2data_weakmap(
            (s2data_t *)sp->body->terms[0].terminal->str);

        assert( p[0] == '0' );
        assert( p[1] == 'b' || p[1] == 'B' );

        *valreg = 0;
        p += 2;

        while( *p )
        {
            *valreg <<= 1;
            if( *p == '1' ) *valreg |= 1;
            if( *p == '0' ) *valreg |= 0;
            p++;
        }

        signedness = 0;
    }
    else if( theRule == pp_const_declit )
    {
        const char *p = s2data_weakmap(
            (s2data_t *)sp->body->terms[0].terminal->str);

        *valreg = 0;

        signedness = 1;
        while( *p )
        {
            *valreg *= 10;
            if( '0' <= *p && *p <= '9' ) *valreg += *p - '0';
            if( *p == 'u' || *p == 'U' ) signedness = 0;
            p++;
        }
    }
    else if( theRule == pp_const_octlit )
    {
        const char *p = s2data_weakmap(
            (s2data_t *)sp->body->terms[0].terminal->str);

        *valreg = 0;

        while( *p )
        {
            *valreg <<= 3;
            if( '0' <= *p && *p <= '7' ) *valreg += *p - '0';
            p++;
        }

        signedness = 0;
        if( s2data_len((s2data_t *)sp->body->terms[0].terminal->str) == 1 &&
            *valreg == 0 ) signedness = 1;
    }
    else if( theRule == pp_const_hexlit )
    {
        const char *p = s2data_weakmap(
            (s2data_t *)sp->body->terms[0].terminal->str);

        assert( p[0] == '0' );
        assert( p[1] == 'x' || p[1] == 'X' );

        *valreg = 0;
        p += 2;

        while( *p )
        {
            *valreg <<= 4;
            if( '0' <= *p && *p <= '9' ) *valreg += *p - '0';
            if( 'a' <= *p && *p <= 'f' ) *valreg += *p + 10 - 'a';
            if( 'A' <= *p && *p <= 'A' ) *valreg += *p + 10 - 'A';
            p++;
        }

        signedness = 0;
    }
    else if( theRule == pp_const_charlit )
    {
        // 2026-05-16 TODO: handle escape sequences.
        *valreg = ((uint8_t *)s2data_weakmap(
                       (s2data_t *)sp->body->terms[0].terminal->str))[0];
    }
    else if( theRule == pp_primary_paren )
    {
        *valreg = uintmax_eval_operand(1);
        signedness = signedness_of_operand(1);
    }
// omitting a lot for now (2026-05-15). 2026-07-01: probably no longer the case.
    else if( theRule == pp_unary_positive )
    {
        *valreg = intmax_eval_operand(1);
        signedness = signedness_of_operand(1);
    }
    else if( theRule == pp_unary_negative )
    {
        *valreg = -intmax_eval_operand(1);
        signedness = signedness_of_operand(1);
    }
    else if( theRule == pp_unary_bitcompl )
    {
        *valreg = -intmax_eval_operand(1);
        signedness = signedness_of_operand(1);
    }
    else if( theRule == pp_unary_logicnot )
    {
        *valreg = !intmax_eval_operand(1);
        signedness = 1; // did checking, unary `!` is always signed.
    }
    else if( theRule == pp_mulexpr_multiply )
    {
        *valreg =
            uintmax_eval_operand(0) *
            uintmax_eval_operand(2);
        signedness =
            signedness_of_operand(0) &
            signedness_of_operand(2);
    }
    else if( theRule == pp_mulexpr_divide )
    {
        signedness =
            signedness_of_operand(0) &
            signedness_of_operand(2);
        if( signedness == 1 )
            *(intmax_t *)valreg =
                intmax_eval_operand(0) /
                intmax_eval_operand(2);
        else // signedness == 0 // i.e. unsigned.
            *(uintmax_t *)valreg =
                uintmax_eval_operand(0) /
                uintmax_eval_operand(2);
    }
    else if( theRule == pp_mulexpr_remainder )
    {
        signedness =
            signedness_of_operand(0) &
            signedness_of_operand(2);
        if( signedness == 1 )
            *(intmax_t *)valreg =
                intmax_eval_operand(0) +
                intmax_eval_operand(2);
        else // signedness == 0 // i.e. unsigned.
            *(uintmax_t *)valreg =
                uintmax_eval_operand(0) %
                uintmax_eval_operand(2);
    }
    else if( theRule == pp_addexpr_add )
    {
        *valreg =
            uintmax_eval_operand(0) +
            uintmax_eval_operand(2);
        signedness =
            signedness_of_operand(0) &
            signedness_of_operand(2);
    }
    else if( theRule == pp_addexpr_subtract )
    {
        *valreg =
            uintmax_eval_operand(0) -
            uintmax_eval_operand(2);
        signedness =
            signedness_of_operand(0) &
            signedness_of_operand(2);
    }
    else if( theRule == pp_shiftexpr_lshift )
    {
        *valreg =
            uintmax_eval_operand(0) <<
            uintmax_eval_operand(2);
        signedness =
            signedness_of_operand(0);
    }
    else if( theRule == pp_shiftexpr_rshift )
    {
        signedness =
            signedness_of_operand(0);
        if( signedness == 1 )
            *valreg =
                intmax_eval_operand(0) <<
                intmax_eval_operand(2);
        else // signedness == 0 // i.e. unsigned.
            *valreg =
                uintmax_eval_operand(0) <<
                uintmax_eval_operand(2);
    }
    else if( theRule == pp_relops_lt )
    {
        signedness =
            signedness_of_operand(0) &
            signedness_of_operand(2);
        if( signedness == 1 )
            *valreg =
                intmax_eval_operand(0) <
                intmax_eval_operand(2);
        else // signedness == 0 // i.e. unsigned.
            *valreg =
                uintmax_eval_operand(0) <
                uintmax_eval_operand(2);
    }
    else if( theRule == pp_relops_gt )
    {
        signedness =
            signedness_of_operand(0) &
            signedness_of_operand(2);
        if( signedness == 1 )
            *valreg =
                intmax_eval_operand(0) <
                intmax_eval_operand(2);
        else // signedness == 0 // i.e. unsigned.
            *valreg =
                uintmax_eval_operand(0) <
                uintmax_eval_operand(2);
    }
    else if( theRule == pp_relops_le )
    {
        signedness =
            signedness_of_operand(0) &
            signedness_of_operand(2);
        if( signedness == 1 )
            *valreg =
                intmax_eval_operand(0) <=
                intmax_eval_operand(2);
        else // signedness == 0 // i.e. unsigned.
            *valreg =
                uintmax_eval_operand(0) <=
                uintmax_eval_operand(2);
    }
    else if( theRule == pp_relops_ge )
    {
        signedness =
            signedness_of_operand(0) &
            signedness_of_operand(2);
        if( signedness == 1 )
            *valreg =
                intmax_eval_operand(0) <=
                intmax_eval_operand(2);
        else // signedness == 0 // i.e. unsigned.
            *valreg =
                uintmax_eval_operand(0) <=
                uintmax_eval_operand(2);
    }
    else if( theRule == pp_eqops_eq )
    {
        *valreg =
            uintmax_eval_operand(0) ==
            uintmax_eval_operand(2);
        signedness =
            signedness_of_operand(0) &
            signedness_of_operand(2);
    }
    else if( theRule == pp_eqops_ne )
    {
        *valreg =
            uintmax_eval_operand(0) !=
            uintmax_eval_operand(2);
        signedness =
            signedness_of_operand(0) &
            signedness_of_operand(2);
    }
    else if( theRule == pp_bitand_bitand )
    {
        *valreg =
            uintmax_eval_operand(0) &
            uintmax_eval_operand(2);
        signedness =
            signedness_of_operand(0) &
            signedness_of_operand(2);
    }
    else if( theRule == pp_bitxor_bitxor )
    {
        *valreg =
            uintmax_eval_operand(0) ^
            uintmax_eval_operand(2);
        signedness =
            signedness_of_operand(0) &
            signedness_of_operand(2);
    }
    else if( theRule == pp_bitor_bitor )
    {
        *valreg =
            uintmax_eval_operand(0) |
            uintmax_eval_operand(2);
        signedness =
            signedness_of_operand(0) &
            signedness_of_operand(2);
    }
    else if( theRule == pp_logicand_logicand )
    {
        // 2026-05-15:
        // Supposedly no need for short-circuit evaluation in this context.
        // So not complicating it for now.
        // 2026-07-03: did check that type is always signed.
        *valreg =
            uintmax_eval_operand(0) &&
            uintmax_eval_operand(2);
        signedness = 1;
    }
    else if( theRule == pp_logicor_logicor )
    {
        *valreg =
            uintmax_eval_operand(0) ||
            uintmax_eval_operand(2);
        signedness = 1;
    }
    else if( theRule == pp_tenary_tenary )
    {
        *valreg =
            uintmax_eval_operand(0) ?
            uintmax_eval_operand(2) :
            uintmax_eval_operand(4) ;
        signedness =
            uintmax_eval_operand(0) ?
            signedness_of_operand(2) :
            signedness_of_operand(4);
    }
    else if( theRule == pp_exprlist_exprlist )
    {
        *valreg = uintmax_eval_operand(2);
        signedness = signedness_of_operand(2);
    }

    if( sp->body == expr )
    {
        *ret = *valreg;
        while( sp != &rs_anch )
        {
            bp = sp;
            sp = sp->ret;
            free(bp);
        }
        s2data_unmap((s2data_t *)sp->body->value);
        return signedness != 0;
    }


    bp = sp;
    sp = sp->ret;
    free(bp);
    signedness_of_operand(sp->operand_index) = signedness;
    sp->operand_index ++;
    goto start_eval_1term;
}

void CtrlLine_ArgCollect(
    s2list_t *atoks,
    cpptu_t *restrict ctx_tu,
    void *restrict ctx_shifter,
    token_shifter_t shifter)
{
    bool space_delimit_next_token = false;
    lex_token_t *tok;
    RegexLexContext *shifter_rope = ctx_shifter;
    assert( shifter == (token_shifter_t)RegexLexFromRope_Shift );

    do {
        tok = look_ahead_for_genuine_newline(shifter_rope) ?
            NULL : // encountered a newline, the control line is terminated.
            shifter(ctx_shifter);
        if( !tok ) break;

        if( tok->completion == langlex_comment )
        {
            // 2026-07-21:
            // An essential mid-processing, as comments are
            // considered whitespaces before such significance
            // is lost during syntax parsing phase.
            space_delimit_next_token = true;
            if( strncmp(s2data_weakmap(tok->str), "//", 2) == 0 )
                ctx_tu->ctx_shifter.offsub -= 1; // save 1 'genuine' newline.
            s2obj_release(tok->pobj);
            continue;
        }
        else if( space_delimit_next_token )
        {
            // 2026-07-21:
            // This assertion is so that in case other non-flag attrs
            // are added in the future.
            assert( tok->attrs == 0 || tok->attrs == 1 );

            tok->attrs |= TOKATTR_BLANKDELIM;
            space_delimit_next_token = false;
        }
        // In effect, the above clauses implements comment stripping.

        s2list_push(atoks, tok->pobj, s2_setter_gave);
    } while( tok );
}

int cppEvaluateCtrlExpr(
    cpptu_t *restrict ctx_tu,
    void *restrict ctx_shifter,
    token_shifter_t shifter)
{
    // Directive was `if` or `elif`, handle the controlling expression.

    //
    // 1. Collect the tokens on a logical line into a SafeTypes2 list,

    int i;
    uintmax_t evalres;
    lalr_stack_t *parsed;

    lex_token_t *tok;

    // source input for building pp-ctrl-expr ast.
    // identifiers are collapsed into literals.
    s2list_t *astsource = s2list_create();

    // the logical input line.
    //- s2list_t *logicline; // 2026-07-24: logical line collected by caller.

    // the evaluation line for macro expansions.
    s2list_t *evalline = s2list_create();

    struct cppMacroExpandShifter ppeval = {
        .flags = MACEXP_FLAG_EVALCTX_CTRLLINE,
        .pushlist = evalline,
        .ctx_tu = ctx_tu,
    };

    assert( shifter == (token_shifter_t)shift_from_s2list );
    ppeval.coldlist = ctx_shifter;
    ppeval.coldlist_shifter = (token_shifter_t)shift_from_s2list;

    //
    // 2. macro-expand it,

    do {
        tok = cppTokenJet(&ppeval);

        if( PPTokGraduate(tok) != 0 )
        {
            ccDiagnoseError(ctx_tu, "Invalid Token", spelling_and_site(tok));
            s2obj_release(astsource->pobj);
            s2obj_release(evalline->pobj);
            return -1;
        }

        if( !tok ) break;

        if( tok->completion == langlex_identifier )
        {
            if( strcmp(s2data_weakmap(tok->str), "true") == 0 )
            {
                tok->completion = langlex_declit;
                s2data_trunc(tok->str, 1);
                *(char *)s2data_weakmap(tok->str) = '1';
            }
            else
            {
                tok->completion = langlex_octlit;
                s2data_trunc(tok->str, 1);
                *(char *)s2data_weakmap(tok->str) = '0';
            }
        }

        s2list_push(astsource, tok->pobj, s2_setter_gave);
    } while( true );

    //
    // 3. invoke PP expression handler on it.

    i = lalr_parse(&parsed, GRAMMAR_RULES, NULL, NS_RULES,
                   (token_shifter_t)shift_from_s2list, astsource);

    if( i != 0 )
    {
        ccDiagnoseError(ctx_tu, "Parsing Error", " parser return: %d.", i);
        s2obj_release(parsed->pobj);
        s2obj_release(astsource->pobj);
        s2obj_release(evalline->pobj);
        return -1;
    }

    i = intmax_eval_start(
        &evalres, parsed->bottom->production);

    s2obj_release(parsed->pobj);
    s2obj_release(astsource->pobj);
    s2obj_release(evalline->pobj);

    return i >= 0 ? evalres != 0 : i;
}
