/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_sm4_ope.c
 *
 * Abstract:
 *   SM4-OPE algorithm implementation - Order-Preserving Encryption using SM4.
 *   Provides encryption that preserves the order of plaintext values.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/common/tecc_algorithm/tecc_sm4_ope.c
 */

#include "../../../src/common/tecc_types.h"
#include "../../include/common/tecc_algorithm/tecc_sm4_ope.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../../include/common/tecc_algorithm/tecc_sm4_internal.h"

/* Constants */
#define SM4_OPE_MAX_DATA_LEN (256 * 1024) /* 256KB, aligned with TECC_MAX_DATA_SIZE */
#define SM4_OPE_CHUNK_SIZE   12           /* enc_bytes_internal handles up to 12 bytes per chunk */



/*
 * Convert int32 to byte array (big-endian)
 */
static void
trans_i32tou8(int in, uint8_t *out)
{
    int k = sizeof(int);
    int i;
    for (i = 0; i < k; i++)
    {
        out[i] = (uint8_t)(in >> (8 * (k - i - 1)));
    }
}

/*
 * SM4-OPE core encryption function (internal use)
 * Performs bit-level encryption that preserves order
 */
static int
enc_bytes_internal(uint8_t *msg, int msglen, uint8_t *key, int keylen, uint8_t *out)
{
    TeccSM4Context *ctx = NULL;
    uint8_t encin[16] = {0};
    uint8_t encout[16] = {0};
    uint8_t FF = 0xFF;
    uint8_t one;
    uint16_t getover;
    int i_th = 0;
    int result = 0;
    int i;
    int out_idx1;
    int out_idx2;

    /* Input parameter validation */
    if (!msg || msglen <= 0 || !key || keylen <= 0 || !out)
    {
        SM4_OPE_DEBUG_PRINT("SM4-OPE DEBUG[internal]: Invalid parameters\n");
        return 0;
    }

    /* Check max chunk size */
    if (msglen > SM4_OPE_CHUNK_SIZE)
    {
        SM4_OPE_DEBUG_PRINT("SM4-OPE DEBUG[internal]: Chunk too large: %d > %d\n", msglen, SM4_OPE_CHUNK_SIZE);
        return 0;
    }

    /* Create SM4 context */
    ctx = tecc_sm4_context_new();
    if (!ctx)
    {
        SM4_OPE_DEBUG_PRINT("SM4-OPE DEBUG[internal]: Failed to create SM4 context\n");
        return 0;
    }

    /* Initialize SM4 encryption once */
    if (tecc_sm4_set_key_encrypt(ctx, key) != 1)
    {
        SM4_OPE_DEBUG_PRINT("SM4-OPE DEBUG[internal]: Failed to set SM4 key\n");
        tecc_sm4_context_free(ctx);
        return 0;
    }

    i_th = (8 * msglen - 1) / 8;

    /* Process highest bit */
    trans_i32tou8(8 * msglen - 1, encin);
    memcpy(encin + sizeof(int), msg, i_th);
    one = FF << 1;
    encin[sizeof(int) + i_th] = msg[i_th] & one;
    tecc_sm4_ecb_encrypt(ctx, encin, encout);
    out[2 * msglen - 1] = (encout[15] & 0x1) + (msg[msglen - 1] & 0x1);

    /* Process remaining bits from high to low */
    for (i = 8 * msglen - 2; i >= 0; i--)
    {
        memset(encin, 0, 16);
        i_th = i / 8;

        trans_i32tou8(i, encin);
        memcpy(encin + sizeof(int), msg, i_th);
        one = FF << (8 - i % 8);
        encin[sizeof(int) + i_th] = msg[i_th] & one;
        tecc_sm4_ecb_encrypt(ctx, encin, encout);

        out_idx1 = i / 4 + 1;
        out_idx2 = i / 4;
        if (i % 4 == 3)
        {
            getover = out[out_idx1] + ((uint16_t)(encout[15] & 0x3) << 7) +
                     (((uint16_t)((msg[i / 8]) >> (7 - (i % 8))) & 0x1) << 8);
            out[out_idx1] = getover & FF;
            out[out_idx2] = (getover >> 8) & FF;
        }
        else
        {
            out[out_idx2] = out[out_idx2] +
                           ((encout[15] & 0x3) << (5 - ((2 * i) % 8))) +
                           (((msg[i / 8] >> (7 - (i % 8))) & 0x1) << (6 - ((2 * i) % 8)));
        }
    }

    result = 1;
    tecc_sm4_context_free(ctx);
    return result;
}

/*
 * OPE ciphertext comparison (order-preserving property)
 */
int
OPE_cmp(const uint8_t *ctx_1, const uint8_t *ctx_2, int ctx_len)
{
    int i;

    if (!ctx_1 || !ctx_2 || ctx_len <= 0)
    {
        return 0;
    }

    for (i = 0; i < ctx_len; i++)
    {
        if (ctx_1[i] > ctx_2[i])
        {
            return 1;
        }
        else if (ctx_1[i] < ctx_2[i])
        {
            return -1;
        }
    }
    return 0;
}

/*
 * Clear sensitive memory
 */
void
tecc_sm4_ope_clear_memory(void *data, size_t len)
{
    /* Simple secure cleanup implementation */
    if (data && len > 0)
    {
        memset(data, 0, len);
    }
}

/*
 * Validate SM4-OPE data format
 */
int
tecc_sm4_ope_validate_format(const unsigned char *ciphertext, size_t cipher_len)
{
    if (!ciphertext || cipher_len == 0)
    {
        return SM4_OPE_ERROR_INVALID_PARAM;
    }

    /* Check data length limit */
    if (cipher_len > SM4_OPE_MAX_DATA_LEN * 2)
    {
        return SM4_OPE_ERROR_DATA_TOO_LARGE;
    }

    /* OPE ciphertext length must be even (because output is twice the input) */
    if (cipher_len % 2 != 0)
    {
        return SM4_OPE_ERROR_INVALID_FORMAT;
    }

    return SM4_OPE_SUCCESS;
}








/* Begin - ReqID:TECC4.0 */
/*
 * OPE algorithm normalization functions
 * These functions use the common normalization implementation
 */






/*
 * SM4-OPE encryption - zero-copy version, caller provides pre-allocated buffer
 */
/*
 * SM4-OPE encryption - zero-copy version, caller provides pre-allocated buffer
 * Supports arbitrary-length plaintext by chunked encryption (12 bytes per chunk).
 */
int
tecc_sm4_ope_encrypt(const unsigned char *key, size_t key_len,
                    const unsigned char *plaintext, size_t plain_len,
                    unsigned char *output_buffer, size_t buffer_size,
                    size_t *output_len)
{
    size_t total_len;
    size_t offset = 0;
    size_t chunk_size;

    /* Parameter validation */
    if (!key || key_len != TECC_SM4_KEY_SIZE || !plaintext || plain_len == 0 ||
        !output_buffer || !output_len)
    {
        SM4_OPE_DEBUG_PRINT("SM4-OPE DEBUG: Invalid parameters\n");
        return SM4_OPE_ERROR_INVALID_PARAM;
    }

    /* Check data length limit */
    if (plain_len > SM4_OPE_MAX_DATA_LEN)
    {
        SM4_OPE_DEBUG_PRINT("SM4-OPE DEBUG: Data too large: %zu > %d\n", plain_len, SM4_OPE_MAX_DATA_LEN);
        return SM4_OPE_ERROR_DATA_TOO_LARGE;
    }

    /* Calculate total length: OPE output is twice the input size */
    total_len = plain_len * 2;

    /* Check if output buffer is large enough */
    if (buffer_size < total_len)
    {
        SM4_OPE_DEBUG_PRINT("SM4-OPE DEBUG: Buffer too small: %zu < %zu\n", buffer_size, total_len);
        return SM4_OPE_ERROR_MEMORY_ALLOC;
    }

    /* Initialize output length */
    *output_len = 0;

    SM4_OPE_DEBUG_PRINT("SM4-OPE DEBUG: Starting encryption: plain_len=%zu, total_len=%zu\n", plain_len, total_len);
    SM4_OPE_DEBUG_PRINT("SM4-OPE DEBUG: Key (first 8 bytes): ");
    {
        size_t i;
        for (i = 0; i < 8 && i < key_len; i++) {
            SM4_OPE_DEBUG_PRINT("%02X", key[i]);
        }
    }
    SM4_OPE_DEBUG_PRINT("\n");

    /* Encrypt in chunks of SM4_OPE_CHUNK_SIZE (12 bytes) */
    while (offset < plain_len)
    {
        size_t remain = plain_len - offset;
        int ret;

        if (remain > SM4_OPE_CHUNK_SIZE)
            chunk_size = SM4_OPE_CHUNK_SIZE;
        else
            chunk_size = remain;

        ret = enc_bytes_internal((uint8_t *)plaintext + offset, (int)chunk_size,
                                 (uint8_t *)key, (int)key_len,
                                 output_buffer + offset * 2);
        if (!ret)
        {
            SM4_OPE_DEBUG_PRINT("SM4-OPE DEBUG: enc_bytes_internal failed at offset=%zu\n", offset);
            tecc_sm4_ope_clear_memory(output_buffer, buffer_size);
            *output_len = 0;
            return SM4_OPE_ERROR_ENCRYPT_FAILED;
        }

        offset += chunk_size;
    }

    /* Success: set output length */
    *output_len = total_len;
    SM4_OPE_DEBUG_PRINT("SM4-OPE DEBUG: Encryption successful, output_len=%zu\n", total_len);

    return SM4_OPE_SUCCESS;
}


/*
 * Calculate required buffer size for encryption
 */
size_t
tecc_sm4_ope_calc_encrypt_buffer_size(size_t plain_len)
{
    /* OPE algorithm output size is twice the input size */
    if (plain_len > SM4_OPE_MAX_DATA_LEN)
    {
        return 0; /* Invalid size */
    }

    return plain_len * 2;
}

/*
 * Recover one OPE chunk plaintext from ciphertext via order-preserving
 * bit search (MSB-first).  Returns 1 on success.
 * ReqID:TECC4.0
 */
static int
dec_bytes_internal(const uint8_t *ct, int ctlen, const uint8_t *key, int keylen,
                   uint8_t *out)
{
    int         n;
    int         bit;
    int         byte_i;
    int         shift;
    uint8_t     tmp_ct[2 * SM4_OPE_CHUNK_SIZE];

    if (!ct || ctlen <= 0 || (ctlen % 2) || !key || !out)
        return 0;

    n = ctlen / 2;
    if (n > SM4_OPE_CHUNK_SIZE)
        return 0;

    memset(out, 0, (size_t)n);
    for (bit = 0; bit < n * 8; bit++)
    {
        byte_i = bit / 8;
        shift = 7 - (bit % 8);
        out[byte_i] |= (uint8_t)(1u << shift);

        if (!enc_bytes_internal(out, n, (uint8_t *) key, keylen, tmp_ct))
            return 0;

        /* Order-preserving: candidate > target means this bit must be 0. */
        if (OPE_cmp(tmp_ct, ct, ctlen) > 0)
            out[byte_i] &= (uint8_t)~(1u << shift);
    }

    if (!enc_bytes_internal(out, n, (uint8_t *) key, keylen, tmp_ct))
        return 0;
    return OPE_cmp(tmp_ct, ct, ctlen) == 0;
}

/*
 * SM4-OPE decryption - recover the normalized plaintext that was fed to
 * OPE encryption (debug / tecc_decrypt_raw path; not a business original).
 * Chunk layout mirrors tecc_sm4_ope_encrypt (12-byte plaintext chunks).
 * ReqID:TECC4.0
 */
int
tecc_sm4_ope_decrypt(const unsigned char *key, size_t key_len,
                     const unsigned char *ciphertext, size_t cipher_len,
                     unsigned char *output_buffer, size_t buffer_size,
                     size_t *output_len)
{
    size_t      total_plain;
    size_t      offset = 0;
    size_t      chunk_size;

    if (!key || key_len != TECC_SM4_KEY_SIZE || !ciphertext || cipher_len == 0 ||
        !output_buffer || !output_len)
    {
        SM4_OPE_DEBUG_PRINT("SM4-OPE DEBUG: Invalid decrypt parameters\n");
        return SM4_OPE_ERROR_INVALID_PARAM;
    }

    if (cipher_len % 2 != 0)
        return SM4_OPE_ERROR_INVALID_FORMAT;

    total_plain = cipher_len / 2;
    if (total_plain > SM4_OPE_MAX_DATA_LEN)
        return SM4_OPE_ERROR_DATA_TOO_LARGE;

    if (buffer_size < total_plain)
    {
        SM4_OPE_DEBUG_PRINT("SM4-OPE DEBUG: Decrypt buffer too small: %zu < %zu\n",
                            buffer_size, total_plain);
        return SM4_OPE_ERROR_MEMORY_ALLOC;
    }

    *output_len = 0;

    while (offset < total_plain)
    {
        size_t      remain = total_plain - offset;

        chunk_size = (remain > SM4_OPE_CHUNK_SIZE) ? SM4_OPE_CHUNK_SIZE : remain;

        if (!dec_bytes_internal(ciphertext + offset * 2, (int) (chunk_size * 2),
                                key, (int) key_len, output_buffer + offset))
        {
            SM4_OPE_DEBUG_PRINT("SM4-OPE DEBUG: dec_bytes_internal failed at offset=%zu\n",
                                offset);
            tecc_sm4_ope_clear_memory(output_buffer, buffer_size);
            *output_len = 0;
            return SM4_OPE_ERROR_DECRYPT_FAILED;
        }

        offset += chunk_size;
    }

    *output_len = total_plain;
    return SM4_OPE_SUCCESS;
}

/*
 * Calculate required buffer size for decryption
 */
size_t
tecc_sm4_ope_calc_decrypt_buffer_size(size_t cipher_len)
{
    /* ReqID:TECC4.0 - OPE ciphertext is twice the plaintext length */
    if (cipher_len == 0 || (cipher_len % 2) != 0)
        return 0;

    return cipher_len / 2;
}

/* End - ReqID:TECC4.0 */
