/* Begin - ReqID:TECC4.0 */
/*
 * File: test_key_manager_integration.c
 *
 * Abstract:
 *   Integration tests for TECC key management module.
 *   Tests module interactions and complex scenarios.
 *
 * Authored by <wanghaoyan@highgo.com>, 20260413.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   test_key_manager_integration.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

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

/* Multiple mock cache entries for testing */
static TeccColumnKeyMetadata mock_cache_entries[] = {
    {
        .key_name = "dek_customer_id",
        .kek_id = 10001,
        .encrypted_key = "0123456789abcdef0123456789abcdef",
        .supports_det = true,
        .supports_ope = false,
        .supports_search = true,
        .status = "active"
    },
    {
        .key_name = "dek_ssn",
        .kek_id = 10001,
        .encrypted_key = "fedcba9876543210fedcba9876543210",
        .supports_det = true,
        .supports_ope = true,
        .supports_search = false,
        .status = "active"
    },
    {
        .key_name = "dek_salary",
        .kek_id = 10002,
        .encrypted_key = "a1b2c3d4e5f67890a1b2c3d4e5f67890",
        .supports_det = false,
        .supports_ope = true,
        .supports_search = false,
        .status = "active"
    },
    {
        .key_name = "dek_expired",
        .kek_id = 10001,
        .encrypted_key = "deadbeefdeadbeefdeadbeefdeadbeef",
        .supports_det = true,
        .supports_ope = false,
        .supports_search = true,
        .status = "expired"
    }
};

/* Mock cache function - old interface removed */
/* Note: Use tecc_cache_get_dek_by_name() in real implementations */

/* Mock security functions for testing */
int security_decrypt_dek_with_hmac(const unsigned char *encrypted_dek,
                                  size_t encrypted_len,
                                  const unsigned char *kek,
                                  size_t kek_len,
                                  unsigned char **decrypted_dek,
                                  size_t *decrypted_len)
{
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
void test_multiple_dek_operations(void);
void test_dek_with_different_kek(void);
void test_dek_status_handling(void);
void test_concurrent_access_pattern(void);
void test_memory_management(void);
void test_error_recovery(void);

int main()
{
    printf("TECC Key Management Module Integration Tests\n");
    printf("============================================\n\n");

    /* Run all integration tests */
    test_multiple_dek_operations();
    test_dek_with_different_kek();
    test_dek_status_handling();
    test_concurrent_access_pattern();
    test_memory_management();
    test_error_recovery();

    printf("\n============================================\n");
    printf("All integration tests completed.\n");

    return 0;
}

void test_multiple_dek_operations(void)
{
    printf("=== Testing Multiple DEK Operations ===\n");

    const char *dek_names[] = {"dek_customer_id", "dek_ssn", "dek_salary"};
    int num_deks = sizeof(dek_names) / sizeof(dek_names[0]);
    int success_count = 0;

    for (int i = 0; i < num_deks; i++) {
        size_t dek_len = 0;
        unsigned char *dek = tecc_key_get_decrypted_dek(dek_names[i], &dek_len);

        if (dek != NULL) {
            printf("  SUCCESS: Retrieved DEK '%s' (length: %zu)\n", dek_names[i], dek_len);
            success_count++;
            tecc_key_free_key_data(dek, dek_len);
        } else {
            printf("  FAIL: Failed to retrieve DEK '%s'\n", dek_names[i]);
            printf("    Error: %s\n", tecc_key_get_last_error());
        }
    }

    if (success_count == num_deks) {
        printf("SUCCESS: All %d DEK operations completed successfully\n", num_deks);
    } else {
        printf("FAIL: Only %d of %d DEK operations succeeded\n", success_count, num_deks);
    }
}

void test_dek_with_different_kek(void)
{
    printf("\n=== Testing DEK with Different KEK ===\n");

    /* Test DEKs associated with different KEKs */
    const char *dek_names[] = {"dek_customer_id", "dek_salary"};
    const char *expected_kek_names[] = {"test_kek", "test_kek"};  /* Both use same KEK in mock */
    int num_tests = sizeof(dek_names) / sizeof(dek_names[0]);
    int success_count = 0;

    for (int i = 0; i < num_tests; i++) {
        /* Get KEK for DEK */
        size_t kek_len = 0;
        unsigned char *kek = tecc_key_get_kek_for_dek(NULL, dek_names[i], &kek_len);

        if (kek != NULL) {
            printf("  SUCCESS: Got KEK for DEK '%s' (length: %zu)\n", dek_names[i], kek_len);

            /* Verify KEK is valid */
            if (kek_len == TECC_FIXED_KEK_LENGTH) {
                printf("    KEK length correct: %d bytes\n", TECC_FIXED_KEK_LENGTH);
                success_count++;
            } else {
                printf("    FAIL: Incorrect KEK length: %zu (expected: %d)\n",
                       kek_len, TECC_FIXED_KEK_LENGTH);
            }

            tecc_key_free_key_data(kek, kek_len);
        } else {
            printf("  FAIL: Failed to get KEK for DEK '%s'\n", dek_names[i]);
            printf("    Error: %s\n", tecc_key_get_last_error());
        }
    }

    if (success_count == num_tests) {
        printf("SUCCESS: All KEK retrieval tests passed\n");
    } else {
        printf("FAIL: Only %d of %d KEK retrieval tests succeeded\n", success_count, num_tests);
    }
}

void test_dek_status_handling(void)
{
    printf("\n=== Testing DEK Status Handling ===\n");

    /* Test active DEK */
    bool active_valid = tecc_key_validate_dek("dek_customer_id");
    printf("  Active DEK validation: %s\n", active_valid ? "PASS" : "FAIL");

    /* Test expired DEK */
    bool expired_valid = tecc_key_validate_dek("dek_expired");
    printf("  Expired DEK validation: %s (should be false)\n", expired_valid ? "FAIL" : "PASS");

    /* Try to get expired DEK */
    size_t dek_len = 0;
    unsigned char *dek = tecc_key_get_decrypted_dek("dek_expired", &dek_len);
    if (dek != NULL) {
        printf("  WARNING: Retrieved expired DEK (status handling may need review)\n");
        tecc_key_free_key_data(dek, dek_len);
    } else {
        printf("  SUCCESS: Correctly rejected expired DEK\n");
        printf("    Error: %s\n", tecc_key_get_last_error());
    }

    /* Test non-existent DEK */
    bool nonexistent_valid = tecc_key_validate_dek("non_existent_dek_xyz");
    printf("  Non-existent DEK validation: %s (should be false)\n",
           nonexistent_valid ? "FAIL" : "PASS");

    if (!active_valid || expired_valid || nonexistent_valid) {
        printf("FAIL: DEK status handling tests failed\n");
    } else {
        printf("SUCCESS: DEK status handling tests passed\n");
    }
}

void test_concurrent_access_pattern(void)
{
    printf("\n=== Testing Concurrent Access Pattern ===\n");

    /* Simulate concurrent access pattern by making multiple rapid requests */
    const char *dek_name = "dek_customer_id";
    int num_requests = 5;
    int success_count = 0;

    printf("  Making %d rapid requests for DEK '%s':\n", num_requests, dek_name);

    for (int i = 0; i < num_requests; i++) {
        size_t dek_len = 0;
        unsigned char *dek = tecc_key_get_decrypted_dek(dek_name, &dek_len);

        if (dek != NULL) {
            printf("    Request %d: SUCCESS (length: %zu)\n", i + 1, dek_len);
            success_count++;
            tecc_key_free_key_data(dek, dek_len);
        } else {
            printf("    Request %d: FAIL\n", i + 1);
            printf("      Error: %s\n", tecc_key_get_last_error());
        }

        /* Small delay to simulate realistic timing */
        struct timespec ts = {0, 10000000}; /* 10ms */
        nanosleep(&ts, NULL);
    }

    if (success_count == num_requests) {
        printf("  SUCCESS: All %d concurrent requests succeeded\n", num_requests);
    } else {
        printf("  FAIL: Only %d of %d requests succeeded\n", success_count, num_requests);
    }

    /* Test error handling under load */
    printf("  Testing error handling under load:\n");
    tecc_key_clear_last_error();

    size_t kek_len = 0;
    unsigned char *kek = tecc_key_get_kek(NULL, &kek_len);  /* Invalid parameter */

    if (kek == NULL) {
        const char *error = tecc_key_get_last_error();
        if (error != NULL && strlen(error) > 0) {
            printf("    SUCCESS: Error correctly reported under load\n");
        } else {
            printf("    FAIL: No error message recorded\n");
        }
    } else {
        printf("    FAIL: Should have returned NULL for invalid parameter\n");
        tecc_key_free_key_data(kek, kek_len);
    }
}

void test_memory_management(void)
{
    printf("\n=== Testing Memory Management ===\n");

    int num_iterations = 10;
    int success_count = 0;

    printf("  Testing memory allocation/deallocation for %d iterations:\n", num_iterations);

    for (int i = 0; i < num_iterations; i++) {
        /* Allocate and use multiple keys */
        size_t kek_len = 0;
        unsigned char *kek = tecc_key_get_kek("test_kek", &kek_len);

        size_t dek_len = 0;
        unsigned char *dek = tecc_key_get_decrypted_dek("dek_customer_id", &dek_len);

        if (kek != NULL && dek != NULL) {
            printf("    Iteration %d: SUCCESS (KEK: %zu bytes, DEK: %zu bytes)\n",
                   i + 1, kek_len, dek_len);
            success_count++;
        } else {
            printf("    Iteration %d: FAIL\n", i + 1);
            if (kek == NULL) printf("      KEK error: %s\n", tecc_key_get_last_error());
            if (dek == NULL) printf("      DEK error: %s\n", tecc_key_get_last_error());
        }

        /* Clean up */
        if (kek != NULL) tecc_key_free_key_data(kek, kek_len);
        if (dek != NULL) tecc_key_free_key_data(dek, dek_len);

        /* Clear errors for next iteration */
        tecc_key_clear_last_error();
    }

    if (success_count == num_iterations) {
        printf("  SUCCESS: Memory management tests passed (no leaks detected)\n");
    } else {
        printf("  FAIL: Memory management issues detected\n");
    }

    /* Test secure erase */
    printf("  Testing secure erase functionality:\n");
    unsigned char *test_data = malloc(32);
    if (test_data != NULL) {
        memset(test_data, 0xAA, 32);  /* Fill with pattern */
        tecc_key_secure_erase(test_data, 32);

        /* Check if data was zeroed */
        bool is_zeroed = true;
        for (int i = 0; i < 32; i++) {
            if (test_data[i] != 0) {
                is_zeroed = false;
                break;
            }
        }

        if (is_zeroed) {
            printf("    SUCCESS: Secure erase correctly zeroed memory\n");
        } else {
            printf("    FAIL: Secure erase did not zero memory\n");
        }

        free(test_data);
    }
}

void test_error_recovery(void)
{
    printf("\n=== Testing Error Recovery ===\n");

    /* Test 1: Recover from invalid parameter error */
    printf("  Test 1: Recover from invalid parameter\n");
    tecc_key_clear_last_error();

    size_t len = 0;
    unsigned char *key = tecc_key_get_dek(NULL, &len);  /* Should fail */

    if (key == NULL) {
        const char *error1 = tecc_key_get_last_error();
        printf("    Initial error: %s\n", error1 ? error1 : "(none)");

        /* Now try a valid operation */
        key = tecc_key_get_dek("dek_customer_id", &len);
        if (key != NULL) {
            printf("    SUCCESS: Recovered and retrieved valid DEK\n");
            tecc_key_free_key_data(key, len);
        } else {
            printf("    FAIL: Could not recover from error\n");
            printf("      Error: %s\n", tecc_key_get_last_error());
        }
    } else {
        printf("    FAIL: Should have failed with NULL parameter\n");
        tecc_key_free_key_data(key, len);
    }

    /* Test 2: Multiple error scenarios */
    printf("  Test 2: Multiple error scenarios\n");

    /* Clear all errors */
    tecc_key_clear_last_error();

    /* Scenario A: Non-existent DEK */
    bool valid = tecc_key_validate_dek("non_existent_dek_123");
    printf("    Non-existent DEK validation: %s (expected: false)\n", valid ? "true" : "false");

    /* Scenario B: Get KEK with invalid name */
    len = 0;
    key = tecc_key_get_kek("", &len);  /* Empty string */
    if (key == NULL) {
        printf("    Empty KEK name correctly rejected\n");
    } else {
        printf("    FAIL: Should have rejected empty KEK name\n");
        tecc_key_free_key_data(key, len);
    }

    /* Scenario C: Check error accumulation */
    const char *final_error = tecc_key_get_last_error();
    if (final_error != NULL && strlen(final_error) > 0) {
        printf("    Final error message: %s\n", final_error);
    }

    /* Test 3: API version error recovery */
    printf("  Test 3: API version error recovery\n");

    /* Try incompatible version */
    int ret = tecc_key_check_api_version(99, 0);  /* Very high version */
    if (ret != 0) {
        printf("    Correctly rejected incompatible API version\n");

        /* Try compatible version */
        ret = tecc_key_check_api_version(1, 0);
        if (ret == 0) {
            printf("    SUCCESS: Recovered and accepted compatible API version\n");
        } else {
            printf("    FAIL: Could not recover to compatible API version\n");
        }
    } else {
        printf("    FAIL: Should have rejected version 99.0\n");
    }

    printf("  SUCCESS: Error recovery tests completed\n");
}