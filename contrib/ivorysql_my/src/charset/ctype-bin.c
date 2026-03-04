#include "ctype.h"

static const uchar ctype_bin[] = {
    0,  32,  32,  32,  32,  32,  32,  32,  32,  32,  40,  40, 40, 40, 40, 32,
    32, 32,  32,  32,  32,  32,  32,  32,  32,  32,  32,  32, 32, 32, 32, 32,
    32, 72,  16,  16,  16,  16,  16,  16,  16,  16,  16,  16, 16, 16, 16, 16,
    16, 132, 132, 132, 132, 132, 132, 132, 132, 132, 132, 16, 16, 16, 16, 16,
    16, 16,  129, 129, 129, 129, 129, 129, 1,   1,   1,   1,  1,  1,  1,  1,
    1,  1,   1,   1,   1,   1,   1,   1,   1,   1,   1,   1,  16, 16, 16, 16,
    16, 16,  130, 130, 130, 130, 130, 130, 2,   2,   2,   2,  2,  2,  2,  2,
    2,  2,   2,   2,   2,   2,   2,   2,   2,   2,   2,   2,  16, 16, 16, 16,
    32, 0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,  0,  0,  0,
    0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,  0,  0,  0,
    0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,  0,  0,  0,
    0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,  0,  0,  0,
    0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,  0,  0,  0,
    0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,  0,  0,  0,
    0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,  0,  0,  0,
    0,  0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,  0,  0,  0,  0,
    0,
};

static const uchar bin_char_array[] = {
    0,   1,   2,   3,   4,   5,   6,   7,   8,   9,   10,  11,  12,  13,  14,
    15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,
    30,  31,  32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,
    45,  46,  47,  48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,
    60,  61,  62,  63,  64,  65,  66,  67,  68,  69,  70,  71,  72,  73,  74,
    75,  76,  77,  78,  79,  80,  81,  82,  83,  84,  85,  86,  87,  88,  89,
    90,  91,  92,  93,  94,  95,  96,  97,  98,  99,  100, 101, 102, 103, 104,
    105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119,
    120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134,
    135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149,
    150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164,
    165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179,
    180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194,
    195, 196, 197, 198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209,
    210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224,
    225, 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237, 238, 239,
    240, 241, 242, 243, 244, 245, 246, 247, 248, 249, 250, 251, 252, 253, 254,
    255};

static int 
my_mb_wc_bin(const CHARSET_INFO *cs, my_wc_t *wc, const uchar *str, const uchar *end)
{
    if (str >= end)
        return MY_CS_TOOSMALL;

    *wc = str[0];
    return 1;
}

static int 
my_wc_mb_bin(const CHARSET_INFO *cs, my_wc_t wc, uchar *s, uchar *e)
{
    if (s >= e)
        return MY_CS_TOOSMALL;

    if (wc < 256)
    {
        s[0] = (char)wc;
        return 1;
    }
    return MY_CS_ILUNI;
}

static int 
my_strnncoll_binary(const CHARSET_INFO *cs, const uchar *s, size_t slen, const uchar *t, size_t tlen, bool t_is_prefix)
{
    size_t len = Min(slen, tlen);
    const int cmp = len == 0 ? 0 : memcmp(s, t, len); // memcmp(a, b, 0) == 0
    return cmp ? cmp : (int)((t_is_prefix ? len : slen) - tlen);
}

static int 
my_strnncollsp_binary(const CHARSET_INFO *cs, const uchar *s, size_t slen, const uchar *t, size_t tlen)
{
    return my_strnncoll_binary(cs, s, slen, t, tlen, false);
}

static size_t 
my_strnxfrm_8bit_bin_no_pad(const CHARSET_INFO *cs, uchar *dst,
                            size_t dstlen, uint32 nweights,
                            const uchar *src, size_t srclen,
                            uint32 flags)
{
    srclen = Min(srclen, dstlen);
    srclen = Min(srclen, nweights);
    if (dst != src && srclen > 0)
        memcpy(dst, src, srclen);
    if ((flags & MY_STRXFRM_PAD_TO_MAXLEN) && srclen < dstlen)
    {
        cs->cset->fill(cs, (char *) dst + srclen, dstlen - srclen,
                       cs->pad_char);
        return dstlen;
    }
    return srclen;
}

static void 
my_hash_sort_bin(const CHARSET_INFO *cs,
                 const uchar *key, size_t len, 
                 uint64 *nr1, uint64 *nr2)
{
    const uchar *pos = key;
    uint64 tmp1;
    uint64 tmp2;

    key += len;

    tmp1 = *nr1;
    tmp2 = *nr2;

    for (; pos < key; pos++)
    {
        tmp1 ^= (uint64)((((uint32)tmp1 & 63) + tmp2) * ((uint32)*pos)) + (tmp1 << 8);
        tmp2 += 3;
    }

    *nr1 = tmp1;
    *nr2 = tmp2;
}

static int 
my_strnncoll_8bit_bin(const CHARSET_INFO *cs,
                      const uchar *s, size_t slen,
                      const uchar *t, size_t tlen,
                      bool t_is_prefix)
{
    size_t len = Min(slen, tlen);
    int cmp = memcmp(s, t, len);
    return cmp ? cmp : (int)((t_is_prefix ? len : slen) - tlen);
}

static int 
my_strnncollsp_8bit_bin(const CHARSET_INFO *cs,
                        const uchar *a, size_t a_length,
                        const uchar *b, size_t b_length)
{
    const uchar *end;
    size_t length;
    int res;
    bool diff_if_only_endspace_difference = 0;

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
        if (diff_if_only_endspace_difference)
            res = 1; /* Assume 'a' is bigger */
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

#define set_if_smaller(a,b) do { if ((a) > (b)) (a)=(b); } while(0)

static size_t
my_strnxfrm_8bit_bin(const CHARSET_INFO *cs,
                     uchar *dst, size_t dstlen, uint32 nweights,
                     const uchar *src, size_t srclen, uint32 flags)
{
    set_if_smaller(srclen, dstlen);
    set_if_smaller(srclen, nweights);
    if (dst != src)
        memcpy(dst, src, srclen);
    return my_strxfrm_pad_desc_and_reverse(cs, dst, dst + srclen, dst + dstlen,
                                           (uint32)(nweights - srclen), flags, 0);
}

static void 
my_hash_sort_8bit_bin(const CHARSET_INFO *cs,
                      const uchar *key, size_t len,
                      uint64 *nr1, uint64 *nr2)
{
    const uchar *pos = key;
    uint64 tmp1;
    uint64 tmp2;

    /*
       Remove trailing spaces. We have to do this to be able to compare
      'A ' and 'A' as identical
    */
    key = skip_trailing_space(key, len);

    tmp1 = *nr1;
    tmp2 = *nr2;

    for (; pos < (uchar *)key; pos++)
    {
        tmp1 ^= (uint64)((((uint32)tmp1 & 63) + tmp2) *
                        ((uint32)*pos)) +
                (tmp1 << 8);
        tmp2 += 3;
    }

    *nr1 = tmp1;
    *nr2 = tmp2;
}

static MY_COLLATION_HANDLER my_collation_binary_handler = {
    NULL,
    NULL,
    my_strnncoll_binary,
    my_strnncollsp_binary,
    my_strnxfrm_8bit_bin_no_pad,
    my_strnxfrmlen_simple,
    my_hash_sort_bin
};

static MY_CHARSET_HANDLER my_charset_binary_handler = {
    NULL,
    my_mb_wc_bin,
    my_wc_mb_bin,
    my_fill_8bit,
};

CHARSET_INFO my_charset_bin = {
    63,
    MY_CS_COMPILED | MY_CS_BINSORT | MY_CS_PRIMARY, /* state            */
    "binary",                                       /* csname           */
    "binary",                                       /* coll_name        */
    1,                                              /* sort_len         */
    ctype_bin,                                      /* ctype            */
    bin_char_array,                                 /* to_lower         */
    bin_char_array,                                 /* to_upper         */
    NULL,                                           /* sort_order       */
    &my_unicase_default,                            /* caseinfo         */
    1,                                              /* strxfrm_multiply */
    1,                                              /* mbminlen         */
    1,                                              /* mbmaxlen         */
    1,                                              /* mbmaxlenlen      */
    0,                                              /* pad char         */
    &my_charset_binary_handler,
    &my_collation_binary_handler,
};

MY_COLLATION_HANDLER my_collation_8bit_bin_handler = {
    NULL,
    NULL,
    my_strnncoll_8bit_bin,
    my_strnncollsp_8bit_bin,
    my_strnxfrm_8bit_bin,
    my_strnxfrmlen_simple,
    my_hash_sort_8bit_bin,
};
