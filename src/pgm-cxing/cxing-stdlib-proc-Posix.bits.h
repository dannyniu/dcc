/* DannyNiu/NJF, 2026-06-09. Public Domain. */

#include "cxing-stdlib-proc-common.bits.h"
#include "../infra/kvtab.h"
#include <fcntl.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;
extern void CxingImpl_FileRef_Close(FILE *fp);

void setcloexec(int fd)
{
    int flags = fcntl(fd, F_GETFD);
    flags |= FD_CLOEXEC;
    fcntl(fd, F_SETFD, flags);
}

struct value_nativeobj CxingImpl_CmdInterp_Exec(
    int argn, struct value_nativeobj args[])
{
    cxing_cmd_interp_t *ctx;
    size_t t;

    pid_t me;
    int subret;
    int infile[3]  = { -1, -1, 0 };
    int outfile[3] = { -1, -1, 0 };
    int errfile[3] = { -1, -1, 0 };

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

    // setup standard io channels.

    if( ctx->StdInHandling == StandardHandle_ObtainPipe )
    {
        infile[2] = pipe(infile);
    }

    if( ctx->StdOutHandling == StandardHandle_ObtainPipe && infile[2] == 0 )
    {
        outfile[2] = pipe(outfile);
    }

    if( ctx->StdErrHandling == StandardHandle_ObtainPipe && outfile[2] == 0 )
    {
        errfile[2] = pipe(errfile);
    }

    if( infile[2] != 0 || outfile[2] != 0 || errfile[2] != 0 )
    {
        subret = errno;
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

    if( ctx->StdInHandling == StandardHandle_ObtainPipe )
    {
        setcloexec(infile[1]);
    }

    if( ctx->StdOutHandling == StandardHandle_ObtainPipe )
    {
        setcloexec(outfile[0]);
    }

    if( ctx->StdErrHandling == StandardHandle_ObtainPipe )
    {
        setcloexec(errfile[0]);
    }

    // spawn process.

    me = fork();
    if( me > 0 )
    {
        // Create ProcHndl object and return it.

        FILE *fp;
        kvtab_t *ret = kvtab_create(4);

        if( !ret )
        {
            return (struct value_nativeobj){
                .proper.l = errno,
                .type = (const void *)&type_nativeobj_null };
        }
        s2obj_keep(ret->pobj);
        s2obj_release(ret->pobj);

        ret->entries[0].k = "infile";
        ret->entries[1].k = "outfile";
        ret->entries[2].k = "errfile";

        ret->entries[0].v = NULL;
        ret->entries[1].v = NULL;
        ret->entries[2].v = NULL;

        // The last one for PID of the child process.
        ret->count = 3;
        ret->entries[3].k = NULL;
        ret->entries[3].v = (void *)(intptr_t)me;

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
        }

        if( !fp )
        {
            s2obj_leave(ret->pobj);
            return (struct value_nativeobj){
                .proper.l = subret,
                .type = (const void *)&type_nativeobj_null };
        }

        return (struct value_nativeobj){
            .proper.p = ret,
            .type = (const void *)&type_nativeobj_ProcHndl };
    }
    else if( me == 0 )
    {
        // 2026-06-10:
        // At this point, it's the newborn process,
        // its failure isn't as critical to the
        // running CXING program that spawned it.
        // Error handling don't need to be as
        // comprehensive as before.

        // 2026-06-10:
        // Mimick `posix_spawnp` behavior with respect to this -
        // i.e. look in the working directory of child if there's
        // such entry in PATH and program is invoked by name.
        if( ctx->working_directory ) chdir(ctx->working_directory);

        if( ctx->StdInHandling == StandardHandle_ObtainPipe )
        {
            dup2(infile[0], 0);
            if( infile[0] != 0 ) close(infile[0]);
            close(infile[1]);
        }

        if( ctx->StdOutHandling == StandardHandle_ObtainPipe )
        {
            dup2(outfile[1], 1);
            if( outfile[1] != 1 ) close(outfile[1]);
            close(outfile[0]);
        }

        if( ctx->StdErrHandling == StandardHandle_ObtainPipe )
        {
            dup2(errfile[1], 2);
            if( errfile[1] != 2 ) close(errfile[1]);
            close(errfile[0]);
        }

        // 2026-06-10 TODO: Handle supplied files.
        // 2026-06-10:
        // So far, all files (specifically general files)
        // are: `s2ref_t<FILE *>` on top of file descriptors.
        // Sockets'll also be file descriptors on POSIX.
        // For Windows, it'll be another story.

        if( ctx->StdInHandling == StandardHandle_Supplied )
        {
            FILE *fp = s2ref_unwrap(ctx->StdInFp.proper.p);
            dup2(fileno(fp), 0);
        }

        if( ctx->StdOutHandling == StandardHandle_Supplied )
        {
            FILE *fp = s2ref_unwrap(ctx->StdOutFp.proper.p);
            dup2(fileno(fp), 1);
        }

        if( ctx->StdErrHandling == StandardHandle_Supplied )
        {
            FILE *fp = s2ref_unwrap(ctx->StdErrFp.proper.p);
            dup2(fileno(fp), 2);
        }

        if( !ctx->EnvP ) ctx->EnvP = environ;

        for(t=0; ctx->EnvP[t]; t++)
        {
            if( 0 == strncmp(ctx->EnvP[t], "PATH=", 5) )
                break;
        }

        // 2026-06-11:
        // Caller is supposed to provide PATH for finding binaries.
        // TODO: in case they don't, look for it in configs the hard way.
        assert( ctx->EnvP[t] );

        if( !strchr(ctx->ArgV[0], '/') )
        {
            // Called by name.
            const char *p = ctx->EnvP[t] + 5;
            s2data_t *path_builder = s2data_create(0);
            assert( path_builder );

            for(t=0; p[t]; t++)
            {
                if( p[t] == ':' )
                {
                    assert( 0 == s2data_putc(path_builder, '/') );
                    assert( 0 == s2data_puts(
                                path_builder,
                                ctx->ArgV[0],
                                strlen(ctx->ArgV[0])) );
                    assert( 0 == s2data_putfin(path_builder) );
                    execve(s2data_weakmap(path_builder),
                           ctx->ArgV, ctx->EnvP);

                    // A successful `exec` don't return.
                    // So try the next path component now.
                    assert( 0 == s2data_trunc(path_builder, 0) );
                    continue;
                }
                assert( 0 == s2data_putc(path_builder, p[t]) );
            }

            // The POSIX shell convention for command not found.
            _Exit(127);
            assert( 0 );
        }
        else
        {
            // Called by path, easier.

            execve(ctx->ArgV[0], ctx->ArgV, ctx->EnvP);
            if( errno == ENOENT || errno == ENOTDIR )
            {
                // The POSIX shell convention for command not found.
                _Exit(127);
            }
            else if( errno == ENOEXEC )
            {
                // The POSIX shell convention for incorrect format.
                _Exit(126);
            }
            else _Exit(1);
            assert( 0 );
        }
    }
    else
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }
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
    AssertArgN(2);
    AssertArgImpl(0, ProcHndl, "process handle");
    AssertArgImpl(1, s2impl_str, "string");

    ret = kvtab_get(
        args[0].proper.p,
        s2data_weakmap(args[1].proper.p));

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
    pid_t pid;
    int exit_stat;
    
    AssertArgN(1);
    AssertArgImpl(0, ProcHndl, "process handle");

    ctx = args[0].proper.p;
    pid = (pid_t)(intptr_t)ctx->entries[3].v;
    if( waitpid(pid, &exit_stat, 0) == pid )
    {
        return (struct value_nativeobj){
            .proper.l = WEXITSTATUS(exit_stat),
            .type = (const void *)&type_nativeobj_long };
    }
    else
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }
}

static struct value_nativeobj CxingImpl_ProcHndl_Kill0(
    int argn, struct value_nativeobj args[], int signo)
{
    kvtab_t *ctx;
    
    AssertArgN(1);
    AssertArgImpl(0, ProcHndl, "process handle");

    ctx = args[0].proper.p;
    if( 0 == kill((pid_t)(intptr_t)ctx->entries[3].v, signo) )
    {
        return (struct value_nativeobj){
            .proper.l = 0,
            .type = (const void *)&type_nativeobj_long };
    }
    else
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    return CxingImpl_s2Obj_Final(argn, args);
}

struct value_nativeobj CxingImpl_ProcHndl_Terminate(
    int argn, struct value_nativeobj args[])
{
    return CxingImpl_ProcHndl_Kill0(argn, args, SIGTERM);
}

struct value_nativeobj CxingImpl_ProcHndl_Kill(
    int argn, struct value_nativeobj args[])
{
    return CxingImpl_ProcHndl_Kill0(argn, args, SIGKILL);
}

struct value_nativeobj CxingImpl_ProcHndl_Stop(
    int argn, struct value_nativeobj args[])
{
    return CxingImpl_ProcHndl_Kill0(argn, args, SIGSTOP);
}

struct value_nativeobj CxingImpl_ProcHndl_Continue(
    int argn, struct value_nativeobj args[])
{
    return CxingImpl_ProcHndl_Kill0(argn, args, SIGCONT);
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
