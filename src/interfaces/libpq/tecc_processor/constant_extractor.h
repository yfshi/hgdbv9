/* Begin - ReqID:TECC4.0 */
/*
 * File: constant_extractor.h
 *
 * Abstract:
 *   Constant extractor for TECC module.
 *   Provides functions for extracting and processing hardcoded values from complex expressions.
 *
 * Authored by <wangxp@highgo.com>, 20260418.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/constant_extractor.h
 */

#ifndef CONSTANT_EXTRACTOR_H
#define CONSTANT_EXTRACTOR_H

#include "tecc_processor_types.h"
#include "../tecc_common/tecc_common_types.h"

/* Include frontend parser types */
#include "../frontend_parser/tecc_nodes.h"

/*
 * Function prototypes for constant extraction and processing
 */

/* Extract all constants from an expression tree */
List *tecc_extract_constants_from_expression(const Node *expr);

/* Extract constants from WHERE clause with enhanced logic */
List *tecc_extract_constants_from_where_clause(const Node *where_clause,
                                              TeccOperatorType operator_type);

/* Check if constant needs encryption */
bool tecc_constant_needs_encryption(const TeccConstantInfo *constant_info,
                                   const TeccStatementData *stmt_data);

/* Select appropriate encryption algorithm for constant */
const char *tecc_select_encryption_algorithm(const TeccConstantInfo *constant_info,
                                            const void *column_metadata);

/* Encrypt constant value */
TeccProcessorResult tecc_encrypt_constant_value(TeccConstantInfo *constant_info,
                                               const void *column_metadata,
                                               PGconn *conn);

/* Process all constants in statement */
TeccProcessorResult tecc_process_constants_in_statement(TeccStatementData *stmt_data);

/* Create constant information from AST node */
TeccConstantInfo *tecc_create_constant_info_from_node(const Node *constant_node,
                                                     const Node *parent_expr,
                                                     TeccOperatorType operator_type);

/* Free constant information */
void tecc_free_constant_info(TeccConstantInfo *constant_info);

/* Free constant information list */
void tecc_free_constant_info_list(List *constant_list);

/* Batch encrypt constants for performance optimization */
TeccProcessorResult tecc_batch_encrypt_constants(List *constant_list,
                                                const void *column_metadata,
                                                PGconn *conn);

/* Validate constant format and type */
bool tecc_validate_constant(const TeccConstantInfo *constant_info);

/* Convert constant to string representation */
char *tecc_constant_to_string(const TeccConstantInfo *constant_info);

/* Debug function: print constant information */
void tecc_debug_print_constant_info(const TeccConstantInfo *constant_info);

#endif /* CONSTANT_EXTRACTOR_H */
/* End - ReqID:TECC4.0 */