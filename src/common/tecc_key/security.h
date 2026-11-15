/*-------------------------------------------------------------------------
 *
 * security.h
 *    Security layer for TECC key management module
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/common/tecc_key/security.h
 *
 * ReqID:TECC4.0 - Key management security layer
 * Authored by <wanghaoyan@highgo.com>, 20260408.
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_KEY_SECURITY_H
#define TECC_KEY_SECURITY_H

#include "postgres.h"
#include "c.h"
#include "port/pg_bswap.h"

/* Error codes for security operations */
typedef enum SecurityErrorCode
{
    SECURITY_SUCCESS = 0,
    SECURITY_ERROR_INVALID_PARAM = 1000,
    SECURITY_ERROR_OUT_OF_MEMORY = 1001,
    SECURITY_ERROR_ENCRYPT_FAILED = 1100,
    SECURITY_ERROR_DECRYPT_FAILED = 1101,
    SECURITY_ERROR_HMAC_FAILED = 1102,
    SECURITY_ERROR_HMAC_VERIFY_FAILED = 1103,
    SECURITY_ERROR_HMAC_INIT_FAILED = 1104,
    SECURITY_ERROR_HMAC_UPDATE_FAILED = 1105,
    SECURITY_ERROR_HMAC_FINAL_FAILED = 1106,
    SECURITY_ERROR_HMAC_KEY_DERIVATION_FAILED = 1107,
    SECURITY_ERROR_KEY_DERIVATION_FAILED = 1108,
    SECURITY_ERROR_RNG_FAILURE = 1109,
    SECURITY_ERROR_POOR_RANDOMNESS = 1110,
    SECURITY_ERROR_WEAK_KEY = 1111,
    SECURITY_ERROR_UNSUPPORTED_ALGORITHM = 1112
} SecurityErrorCode;

/* Encryption algorithm identifiers */
typedef enum EncryptionAlgorithm
{
    ENCRYPT_ALGO_SM4_CBC = 0x00000001,  /* SM4-CBC algorithm */
    ENCRYPT_ALGO_AES_256_CBC = 0x00000002  /* AES-256-CBC algorithm */
} EncryptionAlgorithm;

/* HMAC algorithm identifiers */
typedef enum HmacAlgorithm
{
    HMAC_ALGO_SHA256 = 0x00000001,  /* HMAC-SHA256 */
    HMAC_ALGO_SHA384 = 0x00000002,  /* HMAC-SHA384 */
    HMAC_ALGO_SHA512 = 0x00000003   /* HMAC-SHA512 */
} HmacAlgorithm;

/* Random number generator interface */
typedef struct RandomNumberGenerator
{
    /* Initialize RNG */
    int (*init)(void);

    /* Generate random bytes */
    int (*generate_bytes)(unsigned char *buffer, size_t length);

    /* Generate random integer in range [min, max] */
    int (*generate_int)(int min, int max, int *result);

    /* Cleanup RNG */
    void (*cleanup)(void);
} RandomNumberGenerator;

/* PBKDF2 configuration */
typedef struct Pbkdf2Config
{
    const char *hash_algorithm;   /* Hash algorithm: SHA256/SHA384/SHA512 */
    int iterations;               /* Iteration count: 10000-100000 */
    size_t salt_length;           /* Salt length: 16-32 bytes */
    size_t derived_key_length;    /* Derived key length */
} Pbkdf2Config;

/* HMAC key manager for caching derived HMAC keys */
typedef struct HmacKeyManager
{
    unsigned char *hmac_key;      /* HMAC key */
    size_t hmac_key_len;          /* Key length */
    time_t generated_time;        /* Generation time */
    time_t expiration_time;       /* Expiration time */
    pthread_mutex_t lock;         /* Thread safety lock */
    bool is_valid;                /* Whether the key is valid */
} HmacKeyManager;

/* Function prototypes */

/* Random number generation */
extern RandomNumberGenerator *security_get_default_rng(void);
extern int security_generate_random_bytes(unsigned char *buffer, size_t length);
extern int security_generate_random_int(int min, int max, int *result);
extern bool security_check_randomness_quality(const unsigned char *data, size_t length, double threshold);

/* Key generation */
extern int security_generate_dek_key(int key_length, unsigned char **dek_data, size_t *dek_length);
extern int security_generate_dek_with_recovery(int key_length, unsigned char **dek_data,
                                              size_t *dek_length, int max_retries);
extern bool security_is_weak_key(const unsigned char *key, size_t key_len);

/* Encryption/decryption */
extern int security_encrypt_data(const unsigned char *plaintext, size_t plain_len,
                                const unsigned char *key, size_t key_len,
                                EncryptionAlgorithm algorithm,
                                unsigned char **ciphertext, size_t *cipher_len);
extern int security_decrypt_data(const unsigned char *ciphertext, size_t cipher_len,
                                const unsigned char *key, size_t key_len,
                                EncryptionAlgorithm algorithm,
                                unsigned char **plaintext, size_t *plain_len);

/* SM4-CBC specific operations */
extern int security_sm4_cbc_encrypt(const unsigned char *plaintext, size_t plain_len,
                                   const unsigned char *key, size_t key_len,
                                   unsigned char **ciphertext, size_t *cipher_len);
extern int security_sm4_cbc_decrypt(const unsigned char *ciphertext, size_t cipher_len,
                                   const unsigned char *key, size_t key_len,
                                   unsigned char **plaintext, size_t *plain_len);

/* DEK encryption/decryption with HMAC */
extern int security_encrypt_dek_with_hmac(const unsigned char *plain_dek, size_t plain_dek_len,
                                         const unsigned char *kek, size_t kek_len,
                                         unsigned char **encrypted_dek, size_t *encrypted_dek_len);
extern int security_decrypt_dek_with_hmac(const unsigned char *encrypted_dek, size_t encrypted_dek_len,
                                         const unsigned char *kek, size_t kek_len,
                                         unsigned char **plain_dek, size_t *plain_dek_len);

/* DEK decryption with automatic format detection (SM2 envelope vs SM4-CBC+HMAC) */
extern int security_decrypt_dek_auto(const unsigned char *encrypted_dek, size_t encrypted_dek_len,
                                     const unsigned char *kek, size_t kek_len,
                                     unsigned char **plain_dek, size_t *plain_dek_len);

/* HMAC operations */
extern int security_generate_hmac(const unsigned char *data, size_t data_len,
                                 const unsigned char *key, size_t key_len,
                                 HmacAlgorithm algorithm,
                                 unsigned char *hmac_output, size_t *hmac_len);
extern int security_verify_hmac(const unsigned char *data, size_t data_len,
                               const unsigned char *key, size_t key_len,
                               HmacAlgorithm algorithm,
                               const unsigned char *hmac_to_verify, size_t hmac_len);

/* HMAC key derivation */
extern int security_derive_hmac_key(const unsigned char *kek, size_t kek_len,
                                   unsigned char **hmac_key, size_t *hmac_key_len);

/* Key derivation */
extern int security_derive_key_from_kek(const unsigned char *kek, size_t kek_len,
                                       const unsigned char *salt, size_t salt_len,
                                       const char *context,
                                       unsigned char **derived_key, size_t *derived_len);

/* Memory security */
extern void security_secure_erase(void *data, size_t length);
extern void *security_secure_alloc(size_t size);
extern void security_secure_free(void *ptr, size_t size);

/* Constant time comparison */
extern int security_constant_time_compare(const void *a, const void *b, size_t len);

/* KEK file HMAC operations */
extern int security_generate_kek_file_hmac(const unsigned char *file_data,
                                         size_t data_len,
                                         const unsigned char *hmac_key,
                                         size_t hmac_key_len,
                                         unsigned char *hmac_output);
extern int security_verify_kek_file_hmac(const unsigned char *file_data,
                                       size_t data_len,
                                       const unsigned char *hmac_key,
                                       size_t hmac_key_len);

/* HMAC key manager operations */
extern HmacKeyManager *security_hmac_key_manager_create(void);
extern void security_hmac_key_manager_destroy(HmacKeyManager *manager);
extern int security_hmac_key_manager_get_key(HmacKeyManager *manager,
                                           const unsigned char *kek,
                                           size_t kek_len,
                                           unsigned char **hmac_key,
                                           size_t *hmac_key_len);
extern void security_hmac_key_manager_invalidate(HmacKeyManager *manager);
extern bool security_hmac_key_manager_is_valid(HmacKeyManager *manager);

/* Error handling */
extern const char *security_get_error_message(SecurityErrorCode error_code);

#endif /* TECC_KEY_SECURITY_H */