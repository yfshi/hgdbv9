/* Begin - ReqID:TECC4.0 */
/*
 * File: test_formatter_standalone.c
 *
 * Abstract:
 *   Standalone test program for TECC data formatter module.
 *   Tests zero-copy interfaces and ICU integration functionality using mock functions.
 *   This version does not link with the real formatter module.
 *
 * Authored by <wangxp@highgo.com>, 20260412.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_test/test_formatter/test_formatter_standalone.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

/* Mock type definitions for testing */
typedef unsigned int Oid;
typedef int TeccAlgorithmType;
typedef int TeccFormatterErrorCode;

/* Algorithm types */
#define TECC_ALGO_RND     0
#define TECC_ALGO_DET     1
#define TECC_ALGO_OPE     2
#define TECC_ALGO_SEARCH  3

/* Data type OIDs */
#define INT2OID    21
#define INT4OID    23
#define INT8OID    20
#define FLOAT4OID  700
#define FLOAT8OID  701
#define NUMERICOID 1700
#define VARCHAROID 1043
#define BPCHAROID  1042
#define TEXTOID    25

/* Error codes */
#define TECC_FORMATTER_SUCCESS                0
#define TECC_FORMATTER_ERROR_INVALID_PARAM    -1
#define TECC_FORMATTER_ERROR_BUFFER_TOO_SMALL -2
#define TECC_FORMATTER_ERROR_DATA_TOO_LARGE   -3
#define TECC_FORMATTER_ERROR_INVALID_TYPE     -4
#define TECC_FORMATTER_ERROR_MEMORY_ALLOC     -5

/* Constants */
#define TECC_TEXT_LENGTH_PREFIX_SIZE 4
#define TECC_MAX_DATA_SIZE 65536
#define InvalidOid 0

/* Crypto context structure */
typedef struct TeccCryptoContextCore
{
    Oid data_type;
    TeccAlgorithmType algo_type;
    Oid collation_oid;
} TeccCryptoContextCore;

/* Crypto buffer structure */
typedef struct TeccCryptoBuffer
{
    void *data;
    size_t size;
    size_t used;
} TeccCryptoBuffer;

/* Mock formatter functions for testing */
static TeccFormatterErrorCode mock_tecc_formatter_normalize_data_to_buffer(
    TeccCryptoContextCore *context,
    const unsigned char *input_data,
    size_t input_len,
    TeccCryptoBuffer *buffer)
{
    /* Parameter validation */
    if (!context || !input_data || !buffer || !buffer->data)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    if (input_len == 0 || input_len > TECC_MAX_DATA_SIZE)
        return TECC_FORMATTER_ERROR_DATA_TOO_LARGE;

    /* Check buffer size */
    size_t required_size = input_len;

    if (context->algo_type == TECC_ALGO_RND) {
        /* RND algorithm adds length prefix */
        required_size = TECC_TEXT_LENGTH_PREFIX_SIZE + input_len;
    }

    if (buffer->size < required_size)
        return TECC_FORMATTER_ERROR_BUFFER_TOO_SMALL;

    /* Process based on algorithm type */
    unsigned char *buf_data = (unsigned char *)buffer->data;

    if (context->algo_type == TECC_ALGO_RND) {
        /* Add 4-byte network byte order length prefix */
        buf_data[0] = (input_len >> 24) & 0xFF;
        buf_data[1] = (input_len >> 16) & 0xFF;
        buf_data[2] = (input_len >> 8) & 0xFF;
        buf_data[3] = input_len & 0xFF;

        /* Copy input data */
        memcpy(buf_data + TECC_TEXT_LENGTH_PREFIX_SIZE, input_data, input_len);
        buffer->used = TECC_TEXT_LENGTH_PREFIX_SIZE + input_len;
    } else {
        /* Other algorithms: copy data directly */
        memcpy(buf_data, input_data, input_len);
        buffer->used = input_len;
    }

    return TECC_FORMATTER_SUCCESS;
}

static TeccFormatterErrorCode mock_tecc_formatter_normalize_data_inplace(
    TeccCryptoContextCore *context,
    TeccCryptoBuffer *buffer)
{
    /* Parameter validation */
    if (!context || !buffer || !buffer->data)
        return TECC_FORMATTER_ERROR_INVALID_PARAM;

    if (buffer->used == 0 || buffer->used > TECC_MAX_DATA_SIZE)
        return TECC_FORMATTER_ERROR_DATA_TOO_LARGE;

    /* For in-place operation, we need to handle the case where output might be larger */
    size_t input_len = buffer->used;

    /* Check if we have enough space for potential expansion */
    size_t required_size = input_len;
    if (context->algo_type == TECC_ALGO_RND) {
        required_size = TECC_TEXT_LENGTH_PREFIX_SIZE + input_len;
    }

    if (buffer->size < required_size)
        return TECC_FORMATTER_ERROR_BUFFER_TOO_SMALL;

    /* If output size is same as input, we can process in-place */
    if (required_size == input_len) {
        /* Already in correct format for non-RND algorithms */
        return TECC_FORMATTER_SUCCESS;
    }

    /* For RND algorithm, we need to add length prefix */
    if (context->algo_type == TECC_ALGO_RND) {
        /* Move data to make room for prefix */
        memmove((unsigned char *)buffer->data + TECC_TEXT_LENGTH_PREFIX_SIZE,
                buffer->data, input_len);

        /* Add length prefix */
        unsigned char *buf_data = (unsigned char *)buffer->data;
        buf_data[0] = (input_len >> 24) & 0xFF;
        buf_data[1] = (input_len >> 16) & 0xFF;
        buf_data[2] = (input_len >> 8) & 0xFF;
        buf_data[3] = input_len & 0xFF;

        buffer->used = TECC_TEXT_LENGTH_PREFIX_SIZE + input_len;
    }

    return TECC_FORMATTER_SUCCESS;
}

/* Mock ICU functions for testing */
static bool mock_tecc_formatter_icu_is_available(void)
{
    /* For testing, assume ICU is available */
    return true;
}

static TeccFormatterErrorCode mock_tecc_formatter_icu_get_collation_name(Oid collation_oid,
                                                                         char *name_buf,
                                                                         size_t buf_size)
{
    /* Simplified: return a default collation name */
    if (buf_size < 10)
        return TECC_FORMATTER_ERROR_BUFFER_TOO_SMALL;

    strcpy(name_buf, "en_US.utf8");
    return TECC_FORMATTER_SUCCESS;
}

static TeccFormatterErrorCode mock_tecc_formatter_icu_create_collator(const char *collation_locale,
                                                                      void **collator)
{
    /* Simplified: just set a dummy pointer */
    *collator = (void *)0x12345678;
    return TECC_FORMATTER_SUCCESS;
}

static TeccFormatterErrorCode mock_tecc_formatter_icu_generate_sortkey(void *collator,
                                                                       const unsigned char *input_text,
                                                                       size_t input_len,
                                                                       unsigned char **sortkey,
                                                                       size_t *sortkey_len)
{
    /* Simplified: create a dummy sort key */
    *sortkey_len = input_len + 4;  /* Add some overhead */
    *sortkey = (unsigned char *)malloc(*sortkey_len);
    if (!*sortkey)
        return TECC_FORMATTER_ERROR_MEMORY_ALLOC;

    /* Create a simple "sort key": length prefix + data */
    (*sortkey)[0] = (input_len >> 24) & 0xFF;
    (*sortkey)[1] = (input_len >> 16) & 0xFF;
    (*sortkey)[2] = (input_len >> 8) & 0xFF;
    (*sortkey)[3] = input_len & 0xFF;
    memcpy(*sortkey + 4, input_text, input_len);

    return TECC_FORMATTER_SUCCESS;
}

static void mock_tecc_formatter_icu_destroy_collator(void *collator)
{
    /* Nothing to do for dummy collator */
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

/* Simple memory allocation for testing */
static void *test_palloc(size_t size)
{
    void *ptr = malloc(size);
    if (ptr)
        memset(ptr, 0, size);
    return ptr;
}

static void test_pfree(void *ptr)
{
    if (ptr)
        free(ptr);
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
    buffer.data = test_palloc(buffer.size);
    buffer.used = 0;

    /* Test tecc_formatter_normalize_data_to_buffer */
    TeccFormatterErrorCode result = mock_tecc_formatter_normalize_data_to_buffer(
        &context, (const unsigned char *)test_data, test_len, &buffer);

    bool passed = (result == TECC_FORMATTER_SUCCESS && buffer.used > 0);
    print_test_result("tecc_formatter_normalize_data_to_buffer (INT4)", passed);

    /* Clean up */
    test_pfree(buffer.data);

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
    buffer.data = test_palloc(buffer.size);
    buffer.used = 0;

    /* Test tecc_formatter_normalize_data_to_buffer */
    TeccFormatterErrorCode result = mock_tecc_formatter_normalize_data_to_buffer(
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
    test_pfree(buffer.data);

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
    buffer.data = test_palloc(buffer.size);
    buffer.used = original_len;

    /* Copy input data to buffer */
    memcpy(buffer.data, original_data, original_len);

    /* Test tecc_formatter_normalize_data_inplace */
    TeccFormatterErrorCode result = mock_tecc_formatter_normalize_data_inplace(
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
    test_pfree(buffer.data);

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
    buffer.data = test_palloc(buffer.size);
    buffer.used = 0;

    /* Test tecc_formatter_normalize_data_to_buffer */
    TeccFormatterErrorCode result = mock_tecc_formatter_normalize_data_to_buffer(
        &context, (const unsigned char *)test_data, test_len, &buffer);

    bool passed = (result == TECC_FORMATTER_SUCCESS && buffer.used > 0);

    /* For OPE with collation, mock function should work like other algorithms */
    if (passed) {
        /* Mock function doesn't differentiate OPE with collation */
        passed = (buffer.used == test_len);
    }

    print_test_result("tecc_formatter_normalize_data_to_buffer (OPE with collation)", passed);

    /* Clean up */
    test_pfree(buffer.data);

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
        buffer.data = test_palloc(buffer.size);
        buffer.used = 0;

        const char *test_data = "test";
        TeccFormatterErrorCode result = mock_tecc_formatter_normalize_data_to_buffer(
            NULL, (const unsigned char *)test_data, strlen(test_data), &buffer);

        bool passed = (result == TECC_FORMATTER_ERROR_INVALID_PARAM);
        print_test_result("NULL context parameter", passed);
        all_passed = all_passed && passed;

        test_pfree(buffer.data);
    }

    /* Test 5.2: NULL input data */
    {
        TeccCryptoContextCore context;
        memset(&context, 0, sizeof(context));
        context.data_type = TEXTOID;
        context.algo_type = TECC_ALGO_RND;

        TeccCryptoBuffer buffer;
        buffer.size = 1024;
        buffer.data = test_palloc(buffer.size);
        buffer.used = 0;

        TeccFormatterErrorCode result = mock_tecc_formatter_normalize_data_to_buffer(
            &context, NULL, 10, &buffer);

        bool passed = (result == TECC_FORMATTER_ERROR_INVALID_PARAM);
        print_test_result("NULL input data", passed);
        all_passed = all_passed && passed;

        test_pfree(buffer.data);
    }

    /* Test 5.3: NULL buffer */
    {
        TeccCryptoContextCore context;
        memset(&context, 0, sizeof(context));
        context.data_type = TEXTOID;
        context.algo_type = TECC_ALGO_RND;

        const char *test_data = "test";
        TeccFormatterErrorCode result = mock_tecc_formatter_normalize_data_to_buffer(
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
        buffer.data = test_palloc(buffer.size);
        buffer.used = 0;

        TeccFormatterErrorCode result = mock_tecc_formatter_normalize_data_to_buffer(
            &context, (const unsigned char *)test_data, test_len, &buffer);

        bool passed = (result == TECC_FORMATTER_ERROR_BUFFER_TOO_SMALL);
        print_test_result("Buffer too small", passed);
        all_passed = all_passed && passed;

        test_pfree(buffer.data);
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
            buffer.data = test_palloc(buffer.size);
            buffer.used = 0;

            TeccFormatterErrorCode result = mock_tecc_formatter_normalize_data_to_buffer(
                &context, (const unsigned char *)test_data, test_len, &buffer);

            bool passed = (result == TECC_FORMATTER_SUCCESS && buffer.used > 0);
            char test_name[100];
            sprintf(test_name, "Algorithm %s (no collation)", algo_names[i]);
            print_test_result(test_name, passed);
            all_passed = all_passed && passed;

            test_pfree(buffer.data);
        } else {
            /* Test other algorithms */
            TeccCryptoBuffer buffer;
            buffer.size = 1024;
            buffer.data = test_palloc(buffer.size);
            buffer.used = 0;

            TeccFormatterErrorCode result = mock_tecc_formatter_normalize_data_to_buffer(
                &context, (const unsigned char *)test_data, test_len, &buffer);

            bool passed = (result == TECC_FORMATTER_SUCCESS && buffer.used > 0);
            char test_name[100];
            sprintf(test_name, "Algorithm %s", algo_names[i]);
            print_test_result(test_name, passed);
            all_passed = all_passed && passed;

            test_pfree(buffer.data);
        }
    }

    return all_passed;
}

/* Test 7: ICU integration functions */
static bool test_icu_integration(void)
{
    printf("\n=== Test 7: ICU integration functions ===\n");

    bool all_passed = true;

    /* Test 7.1: ICU availability check */
    {
        bool available = mock_tecc_formatter_icu_is_available();
        bool passed = (available == true);
        print_test_result("ICU availability check", passed);
        all_passed = all_passed && passed;
    }

    /* Test 7.2: Get collation locale */
    {
        char collation_locale[64];
        TeccFormatterErrorCode result = mock_tecc_formatter_icu_get_collation_name(
            100, collation_locale, sizeof(collation_locale));

        bool passed = (result == TECC_FORMATTER_SUCCESS &&
                      strcmp(collation_locale, "en_US.utf8") == 0);
        print_test_result("Get collation locale", passed);
        all_passed = all_passed && passed;
    }

    /* Test 7.3: Create collator */
    {
        void *collator = NULL;
        TeccFormatterErrorCode result = mock_tecc_formatter_icu_create_collator(
            "en_US.utf8", &collator);

        bool passed = (result == TECC_FORMATTER_SUCCESS && collator != NULL);
        print_test_result("Create collator", passed);
        all_passed = all_passed && passed;

        if (collator) {
            mock_tecc_formatter_icu_destroy_collator(collator);
        }
    }

    /* Test 7.4: Generate sort key */
    {
        void *collator = (void *)0x12345678;
        const char *test_text = "Test text for sorting";
        size_t test_len = strlen(test_text);
        unsigned char *sortkey = NULL;
        size_t sortkey_len = 0;

        TeccFormatterErrorCode result = mock_tecc_formatter_icu_generate_sortkey(
            collator, (const unsigned char *)test_text, test_len, &sortkey, &sortkey_len);

        bool passed = (result == TECC_FORMATTER_SUCCESS && sortkey != NULL && sortkey_len > 0);
        print_test_result("Generate sort key", passed);
        all_passed = all_passed && passed;

        if (sortkey) {
            free(sortkey);
        }
    }

    return all_passed;
}

/* Main test function */
int main(void)
{
    printf("========================================\n");
    printf("TECC Data Formatter Module Test Suite (Standalone)\n");
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

    if (test_icu_integration()) tests_passed++;
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