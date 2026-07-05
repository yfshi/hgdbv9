#ifndef MYS_CTYPE_H
#define MYS_CTYPE_H

#include <assert.h>
#ifdef WIN32
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#include "postgres.h"
#include "c.h"

#include "../include/ivorysql_my.h"

#define MY_CS_REPLACEMENT_CHARACTER 0xFFFD

#define MY_CHARSET_UNDEFINED     (0)        // for unit testing
#define MY_CS_COMPILED           (1 << 0)   // compiled-in charsets
#define MY_CS_CONFIG_UNUSED      (1 << 1)   // unused bitmask
#define MY_CS_INDEX_UNUSED       (1 << 2)   // unused bitmask
#define MY_CS_LOADED             (1 << 3)   // charsets that are currently loaded
#define MY_CS_BINSORT            (1 << 4)   // if binary sort order
#define MY_CS_PRIMARY            (1 << 5)   // if primary collation
#define MY_CS_STRNXFRM           (1 << 6)   // if _not_ set, sort_order will
                                            // give same result as strnxfrm --
                                            // all new collations should have
                                            // this flag set,
                                            // do not check it in new code
#define MY_CS_UNICODE            (1 << 7)   // if a charset is BMP Unicode
#define MY_CS_READY              (1 << 8)   // if a charset is initialized
#define MY_CS_AVAILABLE          (1 << 9)   // if either compiled-in or loaded
#define MY_CS_CSSORT             (1 << 10)  // if case sensitive sort order
#define MY_CS_HIDDEN             (1 << 11)  // don't display in SHOW
#define MY_CS_PUREASCII          (1 << 12)  // if a charset is pure ascii
#define MY_CS_NONASCII           (1 << 13)  // if not ASCII-compatible
#define MY_CS_UNICODE_SUPPLEMENT (1 << 14)  // Non-BMP Unicode characters
#define MY_CS_LOWER_SORT         (1 << 15)  // if use lower case as weight

#define MY_CS_ILSEQ                 0       /* Wrong by sequence: wb_wc                   */
#define MY_CS_ILUNI                 0       /* Cannot encode Unicode to charset: wc_mb    */
#define MY_CS_TOOSMALL              -101    /* Need at least one byte:    wc_mb and mb_wc */
#define MY_CS_TOOSMALL2             -102    /* Need at least two bytes:   wc_mb and mb_wc */
#define MY_CS_TOOSMALL3             -103    /* Need at least three bytes: wc_mb and mb_wc */
#define MY_CS_TOOSMALL4             -104    /* Need at least 4 bytes: wc_mb and mb_wc */
#define MY_CS_TOOSMALLN(n)          (-100 - (n))

#define MY_STRXFRM_PAD_TO_MAXLEN    0x00000080
#define MY_STRXFRM_PAD_WITH_SPACE   0x00000040

#define MY_STRXFRM_DESC_LEVEL1      0x00000100

#define MY_STRXFRM_REVERSE_LEVEL1   0x00010000

#ifdef __i386__
    #define MB2(x)                (((x) >> 8) + (((x) & 0xFF) << 8))
#else
    #define MB2(x)                (x)
#endif

typedef unsigned long int my_wc_t;

typedef struct MY_COLLATION_HANDLER MY_COLLATION_HANDLER;
typedef struct CHARSET_INFO CHARSET_INFO;
typedef struct MY_CHARSET_HANDLER MY_CHARSET_HANDLER;
typedef int (*my_charset_conv_mb_wc)(const CHARSET_INFO *, my_wc_t *,
                                     const uchar *, const uchar *);
typedef int (*my_charset_conv_wc_mb)(const CHARSET_INFO *, my_wc_t, uchar *,
                                     uchar *);

typedef struct MY_UNICASE_CHARACTER
{
    uint32 toupper;
    uint32 tolower;
    uint32 sort;
} MY_UNICASE_CHARACTER;

typedef struct MY_UNICASE_INFO
{
    my_wc_t maxchar;
    const MY_UNICASE_CHARACTER **page;
} MY_UNICASE_INFO;

struct CHARSET_INFO
{
    uint32 number;
    uint32 state;
    const char *csname;
    const char *coll_name;
    int   sort_len;                 // Collect from 'SHOW COLLATION'
    const unsigned char *ctype;
    const unsigned char *to_lower;
    const unsigned char *to_upper;
    const unsigned char *sort_order;
    const MY_UNICASE_INFO *caseinfo;
    uint32 strxfrm_multiply;
    uint32 mbminlen;
    uint32 mbmaxlen;
    uint32 mbmaxlenlen;
    unsigned char pad_char;
    MY_CHARSET_HANDLER *cset;
    MY_COLLATION_HANDLER *coll;
};

struct MY_COLLATION_HANDLER
{
    bool (*init)();
    void (*uninit)();
    int (*strnncoll)(const CHARSET_INFO *, const unsigned char *, size_t, const unsigned char *, size_t, bool);
    int (*strnncollsp)(const CHARSET_INFO *, const unsigned char *, size_t, const unsigned char *, size_t);
    size_t (*strnxfrm)(const CHARSET_INFO *, unsigned char *dst, size_t dstlen,
                       uint32 num_codepoints, const unsigned char *src, size_t srclen,
                       uint32 flags);
    size_t (*strnxfrmlen)(const CHARSET_INFO *, size_t num_bytes);
    void (*hash_sort)(const CHARSET_INFO *cs, const unsigned char *key, size_t len,
                      uint64 *nr1, uint64 *nr2);
};

struct MY_CHARSET_HANDLER
{
    uint32 (*ismbchar)(const CHARSET_INFO *, const char *, const char *);
    my_charset_conv_mb_wc mb_wc;
    my_charset_conv_wc_mb wc_mb;
    void (*fill)(const CHARSET_INFO *, char *to, size_t len, int fill);
};

static inline uchar *
store16be(uchar *ptr, uint16 val)
{
    val = htons(val);
    memcpy(ptr, &val, sizeof(val));
    return ptr + sizeof(val);
}

static inline const uchar *
skip_trailing_space(const uchar *ptr, size_t len)
{
    const uchar *end = ptr + len;
    while (end - ptr >= 8)
    {
        uint64_t chunk;
        memcpy(&chunk, end - 8, sizeof(chunk));
        if (chunk != 0x2020202020202020ULL)
            break;
        end -= 8;
    }
    while (end > ptr && end[-1] == 0x20)
        end--;
    return (end);
}

static inline 
void MY_PUT_MB2(unsigned char *s, uint16 code)
{
    s[0] = code >> 8;
    s[1] = code & 0xFF;
}

extern MY_UNICASE_INFO my_unicase_default;

extern MY_COLLATION_HANDLER my_collation_mb_bin_handler;
extern MY_COLLATION_HANDLER my_collation_8bit_simple_ci_handler;
extern MY_COLLATION_HANDLER my_collation_8bit_bin_handler;

extern MY_CHARSET_HANDLER simple_charset_handler;
extern MY_CHARSET_HANDLER my_charset_utf8mb4_handler;

extern CHARSET_INFO my_charset_bin;
extern CHARSET_INFO my_charset_gbk_chinese_ci;
extern CHARSET_INFO my_charset_gbk_bin;
extern CHARSET_INFO my_charset_utf8mb3_general_ci;
extern CHARSET_INFO my_charset_utf8mb3_tolower_ci;
extern CHARSET_INFO my_charset_utf8mb3_general_mysql500_ci;
extern CHARSET_INFO my_charset_utf8mb3_bin;
extern CHARSET_INFO my_charset_utf8mb4_general_ci;
extern CHARSET_INFO my_charset_utf8mb4_bin;
extern CHARSET_INFO my_charset_utf8mb4_unicode_ci;
extern CHARSET_INFO my_charset_utf8_general_ci;
extern CHARSET_INFO my_charset_utf8_tolower_ci;
extern CHARSET_INFO my_charset_utf8_bin;
extern CHARSET_INFO my_charset_utf8_general_mysql500_ci;
extern CHARSET_INFO my_charset_utf8mb3_unicode_ci;
extern CHARSET_INFO my_charset_utf8_unicode_ci;
extern CHARSET_INFO my_charset_gb18030_chinese_ci;
extern CHARSET_INFO my_charset_gb18030_bin;
extern CHARSET_INFO my_charset_gb18030_unicode_520_ci;
extern CHARSET_INFO my_charset_gb2312_bin;
extern CHARSET_INFO my_charset_gb2312_chinese_ci;
extern CHARSET_INFO my_charset_big5_bin;
extern CHARSET_INFO my_charset_big5_chinese_ci;
extern CHARSET_INFO my_charset_eucjpms_bin;
extern CHARSET_INFO my_charset_eucjpms_japanese_ci;
extern CHARSET_INFO my_charset_euckr_bin;
extern CHARSET_INFO my_charset_euckr_korean_ci;
extern CHARSET_INFO my_charset_cp932_bin;
extern CHARSET_INFO my_charset_cp932_japanese_ci;
extern CHARSET_INFO my_charset_latin1;
extern CHARSET_INFO my_charset_latin1_german2_ci;
extern CHARSET_INFO my_charset_latin1_bin;
extern CHARSET_INFO my_charset_sjis_bin;
extern CHARSET_INFO my_charset_sjis_japanese_ci;
extern CHARSET_INFO my_charset_ujis_bin;
extern CHARSET_INFO my_charset_ujis_japanese_ci;
extern CHARSET_INFO my_charset_ascii_bin;
extern CHARSET_INFO my_charset_ascii_general_ci;
extern CHARSET_INFO my_charset_cp1250_bin;
extern CHARSET_INFO my_charset_cp1250_general_ci;
extern CHARSET_INFO my_charset_cp1251_bin;
extern CHARSET_INFO my_charset_cp1251_general_ci;
extern CHARSET_INFO my_charset_cp1256_bin;
extern CHARSET_INFO my_charset_cp1256_general_ci;
extern CHARSET_INFO my_charset_cp1257_bin;
extern CHARSET_INFO my_charset_cp1257_general_ci;
extern CHARSET_INFO my_charset_cp866_bin;
extern CHARSET_INFO my_charset_cp866_general_ci;
extern CHARSET_INFO my_charset_greek_bin;
extern CHARSET_INFO my_charset_greek_general_ci;
extern CHARSET_INFO my_charset_hebrew_bin;
extern CHARSET_INFO my_charset_hebrew_general_ci;
extern CHARSET_INFO my_charset_koi8r_bin;
extern CHARSET_INFO my_charset_koi8r_general_ci;
extern CHARSET_INFO my_charset_koi8u_bin;
extern CHARSET_INFO my_charset_koi8u_general_ci;
extern CHARSET_INFO my_charset_latin2_bin;
extern CHARSET_INFO my_charset_latin2_general_ci;
extern CHARSET_INFO my_charset_latin5_bin;
extern CHARSET_INFO my_charset_latin5_turkish_ci;
extern CHARSET_INFO my_charset_latin7_bin;
extern CHARSET_INFO my_charset_latin7_general_ci;
/* Begin - bug#8474 */
extern CHARSET_INFO my_charset_utf8_esperanto_uca_ci;
extern CHARSET_INFO my_charset_utf8mb4_0900_ai_ci;
/* End - bug#8474 */


int my_strnncoll_mb_bin(const CHARSET_INFO *cs, const uchar *s,
                        size_t slen, const uchar *t, size_t tlen,
                        bool t_is_prefix);
int my_strnncollsp_mb_bin(const CHARSET_INFO *cs,
                          const uchar *a, size_t a_length, const uchar *b,
                          size_t b_length);
void my_fill_8bit(const CHARSET_INFO *cs, char *s, size_t l, int fill);
void my_hash_sort_simple(const CHARSET_INFO *cs, const uchar *key, size_t len, uint64 *nr1, uint64 *nr2);
void my_hash_sort_mb_bin(const CHARSET_INFO *cs,
                         const uchar *key, size_t len, uint64 *nr1,
                         uint64 *nr2);
size_t my_strnxfrmlen_simple(const CHARSET_INFO *cs, size_t len);
Datum do_mysql_hash(const unsigned char *key, int32_t len, Oid coll);
void do_import_mysql_collation(void);
int do_mysql_strcoll(uchar *arg1, int len1, uchar *arg2, int len2, Oid collid, bool trim);
size_t do_mysql_strnxfrm(unsigned char *dst, size_t dstlen,
							 uint32 nweights, const unsigned char *src, size_t srclen,
							 uint32 flags, Oid coll);
size_t do_mysql_strnxfrmlen(size_t srclen, uint32 nweights, Oid coll);
int my_strnncoll_simple(const CHARSET_INFO *cs, const uchar *s, size_t slen, const uchar *t, size_t tlen, bool t_is_prefix);
int my_strnncollsp_simple(const CHARSET_INFO *cs, const uchar *a, size_t a_length, const uchar *b, size_t b_length);
size_t
my_strnxfrm_mb(const CHARSET_INFO *cs, uchar *dst, size_t dstlen,
               uint32 nweights, const uchar *src, size_t srclen, 
               uint32 flags);
size_t
my_strxfrm_pad_desc_and_reverse(const CHARSET_INFO *cs,
                                uchar *str, uchar *frmend, uchar *strend,
                                uint32 nweights, uint32 flags, uint32 level);
size_t
my_strnxfrm_simple(const CHARSET_INFO *cs,
                   uchar *dst, size_t dstlen, uint32 nweights,
                   const uchar *src, size_t srclen, uint32 flags);

void my_strxfrm_desc_and_reverse(uchar *str, uchar *strend,
                                 uint32 flags, uint32 level);

#endif