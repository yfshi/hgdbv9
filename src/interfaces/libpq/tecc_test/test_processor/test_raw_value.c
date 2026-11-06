/* Begin - ReqID:TECC4.0 */
/*
 * File: test_raw_value.c
 *
 * Abstract:
 *   Test program for raw_value data structure.
 *
 * Authored by <wangxp@highgo.com>, 20260411.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   test_raw_value.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Define FRONTEND for frontend compilation */
#define FRONTEND 1

/* Include raw_value header */
#include "../../tecc_processor/raw_value.h"

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
    (void)fmt;
    /* Do nothing for testing */
}

/* Define debug level constants */
#define TECC_LOG_DEBUG1 3
#define TECC_LOG_ERROR 0

/* Test functions */
void test_create_from_string(void)
{
    printf("Testing tecc_raw_value_create_from_string...\n");

    TeccRawValue *value = tecc_raw_value_create_from_string("test string", 11);
    if (value)
    {
        printf("  Created string value: type=%d, length=%zu\n",
               value->type, value->length);
        tecc_raw_value_destroy(value);
        printf("  PASS\n");
    }
    else
    {
        printf("  FAIL: Could not create value\n");
    }
}

void test_create_from_int(void)
{
    printf("Testing tecc_raw_value_create_from_int...\n");

    TeccRawValue *value = tecc_raw_value_create_from_int(12345);
    if (value)
    {
        printf("  Created int value: type=%d, value=%d\n",
               value->type, value->original.integer_value);
        tecc_raw_value_destroy(value);
        printf("  PASS\n");
    }
    else
    {
        printf("  FAIL: Could not create value\n");
    }
}

void test_create_null(void)
{
    printf("Testing tecc_raw_value_create_null...\n");

    TeccRawValue *value = tecc_raw_value_create_null();
    if (value)
    {
        printf("  Created null value: is_null=%d\n", value->is_null);
        tecc_raw_value_destroy(value);
        printf("  PASS\n");
    }
    else
    {
        printf("  FAIL: Could not create value\n");
    }
}

void test_copy(void)
{
    printf("Testing tecc_raw_value_copy...\n");

    TeccRawValue *original = tecc_raw_value_create_from_string("original", 8);
    if (!original)
    {
        printf("  FAIL: Could not create original value\n");
        return;
    }

    TeccRawValue *copy = tecc_raw_value_copy(original);
    if (copy)
    {
        printf("  Copied value: original type=%d, copy type=%d\n",
               original->type, copy->type);
        tecc_raw_value_destroy(copy);
        printf("  PASS\n");
    }
    else
    {
        printf("  FAIL: Could not copy value\n");
    }

    tecc_raw_value_destroy(original);
}

void test_to_string(void)
{
    printf("Testing tecc_raw_value_to_string...\n");

    TeccRawValue *value = tecc_raw_value_create_from_string("test", 4);
    if (!value)
    {
        printf("  FAIL: Could not create value\n");
        return;
    }

    char *str = tecc_raw_value_to_string(value);
    if (str)
    {
        printf("  String representation: %s\n", str);
        free(str);
        printf("  PASS\n");
    }
    else
    {
        printf("  FAIL: Could not convert to string\n");
    }

    tecc_raw_value_destroy(value);
}

int main(void)
{
    printf("=== Testing raw_value data structure ===\n\n");

    test_create_from_string();
    printf("\n");

    test_create_from_int();
    printf("\n");

    test_create_null();
    printf("\n");

    test_copy();
    printf("\n");

    test_to_string();
    printf("\n");

    printf("=== All tests completed ===\n");
    return 0;
}
/* End - ReqID:TECC4.0 */