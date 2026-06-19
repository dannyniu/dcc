/* DannyNiu/NJF, 2026-06-18. Public Domain. */

#include "ArcBase.h"

arc_base_t *arc_create(size_t sz, void (*fin_cb)(void *))
{
    arc_base_t *ret = calloc(1, sz);
    if( !ret ) return NULL;

    ret->fin_cb = fin_cb;
    atomic_init(&ret->refcnt, 1);
    return ret;
}

void *arc_retain(arc_base_t *obj)
{
    atomic_fetch_add_explicit(&obj->refcnt, 1, memory_order_relaxed);
    return obj;
}

void arc_release(arc_base_t *obj)
{
    int rc = atomic_fetch_sub_explicit(&obj->refcnt, 1, memory_order_release);
    if( rc == 1 )
    {
        atomic_thread_fence(memory_order_acquire);
        if( obj->fin_cb ) obj->fin_cb(obj);
        free(obj);
    }
}
