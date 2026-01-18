/* DannyNiu/NJF, 2026-01-11. Public Domain. */

#include "runtime.h"

#include <sys/mman.h>

static void *VirtAlloc_Writable(size_t cnt, size_t sz)
{
    void *ret = mmap(
        NULL, cnt*sz,
        PROT_READ|PROT_WRITE,
        MAP_ANON|MAP_PRIVATE, -1, 0);

    if( ret == MAP_FAILED )
    {
        perror("VirtAlloc_Writable");
        CxingFatal("[%s]: Unable to obtain writable page(s).\n", __func__);
    }
    return ret;
}

static bool MemProt_Executable(void *p, size_t cnt, size_t sz)
{
    return mprotect(p, cnt*sz, PROT_EXEC|PROT_READ) == 0;
}

static void VirtFree(void *p, size_t cnt, size_t sz)
{
    munmap(p, cnt*sz);
}
