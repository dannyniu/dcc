/* DannyNiu/NJF, 2026-07-19. Public Domain. */

#include "cpp-c.h"

int cppProcessIncludeDirective(
    cpptu_t *restrict ctx_tu,
    void *restrict ctx_shifter,
    token_shifter_t shifter)
{
    // Directive was `define`, handle the rest of the line.

    s2list_t *argtoks = (s2list_t *)s2obj_retain(ctx_shifter);
    char *m;

    struct cppMacroExpandShifter ppeval = {
        .flags = MACEXP_FLAG_EVALCTX_CTRLLINE,
        .ctx_tu = ctx_tu };

    assert( shifter == (token_shifter_t)shift_from_s2list );

    m = ResolveImportationFilename(argtoks, ctx_tu->IncPaths, &ppeval);

    if( !m ) ccDiagnoseError(ctx_tu, "Cannot open include file.", " path: \"%s\"", m);

    ctx_tu->Includee = cpptu_create(m, ctx_tu);
    (free)(m);
    s2obj_release(argtoks->pobj);
    if( ctx_tu->Includee ) return 0;
    else return -1;
}
