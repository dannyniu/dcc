/* DannyNiu/NJF, 2026-06-28. Public Domain. */

#include "cxing-stdlib.h"
#ifdef _WIN32
#include <winsock2.h>
#include <ws2ipdef.h>
#include <windows.h>
typedef int PLATFORM_INT;
typedef int PLATFORM_BOOL;
typedef int32_t PLATFORM_SOCKLEN;
typedef struct timeval PLATFORM_TIMEVAL;
#else // Assume POSIX.
#include <netinet/in.h>
#include <sys/socket.h>
typedef int PLATFORM_INT;
typedef int PLATFORM_BOOL;
typedef socklen_t PLATFORM_SOCKLEN;
typedef struct timeval PLATFORM_TIMEVAL;
#endif // _WIN32

#define DSFA_GetImpl0 SockLinger_GetImpl0
#define DSFA_SetImpl0 SockLinger_SetImpl0
#define DSFA_InitSetImpl0 SockLinger_InitSetImpl0
#define DSFA_Xmacros_Defs "cxing-stdsocks-dsfa-defs.sock-linger.inc"
#include "cxing-stdlib-dsfa.bits.h"
#undef DSFA_GetImpl0
#undef DSFA_SetImpl0
#undef DSFA_InitSetImpl0
#undef DSFA_Xmacros_Defs

CxingMethodValueWithImpl(SockLinger, Get);
CxingMethodValueWithImpl(SockLinger, Set);
CxingMethodValueWithImpl(SockLinger, InitSet);
CxingMethodValueWithImpl(SockLinger, Copy);
CxingMethodValueWithImpl(SockLinger, Final);

#define DSFA_GetImpl0 IPv6MReq_GetImpl0
#define DSFA_SetImpl0 IPv6MReq_SetImpl0
#define DSFA_InitSetImpl0 IPv6MReq_InitSetImpl0
#define DSFA_Xmacros_Defs "cxing-stdsocks-dsfa-defs.ipv6-mreq.inc"
#include "cxing-stdlib-dsfa.bits.h"
#undef DSFA_GetImpl0
#undef DSFA_SetImpl0
#undef DSFA_InitSetImpl0
#undef DSFA_Xmacros_Defs

CxingMethodValueWithImpl(IPv6MReq, Get);
CxingMethodValueWithImpl(IPv6MReq, Set);
CxingMethodValueWithImpl(IPv6MReq, InitSet);
CxingMethodValueWithImpl(IPv6MReq, Copy);
CxingMethodValueWithImpl(IPv6MReq, Final);

#define Implement_DSFA_Type(typeabbrev, typename, backingtype)          \
    struct value_nativeobj CxingImpl_##typeabbrev##_Get(                \
        int argn, struct value_nativeobj args[])                        \
    {                                                                   \
        const void *backing;                                            \
        const char *key;                                                \
        AssertArgN(2);                                                  \
        AssertArgImpl(0, typeabbrev, typename);                         \
        AssertArgImpl(1, s2impl_str, "string");                         \
        backing = s2data_weakmap(args[0].proper.p);                     \
        key = s2data_weakmap(args[1].proper.p);                         \
        return typeabbrev##_GetImpl0(backing, key);                     \
    }                                                                   \
                                                                        \
    struct value_nativeobj CxingImpl_##typeabbrev##_Set(                \
        int argn, struct value_nativeobj args[])                        \
    {                                                                   \
        AssertArgN(3);                                                  \
        AssertArgImpl(0, typeabbrev, typename);                         \
        AssertArgImpl(1, s2impl_str, "string");                         \
        return typeabbrev##_SetImpl0(argn, args);                       \
    }                                                                   \
                                                                        \
    struct value_nativeobj CxingImpl_##typeabbrev##_InitSet(            \
        int argn, struct value_nativeobj args[])                        \
    {                                                                   \
        AssertArgN(3);                                                  \
        AssertArgImpl(0, typeabbrev, typename);                         \
        AssertArgImpl(1, s2impl_str, "string");                         \
        return typeabbrev##_InitSetImpl0(argn, args);                   \
    }                                                                   \
                                                                        \
    struct value_nativeobj CxingImpl_##typeabbrev##_Copy(               \
        int argn, struct value_nativeobj args[])                        \
    {                                                                   \
        AssertArgN(1);                                                  \
        AssertArgImpl(0, typeabbrev, typename);                         \
        s2obj_retain(args[0].proper.p);                                 \
        return args[0];                                                 \
    }                                                                   \
                                                                        \
    struct value_nativeobj CxingImpl_##typeabbrev##_Final(              \
        int argn, struct value_nativeobj args[])                        \
    {                                                                   \
        AssertArgN(1);                                                  \
        AssertArgImpl(0, typeabbrev, typename);                         \
        s2obj_release(args[0].proper.p);                                \
        return (struct value_nativeobj){                                \
            .proper.p = NULL,                                           \
            .type = (const void *)&type_nativeobj_morgoth };            \
    }                                                                   \
                                                                        \
    struct value_nativeobj CxingSockets_##typeabbrev(                   \
        int argn, struct value_nativeobj args[])                        \
    {                                                                   \
        s2data_t *ret;                                                  \
        void *backing;                                                  \
                                                                        \
        (void)argn;                                                     \
        (void)args;                                                     \
                                                                        \
        ret = s2data_create(sizeof(backingtype));                       \
        if( !ret )                                                      \
        {                                                               \
            return (struct value_nativeobj){                            \
                .proper.l = errno,                                      \
                .type = (const void *)&type_nativeobj_null };           \
        }                                                               \
        ret->base.type = 0x0400 | sizeof(backingtype);                  \
                                                                        \
        backing = s2data_weakmap(ret);                                  \
        memset(backing, 0, s2data_len(ret));                            \
                                                                        \
        return (struct value_nativeobj){                                \
            .proper.p = ret,                                            \
            .type = (const void *)&type_nativeobj_##typeabbrev };       \
    }                                                                   \
                                                                        \
    const type_nativeobj_struct_p5 type_nativeobj_##typeabbrev = {      \
        .typeid = valtyp_obj,                                           \
        .n_entries = 5,                                                 \
        .static_members = {                                             \
            { .name = "__get__", .member = &CxingValue_##typeabbrev##_Get }, \
            { .name = "__set__", .member = &CxingValue_##typeabbrev##_Set }, \
            { .name = "__initset__", .member = &CxingValue_##typeabbrev##_InitSet }, \
            { .name = "__copy__", .member = &CxingValue_##typeabbrev##_Copy }, \
            { .name = "__final__", .member = &CxingValue_##typeabbrev##_Final }, \
        },                                                              \
    };

Implement_DSFA_Type(SockLinger, "`sock_linger` structure", struct linger);
Implement_DSFA_Type(IPv6MReq, "`ipv6_mreq` structure", struct ipv6_mreq);
