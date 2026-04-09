/* DannyNiu/NJF, 2026-01-10. Public Domain. */

#include "expr.h"

#if true
#if __has_include(<readline/readline.h>)
#include <readline/readline.h>

s2data_t *s2data_stdio_getline(FILE *fp)
{
    assert( fp == stdin );
    char *line = readline("input> ");
    if( !line ) return NULL;
    s2data_t *ret = s2data_from_str(line);
    (free)(line);
    return ret;
}

#else // There's no 'readline.h'
s2data_t *s2data_stdio_getline(FILE *fp)
{
    s2data_t *ret;
    int c;

    fprintf(stderr, "input> ");
    fflush(stderr);

    ret = s2data_create(0);
    if( !ret ) return NULL;

    while( (c = getc(fp)) >= 0 )
    {
        if( s2data_putc(ret, c) == -1 )
        {
            s2obj_release(ret->pobj);
            return NULL;
        }
        if( c == '\n' ) break;
    }

    if( s2data_putfin(ret) == -1 )
    {
        s2obj_release(ret->pobj);
        return NULL;
    }
    return ret;
}

#endif // __has_include(<readline/readline.h>)
#endif

int main(int argc, char *argv[])
{
    s2data_t *accum = NULL;
    s2data_t *line;
    while( (line = s2data_stdio_getline(stdin)) )
    {
        accum = StrLit_Unquote(accum, line);
    }

    fwrite(s2data_weakmap(accum), 1, s2data_len(accum), stdout);
    return 0;
}
