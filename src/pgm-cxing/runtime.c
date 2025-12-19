/* DannyNiu/NJF, 2025-10-02. Public Domain. */

#define _CRT_RAND_S // To use the `rand_s` Win32 API.
#include "runtime.h"
#include <SafeTypes2.h>

static s2data_t *PropName_copy_dat;
static s2data_t *PropName_final_dat;
static s2data_t *PropName_equals_dat;
static s2data_t *PropName_cmpwith_dat;
struct value_nativeobj CxingPropName_copy;
struct value_nativeobj CxingPropName_final;
struct value_nativeobj CxingPropName_equals;
struct value_nativeobj CxingPropName_cmpwith;

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
        size_t t = 0;
        while( t < sizeof(seed) )
        {
            rand_s(&buf);
            seed[t++] = buf;
        }
#else // Not Win32 platform.
        CxingWarning("IO error when opening CSPRNG for seeding SipHash.");
#endif // _WIN32
    }

    //
    // Initialize value native objects for string constants.

    i = 0;

    PropName_copy_dat = s2data_from_str("__copy__");
    if( !PropName_copy_dat )
    {
        CxingFatal("String constant '__copy__' failed to allocate!");
        ret = false;
        goto strings_dealloc;
    }
    CxingPropName_copy = (struct value_nativeobj){
        .proper.p = PropName_copy_dat,
        .type = type_nativeobj_s2data_str,
    };
    i++;

    PropName_final_dat = s2data_from_str("__final__");
    if( !PropName_final_dat )
    {
        CxingFatal("String constant '__final__' failed to allocate!");
        ret = false;
        goto strings_dealloc;
    }
    CxingPropName_final = (struct value_nativeobj){
        .proper.p = PropName_final_dat,
        .type = type_nativeobj_s2data_str,
    };
    i++;

    PropName_equals_dat = s2data_from_str("equals");
    if( !PropName_equals_dat )
    {
        CxingFatal("String constant 'equals' failed to allocate!");
        ret = false;
        goto strings_dealloc;
    }
    CxingPropName_equals = (struct value_nativeobj){
        .proper.p = PropName_equals_dat,
        .type = type_nativeobj_s2data_str,
    };
    i++;

    PropName_cmpwith_dat = s2data_from_str("cmpwith");
    if( !PropName_cmpwith_dat )
    {
        CxingFatal("String constant 'cmpwith' failed to allocate!");
        ret = false;
        goto strings_dealloc;
    }
    CxingPropName_cmpwith = (struct value_nativeobj){
        .proper.p = PropName_cmpwith_dat,
        .type = type_nativeobj_s2data_str,
    };
    i++;

strings_dealloc:
    switch( i ) {
    case 4: s2obj_release(PropName_cmpwith_dat->pobj); // FALLTHRU.
    case 3: s2obj_release(PropName_equals_dat->pobj); // FALLTHRU.
    case 2: s2obj_release(PropName_final_dat->pobj); // FALLTHRU.
    case 1: s2obj_release(PropName_copy_dat->pobj); // FALLTHRU.
    }

    return ret;
}

void CxingDiagnose(const char *msg)
{
    fprintf(stderr, "[CxingDiagnose]: %s\n", msg);
    exit(1);
}

void CxingWarning(const char *msg)
{
    fprintf(stderr, "[CxingWarning]: %s\n", msg);
}

void CxingFatal(const char *msg)
{
    fprintf(stderr, "[CxingFatal]: %s\n", msg);
    abort();
}
