/* DannyNiu/NJF, 2026-07-05. Public Domain. */

#include "dequoting.h"
#include "diagnose.h"
#include <s2data.h>

static int Base36_Char2Int(int c)
{
    if( '0' <= c && c <= '9' )
        return c - '0';

    if( 'A' <= c && c <= 'Z' )
        return 10 + c - 'A';

    if( 'a' <= c && c <= 'z' )
        return 10 + c - 'a';

    else return -1; // caller must catch error!
}

static int32_t ChrLit_Unquote(const uint8_t **esc)
{
    const uint8_t *ptr = *esc;
    int32_t c, d, t;
    enum {
        sluq_init,
        sluq_fin,
        sluq_backslash,
        sluq_hex1,
        sluq_hex2,
        sluq_octal1, // unused placeholder, pseudo-state.
        sluq_octal2,
        sluq_octal3,
        // TODO: Unicode Literals.
        sluq_unicode1,
        sluq_unicode2,
        sluq_unicode3,
        sluq_unicode4,
        sluq_Unicode1,
        sluq_Unicode2,
        sluq_Unicode3,
        sluq_Unicode4,
        sluq_Unicode5,
        sluq_Unicode6,
        sluq_Unicode7,
        sluq_Unicode8,
        sluq_UnicodeSeq,
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

            else if( ptr[t] == 'e' ) // non-standard in C.
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

            else if( ptr[t] == 'u' )
            {
                fsm = sluq_unicode1;
            }

            else if( ptr[t] == 'U' )
            {
                fsm = sluq_Unicode1;
            }

            else
            {
                fprintf(stderr, "[%s]: Unrecognized escape byte: 0x%02x!\n", __func__, ptr[t]);
            }
        }

        else if( fsm == sluq_hex1 )
        {
            d = Base36_Char2Int(ptr[t]);
            if( 0 > d || d >= 16 )
            {
                fprintf(stderr, "[%s]: Erroneous digit in hex literal: '%c' (%d)!\n", __func__, ptr[t], ptr[t]);
            }

            c = d << 4;
            fsm = sluq_hex2;
        }

        else if( fsm == sluq_hex2 )
        {
            d = Base36_Char2Int(ptr[t]);
            if( 0 > d || d >= 16 )
            {
                fprintf(stderr, "[%s]: Erroneous digit in hex literal: %d!\n", __func__, d);
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

        else if( fsm == sluq_unicode1 || fsm == sluq_Unicode1 )
        {
            if( ptr[t] == '{' )
            {
                fsm = sluq_UnicodeSeq;
            }
            else
            {
                d = Base36_Char2Int(ptr[t]);
                if( 0 > d || d >= 16 )
                {
                    fprintf(stderr, "[%s]: Erroneous digit in Unicode literal: '%c' (%d)!\n", __func__, ptr[t], ptr[t]);
                }
                c = d;
                fsm++;
            }
        }

        else if( fsm > sluq_unicode1 && fsm <= sluq_unicode4 )
        {
            d = Base36_Char2Int(ptr[t]);
            if( 0 > d || d >= 16 )
            {
                fprintf(stderr, "[%s]: Erroneous digit in Unicode literal: '%c' (%d)!\n", __func__, ptr[t], ptr[t]);
            }

            c <<= 4;
            c |= d;

            if( fsm < sluq_unicode4 ) fsm ++; else
            {
                c |= 0x40000000;
                fsm = sluq_fin;
            }
        }

        else if( fsm > sluq_Unicode1 && fsm <= sluq_Unicode8 )
        {
            d = Base36_Char2Int(ptr[t]);
            if( 0 > d || d >= 16 )
            {
                fprintf(stderr, "[%s]: Erroneous digit in Unicode literal: '%c' (%d)!\n", __func__, ptr[t], ptr[t]);
            }

            c <<= 4;
            c |= d;

            if( fsm < sluq_Unicode8 ) fsm ++; else
            {
                c |= 0x40000000;
                fsm = sluq_fin;
            }
        }

        else if( fsm > sluq_UnicodeSeq )
        {
            if( ptr[t] == '}' )
            {
                fsm = sluq_fin;
                c |= 0x40000000;
            }
            else
            {
                d = Base36_Char2Int(ptr[t]);
                if( 0 > d || d >= 16 )
                {
                    fprintf(stderr, "[%s]: Erroneous digit in Unicode literal: '%c' (%d)!\n", __func__, ptr[t], ptr[t]);
                }

                c <<= 4;
                c |= d;
            }
        }

        else assert( 0 );
    }

    *esc = ptr+t;
    return c;
}

s2data_t *StrLit_Unquote(s2data_t *base, s2data_t *lit, int prefix_skipc)
{
    uint8_t *ptr;
    size_t len, t;
    int c;

    if( !base )
    {
        if( !(base = s2data_create(0)) ) return NULL;
    }

    len = s2data_len(lit);
    ptr = s2data_map(lit, 0, len);

    ptr += prefix_skipc;
    len -= prefix_skipc;
    assert( ptr[0] == '\"' && ptr[len-1] == '\"' );

    for(t=1; t+1<len; )
    {
        const uint8_t *pa = ptr + t;
        c = ChrLit_Unquote(&pa);

        if( c & 0x40000000 )
        {
            // resulted from a Unicode literal.
            // encode them as UTF-8.
            uint32_t m = c & 0x3fffffff;
            if( m <= 0x7f )
            {
                c = s2data_putc(base, m) == 0;
            }
            else if( m <= 0x7ff )
            {
                c = s2data_putc(base, 0xc0|((m >> 6) & 0x1f)) == 0 &&
                    s2data_putc(base, 0x80|(m & 0x3f)) == 0;
            }
            else if( m <= 0xffff )
            {
                c = s2data_putc(base, 0xe0|((m >> 12) & 0x0f)) == 0 &&
                    s2data_putc(base, 0x80|((m >> 6) & 0x3f)) == 0 &&
                    s2data_putc(base, 0x80|(m & 0x3f)) == 0;
            }
            else if( m <= 0x10ffff )
            {
                c = s2data_putc(base, 0xf0|((m >> 18) & 0x0f)) == 0 &&
                    s2data_putc(base, 0x80|((m >> 12) & 0x3f)) == 0 &&
                    s2data_putc(base, 0x80|((m >> 6) & 0x3f)) == 0 &&
                    s2data_putc(base, 0x80|(m & 0x3f)) == 0;
            }
            else assert( 0 );

            if( !c )
            {
                return NULL;
            }
        }

        t = pa - ptr;
        if( s2data_putc(base, c) != 0 )
        {
            return NULL;
        }
    }

    s2data_unmap(lit);
    if( s2data_putfin(base) != 0 )
    {
        return NULL;
    }
    return base;
}
