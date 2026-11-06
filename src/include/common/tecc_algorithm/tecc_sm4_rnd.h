/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_sm4_rnd.h
 *
 * Abstract:
 *   SM4-RND algorithm header for TECC algorithm module.
 *   Provides authenticated encryption using SM4-CBC and HMAC-SM3.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/common/tecc_algorithm/tecc_sm4_rnd.h
 */

#ifndef TECC_SM4_RND_H
#define TECC_SM4_RND_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include "../../../../src/common/tecc_types.h"

/* Portable packed struct attribute */
#if defined(_MSC_VER)
#define TECC_BEGIN_PACKED __pragma(pack(push, 1))
#define TECC_END_PACKED __pragma(pack(pop))
#define TECC_ATTR_PACKED
#else
#define TECC_BEGIN_PACKED
#define TECC_END_PACKED
#define TECC_ATTR_PACKED __attribute__((packed))
#endif

/* Forward declaration for TeccCryptoBuffer */
typedef struct TeccCryptoBuffer TeccCryptoBuffer;

/* Debug macros */
#ifdef SM4_RND_DEBUG
#define SM4_RND_DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define SM4_RND_DEBUG_PRINT(...) ((void)0)
#endif

/* SM4-RND data format structure */
TECC_BEGIN_PACKED
typedef struct Sm4RndDataFormat
{
    uint8_t     auth_tag[32];   /* Authentication tag: 32 bytes (HMAC-SM3) */
    uint8_t     iv[16];         /* Initialization vector: 16 bytes */
    size_t      ciphertext_len; /* Ciphertext length: variable */
    uint8_t     ciphertext[1];  /* Encrypted data: variable length (SM4-CBC) */
} TECC_ATTR_PACKED Sm4RndDataFormat;
TECC_END_PACKED

/* SM4-RND algorithm error codes */
typedef enum
{
    SM4_RND_SUCCESS = 0,
    SM4_RND_ERROR_INVALID_PARAM = -1,
    SM4_RND_ERROR_MEMORY_ALLOC = -2,
    SM4_RND_ERROR_OPENSSL = -3,
    SM4_RND_ERROR_INVALID_FORMAT = -4,
    SM4_RND_ERROR_AUTH_FAILED = -5,
    SM4_RND_ERROR_DECRYPT_FAILED = -6,
    SM4_RND_ERROR_KEY_DERIVATION = -7,
    SM4_RND_ERROR_INVALID_LENGTH = -8,
    SM4_RND_ERROR_BUFFER_OVERFLOW = -9
} Sm4RndErrorCode;

/*
 * SM4-RND encryption interface - zero-copy version
 * Caller provides pre-allocated output buffer
 */
int tecc_sm4_rnd_encrypt(const unsigned char *key, size_t key_len,
                        const unsigned char *plaintext, size_t plain_len,
                        unsigned char *output_buffer, size_t buffer_size,
                        size_t *output_len);

/*
 * SM4-RND decryption interface - zero-copy version
 * Caller provides pre-allocated output buffer
 */
int tecc_sm4_rnd_decrypt(const unsigned char *key, size_t key_len,
                        const unsigned char *ciphertext, size_t cipher_len,
                        unsigned char *output_buffer, size_t buffer_size,
                        size_t *output_len);

/* ReqID:TECC4.0 — variants accepting pre-derived keys to skip per-call HKDF */
int tecc_sm4_rnd_encrypt_with_keys(const unsigned char *key, size_t key_len,
                        const unsigned char *plaintext, size_t plain_len,
                        unsigned char *output_buffer, size_t buffer_size,
                        size_t *output_len,
                        const unsigned char *derived_enc_key,
                        const unsigned char *derived_auth_key);

int tecc_sm4_rnd_decrypt_with_keys(const unsigned char *key, size_t key_len,
                        const unsigned char *ciphertext, size_t cipher_len,
                        unsigned char *output_buffer, size_t buffer_size,
                        size_t *output_len,
                        const unsigned char *derived_enc_key,
                        const unsigned char *derived_auth_key);

/*
 * Key derivation interface
 */
int tecc_sm4_rnd_derive_keys(const unsigned char *master_key, size_t master_key_len,
                        unsigned char *enc_key, unsigned char *auth_key);

/*
 * Data format validation interface
 */
int tecc_sm4_rnd_validate_format(const unsigned char *ciphertext, size_t cipher_len);

/*
 * Get SM4-RND data format size
 */
size_t tecc_sm4_rnd_get_format_size(size_t plain_len);

/*
 * Clear sensitive memory
 */
void tecc_sm4_rnd_clear_memory(void *data, size_t len);

/* HKDF-SM3 key derivation */
int hkdf_sm3_derive(const unsigned char *salt, size_t salt_len,
                   const unsigned char *ikm, size_t ikm_len,
                   const unsigned char *info, size_t info_len,
                   unsigned char *okm, size_t okm_len);



/* Begin - ReqID:TECC4.0 */
/*
 * SM4-RND batch decryption with pre-derived keys.
 * Shares one SM4Context across all items — eliminates per-row key expansion.
 *
 * On failure, any partially-allocated plaintext buffers are freed.
 * Output plaintexts[i] are malloc'd; caller must free each on success.
 */
int tecc_sm4_rnd_batch_decrypt_with_keys(
    const unsigned char *enc_key,
    const unsigned char *auth_key,
    const unsigned char **ciphertexts,
    const size_t *ciphertext_lens,
    size_t count,
    unsigned char **plaintexts,
    size_t *plaintext_lens);
/* End - ReqID:TECC4.0 */

/* Calculate required buffer size for encryption */
size_t tecc_sm4_rnd_calc_encrypt_buffer_size(size_t plain_len);

/* Calculate required buffer size for decryption */
size_t tecc_sm4_rnd_calc_decrypt_buffer_size(size_t cipher_len);

#endif /* TECC_SM4_RND_H */
/* End - ReqID:TECC4.0 */