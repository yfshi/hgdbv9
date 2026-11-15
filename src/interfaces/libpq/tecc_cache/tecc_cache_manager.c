/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_cache_manager.c
 * Abstract:
 *   TECC缓存管理器核心实现，基于简化设计的两层架构。
 *   提供缓存管理器的初始化和基础管理功能。
 *
 * Authored by <wanghaoyan>, 2026-04-15.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_cache/tecc_cache_manager.c
 */

#include "tecc_metadata_cache.h"
#include "tecc_data_fetcher_adapter.h"
#include "../tecc_libpq_integration/fe-tecc-integration.h"  /* 添加tecc_get_state函数声明 */
#include "../tecc_data_fetcher/include/tecc_data_fetcher.h"  /* 添加tecc_data_fetcher_get_connection函数声明 */
#include "common/fe_memutils.h"  /* PostgreSQL前端内存管理函数 */
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
/*
 * Secure random generation.
 *
 * On Linux: use getrandom(2) to avoid the OpenSSL 3.0 CRYPTO_THREAD_write_lock
 * self-deadlock observed with RAND_bytes (100% CPU spin).
 * On Windows: use OpenSSL RAND_bytes (the deadlock is Linux-specific).
 */
#ifndef _WIN32
#if defined(HAVE_GETRANDOM) || defined(__GLIBC__) && __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 25
#include <sys/random.h>
#else
#include <sys/syscall.h>
#define getrandom(buf, len, flags)  syscall(SYS_getrandom, buf, len, flags)
#endif
#endif /* !_WIN32 */
#include <ctype.h>   /* 用于tolower函数（大小写不敏感哈希） */
#include <openssl/hmac.h>  /* HMAC-SHA256支持 */
#ifdef _WIN32
#include <openssl/rand.h>  /* RAND_bytes用于Windows */
#endif

/* 线程局部存储宏定义
 * 在支持__thread的编译器（GCC/Clang）中使用__thread
 * TODO: 添加Windows平台支持（使用__declspec(thread)或TLS API）
 */
#if defined(__GNUC__) || defined(__clang__)
#define TECC_THREAD_LOCAL __thread
#else
/* 其他平台暂时使用普通静态变量，但这不是线程安全的
 * 这只是一个临时解决方案，需要根据具体平台实现
 */
#define TECC_THREAD_LOCAL
#pragma message("Warning: Using non-thread-safe storage for thread-local variables. Please implement proper thread-local storage for this platform.")
#endif


/* 哈希函数声明 */
static uint32 hash_kek_key(const void *key, Size keysize);
static uint32 hash_dek_key(const void *key, Size keysize);
static uint32 hash_column_key(const void *key, Size keysize);
static int compare_kek_keys(const void *key1, const void *key2, Size keysize);
static int compare_dek_keys(const void *key1, const void *key2, Size keysize);
static int compare_column_keys(const void *key1, const void *key2, Size keysize);

/* 名称查找哈希函数声明 */
static uint32 hash_kek_name_key(const void *key, Size keysize);
static uint32 hash_dek_name_key(const void *key, Size keysize);
static uint32 hash_column_name_key(const void *key, Size keysize);
static int compare_kek_name_keys(const void *key1, const void *key2, Size keysize);
static int compare_dek_name_keys(const void *key1, const void *key2, Size keysize);
static int compare_column_name_keys(const void *key1, const void *key2, Size keysize);

/* 内部函数声明 */
static TeccCacheManager *create_cache_manager(PGconn *conn);
static void destroy_cache_manager(TeccCacheManager *manager, bool cleanup_already_done);
static TeccCacheManager *get_cache_manager_for_connection(PGconn *conn);
static int tecc_cache_load_all_keys(TeccState *state);

/* 重试机制相关函数声明 */
static int refresh_cache_with_retry(const TeccState *state, void *arg);

/* 重试操作参数结构 */
typedef struct {
    PGconn *conn;
    TeccCacheRefreshType refresh_type;
} RefreshCacheArg;

/* 异常状态恢复相关函数声明 */
static bool validate_cache_manager_state(TeccCacheManager *manager);
static int recover_cache_manager_state(TeccCacheManager *manager);

/* 哈希表管理函数 */
static FeHashTable *create_kek_hash_table(void);
static FeHashTable *create_dek_hash_table(void);
static FeHashTable *create_column_hash_table(void);
static FeHashTable *create_kek_name_hash_table(void);
static FeHashTable *create_dek_name_hash_table(void);
static FeHashTable *create_column_name_hash_table(void);
static FeHashTable *create_table_schema_hash_table(void);
static void clear_hash_table_data_pointers(FeHashTable *htab);
static void destroy_hash_table(FeHashTable *hash_table);
static void destroy_all_kek_items(FeHashTable *htab);
static void destroy_all_dek_items(FeHashTable *htab);
static void destroy_all_column_items(FeHashTable *htab);

/* 基础内存管理函数 */
static void destroy_kek_cache_item(TeccKekCacheItem *item);
static void destroy_dek_cache_item(TeccDekCacheItem *item);
static void destroy_column_cache_item(TeccColumnCacheItem *item);

/* 缓存刷新辅助函数 */
static int mark_kek_cache_for_refresh(TeccCacheManager *manager);
static int mark_dek_cache_for_refresh(TeccCacheManager *manager);
static int refresh_guc_params(TeccCacheManager *manager);

/* 定时刷新检查函数 */
static bool check_timed_refresh_needed(TeccCacheManager *manager);

/* 缓存清理辅助函数 */
int tecc_cache_clear_specific(TeccCacheManager *manager, TeccCacheRefreshType cache_type);

/* 全局缓存管理器已移除，改为连接特定存储 */

/*
 * tecc_cache_init - 初始化缓存模块
 *
 * 参数:
 *   state - Tecc状态
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
int
tecc_cache_init(TeccState *state)
{
    TeccCacheManager *manager = NULL;
    /* 全局缓存管理器已移除，改为连接特定存储 */

    /* 参数检查 */
    if (state == NULL || state->conn == NULL) {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    /* 检查是否已经初始化 */
    if (state->metadata_cache != NULL) {
        manager = (TeccCacheManager *)state->metadata_cache;
        if (manager->initialized) {
            TECC_CACHE_DEBUG("缓存已经初始化，跳过重复初始化");
            return TECC_CACHE_OK;
        }
    }

    /* 日志系统已集成到通用日志系统中，无需单独初始化 */

    /* 1. 创建缓存管理器 */
    manager = create_cache_manager(state->conn);
    if (manager == NULL) {
        TECC_CACHE_ERROR("缓存初始化失败：无法创建缓存管理器（内存不足）");
        return TECC_CACHE_ERROR_MEMORY;
    }

    /* 2. 初始化锁机制 */
    /* 初始化互斥锁（兼容性保留） */
    manager->lock = pg_malloc(sizeof(pthread_mutex_t));
    if (manager->lock == NULL) {
        TECC_CACHE_ERROR("缓存初始化失败：无法分配互斥锁内存");
        destroy_cache_manager(manager, false);
        return TECC_CACHE_ERROR_MEMORY;
    }

    if (pthread_mutex_init((pthread_mutex_t *)manager->lock, NULL) != 0) {
        TECC_CACHE_ERROR("缓存初始化失败：互斥锁初始化错误");
        pg_free(manager->lock);
        destroy_cache_manager(manager, false);
        return TECC_CACHE_ERROR_LOCK;
    }

    /* 初始化读写锁 */
    if (pthread_rwlock_init(&manager->global_rwlock, NULL) != 0) {
        TECC_CACHE_ERROR("缓存初始化失败：读写锁初始化错误");
        pthread_mutex_destroy((pthread_mutex_t *)manager->lock);
        pg_free(manager->lock);
        destroy_cache_manager(manager, false);
        return TECC_CACHE_ERROR_LOCK;
    }

    /* 3. 设置连接关联 */
    /* 连接已经存储在manager->conn字段中 */

    /* 4. 初始化数据获取器适配器（通过TeccState） */
    /* 注意：数据获取器适配器通过TeccState参数访问，这里不需要额外初始化 */

    /* 5. 初始化缓存哈希表 */
    /* 创建OID查找哈希表 */
    manager->kek_cache = create_kek_hash_table();
    manager->dek_cache = create_dek_hash_table();
    manager->column_cache = create_column_hash_table();

    if (manager->kek_cache == NULL || manager->dek_cache == NULL || manager->column_cache == NULL) {
        if (manager->kek_cache != NULL) {
            destroy_hash_table(manager->kek_cache);
            manager->kek_cache = NULL;
        }
        if (manager->dek_cache != NULL) {
            destroy_hash_table(manager->dek_cache);
            manager->dek_cache = NULL;
        }
        if (manager->column_cache != NULL) {
            destroy_hash_table(manager->column_cache);
            manager->column_cache = NULL;
        }
        pthread_rwlock_destroy(&manager->global_rwlock);
        pthread_mutex_destroy((pthread_mutex_t *)manager->lock);
        pg_free(manager->lock);
        destroy_cache_manager(manager, false);
        return TECC_CACHE_ERROR_MEMORY;
    }

    /* 6. 初始化名称查找哈希表 */
    /* 创建名称查找哈希表 */
    manager->kek_name_cache = create_kek_name_hash_table();
    manager->dek_name_cache = create_dek_name_hash_table();
    manager->column_name_cache = create_column_name_hash_table();
    manager->table_schema_cache = create_table_schema_hash_table();

    if (manager->kek_name_cache == NULL || manager->dek_name_cache == NULL || manager->column_name_cache == NULL) {
        if (manager->kek_name_cache != NULL) {
            destroy_hash_table(manager->kek_name_cache);
            manager->kek_name_cache = NULL;
        }
        if (manager->dek_name_cache != NULL) {
            destroy_hash_table(manager->dek_name_cache);
            manager->dek_name_cache = NULL;
        }
        if (manager->column_name_cache != NULL) {
            destroy_hash_table(manager->column_name_cache);
            manager->column_name_cache = NULL;
        }
        if (manager->table_schema_cache != NULL) {
            destroy_hash_table(manager->table_schema_cache);
            manager->table_schema_cache = NULL;
        }
        if (manager->kek_cache != NULL) {
            destroy_hash_table(manager->kek_cache);
            manager->kek_cache = NULL;
        }
        if (manager->dek_cache != NULL) {
            destroy_hash_table(manager->dek_cache);
            manager->dek_cache = NULL;
        }
        if (manager->column_cache != NULL) {
            destroy_hash_table(manager->column_cache);
            manager->column_cache = NULL;
        }
        pthread_rwlock_destroy(&manager->global_rwlock);
        pthread_mutex_destroy((pthread_mutex_t *)manager->lock);
        pg_free(manager->lock);
        destroy_cache_manager(manager, false);
        return TECC_CACHE_ERROR_MEMORY;
    }

    /* 6. 将管理器与连接关联（在加载GUC参数之前） */
    /* 必须将管理器存储在TeccState结构中以确保线程安全 */
    /* 将缓存管理器存储在TeccState的metadata_cache字段中 */
    state->metadata_cache = manager;

    /* 7. 加载GUC参数 */
    /* 注意：这里传递state参数给tecc_cache_load_guc_params */
    /* GUC参数加载失败，但不影响缓存基本功能 */
    /* 可以继续初始化，GUC参数将在需要时重新尝试加载 */
    /* 日志级别通过环境变量TECC_LOG_LEVEL控制，无需手动设置 */
    (void)tecc_cache_load_guc_params(state);

    /* 8. 设置状态标志（必须在批量加载之前，否则insert会被拒绝） */
    manager->initialized = true;

    /* 7.5 批量加载KEK/DEK缓存（warming） */
    /* 使用数据获取器批量接口加载全部密钥元数据，减少后续逐条DB查询 */
    (void)tecc_cache_load_all_keys(state);

    TECC_CACHE_INFO("缓存模块初始化成功，连接: %p", state->conn);

    return TECC_CACHE_OK;
}

/*
 * tecc_cache_cleanup - 清理缓存模块
 *
 * 参数:
 *   state - Tecc状态
 */
void
tecc_cache_cleanup(TeccState *state)
{
    TeccCacheManager *manager = NULL;

    /* 参数检查 */
    if (state == NULL) {
        return;
    }

    TECC_CACHE_DEBUG("开始清理缓存模块，连接: %p", state->conn);

    /* 1. 获取缓存管理器 */
    /* 直接从TeccState获取缓存管理器 */
    if (state->metadata_cache != NULL) {
        manager = (TeccCacheManager *)state->metadata_cache;
    } else {
        /* 缓存管理器不存在，直接返回 */
        return;
    }

    if (manager == NULL || manager->conn != state->conn) {
        /* 缓存管理器不存在或连接不匹配 */
        return;
    }

    /* 2. 清理所有缓存——先释放item对象，再销毁hash表 */
    /* KEK items同时存在于kek_cache和kek_name_cache，只从主表释放一次 */
    if (manager->kek_cache != NULL) {
        destroy_all_kek_items(manager->kek_cache);
        destroy_hash_table(manager->kek_cache);
        manager->kek_cache = NULL;
    }

    /* DEK items同时存在于dek_cache和dek_name_cache，只从主表释放一次 */
    if (manager->dek_cache != NULL) {
        destroy_all_dek_items(manager->dek_cache);
        destroy_hash_table(manager->dek_cache);
        manager->dek_cache = NULL;
    }

    /* Column items同时存在于column_cache和column_name_cache，只从主表释放一次 */
    if (manager->column_cache != NULL) {
        destroy_all_column_items(manager->column_cache);
        destroy_hash_table(manager->column_cache);
        manager->column_cache = NULL;
    }

    /* 清理名称查找哈希表 — item已由destroy_all_*_items释放，先清空悬空data指针 */
    if (manager->kek_name_cache != NULL) {
        clear_hash_table_data_pointers(manager->kek_name_cache);
        destroy_hash_table(manager->kek_name_cache);
        manager->kek_name_cache = NULL;
    }
    if (manager->dek_name_cache != NULL) {
        clear_hash_table_data_pointers(manager->dek_name_cache);
        destroy_hash_table(manager->dek_name_cache);
        manager->dek_name_cache = NULL;
    }
    if (manager->column_name_cache != NULL) {
        clear_hash_table_data_pointers(manager->column_name_cache);
        destroy_hash_table(manager->column_name_cache);
        manager->column_name_cache = NULL;
    }
    if (manager->table_schema_cache != NULL) {
        destroy_hash_table(manager->table_schema_cache);
        manager->table_schema_cache = NULL;
    }

    /* 3. 销毁锁 */
    /* 销毁读写锁 */
    pthread_rwlock_destroy(&manager->global_rwlock);

    /* 销毁互斥锁（兼容性保留） */
    if (manager->lock != NULL) {
        pthread_mutex_destroy((pthread_mutex_t *)manager->lock);
        pg_free(manager->lock);
        manager->lock = NULL;
    }

    /* 4. 清理数据获取器适配器关联 */
    /* 注意：数据获取器适配器通过TeccState参数访问，这里不需要额外清理 */

    /* 5. 释放管理器内存 */
    destroy_cache_manager(manager, true);

    /* 6. 清除引用 */
    /* 清除TeccState中的引用 */
    if (state != NULL && state->metadata_cache == manager) {
        state->metadata_cache = NULL;
    }

    /* 全局缓存管理器已移除，无需清理全局引用 */

    TECC_CACHE_DEBUG("缓存模块清理完成，连接: %p", state->conn);
}

/*
 * tecc_cache_refresh - 刷新元数据缓存
 *
 * 参数:
 *   state - Tecc状态
 *   refresh_type - 刷新类型
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
/*
 * refresh_cache_with_retry - 带重试的缓存刷新操作（用于重试机制）
 *
 * 参数:
 *   state - Tecc状态
 *   arg - 操作参数（RefreshCacheArg指针）
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
static int
refresh_cache_with_retry(const TeccState *state, void *arg)
{
    RefreshCacheArg *refresh_arg = (RefreshCacheArg *)arg;
    TeccCacheRefreshType refresh_type;
    TeccCacheManager *manager = NULL;

    refresh_type = refresh_arg->refresh_type;

    if (state == NULL || state->conn == NULL) {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    /* 获取缓存管理器 */
    manager = get_cache_manager(state);
    if (manager == NULL) {
        return TECC_CACHE_ERROR_NOT_INITIALIZED;
    }

    /* 根据refresh_type刷新指定类型的缓存。
     * ReqID:TECC4.0 - 使用直接清空代替懒标记，确保缓存立即刷新而非延迟到下次访问。 */
    switch (refresh_type) {
        case TECC_CACHE_REFRESH_ALL:
            /* 立即清空所有缓存，下次访问时从数据库重新加载 */
            if (tecc_cache_clear_specific(manager, TECC_CACHE_REFRESH_KEK) != TECC_CACHE_OK) {
                return TECC_CACHE_ERROR_REFRESH;
            }
            if (tecc_cache_clear_specific(manager, TECC_CACHE_REFRESH_DEK) != TECC_CACHE_OK) {
                return TECC_CACHE_ERROR_REFRESH;
            }
            if (tecc_cache_clear_specific(manager, TECC_CACHE_REFRESH_COLUMN) != TECC_CACHE_OK) {
                return TECC_CACHE_ERROR_REFRESH;
            }
            if (refresh_guc_params(manager) != TECC_CACHE_OK) {
                return TECC_CACHE_ERROR_REFRESH;
            }

            /* 重置定时刷新计时器，避免重复刷新 */
            CACHE_WLOCK(manager);
            manager->last_timed_refresh_time = time(NULL);
            CACHE_RWUNLOCK(manager);
            TECC_CACHE_DEBUG("刷新所有缓存完成（立即清空模式）");
            break;

        case TECC_CACHE_REFRESH_KEK:
            /* 立即清空KEK缓存 */
            if (tecc_cache_clear_specific(manager, TECC_CACHE_REFRESH_KEK) != TECC_CACHE_OK) {
                return TECC_CACHE_ERROR_REFRESH;
            }

            /* 重置定时刷新计时器，避免重复刷新 */
            CACHE_WLOCK(manager);
            manager->last_timed_refresh_time = time(NULL);
            CACHE_RWUNLOCK(manager);
            TECC_CACHE_DEBUG("刷新KEK缓存完成（立即清空模式）");
            break;

        case TECC_CACHE_REFRESH_DEK:
            /* 立即清空DEK缓存 */
            if (tecc_cache_clear_specific(manager, TECC_CACHE_REFRESH_DEK) != TECC_CACHE_OK) {
                return TECC_CACHE_ERROR_REFRESH;
            }

            /* 重置定时刷新计时器，避免重复刷新 */
            CACHE_WLOCK(manager);
            manager->last_timed_refresh_time = time(NULL);
            CACHE_RWUNLOCK(manager);
            TECC_CACHE_DEBUG("刷新DEK缓存完成（立即清空模式）");
            break;

        case TECC_CACHE_REFRESH_COLUMN:
            /* 立即清空加密列缓存 */
            if (tecc_cache_clear_specific(manager, TECC_CACHE_REFRESH_COLUMN) != TECC_CACHE_OK) {
                return TECC_CACHE_ERROR_REFRESH;
            }

            /* 重置定时刷新计时器，避免重复刷新 */
            CACHE_WLOCK(manager);
            manager->last_timed_refresh_time = time(NULL);
            CACHE_RWUNLOCK(manager);
            TECC_CACHE_DEBUG("刷新加密列缓存完成（立即清空模式）");
            break;

        case TECC_CACHE_REFRESH_GUC:
            /* 只刷新GUC参数缓存 */
            if (refresh_guc_params(manager) != TECC_CACHE_OK) {
                return TECC_CACHE_ERROR_REFRESH;
            }

            /* 重置定时刷新计时器，避免重复刷新 */
            CACHE_WLOCK(manager);
            manager->last_timed_refresh_time = time(NULL);
            CACHE_RWUNLOCK(manager);
            TECC_CACHE_DEBUG("显式刷新GUC参数缓存，已重置定时刷新计时器");
            break;

        default:
            return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    /* 更新最后同步时间 */
    manager->last_sync_time = time(NULL);

    return TECC_CACHE_OK;
}

int
tecc_cache_refresh(const TeccState *state, TeccCacheRefreshType refresh_type)
{
    RefreshCacheArg refresh_arg;
    int result;
    const char *refresh_type_str = NULL;

    /* 参数检查 */
    if (state == NULL) {
        TECC_CACHE_ERROR("显式调用刷新缓存失败：状态参数为NULL");
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    if (state->conn == NULL) {
        TECC_CACHE_ERROR("显式调用刷新缓存失败：数据库连接为NULL");
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    /* 验证刷新类型 */
    if (refresh_type < TECC_CACHE_REFRESH_ALL || refresh_type > TECC_CACHE_REFRESH_GUC) {
        TECC_CACHE_ERROR("显式调用刷新缓存失败：无效的刷新类型: %d", refresh_type);
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    /* 获取刷新类型字符串表示 */
    switch (refresh_type) {
        case TECC_CACHE_REFRESH_ALL:
            refresh_type_str = "ALL";
            break;
        case TECC_CACHE_REFRESH_KEK:
            refresh_type_str = "KEK";
            break;
        case TECC_CACHE_REFRESH_DEK:
            refresh_type_str = "DEK";
            break;
        case TECC_CACHE_REFRESH_COLUMN:
            refresh_type_str = "COLUMN";
            break;
        case TECC_CACHE_REFRESH_GUC:
            refresh_type_str = "GUC";
            break;
        default:
            refresh_type_str = "UNKNOWN";
            break;
    }

    TECC_CACHE_DEBUG("开始显式调用刷新缓存，刷新类型: %s (%d)", refresh_type_str, refresh_type);

    /* 准备刷新参数 */
    refresh_arg.conn = state->conn;
    refresh_arg.refresh_type = refresh_type;

    /* 使用重试机制执行缓存刷新 */
    TECC_CACHE_DEBUG("【DEBUG】tecc_cache_refresh: calling tecc_cache_retry_with_recovery, refresh_type=%d", refresh_type);
    result = tecc_cache_retry_with_recovery(state, refresh_cache_with_retry, &refresh_arg, NULL);
    TECC_CACHE_DEBUG("【DEBUG】tecc_cache_refresh: tecc_cache_retry_with_recovery returned %d", result);

    if (result == TECC_CACHE_OK) {
        TECC_CACHE_INFO("显式调用刷新缓存成功，刷新类型: %s (%d)", refresh_type_str, refresh_type);
    } else {
        TECC_CACHE_ERROR("显式调用刷新缓存失败，刷新类型: %s (%d)，错误码: %d",
                        refresh_type_str, refresh_type, result);
    }

    return result;
}

/*
 * tecc_cache_set_timed_refresh_interval - 设置定时刷新间隔
 *
 * 参数:
 *   state - Tecc状态
 *   interval_sec - 定时刷新间隔（秒），0表示禁用
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
int
tecc_cache_set_timed_refresh_interval(TeccState *state, int interval_sec)
{
    TeccCacheManager *manager;

    if (state == NULL) {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    if (interval_sec < 0) {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    manager = get_cache_manager(state);
    if (manager == NULL || !manager->initialized) {
        return TECC_CACHE_ERROR_NOT_INITIALIZED;
    }

    CACHE_WLOCK(manager);
    manager->timed_refresh_interval_sec = interval_sec;
    manager->timed_refresh_enabled = (interval_sec > 0);
    if (interval_sec > 0) {
        manager->last_timed_refresh_time = time(NULL);
    }
    CACHE_RWUNLOCK(manager);

    TECC_CACHE_DEBUG("定时刷新间隔设置为：%d秒", interval_sec);
    return TECC_CACHE_OK;
}

/*
 * tecc_cache_enable_timed_refresh - 启用/禁用定时刷新
 *
 * 参数:
 *   state - Tecc状态
 *   enable - true启用，false禁用
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
int
tecc_cache_enable_timed_refresh(TeccState *state, bool enable)
{
    TeccCacheManager *manager;

    if (state == NULL) {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    manager = get_cache_manager(state);
    if (manager == NULL || !manager->initialized) {
        return TECC_CACHE_ERROR_NOT_INITIALIZED;
    }

    CACHE_WLOCK(manager);
    manager->timed_refresh_enabled = enable;
    if (enable && manager->timed_refresh_interval_sec <= 0) {
        /* 如果启用但间隔未设置，使用默认值（300秒） */
        manager->timed_refresh_interval_sec = 300;
        manager->last_timed_refresh_time = time(NULL);
    }
    CACHE_RWUNLOCK(manager);

    TECC_CACHE_DEBUG("定时刷新%s", enable ? "已启用" : "已禁用");
    return TECC_CACHE_OK;
}

/*
 * tecc_cache_get_timed_refresh_status - 获取定时刷新状态
 *
 * 参数:
 *   state - Tecc状态
 *   enabled - 返回定时刷新是否启用
 *   interval_sec - 返回定时刷新间隔（秒）
 *   last_refresh_time - 返回上次定时刷新时间
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
int
tecc_cache_get_timed_refresh_status(TeccState *state,
                                    bool *enabled,
                                    int *interval_sec,
                                    time_t *last_refresh_time)
{
    TeccCacheManager *manager;

    if (state == NULL || enabled == NULL || interval_sec == NULL || last_refresh_time == NULL) {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    manager = get_cache_manager(state);
    if (manager == NULL || !manager->initialized) {
        return TECC_CACHE_ERROR_NOT_INITIALIZED;
    }

    CACHE_RLOCK(manager);
    *enabled = manager->timed_refresh_enabled;
    *interval_sec = manager->timed_refresh_interval_sec;
    *last_refresh_time = manager->last_timed_refresh_time;
    CACHE_RWUNLOCK(manager);

    return TECC_CACHE_OK;
}

/*
 * tecc_cache_clear - 清理所有缓存
 *
 * 参数:
 *   state - Tecc状态
 */
void
tecc_cache_clear(const TeccState *state)
{
    TeccCacheManager *manager = NULL;
	TECC_LOG(TECC_LOG_INFO, "CACHE_CLEAR: called (full cache wipe)");

    /* 参数检查 */
    if (state == NULL) {
        return;
    }

    /* 获取缓存管理器 */
    manager = get_cache_manager(state);
    if (manager == NULL) {
        return;  /* 缓存管理器未初始化 */
    }

    /* 加锁保护缓存清理操作 */
    CACHE_WLOCK(manager);

    /* 清理所有缓存数据 */
    /* 清理KEK缓存 — 先安全释放item并清空哈希条目data指针，再销毁哈希表 */
    if (manager->kek_cache != NULL) {
        destroy_all_kek_items(manager->kek_cache);
        destroy_hash_table(manager->kek_cache);
        manager->kek_cache = create_kek_hash_table();
        if (manager->kek_cache == NULL) {
            manager->kek_cache = NULL;
        }
    }

    /* 清理DEK缓存 */
    if (manager->dek_cache != NULL) {
        destroy_all_dek_items(manager->dek_cache);
        destroy_hash_table(manager->dek_cache);
        manager->dek_cache = create_dek_hash_table();
        if (manager->dek_cache == NULL) {
            manager->dek_cache = NULL;
        }
    }

    /* 清理加密列缓存 */
    if (manager->column_cache != NULL) {
        destroy_all_column_items(manager->column_cache);
        destroy_hash_table(manager->column_cache);
        manager->column_cache = create_column_hash_table();
        if (manager->column_cache == NULL) {
            manager->column_cache = NULL;
        }
    }

    /* 清理名称查找哈希表 — item已由destroy_all_*_items释放，先清空悬空data指针 */
    if (manager->kek_name_cache != NULL) {
        clear_hash_table_data_pointers(manager->kek_name_cache);
        destroy_hash_table(manager->kek_name_cache);
        manager->kek_name_cache = create_kek_name_hash_table();
        if (manager->kek_name_cache == NULL) {
            manager->kek_name_cache = NULL;
        }
    }
    if (manager->dek_name_cache != NULL) {
        clear_hash_table_data_pointers(manager->dek_name_cache);
        destroy_hash_table(manager->dek_name_cache);
        manager->dek_name_cache = create_dek_name_hash_table();
        if (manager->dek_name_cache == NULL) {
            manager->dek_name_cache = NULL;
        }
    }
    if (manager->column_name_cache != NULL) {
        clear_hash_table_data_pointers(manager->column_name_cache);
        destroy_hash_table(manager->column_name_cache);
        manager->column_name_cache = create_column_name_hash_table();
        if (manager->column_name_cache == NULL) {
            manager->column_name_cache = NULL;
        }
    }
    if (manager->table_schema_cache != NULL) {
        destroy_hash_table(manager->table_schema_cache);
        manager->table_schema_cache = create_table_schema_hash_table();
        if (manager->table_schema_cache == NULL) {
            manager->table_schema_cache = NULL;
        }
    }

    /* 重置缓存大小和统计信息 */
    manager->current_size = 0;
    manager->load_count = 0;
    manager->last_sync_time = 0;
    manager->cache_hits = 0;
    manager->cache_misses = 0;
    manager->name_cache_hits = 0;
    manager->name_cache_misses = 0;

	/* ReqID:TECC4.0 - 重置延迟刷新标记，缓存已清空无需再刷新 */
	manager->kek_cache_needs_refresh = false;
	manager->dek_cache_needs_refresh = false;
	manager->column_cache_needs_refresh = false;

    /* GUC params are session-level and never change — keep them loaded
     * so the next access does not need to reload via temp connection. */
    manager->guc_loaded = true;

    /* 解锁 */
    CACHE_RWUNLOCK(manager);
}

/*
 * tecc_cache_clear_specific - 清理特定类型的缓存
 *
 * 参数:
 *   manager - 缓存管理器
 *   cache_type - 缓存类型
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
int
tecc_cache_clear_specific(TeccCacheManager *manager, TeccCacheRefreshType cache_type)
{
    if (manager == NULL || !manager->initialized) {
        return TECC_CACHE_ERROR_NOT_INITIALIZED;
    }

    CACHE_WLOCK(manager);

    switch (cache_type) {
        case TECC_CACHE_REFRESH_KEK:
            /* 清理KEK缓存 */
            if (manager->kek_cache != NULL) {
                destroy_all_kek_items(manager->kek_cache);
                destroy_hash_table(manager->kek_cache);
                manager->kek_cache = create_kek_hash_table();
                if (manager->kek_cache == NULL) {
                    CACHE_RWUNLOCK(manager);
                    return TECC_CACHE_ERROR_MEMORY;
                }
            }
            if (manager->kek_name_cache != NULL) {
                /* 名称缓存与主缓存共享item指针，item已由destroy_all_kek_items释放，
                 * 先清空悬空data指针防止fe_hash_destroy双重释放 */
                clear_hash_table_data_pointers(manager->kek_name_cache);
                destroy_hash_table(manager->kek_name_cache);
                manager->kek_name_cache = create_kek_name_hash_table();
                if (manager->kek_name_cache == NULL) {
                    CACHE_RWUNLOCK(manager);
                    return TECC_CACHE_ERROR_MEMORY;
                }
            }
            /* 重置KEK缓存刷新标记 */
            manager->kek_cache_needs_refresh = false;
            TECC_CACHE_DEBUG("KEK缓存已清理");
            break;

        case TECC_CACHE_REFRESH_DEK:
            /* 清理DEK缓存 */
            if (manager->dek_cache != NULL) {
                destroy_all_dek_items(manager->dek_cache);
                destroy_hash_table(manager->dek_cache);
                manager->dek_cache = create_dek_hash_table();
                if (manager->dek_cache == NULL) {
                    CACHE_RWUNLOCK(manager);
                    return TECC_CACHE_ERROR_MEMORY;
                }
            }
            if (manager->dek_name_cache != NULL) {
                /* 名称缓存与主缓存共享item指针，item已由destroy_all_dek_items释放，
                 * 先清空悬空data指针防止fe_hash_destroy双重释放 */
                clear_hash_table_data_pointers(manager->dek_name_cache);
                destroy_hash_table(manager->dek_name_cache);
                manager->dek_name_cache = create_dek_name_hash_table();
                if (manager->dek_name_cache == NULL) {
                    CACHE_RWUNLOCK(manager);
                    return TECC_CACHE_ERROR_MEMORY;
                }
            }
            /* 重置DEK缓存刷新标记 */
            manager->dek_cache_needs_refresh = false;
            TECC_CACHE_DEBUG("DEK缓存已清理");
            break;

        case TECC_CACHE_REFRESH_COLUMN:
            /* 清理列缓存 */
            if (manager->column_cache != NULL) {
                destroy_all_column_items(manager->column_cache);
                destroy_hash_table(manager->column_cache);
                manager->column_cache = create_column_hash_table();
                if (manager->column_cache == NULL) {
                    CACHE_RWUNLOCK(manager);
                    return TECC_CACHE_ERROR_MEMORY;
                }
            }
            if (manager->column_name_cache != NULL) {
                /* 名称缓存与主缓存共享item指针，item已由destroy_all_column_items释放，
                 * 先清空悬空data指针防止fe_hash_destroy双重释放 */
                clear_hash_table_data_pointers(manager->column_name_cache);
                destroy_hash_table(manager->column_name_cache);
                manager->column_name_cache = create_column_name_hash_table();
                if (manager->column_name_cache == NULL) {
                    CACHE_RWUNLOCK(manager);
                    return TECC_CACHE_ERROR_MEMORY;
                }
            }
            if (manager->table_schema_cache != NULL) {
                destroy_hash_table(manager->table_schema_cache);
                manager->table_schema_cache = create_table_schema_hash_table();
                if (manager->table_schema_cache == NULL) {
                    CACHE_RWUNLOCK(manager);
                    return TECC_CACHE_ERROR_MEMORY;
                }
            }
            /* 重置列缓存刷新标记 */
            manager->column_cache_needs_refresh = false;
            TECC_CACHE_DEBUG("加密列缓存已清理");
            break;

        case TECC_CACHE_REFRESH_ALL:
            /* 清理所有缓存 */
            /* 清理KEK缓存 — 先从主表安全释放item，再清空名称缓存悬空指针后销毁两张表 */
            if (manager->kek_cache != NULL) {
                destroy_all_kek_items(manager->kek_cache);
                destroy_hash_table(manager->kek_cache);
                manager->kek_cache = create_kek_hash_table();
                if (manager->kek_cache == NULL) {
                    CACHE_RWUNLOCK(manager);
                    return TECC_CACHE_ERROR_MEMORY;
                }
            }
            if (manager->kek_name_cache != NULL) {
                clear_hash_table_data_pointers(manager->kek_name_cache);
                destroy_hash_table(manager->kek_name_cache);
                manager->kek_name_cache = create_kek_name_hash_table();
                if (manager->kek_name_cache == NULL) {
                    CACHE_RWUNLOCK(manager);
                    return TECC_CACHE_ERROR_MEMORY;
                }
            }
            /* 重置KEK缓存刷新标记 */
            manager->kek_cache_needs_refresh = false;

            /* 清理DEK缓存 */
            if (manager->dek_cache != NULL) {
                destroy_all_dek_items(manager->dek_cache);
                destroy_hash_table(manager->dek_cache);
                manager->dek_cache = create_dek_hash_table();
                if (manager->dek_cache == NULL) {
                    CACHE_RWUNLOCK(manager);
                    return TECC_CACHE_ERROR_MEMORY;
                }
            }
            if (manager->dek_name_cache != NULL) {
                clear_hash_table_data_pointers(manager->dek_name_cache);
                destroy_hash_table(manager->dek_name_cache);
                manager->dek_name_cache = create_dek_name_hash_table();
                if (manager->dek_name_cache == NULL) {
                    CACHE_RWUNLOCK(manager);
                    return TECC_CACHE_ERROR_MEMORY;
                }
            }
            /* 重置DEK缓存刷新标记 */
            manager->dek_cache_needs_refresh = false;

            /* 清理列缓存 */
            if (manager->column_cache != NULL) {
                destroy_all_column_items(manager->column_cache);
                destroy_hash_table(manager->column_cache);
                manager->column_cache = create_column_hash_table();
                if (manager->column_cache == NULL) {
                    CACHE_RWUNLOCK(manager);
                    return TECC_CACHE_ERROR_MEMORY;
                }
            }
            if (manager->column_name_cache != NULL) {
                clear_hash_table_data_pointers(manager->column_name_cache);
                destroy_hash_table(manager->column_name_cache);
                manager->column_name_cache = create_column_name_hash_table();
                if (manager->column_name_cache == NULL) {
                    CACHE_RWUNLOCK(manager);
                    return TECC_CACHE_ERROR_MEMORY;
                }
            }
            if (manager->table_schema_cache != NULL) {
                destroy_hash_table(manager->table_schema_cache);
                manager->table_schema_cache = create_table_schema_hash_table();
                if (manager->table_schema_cache == NULL) {
                    CACHE_RWUNLOCK(manager);
                    return TECC_CACHE_ERROR_MEMORY;
                }
            }
            /* 重置列缓存刷新标记 */
            manager->column_cache_needs_refresh = false;

            TECC_CACHE_DEBUG("所有缓存已清理");
            break;

        default:
            CACHE_RWUNLOCK(manager);
            return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    /* 更新缓存统计信息 */
    manager->current_size = 0;  /* 重置缓存大小，实际大小会在重新加载时更新 */
    manager->last_sync_time = time(NULL);

    CACHE_RWUNLOCK(manager);
    return TECC_CACHE_OK;
}

/*
 * tecc_cache_get_stats - 获取缓存使用统计
 *
 * 参数:
 *   conn - 数据库连接
 *
 * 返回:
 *   缓存统计信息
 */
TeccCacheStats
tecc_cache_get_stats(PGconn *conn)
{
    TeccCacheManager *manager = NULL;
    TeccCacheStats stats;

    /* 参数检查 */
    memset(&stats, 0, sizeof(stats));
    if (conn == NULL) {
        return stats;
    }

    /* 获取缓存管理器 */
    manager = get_cache_manager_for_connection(conn);
    if (manager == NULL) {
        return stats;  /* 缓存管理器未初始化 */
    }

    /* 加锁保护统计信息读取（使用读锁） */
    CACHE_RLOCK(manager);

    /* 填充统计信息 */
    stats.cache_hits = manager->cache_hits;
    stats.cache_misses = manager->cache_misses;
    stats.name_cache_hits = manager->name_cache_hits;
    stats.name_cache_misses = manager->name_cache_misses;
    stats.load_count = manager->load_count;
    stats.current_size = manager->current_size;
    stats.max_size = manager->max_cache_size;

    /* 解锁 */
    CACHE_UNLOCK(manager);

    return stats;
}

/*
 * create_cache_manager - 创建缓存管理器
 *
 * 参数:
 *   conn - 数据库连接
 *
 * 返回:
 *   创建的缓存管理器指针，失败返回NULL
 */
static TeccCacheManager *
create_cache_manager(PGconn *conn)
{
    TeccCacheManager *manager = NULL;

    /* 参数检查 */
    if (conn == NULL) {
        return NULL;
    }

    /* 分配内存 */
    manager = (TeccCacheManager *)pg_malloc(sizeof(TeccCacheManager));
    if (manager == NULL) {
        return NULL;
    }

    /* 初始化基本字段 */
    memset(manager, 0, sizeof(TeccCacheManager));
    manager->conn = conn;
    manager->max_cache_size = 1000;  /* 默认最大缓存大小 */
    manager->current_size = 0;
    manager->load_count = 0;
    manager->last_sync_time = 0;
    manager->lock = NULL;  /* 锁将在后续初始化 */

    /* 初始化GUC参数结构 */
    manager->guc_params.search_path = NULL;
    manager->guc_params.sql_compatibility = NULL;
    manager->guc_params.current_schema = NULL;
    manager->guc_params.standard_conforming_strings = true;
    manager->guc_params.backslash_quote = 's';  /* 默认值 */
    manager->guc_params.escape_string_warning = true;
    manager->guc_params.role = NULL;

    /* 初始化状态标志 */
    manager->initialized = false;
    manager->guc_loaded = false;

    /* 初始化错误恢复状态（连接特定） */
    manager->recoverable_errors_mask = TECC_CACHE_RECOVERABLE_ALL;
    manager->random_seed = (unsigned int)time(NULL) ^ (unsigned int)(uintptr_t)conn;  /* 基于时间和连接指针的随机种子 */

    /* 初始化缓存刷新状态标记 */
    manager->kek_cache_needs_refresh = false;
    manager->dek_cache_needs_refresh = false;
    manager->column_cache_needs_refresh = false;

    /* 初始化定时刷新配置 */
    manager->timed_refresh_enabled = false;
    manager->timed_refresh_interval_sec = 0;
    manager->last_timed_refresh_time = time(NULL);

    /* 初始化元数据标记检查配置 */
    manager->last_metadata_check_time = 0;
    manager->metadata_check_interval_sec = 60;   /* 每60秒检查一次，避免每次查询都触发递归检查 */
    manager->metadata_check_enabled = true;      /* 默认启用标记检查 */

    /* 缓存哈希表将在后续初始化 */
    manager->kek_cache = NULL;
    manager->dek_cache = NULL;
    manager->column_cache = NULL;
    manager->kek_name_cache = NULL;
    manager->dek_name_cache = NULL;
    manager->column_name_cache = NULL;
    manager->table_schema_cache = NULL;

    return manager;
}

/*
 * destroy_cache_manager - 销毁缓存管理器
 *
 * 参数:
 *   manager - 缓存管理器指针
 *   cleanup_already_done - 是否已经执行了清理操作（避免双重释放）
 */
static void
destroy_cache_manager(TeccCacheManager *manager, bool cleanup_already_done)
{
    if (manager == NULL) {
        return;
    }

    /* 如果已经执行了清理操作，只释放管理器内存 */
    if (cleanup_already_done) {
        pg_free(manager);
        return;
    }

    /* 释放GUC参数内存 */
    if (manager->guc_params.search_path != NULL) {
        pg_free(manager->guc_params.search_path);
        manager->guc_params.search_path = NULL;
    }
    if (manager->guc_params.sql_compatibility != NULL) {
        pg_free(manager->guc_params.sql_compatibility);
        manager->guc_params.sql_compatibility = NULL;
    }
    if (manager->guc_params.current_schema != NULL) {
        pg_free(manager->guc_params.current_schema);
        manager->guc_params.current_schema = NULL;
    }
    if (manager->guc_params.role != NULL) {
        pg_free(manager->guc_params.role);
        manager->guc_params.role = NULL;
    }

    /* 释放锁资源（如果有） */
    if (manager->lock != NULL) {
        pthread_mutex_destroy((pthread_mutex_t *)manager->lock);
        pg_free(manager->lock);
        manager->lock = NULL;
    }

    /* 销毁读写锁 */
    pthread_rwlock_destroy(&manager->global_rwlock);

    /* 释放缓存哈希表（如果有）——先释放item对象，再销毁hash表 */
    /* KEK items同时存在于kek_cache和kek_name_cache，只从主表释放一次 */
    if (manager->kek_cache != NULL) {
        destroy_all_kek_items(manager->kek_cache);
        destroy_hash_table(manager->kek_cache);
        manager->kek_cache = NULL;
    }
    /* DEK items同时存在于dek_cache和dek_name_cache，只从主表释放一次 */
    if (manager->dek_cache != NULL) {
        destroy_all_dek_items(manager->dek_cache);
        destroy_hash_table(manager->dek_cache);
        manager->dek_cache = NULL;
    }
    /* Column items同时存在于column_cache和column_name_cache，只从主表释放一次 */
    if (manager->column_cache != NULL) {
        destroy_all_column_items(manager->column_cache);
        destroy_hash_table(manager->column_cache);
        manager->column_cache = NULL;
    }

    /* 释放名称查找哈希表（如果有）——item已由destroy_all_*_items释放，先清空悬空data指针 */
    if (manager->kek_name_cache != NULL) {
        clear_hash_table_data_pointers(manager->kek_name_cache);
        destroy_hash_table(manager->kek_name_cache);
        manager->kek_name_cache = NULL;
    }
    if (manager->dek_name_cache != NULL) {
        clear_hash_table_data_pointers(manager->dek_name_cache);
        destroy_hash_table(manager->dek_name_cache);
        manager->dek_name_cache = NULL;
    }
    if (manager->column_name_cache != NULL) {
        clear_hash_table_data_pointers(manager->column_name_cache);
        destroy_hash_table(manager->column_name_cache);
        manager->column_name_cache = NULL;
    }
    if (manager->table_schema_cache != NULL) {
        destroy_hash_table(manager->table_schema_cache);
        manager->table_schema_cache = NULL;
    }

    /* 重置状态标志（确保清理后状态一致） */
    manager->initialized = false;
    manager->guc_loaded = false;
    manager->current_size = 0;
    manager->load_count = 0;
    manager->cache_hits = 0;
    manager->cache_misses = 0;
    manager->name_cache_hits = 0;
    manager->name_cache_misses = 0;

    /* 释放管理器本身 */
    pg_free(manager);
}




/*
 * destroy_kek_cache_item - 销毁KEK缓存项
 *
 * 参数:
 *   item - KEK缓存项指针
 */
static void
destroy_kek_cache_item(TeccKekCacheItem *item)
{
    if (item == NULL) {
        return;
    }

    /* 安全清理敏感元数据 */
    tecc_cache_secure_clear_kek_metadata(item);

    /* 释放字符串字段（安全清理后释放内存） */
    if (item->key_name != NULL) {
        pg_free(item->key_name);
        item->key_name = NULL;
    }
    if (item->key_namespace != NULL) {
        pg_free(item->key_namespace);
        item->key_namespace = NULL;
    }
    if (item->kms_path != NULL) {
        pg_free(item->kms_path);
        item->kms_path = NULL;
    }
    /* 移除了冗余字段的释放：key_store, key_path, kek_algorithm, plain_key_value */

    /* ReqID:TECC4.0 — 可靠擦除并释放缓存的明文KEK */
    if (item->plain_kek != NULL)
    {
        tecc_cache_secure_clear_memory(item->plain_kek, item->plain_kek_length);
        pg_free(item->plain_kek);
        item->plain_kek = NULL;
        item->plain_kek_length = 0;
    }

    /* 安全清理结构体剩余部分并释放内存 */
    tecc_cache_secure_clear_memory(item, sizeof(TeccKekCacheItem));
    pg_free(item);
}

/*
 * destroy_dek_cache_item - 销毁DEK缓存项
 *
 * 参数:
 *   item - DEK缓存项指针
 */
static void
destroy_dek_cache_item(TeccDekCacheItem *item)
{
    if (item == NULL) {
        return;
    }

    /* 安全清理敏感数据 */
    tecc_cache_secure_clear_dek_key(item);

    /* 释放字符串字段（安全清理后释放内存） */
    if (item->key_name != NULL) {
        pg_free(item->key_name);
        item->key_name = NULL;
    }
    if (item->key_namespace != NULL) {
        pg_free(item->key_namespace);
        item->key_namespace = NULL;
    }

    /* 移除了冗余字段的释放：encrypted_value, support_algo, plain_key_value */

    /* 释放加密密钥数据（安全清理后释放内存） */
    if (item->encrypted_key != NULL) {
        pg_free(item->encrypted_key);
        item->encrypted_key = NULL;
    }

    /* ReqID:TECC4.0 — 可靠擦除并释放缓存的解密后DEK */
    if (item->plain_dek != NULL)
    {
        tecc_cache_secure_clear_memory(item->plain_dek, item->plain_dek_length);
        pg_free(item->plain_dek);
        item->plain_dek = NULL;
        item->plain_dek_length = 0;
    }

    /* 安全清理结构体剩余部分并释放内存 */
    tecc_cache_secure_clear_memory(item, sizeof(TeccDekCacheItem));
    pg_free(item);
}

/*
 * destroy_column_cache_item - 销毁列缓存项
 *
 * 参数:
 *   item - 列缓存项指针
 */
static void
destroy_column_cache_item(TeccColumnCacheItem *item)
{
    if (item == NULL) {
        return;
    }

    /* 释放字符串字段 */
    if (item->table_name != NULL) {
        pg_free(item->table_name);
        item->table_name = NULL;
    }
    if (item->schema_name != NULL) {
        pg_free(item->schema_name);
        item->schema_name = NULL;
    }
    if (item->database_name != NULL) {
        pg_free(item->database_name);
        item->database_name = NULL;
    }
    if (item->column_name != NULL) {
        pg_free(item->column_name);
        item->column_name = NULL;
    }
    if (item->collation_locale != NULL) {
        pg_free(item->collation_locale);
        item->collation_locale = NULL;
    }

    /* 释放缓存项本身 */
    pg_free(item);
}

/*
 * tecc_cache_load_guc_params - 加载GUC参数到缓存
 *
 * 参数:
 *   conn - 数据库连接
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
int
tecc_cache_load_guc_params(const TeccState *state)
{
    TeccCacheManager *manager = NULL;
    TeccFetcherErrorCode fetcher_error;
    int result;

    /* 参数检查 */
    if (state == NULL) {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    /* 直接从TeccState获取缓存管理器 */
    manager = (TeccCacheManager *)state->metadata_cache;
    if (manager == NULL) {
        return TECC_CACHE_ERROR_NOT_INITIALIZED;  /* 缓存管理器未初始化 */
    }

    /* 加锁保护GUC参数加载 */
    CACHE_WLOCK(manager);

    /* 检查是否已加载 */
    if (manager->guc_loaded) {
        CACHE_RWUNLOCK(manager);
        return TECC_CACHE_OK;  /* 已加载，无需重复加载 */
    }

    /* 通过简化的数据获取器适配器查询GUC参数 */
    fetcher_error = tecc_fetch_guc_for_cache(state, manager);
    result = (fetcher_error == TECC_FETCHER_OK) ? TECC_CACHE_OK : tecc_cache_convert_error(fetcher_error);

    /* 解锁 */
    CACHE_RWUNLOCK(manager);

    return result;
}

/*
 * parse_current_schema_from_search_path - 从search_path字符串解析current_schema
 *
 * 按PG规则：取search_path第一个schema，处理"$user"替换。
 * 返回pg_malloc分配的字符串，调用者负责pg_free。
 */
static char *
parse_current_schema_from_search_path(const char *search_path, const char *user_name)
{
    const char *p;

    if (search_path == NULL || search_path[0] == '\0')
        return NULL;

    p = search_path;

    /* 跳过前导空白 */
    while (*p == ' ' || *p == '\t')
        p++;
    if (*p == '\0')
        return NULL;

    /* 处理"$user" */
    if (*p == '"' && strncmp(p, "\"$user\"", 7) == 0)
    {
        if (user_name != NULL && user_name[0] != '\0')
            return pg_strdup(user_name);
        /* 没有用户信息则跳过，取下一个schema */
        p += 7;
        while (*p == ' ' || *p == '\t' || *p == ',')
            p++;
        if (*p == '\0')
            return NULL;
    }

    /* 跳过逗号后的空白 */
    while (*p == ' ' || *p == '\t' || *p == ',')
        p++;

    /* 解析第一个schema名 */
    {
        const char *start;
        int len;

        if (*p == '"')
        {
            /* 引号包裹的标识符 */
            p++;
            start = p;
            while (*p && *p != '"')
                p++;
            len = (int)(p - start);
        }
        else
        {
            /* 普通标识符 */
            start = p;
            while (*p && *p != ' ' && *p != '\t' && *p != ',')
                p++;
            len = (int)(p - start);
        }

        if (len <= 0)
            return NULL;

        {
            char *schema_name = (char *)pg_malloc(len + 1);
            if (schema_name == NULL)
                return NULL;
            memcpy(schema_name, start, len);
            schema_name[len] = '\0';
            return schema_name;
        }
    }
}

/*
 * sync_guc_params_from_live_conn - 从libpq连接同步search_path/current_schema到缓存
 *
 * 当PQparameterStatus检测到search_path变化时，更新缓存和解析后的列表。
 * 避免对search_path_schemas做懒检测，一次性同步。
 */
static void
sync_guc_params_from_live_conn(const TeccState *state, TeccCacheManager *manager)
{
    const char *live_sp;
    const char *user_name;
    char *computed_schema;
    bool changed = false;

    if (state == NULL || manager == NULL || state->conn == NULL)
        return;

    live_sp = PQparameterStatus(state->conn, "search_path");
    if (live_sp == NULL || live_sp[0] == '\0')
        return;

    /* 检查是否有变化（在mutex保护下） */
    CACHE_LOCK(manager);
    if (manager->guc_params.search_path != NULL &&
        strcmp(manager->guc_params.search_path, live_sp) == 0)
    {
        CACHE_UNLOCK(manager);
        return;
    }
    changed = true;

    TECC_CACHE_DEBUG("sync_guc_params_from_live_conn: search_path changed: '%s' -> '%s'",
                     manager->guc_params.search_path ? manager->guc_params.search_path : "(null)",
                     live_sp);

    /* 更新search_path缓存 */
    if (manager->guc_params.search_path != NULL)
    {
        pg_free(manager->guc_params.search_path);
        manager->guc_params.search_path = NULL;
    }
    manager->guc_params.search_path = pg_strdup(live_sp);

    /* 计算并更新current_schema */
    user_name = PQparameterStatus(state->conn, "session_authorization");
    if (user_name == NULL || user_name[0] == '\0')
        user_name = (state->conn != NULL) ? PQuser(state->conn) : NULL;

    if (manager->guc_params.current_schema != NULL)
    {
        pg_free(manager->guc_params.current_schema);
        manager->guc_params.current_schema = NULL;
    }
    computed_schema = parse_current_schema_from_search_path(live_sp, user_name);
    if (computed_schema != NULL)
    {
        manager->guc_params.current_schema = computed_schema;
        TECC_CACHE_DEBUG("sync_guc_params_from_live_conn: current_schema = %s",
                        computed_schema);
    }

    CACHE_UNLOCK(manager);

    /* 重新解析search_path_schemas（parse函数有自己加锁，不能在持有锁时调用） */
    if (changed)
    {
        /* ReqID:TECC4.0 - search_path 变化：表→schema 映射缓存失效重建
         * （同一表名在不同 search_path 下解析结果不同，必须整体失效） */
        if (manager->table_schema_cache != NULL)
        {
            destroy_hash_table(manager->table_schema_cache);
            manager->table_schema_cache = create_table_schema_hash_table();
        }
        tecc_cache_parse_search_path(manager, state->conn);
    }
}

/*
 * tecc_cache_update_search_path - 将search_path值直接更新到缓存
 *
 * 当TECC拦截到"SET search_path TO ..."时调用，比PQparameterStatus更可靠，
 * 因为当前TECC服务器中search_path未设置GUC_REPORT标志。
 * 调用者确保new_search_path非空。
 */
void
tecc_cache_update_search_path(struct TeccState *state, const char *new_search_path)
{
    TeccCacheManager *manager;
    const char *user_name;
    char *computed_schema;

    if (state == NULL || new_search_path == NULL || new_search_path[0] == '\0')
        return;

    manager = (TeccCacheManager *)state->metadata_cache;
    if (manager == NULL)
        return;

    TECC_CACHE_DEBUG("tecc_cache_update_search_path: updating to '%s'", new_search_path);

    CACHE_LOCK(manager);

    /* 更新search_path缓存 */
    if (manager->guc_params.search_path != NULL)
    {
        pg_free(manager->guc_params.search_path);
        manager->guc_params.search_path = NULL;
    }
    manager->guc_params.search_path = pg_strdup(new_search_path);

    /* 重新计算current_schema */
    if (manager->guc_params.current_schema != NULL)
    {
        pg_free(manager->guc_params.current_schema);
        manager->guc_params.current_schema = NULL;
    }
    user_name = NULL;
    if (state->conn != NULL)
    {
        user_name = PQparameterStatus(state->conn, "session_authorization");
        if (user_name == NULL || user_name[0] == '\0')
            user_name = PQuser(state->conn);
    }
    computed_schema = parse_current_schema_from_search_path(new_search_path, user_name);
    if (computed_schema != NULL)
    {
        manager->guc_params.current_schema = computed_schema;
        TECC_CACHE_DEBUG("tecc_cache_update_search_path: current_schema = %s", computed_schema);
    }

    CACHE_UNLOCK(manager);

    /* ReqID:TECC4.0 - search_path 变化：表→schema 映射缓存失效重建 */
    if (manager->table_schema_cache != NULL)
    {
        destroy_hash_table(manager->table_schema_cache);
        manager->table_schema_cache = create_table_schema_hash_table();
    }

    /* 重新解析search_path_schemas */
    tecc_cache_parse_search_path(manager, state->conn);
}

/*
 * tecc_cache_get_guc_param - 获取GUC参数值
 *
 * search_path和current_schema使用PQparameterStatus直接获取实时值，
 * 其他参数从缓存读取（这些参数不会在会话中变化）。
 */
const char *
tecc_cache_get_guc_param(const TeccState *state, const char *param_name)
{
    TeccCacheManager *manager = NULL;
    size_t len;
    size_t i;
    const char *result;

    /* 参数检查 */
    if (state == NULL || param_name == NULL) {
        TECC_CACHE_DEBUG("无效参数：state或param_name为NULL");
        return NULL;
    }
    /* 检查字符串长度，防止缓冲区溢出 */
    if (strlen(param_name) >= 256) {
        TECC_CACHE_DEBUG("无效参数：param_name长度超过限制（256）");
        return NULL;
    }
    /* 检查字符串是否包含非法字符 */
    len = strlen(param_name);
    for (i = 0; i < len; i++) {
        if (param_name[i] == '\0') {
            TECC_CACHE_DEBUG("无效参数：param_name包含空字符");
            return NULL;
        }
    }

    /* ============================================================
     * search_path / current_schema — 先同步 ParameterStatus 权威值
     * 到缓存，再从缓存统一读取。
     * 原因：PQparameterStatus 返回 conn->pstatus 内部指针，下次收到
     * 同名的 ParameterStatus 消息时会被 pqSaveParameterStatus free 掉
     * （use-after-free）；统一先 sync 进缓存（strdup 堆内存）再返回，
     * 指针稳定且缓存始终是最新权威值。
     * ============================================================ */
    if (strcmp(param_name, "search_path") == 0 ||
        strcmp(param_name, "current_schema") == 0)
    {
        manager = (TeccCacheManager *)state->metadata_cache;
        if (manager != NULL)
            sync_guc_params_from_live_conn(state, manager);
    }

    /* ============================================================
     * 其他参数 — 从缓存读取
     * ============================================================ */
    /* 直接从TeccState获取缓存管理器 */
    manager = (TeccCacheManager *)state->metadata_cache;
    if (manager == NULL) {
        return NULL;
    }

    /* 加锁保护GUC参数读取 */
    CACHE_RLOCK(manager);

    /* 检查GUC参数是否已加载 */
    if (!manager->guc_loaded) {
        CACHE_RWUNLOCK(manager);

        if (tecc_cache_load_guc_params(state) != TECC_CACHE_OK) {
            return NULL;
        }

        CACHE_RLOCK(manager);
    }

    /* 根据参数名称返回对应的值 */
    result = NULL;

    if (strcmp(param_name, "search_path") == 0) {
        result = manager->guc_params.search_path;
    } else if (strcmp(param_name, "current_schema") == 0) {
        result = manager->guc_params.current_schema;
    } else if (strcmp(param_name, "sql_compatibility") == 0) {
        result = manager->guc_params.sql_compatibility;
    } else if (strcmp(param_name, "standard_conforming_strings") == 0) {
        result = manager->guc_params.standard_conforming_strings ? "on" : "off";
    } else if (strcmp(param_name, "backslash_quote") == 0) {
        static TECC_THREAD_LOCAL char backslash_quote_str[2] = {0};
        backslash_quote_str[0] = manager->guc_params.backslash_quote;
        backslash_quote_str[1] = '\0';
        result = backslash_quote_str;
    } else if (strcmp(param_name, "escape_string_warning") == 0) {
        result = manager->guc_params.escape_string_warning ? "on" : "off";
    } else if (strcmp(param_name, "role") == 0) {
        result = manager->guc_params.role;
    }

    CACHE_RWUNLOCK(manager);
    return result;

    CACHE_UNLOCK(manager);
    return NULL;
}

/*
 * tecc_cache_update_guc_param - 更新缓存的GUC参数值
 *
 * 参数:
 *   conn - 数据库连接
 *   param_name - 参数名称
 *   param_value - 参数值
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
int
tecc_cache_update_guc_param(const TeccState *state, const char *param_name, const char *param_value)
{
    TeccCacheManager *manager = NULL;
    char *new_value = NULL;

    /* 参数检查 */
    if (state == NULL || param_name == NULL || param_value == NULL) {
        TECC_CACHE_DEBUG("无效参数：state、param_name或param_value为NULL");
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }
    /* 检查字符串长度，防止缓冲区溢出 */
    if (strlen(param_name) >= 256) {  /* 合理的参数名称长度限制 */
        TECC_CACHE_DEBUG("无效参数：param_name长度超过限制（256）");
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }
    if (strlen(param_value) >= 1024) {  /* 合理的参数值长度限制 */
        TECC_CACHE_DEBUG("无效参数：param_value长度超过限制（1024）");
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }
    /* 检查字符串是否包含非法字符 */
    if (strchr(param_name, '\0') != NULL || strchr(param_value, '\0') != NULL) {
        TECC_CACHE_DEBUG("无效参数：字符串包含空字符");
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    /* 直接从TeccState获取缓存管理器 */
    manager = (TeccCacheManager *)state->metadata_cache;
    if (manager == NULL) {
        return TECC_CACHE_ERROR_NOT_INITIALIZED;  /* 缓存管理器未初始化 */
    }

    /* 加锁保护GUC参数更新 */
    CACHE_WLOCK(manager);

    /* 复制新值 */
    new_value = strdup(param_value);
    if (new_value == NULL) {
        CACHE_RWUNLOCK(manager);
        return TECC_CACHE_ERROR_MEMORY;
    }

    /* 根据参数名称更新对应的值 */
    if (strcmp(param_name, "search_path") == 0) {
        if (manager->guc_params.search_path != NULL) {
            pg_free(manager->guc_params.search_path);
        }
        manager->guc_params.search_path = new_value;
    } else if (strcmp(param_name, "sql_compatibility") == 0) {
        if (manager->guc_params.sql_compatibility != NULL) {
            pg_free(manager->guc_params.sql_compatibility);
        }
        manager->guc_params.sql_compatibility = new_value;
    } else if (strcmp(param_name, "current_schema") == 0) {
        if (manager->guc_params.current_schema != NULL) {
            pg_free(manager->guc_params.current_schema);
        }
        manager->guc_params.current_schema = new_value;
    } else if (strcmp(param_name, "standard_conforming_strings") == 0) {
        if (strcmp(param_value, "on") == 0 || strcmp(param_value, "true") == 0 || strcmp(param_value, "1") == 0) {
            manager->guc_params.standard_conforming_strings = true;
        } else if (strcmp(param_value, "off") == 0 || strcmp(param_value, "false") == 0 || strcmp(param_value, "0") == 0) {
            manager->guc_params.standard_conforming_strings = false;
        } else {
            pg_free(new_value);
            return TECC_CACHE_ERROR_INVALID_PARAM;
        }
        pg_free(new_value);  /* 布尔值不需要字符串存储 */
    } else if (strcmp(param_name, "backslash_quote") == 0) {
        if (strlen(param_value) == 1) {
            manager->guc_params.backslash_quote = param_value[0];
        } else {
            pg_free(new_value);
            return TECC_CACHE_ERROR_INVALID_PARAM;
        }
        pg_free(new_value);  /* 字符值不需要字符串存储 */
    } else if (strcmp(param_name, "escape_string_warning") == 0) {
        if (strcmp(param_value, "on") == 0 || strcmp(param_value, "true") == 0 || strcmp(param_value, "1") == 0) {
            manager->guc_params.escape_string_warning = true;
        } else if (strcmp(param_value, "off") == 0 || strcmp(param_value, "false") == 0 || strcmp(param_value, "0") == 0) {
            manager->guc_params.escape_string_warning = false;
        } else {
            pg_free(new_value);
            return TECC_CACHE_ERROR_INVALID_PARAM;
        }
        pg_free(new_value);  /* 布尔值不需要字符串存储 */
    } else if (strcmp(param_name, "role") == 0) {
        if (manager->guc_params.role != NULL) {
            pg_free(manager->guc_params.role);
        }
        manager->guc_params.role = new_value;
    } else {
        /* 未知参数名称 */
        pg_free(new_value);
        CACHE_RWUNLOCK(manager);
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    CACHE_RWUNLOCK(manager);
    return TECC_CACHE_OK;
}

/*
 * get_cache_manager_for_connection - 获取指定连接的缓存管理器
 *
 * 参数:
 *   conn - 数据库连接
 *
 * 返回:
 *   缓存管理器指针，未找到返回NULL
 *
 * 注意：这是占位实现，使用全局变量。后续需要改为从连接上下文中获取。
 */
static TeccCacheManager *
get_cache_manager_for_connection(PGconn *conn)
{
    TeccState *state = NULL;

    /* 参数检查 */
    if (conn == NULL) {
        return NULL;
    }

    /* 首先尝试从TeccState获取缓存管理器 */
    state = tecc_get_state(conn);
    if (state != NULL && state->metadata_cache != NULL) {
        TeccCacheManager *manager = (TeccCacheManager *)state->metadata_cache;
        /* 验证连接匹配 */
        if (manager->conn == conn) {
            return manager;
        }
    }

    /* 全局缓存管理器已移除，仅从TeccState获取 */
    return NULL;
}

/*
 * mark_kek_cache_for_refresh - 标记KEK缓存需要刷新
 *
 * 参数:
 *   manager - 缓存管理器
 *
 * 返回:
 *   0表示成功，非0表示错误码
 *
 * 说明:
 *   此函数仅设置刷新标记，实际的缓存清空和重新加载将在后续查询时
 *   通过延迟刷新机制执行。
 */
static int
mark_kek_cache_for_refresh(TeccCacheManager *manager)
{
    /* 加锁保护缓存刷新标记设置 */
    CACHE_WLOCK(manager);

    /* 设置KEK缓存需要刷新标记 */
    manager->kek_cache_needs_refresh = true;

    /* 更新最后同步时间 */
    manager->last_sync_time = time(NULL);

    /* 解锁 */
    CACHE_RWUNLOCK(manager);

    TECC_CACHE_DEBUG("标记KEK缓存需要刷新");

    return TECC_CACHE_OK;
}

/*
 * mark_dek_cache_for_refresh - 标记DEK缓存需要刷新
 *
 * 参数:
 *   manager - 缓存管理器
 *
 * 返回:
 *   0表示成功，非0表示错误码
 *
 * 说明:
 *   此函数仅设置刷新标记，实际的缓存清空和重新加载将在后续查询时
 *   通过延迟刷新机制执行。
 */
static int
mark_dek_cache_for_refresh(TeccCacheManager *manager)
{
    /* 加锁保护缓存刷新标记设置 */
    CACHE_WLOCK(manager);

    /* 设置DEK缓存需要刷新标记 */
    manager->dek_cache_needs_refresh = true;

    /* 更新最后同步时间 */
    manager->last_sync_time = time(NULL);

    /* 解锁 */
    CACHE_RWUNLOCK(manager);

    TECC_CACHE_DEBUG("标记DEK缓存需要刷新");

    return TECC_CACHE_OK;
}

/*
 * tecc_cache_mark_kek_refresh - 公共接口：标记KEK缓存需要刷新
 */
void
tecc_cache_mark_kek_refresh(const TeccState *state)
{
    TeccCacheManager *manager = get_cache_manager(state);
    if (manager != NULL)
    {
        int ret = mark_kek_cache_for_refresh(manager);
        if (ret == TECC_CACHE_OK)
        {
            TECC_CACHE_DEBUG("公共接口：KEK缓存已标记为需要刷新");
        }
        else
        {
            TECC_CACHE_DEBUG("公共接口：标记KEK缓存刷新失败，错误码: %d", ret);
        }
    }
    else
    {
        TECC_CACHE_DEBUG("公共接口：标记KEK缓存刷新失败，无法获取缓存管理器");
    }
}

/*
 * tecc_cache_mark_dek_refresh - 公共接口：标记DEK缓存需要刷新
 */
void
tecc_cache_mark_dek_refresh(const TeccState *state)
{
    TeccCacheManager *manager = get_cache_manager(state);
    if (manager != NULL)
    {
        int ret = mark_dek_cache_for_refresh(manager);
        if (ret == TECC_CACHE_OK)
        {
            TECC_CACHE_DEBUG("公共接口：DEK缓存已标记为需要刷新");
        }
        else
        {
            TECC_CACHE_DEBUG("公共接口：标记DEK缓存刷新失败，错误码: %d", ret);
        }
    }
    else
    {
        TECC_CACHE_DEBUG("公共接口：标记DEK缓存刷新失败，无法获取缓存管理器");
    }
}

/*
 * refresh_guc_params - 刷新GUC参数缓存
 *
 * 参数:
 *   manager - 缓存管理器
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
static int
refresh_guc_params(TeccCacheManager *manager)
{
    /* 清理现有GUC参数缓存 */
    if (manager->guc_params.search_path != NULL) {
        pg_free(manager->guc_params.search_path);
        manager->guc_params.search_path = NULL;
    }
    if (manager->guc_params.sql_compatibility != NULL) {
        pg_free(manager->guc_params.sql_compatibility);
        manager->guc_params.sql_compatibility = NULL;
    }
    if (manager->guc_params.current_schema != NULL) {
        pg_free(manager->guc_params.current_schema);
        manager->guc_params.current_schema = NULL;
    }
    if (manager->guc_params.role != NULL) {
        pg_free(manager->guc_params.role);
        manager->guc_params.role = NULL;
    }

    /* 重置GUC参数标志 */
    manager->guc_loaded = false;

    /* 重新加载GUC参数 */
    /* 注意：这里需要调用tecc_cache_load_guc_params，但需要PGconn参数 */
    /* 由于这是内部函数，我们假设GUC参数会在需要时重新加载 */

    return TECC_CACHE_OK;
}

/* ==================== 哈希函数实现 ==================== */

/*
 * hash_kek_key - KEK哈希键哈希函数
 *
 * 参数:
 *   key - 哈希键指针
 *   keysize - 键大小
 *
 * 返回:
 *   哈希值
 */
static uint32
hash_kek_key(const void *key, Size keysize)
{
    const KekHashKey *kek_key = (const KekHashKey *)key;
    /* 使用简单的OID哈希 */
    return (uint32)kek_key->oid;
}

/*
 * hash_dek_key - DEK哈希键哈希函数
 *
 * 参数:
 *   key - 哈希键指针
 *   keysize - 键大小
 *
 * 返回:
 *   哈希值
 */
static uint32
hash_dek_key(const void *key, Size keysize)
{
    const DekHashKey *dek_key = (const DekHashKey *)key;
    /* 使用简单的OID哈希 */
    return (uint32)dek_key->oid;
}

/*
 * hash_column_key - 列哈希键哈希函数
 *
 * 参数:
 *   key - 哈希键指针
 *   keysize - 键大小
 *
 * 返回:
 *   哈希值
 */
static uint32
hash_column_key(const void *key, Size keysize)
{
    const ColumnHashKey *column_key = (const ColumnHashKey *)key;
    /* 组合表OID和列号的哈希 */
    uint32 hash = (uint32)column_key->table_oid;
    hash = hash * 31 + (uint32)column_key->column_attnum;
    return hash;
}

/*
 * hash_kek_name_key - KEK名称哈希键哈希函数
 *
 * 参数:
 *   key - 哈希键指针
 *   keysize - 键大小
 *
 * 返回:
 *   哈希值
 */
static uint32
hash_kek_name_key(const void *key, Size keysize)
{
    const KekNameHashKey *name_key = (const KekNameHashKey *)key;
    uint32 hash = 0;
    const char *p;

    /* 使用djb2哈希算法（大小写不敏感） */
    for (p = name_key->key_name; *p; p++) {
        hash = hash * 31 + tolower((unsigned char)*p);
    }
    for (p = name_key->key_namespace; *p; p++) {
        hash = hash * 31 + tolower((unsigned char)*p);
    }
    return hash;
}

/*
 * hash_dek_name_key - DEK名称哈希键哈希函数
 *
 * 参数:
 *   key - 哈希键指针
 *   keysize - 键大小
 *
 * 返回:
 *   哈希值
 */
static uint32
hash_dek_name_key(const void *key, Size keysize)
{
    const DekNameHashKey *name_key = (const DekNameHashKey *)key;
    uint32 hash = 0;
    const char *p;

    /* 使用djb2哈希算法（大小写不敏感） */
    for (p = name_key->key_name; *p; p++) {
        hash = hash * 31 + tolower((unsigned char)*p);
    }
    for (p = name_key->key_namespace; *p; p++) {
        hash = hash * 31 + tolower((unsigned char)*p);
    }
    return hash;
}

/*
 * hash_column_name_key - 列名称哈希键哈希函数
 *
 * 参数:
 *   key - 哈希键指针
 *   keysize - 键大小
 *
 * 返回:
 *   哈希值
 */
static uint32
hash_column_name_key(const void *key, Size keysize)
{
    const ColumnNameHashKey *name_key = (const ColumnNameHashKey *)key;
    uint32 hash = 0;
    const char *p;

    /* 使用djb2哈希算法，组合所有名称部分 */
    for (p = name_key->database_name; *p; p++) {
        hash = hash * 31 + *p;
    }
    for (p = name_key->schema_name; *p; p++) {
        hash = hash * 31 + *p;
    }
    for (p = name_key->table_name; *p; p++) {
        hash = hash * 31 + *p;
    }
    for (p = name_key->column_name; *p; p++) {
        hash = hash * 31 + *p;
    }
    return hash;
}

/*
 * compare_kek_keys - KEK哈希键比较函数
 *
 * 参数:
 *   key1 - 第一个键指针
 *   key2 - 第二个键指针
 *   keysize - 键大小
 *
 * 返回:
 *   0表示相等，非0表示不相等
 */
static int
compare_kek_keys(const void *key1, const void *key2, Size keysize)
{
    const KekHashKey *k1 = (const KekHashKey *)key1;
    const KekHashKey *k2 = (const KekHashKey *)key2;

    if (k1->oid == k2->oid) {
        return 0;
    }
    return 1;
}

/*
 * compare_dek_keys - DEK哈希键比较函数
 *
 * 参数:
 *   key1 - 第一个键指针
 *   key2 - 第二个键指针
 *   keysize - 键大小
 *
 * 返回:
 *   0表示相等，非0表示不相等
 */
static int
compare_dek_keys(const void *key1, const void *key2, Size keysize)
{
    const DekHashKey *k1 = (const DekHashKey *)key1;
    const DekHashKey *k2 = (const DekHashKey *)key2;

    if (k1->oid == k2->oid) {
        return 0;
    }
    return 1;
}

/*
 * compare_column_keys - 列哈希键比较函数
 *
 * 参数:
 *   key1 - 第一个键指针
 *   key2 - 第二个键指针
 *   keysize - 键大小
 *
 * 返回:
 *   0表示相等，非0表示不相等
 */
static int
compare_column_keys(const void *key1, const void *key2, Size keysize)
{
    const ColumnHashKey *k1 = (const ColumnHashKey *)key1;
    const ColumnHashKey *k2 = (const ColumnHashKey *)key2;

    if (k1->table_oid == k2->table_oid && k1->column_attnum == k2->column_attnum) {
        return 0;
    }
    return 1;
}

/*
 * compare_kek_name_keys - KEK名称哈希键比较函数
 *
 * 参数:
 *   key1 - 第一个键指针
 *   key2 - 第二个键指针
 *   keysize - 键大小
 *
 * 返回:
 *   0表示相等，非0表示不相等
 */
static int
compare_kek_name_keys(const void *key1, const void *key2, Size keysize)
{
    const KekNameHashKey *k1 = (const KekNameHashKey *)key1;
    const KekNameHashKey *k2 = (const KekNameHashKey *)key2;

    if (pg_strcasecmp(k1->key_name, k2->key_name) != 0) return 1;
    if (pg_strcasecmp(k1->key_namespace, k2->key_namespace) != 0) return 1;
    return 0;
}

/*
 * compare_dek_name_keys - DEK名称哈希键比较函数
 *
 * 参数:
 *   key1 - 第一个键指针
 *   key2 - 第二个键指针
 *   keysize - 键大小
 *
 * 返回:
 *   0表示相等，非0表示不相等
 */
static int
compare_dek_name_keys(const void *key1, const void *key2, Size keysize)
{
    const DekNameHashKey *k1 = (const DekNameHashKey *)key1;
    const DekNameHashKey *k2 = (const DekNameHashKey *)key2;

    if (pg_strcasecmp(k1->key_name, k2->key_name) != 0) return 1;
    if (pg_strcasecmp(k1->key_namespace, k2->key_namespace) != 0) return 1;
    return 0;
}

/*
 * compare_column_name_keys - 列名称哈希键比较函数
 *
 * 参数:
 *   key1 - 第一个键指针
 *   key2 - 第二个键指针
 *   keysize - 键大小
 *
 * 返回:
 *   0表示相等，非0表示不相等
 */
static int
compare_column_name_keys(const void *key1, const void *key2, Size keysize)
{
    const ColumnNameHashKey *k1 = (const ColumnNameHashKey *)key1;
    const ColumnNameHashKey *k2 = (const ColumnNameHashKey *)key2;

    if (strcmp(k1->database_name, k2->database_name) != 0) return 1;
    if (strcmp(k1->schema_name, k2->schema_name) != 0) return 1;
    if (strcmp(k1->table_name, k2->table_name) != 0) return 1;
    if (strcmp(k1->column_name, k2->column_name) != 0) return 1;
    return 0;
}

/* ReqID:TECC4.0 - 未限定表名→schema 映射缓存值（键：TableSchemaHashKey） */
typedef struct {
    char schema_name[NAMEDATALEN];       /* 解析出的真实 schema */
    bool not_found;                      /* 表不在 search_path 任何 schema */
} TableSchemaCacheValue;

/*
 * hash_table_schema_key - 表名→schema 缓存键哈希函数
 *
 * ReqID:TECC4.0 - 组合 database_name + table_name（djb2 风格，同列名缓存）
 */
static uint32
hash_table_schema_key(const void *key, Size keysize)
{
    const TableSchemaHashKey *ts_key = (const TableSchemaHashKey *)key;
    uint32 hash = 0;
    const char *p;

    for (p = ts_key->database_name; *p; p++) {
        hash = hash * 31 + *p;
    }
    for (p = ts_key->table_name; *p; p++) {
        hash = hash * 31 + *p;
    }
    return hash;
}

/*
 * compare_table_schema_keys - 表名→schema 缓存键比较函数
 *
 * ReqID:TECC4.0 - 0表示相等，非0表示不相等
 */
static int
compare_table_schema_keys(const void *key1, const void *key2, Size keysize)
{
    const TableSchemaHashKey *k1 = (const TableSchemaHashKey *)key1;
    const TableSchemaHashKey *k2 = (const TableSchemaHashKey *)key2;

    if (strcmp(k1->database_name, k2->database_name) != 0) return 1;
    if (strcmp(k1->table_name, k2->table_name) != 0) return 1;
    return 0;
}

/*
 * create_table_schema_hash_table - 创建表名→schema 映射哈希表
 *
 * ReqID:TECC4.0 - 返回创建的哈希表指针，失败返回NULL
 */
static FeHashTable *
create_table_schema_hash_table(void)
{
    return fe_hash_create("TECC Table Schema Cache",  /* 表名 */
                          sizeof(TableSchemaHashKey), /* 键大小 */
                          sizeof(TableSchemaHashKey) + sizeof(TableSchemaCacheValue), /* 条目大小 */
                          hash_table_schema_key,     /* 哈希函数 */
                          compare_table_schema_keys, /* 比较函数 */
                          64);                       /* 初始大小 */
}

/*
 * create_kek_hash_table - 创建KEK哈希表
 *
 * 返回:
 *   创建的哈希表指针，失败返回NULL
 */
static FeHashTable *
create_kek_hash_table(void)
{
    /* 创建哈希表 */
    return fe_hash_create("TECC KEK Cache",  /* 表名 */
                          sizeof(KekHashKey), /* 键大小 */
                          sizeof(KekHashKey) + sizeof(TeccKekCacheItem *), /* 条目大小 */
                          hash_kek_key,      /* 哈希函数 */
                          compare_kek_keys,  /* 比较函数 */
                          100);              /* 初始大小 */
}

/*
 * create_dek_hash_table - 创建DEK哈希表
 *
 * 返回:
 *   创建的哈希表指针，失败返回NULL
 */
static FeHashTable *
create_dek_hash_table(void)
{
    /* 创建哈希表 */
    return fe_hash_create("TECC DEK Cache",  /* 表名 */
                          sizeof(DekHashKey), /* 键大小 */
                          sizeof(DekHashKey) + sizeof(TeccDekCacheItem *), /* 条目大小 */
                          hash_dek_key,      /* 哈希函数 */
                          compare_dek_keys,  /* 比较函数 */
                          100);              /* 初始大小 */
}

/*
 * create_column_hash_table - 创建列哈希表
 *
 * 返回:
 *   创建的哈希表指针，失败返回NULL
 */
static FeHashTable *
create_column_hash_table(void)
{
    /* 创建哈希表 */
    return fe_hash_create("TECC Column Cache",  /* 表名 */
                          sizeof(ColumnHashKey), /* 键大小 */
                          sizeof(ColumnHashKey) + sizeof(TeccColumnCacheItem *), /* 条目大小 */
                          hash_column_key,     /* 哈希函数 */
                          compare_column_keys, /* 比较函数 */
                          500);                /* 初始大小 */
}

/*
 * create_kek_name_hash_table - 创建KEK名称查找哈希表
 *
 * 返回:
 *   创建的哈希表指针，失败返回NULL
 */
static FeHashTable *
create_kek_name_hash_table(void)
{
    /* 创建名称查找哈希表 */
    return fe_hash_create("TECC KEK Name Cache",  /* 表名 */
                          sizeof(KekNameHashKey), /* 键大小 */
                          sizeof(KekNameHashKey) + sizeof(TeccKekCacheItem *), /* 条目大小 */
                          hash_kek_name_key,      /* 哈希函数 */
                          compare_kek_name_keys,  /* 比较函数 */
                          100);                   /* 初始大小 */
}

/*
 * create_dek_name_hash_table - 创建DEK名称查找哈希表
 *
 * 返回:
 *   创建的哈希表指针，失败返回NULL
 */
static FeHashTable *
create_dek_name_hash_table(void)
{
    /* 创建名称查找哈希表 */
    return fe_hash_create("TECC DEK Name Cache",  /* 表名 */
                          sizeof(DekNameHashKey), /* 键大小 */
                          sizeof(DekNameHashKey) + sizeof(TeccDekCacheItem *), /* 条目大小 */
                          hash_dek_name_key,      /* 哈希函数 */
                          compare_dek_name_keys,  /* 比较函数 */
                          100);                   /* 初始大小 */
}

/*
 * create_column_name_hash_table - 创建列名称查找哈希表
 *
 * 返回:
 *   创建的哈希表指针，失败返回NULL
 */
static FeHashTable *
create_column_name_hash_table(void)
{
    /* 创建名称查找哈希表 */
    return fe_hash_create("TECC Column Name Cache",  /* 表名 */
                          sizeof(ColumnNameHashKey), /* 键大小 */
                          sizeof(ColumnNameHashKey) + sizeof(TeccColumnCacheItem *), /* 条目大小 */
                          hash_column_name_key,     /* 哈希函数 */
                          compare_column_name_keys, /* 比较函数 */
                          500);                     /* 初始大小 */
}

/*
 * destroy_hash_table - 销毁哈希表
 *
 * 参数:
 *   hash_table - 哈希表指针
 */
static void
destroy_hash_table(FeHashTable *hash_table)
{
    if (hash_table != NULL) {
        fe_hash_destroy(hash_table);
    }
}

/*
 * clear_hash_table_data_pointers - 清空哈希表所有条目的data指针（置NULL）
 *
 * 用于名称缓存哈希表（dek_name_cache/kek_name_cache/column_name_cache），
 * 在主缓存已通过 destroy_all_*_items 释放了共享的 item 后，
 * 将名称缓存中悬空的 data 指针置 NULL，防止 fe_hash_destroy 双重释放。
 */
static void
clear_hash_table_data_pointers(FeHashTable *htab)
{
    FeHashSeqStatus status;
    void *entry;
    if (htab == NULL)
        return;
    fe_hash_seq_init(&status, htab);
    while ((entry = fe_hash_seq_search(&status)) != NULL)
    {
        *(void **)entry = NULL;
    }
}

/*
 * destroy_all_kek_items - 遍历KEK哈希表，释放所有KEK缓存项
 *
 * 释放后将哈希条目的data指针置NULL，防止后续fe_hash_destroy双重释放。
 */
static void
destroy_all_kek_items(FeHashTable *htab)
{
    FeHashSeqStatus status;
    void *entry;
    if (htab == NULL)
        return;
    fe_hash_seq_init(&status, htab);
    while ((entry = fe_hash_seq_search(&status)) != NULL)
    {
        TeccKekCacheItem *item = *(TeccKekCacheItem **)entry;
        if (item != NULL)
        {
            destroy_kek_cache_item(item);
            *(void **)entry = NULL;
        }
    }
}

/*
 * destroy_all_dek_items - 遍历DEK哈希表，释放所有DEK缓存项
 *
 * 释放后将哈希条目的data指针置NULL，防止后续fe_hash_destroy双重释放。
 */
static void
destroy_all_dek_items(FeHashTable *htab)
{
    FeHashSeqStatus status;
    void *entry;
    if (htab == NULL)
        return;
    fe_hash_seq_init(&status, htab);
    while ((entry = fe_hash_seq_search(&status)) != NULL)
    {
        TeccDekCacheItem *item = *(TeccDekCacheItem **)entry;
        if (item != NULL)
        {
            destroy_dek_cache_item(item);
            *(void **)entry = NULL;
        }
    }
}

/*
 * destroy_all_column_items - 遍历Column哈希表，释放所有Column缓存项
 *
 * 释放后将哈希条目的data指针置NULL，防止后续fe_hash_destroy双重释放。
 */
static void
destroy_all_column_items(FeHashTable *htab)
{
    FeHashSeqStatus status;
    void *entry;
    if (htab == NULL)
        return;
    fe_hash_seq_init(&status, htab);
    while ((entry = fe_hash_seq_search(&status)) != NULL)
    {
        TeccColumnCacheItem *item = *(TeccColumnCacheItem **)entry;
        if (item != NULL)
        {
            destroy_column_cache_item(item);
            *(void **)entry = NULL;
        }
    }
}

/*
 * tecc_cache_get_manager - 获取指定连接的缓存管理器
 *
 * 参数:
 *   conn - 数据库连接
 *
 * 返回:
 *   缓存管理器指针，未找到返回NULL
 */
TeccCacheManager *
tecc_cache_get_manager(const TeccState *state)
{
    return get_cache_manager(state);
}

/*
 * get_cache_manager - 获取缓存管理器（连接特定）
 *
 * 参数:
 *   state - TECC状态结构，包含连接信息
 *
 * 返回:
 *   缓存管理器指针，未找到返回NULL
 */
TeccCacheManager *
get_cache_manager(const TeccState *state)
{
    /* 参数检查 */
    if (state == NULL || state->conn == NULL) {
        return NULL;
    }

    /* 从TeccState获取缓存管理器 */
    if (state->metadata_cache != NULL) {
        TeccCacheManager *manager = (TeccCacheManager *)state->metadata_cache;
        /* ReqID:TECC4.0 - 增强验证：检查连接匹配、初始化状态和锁有效性 */
        if (manager->conn == state->conn &&
            manager->initialized &&
            manager->lock != NULL) {
            return manager;
        }
        /* ReqID:TECC4.0 - 缓存管理器状态异常告警，便于跟踪潜在的内存损坏问题 */
        TECC_CACHE_WARN("get_cache_manager: manager=%p INVALID "
            "(conn_match=%d, initialized=%d, lock=%p, load_count=%d) "
            "— possible memory corruption or stale cache manager",
            manager,
            (manager->conn == state->conn) ? 1 : 0,
            manager->initialized,
            manager->lock,
            manager->load_count);
    }

    return NULL;
}

/* ==================== 缓存项内存管理函数实现 ==================== */


/*
 * tecc_cache_free_kek_item - 释放KEK缓存项内存（公共接口）
 *
 * 参数:
 *   kek_item - KEK缓存项指针
 */
void
tecc_cache_free_kek_item(TeccKekCacheItem *kek_item)
{
    destroy_kek_cache_item(kek_item);
}

/*
 * tecc_cache_free_dek_item - 释放DEK缓存项内存（公共接口）
 *
 * 参数:
 *   dek_item - DEK缓存项指针
 */
void
tecc_cache_free_dek_item(TeccDekCacheItem *dek_item)
{
    destroy_dek_cache_item(dek_item);
}

/*
 * tecc_cache_free_column_item - 释放加密列缓存项内存（公共接口）
 *
 * 参数:
 *   column_item - 加密列缓存项指针
 */
void
tecc_cache_free_column_item(TeccColumnCacheItem *column_item)
{
    destroy_column_cache_item(column_item);
}

/*
 * tecc_cache_free_column_items - 批量释放加密列缓存项数组内存
 *
 * 参数:
 *   column_items - 加密列缓存项指针数组
 *   count - 数组元素数量
 */
void
tecc_cache_free_column_items(TeccColumnCacheItem **column_items, int count)
{
    if (column_items == NULL || count <= 0) {
        return;
    }

    {
        int i;
        for (i = 0; i < count; i++) {
            if (column_items[i] != NULL) {
                tecc_cache_free_column_item(column_items[i]);
            }
        }
    }

    /* 释放数组本身 */
    pg_free(column_items);
}

/* ==================== 错误恢复和重试机制实现 ==================== */

/* 全局可恢复错误掩码（线程安全访问） */
static TeccCacheRecoverableErrors recoverable_errors_mask = TECC_CACHE_RECOVERABLE_ALL;
static pthread_mutex_t recoverable_errors_lock = PTHREAD_MUTEX_INITIALIZER;

/* 简单随机数生成器，用于抖动计算（线程安全版本） */
static unsigned int simple_rand(unsigned int *seed)
{
    if (seed == NULL) {
        return 0;
    }
    *seed = *seed * 1103515245 + 12345;
    return (unsigned int)(*seed / 65536) % 32768;
}

/*
 * tecc_cache_is_recoverable_error - 检查错误是否可恢复
 *
 * 参数:
 *   state - Tecc状态（可为NULL，使用全局默认值）
 *   error_code - 错误码
 *
 * 返回:
 *   true表示可恢复，false表示不可恢复
 */
bool
tecc_cache_is_recoverable_error(const TeccState *state, int error_code)
{
    TeccCacheRecoverableErrors error_mask = 0;
    TeccCacheRecoverableErrors current_mask;

    /* 根据错误码确定对应的掩码位 */
    switch (error_code) {
        case TECC_CACHE_ERROR_CONNECTION:
            error_mask = TECC_CACHE_RECOVERABLE_CONNECTION;
            break;
        case TECC_CACHE_ERROR_QUERY:
            error_mask = TECC_CACHE_RECOVERABLE_QUERY;
            break;
        case TECC_CACHE_ERROR_TIMEOUT:
            error_mask = TECC_CACHE_RECOVERABLE_TIMEOUT;
            break;
        case TECC_CACHE_ERROR_PERMISSION:
            error_mask = TECC_CACHE_RECOVERABLE_PERMISSION;
            break;
        default:
            /* 其他错误默认不可恢复 */
            return false;
    }

    /* 检查该错误类型是否在可恢复掩码中 */
    if (state != NULL) {
        TeccCacheManager *manager = get_cache_manager(state);
        if (manager != NULL) {
            /* 使用连接特定的错误掩码 */
            return (manager->recoverable_errors_mask & error_mask) != 0;
        }
    }

    /* 如果没有连接状态，使用全局默认值（向后兼容） */
    pthread_mutex_lock(&recoverable_errors_lock);
    current_mask = recoverable_errors_mask;
    pthread_mutex_unlock(&recoverable_errors_lock);
    return (current_mask & error_mask) != 0;
}

/*
 * calculate_backoff_delay - 计算退避延迟时间
 *
 * 参数:
 *   attempt - 当前尝试次数（从0开始）
 *   config - 重试配置
 *   random_seed - 随机数种子指针（连接特定）
 *
 * 返回:
 *   延迟时间（毫秒）
 */
static int
calculate_backoff_delay(int attempt, const TeccCacheRetryConfig *config, unsigned int *random_seed)
{
    int delay;
    int jitter;

    if (config->exponential_backoff) {
        /* 指数退避：base_delay * 2^attempt */
        delay = config->base_delay_ms * (1 << attempt);
    } else {
        /* 线性退避：base_delay * (attempt + 1) */
        delay = config->base_delay_ms * (attempt + 1);
    }

    /* 添加抖动（如果有） */
    if (config->jitter_enabled && random_seed != NULL) {
        jitter = simple_rand(random_seed) % (delay / 4);  /* 最多25%的抖动 */
        delay += jitter;
    }

    /* 限制最大延迟 */
    if (delay > config->max_delay_ms) {
        delay = config->max_delay_ms;
    }

    return delay;
}

/*
 * tecc_cache_retry_with_recovery - 带恢复的重试机制
 *
 * 参数:
 *   state - Tecc状态
 *   operation - 要执行的操作函数
 *   operation_arg - 操作函数的参数
 *   config - 重试配置（可为NULL使用默认配置）
 *
 * 返回:
 *   操作结果错误码
 */
int
tecc_cache_retry_with_recovery(const TeccState *state,
                               int (*operation)(const TeccState *state, void *arg),
                               void *operation_arg,
                               const TeccCacheRetryConfig *config)
{
    int retry_count = 0;
    int result;
    TeccCacheRetryConfig default_config;
    const TeccCacheRetryConfig *retry_config;

    /* C90兼容的结构体初始化 */
    default_config.max_retries = TECC_CACHE_DEFAULT_MAX_RETRIES;
    default_config.base_delay_ms = TECC_CACHE_DEFAULT_BASE_DELAY_MS;
    default_config.max_delay_ms = TECC_CACHE_DEFAULT_MAX_DELAY_MS;
    default_config.exponential_backoff = TECC_CACHE_DEFAULT_EXPONENTIAL_BACKOFF;
    default_config.jitter_enabled = TECC_CACHE_DEFAULT_JITTER_ENABLED;
    retry_config = config ? config : &default_config;

    if (operation == NULL) {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    /* 首次尝试 */
    TECC_CACHE_DEBUG("【DEBUG】tecc_cache_retry_with_recovery: about to call operation, state=%p, arg=%p", (const void *)state, operation_arg);
    result = operation(state, operation_arg);
    TECC_CACHE_DEBUG("【DEBUG】tecc_cache_retry_with_recovery: operation returned %d", result);
    if (result == TECC_CACHE_OK) {
        TECC_CACHE_DEBUG("【DEBUG】tecc_cache_retry_with_recovery: operation OK, returning");
        return TECC_CACHE_OK;
    }

    /* 检查是否可恢复错误 */
    if (!tecc_cache_is_recoverable_error(state, result)) {
        TECC_CACHE_INFO("操作失败，错误不可恢复: %d", result);
        return result;
    }

    TECC_CACHE_INFO("操作失败，开始重试机制，错误码: %d", result);

    /* 重试循环 */
    while (retry_count < retry_config->max_retries) {
        int delay_ms;
        TeccCacheManager *manager = get_cache_manager(state);

        /* 计算延迟时间 */
        delay_ms = calculate_backoff_delay(retry_count, retry_config,
                                          manager ? &manager->random_seed : NULL);
        TECC_CACHE_INFO("第 %d 次重试，延迟 %d 毫秒", retry_count + 1, delay_ms);

        /* 等待延迟时间 */
        if (delay_ms > 0) {
#ifdef _WIN32
            Sleep((DWORD)delay_ms);
#else
            struct timespec ts;
            ts.tv_sec = delay_ms / 1000;
            ts.tv_nsec = (delay_ms % 1000) * 1000000;
            nanosleep(&ts, NULL);
#endif
        }

        /* 尝试恢复连接（如果是连接错误） */
        if (result == TECC_CACHE_ERROR_CONNECTION) {
            int recovery_result = tecc_cache_recover_connection(state);
            if (recovery_result != TECC_CACHE_OK) {
                TECC_CACHE_WARN("连接恢复失败，错误码: %d", recovery_result);
            }
        }

        /* 再次尝试操作 */
        result = operation(state, operation_arg);
        if (result == TECC_CACHE_OK) {
            TECC_CACHE_INFO("重试成功，在第 %d 次尝试后", retry_count + 1);

            /* 成功恢复后预热缓存 */
            tecc_cache_warmup_after_recovery(state);

            return TECC_CACHE_OK;
        }

        /* 检查错误是否仍然可恢复 */
        if (!tecc_cache_is_recoverable_error(state, result)) {
            TECC_CACHE_INFO("重试过程中错误变为不可恢复: %d", result);
            break;
        }

        retry_count++;
    }

    TECC_CACHE_ERROR("操作在 %d 次重试后仍然失败，最终错误码: %d",
                     retry_config->max_retries, result);
    return result;
}

/*
 * tecc_cache_recover_connection - 恢复数据库连接
 *
 * 参数:
 *   state - Tecc状态
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
int
tecc_cache_recover_connection(const TeccState *state)
{
    PGconn *conn = NULL;
    TeccCacheManager *manager = NULL;

    if (state == NULL) {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    /* 获取缓存管理器 */
    manager = get_cache_manager(state);
    if (manager == NULL) {
        return TECC_CACHE_ERROR_INTERNAL;
    }

    /* 获取数据库连接 */
    conn = manager->conn;
    if (conn == NULL) {
        return TECC_CACHE_ERROR_CONNECTION;
    }

    TECC_CACHE_INFO("尝试恢复数据库连接");

    /* 检查连接状态 */
    if (PQstatus(conn) != CONNECTION_OK) {
        TECC_CACHE_WARN("数据库连接已断开，尝试重新连接");

        /* 尝试重置连接 */
        PQreset(conn);

        /* 检查重置后的状态 */
        if (PQstatus(conn) != CONNECTION_OK) {
            TECC_CACHE_ERROR("数据库连接恢复失败");
            return TECC_CACHE_ERROR_CONNECTION;
        }
    }

    TECC_CACHE_INFO("数据库连接恢复成功");
    return TECC_CACHE_OK;
}

/*
 * tecc_cache_warmup_after_recovery - 恢复后预热缓存
 *
 * 参数:
 *   state - Tecc状态
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
int
tecc_cache_warmup_after_recovery(const TeccState *state)
{
    TeccCacheManager *manager = NULL;
    int result;

    if (state == NULL) {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    /* 获取缓存管理器 */
    manager = get_cache_manager(state);
    if (manager == NULL) {
        return TECC_CACHE_ERROR_INTERNAL;
    }

    TECC_CACHE_INFO("开始恢复后缓存预热");

    /* 刷新所有缓存 */
    result = tecc_cache_refresh(state, TECC_CACHE_REFRESH_ALL);
    if (result != TECC_CACHE_OK) {
        TECC_CACHE_WARN("缓存预热失败，错误码: %d", result);
        return result;
    }

    TECC_CACHE_INFO("缓存预热完成");
    return TECC_CACHE_OK;
}

/*
 * tecc_cache_set_recoverable_errors_mask - 设置可恢复错误掩码
 *
 * 参数:
 *   state - Tecc状态（可为NULL，设置全局默认值）
 *   mask - 可恢复错误掩码
 */
void
tecc_cache_set_recoverable_errors_mask(const TeccState *state, TeccCacheRecoverableErrors mask)
{
    if (state != NULL) {
        TeccCacheManager *manager = get_cache_manager(state);
        if (manager != NULL) {
            /* 设置连接特定的错误掩码 */
            manager->recoverable_errors_mask = mask;
            TECC_CACHE_INFO("设置连接特定可恢复错误掩码: 0x%02X", mask);
            return;
        }
    }

    /* 如果没有连接状态，设置全局默认值（向后兼容） */
    pthread_mutex_lock(&recoverable_errors_lock);
    recoverable_errors_mask = mask;
    pthread_mutex_unlock(&recoverable_errors_lock);
    TECC_CACHE_INFO("设置全局可恢复错误掩码: 0x%02X", mask);
}

/*
 * tecc_cache_get_recoverable_errors_mask - 获取可恢复错误掩码
 *
 * 参数:
 *   state - Tecc状态（可为NULL，获取全局默认值）
 *
 * 返回:
 *   当前可恢复错误掩码
 */
TeccCacheRecoverableErrors
tecc_cache_get_recoverable_errors_mask(const TeccState *state)
{
    TeccCacheRecoverableErrors mask;

    if (state != NULL) {
        TeccCacheManager *manager = get_cache_manager(state);
        if (manager != NULL) {
            /* 返回连接特定的错误掩码 */
            return manager->recoverable_errors_mask;
        }
    }

    /* 如果没有连接状态，返回全局默认值（向后兼容） */
    pthread_mutex_lock(&recoverable_errors_lock);
    mask = recoverable_errors_mask;
    pthread_mutex_unlock(&recoverable_errors_lock);
    return mask;
}

/* ==================== 异常状态自动恢复机制实现 ==================== */

/*
 * validate_cache_manager_state - 验证缓存管理器状态
 *
 * 参数:
 *   manager - 缓存管理器指针
 *
 * 返回:
 *   true表示状态有效，false表示状态异常
 */
static bool
validate_cache_manager_state(TeccCacheManager *manager)
{
    if (manager == NULL) {
        TECC_CACHE_DEBUG("缓存管理器指针为NULL");
        return false;
    }

    /* 加锁保护状态验证 */
    CACHE_RLOCK(manager);

    /* 检查初始化标志 */
    if (!manager->initialized) {
        TECC_CACHE_DEBUG("缓存管理器未初始化");
        CACHE_RWUNLOCK(manager);
        return false;
    }

    /* 检查数据库连接 */
    if (manager->conn == NULL) {
        TECC_CACHE_DEBUG("缓存管理器数据库连接为NULL");
        CACHE_RWUNLOCK(manager);
        return false;
    }

    /* 检查连接状态 */
    if (PQstatus(manager->conn) != CONNECTION_OK) {
        TECC_CACHE_DEBUG("缓存管理器数据库连接状态异常");
        CACHE_RWUNLOCK(manager);
        return false;
    }

    /* 检查哈希表指针 */
    if (manager->kek_cache == NULL || manager->dek_cache == NULL || manager->column_cache == NULL) {
        TECC_CACHE_DEBUG("缓存管理器哈希表指针异常");
        CACHE_RWUNLOCK(manager);
        return false;
    }

    /* 检查锁指针 */
    if (manager->lock == NULL) {
        TECC_CACHE_DEBUG("缓存管理器锁指针为NULL");
        CACHE_RWUNLOCK(manager);
        return false;
    }

    /* 状态验证通过 */
    CACHE_RWUNLOCK(manager);
    return true;
}

/*
 * recover_cache_manager_state - 恢复缓存管理器状态
 *
 * 参数:
 *   manager - 缓存管理器指针
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
static int
recover_cache_manager_state(TeccCacheManager *manager)
{
    if (manager == NULL) {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    /* 加锁保护状态恢复操作 */
    CACHE_WLOCK(manager);

    TECC_CACHE_INFO("开始恢复缓存管理器状态");

    /* 1. 恢复数据库连接 */
    if (manager->conn != NULL && PQstatus(manager->conn) != CONNECTION_OK) {
        TECC_CACHE_WARN("数据库连接异常，尝试恢复");
        PQreset(manager->conn);
        if (PQstatus(manager->conn) != CONNECTION_OK) {
            TECC_CACHE_ERROR("数据库连接恢复失败");
            CACHE_RWUNLOCK(manager);
            return TECC_CACHE_ERROR_CONNECTION;
        }
        TECC_CACHE_INFO("数据库连接恢复成功");
    }

    /* 2. 重新初始化哈希表（如果指针异常） */
    if (manager->kek_cache == NULL || manager->dek_cache == NULL || manager->column_cache == NULL) {
        TECC_CACHE_WARN("缓存哈希表异常，尝试重新创建");

        /* 清理现有哈希表 — 先安全释放item并清空data指针 */
        if (manager->kek_cache != NULL) {
            destroy_all_kek_items(manager->kek_cache);
            destroy_hash_table(manager->kek_cache);
            manager->kek_cache = NULL;
        }
        if (manager->dek_cache != NULL) {
            destroy_all_dek_items(manager->dek_cache);
            destroy_hash_table(manager->dek_cache);
            manager->dek_cache = NULL;
        }
        if (manager->column_cache != NULL) {
            destroy_all_column_items(manager->column_cache);
            destroy_hash_table(manager->column_cache);
            manager->column_cache = NULL;
        }

        /* 重新创建哈希表 */
        manager->kek_cache = create_kek_hash_table();
        manager->dek_cache = create_dek_hash_table();
        manager->column_cache = create_column_hash_table();

        if (manager->kek_cache == NULL || manager->dek_cache == NULL || manager->column_cache == NULL) {
            TECC_CACHE_ERROR("缓存哈希表重新创建失败");
            CACHE_RWUNLOCK(manager);
            return TECC_CACHE_ERROR_MEMORY;
        }

        TECC_CACHE_INFO("缓存哈希表重新创建成功");
    }

    /* 3. 重新初始化名称查找哈希表 */
    if (manager->kek_name_cache == NULL || manager->dek_name_cache == NULL || manager->column_name_cache == NULL) {
        TECC_CACHE_WARN("名称查找哈希表异常，尝试重新创建");

        /* 清理现有哈希表 — 先清空悬空data指针防止双重释放 */
        if (manager->kek_name_cache != NULL) {
            clear_hash_table_data_pointers(manager->kek_name_cache);
            destroy_hash_table(manager->kek_name_cache);
            manager->kek_name_cache = NULL;
        }
        if (manager->dek_name_cache != NULL) {
            clear_hash_table_data_pointers(manager->dek_name_cache);
            destroy_hash_table(manager->dek_name_cache);
            manager->dek_name_cache = NULL;
        }
        if (manager->column_name_cache != NULL) {
            clear_hash_table_data_pointers(manager->column_name_cache);
            destroy_hash_table(manager->column_name_cache);
            manager->column_name_cache = NULL;
        }

        /* 重新创建哈希表 */
        manager->kek_name_cache = create_kek_name_hash_table();
        manager->dek_name_cache = create_dek_name_hash_table();
        manager->column_name_cache = create_column_name_hash_table();

        if (manager->kek_name_cache == NULL || manager->dek_name_cache == NULL || manager->column_name_cache == NULL) {
            TECC_CACHE_ERROR("名称查找哈希表重新创建失败");
            CACHE_RWUNLOCK(manager);
            return TECC_CACHE_ERROR_MEMORY;
        }

        TECC_CACHE_INFO("名称查找哈希表重新创建成功");
    }

    /* 4. 重新初始化锁（如果异常） */
    /* 重新初始化读写锁 */
    pthread_rwlock_destroy(&manager->global_rwlock);
    if (pthread_rwlock_init(&manager->global_rwlock, NULL) != 0) {
        TECC_CACHE_ERROR("缓存读写锁重新创建失败：初始化错误");
        CACHE_RWUNLOCK(manager);
        return TECC_CACHE_ERROR_LOCK;
    }

    /* 重新初始化互斥锁（兼容性保留） */
    if (manager->lock == NULL) {
        TECC_CACHE_WARN("缓存互斥锁异常，尝试重新创建");
        manager->lock = pg_malloc(sizeof(pthread_mutex_t));
        if (manager->lock == NULL) {
            TECC_CACHE_ERROR("缓存互斥锁重新创建失败：内存不足");
            CACHE_RWUNLOCK(manager);
            return TECC_CACHE_ERROR_MEMORY;
        }

        if (pthread_mutex_init((pthread_mutex_t *)manager->lock, NULL) != 0) {
            TECC_CACHE_ERROR("缓存互斥锁重新创建失败：初始化错误");
            pg_free(manager->lock);
            manager->lock = NULL;
            CACHE_RWUNLOCK(manager);
            return TECC_CACHE_ERROR_LOCK;
        }

        TECC_CACHE_INFO("缓存互斥锁重新创建成功");
    } else {
        /* 互斥锁存在，重新初始化 */
        pthread_mutex_destroy((pthread_mutex_t *)manager->lock);
        if (pthread_mutex_init((pthread_mutex_t *)manager->lock, NULL) != 0) {
            TECC_CACHE_ERROR("缓存互斥锁重新初始化失败");
            CACHE_RWUNLOCK(manager);
            return TECC_CACHE_ERROR_LOCK;
        }
    }

    TECC_CACHE_INFO("缓存锁重新创建成功");

    /* 5. 重新加载GUC参数 */
    manager->guc_loaded = false;
    /* 注意：GUC参数加载需要TeccState，这里无法直接加载 */

    /* 6. 重置统计信息 */
    manager->current_size = 0;
    manager->cache_hits = 0;
    manager->cache_misses = 0;
    manager->name_cache_hits = 0;
    manager->name_cache_misses = 0;

    /* 7. 设置状态标志 */
    manager->initialized = true;

    /* 解锁 */
    CACHE_RWUNLOCK(manager);

    TECC_CACHE_INFO("缓存管理器状态恢复完成");
    return TECC_CACHE_OK;
}


/*
 * tecc_cache_auto_recover - 缓存模块自动恢复（公共接口）
 *
 * 参数:
 *   state - Tecc状态
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
int
tecc_cache_auto_recover(const TeccState *state)
{
    TeccCacheManager *manager = NULL;
    int result;

    if (state == NULL) {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    TECC_CACHE_INFO("开始缓存模块自动恢复");

    /* 获取缓存管理器 */
    manager = get_cache_manager(state);
    if (manager == NULL) {
        TECC_CACHE_DEBUG("无法获取缓存管理器，尝试重新初始化");
        /* 尝试重新初始化缓存 */
        result = tecc_cache_init((TeccState *)state);
        if (result != TECC_CACHE_OK) {
            TECC_CACHE_ERROR("缓存重新初始化失败，错误码: %d", result);
            return result;
        }
        TECC_CACHE_INFO("缓存重新初始化成功");
        return TECC_CACHE_OK;
    }

    /* 验证缓存管理器状态 */
    if (!validate_cache_manager_state(manager)) {
        TECC_CACHE_WARN("缓存管理器状态异常，开始恢复");
        result = recover_cache_manager_state(manager);
        if (result != TECC_CACHE_OK) {
            TECC_CACHE_ERROR("缓存管理器状态恢复失败，错误码: %d", result);
            return result;
        }
        TECC_CACHE_INFO("缓存管理器状态恢复成功");
    } else {
        TECC_CACHE_DEBUG("缓存管理器状态正常，无需恢复");
        result = TECC_CACHE_OK;
    }

    return result;
}

/* ==================== 安全内存清理函数实现 ==================== */

/*
 * tecc_cache_secure_clear_memory - 安全清理内存中的敏感数据
 *
 * 参数:
 *   ptr - 要清理的内存指针
 *   size - 要清理的内存大小
 */
void
tecc_cache_secure_clear_memory(void *ptr, size_t size)
{
    volatile unsigned char *vptr = (volatile unsigned char *)ptr;
    size_t i;

    if (ptr == NULL || size == 0) {
        return;
    }

    /* 多次清零确保数据被彻底清除 */
    for (i = 0; i < 3; i++) {
        memset((void *)vptr, 0, size);

        /* 使用volatile确保编译器不会优化清零操作 */
#ifdef _MSC_VER
        _ReadWriteBarrier();
#else
        asm volatile("" : : "r"(vptr) : "memory");
#endif
    }

    /* 最后设置一个非零值，防止某些内存分析工具恢复数据 */
    memset((void *)vptr, 0xFF, size);
    memset((void *)vptr, 0x00, size);

    TECC_CACHE_DEBUG("安全清理了 %zu 字节内存", size);
}

/*
 * tecc_cache_secure_clear_dek_key - 安全清理DEK加密密钥数据
 *
 * 参数:
 *   dek_item - DEK缓存项指针
 */
void
tecc_cache_secure_clear_dek_key(TeccDekCacheItem *dek_item)
{
    size_t len;
    size_t base_size;

    if (dek_item == NULL) {
        return;
    }

    TECC_CACHE_DEBUG("开始安全清理DEK密钥数据");

    /* 安全清理加密密钥数据 */
    if (dek_item->encrypted_key != NULL && dek_item->key_length > 0) {
        tecc_cache_secure_clear_memory(dek_item->encrypted_key, dek_item->key_length);
    }

    /* 安全清理字符串字段 */
    if (dek_item->key_name != NULL) {
        len = strlen(dek_item->key_name);
        tecc_cache_secure_clear_memory(dek_item->key_name, len + 1);
    }

    if (dek_item->key_namespace != NULL) {
        len = strlen(dek_item->key_namespace);
        tecc_cache_secure_clear_memory(dek_item->key_namespace, len + 1);
    }

    /* ReqID:TECC4.0 — 安全清理缓存的解密后DEK */
    if (dek_item->plain_dek != NULL && dek_item->plain_dek_length > 0) {
        tecc_cache_secure_clear_memory(dek_item->plain_dek, dek_item->plain_dek_length);
    }

    /* 安全清理整个结构体（除了指针字段） */
    base_size = sizeof(TeccDekCacheItem) - (3 * sizeof(void *));
    tecc_cache_secure_clear_memory(dek_item, base_size);

    TECC_CACHE_DEBUG("DEK密钥数据安全清理完成");
}

/*
 * tecc_cache_secure_clear_kek_metadata - 安全清理KEK元数据
 *
 * 参数:
 *   kek_item - KEK缓存项指针
 */
void
tecc_cache_secure_clear_kek_metadata(TeccKekCacheItem *kek_item)
{
    size_t len;
    size_t base_size;

    if (kek_item == NULL) {
        return;
    }

    TECC_CACHE_DEBUG("开始安全清理KEK元数据");

    /* 安全清理字符串字段（包含敏感信息如KMS路径） */
    if (kek_item->key_name != NULL) {
        len = strlen(kek_item->key_name);
        tecc_cache_secure_clear_memory(kek_item->key_name, len + 1);
    }

    if (kek_item->key_namespace != NULL) {
        len = strlen(kek_item->key_namespace);
        tecc_cache_secure_clear_memory(kek_item->key_namespace, len + 1);
    }

    if (kek_item->kms_path != NULL) {
        len = strlen(kek_item->kms_path);
        tecc_cache_secure_clear_memory(kek_item->kms_path, len + 1);
    }

    /* ReqID:TECC4.0 — 安全清理缓存的明文KEK */
    if (kek_item->plain_kek != NULL && kek_item->plain_kek_length > 0) {
        /* 防御性检查：plain_kek_length应为合理大小（典型KEK 16-64字节，最大不超过1MB） */
        if (kek_item->plain_kek_length > 1048576) {
            TECC_CACHE_DEBUG("KEK元数据清理跳过：plain_kek_length=%zu 超出合理范围",
                             kek_item->plain_kek_length);
        } else {
            tecc_cache_secure_clear_memory(kek_item->plain_kek, kek_item->plain_kek_length);
        }
    }

    /* 安全清理整个结构体（除了指针字段：key_name, key_namespace, kms_path, plain_kek） */
    base_size = sizeof(TeccKekCacheItem) - (4 * sizeof(void *));
    tecc_cache_secure_clear_memory(kek_item, base_size);

    TECC_CACHE_DEBUG("KEK元数据安全清理完成");
}

/* ==================== 缓存完整性验证函数实现 ==================== */

/*
 * tecc_cache_generate_hmac_key - 生成HMAC密钥
 *
 * 参数:
 *   state - TECC状态
 *   manager - 缓存管理器
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
int
tecc_cache_generate_hmac_key(const TeccState *state, TeccCacheManager *manager)
{
    if (state == NULL || manager == NULL) {
        TECC_CACHE_DEBUG("无效参数：state或manager为NULL");
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    TECC_CACHE_DEBUG("开始生成HMAC密钥");

    /* ReqID:TECC4.0 - 使用getrandom替代RAND_bytes避免OpenSSL死锁（Linux）
     * Windows MSVC：RAND_bytes在OpenSSL 3.0上观察到的死锁是Linux-specific问题 */
#if defined(_WIN32)
    if (RAND_bytes(manager->hmac_key, (int)sizeof(manager->hmac_key)) != 1) {
#else
    if (getrandom(manager->hmac_key, sizeof(manager->hmac_key), 0) !=
        (ssize_t)sizeof(manager->hmac_key)) {
#endif
        TECC_CACHE_ERROR("生成HMAC密钥失败：随机数生成错误");
        return TECC_CACHE_ERROR_SECURITY;
    }

    manager->integrity_enabled = true;
    TECC_CACHE_DEBUG("HMAC密钥生成成功，完整性验证已启用");

    return TECC_CACHE_OK;
}

/*
 * tecc_cache_sign_item - 为缓存项生成HMAC签名
 *
 * 参数:
 *   manager - 缓存管理器
 *   item - 缓存项指针
 *   item_type - 缓存项类型（1=KEK, 2=DEK, 3=Column）
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
int
tecc_cache_sign_item(TeccCacheManager *manager, void *item, int item_type)
{
    unsigned char *data_to_sign = NULL;
    size_t data_len = 0;
    time_t current_time;
    unsigned char *hmac_result = NULL;
    unsigned int hmac_len = 0;
    TeccKekCacheItem *kek_item;
    TeccDekCacheItem *dek_item;
    TeccColumnCacheItem *column_item;

    if (manager == NULL || item == NULL) {
        TECC_CACHE_DEBUG("无效参数：manager或item为NULL");
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }
    /* 验证item_type参数 */
    if (item_type < 1 || item_type > 3) {
        TECC_CACHE_DEBUG("无效参数：item_type超出范围（1-3）");
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    if (!manager->integrity_enabled) {
        /* 完整性验证未启用，直接返回成功 */
        return TECC_CACHE_OK;
    }

    TECC_CACHE_DEBUG("开始为缓存项生成HMAC签名（类型: %d）", item_type);

    /* 计算要签名的数据 */
    current_time = time(NULL);

    switch (item_type) {
        case 1: { /* KEK缓存项 */
            kek_item = (TeccKekCacheItem *)item;

            /* 计算数据长度：排除签名字段本身 */
            data_len = sizeof(TeccKekCacheItem) - sizeof(kek_item->hmac_signature) - sizeof(kek_item->signature_time);
            data_to_sign = (unsigned char *)kek_item;

            /* 设置签名时间 */
            kek_item->signature_time = current_time;
            break;
        }
        case 2: { /* DEK缓存项 */
            dek_item = (TeccDekCacheItem *)item;

            /* 计算数据长度：排除签名字段本身 */
            data_len = sizeof(TeccDekCacheItem) - sizeof(dek_item->hmac_signature) - sizeof(dek_item->signature_time);
            data_to_sign = (unsigned char *)dek_item;

            /* 设置签名时间 */
            dek_item->signature_time = current_time;
            break;
        }
        case 3: { /* 列缓存项 */
            column_item = (TeccColumnCacheItem *)item;

            /* 计算数据长度：排除签名字段本身 */
            data_len = sizeof(TeccColumnCacheItem) - sizeof(column_item->hmac_signature) - sizeof(column_item->signature_time);
            data_to_sign = (unsigned char *)column_item;

            /* 设置签名时间 */
            column_item->signature_time = current_time;
            break;
        }
        default:
            TECC_CACHE_ERROR("无效的缓存项类型: %d", item_type);
            return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    /* 生成HMAC-SHA256签名 */
    hmac_result = HMAC(EVP_sha256(),
                       manager->hmac_key, sizeof(manager->hmac_key),
                       data_to_sign, data_len,
                       NULL, &hmac_len);

    if (hmac_result == NULL || hmac_len != 32) {
        TECC_CACHE_ERROR("HMAC签名生成失败");
        return TECC_CACHE_ERROR_SECURITY;
    }

    /* 存储签名到缓存项 */
    switch (item_type) {
        case 1: {
            memcpy(kek_item->hmac_signature, hmac_result, 32);
            break;
        }
        case 2: {
            memcpy(dek_item->hmac_signature, hmac_result, 32);
            break;
        }
        case 3: {
            memcpy(column_item->hmac_signature, hmac_result, 32);
            break;
        }
    }

    TECC_CACHE_DEBUG("缓存项HMAC签名生成成功（类型: %d）", item_type);
    return TECC_CACHE_OK;
}

/*
 * tecc_cache_verify_item - 验证缓存项的HMAC签名
 *
 * 参数:
 *   manager - 缓存管理器
 *   item - 缓存项指针
 *   item_type - 缓存项类型（1=KEK, 2=DEK, 3=Column）
 *
 * 返回:
 *   0表示签名有效，非0表示签名无效或错误
 */
int
tecc_cache_verify_item(TeccCacheManager *manager, void *item, int item_type)
{
    unsigned char *data_to_verify = NULL;
    size_t data_len = 0;
    unsigned char stored_signature[32];
    unsigned char *hmac_result = NULL;
    unsigned int hmac_len = 0;
    time_t current_time;
    time_t signature_time = 0;
    TeccKekCacheItem *kek_item;
    TeccDekCacheItem *dek_item;
    TeccColumnCacheItem *column_item;

    if (manager == NULL || item == NULL) {
        TECC_CACHE_DEBUG("无效参数：manager或item为NULL");
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }
    /* 验证item_type参数 */
    if (item_type < 1 || item_type > 3) {
        TECC_CACHE_DEBUG("无效参数：item_type超出范围（1-3）");
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    if (!manager->integrity_enabled) {
        /* 完整性验证未启用，直接返回成功 */
        return TECC_CACHE_OK;
    }

    TECC_CACHE_DEBUG("开始验证缓存项HMAC签名（类型: %d）", item_type);

    switch (item_type) {
        case 1: { /* KEK缓存项 */
            kek_item = (TeccKekCacheItem *)item;

            /* 计算数据长度：排除签名字段本身 */
            data_len = sizeof(TeccKekCacheItem) - sizeof(kek_item->hmac_signature) - sizeof(kek_item->signature_time);
            data_to_verify = (unsigned char *)kek_item;

            /* 保存存储的签名 */
            memcpy(stored_signature, kek_item->hmac_signature, 32);
            break;
        }
        case 2: { /* DEK缓存项 */
            dek_item = (TeccDekCacheItem *)item;

            /* 计算数据长度：排除签名字段本身 */
            data_len = sizeof(TeccDekCacheItem) - sizeof(dek_item->hmac_signature) - sizeof(dek_item->signature_time);
            data_to_verify = (unsigned char *)dek_item;

            /* 保存存储的签名 */
            memcpy(stored_signature, dek_item->hmac_signature, 32);
            break;
        }
        case 3: { /* 列缓存项 */
            column_item = (TeccColumnCacheItem *)item;

            /* 计算数据长度：排除签名字段本身 */
            data_len = sizeof(TeccColumnCacheItem) - sizeof(column_item->hmac_signature) - sizeof(column_item->signature_time);
            data_to_verify = (unsigned char *)column_item;

            /* 保存存储的签名 */
            memcpy(stored_signature, column_item->hmac_signature, 32);
            break;
        }
        default:
            TECC_CACHE_ERROR("无效的缓存项类型: %d", item_type);
            return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    /* 生成HMAC-SHA256签名进行验证 */
    hmac_result = HMAC(EVP_sha256(),
                       manager->hmac_key, sizeof(manager->hmac_key),
                       data_to_verify, data_len,
                       NULL, &hmac_len);

    if (hmac_result == NULL || hmac_len != 32) {
        TECC_CACHE_ERROR("HMAC签名验证失败：计算错误");
        return TECC_CACHE_ERROR_SECURITY;
    }

    /* 比较签名 */
    if (memcmp(stored_signature, hmac_result, 32) != 0) {
        TECC_CACHE_ERROR("缓存项HMAC签名验证失败：签名不匹配（类型: %d）", item_type);
        return TECC_CACHE_ERROR_INTEGRITY;
    }

    /* 检查签名时间（防止重放攻击） */
    current_time = time(NULL);

    switch (item_type) {
        case 1:
            signature_time = kek_item->signature_time;
            break;
        case 2:
            signature_time = dek_item->signature_time;
            break;
        case 3:
            signature_time = column_item->signature_time;
            break;
    }

    /* 签名时间不能在未来，且不能太旧（超过24小时） */
    if (signature_time > current_time || (current_time - signature_time) > 86400) {
        TECC_CACHE_ERROR("缓存项HMAC签名验证失败：签名时间无效（类型: %d，签名时间: %ld，当前时间: %ld）",
                         item_type, signature_time, current_time);
        return TECC_CACHE_ERROR_INTEGRITY;
    }

    TECC_CACHE_DEBUG("缓存项HMAC签名验证成功（类型: %d）", item_type);
    return TECC_CACHE_OK;
}

/*
 * tecc_cache_enable_integrity - 启用缓存完整性验证
 *
 * 参数:
 *   state - TECC状态
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
int
tecc_cache_enable_integrity(const TeccState *state)
{
    TeccCacheManager *manager;
    int ret;

    manager = get_cache_manager(state);
    if (manager == NULL) {
        return TECC_CACHE_ERROR_NOT_INITIALIZED;
    }

    TECC_CACHE_DEBUG("启用缓存完整性验证");

    /* 生成HMAC密钥 */
    ret = tecc_cache_generate_hmac_key(state, manager);
    if (ret != TECC_CACHE_OK) {
        TECC_CACHE_ERROR("启用缓存完整性验证失败：无法生成HMAC密钥");
        return ret;
    }

    manager->integrity_enabled = true;
    TECC_CACHE_DEBUG("缓存完整性验证已启用");

    return TECC_CACHE_OK;
}

/*
 * tecc_cache_disable_integrity - 禁用缓存完整性验证
 *
 * 参数:
 *   state - TECC状态
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
int
tecc_cache_disable_integrity(const TeccState *state)
{
    TeccCacheManager *manager = get_cache_manager(state);
    if (manager == NULL) {
        return TECC_CACHE_ERROR_NOT_INITIALIZED;
    }

    TECC_CACHE_DEBUG("禁用缓存完整性验证");

    /* 安全清理HMAC密钥 */
    tecc_cache_secure_clear_memory(manager->hmac_key, sizeof(manager->hmac_key));
    manager->integrity_enabled = false;

    TECC_CACHE_DEBUG("缓存完整性验证已禁用");

    return TECC_CACHE_OK;
}

/*
 * check_timed_refresh_needed - 检查是否需要执行定时刷新
 *
 * 参数:
 *   manager - 缓存管理器
 *
 * 返回:
 *   true表示需要刷新，false表示不需要
 */
static bool
check_timed_refresh_needed(TeccCacheManager *manager)
{
    time_t current_time;

    if (manager == NULL || !manager->initialized) {
        return false;
    }

    /* 如果定时刷新未启用或间隔为0，则不需要刷新 */
    if (!manager->timed_refresh_enabled || manager->timed_refresh_interval_sec <= 0) {
        return false;
    }

    current_time = time(NULL);

    /* 检查是否超过刷新间隔 */
    if (difftime(current_time, manager->last_timed_refresh_time) >= manager->timed_refresh_interval_sec) {
        TECC_CACHE_DEBUG("定时刷新检查：需要刷新（间隔：%d秒，已过：%.0f秒）",
                        manager->timed_refresh_interval_sec,
                        difftime(current_time, manager->last_timed_refresh_time));
        return true;
    }

    return false;
}

/*
 * perform_timed_refresh_if_needed - 执行定时刷新（如果需要）
 *
 * 参数:
 *   manager - 缓存管理器
 */
void
perform_timed_refresh_if_needed(TeccCacheManager *manager)
{
    bool need_refresh = false;

    if (manager == NULL || !manager->initialized) {
        return;
    }

    /* 检查是否需要定时刷新 */
    need_refresh = check_timed_refresh_needed(manager);

    if (need_refresh) {
        CACHE_WLOCK(manager);

        /* 标记所有缓存需要刷新 */
        manager->kek_cache_needs_refresh = true;
        manager->dek_cache_needs_refresh = true;
        manager->column_cache_needs_refresh = true;

        /* 更新定时刷新时间 */
        manager->last_timed_refresh_time = time(NULL);

        CACHE_RWUNLOCK(manager);

        TECC_CACHE_DEBUG("定时刷新：已标记所有缓存需要刷新");
    }
}

/*
 * tecc_check_marks_via_fastpath - 通过快速路径检查缓存标记
 * 返回：0成功，-1失败（需要降级到SQL查询）
 */
static int
tecc_check_marks_via_fastpath(PGconn *conn, int *marks)
{
    PQArgBlock args[1];
    int result_buf;
    int result_len;
    PGresult *res;

    /* 调用快速路径函数 tecc_get_cache_marks_wrapper() */
    res = PQfn(conn, TECC_GET_CACHE_MARKS_FNID,
               &result_buf, &result_len, 1, args, 0);

    if (res != NULL && PQresultStatus(res) == PGRES_COMMAND_OK && result_len > 0) {
        *marks = result_buf;
        PQclear(res);
        return 0; /* 成功 */
    }

    /* 失败处理 */
    if (res != NULL) {
        PQclear(res);
    }
    return -1; /* 失败，需要降级 */
}

/*
 * tecc_clear_marks_via_fastpath - 通过快速路径清除标记
 */
static int
tecc_clear_marks_via_fastpath(PGconn *conn, int mask)
{
    PQArgBlock args[1];
    int result_buf;
    int result_len;
    PGresult *res;

    /* 设置参数：标记位掩码 */
    args[0].len = 4;
    args[0].isint = 1;
    args[0].u.integer = mask;

    /* 调用快速路径函数 tecc_clear_cache_marks_wrapper()
     * 注意：该函数返回void，result_is_int必须为0，
     * 否则PQfn会尝试将长度为0的void结果作为整数读取 */
    res = PQfn(conn, TECC_CLEAR_CACHE_MARKS_FNID,
               &result_buf, &result_len, 0, args, 1);

    if (res != NULL && PQresultStatus(res) == PGRES_COMMAND_OK) {
        PQclear(res);
        return 0; /* 成功 */
    }

    if (res != NULL) {
        PQclear(res);
    }
    return -1; /* 失败 */
}

/*
 * check_metadata_update_mark_sql - 降级到SQL查询的标记检查
 * 保持原有逻辑，用于快速路径失败时的降级
 *
 * qconn: 执行查询的连接。非管道模式为主连接，管道模式为临时连接。
 */
static int
check_metadata_update_mark_sql(const TeccState *state, PGconn *qconn)
{
    PGresult *res = NULL;
    int result = TECC_CACHE_OK;
    bool kek_updated = false;
    bool dek_updated = false;
    bool column_updated = false;
    bool any_updated = false;

    if (state == NULL || qconn == NULL) {
        TECC_CACHE_ERROR("检查元数据更新标记失败：状态或连接为NULL");
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    TECC_CACHE_DEBUG("使用SQL查询检查TECC元数据更新标记");

    /* 一次查询获取三个类型标记状态 */
    res = PQexec(qconn,
                 "SELECT hg_is_kek_updated(), hg_is_dek_updated(), hg_is_column_updated()");

    if (res == NULL || PQresultStatus(res) != PGRES_TUPLES_OK) {
        TECC_CACHE_ERROR("检查元数据更新标记失败：%s", res ? PQerrorMessage(qconn) : "(PQexec NULL)");
        if (res) PQclear(res);
        return TECC_CACHE_ERROR_QUERY;
    }

    /* 解析三个标记结果 */
    if (PQntuples(res) > 0) {
        if (!PQgetisnull(res, 0, 0))
            kek_updated = strcmp(PQgetvalue(res, 0, 0), "t") == 0;
        if (!PQgetisnull(res, 0, 1))
            dek_updated = strcmp(PQgetvalue(res, 0, 1), "t") == 0;
        if (!PQgetisnull(res, 0, 2))
            column_updated = strcmp(PQgetvalue(res, 0, 2), "t") == 0;
    }
    PQclear(res);
    res = NULL;

    any_updated = kek_updated || dek_updated || column_updated;

    if (any_updated) {
        TECC_CACHE_DEBUG("TECC元数据已更新 (kek=%d, dek=%d, column=%d)，选择性刷新缓存",
                         kek_updated, dek_updated, column_updated);

        if (kek_updated) {
            result = tecc_cache_refresh(state, TECC_CACHE_REFRESH_KEK);
            if (result != TECC_CACHE_OK)
                TECC_CACHE_DEBUG("KEK缓存刷新失败");
        }

        if (dek_updated) {
            result = tecc_cache_refresh(state, TECC_CACHE_REFRESH_DEK);
            if (result != TECC_CACHE_OK)
                TECC_CACHE_DEBUG("DEK缓存刷新失败");
        }

        if (column_updated) {
            result = tecc_cache_refresh(state, TECC_CACHE_REFRESH_COLUMN);
            if (result != TECC_CACHE_OK)
                TECC_CACHE_DEBUG("加密列缓存刷新失败");
        }

        /* 清除所有标记 */
        res = PQexec(qconn,
                     "SELECT hg_clear_kek_mark(), hg_clear_dek_mark(), hg_clear_column_mark()");
        if (res == NULL || PQresultStatus(res) != PGRES_TUPLES_OK) {
            TECC_CACHE_DEBUG("清除元数据更新标记失败：%s", res ? PQerrorMessage(qconn) : "(PQexec NULL)");
        }
        if (res) PQclear(res);
        res = NULL;

        result = TECC_CACHE_OK;
    } else {
        TECC_CACHE_DEBUG("TECC元数据未更新");
    }

    return result;
}

/*
 * check_metadata_update_mark - 检查TECC元数据更新标记
 *
 * 这个函数检查后端类型更新标记（KEK、DEK、加密列），如果标记为true则选择性刷新缓存。
 * 优先使用快速路径调用，失败时降级到SQL查询。
 *
 * 参数:
 *   state - TECC状态
 *
 * 返回:
 *   TECC_CACHE_OK - 成功（无论是否需要刷新）
 *   TECC_CACHE_ERROR - 失败
 */
int
check_metadata_update_mark(const TeccState *state)
{
    int marks = 0;
    bool kek_updated = false;
    bool dek_updated = false;
    bool column_updated = false;
    bool any_updated = false;
    int result = TECC_CACHE_OK;
    TeccCacheManager *cache_mgr = NULL;
    PGconn *qconn = NULL;       /* ReqID:TECC4.0 - 管道模式下用独立连接 */
    bool use_temp_conn = false; /* ReqID:TECC4.0 */

    if (state == NULL || state->conn == NULL) {
        TECC_CACHE_ERROR("检查元数据更新标记失败：状态或连接为NULL");
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    /* 防止递归调用：外层PQsendQueryStart已设置check_in_progress，此处做防御性检查 */
    cache_mgr = get_cache_manager(state);
    if (cache_mgr != NULL && cache_mgr->check_in_progress)
    {
        TECC_CACHE_DEBUG("检查元数据更新标记：检测到递归调用，跳过本次检查");
        return TECC_CACHE_OK;
    }
    /* 不需要设置check_in_progress，由调用方（PQsendQueryStart）负责 */

    /* ReqID:TECC4.0 - 连接选择：非管道模式用主连接（行为不变），
     * 管道模式主连接禁止同步 PQexec/PQfn，改用 data_fetcher 独立连接。 */
    if (state->conn->pipelineStatus != PQ_PIPELINE_OFF)
    {
        qconn = tecc_data_fetcher_get_connection(state);
        if (qconn == NULL)
        {
            TECC_CACHE_ERROR("检查元数据更新标记失败：管道模式下获取临时连接失败");
            return TECC_CACHE_ERROR_CONNECTION;
        }
        use_temp_conn = true;
    }
    else
    {
        qconn = state->conn;
    }

    TECC_CACHE_DEBUG("开始检查TECC元数据更新标记（优先使用快速路径）");

    /* 第一步：尝试快速路径 */
    if (tecc_check_marks_via_fastpath(qconn, &marks) == 0) {
        /* 解析位掩码 */
        kek_updated = (marks & TECC_MARK_KEK) != 0;
        dek_updated = (marks & TECC_MARK_DEK) != 0;
        column_updated = (marks & TECC_MARK_COLUMN) != 0;
        any_updated = kek_updated || dek_updated || column_updated;

        if (any_updated) {
            TECC_CACHE_DEBUG("快速路径：TECC元数据已更新 (kek=%d, dek=%d, column=%d, mask=0x%02x)，选择性刷新缓存",
                             kek_updated, dek_updated, column_updated, marks);

            /* 选择性刷新缓存 */
            if (kek_updated) {
                result = tecc_cache_refresh(state, TECC_CACHE_REFRESH_KEK);
                if (result != TECC_CACHE_OK)
                    TECC_CACHE_DEBUG("KEK缓存刷新失败");
            }

            if (dek_updated) {
                result = tecc_cache_refresh(state, TECC_CACHE_REFRESH_DEK);
                TECC_CACHE_DEBUG("【DEBUG】check_metadata_update_mark: tecc_cache_refresh(DEK) returned %d", result);
                if (result != TECC_CACHE_OK)
                    TECC_CACHE_DEBUG("DEK缓存刷新失败");
            }

            if (column_updated) {
                result = tecc_cache_refresh(state, TECC_CACHE_REFRESH_COLUMN);
                if (result != TECC_CACHE_OK)
                    TECC_CACHE_DEBUG("加密列缓存刷新失败");

                				/* PENDING sentinels cleaned up with cache */            }

            /* 批量清除标记 */
            if (tecc_clear_marks_via_fastpath(qconn, marks) != 0) {
                /* 清除失败，降级到SQL清除 */
                PGresult *res = PQexec(qconn,
                    "SELECT hg_clear_kek_mark(), hg_clear_dek_mark(), hg_clear_column_mark()");
                if (res != NULL) {
                    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
                        TECC_CACHE_DEBUG("清除元数据更新标记失败：%s", PQerrorMessage(qconn));
                    }
                    PQclear(res);
                }
            }
        } else {
            TECC_CACHE_DEBUG("快速路径：TECC元数据未更新 (mask=0x%02x)", marks);
        }

        goto done;
    }

    /* 第二步：快速路径失败，降级到SQL查询 */
    TECC_CACHE_DEBUG("快速路径失败，降级到SQL查询");
    result = check_metadata_update_mark_sql(state, qconn);

done:
    /* ReqID:TECC4.0 - 管道模式下释放临时连接 */
    if (use_temp_conn)
        tecc_data_fetcher_release_connection(state, qconn);
    TECC_CACHE_DEBUG("【DEBUG】check_metadata_update_mark: completed, result=%d, kek_updated=%d, dek_updated=%d, column_updated=%d",
                     result, kek_updated, dek_updated, column_updated);
    return result;
}

/*
 * should_check_metadata_mark - 判断是否需要检查元数据标记
 *
 * 基于配置频率决定是否检查标记，避免频繁查询。
 * 实现指数退避机制。
 *
 * 参数:
 *   manager - 缓存管理器
 *
 * 返回:
 *   true - 需要检查
 *   false - 不需要检查
 */
bool
should_check_metadata_mark(const TeccCacheManager *manager)
{
    time_t current_time;
    time_t last_check_time;
    int check_interval;

    if (manager == NULL || !manager->initialized) {
        return false;
    }

    current_time = time(NULL);
    last_check_time = manager->last_metadata_check_time;
    check_interval = manager->metadata_check_interval_sec;

    /* 如果从未检查过，需要检查 */
    if (last_check_time == 0) {
        return true;
    }

    /* 检查是否超过检查间隔 */
    if (difftime(current_time, last_check_time) >= check_interval) {
        return true;
    }

    return false;
}

/*
 * tecc_cache_parse_search_path - 解析search_path字符串为schema列表
 *
 * 将PG的search_path格式（如 "\"$user\", public"）解析为有序的schema名称数组。
 * 处理 $user 替换、引号剥离等。
 */
void
tecc_cache_parse_search_path(TeccCacheManager *manager, PGconn *conn)
{
    char *search_path_str;
    char **schemas = NULL;
    int count = 0;
    int cap = 4;
    const char *user_name = NULL;

    if (manager == NULL)
        return;

    CACHE_LOCK(manager);

    /* 释放旧列表 */
    if (manager->guc_params.search_path_schemas != NULL)
    {
        int k;
        for (k = 0; k < manager->guc_params.search_path_schema_count; k++)
            pg_free(manager->guc_params.search_path_schemas[k]);
        pg_free(manager->guc_params.search_path_schemas);
        manager->guc_params.search_path_schemas = NULL;
        manager->guc_params.search_path_schema_count = 0;
    }

    search_path_str = manager->guc_params.search_path;
    if (search_path_str == NULL || search_path_str[0] == '\0')
    {
        CACHE_UNLOCK(manager);
        return;
    }

    /* 获取用户名用于$user替换 */
    if (conn != NULL)
        user_name = PQuser(conn);

    schemas = (char **) pg_malloc(sizeof(char *) * cap);
    if (schemas == NULL)
    {
        CACHE_UNLOCK(manager);
        return;
    }

    {
        const char *p = search_path_str;
        const char *start;
        char *schema_name;

        while (*p)
        {
            /* 跳过空白和逗号 */
            while (*p == ' ' || *p == '\t' || *p == ',')
                p++;
            if (*p == '\0')
                break;

            start = p;
            if (*p == '"')
            {
                /* 引号包裹的schema名 */
                p++;
                start = p;
                while (*p && *p != '"')
                    p++;
                if (*p == '"')
                {
                    int len = (int)(p - start);
                    schema_name = (char *) pg_malloc(len + 1);
                    if (schema_name != NULL)
                    {
                        memcpy(schema_name, start, len);
                        schema_name[len] = '\0';
                    }
                    p++; /* 跳过结束引号 */
                }
                else
                {
                    /* 未闭合引号，使用到字符串末尾 */
                    schema_name = pg_strdup(start);
                }
            }
            else
            {
                /* 无引号schema名 */
                while (*p && *p != ',' && *p != ' ' && *p != '\t')
                    p++;
                {
                    int len = (int)(p - start);
                    if (len > 0)
                    {
                        schema_name = (char *) pg_malloc(len + 1);
                        if (schema_name != NULL)
                        {
                            memcpy(schema_name, start, len);
                            schema_name[len] = '\0';
                        }
                    }
                    else
                    {
                        schema_name = NULL;
                    }
                }
            }

            if (schema_name != NULL)
            {
                /* 处理$user替换 */
                if (strcmp(schema_name, "$user") == 0)
                {
                    pg_free(schema_name);
                    if (user_name != NULL && user_name[0] != '\0')
                    {
                        schema_name = pg_strdup(user_name);
                    }
                    else
                    {
                        continue; /* 无法解析$user，跳过 */
                    }
                }

                /* 扩容 */
                if (count >= cap)
                {
                    cap *= 2;
                    schemas = (char **) pg_realloc(schemas, sizeof(char *) * cap);
                    if (schemas == NULL)
                    {
                        CACHE_UNLOCK(manager);
                        return;
                    }
                }
                schemas[count++] = schema_name;
            }
        }
    }

    manager->guc_params.search_path_schemas = schemas;
    manager->guc_params.search_path_schema_count = count;
    CACHE_UNLOCK(manager);
}

/*
 * tecc_cache_get_search_path_schemas - 获取搜索路径schema列表
 *
 * 在返回前检查PQparameterStatus，如果search_path发生变化则重新解析。
 */
const char **
tecc_cache_get_search_path_schemas(const TeccState *state, int *count)
{
    TeccCacheManager *manager;

    if (state == NULL || count == NULL)
        return NULL;

    manager = (TeccCacheManager *)state->metadata_cache;
    if (manager == NULL)
    {
        *count = 0;
        return NULL;
    }

    /* 在返回前同步search_path，确保schema列表是最新的 */
    sync_guc_params_from_live_conn(state, manager);

    CACHE_RLOCK(manager);
    *count = manager->guc_params.search_path_schema_count;
    CACHE_RWUNLOCK(manager);

    return (const char **)manager->guc_params.search_path_schemas;
}

/*
 * tecc_cache_get_table_schema - 解析未限定表名在 search_path 下的真实 schema
 *
 * 优先查缓存（table→schema 映射），未命中时经数据获取器独立连接查 pg_class，
 * 结果回填缓存。search_path 变化时映射缓存整体重建（sync/update 中处理）。
 *
 * 返回:
 *   找到：缓存内 schema 名指针（语句处理期间有效，search_path 变更会失效重建）；
 *   表不在 search_path 任何 schema 或查询失败：NULL（查询失败不写负缓存）。
 */
const char *
tecc_cache_get_table_schema(const TeccState *state,
                            const char *database_name,
                            const char *table_name)
{
    TeccCacheManager *manager;
    TableSchemaHashKey nk;
    TableSchemaCacheValue *val;
    const char **schemas;
    int nschemas;
    char *resolved = NULL;

    /* 参数检查 */
    if (state == NULL || database_name == NULL || table_name == NULL)
        return NULL;

    manager = (TeccCacheManager *)state->metadata_cache;
    if (manager == NULL)
        return NULL;

    /* 1. 命中缓存直接返回 */
    memset(&nk, 0, sizeof(nk));
    strncpy(nk.database_name, database_name, NAMEDATALEN - 1);
    strncpy(nk.table_name, table_name, NAMEDATALEN - 1);

    CACHE_RLOCK(manager);
    if (manager->table_schema_cache != NULL)
    {
        val = (TableSchemaCacheValue *)fe_hash_search(manager->table_schema_cache,
                                                      &nk, HASH_FIND, NULL);
        if (val != NULL)
        {
            const char *result = val->not_found ? NULL : val->schema_name;
            CACHE_RWUNLOCK(manager);
            return result;
        }
    }
    CACHE_RWUNLOCK(manager);

    /* 2. 未命中：取 search_path 解析列表（内部同步最新值） */
    schemas = tecc_cache_get_search_path_schemas(state, &nschemas);
    if (schemas == NULL || nschemas <= 0)
        return NULL;

    /* 3. 服务器解析（池连接，按 search_path 顺序） */
    if (tecc_fetch_resolve_table_schema(state, database_name, table_name,
                                        schemas, nschemas, &resolved) != TECC_FETCHER_OK)
    {
        /* 查询失败：不写负缓存，返回 NULL 由调用方 fallback */
        return NULL;
    }

    /* 4. 回填缓存（找到写 schema，未找到写 not_found 哨兵） */
    CACHE_LOCK(manager);
    if (manager->table_schema_cache != NULL)
    {
        val = (TableSchemaCacheValue *)fe_hash_search(manager->table_schema_cache,
                                                      &nk, HASH_ENTER, NULL);
        if (val != NULL)
        {
            if (resolved != NULL)
            {
                strncpy(val->schema_name, resolved, NAMEDATALEN - 1);
                val->schema_name[NAMEDATALEN - 1] = '\0';
                val->not_found = false;
            }
            else
            {
                val->schema_name[0] = '\0';
                val->not_found = true;
            }
        }
    }
    CACHE_UNLOCK(manager);

    if (resolved != NULL)
        pg_free(resolved);

    /* 返回刚回填的缓存值（语句处理期间有效，search_path 变更会整体重建失效） */
    if (val != NULL && !val->not_found)
        return val->schema_name;
    return NULL;
}

/*
 * tecc_cache_load_all_keys - 批量加载所有KEK/DEK到缓存
 *
 * 在缓存初始化后调用，预加载全部密钥元数据，
 * 避免后续语句触发逐条DB查询。
 * 使用数据获取器批量接口，仅消耗一个池连接。
 */
static int
tecc_cache_load_all_keys(TeccState *state)
{
    TeccCacheQueryParams query;
    TeccKekCacheItem **kek_items = NULL;
    TeccDekCacheItem **dek_items = NULL;
    int count = 0;
    int i;
    TeccFetcherErrorCode ret;
    const TeccState *cstate = (const TeccState *)state;

    /* 1. 批量加载KEK */
    memset(&query, 0, sizeof(query));
    query.flags = TECC_QUERY_BATCH;
    query.params.batch.filter = NULL;  /* 所有namespace */

    ret = tecc_fetch_kek_for_cache(cstate, &query, (TeccKekCacheItem **)&kek_items, &count);
    if (ret == TECC_FETCHER_OK && kek_items != NULL && count > 0)
    {
        for (i = 0; i < count; i++)
        {
            if (kek_items[i] != NULL)
            {
                int insert_ret = tecc_cache_insert_kek_item(state, kek_items[i]);
                if (insert_ret != TECC_CACHE_OK)
                {
                    tecc_cache_free_kek_item(kek_items[i]);
                }
            }
        }
        pg_free(kek_items);
    }
    else if (kek_items != NULL)
    {
        for (i = 0; i < count; i++)
        {
            if (kek_items[i] != NULL)
                tecc_cache_free_kek_item(kek_items[i]);
        }
        pg_free(kek_items);
    }

    /* 2. 批量加载DEK */
    count = 0;
    memset(&query, 0, sizeof(query));
    query.flags = TECC_QUERY_BATCH;
    query.params.batch.filter = NULL;

    ret = tecc_fetch_dek_for_cache(cstate, &query, (TeccDekCacheItem **)&dek_items, &count);
    if (ret == TECC_FETCHER_OK && dek_items != NULL && count > 0)
    {
        for (i = 0; i < count; i++)
        {
            if (dek_items[i] != NULL)
            {
                int insert_ret = tecc_cache_insert_dek_item(state, dek_items[i]);
                if (insert_ret != TECC_CACHE_OK)
                {
                    tecc_cache_free_dek_item(dek_items[i]);
                }
            }
        }
        pg_free(dek_items);
    }
    else if (dek_items != NULL)
    {
        for (i = 0; i < count; i++)
        {
            if (dek_items[i] != NULL)
                tecc_cache_free_dek_item(dek_items[i]);
        }
        pg_free(dek_items);
    }

    return TECC_CACHE_OK;
}

/* End - ReqID:TECC4.0 */