/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_cipherdata.c
 *
 * Abstract:
 *   CipherDataType handling implementation for TECC data formatter module.
 *   Provides CipherDataType assembly and parsing services.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_cipherdata.c
 */

#include "tecc_cipherdata.h"
#include "common/fe_memutils.h"  /* For pg_malloc, pg_free */

#include <string.h>

/*
 * tecc_cipherdata_build - Build CipherDataType from algorithm ciphertexts
 *
 * Assembles a CipherDataType structure from individual algorithm ciphertexts.
 * The structure follows PostgreSQL varlena format with proper header.
 */
TeccFormatterErrorCode
tecc_cipherdata_build(
    TeccAlgorithmType algo_type,
    Oid data_type,
    unsigned char **encrypted_data,
    size_t *encrypted_lens,
    CipherDataType **cipherdata,
    size_t *cipherdata_len)
{
    size_t total_len;
    CipherDataType *cd;
    char *dest_ptr;

    /* 1. Parameter validation */
    TECC_RETURN_IF_NULL(encrypted_data, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(encrypted_lens, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(cipherdata, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(cipherdata_len, TECC_FORMATTER_ERROR_INVALID_PARAM);

    /* 2. Calculate total length */
    total_len = sizeof(CipherDataType) +
                encrypted_lens[TECC_ALGO_RND] +
                encrypted_lens[TECC_ALGO_DET] +
                encrypted_lens[TECC_ALGO_OPE] +
                encrypted_lens[TECC_ALGO_SEARCH];

    /* 3. Allocate memory */
    cd = (CipherDataType *)pg_malloc0(total_len);
    TECC_RETURN_IF_MEMORY_ERROR(cd, TECC_FORMATTER_ERROR_MEMORY_ALLOC);

    /* 4. Set varlena header */
    SET_VARSIZE(cd, total_len);

    /* 5. Set CipherDataType header */
    cd->version = TECC_FORMATTER_VERSION;
    cd->dek_dist_oid = InvalidOid;         /* DEK identity (caller must set if known) */
    cd->rnd_length = encrypted_lens[TECC_ALGO_RND];
    cd->det_length = encrypted_lens[TECC_ALGO_DET];
    cd->ope_length = encrypted_lens[TECC_ALGO_OPE];
    cd->search_length = encrypted_lens[TECC_ALGO_SEARCH];

    /* 6. Copy algorithm ciphertext data in fixed order */
    dest_ptr = CIPHERDATA_RND_PTR(cd);

    /* RND ciphertext */
    if (cd->rnd_length > 0) {
        memcpy(dest_ptr, encrypted_data[TECC_ALGO_RND], cd->rnd_length);
    }
    dest_ptr += cd->rnd_length;

    /* DET ciphertext */
    if (cd->det_length > 0) {
        memcpy(dest_ptr, encrypted_data[TECC_ALGO_DET], cd->det_length);
    }
    dest_ptr += cd->det_length;

    /* OPE ciphertext */
    if (cd->ope_length > 0) {
        memcpy(dest_ptr, encrypted_data[TECC_ALGO_OPE], cd->ope_length);
    }
    dest_ptr += cd->ope_length;

    /* SEARCH ciphertext */
    if (cd->search_length > 0) {
        memcpy(dest_ptr, encrypted_data[TECC_ALGO_SEARCH], cd->search_length);
    }

    *cipherdata = cd;
    *cipherdata_len = total_len;

    return TECC_FORMATTER_SUCCESS;
}

/*
 * tecc_cipherdata_parse - Parse CipherDataType to extract algorithm ciphertexts
 *
 * Parses a CipherDataType structure and extracts individual algorithm ciphertexts.
 */
TeccFormatterErrorCode
tecc_cipherdata_parse(
    const CipherDataType *cipherdata,
    size_t cipherdata_len,
    TeccParsedCipherdata *parsed_data)
{
    /* 1. Parameter validation */
    TECC_RETURN_IF_NULL(cipherdata, TECC_FORMATTER_ERROR_INVALID_PARAM);
    TECC_RETURN_IF_NULL(parsed_data, TECC_FORMATTER_ERROR_INVALID_PARAM);

    /* 2. Validate structure size */
    if (cipherdata_len < sizeof(CipherDataType))
        return TECC_FORMATTER_ERROR_INVALID_HEADER;

    /* 3. VARSIZE check skipped - frontend VARSIZE macro incompatible with backend encoding */
    /*    Length validation done via sizeof(CipherDataType) + algorithm lengths below */

    /* 4. Validate version */
    if (CIPHERDATA_VERSION(cipherdata) != TECC_FORMATTER_VERSION)
        return TECC_FORMATTER_ERROR_VERSION_MISMATCH;

    /* 5. Extract algorithm information */
    parsed_data->version = CIPHERDATA_VERSION(cipherdata);
    parsed_data->rnd_data_len = CIPHERDATA_RND_LENGTH(cipherdata);
    parsed_data->det_data_len = CIPHERDATA_DET_LENGTH(cipherdata);
    parsed_data->ope_data_len = CIPHERDATA_OPE_LENGTH(cipherdata);
    parsed_data->search_data_len = CIPHERDATA_SEARCH_LENGTH(cipherdata);

    /* 6. Set data pointers */
    parsed_data->rnd_data = (const unsigned char *)CIPHERDATA_RND_PTR(cipherdata);
    parsed_data->det_data = (const unsigned char *)CIPHERDATA_DET_PTR(cipherdata);
    parsed_data->ope_data = (const unsigned char *)CIPHERDATA_OPE_PTR(cipherdata);
    parsed_data->search_data = (const unsigned char *)CIPHERDATA_SEARCH_PTR(cipherdata);

    /* 7. Validate data boundaries */
    {
        size_t expected_size = sizeof(CipherDataType) +
                              parsed_data->rnd_data_len +
                              parsed_data->det_data_len +
                              parsed_data->ope_data_len +
                              parsed_data->search_data_len;

        if (expected_size != cipherdata_len)
            return TECC_FORMATTER_ERROR_DATA_FORMAT;
    }

    return TECC_FORMATTER_SUCCESS;
}

/*
 * tecc_cipherdata_validate - Validate CipherDataType structure
 *
 * Validates the integrity and format of a CipherDataType structure.
 */
TeccFormatterErrorCode
tecc_cipherdata_validate(
    const CipherDataType *cipherdata,
    size_t cipherdata_len)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    size_t data_size = 0;
    size_t expected_size = 0;
    const char *data_end = NULL;

    /* 1. Parameter validation */
    TECC_RETURN_IF_NULL(cipherdata, TECC_FORMATTER_ERROR_INVALID_PARAM);

    /* 2. Validate minimum size */
    if (cipherdata_len < sizeof(CipherDataType))
        return TECC_FORMATTER_ERROR_INVALID_HEADER;

    /* 3. VARSIZE check skipped - frontend VARSIZE macro incompatible with backend encoding */

    /* 4. Validate version */
    if (CIPHERDATA_VERSION(cipherdata) < TECC_FORMATTER_MIN_VERSION ||
        CIPHERDATA_VERSION(cipherdata) > TECC_FORMATTER_MAX_VERSION)
        return TECC_FORMATTER_ERROR_VERSION_MISMATCH;

    /* 5. Validate lengths */
    data_size = CIPHERDATA_RND_LENGTH(cipherdata) +
                      CIPHERDATA_DET_LENGTH(cipherdata) +
                      CIPHERDATA_OPE_LENGTH(cipherdata) +
                      CIPHERDATA_SEARCH_LENGTH(cipherdata);

    expected_size = sizeof(CipherDataType) + data_size;

    if (expected_size != cipherdata_len)
        return TECC_FORMATTER_ERROR_DATA_FORMAT;

    /* 6. Validate data pointers */
    if (data_size > 0) {
        data_end = CIPHERDATA_SEARCH_PTR(cipherdata) +
                              CIPHERDATA_SEARCH_LENGTH(cipherdata);

        if (data_end > (char *)cipherdata + cipherdata_len)
            return TECC_FORMATTER_ERROR_DATA_FORMAT;
    }

    return TECC_FORMATTER_SUCCESS;
}

/*
 * tecc_cipherdata_calculate_size - Calculate CipherDataType size requirements
 *
 * Calculates the total size needed for a CipherDataType structure given
 * the sizes of individual algorithm ciphertexts.
 */
size_t
tecc_cipherdata_calculate_size(
    size_t rnd_len,
    size_t det_len,
    size_t ope_len,
    size_t search_len)
{
    return sizeof(CipherDataType) + rnd_len + det_len + ope_len + search_len;
}

/* End - ReqID:TECC4.0 */