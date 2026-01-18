/* DannyNiu/NJF, 2026-01-14. Public Domain. */

#include <windows.h>

static void *VirtAlloc_Writable(size_t cnt, size_t sz)
{
    return VirtualAlloc(
        NULL, cnt*sz, MEM_COMMIT|MEM_RESERVE,
        PAGE_EXECUTE_READWRITE);
}

static bool MemProt_Executable(void *p, size_t cnt, size_t sz)
{
    DWORD oldProt;
    return VirtualProtect(p, cnt*sz, PAGE_EXECUTE_READ, &oldProt) &&
        FlushInstructionCache(GetCurrentProcess(), p, cnt*sz);
}

static void VirtFree(void *p, size_t cnt, size_t sz)
{
    (void)cnt;
    (void)sz;
    VirtualFree(p, 0, MEM_RELEASE);
}
