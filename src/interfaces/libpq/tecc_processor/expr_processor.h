/* Begin - ReqID:TECC4.0 */
/*
 * File: expr_processor.h
 *
 * Abstract:
 *   Expression processor interface for TECC expression processor module.
 *   Provides functions for expanding and processing SQL expression nodes.
 *
 * Authored by <wangxp@highgo.com>, 20260404.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_expression/expr_processor.h
 */

#ifndef TECC_EXPR_PROCESSOR_H
#define TECC_EXPR_PROCESSOR_H

#include <stdbool.h>
#include "../frontend_parser/fe_adapter.h"
#include "internal/tecc_processor_internal.h"

/* Frontend parser already provides node definitions in fe_adapter.h */
/* Use forward declarations for specific node types */
typedef struct ColumnRef ColumnRef;
typedef struct FuncCall FuncCall;
typedef struct A_Expr A_Expr;
typedef struct BoolExpr BoolExpr;
typedef struct ParamRef ParamRef;
typedef struct TypeCast TypeCast;
typedef struct ArrayExpr ArrayExpr;
typedef struct SetOperationStmt SetOperationStmt;

/* Expression processor error codes */
typedef enum TeccExpressionError
{
    TECC_EXPRESSION_SUCCESS = 0,
    TECC_EXPRESSION_ERROR = -1,
    TECC_EXPRESSION_MEMORY_ERROR = -2,
    TECC_EXPRESSION_INVALID_ARGUMENT = -3,
    TECC_EXPRESSION_SYNTAX_ERROR = -4,
    TECC_EXPRESSION_UNSUPPORTED_FEATURE = -5,
    TECC_EXPRESSION_COLUMN_NOT_FOUND = -6,
    TECC_EXPRESSION_ENCRYPTION_CHECK_FAILED = -7
} TeccExpressionError;

/*
 * Main expression expansion function
 */

/* Expand any expression node */
bool tecc_expr_expand(const Node *expr_node, TeccExprPartsList **expr_parts);

/* Expand expression with GUC context */
bool tecc_expr_expand_with_guc(const Node *expr_node,
                              TeccExpressionContext *expr_ctx,
                              TeccExprPartsList **expr_parts);

/* Expand column reference */
bool tecc_expand_column_ref(const struct ColumnRef *col_ref, TeccColumnRefData **col_data);

/* Expand column reference with state context */
bool tecc_expand_column_ref_with_state(const struct ColumnRef *col_ref,
                                      TeccColumnRefData **col_data,
                                      const TeccState *state);

/* Expand column reference with statement data context */
bool tecc_expand_column_ref_with_stmt_data(const struct ColumnRef *col_ref,
                                          TeccColumnRefData **col_data,
                                          const TeccStatementData *stmt_data);

/* Expand function name */
bool tecc_expand_function_name(const struct FuncCall *func_call, TeccFuncNameData **func_data);

/* Expand condition expression */
bool tecc_expand_condition_expr(const Node *expr, TeccExprPartsList **expr_parts);

/*
 * Expression parts list management
 */

/* Create expression parts list */
TeccExprPartsList *tecc_expr_parts_list_create(void);

/* Destroy expression parts list */
void tecc_expr_parts_list_destroy(TeccExprPartsList *list);

/* Add expression part to list */
bool tecc_expr_parts_list_add(TeccExprPartsList *list, TeccExprPart *part);

/* Get expression part from list */
TeccExprPart *tecc_expr_parts_list_get(const TeccExprPartsList *list, size_t index);

/* Get list size */
size_t tecc_expr_parts_list_size(const TeccExprPartsList *list);

/* Clear list */
void tecc_expr_parts_list_clear(TeccExprPartsList *list);

/*
 * Expression part management
 */

/* Create expression part */
TeccExprPart *tecc_expr_part_create(int type);

/* Destroy expression part */
void tecc_expr_part_destroy(TeccExprPart *part);

/* Set column reference data in expression part */
void tecc_expr_part_set_column_ref(TeccExprPart *part, TeccColumnRefData *col_data);

/* Set function name data in expression part */
void tecc_expr_part_set_func_name(TeccExprPart *part, TeccFuncNameData *func_data);

/* Get column reference data from expression part */
TeccColumnRefData *tecc_expr_part_get_column_ref(const TeccExprPart *part);

/* Get function name data from expression part */
TeccFuncNameData *tecc_expr_part_get_func_name(const TeccExprPart *part);

/* ==================== Expression GUC Integration Functions ==================== */

/* Initialize expression GUC processing */
bool tecc_expr_guc_init(TeccExpressionContext *expr_ctx);

/* Cleanup expression GUC processing */
void tecc_expr_guc_cleanup(TeccExpressionContext *expr_ctx);

/* Get current GUC parameters for expression processing */
const TeccGucParams *tecc_expr_get_guc_params(TeccExpressionContext *expr_ctx);

/* Apply GUC parameters to string literal
 * Note: Caller is responsible for freeing the returned string using pg_free()
 */
bool tecc_expr_apply_guc_to_string(TeccExpressionContext *expr_ctx,
                                  const char *str_literal,
                                  char **processed_str);

/* Apply GUC parameters to numeric constant
 * Note: Caller is responsible for freeing the returned string using pg_free()
 */
bool tecc_expr_apply_guc_to_numeric(TeccExpressionContext *expr_ctx,
                                   const char *numeric_str,
                                   char **processed_numeric);

/* Apply search path to unqualified name
 * Note: Caller is responsible for freeing the returned string using pg_free()
 */
bool tecc_expr_apply_search_path(TeccExpressionContext *expr_ctx,
                                const char *unqualified_name,
                                const char *object_type,
                                char **qualified_name);

/* Check if GUC parameters are available */
bool tecc_expr_guc_params_available(TeccExpressionContext *expr_ctx);

/* Enable/disable GUC parameter processing */
void tecc_expr_set_guc_processing(TeccExpressionContext *expr_ctx, bool enabled);

/* Update expression GUC parameters */
bool tecc_expr_update_guc_params(TeccExpressionContext *expr_ctx,
                                const TeccGucParams *new_params);

#endif /* TECC_EXPR_PROCESSOR_H */
/* End - ReqID:TECC4.0 */