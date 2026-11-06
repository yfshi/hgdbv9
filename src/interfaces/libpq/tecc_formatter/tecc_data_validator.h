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
 * Validate encrypted data integrity
 */
extern TeccFormatterErrorCode tecc_data_validator_validate_encrypted_data(
    const unsigned char *encrypted_data,
    size_t encrypted_len,
    TeccAlgorithmType algo_type
);

/*
 * Adjust data size if needed
 */
extern TeccFormatterErrorCode tecc_data_validator_adjust_data_size(
    const unsigned char *input_data,
    size_t input_len,
    Oid data_type,
    TeccAlgorithmType algo_type,
    unsigned char **adjusted_data,
    size_t *adjusted_len
);

/*
 * Check data compatibility with encryption algorithm
 */
extern bool tecc_data_validator_is_compatible(
    const unsigned char *data,
    size_t len,
    Oid data_type,
    TeccAlgorithmType algo_type
);

#endif /* TECC_DATA_VALIDATOR_H */