/* Begin - ReqID:TECC4.0 */
/*
 * File: fe_adapter.h
 * Abstract:
 *   Frontend adapter layer for TECC simplified parser.
 *   Based on openGauss frontend parser analysis.
 *
 * Authored by <wangxp@highgo.com>, 20260306.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/frontend_parser/fe_adapter.h
 */

#ifndef FE_ADAPTER_H
#define FE_ADAPTER_H

#include "feparser_memutils.h"
/* #include "fe_bison_types_simple.h" */  /* 不再使用simple版本 */
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

/* 使用前向声明避免头文件冲突 */
typedef struct Node Node;
typedef struct List List;
typedef struct Value Value;
typedef struct ColumnRef ColumnRef;
typedef struct FuncCall FuncCall;
typedef struct A_Expr A_Expr;
typedef struct BoolExpr BoolExpr;
typedef struct A_Const A_Const;
typedef struct ParamRef ParamRef;
typedef struct TypeCast TypeCast;
typedef struct ArrayExpr ArrayExpr;
typedef struct SetOperationStmt SetOperationStmt;

/* PostgreSQL基础类型 */
typedef unsigned int Oid;
#define InvalidOid ((Oid) 0)
#ifndef NAMEDATALEN
#define NAMEDATALEN 64
#endif

/* NameData结构 - 使用PostgreSQL的定义，避免重复定义 */
/* PostgreSQL already defines NameData in c.h, so we don't redefine it here */
/* We use char arrays in our own structures to avoid dependency on NameData */
#ifndef NAMEDATALEN
#define NAMEDATALEN 64
#endif

/* ReqID:TECC4.0 - Node type constants */
/* Frontend parser node type constants - use FE_ prefix to avoid conflicts with PostgreSQL */
#define FE_T_Node 0
#define FE_T_List 1
#define FE_T_Integer 226
#define FE_T_String 228
#define FE_T_Float 227
#define FE_T_Null 230
#define FE_T_A_Const 356
#define FE_T_ColumnRef 354
#define FE_T_TypeCast 364
#define FE_T_A_Expr 353
#define FE_T_SetOp 10  /* 暂不确定后端值，保持原值 */
#define FE_T_ArrayExpr 137
#define FE_T_FuncCall 357
#define FE_T_DefElem 378
#define FE_T_CreateKekStmt 100
#define FE_T_CreateDekStmt 101
#define FE_T_DropStmt 102

/* DML statement node types */
#define FE_T_SelectStmt 241
#define FE_T_InsertStmt 238
#define FE_T_UpdateStmt 240
#define FE_T_DeleteStmt 239

/* Expression node types */
#define FE_T_Var 110
#define FE_T_Const 111
#define FE_T_OpExpr 212  /* 暂不确定后端值，保持原值 */
#define FE_T_BoolExpr 123
#define FE_T_NullTest 144
#define FE_T_Param 112
#define FE_T_ResTarget 362

/* Additional statement node types */
#define FE_T_PrepareStmt 220  /* 暂不确定后端值，保持原值 */
#define FE_T_ExecuteStmt 221  /* 暂不确定后端值，保持原值 */
#define FE_T_DeclareCursorStmt 222  /* 暂不确定后端值，保持原值 */
#define FE_T_CopyStmt 253
#define FE_T_GroupClause 1012

/* DDL statement node types */
#define FE_T_CreateStmt 254
#define FE_T_AlterTableStmt 244
#define FE_T_ColumnDef 374

/* RangeVar and Alias node types */
#define FE_T_Alias 106
#define FE_T_RangeVar 107

/* SortBy node type */
#define FE_T_SortBy 366

/* ReqID:TECC4.0 - Enum types - use PostgreSQL's definitions from nodes/parsenodes.h */
/* typedef enum DropBehavior - already defined in PostgreSQL headers */
/* typedef enum SortByDir - already defined in PostgreSQL headers */
/* typedef enum SortByNulls - already defined in PostgreSQL headers */
/* typedef enum SetQuantifier - already defined in PostgreSQL headers */

/* ObjectType values - aligned with PostgreSQL backend */
/* These values should match src/include/nodes/parsenodes.h */
/* Use PostgreSQL's definitions from nodes/parsenodes.h */


/* ReqID:TECC4.0 - DefElem Structure - use PostgreSQL's definition from nodes/parsenodes.h */
/* typedef struct DefElem - already defined in PostgreSQL headers */

/* ReqID:TECC4.0 - Create Statement Structures - use PostgreSQL's definitions from nodes/parsenodes.h */
/* typedef struct CreateKekStmt - already defined in PostgreSQL headers */
/* typedef struct CreateDekStmt - already defined in PostgreSQL headers */

/* ReqID:TECC4.0 - Drop Statement Structure - use PostgreSQL's definition from nodes/parsenodes.h */
/* typedef struct DropStmt - already defined in PostgreSQL headers */

/* ReqID:TECC4.0 - IntoClause Structure - use PostgreSQL's definition from nodes/parsenodes.h */
/* typedef struct IntoClause - already defined in PostgreSQL headers */

/* ReqID:TECC4.0 - DML Statement Structures - use PostgreSQL's definitions from nodes/parsenodes.h */
/* typedef struct SelectStmt - already defined in PostgreSQL headers */
/* typedef struct InsertStmt - already defined in PostgreSQL headers */
/* typedef struct UpdateStmt - already defined in PostgreSQL headers */
/* typedef struct DeleteStmt - already defined in PostgreSQL headers */

/* ReqID:TECC4.0 - Expression Node Structures - use PostgreSQL's definitions from nodes/primnodes.h */
/* typedef struct Var - already defined in PostgreSQL headers */
/* typedef struct Const - already defined in PostgreSQL headers */
/* typedef struct OpExpr - already defined in PostgreSQL headers */
/* typedef struct BoolExpr - already defined in PostgreSQL headers */
/* typedef struct NullTest - already defined in PostgreSQL headers */
/* typedef struct Param - already defined in PostgreSQL headers */

/* ReqID:TECC4.0 - Group Clause Structure - use PostgreSQL's definition from nodes/parsenodes.h */
/* typedef struct GroupClause - already defined in PostgreSQL headers */

/* ReqID:TECC4.0 - Additional Statement Structures - use PostgreSQL's definitions from nodes/parsenodes.h */
/* typedef struct PrepareStmt - already defined in PostgreSQL headers */
/* typedef struct ExecuteStmt - already defined in PostgreSQL headers */
/* typedef struct DeclareCursorStmt - already defined in PostgreSQL headers */
/* typedef struct CopyStmt - already defined in PostgreSQL headers */

/* ReqID:TECC4.0 - Alias and RangeVar - use PostgreSQL's definitions from nodes/primnodes.h */
/* typedef struct Alias - already defined in PostgreSQL headers */
/* typedef struct RangeVar - already defined in PostgreSQL headers */

/* ReqID:TECC4.0 - SortBy Structure - use PostgreSQL's definition from nodes/parsenodes.h */
/* typedef struct SortBy - already defined in PostgreSQL headers */

/* ReqID:TECC4.0 - Result Target Structure - use PostgreSQL's definition from nodes/parsenodes.h */
/* typedef struct ResTarget - already defined in PostgreSQL headers */

/* ReqID:TECC4.0 - Macro definitions */
/* Use PostgreSQL's definitions for makeNode, IsA, NIL */

/* ReqID:TECC4.0 - Function declarations */
#ifdef __cplusplus
extern "C" {
#endif

/* Use PostgreSQL's list functions from pg_list.h */

/* Value creation functions */
Node *makeIntConst(int64_t val, int location, int data_size);
Node *makeStringConst(const char *str, int location, int data_size);

/* RangeVar and Alias creation functions */
struct RangeVar *makeRangeVar(char *schemaname, char *relname, int location);
struct Alias *makeAlias(char *aliasname, List *colnames);

/* Validation functions - 暂时注释掉，因为core_yyscan_t类型未定义 */
/* void check_qualified_name(List *names, core_yyscan_t yyscanner); */
/* List *check_func_name(List *names, core_yyscan_t yyscanner); */
/* List *check_setting_name(List *names, core_yyscan_t yyscanner); */
/* List *check_indirection(List *indirection, core_yyscan_t yyscanner); */

/* TECC expression processing functions */
/* NameData and Oid types are defined in PostgreSQL headers (c.h, postgres_ext.h) */

typedef struct TeccColumnRefData {
    char catalog_name[NAMEDATALEN];      /* 目录名 */
    char schema_name[NAMEDATALEN];       /* 模式名 */
    char table_name[NAMEDATALEN];        /* 表名 */
    char table_alias[NAMEDATALEN];       /* 表别名，无别名时与 table_name 相同 */
    char column_name[NAMEDATALEN];       /* 列名 */
    char alias_fqdn[NAMEDATALEN*4];      /* 列全名别名 */
    bool is_encrypted;                   /* 是否为加密列 */
    Oid column_oid;                      /* 列OID */
    Oid table_oid;                       /* 表OID */
    int location;                        /* 在SQL中的位置 */
    /* Bug 9323 - 输出列重命名：子查询/CTE 可以用别的名字把加密列暴露给外层
     * （SELECT name AS n、WITH cte(n) AS ...、FROM (...) s(n)），外层只能按
     * 重命名后的名字引用该列。此处记暴露名，匹配时与 column_name 并列；
     * 列元数据/DEK 仍按 column_name + table_name 查，二者不可互换。
     * 位置固定在 location 之后，以保持与 TeccProcessorColumnRef 前缀布局一致。 */
    char column_alias[NAMEDATALEN];      /* 输出列别名，未重命名时为空 */
    char dek_name[NAMEDATALEN];          /* DEK名称（来自 ENCRYPTED WITH） */
    char support_algo[NAMEDATALEN];      /* 支持的密态查询类型（来自 ENCRYPTED WITH） */
    /* ReqID:TECC4.0 - 一列多 DEK：he-add/he-mul 各自的同态 DEK */
    char he_add_dek_name[NAMEDATALEN];   /* he-add DEK（EC-ElGamal，来自 HE_ADD_DEK） */
    char he_mul_dek_name[NAMEDATALEN];   /* he-mul DEK（FF ElGamal，来自 HE_MUL_DEK） */
    /* ReqID:TECC4.0 - pending_tables support */
    char original_type_name[NAMEDATALEN]; /* 原始加密列类型名（如 int4_enc），用于pending_tables */
    /* ReqID:TECC4.0 - collation locale for ICU sort key generation */
    char collation_locale[NAMEDATALEN];   /* 排序规则locale字符串（如 "zh_CN.utf8"） */
} TeccColumnRefData;

typedef struct TeccFuncNameData {
    char catalog_name[NAMEDATALEN];      /* 函数所属目录名 */
    char schema_name[NAMEDATALEN];       /* 函数所属模式名 */
    char function_name[NAMEDATALEN];     /* 函数名 */
    int arg_count;                       /* 参数数量 */
    List *args;                          /* 参数列表 */
    bool is_supported;                   /* 是否支持加密列 */
    Oid func_oid;                        /* 函数OID */
} TeccFuncNameData;

typedef struct TeccExprPart {
    int type;  /* NodeTag value */
    union {
        TeccColumnRefData *column_ref;
        TeccFuncNameData *func_name;
        void *other;
    } data;
} TeccExprPart;

typedef struct TeccExprPartsList {
    size_t count;
    size_t capacity;
    TeccExprPart **parts;
} TeccExprPartsList;

/* Expression expansion functions */
bool tecc_expr_expand(const Node *expr_node, TeccExprPartsList **expr_parts);
bool tecc_expand_column_ref(const struct ColumnRef *col_ref, TeccColumnRefData **col_data);
bool tecc_expand_function_name(const struct FuncCall *func_call, TeccFuncNameData **func_data);
bool tecc_expand_condition_expr(const Node *expr, TeccExprPartsList **expr_parts);

/* Parser interface functions */
List *tecc_parser_parse(const char *query_string);
void tecc_parser_free_tree(List *parse_tree);

/* Raw parser interface for libpq export */
List *tecc_raw_parser(const char *str);

#ifdef __cplusplus
}
#endif

#endif /* FE_ADAPTER_H */
/* End - ReqID:TECC4.0 */