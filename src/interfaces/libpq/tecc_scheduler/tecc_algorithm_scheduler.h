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
/* ReqID:TECC4.0 - HE 批量解密并发：外部 BN_CTX/BIGNUM 类型（私钥预取 + BN_CTX 复用） */
#include <openssl/bn.h>

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
extern int tecc_execute_algo_he_add(TeccCryptoContext *context);
extern int tecc_execute_algo_he_mul(TeccCryptoContext *context);

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
                                      bool is_wildcard_pattern,
                                      char escape_char);
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
/* ReqID:TECC4.0 - Decrypt HE-only cipherdata (EG_ADD or EG_MUL slot).
 * Returns decimal string in *plaintext (palloc'd), length in *plain_len.
 * dek_name/state are used to load the DEK's real HE private key (problem 3);
 * pass NULL to fall back to the hardcoded test key. */
extern int tecc_he_decrypt_cipherdata(const unsigned char *eg_add_data, size_t eg_add_len,
                                      const unsigned char *eg_mul_data, size_t eg_mul_len,
                                      const char *dek_name, void *tecc_state,
                                      char **plaintext, size_t *plain_len);

/* ReqID:TECC4.0 - Load a DEK's real HE private key (EC for he-add, FF for he-mul),
 * unwrapped with the KEK via SM2 envelope decryption.  Returns 0 on success with
 * *out_priv allocated (caller frees with BN_free); -1 on error.  Used by batch
 * decrypt to load the key once per DEK instead of per row. */
extern int tecc_he_load_dek_privkey(void *tecc_state, const char *dek_name,
                                    int is_ec, BIGNUM **out_priv);

/* ReqID:TECC4.0 - Pure HE decrypt core (no private-key load, no BN_CTX create/free).
 * Accepts an externally owned BN_CTX and a pre-loaded private key (may be NULL to
 * fall back to the hardcoded test key), so batch decrypt can reuse one BN_CTX and
 * one private key across all rows.  Returns decimal string in *plaintext. */
extern int tecc_he_decrypt_cipherdata_ex(const unsigned char *eg_add_data, size_t eg_add_len,
                                         const unsigned char *eg_mul_data, size_t eg_mul_len,
                                         const BIGNUM *per_dek_priv, BN_CTX *bn_ctx,
                                         char **plaintext, size_t *plain_len);

/* ReqID:TECC4.0 - Load a DEK's HE key material (he_pubkey / he_privkey) from
 * hg_dek_args. Returns 0 on success with *hex_out allocated, -1 on error.
 * 带元数据缓存（字段级并行前主线程预热，worker 内命中不碰主连接）。 */
int tecc_he_load_dek_key_hex(void *state, const char *dek_name,
                             const char *key, char **hex_out);

/* ReqID:TECC4.0 - Bug 9263: 清空 HE 元数据缓存（tecc_crypto_scheduler.c 内的
 * 进程级静态缓存）。ALTER KEK 轮换后用新 KEK 公钥重封了 HE 私钥信封，而该缓存
 * 里存的正是【旧信封 hex】，不清就会出现"旧信封 + 新 KEK"配错解封。 */
void tecc_he_meta_cache_clear(void);

/* ReqID:TECC4.0 - P0-2 一列多 DEK：查询列的 he-add/he-mul DEK 名称
 * （hg_encrypted_columns_info 视图，按 table_oid+column_attnum）。
 * 返回 0 成功（name_out 填充），-1 无此 DEK。带元数据缓存（同公钥缓存）。 */
int tecc_load_column_he_dek_name(void *state, Oid table_oid, int16 column_attnum,
                                 const char *key, char *name_out, size_t name_out_size);

/* ReqID:TECC4.0 - Load a KEK arg (key_algo / public_key) from hg_kek_args.
 * Returns 0 on success with *hex_out allocated, -1 on error. */
int tecc_he_load_kek_arg_hex(void *state, const char *kek_name,
                             const char *key, char **hex_out);
