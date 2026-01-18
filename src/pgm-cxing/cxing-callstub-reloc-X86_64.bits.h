/* DannyNiu/NJF, 2026-01-14. Public Domain. */

#include "cxing-interp.h"
#include "runtime.h"

static uint8_t CallStub[] = {
    // Relocated to the address of CXING function call xfer.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // Relocated to the address of TU module.
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

    // `CALL` the next instruction.
    0xe8, 0x00, 0x00,  0x00, 0x00,

    // MOV RDX, [RSP]
    0x48, 0x8b, 0x14, 0x24,

    // MOV RCX, [<Address of the Module>]
    0x48, 0x8b, 0x0d,
    0xe8, 0xff, 0xff, 0xff,

    // MOV R8, [<Address of the Stub>]
    0x4c, 0x8b, 0x05,
    0xd9, 0xff, 0xff, 0xff,

    // CALL R8
    0x41, 0xff, 0xd0,

    // ADD RSP, 8
    0x48, 0x83, 0xc4, 0x08,

    // RET
    0xc3,
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

    rp -= 0x15; // The address of the `POP RDX` instruction.
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
