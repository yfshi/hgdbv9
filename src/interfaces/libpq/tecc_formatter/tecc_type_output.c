/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_type_output.c
 *
 * Abstract:
 *   Binary-to-text output conversion for TECC decrypted data.
 *   Implements output functions for all encrypted types, referencing
 *   backend *_out function logic (int2out, int4out, int8out,
 *   float4out, float8out, numeric_out, textout).
 *
 * Authored by <wangxp@highgo.com>, 20260424.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_type_output.c
 */

#include "tecc_type_output.h"
#include "tecc_formatter_internal.h"
#include "tecc_data_validator.h"
#include "common/shortest_dec.h"
#include "mb/pg_wchar.h"   /* ReqID:TECC4.0 - pg_encoding_mblen（字符长度/typmod 判定） */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <ctype.h>

/* ReqID:TECC4.0 - Normalized format constants.
 * The TECC normalized binary format is used by all numeric encrypted types
 * for RND/DET/OPE algorithm plaintext. Format:
 *   sign(1B) + weight(2B, network byte order, +16384 bias) + reserved(3B) + digits(variable).
 * Negative weight: stored as UINT16_MAX - (weight + 16384) (取反).
 * Negative digits: stored as 9's complement for OPE memcmp ordering.
 * NDigits = (total_len - TECC_NORM_HEADER_SIZE) / sizeof(uint16_t).
 * Sign order (memcmp asc): NINF < NEG < ZERO < POS < INF < NAN */

/* Normalized format sign constants (match tecc_formatter_internal.h) */
#define TECC_NORM_SIGN_NINF       0x00
#define TECC_NORM_SIGN_NEGATIVE   0x01
#define TECC_NORM_SIGN_ZERO       0x02
#define TECC_NORM_SIGN_POSITIVE   0x03
#define TECC_NORM_SIGN_INF        0x04
#define TECC_NORM_SIGN_NAN        0x05

/* Header sizes */
#define TECC_NORM_HEADER_SIZE     6       /* sign(1) + weight(2) + reserved(3), 不含dscale */
#define TECC_NORM_DSCALE_SIZE     2       /* dscale占2字节，放在digits末尾 */
/* 解码时ndigits推导: (len - TECC_NORM_HEADER_SIZE - TECC_NORM_DSCALE_SIZE) / 2 */
#define TECC_NORM_WEIGHT_BIAS     16384

/* ==================== NumericData structures (frontend version) ==================== */

/*
 * Frontend-compatible definitions for NumericData parsing.
 * Reference: src/backend/utils/adt/numeric.c
 *
 * PG14 uses NBASE=10000, DEC_DIGITS=4, NumericDigit=int16.
 */
#define TECC_NBASE              10000
#define TECC_DEC_DIGITS         4
#define TECC_HALF_NBASE         5000
#define TECC_NUMERIC_DIGIT_MAX  9999

/* NumericDigit type for frontend */
typedef int16 TeccNumericDigit;

/* NumericLong format */
typedef struct TeccNumericLong
{
    uint16  n_sign_dscale;
    int16   n_weight;
    TeccNumericDigit n_data[FLEXIBLE_ARRAY_MEMBER];
} TeccNumericLong;

/* NumericShort format */
typedef struct TeccNumericShort
{
    uint16   n_header;
    TeccNumericDigit n_data[FLEXIBLE_ARRAY_MEMBER];
} TeccNumericShort;

union TeccNumericChoice
{
    uint16  n_header;
    struct TeccNumericLong n_long;
    struct TeccNumericShort n_short;
};

/* NumericData structure (matches backend) */
typedef struct TeccNumericData
{
    int32  vl_len_;
    union TeccNumericChoice choice;
} TeccNumericData;

/* Numeric sign constants */
#define TECC_NUMERIC_SIGN_MASK  0xC000
#define TECC_NUMERIC_POS        0x0000
#define TECC_NUMERIC_NEG        0x4000
#define TECC_NUMERIC_SHORT      0x8000
#define TECC_NUMERIC_SPECIAL    0xC000

#define TECC_NUMERIC_NAN        0xC000
#define TECC_NUMERIC_PINF       0xD000
#define TECC_NUMERIC_NINF       0xF000

#define TECC_NUMERIC_SHORT_SIGN_MASK       0x2000
#define TECC_NUMERIC_SHORT_DSCALE_MASK     0x1F80
#define TECC_NUMERIC_SHORT_DSCALE_SHIFT    7
#define TECC_NUMERIC_SHORT_WEIGHT_SIGN_MASK 0x0040
#define TECC_NUMERIC_SHORT_WEIGHT_MASK     0x003F

#define TECC_NUMERIC_DSCALE_MASK   0x3FFF

/* Varlena size macro for frontend */
#define TECC_VARSIZE(ptr)   (((int32 *)(ptr))[0] & 0x3FFFFFFF)
#define TECC_VARHDRSZ       ((int32) sizeof(int32))

/* Numeric header size macros */
#define TECC_NUMERIC_HDRSZ_LONG  (TECC_VARHDRSZ + sizeof(uint16) + sizeof(int16))
#define TECC_NUMERIC_HDRSZ_SHORT (TECC_VARHDRSZ + sizeof(uint16))

/* Numeric header size for PostgreSQL NumericData varlena.
 * Note: different from TECC_NUMERIC_HEADER_SIZE (6) which is the
 * normalized binary format header. */
#define TECC_NUMERIC_HEADER_IS_SHORT(n) \
    (((n)->choice.n_header & 0x8000) != 0)

#define TECC_NUMERIC_VARLENA_HDRSZ(n) \
    (TECC_VARHDRSZ + sizeof(uint16) + \
     (TECC_NUMERIC_HEADER_IS_SHORT(n) ? 0 : sizeof(int16)))

/* Numeric accessor macros */
#define TECC_NUMERIC_FLAGBITS(n)  ((n)->choice.n_header & TECC_NUMERIC_SIGN_MASK)

#define TECC_NUMERIC_IS_SHORT(n)  (TECC_NUMERIC_FLAGBITS(n) == TECC_NUMERIC_SHORT)
#define TECC_NUMERIC_IS_SPECIAL(n) (TECC_NUMERIC_FLAGBITS(n) == TECC_NUMERIC_SPECIAL)
#define TECC_NUMERIC_IS_NAN(n)    ((n)->choice.n_header == TECC_NUMERIC_NAN)
#define TECC_NUMERIC_IS_PINF(n)   ((n)->choice.n_header == TECC_NUMERIC_PINF)
#define TECC_NUMERIC_IS_NINF(n)   ((n)->choice.n_header == TECC_NUMERIC_NINF)

#define TECC_NUMERIC_SIGN(n) \
    (TECC_NUMERIC_IS_SHORT(n) ? \
        (((n)->choice.n_short.n_header & TECC_NUMERIC_SHORT_SIGN_MASK) ? \
         TECC_NUMERIC_NEG : TECC_NUMERIC_POS) : \
        (TECC_NUMERIC_IS_SPECIAL(n) ? \
         TECC_NUMERIC_FLAGBITS(n) : TECC_NUMERIC_FLAGBITS(n)))

#define TECC_NUMERIC_DSCALE(n) \
    (TECC_NUMERIC_HEADER_IS_SHORT((n)) ? \
        ((n)->choice.n_short.n_header & TECC_NUMERIC_SHORT_DSCALE_MASK) \
            >> TECC_NUMERIC_SHORT_DSCALE_SHIFT \
        : ((n)->choice.n_long.n_sign_dscale & TECC_NUMERIC_DSCALE_MASK))

#define TECC_NUMERIC_WEIGHT(n) \
    (TECC_NUMERIC_HEADER_IS_SHORT((n)) ? \
        (((n)->choice.n_short.n_header & TECC_NUMERIC_SHORT_WEIGHT_SIGN_MASK ? \
            ~TECC_NUMERIC_SHORT_WEIGHT_MASK : 0) \
         | ((n)->choice.n_short.n_header & TECC_NUMERIC_SHORT_WEIGHT_MASK)) \
        : ((n)->choice.n_long.n_weight))

#define TECC_NUMERIC_DIGITS(num) \
    (TECC_NUMERIC_HEADER_IS_SHORT(num) ? \
        (num)->choice.n_short.n_data : (num)->choice.n_long.n_data)

#define TECC_NUMERIC_NDIGITS(num) \
    ((TECC_VARSIZE(num) - TECC_NUMERIC_VARLENA_HDRSZ(num)) / sizeof(TeccNumericDigit))

/* Varlena data accessor for text types */
#define TECC_VARDATA(ptr)    ((char *)(ptr) + TECC_VARHDRSZ)
#define TECC_VARSIZE_EXHDR(ptr) (TECC_VARSIZE(ptr) - TECC_VARHDRSZ)

/*
 * tecc_decode_normalized_to_text - Decode TECC normalized binary to PG numeric text.
 *
 * Decodes the shared normalized binary format (used by RND/DET/OPE) to a
 * human-readable numeric string. The format is:
 *   sign(1) + weight(2, network byte order, +TECC_NORM_WEIGHT_BIAS bias) +
 *   reserved(3) + digits(ndigits * 2, network byte order uint16).
 * Negative digits are stored as 9's complement for OPE ordering.
 * NDigits is derived from (data_len - TECC_NORM_HEADER_SIZE) / sizeof(uint16_t).
 *
 * Output: pg_malloc'd null-terminated string (e.g., "42", "-3.14", "NaN").
 * Returns NULL on invalid input.
 */
static char *
tecc_decode_normalized_to_text(const unsigned char *data, size_t len)
{
    int sign;
    int weight;
    int ndigits;
    uint16_t dscale;
    const uint16_t *digits_ptr;
    int i;
    char *result;
    int result_len;
    int int_digits;
    char *p;
    int last_sig_digit;  /* 最后一个有效digit的索引（排除尾随零） */

    if (data == NULL || len < (size_t)TECC_NORM_HEADER_SIZE)
        return NULL;

    sign = (int)data[0];
    /* ReqID:TECC4.0 - Weight is network byte order uint16 with +16384 bias.
     * For NEGATIVE: stored as UINT16_MAX - (weight + 16384), so decode reverses.
     * Digits are native byte order uint16 (matching encode which writes via
     * uint16_t pointer assignment). Only weight needs byte-swap on little-endian. */
    {
        unsigned int raw_weight = ((unsigned int)(unsigned char)data[1] << 8) | (unsigned int)(unsigned char)data[2];
        if (sign == TECC_NORM_SIGN_NEGATIVE)
            weight = (int)(65535 - raw_weight) - TECC_NORM_WEIGHT_BIAS;
        else
            weight = (int)raw_weight - TECC_NORM_WEIGHT_BIAS;
    }
    ndigits = (int)((len - TECC_NORM_HEADER_SIZE - TECC_NORM_DSCALE_SIZE) / sizeof(uint16_t));

    /* ReqID:TECC4.0 - 从末尾2字节读dscale */
    if (len >= TECC_NORM_DSCALE_SIZE)
    {
        dscale = (uint16_t)((unsigned int)(unsigned char)data[len - 2] << 8) |
                 (uint16_t)(unsigned char)data[len - 1];
    }
    else
    {
        dscale = 0;
    }

    /* Handle special values */
    switch (sign)
    {
        case TECC_NORM_SIGN_NAN:
            return pg_strdup("NaN");
        case TECC_NORM_SIGN_INF:
            return pg_strdup("Infinity");
        case TECC_NORM_SIGN_NINF:
            return pg_strdup("-Infinity");
        case TECC_NORM_SIGN_ZERO:
            /* ReqID:TECC4.0: 用dscale控制零值输出精度，
             * 不再依赖ndigits中的尾随零。 */
            if (dscale > 0)
            {
                char *zresult = (char *)pg_malloc(dscale + 3);
                if (zresult == NULL)
                    return NULL;
                zresult[0] = '0';
                zresult[1] = '.';
                memset(zresult + 2, '0', dscale);
                zresult[2 + dscale] = '\0';
                return zresult;
            }
            return pg_strdup("0");
        default:
            break;
    }

    /* Validate sign */
    if (sign != TECC_NORM_SIGN_POSITIVE && sign != TECC_NORM_SIGN_NEGATIVE)
        return NULL;

    if (ndigits < 0 || ndigits > 2000)
        return NULL;

    digits_ptr = (const uint16_t *)(data + TECC_NORM_HEADER_SIZE);

    /* ReqID:TECC4.0 - 确定最后一个有效digit的位置（排除尾随零） */
    last_sig_digit = ndigits - 1;
    while (last_sig_digit >= 0 && last_sig_digit < ndigits)
    {
        unsigned int raw = (unsigned int)digits_ptr[last_sig_digit];
        int d = (sign == TECC_NORM_SIGN_NEGATIVE) ? (9 - (int)raw) : (int)raw;
        if (d != 0)
            break;
        last_sig_digit--;
    }
    if (last_sig_digit < 0)
    {
        /* All digits zero — should have been caught by ZERO case above */
        return pg_strdup("0");
    }

    /* Calculate number of integer digits (before decimal point) */
    int_digits = weight + 1;

    /* 计算dscale决定的小数位数：
     * - 如果dscale=0，只输出整数部分（无小数点）
     * - 如果dscale>0，输出小数部分，不足的补零 */
    {
        int needed_frac_digits = (dscale > 0) ? (int)dscale : 0;
        int leading_zero_count = (weight < -1) ? (-weight - 1) : 0;

        /* 小数部分所需的digit来自digits（从第int_digits个开始），
         * 如果digits不够则补零 */
        int frac_from_digits = (int_digits < ndigits) ? (ndigits - int_digits) : 0;
        int total_frac = (needed_frac_digits > frac_from_digits) ?
                          needed_frac_digits : frac_from_digits;

        result_len = 1 + (int_digits > 0 ? int_digits : 1) +
                     (total_frac > 0 ? 1 : 0) +  /* '.' */
                     leading_zero_count +
                     ((int_digits > 0 && ndigits > int_digits) ?
                      (ndigits - int_digits) : ndigits) +
                     /* dscale补零 */
                     ((needed_frac_digits > frac_from_digits) ?
                      (needed_frac_digits - frac_from_digits) : 0) +
                     1;  /* NUL */
    }
    result = (char *)pg_malloc0(result_len);
    if (result == NULL)
        return NULL;
    p = result;

    /* Sign */
    if (sign == TECC_NORM_SIGN_NEGATIVE)
        *p++ = '-';

    /* Integer part (before decimal point) */
    if (int_digits <= 0)
    {
        *p++ = '0';
    }
    else
    {
        int wrote_first = 0;
        for (i = 0; i < ndigits && i < int_digits; i++)
        {
            int d;
            unsigned int raw = (unsigned int)digits_ptr[i];
            if (sign == TECC_NORM_SIGN_NEGATIVE)
                d = 9 - (int)raw;
            else
                d = (int)raw;
            if (d < 0 || d > 9)
                continue;
            if (d != 0 || wrote_first || i == int_digits - 1)
            {
                *p++ = (char)(d + '0');
                wrote_first = 1;
            }
        }
        /* Pad with zeros if ndigits < int_digits */
        for (i = ndigits; i < int_digits; i++)
            *p++ = '0';
    }

    /* Decimal part: use dscale to determine how many fractional digits to output.
     * ReqID:TECC4.0 - 用dscale控制小数位数。当dscale=0且weight<-1时（tiny值），
     * 回退到输出全部available小数digit（旧行为）。 */
    {
        int frac_digits_available;
        int needed_frac_digits;
        int frac_idx;
        int leading_zero_count;
        int total_frac;

        /* dscale=0时，用所有可用的尾数digit */
        if (dscale > 0)
            needed_frac_digits = (int)dscale;
        else if (weight < -1)
        {
            /* 极小值（如0.0000001），无dscale时输出全部小数digit */
            needed_frac_digits = (int_digits < ndigits) ?
                                 (ndigits - int_digits) : 0;
            if (weight < -1)
                needed_frac_digits += (-weight - 1);
        }
        else
            needed_frac_digits = 0;

        if (int_digits <= 0)
        {
            frac_digits_available = (last_sig_digit >= 0) ? last_sig_digit + 1 : 0;
            frac_idx = 0;
        }
        else if (int_digits <= last_sig_digit)
        {
            frac_digits_available = last_sig_digit - int_digits + 1;
            frac_idx = int_digits;
        }
        else
        {
            frac_digits_available = 0;
            frac_idx = ndigits;
        }

        leading_zero_count = (weight < -1) ? (-weight - 1) : 0;
        total_frac = (needed_frac_digits > frac_digits_available) ?
                      needed_frac_digits : frac_digits_available;

        if (total_frac > 0)
        {
            int digits_written = 0;

            *p++ = '.';

            /* 前导零（0.00xxx） */
            if (weight < -1)
            {
                int z;
                for (z = 0; z < leading_zero_count && digits_written < needed_frac_digits; z++)
                {
                    *p++ = '0';
                    digits_written++;
                }
            }

            /* 输出有效小数digit */
            {
                int j;
                for (j = 0; j < frac_digits_available && digits_written < needed_frac_digits; j++)
                {
                    int d;
                    unsigned int raw;
                    int idx = frac_idx + j;
                    if (idx < 0 || idx >= ndigits)
                        break;
                    raw = (unsigned int)digits_ptr[idx];
                    if (sign == TECC_NORM_SIGN_NEGATIVE)
                        d = 9 - (int)raw;
                    else
                        d = (int)raw;
                    if (d < 0 || d > 9)
                        d = 0;
                    *p++ = (char)(d + '0');
                    digits_written++;
                }
            }

            /* dscale要求的小数位数不足时补零 */
            if (dscale > 0)
            {
                while (digits_written < (int)dscale)
                {
                    *p++ = '0';
                    digits_written++;
                }
            }
        }
    }

    *p = '\0';
    return result;
}

/* ==================== int2 output ==================== */

/*
 * tecc_type_output_int2 - Decode normalized binary to int2 text.
 *
 * Directly extracts sign/weight/digits from the normalized binary and
 * computes the integer value, skipping decode_to_text → strtol detour.
 */
char *
tecc_type_output_int2(const char *data, size_t len)
{
    int sign;
    int weight;
    int ndigits;
    const uint16_t *digits_ptr;
    unsigned int raw_weight;
    uint64_t abs_val;
    int32_t result;
    char *out;
    int i;

    if (data == NULL || len < (size_t)TECC_NORM_HEADER_SIZE)
        return NULL;

    sign = (int)data[0];
    if (sign == TECC_NORM_SIGN_NAN || sign == TECC_NORM_SIGN_INF ||
        sign == TECC_NORM_SIGN_NINF)
        return NULL;

    raw_weight = ((unsigned int)(unsigned char)data[1] << 8) | (unsigned int)(unsigned char)data[2];
    if (sign == TECC_NORM_SIGN_NEGATIVE)
        weight = (int)(65535 - raw_weight) - TECC_NORM_WEIGHT_BIAS;
    else
        weight = (int)raw_weight - TECC_NORM_WEIGHT_BIAS;

    ndigits = (int)((len - TECC_NORM_HEADER_SIZE - TECC_NORM_DSCALE_SIZE) / sizeof(uint16_t));
    digits_ptr = (const uint16_t *)(data + TECC_NORM_HEADER_SIZE);

    if (weight < 0) return NULL;
    for (i = weight + 1; i < ndigits; i++)
    {
        unsigned int fd = (unsigned int)digits_ptr[i];
        if (sign == TECC_NORM_SIGN_NEGATIVE) fd = 9 - fd;
        if (fd != 0) return NULL;
    }

    abs_val = 0;
    {
        int int_limit = (weight + 1 < ndigits) ? weight + 1 : ndigits;
        for (i = 0; i < int_limit; i++)
        {
            unsigned int d = (unsigned int)digits_ptr[i];
            if (sign == TECC_NORM_SIGN_NEGATIVE) d = 9 - d;
            if (abs_val > (uint64_t)INT64_MAX / 10) return NULL;
            abs_val = abs_val * 10 + d;
        }
    }
    for (i = ndigits; i <= weight; i++)
    {
        if (abs_val > (uint64_t)INT64_MAX / 10) return NULL;
        abs_val *= 10;
    }

    if (sign == TECC_NORM_SIGN_NEGATIVE)
    {
        if (abs_val > (uint64_t)INT16_MAX + 1) return NULL;
        if (abs_val == (uint64_t)INT16_MAX + 1)
            result = INT16_MIN;
        else
            result = -(int32_t)abs_val;
    }
    else
    {
        if (abs_val > (uint64_t)INT16_MAX) return NULL;
        result = (int32_t)abs_val;
    }

    out = (char *)pg_malloc(8);
    snprintf(out, 8, "%d", result);
    return out;
}

/* ==================== int4 output ==================== */

/*
 * tecc_type_output_int4 - Decode normalized binary to int4 text.
 *
 * Directly extracts sign/weight/digits, skips decode_to_text → strtol detour.
 */
char *
tecc_type_output_int4(const char *data, size_t len)
{
    int sign;
    int weight;
    int ndigits;
    const uint16_t *digits_ptr;
    unsigned int raw_weight;
    uint64_t abs_val;
    int32_t result;
    char *out;
    int i;

    if (data == NULL || len < (size_t)TECC_NORM_HEADER_SIZE)
        return NULL;

    sign = (int)data[0];
    if (sign == TECC_NORM_SIGN_NAN || sign == TECC_NORM_SIGN_INF ||
        sign == TECC_NORM_SIGN_NINF)
        return NULL;

    raw_weight = ((unsigned int)(unsigned char)data[1] << 8) | (unsigned int)(unsigned char)data[2];
    if (sign == TECC_NORM_SIGN_NEGATIVE)
        weight = (int)(65535 - raw_weight) - TECC_NORM_WEIGHT_BIAS;
    else
        weight = (int)raw_weight - TECC_NORM_WEIGHT_BIAS;

    ndigits = (int)((len - TECC_NORM_HEADER_SIZE - TECC_NORM_DSCALE_SIZE) / sizeof(uint16_t));
    digits_ptr = (const uint16_t *)(data + TECC_NORM_HEADER_SIZE);

    if (weight < 0) return NULL;
    for (i = weight + 1; i < ndigits; i++)
    {
        unsigned int fd = (unsigned int)digits_ptr[i];
        if (sign == TECC_NORM_SIGN_NEGATIVE) fd = 9 - fd;
        if (fd != 0) return NULL;
    }

    abs_val = 0;
    {
        int int_limit = (weight + 1 < ndigits) ? weight + 1 : ndigits;
        for (i = 0; i < int_limit; i++)
        {
            unsigned int d = (unsigned int)digits_ptr[i];
            if (sign == TECC_NORM_SIGN_NEGATIVE) d = 9 - d;
            if (abs_val > (uint64_t)INT64_MAX / 10) return NULL;
            abs_val = abs_val * 10 + d;
        }
    }
    for (i = ndigits; i <= weight; i++)
    {
        if (abs_val > (uint64_t)INT64_MAX / 10) return NULL;
        abs_val *= 10;
    }

    if (sign == TECC_NORM_SIGN_NEGATIVE)
    {
        if (abs_val > (uint64_t)INT32_MAX + 1) return NULL;
        if (abs_val == (uint64_t)INT32_MAX + 1)
            result = INT32_MIN;
        else
            result = -(int32_t)abs_val;
    }
    else
    {
        if (abs_val > (uint64_t)INT32_MAX) return NULL;
        result = (int32_t)abs_val;
    }

    out = (char *)pg_malloc(13);
    snprintf(out, 13, "%d", result);
    return out;
}

/* ==================== int8 output ==================== */

/*
 * tecc_type_output_int8 - Decode normalized binary to int8 text.
 *
 * Directly extracts sign/weight/digits, skips decode_to_text → strtoll detour.
 */
char *
tecc_type_output_int8(const char *data, size_t len)
{
    int sign;
    int weight;
    int ndigits;
    const uint16_t *digits_ptr;
    unsigned int raw_weight;
    uint64_t abs_val;
    int64_t result;
    char *out;
    int i;

    if (data == NULL || len < (size_t)TECC_NORM_HEADER_SIZE)
        return NULL;

    sign = (int)data[0];
    if (sign == TECC_NORM_SIGN_NAN || sign == TECC_NORM_SIGN_INF ||
        sign == TECC_NORM_SIGN_NINF)
        return NULL;

    raw_weight = ((unsigned int)(unsigned char)data[1] << 8) | (unsigned int)(unsigned char)data[2];
    if (sign == TECC_NORM_SIGN_NEGATIVE)
        weight = (int)(65535 - raw_weight) - TECC_NORM_WEIGHT_BIAS;
    else
        weight = (int)raw_weight - TECC_NORM_WEIGHT_BIAS;

    ndigits = (int)((len - TECC_NORM_HEADER_SIZE - TECC_NORM_DSCALE_SIZE) / sizeof(uint16_t));
    digits_ptr = (const uint16_t *)(data + TECC_NORM_HEADER_SIZE);

    if (weight < 0) return NULL;
    for (i = weight + 1; i < ndigits; i++)
    {
        unsigned int fd = (unsigned int)digits_ptr[i];
        if (sign == TECC_NORM_SIGN_NEGATIVE) fd = 9 - fd;
        if (fd != 0) return NULL;
    }

    abs_val = 0;
    {
        int int_limit = (weight + 1 < ndigits) ? weight + 1 : ndigits;
        for (i = 0; i < int_limit; i++)
        {
            unsigned int d = (unsigned int)digits_ptr[i];
            if (sign == TECC_NORM_SIGN_NEGATIVE) d = 9 - d;
            if (abs_val > (uint64_t)INT64_MAX / 10) return NULL;
            abs_val = abs_val * 10 + d;
        }
    }
    for (i = ndigits; i <= weight; i++)
    {
        if (abs_val > (uint64_t)INT64_MAX / 10) return NULL;
        abs_val *= 10;
    }

    if (sign == TECC_NORM_SIGN_NEGATIVE)
    {
        if (abs_val > (uint64_t)INT64_MAX + 1) return NULL;
        if (abs_val == (uint64_t)INT64_MAX + 1)
            result = INT64_MIN;
        else
            result = -(int64_t)abs_val;
    }
    else
    {
        if (abs_val > (uint64_t)INT64_MAX) return NULL;
        result = (int64_t)abs_val;
    }

    out = (char *)pg_malloc(24);
    snprintf(out, 24, "%" PRId64, result);
    return out;
}

/* ==================== float4 output ==================== */

/*
 * tecc_type_output_float4 - Decode normalized binary to float4 text.
 *
 * Input: TECC normalized binary (sign+weight+reserved+digits).
 * Output: pg_malloc'd text (e.g., "3.40282e+38").
 *
 * Uses PG-style shortest-round-trip precision (ndig_trick) so output
 * matches backend float4out with extra_float_digits = 0.
 */
char *
tecc_type_output_float4(const char *data, size_t len)
{
    char *numeric_text;
    char *endptr;
    double val;
    char *result;

    if (data == NULL || len < (size_t)TECC_NORM_HEADER_SIZE)
        return NULL;

    /* ReqID:TECC4.0 - pass through special values */
    if (data[0] == TECC_NORM_SIGN_NAN)
        return pg_strdup("NaN");
    if (data[0] == TECC_NORM_SIGN_INF)
        return pg_strdup("Infinity");
    if (data[0] == TECC_NORM_SIGN_NINF)
        return pg_strdup("-Infinity");

    numeric_text = tecc_decode_normalized_to_text((const unsigned char *)data, len);
    if (numeric_text == NULL)
        return NULL;

    errno = 0;
    val = strtod(numeric_text, &endptr);
    pg_free(numeric_text);

    /* ReqID:TECC4.0 - Only reject true overflow (infinity), not gradual
     * underflow (subnormal like 1e-308). PG backend accepts these. */
    if (endptr == numeric_text)
        return NULL;
    if (errno == ERANGE && isinf(val))
        return NULL;

    result = (char *)pg_malloc(32);
    /* ReqID:TECC4.0 - Use PG's float_to_shortest_decimal_buf (Ryū algorithm) so
     * output matches backend float4out with extra_float_digits = 1.
     * Previously used %.6g which truncated precision, then %.9g which could show
     * binary noise digits vs PG backend output. */
    {
        float fval = (float)val;
        float_to_shortest_decimal_buf(fval, result);
    }
    return result;
}

/* ==================== float8 output ==================== */

/*
 * tecc_type_output_float8 - Decode normalized binary to float8 text.
 *
 * Input: TECC normalized binary (sign+weight+reserved+digits).
 * Output: pg_malloc'd text (e.g., "1.7976931348623157e+308").
 *
 * Uses PG-style shortest-round-trip precision (ndig_trick) so output
 * matches backend float8out with extra_float_digits = 0.
 */
char *
tecc_type_output_float8(const char *data, size_t len)
{
    char *numeric_text;
    char *endptr;
    double val;
    char *result;

    if (data == NULL || len < (size_t)TECC_NORM_HEADER_SIZE)
        return NULL;

    /* ReqID:TECC4.0 - pass through special values */
    if (data[0] == TECC_NORM_SIGN_NAN)
        return pg_strdup("NaN");
    if (data[0] == TECC_NORM_SIGN_INF)
        return pg_strdup("Infinity");
    if (data[0] == TECC_NORM_SIGN_NINF)
        return pg_strdup("-Infinity");

    numeric_text = tecc_decode_normalized_to_text((const unsigned char *)data, len);
    if (numeric_text == NULL)
        return NULL;

    errno = 0;
    val = strtod(numeric_text, &endptr);

    /* ReqID:TECC4.0 - Only reject true overflow (infinity), not gradual
     * underflow (subnormal like 1e-308). PG backend accepts these. */
    if (endptr == numeric_text)
    {
        pg_free(numeric_text);
        return NULL;
    }
    if (errno == ERANGE && isinf(val))
    {
        pg_free(numeric_text);
        return NULL;
    }

    result = (char *)pg_malloc(32);
    /* ReqID:TECC4.0 - Use PG's double_to_shortest_decimal_buf (Ryū algorithm) so
     * output matches backend float8out with extra_float_digits = 1.
     * Previously used %.17g which showed binary noise digits vs PG backend output. */
    pg_free(numeric_text);

    double_to_shortest_decimal_buf(val, result);
    return result;
}

/* ==================== text/varchar/bpchar output ==================== */

/*
 * ReqID:TECC4.0 - 按列声明长度（typmod）计算字符型文本的有效长度（Bug 9327）。
 *
 * 普通 varchar(N)/char(N) 列的长度约束由服务端输入函数（varchar_input/
 * bpchar_input）实现；密态列的值到服务端时已是密文，服务端无从判断明文长度，
 * 因此写入校验与读取定型都必须在前端做，依据列元数据里记录的 typmod（写入侧
 * 在加密前调用，读取侧在二进制转文本之后调用 —— 两侧共用本函数，判据一致）。
 *
 * 判据与服务端完全一致（src/backend/utils/adt/varchar.c）：按【字符】计数（多
 * 字节字符按一个字符算），超过 N 个字符时，只有第 N 个字符之后的字符全是空格
 * 才被服务端接受 —— 服务端会把它们截掉（varchar）/截掉后补齐（bpchar）再存。
 *
 * 返回 true  —— *effective_len 是应采用的长度：非字符型、未声明长度或未超长时
 *               等于 text_len；超长且超出部分全是空格时是前 N 个字符的字节长度，
 *               调用方应据此截断，从而与服务端实际存储的值一致。
 * 返回 false —— 超出部分含非空格字符。写入侧据此拒绝（服务端行为）；读取侧据
 *               此保持原值不动 —— 历史数据（旧版本写入、或未经本校验的写入路径）
 *               里可能真的存着这种值，静默截断等于丢数据。
 */
static bool
tecc_char_typmod_clip_len(Oid data_type, int type_mod,
                          const char *text, size_t text_len,
                          int encoding, bool strict, size_t *effective_len)
{
    int32       maxlen;
    size_t      mbmaxlen;
    size_t      pos;
    int32       nchars;
    size_t      i;

    if (effective_len != NULL)
        *effective_len = text_len;

    if (data_type != TECC_OUTPUT_VARCHAROID && data_type != TECC_OUTPUT_BPCHAROID)
        return true;

    /* typmod < VARHDRSZ 表示未声明长度限制 */
    if (type_mod < (int) sizeof(int32))
        return true;

    maxlen = (int32) type_mod - (int32) sizeof(int32);
    if (maxlen < 0 || text_len <= (size_t) maxlen)
        return true;

    /* 求前 maxlen 个字符占用的字节数，不切开多字节字符 */
    pos = 0;
    nchars = 0;
    while (pos < text_len && nchars < maxlen)
    {
        int mblen = pg_encoding_mblen(encoding, text + pos);

        if (mblen <= 0)
            mblen = 1;
        if (pos + (size_t) mblen > text_len)
            break;
        pos += (size_t) mblen;
        nchars++;
    }
    mbmaxlen = pos;

    /* 非严格模式即显式 cast 的语义：超出部分直接截掉，不再看其内容 */
    if (!strict)
    {
        if (effective_len != NULL)
            *effective_len = mbmaxlen;
        return true;
    }

    /* 第 maxlen 个字符之后还有非空格字符 —— 与服务端一样不接受 */
    for (i = mbmaxlen; i < text_len; i++)
    {
        if (text[i] != ' ')
            return false;
    }

    if (effective_len != NULL)
        *effective_len = mbmaxlen;
    return true;
}

bool
tecc_char_typmod_effective_len(Oid data_type, int type_mod,
                               const char *text, size_t text_len,
                               int encoding, size_t *effective_len)
{
    return tecc_char_typmod_clip_len(data_type, type_mod, text, text_len,
                                     encoding, true, effective_len);
}

/*
 * ReqID:TECC4.0 - Bug 9327 显式 cast 的长度定型（原地静默截断）。
 *
 * 原生 PG 对显式 cast（'x'::varchar(N)）是【静默截断】：超出部分是普通字符也照
 * 截、不报错（见 varchar()/bpchar() 的 isExplicit 分支）；只有赋值转换与字面量
 * 输入（varchar_input/bpchar_input）才在超出部分含非空格时报错。带 typmod 的密态
 * cast（'x'::varchar_enc(N) ENCRYPTED WITH (...)）到服务端时已是密文，服务端无从
 * 做这次截断，必须在前端补上，否则写入的值会长于 cast 声明的 N。
 *
 * declared_len 是类型名上写的那个数（语法里的 N，未声明时为 -1）；服务端 atttypmod
 * 是 N + VARHDRSZ，仅字符型基础类型做换算 —— 数值型的 typmod 在语法里被简化为只
 * 保留第一个数，还原不出 numeric 的编码，故不做处理。
 *
 * text 为 NUL 结尾缓冲区，原地改写；只可能变短，调用方无需提供容量。
 */
void
tecc_char_cast_len_clip(int declared_len, Oid base_oid,
                        char *text, int encoding)
{
    int32       type_mod;
    size_t      len;
    size_t      effective_len = 0;

    if (text == NULL)
        return;

    if (base_oid != TECC_OUTPUT_VARCHAROID && base_oid != TECC_OUTPUT_BPCHAROID)
        return;

    if (declared_len < 0)
        return;

    type_mod = (int32) declared_len + (int32) sizeof(int32);

    len = strlen(text);
    if (len == 0)
        return;

    if (tecc_char_typmod_clip_len(base_oid, type_mod, text, len, encoding,
                                  false, &effective_len)
        && effective_len < len)
        text[effective_len] = '\0';
}

/*
 * tecc_type_output_char - varchar/bpchar 输出：按列声明长度定型（Bug 9327）
 *
 * 先按密文格式取出文本，再按 typmod 定型：超出部分是空格时截断（服务端存储时
 * 也是这么做的），含非空格字符时保持原值（历史数据可能没经过写入校验）。
 * 修复写入侧之后新写入的值本身就在声明长度内，此处对它们是无操作。
 *
 * 与数值类型对称：tecc_type_output_numeric 也是按 typmod 定型（按 scale 取整）。
 *
 * 编码：前端文本管线按 UTF-8 处理（ICU 归一化同样假设 UTF-8），此处按 PG_UTF8
 * 计数；即使编码判断有偏差，最坏也只是少截一次尾部空格，不会丢非空格数据。
 *
 * Output: pg_malloc'd null-terminated string
 */
static char *
tecc_type_output_char(Oid type_oid, const char *data, size_t len, int type_mod)
{
    char       *result;
    size_t      effective_len = 0;
    size_t      result_len;

    result = tecc_type_output_text(data, len);
    if (result == NULL)
        return NULL;

    result_len = strlen(result);
    if (tecc_char_typmod_effective_len(type_oid, type_mod,
                                       result, result_len, PG_UTF8,
                                       &effective_len)
        && effective_len < result_len)
        result[effective_len] = '\0';

    return result;
}

/*
 * tecc_type_output_text - Convert text/bpchar/varchar binary to C string
 *
 * Two formats are supported (chosen at encryption time per algorithm):
 *   1. RND format:  4-byte network byte order length prefix + text data
 *      (prefix is needed because PKCS#7 padding makes decrypted length > original)
 *   2. DET/SEARCH format: raw text, no prefix
 *      (no prefix ensures server-side LIKE matching is not disrupted)
 *
 * Output: pg_malloc'd null-terminated string
 */
char *
tecc_type_output_text(const char *data, size_t len)
{
    char *result;

    if (data == NULL || len == 0)
        return NULL;

    /* RND format: 4-byte network byte order length prefix + text.
     * Validate strictly: prefix length must equal data length minus header,
     * because PKCS#7 padding has already been stripped at decrypt. */
    if (len >= TECC_TEXT_LENGTH_PREFIX_SIZE)
    {
        uint32 be_len = ((unsigned char)data[0] << 24) |
                        ((unsigned char)data[1] << 16) |
                        ((unsigned char)data[2] << 8) |
                        (unsigned char)data[3];
        if (be_len == len - TECC_TEXT_LENGTH_PREFIX_SIZE)
        {
            result = (char *)pg_malloc(be_len + 1);
            memcpy(result, data + TECC_TEXT_LENGTH_PREFIX_SIZE, be_len);
            result[be_len] = '\0';
            return result;
        }
    }

    /* DET/SEARCH format: raw text, no prefix */
    result = (char *)pg_malloc(len + 1);
    memcpy(result, data, len);
    result[len] = '\0';
    return result;
}

/* ==================== numeric output ==================== */

/*
 * tecc_round_numeric_to_scale — Round/pad numeric text to target scale.
 *
 * Input:  pg_malloc'd numeric string (e.g. "100.560", "-3.14", "42").
 * Output: New pg_malloc'd string with exactly @scale fractional digits.
 *         Caller must free the original string.
 *
 * Examples (scale=2):
 *   "100.560" → "100.56"   (round down)
 *   "100.556" → "100.56"   (round up at third decimal → already rounded; handles "100.556"→"100.56")
 *   "100.5"   → "100.50"   (pad)
 *   "42"      → "42.00"    (add decimal)
 *   "-3.145"  → "-3.15"    (round up, negative)
 */
static char *
tecc_round_numeric_to_scale(char *str, int scale)
{
    char       *dot;
    char       *end;
    int         frac_len;
    int         int_len;
    char       *result;
    int         result_len;

    if (str == NULL || scale < 0)
        return str;

    /* NaN, Infinity — no rounding */
    if (strcmp(str, "NaN") == 0 || strcmp(str, "Infinity") == 0 ||
        strcmp(str, "-Infinity") == 0)
        return str;

    dot = strchr(str, '.');
    if (dot != NULL)
        frac_len = (int)strlen(dot + 1);
    else
        frac_len = 0;

    /* Already at target scale — nothing to do */
    if (frac_len == scale)
        return str;

    /* Calculate integer part length (excluding sign) */
    int_len = (dot != NULL) ? (int)(dot - str) : (int)strlen(str);
    if (int_len > 0 && str[0] == '-')
        int_len--;

    /* If actual fractional digits > scale: round */
    if (frac_len > scale)
    {
        int         carry;
        int         i;
        int         round_digit;
        char       *frac_start;

        /* Find the digit at position 'scale' (the one to check for rounding) */
        frac_start = dot + 1;
        round_digit = frac_start[scale] - '0';

        /* Truncate at scale digits */
        frac_start[scale] = '\0';

        if (round_digit >= 5)
        {
            /* Propagate carry through fractional digits, then integer part */
            carry = 1;
            /* First, carry through the fractional part */
            for (i = scale - 1; i >= 0 && carry > 0; i--)
            {
                int d = frac_start[i] - '0' + carry;
                if (d >= 10)
                {
                    frac_start[i] = '0';
                    carry = 1;
                }
                else
                {
                    frac_start[i] = (char)(d + '0');
                    carry = 0;
                }
            }
            /* If carry propagates past the decimal point, update integer part */
            if (carry > 0)
            {
                int         int_start = (str[0] == '-') ? 1 : 0;
                int         ii;
                int         total_int = (int)(dot - (str + int_start));

                for (ii = total_int - 1; ii >= 0 && carry > 0; ii--)
                {
                    int d = (str[int_start + ii] - '0') + carry;
                    if (d >= 10)
                    {
                        carry = 1;
                    }
                    else
                    {
                        carry = 0;
                    }
                }
                if (carry > 0)
                {
                    /* Need to insert a leading '1' (e.g. 99.99 → 100.00) */
                    int new_int_len = total_int + 1;
                    result_len = (str[0] == '-' ? 1 : 0) + new_int_len + 1 + scale + 1;
                    result = (char *)pg_malloc(result_len);
                    if (result == NULL)
                        return str;
                    end = result;
                    if (str[0] == '-')
                        *end++ = '-';
                    *end++ = '1';
                    for (ii = 1; ii < new_int_len; ii++)
                        *end++ = '0';
                    *end++ = '.';
                    for (ii = 0; ii < scale; ii++)
                        *end++ = '0';
                    *end = '\0';
                    pg_free(str);
                    return result;
                }
                else
                {
                    /* Apply carry to integer digits */
                    int int_start = (str[0] == '-') ? 1 : 0;
                    for (ii = total_int - 1; ii >= 0; ii--)
                    {
                        int d = (str[int_start + ii] - '0') + 1;
                        if (d >= 10)
                            str[int_start + ii] = '0';
                        else
                        {
                            str[int_start + ii] = (char)(d + '0');
                            break;
                        }
                    }
                }
            }
        }
        /* Scale is now matched — trim string at correct position */
        if (scale > 0)
        {
            *(dot + 1 + scale) = '\0';
        }
        else
        {
            *dot = '\0';   /* Remove decimal point entirely */
        }

        /* Reallocate to actual size */
        result_len = (int)strlen(str) + 1;
        result = (char *)pg_malloc(result_len);
        if (result == NULL)
            return str;
        memcpy(result, str, result_len);
        pg_free(str);
        return result;
    }

    /* frac_len < scale: pad with zeros */
    {
        int pad = scale - frac_len;
        result_len = (int)strlen(str) + pad + 1;
        if (dot == NULL)
            result_len++;  /* extra byte for decimal point we'll add */
        result = (char *)pg_malloc(result_len);
        if (result == NULL)
            return str;
        end = result;
        /* Copy integer part (including sign) */
        if (dot != NULL)
        {
            size_t int_part_len = (size_t)(dot - str + 1);
            memcpy(end, str, int_part_len);
            end += int_part_len;
            /* Copy existing fractional digits */
            if (frac_len > 0)
            {
                memcpy(end, dot + 1, (size_t)frac_len);
                end += frac_len;
            }
        }
        else
        {
            /* No decimal point — add one */
            size_t len = strlen(str);
            memcpy(end, str, len);
            end += len;
            *end++ = '.';
        }
        /* Pad with zeros */
        memset(end, '0', (size_t)pad);
        end += pad;
        *end = '\0';
        pg_free(str);
        return result;
    }
}


/*
 * tecc_type_output_numeric - Decode normalized binary to numeric text.
 * Reference: backend numeric_out, but input is TECC normalized binary.
 *
 * Input: TECC normalized binary (sign+weight+reserved+digits).
 * Output: pg_malloc'd numeric text representation (e.g., "50000.00").
 * For NUMERICOID, type_mod is used for scale/rounding; ignored for other types.
 */
char *
tecc_type_output_numeric(const char *data, size_t len, int type_mod)
{
    char       *result;

    result = tecc_decode_normalized_to_text((const unsigned char *)data, len);
    if (result == NULL)
        return NULL;

    /* ReqID:TECC4.0 - Apply typmod scale constraint when present.
     * For NUMERIC(p,s), round/pad the decoded text to exactly 's'
     * fractional digits, matching backend numeric_out behavior. */
    if (type_mod >= (int)sizeof(int32))
    {
        int scale = (type_mod - (int)sizeof(int32)) & 0xFFFF;

        result = tecc_round_numeric_to_scale(result, scale);
    }

    return result;
}


/* ==================== Main dispatch function ==================== */

/*
 * tecc_type_output_data - Main dispatch: convert binary to text by type OID
 */
char *
tecc_type_output_data(Oid type_oid, int type_mod,
                      const char *binary_data, size_t binary_len)
{
    if (binary_data == NULL || binary_len == 0)
        return NULL;

    /* ReqID:TECC4.0 - RND/DET/OPE decryption produces TECC normalized binary
     * (sign+weight+reserved+digits) shared across all numeric encrypted types
     * for cross-type ciphertext compatibility. Decode via column type OID. */
    switch (type_oid)
    {
        case TECC_OUTPUT_INT2OID:
            return tecc_type_output_int2(binary_data, binary_len);

        case TECC_OUTPUT_INT4OID:
            return tecc_type_output_int4(binary_data, binary_len);

        case TECC_OUTPUT_INT8OID:
            return tecc_type_output_int8(binary_data, binary_len);

        case TECC_OUTPUT_FLOAT4OID:
            return tecc_type_output_float4(binary_data, binary_len);

        case TECC_OUTPUT_FLOAT8OID:
            return tecc_type_output_float8(binary_data, binary_len);

        case TECC_OUTPUT_NUMERICOID:
            return tecc_type_output_numeric(binary_data, binary_len, type_mod);

        case TECC_OUTPUT_VARCHAROID:
        case TECC_OUTPUT_BPCHAROID:
            return tecc_type_output_char(type_oid, binary_data, binary_len,
                                         type_mod);

        case TECC_OUTPUT_TEXTOID:
            return tecc_type_output_text(binary_data, binary_len);

        default:
            /* Unknown type: return NULL, caller handles fallback */
            return NULL;
    }
}
/* ==================== Numeric input: text to NumericData binary ==================== */

/*
 * Internal helper: parse numeric text, extract digits, dweight, dscale.
 * This is a frontend reimplementation of the parsing portion of
 * backend set_var_from_str (numeric.c:6862).
 */
static int
tecc_parse_numeric_text(const char *str, int len,
                         int *sign, int *dweight, int *dscale,
                         int *ddigits, int *exponent,
                         int *decdigits, int decdigits_max)
{
    int i = 0;
    int have_dp = 0;
    int num_digits = 0;
    int di = 0;
    int sig = TECC_NUMERIC_POS;
    int exp = 0;
    int before_dp = 0;  /* digits before decimal point */
    int after_dp = 0;   /* digits after decimal point */

    *sign = TECC_NUMERIC_POS;

    /* Sign */
    if (i < len && str[i] == '-')
    {
        sig = TECC_NUMERIC_NEG;
        i++;
    }
    else if (i < len && str[i] == '+')
    {
        i++;
    }

    /* Parse main body (digits and decimal point) */
    while (i < len)
    {
        char c = str[i];
        if (c >= '0' && c <= '9')
        {
            int d = c - '0';
            if (num_digits == 0 && d != 0)
            {
                /* first non-zero digit found */
            }

            decdigits[di++] = d;
            num_digits++;

            if (!have_dp)
                before_dp++;
            else
                after_dp++;

            if (di >= decdigits_max)
                return -1;
            i++;
        }
        else if (c == '.')
        {
            if (have_dp) return -1;
            have_dp = 1;
            i++;
        }
        else if (c == 'e' || c == 'E')
        {
            int exp_sign = 1;
            int exp_val = 0;
            i++;
            if (i < len && str[i] == '-') { exp_sign = -1; i++; }
            else if (i < len && str[i] == '+') { i++; }
            while (i < len && str[i] >= '0' && str[i] <= '9')
            {
                exp_val = exp_val * 10 + (str[i] - '0');
                i++;
            }
            exp = exp_sign * exp_val;
            break;
        }
        else if (c == ' ')
        {
            i++;
            while (i < len && str[i] == ' ') i++;
            if (i < len) return -1;
            break;
        }
        else
        {
            return -1;
        }
    }

    if (num_digits == 0)
        return -1;

    /* Calculate dweight and dscale */
    if (have_dp)
    {
        /* before_dp counts digits before DP - dweight = before_dp - 1 */
        if (before_dp > 0)
            *dweight = before_dp - 1;
        else
            *dweight = -1;  /* e.g. ".123" */
        *dscale = after_dp;
    }
    else
    {
        /* No decimal point: all digits are integer part */
        if (num_digits > 0)
            *dweight = num_digits - 1;
        else
            *dweight = -1;
        *dscale = 0;
    }

    /* Apply exponent */
    if (exp != 0)
    {
        if (exp > 0)
        {
            /* Shift decimal point right */
            int shift = exp;
            if (*dscale >= shift)
            {
                *dscale -= shift;
                *dweight += shift;
            }
            else
            {
                *dweight += *dscale + (shift - *dscale);
                *dscale = 0;
            }
        }
        else
        {
            /* Shift decimal point left */
            int shift = -exp;
            if (*dweight >= shift)
            {
                *dweight -= shift;
                *dscale += shift;
            }
            else
            {
                *dscale += *dweight + 1 + (shift - *dweight - 1);
                *dweight = -1;
            }
        }
    }

    *sign = sig;
    *exponent = 0;  /* Already applied */
    *ddigits = di;
    return 0;
}

/*
 * tecc_type_input_numeric_from_text - Convert numeric text to NumericData varlena binary
 *
 * Converts a numeric text string (e.g., "50000.00", "-123.456") to a PostgreSQL
 * NumericData varlena binary representation suitable for RND encryption.
 *
 * Reference: backend set_var_from_str (numeric.c:6862) + make_result (numeric.c:7344)
 *
 * The output is a pg_malloc'd buffer containing a complete NumericData structure
 * (4-byte varlena header + NumericChoice with sign/weight/dscale/digits).
 * Both short and long formats are used as appropriate.
 *
 * @param text       Null-terminated numeric text string
 * @param text_len   Length of text string
 * @param binary_out Output: pg_malloc'd NumericData binary
 * @param binary_len Output: length of binary data
 * @return           0 on success, -1 on error
 */
int
tecc_type_input_numeric_from_text(const char *text, size_t text_len,
                                   unsigned char **binary_out,
                                   size_t *binary_len)
{
    int sign;                   /* TECC_NUMERIC_POS or TECC_NUMERIC_NEG */
    int dweight;                /* Decimal weight: digits before DP - 1 */
    int dscale;                 /* Decimal scale: digits after DP */
    int ddigits;                /* Total decimal digits count */
    int exponent;               /* Dummy, exponent already applied in parse */
    int decdigits[TECC_DEC_DIGITS * 4 + 1000]; /* Buffer for decimal digits + NBASE padding */
    int ndigits;                /* NBASE digit count */
    int weight;                 /* NBASE weight */
    int offset;                 /* Alignment offset for NBASE packing */
    int strip_leading;          /* Leading NBASE digits to strip */
    int strip_trailing;         /* Trailing NBASE digits to strip */
    TeccNumericDigit *nbase_digits = NULL; /* Allocated NBASE digit array */
    int total_size;             /* Total varlena size */
    char *result;               /* Allocated NumericData */
    int short_format;           /* Whether to use short format */
    int zero_value;             /* Whether value is zero */
    int parse_ret;

    /* Validate input */
    if (text == NULL || text_len == 0 || binary_out == NULL || binary_len == NULL)
        return -1;

    /* Check for special values (NaN, Infinity) before numeric parsing,
     * since tecc_parse_numeric_text only handles digit strings. */
    {
        const char *p = text;
        while (p < text + (int)text_len && *p == ' ') p++;

        /* Check for NaN (case-insensitive) */
        if ((text_len - (p - text)) >= 3 &&
            pg_strncasecmp(p, "NaN", 3) == 0 &&
            ((text_len - (p - text)) == 3 || !isalnum((unsigned char)p[3])))
        {
            *binary_len = TECC_NUMERIC_HDRSZ_SHORT;
            result = (char *)pg_malloc(*binary_len);
            if (!result) return -1;
            SET_VARSIZE(result, *binary_len);
            *((uint16 *)(result + TECC_VARHDRSZ)) = TECC_NUMERIC_NAN;
            *binary_out = (unsigned char *)result;
            return 0;
        }

        /* Reject Infinity/Inf for NUMERIC — PG numeric_in rejects these */
        if ((text_len - (p - text)) >= 3 &&
            ((p[0] == '+' || p[0] == '-') ?
             (pg_strncasecmp(p + 1, "Inf", 3) == 0) :
             (pg_strncasecmp(p, "Inf", 3) == 0)))
        {
            return -1;
        }
    }

    /* Parse the numeric text (regular numeric strings only) */
    memset(decdigits, 0, sizeof(decdigits));
    parse_ret = tecc_parse_numeric_text(text, (int)text_len,
                                         &sign, &dweight, &dscale,
                                         &ddigits, &exponent,
                                         decdigits,
                                         (int)(sizeof(decdigits)/sizeof(decdigits[0])));
    if (parse_ret != 0)
        return -1;

    /* If ddigits == 0 after parsing, not a valid number (NaN/Inf already handled above) */
    if (ddigits == 0)
        return -1;

    /* Calculate NBASE weight */
    if (dweight >= 0)
        weight = (dweight + 1 + TECC_DEC_DIGITS - 1) / TECC_DEC_DIGITS - 1;
    else
        weight = -((-dweight - 1) / TECC_DEC_DIGITS + 1);

    offset = (weight + 1) * TECC_DEC_DIGITS - (dweight + 1);

    /*
     * Add padding zeros for NBASE alignment (same as backend set_var_from_str).
     * Front-pad with DEC_DIGITS zeros, then move digits to align at offset.
     */
    {
        /* Shift existing digits right by offset */
        if (offset > 0)
        {
            int j;
            for (j = ddigits - 1; j >= 0; j--)
                decdigits[j + offset] = decdigits[j];
            for (j = 0; j < offset; j++)
                decdigits[j] = 0;
            ddigits += offset;
        }
        /* Pad to multiple of DEC_DIGITS */
        while (ddigits % TECC_DEC_DIGITS != 0)
            decdigits[ddigits++] = 0;
    }

    /* Calculate NBASE ndigits */
    ndigits = ddigits / TECC_DEC_DIGITS;

    /* Allocate NBASE digit array */
    nbase_digits = (TeccNumericDigit *)pg_malloc(ndigits * sizeof(TeccNumericDigit));
    if (!nbase_digits)
        return -1;

    /* Pack decimal digits into NBASE (4 decimal digits per TeccNumericDigit) */
    {
        int d; /* Index into decdigits */
        int n_idx; /* Index into nbase_digits */
        for (d = 0, n_idx = 0; n_idx < ndigits; n_idx++)
        {
            int val = 0;
            int k;
            for (k = 0; k < TECC_DEC_DIGITS && d < ddigits; k++, d++)
            {
                val = val * 10 + decdigits[d];
            }
            nbase_digits[n_idx] = (TeccNumericDigit)val;
        }
    }

    /* Strip leading zeros (frontend version of strip_var) */
    strip_leading = 0;
    while (strip_leading < ndigits && nbase_digits[strip_leading] == 0)
        strip_leading++;
    if (strip_leading > 0)
    {
        weight -= strip_leading;
        ndigits -= strip_leading;
        if (ndigits > 0)
        {
            memmove(nbase_digits, nbase_digits + strip_leading,
                    ndigits * sizeof(TeccNumericDigit));
        }
    }

    /* Strip trailing zeros */
    if (ndigits > 0)
    {
        strip_trailing = 0;
        while (strip_trailing < ndigits &&
               nbase_digits[ndigits - 1 - strip_trailing] == 0)
            strip_trailing++;
        ndigits -= strip_trailing;

        /* Also adjust dscale for stripped trailing NBASE digits */
        if (strip_trailing > 0 && dscale > 0)
        {
            int dscale_reduce = strip_trailing * TECC_DEC_DIGITS;
            dscale = (dscale > dscale_reduce) ? (dscale - dscale_reduce) : 0;
        }
    }

    /* Handle zero value */
    zero_value = 0;
    if (ndigits <= 0)
    {
        ndigits = 0;
        weight = 0;
        sign = TECC_NUMERIC_POS;
        dscale = 0;
        zero_value = 1;
    }

    /* Decide short or long format (same as backend make_result) */
    short_format = (!zero_value) &&
                   (dscale <= TECC_NUMERIC_SHORT_DSCALE_MASK >> TECC_NUMERIC_SHORT_DSCALE_SHIFT) &&
                   (weight >= -64 && weight <= 63);
    /* Also use short for zero */
    if (zero_value)
        short_format = 1;

    /* Allocate result buffer */
    if (short_format)
        total_size = TECC_NUMERIC_HDRSZ_SHORT + ndigits * sizeof(TeccNumericDigit);
    else
        total_size = TECC_NUMERIC_HDRSZ_LONG + ndigits * sizeof(TeccNumericDigit);

    result = (char *)pg_malloc(total_size);
    if (!result)
    {
        pg_free(nbase_digits);
        return -1;
    }

    /* Fill in the NumericData structure */
    SET_VARSIZE(result, total_size);

    if (short_format)
    {
        uint16 n_header = TECC_NUMERIC_SHORT;

        if (sign == TECC_NUMERIC_NEG && !zero_value)
            n_header |= TECC_NUMERIC_SHORT_SIGN_MASK;

        n_header |= (dscale << TECC_NUMERIC_SHORT_DSCALE_SHIFT);

        if (weight < 0)
            n_header |= TECC_NUMERIC_SHORT_WEIGHT_SIGN_MASK;
        n_header |= (weight & TECC_NUMERIC_SHORT_WEIGHT_MASK);

        *((uint16 *)(result + TECC_VARHDRSZ)) = n_header;
    }
    else
    {
        uint16 n_sign_dscale = (uint16)(sign & TECC_NUMERIC_SIGN_MASK);
        n_sign_dscale |= (uint16)(dscale & TECC_NUMERIC_DSCALE_MASK);
        *((uint16 *)(result + TECC_VARHDRSZ)) = n_sign_dscale;
        *((int16 *)(result + TECC_VARHDRSZ + sizeof(uint16))) = (int16)weight;
    }

    /* Copy NBASE digits */
    if (ndigits > 0)
    {
        TeccNumericDigit *dest;
        if (short_format)
            dest = ((TeccNumericShort *)(result + TECC_VARHDRSZ))->n_data;
        else
            dest = ((TeccNumericLong *)(result + TECC_VARHDRSZ))->n_data;

        memcpy(dest, nbase_digits, ndigits * sizeof(TeccNumericDigit));
    }

    pg_free(nbase_digits);

    *binary_out = (unsigned char *)result;
    *binary_len = (size_t)total_size;
    return 0;
}

/*
 * ReqID:TECC4.0 - tecc_formatter_prepare_plaintext
 *   Input validation + text to PG internal binary conversion.
 *
 * Converts SQL text representation to RND-ready plaintext format.
 * For numeric types: text -> PG internal binary (int16/int32/int64/float/double/NumericData).
 * For text types:   text -> 4-byte network byte order length prefix + text data.
 *
 * This is the common data preparation step shared by all encryption algorithms.
 * Validation rules are server-consistent (match PG backend *_in functions).
 * DET/OPE/SEARCH normalization is applied on top of this result when needed.
 */
TeccFormatterErrorCode
tecc_formatter_prepare_plaintext(Oid data_type, int type_mod,
                                  const char *text, size_t text_len,
                                  unsigned char **binary_out, size_t *binary_len)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    TeccFormatterErrorCode val_ret;

    /* Validate parameters */
    if (text == NULL || binary_out == NULL || binary_len == NULL)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    *binary_out = NULL;
    *binary_len = 0;

    if (text_len == 0 && TECC_IS_NUMERIC_TYPE(data_type))
        return TECC_FORMATTER_ERROR_INVALID_INPUT;

    /* Step 1: Input validation (algorithm-agnostic, server-consistent rules) */
    val_ret = tecc_validate_input_data((const unsigned char *)text, text_len,
                                        data_type, TECC_ALGO_RND);
    if (val_ret != TECC_FORMATTER_SUCCESS)
        return val_ret;

    /* Step 2: Convert text to PG internal binary based on data type */
    switch (data_type)
    {
        /* ReqID:TECC4.0 - All numeric types normalize to NumericData (PG numeric varlena)
         * for cross-type ciphertext compatibility. INT2/4/8 and FLOAT4/8 all go
         * through the same tecc_type_input_numeric_from_text path.
         * The column type OID at decrypt time drives the output conversion. */
        case INT2OID:
        case INT4OID:
        case INT8OID:
        case FLOAT4OID:
        case FLOAT8OID:
        case NUMERICOID:
        {
            int ret;

            ret = tecc_type_input_numeric_from_text(text, text_len,
                                                     binary_out, binary_len);
            if (ret != 0)
                return TECC_FORMATTER_ERROR_SYSTEM;
            return TECC_FORMATTER_SUCCESS;
        }

        case VARCHAROID:
        case BPCHAROID:
        case TEXTOID:
        {
            /* Add 4-byte network byte order length prefix */
            if (text_len > UINT32_MAX - TECC_TEXT_LENGTH_PREFIX_SIZE)
                return TECC_FORMATTER_ERROR_DATA_TOO_LARGE;

            *binary_len = TECC_TEXT_LENGTH_PREFIX_SIZE + text_len;
            *binary_out = (unsigned char *)pg_malloc(*binary_len);
            if (*binary_out == NULL)
                return TECC_FORMATTER_ERROR_MEMORY_ALLOC;

            (*binary_out)[0] = (unsigned char)((text_len >> 24) & 0xFF);
            (*binary_out)[1] = (unsigned char)((text_len >> 16) & 0xFF);
            (*binary_out)[2] = (unsigned char)((text_len >> 8) & 0xFF);
            (*binary_out)[3] = (unsigned char)(text_len & 0xFF);
            memcpy(*binary_out + TECC_TEXT_LENGTH_PREFIX_SIZE, text, text_len);
            return TECC_FORMATTER_SUCCESS;
        }

        default:
            return TECC_FORMATTER_ERROR_INVALID_TYPE;
    }
}
/* End - ReqID:TECC4.0 */
