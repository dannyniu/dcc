/* DannyNiu/NJF, 2026-06-30. Public Domain. */

#include "cxing-stdlib.h"
#ifdef _WIN32
#include <io.h>
#include <winsock2.h>
#define poll WSAPoll
#else // Assume POSIX.
#include <poll.h>
#endif // _WIN32

typedef struct CxingPollSetHdr {
    int32_t     refcnt;
    int32_t     capacity;
    int32_t     filled;
    int32_t     entrysize;
} CxingPollSet;

CxingMethodValueWithImpl(PollSet, Copy);
CxingMethodValueWithImpl(PollSet, Final);
CxingMethodValueWithImpl(PollSet, InitSet);
CxingMethodValueWithImpl(PollSet, Poll);
CxingMethodValueWithImpl(PollSet, REvents);

struct value_nativeobj CxingImpl_PollSet_Copy(
    int argn, struct value_nativeobj args[])
{
    CxingPollSet *pollset;
    AssertArgN(1);
    AssertArgImpl(0, PollSet, "polling set");

    pollset = args[0].proper.p;
    pollset->refcnt ++;
    return args[0];
}

struct value_nativeobj CxingImpl_PollSet_Final(
    int argn, struct value_nativeobj args[])
{
    CxingPollSet *pollset;
    AssertArgN(1);
    AssertArgImpl(0, PollSet, "polling set");

    pollset = args[0].proper.p;
    if( 0 == --pollset->refcnt )
    {
        free(pollset);
    }
    return (struct value_nativeobj){
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth };
}

struct value_nativeobj CxingImpl_PollSet_InitSet(
    int argn, struct value_nativeobj args[])
{
    CxingPollSet *pollset;
    int fd;
#ifdef _WIN32
    HANDLE hndl;
#endif // _WIN32
    struct pollfd *pollfds;

    AssertArgN(3);
    AssertArgImpl(0, PollSet, "polling set");
    AssertArgImpls(
        1,
        AcceptArgImpl(1, Socket)
        AcceptArgImpl(1, RegFile)
        AcceptArgImpl(1, Pipe)
        AcceptArgImpl(1, s2impl_str),
        "input/output handle");

    if( args[1].type == (const void *)&type_nativeobj_s2impl_str )
    {
        if( 0 != strcmp(s2data_weakmap(args[1].proper.p), "__proto__") )
        {
            CxingDebug("The keys to initialize an entry of a polling set "
                       "should be input/output handles. "
                       "However an unrecognized string was encountered.\n");
            return (struct value_nativeobj){
                .proper.p = NULL,
                .type = (const void *)&type_nativeobj_morgoth };
        }

        return args[0];
    }

    if( !IsInteger(args[2]) )
    {
        CxingDebug("The `events` field should "
                   "be specified as an integer flag!\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    pollset = args[0].proper.p;
    if( pollset->filled == pollset->capacity )
    {
        CxingDebug("The polling set is full, further items will be ignored!");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    if( args[1].type == (const void *)&type_nativeobj_Socket )
    {
#ifdef _WIN32
        hndl = s2ref_unwrap(args[1].proper.p);
#else // Assume POSIX.
        fd = (int)(intptr_t)s2ref_unwrap(args[1].proper.p);
#endif // _WIN32
    }

    else // Assume they're either regular files or pipes.
    {
        fd = fileno(s2ref_unwrap(args[1].proper.p));
#ifdef _WIN32
        hndl = (HANDLE)_get_osfhandle(fd);
#endif // _WIN32
    }

    pollfds = (void *)(pollset + 1);
#ifdef _WIN32
    pollfds[pollset->filled].fd = (SOCKET)hndl;
#else // Assume POSIX.
    pollfds[pollset->filled].fd = fd;
#endif // _WIN32
    pollfds[pollset->filled].events = args[2].proper.l;

    pollset->filled ++;
    return (struct value_nativeobj){
        .proper.l = 0,
        .type = (const void *)&type_nativeobj_long };
}

struct value_nativeobj CxingImpl_PollSet_Poll(
    int argn, struct value_nativeobj args[])
{
    CxingPollSet *pollset;
    struct pollfd *pollfds;

    AssertArgN(2);
    AssertArgImpl(0, PollSet, "polling set");

    if( !IsInteger(args[1]) )
    {
        CxingDebug("The timeout argument need to be in milliseconds "
                   "and specified as an integer.\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    pollset = args[0].proper.p;
    pollfds = (void *)(pollset + 1);
    return (struct value_nativeobj){
        .proper.l = poll(pollfds, pollset->filled, args[1].proper.l),
        .type = (const void *)&type_nativeobj_long };
}

struct value_nativeobj CxingImpl_PollSet_REvents(
    int argn, struct value_nativeobj args[])
{
    CxingPollSet *pollset;
    struct pollfd *pollfds;
    AssertArgN(2);
    AssertArgImpl(0, PollSet, "polling set");

    if( !IsInteger(args[1]) )
    {
        CxingDebug("Non-integer index on polling sets yields `null`.\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    pollset = args[0].proper.p;
    pollfds = (void *)(pollset + 1);

    if( args[1].proper.u >= (uint64_t)pollset->filled )
    {
        // Index out of range.
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    return (struct value_nativeobj){
        .proper.l = pollfds[args[1].proper.u].revents,
        .type = (const void *)&type_nativeobj_long };
}

struct value_nativeobj CxingSockets_PollSet(
    int argn, struct value_nativeobj args[])
{
    CxingPollSet *ret;
    AssertArgN(1);

    if( !IsInteger(args[0]) )
    {
        CxingDebug("The number of entries in the polling set "
                   "need to be specified at creation upfront.\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    ret = calloc(1, sizeof(struct CxingPollSetHdr) +
                 args[0].proper.u * sizeof(struct pollfd));

    if( !ret )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    ret->refcnt = 1;
    ret->capacity = args[0].proper.u;
    ret->filled = 0;
    ret->entrysize = sizeof(struct pollfd);
    return (struct value_nativeobj){
        .proper.p = ret,
        .type = (const void *)&type_nativeobj_PollSet };
}

const type_nativeobj_struct_p5 type_nativeobj_PollSet = {
    .typeid = valtyp_obj,
    .n_entries = 5,
    .static_members = {
        { .name = "__copy__", .member = &CxingValue_PollSet_Copy },
        { .name = "__final__", .member = &CxingValue_PollSet_Final },
        { .name = "__initset__", .member = &CxingValue_PollSet_InitSet },
        { .name = "poll", .member = &CxingValue_PollSet_Poll },
        { .name = "revents", .member = &CxingValue_PollSet_REvents },
    },
};
