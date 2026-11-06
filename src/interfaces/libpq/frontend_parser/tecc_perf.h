/*
 * TECC前端解析器性能优化头文件
 * 严格按照详细设计文档第4章要求实现
 * 支持处理器模块的性能优化需求
 */

#ifndef TECC_PERF_H
#define TECC_PERF_H

#include "tecc_nodes.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 性能统计结构 ==================== */

/* 解析器性能统计 */
typedef struct TeccParserStatistics {
    uint64_t parse_time_ns;          /* 解析时间（纳秒） */
    uint64_t semantic_time_ns;       /* 语义分析时间（纳秒） */
    uint64_t error_time_ns;          /* 错误处理时间（纳秒） */
    uint64_t total_time_ns;          /* 总时间（纳秒） */

    size_t memory_used;              /* 内存使用量（字节） */
    size_t peak_memory;              /* 峰值内存使用量（字节） */

    int nodes_created;               /* 创建的节点数 */
    int nodes_freed;                 /* 释放的节点数 */
    int strings_allocated;           /* 分配的字符串数 */
    int strings_freed;               /* 释放的字符串数 */

    int parse_calls;                 /* 解析调用次数 */
    int semantic_calls;              /* 语义分析调用次数 */
    int error_calls;                 /* 错误处理调用次数 */

    int cache_hits;                  /* 缓存命中次数 */
    int cache_misses;                /* 缓存未命中次数 */

    int parallel_tasks;              /* 并行任务数 */
    int max_parallel_tasks;          /* 最大并行任务数 */
} TeccParserStatistics;

/* 性能优化选项 */
typedef struct TeccPerfOptions {
    bool enable_memory_pool;         /* 是否启用内存池 */
    bool enable_cache;               /* 是否启用缓存 */
    bool enable_parallel_parsing;    /* 是否启用并行解析 */
    bool enable_statistics;          /* 是否启用性能统计 */

    size_t memory_pool_size;         /* 内存池大小（字节） */
    size_t cache_size;               /* 缓存大小（条目数） */
    int max_parallel_tasks;          /* 最大并行任务数 */

    int parse_timeout_ms;            /* 解析超时时间（毫秒） */
    int semantic_timeout_ms;         /* 语义分析超时时间（毫秒） */

    bool aggressive_optimization;    /* 是否启用激进优化 */
    bool profile_mode;               /* 是否启用性能分析模式 */
} TeccPerfOptions;

/* ==================== 性能统计接口 ==================== */

/*
 * tecc_perf_get_statistics - 获取性能统计信息
 *
 * 功能：获取当前解析器的性能统计信息
 * 兼容性要求：
 * 1. 统计信息必须详细，支持处理器模块的性能分析
 * 2. 必须支持处理器模块的监控需求
 *
 * @param stats 统计信息输出参数
 */
void tecc_perf_get_statistics(TeccParserStatistics *stats);

/*
 * tecc_perf_reset_statistics - 重置性能统计
 *
 * 功能：重置所有性能统计计数器
 * 兼容性要求：
 * 1. 重置必须彻底，所有计数器归零
 * 2. 必须支持处理器模块的统计重置需求
 */
void tecc_perf_reset_statistics(void);

/*
 * tecc_perf_print_statistics - 打印性能统计信息
 *
 * 功能：以可读格式打印性能统计信息
 * 兼容性要求：
 * 1. 输出格式必须与处理器模块兼容
 * 2. 必须支持处理器模块的调试需求
 *
 * @param verbose 是否详细输出
 */
void tecc_perf_print_statistics(bool verbose);

/* ==================== 内存优化接口 ==================== */

/*
 * tecc_perf_init_memory_pool - 初始化内存池
 *
 * 功能：初始化内存池以提高内存分配性能
 * 兼容性要求：
 * 1. 内存池必须与处理器模块的内存管理兼容
 * 2. 必须支持处理器模块的内存优化需求
 *
 * @param pool_size 内存池大小
 * @return 是否初始化成功
 */
bool tecc_perf_init_memory_pool(size_t pool_size);

/*
 * tecc_perf_destroy_memory_pool - 销毁内存池
 *
 * 功能：销毁内存池并释放所有内存
 * 兼容性要求：
 * 1. 必须彻底释放所有内存池内存
 * 2. 必须支持处理器模块的资源清理需求
 */
void tecc_perf_destroy_memory_pool(void);

/*
 * tecc_perf_get_memory_usage - 获取内存使用情况
 *
 * 功能：获取当前内存使用情况
 * 兼容性要求：
 * 1. 统计必须准确，反映实际内存使用
 * 2. 必须支持处理器模块的内存监控需求
 *
 * @param current_usage 当前使用量输出参数
 * @param peak_usage 峰值使用量输出参数
 */
void tecc_perf_get_memory_usage(size_t *current_usage, size_t *peak_usage);

/* ==================== 缓存接口 ==================== */

/*
 * tecc_perf_init_cache - 初始化缓存
 *
 * 功能：初始化解析结果缓存
 * 兼容性要求：
 * 1. 缓存必须与处理器模块的缓存需求兼容
 * 2. 必须支持处理器模块的性能优化需求
 *
 * @param cache_size 缓存大小
 * @return 是否初始化成功
 */
bool tecc_perf_init_cache(size_t cache_size);

/*
 * tecc_perf_destroy_cache - 销毁缓存
 *
 * 功能：销毁缓存并释放所有缓存条目
 * 兼容性要求：
 * 1. 必须彻底释放所有缓存内存
 * 2. 必须支持处理器模块的缓存清理需求
 */
void tecc_perf_destroy_cache(void);

/*
 * tecc_perf_cache_put - 将解析结果放入缓存
 *
 * 功能：将SQL解析结果放入缓存
 * 兼容性要求：
 * 1. 缓存键必须唯一，避免冲突
 * 2. 缓存值必须完整，包含所有必要信息
 *
 * @param sql SQL语句
 * @param parse_tree 解析树
 * @param semantic_errors 语义错误
 * @return 是否缓存成功
 */
bool tecc_perf_cache_put(const char *sql, const Node *parse_tree,
                        const ParserError *semantic_errors);

/*
 * tecc_perf_cache_get - 从缓存获取解析结果
 *
 * 功能：从缓存获取SQL解析结果
 * 兼容性要求：
 * 1. 查找必须快速，支持处理器模块的实时需求
 * 2. 缓存命中必须返回完整的解析结果
 *
 * @param sql SQL语句
 * @param parse_tree 解析树输出参数
 * @param semantic_errors 语义错误输出参数
 * @return 是否命中缓存
 */
bool tecc_perf_cache_get(const char *sql, Node **parse_tree,
                        ParserError **semantic_errors);

/*
 * tecc_perf_cache_clear - 清空缓存
 *
 * 功能：清空所有缓存条目
 * 兼容性要求：
 * 1. 清空必须彻底，释放所有缓存内存
 * 2. 必须支持处理器模块的缓存管理需求
 */
void tecc_perf_cache_clear(void);

/*
 * tecc_perf_cache_get_stats - 获取缓存统计信息
 *
 * 功能：获取缓存命中率等统计信息
 * 兼容性要求：
 * 1. 统计必须准确，反映缓存性能
 * 2. 必须支持处理器模块的缓存优化需求
 *
 * @param hits 命中次数输出参数
 * @param misses 未命中次数输出参数
 * @param size 当前缓存大小输出参数
 */
void tecc_perf_cache_get_stats(int *hits, int *misses, size_t *size);

/* ==================== 并行解析接口 ==================== */

/*
 * tecc_perf_init_parallel - 初始化并行解析
 *
 * 功能：初始化并行解析环境
 * 兼容性要求：
 * 1. 并行解析必须与处理器模块的并发需求兼容
 * 2. 必须支持处理器模块的并行处理需求
 *
 * @param max_tasks 最大并行任务数
 * @return 是否初始化成功
 */
bool tecc_perf_init_parallel(int max_tasks);

/*
 * tecc_perf_destroy_parallel - 销毁并行解析环境
 *
 * 功能：销毁并行解析环境并释放所有资源
 * 兼容性要求：
 * 1. 必须彻底释放所有并行解析资源
 * 2. 必须支持处理器模块的资源清理需求
 */
void tecc_perf_destroy_parallel(void);

/*
 * tecc_perf_parse_parallel - 并行解析SQL语句
 *
 * 功能：并行解析多个SQL语句
 * 兼容性要求：
 * 1. 并行解析必须正确，避免数据竞争
 * 2. 性能必须优于串行解析
 *
 * @param sql_list SQL语句列表
 * @param count SQL语句数量
 * @param parse_trees 解析树输出参数数组
 * @param errors 错误信息输出参数数组
 * @return 成功解析的语句数量
 */
int tecc_perf_parse_parallel(const char **sql_list, int count,
                            Node **parse_trees, ParserError **errors);

/* ==================== 性能优化配置 ==================== */

/*
 * tecc_perf_init_options - 初始化性能优化选项
 *
 * 功能：使用默认值初始化性能优化选项
 * 兼容性要求：
 * 1. 默认值必须与处理器模块的默认行为一致
 * 2. 必须支持处理器模块的选项初始化需求
 *
 * @param options 要初始化的选项结构
 */
void tecc_perf_init_options(TeccPerfOptions *options);

/*
 * tecc_perf_apply_options - 应用性能优化选项
 *
 * 功能：应用性能优化选项到解析器
 * 兼容性要求：
 * 1. 选项应用必须立即生效
 * 2. 必须支持处理器模块的动态配置需求
 *
 * @param options 要应用的性能优化选项
 * @return 是否应用成功
 */
bool tecc_perf_apply_options(const TeccPerfOptions *options);

/*
 * tecc_perf_get_current_options - 获取当前性能优化选项
 *
 * 功能：获取当前生效的性能优化选项
 * 兼容性要求：
 * 1. 获取的选项必须反映当前实际配置
 * 2. 必须支持处理器模块的配置查询需求
 *
 * @param options 选项输出参数
 */
void tecc_perf_get_current_options(TeccPerfOptions *options);

#ifdef __cplusplus
}
#endif

#endif /* TECC_PERF_H */