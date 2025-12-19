/* DannyNiu/NJF, 2024-12-26. Public Domain. */

#include <librematch.h>
#include "langlex-c.h"

#define LEX_ENUM(e) { .enumerant = e, .str = #e },
const struct lex_enum_strtab langlex_token_strtab[] = {
#include "langlex-c-def.inc"
    { 0, NULL },
};
#undef LEX_ENUM

#define Identifier "[_[:alpha:]][_[:alnum:]]*"
#define BinDigitSeq "[01]('?[01])*"
#define OctDigitSeq "[0-7]('?[0-7])*"
#define DecDigitSeq "[0-9]('?[0-9])*"
#define HexDigitSeq "[0-9a-z]('?[0-9a-z])*"

#define DecFracLit "(("DecDigitSeq")?[.]"DecDigitSeq"|"DecDigitSeq"[.])"
#define HexFracLit "(("HexDigitSeq")?[.]"HexDigitSeq"|"HexDigitSeq"[.])"

#define FpSuffixOpt "([fl][ij]?|[ij][fl]?)?"
#define IntSuffixOpt "(ull?|ll?u)?"

#define EscSeq "\\\\([\\\"\'?abfnrtv]|[0-7]{1,3}|o\\{[0-7]+\\}|x[0-9A-Fa-f]+|x\\{[0-9A-Fa-f]+\\})"

lex_elem_t CLexElems[] = {
    { .pattern = "(u8|[uUL])?\'([^\\\']|"EscSeq")*\'",
      .cflags = LIBREG_EXTENDED, .completion = langlex_charlit },
    { .pattern = "(u8|[uUL])?\"([^\\\"]|"EscSeq")*\"",
      .cflags = LIBREG_EXTENDED, .completion = langlex_strlit },

    { .pattern = Identifier, .cflags = LIBREG_EXTENDED,
      .completion = langlex_identifier },

    { .pattern = "\\.?[0-9]('?[_0-9a-zA-Z]|[eEpP][-+]|\\.)*",
      .cflags = LIBREG_EXTENDED,
      .completion = langlex_ppnum },

    { .pattern =
      "%:%:|\\.{3,3}|(<<|>>|[*/%+-&^|])=|->|"
      "[+][+]|--|<<|>>|[<>=!]=|&&|[|][|]|::|"
      "##|<:|:>|<%|%>|%:|"
      "[[.].][.[.](){}.&*+[.-.]~!/%<>\\^|?:;=,#]",
      .cflags = LIBREG_EXTENDED, .completion = langlex_punct },

    {},
};

lex_elem_t CNumLexElems[] = {
  { .pattern = "[1-9]('?[0-9])*"IntSuffixOpt,
    .cflags = LIBREG_ICASE|LIBREG_EXTENDED,
    .completion = langlex_declit },

  { .pattern = "(0('?[0-7])*|0o[0-7]('?[0-7])*)"IntSuffixOpt,
    .cflags = LIBREG_ICASE|LIBREG_EXTENDED,
    .completion = langlex_octlit },

  { .pattern = "0x"HexDigitSeq IntSuffixOpt,
    .cflags = LIBREG_ICASE|LIBREG_EXTENDED,
    .completion = langlex_hexlit },

  { .pattern = "0b"BinDigitSeq IntSuffixOpt,
    .cflags = LIBREG_ICASE|LIBREG_EXTENDED,
    .completion = langlex_binlit },

  { .pattern = DecFracLit"(e[-+]"DecDigitSeq")?"FpSuffixOpt,
    .cflags = LIBREG_ICASE|LIBREG_EXTENDED,
    .completion = langlex_decfplit },

  { .pattern = DecDigitSeq"(e[-+]"DecDigitSeq")"FpSuffixOpt,
    .cflags = LIBREG_ICASE|LIBREG_EXTENDED,
    .completion = langlex_decfplit },

  { .pattern = "0x"HexFracLit"p[-+]"HexDigitSeq FpSuffixOpt,
    .cflags = LIBREG_ICASE|LIBREG_EXTENDED,
    .completion = langlex_hexfplit },

  { .pattern = "0x"HexDigitSeq"p[-+]"HexDigitSeq FpSuffixOpt,
    .cflags = LIBREG_ICASE|LIBREG_EXTENDED,
    .completion = langlex_hexfplit },

  {},
};
