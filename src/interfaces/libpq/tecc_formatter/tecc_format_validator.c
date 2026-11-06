/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_format_validator.c
 *
 * Abstract:
 *   Format validation implementation for TECC data formatter module.
 *   Provides format validation services for various data representations.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_format_validator.c
 */

#include "tecc_format_validator.h"
#include "tecc_cipherdata.h"      /* For tecc_cipherdata_validate */
#include "common/fe_memutils.h"  /* For pg_malloc, pg_free */

#include <string.h>
#include <ctype.h>

/*
 * tecc_format_validator_validate_normalized - Validate normalized data format
 *
 * Validates that normalized data conforms to expected format for the
 * given data type and algorithm.
 */
TeccFormatterErrorCode
tecc_format_validator_validate_normalized(
    const unsigned char *data,
    size_t len,
    Oid data_type,
    TeccAlgorithmType algo_type)
{
    TECC_RETURN_IF_NULL(data, TECC_FORMATTER_ERROR_INVALID_PARAM);

    if (len == 0 )
        return TECC_FORMATTER_ERROR_DATA_FORMAT;

    /* Type-specific validation */
    if (TECC_IS_NUMERIC_TYPE(data_type)) {
        /* Numeric: check OPE format if applicable */
        if (algo_type == TECC_ALGO_OPE) {
            uint8_t sign;

            if (len < TECC_NUMERIC_HEADER_SIZE)
                return TECC_FORMATTER_ERROR_DATA_FORMAT;

            /* Validate sign byte */
            sign = data[0];
            if (sign > TECC_NUMERIC_SIGN_NINF)
                return TECC_FORMATTER_ERROR_DATA_FORMAT;
        }
    } else if (TECC_IS_TEXT_TYPE(data_type)) {
        /* Text: check length prefix for RND */
        if (algo_type == TECC_ALGO_RND) {
            uint32_t declared_len;

            if (len < TECC_TEXT_LENGTH_PREFIX_SIZE)
                return TECC_FORMATTER_ERROR_DATA_FORMAT;

            /* Validate that length matches actual data */
            declared_len = ((uint32_t)data[0] << 24) |
                                   ((uint32_t)data[1] << 16) |
                                   ((uint32_t)data[2] << 8) |
                                   (uint32_t)data[3];

            if (declared_len != len - TECC_TEXT_LENGTH_PREFIX_SIZE)
                return TECC_FORMATTER_ERROR_DATA_FORMAT;
        }
    }

    return TECC_FORMATTER_SUCCESS;
}

/*
 * tecc_format_validator_validate_encrypted - Validate encrypted data format
 *
 * Validates encrypted data format for specific algorithm.
 */
TeccFormatterErrorCode
tecc_format_validator_validate_encrypted(
    const unsigned char *data,
    size_t len,
    TeccAlgorithmType algo_type)
{
    TECC_RETURN_IF_NULL(data, TECC_FORMATTER_ERROR_INVALID_PARAM);

    if (len == 0 )
        return TECC_FORMATTER_ERROR_DATA_FORMAT;

    /* Algorithm-specific validation */
    switch (algo_type) {
        case TECC_ALGO_RND:
        case TECC_ALGO_DET:
            /* No specific format requirements */
            break;

        case TECC_ALGO_OPE:
            /* OPE encrypted data should have numeric structure */
            /* TODO: Add OPE-specific validation */
            break;

        case TECC_ALGO_SEARCH:
            /* SEARCH encrypted data may have segment structure */
            /* TODO: Add SEARCH-specific validation */
            break;

        default:
            return TECC_FORMATTER_ERROR_INVALID_PARAM;
    }

    return TECC_FORMATTER_SUCCESS;
}

/*
 * tecc_format_validator_validate_hex - Validate hex string format
 *
 * Validates hexadecimal string format.
 */
TeccFormatterErrorCode
tecc_format_validator_validate_hex(
    const char *hex_string)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    size_t len;
    const char *hex_data;

    TECC_RETURN_IF_NULL(hex_string, TECC_FORMATTER_ERROR_INVALID_PARAM);

    len = strlen(hex_string);
    if (len == 0)
        return TECC_FORMATTER_ERROR_DATA_FORMAT;

    /* Strip PostgreSQL prefix if present */
    hex_data = hex_string;
    if (hex_string[0] == '\\' && hex_string[1] == 'x') {
        hex_data = hex_string + 2;
        len -= 2;
    }

    /* Check even length */
    if (len % 2 != 0)
        return TECC_FORMATTER_ERROR_DATA_FORMAT;

    /* Validate hex characters */
    {
        size_t i;
        for (i = 0; i < len; i++) {
            if (!isxdigit((unsigned char)hex_data[i]))
                return TECC_FORMATTER_ERROR_DATA_FORMAT;
        }
    }

    return TECC_FORMATTER_SUCCESS;
}

/*
 * tecc_format_validator_validate_cipherdata - Validate CipherDataType format
 *
 * Validates CipherDataType structure integrity.
 */
TeccFormatterErrorCode
tecc_format_validator_validate_cipherdata(
    const CipherDataType *cipherdata,
    size_t cipherdata_len)
{
    /* Delegate to cipherdata module */
    return tecc_cipherdata_validate(cipherdata, cipherdata_len);
}

/* End - ReqID:TECC4.0 */