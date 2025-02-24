/* DannyNiu/NJF, 2024-12-30. Public Domain. */

#define dcc_lalr_defining_grammar
#include "fpcalc-grammar.h"
#include "../lex/langlex.h"

strvec_t *ns_rules_fpcalc;

static void *fpcalc_goal(lalr_rule_params)
{
    int32_t production = hRule(".");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_prod,   .value = "assignment-expression" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *assignexpr_degenerate(lalr_rule_params)
{
    int32_t production = hRule("assignment-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_prod,   .value = "additive-expression" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *assignexpr_assignment(lalr_rule_params)
{
    int32_t production = hRule("assignment-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_prod,   .value = "identified-expression" },
        { symtype_stoken, .value = "=" },
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
        { symtype_prod  , .value = "primary-expression" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *unaryexpr_positive(lalr_rule_params)
{
    int32_t production = hRule("unary-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_stoken, .value = "+" },
        { symtype_prod  , .value = "primary-expression" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *unaryexpr_negative(lalr_rule_params)
{
    int32_t production = hRule("unary-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_stoken, .value = "-" },
        { symtype_prod  , .value = "primary-expression" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *primary_identexpr(lalr_rule_params)
{
    int32_t production = hRule("primary-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_prod  , .value = "identified-expression" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *primary_number_int(lalr_rule_params)
{
    int32_t production = hRule("primary-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_vtoken, .vtype = langlex_int_dec },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *primary_number_fract(lalr_rule_params)
{
    int32_t production = hRule("primary-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_vtoken, .vtype = langlex_fp_dec },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *primary_number_fp(lalr_rule_params)
{
    int32_t production = hRule("primary-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_vtoken, .vtype = langlex_fp_dec_exp },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *primary_number_zero_1digit(lalr_rule_params)
{
    int32_t production = hRule("primary-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_vtoken, .vtype = langlex_int_prefixed },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *primary_paren(lalr_rule_params)
{
    int32_t production = hRule("primary-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_stoken, .value = "(" },
        { symtype_prod  , .value = "additive-expression" },
        { symtype_stoken, .value = ")" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *addexprlist_base(lalr_rule_params)
{
    int32_t production = hRule("additive-expression-list");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_prod,   .value = "additive-expression" },
        { symtype_stoken, .value = "," },
        { symtype_prod,   .value = "additive-expression" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *addexprlist_recursion(lalr_rule_params)
{
    int32_t production = hRule("additive-expression-list");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_prod,   .value = "additive-expression-list" },
        { symtype_stoken, .value = "," },
        { symtype_prod,   .value = "additive-expression" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *identexpr_label(lalr_rule_params)
{
    int32_t production = hRule("identified-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_vtoken, .vtype = langlex_identifier },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *identexpr_function_noparam(lalr_rule_params)
{
    int32_t production = hRule("identified-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_vtoken, .vtype = langlex_identifier },
        { symtype_stoken, .value = "(" },
        { symtype_stoken, .value = ")" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *identexpr_function_1param(lalr_rule_params)
{
    int32_t production = hRule("identified-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_vtoken, .vtype = langlex_identifier },
        { symtype_stoken, .value = "(" },
        { symtype_prod  , .value = "additive-expression" },
        { symtype_stoken, .value = ")" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

static void *identexpr_function_multiparam(lalr_rule_params)
{
    int32_t production = hRule("identified-expression");
    static lalr_rule_symbol_t symbolseq[] = {
        { symtype_vtoken, .vtype = langlex_identifier },
        { symtype_stoken, .value = "(" },
        { symtype_prod  , .value = "additive-expression-list" },
        { symtype_stoken, .value = ")" },
        {0},
    };

    (void)ctx;
    return lalr_rule_actions_generic(lalr_rule_gen_args);
}

lalr_rule_t fpcalc_grammar_rules[] = {
    fpcalc_goal,

    assignexpr_degenerate, // 1
    assignexpr_assignment,

    addexpr_degenerate,
    addexpr_addition,
    addexpr_subtraction, // 5

    mulexpr_degenerate,
    mulexpr_multiplication,
    mulexpr_division,

    unaryexpr_degenerate,
    unaryexpr_positive, // 10
    unaryexpr_negative,

    primary_identexpr,
    primary_number_int,
    primary_number_fract,
    primary_number_fp, // 15
    primary_number_zero_1digit,
    primary_paren,

    addexprlist_base,
    addexprlist_recursion,
    identexpr_label, // 20
    identexpr_function_noparam,
    identexpr_function_1param,
    identexpr_function_multiparam,

    NULL,
};
