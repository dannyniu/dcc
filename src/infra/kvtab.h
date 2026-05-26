/* DannyNiu/NJF, 2026-05-03. Public Domain. */

// light-weight map, for small number of key-value pairs.
// entries are assumed to be free of reference cycles, and
// as such they're retain'd and release'd (rather than kept' and left').

#ifndef dcc_infra_kvtab_h
#define dcc_infra_kvtab_h 1

#include <s2obj.h>

#define S2_OBJ_TYPE_KVTAB 0x2033
#define s2_is_kvtab(obj) (((s2obj_t *)obj)->type == S2_OBJ_TYPE_KVTAB)

#define T struct kvtab_ctx
typedef T kvtab_t;
struct kvent {
    s2obj_t *v;
    const char *k;
};

// 2026-05-05: all fields' public.
T {
    s2obj_base;

    // count is int rather than long.
    // although not good for alignment,
    // there really isn't many members.
    int count;

    struct kvent entries[1];
};

T *kvtab_create(int count);
void *kvtab_get(T *x, const char *k);

#ifndef DCC_Implementing_KVTab
#undef T
#endif /* DCC_Implementing_Tuple */

#endif /* dcc_infra_tuple_h */
