/* DannyNiu/NJF, 2026-06-19. Public Domain. */

#include "runtime.h"

#if _WIN32
__declspec(dllexport)
#endif // _WIN32

struct value_nativeobj CxingSoLoadTestMe(
    int argn, struct value_nativeobj args[])
{
    printf("Hello!\n"
           "This function is intended to be invoked by the\n"
           "dynamic linking mechanism. You've provided:\n"
           "- %d arguments,\n"
           "- %p points to them.\n",
           argn, args);
    printf("Now an integer will be returned.\n");
    return (struct value_nativeobj){
        .proper.l = 19491001,
        .type = (const void *)&type_nativeobj_long };
}
