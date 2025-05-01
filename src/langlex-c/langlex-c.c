/* DannyNiu/NJF, 2024-12-26. Public Domain. */

#include "langlex-c.h"

#define LEX_ENUM(e) { .enumerant = e, .str = #e },
const struct lex_enum_strtab langlex_token_strtab[] = {
#include "langlex-c-def.inc"
    { 0, NULL },
};
#undef LEX_ENUM

const struct lex_fsm_trans langlex_fsm[] =
{
    // identifiers
    { .now = lex_token_start,
      .expect =
      "abcdefghijklmnopqrstvwxyz"  // exclude and 'u',
      "ABCDEFGHIJK_MNOPQRSTVWXYZ", // exclude 'L', 'U', and digits.
      .next = langlex_identifier },

    { .now = langlex_identifier,
      .expect = "_0123456789"
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
      .next = langlex_identifier },

    // expecting decimal integer or floating point.
    { .now = lex_token_start,
      .expect = "123456789", // excludes "0"
      .next = langlex_int_dec },

    { .now = langlex_int_dec,
      .expect = "0123456789\'",
      .next = langlex_int_dec },

    { .now = langlex_period,
      .expect = ".",
      .next = langlex_period2 },

    { .now = langlex_period2,
      .expect = ".",
      .next = langlex_punct },

    // this is a decimal floating-point constant literal.
    { .now = langlex_int_dec,
      .expect = ".",
      .next = langlex_fp_dec },

    { .now = langlex_int_prefixed, // fractions with leading 0.
      .expect = ".",
      .next = langlex_fp_dec },

    { .now = lex_token_start,
      .expect = ".", // dec fp literal with a leading radix point.
      .next = langlex_period },

    { .now = langlex_fp_dec,
      .expect = "0123456789\'",
      .next = langlex_fp_dec },

    { .now = langlex_period,
      .expect = "0123456789\'",
      .next = langlex_fp_dec },

    { .now = langlex_int_dec,
      .expect = "eE",
      .next = langlex_fp_dec_exp_sign_opt },

    { .now = langlex_fp_dec,
      .expect = "eE",
      .next = langlex_fp_dec_exp_sign_opt },

    { .now = langlex_fp_dec_exp_sign_opt,
      .expect = "+-0123456789",
      .next = langlex_fp_dec_exp },

    { .now = langlex_fp_dec_exp,
      .expect = "0123456789\'",
      .next = langlex_fp_dec_exp },

    // 2024-12-26: no support for complex or decimal floating-points for now.
    { .now = langlex_fp_dec_exp,
      .expect = "fl",
      .next = langlex_fp_dec_suffixed },

    // binary, octal or hex numbers
    { .now = lex_token_start,
      .expect = "0",
      .next = langlex_int_prefixed },

    // octal integer
    { .now = langlex_int_prefixed,
      .expect = "oO",
      .next = langlex_int_oct },

    { .now = langlex_int_prefixed,
      .expect = "01234567\'",
      .next = langlex_int_oct },

    { .now = langlex_int_oct,
      .expect = "01234567\'",
      .next = langlex_int_oct },

    // binary number.
    { .now = langlex_int_prefixed,
      .expect = "bB",
      .next = langlex_int_bin },

    { .now = langlex_int_bin,
      .expect = "01\'",
      .next = langlex_int_bin },

    // hex number.
    { .now = langlex_int_prefixed,
      .expect = "xX",
      .next = langlex_int_hex },

    { .now = langlex_int_hex,
      .expect = ".", // hex fp literal with a leading radix point.
      .next = langlex_fp_hex },

    { .now = langlex_int_hex,
      .expect = "0123456789abcdefABCDEF\'",
      .next = langlex_int_hex },

    // this is a hex floating-point constant literal.
    { .now = langlex_int_hex,
      .expect = ".",
      .next = langlex_fp_hex },

    { .now = langlex_fp_hex,
      .expect = "0123456789abcdefABCDEF\'",
      .next = langlex_fp_hex },

    { .now = langlex_int_hex,
      .expect = "pP",
      .next = langlex_fp_hex_exp_sign_opt },

    { .now = langlex_fp_hex,
      .expect = "pP",
      .next = langlex_fp_hex_exp_sign_opt },

    { .now = langlex_fp_hex_exp_sign_opt,
      .expect = "+-0123456789abcdefABCDEF",
      .next = langlex_fp_hex_exp },

    { .now = langlex_fp_hex_exp,
      .expect = "0123456789abcdefABCDEF\'",
      .next = langlex_fp_hex_exp },

    // 2024-12-26: no support for complex or decimal floating-points for now.
    { .now = langlex_fp_hex_exp,
      .expect = "fl",
      .next = langlex_fp_hex_suffixed },

    // character and string literals, and
    // identifiers that share prefix with them.
    { .now = lex_token_start,
      .expect = "\"",
      .next = langlex_strlit },

    { .now = lex_token_start,
      .expect = "LU",
      .next = langlex_letter_prefix },

    { .now = lex_token_start,
      .expect = "u",
      .next = langlex_letter_prefix_lower_u },

    { .now = langlex_letter_prefix_lower_u,
      .expect = "8",
      .next = langlex_letters_u8 },

    { .now = langlex_letter_prefix,
      .expect = "_0123456789"
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
      .next = langlex_identifier },

    { .now = langlex_letter_prefix_lower_u,
      .expect = "_0123456789"
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
      .next = langlex_identifier },

    { .now = langlex_letters_u8,
      .expect = "_0123456789"
      "abcdefghijklmnopqrstuvwxyz"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ",
      .next = langlex_identifier },

    // string literals
    { .now = langlex_letter_prefix,
      .expect = "\"",
      .next = langlex_strlit },

    { .now = langlex_letter_prefix_lower_u,
      .expect = "\"",
      .next = langlex_strlit },

    { .now = langlex_letters_u8,
      .expect = "\"",
      .next = langlex_strlit },

    { .now = langlex_strlit,
      .expect = "\"\\\n", .flags = lex_expect_compl,
      .next = langlex_strlit },

    { .now = langlex_strlit,
      .expect = "\\",
      .next = langlex_strlit_escape },

    { .now = langlex_strlit_escape,
      // allow any char to complete an escape sequence in syntax.
      // only handle them in semantic.
      .expect = "", .flags = lex_expect_compl,
      .next = langlex_strlit },

    { .now = langlex_strlit,
      .expect = "\"",
      .next = lex_token_complete },

    // character literals
    { .now = langlex_letter_prefix,
      .expect = "\'",
      .next = langlex_charlit },

    { .now = langlex_letters_u8,
      .expect = "\'",
      .next = langlex_charlit },

    { .now = langlex_charlit,
      .expect = "\'\\\n", .flags = lex_expect_compl,
      .next = langlex_charlit },

    { .now = langlex_charlit,
      .expect = "\\",
      .next = langlex_charlit_escape },

    { .now = langlex_charlit_escape,
      // allow any char to complete an escape sequence in syntax.
      // only handle them in semantic.
      .expect = "", .flags = lex_expect_compl,
      .next = langlex_charlit },

    { .now = langlex_charlit,
      .expect = "\'",
      .next = lex_token_complete },

    // End-Of-List.
    { .now = lex_token_complete },
};

const char *langlex_puncts[] = {
    "[", "]", "(", ")", "{", "}", ".", "->",
    "++", "--", "&", "*", "+", "-", "~", "!",
    "/", "%", "<<", ">>", "<", ">", "<=", ">=", "==", "!=",
    "^", "|", "&&", "||",
    "?", ":", "::", ";", "...",
    "=", "*=", "/=", "%=", "+=", "-=", "<<=", ">>=", "&=", "^=", "|=",
    ",", "#", "##",
    "<:", ":>", "<%", "%>", "%:", "%:%:",
    NULL
};

