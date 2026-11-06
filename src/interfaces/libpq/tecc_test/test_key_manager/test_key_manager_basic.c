/* Begin - ReqID:TECC4.0 */
/*
 * File: test_key_manager_basic.c
 *
 * Abstract:
 *   Basic tests for TECC key management module.
 *   Tests core functionality of high-level API.
 *
 * Authored by <wanghaoyan@highgo.com>, 20260413.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   test_key_manager_basic.c
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

/* Mock cache function */
/* TODO: Replace with new cache interface tecc_cache_get_dek_by_name() */
/* Note: Old interface tecc_metadata_cache_get_column_key() removed, use tecc_cache_get_dek_by_name() */
{
    static TeccColumnKeyMetadata mock_cache = {
        .key_name = "test_dek_001",
        .kek_id = 10001,
        .encrypted_key = "a1b2c3d4e5f67890a1b2c3d4e5f67890",  /* 32 hex chars = 16 bytes */
        .supports_det = true,
        .supports_ope = false,
        .supports_search = true,
        .status = "active"
    };

    if (strcmp(key_name, "test_dek_001") == 0)
        return &mock_cache;
    return NULL;
} */

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
void test_tecc_key_get_kek(void);
void test_tecc_key_get_dek(void);
void test_tecc_key_decrypt_dek(void);
void test_tecc_key_get_decrypted_dek(void);
void test_tecc_key_get_kek_for_dek(void);
void test_tecc_key_validate_dek(void);
void test_tecc_key_validate_kek(void);
void test_api_version(void);
void test_error_handling(void);

int main()
{
    printf("TECC Key Management Module Basic Tests\n");
    printf("======================================\n\n");

    /* Run all tests */
    test_tecc_key_get_kek();
    test_tecc_key_get_dek();
    test_tecc_key_decrypt_dek();
    test_tecc_key_get_decrypted_dek();
    test_tecc_key_get_kek_for_dek();
    test_tecc_key_validate_dek();
    test_tecc_key_validate_kek();
    test_api_version();
    test_error_handling();

    printf("\n======================================\n");
    printf("All basic tests completed.\n");

    return 0;
}

void test_tecc_key_get_kek(void)
{
    printf("=== Testing tecc_key_get_kek_with_state() ===\n");

    size_t kek_len = 0;
    unsigned char *kek = tecc_key_get_kek_with_state(NULL, "test_kek", &kek_len);

    if (kek == NULL)
    {
        printf("FAIL: tecc_key_get_kek_with_state returned NULL\n");
        return;
    }

    if (kek_len != TECC_FIXED_KEK_LENGTH)
    {
        printf("FAIL: Expected KEK length %d, got %zu\n", TECC_FIXED_KEK_LENGTH, kek_len);
        tecc_key_free_key_data(kek, kek_len);
        return;
    }

    /* Verify fixed KEK data */
    bool data_match = true;
    for (size_t i = 0; i < kek_len; i++)
    {
        if (kek[i] != TECC_FIXED_KEK_DATA[i])
        {
            data_match = false;
            break;
        }
    }

    if (!data_match)
    {
        printf("FAIL: KEK data doesn't match fixed KEK\n");
    }
    else
    {
        printf("SUCCESS: tecc_key_get_kek_with_state() works correctly\n");
        printf("  KEK length: %zu bytes\n", kek_len);
        printf("  KEK data matches fixed KEK\n");
    }

    tecc_key_free_key_data(kek, kek_len);
}

void test_tecc_key_get_dek(void)
{
    printf("\n=== Testing tecc_key_get_dek() ===\n");

    size_t dek_len = 0;
    unsigned char *dek = tecc_key_get_dek("test_dek_001", &dek_len);

    if (dek == NULL)
    {
        printf("FAIL: tecc_key_get_dek returned NULL\n");
        printf("  Error: %s\n", tecc_key_get_last_error());
        return;
    }

    printf("SUCCESS: tecc_key_get_dek() works correctly\n");
    printf("  DEK length: %zu bytes\n", dek_len);
    printf("  DEK data retrieved from cache\n");

    tecc_key_free_key_data(dek, dek_len);
}

void test_tecc_key_decrypt_dek(void)
{
    printf("\n=== Testing tecc_key_decrypt_dek() ===\n");

    /* Create test data */
    unsigned char test_encrypted_dek[16] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
        0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10
    };

    /* Get KEK for decryption */
    size_t kek_len = 0;
    unsigned char *kek = tecc_key_get_kek_with_state(NULL, "test_kek", &kek_len);
    if (kek == NULL)
    {
        printf("FAIL: Cannot get KEK for decryption test\n");
        return;
    }

    /* Decrypt DEK */
    unsigned char *decrypted_dek = NULL;
    size_t decrypted_len = 0;
    int ret = tecc_key_decrypt_dek(test_encrypted_dek, sizeof(test_encrypted_dek),
                                  kek, kek_len,
                                  &decrypted_dek, &decrypted_len);

    if (ret != 0)
    {
        printf("FAIL: tecc_key_decrypt_dek returned error %d\n", ret);
        printf("  Error: %s\n", tecc_key_get_last_error());
    }
    else if (decrypted_dek == NULL)
    {
        printf("FAIL: tecc_key_decrypt_dek returned NULL decrypted data\n");
    }
    else if (decrypted_len != sizeof(test_encrypted_dek))
    {
        printf("FAIL: Expected decrypted length %zu, got %zu\n",
               sizeof(test_encrypted_dek), decrypted_len);
    }
    else
    {
        printf("SUCCESS: tecc_key_decrypt_dek() works correctly\n");
        printf("  Decrypted length: %zu bytes\n", decrypted_len);
        printf("  Decryption successful\n");
    }

    /* Cleanup */
    if (decrypted_dek != NULL)
        tecc_key_free_key_data(decrypted_dek, decrypted_len);
    tecc_key_free_key_data(kek, kek_len);
}

void test_tecc_key_get_decrypted_dek(void)
{
    printf("\n=== Testing tecc_key_get_decrypted_dek() ===\n");

    size_t dek_len = 0;
    unsigned char *dek = tecc_key_get_decrypted_dek("test_dek_001", &dek_len);

    if (dek == NULL)
    {
        printf("FAIL: tecc_key_get_decrypted_dek returned NULL\n");
        printf("  Error: %s\n", tecc_key_get_last_error());
        return;
    }

    printf("SUCCESS: tecc_key_get_decrypted_dek() works correctly\n");
    printf("  Decrypted DEK length: %zu bytes\n", dek_len);
    printf("  Combined get+decrypt operation successful\n");

    tecc_key_free_key_data(dek, dek_len);
}

void test_tecc_key_get_kek_for_dek(void)
{
    printf("\n=== Testing tecc_key_get_kek_for_dek() ===\n");

    size_t kek_len = 0;
    unsigned char *kek = tecc_key_get_kek_for_dek(NULL, "test_dek_001", &kek_len);

    if (kek == NULL)
    {
        printf("FAIL: tecc_key_get_kek_for_dek returned NULL\n");
        printf("  Error: %s\n", tecc_key_get_last_error());
        return;
    }

    if (kek_len != TECC_FIXED_KEK_LENGTH)
    {
        printf("FAIL: Expected KEK length %d, got %zu\n", TECC_FIXED_KEK_LENGTH, kek_len);
        tecc_key_free_key_data(kek, kek_len);
        return;
    }

    printf("SUCCESS: tecc_key_get_kek_for_dek() works correctly\n");
    printf("  KEK length: %zu bytes\n", kek_len);
    printf("  Got KEK for DEK successfully\n");

    tecc_key_free_key_data(kek, kek_len);
}

void test_tecc_key_validate_dek(void)
{
    printf("\n=== Testing tecc_key_validate_dek() ===\n");

    bool valid = tecc_key_validate_dek("test_dek_001");
    if (!valid)
    {
        printf("FAIL: tecc_key_validate_dek returned false for valid DEK\n");
        printf("  Error: %s\n", tecc_key_get_last_error());
        return;
    }

    bool invalid = tecc_key_validate_dek("non_existent_dek");
    if (invalid)
    {
        printf("FAIL: tecc_key_validate_dek returned true for non-existent DEK\n");
        return;
    }

    printf("SUCCESS: tecc_key_validate_dek() works correctly\n");
    printf("  Valid DEK: true\n");
    printf("  Invalid DEK: false\n");
}

void test_tecc_key_validate_kek(void)
{
    printf("\n=== Testing tecc_key_validate_kek() ===\n");

    bool valid = tecc_key_validate_kek("test_kek");
    if (!valid)
    {
        printf("FAIL: tecc_key_validate_kek returned false\n");
        return;
    }

    printf("SUCCESS: tecc_key_validate_kek() works correctly\n");
    printf("  KEK validation: true (always true for fixed KEK)\n");
}

void test_api_version(void)
{
    printf("\n=== Testing API Version Functions ===\n");

    /* Test version checking */
    int ret = tecc_key_check_api_version(1, 0);
    if (ret != 0)
    {
        printf("FAIL: tecc_key_check_api_version(1, 0) returned %d\n", ret);
        return;
    }

    ret = tecc_key_check_api_version(2, 0);
    if (ret == 0)
    {
        printf("FAIL: tecc_key_check_api_version(2, 0) should fail\n");
        return;
    }

    /* Test API info */
    TeccKeyApiInfo *info = tecc_key_get_api_info();
    if (info == NULL)
    {
        printf("FAIL: tecc_key_get_api_info returned NULL\n");
        return;
    }

    printf("SUCCESS: API version functions work correctly\n");
    printf("  API Version: %d.%d.%d\n",
           info->api_version_major, info->api_version_minor, info->api_version_patch);
    printf("  Module: %s\n", info->module_name);

    /* Free API info */
    if (info->module_name != NULL) free(info->module_name);
    if (info->build_date != NULL) free(info->build_date);
    if (info->git_hash != NULL) free(info->git_hash);
    free(info);
}

void test_error_handling(void)
{
    printf("\n=== Testing Error Handling ===\n");

    /* Clear any previous error */
    tecc_key_clear_last_error();

    /* Trigger an error */
    size_t key_len = 0;
    unsigned char *key = tecc_key_get_dek(NULL, &key_len);

    if (key != NULL)
    {
        printf("FAIL: tecc_key_get_dek(NULL) should return NULL\n");
        tecc_key_free_key_data(key, key_len);
        return;
    }

    const char *error = tecc_key_get_last_error();
    if (error == NULL || strlen(error) == 0)
    {
        printf("FAIL: No error message recorded\n");
        return;
    }

    printf("SUCCESS: Error handling works correctly\n");
    printf("  Error message: %s\n", error);

    /* Test error clearing */
    tecc_key_clear_last_error();
    error = tecc_key_get_last_error();
    if (error != NULL && strlen(error) > 0)
    {
        printf("FAIL: Error not cleared\n");
        return;
    }

    printf("  Error cleared successfully\n");
}