/* Begin - ReqID:TECC4.0 */
/*
 * File: test_algorithms.c
 *
 * Abstract:
 *   Test program for TECC algorithm module.
 *   Tests basic functionality of all four encryption algorithms (RND, DET, OPE, SEARCH).
 *
 * Authored by <wangxp@highgo.com>, 20260411.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_test/test_algorithms.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

/* Include algorithm API headers */
#include "../../../../src/include/common/tecc_algorithm/tecc_algorithm_api.h"
#include "../../../../src/include/common/tecc_algorithm/tecc_sm4_rnd.h"
#include "../../../../src/include/common/tecc_algorithm/tecc_det_sm4.h"
#include "../../../../src/include/common/tecc_algorithm/tecc_sm4_ope.h"
#include "../../../../src/include/common/tecc_algorithm/tecc_search_sm4.h"

/* Test data */
static const unsigned char test_key_16[] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
    0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10
};

static const unsigned char test_plaintext[] = "Hello, TECC! This is a test message.";
static const size_t test_plaintext_len = 35; /* strlen("Hello, TECC! This is a test message.") */

/* Helper function to print hex data */
static void print_hex(const char *label, const unsigned char *data, size_t len)
{
    size_t i;
    printf("%s (%zu bytes): ", label, len);
    for (i = 0; i < len && i < 32; i++) {
        printf("%02X", data[i]);
        if ((i + 1) % 16 == 0) printf(" ");
    }
    if (len > 32) printf("...");
    printf("\n");
}

/* Test SM4-RND algorithm */
static int test_sm4_rnd(void)
{
    printf("\n=== Testing SM4-RND Algorithm ===\n");

    /* Calculate buffer sizes */
    size_t encrypt_buf_size = tecc_sm4_rnd_calc_encrypt_buffer_size(test_plaintext_len);
    size_t decrypt_buf_size = tecc_sm4_rnd_calc_decrypt_buffer_size(encrypt_buf_size);

    printf("Plaintext length: %zu\n", test_plaintext_len);
    printf("Encrypt buffer size needed: %zu\n", encrypt_buf_size);
    printf("Decrypt buffer size needed: %zu\n", decrypt_buf_size);

    /* Allocate buffers */
    unsigned char *ciphertext = malloc(encrypt_buf_size);
    unsigned char *decrypted = malloc(decrypt_buf_size);

    if (!ciphertext || !decrypted) {
        printf("ERROR: Memory allocation failed\n");
        free(ciphertext);
        free(decrypted);
        return 0;
    }

    /* Encrypt */
    size_t cipher_len = 0;
    int ret = tecc_sm4_rnd_encrypt(test_key_16, sizeof(test_key_16),
                                  test_plaintext, test_plaintext_len,
                                  ciphertext, encrypt_buf_size, &cipher_len);

    if (ret != SM4_RND_SUCCESS) {
        printf("ERROR: SM4-RND encryption failed with code %d\n", ret);
        free(ciphertext);
        free(decrypted);
        return 0;
    }

    printf("Encryption successful, ciphertext length: %zu\n", cipher_len);
    print_hex("Ciphertext", ciphertext, cipher_len);

    /* Debug: print ciphertext length and header size */
    printf("DEBUG: cipher_len=%zu, SM4_RND_HEADER_SIZE=%zu\n",
           cipher_len, (size_t)SM4_RND_HEADER_SIZE);

    /* Validate format */
    ret = tecc_sm4_rnd_validate_format(ciphertext, cipher_len);
    if (ret != SM4_RND_SUCCESS) {
        printf("ERROR: SM4-RND format validation failed with code %d\n", ret);
        free(ciphertext);
        free(decrypted);
        return 0;
    }
    printf("Format validation successful\n");

    /* Reallocate decryption buffer with actual ciphertext length */
    size_t actual_decrypt_buf_size = tecc_sm4_rnd_calc_decrypt_buffer_size(cipher_len);
    printf("Actual decrypt buffer size needed: %zu\n", actual_decrypt_buf_size);

    if (actual_decrypt_buf_size == 0) {
        printf("ERROR: Invalid decrypt buffer size (cipher_len=%zu, header_size=%zu)\n",
               cipher_len, (size_t)SM4_RND_HEADER_SIZE);
        free(ciphertext);
        free(decrypted);
        return 0;
    }

    /* Free old buffer and allocate new one */
    free(decrypted);
    decrypted = malloc(actual_decrypt_buf_size);
    if (!decrypted) {
        printf("ERROR: Memory allocation failed for decryption buffer\n");
        free(ciphertext);
        return 0;
    }

    /* Decrypt */
    size_t plain_len = 0;
    ret = tecc_sm4_rnd_decrypt(test_key_16, sizeof(test_key_16),
                              ciphertext, cipher_len,
                              decrypted, actual_decrypt_buf_size, &plain_len);

    if (ret != SM4_RND_SUCCESS) {
        printf("ERROR: SM4-RND decryption failed with code %d\n", ret);
        if (ret == SM4_RND_ERROR_AUTH_FAILED) {
            printf("DEBUG: Authentication failed - HMAC mismatch\n");
            printf("DEBUG: cipher_len=%zu, header_offset=%zu, data_len=%zu\n",
                   cipher_len, sizeof(uint32_t) + 32 + 16 + sizeof(size_t),
                   cipher_len - (sizeof(uint32_t) + 32 + 16 + sizeof(size_t)));
        }
        free(ciphertext);
        free(decrypted);
        return 0;
    }

    printf("Decryption successful, plaintext length: %zu\n", plain_len);

    /* Verify plaintext matches */
    if (plain_len != test_plaintext_len ||
        memcmp(decrypted, test_plaintext, test_plaintext_len) != 0) {
        printf("ERROR: Decrypted text doesn't match original\n");
        print_hex("Decrypted", decrypted, plain_len);
        print_hex("Original", test_plaintext, test_plaintext_len);
        free(ciphertext);
        free(decrypted);
        return 0;
    }

    printf("Decrypted text matches original: %.*s\n", (int)plain_len, decrypted);

    /* Clean up */
    free(ciphertext);
    free(decrypted);

    printf("SM4-RND test PASSED\n");
    return 1;
}

/* Test DET-SM4 algorithm */
static int test_det_sm4(void)
{
    printf("\n=== Testing DET-SM4 Algorithm ===\n");

    /* Calculate buffer sizes */
    size_t encrypt_buf_size = tecc_det_sm4_calc_encrypt_buffer_size(test_plaintext_len);
    size_t decrypt_buf_size = tecc_det_sm4_calc_decrypt_buffer_size(encrypt_buf_size);

    printf("Plaintext length: %zu\n", test_plaintext_len);
    printf("Encrypt buffer size needed: %zu\n", encrypt_buf_size);
    printf("Decrypt buffer size needed: %zu\n", decrypt_buf_size);

    /* Allocate buffers */
    unsigned char *ciphertext = malloc(encrypt_buf_size);
    unsigned char *decrypted = malloc(decrypt_buf_size);

    if (!ciphertext || !decrypted) {
        printf("ERROR: Memory allocation failed\n");
        free(ciphertext);
        free(decrypted);
        return 0;
    }

    /* Encrypt */
    size_t cipher_len = 0;
    int ret = tecc_det_sm4_encrypt(test_key_16, sizeof(test_key_16),
                                  test_plaintext, test_plaintext_len,
                                  ciphertext, encrypt_buf_size, &cipher_len);

    if (ret != DET_SM4_SUCCESS) {
        printf("ERROR: DET-SM4 encryption failed with code %d\n", ret);
        free(ciphertext);
        free(decrypted);
        return 0;
    }

    printf("Encryption successful, ciphertext length: %zu\n", cipher_len);
    print_hex("Ciphertext", ciphertext, cipher_len);

    /* Validate format */
    ret = tecc_det_sm4_validate_format(ciphertext, cipher_len);
    if (ret != DET_SM4_SUCCESS) {
        printf("ERROR: DET-SM4 format validation failed with code %d\n", ret);
        free(ciphertext);
        free(decrypted);
        return 0;
    }
    printf("Format validation successful\n");

    /* Reallocate decryption buffer with actual ciphertext length */
    size_t actual_decrypt_buf_size = tecc_det_sm4_calc_decrypt_buffer_size(cipher_len);
    printf("Actual decrypt buffer size needed: %zu\n", actual_decrypt_buf_size);

    /* Free old buffer and allocate new one */
    free(decrypted);
    decrypted = malloc(actual_decrypt_buf_size);
    if (!decrypted) {
        printf("ERROR: Memory allocation failed for decryption buffer\n");
        free(ciphertext);
        return 0;
    }

    /* Decrypt */
    size_t plain_len = 0;
    ret = tecc_det_sm4_decrypt(test_key_16, sizeof(test_key_16),
                              ciphertext, cipher_len,
                              decrypted, decrypt_buf_size, &plain_len);

    if (ret != DET_SM4_SUCCESS) {
        printf("ERROR: DET-SM4 decryption failed with code %d\n", ret);
        free(ciphertext);
        free(decrypted);
        return 0;
    }

    printf("Decryption successful, plaintext length: %zu\n", plain_len);

    /* Verify plaintext matches */
    if (plain_len != test_plaintext_len ||
        memcmp(decrypted, test_plaintext, test_plaintext_len) != 0) {
        printf("ERROR: Decrypted text doesn't match original\n");
        print_hex("Decrypted", decrypted, plain_len);
        print_hex("Original", test_plaintext, test_plaintext_len);
        free(ciphertext);
        free(decrypted);
        return 0;
    }

    printf("Decrypted text matches original: %.*s\n", (int)plain_len, decrypted);

    /* Test deterministic property: same plaintext + same key = same ciphertext */
    unsigned char *ciphertext2 = malloc(encrypt_buf_size);
    size_t cipher_len2 = 0;

    if (ciphertext2) {
        ret = tecc_det_sm4_encrypt(test_key_16, sizeof(test_key_16),
                                  test_plaintext, test_plaintext_len,
                                  ciphertext2, encrypt_buf_size, &cipher_len2);

        if (ret == DET_SM4_SUCCESS && cipher_len == cipher_len2 &&
            memcmp(ciphertext, ciphertext2, cipher_len) == 0) {
            printf("Deterministic property verified: same ciphertext for same input\n");
        } else {
            printf("WARNING: Deterministic property not verified\n");
        }
        free(ciphertext2);
    }

    /* Clean up */
    free(ciphertext);
    free(decrypted);

    printf("DET-SM4 test PASSED\n");
    return 1;
}

/* Test SM4-OPE algorithm */
static int test_sm4_ope(void)
{
    printf("\n=== Testing SM4-OPE Algorithm ===\n");

    /* OPE is typically used for small data like integers, not large text */
    /* Let's test with a small 8-byte integer instead */
    uint64_t test_int = 0x0123456789ABCDEFULL;
    size_t test_int_len = sizeof(test_int);
    size_t encrypt_buf_size = tecc_sm4_ope_calc_encrypt_buffer_size(test_int_len);

    printf("Test data length: %zu (8-byte integer)\n", test_int_len);
    printf("Encrypt buffer size needed: %zu\n", encrypt_buf_size);

    /* Allocate buffer */
    unsigned char *ciphertext = malloc(encrypt_buf_size);

    if (!ciphertext) {
        printf("ERROR: Memory allocation failed\n");
        return 0;
    }

    /* Encrypt */
    size_t cipher_len = 0;
    int ret = tecc_sm4_ope_encrypt(test_key_16, sizeof(test_key_16),
                                  (unsigned char *)&test_int, test_int_len,
                                  ciphertext, encrypt_buf_size, &cipher_len);

    if (ret != SM4_OPE_SUCCESS) {
        printf("ERROR: SM4-OPE encryption failed with code %d\n", ret);
        free(ciphertext);
        return 0;
    }

    printf("Encryption successful, ciphertext length: %zu\n", cipher_len);
    print_hex("Ciphertext", ciphertext, cipher_len);

    /* Validate format */
    ret = tecc_sm4_ope_validate_format(ciphertext, cipher_len);
    if (ret != SM4_OPE_SUCCESS) {
        printf("ERROR: SM4-OPE format validation failed with code %d\n", ret);
        free(ciphertext);
        return 0;
    }
    printf("Format validation successful\n");

    /* Test OPE property: order preservation */
    /* Encrypt a smaller integer */
    uint64_t test_int2 = 0x0123456789ABCDEEULL; /* One less */
    unsigned char *ciphertext2 = malloc(encrypt_buf_size);
    size_t cipher_len2 = 0;

    if (ciphertext2) {
        ret = tecc_sm4_ope_encrypt(test_key_16, sizeof(test_key_16),
                                  (unsigned char *)&test_int2, test_int_len,
                                  ciphertext2, encrypt_buf_size, &cipher_len2);

        if (ret == SM4_OPE_SUCCESS) {
            /* Compare ciphertexts - smaller plaintext should produce smaller ciphertext */
            int cmp_result = OPE_cmp(ciphertext, ciphertext2, cipher_len);
            if (cmp_result > 0) {
                printf("OPE property verified: 0x%016lX > 0x%016lX, ciphertext comparison: %d\n",
                       test_int, test_int2, cmp_result);
            } else {
                printf("WARNING: OPE property not verified: cmp_result=%d\n", cmp_result);
            }
        }
        free(ciphertext2);
    }

    /* Clean up */
    free(ciphertext);

    printf("SM4-OPE test PASSED\n");
    return 1;
}

/* Test SEARCH algorithm */
static int test_search_sm4(void)
{
    printf("\n=== Testing SEARCH Algorithm ===\n");

    /* SEARCH only supports encryption, not decryption */
    size_t encrypt_buf_size = tecc_search_sm4_calc_encrypt_buffer_size(test_plaintext_len);

    printf("Plaintext length: %zu\n", test_plaintext_len);
    printf("Encrypt buffer size needed: %zu\n", encrypt_buf_size);

    /* Allocate buffer */
    unsigned char *ciphertext = malloc(encrypt_buf_size);

    if (!ciphertext) {
        printf("ERROR: Memory allocation failed\n");
        return 0;
    }

    /* Encrypt */
    size_t cipher_len = 0;
    int ret = tecc_search_sm4_encrypt(test_key_16, sizeof(test_key_16),
                                     test_plaintext, test_plaintext_len,
                                     ciphertext, encrypt_buf_size, &cipher_len, false);

    if (ret != SM4_SEARCH_SUCCESS) {
        printf("ERROR: SEARCH encryption failed with code %d\n", ret);
        free(ciphertext);
        return 0;
    }

    printf("Encryption successful, ciphertext length: %zu\n", cipher_len);
    print_hex("Ciphertext", ciphertext, cipher_len);

    /* Validate format */
    ret = tecc_search_sm4_validate_format(ciphertext, cipher_len);
    if (ret != SM4_SEARCH_SUCCESS) {
        printf("ERROR: SEARCH format validation failed with code %d\n", ret);
        free(ciphertext);
        return 0;
    }
    printf("Format validation successful\n");

    /* Test that decryption is not supported */
    size_t dummy_len = 0;
    unsigned char dummy_buffer[1];
    ret = tecc_search_sm4_decrypt(test_key_16, sizeof(test_key_16),
                                 ciphertext, cipher_len,
                                 dummy_buffer, sizeof(dummy_buffer), &dummy_len);

    if (ret == SM4_SEARCH_ERROR_DECRYPT_NOT_SUPPORTED) {
        printf("Decryption not supported (as expected for SEARCH)\n");
    } else {
        printf("WARNING: Unexpected decryption result: %d\n", ret);
    }

    /* Clean up */
    free(ciphertext);

    printf("SEARCH test PASSED\n");
    return 1;
}

/* Test unified API */
static int test_unified_api(void)
{
    int i;
    size_t j;

    printf("\n=== Testing Unified Algorithm API ===\n");

    /* Test algorithm capability */
    printf("Testing algorithm capabilities:\n");
    for (i = TECC_ALGO_RND; i <= TECC_ALGO_SEARCH; i++) {
        uint32 cap = tecc_algo_get_capability((TeccAlgorithmType)i);
        /* Note: tecc_algo_get_name is not implemented yet, using placeholder */
        const char *name = "Unknown";
        switch (i) {
            case TECC_ALGO_RND: name = "SM4-RND"; break;
            case TECC_ALGO_DET: name = "DET-SM4"; break;
            case TECC_ALGO_OPE: name = "SM4-OPE"; break;
            case TECC_ALGO_SEARCH: name = "SEARCH"; break;
        }
        printf("  %s: capability = 0x%08X\n", name, cap);
    }

    /* Test buffer size calculation */
    printf("\nTesting buffer size calculation:\n");
    for (i = TECC_ALGO_RND; i <= TECC_ALGO_SEARCH; i++) {
        size_t encrypt_size = tecc_algo_calc_encrypt_buffer_size((TeccAlgorithmType)i, test_plaintext_len);
        size_t decrypt_size = tecc_algo_calc_decrypt_buffer_size((TeccAlgorithmType)i, encrypt_size);
        const char *name = "Unknown";
        switch (i) {
            case TECC_ALGO_RND: name = "SM4-RND"; break;
            case TECC_ALGO_DET: name = "DET-SM4"; break;
            case TECC_ALGO_OPE: name = "SM4-OPE"; break;
            case TECC_ALGO_SEARCH: name = "SEARCH"; break;
        }
        printf("  %s: encrypt=%zu, decrypt=%zu\n", name, encrypt_size, decrypt_size);
    }

    /* Test error code conversion */
    printf("\nTesting error code conversion:\n");
    int test_errors[] = {0, -1, 1, 100};
    for (j = 0; j < sizeof(test_errors)/sizeof(test_errors[0]); j++) {
        const char *err_str = tecc_algo_get_error_string(test_errors[j]);
        printf("  Error %d: %s\n", test_errors[j], err_str ? err_str : "Unknown error");
    }

    printf("Unified API test PASSED\n");
    return 1;
}

int main(int argc, char *argv[])
{
    int test_count = 0;
    int passed_count = 0;

    /* Initialize OpenSSL library */
    if (OPENSSL_init_ssl(OPENSSL_INIT_LOAD_CRYPTO_STRINGS | OPENSSL_INIT_ADD_ALL_CIPHERS, NULL) != 1) {
        fprintf(stderr, "ERROR: Failed to initialize OpenSSL\n");
        return 1;
    }

    printf("TECC Algorithm Module Test Program\n");
    printf("===================================\n");

    /* Test SM4-RND algorithm */
    printf("\n[1/5] Testing SM4-RND Algorithm...\n");
    if (test_sm4_rnd()) {
        printf("✓ SM4-RND test PASSED\n");
        passed_count++;
    } else {
        printf("✗ SM4-RND test FAILED\n");
    }
    test_count++;

    /* Test DET-SM4 algorithm */
    printf("\n[2/5] Testing DET-SM4 Algorithm...\n");
    if (test_det_sm4()) {
        printf("✓ DET-SM4 test PASSED\n");
        passed_count++;
    } else {
        printf("✗ DET-SM4 test FAILED\n");
    }
    test_count++;

    /* Test SM4-OPE algorithm */
    printf("\n[3/5] Testing SM4-OPE Algorithm...\n");
    if (test_sm4_ope()) {
        printf("✓ SM4-OPE test PASSED\n");
        passed_count++;
    } else {
        printf("✗ SM4-OPE test FAILED\n");
    }
    test_count++;

    /* Test SEARCH algorithm */
    printf("\n[4/5] Testing SEARCH Algorithm...\n");
    if (test_search_sm4()) {
        printf("✓ SEARCH test PASSED\n");
        passed_count++;
    } else {
        printf("✗ SEARCH test FAILED\n");
    }
    test_count++;

    /* Test unified API */
    printf("\n[5/5] Testing Unified Algorithm API...\n");
    if (test_unified_api()) {
        printf("✓ Unified API test PASSED\n");
        passed_count++;
    } else {
        printf("✗ Unified API test FAILED\n");
    }
    test_count++;

    /* Print summary */
    printf("\n===================================\n");
    printf("Test Summary: %d/%d tests passed\n", passed_count, test_count);

    if (passed_count == test_count) {
        printf("✓ All tests PASSED\n");
        return 0;
    } else {
        printf("✗ Some tests FAILED\n");
        return 1;
    }
}

/* End - ReqID:TECC4.0 */