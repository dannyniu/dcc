/* DannyNiu/NJF, 2026-06-18. Public Domain. */

# if defined(_WIN32)
#   include "cxing-stdlib-thrd-WinNT.bits.h"

# elif __has_include(<pthread.h>)
#   include "cxing-stdlib-thrd-Posix.bits.h"

# else
#   error "Multi-threading hasn't been implemented for this platform yet."
# endif
