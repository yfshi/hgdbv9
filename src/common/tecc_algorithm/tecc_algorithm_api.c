/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_algorithm_api.c
 *
 * Abstract:
 *   Unified algorithm API implementation for TECC encryption/decryption algorithm module.
 *   Provides standardized interfaces for all TECC algorithms (RND, DET, OPE, SEARCH).
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/common/tecc_algorithm/tecc_algorithm_api.c
 */

#include "../../include/common/tecc_algorithm/tecc_algorithm_api.h"
/* #include "../../include/common/tecc_algorithm/tecc_algo_buffer.h" */
#include "../../include/common/tecc_algorithm/tecc_sm4_rnd.h"
#include "../../include/common/tecc_algorithm/tecc_det_sm4.h"
#include "../../include/common/tecc_algorithm/tecc_sm4_ope.h"
#include "../../include/common/tecc_algorithm/tecc_search_sm4.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/time.h>
#endif

/* Forward declarations for buffer size calculation functions */
size_t tecc_algo_calc_encrypt_buffer_size(TeccAlgorithmType algo_type, size_t plain_len);
size_t tecc_algo_calc_decrypt_buffer_size(TeccAlgorithmType algo_type, size_t cipher_len);

/* Error code to string mapping */
static const struct {
    int error_code;
    const char *error_string;
} g_error_strings[] = {
    {TECC_ALGO_SUCCESS, "Success"},
    {TECC_ALGO_INVALID_PARAM, "Invalid parameter"},
    {TECC_ALGO_NULL_POINTER, "Null pointer"},
    {TECC_ALGO_INVALID_ALGORITHM, "Invalid algorithm type"},
    {TECC_ALGO_INVALID_KEY_LENGTH, "Invalid key length"},
    {TECC_ALGO_INVALID_DATA_LENGTH, "Invalid data length"},
    {TECC_ALGO_MEMORY_ERROR, "Memory allocation failed"},
    {TECC_ALGO_BUFFER_OVERFLOW, "Buffer overflow"},
    {TECC_ALGO_ENCRYPTION_FAILED, "Encryption failed"},
    {TECC_ALGO_DECRYPTION_FAILED, "Decryption failed"},
    {TECC_ALGO_AUTHENTICATION_FAILED, "Authentication failed"},
    {TECC_ALGO_NOT_IMPLEMENTED, "Function not implemented"},
    {TECC_ALGO_OPERATION_NOT_SUPPORTED, "Operation not supported"},
    {TECC_ALGO_DATA_FORMAT_ERROR, "Data format error"},
    {TECC_ALGO_DATA_TOO_LARGE, "Data too large"},
    {TECC_ALGO_DATA_CORRUPTED, "Data corrupted"},
    {TECC_ALGO_SYSTEM_ERROR, "System error"},
    {TECC_ALGO_CRYPTO_LIBRARY_ERROR, "Crypto library error"},
    {0, NULL}
};

/* Algorithm capability mapping */
static const struct {
    TeccAlgorithmType algo_type;
    uint32 capabilities;
} g_algo_capabilities[] = {
    {TECC_ALGO_RND, TECC_ALGO_CAP_ENCRYPT | TECC_ALGO_CAP_DECRYPT | TECC_ALGO_CAP_AUTH},
    {TECC_ALGO_DET, TECC_ALGO_CAP_ENCRYPT | TECC_ALGO_CAP_DECRYPT},
    {TECC_ALGO_OPE, TECC_ALGO_CAP_ENCRYPT | TECC_ALGO_CAP_COMPARE | TECC_ALGO_CAP_DECRYPT},
    {TECC_ALGO_SEARCH, TECC_ALGO_CAP_ENCRYPT},
    {0, 0}
};


/*
 * Get algorithm capability
 */
uint32
tecc_algo_get_capability(TeccAlgorithmType algo_type)
{
    int i;
    for (i = 0; g_algo_capabilities[i].algo_type != 0; i++) {
        if (g_algo_capabilities[i].algo_type == algo_type) {
            return g_algo_capabilities[i].capabilities;
        }
    }
    return 0;
}

/*
 * Get error string
 */
const char *
tecc_algo_get_error_string(int error_code)
{
    int i;
    for (i = 0; g_error_strings[i].error_string != NULL; i++) {
        if (g_error_strings[i].error_code == error_code) {
            return g_error_strings[i].error_string;
        }
    }
    return "Unknown error";
}




/*
 * Convert algorithm error to scheduler error
 */
int
tecc_algo_error_to_scheduler_error(int algo_error)
{
    switch (algo_error) {
        case TECC_ALGO_SUCCESS:
            return TECC_CRYPTO_SUCCESS;

        case TECC_ALGO_INVALID_PARAM:
        case TECC_ALGO_NULL_POINTER:
        case TECC_ALGO_INVALID_ALGORITHM:
        case TECC_ALGO_INVALID_KEY_LENGTH:
        case TECC_ALGO_INVALID_DATA_LENGTH:
            return TECC_CRYPTO_INVALID_PARAM;

        case TECC_ALGO_MEMORY_ERROR:
        case TECC_ALGO_BUFFER_OVERFLOW:
            return TECC_CRYPTO_MEMORY_ERROR;

        case TECC_ALGO_ENCRYPTION_FAILED:
        case TECC_ALGO_DECRYPTION_FAILED:
        case TECC_ALGO_AUTHENTICATION_FAILED:
            return TECC_CRYPTO_ALGO_ERROR;

        case TECC_ALGO_DATA_FORMAT_ERROR:
        case TECC_ALGO_DATA_TOO_LARGE:
        case TECC_ALGO_DATA_CORRUPTED:
            return TECC_CRYPTO_DATA_ERROR;

        case TECC_ALGO_SYSTEM_ERROR:
        case TECC_ALGO_CRYPTO_LIBRARY_ERROR:
            return TECC_CRYPTO_SYSTEM_ERROR;

        case TECC_ALGO_NOT_IMPLEMENTED:
        case TECC_ALGO_OPERATION_NOT_SUPPORTED:
            return TECC_CRYPTO_SCHEDULE_ERROR;

        default:
            return TECC_CRYPTO_SYSTEM_ERROR;
    }
}

/*
 * Convert algorithm error to formatter error
 */
int
tecc_algo_error_to_formatter_error(int algo_error)
{
    switch (algo_error) {
        case TECC_ALGO_SUCCESS:
            return TECC_FORMATTER_SUCCESS;

        case TECC_ALGO_INVALID_PARAM:
        case TECC_ALGO_NULL_POINTER:
        case TECC_ALGO_INVALID_ALGORITHM:
        case TECC_ALGO_INVALID_KEY_LENGTH:
        case TECC_ALGO_INVALID_DATA_LENGTH:
            return TECC_FORMATTER_ERROR_INVALID_PARAM;

        case TECC_ALGO_MEMORY_ERROR:
        case TECC_ALGO_BUFFER_OVERFLOW:
            return TECC_FORMATTER_ERROR_MEMORY_ALLOC;

        case TECC_ALGO_DATA_FORMAT_ERROR:
        case TECC_ALGO_DATA_TOO_LARGE:
        case TECC_ALGO_DATA_CORRUPTED:
            return TECC_FORMATTER_ERROR_DATA_FORMAT;

        case TECC_ALGO_SYSTEM_ERROR:
        case TECC_ALGO_CRYPTO_LIBRARY_ERROR:
            return TECC_FORMATTER_ERROR_SYSTEM;

        default:
            return TECC_FORMATTER_ERROR_SYSTEM;
    }
}

/*
 * Secure memory cleanup
 */
void
tecc_algo_secure_cleanup(void *data, size_t len)
{
    if (data && len > 0) {
        volatile unsigned char *p = (volatile unsigned char *)data;
        size_t i;
        for (i = 0; i < len; i++) {
            p[i] = 0;
        }
    }
}

/*
 * Constant time memory comparison
 */
int
tecc_algo_constant_time_memcmp(const void *s1, const void *s2, size_t n)
{
    const volatile unsigned char *p1 = (const volatile unsigned char *)s1;
    const volatile unsigned char *p2 = (const volatile unsigned char *)s2;
    unsigned char result = 0;
    size_t i;

    for (i = 0; i < n; i++) {
        result |= p1[i] ^ p2[i];
    }

    return result;  /* 0 means equal */
}




/*
 * Zero-copy encryption interface (compatible with algorithm module interfaces)
 */
int
tecc_algo_encrypt_zero_copy(TeccAlgorithmType algo_type,
                           const unsigned char *key,
                           size_t key_len,
                           const unsigned char *plaintext,
                           size_t plain_len,
                           unsigned char *output_buffer,
                           size_t buffer_size,
                           size_t *output_len,
                           bool is_wildcard_pattern,
                           char escape_char)
{
    int ret;
    size_t required_size;

    /* Parameter validation */
    if (!key || key_len < TECC_SM4_KEY_SIZE || !plaintext ||
        !output_buffer || !output_len)
    {
        return TECC_ALGO_INVALID_PARAM;
    }

    if (plain_len > TECC_MAX_DATA_SIZE)
    {
        return TECC_ALGO_DATA_TOO_LARGE;
    }

    /* Calculate required buffer size */
    required_size = tecc_algo_calc_encrypt_buffer_size(algo_type, plain_len);
    if (buffer_size < required_size)
    {
        return TECC_ALGO_BUFFER_OVERFLOW;
    }

    /* Call specific algorithm implementation */
    /* Use only first TECC_SM4_KEY_SIZE bytes - SM4 accepts 16-byte keys */
    switch (algo_type) {
        case TECC_ALGO_RND:
            ret = tecc_sm4_rnd_encrypt(key, TECC_SM4_KEY_SIZE, plaintext, plain_len,
                                      output_buffer, buffer_size, output_len);
            break;

        case TECC_ALGO_DET:
            /* DET expects exactly TECC_SM4_KEY_SIZE (16) bytes; SM4 only uses first 16 bytes */
            ret = tecc_det_sm4_encrypt(key, TECC_SM4_KEY_SIZE, plaintext, plain_len,
                                      output_buffer, buffer_size, output_len);
            break;

        case TECC_ALGO_OPE:
            ret = tecc_sm4_ope_encrypt(key, TECC_SM4_KEY_SIZE, plaintext, plain_len,
                                      output_buffer, buffer_size, output_len);
            break;

        case TECC_ALGO_SEARCH:
            /* SEARCH expects exactly SM4_SEARCH_KEY_SIZE (16) bytes.
             * ReqID:TECC4.0 - escape_char passed through from caller. */
            ret = tecc_search_sm4_encrypt(key, TECC_SM4_KEY_SIZE, plaintext, plain_len,
                                         output_buffer, buffer_size, output_len,
                                         is_wildcard_pattern, escape_char);
            break;

        default:
            return TECC_ALGO_INVALID_ALGORITHM;
    }

    return ret;
}

/*
 * Zero-copy decryption interface (compatible with algorithm module interfaces)
 */
int
tecc_algo_decrypt_zero_copy(TeccAlgorithmType algo_type,
                           const unsigned char *key,
                           size_t key_len,
                           const unsigned char *ciphertext,
                           size_t cipher_len,
                           unsigned char *output_buffer,
                           size_t buffer_size,
                           size_t *output_len)
{
    int ret;
    size_t required_size;

    /* Parameter validation */
    if (!key || key_len < TECC_SM4_KEY_SIZE || !ciphertext || cipher_len == 0 ||
        !output_buffer || !output_len)
    {
        return TECC_ALGO_INVALID_PARAM;
    }

    if (cipher_len > TECC_MAX_DATA_SIZE * 2)  /* Allow for headers */
    {
        return TECC_ALGO_DATA_TOO_LARGE;
    }

    /* Calculate required buffer size */
    required_size = tecc_algo_calc_decrypt_buffer_size(algo_type, cipher_len);
    if (buffer_size < required_size)
    {
        return TECC_ALGO_BUFFER_OVERFLOW;
    }

    /* Call specific algorithm implementation */
    /* Use only first TECC_SM4_KEY_SIZE bytes - SM4 accepts 16-byte keys */
    switch (algo_type) {
        case TECC_ALGO_RND:
            ret = tecc_sm4_rnd_decrypt(key, TECC_SM4_KEY_SIZE, ciphertext, cipher_len,
                                      output_buffer, buffer_size, output_len);
            break;

        case TECC_ALGO_DET:
            ret = tecc_det_sm4_decrypt(key, TECC_SM4_KEY_SIZE, ciphertext, cipher_len,
                                      output_buffer, buffer_size, output_len);
            break;

        case TECC_ALGO_OPE:
            /* ReqID:TECC4.0 - Recover normalized OPE input (debug / raw segment). */
            ret = tecc_sm4_ope_decrypt(key, TECC_SM4_KEY_SIZE, ciphertext, cipher_len,
                                       output_buffer, buffer_size, output_len);
            break;

        case TECC_ALGO_SEARCH:
            /* SEARCH does not support decryption */
            ret = TECC_ALGO_OPERATION_NOT_SUPPORTED;
            break;

        default:
            return TECC_ALGO_INVALID_ALGORITHM;
    }

    return ret;
}

/*
 * Calculate required buffer size for encryption
 */
size_t
tecc_algo_calc_encrypt_buffer_size(TeccAlgorithmType algo_type,
                                  size_t plain_len)
{
    switch (algo_type) {
        case TECC_ALGO_RND:
            return tecc_sm4_rnd_calc_encrypt_buffer_size(plain_len);

        case TECC_ALGO_DET:
            return tecc_det_sm4_calc_encrypt_buffer_size(plain_len);

        case TECC_ALGO_OPE:
            return tecc_sm4_ope_calc_encrypt_buffer_size(plain_len);

        case TECC_ALGO_SEARCH:
            return tecc_search_sm4_calc_encrypt_buffer_size(plain_len);

        default:
            return 0;
    }
}

/*
 * Calculate required buffer size for decryption
 */
size_t
tecc_algo_calc_decrypt_buffer_size(TeccAlgorithmType algo_type,
                                  size_t cipher_len)
{
    switch (algo_type) {
        case TECC_ALGO_RND:
            return tecc_sm4_rnd_calc_decrypt_buffer_size(cipher_len);

        case TECC_ALGO_DET:
            return tecc_det_sm4_calc_decrypt_buffer_size(cipher_len);

        case TECC_ALGO_OPE:
            return tecc_sm4_ope_calc_decrypt_buffer_size(cipher_len);

        case TECC_ALGO_SEARCH:
            return tecc_search_sm4_calc_decrypt_buffer_size(cipher_len);

        default:
            return 0;
    }
}

/*
 * Extended zero-copy algorithm interface with operator type support
 */
int
tecc_algo_encrypt_zero_copy_ex(TeccAlgorithmType algo_type,
                              TeccOperatorType op_type,
                              const unsigned char *key,
                              size_t key_len,
                              const unsigned char *plaintext,
                              size_t plain_len,
                              unsigned char *output_buffer,
                              size_t buffer_size,
                              size_t *output_len)
{
    /* Extended operator type support not yet implemented */
    /* Use standard zero-copy encryption for basic functionality */
    (void)op_type;
    return tecc_algo_encrypt_zero_copy(algo_type, key, key_len, plaintext,
                                      plain_len, output_buffer, buffer_size,
                                      output_len, false, 0);
}

/*
 * Extended zero-copy algorithm interface with operator type support
 */
int
tecc_algo_decrypt_zero_copy_ex(TeccAlgorithmType algo_type,
                              TeccOperatorType op_type,
                              const unsigned char *key,
                              size_t key_len,
                              const unsigned char *ciphertext,
                              size_t cipher_len,
                              unsigned char *output_buffer,
                              size_t buffer_size,
                              size_t *output_len)
{
    /* Extended operator type support not yet implemented */
    /* Use standard zero-copy decryption for basic functionality */
    (void)op_type;
    return tecc_algo_decrypt_zero_copy(algo_type, key, key_len, ciphertext,
                                      cipher_len, output_buffer, buffer_size,
                                      output_len);
}

/* End - ReqID:TECC4.0 */
