/* DannyNiu/NJF, 2026-08-14. Public Domain. */

#ifndef dcc_mental_comp_h
#define dcc_mental_comp_h 1

// Declarations for META - The Mental Picture of the Compiler.

#include "../c-misc/omega.h"
#include "stack-parts.h"

// For error reporting.
#include "../cpp-c/cpp-c.h"

mInstr_t *MetaInstrIMM(lex_token_t *vtoken, dccOmega_t *omega, cpptu_t *ctx_tu);
mInstr_t *MetaInstrADD(lalr_prod_t *prod, dccOmega_t *omega, cpptu_t *ctx_tu);
mInstr_t *MetaInstrSUB(lalr_prod_t *prod, dccOmega_t *omega, cpptu_t *ctx_tu);
mInstr_t *MetaInstrMUL(lalr_prod_t *prod, dccOmega_t *omega, cpptu_t *ctx_tu);
mInstr_t *MetaInstrDIV(lalr_prod_t *prod, dccOmega_t *omega, cpptu_t *ctx_tu);
mInstr_t *MetaInstrREM(lalr_prod_t *prod, dccOmega_t *omega, cpptu_t *ctx_tu);

typedef struct {
    CookieTable consumer_stats, usage_stats;
    s2list_t *mini_stream;
    s2list_t *scoped_decls;
    dccOmega_t *abi_oracle;
    cpptu_t *ctx_tu;

    // Negative for amd64 and arm64, and perhaps many other "mainstream" architectures.
    int32_t sz_localdecls, sz_reusedops;

    // relative to the beginning of the ephemerals partition.
    int32_t ptr_sp_actual;

    // Positive add-back. Yielding effective stack pointer value.
    int32_t off_sp_addend;

    // a pack of `dcc_stack_entry_info_t` elements.
    s2data_t *stack_entries_reusedops;

    // a pack of `dcc_stack_entry_info_t` elements.
    s2data_t *stack_entries_ephemerals;
} dccMeta_t;

// Actions for 'OperandDiscount' for expressions whose production rule has no sequence points,
// such as arithmetic add/sub/mul/div, and bitwise and/or/xor, and shifts, etc.
void OperandDiscount_UnsequencedExpr(lalr_prod_t *node, dccMeta_t *ctx);

// Same as above, except that it's actions for 'CollectReuseStats'.
void CollectReuseStats_UnsequencedExpr(lalr_prod_t *node, dccMeta_t *ctx);

// 2026-09-25:
// Interacts with the AST/DAG traverser by operating directly on the 'operand_index'.
void BinaryOperation_Exhalation(
    lalr_prod_t *node, dccMeta_t *ctx,
    ptrdiff_t *operand_index);

// 2026-09-26:
// scalar literals should be trivial, not sure about strings (i.e. character arrays).
void TerminalLeaf_Exhalation(
    lalr_prod_t *node, dccMeta_t *ctx,
    ptrdiff_t *operand_index);

// Emits restore sequence for the operand `dish`.
bool PopSequence(
    mInstr_t *dish, // the (previously) pushed operand.
    dccMeta_t *ctx, // The mental picture of the compiler.
    regid_t regid); // the register to assign to `dest_actual` of the operand.

// Emits save sequence for the operand `dish`.
void PushSequence(
    mInstr_t *dish, // the to be pushed operand.
    dccMeta_t *ctx); // The mental picture of the compiler.



// 2026-09-26 TODO: document these:
int SemaTrav_NodeColoring(lalr_prod_t *code, dccMeta_t *ctx);
int SemaTrav_OperandDiscount(lalr_prod_t *code, dccMeta_t *ctx);
int SemaTrav_CollectReuseStats(lalr_prod_t *code, dccMeta_t *ctx);

int XhaleTrav_MiniStream(lalr_prod_t *code, dccMeta_t *ctx);

int64_t MiniStream_InsertSaveRestores(
    dccMeta_t *ctx, bool dryrun,
    omega_register_allocator_t *regfile,
    omega_regset_t regfile_subset);

#endif // dcc_mental_comp_h
