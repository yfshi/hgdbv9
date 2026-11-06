/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_hex_encoder.h
 *
 * Abstract:
 *   Hexadecimal encoding interfaces for TECC data formatter module.
 *   Provides binary-to-hex and hex-to-binary conversion services.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_hex_encoder.h
 */

#ifndef TECC_HEX_ENCODER_H
#define TECC_HEX_ENCODER_H

#include "tecc_formatter.h"
#include "tecc_formatter_internal.h"

/*
 * Encode binary data to hexadecimal string
 */
extern TeccFormatterErrorCode tecc_hex_encoder_encode(
    const unsigned char *binary_data,
    size_t binary_len,
    char **hex_string
);

/*
 * Decode hexadecimal string to binary data
 */
extern TeccFormatterErrorCode tecc_hex_encoder_decode(
    const char *hex_string,
    unsigned char **binary_data,
    size_t *binary_len
);

/*
 * Validate hexadecimal string format
 */
extern TeccFormatterErrorCode tecc_hex_encoder_validate_string(
    const char *hex_string,
    size_t *binary_len
);

/*
 * Check if string has PostgreSQL hex prefix
 */
extern bool tecc_hex_encoder_has_prefix(const char *hex_string);

/*
 * Strip PostgreSQL hex prefix if present
 */
extern const char *tecc_hex_encoder_strip_prefix(const char *hex_string);

#endif /* TECC_HEX_ENCODER_H */