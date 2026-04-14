/* DannyNiu/NJF, 2025-03-29. Public Domain. */

#ifdef NextHeader

NS_RULES = strvec_create();

perfcounter = clock();
i = lalr_parse(&parsed, GRAMMAR_RULES, NULL, NS_RULES,
               (token_shifter_t)RegexLexFromRope_Shift, (void *)&lexer);
printf("parsing returned: %d after %ld clock cycles, stack:\n",
       i, clock() - perfcounter);

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

lalr_parse_accel_cache_clear();
s2obj_release(parsed->pobj);
s2obj_release(NS_RULES->pobj);
subret = EXIT_SUCCESS;

#endif /* NextHeader */
