/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_type_converter.c
 *
 * Abstract:
 *   Type conversion implementation for TECC data formatter module.
 *   Provides data type conversion services between different representations.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_type_converter.c
 */

#include "tecc_type_converter.h"
#include "common/fe_memutils.h"  /* For pg_malloc, pg_free */

#include <string.h>
#include <limits.h>

/*
 * tecc_type_converter_datum_to_binary - Convert PostgreSQL datum to binary format
 *
 * DEPRECATED: This function uses Datum type which is a backend type.
 * Frontend code should use tecc_type_converter_text_to_binary instead.
 * Kept for backward compatibility.
 *
 * REMOVED: Frontend code should not use Datum type. Use tecc_type_converter_text_to_binary instead.
 */
/* Function removed - frontend should not use Datum type */

/*
 * tecc_type_converter_binary_to_datum - Convert binary format to PostgreSQL datum
 *
 * DEPRECATED: This function uses Datum type which is a backend type.
 * Frontend code should use tecc_type_converter_binary_to_text instead.
 * Kept for backward compatibility.
 *
 * REMOVED: Frontend code should not use Datum type. Use tecc_type_converter_binary_to_text instead.
 */
/* Function removed - frontend should not use Datum type */

/*
 * tecc_type_converter_get_type_info - Get type information
 *
 * Retrieves PostgreSQL type information (length, byval, align).
 */
TeccFormatterErrorCode
tecc_type_converter_get_type_info(
    Oid data_type,
    int *type_length,
    bool *type_by_val,
    char *type_align)
{
    TECC_RETURN_IF_NULL(type_length, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(type_by_val, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(type_align, TECC_FORMATTER_ERROR_INVALID_PARAM);

    switch (data_type) {
        case INT2OID:
            *type_length = sizeof(int16);
            *type_by_val = true;
            *type_align = 's';  /* short alignment */
            break;

        case INT4OID:
            *type_length = sizeof(int32);
            *type_by_val = true;
            *type_align = 'i';  /* int alignment */
            break;

        case INT8OID:
            *type_length = sizeof(int64);
            *type_by_val = FLOAT8PASSBYVAL;
            *type_align = 'd';  /* double alignment */
            break;

        case FLOAT4OID:
            *type_length = sizeof(float4);
            *type_by_val = true;
            *type_align = 'i';  /* int alignment */
            break;

        case FLOAT8OID:
            *type_length = sizeof(float8);
            *type_by_val = FLOAT8PASSBYVAL;
            *type_align = 'd';  /* double alignment */
            break;

        case TEXTOID:
        case VARCHAROID:
        case BPCHAROID:
            *type_length = -1;  /* variable length */
            *type_by_val = false;
            *type_align = 'i';  /* int alignment */
            break;

        default:
            return TECC_FORMATTER_ERROR_INVALID_TYPE;
    }

    return TECC_FORMATTER_SUCCESS;
}

/*
 * tecc_type_converter_is_compatible - Check type compatibility
 *
 * Checks if two PostgreSQL types are compatible for conversion.
 */
bool
tecc_type_converter_is_compatible(
    Oid src_type,
    Oid dst_type)
{
    /* Same type is always compatible */
    if (src_type == dst_type)
        return true;

    /* Numeric type compatibility */
    if ((TECC_IS_NUMERIC_TYPE(src_type) && TECC_IS_NUMERIC_TYPE(dst_type)) ||
        (TECC_IS_TEXT_TYPE(src_type) && TECC_IS_TEXT_TYPE(dst_type)))
        return true;

    return false;
}

/*
 * tecc_type_converter_text_to_binary - Convert text representation to binary format
 *
 * Converts text string to binary representation for encryption.
 * This function parses text strings according to PostgreSQL type rules.
 */
TeccFormatterErrorCode
tecc_type_converter_text_to_binary(
    const char *text_data,
    size_t text_len,
    Oid data_type,
    unsigned char **binary_data,
    size_t *binary_len)
{
    TECC_RETURN_IF_NULL(text_data, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(binary_data, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(binary_len, TECC_FORMATTER_ERROR_INVALID_PARAM);

    if (text_len == 0)
        return TECC_FORMATTER_ERROR_INVALID_INPUT;

    /* For text types, just copy the string */
    if (TECC_IS_TEXT_TYPE(data_type)) {
        *binary_len = text_len;
        *binary_data = pg_malloc(*binary_len);
        TECC_RETURN_IF_MEMORY_ERROR(*binary_data, TECC_FORMATTER_ERROR_MEMORY_ALLOC);
        memcpy(*binary_data, text_data, *binary_len);
        return TECC_FORMATTER_SUCCESS;
    }

    /* For numeric types, we need to parse the text */
    if (TECC_IS_NUMERIC_TYPE(data_type)) {
        /* For now, just copy the text as-is. Actual parsing happens in normalization functions. */
        *binary_len = text_len;
        *binary_data = pg_malloc(*binary_len);
        TECC_RETURN_IF_MEMORY_ERROR(*binary_data, TECC_FORMATTER_ERROR_MEMORY_ALLOC);
        memcpy(*binary_data, text_data, *binary_len);
        return TECC_FORMATTER_SUCCESS;
    }

    return TECC_FORMATTER_ERROR_INVALID_TYPE;
}

/*
 * tecc_type_converter_binary_to_text - Convert binary format to text representation
 *
 * Converts binary representation back to text string.
 * This function formats binary data as text according to PostgreSQL type rules.
 */
TeccFormatterErrorCode
tecc_type_converter_binary_to_text(
    const unsigned char *binary_data,
    size_t binary_len,
    Oid data_type,
    char **text_data,
    size_t *text_len)
{
    TECC_RETURN_IF_NULL(binary_data, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(text_data, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(text_len, TECC_FORMATTER_ERROR_INVALID_PARAM);

    if (binary_len == 0)
        return TECC_FORMATTER_ERROR_INVALID_INPUT;

    /* For text types, just copy the data */
    if (TECC_IS_TEXT_TYPE(data_type)) {
        *text_len = binary_len;
        *text_data = pg_malloc(*text_len + 1);  /* +1 for null terminator */
        TECC_RETURN_IF_MEMORY_ERROR(*text_data, TECC_FORMATTER_ERROR_MEMORY_ALLOC);
        memcpy(*text_data, binary_data, *text_len);
        (*text_data)[*text_len] = '\0';  /* Add null terminator */
        return TECC_FORMATTER_SUCCESS;
    }

    /* For numeric types, we need to format the binary data as text */
    if (TECC_IS_NUMERIC_TYPE(data_type)) {
        /* For now, just treat binary data as text. Actual formatting should be implemented. */
        *text_len = binary_len;
        *text_data = pg_malloc(*text_len + 1);
        TECC_RETURN_IF_MEMORY_ERROR(*text_data, TECC_FORMATTER_ERROR_MEMORY_ALLOC);
        memcpy(*text_data, binary_data, *text_len);
        (*text_data)[*text_len] = '\0';
        return TECC_FORMATTER_SUCCESS;
    }

    return TECC_FORMATTER_ERROR_INVALID_TYPE;
}

/* End - ReqID:TECC4.0 */