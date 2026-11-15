/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_det_sm4.c
 *
 * Abstract:
 *   DET-SM4 algorithm implementation - Deterministic encryption based on SM4 ECB mode.
 *   Implements both traditional encryption/decryption and zero-copy inplace operations.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/common/tecc_algorithm/tecc_det_sm4.c
 */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../../include/common/tecc_algorithm/tecc_sm4_internal.h"
#include "../../../src/common/tecc_types.h"
#include "../../include/common/tecc_algorithm/tecc_det_sm4.h"

/* DET-SM4 data format constants */

/* Maximum data length limit (prevent memory exhaustion attacks) */
#define DET_SM4_MAX_DATA_LEN (256 * 1024) /* 256KB, aligned with TECC_MAX_DATA_SIZE */


/*
 * Clear sensitive data
 */
void
tecc_det_sm4_clear_memory(void *data, size_t len)
{
    /* Simple secure cleanup implementation */
    if (data && len > 0)
    {
        volatile unsigned char *p = data;
        size_t i;
        for (i = 0; i < len; i++)
            p[i] = 0;
    }
}

/*
 * Validate DET-SM4 data format
 */
int
tecc_det_sm4_validate_format(const unsigned char *ciphertext, size_t cipher_len)
{
    size_t cipher_data_len;

    if (!ciphertext || cipher_len == 0)
    {
        return DET_SM4_ERROR_INVALID_PARAM;
    }

    /* Check data length (must be multiple of SM4 block size) */
    cipher_data_len = cipher_len;
    if (cipher_data_len % TECC_SM4_BLOCK_SIZE != 0)
    {
        return DET_SM4_ERROR_INVALID_FORMAT;
    }

    /* Check data length range */
    if (cipher_data_len > DET_SM4_MAX_DATA_LEN)
    {
        return DET_SM4_ERROR_DATA_TOO_LARGE;
    }

    return DET_SM4_SUCCESS;
}








/*
 * Calculate required buffer size for DET-SM4 encryption
 */
size_t
tecc_det_sm4_calc_encrypt_buffer_size(size_t plain_len)
{
    /* Calculate PKCS#7 padding length */
    size_t pad_len = TECC_SM4_BLOCK_SIZE - (plain_len % TECC_SM4_BLOCK_SIZE);
    if (pad_len == 0) pad_len = TECC_SM4_BLOCK_SIZE;

    /* Total size = header + padded ciphertext */
    return plain_len + pad_len;
}

/*
 * Calculate required buffer size for DET-SM4 decryption
 */
size_t
tecc_det_sm4_calc_decrypt_buffer_size(size_t cipher_len)
{
    /* For decryption, we need space for the padded plaintext */
    if (cipher_len == 0)
        return 0;

    {
        size_t cipher_data_len = cipher_len;
        /* Maximum plaintext size is the cipher data length (after removing padding) */
        return cipher_data_len;  /* This includes padding bytes */
    }
}

/*
 * DET-SM4 encryption - zero-copy version, caller provides pre-allocated buffer
 */
int
tecc_det_sm4_encrypt(const unsigned char *key, size_t key_len,
                    const unsigned char *plaintext, size_t plain_len,
                    unsigned char *output_buffer, size_t buffer_size,
                    size_t *output_len)
{
    size_t required_size;
    size_t padded_len;
    size_t pad_len;
    TeccSM4Context *sm4_ctx = NULL;
    size_t pos;

    /* Parameter validation */
    if (!key || key_len != TECC_SM4_KEY_SIZE || !plaintext ||
        !output_buffer || !output_len)
    {
        return DET_SM4_ERROR_INVALID_PARAM;
    }

    if (plain_len > DET_SM4_MAX_DATA_LEN)
    {
        return DET_SM4_ERROR_DATA_TOO_LARGE;
    }

    /* Calculate required buffer size */
    required_size = tecc_det_sm4_calc_encrypt_buffer_size(plain_len);
    if (buffer_size < required_size)
    {
        return DET_SM4_ERROR_MEMORY_ALLOC;
    }

    /* Calculate padding length */
    pad_len = TECC_SM4_BLOCK_SIZE - (plain_len % TECC_SM4_BLOCK_SIZE);
    if (pad_len == 0) pad_len = TECC_SM4_BLOCK_SIZE;
    padded_len = plain_len + pad_len;

    /* Copy plaintext to output buffer (will be encrypted in-place) */
    memcpy(output_buffer, plaintext, plain_len);

    /* Add PKCS#7 padding */
    {
        size_t i;
        for (i = 0; i < pad_len; i++)
        {
            output_buffer[plain_len + i] = (unsigned char)pad_len;
        }
    }

    /* Initialize internal SM4 context for ECB encryption */
    sm4_ctx = tecc_sm4_context_new();
    if (!sm4_ctx)
    {
        return DET_SM4_ERROR_OPENSSL;
    }

    if (tecc_sm4_set_key_encrypt(sm4_ctx, key) != 1)
    {
        tecc_sm4_context_free(sm4_ctx);
        return DET_SM4_ERROR_OPENSSL;
    }

    /* Encrypt each 16-byte block in-place (ECB mode, no chaining) */
    for (pos = 0; pos < padded_len; pos += TECC_SM4_BLOCK_SIZE)
    {
        if (tecc_sm4_ecb_encrypt(sm4_ctx,
                                 output_buffer + pos,
                                 output_buffer + pos) != 1)
        {
            tecc_sm4_context_free(sm4_ctx);
            return DET_SM4_ERROR_OPENSSL;
        }
    }

    /* Clean up */
    tecc_sm4_context_free(sm4_ctx);

    /* Set output length */
    *output_len = padded_len;

    return DET_SM4_SUCCESS;
}

/*
 * DET-SM4 decryption - zero-copy version, caller provides pre-allocated buffer
 */
int
tecc_det_sm4_decrypt(const unsigned char *key, size_t key_len,
                    const unsigned char *ciphertext, size_t cipher_len,
                    unsigned char *output_buffer, size_t buffer_size,
                    size_t *output_len)
{
    int result = DET_SM4_SUCCESS;
    size_t cipher_data_len;
    size_t padded_len;
    unsigned char *padded_text;
    size_t pad_len;
    TeccSM4Context *sm4_ctx = NULL;
    size_t pos;

    /* Parameter validation */
    if (!key || key_len != TECC_SM4_KEY_SIZE || !ciphertext || cipher_len == 0 ||
        !output_buffer || !output_len)
    {
        return DET_SM4_ERROR_INVALID_PARAM;
    }

    /* Validate data format */
    result = tecc_det_sm4_validate_format(ciphertext, cipher_len);
    if (result != DET_SM4_SUCCESS)
    {
        return result;
    }

    cipher_data_len = cipher_len;

    /* Check buffer size */
    if (buffer_size < cipher_data_len)
    {
        return DET_SM4_ERROR_MEMORY_ALLOC;
    }

    /* Copy ciphertext to output buffer for in-place decryption */
    memcpy(output_buffer, ciphertext, cipher_data_len);
    padded_text = output_buffer;
    padded_len = cipher_data_len;

    /* Initialize internal SM4 context for ECB decryption */
    sm4_ctx = tecc_sm4_context_new();
    if (!sm4_ctx)
    {
        return DET_SM4_ERROR_OPENSSL;
    }

    if (tecc_sm4_set_key_decrypt(sm4_ctx, key) != 1)
    {
        tecc_sm4_context_free(sm4_ctx);
        return DET_SM4_ERROR_OPENSSL;
    }

    /* Decrypt each 16-byte block in-place (ECB mode, no chaining) */
    for (pos = 0; pos < padded_len; pos += TECC_SM4_BLOCK_SIZE)
    {
        if (tecc_sm4_ecb_decrypt(sm4_ctx,
                                 padded_text + pos,
                                 padded_text + pos) != 1)
        {
            tecc_sm4_context_free(sm4_ctx);
            return DET_SM4_ERROR_DECRYPT_FAILED;
        }
    }

    /* Clean up */
    tecc_sm4_context_free(sm4_ctx);

    /* Validate PKCS#7 padding */
    if (padded_len == 0 || padded_len % TECC_SM4_BLOCK_SIZE != 0)
    {
        return DET_SM4_ERROR_DECRYPT_FAILED;
    }

    pad_len = padded_text[padded_len - 1];
    if (pad_len == 0 || pad_len > TECC_SM4_BLOCK_SIZE || pad_len > padded_len)
    {
        return DET_SM4_ERROR_DECRYPT_FAILED;
    }

    /* Verify all padding bytes */
    {
        size_t i;
        for (i = 1; i <= pad_len; i++)
        {
            if (padded_text[padded_len - i] != pad_len)
            {
                return DET_SM4_ERROR_DECRYPT_FAILED;
            }
        }
    }

    /* Calculate original data length */
    *output_len = padded_len - pad_len;

    /* Note: The plaintext is at the beginning of output_buffer,
     * but the buffer still contains padding bytes after *output_len.
     * Caller should use *output_len to determine actual plaintext length.
     */

    return DET_SM4_SUCCESS;
}

/* Begin - ReqID:TECC4.0 */
/*
 * DET algorithm normalization functions
 * These functions use the common normalization implementation
 */








/* End - ReqID:TECC4.0 */