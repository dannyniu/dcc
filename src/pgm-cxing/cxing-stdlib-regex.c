/* DannyNiu/NJF, 2026-05-24. Public Domain. */

#include "cxing-stdlib.h"
#include "../infra/kvtab.h"
#include <librematch.h>

#define S2_OBJ_TYPE_REGEXP 0x211B
#define s2_is_regexp(obj) (((s2obj_t *)obj)->type == S2_OBJ_TYPE_REGEXP)

typedef struct cxing_regex_ctx {
    s2obj_base;

    libregexp_t preg;
} cxing_regex_t;

CxingMethodValueWithImpl(Regex, Copy);
CxingMethodValueWithImpl(Regex, Final);
CxingMethodValueWithImpl(Regex, Split);
CxingMethodValueWithImpl(Regex, Match);
CxingMethodValueWithImpl(Regex, Capture);
CxingMethodValueWithImpl(Regex, Replace);

const type_nativeobj_struct_p6 type_nativeobj_Regex = {
    .typeid = valtyp_obj,
    .n_entries = 6,
    .static_members = {
        { .name = "__copy__", .member = &CxingValue_Regex_Copy },
        { .name = "__final__", .member = &CxingValue_Regex_Final },
        { .name = "split", .member = &CxingValue_Regex_Split },
        { .name = "match", .member = &CxingValue_Regex_Match },
        { .name = "capture", .member = &CxingValue_Regex_Capture },
        { .name = "replace", .member = &CxingValue_Regex_Replace },
    },
};

static int regcomp_map_flags(int64_t x)
{
    int ret = 0;
    if( x == 0 ) return 0;

    while( x )
    {
        int f = x & 63;

        if( f == 34 ) // lower-case 'i'.
        {
            ret |= LIBREG_ICASE;
        }
        else if( f == 39 ) // lower-case 'n'.
        {
            ret |= LIBREG_NEWLINE;
        }

        x >>= 6;
    }
    return ret;
}

static void CxingRegexFinal(cxing_regex_t *x)
{
    libregfree(&x->preg);
}

struct value_nativeobj CxingImpl_Regex_Copy(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, Regex, "regular expression");
    return CxingImpl_s2Obj_Copy(argn, args);
}

struct value_nativeobj CxingImpl_Regex_Final(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, Regex, "regular expression");
    return CxingImpl_s2Obj_Final(argn, args);
}

static struct value_nativeobj CxingRegex_Compile(
    int argn, struct value_nativeobj args[], int cflags_additional)
{
    cxing_regex_t *ret;
    int subret;

    AssertArgN(2);
    AssertArgImpl(0, s2impl_str, "string");
    if( !IsInteger(args[1]) )
    {
        CxingDebug("Regex compilation flags need to be an integer!\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    ret = (void *)s2gc_obj_alloc(S2_OBJ_TYPE_REGEXP, sizeof(cxing_regex_t));
    if( !ret )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    ret->base.finalf = (s2func_final_t)CxingRegexFinal;
    s2obj_keep(ret->pobj);
    s2obj_release(ret->pobj);

    subret = libregcomp(
        &ret->preg, s2data_weakmap(args[0].proper.p),
        regcomp_map_flags(args[1].proper.l) | cflags_additional);
    if( subret != 0 )
    {
        s2obj_leave(ret->pobj);

        // For the reference implementation, the return value from failing
        // regex compulations are negative, which are guaranteed to be
        // separate from `errno` values.
        return (struct value_nativeobj){
            .proper.l = subret,
            .type = (const void *)&type_nativeobj_null };
    }

    return (struct value_nativeobj){
        .proper.p = ret,
        .type = (const void *)&type_nativeobj_Regex };
}

struct value_nativeobj CxingRegex_EREComp(
    int argn, struct value_nativeobj args[])
{
    return CxingRegex_Compile(argn, args, LIBREG_EXTENDED);
}

struct value_nativeobj CxingRegex_BREComp(
    int argn, struct value_nativeobj args[])
{
    return CxingRegex_Compile(argn, args, 0);
}

extern struct value_nativeobj CxingImpl_Array_InitSet(
    int argn, struct value_nativeobj args[]);

extern struct value_nativeobj CxingImpl_Array_Trunc(
    int argn, struct value_nativeobj args[]);

struct value_nativeobj CxingImpl_Regex_Split(
    int argn, struct value_nativeobj args[])
{
    cxing_regex_t *regex;
    s2data_t *strobj;
    const char *str;
    size_t len, t;
    libre_match_t matches;
    int subret;

    s2data_t *segment;
    struct value_nativeobj arrargs[3];

    AssertArgN(2);
    AssertArgImpl(0, Regex, "regular expression");
    AssertArgImpl(1, s2impl_str, "string");

    regex = args[0].proper.p;
    strobj = args[1].proper.p;
    str = s2data_weakmap(strobj);
    len = s2data_len(strobj);
    t = 0;

    arrargs[0] = CxingImpl_Array_Create(0, NULL);
    if( !ValueNativeObj2Logic(arrargs[0]) )
        return arrargs[0]; // array creation failed perhaps.

    arrargs[1].proper.l = 0;
    arrargs[1].type = (const void *)&type_nativeobj_long;

    while( true )
    {
        assert( t <= len );
        if( t == len )
        {
            return arrargs[0];
        }

        subret = libregexec(&regex->preg, str + t, 1, &matches, 0);
        if( subret != 0 )
        {
            // Assumed to be no match.
            // Put the rest of the string onto the back of the array.
            segment = s2data_create(len - t);
            if( !segment )
            {
                // Failed, return `null`.
                ValueDestroy(arrargs[0]);
                return (struct value_nativeobj){
                    .proper.l = errno,
                    .type = (const void *)&type_nativeobj_null };
            }

            memcpy(s2data_weakmap(segment), str + t, len - t);
            arrargs[2].proper.p = segment;
            arrargs[2].type = (const void *)&type_nativeobj_s2impl_str;

            // Could fail, but return partial result anyway.
            CxingImpl_Array_InitSet(3, arrargs);
            s2obj_release(segment->pobj);
            return arrargs[0];
        }

        // Matched a delimiter.
        segment = s2data_create(matches.rm_so);
        if( !segment )
        {
            // Failed, return `null`.
            subret = errno;
            ValueDestroy(arrargs[0]);
            return (struct value_nativeobj){
                .proper.l = subret,
                .type = (const void *)&type_nativeobj_null };
        }
        memcpy(s2data_weakmap(segment), str + t, matches.rm_eo);
        arrargs[2].proper.p = segment;
        arrargs[2].type = (const void *)&type_nativeobj_s2impl_str;

        if( IsNull(CxingImpl_Array_InitSet(3, arrargs)) )
        {
            // If fails, return `null`.
            // Because the argument to `__initset__` is guaranteed to be a string,
            // a `null` return can be distinguished as failure.
            ValueDestroy(arrargs[0]);
            return (struct value_nativeobj){
                .proper.l = errno,
                .type = (const void *)&type_nativeobj_null };
        }
        s2obj_release(segment->pobj);
        arrargs[1].proper.l ++;
        t += matches.rm_eo;
    }
}

struct value_nativeobj CxingImpl_Regex_Match(
    int argn, struct value_nativeobj args[])
{
    cxing_regex_t *regex;
    s2data_t *strobj;
    const char *str;

    AssertArgN(2);
    AssertArgImpl(0, Regex, "regular expression");
    AssertArgImpl(1, s2impl_str, "string");
    
    regex = args[0].proper.p;
    strobj = args[1].proper.p;
    str = s2data_weakmap(strobj);

    if( 0 == libregexec(&regex->preg, str, 0, NULL, 0) )
    {
        // Matched successfully,
        return (struct value_nativeobj){
            .proper.l = true,
            .type = (const void *)&type_nativeobj_long };
    }
    else
    {
        // Not matched,
        return (struct value_nativeobj){
            .proper.l = false,
            .type = (const void *)&type_nativeobj_long };
    }
}

struct value_nativeobj CxingImpl_Regex_Capture(
    int argn, struct value_nativeobj args[])
{
    cxing_regex_t *regex;
    s2data_t *strobj;
    const char *str;
    libre_match_t *matches;
    size_t len, t;
    int subret;

    struct value_nativeobj arrargs[3];

    AssertArgN(2);
    AssertArgImpl(0, Regex, "regular expression");
    AssertArgImpl(1, s2impl_str, "string");

    regex = args[0].proper.p;
    strobj = args[1].proper.p;
    str = s2data_weakmap(strobj);

    matches = calloc(regex->preg.re_nsub+1, sizeof(libre_match_t));
    if( !matches )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    subret = libregexec(&regex->preg, str, regex->preg.re_nsub+1, matches, 0);

    if( subret != 0 )
    {
        free(matches);
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    arrargs[0] = CxingImpl_Array_Create(0, NULL);
    if( IsNull(arrargs[0]) )
    {
        free(matches);
        return arrargs[0];
    }

    arrargs[1] = (struct value_nativeobj){
        .proper.u = regex->preg.re_nsub,
        .type = (const void *)&type_nativeobj_ulong };

    if( IsNull(arrargs[2] = CxingImpl_Array_Trunc(2, arrargs)) )
    {
        free(matches);
        return arrargs[2];
    }

    ValueDestroy(arrargs[0]); // consume rvalue.

    for(t=0; t<=regex->preg.re_nsub; t++)
    {
        s2data_t *ms;
        len = matches[t].rm_eo - matches[t].rm_so;
        ms = s2data_create(len);
        if( !ms )
        {
            subret = errno;
            ValueDestroy(arrargs[0]);
            free(matches);
            return (struct value_nativeobj){
                .proper.l = subret,
                .type = (const void *)&type_nativeobj_null };
        }

        memcpy(s2data_weakmap(ms), str + matches[t].rm_so, len);

        arrargs[2].proper.p = ms;
        arrargs[2].type = (const void *)&type_nativeobj_s2impl_str;
        arrargs[1].proper.u = t;
        arrargs[1].type = (const void *)&type_nativeobj_long;

        if( IsNull(CxingImpl_Array_InitSet(3, arrargs)) )
        {
            subret = errno;
            ValueDestroy(arrargs[0]);
            free(matches);
            return (struct value_nativeobj){
                .proper.l = subret,
                .type = (const void *)&type_nativeobj_null };
        }
        s2obj_release(ms->pobj);
    }

    free(matches);
    return arrargs[0];
}

struct value_nativeobj CxingImpl_Regex_Replace(
    int argn, struct value_nativeobj args[])
{
    cxing_regex_t *regex;
    s2data_t *strobj;
    s2data_t *replacement;
    const char *str, *repl;
    size_t len, t; // the subject string,
    size_t lim, u; // the replacement string.
    libre_match_t matches[10];
    int subret;
    s2data_t *ret;

    AssertArgN(4);
    AssertArgImpl(0, Regex, "regular expression");
    AssertArgImpl(1, s2impl_str, "string");
    AssertArgImpl(2, s2impl_str, "string");

    if( !IsInteger(args[3]) )
    {
        CxingDebug("The `limit` argument should be an integer "
                   "for the `replace` method.\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    regex = args[0].proper.p;
    strobj = args[1].proper.p;
    replacement = args[2].proper.p;
    str = s2data_weakmap(strobj);
    repl = s2data_weakmap(replacement);
    len = s2data_len(strobj);
    lim = s2data_len(replacement);
    t = 0;

    if( !(ret = s2data_create(0)) )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    s2obj_keep(ret->pobj);
    s2obj_release(ret->pobj);

    while( (args[3].type->typeid == valtyp_long &&
            args[3].proper.l == -1) ||
           (args[3].proper.u --> 0 ) )
    {
        assert( t <= len );

        subret = libregexec(&regex->preg, str + t, 10, matches, 0);
        if( subret != 0 )
        {
            break;
        }

        // Put the unmatched part between the start and the match
        // verbatim onto the return string.
        if( 0 != s2data_puts(ret, str + t, matches[0].rm_so) )
        {
            s2obj_leave(ret->pobj);
            return (struct value_nativeobj){
                .proper.l = errno,
                .type = (const void *)&type_nativeobj_null };
        }

        for(u=0; u<lim; u++)
        {
            if( repl[u] != '$' || u+1 >= lim )
            {
                // Not a capture, verbatim.
                if( 0 != s2data_putc(ret, repl[u]) )
                {
                    s2obj_leave(ret->pobj);
                    return (struct value_nativeobj){
                        .proper.l = errno,
                        .type = (const void *)&type_nativeobj_null };
                }
                continue;
            }

            u++;

            if( '0' <= repl[u] && repl[u] <= '9' )
            {
                unsigned i = repl[u] - '0';
                if( i >= 10 || i >= regex->preg.re_nsub + 1 )
                    // Capture index out of bound.
                    continue;

                if( 0 != s2data_puts(
                    ret,
                    str + t + matches[i].rm_so,
                    matches[i].rm_eo - matches[i].rm_so) )
                {
                    s2obj_leave(ret->pobj);
                    return (struct value_nativeobj){
                        .proper.l = errno,
                        .type = (const void *)&type_nativeobj_null };
                }
            }

            if( ispunct(repl[u]) )
            {
                CxingDebug("The meaning of the sequence \"$%c\" is unspecified "
                           "and is potentially non-portable.\n", repl[u]);
            }
            continue;
        }
        t += matches[0].rm_eo;
    }

    // Assumed to be no match.
    // Append the rest of the string.
    if( 0 != s2data_puts(ret, str + t, len - t) )
    {
        s2obj_leave(ret->pobj);
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    if( 0 != s2data_putfin(ret) )
    {
        s2obj_leave(ret->pobj);
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    return (struct value_nativeobj){
        .proper.p = ret,
        .type = (const void *)&type_nativeobj_s2impl_str };
}

cxing_builtin_def_t CxingStdlibRegexBuiltins[] = {
    { "ere_comp", (struct value_nativeobj){
            .proper.p = CxingRegex_EREComp,
            .type = (const void *)&type_nativeobj_subr } },

    { "bre_comp", (struct value_nativeobj){
            .proper.p = CxingRegex_BREComp,
            .type = (const void *)&type_nativeobj_subr } },

    {}
};
