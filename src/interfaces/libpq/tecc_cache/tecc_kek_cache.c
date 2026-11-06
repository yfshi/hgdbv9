/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_kek_cache.c
 * Abstract:
 *   TECC KEK缓存实现，基于简化设计的两层架构。
 *   提供KEK相关的缓存查询和管理功能。
 *
 * Authored by <Claude Code>, 2026-04-15.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_cache/tecc_kek_cache.c
 */

#include "postgres_fe.h"
#include "tecc_metadata_cache.h"
#include "tecc_data_fetcher_adapter.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/* 内部函数声明 */
static void destroy_kek_cache_item(TeccKekCacheItem *item);
static TeccKekCacheItem *find_kek_in_cache_by_oid(const TeccState *state, Oid kek_oid);
static TeccKekCacheItem *find_kek_in_cache_by_name(const TeccState *state, const char *key_name, const char *key_namespace);
static int insert_kek_into_cache(const TeccState *state, TeccKekCacheItem *item);
static int load_kek_from_database_by_oid(const TeccState *state, Oid kek_oid, TeccKekCacheItem **item);
static int load_kek_from_database_by_name(const TeccState *state, const char *key_name, const char *key_namespace, TeccKekCacheItem **item);

/* 辅助函数声明 */

/*
 * tecc_cache_get_kek_by_oid - 根据OID查询KEK信息
 *
 * 参数:
 *   kek_oid - KEK对象标识符
 *
 * 返回:
 *   KEK缓存项指针，NULL表示KEK不存在
 */
TeccKekCacheItem *
tecc_cache_get_kek_by_oid(const TeccState *state, Oid kek_oid)
{
    TeccCacheManager *manager = NULL;
    TeccKekCacheItem *cached_item = NULL;
    TeccKekCacheItem *new_item = NULL;
    int ret;
    bool auto_recovery_attempted = false;

    /* 输入验证和边界检查 */
    if (state == NULL) {
        TECC_CACHE_DEBUG("无效参数：state为NULL");
        return NULL;
    }
    if (kek_oid == InvalidOid) {
        TECC_CACHE_DEBUG("无效参数：kek_oid为InvalidOid");
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

    /* 3. 检查KEK缓存是否需要刷新并加锁 */
    CACHE_RLOCK(manager);
    if (manager->kek_cache_needs_refresh) {
        CACHE_RWUNLOCK(manager);
        /* 执行延迟刷新：获取写锁并清空缓存 */
        CACHE_WLOCK(manager);
        if (manager->kek_cache_needs_refresh) {
            int clear_ret;

            TECC_CACHE_DEBUG("执行KEK缓存延迟刷新");

            // 调用缓存清理函数
            clear_ret = tecc_cache_clear_specific(manager, TECC_CACHE_REFRESH_KEK);
            if (clear_ret != TECC_CACHE_OK) {
                TECC_CACHE_DEBUG("KEK缓存清理失败，错误码: %d", clear_ret);
                /* 清理失败，但仍然重置标记避免无限重试 */
                manager->kek_cache_needs_refresh = false;
            }

            TECC_CACHE_DEBUG("KEK缓存延迟刷新完成");
        }
        CACHE_RWUNLOCK(manager);
        /* 重新获取读锁继续查询 */
        CACHE_RLOCK(manager);
    }

    /* 3. 检查缓存命中 */
    cached_item = find_kek_in_cache_by_oid(state, kek_oid);
    if (cached_item != NULL) {
        /* 更新命中次数 */
        cached_item->hit_times++;
        /* 更新管理器级别统计 */
        manager->cache_hits++;
        /* 解锁并返回结果 */
        CACHE_RWUNLOCK(manager);
        return cached_item;
    }

    /* 4. 缓存未命中，解锁以便进行数据库查询（避免长时间持有锁） */
    /* 更新未命中统计 */
    manager->cache_misses++;
    CACHE_RWUNLOCK(manager);

    /* 5. 从数据库加载 */
    ret = load_kek_from_database_by_oid(state, kek_oid, &new_item);
    if (ret != TECC_CACHE_OK || new_item == NULL) {
        /* 查询失败或KEK不存在 */
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

    /* 6. 加锁插入缓存 */
    CACHE_WLOCK(manager);

    /* 7. 再次检查缓存（防止其他线程已插入） */
    cached_item = find_kek_in_cache_by_oid(state, kek_oid);
    if (cached_item != NULL) {
        /* 其他线程已插入，使用现有缓存项 */
        destroy_kek_cache_item(new_item);
        cached_item->hit_times++;
        CACHE_RWUNLOCK(manager);
        return cached_item;
    }

    /* 8. 更新缓存 */
    if (insert_kek_into_cache(state, new_item) != TECC_CACHE_OK) {
        /* 插入缓存失败，清理资源 */
        CACHE_RWUNLOCK(manager);
        destroy_kek_cache_item(new_item);
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
    CACHE_RWUNLOCK(manager);
    return new_item;
}

/*
 * tecc_cache_get_kek_by_name - 根据名称查询KEK信息
 *
 * 参数:
 *   key_name - KEK名称
 *   key_namespace - 命名空间
 *
 * 返回:
 *   KEK缓存项指针，NULL表示KEK不存在
 */
TeccKekCacheItem *
tecc_cache_get_kek_by_name(const TeccState *state, const char *key_name, const char *key_namespace)
{
    TeccCacheManager *manager = NULL;
    TeccKekCacheItem *cached_item = NULL;
    TeccKekCacheItem *new_item = NULL;
    int ret;
    bool auto_recovery_attempted = false;

    /* 输入验证和边界检查 */
    if (state == NULL) {
        TECC_CACHE_DEBUG("无效参数：state为NULL");
        return NULL;
    }
    if (key_name == NULL || key_name[0] == '\0') {
        TECC_CACHE_DEBUG("无效参数：key_name为NULL或空字符串");
        return NULL;
    }
    if (key_namespace == NULL) {
        TECC_CACHE_DEBUG("key_namespace为NULL，将搜索所有命名空间并回填缓存");
    } else {
        /* 检查字符串长度，防止缓冲区溢出 */
        if (strlen(key_namespace) >= NAMEDATALEN) {
            TECC_CACHE_DEBUG("无效参数：key_namespace长度超过限制（%d）", NAMEDATALEN);
            return NULL;
        }
        /* 检查字符串中间是否包含空字符（不是终止符） */
        if (memchr(key_namespace, '\0', strlen(key_namespace)) != NULL) {
            TECC_CACHE_DEBUG("无效参数：字符串中间包含空字符");
            return NULL;
        }
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

    /* 3. 检查KEK缓存是否需要刷新并加锁 */
    CACHE_LOCK(manager);
    if (manager->kek_cache_needs_refresh) {
        CACHE_UNLOCK(manager);
        /* 执行延迟刷新：获取写锁并清空缓存 */
        CACHE_WLOCK(manager);
        if (manager->kek_cache_needs_refresh) {
            int clear_ret;

            TECC_CACHE_DEBUG("执行KEK缓存延迟刷新（按名称查询）");

            // 调用缓存清理函数
            clear_ret = tecc_cache_clear_specific(manager, TECC_CACHE_REFRESH_KEK);
            if (clear_ret != TECC_CACHE_OK) {
                TECC_CACHE_DEBUG("KEK缓存清理失败，错误码: %d", clear_ret);
                /* 清理失败，但仍然重置标记避免无限重试 */
                manager->kek_cache_needs_refresh = false;
            }

            TECC_CACHE_DEBUG("KEK缓存延迟刷新完成（按名称查询）");
        }
        CACHE_RWUNLOCK(manager);
        /* 重新获取锁继续查询 */
        CACHE_LOCK(manager);
    }

    /* 3. 检查缓存命中 */
    cached_item = find_kek_in_cache_by_name(state, key_name, key_namespace);
    if (cached_item != NULL) {
        /* 负缓存命中 */
        if (cached_item->cache_status == TECC_CACHE_STATUS_NOT_FOUND) {
            TECC_CACHE_DEBUG("负缓存命中（不存在）: name=%s", key_name);
            CACHE_UNLOCK(manager);
            return NULL;
        }
        if (cached_item->cache_status == TECC_CACHE_STATUS_PERMISSION_DENIED) {
            TECC_CACHE_DEBUG("负缓存命中（无权限）: name=%s", key_name);
            CACHE_UNLOCK(manager);
            return NULL;
        }
        /* 更新命中次数 */
        cached_item->hit_times++;
        /* 解锁并返回结果 */
        CACHE_UNLOCK(manager);
        return cached_item;
    }

    /* 4. 缓存未命中，解锁以便进行数据库查询（避免长时间持有锁） */
    CACHE_UNLOCK(manager);

    /* 5. 从数据库加载（NULL命名空间表示搜索所有schema） */
    ret = load_kek_from_database_by_name(state, key_name, key_namespace, &new_item);
    if (ret != TECC_CACHE_OK || new_item == NULL) {
        TECC_CACHE_DEBUG("数据库查询失败: name=%s, namespace=%s, ret=%d", key_name, key_namespace ? key_namespace : "NULL", ret);
        /* 如果是可恢复错误且未尝试过自动恢复，尝试恢复 */
        if (!auto_recovery_attempted && tecc_cache_is_recoverable_error(state, ret)) {
            auto_recovery_attempted = true;
            TECC_CACHE_DEBUG("数据库查询失败（错误码: %d），尝试自动恢复", ret);
            if (tecc_cache_auto_recover(state) == TECC_CACHE_OK) {
                goto retry_with_recovery;
            }
        }
        /* 对不可恢复的错误插入负缓存 */
        if (ret == TECC_CACHE_ERROR_NOT_FOUND || ret == TECC_CACHE_ERROR_PERMISSION) {
            TeccKekCacheItem *sentinel = (TeccKekCacheItem *)pg_malloc0(sizeof(TeccKekCacheItem));
            if (sentinel != NULL) {
                sentinel->key_name = pg_strdup(key_name);
                sentinel->key_namespace = pg_strdup(key_namespace ? key_namespace : "");
                if (ret == TECC_CACHE_ERROR_PERMISSION)
                    sentinel->cache_status = TECC_CACHE_STATUS_PERMISSION_DENIED;
                else
                    sentinel->cache_status = TECC_CACHE_STATUS_NOT_FOUND;
                CACHE_LOCK(manager);
                (void)insert_kek_into_cache(state, sentinel);
                CACHE_UNLOCK(manager);
            }
        }
        return NULL;
    }

    /* 6. 加锁插入缓存 */
    CACHE_LOCK(manager);

    /* 7. 再次检查缓存（防止其他线程已插入） */
    cached_item = find_kek_in_cache_by_name(state, key_name, key_namespace);
    if (cached_item != NULL) {
        /* 其他线程已插入，使用现有缓存项 */
        destroy_kek_cache_item(new_item);
        cached_item->hit_times++;
        CACHE_UNLOCK(manager);
        return cached_item;
    }

    /* 8. 更新缓存 */
    if (insert_kek_into_cache(state, new_item) != TECC_CACHE_OK) {
        /* 插入缓存失败，清理资源 */
        CACHE_UNLOCK(manager);
        destroy_kek_cache_item(new_item);
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
    return new_item;
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
    if (item == NULL)
    {
        return;
    }

    /* 安全清理敏感元数据 */
    tecc_cache_secure_clear_kek_metadata(item);

    /* 释放字符串字段（安全清理后释放内存） */
    if (item->key_name != NULL)
    {
        pg_free(item->key_name);
        item->key_name = NULL;
    }
    if (item->key_namespace != NULL)
    {
        pg_free(item->key_namespace);
        item->key_namespace = NULL;
    }
    if (item->kms_path != NULL)
    {
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
 * find_kek_in_cache_by_oid - 在缓存中按OID查找KEK
 *
 * 参数:
 *   kek_oid - KEK对象标识符
 *
 * 返回:
 *   找到的KEK缓存项指针，NULL表示未找到
 */
static TeccKekCacheItem *
find_kek_in_cache_by_oid(const TeccState *state, Oid kek_oid)
{
    TeccCacheManager *manager = NULL;
    KekHashKey key;
    bool found = false;
    void *entry = NULL;
    TeccKekCacheItem *item = NULL;
    int verify_ret;

    /* 获取缓存管理器 */
    manager = get_cache_manager(state);
    if (manager == NULL || !manager->initialized || manager->kek_cache == NULL) {
        TECC_CACHE_DEBUG("缓存管理器未初始化或无效，无法查找KEK（OID: %u）", kek_oid);
        return NULL;
    }

    /* 设置哈希键 */
    key.oid = kek_oid;

    /* 在哈希表中查找 */
    entry = fe_hash_search(manager->kek_cache, &key, HASH_FIND, &found);
    if (!found || entry == NULL) {
        return NULL;
    }

    item = *(TeccKekCacheItem **)entry;

    /* 验证HMAC签名（如果完整性验证已启用） */
    if (manager->integrity_enabled && item != NULL) {
        verify_ret = tecc_cache_verify_item(manager, item, 1); /* 1表示KEK类型 */
        if (verify_ret != TECC_CACHE_OK) {
            TECC_CACHE_DEBUG("KEK缓存项签名验证失败（OID: %u），错误码: %d", kek_oid, verify_ret);
            /* 签名验证失败，返回NULL表示缓存项无效 */
            return NULL;
        }
    }

    /* 返回缓存项指针 */
    return item;
}

/*
 * find_kek_in_cache_by_name - 在缓存中按名称查找KEK
 *
 * 参数:
 *   key_name - KEK名称
 *   key_namespace - 命名空间
 *
 * 返回:
 *   找到的KEK缓存项指针，NULL表示未找到
 */
static TeccKekCacheItem *
find_kek_in_cache_by_name(const TeccState *state, const char *key_name, const char *key_namespace)
{
    TeccCacheManager *manager = NULL;
    KekNameHashKey name_key;
    void *entry = NULL;
    TeccKekCacheItem *item = NULL;

    /* 参数检查 */
    if (key_name == NULL) {
        return NULL;
    }

    /* 获取缓存管理器 */
    manager = get_cache_manager(state);
    if (manager == NULL || !manager->initialized || manager->kek_name_cache == NULL) {
        TECC_CACHE_DEBUG("缓存管理器未初始化或无效，无法查找KEK（名称: %s）", key_name);
        return NULL;
    }

    /* 如果namespace为NULL，遍历所有缓存条目按名称匹配 */
    if (key_namespace == NULL) {
        FeHashSeqStatus scan;
        void *entry2;

        fe_hash_seq_init(&scan, manager->kek_name_cache);
        while ((entry2 = fe_hash_seq_search(&scan)) != NULL) {
            TeccKekCacheItem *cached = *(TeccKekCacheItem **)entry2;
            if (cached != NULL && cached->key_name != NULL &&
                strcmp(cached->key_name, key_name) == 0) {
                return cached;
            }
        }
        return NULL;
    }

    /* 构建名称键 */
    strncpy(name_key.key_name, key_name, NAMEDATALEN - 1);
    strncpy(name_key.key_namespace, key_namespace, NAMEDATALEN - 1);
    name_key.key_name[NAMEDATALEN - 1] = '\0';
    name_key.key_namespace[NAMEDATALEN - 1] = '\0';

    /* 直接查询名称哈希表 */
    entry = fe_hash_search(manager->kek_name_cache, &name_key, HASH_FIND, NULL);
    if (entry != NULL) {
        item = *(TeccKekCacheItem **)entry;
        return item;
    }

    /* 未找到匹配项 */
    return NULL;
}

/*
 * load_kek_from_database_by_oid - 从数据库按OID加载KEK信息
 *
 * 参数:
 *   kek_oid - KEK对象标识符
 *   item - 返回的KEK缓存项指针
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
static int
load_kek_from_database_by_oid(const TeccState *state, Oid kek_oid, TeccKekCacheItem **item)
{
    /* 桩实现：按OID查询KEK */
    /* 注意：调用者已改为通过名称查询KEK（使用DEK缓存项中的kek_name字段） */
    (void)state;
    (void)kek_oid;
    (void)item;

    return TECC_CACHE_ERROR_NOT_FOUND;
}

/*
 * load_kek_from_database_by_name - 从数据库按名称加载KEK信息
 *
 * 参数:
 *   key_name - KEK名称
 *   key_namespace - 命名空间
 *   item - 返回的KEK缓存项指针
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
static int
load_kek_from_database_by_name(const TeccState *state,
                               const char *key_name,
                               const char *key_namespace,
                               TeccKekCacheItem **item)
{
    /* 通过数据获取器适配器查询KEK元数据 */
    TeccCacheQueryParams query;
    TeccFetcherErrorCode fetcher_error;

    /* 参数检查 */
    if (state == NULL || key_name == NULL || item == NULL) {
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    /* 使用简化的适配器接口查询KEK元数据 */
    memset(&query, 0, sizeof(query));
    query.flags = TECC_QUERY_BY_NAME;
    query.params.by_name.name = key_name;
    query.params.by_name.namespace = key_namespace;

    fetcher_error = tecc_fetch_kek_for_cache(state, &query, item, NULL);
    if (fetcher_error != TECC_FETCHER_OK) {
        /* 查询失败，转换为缓存错误码 */
        return tecc_cache_convert_error(fetcher_error);
    }

    /* 检查是否找到KEK */
    if (*item == NULL) {
        return TECC_CACHE_ERROR_NOT_FOUND;
    }

    return TECC_CACHE_OK;
}


/*
 * insert_kek_into_cache - 将KEK缓存项插入到缓存中
 *
 * 参数:
 *   item - KEK缓存项指针
 *
 * 返回:
 *   0表示成功，非0表示错误码
 */
static int
insert_kek_into_cache(const TeccState *state, TeccKekCacheItem *item)
{
    TeccCacheManager *manager = NULL;
    KekHashKey oid_key;
    KekNameHashKey name_key;
    void *entry = NULL;
    bool found = false;
    int sign_ret;

    /* 参数检查 */
    if (item == NULL) {
        TECC_CACHE_DEBUG("无效参数：item为NULL");
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }
    if (item->kek_oid == InvalidOid && item->cache_status == TECC_CACHE_STATUS_ENCRYPTED) {
        TECC_CACHE_DEBUG("无效参数：非负缓存项缺少OID");
        return TECC_CACHE_ERROR_INVALID_PARAM;
    }

    /* 获取缓存管理器 */
    manager = get_cache_manager(state);
    if (manager == NULL || !manager->initialized) {
        TECC_CACHE_DEBUG("缓存管理器未初始化或无效");
        return TECC_CACHE_ERROR_NOT_INITIALIZED;
    }

    /* 0. 生成HMAC签名（跳过负缓存项）*/
    if (manager->integrity_enabled && item->cache_status == TECC_CACHE_STATUS_ENCRYPTED) {
        sign_ret = tecc_cache_sign_item(manager, item, 1);
        if (sign_ret != TECC_CACHE_OK) {
            TECC_CACHE_DEBUG("KEK缓存项签名失败（OID: %u），错误码: %d", item->kek_oid, sign_ret);
        }
    }

    /* 1. 插入到OID哈希表（负缓存项无有效OID则跳过）*/
    if (item->kek_oid != InvalidOid && manager->kek_cache != NULL) {
        oid_key.oid = item->kek_oid;
        entry = fe_hash_search(manager->kek_cache, &oid_key, HASH_ENTER, &found);
        if (entry == NULL) {
            return TECC_CACHE_ERROR_MEMORY;
        }
        *(TeccKekCacheItem **)entry = item;
    }

    /* 2. 插入到名称哈希表（如果名称可用） */
    if (manager->kek_name_cache != NULL && item->key_name != NULL && item->key_namespace != NULL) {
        /* 构建名称键 */
        strncpy(name_key.key_name, item->key_name, NAMEDATALEN - 1);
        strncpy(name_key.key_namespace, item->key_namespace, NAMEDATALEN - 1);
        name_key.key_name[NAMEDATALEN - 1] = '\0';
        name_key.key_namespace[NAMEDATALEN - 1] = '\0';

        entry = fe_hash_search(manager->kek_name_cache, &name_key, HASH_ENTER, &found);
        if (entry != NULL) {
            /* 指向同一个缓存项 */
            *(TeccKekCacheItem **)entry = item;
            TECC_CACHE_DEBUG("成功插入KEK到名称缓存: name=%s, namespace=%s, oid=%u",
                item->key_name, item->key_namespace, item->kek_oid);
        } else {
            TECC_CACHE_DEBUG("名称哈希表插入失败: name=%s, namespace=%s",
                item->key_name, item->key_namespace);
        }
        /* 注意：名称哈希表插入失败不影响整体功能，可以继续 */
    } else {
        TECC_CACHE_DEBUG("无法插入到名称缓存: kek_name_cache=%p, key_name=%p, key_namespace=%p",
            manager->kek_name_cache, item->key_name, item->key_namespace);
    }

    /* 3. 更新缓存统计信息 */
    if (!found) {
        manager->current_size++;
    }

    return TECC_CACHE_OK;
}


/*
 * tecc_cache_get_kek_by_name_search_path - 按名称查找KEK（search_path包装）
 *
 * 按current_schema → search_path_schemas[]顺序遍历查找KEK。
 */
TeccKekCacheItem *
tecc_cache_get_kek_by_name_search_path(const TeccState *state, const char *key_name)
{
	const char *current_schema;
	const char **schemas;
	int nschemas, i;
	TeccKekCacheItem *item;

	/* 0. 处理 schema.key_name 限定名 */
	{
		const char *dot = strchr(key_name, '.');
		if (dot != NULL)
		{
			char *schema_name;
			size_t schema_len = dot - key_name;

			schema_name = (char *)pg_malloc(schema_len + 1);
			memcpy(schema_name, key_name, schema_len);
			schema_name[schema_len] = '\0';
			item = tecc_cache_get_kek_by_name(state, dot + 1, schema_name);
			if (item != NULL)
			{
				pg_free(schema_name);
				return item;
			}
			TECC_LOG(TECC_LOG_WARNING, "KEK qualified name '%s': schema '%s', key '%s' not found",
					 key_name, schema_name, dot + 1);
			pg_free(schema_name);
			/* 限定名精确查找失败，不回退到 search_path */
			return NULL;
		}
	}

	/* 1. 先查current_schema */
	current_schema = tecc_cache_get_guc_param(state, "current_schema");
	if (current_schema != NULL)
	{
		item = tecc_cache_get_kek_by_name(state, key_name, current_schema);
		if (item != NULL)
			return item;
	}

	/* 2. 遍历search_path_schemas */
	schemas = tecc_cache_get_search_path_schemas(state, &nschemas);
	for (i = 0; i < nschemas; i++)
	{
		if (schemas[i] == NULL)
			continue;
		item = tecc_cache_get_kek_by_name(state, key_name, schemas[i]);
		if (item != NULL)
			return item;
	}

	TECC_LOG(TECC_LOG_DEBUG1, "KEK search_path: key '%s' not found in current_schema/search_path (search_path not configured)",
			 key_name);
	return NULL;
}

/*
 * tecc_cache_insert_kek_item - 公开的KEK缓存项插入接口
 *
 * 用于批量加载或PENDING转正式。自动处理OID/名称哈希表插入。
 */
int
tecc_cache_insert_kek_item(TeccState *state, TeccKekCacheItem *item)
{
    const TeccState *cstate = (const TeccState *)state;
    return insert_kek_into_cache(cstate, item);
}

/*
 * tecc_cache_remove_pending_kek - 移除PENDING KEK缓存项
 *
 * 在CREATE KEK失败后调用。只从名称哈希表中移除。
 */
void
tecc_cache_remove_pending_kek(TeccState *state, const char *key_name, const char *key_namespace)
{
    TeccCacheManager *manager;
    KekNameHashKey name_key;
    void *entry;

    if (state == NULL || key_name == NULL || key_namespace == NULL)
        return;

    manager = get_cache_manager((const TeccState *)state);
    if (manager == NULL || manager->kek_name_cache == NULL)
        return;

    memset(&name_key, 0, sizeof(name_key));
    strncpy(name_key.key_name, key_name, NAMEDATALEN - 1);
    strncpy(name_key.key_namespace, key_namespace, NAMEDATALEN - 1);

    CACHE_LOCK(manager);
    entry = fe_hash_search(manager->kek_name_cache, &name_key, HASH_REMOVE, NULL);
    if (entry != NULL)
    {
        TeccKekCacheItem *item = *(TeccKekCacheItem **)entry;
        if (item != NULL)
            destroy_kek_cache_item(item);
        pg_free(entry);
    }
    CACHE_UNLOCK(manager);
}

/* End - ReqID:TECC4.0 */