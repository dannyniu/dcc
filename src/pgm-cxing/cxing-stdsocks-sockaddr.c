/* DannyNiu/NJF, 2026-06-27. Public Domain. */

#include "cxing-stdlib.h"
#include <netinet/in.h>

#define DSFA_GetImpl0 SockAddr_IPv4_GetImpl0
#define DSFA_SetImpl0 SockAddr_IPv4_SetImpl0
#define DSFA_InitSetImpl0 SockAddr_IPv4_InitSetImpl0
#define DSFA_Xmacros_Defs "cxing-stdsocks-sockaddr.dsfa-defs.inc"
#define SOCKFAM 4
#include "cxing-stdlib-dsfa.bits.h"
#undef DSFA_GetImpl0
#undef DSFA_SetImpl0
#undef DSFA_InitSetImpl0
#undef DSFA_Xmacros_Defs
#undef SOCKFAM

#define DSFA_GetImpl0 SockAddr_IPv6_GetImpl0
#define DSFA_SetImpl0 SockAddr_IPv6_SetImpl0
#define DSFA_InitSetImpl0 SockAddr_IPv6_InitSetImpl0
#define DSFA_Xmacros_Defs "cxing-stdsocks-sockaddr.dsfa-defs.inc"
#define SOCKFAM 6
#include "cxing-stdlib-dsfa.bits.h"
#undef DSFA_GetImpl0
#undef DSFA_SetImpl0
#undef DSFA_InitSetImpl0
#undef DSFA_Xmacros_Defs
#undef SOCKFAM

#define DSFA_GetImpl0 SockAddr_GetImpl0
#define DSFA_SetImpl0 SockAddr_SetImpl0
#define DSFA_InitSetImpl0 SockAddr_InitSetImpl0
#define DSFA_Xmacros_Defs "cxing-stdsocks-sockaddr.dsfa-defs.inc"
#define SOCKFAM 256
#include "cxing-stdlib-dsfa.bits.h"
#undef DSFA_GetImpl0
#undef DSFA_SetImpl0
#undef DSFA_InitSetImpl0
#undef DSFA_Xmacros_Defs
#undef SOCKFAM

CxingMethodValueWithImpl(SockAddr, Get);
CxingMethodValueWithImpl(SockAddr, Set);
CxingMethodValueWithImpl(SockAddr, InitSet);
CxingMethodValueWithImpl(SockAddr, Copy);
CxingMethodValueWithImpl(SockAddr, Final);

struct value_nativeobj CxingImpl_SockAddr_Get(
    int argn, struct value_nativeobj args[])
{
    struct sockaddr *backing;
    const char *key;

    AssertArgN(2);
    AssertArgImpl(0, SockAddr, "socket address");
    AssertArgImpl(1, s2impl_str, "string");

    backing = s2data_weakmap(args[0].proper.p);
    key = s2data_weakmap(args[1].proper.p);

    if( backing->sa_family == AF_INET )
        return SockAddr_IPv4_GetImpl0(backing, key);

    if( backing->sa_family == AF_INET6 )
        return SockAddr_IPv6_GetImpl0(backing, key);

    return SockAddr_GetImpl0(backing, key);
}

struct value_nativeobj CxingImpl_SockAddr_Set(
    int argn, struct value_nativeobj args[])
{
    struct sockaddr *backing;

    AssertArgN(3);
    AssertArgImpl(0, SockAddr, "socket address");
    AssertArgImpl(1, s2impl_str, "string");

    backing = s2data_weakmap(args[0].proper.p);

    if( backing->sa_family == AF_INET )
        return SockAddr_IPv4_SetImpl0(argn, args);

    if( backing->sa_family == AF_INET6 )
        return SockAddr_IPv6_SetImpl0(argn, args);

    return SockAddr_SetImpl0(argn, args);
}

struct value_nativeobj CxingImpl_SockAddr_InitSet(
    int argn, struct value_nativeobj args[])
{
    struct sockaddr *backing;

    AssertArgN(3);
    AssertArgImpl(0, SockAddr, "socket address");
    AssertArgImpl(1, s2impl_str, "string");

    backing = s2data_weakmap(args[0].proper.p);

    if( backing->sa_family == AF_INET )
        return SockAddr_IPv4_InitSetImpl0(argn, args);

    if( backing->sa_family == AF_INET6 )
        return SockAddr_IPv6_InitSetImpl0(argn, args);

    return SockAddr_InitSetImpl0(argn, args);
}

struct value_nativeobj CxingImpl_SockAddr_Copy(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, SockAddr, "socket address");
    s2obj_retain(args[0].proper.p);
    return args[0];
}

struct value_nativeobj CxingImpl_SockAddr_Final(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, SockAddr, "socket address");
    s2obj_release(args[0].proper.p);
    return (struct value_nativeobj){
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth };
}

struct value_nativeobj CxingSockets_SockAddr(
    int argn, struct value_nativeobj args[])
{
    s2data_t *ret;
    void *backing;

    (void)argn;
    (void)args;

    ret = s2data_create(sizeof(struct sockaddr_storage));
    if( !ret )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }
    ret->base.type = 0x0400 | sizeof(struct sockaddr_storage);

    backing = s2data_weakmap(ret);
    memset(backing, 0, s2data_len(ret));

    return (struct value_nativeobj){
        .proper.p = ret,
        .type = (const void *)&type_nativeobj_SockAddr };
}

const type_nativeobj_struct_p5 type_nativeobj_SockAddr = {
    .typeid = valtyp_obj,
    .n_entries = 5,
    .static_members = {
        { .name = "__get__", .member = &CxingValue_SockAddr_Get },
        { .name = "__set__", .member = &CxingValue_SockAddr_Set },
        { .name = "__initset__", .member = &CxingValue_SockAddr_InitSet },
        { .name = "__copy__", .member = &CxingValue_SockAddr_Copy },
        { .name = "__final__", .member = &CxingValue_SockAddr_Final },
    },
};
