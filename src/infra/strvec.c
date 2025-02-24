/* DannyNiu/NJF, 2024-12-17. Public Domain */

#define safetypes2_implementing_strvec
#include "strvec.h"

void strvec_final(T *restrict ctx)
{
    int32_t len = strvec_len(ctx);
    int32_t i;
    char **stab = s2data_map(ctx->ptab, 0, 0);

    for(i=1; i<len; i++) // 0 is the literal "" not allocated from heap.
    {
        if( stab[i] )
            free(stab[i]);
    }
    s2data_unmap(ctx->ptab);

    s2obj_release(&ctx->ptab->base);
}

T *strvec_create()
{
    const char **stab;
    T *ret;
    s2data_t *ptab = s2data_create(sizeof(void *));
    if( !ptab ) return NULL;

    ret = (T *)s2gc_obj_alloc(0x0300 + sizeof(void *), sizeof(T));
    if( !ret )
    {
        s2obj_release(&ptab->base);
        return NULL;
    }

    stab = s2data_map(ptab, 0, 0);
    stab[0] = "";
    s2data_unmap(ptab);

    ret->ptab = ptab;
    ret->base.finalf = (s2func_final_t)strvec_final;
    return ret;
}

int32_t strvec_len(T *restrict ctx)
{
    return s2data_len(ctx->ptab) / sizeof(void *);
}

int32_t strvec_str2i(T *restrict ctx, const char *s)
{
    int32_t len = strvec_len(ctx);
    int32_t i;
    char **stab = s2data_map(ctx->ptab, 0, 0);

    for(i=0; i<len; i++)
    {
        if( stab[i] == NULL || strcmp(stab[i], s) == 0 )
            break;
    }
    if( i < len )
    {
        s2data_unmap(ctx->ptab);
        return i;
    }

    s2data_unmap(ctx->ptab);
    if( s2data_trunc(ctx->ptab, (len + 1) * sizeof(void *)) == -1 )
        return -1;

    stab = s2data_map(ctx->ptab, 0, 0);
    if( !(stab[i] = calloc(1, strlen(s)+1)) )
    {
        s2data_unmap(ctx->ptab);
        return -1;
    }
    strcpy(stab[i], s);
    s2data_unmap(ctx->ptab);

    return i;
}

const char *strvec_i2str(T *restrict ctx, int32_t i)
{
    int32_t len = strvec_len(ctx);
    const char **stab = s2data_map(ctx->ptab, 0, 0);
    const char *ret;

    if( i < len )
    {
        ret = stab[i];
        s2data_unmap(ctx->ptab);
        return ret;
    }
    else
    {
        s2data_unmap(ctx->ptab);
        return NULL;
    }
}
