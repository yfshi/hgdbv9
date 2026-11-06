/* Begin - ReqID:TECC4.0 */
/*
 * File: test_simple_standalone.c
 *
 * Abstract:
 *   Simple standalone test for TECC key management module.
 *   Tests basic functionality without requiring full module compilation.
 *
 * Authored by <wanghaoyan@highgo.com>, 20260413.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   test_simple_standalone.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Simple test structure */
typedef struct {
    const char *test_name;
    bool (*test_function)(void);
} TestCase;

/* Mock fixed KEK */
#define TECC_FIXED_KEK_LENGTH 32
static const unsigned char TECC_FIXED_KEK_DATA[32] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
    0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F
};

/* Mock cache entry */
typedef struct {
    char key_name[64];
    unsigned int kek_id;
    char encrypted_key[1024];
    bool supports_det;
    bool supports_ope;
    bool supports_search;
    char status[16];
} MockCacheEntry;

static MockCacheEntry mock_cache[] = {
    {
        .key_name = "test_dek_001",
        .kek_id = 10001,
        .encrypted_key = "a1b2c3d4e5f67890a1b2c3d4e5f67890",
        .supports_det = true,
        .supports_ope = false,
        .supports_search = true,
        .status = "active"
    }
};

/* Test counters */
static int tests_passed = 0;
static int tests_failed = 0;

/* Helper functions */
void print_test_result(const char *test_name, bool passed)
{
    if (passed) {
        printf("  ✓ %s\n", test_name);
        tests_passed++;
    } else {
        printf("  ✗ %s\n", test_name);
        tests_failed++;
    }
}

int hex_char_to_int(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

int hex_string_to_binary(const char *hex_str, unsigned char *binary, size_t binary_size)
{
    size_t hex_len;
    size_t i, j;

    if (hex_str == NULL || binary == NULL || binary_size == 0)
        return -1;

    hex_len = strlen(hex_str);
    if (hex_len % 2 != 0)
        return -1;

    if (binary_size < hex_len / 2)
        return -1;

    for (i = 0, j = 0; i < hex_len; i += 2, j++) {
        int high_nibble = hex_char_to_int(hex_str[i]);
        int low_nibble = hex_char_to_int(hex_str[i + 1]);

        if (high_nibble < 0 || low_nibble < 0)
            return -1;

        binary[j] = (high_nibble << 4) | low_nibble;
    }

    return j;
}

/* Mock tecc_key_get_kek function */
unsigned char *mock_tecc_key_get_kek(const char *kek_name, size_t *key_len)
{
    if (kek_name == NULL || key_len == NULL || strlen(kek_name) == 0)
        return NULL;

    unsigned char *kek = malloc(TECC_FIXED_KEK_LENGTH);
    if (kek == NULL)
        return NULL;

    memcpy(kek, TECC_FIXED_KEK_DATA, TECC_FIXED_KEK_LENGTH);
    *key_len = TECC_FIXED_KEK_LENGTH;

    return kek;
}

/* Mock tecc_key_get_dek function */
unsigned char *mock_tecc_key_get_dek(const char *dek_name, size_t *key_len)
{
    if (dek_name == NULL || key_len == NULL)
        return NULL;

    /* Search in mock cache */
    for (size_t i = 0; i < sizeof(mock_cache) / sizeof(mock_cache[0]); i++) {
        if (strcmp(dek_name, mock_cache[i].key_name) == 0) {
            /* Convert hex string to binary */
            size_t hex_len = strlen(mock_cache[i].encrypted_key);
            size_t bin_len = hex_len / 2;
            unsigned char *dek = malloc(bin_len);

            if (dek == NULL)
                return NULL;

            if (hex_string_to_binary(mock_cache[i].encrypted_key, dek, bin_len) != (int)bin_len) {
                free(dek);
                return NULL;
            }

            *key_len = bin_len;
            return dek;
        }
    }

    return NULL;
}

/* Mock tecc_key_decrypt_dek function */
int mock_tecc_key_decrypt_dek(const unsigned char *encrypted_dek,
                             size_t encrypted_len,
                             const unsigned char *kek,
                             size_t kek_len,
                             unsigned char **decrypted_dek,
                             size_t *decrypted_len)
{
    if (encrypted_dek == NULL || encrypted_len == 0 ||
        kek == NULL || kek_len == 0 ||
        decrypted_dek == NULL || decrypted_len == NULL)
        return -1;

    *decrypted_dek = malloc(encrypted_len);
    if (*decrypted_dek == NULL)
        return -1;

    memcpy(*decrypted_dek, encrypted_dek, encrypted_len);
    *decrypted_len = encrypted_len;

    return 0;
}

/* Mock tecc_key_free_key_data function */
void mock_tecc_key_free_key_data(unsigned char *key_data, size_t key_len)
{
    if (key_data != NULL) {
        memset(key_data, 0, key_len);
        free(key_data);
    }
}

/* Test functions */
bool test_kek_retrieval(void)
{
    size_t kek_len = 0;
    unsigned char *kek = mock_tecc_key_get_kek("test_kek", &kek_len);

    bool passed = (kek != NULL) && (kek_len == TECC_FIXED_KEK_LENGTH);

    if (passed) {
        /* Verify KEK data */
        for (size_t i = 0; i < kek_len; i++) {
            if (kek[i] != TECC_FIXED_KEK_DATA[i]) {
                passed = false;
                break;
            }
        }
    }

    if (kek != NULL)
        mock_tecc_key_free_key_data(kek, kek_len);

    return passed;
}

bool test_dek_retrieval(void)
{
    size_t dek_len = 0;
    unsigned char *dek = mock_tecc_key_get_dek("test_dek_001", &dek_len);

    bool passed = (dek != NULL) && (dek_len == 16); /* 32 hex chars = 16 bytes */

    if (dek != NULL)
        mock_tecc_key_free_key_data(dek, dek_len);

    return passed;
}

bool test_dek_decryption(void)
{
    /* First get encrypted DEK */
    size_t encrypted_len = 0;
    unsigned char *encrypted_dek = mock_tecc_key_get_dek("test_dek_001", &encrypted_len);

    if (encrypted_dek == NULL)
        return false;

    /* Get KEK for decryption */
    size_t kek_len = 0;
    unsigned char *kek = mock_tecc_key_get_kek("test_kek", &kek_len);

    if (kek == NULL) {
        mock_tecc_key_free_key_data(encrypted_dek, encrypted_len);
        return false;
    }

    /* Decrypt DEK */
    unsigned char *decrypted_dek = NULL;
    size_t decrypted_len = 0;
    int ret = mock_tecc_key_decrypt_dek(encrypted_dek, encrypted_len,
                                       kek, kek_len,
                                       &decrypted_dek, &decrypted_len);

    bool passed = (ret == 0) && (decrypted_dek != NULL) && (decrypted_len == encrypted_len);

    /* Cleanup */
    if (encrypted_dek != NULL)
        mock_tecc_key_free_key_data(encrypted_dek, encrypted_len);
    if (kek != NULL)
        mock_tecc_key_free_key_data(kek, kek_len);
    if (decrypted_dek != NULL)
        mock_tecc_key_free_key_data(decrypted_dek, decrypted_len);

    return passed;
}

bool test_invalid_parameters(void)
{
    /* Test NULL parameters */
    size_t len = 0;
    unsigned char *key = mock_tecc_key_get_kek(NULL, &len);
    if (key != NULL) {
        mock_tecc_key_free_key_data(key, len);
        return false;
    }

    key = mock_tecc_key_get_dek(NULL, &len);
    if (key != NULL) {
        mock_tecc_key_free_key_data(key, len);
        return false;
    }

    /* Test empty string */
    key = mock_tecc_key_get_kek("", &len);
    if (key != NULL) {
        mock_tecc_key_free_key_data(key, len);
        return false;
    }

    /* Test non-existent key */
    key = mock_tecc_key_get_dek("non_existent_key", &len);
    if (key != NULL) {
        mock_tecc_key_free_key_data(key, len);
        return false;
    }

    return true;
}

bool test_memory_management(void)
{
    /* Test multiple allocations and deallocations */
    bool passed = true;

    for (int i = 0; i < 10; i++) {
        size_t kek_len = 0;
        unsigned char *kek = mock_tecc_key_get_kek("test_kek", &kek_len);

        size_t dek_len = 0;
        unsigned char *dek = mock_tecc_key_get_dek("test_dek_001", &dek_len);

        if (kek == NULL || dek == NULL) {
            passed = false;
        }

        if (kek != NULL)
            mock_tecc_key_free_key_data(kek, kek_len);
        if (dek != NULL)
            mock_tecc_key_free_key_data(dek, dek_len);

        if (!passed)
            break;
    }

    return passed;
}

/* Test suite */
static TestCase test_cases[] = {
    {"KEK retrieval", test_kek_retrieval},
    {"DEK retrieval", test_dek_retrieval},
    {"DEK decryption", test_dek_decryption},
    {"Invalid parameter handling", test_invalid_parameters},
    {"Memory management", test_memory_management},
};

int main()
{
    printf("TECC Key Management Module - Simple Standalone Test\n");
    printf("===================================================\n\n");

    printf("Running %zu tests:\n", sizeof(test_cases) / sizeof(test_cases[0]));

    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        printf("\nTest %zu: %s\n", i + 1, test_cases[i].test_name);
        bool result = test_cases[i].test_function();
        print_test_result(test_cases[i].test_name, result);
    }

    printf("\n===================================================\n");
    printf("Test Summary:\n");
    printf("  Total tests: %d\n", tests_passed + tests_failed);
    printf("  Passed: %d\n", tests_passed);
    printf("  Failed: %d\n", tests_failed);

    if (tests_failed == 0) {
        printf("\n✅ All tests passed!\n");
        return 0;
    } else {
        printf("\n❌ Some tests failed.\n");
        return 1;
    }
}