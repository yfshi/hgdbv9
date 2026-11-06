/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_sm4_rnd.h
 *
 * Abstract:
 *   SM4-RND algorithm header file - SM4-based authenticated encryption algorithm.
 *   Provides fixed-random encryption with HMAC-SM3 authentication.
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
#include <openssl/evp.h>

/* C90 doesn't have stdbool.h, define bool ourselves */
#ifndef __cplusplus
#ifndef bool
typedef unsigned char bool;
#define true 1
#define false 0
#endif
#endif
#include <openssl/hmac.h>
#include <openssl/rand.h>

/* Include shared types */
#include "../../../common/tecc_types.h"


/* SM4-RND algorithm error codes */
typedef enum Sm4RndErrorCode
{
    SM4_RND_SUCCESS = 0,
    SM4_RND_ERROR_INVALID_PARAM = -1,
    SM4_RND_ERROR_MEMORY_ALLOC = -2,
    SM4_RND_ERROR_OPENSSL = -3,
    SM4_RND_ERROR_INVALID_FORMAT = -4,
    SM4_RND_ERROR_AUTH_FAILED = -5,
    SM4_RND_ERROR_DECRYPT_FAILED = -6,
    SM4_RND_ERROR_KEY_DERIVATION = -7,
    SM4_RND_ERROR_CRYPTO_FAILED = -8,
    SM4_RND_ERROR_INVALID_LENGTH = -9,
    SM4_RND_ERROR_BUFFER_OVERFLOW = -10
} Sm4RndErrorCode;

/* SM4-RND data format structure */
typedef struct Sm4RndDataFormat
{
    uint32 version;        /* Version number: 4 bytes */
    unsigned char auth_tag[32];  /* Authentication tag: 32 bytes (HMAC-SM3) */
    unsigned char iv[16];    /* Initialization vector: 16 bytes */
    size_t ciphertext_len;   /* Ciphertext length */
    unsigned char ciphertext[FLEXIBLE_ARRAY_MEMBER]; /* Variable-length ciphertext */
} Sm4RndDataFormat;

/* SM4-RND algorithm constants - now defined in tecc_types.h */

/* Function prototypes */

/* Traditional encryption/decryption interface */
extern int tecc_sm4_rnd_encrypt(const unsigned char *plaintext, size_t plain_len,
                               const unsigned char *key,
                               unsigned char **ciphertext, size_t *cipher_len);

extern int tecc_sm4_rnd_decrypt(const unsigned char *ciphertext, size_t cipher_len,
                               const unsigned char *key,
                               unsigned char **plaintext, size_t *plain_len);


/* Key derivation */
extern int tecc_sm4_rnd_derive_keys(const unsigned char *master_key, size_t key_len,
                                   unsigned char *enc_key, unsigned char *auth_key);

/* ReqID:TECC4.0 — zero-copy variants with pre-derived keys */
extern int tecc_sm4_rnd_encrypt_with_keys(const unsigned char *key, size_t key_len,
                        const unsigned char *plaintext, size_t plain_len,
                        unsigned char *output_buffer, size_t buffer_size,
                        size_t *output_len,
                        const unsigned char *derived_enc_key,
                        const unsigned char *derived_auth_key);

extern int tecc_sm4_rnd_decrypt_with_keys(const unsigned char *key, size_t key_len,
                        const unsigned char *ciphertext, size_t cipher_len,
                        unsigned char *output_buffer, size_t buffer_size,
                        size_t *output_len,
                        const unsigned char *derived_enc_key,
                        const unsigned char *derived_auth_key);

/* Data format validation */
extern int tecc_sm4_rnd_validate_format(const unsigned char *ciphertext, size_t cipher_len);

/* Utility functions */
extern size_t tecc_sm4_rnd_get_format_size(size_t plain_len);
extern uint32 tecc_sm4_rnd_get_version(void);
extern void tecc_sm4_rnd_clear_memory(void *data, size_t len);


#endif /* TECC_SM4_RND_H */
/* End - ReqID:TECC4.0 */