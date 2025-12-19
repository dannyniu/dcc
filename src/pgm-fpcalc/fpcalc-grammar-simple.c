/* DannyNiu/NJF, 2024-12-30. Public Domain. */

#define dcc_lalr_defining_grammar
#include "fpcalc-grammar.h"
#include "../langlex/langlex-c.h"

strvec_t *ns_rules_fpcalc = NULL;

void *fpcalc_goal(lalr_rule_params)
{
    int32_t production = hRule(".");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_prod,   .value = "additive-expression" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

void *addexpr_degenerate(lalr_rule_params)
{
    int32_t production = hRule("additive-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_prod,   .value = "multiplicative-expression" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

void *addexpr_addition(lalr_rule_params)
{
    int32_t production = hRule("additive-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_prod,   .value = "additive-expression" },
        { symtype_stoken, .value = "+" },
        { symtype_prod,   .value = "multiplicative-expression" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

void *addexpr_subtraction(lalr_rule_params)
{
    int32_t production = hRule("additive-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_prod,   .value = "additive-expression" },
        { symtype_stoken, .value = "-" },
        { symtype_prod,   .value = "multiplicative-expression" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

void *mulexpr_degenerate(lalr_rule_params)
{
    int32_t production = hRule("multiplicative-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_prod,   .value = "primary-expression" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

void *mulexpr_multiplication(lalr_rule_params)
{
    int32_t production = hRule("multiplicative-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_prod,   .value = "multiplicative-expression" },
        { symtype_stoken, .value = "*" },
        { symtype_prod,   .value = "primary-expression" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

void *mulexpr_division(lalr_rule_params)
{
    int32_t production = hRule("multiplicative-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_prod,   .value = "multiplicative-expression" },
        { symtype_stoken, .value = "/" },
        { symtype_prod,   .value = "primary-expression" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

void *primary_paren(lalr_rule_params)
{
    int32_t production = hRule("primary-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_stoken, .value = "(" },
        { symtype_prod,   .value = "additive-expression" },
        { symtype_stoken, .value = ")" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

void *primary_label(lalr_rule_params)
{
    int32_t production = hRule("primary-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_vtoken, .vtype = langlex_identifier },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

void *primary_number(lalr_rule_params)
{
    int32_t production = hRule("primary-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_vtoken, .vtype = langlex_ppnum },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

lalr_rule_t fpcalc_grammar_rules[] = {
    fpcalc_goal,

    addexpr_degenerate,
    addexpr_addition,
    addexpr_subtraction,

    mulexpr_degenerate,
    mulexpr_multiplication,
    mulexpr_division,

    primary_paren,
    primary_label,
    primary_number,

    NULL,
};
