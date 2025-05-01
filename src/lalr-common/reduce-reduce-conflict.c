/* DannyNiu/NJF, 2025-03-29. Public Domain. */

#define dcc_lalr_defining_grammar
#include "reduce-reduce-conflict.h"
#include "../langlex-c/langlex-c.h"

strvec_t *ns_rules_test;

static void *goal(lalr_rule_params)
{
    int32_t production = hRule(".");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_prod,   .value = "expression" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *expression_degenerate(lalr_rule_params)
{
    int32_t production = hRule("expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_prod,   .value = "additive-expression" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *expression_list(lalr_rule_params)
{
    int32_t production = hRule("expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_prod,   .value = "expression" },
        { symtype_stoken, .value = "," },
        { symtype_prod,   .value = "additive-expression" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *addexpr_degenerate(lalr_rule_params)
{
    int32_t production = hRule("additive-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_prod,   .value = "multiplicative-expression" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *addexpr_addition(lalr_rule_params)
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

static void *addexpr_subtraction(lalr_rule_params)
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

static void *mulexpr_degenerate(lalr_rule_params)
{
    int32_t production = hRule("multiplicative-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_prod,   .value = "unary-expression" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *mulexpr_multiplication(lalr_rule_params)
{
    int32_t production = hRule("multiplicative-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_prod,   .value = "multiplicative-expression" },
        { symtype_stoken, .value = "*" },
        { symtype_prod,   .value = "unary-expression" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *mulexpr_division(lalr_rule_params)
{
    int32_t production = hRule("multiplicative-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_prod,   .value = "multiplicative-expression" },
        { symtype_stoken, .value = "/" },
        { symtype_prod,   .value = "unary-expression" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *unaryexpr_degenerate(lalr_rule_params)
{
    int32_t production = hRule("unary-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_prod,   .value = "primary-expression" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *unaryexpr_plus(lalr_rule_params)
{
    int32_t production = hRule("unary-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_stoken, .value = "+" },
        { symtype_prod,   .value = "unary-expression" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *unaryexpr_minus(lalr_rule_params)
{
    int32_t production = hRule("unary-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_stoken, .value = "-" },
        { symtype_prod,   .value = "unary-expression" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *primaryexpr_paren(lalr_rule_params)
{
    int32_t production = hRule("primary-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_stoken, .value = "(" },
        { symtype_prod,   .value = "expression" },
        { symtype_stoken, .value = ")" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *primaryexpr_identexpr(lalr_rule_params)
{
    int32_t production = hRule("primary-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_prod,   .value = "identified-expression" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *arguments_1arg(lalr_rule_params)
{
    int32_t production = hRule("arguments");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_prod,   .value = "additive-expression" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *arguments_morearg(lalr_rule_params)
{
    int32_t production = hRule("arguments");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_prod,   .value = "arguments" },
        { symtype_stoken, .value = "," },
        { symtype_prod,   .value = "additive-expression" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *identexpr_func(lalr_rule_params)
{
    int32_t production = hRule("arguments");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_vtoken, .vtype = langlex_identifier },
        { symtype_stoken, .value = "(" },
        { symtype_prod,   .value = "arguments" },
        { symtype_stoken, .value = ")" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *identexpr_subr(lalr_rule_params)
{
    int32_t production = hRule("arguments");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_vtoken, .vtype = langlex_identifier },
        { symtype_stoken, .value = "(" },
        { symtype_stoken, .value = ")" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *identexpr_ident(lalr_rule_params)
{
    int32_t production = hRule("arguments");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_vtoken, .vtype = langlex_identifier },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

lalr_rule_t test_grammar_rules[] = {
    goal,

    expression_degenerate, // 1
    expression_list,

    addexpr_degenerate,
    addexpr_addition,
    addexpr_subtraction, // 5

    mulexpr_degenerate,
    mulexpr_multiplication,
    mulexpr_division,

    unaryexpr_degenerate,
    unaryexpr_plus, // 10
    unaryexpr_minus,

    primaryexpr_paren,
    primaryexpr_identexpr,

    arguments_1arg,
    arguments_morearg, // 15

    identexpr_func,
    identexpr_subr,
    identexpr_ident,

    NULL,
};
