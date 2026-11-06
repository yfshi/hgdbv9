/* Begin - ReqID:TECC4.0 */
/*
 * File: func_processor.h
 *
 * Abstract:
 *   TECC function processor for analyzing function calls with encrypted parameters.
 *   Handles function call analysis, function support checking, and parameter processing.
 *
 * Authored by <wangxp@highgo.com>, 20260410.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/func_processor.h
 */

#ifndef TECC_FUNC_PROCESSOR_H
#define TECC_FUNC_PROCESSOR_H

#include "tecc_processor_types.h"

/* Include PostgreSQL frontend headers */
#include "postgres_fe.h"
/* Include libpq for database connection types */
#include "../libpq-fe.h"

/* Include external module interfaces */
#include "../tecc_cache/tecc_metadata_cache.h"
/* Expression processor is part of processor module, not separate module */
#include "expr_processor.h"
/* Function hardcoded values processor */
#include "func_hardcoded_values.h"

/*
 * Function processor result codes
 */
typedef enum TeccFuncProcessorResult {
    TECC_FUNC_PROCESSOR_SUCCESS = 0,
    TECC_FUNC_PROCESSOR_ERROR,
    TECC_FUNC_PROCESSOR_UNSUPPORTED_FUNCTION,
    TECC_FUNC_PROCESSOR_INVALID_ARGUMENTS,
    TECC_FUNC_PROCESSOR_ENCRYPTION_ERROR,
    TECC_FUNC_PROCESSOR_MEMORY_ERROR,
    TECC_FUNC_PROCESSOR_INVALID_INPUT
} TeccFuncProcessorResult;

/*
 * Function call information structure
 */
typedef struct TeccFuncCallInfo {
    char *function_name;       /* Function name */
    char *schema_name;         /* Schema name (optional) */
    char *catalog_name;        /* Catalog name (optional) */
    List *args;                /* Function arguments list */
    int location;              /* Location in SQL string */
    bool is_aggregate;         /* Whether function is aggregate */
    bool is_window;            /* Whether function is window function */
    bool is_variadic;          /* Whether function is variadic */
    Oid return_type;           /* Function return type OID */
    bool supports_encryption;  /* Whether function supports encrypted parameters */
} TeccFuncCallInfo;

/*
 * Function parameter information structure
 */
typedef struct TeccFuncParamInfo {
    int param_index;           /* Parameter index (0-based) */
    Node *param_expr;          /* Parameter expression node */
    bool is_encrypted;         /* Whether parameter involves encrypted column */
    char *table_name;          /* Table name (if column reference) */
    char *column_name;         /* Column name (if column reference) */
    Oid param_type;            /* Parameter type OID */
    bool needs_encryption;     /* Whether parameter needs encryption */
    bool needs_decryption;     /* Whether parameter needs decryption */
} TeccFuncParamInfo;

/*
 * Function processing context
 */
typedef struct TeccFuncProcessorContext {
    PGconn *conn;              /* Database connection */
    TeccFuncCallInfo *func_call; /* Current function call */
    TeccFuncParamInfo **params; /* Array of parameter information */
    int num_params;            /* Number of parameters */
    int num_encrypted_params;  /* Number of encrypted parameters */
    int error_code;            /* Error code if processing failed */
    char error_msg[256];       /* Error message if processing failed */
} TeccFuncProcessorContext;

/*
 * Public interface functions
 */

/* Initialize function processor context */
TeccFuncProcessorContext *tecc_func_processor_init(PGconn *conn);

/* Process a function call */
TeccFuncProcessorResult tecc_func_processor_process_call(
    TeccFuncProcessorContext *context,
    const Node *func_call_node,
    TeccStatementData *statement_data);

/* Check if a function supports encrypted parameters */
bool tecc_func_processor_check_support(
    TeccFuncProcessorContext *context,
    const char *function_name,
    const char *schema_name);

/* Analyze function arguments for encryption requirements */
TeccFuncProcessorResult tecc_func_processor_analyze_args(
    TeccFuncProcessorContext *context,
    List *args,
    TeccStatementData *statement_data);

/* Process function parameters (encryption/decryption) */
TeccFuncProcessorResult tecc_func_processor_process_params(
    TeccFuncProcessorContext *context,
    TeccStatementData *statement_data);

/* Get function call information */
TeccFuncCallInfo *tecc_func_processor_get_call_info(
    TeccFuncProcessorContext *context);

/* Get parameter information */
TeccFuncParamInfo *tecc_func_processor_get_param_info(
    TeccFuncProcessorContext *context,
    int param_index);

/* Clean up function processor context */
void tecc_func_processor_cleanup(TeccFuncProcessorContext *context);

/* Unified function processing interface for external modules */
int tecc_func_processor_handle_function_call(
    PGconn *conn,
    const Node *func_call_node,
    TeccStatementData *statement_data);

/* CREATE FUNCTION/PROCEDURE statement processing */
bool tecc_func_processor_run_pre_create_function_stmt(
    const List *options,
    TeccStatementData *statement_data,
    bool is_do_stmt);

/* Function expression processing */
bool tecc_func_processor_process(
    const TeccExprPartsList *target_expr_vec,
    TeccStatementData *statement_data);

#endif /* TECC_FUNC_PROCESSOR_H */
/* End - ReqID:TECC4.0 */