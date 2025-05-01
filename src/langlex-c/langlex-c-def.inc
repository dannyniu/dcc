/* DannyNiu/NJF, 2024-12-29. Public Domain. */

// 0 is reserved for `lex_token_complete` as end-of-list.
LEX_ENUM(langlex_identifier)
LEX_ENUM(langlex_letter_prefix)
LEX_ENUM(langlex_letter_prefix_lower_u)
LEX_ENUM(langlex_letters_u8)

LEX_ENUM(langlex_int_dec)
LEX_ENUM(langlex_int_prefixed)
LEX_ENUM(langlex_int_oct)
LEX_ENUM(langlex_int_bin)
LEX_ENUM(langlex_int_hex)

LEX_ENUM(langlex_period)
LEX_ENUM(langlex_period2)

LEX_ENUM(langlex_fp_dec)
LEX_ENUM(langlex_fp_dec_exp_sign_opt)
LEX_ENUM(langlex_fp_dec_exp)
LEX_ENUM(langlex_fp_dec_suffixed)
LEX_ENUM(langlex_fp_hex)
LEX_ENUM(langlex_fp_hex_exp_sign_opt)
LEX_ENUM(langlex_fp_hex_exp)
LEX_ENUM(langlex_fp_hex_suffixed)

LEX_ENUM(langlex_charlit)
LEX_ENUM(langlex_charlit_escape)
LEX_ENUM(langlex_charlit_escape_oct2rem)
LEX_ENUM(langlex_strlit)
LEX_ENUM(langlex_strlit_escape)
LEX_ENUM(langlex_punct)
