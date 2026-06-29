/* DannyNiu/NJF, 2026-06-22. Public Domain. */

#include "cxing-stdlib.h"
#include <fcntl.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

socklen_t SockAddrLen(struct sockaddr *backing)
{
    switch( backing->sa_family )
    {
    case AF_INET: return sizeof(struct sockaddr_in);
    case AF_INET6: return sizeof(struct sockaddr_in6);
    }

    return 0;
}

// TODO 2026-06-22: All of these. Plan first.

CxingMethodValueWithImpl(Socket, Read);
CxingMethodValueWithImpl(Socket, GetDelim);
CxingMethodValueWithImpl(Socket, GetLine);
CxingMethodValueWithImpl(Socket, Write);
CxingMethodValueWithImpl(Socket, Copy);
CxingMethodValueWithImpl(Socket, Final);
CxingMethodValueWithImpl(Socket, Flush);
CxingMethodValueWithImpl(Socket, Close);
CxingMethodValueWithImpl(Socket, SetSync);

CxingMethodValueWithImpl(Socket, Send);
CxingMethodValueWithImpl(Socket, Recv);
CxingMethodValueWithImpl(Socket, SendTo);
CxingMethodValueWithImpl(Socket, RecvFrom);
CxingMethodValueWithImpl(Socket, Shutdown);
CxingMethodValueWithImpl(Socket, Bind);
CxingMethodValueWithImpl(Socket, Connect);
CxingMethodValueWithImpl(Socket, Listen);
CxingMethodValueWithImpl(Socket, Accept);

CxingMethodValueWithImpl(Socket, GetConfig);
CxingMethodValueWithImpl(Socket, SetConfig);

// GenFile methods of Sockets assumes implementation based on
// - `s2ref_t<int>` for POSIX.
// - `s2ref_t<HANDLE>` on Win32.

#ifdef _WIN32

#error TODO: Define `CxSockErr()` function-like macro..
static void CxingImpl_Socket_Close0(void *socket)
{
    closesocket(socket);
}

#else // Assume POSIX.

#define CxSockErr() errno
static void CxingImpl_Socket_Close0(void *socket)
{
    close((intptr_t)socket);
}

#endif // _WIN32

#define SocketThis (intptr_t)s2ref_unwrap(args[0].proper.p)

struct value_nativeobj CxingImpl_Socket_Read(
    int argn, struct value_nativeobj args[])
{
    s2data_t *ret;
    ssize_t sret;
    AssertArgN(2);
    AssertArgImpl(0, Socket, "socket");

    if( !IsInteger(args[1]) )
    {
        CxingDebug("The `length` argument should be an integer "
                   "for `read` method.\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    ret = s2data_create((size_t)args[1].proper.u);
    if( !ret )
        return (struct value_nativeobj){
            .proper.l = CxSockErr(),
            .type = (const void *)&type_nativeobj_null };

    s2obj_keep(ret->pobj);
    s2obj_release(ret->pobj);

    sret = recv(SocketThis, s2data_weakmap(ret), args[1].proper.u, 0);

    if( sret >= 0 )
    {
        s2data_trunc(ret, sret);
        return (struct value_nativeobj){
            .proper.p = ret,
            .type = (const void *)&type_nativeobj_s2impl_str };
    }
    else
    {
        sret = CxSockErr();
        s2obj_leave(ret->pobj);
        return (struct value_nativeobj){
            .proper.l = sret,
            .type = (const void *)&type_nativeobj_null };
    }
}

struct value_nativeobj CxingImpl_Socket_GetDelim(
    int argn, struct value_nativeobj args[])
{
    s2data_t *ret = NULL;
    int c, d;

    // 2026-06-23 TODO:
    // How to warn the developer when it's not a stream socket?

    AssertArgN(2);
    AssertArgImpl(0, Socket, "socket");

    if( !IsInteger(args[1]) )
        AssertArgImpl(1, s2impl_str, "string");

    ret = s2data_create(0);
    if( !ret )
        return (struct value_nativeobj){
            .proper.l = CxSockErr(),
            .type = (const void *)&type_nativeobj_null };

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

    while( true )
    {
        uint8_t buf[64], *eptr;
        ssize_t sret = recv(SocketThis, buf, sizeof buf, MSG_PEEK);

        // ret.len == 0:
        // - sret < 0: cast back errno.
        // - sret == 0: return ret.
        // - sret > 0: puts,
        // ret.len > 0:
        // - sret < 0: return ret,
        // - sret == 0: return ret,
        // - sret > 0 puts.
        // But don't return 'rotten' strings.
        // The 'rotten semantic' is a considered concept in the spec,
        // but had not yet been finalized or formalized as of 2026-06-22.
        // (helping note from 2026-06-22).

        if( s2data_len(ret) == 0 )
        {
            if( sret < 0 )
            {
                c = CxSockErr();
                s2obj_leave(ret->pobj);
                return (struct value_nativeobj){
                    .proper.l = c,
                    .type = (const void *)&type_nativeobj_null };
            }
        }

        if( sret == 0 )
        {
            if( 0 != s2data_putfin(ret) )
            {
                c = CxSockErr();
                s2obj_leave(ret->pobj);
                return (struct value_nativeobj){
                    .proper.l = c,
                    .type = (const void *)&type_nativeobj_null };
            }

            // EOF at this point.
            return (struct value_nativeobj){
                .proper.p = ret,
                .type = (const void *)&type_nativeobj_s2impl_str };
        }

        if( !(eptr = memchr(buf, d, sret)) )
        {
            if( 0 != s2data_puts(ret, buf, sret) )
            {
                c = CxSockErr();
                s2obj_leave(ret->pobj);
                return (struct value_nativeobj){
                    .proper.l = c,
                    .type = (const void *)&type_nativeobj_null };
            }

            // hasn't encountered the delimitor or EOF yet.
            recv(SocketThis, buf, sret, 0); // 2026-06-23: Perhaps assert `== sret`?
        }

        else
        {
            // calculate the length first.
            sret = 1 + eptr - buf;

            // then try grow and stuff the string.
            if( 0 != s2data_puts(ret, buf, sret) ||
                0 != s2data_putfin(ret) )
            {
                // the string rotted,
                c = CxSockErr();
                s2obj_leave(ret->pobj);
                return (struct value_nativeobj){
                    .proper.l = c,
                    .type = (const void *)&type_nativeobj_null };
            }

            // commit the IO operation by removing the peeked segment.
            // ignore any error at this point, and there shouldn't be any.
            recv(SocketThis, buf, sret, 0); // 2026-06-23: Perhaps assert `== sret`?

            return (struct value_nativeobj){
                .proper.p = ret,
                .type = (const void *)&type_nativeobj_s2impl_str };
        }
    }
}

struct value_nativeobj CxingImpl_Socket_GetLine(
    int argn, struct value_nativeobj args[])
{
    struct value_nativeobj args_passdown[2] = {
        [0] = args[0],
        [1].proper.l = '\n',
        [1].type = (const void *)&type_nativeobj_long };

    AssertArgN(1);
    AssertArgImpl(0, Socket, "socket");

    (void)argn;
    return CxingImpl_Socket_GetDelim(
        2, args_passdown);
}

struct value_nativeobj CxingImpl_Socket_Write(
    int argn, struct value_nativeobj args[])
{
    s2data_t *buf;
    ssize_t sret;
    AssertArgN(2);
    AssertArgImpl(0, Socket, "socket");
    AssertArgImpl(1, s2impl_str, "string");

    buf = args[1].proper.p;
    sret = send(SocketThis, s2data_weakmap(buf), s2data_len(buf), 0);

    if( sret >= 0 )
    {
        return (struct value_nativeobj){
            .proper.l = sret,
            .type = (const void *)&type_nativeobj_long };
    }
    else
    {
        return (struct value_nativeobj){
            .proper.l = CxSockErr(),
            .type = (const void *)&type_nativeobj_null };
    }
}

struct value_nativeobj CxingImpl_Socket_Flush(
    int argn, struct value_nativeobj args[])
{
    (void)argn;
    (void)args;
    AssertArgN(1);
    AssertArgImpl(0, Socket, "socket");
    // This is really a nop for sockets.
    return (struct value_nativeobj){
        .proper.l = 0,
        .type = (const void *)&type_nativeobj_long };
}

struct value_nativeobj CxingImpl_Socket_Close(
    int argn, struct value_nativeobj args[])
{
    s2ref_t *fr;
    AssertArgN(1);
    AssertArgImpl(0, Socket, "socket");

    fr = args[0].proper.p;
#ifdef _WIN32
    closesocket(SocketThis);
#else // Assume POSIX.
    close(SocketThis);
#endif // _WIN32
    fr->ptr = NULL;
    fr->finalizer = NULL;

    return (struct value_nativeobj){
        .proper.l = 0,
        .type = (const void *)&type_nativeobj_long };
}

struct value_nativeobj CxingImpl_Socket_SetSync(
    int argn, struct value_nativeobj args[])
{
#ifdef _WIN32
    SOCKET fd;
    DWORD sn;
#else // assume POSIX.
    int fd;
    int sn;
#endif // _WIN32

    AssertArgN(2);
    AssertArgImpl(0, Socket, "socket");
    if( !IsInteger(args[1]) )
    {
        CxingDebug("The argument should be a Boolean "
                   "for `setsync` method.\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    fd = SocketThis;
    sn = !ValueNativeObj2Logic(args[1]);

    if( setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &sn, sizeof sn) == 0 )
    {
        return (struct value_nativeobj){
            .proper.l = 0,
            .type = (const void *)&type_nativeobj_long };
    }
    else
    {
        return (struct value_nativeobj){
            .proper.l = CxSockErr(),
            .type = (const void *)&type_nativeobj_null };
    }
}

struct value_nativeobj CxingImpl_Socket_Send(
    int argn, struct value_nativeobj args[])
{
    s2data_t *buf;
    ssize_t sret;
    AssertArgN(3);
    AssertArgImpl(0, Socket, "socket");
    AssertArgImpl(1, s2impl_str, "string");

    if( !IsInteger(args[2]) )
    {
        CxingDebug("The `flags` argument should be an integer "
                   "for `send` method.\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    buf = args[1].proper.p;
    sret = send(SocketThis, s2data_weakmap(buf),
                s2data_len(buf), args[2].proper.l);

    if( sret >= 0 )
    {
        return (struct value_nativeobj){
            .proper.l = sret,
            .type = (const void *)&type_nativeobj_long };
    }
    else
    {
        return (struct value_nativeobj){
            .proper.l = CxSockErr(),
            .type = (const void *)&type_nativeobj_null };
    }
}

struct value_nativeobj CxingImpl_Socket_Recv(
    int argn, struct value_nativeobj args[])
{
    s2data_t *ret;
    ssize_t sret;
    AssertArgN(3);
    AssertArgImpl(0, Socket, "socket");

    if( !IsInteger(args[1]) )
    {
        CxingDebug("The `length` argument should be an integer "
                   "for `read` method.\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    if( !IsInteger(args[2]) )
    {
        CxingDebug("The `flags` argument should be an integer "
                   "for `recv` method.\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    ret = s2data_create((size_t)args[1].proper.u);
    if( !ret )
        return (struct value_nativeobj){
            .proper.l = CxSockErr(),
            .type = (const void *)&type_nativeobj_null };

    s2obj_keep(ret->pobj);
    s2obj_release(ret->pobj);

    sret = recv(SocketThis, s2data_weakmap(ret),
                args[1].proper.u, args[2].proper.l);

    if( sret >= 0 )
    {
        s2data_trunc(ret, sret);
        return (struct value_nativeobj){
            .proper.p = ret,
            .type = (const void *)&type_nativeobj_s2impl_str };
    }
    else
    {
        sret = CxSockErr();
        s2obj_leave(ret->pobj);
        return (struct value_nativeobj){
            .proper.l = sret,
            .type = (const void *)&type_nativeobj_null };
    }
}

// TODO: `sendto` and `recvfrom` - was blocked on `sockaddr`.

struct value_nativeobj CxingImpl_Socket_SendTo(
    int argn, struct value_nativeobj args[])
{
    s2data_t *buf;
    s2data_t *peer;
    ssize_t sret;
    AssertArgN(4);
    AssertArgImpl(0, Socket, "socket");
    AssertArgImpl(1, s2impl_str, "string");
    AssertArgImpl(3, SockAddr, "socket address");

    if( !IsInteger(args[2]) )
    {
        CxingDebug("The `flags` argument should be an integer "
                   "for `sendto` method.\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    buf = args[1].proper.p;
    peer = args[3].proper.p;
    sret = sendto(SocketThis, s2data_weakmap(buf),
                  s2data_len(buf), args[2].proper.l,
                  s2data_weakmap(peer), s2data_len(peer));

    if( sret >= 0 )
    {
        return (struct value_nativeobj){
            .proper.l = sret,
            .type = (const void *)&type_nativeobj_long };
    }
    else
    {
        return (struct value_nativeobj){
            .proper.l = CxSockErr(),
            .type = (const void *)&type_nativeobj_null };
    }
}

struct value_nativeobj CxingImpl_Socket_RecvFrom(
    int argn, struct value_nativeobj args[])
{
    s2data_t *ret;
    s2data_t *peer;
    socklen_t salen;
    ssize_t sret;
    AssertArgN(4);
    AssertArgImpl(0, Socket, "socket");
    AssertArgImpl(3, SockAddr, "socket address");

    if( !IsInteger(args[1]) )
    {
        CxingDebug("The `length` argument should be an integer "
                   "for `read` method.\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    if( !IsInteger(args[2]) )
    {
        CxingDebug("The `flags` argument should be an integer "
                   "for `recvfrom` method.\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    ret = s2data_create((size_t)args[1].proper.u);
    if( !ret )
        return (struct value_nativeobj){
            .proper.l = CxSockErr(),
            .type = (const void *)&type_nativeobj_null };

    s2obj_keep(ret->pobj);
    s2obj_release(ret->pobj);

    peer = args[3].proper.p;
    salen = s2data_len(peer);
    sret = recvfrom(SocketThis, s2data_weakmap(ret),
                    args[1].proper.u, args[2].proper.l,
                    s2data_weakmap(peer), &salen);

    if( sret >= 0 )
    {
        s2data_trunc(ret, sret);
        return (struct value_nativeobj){
            .proper.p = ret,
            .type = (const void *)&type_nativeobj_s2impl_str };
    }
    else
    {
        sret = CxSockErr();
        s2obj_leave(ret->pobj);
        return (struct value_nativeobj){
            .proper.l = sret,
            .type = (const void *)&type_nativeobj_null };
    }
}

struct value_nativeobj CxingImpl_Socket_Shutdown(
    int argn, struct value_nativeobj args[])
{
    ssize_t sret;
    AssertArgN(2);
    AssertArgImpl(0, Socket, "socket");

    if( !IsInteger(args[1]) )
    {
        CxingDebug("The `how` argument should be an integer "
                   "for `shutdown` method.\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    sret = shutdown(SocketThis, args[2].proper.l);

    if( sret == 0 )
    {
        return (struct value_nativeobj){
            .proper.l = sret,
            .type = (const void *)&type_nativeobj_long };
    }
    else
    {
        return (struct value_nativeobj){
            .proper.l = CxSockErr(),
            .type = (const void *)&type_nativeobj_null };
    }
}

struct value_nativeobj CxingImpl_Socket_Bind(
    int argn, struct value_nativeobj args[])
{
    ssize_t sret;
    s2data_t *sa;
    struct sockaddr *backing;

    AssertArgN(2);
    AssertArgImpl(0, Socket, "socket");
    AssertArgImpl(1, SockAddr, "socket address");

    sa = args[1].proper.p;
    backing = s2data_weakmap(sa);
    sret = bind(SocketThis, backing, SockAddrLen(backing));

    if( sret == 0 )
    {
        return (struct value_nativeobj){
            .proper.l = sret,
            .type = (const void *)&type_nativeobj_long };
    }
    else
    {
        return (struct value_nativeobj){
            .proper.l = CxSockErr(),
            .type = (const void *)&type_nativeobj_null };
    }
}

struct value_nativeobj CxingImpl_Socket_Connect(
    int argn, struct value_nativeobj args[])
{
    ssize_t sret;
    s2data_t *sa;
    struct sockaddr *backing;

    AssertArgN(2);
    AssertArgImpl(0, Socket, "socket");
    AssertArgImpl(1, SockAddr, "socket address");

    sa = args[1].proper.p;
    backing = s2data_weakmap(sa);
    sret = connect(SocketThis, backing, SockAddrLen(backing));

    if( sret == 0 )
    {
        return (struct value_nativeobj){
            .proper.l = sret,
            .type = (const void *)&type_nativeobj_long };
    }
    else
    {
        return (struct value_nativeobj){
            .proper.l = CxSockErr(),
            .type = (const void *)&type_nativeobj_null };
    }
}

struct value_nativeobj CxingImpl_Socket_Listen(
    int argn, struct value_nativeobj args[])
{
    ssize_t sret;

    if( argn < 2 )
    {
        CxingDebug("Note that the `backlog` argument "
                   "is required for listening sockets!\n");
    }

    AssertArgN(2);
    AssertArgImpl(0, Socket, "socket");

    if( !IsInteger(args[1]) )
    {
        CxingDebug("The `backlog` argument should be an integer "
                   "for `listen` method.\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    sret = listen(SocketThis, args[1].proper.l);

    if( sret == 0 )
    {
        return (struct value_nativeobj){
            .proper.l = sret,
            .type = (const void *)&type_nativeobj_long };
    }
    else
    {
        return (struct value_nativeobj){
            .proper.l = CxSockErr(),
            .type = (const void *)&type_nativeobj_null };
    }
}

struct value_nativeobj CxingImpl_Socket_Accept(
    int argn, struct value_nativeobj args[])
{
    s2ref_t *RefCntSocket;
    s2data_t *sa = NULL;
    socklen_t sz = 0;
    void *backing = NULL;
    int oobinline = true;
    AssertArgN(1);
    AssertArgImpl(0, Socket, "socket");

    if( !(RefCntSocket = s2ref_create(NULL, NULL)) )
    {
        return (struct value_nativeobj){
            .proper.l = CxSockErr(),
            .type = (const void *)&type_nativeobj_null };
    }

    if( argn >= 2 )
    {
        AssertArgImpl(1, SockAddr, "socket address");
        sa = args[1].proper.p;
        sz = s2data_len(sa);
        backing = s2data_weakmap(sa);
    }

    RefCntSocket->ptr = (void *)(intptr_t)accept(
        SocketThis, backing, sa ? &sz : NULL);

#ifdef _WIN32
    if( RefCntSocket->ptr == INVALID_SOCKET )
    {
        return (struct value_nativeobj){
            .proper.l = CxSockErr(),
            .type = (const void *)&type_nativeobj_null };
    }
#else // Assume POSIX.
    if( (intptr_t)RefCntSocket->ptr == -1 )
    {
        return (struct value_nativeobj){
            .proper.l = CxSockErr(),
            .type = (const void *)&type_nativeobj_null };
    }
#endif // _WIN32

    setsockopt((intptr_t)RefCntSocket->ptr,
               SOL_SOCKET, SO_OOBINLINE,
               &oobinline, sizeof(oobinline));

    RefCntSocket->finalizer = (s2ref_final_func_t)CxingImpl_Socket_Close0;
    return (struct value_nativeobj){
        .proper.p = RefCntSocket,
        .type = (const void *)&type_nativeobj_Socket };
}

struct value_nativeobj CxingImpl_Socket_Create(
    int argn, struct value_nativeobj args[])
{
    s2ref_t *RefCntSocket;
    int oobinline = true;
    AssertArgN(3);

    if( !IsInteger(args[0]) )
    {
        CxingDebug("The `domain` argument should be an integer "
                   "when creating a socket.\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    if( !IsInteger(args[1]) )
    {
        CxingDebug("The `type` argument should be an integer "
                   "when creating a socket.\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    if( !IsInteger(args[2]) )
    {
        CxingDebug("The `protocol` argument should be an integer "
                   "when creating a socket.\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    if( !(RefCntSocket = s2ref_create(NULL, NULL)) )
    {
        return (struct value_nativeobj){
            .proper.l = CxSockErr(),
            .type = (const void *)&type_nativeobj_null };
    }

    RefCntSocket->ptr = (void *)(intptr_t)socket(
        args[0].proper.l,
        args[1].proper.l,
        args[2].proper.l);

#ifdef _WIN32
    if( RefCntSocket->ptr == INVALID_SOCKET )
    {
        return (struct value_nativeobj){
            .proper.l = CxSockErr(),
            .type = (const void *)&type_nativeobj_null };
    }
#else // Assume POSIX.
    if( (intptr_t)RefCntSocket->ptr == -1 )
    {
        return (struct value_nativeobj){
            .proper.l = CxSockErr(),
            .type = (const void *)&type_nativeobj_null };
    }
#endif // _WIN32

    setsockopt((intptr_t)RefCntSocket->ptr,
               SOL_SOCKET, SO_OOBINLINE,
               &oobinline, sizeof(oobinline));

    RefCntSocket->finalizer = (s2ref_final_func_t)CxingImpl_Socket_Close0;
    return (struct value_nativeobj){
        .proper.p = RefCntSocket,
        .type = (const void *)&type_nativeobj_Socket };
}

struct value_nativeobj CxingImpl_Socket_Copy(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, Socket, "socket");

    s2obj_retain(args[0].proper.p);
    return args[0];
}

struct value_nativeobj CxingImpl_Socket_Final(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, Socket, "socket");

    s2obj_release(args[0].proper.p);
    return (struct value_nativeobj){
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth };
}

const type_nativeobj_struct_p20 type_nativeobj_Socket = {
    .typeid = valtyp_obj,
    .n_entries = 20,
    .static_members = {
        { .name = "read", .member = &CxingValue_Socket_Read },
        { .name = "getdelim", .member = &CxingValue_Socket_GetDelim },
        { .name = "getline", .member = &CxingValue_Socket_GetLine },
        { .name = "write", .member = &CxingValue_Socket_Write },
        { .name = "__copy__", .member = &CxingValue_Socket_Copy },
        { .name = "__final__", .member = &CxingValue_Socket_Final },
        { .name = "flush", .member = &CxingValue_Socket_Flush },
        { .name = "close", .member = &CxingValue_Socket_Close },
        { .name = "setsync", .member = &CxingValue_Socket_SetSync },

        { .name = "send", .member = &CxingValue_Socket_Send },
        { .name = "recv", .member = &CxingValue_Socket_Recv },
        { .name = "sendto", .member = &CxingValue_Socket_SendTo },
        { .name = "recvfrom", .member = &CxingValue_Socket_RecvFrom },

        { .name = "shutdown", .member = &CxingValue_Socket_Shutdown },
        { .name = "bind", .member = &CxingValue_Socket_Bind },
        { .name = "connect", .member = &CxingValue_Socket_Connect },
        { .name = "listen", .member = &CxingValue_Socket_Listen },
        { .name = "accept", .member = &CxingValue_Socket_Accept },

        { .name = "getconfig", .member = &CxingValue_Socket_GetConfig },
        { .name = "setconfig", .member = &CxingValue_Socket_SetConfig },
    },
};

// TODO 2026-06-23:

struct value_nativeobj CxingSockets_SockAddr(
    int argn, struct value_nativeobj args[]);

struct value_nativeobj CxingSockets_SockLinger(
    int argn, struct value_nativeobj args[]);

struct value_nativeobj CxingSockets_IPv6MReq(
    int argn, struct value_nativeobj args[]);

#ifdef _WIN32
#error TODO: Implement Me!
#else // Assume POSIX.
typedef int PLATFORM_INT;
typedef int PLATFORM_BOOL;
typedef socklen_t PLATFORM_SOCKLEN;
typedef struct timeval PLATFORM_TIMEVAL;
#endif // _WIN32

struct value_nativeobj CxingImpl_Socket_GetConfig(
    int argn, struct value_nativeobj args[])
{
    s2data_t *key;
    const char *optname;

    AssertArgN(2);
    AssertArgImpl(0, Socket, "socket");
    AssertArgImpl(1, s2impl_str, "string");

    key = args[1].proper.p;
    optname = s2data_weakmap(key);

#define X_SockOpt_Int(writable, level, opt)                     \
    if( 0 == strcmp(optname, #level "/" #opt) )                 \
    {                                                           \
        PLATFORM_INT v;                                         \
        PLATFORM_SOCKLEN sz = sizeof v;                         \
        getsockopt(SocketThis, level, opt, &v, &sz);            \
        return (struct value_nativeobj){                        \
            .proper.l = v,                                      \
            .type = (const void *)&type_nativeobj_long };       \
    }

#define X_SockOpt_Bool(writable, level, opt)                    \
    if( 0 == strcmp(optname, #level "/" #opt) )                 \
    {                                                           \
        PLATFORM_BOOL v = 19700101;                             \
        PLATFORM_SOCKLEN sz = sizeof v;                         \
        getsockopt(SocketThis, level, opt, &v, &sz);            \
        return (struct value_nativeobj){                        \
            .proper.l = v,                                      \
            .type = (const void *)&type_nativeobj_long };       \
    }

#define X_SockOpt_Time(writable, level, opt)                    \
    if( 0 == strcmp(optname, #level "/" #opt) )                 \
    {                                                           \
        PLATFORM_TIMEVAL ov;                                    \
        PLATFORM_SOCKLEN sz = sizeof ov;                        \
        double v;                                               \
        getsockopt(SocketThis, level, opt, &ov, &sz);           \
        v = ov.tv_sec + ov.tv_usec * 0.000001;                  \
        return (struct value_nativeobj){                        \
            .proper.f = v,                                      \
            .type = (const void *)&type_nativeobj_double };     \
    }

#define X_SockOpt_DSFA(typeabbrev, writable, level, opt)        \
    if( 0 == strcmp(optname, #level "/" #opt) )                 \
    {                                                           \
        struct value_nativeobj valobj;                          \
        void *backing;                                          \
        PLATFORM_SOCKLEN sz;                                    \
        valobj = CxingSockets_##typeabbrev(0, NULL);            \
        if( IsNull(valobj) ) return valobj;                     \
        sz = s2data_len(valobj.proper.p);                       \
        backing = s2data_weakmap(valobj.proper.p);              \
        getsockopt(SocketThis, level, opt, backing, &sz);       \
        return valobj;                                          \
}

#include "cxing-stdsocks-options.inc"
#undef X_SockOpt_Int
#undef X_SockOpt_Bool
#undef X_SockOpt_Time
#undef X_SockOpt_DSFA

    (void)(SOL_SOCKET + SO_ACCEPTCONN);
    if( 0 == strcmp(optname, "peername") )
    {
        struct value_nativeobj valobj;
        void *backing;
        socklen_t sz;
        valobj = CxingSockets_SockAddr(0, NULL);
        if( IsNull(valobj) ) return valobj;
        sz = sizeof(struct sockaddr_storage);
        backing = s2data_weakmap(valobj.proper.p);
        getpeername(SocketThis, backing, &sz);
        return valobj;
    }

    if( 0 == strcmp(optname, "sockname") )
    {
        struct value_nativeobj valobj;
        void *backing;
        socklen_t sz;
        valobj = CxingSockets_SockAddr(0, NULL);
        if( IsNull(valobj) ) return valobj;
        sz = sizeof(struct sockaddr_storage);
        backing = s2data_weakmap(valobj.proper.p);
        getsockname(SocketThis, backing, &sz);
        return valobj;
    }

    CxingDebug("This socket option hasn't been implemented yet!\n");
    return (struct value_nativeobj){
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth };
}

struct value_nativeobj CxingImpl_Socket_SetConfig(
    int argn, struct value_nativeobj args[])
{
    s2data_t *key;
    const char *optname;

    AssertArgN(3);
    AssertArgImpl(0, Socket, "socket");
    AssertArgImpl(1, s2impl_str, "string");

    key = args[1].proper.p;
    optname = s2data_weakmap(key);

#define X_SockOpt_Int(writable, level, opt)                             \
    if( 0 == strcmp(optname, #level "/" #opt) && writable )             \
    {                                                                   \
        PLATFORM_INT v;                                                 \
        PLATFORM_SOCKLEN sz = sizeof v;                                 \
        if( !IsInteger(args[2]) )                                       \
        {                                                               \
            CxingDebug("The `%s/%s` option needs to be an integer.\n",  \
                       #level, #opt);                                   \
            return (struct value_nativeobj){                            \
                .proper.p = NULL,                                       \
                .type = (const void *)&type_nativeobj_morgoth };        \
        }                                                               \
        v = args[2].proper.l;                                           \
        setsockopt(SocketThis, level, opt, &v, sz);                     \
        return (struct value_nativeobj){                                \
            .proper.l = v,                                              \
            .type = (const void *)&type_nativeobj_long };               \
    }

#define X_SockOpt_Bool(writable, level, opt)                            \
    if( 0 == strcmp(optname, #level "/" #opt) && writable )             \
    {                                                                   \
        PLATFORM_BOOL v;                                                \
        PLATFORM_SOCKLEN sz = sizeof v;                                 \
        if( !IsInteger(args[2]) )                                       \
        {                                                               \
            CxingDebug("The `%s/%s` option needs to be integral.\n",    \
                       #level, #opt);                                   \
            return (struct value_nativeobj){                            \
                .proper.p = NULL,                                       \
                .type = (const void *)&type_nativeobj_morgoth };        \
        }                                                               \
        v = args[2].proper.l != 0;                                      \
        setsockopt(SocketThis, level, opt, &v, sz);                     \
        return (struct value_nativeobj){                                \
            .proper.l = v,                                              \
            .type = (const void *)&type_nativeobj_long };               \
    }

#define X_SockOpt_Time(writable, level, opt)                            \
    if( 0 == strcmp(optname, #level "/" #opt) && writable )             \
    {                                                                   \
        PLATFORM_TIMEVAL ov;                                            \
        PLATFORM_SOCKLEN sz = sizeof ov;                                \
        double v;                                                       \
        if( args[2].type->typeid == valtyp_double )                     \
        {                                                               \
            v = args[2].proper.f;                                       \
        }                                                               \
        else if( args[2].type->typeid == valtyp_long )                  \
        {                                                               \
            v = args[2].proper.l;                                       \
        }                                                               \
        else if( args[2].type->typeid == valtyp_ulong )                 \
        {                                                               \
            v = args[2].proper.u;                                       \
        }                                                               \
        else                                                            \
        {                                                               \
            CxingDebug("The `%s/%s` option needs to be a scalar.\n",    \
                       #level, #opt);                                   \
            return (struct value_nativeobj){                            \
                .proper.p = NULL,                                       \
                .type = (const void *)&type_nativeobj_morgoth };        \
        }                                                               \
        ov.tv_sec = v;                                                  \
        ov.tv_usec = v * 1000000;                                       \
        ov.tv_usec %= 1000000;                                          \
        setsockopt(SocketThis, level, opt, &ov, sz);                    \
        return (struct value_nativeobj){                                \
            .proper.f = v,                                              \
            .type = (const void *)&type_nativeobj_double };             \
    }

#define X_SockOpt_DSFA(typeabbrev, writable, level, opt)        \
    if( 0 == strcmp(optname, #level "/" #opt) && writable )     \
    {                                                           \
        void *backing;                                          \
        PLATFORM_SOCKLEN sz;                                    \
        AssertArgImpl(2, typeabbrev, #typeabbrev);              \
        sz = s2data_len(args[2].proper.p);                      \
        backing = s2data_weakmap(args[2].proper.p);             \
        setsockopt(SocketThis, level, opt, backing, sz);        \
        return args[2];                                         \
    }

#include "cxing-stdsocks-options.inc"
#undef X_SockOpt_Int
#undef X_SockOpt_Bool
#undef X_SockOpt_Time
#undef X_SockOpt_DSFA

    CxingDebug("Modifying this socket option is not supported.\n");
    return (struct value_nativeobj){
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth };
}

struct value_nativeobj CxingSockets_GetAddrInfo(
    int argn, struct value_nativeobj args[]);

struct value_nativeobj CxingSockets_GetNameInfo(
    int argn, struct value_nativeobj args[]);

cxing_builtin_def_t CxingStdlibSocketsBuiltins[] = {
    { "socket", (struct value_nativeobj){
            .proper.p = CxingImpl_Socket_Create,
            .type = (const void *)&type_nativeobj_subr } },

    { "sockaddr", (struct value_nativeobj){
            .proper.p = CxingSockets_SockAddr,
            .type = (const void *)&type_nativeobj_subr } },

    { "sock_linger", (struct value_nativeobj){
            .proper.p = CxingSockets_SockLinger,
            .type = (const void *)&type_nativeobj_subr } },

    { "ipv6_mreq", (struct value_nativeobj){
            .proper.p = CxingSockets_IPv6MReq,
            .type = (const void *)&type_nativeobj_subr } },

#define STDSOCK_ICONST(id) {                                    \
        #id , (struct value_nativeobj){                         \
            .proper.l = id ,                                    \
            .type = (const void *)&type_nativeobj_long } },
#include "cxing-stdsocks-iconsts.inc"

    { "getaddrinfo", (struct value_nativeobj){
            .proper.p = CxingSockets_GetAddrInfo,
            .type = (const void *)&type_nativeobj_subr } },

    { "getnameinfo", (struct value_nativeobj){
            .proper.p = CxingSockets_GetNameInfo,
            .type = (const void *)&type_nativeobj_subr } },

    { 0 },
};
