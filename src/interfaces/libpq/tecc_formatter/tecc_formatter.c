/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_formatter.c
 *
 * Abstract:
 *   Data formatter module implementation for TECC processor module.
 *   Provides data normalization, CipherDataType building, and hex encoding services.
 *   Implements unified interface between crypto scheduler and algorithm modules.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_formatter.c
 */

/* Begin - ReqID:TECC4.0 */

#include "tecc_formatter.h"
#include "tecc_formatter_internal.h"
#include "tecc_normalizer.h"
#include "common/shortest_dec.h"
#include "tecc_data_validator.h"
#include "tecc_icu_normalizer.h"
/* Note: tecc_formatter_guc.h has been removed, GUC parameter integration simplified */

#include <string.h>
#include <stdlib.h>  /* For malloc, free (deprecated - use pg_malloc/pg_free) */
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <errno.h>
#include <float.h>

/* PostgreSQL frontend memory utilities */
#include "common/fe_memutils.h"  /* For pg_malloc, pg_free */

/* Simple type definitions for client-side use */
typedef int32_t pg_int32;
typedef int16_t pg_int16;
typedef uint16_t pg_uint16;

/* TECC logging system */
#include "../tecc_common/tecc_logging.h"

/* Internal function prototypes */
static TeccFormatterErrorCode tecc_normalize_number_to_buffer(TeccCryptoContextCore *context,
                                                             const unsigned char *input_data,
                                                             size_t input_len,
                                                             TeccCryptoBuffer *buffer);
static TeccFormatterErrorCode tecc_normalize_text_to_buffer(TeccCryptoContextCore *context,
                                                           const unsigned char *input_data,
                                                           size_t input_len,
                                                           TeccCryptoBuffer *buffer);

/* GUC integration helper functions */
static TeccFormatterErrorCode tecc_apply_guc_to_text(TeccCryptoContextCore *context,
                                                    const unsigned char *input_data,
                                                    size_t input_len,
                                                    char **output_text,
                                                    size_t *output_len);
static TeccFormatterErrorCode tecc_apply_guc_to_numeric(TeccCryptoContextCore *context,
                                                       const char *numeric_str,
                                                       size_t numeric_len,
                                                       char **processed_numeric);

/* Text parsing helper functions for numeric normalization */
static TeccFormatterErrorCode parse_int_from_text(const char *text, size_t text_len, Oid data_type, int64_t *int_value);
static TeccFormatterErrorCode parse_float_from_text(const char *text, size_t text_len, Oid data_type, double *float_value);
/* ReqID:TECC4.0 - 256-byte hex lookup table, zero branches per nibble */
static const signed char hex_lut[256] = {
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	 0, 1, 2, 3, 4, 5, 6, 7,  8, 9,-1,-1,-1,-1,-1,-1,
	-1,10,11,12,13,14,15,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,10,11,12,13,14,15,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1
};

/* ReqID:TECC4.0 - LUT-based hex decode: single array access vs 6-branch chain */
static int hex_char_to_int(char c)
{
	return hex_lut[(unsigned char)c];
}

/* Numeric normalization helper functions */
static TeccFormatterErrorCode normalize_int_value(TeccCryptoContextCore *context,
                                                 int64_t int_value,
                                                 TeccNumericNormalized *norm_num);
static TeccFormatterErrorCode normalize_float_value(TeccCryptoContextCore *context,
                                                   double float_value,
                                                   TeccNumericNormalized *norm_num);
static TeccFormatterErrorCode normalize_string_number(TeccCryptoContextCore *context,
                                                     const char *str_value,
                                                     size_t str_len,
                                                     TeccNumericNormalized *norm_num);
static TeccFormatterErrorCode normalize_numeric_text(TeccCryptoContextCore *context,
                                                    const char *numeric_text,
                                                    size_t text_len,
                                                    TeccNumericNormalized *norm_num);
static TeccFormatterErrorCode encode_normalized_format_to_buffer(TeccNumericNormalized *norm_num,
                                                                unsigned char *buffer,
                                                                size_t buffer_size,
                                                                size_t *output_len);
/* Internal ICU normalization support */
static TeccFormatterErrorCode tecc_icu_generate_sortkey(const char *collation_locale,
                                                       const unsigned char *input_text,
                                                       size_t input_len,
                                                       unsigned char **sortkey,
                                                       size_t *sortkey_len);

/*
 * tecc_formatter_normalize_data - Data normalization processing (pre-encryption)
 * @deprecated Use tecc_formatter_normalize_data_zero_copy instead
 *
 * Based on data type, algorithm type and collation rules in encryption context,
 * normalize input data for encryption.
 * Special handling:
 * - Numeric types: Convert to OPE normalized format (sign+weight+ndigits+digits)
 * - Text types: Process according to algorithm type
 *   - RND: Add 4-byte length prefix
 *   - DET: Keep original format
 *   - OPE: Generate ICU sort key or keep original format based on collation_oid
 *   - SEARCH: Keep original format
 *
 * @param context Encryption context (contains type, algorithm, collation info)
 * @param input_data Input data pointer
 * @param input_len Input data length
 * @param output_data Output normalized data pointer (caller must free)
 * @param output_len Output data length
 *
 * @return TeccFormatterErrorCode Error code
 *
 * @note
 * - Output data allocated using context->mem_ctx
 * - For OPE algorithm and character types, generate ICU sort key if collation_oid valid
 * - Fall back to original text format if ICU sort key generation fails
 */
/* TeccFormatterErrorCode
 * tecc_formatter_normalize_data(TeccCryptoContextCore *context,
 *                               const unsigned char *input_data,
 *                               size_t input_len,
 *                               unsigned char **output_data,
 *                               size_t *output_len)
 * {
 *     // 1. Parameter validation
 *     if (!context || !input_data || !output_data || !output_len)
 *         return TECC_FORMATTER_ERROR_INVALID_PARAM;
 *
 *     if (input_len == 0 || input_len > TECC_MAX_DATA_SIZE)
 *         return TECC_FORMATTER_ERROR_DATA_TOO_LARGE;
 *
 *     // 2. Validate input data before normalization
 *     TeccFormatterErrorCode validation_result = tecc_validate_input_data(
 *         input_data, input_len, context->data_type, context->algo_type);
 *     if (validation_result != TECC_FORMATTER_SUCCESS) {
 *         return validation_result;
 *     }
 *
 *     // 3. Select normalizer based on data type
 *     switch (context->data_type) {
 *         // Integer types
 *         case INT2OID:
 *         case INT4OID:
 *         case INT8OID:
 *         // Floating point types
 *         case FLOAT4OID:
 *         case FLOAT8OID:
 *         // Numeric type
 *         case NUMERICOID:
 *             // Numeric type normalization
 *             return tecc_normalizer_normalize_numeric(context, input_data, input_len,
 *                                                    output_data, output_len);
 *
 *         // Text types
 *         case VARCHAROID:
 *         case BPCHAROID:
 *         case TEXTOID:
 *             // Text type normalization
 *             return tecc_normalizer_normalize_text(context, input_data, input_len,
 *                                                 output_data, output_len);
 *
 *         default:
 *             return TECC_FORMATTER_ERROR_INVALID_TYPE;
 *     }
 * } */

/*
 * tecc_formatter_normalize_data - Data normalization with buffer (zero-copy)
 *
 * Normalizes data using provided buffer for output.
 * Supports both separate input and in-place operation.
 *
 * @param context Encryption context
 * @param input_data Input data to normalize
 * @param input_len Input data length
 * @param buffer Output buffer for normalized data
 *
 * @return TeccFormatterErrorCode Error code
 */
TeccFormatterErrorCode
tecc_formatter_normalize_data(TeccCryptoContextCore *context,
                             const unsigned char *input_data,
                             size_t input_len,
                             TeccCryptoBuffer *buffer)
{
    /* 1. Parameter validation */
    if (!context || !input_data || !buffer || !buffer->data)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    /* 2. Validate input data before normalization */
    {
        TeccFormatterErrorCode validation_result;
        if (input_len == 0) {
            /* ReqID:TECC4.0 - Empty string: produce a canonical 16-byte zero
             * block. All three text types (bpchar, varchar, text) use rtrim'd
             * data for the encrypted core, so pure trailing-space values produce
             * the same empty-string core. The space_count in the ciphertext
             * format records the original trailing-space count for backend
             * comparison functions (bpchar ignores, text/varchar compares). */
            if (buffer && buffer->data)
            {
                if (buffer->size >= 16)
                {
                    memset(buffer->data, 0, 16);
                    buffer->used = 16;
                    return TECC_FORMATTER_SUCCESS;
                }
                buffer->used = 0;
                return TECC_FORMATTER_SUCCESS;
            }
            return TECC_FORMATTER_ERROR_INVALID_PARAM;
        }
        validation_result = tecc_validate_input_data(
            input_data, input_len, context->data_type, context->algo_type);
        if (validation_result != TECC_FORMATTER_SUCCESS) {
            return validation_result;
        }
    }

    /* 3. Select normalizer based on data type */
    switch (context->data_type) {
        /* Integer types */
        case INT2OID:
        case INT4OID:
        case INT8OID:
        /* Floating point types */
        case FLOAT4OID:
        case FLOAT8OID:
        /* Numeric type */
        case NUMERICOID:
            /* Numeric type normalization with buffer */
            return tecc_normalize_number_to_buffer(context, input_data, input_len, buffer);

        /* Text types */
        case VARCHAROID:
        case BPCHAROID:
        case TEXTOID:
            /* Text type normalization with buffer */
            return tecc_normalize_text_to_buffer(context, input_data, input_len, buffer);

        default:
            return TECC_FORMATTER_ERROR_INVALID_TYPE;
    }
}

/*
 * tecc_formatter_build_cipherdata - Build CipherDataType structure
 * @deprecated Use tecc_formatter_build_cipherdata_zero_copy instead
 *
 * Build CipherDataType structure compatible with PostgreSQL backend type system:
 * - Set varlena header (vl_len_)
 * - Set version number and algorithm ciphertext lengths
 * - Store in order: RND ciphertext → DET ciphertext → OPE ciphertext → SEARCH ciphertext
 *
 * @param algo_type Main algorithm type (for version control)
 * @param data_type Data type OID (PostgreSQL type OID)
 * @param encrypted_data Encrypted data array (algorithm results)
 * @param encrypted_lens Algorithm data lengths
 * @param cipherdata Output CipherDataType pointer (varlena format)
 * @param cipherdata_len Output data length
 *
 * @return TeccFormatterErrorCode Error code
 *
 * @note
 * - Output structure must be fully compatible with backend pg_type definition
 * - Use SET_VARSIZE macro for varlena header
 * - All algorithm ciphertexts stored, even if some lengths are 0
 */
/* TeccFormatterErrorCode
 * tecc_formatter_build_cipherdata(TeccAlgorithmType algo_type,
 *                                Oid data_type,
 *                                unsigned char **encrypted_data,
 *                                size_t *encrypted_lens,
 *                                CipherDataType **cipherdata,
 *                                size_t *cipherdata_len)
 * {
 *     size_t total_len;
 *     CipherDataType *cd;
 *     char *dest_ptr;
 *
 *     // 1. Parameter validation
 *     if (!encrypted_data || !encrypted_lens || !cipherdata || !cipherdata_len)
 *         return TECC_FORMATTER_ERROR_INVALID_PARAM;
 *
 *     // 2. Calculate total length
 *     total_len = sizeof(CipherDataType) +
 *                 encrypted_lens[TECC_ALGO_RND] +
 *                 encrypted_lens[TECC_ALGO_DET] +
 *                 encrypted_lens[TECC_ALGO_OPE] +
 *                 encrypted_lens[TECC_ALGO_SEARCH];
 *
 *     // 3. Allocate memory using formatter memory context
 *     cd = (CipherDataType *)pg_malloc0(total_len);
 *     if (!cd)
 *         return TECC_FORMATTER_ERROR_MEMORY_ALLOC;
 *
 *     // 4. Set varlena header
 *     SET_VARSIZE(cd, total_len);
 *
 *     // 5. Set CipherDataType header
 *     cd->version = 1;  // Current version number
 *     cd->rnd_length = encrypted_lens[TECC_ALGO_RND];
 *     cd->det_length = encrypted_lens[TECC_ALGO_DET];
 *     cd->ope_length = encrypted_lens[TECC_ALGO_OPE];
 *     cd->search_length = encrypted_lens[TECC_ALGO_SEARCH];
 *
 *     // 6. Copy algorithm ciphertext data (in fixed order)
 *     dest_ptr = CIPHERDATA_RND_PTR(cd);
 *
 *     // RND ciphertext
 *     if (cd->rnd_length > 0) {
 *         memcpy(dest_ptr, encrypted_data[TECC_ALGO_RND], cd->rnd_length);
 *     }
 *     dest_ptr += cd->rnd_length;
 *
 *     // DET ciphertext
 *     if (cd->det_length > 0) {
 *         memcpy(dest_ptr, encrypted_data[TECC_ALGO_DET], cd->det_length);
 *     }
 *     dest_ptr += cd->det_length;
 *
 *     // OPE ciphertext
 *     if (cd->ope_length > 0) {
 *         memcpy(dest_ptr, encrypted_data[TECC_ALGO_OPE], cd->ope_length);
 *     }
 *     dest_ptr += cd->ope_length;
 *
 *     // SEARCH ciphertext
 *     if (cd->search_length > 0) {
 *         memcpy(dest_ptr, encrypted_data[TECC_ALGO_SEARCH], cd->search_length);
 *     }
 *
 *     *cipherdata = cd;
 *     *cipherdata_len = total_len;
 *
 *     return TECC_FORMATTER_SUCCESS;
 * } */

/*
 * tecc_formatter_parse_cipherdata - Parse CipherDataType structure
 *
 * Parse CipherDataType structure to extract algorithm ciphertexts.
 * Must be compatible with backend CipherDataType format.
 *
 * @param cipherdata CipherDataType structure to parse
 * @param cipherdata_len CipherDataType structure length
 * @param parsed_data Output parsed data structure
 *
 * @return TeccFormatterErrorCode Error code
 */
TeccFormatterErrorCode
tecc_formatter_parse_cipherdata(const CipherDataType *cipherdata,
                                size_t cipherdata_len,
                                TeccParsedCipherdata *parsed_data)
{
    /* 1. Parameter validation */
    size_t total_cipher_len;

    if (!cipherdata || !parsed_data)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    /* 2. Verify minimum size — use v1.0 minimum (28) for upgrade compat */
    /* ReqID:TECC4.0 - 最小长度按 v2.0 空 mask（仅 base 头）计算，
     * v1.0 的固定 28B 头更大，会在下方长度校验中被精确核对。 */
    if (cipherdata_len < CIPHERDATA_MIN_SIZE)
        return TECC_FORMATTER_ERROR_INVALID_HEADER;

    /* Note: VARSIZE check skipped here because frontend VARSIZE macro
     * is incompatible with backend SET_VARSIZE encoding.  Length validation
     * is done below via sum of header + algorithm lengths. */

    /* 3. Verify version (major only; low 16 bits carry the segment mask) */
    if (!CIPHERDATA_VERSION_VALID(CIPHERDATA_VERSION(cipherdata)))
        return TECC_FORMATTER_ERROR_VERSION_MISMATCH;

    /* 4. Extract algorithm ciphertexts (mask-aware: absent segments read 0) */
    parsed_data->version = CIPHERDATA_VERSION(cipherdata);
    parsed_data->rnd_data_len = CIPHERDATA_RND_LENGTH(cipherdata);
    parsed_data->det_data_len = CIPHERDATA_DET_LENGTH(cipherdata);
    parsed_data->ope_data_len = CIPHERDATA_OPE_LENGTH(cipherdata);
    parsed_data->search_data_len = CIPHERDATA_SEARCH_LENGTH(cipherdata);

    /* ReqID:TECC4.0 - Extract HE ciphertext lengths (absent for v1.0) */
    parsed_data->eg_add_data_len = CIPHERDATA_EG_ADD_LENGTH(cipherdata);
    parsed_data->eg_mul_data_len = CIPHERDATA_EG_MUL_LENGTH(cipherdata);

    /* 5. Set pointers to algorithm data */
    parsed_data->rnd_data = (const unsigned char *)CIPHERDATA_RND_PTR(cipherdata);
    parsed_data->det_data = (const unsigned char *)CIPHERDATA_DET_PTR(cipherdata);
    parsed_data->ope_data = (const unsigned char *)CIPHERDATA_OPE_PTR(cipherdata);
    parsed_data->search_data = (const unsigned char *)CIPHERDATA_SEARCH_PTR(cipherdata);

    /* ReqID:TECC4.0 - Set HE data pointers (NULL when the segment is absent) */
    {
        if (parsed_data->eg_add_data_len > 0)
        {
            parsed_data->eg_add_data = (const unsigned char *)CIPHERDATA_EG_ADD_PTR(cipherdata);
            /* Per-segment DEK distributed ID from the 4-byte prefix */
            if (parsed_data->eg_add_data_len >= CIPHERDATA_HE_DEK_ID_SIZE)
                parsed_data->eg_add_dek_id = *(Oid *)parsed_data->eg_add_data;
            else
                parsed_data->eg_add_dek_id = InvalidOid;
        }
        else
        {
            parsed_data->eg_add_data = NULL;
            parsed_data->eg_add_dek_id = InvalidOid;
        }

        if (parsed_data->eg_mul_data_len > 0)
        {
            parsed_data->eg_mul_data = (const unsigned char *)CIPHERDATA_EG_MUL_PTR(cipherdata);
            if (parsed_data->eg_mul_data_len >= CIPHERDATA_HE_DEK_ID_SIZE)
                parsed_data->eg_mul_dek_id = *(Oid *)parsed_data->eg_mul_data;
            else
                parsed_data->eg_mul_dek_id = InvalidOid;
        }
        else
        {
            parsed_data->eg_mul_data = NULL;
            parsed_data->eg_mul_dek_id = InvalidOid;
        }
    }

    /* 6. Verify data boundaries — mask-aware header size (includes vl_len_) */
    {
        size_t hdr_sz = CIPHERDATA_HEADER_SIZE(CIPHERDATA_RAW_VERSION(cipherdata));
        total_cipher_len = hdr_sz +
                       parsed_data->rnd_data_len +
                       parsed_data->det_data_len +
                       parsed_data->ope_data_len +
                       parsed_data->search_data_len +
                       parsed_data->eg_add_data_len +
                       parsed_data->eg_mul_data_len;
    }

    if (total_cipher_len != cipherdata_len)
        return TECC_FORMATTER_ERROR_DATA_FORMAT;

    return TECC_FORMATTER_SUCCESS;
}

/*
 * tecc_formatter_hex_encode - Binary data to hexadecimal string
 *
 * Convert binary data to hexadecimal string representation, compatible with
 * backend encrypted type output function format:
 * - Each byte converted to 2 hexadecimal characters
 * - Optional "\\x" prefix (PostgreSQL hex format)
 * - Use lowercase letters for a-f
 * - Add string terminator '\0'
 *
 * @param binary_data Binary data (CipherDataType structure)
 * @param binary_len Binary data length
 * @param hex_string Output hexadecimal string (caller must free)
 *
 * @return TeccFormatterErrorCode Error code
 *
 * @note
 * - Output format compatible with backend <type>_enc_out function
 * - Supports standard PostgreSQL hexadecimal string format
 * - Caller responsible for freeing returned string memory
 */
TeccFormatterErrorCode
tecc_formatter_hex_encode(const unsigned char *binary_data,
                         size_t binary_len,
                         char **hex_string)
{
    /* 1. Parameter validation */
    if (!binary_data || !hex_string)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    if (binary_len == 0 )
        return TECC_FORMATTER_ERROR_DATA_TOO_LARGE;

    /* 2. Allocate output buffer (optional \\x prefix + 2x length + terminator) */
    *hex_string = pg_malloc(binary_len * 2 + 3);  /* +3 for "\\x" and '\0' */
    if (!*hex_string)
        return TECC_FORMATTER_ERROR_MEMORY_ALLOC;

    /* 3. Add PostgreSQL hexadecimal prefix and perform conversion */
    {
        char *p = *hex_string;
        const char hex_digits[] = "0123456789abcdef";
        size_t i;

        *p++ = '\\';
        *p++ = 'x';

        /* Perform hexadecimal conversion */
        for (i = 0; i < binary_len; i++) {
            *p++ = hex_digits[binary_data[i] >> 4];   /* High 4 bits */
            *p++ = hex_digits[binary_data[i] & 0x0F]; /* Low 4 bits */
        }

        *p = '\0';  /* String terminator */
    }

    return TECC_FORMATTER_SUCCESS;
}

/*
 * tecc_formatter_hex_decode - Hexadecimal string to binary data
 *
 * Convert hexadecimal string to binary data, compatible with backend
 * encrypted type input function format:
 * - Support PostgreSQL hexadecimal string format (optional "\\x" prefix)
 * - Support uppercase/lowercase letters
 * - Validate string format (even length, valid characters)
 * - Handle format errors and invalid input
 *
 * @param hex_string Hexadecimal string (may contain "\\x" prefix)
 * @param binary_data Output binary data (caller must free)
 * @param binary_len Output data length
 *
 * @return TeccFormatterErrorCode Error code
 *
 * @note
 * - Input format compatible with backend <type>_enc_in function
 * - Supports standard PostgreSQL hexadecimal string format
 * - Caller responsible for freeing returned binary data memory
 */
TeccFormatterErrorCode
tecc_formatter_hex_decode(const char *hex_string,
                         unsigned char **binary_data,
                         size_t *binary_len)
{
    /* 1. Parameter validation */
    size_t hex_len;
    const char *p;
    size_t i;

    if (!hex_string || !binary_data || !binary_len)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    hex_len = strlen(hex_string);
    if (hex_len == 0)
        return TECC_FORMATTER_ERROR_DATA_FORMAT;

    /* 2. Skip optional PostgreSQL hexadecimal prefix */
    p = hex_string;
    if (hex_len >= 2 && p[0] == '\\' && p[1] == 'x') {
        p += 2;
        hex_len -= 2;
    }

    /* 3. Validate length (must be even) */
    if (hex_len % 2 != 0)
        return TECC_FORMATTER_ERROR_DATA_FORMAT;

    /* 4. Check decoded binary data size */
    *binary_len = hex_len / 2;
    if (*binary_len == 0)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    /* 5. Allocate output buffer */
    *binary_data = pg_malloc(*binary_len);
    if (!*binary_data)
        return TECC_FORMATTER_ERROR_MEMORY_ALLOC;

    /* 6. Perform conversion */
    for (i = 0; i < *binary_len; i++) {
        int high;
        int low;

        /* Convert high 4 bits */
        high = hex_char_to_int(p[i * 2]);
        if (high < 0) {
            pg_free(*binary_data);
            *binary_data = NULL;
            return TECC_FORMATTER_ERROR_DATA_FORMAT;
        }

        /* Convert low 4 bits */
        low = hex_char_to_int(p[i * 2 + 1]);
        if (low < 0) {
            pg_free(*binary_data);
            *binary_data = NULL;
            return TECC_FORMATTER_ERROR_DATA_FORMAT;
        }

        (*binary_data)[i] = (high << 4) | low;
    }

    return TECC_FORMATTER_SUCCESS;
}

/* Begin - ReqID:TECC4.0 */
/*
 * Batch hex decode — decodes multiple hex strings at once,
 * pre-allocating one contiguous buffer. Each binary_data[i] points
 * into this buffer; caller frees binary_data[0] (the base pointer).
 *
 * Returns success only if ALL items decode successfully.
 */
TeccFormatterErrorCode
tecc_formatter_batch_hex_decode(
    const char **hex_strings,
    size_t count,
    unsigned char **binary_data,
    size_t *binary_lens)
{
    size_t total_binary_len;
    unsigned char *buffer;
    unsigned char *cursor;
    size_t i;

    if (!hex_strings || count == 0 || !binary_data || !binary_lens)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    /* First pass: validate all strings and compute total size */
    total_binary_len = 0;
    for (i = 0; i < count; i++)
    {
        const char *p;
        size_t hex_len;
        size_t bin_len;

        if (!hex_strings[i])
            return TECC_FORMATTER_ERROR_INVALID_PARAM;

        p = hex_strings[i];
        hex_len = strlen(p);

        if (hex_len == 0)
            return TECC_FORMATTER_ERROR_DATA_FORMAT;

        /* Skip optional \x prefix */
        if (hex_len >= 2 && p[0] == '\\' && p[1] == 'x')
        {
            hex_len -= 2;
        }

        if (hex_len % 2 != 0)
            return TECC_FORMATTER_ERROR_DATA_FORMAT;

        bin_len = hex_len / 2;
        if (bin_len == 0)
            return TECC_FORMATTER_ERROR_INVALID_PARAM;

        binary_lens[i] = bin_len;
        total_binary_len += bin_len;
    }

    /* Allocate one contiguous buffer for all binary data */
    buffer = pg_malloc(total_binary_len);
    if (!buffer)
        return TECC_FORMATTER_ERROR_MEMORY_ALLOC;

    /* Second pass: decode each hex string into the buffer
     * ReqID:TECC4.0 - no strlen needed; binary_lens[i] from pass 1 is enough.
     * hex \\x prefix handled by simple char check. */
    cursor = buffer;
    for (i = 0; i < count; i++)
    {
        const char *p;
        size_t j;

        p = hex_strings[i];
        if (p[0] == '\\' && p[1] == 'x')
            p += 2;

        /* Guard: cursor must not exceed allocated buffer */
        if (cursor + binary_lens[i] > buffer + total_binary_len)
        {
            TECC_LOG(TECC_LOG_ERROR,
                     "TECC: hex decode cursor overflow: item=%zu/%zu, "
                     "cursor=%td, need=%zu, total=%zu",
                     i, count, cursor - buffer,
                     binary_lens[i], total_binary_len);
            pg_free(buffer);
            return TECC_FORMATTER_ERROR_DATA_TOO_LARGE;
        }

        binary_data[i] = cursor;

        for (j = 0; j < binary_lens[i]; j++)
        {
            int high;
            int low;

            high = hex_char_to_int(p[j * 2]);
            if (high < 0)
            {
                pg_free(buffer);
                return TECC_FORMATTER_ERROR_DATA_FORMAT;
            }

            low = hex_char_to_int(p[j * 2 + 1]);
            if (low < 0)
            {
                pg_free(buffer);
                return TECC_FORMATTER_ERROR_DATA_FORMAT;
            }

            cursor[j] = (unsigned char)((high << 4) | low);
        }

        cursor += binary_lens[i];
    }

    return TECC_FORMATTER_SUCCESS;
}
/* End - ReqID:TECC4.0 */

/*
 * tecc_formatter_allocate_buffer - Buffer allocation with unified memory management
 *
 * Allocate buffer from context-associated memory pool.
 *
 * @param context Encryption context (must contain valid mem_pool)
 * @param buffer_type Buffer type
 * @param size_needed Required buffer size
 * @param buffer_desc Output buffer descriptor
 *
 * @return TeccFormatterErrorCode Error code
 */
TeccFormatterErrorCode
tecc_formatter_allocate_buffer(TeccCryptoContextCore *context,
                              TeccBufferType buffer_type,
                              size_t size_needed,
                              TeccBufferDesc **buffer_desc)
{
    void *buffer;

    if (!context || !buffer_desc)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    /* Allocate buffer directly */
    buffer = pg_malloc(size_needed);
    if (!buffer)
        return TECC_FORMATTER_ERROR_MEMORY_ALLOC;

    /* Fill buffer descriptor */
    *buffer_desc = &context->buffers[buffer_type];
    (*buffer_desc)->type = buffer_type;
    (*buffer_desc)->data = buffer;
    (*buffer_desc)->size = size_needed;
    (*buffer_desc)->capacity = size_needed;
    (*buffer_desc)->in_use = true;

    return TECC_FORMATTER_SUCCESS;
}

/*
 * tecc_formatter_calculate_normalize_size - Calculate buffer size for normalization
 *
 * Calculate buffer size requirements for normalization operation.
 *
 * @param context Encryption context
 * @param input_size Input data size
 * @param output_size_needed Output buffer requirement
 * @param work_size_needed Work buffer requirement
 *
 * @return TeccFormatterErrorCode Error code
 */
TeccFormatterErrorCode
tecc_formatter_calculate_normalize_size(
    TeccCryptoContextCore *context,
    size_t input_size,
    size_t *output_size_needed,
    size_t *work_size_needed)
{
    size_t base_output_size;

    if (!context || !output_size_needed || !work_size_needed)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    /* Base output size is input size plus overhead */
    base_output_size = input_size;

    /* Additional requirements based on data type and algorithm */
    switch (context->data_type) {
        case INT2OID:
        case INT4OID:
        case INT8OID:
        case FLOAT4OID:
        case FLOAT8OID:
        case NUMERICOID:
            /* ReqID:TECC4.0 - When OPE is supported, plaintext is padded
             * to a multiple of TECC_NUMERIC_OPE_CHUNK_SIZE (12) so all
             * OPE chunks are the same size and enc_bytes_internal uses
             * consistent bit counts for cross-type memcmp ordering.
             * Upper bound: round_up(header + max_digits*2, 12). */
            if (context->algo_support.supports_ope) {
                size_t max_digits;
                switch (context->data_type) {
                    case INT2OID:  max_digits = 6;  break;  /* 32767 */
                    case INT4OID:  max_digits = 11; break;  /* 2147483647 */
                    case INT8OID:  max_digits = 20; break;  /* 9e18 */
                    case FLOAT4OID:
                    case FLOAT8OID: max_digits = 40; break;  /* generous */
                    case NUMERICOID: max_digits = TECC_NUMERIC_MAX_DIGITS; break;
                    default:       max_digits = TECC_NUMERIC_MAX_DIGITS; break;
                }
                {
                    /* ReqID:TECC4.0 - OPE padding 发生在 dscale 之前:
                     * tecc_normalize_number_to_buffer 先对
                     * (header + ndigits*2) 做 OPE chunk 补齐, 然后
                     * encode_normalized_format_to_buffer 在末尾追加 dscale。
                     * 所以 max 尺寸 = round_up(header+max_digits*2, 12) + dscale
                     * Bug 8178: 之前公式把 dscale 加到 padding 之前导致 int2
                     * (-32768) 归一化 buffer 太小, 退化为加密原始文本。 */
                    size_t pre_pad = TECC_NUMERIC_HEADER_SIZE
                                   + max_digits * sizeof(uint16_t);
                    size_t padded = ((pre_pad + TECC_NUMERIC_OPE_CHUNK_SIZE - 1)
                                    / TECC_NUMERIC_OPE_CHUNK_SIZE)
                                   * TECC_NUMERIC_OPE_CHUNK_SIZE;
                    *output_size_needed = padded + sizeof(uint16_t);
                }
            } else {
                *output_size_needed = base_output_size + 48;
            }
            *work_size_needed = input_size * 2;
            break;

        case VARCHAROID:
        case BPCHAROID:
        case TEXTOID:
            /* Text types */
            if (context->algo_support.supports_ope) {
                /* ReqID:TECC4.0 - When OPE is supported, plaintext is padded
                 * to a multiple of TECC_TEXT_OPE_CHUNK_SIZE (12) so all
                 * OPE chunks use the same msglen in enc_bytes_internal.
                 * Estimate: input_size * ICU multiplier, aligned to 12.
                 * Note: for collated text, also apply MIN_ICU_SORTKEY_SIZE
                 * to stay consistent with tecc_normalize_text_to_buffer. */
                size_t est = input_size * TECC_TEXT_MAX_ICU_SORTKEY_MULTIPLIER;
                if (est < TECC_TEXT_MIN_ICU_SORTKEY_SIZE)
                    est = TECC_TEXT_MIN_ICU_SORTKEY_SIZE;
                *output_size_needed = ((est + TECC_TEXT_OPE_CHUNK_SIZE - 1)
                                      / TECC_TEXT_OPE_CHUNK_SIZE)
                                     * TECC_TEXT_OPE_CHUNK_SIZE;
                *work_size_needed = *output_size_needed + input_size;
            } else if (context->collation_locale != NULL && context->collation_locale[0] != '\0') {
                /* ICU sort key: use same multiplier and minimum as normalize function */
                *output_size_needed = input_size * TECC_TEXT_MAX_ICU_SORTKEY_MULTIPLIER;
                if (*output_size_needed < TECC_TEXT_MIN_ICU_SORTKEY_SIZE)
                    *output_size_needed = TECC_TEXT_MIN_ICU_SORTKEY_SIZE;
                *work_size_needed = input_size * 3;  /* ICU work buffer */
            } else if (context->selected_algorithms & TECC_ALGO_RND) {
                /* RND: add 4-byte length prefix */
                *output_size_needed = input_size + 4;
                *work_size_needed = input_size;  /* Minimal work buffer */
            } else {
                /* Other algorithms: keep original */
                *output_size_needed = input_size;
                *work_size_needed = input_size;  /* Minimal work buffer */
            }
            break;

        default:
            *output_size_needed = input_size;
            *work_size_needed = input_size;
            break;
    }

    return TECC_FORMATTER_SUCCESS;
}

/*
 * tecc_formatter_error_to_scheduler_error - Convert formatter error to scheduler error
 *
 * Map formatter module error codes to scheduler module error codes.
 *
 * @param fmt_error Formatter error code
 * @return Scheduler error code
 */
int
tecc_formatter_error_to_scheduler_error(TeccFormatterErrorCode fmt_error)
{
    if (fmt_error == TECC_FORMATTER_SUCCESS)
        return 0;  /* TECC_CRYPTO_SUCCESS */

    /* Map formatter error ranges to scheduler error codes */
    if (fmt_error >= -1000 && fmt_error <= -1100)  /* Parameter errors */
        return -1;  /* TECC_CRYPTO_INVALID_PARAM */

    if (fmt_error >= -1101 && fmt_error <= -1200)  /* Memory errors */
        return -2;  /* TECC_CRYPTO_MEMORY_ERROR */

    if (fmt_error >= -1201 && fmt_error <= -1300)  /* Data errors */
        return -3;  /* TECC_CRYPTO_DATA_ERROR */

    if (fmt_error >= -1301 && fmt_error <= -1400)  /* Format errors */
        return -4;  /* TECC_CRYPTO_FORMAT_ERROR */

    if (fmt_error >= -1401 && fmt_error <= -1500)  /* Collation errors */
        return -5;  /* TECC_CRYPTO_COLLATION_ERROR */

    /* Default to system error */
    return -6;  /* TECC_CRYPTO_SYSTEM_ERROR */
}

/*
 * tecc_formatter_calculate_normalized_size - Calculate normalized data size
 *
 * Calculates the size needed for normalized data output.
 * Used for pre-allocating buffers.
 *
 * @param context Encryption context
 * @param input_len Input data length
 * @return Required buffer size for normalized output
 */
size_t
tecc_formatter_calculate_normalized_size(
    TeccCryptoContextCore *context,
    size_t input_len)
{
    size_t output_size_needed = 0;
    size_t work_size_needed = 0;

    if (!context || input_len == 0)
    {
        /* ReqID:TECC4.0 - Empty string returns 16 bytes to accommodate
         * the canonical 16-byte zero block for empty-string cores. */
        if (context != NULL)
            return 16;
        return 0;
    }

    /* Use existing calculate_normalize_size function */
    if (tecc_formatter_calculate_normalize_size(context, input_len,
                                                &output_size_needed, &work_size_needed)
        != TECC_FORMATTER_SUCCESS) {
        return 0;
    }

    return output_size_needed;
}

/*
 * tecc_formatter_initialize - Initialize formatter module
 *
 * Optional initialization function for formatter module.
 * Initializes ICU normalizer.
 */
void
tecc_formatter_initialize(void)
{
    /* Initialize ICU normalizer - ICU is mandatory for TECC formatter */
    tecc_icu_normalizer_init();
}

/*
 * tecc_formatter_cleanup - Cleanup formatter module
 *
 * Optional cleanup function for formatter module.
 * Currently a placeholder for future expansion.
 */
void
tecc_formatter_cleanup(void)
{
    /* Nothing to cleanup currently */
}

/* ========== Internal Implementation Functions ========== */

/* Begin - ReqID:TECC4.0 */
/*
 * tecc_normalize_number - Numeric type normalization
 *
 * Convert PostgreSQL numeric types to OPE normalized format:
 * - Extract sign bit (1 byte)
 * - Extract weight (2 bytes, big-endian)
 * - Extract digit array (each 2 bytes represents 4 decimal digits)
 *
 * OPE Normalized Format (sign+weight+ndigits+digits):
 * [sign:1字节][weight:2字节大端序][ndigits:2字节大端序][digits:变长]
 *
 * @param context Encryption context
 * @param input_data Input numeric data (PostgreSQL internal format)
 * @param input_len Input length
 * @param output_data Output normalized data
 * @param output_len Output length
 */
/*
 * tecc_normalize_number_to_buffer - Numeric type normalization with buffer
 *
 * Convert PostgreSQL numeric types to OPE normalized format using provided buffer.
 * Supports in-place operation when buffer contains input data.
 *
 * @param context Encryption context
 * @param input_data Input numeric data (PostgreSQL internal format)
 * @param input_len Input length
 * @param buffer Output buffer for normalized data
 */
static TeccFormatterErrorCode
tecc_normalize_number_to_buffer(TeccCryptoContextCore *context,
                               const unsigned char *input_data,
                               size_t input_len,
                               TeccCryptoBuffer *buffer)
{
    TeccFormatterErrorCode ret = TECC_FORMATTER_SUCCESS;
    TeccNumericNormalized norm_num;
    Oid data_type = context->data_type;

    /* Validate parameters */
    if (!context || !input_data || !buffer || !buffer->data)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    if (input_len == 0)
        return TECC_FORMATTER_ERROR_INVALID_INPUT;

    /* Initialize normalized structure */
    memset(&norm_num, 0, sizeof(TeccNumericNormalized));

    /* Convert input data based on data type */
    /* ReqID:TECC4.0 - All numeric types go through normalize_string_number
     * to ensure consistent normalization for cross-type DET equality
     * and OPE ordering comparison. */
    switch (data_type)
    {
        case INT2OID:
        case INT4OID:
        case INT8OID:
        case FLOAT4OID:
        case FLOAT8OID:
        case NUMERICOID:
            /* All numeric types: normalize via string representation */
            ret = normalize_string_number(context,
                                         (const char *)input_data,
                                         input_len,
                                         &norm_num);
            break;

        default:
            /* Unsupported numeric type */
            return TECC_FORMATTER_ERROR_UNSUPPORTED_TYPE;
    }

    if (ret != TECC_FORMATTER_SUCCESS)
        return ret;

    /* ReqID:TECC4.0 - For OPE, pad digits with trailing zeros so
     * the encoded size is a multiple of TECC_NUMERIC_OPE_CHUNK_SIZE (12).
     * This ensures every OPE chunk passed to enc_bytes_internal is exactly
     * 12 bytes, so the embedded bit count (8*12-1=95) is consistent across
     * all values. Without padding, values like 5.0 (encoded 8 bytes) and
     * 1.5 (encoded 10 bytes) would produce incomparable ciphertexts.
     * Trailing zeros are value-neutral (stripped during normalization). */
    if (context->algo_support.supports_ope &&
        context->algo_type == TECC_ALGO_OPE)
    {
        size_t raw_size = TECC_NUMERIC_HEADER_SIZE
                        + (size_t)norm_num.ndigits * sizeof(uint16_t);
        size_t rem = raw_size % TECC_NUMERIC_OPE_CHUNK_SIZE;
        if (rem != 0)
        {
            size_t extra_bytes = TECC_NUMERIC_OPE_CHUNK_SIZE - rem;
            size_t extra_digits = extra_bytes / sizeof(uint16_t);
            int pad_i;
            for (pad_i = 0; pad_i < (int)extra_digits; pad_i++)
                norm_num.digits[norm_num.ndigits + pad_i] = 0;
            norm_num.ndigits += (int)extra_digits;
        }
    }

    /* Encode normalized structure directly to buffer (zero-copy) */
    {
        size_t encoded_len = 0;

        ret = encode_normalized_format_to_buffer(&norm_num,
                                                (unsigned char *)buffer->data,
                                                buffer->size,
                                                &encoded_len);
        if (ret != TECC_FORMATTER_SUCCESS)
            return ret;

        /* Update buffer used size */
        buffer->used = encoded_len;
    }
    return TECC_FORMATTER_SUCCESS;
}

/*
 * tecc_normalize_text - Text type normalization
 *
 * Process text differently based on algorithm type:
 * - RND: Add 4-byte network byte order length prefix
 * - DET/OPE/SEARCH: Keep original format
 * - OPE+collation: Generate ICU sort key
 *
 * @param context Encryption context (contains collation_oid)
 * @param input_data Input text data
 * @param input_len Input text length
 * @param output_data Output normalized data
 * @param output_len Output length
 */
/*
 * hex_char_to_int - Convert hex character to integer value
 */

/*
 * normalize_int_value - Normalize integer value for OPE algorithm
 *
 * Converts integer value to normalized numeric format for OPE encryption.
 * The normalized format includes sign, weight, and digit representation.
 *
 * @param context Encryption context (not used for integer normalization)
 * @param int_value Integer value to normalize
 * @param norm_num Output normalized numeric structure
 */
static TeccFormatterErrorCode
normalize_int_value(TeccCryptoContextCore *context,
                   int64_t int_value,
                   TeccNumericNormalized *norm_num)
{
    int64_t temp;
    int digit_count;
    int16_t weight;

    /* Validate parameters */
    if (!norm_num)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    /* Initialize normalized structure */
    memset(norm_num, 0, sizeof(TeccNumericNormalized));

    /* Handle special cases */
    if (int_value == 0) {
        norm_num->sign = TECC_NUMERIC_SIGN_ZERO;
        norm_num->weight = 0;
        norm_num->ndigits = 1;
        norm_num->digits[0] = 0;
        return TECC_FORMATTER_SUCCESS;
    }

    /* Determine sign */
    if (int_value > 0) {
        norm_num->sign = TECC_NUMERIC_SIGN_POSITIVE;
    } else {
        norm_num->sign = TECC_NUMERIC_SIGN_NEGATIVE;
        int_value = -int_value;  /* Work with absolute value */
    }

    /* Convert integer to decimal digits */
    temp = int_value;
    digit_count = 0;
    weight = 0;

    /* Count digits and determine weight */
    while (temp > 0) {
        if (digit_count >= TECC_NUMERIC_MAX_DIGITS) {
            return TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW;
        }

        /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
        {
        uint16_t digit = temp % 10;
        norm_num->digits[digit_count] = digit;
        digit_count++;
        temp /= 10;
        }  /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    }

    /* Weight is (digit_count - 1) for integers */
    weight = digit_count - 1;

    /* Check weight range */
    if (weight < TECC_NUMERIC_MIN_WEIGHT || weight > TECC_NUMERIC_MAX_WEIGHT) {
        return TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW;
    }

    /* Store normalized values */
    norm_num->weight = weight;
    norm_num->ndigits = digit_count;

    /* Digits are stored in reverse order (most significant first) */
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    {
    int i, j;
    for (i = 0, j = digit_count - 1; i < j; i++, j--) {
        uint16_t temp_digit = norm_num->digits[i];
        norm_num->digits[i] = norm_num->digits[j];
        norm_num->digits[j] = temp_digit;
    }
    }  /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */

    return TECC_FORMATTER_SUCCESS;
}

/*
 * normalize_float_value - Normalize floating-point value for OPE algorithm
 *
 * Converts floating-point value to normalized numeric format for OPE encryption.
 * Handles special cases like NaN, infinity, and normal floating-point values.
 *
 * @param context Encryption context (not used for float normalization)
 * @param float_value Floating-point value to normalize
 * @param norm_num Output normalized numeric structure
 */
static TeccFormatterErrorCode
normalize_float_value(TeccCryptoContextCore *context,
                     double float_value,
                     TeccNumericNormalized *norm_num)
{
    char buffer[64];
    int len;

    /* Validate parameters */
    if (!norm_num)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    /* Initialize normalized structure */
    memset(norm_num, 0, sizeof(TeccNumericNormalized));

    /* Handle special floating-point values */
    /* In C90, we need to check for NaN and infinity manually */
    /* Check for NaN: NaN is not equal to itself */
    if (float_value != float_value) {
        norm_num->sign = TECC_NUMERIC_SIGN_NAN;
        norm_num->weight = 0;
        norm_num->ndigits = 0;
        return TECC_FORMATTER_SUCCESS;
    }

    /* Check for infinity: large values that overflow */
    /* This is a simplified check - in production code we should use platform-specific methods */
    if (float_value > 1e308 || float_value < -1e308) {
        if (float_value > 0) {
            norm_num->sign = TECC_NUMERIC_SIGN_INF;
        } else {
            norm_num->sign = TECC_NUMERIC_SIGN_NINF;
        }
        norm_num->weight = 0;
        norm_num->ndigits = 0;
        return TECC_FORMATTER_SUCCESS;
    }

    /* Handle zero */
    if (float_value == 0.0) {
        norm_num->sign = TECC_NUMERIC_SIGN_ZERO;
        norm_num->weight = 0;
        norm_num->ndigits = 0;
        return TECC_FORMATTER_SUCCESS;
    }

    /* Determine sign */
    if (float_value > 0) {
        norm_num->sign = TECC_NUMERIC_SIGN_POSITIVE;
    } else {
        norm_num->sign = TECC_NUMERIC_SIGN_NEGATIVE;
        float_value = -float_value;  /* Work with absolute value */
    }

    /* Convert float to string for parsing (could be optimized with direct conversion) */
    len = snprintf(buffer, sizeof(buffer), "%.15g", float_value);

    if (len < 0 || len >= (int)sizeof(buffer)) {
        return TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW;
    }

    /* Parse the string representation.
     * normalize_string_number zeroes norm_num (memset), so save/restore sign. */
    {
        uint8_t saved_sign = norm_num->sign;
        TeccFormatterErrorCode ret;

        ret = normalize_string_number(context, buffer, len, norm_num);
        if (ret == TECC_FORMATTER_SUCCESS
            && norm_num->sign != TECC_NUMERIC_SIGN_ZERO)
        {
            /* Restore original sign — normalize_string_number only sees
             * the absolute-value text and always sets POSITIVE. */
            norm_num->sign = saved_sign;
        }
        return ret;
    }
}

/*
 * normalize_numeric_text - Normalize numeric text string for OPE algorithm
 *
 * Parses numeric text string (e.g., "123.45", "-67.89") and converts to
 * normalized numeric format for OPE encryption.
 *
 * @param context Encryption context (used for GUC parameter processing)
 * @param numeric_text Numeric text string to normalize
 * @param text_len Length of numeric text string
 * @param norm_num Output normalized numeric structure
 */
static TeccFormatterErrorCode
normalize_numeric_text(TeccCryptoContextCore *context,
                      const char *numeric_text,
                      size_t text_len,
                      TeccNumericNormalized *norm_num)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    char *processed_numeric = NULL;
    TeccFormatterErrorCode guc_result;
    const char *text_to_parse;
    size_t len_to_parse;
    TeccFormatterErrorCode result;

    /* Validate parameters */
    if (!numeric_text || !norm_num || text_len == 0)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    /* Initialize normalized structure */
    memset(norm_num, 0, sizeof(TeccNumericNormalized));

    /* Apply GUC parameters to numeric string */
    guc_result = tecc_apply_guc_to_numeric(context,
                                                                 numeric_text,
                                                                 text_len,
                                                                 &processed_numeric);

    text_to_parse = numeric_text;
    len_to_parse = text_len;

    if (guc_result == TECC_FORMATTER_SUCCESS && processed_numeric)
    {
        /* Use GUC-processed numeric string */
        text_to_parse = processed_numeric;
        len_to_parse = strlen(processed_numeric);
    }

    /* Use string number normalization (full numeric parsing could be added here) */
    result = normalize_string_number(context,
                                     text_to_parse,
                                     len_to_parse,
                                     norm_num);

    /* Cleanup processed numeric string */
    if (processed_numeric)
        pg_free(processed_numeric);

    return result;
}

/*
 * encode_normalized_format_to_buffer - Encode normalized numeric structure to buffer
 *
 * Converts normalized numeric structure to binary format and stores in provided buffer.
 * Format: sign(1) + weight(2) + reserved(3) + digits(variable) + dscale(2)
 * dscale放在末尾确保不影响DET/OPE的排序比较。
 *
 * @param norm_num Normalized numeric structure
 * @param buffer Output buffer for encoded data
 * @param buffer_size Size of output buffer
 * @param output_len Actual output length
 */
static TeccFormatterErrorCode
encode_normalized_format_to_buffer(TeccNumericNormalized *norm_num,
                                  unsigned char *buffer,
                                  size_t buffer_size,
                                  size_t *output_len)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    size_t header_size;
    size_t digits_size;
    size_t total_size;

    /* Validate parameters */
    if (!norm_num || !buffer || !output_len)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    /* Calculate required size: header + digits + dscale(2) */
    header_size = TECC_NUMERIC_HEADER_SIZE;
    digits_size = norm_num->ndigits * sizeof(uint16_t);
    total_size = header_size + digits_size + sizeof(uint16_t);

    /* Check buffer size */
    if (buffer_size < total_size)
        return TECC_FORMATTER_ERROR_BUFFER_TOO_SMALL;

    /* Encode header */
    buffer[0] = norm_num->sign;  /* sign */

    /* ReqID:TECC4.0 - Weight is stored with a bias of 16384 so that
     * memcmp on network-byte-order uint16 gives correct ordering for
     * both negative and positive weights.  Without the bias, a negative
     * weight (e.g. -4 = 0xFFFC) would appear larger than a positive one
     * (e.g. 4 = 0x0004) under raw byte comparison.
     *
     * For NEGATIVE numbers, the biased weight is ALSO complemented:
     *   encoded = UINT16_MAX - (weight + 16384)
     * This makes more-negative values (with larger absolute weight) have
     * SMALLER encoded bytes, ensuring correct memcmp ordering across all
     * negative values regardless of their weight. */
    {
        uint16_t biased_weight = (uint16_t)((int32_t)norm_num->weight + 16384);
        if (norm_num->sign == TECC_NUMERIC_SIGN_NEGATIVE)
            biased_weight = 65535 - biased_weight;
        buffer[1] = (biased_weight >> 8) & 0xFF;
        buffer[2] = biased_weight & 0xFF;
    }

    /* reserved bytes (3-5) */
    buffer[3] = 0;
    buffer[4] = 0;
    buffer[5] = 0;

    /* Encode digits.
     * ReqID:TECC4.0 - For negative numbers, store 9's complement of
     * each digit so that memcmp order matches mathematical order:
     * e.g. -80000 (digit 8→1) < -50000 (digit 5→4) in encoded bytes.
     * ReqID:TECC4.0 - NDigits is not stored in the header; it is derived
     * from (total_len - HEADER_SIZE - DSCALE_SIZE) / sizeof(uint16_t)
     * during decoding (TECC_NORM_HEADER_SIZE accounts for dscale). */
    if (norm_num->ndigits > 0) {
        uint16_t *digits_ptr = (uint16_t *)(buffer + header_size);
        int i;
        for (i = 0; i < norm_num->ndigits; i++) {
            if (norm_num->sign == TECC_NUMERIC_SIGN_NEGATIVE)
                digits_ptr[i] = (uint16_t)(9 - norm_num->digits[i]);
            else
                digits_ptr[i] = norm_num->digits[i];
        }
    }

    /* ReqID:TECC4.0 - 追加dscale（2字节，大端）在末尾。
     * dscale只在RND解密输出时使用，DET/OPE加密前会截掉末尾2字节。 */
    {
        unsigned char *dscale_ptr = buffer + header_size + digits_size;
        dscale_ptr[0] = (unsigned char)((norm_num->dscale >> 8) & 0xFF);
        dscale_ptr[1] = (unsigned char)(norm_num->dscale & 0xFF);
    }

    *output_len = total_size;
    return TECC_FORMATTER_SUCCESS;
}

/*
 * normalize_string_number - Normalize string representation of a number
 *
 * Parses string representation of a number and converts to normalized format.
 * This is a simplified implementation for basic numeric string parsing.
 *
 * @param context Encryption context (not used)
 * @param str_value String representation of number
 * @param str_len Length of string
 * @param norm_num Output normalized numeric structure
 */
static TeccFormatterErrorCode
normalize_string_number(TeccCryptoContextCore *context,
                       const char *str_value,
                       size_t str_len,
                       TeccNumericNormalized *norm_num)
{
    const char *p;
    size_t i;
    int sign;
    int digit_count;
    int weight;
    int decimal_position;  /* digits before decimal point */

    /* Validate parameters */
    if (!str_value || !norm_num || str_len == 0)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    /* Initialize normalized structure */
    memset(norm_num, 0, sizeof(TeccNumericNormalized));


    /* ReqID:TECC4.0 - For float4/8, parse text to float and re-format with
     * Ryu shortest-decimal (matching backend float4out/float8out).
     * This ensures canonical text representation (e.g. "1.1" not "1.10000002")
     * for consistent DET encryption across INSERT and WHERE paths. */
    if (context && (context->data_type == FLOAT4OID || context->data_type == FLOAT8OID))
    {
        char canonical[64];
        char *endptr = NULL;

        /* Skip leading sign for strtod (already handled below, but strtod needs it) */
        double val = strtod(str_value, &endptr);
        if (endptr == str_value)
            return TECC_FORMATTER_ERROR_INVALID_INPUT;

        /* Only re-format finite values; NaN/Inf handled by existing logic below */
        if (isfinite(val))
        {
            if (context->data_type == FLOAT4OID)
                float_to_shortest_decimal_buf((float)val, canonical);
            else
                double_to_shortest_decimal_buf(val, canonical);

            str_value = canonical;
            str_len = strlen(canonical);
        }
    }

    i = 0;
    sign = 1;
    digit_count = 0;
    weight = 0;
    decimal_position = 0;

    p = str_value;

    /* Skip leading whitespace */
    while (i < str_len && isspace((unsigned char)p[i]))
        i++;

    if (i >= str_len)
        return TECC_FORMATTER_ERROR_INVALID_INPUT;

    /* Check sign */
    if (p[i] == '-') {
        sign = -1;
        i++;
    } else if (p[i] == '+') {
        i++;
    }

    /* ReqID:TECC4.0 - Check for NaN, Infinity before digit parsing */
    if (i < str_len) {
        /* Case-insensitive NaN check */
        if (str_len - i >= 3) {
            size_t remaining_len = str_len - i;
            if ((remaining_len == 3 || !isalnum((unsigned char)p[i+3])) &&
                pg_strncasecmp(p + i, "NaN", 3) == 0) {
                norm_num->sign = TECC_NUMERIC_SIGN_NAN;
                norm_num->weight = 0;
                norm_num->ndigits = 0;
                return TECC_FORMATTER_SUCCESS;
            }
        }
        /* Case-insensitive Infinity/Inf check */
        if (str_len - i >= 3 &&
            (pg_strncasecmp(p + i, "Inf", 3) == 0 ||
             pg_strncasecmp(p + i, "inf", 3) == 0)) {
            /* Reject Infinity for NUMERIC — PG numeric_in rejects these */
            if (context && context->data_type == NUMERICOID)
                return TECC_FORMATTER_ERROR_INVALID_INPUT;
            norm_num->sign = (sign < 0) ? TECC_NUMERIC_SIGN_NINF : TECC_NUMERIC_SIGN_INF;
            norm_num->weight = 0;
            norm_num->ndigits = 0;
            return TECC_FORMATTER_SUCCESS;
        }
    }

    /* ReqID:TECC4.0 - Align with backend numeric_in: require at least one digit */
    if (i >= str_len)
        return TECC_FORMATTER_ERROR_INVALID_INPUT;

    /* Parse digits before decimal point */
    {
        bool has_leading_digits = false;
        while (i < str_len && isdigit((unsigned char)p[i])) {
            int digit = p[i] - '0';
            has_leading_digits = true;
            if (digit_count >= TECC_NUMERIC_MAX_DIGITS)
                return TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW;
            norm_num->digits[digit_count] = digit;
            digit_count++;
            decimal_position++;
            i++;
        }

        /* Decimal point */
        if (i < str_len && p[i] == '.') {
            i++;
            while (i < str_len && isdigit((unsigned char)p[i])) {
                int digit = p[i] - '0';
                if (digit_count >= TECC_NUMERIC_MAX_DIGITS)
                    return TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW;
                norm_num->digits[digit_count] = digit;
                digit_count++;
                i++;
            }
        }

        /* Must have at least one digit somewhere */
        if (!has_leading_digits && digit_count == 0)
            return TECC_FORMATTER_ERROR_INVALID_INPUT;
    }

    /* ReqID:TECC4.0 - Handle scientific notation (E/e+NNN) */
    if (i < str_len && (p[i] == 'e' || p[i] == 'E')) {
        int exp_sign = 1;
        int exp_value = 0;
        bool has_exp_digit = false;
        i++;

        if (i < str_len && (p[i] == '-' || p[i] == '+')) {
            if (p[i] == '-')
                exp_sign = -1;
            i++;
        }

        while (i < str_len && isdigit((unsigned char)p[i])) {
            has_exp_digit = true;
            exp_value = exp_value * 10 + (p[i] - '0');
            if (exp_value > 9999)  /* Reasonable exponent limit */
                return TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW;
            i++;
        }

        if (!has_exp_digit)
            return TECC_FORMATTER_ERROR_INVALID_INPUT;

        /* Apply exponent to weight */
        decimal_position += exp_sign * exp_value;
    }

    /* Check for trailing non-whitespace characters (allow null terminator) */
    while (i < str_len) {
        unsigned char c = (unsigned char)p[i];
        if (c != '\0' && !isspace(c))
            return TECC_FORMATTER_ERROR_INVALID_INPUT;
        i++;
    }

    /* Begin - ReqID:TECC4.0: 计算dscale并strip尾随零，确保DET加密时
     * 1.0和1产生相同的归一化digits（数值语义等价），dscale独立保存用于
     * RND解密时的scale恢复。对齐PG backend set_var_from_str→strip_var行为。 */
    {
        int all_zero = 1;
        int zi;
        int raw_dscale = 0;

        /* 计算原始dscale（小数点后的digit数） */
        if (decimal_position > 0 && decimal_position < digit_count + 1)
            raw_dscale = digit_count - decimal_position;
        else if (decimal_position == 0)
            raw_dscale = digit_count;  /* e.g. ".123" */
        else
            raw_dscale = 0;

        /* 全零值检测 */
        for (zi = 0; zi < digit_count; zi++) {
            if (norm_num->digits[zi] != 0) {
                all_zero = 0;
                break;
            }
        }
        if (all_zero) {
            norm_num->sign = TECC_NUMERIC_SIGN_ZERO;
            norm_num->weight = 0;
            norm_num->ndigits = 0;
            norm_num->dscale = (uint16_t)((raw_dscale > 0) ? raw_dscale : 0);
            return TECC_FORMATTER_SUCCESS;
        }

        /* 非全零值：strip尾随零（对齐PG strip_var）。
         * 只减少digit_count，保留dscale不变——PG的strip_var也保留dscale，
         * 这样解密时可根据dscale恢复输入精度（如1.0→"1.0", 1→"1"）。 */
        while (digit_count > 0 && norm_num->digits[digit_count - 1] == 0) {
            digit_count--;
        }

        /* 保存dscale（只有NUMERIC类型有意义，int/float为0） */
        norm_num->dscale = (uint16_t)((raw_dscale > 0) ? raw_dscale : 0);

        /* 如果strip后digit_count为0（如".0"），归为零 */
        if (digit_count == 0) {
            norm_num->sign = TECC_NUMERIC_SIGN_ZERO;
            norm_num->weight = 0;
            norm_num->ndigits = 0;
            return TECC_FORMATTER_SUCCESS;
        }
    }

    /* Set weight: decimal_position - 1 (for integer part) */
    weight = decimal_position - 1;

    /* ReqID:TECC4.0 - Strip leading zeros and adjust weight */
    {
        int lead_zeros = 0;
        while (lead_zeros < digit_count && norm_num->digits[lead_zeros] == 0) {
            lead_zeros++;
        }
        if (lead_zeros > 0) {
            int j;
            for (j = 0; j < digit_count - lead_zeros; j++)
                norm_num->digits[j] = norm_num->digits[j + lead_zeros];
            digit_count -= lead_zeros;
            weight -= lead_zeros;
        }
    }

    /* Set normalized values */
    if (sign > 0)
        norm_num->sign = TECC_NUMERIC_SIGN_POSITIVE;
    else
        norm_num->sign = TECC_NUMERIC_SIGN_NEGATIVE;

    norm_num->weight = weight;
    norm_num->ndigits = digit_count;

    /* Check weight range */
    if (norm_num->weight < TECC_NUMERIC_MIN_WEIGHT || norm_num->weight > TECC_NUMERIC_MAX_WEIGHT)
        return TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW;

    /* ReqID:TECC4.0 - Apply NUMERIC typmod precision/scale validation and rounding */
    if (context && context->type_mod >= (int)sizeof(int32)
        && context->data_type == NUMERICOID)
    {
        int precision = ((context->type_mod - (int)sizeof(int32)) >> 16) & 0xFFFF;
        int scale = (context->type_mod - (int)sizeof(int32)) & 0xFFFF;

        if (precision > 0 && precision <= TECC_NUMERIC_MAX_DIGITS)
        {
            int int_digits = (norm_num->weight >= 0) ? (norm_num->weight + 1) : 0;

            /* Check integer part fits in precision - scale */
            if (int_digits > precision - scale)
                return TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW;

            /* Round to 'scale' decimal places at position weight+scale+1 */
            {
                int round_pos = norm_num->weight + scale + 1;
                int old_ndigits = (int)norm_num->ndigits;

                if (round_pos < old_ndigits)
                {
                    int carry = 0;

                    if (round_pos >= 0)
                    {
                        carry = (norm_num->digits[round_pos] >= 5) ? 1 : 0;
                        norm_num->ndigits = (uint16_t)round_pos;
                    }
                    else
                    {
                        /* All significant digits beyond rounding precision */
                        carry = (norm_num->digits[0] >= 5) ? 1 : 0;
                        norm_num->ndigits = 0;
                    }

                    if (carry)
                    {
                        int j;
                        for (j = (int)norm_num->ndigits - 1; j >= 0 && carry > 0; j--)
                        {
                            int new_val = (int)norm_num->digits[j] + carry;
                            if (new_val >= 10)
                            {
                                norm_num->digits[j] = 0;
                                carry = 1;
                            }
                            else
                            {
                                norm_num->digits[j] = (uint16_t)new_val;
                                carry = 0;
                            }
                        }
                        if (carry > 0)
                        {
                            int k;
                            for (k = (int)norm_num->ndigits; k > 0; k--)
                                norm_num->digits[k] = norm_num->digits[k - 1];
                            norm_num->digits[0] = 1;
                            norm_num->ndigits++;
                            norm_num->weight++;
                        }
                    }

                    /* Strip trailing zeros after rounding */
                    while (norm_num->ndigits > 0
                           && norm_num->digits[norm_num->ndigits - 1] == 0)
                        norm_num->ndigits--;

                    /* Handle rounding to zero */
                    if (norm_num->ndigits == 0)
                    {
                        norm_num->sign = TECC_NUMERIC_SIGN_ZERO;
                        norm_num->weight = 0;
                        norm_num->ndigits = 1;
                        norm_num->digits[0] = 0;
                        return TECC_FORMATTER_SUCCESS;
                    }

                    /* Re-check integer digits after rounding carry */
                    int_digits = (norm_num->weight >= 0) ? (norm_num->weight + 1) : 0;
                    if (int_digits > precision - scale)
                        return TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW;
                }
            }
        }
    }

    return TECC_FORMATTER_SUCCESS;
}

/*
 * tecc_normalize_text_to_buffer - Text type normalization with buffer
 *
 * Process text differently based on algorithm type and write result to buffer:
 * - RND: Add 4-byte network byte order length prefix
 * - DET/OPE/SEARCH: Keep original format
 * - OPE+collation: Generate ICU sort key
 *
 * @param context Encryption context (contains collation_oid)
 * @param input_data Input text data
 * @param input_len Input text length
 * @param buffer Output buffer for normalized data
 */
static TeccFormatterErrorCode
tecc_normalize_text_to_buffer(TeccCryptoContextCore *context,
                             const unsigned char *input_data,
                             size_t input_len,
                             TeccCryptoBuffer *buffer)
{
    size_t required_size;

    /* Validate parameters */
    if (!context || !input_data || !buffer || !buffer->data)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    required_size = 0;

    /* Check input length */
    if (input_len == 0)
    {
        /* ReqID:TECC4.0 - Empty string: produce a fixed 16-byte zero block.
         * This is the canonical encrypted-core representation for empty-string
         * input, used uniformly for DET and OPE. All three text types (bpchar,
         * varchar, text) use rtrim'd data for the encrypted core, so a pure
         * trailing-space value like bpchar('   ') produces the same empty-string
         * core as varchar(''). The space_count in the ciphertext format records
         * the number of trailing spaces removed, allowing backend comparison
         * functions to distinguish empty from whitespace-only values. */
        memset((unsigned char *)buffer->data, 0, 16);
        buffer->used = 16;
        return TECC_FORMATTER_SUCCESS;
    }

    /* Check buffer size */

    /* Calculate required buffer size based on algorithm type */
    if (context->algo_type == TECC_ALGO_RND) {
        /* RND: Add 4-byte length prefix */
        required_size = TECC_TEXT_LENGTH_PREFIX_SIZE + input_len;
    } else if (context->collation_locale != NULL
               && context->collation_locale[0] != '\0'
               && !tecc_is_raw_byte_collation(context->collation_locale)) {
        /* ReqID:TECC4.0 - ICU sort key for any algorithm (DET/OPE/SEARCH)
         * when collation is set. Estimate size with multiplier. For OPE,
         * align to 12-byte multiple for chunk padding. */
        required_size = input_len * TECC_TEXT_MAX_ICU_SORTKEY_MULTIPLIER;
        if (required_size < TECC_TEXT_MIN_ICU_SORTKEY_SIZE)
            required_size = TECC_TEXT_MIN_ICU_SORTKEY_SIZE;
        if (context->algo_type == TECC_ALGO_OPE)
            required_size = ((required_size + TECC_TEXT_OPE_CHUNK_SIZE - 1)
                            / TECC_TEXT_OPE_CHUNK_SIZE) * TECC_TEXT_OPE_CHUNK_SIZE;
    } else if (context->algo_type == TECC_ALGO_OPE) {
        /* ReqID:TECC4.0 - OPE without collation: pad to 12-byte multiple
         * so all OPE chunks use the same msglen in enc_bytes_internal. */
        required_size = ((input_len + TECC_TEXT_OPE_CHUNK_SIZE - 1)
                        / TECC_TEXT_OPE_CHUNK_SIZE) * TECC_TEXT_OPE_CHUNK_SIZE;
    } else {
        /* DET, SEARCH: Keep original format */
        required_size = input_len;
    }

    /* Check if buffer is large enough */
    if (buffer->size < required_size)
        return TECC_FORMATTER_ERROR_BUFFER_TOO_SMALL;

    /* Process based on algorithm type */
    if (context->algo_type == TECC_ALGO_RND) {
        /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
        char *processed_text = NULL;
        size_t processed_len = 0;
        TeccFormatterErrorCode guc_result = tecc_apply_guc_to_text(context,
                                                                  input_data,
                                                                  input_len,
                                                                  &processed_text,
                                                                  &processed_len);

        if (guc_result != TECC_FORMATTER_SUCCESS)
        {
            /* GUC processing failed - use original input */
            processed_text = (char *)pg_malloc(input_len);
            if (!processed_text)
                return TECC_FORMATTER_ERROR_MEMORY_ALLOC;
            memcpy(processed_text, input_data, input_len);
            processed_len = input_len;
        }

        /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
        {
        /* Add 4-byte network byte order length prefix */
        unsigned char *buf_data = (unsigned char *)buffer->data;

        /* Store length in network byte order (big-endian) */
        buf_data[0] = (processed_len >> 24) & 0xFF;
        buf_data[1] = (processed_len >> 16) & 0xFF;
        buf_data[2] = (processed_len >> 8) & 0xFF;
        buf_data[3] = processed_len & 0xFF;

        /* Copy processed data */
        memcpy(buf_data + TECC_TEXT_LENGTH_PREFIX_SIZE, processed_text, processed_len);

        buffer->used = TECC_TEXT_LENGTH_PREFIX_SIZE + processed_len;

        /* Cleanup processed text */
        if (processed_text)
            pg_free(processed_text);

        return TECC_FORMATTER_SUCCESS;
        }  /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    } else if (context->collation_locale != NULL
               && context->collation_locale[0] != '\0'
               && !tecc_is_raw_byte_collation(context->collation_locale)) {
        /* ReqID:TECC4.0 - ICU sort key for all algorithm types when collation
         * is set. The server now uses ICU for all text comparisons (see
         * pg_locale.c), so generating ICU sort keys here ensures plain text
         * and ciphertext ordering are consistent. For OPE, additionally pad
         * to 12-byte chunk alignment. */
        {
        size_t sortkey_len = 0;
        size_t padded_len;
        unsigned char *sortkey = NULL;
        TeccFormatterErrorCode result;

        /* ReqID:TECC4.0 - Use dynamic allocation for sort key since
         * ICU sort key length can be much larger than input (up to
         * ~10x for short strings). Pre-allocated buffer estimation
         * (input_len * 3) is insufficient and causes buffer overflows. */
        result = tecc_icu_generate_sortkey(
            context->collation_locale, input_data, input_len,
            &sortkey, &sortkey_len);

        if (result != TECC_FORMATTER_SUCCESS)
            return result;

        if (context->algo_type == TECC_ALGO_OPE)
        {
            /* ReqID:TECC4.0 - OPE: pad sort key to 12-byte multiple */
            padded_len = ((sortkey_len + TECC_TEXT_OPE_CHUNK_SIZE - 1)
                         / TECC_TEXT_OPE_CHUNK_SIZE) * TECC_TEXT_OPE_CHUNK_SIZE;
        }
        else
        {
            /* DET/SEARCH: use sort key directly, no padding needed */
            padded_len = sortkey_len;
        }

        /* Reallocate buffer if needed */
        if (buffer->size < padded_len)
        {
            unsigned char *new_data = pg_realloc(buffer->data, padded_len);
            if (!new_data)
            {
                pg_free(sortkey);
                return TECC_FORMATTER_ERROR_MEMORY_ALLOC;
            }
            buffer->data = new_data;
            buffer->size = padded_len;
        }

        memcpy(buffer->data, sortkey, sortkey_len);
        if (padded_len > sortkey_len)
            memset((unsigned char *)buffer->data + sortkey_len, 0,
                   padded_len - sortkey_len);
        buffer->used = padded_len;

        pg_free(sortkey);
        return TECC_FORMATTER_SUCCESS;
        }  /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    } else if (context->algo_type == TECC_ALGO_OPE) {
        /* ReqID:TECC4.0 - OPE without collation: copy text and pad
         * with zero bytes to 12-byte multiple so all OPE chunks use
         * the same msglen (12) in enc_bytes_internal, making ciphertexts
         * comparable via memcmp. */
        {
        size_t padded_len;
        padded_len = ((input_len + TECC_TEXT_OPE_CHUNK_SIZE - 1)
                     / TECC_TEXT_OPE_CHUNK_SIZE) * TECC_TEXT_OPE_CHUNK_SIZE;
        memcpy(buffer->data, input_data, input_len);
        memset((unsigned char *)buffer->data + input_len, 0,
               padded_len - input_len);
        buffer->used = padded_len;
        return TECC_FORMATTER_SUCCESS;
        }
    } else {
        /* DET, SEARCH: Apply GUC parameters */
        char *processed_text = NULL;
        size_t processed_len = 0;
        TeccFormatterErrorCode guc_result = tecc_apply_guc_to_text(context,
                                                                  input_data,
                                                                  input_len,
                                                                  &processed_text,
                                                                  &processed_len);

        if (guc_result != TECC_FORMATTER_SUCCESS || !processed_text)
        {
            /* GUC processing failed - use original input */
            memcpy(buffer->data, input_data, input_len);
            buffer->used = input_len;
        }
        else
        {
            /* Use GUC-processed text */
            memcpy(buffer->data, processed_text, processed_len);
            buffer->used = processed_len;
            pg_free(processed_text);
        }

        return TECC_FORMATTER_SUCCESS;
    }
}


/*
 * tecc_icu_generate_sortkey - Generate ICU sort key for text data
 *
 * Generates a binary sort key using ICU collation for text comparison.
 * This is used by OPE algorithm to maintain order-preserving encryption.
 *
 * @param collation_locale Collation locale string (e.g. "en_US.UTF-8")
 * @param input_text Input text data
 * @param input_len Input text length
 * @param sortkey Pointer to receive sort key buffer (caller must free)
 * @param sortkey_len Pointer to receive sort key length
 * @return Error code
 */
static TeccFormatterErrorCode tecc_icu_generate_sortkey(const char *collation_locale,
                                                       const unsigned char *input_text,
                                                       size_t input_len,
                                                       unsigned char **sortkey,
                                                       size_t *sortkey_len)
{
    /* Parameter validation */
    if (!collation_locale || !input_text || !sortkey || !sortkey_len)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    if (input_len == 0 )
        return TECC_FORMATTER_ERROR_DATA_TOO_LARGE;

    /* Use the ICU normalizer function */
    return tecc_icu_normalizer_generate_sortkey(collation_locale, input_text, input_len, sortkey, sortkey_len);
}

/* ==================== GUC Integration Helper Functions ==================== */

/*
 * tecc_formatter_get_guc_params - Get GUC parameters from context
 *
 * @param context Encryption context
 * @return Pointer to GUC parameters, or NULL if not available
 */
/*
 * tecc_apply_guc_to_text - Apply GUC parameters to text data
 *
 * @param context Encryption context
 * @param input_data Input text data
 * @param input_len Input text length
 * @param output_text Output text data (caller must free)
 * @param output_len Output text length
 * @return Error code
 */
static TeccFormatterErrorCode tecc_apply_guc_to_text(TeccCryptoContextCore *context,
                                                    const unsigned char *input_data,
                                                    size_t input_len,
                                                    char **output_text,
                                                    size_t *output_len)
{
    /* Simplified GUC processing for text data */
    /* Note: tecc_formatter_guc module has been removed, GUC processing simplified */

    /* Just copy input to output */
    *output_text = (char *)pg_malloc(input_len + 1);
    if (!*output_text)
        return TECC_FORMATTER_ERROR_MEMORY_ALLOC;

    memcpy(*output_text, input_data, input_len);
    (*output_text)[input_len] = '\0';
    *output_len = input_len;

    return TECC_FORMATTER_SUCCESS;
}

/*
 * tecc_apply_guc_to_numeric - Apply GUC parameters to numeric string
 *
 * @param context Encryption context
 * @param numeric_str Numeric string
 * @param processed_numeric Processed numeric string (caller must free)
 * @return Error code
 */
static TeccFormatterErrorCode tecc_apply_guc_to_numeric(TeccCryptoContextCore *context,
                                                       const char *numeric_str,
                                                       size_t numeric_len,
                                                       char **processed_numeric)
{
    /* Simplified GUC processing for numeric data */
    /* Note: tecc_formatter_guc module has been removed, GUC processing simplified */

    /* Just copy input to output using explicit length (input may not be null-terminated) */
    *processed_numeric = (char *)pg_malloc(numeric_len + 1);
    if (!*processed_numeric)
        return TECC_FORMATTER_ERROR_MEMORY_ALLOC;

    memcpy(*processed_numeric, numeric_str, numeric_len);
    (*processed_numeric)[numeric_len] = '\0';
    return TECC_FORMATTER_SUCCESS;
}

/*
 * tecc_formatter_format_cipherdata - Format cipherdata for transmission (legacy interface)
 *
 * @param context Encryption context
 * @param cipherdata Output cipherdata pointer
 * @param cipherdata_len Output cipherdata length
 * @return Error code
 * @deprecated Use zero-copy interfaces instead
 */
TeccFormatterErrorCode
tecc_formatter_format_cipherdata(TeccCryptoContextExtended *context,
                                 CipherDataType **cipherdata,
                                 size_t *cipherdata_len)
{
    size_t total_len;
    CipherDataType *cd;
    char *dest_ptr;
    int32 rnd_len, det_len, ope_len, search_len, eg_add_len, eg_mul_len;
    uint16 mask;
    char *hdr_ptr;

    if (!context || !cipherdata || !cipherdata_len)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    /* Compute individual segment lengths (HE lengths include 4B DEK ID prefix) */
    rnd_len = context->rnd_cipher_len;
    det_len = context->det_cipher_len;
    ope_len = context->ope_cipher_len;
    search_len = context->search_cipher_len;
    eg_add_len = context->he_add_cipher_len +
        (context->he_add_cipher_len > 0 ? CIPHERDATA_HE_DEK_ID_SIZE : 0);
    eg_mul_len = context->he_mul_cipher_len +
        (context->he_mul_cipher_len > 0 ? CIPHERDATA_HE_DEK_ID_SIZE : 0);

    /* Compute segment mask */
    mask = 0;
    if (rnd_len > 0)        mask |= CIPHERDATA_SEG_RND;
    if (det_len > 0)        mask |= CIPHERDATA_SEG_DET;
    if (ope_len > 0)        mask |= CIPHERDATA_SEG_OPE;
    if (search_len > 0)     mask |= CIPHERDATA_SEG_SEARCH;
    if (eg_add_len > 0)     mask |= CIPHERDATA_SEG_EG_ADD;
    if (eg_mul_len > 0)     mask |= CIPHERDATA_SEG_EG_MUL;

    /* Calculate total length with compact header */
    total_len = sizeof(int32) + CIPHERDATA_FIELDS_BASE_SIZE +
                CIPHERDATA_POPCOUNT16(mask) * sizeof(int32) +
                rnd_len + det_len + ope_len + search_len +
                eg_add_len + eg_mul_len;

    /* Allocate memory */
    cd = (CipherDataType *)pg_malloc0(total_len);
    if (!cd)
        return TECC_FORMATTER_ERROR_MEMORY_ALLOC;

    /* Set varlena header */
    SET_VARSIZE(cd, total_len);

    /* Set version with mask in low 16 bits */
    cd->version = CIPHERDATA_VERSION_2_0 | (int32)mask;

    /* Set DEK identity */
    cd->dek_dist_oid = InvalidOid;

    /* Write length fields at mask-computed offsets.
     * cd is allocated with a 4-byte varlena header, so the field area begins
     * at cd->data; length fields precede the ciphertext data. */
    hdr_ptr = cd->data;
    if (mask & CIPHERDATA_SEG_RND)      { *(int32 *)hdr_ptr = rnd_len;     hdr_ptr += sizeof(int32); }
    if (mask & CIPHERDATA_SEG_DET)      { *(int32 *)hdr_ptr = det_len;     hdr_ptr += sizeof(int32); }
    if (mask & CIPHERDATA_SEG_OPE)      { *(int32 *)hdr_ptr = ope_len;     hdr_ptr += sizeof(int32); }
    if (mask & CIPHERDATA_SEG_SEARCH)   { *(int32 *)hdr_ptr = search_len;  hdr_ptr += sizeof(int32); }
    if (mask & CIPHERDATA_SEG_EG_ADD)   { *(int32 *)hdr_ptr = eg_add_len;  hdr_ptr += sizeof(int32); }
    if (mask & CIPHERDATA_SEG_EG_MUL)   { *(int32 *)hdr_ptr = eg_mul_len;  hdr_ptr += sizeof(int32); }

    /* Copy ciphertexts */
    dest_ptr = hdr_ptr;

    if (rnd_len > 0 && context->rnd_ciphertext)
    {
        memcpy(dest_ptr, context->rnd_ciphertext, rnd_len);
        dest_ptr += rnd_len;
    }

    if (det_len > 0 && context->det_ciphertext)
    {
        memcpy(dest_ptr, context->det_ciphertext, det_len);
        dest_ptr += det_len;
    }

    if (ope_len > 0 && context->ope_ciphertext)
    {
        memcpy(dest_ptr, context->ope_ciphertext, ope_len);
        dest_ptr += ope_len;
    }

    if (context->search_cipher_len > 0 && context->search_ciphertext)
    {
        memcpy(dest_ptr, context->search_ciphertext, context->search_cipher_len);
        dest_ptr += context->search_cipher_len;
    }

    /* ReqID:TECC4.0 - Copy HE ciphertexts after SEARCH slot.
     * Each HE segment carries a 4-byte DEK distributed ID prefix. */
    if (context->he_add_cipher_len > 0 && context->he_add_ciphertext)
    {
        Oid add_dek_id = context->he_add_dek_id;

        if (!OidIsValid(add_dek_id))
            add_dek_id = cd->dek_dist_oid;
        memcpy(dest_ptr, &add_dek_id, CIPHERDATA_HE_DEK_ID_SIZE);
        dest_ptr += CIPHERDATA_HE_DEK_ID_SIZE;
        memcpy(dest_ptr, context->he_add_ciphertext, context->he_add_cipher_len);
        dest_ptr += context->he_add_cipher_len;
    }

    if (context->he_mul_cipher_len > 0 && context->he_mul_ciphertext)
    {
        Oid mul_dek_id = context->he_mul_dek_id;

        if (!OidIsValid(mul_dek_id))
            mul_dek_id = cd->dek_dist_oid;
        memcpy(dest_ptr, &mul_dek_id, CIPHERDATA_HE_DEK_ID_SIZE);
        dest_ptr += CIPHERDATA_HE_DEK_ID_SIZE;
        memcpy(dest_ptr, context->he_mul_ciphertext, context->he_mul_cipher_len);
        dest_ptr += context->he_mul_cipher_len;
    }

    *cipherdata = cd;
    *cipherdata_len = total_len;

    return TECC_FORMATTER_SUCCESS;
}

/*
 * parse_int_from_text - Parse integer from text string
 *
 * Parses integer text string and converts to int64_t value.
 * Validates range based on data type.
 */
static TeccFormatterErrorCode
parse_int_from_text(const char *text, size_t text_len, Oid data_type, int64_t *int_value)
{
    char *endptr;
    long long_val;
    long long long_long_val;
    char *temp_buf;
    const char *ptr;
    bool has_digit = false;

    if (!text || !int_value || text_len == 0)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    /* Ensure text is null-terminated for strtol */
    temp_buf = pg_malloc(text_len + 1);
    if (!temp_buf)
        return TECC_FORMATTER_ERROR_MEMORY_ALLOC;

    memcpy(temp_buf, text, text_len);
    temp_buf[text_len] = '\0';

    /* ReqID:TECC4.0 - Align with backend pg_strtoint32:
     * Skip leading whitespace, require at least one digit,
     * reject sign-only input ("+" or "-"). */
    ptr = temp_buf;
    while (*ptr && isspace((unsigned char)*ptr))
        ptr++;

    if (*ptr == '+' || *ptr == '-')
        ptr++;

    if (!isdigit((unsigned char)*ptr))
    {
        pg_free(temp_buf);
        return TECC_FORMATTER_ERROR_INVALID_INPUT;
    }
    /* Count digits to reject sign-only after strtol passes */
    while (*ptr && isdigit((unsigned char)*ptr))
    {
        has_digit = true;
        ptr++;
    }
    if (!has_digit)
    {
        pg_free(temp_buf);
        return TECC_FORMATTER_ERROR_INVALID_INPUT;
    }

    errno = 0;

    /* Parse based on data type */
    switch (data_type) {
        case INT2OID:
            long_val = strtol(temp_buf, &endptr, 10);
            if (errno == ERANGE || long_val < PG_INT16_MIN || long_val > PG_INT16_MAX)
            {
                pg_free(temp_buf);
                return TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW;
            }
            *int_value = (int64_t)long_val;
            break;

        case INT4OID:
            long_val = strtol(temp_buf, &endptr, 10);
            if (errno == ERANGE || long_val < PG_INT32_MIN || long_val > PG_INT32_MAX)
            {
                pg_free(temp_buf);
                return TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW;
            }
            *int_value = (int64_t)long_val;
            break;

        case INT8OID:
            long_long_val = strtoll(temp_buf, &endptr, 10);
            if (errno == ERANGE)
            {
                pg_free(temp_buf);
                return TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW;
            }
            *int_value = (int64_t)long_long_val;
            break;

        default:
            pg_free(temp_buf);
            return TECC_FORMATTER_ERROR_INVALID_TYPE;
    }

    /* Check for conversion errors */
    if (endptr == temp_buf)  /* No digits converted */
    {
        pg_free(temp_buf);
        return TECC_FORMATTER_ERROR_DATA_FORMAT;
    }

    /* Check for trailing non-whitespace characters */
    while (*endptr != '\0') {
        if (!isspace((unsigned char)*endptr)) {
            pg_free(temp_buf);
            return TECC_FORMATTER_ERROR_DATA_FORMAT;
        }
        endptr++;
    }

    pg_free(temp_buf);
    return TECC_FORMATTER_SUCCESS;
}

/*
 * parse_float_from_text - Parse floating-point number from text string
 *
 * Parses floating-point text string and converts to double value.
 * Validates range based on data type.
 */
static TeccFormatterErrorCode
parse_float_from_text(const char *text, size_t text_len, Oid data_type, double *float_value)
{
    char *endptr;
    float float_val;
    double double_val;
    char *temp_buf;
    const char *ptr;

    if (!text || !float_value || text_len == 0)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    /* Ensure text is null-terminated for strtof/strtod */
    temp_buf = pg_malloc(text_len + 1);
    if (!temp_buf)
        return TECC_FORMATTER_ERROR_MEMORY_ALLOC;

    memcpy(temp_buf, text, text_len);
    temp_buf[text_len] = '\0';

    /* ReqID:TECC4.0 - Align with backend float4in/float8in:
     * Skip leading whitespace, reject sign-only input,
     * reject NaN and Infinity (backend rejects these). */
    ptr = temp_buf;
    while (*ptr && isspace((unsigned char)*ptr))
        ptr++;

    /* Reject NaN, Infinity, -Infinity (backend rejects for float4in/float8in) */
    if ((ptr[0] == 'N' || ptr[0] == 'n') &&
        (ptr[1] == 'a' || ptr[1] == 'A') &&
        (ptr[2] == 'N' || ptr[2] == 'n') && !isalnum((unsigned char)ptr[3]))
    {
        pg_free(temp_buf);
        return TECC_FORMATTER_ERROR_INVALID_INPUT;
    }
    if ((ptr[0] == 'I' || ptr[0] == 'i') &&
        (ptr[1] == 'n' || ptr[1] == 'N') &&
        (ptr[2] == 'f' || ptr[2] == 'F'))
    {
        pg_free(temp_buf);
        return TECC_FORMATTER_ERROR_INVALID_INPUT;
    }
    if ((ptr[0] == '+' || ptr[0] == '-') &&
        (ptr[1] == 'I' || ptr[1] == 'i') &&
        (ptr[2] == 'n' || ptr[2] == 'N') &&
        (ptr[3] == 'f' || ptr[3] == 'F'))
    {
        pg_free(temp_buf);
        return TECC_FORMATTER_ERROR_INVALID_INPUT;
    }

    /* Check for sign-only: "+" or "-" followed by no digit */
    {
        const char *check = ptr;
        if (*check == '+' || *check == '-')
            check++;
        if (*check == '\0' || (!isdigit((unsigned char)*check) && *check != '.'))
        {
            pg_free(temp_buf);
            return TECC_FORMATTER_ERROR_INVALID_INPUT;
        }
    }

    errno = 0;

    /* Parse based on data type */
    switch (data_type) {
        case FLOAT4OID:
            float_val = strtof(temp_buf, &endptr);
            if (errno == ERANGE)
            {
                pg_free(temp_buf);
                return TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW;
            }
            *float_value = (double)float_val;
            break;

        case FLOAT8OID:
            double_val = strtod(temp_buf, &endptr);
            if (errno == ERANGE)
            {
                pg_free(temp_buf);
                return TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW;
            }
            *float_value = double_val;
            break;

        default:
            pg_free(temp_buf);
            return TECC_FORMATTER_ERROR_INVALID_TYPE;
    }

    /* Check for conversion errors */
    if (endptr == temp_buf)  /* No digits converted */
    {
        pg_free(temp_buf);
        return TECC_FORMATTER_ERROR_DATA_FORMAT;
    }

    /* Check for trailing non-whitespace characters */
    while (*endptr != '\0') {
        if (!isspace((unsigned char)*endptr)) {
            pg_free(temp_buf);
            return TECC_FORMATTER_ERROR_DATA_FORMAT;
        }
        endptr++;
    }

    pg_free(temp_buf);
    return TECC_FORMATTER_SUCCESS;
}

/*
 * tecc_formatter_ope_text_pad - Pad text plaintext to 12-byte multiple for OPE
 *
 * ReqID:TECC4.0 - OPE encryption (enc_bytes_internal) embeds the chunk
 * byte-length in the SM4 input. All chunks must use the same msglen (12)
 * for ciphertexts to be comparable via memcmp. This function pads the
 * normalized text plaintext to the next 12-byte boundary.
 *
 * Modify - ReqID:TECC4.0: Use 0x00 terminator + 0xFF padding instead of
 * plain 0x00 padding. Pure 0x00 padding causes different-length short texts
 * to produce completely different 12-byte blocks (e.g. "a" → [61 00...],
 * "test" → [74 65 73 74 00...]), and OPE's bit-level encryption cannot
 * guarantee order-preservation across such different blocks.
 *
 * New scheme: append 0x00 terminator, then pad with 0xFF to 12-byte multiple.
 *   0xFF > any printable ASCII byte, so shorter strings correctly sort before
 *   longer ones with the same prefix. The 0x00 terminator ensures strlen on
 *   decrypt returns the original text length (decrypt uses RND path, which
 *   recovers text from length prefix, not from OPE padding).
 *
 * @param input_data Normalized text data
 * @param input_len Length of input data
 * @param output_data Pointer to receive padded data (caller must pg_free)
 * @param output_len Pointer to receive padded length
 */
TeccFormatterErrorCode
tecc_formatter_ope_text_pad(const unsigned char *input_data,
                           size_t input_len,
                           unsigned char **output_data,
                           size_t *output_len)
{
    unsigned char *padded;

    if (!input_data || !output_data || !output_len)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    if (input_len == 0)
    {
        /* ReqID:TECC4.0 - Empty string: produce a block with 0x00 terminator
         * followed by 0xFF padding at TECC_TEXT_OPE_CHUNK_SIZE (12) so OPE
         * encryption always works on fixed-size chunks.
         * 0x00 terminator + 0xFF ensures empty string sorts before any
         * non-empty string. */
        *output_data = (unsigned char *)pg_malloc(TECC_TEXT_OPE_CHUNK_SIZE);
        if (!*output_data)
            return TECC_FORMATTER_ERROR_MEMORY_ALLOC;
        memset(*output_data, 0xFF, TECC_TEXT_OPE_CHUNK_SIZE);
        (*output_data)[0] = 0x00;  /* 0x00 terminator at position 0 */
        *output_len = TECC_TEXT_OPE_CHUNK_SIZE;
        return TECC_FORMATTER_SUCCESS;
    }

    {
        /*
         * Modify - ReqID:TECC4.0
         * Old scheme: memset(padded + input_len, 0, ...) — plain 0x00 padding.
         *   Different-length texts produce completely different 12-byte blocks,
         *   OPE's bit-level SM4-ECB encryption cannot guarantee order-preservation.
         *
         * New scheme: insert 0x00 terminator after text, then 0xFF padding.
         *   0xFF > any printable ASCII byte, so shorter strings with 0x00
         *   correctly sort before longer strings with the same prefix.
         */
        size_t term_len = input_len + 1;  /* +1 for 0x00 terminator */
        size_t rem = term_len % TECC_TEXT_OPE_CHUNK_SIZE;
        size_t padded_len;

        if (rem == 0)
            padded_len = term_len;
        else
            padded_len = term_len + (TECC_TEXT_OPE_CHUNK_SIZE - rem);

        padded = (unsigned char *)pg_malloc(padded_len);
        if (!padded)
            return TECC_FORMATTER_ERROR_MEMORY_ALLOC;

        memcpy(padded, input_data, input_len);
        padded[input_len] = 0x00;  /* 0x00 terminator */
        if (padded_len > term_len)
            memset(padded + term_len, 0xFF, padded_len - term_len);  /* 0xFF padding */
        *output_data = padded;
        *output_len = padded_len;
    }
    return TECC_FORMATTER_SUCCESS;
}

/*
 * tecc_formatter_ope_text_icu_pad - Generate ICU sort key and pad for OPE
 *
 * For text types with collation, this generates the ICU sort key from raw
 * text data and pads to 12-byte multiple so all OPE chunks use consistent
 * msglen (12) in enc_bytes_internal.
 */
TeccFormatterErrorCode
tecc_formatter_ope_text_icu_pad(const char *collation_locale,
                               const unsigned char *input_data,
                               size_t input_len,
                               unsigned char **output_data,
                               size_t *output_len)
{
    unsigned char *sortkey;
    size_t sortkey_len;
    size_t rem;
    TeccFormatterErrorCode result;

    if (!collation_locale || !input_data || !output_data || !output_len)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    if (collation_locale[0] == '\0')
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    if (input_len == 0)
    {
        /* ReqID:TECC4.0 - Empty string with collation: produce a zero-padded
         * block at TECC_TEXT_OPE_CHUNK_SIZE (12), same reasoning as
         * tecc_formatter_ope_text_pad. ICU sort key of empty string is
         * also empty, so zero padding is the correct OPE input. */
        *output_data = (unsigned char *)pg_malloc(TECC_TEXT_OPE_CHUNK_SIZE);
        if (!*output_data)
            return TECC_FORMATTER_ERROR_MEMORY_ALLOC;
        memset(*output_data, 0, TECC_TEXT_OPE_CHUNK_SIZE);
        *output_len = TECC_TEXT_OPE_CHUNK_SIZE;
        return TECC_FORMATTER_SUCCESS;
    }

    sortkey = NULL;
    sortkey_len = 0;

    /* Step 1: Generate ICU sort key */
    result = tecc_icu_generate_sortkey(collation_locale, input_data, input_len,
                                       &sortkey, &sortkey_len);
    if (result != TECC_FORMATTER_SUCCESS)
        return result;

    /* Step 2: Pad sort key to 12-byte multiple */
    rem = sortkey_len % TECC_TEXT_OPE_CHUNK_SIZE;
    if (rem == 0)
    {
        *output_data = sortkey;
        *output_len = sortkey_len;
        return TECC_FORMATTER_SUCCESS;
    }

    {
        size_t padded_len;
        unsigned char *padded;

        padded_len = sortkey_len + (TECC_TEXT_OPE_CHUNK_SIZE - rem);
        padded = (unsigned char *)pg_malloc(padded_len);
        if (!padded)
        {
            pg_free(sortkey);
            return TECC_FORMATTER_ERROR_MEMORY_ALLOC;
        }

        memcpy(padded, sortkey, sortkey_len);
        memset(padded + sortkey_len, 0, padded_len - sortkey_len);
        pg_free(sortkey);

        *output_data = padded;
        *output_len = padded_len;
    }
    return TECC_FORMATTER_SUCCESS;
}
