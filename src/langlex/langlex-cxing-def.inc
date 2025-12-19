/* DannyNiu/NJF, 2025-11-22. Public Domain. */

// 0 is reserved for `lex_token_complete` as end-of-list.
LEX_ENUM(langlex_keyword)
LEX_ENUM(langlex_identifier)

LEX_ENUM(langlex_declit)
LEX_ENUM(langlex_octlit)
LEX_ENUM(langlex_hexlit)
LEX_ENUM(langlex_r64lit)
LEX_ENUM(langlex_decfplit)
LEX_ENUM(langlex_hexfplit)

LEX_ENUM(langlex_charlit)
LEX_ENUM(langlex_strlit)
LEX_ENUM(langlex_rawlit)
LEX_ENUM(langlex_punct)
