/* DannyNiu/NJF, 2026-06-10. Public Domain. */

# if defined(_WIN32)
#   include "cxing-stdlib-proc-WinNT.bits.h"

# elif __has_include(<unistd.h>)
#   include "cxing-stdlib-proc-Posix.bits.h"

# else
#   error "Process management hasn't been implemented for this platform yet."
# endif
