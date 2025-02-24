/* DannyNiu/NJF, 2024-12-29. Public Domain. */

#include "langlex.h"
#include <ctype.h>

typedef struct lex_getc_checking {
    lex_getc_base_t base;
    FILE *fp;
} lex_getc_checking_t;

static int lex_getc(lex_getc_checking_t *ctx)
{
    return fgetc(ctx->fp);
}

static int lex_ungetc(int c, lex_getc_checking_t *ctx)
{
    return ungetc(c, ctx->fp);
}

static lex_getc_checking_t fc = {
    .base.getc = (lex_getc_func_t)lex_getc,
    .base.ungetc = (lex_ungetc_func_t)lex_ungetc,
};

int main(int argc, char *argv[])
{
    lex_token_t *token;
    int32_t lineno, column;
    int c;
    int i;

    fc.fp = fopen(argv[1], "r");
    lineno = 1;
    column = 0;
    (void)argc;

    while( !feof(fc.fp) )
    {
        if( (c = fc.base.getc(&fc.base)) == EOF ) break;
        if( c == '\n' )
        {
            lineno ++;
            column = 1;
        }
        else if( isspace(c) )
        {
            column ++;
        }

        if( !isspace(c) )
        {
            fc.base.ungetc(c, &fc.base);
            token = lex_token_parse(langlex_fsm, &fc.base);

            if( !token )
                token = lex_token_match(
                    langlex_puncts, langlex_punct, &fc.base);

            if( !token )
            {
                fprintf(stderr, "Encountered malformed token!\n");
                return EXIT_FAILURE;
            }

            token->lineno = lineno;
            token->column = column;
            column += s2data_len(token->str);

            for(i=0; langlex_token_strtab[i].str; i++)
                if(  langlex_token_strtab[i].enumerant ==
                     token->completion )
                    break;

            printf("token %-15s at %d,%d\t""of type %s\n",
                   (char *)s2data_weakmap(token->str),
                   token->lineno, token->column,
                   langlex_token_strtab[i].str);
        }
    }

    return EXIT_SUCCESS;
}
