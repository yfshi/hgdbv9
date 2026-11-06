/* Begin - ReqID:TECC4.0 */
/*
 * File: algorithm_selector.h
 *
 * Abstract:
 *   Algorithm selector for TECC module.
 *   Provides intelligent algorithm selection for encryption based on context.
 *
 * Authored by <wangxp@highgo.com>, 20260418.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/algorithm_selector.h
 */

#ifndef ALGORITHM_SELECTOR_H
#define ALGORITHM_SELECTOR_H

#include "tecc_processor_types.h"
#include "../tecc_common/tecc_common_types.h"

/* Include common algorithm types */
#include "../../../common/tecc_types.h"

/*
 * Algorithm selection context structure
 */
typedef struct TeccAlgorithmSelectionContext
{
    /* Input parameters */
    TeccOperatorType operator_type;          /* Operator type */
    TeccEncryptionType column_encryption_type; /* Column encryption type */
    TeccValueType value_type;                /* Value type */
    bool is_in_where_clause;                 /* Whether in WHERE clause */
    bool is_in_function_arg;                 /* Whether in function argument */

    /* Output selection */
    TeccAlgorithmType selected_algorithm;    /* Selected algorithm */
    const char *algorithm_name;              /* Algorithm name string */

    /* Selection rationale */
    char selection_reason[256];              /* Reason for algorithm selection */
} TeccAlgorithmSelectionContext;

/*
 * Function prototypes for algorithm selection
 */

/* Select encryption algorithm based on context */
TeccAlgorithmType tecc_select_algorithm(const TeccAlgorithmSelectionContext *context);

/* Create algorithm selection context from constant info */
TeccAlgorithmSelectionContext *tecc_create_algorithm_context_from_constant(
    const TeccConstantInfo *constant_info,
    const void *column_metadata);

/* Create algorithm selection context from parameter info */
TeccAlgorithmSelectionContext *tecc_create_algorithm_context_from_param(
    const TeccParamMapEntry *param_entry,
    const void *column_metadata);

/* Validate algorithm selection */
bool tecc_validate_algorithm_selection_for_context(TeccAlgorithmType algorithm,
                                                  const TeccAlgorithmSelectionContext *context);

/* Get algorithm name string */
const char *tecc_get_algorithm_name(TeccAlgorithmType algorithm);

/* Get algorithm type from name */
TeccAlgorithmType tecc_get_algorithm_type_from_name(const char *algorithm_name);

/* Check algorithm compatibility with operator */
bool tecc_check_algorithm_operator_compatibility(TeccAlgorithmType algorithm,
                                                TeccOperatorType operator_type);

/* Check algorithm compatibility with encryption type */
bool tecc_check_algorithm_encryption_compatibility(TeccAlgorithmType algorithm,
                                                  TeccEncryptionType encryption_type);

/* Get default algorithm for scenario */
TeccAlgorithmType tecc_get_default_algorithm(TeccOperatorType operator_type,
                                            TeccEncryptionType encryption_type);

/* Free algorithm selection context */
void tecc_free_algorithm_context(TeccAlgorithmSelectionContext *context);

/* Debug function: print algorithm selection context */
void tecc_debug_print_algorithm_context(const TeccAlgorithmSelectionContext *context);

#endif /* ALGORITHM_SELECTOR_H */
/* End - ReqID:TECC4.0 */