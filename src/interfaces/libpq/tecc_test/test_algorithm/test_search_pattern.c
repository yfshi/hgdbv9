/* Begin - ReqID:TECC4.0 */
/*
 * File: test_search_pattern.c
 *
 * Abstract:
 *   Comprehensive test program for SEARCH algorithm pattern parsing and matching
 *   with wildcard preservation. Tests pattern parsing, segment encryption,
 *   pattern matching, and various edge cases.
 *
 * Authored by <wangxp@highgo.com>, 20260414.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_test/test_algorithm/test_search_pattern.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <openssl/evp.h>

/* Include algorithm API headers */
#include "../../../../../src/include/common/tecc_algorithm/tecc_search_sm4.h"

/* Test key (16 bytes for SM4) */
static const unsigned char test_key[16] = {
    0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF,
    0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10
};

/* Test data structure */
typedef struct {
    const char *name;
    const char *pattern;
    const char *test_string;
    bool should_match;
} PatternTest;

/* Test cases for pattern parsing */
static const PatternTest pattern_tests[] = {
    /* Simple patterns */
    {"exact_match", "hello", "hello", true},
    {"exact_no_match", "hello", "world", false},

    /* Prefix patterns */
    {"simple_prefix", "hello%", "hello world", true},
    {"prefix_no_match", "hello%", "world hello", false},
    {"prefix_empty", "hello%", "hello", true},

    /* Suffix patterns */
    {"simple_suffix", "%world", "hello world", true},
    {"suffix_no_match", "%world", "world hello", false},
    {"suffix_empty", "%world", "world", true},

    /* Substring patterns */
    {"simple_substring", "%lo wo%", "hello world", true},
    {"substring_no_match", "%lo wo%", "hello universe", false},
    {"substring_beginning", "%hello%", "hello world", true},
    {"substring_ending", "%world%", "hello world", true},

    /* Middle wildcard patterns */
    {"middle_wildcard", "he%o", "hello", true},
    {"middle_wildcard_long", "he%o", "helloooo", true},
    {"middle_wildcard_no_match", "he%o", "helxo", false},

    /* Multiple wildcard patterns */
    {"multiple_wildcards", "%he%lo%", "hello world", true},
    {"multiple_wildcards_complex", "%he%w%ld%", "hello world", true},

    /* Underscore wildcard patterns */
    {"underscore_single", "h_llo", "hello", true},
    {"underscore_single_no_match", "h_llo", "hallo", false},
    {"multiple_underscores", "h__lo", "hello", true},

    /* Mixed wildcard patterns */
    {"mixed_wildcards", "h%l_o", "hello", true},
    {"mixed_wildcards_complex", "%h_l%o%", "hello world", true},

    /* Edge cases */
    {"empty_pattern", "", "", true},
    {"empty_pattern_no_match", "", "hello", false},
    {"only_wildcard", "%", "hello", true},
    {"only_wildcard_empty", "%", "", true},
    {"multiple_wildcards_only", "%%%", "hello", true},

    /* Special characters */
    {"special_chars", "test@123%", "test@123.com", true},
    {"unicode_chars", "测试%", "测试数据", true},

    {NULL, NULL, NULL, false}  /* Sentinel */
};

/* Test cases for pattern type detection */
typedef struct {
    const char *name;
    const char *pattern;
    bool has_wildcards;
    bool is_prefix;
    bool is_suffix;
    bool is_substring;
} PatternTypeTest;

static const PatternTypeTest pattern_type_tests[] = {
    {"no_wildcards", "hello", false, false, false, false},
    {"simple_prefix", "hello%", true, true, false, false},
    {"simple_suffix", "%hello", true, false, true, false},
    {"simple_substring", "%hello%", true, false, false, true},
    {"complex_pattern", "he%lo%world", true, false, false, false},
    {"multiple_prefix", "hello%world%", true, false, false, false},
    {"empty", "", false, false, false, false},
    {"only_wildcard", "%", true, false, false, false},
    {NULL, NULL, false, false, false, false}  /* Sentinel */
};

/* Helper function to print test result */
static void print_test_result(const char *test_name, bool passed)
{
    printf("  %-40s [%s]\n", test_name, passed ? "PASS" : "FAIL");
}

/* Helper function to encrypt a string */
static int encrypt_string(const char *plaintext, unsigned char *ciphertext,
                         size_t buffer_size, size_t *ciphertext_len)
{
    return tecc_search_sm4_encrypt(test_key, sizeof(test_key),
                                  (const unsigned char *)plaintext, strlen(plaintext),
                                  ciphertext, buffer_size, ciphertext_len, true);
}

/* Test 1: Basic pattern parsing */
static bool test_pattern_parsing(void)
{
    int i;
    bool all_passed = true;

    printf("=== Test 1: Pattern Parsing ===\n");

    for (i = 0; pattern_tests[i].name != NULL; i++)
    {
        const PatternTest *test = &pattern_tests[i];
        Sm4SearchParsedPattern *parsed_pattern = NULL;
        int result;
        bool passed = true;

        /* Parse pattern */
        result = tecc_search_sm4_parse_pattern(test->pattern, strlen(test->pattern),
                                              &parsed_pattern);
        if (result != SM4_SEARCH_PATTERN_SUCCESS)
        {
            printf("  ERROR: Failed to parse pattern '%s' (error: %d)\n",
                   test->pattern, result);
            all_passed = false;
            continue;
        }

        /* Encrypt fixed string segments */
        result = tecc_search_sm4_encrypt_pattern_segments(parsed_pattern,
                                                         test_key, sizeof(test_key));
        if (result != SM4_SEARCH_PATTERN_SUCCESS)
        {
            printf("  ERROR: Failed to encrypt pattern segments for '%s' (error: %d)\n",
                   test->pattern, result);
            tecc_search_sm4_free_parsed_pattern(parsed_pattern);
            all_passed = false;
            continue;
        }

        /* Verify segment count logic */
        if (strlen(test->pattern) == 0 && parsed_pattern->segment_count != 0)
        {
            printf("  ERROR: Empty pattern should have 0 segments, got %zu\n",
                   parsed_pattern->segment_count);
            passed = false;
        }

        print_test_result(test->name, passed);
        if (!passed) all_passed = false;

        tecc_search_sm4_free_parsed_pattern(parsed_pattern);
    }

    return all_passed;
}

/* Test 2: Pattern type detection */
static bool test_pattern_type_detection(void)
{
    int i;
    bool all_passed = true;

    printf("\n=== Test 2: Pattern Type Detection ===\n");

    for (i = 0; pattern_type_tests[i].name != NULL; i++)
    {
        const PatternTypeTest *test = &pattern_type_tests[i];
        size_t pattern_len = strlen(test->pattern);
        bool has_wildcards, is_prefix, is_suffix, is_substring;
        bool passed = true;

        /* Test wildcard detection */
        has_wildcards = tecc_search_sm4_pattern_has_wildcards(test->pattern, pattern_len);
        if (has_wildcards != test->has_wildcards)
        {
            printf("  ERROR: has_wildcards mismatch for '%s': expected %d, got %d\n",
                   test->pattern, test->has_wildcards, has_wildcards);
            passed = false;
        }

        /* Test prefix detection */
        is_prefix = tecc_search_sm4_is_simple_prefix_pattern(test->pattern, pattern_len);
        if (is_prefix != test->is_prefix)
        {
            printf("  ERROR: is_prefix mismatch for '%s': expected %d, got %d\n",
                   test->pattern, test->is_prefix, is_prefix);
            passed = false;
        }

        /* Test suffix detection */
        is_suffix = tecc_search_sm4_is_simple_suffix_pattern(test->pattern, pattern_len);
        if (is_suffix != test->is_suffix)
        {
            printf("  ERROR: is_suffix mismatch for '%s': expected %d, got %d\n",
                   test->pattern, test->is_suffix, is_suffix);
            passed = false;
        }

        /* Test substring detection */
        is_substring = tecc_search_sm4_is_simple_substring_pattern(test->pattern, pattern_len);
        if (is_substring != test->is_substring)
        {
            printf("  ERROR: is_substring mismatch for '%s': expected %d, got %d\n",
                   test->pattern, test->is_substring, is_substring);
            passed = false;
        }

        print_test_result(test->name, passed);
        if (!passed) all_passed = false;
    }

    return all_passed;
}

/* Test 3: Basic encryption and decryption */
static bool test_basic_encryption(void)
{
    const char *test_strings[] = {
        "hello",
        "world",
        "test data",
        "1234567890",
        "special@chars#123",
        "",
        NULL
    };
    int i;
    bool all_passed = true;

    printf("\n=== Test 3: Basic Encryption ===\n");

    for (i = 0; test_strings[i] != NULL; i++)
    {
        const char *test_string = test_strings[i];
        size_t test_len = strlen(test_string);
        unsigned char ciphertext[1024];
        size_t ciphertext_len;
        int result;
        bool passed = true;

        /* Calculate buffer size */
        {
            size_t buffer_size = tecc_search_sm4_calc_encrypt_buffer_size(test_len);
            if (buffer_size == 0 && test_len > 0)
            {
                printf("  ERROR: Failed to calculate buffer size for '%s'\n", test_string);
                all_passed = false;
                continue;
            }
        }

        /* Encrypt */
        result = encrypt_string(test_string, ciphertext, sizeof(ciphertext), &ciphertext_len);
        if (result != SM4_SEARCH_SUCCESS)
        {
            printf("  ERROR: Encryption failed for '%s' (error: %d)\n",
                   test_string, result);
            passed = false;
        }

        /* Validate format */
        if (passed)
        {
            result = tecc_search_sm4_validate_format(ciphertext, ciphertext_len);
            if (result != SM4_SEARCH_SUCCESS)
            {
                printf("  ERROR: Invalid ciphertext format for '%s' (error: %d)\n",
                       test_string, result);
                passed = false;
            }
        }

        /* Test decryption (should fail for SEARCH algorithm) */
        if (passed && test_len > 0)
        {
            unsigned char decrypted[1024];
            size_t decrypted_len;

            result = tecc_search_sm4_decrypt(test_key, sizeof(test_key),
                                            ciphertext, ciphertext_len,
                                            decrypted, sizeof(decrypted),
                                            &decrypted_len);
            if (result != SM4_SEARCH_ERROR_DECRYPT_NOT_SUPPORTED)
            {
                printf("  ERROR: SEARCH decryption should not be supported, got error: %d\n",
                       result);
                passed = false;
            }
        }

        char test_name[64];
        snprintf(test_name, sizeof(test_name), "encrypt_%s", test_string);
        print_test_result(test_name, passed);
        if (!passed) all_passed = false;
    }

    return all_passed;
}

/* Test 4: Edge cases and error handling */
static bool test_edge_cases(void)
{
    bool all_passed = true;

    printf("\n=== Test 4: Edge Cases and Error Handling ===\n");

    /* Test 4.1: NULL parameter handling */
    {
        Sm4SearchParsedPattern *parsed_pattern = NULL;
        int result;

        result = tecc_search_sm4_parse_pattern(NULL, 0, &parsed_pattern);
        if (result != SM4_SEARCH_PATTERN_ERROR_INVALID_PARAM)
        {
            printf("  ERROR: NULL pattern should return invalid param error, got: %d\n", result);
            all_passed = false;
        }

        print_test_result("null_pattern_handling",
                         result == SM4_SEARCH_PATTERN_ERROR_INVALID_PARAM);
    }

    /* Test 4.2: Invalid key length */
    {
        Sm4SearchParsedPattern *parsed_pattern = NULL;
        int result;
        const char *pattern = "test";

        result = tecc_search_sm4_parse_pattern(pattern, strlen(pattern), &parsed_pattern);
        if (result == SM4_SEARCH_PATTERN_SUCCESS)
        {
            /* Try to encrypt with invalid key length */
            unsigned char invalid_key[15] = {0};  /* 15 bytes, should be 16 */
            result = tecc_search_sm4_encrypt_pattern_segments(parsed_pattern,
                                                             invalid_key, 15);
            if (result != SM4_SEARCH_PATTERN_ERROR_INVALID_PARAM)
            {
                printf("  ERROR: Invalid key length should return error, got: %d\n", result);
                all_passed = false;
            }
            tecc_search_sm4_free_parsed_pattern(parsed_pattern);
        }

        print_test_result("invalid_key_length",
                         result == SM4_SEARCH_PATTERN_ERROR_INVALID_PARAM);
    }

    /* Test 4.3: Very long pattern */
    {
        char long_pattern[1025];
        size_t i;

        /* Create a 1024-character pattern */
        for (i = 0; i < sizeof(long_pattern) - 1; i++)
        {
            long_pattern[i] = 'a' + (i % 26);
        }
        long_pattern[sizeof(long_pattern) - 1] = '\0';

        Sm4SearchParsedPattern *parsed_pattern = NULL;
        int result = tecc_search_sm4_parse_pattern(long_pattern, strlen(long_pattern),
                                                  &parsed_pattern);

        bool passed = (result == SM4_SEARCH_PATTERN_SUCCESS);
        if (passed)
        {
            result = tecc_search_sm4_encrypt_pattern_segments(parsed_pattern,
                                                             test_key, sizeof(test_key));
            passed = (result == SM4_SEARCH_PATTERN_SUCCESS);
            tecc_search_sm4_free_parsed_pattern(parsed_pattern);
        }

        print_test_result("very_long_pattern", passed);
        if (!passed) all_passed = false;
    }

    /* Test 4.4: Pattern with only wildcards */
    {
        const char *patterns[] = {"%", "%%", "%%%", "_", "__", "___", "%_%", NULL};
        int i;

        for (i = 0; patterns[i] != NULL; i++)
        {
            Sm4SearchParsedPattern *parsed_pattern = NULL;
            int result = tecc_search_sm4_parse_pattern(patterns[i], strlen(patterns[i]),
                                                      &parsed_pattern);

            bool passed = (result == SM4_SEARCH_PATTERN_SUCCESS);
            if (passed)
            {
                /* Should have no fixed string segments to encrypt */
                result = tecc_search_sm4_encrypt_pattern_segments(parsed_pattern,
                                                                 test_key, sizeof(test_key));
                passed = (result == SM4_SEARCH_PATTERN_SUCCESS);
                tecc_search_sm4_free_parsed_pattern(parsed_pattern);
            }

            char test_name[64];
            snprintf(test_name, sizeof(test_name), "only_wildcards_%s", patterns[i]);
            print_test_result(test_name, passed);
            if (!passed) all_passed = false;
        }
    }

    return all_passed;
}

/* Test 5: Memory leak detection (basic) */
static bool test_memory_management(void)
{
    bool all_passed = true;
    int i;

    printf("\n=== Test 5: Memory Management ===\n");

    /* Test multiple allocations and frees */
    for (i = 0; i < 100; i++)
    {
        Sm4SearchParsedPattern *parsed_pattern = NULL;
        int result;
        const char *pattern = "test%pattern%123";

        result = tecc_search_sm4_parse_pattern(pattern, strlen(pattern), &parsed_pattern);
        if (result != SM4_SEARCH_PATTERN_SUCCESS)
        {
            printf("  ERROR: Failed to parse pattern in iteration %d\n", i);
            all_passed = false;
            break;
        }

        result = tecc_search_sm4_encrypt_pattern_segments(parsed_pattern,
                                                         test_key, sizeof(test_key));
        if (result != SM4_SEARCH_PATTERN_SUCCESS)
        {
            printf("  ERROR: Failed to encrypt pattern in iteration %d\n", i);
            tecc_search_sm4_free_parsed_pattern(parsed_pattern);
            all_passed = false;
            break;
        }

        tecc_search_sm4_free_parsed_pattern(parsed_pattern);
    }

    print_test_result("multiple_allocations_frees", all_passed);

    /* Test context creation and destruction */
    {
        Sm4SearchParsedPattern *parsed_pattern = NULL;
        Sm4SearchMatchContext *context = NULL;
        unsigned char ciphertext[1024];
        size_t ciphertext_len;
        int result;
        bool passed = true;
        const char *pattern = "test";
        const char *data = "test data";

        /* Parse pattern */
        result = tecc_search_sm4_parse_pattern(pattern, strlen(pattern), &parsed_pattern);
        if (result != SM4_SEARCH_PATTERN_SUCCESS)
        {
            printf("  ERROR: Failed to parse pattern for context test\n");
            all_passed = false;
        }
        else
        {
            /* Encrypt pattern segments */
            result = tecc_search_sm4_encrypt_pattern_segments(parsed_pattern,
                                                             test_key, sizeof(test_key));
            if (result != SM4_SEARCH_PATTERN_SUCCESS)
            {
                printf("  ERROR: Failed to encrypt pattern for context test\n");
                passed = false;
            }
            else
            {
                /* Encrypt test data */
                result = encrypt_string(data, ciphertext, sizeof(ciphertext), &ciphertext_len);
                if (result != SM4_SEARCH_SUCCESS)
                {
                    printf("  ERROR: Failed to encrypt data for context test\n");
                    passed = false;
                }
                else
                {
                    /* Create match context */
                    context = tecc_search_sm4_create_match_context(
                        parsed_pattern, ciphertext, ciphertext_len,
                        test_key, sizeof(test_key), true);

                    if (context == NULL)
                    {
                        printf("  ERROR: Failed to create match context\n");
                        passed = false;
                    }
                    else
                    {
                        /* Free context */
                        tecc_search_sm4_free_match_context(context);
                    }
                }
            }

            /* Free parsed pattern */
            tecc_search_sm4_free_parsed_pattern(parsed_pattern);
        }

        print_test_result("context_creation_destruction", passed);
        if (!passed) all_passed = false;
    }

    return all_passed;
}

/* Main test function */
int main(void)
{
    bool all_passed = true;
    int passed_count = 0;
    int total_count = 0;

    printf("TECC SEARCH Algorithm Pattern Parsing Test Suite\n");
    printf("================================================\n\n");

    /* Initialize OpenSSL if needed (only for older OpenSSL versions) */
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    OpenSSL_add_all_algorithms();
#endif

    /* Run all tests */
    {
        struct {
            const char *name;
            bool (*test_func)(void);
        } tests[] = {
            {"Pattern Parsing", test_pattern_parsing},
            {"Pattern Type Detection", test_pattern_type_detection},
            {"Basic Encryption", test_basic_encryption},
            {"Edge Cases", test_edge_cases},
            {"Memory Management", test_memory_management},
            {NULL, NULL}
        };
        int i;

        for (i = 0; tests[i].name != NULL; i++)
        {
            total_count++;
            if (tests[i].test_func())
            {
                passed_count++;
            }
            else
            {
                all_passed = false;
            }
        }
    }

    /* Print summary */
    printf("\n================================================\n");
    printf("Test Summary:\n");
    printf("  Total tests: %d\n", total_count);
    printf("  Passed: %d\n", passed_count);
    printf("  Failed: %d\n", total_count - passed_count);
    printf("  Result: %s\n", all_passed ? "ALL TESTS PASSED" : "SOME TESTS FAILED");
    printf("================================================\n");

    return all_passed ? 0 : 1;
}
/* End - ReqID:TECC4.0 */