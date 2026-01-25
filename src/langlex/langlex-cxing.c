/* DannyNiu/NJF, 2025-11-22. Public Domain. */

#include <librematch.h>
#include "langlex-cxing.h"
#include "../pgm-cxing/expr.h"

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

#define EscSeq "\\\\([\\\"\'?abefnrtv]|[0-7]{1,3}|o\\{[0-7]+\\}|x[0-9A-Fa-f]+|x\\{[0-9A-Fa-f]+\\})"

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

    { .pattern = "/\\*([!*]|\\*[^/])\\*/",
      .cflags = LIBREG_EXTENDED,
      .completion = langlex_comment },

    { .pattern = "(#|//)[^\n]\n",
      .cflags = LIBREG_EXTENDED,
      .completion = langlex_comment },

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

lex_token_t *PP_StripComments_Cxing(pp_strip_comments_ctx *ctx)
{
    lex_token_t *ret;
    while( true )
    {
        ret = RegexLexFromRope_Shift(&ctx->lexer);
        if( !ret ) return NULL;

        if( ret->completion == langlex_comment )
        {
            s2obj_release(ret->pobj);
            continue;
        }

        return ret;
    }
}

lex_token_t *PP_StrLitConcat_Cxing(pp_strlit_concat_ctx *ctx)
{
    lex_token_t *ret;
    lex_token_t *cooked = NULL;

    if( ctx->buffer )
    {
        ret = ctx->buffer;
        ctx->buffer = NULL;
        return ret;
    }

    ret = PP_StripComments_Cxing(&ctx->cmtstrip);
    if( !ret ) return NULL;

    while( ret->completion == langlex_strlit ||
           ret->completion == langlex_rawlit )
    {
        if( !cooked )
        {
            cooked = lex_token_create(); // TODO 2026-01-25: handle error.
            cooked->lineno = ret->lineno;
            cooked->column = ret->column;
            cooked->completion = langlex_str_cooked;
        }

        if( ret->completion == langlex_strlit )
            StrLit_Unquote(cooked->str, ret->str);
        else StrLit_CookRaw(cooked->str, ret->str);

        s2obj_release(ret->pobj);

        ret = PP_StripComments_Cxing(&ctx->cmtstrip);
        if( !ret ) break;
    }

    if( cooked )
    {
        ctx->buffer = ret;
        return cooked;
    }
    else return ret;
}

void CxingTokenizerInit(cxing_tokenizer *shifter, source_rope_t *rope)
{
    memset(shifter, 0, sizeof *shifter);
    shifter->cmtstrip.lexer.regices = LexElems;
    RegexLexFromRope_Init(&shifter->cmtstrip.lexer, rope);
}
