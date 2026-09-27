/* DannyNiu/NJF, 2026-09-26. Public Domain. */

#include "mini.h"

#define m(op, ...) { .opcode = m##op, .name = #op }
struct mini_mnemonic mini_mnemonics_def[] =
{
#include "mini-opcodes.inc"
    {},
}, *mini_mnemonics = mini_mnemonics_def;

static void mInstrFinal(mInstr_t *x)
{
    if( x->payload ) s2obj_release(x->payload);
    if( x->pre ) s2obj_release(x->pre->pobj);
    if( x->post ) s2obj_release(x->post->pobj);
}

mInstr_t *mInstrCreate()
{
    mInstr_t *ret = (mInstr_t *)s2gc_obj_alloc(
        S2_OBJ_TYPE_MINI_INSTR, sizeof(mInstr_t));
    ret->base.finalf = (s2func_final_t)mInstrFinal;
    return ret;
}
