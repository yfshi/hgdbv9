/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_encoder_api.h
 *
 * Abstract:
 *   Unified encoding API interfaces for TECC data formatter module.
 *   Provides unified interface for various encoding schemes.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_encoder_api.h
 */

#ifndef TECC_ENCODER_API_H
#define TECC_ENCODER_API_H

#include "tecc_formatter.h"
#include "tecc_formatter_internal.h"

/* Encoding types */
typedef enum TeccEncodingType
{
    TECC_ENCODING_HEX,          /* Hexadecimal encoding */
    TECC_ENCODING_BASE64,       /* Base64 encoding */
    TECC_ENCODING_URL_SAFE,     /* URL-safe encoding */
    TECC_ENCODING_BINARY,       /* Raw binary (no encoding) */
    TECC_ENCODING_MAX
} TeccEncodingType;

/*
 * Encode data using specified encoding
 */
extern TeccFormatterErrorCode tecc_encoder_api_encode(
    TeccEncodingType encoding_type,
    const unsigned char *input_data,
    size_t input_len,
    char **output_data,
    size_t *output_len
);

/*
 * Decode data using specified encoding
 */
extern TeccFormatterErrorCode tecc_encoder_api_decode(
    TeccEncodingType encoding_type,
    const char *input_data,
    size_t input_len,
    unsigned char **output_data,
    size_t *output_len
);

/*
 * Get encoding information
 */
extern TeccFormatterErrorCode tecc_encoder_api_get_info(
    TeccEncodingType encoding_type,
    size_t *encoded_size_multiplier,
    const char **encoding_name
);

#endif /* TECC_ENCODER_API_H */