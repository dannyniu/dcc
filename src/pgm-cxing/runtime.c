/* DannyNiu/NJF, 2025-10-02. Public Domain. */

#define _CRT_RAND_S // To use the `rand_s` Win32 API.
#include "runtime.h"
#include <SafeTypes2.h>

static s2data_t *PropName_copy_dat;
static s2data_t *PropName_final_dat;
struct value_nativeobj CxingPropName_copy;
struct value_nativeobj CxingPropName_final;

bool CxingRuntimeInit()
{
    FILE *csprng;
    uint8_t seed[16] = {0};
    bool ret = true;
    int i;

    //
    // Initialize seed for SipHash.

    csprng = fopen("/dev/urandom", "rb");
    if( csprng )
    {
        fread(seed, 1, sizeof(seed), csprng);
        fclose(csprng);
        if( ferror(csprng) )
        {
            CxingWarning("IO error when reading CSPRNG for seeding SipHash.");
        }
        siphash_setkey(seed, sizeof(seed));
    }
    else
    {
#ifdef _WIN32
        unsigned int buf; // Eventhough we know the size of int on windows.
        while( i < sizeof(seed) )
        {
            rand_s(&buf);
            seed[i++] = buf;
        }
#else // Not Win32 platform.
        CxingWarning("IO error when opening CSPRNG for seeding SipHash.");
#endif // _WIN32
    }

    //
    // Initialize value native objects for string constants.

    i = 0;

    PropName_final_dat = s2data_from_str("__copy__");
    if( !PropName_final_dat )
    {
        CxingFatal("String constant '__copy__' failed to allocate!");
        ret = false;
        goto strings_dealloc;
    }

    i++;

    PropName_final_dat = s2data_from_str("__final__");
    if( !PropName_final_dat )
    {
        CxingFatal("String constant '__final__' failed to allocate!");
        ret = false;
        goto strings_dealloc;
    }

    i++;

    CxingPropName_final = (struct value_nativeobj){
        .proper.p = PropName_final_dat,
        .type = type_nativeobj_s2data_str,
    };

strings_dealloc:
    switch( i ) {
    case 2: s2obj_release(PropName_final_dat->pobj); // FALLTHRU.
    case 1: s2obj_release(PropName_copy_dat->pobj); // FALLTHRU.
    }

    return ret;
}
