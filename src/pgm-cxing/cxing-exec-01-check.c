/* DannyNiu/NJF, 2025-12-30. Public Domain. */

// Execute the `main` function in a cxing source code.

#include "../langlex/langlex-cxing.h"
#include "cxing-grammar.h"
#define GRAMMAR_RULES cxing_grammar_rules
#define NS_RULES ns_rules_cxing
#define var_lex_elems LexElems

#include "../lex-common/rope.h"
#include "../infra/strvec.h"
#include "../lalr-common/lalr.h"

#include "runtime.h"
#include "cxing-interp.h"

#if false // no need to link with readline
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
#endif // false // no need to link with readline

int logger(void *ctx, const char *msg)
{
    (void)ctx;
    fprintf(stderr, "%s\n", msg);
    return 0;
}

int main(int argc, char *argv[])
{
    union {
        lex_getc_base_t base;
        lex_getc_fp_t fromfile;
        lex_getc_str_t fromstr;
    } expr_getc;
    source_rope_t *rope;
    cxing_tokenizer tokenizer;

    lalr_stack_t *parsed;
    lalr_term_t *te;
    int indentlevel = 0;
    int subret = 0, i;

    struct value_nativeobj args[1] = {
        { .proper.l= 6, .type = (const void *)&type_nativeobj_long }
    };
    struct value_nativeobj fret;

#if INTERCEPT_MEM_CALLS
    long acq_before = 0;
    long rel_before = 0;
    long acq_after = 0;
    long rel_after = 0;
#endif /* INTERCEPT_MEM_CALLS */

    CXParserInitCommon();
    CxingRuntimeInit();

    assert( argc > 2 );

    if( strcmp(argv[1], "-f") == 0 )
    {
        lex_getc_init_from_fp(&expr_getc.fromfile, fopen(argv[2], "r"));
    }
    else if( strcmp(argv[1], "-e") == 0 )
    {
        lex_getc_init_from_str(&expr_getc.fromstr, argv[2]);
    }
    rope = CreateRopeFromGetc(&expr_getc.base, 0);
    CxingTokenizerInit(&tokenizer, rope);

    tokenizer.cmtstrip.lexer.logger_base = (struct logging_ctxbase){
        .logger = (logger_func)logger,
    };

#if INTERCEPT_MEM_CALLS
    acq_before = allocs;
    rel_before = frees;
#endif /* INTERCEPT_MEM_CALLS */

    i = lalr_parse(&parsed, GRAMMAR_RULES, NULL, NS_RULES,
               (token_shifter_t)CxingTokenizer_Shifter, (void *)&tokenizer);
    printf("parsing returned: %d, stack:\n", i);
    s2obj_release(rope->pobj);

    te = parsed->bottom;
    while( te )
    {
        printf("%p\t: ", te);

        if( s2_is_prod(te->production) )
        {
            print_prod(te->production, indentlevel, NS_RULES);
        }
        else print_token(te->terminal, indentlevel);

        te = te->up;
    }

    //if( false )
    fret = CxingExecuteFunction(
        NULL,
        parsed->bottom->production->terms[3].production,
        parsed->bottom->production->terms[2].production, 1, args);
    printf("return value type: %lld ", fret.type->typeid);
    if( fret.type == (const void *)&type_nativeobj_s2impl_str )
    {
        printf("and is a string: %s\n",
               (const char *)s2data_weakmap(fret.proper.p));
    }
    else if( fret.type->typeid == valtyp_double )
    {
        printf("and is a floating-point number: %g\n", fret.proper.f);
    }
    else printf("and its hex is: %llx\n", fret.proper.u);

    s2obj_release(parsed->pobj);
    CxingRuntimeFinal();
    CXParserFinalCommon();
    subret = EXIT_SUCCESS;

#if INTERCEPT_MEM_CALLS
    acq_after = allocs;
    rel_after = frees;
    printf("acq-before: %ld, acq-after: %ld.\n", acq_before, acq_after);
    printf("rel-before: %ld, rel-after: %ld.\n", rel_before, rel_after);
    printf("mem-acquire: %ld, mem-release: %ld.\n", allocs, frees);
    for(i=0; i<4; i++)
    {
        if( mh[i] ) subret = EXIT_FAILURE;
        printf("%08lx%c", (long)mh[i], i==3 ? '\n' : ' ');
    }
#endif /* INTERCEPT_MEM_CALLS */
    return subret;
}
