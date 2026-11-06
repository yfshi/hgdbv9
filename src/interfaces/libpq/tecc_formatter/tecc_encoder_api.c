/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_encoder_api.c
 *
 * Abstract:
 *   Unified encoding API implementation for TECC data formatter module.
 *   Provides unified interface for various encoding schemes.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_encoder_api.c
 */

#include "tecc_encoder_api.h"
#include "common/fe_memutils.h"  /* For pg_malloc, pg_free */

#include "tecc_hex_encoder.h"

/*
 * tecc_encoder_api_encode - Encode data using specified encoding
 *
 * Encodes binary data using the specified encoding scheme.
 */
TeccFormatterErrorCode
tecc_encoder_api_encode(
    TeccEncodingType encoding_type,
    const unsigned char *input_data,
    size_t input_len,
    char **output_data,
    size_t *output_len)
{
    TECC_RETURN_IF_NULL(input_data, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(output_data, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(output_len, TECC_FORMATTER_ERROR_INVALID_PARAM);

    switch (encoding_type) {
        case TECC_ENCODING_HEX:
            return tecc_hex_encoder_encode(input_data, input_len, output_data);

        case TECC_ENCODING_BASE64:
            /* TODO: Implement Base64 encoding */
            return TECC_FORMATTER_ERROR_ENCODING_FAILED;

        case TECC_ENCODING_URL_SAFE:
            /* TODO: Implement URL-safe encoding */
            return TECC_FORMATTER_ERROR_ENCODING_FAILED;

        case TECC_ENCODING_BINARY:
            /* Raw binary: copy as-is */
            *output_data = pg_malloc(input_len);
            TECC_RETURN_IF_MEMORY_ERROR(*output_data, TECC_FORMATTER_ERROR_MEMORY_ALLOC);
            memcpy(*output_data, input_data, input_len);
            *output_len = input_len;
            return TECC_FORMATTER_SUCCESS;

        default:
            return TECC_FORMATTER_ERROR_INVALID_PARAM;
    }
}

/*
 * tecc_encoder_api_decode - Decode data using specified encoding
 *
 * Decodes encoded data back to binary format.
 */
TeccFormatterErrorCode
tecc_encoder_api_decode(
    TeccEncodingType encoding_type,
    const char *input_data,
    size_t input_len,
    unsigned char **output_data,
    size_t *output_len)
{
    TECC_RETURN_IF_NULL(input_data, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(output_data, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(output_len, TECC_FORMATTER_ERROR_INVALID_PARAM);

    switch (encoding_type) {
        case TECC_ENCODING_HEX:
            return tecc_hex_encoder_decode(input_data, output_data, output_len);

        case TECC_ENCODING_BASE64:
            /* TODO: Implement Base64 decoding */
            return TECC_FORMATTER_ERROR_DECODING_FAILED;

        case TECC_ENCODING_URL_SAFE:
            /* TODO: Implement URL-safe decoding */
            return TECC_FORMATTER_ERROR_DECODING_FAILED;

        case TECC_ENCODING_BINARY:
            /* Raw binary: copy as-is */
            *output_data = pg_malloc(input_len);
            TECC_RETURN_IF_MEMORY_ERROR(*output_data, TECC_FORMATTER_ERROR_MEMORY_ALLOC);
            memcpy(*output_data, input_data, input_len);
            *output_len = input_len;
            return TECC_FORMATTER_SUCCESS;

        default:
            return TECC_FORMATTER_ERROR_INVALID_PARAM;
    }
}

/*
 * tecc_encoder_api_get_info - Get encoding information
 *
 * Retrieves information about encoding scheme.
 */
TeccFormatterErrorCode
tecc_encoder_api_get_info(
    TeccEncodingType encoding_type,
    size_t *encoded_size_multiplier,
    const char **encoding_name)
{
    TECC_RETURN_IF_NULL(encoded_size_multiplier, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(encoding_name, TECC_FORMATTER_ERROR_INVALID_PARAM);

    switch (encoding_type) {
        case TECC_ENCODING_HEX:
            *encoded_size_multiplier = 2;  /* 2 hex chars per byte */
            *encoding_name = "hexadecimal";
            break;

        case TECC_ENCODING_BASE64:
            *encoded_size_multiplier = 4;  /* ~4 base64 chars per 3 bytes */
            *encoding_name = "base64";
            break;

        case TECC_ENCODING_URL_SAFE:
            *encoded_size_multiplier = 4;  /* Similar to base64 */
            *encoding_name = "url-safe";
            break;

        case TECC_ENCODING_BINARY:
            *encoded_size_multiplier = 1;  /* No expansion */
            *encoding_name = "binary";
            break;

        default:
            return TECC_FORMATTER_ERROR_INVALID_PARAM;
    }

    return TECC_FORMATTER_SUCCESS;
}

/* End - ReqID:TECC4.0 */