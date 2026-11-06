/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_normalizer.h
 *
 * Abstract:
 *   Normalization interfaces for TECC data formatter module.
 *   Provides data normalization services for various data types.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_normalizer.h
 */

#ifndef TECC_NORMALIZER_H
#define TECC_NORMALIZER_H

#include "tecc_formatter.h"
#include "tecc_formatter_internal.h"

/*
 * Normalize numeric data for encryption
 * Converts PostgreSQL numeric types to standardized format
 */
extern TeccFormatterErrorCode tecc_normalizer_normalize_numeric(
    TeccCryptoContextCore *context,
    const unsigned char *input_data,
    size_t input_len,
    unsigned char **output_data,
    size_t *output_len
);

/*
 * Normalize text data for encryption
 * Handles encoding conversion and algorithm-specific formatting
 */
extern TeccFormatterErrorCode tecc_normalizer_normalize_text(
    TeccCryptoContextCore *context,
    const unsigned char *input_data,
    size_t input_len,
    unsigned char **output_data,
    size_t *output_len
);

/*
 * Validate normalized data
 */
extern TeccFormatterErrorCode tecc_normalizer_validate_normalized_data(
    const unsigned char *data,
    size_t len,
    Oid data_type,
    TeccAlgorithmType algo_type
);

#endif /* TECC_NORMALIZER_H */