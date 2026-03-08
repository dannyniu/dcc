/* DannyNiu/NJF, 2026-03-08. Public Domain. */

#include <s2data.h>

s2data_t s2_true_def = (s2data_t){
    .base.type = S2_OBJ_TYPE_INT(1),
    .len = 1,
    .buf[0] = 1,
};

s2data_t s2_false_def = (s2data_t){
    .base.type = S2_OBJ_TYPE_INT(1),
    .len = 1,
    .buf[0] = 0,
};
