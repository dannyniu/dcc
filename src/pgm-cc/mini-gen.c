/* DannyNiu/NJF, 2026-08-28. Public Domain. */

#include "mental-comp.h"

DEPRECATED void OperandDiscount_UnsequencedExpr(lalr_prod_t *node, dccMeta_t *ctx)
{
    // Eliminate second computations of reusable operands. I.e.:
    // When an operand is used in an reused expression,
    // discount that use of the operand.

    mInstr_t *dish = (mInstr_t *)node->value;
    if( dish->reuseCandidate )
    {
        size_t i;
        lalr_prod_t *term;

        for(i=0; i<node->terms_count; i++)
        {
            term = node->terms[i].production;
            if( !s2_is_prod(term) )
                continue;

            if( !term->value )
                continue;

            -- *CookieTable_pEntry(
                &ctx->consumer_stats,
                &((mInstr_t *)term->value)->cookie);
        }
    }
}

DEPRECATED void CollectReuseStats_UnsequencedExpr(lalr_prod_t *node, dccMeta_t *ctx)
{
    mInstr_t *dish = (mInstr_t *)node->value;

    if( *CookieTable_pEntry(
            &ctx->consumer_stats,
            &dish->cookie) > 1 )
    {
        // 2026-08-14 TODO (note retained once 2026-08-24):
        // Node is a reused operand,
        // grow stack partition for reused operands.
    }
    else dish->reuseCandidate = false;

}

// To pop 1 element into a register,
// - retrieve the stack entry info for the element,
// - issue a load instruction, with an effective stack pointer, calculated from
//   the physical stack pointer value, summed with the stack offset addend.
// - pop the stack entry,
// - obtain the effective stack pointer from the previous stack entry,
// - calculate the previous physical stack pointer by rounding the effective
//   stack pointer according to the stack alignment requirement of the ABI.
// - calculate the pop-addend,
//   if it's not zero, issue stack size decrement instruction.

// To push 1 element onto the stack,
// - subtract the size of the element from the effective stack pointer,
// - round the value according to ABI's alignment requirement of the data type,
// - use this as the datum's stack address in the stack entry info.
// - round the value further according to the ABI's alignment requirement for the stack.
// - if the calculated value is different from the phyiscal stack pointer as remembered
//   by the current mental picture of the compiler, issue stack space increment instruction.
// - remember the current physical stack pointer, and the stack offset addend used
//   for recalculating the effective stack pointer into the compiler's mental picture,
// - issue a store instruction from the register to the stack slot.

bool PopSequence(
    mInstr_t *dish, // the (previously) pushed operand.
    dccMeta_t *ctx, // The mental picture of the compiler.
    regid_t regid) // the register to assign to `dest_actual` of the operand.
{
    dcc_stack_entry_info_t *se = s2data_weakmap(ctx->stack_entries_ephemerals);
    ptrdiff_t si = s2data_len(ctx->stack_entries_ephemerals) / sizeof(*se) - 1;
    mInstr_t *inst_pop;

    if( si < 0 )
        return false; // it's empty.

    if( memcmp(&dish->cookie, &se->cookie, sizeof(se[si].cookie)) != 0 )
        return false; // wasn't push-saved on (partition-III of) the stack.

    inst_pop = mInstrCreate();
    inst_pop->dest_actual = inst_pop->dest_compute = dish->dest_actual = regid;
    inst_pop->opcode = mLD;
    inst_pop->type = dish->type;
    inst_pop->payload = (inst_pop->op = mInstrCreate())->pobj;

    // 2026-08-24:
    // Because it's likely that loads/stores from/to the stack are likely to
    // involve immediates, we'll not assign registers for this sequence, and let
    // ABI oracle fuse the operand addend as part of the instruction's immediate.
    inst_pop->op->opcode = mAS;
    inst_pop->op->type = mPointer;
    inst_pop->op->misc = ctx->off_sp_addend;

    s2list_push(ctx->mini_stream, inst_pop->pobj, s2_setter_gave);

    // pop 1 stack entry.
    if( si > 0 )
    {
        if( se[si - 1].addrend - ctx->ptr_sp_actual > ctx->abi_oracle->stack_align )
        {
            mInstr_t *pop_arith;
            int32_t spe; // effective stack pointer,
            int32_t spa; // stack pointer 'aligned',
            int32_t osp; // offset stack pointer addend.
            spe = se[si - 1].addrend;
            spa = spe - (osp = spe % ctx->abi_oracle->stack_align);

            pop_arith = mInstrCreate();

            // See 2026-08-24 note above `inst_pop->op->opcode = mAS;`.
            pop_arith->opcode = mDES;
            pop_arith->misc = spa - ctx->ptr_sp_actual;

            ctx->ptr_sp_actual = spa;
            ctx->off_sp_addend = osp;

            s2list_push(ctx->mini_stream, pop_arith->pobj, s2_setter_gave);
        }
    }
    s2data_trunc(ctx->stack_entries_ephemerals, si * sizeof(*se));

    return true;
}

void PushSequence(
    mInstr_t *dish, // the to be pushed operand.
    dccMeta_t *ctx) // The mental picture of the compiler.
{
    dcc_stack_entry_info_t *se = s2data_weakmap(ctx->stack_entries_ephemerals);
    ptrdiff_t si = s2data_len(ctx->stack_entries_ephemerals) / sizeof(*se);
    mInstr_t *inst_push = mInstrCreate();

    unsigned opsz = ctx->abi_oracle->types_descs[dish->type]->size;
    unsigned algn = ctx->abi_oracle->types_descs[dish->type]->align;

    int32_t spe; // effective stack pointer,
    int32_t spa; // stack pointer 'aligned',
    int32_t osp; // offset stack pointer addend.

    s2data_trunc(ctx->stack_entries_ephemerals, (si + 1) * sizeof(*se));

    spe = se[si - 1].addrend - opsz;
    spe -= spe % algn;
    spa = spe - (osp = spe % ctx->abi_oracle->stack_align);

    if( spa != ctx->ptr_sp_actual )
    {
        mInstr_t *push_arith = mInstrCreate();

        push_arith->opcode = mENS;
        push_arith->misc = ctx->ptr_sp_actual - spa;

        s2list_push(ctx->mini_stream, push_arith->pobj, s2_setter_gave);
    }

    ctx->ptr_sp_actual = spa;
    ctx->off_sp_addend = osp;

    inst_push->opcode = mST;
    inst_push->type = dish->type;
    inst_push->payload = (inst_push->op = mInstrCreate())->pobj;

    // See 2026-08-24 note above `inst_pop->op->opcode = mAS;` in `PopSequence`.
    inst_push->op->opcode = mAS;
    inst_push->op->type = mPointer;
    inst_push->op->misc = ctx->off_sp_addend;
    inst_push->op->payload = s2obj_retain((inst_push->op1 = dish)->pobj);

    s2list_push(ctx->mini_stream, inst_push->pobj, s2_setter_gave);
}

#define eprint(func, ...) //- fprint##func(stderr, __VA_ARGS__)

// 2026-09-26:
//
// To insert pop/push sequences for a newly emitted mini stream:
// ((for now 2026-09-26:) look at one node at a time)
// - if it's a branch (i.e. compute) node, find available registers for each of the operands with their cookies:
//   (for each operands - in the reverse order of them being looked at in the 'find available register' algorithm)
//   - if the finding returned the special ''value present(regid)'' indication,
//     - then `dest_actual` is simple assigned `regid` and nothing more is done,
//   - otherwise, the operand must have been evicted:
//     - issue a pop sequence, then set `dest_actual` of the operand, and both `dest_*` fields of the pop-load instruction to the found available register.
//   - finally, find an available register for `dest_compute`.
//     - (added 2026-10-06) the register is simply assigned and not evicted.
// - if it's a leaf node (e.g. an mIMM immediate), also do find an available register:
//   - if the finding returned ''has vacancy(regid)'', call `setregcookie` for the current node.
//   - if the ''value present(regid)'' indication, then nothing is done for it.
//   - otherwise, if the operand node returned is ordered before the current node, then
//     - assert the indication is ''to-be-used'',
//     - emit push sequence for the operand node returned from finding the available register.
//   - set `dest_actual` (and? `dest_compute`? but leaves don't have compute to begin with) to the returned `regid`.
//
// To find available register with input `cookie`:
// (start with an all-clear register file)
// - call `cookie2regid` with `cookie`,
//   - if the return value is non-zero, return a special ''value present(regid)'' indication.
// - call `getavailable`, keep this return value.
// - loop:
//   - break on (TODO 2026-09-26 define:) appropriate conditions.
//   - look at (the cookies of) the operand(s) of the next instruction,
//   - mark register(s) by calling `cookie2regid` then `markregister` in sequence,
//   - call `getavailable`, if it's zero, then return:
//     - the already-kept value `regid`,
//     - the operand node,
//     - the indication ''to-be-used''.
//   - otherwise, replace the kept return value.
// return the a special ''has vacancy(regid)'' indication.
// (before returning, call `clearallmarks`)
//
//
// 2026-10-05 TODO:
// Revise sketch for `MiniStream_InsertSaveRestores`,
// Add in handling of lazily evicted/re-evaluated reused operands,
// and verify and re-check consistency of current push/pop logic.
// (These are for tomorrow, i.e. 2026-10-06.)
//
//
// 2026-10-06:
// Simultaneous push+pop pollutes the LIFO order of the stack, and needs to be
// avoided. When both operand-popping and compute-destination-push sequences
// are to be emitted:
// - the implication of push sequence conflicting with the pop sequence is that:
//   - the 'find available register' procedure returned a ''to-be-used'' indication,
//   - suggesting there is currently no available register.
//
// However, the fact that there's a pop for an immediate computation mInstr
// suggests that even-later-used operands such as the one would've been evicted
// by the push sequence must have already been pushed to deeper slot(s) than
// the just-popped operands. __As such__, this push ought to be omitted.
//
// The today's revision stems from yesterday's idea of:
// - eliminate partition-II,
// - preserve all computations constituting the recomputation of reused operands,
// - if leftover values in the register file coincides with a later reused operand,
//   then any ''unsequenced'' re-computations leading up to that value are eliminated.
//
// Extending the above discission to its completion:
// The above reasoning process had the assumption that we're operating under
// a single expression AST tree, whereas for the new idea, we're operating
// under a more general indirect-use/multi-tree model. However, since a later
// reused operand would carry a re-computation sequence, the concern disappears
// by virtue of not eliminating that re-computation sequence.

typedef enum {
    reg_value_present = 1,
    reg_tobe_used,
    reg_has_vacancy,
} regavail_stat_t;

regavail_stat_t FindAvailableRegister(
    mInstr_t *dish, // in lieu of the input `cookie`.
    regid_t *regid, mInstr_t **eviction, // output arguments
    const struct s2ctx_list_element *lookforward, // instruction stream,
    omega_register_allocator_t *regfile,
    omega_regset_t regfile_subset) // working context.
{
    regid_t rret, ireg;

    rret = regfile->cookie2regid(regfile, dish);
    if( rret )
    {
        *regid = rret;
        return reg_value_present;
    }

    rret = regfile->getavailable(regfile, dish);
    while( true )
    {
        mInstr_t *cur;
        if( !lookforward ) break; // See 2026-09-26 TODO above in sketch pseudo-code.
        else cur = (mInstr_t *)lookforward->value;

        if( !cur ) break;

#define FindAvailReg_OperandExamine(operand)                            \
        if( operand->registerLoadable )                                 \
        {                                                               \
            eprint(_minstr, operand);                                   \
            ireg = regfile->cookie2regid(regfile, operand);             \
            if( ireg )                                                  \
            {                                                           \
                regfile->markregister(regfile, ireg);                   \
                ireg = regfile->getavailable(regfile, dish);            \
                if( !ireg )                                             \
                {                                                       \
                    *regid = rret;                                      \
                    if( eviction ) *eviction = operand;                 \
                    regfile->clearallmarks(regfile, regfile_subset);    \
                    return reg_tobe_used;                               \
                }                                                       \
                else rret = ireg;                                       \
            }                                                           \
        }

        eprint(f, ">-< %p\n", cur);

        // 2026-09-26:
        // - <s answered="see below">Potential issue 1:
        //   should not dictate operand order at here by this function?
        //   but they're used in equally-distant-future by this point?</s>
        // - ..Actually..:
        //   this order here determines the order of push, and it's suffice that
        //   the order of pop implemented respectively be consistent with this one here.
        if( cur->op ){ FindAvailReg_OperandExamine(cur->op); }
        if( cur->op1 ){ FindAvailReg_OperandExamine(cur->op1); }
        if( cur->op2 ){ FindAvailReg_OperandExamine(cur->op2); }

        lookforward = lookforward->next;
        continue;
    }
    *regid = rret;
    regfile->clearallmarks(regfile, regfile_subset);
    return reg_has_vacancy;
}

int64_t MiniStream_InsertSaveRestores(
    dccMeta_t *ctx, bool dryrun,
    omega_register_allocator_t *regfile,
    omega_regset_t regfile_subset)
{
    const struct s2ctx_list_element *node_anch = &ctx->mini_stream->anch_head;
    const struct s2ctx_list_element *listptr;
    cookie_t cpmt; // abbrev: c=cookie, pmt=permutation.
    mInstr_t *cur, *nxt;
    int64_t paircnt_pp = 0; // count of push/pop pairs.
    int64_t replctr; // replacement counter.

    regavail_stat_t indicat;
    int popped;
    regid_t regid;


start_continue_process_1node:
    if( node_anch == &ctx->mini_stream->anch_head )
        s2list_seek(ctx->mini_stream, 0, S2_LIST_SEEK_SET);
    else s2list_seek(ctx->mini_stream, 1, S2_LIST_SEEK_CUR);
    if( (node_anch = node_anch->next) == &ctx->mini_stream->anch_tail ) return paircnt_pp;
    cur = (mInstr_t *)node_anch->value;
    assert( cur );

    // <recomp-elimin added="2026-10-07">

    listptr = node_anch;
    replctr = 0;
    memset(&cpmt, 0, sizeof(cpmt));
    regid = 0;
    while( true )
    {
        nxt = (mInstr_t *)listptr->value;
        if( !nxt ) break;
        if( !nxt->registerLoadable )
            break; // 2026-10-07: consider excempting this under certain condition.

        if( !nxt->reuseCandidate ) break;

        // Xor the cookie with the value of the (sub-)expression.
        cpmt = CookieXor(cpmt, nxt->cookie);

        // The operands of the (sub-)expression would cancel the previous ones out.
        // Leaving the (cookie of) root value of the (sub-)expression in `cpmt`.
        if( nxt->op && nxt->op->registerLoadable ) cpmt = CookieXor(cpmt, nxt->op->cookie);
        if( nxt->op1 && nxt->op1->registerLoadable ) cpmt = CookieXor(cpmt, nxt->op1->cookie);
        if( nxt->op2 && nxt->op2->registerLoadable ) cpmt = CookieXor(cpmt, nxt->op2->cookie);

        regid = regfile->cookie2regid(regfile, nxt);
        if( regid != 0 ) break;

        listptr = listptr->next;
        replctr ++;
    }

    if( regid != 0 && memcmp(&cpmt, &nxt->cookie, sizeof(cpmt)) == 0 )
    {
        // replace this subsequence
        while( replctr-->0 )
        {
            s2obj_t *tmp;
            s2list_shift(ctx->mini_stream, &tmp);
            s2obj_release(tmp);
        }
        nxt->opcode = mNOP;
        nxt->dest_actual = nxt->dest_compute = regid;

        if( nxt->payload ) s2obj_release(nxt->payload);
        if( nxt->pre ) s2obj_release(nxt->pre->pobj);
        if( nxt->post ) s2obj_release(nxt->post->pobj);

        nxt->payload = NULL;
        nxt->pre = nxt->post = NULL;
    }

    // </recomp-elimin>

    popped = false;

#define OperandRestoreEmitSeq(operand)                          \
    if( operand->registerLoadable )                             \
    {                                                           \
        indicat = FindAvailableRegister(                        \
            operand, &regid, NULL,                              \
            node_anch->next,                                    \
            regfile, regfile_subset);                           \
        if( indicat == reg_value_present )                      \
        {                                                       \
            operand->dest_actual = regid;                       \
        }                                                       \
        else                                                    \
        {                                                       \
            paircnt_pp ++;                                      \
            popped = true;                                      \
            if( !dryrun ) PopSequence(operand, ctx, regid);     \
        }                                                       \
        regfile->setregcookie(regfile, regid, operand);         \
    }

    // 2026-09-26:
    // reverse order of them being looked at by the 'find available register' algorithm.
    if( cur->op2 ) OperandRestoreEmitSeq(cur->op2);
    if( cur->op1 ) OperandRestoreEmitSeq(cur->op1);
    if( cur->op ) OperandRestoreEmitSeq(cur->op);

    // 2026-09-27:
    // contains arithmetic value - hence register-loadable, so assign one.
    if( cur->registerLoadable )
    {
        mInstr_t *op_tbu; // the to be used operand.

        eprint(f, "\narith-rload.\n");
        indicat = FindAvailableRegister(
            cur, &regid, &op_tbu,
            node_anch->next,
            regfile, regfile_subset);

        eprint(_regid, regid);
        eprint(f, "/%d.%s.\n", indicat, mini_mnemonics[cur->opcode].name);
        if( indicat == reg_has_vacancy )
        {
            regfile->setregcookie(regfile, regid, cur);
        }
        else if( indicat == reg_value_present )
            ; // does nothing.
        else
        {
            assert( indicat == reg_tobe_used );
            eprint(_minstr, cur);
            eprint(_minstr, op_tbu);
            eprint(f, "-- --\n");
            if( // retro note for 2026-10-06:
                // sacrifice that register in belief of a future recomputation.
                !popped
                // 2026-10-07:
                // we don't want to push operands that're recomputed in the future,
                // we care only about computations with possibly missing operands.
                && op_tbu->seqno < cur->seqno )
            {
                paircnt_pp ++;
                if( !dryrun ) PushSequence(op_tbu, ctx);
            }
            regfile->setregcookie(regfile, regid, cur);
        }
        eprint(f, "== == ==\n");

        cur->dest_compute = regid;
        if( (!cur->op || !cur->op->registerLoadable) &&
            (!cur->op1 || !cur->op1->registerLoadable) &&
            (!cur->op2 || !cur->op2->registerLoadable) )
            cur->dest_actual = regid; // leaf node, assign actual destination directly.
    }


    goto start_continue_process_1node;
}
