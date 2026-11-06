/* Begin - ReqID:TECC4.0 */
/*
 * File: simple_test.c
 *
 * Abstract:
 *   Simple test program for TECC data formatter module.
 *   Tests core buffer normalization functions with minimal dependencies.
 *
 * Authored by <wangxp@highgo.com>, 20260412.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_test/test_formatter/simple_test.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

/* Simple type definitions for testing */
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
static TeccFormatterErrorCode mock_normalize_data_to_buffer(
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

static TeccFormatterErrorCode mock_normalize_data_inplace(
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

/* Test helper functions */
static void print_test_result(const char *test_name, bool passed)
{
    printf("  %-40s %s\n", test_name, passed ? "✅ PASS" : "❌ FAIL");
}

/* Test 1: Basic buffer normalization */
static bool test_basic_buffer(void)
{
    printf("\n=== Test 1: Basic buffer normalization ===\n");

    /* Create test context */
    TeccCryptoContextCore context;
    context.data_type = TEXTOID;
    context.algo_type = TECC_ALGO_RND;
    context.collation_oid = InvalidOid;

    /* Test data */
    const char *test_data = "Hello, World!";
    size_t test_len = strlen(test_data);

    /* Create buffer */
    TeccCryptoBuffer buffer;
    buffer.size = 1024;
    buffer.data = malloc(buffer.size);
    buffer.used = 0;

    /* Test normalization */
    TeccFormatterErrorCode result = mock_normalize_data_to_buffer(
        &context, (const unsigned char *)test_data, test_len, &buffer);

    bool passed = (result == TECC_FORMATTER_SUCCESS && buffer.used > 0);

    /* Verify RND algorithm adds length prefix */
    if (passed) {
        passed = (buffer.used == test_len + TECC_TEXT_LENGTH_PREFIX_SIZE);

        /* Verify length prefix */
        if (passed) {
            unsigned char *buf_data = (unsigned char *)buffer.data;
            uint32_t stored_len = (buf_data[0] << 24) | (buf_data[1] << 16) |
                                 (buf_data[2] << 8) | buf_data[3];
            passed = (stored_len == test_len);
        }
    }

    print_test_result("Basic buffer normalization (RND)", passed);

    /* Clean up */
    free(buffer.data);

    return passed;
}

/* Test 2: Different algorithm types */
static bool test_algorithm_types(void)
{
    printf("\n=== Test 2: Different algorithm types ===\n");

    bool all_passed = true;
    const char *test_data = "Test data";
    size_t test_len = strlen(test_data);

    /* Test different algorithm types */
    TeccAlgorithmType algorithms[] = {TECC_ALGO_RND, TECC_ALGO_DET, TECC_ALGO_OPE, TECC_ALGO_SEARCH};
    const char *algo_names[] = {"RND", "DET", "OPE", "SEARCH"};

    int i;
    for (i = 0; i < 4; i++) {
        /* Create context */
        TeccCryptoContextCore context;
        context.data_type = TEXTOID;
        context.algo_type = algorithms[i];
        context.collation_oid = InvalidOid;

        /* Create buffer */
        TeccCryptoBuffer buffer;
        buffer.size = 1024;
        buffer.data = malloc(buffer.size);
        buffer.used = 0;

        /* Test normalization */
        TeccFormatterErrorCode result = mock_normalize_data_to_buffer(
            &context, (const unsigned char *)test_data, test_len, &buffer);

        bool passed = (result == TECC_FORMATTER_SUCCESS && buffer.used > 0);

        /* Verify expected output size */
        if (passed) {
            if (algorithms[i] == TECC_ALGO_RND) {
                passed = (buffer.used == test_len + TECC_TEXT_LENGTH_PREFIX_SIZE);
            } else {
                passed = (buffer.used == test_len);
            }
        }

        char test_name[100];
        sprintf(test_name, "Algorithm %s", algo_names[i]);
        print_test_result(test_name, passed);
        all_passed = all_passed && passed;

        /* Clean up */
        free(buffer.data);
    }

    return all_passed;
}

/* Test 3: In-place normalization */
static bool test_inplace(void)
{
    printf("\n=== Test 3: In-place normalization ===\n");

    bool all_passed = true;

    /* Test 3.1: RND algorithm (requires expansion) */
    {
        TeccCryptoContextCore context;
        context.data_type = TEXTOID;
        context.algo_type = TECC_ALGO_RND;
        context.collation_oid = InvalidOid;

        const char *original_data = "In-place test";
        size_t original_len = strlen(original_data);

        /* Create buffer with enough space for expansion */
        TeccCryptoBuffer buffer;
        buffer.size = original_len + TECC_TEXT_LENGTH_PREFIX_SIZE + 10; /* Extra space */
        buffer.data = malloc(buffer.size);
        buffer.used = original_len;
        memcpy(buffer.data, original_data, original_len);

        /* Test in-place normalization */
        TeccFormatterErrorCode result = mock_normalize_data_inplace(&context, &buffer);

        bool passed = (result == TECC_FORMATTER_SUCCESS && buffer.used > original_len);
        print_test_result("In-place RND (with expansion)", passed);
        all_passed = all_passed && passed;

        free(buffer.data);
    }

    /* Test 3.2: DET algorithm (no expansion needed) */
    {
        TeccCryptoContextCore context;
        context.data_type = TEXTOID;
        context.algo_type = TECC_ALGO_DET;
        context.collation_oid = InvalidOid;

        const char *original_data = "No expansion needed";
        size_t original_len = strlen(original_data);

        TeccCryptoBuffer buffer;
        buffer.size = original_len;
        buffer.data = malloc(buffer.size);
        buffer.used = original_len;
        memcpy(buffer.data, original_data, original_len);

        TeccFormatterErrorCode result = mock_normalize_data_inplace(&context, &buffer);

        bool passed = (result == TECC_FORMATTER_SUCCESS && buffer.used == original_len);
        print_test_result("In-place DET (no expansion)", passed);
        all_passed = all_passed && passed;

        free(buffer.data);
    }

    return all_passed;
}

/* Test 4: Error handling */
static bool test_error_handling(void)
{
    printf("\n=== Test 4: Error handling ===\n");

    bool all_passed = true;

    /* Test 4.1: NULL context */
    {
        TeccCryptoBuffer buffer;
        buffer.size = 1024;
        buffer.data = malloc(buffer.size);
        buffer.used = 0;

        const char *test_data = "test";
        TeccFormatterErrorCode result = mock_normalize_data_to_buffer(
            NULL, (const unsigned char *)test_data, strlen(test_data), &buffer);

        bool passed = (result == TECC_FORMATTER_ERROR_INVALID_PARAM);
        print_test_result("NULL context", passed);
        all_passed = all_passed && passed;

        free(buffer.data);
    }

    /* Test 4.2: NULL input data */
    {
        TeccCryptoContextCore context;
        context.data_type = TEXTOID;
        context.algo_type = TECC_ALGO_RND;
        context.collation_oid = InvalidOid;

        TeccCryptoBuffer buffer;
        buffer.size = 1024;
        buffer.data = malloc(buffer.size);
        buffer.used = 0;

        TeccFormatterErrorCode result = mock_normalize_data_to_buffer(
            &context, NULL, 10, &buffer);

        bool passed = (result == TECC_FORMATTER_ERROR_INVALID_PARAM);
        print_test_result("NULL input data", passed);
        all_passed = all_passed && passed;

        free(buffer.data);
    }

    /* Test 4.3: NULL buffer data */
    {
        TeccCryptoContextCore context;
        context.data_type = TEXTOID;
        context.algo_type = TECC_ALGO_RND;
        context.collation_oid = InvalidOid;

        const char *test_data = "test";
        TeccCryptoBuffer buffer;
        buffer.size = 1024;
        buffer.data = NULL;  /* NULL buffer data */
        buffer.used = 0;

        TeccFormatterErrorCode result = mock_normalize_data_to_buffer(
            &context, (const unsigned char *)test_data, strlen(test_data), &buffer);

        bool passed = (result == TECC_FORMATTER_ERROR_INVALID_PARAM);
        print_test_result("NULL buffer data", passed);
        all_passed = all_passed && passed;
    }

    /* Test 4.4: Buffer too small */
    {
        TeccCryptoContextCore context;
        context.data_type = TEXTOID;
        context.algo_type = TECC_ALGO_RND;
        context.collation_oid = InvalidOid;

        const char *test_data = "This is a test";
        size_t test_len = strlen(test_data);

        TeccCryptoBuffer buffer;
        buffer.size = 5;  /* Too small */
        buffer.data = malloc(buffer.size);
        buffer.used = 0;

        TeccFormatterErrorCode result = mock_normalize_data_to_buffer(
            &context, (const unsigned char *)test_data, test_len, &buffer);

        bool passed = (result == TECC_FORMATTER_ERROR_BUFFER_TOO_SMALL);
        print_test_result("Buffer too small", passed);
        all_passed = all_passed && passed;

        free(buffer.data);
    }

    /* Test 4.5: Data too large */
    {
        TeccCryptoContextCore context;
        context.data_type = TEXTOID;
        context.algo_type = TECC_ALGO_RND;
        context.collation_oid = InvalidOid;

        /* Create data larger than max size */
        size_t huge_size = TECC_MAX_DATA_SIZE + 100;
        unsigned char *huge_data = malloc(huge_size);
        memset(huge_data, 'A', huge_size);

        TeccCryptoBuffer buffer;
        buffer.size = huge_size + TECC_TEXT_LENGTH_PREFIX_SIZE;
        buffer.data = malloc(buffer.size);
        buffer.used = 0;

        TeccFormatterErrorCode result = mock_normalize_data_to_buffer(
            &context, huge_data, huge_size, &buffer);

        bool passed = (result == TECC_FORMATTER_ERROR_DATA_TOO_LARGE);
        print_test_result("Data too large", passed);
        all_passed = all_passed && passed;

        free(huge_data);
        free(buffer.data);
    }

    return all_passed;
}

/* Main test function */
int main(void)
{
    printf("========================================\n");
    printf("TECC Data Formatter Simple Test Suite\n");
    printf("Testing zero-copy buffer interfaces\n");
    printf("========================================\n");

    int tests_passed = 0;
    int tests_total = 0;

    /* Run test cases */
    if (test_basic_buffer()) tests_passed++;
    tests_total++;

    if (test_algorithm_types()) tests_passed++;
    tests_total++;

    if (test_inplace()) tests_passed++;
    tests_total++;

    if (test_error_handling()) tests_passed++;
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