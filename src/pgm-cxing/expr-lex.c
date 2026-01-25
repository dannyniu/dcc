/* DannyNiu/NJF, 2026-01-24. Public Domain. */

#include "expr.h"
#include "runtime.h"

uint64_t Radix64Literal(const char *x)
{
    uint64_t ret = 0;
    size_t t = 0;
    if( x[0] != '0' || x[1] != '\\' )
        return -1;

    while( x[t] )
    {
        unsigned c;
        if( 'A' <= x[t] && x[t] <= 'Z' )
            c = 0 + x[t] - 'A';
        else if( 'a' <= x[t] && x[t] <= 'z' )
            c = 26 + x[t] - 'a';
        else if( '0' <= x[t] && x[t] <= '9' )
            c = 52 + x[t] - '0';
        else if( x[t] == '.' )
            c = 62;
        else if( x[t] == '_' )
            c = 63;
        else assert( 0 );

        ret = (ret << 6) | c;
    }

    return ret;
}

int Base36_Char2Int(int c)
{
    if( '0' <= c && c <= '9' )
        return c - '0';

    if( 'A' <= c && c <= 'Z' )
        return 10 + c - 'A';

    if( 'a' <= c && c <= 'z' )
        return 10 + c - 'a';

    else return -1; // caller must catch error!
}

int ChrLit_Unquote(const uint8_t **esc)
{
    const uint8_t *ptr = *esc;
    int c, d, t;
    enum {
        sluq_init,
        sluq_fin,
        sluq_backslash,
        sluq_hex1,
        sluq_hex2,
        sluq_octal1, // unused placeholder, pseudo-state.
        sluq_octal2,
        sluq_octal3,
    } fsm = sluq_init;

    for(t=0; ptr[t]; t++)
    {
        if( fsm == sluq_fin )
        {
            break;
        }

        else if( fsm == sluq_backslash )
        {
            c = 0;

            if( ptr[t] == 'x' || ptr[t] == 'X')
                fsm = sluq_hex1;

            else if( '0' <= ptr[t] && ptr[t] <= '3' )
            {
                c = Base36_Char2Int(ptr[t]);
                fsm = sluq_octal2;
            }

            else if( ptr[t] == 'a' )
            {
                c = '\a';
                fsm = sluq_fin;
            }

            else if( ptr[t] == 'b' )
            {
                c = '\b';
                fsm = sluq_fin;
            }

            else if( ptr[t] == 'e' )
            {
                c = '\x1b';
                fsm = sluq_fin;
            }

            else if( ptr[t] == 'f' )
            {
                c = '\f';
                fsm = sluq_fin;
            }

            else if( ptr[t] == 'n' )
            {
                c = '\n';
                fsm = sluq_fin;
            }

            else if( ptr[t] == 'r' )
            {
                c = '\r';
                fsm = sluq_fin;
            }

            else if( ptr[t] == 't' )
            {
                c = '\t';
                fsm = sluq_fin;
            }

            else if( ptr[t] == 'v' )
            {
                c = '\v';
                fsm = sluq_fin;
            }

            else if( ptr[t] == '\"' || ptr[t] == '\'' )
            {
                c = ptr[t];
                fsm = sluq_fin;
            }

            else
            {
                CxingDebug("[%s]: Unrecognized escape byte: 0x%02x!\n",
                           __func__, ptr[t]);
            }
        }

        else if( fsm == sluq_hex1 )
        {
            d = Base36_Char2Int(ptr[t]);
            if( 0 > d || d >= 16 )
            {
                CxingDebug("[%s]: Erroneous digit in hex literal: %d!\n",
                           __func__, d);
            }

            c = d << 4;
            fsm = sluq_hex2;
        }

        else if( fsm == sluq_hex2 )
        {
            d = Base36_Char2Int(ptr[t]);
            if( 0 > d || d >= 16 )
            {
                CxingDebug("[%s]: Erroneous digit in hex literal: %d!\n",
                           __func__, d);
            }

            c |= d;
            fsm = sluq_fin;
        }

        else if( fsm == sluq_octal2 )
        {
            d = Base36_Char2Int(ptr[t]);
            if( 0 > d || d >= 8 )
            {
                fsm = sluq_fin;
            }
            else
            {
                c = c * 8 + d;
                fsm = sluq_octal3;
            }
        }

        else if( fsm == sluq_octal3 )
        {
            d = Base36_Char2Int(ptr[t]);
            if( 0 > d || d >= 8 )
            {
                // nop.
            }
            else
            {
                c = c * 8 + d;
            }
            fsm = sluq_fin;
        }

        else if( fsm == sluq_init )
        {
            if( ptr[t] == '\\' )
            {
                fsm = sluq_backslash;
            }
            else
            {
                c = ptr[t];
                fsm = sluq_fin;
            }
        }

        else assert( 0 );
    }

    *esc = ptr+t;
    return c;
}

s2data_t *StrLit_Unquote(s2data_t *base, s2data_t *lit)
{
    uint8_t *ptr;
    size_t len, t;
    int c;

    if( !base )
    {
        if( !(base = s2data_create(0)) ) return NULL;
        s2obj_keep(base->pobj);
        s2obj_release(base->pobj);
    }

    len = s2data_len(lit);
    ptr = s2data_map(lit, 0, len);

    assert( ptr[0] == '\"' && ptr[len-1] == '\"' );

    for(t=1; t+1<len; )
    {
        const uint8_t *pa = ptr + t;
        c = ChrLit_Unquote(&pa);
        t = pa - ptr;
        if( s2data_putc(base, c) != 0 )
        {
            CxingDiagnose("[%s]: Unable to append bytes to string object!\n",
                          __func__);
        }
    }

    s2data_unmap(lit);
    if( s2data_putfin(base) != 0 )
    {
        CxingDiagnose("[%s]: Unable to append bytes to string object!\n",
                          __func__);
    }
    return base;
}

s2data_t *StrLit_CookRaw(s2data_t *base, s2data_t *lit)
{
    uint8_t *ptr;
    size_t len;

    if( !base )
    {
        if( !(base = s2data_create(0)) ) return NULL;
        s2obj_keep(base->pobj);
        s2obj_release(base->pobj);
    }

    len = s2data_len(lit)
        // one for the backslash,
        // two for the quotes.
        // assumes lexer and parser did their job.
        - 3;

    ptr = s2data_map(lit, 2, len);

    if( s2data_puts(base, ptr, len) != 0 )
    {
        CxingDiagnose("[%s]: Unable to append bytes to string object!\n",
                          __func__);
    }

    if( s2data_putfin(base) != 0 )
    {
        CxingDiagnose("[%s]: Unable to append bytes to string object!\n",
                          __func__);
    }

    s2data_unmap(lit);
    return base;
}
