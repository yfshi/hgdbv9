/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_normalization_common.c
 *
 * Abstract:
 *   Common normalization implementation for TECC encryption algorithms.
 *   Provides shared normalization functions for OPE and DET algorithms.
 *
 * Authored by <wangxp@highgo.com>, 20260409.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_normalization_common.c
 */

#include "tecc_normalization_common.h"
#include "tecc_formatter_utils.h"
#include "tecc_icu_normalizer.h"  /* For tecc_icu_normalizer_generate_sortkey */
#include "common/shortest_dec.h"
#include "../../../include/common/tecc_algorithm/tecc_algorithm_api.h"
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <limits.h>

/* Frontend memory management */
#include "../../../include/common/fe_memutils.h"

/* PostgreSQL numeric type definitions for client-side parsing */
/* These definitions are based on src/backend/utils/adt/numeric.c */

/* Numeric digit type (NBASE = 10000) */
typedef uint16 NumericDigit;

/* Numeric header flags */
#define NUMERIC_SIGN_MASK    0xC000
#define NUMERIC_POS          0x0000
#define NUMERIC_NEG          0x4000
#define NUMERIC_SHORT        0x8000
#define NUMERIC_SPECIAL      0xC000

/* Special values */
#define NUMERIC_EXT_SIGN_MASK    0xF000
#define NUMERIC_NAN              0xC000
#define NUMERIC_PINF             0xD000
#define NUMERIC_NINF             0xF000

/* Short format masks */
#define NUMERIC_SHORT_SIGN_MASK      0x2000
#define NUMERIC_SHORT_WEIGHT_MASK    0x1F00
#define NUMERIC_SHORT_WEIGHT_SIGN_MASK 0x0080
#define NUMERIC_SHORT_DSCALE_MASK    0x007F
#define NUMERIC_SHORT_DSCALE_SHIFT   0
#define NUMERIC_SHORT_WEIGHT_SHIFT   8

/* Long format masks */
#define NUMERIC_DSCALE_MASK          0x3FFF

/* Numeric data structures */
struct NumericLong
{
    uint16      n_sign_dscale;  /* Sign + display scale */
    int16       n_weight;       /* Weight of 1st digit */
    NumericDigit n_data[1];     /* Digits (flexible array) */
};

struct NumericShort
{
    uint16      n_header;       /* Header word */
    NumericDigit n_data[1];     /* Digits (flexible array) */
};

union NumericChoice
{
    uint16              n_header;       /* Header word */
    struct NumericLong  n_long;         /* Long form (4-byte header) */
    struct NumericShort n_short;        /* Short form (2-byte header) */
};

struct NumericData
{
    int32           vl_len_;            /* varlena header */
    union NumericChoice choice;         /* choice of format */
};

/* Helper macros for numeric parsing */
#define NUMERIC_IS_SHORT(n)     ((((struct NumericData *)(n))->choice.n_header & NUMERIC_SIGN_MASK) == NUMERIC_SHORT)
#define NUMERIC_IS_SPECIAL(n)   ((((struct NumericData *)(n))->choice.n_header & NUMERIC_SIGN_MASK) == NUMERIC_SPECIAL)
#define NUMERIC_HEADER_IS_SHORT(n)  ((((struct NumericData *)(n))->choice.n_header & 0x8000) != 0)
#define NUMERIC_HEADER_SIZE(n) \
    (sizeof(int32) + sizeof(uint16) + \
     (NUMERIC_HEADER_IS_SHORT(n) ? 0 : sizeof(int16)))

#define NUMERIC_NDIGITS(num) \
    ((VARSIZE(num) - NUMERIC_HEADER_SIZE(num)) / sizeof(NumericDigit))

/* VARSIZE macro for varlena types */
#ifndef VARSIZE
#define VARSIZE(PTR)            (((int32 *)(PTR))[0] & 0x3FFFFFFF)  /* Clear high bits */
#endif

/*
 * tecc_normalize_numeric_text_common - Normalize numeric text string to common format
 *
 * DEPRECATED: This function is the new text-based version of tecc_normalize_numeric_common.
 * Frontend code should use this function instead of the Datum-based version.
 */
int
tecc_normalize_numeric_text_common(const char *numeric_text,
                                   size_t text_len,
                                   TeccNumericNormalizedCommon *norm_num)
{
    TeccFormatterErrorCode fmt_result;
    uint8_t sign;
    int16_t weight;
    uint16_t digits[TECC_NUMERIC_MAX_DIGITS_COMMON];
    int ndigits;
    int i;
    size_t packed_digits_count;

    if (!norm_num || !numeric_text)
        return TECC_NORMALIZATION_ERROR_INVALID_PARAM;

    /* Initialize output structure */
    norm_num->sign = TECC_NUMERIC_SIGN_POSITIVE_COMMON;
    norm_num->weight = 0;
    norm_num->ndigits = 0;
    norm_num->digits = NULL;

    /* Parse numeric text using formatter utility function */
    fmt_result = tecc_formatter_parse_numeric_text(numeric_text, text_len,
                                                   &sign, &weight, digits, &ndigits);

    if (fmt_result != TECC_FORMATTER_SUCCESS) {
        /* Map formatter error to normalization error */
        switch (fmt_result) {
            case TECC_FORMATTER_ERROR_INVALID_PARAM:
                return TECC_NORMALIZATION_ERROR_INVALID_PARAM;
            case TECC_FORMATTER_ERROR_MEMORY_ALLOC:
                return TECC_NORMALIZATION_ERROR_MEMORY_ALLOC;
            case TECC_FORMATTER_ERROR_DATA_FORMAT:
            case TECC_FORMATTER_ERROR_INVALID_INPUT:
                return TECC_NORMALIZATION_ERROR_INVALID_TYPE;
            default:
                return TECC_NORMALIZATION_ERROR_INVALID_TYPE;
        }
    }

    /* Map sign from formatter to common format */
    switch (sign) {
        case TECC_NUMERIC_SIGN_POSITIVE:
            norm_num->sign = TECC_NUMERIC_SIGN_POSITIVE_COMMON;
            break;
        case TECC_NUMERIC_SIGN_NEGATIVE:
            norm_num->sign = TECC_NUMERIC_SIGN_NEGATIVE_COMMON;
            break;
        case TECC_NUMERIC_SIGN_ZERO:
            norm_num->sign = TECC_NUMERIC_SIGN_ZERO_COMMON;
            break;
        case TECC_NUMERIC_SIGN_NAN:
            norm_num->sign = TECC_NUMERIC_SIGN_NAN_COMMON;
            break;
        case TECC_NUMERIC_SIGN_INF:
            norm_num->sign = TECC_NUMERIC_SIGN_INF_COMMON;
            break;
        case TECC_NUMERIC_SIGN_NINF:
            norm_num->sign = TECC_NUMERIC_SIGN_NINF_COMMON;
            break;
        default:
            norm_num->sign = TECC_NUMERIC_SIGN_NAN_COMMON;
            break;
    }

    /* For special values (NaN, Inf, Zero), we're done */
    if (sign == TECC_NUMERIC_SIGN_NAN || sign == TECC_NUMERIC_SIGN_INF ||
        sign == TECC_NUMERIC_SIGN_NINF || sign == TECC_NUMERIC_SIGN_ZERO) {
        norm_num->weight = 0;
        norm_num->ndigits = 0;
        norm_num->digits = NULL;
        return TECC_NORMALIZATION_SUCCESS;
    }

    norm_num->weight = weight;
    norm_num->ndigits = ndigits;

    /* Allocate and copy digits if we have any */
    if (ndigits > 0) {
        packed_digits_count = (ndigits + 3) / 4;  /* 4 digits per uint16 */
        norm_num->digits = (uint16_t *)pg_malloc(packed_digits_count * sizeof(uint16_t));
        if (!norm_num->digits)
            return TECC_NORMALIZATION_ERROR_MEMORY_ALLOC;

        /* Copy digits */
        for (i = 0; i < packed_digits_count; i++) {
            norm_num->digits[i] = digits[i];
        }
    }

    /* Validate range */
    if (norm_num->weight < TECC_NUMERIC_MIN_WEIGHT_COMMON ||
        norm_num->weight > TECC_NUMERIC_MAX_WEIGHT_COMMON)
        return TECC_NORMALIZATION_ERROR_NUMERIC_OVERFLOW;

    if (norm_num->ndigits < 0 || norm_num->ndigits > TECC_NUMERIC_MAX_DIGITS_COMMON)
        return TECC_NORMALIZATION_ERROR_NUMERIC_OVERFLOW;

    return TECC_NORMALIZATION_SUCCESS;
}

/*
 * tecc_normalize_numeric_common - Normalize PostgreSQL numeric type to common format
 *
 * DEPRECATED: This function uses Datum type which is a backend type.
 * Frontend code should use tecc_normalize_numeric_text_common instead.
 * REMOVED: Frontend code should not use Datum type.
 */

/*
 * tecc_encode_numeric_normalized_common - Encode normalized numeric structure to byte stream
 */
int
tecc_encode_numeric_normalized_common(TeccNumericNormalizedCommon *norm_num,
                                      unsigned char **output_data,
                                      size_t *output_len)
{
    size_t packed_digits_count;
    size_t total_size;
    unsigned char *buffer;

    if (!norm_num || !output_data || !output_len)
        return TECC_NORMALIZATION_ERROR_INVALID_PARAM;

    /* Calculate required size: header + packed digits */
    packed_digits_count = (norm_num->ndigits + 3) / 4;  /* 4 digits per uint16 */
    total_size = TECC_NUMERIC_HEADER_SIZE_COMMON + packed_digits_count * sizeof(uint16_t);

    /* Allocate output buffer */
    buffer = (unsigned char *)pg_malloc(total_size);
    if (!buffer)
        return TECC_NORMALIZATION_ERROR_MEMORY_ALLOC;

    /* Encode header */
    buffer[0] = norm_num->sign;  /* Sign (1 byte) */

    /* Weight (2 bytes, big-endian) */
    buffer[1] = (norm_num->weight >> 8) & 0xFF;
    buffer[2] = norm_num->weight & 0xFF;

    /* NDigits (2 bytes, big-endian) */
    buffer[3] = (norm_num->ndigits >> 8) & 0xFF;
    buffer[4] = norm_num->ndigits & 0xFF;

    /* Reserved bytes (3 bytes) */
    buffer[5] = 0;
    buffer[6] = 0;
    buffer[7] = 0;

    /* Copy packed digits */
    if (packed_digits_count > 0 && norm_num->ndigits > 0) {
        memcpy(buffer + TECC_NUMERIC_HEADER_SIZE_COMMON, norm_num->digits,
               packed_digits_count * sizeof(uint16_t));
    }

    *output_data = buffer;
    *output_len = total_size;

    return TECC_NORMALIZATION_SUCCESS;
}

/*
 * tecc_encode_numeric_normalized_to_buffer_common - Encode normalized numeric structure to existing buffer
 */
int
tecc_encode_numeric_normalized_to_buffer_common(TeccNumericNormalizedCommon *norm_num,
                                                unsigned char *buffer,
                                                size_t buffer_size,
                                                size_t *output_len)
{
    size_t packed_digits_count;
    size_t total_size;

    if (!norm_num || !buffer || !output_len)
        return TECC_NORMALIZATION_ERROR_INVALID_PARAM;

    /* Calculate required size: header + packed digits */
    packed_digits_count = (norm_num->ndigits + 3) / 4;  /* 4 digits per uint16 */
    total_size = TECC_NUMERIC_HEADER_SIZE_COMMON + packed_digits_count * sizeof(uint16_t);

    /* Check if buffer is large enough */
    if (buffer_size < total_size)
        return TECC_NORMALIZATION_ERROR_MEMORY_ALLOC;

    /* Encode header */
    buffer[0] = norm_num->sign;  /* Sign (1 byte) */

    /* Weight (2 bytes, big-endian) */
    buffer[1] = (norm_num->weight >> 8) & 0xFF;
    buffer[2] = norm_num->weight & 0xFF;

    /* NDigits (2 bytes, big-endian) */
    buffer[3] = (norm_num->ndigits >> 8) & 0xFF;
    buffer[4] = norm_num->ndigits & 0xFF;

    /* Reserved bytes (3 bytes) */
    buffer[5] = 0;
    buffer[6] = 0;
    buffer[7] = 0;

    /* Copy packed digits */
    if (packed_digits_count > 0 && norm_num->ndigits > 0) {
        memcpy(buffer + TECC_NUMERIC_HEADER_SIZE_COMMON, norm_num->digits,
               packed_digits_count * sizeof(uint16_t));
    }

    *output_len = total_size;
    return TECC_NORMALIZATION_SUCCESS;
}

/*
 * tecc_free_numeric_normalized_common - Free normalized numeric structure
 */
void
tecc_free_numeric_normalized_common(TeccNumericNormalizedCommon *norm_num)
{
    if (!norm_num)
        return;

    if (norm_num->digits)
    {
        pg_free(norm_num->digits);
        norm_num->digits = NULL;
    }

    norm_num->sign = 0;
    norm_num->weight = 0;
    norm_num->ndigits = 0;
}

/*
 * tecc_normalize_int2_common - Normalize int2 to common format
 */
static int
tecc_normalize_int2_common(int16 value,
                           TeccNumericNormalizedCommon *norm_num)
{
    int64 abs_value;
    int digit_count;
    int64 temp;
    size_t packed_count;
    uint16_t packed_digit;
    int packed_index;
    int digit_index;

    if (!norm_num)
        return TECC_NORMALIZATION_ERROR_INVALID_PARAM;

    /* Initialize structure */
    norm_num->sign = (value >= 0) ? TECC_NUMERIC_SIGN_POSITIVE_COMMON : TECC_NUMERIC_SIGN_NEGATIVE_COMMON;
    norm_num->weight = 0;

    /* Handle zero */
    if (value == 0)
    {
        norm_num->sign = TECC_NUMERIC_SIGN_ZERO_COMMON;
        norm_num->ndigits = 0;
        norm_num->digits = NULL;
        return TECC_NORMALIZATION_SUCCESS;
    }

    /* Convert to absolute value */
    abs_value = (value < 0) ? -(int64)value : (int64)value;

    /* Count digits */
    digit_count = 0;
    temp = abs_value;
    while (temp > 0)
    {
        digit_count++;
        temp /= 10;
    }

    norm_num->ndigits = digit_count;

    /* Allocate digits array */
    packed_count = (digit_count + 3) / 4;
    norm_num->digits = (uint16_t *)pg_malloc(packed_count * sizeof(uint16_t));
    if (!norm_num->digits)
        return TECC_NORMALIZATION_ERROR_MEMORY_ALLOC;

    /* Extract digits */
    temp = abs_value;
    packed_digit = 0;
    packed_index = 0;
    digit_index = 0;

    while (temp > 0)
    {
        int digit = temp % 10;
        temp /= 10;

        /* Pack 4 digits into one uint16 */
        if (digit_index % 4 == 0 && digit_index > 0)
        {
            norm_num->digits[packed_index++] = packed_digit;
            packed_digit = 0;
        }

        packed_digit = (packed_digit << 4) | (digit & 0x0F);
        digit_index++;
    }

    /* Store last packed digit */
    if (digit_index > 0)
    {
        /* Pad with zeros if needed */
        int remaining = digit_index % 4;
        if (remaining > 0)
        {
            packed_digit <<= (4 - remaining) * 4;
        }
        norm_num->digits[packed_index] = packed_digit;
    }

    return TECC_NORMALIZATION_SUCCESS;
}

/*
 * tecc_normalize_int4_common - Normalize int4 to common format
 */
static int
tecc_normalize_int4_common(int32 value,
                           TeccNumericNormalizedCommon *norm_num)
{
    int64 abs_value;
    int digit_count;
    int64 temp;
    size_t packed_count;
    uint16_t packed_digit;
    int packed_index;
    int digit_index;

    if (!norm_num)
        return TECC_NORMALIZATION_ERROR_INVALID_PARAM;

    /* Initialize structure */
    norm_num->sign = (value >= 0) ? TECC_NUMERIC_SIGN_POSITIVE_COMMON : TECC_NUMERIC_SIGN_NEGATIVE_COMMON;
    norm_num->weight = 0;

    /* Handle zero */
    if (value == 0)
    {
        norm_num->sign = TECC_NUMERIC_SIGN_ZERO_COMMON;
        norm_num->ndigits = 0;
        norm_num->digits = NULL;
        return TECC_NORMALIZATION_SUCCESS;
    }

    /* Convert to absolute value */
    abs_value = (value < 0) ? -(int64)value : (int64)value;

    /* Count digits */
    digit_count = 0;
    temp = abs_value;
    while (temp > 0)
    {
        digit_count++;
        temp /= 10;
    }

    norm_num->ndigits = digit_count;

    /* Allocate digits array */
    packed_count = (digit_count + 3) / 4;
    norm_num->digits = (uint16_t *)pg_malloc(packed_count * sizeof(uint16_t));
    if (!norm_num->digits)
        return TECC_NORMALIZATION_ERROR_MEMORY_ALLOC;

    /* Extract digits */
    temp = abs_value;
    packed_digit = 0;
    packed_index = 0;
    digit_index = 0;

    while (temp > 0)
    {
        int digit = temp % 10;
        temp /= 10;

        /* Pack 4 digits into one uint16 */
        if (digit_index % 4 == 0 && digit_index > 0)
        {
            norm_num->digits[packed_index++] = packed_digit;
            packed_digit = 0;
        }

        packed_digit = (packed_digit << 4) | (digit & 0x0F);
        digit_index++;
    }

    /* Store last packed digit */
    if (digit_index > 0)
    {
        /* Pad with zeros if needed */
        int remaining = digit_index % 4;
        if (remaining > 0)
        {
            packed_digit <<= (4 - remaining) * 4;
        }
        norm_num->digits[packed_index] = packed_digit;
    }

    return TECC_NORMALIZATION_SUCCESS;
}

/*
 * tecc_normalize_int8_common - Normalize int8 to common format
 */
static int
tecc_normalize_int8_common(int64 value,
                           TeccNumericNormalizedCommon *norm_num)
{
    int64 abs_value;
    int digit_count;
    int64 temp;
    size_t packed_count;
    uint16_t packed_digit;
    int packed_index;
    int digit_index;

    if (!norm_num)
        return TECC_NORMALIZATION_ERROR_INVALID_PARAM;

    /* Initialize structure */
    norm_num->sign = (value >= 0) ? TECC_NUMERIC_SIGN_POSITIVE_COMMON : TECC_NUMERIC_SIGN_NEGATIVE_COMMON;
    norm_num->weight = 0;

    /* Handle zero */
    if (value == 0)
    {
        norm_num->sign = TECC_NUMERIC_SIGN_ZERO_COMMON;
        norm_num->ndigits = 0;
        norm_num->digits = NULL;
        return TECC_NORMALIZATION_SUCCESS;
    }

    /* Convert to absolute value */
    abs_value = (value < 0) ? -value : value;

    /* Count digits */
    digit_count = 0;
    temp = abs_value;
    while (temp > 0)
    {
        digit_count++;
        temp /= 10;
    }

    norm_num->ndigits = digit_count;

    /* Allocate digits array */
    packed_count = (digit_count + 3) / 4;
    norm_num->digits = (uint16_t *)pg_malloc(packed_count * sizeof(uint16_t));
    if (!norm_num->digits)
        return TECC_NORMALIZATION_ERROR_MEMORY_ALLOC;

    /* Extract digits */
    temp = abs_value;
    packed_digit = 0;
    packed_index = 0;
    digit_index = 0;

    while (temp > 0)
    {
        int digit = temp % 10;
        temp /= 10;

        /* Pack 4 digits into one uint16 */
        if (digit_index % 4 == 0 && digit_index > 0)
        {
            norm_num->digits[packed_index++] = packed_digit;
            packed_digit = 0;
        }

        packed_digit = (packed_digit << 4) | (digit & 0x0F);
        digit_index++;
    }

    /* Store last packed digit */
    if (digit_index > 0)
    {
        /* Pad with zeros if needed */
        int remaining = digit_index % 4;
        if (remaining > 0)
        {
            packed_digit <<= (4 - remaining) * 4;
        }
        norm_num->digits[packed_index] = packed_digit;
    }

    return TECC_NORMALIZATION_SUCCESS;
}

/*
 * tecc_normalize_float4_common - Normalize float4 to common format
 */
static int
tecc_normalize_float4_common(float value,
                             TeccNumericNormalizedCommon *norm_num)
{
    /* ReqID:TECC4.0 - Use float_to_shortest_decimal_buf (Ryū) instead of "%f".
     * "%f" truncates to 6 decimal places, losing precision for small values
     * (e.g. 1e-7 → "0.000000"). Ryū preserves full float4 precision. */
    char buffer[FLOAT_SHORTEST_DECIMAL_LEN];
    float_to_shortest_decimal_buf(value, buffer);

    return tecc_normalize_numeric_text_common(buffer, strlen(buffer), norm_num);
}

/*
 * tecc_normalize_float8_common - Normalize float8 to common format
 */
static int
tecc_normalize_float8_common(double value,
                             TeccNumericNormalizedCommon *norm_num)
{
    /* ReqID:TECC4.0 - Use double_to_shortest_decimal_buf (Ryū) instead of "%lf".
     * "%lf" truncates to 6 decimal places, same precision issue as float4.
     * Ryū preserves full float8 precision. */
    char buffer[DOUBLE_SHORTEST_DECIMAL_LEN];
    double_to_shortest_decimal_buf(value, buffer);

    return tecc_normalize_numeric_text_common(buffer, strlen(buffer), norm_num);
}

#if 0
/*
 * tecc_normalize_int16_common - Normalize int16 to common format
 */
static int
tecc_normalize_int16_common(int16 value,
                            TeccNumericNormalizedCommon *norm_num)
{
    int64 abs_value;
    int digit_count;
    int64 temp;
    size_t packed_count;
    uint16_t packed_digit;
    int packed_index;
    int digit_index;

    if (!norm_num)
        return TECC_NORMALIZATION_ERROR_INVALID_PARAM;

    /* Initialize structure */
    norm_num->sign = (value >= 0) ? TECC_NUMERIC_SIGN_POSITIVE_COMMON : TECC_NUMERIC_SIGN_NEGATIVE_COMMON;
    norm_num->weight = 0;

    /* Handle zero */
    if (value == 0)
    {
        norm_num->sign = TECC_NUMERIC_SIGN_ZERO_COMMON;
        norm_num->ndigits = 0;
        norm_num->digits = NULL;
        return TECC_NORMALIZATION_SUCCESS;
    }

    /* Convert to absolute value */
    abs_value = (value < 0) ? -(int64)value : (int64)value;

    /* Count digits */
    digit_count = 0;
    temp = abs_value;
    while (temp > 0)
    {
        digit_count++;
        temp /= 10;
    }

    norm_num->ndigits = digit_count;

    /* Allocate digits array */
    packed_count = (digit_count + 3) / 4;
    norm_num->digits = (uint16_t *)pg_malloc(packed_count * sizeof(uint16_t));
    if (!norm_num->digits)
        return TECC_NORMALIZATION_ERROR_MEMORY_ALLOC;

    /* Extract digits */
    temp = abs_value;
    packed_digit = 0;
    packed_index = 0;
    digit_index = 0;

    while (temp > 0)
    {
        int digit = temp % 10;
        temp /= 10;

        /* Pack 4 digits into one uint16 */
        if (digit_index % 4 == 0 && digit_index > 0)
        {
            norm_num->digits[packed_index++] = packed_digit;
            packed_digit = 0;
        }

        packed_digit = (packed_digit << 4) | (digit & 0x0F);
        digit_index++;
    }

    /* Store last packed digit */
    if (digit_index > 0)
    {
        /* Pad with zeros if needed */
        int remaining = digit_index % 4;
        if (remaining > 0)
        {
            packed_digit <<= (4 - remaining) * 4;
        }
        norm_num->digits[packed_index] = packed_digit;
    }

    return TECC_NORMALIZATION_SUCCESS;
}
#endif

#if 0
/*
 * tecc_normalize_int32_common - Normalize int32 to common format
 */
static int
tecc_normalize_int32_common(int32 value,
                            TeccNumericNormalizedCommon *norm_num)
{
    int64 abs_value;
    int digit_count;
    int64 temp;
    size_t packed_count;
    uint16_t packed_digit;
    int packed_index;
    int digit_index;

    if (!norm_num)
        return TECC_NORMALIZATION_ERROR_INVALID_PARAM;

    /* Initialize structure */
    norm_num->sign = (value >= 0) ? TECC_NUMERIC_SIGN_POSITIVE_COMMON : TECC_NUMERIC_SIGN_NEGATIVE_COMMON;
    norm_num->weight = 0;

    /* Handle zero */
    if (value == 0)
    {
        norm_num->sign = TECC_NUMERIC_SIGN_ZERO_COMMON;
        norm_num->ndigits = 0;
        norm_num->digits = NULL;
        return TECC_NORMALIZATION_SUCCESS;
    }

    /* Convert to absolute value */
    abs_value = (value < 0) ? -(int64)value : (int64)value;

    /* Count digits */
    digit_count = 0;
    temp = abs_value;
    while (temp > 0)
    {
        digit_count++;
        temp /= 10;
    }

    norm_num->ndigits = digit_count;

    /* Allocate digits array */
    packed_count = (digit_count + 3) / 4;
    norm_num->digits = (uint16_t *)pg_malloc(packed_count * sizeof(uint16_t));
    if (!norm_num->digits)
        return TECC_NORMALIZATION_ERROR_MEMORY_ALLOC;

    /* Extract digits */
    temp = abs_value;
    packed_digit = 0;
    packed_index = 0;
    digit_index = 0;

    while (temp > 0)
    {
        int digit = temp % 10;
        temp /= 10;

        /* Pack 4 digits into one uint16 */
        if (digit_index % 4 == 0 && digit_index > 0)
        {
            norm_num->digits[packed_index++] = packed_digit;
            packed_digit = 0;
        }

        packed_digit = (packed_digit << 4) | (digit & 0x0F);
        digit_index++;
    }

    /* Store last packed digit */
    if (digit_index > 0)
    {
        /* Pad with zeros if needed */
        int remaining = digit_index % 4;
        if (remaining > 0)
        {
            packed_digit <<= (4 - remaining) * 4;
        }
        norm_num->digits[packed_index] = packed_digit;
    }

    return TECC_NORMALIZATION_SUCCESS;
}
#endif

#if 0
/*
 * tecc_normalize_int64_common - Normalize int64 to common format
 */
static int
tecc_normalize_int64_common(int64 value,
                            TeccNumericNormalizedCommon *norm_num)
{
    int64 abs_value;
    int digit_count;
    int64 temp;
    size_t packed_count;
    uint16_t packed_digit;
    int packed_index;
    int digit_index;

    if (!norm_num)
        return TECC_NORMALIZATION_ERROR_INVALID_PARAM;

    /* Initialize structure */
    norm_num->sign = (value >= 0) ? TECC_NUMERIC_SIGN_POSITIVE_COMMON : TECC_NUMERIC_SIGN_NEGATIVE_COMMON;
    norm_num->weight = 0;

    /* Handle zero */
    if (value == 0)
    {
        norm_num->sign = TECC_NUMERIC_SIGN_ZERO_COMMON;
        norm_num->ndigits = 0;
        norm_num->digits = NULL;
        return TECC_NORMALIZATION_SUCCESS;
    }

    /* Convert to absolute value */
    abs_value = (value < 0) ? -value : value;

    /* Count digits */
    digit_count = 0;
    temp = abs_value;
    while (temp > 0)
    {
        digit_count++;
        temp /= 10;
    }

    norm_num->ndigits = digit_count;

    /* Allocate digits array */
    packed_count = (digit_count + 3) / 4;
    norm_num->digits = (uint16_t *)pg_malloc(packed_count * sizeof(uint16_t));
    if (!norm_num->digits)
        return TECC_NORMALIZATION_ERROR_MEMORY_ALLOC;

    /* Extract digits */
    temp = abs_value;
    packed_digit = 0;
    packed_index = 0;
    digit_index = 0;

    while (temp > 0)
    {
        int digit = temp % 10;
        temp /= 10;

        /* Pack 4 digits into one uint16 */
        if (digit_index % 4 == 0 && digit_index > 0)
        {
            norm_num->digits[packed_index++] = packed_digit;
            packed_digit = 0;
        }

        packed_digit = (packed_digit << 4) | (digit & 0x0F);
        digit_index++;
    }

    /* Store last packed digit */
    if (digit_index > 0)
    {
        /* Pad with zeros if needed */
        int remaining = digit_index % 4;
        if (remaining > 0)
        {
            packed_digit <<= (4 - remaining) * 4;
        }
        norm_num->digits[packed_index] = packed_digit;
    }

    return TECC_NORMALIZATION_SUCCESS;
}
#endif

/*
 * tecc_normalize_text_with_icu_common - Normalize text with ICU sort key generation
 */
int
tecc_normalize_text_with_icu_common(const char *collation_locale,
                                    const char *text,
                                    size_t text_len,
                                    unsigned char **output_data,
                                    size_t *output_len)
{
    TeccFormatterErrorCode error_code;

    if (!collation_locale || !text || !output_data || !output_len)
        return TECC_NORMALIZATION_ERROR_INVALID_PARAM;

    if (text_len == 0)
        return TECC_NORMALIZATION_ERROR_INVALID_PARAM;

    /* Use ICU normalizer to generate sort key */
    error_code = tecc_icu_normalizer_generate_sortkey(collation_locale,
                                                     (const unsigned char *)text,
                                                     text_len,
                                                     output_data,
                                                     output_len);

    /* Map TeccFormatterErrorCode to normalization error code */
    switch (error_code) {
        case TECC_FORMATTER_SUCCESS:
            return TECC_NORMALIZATION_SUCCESS;
        case TECC_FORMATTER_ERROR_INVALID_PARAM:
            return TECC_NORMALIZATION_ERROR_INVALID_PARAM;
        case TECC_FORMATTER_ERROR_MEMORY_ALLOC:
            return TECC_NORMALIZATION_ERROR_MEMORY_ALLOC;
        case TECC_FORMATTER_ERROR_ICU_NOT_AVAILABLE:
            return TECC_NORMALIZATION_ERROR_ICU_NOT_AVAILABLE;
        case TECC_FORMATTER_ERROR_COLLATION_INVALID:
            return TECC_NORMALIZATION_ERROR_COLLATION_INVALID;
        case TECC_FORMATTER_ERROR_ENCODING_FAILED:
            return TECC_NORMALIZATION_ERROR_ENCODING_FAILED;
        case TECC_FORMATTER_ERROR_SORTKEY_FAILED:
            return TECC_NORMALIZATION_ERROR_SORTKEY_FAILED;
        default:
            return TECC_NORMALIZATION_ERROR_INVALID_PARAM;
    }
}

/*
 * tecc_normalize_text_with_icu_to_buffer_common - Normalize text with ICU sort key to existing buffer
 */
int
tecc_normalize_text_with_icu_to_buffer_common(const char *collation_locale,
                                              const char *text,
                                              size_t text_len,
                                              unsigned char *buffer,
                                              size_t buffer_size,
                                              size_t *output_len)
{
    TeccFormatterErrorCode error_code;
    unsigned char *sortkey = NULL;
    size_t sortkey_len = 0;

    if (!collation_locale || !text || !buffer || !output_len)
        return TECC_NORMALIZATION_ERROR_INVALID_PARAM;

    if (text_len == 0)
        return TECC_NORMALIZATION_ERROR_INVALID_PARAM;

    /* Use ICU normalizer to generate sort key */
    error_code = tecc_icu_normalizer_generate_sortkey(collation_locale,
                                                     (const unsigned char *)text,
                                                     text_len,
                                                     &sortkey,
                                                     &sortkey_len);

    if (error_code != TECC_FORMATTER_SUCCESS) {
        /* Map TeccFormatterErrorCode to normalization error code */
        switch (error_code) {
            case TECC_FORMATTER_ERROR_INVALID_PARAM:
                return TECC_NORMALIZATION_ERROR_INVALID_PARAM;
            case TECC_FORMATTER_ERROR_MEMORY_ALLOC:
                return TECC_NORMALIZATION_ERROR_MEMORY_ALLOC;
            case TECC_FORMATTER_ERROR_ICU_NOT_AVAILABLE:
                return TECC_NORMALIZATION_ERROR_ICU_NOT_AVAILABLE;
            case TECC_FORMATTER_ERROR_COLLATION_INVALID:
                return TECC_NORMALIZATION_ERROR_COLLATION_INVALID;
            case TECC_FORMATTER_ERROR_ENCODING_FAILED:
                return TECC_NORMALIZATION_ERROR_ENCODING_FAILED;
            case TECC_FORMATTER_ERROR_SORTKEY_FAILED:
                return TECC_NORMALIZATION_ERROR_SORTKEY_FAILED;
            default:
                return TECC_NORMALIZATION_ERROR_INVALID_PARAM;
        }
    }

    /* Check if buffer is large enough */
    if (buffer_size < sortkey_len) {
        pg_free(sortkey);
        return TECC_NORMALIZATION_ERROR_BUFFER_TOO_SMALL;
    }

    /* Copy sort key to provided buffer */
    memcpy(buffer, sortkey, sortkey_len);
    *output_len = sortkey_len;

    /* Free the temporary sort key */
    pg_free(sortkey);

    return TECC_NORMALIZATION_SUCCESS;
}

/*
 * tecc_normalize_text_simple_common - Normalize text without ICU (simple length prefix)
 */
int
tecc_normalize_text_simple_common(const char *text,
                                  size_t text_len,
                                  unsigned char **output_data,
                                  size_t *output_len)
{
    size_t total_size;
    unsigned char *buffer;

    if (!text || !output_data || !output_len)
        return TECC_NORMALIZATION_ERROR_INVALID_PARAM;

    /* Calculate total size: 4-byte length prefix + text */
    total_size = 4 + text_len;

    /* Allocate output buffer */
    buffer = (unsigned char *)pg_malloc(total_size);
    if (!buffer)
        return TECC_NORMALIZATION_ERROR_MEMORY_ALLOC;

    /* Encode length prefix (big-endian) */
    buffer[0] = (text_len >> 24) & 0xFF;
    buffer[1] = (text_len >> 16) & 0xFF;
    buffer[2] = (text_len >> 8) & 0xFF;
    buffer[3] = text_len & 0xFF;

    /* Copy text */
    memcpy(buffer + 4, text, text_len);

    *output_data = buffer;
    *output_len = total_size;

    return TECC_NORMALIZATION_SUCCESS;
}

/*
 * tecc_normalize_text_simple_to_buffer_common - Normalize text without ICU to existing buffer
 */
int
tecc_normalize_text_simple_to_buffer_common(const char *text,
                                            size_t text_len,
                                            unsigned char *buffer,
                                            size_t buffer_size,
                                            size_t *output_len)
{
    size_t total_size;

    if (!text || !buffer || !output_len)
        return TECC_NORMALIZATION_ERROR_INVALID_PARAM;

    /* Calculate total size: 4-byte length prefix + text */
    total_size = 4 + text_len;

    /* Check if buffer is large enough */
    if (buffer_size < total_size)
        return TECC_NORMALIZATION_ERROR_MEMORY_ALLOC;

    /* Encode length prefix (big-endian) */
    buffer[0] = (text_len >> 24) & 0xFF;
    buffer[1] = (text_len >> 16) & 0xFF;
    buffer[2] = (text_len >> 8) & 0xFF;
    buffer[3] = text_len & 0xFF;

    /* Copy text */
    memcpy(buffer + 4, text, text_len);

    *output_len = total_size;
    return TECC_NORMALIZATION_SUCCESS;
}

/*
 * tecc_normalize_int2_text_common - Normalize int2 text string to common format
 */
int
tecc_normalize_int2_text_common(const char *int2_text,
                                size_t text_len,
                                TeccNumericNormalizedCommon *norm_num)
{
    int16 value;
    char *endptr;
    long long_val;

    if (!int2_text || !norm_num)
        return TECC_NORMALIZATION_ERROR_INVALID_PARAM;

    /* Parse int2 text */
    errno = 0;
    long_val = strtol(int2_text, &endptr, 10);

    /* Check for conversion errors */
    if (errno == ERANGE || long_val < INT16_MIN || long_val > INT16_MAX)
        return TECC_NORMALIZATION_ERROR_NUMERIC_OVERFLOW;

    if (endptr == int2_text)  /* No digits converted */
        return TECC_NORMALIZATION_ERROR_INVALID_TYPE;

    /* Skip trailing whitespace */
    while (*endptr && isspace((unsigned char)*endptr))
        endptr++;

    /* Check for extra characters */
    if (*endptr != '\0')
        return TECC_NORMALIZATION_ERROR_INVALID_TYPE;

    value = (int16)long_val;

    /* Use existing binary normalization function */
    return tecc_normalize_int2_common(value, norm_num);
}

/*
 * tecc_normalize_int4_text_common - Normalize int4 text string to common format
 */
int
tecc_normalize_int4_text_common(const char *int4_text,
                                size_t text_len,
                                TeccNumericNormalizedCommon *norm_num)
{
    int32 value;
    char *endptr;
    long long_val;

    if (!int4_text || !norm_num)
        return TECC_NORMALIZATION_ERROR_INVALID_PARAM;

    /* Parse int4 text */
    errno = 0;
    long_val = strtol(int4_text, &endptr, 10);

    /* Check for conversion errors */
    if (errno == ERANGE || long_val < INT32_MIN || long_val > INT32_MAX)
        return TECC_NORMALIZATION_ERROR_NUMERIC_OVERFLOW;

    if (endptr == int4_text)  /* No digits converted */
        return TECC_NORMALIZATION_ERROR_INVALID_TYPE;

    /* Skip trailing whitespace */
    while (*endptr && isspace((unsigned char)*endptr))
        endptr++;

    /* Check for extra characters */
    if (*endptr != '\0')
        return TECC_NORMALIZATION_ERROR_INVALID_TYPE;

    value = (int32)long_val;

    /* Use existing binary normalization function */
    return tecc_normalize_int4_common(value, norm_num);
}

/*
 * tecc_normalize_int8_text_common - Normalize int8 text string to common format
 */
int
tecc_normalize_int8_text_common(const char *int8_text,
                                size_t text_len,
                                TeccNumericNormalizedCommon *norm_num)
{
    int64 value;
    char *endptr;
    long long long_val;

    if (!int8_text || !norm_num)
        return TECC_NORMALIZATION_ERROR_INVALID_PARAM;

    /* Parse int8 text */
    errno = 0;
    long_val = strtoll(int8_text, &endptr, 10);

    /* Check for conversion errors */
    if (errno == ERANGE)
        return TECC_NORMALIZATION_ERROR_NUMERIC_OVERFLOW;

    if (endptr == int8_text)  /* No digits converted */
        return TECC_NORMALIZATION_ERROR_INVALID_TYPE;

    /* Skip trailing whitespace */
    while (*endptr && isspace((unsigned char)*endptr))
        endptr++;

    /* Check for extra characters */
    if (*endptr != '\0')
        return TECC_NORMALIZATION_ERROR_INVALID_TYPE;

    value = (int64)long_val;

    /* Use existing binary normalization function */
    return tecc_normalize_int8_common(value, norm_num);
}

/*
 * tecc_normalize_float4_text_common - Normalize float4 text string to common format
 */
int
tecc_normalize_float4_text_common(const char *float4_text,
                                  size_t text_len,
                                  TeccNumericNormalizedCommon *norm_num)
{
    float4 value;
    char *endptr;
    double double_val;

    if (!float4_text || !norm_num)
        return TECC_NORMALIZATION_ERROR_INVALID_PARAM;

    /* Parse float4 text */
    errno = 0;
    double_val = strtod(float4_text, &endptr);

    /* ReqID:TECC4.0 - NaN/Infinity detection and ERANGE handling
     * matching PG backend float4in logic. */

    /* If strtod parsed nothing, check for NaN/Infinity explicitly */
    if (endptr == float4_text)
    {
        if (pg_strncasecmp(float4_text, "NaN", 3) == 0 ||
            pg_strncasecmp(float4_text, "nan", 3) == 0)
        {
            norm_num->sign = TECC_NUMERIC_SIGN_NAN;
            norm_num->weight = 0;
            norm_num->ndigits = 0;
            norm_num->digits = NULL;
            return TECC_NORMALIZATION_SUCCESS;
        }
        /* Check for optional sign + Infinity/Inf */
        {
            const char *p = float4_text;
            int sign = 1;
            if (*p == '-') { sign = -1; p++; }
            else if (*p == '+') { p++; }
            if (pg_strncasecmp(p, "Infinity", 8) == 0 ||
                pg_strncasecmp(p, "Inf", 3) == 0)
            {
                norm_num->sign = (sign < 0) ? TECC_NUMERIC_SIGN_NINF
                                            : TECC_NUMERIC_SIGN_INF;
                norm_num->weight = 0;
                norm_num->ndigits = 0;
                norm_num->digits = NULL;
                return TECC_NORMALIZATION_SUCCESS;
            }
        }
        return TECC_NORMALIZATION_ERROR_INVALID_TYPE;
    }

    /* ReqID:TECC4.0 - ERANGE: accept gradual underflow (value→0),
     * reject true overflow (isinf). Matches PG float4in behavior. */
    if (errno == ERANGE)
    {
        if (double_val == 0.0)
        {
            /* Gradual underflow — accept */
        }
        else if (isinf(double_val))
        {
            return TECC_NORMALIZATION_ERROR_NUMERIC_OVERFLOW;
        }
        else
        {
            return TECC_NORMALIZATION_ERROR_NUMERIC_OVERFLOW;
        }
    }

    /* If strtod returned NaN/Infinity without ERANGE, this is an explicit
     * special value input — handle it directly rather than relying on
     * tecc_normalize_float4_common's "%f" format. */
    if (isnan(double_val))
    {
        norm_num->sign = TECC_NUMERIC_SIGN_NAN;
        norm_num->weight = 0;
        norm_num->ndigits = 0;
        norm_num->digits = NULL;
        return TECC_NORMALIZATION_SUCCESS;
    }
    if (isinf(double_val))
    {
        norm_num->sign = (double_val < 0) ? TECC_NUMERIC_SIGN_NINF
                                          : TECC_NUMERIC_SIGN_INF;
        norm_num->weight = 0;
        norm_num->ndigits = 0;
        norm_num->digits = NULL;
        return TECC_NORMALIZATION_SUCCESS;
    }

    /* Skip trailing whitespace */
    while (*endptr && isspace((unsigned char)*endptr))
        endptr++;

    /* Check for extra characters */
    if (*endptr != '\0')
        return TECC_NORMALIZATION_ERROR_INVALID_TYPE;

    /* Check for overflow/underflow */
    if (double_val > FLT_MAX || double_val < -FLT_MAX)
        return TECC_NORMALIZATION_ERROR_NUMERIC_OVERFLOW;

    value = (float4)double_val;

    /* Use existing binary normalization function */
    return tecc_normalize_float4_common(value, norm_num);
}

/*
 * tecc_normalize_float8_text_common - Normalize float8 text string to common format
 */
int
tecc_normalize_float8_text_common(const char *float8_text,
                                  size_t text_len,
                                  TeccNumericNormalizedCommon *norm_num)
{
    float8 value;
    char *endptr;
    double double_val;

    if (!float8_text || !norm_num)
        return TECC_NORMALIZATION_ERROR_INVALID_PARAM;

    /* Parse float8 text */
    errno = 0;
    double_val = strtod(float8_text, &endptr);

    /* ReqID:TECC4.0 - NaN/Infinity detection and ERANGE handling
     * matching PG backend float8in logic. */
    if (endptr == float8_text)
    {
        if (pg_strncasecmp(float8_text, "NaN", 3) == 0 ||
            pg_strncasecmp(float8_text, "nan", 3) == 0)
        {
            norm_num->sign = TECC_NUMERIC_SIGN_NAN;
            norm_num->weight = 0;
            norm_num->ndigits = 0;
            norm_num->digits = NULL;
            return TECC_NORMALIZATION_SUCCESS;
        }
        {
            const char *p = float8_text;
            int sign = 1;
            if (*p == '-') { sign = -1; p++; }
            else if (*p == '+') { p++; }
            if (pg_strncasecmp(p, "Infinity", 8) == 0 ||
                pg_strncasecmp(p, "Inf", 3) == 0)
            {
                norm_num->sign = (sign < 0) ? TECC_NUMERIC_SIGN_NINF
                                            : TECC_NUMERIC_SIGN_INF;
                norm_num->weight = 0;
                norm_num->ndigits = 0;
                norm_num->digits = NULL;
                return TECC_NORMALIZATION_SUCCESS;
            }
        }
        return TECC_NORMALIZATION_ERROR_INVALID_TYPE;
    }

    if (errno == ERANGE)
    {
        if (double_val == 0.0)
        {
            /* Gradual underflow — accept */
        }
        else if (isinf(double_val))
        {
            return TECC_NORMALIZATION_ERROR_NUMERIC_OVERFLOW;
        }
        else
        {
            return TECC_NORMALIZATION_ERROR_NUMERIC_OVERFLOW;
        }
    }

    if (isnan(double_val))
    {
        norm_num->sign = TECC_NUMERIC_SIGN_NAN;
        norm_num->weight = 0;
        norm_num->ndigits = 0;
        norm_num->digits = NULL;
        return TECC_NORMALIZATION_SUCCESS;
    }
    if (isinf(double_val))
    {
        norm_num->sign = (double_val < 0) ? TECC_NUMERIC_SIGN_NINF
                                          : TECC_NUMERIC_SIGN_INF;
        norm_num->weight = 0;
        norm_num->ndigits = 0;
        norm_num->digits = NULL;
        return TECC_NORMALIZATION_SUCCESS;
    }

    /* Skip trailing whitespace */
    while (*endptr && isspace((unsigned char)*endptr))
        endptr++;

    /* Check for extra characters */
    if (*endptr != '\0')
        return TECC_NORMALIZATION_ERROR_INVALID_TYPE;

    value = (float8)double_val;

    /* Use existing binary normalization function */
    return tecc_normalize_float8_common(value, norm_num);
}

/* End - ReqID:TECC4.0 */