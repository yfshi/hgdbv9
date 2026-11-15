/* Begin - ReqID:TECC4.0 */
/*
 * File: create_stmt_processor.h
 *
 * Abstract:
 *   CREATE statement processor submodule for TECC processor module.
 *   Handles processing of CREATE TABLE statements and related DDL
 *   operations for encrypted columns.
 *
 * Authored by <wangxp@highgo.com>, 20260403.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_processor/create_stmt_processor.h
 */

#ifndef CREATE_STMT_PROCESSOR_H
#define CREATE_STMT_PROCESSOR_H

#include "tecc_processor.h"
#include "internal/tecc_processor_internal.h"
#include "../frontend_parser/fe_adapter.h"  /* For PostgreSQL types */
#include "../frontend_parser/tecc_nodes.h"  /* For constraint type constants */

/* Forward declarations for CREATE statement types */
typedef struct CreateStmt CreateStmt;
typedef struct ColumnDef ColumnDef;

/* Constraint struct is defined in tecc_nodes.h */

/*
 * CREATE statement processor result codes
 */
typedef enum TeccCreateStmtProcessorResult
{
    TECC_CREATE_PROCESSOR_SUCCESS = 0,
    TECC_CREATE_PROCESSOR_ERROR = -1,
    TECC_CREATE_PROCESSOR_INVALID_CONSTRAINT = -2,
    TECC_CREATE_PROCESSOR_UNSUPPORTED_TYPE = -3,
    TECC_CREATE_PROCESSOR_DUPLICATE_COLUMN = -4
} TeccCreateStmtProcessorResult;

/*
 * CREATE statement processing context
 */
typedef struct TeccCreateStmtProcessorContext
{
    char *table_name;
    char *schema_name;
    List *columns;
    List *constraints;
    List *encrypted_columns;
    bool has_encrypted_columns;
    bool needs_dek_creation;
    List *default_encryptions;  /* ReqID:TECC4.0 - DEFAULT 加密信息列表 */
    List *bound_encryptions;   /* ReqID:TECC4.0 - 分区边界加密信息列表 */
    int current_attnum;        /* ReqID:TECC4.0 - Current column attnum (1-based) for pending_tables */
} TeccCreateStmtProcessorContext;

/*
 * CREATE statement processor interface functions
 */

/* Main CREATE statement processing entry point */
TeccCreateStmtProcessorResult tecc_create_stmt_processor_run(const CreateStmt *create_stmt,
                                                              TeccStatementData *statement_data);

/* Process CREATE TABLE statement */
TeccCreateStmtProcessorResult tecc_create_stmt_processor_process_table(const CreateStmt *create_stmt,
                                                                        TeccStatementData *statement_data);

/* Check column definition */
TeccCreateStmtProcessorResult tecc_create_stmt_processor_check_column(const ColumnDef *column_def,
                                                                       TeccCreateStmtProcessorContext *context,
                                                                       TeccStatementData *statement_data);

/* Check if column type supports encryption */
bool tecc_create_stmt_processor_type_supports_encryption(const char *type_name);

/* Get encryption algorithm for column type */
const char *tecc_create_stmt_processor_get_column_algorithm(const char *type_name);

/*
 * ReqID:TECC4.0 - DEFAULT encryption info for CREATE TABLE encrypted columns
 */
typedef struct TeccDefaultEncInfo
{
    char *column_name;          /* Column name */
    char *plaintext_value;      /* Original DEFAULT value text in SQL */
    char *encrypted_hex;        /* Encrypted hex string */
    char *column_type;          /* Encrypted column type name (e.g. int4_enc) */
    int value_location;         /* Byte offset of value in original SQL */
    int value_length;           /* Byte length of value text in original SQL */
} TeccDefaultEncInfo;


/*
 * ReqID:TECC4.0 - Partition boundary encryption info
 */
typedef struct TeccBoundEncInfo
{
    char   *plaintext_value;    /* Original plaintext value */
    char   *encrypted_hex;      /* Encrypted hex string */
    char   *column_type;        /* Encrypted type name (e.g. "int2_enc", "text_enc") */
    int     value_location;     /* Byte offset of value in original SQL */
    int     value_length;       /* Byte length of value text in original SQL */
} TeccBoundEncInfo;

/* ReqID:TECC4.0 - Forward declarations for types from tecc_metadata_cache.h */
struct TeccColumnCacheItem;
struct TeccState;

/*
 * ReqID:TECC4.0 - Shared boundary value encryption helper.
 *
 * Encrypts a single partition boundary value. Extracts plaintext from
 * a Const node, looks up DEK/KEK from cache, and calls the encryption
 * scheduler. Used by both CREATE TABLE ... PARTITION OF and
 * ALTER TABLE ... ATTACH PARTITION to avoid code duplication.
 *
 * Returns a pg_malloc'd TeccBoundEncInfo on success (caller owns it).
 * On non-fatal skip (null value, missing DEK, etc.): returns NULL, *is_error=false.
 * On fatal error: returns NULL, *is_error=true, error set via tecc_processor_set_error.
 */
extern TeccBoundEncInfo *tecc_encrypt_boundary_value(
    PGconn *conn,
    struct TeccState *state,
    Const *c,
    struct TeccColumnCacheItem *col_cache,
    const char *table_name,
    const char *column_name,
    bool *is_error,
    TeccStatementData *statement_data);

/* Free a TeccBoundEncInfo returned by tecc_encrypt_boundary_value */
extern void tecc_free_bound_enc_info(TeccBoundEncInfo *info);

/* ReqID:TECC4.0 - Query partition key column names (shared with alter_stmt_processor) */
extern char **tecc_query_partkey_columns(PGconn *conn,
                                         const char *schema_name,
                                         const char *table_name,
                                         int *ncols);

/* ReqID:TECC4.0 - Map encrypted type name to base type OID (e.g. "int4_enc"->INT4OID) */
extern Oid map_enc_type_name_to_base_oid(const char *type_name);

/* ReqID:TECC4.0 - 解析"目标列还不存在"的加密列将来生效的 collation locale 文本
 * （CTAS、物化视图/视图定义里的带 ENCRYPTED WITH cast、加密列 DEFAULT 共用）。
 * 有 COLLATE 子句时按它解析，否则文本类型取库默认排序规则的 collcollate；
 * 非文本类型或解析失败返回 NULL（调用方按"拿不到 locale"处理，行为同修复前）。
 * 返回值 pg_strdup，调用方负责 pg_free。 */
extern char *resolve_new_column_collation_locale(PGconn *conn, Oid base_oid,
                                                 const CollateClause *coll_clause);

/* ReqID:TECC4.0 - 从 ENCRYPTED WITH 选项里取同态 DEK 名（HE_ADD_DEK / HE_MUL_DEK），
 * 供"目标列还不存在"的写入路径（CTAS 的结果表列、视图/物化视图定义里的 cast、
 * 加密列 DEFAULT）把同态 DEK 传给加密入口 —— 这些路径拿不到列元数据，同态 DEK
 * 没有别的来源。返回解析树内的字符串指针（不复制、不要释放），没有则置 NULL。 */
extern void tecc_typename_get_he_dek_names(const TypeName *type_name,
                                           const char **he_add_dek_name,
                                           const char **he_mul_dek_name);
#endif /* CREATE_STMT_PROCESSOR_H */
/* End - ReqID:TECC4.0 */