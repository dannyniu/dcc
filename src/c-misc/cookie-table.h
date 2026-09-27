/* DannyNiu/NJF, 2026-08-08. Public Domain */

#ifndef c_misc_cookie_table_h
#define c_misc_cookie_table_h 1

#include "../common.h"

#define ZSALT_LEN 16
extern uint8_t zsalt[ZSALT_LEN];

// Complete type, can be declared on-stack.
typedef struct CookieTable CookieTable;

// Complete type. `memcmp`-able.
typedef struct {
    uint8_t cookie_bits[15];
    uint8_t cookie_type;
} cookie_t;

// See "plan-2026-08-01.md" (or later document(s)).
enum {
    // in one sense, the cookie value is an/the invalid one.
    // in another sense, this is the 'uninitialized' value.
    cookie_type_invalid = 0,

    // deterministically hashed from operands - i.e. branch nodes.
    cookie_type_hashed,

    // hashed from constant literal sources - i.e. leaf nodes.
    cookie_type_constlit,

    // leaf nodes that aren't 'safe' in the HTTP sense of the term. e.g.:
    // - loads from `volatile`s.
    // - return value of function calls.
    cookie_type_distinct,

    // an external declaration symbol to be relocated.
    cookie_type_relocsym,
};

// default-initialized (i.e. all-zero, `calloc`'d).
struct CookieTableEntry
{
    cookie_t cookie;

    // info>0, subtable==nullptr:
    // info contains stats for the cookie.
    //
    // info==0, subtable==nullptr:
    // entry is available.
    //
    // info==0, subtable!=nullptr:
    // there had been collision, consult subtable.
    intptr_t info;
    CookieTable *subtable;
};

struct CookieTable {
    struct CookieTableEntry entries[256];
};

// Just destroy content, doesn't do the freeing.
void CookieTable_Destroy(CookieTable *tab);

// Returns the memory location that keeps the stat of the cookie.
// This location may be moved on subsequent calls, but its value will be preserevd.
intptr_t *CookieTable_pEntry(CookieTable *root, const void *cookie);

#endif // c_misc_cookie_table_h
