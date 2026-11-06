/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_normalizer.c
 *
 * Abstract:
 *   Normalization implementation for TECC data formatter module.
 *   Provides data normalization services for various data types.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_normalizer.c
 */

#include "tecc_normalizer.h"
#include "tecc_formatter_log.h"  /* Formatter logging functions */
#include "tecc_normalization_common.h"  /* Common normalization functions */
#include <string.h>
#include <math.h>
/* Type OIDs are defined in tecc_common_types.h */

/*
 * tecc_normalizer_normalize_numeric - Normalize numeric data for encryption
 *
 * Converts PostgreSQL numeric types to standardized format suitable for
 * encryption algorithms, especially OPE (Order-Preserving Encryption).
 */
TeccFormatterErrorCode
tecc_normalizer_normalize_numeric(
    TeccCryptoContextCore *context,
    const unsigned char *input_data,
    size_t input_len,
    unsigned char **output_data,
    size_t *output_len)
{
    TeccNumericNormalizedCommon norm_num;
    int norm_result;

    TECC_RETURN_IF_NULL(context, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(input_data, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(output_data, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(output_len, TECC_FORMATTER_ERROR_INVALID_PARAM);

    /* Initialize output parameters */
    *output_data = NULL;
    *output_len = 0;

    /* Initialize normalized structure */
    memset(&norm_num, 0, sizeof(TeccNumericNormalizedCommon));

    /* Dispatch based on data type */
    switch (context->data_type)
    {
        case INT2OID:
            /* For int2 type, parse text representation */
            norm_result = tecc_normalize_int2_text_common((const char *)input_data, input_len, &norm_num);
            break;
        case INT4OID:
            /* For int4 type, parse text representation */
            norm_result = tecc_normalize_int4_text_common((const char *)input_data, input_len, &norm_num);
            break;
        case INT8OID:
            /* For int8 type, parse text representation */
            norm_result = tecc_normalize_int8_text_common((const char *)input_data, input_len, &norm_num);
            break;
        case FLOAT4OID:
            /* For float4 type, parse text representation */
            norm_result = tecc_normalize_float4_text_common((const char *)input_data, input_len, &norm_num);
            break;
        case FLOAT8OID:
            /* For float8 type, parse text representation */
            norm_result = tecc_normalize_float8_text_common((const char *)input_data, input_len, &norm_num);
            break;
        case NUMERICOID:
            /* For numeric type, parse text representation */
            norm_result = tecc_normalize_numeric_text_common((const char *)input_data, input_len, &norm_num);
            break;
        default:
            tecc_formatter_error("Unsupported data type for numeric normalization: %u", context->data_type);
            return TECC_FORMATTER_ERROR_INVALID_PARAM;
    }

    if (norm_result != TECC_NORMALIZATION_SUCCESS)
    {
        tecc_formatter_error("Numeric normalization failed: %d", norm_result);
        return TECC_FORMATTER_ERROR_SYSTEM;
    }

    /* Encode normalized structure to byte stream */
    norm_result = tecc_encode_numeric_normalized_common(&norm_num, output_data, output_len);

    /* Free normalized structure */
    tecc_free_numeric_normalized_common(&norm_num);

    if (norm_result != TECC_NORMALIZATION_SUCCESS)
    {
        tecc_formatter_error("Numeric encoding failed: %d", norm_result);
        if (*output_data)
        {
            pg_free(*output_data);
            *output_data = NULL;
        }
        return TECC_FORMATTER_ERROR_SYSTEM;
    }

    return TECC_FORMATTER_SUCCESS;
}

/*
 * tecc_normalizer_normalize_text - Normalize text data for encryption
 *
 * Handles encoding conversion and algorithm-specific formatting for text data.
 */
TeccFormatterErrorCode
tecc_normalizer_normalize_text(
    TeccCryptoContextCore *context,
    const unsigned char *input_data,
    size_t input_len,
    unsigned char **output_data,
    size_t *output_len)
{
    int norm_result;

    TECC_RETURN_IF_NULL(context, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(input_data, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(output_data, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(output_len, TECC_FORMATTER_ERROR_INVALID_PARAM);

    /* Initialize output parameters */
    *output_data = NULL;
    *output_len = 0;

    /* Check if ICU sort key is needed (for text with collation) */
    /* ReqID:TECC4.0 - Skip ICU for C/POSIX collation: they use byte-wise
     * comparison, not ICU sorting. Otherwise ICU sort key would produce
     * collation-ordered ciphertexts incompatible with backend C ordering. */
    if (context->collation_locale[0] != '\0' &&
        strcmp(context->collation_locale, "C") != 0 &&
        strcmp(context->collation_locale, "POSIX") != 0)
    {
        /* Generate ICU sort key for text with collation */
        norm_result = tecc_normalize_text_with_icu_common(context->collation_locale,
                                                         (const char *)input_data,
                                                         input_len,
                                                         output_data,
                                                         output_len);
    }
    else
    {
        /* Simple text normalization (length prefix) */
        norm_result = tecc_normalize_text_simple_common((const char *)input_data,
                                                       input_len,
                                                       output_data,
                                                       output_len);
    }

    if (norm_result != TECC_NORMALIZATION_SUCCESS)
    {
        tecc_formatter_error("Text normalization failed: %d", norm_result);
        return TECC_FORMATTER_ERROR_SYSTEM;
    }

    return TECC_FORMATTER_SUCCESS;
}

/*
 * tecc_normalizer_validate_normalized_data - Validate normalized data
 *
 * Validates that normalized data conforms to expected format for the
 * given data type and algorithm.
 */
TeccFormatterErrorCode
tecc_normalizer_validate_normalized_data(
    const unsigned char *data,
    size_t len,
    Oid data_type,
    TeccAlgorithmType algo_type)
{
    TECC_RETURN_IF_NULL(data, TECC_FORMATTER_ERROR_INVALID_PARAM);

    /* Basic validation - check size limits */
    if (len == 0)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    /* Algorithm-specific validation */
    switch (algo_type)
    {
        case TECC_ALGO_OPE:
            /* OPE requires normalized data format */
            if (len < 8)  /* Minimum size for normalized numeric data */
                return TECC_FORMATTER_ERROR_DATA_FORMAT;
            break;

        case TECC_ALGO_DET:
            /* DET can work with raw or normalized data */
            /* No specific format requirement */
            break;

        case TECC_ALGO_RND:
        case TECC_ALGO_SEARCH:
            /* RND and SEARCH don't require normalization */
            /* No validation needed */
            break;

        default:
            return TECC_FORMATTER_ERROR_INVALID_PARAM;
    }

    return TECC_FORMATTER_SUCCESS;
}

/* End - ReqID:TECC4.0 */