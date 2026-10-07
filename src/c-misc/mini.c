/* DannyNiu/NJF, 2026-09-26. Public Domain. */

#include "mini.h"

#define m(op, ...) { .opcode = m##op, .name = #op }
struct mini_mnemonic mini_mnemonics_def[] =
{
#include "mini-opcodes.bits.h"
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

// 2026-09-27:
// For debugging:

void fprint_regid(FILE *fp, regid_t x)
{
    int c = '?';
    switch( x >> 16 )
    {
    case 1: c = 'c'; break;
    case 2: c = 's'; break;
    case 4: c = 'i'; break;
    case 8: c = 'l'; break;
    case 16: c = 'q'; break;
    }
    fprintf(fp, "%c.%d", c, x & 0xffff);
}

void fprint_operand(FILE *fp, mInstr_t *op)
{
    if( !op ) { fprintf(fp, "-/-"); return; }
    fprint_regid(fp, op->dest_actual), fprintf(fp, "/"), fprint_regid(fp, op->dest_compute);
}

void fprint_minstr(FILE *fp, mInstr_t *ins)
{
    fprintf(fp, "%p: %s\t", ins, mini_mnemonics[ins->opcode].name);
    fprint_operand(fp, ins), fprintf(fp, ",\t");
    fprint_operand(fp, ins->op), fprintf(fp, ",\t");
    fprint_operand(fp, ins->op1), fprintf(fp, ",\t");
    fprint_operand(fp, ins->op2), fprintf(fp, "\n");
}

void print_regid(regid_t x){ fprint_regid(stdout, x); }
void print_operand(mInstr_t *op){ fprint_operand(stdout, op); }
void print_minstr(mInstr_t *ins){ fprint_minstr(stdout, ins); }
