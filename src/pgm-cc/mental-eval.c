/* DannyNiu/NJF, 2026-08-14. Public Domain. */

// This file implements the META functions that generates MINI instructions.

#include "mental-comp.h"

// This is from the MySuiteA project,
// a work also by @dannyniu dedicated to the Public Domain.
#include "../c-misc/vlong.h"

#include <siphash.h>

static inline int Base36_Char2Int(int c)
{
    if( '0' <= c && c <= '9' )
        return c - '0';

    if( 'A' <= c && c <= 'Z' )
        return 10 + c - 'A';

    if( 'a' <= c && c <= 'z' )
        return 10 + c - 'a';

    else return -1; // caller must catch error!
}

mInstr_t *MetaInstrIMM(lex_token_t *vtoken, dccOmega_t *omega, cpptu_t *ctx_tu)
{
    VLONG_T(3) accum = VLONG_INIT(3);
    vlong_t *ax = (vlong_t *)&accum;
    char *lit = s2data_weakmap(vtoken->str);

    mInstr_t *ret = mInstrCreate();
    s2data_t *payload = s2data_create(sizeof(accum) + sizeof(short));

    siphash_t hctx;

    int radix = 0, d;
    long sz;
    uint32_t lim;

    if( vtoken->completion == langlex_declit )
        radix = 10;

    if( vtoken->completion == langlex_octlit )
    {
        if( lit[1] == 'o' || lit[1] == 'O' )
            lit += 2;
        radix = 8;
    }

    if( vtoken->completion == langlex_hexlit )
    {
        lit += 2;
        radix = 16;
    }

    if( vtoken->completion == langlex_binlit )
    {
        lit += 2;
        radix = 2;
    }

    // TODO 2026-08-14: floating point literals.

    while( *lit )
    {
        if( strchr("LUlu", *lit) )
            break; // point at suffix.

        d = Base36_Char2Int(*lit);
        if( d < 0 || d >= radix )
        {
            // 2026-08-14:
            // Correctness assumed to be guaranteed by lexer.
            lit ++;
            continue;
        }

        vlong_muls(ax, ax, radix, false);
        vlong_adds(ax, ax, d, 0);
        lit ++;
    }

    // then check for overflow (for integers).
    d = miInt;
    if( strspn(lit, "lL") == 2 || strcmp(lit+1, "lL") == 2 )
        d = miLlong;
    else if( strspn(lit, "lL") == 1 || strcmp(lit+1, "lL") == 1 )
        d = miLong;
    if( strchr(lit, 'u') || strchr(lit, 'U') )
        d ++; // Assume unsigned type enums are next to signed ones.

    assert( omega->types_descs[d]->desc_size >= sizeof(omega_type_desc_t) );

    sz = omega->types_descs[d]->size;
    lim = UINT32_C(1) << (sz * 8 % 32);

    if( sz < 4 )
    {
        if( ax->v[0] >= lim )
            ccDiagnoseWarn(ctx_tu, "Integer literal overflow", spelling_and_site(vtoken));
    }

    else if( sz < 8 )
    {
        if( ax->v[1] >= lim )
            ccDiagnoseWarn(ctx_tu, "Integer literal overflow", spelling_and_site(vtoken));
    }

    else
    {
        // No longer bother to check for now (2026-08-14).
    }

    lit = s2data_weakmap(payload);
    memcpy(lit, ax, sizeof(accum));
    *(short *)(lit + sizeof(accum)) = d;

    SipHash_o128_Init(&hctx, zsalt, ZSALT_LEN);
    SipHash_c2_Update(&hctx, lit, s2data_len(payload));
    SipHash_c2d4o128_Final(&hctx, ret->cookie.cookie_bits, 15);
    ret->cookie.cookie_type = cookie_type_constlit;

    ret->payload = payload->pobj;
    ret->opcode = mIMM;
    ret->type = d;
    ret->registerLoadable = true;
    return ret;
}

static short CommonType(short typ1, short typ2, dccOmega_t *omega)
{
    if( typ1 == mVoid || typ2 == mVoid )
    {
        // contract with the caller.
        assert( 0 );
        return -1;
    }

    // 2026-08-14: No consideration for decimal types as of now.
    if( typ1 == mfLdouble || typ2 == mfLdouble ) return mfLdouble;
    if( typ1 == mfDouble || typ2 == mfDouble ) return mfDouble;
    if( typ1 == mfFloat || typ2 == mfFloat ) return mfFloat;

    // Consult ABI oracle for the signedness of plain `char`.
    if( omega->char_is_signed )
    {
        if( typ1 == mChar ) typ1 = miChar;
        if( typ2 == mChar ) typ2 = miChar;
    }
    else
    {
        if( typ1 == mChar ) typ1 = muChar;
        if( typ2 == mChar ) typ2 = muChar;
    }

    if( typ1 == typ2 ) // same type.
        return typ1;

    if( ((typ1 ^ typ2) & 1) == 0 && // same signedness.
        typ1 > mChar && typ2 > mChar ) // choose the type with higher rank.
        return typ1 > typ2 ? typ1 : typ2;

    if( (typ1 & 1) == (miInt & 1) )
    {
        // ensure typ1 is the unsigned one.
        typ1 ^= typ2;
        typ2 ^= typ1;
        typ1 ^= typ2;
    }

    // assumption on the value of type enums.
    assert( muInt > miInt );

    if( typ1 > typ2 ) // the unsigned type has greater or equal rank.
        return typ1;

    if( omega->types_descs[typ2]->size > // the signed type is the superset.
        omega->types_descs[typ1]->size )
        return typ2;

    return (typ2 & ~1) | (typ1 & 1);
}

static short ImplicitPromote(short typ, bool char_is_signed)
{
    if( typ == mVoid ) return typ;

    if( typ == mChar )
        typ = char_is_signed ? miChar : muChar;

    while( typ < miInt ) typ += 2;
    return typ;
}

mInstr_t *MetaInstrCVT(short desttype, short srctype, mInstr_t *operand)
{
    mInstr_t *ret;
    siphash_t hctx;

    ret = mInstrCreate();

    ret->op = operand;
    ret->opcode = mCVT;

    ret->type = desttype;
    ret->type1 = srctype;

    SipHash_o128_Init(&hctx, zsalt, ZSALT_LEN);
    SipHash_c2_Update(&hctx, &ret->opcode, sizeof(ret->opcode));
    SipHash_c2_Update(&hctx, &ret->type, sizeof(ret->type));
    SipHash_c2_Update(&hctx, &ret->type1, sizeof(ret->type1));
    SipHash_c2_Update(&hctx, &operand->cookie, ZSALT_LEN);
    SipHash_c2d4o128_Final(&hctx, &ret->cookie, ZSALT_LEN);

    return ret;
}

mInstr_t *MetaInstrArithOp(lalr_prod_t *prod, dccOmega_t *omega, cpptu_t *ctx_tu, bool integers_only, bool is_commutative, short opcode)
{
    mInstr_t *ret;
    mInstr_t *op1, *op2;
    short ty1, ty2, ta, tb, tr;
    int i;

    siphash_t hctx;
    uint8_t saved_bits[ZSALT_LEN];

    ret = mInstrCreate();

    // an operator in the middle, so 0 and 2 are correct!
    op1 = (mInstr_t *)prod->terms[0].production->value;
    op2 = (mInstr_t *)prod->terms[2].production->value;

    ty1 = op1->type;
    ty2 = op2->type;

    ta = ImplicitPromote(ty1, omega->char_is_signed);
    tb = ImplicitPromote(ty2, omega->char_is_signed);
    tr = CommonType(ta, tb, omega);

    do
    {
        if( !integers_only )
            break;

        if( ty1 >= mfFloat && ty1 <= mfLdouble )
        {
            ccDiagnoseError(ctx_tu, "The left-hand-side operand is not an integer", spelling_and_site(prod->terms[1].terminal));
            return NULL;
        }

        if( ty2 >= mfFloat && ty2 <= mfLdouble )
        {
            ccDiagnoseError(ctx_tu, "The right-hand-side operand is not an integer", spelling_and_site(prod->terms[1].terminal));
            return NULL;
        }
    }
    while( false );

    if( tr != ty1 )
    {
        op1 = MetaInstrCVT(tr, ty1, op1);
        op1->pre = ret->pre;
        ret->pre = op1;
    }
    if( tr != ty2 )
    {
        op2 = MetaInstrCVT(tr, ty2, op2);
        op2->pre = ret->pre;
        ret->pre = op2;
    }

    SipHash_o128_Init(&hctx, zsalt, ZSALT_LEN);
    SipHash_c2_Update(&hctx, &opcode, sizeof(opcode));
    SipHash_c2_Update(&hctx, &op1->cookie, ZSALT_LEN);
    SipHash_c2_Update(&hctx, &op2->cookie, ZSALT_LEN);
    SipHash_c2d4o128_Final(&hctx, &ret->cookie, ZSALT_LEN);

    if( is_commutative )
    {
        SipHash_o128_Init(&hctx, zsalt, ZSALT_LEN);
        SipHash_c2_Update(&hctx, &opcode, sizeof(opcode));
        SipHash_c2_Update(&hctx, &op2->cookie, ZSALT_LEN);
        SipHash_c2_Update(&hctx, &op1->cookie, ZSALT_LEN);
        SipHash_c2d4o128_Final(&hctx, saved_bits, ZSALT_LEN);

        for(i=0; i<ZSALT_LEN; i++)
            saved_bits[i] ^= ((uint8_t *)&ret->cookie)[i];

        SipHash_o128_Init(&hctx, zsalt, ZSALT_LEN);
        SipHash_c2_Update(&hctx, &opcode, sizeof(opcode));
        SipHash_c2_Update(&hctx, saved_bits, ZSALT_LEN);
        SipHash_c2d4o128_Final(&hctx, &ret->cookie, ZSALT_LEN);
    }

    ret->cookie.cookie_type = cookie_type_hashed;

    ret->op = op1;
    ret->op1 = op2;

    ret->opcode = opcode;
    ret->type = tr;
    ret->registerLoadable = true;
    return ret;
}

mInstr_t *MetaInstrADD(lalr_prod_t *prod, dccOmega_t *omega, cpptu_t *ctx_tu)
{
    // 2026-08-14 TODO: handle pointer operands.

    return MetaInstrArithOp(prod, omega, ctx_tu, false, true, mADD);
}

mInstr_t *MetaInstrSUB(lalr_prod_t *prod, dccOmega_t *omega, cpptu_t *ctx_tu)
{
    // 2026-08-14 TODO: handle pointer operands.

    return MetaInstrArithOp(prod, omega, ctx_tu, false, false, mSUB);
}

mInstr_t *MetaInstrMUL(lalr_prod_t *prod, dccOmega_t *omega, cpptu_t *ctx_tu)
{
    return MetaInstrArithOp(prod, omega, ctx_tu, false, true, mMUL);
}

mInstr_t *MetaInstrDIV(lalr_prod_t *prod, dccOmega_t *omega, cpptu_t *ctx_tu)
{
    return MetaInstrArithOp(prod, omega, ctx_tu, false, false, mDIV);
}

mInstr_t *MetaInstrREM(lalr_prod_t *prod, dccOmega_t *omega, cpptu_t *ctx_tu)
{
    // 2026-08-14 TODO: restrict to integers.
    return MetaInstrArithOp(prod, omega, ctx_tu, true, false, mREM);
}
