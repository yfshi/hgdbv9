# TECC解析器与处理器集成使用文档

## 概述

本文档详细说明TECC处理器模块如何调用前端解析器进行SQL语句解析。通过本文档，开发者可以了解处理器与解析器的集成方式、接口调用流程以及实际使用示例。

## 目录

1. [接口定义](#接口定义)
2. [调用流程](#调用流程)
3. [数据结构](#数据结构)
4. [示例代码](#示例代码)
5. [错误处理](#错误处理)
6. [内存管理](#内存管理)
7. [集成验证](#集成验证)
8. [最佳实践](#最佳实践)
9. [故障排除](#故障排除)
10. [总结](#总结)

## 接口定义

### 核心解析接口

处理器通过以下两个核心接口与解析器交互：

```c
/* 解析SQL语句，返回List*格式的解析树 */
List *tecc_parser_parse(const char *query_string);

/* 释放解析树占用的内存 */
void tecc_parser_free_tree(List *parse_tree);
```

### 参数映射接口

TECC解析器支持参数索引到列三元组（schema.table.column）的映射，用于参数绑定期的加密判定。处理器可以通过以下接口访问和管理参数映射：

```c
/* 获取参数映射
 * @param result 解析结果
 * @param param_index 参数索引（从1开始）
 * @param schema_name 模式名输出参数（可选）
 * @param table_name 表名输出参数（可选）
 * @param column_name 列名输出参数（可选）
 * @return 是否找到映射
 */
bool tecc_parser_get_param_mapping(const TeccParseResult *result, int param_index,
                                  const char **schema_name, const char **table_name,
                                  const char **column_name);

/* 添加参数映射
 * @param result 解析结果
 * @param param_index 参数索引（从1开始）
 * @param schema_name 模式名
 * @param table_name 表名
 * @param column_name 列名
 * @return 是否成功添加
 */
bool tecc_parser_add_param_mapping(TeccParseResult *result, int param_index,
                                  const char *schema_name, const char *table_name,
                                  const char *column_name);

/* 清除所有参数映射
 * @param result 解析结果
 */
void tecc_parser_clear_param_mappings(TeccParseResult *result);

/* 获取参数映射数量
 * @param result 解析结果
 * @return 参数映射数量
 */
int tecc_parser_get_param_mapping_count(const TeccParseResult *result);
```

### 头文件包含

处理器需要包含以下头文件：

```c
#include "../frontend_parser/fe_adapter.h"  /* 前端解析器适配器接口 */
```

## 调用流程

### 1. 处理器调用解析器的完整流程

以下是处理器模块中调用解析器的典型流程（基于`tecc_processor.c`的实际代码）：

```c
/* 步骤1: 检查是否已解析，如未解析则调用解析器 */
if (statement_data->parse_tree == NIL && statement_data->query != NULL)
{
    /* 调用前端解析器解析SQL */
    statement_data->parse_tree = tecc_parser_parse(statement_data->query);
    
    if (statement_data->parse_tree == NULL)
    {
        /* 解析失败处理 */
        tecc_processor_debug(TECC_PROCESSOR_DEBUG_LEVEL_ERROR,
                           "SQL parsing failed");
        return processor_handle_error(statement_data, TECC_PROCESSOR_PARSE_ERROR,
                                    "SQL parsing failed");
    }
    
    tecc_processor_debug(TECC_PROCESSOR_DEBUG_LEVEL_DEBUG,
                        "SQL parsing completed successfully");
}

/* 步骤2: 遍历解析树处理每个语句 */
if (statement_data->parse_tree != NULL)
{
    ListCell *lc;
    foreach(lc, statement_data->parse_tree)
    {
        Node *stmt = (Node *) lfirst(lc);
        TeccProcessorResult result = processor_process_statement(stmt, statement_data);
        if (result != TECC_PROCESSOR_SUCCESS)
            return result;
    }
}
```

### 2. 简化的调用示例

```c
/* 初始化处理器数据结构 */
TeccStatementData statement_data = {
    .query = "SELECT * FROM encrypted_table WHERE id = 1",
    .parse_tree = NIL,
    .needs_rewrite = 0
};

/* 调用解析器 */
List *parse_tree = tecc_parser_parse(statement_data.query);

if (parse_tree == NULL) {
    printf("SQL解析失败\n");
    return;
}

printf("解析成功，获得 %d 个语句节点\n", parse_tree->length);

/* 遍历解析树 */
ListCell *cell = parse_tree->head;
while (cell != NULL) {
    Node *stmt = (Node *)cell->data;
    
    /* 根据节点类型处理不同语句 */
    switch (stmt->type) {
        case T_SelectStmt:
            printf("处理SELECT语句\n");
            process_select_statement((SelectStmt *)stmt, &statement_data);
            break;
        case T_InsertStmt:
            printf("处理INSERT语句\n");
            process_insert_statement((InsertStmt *)stmt, &statement_data);
            break;
        case T_UpdateStmt:
            printf("处理UPDATE语句\n");
            process_update_statement((UpdateStmt *)stmt, &statement_data);
            break;
        case T_DeleteStmt:
            printf("处理DELETE语句\n");
            process_delete_statement((DeleteStmt *)stmt, &statement_data);
            break;
        case T_CreateKekStmt:
            printf("处理CREATE KEK语句\n");
            process_create_kek_statement((CreateKekStmt *)stmt, &statement_data);
            break;
        case T_CreateDekStmt:
            printf("处理CREATE DEK语句\n");
            process_create_dek_statement((CreateDekStmt *)stmt, &statement_data);
            break;
        case T_DropStmt:
            printf("处理DROP语句\n");
            process_drop_statement((DropStmt *)stmt, &statement_data);
            break;
        default:
            printf("未知语句类型: %d\n", stmt->type);
    }
    
    cell = cell->next;
}

/* 释放解析树 */
tecc_parser_free_tree(parse_tree);
```

## 数据结构

### 1. 解析树结构

解析器返回的解析树使用PostgreSQL兼容的数据结构：

```c
/* 基础节点结构 */
typedef struct Node {
    NodeTag type;      /* 节点类型标识 */
    int location;      /* 在SQL中的起始位置 */
    int length;        /* 节点长度 */
} Node;

/* 列表单元格 */
typedef struct ListCell {
    void *data;           /* 指向节点的指针 */
    struct ListCell *next; /* 下一个单元格 */
} ListCell;

/* 列表结构 */
typedef struct List {
    NodeTag type;      /* 列表类型标识 */
    int location;      /* 列表位置 */
    int length;        /* 列表长度（单元格数量） */
    ListCell *head;    /* 列表头指针 */
    ListCell *tail;    /* 列表尾指针 */
} List;
```

### 2. 节点类型常量

解析器支持的节点类型常量定义在`tecc_nodes.h`中，使用PostgreSQL兼容的值：

```c
/* 基础节点类型 */
#define T_Invalid          0   /* 无效节点类型 */
#define T_List             231 /* 列表节点 */
#define T_Value            225 /* Value节点 */

/* SQL语句节点类型（使用PostgreSQL兼容值） */
#define T_SelectStmt       241 /* SELECT语句节点 */
#define T_InsertStmt       238 /* INSERT语句节点 */
#define T_UpdateStmt       240 /* UPDATE语句节点 */
#define T_DeleteStmt       239 /* DELETE语句节点 */
#define T_DropStmt         256 /* DROP语句节点 */

/* TECC专用语句节点类型（使用专用值，避免与PostgreSQL冲突） */
#define T_CreateKekStmt    1000 /* CREATE KEK语句节点 - TECC专用 */
#define T_CreateDekStmt    1001 /* CREATE DEK语句节点 - TECC专用 */

/* 表达式节点类型（使用PostgreSQL兼容值） */
#define T_ColumnRef        354 /* 列引用节点 */
#define T_FuncCall         357 /* 函数调用节点 */
#define T_A_Expr           353 /* 操作符表达式节点 */
#define T_BoolExpr         123 /* 布尔表达式节点 */
#define T_NullTest         144 /* 空值测试节点 */
#define T_Const            111 /* 常量节点 */
#define T_DefElem          378 /* 定义元素节点 */

/* 其他节点类型（使用PostgreSQL兼容值） */
#define T_ResTarget        362 /* 结果目标节点 */
#define T_RangeVar         107 /* 范围变量节点 */
#define T_GroupClause      1012 /* 分组子句节点（自定义值） */
#define T_SortBy           366 /* 排序子句节点 */
```

### 3. 参数映射数据结构

解析器支持参数索引到列三元组的映射，用于参数绑定期的加密判定。相关数据结构如下：

```c
/* 参数列映射结构 - 支持参数索引到列三元组的映射 */
typedef struct TeccParamColumnMapping {
    int param_index;                    /* 参数索引（如$1对应1） */
    char *schema_name;                  /* 模式名 */
    char *table_name;                   /* 表名 */
    char *column_name;                  /* 列名 */
    struct TeccParamColumnMapping *next; /* 下一个映射 - 支持链表结构 */
} TeccParamColumnMapping;

/* 参数映射结果结构 - 存储所有参数映射 */
typedef struct TeccParamMappingResult {
    TeccParamColumnMapping *mappings;   /* 参数映射链表 */
    int mapping_count;                  /* 映射数量 */
    bool has_mappings;                  /* 是否有映射 */
} TeccParamMappingResult;

/* 解析结果结构包含参数映射 */
typedef struct TeccParseResult {
    Node *parse_tree;                   /* 解析树 */
    ParserError *errors;                /* 错误信息链表 */
    int error_count;                    /* 错误数量 */
    int warning_count;                  /* 警告数量 */
    size_t parse_time_ms;               /* 解析时间(毫秒) */
    size_t memory_used_kb;              /* 内存使用量(KB) */
    bool success;                       /* 解析是否成功 */
    TeccParamMappingResult param_mappings; /* 参数映射结果 */
} TeccParseResult;
```

### 4. 处理器数据结构

处理器使用`TeccStatementData`结构存储解析和处理结果：

```c
struct TeccStatementData {
    PGconn *conn;                 /* 数据库连接 */
    const char *query;            /* 原始SQL语句 */
    const char *stmtName;         /* 语句名称 */
    size_t nParams;               /* 参数数量 */
    const Oid *paramTypes;        /* 参数类型数组 */
    const char * const * paramValues; /* 参数值数组 */
    const int *paramLengths;      /* 参数长度数组 */
    const int *paramFormats;      /* 参数格式数组 */
    size_t offset;                /* 偏移量 */
    
    /* TECC处理相关字段 */
    List *parse_tree;             /* 解析后的SQL树 */
    List *encrypted_columns;      /* 识别的加密列列表 */
    int needs_rewrite;            /* 是否需要SQL改写 */
    int error_code;               /* 错误代码 */
    char error_msg[256];          /* 错误消息 */
    
    /* SQL改写支持 */
    char *rewritten_query;        /* 改写后的查询字符串 */
    TeccRawValuesList *raw_values_list; /* 原始值列表 */
    int where_needs_rewrite;      /* WHERE子句是否需要改写 */
};
```

## 示例代码

### 1. 完整的处理器调用示例

```c
#include "../frontend_parser/fe_adapter.h"
#include "tecc_processor.h"
#include <stdio.h>
#include <stdlib.h>

/* 处理SELECT语句 */
static void process_select_statement(const SelectStmt *select_stmt,
                                   TeccStatementData *stmt_data) {
    printf("处理SELECT语句:\n");
    
    /* 检查目标列表中的加密列 */
    if (select_stmt->targetList) {
        ListCell *lc;
        foreach(lc, select_stmt->targetList) {
            ResTarget *target = (ResTarget *)lfirst(lc);
            if (target && target->val) {
                /* 检查是否为加密列引用 */
                if (IsA(target->val, ColumnRef)) {
                    ColumnRef *col_ref = (ColumnRef *)target->val;
                    printf("  列引用: ");
                    ListCell *field;
                    foreach(field, col_ref->fields) {
                        char *name = (char *)lfirst(field);
                        printf("%s.", name);
                    }
                    printf("\n");
                }
            }
        }
    }
    
    /* 检查WHERE子句 */
    if (select_stmt->whereClause) {
        printf("  WHERE子句需要检查加密条件\n");
        stmt_data->where_needs_rewrite = 1;
    }
}

/* 主处理函数 */
TeccProcessorResult tecc_processor_run_pre_query(TeccStatementData *stmt_data) {
    if (!stmt_data || !stmt_data->query) {
        return TECC_PROCESSOR_INVALID_ARGUMENT;
    }
    
    /* 步骤1: 调用解析器解析SQL */
    stmt_data->parse_tree = tecc_parser_parse(stmt_data->query);
    if (!stmt_data->parse_tree) {
        snprintf(stmt_data->error_msg, sizeof(stmt_data->error_msg),
                "SQL解析失败: %s", stmt_data->query);
        stmt_data->error_code = TECC_PROCESSOR_PARSE_ERROR;
        return TECC_PROCESSOR_PARSE_ERROR;
    }
    
    printf("✅ SQL解析成功，获得 %d 个语句\n", stmt_data->parse_tree->length);
    
    /* 步骤2: 遍历解析树处理每个语句 */
    ListCell *lc;
    foreach(lc, stmt_data->parse_tree) {
        Node *stmt = (Node *)lfirst(lc);
        
        switch (stmt->type) {
            case T_SelectStmt:
                process_select_statement((SelectStmt *)stmt, stmt_data);
                stmt_data->needs_rewrite = 1; /* SELECT语句可能需要改写 */
                break;
                
            case T_InsertStmt:
                printf("处理INSERT语句\n");
                /* 处理INSERT语句逻辑 */
                break;
                
            case T_UpdateStmt:
                printf("处理UPDATE语句\n");
                /* 处理UPDATE语句逻辑 */
                break;
                
            case T_DeleteStmt:
                printf("处理DELETE语句\n");
                /* 处理DELETE语句逻辑 */
                break;
                
            case T_CreateKekStmt:
                printf("处理CREATE KEK语句\n");
                /* 处理CREATE KEK语句逻辑 */
                break;
                
            case T_CreateDekStmt:
                printf("处理CREATE DEK语句\n");
                /* 处理CREATE DEK语句逻辑 */
                break;
                
            case T_DropStmt:
                printf("处理DROP语句\n");
                /* 处理DROP语句逻辑 */
                break;
                
            default:
                printf("未知语句类型: %d\n", stmt->type);
                break;
        }
    }
    
    /* 步骤3: 检查是否需要SQL改写 */
    if (stmt_data->needs_rewrite || stmt_data->where_needs_rewrite) {
        printf("⚠️ 检测到加密列，需要SQL改写\n");
        return TECC_PROCESSOR_NEEDS_REWRITE;
    }
    
    return TECC_PROCESSOR_SUCCESS;
}

/* 清理函数 */
void tecc_processor_cleanup_statement(TeccStatementData *stmt_data) {
    if (stmt_data && stmt_data->parse_tree) {
        /* 释放解析树 */
        tecc_parser_free_tree(stmt_data->parse_tree);
        stmt_data->parse_tree = NULL;
    }
    
    if (stmt_data && stmt_data->rewritten_query) {
        free(stmt_data->rewritten_query);
        stmt_data->rewritten_query = NULL;
    }
}
```

### 2. 简单的集成测试

```c
/* 测试处理器调用解析器 */
void test_processor_integration() {
    printf("=== TECC处理器与解析器集成测试 ===\n\n");
    
    /* 测试用例 */
    const char *test_queries[] = {
        "SELECT * FROM users WHERE encrypted_column = 'value';",
        "INSERT INTO users (id, name, encrypted_data) VALUES (1, 'test', 'secret');",
        "UPDATE users SET encrypted_column = 'new_value' WHERE id = 1;",
        "DELETE FROM users WHERE encrypted_column = 'value';",
        "CREATE KEK master_key WITH (algorithm='AES256');",
        "CREATE DEK data_key WITH (kek='master_key');",
        "DROP DEK data_key;",
        NULL
    };
    
    for (int i = 0; test_queries[i] != NULL; i++) {
        printf("测试 %d: %s\n", i + 1, test_queries[i]);
        
        /* 创建语句数据 */
        TeccStatementData stmt_data = {0};
        stmt_data.query = test_queries[i];
        
        /* 调用处理器预处理 */
        TeccProcessorResult result = tecc_processor_run_pre_query(&stmt_data);
        
        if (result == TECC_PROCESSOR_SUCCESS) {
            printf("  ✅ 处理成功\n");
        } else if (result == TECC_PROCESSOR_NEEDS_REWRITE) {
            printf("  ⚠️ 处理成功，需要SQL改写\n");
        } else {
            printf("  ❌ 处理失败: %s\n", stmt_data.error_msg);
        }
        
        /* 清理 */
        tecc_processor_cleanup_statement(&stmt_data);
        printf("\n");
    }
    
    printf("=== 集成测试完成 ===\n");
}
```

## 错误处理

### 1. 解析错误处理

处理器需要处理解析器可能返回的错误：

```c
/* 调用解析器 */
stmt_data->parse_tree = tecc_parser_parse(stmt_data->query);

if (stmt_data->parse_tree == NULL) {
    /* 解析失败处理 */
    stmt_data->error_code = TECC_PROCESSOR_PARSE_ERROR;
    
    /* 根据错误类型设置错误消息 */
    if (stmt_data->query == NULL) {
        snprintf(stmt_data->error_msg, sizeof(stmt_data->error_msg),
                "SQL查询字符串为空");
    } else if (strlen(stmt_data->query) == 0) {
        snprintf(stmt_data->error_msg, sizeof(stmt_data->error_msg),
                "SQL查询字符串为空字符串");
    } else {
        snprintf(stmt_data->error_msg, sizeof(stmt_data->error_msg),
                "SQL语法错误: %s", stmt_data->query);
    }
    
    return TECC_PROCESSOR_PARSE_ERROR;
}
```

### 2. 节点类型错误处理

```c
/* 处理未知节点类型 */
switch (stmt->type) {
    case T_SelectStmt:
        /* 处理SELECT语句 */
        break;
    case T_InsertStmt:
        /* 处理INSERT语句 */
        break;
    /* ... 其他已知类型 ... */
    default:
        /* 未知节点类型处理 */
        stmt_data->error_code = TECC_PROCESSOR_UNSUPPORTED_STMT;
        snprintf(stmt_data->error_msg, sizeof(stmt_data->error_msg),
                "不支持的语句类型: %d", stmt->type);
        return TECC_PROCESSOR_UNSUPPORTED_STMT;
}
```

## 内存管理

### 1. 解析树内存管理

处理器必须正确管理解析树的内存生命周期：

```c
/* 正确示例：及时释放解析树 */
void process_query(TeccStatementData *stmt_data) {
    /* 解析SQL */
    stmt_data->parse_tree = tecc_parser_parse(stmt_data->query);
    
    if (!stmt_data->parse_tree) {
        return; /* 解析失败 */
    }
    
    /* 处理解析树... */
    
    /* 处理完成后释放解析树 */
    tecc_parser_free_tree(stmt_data->parse_tree);
    stmt_data->parse_tree = NULL;
}

/* 错误示例：内存泄漏 */
void process_query_leak(TeccStatementData *stmt_data) {
    stmt_data->parse_tree = tecc_parser_parse(stmt_data->query);
    /* 忘记调用 tecc_parser_free_tree() ! */
}
```

### 2. 异常安全的内存管理

```c
TeccProcessorResult safe_process_query(TeccStatementData *stmt_data) {
    List *parse_tree = NULL;
    TeccProcessorResult result = TECC_PROCESSOR_SUCCESS;
    
    /* 解析SQL */
    parse_tree = tecc_parser_parse(stmt_data->query);
    if (!parse_tree) {
        stmt_data->error_code = TECC_PROCESSOR_PARSE_ERROR;
        strcpy(stmt_data->error_msg, "SQL解析失败");
        return TECC_PROCESSOR_PARSE_ERROR;
    }
    
    /* 保存解析树引用 */
    stmt_data->parse_tree = parse_tree;
    
    /* 处理逻辑（可能抛出异常） */
    result = process_parsed_tree(stmt_data);
    
    /* 清理：无论处理成功还是失败，都要释放解析树 */
    if (parse_tree) {
        tecc_parser_free_tree(parse_tree);
        stmt_data->parse_tree = NULL;
    }
    
    return result;
}
```

## 集成验证

### 1. 验证脚本

使用提供的验证脚本检查集成兼容性：

```bash
# 运行集成验证脚本
./verify_processor_integration.sh
```

### 2. 手动验证步骤

1. **编译检查**：
   ```bash
   # 编译处理器模块，确保能链接解析器
   make -C ../tecc_processor
   ```

2. **接口兼容性测试**：
   ```bash
   # 运行接口兼容性测试
   ./simple_processor_test
   ```

3. **完整集成测试**：
   ```bash
   # 运行完整的集成测试
   make test_integration
   ./test_integration
   ```

### 3. 验证清单

- [ ] 处理器能成功调用`tecc_parser_parse()`
- [ ] 解析树格式符合处理器要求
- [ ] 位置信息正确（支持SQL改写）
- [ ] 内存管理正确（无泄漏）
- [ ] 错误处理完善
- [ ] 支持所有TECC语句类型

## 最佳实践

### 1. 调用模式

```c
/* 推荐模式：清晰的生命周期管理 */
void recommended_pattern(TeccStatementData *stmt_data) {
    /* 阶段1: 解析 */
    stmt_data->parse_tree = tecc_parser_parse(stmt_data->query);
    if (!stmt_data->parse_tree) {
        handle_parse_error(stmt_data);
        return;
    }
    
    /* 阶段2: 处理 */
    process_parsed_statements(stmt_data);
    
    /* 阶段3: 清理 */
    tecc_parser_free_tree(stmt_data->parse_tree);
    stmt_data->parse_tree = NULL;
}
```

### 2. 性能优化

```c
/* 缓存解析结果（如果查询重复） */
static List *cached_parse_tree = NULL;
static char *cached_query = NULL;

List *get_parsed_tree(const char *query) {
    /* 检查缓存 */
    if (cached_query && strcmp(cached_query, query) == 0) {
        return cached_parse_tree;
    }
    
    /* 解析并缓存 */
    if (cached_parse_tree) {
        tecc_parser_free_tree(cached_parse_tree);
    }
    
    if (cached_query) {
        free(cached_query);
    }
    
    cached_query = strdup(query);
    cached_parse_tree = tecc_parser_parse(query);
    
    return cached_parse_tree;
}
```

## 故障排除

### 常见问题

1. **解析返回NULL**
   - 检查SQL语法是否正确
   - 检查输入字符串是否为空
   - 查看错误日志

2. **节点类型不匹配**
   - 确认使用的节点类型常量正确
   - 检查`tecc_nodes.h`中的类型定义

3. **内存泄漏**
   - 确保每个`tecc_parser_parse()`调用都有对应的`tecc_parser_free_tree()`
   - 使用内存检测工具（如valgrind）检查

4. **位置信息错误**
   - 确认位置信息从0开始
   - 检查节点长度是否正确

### 调试技巧

```c
/* 启用调试输出 */
#define TECC_PARSER_DEBUG 1

List *debug_tecc_parser_parse(const char *query) {
    #ifdef TECC_PARSER_DEBUG
    printf("[DEBUG] 解析SQL: %s\n", query);
    #endif
    
    List *result = tecc_parser_parse(query);
    
    #ifdef TECC_PARSER_DEBUG
    if (result) {
        printf("[DEBUG] 解析成功，节点数: %d\n", result->length);
    } else {
        printf("[DEBUG] 解析失败\n");
    }
    #endif
    
    return result;
}
```

## 总结

TECC处理器通过`tecc_parser_parse()`和`tecc_parser_free_tree()`两个核心接口与前端解析器集成。集成过程简单直接，但需要注意：

1. **正确管理内存生命周期**：及时释放解析树
2. **处理所有错误情况**：解析失败、未知节点类型等
3. **利用位置信息**：支持SQL改写功能
4. **遵循最佳实践**：清晰的调用模式和完善的错误处理

通过本文档的指导和示例代码，开发者可以顺利实现处理器与解析器的集成，为TECC系统的SQL加密列识别和改写功能提供基础支持。

---

**文档版本**: 1.0  
**最后更新**: 2026-04-10  
**适用版本**: TECC4.0  
**相关文件**: `tecc_processor.c`, `fe_adapter.h`, `tecc_nodes.h`