/* Begin - ReqID:TECC4.0 */
/*
 * File: stmt_processor.h
 *
 * Abstract:
 *   Statement processor submodule for TECC processor module.
 *   Handles SQL statement type identification and routing to
 *   appropriate specialized processors.
 *
 * Authored by <wangxp@highgo.com>, 20260403.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/stmt_processor.h
 */

#ifndef STMT_PROCESSOR_H
#define STMT_PROCESSOR_H

#include "tecc_processor.h"
#include "internal/tecc_processor_internal.h"

/* ReqID:TECC4.0 - Error recording function from fe-tecc-integration */
extern void tecc_record_error(struct TeccState *state, int error_code,
                              const char *error_msg, const char *function_name,
                              int line_number, bool recoverable);

/*
 * Statement processing result codes
 */
typedef enum TeccStmtProcessorResult
{
    TECC_STMT_PROCESSOR_SUCCESS = 0,
    TECC_STMT_PROCESSOR_ERROR = -1,
    TECC_STMT_PROCESSOR_UNSUPPORTED_STMT = -2,
    TECC_STMT_PROCESSOR_NO_ENCRYPTED_COLUMNS = -3
} TeccStmtProcessorResult;

/*
 * Statement type enumeration - defined in tecc_common_types.h
 * This is a forward declaration to avoid duplicate definition
 */
typedef enum TeccStatementType TeccStatementType;

/*
 * Statement processing context
 */
typedef struct TeccStmtProcessorContext
{
    TeccStatementType stmt_type;
    List *encrypted_columns;
    bool needs_rewrite;
    void *stmt_specific_context;
} TeccStmtProcessorContext;

/*
 * Statement processor interface functions
 */

/* Main statement processing entry point */
TeccStmtProcessorResult tecc_stmt_processor_run_pre_query(TeccStatementData *statement_data);

/* Statement-specific processing */
TeccStmtProcessorResult tecc_stmt_processor_run_pre_statement(const Node *stmt,
                                                             TeccStatementData *statement_data);

/* Process SELECT statement */
TeccStmtProcessorResult tecc_stmt_processor_process_select(const SelectStmt *select_stmt,
                                                          TeccStatementData *statement_data);

/* Process INSERT statement */
TeccStmtProcessorResult tecc_stmt_processor_process_insert(const InsertStmt *insert_stmt,
                                                          TeccStatementData *statement_data);

/* Process UPDATE statement */
TeccStmtProcessorResult tecc_stmt_processor_process_update(const UpdateStmt *update_stmt,
                                                          TeccStatementData *statement_data);

/* Process DELETE statement */
TeccStmtProcessorResult tecc_stmt_processor_process_delete(const DeleteStmt *delete_stmt,
                                                          TeccStatementData *statement_data);

/* Process CREATE KEK statement */
TeccStmtProcessorResult tecc_stmt_processor_process_create_kek(const CreateKekStmt *create_kek_stmt,
                                                              TeccStatementData *statement_data);

/* Process CREATE DEK statement */
TeccStmtProcessorResult tecc_stmt_processor_process_create_dek(const CreateDekStmt *create_dek_stmt,
                                                              TeccStatementData *statement_data);

/* Process CREATE TABLE statement */
TeccStmtProcessorResult tecc_stmt_processor_process_create_table(const CreateStmt *create_stmt,
                                                                TeccStatementData *statement_data);

/* Process DROP statement */
TeccStmtProcessorResult tecc_stmt_processor_process_drop(const DropStmt *drop_stmt,
                                                        TeccStatementData *statement_data);

/* Process COPY statement */
TeccStmtProcessorResult tecc_stmt_processor_process_copy(const CopyStmt *copy_stmt,
                                                        TeccStatementData *statement_data);

/* Process ALTER TABLE statement */
TeccStmtProcessorResult tecc_stmt_processor_process_alter_table(const AlterTableStmt *alter_stmt,
                                                                TeccStatementData *statement_data);

/* Utility functions */
TeccStatementType tecc_stmt_processor_identify_statement(const Node *stmt);
bool tecc_stmt_processor_statement_has_dml(const Node *stmt);
bool tecc_stmt_processor_statement_has_encrypted_columns(const Node *stmt);

/* ReqID:TECC4.0 - find end of TypeCast expression (including ENCRYPTED WITH) */
size_t find_typecast_expr_end(const char *query, const TypeCast *type_cast);

/* Process constants in FuncCall arguments that appear alongside encrypted columns */
void stmt_process_funcall_constants(FuncCall *fc, TeccStatementData *statement_data);

#endif /* STMT_PROCESSOR_H */
/* End - ReqID:TECC4.0 */