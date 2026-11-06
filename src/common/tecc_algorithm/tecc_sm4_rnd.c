/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_sm4_rnd.c
 *
 * Abstract:
 *   SM4-RND algorithm implementation - SM4-based authenticated encryption algorithm.
 *   Provides fixed-random encryption with HMAC-SM3 authentication.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/common/tecc_algorithm/tecc_sm4_rnd.c
 */

#include "../../include/common/tecc_algorithm/tecc_sm4_rnd.h"
/* #include "../../include/common/tecc_algorithm/tecc_algo_buffer.h" */
/* Include shared types */
#include "../../../src/common/tecc_types.h"
#include "../../include/common/tecc_algorithm/tecc_algorithm_api.h"
#include "../../include/common/tecc_algorithm/tecc_byteorder.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <openssl/rand.h>
#include "../../include/common/tecc_algorithm/tecc_sm4_internal.h"
#include "../../include/common/tecc_algorithm/tecc_sm3_internal.h"

/* Helper function declarations */
static int generate_random_bytes(unsigned char *buf, size_t len);
static int sm4_cbc_encrypt(const unsigned char *key, const unsigned char *iv,
                          const unsigned char *plaintext, size_t plain_len,
                          unsigned char *ciphertext, size_t *cipher_len);
static int sm4_cbc_decrypt(const unsigned char *key, const unsigned char *iv,
                          const unsigned char *ciphertext, size_t cipher_len,
                          unsigned char *plaintext, size_t *plain_len);
static int hmac_sm3(const unsigned char *key, size_t key_len,
                   const unsigned char *data, size_t data_len,
                   unsigned char *mac, size_t mac_len);

/*
 * Generate random bytes via OpenSSL RAND_priv_bytes.
 *
 * RAND_priv_bytes uses a per-thread private DRBG instance that does NOT
 * go through RAND_get_rand_method → CRYPTO_THREAD_write_lock.  This avoids
 * the self-deadlock observed with RAND_bytes under OpenSSL 3.0 where a
 * recursive rwlock acquisition spins forever at 100% CPU.
 */
/*
 * Generate cryptographically secure random bytes via getrandom(2).
 *
 * We avoid OpenSSL RAND bytes entirely: under OpenSSL 3.0, all RAND
 * entry points eventually call RAND_get_rand_method which acquires
 * CRYPTO_THREAD_write_lock.  Under conditions still not fully understood
 * a recursive rwlock occurs and glibc spins at 100% CPU.
 *
 * getrandom(2) is available since Linux 3.17 / glibc 2.25 and is the
 * recommended interface for small amounts of CSPRNG data.  Falls back
 * to /dev/urandom on ancient kernels.
 */
#include <unistd.h>
/*
 * Secure random generation.
 * On Linux: use getrandom(2) to avoid the OpenSSL 3.0 CRYPTO_THREAD_write_lock
 * self-deadlock observed with RAND_bytes.
 * On Windows/MSVC: use OpenSSL RAND_bytes (the deadlock is Linux-specific).
 */
#ifndef _WIN32
#if defined(HAVE_GETRANDOM) || defined(__GLIBC__) && __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 25
#include <sys/random.h>
#else
#include <sys/syscall.h>
#define getrandom(buf, len, flags)  syscall(SYS_getrandom, buf, len, flags)
#endif
#endif /* !_WIN32 */
#ifdef _WIN32
#include <openssl/rand.h>
#endif
static int
generate_random_bytes(unsigned char *buf, size_t len)
{
#ifdef _WIN32
    if (!buf || len == 0)
        return SM4_RND_ERROR_INVALID_PARAM;

    if (RAND_bytes(buf, (int)len) == 1)
        return SM4_RND_SUCCESS;

    return SM4_RND_ERROR_OPENSSL;
#else
    ssize_t n;

    if (!buf || len == 0)
        return SM4_RND_ERROR_INVALID_PARAM;

    n = getrandom(buf, len, 0);
    if (n == (ssize_t)len)
        return SM4_RND_SUCCESS;

    /* getrandom may return fewer bytes than requested (never for small
     * lengths like 16, but handle it anyway). */
    if (n > 0) {
        size_t remaining = len - (size_t)n;
        unsigned char *p = buf + n;
        while (remaining > 0) {
            ssize_t m = getrandom(p, remaining, 0);
            if (m <= 0)
                goto fallback;
            p += m;
            remaining -= (size_t)m;
        }
        return SM4_RND_SUCCESS;
    }

fallback:
    /* /dev/urandom fallback — EAGAIN on early boot or kernel < 3.17 */
    {
        FILE *fp = fopen("/dev/urandom", "rb");
        size_t nread;

        if (!fp)
            return SM4_RND_ERROR_OPENSSL;

        nread = fread(buf, 1, len, fp);
        fclose(fp);

        return (nread == len) ? SM4_RND_SUCCESS : SM4_RND_ERROR_OPENSSL;
    }
#endif /* _WIN32 */
}

/*
 * SM4-CBC encryption
 */
static int
sm4_cbc_encrypt(const unsigned char *key, const unsigned char *iv,
               const unsigned char *plaintext, size_t plain_len,
               unsigned char *ciphertext, size_t *cipher_len)
{
    TeccSM4Context *sm4_ctx = NULL;
    int result = SM4_RND_SUCCESS;

    if (!key || !iv || !plaintext || plain_len == 0 || !ciphertext || !cipher_len)
        return SM4_RND_ERROR_INVALID_PARAM;

    /* Validate: plaintext must be multiple of block size (PKCS#7 already applied) */
    if (plain_len % TECC_SM4_BLOCK_SIZE != 0)
        return SM4_RND_ERROR_INVALID_PARAM;

    sm4_ctx = tecc_sm4_context_new();
    if (!sm4_ctx)
        return SM4_RND_ERROR_OPENSSL;

    if (tecc_sm4_set_key_encrypt(sm4_ctx, key) != 1)
    {
        result = SM4_RND_ERROR_OPENSSL;
        goto cleanup;
    }

    if (tecc_sm4_cbc_encrypt(sm4_ctx, iv, plaintext, plain_len, ciphertext) != 1)
    {
        result = SM4_RND_ERROR_OPENSSL;
        goto cleanup;
    }

    *cipher_len = plain_len;

cleanup:
    if (sm4_ctx)
        tecc_sm4_context_free(sm4_ctx);

    return result;
}

/*
 * SM4-CBC decryption
 */
static int
sm4_cbc_decrypt(const unsigned char *key, const unsigned char *iv,
               const unsigned char *ciphertext, size_t cipher_len,
               unsigned char *plaintext, size_t *plain_len)
{
    TeccSM4Context *sm4_ctx = NULL;
    int result = SM4_RND_SUCCESS;

    if (!key || !iv || !ciphertext || cipher_len == 0 || !plaintext || !plain_len)
        return SM4_RND_ERROR_INVALID_PARAM;

    /* Check ciphertext length (must be multiple of block size) */
    if (cipher_len % TECC_SM4_BLOCK_SIZE != 0)
        return SM4_RND_ERROR_INVALID_FORMAT;

    sm4_ctx = tecc_sm4_context_new();
    if (!sm4_ctx)
        return SM4_RND_ERROR_OPENSSL;

    if (tecc_sm4_set_key_decrypt(sm4_ctx, key) != 1)
    {
        result = SM4_RND_ERROR_OPENSSL;
        goto cleanup;
    }

    if (tecc_sm4_cbc_decrypt(sm4_ctx, iv, ciphertext, cipher_len, plaintext) != 1)
    {
        result = SM4_RND_ERROR_DECRYPT_FAILED;
        goto cleanup;
    }

    *plain_len = cipher_len;

cleanup:
    if (sm4_ctx)
        tecc_sm4_context_free(sm4_ctx);

    return result;
}

/*
 * PKCS#7 padding in-place (zero-copy version)
 * Assumes buffer has enough space for padding
 */
static int
pkcs7_pad_inplace(unsigned char *buffer, size_t plain_len, size_t padded_len)
{
    size_t pad_len;

    if (!buffer)
        return SM4_RND_ERROR_INVALID_PARAM;

    pad_len = padded_len - plain_len;

    /* According to PKCS#7 standard:
     * - If data length is multiple of block size, add a full block of padding
     * - Each padding byte contains the padding length value
     */
    if (pad_len == 0) {
        /* Data length is multiple of block size, add full block */
        pad_len = TECC_SM4_BLOCK_SIZE;
    }

    if (pad_len > TECC_SM4_BLOCK_SIZE)
        return SM4_RND_ERROR_INVALID_PARAM;

    /* Add padding bytes after plaintext */
    memset(buffer + plain_len, (unsigned char)pad_len, pad_len);

    return SM4_RND_SUCCESS;
}

/*
 * HMAC-SM3 calculation
 */
static int
hmac_sm3(const unsigned char *key, size_t key_len,
        const unsigned char *data, size_t data_len,
        unsigned char *mac, size_t mac_len)
{
    if (!key || key_len == 0 || !data || data_len == 0 || !mac || mac_len < 32)
        return SM4_RND_ERROR_INVALID_PARAM;

    if (tecc_sm3_hmac(key, key_len, data, data_len, mac) != 1)
        return SM4_RND_ERROR_OPENSSL;

    return SM4_RND_SUCCESS;
}

/*
 * Key derivation using HKDF-SM3
 */
int
tecc_sm4_rnd_derive_keys(const unsigned char *master_key, size_t key_len,
                        unsigned char *enc_key, unsigned char *auth_key)
{
    unsigned char salt[32] = {0};
    unsigned char info[32] = {0};
    int result;

    if (!master_key || key_len == 0 || !enc_key || !auth_key)
        return SM4_RND_ERROR_INVALID_PARAM;

    /* Use fixed salt (in real application should use random salt) */
    memset(salt, 0, 32);
    memcpy(salt, "TECC-SM4-RND-SALT-20260406", 27);  /* 27 characters including null */

    /* Derive encryption key using HKDF-SM3 */
    memset(info, 0, 32);
    memcpy(info, "TECC-SM4-ENC-KEY", 16);  /* 16 characters including null */
    result = hkdf_sm3_derive(salt, 32, master_key, key_len, info, 32, enc_key, 16);
    if (result != 0)
        return SM4_RND_ERROR_KEY_DERIVATION;

    /* Derive authentication key */
    memset(info, 0, 32);
    memcpy(info, "TECC-SM4-AUTH-KEY", 17);  /* 17 characters including null */
    result = hkdf_sm3_derive(salt, 32, master_key, key_len, info, 32, auth_key, 32);
    if (result != 0)
    {
        tecc_sm4_rnd_clear_memory(enc_key, 16);
        return SM4_RND_ERROR_KEY_DERIVATION;
    }

    return SM4_RND_SUCCESS;
}

/*
 * Validate SM4-RND data format
 */
int
tecc_sm4_rnd_validate_format(const unsigned char *ciphertext, size_t cipher_len)
{

    if (!ciphertext || cipher_len == 0)
        return SM4_RND_ERROR_INVALID_PARAM;

    /* Check data length */
    /* Minimum size: auth_tag(32) + iv(16) + ciphertext_len(8) + 1 byte ciphertext */
    {
        size_t min_size = 32 + 16 + sizeof(size_t) + 1;
        if (cipher_len < min_size)
            return SM4_RND_ERROR_INVALID_FORMAT;
    }

    return SM4_RND_SUCCESS;
}

/*
 * Get SM4-RND output format size
 */
size_t
tecc_sm4_rnd_get_format_size(size_t plain_len)
{
    /* Calculate padded ciphertext size (PKCS#7 padding to 16-byte multiple) */
    /* According to PKCS#7 standard, if data length is multiple of block size,
     * we need to add a full block of padding */
    size_t padded_len;

    if (plain_len % TECC_SM4_BLOCK_SIZE == 0) {
        /* Data length is multiple of block size, add full block */
        padded_len = plain_len + TECC_SM4_BLOCK_SIZE;
    } else {
        /* Normal padding calculation */
        padded_len = ((plain_len + TECC_SM4_BLOCK_SIZE - 1) / TECC_SM4_BLOCK_SIZE) *
                    TECC_SM4_BLOCK_SIZE;
    }

    /* Total size = header size + padded ciphertext size */
    return SM4_RND_HEADER_SIZE + padded_len;
}

/*
 * Calculate required buffer size for encryption
 */
size_t
tecc_sm4_rnd_calc_encrypt_buffer_size(size_t plain_len)
{
    /* Calculate padded ciphertext size (PKCS#7 padding to 16-byte multiple) */
    /* According to PKCS#7 standard, if data length is multiple of block size,
     * we need to add a full block of padding */
    size_t padded_len;
    size_t header_offset;

    if (plain_len % TECC_SM4_BLOCK_SIZE == 0) {
        /* Data length is multiple of block size, add full block */
        padded_len = plain_len + TECC_SM4_BLOCK_SIZE;
    } else {
        /* Normal padding calculation */
        padded_len = ((plain_len + TECC_SM4_BLOCK_SIZE - 1) / TECC_SM4_BLOCK_SIZE) *
                    TECC_SM4_BLOCK_SIZE;
    }

    /* Total size = offset to ciphertext field + padded ciphertext size */
    /* Sm4RndDataFormat structure: auth_tag(32) + iv(16) + ciphertext_len(sizeof(size_t)) + ciphertext */
    header_offset = 32 + 16 + sizeof(size_t);
    return header_offset + padded_len;
}

/*
 * Calculate required buffer size for decryption
 */
size_t
tecc_sm4_rnd_calc_decrypt_buffer_size(size_t cipher_len)
{
    /* For decryption, output buffer needs to hold plaintext */
    /* Header size: version(4) + auth_tag(32) + iv(16) + ciphertext_len(8) */
    size_t header_offset = offsetof(Sm4RndDataFormat, ciphertext[0]);

    if (cipher_len < header_offset)
        return 0;

    return cipher_len - header_offset;
}

/*
 * Clear sensitive memory
 */
void
tecc_sm4_rnd_clear_memory(void *data, size_t len)
{
    tecc_algo_secure_cleanup(data, len);
}

/*
 * SM4-RND encryption interface - zero-copy version
 * Caller provides pre-allocated output buffer
 */
int
/* ReqID:TECC4.0 — encrypt with optional pre-derived keys to skip HKDF */
tecc_sm4_rnd_encrypt_with_keys(const unsigned char *key, size_t key_len,
                    const unsigned char *plaintext, size_t plain_len,
                    unsigned char *output_buffer, size_t buffer_size,
                    size_t *output_len,
                    const unsigned char *derived_enc_key,
                    const unsigned char *derived_auth_key)
{
    int result;
    unsigned char enc_key[16];
    unsigned char auth_key[32];
    bool have_derived_keys;
    unsigned char iv[16];
    size_t padded_len;
    size_t required_size;
    size_t header_offset;
    Sm4RndDataFormat *format;
    unsigned char *hmac_buffer;
    unsigned char *ptr;
    size_t hmac_data_len;
    int j;

    SM4_RND_DEBUG_PRINT("DEBUG[1]: tecc_sm4_rnd_encrypt called: plain_len=%zu, buffer_size=%zu\n",
           plain_len, buffer_size);
    SM4_RND_DEBUG_PRINT("DEBUG[1]: key_len=%zu, output_buffer=%p, output_len=%p\n",
           key_len, output_buffer, output_len);

    /* Parameter validation */
    SM4_RND_DEBUG_PRINT("DEBUG[2]: Starting parameter validation\n");
    if (!key || key_len == 0 || !plaintext ||
        !output_buffer || !output_len) {
        SM4_RND_DEBUG_PRINT("DEBUG[2]: Parameter validation failed\n");
        return SM4_RND_ERROR_INVALID_PARAM;
    }

    SM4_RND_DEBUG_PRINT("DEBUG[2]: TECC_MAX_DATA_SIZE=%zu\n", (size_t)TECC_MAX_DATA_SIZE);
    if (plain_len > TECC_MAX_DATA_SIZE) {
        SM4_RND_DEBUG_PRINT("DEBUG[2]: plain_len too large\n");
        return SM4_RND_ERROR_INVALID_LENGTH;
    }

    /* Calculate required buffer size */
    SM4_RND_DEBUG_PRINT("DEBUG[3]: Calculating buffer sizes\n");
    SM4_RND_DEBUG_PRINT("DEBUG[3]: TECC_SM4_BLOCK_SIZE=%zu\n", (size_t)TECC_SM4_BLOCK_SIZE);
    /* According to PKCS#7 standard, if data length is multiple of block size,
     * we need to add a full block of padding */
    if (plain_len % TECC_SM4_BLOCK_SIZE == 0) {
        /* Data length is multiple of block size, add full block */
        padded_len = plain_len + TECC_SM4_BLOCK_SIZE;
    } else {
        /* Normal padding calculation */
        padded_len = ((plain_len + TECC_SM4_BLOCK_SIZE - 1) / TECC_SM4_BLOCK_SIZE) *
                    TECC_SM4_BLOCK_SIZE;
    }
    /* Sm4RndDataFormat structure: auth_tag(32) + iv(16) + ciphertext_len(sizeof(size_t)) + ciphertext */
    header_offset = 32 + 16 + sizeof(size_t);
    required_size = header_offset + padded_len;
    SM4_RND_DEBUG_PRINT("DEBUG[3]: padded_len=%zu, required_size=%zu, header_offset=%zu\n",
           padded_len, required_size, header_offset);

    if (buffer_size < required_size) {
        SM4_RND_DEBUG_PRINT("DEBUG[3]: Buffer overflow: buffer_size=%zu < required_size=%zu\n",
               buffer_size, required_size);
        return SM4_RND_ERROR_BUFFER_OVERFLOW;
    }

    /* Clear output buffer */
    SM4_RND_DEBUG_PRINT("DEBUG[4]: Clearing output buffer\n");
    memset(output_buffer, 0, buffer_size);

    /* Derive encryption and authentication keys */
    SM4_RND_DEBUG_PRINT("DEBUG[4]: Deriving keys\n");
    have_derived_keys = (derived_enc_key != NULL && derived_auth_key != NULL);
    if (have_derived_keys)
    {
        memcpy(enc_key, derived_enc_key, 16);
        memcpy(auth_key, derived_auth_key, 32);
        result = SM4_RND_SUCCESS;
    }
    else
    {
        result = tecc_sm4_rnd_derive_keys(key, key_len, enc_key, auth_key);
    }
    SM4_RND_DEBUG_PRINT("DEBUG[4]: Key derivation result: %d\n", result);
    if (result != SM4_RND_SUCCESS)
        goto cleanup;

    /* Debug: print derived keys */
    SM4_RND_DEBUG_PRINT("DEBUG[4]: Derived enc_key (first 8 bytes): ");
    {
        int i;
        for (i = 0; i < 8 && i < sizeof(enc_key); i++) {
            SM4_RND_DEBUG_PRINT("%02X", enc_key[i]);
        }
    }
    SM4_RND_DEBUG_PRINT("\n");
    SM4_RND_DEBUG_PRINT("DEBUG[4]: Derived auth_key (first 8 bytes): ");
    {
        int i;
        for (i = 0; i < 8 && i < sizeof(auth_key); i++) {
            SM4_RND_DEBUG_PRINT("%02X", auth_key[i]);
        }
    }
    SM4_RND_DEBUG_PRINT("\n");

    /* Generate random IV */
    result = generate_random_bytes(iv, sizeof(iv));
    if (result != SM4_RND_SUCCESS)
        goto cleanup_keys;

    /* Prepare output buffer */
    format = (Sm4RndDataFormat *)output_buffer;
    memcpy(format->iv, iv, sizeof(iv));
    format->ciphertext_len = padded_len;

    /* Copy plaintext to ciphertext area (will be encrypted in-place) */
    memcpy(format->ciphertext, plaintext, plain_len);

    /* Add PKCS#7 padding */
    result = pkcs7_pad_inplace(format->ciphertext, plain_len, padded_len);
    if (result != SM4_RND_SUCCESS)
        goto cleanup_keys;

    /* Perform SM4-CBC encryption in-place */
    result = sm4_cbc_encrypt(enc_key, iv, format->ciphertext, padded_len,
                            format->ciphertext, &padded_len);
    if (result != SM4_RND_SUCCESS)
        goto cleanup_keys;

    /* Update ciphertext length */
    format->ciphertext_len = padded_len;

    /* Calculate authentication tag */
    /* HMAC covers: version + auth_tag(zeros) + iv + ciphertext_len + ciphertext */
    SM4_RND_DEBUG_PRINT("DEBUG: HMAC computed\n");
    SM4_RND_DEBUG_PRINT("DEBUG: ciphertext[0-7]: ");
    {
        int k;
        for (k = 0; k < 8 && k < padded_len; k++) {
            SM4_RND_DEBUG_PRINT("%02X", format->ciphertext[k]);
        }
    }
    SM4_RND_DEBUG_PRINT("\n");

    /* Clear auth_tag area before HMAC calculation (it should be zeros) */
    memset(format->auth_tag, 0, sizeof(format->auth_tag));

    /* Create temporary buffer for HMAC calculation */
    hmac_data_len = sizeof(format->auth_tag) + sizeof(format->iv) + sizeof(format->ciphertext_len) + padded_len;
    hmac_buffer = malloc(hmac_data_len);
    if (!hmac_buffer) {
        result = SM4_RND_ERROR_MEMORY_ALLOC;
        goto cleanup_keys;
    }

    /* Copy data to temporary buffer */
    ptr = hmac_buffer;
    memset(ptr, 0, sizeof(format->auth_tag));  /* auth_tag should be zeros */
    ptr += sizeof(format->auth_tag);
    memcpy(ptr, format->iv, sizeof(format->iv));
    ptr += sizeof(format->iv);
    memcpy(ptr, &format->ciphertext_len, sizeof(format->ciphertext_len));
    ptr += sizeof(format->ciphertext_len);
    memcpy(ptr, format->ciphertext, padded_len);

    result = hmac_sm3(auth_key, 32, hmac_buffer, hmac_data_len,
                     format->auth_tag, sizeof(format->auth_tag));

    /* Free temporary buffer */
    free(hmac_buffer);
    if (result != SM4_RND_SUCCESS)
        goto cleanup_keys;

    /* Debug: print calculated HMAC */
    SM4_RND_DEBUG_PRINT("DEBUG[encrypt]: Calculated HMAC (first 8 bytes): ");
    for (j = 0; j < 8 && j < sizeof(format->auth_tag); j++) {
    }
    SM4_RND_DEBUG_PRINT("\n");

    /* Set output length */
    /* Sm4RndDataFormat structure: version(4) + auth_tag(32) + iv(16) + ciphertext_len(sizeof(size_t)) + ciphertext */
    *output_len = header_offset + padded_len;

    result = SM4_RND_SUCCESS;

cleanup_keys:
    /* Clear sensitive data */
    tecc_algo_secure_cleanup(enc_key, sizeof(enc_key));
    tecc_algo_secure_cleanup(auth_key, sizeof(auth_key));

cleanup:
    return result;
}

/* ReqID:TECC4.0 — backward-compatible wrapper */
int
tecc_sm4_rnd_encrypt(const unsigned char *key, size_t key_len,
                    const unsigned char *plaintext, size_t plain_len,
                    unsigned char *output_buffer, size_t buffer_size,
                    size_t *output_len)
{
    return tecc_sm4_rnd_encrypt_with_keys(key, key_len,
                    plaintext, plain_len,
                    output_buffer, buffer_size, output_len,
                    NULL, NULL);
}

/*
 * PKCS#7 unpadding in-place (zero-copy version)
 * Returns pointer to plaintext and plaintext length
 */
static int
pkcs7_unpad_inplace(const unsigned char *padded_text, size_t padded_len,
                   const unsigned char **plaintext, size_t *plain_len)
{
    size_t pad_len;
    size_t i;

    if (!padded_text || !plaintext || !plain_len)
        return SM4_RND_ERROR_INVALID_PARAM;

    if (padded_len == 0 || padded_len % TECC_SM4_BLOCK_SIZE != 0)
        return SM4_RND_ERROR_INVALID_PARAM;

    /* Get padding length */
    pad_len = padded_text[padded_len - 1];

    /* Validate padding length */
    if (pad_len == 0 || pad_len > TECC_SM4_BLOCK_SIZE || pad_len > padded_len)
        return SM4_RND_ERROR_DECRYPT_FAILED;

    /* Validate padding bytes */
    for (i = padded_len - pad_len; i < padded_len; i++)
    {
        if (padded_text[i] != pad_len)
            return SM4_RND_ERROR_DECRYPT_FAILED;
    }

    /* Calculate original data length */
    *plain_len = padded_len - pad_len;
    *plaintext = padded_text;  /* Plaintext starts at beginning of buffer */

    return SM4_RND_SUCCESS;
}

/*
 * SM4-RND decryption interface - zero-copy version
 * Caller provides pre-allocated output buffer
 */
int
/* ReqID:TECC4.0 — decrypt with optional pre-derived keys to skip HKDF */
tecc_sm4_rnd_decrypt_with_keys(const unsigned char *key, size_t key_len,
                    const unsigned char *ciphertext, size_t cipher_len,
                    unsigned char *output_buffer, size_t buffer_size,
                    size_t *output_len,
                    const unsigned char *derived_enc_key,
                    const unsigned char *derived_auth_key)
{
    int result;
    unsigned char enc_key[16];
    unsigned char auth_key[32];
    bool have_derived_keys;
    const Sm4RndDataFormat *format;
    unsigned char calc_auth_tag[32];
    size_t data_len;
    const unsigned char *plaintext_ptr;
    size_t plain_len;
    int i;
    int k;
    unsigned char *hmac_buffer;
    unsigned char *ptr;
    size_t hmac_data_len;
    size_t header_offset;

    SM4_RND_DEBUG_PRINT("DEBUG[decrypt-start]: tecc_sm4_rnd_decrypt called\n");
    SM4_RND_DEBUG_PRINT("DEBUG[decrypt-start]: cipher_len=%zu, buffer_size=%zu\n", cipher_len, buffer_size);

    /* Parameter validation */
    if (!key || key_len == 0 || !ciphertext || cipher_len == 0 ||
        !output_buffer || !output_len)
        return SM4_RND_ERROR_INVALID_PARAM;

    if (cipher_len < SM4_RND_MIN_SIZE || cipher_len > SM4_RND_MAX_SIZE)
        return SM4_RND_ERROR_INVALID_LENGTH;

    /* Initialize output parameters */
    *output_len = 0;

    /* Validate data format */
    result = tecc_sm4_rnd_validate_format(ciphertext, cipher_len);
    if (result != SM4_RND_SUCCESS)
        return result;

    format = (const Sm4RndDataFormat *)ciphertext;

    /* Derive encryption and authentication keys */
    have_derived_keys = (derived_enc_key != NULL && derived_auth_key != NULL);
    if (have_derived_keys)
    {
        memcpy(enc_key, derived_enc_key, 16);
        memcpy(auth_key, derived_auth_key, 32);
        result = SM4_RND_SUCCESS;
    }
    else
    {
        result = tecc_sm4_rnd_derive_keys(key, key_len, enc_key, auth_key);
    }
    if (result != SM4_RND_SUCCESS)
        goto cleanup;

    /* Debug: print derived keys */
    SM4_RND_DEBUG_PRINT("DEBUG[decrypt]: Derived enc_key (first 8 bytes): ");
    for (i = 0; i < 8 && i < sizeof(enc_key); i++) {
        SM4_RND_DEBUG_PRINT("%02X", enc_key[i]);
    }
    SM4_RND_DEBUG_PRINT("\n");
    SM4_RND_DEBUG_PRINT("DEBUG[decrypt]: Derived auth_key (first 8 bytes): ");
    for (i = 0; i < 8 && i < sizeof(auth_key); i++) {
        SM4_RND_DEBUG_PRINT("%02X", auth_key[i]);
    }
    SM4_RND_DEBUG_PRINT("\n");

    /* Verify authentication tag */
    /* Calculate header offset: auth_tag(32) + iv(16) + ciphertext_len(sizeof(size_t)) */
    header_offset = 32 + 16 + sizeof(size_t);
    data_len = cipher_len - header_offset;

    /* HMAC should cover: version + auth_tag(zeros) + iv + ciphertext_len + ciphertext */
    /* We need to create a temporary buffer with auth_tag area zeroed */

    /* Debug: print HMAC input information */
    SM4_RND_DEBUG_PRINT("DEBUG[decrypt]: HMAC calculation:\n");
    SM4_RND_DEBUG_PRINT("  data_len: %zu\n", data_len);
    SM4_RND_DEBUG_PRINT("  ciphertext[0-7]: ");
    {
        for (k = 0; k < 8 && k < data_len; k++) {
        }
    }
    SM4_RND_DEBUG_PRINT("\n");

    /* Create temporary buffer for HMAC calculation */
    hmac_data_len = sizeof(format->auth_tag) + sizeof(format->iv) + sizeof(format->ciphertext_len) + data_len;
    SM4_RND_DEBUG_PRINT("DEBUG[decrypt]: hmac_data_len=%zu\n", hmac_data_len);
    hmac_buffer = malloc(hmac_data_len);
    if (!hmac_buffer) {
        result = SM4_RND_ERROR_MEMORY_ALLOC;
        goto cleanup_keys;
    }

    /* Copy data to temporary buffer */
    ptr = hmac_buffer;

    /* Will be zeroed next - copy and zero it */
    memcpy(ptr, format->auth_tag, sizeof(format->auth_tag));

    /* Set auth_tag area to zeros */
    memset(ptr, 0, sizeof(format->auth_tag));
    ptr += sizeof(format->auth_tag);

    /* Copy iv */
    memcpy(ptr, format->iv, sizeof(format->iv));
    ptr += sizeof(format->iv);

    /* Copy ciphertext_len */
    memcpy(ptr, &format->ciphertext_len, sizeof(format->ciphertext_len));
    ptr += sizeof(format->ciphertext_len);

    /* Copy ciphertext */
    memcpy(ptr, format->ciphertext, data_len);

    /* Calculate HMAC */
    result = hmac_sm3(auth_key, 32, hmac_buffer, hmac_data_len,
                     calc_auth_tag, sizeof(calc_auth_tag));

    /* Debug: print HMAC results */
    if (result == SM4_RND_SUCCESS) {
        SM4_RND_DEBUG_PRINT("DEBUG[decrypt]: Calculated HMAC (first 8 bytes): ");
        for (i = 0; i < 8 && i < sizeof(calc_auth_tag); i++) {
            SM4_RND_DEBUG_PRINT("%02X", calc_auth_tag[i]);
        }
        SM4_RND_DEBUG_PRINT("\n");
        SM4_RND_DEBUG_PRINT("DEBUG[decrypt]: Actual HMAC from ciphertext (first 8 bytes): ");
        for (i = 0; i < 8 && i < sizeof(format->auth_tag); i++) {
            SM4_RND_DEBUG_PRINT("%02X", format->auth_tag[i]);
        }
        SM4_RND_DEBUG_PRINT("\n");
    }

    /* Clean up temporary buffer */
    free(hmac_buffer);
    if (result != SM4_RND_SUCCESS)
        goto cleanup_keys;

    if (tecc_algo_constant_time_memcmp(format->auth_tag, calc_auth_tag,
                                      sizeof(format->auth_tag)) != 0)
    {
        result = SM4_RND_ERROR_AUTH_FAILED;
        goto cleanup_keys;
    }

    /* Decrypt ciphertext to output buffer */
    result = sm4_cbc_decrypt(enc_key, format->iv,
                            format->ciphertext, data_len,
                            output_buffer, &data_len);
    if (result != SM4_RND_SUCCESS)
        goto cleanup_keys;

    /* Remove PKCS#7 padding in-place */
    result = pkcs7_unpad_inplace(output_buffer, data_len,
                                &plaintext_ptr, &plain_len);
    if (result != SM4_RND_SUCCESS)
        goto cleanup_keys;

    /* Check if output buffer is large enough */
    if (buffer_size < plain_len)
    {
        result = SM4_RND_ERROR_BUFFER_OVERFLOW;
        goto cleanup_keys;
    }

    /* If plaintext is not at the beginning of buffer, move it */
    if (plaintext_ptr != output_buffer)
    {
        memmove(output_buffer, plaintext_ptr, plain_len);
    }

    /* Set output length */
    *output_len = plain_len;

    result = SM4_RND_SUCCESS;

cleanup_keys:
    /* Clear sensitive data */
    tecc_algo_secure_cleanup(enc_key, sizeof(enc_key));
    tecc_algo_secure_cleanup(auth_key, sizeof(auth_key));

cleanup:
    return result;
}

/* ReqID:TECC4.0 — backward-compatible wrapper */
int
tecc_sm4_rnd_decrypt(const unsigned char *key, size_t key_len,
                    const unsigned char *ciphertext, size_t cipher_len,
                    unsigned char *output_buffer, size_t buffer_size,
                    size_t *output_len)
{
    return tecc_sm4_rnd_decrypt_with_keys(key, key_len,
                    ciphertext, cipher_len,
                    output_buffer, buffer_size, output_len,
                    NULL, NULL);
}

/* Begin - ReqID:TECC4.0 */
/*
 * SM4-RND batch decryption with pre-derived keys.
 * Shares one SM4Context across all items — eliminates per-row
 * SM4 key expansion (32 rounds) and SM4 context malloc/free.
 *
 * On failure, any partially-allocated plaintext buffers are freed
 * and the output array is cleared to NULL.
 */
int
tecc_sm4_rnd_batch_decrypt_with_keys(
    const unsigned char *enc_key,
    const unsigned char *auth_key,
    const unsigned char **ciphertexts,
    const size_t *ciphertext_lens,
    size_t count,
    unsigned char **plaintexts,
    size_t *plaintext_lens)
{
    TeccSM4Context *sm4_ctx;
    int result;
    size_t i;

    if (!enc_key || !auth_key || !ciphertexts || !ciphertext_lens ||
        count == 0 || !plaintexts || !plaintext_lens)
        return SM4_RND_ERROR_INVALID_PARAM;

    for (i = 0; i < count; i++)
    {
        plaintexts[i] = NULL;
        plaintext_lens[i] = 0;
    }

    /* Create SM4 context and set decrypt key once for all items */
    sm4_ctx = tecc_sm4_context_new();
    if (!sm4_ctx)
        return SM4_RND_ERROR_MEMORY_ALLOC;

    if (tecc_sm4_set_key_decrypt(sm4_ctx, enc_key) != 1)
    {
        tecc_sm4_context_free(sm4_ctx);
        return SM4_RND_ERROR_OPENSSL;
    }

    result = SM4_RND_SUCCESS;

    /* Create HMAC snapshot — share init state across all items */
    {
        TeccSM3HMACSnapshot *hmac_snap;

        hmac_snap = tecc_sm3_hmac_snapshot_create(auth_key, 32);
        if (!hmac_snap)
        {
            tecc_sm4_context_free(sm4_ctx);
            return SM4_RND_ERROR_MEMORY_ALLOC;
        }

    for (i = 0; i < count; i++)
    {
        const unsigned char *ciphertext;
        size_t cipher_len;
        const Sm4RndDataFormat *format;
        unsigned char calc_auth_tag[32];
        size_t data_len;
        size_t header_offset;
        unsigned char *hmac_buffer;
        size_t hmac_data_len;
        unsigned char *output_buffer;
        const unsigned char *plaintext_ptr;
        size_t plain_len;
        unsigned char *ptr;

        ciphertext = ciphertexts[i];
        cipher_len = ciphertext_lens[i];

        /* Validate format */
        if (tecc_sm4_rnd_validate_format(ciphertext, cipher_len) != SM4_RND_SUCCESS)
        {
            result = SM4_RND_ERROR_INVALID_FORMAT;
            break;
        }

        format = (const Sm4RndDataFormat *)ciphertext;

        /* HMAC verification: build buffer with zeroed auth_tag */
        header_offset = 32 + 16 + sizeof(size_t);
        data_len = cipher_len - header_offset;

        hmac_data_len = sizeof(format->auth_tag) +
                        sizeof(format->iv) + sizeof(format->ciphertext_len) +
                        data_len;
        hmac_buffer = malloc(hmac_data_len);
        if (!hmac_buffer)
        {
            result = SM4_RND_ERROR_MEMORY_ALLOC;
            break;
        }

        /* Build HMAC input: zeroed_auth + iv + ct_len + ct */
        ptr = hmac_buffer;
        memset(ptr, 0, sizeof(format->auth_tag));
        ptr += sizeof(format->auth_tag);
        memcpy(ptr, format->iv, sizeof(format->iv));
        ptr += sizeof(format->iv);
        memcpy(ptr, &format->ciphertext_len, sizeof(format->ciphertext_len));
        ptr += sizeof(format->ciphertext_len);
        memcpy(ptr, format->ciphertext, data_len);

        result = tecc_sm3_hmac_snapshot_compute(hmac_snap, hmac_buffer,
                                                 hmac_data_len, calc_auth_tag)
                 ? SM4_RND_SUCCESS : SM4_RND_ERROR_OPENSSL;
        free(hmac_buffer);

        if (result != SM4_RND_SUCCESS)
            break;

        if (tecc_algo_constant_time_memcmp(format->auth_tag, calc_auth_tag,
                                           sizeof(format->auth_tag)) != 0)
        {
            result = SM4_RND_ERROR_AUTH_FAILED;
            break;
        }

        /* Allocate output buffer */
        output_buffer = malloc(data_len);
        if (!output_buffer)
        {
            result = SM4_RND_ERROR_MEMORY_ALLOC;
            break;
        }

        /* SM4-CBC decrypt — reuses shared sm4_ctx (key already set) */
        if (tecc_sm4_cbc_decrypt(sm4_ctx, format->iv,
                                 format->ciphertext, data_len,
                                 output_buffer) != 1)
        {
            free(output_buffer);
            result = SM4_RND_ERROR_DECRYPT_FAILED;
            break;
        }

        /* PKCS#7 unpad */
        result = pkcs7_unpad_inplace(output_buffer, data_len,
                                     &plaintext_ptr, &plain_len);
        if (result != SM4_RND_SUCCESS)
        {
            free(output_buffer);
            break;
        }

        /* Move plaintext to start of buffer if needed */
        if (plaintext_ptr != output_buffer)
            memmove(output_buffer, plaintext_ptr, plain_len);

        plaintexts[i] = output_buffer;
        plaintext_lens[i] = plain_len;
    }

    tecc_sm3_hmac_snapshot_free(hmac_snap);
    }

    tecc_sm4_context_free(sm4_ctx);

    /* On failure, free all partially-allocated outputs */
    if (result != SM4_RND_SUCCESS)
    {
        size_t j;

        for (j = 0; j < i; j++)
        {
            if (plaintexts[j])
            {
                free(plaintexts[j]);
                plaintexts[j] = NULL;
                plaintext_lens[j] = 0;
            }
        }
    }

    return result;
}
/* End - ReqID:TECC4.0 */

