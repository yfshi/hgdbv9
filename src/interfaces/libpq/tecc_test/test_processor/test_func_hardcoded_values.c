/* Begin - ReqID:TECC4.0 */
/*
 * File: test_func_hardcoded_values.c
 *
 * Abstract:
 *   Test program for func_hardcoded_values data structure.
 *
 * Authored by <wangxp@highgo.com>, 20260411.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   test_func_hardcoded_values.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Define FRONTEND for frontend compilation */
#define FRONTEND 1

/* Include func_hardcoded_values header */
#include "../../tecc_processor/func_hardcoded_values.h"

/* Simple memory allocation functions for testing */
void *test_palloc(size_t size)
{
    void *ptr = malloc(size);
    if (ptr)
        memset(ptr, 0, size);
    return ptr;
}

void *test_palloc0(size_t size)
{
    return calloc(1, size);
}

void test_pfree(void *ptr)
{
    free(ptr);
}

char *test_pstrdup(const char *str)
{
    if (!str) return NULL;
    return strdup(str);
}

/* Override tecc_processor memory functions */
#define tecc_processor_palloc test_palloc
#define tecc_processor_palloc0 test_palloc0
#define tecc_processor_pfree test_pfree
#define tecc_processor_pstrdup test_pstrdup

/* Simple debug function */
void tecc_processor_debug(int level, const char *fmt, ...)
{
    (void)level;
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

/* Define debug level constants */
#define TECC_LOG_DEBUG1 3
#define TECC_LOG_INFO 2
#define TECC_LOG_WARNING 1
#define TECC_LOG_ERROR 0

/* Test helper: Create a simple options list */
List *create_test_options(void)
{
    /* In a real test, we would create proper DefElem nodes */
    /* For now, return NULL to test error handling */
    return NULL;
}

/* Test functions */
void test_contains_encrypted_column_reference(void)
{
    printf("Testing contains_encrypted_column_reference...\n");

    /* Test cases */
    const char *test_cases[] = {
        "SELECT * FROM users WHERE id = 1",  /* No encrypted type */
        "SELECT name_enc FROM users",        /* Contains _enc */
        "INSERT INTO table (int4_enc) VALUES (1)", /* Contains int4_enc */
        "UPDATE table SET varchar_enc = 'test'", /* Contains varchar_enc */
        "CREATE TABLE t (id int, data text_enc)" /* Contains text_enc */
    };

    const bool expected_results[] = {
        false, true, true, true, true
    };

    size_t i;
    for (i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        /* Note: We can't directly call the static function */
        /* This is just to show the test structure */
        printf("  Test %zu: %s\n", i + 1,
               expected_results[i] ? "Should detect encrypted column" :
                                     "Should not detect encrypted column");
    }

    printf("  (Note: Function is static, cannot test directly)\n");
}

void test_language_detection(void)
{
    printf("Testing language detection functions...\n");

    /* Test is_plpgsql_language */
    const char *plpgsql_langs[] = {"plpgsql", "plpgsqlu", "sql", "other"};
    const bool plpgsql_expected[] = {true, true, false, false};

    /* Test is_sql_language */
    const char *sql_langs[] = {"sql", "sqlu", "plpgsql", "other"};
    const bool sql_expected[] = {true, true, false, false};

    size_t i;
    printf("  PL/pgSQL language detection:\n");
    for (i = 0; i < sizeof(plpgsql_langs) / sizeof(plpgsql_langs[0]); i++) {
        /* Note: We can't directly call the static functions */
        printf("    %s: %s\n", plpgsql_langs[i],
               plpgsql_expected[i] ? "should be PL/pgSQL" : "should not be PL/pgSQL");
    }

    printf("  SQL language detection:\n");
    for (i = 0; i < sizeof(sql_langs) / sizeof(sql_langs[0]); i++) {
        printf("    %s: %s\n", sql_langs[i],
               sql_expected[i] ? "should be SQL" : "should not be SQL");
    }

    printf("  (Note: Functions are static, cannot test directly)\n");
}

void test_extract_sql_from_body(void)
{
    printf("Testing extract_sql_from_body...\n");

    const char *test_body = "BEGIN\n  -- This is a comment\n  SELECT * FROM users;\nEND;";
    size_t body_size = strlen(test_body);

    /* Note: We can't directly call the static function */
    printf("  Test body: %s\n", test_body);
    printf("  Body size: %zu\n", body_size);
    printf("  (Note: Function is static, cannot test directly)\n");
}

void test_make_sql_body(void)
{
    printf("Testing tecc_func_hardcoded_values_make_sql_body...\n");

    const char *test_body = "SELECT * FROM users WHERE id = 1;";
    /* Note: These variables are declared but not used in current test stub */
    /* size_t body_size = strlen(test_body); */
    /* size_t body_location = 100; */
    /* size_t bodies_size = 0; */

    /* Note: This function requires contains_encrypted_column_reference */
    printf("  Test body: %s\n", test_body);
    printf("  (Note: Requires full implementation to test)\n");
}

int main(void)
{
    printf("=== Testing func_hardcoded_values module ===\n\n");

    test_contains_encrypted_column_reference();
    printf("\n");

    test_language_detection();
    printf("\n");

    test_extract_sql_from_body();
    printf("\n");

    test_make_sql_body();
    printf("\n");

    printf("=== All tests completed ===\n");
    printf("Note: Many functions are static and require integration testing\n");
    printf("with the full processor module to test properly.\n");

    return 0;
}
/* End - ReqID:TECC4.0 */