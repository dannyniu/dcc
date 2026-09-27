/* DannyNiu/NJF, 2026-08-08. Public Domain */

#include "cookie-table.h"

uint8_t zsalt[ZSALT_LEN] = {0};

void CookieTable_Destroy(CookieTable *tab)
{
    int i;
    for(i=0; i<256; i++)
    {
        if( tab->entries[i].subtable )
        {
            assert( tab->entries[i].info == 0 );
            CookieTable_Destroy(tab->entries[i].subtable);
            free(tab->entries[i].subtable);
        }
    }
    memset(tab, 0, sizeof(CookieTable));
}

intptr_t *CookieTable_pEntry(CookieTable *root, const void *cookie)
{
    int level = 0;
    const uint8_t *cptr = cookie;
    CookieTable *tab = root, *tmp;

    while( true )
    {
        assert( level + 1 < 16 );

        // available entry, fill cookie first.
        if( tab->entries[ cptr[level] ].info == 0 &&
            !tab->entries[ cptr[level] ].subtable )
            memcpy(&tab->entries[ cptr[level] ].cookie, cookie, 16);

        // cookie matches, this is the entry.
        if( memcmp(cookie, &tab->entries[ cptr[level] ].cookie, 16) == 0 )
            return &tab->entries[ cptr[level] ].info;

        // already-resolved collision, continue.
        if( (tmp = tab->entries[ cptr[level] ].subtable) )
        {
            tab = tmp;
            level ++;
            continue;
        }

        // try to resolve collision.
        tmp = calloc(1, sizeof(CookieTable));
        assert( tmp );

        memcpy(tmp->entries + cptr[level+1],
               tab->entries + cptr[level],
               sizeof(struct CookieTableEntry));

        memset(tab->entries + cptr[level], 0,
               sizeof(struct CookieTableEntry));
        tab->entries[ cptr[level] ].subtable = tmp;

        level ++;
        continue;
    }
}
