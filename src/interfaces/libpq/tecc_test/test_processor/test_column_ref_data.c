/* Begin - ReqID:TECC4.0 */
/*
 * File: test_column_ref_data.c
 *
 * Abstract:
 *   Test program for column_ref_data data structure.
 *
 * Authored by <wangxp@highgo.com>, 20260411.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   test_column_ref_data.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Define FRONTEND for frontend compilation */
#define FRONTEND 1

/* Include column_ref_data header */
#include "../../tecc_processor/column_ref_data.h"

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
void test_create_and_init(void)
{
    printf("Testing tecc_column_ref_data_create and tecc_column_ref_data_init...\n");

    TeccColumnRefData *data = tecc_column_ref_data_create();
    if (data)
    {
        tecc_column_ref_data_init(data, "test_catalog", "test_schema", "test_table", "test_column");

        printf("  Created column ref: catalog=%s, schema=%s, table=%s, column=%s\n",
               data->catalog_name, data->schema_name, data->table_name, data->column_name);
        printf("  FQDN: %s\n", tecc_column_ref_data_get_fqdn(data));
        printf("  Is FQDN: %s\n", tecc_column_ref_data_is_fqdn(data) ? "true" : "false");

        tecc_column_ref_data_destroy(data);
        printf("  PASS\n");
    }
    else
    {
        printf("  FAIL: Could not create column ref data\n");
    }
}

void test_encryption_status(void)
{
    printf("Testing encryption status functions...\n");

    TeccColumnRefData *data = tecc_column_ref_data_create();
    if (data)
    {
        tecc_column_ref_data_init(data, "cat", "sch", "tbl", "col");

        printf("  Initial encrypted status: %s\n",
               tecc_column_ref_data_is_encrypted(data) ? "true" : "false");

        tecc_column_ref_data_set_encrypted(data, true);
        printf("  After set encrypted: %s\n",
               tecc_column_ref_data_is_encrypted(data) ? "true" : "false");

        tecc_column_ref_data_destroy(data);
        printf("  PASS\n");
    }
    else
    {
        printf("  FAIL: Could not create column ref data\n");
    }
}

void test_oid_functions(void)
{
    printf("Testing OID functions...\n");

    TeccColumnRefData *data = tecc_column_ref_data_create();
    if (data)
    {
        tecc_column_ref_data_init(data, "cat", "sch", "tbl", "col");

        tecc_column_ref_data_set_column_oid(data, 12345);
        tecc_column_ref_data_set_table_oid(data, 67890);

        printf("  Column OID: %u\n", tecc_column_ref_data_get_column_oid(data));
        printf("  Table OID: %u\n", tecc_column_ref_data_get_table_oid(data));

        tecc_column_ref_data_destroy(data);
        printf("  PASS\n");
    }
    else
    {
        printf("  FAIL: Could not create column ref data\n");
    }
}

void test_copy_function(void)
{
    printf("Testing tecc_column_ref_data_copy...\n");

    TeccColumnRefData *original = tecc_column_ref_data_create();
    if (!original)
    {
        printf("  FAIL: Could not create original\n");
        return;
    }

    tecc_column_ref_data_init(original, "cat1", "sch1", "tbl1", "col1");
    tecc_column_ref_data_set_encrypted(original, true);
    tecc_column_ref_data_set_column_oid(original, 100);
    tecc_column_ref_data_set_table_oid(original, 200);

    TeccColumnRefData *copy = tecc_column_ref_data_copy(original);
    if (copy)
    {
        printf("  Original FQDN: %s\n", tecc_column_ref_data_get_fqdn(original));
        printf("  Copy FQDN: %s\n", tecc_column_ref_data_get_fqdn(copy));
        printf("  Are equal: %s\n",
               tecc_column_ref_data_equal(original, copy) ? "true" : "false");

        tecc_column_ref_data_destroy(copy);
        printf("  PASS\n");
    }
    else
    {
        printf("  FAIL: Could not copy\n");
    }

    tecc_column_ref_data_destroy(original);
}

void test_to_string(void)
{
    printf("Testing tecc_column_ref_data_to_string...\n");

    TeccColumnRefData *data = tecc_column_ref_data_create();
    if (!data)
    {
        printf("  FAIL: Could not create data\n");
        return;
    }

    tecc_column_ref_data_init(data, "mycat", "mysch", "mytbl", "mycol");
    tecc_column_ref_data_set_encrypted(data, true);
    tecc_column_ref_data_set_column_oid(data, 111);
    tecc_column_ref_data_set_table_oid(data, 222);
    tecc_column_ref_data_set_location(data, 50);

    char *str = tecc_column_ref_data_to_string(data);
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

    tecc_column_ref_data_destroy(data);
}

int main(void)
{
    printf("=== Testing column_ref_data data structure ===\n\n");

    test_create_and_init();
    printf("\n");

    test_encryption_status();
    printf("\n");

    test_oid_functions();
    printf("\n");

    test_copy_function();
    printf("\n");

    test_to_string();
    printf("\n");

    printf("=== All tests completed ===\n");
    return 0;
}
/* End - ReqID:TECC4.0 */