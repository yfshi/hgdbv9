/* Begin - ReqID:TECC4.0 */
/*
 * File: pre_stmt_processor.h
 *
 * Abstract:
 *   TECC pre-statement processor for statement preprocessing phase.
 *   Handles preprocessing stage management, parameter type replacement, and query string reconstruction.
 *
 * Authored by <wangxp@highgo.com>, 20260410.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/pre_stmt_processor.h
 */

#ifndef TECC_PRE_STMT_PROCESSOR_H
#define TECC_PRE_STMT_PROCESSOR_H

#include "tecc_processor_types.h"

/* Include PostgreSQL headers */
#include "postgres_fe.h"
#include "../libpq-fe.h"

/* Include external module interfaces */
#include "../frontend_parser/fe_adapter.h"
#include "prepared_statement.h"
#include "prepared_statements_list.h"

/*
 * Pre-statement processor result codes
 */
typedef enum TeccPreStmtProcessorResult {
    TECC_PRE_STMT_PROCESSOR_SUCCESS = 0,
    TECC_PRE_STMT_PROCESSOR_ERROR,
    TECC_PRE_STMT_PROCESSOR_INVALID_STATEMENT,
    TECC_PRE_STMT_PROCESSOR_PARSE_ERROR,
    TECC_PRE_STMT_PROCESSOR_TYPE_REPLACEMENT_ERROR,
    TECC_PRE_STMT_PROCESSOR_QUERY_RECONSTRUCTION_ERROR,
    TECC_PRE_STMT_PROCESSOR_MEMORY_ERROR,
    TECC_PRE_STMT_PROCESSOR_INVALID_INPUT
} TeccPreStmtProcessorResult;

/*
 * Preprocessing context structure
 */
typedef struct TeccPreStmtProcessorContext {
    PGconn *conn;              /* Database connection */
    TeccStatementData *stmt_data; /* Statement data being processed */
    List *parse_tree;          /* Parsed statement tree */
    bool needs_type_replacement; /* Whether type replacement is needed */
    bool needs_query_reconstruction; /* Whether query reconstruction is needed */
    char *reconstructed_query; /* Reconstructed query string */
    int error_code;            /* Error code if processing failed */
    char error_msg[256];       /* Error message if processing failed */
} TeccPreStmtProcessorContext;

/*
 * Type replacement information
 */
typedef struct TeccTypeReplacementInfo {
    Oid original_type;         /* Original type OID */
    Oid replacement_type;      /* Replacement type OID */
    int param_index;           /* Parameter index (for prepared statements) */
    char *reason;              /* Reason for replacement */
} TeccTypeReplacementInfo;

/*
 * Public interface functions
 */

/* Initialize pre-statement processor context */
TeccPreStmtProcessorContext *tecc_pre_stmt_processor_init(PGconn *conn);

/* Process a statement in preprocessing phase */
TeccPreStmtProcessorResult tecc_pre_stmt_processor_process(
    TeccPreStmtProcessorContext *context,
    TeccStatementData *stmt_data);

/* Handle prepared statement preprocessing */
TeccPreStmtProcessorResult tecc_pre_stmt_processor_handle_prepared(
    TeccPreStmtProcessorContext *context,
    TeccPreparedStatement *prepared_stmt);

/* Perform parameter type replacement */
TeccPreStmtProcessorResult tecc_pre_stmt_processor_replace_param_types(
    TeccPreStmtProcessorContext *context,
    TeccTypeReplacementInfo **replacements,
    int num_replacements);

/* Reconstruct query string after processing */
TeccPreStmtProcessorResult tecc_pre_stmt_processor_reconstruct_query(
    TeccPreStmtProcessorContext *context);

/* Check if statement needs preprocessing */
bool tecc_pre_stmt_processor_needs_preprocessing(
    TeccPreStmtProcessorContext *context,
    TeccStatementData *stmt_data);

/* Get reconstructed query string */
const char *tecc_pre_stmt_processor_get_reconstructed_query(
    TeccPreStmtProcessorContext *context);

/* Clean up pre-statement processor context */
void tecc_pre_stmt_processor_cleanup(TeccPreStmtProcessorContext *context);

/* Unified pre-statement processing interface for external modules */
TeccPreStmtProcessorResult tecc_pre_stmt_processor_run(
    PGconn *conn,
    TeccStatementData *stmt_data);

#endif /* TECC_PRE_STMT_PROCESSOR_H */
/* End - ReqID:TECC4.0 */