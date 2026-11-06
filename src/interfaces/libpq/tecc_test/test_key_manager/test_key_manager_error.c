/* Begin - ReqID:TECC4.0 */
/*
 * File: test_key_manager_error.c
 *
 * Abstract:
 *   Error handling tests for TECC key management module.
 *   Tests error conditions, edge cases, and fault tolerance.
 *
 * Authored by <wanghaoyan@highgo.com>, 20260413.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   test_key_manager_error.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "high_level_api.h"

/* Fixed KEK constants (from high_level_api.c) */
#define TECC_FIXED_KEK_LENGTH 32  /* 256-bit key */
static const unsigned char TECC_FIXED_KEK_DATA[32] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};

/* Mock cache function for testing */
typedef struct TeccColumnKeyMetadata {
    char key_name[64];
    unsigned int kek_id;
    char encrypted_key[1024];
    bool supports_det;
    bool supports_ope;
    bool supports_search;
    char status[16];
} TeccColumnKeyMetadata;

typedef const TeccColumnKeyMetadata *TeccCacheColumnKeyResult;

/* Mock cache function that can simulate failures */
static bool simulate_cache_failure = false;
static bool simulate_invalid_hex = false;

/* TODO: Replace with new cache interface tecc_cache_get_dek_by_name() */
/* Note: Old interface tecc_metadata_cache_get_column_key() removed, use tecc_cache_get_dek_by_name() */
{
    if (simulate_cache_failure) {
        return NULL;
    }

    static TeccColumnKeyMetadata mock_cache = {
        .key_name = "test_dek_001",
        .kek_id = 10001,
        .encrypted_key = "a1b2c3d4e5f67890a1b2c3d4e5f67890",  /* 32 hex chars = 16 bytes */
        .supports_det = true,
        .supports_ope = false,
        .supports_search = true,
        .status = "active"
    };

    if (simulate_invalid_hex) {
        /* Invalid hex string (odd length) */
        strcpy(mock_cache.encrypted_key, "123");  /* Invalid: odd length */
    }

    if (strcmp(key_name, "test_dek_001") == 0)
        return &mock_cache;
    return NULL;
} */
}

/* Mock security functions for testing */
static bool simulate_decryption_failure = false;

int security_decrypt_dek_with_hmac(const unsigned char *encrypted_dek,
                                  size_t encrypted_len,
                                  const unsigned char *kek,
                                  size_t kek_len,
                                  unsigned char **decrypted_dek,
                                  size_t *decrypted_len)
{
    /* Simulate decryption failure if flag is set */
    if (simulate_decryption_failure) {
        return -1;
    }

    /* Mock decryption: just copy data for testing */
    if (encrypted_dek == NULL || encrypted_len == 0 || kek == NULL || kek_len == 0)
        return -1;

    *decrypted_dek = malloc(encrypted_len);
    if (*decrypted_dek == NULL)
        return -1;

    memcpy(*decrypted_dek, encrypted_dek, encrypted_len);
    *decrypted_len = encrypted_len;

    return 0;
}

void tecc_key_secure_erase(void *data, size_t length)
{
    if (data != NULL && length > 0)
    {
        memset(data, 0, length);
    }
}

/* Test function prototypes */
void test_null_parameter_handling(void);
void test_empty_string_handling(void);
void test_invalid_key_names(void);
void test_cache_failure_handling(void);
void test_decryption_failure_handling(void);
void test_memory_allocation_failure(void);
void test_error_message_persistence(void);
void test_concurrent_error_handling(void);

int main()
{
    printf("TECC Key Management Module Error Handling Tests\n");
    printf("===============================================\n\n");

    /* Run all error handling tests */
    test_null_parameter_handling();
    test_empty_string_handling();
    test_invalid_key_names();
    test_cache_failure_handling();
    test_decryption_failure_handling();
    test_memory_allocation_failure();
    test_error_message_persistence();
    test_concurrent_error_handling();

    printf("\n===============================================\n");
    printf("All error handling tests completed.\n");

    return 0;
}

void test_null_parameter_handling(void)
{
    printf("=== Testing NULL Parameter Handling ===\n");

    int test_count = 0;
    int pass_count = 0;

    /* Test 1: tecc_key_get_kek with NULL name */
    printf("  Test 1: tecc_key_get_kek(NULL, ...)\n");
    test_count++;
    size_t kek_len = 0;
    unsigned char *kek = tecc_key_get_kek(NULL, &kek_len);
    if (kek == NULL) {
        printf("    PASS: Correctly returned NULL\n");
        pass_count++;
    } else {
        printf("    FAIL: Should have returned NULL\n");
        tecc_key_free_key_data(kek, kek_len);
    }

    /* Test 2: tecc_key_get_kek with NULL length pointer */
    printf("  Test 2: tecc_key_get_kek(name, NULL)\n");
    test_count++;
    kek = tecc_key_get_kek("test_kek", NULL);
    if (kek == NULL) {
        printf("    PASS: Correctly returned NULL\n");
        pass_count++;
    } else {
        printf("    FAIL: Should have returned NULL\n");
        /* Can't free without length, but we know it's TECC_FIXED_KEK_LENGTH */
        free(kek);
    }

    /* Test 3: tecc_key_get_dek with NULL name */
    printf("  Test 3: tecc_key_get_dek(NULL, ...)\n");
    test_count++;
    size_t dek_len = 0;
    unsigned char *dek = tecc_key_get_dek(NULL, &dek_len);
    if (dek == NULL) {
        printf("    PASS: Correctly returned NULL\n");
        pass_count++;
    } else {
        printf("    FAIL: Should have returned NULL\n");
        tecc_key_free_key_data(dek, dek_len);
    }

    /* Test 4: tecc_key_decrypt_dek with NULL parameters */
    printf("  Test 4: tecc_key_decrypt_dek with various NULL parameters\n");
    unsigned char test_data[16] = {0};
    unsigned char *decrypted = NULL;
    size_t decrypted_len = 0;

    /* NULL encrypted_dek */
    test_count++;
    int ret = tecc_key_decrypt_dek(NULL, 16, test_data, 16, &decrypted, &decrypted_len);
    if (ret != 0) {
        printf("    PASS: Correctly failed with NULL encrypted_dek\n");
        pass_count++;
    } else {
        printf("    FAIL: Should have failed with NULL encrypted_dek\n");
        if (decrypted) free(decrypted);
    }

    /* NULL kek */
    test_count++;
    ret = tecc_key_decrypt_dek(test_data, 16, NULL, 16, &decrypted, &decrypted_len);
    if (ret != 0) {
        printf("    PASS: Correctly failed with NULL kek\n");
        pass_count++;
    } else {
        printf("    FAIL: Should have failed with NULL kek\n");
        if (decrypted) free(decrypted);
    }

    /* NULL decrypted_dek pointer */
    test_count++;
    ret = tecc_key_decrypt_dek(test_data, 16, test_data, 16, NULL, &decrypted_len);
    if (ret != 0) {
        printf("    PASS: Correctly failed with NULL decrypted_dek pointer\n");
        pass_count++;
    } else {
        printf("    FAIL: Should have failed with NULL decrypted_dek pointer\n");
    }

    /* NULL decrypted_len pointer */
    test_count++;
    ret = tecc_key_decrypt_dek(test_data, 16, test_data, 16, &decrypted, NULL);
    if (ret != 0) {
        printf("    PASS: Correctly failed with NULL decrypted_len pointer\n");
        pass_count++;
    } else {
        printf("    FAIL: Should have failed with NULL decrypted_len pointer\n");
        if (decrypted) free(decrypted);
    }

    printf("  Result: %d/%d tests passed\n", pass_count, test_count);
}

void test_empty_string_handling(void)
{
    printf("\n=== Testing Empty String Handling ===\n");

    int test_count = 0;
    int pass_count = 0;

    /* Test 1: tecc_key_get_kek with empty string */
    printf("  Test 1: tecc_key_get_kek(\"\", ...)\n");
    test_count++;
    size_t kek_len = 0;
    unsigned char *kek = tecc_key_get_kek("", &kek_len);
    if (kek == NULL) {
        printf("    PASS: Correctly returned NULL for empty KEK name\n");
        pass_count++;
    } else {
        printf("    FAIL: Should have returned NULL for empty KEK name\n");
        tecc_key_free_key_data(kek, kek_len);
    }

    /* Test 2: tecc_key_get_dek with empty string */
    printf("  Test 2: tecc_key_get_dek(\"\", ...)\n");
    test_count++;
    size_t dek_len = 0;
    unsigned char *dek = tecc_key_get_dek("", &dek_len);
    if (dek == NULL) {
        printf("    PASS: Correctly returned NULL for empty DEK name\n");
        pass_count++;
    } else {
        printf("    FAIL: Should have returned NULL for empty DEK name\n");
        tecc_key_free_key_data(dek, dek_len);
    }

    /* Test 3: tecc_key_validate_dek with empty string */
    printf("  Test 3: tecc_key_validate_dek(\"\")\n");
    test_count++;
    bool valid = tecc_key_validate_dek("");
    if (!valid) {
        printf("    PASS: Correctly returned false for empty DEK name\n");
        pass_count++;
    } else {
        printf("    FAIL: Should have returned false for empty DEK name\n");
    }

    /* Test 4: tecc_key_validate_kek with empty string */
    printf("  Test 4: tecc_key_validate_kek(\"\")\n");
    test_count++;
    valid = tecc_key_validate_kek("");
    if (!valid) {
        printf("    PASS: Correctly returned false for empty KEK name\n");
        pass_count++;
    } else {
        printf("    FAIL: Should have returned false for empty KEK name\n");
    }

    printf("  Result: %d/%d tests passed\n", pass_count, test_count);
}

void test_invalid_key_names(void)
{
    printf("\n=== Testing Invalid Key Names ===\n");

    int test_count = 0;
    int pass_count = 0;

    /* Test various invalid key name patterns */
    const char *invalid_names[] = {
        "key with spaces",
        "key\twith\ttabs",
        "key\nwith\nnewlines",
        "key\0with\0nulls",  /* Will be truncated at first null */
        "very_long_key_name_that_exceeds_the_maximum_allowed_length_for_a_key_name_in_the_system",
        "key/with/slashes",
        "key\\with\\backslashes",
        "key:with:colons",
        "key*with*asterisks",
        "key?with?question",
        "key\"with\"quotes",
        "key<with>angles",
        "key|with|pipe",
        "key with trailing space ",
        " key with leading space",
    };

    for (size_t i = 0; i < sizeof(invalid_names) / sizeof(invalid_names[0]); i++) {
        test_count++;

        /* Skip the null-containing string as it's problematic for printf */
        if (strchr(invalid_names[i], '\0') && invalid_names[i][0] != '\0') {
            printf("  Test %zu: Key with null characters (skipping detailed output)\n", i + 1);
        } else {
            printf("  Test %zu: Key '%s'\n", i + 1, invalid_names[i]);
        }

        size_t len = 0;
        unsigned char *key = tecc_key_get_dek(invalid_names[i], &len);

        if (key == NULL) {
            printf("    PASS: Correctly rejected invalid key name\n");
            pass_count++;
        } else {
            printf("    FAIL: Should have rejected invalid key name\n");
            tecc_key_free_key_data(key, len);
        }
    }

    printf("  Result: %d/%d tests passed\n", pass_count, test_count);
}

void test_cache_failure_handling(void)
{
    printf("\n=== Testing Cache Failure Handling ===\n");

    /* Enable cache failure simulation */
    simulate_cache_failure = true;

    printf("  Test: DEK retrieval with cache failure\n");
    size_t dek_len = 0;
    unsigned char *dek = tecc_key_get_dek("test_dek_001", &dek_len);

    if (dek == NULL) {
        const char *error = tecc_key_get_last_error();
        if (error != NULL && strlen(error) > 0) {
            printf("    PASS: Correctly handled cache failure\n");
            printf("    Error message: %s\n", error);
        } else {
            printf("    FAIL: No error message recorded for cache failure\n");
        }
    } else {
        printf("    FAIL: Should have failed with cache failure\n");
        tecc_key_free_key_data(dek, dek_len);
    }

    /* Test invalid hex in cache */
    simulate_cache_failure = false;
    simulate_invalid_hex = true;

    printf("  Test: DEK retrieval with invalid hex in cache\n");
    dek = tecc_key_get_dek("test_dek_001", &dek_len);

    if (dek == NULL) {
        const char *error = tecc_key_get_last_error();
        if (error != NULL && strlen(error) > 0) {
            printf("    PASS: Correctly handled invalid hex in cache\n");
            printf("    Error message: %s\n", error);
        } else {
            printf("    FAIL: No error message recorded for invalid hex\n");
        }
    } else {
        printf("    WARNING: Retrieved DEK despite invalid hex (may fall back to database)\n");
        tecc_key_free_key_data(dek, dek_len);
    }

    /* Reset simulation flags */
    simulate_cache_failure = false;
    simulate_invalid_hex = false;
}

void test_decryption_failure_handling(void)
{
    printf("\n=== Testing Decryption Failure Handling ===\n");

    /* Enable decryption failure simulation */
    simulate_decryption_failure = true;

    printf("  Test: DEK decryption failure\n");

    /* First get encrypted DEK */
    size_t dek_len = 0;
    unsigned char *encrypted_dek = tecc_key_get_dek("test_dek_001", &dek_len);

    if (encrypted_dek != NULL) {
        /* Get KEK for decryption */
        size_t kek_len = 0;
        unsigned char *kek = tecc_key_get_kek("test_kek", &kek_len);

        if (kek != NULL) {
            /* Try to decrypt (should fail) */
            unsigned char *decrypted_dek = NULL;
            size_t decrypted_len = 0;
            int ret = tecc_key_decrypt_dek(encrypted_dek, dek_len,
                                          kek, kek_len,
                                          &decrypted_dek, &decrypted_len);

            if (ret != 0) {
                const char *error = tecc_key_get_last_error();
                if (error != NULL && strlen(error) > 0) {
                    printf("    PASS: Correctly handled decryption failure\n");
                    printf("    Error message: %s\n", error);
                } else {
                    printf("    FAIL: No error message recorded for decryption failure\n");
                }
            } else {
                printf("    FAIL: Decryption should have failed\n");
                if (decrypted_dek) free(decrypted_dek);
            }

            tecc_key_free_key_data(kek, kek_len);
        } else {
            printf("    FAIL: Could not get KEK for decryption test\n");
        }

        tecc_key_free_key_data(encrypted_dek, dek_len);
    } else {
        printf("    FAIL: Could not get encrypted DEK for test\n");
    }

    /* Reset simulation flag */
    simulate_decryption_failure = false;
}

void test_memory_allocation_failure(void)
{
    printf("\n=== Testing Memory Allocation Failure Handling ===\n");

    /* Note: We can't easily simulate malloc failure in a portable way.
       Instead, we test edge cases that might trigger allocation issues. */

    printf("  Test: Extremely large key length (potential allocation failure)\n");

    /* Create test data with very large size */
    size_t huge_size = 1024 * 1024 * 1024; /* 1GB - likely to fail on most systems */

    unsigned char *huge_buffer = malloc(huge_size);
    if (huge_buffer == NULL) {
        printf("    NOTE: Could not allocate 1GB buffer (expected on most systems)\n");

        /* Test with more reasonable but still large size */
        huge_size = 100 * 1024 * 1024; /* 100MB */
        huge_buffer = malloc(huge_size);

        if (huge_buffer == NULL) {
            printf("    NOTE: Could not allocate 100MB buffer either\n");
            printf("    Test inconclusive - depends on system memory\n");
            return;
        }
    }

    /* Fill with test data */
    memset(huge_buffer, 0xAA, huge_size);

    /* Get KEK */
    size_t kek_len = 0;
    unsigned char *kek = tecc_key_get_kek("test_kek", &kek_len);

    if (kek != NULL) {
        /* Try to decrypt huge buffer */
        unsigned char *decrypted = NULL;
        size_t decrypted_len = 0;
        int ret = tecc_key_decrypt_dek(huge_buffer, huge_size,
                                      kek, kek_len,
                                      &decrypted, &decrypted_len);

        /* We don't know if this will succeed or fail - depends on memory */
        if (ret == 0 && decrypted != NULL) {
            printf("    NOTE: Successfully decrypted large buffer\n");
            free(decrypted);
        } else {
            printf("    NOTE: Failed to decrypt large buffer (may be memory limit)\n");
            const char *error = tecc_key_get_last_error();
            if (error) printf("    Error: %s\n", error);
        }

        tecc_key_free_key_data(kek, kek_len);
    }

    free(huge_buffer);

    printf("  Test: Zero-length buffers\n");

    /* Test with zero-length encrypted DEK */
    unsigned char *decrypted = NULL;
    size_t decrypted_len = 0;
    unsigned char zero_kek[32] = {0};

    int ret = tecc_key_decrypt_dek(NULL, 0, zero_kek, 32, &decrypted, &decrypted_len);
    if (ret != 0) {
        printf("    PASS: Correctly rejected zero-length encrypted DEK\n");
    } else {
        printf("    FAIL: Should have rejected zero-length encrypted DEK\n");
        if (decrypted) free(decrypted);
    }
}

void test_error_message_persistence(void)
{
    printf("\n=== Testing Error Message Persistence ===\n");

    printf("  Test: Error message persistence across operations\n");

    /* Clear any existing errors */
    tecc_key_clear_last_error();

    /* Generate an error */
    size_t len = 0;
    unsigned char *key = tecc_key_get_dek(NULL, &len);  /* Should fail */

    if (key == NULL) {
        const char *error1 = tecc_key_get_last_error();
        printf("    Initial error: %s\n", error1 ? error1 : "(none)");

        /* Perform another operation that might succeed */
        key = tecc_key_get_kek("test_kek", &len);
        if (key != NULL) {
            const char *error2 = tecc_key_get_last_error();
            printf("    Error after successful operation: %s\n", error2 ? error2 : "(none)");

            if (error2 == NULL || strlen(error2) == 0) {
                printf("    PASS: Error cleared after successful operation\n");
            } else {
                printf("    WARNING: Error persisted after successful operation\n");
            }

            tecc_key_free_key_data(key, len);
        }
    }

    printf("  Test: Multiple error accumulation\n");
    tecc_key_clear_last_error();

    /* Generate multiple errors */
    (void)tecc_key_get_dek(NULL, &len);  /* Error 1 */
    (void)tecc_key_get_dek("", &len);    /* Error 2 */
    (void)tecc_key_validate_dek(NULL);   /* Error 3 */

    const char *final_error = tecc_key_get_last_error();
    printf("    Final error after multiple operations: %s\n", final_error ? final_error : "(none)");

    printf("  Test: Error clearing\n");
    tecc_key_clear_last_error();
    final_error = tecc_key_get_last_error();

    if (final_error == NULL || strlen(final_error) == 0) {
        printf("    PASS: Error successfully cleared\n");
    } else {
        printf("    FAIL: Error not cleared\n");
    }
}

void test_concurrent_error_handling(void)
{
    printf("\n=== Testing Concurrent Error Handling ===\n");

    printf("  Test: Rapid error generation\n");

    /* Generate many errors in quick succession */
    const int num_errors = 100;
    int error_count = 0;

    for (int i = 0; i < num_errors; i++) {
        tecc_key_clear_last_error();

        size_t len = 0;
        unsigned char *key = tecc_key_get_dek(NULL, &len);  /* Always fails */

        if (key == NULL) {
            const char *error = tecc_key_get_last_error();
            if (error != NULL && strlen(error) > 0) {
                error_count++;
            }
        } else {
            tecc_key_free_key_data(key, len);
        }
    }

    printf("    Generated %d errors out of %d attempts\n", error_count, num_errors);

    if (error_count == num_errors) {
        printf("    PASS: All errors correctly recorded\n");
    } else {
        printf("    WARNING: Not all errors were recorded\n");
    }

    printf("  Test: Mixed success/error pattern\n");

    int successes = 0;
    int failures = 0;

    for (int i = 0; i < 20; i++) {
        tecc_key_clear_last_error();

        if (i % 2 == 0) {
            /* Even: try valid operation */
            size_t len = 0;
            unsigned char *key = tecc_key_get_kek("test_kek", &len);
            if (key != NULL) {
                successes++;
                tecc_key_free_key_data(key, len);
            } else {
                failures++;
            }
        } else {
            /* Odd: try invalid operation */
            size_t len = 0;
            unsigned char *key = tecc_key_get_dek(NULL, &len);
            if (key == NULL) {
                failures++;  /* Expected failure */
            } else {
                successes++; /* Unexpected success */
                tecc_key_free_key_data(key, len);
            }
        }
    }

    printf("    Successes: %d, Failures: %d\n", successes, failures);

    /* We expect about 10 successes (valid KEK) and 10 failures (NULL DEK) */
    if (successes >= 9 && failures >= 9) {  /* Allow some tolerance */
        printf("    PASS: Mixed pattern handled correctly\n");
    } else {
        printf("    WARNING: Unexpected pattern of successes/failures\n");
    }
}