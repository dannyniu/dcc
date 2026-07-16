/* DannyNiu/NJF, 2024-12-29. Public Domain. */

// 0 is reserved for `lex_token_complete` as end-of-list.
LEX_ENUM(langlex_identifier)

LEX_ENUM(langlex_binlit)
LEX_ENUM(langlex_declit)
LEX_ENUM(langlex_octlit)
LEX_ENUM(langlex_hexlit)
LEX_ENUM(langlex_decfplit)
LEX_ENUM(langlex_hexfplit)

LEX_ENUM(langlex_ppnum)

LEX_ENUM(langlex_charlit)
LEX_ENUM(langlex_strlit)
LEX_ENUM(langlex_punct)

// 2026-07-06:
// Because system headers (i.e. ones encosed in angle brackets) are lexed
// differently than user headers (i.e. ones encosed in double quotes),
// we need a catch-all completion to accept characters (bytes actually)
// that could be valid _or_ invalid when appearing elsewhere.
// This completion is automatically rejected by the C parser for there being
// no grammar rule accepting them.
LEX_ENUM(langlex_invalid)
