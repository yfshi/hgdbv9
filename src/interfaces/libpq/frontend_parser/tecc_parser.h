/*
 * TECC前端解析器核心接口
 * 严格按照详细设计文档第4章要求实现
 * 确保与处理器模块的接口完全兼容
 *
 * 设计原则：
 * 1. 接口一致性：函数签名、参数类型、返回值与设计文档完全一致
 * 2. 错误处理兼容：错误码定义、错误信息格式与处理器模块兼容
 * 3. 内存管理一致：内存分配释放方式与处理器模块兼容
 * 4. 线程安全：支持多线程并发调用
 * 5. 性能优化：支持大规模SQL语句的高效解析
 */

#ifndef TECC_PARSER_H
#define TECC_PARSER_H

#include "tecc_nodes.h"
#include <stdbool.h>
#include <stddef.h>

/* 前向声明性能相关结构，避免循环依赖 */
typedef struct TeccPerfOptions TeccPerfOptions;
typedef struct TeccParserStatistics TeccParserStatistics;

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 解析选项定义 ==================== */

/*
 * 解析选项结构 - 支持处理器模块的定制化解析需求
 *
 * 兼容性要求：
 * 1. 选项字段必须支持处理器模块的特定解析需求
 * 2. 默认值必须与处理器模块的默认行为一致
 * 3. 扩展性必须支持处理器模块的未来需求
 */
typedef struct TeccParseOptions {
    bool enable_position_tracking;      /* 启用位置跟踪 - 必须支持处理器模块的SQL改写 */
    bool enable_error_recovery;         /* 启用错误恢复 - 必须支持处理器模块的错误处理 */
    bool enable_semantic_check;         /* 启用语义检查 - 必须支持处理器模块的语义分析 */
    bool enable_performance_opt;        /* 启用性能优化 - 必须支持处理器模块的性能需求 */
    int max_error_count;                /* 最大错误数 - 必须支持处理器模块的错误收集 */
    int max_recursion_depth;            /* 最大递归深度 - 必须支持处理器模块的复杂SQL处理 */
    void *user_data;                    /* 用户数据 - 必须支持处理器模块的上下文传递 */
} TeccParseOptions;

/* ==================== 解析结果定义 ==================== */

/*
 * 解析结果结构 - 必须支持处理器模块的结果处理需求
 *
 * 兼容性要求：
 * 1. 解析树必须与处理器模块的TeccStatementData兼容
 * 2. 错误信息必须支持处理器模块的错误处理流程
 * 3. 统计信息必须支持处理器模块的性能分析
 */
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

typedef struct TeccParseResult {
    Node *parse_tree;                   /* 解析树 - 必须与处理器模块的节点结构兼容 */
    ParserError *errors;                /* 错误信息链表 - 必须支持处理器模块的错误收集 */
    int error_count;                    /* 错误数量 - 必须支持处理器模块的错误统计 */
    int warning_count;                  /* 警告数量 - 必须支持处理器模块的警告处理 */
    size_t parse_time_ms;               /* 解析时间(毫秒) - 支持处理器模块的性能监控 */
    size_t memory_used_kb;              /* 内存使用量(KB) - 支持处理器模块的资源监控 */
    bool success;                       /* 解析是否成功 - 必须支持处理器模块的结果判断 */
    TeccParamMappingResult param_mappings; /* 参数映射结果 - 支持处理器模块的参数绑定 */
} TeccParseResult;

/* ==================== 解析上下文定义 ==================== */

/*
 * 解析上下文结构 - 必须支持处理器模块的解析状态管理
 *
 * 兼容性要求：
 * 1. 状态信息必须支持处理器模块的解析流程控制
 * 2. 资源管理必须支持处理器模块的内存和文件管理
 * 3. 错误处理必须支持处理器模块的错误恢复机制
 */
typedef struct TeccParseContext {
    const char *query_string;           /* 原始SQL字符串 - 必须支持处理器模块的SQL处理 */
    size_t query_length;                /* SQL字符串长度 - 必须支持处理器模块的字符串操作 */
    TeccParseOptions options;           /* 解析选项 - 必须支持处理器模块的定制化需求 */
    TeccParseResult result;             /* 解析结果 - 必须支持处理器模块的结果处理 */
    void *scanner_state;                /* 扫描器状态 - 必须支持处理器模块的词法分析 */
    void *parser_state;                 /* 解析器状态 - 必须支持处理器模块的语法分析 */
    struct LocationMapping *location_mappings; /* 位置映射 - 必须支持处理器模块的SQL改写 */
    int current_line;                   /* 当前行号 - 必须支持处理器模块的错误定位 */
    int current_column;                 /* 当前列号 - 必须支持处理器模块的错误定位 */
    size_t total_allocated;             /* 总分配内存 - 必须支持处理器模块的资源监控 */
} TeccParseContext;

/* ==================== 核心解析接口 ==================== */

/*
 * tecc_parser_parse - 基础解析接口
 *
 * 功能：解析SQL语句，生成解析树
 * 兼容性要求：
 * 1. 输入参数必须支持处理器模块的SQL字符串传递
 * 2. 返回值必须与处理器模块的解析树结构兼容
 * 3. 错误处理必须支持处理器模块的错误码定义
 *
 * @param query_string SQL语句字符串
 * @return 解析树根节点，失败返回NULL
 */
Node *tecc_parser_parse_node(const char *query_string);

/*
 * tecc_parser_parse_ex - 扩展解析接口
 *
 * 功能：使用指定选项解析SQL语句，返回完整的解析结果
 * 兼容性要求：
 * 1. 选项参数必须支持处理器模块的定制化需求
 * 2. 结果结构必须包含处理器模块需要的所有信息
 * 3. 性能必须满足处理器模块的实时解析需求
 *
 * @param query_string SQL语句字符串
 * @param options 解析选项，可为NULL使用默认选项
 * @param result 解析结果输出参数
 * @return 解析是否成功
 */
bool tecc_parser_parse_ex(const char *query_string,
                         const TeccParseOptions *options,
                         TeccParseResult *result);

/*
 * tecc_parser_free_tree - 释放解析树
 *
 * 功能：释放解析树占用的所有内存
 * 兼容性要求：
 * 1. 内存释放方式必须与处理器模块的内存管理兼容
 * 2. 必须正确处理嵌套结构和循环引用
 * 3. 必须支持NULL指针的安全处理
 *
 * @param parse_tree 要释放的解析树
 */
void tecc_parser_free_tree_node(Node *parse_tree);

/*
 * tecc_parser_free_result - 释放解析结果
 *
 * 功能：释放解析结果占用的所有内存
 * 兼容性要求：
 * 1. 必须同时释放解析树和错误信息
 * 2. 内存释放必须彻底，避免内存泄漏
 * 3. 必须支持部分释放（如只释放错误信息）
 *
 * @param result 要释放的解析结果
 */
void tecc_parser_free_result(TeccParseResult *result);

/*
 * tecc_parser_get_error - 获取错误信息
 *
 * 功能：获取解析过程中的错误信息
 * 兼容性要求：
 * 1. 错误码必须与处理器模块的错误码定义一致
 * 2. 错误信息格式必须支持处理器模块的错误处理
 * 3. 位置信息必须支持处理器模块的错误定位
 *
 * @param result 解析结果
 * @param index 错误索引（0表示第一个错误）
 * @return 错误信息指针，失败返回NULL
 */
ParserError *tecc_parser_get_error(const TeccParseResult *result, int index);

/*
 * tecc_parser_get_error_count - 获取错误数量
 *
 * 功能：获取解析过程中的错误数量
 * 兼容性要求：
 * 1. 必须同时统计错误和警告
 * 2. 数量统计必须准确，支持处理器模块的错误处理
 *
 * @param result 解析结果
 * @return 错误数量
 */
int tecc_parser_get_error_count(const TeccParseResult *result);

/*
 * tecc_parser_get_warning_count - 获取警告数量
 *
 * 功能：获取解析过程中的警告数量
 * 兼容性要求：
 * 1. 警告统计必须与错误统计分离
 * 2. 必须支持处理器模块的警告处理需求
 *
 * @param result 解析结果
 * @return 警告数量
 */
int tecc_parser_get_warning_count(const TeccParseResult *result);

/*
 * tecc_parser_clone_tree - 克隆解析树
 *
 * 功能：深度克隆解析树
 * 兼容性要求：
 * 1. 克隆必须完全，包括所有嵌套结构
 * 2. 内存分配方式必须与原始解析树一致
 * 3. 必须支持处理器模块的解析树复制需求
 *
 * @param parse_tree 要克隆的解析树
 * @return 克隆后的解析树，失败返回NULL
 */
Node *tecc_parser_clone_tree(const Node *parse_tree);

/*
 * tecc_parser_validate - 验证解析树
 *
 * 功能：验证解析树的完整性和正确性
 * 兼容性要求：
 * 1. 验证规则必须与处理器模块的语义检查一致
 * 2. 必须支持处理器模块的预验证需求
 * 3. 错误报告必须详细，支持处理器模块的错误分析
 *
 * @param parse_tree 要验证的解析树
 * @param errors 错误信息输出参数
 * @return 验证是否通过
 */
bool tecc_parser_validate(const Node *parse_tree, ParserError **errors);

/* ==================== 工具函数 ==================== */

/*
 * tecc_parser_init_options - 初始化解析选项
 *
 * 功能：使用默认值初始化解析选项
 * 兼容性要求：
 * 1. 默认值必须与处理器模块的默认行为一致
 * 2. 必须支持处理器模块的选项初始化需求
 *
 * @param options 要初始化的选项结构
 */
void tecc_parser_init_options(TeccParseOptions *options);

/*
 * tecc_parser_init_result - 初始化解析结果
 *
 * 功能：初始化解析结果结构
 * 兼容性要求：
 * 1. 初始化必须彻底，避免未初始化字段
 * 2. 必须支持处理器模块的结果初始化需求
 *
 * @param result 要初始化的结果结构
 */
void tecc_parser_init_result(TeccParseResult *result);

/*
 * tecc_parser_get_param_mapping - 获取参数列映射
 *
 * 功能：获取参数索引到列三元组的映射
 * 兼容性要求：
 * 1. 必须支持处理器模块的参数绑定需求
 * 2. 映射信息必须准确，支持加密判定
 * 3. 必须支持多参数映射
 *
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

/*
 * tecc_parser_add_param_mapping - 添加参数列映射
 *
 * 功能：添加参数索引到列三元组的映射
 * 兼容性要求：
 * 1. 必须支持处理器模块的动态映射添加
 * 2. 必须处理重复映射（覆盖或忽略）
 * 3. 内存管理必须与解析器一致
 *
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

/*
 * tecc_parser_clear_param_mappings - 清除所有参数映射
 *
 * 功能：清除解析结果中的所有参数映射
 * 兼容性要求：
 * 1. 必须彻底清除映射，释放相关内存
 * 2. 必须支持处理器模块的映射清理需求
 *
 * @param result 解析结果
 */
void tecc_parser_clear_param_mappings(TeccParseResult *result);

/*
 * tecc_parser_get_param_mapping_count - 获取参数映射数量
 *
 * 功能：获取解析结果中的参数映射数量
 * 兼容性要求：
 * 1. 必须准确统计映射数量
 * 2. 必须支持处理器模块的映射统计需求
 *
 * @param result 解析结果
 * @return 参数映射数量
 */
int tecc_parser_get_param_mapping_count(const TeccParseResult *result);

/*
 * tecc_parser_set_default_options - 设置全局默认选项
 *
 * 功能：设置全局默认解析选项
 * 兼容性要求：
 * 1. 必须支持处理器模块的全局配置需求
 * 2. 线程安全，支持多线程并发访问
 *
 * @param options 默认选项
 */
void tecc_parser_set_default_options(const TeccParseOptions *options);

/*
 * tecc_parser_get_default_options - 获取全局默认选项
 *
 * 功能：获取全局默认解析选项
 * 兼容性要求：
 * 1. 线程安全，支持多线程并发访问
 * 2. 返回的选项必须可修改，支持处理器模块的定制化
 *
 * @return 默认选项指针
 */
TeccParseOptions *tecc_parser_get_default_options(void);

/* ==================== 性能监控接口 ==================== */

/* 解析器统计信息结构（在tecc_perf.h中定义） */

/*
 * tecc_parser_get_statistics - 获取解析统计信息
 *
 * 功能：获取解析器的性能统计信息
 * 兼容性要求：
 * 1. 统计信息必须详细，支持处理器模块的性能分析
 * 2. 必须支持处理器模块的性能监控需求
 *
 * @param stats 统计信息输出参数
 */
void tecc_parser_get_statistics(TeccParserStatistics *stats);

/*
 * tecc_parser_reset_statistics - 重置解析统计信息
 *
 * 功能：重置解析器的性能统计信息
 * 兼容性要求：
 * 1. 必须支持处理器模块的统计重置需求
 * 2. 线程安全，支持多线程并发访问
 */
void tecc_parser_reset_statistics(void);

/* ==================== 错误处理接口 ==================== */

/*
 * tecc_parser_set_error_handler - 设置错误处理函数
 *
 * 功能：设置自定义错误处理函数
 * 兼容性要求：
 * 1. 必须支持处理器模块的自定义错误处理需求
 * 2. 错误处理函数必须符合指定的接口规范
 *
 * @param handler 错误处理函数
 * @param user_data 用户数据
 */
void tecc_parser_set_error_handler(
    void (*handler)(int error_code, const char *error_message,
                   const LocationInfo *location, void *user_data),
    void *user_data);

/*
 * tecc_parser_clear_errors - 清除错误信息
 *
 * 功能：清除解析结果中的错误信息
 * 兼容性要求：
 * 1. 必须彻底清除错误信息，释放相关内存
 * 2. 必须支持处理器模块的错误清理需求
 *
 * @param result 解析结果
 */
void tecc_parser_clear_errors(TeccParseResult *result);

/* ==================== 性能优化解析函数 ==================== */

/*
 * tecc_parse_sql - 性能优化版本的SQL解析
 *
 * 功能：解析SQL语句，支持缓存和性能优化
 * 兼容性要求：
 * 1. 必须支持处理器模块的性能优化需求
 * 2. 必须支持缓存机制，提高重复查询性能
 * 3. 错误处理必须与处理器模块兼容
 *
 * @param sql SQL语句字符串
 * @param errors 错误信息输出参数
 * @return 解析树根节点，失败返回NULL
 */
Node *tecc_parse_sql(const char *sql, ParserError **errors);

/* ==================== 性能优化接口 ==================== */

/*
 * tecc_parser_enable_performance_opt - 启用性能优化
 *
 * 功能：启用解析器的性能优化功能
 * 兼容性要求：
 * 1. 必须支持处理器模块的性能优化需求
 * 2. 优化选项必须可配置，支持处理器模块的定制化
 *
 * @param enable 是否启用性能优化
 * @param options 性能优化选项，可为NULL使用默认选项
 * @return 是否成功启用
 */
bool tecc_parser_enable_performance_opt(bool enable, const TeccPerfOptions *options);

/*
 * tecc_parser_get_performance_stats - 获取性能统计信息
 *
 * 功能：获取解析器的详细性能统计信息
 * 兼容性要求：
 * 1. 统计信息必须详细，支持处理器模块的性能分析
 * 2. 必须支持处理器模块的性能监控需求
 *
 * @param stats 性能统计信息输出参数
 */
void tecc_parser_get_performance_stats(TeccParserStatistics *stats);

/*
 * tecc_parser_enable_cache - 启用解析缓存
 *
 * 功能：启用解析结果缓存以提高性能
 * 兼容性要求：
 * 1. 缓存必须与处理器模块的缓存需求兼容
 * 2. 缓存大小必须可配置，支持处理器模块的资源管理
 *
 * @param enable 是否启用缓存
 * @param cache_size 缓存大小（条目数）
 * @return 是否成功启用
 */
bool tecc_parser_enable_cache(bool enable, size_t cache_size);

/*
 * tecc_parser_clear_cache - 清空解析缓存
 *
 * 功能：清空所有缓存条目
 * 兼容性要求：
 * 1. 必须彻底清空缓存，释放所有缓存内存
 * 2. 必须支持处理器模块的缓存管理需求
 */
void tecc_parser_clear_cache(void);

/*
 * tecc_parser_enable_memory_pool - 启用内存池
 *
 * 功能：启用内存池以提高内存分配性能
 * 兼容性要求：
 * 1. 内存池必须与处理器模块的内存管理兼容
 * 2. 内存池大小必须可配置，支持处理器模块的资源管理
 *
 * @param enable 是否启用内存池
 * @param pool_size 内存池大小（字节）
 * @return 是否成功启用
 */
bool tecc_parser_enable_memory_pool(bool enable, size_t pool_size);

/*
 * tecc_parser_enable_parallel_parsing - 启用并行解析
 *
 * 功能：启用并行解析以提高性能
 * 兼容性要求：
 * 1. 并行解析必须与处理器模块的并发需求兼容
 * 2. 最大并行任务数必须可配置，支持处理器模块的资源管理
 *
 * @param enable 是否启用并行解析
 * @param max_tasks 最大并行任务数
 * @return 是否成功启用
 */
bool tecc_parser_enable_parallel_parsing(bool enable, int max_tasks);

/* ==================== Bison解析器接口 ==================== */

/*
 * tecc_bison_parse - 使用Bison解析SQL语句
 *
 * 功能：调用Bison生成的解析器解析SQL
 * 兼容性要求：
 * 1. 必须与现有的tecc_parser_parse_ex接口兼容
 * 2. 必须支持位置跟踪和错误恢复
 * 3. 必须使用tecc_nodes.h中的数据结构
 *
 * @param query_string SQL语句字符串
 * @param result 解析结果输出参数
 * @return 解析是否成功
 */
bool tecc_bison_parse(const char *query_string, TeccParseResult *result);

/* ==================== 适配器函数声明 ==================== */

/*
 * tecc_parser_parse_list - 适配器函数，返回List*格式的解析树
 *
 * 功能：解析SQL语句，返回List*格式的解析树
 * 兼容性要求：
 * 1. 与设计文档第5.1节要求的接口完全一致
 * 2. 内部调用tecc_parser_parse_ex获取Node*解析树
 * 3. 将Node*包装为List*格式返回
 *
 * @param query_string SQL语句字符串
 * @return List*格式的解析树，失败返回NULL
 */
List *tecc_parser_parse_list(const char *query_string);

/*
 * tecc_parser_free_tree_list - 适配器函数，释放List*格式的解析树
 *
 * 功能：释放解析树占用的所有内存
 * 兼容性要求：
 * 1. 正确处理List*结构中的Node*节点
 * 2. 同时释放List结构和内部的Node结构
 * 3. 支持NULL指针的安全处理
 *
 * @param parse_tree 要释放的List*格式解析树
 */
void tecc_parser_free_tree_list(List *parse_tree);

/* ==================== 全局变量声明 ==================== */

/*
 * tecc_parsetree - 全局解析树变量
 *
 * 功能：存储Bison解析器生成的解析树
 * 注意：由tecc_grammar.c定义，其他文件通过此声明访问
 */
extern List *tecc_parsetree;

#ifdef __cplusplus
}
#endif

#endif /* TECC_PARSER_H */