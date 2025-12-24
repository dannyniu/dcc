/* DannyNiu/NJF, 2025-02-23. Public Domain. */

#include "lalr.h"

void print_token(lex_token_t *tn, int indentlevel)
{
    (void)indentlevel;
    printf("%s\n", (char *)s2data_weakmap(tn->str));
}

void print_prod(lalr_prod_t *prod, int indentlevel, strvec_t *ns)
{
    size_t t;
    printf("%d:%s<%d:%s>\n",
           prod->rule,
           strvec_i2str(ns, prod->production),
           prod->semantic_rule,
           strvec_i2str(ns, prod->semantic_production));

    for(t=0; t<prod->terms_count; t++)
    {
        printf("%*s [%zi] ", indentlevel * 2 + 3, "", t);
        if( s2_is_prod(prod->terms[t].production) )
            print_prod(prod->terms[t].production, indentlevel + 1, ns);
        else
        {
            lex_token_t *tn = prod->terms[t].terminal;
            assert( s2_is_token(tn) );
            print_token(tn, indentlevel);
        }
    }
}
