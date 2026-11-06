/*
 * TECC解析器使用示例
 * 演示如何在前端代码中使用统一版TECC解析器
 */

#include "tecc_parser_simple.h"
#include <stdio.h>
#include <stdlib.h>

/* 处理器兼容接口 */
List *tecc_parser_parse(const char *query_string);
void tecc_parser_free_tree(List *parse_tree);

/* 示例：处理SQL语句 */
void process_sql_statement(const char *sql) {
    printf("处理SQL: %s\n", sql);

    /* 1. 解析SQL语句 */
    List *parse_tree = tecc_parser_parse(sql);

    if (!parse_tree) {
        printf("  解析失败\n");
        return;
    }

    printf("  解析成功，获得 %d 个语句\n", parse_tree->length);

    /* 2. 遍历解析树 */
    if (parse_tree->head) {
        ListCell *cell = parse_tree->head;
        int stmt_count = 0;

        while (cell) {
            Node *stmt = (Node *)cell->data;
            if (stmt) {
                stmt_count++;

                /* 根据语句类型处理 */
                switch (stmt->type) {
                    case T_SelectStmt:
                        printf("    语句 %d: SELECT (位置: %d-%d)\n",
                               stmt_count, stmt->location, stmt->location + stmt->length);
                        /* 这里可以添加SELECT语句的具体处理逻辑 */
                        break;

                    case T_InsertStmt:
                        printf("    语句 %d: INSERT (位置: %d-%d)\n",
                               stmt_count, stmt->location, stmt->location + stmt->length);
                        /* 这里可以添加INSERT语句的具体处理逻辑 */
                        break;

                    case T_CreateKekStmt:
                        printf("    语句 %d: CREATE KEK (位置: %d-%d)\n",
                               stmt_count, stmt->location, stmt->location + stmt->length);
                        /* 这里可以添加CREATE KEK语句的具体处理逻辑 */
                        break;

                    case T_CreateDekStmt:
                        printf("    语句 %d: CREATE DEK (位置: %d-%d)\n",
                               stmt_count, stmt->location, stmt->location + stmt->length);
                        /* 这里可以添加CREATE DEK语句的具体处理逻辑 */
                        break;

                    default:
                        printf("    语句 %d: 类型=%d (位置: %d-%d)\n",
                               stmt_count, stmt->type, stmt->location, stmt->location + stmt->length);
                        break;
                }
            }
            cell = cell->next;
        }
    }

    /* 3. 语义验证（可选） */
    if (parse_tree->head) {
        Node *first_stmt = (Node *)parse_tree->head->data;
        ParserError *errors = NULL;

        if (tecc_parser_validate(first_stmt, &errors)) {
            printf("  语义验证: 通过\n");
        } else {
            printf("  语义验证: 失败\n");
            /* 处理错误信息 */
            while (errors) {
                ParserError *next = errors->next;
                free(errors->error_message);
                free(errors);
                errors = next;
            }
        }
    }

    /* 4. 获取统计信息（可选） */
    TeccParserStatistics stats = tecc_parser_get_statistics();
    printf("  统计: 总解析次数=%d, 成功=%d, 错误=%d\n",
           stats.parse_count, stats.success_count, stats.error_count);

    /* 5. 释放解析树 */
    tecc_parser_free_tree(parse_tree);
    printf("  内存已释放\n\n");
}

/* 示例：使用扩展解析接口 */
void use_extended_interface(const char *sql) {
    printf("使用扩展接口解析: %s\n", sql);

    Node *result_node = NULL;
    TeccParserStatistics stats;
    char *error_msg = NULL;

    /* 配置解析选项 */
    TeccParseOptions options;
    tecc_parser_init_options(&options);
    options.enable_position_tracking = true;
    options.enable_error_recovery = true;
    options.enable_semantic_check = true;

    /* 调用扩展解析接口 */
    bool success = tecc_parser_parse_ex(sql, &options, &result_node, &stats, &error_msg);

    if (success && result_node) {
        printf("  解析成功\n");
        printf("  节点类型: %d, 位置: %d, 长度: %d\n",
               result_node->type, result_node->location, result_node->length);
        printf("  统计: 解析次数=%d, 成功=%d\n", stats.parse_count, stats.success_count);

        tecc_parser_free_tree_node(result_node);
    } else {
        printf("  解析失败: %s\n", error_msg ? error_msg : "未知错误");
        if (error_msg) free(error_msg);
    }

    printf("\n");
}

int main() {
    printf("=== TECC解析器使用示例 ===\n\n");

    /* 重置统计信息 */
    tecc_parser_reset_statistics();

    /* 示例1: 基本使用 */
    printf("1. 基本使用示例:\n");
    process_sql_statement("SELECT * FROM encrypted_users WHERE id = 1;");
    process_sql_statement("INSERT INTO logs (message) VALUES ('encrypted data');");
    process_sql_statement("CREATE KEK master_key_2024;");
    process_sql_statement("DROP DEK old_data_key;");

    /* 示例2: 扩展接口使用 */
    printf("2. 扩展接口示例:\n");
    use_extended_interface("SELECT name, age FROM employees WHERE department = 'IT';");
    use_extended_interface("UPDATE products SET price = price * 1.1 WHERE category = 'electronics';");

    /* 示例3: 错误处理 */
    printf("3. 错误处理示例:\n");
    process_sql_statement("");  /* 空字符串 */
    process_sql_statement("INVALID SQL STATEMENT;");  /* 无效SQL */

    /* 显示最终统计信息 */
    printf("4. 最终统计信息:\n");
    TeccParserStatistics final_stats = tecc_parser_get_statistics();
    printf("  总解析次数: %d\n", final_stats.parse_count);
    printf("  总成功次数: %d\n", final_stats.success_count);
    printf("  总错误次数: %d\n", final_stats.error_count);
    printf("  成功率: %.1f%%\n",
           final_stats.parse_count > 0 ?
           (float)final_stats.success_count / final_stats.parse_count * 100 : 0.0);

    printf("\n=== 示例完成 ===\n");
    printf("统一版TECC解析器已准备好用于前端SQL处理。\n");

    return 0;
}