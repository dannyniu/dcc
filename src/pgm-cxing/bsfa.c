/* DannyNiu/NJF, 2026-06-28. Public Domain. */

#include "bsfa.h"
#include <stdlib.h>

// to intercpet memory management calls.
#include "common.h"

struct bsfa *BSFA_Retain(struct bsfa *bsfa)
{
    bsfa->refcnt ++;
    return bsfa;
}

struct fsfa *FSFA_Retain(struct fsfa *fsfa)
{
    fsfa->refcnt ++;
    return fsfa;
}

void BSFA_Release(struct bsfa *bsfa)
{
    if( --bsfa->refcnt ) return;
    bsfa->freer(bsfa->backing);
    free(bsfa);
}

void FSFA_Release(struct fsfa *fsfa)
{
    if( --fsfa->refcnt ) return;
    BSFA_Release(fsfa->serving);
    free(fsfa);
}
