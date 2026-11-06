/* Begin - ReqID:TECC4.0 */
/*
 * File: param_expression_processor.h
 *
 * Abstract:
 *   Parameter expression processor for TECC module.
 *   Provides functions for processing complex parameters in nested expressions.
 *
 * Authored by <wangxp@highgo.com>, 20260418.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/param_expression_processor.h
 */

#ifndef PARAM_EXPRESSION_PROCESSOR_H
#define PARAM_EXPRESSION_PROCESSOR_H

#include "tecc_processor_types.h"
#include "../tecc_common/tecc_common_types.h"

/* Include frontend parser types */
#include "../frontend_parser/tecc_nodes.h"

/*
 * Parameter information structure for complex parameter handling
 */
typedef struct TeccParamInfo
{
    int param_index;                    /* Parameter index (0-based) */
    void *param_node;                   /* Parameter AST node (opaque pointer) */
    int expression_depth;               /* Depth in expression tree */
    TeccValueType value_type;           /* Inferred value type */
    bool is_in_function_call;           /* Whether parameter is in function call */
    bool is_in_subquery;                /* Whether parameter is in subquery */
    bool is_in_op_expr;                 /* Whether parameter is in operator expression */
    void *parent_context;               /* Parent expression context (opaque pointer) */
} TeccParamInfo;

/*
 * Function prototypes for complex parameter processing
 */

/* Find all parameters in an expression tree */
List *tecc_find_params_in_expression(const Node *expr, int depth);

/* Find parameters in function call */
List *tecc_find_params_in_func_call(const void *func_call_node, int depth);

/* Find parameters in subquery */
List *tecc_find_params_in_subquery(const void *subquery_node, int depth);

/* Find parameters in operator expression */
List *tecc_find_params_in_op_expr(const void *op_expr_node, int depth);

/* Extract parameter mapping from parse tree with enhanced logic */
int tecc_extract_param_mapping_enhanced(TeccStatementData *stmt_data);

/* Infer parameter type based on context */
Oid tecc_infer_param_type(const TeccParamInfo *param_info,
                         const TeccStatementData *stmt_data);

/* Check if parameter needs encryption with enhanced logic */
bool tecc_param_needs_encryption_enhanced(const TeccParamInfo *param_info,
                                         const TeccStatementData *stmt_data);

/* Process complex parameters in statement */
TeccProcessorResult tecc_process_complex_parameters(TeccStatementData *stmt_data);

/* Create enhanced parameter map entry */
TeccParamMapEntry *tecc_create_enhanced_param_entry(const TeccParamInfo *param_info,
                                                   const TeccStatementData *stmt_data);

/* Free parameter information list */
void tecc_free_param_info_list(List *param_list);

/* Debug function: print parameter information */
void tecc_debug_print_param_info(const TeccParamInfo *param_info);

#endif /* PARAM_EXPRESSION_PROCESSOR_H */
/* End - ReqID:TECC4.0 */