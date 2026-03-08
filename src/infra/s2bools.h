/* DannyNiu/NJF, 2026-03-08. Public Domain. */

#ifndef dcc_s2bools_h
#define dcc_s2bools_h

#include <s2data.h>

extern s2data_t s2_true_def, s2_false_def;
#define s2_true ((s2obj_t *)&s2_true_def)
#define s2_false ((s2obj_t *)&s2_false_def)

#endif /* dcc_s2bools_h */
