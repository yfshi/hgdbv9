/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_processor_types.h
 *
 * Abstract:
 *   Type definitions for TECC processor module.
 *   Contains all public data structures and enums.
 *
 * Authored by <wangxp@highgo.com>, 20260403.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/tecc_processor_types.h
 */

#ifndef TECC_PROCESSOR_TYPES_H
#define TECC_PROCESSOR_TYPES_H

#include "../tecc_common/tecc_common_types.h"

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>


/* Basic type definitions */
typedef unsigned int Oid;

/* Forward declarations for frontend parser types */
typedef struct Node Node;
typedef struct List List;
typedef struct SelectStmt SelectStmt;
typedef struct InsertStmt InsertStmt;
typedef struct UpdateStmt UpdateStmt;
typedef struct DeleteStmt DeleteStmt;
typedef struct CreateKekStmt CreateKekStmt;
typedef struct CreateDekStmt CreateDekStmt;
typedef struct CreateStmt CreateStmt;
typedef struct DropStmt DropStmt;
typedef struct StringInfoData *StringInfo;
typedef struct ColumnRef ColumnRef;
typedef struct ResTarget ResTarget;
typedef struct ColumnDef ColumnDef;

/* Processor result codes - hierarchical error classification */
typedef enum TeccProcessorResult {
    /* Success */
    TECC_PROCESSOR_SUCCESS = 0,

    /* General errors */
    TECC_PROCESSOR_ERROR,

    /* Syntax layer errors (parsing, tokenization) */
    TECC_PROCESSOR_SYNTAX_ERROR,
    TECC_PROCESSOR_PARSE_ERROR,
    TECC_PROCESSOR_TOKENIZATION_ERROR,
    TECC_PROCESSOR_SQL_SYNTAX_ERROR,

    /* Semantic layer errors (validation, analysis) */
    TECC_PROCESSOR_SEMANTIC_ERROR,
    TECC_PROCESSOR_TYPE_MISMATCH,
    TECC_PROCESSOR_INVALID_INPUT,
    TECC_PROCESSOR_UNSUPPORTED_STATEMENT,
    TECC_PROCESSOR_UNSUPPORTED_OPERATION,
    TECC_PROCESSOR_INVALID_COLUMN_REFERENCE,
    TECC_PROCESSOR_INVALID_TABLE_REFERENCE,
    TECC_PROCESSOR_INVALID_FUNCTION_CALL,

    /* Data layer errors (encryption, decryption, data processing) */
    TECC_PROCESSOR_DATA_ERROR,
    TECC_PROCESSOR_ENCRYPTION_ERROR,
    TECC_PROCESSOR_DECRYPTION_ERROR,
    TECC_PROCESSOR_KEY_ERROR,
    TECC_PROCESSOR_ALGORITHM_ERROR,
    TECC_PROCESSOR_DATA_FORMAT_ERROR,
    TECC_PROCESSOR_DATA_SIZE_ERROR,

    /* Metadata and cache errors */
    TECC_PROCESSOR_METADATA_ERROR,
    TECC_PROCESSOR_CACHE_ERROR,
    TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS,
    TECC_PROCESSOR_MISSING_METADATA,
    TECC_PROCESSOR_INVALID_METADATA,

    /* System and resource errors */
    TECC_PROCESSOR_MEMORY_ERROR,
    TECC_PROCESSOR_RESOURCE_ERROR,
    TECC_PROCESSOR_IO_ERROR,
    TECC_PROCESSOR_NETWORK_ERROR,

    /* Internal errors */
    TECC_PROCESSOR_INTERNAL_ERROR,
    TECC_PROCESSOR_LOGIC_ERROR,
    TECC_PROCESSOR_ASSERTION_FAILED,

    /* Special processing states */
    TECC_PROCESSOR_NEEDS_REWRITE,
    TECC_PROCESSOR_PARTIAL_SUCCESS,
    TECC_PROCESSOR_WARNING
} TeccProcessorResult;

/* Column reference structure for internal use - DEPRECATED, use TeccColumnRefData instead */
typedef struct TeccColumnRef
{
    char *table_name;
    char *column_name;
    char *schema_name;
    char *catalog_name;
    int location;
    bool is_encrypted;
} TeccColumnRef;

/* Raw value types - now defined in tecc_common_types.h */
/* Forward declarations for raw value types */
typedef enum TeccRawValueType TeccRawValueType;
typedef enum TeccRawValueState TeccRawValueState;
typedef struct TeccRawValue TeccRawValue;
typedef struct TeccRawValuesList TeccRawValuesList;

/* Statement data structure is now defined in tecc_common_types.h */
/* typedef struct TeccStatementData TeccStatementData; */

/* Forward declaration for PGconn - use the correct type from libpq-fe.h */
/* typedef struct pg_conn PGconn; */  /* Already defined in libpq-fe.h */

/* ==================== Expression Processor Types ==================== */

/* Forward declarations for expression processor types */
typedef struct TeccExprPart TeccExprPart;
typedef struct TeccColumnRefData TeccColumnRefData;
typedef struct TeccFuncNameData TeccFuncNameData;
typedef struct TeccExprPartsList TeccExprPartsList;

/* Expression processor context structure */
typedef struct TeccExpressionContext
{
    /* Memory context */
    void *mem_ctx;                    /* Memory context (opaque pointer) */

    /* Expression processing state */
    TeccExprPartsList *expr_parts;    /* Expression parts list */

    /* GUC parameter integration */
    const TeccGucParams *guc_params; /* Current GUC parameters for expression processing */
    bool guc_params_enabled;          /* Whether GUC processing is enabled */
    const void *guc_params_cache;     /* GUC parameters cache pointer (const TeccState *) */

    /* Statistics */
    unsigned long guc_applications;   /* Number of GUC parameter applications */
    unsigned long guc_errors;         /* Number of GUC processing errors */

    /* Error handling */
    char error_message[256];          /* Error message buffer */
} TeccExpressionContext;

#endif /* TECC_PROCESSOR_TYPES_H */
/* End - ReqID:TECC4.0 */