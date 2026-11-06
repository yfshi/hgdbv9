/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_cipherdata.h
 *
 * Abstract:
 *   CipherDataType handling interfaces for TECC data formatter module.
 *   Provides CipherDataType assembly and parsing services.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_cipherdata.h
 */

#ifndef TECC_CIPHERDATA_H
#define TECC_CIPHERDATA_H

#include "tecc_formatter.h"
#include "tecc_formatter_internal.h"

/*
 * Build CipherDataType from algorithm ciphertexts
 */
extern TeccFormatterErrorCode tecc_cipherdata_build(
    TeccAlgorithmType algo_type,
    Oid data_type,
    unsigned char **encrypted_data,
    size_t *encrypted_lens,
    CipherDataType **cipherdata,
    size_t *cipherdata_len
);

/*
 * Parse CipherDataType to extract algorithm ciphertexts
 */
extern TeccFormatterErrorCode tecc_cipherdata_parse(
    const CipherDataType *cipherdata,
    size_t cipherdata_len,
    TeccParsedCipherdata *parsed_data
);

/*
 * Validate CipherDataType structure
 */
extern TeccFormatterErrorCode tecc_cipherdata_validate(
    const CipherDataType *cipherdata,
    size_t cipherdata_len
);

/*
 * Calculate CipherDataType size requirements
 */
extern size_t tecc_cipherdata_calculate_size(
    size_t rnd_len,
    size_t det_len,
    size_t ope_len,
    size_t search_len
);

#endif /* TECC_CIPHERDATA_H */