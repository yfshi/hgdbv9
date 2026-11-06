/*-------------------------------------------------------------------------
 *
 * security.c
 *    Security layer implementation for TECC key management module
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/common/tecc_key/security.c
 *
 * ReqID:TECC4.0 - Key management security layer
 * Authored by <wanghaoyan@highgo.com>, 20260408.
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *-------------------------------------------------------------------------
 */
#include "postgres_fe.h"
#include "c.h"
#include "common/fe_memutils.h"
#include "port/pg_bswap.h"
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
/*
 * Secure random generation.
 * On Linux: use getrandom(2) to avoid the OpenSSL 3.0 CRYPTO_THREAD_write_lock
 * self-deadlock observed with RAND_bytes. On Windows: use OpenSSL RAND_bytes
 * (the deadlock is Linux-specific).
 */
#ifndef _WIN32
#if defined(HAVE_GETRANDOM) || defined(__GLIBC__) && __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 25
#include <sys/random.h>
#else
#include <sys/syscall.h>
#define getrandom(buf, len, flags)  syscall(SYS_getrandom, buf, len, flags)
#endif
#endif /* !_WIN32 */
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include "common/tecc_algorithm/tecc_sm4_internal.h"
#include "security.h"
#include "utils.h"

/* Constants */
#define SM4_KEY_LENGTH 16           /* SM4 key length: 128 bits */
#define SM4_BLOCK_SIZE 16           /* SM4 block size: 128 bits */
#define HMAC_SHA256_LENGTH 32       /* HMAC-SHA256 output length */
#define IV_LENGTH 16                /* IV length for CBC mode */
#define DEK_MIN_LENGTH 16           /* Minimum DEK length: 128 bits */
#define DEK_MAX_LENGTH 32           /* Maximum DEK length: 256 bits */
#define HMAC_KEY_DERIVATION_SALT_LENGTH 32
/* Explicit 32-byte salt — must NOT rely on adjacent memory like the old \0 + 2-bytes overread */
static const unsigned char HMAC_KEY_DERIVATION_SALT[HMAC_KEY_DERIVATION_SALT_LENGTH] = {
    'T','E','C','C','_','H','M','A','C','_','K','E','Y','_',
    'D','E','R','I','V','A','T','I','O','N','_','S','A','L','T',
    0x00, 0x46, 0x61
};
#define PBKDF2_ITERATIONS 10000     /* PBKDF2 iteration count */

/* HMAC key manager constants */
#define HMAC_KEY_CACHE_TTL 3600     /* HMAC key cache TTL: 1 hour */
#define HMAC_KEY_REFRESH_THRESHOLD 300  /* Refresh threshold: 5 minutes before expiry */

/* Thread-local error buffer */
static pthread_key_t security_error_key;
static pthread_once_t security_error_key_once = PTHREAD_ONCE_INIT;

/*
 * create_security_error_key
 *    Create thread-specific error key for security module
 */
static void
create_security_error_key(void)
{
    pthread_key_create(&security_error_key, free);
}

/*
 * get_security_error_buffer
 *    Get thread-specific error buffer for security module
 */
static char *
get_security_error_buffer(void)
{
    char *buffer;

    pthread_once(&security_error_key_once, create_security_error_key);

    buffer = pthread_getspecific(security_error_key);
    if (buffer == NULL)
    {
        buffer = malloc(512);
        if (buffer != NULL)
        {
            memset(buffer, 0, 512);
            pthread_setspecific(security_error_key, buffer);
        }
    }

    return buffer;
}

/*
 * set_security_error
 *    Set security error message
 */
static void
set_security_error(const char *format, ...)
{
    va_list args;
    char *buffer;

    buffer = get_security_error_buffer();
    if (buffer == NULL)
        return;

    va_start(args, format);
    vsnprintf(buffer, 512, format, args);
    va_end(args);
}

/*
 * security_get_error_message
 *    Get error message for security error code
 */
const char *
security_get_error_message(SecurityErrorCode error_code)
{
    switch (error_code)
    {
        case SECURITY_SUCCESS:
            return "Success";
        case SECURITY_ERROR_INVALID_PARAM:
            return "Invalid parameters";
        case SECURITY_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case SECURITY_ERROR_ENCRYPT_FAILED:
            return "Encryption failed";
        case SECURITY_ERROR_DECRYPT_FAILED:
            return "Decryption failed";
        case SECURITY_ERROR_HMAC_FAILED:
            return "HMAC generation failed";
        case SECURITY_ERROR_HMAC_VERIFY_FAILED:
            return "HMAC verification failed";
        case SECURITY_ERROR_KEY_DERIVATION_FAILED:
            return "Key derivation failed";
        case SECURITY_ERROR_RNG_FAILURE:
            return "Random number generation failed";
        case SECURITY_ERROR_POOR_RANDOMNESS:
            return "Poor randomness quality";
        case SECURITY_ERROR_WEAK_KEY:
            return "Weak key detected";
        case SECURITY_ERROR_UNSUPPORTED_ALGORITHM:
            return "Unsupported algorithm";
        default:
            return "Unknown security error";
    }
}

/*
 * security_generate_random_bytes
 *    Generate cryptographically secure random bytes (Modify - ReqID:TECC4.0)
 *
 * On Linux: uses getrandom(2) to avoid the OpenSSL 3.0 CRYPTO_THREAD_write_lock
 * self-deadlock observed with RAND_bytes (100% CPU spin). Falls back
 * to /dev/urandom on kernels < 3.17 or extreme EAGAIN conditions.
 * On Windows/MSVC: uses OpenSSL RAND_bytes (the deadlock is Linux-specific).
 */
int
security_generate_random_bytes(unsigned char *buffer, size_t length)
{
#ifdef _WIN32
    if (buffer == NULL || length == 0)
    {
        set_security_error("Invalid parameters for random generation");
        return SECURITY_ERROR_INVALID_PARAM;
    }

    if (RAND_bytes(buffer, (int)length) == 1)
        return SECURITY_SUCCESS;

    set_security_error("RAND_bytes failed");
    return SECURITY_ERROR_RNG_FAILURE;
#else
    ssize_t n;

    if (buffer == NULL || length == 0)
    {
        set_security_error("Invalid parameters for random generation");
        return SECURITY_ERROR_INVALID_PARAM;
    }

    /* getrandom(2) — available since Linux 3.17, glibc 2.25 */
    n = getrandom(buffer, length, 0);
    if (n == (ssize_t)length)
        return SECURITY_SUCCESS;

    /* Partial read — loop for the remainder */
    if (n > 0)
    {
        size_t remaining = length - (size_t)n;
        unsigned char *p = buffer + n;

        while (remaining > 0)
        {
            ssize_t m = getrandom(p, remaining, 0);
            if (m <= 0)
                goto fallback;
            p += m;
            remaining -= (size_t)m;
        }
        return SECURITY_SUCCESS;
    }

fallback:
    /* /dev/urandom fallback */
    {
        FILE *fp = fopen("/dev/urandom", "rb");
        size_t nread;

        if (!fp)
        {
            set_security_error("getrandom failed and /dev/urandom unavailable");
            return SECURITY_ERROR_RNG_FAILURE;
        }

        nread = fread(buffer, 1, length, fp);
        fclose(fp);

        if (nread == length)
            return SECURITY_SUCCESS;

        set_security_error("getrandom + /dev/urandom read %zu of %zu bytes",
                           nread, length);
        return SECURITY_ERROR_RNG_FAILURE;
    }
#endif /* _WIN32 */
}

/*
 * security_generate_random_int
 *    Generate cryptographically secure random integer in range [min, max]
 */
int
security_generate_random_int(int min, int max, int *result)
{
    unsigned char random_bytes[4];
    unsigned int random_value;
    int range;
    int ret;

    if (result == NULL || min > max)
    {
        set_security_error("Invalid parameters for random integer generation");
        return SECURITY_ERROR_INVALID_PARAM;
    }

    /* Generate 4 random bytes */
    ret = security_generate_random_bytes(random_bytes, sizeof(random_bytes));
    if (ret != SECURITY_SUCCESS)
        return ret;

    /* Convert to unsigned integer */
    random_value = (random_bytes[0] << 24) | (random_bytes[1] << 16) |
                   (random_bytes[2] << 8) | random_bytes[3];

    /* Map to range [min, max] */
    range = max - min + 1;
    *result = min + (random_value % range);

    return SECURITY_SUCCESS;
}

/*
 * security_check_randomness_quality
 *    Check randomness quality (basic entropy check)
 */
bool
security_check_randomness_quality(const unsigned char *data, size_t length, double threshold)
{
    size_t i;
    int byte_counts[256] = {0};
    double entropy = 0.0;
    double probability;

    if (data == NULL || length == 0 || threshold < 0.0 || threshold > 8.0)
        return false;

    /* Count byte frequencies */
    for (i = 0; i < length; i++)
    {
        byte_counts[data[i]]++;
    }

    /* Calculate Shannon entropy */
    for (i = 0; i < 256; i++)
    {
        if (byte_counts[i] > 0)
        {
            probability = (double)byte_counts[i] / length;
            entropy -= probability * log2(probability);
        }
    }

    /* Check if entropy meets threshold (bits per byte) */
    return entropy >= threshold;
}

/*
 * security_generate_dek_key
 *    Generate DEK key with specified length
 */
int
security_generate_dek_key(int key_length, unsigned char **dek_data, size_t *dek_length)
{
    unsigned char *key = NULL;
    int ret;

    if (dek_data == NULL || dek_length == NULL)
    {
        set_security_error("Invalid parameters for DEK generation");
        return SECURITY_ERROR_INVALID_PARAM;
    }

    /* Validate key length */
    if (key_length < DEK_MIN_LENGTH || key_length > DEK_MAX_LENGTH)
    {
        set_security_error("Invalid DEK length: %d (must be %d-%d)",
                          key_length, DEK_MIN_LENGTH, DEK_MAX_LENGTH);
        return SECURITY_ERROR_INVALID_PARAM;
    }

    /* Allocate memory for key */
    key = (unsigned char *)pg_malloc(key_length);
    if (key == NULL)
    {
        set_security_error("Failed to allocate memory for DEK");
        return SECURITY_ERROR_OUT_OF_MEMORY;
    }

    /* Generate random key */
    ret = security_generate_random_bytes(key, key_length);
    if (ret != SECURITY_SUCCESS)
    {
        pg_free(key);
        return ret;
    }

    /* Check for weak keys */
    if (security_is_weak_key(key, key_length))
    {
        set_security_error("Generated weak DEK key");
        memset(key, 0, key_length);
        pg_free(key);
        return SECURITY_ERROR_WEAK_KEY;
    }

    *dek_data = key;
    *dek_length = key_length;

    return SECURITY_SUCCESS;
}

/*
 * security_generate_dek_with_recovery
 *    Generate DEK key with retry mechanism
 */
int
security_generate_dek_with_recovery(int key_length, unsigned char **dek_data,
                                   size_t *dek_length, int max_retries)
{
    int ret;
    int retry_count = 0;

    if (max_retries <= 0)
        max_retries = 3;

    while (retry_count < max_retries)
    {
        ret = security_generate_dek_key(key_length, dek_data, dek_length);
        if (ret == SECURITY_SUCCESS)
            return SECURITY_SUCCESS;

        /* Only retry for weak key errors */
        if (ret != SECURITY_ERROR_WEAK_KEY)
            return ret;

        retry_count++;
    }

    set_security_error("Failed to generate DEK after %d retries", max_retries);
    return SECURITY_ERROR_WEAK_KEY;
}

/*
 * security_is_weak_key
 *    Check if key is weak (all zeros, all ones, simple patterns)
 */
bool
security_is_weak_key(const unsigned char *key, size_t key_len)
{
    size_t i;
    bool all_zero = true;
    bool all_one = true;
    bool repeating_pattern = true;
    unsigned char first_byte;
    bool incrementing = true;
    bool decrementing = true;

    if (key == NULL || key_len == 0)
        return true;

    first_byte = key[0];

    for (i = 0; i < key_len; i++)
    {
        if (key[i] != 0x00)
            all_zero = false;
        if (key[i] != 0xFF)
            all_one = false;
        if (key[i] != first_byte)
            repeating_pattern = false;
    }

    /* Also check for simple incrementing/decrementing patterns */
    for (i = 1; i < key_len; i++)
    {
        if (key[i] != key[i-1] + 1)
            incrementing = false;
        if (key[i] != key[i-1] - 1)
            decrementing = false;
    }

    return all_zero || all_one || repeating_pattern || incrementing || decrementing;
}

/*
 * security_sm4_cbc_encrypt
 *    Encrypt data using SM4-CBC algorithm
 */
int
security_sm4_cbc_encrypt(const unsigned char *plaintext, size_t plain_len,
                        const unsigned char *key, size_t key_len,
                        unsigned char **ciphertext, size_t *cipher_len)
{
    TeccSM4Context *sm4_ctx = NULL;
    unsigned char iv[IV_LENGTH];
    unsigned char *cipher = NULL;
    unsigned char *padded_plain = NULL;
    size_t pad_len;
    size_t padded_len;
    int ret = SECURITY_SUCCESS;
    size_t i;

    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                "security_sm4_cbc_encrypt called: plain_len=%zu, key_len=%zu",
                plain_len, key_len);

    if (plaintext == NULL || plain_len == 0 || key == NULL ||
        ciphertext == NULL || cipher_len == NULL)
    {
        set_security_error("Invalid parameters for SM4 encryption");
        return SECURITY_ERROR_INVALID_PARAM;
    }

    /* Validate key length - SM4 requires 16-byte key */
    if (key_len < SM4_KEY_LENGTH)
    {
        set_security_error("Invalid SM4 key length: %zu (must be at least %d)",
                          key_len, SM4_KEY_LENGTH);
        return SECURITY_ERROR_INVALID_PARAM;
    }

    /* Generate random IV */
    if (security_generate_random_bytes(iv, sizeof(iv)) != SECURITY_SUCCESS)
    {
        set_security_error("Failed to generate IV for SM4 encryption");
        return SECURITY_ERROR_RNG_FAILURE;
    }

    /* Calculate PKCS#7 padding */
    pad_len = SM4_BLOCK_SIZE - (plain_len % SM4_BLOCK_SIZE);
    if (pad_len == 0)
        pad_len = SM4_BLOCK_SIZE;
    padded_len = plain_len + pad_len;

    /* Allocate ciphertext buffer (IV + padded plaintext) */
    cipher = (unsigned char *)pg_malloc(IV_LENGTH + padded_len);
    if (cipher == NULL)
    {
        set_security_error("Failed to allocate memory for ciphertext");
        return SECURITY_ERROR_OUT_OF_MEMORY;
    }

    /* Copy IV to beginning of ciphertext */
    memcpy(cipher, iv, IV_LENGTH);

    /* Prepare padded plaintext */
    padded_plain = (unsigned char *)pg_malloc(padded_len);
    if (padded_plain == NULL)
    {
        set_security_error("Failed to allocate memory for padded plaintext");
        ret = SECURITY_ERROR_OUT_OF_MEMORY;
        goto cleanup;
    }
    memcpy(padded_plain, plaintext, plain_len);
    for (i = 0; i < pad_len; i++)
        padded_plain[plain_len + i] = (unsigned char)pad_len;

    /* Create and initialize SM4 context */
    sm4_ctx = tecc_sm4_context_new();
    if (sm4_ctx == NULL)
    {
        set_security_error("Failed to create SM4 context");
        ret = SECURITY_ERROR_ENCRYPT_FAILED;
        goto cleanup;
    }

    if (tecc_sm4_set_key_encrypt(sm4_ctx, key) != 1)
    {
        set_security_error("Failed to set SM4 encryption key");
        ret = SECURITY_ERROR_ENCRYPT_FAILED;
        goto cleanup;
    }

    /* SM4-CBC encrypt the padded plaintext */
    if (tecc_sm4_cbc_encrypt(sm4_ctx, iv, padded_plain, padded_len,
                             cipher + IV_LENGTH) != 1)
    {
        set_security_error("SM4 encryption failed");
        ret = SECURITY_ERROR_ENCRYPT_FAILED;
        goto cleanup;
    }

    /* Set output parameters */
    *ciphertext = cipher;
    *cipher_len = IV_LENGTH + padded_len;

cleanup:
    if (sm4_ctx != NULL)
        tecc_sm4_context_free(sm4_ctx);

    if (padded_plain != NULL)
    {
        memset(padded_plain, 0, padded_len);
        pg_free(padded_plain);
    }

    if (ret != SECURITY_SUCCESS && cipher != NULL)
    {
        memset(cipher, 0, IV_LENGTH + padded_len);
        pg_free(cipher);
    }

    return ret;
}

/*
 * security_sm4_cbc_decrypt
 *    Decrypt data using SM4-CBC algorithm
 */
int
security_sm4_cbc_decrypt(const unsigned char *ciphertext, size_t cipher_len,
                        const unsigned char *key, size_t key_len,
                        unsigned char **plaintext, size_t *plain_len)
{
    TeccSM4Context *sm4_ctx = NULL;
    unsigned char iv[IV_LENGTH];
    unsigned char *padded_plain = NULL;
    size_t data_len;
    size_t pad_len;
    int ret = SECURITY_SUCCESS;
    size_t i;

    if (ciphertext == NULL || cipher_len <= IV_LENGTH || key == NULL ||
        plaintext == NULL || plain_len == NULL)
    {
        set_security_error("Invalid parameters for SM4 decryption");
        return SECURITY_ERROR_INVALID_PARAM;
    }

    /* Validate key length - SM4 requires 16-byte key */
    if (key_len < SM4_KEY_LENGTH)
    {
        set_security_error("Invalid SM4 key length: %zu (must be at least %d)",
                          key_len, SM4_KEY_LENGTH);
        return SECURITY_ERROR_INVALID_PARAM;
    }

    /* Extract IV from ciphertext */
    memcpy(iv, ciphertext, IV_LENGTH);

    data_len = cipher_len - IV_LENGTH;

    /* Validate data length is multiple of block size */
    if (data_len % SM4_BLOCK_SIZE != 0)
    {
        set_security_error("Invalid ciphertext length: not multiple of block size");
        return SECURITY_ERROR_INVALID_PARAM;
    }

    /* Allocate buffer for padded plaintext */
    padded_plain = (unsigned char *)pg_malloc(data_len);
    if (padded_plain == NULL)
    {
        set_security_error("Failed to allocate memory for plaintext");
        return SECURITY_ERROR_OUT_OF_MEMORY;
    }

    /* Create and initialize SM4 context */
    sm4_ctx = tecc_sm4_context_new();
    if (sm4_ctx == NULL)
    {
        set_security_error("Failed to create SM4 context");
        ret = SECURITY_ERROR_DECRYPT_FAILED;
        goto cleanup;
    }

    if (tecc_sm4_set_key_decrypt(sm4_ctx, key) != 1)
    {
        set_security_error("Failed to set SM4 decryption key");
        ret = SECURITY_ERROR_DECRYPT_FAILED;
        goto cleanup;
    }

    /* SM4-CBC decrypt */
    if (tecc_sm4_cbc_decrypt(sm4_ctx, iv, ciphertext + IV_LENGTH, data_len,
                             padded_plain) != 1)
    {
        set_security_error("SM4 decryption failed");
        ret = SECURITY_ERROR_DECRYPT_FAILED;
        goto cleanup;
    }

    /* Remove PKCS#7 padding */
    pad_len = padded_plain[data_len - 1];
    if (pad_len == 0 || pad_len > SM4_BLOCK_SIZE || pad_len > data_len)
    {
        set_security_error("Invalid PKCS#7 padding length: %zu", (size_t)pad_len);
        ret = SECURITY_ERROR_DECRYPT_FAILED;
        goto cleanup;
    }

    /* Verify all padding bytes */
    for (i = 1; i <= pad_len; i++)
    {
        if (padded_plain[data_len - i] != pad_len)
        {
            set_security_error("Invalid PKCS#7 padding at position %zu", (size_t)(data_len - i));
            ret = SECURITY_ERROR_DECRYPT_FAILED;
            goto cleanup;
        }
    }

    /* Set output parameters */
    *plaintext = padded_plain;
    *plain_len = data_len - pad_len;

    /* Transfer ownership - plaintext now owns padded_plain buffer */
    padded_plain = NULL;

cleanup:
    if (sm4_ctx != NULL)
        tecc_sm4_context_free(sm4_ctx);

    if (ret != SECURITY_SUCCESS && padded_plain != NULL)
    {
        memset(padded_plain, 0, data_len);
        pg_free(padded_plain);
    }

    return ret;
}

/*
 * security_encrypt_data
 *    Generic encryption function supporting multiple algorithms
 */
int
security_encrypt_data(const unsigned char *plaintext, size_t plain_len,
                     const unsigned char *key, size_t key_len,
                     EncryptionAlgorithm algorithm,
                     unsigned char **ciphertext, size_t *cipher_len)
{
    switch (algorithm)
    {
        case ENCRYPT_ALGO_SM4_CBC:
        {
            /* SM4-CBC uses only first 16 bytes of key */
            size_t sm4_key_len = 16;
            if (key_len < sm4_key_len)
            {
                set_security_error("Key too short for SM4 encryption: %zu bytes", key_len);
                return SECURITY_ERROR_INVALID_PARAM;
            }
            return security_sm4_cbc_encrypt(plaintext, plain_len, key, sm4_key_len,
                                           ciphertext, cipher_len);
        }
        case ENCRYPT_ALGO_AES_256_CBC:
            /* TODO: Implement AES-256-CBC */
            set_security_error("AES-256-CBC not implemented yet");
            return SECURITY_ERROR_UNSUPPORTED_ALGORITHM;
        default:
            set_security_error("Unsupported encryption algorithm: %d", algorithm);
            return SECURITY_ERROR_UNSUPPORTED_ALGORITHM;
    }
}

/*
 * security_decrypt_data
 *    Generic decryption function supporting multiple algorithms
 */
int
security_decrypt_data(const unsigned char *ciphertext, size_t cipher_len,
                     const unsigned char *key, size_t key_len,
                     EncryptionAlgorithm algorithm,
                     unsigned char **plaintext, size_t *plain_len)
{
    switch (algorithm)
    {
        case ENCRYPT_ALGO_SM4_CBC:
        {
            /* SM4-CBC uses only first 16 bytes of key */
            size_t sm4_key_len = 16;
            if (key_len < sm4_key_len)
            {
                set_security_error("Key too short for SM4 decryption: %zu bytes", key_len);
                return SECURITY_ERROR_INVALID_PARAM;
            }
            return security_sm4_cbc_decrypt(ciphertext, cipher_len, key, sm4_key_len,
                                           plaintext, plain_len);
        }
        case ENCRYPT_ALGO_AES_256_CBC:
            /* TODO: Implement AES-256-CBC */
            set_security_error("AES-256-CBC not implemented yet");
            return SECURITY_ERROR_UNSUPPORTED_ALGORITHM;
        default:
            set_security_error("Unsupported encryption algorithm: %d", algorithm);
            return SECURITY_ERROR_UNSUPPORTED_ALGORITHM;
    }
}

/*
 * security_generate_hmac
 *    Generate HMAC for data
 */
int
security_generate_hmac(const unsigned char *data, size_t data_len,
                      const unsigned char *key, size_t key_len,
                      HmacAlgorithm algorithm,
                      unsigned char *hmac_output, size_t *hmac_len)
{
    const EVP_MD *md = NULL;
    unsigned int hmac_length = 0;

    if (data == NULL || data_len == 0 || key == NULL || key_len == 0 ||
        hmac_output == NULL || hmac_len == NULL)
    {
        set_security_error("Invalid parameters for HMAC generation");
        return SECURITY_ERROR_INVALID_PARAM;
    }

    /* Select hash algorithm */
    switch (algorithm)
    {
        case HMAC_ALGO_SHA256:
            md = EVP_sha256();
            *hmac_len = HMAC_SHA256_LENGTH;
            break;
        case HMAC_ALGO_SHA384:
            md = EVP_sha384();
            *hmac_len = 48;  /* SHA384 output length */
            break;
        case HMAC_ALGO_SHA512:
            md = EVP_sha512();
            *hmac_len = 64;  /* SHA512 output length */
            break;
        default:
            set_security_error("Unsupported HMAC algorithm: %d", algorithm);
            return SECURITY_ERROR_UNSUPPORTED_ALGORITHM;
    }

    /* Generate HMAC */
    if (HMAC(md, key, key_len, data, data_len, hmac_output, &hmac_length) == NULL)
    {
        set_security_error("HMAC generation failed: %s",
                          ERR_error_string(ERR_get_error(), NULL));
        return SECURITY_ERROR_HMAC_FAILED;
    }

    if (hmac_length != *hmac_len)
    {
        set_security_error("Unexpected HMAC length: %u (expected %zu)",
                          hmac_length, *hmac_len);
        return SECURITY_ERROR_HMAC_FAILED;
    }

    return SECURITY_SUCCESS;
}

/*
 * security_verify_hmac
 *    Verify HMAC for data
 */
int
security_verify_hmac(const unsigned char *data, size_t data_len,
                    const unsigned char *key, size_t key_len,
                    HmacAlgorithm algorithm,
                    const unsigned char *hmac_to_verify, size_t hmac_len)
{
    unsigned char calculated_hmac[EVP_MAX_MD_SIZE];
    size_t calculated_len = 0;
    int ret;

    if (data == NULL || data_len == 0 || key == NULL || key_len == 0 ||
        hmac_to_verify == NULL || hmac_len == 0)
    {
        set_security_error("Invalid parameters for HMAC verification");
        return SECURITY_ERROR_INVALID_PARAM;
    }

    /* Generate HMAC for comparison */
    ret = security_generate_hmac(data, data_len, key, key_len, algorithm,
                                calculated_hmac, &calculated_len);
    if (ret != SECURITY_SUCCESS)
        return ret;

    /* Compare HMACs using constant-time comparison */
    if (hmac_len != calculated_len)
    {
        set_security_error("HMAC length mismatch: %zu != %zu",
                          hmac_len, calculated_len);
        return SECURITY_ERROR_HMAC_VERIFY_FAILED;
    }

    if (security_constant_time_compare(hmac_to_verify, calculated_hmac, hmac_len) != 0)
    {
        set_security_error("HMAC verification failed");
        return SECURITY_ERROR_HMAC_VERIFY_FAILED;
    }

    return SECURITY_SUCCESS;
}

/*
 * security_derive_hmac_key
 *    Derive HMAC key from KEK
 */
int
security_derive_hmac_key(const unsigned char *kek, size_t kek_len,
                        unsigned char **hmac_key, size_t *hmac_key_len)
{

    /* Use key derivation function */
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                "security_derive_hmac_key: Calling security_derive_key_from_kek");
    return security_derive_key_from_kek(kek, kek_len,
                                       (const unsigned char *)HMAC_KEY_DERIVATION_SALT,
                                       HMAC_KEY_DERIVATION_SALT_LENGTH,
                                       "HMAC_KEY",
                                       hmac_key, hmac_key_len);
}

/*
 * security_derive_key_from_kek
 *    Derive key from KEK using PBKDF2-HMAC-SHA256
 */
int
security_derive_key_from_kek(const unsigned char *kek, size_t kek_len,
                            const unsigned char *salt, size_t salt_len,
                            const char *context,
                            unsigned char **derived_key, size_t *derived_len)
{
    unsigned char *key = NULL;
    size_t key_len = 32;  /* 256-bit derived key */
    unsigned char *actual_salt = NULL;
    size_t actual_salt_len = 0;
    unsigned char *info = NULL;
    size_t info_len = 0;
    int ret = SECURITY_SUCCESS;
    unsigned int iterations;
    size_t default_salt_length = 16;  /* 128-bit salt */

    /* Parameter validation */
    if (kek == NULL || kek_len == 0 || derived_key == NULL || derived_len == NULL)
    {
        set_security_error("Invalid parameters for key derivation");
        return SECURITY_ERROR_INVALID_PARAM;
    }

    /* Debug logging */
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                "PBKDF2: Starting key derivation, kek_len=%zu, salt_len=%zu, context=%s",
            kek_len, salt_len, context ? context : "(null)");

    /* Default PBKDF2 configuration */
    iterations = PBKDF2_ITERATIONS;

    /* Step 1: Handle salt - use provided salt or generate random salt */
    if (salt == NULL || salt_len == 0)
    {
        /* Generate random salt */
        actual_salt_len = default_salt_length;
        actual_salt = (unsigned char *)pg_malloc(actual_salt_len);
        if (actual_salt == NULL)
        {
            set_security_error("Failed to allocate memory for salt");
            return SECURITY_ERROR_OUT_OF_MEMORY;
        }

        ret = security_generate_random_bytes(actual_salt, actual_salt_len);
        if (ret != SECURITY_SUCCESS)
        {
            pg_free(actual_salt);
            set_security_error("Failed to generate random salt");
            return SECURITY_ERROR_RNG_FAILURE;
        }

        /* Debug: log generated salt */
        tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                    "PBKDF2: Generated random salt, length=%zu", actual_salt_len);
    }
    else
    {
        /* Use provided salt */
        actual_salt = (unsigned char *)salt;
        actual_salt_len = salt_len;
    }

    /* Step 2: Build derivation info (KEK + context) */
    if (context != NULL)
    {
        size_t context_len = strlen(context);
        info_len = kek_len + context_len;
        info = (unsigned char *)pg_malloc(info_len);
        if (info == NULL)
        {
            if (salt == NULL || salt_len == 0)
            {
                memset(actual_salt, 0, actual_salt_len);
                pg_free(actual_salt);
            }
            set_security_error("Failed to allocate memory for derivation info");
            return SECURITY_ERROR_OUT_OF_MEMORY;
        }

        memcpy(info, kek, kek_len);
        memcpy(info + kek_len, context, context_len);
    }
    else
    {
        /* If no context provided, use KEK as info */
        info_len = kek_len;
        info = (unsigned char *)pg_malloc(info_len);
        if (info == NULL)
        {
            if (salt == NULL || salt_len == 0)
            {
                memset(actual_salt, 0, actual_salt_len);
                pg_free(actual_salt);
            }
            set_security_error("Failed to allocate memory for derivation info");
            return SECURITY_ERROR_OUT_OF_MEMORY;
        }
        memcpy(info, kek, kek_len);
    }

    /* Step 3: Allocate memory for derived key */
    key = (unsigned char *)pg_malloc(key_len);
    if (key == NULL)
    {
        if (salt == NULL || salt_len == 0)
        {
            memset(actual_salt, 0, actual_salt_len);
            pg_free(actual_salt);
        }
        memset(info, 0, info_len);
        pg_free(info);
        set_security_error("Failed to allocate memory for derived key");
        return SECURITY_ERROR_OUT_OF_MEMORY;
    }

    /* Step 4: Perform PBKDF2-HMAC-SHA256 key derivation.
     * Manual implementation using HMAC() for cross-version compatibility
     * with both OpenSSL 1.0.x and 3.0.x.  Avoids the deprecated
     * PKCS5_PBKDF2_HMAC on 3.x (which can fail due to provider refcount
     * issues) as well as the version-specific EVP_KDF API (3.0+ only).
     * HMAC() is already used elsewhere in this file (security_generate_hmac)
     * and works on all supported OpenSSL versions. */
    {
        unsigned char _U[32], _T[32], *_sb;
        unsigned int _outlen;
        int _i;

        /* Allocate salt || INT32_BE(1) block */
        _sb = (unsigned char *)pg_malloc(actual_salt_len + 4);
        if (_sb == NULL)
            ret = SECURITY_ERROR_OUT_OF_MEMORY;
        else
        {
            ret = SECURITY_ERROR_KEY_DERIVATION_FAILED;

            /* Build salt || 0x00000001 as input to first HMAC */
            memcpy(_sb, actual_salt, actual_salt_len);
            _sb[actual_salt_len]     = 0x00;
            _sb[actual_salt_len + 1] = 0x00;
            _sb[actual_salt_len + 2] = 0x00;
            _sb[actual_salt_len + 3] = 0x01;

            /* ReqID:TECC4.0 */
            tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                        "PBKDF2: Manual PBKDF2-HMAC-SHA256, info_len=%zu, salt_len=%zu, iterations=%u, key_len=%zu",
                    info_len, actual_salt_len, iterations, key_len);

            /* U_1 = HMAC-SHA256(password, salt || INT32_BE(1)) */
            if (HMAC(EVP_sha256(), info, (int)info_len,
                     _sb, (int)(actual_salt_len + 4), _U, &_outlen) != NULL)
            {
                memcpy(_T, _U, 32);

                /* Remaining iterations: U_i = HMAC(password, U_{i-1}), T ^= U_i */
                for (_i = 1; _i < (int)iterations; _i++)
                {
                    int _j;
                    if (HMAC(EVP_sha256(), info, (int)info_len,
                             _U, 32, _U, &_outlen) == NULL)
                        break;
                    for (_j = 0; _j < 32; _j++)
                        _T[_j] ^= _U[_j];
                }

                if (_i >= (int)iterations)
                {
                    /* Copy result, truncating to key_len if < 32 bytes */
                    memcpy(key, _T, (key_len < 32) ? key_len : 32);
                    ret = SECURITY_SUCCESS;
                }
            }

            memset(_sb, 0, actual_salt_len + 4);
            pg_free(_sb);
        }

        /* Clean up local HMAC state from stack */
        memset(_U, 0, sizeof(_U));
        memset(_T, 0, sizeof(_T));

        /* ReqID:TECC4.0 */
        tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                    "PBKDF2: result=%s",
                (ret == SECURITY_SUCCESS) ? "success" : "FAILED");
    }

    if (ret != SECURITY_SUCCESS)
    {
        /* PBKDF2 failed - log OpenSSL error queue to stderr (no heap alloc) */
        {
            char _buf[256];
            int _n = snprintf(_buf, sizeof(_buf), "PBKDF2 FAILED info_len=%d salt_len=%d\n",
                (int)info_len, (int)actual_salt_len);
            int _fd2 = 2;
            if (_n > 0) write(_fd2, _buf, _n < (int)sizeof(_buf) ? _n : (int)sizeof(_buf));
            unsigned long _e;
            while ((_e = ERR_get_error()) != 0) {
                ERR_error_string_n(_e, _buf, sizeof(_buf));
                write(_fd2, _buf, strlen(_buf));
                write(_fd2, "\n", 1);
            }
        }
        if (salt == NULL || salt_len == 0)
        {
            memset(actual_salt, 0, actual_salt_len);
            pg_free(actual_salt);
        }
        memset(info, 0, info_len);
        pg_free(info);
        memset(key, 0, key_len);
        pg_free(key);
        set_security_error("PBKDF2 key derivation failed");
        return SECURITY_ERROR_KEY_DERIVATION_FAILED;
    }

    /* Step 5: Cleanup and return results */
    if (salt == NULL || salt_len == 0)
    {
        memset(actual_salt, 0, actual_salt_len);
        pg_free(actual_salt);
    }

    memset(info, 0, info_len);
    pg_free(info);

    /* Debug: log successful derivation */

    *derived_key = key;
    *derived_len = key_len;

    return SECURITY_SUCCESS;
}

/*
 * security_encrypt_dek_with_hmac
 *    Encrypt DEK with HMAC for integrity
 */
int
security_encrypt_dek_with_hmac(const unsigned char *plain_dek, size_t plain_dek_len,
                              const unsigned char *kek, size_t kek_len,
                              unsigned char **encrypted_dek, size_t *encrypted_dek_len)
{
    unsigned char *ciphertext = NULL;
    size_t cipher_len = 0;
    unsigned char *hmac_key = NULL;
    size_t hmac_key_len = 0;
    unsigned char hmac[HMAC_SHA256_LENGTH];
    unsigned char *combined = NULL;
    size_t combined_len = 0;
    int ret;
    size_t sm4_key_len = 16;

    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                "[ENCRYPT] DEK encryption start, plain_dek_len=%zu, kek_len=%zu",
                plain_dek_len, kek_len);

    if (plain_dek == NULL || plain_dek_len == 0 || kek == NULL || kek_len == 0 ||
        encrypted_dek == NULL || encrypted_dek_len == NULL)
    {
        set_security_error("Invalid parameters for DEK encryption with HMAC");
        return SECURITY_ERROR_INVALID_PARAM;
    }

    /* Step 1: Encrypt DEK using SM4-CBC - use first 16 bytes of KEK */
    if (kek_len < sm4_key_len)
    {
        set_security_error("KEK too short for SM4 encryption: %zu bytes", kek_len);
        return SECURITY_ERROR_INVALID_PARAM;
    }
    ret = security_sm4_cbc_encrypt(plain_dek, plain_dek_len, kek, sm4_key_len,
                                  &ciphertext, &cipher_len);
    if (ret != SECURITY_SUCCESS)
    {
        return ret;
    }

    /* Step 2: Derive HMAC key from KEK */
    ret = security_derive_hmac_key(kek, kek_len, &hmac_key, &hmac_key_len);
    if (ret != SECURITY_SUCCESS)
    {
        memset(ciphertext, 0, cipher_len);
        pg_free(ciphertext);
        return ret;
    }

    /* Step 3: Generate HMAC for ciphertext */
    ret = security_generate_hmac(ciphertext, cipher_len, hmac_key, hmac_key_len,
                                HMAC_ALGO_SHA256, hmac, &hmac_key_len);
    if (ret != SECURITY_SUCCESS)
    {
        memset(ciphertext, 0, cipher_len);
        pg_free(ciphertext);
        memset(hmac_key, 0, hmac_key_len);
        pg_free(hmac_key);
        return ret;
    }

    /* Step 4: Combine ciphertext and HMAC */
    combined_len = cipher_len + HMAC_SHA256_LENGTH;
    combined = (unsigned char *)pg_malloc(combined_len);
    if (combined == NULL)
    {
        memset(ciphertext, 0, cipher_len);
        pg_free(ciphertext);
        memset(hmac_key, 0, hmac_key_len);
        pg_free(hmac_key);
        set_security_error("Failed to allocate memory for combined data");
        return SECURITY_ERROR_OUT_OF_MEMORY;
    }

    memcpy(combined, ciphertext, cipher_len);
    memcpy(combined + cipher_len, hmac, HMAC_SHA256_LENGTH);

    /* Step 5: Cleanup and set output */
    memset(ciphertext, 0, cipher_len);
    pg_free(ciphertext);
    memset(hmac_key, 0, hmac_key_len);
    pg_free(hmac_key);

    *encrypted_dek = combined;
    *encrypted_dek_len = combined_len;

    return SECURITY_SUCCESS;
}

/*
 * security_decrypt_dek_with_hmac
 *    Decrypt DEK with HMAC verification
 */
int
security_decrypt_dek_with_hmac(const unsigned char *encrypted_dek, size_t encrypted_dek_len,
                              const unsigned char *kek, size_t kek_len,
                              unsigned char **plain_dek, size_t *plain_dek_len)
{
    unsigned char *ciphertext = NULL;
    size_t cipher_len = 0;
    unsigned char *hmac_key = NULL;
    size_t hmac_key_len = 0;
    const unsigned char *received_hmac = NULL;
    int ret;
    size_t sm4_key_len = 16;

    if (encrypted_dek == NULL || encrypted_dek_len <= HMAC_SHA256_LENGTH ||
        kek == NULL || kek_len == 0 || plain_dek == NULL || plain_dek_len == NULL)
    {
        set_security_error("Invalid parameters for DEK decryption with HMAC");
        return SECURITY_ERROR_INVALID_PARAM;
    }

    /* Split encrypted data into ciphertext and HMAC */
    cipher_len = encrypted_dek_len - HMAC_SHA256_LENGTH;
    ciphertext = (unsigned char *)pg_malloc(cipher_len);
    if (ciphertext == NULL)
    {
        set_security_error("Failed to allocate memory for ciphertext");
        return SECURITY_ERROR_OUT_OF_MEMORY;
    }
    memcpy(ciphertext, encrypted_dek, cipher_len);
    received_hmac = encrypted_dek + cipher_len;

    /* Step 1: Derive HMAC key from KEK */
    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                "[DECRYPT] DEK decryption start, encrypted_len=%zu, kek_len=%zu",
                encrypted_dek_len, kek_len);
    ret = security_derive_hmac_key(kek, kek_len, &hmac_key, &hmac_key_len);
    if (ret != SECURITY_SUCCESS)
    {
        memset(ciphertext, 0, cipher_len);
        pg_free(ciphertext);
        return ret;
    }

    /* Step 2: Verify HMAC */
    /* Debug: print HMAC comparison */
    {
        unsigned char debug_computed_hmac[HMAC_SHA256_LENGTH];
        size_t debug_hmac_len = HMAC_SHA256_LENGTH;
        memset(debug_computed_hmac, 0, sizeof(debug_computed_hmac));
        ret = security_generate_hmac(ciphertext, cipher_len, hmac_key, hmac_key_len,
                                    HMAC_ALGO_SHA256, debug_computed_hmac, &debug_hmac_len);
        tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                    "[HMAC DEBUG] Received HMAC(first 16): %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                    received_hmac[0], received_hmac[1], received_hmac[2], received_hmac[3],
                    received_hmac[4], received_hmac[5], received_hmac[6], received_hmac[7],
                    received_hmac[8], received_hmac[9], received_hmac[10], received_hmac[11],
                    received_hmac[12], received_hmac[13], received_hmac[14], received_hmac[15]);
        if (ret == SECURITY_SUCCESS)
        {
            tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                        "[HMAC DEBUG] Computed HMAC: %02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
                        debug_computed_hmac[0], debug_computed_hmac[1], debug_computed_hmac[2], debug_computed_hmac[3],
                        debug_computed_hmac[4], debug_computed_hmac[5], debug_computed_hmac[6], debug_computed_hmac[7],
                        debug_computed_hmac[8], debug_computed_hmac[9], debug_computed_hmac[10], debug_computed_hmac[11],
                        debug_computed_hmac[12], debug_computed_hmac[13], debug_computed_hmac[14], debug_computed_hmac[15]);
            tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                        "[HMAC DEBUG] kek_len=%zu, cipher_len=%zu, hmac_key_len=%zu",
                        kek_len, cipher_len, hmac_key_len);
        }
    }

    ret = security_verify_hmac(ciphertext, cipher_len, hmac_key, hmac_key_len,
                              HMAC_ALGO_SHA256, received_hmac, HMAC_SHA256_LENGTH);
    if (ret != SECURITY_SUCCESS)
    {
        tecc_key_log(LOG_LEVEL_WARNING, __func__, __FILE__, __LINE__,
                    "HMAC verification FAILED, skipping SM4 decrypt");
        memset(ciphertext, 0, cipher_len);
        pg_free(ciphertext);
        memset(hmac_key, 0, hmac_key_len);
        pg_free(hmac_key);
        return ret;
    }

    /* Step 3: Decrypt ciphertext - use first 16 bytes of KEK for SM4 */
    if (kek_len < sm4_key_len)
    {
        set_security_error("KEK too short for SM4 decryption: %zu bytes", kek_len);
        memset(ciphertext, 0, cipher_len);
        pg_free(ciphertext);
        memset(hmac_key, 0, hmac_key_len);
        pg_free(hmac_key);
        return SECURITY_ERROR_INVALID_PARAM;
    }
    ret = security_sm4_cbc_decrypt(ciphertext, cipher_len, kek, sm4_key_len,
                                  plain_dek, plain_dek_len);

    /* Step 4: Cleanup */
    memset(ciphertext, 0, cipher_len);
    pg_free(ciphertext);
    memset(hmac_key, 0, hmac_key_len);
    pg_free(hmac_key);

    return ret;
}

/*
 * security_secure_erase
 *    Securely erase memory
 */
void
security_secure_erase(void *data, size_t length)
{
    if (data != NULL && length > 0)
    {
        /* Multiple passes for secure erasure */
        volatile unsigned char *p = (volatile unsigned char *)data;
        size_t i;

        /* Pass 1: Write zeros */
        for (i = 0; i < length; i++)
            p[i] = 0x00;

        /* Pass 2: Write ones */
        for (i = 0; i < length; i++)
            p[i] = 0xFF;

        /* Pass 3: Write random pattern */
        for (i = 0; i < length; i++)
            p[i] = (unsigned char)(rand() % 256);

        /* Final pass: Write zeros */
        for (i = 0; i < length; i++)
            p[i] = 0x00;
    }
}

/*
 * security_secure_alloc
 *    Allocate memory with secure erasure on free
 */
void *
security_secure_alloc(size_t size)
{
    void *ptr = pg_malloc(size);
    if (ptr != NULL)
        memset(ptr, 0, size);
    return ptr;
}

/*
 * security_secure_free
 *    Free memory with secure erasure
 */
void
security_secure_free(void *ptr, size_t size)
{
    if (ptr != NULL)
    {
        security_secure_erase(ptr, size);
        pg_free(ptr);
    }
}

/*
 * security_constant_time_compare
 *    Constant-time memory comparison to prevent timing attacks
 */
int
security_constant_time_compare(const void *a, const void *b, size_t len)
{
    const unsigned char *pa = (const unsigned char *)a;
    const unsigned char *pb = (const unsigned char *)b;
    unsigned char result = 0;
    size_t i;

    for (i = 0; i < len; i++)
        result |= pa[i] ^ pb[i];

    return result;
}

/*
 * security_generate_kek_file_hmac
 *    Generate HMAC for KEK file data
 *
 * Parameters:
 *   file_data: KEK file data (excluding the HMAC field at the end)
 *   data_len: Length of file data (excluding HMAC field)
 *   hmac_key: HMAC key
 *   hmac_key_len: Length of HMAC key
 *   hmac_output: Output buffer for HMAC (must be at least 32 bytes)
 *
 * Returns:
 *   SECURITY_SUCCESS on success, error code on failure
 */
int
security_generate_kek_file_hmac(const unsigned char *file_data,
                               size_t data_len,
                               const unsigned char *hmac_key,
                               size_t hmac_key_len,
                               unsigned char *hmac_output)
{
    const EVP_MD *md = NULL;
    HMAC_CTX *ctx = NULL;
    unsigned int hmac_len = HMAC_SHA256_LENGTH;
    int ret = SECURITY_SUCCESS;

    /* Parameter validation */
    if (file_data == NULL || data_len == 0 ||
        hmac_key == NULL || hmac_key_len == 0 ||
        hmac_output == NULL)
    {
        set_security_error("Invalid parameters for KEK file HMAC generation");
        return SECURITY_ERROR_INVALID_PARAM;
    }

    /* Get SHA256 digest algorithm */
    md = EVP_sha256();
    if (md == NULL)
    {
        set_security_error("Failed to get SHA256 digest algorithm");
        return SECURITY_ERROR_UNSUPPORTED_ALGORITHM;
    }

    /* Create HMAC context */
#ifdef HAVE_HMAC_CTX_NEW
    ctx = HMAC_CTX_new();
#else
    ctx = (HMAC_CTX *)malloc(sizeof(HMAC_CTX));
    if (ctx != NULL)
        HMAC_CTX_init(ctx);
#endif
    if (ctx == NULL)
    {
        set_security_error("Failed to create HMAC context");
        return SECURITY_ERROR_HMAC_INIT_FAILED;
    }

    /* Initialize HMAC with SHA256 */
    if (!HMAC_Init_ex(ctx, hmac_key, hmac_key_len, md, NULL))
    {
        set_security_error("Failed to initialize HMAC context");
        ret = SECURITY_ERROR_HMAC_INIT_FAILED;
        goto cleanup;
    }

    /* Update with file data (excluding HMAC field) */
    if (!HMAC_Update(ctx, file_data, data_len))
    {
        set_security_error("Failed to update HMAC with file data");
        ret = SECURITY_ERROR_HMAC_UPDATE_FAILED;
        goto cleanup;
    }

    /* Finalize HMAC */
    if (!HMAC_Final(ctx, hmac_output, &hmac_len))
    {
        set_security_error("Failed to finalize HMAC");
        ret = SECURITY_ERROR_HMAC_FINAL_FAILED;
        goto cleanup;
    }

    /* Verify HMAC length */
    if (hmac_len != HMAC_SHA256_LENGTH)
    {
        set_security_error("Invalid HMAC length: %u (expected %d)",
                          hmac_len, HMAC_SHA256_LENGTH);
        ret = SECURITY_ERROR_HMAC_FAILED;
        goto cleanup;
    }

cleanup:
    if (ctx != NULL)
    {
#ifdef HAVE_HMAC_CTX_FREE
        HMAC_CTX_free(ctx);
#else
        HMAC_CTX_cleanup(ctx);
        free(ctx);
#endif
    }

    return ret;
}

/*
 * security_verify_kek_file_hmac
 *    Verify HMAC for KEK file data
 *
 * Parameters:
 *   file_data: Complete KEK file data (including HMAC field at the end)
 *   data_len: Total length of file data (including HMAC field)
 *   hmac_key: HMAC key
 *   hmac_key_len: Length of HMAC key
 *
 * Returns:
 *   SECURITY_SUCCESS on success, SECURITY_ERROR_HMAC_VERIFY_FAILED on failure
 */
int
security_verify_kek_file_hmac(const unsigned char *file_data,
                             size_t data_len,
                             const unsigned char *hmac_key,
                             size_t hmac_key_len)
{
    unsigned char calculated_hmac[HMAC_SHA256_LENGTH];
    const unsigned char *stored_hmac = NULL;
    size_t data_to_hash = 0;
    int ret = SECURITY_SUCCESS;

    /* Parameter validation */
    if (file_data == NULL || data_len <= HMAC_SHA256_LENGTH ||
        hmac_key == NULL || hmac_key_len == 0)
    {
        set_security_error("Invalid parameters for KEK file HMAC verification");
        return SECURITY_ERROR_INVALID_PARAM;
    }

    /* Extract stored HMAC from end of file data */
    data_to_hash = data_len - HMAC_SHA256_LENGTH;
    stored_hmac = file_data + data_to_hash;

    /* Generate HMAC for file data (excluding HMAC field) */
    ret = security_generate_kek_file_hmac(file_data, data_to_hash,
                                         hmac_key, hmac_key_len,
                                         calculated_hmac);
    if (ret != SECURITY_SUCCESS)
    {
        set_security_error("Failed to generate HMAC for verification");
        return ret;
    }

    /* Compare HMACs using constant-time comparison */
    if (security_constant_time_compare(stored_hmac, calculated_hmac,
                                      HMAC_SHA256_LENGTH) != 0)
    {
        set_security_error("KEK file HMAC verification failed - data may be tampered");
        return SECURITY_ERROR_HMAC_VERIFY_FAILED;
    }

    /* Secure cleanup of calculated HMAC */
    security_secure_erase(calculated_hmac, sizeof(calculated_hmac));

    return SECURITY_SUCCESS;
}