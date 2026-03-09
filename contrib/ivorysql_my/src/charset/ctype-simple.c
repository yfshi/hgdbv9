#include "ctype.h"

void my_fill_8bit(const CHARSET_INFO *cs, char *s, size_t l, int fill)
{
    memset(s, fill, l);
}

void my_hash_sort_simple(const CHARSET_INFO *cs, const uchar *key, size_t len,
                         uint64 *nr1, uint64 *nr2)
{
    const uchar *sort_order = cs->sort_order;
    const uchar *end;
    uint64 tmp1;
    uint64 tmp2;

    /*
      Remove end space. We have to do this to be able to compare
      'A ' and 'A' as identical
    */
    end = skip_trailing_space(key, len);

    tmp1 = *nr1;
    tmp2 = *nr2;

    for (; key < end; key++)
    {
        tmp1 ^=
            (uint64)((((uint32)tmp1 & 63) + tmp2) * ((uint32)sort_order[(uint32)*key])) +
            (tmp1 << 8);
        tmp2 += 3;
    }

    *nr1 = tmp1;
    *nr2 = tmp2;
}

size_t
my_strnxfrmlen_simple(const CHARSET_INFO *cs, size_t len)
{
    return len * (cs->strxfrm_multiply ? cs->strxfrm_multiply : 1);
}

int 
my_strnncoll_simple(const CHARSET_INFO *cs, const uchar *s, size_t slen, const uchar *t, size_t tlen, bool t_is_prefix)
{
    size_t len = (slen > tlen) ? tlen : slen;
    const uchar *map = cs->sort_order;
    if (t_is_prefix && slen > tlen)
        slen = tlen;
    while (len--)
    {
        if (map[*s++] != map[*t++])
            return ((int)map[s[-1]] - (int)map[t[-1]]);
    }
    /*
    We can't use (slen - tlen) here as the result may be outside of the
    precision of a signed int
    */
    return slen > tlen ? 1 : slen < tlen ? -1
                                         : 0;
}

int my_strnncollsp_simple(const CHARSET_INFO *cs, const uchar *a, size_t a_length, const uchar *b, size_t b_length)
{
    const uchar *map = cs->sort_order, *end;
    size_t length;
    int res;

    end = a + (length = Min(a_length, b_length));
    while (a < end)
    {
        if (map[*a++] != map[*b++])
            return ((int)map[a[-1]] - (int)map[b[-1]]);
    }
    res = 0;
    if (a_length != b_length)
    {
        int swap = 1;
        /*
        Check the next not space character of the longer key. If it's < ' ',
        then it's smaller than the other key.
        */
        if (a_length < b_length)
        {
            /* put shorter key in s */
            a_length = b_length;
            a = b;
            swap = -1; /* swap sign of result */
            res = -res;
        }
        for (end = a + a_length - length; a < end; a++)
        {
            if (map[*a] != map[(int)' '])
                return (map[*a] < map[(int)' ']) ? -swap : swap;
        }
    }
    return res;
}

void
my_strxfrm_desc_and_reverse(uchar *str, uchar *strend,
                            uint32 flags, uint32 level)
{
    if (flags & (MY_STRXFRM_DESC_LEVEL1 << level))
    {
        if (flags & (MY_STRXFRM_REVERSE_LEVEL1 << level))
        {
            for (strend--; str <= strend;)
            {
                uchar tmp = *str;
                *str++ = ~*strend;
                *strend-- = ~tmp;
            }
        }
        else
        {
            for (; str < strend; str++)
                *str = ~*str;
        }
    }
    else if (flags & (MY_STRXFRM_REVERSE_LEVEL1 << level))
    {
        for (strend--; str < strend;)
        {
            uchar tmp = *str;
            *str++ = *strend;
            *strend-- = tmp;
        }
    }
}

size_t
my_strxfrm_pad_desc_and_reverse(const CHARSET_INFO *cs,
                                uchar *str, uchar *frmend, uchar *strend,
                                uint32 nweights, uint32 flags, uint32 level)
{
    if (nweights && frmend < strend && (flags & MY_STRXFRM_PAD_WITH_SPACE))
    {
        uint32 fill_length = Min((uint32)(strend - frmend), nweights * cs->mbminlen);
        cs->cset->fill(cs, (char *)frmend, fill_length, cs->pad_char);
        frmend += fill_length;
    }
    my_strxfrm_desc_and_reverse(str, frmend, flags, level);
    if ((flags & MY_STRXFRM_PAD_TO_MAXLEN) && frmend < strend)
    {
        size_t fill_length = strend - frmend;
        cs->cset->fill(cs, (char *)frmend, fill_length, cs->pad_char);
        frmend = strend;
    }
    return frmend - str;
}

size_t
my_strnxfrm_simple(const CHARSET_INFO *cs,
                   uchar *dst, size_t dstlen, uint32 nweights,
                   const uchar *src, size_t srclen, uint32 flags)
{
    const uchar *map = cs->sort_order;
    uchar *d0 = dst;
    const uchar *end;
    const uchar *remainder;
    size_t frmlen;
    if ((frmlen = Min(dstlen, nweights)) > srclen)
        frmlen = srclen;
    end = src + frmlen;

    // Do the first few bytes.
    remainder = src + (frmlen % 8);
    for (; src < remainder;)
        *dst++ = map[*src++];

    // Unroll loop for rest of string.
    for (; src < end;)
    {
        *dst++ = map[*src++];
        *dst++ = map[*src++];
        *dst++ = map[*src++];
        *dst++ = map[*src++];
        *dst++ = map[*src++];
        *dst++ = map[*src++];
        *dst++ = map[*src++];
        *dst++ = map[*src++];
    }
    return my_strxfrm_pad_desc_and_reverse(cs, d0, dst, d0 + dstlen,
                                           (uint32)(nweights - frmlen), flags, 0);
}

MY_COLLATION_HANDLER my_collation_8bit_simple_ci_handler = {
    NULL,
    NULL,
    my_strnncoll_simple,
    my_strnncollsp_simple,
    my_strnxfrm_simple,
    my_strnxfrmlen_simple,
    my_hash_sort_simple
};

MY_CHARSET_HANDLER simple_charset_handler = {
    NULL,
    NULL,
    NULL,
    my_fill_8bit
};
