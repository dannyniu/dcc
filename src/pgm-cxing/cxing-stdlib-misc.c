/* DannyNiu/NJF, 2026-05-08. Public Domain. */

#include "cxing-stdlib.h"

int CxingInitialization_DefineStandardLibrary()
{
    cxing_builtin_def_t *CxingStandardLibraries[] = {
        CxingStdlibStructBuiltins,
        CxingStdlibIoBuiltins,
        CxingStdlibMathBuiltins,
        CxingStdlibRegexBuiltins,
        NULL,
    };
    int i, j;

    for(j=0; CxingStandardLibraries[j]; j++)
    {
        for(i=0; CxingStandardLibraries[j][i].name; i++)
        {
            if( 0 != CxingBuiltinsExtend(
                    CxingStandardLibraries[j][i].name,
                    CxingStandardLibraries[j][i].val) )
            {
                return -1;
            }
        }
    }
    return 0;
}
