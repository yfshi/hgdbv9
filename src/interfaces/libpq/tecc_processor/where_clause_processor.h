/* Begin - ReqID:TECC4.0 */
/*
 * File: where_clause_processor.h
 *
 * Abstract:
 *   WHERE clause processor submodule for TECC processor module.
 *   Handles processing of WHERE clauses in SQL statements,
 *   including operator compatibility checking and encryption
 *   of comparison values.
 *
 * Authored by <wangxp@highgo.com>, 20260403.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/where_clause_processor.h
 */

#ifndef WHERE_CLAUSE_PROCESSOR_H
#define WHERE_CLAUSE_PROCESSOR_H

#include "tecc_processor.h"
#include "internal/tecc_processor_internal.h"

/*
 * WHERE clause processor result codes
 */
typedef enum TeccWhereClauseProcessorResult
{
    TECC_WHERE_PROCESSOR_SUCCESS = 0,
    TECC_WHERE_PROCESSOR_ERROR = -1,
    TECC_WHERE_PROCESSOR_UNSUPPORTED_OPERATOR = -2,
    TECC_WHERE_PROCESSOR_INCOMPATIBLE_TYPES = -3,
    TECC_WHERE_PROCESSOR_ENCRYPTION_NEEDED = -4,
    TECC_WHERE_PROCESSOR_NO_ENCRYPTED_COLUMNS = -5,
    TECC_WHERE_PROCESSOR_MISSING_DET = -6,     /* 需要DET算法但列没有 */
    TECC_WHERE_PROCESSOR_MISSING_OPE = -7,     /* 需要OPE算法但列没有 */
    TECC_WHERE_PROCESSOR_MISSING_SEARCH = -8,  /* 需要SEARCH算法但列没有 */
    TECC_WHERE_PROCESSOR_MISSING_DET_OPE = -9  /* =/<>等需DET或OPE但列都没有 */
} TeccWhereClauseProcessorResult;

/*
 * Operator support information
 */
typedef struct TeccOperatorSupport
{
    char *operator_name;
    bool supports_deterministic;
    bool supports_order_preserving;
    bool supports_searchable;
    bool needs_plaintext;
    char *required_algorithm;
} TeccOperatorSupport;

/*
 * WHERE clause processing context
 */
typedef struct TeccWhereClauseProcessorContext
{
    List *encrypted_columns;
    List *processed_conditions;
    bool needs_rewrite;
    int operator_count;
    int encrypted_operator_count;
} TeccWhereClauseProcessorContext;

/*
 * WHERE clause processor interface functions
 */

/* Main WHERE clause processing entry point */
TeccWhereClauseProcessorResult tecc_where_clause_processor_process(Node *where_clause,
                                                                   TeccStatementData *statement_data);

/* Process individual condition */
TeccWhereClauseProcessorResult tecc_where_clause_processor_process_condition(Node *condition,
                                                                              TeccStatementData *statement_data);

/* Process arbitrary expression node (SELECT target list, RETURNING, etc.) */
TeccWhereClauseProcessorResult tecc_where_clause_processor_process_expr(Node *expr,
                                                                        TeccStatementData *statement_data);

/* Check operator compatibility */
TeccWhereClauseProcessorResult tecc_where_clause_processor_check_operator(const char *operator_name,
                                                                           const char *left_type,
                                                                           const char *right_type,
                                                                           TeccStatementData *statement_data);

/* Get supported algorithms for operator */
List *tecc_where_clause_processor_get_supported_algorithms(const char *operator_name);

/* Check if operator needs encryption */
bool tecc_where_clause_processor_operator_needs_encryption(const char *operator_name,
                                                           const char *column_type);

/* Process comparison expression */
TeccWhereClauseProcessorResult tecc_where_clause_processor_process_comparison(Node *left_expr,
                                                                               Node *right_expr,
                                                                               const char *operator_name,
                                                                               TeccStatementData *statement_data);

/* Extract column references from WHERE clause */
List *tecc_where_clause_processor_extract_columns(Node *where_clause);

/* Check if WHERE clause contains encrypted columns */
bool tecc_where_clause_processor_has_encrypted_columns(Node *where_clause,
                                                       TeccStatementData *statement_data);

/* Rewrite WHERE clause in SQL query */
TeccWhereClauseProcessorResult tecc_where_clause_processor_rewrite_clause(TeccStatementData *statement_data);

/* Set operator type for SEARCH algorithm interface extension */
TeccWhereClauseProcessorResult tecc_where_clause_processor_set_operator_type(
    TeccStatementData *statement_data,
    const char *operator_name);

#endif /* WHERE_CLAUSE_PROCESSOR_H */
/* End - ReqID:TECC4.0 */