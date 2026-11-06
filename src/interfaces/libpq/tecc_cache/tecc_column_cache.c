/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_column_cache.c
 * Abstract:
 *   TECC加密列缓存实现，基于简化设计的两层架构。
 *   提供加密列相关的缓存查询和管理功能。
 *
 * Authored by <Claude Code>, 2026-04-15.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_cache/tecc_column_cache.c
 */

#include "postgres_fe.h"
#include "tecc_metadata_cache.h"
#include "tecc_data_fetcher_adapter.h"
#include "../tecc_data_fetcher/include/tecc_permission_check.h"
#include "../tecc_libpq_integration/fe-tecc-integration.h"
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#define TECC_ATTR_UNUSED
#else
#define TECC_ATTR_UNUSED __attribute__((unused))
#endif

/* 内部函数声明 */
static TeccColumnCacheItem *create_column_cache_item(void) TECC_ATTR_UNUSED;
static void destroy_column_cache_item(TeccColumnCacheItem *item);
static TeccColumnCacheItem *find_column_in_cache(const TeccState *state, Oid table_oid, int column_attnum);
static TeccColumnCacheItem *find_column_in_cache_by_name(const TeccState *state, const char *database_name,
                                                         const char *schema_name,
                                                         const char *table_name,
                                                         const char *column_name) TECC_ATTR_UNUSED;
static int insert_column_into_cache(const TeccState *state, TeccColumnCacheItem *item);
static int load_column_from_database(const TeccState *state, Oid table_oid, int column_attnum, TeccColumnCacheItem **item) TECC_ATTR_UNUSED;
static int load_column_from_database_by_name(const TeccState *state,
                                             const char *database_name,
                                             const char *schema_name,
                                             const char *table_name,
                                             const char *column_name,
                                             TeccColumnCacheItem **item) TECC_ATTR_UNUSED;
static int load_all_columns_from_database(const TeccState *state, Oid table_oid, TeccColumnCacheItem ***items, int *count);
static int load_all_columns_from_database_by_name(const TeccState *state,
                                                  const char *database_name,
                                                  const char *schema_name,
                                                  const char *table_name,
                                                  TeccColumnCacheItem ***items,
                                                  int *count);

/* 表级负缓存哨兵函数声明 */
static bool name_sentinel_exists(const TeccState *state,
                                 const char *database_name,
                                 const char *schema_name,
                                 const char *table_name);
static bool oid_sentinel_exists(const TeccState *state, Oid table_oid);
static int insert_name_sentinel(const TeccState *state,
                                const char *database_name,
                                const char *schema_name,
                                const char *table_name,
                                TeccCacheItemStatus sentinel_status);
static int insert_oid_sentinel(const TeccState *state, Oid table_oid);

/* 辅助函数声明（在其他文件中定义） */
/* get_cache_manager函数在其他缓存文件中定义，已在头文件中声明 */

/*
 * tecc_cache_get_encrypted_column - 根据表OID和列号查询加密列信息
 *
 * 参数:
 *   state - TECC状态结构，包含连接和上下文信息
 *   table_oid - 表对象标识符
 *   column_attnum - 列号（从1开始）
 *
 * 返回:
 *   加密列缓存项指针，NULL表示列未加密或不存在
 */
TeccColumnCacheItem *
tecc_cache_get_encrypted_column(const TeccState *state, Oid table_oid, int column_attnum)
{
    TeccCacheManager *manager = NULL;
    TeccColumnCacheItem *cached_item = NULL;
    TeccColumnCacheItem *new_item = NULL;
    int ret;
    bool auto_recovery_attempted = false;
    TeccCacheQueryParams query;
    TeccFetcherErrorCode fetcher_error;

    /* 参数检查 */
    if (state == NULL) {
        TECC_CACHE_DEBUG("无效参数：state为NULL");
        return NULL;
    }
    if (table_oid == InvalidOid) {
        TECC_CACHE_DEBUG("无效参数：table_oid为InvalidOid");
        return NULL;
    }
    if (column_attnum <= 0) {
        TECC_CACHE_DEBUG("无效参数：column_attnum必须大于0");
        return NULL;
    }

retry_with_recovery:
    /* 1. 获取缓存管理器 */
    manager = get_cache_manager(state);
    if (manager == NULL) {
        /* 如果管理器为NULL且未尝试过自动恢复，尝试恢复 */
        if (!auto_recovery_attempted) {
            auto_recovery_attempted = true;
            TECC_CACHE_DEBUG("缓存管理器为NULL，尝试自动恢复");
            if (tecc_cache_auto_recover(state) == TECC_CACHE_OK) {
                goto retry_with_recovery;
            }
        }
        return NULL;
    }

    /* 2. 检查并执行定时刷新（如果需要） */
    perform_timed_refresh_if_needed(manager);

    /* 3. 检查加密列缓存是否需要刷新并加锁 */
    CACHE_LOCK(manager);
    if (manager->column_cache_needs_refresh) {
        CACHE_UNLOCK(manager);
        /* 执行延迟刷新：获取写锁并清空缓存 */
        CACHE_WLOCK(manager);
        if (manager->column_cache_needs_refresh) {
            int clear_ret;

            TECC_CACHE_DEBUG("执行加密列缓存延迟刷新");

            // 调用缓存清理函数
            clear_ret = tecc_cache_clear_specific(manager, TECC_CACHE_REFRESH_COLUMN);
            if (clear_ret != TECC_CACHE_OK) {
                TECC_CACHE_DEBUG("加密列缓存清理失败，错误码: %d", clear_ret);
                /* 清理失败，但仍然重置标记避免无限重试 */
                manager->column_cache_needs_refresh = false;
            }

            TECC_CACHE_DEBUG("加密列缓存延迟刷新完成");
        }
        CACHE_RWUNLOCK(manager);
        /* 重新获取锁继续查询 */
        CACHE_LOCK(manager);
    }

    /* 3. 检查缓存命中 */
    cached_item = find_column_in_cache(state, table_oid, column_attnum);
    if (cached_item != NULL) {
        /* 更新命中次数 */
        cached_item->hit_times++;
        /* 解锁并返回结果 */
        CACHE_UNLOCK(manager);
        return cached_item;
    }

    /* 5. 缓存未命中，解锁以便进行数据库查询（避免长时间持有锁） */
    CACHE_UNLOCK(manager);

    /* 5a. 检查OID哨兵 — 该表已确认无此列 */
    if (oid_sentinel_exists(state, table_oid)) {
        return NULL;
    }

    /* 6. 缓存未命中时调用简化的适配器函数 */
    memset(&query, 0, sizeof(query));
    query.flags = TECC_QUERY_BY_OID;
    query.params.by_oid.oid = table_oid;
    query.params.by_oid.attnum = column_attnum;

    fetcher_error = tecc_fetch_column_for_cache(state, &query, &new_item, NULL);
    if (fetcher_error != TECC_FETCHER_OK || new_item == NULL) {
        /* 查询失败或列未加密 */
        ret = tecc_cache_convert_error(fetcher_error);
        /* 如果是可恢复错误且未尝试过自动恢复，尝试恢复 */
        if (!auto_recovery_attempted && tecc_cache_is_recoverable_error(state, ret)) {
            auto_recovery_attempted = true;
            TECC_CACHE_DEBUG("数据库查询失败（错误码: %d），尝试自动恢复", ret);
            if (tecc_cache_auto_recover(state) == TECC_CACHE_OK) {
                goto retry_with_recovery;
            }
        }
        return NULL;
    }

    /* 7. 加锁插入缓存 */
    CACHE_LOCK(manager);

    /* 8. 再次检查缓存（防止其他线程已插入） */
    cached_item = find_column_in_cache(state, table_oid, column_attnum);
    if (cached_item != NULL) {
        /* 其他线程已插入，使用现有缓存项 */
        destroy_column_cache_item(new_item);
        cached_item->hit_times++;
        CACHE_UNLOCK(manager);
        return cached_item;
    }

    /* 8. 更新缓存 */
    if (insert_column_into_cache(state, new_item) != TECC_CACHE_OK) {
        /* 插入缓存失败，清理资源 */
        CACHE_UNLOCK(manager);
        destroy_column_cache_item(new_item);
        /* 如果是可恢复错误且未尝试过自动恢复，尝试恢复 */
        if (!auto_recovery_attempted) {
            auto_recovery_attempted = true;
            TECC_CACHE_DEBUG("缓存插入失败，尝试自动恢复");
            if (tecc_cache_auto_recover(state) == TECC_CACHE_OK) {
                goto retry_with_recovery;
            }
        }
        return NULL;
    }

    /* 9. 更新加载次数统计和缓存大小 */
    manager->load_count++;
    manager->current_size++;

    /* 10. 解锁并返回结果 */
    CACHE_UNLOCK(manager);

    /* 11. 插入名称哨兵，标记该表已加载（仅在name可用时）
     * 注意：OID哨兵不在此处插入，因为OID路径只加载了单列，
     * 插入OID哨兵会错误阻止同表其他列的加载。OID哨兵仅在
     * tecc_cache_get_columns_by_table_name批量加载时插入。 */
    if (new_item->database_name != NULL && new_item->schema_name != NULL &&
        new_item->table_name != NULL) {
        (void)insert_name_sentinel(state, new_item->database_name,
                                    new_item->schema_name, new_item->table_name,
                                    TECC_CACHE_STATUS_TABLE_LOADED);
    }

    return new_item;
}

/*
 * tecc_cache_get_encrypted_column_by_name - 根据数据库名、模式名、表名和列名查询加密列信息
 *
 * 参数:
 *   state - TECC状态结构，包含连接和上下文信息
 *   database_name - 数据库名称
 *   schema_name - 模式名称
 *   table_name - 表名称
 *   column_name - 列名称
 *
 * 返回:
 *   加密列缓存项指针，NULL表示列未加密或不存在
 */
TeccColumnCacheItem *
tecc_cache_get_encrypted_column_by_name(const TeccState *state,
                                        const char *database_name,
                                        const char *schema_name,
                                        const char *table_name,
                                        const char *column_name)
{
    TeccCacheManager *manager = NULL;
    TeccColumnCacheItem *cached_item = NULL;

    /* 参数检查 */
    if (state == NULL || database_name == NULL || table_name == NULL ||
        column_name == NULL) {
        return NULL;
    }

    /* 如果未指定schema，按search_path顺序查找 */
    if (schema_name == NULL)
    {
        TeccCacheManager *sp_manager;
        int si;
        int schema_count;
        char **schemas;

        sp_manager = get_cache_manager(state);
        if (sp_manager == NULL)
            return NULL;

        CACHE_LOCK(sp_manager);
        schema_count = sp_manager->guc_params.search_path_schema_count;
        schemas = sp_manager->guc_params.search_path_schemas;
        CACHE_UNLOCK(sp_manager);

        for (si = 0; si < schema_count; si++)
        {
            const char *try_schema = schemas[si];
            if (try_schema == NULL)
                continue;

            {
                TeccColumnCacheItem *result;
                result = tecc_cache_get_encrypted_column_by_name(
                    state, database_name, try_schema, table_name, column_name);
                if (result != NULL)
                    return result;
            }
        }
        return NULL;
    }

    /* ReqID:TECC4.0 - pending_tables replaced by PENDING cache sentinel */



	/* 1. 获取缓存管理器 — DML路径不做自动恢复，查不到直接返回NULL */
	manager = get_cache_manager(state);
	if (manager == NULL) {
		return NULL;
	}

    /* 2. 检查并执行定时刷新（如果需要） */
    perform_timed_refresh_if_needed(manager);

    /* 3. 检查加密列缓存是否需要刷新并加锁 */
    CACHE_LOCK(manager);
    if (manager->column_cache_needs_refresh) {
        CACHE_UNLOCK(manager);
        /* 执行延迟刷新：获取写锁并清空缓存 */
        CACHE_WLOCK(manager);
        if (manager->column_cache_needs_refresh) {
            int clear_ret;

            TECC_CACHE_DEBUG("执行加密列缓存延迟刷新（按名称查询）");

            // 调用缓存清理函数
            clear_ret = tecc_cache_clear_specific(manager, TECC_CACHE_REFRESH_COLUMN);
            if (clear_ret != TECC_CACHE_OK) {
                TECC_CACHE_DEBUG("加密列缓存清理失败，错误码: %d", clear_ret);
                /* 清理失败，但仍然重置标记避免无限重试 */
                manager->column_cache_needs_refresh = false;
            }

            TECC_CACHE_DEBUG("加密列缓存延迟刷新完成（按名称查询）");
        }
        CACHE_RWUNLOCK(manager);
        /* 重新获取锁继续查询 */
        CACHE_LOCK(manager);
    }

    /* 4. 先检查名称哈希表缓存命中 */
    cached_item = find_column_in_cache_by_name(state, database_name, schema_name, table_name, column_name);
    if (cached_item != NULL) {
        /* 更新命中次数 */
        cached_item->hit_times++;
        /* 解锁并返回结果 */
        CACHE_UNLOCK(manager);
        return cached_item;
    }

    /* 5. 缓存未命中 — 检查是否有表级负缓存（名称哨兵）*/
    {
        bool sentinel_found = name_sentinel_exists(state, database_name, schema_name, table_name);
        if (sentinel_found) {
            /* 该表已加载过，此列不在缓存中即非加密列 */
            CACHE_UNLOCK(manager);
            return NULL;
        }
    }

    /* 6. 既无列缓存也无表哨兵 — 首次访问该表，解锁后触发懒加载 */
    CACHE_UNLOCK(manager);

    {
        int batch_count = 0;
        TeccColumnCacheItem **batch_items = NULL;

        /* 通过tecc_cache_get_columns_by_table_name批量加载整表 */
        batch_items = tecc_cache_get_columns_by_table_name(
            state, database_name, schema_name, table_name, &batch_count);

        /* 批量加载已通过内部逻辑插入哨兵，无需额外操作 */
        if (batch_items != NULL) {
            pg_free(batch_items);
        }
    }

    /* 7. 重新检查缓存 */
    CACHE_LOCK(manager);
    cached_item = find_column_in_cache_by_name(state, database_name, schema_name,
                                                table_name, column_name);
    if (cached_item != NULL) {
        cached_item->hit_times++;
        CACHE_UNLOCK(manager);
        return cached_item;
    }
    CACHE_UNLOCK(manager);
    return NULL;
}

/*
 * tecc_cache_get_columns_by_table - 批量查询表中所有加密列信息
 *
 * 参数:
 *   state - TECC状态结构，包含连接和上下文信息
 *   table_oid - 表对象标识符
 *   count - 返回的加密列数量
 *
 * 返回:
 *   加密列缓存项指针数组，NULL表示表中无加密列
 */
TeccColumnCacheItem **
tecc_cache_get_columns_by_table(const TeccState *state, Oid table_oid, int *count)
{
    TeccColumnCacheItem **items = NULL;
    int item_count = 0;
    TeccCacheManager *manager = NULL;
    TeccCacheQueryParams query;
    TeccFetcherErrorCode fetcher_error;

    /* 参数检查 */
    if (state == NULL || count == NULL) {
        TECC_CACHE_DEBUG("无效参数：state或count为NULL");
        if (count) {
            *count = 0;
        }
        return NULL;
    }
    if (table_oid == InvalidOid) {
        TECC_CACHE_DEBUG("无效参数：table_oid为InvalidOid");
        *count = 0;
        return NULL;
    }

    /* 1. 检查是否已缓存该表的所有列 */
    /* 简单实现：遍历缓存检查是否有该表的列 */
    manager = get_cache_manager(state);
    if (manager != NULL && manager->column_cache != NULL) {
        FeHashSeqStatus status;
        void *entry = NULL;
        int cached_count = 0;

        fe_hash_seq_init(&status, manager->column_cache);
        while ((entry = fe_hash_seq_search(&status)) != NULL) {
            TeccColumnCacheItem *item = *(TeccColumnCacheItem **)entry;
            if (item != NULL && item->table_oid == table_oid) {
                cached_count++;
            }
        }

        /* 如果有缓存项，可以进一步优化返回缓存的列 */
        /* 目前简化处理：总是从数据库加载 */
    }

    /* 2. 未缓存时批量加载 */
    memset(&query, 0, sizeof(query));
    query.flags = TECC_QUERY_BATCH | TECC_QUERY_BY_OID;
    query.params.by_oid.oid = table_oid;

    fetcher_error = tecc_fetch_column_for_cache(state, &query, (TeccColumnCacheItem **)&items, &item_count);
    if (fetcher_error != TECC_FETCHER_OK || items == NULL || item_count == 0) {
        (void)tecc_cache_convert_error(fetcher_error);
        if (count) {
            *count = 0;
        }
        return NULL;
    }

    /* 3. 更新缓存 - 批量插入所有列到缓存 */
    {
        int i;
        for (i = 0; i < item_count; i++) {
            if (items[i] != NULL) {
                /* 插入缓存，忽略错误继续处理其他列 */
                (void)insert_column_into_cache(state, items[i]);
            }
        }
    }

    /* 4. 返回数组和数量 */
    *count = item_count;
    return items;
}

/*
 * collect_cached_encrypted_columns - 扫描名称缓存，收集某表的所有ENCRYPTED列指针
 *
 * 返回的指针指向缓存哈希表中的实际item，调用者不能free单个item。
 * 调用者需要 pg_free() 返回的数组本身。
 */
static TeccColumnCacheItem **
collect_cached_encrypted_columns(const TeccState *state,
                                 const char *database_name,
                                 const char *schema_name,
                                 const char *table_name,
                                 int *count)
{
    TeccCacheManager *mgr;
    FeHashSeqStatus seq;
    void *data_ptr;
    int found = 0, idx = 0;
    TeccColumnCacheItem **result;

    if (state == NULL || count == NULL)
        return NULL;

    mgr = get_cache_manager(state);
    if (mgr == NULL || mgr->column_name_cache == NULL) {
        *count = 0;
        return NULL;
    }

    /* Pass 1: count */
    fe_hash_seq_init(&seq, mgr->column_name_cache);
    while ((data_ptr = fe_hash_seq_search(&seq)) != NULL) {
        TeccColumnCacheItem *it = *(TeccColumnCacheItem **)data_ptr;
        if (it != NULL &&
            it->cache_status == TECC_CACHE_STATUS_ENCRYPTED &&
            it->database_name != NULL &&
            strcmp(it->database_name, database_name) == 0 &&
            it->schema_name != NULL &&
            strcmp(it->schema_name, schema_name) == 0 &&
            it->table_name != NULL &&
            strcmp(it->table_name, table_name) == 0)
            found++;
    }

    if (found == 0) {
        *count = 0;
        return NULL;
    }

    result = (TeccColumnCacheItem **)pg_malloc(sizeof(TeccColumnCacheItem *) * found);
    if (result == NULL) {
        *count = 0;
        return NULL;
    }

    /* Pass 2: collect pointers */
    fe_hash_seq_init(&seq, mgr->column_name_cache);
    while ((data_ptr = fe_hash_seq_search(&seq)) != NULL) {
        TeccColumnCacheItem *it = *(TeccColumnCacheItem **)data_ptr;
        if (it != NULL &&
            it->cache_status == TECC_CACHE_STATUS_ENCRYPTED &&
            it->database_name != NULL &&
            strcmp(it->database_name, database_name) == 0 &&
            it->schema_name != NULL &&
            strcmp(it->schema_name, schema_name) == 0 &&
            it->table_name != NULL &&
            strcmp(it->table_name, table_name) == 0) {
            result[idx++] = it;
        }
    }

    *count = found;
    return result;
}

/*
 * tecc_cache_get_columns_by_table_name - 根据数据库名、模式名和表名批量查询加密列信息
 *
 * 参数:
 *   state - TECC状态结构，包含连接和上下文信息
 *   database_name - 数据库名称
 *   schema_name - 模式名称
 *   table_name - 表名称
 *   count - 返回的加密列数量
 *
 * 返回:
 *   加密列缓存项指针数组，NULL表示表中无加密列
 */
TeccColumnCacheItem **
tecc_cache_get_columns_by_table_name(TeccState *state,
                                     const char *database_name,
                                     const char *schema_name,
                                     const char *table_name,
                                     int *count)
{
    TeccColumnCacheItem **items = NULL;
    int ret;

    /* 参数检查 */
    if (state == NULL || database_name == NULL || schema_name == NULL ||
        table_name == NULL || count == NULL) {
        *count = 0;
        return NULL;
    }


    /* ReqID:TECC4.0 - pending_tables replaced by PENDING cache sentinel */

    /* 0. 检查表级负缓存（名称哨兵）*/
    {
        bool sf = name_sentinel_exists(state, database_name, schema_name, table_name);
        if (sf) {
            TeccCacheManager *mgr = get_cache_manager(state);
            if (mgr != NULL && mgr->column_name_cache != NULL) {
                ColumnNameHashKey nk;
                void *e;
                memset(&nk, 0, sizeof(nk));
                strncpy(nk.database_name, database_name, NAMEDATALEN - 1);
                strncpy(nk.schema_name, schema_name, NAMEDATALEN - 1);
                strncpy(nk.table_name, table_name, NAMEDATALEN - 1);
                e = fe_hash_search(mgr->column_name_cache, &nk, HASH_FIND, NULL);
                if (e != NULL) {
                    TeccColumnCacheItem *s = *(TeccColumnCacheItem **)e;
                    if (s != NULL && s->cache_status == TECC_CACHE_STATUS_NOT_ENCRYPTED) {
                        *count = 0;
                        return NULL;
                    }
                    if (s != NULL && s->cache_status == TECC_CACHE_STATUS_TABLE_LOADED) {
                        /* Table has encrypted columns in cache.
                         * Return cache-owned pointers — caller must NOT
                         * free individual items, only pg_free() the array. */
                        return collect_cached_encrypted_columns(state,
                            database_name, schema_name, table_name, count);
                    }
                }
            }
        }
    }

    /* 1. 直接调用按名称批量查询接口 */

    TECC_CACHE_DEBUG("DEBUG LOOP: Calling tecc_fetch_all_columns_for_table_by_name_for_cache for %s.%s.%s",
                    database_name, schema_name, table_name);
    ret = tecc_fetch_all_columns_for_table_by_name_for_cache(state,
                                                            database_name,
                                                            schema_name,
                                                            table_name,
                                                            &items,
                                                            count);

    TECC_CACHE_DEBUG("DEBUG LOOP: tecc_fetch_all_columns_for_table_by_name_for_cache returned: ret=%d, items=%p, count=%d",
                    ret, items, count ? *count : -1);

    if (ret != TECC_CACHE_OK || items == NULL || *count == 0) {
        TECC_CACHE_DEBUG("按名称查询表加密列失败: database=%s, schema=%s, table=%s, error=%d",
                        database_name, schema_name, table_name, ret);
        /* 表确实无加密列，插入名称哨兵 */
        (void)insert_name_sentinel(state, database_name, schema_name, table_name,
                                    TECC_CACHE_STATUS_NOT_ENCRYPTED);
        if (count) {
            *count = 0;
        }
        return NULL;
    }

    /* 2. Insert clones into cache, free DB-allocated originals */
    {
        int i;
        for (i = 0; i < *count; i++) {
            if (items[i] != NULL) {
                (void)insert_column_into_cache(state, items[i]);
            }
        }
    }

    /* Insert name + OID sentinels */
    (void)insert_name_sentinel(state, database_name, schema_name, table_name,
                                TECC_CACHE_STATUS_TABLE_LOADED);
    if (*count > 0 && items[0] != NULL) {
        (void)insert_oid_sentinel(state, items[0]->table_oid);
    }

    /* Free DB-allocated originals — clones are now in the cache */
    {
        int i;
        for (i = 0; i < *count; i++) {
            if (items[i] != NULL)
                tecc_cache_free_column_item(items[i]);
        }
        pg_free(items);
        items = NULL;
    }

    if (state != NULL)
        tecc_cache_remove_pending(state, database_name, schema_name, table_name);

    /* 3. Return cache-owned pointers — caller must NOT free individual
     *    items, only pg_free() the returned array. */
    return collect_cached_encrypted_columns(state,
        database_name, schema_name, table_name, count);
}

/*
 * create_column_cache_item - 创建加密列缓存项
 *
 * 返回:
 *   创建的加密列缓存项指针
 */
static TeccColumnCacheItem *
create_column_cache_item(void)
{
    TeccColumnCacheItem *item = NULL;

    /* 分配内存 */
    item = (TeccColumnCacheItem *)pg_malloc(sizeof(TeccColumnCacheItem));
    if (item == NULL)
    {
        return NULL;
    }

    /* 初始化所有字段 */
    memset(item, 0, sizeof(TeccColumnCacheItem));

    /* 设置默认值 */
    item->table_oid = InvalidOid;
    item->schema_oid = InvalidOid;
    item->database_oid = InvalidOid;
    item->column_attnum = 0;
    item->original_type_oid = InvalidOid;
    item->original_type_mod = -1;
    item->dek_id = InvalidOid;
    item->encryption_type = 0;
    item->collation_oid = InvalidOid;
    item->create_date = 0;
    item->hit_times = 0;

    /* 字符串字段初始化为NULL */
    item->table_name = NULL;
    item->schema_name = NULL;
    item->database_name = NULL;
    item->column_name = NULL;
    item->collation_locale = NULL;

    return item;
}

/*
 * destroy_column_cache_item - 销毁加密列缓存项
 *
 * 参数:
 *   item - 加密列缓存项指针
 */
static void
destroy_column_cache_item(TeccColumnCacheItem *item)
{
    if (item == NULL)
    {
        return;
    }

    /* 释放字符串字段 */
    if (item->table_name != NULL)
    {
        pg_free(item->table_name);
    }
    if (item->schema_name != NULL)
    {
        pg_free(item->schema_name);
    }
    if (item->database_name != NULL)
    {
        pg_free(item->database_name);
    }
    if (item->column_name != NULL)
    {
        pg_free(item->column_name);
    }
    if (item->collation_locale != NULL)
    {
        pg_free(item->collation_locale);
    }

    /* 释放缓存项本身 */
    pg_free(item);
}

/*
 * clone_column_cache_item - 深拷贝加密列缓存项
 *
 * 创建一个独立的内存副本，所有字符串字段都pg_strdup复制。
 * 调用者负责用destroy_column_cache_item释放返回的副本。
 *
 * 参数:
 *   src - 源缓存项指针
 *
 * 返回:
 *   复制的缓存项指针，NULL表示复制失败
 */
static TeccColumnCacheItem *
clone_column_cache_item(const TeccColumnCacheItem *src)
{
    TeccColumnCacheItem *dst;

    if (src == NULL)
        return NULL;

    dst = (TeccColumnCacheItem *)pg_malloc(sizeof(TeccColumnCacheItem));
    if (dst == NULL)
        return NULL;

    /* 复制所有标量字段 */
    memcpy(dst, src, sizeof(TeccColumnCacheItem));

    /* 将字符串指针指向深拷贝副本 */
    dst->table_name = src->table_name ? pg_strdup(src->table_name) : NULL;
    dst->schema_name = src->schema_name ? pg_strdup(src->schema_name) : NULL;
    dst->database_name = src->database_name ? pg_strdup(src->database_name) : NULL;
    dst->column_name = src->column_name ? pg_strdup(src->column_name) : NULL;
    dst->collation_locale = src->collation_locale ? pg_strdup(src->collation_locale) : NULL;

    return dst;
}

/*
 * find_column_in_cache - 在缓存中查找加密列
 *
 * 参数:
 *   table_oid - 表对象标识符
 *   column_attnum - 列号
 *
 * 返回:
 *   找到的加密列缓存项指针，NULL表示未找到
 */
static TeccColumnCacheItem *
find_column_in_cache(const TeccState *state, Oid table_oid, int column_attnum)
{
    TeccCacheManager *manager = NULL;
    ColumnHashKey key;
    bool found = false;
    void *entry = NULL;

    /* 获取缓存管理器 */
    manager = get_cache_manager(state);
    if (manager == NULL || !manager->initialized || manager->column_cache == NULL) {
        TECC_CACHE_DEBUG("缓存管理器未初始化或无效，无法查找加密列");
        return NULL;
    }

    /* 设置哈希键 */
    key.table_oid = table_oid;
    key.column_attnum = column_attnum;

    /* 在哈希表中查找 */
    entry = fe_hash_search(manager->column_cache, &key, HASH_FIND, &found);
    if (!found || entry == NULL) {
        return NULL;
    }

    /* 返回缓存项指针 */
    return *(TeccColumnCacheItem **)entry;
}

/*
 * find_column_in_cache_by_name - 在缓存中按名称查找加密列
 *
 * 参数:
 *   database_name - 数据库名称
 *   schema_name - 模式名称
 *   table_name - 表名称
 *   column_name - 列名称
 *
 * 返回:
 *   找到的加密列缓存项指针，NULL表示未找到
 */
static TeccColumnCacheItem *
find_column_in_cache_by_name(const TeccState *state, const char *database_name,
                             const char *schema_name,
                             const char *table_name,
                             const char *column_name)
{
    TeccCacheManager *manager = NULL;
    ColumnNameHashKey name_key;
    void *entry = NULL;
    TeccColumnCacheItem *item = NULL;

    /* 参数检查 */
    if (database_name == NULL || schema_name == NULL || table_name == NULL || column_name == NULL) {
        return NULL;
    }

    /* 获取缓存管理器 */
    manager = get_cache_manager(state);
    if (manager == NULL || manager->column_name_cache == NULL) {
        return NULL;
    }

    /* 构建名称键 */
    strncpy(name_key.database_name, database_name, NAMEDATALEN - 1);
    strncpy(name_key.schema_name, schema_name, NAMEDATALEN - 1);
    strncpy(name_key.table_name, table_name, NAMEDATALEN - 1);
    strncpy(name_key.column_name, column_name, NAMEDATALEN - 1);
    name_key.database_name[NAMEDATALEN - 1] = '\0';
    name_key.schema_name[NAMEDATALEN - 1] = '\0';
    name_key.table_name[NAMEDATALEN - 1] = '\0';
    name_key.column_name[NAMEDATALEN - 1] = '\0';

    /* 直接查询名称哈希表 */
    entry = fe_hash_search(manager->column_name_cache, &name_key, HASH_FIND, NULL);
    if (entry != NULL) {
        item = *(TeccColumnCacheItem **)entry;
        return item;
    }

    /* 未找到匹配项 */
    return NULL;
}

/*
 * load_column_from_database - 从数据库加载加密列信息
 *
 * 参数:
 *   state - TECC状态结构，包含连接和上下文信息
 *   table_oid - 表对象标识符
 *   column_attnum - 列号
 *   item - 返回的加密列缓存项指针
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
static int
load_column_from_database(const TeccState *state, Oid table_oid, int column_attnum, TeccColumnCacheItem **item)
{
    TeccCacheQueryParams query;
    TeccFetcherErrorCode fetcher_error;

    /* 参数检查 */
    if (state == NULL || item == NULL) {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    /* 调用简化的适配器函数加载数据 */
    memset(&query, 0, sizeof(query));
    query.flags = TECC_QUERY_BY_OID;
    query.params.by_oid.oid = table_oid;
    query.params.by_oid.attnum = column_attnum;

    fetcher_error = tecc_fetch_column_for_cache(state, &query, item, NULL);
    if (fetcher_error != TECC_FETCHER_OK) {
        return tecc_cache_convert_error(fetcher_error);
    }

    return TECC_CACHE_OK;
}

/*
 * load_column_from_database_by_name - 从数据库按名称加载加密列信息
 *
 * 参数:
 *   state - TECC状态结构，包含连接和上下文信息
 *   database_name - 数据库名称
 *   schema_name - 模式名称
 *   table_name - 表名称
 *   column_name - 列名称
 *   item - 返回的加密列缓存项指针
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
static int
load_column_from_database_by_name(const TeccState *state,
                                  const char *database_name,
                                  const char *schema_name,
                                  const char *table_name,
                                  const char *column_name,
                                  TeccColumnCacheItem **item)
{
    TeccCacheQueryParams query;
    TeccFetcherErrorCode fetcher_error;

    /* 参数检查 */
    if (state == NULL || database_name == NULL || schema_name == NULL ||
        table_name == NULL || column_name == NULL || item == NULL) {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    /* 调用简化的适配器函数按名称加载数据 */
    /* TeccCacheQueryParams query; 已移到函数开头 */
    memset(&query, 0, sizeof(query));
    query.flags = TECC_QUERY_BY_NAME;
    query.database_name = database_name;
    query.schema_name = schema_name;
    query.table_name = table_name;
    query.column_name = column_name;

    fetcher_error = tecc_fetch_column_for_cache(state, &query, item, NULL);
    if (fetcher_error != TECC_FETCHER_OK) {
        return tecc_cache_convert_error(fetcher_error);
    }

    return TECC_CACHE_OK;
}

/*
 * load_all_columns_from_database - 从数据库加载表中所有加密列信息
 *
 * 参数:
 *   state - TECC状态结构，包含连接和上下文信息
 *   table_oid - 表对象标识符
 *   items - 返回的加密列缓存项指针数组
 *   count - 返回的加密列数量
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
static int TECC_ATTR_UNUSED
load_all_columns_from_database(const TeccState *state, Oid table_oid, TeccColumnCacheItem ***items, int *count)
{
    TeccCacheQueryParams query;
    TeccFetcherErrorCode fetcher_error;

    /* 参数检查 */
    if (state == NULL || items == NULL || count == NULL) {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    /* 调用简化的适配器函数批量加载数据 */
    /* TeccCacheQueryParams query; 已移到函数开头 */
    memset(&query, 0, sizeof(query));
    query.flags = TECC_QUERY_BATCH | TECC_QUERY_BY_OID;
    query.params.by_oid.oid = table_oid;

    fetcher_error = tecc_fetch_column_for_cache(state, &query, (TeccColumnCacheItem **)items, count);
    if (fetcher_error != TECC_FETCHER_OK) {
        return tecc_cache_convert_error(fetcher_error);
    }

    return TECC_CACHE_OK;
}

/*
 * load_all_columns_from_database_by_name - 从数据库按名称加载表中所有加密列信息
 *
 * 参数:
 *   state - TECC状态结构，包含连接和上下文信息
 *   database_name - 数据库名称
 *   schema_name - 模式名称
 *   table_name - 表名称
 *   items - 返回的加密列缓存项指针数组
 *   count - 返回的加密列数量
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
static int TECC_ATTR_UNUSED
load_all_columns_from_database_by_name(const TeccState *state,
                                       const char *database_name,
                                       const char *schema_name,
                                       const char *table_name,
                                       TeccColumnCacheItem ***items,
                                       int *count)
{
    TeccCacheQueryParams query;
    TeccFetcherErrorCode fetcher_error;

    /* 参数检查 */
    if (state == NULL || database_name == NULL || schema_name == NULL ||
        table_name == NULL || items == NULL || count == NULL) {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    /* 调用简化的适配器函数按名称批量加载数据 */
    /* TeccCacheQueryParams query; 已移到函数开头 */
    memset(&query, 0, sizeof(query));
    query.flags = TECC_QUERY_BATCH | TECC_QUERY_BY_NAME;
    query.database_name = database_name;
    query.schema_name = schema_name;
    query.table_name = table_name;

    fetcher_error = tecc_fetch_column_for_cache(state, &query, (TeccColumnCacheItem **)items, count);
    if (fetcher_error != TECC_FETCHER_OK) {
        return tecc_cache_convert_error(fetcher_error);
    }

    return TECC_CACHE_OK;
}

/*
 * insert_column_into_cache - 将加密列缓存项插入到缓存中
 *
 * 参数:
 *   item - 加密列缓存项指针
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
static int
insert_column_into_cache(const TeccState *state, TeccColumnCacheItem *item)
{
    TeccCacheManager *manager = NULL;
    ColumnHashKey oid_key;
    ColumnNameHashKey name_key;
    void *entry = NULL;
    bool found = false;

    TeccColumnCacheItem *cloned_item = NULL;

    /* 参数检查 */
    if (item == NULL) {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }
    /* 非哨兵项需要有效的OID和attnum */
    /* 正常加密列缓存项需要有效的OID和attnum；哨兵项（NOT_ENCRYPTED/PENDING）可跳过 */
    if (item->cache_status == TECC_CACHE_STATUS_ENCRYPTED &&
        (item->table_oid == InvalidOid || item->column_attnum <= 0)) {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    /* 深拷贝item，使缓存拥有独立的内存副本 */
    cloned_item = clone_column_cache_item(item);
    if (cloned_item == NULL) {
        return TECC_CACHE_ERROR_MEMORY;
    }

    /* 获取缓存管理器 */
    manager = get_cache_manager(state);
    if (manager == NULL || !manager->initialized) {
        destroy_column_cache_item(cloned_item);
        TECC_CACHE_DEBUG("缓存管理器未初始化或无效，无法插入加密列");
        return TECC_CACHE_ERROR_NOT_INITIALIZED;
    }

    /* 1. 插入到OID哈希表（哨兵项可能无有效OID，跳过） */
    if (cloned_item->table_oid != InvalidOid && cloned_item->column_attnum > 0 &&
        manager->column_cache != NULL) {
        oid_key.table_oid = cloned_item->table_oid;
        oid_key.column_attnum = cloned_item->column_attnum;
        entry = fe_hash_search(manager->column_cache, &oid_key, HASH_ENTER, &found);
        if (entry == NULL) {
            destroy_column_cache_item(cloned_item);
            return TECC_CACHE_ERROR_MEMORY;
        }
        /* 如果之前已有条目，释放旧条目 */
        if (found && *(TeccColumnCacheItem **)entry != NULL) {
            destroy_column_cache_item(*(TeccColumnCacheItem **)entry);
        }
        *(TeccColumnCacheItem **)entry = cloned_item;
    }

    /* 2. 插入到名称哈希表（如果名称可用） */
    if (manager->column_name_cache != NULL &&
        cloned_item->database_name != NULL && cloned_item->schema_name != NULL &&
        cloned_item->table_name != NULL && cloned_item->column_name != NULL) {
        bool name_found = false;
        /* 构建名称键 */
        strncpy(name_key.database_name, cloned_item->database_name, NAMEDATALEN - 1);
        strncpy(name_key.schema_name, cloned_item->schema_name, NAMEDATALEN - 1);
        strncpy(name_key.table_name, cloned_item->table_name, NAMEDATALEN - 1);
        strncpy(name_key.column_name, cloned_item->column_name, NAMEDATALEN - 1);
        name_key.database_name[NAMEDATALEN - 1] = '\0';
        name_key.schema_name[NAMEDATALEN - 1] = '\0';
        name_key.table_name[NAMEDATALEN - 1] = '\0';
        name_key.column_name[NAMEDATALEN - 1] = '\0';

        entry = fe_hash_search(manager->column_name_cache, &name_key, HASH_ENTER, &name_found);
        if (entry != NULL) {
            /* 指向同一个缓存的克隆项 */
            *(TeccColumnCacheItem **)entry = cloned_item;
        }
        /* 注意：名称哈希表插入失败不影响整体功能，可以继续 */
    }

    /* 3. 更新缓存统计信息 */
    if (!found) {
        manager->current_size++;
    }

    return TECC_CACHE_OK;
}

/* get_cache_manager函数在其他缓存文件中定义 */


/* ==================== 表级负缓存哨兵函数 ==================== */

/*
 * name_sentinel_exists - 检查名称缓存中是否存在表级哨兵
 *
 * 表级哨兵的key中 column_name 为空串，cache_status=NOT_ENCRYPTED。
 * 存在哨兵意味着该表已通过批量查询确认过加密列信息。
 */
static bool
name_sentinel_exists(const TeccState *state,
                     const char *database_name,
                     const char *schema_name,
                     const char *table_name)
{
    TeccCacheManager *manager;
    ColumnNameHashKey name_key;
    void *entry;

    if (database_name == NULL || schema_name == NULL || table_name == NULL)
        return false;

    manager = get_cache_manager(state);
    if (manager == NULL || manager->column_name_cache == NULL)
        return false;

    /* 用空串列名构建哨兵key（需memset清零确保哈希一致性）*/
    memset(&name_key, 0, sizeof(name_key));
    strncpy(name_key.database_name, database_name, NAMEDATALEN - 1);
    strncpy(name_key.schema_name, schema_name, NAMEDATALEN - 1);
    strncpy(name_key.table_name, table_name, NAMEDATALEN - 1);
    /* column_name保持全零 */

    entry = fe_hash_search(manager->column_name_cache, &name_key, HASH_FIND, NULL);
    if (entry != NULL) {
        TeccColumnCacheItem *item = *(TeccColumnCacheItem **)entry;
        if (item != NULL &&
            (item->cache_status == TECC_CACHE_STATUS_NOT_ENCRYPTED ||
             item->cache_status == TECC_CACHE_STATUS_TABLE_LOADED)) {
            return true;
        }
    }
    return false;
}

/*
 * oid_sentinel_exists - 检查OID缓存中是否存在表级哨兵
 *
 * OID哨兵的key中 column_attnum=0（无有效列使用该值），cache_status=NOT_ENCRYPTED。
 */
static bool
oid_sentinel_exists(const TeccState *state, Oid table_oid)
{
    TeccCacheManager *manager;
    ColumnHashKey oid_key;
    void *entry;

    if (table_oid == InvalidOid)
        return false;

    manager = get_cache_manager(state);
    if (manager == NULL || manager->column_cache == NULL)
        return false;

    oid_key.table_oid = table_oid;
    oid_key.column_attnum = 0;

    entry = fe_hash_search(manager->column_cache, &oid_key, HASH_FIND, NULL);
    if (entry != NULL) {
        TeccColumnCacheItem *item = *(TeccColumnCacheItem **)entry;
        if (item != NULL && item->cache_status == TECC_CACHE_STATUS_NOT_ENCRYPTED)            return true;
    }
    return false;
}

/*
 * insert_name_sentinel - 插入名称缓存表级哨兵
 *
 * 标记"该表已加载过加密列元数据"，后续列级查询命中哨兵即知无需再查库。
 */
static int
insert_name_sentinel(const TeccState *state,
                     const char *database_name,
                     const char *schema_name,
                     const char *table_name,
                     TeccCacheItemStatus sentinel_status)
{
    TeccColumnCacheItem *sentinel;

    /* 已存在则跳过 */
    if (name_sentinel_exists(state, database_name, schema_name, table_name))
        return TECC_CACHE_OK;

    sentinel = create_column_cache_item();
    if (sentinel == NULL)
        return TECC_CACHE_ERROR_MEMORY;

    sentinel->cache_status = sentinel_status;
    sentinel->database_name = pg_strdup(database_name);
    sentinel->schema_name = pg_strdup(schema_name);
    sentinel->table_name = pg_strdup(table_name);
    sentinel->column_name = pg_strdup("");

    return insert_column_into_cache(state, sentinel);
}

/*
 * insert_oid_sentinel - 插入OID缓存表级哨兵
 */
static int
insert_oid_sentinel(const TeccState *state, Oid table_oid)
{
    TeccColumnCacheItem *sentinel;

    if (table_oid == InvalidOid)
        return TECC_CACHE_ERROR_INVALID_PARAM;

    if (oid_sentinel_exists(state, table_oid))
        return TECC_CACHE_OK;

    sentinel = create_column_cache_item();
    if (sentinel == NULL)
        return TECC_CACHE_ERROR_MEMORY;

    sentinel->cache_status = TECC_CACHE_STATUS_NOT_ENCRYPTED;
    sentinel->table_oid = table_oid;
    sentinel->column_attnum = 0;

    return insert_column_into_cache(state, sentinel);
}

/*
 * tecc_cache_insert_pending - Insert a pending refresh sentinel for a table
 *
 * Marks a table as having encrypted columns that need to be loaded from DB.
 * Called during CREATE TABLE preprocessing to replace pending_tables bridge.
 * The sentinel is removed in postprocess when the metadata is loaded.
 */
int
tecc_cache_insert_pending(TeccState *state, const char *database_name,
                           const char *schema_name, const char *table_name)
{
    TeccColumnCacheItem *sentinel;

    if (state == NULL || database_name == NULL || schema_name == NULL || table_name == NULL)
        return TECC_CACHE_ERROR_INVALID_PARAM;

    sentinel = create_column_cache_item();
    if (sentinel == NULL)
        return TECC_CACHE_ERROR_MEMORY;

    sentinel->cache_status = TECC_CACHE_STATUS_PENDING;
    sentinel->database_name = pg_strdup(database_name);
    sentinel->schema_name = pg_strdup(schema_name);
    sentinel->table_name = pg_strdup(table_name);
    sentinel->column_name = pg_strdup("");

    return insert_column_into_cache(state, sentinel);
}

/*
 * tecc_cache_remove_pending - Remove a pending refresh sentinel for a table
 */
int
tecc_cache_remove_pending(TeccState *state, const char *database_name,
                           const char *schema_name, const char *table_name)
{
    TeccCacheManager *manager;
    ColumnNameHashKey name_key;
    void *entry;

    if (state == NULL || database_name == NULL || schema_name == NULL || table_name == NULL)
        return TECC_CACHE_ERROR_INVALID_PARAM;

    manager = get_cache_manager(state);
    if (manager == NULL || manager->column_name_cache == NULL)
        return TECC_CACHE_ERROR_NOT_INITIALIZED;

    memset(&name_key, 0, sizeof(name_key));
    strncpy(name_key.database_name, database_name, NAMEDATALEN - 1);
    strncpy(name_key.schema_name, schema_name, NAMEDATALEN - 1);
    strncpy(name_key.table_name, table_name, NAMEDATALEN - 1);

    /* Use HASH_FIND first — HASH_REMOVE would also delete the
     * NOT_ENCRYPTED sentinel (same key, column_name="") that was
     * just inserted by insert_name_sentinel above. */
    entry = fe_hash_search(manager->column_name_cache, &name_key, HASH_FIND, NULL);
    if (entry != NULL) {
        TeccColumnCacheItem *item = *(TeccColumnCacheItem **)entry;
        if (item != NULL && item->cache_status == TECC_CACHE_STATUS_PENDING) {
            void *removed = fe_hash_search(manager->column_name_cache, &name_key, HASH_REMOVE, NULL);
            destroy_column_cache_item(item);
            pg_free(removed);
        }
    }
    return TECC_CACHE_OK;
}

/*
 * tecc_cache_insert_column_item - Insert a single column cache item into formal cache
 *
 * Public wrapper around insert_column_into_cache for use by tecc_refresh_table_cache.
 */
int
tecc_cache_insert_column_item(TeccState *state, TeccColumnCacheItem *item)
{
    if (state == NULL || item == NULL)
        return TECC_CACHE_ERROR_INVALID_PARAM;

    return insert_column_into_cache(state, item);
}

/* ==================== 缓存项转换函数实现 ==================== */

/*
 * tecc_convert_column_cache_item_to_metadata - 将列缓存项转换为加密列元数据
 *
 * 参数:
 *   state - TECC状态结构
 *   cache_item - 列缓存项指针
 *
 * 返回:
 *   转换后的加密列元数据指针，NULL表示转换失败
 */
TeccEncryptedColumnMetadata *
tecc_convert_column_cache_item_to_metadata(
    const TeccState *state,
    const TeccColumnCacheItem *cache_item)
{
    TeccEncryptedColumnMetadata *metadata = NULL;
    TeccDekCacheItem *dek_item = NULL;

    /* 参数检查 */
    if (cache_item == NULL) {
        return NULL;
    }

    /* 分配元数据结构 */
    metadata = (TeccEncryptedColumnMetadata *)pg_malloc(sizeof(TeccEncryptedColumnMetadata));
    if (metadata == NULL) {
        return NULL;
    }

    /* 初始化结构体 */
    memset(metadata, 0, sizeof(TeccEncryptedColumnMetadata));

    /* 复制基本信息 */
    metadata->table_oid = cache_item->table_oid;
    metadata->column_attnum = cache_item->column_attnum;
    if (cache_item->column_name != NULL) {
        metadata->column_name = pg_strdup(cache_item->column_name);
    }
    metadata->data_type_original_oid = cache_item->original_type_oid;
    metadata->data_type_original_mod = cache_item->original_type_mod;

    /* 复制加密信息 */
    /* 直接从缓存项获取DEK名称和KEK名称 */
    metadata->dek_name = NULL;
    metadata->kek_name = NULL;

    /* 获取DEK名称 - 前端只需要名称，不需要OID */
    if (cache_item->dek_name != NULL && strlen(cache_item->dek_name) > 0) {
        metadata->dek_name = pg_strdup(cache_item->dek_name);
    } else {
        /* DEK名称缺失，返回错误 */
        TECC_CACHE_DEBUG("DEK名称缺失，无法创建列元数据");
        tecc_free_encrypted_column_metadata(metadata);
        return NULL;
    }

    /* 通过DEK名称获取KEK名称 */
    if (state != NULL && metadata->dek_name != NULL) {
        dek_item = tecc_cache_get_dek_by_name_search_path(state, metadata->dek_name);
        if (dek_item != NULL && dek_item->kek_oid != InvalidOid) {
            TeccKekCacheItem *kek_item = tecc_cache_get_kek_by_oid(state, dek_item->kek_oid);
            if (kek_item != NULL && kek_item->key_name != NULL) {
                metadata->kek_name = pg_strdup(kek_item->key_name);
                /* 注意：不需要释放kek_item，缓存模块管理其生命周期 */
            }
            /* 注意：不需要释放dek_item，缓存模块管理其生命周期 */
        }
    }

    /* 如果无法获取KEK名称，保持为NULL */
    /* 注意：保持NULL而非空字符串，以便上层调用者能正确判断KEK不存在 */

    metadata->encryption_type = cache_item->encryption_type;
    metadata->column_key_id = cache_item->dek_id;

    /* 设置加密算法标志 */
    /* 使用HG_ENCRYPTION_TYPE_*常量 */
    /* encryption_type==0表示默认/未设置，此时只启用RND */
    metadata->enable_det = (cache_item->encryption_type & 0x01) != 0;
    metadata->enable_ope = (cache_item->encryption_type & 0x02) != 0;  /* HG_ENCRYPTION_TYPE_OPE */
    metadata->enable_search = (cache_item->encryption_type & 0x04) != 0; /* HG_ENCRYPTION_TYPE_SEARCH */

    /* 复制排序规则信息 */
    metadata->collation_oid = cache_item->collation_oid;
    if (cache_item->collation_locale != NULL) {
        metadata->collation_locale = pg_strdup(cache_item->collation_locale);
    }

    /* 设置创建时间和创建者 */
    metadata->create_date = cache_item->create_date;
    metadata->creator = 0;  /* 默认为0 */

    return metadata;
}

/*
 * tecc_free_encrypted_column_metadata - 释放加密列元数据
 * 注意：此函数在数据获取器模块中定义，缓存模块使用该实现
 *
 * 参数:
 *   metadata - 要释放的加密列元数据指针
 */
/* void tecc_free_encrypted_column_metadata(TeccEncryptedColumnMetadata *metadata)
{
    // 此函数在数据获取器模块中定义
} */

/*
 * tecc_cache_check_column_dek_permission - 检查并缓存列DEK权限
 *
 * 查找加密列缓存项，检查并缓存当前用户对该列DEK的USAGE权限。
 * 结果缓存在列缓存项中，避免每次解密都查询数据库。
 *
 * 参数:
 *   state - TECC状态结构
 *   table_name - 表名称
 *   column_name - 列名称
 *
 * 返回:
 *   true表示有权限，false表示无权限或检查失败
 */
bool
tecc_cache_check_column_dek_permission(TeccState *state,
                                       const char *schema_name,
                                       const char *table_name,
                                       const char *column_name)
{
    TeccCacheManager *manager;
    TeccColumnCacheItem *cached_item;
    TeccFetcherErrorCode perm_ret;
    const char *db_name;
    bool has_permission;

    if (state == NULL || table_name == NULL || column_name == NULL)
    {
        TECC_CACHE_DEBUG("check_dek_permission: NULL param (state=%p, tbl=%s, col=%s)",
                        (void*)state,
                        table_name ? table_name : "NULL",
                        column_name ? column_name : "NULL");
        return false;
    }

    db_name = PQdb(state->conn);
    if (db_name == NULL || db_name[0] == '\0')
        db_name = "postgres";

    /* 确保列已在缓存中 */
    cached_item = tecc_cache_get_encrypted_column_by_name(state, db_name, schema_name,
                                                          table_name, column_name);
    if (cached_item == NULL)
    {
        TECC_CACHE_DEBUG("check_dek_permission: %s.%s not in cache",
                        table_name, column_name);
        return false;
    }

    /* 没有dek_name则无法检查权限 */
    if (cached_item->dek_name == NULL || cached_item->dek_name[0] == '\0')
    {
        TECC_CACHE_DEBUG("check_dek_permission: no dek_name for %s.%s",
                        table_name, column_name);
        return false;
    }

    manager = get_cache_manager(state);
    if (manager == NULL)
    {
        TECC_CACHE_DEBUG("check_dek_permission: no cache manager for %s.%s",
                        table_name, column_name);
        return false;
    }

    /* 检查是否已有缓存结果 */
    CACHE_LOCK(manager);
    if (cached_item->dek_permission_checked)
    {
        has_permission = cached_item->has_dek_permission;
        CACHE_UNLOCK(manager);
        return has_permission;
    }
    CACHE_UNLOCK(manager);

    /* 执行权限检查（涉及SQL查询，必须先释放锁） */
    perm_ret = tecc_check_dek_permission(state, cached_item->dek_name, NULL);
    has_permission = (perm_ret == TECC_FETCHER_OK);

    /* 缓存结果 */
    CACHE_LOCK(manager);
    cached_item->has_dek_permission = has_permission;
    cached_item->dek_permission_checked = true;
    CACHE_UNLOCK(manager);

    return has_permission;
}

/* End - ReqID:TECC4.0 */