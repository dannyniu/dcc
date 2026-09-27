/* DannyNiu/NJF, 2026-08-30. Public Domain. */

#include "omega-supported.h"

omega_type_desc_t aarch64_types_desc_defs[] =
{
    [mVoid]   = { .desc_size = sizeof(omega_type_desc_t) },
    [mChar]   = { .desc_size = sizeof(omega_type_desc_t), .size = 1, .align = 1 },
    [miChar]  = { .desc_size = sizeof(omega_type_desc_t), .size = 1, .align = 1 },
    [muChar]  = { .desc_size = sizeof(omega_type_desc_t), .size = 1, .align = 1 },
    [miShort] = { .desc_size = sizeof(omega_type_desc_t), .size = 2, .align = 2 },
    [muShort] = { .desc_size = sizeof(omega_type_desc_t), .size = 2, .align = 2 },
    [miInt]   = { .desc_size = sizeof(omega_type_desc_t), .size = 4, .align = 4 },
    [muInt]   = { .desc_size = sizeof(omega_type_desc_t), .size = 4, .align = 4 },
    [miLong]  = { .desc_size = sizeof(omega_type_desc_t), .size = 8, .align = 8 },
    [muLong]  = { .desc_size = sizeof(omega_type_desc_t), .size = 8, .align = 8 },
    [miLlong] = { .desc_size = sizeof(omega_type_desc_t), .size = 8, .align = 8 },
    [muLlong] = { .desc_size = sizeof(omega_type_desc_t), .size = 8, .align = 8 },

    [mPointer] = { .desc_size = sizeof(omega_type_desc_t), .size = 8, .align = 8 },

    [mfFloat] = { .desc_size = sizeof(omega_type_desc_t), .size = 4, .align = 4 },
    [mfDouble] = { .desc_size = sizeof(omega_type_desc_t), .size = 8, .align = 8 },
    [mfLdouble] = { .desc_size = sizeof(omega_type_desc_t), .size = 16, .align = 16 },

    {}
};

omega_type_desc_t *aarch64_type_desc[] =
{
    aarch64_types_desc_defs + 0,
    aarch64_types_desc_defs + 1,
    aarch64_types_desc_defs + 2,
    aarch64_types_desc_defs + 3,
    aarch64_types_desc_defs + 4,
    aarch64_types_desc_defs + 5,
    aarch64_types_desc_defs + 6,
    aarch64_types_desc_defs + 7,
    aarch64_types_desc_defs + 8,
    aarch64_types_desc_defs + 9,
    aarch64_types_desc_defs + 10,
    aarch64_types_desc_defs + 11,
    aarch64_types_desc_defs + 12,
    aarch64_types_desc_defs + 13,
    aarch64_types_desc_defs + 14,
    aarch64_types_desc_defs + 15,
    NULL,
};

typedef struct aarch64_register_allocator aarch64_register_allocator_t;

#define GPR_CNT 32
#define SIMD_CNT 32

struct aarch64_register_allocator {
    omega_register_allocator_t base;

    register_file_entry_t GPR[GPR_CNT];
    register_file_entry_t SIMD[SIMD_CNT];
};

regid_t aarch64_cookie2regid(
    aarch64_register_allocator_t *ctx, mInstr_t *dish)
{
    if( dish->type == mVoid )
    {
        return 0;
    }

    if( dish->type <= mPointer )
    {
        int i;
        for(i=0; i<GPR_CNT; i++)
        {
            if( memcmp(&dish->cookie, &ctx->GPR[i].value_current, ZSALT_LEN) )
                continue;

            return (ctx->GPR[i].regid & 0xffff) | (aarch64_type_desc[dish->type]->size << 16);
        }
    }
    else if( dish->type <= mfLdouble )
    {
        int i;
        for(i=0; i<SIMD_CNT; i++)
        {
            if( memcmp(&dish->cookie, &ctx->SIMD[i].value_current, ZSALT_LEN) )
                continue;

            return (ctx->SIMD[i].regid & 0xffff) | (aarch64_type_desc[dish->type]->size << 16);
        }
    }

    return 0;
}

cookie_t aarch64_getregcookie(
    aarch64_register_allocator_t *ctx, regid_t regid)
{
    regid &= 0xffff;
    if( regid > GPR_CNT + SIMD_CNT ) return (cookie_t){};

    if( regid >= GPR_CNT )
        return ctx->SIMD[regid - GPR_CNT].value_current;

    return ctx->GPR[regid].value_current;
}

int aarch64_setregcookie(
    aarch64_register_allocator_t *ctx, regid_t regid, mInstr_t *dish)
{
    regid &= 0xffff;
    if( regid > GPR_CNT + SIMD_CNT ) return s2_access_error;

    if( regid >= GPR_CNT )
        memcpy(&ctx->SIMD[regid - GPR_CNT].value_current,
               &dish->cookie, ZSALT_LEN);

    else
        memcpy(&ctx->GPR[regid].value_current,
               &dish->cookie, ZSALT_LEN);

    return s2_access_success;
}

void aarch64_markregister(
    aarch64_register_allocator_t *ctx, regid_t regid)
{
    regid &= 0xffff;
    if( regid > GPR_CNT + SIMD_CNT ) return;

    if( regid >= GPR_CNT )
    {
        ctx->SIMD[regid - GPR_CNT].mark = 1;
    }
    else ctx->GPR[regid].mark = 1;
}

regid_t aarch64_getavailable(
    aarch64_register_allocator_t *ctx, mInstr_t *dish)
{
    if( dish->type == mVoid )
    {
        return -1;
    }

    if( dish->type <= mPointer )
    {
        int i;
        for(i=0; i<GPR_CNT; i++)
        {
            if( ctx->GPR[i].mark )
                continue;

            return (ctx->GPR[i].regid & 0xffff) | (aarch64_type_desc[dish->type]->size << 16);
        }
    }
    else if( dish->type <= mfLdouble )
    {
        int i;
        for(i=0; i<SIMD_CNT; i++)
        {
            if( ctx->SIMD[i].mark )
                continue;

            return (ctx->SIMD[i].regid & 0xffff) | (aarch64_type_desc[dish->type]->size << 16);
        }
    }

    return 0;
}

void aarch64_clearallmarks(
    aarch64_register_allocator_t *ctx, omega_regset_t regset)
{
    int ri;

    for(ri=9; ri<=15; ri++)
        ctx->GPR[ri].mark = 0;

    for(ri=16; ri<=32; ri++)
        ctx->SIMD[ri].mark = 0;

    if( regset & omega_regset_callee_saved_GPR )
        for(ri=19; ri<=28; ri++)
            ctx->GPR[ri].mark = 0;

    if( regset & omega_regset_callee_saved_FPR )
        for(ri=8; ri<=15; ri++)
            ctx->SIMD[ri].mark = 0;
}

void aarch64_initregalloc(
    aarch64_register_allocator_t *ctx, omega_regset_t regset)
{
    // 2026-09-02:
    // Exhalation produces a stream of compute instructions sans register
    // allocation, interleaved with stores and saving of reused operands.

    int ri;

    memset(ctx, 0, sizeof(*ctx));

    assert( GPR_CNT == 32 && SIMD_CNT == 32);
    for(ri=0; ri<32; ri++)
    {
        ctx->GPR[ri].regid = ri + 0x80000;
        ctx->SIMD[ri].regid = ri + 0x100020;
        ctx->GPR[ri].mark = ctx->SIMD[ri].mark = 1;
    }

    aarch64_clearallmarks(ctx, regset);

    ctx->base.initregalloc = (omega_initregalloc_t)aarch64_initregalloc;

    ctx->base.cookie2regid = (omega_cookie2regid_t)aarch64_cookie2regid;
    ctx->base.getregcookie = (omega_getregcookie_t)aarch64_getregcookie;
    ctx->base.setregcookie = (omega_setregcookie_t)aarch64_setregcookie;

    ctx->base.markregister = (omega_markregister_t)aarch64_markregister;
    ctx->base.getavailable = (omega_getavailable_t)aarch64_getavailable;
    ctx->base.clearallmarks = (omega_clearallmarks_t)aarch64_clearallmarks;

    ctx->base.intersect = NULL; // 2026-09-25 TODO: Implement it!
}

aarch64_register_allocator_t *aarch64_regalloc_create()
{
    aarch64_register_allocator_t *ret = calloc(1, sizeof(aarch64_register_allocator_t));
    aarch64_initregalloc(ret, 0);
    return ret;
}

dccOmega_t OmegaAArch64 = {
    .char_is_signed = false,
    .stack_align = 16,
    .types_descs = aarch64_type_desc,
    .create_register_allocator_ctx = (create_register_allocator_ctx_func)aarch64_regalloc_create,
};
