/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_formatter.h
 *
 * Abstract:
 *   Data formatter module interface for TECC processor module.
 *   Provides data normalization, format conversion, and encoding services.
 *   Implements unified interface between crypto scheduler and algorithm modules.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_formatter/tecc_formatter.h
 */

#ifndef TECC_FORMATTER_H
#define TECC_FORMATTER_H

/* Avoid including PostgreSQL headers that use C99 features */
#include <stddef.h>  /* for size_t */
#include <stdint.h>  /* for int32_t */

/* PostgreSQL frontend header - must be included first */
#include "postgres_fe.h"

/* Memory allocation functions - use PostgreSQL frontend memory utilities */
#include "common/fe_memutils.h"  /* For pg_malloc, pg_free */

/* Include common types after memory utilities */
#include "../tecc_common/tecc_common_types.h"

/* Note: tecc_formatter_guc.h has been removed, GUC parameter integration simplified */

/* Varlena macros are defined in PostgreSQL headers */

/* Forward declarations for compatibility */
/* Note: Formatter modules use TeccCryptoContextCore directly */
/* typedef TeccCryptoContextCore TeccCryptoContext; */

/* CipherDataType and access macros are defined in utils/cipherdatatype.h */
/* TeccFormatterErrorCode is defined in tecc_common_types.h */

/*
 * Data normalization interface - Corresponds to scheduler spec 5.1.1
 * @note This interface is deprecated, use zero-copy versions instead
 */
/* extern TeccFormatterErrorCode tecc_formatter_normalize_data(
 *     TeccCryptoContextCore *context,
 *     const unsigned char *input_data,
 *     size_t input_len,
 *     unsigned char **output_data,
 *     size_t *output_len
 * );
 */

/*
 * CipherDataType building interface - Corresponds to scheduler spec 5.3.1
 * @note Must be fully compatible with PostgreSQL backend CipherDataType format
 * @note This interface is deprecated, use zero-copy versions instead
 */
/* extern TeccFormatterErrorCode tecc_formatter_build_cipherdata(
 *     TeccAlgorithmType algo_type,
 *     Oid data_type,
 *     unsigned char **encrypted_data,
 *     size_t *encrypted_lens,
 *     CipherDataType **cipherdata,
 *     size_t *cipherdata_len
 * );
 */

/*
 * CipherDataType parsing interface - Corresponds to scheduler spec 5.3.2
 */
extern TeccFormatterErrorCode tecc_formatter_parse_cipherdata(
    const CipherDataType *cipherdata,
    size_t cipherdata_len,
    TeccParsedCipherdata *parsed_data
);

/*
 * Format cipherdata for transmission (legacy interface)
 * @deprecated Use zero-copy interfaces instead
 */
extern TeccFormatterErrorCode tecc_formatter_format_cipherdata(
    TeccCryptoContextExtended *context,
    CipherDataType **cipherdata,
    size_t *cipherdata_len
);

/*
 * Hex encoding interface - Corresponds to scheduler spec 5.2.1
 * @note Output format must be compatible with PostgreSQL backend I/O functions
 */
extern TeccFormatterErrorCode tecc_formatter_hex_encode(
    const unsigned char *binary_data,
    size_t binary_len,
    char **hex_string
);

/*
 * Hex decoding interface - Corresponds to scheduler spec 5.2.2
 * @note Input format must be compatible with PostgreSQL backend I/O functions
 */
extern TeccFormatterErrorCode tecc_formatter_hex_decode(
    const char *hex_string,
    unsigned char **binary_data,
    size_t *binary_len
);

/* Begin - ReqID:TECC4.0 */
/*
 * Batch hex decode — decodes multiple hex strings at once.
 * Allocates one contiguous buffer. binary_data[i] points into it.
 * Caller frees binary_data[0] (the base pointer) via pg_free.
 */
extern TeccFormatterErrorCode tecc_formatter_batch_hex_decode(
    const char **hex_strings,
    size_t count,
    unsigned char **binary_data,
    size_t *binary_lens);
/* End - ReqID:TECC4.0 */

/*
 * Buffer allocation interface - Unified memory management support
 * @param context Encryption context (must contain valid mem_pool)
 * @param buffer_type Buffer type
 * @param size_needed Required buffer size
 * @param buffer_desc Output buffer descriptor
 * @return Error code
 * @note Allocates buffers from context-associated memory pool
 */
extern TeccFormatterErrorCode tecc_formatter_allocate_buffer(
    TeccCryptoContextCore *context,
    TeccBufferType buffer_type,
    size_t size_needed,
    TeccBufferDesc **buffer_desc
);

/*
 * Calculate buffer size needed for normalization
 * @param context Encryption context
 * @param input_size Input data size
 * @param output_size_needed Output buffer requirement
 * @param work_size_needed Work buffer requirement
 * @return Error code
 */
extern TeccFormatterErrorCode tecc_formatter_calculate_normalize_size(
    TeccCryptoContextCore *context,
    size_t input_size,
    size_t *output_size_needed,
    size_t *work_size_needed
);

/* Internal utility functions (used by formatter implementation) */

/*
 * Convert formatter error code to scheduler error code
 */
extern int tecc_formatter_error_to_scheduler_error(TeccFormatterErrorCode fmt_error);

/*
 * Initialize formatter module (optional)
 */
extern void tecc_formatter_initialize(void);

/*
 * Cleanup formatter module (optional)
 */
extern void tecc_formatter_cleanup(void);

/*
 * Zero-copy interfaces for efficient memory management
 */

/*
 * tecc_formatter_normalize_data - Data normalization with buffer (zero-copy)
 *
 * Normalizes data using provided buffer for output.
 * Supports both separate input and in-place operation.
 * For in-place operation, pass buffer->data as input_data and buffer->used as input_len.
 *
 * @param context Encryption context
 * @param input_data Input data to normalize (can be buffer->data for in-place)
 * @param input_len Input data length (can be buffer->used for in-place)
 * @param buffer Output buffer for normalized data
 * @return Error code
 */
extern TeccFormatterErrorCode tecc_formatter_normalize_data(
    TeccCryptoContextCore *context,
    const unsigned char *input_data,
    size_t input_len,
    TeccCryptoBuffer *buffer
);

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
extern size_t tecc_formatter_calculate_normalized_size(
    TeccCryptoContextCore *context,
    size_t input_len
);

/*
 * tecc_formatter_ope_text_pad - Pad text plaintext to 12-byte multiple for OPE
 *
 * ReqID:TECC4.0 - OPE encryption requires fixed-size chunks (12 bytes)
 * for enc_bytes_internal to produce comparable ciphertexts. This function
 * pads the normalized text plaintext with zero bytes to the next 12-byte
 * boundary, ensuring all OPE chunks use consistent msglen.
 *
 * For non-text types, the normalized format already handles alignment.
 *
 * @param input_data Normalized text data (raw text from unified normalization)
 * @param input_len Length of input data
 * @param output_data Pointer to receive padded data (caller must pg_free)
 * @param output_len Pointer to receive padded length
 * @return TeccFormatterErrorCode
 */
extern TeccFormatterErrorCode tecc_formatter_ope_text_pad(
    const unsigned char *input_data,
    size_t input_len,
    unsigned char **output_data,
    size_t *output_len);

/*
 * tecc_formatter_ope_text_icu_pad - Generate ICU sort key and pad for OPE
 *
 * ReqID:TECC4.0 - For text types with collation, this generates the ICU
 * sort key from raw text data and pads to 12-byte multiple for OPE.
 * Combines ICU sort key generation + OPE chunk alignment in one call.
 *
 * @param collation_locale Collation locale (e.g. "en_US.utf8")
 * @param input_data Raw text data
 * @param input_len Length of input data
 * @param output_data Pointer to receive padded sort key (caller must pg_free)
 * @param output_len Pointer to receive padded length
 * @return TeccFormatterErrorCode
 */
extern TeccFormatterErrorCode tecc_formatter_ope_text_icu_pad(
    const char *collation_locale,
    const unsigned char *input_data,
    size_t input_len,
    unsigned char **output_data,
    size_t *output_len);

#endif /* TECC_FORMATTER_H */
/* End - ReqID:TECC4.0 */