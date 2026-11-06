/* Begin - ReqID:TECC4.0 */
/*
 * File: test_processor_error_handling.c
 *
 * Abstract:
 *   Unit tests for TECC processor module error handling.
 *   Tests the hierarchical error classification, error message mapping,
 *   and error recovery mechanisms.
 *
 * Authored by <wangxp@highgo.com>, 20260412.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   test_processor_error_handling.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* Define FRONTEND for frontend compilation */
#define FRONTEND 1

/* Include processor headers */
#include "../../tecc_processor/tecc_processor.h"
#include "../../tecc_processor/tecc_processor_types.h"
#include "../../tecc_processor/internal/tecc_processor_internal.h"

/* Test counters */
static int tests_passed = 0;
static int tests_failed = 0;

/* Mock statement data for testing */
static TeccStatementData test_stmt_data;

/* Mock PostgreSQL frontend functions for testing */
void *pg_malloc(size_t size)
{
    void *ptr = malloc(size);
    if (ptr == NULL)
    {
        fprintf(stderr, "Memory allocation failed: %zu bytes\n", size);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *pg_malloc0(size_t size)
{
    void *ptr = calloc(1, size);
    if (ptr == NULL)
    {
        fprintf(stderr, "Memory allocation failed: %zu bytes\n", size);
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void pg_free(void *ptr)
{
    if (ptr != NULL)
        free(ptr);
}

char *pg_strdup(const char *str)
{
    if (str == NULL)
        return NULL;

    char *new_str = malloc(strlen(str) + 1);
    if (new_str == NULL)
    {
        fprintf(stderr, "Memory allocation failed for string duplication\n");
        exit(EXIT_FAILURE);
    }
    strcpy(new_str, str);
    return new_str;
}

int pg_vsnprintf(char *str, size_t count, const char *fmt, va_list args)
{
    return vsnprintf(str, count, fmt, args);
}

int pg_fprintf(FILE *stream, const char *fmt, ...)
{
    va_list args;
    int result;

    va_start(args, fmt);
    result = vfprintf(stream, fmt, args);
    va_end(args);

    return result;
}

int pg_printf(const char *fmt, ...)
{
    va_list args;
    int result;

    va_start(args, fmt);
    result = vprintf(fmt, args);
    va_end(args);

    return result;
}

/* Note: List functions are already defined in included headers */

/* Initialize test statement data */
static void init_test_stmt_data(void)
{
    memset(&test_stmt_data, 0, sizeof(test_stmt_data));
    test_stmt_data.conn = NULL;
    test_stmt_data.query = "SELECT * FROM test_table";
    test_stmt_data.parse_tree = NULL;
    test_stmt_data.encrypted_columns = NULL;
    test_stmt_data.needs_rewrite = 0;
    test_stmt_data.error_code = TECC_PROCESSOR_SUCCESS;
    test_stmt_data.error_message[0] = '\0';
}

/* Test function: error message mapping */
static void test_error_message_mapping(void)
{
    printf("Testing error message mapping...\n");

    /* Test success case */
    const char *msg = tecc_processor_get_error_message(TECC_PROCESSOR_SUCCESS);
    if (strcmp(msg, "Success") == 0)
    {
        printf("  ✓ TECC_PROCESSOR_SUCCESS: %s\n", msg);
        tests_passed++;
    }
    else
    {
        printf("  ✗ TECC_PROCESSOR_SUCCESS: expected 'Success', got '%s'\n", msg);
        tests_failed++;
    }

    /* Test syntax error */
    msg = tecc_processor_get_error_message(TECC_PROCESSOR_SYNTAX_ERROR);
    if (strstr(msg, "Syntax") != NULL)
    {
        printf("  ✓ TECC_PROCESSOR_SYNTAX_ERROR: %s\n", msg);
        tests_passed++;
    }
    else
    {
        printf("  ✗ TECC_PROCESSOR_SYNTAX_ERROR: expected syntax error message, got '%s'\n", msg);
        tests_failed++;
    }

    /* Test semantic error */
    msg = tecc_processor_get_error_message(TECC_PROCESSOR_SEMANTIC_ERROR);
    if (strstr(msg, "Semantic") != NULL)
    {
        printf("  ✓ TECC_PROCESSOR_SEMANTIC_ERROR: %s\n", msg);
        tests_passed++;
    }
    else
    {
        printf("  ✗ TECC_PROCESSOR_SEMANTIC_ERROR: expected semantic error message, got '%s'\n", msg);
        tests_failed++;
    }

    /* Test data error */
    msg = tecc_processor_get_error_message(TECC_PROCESSOR_DATA_ERROR);
    if (strstr(msg, "Data") != NULL)
    {
        printf("  ✓ TECC_PROCESSOR_DATA_ERROR: %s\n", msg);
        tests_passed++;
    }
    else
    {
        printf("  ✗ TECC_PROCESSOR_DATA_ERROR: expected data error message, got '%s'\n", msg);
        tests_failed++;
    }

    /* Test unknown error */
    msg = tecc_processor_get_error_message(9999); /* Non-existent error code */
    if (strcmp(msg, "Unknown error") == 0)
    {
        printf("  ✓ Unknown error code: %s\n", msg);
        tests_passed++;
    }
    else
    {
        printf("  ✗ Unknown error code: expected 'Unknown error', got '%s'\n", msg);
        tests_failed++;
    }
}

/* Test function: error setting and clearing */
static void test_error_setting(void)
{
    printf("Testing error setting and clearing...\n");

    init_test_stmt_data();

    /* Test setting error */
    tecc_processor_set_error(&test_stmt_data, TECC_PROCESSOR_SYNTAX_ERROR,
                            "Test syntax error at line %d", 42);

    if (test_stmt_data.error_code == TECC_PROCESSOR_SYNTAX_ERROR &&
        strstr(test_stmt_data.error_message, "Test syntax error") != NULL)
    {
        printf("  ✓ Error set correctly: code=%d, message='%s'\n",
               test_stmt_data.error_code, test_stmt_data.error_message);
        tests_passed++;
    }
    else
    {
        printf("  ✗ Error not set correctly: code=%d, message='%s'\n",
               test_stmt_data.error_code, test_stmt_data.error_message);
        tests_failed++;
    }

    /* Test clearing error */
    tecc_processor_clear_error(&test_stmt_data);

    if (test_stmt_data.error_code == TECC_PROCESSOR_SUCCESS &&
        test_stmt_data.error_message[0] == '\0')
    {
        printf("  ✓ Error cleared correctly\n");
        tests_passed++;
    }
    else
    {
        printf("  ✗ Error not cleared: code=%d, message='%s'\n",
               test_stmt_data.error_code, test_stmt_data.error_message);
        tests_failed++;
    }

    /* Test setting error with NULL statement data (should not crash) */
    tecc_processor_set_error(NULL, TECC_PROCESSOR_SYNTAX_ERROR, "Test error");
    printf("  ✓ NULL statement data handled gracefully\n");
    tests_passed++;
}

/* Test function: recoverable error detection */
static void test_recoverable_errors(void)
{
    printf("Testing recoverable error detection...\n");

    /* Test recoverable errors */
    bool recoverable;

    recoverable = tecc_processor_is_recoverable_error(TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS);
    if (recoverable)
    {
        printf("  ✓ TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS is recoverable\n");
        tests_passed++;
    }
    else
    {
        printf("  ✗ TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS should be recoverable\n");
        tests_failed++;
    }

    recoverable = tecc_processor_is_recoverable_error(TECC_PROCESSOR_NEEDS_REWRITE);
    if (recoverable)
    {
        printf("  ✓ TECC_PROCESSOR_NEEDS_REWRITE is recoverable\n");
        tests_passed++;
    }
    else
    {
        printf("  ✗ TECC_PROCESSOR_NEEDS_REWRITE should be recoverable\n");
        tests_failed++;
    }

    recoverable = tecc_processor_is_recoverable_error(TECC_PROCESSOR_CACHE_ERROR);
    if (recoverable)
    {
        printf("  ✓ TECC_PROCESSOR_CACHE_ERROR is recoverable\n");
        tests_passed++;
    }
    else
    {
        printf("  ✗ TECC_PROCESSOR_CACHE_ERROR should be recoverable\n");
        tests_failed++;
    }

    /* Test non-recoverable errors */
    recoverable = tecc_processor_is_recoverable_error(TECC_PROCESSOR_SYNTAX_ERROR);
    if (!recoverable)
    {
        printf("  ✓ TECC_PROCESSOR_SYNTAX_ERROR is not recoverable\n");
        tests_passed++;
    }
    else
    {
        printf("  ✗ TECC_PROCESSOR_SYNTAX_ERROR should not be recoverable\n");
        tests_failed++;
    }

    recoverable = tecc_processor_is_recoverable_error(TECC_PROCESSOR_MEMORY_ERROR);
    if (!recoverable)
    {
        printf("  ✓ TECC_PROCESSOR_MEMORY_ERROR is not recoverable\n");
        tests_passed++;
    }
    else
    {
        printf("  ✗ TECC_PROCESSOR_MEMORY_ERROR should not be recoverable\n");
        tests_failed++;
    }
}

/* Test function: error recovery attempt */
static void test_error_recovery(void)
{
    printf("Testing error recovery attempt...\n");

    init_test_stmt_data();

    /* Test recovery with recoverable error */
    tecc_processor_set_error(&test_stmt_data, TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS,
                            "No encrypted columns found");

    bool recovered = tecc_processor_attempt_recovery(&test_stmt_data,
                                                    TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS);

    if (recovered && test_stmt_data.error_code == TECC_PROCESSOR_SUCCESS)
    {
        printf("  ✓ TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS recovered successfully\n");
        tests_passed++;
    }
    else
    {
        printf("  ✗ TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS recovery failed: recovered=%d, code=%d\n",
               recovered, test_stmt_data.error_code);
        tests_failed++;
    }

    /* Test recovery with non-recoverable error */
    init_test_stmt_data();
    tecc_processor_set_error(&test_stmt_data, TECC_PROCESSOR_SYNTAX_ERROR,
                            "Syntax error");

    recovered = tecc_processor_attempt_recovery(&test_stmt_data,
                                               TECC_PROCESSOR_SYNTAX_ERROR);

    if (!recovered && test_stmt_data.error_code == TECC_PROCESSOR_SYNTAX_ERROR)
    {
        printf("  ✓ TECC_PROCESSOR_SYNTAX_ERROR correctly not recovered\n");
        tests_passed++;
    }
    else
    {
        printf("  ✗ TECC_PROCESSOR_SYNTAX_ERROR recovery status incorrect: recovered=%d, code=%d\n",
               recovered, test_stmt_data.error_code);
        tests_failed++;
    }

    /* Test recovery with NULL statement data */
    recovered = tecc_processor_attempt_recovery(NULL, TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS);
    if (!recovered)
    {
        printf("  ✓ NULL statement data handled gracefully in recovery\n");
        tests_passed++;
    }
    else
    {
        printf("  ✗ NULL statement data recovery should return false\n");
        tests_failed++;
    }
}

/* Test function: error setting with recovery */
static void test_error_setting_with_recovery(void)
{
    printf("Testing error setting with recovery...\n");

    init_test_stmt_data();

    /* Test with recoverable error */
    bool recovered = tecc_processor_set_error_with_recovery(&test_stmt_data,
                                                           TECC_PROCESSOR_NEEDS_REWRITE,
                                                           "SQL needs rewrite for encrypted columns");

    if (recovered && test_stmt_data.error_code == TECC_PROCESSOR_SUCCESS)
    {
        printf("  ✓ Error with recovery: TECC_PROCESSOR_NEEDS_REWRITE recovered\n");
        tests_passed++;
    }
    else
    {
        printf("  ✗ Error with recovery failed: recovered=%d, code=%d\n",
               recovered, test_stmt_data.error_code);
        tests_failed++;
    }

    /* Test with non-recoverable error */
    init_test_stmt_data();
    recovered = tecc_processor_set_error_with_recovery(&test_stmt_data,
                                                      TECC_PROCESSOR_SYNTAX_ERROR,
                                                      "Syntax error at line %d", 10);

    if (!recovered && test_stmt_data.error_code == TECC_PROCESSOR_SYNTAX_ERROR &&
        strstr(test_stmt_data.error_message, "Syntax error") != NULL)
    {
        printf("  ✓ Error with recovery: TECC_PROCESSOR_SYNTAX_ERROR not recovered (as expected)\n");
        tests_passed++;
    }
    else
    {
        printf("  ✗ Error with recovery incorrect: recovered=%d, code=%d, message='%s'\n",
               recovered, test_stmt_data.error_code, test_stmt_data.error_message);
        tests_failed++;
    }

    /* Test with NULL statement data */
    recovered = tecc_processor_set_error_with_recovery(NULL,
                                                      TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS,
                                                      "Test error");
    if (!recovered)
    {
        printf("  ✓ NULL statement data handled gracefully in set_error_with_recovery\n");
        tests_passed++;
    }
    else
    {
        printf("  ✗ NULL statement data should return false\n");
        tests_failed++;
    }
}

/* Main test runner */
int main(void)
{
    printf("========================================\n");
    printf("TECC Processor Error Handling Unit Tests\n");
    printf("========================================\n\n");

    /* Run all test functions */
    test_error_message_mapping();
    printf("\n");

    test_error_setting();
    printf("\n");

    test_recoverable_errors();
    printf("\n");

    test_error_recovery();
    printf("\n");

    test_error_setting_with_recovery();
    printf("\n");

    /* Print summary */
    printf("========================================\n");
    printf("Test Summary:\n");
    printf("  Tests passed: %d\n", tests_passed);
    printf("  Tests failed: %d\n", tests_failed);
    printf("  Total tests:  %d\n", tests_passed + tests_failed);
    printf("========================================\n");

    if (tests_failed == 0)
    {
        printf("All tests passed! ✓\n");
        return EXIT_SUCCESS;
    }
    else
    {
        printf("Some tests failed. ✗\n");
        return EXIT_FAILURE;
    }
}

/* End - ReqID:TECC4.0 */