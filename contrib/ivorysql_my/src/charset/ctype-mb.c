#include "ctype.h"

int my_strnncoll_mb_bin(const CHARSET_INFO *cs, const uchar *s,
                        size_t slen, const uchar *t, size_t tlen,
                        bool t_is_prefix)
{
    size_t len = Min(slen, tlen);
    int cmp = len == 0 ? 0 : memcmp(s, t, len);
    return cmp ? cmp : (int)((t_is_prefix ? len : slen) - tlen);
}

int my_strnncollsp_mb_bin(const CHARSET_INFO *cs,
                          const uchar *a, size_t a_length, const uchar *b,
                          size_t b_length)
{
    const uchar *end;
    size_t length;
    int res;

    end = a + (length = Min(a_length, b_length));
    while (a < end)
    {
        if (*a++ != *b++)
            return ((int)a[-1] - (int)b[-1]);
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
            if (*a != ' ')
                return (*a < ' ') ? -swap : swap;
        }
    }
    return res;
}

void my_hash_sort_mb_bin(const CHARSET_INFO *cs,
                         const uchar *key, size_t len, uint64 *nr1,
                         uint64 *nr2)
{
    const uchar *pos = key;

    /*
       Remove trailing spaces. We have to do this to be able to compare
      'A ' and 'A' as identical
    */
    key = skip_trailing_space(key, len);

    for (; pos < key; pos++)
    {
        nr1[0] ^=
            (uint64)((((uint)nr1[0] & 63) + nr2[0]) * ((uint)*pos)) + (nr1[0] << 8);
        nr2[0] += 3;
    }
}

#define my_strnxfrm_mb_non_ascii_char(cs, dst, src, se)                             \
    {                                                                               \
        int val = cs->cset->ismbchar(cs, (const char *)src, (const char *)se);      \
        if (val == 4)                                                               \
            *dst++ = *src++;                                                        \
        if (val >= 3)                                                               \
            *dst++ = *src++;                                                        \
        if (val >= 2)                                                               \
            *dst++ = *src++;                                                        \
        if (val == 0)                                                               \
            *dst++ = *src++; /* byte in range 0x80..0xFF which is not MB head */    \
    }

size_t
my_strnxfrm_mb(const CHARSET_INFO *cs, uchar *dst, size_t dstlen,
               uint nweights, const uchar *src, size_t srclen, 
               uint flags)
{
    uchar *d0 = dst;
    uchar *de = dst + dstlen;
    const uchar *se = src + srclen;
    const uchar *sort_order = cs->sort_order;

    assert(cs->mbmaxlen <= 4);

    /*
      If "srclen" is smaller than both "dstlen" and "nweights"
      then we can run a simplified loop -
      without checking "nweights" and "de".
    */
    if (dstlen >= srclen && nweights >= srclen)
    {
        if (sort_order)
        {
            /* Optimized version for a case insensitive collation */
            for (; src < se; nweights--)
            {
                if (*src < 128) /* quickly catch ASCII characters */
                    *dst++ = sort_order[*src++];
                else
                    my_strnxfrm_mb_non_ascii_char(cs, dst, src, se);
            }
        }
        else
        {
            /* Optimized version for a case sensitive collation (no sort_order) */
            for (; src < se; nweights--)
            {
                if (*src < 128) /* quickly catch ASCII characters */
                    *dst++ = *src++;
                else
                    my_strnxfrm_mb_non_ascii_char(cs, dst, src, se);
            }
        }
        goto pad;
    }

    /*
      A thorough loop, checking all possible limits:
      "se", "nweights" and "de".
    */
    for (; src < se && nweights && dst < de; nweights--)
    {
        int chlen;
        if (*src < 128 || !(chlen = cs->cset->ismbchar(cs, (const char *)src,
                                                       (const char *)se)))
        {
            /* Single byte character */
            *dst++ = sort_order ? sort_order[*src++] : *src++;
        }
        else
        {
            /* Multi-byte character */
            size_t len = (dst + chlen <= de) ? chlen : de - dst;
            memcpy(dst, src, len);
            dst += len;
            src += len;
        }
    }

pad:
    return my_strxfrm_pad_desc_and_reverse(cs, d0, dst, de, nweights, flags, 0);
}

MY_COLLATION_HANDLER my_collation_mb_bin_handler = {
    NULL,
    NULL,
    my_strnncoll_mb_bin,
    my_strnncollsp_mb_bin,
    my_strnxfrm_mb,
    my_strnxfrmlen_simple,
    my_hash_sort_mb_bin
};
