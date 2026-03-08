/* DannyNiu/NJF, 2026-02-25. Public Domain. */

#ifndef DCC_RevFinDict_H
#define DCC_RevFinDict_H

// A dictionary that finalizes its members in reverse order of insertion.

#include <s2dict.h>

#define S2_OBJ_TYPE_RFDICT 0x2032
#define s2_is_rfdict(obj) (((s2obj_t *)obj)->type == 0x2032)

#define T struct rfdict_ctx
typedef T rfdict_t;

T {
    s2obj_base;
    s2dict_t *indtab;
    long len;
    long cap;
    s2obj_t **valtab;
};

// Refer to docs for `s2dict_t`.
T *rfdict_create();
int rfdict_get(T *vd, s2data_t *key, s2obj_t **out);
int rfdict_set(T *vd, s2data_t *key, s2obj_t *value, int semantic);
#define rfdict_get_T(membertype) \
    ((int (*)(rfdict_t *dict, s2data_t *key, membertype **out))rfdict_get)

#ifndef DCC_Implementing_RFDict
#undef T
#endif /* DCC_Implementing_RFDict */

#endif /* DCC_RevFinDict_H */
