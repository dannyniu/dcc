/* DannyNiu/NJF, 2026-01-24. Public Domain. */

#include "cxing-stdlib.h"
#include <fcntl.h>
#include <unistd.h>

struct value_nativeobj CxingStdlibFunc_Print(
    int argn, struct value_nativeobj args[])
{
    s2data_t *x;
    size_t ret;

    if( argn < 1 )
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };

    if( args[0].type->typeid == valtyp_long )
    {
        ret = printf("%lld\n", (long long)args[0].proper.l);
    }

    else if( args[0].type->typeid == valtyp_ulong )
    {
        ret = printf("%llu\n", (unsigned long long)args[0].proper.u);
    }

    else if( args[0].type->typeid == valtyp_double )
    {
        ret = printf("%f\n", args[0].proper.f);
    }

    else if( args[0].type == (const void *)&type_nativeobj_s2impl_str )
    {
        x = args[0].proper.p;
        ret = fwrite(s2data_weakmap(x), 1, s2data_len(x), stdout);
        ret += putchar('\n') != EOF;
    }

    else if( IsNull(args[0]) )
    {
        ret = printf("<null/%llu/%llu>\n",
                     args[0].type->typeid,
                     args[0].proper.u);
    }

    else
    {
        ret = printf("<%p/%s>\n",
                     args[0].proper.p,
                     args[0].type->typeid == valtyp_obj ? "obj" :
                     args[0].type->typeid == valtyp_subr ? "subr" :
                     args[0].type->typeid == valtyp_method ? "method" :
                     "-unknown-");
    }

    return (struct value_nativeobj){
        .proper.u = ret,
        .type = (const void *)&type_nativeobj_ulong };
}

struct value_nativeobj CxingStdlibFunc_Input(
    int argn, struct value_nativeobj args[])
{
    s2data_t *x = s2data_create(0);
    if( !x || feof(stdin) )
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };

    s2obj_keep(x->pobj);
    s2obj_release(x->pobj);

    (void)argn;
    (void)args;

    // After this point, for the moment ignore ENOMEM, as
    // the next `input` call will result in a `null`.

    while( true )
    {
        int c = getchar();
        if( c < 0 || c == '\n' )
        {
            s2data_putfin(x);
            return (struct value_nativeobj){
                .proper.p = x,
                .type = (const void *)&type_nativeobj_s2impl_str };
        }
        else if( c == '\r' )
        {
            c = getchar();
            if( c == '\n' )
            {
                s2data_putfin(x);
                return (struct value_nativeobj){
                    .proper.p = x,
                    .type = (const void *)&type_nativeobj_s2impl_str };
            }
            else
            {
                s2data_putc(x, '\r');
                s2data_putc(x, c);
            }
        }
        else s2data_putc(x, c);
    }
}

#define Stdio_MethodImpl(stdtype, name)                         \
    struct value_nativeobj CxingImpl_##stdtype##_##name(        \
        int argn, struct value_nativeobj args[]);               \
    struct value_nativeobj CxingValue_##stdtype##_##name =      \
        (struct value_nativeobj){                               \
        .proper.p = CxingImpl_##stdtype##_##name,               \
        .type = (const void *)&type_nativeobj_method };

Stdio_MethodImpl(RegFile, Read);
Stdio_MethodImpl(RegFile, GetDelim);
Stdio_MethodImpl(RegFile, GetLine);
Stdio_MethodImpl(RegFile, Write);
Stdio_MethodImpl(RegFile, Copy);
Stdio_MethodImpl(RegFile, Final);
Stdio_MethodImpl(RegFile, Flush);
Stdio_MethodImpl(RegFile, SetSync);
Stdio_MethodImpl(RegFile, LSeek);

// GenFile assumes implementation based on `s2ref_t<FILE *>`.

struct value_nativeobj CxingImpl_GenFile_Read(
    int argn, struct value_nativeobj args[])
{
    s2data_t *ret = s2data_create((size_t)args[1].proper.u);
    if( !ret )
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };

    s2obj_keep(ret->pobj);
    s2obj_release(ret->pobj);

    (void)argn;

    s2data_trunc(
        ret,
        fread(
            s2data_weakmap(ret),
            1, args[1].proper.u,
            s2ref_unwrap(args[0].proper.p)));

    return (struct value_nativeobj){
        .proper.p = ret,
        .type = (const void *)&type_nativeobj_s2impl_str };
}

struct value_nativeobj CxingImpl_GenFile_GetDelim(
    int argn, struct value_nativeobj args[])
{
    s2data_t *ret = NULL;
    FILE *fp = NULL;
    int c, d;

    (void)argn;

    ret = s2data_create(0);
    if( !ret )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    s2obj_keep(ret->pobj);
    s2obj_release(ret->pobj);

    if( IsInteger(args[1]) )
    {
        d = args[1].proper.u & 255;
    }
    else
    {
        // assume it must be a string,
        uint8_t *p = s2data_weakmap(args[1].proper.p);
        d = p[0];
    }

    fp = s2ref_unwrap(args[0].proper.p);

    while( (c = fgetc(fp)) >= 0 )
    {
        s2data_putc(ret, c);
        if( c == d ) break;
    }

    s2data_putfin(ret);
    return (struct value_nativeobj){
        .proper.p = ret,
        .type = (const void *)&type_nativeobj_s2impl_str };
}

struct value_nativeobj CxingImpl_GenFile_GetLine(
    int argn, struct value_nativeobj args[])
{
    struct value_nativeobj args_passdown[2] = {
        [0] = args[0],
        [1].proper.l = '\n',
        [1].type = (const void *)&type_nativeobj_long };

    (void)argn;
    return CxingImpl_GenFile_GetDelim(
        2, args_passdown);
}

struct value_nativeobj CxingImpl_GenFile_Write(
    int argn, struct value_nativeobj args[])
{
    const void *dat = s2data_weakmap(args[1].proper.p);
    size_t len = s2data_len(args[1].proper.p);
    size_t ret = fwrite(dat, 1, len, s2ref_unwrap(args[0].proper.p));

    (void)argn;

    if( ret < len )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_long };
    }
    else
    {
        return (struct value_nativeobj){
            .proper.u = ret,
            .type = (const void *)&type_nativeobj_ulong };
    }
}

struct value_nativeobj CxingImpl_GenFile_Flush(
    int argn, struct value_nativeobj args[])
{
    (void)argn;
    if( fflush(s2ref_unwrap(args[0].proper.p)) == 0 )
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
}

struct value_nativeobj CxingImpl_GenFile_SetSync(
    int argn, struct value_nativeobj args[])
{
#ifdef _WIN32
    (void)argn;
    (void)args;
    CxingWarning("The `setsync` method had not been implemented for Windows (yet).");
    return (struct value_nativeobj){
        .proper.l = EOPNOTSUPP,
        .type = (const void *)&type_nativeobj_null };
#else
    int fd = fileno(s2ref_unwrap(args[0].proper.p));
    int sn = ValueNativeObj2Logic(args[1]);
    int fl = fcntl(fd, F_GETFL);

    (void)argn;

    if( sn )
    {
        fl |= O_SYNC;
    }
    else
    {
        fl &= ~O_SYNC;
    }

    if( fcntl(fd, F_SETFL, fl) != -1 )
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
#endif
}

static void CxingImpl_GenFile_Close(FILE *fp)
{
    fclose(fp);
}

// RegFile are openned by `open`.

struct value_nativeobj CxingImpl_RegFile_Read(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, RegFile, "RegularFile");
    if( !IsInteger(args[1]) )
    {
        CxingDebug("The length argument should be an integer "
                   " for `read` method.\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    return CxingImpl_GenFile_Read(argn, args);
}

struct value_nativeobj CxingImpl_RegFile_GetDelim(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(2);
    AssertArgImpl(0, RegFile, "RegularFile");
    if( !IsInteger(args[1]) )
        AssertArgImpl(1, s2impl_str, "string");

    return CxingImpl_GenFile_GetDelim(argn, args);
}

struct value_nativeobj CxingImpl_RegFile_GetLine(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, RegFile, "RegularFile");

    return CxingImpl_GenFile_GetLine(argn, args);
}

struct value_nativeobj CxingImpl_RegFile_Write(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(2);
    AssertArgImpl(0, RegFile, "RegularFile");
    AssertArgImpl(1, s2impl_str, "string");

    return CxingImpl_GenFile_Write(argn, args);
}

struct value_nativeobj CxingImpl_RegFile_Copy(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, RegFile, "RegularFile");
    return CxingImpl_s2Obj_Copy(argn, args);
}

struct value_nativeobj CxingImpl_RegFile_Final(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, RegFile, "RegularFile");
    return CxingImpl_s2Obj_Final(argn, args);
}

struct value_nativeobj CxingImpl_RegFile_Flush(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, RegFile, "RegularFile");

    return CxingImpl_GenFile_Flush(argn, args);
}

struct value_nativeobj CxingImpl_RegFile_SetSync(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(2);
    AssertArgImpl(0, RegFile, "RegularFile");
    // Allow developer to use whatever Boolean value they like _for now_.

    return CxingImpl_GenFile_SetSync(argn, args);
}

struct value_nativeobj CxingImpl_RegFile_LSeek(
    int argn, struct value_nativeobj args[])
{
    int whence = -1;
    int64_t ret;
    static const int aSET = ('S'-'A')*512 + ('E'-'A')*64 + ('T'-'A');
    static const int aCUR = ('C'-'A')*512 + ('U'-'A')*64 + ('R'-'A');
    static const int aEND = ('E'-'A')*512 + ('N'-'A')*64 + ('D'-'A');

    AssertArgN(3);
    AssertArgImpl(0, RegFile, "RegularFile");
    if( !IsInteger(args[1]) )
    {
        CxingDebug("The `offset` argument should be an integer "
                   " for `lseek` method.\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }
    if( !IsInteger(args[2]) )
    {
        CxingDebug("The `whence` argument should be an integer "
                   " for `lseek` method.\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    whence = args[2].proper.l;
    if( whence == aSET ) whence = SEEK_SET;
    else if( whence == aCUR ) whence = SEEK_CUR;
    else if( whence == aEND ) whence = SEEK_END;

    if( fseeko(args[0].proper.p, args[1].proper.l, whence) != 0 )
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };

    if( (ret = ftello(args[0].proper.p)) == -1 )
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };


    return (struct value_nativeobj){
        .proper.l = ret,
        .type = (const void *)&type_nativeobj_long };
}

const type_nativeobj_struct_p10 type_nativeobj_RegFile = {
    .typeid = valtyp_obj,
    .n_entries = 9,
    .static_members = {
        { .name = "read", .member = &CxingValue_RegFile_Read },
        { .name = "getdelim", .member = &CxingValue_RegFile_GetDelim },
        { .name = "getline", .member = &CxingValue_RegFile_GetLine },
        { .name = "write", .member = &CxingValue_RegFile_Write },
        { .name = "__copy__", .member = &CxingValue_RegFile_Copy },
        { .name = "__final__", .member = &CxingValue_RegFile_Final },
        { .name = "flush", .member = &CxingValue_RegFile_Flush },
        { .name = "setsync", .member = &CxingValue_RegFile_SetSync },
        { .name = "lseek", .member = &CxingValue_RegFile_LSeek },
    },
};

struct value_nativeobj CxingImpl_RegFile_Open(
    int argn, struct value_nativeobj args[])
{
    s2ref_t *ret;
    FILE *fp;
    int fd;
    int mode = 0;
    char modes[8] = {};
    uint64_t um;

    AssertArgN(2);
    AssertArgImpl(0, s2impl_str, "string");
    if( !IsInteger(args[1]) )
    {
        CxingDebug("The 2nd argument - `mode` should be a base64 integer.\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    um = args[1].proper.u;
    while( um )
    {
        int m = um & 63;

        if( m == 26 + 'r' - 'a' )
            mode = O_RDONLY;
        else if( m == 26 + 'w' - 'a' )
            mode = O_WRONLY | O_CREAT | O_TRUNC;
        else if( m == 'R' - 'A' )
            mode = O_RDWR;
        else if( m == 'W' - 'A' )
            mode = O_RDWR | O_CREAT | O_TRUNC;
        else if( m == 26 ) // value of lower-case 'a' in base64 alphabet.
            mode |= O_APPEND;
        else if( m == 26 + 'e' - 'a' )
        {
#ifdef _WIN32
            mode |= O_NOINHERIT;
#else // assume POSIX.
            mode |= O_CLOEXEC;
#endif // _WIN32
        }
        else if( m == 26 + 'x' - 'a' )
            mode |= O_EXCL;

        um >>= 6;
    }

#ifdef _WIN32
    mode |= O_BINARY;
#endif // _WIN32

    if( (O_RDONLY) == (O_RDONLY & mode) )
        strcpy(modes, "rb");

    else if( (O_WRONLY | O_CREAT | O_TRUNC) ==
             ((O_WRONLY | O_CREAT | O_TRUNC) & mode) )
        strcpy(modes, "wb");

    else if( (O_WRONLY | O_CREAT | O_APPEND) ==
             ((O_WRONLY | O_CREAT | O_APPEND) & mode) )
        strcpy(modes, "ab");

    else if( (O_RDWR) == (O_RDWR & mode) )
        strcpy(modes, "r+b");

    else if( (O_RDWR | O_CREAT | O_TRUNC) ==
             ((O_RDWR | O_CREAT | O_TRUNC) & mode) )
        strcpy(modes, "w+b");

    else if( (O_RDWR | O_CREAT | O_APPEND) ==
             ((O_RDWR | O_CREAT | O_APPEND) & mode) )
        strcpy(modes, "a+b");

    fd = open(s2data_weakmap(args[0].proper.p), mode);
    if( fd < 0 )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    fp = fdopen(fd, modes);
    if( !fp )
    {
        close(fd);
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    ret = s2ref_create(
        fp, (s2ref_final_func_t)CxingImpl_GenFile_Close);
    s2obj_keep(ret->pobj);
    s2obj_release(ret->pobj);

    if( !ret )
    {
        fclose(fp);
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    return (struct value_nativeobj){
        .proper.p = ret,
        .type = (const void *)&type_nativeobj_RegFile };
}
