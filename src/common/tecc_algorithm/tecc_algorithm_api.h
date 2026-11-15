/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_algorithm_api.h
 *
 * Abstract:
 *   Unified algorithm API for TECC encryption/decryption algorithm module.
 *   Provides standardized interfaces for all TECC algorithms (RND, DET, OPE, SEARCH).
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/common/tecc_algorithm/tecc_algorithm_api.h
 */

#ifndef TECC_ALGORITHM_API_H
#define TECC_ALGORITHM_API_H

#include <stddef.h>
#include <stdint.h>
#include <openssl/evp.h>

/* C90 doesn't have stdbool.h, define bool ourselves */
#ifndef __cplusplus
typedef unsigned char bool;
#define true 1
#define false 0
#endif
#include <openssl/hmac.h>
#include <openssl/rand.h>

/* Include shared types */
#include "../../../src/common/tecc_types.h"

/* Algorithm capability flags and error codes are now defined in tecc_types.h */



/* Constants */
#define TECC_MAX_KEY_SIZE        32     /* Maximum key length */
#define TECC_MAX_DATA_SIZE       (256 * 1024)  /* Maximum data length (256KB) */
#define TECC_SM4_BLOCK_SIZE      16     /* SM4 block size */
#define TECC_SM4_KEY_SIZE        16     /* SM4 key length */
#define TECC_HMAC_SM3_SIZE       32     /* HMAC-SM3 output length */

/* SM4-RND constants - now defined in tecc_types.h */
/* DET-SM4 constants - now defined in tecc_types.h */

/* Function prototypes */



/* Algorithm capability and information */
extern uint32 tecc_algo_get_capability(TeccAlgorithmType algo_type);
extern const char *tecc_algo_get_error_string(int error_code);


/* Error code conversion */
extern int tecc_algo_error_to_scheduler_error(int algo_error);
extern int tecc_algo_error_to_formatter_error(int algo_error);

/* Utility functions */
extern void tecc_algo_secure_cleanup(void *data, size_t len);
extern int tecc_algo_constant_time_memcmp(const void *s1, const void *s2, size_t n);

/* Zero-copy algorithm interface (与算法模块接口兼容) */
extern int tecc_algo_encrypt_zero_copy(TeccAlgorithmType algo_type,
                                      const unsigned char *key,
                                      size_t key_len,
                                      const unsigned char *plaintext,
                                      size_t plain_len,
                                      unsigned char *output_buffer,
                                      size_t buffer_size,
                                      size_t *output_len);

extern int tecc_algo_decrypt_zero_copy(TeccAlgorithmType algo_type,
                                      const unsigned char *key,
                                      size_t key_len,
                                      const unsigned char *ciphertext,
                                      size_t cipher_len,
                                      unsigned char *output_buffer,
                                      size_t buffer_size,
                                      size_t *output_len);

/* Buffer size calculation functions */
extern size_t tecc_algo_calc_encrypt_buffer_size(TeccAlgorithmType algo_type,
                                                size_t plain_len);

extern size_t tecc_algo_calc_decrypt_buffer_size(TeccAlgorithmType algo_type,
                                                size_t cipher_len);

#endif /* TECC_ALGORITHM_API_H */
/* End - ReqID:TECC4.0 */