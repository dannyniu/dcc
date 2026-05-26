/* DannyNiu/NJF, 2026-02-25. Public Domain. */

#define DCC_Implementing_RFDict
#include "rfdict.h"

#define VARDECL_CAPACITY_INCREMENTATION 8

static void rfdict_final(T *vd)
{
    long i;

    for(i=vd->len; i-->0; )
    {
        s2obj_leave(vd->valtab[i]);
    }
    free(vd->valtab);

    s2obj_release(vd->indtab->pobj);
}

typedef struct rfdict_iter {
    struct s2ctx_iter base;
    T *target;
    long pos;
} rfdict_iter_t;

static int rfdict_iter_step(rfdict_iter_t *restrict iter)
{
    if( iter->pos >= iter->target->len )
    {
        iter->base.key = (void *)0;
        iter->base.value = NULL;
        return 0;
    }

    iter->base.key = (void *)iter->pos;
    iter->base.value = iter->target->valtab[iter->pos ++];
    return 1;
}

static rfdict_iter_t *rfdict_iter_create(
    rfdict_t *restrict target)
{
    rfdict_iter_t *iter = NULL;

    iter = (calloc)(1, sizeof(rfdict_iter_t));
    if( !iter ) return NULL;

    iter->base.final = (s2iter_final_func_t)free;
    iter->base.next = (s2iter_stepfunc_t)rfdict_iter_step;
    iter->target = target;

    return iter;
}

T *rfdict_create()
{
    T *vd;

    vd = (T *)s2gc_obj_alloc(S2_OBJ_TYPE_RFDICT, sizeof(T));
    if( !vd ) return NULL;

    vd->base.itercreatf = (s2func_iter_create_t)rfdict_iter_create;
    vd->base.finalf = (s2func_final_t)rfdict_final;

    vd->indtab = s2dict_create();
    vd->valtab = calloc(VARDECL_CAPACITY_INCREMENTATION, sizeof(s2obj_t *));

    if( !vd->indtab || !vd->valtab )
    {
        if( vd->indtab ) s2obj_release(vd->indtab->pobj);
        if( vd->valtab ) free(vd->valtab);
        return NULL;
    }

    vd->len = 0;
    vd->cap = VARDECL_CAPACITY_INCREMENTATION;
    return vd;
}

int rfdict_get(T *vd, s2data_t *key, s2obj_t **out)
{
    s2data_t *ind;
    long *ii;
    int subret;

    subret = s2dict_get_T(s2data_t)(vd->indtab, key, &ind);
    if( subret != s2_access_success )
    {
        *out = NULL;
        return subret;
    }

    ii = s2data_weakmap(ind);
    *out = vd->valtab[*ii];

    return s2_access_success;
}

int rfdict_set(T *vd, s2data_t *key, s2obj_t *value, int semantic)
{
    s2data_t *ind;
    s2obj_t *tmp = NULL;
    long *ii = NULL;
    int subret;

    subret = s2dict_get_T(s2data_t)(vd->indtab, key, &ind);
    if( subret == s2_access_error )
    {
        return subret;
    }

    if( subret == s2_access_nullval )
    {
        assert( vd->len < vd->cap );
        if( ++vd->len == vd->cap )
        {
            void *tmp = realloc(vd->valtab, sizeof(s2obj_t *) *
                                (vd->cap + VARDECL_CAPACITY_INCREMENTATION));
            if( !tmp )
            {
                --vd->len;
                return s2_access_error;
            }

            vd->cap += VARDECL_CAPACITY_INCREMENTATION;
            vd->valtab = tmp;
        }

        ind = s2data_create(sizeof(long));
        if( !ind ) return s2_access_error;

        s2dict_set(vd->indtab, key, ind->pobj, s2_setter_gave);

        ii = s2data_weakmap(ind);
        *ii = vd->len - 1;
    }

    else if( subret == s2_access_success )
    {
        ii = s2data_weakmap(ind);
        assert( *ii < vd->cap );
        tmp = vd->valtab[*ii];
    }

    assert( ii );
    vd->valtab[*ii] = value;

    switch( semantic ){
    case s2_setter_kept:
        s2obj_keep(value);
        break;

    case s2_setter_gave:
        s2obj_keep(value);
        s2obj_release(value);
        break;
    }

    if( tmp ) s2obj_leave(tmp);

    return s2_access_success;
}
