/* DannyNiu/NJF, 2026-06-11. Public Domain. */

#include "cxing-stdlib-proc-common.bits.h"
#include "../infra/kvtab.h"
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <windows.h>

// 2026-06-14:
// this declaration is not needed on MinGW.
// extern char **environ;
extern void CxingImpl_FileRef_Close(FILE *fp);

static HANDLE DupHndl(intptr_t hndl)
{
    HANDLE req = (HANDLE)hndl, resp;
    HANDLE me = GetCurrentProcess();
    if( !DuplicateHandle(
            me, req, me, &resp,
            0, TRUE, DUPLICATE_SAME_ACCESS) )
        return INVALID_HANDLE_VALUE;
    else return resp;
}

static void FinHndl(HANDLE hndl)
{
    CloseHandle(hndl);
}

struct value_nativeobj CxingImpl_CmdInterp_Exec(
    int argn, struct value_nativeobj args[])
{
    cxing_cmd_interp_t *ctx;
    size_t t;
    int subret;

    // 2026-06-14:
    // the following objects are declared in (sorta) initialization order.
    // such organization helps keep track of finalization in case of error.
    kvtab_t *ret;
    s2data_t *cmdl;

    int infile[3]  = { -1, -1, 0 };
    int outfile[3] = { -1, -1, 0 };
    int errfile[3] = { -1, -1, 0 };
    HANDLE inhnd, outhnd, errhnd;

    STARTUPINFO StartupInfo = {};
    PROCESS_INFORMATION ProcessInfo;
    FILE *fp;

    AssertArgN(1);
    AssertArgImpl(0, CmdInterp, "");

    ctx = args[0].proper.p;

    // validate input consistency.

    if( ctx->ArgC <= 0 )
    {
        CxingDebug("Argument count not specified for invoking program!\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_null };
    }

    if( !ctx->ArgV )
    {
        CxingDebug("Argument vector not specified for invoking program!\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_null };
    }

    for(t=0; ctx->ArgV[t]; t++)
        ;

    if( (size_t)(int)t != t || t != (size_t)ctx->ArgC )
    {
        CxingDebug("Inconsistent argument count and vector length! "
                   "Program invocation refused.\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_null };
    }

    if( !(ret = kvtab_create(4)) ||
        !(ret->entries[3].v = s2ref_create(NULL, NULL)->pobj) )
    {
        if( ret ) s2obj_release(ret->pobj);
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    s2obj_keep(ret->pobj);
    s2obj_release(ret->pobj);

    // build command line from argument vector.

    subret = (cmdl = s2data_create(0)) &&
        0 == s2data_putc(cmdl, '\"') &&
        0 == s2data_puts(cmdl, ctx->ArgV[0], strlen(ctx->ArgV[0])) &&
        0 == s2data_puts(cmdl, "\" ", 2);

    if( !subret )
    {
        subret = errno;
        s2obj_leave(ret->pobj);
        if( cmdl ) s2obj_release(cmdl->pobj);
        return (struct value_nativeobj){
            .proper.l = subret,
            .type = (const void *)&type_nativeobj_null };
    }

    for(t=1; (int)t < ctx->ArgC; t++)
    {
        size_t p = 0;
        int c;

        if( s2data_putc(cmdl, '\"') != 0 )
            break;

        while( (c = ctx->ArgV[t][p++]) )
        {
            if( c == '\\' || c == '\"' )
                if( s2data_putc(cmdl, '\\') != 0 )
                    break;

            if( s2data_putc(cmdl, c) != 0 )
                break;
        }
        if( c ) break;

        if( s2data_puts(cmdl, "\" ", 2) != 0 )
            break;
    }

    // handle errors occured while building command line.

    if( (int)t < ctx->ArgC || 0 != s2data_putfin(cmdl) )
    {
        subret = errno;
        s2obj_leave(ret->pobj);
        s2obj_release(cmdl->pobj);
        return (struct value_nativeobj){
            .proper.l = subret,
            .type = (const void *)&type_nativeobj_null };
    }

    // setup standard io channels.

    if( ctx->StdInHandling == StandardHandle_ObtainPipe )
    {
        infile[2] = _pipe(infile, 256, O_BINARY|O_NOINHERIT);
    }

    if( ctx->StdOutHandling == StandardHandle_ObtainPipe && infile[2] == 0 )
    {
        outfile[2] = _pipe(outfile, 256, O_BINARY|O_NOINHERIT);
    }

    if( ctx->StdErrHandling == StandardHandle_ObtainPipe && outfile[2] == 0 )
    {
        errfile[2] = _pipe(errfile, 256, O_BINARY|O_NOINHERIT);
    }

    if( infile[2] != 0 || outfile[2] != 0 || errfile[2] != 0 )
    {
        subret = errno;
        s2obj_leave(ret->pobj);
        s2obj_release(cmdl->pobj);
        close(infile[0]);
        close(infile[1]);
        close(outfile[0]);
        close(outfile[1]);
        close(errfile[0]);
        close(errfile[1]);
        return (struct value_nativeobj){
            .proper.l = subret,
            .type = (const void *)&type_nativeobj_null };
    }

    inhnd = outhnd = errhnd = INVALID_HANDLE_VALUE;

    switch( ctx->StdInHandling )
    {
    case StandardHandle_Inherit:
        inhnd = GetStdHandle(STD_INPUT_HANDLE);
        break;

    case StandardHandle_ObtainPipe:
        inhnd = DupHndl(_get_osfhandle(infile[0]));
        break;

    case StandardHandle_Supplied:
        inhnd = DupHndl(_get_osfhandle(
                            fileno(s2ref_unwrap(ctx->StdInFp.proper.p))));
        break;
    }

    switch( ctx->StdOutHandling )
    {
    case StandardHandle_Inherit:
        outhnd = GetStdHandle(STD_OUTPUT_HANDLE);
        break;

    case StandardHandle_ObtainPipe:
        outhnd = DupHndl(_get_osfhandle(outfile[1]));
        break;

    case StandardHandle_Supplied:
        outhnd = DupHndl(_get_osfhandle(
                             fileno(s2ref_unwrap(ctx->StdOutFp.proper.p))));
        break;
    }

    switch( ctx->StdErrHandling )
    {
    case StandardHandle_Inherit:
        errhnd = GetStdHandle(STD_ERROR_HANDLE);
        break;

    case StandardHandle_ObtainPipe:
        errhnd = DupHndl(_get_osfhandle(errfile[1]));
        break;

    case StandardHandle_Supplied:
        errhnd = DupHndl(_get_osfhandle(
                             fileno(s2ref_unwrap(ctx->StdErrFp.proper.p))));
        break;
    }

    // 2026-06-13: Ignore invalid handle errors for this instance.

    StartupInfo.cb = sizeof(STARTUPINFO);
    StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    StartupInfo.hStdInput = inhnd;
    StartupInfo.hStdOutput = outhnd;
    StartupInfo.hStdError = errhnd;

    subret = CreateProcess(
        NULL, s2data_weakmap(cmdl),
        NULL, NULL, TRUE, 0,
        ctx->EnvBlk,
        ctx->working_directory,
        &StartupInfo,
        &ProcessInfo);
    s2obj_release(cmdl->pobj);

    if( !subret )
    {
        int64_t cast = CXErrNS(GetLastError, ());
        s2obj_leave(ret->pobj);
        s2obj_release(cmdl->pobj);
        close(infile[0]);
        close(infile[1]);
        close(outfile[0]);
        close(outfile[1]);
        close(errfile[0]);
        close(errfile[1]);
        if( ctx->StdInHandling != StandardHandle_Inherit ) CloseHandle(inhnd);
        if( ctx->StdOutHandling != StandardHandle_Inherit ) CloseHandle(outhnd);
        if( ctx->StdErrHandling != StandardHandle_Inherit ) CloseHandle(errhnd);
        return (struct value_nativeobj){
            .proper.l = cast,
            .type = (const void *)&type_nativeobj_null };
    }
    if( ctx->StdInHandling != StandardHandle_Inherit ) CloseHandle(inhnd);
    if( ctx->StdOutHandling != StandardHandle_Inherit ) CloseHandle(outhnd);
    if( ctx->StdErrHandling != StandardHandle_Inherit ) CloseHandle(errhnd);
    CloseHandle(ProcessInfo.hThread);

    // Create ProcHndl object and return it.

    ret->entries[0].k = "infile";
    ret->entries[1].k = "outfile";
    ret->entries[2].k = "errfile";

    ret->entries[0].v = NULL;
    ret->entries[1].v = NULL;
    ret->entries[2].v = NULL;

    // The last one for PID (process handle on Windows) of the child process.
    ret->count = 4;

    // 2026-06-14: needs a non-NULL key for `kvtab_get`.
    ret->entries[3].k = "__hProcHndl__";
    ((s2ref_t *)ret->entries[3].v)->ptr = ProcessInfo.hProcess;

    // 2026-06-14: surprisingly, the type signature matches.
    ((s2ref_t *)ret->entries[3].v)->finalizer = FinHndl;

    fp = (void *)true;

    if( ctx->StdInHandling == StandardHandle_ObtainPipe )
    {
        if( !(fp = fdopen(infile[1], "wb")) ||
            !(ret->entries[0].v = s2ref_create(
                  fp, (s2ref_final_func_t)CxingImpl_FileRef_Close)->pobj) )
        {
            subret = errno;
            if( fp ) fclose(fp);
            else close(infile[1]);
            fp = (void *)false;
        }

        close(infile[0]);
        infile[0] = infile[1] = -1;
    }

    if( ctx->StdOutHandling == StandardHandle_ObtainPipe && fp )
    {
        if( !(fp = fdopen(outfile[0], "rb")) ||
            !(ret->entries[1].v = s2ref_create(
                  fp, (s2ref_final_func_t)CxingImpl_FileRef_Close)->pobj) )
        {
            subret = errno;
            if( fp ) fclose(fp);
            else close(outfile[0]);
            fp = (void *)false;
        }

        close(outfile[1]);
        outfile[0] = outfile[1] = -1;
    }

    if( ctx->StdErrHandling == StandardHandle_ObtainPipe && fp )
    {
        if( !(fp = fdopen(errfile[0], "rb")) ||
            !(ret->entries[2].v = s2ref_create(
                  fp, (s2ref_final_func_t)CxingImpl_FileRef_Close)->pobj) )
        {
            subret = errno;
            if( fp ) fclose(fp);
            else close(errfile[0]);
            fp = (void *)false;
        }

        close(errfile[1]);
        errfile[0] = errfile[1] = -1;
    }

    if( !fp )
    {
        s2obj_leave(ret->pobj);
        close(infile[0]);
        close(infile[1]);
        close(outfile[0]);
        close(outfile[1]);
        close(errfile[0]);
        close(errfile[1]);
        return (struct value_nativeobj){
            .proper.l = subret,
            .type = (const void *)&type_nativeobj_null };
    }

    return (struct value_nativeobj){
        .proper.p = ret,
        .type = (const void *)&type_nativeobj_ProcHndl };
}

CxingMethodValueWithImpl(ProcHndl, Get);
CxingMethodValueWithImpl(ProcHndl, Copy);
CxingMethodValueWithImpl(ProcHndl, Final);
CxingMethodValueWithImpl(ProcHndl, Wait);
CxingMethodValueWithImpl(ProcHndl, Terminate);
CxingMethodValueWithImpl(ProcHndl, Kill);
CxingMethodValueWithImpl(ProcHndl, Stop);
CxingMethodValueWithImpl(ProcHndl, Continue);

struct value_nativeobj CxingImpl_ProcHndl_Get(
    int argn, struct value_nativeobj args[])
{
    s2ref_t *ret;
    const char *k;
    AssertArgN(2);
    AssertArgImpl(0, ProcHndl, "process handle");
    AssertArgImpl(1, s2impl_str, "string");

    k = s2data_weakmap(args[1].proper.p);
    if( 0 == strcmp("infile", k) ||
        0 == strcmp("outfile", k) ||
        0 == strcmp("errfile", k) )
        ret = kvtab_get(args[0].proper.p, k);
    else ret = NULL;

    if( ret )
    {
        return (struct value_nativeobj){
            .proper.p = ret,
            .type = (const void *)&type_nativeobj_Pipe };
    }
    else
    {
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }
}

struct value_nativeobj CxingImpl_ProcHndl_Copy(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, ProcHndl, "process handle");

    return CxingImpl_s2Obj_Copy(argn, args);
}

struct value_nativeobj CxingImpl_ProcHndl_Final(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, ProcHndl, "process handle");

    return CxingImpl_s2Obj_Final(argn, args);
}

struct value_nativeobj CxingImpl_ProcHndl_Wait(
    int argn, struct value_nativeobj args[])
{
    kvtab_t *ctx;
    HANDLE pid;
    int exit_stat;

    AssertArgN(1);
    AssertArgImpl(0, ProcHndl, "process handle");

    ctx = args[0].proper.p;
    pid = s2ref_unwrap((s2ref_t *)ctx->entries[3].v);
    if( cwait(&exit_stat, (intptr_t)pid, WAIT_CHILD) == -1 )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }
    else
    {
        return (struct value_nativeobj){
            .proper.l = exit_stat,
            .type = (const void *)&type_nativeobj_long };
    }
}

static struct value_nativeobj CxingImpl_ProcHndl_Kill0(
    int argn, struct value_nativeobj args[], int signo)
{
    kvtab_t *ctx;

    AssertArgN(1);
    AssertArgImpl(0, ProcHndl, "process handle");

    ctx = args[0].proper.p;

    if( TerminateProcess(s2ref_unwrap((s2ref_t *)ctx->entries[3].v), 128+signo) )
    {
        return (struct value_nativeobj){
            .proper.l = 0,
            .type = (const void *)&type_nativeobj_long };
    }
    else
    {
        return (struct value_nativeobj){
            .proper.l = CXErrNS(GetLastError, ()),
            .type = (const void *)&type_nativeobj_null };
    }

    return CxingImpl_s2Obj_Final(argn, args);
}

struct value_nativeobj CxingImpl_ProcHndl_Terminate(
    int argn, struct value_nativeobj args[])
{
    return CxingImpl_ProcHndl_Kill0(argn, args, 15);
}

struct value_nativeobj CxingImpl_ProcHndl_Kill(
    int argn, struct value_nativeobj args[])
{
    return CxingImpl_ProcHndl_Kill0(argn, args, 9);
}

struct value_nativeobj CxingImpl_ProcHndl_Stop(
    int argn, struct value_nativeobj args[])
{
    (void)argn;
    (void)args;
    CxingDebug("SIGSTOP is neither supported nor emulated on Windows.\n");
    return (struct value_nativeobj){
        .proper.l = ENOSYS,
        .type = (const void *)&type_nativeobj_null };
}

struct value_nativeobj CxingImpl_ProcHndl_Continue(
    int argn, struct value_nativeobj args[])
{
    (void)argn;
    (void)args;
    CxingDebug("SIGCONTP is neither supported nor emulated on Windows.\n");
    return (struct value_nativeobj){
        .proper.l = ENOSYS,
        .type = (const void *)&type_nativeobj_null };
}

const type_nativeobj_struct_p8 type_nativeobj_ProcHndl = {
    .typeid = valtyp_obj,
    .n_entries = 8,
    .static_members = {
        { .name = "__get__", .member = &CxingValue_ProcHndl_Get },
        { .name = "__copy__", .member = &CxingValue_ProcHndl_Copy },
        { .name = "__final__", .member = &CxingValue_ProcHndl_Final },
        { .name = "Wait", .member = &CxingValue_ProcHndl_Wait },
        { .name = "Terminate", .member = &CxingValue_ProcHndl_Terminate },
        { .name = "Kill", .member = &CxingValue_ProcHndl_Kill },
        { .name = "Stop", .member = &CxingValue_ProcHndl_Stop },
        { .name = "Continue", .member = &CxingValue_ProcHndl_Continue },
    },
};

cxing_builtin_def_t CxingStdlibProcBuiltins[] = {
    { "CmdInterp", (struct value_nativeobj){
            .proper.p = CmdInterp,
            .type = (const void *)&type_nativeobj_subr } },

    {}
};
