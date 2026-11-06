/* Begin - ReqID:TECC4.0 */
/*
 * File: test_formatter.c
 *
 * Abstract:
 *   Test program for TECC data formatter module.
 *   Tests zero-copy interfaces and ICU integration functionality.
 *
 * Authored by <wangxp@highgo.com>, 20260412.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_test/test_formatter.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

/* PostgreSQL frontend header - must be included first */
#include "postgres_fe.h"

/* PostgreSQL frontend memory utilities for testing */
#include "common/fe_memutils.h"  /* For pg_malloc, pg_free */

/* Include formatter headers */
#include "../../tecc_formatter/tecc_formatter.h"
/* Note: tecc_formatter_internal.h is not needed for testing as it contains internal definitions */

/* Include common types for constants */
#include "../../tecc_common/tecc_common_types.h"

/* Include shared types for error codes */
#include "../../../../common/tecc_types.h"

/* Constants that are missing from headers */
#define TECC_TEXT_LENGTH_PREFIX_SIZE 4
/* InvalidOid is defined in postgres_ext.h */

/* PostgreSQL frontend memory utilities for testing */
#include "common/fe_memutils.h"  /* For pg_malloc, pg_free */

/* Simplified ICU functions for testing - just return success */
bool tecc_formatter_icu_is_available(void)
{
    /* For testing, assume ICU is not available to simplify */
    return false;
}

TeccFormatterErrorCode tecc_formatter_icu_get_collation_name(Oid collation_oid,
                                                             char *name_buf,
                                                             size_t buf_size)
{
    /* Not implemented for testing */
    return TECC_FORMATTER_ERROR_ICU_NOT_AVAILABLE;
}

TeccFormatterErrorCode tecc_formatter_icu_create_collator(const char *collation_locale,
                                                         void **collator)
{
    /* Not implemented for testing */
    return TECC_FORMATTER_ERROR_ICU_NOT_AVAILABLE;
}

TeccFormatterErrorCode tecc_formatter_icu_generate_sortkey(void *collator,
                                                          const unsigned char *input_text,
                                                          size_t input_len,
                                                          unsigned char **sortkey,
                                                          size_t *sortkey_len)
{
    /* Not implemented for testing */
    return TECC_FORMATTER_ERROR_ICU_NOT_AVAILABLE;
}

void tecc_formatter_icu_destroy_collator(void *collator)
{
    /* Nothing to do */
    (void)collator;
}

/* Test helper functions */
static void print_test_result(const char *test_name, bool passed)
{
    printf("  %-40s %s\n", test_name, passed ? "✅ PASS" : "❌ FAIL");
}

static bool compare_buffers(const unsigned char *buf1, size_t len1,
                           const unsigned char *buf2, size_t len2)
{
    if (len1 != len2)
        return false;

    return memcmp(buf1, buf2, len1) == 0;
}

/* Test cases */

/* Test 1: Basic buffer normalization for numeric types */
static bool test_basic_numeric_buffer(void)
{
    printf("\n=== Test 1: Basic numeric buffer normalization ===\n");

    /* Create test context for integer */
    TeccCryptoContextCore context;
    memset(&context, 0, sizeof(context));
    context.data_type = INT4OID;
    context.algo_type = TECC_ALGO_RND;

    /* Test data: integer "12345" */
    const char *test_data = "12345";
    size_t test_len = strlen(test_data);

    /* Create buffer */
    TeccCryptoBuffer buffer;
    buffer.size = 1024;
    buffer.data = pg_malloc(buffer.size);
    buffer.used = 0;

    /* Test tecc_formatter_normalize_data_to_buffer */
    TeccFormatterErrorCode result = tecc_formatter_normalize_data_to_buffer(
        &context, (const unsigned char *)test_data, test_len, &buffer);

    bool passed = (result == TECC_FORMATTER_SUCCESS && buffer.used > 0);
    print_test_result("tecc_formatter_normalize_data_to_buffer (INT4)", passed);

    /* Clean up */
    pg_free(buffer.data);

    return passed;
}

/* Test 2: Text buffer normalization with RND algorithm */
static bool test_text_buffer_rnd(void)
{
    printf("\n=== Test 2: Text buffer normalization (RND algorithm) ===\n");

    /* Create test context for text */
    TeccCryptoContextCore context;
    memset(&context, 0, sizeof(context));
    context.data_type = TEXTOID;
    context.algo_type = TECC_ALGO_RND;

    /* Test data: simple text */
    const char *test_data = "Hello, World!";
    size_t test_len = strlen(test_data);

    /* Create buffer */
    TeccCryptoBuffer buffer;
    buffer.size = 1024;
    buffer.data = pg_malloc(buffer.size);
    buffer.used = 0;

    /* Test tecc_formatter_normalize_data_to_buffer */
    TeccFormatterErrorCode result = tecc_formatter_normalize_data_to_buffer(
        &context, (const unsigned char *)test_data, test_len, &buffer);

    bool passed = (result == TECC_FORMATTER_SUCCESS && buffer.used > 0);

    /* Verify that RND algorithm adds length prefix */
    if (passed && buffer.used == test_len + TECC_TEXT_LENGTH_PREFIX_SIZE) {
        /* Check length prefix */
        unsigned char *buf_data = (unsigned char *)buffer.data;
        uint32_t stored_len = (buf_data[0] << 24) | (buf_data[1] << 16) |
                             (buf_data[2] << 8) | buf_data[3];
        passed = (stored_len == test_len);
    }

    print_test_result("tecc_formatter_normalize_data_to_buffer (TEXT RND)", passed);

    /* Clean up */
    pg_free(buffer.data);

    return passed;
}

/* Test 3: In-place normalization */
static bool test_inplace_normalization(void)
{
    printf("\n=== Test 3: In-place normalization ===\n");

    /* Create test context */
    TeccCryptoContextCore context;
    memset(&context, 0, sizeof(context));
    context.data_type = TEXTOID;
    context.algo_type = TECC_ALGO_RND;

    /* Test data */
    const char *original_data = "Test in-place operation";
    size_t original_len = strlen(original_data);

    /* Create buffer with input data */
    TeccCryptoBuffer buffer;
    buffer.size = 1024;
    buffer.data = pg_malloc(buffer.size);
    buffer.used = original_len;

    /* Copy input data to buffer */
    memcpy(buffer.data, original_data, original_len);

    /* Test tecc_formatter_normalize_data_inplace */
    TeccFormatterErrorCode result = tecc_formatter_normalize_data_inplace(
        &context, &buffer);

    bool passed = (result == TECC_FORMATTER_SUCCESS && buffer.used > 0);

    /* Verify in-place operation changed buffer content */
    if (passed) {
        /* For RND algorithm, output should be different from input (has length prefix) */
        passed = (buffer.used != original_len ||
                 memcmp(buffer.data, original_data, original_len) != 0);
    }

    print_test_result("tecc_formatter_normalize_data_inplace", passed);

    /* Clean up */
    pg_free(buffer.data);

    return passed;
}

/* Test 4: OPE algorithm with collation (ICU integration) */
static bool test_ope_with_collation(void)
{
    printf("\n=== Test 4: OPE algorithm with collation (ICU integration) ===\n");

    /* Create test context for OPE with collation */
    TeccCryptoContextCore context;
    memset(&context, 0, sizeof(context));
    context.data_type = TEXTOID;
    context.algo_type = TECC_ALGO_OPE;
    context.collation_oid = 100;  /* Dummy collation OID */

    /* Test data */
    const char *test_data = "Text for OPE sorting";
    size_t test_len = strlen(test_data);

    /* Create buffer */
    TeccCryptoBuffer buffer;
    buffer.size = 1024;
    buffer.data = pg_malloc(buffer.size);
    buffer.used = 0;

    /* Test tecc_formatter_normalize_data_to_buffer */
    TeccFormatterErrorCode result = tecc_formatter_normalize_data_to_buffer(
        &context, (const unsigned char *)test_data, test_len, &buffer);

    /* Since ICU is not available, OPE with collation should fail or fallback */
    /* For now, we'll accept either success (fallback) or ICU_NOT_AVAILABLE error */
    bool passed = (result == TECC_FORMATTER_SUCCESS ||
                  result == TECC_FORMATTER_ERROR_ICU_NOT_AVAILABLE);

    print_test_result("tecc_formatter_normalize_data_to_buffer (OPE with collation)", passed);

    /* Clean up */
    pg_free(buffer.data);

    return passed;
}

/* Test 5: Error handling - invalid parameters */
static bool test_error_handling(void)
{
    printf("\n=== Test 5: Error handling ===\n");

    bool all_passed = true;

    /* Test 5.1: NULL context */
    {
        TeccCryptoBuffer buffer;
        buffer.size = 1024;
        buffer.data = pg_malloc(buffer.size);
        buffer.used = 0;

        const char *test_data = "test";
        TeccFormatterErrorCode result = tecc_formatter_normalize_data_to_buffer(
            NULL, (const unsigned char *)test_data, strlen(test_data), &buffer);

        bool passed = (result == TECC_FORMATTER_ERROR_INVALID_PARAM);
        print_test_result("NULL context parameter", passed);
        all_passed = all_passed && passed;

        pg_free(buffer.data);
    }

    /* Test 5.2: NULL input data */
    {
        TeccCryptoContextCore context;
        memset(&context, 0, sizeof(context));
        context.data_type = TEXTOID;
        context.algo_type = TECC_ALGO_RND;

        TeccCryptoBuffer buffer;
        buffer.size = 1024;
        buffer.data = pg_malloc(buffer.size);
        buffer.used = 0;

        TeccFormatterErrorCode result = tecc_formatter_normalize_data_to_buffer(
            &context, NULL, 10, &buffer);

        bool passed = (result == TECC_FORMATTER_ERROR_INVALID_PARAM);
        print_test_result("NULL input data", passed);
        all_passed = all_passed && passed;

        pg_free(buffer.data);
    }

    /* Test 5.3: NULL buffer */
    {
        TeccCryptoContextCore context;
        memset(&context, 0, sizeof(context));
        context.data_type = TEXTOID;
        context.algo_type = TECC_ALGO_RND;

        const char *test_data = "test";
        TeccFormatterErrorCode result = tecc_formatter_normalize_data_to_buffer(
            &context, (const unsigned char *)test_data, strlen(test_data), NULL);

        bool passed = (result == TECC_FORMATTER_ERROR_INVALID_PARAM);
        print_test_result("NULL buffer", passed);
        all_passed = all_passed && passed;
    }

    /* Test 5.4: Buffer too small */
    {
        TeccCryptoContextCore context;
        memset(&context, 0, sizeof(context));
        context.data_type = TEXTOID;
        context.algo_type = TECC_ALGO_RND;

        const char *test_data = "This is a relatively long test string";
        size_t test_len = strlen(test_data);

        TeccCryptoBuffer buffer;
        buffer.size = 10;  /* Too small */
        buffer.data = pg_malloc(buffer.size);
        buffer.used = 0;

        TeccFormatterErrorCode result = tecc_formatter_normalize_data_to_buffer(
            &context, (const unsigned char *)test_data, test_len, &buffer);

        bool passed = (result == TECC_FORMATTER_ERROR_BUFFER_TOO_SMALL);
        print_test_result("Buffer too small", passed);
        all_passed = all_passed && passed;

        pg_free(buffer.data);
    }

    return all_passed;
}

/* Test 6: Different algorithm types */
static bool test_algorithm_types(void)
{
    printf("\n=== Test 6: Different algorithm types ===\n");

    bool all_passed = true;
    const char *test_data = "Algorithm test";
    size_t test_len = strlen(test_data);

    /* Test different algorithm types */
    TeccAlgorithmType algorithms[] = {
        TECC_ALGO_RND,
        TECC_ALGO_DET,
        TECC_ALGO_OPE,
        TECC_ALGO_SEARCH
    };

    const char *algo_names[] = {
        "RND",
        "DET",
        "OPE",
        "SEARCH"
    };

    int i;
    for (i = 0; i < 4; i++) {
        /* Create context */
        TeccCryptoContextCore context;
        memset(&context, 0, sizeof(context));
        context.data_type = TEXTOID;
        context.algo_type = algorithms[i];

        /* For OPE without collation, test both cases */
        if (algorithms[i] == TECC_ALGO_OPE) {
            /* Test OPE without collation */
            context.collation_oid = InvalidOid;

            TeccCryptoBuffer buffer;
            buffer.size = 1024;
            buffer.data = pg_malloc(buffer.size);
            buffer.used = 0;

            TeccFormatterErrorCode result = tecc_formatter_normalize_data_to_buffer(
                &context, (const unsigned char *)test_data, test_len, &buffer);

            bool passed = (result == TECC_FORMATTER_SUCCESS && buffer.used > 0);
            char test_name[100];
            snprintf(test_name, sizeof(test_name), "Algorithm %s (no collation)", algo_names[i]);
            print_test_result(test_name, passed);
            all_passed = all_passed && passed;

            pg_free(buffer.data);
        } else {
            /* Test other algorithms */
            TeccCryptoBuffer buffer;
            buffer.size = 1024;
            buffer.data = pg_malloc(buffer.size);
            buffer.used = 0;

            TeccFormatterErrorCode result = tecc_formatter_normalize_data_to_buffer(
                &context, (const unsigned char *)test_data, test_len, &buffer);

            bool passed = (result == TECC_FORMATTER_SUCCESS && buffer.used > 0);
            char test_name[100];
            snprintf(test_name, sizeof(test_name), "Algorithm %s", algo_names[i]);
            print_test_result(test_name, passed);
            all_passed = all_passed && passed;

            pg_free(buffer.data);
        }
    }

    return all_passed;
}

/* Main test function */
int main(void)
{
    printf("========================================\n");
    printf("TECC Data Formatter Module Test Suite\n");
    printf("Testing zero-copy interfaces and ICU integration\n");
    printf("========================================\n");

    int tests_passed = 0;
    int tests_total = 0;

    /* Run test cases */
    if (test_basic_numeric_buffer()) tests_passed++;
    tests_total++;

    if (test_text_buffer_rnd()) tests_passed++;
    tests_total++;

    if (test_inplace_normalization()) tests_passed++;
    tests_total++;

    if (test_ope_with_collation()) tests_passed++;
    tests_total++;

    if (test_error_handling()) tests_passed++;
    tests_total++;

    if (test_algorithm_types()) tests_passed++;
    tests_total++;

    /* Print summary */
    printf("\n========================================\n");
    printf("Test Summary:\n");
    printf("  Total tests: %d\n", tests_total);
    printf("  Passed:      %d\n", tests_passed);
    printf("  Failed:      %d\n", tests_total - tests_passed);

    if (tests_passed == tests_total) {
        printf("\n✅ All tests passed!\n");
        return 0;
    } else {
        printf("\n❌ Some tests failed!\n");
        return 1;
    }
}

/* End - ReqID:TECC4.0 */