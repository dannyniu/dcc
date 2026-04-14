/* DannyNiu/NJF, 2026-04-12. Public Domain. */

#ifdef NextHeader

(void)parsed;
(void)te;
(void)indentlevel;

perfcounter = clock();

cpptu_t tu_obj = { .base.type = S2_OBJ_TYPE_CPPTU }, *tu = &tu_obj; // (cpptu_t *)s2gc_obj_alloc(S2_OBJ_TYPE_CPPTU, sizeof(cpptu_t));
int ll = 1; // last line.
FILE *doti = fmemopen(NULL, 1280, "w+");

tu->macros = s2list_create();
tu->ctx_shifter = &lexer;
tu->shifter = (token_shifter_t)RegexLexFromRope_Shift;
tu->pushlist = s2list_create();

tu->lash.sv = NULL;
tu->lash.ctx_shifter = tu->ctx_shifter;
tu->lash.shifter = tu->shifter;

tu->rescan_stackbase.coldlist = &tu->lash;
tu->rescan_stackbase.coldlist_shifter = (token_shifter_t)cppBufferedShifterCoroutine;
tu->rescan_stackbase.ctx_tu = tu;
tu->rescan_stackbase.pushlist = tu->pushlist;

//*
while( true )
{
    lex_token_t *tok = cppMainProgramCoroutine(tu);
    if( !tok ) break;

    for(i=0; langlex_token_strtab[i].str; i++)
        if(  langlex_token_strtab[i].enumerant ==
             tok->completion )
            break;

    if( tok->lineno > ll )
    {
        fprintf(doti, "\n");
        ll = tok->lineno;
    }
    fprintf(doti, "%s ", (const char *)s2data_weakmap(tok->str));

    s2obj_release(tok->pobj);
}
fprintf(doti, "\n");

if( doti != stdout && 0 )
{
    fseek(doti, 0, SEEK_SET);
    while( (ll = fgetc(doti)) >= 0 )
        putchar(ll);
    fclose(doti);
}//*/

s2obj_release(tu->macros->pobj);
s2obj_release(tu->pushlist->pobj);
//s2obj_release(tu->pobj);

perfcounter = clock() - perfcounter;
subret = EXIT_SUCCESS;

#endif /* NextHeader */
