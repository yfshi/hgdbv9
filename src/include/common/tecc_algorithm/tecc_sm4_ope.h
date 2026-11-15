/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_sm4_ope.h
 *
 * Abstract:
 *   SM4-OPE algorithm header for TECC algorithm module.
 *   Provides order-preserving encryption using SM4.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/common/tecc_algorithm/tecc_sm4_ope.h
 */

#ifndef TECC_SM4_OPE_H
#define TECC_SM4_OPE_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include "../../../../src/common/tecc_types.h"

/* For context-parameter functions, include algorithm API in implementation files */

/* Debug macros */
#ifdef SM4_OPE_DEBUG
#define SM4_OPE_DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define SM4_OPE_DEBUG_PRINT(...) ((void)0)
#endif

/* SM4-OPE algorithm error codes */
typedef enum
{
    SM4_OPE_SUCCESS = 0,
    SM4_OPE_ERROR_INVALID_PARAM = -1,
    SM4_OPE_ERROR_MEMORY_ALLOC = -2,
    SM4_OPE_ERROR_ENCRYPT_FAILED = -3,
    SM4_OPE_ERROR_DECRYPT_FAILED = -4,
    SM4_OPE_ERROR_DATA_TOO_LARGE = -5,
    SM4_OPE_ERROR_INVALID_FORMAT = -6,
    SM4_OPE_ERROR_DECRYPT_NOT_SUPPORTED = -7,  /* Reserved (unused) */
    SM4_OPE_ERROR_OPENSSL = -8,
    SM4_OPE_ERROR_KEY_DERIVATION = -9
} Sm4OpeErrorCode;

/*
 * SM4-OPE encryption interface - 零拷贝版本，调用者提供预分配的输出缓冲区
 */
int tecc_sm4_ope_encrypt(const unsigned char *key, size_t key_len,
                        const unsigned char *plaintext, size_t plain_len,
                        unsigned char *output_buffer, size_t buffer_size,
                        size_t *output_len);

/*
 * SM4-OPE decryption - recover normalized OPE input (debug path).
 * ReqID:TECC4.0
 */
int tecc_sm4_ope_decrypt(const unsigned char *key, size_t key_len,
                         const unsigned char *ciphertext, size_t cipher_len,
                         unsigned char *output_buffer, size_t buffer_size,
                         size_t *output_len);


/*
 * OPE ciphertext comparison (order-preserving property)
 */
int OPE_cmp(const uint8_t *ctx_1, const uint8_t *ctx_2, int ctx_len);

/*
 * Data format validation interface
 */
int tecc_sm4_ope_validate_format(const unsigned char *ciphertext, size_t cipher_len);

/*
 * Clear sensitive memory
 */
void tecc_sm4_ope_clear_memory(void *data, size_t len);


/* Calculate required buffer size for encryption */
size_t tecc_sm4_ope_calc_encrypt_buffer_size(size_t plain_len);

/* Calculate required buffer size for decryption */
size_t tecc_sm4_ope_calc_decrypt_buffer_size(size_t cipher_len);


/*
 * OPE algorithm normalization functions
 * These functions normalize data for order-preserving encryption
 */



#endif /* TECC_SM4_OPE_H */
/* End - ReqID:TECC4.0 */