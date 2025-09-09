/* DannyNiu/NJF, 2024-12-29. Public Domain. */

#include "langlex-c.h"

int logger(void *ctx, const char *msg)
{
    (void)ctx;
    fprintf(stderr, "%s\n", msg);
    return 0;
}

static lex_getc_fp_t fc;
static source_rope_t *rope;
static RegexLexContext lexer;

int main(int argc, char *argv[])
{
    lex_token_t *token;
    int c;
    int i;

    for(i=0; CLexElems[i].pattern; i++)
    {
        c = libregcomp(
            &CLexElems[i].preg, CLexElems[i].pattern, CLexElems[i].cflags);
    }
    lexer.regices = CLexElems;
    lexer.logger_base = (struct logging_ctxbase){
        .logger = (logger_func)logger,
    };

    lex_getc_init_from_fp(&fc, fopen(argv[1], "r"));
    rope = CreateRopeFromGetc(&fc.base, RopeCreatFlag_LineConti);
    RegexLexFromRope_Init(&lexer, rope);
    (void)argc;

    while( true )
    {
        token = RegexLexFromRope_Shift(&lexer);
        if( !token ) break;

        for(i=0; langlex_token_strtab[i].str; i++)
            if(  langlex_token_strtab[i].enumerant ==
                    token->completion )
                break;

        printf("token %-15s at %d,%d\t""of type %s\n",
                (char *)s2data_weakmap(token->str),
                token->lineno, token->column,
                langlex_token_strtab[i].str);
    }

    return EXIT_SUCCESS;
}
