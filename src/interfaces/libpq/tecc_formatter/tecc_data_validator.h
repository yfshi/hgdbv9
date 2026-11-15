/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_data_validator.h
 *
 * Abstract:
 *   Data validation interfaces for TECC data formatter module.
 *   Provides data validation and adjustment services.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_data_validator.h
 */

#ifndef TECC_DATA_VALIDATOR_H
#define TECC_DATA_VALIDATOR_H

#include "tecc_formatter.h"
#include "tecc_formatter_internal.h"

/*
 * Validate input data before normalization
 * Note: This function name follows design document: tecc_validate_input_data
 * For backward compatibility, tecc_data_validator_validate_input is also defined
 */
extern TeccFormatterErrorCode tecc_validate_input_data(
    const unsigned char *data,
    size_t len,
    Oid data_type,
    TeccAlgorithmType algo_type
);

/* Backward compatibility alias */
#define tecc_data_validator_validate_input tecc_validate_input_data

/*
 * ReqID:TECC4.0 - Validate integer decimal string against type range.
 * Used by HE decryption path where results are decimal strings
 * (from BN_bn2dec) rather than normalized binary.
 * Returns TECC_FORMATTER_SUCCESS if in range, error code otherwise.
 */
extern TeccFormatterErrorCode tecc_validate_integer_range(
	const char *str, size_t len, Oid data_type);

#endif /* TECC_DATA_VALIDATOR_H */