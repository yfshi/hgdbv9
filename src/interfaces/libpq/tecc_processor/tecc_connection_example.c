/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_connection_example.c
 *
 * Abstract:
 *   Example implementation showing how TECC processor module should
 *   manage database connections for data fetcher and cache modules.
 *
 * Authored by <wangxp@highgo.com>, 20260414.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/tecc_connection_example.c
 */

#include "expr_processor.h"
#include "libpq-fe.h"  /* For PQdb function */
#include "../tecc_cache/tecc_metadata_cache.h"
#include "../tecc_data_fetcher/include/tecc_data_fetcher.h"
#include <stdio.h>

/*
 * tecc_processor_example_with_cache - Example showing connection management
 *                                     when using cache module
 *
 * stmt_data: Statement data containing database connection
 * table_name: Table name to check for encrypted columns
 * column_name: Column name to check
 *
 * Returns: 0 on success, -1 on error
 *
 * This example shows how the processor module should manage connections
 * when interacting with cache and data fetcher modules.
 */
int
tecc_processor_example_with_cache(TeccStatementData *stmt_data,
                                  const char *table_name,
                                  const char *column_name)
{
    TeccCacheEncryptedColumnResult encrypted_column = NULL;
    TeccState *state = NULL;
    const char *database_name = NULL;
    const char *schema_name = NULL;

    if (stmt_data == NULL || stmt_data->conn == NULL) {
        fprintf(stderr, "Error: Invalid statement data or connection\n");
        return -1;
    }

    /* Step 1: Get TECC state from connection */
    state = tecc_get_state(stmt_data->conn);
    if (state == NULL || !state->initialized) {
        fprintf(stderr, "Error: TECC state not initialized\n");
        return -1;
    }

    /* Step 2: Get database name from connection */
    database_name = PQdb(stmt_data->conn);
    if (database_name == NULL || database_name[0] == '\0') {
        fprintf(stderr, "Error: Cannot get database name from connection\n");
        return -1;
    }

    /* Step 3: Get current schema from connection */
    schema_name = tecc_cache_get_guc_param(state, "current_schema");
    if (schema_name == NULL || schema_name[0] == '\0') {
        fprintf(stderr, "Error: Cannot get current schema from connection\n");
        return -1;
    }

    /* Step 4: Set current connection for data fetcher module */
    tecc_set_current_connection(stmt_data->conn);

    /* Step 5: Check if cache needs to be loaded */
    /* In a real implementation, we would check cache state here */

    /* Step 6: Query cache for encrypted column metadata using new interface */
    encrypted_column = tecc_cache_get_encrypted_column_by_name(state,
                                                              database_name,
                                                              schema_name,
                                                              table_name,
                                                              column_name);

    if (encrypted_column == NULL) {
        /* Cache miss - column is not encrypted or not in cache */
        printf("Column '%s.%s' is not encrypted or not in cache\n",
               table_name ? table_name : "NULL",
               column_name ? column_name : "NULL");
    } else {
        /* Cache hit - column is encrypted */
        printf("Column '%s.%s' is encrypted with DEK ID: %u\n",
               table_name ? table_name : "NULL",
               column_name ? column_name : "NULL",
               encrypted_column->column_key_id);

        /* Example: Fetch KEK metadata for this column's DEK */
        /* In a real implementation, we would:
         * 1. Get DEK metadata from cache
         * 2. Get KEK metadata using data fetcher
         * 3. Perform encryption/decryption operations
         */
    }

    /* Step 4: Clear connection setting */
    tecc_set_current_connection(NULL);

    return 0;
}

/*
 * tecc_processor_example_direct_fetch - Example showing direct data fetcher usage
 *
 * stmt_data: Statement data containing database connection
 * kek_name: KEK name to fetch
 * kek_namespace: KEK namespace
 *
 * Returns: 0 on success, -1 on error
 *
 * This example shows how to directly use data fetcher without cache.
 */
int
tecc_processor_example_direct_fetch(TeccStatementData *stmt_data,
                                    const char *kek_name,
                                    const char *kek_namespace)
{
    TeccKekMetadata *kek_metadata = NULL;
    TeccFetcherErrorCode error_code;

    if (stmt_data == NULL || stmt_data->conn == NULL) {
        fprintf(stderr, "Error: Invalid statement data or connection\n");
        return -1;
    }

    if (kek_name == NULL) {
        fprintf(stderr, "Error: KEK name cannot be NULL\n");
        return -1;
    }

    /* Step 1: Set current connection for data fetcher module */
    tecc_set_current_connection(stmt_data->conn);

    /* Step 2: Directly fetch KEK metadata using data fetcher */
    error_code = tecc_fetch_kek_metadata(state, kek_name, kek_namespace, NULL, &kek_metadata);

    if (error_code != TECC_FETCHER_OK) {
        fprintf(stderr, "Error fetching KEK metadata: %s\n",
                tecc_fetcher_error_message(error_code));
        tecc_set_current_connection(NULL);
        return -1;
    }

    /* Step 3: Process the metadata */
    if (kek_metadata != NULL) {
        printf("Successfully fetched KEK: %s\n", kek_metadata->key_name);
        printf("  Namespace: %s\n", kek_metadata->key_namespace ? kek_metadata->key_namespace : "NULL");
        printf("  Owner: %s\n", kek_metadata->key_owner ? kek_metadata->key_owner : "NULL");

        /* Free the metadata */
        tecc_free_kek_metadata(kek_metadata);
    }

    /* Step 4: Clear connection setting */
    tecc_set_current_connection(NULL);

    return 0;
}

/*
 * tecc_processor_load_cache - Example showing cache loading with connection management
 *
 * stmt_data: Statement data containing database connection
 *
 * Returns: 0 on success, -1 on error
 *
 * This example shows how to load cache with proper connection management.
 */
int
tecc_processor_load_cache(TeccStatementData *stmt_data)
{
    if (stmt_data == NULL || stmt_data->conn == NULL) {
        fprintf(stderr, "Error: Invalid statement data or connection\n");
        return -1;
    }

    /* Step 1: Get TECC state from connection */
    TeccState *state = tecc_get_state(stmt_data->conn);
    if (state == NULL) {
        fprintf(stderr, "Error: TECC state not available\n");
        return -1;
    }

    /* Step 2: Initialize cache with connection */
    tecc_cache_init(state);

    /* Step 3: Set current connection for data fetcher */
    tecc_set_current_connection(stmt_data->conn);

    /* Step 4: Load all metadata into cache */
    printf("Loading metadata cache...\n");
    if (tecc_cache_load_all_metadata(state) != 0) {
        fprintf(stderr, "Error loading metadata cache\n");
        tecc_set_current_connection(NULL);
        return -1;
    }

    /* Step 4: Clear connection setting */
    tecc_set_current_connection(NULL);

    printf("Metadata cache loaded successfully\n");
    return 0;
}

/* End - ReqID:TECC4.0 */