/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_formatter_internal.h
 *
 * Abstract:
 *   Internal definitions for TECC data formatter module.
 *   Defines internal constants, macros, and helper functions.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_formatter_internal.h
 */

#ifndef TECC_FORMATTER_INTERNAL_H
#define TECC_FORMATTER_INTERNAL_H

#include "tecc_formatter.h"
#include "postgres_fe.h"
#include "c.h"
/* Note: postgres.h is a backend header and should not be included in frontend code */
/* Using postgres_fe.h provides frontend-compatible definitions */

/* Internal constants - support both V1.0 (SM4) and V2.0 (SM4 + HE) cipherdata */
#define TECC_FORMATTER_VERSION CIPHERDATA_VERSION_2_0
#define TECC_FORMATTER_MIN_VERSION CIPHERDATA_VERSION_1_0
#define TECC_FORMATTER_MAX_VERSION CIPHERDATA_VERSION_2_0

/* Buffer size constants */
#define TECC_FORMATTER_DEFAULT_WORK_BUFFER_SIZE (64 * 1024)  /* 64KB default work buffer */
#define TECC_FORMATTER_MIN_WORK_BUFFER_SIZE (4 * 1024)     /* 4KB minimum work buffer */

/*
 * ReqID:TECC4.0 - OPE 归一化编码格式
 *
 * 浮点数/数值的 OPE 归一化编码格式:
 *   sign(1B) + weight(2B, 大端序) + reserved(3B) + digits(ndigits×2B, 大端序uint16)
 *   ndigits 不显式存储，由 (total_len - HEADER_SIZE) / sizeof(uint16_t) 推导。
 *
 * memcmp 比较顺序即数学大小顺序，规则:
 *   sign 决定大组顺序 → weight 决定组内大小 → digits 决定同weight内的顺序
 *
 * Sign 编码（按 memcmp 递增顺序）:
 *   NINF=0x00 < NEG=0x01 < ZERO=0x02 < POS=0x03 < INF=0x04 < NAN=0x05
 *
 * Weight 编码:
 *   所有类型: raw = weight + WEIGHT_BIAS(16384)，保证负weight在高位排序正确
 *   负数(NEG): encoded = UINT16_MAX - raw （取反)，使得更负的数 → 更小的weight编码
 *   其他: encoded = raw
 *
 * Digits 编码:
 *   正数: digits 原文写入（大端序uint16）
 *   负数: digits = 9 - digit（9's complement），使得更负的数 → 更小的digit编码
 *   OPE 对齐补齐的 0 在负数的 9's complement 下变成 9，不影响顺序
 *
 * 特殊值:
 *   NINF: sign=0x00, weight=0, ndigits=0（最小的编码）
 *   ZERO: sign=0x02, weight=0, ndigits=0
 *   INF:  sign=0x04, weight=0, ndigits=0
 *   NAN:  sign=0x05, weight=0, ndigits=0（最大的编码）
 *
 * 示例排序（asc）:
 *   -Inf < -80000 < -8 < -1.5 < -1 < -1e-10 < 0 < 1e-10 < 1 < 1.5 < 8 < 80000 < +Inf < NaN
 */
#define TECC_NUMERIC_SIGN_NINF       0x00
#define TECC_NUMERIC_SIGN_NEGATIVE   0x01
#define TECC_NUMERIC_SIGN_ZERO       0x02
#define TECC_NUMERIC_SIGN_POSITIVE   0x03
#define TECC_NUMERIC_SIGN_INF        0x04
#define TECC_NUMERIC_SIGN_NAN        0x05

#define TECC_NUMERIC_HEADER_SIZE      6      /* Sign(1) + Weight(2) + Reserved(3). NDigits derived from (len-6)/2 */
#define TECC_NUMERIC_MAX_DIGITS       1000   /* Maximum decimal digits (PG NUMERIC limit) */
#define TECC_NUMERIC_PAD_DIGITS       1010   /* Max digits with OPE 12-byte alignment padding (+10 safe) */
#define TECC_NUMERIC_OPE_CHUNK_SIZE   12     /* OPE plaintext chunk size (must match SM4_OPE_CHUNK_SIZE) */
#define TECC_NUMERIC_MAX_WEIGHT       16383  /* Maximum weight value */
#define TECC_NUMERIC_MIN_WEIGHT       -16383 /* Minimum weight value */
#define TECC_NUMERIC_WEIGHT_BIAS      16384  /* Weight bias for OPE memcmp ordering */

/* Text normalization constants */
#define TECC_TEXT_LENGTH_PREFIX_SIZE  4      /* 4-byte network byte order prefix */
#define TECC_TEXT_MAX_ICU_SORTKEY_MULTIPLIER 3 /* ICU sort key max expansion factor */
#define TECC_TEXT_MIN_ICU_SORTKEY_SIZE 8     /* Minimum ICU sort key size */
#define TECC_TEXT_OPE_CHUNK_SIZE       TECC_NUMERIC_OPE_CHUNK_SIZE /* OPE chunk size (12 bytes) */

/* Hex encoding constants */
#define TECC_HEX_PREFIX_LENGTH        2      /* "\\x" prefix length */
#define TECC_HEX_MIN_STRING_LENGTH    2      /* Minimum hex string length (1 byte) */

/* Algorithm-specific constants */
#define TECC_ALGO_RND_HEADER_SIZE     0      /* RND: no header */
#define TECC_ALGO_DET_HEADER_SIZE     0      /* DET: no header */
#define TECC_ALGO_OPE_HEADER_SIZE     TECC_NUMERIC_HEADER_SIZE /* OPE: numeric header */
#define TECC_ALGO_SEARCH_HEADER_SIZE  0      /* SEARCH: no header */

/* Internal macros */

/* Check if buffer is valid */
#define TECC_BUFFER_VALID(buf, size) ((buf) != NULL && (size) > 0)

/* Check if buffer is within size limits */
#define TECC_BUFFER_SIZE_VALID(size) ((size) > 0)

/* Align size to boundary */
#define TECC_ALIGN_SIZE(size, boundary) (((size) + (boundary) - 1) & ~((boundary) - 1))

/* Round up to power of 2 */
#define TECC_ROUND_UP_POW2(size) \
    do { \
        size_t _s = (size); \
        size_t _r = 1; \
        while (_r < _s) _r <<= 1; \
        (size) = _r; \
    } while (0)

/* Check if Oid is a numeric type */
#define TECC_IS_NUMERIC_TYPE(oid) \
    ((oid) == INT2OID || (oid) == INT4OID || (oid) == INT8OID || \
     (oid) == FLOAT4OID || (oid) == FLOAT8OID || (oid) == NUMERICOID)

/* Check if Oid is a text type */
#define TECC_IS_TEXT_TYPE(oid) \
    ((oid) == VARCHAROID || (oid) == BPCHAROID || (oid) == TEXTOID)

/* Check if algorithm supports in-place operation */
#define TECC_ALGO_SUPPORTS_INPLACE(algo) ((algo) == TECC_ALGO_RND)

/* Internal data structures */

/* Numeric normalized format for OPE algorithm */
typedef struct TeccNumericNormalized
{
    uint8_t  sign;                      /* Sign: POSITIVE, NEGATIVE, ZERO, etc. */
    int16_t  weight;                    /* Weight: position of decimal point */
    uint16_t ndigits;                   /* Number of decimal digits (strip后的有效digits) */
    uint16_t dscale;                    /* Display scale: 小数位数, NUMERIC类型用, int/float为0 */
    uint16_t digits[TECC_NUMERIC_PAD_DIGITS]; /* Decimal digits (up to 1000 + OPE padding) */
} TeccNumericNormalized;

/* Text normalization options */
typedef struct TeccTextNormalizeOptions
{
    bool     add_length_prefix;         /* Add 4-byte length prefix */
    bool     generate_icu_sortkey;      /* Generate ICU sort key */
    bool     preserve_original;         /* Keep original format */
    bool     validate_encoding;         /* Validate text encoding */
    Oid      collation_oid;             /* Collation OID for sort key */
    char     collation_locale[NAMEDATALEN]; /* Collation locale string for ICU */
} TeccTextNormalizeOptions;

/* Buffer pool for efficient memory management (frontend version) */
typedef struct TeccFormatterBufferPool
{
    void        **buffers;              /* Buffer array */
    size_t      *sizes;                 /* Buffer sizes */
    bool        *in_use;                /* Usage flags */
    int          count;                 /* Number of buffers */
    int          capacity;              /* Maximum capacity */
    size_t       total_allocated;       /* Total allocated memory */
    size_t       total_used;            /* Total used memory */
} TeccFormatterBufferPool;

/* Internal function prototypes */

/* Memory management functions */
extern TeccFormatterBufferPool *tecc_formatter_buffer_pool_create(int capacity);
extern void *tecc_formatter_buffer_pool_alloc(TeccFormatterBufferPool *pool, size_t size);
extern bool tecc_formatter_buffer_pool_free(TeccFormatterBufferPool *pool, void *buffer);
extern void tecc_formatter_buffer_pool_destroy(TeccFormatterBufferPool *pool);
extern size_t tecc_formatter_buffer_pool_get_stats(TeccFormatterBufferPool *pool);

/* Numeric normalization helper functions */
extern TeccFormatterErrorCode tecc_formatter_parse_numeric_text(const char *numeric_text,
                                                                size_t text_len,
                                                                uint8_t *sign,
                                                                int16_t *weight,
                                                                uint16_t *digits,
                                                                int *ndigits);
extern TeccFormatterErrorCode tecc_formatter_encode_numeric_normalized(TeccNumericNormalized *norm_num,
                                                                       unsigned char **output_data,
                                                                       size_t *output_len);
extern TeccFormatterErrorCode tecc_formatter_encode_numeric_normalized_to_buffer(TeccNumericNormalized *norm_num,
                                                                                 unsigned char *buffer,
                                                                                 size_t buffer_size,
                                                                                 size_t *output_len);
extern TeccFormatterErrorCode tecc_formatter_validate_numeric_range(int16_t weight, int ndigits);

/* Text normalization helper functions */
extern TeccFormatterErrorCode tecc_formatter_get_text_normalize_options(TeccCryptoContextCore *context,
                                                                        TeccTextNormalizeOptions *options);
extern TeccFormatterErrorCode tecc_formatter_validate_text_encoding(const unsigned char *text, size_t len);
extern size_t tecc_formatter_estimate_normalized_text_size(TeccCryptoContextCore *context, size_t input_len);

/* ICU helper functions - use tecc_icu_normalizer_* functions instead */
/* Note: tecc_formatter_icu_* functions have been removed, use tecc_icu_normalizer_* functions directly */

/* Hex encoding helper functions */
extern TeccFormatterErrorCode tecc_formatter_validate_hex_string(const char *hex_string, size_t *binary_len);
extern bool tecc_formatter_has_hex_prefix(const char *hex_string);
extern const char *tecc_formatter_skip_hex_prefix(const char *hex_string);

/* Algorithm-specific helper functions */
extern TeccFormatterErrorCode tecc_formatter_get_algo_header_size(TeccAlgorithmType algo_type, size_t *header_size);
extern TeccFormatterErrorCode tecc_formatter_validate_algo_data_size(TeccAlgorithmType algo_type, size_t data_size);

/* Debug and logging functions */
#ifdef TECC_FORMATTER_DEBUG
extern void tecc_formatter_log_debug(const char *func_name, const char *format, ...) pg_attribute_printf(2, 3);
extern void tecc_formatter_log_info(const char *func_name, const char *format, ...) pg_attribute_printf(2, 3);
extern void tecc_formatter_log_warning(const char *func_name, const char *format, ...) pg_attribute_printf(2, 3);
extern void tecc_formatter_log_error(const char *func_name, const char *format, ...) pg_attribute_printf(2, 3);

#define TECC_FORMATTER_DEBUG_LOG(fmt, ...) tecc_formatter_log_debug(__func__, fmt, ##__VA_ARGS__)
#define TECC_FORMATTER_INFO_LOG(fmt, ...) tecc_formatter_log_info(__func__, fmt, ##__VA_ARGS__)
#define TECC_FORMATTER_WARNING_LOG(fmt, ...) tecc_formatter_log_warning(__func__, fmt, ##__VA_ARGS__)
#define TECC_FORMATTER_ERROR_LOG(fmt, ...) tecc_formatter_log_error(__func__, fmt, ##__VA_ARGS__)
#else
#define TECC_FORMATTER_DEBUG_LOG(fmt, ...) ((void)0)
#define TECC_FORMATTER_INFO_LOG(fmt, ...) ((void)0)
#define TECC_FORMATTER_WARNING_LOG(fmt, ...) ((void)0)
#define TECC_FORMATTER_ERROR_LOG(fmt, ...) ((void)0)
#endif

/* Validation macros */
#define TECC_RETURN_IF_NULL(ptr, error_code) \
    do { \
        if ((ptr) == NULL) \
            return (error_code); \
    } while (0)

#define TECC_RETURN_IF_INVALID_PARAM(condition, error_code) \
    do { \
        if (!(condition)) \
            return (error_code); \
    } while (0)

#define TECC_RETURN_IF_MEMORY_ERROR(ptr, error_code) \
    do { \
        if ((ptr) == NULL) \
            return (error_code); \
    } while (0)

/* Utility macros */
#define TECC_MIN(a, b) ((a) < (b) ? (a) : (b))
#define TECC_MAX(a, b) ((a) > (b) ? (a) : (b))
#define TECC_CLAMP(x, min, max) ((x) < (min) ? (min) : (x) > (max) ? (max) : (x))

/* Varlena header macros for frontend compatibility */
#ifndef VARSIZE
#define VARSIZE(PTR)            (((int32_t *)(PTR))[0] & 0x3FFFFFFF)  /* Clear high bits */
#endif

#ifndef SET_VARSIZE
#define SET_VARSIZE(PTR, len)   (((int32_t *)(PTR))[0] = (int32_t)(len))
#endif

/* Check if collation locale uses raw byte comparison (C/POSIX).
 * These libc collations compare bytes directly and must NOT use ICU sort keys
 * when generating OPE normalized text, otherwise ORDER BY ordering won't match
 * PostgreSQL's strcmp() semantics for "C" collation.
 *
 * Returns true if locale is NULL, empty, "C", or "POSIX" (case-insensitive). */
static inline bool
tecc_is_raw_byte_collation(const char *locale)
{
    if (!locale || locale[0] == '\0')
        return true;  /* No locale = raw byte comparison */
    if (pg_strcasecmp(locale, "C") == 0 || pg_strcasecmp(locale, "POSIX") == 0)
        return true;  /* C/POSIX = byte-level collation */
    return false;
}

#endif /* TECC_FORMATTER_INTERNAL_H */