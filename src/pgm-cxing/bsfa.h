/* DannyNiu/NJF, 2026-06-28. Public Domain. */

#ifndef dcc_cxing_bsfa
#define dcc_cxing_bsfa 1

#include <stdint.h>

struct bsfa { // backing storage field access.
    int32_t refcnt;
    void *backing;
    void (*freer)(void *);
};

struct fsfa { // front-side field access.
    int32_t refcnt;
    void *backing;
    struct bsfa *serving;
};

struct bsfa *BSFA_Retain(struct bsfa *bsfa);
struct fsfa *FSFA_Retain(struct fsfa *fsfa);
void BSFA_Release(struct bsfa *bsfa);
void FSFA_Release(struct fsfa *fsfa);

#endif /* dcc_cxing_bsfa */
