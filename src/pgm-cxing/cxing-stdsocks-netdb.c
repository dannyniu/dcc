/* DannyNiu/NJF, 2026-06-28. Public Domain. */

#include "cxing-stdlib.h"
#include "bsfa.h"
#include "../infra/kvtab.h"
#include <netdb.h>

socklen_t SockAddrLen(struct sockaddr *backing);

CxingMethodValueWithImpl(AddrInfos, Get);
CxingMethodValueWithImpl(AddrInfos, Copy);
CxingMethodValueWithImpl(AddrInfos, Final);

CxingMethodValueWithImpl(Addr1Info, Get);
CxingMethodValueWithImpl(Addr1Info, Copy);
CxingMethodValueWithImpl(Addr1Info, Final);

struct value_nativeobj CxingImpl_AddrInfos_GetImpl0(
    int argn, struct value_nativeobj args[])
{
    struct fsfa *ret;
    struct bsfa *aifa = args[0].proper.p;
    struct addrinfo *backing = aifa->backing;
    uint64_t ind;
    (void)argn;

    ind = args[1].proper.u;
    while( ind-- )
    {
        backing = backing->ai_next; // should be short, do a linear traversal.
        if( !backing ) break;
    }

    if( !backing )
    {
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    if( !(ret = calloc(1, sizeof *ret)) )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    ret->refcnt = 1;
    ret->backing = backing;
    ret->serving = aifa;
    BSFA_Retain(aifa);

    return (struct value_nativeobj){
        .proper.p = ret,
        .type = (const void *)&type_nativeobj_Addr1Info };
}

struct value_nativeobj CxingImpl_AddrInfos_Get(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(2);
    AssertArgImpl(0, AddrInfos, "address infos");

    if( !IsInteger(args[1]) )
    {
        CxingDebug("The array index should be an integer.\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    return CxingImpl_AddrInfos_GetImpl0(argn, args);
}

struct value_nativeobj CxingImpl_AddrInfos_Copy(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, AddrInfos, "address infos");
    BSFA_Retain(args[0].proper.p);
    return args[0];
}

struct value_nativeobj CxingImpl_AddrInfos_Final(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, AddrInfos, "address infos");
    BSFA_Release(args[0].proper.p);
    return (struct value_nativeobj){
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth };
}

struct value_nativeobj CxingSockets_SockAddr(
    int argn, struct value_nativeobj args[]);

struct value_nativeobj CxingImpl_Addr1Info_GetImpl0(
    int argn, struct value_nativeobj args[])
{
    struct fsfa *aifa = args[0].proper.p;
    struct addrinfo *backing = aifa->backing;
    const char *k = s2data_weakmap(args[1].proper.p);
    int64_t lret;
    (void)argn;

    if( 0 == strcmp(k, "cname") )
    {
        s2data_t *ret = NULL;
        if( backing->ai_canonname )
        {
            ret = s2data_from_str(backing->ai_canonname);
        }
        else
        {
            return (struct value_nativeobj){
                .proper.p = NULL,
                .type = (const void *)&type_nativeobj_morgoth };
        }

        if( !ret )
        {
            return (struct value_nativeobj){
                .proper.l = errno,
                .type = (const void *)&type_nativeobj_null };
        }

        return (struct value_nativeobj){
            .proper.p = ret,
            .type = (const void *)&type_nativeobj_s2impl_str };
    }

    if( 0 == strcmp(k, "addr") )
    {
        struct value_nativeobj ret = CxingSockets_SockAddr(0, NULL);
        void *sink;
        if( IsNull(ret) ) return ret;

        sink = s2data_weakmap(ret.proper.p);
        memcpy(sink, backing->ai_addr, backing->ai_addrlen);
        return ret;
    }

    if( 0 == strcmp(k, "flags") )
        lret = backing->ai_flags;

    if( 0 == strcmp(k, "family") )
        lret = backing->ai_family;

    if( 0 == strcmp(k, "socktype") )
        lret = backing->ai_socktype;

    if( 0 == strcmp(k, "protocol") )
        lret = backing->ai_protocol;

    if( 0 == strcmp(k, "addrlen") )
        lret = backing->ai_addrlen;

    return (struct value_nativeobj){
        .proper.l = lret,
        .type = (const void *)&type_nativeobj_long };
}

struct value_nativeobj CxingImpl_Addr1Info_Get(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(2);
    AssertArgImpl(0, Addr1Info, "address info");
    AssertArgImpl(1, s2impl_str, "string");

    return CxingImpl_Addr1Info_GetImpl0(argn, args);
}

struct value_nativeobj CxingImpl_Addr1Info_Copy(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, Addr1Info, "address info");
    FSFA_Retain(args[0].proper.p);
    return args[0];
}

struct value_nativeobj CxingImpl_Addr1Info_Final(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, Addr1Info, "address info");
    FSFA_Release(args[0].proper.p);
    return (struct value_nativeobj){
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth };
}

const type_nativeobj_struct_p3 type_nativeobj_AddrInfos = {
    .typeid = valtyp_obj,
    .n_entries = 3,
    .static_members = {
        { .name = "get", .member = &CxingValue_AddrInfos_Get },
        { .name = "__copy__", .member = &CxingValue_AddrInfos_Copy },
        { .name = "__final__", .member = &CxingValue_AddrInfos_Final },
    }
};

const type_nativeobj_struct_p3 type_nativeobj_Addr1Info = {
    .typeid = valtyp_obj,
    .n_entries = 3,
    .static_members = {
        { .name = "get", .member = &CxingValue_Addr1Info_Get },
        { .name = "__copy__", .member = &CxingValue_Addr1Info_Copy },
        { .name = "__final__", .member = &CxingValue_Addr1Info_Final },
    }
};

#ifdef _WIN32
#error TODO: Define `CxAddrInfoErr()` function-like macro..

#else // Assume POSIX.
int64_t CxAddrInfoErr(int64_t EAI)
{
    if( EAI == EAI_SYSTEM )
        return errno;
    else return CXErrNS(EAI,);
}

#endif // _WIN32

struct value_nativeobj CxingSockets_GetAddrInfo(
    int argn, struct value_nativeobj args[])
{
    char *nodename = NULL, *servname = NULL;
    struct addrinfo hints = {};
    struct addrinfo *res = NULL;
    int64_t subret;
    struct bsfa *ret;

    AssertArgN(6);
    AssertArgImpl(0, s2impl_str, "string");
    AssertArgImpl(1, s2impl_str, "string");

    if( !IsInteger(args[2]) || !IsInteger(args[3]) ||
        !IsInteger(args[4]) || !IsInteger(args[5]) )
    {
        CxingDebug("The arguments `family`, `socktype`, `protocol`, "
                   "and `flags` are required to be integers.\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    nodename = s2data_weakmap(args[0].proper.p);
    servname = s2data_weakmap(args[1].proper.p);
    hints.ai_family = args[2].proper.l;
    hints.ai_socktype = args[3].proper.l;
    hints.ai_protocol = args[4].proper.l;
    hints.ai_flags = args[5].proper.l;

    subret = getaddrinfo(nodename, servname, &hints, &res);
    if( subret != 0 )
    {
        subret = CxAddrInfoErr(subret);
        if( res ) freeaddrinfo(res);
        // TODO 2026-06-28: Namespaces for EAI_* and errno constants.
        return (struct value_nativeobj){
            .proper.l = subret,
            .type = (const void *)&type_nativeobj_null };
    }

    ret = calloc(1, sizeof *ret);
    if( !ret )
    {
        if( res ) freeaddrinfo(res);
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    ret->refcnt = 1;
    ret->backing = res;
    ret->freer = (void (*)(void *))freeaddrinfo;
    return (struct value_nativeobj){
        .proper.p = ret,
        .type = (const void *)&type_nativeobj_AddrInfos };
}

struct value_nativeobj CxingSockets_GetNameInfo(
    int argn, struct value_nativeobj args[])
{
    struct sockaddr *sa;
    int flags = 0;
    kvtab_t *ret;
    int subret = 0;

    AssertArgN(2);
    AssertArgImpl(0, SockAddr, "socket address");

    if( !IsInteger(args[1]) )
    {
        CxingDebug("The flags argument must be an integer "
                   "for the `getnameinfo` function!\n");
    }
    else flags = args[1].proper.l;

    ret = kvtab_create(2);
    if( !ret )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    // 255 octet limit on FQDN.
    ret->entries[0].v = s2data_create(256)->pobj;

    // RFC-6335 indicates a 14-character limit mentioned on the IANA website.
    ret->entries[1].v = s2data_create(32)->pobj;

    if( !ret->entries[0].v || !ret->entries[1].v )
    {
        subret = errno;
        if( ret->entries[0].v ) s2obj_release(ret->entries[0].v);
        if( ret->entries[1].v ) s2obj_release(ret->entries[1].v);

        return (struct value_nativeobj){
            .proper.l = subret,
            .type = (const void *)&type_nativeobj_null };
    }

    s2obj_keep(ret->pobj);
    s2obj_release(ret->pobj);

    ret->entries[0].k = "node";
    ret->entries[1].k = "service";

    sa = args[0].proper.p;
    subret = getnameinfo(
        sa, SockAddrLen(sa),
        s2data_weakmap((s2data_t *)ret->entries[0].v), 256,
        s2data_weakmap((s2data_t *)ret->entries[1].v), 32, flags);

    if( subret == 0 )
    {
        s2data_t *x;
        x = (s2data_t *)ret->entries[0].v;
        s2data_trunc(x, strlen(s2data_weakmap(x)));
        x = (s2data_t *)ret->entries[1].v;
        s2data_trunc(x, strlen(s2data_weakmap(x)));

        return (struct value_nativeobj){
            .proper.p = ret,
            .type = (const void *)&type_nativeobj_NameInfo };
    }
    printf("xx %x xx\n", subret);
    s2obj_leave(ret->pobj);

    return (struct value_nativeobj){
        .proper.l = CxAddrInfoErr(subret),
        .type = (const void *)&type_nativeobj_null };
}

CxingMethodValueWithImpl(NameInfo, Get);
CxingMethodValueWithImpl(NameInfo, Copy);
CxingMethodValueWithImpl(NameInfo, Final);

struct value_nativeobj CxingImpl_NameInfo_Copy(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, NameInfo, "name information");

    return CxingImpl_s2Obj_Copy(argn, args);
}

struct value_nativeobj CxingImpl_NameInfo_Final(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, NameInfo, "name information");

    return CxingImpl_s2Obj_Final(argn, args);
}

const type_nativeobj_struct_p3 type_nativeobj_NameInfo = {
    .typeid = valtyp_obj,
    .n_entries = 3,
    .static_members = {
        { .name = "__get__", .member = &CxingValue_NameInfo_Get },
        { .name = "__copy__", .member = &CxingValue_NameInfo_Copy },
        { .name = "__final__", .member = &CxingValue_NameInfo_Final },
    },
};

struct value_nativeobj CxingImpl_NameInfo_Get(
    int argn, struct value_nativeobj args[])
{
    s2data_t *ret;
    AssertArgN(2);
    AssertArgImpl(0, NameInfo, "name information");
    AssertArgImpl(1, s2impl_str, "string");

    ret = kvtab_get(
        args[0].proper.p,
        s2data_weakmap(args[1].proper.p));

    if( ret )
    {
        return (struct value_nativeobj){
            .proper.p = ret,
            .type = (const void *)&type_nativeobj_s2impl_str };
    }
    else
    {
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }
}
