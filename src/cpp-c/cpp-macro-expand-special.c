/* DannyNiu/NJF, 2026-05-16. Public Domain. */

#include "cpp-c.h"
#include "../c-misc/dequoting.h"
#include "../pathutils/pathutils.h"
#include <sys/types.h>
#include <sys/stat.h>

const char *special_funclike_macros[] = {
    "defined", "__has_include",
    NULL,
};

const char *special_objlike_macros[] = {
    "__STDC__",
    "__STDC_VERSION__",
    NULL,
};

bool SpecialEval_Defined(lex_token_t *identifier)
{
    const char *tokstr  = s2data_weakmap(identifier->str);
    int i;

    for(i=0; special_funclike_macros[i]; i++)
    {
        if( 0 == strcmp(special_funclike_macros[i], tokstr) )
        {
            return true;
        }
    }

    for(i=0; special_objlike_macros[i]; i++)
    {
        if( 0 == strcmp(special_objlike_macros[i], tokstr) )
        {
            return true;
        }
    }

    return false;
}

char *ResolveImportationFilename(
    s2list_t *atoks, s2list_t *incpaths,
    struct cppMacroExpandShifter *ctx)
{
    lex_token_t *tx, *cur;

    s2data_t *dirname = NULL; // Non-NULL implies quote-form of header name.
    s2data_t *filename = NULL;
    char *m;
    struct stat fileinfo;
    int subret = -1, post_expand = false;

eval_has_include_start:
    s2list_seek(atoks, 0, S2_LIST_SEEK_SET);
    s2list_get_T(lex_token_t)(atoks, &tx);
    cur = tx;
    if( s2list_len(atoks) == 1 && tx->completion == langlex_strlit )
    {
        // Actually, the standard did not permit such prefixes.
        int skipc = 0;
        m = s2data_weakmap(tx->str);
        if( strncmp("u8\"", m, 3) == 0 ) skipc = 2;
        else if( strchr("uUL", m[0]) ) skipc = 1;
        else skipc = 0;

        // 2026-07-05 Idea: maybe support the GNU `-iquote` option? (lo-prio)
        filename = StrLit_Unquote(NULL, tx->str, skipc);
        dirname = ctx->ctx_tu->HotFile;
        m = PathReplaceBasename(
            s2data_weakmap(dirname),
            s2data_weakmap(filename));
        subret = stat(m, &fileinfo);

        // A non-NULL `m` is returned as _the_ indication of a successful resolution.
        //- (free)(m); // the creating function was not subject to mem-intercept.
    }
    else if( strcmp("<", s2data_weakmap(tx->str)) == 0 )
    {
        // 2026-07-06: needs lexing all valid _and_ invalid chars.
        int sep = -1;
        filename = s2data_create(0);

        while( true )
        {
            s2list_seek(atoks, 1, S2_LIST_SEEK_CUR);
            s2list_get_T(lex_token_t)(atoks, &tx);
            m = s2data_weakmap(tx->str);
            if( strncmp(m, ">", 1) == 0 )
                break;

            s2data_puts(filename, m, strlen(m));
            if( sep > 0 ) // All filenames ARE nul-terminated!
            {
                if( (tx->attrs & TOKATTR_BLANKDELIM) == TOKATTR_BLANKDELIM )
                    s2data_putc(filename, sep);
            }
            sep = ' ';
            continue;
        }

        if( s2list_pos(atoks) + 1 < s2list_len(atoks) )
        {
            ccDiagnoseError(ctx->ctx_tu, "Excess tokens after closing angle bracket", spelling_and_site(tx));
        }
    }
    else if( !post_expand )
    {
        s2list_t *formedarg = s2list_create();
        struct cppMacroExpandShifter argeval = *ctx;
        struct MacroArgPointer argptr = {
            .args_found = atoks,
            .argp = atoks->anch_head.next,
        };

        argeval.coldlist = &argptr;
        argeval.coldlist_shifter = (token_shifter_t)ArgTokSeqShifter;
        argeval.pushlist = formedarg;
        argeval.hotlist = NULL;

        while( argptr.argp != &argptr.args_found->anch_tail )
        {
            Reached("Calling ScanningRecursion from `ExpandSpecial`.\n");
            ScanningRecursion(&argeval);
            Reached("ScanningRecursion Retruned to `ExpandSpecial`.\n");
        }

        atoks = formedarg;
        post_expand = true;
        goto eval_has_include_start;
    }
    else
    {
        ccDiagnoseError(ctx->ctx_tu, "Invalid token sequence for a header name.", " expanded from" spelling_and_site(cur));
    }

    if( subret != 0 )
    {
        s2data_t *path = s2data_create(0);
        s2iter_t *it = s2obj_iter_create(incpaths->pobj);
        int i;
        for(i=it->next(it); i>0; i=it->next(it))
        {
            dirname = (s2data_t *)it->value;
            s2data_puts(path, s2data_weakmap(dirname), s2data_len(dirname));
            s2data_putc(path, '/');
            s2data_puts(path, s2data_weakmap(filename), s2data_len(filename));

            s2data_putfin(path);
            subret = stat(s2data_weakmap(path), &fileinfo);
            if( subret == 0 )
            {
                m = strdup(s2data_weakmap(path)); // XPGv4 API added in C23.
                break;
            }
            s2data_trunc(path, 0);
        }
        s2obj_release(path->pobj);
        it->final(it);
    }

    if( post_expand ) s2obj_release(atoks->pobj);

    s2obj_release(filename->pobj);

    return subret == 0 ? m : NULL;
}

s2list_t *ExpandSpecial(
    struct cppMacroExpandShifter *ctx,
    lex_token_t *macname,
    s2list_t *args) // a list of lists of tokens.
{
    s2list_t *ret = s2list_create();
    lex_token_t *cur;
    const char *mname = s2data_weakmap(macname->str);

    s2list_t *atoks;
    lex_token_t *tx;

    // 2026-07-04:
    // This should be guaranteed by the caller when collecting arguments.
    assert( s2list_len(args) == 1 );
    s2list_seek(args, 0, S2_LIST_SEEK_SET);
    s2list_get_T(s2list_t)(args, &atoks);

    if( 0 == strcmp(mname, "defined") )
    {
        if( s2list_len(atoks) != 1 )
        {
            // 2026-05-16: or too few.
            ccDiagnoseError(ctx->ctx_tu, "Excess or insuffucent arguments for the `defined` preprocessing predicate", spelling_and_site(macname));
        }

        s2list_seek(atoks, 0, S2_LIST_SEEK_SET);
        s2list_get_T(lex_token_t)(atoks, &tx);

        if( SpecialEval_Defined(tx) ||
            cppLookup1Macro(ctx->ctx_tu, tx) )
        {
            cur = lex_token_create();
            s2data_putc(cur->str, '1');
            s2data_putfin(cur->str);
            cur->completion = langlex_declit;
            cur->lineno = macname->lineno;
            cur->column = macname->column;
        }
        else
        {
            cur = lex_token_create();
            s2data_putc(cur->str, '0');
            s2data_putfin(cur->str);
            cur->completion = langlex_octlit;
            cur->lineno = macname->lineno;
            cur->column = macname->column;
        }

        s2list_push(ret, cur->pobj, s2_setter_gave);
    }
    else if( 0 == strcmp(mname, "__has_include") )
    {
        char *m = ResolveImportationFilename(atoks, ctx->ctx_tu->IncPaths, ctx);

        cur = lex_token_create();
        s2data_putc(cur->str, m ? '1' : '0' );
        s2data_putfin(cur->str);
        cur->completion = langlex_declit;
        cur->lineno = macname->lineno;
        cur->column = macname->column;
        s2list_push(ret, cur->pobj, s2_setter_gave);

        // 2026-07-19.
        // Parenthesised because the resource was
        // acquired without memory interception.
        (free)(m);
    }
    else
    {
        ccDiagnoseError(ctx->ctx_tu, "Unrecognized special macro (implementation underway?)", spelling_and_site(macname));

        // 2026-07-10:
        // The rest shouldn't be difficult.
    }

    if( args ) s2obj_release(args->pobj);
    return ret;
}
