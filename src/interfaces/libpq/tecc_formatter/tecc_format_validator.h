/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_format_validator.h
 *
 * Abstract:
 *   Format validation interfaces for TECC data formatter module.
 *   Provides format validation services for various data representations.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_format_validator.h
 */

#ifndef TECC_FORMAT_VALIDATOR_H
#define TECC_FORMAT_VALIDATOR_H

#include "tecc_formatter.h"
#include "tecc_formatter_internal.h"

/*
 * Validate normalized data format
 */
extern TeccFormatterErrorCode tecc_format_validator_validate_normalized(
    const unsigned char *data,
    size_t len,
    Oid data_type,
    TeccAlgorithmType algo_type
);

/*
 * Validate encrypted data format
 */
extern TeccFormatterErrorCode tecc_format_validator_validate_encrypted(
    const unsigned char *data,
    size_t len,
    TeccAlgorithmType algo_type
);

/*
 * Validate hex string format
 */
extern TeccFormatterErrorCode tecc_format_validator_validate_hex(
    const char *hex_string
);

/*
 * Validate CipherDataType format
 */
extern TeccFormatterErrorCode tecc_format_validator_validate_cipherdata(
    const CipherDataType *cipherdata,
    size_t cipherdata_len
);

#endif /* TECC_FORMAT_VALIDATOR_H */