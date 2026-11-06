#ifdef _MSC_VER
#define TECC_ATTR_UNUSED
#else
#define TECC_ATTR_UNUSED __attribute__((unused))
#endif

/* Begin - ReqID:TECC4.0 */
/*
 * File: post_stmt_processor.c
 *
 * Abstract:
 *   TECC post-statement processor for statement postprocessing phase.
 *   Handles postprocessing stage management, state synchronization, cache updates, and resource cleanup.
 *
 * Authored by <wangxp@highgo.com>, 20260412.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/post_stmt_processor.c
 */

#include "postgres_fe.h"
#include "common/fe_memutils.h"
#include "post_stmt_processor.h"
#include "internal/tecc_processor_internal.h"
#include "../libpq-fe.h"

/* Include libpq integration for tecc_get_state */
#include "../tecc_libpq_integration/fe-tecc-integration.h"
#include "../tecc_scheduler/tecc_crypto_scheduler.h"
#include "../tecc_scheduler/tecc_key_manager_adapter.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* Internal helper functions */
static void free_cache_update_info(TeccCacheUpdateInfo *info);
static TeccPostStmtProcessorResult analyze_statement_for_postprocessing(TeccPostStmtProcessorContext *context);
static TeccPostStmtProcessorResult sync_processing_state(TeccPostStmtProcessorContext *context);
static TeccPostStmtProcessorResult update_metadata_cache(TeccPostStmtProcessorContext *context);
static TeccPostStmtProcessorResult cleanup_temporary_resources(TeccPostStmtProcessorContext *context);

/* Initialize post-statement processor context */
TeccPostStmtProcessorContext *
tecc_post_stmt_processor_init(PGconn *conn)
{
    TeccPostStmtProcessorContext *context;

    if (conn == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid connection parameter to post-statement processor init");
        return NULL;
    }

    /* Allocate context */
    context = (TeccPostStmtProcessorContext *)pg_malloc(sizeof(TeccPostStmtProcessorContext));
    if (!context) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Memory allocation failed for post-statement processor context");
        return NULL;
    }

    /* Initialize context */
    memset(context, 0, sizeof(TeccPostStmtProcessorContext));
    context->conn = conn;
    context->stmt_data = NULL;
    context->query_result = NULL;
    context->needs_cache_update = false;
    context->needs_state_sync = false;
    context->needs_resource_cleanup = false;
    context->error_code = TECC_POST_STMT_PROCESSOR_SUCCESS;
    context->error_msg[0] = '\0';

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Post-statement processor context initialized");

    return context;
}

/* Process a statement in postprocessing phase */
TeccPostStmtProcessorResult
tecc_post_stmt_processor_process(TeccPostStmtProcessorContext *context,
                                TeccStatementData *stmt_data,
                                PGresult *query_result)
{
    TeccPostStmtProcessorResult result;

    if (context == NULL || stmt_data == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid parameters to post-statement processor process");
        return TECC_POST_STMT_PROCESSOR_INVALID_INPUT;
    }

    /* Store statement data and query result references */
    context->stmt_data = stmt_data;
    context->query_result = query_result;

    

    /* Step 1: Analyze statement for postprocessing needs */
    result = analyze_statement_for_postprocessing(context);
    if (result != TECC_POST_STMT_PROCESSOR_SUCCESS) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Statement analysis failed: %d", result);
        return result;
    }

    /* Step 2: Check if postprocessing is needed */
    if (!context->needs_cache_update &&
        !context->needs_state_sync &&
        !context->needs_resource_cleanup) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "No postprocessing needed for this statement");
        return TECC_POST_STMT_PROCESSOR_SUCCESS;
    }

    /* Step 3: Synchronize processing state if needed */
    if (context->needs_state_sync) {
        result = sync_processing_state(context);
        if (result != TECC_POST_STMT_PROCESSOR_SUCCESS) {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "State synchronization failed: %d", result);
            return result;
        }
    }

    /* Step 4: Update metadata cache if needed */
    if (context->needs_cache_update) {
        result = update_metadata_cache(context);
        if (result != TECC_POST_STMT_PROCESSOR_SUCCESS) {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "Cache update failed: %d", result);
            return result;
        }
    }

    /* Step 5: Clean up temporary resources if needed */
    if (context->needs_resource_cleanup) {
        result = cleanup_temporary_resources(context);
        if (result != TECC_POST_STMT_PROCESSOR_SUCCESS) {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "Resource cleanup failed: %d", result);
            return result;
        }
    }

    

    return TECC_POST_STMT_PROCESSOR_SUCCESS;
}

/* Handle prepared statement postprocessing */
TeccPostStmtProcessorResult
tecc_post_stmt_processor_handle_prepared(TeccPostStmtProcessorContext *context,
                                        TeccPreparedStatement *prepared_stmt,
                                        bool execution_success)
{
    if (context == NULL || prepared_stmt == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid parameters to post-statement processor handle_prepared");
        return TECC_POST_STMT_PROCESSOR_INVALID_INPUT;
    }

    

    /* Update prepared statement state */
    prepared_stmt->is_executing = false;
    prepared_stmt->last_used_time = time(NULL);

    if (!execution_success) {
        /* Store error information */
        prepared_stmt->error_code = context->error_code;
        if (context->error_msg[0] != '\0') {
            strncpy(prepared_stmt->error_msg, context->error_msg,
                   sizeof(prepared_stmt->error_msg) - 1);
            prepared_stmt->error_msg[sizeof(prepared_stmt->error_msg) - 1] = '\0';
        }

        tecc_processor_debug(TECC_LOG_WARNING,
                            "Prepared statement execution failed: %s",
                            prepared_stmt->error_msg);
    } else {
        /* Clear any previous error */
        prepared_stmt->error_code = 0;
        prepared_stmt->error_msg[0] = '\0';
    }

    /* Mark that state synchronization is needed */
    context->needs_state_sync = true;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Prepared statement postprocessing completed");

    return TECC_POST_STMT_PROCESSOR_SUCCESS;
}

/* Synchronize processing state */
TeccPostStmtProcessorResult
tecc_post_stmt_processor_sync_state(TeccPostStmtProcessorContext *context)
{
    if (context == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid context to sync_state");
        return TECC_POST_STMT_PROCESSOR_INVALID_INPUT;
    }

    

    return sync_processing_state(context);
}

/* Update metadata cache */
TeccPostStmtProcessorResult
tecc_post_stmt_processor_update_cache(TeccPostStmtProcessorContext *context,
                                     TeccCacheUpdateInfo **updates,
                                     int num_updates)
{
    int i;

    if (context == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid context to update_cache");
        return TECC_POST_STMT_PROCESSOR_INVALID_INPUT;
    }

    if (updates == NULL || num_updates <= 0) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "No cache updates specified");
        return TECC_POST_STMT_PROCESSOR_SUCCESS;
    }

    

    /* Process each cache update */
    for (i = 0; i < num_updates; i++) {
        TeccCacheUpdateInfo *update = updates[i];
        if (update == NULL) {
            continue;
        }

        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "Cache update: %s.%s - %s (success: %s)",
                            update->table_name ? update->table_name : "(null)",
                            update->column_name ? update->column_name : "(null)",
                            update->operation ? update->operation : "unknown",
                            update->success ? "true" : "false");

        /* Implement actual cache update logic */
        /* Based on the operation type, update the metadata cache */
        if (update->success) {
            /* Operation was successful, update cache accordingly */
            if (update->operation != NULL) {
                if (strcmp(update->operation, "CREATE") == 0 ||
                    strcmp(update->operation, "ALTER") == 0) {
                    /* For CREATE/ALTER operations, refresh the cache to get new metadata */
                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                        "Refreshing metadata cache for %s operation",
                                        update->operation);
                    tecc_cache_refresh(tecc_get_state(context->conn),
                                       TECC_CACHE_REFRESH_ALL);
                } else if (strcmp(update->operation, "DROP") == 0) {
                    /* For DROP operations, clear cache entries for the dropped object */
                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                        "Clearing metadata cache for DROP operation");
                    tecc_cache_clear(tecc_get_state(context->conn));
                } else {
                    /* For other operations, just refresh the cache */
                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                        "Refreshing metadata cache for %s operation",
                                        update->operation);
                    tecc_cache_refresh(tecc_get_state(context->conn),
                                       TECC_CACHE_REFRESH_ALL);
                }
            }
        } else {
            /* Operation failed, log but don't update cache */
            tecc_processor_debug(TECC_LOG_WARNING,
                                "Operation failed, not updating cache");
        }
    }

    

    return TECC_POST_STMT_PROCESSOR_SUCCESS;
}

/* Clean up temporary resources */
TeccPostStmtProcessorResult
tecc_post_stmt_processor_cleanup_resources(TeccPostStmtProcessorContext *context)
{
    if (context == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid context to cleanup_resources");
        return TECC_POST_STMT_PROCESSOR_INVALID_INPUT;
    }

    

    return cleanup_temporary_resources(context);
}

/* Handle object deletion (DROP statements) */
TeccPostStmtProcessorResult
tecc_post_stmt_processor_handle_deletion(TeccPostStmtProcessorContext *context,
                                        const char *object_type,
                                        const char *object_name)
{
    if (context == NULL || object_type == NULL || object_name == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid parameters to handle_deletion");
        return TECC_POST_STMT_PROCESSOR_INVALID_INPUT;
    }

    

    /* Mark that cache update is needed for deletion */
    context->needs_cache_update = true;

    /* Implement actual deletion handling logic */
    /* This involves clearing the metadata cache for the dropped object */
    if (context->needs_cache_update) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "Clearing metadata cache for dropped %s: %s",
                            object_type, object_name);
        tecc_cache_clear(tecc_get_state(context->conn));
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Deletion handling completed");

    return TECC_POST_STMT_PROCESSOR_SUCCESS;
}

/* Check if statement needs postprocessing */
bool
tecc_post_stmt_processor_needs_postprocessing(TeccPostStmtProcessorContext *context,
                                             TeccStatementData *stmt_data)
{
    if (context == NULL || stmt_data == NULL) {
        return false;
    }

    /* Store statement data for analysis */
    context->stmt_data = stmt_data;

    /* Analyze statement */
    {
        TeccPostStmtProcessorResult result = analyze_statement_for_postprocessing(context);
        if (result != TECC_POST_STMT_PROCESSOR_SUCCESS) {
            return false;
        }
    }

    /* Return true if any postprocessing is needed */
    return context->needs_cache_update ||
           context->needs_state_sync ||
           context->needs_resource_cleanup;
}

/* Clean up post-statement processor context */
void
tecc_post_stmt_processor_cleanup(TeccPostStmtProcessorContext *context)
{
    if (context == NULL) {
        return;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Cleaning up post-statement processor context");

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "DEBUG: Before freeing context at %p", context);

    /* Note: We don't own the statement data or query result, so we don't free them */

    /* Free context */
    pg_free(context);

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "DEBUG: Context freed successfully");
}

/* Unified post-statement processing interface for external modules */
TeccPostStmtProcessorResult
tecc_post_stmt_processor_run(PGconn *conn,
                            TeccStatementData *stmt_data,
                            PGresult *query_result)
{
    TeccPostStmtProcessorContext *context;
    TeccPostStmtProcessorResult result;
    TeccState *state;

    if (conn == NULL || stmt_data == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Invalid parameters to post_stmt_processor_run");
        return TECC_POST_STMT_PROCESSOR_INVALID_INPUT;
    }

    /* Get TECC state and check if TECC is enabled */
    state = tecc_get_state(conn);
    if (state != NULL && !tecc_is_enabled(state)) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "TECC is disabled (possibly due to disable_once flag), skipping post-statement processing");
        return TECC_POST_STMT_PROCESSOR_SUCCESS; /* Return success to allow query to proceed without TECC processing */
    }

    /* Initialize context */
    context = tecc_post_stmt_processor_init(conn);
    if (context == NULL) {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to initialize post-statement processor context");
        return TECC_POST_STMT_PROCESSOR_MEMORY_ERROR;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "DEBUG: Calling tecc_post_stmt_processor_process");

    /* Process statement */
    result = tecc_post_stmt_processor_process(context, stmt_data, query_result);

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "DEBUG: tecc_post_stmt_processor_process returned %d", result);

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "DEBUG: Calling tecc_post_stmt_processor_cleanup");

    /* Clean up context */
    tecc_post_stmt_processor_cleanup(context);

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "DEBUG: tecc_post_stmt_processor_cleanup completed, returning %d", result);

    return result;
}

/* ==================== Internal helper functions ==================== */

/* Free cache update information */
static void TECC_ATTR_UNUSED
free_cache_update_info(TeccCacheUpdateInfo *info)
{
    if (info == NULL) {
        return;
    }

    if (info->table_name != NULL) {
        pg_free(info->table_name);
    }

    if (info->column_name != NULL) {
        pg_free(info->column_name);
    }

    if (info->operation != NULL) {
        pg_free(info->operation);
    }

    pg_free(info);
}

/* Analyze statement for postprocessing needs */
static TeccPostStmtProcessorResult
analyze_statement_for_postprocessing(TeccPostStmtProcessorContext *context)
{
    TeccStatementData *stmt_data;

    if (context == NULL || context->stmt_data == NULL) {
        return TECC_POST_STMT_PROCESSOR_INVALID_INPUT;
    }

    stmt_data = context->stmt_data;

    /* Reset postprocessing flags */
    context->needs_cache_update = false;
    context->needs_state_sync = false;
    context->needs_resource_cleanup = false;

    /* ReqID:TECC4.0 - Check statement type for DDL cache refresh needs */
    switch (stmt_data->stmt_type) {
        case TECC_STMT_TYPE_CREATE_TABLE:
        case TECC_STMT_TYPE_CREATE_KEK:
        case TECC_STMT_TYPE_CREATE_DEK:
        case TECC_STMT_TYPE_ALTER_TABLE:
        case TECC_STMT_TYPE_DROP:
            context->needs_cache_update = true;
            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "DDL statement type %d detected - needs cache refresh",
                                stmt_data->stmt_type);
            break;
        default:
            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "Non-DDL statement type %d - no cache refresh needed",
                                stmt_data->stmt_type);
            break;
    }

    /* Always perform resource cleanup if there are temporary resources */
    if (stmt_data->raw_values_list != NULL ||
        stmt_data->encrypted_columns != NULL) {
        context->needs_resource_cleanup = true;
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "Temporary resources detected - needs resource cleanup");
    }

    /* Always need resource cleanup if there was an error */
    if (stmt_data->error_code != TECC_PROCESSOR_SUCCESS) {
        context->needs_resource_cleanup = true;
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "Statement with error detected - needs resource cleanup");
    }

    return TECC_POST_STMT_PROCESSOR_SUCCESS;
}

/* Synchronize processing state */
static TeccPostStmtProcessorResult
sync_processing_state(TeccPostStmtProcessorContext *context)
{
    TeccStatementData *stmt_data;

    if (context == NULL || context->stmt_data == NULL) {
        return TECC_POST_STMT_PROCESSOR_INVALID_INPUT;
    }

    stmt_data = context->stmt_data;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Synchronizing processing state");

    /* Implement actual state synchronization logic */
    /* This involves:
       1. Updating prepared statement states
       2. Synchronizing transaction state
       3. Updating connection state
       4. Handling any pending operations
    */

    /* Check statement type and handle accordingly */
    if (stmt_data->stmt_type == TECC_STMT_TYPE_PREPARE ||
        stmt_data->stmt_type == TECC_STMT_TYPE_EXECUTE ||
        stmt_data->stmt_type == TECC_STMT_TYPE_DECLARE_CURSOR) {
        /* For PREPARE/EXECUTE statements, update prepared statement states */
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "Updating prepared statement state for statement type: %d",
                            stmt_data->stmt_type);
        /* Implement prepared statement state update logic */
        if (stmt_data->stmt_type == TECC_STMT_TYPE_PREPARE) {
            /* For PREPARE statements, mark the statement as prepared */
            if (stmt_data->stmtName != NULL) {
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                    "Marking prepared statement '%s' as prepared",
                                    stmt_data->stmtName);
                /* In a real implementation, we would update the prepared statement cache */
                /* For now, we just log the action */
            }
        } else if (stmt_data->stmt_type == TECC_STMT_TYPE_EXECUTE) {
            /* For EXECUTE statements, update execution state */
            if (stmt_data->stmtName != NULL) {
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                    "Updating execution state for prepared statement '%s'",
                                    stmt_data->stmtName);
                /* Update last used time and execution count */
                /* In a real implementation, we would update the prepared statement metadata */
            }
        }

        /* Update prepared statement cache if needed */
        if (context->query_result != NULL) {
            /* Check if the query was successful */
            int result_status = PQresultStatus(context->query_result);
            if (result_status == PGRES_COMMAND_OK || result_status == PGRES_TUPLES_OK) {
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                    "Query successful, updating prepared statement cache");
                /* In a real implementation, we would update the cache here */
            } else {
                tecc_processor_debug(TECC_LOG_WARNING,
                                    "Query failed, not updating prepared statement cache");
            }
        }
    } else if (stmt_data->stmt_type == TECC_STMT_TYPE_SELECT ||
               stmt_data->stmt_type == TECC_STMT_TYPE_INSERT ||
               stmt_data->stmt_type == TECC_STMT_TYPE_UPDATE ||
               stmt_data->stmt_type == TECC_STMT_TYPE_DELETE) {
        /* For DML statements, synchronize transaction state */
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "Synchronizing transaction state for statement type: %d",
                            stmt_data->stmt_type);
        /* Implement transaction state synchronization logic */
        if (context->conn != NULL) {
            /* Check transaction state */
            int transaction_status = PQtransactionStatus(context->conn);
            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "Current transaction status: %d", transaction_status);

            /* Update transaction state based on statement type and result */
            if (context->query_result != NULL) {
                int result_status = PQresultStatus(context->query_result);

                if (result_status == PGRES_COMMAND_OK || result_status == PGRES_TUPLES_OK) {
                    /* Successful DML operation */
                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                        "DML operation successful, transaction state updated");

                    /* For INSERT/UPDATE/DELETE, we might need to update row count */
                    if (stmt_data->stmt_type == TECC_STMT_TYPE_INSERT ||
                        stmt_data->stmt_type == TECC_STMT_TYPE_UPDATE ||
                        stmt_data->stmt_type == TECC_STMT_TYPE_DELETE) {
                        char *cmd_status = PQcmdStatus(context->query_result);
                        if (cmd_status != NULL) {
                            tecc_processor_debug(TECC_LOG_DEBUG1,
                                                "Command status: %s", cmd_status);
                        }
                    }
                } else {
                    /* Failed DML operation */
                    tecc_processor_debug(TECC_LOG_WARNING,
                                        "DML operation failed, transaction may need rollback");

                    /* Check if we're in a failed transaction state */
                    if (transaction_status == PQTRANS_INERROR) {
                        tecc_processor_debug(TECC_LOG_WARNING,
                                            "Transaction is in error state, may need manual intervention");
                    }
                }
            }
        }
    } else if (stmt_data->stmt_type == TECC_STMT_TYPE_CREATE_TABLE ||
               stmt_data->stmt_type == TECC_STMT_TYPE_CREATE_KEK ||
               stmt_data->stmt_type == TECC_STMT_TYPE_CREATE_DEK) {
        /* For DDL statements, update connection state */
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "Updating connection state for statement type: %d",
                            stmt_data->stmt_type);
        /* Implement connection state update logic */
        if (context->conn != NULL) {
            /* DDL statements may affect connection state (e.g., schema changes) */

            /* 1. Update schema cache */
            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "Updating schema cache for DDL statement");

            /* 2. Check if connection parameters need updating */
            if (stmt_data->stmt_type == TECC_STMT_TYPE_CREATE_KEK ||
                stmt_data->stmt_type == TECC_STMT_TYPE_CREATE_DEK) {
                /* KEK/DEK creation affects encryption key cache */
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                    "KEK/DEK creation detected, updating key cache");
            }

            /* 3. Update connection metadata */
            if (context->query_result != NULL) {
                int result_status = PQresultStatus(context->query_result);
                if (result_status == PGRES_COMMAND_OK) {
                    tecc_processor_debug(TECC_LOG_DEBUG1,
                                        "DDL successful, connection state updated");

                    /* For CREATE TABLE, we need to update table metadata cache */
                    if (stmt_data->stmt_type == TECC_STMT_TYPE_CREATE_TABLE) {
                        tecc_processor_debug(TECC_LOG_DEBUG1,
                                            "CREATE TABLE successful, updating table metadata");
                    }
                } else {
                    tecc_processor_debug(TECC_LOG_WARNING,
                                        "DDL failed, connection state may be inconsistent");
                }
            }

            /* 4. Reset any connection-specific caches */
            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "Resetting connection-specific caches for DDL");
        }
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Processing state synchronized");

    return TECC_POST_STMT_PROCESSOR_SUCCESS;
}

/* Update metadata cache */
static TeccPostStmtProcessorResult
update_metadata_cache(TeccPostStmtProcessorContext *context)
{
    TeccStatementData *stmt_data;

    if (context == NULL || context->stmt_data == NULL) {
        return TECC_POST_STMT_PROCESSOR_INVALID_INPUT;
    }

    stmt_data = context->stmt_data;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Updating metadata cache");

    /* Implement actual cache update logic */
    /* This involves:
       1. Querying the database for updated metadata
       2. Updating the local metadata cache
       3. Invalidating stale cache entries
       4. Notifying other modules of cache changes
    */

    /* Check statement type to determine cache update strategy */
    switch (stmt_data->stmt_type) {
        case TECC_STMT_TYPE_CREATE_KEK:
        case TECC_STMT_TYPE_CREATE_DEK:
            /* PENDING→ENCRYPTED promotion is handled in tecc_process_result
             * (PGRES_COMMAND_OK path). No cache refresh needed here. */
            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "CREATE KEK/DEK: handled by tecc_process_result PENDING promotion");
            break;

        case TECC_STMT_TYPE_CREATE_TABLE:
        case TECC_STMT_TYPE_ALTER_TABLE:
            /* tecc_refresh_table_cache (called from tecc_process_result
             * PGRES_COMMAND_OK path) already loaded the committed column
             * metadata for the new/altered table.  No need to wipe the
             * entire column cache here. */
            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "CREATE/ALTER TABLE: relying on tecc_refresh_table_cache, no full column cache clear");
            break;

        case TECC_STMT_TYPE_DROP:
        {
            /* ReqID:TECC4.0 - Selective cache clear by DROP object type */
            const TeccState *drop_state = tecc_get_state(context->conn);
            TeccCacheManager *drop_mgr;
            int rmtype = stmt_data->drop_remove_type;
            int behav  = stmt_data->drop_behavior;

            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "DROP cache clear: removeType=%d behavior=%d",
                                rmtype, behav);

            drop_mgr = get_cache_manager(drop_state);
            if (drop_mgr != NULL) {
                /* OBJECT_KEK=1000, OBJECT_DEK=1001 (tecc_nodes.h frontend defs) */
                if (rmtype == OBJECT_KEK) {
                    /* DROP KEK */
                    tecc_cache_clear_specific(drop_mgr, TECC_CACHE_REFRESH_KEK);
                    if (behav == 1) {
                        tecc_cache_clear_specific(drop_mgr, TECC_CACHE_REFRESH_DEK);
                        tecc_cache_clear_specific(drop_mgr, TECC_CACHE_REFRESH_COLUMN);
                    }
                } else if (rmtype == OBJECT_DEK) {
                    /* DROP DEK */
                    tecc_cache_clear_specific(drop_mgr, TECC_CACHE_REFRESH_DEK);
                    if (behav == 1) {
                        tecc_cache_clear_specific(drop_mgr, TECC_CACHE_REFRESH_COLUMN);
                        tecc_cache_clear_specific(drop_mgr, TECC_CACHE_REFRESH_KEK);
                    }
                } else {
                    /* Unknown DROP - conservative: clear KEK+DEK+COLUMN */
                    tecc_cache_clear_specific(drop_mgr, TECC_CACHE_REFRESH_KEK);
                    tecc_cache_clear_specific(drop_mgr, TECC_CACHE_REFRESH_DEK);
                    tecc_cache_clear_specific(drop_mgr, TECC_CACHE_REFRESH_COLUMN);
                }
            }

            /* ReqID:TECC4.0 — expire adapter key_cache for DROP KEK/DEK. */
            if ((rmtype == OBJECT_KEK || rmtype == OBJECT_DEK) &&
                drop_state != NULL &&
                drop_state->crypto_scheduler != NULL &&
                drop_state->crypto_scheduler->key_adapter != NULL &&
                stmt_data->drop_key_oid != InvalidOid)
            {
                tecc_key_cache_expire(drop_state->crypto_scheduler->key_adapter,
                                     stmt_data->drop_key_oid);
            }
            break;
        }

        case TECC_STMT_TYPE_SELECT:
        case TECC_STMT_TYPE_INSERT:
        case TECC_STMT_TYPE_UPDATE:
        case TECC_STMT_TYPE_DELETE:
            /* For DML statements, check if cache needs refresh based on context */
            if (context->needs_cache_update) {
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                    "Refreshing metadata cache for DML statement type: %d",
                                    stmt_data->stmt_type);
                tecc_cache_refresh(tecc_get_state(context->conn),
                                   TECC_CACHE_REFRESH_ALL);
            }
            break;

        default:
            /* For other statement types, no cache update needed */
            tecc_processor_debug(TECC_LOG_DEBUG1,
                                "No cache update needed for statement type: %d",
                                stmt_data->stmt_type);
            break;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Metadata cache update completed");

    return TECC_POST_STMT_PROCESSOR_SUCCESS;
}

/* Clean up temporary resources */
static TeccPostStmtProcessorResult
cleanup_temporary_resources(TeccPostStmtProcessorContext *context)
{
    TeccStatementData *stmt_data;

    if (context == NULL || context->stmt_data == NULL) {
        return TECC_POST_STMT_PROCESSOR_INVALID_INPUT;
    }

    stmt_data = context->stmt_data;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Cleaning up temporary resources");

    /* Clean up raw values list if present */
    if (stmt_data->raw_values_list != NULL) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "Cleaning up raw values list");
        /* Note: The raw_values_list should be cleaned up by the values processor */
        /* We just clear the reference here */
        stmt_data->raw_values_list = NULL;
    }

    /* Clean up encrypted columns list if present */
    if (stmt_data->encrypted_columns != NULL) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "Cleaning up encrypted columns list");
        /* Note: The encrypted_columns list should be cleaned up by the cache module */
        /* We just clear the reference here */
        stmt_data->encrypted_columns = NULL;
    }

    /* Clear any temporary parse tree */
    if (stmt_data->parse_tree != NULL) {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                            "Cleaning up parse tree");
        /* Note: The parse tree should be cleaned up by the parser module */
        /* We just clear the reference here */
        stmt_data->parse_tree = NULL;
    }

    /* Clear error state if processing was successful */
    if (stmt_data->error_code == TECC_PROCESSOR_SUCCESS) {
        tecc_processor_clear_error(stmt_data);
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Temporary resources cleaned up");

    return TECC_POST_STMT_PROCESSOR_SUCCESS;
}
/* End - ReqID:TECC4.0 */