/* DannyNiu/NJF, 2024-12-27. Public Domain. */

#ifndef SafeTypes2_StringVector_H
#define SafeTypes2_StringVector_H

// This facility is for converting between strings and integer indicies.
// Whenever a new string whose index is requested, a new entry is made for it,
// and when an existing string is queried again, the index of that entry
// is looked up and returned.
//
// The null string is always present as entry 0 in all instances.

#include <s2data.h>

#define S2_OBJ_TYPE_STRVEC 0x2031
#define s2_is_strvec(obj) (((s2obj_t *)obj)->type == 0x2031)

#define T struct strvec_ctx
typedef T strvec_t;

T {
    s2obj_base;
    s2data_t *ptab;
};

T *strvec_create();
int32_t     strvec_len  (T *restrict ctx);
int32_t     strvec_str2i(T *restrict ctx, const char *s);
const char *strvec_i2str(T *restrict ctx, int32_t i);

#ifndef safetypes2_implementing_strvec
#undef T
#endif /* safetypes2_implementing_strvec */

#endif /* SafeTypes2_StringVector_H */
