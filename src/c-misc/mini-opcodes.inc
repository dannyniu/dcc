m(NOP, = 0), // also used for attaching metadata to `lalr_prod_t` nodes.

m(IMM),
m(SYM),
m(LAB),

m(JMP),
m(CALL),
m(RET),

m(LD),
m(ST),
m(AS),
m(ENS),
m(DES),

m(ADD),
m(SUB),
m(MUL),
m(DIV),
m(REM),

m(LSH),
m(RSH),
m(ROL),
m(ROR),

m(AND),
m(OR),
m(XOR),

m(NOT),
m(NEG), // May be problematic with sNaN for `float`, `double`, etc.
m(FLIP), // Optimize away double-complement in MINI (TODO 2026-08-12).

m(CMP),
m(CVT),
