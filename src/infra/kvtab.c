/* DannyNiu/NJF, 2026-05-05. Public Domain. */

#define DCC_Implementing_KVTab
#include "kvtab.h"

static void kvtab_final(T *x)
{
    int i;
    for(i=0; i<x->count; i++)
        if( x->entries[i].v )
            s2obj_release(x->entries[i].v);
}

T *kvtab_create(int count)
{
    T *ret = (void *)s2gc_obj_alloc(
        S2_OBJ_TYPE_KVTAB,
        offsetof(T, entries) +
        sizeof(struct kvent) * count);

    if( !ret ) return NULL;
    ret->base.finalf = (s2func_final_t)kvtab_final;
    ret->count = count;
    return ret;
}

void *kvtab_get(
    kvtab_t *x, const char *k)
{
    int i;

    for(i=0; i<x->count; i++)
    {
        if( strcmp(k, x->entries[i].k) == 0 )
        {
            return x->entries[i].v;
        }
    }

    return NULL;
}
