/* DannyNiu/NJF, 2026-01-13. Public Domain. */

#include "common.h"

// Inspiration: https://github.com/cpredef/predef

# if defined(_WIN32)
#   include "cxing-callstub-mman-WinNT.bits.h"

#   if defined(__amd64__) || defined(__x86_64__)
#     include "cxing-callstub-reloc-WinNT-X64.bits.h"
#   elif defined(__aarch64__)
#     include "cxing-callstub-reloc-AArch64.bits.h"
#   else
#     error "Call stub bits hasn't been implemented for this platform yet."
#   endif

# elif __has_include(<unistd.h>)
#   include <unistd.h>
#   if _POSIX_VERSION > 0
#     include "cxing-callstub-mman-Posix.bits.h"

#     if defined(__amd64__) || defined(__x86_64__)
#       include "cxing-callstub-reloc-X86_64.bits.h"
#     elif defined(__aarch64__)
#       include "cxing-callstub-reloc-AArch64.bits.h"
#     else
#       error "Call stub bits hasn't been implemented for this platform yet."
#     endif

#   else
#     error "Not a POSIX-conforming system?"
#   endif /* _POSIX_VERSION */

# else
#   error "Call stub bits hasn't been implemented for this platform yet."
# endif

cxing_platform_abi_bridge_t cxing_callxfer_bridge = {
    .callstub = (uint8_t *)CallStub,
    .sz_callstub = sizeof(CallStub),
    .sz_relocdat = 16,
    .relocator = Relocate,
    .prepare_memory_for_stubs = VirtAlloc_Writable,
    .make_stub_memory_executable = MemProt_Executable,
    .free_callstub_memory = VirtFree,
};
