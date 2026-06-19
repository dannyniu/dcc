/* DannyNiu/NJF, 2026-06-18. Public Domain. */

#include "cxing-stdlib.h"
#include "../infra/ArcBase.h"
#include <pthread.h>

CxingMethodValueWithImpl(Mutex, Copy);
CxingMethodValueWithImpl(Mutex, Final);
CxingMethodValueWithImpl(Mutex, Acquire);

typedef struct {
    arc_base_t arc;
    atomic_int cv_assoc;
    pthread_mutex_t mtx_proper;
    struct value_nativeobj vp;
} cxing_mutex_t;

struct value_nativeobj CxingImpl_Mutex_Copy(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, Mutex, "mutex");

    return (struct value_nativeobj){
        .proper.p = arc_retain(args[0].proper.p),
        .type = (const void *)&type_nativeobj_Mutex };
}

struct value_nativeobj CxingImpl_Mutex_Final(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, Mutex, "mutex");
    arc_release(args[0].proper.p);

    return (struct value_nativeobj){
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth };
}

void GiftFinal(cxing_mutex_t *cxmtx)
{
    pthread_mutex_unlock(&cxmtx->mtx_proper);
}

struct value_nativeobj CxingImpl_Mutex_Acquire(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, Mutex, "mutex");

    cxing_mutex_t *cxmtx = args[0].proper.p;
    int subret = pthread_mutex_lock(&cxmtx->mtx_proper);
    if( subret != 0 )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }

    s2ref_t *gift = s2ref_create(cxmtx, (s2ref_final_func_t)GiftFinal);
    if( !gift )
    {
        pthread_mutex_unlock(&cxmtx->mtx_proper);
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }
    s2obj_keep(gift->pobj);
    s2obj_release(gift->pobj);

    return (struct value_nativeobj){
        .proper.p = gift,
        .type = (const void *)&type_nativeobj_Gift };
}

const type_nativeobj_struct_p3 type_nativeobj_Mutex = {
    .typeid = valtyp_obj,
    .n_entries = 3,
    .static_members = {
        { .name = "__copy__", .member = &CxingValue_Mutex_Copy },
        { .name = "__final__", .member = &CxingValue_Mutex_Final },
        { .name = "acquire", .member = &CxingValue_Mutex_Acquire },
    }
};

void cxing_mutex_fin(cxing_mutex_t *cxmtx)
{
    ValueDestroy(cxmtx->vp);
    if( atomic_load_explicit(&cxmtx->cv_assoc, memory_order_acquire) != 0 )
    {
        CxingDebug("Some condition variable didn't go "
                   "out of scope before this mutex. "
                   "Their consistency may be broken.\n");
    }
    pthread_mutex_destroy(&cxmtx->mtx_proper);
}

struct value_nativeobj CxingImpl_Mutex_Create(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);

    cxing_mutex_t *ret = (void *)arc_create(
        sizeof(cxing_mutex_t), (void (*)(void *))cxing_mutex_fin);

    if( !ret ) return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };

    int subret = pthread_mutex_init(&ret->mtx_proper, NULL);
    if( subret != 0 )
    {
        free(ret);
        return (struct value_nativeobj){
            .proper.l = subret,
            .type = (const void *)&type_nativeobj_null };
    }

    ret->vp = ValueCopy(args[0]);
    return (struct value_nativeobj){
        .proper.p = ret,
        .type = (const void *)&type_nativeobj_Mutex };
}

CxingMethodValueWithImpl(Gift, Get);
CxingMethodValueWithImpl(Gift, Set);
CxingMethodValueWithImpl(Gift, Copy);
CxingMethodValueWithImpl(Gift, Final);

struct value_nativeobj CxingImpl_Gift_Get(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(2);
    AssertArgImpl(0, Gift, "gift");
    AssertArgImpl(1, s2impl_str, "string");

    if( 0 != strcmp(s2data_weakmap(args[1].proper.p), "v") )
    {
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_null };
    }

    s2ref_t *gift = args[0].proper.p;
    cxing_mutex_t *cxmtx = s2ref_unwrap(gift);
    return cxmtx->vp;
}

struct value_nativeobj CxingImpl_Gift_Set(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(3);
    AssertArgImpl(0, Gift, "gift");
    AssertArgImpl(1, s2impl_str, "string");

    if( 0 != strcmp(s2data_weakmap(args[1].proper.p), "v") )
    {
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_null };
    }

    s2ref_t *gift = args[0].proper.p;
    cxing_mutex_t *cxmtx = s2ref_unwrap(gift);
    struct value_nativeobj tmp = ValueCopy(args[2]);
    ValueDestroy(cxmtx->vp);
    return cxmtx->vp = tmp;
}

struct value_nativeobj CxingImpl_Gift_Copy(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, Gift, "gift");

    return CxingImpl_s2Obj_Copy(argn, args);
}

struct value_nativeobj CxingImpl_Gift_Final(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, Gift, "gift");

    return CxingImpl_s2Obj_Final(argn, args);
}

const type_nativeobj_struct_p4 type_nativeobj_Gift = {
    .typeid = valtyp_obj,
    .n_entries = 4,
    .static_members = {
        { .name = "__get__", .member = &CxingValue_Gift_Get },
        { .name = "__set__", .member = &CxingValue_Gift_Set },
        { .name = "__copy__", .member = &CxingValue_Gift_Copy },
        { .name = "__final__", .member = &CxingValue_Gift_Final },
    }
};

CxingMethodValueWithImpl(CondVar, Copy);
CxingMethodValueWithImpl(CondVar, Final);
CxingMethodValueWithImpl(CondVar, Wait);
CxingMethodValueWithImpl(CondVar, Broadcast);
CxingMethodValueWithImpl(CondVar, Signal);

typedef struct {
    arc_base_t arc;
    pthread_cond_t condvar;
    cxing_mutex_t *cxmtx;
} cxing_condvar_t;

struct value_nativeobj CxingImpl_CondVar_Copy(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, CondVar, "condition variable");

    return (struct value_nativeobj){
        .proper.p = arc_retain(args[0].proper.p),
        .type = (const void *)&type_nativeobj_CondVar };
}

struct value_nativeobj CxingImpl_CondVar_Final(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, CondVar, "condition variable");
    arc_release(args[0].proper.p);

    return (struct value_nativeobj){
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth };
}

struct value_nativeobj CxingImpl_CondVar_Wait(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, CondVar, "condition variable");

    cxing_condvar_t *cxcv = args[0].proper.p;
    cxing_mutex_t *cxmtx = cxcv->cxmtx;
    int ret = pthread_cond_wait(&cxcv->condvar, &cxmtx->mtx_proper);

    if( ret )
    {
        return (struct value_nativeobj){
            .proper.l = ret,
            .type = (const void *)&type_nativeobj_null };
    }
    else
    {
        return (struct value_nativeobj){
            .proper.l = 0,
            .type = (const void *)&type_nativeobj_long };
    }
}

struct value_nativeobj CxingImpl_CondVar_Broadcast(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, CondVar, "condition variable");

    cxing_condvar_t *cxcv = args[0].proper.p;
    int ret = pthread_cond_broadcast(&cxcv->condvar);

    if( ret )
    {
        return (struct value_nativeobj){
            .proper.l = ret,
            .type = (const void *)&type_nativeobj_null };
    }
    else
    {
        return (struct value_nativeobj){
            .proper.l = 0,
            .type = (const void *)&type_nativeobj_long };
    }
}

struct value_nativeobj CxingImpl_CondVar_Signal(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, CondVar, "condition variable");

    cxing_condvar_t *cxcv = args[0].proper.p;
    int ret = pthread_cond_signal(&cxcv->condvar);

    if( ret )
    {
        return (struct value_nativeobj){
            .proper.l = ret,
            .type = (const void *)&type_nativeobj_null };
    }
    else
    {
        return (struct value_nativeobj){
            .proper.l = 0,
            .type = (const void *)&type_nativeobj_long };
    }
}

const type_nativeobj_struct_p5 type_nativeobj_CondVar = {
    .typeid = valtyp_obj,
    .n_entries = 5,
    .static_members = {
        { .name = "__copy__", .member = &CxingValue_CondVar_Copy },
        { .name = "__final__", .member = &CxingValue_CondVar_Final },
        { .name = "wait", .member = &CxingValue_CondVar_Wait },
        { .name = "broadcast", .member = &CxingValue_CondVar_Broadcast },
        { .name = "signal", .member = &CxingValue_CondVar_Signal },
    }
};

void cxing_condvar_fin(cxing_condvar_t *cxcv)
{
    atomic_fetch_sub_explicit(&cxcv->cxmtx->cv_assoc, 1, memory_order_release);
    pthread_cond_destroy(&cxcv->condvar);
}

struct value_nativeobj CxingImpl_CondVar_Create(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, Mutex, "mutex");

    cxing_condvar_t *ret = (void *)arc_create(
        sizeof(cxing_condvar_t), (void (*)(void *))cxing_condvar_fin);

    if( !ret ) return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };

    int subret = pthread_cond_init(&ret->condvar, NULL);
    if( subret != 0 )
    {
        free(ret);
        return (struct value_nativeobj){
            .proper.l = subret,
            .type = (const void *)&type_nativeobj_null };
    }

    ret->cxmtx = args[0].proper.p;
    atomic_fetch_add_explicit(&ret->cxmtx->cv_assoc, 1, memory_order_release);
    return (struct value_nativeobj){
        .proper.p = ret,
        .type = (const void *)&type_nativeobj_CondVar };
}

CxingMethodValueWithImpl(ThrdHndl, Copy);
CxingMethodValueWithImpl(ThrdHndl, Final);
CxingMethodValueWithImpl(ThrdHndl, Join);
CxingMethodValueWithImpl(ThrdHndl, Detach);
CxingMethodValueWithImpl(ThrdHndl, Equals);

typedef struct {
    arc_base_t arc;
    pthread_t thrd;
} cxing_thrd_t;

struct value_nativeobj CxingImpl_ThrdHndl_Copy(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, ThrdHndl, "thread handle");

    return (struct value_nativeobj){
        .proper.p = arc_retain(args[0].proper.p),
        .type = (const void *)&type_nativeobj_ThrdHndl };
}

struct value_nativeobj CxingImpl_ThrdHndl_Final(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, ThrdHndl, "thread handle");
    arc_release(args[0].proper.p);

    return (struct value_nativeobj){
        .proper.p = NULL,
        .type = (const void *)&type_nativeobj_morgoth };
}

struct value_nativeobj CxingImpl_ThrdHndl_Join(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, ThrdHndl, "thread handle");

    cxing_thrd_t *th = args[0].proper.p;
    int subret = pthread_join(th->thrd, NULL);
    if( subret != 0 )
    {
        return (struct value_nativeobj){
            .proper.l = subret,
            .type = (const void *)&type_nativeobj_null };
    }

    return (struct value_nativeobj){
        .proper.l = 0,
        .type = (const void *)&type_nativeobj_long };
}

struct value_nativeobj CxingImpl_ThrdHndl_Detach(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, ThrdHndl, "thread handle");

    cxing_thrd_t *th = args[0].proper.p;
    int subret = pthread_detach(th->thrd);
    if( subret != 0 )
    {
        return (struct value_nativeobj){
            .proper.l = subret,
            .type = (const void *)&type_nativeobj_null };
    }

    return (struct value_nativeobj){
        .proper.l = 0,
        .type = (const void *)&type_nativeobj_long };
}

struct value_nativeobj CxingImpl_ThrdHndl_Equals(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(2);
    AssertArgImpl(0, ThrdHndl, "thread handle");
    AssertArgImpl(1, ThrdHndl, "thread handle");

    cxing_thrd_t *th = args[0].proper.p;
    cxing_thrd_t *oth = args[1].proper.p;
    return (struct value_nativeobj){
        .proper.l = pthread_equal(th->thrd, oth->thrd),
        .type = (const void *)&type_nativeobj_long };
}

const type_nativeobj_struct_p5 type_nativeobj_ThrdHndl = {
    .typeid = valtyp_obj,
    .n_entries = 5,
    .static_members = {
        { .name = "__copy__", .member = &CxingValue_ThrdHndl_Copy },
        { .name = "__final__", .member = &CxingValue_ThrdHndl_Final },
        { .name = "join", .member = &CxingValue_ThrdHndl_Join },
        { .name = "detach", .member = &CxingValue_ThrdHndl_Detach },
        { .name = "equals", .member = &CxingValue_ThrdHndl_Equals },
    }
};

void *CxingThreadEntry(struct value_nativeobj args[2])
{
    ((cxing_call_proto)args[0].proper.p)(1, &args[1]);
    ValueDestroy(args[1]);
    free(args);
    return NULL;
}

struct value_nativeobj CxingImpl_ThrdCreate(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(2);
    if( args[0].type->typeid != valtyp_subr )
    {
        CxingDebug("The entry point for a new thread "
                   "need to be a subroutine!\n");
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_null };
    }

    cxing_thrd_t *ret = (void *)arc_create(sizeof(cxing_thrd_t), NULL);

    int subret = errno;

    if( !ret )
    {
        if( ret ) free(ret);
        return (struct value_nativeobj){
            .proper.l = subret,
            .type = (const void *)&type_nativeobj_null };
    }

    struct value_nativeobj *args_passdown =
        (void *)calloc(2, sizeof(struct value_nativeobj));

    args_passdown[0] = args[0]; // the subroutine.
    args_passdown[1] = ValueCopy(args[1]); // the argument to the entry point.

    pthread_t newthread;
    subret = pthread_create(&newthread, NULL,
                            (void *(*)(void *))CxingThreadEntry, args_passdown);

    if( subret != 0 )
    {
        free(ret);
        return (struct value_nativeobj){
            .proper.l = subret,
            .type = (const void *)&type_nativeobj_null };
    }

    ret->thrd = newthread;
    return (struct value_nativeobj){
        .proper.p = ret,
        .type = (const void *)&type_nativeobj_ThrdHndl };
}

struct value_nativeobj CxingImpl_ThrdHndl_Self(
    int argn, struct value_nativeobj args[])
{
    (void)argn;
    (void)args;

    cxing_thrd_t *ret = (void *)arc_create(sizeof(cxing_thrd_t), NULL);
    if( !ret )
    {
        return (struct value_nativeobj){
            .proper.l = errno,
            .type = (const void *)&type_nativeobj_null };
    }
    ret->thrd = pthread_self();
    return (struct value_nativeobj){
        .proper.p = ret,
        .type = (const void *)&type_nativeobj_ThrdHndl };
}

struct value_nativeobj CxingImpl_ThrdHndl_Exit(
    int argn, struct value_nativeobj args[])
{
    AssertArgN(1);
    AssertArgImpl(0, ThrdHndl, "thread handle");
    pthread_exit(NULL);
    assert( 0 );
}

cxing_builtin_def_t CxingStdlibThrdBuiltins[] = {
    { "mutex", (struct value_nativeobj){
            .proper.p = CxingImpl_Mutex_Create,
            .type = (const void *)&type_nativeobj_subr } },

    { "condvar", (struct value_nativeobj){
            .proper.p = CxingImpl_CondVar_Create,
            .type = (const void *)&type_nativeobj_subr } },

    { "thrd_create", (struct value_nativeobj){
            .proper.p = CxingImpl_ThrdCreate,
            .type = (const void *)&type_nativeobj_subr } },

    { "thrd_self", (struct value_nativeobj){
            .proper.p = CxingImpl_ThrdHndl_Self,
            .type = (const void *)&type_nativeobj_subr } },

    { "thrd_exit", (struct value_nativeobj){
            .proper.p = CxingImpl_ThrdHndl_Exit,
            .type = (const void *)&type_nativeobj_subr } },

    {}
};
