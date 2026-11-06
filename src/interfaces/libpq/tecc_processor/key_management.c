/* Begin - ReqID:TECC4.0 */
/*
 * File: key_management.c
 *
 * Abstract:
 *   Key management functions for TECC processor module.
 *   Provides functions for retrieving and validating KEK/DEK names
 *   to improve security in import/export operations.
 *
 * Authored by <wangxp@highgo.com>, 20260418.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/key_management.c
 */

#include "postgres_fe.h"
#include "libpq-fe.h"  /* For PQdb function */
#include "internal/tecc_processor_internal.h"
#include "../tecc_libpq_integration/fe-tecc-integration.h"
#include "../tecc_cache/tecc_metadata_cache.h"
#include <string.h>
#include <time.h>

/* Include standard C headers for memory management */
#include "common/fe_memutils.h"  /* For pg_malloc, pg_free */

/*
 * tecc_processor_get_key_names - Get KEK and DEK names for a column
 *
 * Parameters:
 *   conn - Database connection
 *   table_name - Table name
 *   column_name - Column name
 *   kek_name - Output parameter for KEK name (must be freed by caller)
 *   dek_name - Output parameter for DEK name (must be freed by caller)
 *
 * Returns:
 *   0 on success, negative error code on failure
 */
int
tecc_processor_get_key_names(PGconn *conn,
                            const char *table_name,
                            const char *column_name,
                            char **kek_name,
                            char **dek_name)
{
    TeccState *state = NULL;
    TeccColumnCacheItem *column_item = NULL;
    TeccDekCacheItem *dek_item = NULL;
    TeccKekCacheItem *kek_item = NULL;
    int ret = 0;
    const char *database_name = NULL;
    const char *schema_name = NULL;

    /* Validate parameters */
    if (conn == NULL || table_name == NULL || column_name == NULL ||
        kek_name == NULL || dek_name == NULL) {
        return TECC_PROCESSOR_INTERNAL_INVALID_ARGUMENT;
    }

    /* Initialize output parameters */
    *kek_name = NULL;
    *dek_name = NULL;

    /* Get TECC state from connection */
    state = tecc_get_state(conn);
    if (state == NULL || !state->initialized) {
        return TECC_PROCESSOR_INTERNAL_NOT_FOUND;
    }

    /* Get database name from connection */
    database_name = PQdb(conn);
    if (database_name == NULL || database_name[0] == '\0') {
        /* 无法获取数据库名，返回错误 */
        return TECC_PROCESSOR_INTERNAL_NOT_FOUND;
    }

    /* Get current schema from connection */
    schema_name = tecc_cache_get_guc_param(state, "current_schema");
    if (schema_name == NULL || schema_name[0] == '\0') {
        /* 无法获取schema名，返回错误 */
        return TECC_PROCESSOR_INTERNAL_NOT_FOUND;
    }

    /* Get column metadata from cache */
    column_item = tecc_cache_get_encrypted_column_by_name(state,
                                                         database_name,
                                                         schema_name,
                                                         table_name,
                                                         column_name);
    if (column_item == NULL) {
        /* Column not found or not encrypted */
        return TECC_PROCESSOR_INTERNAL_NOT_FOUND;
    }

    /* Get DEK metadata using column's dek_id, fallback to name */
    if (column_item->dek_id != InvalidOid) {
        dek_item = tecc_cache_get_dek_by_oid(state, column_item->dek_id);
        if (dek_item == NULL && column_item->dek_name != NULL)
            dek_item = tecc_cache_get_dek_by_name_search_path(state, column_item->dek_name);
        if (dek_item == NULL) {
            /* DEK not found in cache */
            ret = TECC_PROCESSOR_INTERNAL_NOT_FOUND;
            goto cleanup;
        }

        /* Get DEK name from DEK metadata */
        if (dek_item->key_name != NULL) {
            *dek_name = pg_strdup(dek_item->key_name);
            if (*dek_name == NULL) {
                ret = TECC_PROCESSOR_INTERNAL_MEMORY_ERROR;
                goto cleanup;
            }
        } else {
            /* DEK name not found in DEK metadata */
            ret = TECC_PROCESSOR_INTERNAL_NOT_FOUND;
            goto cleanup;
        }
    } else {
        /* DEK ID not found in column metadata */
        ret = TECC_PROCESSOR_INTERNAL_NOT_FOUND;
        goto cleanup;
    }

    /* Get KEK metadata using DEK's kek_oid, fallback to name */
    kek_item = tecc_cache_get_kek_by_oid(state, dek_item->kek_oid);
    if (kek_item == NULL && dek_item->kek_name != NULL)
        kek_item = tecc_cache_get_kek_by_name_search_path(state, dek_item->kek_name);
    if (kek_item == NULL) {
        /* KEK not found in cache */
        ret = TECC_PROCESSOR_INTERNAL_NOT_FOUND;
        goto cleanup;
    }

    /* Get KEK name */
    if (kek_item->key_name != NULL) {
        *kek_name = pg_strdup(kek_item->key_name);
        if (*kek_name == NULL) {
            ret = TECC_PROCESSOR_INTERNAL_MEMORY_ERROR;
            goto cleanup;
        }
    } else {
        /* KEK name not found */
        ret = TECC_PROCESSOR_INTERNAL_NOT_FOUND;
        goto cleanup;
    }

    /* Success */
    ret = 0;

cleanup:
    /* Clean up on error */
    if (ret != 0) {
        if (*kek_name != NULL) {
            pg_free(*kek_name);
            *kek_name = NULL;
        }
        if (*dek_name != NULL) {
            pg_free(*dek_name);
            *dek_name = NULL;
        }
    }

    return ret;
}

/*
 * tecc_processor_validate_key_names - Validate KEK and DEK names
 *
 * Parameters:
 *   conn - Database connection
 *   kek_name - KEK name to validate
 *   dek_name - DEK name to validate
 *
 * Returns:
 *   true if keys are valid and accessible, false otherwise
 */
bool
tecc_processor_validate_key_names(PGconn *conn,
                                 const char *kek_name,
                                 const char *dek_name)
{
    TeccState *state = NULL;
    TeccDekCacheItem *dek_item = NULL;
    TeccKekCacheItem *kek_item = NULL;

    /* Validate parameters */
    if (conn == NULL || kek_name == NULL || dek_name == NULL) {
        return false;
    }

    /* Get TECC state from connection */
    state = tecc_get_state(conn);
    if (state == NULL || !state->initialized) {
        return false;
    }

    /* Check if KEK exists and is valid */
    kek_item = tecc_cache_get_kek_by_name_search_path(state, kek_name);
    if (kek_item == NULL)
        return false;

    /* Check KEK status */
    if (kek_item->status != 1) /* Assuming 1 means active */
        return false;

    /* Check if DEK exists and is valid */
    dek_item = tecc_cache_get_dek_by_name_search_path(state, dek_name);
    if (dek_item == NULL)
        return false;

    /* Check DEK status */
    if (dek_item->status != 1) /* Assuming 1 means active */
        return false;

    /* Verify that DEK is associated with the correct KEK */
    if (dek_item->kek_oid != kek_item->kek_oid)
        return false;

    /* All checks passed */
    return true;
}

/*
 * tecc_processor_get_kek_name_for_dek - Get KEK name for a given DEK
 *
 * Parameters:
 *   conn - Database connection
 *   dek_name - DEK name
 *   kek_name - Output parameter for KEK name (must be freed by caller)
 *
 * Returns:
 *   0 on success, negative error code on failure
 */
int
tecc_processor_get_kek_name_for_dek(PGconn *conn,
                                   const char *dek_name,
                                   char **kek_name)
{
    TeccState *state = NULL;
    TeccDekCacheItem *dek_item = NULL;
    TeccKekCacheItem *kek_item = NULL;

    /* Validate parameters */
    if (conn == NULL || dek_name == NULL || kek_name == NULL) {
        return TECC_PROCESSOR_INTERNAL_INVALID_ARGUMENT;
    }

    /* Initialize output parameter */
    *kek_name = NULL;

    /* Get TECC state from connection */
    state = tecc_get_state(conn);
    if (state == NULL || !state->initialized) {
        return TECC_PROCESSOR_INTERNAL_NOT_FOUND;
    }

    /* Get DEK metadata */
    dek_item = tecc_cache_get_dek_by_name_search_path(state, dek_name);
    if (dek_item == NULL) {
        /* DEK not found */
        return TECC_PROCESSOR_INTERNAL_NOT_FOUND;
    }

    /* Get KEK metadata, fallback to name */
    kek_item = tecc_cache_get_kek_by_oid(state, dek_item->kek_oid);
    if (kek_item == NULL && dek_item->kek_name != NULL)
        kek_item = tecc_cache_get_kek_by_name_search_path(state, dek_item->kek_name);
    if (kek_item == NULL) {
        /* KEK not found */
        return TECC_PROCESSOR_INTERNAL_NOT_FOUND;
    }

    /* Get KEK name */
    if (kek_item->key_name != NULL) {
        *kek_name = pg_strdup(kek_item->key_name);
        if (*kek_name == NULL) {
            return TECC_PROCESSOR_INTERNAL_MEMORY_ERROR;
        }
    } else {
        /* KEK name not found */
        return TECC_PROCESSOR_INTERNAL_NOT_FOUND;
    }

    return 0;
}

/* End - ReqID:TECC4.0 */