/*
 * File: tecc_det_sm4.h
 * Abstract: DET-SM4国密算法头文件 - 基于SM4 ECB模式的确定性加密算法
 * Authored by <wangxp@highgo.com>, 20260312.
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 * Identification: src/common/crypto/tecc_det_sm4.h
 */

#ifndef TECC_DET_SM4_H
#define TECC_DET_SM4_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include "../../../../src/common/tecc_types.h"

/* Buffer management - not currently used */
/* #include "tecc_algo_buffer.h" */

/* For context-parameter functions, include algorithm API in implementation files */

/* ReqID:TECC4.0 */

/* DET-SM4算法错误码 */
typedef enum
{
    DET_SM4_SUCCESS = 0,
    DET_SM4_ERROR_INVALID_PARAM = -1,
    DET_SM4_ERROR_MEMORY_ALLOC = -2,
    DET_SM4_ERROR_OPENSSL = -3,
    DET_SM4_ERROR_INVALID_FORMAT = -4,
    DET_SM4_ERROR_DECRYPT_FAILED = -5,
    DET_SM4_ERROR_NORMALIZATION = -6,
    DET_SM4_ERROR_DATA_TOO_LARGE = -7,
    DET_SM4_ERROR_INVALID_LENGTH = -8,
    DET_SM4_ERROR_KEY_DERIVATION = -9
} DetSm4ErrorCode;

/* DET-SM4加密接口 - 零拷贝版本，调用者提供预分配的输出缓冲区 */
extern int tecc_det_sm4_encrypt(const unsigned char *key, size_t key_len,
                                const unsigned char *plaintext, size_t plain_len,
                                unsigned char *output_buffer, size_t buffer_size,
                                size_t *output_len);

/* DET-SM4解密接口 - 零拷贝版本，调用者提供预分配的输出缓冲区 */
extern int tecc_det_sm4_decrypt(const unsigned char *key, size_t key_len,
                                const unsigned char *ciphertext, size_t cipher_len,
                                unsigned char *output_buffer, size_t buffer_size,
                                size_t *output_len);

/* 数据格式验证接口 */
extern int tecc_det_sm4_validate_format(const unsigned char *ciphertext, size_t cipher_len);

/* 清除敏感数据 */
extern void tecc_det_sm4_clear_memory(void *data, size_t len);


/* Calculate required buffer size for encryption */
extern size_t tecc_det_sm4_calc_encrypt_buffer_size(size_t plain_len);

/* Calculate required buffer size for decryption */
extern size_t tecc_det_sm4_calc_decrypt_buffer_size(size_t cipher_len);


#endif /* TECC_DET_SM4_H */