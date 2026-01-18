/* DannyNiu/NJF, 2025-11-22. Public Domain. */

#include <librematch.h>
#include "langlex-cxing.h"

#define LEX_ENUM(e) { .enumerant = e, .str = #e },
const struct lex_enum_strtab langlex_token_strtab[] = {
#include "langlex-cxing-def.inc"
    { 0, NULL },
};
#undef LEX_ENUM

#define Identifier "[_[:alpha:]][_[:alnum:]]*"
#define OctDigitSeq "[0-7]+"
#define DecDigitSeq "[0-9]+"
#define HexDigitSeq "[0-9a-f]+"

#define DecFracLit "(("DecDigitSeq")?[.]"DecDigitSeq"|"DecDigitSeq"[.])"
#define HexFracLit "(("HexDigitSeq")?[.]"HexDigitSeq"|"HexDigitSeq"[.])"

#define IntSuffixOpt "u?"

#define EscSeq "\\\\([\\\"\'?abfnrtv]|[0-7]{1,3}|o\\{[0-7]+\\}|x[0-9A-Fa-f]+|x\\{[0-9A-Fa-f]+\\})"

lex_elem_t LexElems[] = {
    { .pattern = "\'([^\\\']|"EscSeq")*\'",
      .cflags = LIBREG_EXTENDED, .completion = langlex_charlit },
    { .pattern = "\"([^\\\"]|"EscSeq")*\"",
      .cflags = LIBREG_EXTENDED, .completion = langlex_strlit },
    { .pattern = "[\\](\"[^\"]*\"|\'[^\']\')",
      .cflags = LIBREG_EXTENDED, .completion = langlex_rawlit },

    { .pattern =
      "true|false|null|"
      "return|break|continue|and|_Then|or|_Fallback|decl|"
      "if|else|elif|while|do|for|subr|method|this|_Include|extern",
      .cflags = LIBREG_EXTENDED, .completion = langlex_keyword },

    { .pattern = Identifier, .cflags = LIBREG_EXTENDED,
      .completion = langlex_identifier },

    { .pattern = "[1-9][0-9]*"IntSuffixOpt,
      .cflags = LIBREG_ICASE|LIBREG_EXTENDED,
      .completion = langlex_declit },

    { .pattern = "0[0-7]*"IntSuffixOpt,
      .cflags = LIBREG_ICASE|LIBREG_EXTENDED,
      .completion = langlex_octlit },

    { .pattern = "0x"HexDigitSeq IntSuffixOpt,
      .cflags = LIBREG_ICASE|LIBREG_EXTENDED,
      .completion = langlex_hexlit },

    { .pattern = DecFracLit"(e[-+]"DecDigitSeq")?",
      .cflags = LIBREG_ICASE|LIBREG_EXTENDED,
      .completion = langlex_decfplit },

    { .pattern = DecDigitSeq"(e[-+]"DecDigitSeq")",
      .cflags = LIBREG_ICASE|LIBREG_EXTENDED,
      .completion = langlex_decfplit },

    { .pattern = "0x"HexFracLit"p[-+]"HexDigitSeq,
      .cflags = LIBREG_ICASE|LIBREG_EXTENDED,
      .completion = langlex_hexfplit },

    { .pattern = "0x"HexDigitSeq"p[-+]"HexDigitSeq,
      .cflags = LIBREG_ICASE|LIBREG_EXTENDED,
      .completion = langlex_hexfplit },

    { .pattern =
      "(<<|>>>?|[-+*/%&^|])=|=[?]|"
      "[+][+]|--|<<|>>>?|[<>=!]=|&&|[|][|]|[?][?]|"
      "[[.].][.[.](){}.&*+[.-.]~!/%<>\\^|?:;=,#]",
      .cflags = LIBREG_EXTENDED, .completion = langlex_punct },

    {},
};
