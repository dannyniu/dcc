/* DannyNiu/NJF, 2026-08-01. Public Domain. */

// MINI = Machine Instruction -Neutral Intermediary.
//
// Abstract instructions:
// (2026-08-01 TODO: exhausiveness. notice retained once 2026-08-08.)
//
// 2026-08-08:
// This is an abstraction:
//
// - where 3-partitioned stack space are calculated, so loads/stores
//   from/to local variables employs stack arithmetic.
//
// - where the actual addresses of global symbols are unknown, so
//   they need symbolic 'placeholders' for later 'relocation'.
//
// - where the mental picture of the compiler knows the placement
//   of variable values (i.e. in registers or on stack),
//   and therefore has ABI calling convention information
//   to correctly place the arguments before `CALL`'ing.
//
// ---
//
// IMM<A>(v) -> $res        ; immediate value
// SYM(:Name) -> $res       ; address of symbols
// LAB(:name)               ; jump labels
//
// JMP(:label, $cond)       ; conditional jumps
// CALL(:sym)               ; function call
// RET                      ; function return
//
// LD<A>($addr) -> $res     ; load into register
// ST<A>($addr, $reg)       ; store into memory
// AS(v) -> $addr           ; add v with stack pointer to yield an address.
// ENS(v)                   ; increase stack space.
// DES(v)                   ; decrease stack space.
//
// {ADD,SUB,MUL,DIV,REM}<A>($1, $2) -> $res     ; arithmetic
// {LSH,RSH,ROL,ROR}<W>($1, $2) -> $res         ; shift and rotate
// {AND,OR,XOR}<W>($1, $2) -> $res              ; bitwise binary
// {NOT,NEG,FLIP}<W>($1) -> $res                ; unary negations
//
// CMP<p,A>($1, $2) -> $res     ; comparison
// CVT<U,V>($1) -> $res         ; conversion
// NOP($1?) -> $res?            ; identity function, or for grouping.
//
// p: gt, ge, eq, ne, lt, le.
// W: char, short, int, long, long long.
// A, U, V: <W>, (half?), float, double, (quad?).

#ifndef dcc_mini_h
#define dcc_mini_h 1

#include "cookie-table.h"
#include <SafeTypes2.h>

enum mini_predicates {
    mini_gt = 1,
    mini_ge,
    mini_eq,
    mini_ne,
    mini_lt,
    mini_le,
};

enum mini_scalar_types {
    mVoid,
    mChar, // 'implementation'-defined signedness.
    miChar, // signed
    muChar, // unsigned
    miShort,
    muShort,
    miInt,
    muInt,
    miLong,
    muLong,
    miLlong,
    muLlong,

    // semantically distinguished from integers,
    // exact form chosen by ABI oracle.
    mPointer,

    // floating points.
    mfFloat,
    mfDouble,
    mfLdouble,
};

#define m(op, ...) m##op __VA_ARGS__
enum mini_opcode {
#include "mini-opcodes.inc"
};
#undef m
struct mini_mnemonic { short opcode; const char *name; };
struct mini_mnemonic *mini_mnemonics;

// identifies a register.
// 0 is never a valid register ID.
// The low 16 bits encodes the 'name' of the register,
// the high 16 bits encodes the width of the register.
// The width can never be 0 for a valid register.
// Different register ID can correspond to register with
// same name, e.g. YMM vs XMM and RAX vs EAX on x86.
typedef int32_t regid_t;

#define S2_OBJ_TYPE_MINI_INSTR 0x2043

// 2026-08-14:
// Fulfills 2 roles:
// 1. Represents data flow dependencies in expressions, coloring
//    nodes with cookies as mentioned in "plans-2026-08-01.md".
// 2. Helps determine operand registers as well as reload and
//    spill eviction sequences.
typedef struct mInstr mInstr_t;
struct mInstr {
    s2obj_base;

    // metadata attached to `lalr_prod_t` nodes.
    bool reuseCandidate;

    // 2026-08-25:
    // It's evaluated, and is waiting for its co-operand
    // to compute an expression. If not, Omega will not
    // indicate this node as yet needing eviction.
    bool evaluated_waiting_for_use;
    //- bool consumed_by_user; // 2026-08-25: wild guess it's not used.

    // 2026-09-26:
    // can be considered loosely as 'non-void'.
    bool registerLoadable;

    // these two are chosen by META in consultation with Omega.
    //
    // 2026-08-28:
    // The 1st one here is the (original) computation destination.
    //
    regid_t dest_compute;
    //
    // 2026-08-28:
    // The 2nd one here is for when the operand is evicted.
    // A subsequent computation instruction (which would be
    // a parent `lalr_prod_t` node) looks at this.
    //
    regid_t dest_actual;

    // mini instruction.
    short opcode;
    short predicate;
    short type, type1;

    // usually only op and op1, occasionally op2.
    // unowned.
    mInstr_t *op, *op1, *op2;

    // 2026-08-14:
    // e.g.
    // - operand data value for IMM (2026-08-24: it's
    //   probably more efficient to put it in `misc`),
    // - `lex_token_t` for SYM, LAB,
    // - `mInstr_t` for operands that're not owned elsewhere.
    //
    // 2026-08-22:
    // owned, needs releasing.
    //
    s2obj_t *payload;

    ptrdiff_t misc;

    cookie_t cookie;

    // 2026-08-22:
    //
    // Evaluation order:
    // 1. operands, then
    // 2. pre-sequence, then
    // 3. opcode operation semantic, then
    // 4. post-sequence.
    //
    // For expressions, the pre-sequence are used to load possibly
    // evicted operands back into registers, the post-sequence are
    // used to save reused operands.
    //
    // Operand fields (i.e. `op{,[12]}`) are owned and freed externally,
    // The `pre` and `post` fields are owned by, and freed along with
    // the `mInstr_t` structure.
    //
    mInstr_t *pre, *post;
};

mInstr_t *mInstrCreate();

// 2026-09-27:
// For Debugging:

void fprint_regid(FILE *fp, regid_t x);
void fprint_operand(FILE *fp, mInstr_t *op);
void fprint_minstr(FILE *fp, mInstr_t *ins);
void print_regid(regid_t x);
void print_operand(mInstr_t *op);
void print_minstr(mInstr_t *ins);

#endif // dcc_mini_h
