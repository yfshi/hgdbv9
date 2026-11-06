/*
 * TECC前端解析器性能优化实现
 * 严格按照详细设计文档第4章要求实现
 * 支持处理器模块的性能优化需求
 *
 * 设计原则：
 * 1. 性能优先：所有优化以提升性能为目标
 * 2. 内存高效：最小化内存使用，避免内存泄漏
 * 3. 可配置：支持动态调整优化参数
 * 4. 可监控：提供详细的性能统计信息
 */

/* 定义_POSIX_C_SOURCE以使用clock_gettime */
#define _POSIX_C_SOURCE 200112L

#include "tecc_perf.h"
#include "tecc_nodes.h"
#include "tecc_parser.h"
#include "tecc_error.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#ifdef __linux__
#include <pthread.h>
#include <sys/time.h>
#endif

/* Portability: MSVC does not support TECC_ATTR_UNUSED */
#ifdef _MSC_VER
#define TECC_ATTR_UNUSED
#else
#define TECC_ATTR_UNUSED __attribute__((unused))
#endif

/* ==================== 内部数据结构 ==================== */

/* 缓存条目 */
typedef struct CacheEntry {
    char *sql;                      /* SQL语句 */
    Node *parse_tree;               /* 解析树 */
    ParserError *semantic_errors;   /* 语义错误 */
    uint64_t timestamp;             /* 时间戳 */
    struct CacheEntry *next;        /* 下一个条目 */
    struct CacheEntry *prev;        /* 上一个条目 */
} CacheEntry;

/* 缓存 */
typedef struct Cache {
    CacheEntry *head;               /* 链表头 */
    CacheEntry *tail;               /* 链表尾 */
    size_t size;                    /* 当前大小 */
    size_t max_size;                /* 最大大小 */
    int hits;                       /* 命中次数 */
    int misses;                     /* 未命中次数 */
} Cache;

/* 内存池块 */
typedef struct MemoryBlock {
    void *memory;                   /* 内存块 */
    size_t size;                    /* 块大小 */
    size_t used;                    /* 已使用大小 */
    struct MemoryBlock *next;       /* 下一个块 */
} MemoryBlock;

/* 内存池 */
typedef struct MemoryPool {
    MemoryBlock *blocks;            /* 内存块链表 */
    size_t total_size;              /* 总大小 */
    size_t used_size;               /* 已使用大小 */
    size_t peak_size;               /* 峰值大小 */
} MemoryPool;

/* 并行任务 */
typedef struct ParallelTask {
    const char *sql;                /* SQL语句 */
    Node *parse_tree;               /* 解析树输出 */
    ParserError *errors;            /* 错误输出 */
    bool completed;                 /* 是否完成 */
    bool success;                   /* 是否成功 */
} ParallelTask;

/* ==================== 全局性能状态 ==================== */

static TeccParserStatistics g_perf_stats = {0};
static TeccPerfOptions g_perf_options = {0};
static Cache *g_cache = NULL;
static MemoryPool *g_memory_pool = NULL;
static bool g_parallel_initialized = false;

#ifdef __linux__
static pthread_mutex_t g_cache_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_stats_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

/* ==================== 内部辅助函数 ==================== */

/* 获取当前时间（纳秒） */
static uint64_t get_current_time_ns(void) {
#ifdef __linux__
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000000 + tv.tv_usec * 1000;
#endif
}

/* 计算字符串哈希值 */
TECC_ATTR_UNUSED static uint64_t hash_string(const char *str) {
    uint64_t hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }

    return hash;
}

/* 创建缓存条目 */
static CacheEntry *create_cache_entry(const char *sql, const Node *parse_tree,
                                     const ParserError *semantic_errors) {
    CacheEntry *entry = (CacheEntry *)tecc_palloc(sizeof(CacheEntry));
    if (!entry) return NULL;

    entry->sql = tecc_pstrdup(sql);
    if (!entry->sql) {
        tecc_pfree(entry);
        return NULL;
    }

    /* 复制解析树 */
    entry->parse_tree = tecc_copy_node(parse_tree);
    if (!entry->parse_tree) {
        tecc_pfree(entry->sql);
        tecc_pfree(entry);
        return NULL;
    }

    /* 复制语义错误 */
    entry->semantic_errors = NULL;
    if (semantic_errors) {
        entry->semantic_errors = tecc_error_copy(semantic_errors);
        if (!entry->semantic_errors) {
            tecc_free_node(entry->parse_tree);
            tecc_pfree(entry->sql);
            tecc_pfree(entry);
            return NULL;
        }
    }

    entry->timestamp = get_current_time_ns();
    entry->next = NULL;
    entry->prev = NULL;

    return entry;
}

/* 释放缓存条目 */
static void free_cache_entry(CacheEntry *entry) {
    if (!entry) return;

    if (entry->sql) tecc_pfree(entry->sql);
    if (entry->parse_tree) tecc_free_node(entry->parse_tree);
    if (entry->semantic_errors) tecc_error_free_list(entry->semantic_errors);
    tecc_pfree(entry);
}

/* 从缓存中移除条目 */
static void remove_cache_entry(Cache *cache, CacheEntry *entry) {
    if (!cache || !entry) return;

    /* 更新链表 */
    if (entry->prev) {
        entry->prev->next = entry->next;
    } else {
        cache->head = entry->next;
    }

    if (entry->next) {
        entry->next->prev = entry->prev;
    } else {
        cache->tail = entry->prev;
    }

    /* 更新大小 */
    cache->size--;

    /* 释放条目 */
    free_cache_entry(entry);
}

/* 添加条目到缓存 */
static void add_to_cache(Cache *cache, CacheEntry *entry) {
    if (!cache || !entry) return;

    /* 如果缓存已满，移除最旧的条目 */
    while (cache->size >= cache->max_size && cache->tail) {
        remove_cache_entry(cache, cache->tail);
    }

    /* 添加到链表头部 */
    entry->next = cache->head;
    entry->prev = NULL;

    if (cache->head) {
        cache->head->prev = entry;
    } else {
        cache->tail = entry;
    }

    cache->head = entry;
    cache->size++;
}

/* 在缓存中查找条目 */
static CacheEntry *find_in_cache(Cache *cache, const char *sql) {
    if (!cache || !sql) return NULL;

    {
        CacheEntry *current = cache->head;
        while (current) {
            if (strcmp(current->sql, sql) == 0) {
                /* 移动到链表头部（LRU策略） */
                if (current != cache->head) {
                    /* 从当前位置移除 */
                    if (current->prev) current->prev->next = current->next;
                    if (current->next) current->next->prev = current->prev;
                    if (current == cache->tail) cache->tail = current->prev;

                    /* 添加到头部 */
                    current->next = cache->head;
                    current->prev = NULL;
                    if (cache->head) cache->head->prev = current;
                    cache->head = current;
                    if (!cache->tail) cache->tail = current;
                }
                return current;
            }
            current = current->next;
        }
    }

    return NULL;
}

/* 创建内存块 */
static MemoryBlock *create_memory_block(size_t size) {
    MemoryBlock *block = (MemoryBlock *)malloc(sizeof(MemoryBlock));
    if (!block) return NULL;

    block->memory = malloc(size);
    if (!block->memory) {
        free(block);
        return NULL;
    }

    block->size = size;
    block->used = 0;
    block->next = NULL;

    return block;
}

/* 从内存池分配内存 */
TECC_ATTR_UNUSED static void *memory_pool_alloc(MemoryPool *pool, size_t size) {
    if (!pool || size == 0) return NULL;

    {
        MemoryBlock *block;
        size_t block_size;
        void *ptr;

        /* 对齐到8字节边界 */
        size = (size + 7) & ~7;

        /* 查找有足够空间的内存块 */
        block = pool->blocks;
        while (block) {
            if (block->used + size <= block->size) {
                ptr = (char *)block->memory + block->used;
                block->used += size;
                pool->used_size += size;

                if (pool->used_size > pool->peak_size) {
                    pool->peak_size = pool->used_size;
                }

                return ptr;
            }
            block = block->next;
        }

        /* 创建新的内存块 */
        block_size = (size > 4096) ? size * 2 : 4096;
        block = create_memory_block(block_size);
        if (!block) return NULL;

        block->next = pool->blocks;
        pool->blocks = block;
        pool->total_size += block_size;

        /* 从新块分配 */
        ptr = block->memory;
        block->used = size;
        pool->used_size += size;

        if (pool->used_size > pool->peak_size) {
            pool->peak_size = pool->used_size;
        }

        return ptr;
    }
}

/* 更新性能统计 */
static void update_statistics(uint64_t parse_time, uint64_t semantic_time,
                             uint64_t error_time, size_t memory_used) {
#ifdef __linux__
    pthread_mutex_lock(&g_stats_mutex);
#endif

    g_perf_stats.parse_time_ns += parse_time;
    g_perf_stats.semantic_time_ns += semantic_time;
    g_perf_stats.error_time_ns += error_time;
    g_perf_stats.total_time_ns += parse_time + semantic_time + error_time;

    g_perf_stats.memory_used = memory_used;
    if (memory_used > g_perf_stats.peak_memory) {
        g_perf_stats.peak_memory = memory_used;
    }

    g_perf_stats.parse_calls++;

#ifdef __linux__
    pthread_mutex_unlock(&g_stats_mutex);
#endif
}

/* ==================== 公共接口实现 ==================== */

/* 获取性能统计信息 */
void tecc_perf_get_statistics(TeccParserStatistics *stats) {
    if (!stats) return;

#ifdef __linux__
    pthread_mutex_lock(&g_stats_mutex);
#endif

    *stats = g_perf_stats;

#ifdef __linux__
    pthread_mutex_unlock(&g_stats_mutex);
#endif
}

/* 重置性能统计 */
void tecc_perf_reset_statistics(void) {
#ifdef __linux__
    pthread_mutex_lock(&g_stats_mutex);
#endif

    memset(&g_perf_stats, 0, sizeof(TeccParserStatistics));

#ifdef __linux__
    pthread_mutex_unlock(&g_stats_mutex);
#endif
}

/* 打印性能统计信息 */
void tecc_perf_print_statistics(bool verbose) {
    TeccParserStatistics stats;
    tecc_perf_get_statistics(&stats);

    printf("=== TECC Parser Performance Statistics ===\n");
    printf("Time Statistics:\n");
    printf("  Parse time: %.2f ms\n", stats.parse_time_ns / 1000000.0);
    printf("  Semantic time: %.2f ms\n", stats.semantic_time_ns / 1000000.0);
    printf("  Error time: %.2f ms\n", stats.error_time_ns / 1000000.0);
    printf("  Total time: %.2f ms\n", stats.total_time_ns / 1000000.0);

    printf("\nMemory Statistics:\n");
    printf("  Current memory: %.2f KB\n", stats.memory_used / 1024.0);
    printf("  Peak memory: %.2f KB\n", stats.peak_memory / 1024.0);

    printf("\nCall Statistics:\n");
    printf("  Parse calls: %d\n", stats.parse_calls);
    printf("  Semantic calls: %d\n", stats.semantic_calls);
    printf("  Error calls: %d\n", stats.error_calls);

    if (verbose) {
        printf("\nDetailed Statistics:\n");
        printf("  Nodes created: %d\n", stats.nodes_created);
        printf("  Nodes freed: %d\n", stats.nodes_freed);
        printf("  Strings allocated: %d\n", stats.strings_allocated);
        printf("  Strings freed: %d\n", stats.strings_freed);
        printf("  Cache hits: %d\n", stats.cache_hits);
        printf("  Cache misses: %d\n", stats.cache_misses);
        printf("  Parallel tasks: %d\n", stats.parallel_tasks);
        printf("  Max parallel tasks: %d\n", stats.max_parallel_tasks);
    }

    if (g_cache) {
        printf("\nCache Statistics:\n");
        printf("  Cache size: %zu\n", g_cache->size);
        printf("  Cache hits: %d\n", g_cache->hits);
        printf("  Cache misses: %d\n", g_cache->misses);
        if (g_cache->hits + g_cache->misses > 0) {
            float hit_rate = (float)g_cache->hits / (g_cache->hits + g_cache->misses) * 100;
            printf("  Hit rate: %.1f%%\n", hit_rate);
        }
    }

    printf("==========================================\n");
}

/* 初始化内存池 */
bool tecc_perf_init_memory_pool(size_t pool_size) {
    if (g_memory_pool) {
        tecc_perf_destroy_memory_pool();
    }

    g_memory_pool = (MemoryPool *)malloc(sizeof(MemoryPool));
    if (!g_memory_pool) return false;

    memset(g_memory_pool, 0, sizeof(MemoryPool));

    {
        /* 创建初始内存块 */
        MemoryBlock *block = create_memory_block(pool_size);
        if (!block) {
            free(g_memory_pool);
            g_memory_pool = NULL;
            return false;
        }

        g_memory_pool->blocks = block;
        g_memory_pool->total_size = pool_size;
    }

    /* 更新性能选项 */
    g_perf_options.enable_memory_pool = true;
    g_perf_options.memory_pool_size = pool_size;

    return true;
}

/* 销毁内存池 */
void tecc_perf_destroy_memory_pool(void) {
    if (!g_memory_pool) return;

    {
        /* 释放所有内存块 */
        MemoryBlock *block = g_memory_pool->blocks;
        while (block) {
            MemoryBlock *next = block->next;
            free(block->memory);
            free(block);
            block = next;
        }
    }

    free(g_memory_pool);
    g_memory_pool = NULL;

    /* 更新性能选项 */
    g_perf_options.enable_memory_pool = false;
    g_perf_options.memory_pool_size = 0;
}

/* 获取内存使用情况 */
void tecc_perf_get_memory_usage(size_t *current_usage, size_t *peak_usage) {
    if (current_usage) *current_usage = 0;
    if (peak_usage) *peak_usage = 0;

    if (g_memory_pool) {
        if (current_usage) *current_usage = g_memory_pool->used_size;
        if (peak_usage) *peak_usage = g_memory_pool->peak_size;
    }
}

/* 初始化缓存 */
bool tecc_perf_init_cache(size_t cache_size) {
    if (g_cache) {
        tecc_perf_destroy_cache();
    }

    g_cache = (Cache *)tecc_palloc(sizeof(Cache));
    if (!g_cache) return false;

    memset(g_cache, 0, sizeof(Cache));
    g_cache->max_size = cache_size;

    /* 更新性能选项 */
    g_perf_options.enable_cache = true;
    g_perf_options.cache_size = cache_size;

    return true;
}

/* 销毁缓存 */
void tecc_perf_destroy_cache(void) {
    if (!g_cache) return;

    {
        /* 释放所有缓存条目 */
        CacheEntry *entry = g_cache->head;
        while (entry) {
            CacheEntry *next = entry->next;
            free_cache_entry(entry);
            entry = next;
        }
    }

    tecc_pfree(g_cache);
    g_cache = NULL;

    /* 更新性能选项 */
    g_perf_options.enable_cache = false;
    g_perf_options.cache_size = 0;
}

/* 将解析结果放入缓存 */
bool tecc_perf_cache_put(const char *sql, const Node *parse_tree,
                        const ParserError *semantic_errors) {
    if (!g_cache || !sql || !parse_tree) return false;

#ifdef __linux__
    pthread_mutex_lock(&g_cache_mutex);
#endif

    {
        /* 创建缓存条目 */
        CacheEntry *entry = create_cache_entry(sql, parse_tree, semantic_errors);
        if (!entry) {
#ifdef __linux__
            pthread_mutex_unlock(&g_cache_mutex);
#endif
            return false;
        }

        /* 添加到缓存 */
        add_to_cache(g_cache, entry);
    }

#ifdef __linux__
    pthread_mutex_unlock(&g_cache_mutex);
#endif

    return true;
}

/* 从缓存获取解析结果 */
bool tecc_perf_cache_get(const char *sql, Node **parse_tree,
                        ParserError **semantic_errors) {
    if (!g_cache || !sql || !parse_tree || !semantic_errors) return false;

#ifdef __linux__
    pthread_mutex_lock(&g_cache_mutex);
#endif

    {
        /* 查找缓存条目 */
        CacheEntry *entry = find_in_cache(g_cache, sql);
        if (!entry) {
            g_cache->misses++;
#ifdef __linux__
            pthread_mutex_unlock(&g_cache_mutex);
#endif
            return false;
        }

        /* 复制解析结果 */
        *parse_tree = tecc_copy_node(entry->parse_tree);
        if (!*parse_tree) {
#ifdef __linux__
            pthread_mutex_unlock(&g_cache_mutex);
#endif
            return false;
        }

        if (entry->semantic_errors) {
            *semantic_errors = tecc_error_copy(entry->semantic_errors);
            if (!*semantic_errors) {
                tecc_free_node(*parse_tree);
                *parse_tree = NULL;
#ifdef __linux__
                pthread_mutex_unlock(&g_cache_mutex);
#endif
                return false;
            }
        } else {
            *semantic_errors = NULL;
        }

        g_cache->hits++;
    }

#ifdef __linux__
    pthread_mutex_unlock(&g_cache_mutex);
#endif

    /* 更新性能统计 */
#ifdef __linux__
    pthread_mutex_lock(&g_stats_mutex);
#endif
    g_perf_stats.cache_hits++;
#ifdef __linux__
    pthread_mutex_unlock(&g_stats_mutex);
#endif

    return true;
}

/* 清空缓存 */
void tecc_perf_cache_clear(void) {
    if (!g_cache) return;

#ifdef __linux__
    pthread_mutex_lock(&g_cache_mutex);
#endif

    {
        /* 释放所有缓存条目 */
        CacheEntry *entry = g_cache->head;
        while (entry) {
            CacheEntry *next = entry->next;
            free_cache_entry(entry);
            entry = next;
        }
    }

    g_cache->head = NULL;
    g_cache->tail = NULL;
    g_cache->size = 0;

#ifdef __linux__
    pthread_mutex_unlock(&g_cache_mutex);
#endif
}

/* 获取缓存统计信息 */
void tecc_perf_cache_get_stats(int *hits, int *misses, size_t *size) {
    if (hits) *hits = 0;
    if (misses) *misses = 0;
    if (size) *size = 0;

    if (!g_cache) return;

#ifdef __linux__
    pthread_mutex_lock(&g_cache_mutex);
#endif

    if (hits) *hits = g_cache->hits;
    if (misses) *misses = g_cache->misses;
    if (size) *size = g_cache->size;

#ifdef __linux__
    pthread_mutex_unlock(&g_cache_mutex);
#endif
}

/* 初始化并行解析 */
bool tecc_perf_init_parallel(int max_tasks) {
    if (max_tasks <= 0) return false;

#ifdef __linux__
    /* 检查是否支持线程 */
    if (pthread_mutex_init(&g_cache_mutex, NULL) != 0 ||
        pthread_mutex_init(&g_stats_mutex, NULL) != 0) {
        return false;
    }
#endif

    g_parallel_initialized = true;

    /* 更新性能选项 */
    g_perf_options.enable_parallel_parsing = true;
    g_perf_options.max_parallel_tasks = max_tasks;

    return true;
}

/* 销毁并行解析环境 */
void tecc_perf_destroy_parallel(void) {
#ifdef __linux__
    pthread_mutex_destroy(&g_cache_mutex);
    pthread_mutex_destroy(&g_stats_mutex);
#endif

    g_parallel_initialized = false;

    /* 更新性能选项 */
    g_perf_options.enable_parallel_parsing = false;
    g_perf_options.max_parallel_tasks = 0;
}

/* 并行解析SQL语句 */
int tecc_perf_parse_parallel(const char **sql_list, int count,
                            Node **parse_trees, ParserError **errors) {
    if (!sql_list || count <= 0 || !parse_trees || !errors) return 0;

    if (!g_parallel_initialized) {
        /* 串行解析 */
        int success_count = 0;
        for (int i = 0; i < count; i++) {
            parse_trees[i] = tecc_parse_sql(sql_list[i], &errors[i]);
            if (parse_trees[i]) {
                success_count++;
            }
        }
        return success_count;
    }

#ifdef __linux__
    /* 简单并行实现：使用多个线程 */
    {
        pthread_t *threads = (pthread_t *)malloc(count * sizeof(pthread_t));
        ParallelTask *tasks = (ParallelTask *)malloc(count * sizeof(ParallelTask));
        int active_threads = 0;
        int success_count = 0;

        /* 线程函数 */
        void *parse_thread(void *arg) {
            ParallelTask *task = (ParallelTask *)arg;

            uint64_t start_time = get_current_time_ns();
            task->parse_tree = tecc_parse_sql(task->sql, &task->errors);
            {
                uint64_t end_time = get_current_time_ns();

                task->success = (task->parse_tree != NULL);
                task->completed = true;

                /* 更新统计 */
                update_statistics(end_time - start_time, 0, 0, 0);
            }

            return NULL;
        }

        if (!threads || !tasks) {
            if (threads) free(threads);
            if (tasks) free(tasks);
            return 0;
        }

        /* 初始化任务 */
        for (int i = 0; i < count; i++) {
            tasks[i].sql = sql_list[i];
            tasks[i].parse_tree = NULL;
            tasks[i].errors = NULL;
            tasks[i].completed = false;
            tasks[i].success = false;
        }

        /* 创建线程 */
        for (int i = 0; i < count; i++) {
            if (pthread_create(&threads[i], NULL, parse_thread, &tasks[i]) == 0) {
                active_threads++;
            }
        }

        /* 等待所有线程完成 */
        for (int i = 0; i < active_threads; i++) {
            pthread_join(threads[i], NULL);
        }

        /* 收集结果 */
        for (int i = 0; i < count; i++) {
            parse_trees[i] = tasks[i].parse_tree;
            errors[i] = tasks[i].errors;
            if (tasks[i].success) {
                success_count++;
            }
        }

        /* 更新并行统计 */
#ifdef __linux__
        pthread_mutex_lock(&g_stats_mutex);
#endif
        g_perf_stats.parallel_tasks += count;
        if (active_threads > g_perf_stats.max_parallel_tasks) {
            g_perf_stats.max_parallel_tasks = active_threads;
        }
#ifdef __linux__
        pthread_mutex_unlock(&g_stats_mutex);
#endif

        free(threads);
        free(tasks);

        return success_count;
    }
#else
    /* 非Linux平台使用串行解析 */
    int success_count = 0;
    for (int i = 0; i < count; i++) {
        parse_trees[i] = tecc_parse_sql(sql_list[i], &errors[i]);
        if (parse_trees[i]) {
            success_count++;
        }
    }
    return success_count;
#endif
}

/* 初始化性能优化选项 */
void tecc_perf_init_options(TeccPerfOptions *options) {
    if (!options) return;

    memset(options, 0, sizeof(TeccPerfOptions));

    /* 默认值 */
    options->enable_memory_pool = false;
    options->enable_cache = true;
    options->enable_parallel_parsing = false;
    options->enable_statistics = true;

    options->memory_pool_size = 1024 * 1024;  /* 1MB */
    options->cache_size = 100;                /* 100个条目 */
    options->max_parallel_tasks = 4;

    options->parse_timeout_ms = 5000;         /* 5秒 */
    options->semantic_timeout_ms = 1000;      /* 1秒 */

    options->aggressive_optimization = false;
    options->profile_mode = false;
}

/* 应用性能优化选项 */
bool tecc_perf_apply_options(const TeccPerfOptions *options) {
    if (!options) return false;

    /* 保存选项 */
    g_perf_options = *options;

    /* 应用内存池设置 */
    if (options->enable_memory_pool) {
        if (!tecc_perf_init_memory_pool(options->memory_pool_size)) {
            return false;
        }
    } else {
        tecc_perf_destroy_memory_pool();
    }

    /* 应用缓存设置 */
    if (options->enable_cache) {
        if (!tecc_perf_init_cache(options->cache_size)) {
            return false;
        }
    } else {
        tecc_perf_destroy_cache();
    }

    /* 应用并行解析设置 */
    if (options->enable_parallel_parsing) {
        if (!tecc_perf_init_parallel(options->max_parallel_tasks)) {
            return false;
        }
    } else {
        tecc_perf_destroy_parallel();
    }

    /* 应用统计设置 */
    if (options->enable_statistics) {
        tecc_perf_reset_statistics();
    }

    return true;
}

/* 获取当前性能优化选项 */
void tecc_perf_get_current_options(TeccPerfOptions *options) {
    if (!options) return;

    *options = g_perf_options;
}