/* DannyNiu/NJF, 2026-06-08. Public Domain. */

#include "cxing-stdlib.h"
#include <SafeTypes2.h>

#define S2_OBJ_TYPE_CMD_INTERP 0x211D
#define s2_is_cmd_interp(obj)                           \
    (((s2obj_t *)obj)->type == S2_OBJ_TYPE_CMD_INTERP)

enum {
    StandardHandle_Inherit = 0,
    StandardHandle_ObtainPipe = 1,
    StandardHandle_Supplied = 2,
};

typedef struct {
    s2obj_base;

    int ArgC;

    // One of the above enumerations.
    int StdInHandling;
    int StdOutHandling;
    int StdErrHandling;

    // The argument vector.
    char **ArgV;
    char **EnvP; // POSIX expects this format.
    char *EnvBlk; // Windows expects this format, wip as of 2026-06-11.
    char *working_directory;

    // If supplied.
    struct value_nativeobj StdInFp;
    struct value_nativeobj StdOutFp;
    struct value_nativeobj StdErrFp;
} cxing_cmd_interp_t;

static void cxing_cmd_interp_final(cxing_cmd_interp_t *x)
{
    if( x->ArgV )
    {
        size_t t;
        for(t=0; x->ArgV[t]; t++)
            free(x->ArgV[t]);
        free(x->ArgV);
        x->ArgV = NULL;
    }

    if( x->EnvP )
    {
        free(x->EnvP);
        free(x->EnvBlk);
        x->EnvP = NULL;
        x->EnvBlk = NULL;
    }

    if( x->working_directory ) (free)(x->working_directory);

    if( x->StdInHandling == StandardHandle_Supplied ) ValueDestroy(x->StdInFp);
    if( x->StdOutHandling == StandardHandle_Supplied ) ValueDestroy(x->StdOutFp);
    if( x->StdErrHandling == StandardHandle_Supplied ) ValueDestroy(x->StdErrFp);
}

#define S2_OBJ_TYPE_PROC_HNDL 0x211E
#define s2_is_proc_hndl(obj)                            \
    (((s2obj_t *)obj)->type == S2_OBJ_TYPE_PROC_HNDL)

CxingMethodValueWithImpl(CmdInterp, Copy);
CxingMethodValueWithImpl(CmdInterp, Final);
CxingMethodValueWithImpl(CmdInterp, Argc);
CxingMethodValueWithImpl(CmdInterp, Argv);
CxingMethodValueWithImpl(CmdInterp, Envp);
CxingMethodValueWithImpl(CmdInterp, ObtainPipeForStdin);
CxingMethodValueWithImpl(CmdInterp, ObtainPipeForStdout);
CxingMethodValueWithImpl(CmdInterp, ObtainPipeForStderr);
CxingMethodValueWithImpl(CmdInterp, SetSourceForStdin);
CxingMethodValueWithImpl(CmdInterp, SetDestForStdout);
CxingMethodValueWithImpl(CmdInterp, SetDestForStderr);
CxingMethodValueWithImpl(CmdInterp, SetCwd);
CxingMethodValueWithImpl(CmdInterp, Exec);

struct value_nativeobj CmdInterp(
    int argn, struct value_nativeobj args[])
{
    cxing_cmd_interp_t *ret = (void *)s2gc_obj_alloc(
        S2_OBJ_TYPE_CMD_INTERP, sizeof(cxing_cmd_interp_t));

    if( !ret )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    (void)argn;
    (void)args;

    s2obj_keep(ret->pobj);
    s2obj_release(ret->pobj);

    ret->base.finalf = (s2func_final_t)cxing_cmd_interp_final;
    ret->ArgC = -1;
    ret->ArgV = NULL;
    ret->StdInFp =
        ret->StdOutFp =
        ret->StdErrFp = (struct value_nativeobj){
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth };

    return (struct value_nativeobj){
        .proper.p = ret,
        .type = (const void *)&type_nativeobj_CmdInterp };
}

struct value_nativeobj CxingImpl_CmdInterp_Copy(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, CmdInterp, "CmdInterp");

    return CxingImpl_s2Obj_Copy(argn, args);
}

struct value_nativeobj CxingImpl_CmdInterp_Final(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, CmdInterp, "CmdInterp");

    return CxingImpl_s2Obj_Final(argn, args);
}

struct value_nativeobj CxingImpl_CmdInterp_Argc(
    int argn, struct value_nativeobj args[])
{
    cxing_cmd_interp_t *ret;

    AssertArgN(2);
    AssertArgImpl(0, CmdInterp, "CmdInterp");

    if( !IsInteger(args[1]) )
    {
        CxingDebug("Argument count must be an integer!\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    ret = args[0].proper.p; // `this`.
    ret->ArgC = args[1].proper.l;
    return ValueCopy(args[0]);
}

extern struct value_nativeobj CxingImpl_Array_Get(
    int argn, struct value_nativeobj args[]);

extern struct value_nativeobj CxingImpl_Array_Len(
    int argn, struct value_nativeobj args[]);

struct value_nativeobj CxingImpl_CmdInterp_Argv(
    int argn, struct value_nativeobj args[])
{
    size_t n, t;
    int subret;
    s2data_t ind = (s2data_t){
        .base.type = S2_OBJ_TYPE_INT(8),
        .len = 8 };
    s2data_t *cpysrc;
    struct value_nativeobj arrargs[3];
    cxing_cmd_interp_t *ret;

    AssertArgN(2);
    AssertArgImpl(0, CmdInterp, "CmdInterp");
    AssertArgImpl(1, array, "array");

    n = CxingImpl_Array_Len(1, &args[1]).proper.u;
    arrargs[0] = args[1];
    arrargs[1] = (struct value_nativeobj){
        .proper.p = &ind,
        .type = (const void *)&type_nativeobj_s2impl_str };
    for(t=0; t<n-1; t++)
    {
        *(uint64_t *)s2data_weakmap(&ind) = t;

        arrargs[2] = CxingImpl_Array_Get(2, arrargs);
        if( arrargs[2].type != (const void *)&type_nativeobj_s2impl_str )
        {
            CxingDebug("Argument %zd is not a string!\n", t);
            return (struct value_nativeobj){
                .proper.p = NULL,
                .type = (const void *)&type_nativeobj_morgoth };
        }
    }

    *(uint64_t *)s2data_weakmap(&ind) = n-1;
    arrargs[2] = CxingImpl_Array_Get(2, arrargs);
    if( !IsNull(arrargs[2]) )
    {
        CxingDebug("Argument array was not terminated by a `null`\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    ret = args[0].proper.p;
    assert( !ret->ArgV );
    ret->ArgV = calloc(n, sizeof(char *));
    if( !ret->ArgV )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    for(t=0; t<n-1; t++)
    {
        *(uint64_t *)s2data_weakmap(&ind) = t;

        cpysrc = CxingImpl_Array_Get(2, arrargs).proper.p;
        ret->ArgV[t] = calloc(s2data_len(cpysrc)+1, 1);
        if( !ret->ArgV[t] ) break;
        memcpy(ret->ArgV[t], s2data_weakmap(cpysrc), s2data_len(cpysrc));
    }

    if( t < n-1 )
    {
        subret = errno;
        while( t-->0 )
        {
            free(ret->ArgV[t]);
            ret->ArgV[t] = NULL;
        }
        free(ret->ArgV);
        ret->ArgV = NULL;
        return (struct value_nativeobj){
            .proper.l = subret,
            .type = (const void *)&type_nativeobj_null };
    }

    return ValueCopy(args[0]);
}

struct value_nativeobj CxingImpl_CmdInterp_Envp(
    int argn, struct value_nativeobj args[])
{
    cxing_cmd_interp_t *ret;
    s2dict_t *envp;
    s2iter_t *it;
    int i, subret;
    size_t filled, capacity, s, t;

    AssertArgN(2);
    AssertArgImpl(0, CmdInterp, "CmdInterp");
    AssertArgImpl(1, s2impl_dict, "dict");

    ret = args[0].proper.p;
    assert( !ret->EnvP );
    filled = capacity = s = t = 0;

    envp = args[1].proper.p;
    it = s2obj_iter_create(envp->pobj);
    if( !it )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    for(i=it->next(it); i>0; i=it->next(it))
    {
        // TODO 2026-06-11: Reimplement to cover `EnvBlk`.
        s2data_t *key = it->key;
        struct value_nativeobj cxval =
            ((s2cxing_value_t *)it->value)->cxing_value;
        s2data_t *val;

        if( cxval.type != (const void *)&type_nativeobj_s2impl_str )
        {
            CxingDebug("Expected string in environment variables.\n");
            break;
        }

        val = cxval.proper.p;

        if( filled >= capacity )
        {
            // 2026-06-09: don't feel like putting a macro here.
            static const unsigned capinc = 32;

            if( !ret->EnvP )
            {
                ret->EnvP = calloc(capacity + capinc + 1, sizeof(char *));
            }
            else
            {
                void *T = realloc(
                    ret->EnvP,
                    (capacity + capinc + 1) *
                    sizeof(char *) );
                if( !T ) free(ret->EnvP);
                ret->EnvP = T;
            }

            if( !ret->EnvP )
            {
                return (struct value_nativeobj){
                    .proper.l = errno,
                    .type = (const void *)&type_nativeobj_null };
            }

            capacity += capinc;
        }

        if( !ret->EnvBlk )
        {
            ret->EnvBlk = calloc(t = s2data_len(key) + 2 + s2data_len(val), 1);
        }
        else
        {
            void *T = realloc(
                ret->EnvBlk,
                (t =
                 s + s2data_len(key) +
                 2 + s2data_len(val) ) + 1);
            if( !T ) free(ret->EnvBlk);
            ret->EnvBlk = T;
        }

        if( !ret->EnvBlk ) break;

        ret->EnvP[filled] = ret->EnvBlk + s;

        memcpy(ret->EnvP[filled], s2data_weakmap(key), s2data_len(key));
        ret->EnvP[filled][s2data_len(key)] = '\0';

        strcat(ret->EnvP[filled], "=");
        strcat(ret->EnvP[filled], s2data_weakmap(val));

        ret->EnvP[filled] = (void *)s;
        filled ++;
        s = t;
    }

    it->final(it);
    ret->EnvBlk[t] = '\0';

    if( i > 0 )
    {
        subret = errno;
        free(ret->EnvP);
        free(ret->EnvBlk);
        ret->EnvP = NULL;
        ret->EnvBlk = NULL;
        return (struct value_nativeobj){
            .proper.l = subret,
            .type = (const void *)&type_nativeobj_null };
    }

    ret->EnvP[filled] = NULL;
    while( filled --> 0 )
    {
        ret->EnvP[filled] = ret->EnvBlk + (ptrdiff_t)ret->EnvP[filled];
    }
    
    return ValueCopy(args[0]);
}

struct value_nativeobj CxingImpl_CmdInterp_ObtainPipeForStdin(
    int argn, struct value_nativeobj args[])
{
    cxing_cmd_interp_t *ret;

    AssertArgN(1);
    AssertArgImpl(0, CmdInterp, "CmdInterp");

    ret = args[0].proper.p;

    ret->StdInHandling = StandardHandle_ObtainPipe;
    assert( IsNull(ret->StdInFp) );
    return ValueCopy(args[0]);
}

struct value_nativeobj CxingImpl_CmdInterp_ObtainPipeForStdout(
    int argn, struct value_nativeobj args[])
{
    cxing_cmd_interp_t *ret;

    AssertArgN(1);
    AssertArgImpl(0, CmdInterp, "CmdInterp");

    ret = args[0].proper.p;

    ret->StdOutHandling = StandardHandle_ObtainPipe;
    assert( IsNull(ret->StdOutFp) );
    return ValueCopy(args[0]);
}

struct value_nativeobj CxingImpl_CmdInterp_ObtainPipeForStderr(
    int argn, struct value_nativeobj args[])
{
    cxing_cmd_interp_t *ret;

    AssertArgN(1);
    AssertArgImpl(0, CmdInterp, "CmdInterp");

    ret = args[0].proper.p;

    ret->StdErrHandling = StandardHandle_ObtainPipe;
    assert( IsNull(ret->StdErrFp) );
    return ValueCopy(args[0]);
}

struct value_nativeobj CxingImpl_CmdInterp_SetSourceForStdin(
    int argn, struct value_nativeobj args[])
{
    cxing_cmd_interp_t *ret;

    AssertArgN(2);
    AssertArgImpl(0, CmdInterp, "CmdInterp");
    AssertArgImpls(
        1,
        AcceptArgImpl(1, RegFile)
        AcceptArgImpl(1, Pipe),
        "General File");

    ret = args[0].proper.p;

    ret->StdInHandling = StandardHandle_Supplied;
    assert( IsNull(ret->StdInFp) );
    ret->StdInFp = ValueCopy(args[1]);
    return ValueCopy(args[0]);
}

struct value_nativeobj CxingImpl_CmdInterp_SetDestForStdout(
    int argn, struct value_nativeobj args[])
{
    cxing_cmd_interp_t *ret;

    AssertArgN(2);
    AssertArgImpl(0, CmdInterp, "CmdInterp");
    AssertArgImpls(
        1,
        AcceptArgImpl(1, RegFile)
        AcceptArgImpl(1, Pipe),
        "General File");

    ret = args[0].proper.p;

    ret->StdOutHandling = StandardHandle_Supplied;
    assert( IsNull(ret->StdOutFp) );
    ret->StdOutFp = ValueCopy(args[1]);
    return ValueCopy(args[0]);
}

struct value_nativeobj CxingImpl_CmdInterp_SetDestForStderr(
    int argn, struct value_nativeobj args[])
{
    cxing_cmd_interp_t *ret;

    AssertArgN(2);
    AssertArgImpl(0, CmdInterp, "CmdInterp");
    AssertArgImpls(
        1,
        AcceptArgImpl(1, RegFile)
        AcceptArgImpl(1, Pipe),
        "General File");

    ret = args[0].proper.p;

    ret->StdErrHandling = StandardHandle_Supplied;
    assert( IsNull(ret->StdErrFp) );
    ret->StdErrFp = ValueCopy(args[1]);
    return ValueCopy(args[0]);
}

struct value_nativeobj CxingImpl_CmdInterp_SetCwd(
    int argn, struct value_nativeobj args[])
{
    cxing_cmd_interp_t *ret;

    AssertArgN(2);
    AssertArgImpl(0, CmdInterp, "CmdInterp");
    AssertArgImpl(1, s2impl_str, "string");

    ret = args[0].proper.p;

    if( !(ret->working_directory = strdup(
              s2data_weakmap(args[1].proper.p))) )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    return ValueCopy(args[0]);
}

const type_nativeobj_struct_p13 type_nativeobj_CmdInterp = {
    .typeid = valtyp_obj,
    .n_entries = 13,
    .static_members = {
        { .name = "__copy__", .member = &CxingValue_CmdInterp_Copy },
        { .name = "__final__", .member = &CxingValue_CmdInterp_Final },
        { .name = "Argc", .member = &CxingValue_CmdInterp_Argc },
        { .name = "Argv", .member = &CxingValue_CmdInterp_Argv },
        { .name = "Envp", .member = &CxingValue_CmdInterp_Envp },
        { .name = "ObtainPipeForStdin", .member = &CxingValue_CmdInterp_ObtainPipeForStdin },
        { .name = "ObtainPipeForStdout", .member = &CxingValue_CmdInterp_ObtainPipeForStdout },
        { .name = "ObtainPipeForStderr", .member = &CxingValue_CmdInterp_ObtainPipeForStderr },
        { .name = "SetSourceForStdin", .member = &CxingValue_CmdInterp_SetSourceForStdin },
        { .name = "SetDestForStdout", .member = &CxingValue_CmdInterp_SetDestForStdout },
        { .name = "SetDestForStderr", .member = &CxingValue_CmdInterp_SetDestForStderr },
        { .name = "SetCwd", .member = &CxingValue_CmdInterp_SetCwd },
        { .name = "Exec", .member = &CxingValue_CmdInterp_Exec },
    },
};
