/*
 * TECC前端解析器节点类型定义
 * 严格按照详细设计文档第4章要求实现
 * 确保与处理器模块的TeccStatementData完全兼容
 *
 * 设计原则：
 * 1. 内存布局一致：结构体字段顺序、类型、大小与处理器模块期望的完全一致
 * 2. 节点类型对齐：NodeTag枚举值与处理器模块使用的节点类型标识完全一致
 * 3. 位置信息完整：所有语句节点必须包含位置信息，支持处理器模块的SQL改写
 * 4. 字符串管理兼容：字符串编码、内存分配方式与处理器模块兼容
 * 5. 列表结构一致：List结构支持嵌套和链式操作，与处理器模块的列表处理兼容
 */

#ifndef TECC_NODES_H
#define TECC_NODES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* ==================== 基础类型定义 ==================== */

/* PostgreSQL基础类型 - 前端兼容版本 */
#ifndef Oid
typedef unsigned int Oid;
#endif

#ifndef InvalidOid
#define InvalidOid ((Oid) 0)
#endif

#ifndef NAMEDATALEN
#define NAMEDATALEN 64
#endif

/* 节点标签类型 - 前端使用int类型 */
typedef int NodeTag;

/* ==================== 宏定义 ==================== */

/* 列表遍历宏和空列表定义已在tecc_frontend_list.h中定义
 * 这里不再重复定义
 */

/* ALTER TABLE操作类型枚举 */
typedef enum AlterTableType {
    AT_AddColumn,               /* 添加列 */
    AT_DropColumn,              /* 删除列 */
    AT_AlterColumnType,         /* 修改列类型 */
    AT_SetNotNull,              /* 设置NOT NULL */
    AT_DropNotNull,             /* 取消NOT NULL */
    AT_AddConstraint,           /* 添加约束 */
    AT_DropConstraint,          /* 删除约束 */
    AT_ChangeOwner,             /* 修改所有者 */
    AT_SetOptions,              /* 设置选项 */
    AT_ResetOptions,            /* 重置选项 */
    AT_EnableTrig,              /* 启用触发器 */
    AT_DisableTrig,             /* 禁用触发器 */
    AT_EnableRule,              /* 启用规则 */
    AT_DisableRule,             /* 禁用规则 */
    AT_EnableRowSecurity,       /* 启用行安全 */
    AT_DisableRowSecurity,      /* 禁用行安全 */
    AT_ForceRowSecurity,        /* 强制行安全 */
    AT_NoForceRowSecurity,      /* 取消强制行安全 */
    AT_GenericOptions,          /* 通用选项 */
    AT_AttachPartition,         /* 附加分区 */
    AT_DetachPartition,         /* 分离分区 */
    AT_AddIdentity,             /* 添加自增 */
    AT_SetIdentity,             /* 设置自增 */
    AT_DropIdentity,            /* 删除自增 */
    AT_SetStatistics,           /* 设置统计 */
    AT_SetStorage,              /* 设置存储 */
    AT_SetCompression,          /* 设置压缩 */
    AT_DropCompression,         /* 删除压缩 */
    AT_SetLogged,               /* 设置日志 */
    AT_SetUnLogged,             /* 取消日志 */
    AT_AddInherit,              /* 添加继承 */
    AT_DropInherit,             /* 删除继承 */
    AT_AddOf,                   /* 添加类型 */
    AT_DropOf,                  /* 删除类型 */
    AT_ReplicaIdentity,         /* 复制标识 */
    AT_EnableTrigAll,           /* 启用所有触发器 */
    AT_DisableTrigAll,          /* 禁用所有触发器 */
    AT_EnableTrigUser,          /* 启用用户触发器 */
    AT_DisableTrigUser,         /* 禁用用户触发器 */
    AT_SetTableSpace,           /* 设置表空间 */
    AT_SetAccessMethod,         /* 设置访问方法 */
    AT_AlterColumnGenericOptions /* 修改列通用选项 */
} AlterTableType;

/* 删除行为枚举 */
typedef enum DropBehavior {
    DROP_RESTRICT,
    DROP_CASCADE
} DropBehavior;

/* PostgreSQL类型定义 - 如果未定义则定义 */
#ifndef Oid
typedef unsigned int Oid;
#endif

/* 前端环境：类型定义已在tecc_frontend_compat.h中提供 */
/* 只需要包含头文件，不需要重复定义 */

/* 使用统一的NodeTag定义 */
/* 注意：我们使用自己的NodeTag，但需要确保TECC特定的节点类型值不冲突 */

/* 基础节点类型常量 */
/* 这些值必须与PostgreSQL的nodes.h中的定义完全一致 */

/* 基础值节点类型 - 使用FE_T_前缀，值与PostgreSQL后端一致 */
/* 注意：前端代码只使用FE_T_前缀的常量，不直接使用后端T_*常量 */

/* FE_T_前缀定义 - 与fe_adapter.h保持一致 */
/* 注意：这些值必须与fe_adapter.h中的定义完全一致 */
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
#define FE_T_SelectStmt 241
#define FE_T_InsertStmt 238
#define FE_T_UpdateStmt 240
#define FE_T_DeleteStmt 239
#define FE_T_Var 110
#define FE_T_Const 111
#define FE_T_OpExpr 212  /* 暂不确定后端值，保持原值 */
#define FE_T_BoolExpr 123
#define FE_T_NullTest 144
#define FE_T_Param 112
#define FE_T_ParamRef 113  /* 参数引用节点类型 */
#define FE_T_RangeFunction 114  /* FROM子句函数调用节点类型 */
#define FE_T_ResTarget 362
#define FE_T_PrepareStmt 220  /* 暂不确定后端值，保持原值 */
#define FE_T_ExecuteStmt 221  /* 暂不确定后端值，保持原值 */
/* T_PrepareStmt和T_ExecuteStmt已删除，前端代码应使用FE_T_PrepareStmt和FE_T_ExecuteStmt */
#define FE_T_DeclareCursorStmt 222  /* 暂不确定后端值，保持原值 */
#define FE_T_DeallocateStmt 223  /* DEALLOCATE statement node type - ReqID:TECC4.0 */
#define FE_T_CreateTableAsStmt 1013  /* CREATE TABLE AS statement node type - ReqID:TECC4.0 */
#define FE_T_CopyStmt 253
#define FE_T_GroupClause 1012
#define FE_T_CreateStmt 254
#define FE_T_AlterTableStmt 244
#define FE_T_AlterTableCmd 245  /* AlterTableCmd节点类型 */
#define FE_T_ColumnDef 374
#define FE_T_Constraint 373  /* 约束节点类型 */
#define FE_T_TypeName 233  /* 暂不确定后端值，保持原值 */
#define FE_T_CollateClause 367  /* COLLATE子句节点类型 */
#define FE_T_BooleanTest 368  /* BooleanTest节点类型 (IS TRUE/FALSE/UNKNOWN) */
#define FE_T_TimeZoneExpr 369  /* AT TIME ZONE 表达式 */
#define T_Invalid 0  /* 无效节点类型，用于错误节点 */
#define FE_T_Alias 106
#define FE_T_RangeVar 107
#define FE_T_SortBy 366

/* SELECT增强新增节点类型 */
#define FE_T_JoinExpr 103
#define FE_T_RangeSubselect 104
#define FE_T_CaseExpr 105
#define FE_T_CaseWhen 118

/* INSERT ON CONFLICT节点类型 */
#define FE_T_OnConflictClause 108
#define FE_T_InferClause 109

/* ON CONFLICT动作常量 */
#define ONCONFLICT_NOTHING 0
#define ONCONFLICT_UPDATE 1

/* RowExpr 节点类型 */
#define FE_T_RowExpr 370

/* SetToDefault 节点类型 (DEFAULT 关键字作为表达式值) */
#define FE_T_SetToDefault 371

/* ReqID:TECC4.0 - Indirection 表达式节点类型 (字段选择/下标) */
#define FE_T_A_Indirection 372
#define FE_T_A_Indices 373
#define FE_T_A_Star 374

/* EXPLAIN 语句节点类型 */
#define FE_T_ExplainStmt 260

/* 分区相关节点类型 */
#define FE_T_PartitionBoundSpec 1014
#define FE_T_PartitionRangeDatum 1015

/* CTE (Common Table Expression) 节点类型 */
#define FE_T_CommonTableExpr 116

/* SubLink 节点类型 (子查询表达式) */
#define FE_T_SubLink 117

/* WindowDef 节点类型 (窗口函数定义) */
#define FE_T_WindowDef 119

/* SubLink 类型常量 */
#define EXISTS_SUBLINK   0
#define EXPR_SUBLINK     1
#define ANY_SUBLINK      2
#define ALL_SUBLINK      3

/* DROP语句行为常量 */
#define DROP_RESTRICT 0
#define DROP_CASCADE 1

/* DROP语句对象类型常量 */
#define OBJECT_KEK        1000
#define OBJECT_DEK        1001
#define OBJECT_TABLE      1002

/* 表达式类型常量 */
#define AEXPR_OP          1
#define AEXPR_OP_ANY      2
#define AEXPR_OP_ALL      3
#define AEXPR_LIKE        4
#define AEXPR_SIMILAR     5
#define AND_EXPR          1
#define OR_EXPR           2
#define NOT_EXPR          3
#define IS_NULL           1
#define IS_NOT_NULL       2
/* ReqID:TECC4.0 - BooleanTest types (IS TRUE/FALSE/UNKNOWN) */
#define IS_TRUE           3
#define IS_NOT_TRUE       4
#define IS_FALSE          5
#define IS_NOT_FALSE      6
#define IS_UNKNOWN        7
#define IS_NOT_UNKNOWN    8
#define PARAM_EXTERN      1

/* 集合操作类型枚举 */
typedef enum SetOperation {
    SETOP_NONE = 0,
    SETOP_UNION,
    SETOP_INTERSECT,
    SETOP_EXCEPT
} SetOperation;

/* ==================== 基础节点结构 ==================== */

/*
 * 基础节点结构 - 必须与处理器模块的Node结构完全兼容
 *
 * 兼容性要求：
 * 1. NodeTag枚举值必须与处理器模块使用的完全一致
 * 2. 结构体必须是标准布局，支持处理器模块的类型转换
 * 3. 内存对齐方式必须与处理器模块兼容
 */
/* 基础节点结构 */
typedef struct Node {
    NodeTag type;
    int location;           /* 在SQL中的起始位置 */
} Node;

/*
 * 值节点结构 - 必须支持处理器模块的值处理需求
 *
 * 兼容性要求：
 * 1. 联合体成员类型与处理器模块的值类型处理兼容
 * 2. location字段支持处理器模块的值位置跟踪
 * 3. 字符串编码必须与处理器模块的字符串处理兼容
 */
/* 值节点结构 */
typedef struct Value {
    NodeTag type;
    union {
        int32_t ival;
        char *str;
        double dval;
        bool bval;
    } val;
    int location;
} Value;

/*
 * 列表单元格结构 - 必须与处理器模块的列表操作兼容
 * 使用tecc_common/tecc_frontend_list.h中的定义
 */
/* 包含公共列表操作头文件 */
#include "../tecc_common/tecc_frontend_list.h"

/* 注意：ListCell和List结构体已在tecc_frontend_list.h中定义
 * 这里不再重复定义，直接使用公共定义
 */

/* ==================== 位置信息结构 ==================== */

/*
 * 位置信息结构 - 必须支持处理器模块的SQL改写需求
 *
 * 兼容性要求：
 * 1. 位置信息必须足够详细，支持精确的SQL元素定位
 * 2. 位置映射必须支持处理器模块的增量改写
 * 3. 性能必须满足处理器模块的大规模SQL处理需求
 */
typedef struct LocationInfo {
    int start_location;     /* 起始位置（字节偏移） - 必须支持处理器模块的字符串操作 */
    int end_location;       /* 结束位置（字节偏移） - 必须支持处理器模块的字符串操作 */
    int start_line;         /* 起始行号 - 支持处理器模块的错误报告 */
    int start_column;       /* 起始列号 - 支持处理器模块的错误报告 */
    int end_line;           /* 结束行号 - 支持处理器模块的错误报告 */
    int end_column;         /* 结束列号 - 支持处理器模块的错误报告 */
} LocationInfo;

/*
 * 位置映射结构 - 必须支持处理器模块的SQL改写位置调整
 *
 * 兼容性要求：
 * 1. 原始位置到新位置的映射必须准确
 * 2. 链式存储必须支持处理器模块的多处改写
 * 3. 性能必须满足处理器模块的实时改写需求
 */
typedef struct LocationMapping {
    int original_start;     /* 原始起始位置 - 必须与LocationInfo对应 */
    int original_end;       /* 原始结束位置 - 必须与LocationInfo对应 */
    int new_start;          /* 新起始位置 - 支持处理器模块的改写调整 */
    int new_end;            /* 新结束位置 - 支持处理器模块的改写调整 */
    struct LocationMapping *next; /* 下一个映射 - 必须支持处理器模块的链式操作 */
} LocationMapping;

/* ==================== 错误信息结构 ==================== */

/*
 * 解析错误信息结构 - 必须与处理器模块的错误处理兼容
 *
 * 兼容性要求：
 * 1. 错误码定义与处理器模块的错误码枚举一致
 * 2. 错误消息格式与处理器模块的错误处理兼容
 * 3. 位置信息支持处理器模块的错误定位需求
 */
typedef struct ParserError {
    NodeTag type;
    int error_code;         /* 错误码 - 必须与处理器模块的错误码一致 */
    char *error_message;    /* 错误消息 - 格式必须与处理器模块兼容 */
    char *sql_fragment;     /* 相关SQL片段 - 支持处理器模块的错误分析 */
    LocationInfo location;  /* 错误位置 - 必须支持处理器模块的错误定位 */
    int severity;           /* 错误严重级别 - 必须与处理器模块的级别定义一致 */
    char *hint;             /* 修复建议 - 支持处理器模块的错误恢复 */
    char *detail;           /* 详细信息 - 支持处理器模块的错误分析 */
    struct ParserError *next; /* 下一个错误 - 必须支持处理器模块的多个错误处理 */
} ParserError;

/*
 * 错误码枚举 - 必须与处理器模块的错误码定义一致
 */
typedef enum ParserErrorCode {
    PARSER_SUCCESS = 0,
    PARSER_SYNTAX_ERROR = 1001,
    PARSER_SEMANTIC_ERROR = 1002,
    PARSER_COLLATION_CHANGE_ERROR = 1003,
    PARSER_TYPE_MISMATCH_ERROR = 1004,
    PARSER_KEYWORD_MISSING_ERROR = 1005,
    PARSER_MEMORY_ERROR = 2001,
    PARSER_IO_ERROR = 2002
} ParserErrorCode;

/* ==================== 语句节点结构 ==================== */

/*
 * SELECT语句节点 - 必须支持处理器模块的SELECT处理流程
 *
 * 兼容性要求：
 * 1. targetList字段必须支持处理器模块的加密列识别
 * 2. whereClause字段必须支持处理器模块的WHERE子句分析
 * 3. 位置信息必须支持处理器模块的SELECT语句改写
 */
#ifndef SELECTSTMT_DEFINED
#define SELECTSTMT_DEFINED
typedef struct SelectStmt {
    NodeTag type;
    List *distinctClause;   /* DISTINCT子句 - 必须支持处理器模块的DISTINCT处理 */
    struct IntoClause *intoClause; /* INTO子句 - 必须支持处理器模块的结果处理 */
    List *targetList;       /* 目标列表 - 必须支持处理器模块的加密列识别 */
    List *valuesLists;      /* VALUES子句列表 - 必须支持处理器模块的VALUES处理 */
    bool defaultValues;     /* DEFAULT VALUES标志（INSERT DEFAULT VALUES时使用） */
    List *fromClause;       /* FROM子句 - 必须支持处理器模块的表引用分析 */
    Node *whereClause;      /* WHERE子句 - 必须支持处理器模块的条件表达式分析 */
    List *groupClause;      /* GROUP BY子句 - 必须支持处理器模块的分组处理 */
    bool groupDistinct;     /* GROUP BY DISTINCT标志 - 必须支持处理器模块的标志检查 */
    Node *havingClause;     /* HAVING子句 - 必须支持处理器模块的聚合条件分析 */
    List *windowClause;     /* WINDOW子句 - 必须支持处理器模块的窗口函数处理 */
    List *sortClause;       /* ORDER BY子句 - 必须支持处理器模块的排序处理 */
    Node *limitOffset;      /* OFFSET子句 - 必须支持处理器模块的分页处理 */
    Node *limitCount;       /* LIMIT子句 - 必须支持处理器模块的分页处理 */
    List *lockingClause;    /* 锁子句 - 必须支持处理器模块的锁处理 */
    List *cteList;          /* WITH子句 - 必须支持处理器模块的CTE处理 */
    int stmt_location;      /* 语句开始位置 - 必须：支持处理器模块的SQL改写 */
    int stmt_len;           /* 语句长度 - 必须：支持处理器模块的SQL改写 */
} SelectStmt;
#endif /* SELECTSTMT_DEFINED */

/*
 * EXPLAIN 语句节点
 * 前端只识别语法结构，options 不解析留给后端
 */
typedef struct ExplainStmt
{
    NodeTag     type;       /* FE_T_ExplainStmt */
    Node       *query;      /* 内部 DML 语句 (SelectStmt/InsertStmt/UpdateStmt/DeleteStmt) */
    List       *options;    /* 选项列表（前端不解析，存 NIL） */
} ExplainStmt;

/*
 * CTE (Common Table Expression) 节点
 * 用于 WITH 子句中的单个 CTE 定义
 */
typedef struct CommonTableExpr
{
    NodeTag     type;           /* FE_T_CommonTableExpr */
    char       *ctename;        /* CTE 名称 */
    List       *aliascolnames;  /* 列别名列表 (Value 字符串节点), 或 NIL */
    Node       *ctequery;       /* CTE 子查询 (SelectStmt) */
    bool        recursive;      /* ReqID:TECC4.0 - WITH RECURSIVE 标志 */
    int         location;       /* 位置信息 */
} CommonTableExpr;

/*
 * SubLink 节点 (子查询表达式)
 * 用于 EXISTS / IN / ANY / ALL subquery 和标量子查询
 */
typedef struct SubLink
{
    NodeTag     type;           /* FE_T_SubLink */
    int         subLinkType;    /* EXISTS_SUBLINK / EXPR_SUBLINK / ANY_SUBLINK / ALL_SUBLINK */
    Node       *testexpr;       /* 外部测试表达式 (EXPR/ANY/ALL 时使用), 或 NULL */
    List       *operName;       /* 操作符名列表 (ANY/ALL 时使用), 或 NIL */
    Node       *subselect;      /* 子查询 (SelectStmt) */
    int         location;       /* 位置信息 */
} SubLink;

/*
 * WindowDef 节点 (窗口函数定义)
 * 用于 OVER 子句和 WINDOW 子句中的窗口定义
 */
typedef struct WindowDef
{
    NodeTag     type;           /* FE_T_WindowDef */
    char       *name;           /* 窗口名称 (WINDOW 子句用), 或 NULL */
    char       *refname;        /* 引用的窗口名 (OVER winname 用), 或 NULL */
    List       *partitionClause; /* PARTITION BY 表达式列表 */
    List       *orderClause;    /* ORDER BY 排序子句列表 */
    int         location;       /* 位置信息 */
} WindowDef;

/*
 * INSERT语句节点 - 必须支持处理器模块的INSERT处理流程
 *
 * 兼容性要求：
 * 1. cols字段必须支持处理器模块的列值匹配
 * 2. selectStmt字段必须支持处理器模块的INSERT...SELECT处理
 * 3. 位置信息必须支持处理器模块的INSERT语句改写
 */
typedef struct InsertStmt {
    NodeTag type;
    struct RangeVar *relation;     /* 目标关系 - 必须支持处理器模块的表引用分析 */
    List *cols;             /* 列列表 - 必须支持处理器模块的列值匹配 */
    Node *selectStmt;       /* SELECT子查询 - 必须支持处理器模块的INSERT...SELECT处理 */
    List *onConflictClause; /* ON CONFLICT子句 - 必须支持处理器模块的冲突处理 */
    List *returningList;    /* RETURNING子句 - 必须支持处理器模块的结果返回处理 */
    List *withClause;       /* WITH子句 - 必须支持处理器模块的CTE处理 */
    int override;           /* OVERRIDE选项 - 必须支持处理器模块的选项处理 */
    int stmt_location;      /* 语句开始位置 - 必须：支持处理器模块的SQL改写 */
    int stmt_len;           /* 语句长度 - 必须：支持处理器模块的SQL改写 */
} InsertStmt;

/*
 * UPDATE语句节点 - 必须支持处理器模块的UPDATE处理流程
 *
 * 兼容性要求：
 * 1. targetList字段必须支持处理器模块的SET子句分析
 * 2. whereClause字段必须支持处理器模块的WHERE子句分析
 * 3. 位置信息必须支持处理器模块的UPDATE语句改写
 */
typedef struct UpdateStmt {
    NodeTag type;
    struct RangeVar *relation;     /* 目标关系 - 必须支持处理器模块的表引用分析 */
    List *targetList;       /* SET目标列表 - 必须支持处理器模块的赋值表达式分析 */
    Node *whereClause;      /* WHERE子句 - 必须支持处理器模块的条件表达式分析 */
    List *fromClause;       /* FROM子句 - 必须支持处理器模块的多表UPDATE处理 */
    List *returningList;    /* RETURNING子句 - 必须支持处理器模块的结果返回处理 */
    List *withClause;       /* WITH子句 - 必须支持处理器模块的CTE处理 */
    int stmt_location;      /* 语句开始位置 - 必须：支持处理器模块的SQL改写 */
    int stmt_len;           /* 语句长度 - 必须：支持处理器模块的SQL改写 */
} UpdateStmt;

/*
 * DELETE语句节点 - 必须支持处理器模块的DELETE处理流程
 *
 * 兼容性要求：
 * 1. whereClause字段必须支持处理器模块的WHERE子句分析
 * 2. usingClause字段必须支持处理器模块的多表DELETE处理
 * 3. 位置信息必须支持处理器模块的DELETE语句改写
 */
typedef struct DeleteStmt {
    NodeTag type;
    struct RangeVar *relation;     /* 目标关系 - 必须支持处理器模块的表引用分析 */
    Node *whereClause;      /* WHERE子句 - 必须支持处理器模块的条件表达式分析 */
    List *usingClause;      /* USING子句 - 必须支持处理器模块的多表DELETE处理 */
    List *returningList;    /* RETURNING子句 - 必须支持处理器模块的结果返回处理 */
    List *withClause;       /* WITH子句 - 必须支持处理器模块的CTE处理 */
    int stmt_location;      /* 语句开始位置 - 必须：支持处理器模块的SQL改写 */
    int stmt_len;           /* 语句长度 - 必须：支持处理器模块的SQL改写 */
} DeleteStmt;

/* ==================== DDL语句节点结构 ==================== */

/*
 * CREATE TABLE语句节点 - 必须支持处理器模块的CREATE TABLE处理流程
 *
 * 兼容性要求：
 * 1. relation字段必须支持处理器模块的表引用分析
 * 2. tableElts字段必须支持处理器模块的列定义和约束分析
 * 3. 位置信息必须支持处理器模块的CREATE TABLE语句改写
 */
typedef struct CreateStmt {
    NodeTag type;
    struct RangeVar *relation;     /* 目标关系 - 必须支持处理器模块的表引用分析 */
    List *tableElts;        /* 表元素列表（列定义、约束等） - 必须支持处理器模块的元素分析 */
    List *options;          /* WITH选项列表 - 必须支持处理器模块的选项处理 */
    List *inhRelations;     /* 继承表列表 - 必须支持处理器模块的继承处理 */
    Node *partbound;        /* 分区边界 - 必须支持处理器模块的分区处理 */
    List *partspec;         /* 分区规范 - 必须支持处理器模块的分区处理 */
    struct TypeName *ofTypename; /* OF子句（类型化表） - 必须支持处理器模块的类型处理 */
    int stmt_location;      /* 语句开始位置 - 必须：支持处理器模块的SQL改写 */
    int stmt_len;           /* 语句长度 - 必须：支持处理器模块的SQL改写 */
} CreateStmt;

/*
 * ALTER TABLE语句节点 - 必须支持处理器模块的ALTER TABLE处理流程
 *
 * 兼容性要求：
 * 1. relation字段必须支持处理器模块的表引用分析
 * 2. cmds字段必须支持处理器模块的ALTER操作分析
 * 3. 位置信息必须支持处理器模块的ALTER TABLE语句改写
 * 4. 特别需要支持排序规则变更约束检查
 */
typedef struct AlterTableStmt {
    NodeTag type;
    struct RangeVar *relation;     /* 目标关系 - 必须支持处理器模块的表引用分析 */
    List *cmds;             /* ALTER操作命令列表 - 必须支持处理器模块的操作分析 */
    int relkind;            /* 关系类型 - 必须支持处理器模块的对象类型识别 */
    bool missing_ok;        /* IF EXISTS选项 - 必须支持处理器模块的条件处理 */
    int stmt_location;      /* 语句开始位置 - 必须：支持处理器模块的SQL改写 */
    int stmt_len;           /* 语句长度 - 必须：支持处理器模块的SQL改写 */
} AlterTableStmt;

/*
 * ALTER TABLE操作命令节点 - 必须支持处理器模块的ALTER操作分析
 *
 * 兼容性要求：
 * 1. subtype字段必须支持处理器模块的操作类型识别
 * 2. name字段必须支持处理器模块的列名识别
 * 3. def字段必须支持处理器模块的操作定义分析
 * 4. 特别需要支持排序规则变更识别
 */
typedef struct AlterTableCmd {
    NodeTag type;
    AlterTableType subtype; /* 操作类型 - 必须支持处理器模块的操作类型识别 */
    char *name;             /* 列名或约束名 - 必须支持处理器模块的列名识别 */
    Node *def;              /* 操作定义 - 必须支持处理器模块的操作定义分析 */
    DropBehavior behavior;  /* 删除行为 */
    bool missing_ok;        /* 是否允许缺失 */
    int location;           /* 位置信息 - 必须：支持处理器模块的SQL改写 */
} AlterTableCmd;

/*
 * 列定义节点 - 必须支持处理器模块的列定义分析
 *
 * 兼容性要求：
 * 1. colname字段必须支持处理器模块的列名识别
 * 2. typeName字段必须支持处理器模块的类型分析
 * 3. 约束字段必须支持处理器模块的约束检查
 */
typedef struct ColumnDef {
    NodeTag type;
    char *colname;          /* 列名 - 必须支持处理器模块的列名识别 */
    struct TypeName *typeName; /* 类型名 - 必须支持处理器模块的类型分析 */
    List *constraints;      /* 约束列表 - 必须支持处理器模块的约束检查 */
    Node *collClause;       /* COLLATE子句 - 用于ALTER COLUMN TYPE/SET COLLATE的语义检查 */
    int location;           /* 位置信息 - 必须：支持处理器模块的SQL改写 */
} ColumnDef;

/*
 * 约束节点 - 必须支持处理器模块的约束分析
 *
 * 兼容性要求：
 * 1. contype字段支持处理器模块的约束类型识别
 * 2. raw_expr字段支持DEFAULT/CHECK等表达式的访问
 * 3. keys/fk_attrs/pk_attrs支持唯一/主键/外键的列名访问
 */
typedef struct Constraint {
    NodeTag type;
    int contype;            /* 约束类型 (CONSTR_DEFAULT等) */
    char *conname;          /* 约束名称, 或NULL */
    List *keys;             /* UNIQUE/PRIMARY KEY: 列名列表 */
    Node *raw_expr;         /* CHECK/DEFAULT: 原始表达式 */
    List *fk_attrs;         /* FOREIGN KEY: 外键列 */
    List *pk_attrs;         /* FOREIGN KEY: 主键列 */
    struct RangeVar *pktable; /* FOREIGN KEY: 主键表 */
    int location;           /* 位置信息 - 字节偏移 */
    int length;             /* 长度 */
} Constraint;

/* ==================== TECC专用语句节点结构 ==================== */

/* CREATE KEK语句节点 */
typedef struct CreateKekStmt {
    NodeTag type;
    List *names;            /* 全限定名列表，如 {"sch2","kek_test"} */
    List *options;          /* WITH选项列表 */
    bool if_not_exists;     /* IF NOT EXISTS选项 */
    int stmt_location;      /* 语句开始位置 - 必须：支持处理器模块的SQL改写 */
    int stmt_len;           /* 语句长度 - 必须：支持处理器模块的SQL改写 */
} CreateKekStmt;

/* CREATE DEK语句节点 */
typedef struct CreateDekStmt {
    NodeTag type;
    List *names;            /* 全限定名列表，如 {"sch1","dek_a"} */
    List *options;          /* WITH选项列表 */
    bool if_not_exists;     /* IF NOT EXISTS选项 */
    int stmt_location;      /* 语句开始位置 - 必须：支持处理器模块的SQL改写 */
    int stmt_len;           /* 语句长度 - 必须：支持处理器模块的SQL改写 */
} CreateDekStmt;

/* DROP语句节点 */
typedef struct DropStmt {
    NodeTag type;
    List *objects;          /* 要删除的对象列表 */
    List *arguments;        /* 参数列表 */
    int removeType;         /* 对象类型 */
    int behavior;           /* 删除行为（CASCADE/RESTRICT） */
    bool missing_ok;        /* IF EXISTS选项 */
    bool concurrent;        /* 并发删除标志 */
    int stmt_location;      /* 语句开始位置 - 必须：支持处理器模块的SQL改写 */
    int stmt_len;           /* 语句长度 - 必须：支持处理器模块的SQL改写 */
} DropStmt;

/*
 * COPY语句节点 - 必须支持处理器模块的COPY处理流程
 *
 * 兼容性要求：
 * 1. relation字段必须支持处理器模块的表引用分析
 * 2. attlist字段必须支持处理器模块的列选择分析
 * 3. is_from字段必须支持处理器模块的COPY方向判断
 * 4. 位置信息必须支持处理器模块的COPY语句改写
 */
typedef struct CopyStmt {
    NodeTag type;
    struct RangeVar *relation;     /* 目标关系 - 必须支持处理器模块的表引用分析 */
    Node *query;            /* SELECT查询（用于COPY (SELECT ...) TO STDOUT）- TECC扩展 */
    List *attlist;          /* 属性列表（列名） - 必须支持处理器模块的列选择分析 */
    bool is_from;           /* true: COPY FROM, false: COPY TO - 必须支持处理器模块的方向判断 */
    char *filename;         /* 文件名（如果使用文件），NULL表示STDIN/STDOUT */
    List *options;          /* WITH选项列表 */
    int stmt_location;      /* 语句开始位置 - 必须：支持处理器模块的SQL改写 */
    int stmt_len;           /* 语句长度 - 必须：支持处理器模块的SQL改写 */
} CopyStmt;

/* ==================== 分区相关节点结构 ==================== */

/*
 * 分区边界规范节点 - 对应 PG 的 PartitionBoundSpec
 * 用于 PARTITION OF ... FOR VALUES
 */
typedef struct PartitionBoundSpec {
    NodeTag type;
    char    strategy;       /* 'r' RANGE, 'l' LIST, 'h' HASH */
    bool    is_default;     /* FOR VALUES DEFAULT */
    int     location;
    List   *listdatums;     /* LIST: List<PartitionRangeDatum> */
    List   *lowerdatums;    /* RANGE: List<PartitionRangeDatum> */
    List   *upperdatums;    /* RANGE: List<PartitionRangeDatum> */
} PartitionBoundSpec;

/*
 * 分区范围边界值节点（单个 FROM/TO 端点）
 * value 为 Const 节点，NULL 表示 MINVALUE/MAXVALUE
 */
typedef struct PartitionRangeDatum {
    NodeTag type;
    int     location;
    Node   *value;          /* Const 节点，或 NULL（MINVALUE/MAXVALUE 时） */
} PartitionRangeDatum;

/* 集合操作语句节点（UNION/INTERSECT/EXCEPT） */
typedef struct SetOperationStmt {
    NodeTag type;
    SetOperation op;            /* 集合操作类型 */
    bool all;                   /* 是否ALL选项 */
    Node *larg;                 /* 左操作数 */
    Node *rarg;                 /* 右操作数 */
    List *colTypes;             /* 输出列类型OID列表 */
    List *colTypmods;           /* 输出列类型修饰符列表 */
    List *colCollations;        /* 输出列排序规则OID列表 */
    List *groupClauses;         /* 分组子句列表 */
    int stmt_location;          /* 语句开始位置 - 必须：支持处理器模块的SQL改写 */
    int stmt_len;               /* 语句长度 - 必须：支持处理器模块的SQL改写 */
} SetOperationStmt;

/* ==================== 表达式节点结构 ==================== */

/* 列引用节点 */
typedef struct ColumnRef {
    NodeTag type;
    List *fields;           /* 字段名列表（可能包含模式名、表名） */
    int location;           /* 位置信息 - 必须支持处理器模块的SQL改写 */
} ColumnRef;

/* 星号节点（SELECT *） */
typedef struct A_Star {
    NodeTag type;
} A_Star;

/* 函数调用节点 */
typedef struct FuncCall {
    NodeTag type;
    List *funcname;         /* 函数名列表 */
    List *args;             /* 参数列表 */
    List *agg_order;        /* 聚合排序 */
    Node *agg_filter;       /* 聚合过滤 */
    bool agg_star;          /* 聚合星号 */
    bool agg_distinct;      /* 聚合去重 */
    bool func_variadic;     /* 可变参数 */
	bool agg_within_group;  /* WITHIN GROUP 有序集聚合 */
    struct WindowDef *over; /* OVER子句 */
    int location;           /* 位置信息 - 必须支持处理器模块的SQL改写 */
} FuncCall;

/* 操作符表达式节点 */
typedef struct A_Expr {
    NodeTag type;
    int kind;               /* 表达式类型 */
    List *name;             /* 操作符名 */
    Node *lexpr;            /* 左表达式 */
    Node *rexpr;            /* 右表达式 */
    Node *escape;           /* ESCAPE 子句 (LIKE/ILIKE/SIMILAR) */
    int location;           /* 位置信息 - 必须支持处理器模块的SQL改写 */
} A_Expr;

/* ReqID:TECC4.0 - 行构造器节点 (ROW(1,2) 或隐式 (1,2)) */
typedef struct RowExpr {
    NodeTag type;
    List *args;             /* 表达式列表 */
    int row_format;         /* 行格式 */
    int location;           /* 位置信息 */
} RowExpr;

/* 类型转换节点 */
typedef struct TypeCast {
    NodeTag type;
    Node *arg;              /* 要转换的表达式 */
    struct TypeName *typeName; /* 目标类型 */
    int location;           /* 位置信息 - 必须支持处理器模块的SQL改写 */
} TypeCast;

/* 布尔表达式节点 */
typedef struct BoolExpr {
    NodeTag type;
    int boolop;             /* 布尔操作类型 */
    List *args;             /* 参数列表 */
    int location;           /* 位置信息 - 必须支持处理器模块的SQL改写 */
} BoolExpr;

/* 空值测试节点 */
typedef struct NullTest {
    NodeTag type;
    Node *arg;              /* 要测试的表达式 */
    int nulltesttype;       /* 测试类型 */
    int location;           /* 位置信息 - 必须支持处理器模块的SQL改写 */
} NullTest;

/* ReqID:TECC4.0 - Boolean test node (IS TRUE/FALSE/UNKNOWN) */
typedef struct BooleanTest {
    NodeTag type;
    Node *arg;              /* 要测试的布尔表达式 */
    int booltesttype;       /* 测试类型 (IS_TRUE, IS_NOT_TRUE, etc.) */
    int location;           /* 位置信息 */
} BooleanTest;

/* ReqID:TECC4.0 - AT TIME ZONE expression node */
typedef struct TimeZoneExpr {
    NodeTag type;
    Node *arg;              /* 日期时间表达式 */
    Node *timezone;         /* 时区表达式 */
    int location;           /* 位置信息 */
} TimeZoneExpr;

/* ReqID:TECC4.0 - SetToDefault 节点 (DEFAULT 作为表达式值) */
typedef struct SetToDefault {
    NodeTag type;
    int location;           /* 位置信息 */
} SetToDefault;

/* ReqID:TECC4.0 - A_Indices: 数组下标或切片 [lidx:uidx] */
typedef struct A_Indices {
    NodeTag type;
    bool is_slice;          /* true=切片[lidx:uidx], false=单下标[uidx] */
    Node *lidx;             /* 切片下界 (单下标时为NULL) */
    Node *uidx;             /* 切片上界 (或单下标值) */
} A_Indices;

/* ReqID:TECC4.0 - A_Indirection: 间接引用表达式 (字段选择/下标) */
typedef struct A_Indirection {
    NodeTag type;
    Node *arg;              /* 被引用的基表达式 */
    List *indirection;      /* A_Indices / String / A_Star 列表 */
} A_Indirection;

/* 参数引用节点 */
typedef struct Param {
    NodeTag type;
    int paramkind;          /* 参数类型 */
    int paramid;            /* 参数ID */
    int paramtype;          /* 参数类型OID */
    int location;           /* 位置信息 - 必须支持处理器模块的SQL改写 */
} Param;

/* 参数引用节点（$n参数） */
typedef struct ParamRef {
    NodeTag type;
    int number;                 /* 参数编号 */
    int location;               /* 位置信息 - 必须支持处理器模块的SQL改写 */
} ParamRef;

/* JOIN表达式节点 */
typedef struct JoinExpr {
    NodeTag type;
    int jointype;               /* JOIN类型 */
    Node *larg;                 /* 左表 */
    Node *rarg;                 /* 右表 */
    Node *quals;                /* ON条件表达式（NATURAL JOIN时为NULL） */
    List *usingClause;          /* USING子句的列名列表 */
    bool isNatural;             /* 是否为NATURAL JOIN */
    int location;               /* 位置信息 */
} JoinExpr;

/* FROM子句子查询节点 */
typedef struct RangeSubselect {
    NodeTag type;
    Node *subquery;             /* 子查询语句 */
    struct Alias *alias;        /* 别名 */
    bool lateral;               /* LATERAL标志 */
    int location;               /* 位置信息 */
} RangeSubselect;

/* FROM子句函数调用节点 - 用于FROM generate_series(...) i */
typedef struct RangeFunction {
    NodeTag type;
    Node *funccall;             /* 函数调用(FuncCall) */
    struct Alias *alias;        /* 表别名 */
    bool lateral;               /* LATERAL标志 */
    int location;               /* 位置信息 */
} RangeFunction;

/* CASE WHEN表达式节点 */
typedef struct CaseExpr {
    NodeTag type;
    Node *arg;                  /* CASE后的参数表达式（可选） */
    Node *defresult;            /* ELSE默认结果（可选） */
    List *args;                 /* WHEN子句列表（CaseWhen节点列表） */
    int location;               /* 位置信息 */
} CaseExpr;

/* CASE WHEN子句节点 */
typedef struct CaseWhen {
    NodeTag type;
    Node *expr;                 /* WHEN条件 */
    Node *result;               /* THEN结果 */
    int location;               /* 位置信息 */
} CaseWhen;

/* INSERT ON CONFLICT子句 */

/* InferClause - 冲突目标（索引推断） */
typedef struct InferClause {
    NodeTag type;
    List *indexElems;      /* 索引表达式（expr列表） */
    Node *whereClause;     /* 部分索引谓词，或NULL */
    char *conname;         /* 约束名称，或NULL */
    int location;
} InferClause;

/* OnConflictClause - 完整冲突处理子句 */
typedef struct OnConflictClause {
    NodeTag type;
    int action;            /* ONCONFLICT_NOTHING或ONCONFLICT_UPDATE */
    Node *infer;           /* InferClause指针，或NULL */
    List *targetList;      /* DO UPDATE SET的目标列表（ResTarget列表） */
    Node *whereClause;     /* DO UPDATE的WHERE条件，或NULL */
    int location;
} OnConflictClause;

/* 表达式基类节点 */
typedef struct Expr {
    NodeTag type;
} Expr;

/* 常量节点 */
typedef struct Const {
    NodeTag type;
    int consttype;          /* 常量类型OID */
    int consttypmod;        /* 类型修饰符 */
    int constcollid;        /* 排序规则OID */
    int constlen;           /* 常量长度 */
    bool constbyval;        /* 是否按值传递 */
    bool constisnull;       /* 是否为空 */
    int location;           /* 位置信息 - 字节偏移量，由scanner在yylex时设置 */
    int data_size;          /* 常量值在原始SQL中的文本长度 */
    union {
        int64_t ival;       /* 整数值 */
        double dval;        /* 浮点值 */
        char *sval;         /* 字符串值 */
    } constvalue;
} Const;

/* 数组表达式节点 */
typedef struct ArrayExpr {
    Expr xpr;                   /* 表达式基类 */
    Oid array_typeid;           /* 数组类型OID */
    Oid array_collid;           /* 数组排序规则OID */
    Oid element_typeid;         /* 元素类型OID */
    List *elements;             /* 元素列表 */
    bool multidims;             /* 是否多维数组 */
    int location;               /* 位置信息 - 必须支持处理器模块的SQL改写 */
} ArrayExpr;

/* 变量节点 */
typedef struct Var {
    NodeTag type;
    int varno;              /* 变量编号 */
    int varattno;           /* 属性编号 */
    int vartype;            /* 变量类型OID */
    int vartypmod;          /* 类型修饰符 */
    int varcollid;          /* 排序规则OID */
    int varlevelsup;        /* 变量层级 */
    int varnoold;           /* 原始变量编号 */
    int varoattno;          /* 原始属性编号 */
    int location;           /* 位置信息 - 必须支持处理器模块的SQL改写 */
} Var;

/* ==================== 其他节点结构 ==================== */

/* 结果目标节点 */
typedef struct ResTarget {
    NodeTag type;
    char *name;             /* 目标名称 */
    List *indirection;      /* 间接引用列表 */
    Node *val;              /* 值表达式 */
    int location;           /* 位置信息 - 必须支持处理器模块的SQL改写 */
} ResTarget;

/* 类型名节点 */
/* COLLATE子句节点 - 必须支持处理器模块的排序规则分析 */
typedef struct CollateClause {
    NodeTag type;
    Node *arg;              /* 输入表达式 */
    List *collname;         /* 排序规则名称列表 */
    int location;           /* 位置信息 - 必须支持处理器模块的SQL改写 */
} CollateClause;

typedef struct TypeName {
    NodeTag type;
    List *names;            /* 类型名列表 */
    int typemod;            /* 类型修饰符 */
    List *arrayBounds;      /* 数组边界 */
    bool setof;             /* SETOF标志 */
    bool pct_type;          /* 百分比类型标志 */
    int location;           /* 位置信息 - 必须支持处理器模块的SQL改写 */
    List *encryptedOpts;    /* ENCRYPTED WITH 选项列表，非加密类型为NIL */
} TypeName;

/* 范围变量节点 */
typedef struct RangeVar {
    NodeTag type;
    char *catalogname;      /* 目录名 */
    char *schemaname;       /* 模式名 */
    char *relname;          /* 关系名 */
    bool inh;               /* 是否继承 */
    char relpersistence;    /* 关系持久性 */
    struct Alias *alias;    /* 别名 */
    int location;           /* 位置信息 - 必须支持处理器模块的SQL改写 */
} RangeVar;

/* 别名节点 */
typedef struct Alias {
    NodeTag type;
    char *aliasname;        /* 别名名称 */
    List *colnames;         /* 列名列表 */
} Alias;

/* 定义元素节点 */
typedef struct DefElem {
    NodeTag type;
    char *defnamespace;     /* 定义命名空间 */
    char *defname;          /* 定义名称 */
    Node *arg;              /* 参数值 */
    int defaction;          /* 定义动作 */
    int location;           /* 位置信息 - 必须支持处理器模块的SQL改写 */
} DefElem;

/* 分组子句节点 */
typedef struct GroupClause {
    NodeTag type;
    List *list;             /* 分组表达式列表 */
    bool distinct;          /* 是否去重 */
} GroupClause;

/* 排序子句节点 */
typedef struct SortBy {
    NodeTag type;
    Node *node;             /* 排序表达式 */
    int sortby_dir;         /* 排序方向 */
    int sortby_nulls;       /* 空值处理 */
    List *useOp;            /* 使用的操作符 */
    int location;           /* 位置信息 - 必须支持处理器模块的SQL改写 */
} SortBy;

/* 排序方向常量 */
#define SORTBY_DEFAULT   0
#define SORTBY_ASC       1
#define SORTBY_DESC      2

/* NULLS FIRST/LAST 常量 */
#define SORTBY_NULLS_DEFAULT  0
#define SORTBY_NULLS_FIRST    1
#define SORTBY_NULLS_LAST     2

/* ReqID:TECC4.0 - INSERT OVERRIDING 常量 */
#define OVERRIDING_NOT_SET         0
#define OVERRIDING_USER_VALUE  1
#define OVERRIDING_SYSTEM_VALUE    2

/* ----------------------
 *      PREPARE Statement
 * ----------------------
 */
typedef struct PrepareStmt {
    NodeTag type;
    int location;           /* 位置信息 - 必须支持处理器模块的SQL改写 */
    int length;             /* 节点长度 */
    char *name;             /* Name of plan, arbitrary */
    List *argtypes;         /* Types of parameters (List of TypeName) */
    Node *query;            /* The query itself (as a raw parsetree) */
} PrepareStmt;

/* ----------------------
 *      EXECUTE Statement
 * ----------------------
 */
typedef struct ExecuteStmt {
    NodeTag type;
    int location;           /* 位置信息 - 必须支持处理器模块的SQL改写 */
    int length;             /* 节点长度 */
    char *name;             /* The name of the plan to execute */
    List *params;           /* Values to assign to parameters (List of A_Const nodes) */
    int nparams;            /* Number of parameters */
    Oid *paramtypes;        /* Parameter type OIDs */
    const char *const *paramValues;     /* Parameter values (const pointer to const char pointers) */
    int *paramLengths;      /* Parameter lengths */
    int *paramFormats;      /* Parameter formats */
} ExecuteStmt;

/* ----------------------
 *      DEALLOCATE Statement
 * ----------------------
 */
typedef struct DeallocateStmt {
    NodeTag type;
    int location;           /* 位置信息 - 必须支持处理器模块的SQL改写 */
    int length;             /* 节点长度 */
    char *name;             /* The name of the plan to remove */
    /* NULL means DEALLOCATE ALL */
} DeallocateStmt;

/* ----------------------
 *      CREATE TABLE AS Statement
 *      Supports: CREATE TABLE ... AS SelectStmt
 *               CREATE TABLE ... AS EXECUTE name
 * ----------------------
 */
typedef struct CreateTableAsStmt {
    NodeTag type;
    int location;           /* 位置信息 - 必须支持处理器模块的SQL改写 */
    int length;             /* 节点长度 */
    RangeVar *relation;     /* 目标表名 - 必须支持处理器模块的加密列识别 */
    Node *query;            /* 查询(SelectStmt或ExecuteStmt) - 必须支持处理器模块的子查询遍历 */
    bool if_not_exists;     /* IF NOT EXISTS标志 */
} CreateTableAsStmt;

/* ----------------------
 *      DECLARE CURSOR Statement
 * ----------------------
 */
typedef struct DeclareCursorStmt {
    NodeTag type;
    int location;           /* 位置信息 */
    int length;             /* 节点长度 */
    char *portalname;       /* name of the cursor */
    Node *query;            /* the SELECT query (raw parse tree) */
} DeclareCursorStmt;

/* ==================== 宏定义 ==================== */

/* 内存分配宏 */
#define makeNode(size) ((void *)tecc_palloc(size))
/* palloc和pfree宏已移除，使用tecc_palloc和tecc_pfree函数代替 */

/* 节点类型检查宏 */
#define IsA(nodeptr, _type_) ((nodeptr)->type == FE_T_##_type_)
#define nodeTag(nodeptr) ((nodeptr)->type)

/* ==================== 函数声明 ==================== */

#ifdef __cplusplus
extern "C" {
#endif

/* 内存管理函数和列表操作函数已在tecc_frontend_list.h中声明
 * 这里不再重复声明，直接包含头文件即可使用
 */

/* 值提取函数 */
extern int intVal(const Value *node);
extern char *strVal(const Value *node);
extern double floatVal(const Value *node);
extern bool boolVal(const Value *node);
extern bool list_is_empty(const List *list);

/* 节点创建函数 */
Node *makeIntConst(int64_t val, int location, int data_size);
Node *makeStringConst(const char *str, int location, int data_size);
Node *makeFloatConst(double val, int location, int data_size);
Node *makeNullConst(int location, int data_size);

/* 语句节点创建函数 */
SelectStmt *makeSelectStmt(int stmt_location, int stmt_len);
InsertStmt *makeInsertStmt(int stmt_location, int stmt_len);
UpdateStmt *makeUpdateStmt(int stmt_location, int stmt_len);
DeleteStmt *makeDeleteStmt(int stmt_location, int stmt_len);
CreateStmt *makeCreateStmt(int stmt_location, int stmt_len);
AlterTableStmt *makeAlterTableStmt(int stmt_location, int stmt_len);
AlterTableCmd *makeAlterTableCmd(AlterTableType subtype, Node *def, char *name, DropBehavior behavior, bool missing_ok, int location);
CreateKekStmt *makeCreateKekStmt(int stmt_location, int stmt_len);
CreateDekStmt *makeCreateDekStmt(int stmt_location, int stmt_len);
DropStmt *makeDropStmt(int stmt_location, int stmt_len);

/* COPY statement node creation function */
CopyStmt *makeCopyStmt(struct RangeVar *relation, Node *query, List *attlist, bool is_from,
                      char *filename, List *options,
                      int stmt_location, int stmt_len);

/* Prepared statement node creation functions */
PrepareStmt *makePrepareStmt(char *name, List *argtypes, Node *query, int location);
ExecuteStmt *makeExecuteStmt(char *name, List *params, int nparams, Oid *paramtypes,
                            const char *const *paramValues, int *paramLengths, int *paramFormats, int location);
DeallocateStmt *makeDeallocateStmt(char *name, int location);
CreateTableAsStmt *makeCreateTableAsStmt(RangeVar *relation, Node *query, bool if_not_exists, int location);

/* 列定义节点创建函数 */
ColumnDef *makeColumnDef(int location, int len);

/* 表达式节点创建函数 */
ColumnRef *makeColumnRef(List *fields, int location);
FuncCall *makeFuncCall(List *funcname, List *args, int location);
A_Expr *makeAExpr(int kind, List *name, Node *lexpr, Node *rexpr, int location);
A_Expr *makeAExprWithEscape(int kind, List *name, Node *lexpr, Node *rexpr, Node *escape, int location);
/* ReqID:TECC4.0 - RowExpr constructor */
RowExpr *makeRowExpr(List *args, int location);
BoolExpr *makeBoolExpr(int boolop, List *args, int location);
NullTest *makeNullTest(Node *arg, int nulltesttype, int location);
/* ReqID:TECC4.0 - BooleanTest constructor */
BooleanTest *makeBooleanTest(Node *arg, int booltesttype, int location);
/* ReqID:TECC4.0 - TimeZoneExpr constructor */
TimeZoneExpr *makeTimeZoneExpr(Node *arg, Node *timezone, int location);
/* ReqID:TECC4.0 - SetToDefault constructor */
SetToDefault *makeSetToDefault(int location);
/* ReqID:TECC4.0 - Indirection constructors */
A_Indices *makeA_Indices(bool is_slice, Node *lidx, Node *uidx);
A_Indirection *makeA_Indirection(Node *arg, List *indirection);
A_Star *makeA_Star(void);
Param *makeParam(int paramkind, int paramid, int paramtype, int location);
Const *makeConst(int consttype, bool constisnull, int location, int data_size);
Var *makeVar(int varno, int varattno, int vartype, int location);
SetOperationStmt *makeSetOperationStmt(int op, bool all, Node *larg, Node *rarg, int location, int len);
JoinExpr *makeJoinExpr(int jointype, Node *larg, Node *rarg, Node *quals, int location);
RangeSubselect *makeRangeSubselect(Node *subquery, struct Alias *alias, int location);
RangeFunction *makeRangeFunction(Node *funccall, struct Alias *alias, bool lateral, int location);
CaseExpr *makeCaseExpr(Node *arg, Node *defresult, List *args, int location);
CaseWhen *makeCaseWhen(Node *expr, Node *result, int location);

/* INSERT ON CONFLICT 节点创建函数 */
OnConflictClause *makeOnConflictClause(int action, int location);
InferClause *makeInferClause(List *indexElems, Node *whereClause, const char *conname, int location);

/* 列表操作函数 */
ListCell *list_head(const List *list);
ListCell *lnext(const ListCell *cell);

/* 约束类型常量 */
#define CONSTR_NULL       0
#define CONSTR_NOTNULL    1
#define CONSTR_CHECK      2
#define CONSTR_DEFAULT    3
#define CONSTR_UNIQUE     4
#define CONSTR_PRIMARY    5
#define CONSTR_FOREIGN    6

/* Additional constraint types for compatibility with PostgreSQL backend */
#define CONSTR_IDENTITY      7
#define CONSTR_GENERATED     8
#define CONSTR_EXCLUSION     9
#define CONSTR_ATTR_DEFERRABLE 10
#define CONSTR_ATTR_DEFERRED 11
#define CONSTR_ATTR_IMMEDIATE 12

/* 其他节点创建函数 */
ResTarget *makeResTarget(char *name, Node *val, int location);
RangeVar *makeRangeVar(char *schemaname, char *relname, int location);
RangeVar *makeRangeVarFromQualifiedName(const char *qualified_name, int location, int length);
Alias *makeAlias(char *aliasname, List *colnames);
DefElem *makeDefElem(char *defname, Node *arg, int location);

/* 缺失的函数声明 */
TypeName *makeTypeName(const char *name, int location, int length);
TypeName *makeTypeNameFromList(List *names, int location, int length);
TypeName *makeEncryptedTypeName(const char *name, List *options, int location, int length);
TypeName *makeEncryptedTypeNameWithTypmod(const char *name, int typmod, List *options, int location, int length);
TypeName *makeEncryptedTypeNameWithTypmod2(const char *name, int typmod1, int typmod2, List *options, int location, int length);
TypeName *makeTypeNameWithTypmod(const char *name, int typmod, int location, int length);
TypeName *makeTypeNameWithTypmod2(const char *name, int typmod1, int typmod2, int location, int length);
Node *makeCollateClause(Node *arg, List *collname, int location);
Node *makeConstraint(int constr_type, int location, int length);
Node *makeConstraintWithExpr(int constr_type, Node *expr, int location, int length);
Node *makeForeignKeyConstraint(List *pktable, List *fk_attrs, List *pk_attrs, int location, int length);
Node *setConstraintName(Node *constraint, const char *name, int location, int length);
Node *makeConstraintWithColumns(int constr_type, List *columns, int location, int length);

/* 节点复制和释放函数 */
Node *tecc_copy_node(const Node *node);
void tecc_free_node(Node *node);
void freeNode(Node *node);
void freeSelectStmt(SelectStmt *stmt);
void freeInsertStmt(InsertStmt *stmt);
void freeUpdateStmt(UpdateStmt *stmt);
void freeDeleteStmt(DeleteStmt *stmt);
void freeCreateStmt(CreateStmt *stmt);
void freeCreateKekStmt(CreateKekStmt *stmt);
void freeCreateDekStmt(CreateDekStmt *stmt);
void freeDropStmt(DropStmt *stmt);
void list_free_node_list(List *list);

/* COPY statement node free function */
void freeCopyStmt(CopyStmt *stmt);


/* 分区相关节点创建和释放函数 */
PartitionBoundSpec *makePartitionBoundSpec(char strategy, bool is_default, int location);
PartitionRangeDatum *makePartitionRangeDatum(Node *value, int location);
void freePartitionBoundSpec(PartitionBoundSpec *spec);
void freePartitionRangeDatum(PartitionRangeDatum *datum);
/* Prepared statement node free functions */
void freePrepareStmt(PrepareStmt *stmt);
void freeExecuteStmt(ExecuteStmt *stmt);
void freeDeallocateStmt(DeallocateStmt *stmt);
void freeCreateTableAsStmt(CreateTableAsStmt *stmt);

/* DECLARE CURSOR node functions */
DeclareCursorStmt *makeDeclareCursorStmt(char *portalname, Node *query, int location);
void freeDeclareCursorStmt(DeclareCursorStmt *stmt);

/* CTE (Common Table Expression) node functions */
CommonTableExpr *makeCommonTableExpr(char *ctename, List *aliascolnames, Node *ctequery, int location);
void set_cte_list_recursive(List *cte_list);    /* ReqID:TECC4.0 - WITH RECURSIVE support */

/* SubLink node functions */
SubLink *makeSubLink(int subLinkType, Node *testexpr, List *operName, Node *subselect, int location);

/* WindowDef node functions */
WindowDef *makeWindowDef(char *name, char *refname, List *partitionClause, List *orderClause, int location);

#ifdef __cplusplus
}
#endif

#endif /* TECC_NODES_H */