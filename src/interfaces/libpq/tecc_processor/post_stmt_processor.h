/* Begin - ReqID:TECC4.0 */
/*
 * File: post_stmt_processor.h
 *
 * Abstract:
 *   TECC post-statement processor for statement postprocessing phase.
 *   Handles postprocessing stage management, state synchronization, cache updates, and resource cleanup.
 *
 * Authored by <wangxp@highgo.com>, 20260410.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/post_stmt_processor.h
 */

#ifndef TECC_POST_STMT_PROCESSOR_H
#define TECC_POST_STMT_PROCESSOR_H

#include "tecc_processor_types.h"

/* Include PostgreSQL headers */
#include "postgres_fe.h"
#include "../libpq-fe.h"

/* Include external module interfaces */
#include "../tecc_cache/tecc_metadata_cache.h"
#include "prepared_statement.h"
#include "prepared_statements_list.h"

/*
 * Post-statement processor result codes
 */
typedef enum TeccPostStmtProcessorResult {
    TECC_POST_STMT_PROCESSOR_SUCCESS = 0,
    TECC_POST_STMT_PROCESSOR_ERROR,
    TECC_POST_STMT_PROCESSOR_INVALID_STATEMENT,
    TECC_POST_STMT_PROCESSOR_CACHE_UPDATE_ERROR,
    TECC_POST_STMT_PROCESSOR_STATE_SYNC_ERROR,
    TECC_POST_STMT_PROCESSOR_RESOURCE_CLEANUP_ERROR,
    TECC_POST_STMT_PROCESSOR_MEMORY_ERROR,
    TECC_POST_STMT_PROCESSOR_INVALID_INPUT
} TeccPostStmtProcessorResult;

/*
 * Postprocessing context structure
 */
typedef struct TeccPostStmtProcessorContext {
    PGconn *conn;              /* Database connection */
    TeccStatementData *stmt_data; /* Statement data being processed */
    PGresult *query_result;    /* Query result (if any) */
    bool needs_cache_update;   /* Whether cache update is needed */
    bool needs_state_sync;     /* Whether state synchronization is needed */
    bool needs_resource_cleanup; /* Whether resource cleanup is needed */
    int error_code;            /* Error code if processing failed */
    char error_msg[256];       /* Error message if processing failed */
} TeccPostStmtProcessorContext;

/*
 * Cache update information
 */
typedef struct TeccCacheUpdateInfo {
    char *table_name;          /* Table name */
    char *column_name;         /* Column name */
    char *operation;           /* Operation (CREATE, ALTER, DROP, etc.) */
    bool success;              /* Whether operation was successful */
    time_t update_time;        /* Update timestamp */
} TeccCacheUpdateInfo;

/*
 * Public interface functions
 */

/* Initialize post-statement processor context */
TeccPostStmtProcessorContext *tecc_post_stmt_processor_init(PGconn *conn);

/* Process a statement in postprocessing phase */
TeccPostStmtProcessorResult tecc_post_stmt_processor_process(
    TeccPostStmtProcessorContext *context,
    TeccStatementData *stmt_data,
    PGresult *query_result);

/* Handle prepared statement postprocessing */
TeccPostStmtProcessorResult tecc_post_stmt_processor_handle_prepared(
    TeccPostStmtProcessorContext *context,
    TeccPreparedStatement *prepared_stmt,
    bool execution_success);

/* Synchronize processing state */
TeccPostStmtProcessorResult tecc_post_stmt_processor_sync_state(
    TeccPostStmtProcessorContext *context);

/* Update metadata cache */
TeccPostStmtProcessorResult tecc_post_stmt_processor_update_cache(
    TeccPostStmtProcessorContext *context,
    TeccCacheUpdateInfo **updates,
    int num_updates);

/* Clean up temporary resources */
TeccPostStmtProcessorResult tecc_post_stmt_processor_cleanup_resources(
    TeccPostStmtProcessorContext *context);

/* Handle object deletion (DROP statements) */
TeccPostStmtProcessorResult tecc_post_stmt_processor_handle_deletion(
    TeccPostStmtProcessorContext *context,
    const char *object_type,
    const char *object_name);

/* Check if statement needs postprocessing */
bool tecc_post_stmt_processor_needs_postprocessing(
    TeccPostStmtProcessorContext *context,
    TeccStatementData *stmt_data);

/* Clean up post-statement processor context */
void tecc_post_stmt_processor_cleanup(TeccPostStmtProcessorContext *context);

/* Unified post-statement processing interface for external modules */
TeccPostStmtProcessorResult tecc_post_stmt_processor_run(
    PGconn *conn,
    TeccStatementData *stmt_data,
    PGresult *query_result);

#endif /* TECC_POST_STMT_PROCESSOR_H */
/* End - ReqID:TECC4.0 */