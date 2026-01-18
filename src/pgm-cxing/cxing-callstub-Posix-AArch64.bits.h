/* DannyNiu/NJF, 2026-01-11. Public Domain. */

#include "cxing-interp.h"

#include <sys/mman.h>

#include "cxing-callstub-reloc-AArch64.bits.h"
#include "cxing-callstub-mman-Posix.bits.h"

cxing_platform_abi_bridge_t cxing_callxfer_bridge = {
    .callstub = (uint8_t *)CallStub,
    .sz_callstub = sizeof(CallStub),
    .sz_relocdat = 16,
    .relocator = Relocate,
    .prepare_memory_for_stubs = VirtAlloc_Writable,
    .make_stub_memory_executable = MemProt_Executable,
    .free_callstub_memory = VirtFree,
};
