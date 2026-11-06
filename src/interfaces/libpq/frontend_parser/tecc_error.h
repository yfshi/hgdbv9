/*
 * TECC前端解析器错误处理头文件
 * 严格按照详细设计文档第4章要求实现
 * 支持处理器模块的错误处理需求
 */

#ifndef TECC_ERROR_H
#define TECC_ERROR_H

#include "tecc_nodes.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 错误码定义 ==================== */

/* 语法错误 (1000-1999) */
#define PARSER_SYNTAX_ERROR               1000
#define PARSER_KEYWORD_MISSING_ERROR      1001
#define PARSER_UNEXPECTED_TOKEN_ERROR     1002
#define PARSER_INVALID_IDENTIFIER_ERROR   1003
#define PARSER_MISSING_PARENTHESIS_ERROR  1004
#define PARSER_INVALID_NUMBER_ERROR       1005
#define PARSER_INVALID_STRING_ERROR       1006

/* 语义错误 (2000-2999) */
#define PARSER_SEMANTIC_ERROR             2000
#define PARSER_TYPE_MISMATCH_ERROR        2001
#define PARSER_COLLATION_CHANGE_ERROR     2002
#define PARSER_UNDEFINED_TABLE_ERROR      2003
#define PARSER_UNDEFINED_COLUMN_ERROR     2004
#define PARSER_DUPLICATE_TABLE_ERROR      2005
#define PARSER_DUPLICATE_COLUMN_ERROR     2006
#define PARSER_INVALID_FUNCTION_ERROR     2007
#define PARSER_INVALID_ARGUMENT_ERROR     2008

/* 警告 (900-999) */
#define PARSER_WARNING                    900

/* 系统错误 (3000-3999) */
#define PARSER_MEMORY_ERROR               3000
#define PARSER_IO_ERROR                   3001
#define PARSER_INTERNAL_ERROR             3002
#define PARSER_CONFIG_ERROR               3003

/* TECC专用错误 (4000-4999) */
#define PARSER_INVALID_KEK_NAME_ERROR     4001
#define PARSER_INVALID_DEK_NAME_ERROR     4002
#define PARSER_KEK_NOT_FOUND_ERROR        4003
#define PARSER_DEK_NOT_FOUND_ERROR        4004
#define PARSER_ENCRYPTION_ALGORITHM_ERROR 4005
#define PARSER_INVALID_KEY_SIZE_ERROR     4006
#define PARSER_ENCRYPTION_MODE_ERROR      4007
#define PARSER_INVALID_CIPHER_TEXT_ERROR  4008
#define PARSER_ENCRYPTED_COLUMN_CONSTRAINT_ERROR 4009

/* COPY命令错误 (4100-4199) */
#define PARSER_COPY_RELATION_QUERY_CONFLICT_ERROR 4100
#define PARSER_COPY_NO_RELATION_OR_QUERY_ERROR    4101
#define PARSER_COPY_FROM_SELECT_NOT_SUPPORTED_ERROR 4102
#define PARSER_COPY_FILE_NOT_SUPPORTED_ERROR     4103

/* ==================== 错误处理接口 ==================== */

/*
 * tecc_error_report - 报告错误
 *
 * 功能：报告解析过程中的错误
 * 兼容性要求：
 * 1. 错误信息必须详细，包含错误码、位置、详情等
 * 2. 必须支持处理器模块的错误处理需求
 * 3. 必须支持错误恢复和继续解析
 *
 * @param error_code 错误码
 * @param sql_fragment 相关的SQL片段
 * @param location 错误位置信息
 * @param detail_format 错误详情格式字符串
 * @param ... 格式字符串参数
 */
void tecc_error_report(int error_code,
                      const char *sql_fragment,
                      const LocationInfo *location,
                      const char *detail_format, ...);

/*
 * tecc_error_report_simple - 报告错误（简化版本）
 *
 * 功能：报告错误，使用简化的位置信息
 * 兼容性要求：
 * 1. 必须支持处理器模块的快速错误报告需求
 * 2. 位置信息必须准确，支持SQL改写
 *
 * @param error_code 错误码
 * @param location 错误位置
 */
void tecc_error_report_simple(int error_code, int location);

/*
 * tecc_error_set_handler - 设置错误处理器
 *
 * 功能：设置自定义错误处理器
 * 兼容性要求：
 * 1. 必须支持处理器模块的自定义错误处理需求
 * 2. 必须支持多个错误处理器
 *
 * @param handler 错误处理函数
 * @param user_data 用户数据
 */
void tecc_error_set_handler(void (*handler)(int error_code,
                                           const char *error_message,
                                           const LocationInfo *location,
                                           void *user_data),
                           void *user_data);

/*
 * tecc_error_remove_handler - 移除错误处理器
 *
 * 功能：移除指定的错误处理器
 * 兼容性要求：
 * 1. 必须支持处理器模块的动态处理器管理需求
 * 2. 必须正确处理NULL指针
 *
 * @param handler 要移除的错误处理函数
 */
void tecc_error_remove_handler(void (*handler)(int error_code,
                                              const char *error_message,
                                              const LocationInfo *location,
                                              void *user_data));

/*
 * tecc_error_clear_all - 清除所有错误
 *
 * 功能：清除当前所有的错误信息
 * 兼容性要求：
 * 1. 必须彻底释放所有错误信息占用的内存
 * 2. 必须重置错误计数器
 */
void tecc_error_clear_all(void);

/*
 * tecc_error_append - 追加错误到全局错误列表
 */
void tecc_error_append(ParserError *error);

/*
 * tecc_error_get_list - 获取错误列表
 *
 * 功能：获取当前所有的错误信息
 * 兼容性要求：
 * 1. 必须返回完整的错误链表
 * 2. 错误信息必须包含所有必要的字段
 *
 * @return 错误信息链表
 */
ParserError *tecc_error_get_list(void);

/*
 * tecc_error_get_count - 获取错误数量
 *
 * 功能：获取当前错误的数量
 * 兼容性要求：
 * 1. 计数必须准确，反映当前错误状态
 * 2. 必须支持处理器模块的错误统计需求
 *
 * @return 错误数量
 */
int tecc_error_get_count(void);

/*
 * tecc_error_get_warning_count - 获取警告数量
 *
 * 功能：获取当前警告的数量
 * 兼容性要求：
 * 1. 计数必须准确，反映当前警告状态
 * 2. 必须支持处理器模块的警告统计需求
 *
 * @return 警告数量
 */
int tecc_error_get_warning_count(void);

/*
 * tecc_error_set_recovery - 设置错误恢复
 *
 * 功能：启用或禁用错误恢复
 * 兼容性要求：
 * 1. 必须支持处理器模块的错误恢复控制需求
 * 2. 状态切换必须立即生效
 *
 * @param enabled 是否启用错误恢复
 */
void tecc_error_set_recovery(bool enabled);

/*
 * tecc_error_set_max_errors - 设置最大错误数
 *
 * 功能：设置最大允许的错误数量
 * 兼容性要求：
 * 1. 必须支持处理器模块的错误限制需求
 * 2. 限制必须立即生效
 *
 * @param max_errors 最大错误数
 */
void tecc_error_set_max_errors(int max_errors);

/*
 * tecc_error_set_language - 设置错误消息语言
 *
 * 功能：设置错误消息的语言
 * 兼容性要求：
 * 1. 必须支持中英文错误消息
 * 2. 语言设置必须立即生效
 *
 * @param language 语言代码（"en"或"zh"）
 */
void tecc_error_set_language(const char *language);

/*
 * tecc_error_create - 创建错误信息
 *
 * 功能：手动创建错误信息
 * 兼容性要求：
 * 1. 必须支持处理器模块的手动错误创建需求
 * 2. 错误信息必须完整
 *
 * @param error_code 错误码
 * @param message 错误消息
 * @param sql_fragment SQL片段
 * @param location 位置信息
 * @param severity 严重级别
 * @param hint 修复提示
 * @param detail 详细描述
 * @return 错误信息
 */
ParserError *tecc_error_create(int error_code,
                              const char *message,
                              const char *sql_fragment,
                              const LocationInfo *location,
                              int severity,
                              const char *hint,
                              const char *detail);

/*
 * tecc_error_copy - 复制错误信息
 *
 * 功能：复制错误信息
 * 兼容性要求：
 * 1. 必须支持处理器模块的错误信息复制需求
 * 2. 复制必须完整，包括所有字段
 *
 * @param error 要复制的错误信息
 * @return 复制的错误信息
 */
ParserError *tecc_error_copy(const ParserError *error);

/*
 * tecc_error_free - 释放错误信息
 *
 * 功能：释放单个错误信息
 * 兼容性要求：
 * 1. 必须彻底释放所有相关内存
 * 2. 必须支持NULL指针
 *
 * @param error 要释放的错误信息
 */
void tecc_error_free(ParserError *error);

/*
 * tecc_error_free_list - 释放错误列表
 *
 * 功能：释放错误信息链表
 * 兼容性要求：
 * 1. 必须彻底释放链表中的所有错误信息
 * 2. 必须支持NULL指针
 *
 * @param errors 要释放的错误信息链表
 */
void tecc_error_free_list(ParserError *errors);

/*
 * tecc_error_to_string - 错误信息转字符串
 *
 * 功能：将错误信息转换为可读的字符串
 * 兼容性要求：
 * 1. 字符串格式必须与处理器模块兼容
 * 2. 必须包含所有重要信息
 *
 * @param error 错误信息
 * @return 字符串表示
 */
char *tecc_error_to_string(const ParserError *error);

/*
 * tecc_error_get_severity_name - 获取错误严重级别名称
 *
 * 功能：获取错误严重级别的可读名称
 * 兼容性要求：
 * 1. 名称必须与处理器模块的级别定义一致
 * 2. 必须支持所有预定义的严重级别
 *
 * @param severity 严重级别
 * @return 严重级别名称
 */
const char *tecc_error_get_severity_name(int severity);

/*
 * tecc_error_is_recoverable - 检查是否可恢复
 *
 * 功能：检查特定错误是否可恢复
 * 兼容性要求：
 * 1. 判断必须准确，反映错误的实际可恢复性
 * 2. 必须支持处理器模块的错误恢复决策
 *
 * @param error_code 错误码
 * @return 是否可恢复
 */
bool tecc_error_is_recoverable(int error_code);

/*
 * tecc_error_reset_context - 重置错误上下文
 *
 * 功能：重置错误处理上下文到初始状态
 * 兼容性要求：
 * 1. 必须彻底清理所有错误信息和处理器
 * 2. 重置后必须可以重新使用
 */
void tecc_error_reset_context(void);

/* Begin - ReqID:TECC4.0 */
/*
 * tecc_error - Bison错误报告函数
 * 这个函数被tecc_grammar.c中的yyerror宏调用
 */
void tecc_error(const char *msg);
/* End - ReqID:TECC4.0 */

#ifdef __cplusplus
}
#endif

#endif /* TECC_ERROR_H */