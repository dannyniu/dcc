/* DannyNiu/NJF, 2026-06-18. Public Domain. */

#ifndef DCC_ArcBase_H
#define DCC_ArcBase_H 1

// The atomic reference counting type. Not compatible with SafeTypes2.

#include <stdatomic.h>
#include <stdlib.h>

typedef struct {
    atomic_int refcnt;
    void (*fin_cb)(void *);
} arc_base_t;

arc_base_t *arc_create(size_t sz, void (*fin_cb)(void *));
void *arc_retain(arc_base_t *obj);
void arc_release(arc_base_t *obj);

#endif /* DCC_ArcBase_H */
