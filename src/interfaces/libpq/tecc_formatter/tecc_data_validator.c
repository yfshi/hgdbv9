/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_data_validator.c
 *
 * Abstract:
 *   Data validation implementation for TECC data formatter module.
 *   Provides data validation and adjustment services.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_data_validator.c
 */

#include "tecc_data_validator.h"
#include "common/fe_memutils.h"  /* For pg_malloc, pg_free */
#include "catalog/pg_type_enc.h"  /* ReqID:TECC4.0 - INT2_ENCOID etc. for HE range check */

#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <math.h>
#include <float.h>
#include <stdint.h>

/* PostgreSQL port functions for case-insensitive string comparison */
#include "port.h"

/* Use PostgreSQL's pg_strncasecmp from port.h */

/* Internal helper functions */
static TeccFormatterErrorCode validate_numeric_data(const unsigned char *data, size_t len, Oid data_type);
static TeccFormatterErrorCode validate_text_data(const unsigned char *data, size_t len, Oid data_type);
static bool is_valid_integer_string(const char *str, size_t len);
static bool is_valid_float_string(const char *str, size_t len);
static TeccFormatterErrorCode validate_float_range(const char *str, size_t len, Oid data_type);
static TeccFormatterErrorCode validate_numeric_special_values(const char *str, size_t len);

/*
 * tecc_validate_input_data - Validate input data before normalization
 *
 * Validates input data format and constraints before processing.
 * This is the canonical function name per design document.
 */
TeccFormatterErrorCode
tecc_validate_input_data(
    const unsigned char *data,
    size_t len,
    Oid data_type,
    TeccAlgorithmType algo_type)
{
    TECC_RETURN_IF_NULL(data, TECC_FORMATTER_ERROR_INVALID_PARAM);

    /* Check size limits: empty string is valid for text types, invalid for others */
    if (len == 0 && !TECC_IS_TEXT_TYPE(data_type))
        return TECC_FORMATTER_ERROR_DATA_FORMAT;


    /* Type-specific validation */
    if (TECC_IS_NUMERIC_TYPE(data_type)) {
        /* Numeric validation */
        TeccFormatterErrorCode numeric_ret = validate_numeric_data(data, len, data_type);
        if (numeric_ret != TECC_FORMATTER_SUCCESS)
            return numeric_ret;
    } else if (TECC_IS_TEXT_TYPE(data_type)) {
        /* Text validation */
        TeccFormatterErrorCode text_ret = validate_text_data(data, len, data_type);
        if (text_ret != TECC_FORMATTER_SUCCESS)
            return text_ret;
    } else {
        /* Unsupported data type */
        return TECC_FORMATTER_ERROR_INVALID_TYPE;
    }

    /* Algorithm-specific validation */
    if (algo_type == TECC_ALGO_OPE && TECC_IS_TEXT_TYPE(data_type)) {
        /* OPE with text requires collation */
        /* Note: Collation validation is done at runtime when generating sort keys */
        /* ICU is mandatory for text OPE - availability checked at compile time */
    }

    return TECC_FORMATTER_SUCCESS;
}

/* Begin - ReqID:TECC4.0 */
/*
 * validate_numeric_data - Validate numeric data
 *
 * Validates numeric data based on data type, following PostgreSQL backend
 * IN function validation logic.
 */
static TeccFormatterErrorCode
validate_numeric_data(const unsigned char *data, size_t len, Oid data_type)
{
    if (len == 0)
        return TECC_FORMATTER_ERROR_DATA_FORMAT;

    /* For client-side validation, we receive data as strings from SQL layer */
    /* The actual binary conversion happens in normalization functions */

    /* Type-specific validation based on PostgreSQL IN function logic */
    switch (data_type) {
        case INT2OID:
            /* int2: validate as string, conversion happens later */
            if (!is_valid_integer_string((const char *)data, len))
                return TECC_FORMATTER_ERROR_DATA_FORMAT;
            /* Add range validation for int2 (-32768 to 32767) */
            return tecc_validate_integer_range((const char *)data, len, INT2OID);

        case INT4OID:
            /* int4: validate as string, conversion happens later */
            if (!is_valid_integer_string((const char *)data, len))
                return TECC_FORMATTER_ERROR_DATA_FORMAT;
            /* Add range validation for int4 (-2147483648 to 2147483647) */
            return tecc_validate_integer_range((const char *)data, len, INT4OID);

        case INT8OID:
            /* int8: validate as string, conversion happens later */
            if (!is_valid_integer_string((const char *)data, len))
                return TECC_FORMATTER_ERROR_DATA_FORMAT;
            /* Add range validation for int8 */
            return tecc_validate_integer_range((const char *)data, len, INT8OID);

        case FLOAT4OID:
            /* float4: validate as string, conversion happens later */
            if (!is_valid_float_string((const char *)data, len))
                return TECC_FORMATTER_ERROR_DATA_FORMAT;
            /* Add empty string check and range validation */
            return validate_float_range((const char *)data, len, FLOAT4OID);

        case FLOAT8OID:
            /* float8: validate as string, conversion happens later */
            if (!is_valid_float_string((const char *)data, len))
                return TECC_FORMATTER_ERROR_DATA_FORMAT;
            /* Add empty string check and range validation */
            return validate_float_range((const char *)data, len, FLOAT8OID);

        case NUMERICOID:
            /* numeric: validate as string, conversion happens later */
            /* PostgreSQL numeric accepts: digits, decimal point, sign, exponent, special values */
            if (!is_valid_float_string((const char *)data, len))
                return TECC_FORMATTER_ERROR_DATA_FORMAT;
            /* Add special value support (NaN, Infinity, Inf) */
            return validate_numeric_special_values((const char *)data, len);

        default:
            return TECC_FORMATTER_ERROR_INVALID_TYPE;
    }

    return TECC_FORMATTER_SUCCESS;
}

/*
 * validate_text_data - Validate text data
 *
 * Validates text data format and encoding, following PostgreSQL backend
 * text IN function validation logic.
 *
 * PostgreSQL text types accept any byte sequence, including null bytes.
 * The cstring_to_text_with_len() function does not require null termination.
 */
static TeccFormatterErrorCode
validate_text_data(const unsigned char *data, size_t len, Oid data_type)
{
    /* PostgreSQL text types accept any byte sequence */
    /* cstring_to_text_with_len() accepts any byte sequence, no null byte checks */

    /* Type-specific validation */
    switch (data_type) {
        case VARCHAROID:
            /* varchar: variable length text, accepts any byte sequence */
            /* No null byte checks - PostgreSQL allows null bytes */
            break;

        case BPCHAROID:
            /* bpchar: fixed-length char, padded with spaces */
            /* No null byte checks - PostgreSQL allows null bytes */
            /* TODO: Check length constraints if specified */
            break;

        case TEXTOID:
            /* text: unlimited length, accepts any byte sequence */
            /* No null byte checks - PostgreSQL allows null bytes */
            break;

        default:
            return TECC_FORMATTER_ERROR_INVALID_TYPE;
    }

    /* Note: PostgreSQL text types accept any byte sequence, so no encoding validation is needed */

    return TECC_FORMATTER_SUCCESS;
}

/*
 * is_valid_integer_string - Check if string is valid integer
 *
 * Validates integer string format, following PostgreSQL backend logic.
 * Allows leading/trailing whitespace, optional sign, requires at least one digit.
 */
static bool
is_valid_integer_string(const char *str, size_t len)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    size_t start = 0;
    size_t end;
    bool has_digit = false;

    if (len == 0)
        return false;

    end = len;

    /* Skip leading whitespace (like PostgreSQL backend) */
    while (start < end && isspace((unsigned char)str[start]))
        start++;

    if (start >= end)
        return false;  /* Only whitespace */

    /* Optional sign */
    if (str[start] == '+' || str[start] == '-') {
        start++;
        if (start >= end)
            return false;  /* Only sign */
    }

    /* Skip trailing whitespace */
    while (end > start && isspace((unsigned char)str[end - 1]))
        end--;

    /* Check all characters are digits */
    {
        size_t i;
        for (i = start; i < end; i++) {
            if (!isdigit((unsigned char)str[i]))
                return false;
            has_digit = true;
        }
    }

    return has_digit;  /* Must have at least one digit */
}

/*
 * is_valid_float_string - Check if string is valid float
 *
 * Validates float string format, following PostgreSQL backend logic.
 * Allows: digits, decimal point, sign, exponent (e/E), special values.
 */
static bool
is_valid_float_string(const char *str, size_t len)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    size_t start = 0;
    size_t end;
    bool has_dot = false;
    bool has_digit = false;
    bool in_exponent = false;
    bool exponent_has_digit = false;

    if (len == 0)
        return false;

    end = len;

    /* Skip leading whitespace (like PostgreSQL backend) */
    while (start < end && isspace((unsigned char)str[start]))
        start++;

    if (start >= end)
        return false;  /* Only whitespace */

    /* Skip trailing whitespace */
    while (end > start && isspace((unsigned char)str[end - 1]))
        end--;

    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    {
        const char *remaining;
        size_t remaining_len;

        /* Check for special values: NaN, Infinity, Inf */
        remaining = str + start;
        remaining_len = end - start;

        /* Check for NaN (case-insensitive) */
        if (remaining_len >= 3 &&
            (pg_strncasecmp(remaining, "NaN", 3) == 0 ||
             pg_strncasecmp(remaining, "nan", 3) == 0)) {
            /* Check if it's exactly "NaN" or has trailing characters */
            if (remaining_len == 3)
                return true;
            /* PostgreSQL allows NaN with optional payload in parentheses */
            if (remaining_len > 3 && remaining[3] == '(') {
                /* Check for matching closing parenthesis */
                const char *paren_start = remaining + 3;
                const char *paren_end = remaining + remaining_len;
                const char *p = paren_start + 1;
                while (p < paren_end && *p != ')')
                    p++;
                if (p < paren_end && *p == ')') {
                    return true;
                }
                /* No matching closing parenthesis - invalid format */
                return false;
            }
        }

        /* Check for Infinity/Inf (case-insensitive) */
        if (remaining_len >= 3 &&
            (pg_strncasecmp(remaining, "Inf", 3) == 0 ||
             pg_strncasecmp(remaining, "inf", 3) == 0)) {
            /* Check for optional "inity" suffix */
            if (remaining_len == 3)
                return true;
            if (remaining_len >= 8 &&
                pg_strncasecmp(remaining, "Infinity", 8) == 0)
                return true;
        }

        /* Check for signed Infinity/Inf */
        if ((remaining[0] == '+' || remaining[0] == '-') && remaining_len >= 4) {
            const char *signless = remaining + 1;
            size_t signless_len = remaining_len - 1;

            if (signless_len >= 3 &&
                (pg_strncasecmp(signless, "Inf", 3) == 0 ||
                 pg_strncasecmp(signless, "inf", 3) == 0)) {
                if (signless_len == 3)
                    return true;
                if (signless_len >= 8 &&
                    pg_strncasecmp(signless, "Infinity", 8) == 0)
                    return true;
            }
        }
    }

    /* Optional sign for regular numbers */
    if (str[start] == '+' || str[start] == '-') {
        start++;
        if (start >= end)
            return false;  /* Only sign */
    }

    /* Check format */
    {
        size_t i;
        for (i = start; i < end; i++) {
            char c = str[i];

            if (isdigit((unsigned char)c)) {
                if (in_exponent)
                    exponent_has_digit = true;
                else
                    has_digit = true;
            } else if (c == '.') {
                if (has_dot || in_exponent)  /* Multiple dots or dot in exponent */
                    return false;
                has_dot = true;
            } else if (c == 'e' || c == 'E') {
                if (in_exponent)  /* Multiple exponents */
                    return false;
                in_exponent = true;
                /* Exponent can have optional sign */
                if (i + 1 < end && (str[i + 1] == '+' || str[i + 1] == '-'))
                    i++;  /* Skip exponent sign */
            } else {
                return false;  /* Invalid character */
            }
        }
    }

    /* Must have at least one digit in mantissa, and if exponent present, must have digit */
    if (in_exponent && !exponent_has_digit)
        return false;

    return has_digit || (has_dot && !in_exponent);  /* Allow ".123" or "123." */
}

/*
 * tecc_validate_integer_range - Validate integer string range
 *
 * Validates integer string range based on data type, following PostgreSQL
 * pg_strtoint16/32/64 logic.
 */
/*
 * ReqID:TECC4.0 - Validate integer decimal string against type range.
 * Public API for HE decryption path (decimal strings from BN_bn2dec).
 */
TeccFormatterErrorCode
tecc_validate_integer_range(const char *str, size_t len, Oid data_type)
{
    const char *ptr = str;
    const char *end = str + len;
    bool neg = false;
    long long result = 0;  /* Use long long to detect overflow before conversion */

    /* Skip leading whitespace */
    while (ptr < end && isspace((unsigned char)*ptr))
        ptr++;

    if (ptr >= end)
        return TECC_FORMATTER_ERROR_DATA_FORMAT;

    /* Handle sign */
    if (*ptr == '-') {
        neg = true;
        ptr++;
    } else if (*ptr == '+') {
        ptr++;
    }

    /* Require at least one digit */
    if (ptr >= end || !isdigit((unsigned char)*ptr))
        return TECC_FORMATTER_ERROR_DATA_FORMAT;

    /* Process digits */
    while (ptr < end && isdigit((unsigned char)*ptr)) {
        int digit = (*ptr++ - '0');

        /* Check for overflow before multiplying */
        if (neg) {
            /* For negative numbers, we accumulate as negative */
            if (result < LLONG_MIN / 10)
                return TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW;
            result *= 10;
            if (result < LLONG_MIN + digit)
                return TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW;
            result -= digit;
        } else {
            /* For positive numbers */
            if (result > LLONG_MAX / 10)
                return TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW;
            result *= 10;
            if (result > LLONG_MAX - digit)
                return TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW;
            result += digit;
        }
    }

    /* Skip trailing whitespace */
    while (ptr < end && isspace((unsigned char)*ptr))
        ptr++;

    /* Check for trailing non-whitespace characters */
    if (ptr < end)
        return TECC_FORMATTER_ERROR_DATA_FORMAT;

    /* Type-specific range checks — handle both native PG OIDs and encrypted OIDs */
    switch (data_type) {
        case INT2OID:
        case INT2_ENCOID:
            if (result < -32768 || result > 32767)
                return TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW;
            break;

        case INT4OID:
        case INT4_ENCOID:
            if (result < -2147483648LL || result > 2147483647LL)
                return TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW;
            break;

        case INT8OID:
        case INT8_ENCOID:
            /* long long already covers int64 range on most platforms */
            if (result < -9223372036854775807LL - 1 || result > 9223372036854775807LL)
                return TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW;
            break;

        default:
            /* Non-integer types (float, numeric, text, etc.) — skip integer range check */
            break;
    }

    return TECC_FORMATTER_SUCCESS;
}

/*
 * validate_float_range - Validate float string range and format
 *
 * Validates float string format and range, following PostgreSQL
 * float4in/float8in logic.
 */
static TeccFormatterErrorCode
validate_float_range(const char *str, size_t len, Oid data_type)
{
    const char *ptr = str;
    const char *end = str + len;

    /* Skip leading whitespace */
    while (ptr < end && isspace((unsigned char)*ptr))
        ptr++;

    if (ptr >= end)
        return TECC_FORMATTER_ERROR_DATA_FORMAT;  /* Empty string after whitespace */

    /* Check for special values first (handled by is_valid_float_string) */
    /* The actual range checking happens during strtod conversion in normalization */

    /* Skip trailing whitespace */
    while (end > ptr && isspace((unsigned char)*(end - 1)))
        end--;

    /* Basic format validation (already done by is_valid_float_string) */
    /* Additional checks for PostgreSQL compatibility */

    /* PostgreSQL float4in/float8in check for empty string after whitespace */
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    {
        char *endptr = NULL;
        double value;

        if (ptr == end)
            return TECC_FORMATTER_ERROR_DATA_FORMAT;

        /* Parse the string to check range */
        errno = 0;
        value = strtod(ptr, &endptr);

    /* ReqID:TECC4.0 - Handle ERANGE like PG backend float4in/float8in:
     * Only true overflow (isinf) is rejected. Gradual underflow
     * (subnormal or zero, e.g. 1e-308 or 1e-46) is accepted by PG. */
    if (errno == ERANGE)
    {
        if (isinf(value))
        {
            /* True overflow: reject, PG reports "out of range" */
            return TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW;
        }
        /* else: gradual underflow (subnormal or zero), PG accepts this */
    }

    /* Check if parsing succeeded */
    if (endptr != end) {
        /* Not all characters were consumed - format error */
        return TECC_FORMATTER_ERROR_DATA_FORMAT;
    }

    /* Check for NaN or Infinity - these are valid */
    if (isnan(value) || isinf(value)) {
        return TECC_FORMATTER_SUCCESS;
    }

    /* Check range based on data type */
    if (data_type == FLOAT4OID) {
        /* float4 range: -FLT_MAX to FLT_MAX */
        if (value < -FLT_MAX || value > FLT_MAX) {
            return TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW;
        }
        /* Also check for underflow to 0 (subnormal numbers are allowed) */
        if (value != 0.0 && fabs(value) < FLT_MIN) {
            /* Value will underflow to 0, which is acceptable */
            /* No error, just note that it will underflow */
        }
    } else if (data_type == FLOAT8OID) {
        /* float8 range: -DBL_MAX to DBL_MAX */
        if (value < -DBL_MAX || value > DBL_MAX) {
            return TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW;
        }
        /* Also check for underflow to 0 (subnormal numbers are allowed) */
        if (value != 0.0 && fabs(value) < DBL_MIN) {
            /* Value will underflow to 0, which is acceptable */
            /* No error, just note that it will underflow */
        }
    }

        return TECC_FORMATTER_SUCCESS;
    }
}

/*
 * validate_numeric_special_values - Validate numeric special values
 *
 * Validates numeric special values (NaN, Infinity, Inf) following
 * PostgreSQL numeric_in logic.
 */
static TeccFormatterErrorCode
validate_numeric_special_values(const char *str, size_t len)
{
    const char *ptr = str;
    const char *end = str + len;

    /* Skip leading whitespace */
    while (ptr < end && isspace((unsigned char)*ptr))
        ptr++;

    if (ptr >= end)
        return TECC_FORMATTER_ERROR_DATA_FORMAT;

    /* Check for NaN (case-insensitive) */
    if (end - ptr >= 3) {
        if (pg_strncasecmp(ptr, "NaN", 3) == 0) {
            ptr += 3;
            /* Check for optional payload in parentheses */
            if (ptr < end && *ptr == '(') {
                const char *payload_start = ptr + 1;
                /* Skip payload - PostgreSQL accepts any payload */
                while (ptr < end && *ptr != ')')
                    ptr++;
                if (ptr < end && *ptr == ')') {
                    /* Check that payload is not empty (optional) */
                    if (ptr == payload_start) {
                        /* Empty payload: NaN() - PostgreSQL allows this */
                    }
                    ptr++;
                } else {
                    /* Unmatched opening parenthesis */
                    return TECC_FORMATTER_ERROR_DATA_FORMAT;
                }
            }
        } else if (pg_strncasecmp(ptr, "Inf", 3) == 0) {
            /* Reject Infinity/Inf for NUMERIC — PG numeric_in rejects these */
            return TECC_FORMATTER_ERROR_DATA_FORMAT;
        } else if ((*ptr == '+' || *ptr == '-') && end - ptr >= 4 &&
                   pg_strncasecmp(ptr + 1, "Inf", 3) == 0) {
            /* Reject signed Infinity/Inf for NUMERIC — PG numeric_in rejects these */
            return TECC_FORMATTER_ERROR_DATA_FORMAT;
        }
        /* else: regular numeric literal — handled by caller */
    }

    /* Skip trailing whitespace */
    while (ptr < end && isspace((unsigned char)*ptr))
        ptr++;

    /* If we matched a special value, ensure no trailing garbage */
    if (ptr < end) {
        /* Check if we actually matched a special value */
        /* If not, it's a regular number - validation happens elsewhere */
        return TECC_FORMATTER_SUCCESS;
    }

    return TECC_FORMATTER_SUCCESS;
}


/* End - ReqID:TECC4.0 */
/* End - ReqID:TECC4.0 */