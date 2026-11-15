/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_hex_encoder.c
 *
 * Abstract:
 *   Hexadecimal encoding implementation for TECC data formatter module.
 *   Provides binary-to-hex and hex-to-binary conversion services.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_hex_encoder.c
 */

#include "tecc_hex_encoder.h"

#include <string.h>
#include <ctype.h>

/*
 * tecc_hex_encoder_encode - Encode binary data to hexadecimal string
 *
 * Converts binary data to hexadecimal string with optional PostgreSQL prefix.
 */
TeccFormatterErrorCode
tecc_hex_encoder_encode(
    const unsigned char *binary_data,
    size_t binary_len,
    char **hex_string)
{
    size_t hex_len;
    size_t i;

    TECC_RETURN_IF_NULL(binary_data, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(hex_string, TECC_FORMATTER_ERROR_INVALID_PARAM);

    if (binary_len == 0 )
        return TECC_FORMATTER_ERROR_DATA_TOO_LARGE;

    /* Allocate output buffer with PostgreSQL hex prefix */
    hex_len = binary_len * 2 + TECC_HEX_PREFIX_LENGTH + 1; /* +1 for null terminator */
    *hex_string = pg_malloc(hex_len);
    TECC_RETURN_IF_MEMORY_ERROR(*hex_string, TECC_FORMATTER_ERROR_MEMORY_ALLOC);

    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    {
        char *p = *hex_string;
        const char hex_digits[] = "0123456789abcdef";

        /* Add PostgreSQL hex prefix */
        *p++ = '\\';
        *p++ = 'x';

        /* Convert to hex */
    for (i = 0; i < binary_len; i++) {
        *p++ = hex_digits[binary_data[i] >> 4];
        *p++ = hex_digits[binary_data[i] & 0x0F];
    }

    *p = '\0';
        return TECC_FORMATTER_SUCCESS;
    }
}

/*
 * tecc_hex_encoder_decode - Decode hexadecimal string to binary data
 *
 * Converts hexadecimal string to binary data, handling PostgreSQL prefix.
 */
TeccFormatterErrorCode
tecc_hex_encoder_decode(
    const char *hex_string,
    unsigned char **binary_data,
    size_t *binary_len)
{
    /* ReqID:TECC4.0 - 复用已 LUT 优化的 tecc_formatter_hex_decode（tecc_formatter.h），
     * 避免重复实现低效的 if 比较解码；仅保留裸 x 前缀容错
     * （text parser 将 \x 的反斜杠吃掉），\x 前缀由 tecc_formatter_hex_decode 处理 */
    if (hex_string != NULL && hex_string[0] == 'x' &&
        hex_string[1] >= '0' && hex_string[1] <= '9')
        hex_string++;

    return tecc_formatter_hex_decode(hex_string, binary_data, binary_len);
}

/*
 * tecc_hex_encoder_validate_string - Validate hexadecimal string format
 *
 * Validates that string contains valid hexadecimal characters and has even length.
 */
TeccFormatterErrorCode
tecc_hex_encoder_validate_string(
    const char *hex_string,
    size_t *binary_len)
{
    size_t len;
    size_t i;

    TECC_RETURN_IF_NULL(hex_string, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(binary_len, TECC_FORMATTER_ERROR_INVALID_PARAM);

    len = strlen(hex_string);
    if (len == 0)
        return TECC_FORMATTER_ERROR_DATA_FORMAT;

    /* Check for even length (after stripping prefix) */
    if (len % 2 != 0)
        return TECC_FORMATTER_ERROR_DATA_FORMAT;

    /* Validate characters */
    for (i = 0; i < len; i++) {
        char c = hex_string[i];
        if (!isxdigit(c))
            return TECC_FORMATTER_ERROR_DATA_FORMAT;
    }

    *binary_len = len / 2;
    return TECC_FORMATTER_SUCCESS;
}

/*
 * tecc_hex_encoder_has_prefix - Check if string has PostgreSQL hex prefix
 *
 * Returns true if string starts with "\\x".
 */
bool
tecc_hex_encoder_has_prefix(const char *hex_string)
{
    if (!hex_string)
        return false;

    return (hex_string[0] == '\\' && hex_string[1] == 'x');
}

/*
 * tecc_hex_encoder_strip_prefix - Strip PostgreSQL hex prefix if present
 *
 * Returns pointer to hex data, skipping "\\x" prefix if present.
 */
const char *
tecc_hex_encoder_strip_prefix(const char *hex_string)
{
    if (!hex_string)
        return NULL;

    if (tecc_hex_encoder_has_prefix(hex_string))
        return hex_string + 2;

    return hex_string;
}

/* End - ReqID:TECC4.0 */