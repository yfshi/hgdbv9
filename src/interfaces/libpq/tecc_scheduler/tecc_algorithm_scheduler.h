/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_algorithm_scheduler.h
 *
 * Abstract:
 *   Algorithm scheduler interface for TECC crypto scheduler module.
 *   Provides algorithm selection and execution scheduling.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_scheduler/tecc_algorithm_scheduler.h
 */

#ifndef TECC_ALGORITHM_SCHEDULER_H
#define TECC_ALGORITHM_SCHEDULER_H

#include "postgres_fe.h"
#include "c.h"
#include "../tecc_common/tecc_common_types.h"
#include "tecc_crypto_context.h"
#include "../../../common/tecc_types.h"  /* For TeccAlgorithmErrorCode */

/* Use TeccAlgorithmErrorCode from tecc_types.h instead of local TeccAlgorithmResult */
/* TeccAlgorithmErrorCode provides comprehensive error codes for algorithm operations */

/* Algorithm selection criteria */
typedef struct TeccAlgorithmSelection
{
    bool require_det;       /* Require DET algorithm */
    bool require_ope;       /* Require OPE algorithm */
    bool require_search;    /* Require SEARCH algorithm */
    bool allow_fallback;    /* Allow fallback to basic algorithms */
    int priority;          /* Algorithm priority (higher = more preferred) */
} TeccAlgorithmSelection;

/* Algorithm capabilities */
typedef struct TeccAlgorithmCaps
{
    bool supports_encryption;   /* Supports encryption */
    bool supports_decryption;   /* Supports decryption */
    bool supports_comparison;   /* Supports comparison (for OPE) */
    bool supports_search;      /* Supports search operations */
    size_t max_input_size;     /* Maximum input size */
    size_t output_overhead;    /* Output size overhead */
} TeccAlgorithmCaps;

/* Function prototypes */

/* Algorithm selection */
extern int tecc_select_algorithms(TeccCryptoContext *context);
extern int tecc_select_algorithms_with_criteria(TeccCryptoContext *context,
                                               const TeccAlgorithmSelection *criteria);
extern bool tecc_is_algorithm_supported(const TeccCryptoContext *context,
                                       TeccAlgorithmType algo_type);

/* Algorithm execution */
extern int tecc_execute_algorithms(TeccCryptoContext *context);
extern int tecc_execute_algorithms(TeccCryptoContext *context);
extern int tecc_execute_encryption(TeccCryptoContext *context);
extern int tecc_execute_decryption(TeccCryptoContext *context);

/* Individual algorithm execution */
/* ReqID:TECC4.0 - derived_enc_key/auth_key: pre-derived SM4-RND keys from cache.
 * Pass NULL to derive internally (backward compatible). */
extern int tecc_execute_algo_rnd(TeccCryptoContext *context,
                                 const unsigned char *derived_enc_key,
                                 const unsigned char *derived_auth_key);
extern int tecc_execute_algo_det(TeccCryptoContext *context);
extern int tecc_execute_algo_ope(TeccCryptoContext *context);
extern int tecc_execute_algo_search(TeccCryptoContext *context);

/* Algorithm information */
extern const char *tecc_get_algorithm_name(TeccAlgorithmType algo_type);
extern TeccAlgorithmCaps tecc_get_algorithm_capabilities(TeccAlgorithmType algo_type);
extern bool tecc_algorithm_requires_collation(TeccAlgorithmType algo_type);

/* Algorithm validation */
extern bool tecc_validate_algorithm_selection(const TeccCryptoContext *context);
extern int tecc_check_algorithm_compatibility(const TeccCryptoContext *context,
                                             TeccAlgorithmType algo_type);

/* Error handling */
extern const char *tecc_get_algorithm_error_message(int error_code);
extern void tecc_handle_algorithm_failure(TeccCryptoContext *context,
                                         TeccAlgorithmType algo_type,
                                         int error_code);

/* Utility functions */
extern int tecc_count_selected_algorithms(const TeccCryptoContext *context);
extern TeccAlgorithmType tecc_get_primary_algorithm(const TeccCryptoContext *context);
extern bool tecc_has_mandatory_algorithms(const TeccCryptoContext *context);

/* Forward declarations for algorithm API functions (zero-copy interfaces) */
extern int tecc_algo_encrypt_zero_copy(TeccAlgorithmType algo_type,
                                      const unsigned char *key,
                                      size_t key_len,
                                      const unsigned char *plaintext,
                                      size_t plain_len,
                                      unsigned char *output_buffer,
                                      size_t buffer_size,
                                      size_t *output_len,
                                      bool is_wildcard_pattern);
extern int tecc_algo_decrypt_zero_copy(TeccAlgorithmType algo_type,
                                      const unsigned char *key,
                                      size_t key_len,
                                      const unsigned char *ciphertext,
                                      size_t cipher_len,
                                      unsigned char *output_buffer,
                                      size_t buffer_size,
                                      size_t *output_len);
extern size_t tecc_algo_calc_encrypt_buffer_size(TeccAlgorithmType algo_type,
                                                size_t plain_len);
extern size_t tecc_algo_calc_decrypt_buffer_size(TeccAlgorithmType algo_type,
                                                size_t cipher_len);

/* Extended zero-copy interface with operator type support */
extern int tecc_algo_encrypt_zero_copy_ex(TeccAlgorithmType algo_type,
                                         TeccOperatorType operator_type,
                                         const unsigned char *key,
                                         size_t key_len,
                                         const unsigned char *plaintext,
                                         size_t plain_len,
                                         unsigned char *output_buffer,
                                         size_t buffer_size,
                                         size_t *output_len);

/* SEARCH algorithm encryption with operator type support */
extern int tecc_algorithm_scheduler_encrypt_search_with_operator(
    TeccCryptoContext *context,
    TeccOperatorType operator_type,
    unsigned char **search_ciphertext,
    size_t *search_cipher_len);

/* Individual algorithm encryption functions */
extern int tecc_algorithm_scheduler_encrypt_rnd(
    TeccCryptoContext *context,
    unsigned char **rnd_ciphertext,
    size_t *rnd_cipher_len,
    const unsigned char *derived_enc_key,
    const unsigned char *derived_auth_key);

#endif /* TECC_ALGORITHM_SCHEDULER_H */
/* End - ReqID:TECC4.0 */