/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_processor.h
 * Abstract:
 *   Unified TECC frontend processor for encrypted column identification and SQL statement processing.
 *   Based on modern client logic design, provides:
 *   - Pre-query processing for encrypted columns
 *   - Pre-execution processing for parameter encryption
 *   - Post-query processing for cache updates
 *   - Integration with TECC parser and metadata cache
 *
 * Authored by <wangxp@highgo.com>, 20260314.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/tecc_processor.h
 */

#ifndef TECC_PROCESSOR_H
#define TECC_PROCESSOR_H

/* Include type definitions first */
#include "tecc_processor_types.h"

/* Include PostgreSQL frontend headers */
#include "postgres_fe.h"

/* Include frontend parser adapter */
#include "../frontend_parser/fe_adapter.h"

/* Include common types for enhanced COPY state */
#include "../tecc_common/tecc_common_types.h"

/* Include new submodule headers */
#include "prepared_statement.h"
#include "prepared_statements_list.h"
#include "pre_stmt_processor.h"
#include "post_stmt_processor.h"

/* Begin - ReqID:TECC4.0 - 复杂参数处理和硬编码值加密支持 */
/* Include complex parameter processing and constant extraction headers */
#include "param_expression_processor.h"
#include "constant_extractor.h"
#include "algorithm_selector.h"
/* End - ReqID:TECC4.0 - 复杂参数处理和硬编码值加密支持 */

/* Statement data structure - defined in tecc_common_types.h */
typedef struct TeccStatementData TeccStatementData;

/* Use existing PGconn definition from libpq-fe.h */
/* Forward declarations for adapter types */
typedef struct Node Node;
typedef struct List List;

/* Forward declarations - defined in types header */

/* Processor result codes - defined in types header */

/* Processor result codes - defined in types header */

/* Initialize processor context */
void tecc_processor_init(PGconn *conn);

/* Clean up processor context */
void tecc_processor_cleanup(PGconn *conn);

/* Run pre-query processing */
TeccProcessorResult tecc_processor_run_pre_query(TeccStatementData *statement_data);

/* Run pre-execution processing */
TeccProcessorResult tecc_processor_run_pre_exec(TeccStatementData *statement_data);

/* Run post-query processing */
TeccProcessorResult tecc_processor_run_post_query(PGconn *conn, PGresult *result);

/* Process specific statement types */
TeccProcessorResult tecc_processor_run_pre_statement(const Node *stmt, TeccStatementData *statement_data);

/* Process SELECT statement */
TeccProcessorResult tecc_processor_run_pre_select_statement(const SelectStmt *select_stmt,
                                                           TeccStatementData *statement_data);

/* Process INSERT statement */
TeccProcessorResult tecc_processor_run_pre_insert_statement(const InsertStmt *insert_stmt,
                                                           TeccStatementData *statement_data);

/* Process UPDATE statement */
TeccProcessorResult tecc_processor_run_pre_update_statement(const UpdateStmt *update_stmt,
                                                           TeccStatementData *statement_data);

/* Process DELETE statement */
TeccProcessorResult tecc_processor_run_pre_delete_statement(const DeleteStmt *delete_stmt,
                                                           TeccStatementData *statement_data);

/* Process CREATE KEK statement */
TeccProcessorResult tecc_processor_run_pre_create_kek_statement(const CreateKekStmt *create_kek_stmt,
                                                               TeccStatementData *statement_data);

/* Process CREATE DEK statement */
TeccProcessorResult tecc_processor_run_pre_alter_kek_statement(const AlterKekStmt *alter_kek_stmt,
                                           TeccStatementData *statement_data);
TeccProcessorResult tecc_processor_run_pre_create_dek_statement(const CreateDekStmt *create_dek_stmt,
                                                               TeccStatementData *statement_data);

/* Process DROP statement */
TeccProcessorResult tecc_processor_run_pre_drop_statement(const DropStmt *drop_stmt,
                                                         TeccStatementData *statement_data);

/* Process COPY statement */
TeccProcessorResult tecc_processor_run_pre_copy_statement(const CopyStmt *copy_stmt,
                                                         TeccStatementData *statement_data);

/* Check if statement contains encrypted columns */
int tecc_processor_has_encrypted_columns(TeccStatementData *statement_data);

/* ReqID:TECC4.0 - 解析表引用的真实 schema（未限定表名按 search_path 服务器解析，
 * 临时表清单优先）。返回指针在语句处理期间有效。 */
const char *tecc_stmt_resolve_table_schema(const TeccStatementData *statement_data,
                                           const char *schemaname,
                                           const char *relname);

/* Get list of encrypted column names */
List *tecc_processor_get_encrypted_columns(TeccStatementData *statement_data);

/* Statement utility functions */
bool tecc_stmt_has_encrypted_columns(TeccStatementData *stmt_data);
int tecc_stmt_get_encrypted_column_count(TeccStatementData *stmt_data);
bool tecc_stmt_needs_rewrite(TeccStatementData *stmt_data);
void tecc_stmt_set_needs_rewrite(TeccStatementData *stmt_data, bool needs_rewrite);

/* Process statement (legacy function) */
List *tecc_process_statement(TeccStatementData *stmt_data);

/* Set processor debug mode */
void tecc_processor_set_debug(int enable);

/* Set connection-specific debug level (thread-safe version) */
void tecc_state_set_debug(PGconn *conn, int enable);

/* get processor statistics */
void tecc_processor_get_stats(int *processed_statements, int *error_count);

/* SQL rewrite functionality is now distributed to submodules:
 * - values_processor: parameterized query rewrite
 * - where_clause_processor: WHERE/HAVING clause rewrite
 * - stmt_processor: CREATE VIEW statement rewrite
 */

/* Unified decryption interface for external modules */
extern char *tecc_processor_decrypt_value(PGconn *conn, const char *encrypted_value, Oid type_id);

/* New submodule interface functions (v4.0 additions) */

/* Function processor interface */
extern int tecc_func_processor_handle_function_call(
    PGconn *conn,
    const Node *func_call_node,
    TeccStatementData *statement_data);

/* Prepared statement interface */
extern TeccPreparedStatementResult tecc_prepared_statement_handle_prepare(
    PGconn *conn,
    const char *stmt_name,
    const char *query_string,
    TeccStatementData *statement_data);

extern TeccPreparedStatementResult tecc_prepared_statement_handle_execute(
    PGconn *conn,
    const char *stmt_name,
    int nParams,
    const Oid *paramTypes,
    const char * const * paramValues,
    const int *paramLengths,
    const int *paramFormats,
    TeccStatementData *statement_data);

/* Pre-statement processor interface */
extern TeccPreStmtProcessorResult tecc_pre_stmt_processor_run(
    PGconn *conn,
    TeccStatementData *stmt_data);

/* Post-statement processor interface */
extern TeccPostStmtProcessorResult tecc_post_stmt_processor_run(
    PGconn *conn,
    TeccStatementData *stmt_data,
    PGresult *query_result);

/* Prepared statements list interface */
extern TeccPreparedStatementsList *tecc_prepared_statements_list_get_default(PGconn *conn);

extern TeccPreparedStatementsListResult tecc_prepared_statements_list_handle_add(
    PGconn *conn,
    const char *stmt_name,
    const char *query_string,
    TeccStatementData *statement_data);

/* Statement data cleanup function */
extern void tecc_processor_cleanup_statement_data(TeccStatementData *statement_data);


/* ==================== COPY Row Processing Interfaces ==================== */
/* Begin - ReqID:TECC4.0 - COPY row processing interfaces for import/export module */

/* Forward declaration for enhanced COPY state */
typedef struct TeccEnhancedCopyState TeccEnhancedCopyState;

/* Process COPY data row for encryption (import) */
extern TeccProcessorResult tecc_processor_process_copy_row(
    TeccEnhancedCopyState *copy_state,
    const char *row_data,
    int row_len,
    char **processed_row,
    int *processed_len);

/* Process COPY result row for decryption (export) */
extern TeccProcessorResult tecc_processor_process_copy_result_row(
    TeccEnhancedCopyState *copy_state,
    const char *encrypted_row,
    int row_len,
    char **decrypted_row,
    int *decrypted_len);

/* Get encrypted column information for COPY processing */
extern TeccProcessorResult tecc_processor_get_copy_encrypted_columns(
    TeccEnhancedCopyState *copy_state,
    TeccEncryptedColumnMetadata ***column_info,
    int *column_count);

/* Process batch of COPY rows for encryption */
extern TeccProcessorResult tecc_processor_process_copy_batch(
    TeccEnhancedCopyState *copy_state,
    const char **row_data_array,
    const int *row_len_array,
    int row_count,
    char ***processed_rows,
    int **processed_len_array);

/* Process batch of COPY result rows for decryption */
extern TeccProcessorResult tecc_processor_process_copy_result_batch(
    TeccEnhancedCopyState *copy_state,
    const char **encrypted_row_array,
    const int *row_len_array,
    int row_count,
    char ***decrypted_rows,
    int **decrypted_len_array);

/* Validate COPY row format */
extern TeccProcessorResult tecc_processor_validate_copy_row(
    TeccEnhancedCopyState *copy_state,
    const char *row_data,
    int row_len,
    int *field_count,
    char ***field_values,
    int **field_lengths);

/* Parse CSV/TEXT format row */
extern TeccProcessorResult tecc_processor_parse_copy_row(
    TeccEnhancedCopyState *copy_state,
    const char *row_data,
    int row_len,
    int *field_count,
    char ***field_values,
    int **field_lengths);

/* Format CSV/TEXT row from field values */
extern TeccProcessorResult tecc_processor_format_copy_row(
    TeccEnhancedCopyState *copy_state,
    const char **field_values,
    const int *field_lengths,
    int field_count,
    char **formatted_row,
    int *formatted_len);

/* Get field value for specific column (considering encryption) */
extern TeccProcessorResult tecc_processor_get_copy_field_value(
    TeccEnhancedCopyState *copy_state,
    const char **field_values,
    const int *field_lengths,
    int field_count,
    int column_index,
    char **field_value,
    int *field_len,
    bool *is_encrypted);

/* Set field value for specific column (with encryption if needed) */
extern TeccProcessorResult tecc_processor_set_copy_field_value(
    TeccEnhancedCopyState *copy_state,
    char ***field_values,
    int **field_lengths,
    int *field_count,
    int column_index,
    const char *new_value,
    int new_value_len);

/* End - ReqID:TECC4.0 - COPY row processing interfaces for import/export module */

#endif /* TECC_PROCESSOR_H */
/* End - ReqID:TECC4.0 */