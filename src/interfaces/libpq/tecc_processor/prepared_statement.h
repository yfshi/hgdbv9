/* Begin - ReqID:TECC4.0 */
/*
 * File: prepared_statement.h
 *
 * Abstract:
 *   TECC prepared statement management for PREPARE/EXECUTE statement support.
 *   Handles prepared statement lifecycle, parameter caching, and state maintenance.
 *
 * Authored by <wangxp@highgo.com>, 20260410.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/prepared_statement.h
 */

#ifndef TECC_PREPARED_STATEMENT_H
#define TECC_PREPARED_STATEMENT_H

#include "tecc_processor_types.h"

/* Include PostgreSQL headers */
#include "postgres_fe.h"
#include "../libpq-fe.h"

/* Include external module interfaces */
#include "../frontend_parser/fe_adapter.h"

/*
 * Prepared statement result codes
 */
typedef enum TeccPreparedStatementResult {
    TECC_PREPARED_STATEMENT_SUCCESS = 0,
    TECC_PREPARED_STATEMENT_ERROR,
    TECC_PREPARED_STATEMENT_NOT_FOUND,
    TECC_PREPARED_STATEMENT_ALREADY_EXISTS,
    TECC_PREPARED_STATEMENT_INVALID_NAME,
    TECC_PREPARED_STATEMENT_INVALID_QUERY,
    TECC_PREPARED_STATEMENT_PARSE_ERROR,
    TECC_PREPARED_STATEMENT_MEMORY_ERROR,
    TECC_PREPARED_STATEMENT_INVALID_INPUT
} TeccPreparedStatementResult;

/*
 * Prepared statement parameter information
 */
typedef struct TeccPreparedStatementParam {
    int param_index;           /* Parameter index (0-based) */
    Oid param_type;            /* Parameter type OID */
    char *param_name;          /* Parameter name (optional) */
    bool is_encrypted;         /* Whether parameter involves encrypted column */
    char *table_name;          /* Table name (if column reference) */
    char *column_name;         /* Column name (if column reference) */
    char *column_type;         /* Encrypted type name (e.g. "int4_enc") */
    bool needs_encryption;     /* Whether parameter needs encryption */
    bool needs_decryption;     /* Whether parameter needs decryption */
} TeccPreparedStatementParam;

/*
 * Prepared statement structure
 */
typedef struct TeccPreparedStatement {
    char *stmt_name;           /* Statement name */
    char *query_string;        /* Original query string */
    List *parse_tree;          /* Parsed query tree */
    TeccPreparedStatementParam **params; /* Array of parameter information */
    int num_params;            /* Number of parameters */
    int num_encrypted_params;  /* Number of encrypted parameters */
    bool is_prepared;          /* Whether statement is prepared */
    bool is_executing;         /* Whether statement is currently executing */
    int ref_count;             /* Reference count for shared usage */
    time_t created_time;       /* Creation timestamp */
    time_t last_used_time;     /* Last usage timestamp */
    int error_code;            /* Error code if processing failed */
    char error_msg[256];       /* Error message if processing failed */
    /* ReqID:TECC4.0 - 参数映射分析期间的「限定名 → 基表」映射。
     * PREPARE 内层里 CTE 名、派生表别名都不是基表，直接拿它们查加密列缓存
     * 必然落空，命中不了就会把参数当明文外发。仅分析期间有效，
     * 由 free_prepared_statement 释放。 */
    List *alias_names;
    List *alias_tables;
    struct TeccPreparedStatement *next; /* Next statement in list */
} TeccPreparedStatement;

/*
 * Prepared statement processing context
 */
typedef struct TeccPreparedStatementContext {
    PGconn *conn;              /* Database connection */
    TeccPreparedStatement *stmt; /* Current prepared statement */
    int error_code;            /* Error code if processing failed */
    char error_msg[256];       /* Error message if processing failed */
} TeccPreparedStatementContext;

/*
 * Public interface functions
 */

/* Initialize prepared statement context */
TeccPreparedStatementContext *tecc_prepared_statement_init(PGconn *conn);

/* Create a new prepared statement */
TeccPreparedStatementResult tecc_prepared_statement_create(
    TeccPreparedStatementContext *context,
    const char *stmt_name,
    const char *query_string,
    TeccStatementData *statement_data);

/* Execute a prepared statement */
TeccPreparedStatementResult tecc_prepared_statement_execute(
    TeccPreparedStatementContext *context,
    const char *stmt_name,
    int nParams,
    const Oid *paramTypes,
    const char * const * paramValues,
    const int *paramLengths,
    const int *paramFormats,
    TeccStatementData *statement_data);

/* Deallocate a prepared statement */
TeccPreparedStatementResult tecc_prepared_statement_deallocate(
    TeccPreparedStatementContext *context,
    const char *stmt_name);

/* Get prepared statement by name */
TeccPreparedStatement *tecc_prepared_statement_get(
    TeccPreparedStatementContext *context,
    const char *stmt_name);

/* Check if prepared statement exists */
bool tecc_prepared_statement_exists(
    TeccPreparedStatementContext *context,
    const char *stmt_name);

/* Analyze prepared statement parameters */
TeccPreparedStatementResult tecc_prepared_statement_analyze_params(
    TeccPreparedStatementContext *context,
    TeccStatementData *statement_data);

/* Process prepared statement parameters (encryption/decryption) */
TeccPreparedStatementResult tecc_prepared_statement_process_params(
    TeccPreparedStatementContext *context,
    TeccStatementData *statement_data);

/* Clean up prepared statement context */
void tecc_prepared_statement_cleanup(TeccPreparedStatementContext *context);

/* Unified prepared statement interface for external modules */
TeccPreparedStatementResult tecc_prepared_statement_handle_prepare(
    PGconn *conn,
    const char *stmt_name,
    const char *query_string,
    TeccStatementData *statement_data);

TeccPreparedStatementResult tecc_prepared_statement_handle_execute(
    PGconn *conn,
    const char *stmt_name,
    int nParams,
    const Oid *paramTypes,
    const char * const * paramValues,
    const int *paramLengths,
    const int *paramFormats,
    TeccStatementData *statement_data);

TeccPreparedStatementResult tecc_prepared_statement_handle_deallocate(
    PGconn *conn,
    const char *stmt_name,
    TeccStatementData *statement_data);

#endif /* TECC_PREPARED_STATEMENT_H */
/* End - ReqID:TECC4.0 */