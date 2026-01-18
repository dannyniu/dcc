/* DannyNiu/NJF, 2026-01-14. Public Domain. */

#include "cxing-interp.h"
#include "runtime.h"

static uint32_t CallStub[] = {
    // Relocated to the address of CXING function call xfer.
    0x00000000, 0x00000000,

    // Relocated to the address of TU module.
    0x00000000, 0x00000000,

    // callxfer:
    //   nop
    //   nop
    // tumodule:
    //   nop
    //   nop
    // CallStub:
    //   SUB SP, SP, 16
    //   STR X30, SP
    //   BL +0
    //   MOV X2, X30
    //   LDR X3, tumodule
    //   LDR X4, callxfer
    //   BLR X4
    //   LDR X30, SP
    //   ADD SP, SP, 16
    //   RET

    0xd1000000 | (16 << 10) | (31 << 5) | 31, // SUB (immediate),
    0xf9000000 | (31 << 5) | 30, // STR (immediate, unsigned offset),
    0x94000000 | 1, // BL,
    0xaa0003e0 | (30 << 16) | 2, // MOV (register),
    0x58000000 | (0x7fffa << 5) | 3, // LDR (literal),
    0x58000000 | (0x7fff7 << 5) | 4, // LDR (literal),
    0xd63f0000 | (4 << 5), // BLR,
    0xf8400400 | (31 << 5) | 30, // LDR (immediate),
    0x91000000 | (16 << 10) | (31 << 5) | 31, // ADD (immediate),
    0xd65f0000 | (30 << 5),
};

static struct value_nativeobj CallXfer(
    int argn, struct value_nativeobj args[],
    uintptr_t referer, cxing_module_t *module)
{
    uint8_t *rp = (uint8_t *)referer;
    ptrdiff_t stind;

    s2data_t *key;
    lalr_prod_t *funcdef;
    int subret;

    rp -= 0x1c; // The address of the `MOV X3, X30` instruction.
    stind = rp - module->CallStubs;
    stind /= sizeof(CallStub);
    key = module->SymTab[stind];

    subret = s2dict_get_T(lalr_prod_t)(
        module->entities, key, &funcdef);

    if( subret == s2_access_error )
    {
        CxingDiagnose("[%s]: "
                      "Error retrieving entity definitions "
                      "for %s.\n", __func__, key);
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    else if( subret == s2_access_nullval )
    {
        CxingDiagnose("[%s]: "
                      "Definition for %s not found - "
                      "this shouldn't happen if the call stub "
                      "for the function exists.\n", __func__, key);
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    if( cxing_grammar_rules[funcdef->semantic_rule] != funcdecl_subr &&
        cxing_grammar_rules[funcdef->semantic_rule] != funcdecl_method )
    {
        CxingDiagnose("[%s]: Definition for %s is not a function.\n",
                      __func__, key);
        return (struct value_nativeobj){
            .proper.p = NULL,
            .type = (const void *)&type_nativeobj_morgoth };
    }

    return CxingExecuteFunction(
        module,
        funcdef->terms[3].production,
        funcdef->terms[2].production,
        argn, args);
}

static void Relocate(
    uint8_t *stubptr,
    cxing_module_t *module)
{
    *(uint64_t *)(stubptr + 0) = (uint64_t)CallXfer;
    *(uint64_t *)(stubptr + 8) = (uint64_t)module;
}
