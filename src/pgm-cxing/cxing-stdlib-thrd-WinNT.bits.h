/* DannyNiu/NJF, 2026-05-30. Public Domain. */

#ifdef _WIN32
#ifdef __MINGW32__
// Use the readily pthread for now.
#include "cxing-stdlib-thrd-Posix.bits.h"
#else
#error Implement Me! (Win32 Threads).
#endif
#endif
