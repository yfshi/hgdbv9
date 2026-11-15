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

/* End - ReqID:TECC4.0 */

/* End - ReqID:TECC4.0 */

/* Process SELECT statement */
TeccStmtProcessorResult tecc_stmt_processor_process_select(const SelectStmt *select_stmt,
                                                          TeccStatementData *statement_data);

/* ReqID:TECC4.0 - 按密态类型名 + ENCRYPTED WITH 里的 SUPPORT_ALGO 解析该 cast
 * 要用的加密算法（det/ope/search/he-add/he-mul）。未写 SUPPORT_ALGO 时按类型给
 * 默认值（int2/4/8→det；float4/8、numeric→ope；text/varchar/bpchar→det）。
 * CTAS 的密态 cast 处理也复用它，避免同一份声明在不同写入路径解出不同算法。 */
void get_typecast_algo_flags(const TypeCast *type_cast,
                             const char *typename_str,
                             bool *supports_det,
                             bool *supports_ope,
                             bool *supports_search,
                             bool *supports_he_add,
                             bool *supports_he_mul);

/* Process CREATE VIEW statement - ReqID:TECC4.0 密态视图支持 */
TeccStmtProcessorResult tecc_stmt_processor_process_view(const ViewStmt *view_stmt,
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

/* Process ALTER KEK statement */
TeccStmtProcessorResult tecc_stmt_processor_process_alter_kek(const AlterKekStmt *alter_kek_stmt,
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

/* End - ReqID:TECC4.0 */

/* ReqID:TECC4.0 - find end of TypeCast expression (including ENCRYPTED WITH) */
size_t find_typecast_expr_end(const char *query, const TypeCast *type_cast);

/* Process constants in FuncCall arguments that appear alongside encrypted columns */
void stmt_process_funcall_constants(FuncCall *fc, TeccStatementData *statement_data);

/* Validate aggregate function call against column encryption algorithms.
 * Returns true if valid or pass-through; false if an error is set. */
bool stmt_validate_aggregate_funcall(FuncCall *fc, TeccStatementData *statement_data);

/* Validate scalar function call against encrypted column arguments.
 * Returns true if valid or pass-through; false if an error is set. */
bool stmt_validate_scalar_funcall(FuncCall *fc, TeccStatementData *statement_data);

/* ReqID:TECC4.0 - 需求 4.4.1.4：coalesce/greatest/least/nullif/decode/power
 * 参数组合统一校验（固定格式错误信息 F 模板）。返回 false 表示已设置错误，
 * 调用方须中止。 */
bool stmt_validate_scalar_arg_combos(FuncCall *fc, TeccStatementData *statement_data);

/* 节点树中是否含"裸 ::密态类型 cast"（普通常量 cast 到密态类型且未带
 * encrypted with 选项；\x 密文字面量与 NULL cast 除外）。 */
bool stmt_node_has_bare_enc_cast(Node *node, TeccStatementData *statement_data);

/* F 模板 locale 检测与文案（zh_* → 中文，其余 → 英文）。 */
bool stmt_msg_locale_is_zh(void);
const char *stmt_f_template(void);
const char *stmt_f_reason_text(int reason_id);

/* F 模板原因枚举（文案表索引，与 stmt_processor.c 文案表对应） */
typedef enum
{
    STMT_F_REASON_BARE_CAST = 0,    /* 普通常量不能直接转换为密态类型 */
    STMT_F_REASON_ENC_PLAIN_MIX,    /* 密态列与普通列不能混用 */
    STMT_F_REASON_TYPE_COMBO,       /* 密态类型之间不支持该类型组合 */
    STMT_F_REASON_CONST_MISMATCH,   /* 常量类型与密态列类型不匹配 */
    STMT_F_REASON_ENC_PLAIN_CONST,  /* 密态类型常量与普通常量不能混用 */
    STMT_F_REASON_CASE_SWITCH,      /* switch 形式暂不支持 */
    STMT_F_REASON_CASE_BRANCH,      /* 分支常量类型无法按密态列加密（含 %s） */
    STMT_F_REASON_COUNT
} StmtFReason;

#endif /* STMT_PROCESSOR_H */
/* End - ReqID:TECC4.0 */