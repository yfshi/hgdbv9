/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_formatter_utils.c
 *
 * Abstract:
 *   Utility implementation for TECC data formatter module.
 *   Provides common utility functions and helpers.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_formatter_utils.c
 */

#include "tecc_formatter_utils.h"
#include "common/fe_memutils.h"  /* For pg_malloc, pg_free */

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>

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
 * tecc_formatter_utils_malloc - Safe memory allocation wrapper
 */
void *
tecc_formatter_utils_malloc(size_t size)
{
    if (size == 0)
        return NULL;

    return pg_malloc(size);
}

/*
 * tecc_formatter_utils_calloc - Safe memory allocation with zeroing
 */
void *
tecc_formatter_utils_calloc(size_t count, size_t size)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    size_t total_size;

    if (count == 0 || size == 0)
        return NULL;

    total_size = count * size;
    if (total_size / count != size)  /* Check for overflow */
        return NULL;

    return pg_malloc0(total_size);
}

/*
 * tecc_formatter_utils_realloc - Safe memory reallocation
 */
void *
tecc_formatter_utils_realloc(void *ptr, size_t size)
{
    /* For now, use simple allocate-copy-free pattern */
    if (ptr == NULL)
        return tecc_formatter_utils_malloc(size);

    if (size == 0) {
        pg_free(ptr);
        return NULL;
    }

    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    {
        void *new_ptr = pg_malloc(size);
    if (!new_ptr)
        return NULL;

    /* Copy old data (size unknown, so we can't copy) */
    /* TODO: Implement proper reallocation with size tracking */

        pg_free(ptr);
        return new_ptr;
    }
}

/*
 * tecc_formatter_utils_strdup - Safe string duplication
 */
char *
tecc_formatter_utils_strdup(const char *str)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    size_t len;
    char *copy;

    if (!str)
        return NULL;

    len = strlen(str) + 1;
    copy = pg_malloc(len);
    if (!copy)
        return NULL;

    memcpy(copy, str, len);
    return copy;
}

/*
 * tecc_formatter_utils_free - Safe memory deallocation
 */
void
tecc_formatter_utils_free(void *ptr)
{
    if (ptr)
        pg_free(ptr);
}

/*
 * tecc_formatter_utils_strlcpy - Safe string copy
 */
size_t
tecc_formatter_utils_strlcpy(char *dst, const char *src, size_t size)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    size_t src_len;

    if (!dst || !src || size == 0)
        return 0;

    src_len = strlen(src);
    if (src_len >= size) {
        memcpy(dst, src, size - 1);
        dst[size - 1] = '\0';
        return size - 1;
    }

    memcpy(dst, src, src_len + 1);
    return src_len;
}

/*
 * tecc_formatter_utils_strlcat - Safe string concatenation
 */
size_t
tecc_formatter_utils_strlcat(char *dst, const char *src, size_t size)
{
    size_t dst_len;
    size_t src_len;
    size_t copy_len;

    if (!dst || !src || size == 0)
        return 0;

    dst_len = strlen(dst);
    if (dst_len >= size - 1)
        return dst_len;

    src_len = strlen(src);
    copy_len = TECC_MIN(src_len, size - dst_len - 1);

    memcpy(dst + dst_len, src, copy_len);
    dst[dst_len + copy_len] = '\0';

    return dst_len + copy_len;
}

/*
 * tecc_formatter_utils_strcasecmp - Case-insensitive string comparison
 */
int
tecc_formatter_utils_strcasecmp(const char *s1, const char *s2)
{
    if (!s1 && !s2)
        return 0;
    if (!s1)
        return -1;
    if (!s2)
        return 1;

    while (*s1 && *s2) {
        int c1 = tolower((unsigned char)*s1);
        int c2 = tolower((unsigned char)*s2);
        if (c1 != c2)
            return c1 - c2;
        s1++;
        s2++;
    }

    return (unsigned char)*s1 - (unsigned char)*s2;
}

/*
 * tecc_formatter_utils_strncasecmp - Case-insensitive string comparison (limited)
 */
int
tecc_formatter_utils_strncasecmp(const char *s1, const char *s2, size_t n)
{
    if (!s1 && !s2)
        return 0;
    if (!s1)
        return -1;
    if (!s2)
        return 1;

    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    {
        size_t i = 0;
    while (i < n && *s1 && *s2) {
        int c1 = tolower((unsigned char)*s1);
        int c2 = tolower((unsigned char)*s2);
        if (c1 != c2)
            return c1 - c2;
        s1++;
        s2++;
        i++;
    }

        if (i == n)
            return 0;

        return (unsigned char)*s1 - (unsigned char)*s2;
    }
}

/*
 * tecc_formatter_utils_is_valid_oid - Check if OID is valid
 */
bool
tecc_formatter_utils_is_valid_oid(Oid oid)
{
    /* PostgreSQL OIDs are unsigned 32-bit integers */
    /* Invalid OIDs are typically 0 or very large values */
    return (oid > 0 && oid < UINT32_MAX);
}

/*
 * tecc_formatter_utils_is_valid_algorithm - Check if algorithm type is valid
 */
bool
tecc_formatter_utils_is_valid_algorithm(TeccAlgorithmType algo)
{
    return (algo > 0 && algo < TECC_ALGO_MAX);
}

/*
 * tecc_formatter_utils_is_valid_data_type - Check if data type is supported
 */
bool
tecc_formatter_utils_is_valid_data_type(Oid data_type)
{
    switch (data_type) {
        case INT2OID:
        case INT4OID:
        case INT8OID:
        case FLOAT4OID:
        case FLOAT8OID:
        case NUMERICOID:
        case VARCHAROID:
        case BPCHAROID:
        case TEXTOID:
            return true;

        default:
            return false;
    }
}

/*
 * tecc_formatter_utils_align_pointer - Align pointer to boundary
 */
void *
tecc_formatter_utils_align_pointer(void *ptr, size_t alignment)
{
    uintptr_t addr;
    uintptr_t aligned;

    if (!ptr || alignment == 0)
        return ptr;

    addr = (uintptr_t)ptr;
    aligned = (addr + alignment - 1) & ~(alignment - 1);

    return (void *)aligned;
}

/*
 * tecc_formatter_utils_align_size - Align size to boundary
 */
size_t
tecc_formatter_utils_align_size(size_t size, size_t alignment)
{
    if (alignment == 0)
        return size;

    return (size + alignment - 1) & ~(alignment - 1);
}

/*
 * tecc_formatter_utils_set_error - Set error in context
 */
void
tecc_formatter_utils_set_error(TeccCryptoContextCore *context,
                               TeccFormatterErrorCode error_code,
                               const char *format, ...)
{
    va_list args;

    if (!context || !format)
        return;

    /* Note: Simplified context doesn't have error_code/error_msg fields
     * In production, these would need to be added to the structure */

    va_start(args, format);
    /* Would set error message if field existed: vsnprintf(context->error_msg, sizeof(context->error_msg), format, args); */
    va_end(args);
}

/*
 * tecc_formatter_utils_error_to_string - Convert error code to string
 */
const char *
tecc_formatter_utils_error_to_string(TeccFormatterErrorCode error)
{
    switch (error) {
        case TECC_FORMATTER_SUCCESS:
            return "Success";
        case TECC_FORMATTER_ERROR_INVALID_PARAM:
            return "Invalid parameter";
        case TECC_FORMATTER_ERROR_NULL_POINTER:
            return "Null pointer";
        case TECC_FORMATTER_ERROR_INVALID_TYPE:
            return "Invalid type";
        case TECC_FORMATTER_ERROR_MEMORY_ALLOC:
            return "Memory allocation failed";
        case TECC_FORMATTER_ERROR_BUFFER_TOO_SMALL:
            return "Buffer too small";
        case TECC_FORMATTER_ERROR_DATA_TOO_LARGE:
            return "Data too large";
        case TECC_FORMATTER_ERROR_DATA_FORMAT:
            return "Invalid data format";
        case TECC_FORMATTER_ERROR_ENCODING_FAILED:
            return "Encoding failed";
        case TECC_FORMATTER_ERROR_DECODING_FAILED:
            return "Decoding failed";
        case TECC_FORMATTER_ERROR_INVALID_HEADER:
            return "Invalid header";
        case TECC_FORMATTER_ERROR_VERSION_MISMATCH:
            return "Version mismatch";
        case TECC_FORMATTER_ERROR_BUILD_FAILED:
            return "Build failed";
        case TECC_FORMATTER_ERROR_ICU_NOT_AVAILABLE:
            return "ICU not available";
        case TECC_FORMATTER_ERROR_COLLATION_INVALID:
            return "Invalid collation";
        case TECC_FORMATTER_ERROR_SORTKEY_FAILED:
            return "Sort key generation failed";
        case TECC_FORMATTER_ERROR_SYSTEM:
            return "System error";
        default:
            return "Unknown error";
    }
}

/* Begin - ReqID:TECC4.0 */
/*
 * tecc_formatter_extract_numeric_sign - Extract sign from numeric datum
 *
 * DEPRECATED: This function uses Datum type which is a backend type.
 * Frontend code should use tecc_formatter_parse_numeric_text instead.
 * Kept for backward compatibility.
 *
 * Extract sign information from PostgreSQL numeric datum.
 * Parses PostgreSQL numeric internal format to extract sign.
 *
 * @param numeric_datum Numeric datum
 * @param sign Output sign value
 * @return Error code
 */
/* Function removed - frontend should not use Datum type */

/*
 * tecc_formatter_extract_numeric_weight - Extract weight from numeric datum
 *
 * DEPRECATED: This function uses Datum type which is a backend type.
 * Frontend code should use tecc_formatter_parse_numeric_text instead.
 * Kept for backward compatibility.
 *
 * Extract weight (decimal point position) from PostgreSQL numeric datum.
 * Parses PostgreSQL numeric internal format to extract weight.
 *
 * @param numeric_datum Numeric datum
 * @param weight Output weight value
 * @return Error code
 */
/* Function removed - frontend should not use Datum type */

/*
 * tecc_formatter_extract_numeric_digits - Extract digits from numeric datum
 *
 * DEPRECATED: This function uses Datum type which is a backend type.
 * Frontend code should use tecc_formatter_parse_numeric_text instead.
 * Kept for backward compatibility.
 *
 * Extract decimal digits from PostgreSQL numeric datum.
 * Parses PostgreSQL numeric internal format to extract digits.
 * Converts NBASE=10000 digits to decimal digits (4 decimal digits per NBASE digit).
 *
 * @param numeric_datum Numeric datum
 * @param digits Output digit array (packed, 4 decimal digits per uint16)
 * @param ndigits Output number of decimal digits
 * @return Error code
 */
/* Function removed - frontend should not use Datum type */


/*
 * tecc_formatter_encode_numeric_normalized - Encode normalized numeric structure to byte stream
 *
 * Convert TeccNumericNormalized structure to OPE normalized byte format:
 * [sign:1字节][weight:2字节大端序][ndigits:2字节大端序][digits:变长]
 *
 * @param norm_num Normalized numeric structure
 * @param output_data Output byte stream
 * @param output_len Output length
 * @return Error code
 */
TeccFormatterErrorCode
tecc_formatter_encode_numeric_normalized(TeccNumericNormalized *norm_num,
                                         unsigned char **output_data,
                                         size_t *output_len)
{
    size_t packed_digits_count;
    size_t total_size;
    unsigned char *buffer;

    if (!norm_num || !output_data || !output_len)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    /* Calculate required size: header + packed digits */
    packed_digits_count = (norm_num->ndigits + 3) / 4;  /* 4 digits per uint16 */
    total_size = TECC_NUMERIC_HEADER_SIZE + packed_digits_count * sizeof(uint16_t);

    /* Allocate output buffer */
    buffer = pg_malloc(total_size);
    if (!buffer)
        return TECC_FORMATTER_ERROR_MEMORY_ALLOC;

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
        memcpy(buffer + TECC_NUMERIC_HEADER_SIZE, norm_num->digits,
               packed_digits_count * sizeof(uint16_t));
    }

    *output_data = buffer;
    *output_len = total_size;

    return TECC_FORMATTER_SUCCESS;
}

/*
 * tecc_formatter_validate_numeric_range - Validate numeric range for OPE
 *
 * Validate that weight and digit count are within acceptable ranges for OPE.
 *
 * @param weight Numeric weight
 * @param ndigits Number of decimal digits
 * @return Error code
 */
TeccFormatterErrorCode
tecc_formatter_validate_numeric_range(int16_t weight, int ndigits)
{
    if (weight < TECC_NUMERIC_MIN_WEIGHT || weight > TECC_NUMERIC_MAX_WEIGHT)
        return TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW;

    if (ndigits < 0 || ndigits > TECC_NUMERIC_MAX_DIGITS)
        return TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW;

    return TECC_FORMATTER_SUCCESS;
}

/* Begin - ReqID:TECC4.0 */
/*
 * tecc_formatter_encode_numeric_normalized_to_buffer - Encode normalized numeric structure to existing buffer
 *
 * Convert TeccNumericNormalized structure to OPE normalized byte format,
 * writing directly to provided buffer.
 *
 * @param norm_num Normalized numeric structure
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @param output_len Actual output length
 * @return Error code
 */
TeccFormatterErrorCode
tecc_formatter_encode_numeric_normalized_to_buffer(TeccNumericNormalized *norm_num,
                                                   unsigned char *buffer,
                                                   size_t buffer_size,
                                                   size_t *output_len)
{
    size_t packed_digits_count;
    size_t total_size;

    if (!norm_num || !buffer || !output_len)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    /* Calculate required size: header + packed digits */
    packed_digits_count = (norm_num->ndigits + 3) / 4;  /* 4 digits per uint16 */
    total_size = TECC_NUMERIC_HEADER_SIZE + packed_digits_count * sizeof(uint16_t);

    /* Check if buffer is large enough */
    if (buffer_size < total_size)
        return TECC_FORMATTER_ERROR_BUFFER_TOO_SMALL;

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
        memcpy(buffer + TECC_NUMERIC_HEADER_SIZE, norm_num->digits,
               packed_digits_count * sizeof(uint16_t));
    }

    *output_len = total_size;
    return TECC_FORMATTER_SUCCESS;
}

/*
 * tecc_formatter_parse_numeric_text - Parse numeric text string to normalized components
 *
 * Parse numeric text string (e.g., "123.45", "-67.89", "NaN", "Infinity")
 * and extract sign, weight, and digits for normalization.
 * This function implements text parsing similar to PostgreSQL backend numeric_in() function.
 *
 * @param numeric_text Numeric text string
 * @param text_len Text length
 * @param sign Output sign value
 * @param weight Output weight (decimal point position)
 * @param digits Output digit array (packed, 4 decimal digits per uint16)
 * @param ndigits Output number of decimal digits
 * @return Error code
 */
TeccFormatterErrorCode
tecc_formatter_parse_numeric_text(const char *numeric_text,
                                  size_t text_len,
                                  uint8_t *sign,
                                  int16_t *weight,
                                  uint16_t *digits,
                                  int *ndigits)
{
    const char *str = numeric_text;
    size_t len = text_len;
    size_t i = 0;
    bool negative = false;
    bool has_dot = false;
    bool has_exponent = false;
    int decimal_point_pos = -1;
    int digit_count = 0;
    int digit_buffer[TECC_NUMERIC_MAX_DIGITS];
    int exponent = 0;
    int packed_idx;
    int calculated_weight = 0;
    int leading_zeros = 0;

    if (!str || len == 0 || !sign || !weight || !digits || !ndigits)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    /* Skip leading whitespace */
    while (i < len && isspace((unsigned char)str[i]))
        i++;

    if (i >= len)
        return TECC_FORMATTER_ERROR_DATA_FORMAT;

    /* Check for special values: NaN, Infinity, Inf */
    if (len - i >= 3) {
        const char *remaining = str + i;
        size_t remaining_len = len - i;

        /* Case-insensitive comparison for NaN */
        if (remaining_len >= 3 &&
            (pg_strncasecmp(remaining, "NaN", 3) == 0 ||
             pg_strncasecmp(remaining, "nan", 3) == 0)) {
            *sign = TECC_NUMERIC_SIGN_NAN;
            *weight = 0;
            *ndigits = 0;
            return TECC_FORMATTER_SUCCESS;
        }

        /* Case-insensitive comparison for Infinity/Inf */
        if (remaining_len >= 3 &&
            (pg_strncasecmp(remaining, "Inf", 3) == 0 ||
             pg_strncasecmp(remaining, "inf", 3) == 0)) {
            /* Check sign */
            if (i > 0 && str[i-1] == '-') {
                *sign = TECC_NUMERIC_SIGN_NINF;
            } else {
                *sign = TECC_NUMERIC_SIGN_INF;
            }
            *weight = 0;
            *ndigits = 0;
            return TECC_FORMATTER_SUCCESS;
        }
    }

    /* Check for sign */
    if (str[i] == '+' || str[i] == '-') {
        negative = (str[i] == '-');
        i++;
        if (i >= len)
            return TECC_FORMATTER_ERROR_DATA_FORMAT;
    }

    /* Parse integer and fractional parts */
    while (i < len) {
        char c = str[i];

        if (isdigit((unsigned char)c)) {
            if (digit_count >= TECC_NUMERIC_MAX_DIGITS)
                return TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW;

            digit_buffer[digit_count++] = c - '0';

            /* Track decimal point position */
            if (has_dot && decimal_point_pos < 0)
                decimal_point_pos = digit_count - 1;
        }
        else if (c == '.') {
            if (has_dot || has_exponent)
                return TECC_FORMATTER_ERROR_DATA_FORMAT;
            has_dot = true;
        }
        else if (c == 'e' || c == 'E') {
            bool exp_negative;
            int exp_value;
            bool exp_has_digit;

            if (has_exponent)
                return TECC_FORMATTER_ERROR_DATA_FORMAT;
            has_exponent = true;
            i++;

            /* Parse exponent sign */
            exp_negative = false;
            if (i < len && (str[i] == '+' || str[i] == '-')) {
                exp_negative = (str[i] == '-');
                i++;
            }

            /* Parse exponent digits */
            exp_value = 0;
            exp_has_digit = false;
            while (i < len && isdigit((unsigned char)str[i])) {
                exp_value = exp_value * 10 + (str[i] - '0');
                exp_has_digit = true;
                i++;
            }

            if (!exp_has_digit)
                return TECC_FORMATTER_ERROR_DATA_FORMAT;

            exponent = exp_negative ? -exp_value : exp_value;
            break;  /* Exponent is the last part */
        }
        else if (isspace((unsigned char)c)) {
            /* Skip trailing whitespace */
            i++;
            while (i < len && isspace((unsigned char)str[i]))
                i++;
            if (i < len)
                return TECC_FORMATTER_ERROR_DATA_FORMAT;
            break;
        }
        else {
            return TECC_FORMATTER_ERROR_DATA_FORMAT;
        }

        i++;
    }

    /* Check if we have any digits */
    if (digit_count == 0)
        return TECC_FORMATTER_ERROR_DATA_FORMAT;

    /* Modify - ReqID:TECC4.0: Keep trailing zeros to preserve scale information.
     * Previously stripped trailing zeros here, losing scale info (1.0→ndigits=1),
     * causing decode to output "1" instead of "1.0". Must match PostgreSQL
     * backend behavior: 1.0::numeric → "1.0", 1.00::numeric → "1.00".
     * Zero detection: check if ALL digits are zero (including trailing zeros). */
    fprintf(stderr, "PARSE_DEBUG: digit_count=%d decimal_point_pos=%d\n", digit_count, decimal_point_pos);
    {
        int zi;
        int all_zero = 1;
        /* Leading zeros were never counted in digit_buffer (they adjust weight,
         * not digit_count). So trailing zeros are the only zeros to check. */
        for (zi = 0; zi < digit_count; zi++) {
            if (digit_buffer[zi] != 0) {
                all_zero = 0;
                break;
            }
        }
        if (all_zero) {
            *sign = TECC_NUMERIC_SIGN_ZERO;
            /* Preserve weight/ndigits for scale info (0.0→2 digits, 0.00→3 digits).
             * Leading zeros (e.g. "0.00123") do not appear in digit_buffer, they
             * only adjust weight. For plain "0" or "0.0", digit_buffer has [0] or
             * [0,0] after encoding. */
            {
                /* ndigits is digit_count (includes trailing zeros).
                 * weight for zero with leading zeros before first non-zero:
                 * The digit buffer may be empty after leading zero removal below.
                 * Set weight so that decode outputs "0" correctly. */
                int adj = 0;
                while (adj < digit_count && digit_buffer[adj] == 0)
                    adj++;
                *weight = (decimal_point_pos >= 0) ? (decimal_point_pos - adj - 1 - exponent) : -1;
                if (*weight < -1) *weight = -1;
            }
            *ndigits = digit_count;
            return TECC_FORMATTER_SUCCESS;
        }
    }

    /* Calculate weight based on decimal point and exponent */
    if (decimal_point_pos >= 0) {
        /* Decimal point found in the number */
        calculated_weight = decimal_point_pos - exponent;
    } else {
        /* No decimal point, weight is based on digit count and exponent */
        calculated_weight = digit_count - 1 - exponent;
    }

    /* Adjust for leading zeros before decimal point */
    while (leading_zeros < digit_count && digit_buffer[leading_zeros] == 0) {
        leading_zeros++;
        calculated_weight--;
    }

    /* Remove leading zeros */
    if (leading_zeros > 0) {
        int j;
        for (j = 0; j < digit_count - leading_zeros; j++) {
            digit_buffer[j] = digit_buffer[j + leading_zeros];
        }
        digit_count -= leading_zeros;
    }

    /* Validate weight range */
    if (calculated_weight < TECC_NUMERIC_MIN_WEIGHT || calculated_weight > TECC_NUMERIC_MAX_WEIGHT)
        return TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW;

    /* Pack digits (4 decimal digits per uint16) */
    packed_idx = 0;
    {
        int j;
        for (j = 0; j < digit_count; j += 4) {
            uint16_t packed = 0;
            int digits_in_group = (digit_count - j < 4) ? (digit_count - j) : 4;
            int k;

            for (k = 0; k < digits_in_group; k++) {
                packed = packed * 10 + digit_buffer[j + k];
            }

            /* Pad with zeros if less than 4 digits */
            for (k = digits_in_group; k < 4; k++) {
                packed *= 10;
            }

        if (packed_idx < TECC_NUMERIC_MAX_DIGITS / 4) {
            digits[packed_idx++] = packed;
        }
    }
    }

    /* Set output values */
    *sign = negative ? TECC_NUMERIC_SIGN_NEGATIVE : TECC_NUMERIC_SIGN_POSITIVE;
    *weight = (int16_t)calculated_weight;
    *ndigits = digit_count;

    return TECC_FORMATTER_SUCCESS;
}

/* End - ReqID:TECC4.0 */