/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_key_manager_adapter.c
 *
 * Abstract:
 *   Key manager adapter implementation for TECC crypto scheduler module.
 *   Provides unified interface to key management module.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_scheduler/tecc_key_manager_adapter.c
 */

#include "tecc_key_manager_adapter.h"
#include "tecc_crypto_scheduler.h"  /* For error codes */
#include "../tecc_processor/internal/tecc_processor_internal.h"
#include "../../../common/tecc_key/security.h"  /* For real encryption functions */
#include "../../../common/tecc_key/high_level_api.h"  /* High-level API for key management */
#include "../../../common/tecc_key/key_manager.h"      /* Key manager functions */
#include "../tecc_hsm_adapter/fe-hsm-adapter.h"  /* For HSM adapter */
#include "../../../common/tecc_types.h"  /* For error code definitions */
#include "../tecc_cache/tecc_metadata_cache.h"  /* For cache functions */
#include "../../../../src/include/common/tecc_algorithm/tecc_sm4_rnd.h"  /* ReqID:TECC4.0 - derive keys */
#include <string.h>
#include <time.h>

/* Frontend memory allocation functions */
#include "../../../../src/include/common/fe_memutils.h"

/* Global key manager adapter instance (deprecated - for backward compatibility) */
/* Note: Global variables removed for thread safety - use connection-specific adapters */
/* static TeccKeyManagerAdapter *g_key_adapter = NULL; */
/* static int g_last_error = 0; */

/*
 * Create key manager adapter
 */
TeccKeyManagerAdapter *
tecc_key_manager_adapter_create(void)
{
    TeccKeyManagerAdapter *adapter;

    adapter = pg_malloc0(sizeof(TeccKeyManagerAdapter));
    if (!adapter)
    {
        tecc_key_set_last_error(NULL, TECC_CRYPTO_MEMORY_ERROR);
        return NULL;
    }

    /* Initialize cache */
    adapter->max_cache_size = 16;
    adapter->cache_enabled = true;
    adapter->last_error = TECC_CRYPTO_SUCCESS;
    adapter->key_cache = pg_malloc0(sizeof(TeccKeyCacheEntry) * adapter->max_cache_size);
    if (!adapter->key_cache)
    {
        pg_free(adapter);
        tecc_key_set_last_error(NULL, TECC_CRYPTO_MEMORY_ERROR);
        return NULL;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Created key manager adapter");

    return adapter;
}

/*
 * Create key manager adapter with TeccState
 */
TeccKeyManagerAdapter *
tecc_key_manager_adapter_create_with_state(void *tecc_state)
{
    TeccKeyManagerAdapter *adapter;

    adapter = pg_malloc0(sizeof(TeccKeyManagerAdapter));
    if (!adapter)
    {
        tecc_key_set_last_error(NULL, TECC_CRYPTO_MEMORY_ERROR);
        return NULL;
    }

    /* Initialize cache */
    adapter->max_cache_size = 16;
    adapter->cache_enabled = true;
    adapter->last_error = TECC_CRYPTO_SUCCESS;
    adapter->tecc_state = tecc_state;  /* Store TeccState for cache access */
    adapter->key_cache = pg_malloc0(sizeof(TeccKeyCacheEntry) * adapter->max_cache_size);
    if (!adapter->key_cache)
    {
        pg_free(adapter);
        tecc_key_set_last_error(NULL, TECC_CRYPTO_MEMORY_ERROR);
        return NULL;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Created key manager adapter with TeccState");

    return adapter;
}

/*
 * Destroy key manager adapter
 */
void
tecc_key_manager_adapter_destroy(TeccKeyManagerAdapter *adapter)
{
    int i;

    if (!adapter)
        return;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Destroying key manager adapter");

    /* Clear key cache */
    if (adapter->key_cache)
    {
        for (i = 0; i < adapter->cache_size; i++)
        {
            if (adapter->key_cache[i].key_data)
            {
                /* Securely wipe key data */
                memset(adapter->key_cache[i].key_data,
                       0,
                       adapter->key_cache[i].key_length);
                pg_free(adapter->key_cache[i].key_data);
            }
        }
        pg_free(adapter->key_cache);
    }

    memset(adapter, 0, sizeof(TeccKeyManagerAdapter));
    pg_free(adapter);

    /* Note: Global variable g_key_adapter removed for thread safety */
}

/*
 * Initialize key manager adapter
 */
bool
tecc_key_manager_adapter_init(TeccKeyManagerAdapter *adapter, void *key_manager)
{
    if (!adapter)
        return false;

    adapter->key_manager = key_manager;

    /* Allocate cache array */
    if (adapter->cache_enabled && adapter->max_cache_size > 0)
    {
        adapter->key_cache = pg_malloc0(
            adapter->max_cache_size * sizeof(TeccKeyCacheEntry));
        if (!adapter->key_cache)
        {
            tecc_key_set_last_error(adapter, TECC_CRYPTO_MEMORY_ERROR);
            return false;
        }
    }

    /* Note: Global variable g_key_adapter removed for thread safety */

    /* Temporarily disable debug log to avoid segmentation fault */
    /* tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Initialized key manager adapter with cache size %d",
                        adapter->max_cache_size); */

    return true;
}

/*
 * Get KEK key (backward compatibility version)
 */
unsigned char *
tecc_key_adapter_get_kek(const char *kek_name, size_t *key_len)
{
    /* Note: Global variable g_key_adapter removed for thread safety */
    /* This function now creates a temporary adapter for backward compatibility */
    TeccKeyManagerAdapter *temp_adapter = tecc_key_manager_adapter_create();
    unsigned char *result = NULL;

    if (temp_adapter)
    {
        tecc_key_manager_adapter_init(temp_adapter, NULL);
        result = tecc_key_get_kek_with_adapter(temp_adapter, kek_name, key_len);
        tecc_key_manager_adapter_destroy(temp_adapter);
    }

    return result;
}

/*
 * Get KEK key with adapter parameter (thread-safe version)
 */
unsigned char *
tecc_key_get_kek_with_adapter(TeccKeyManagerAdapter *adapter,
                              const char *kek_name, size_t *key_len)
{
    unsigned char *key_data;

    if (!key_len)
    {
        tecc_key_set_last_error(adapter, TECC_CRYPTO_INVALID_PARAM);
        return NULL;
    }

    *key_len = 0;

    /* KEK name is required */
    if (!kek_name || strlen(kek_name) == 0)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "KEK name is NULL or empty, cannot get KEK");
        tecc_key_set_last_error(adapter, TECC_CRYPTO_INVALID_PARAM);
        return NULL;
    }

    /* ReqID:TECC4.0 — KEK is cached at metadata layer (TeccKekCacheItem.plain_kek).
     * Adapter key_cache is DEK-only. Load KEK directly via state. */

    /* Call real key management implementation */
    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Getting KEK from real key manager: %s", kek_name);

    /* Use new function with TeccState - tecc_state must be available */
    if (adapter && adapter->tecc_state)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Using tecc_key_get_kek_with_state, tecc_state=%p", adapter->tecc_state);
        key_data = tecc_key_get_kek_with_state(adapter->tecc_state, kek_name, key_len);
    }
    else
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "TeccState is NULL in adapter, cannot get KEK: %s", kek_name);
        tecc_key_set_last_error(adapter, TECC_CRYPTO_INVALID_PARAM);
        return NULL;
    }

    if (key_data)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Successfully got KEK from real key manager: %s (len=%zu)",
                           kek_name, *key_len);
        return key_data;
    }
    else
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "Failed to get KEK from real key manager: %s", kek_name);
        tecc_key_set_last_error(adapter, TECC_CRYPTO_KEY_ERROR);
        return NULL;
    }
}


/*
 * ReqID:TECC4.0 — DEK 解密失败负缓存辅助函数。
 *
 * KEK 明文是连接级缓存（连接建立/首次使用加载一次），同一连接内某 DEK 的
 * 解密结果不变：一旦因 KEK 密钥文件被替换/覆盖而解密失败，之后对每个列、
 * 每行的重试都必然失败。若不记录，SELECT 一张多列/多行加密表会为每格刷屏
 * 同样的报错（Bug: tecc_ktool --force 覆盖 KEK 后报错过多）。
 *
 * 负缓存生命周期 = adapter 生命周期 = 连接生命周期，与"KEK 明文连接级固定"
 * 一致，因此无需随语句重置。
 */
static bool
tecc_adapter_dek_has_failed(TeccKeyManagerAdapter *adapter, Oid dek_oid)
{
    int i;

    if (!adapter)
        return false;
    for (i = 0; i < adapter->num_failed_deks && i < 16; i++)
    {
        if (adapter->failed_dek_oids[i] == dek_oid)
            return true;
    }
    return false;
}

/*
 * ReqID:TECC4.0 — 标记某 DEK 本连接解密失败。
 * 返回 true 表示首次标记（调用方应输出一次用户可见报错）；
 * 返回 false 表示已标记过（重复失败，静默短路即可）。
 */
static bool
tecc_adapter_dek_mark_failed(TeccKeyManagerAdapter *adapter, Oid dek_oid)
{
    if (!adapter)
        return true;
    if (tecc_adapter_dek_has_failed(adapter, dek_oid))
        return false;
    if (adapter->num_failed_deks < 16)
        adapter->failed_dek_oids[adapter->num_failed_deks++] = dek_oid;
    return true;
}

/*
 * Get DEK key with adapter parameter (thread-safe version)
 */
unsigned char *
tecc_key_get_dek_with_adapter(TeccKeyManagerAdapter *adapter,
                              const char *dek_name, size_t *key_len,
                              void *tecc_state)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    unsigned char *decrypted_dek = NULL;
    TeccDekCacheItem *dek_item = NULL;
    unsigned char *kek = NULL;
    size_t kek_len = 0;


    TECC_LOG(TECC_LOG_DEBUG1, "DEK_WITH_ADAPTER: ENTER, dek_name='%s', adapter=%p, cache_enabled=%d",
            dek_name ? dek_name : "(null)", adapter,
            adapter ? adapter->cache_enabled : -1);

    if (!key_len)
    {
        tecc_key_set_last_error(adapter, TECC_CRYPTO_INVALID_PARAM);
        return NULL;
    }

    *key_len = 0;

    /* DEK name is required */
    if (!dek_name || strlen(dek_name) == 0)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "DEK name is NULL or empty, cannot get DEK");
        tecc_key_set_last_error(adapter, TECC_CRYPTO_INVALID_PARAM);
        return NULL;
    }

    /* Call real key management implementation */
    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Getting DEK from real key manager: %s", dek_name);

    /* Try to use cache interface if TeccState is available */
    if (adapter && adapter->tecc_state)
    {
        TeccDekCacheItem *dek_cache_item;
        TECC_LOG(TECC_LOG_DEBUG1, "DEK CACHE GET: before search_path lookup, dek_name='%s'", dek_name);
        dek_cache_item = tecc_cache_get_dek_by_name_search_path((TeccState *)adapter->tecc_state, dek_name);
        TECC_LOG(TECC_LOG_DEBUG1, "DEK CACHE GET: search_path returned dek_cache_item=%p, kek_name='%s'",
                dek_cache_item,
                dek_cache_item ? (dek_cache_item->kek_name ? dek_cache_item->kek_name : "(null)") : "N/A");
        fflush(stderr);
        if (dek_cache_item && dek_cache_item->encrypted_key)
        {
            /* ReqID:TECC4.0 — check adapter key_cache by OID (globally unique) */
            if (adapter->cache_enabled)
            {
                decrypted_dek = tecc_key_cache_get(adapter, dek_cache_item->dek_oid, key_len);
                if (decrypted_dek)
                {
                    return decrypted_dek;
                }

                /* ReqID:TECC4.0 — 负缓存命中：本连接内该 DEK 已解密失败过
                 * （KEK 密钥文件被覆盖等）。KEK 明文连接级固定，重试必然
                 * 再次失败并刷屏，直接短路（静默，首次失败已报错）。 */
                if (tecc_adapter_dek_has_failed(adapter, dek_cache_item->dek_oid))
                {
                    tecc_key_set_last_error(adapter, TECC_CRYPTO_KEY_ERROR);
                    return NULL;
                }
            }

            /* ReqID:TECC4.0 - Bug 9256: KEK 可创建在任意 schema，而 DEK 目录里记的是
             * KEK 的 OID。原先只用 DEK 上的裸 KEK 名按 search_path 查找，KEK 不在
             * search_path 上时必然失败（在 public 下用 sch1.kek 建的表 INSERT 报
             * "Failed to get KEK by name"）。改按 OID 解析（无歧义），无 OID 时才退回按名。 */
            kek = tecc_key_get_kek_by_oid_or_name((void *)adapter->tecc_state,
                                                  dek_cache_item->kek_oid,
                                                  dek_cache_item->kek_name,
                                                  &kek_len);
            if (!kek)
            {
                /* ReqID:TECC4.0 — Bug: tecc_ktool delete 删除 KEK 文件后重连查询报错。
                 * 该日志是纯诊断（KEK 加载失败已由 get_kek 内部 set_last_error 给出可执行
                 * 信息，INSERT 加密路径经调度器报错，SELECT 解密路径在调度器层被归类为
                 * skipped_decrypt 静默返回密文）；原 ERROR 会在每个列/行解密时刷屏，
                 * 降为 DEBUG。同时把本 DEK 记入负缓存：KEK 文件缺失在本连接内恒定，
                 * 重试必然再次失败，直接短路避免重复尝试。需要深度诊断时
                 * TECC_LOG_LEVEL=DEBUG 仍可见。 */
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "Failed to get KEK (oid=%u, name='%s') for DEK: %s",
                                   dek_cache_item->kek_oid,
                                   dek_cache_item->kek_name ? dek_cache_item->kek_name : "(unknown)",
                                   dek_name);
                tecc_adapter_dek_mark_failed(adapter, dek_cache_item->dek_oid);
                tecc_key_set_last_error(adapter, TECC_CRYPTO_KEY_ERROR);
                return NULL;
            }

            /* ReqID:TECC4.0 — 直接解密DEK，避免tecc_key_get_decrypted_dek()内部二次加载KEK */
            {
                int dek_ret = tecc_key_decrypt_dek(dek_cache_item->encrypted_key,
                                                    dek_cache_item->key_length,
                                                    kek, kek_len,
                                                    &decrypted_dek, key_len);
                tecc_key_secure_erase(kek, kek_len);
                pg_free(kek);
                if (dek_ret != 0)
                {
                    /* ReqID:TECC4.0 — Bug: tecc_ktool --force 覆盖 KEK 后报错过多。
                     * 原先每次失败打印 "Failed to decrypt DEK"(含 OID/长度) + 2 条
                     * DIAG（对比 KEK/DEK create_date，而该字段从未被填充、恒为
                     * 1970-01-01，纯误导），且每个列/每行重复整段刷屏。
                     * 现改为：同一 DEK 本连接只报一次、一条可执行的简洁错误。 */
                    if (tecc_adapter_dek_mark_failed(adapter, dek_cache_item->dek_oid))
                    {
                        tecc_processor_debug(TECC_LOG_ERROR,
                                           "Failed to decrypt DEK '%s': the KEK '%s' key file "
                                           "no longer matches this DEK (key material was changed/"
                                           "replaced after the DEK was created). Restore the "
                                           "original KEK key file and reconnect, or DROP and "
                                           "recreate the DEK.",
                                           dek_name,
                                           dek_cache_item->kek_name ? dek_cache_item->kek_name : "(unknown)");
                    }
                    tecc_key_set_last_error(adapter, TECC_CRYPTO_KEY_ERROR);
                    return NULL;
                }
            }

            /* 添加到缓存 */
            if (adapter && adapter->cache_enabled)
            {
                TECC_LOG(TECC_LOG_DEBUG1, "TECC_DEBUG: before tecc_key_cache_add, dek_name=%s, decrypted_dek=%p, key_len=%zu",
                        dek_name, decrypted_dek, *key_len);
                tecc_key_cache_add(adapter, dek_cache_item->dek_oid,
                                   dek_name, decrypted_dek, *key_len);
            }

            TECC_LOG(TECC_LOG_DEBUG1, "TECC_DEBUG: returning decrypted_dek=%p", decrypted_dek);
            return decrypted_dek;
        }
        tecc_processor_debug(TECC_LOG_WARNING,
                           "Failed to get DEK from cache interface: %s, trying old method",
                           dek_name);
    }

    /* 调试：检查tecc_state结构 */
    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "DEBUG: tecc_state=%p, adapter=%p, adapter->tecc_state=%p",
                       tecc_state, adapter, adapter ? adapter->tecc_state : NULL);

    /* 检查tecc_state是否为NULL */
    if (!tecc_state)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "tecc_state is NULL for DEK lookup: %s",
                           dek_name);
        tecc_key_set_last_error(adapter, TECC_CRYPTO_INVALID_PARAM);
        return NULL;
    }

    dek_item = tecc_cache_get_dek_by_name_search_path((const TeccState *)tecc_state, dek_name);
    if (!dek_item)
    {
        tecc_processor_debug(TECC_LOG_ERROR,
                           "DEK not found in cache: %s", dek_name);
        tecc_key_set_last_error(adapter, TECC_CRYPTO_KEY_ERROR);
        return NULL;
    }

    /* ReqID:TECC4.0 — check adapter key_cache by OID before decrypting */
    if (adapter && adapter->cache_enabled)
    {
        decrypted_dek = tecc_key_cache_get(adapter, dek_item->dek_oid, key_len);
        if (decrypted_dek)
        {
            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "DEK found in adapter cache: oid=%u", dek_item->dek_oid);
            return decrypted_dek;
        }

        /* ReqID:TECC4.0 — 负缓存短路（同上方缓存路径），避免重复解密刷屏 */
        if (tecc_adapter_dek_has_failed(adapter, dek_item->dek_oid))
        {
            tecc_key_set_last_error(adapter, TECC_CRYPTO_KEY_ERROR);
            return NULL;
        }
    }

    /* 2. 获取 DEK 绑定的 KEK（ReqID:TECC4.0 - Bug 9256：按 OID 解析，跨 schema 无歧义） */
    kek = tecc_key_get_kek_by_oid_or_name((void *)tecc_state, dek_item->kek_oid,
                                          dek_item->kek_name, &kek_len);
    if (!kek)
    {
        /* ReqID:TECC4.0 — Bug: tecc_ktool delete 删除 KEK 文件后重连查询报错。
         * 同上方缓存路径：诊断日志降为 DEBUG + 本 DEK 记负缓存短路，SELECT
         * 解密路径静默返回密文，用户可见错误由 INSERT/DDL 上层给出。 */
        tecc_processor_debug(TECC_LOG_DEBUG1,
                           "Failed to get KEK (oid=%u, name='%s') for DEK: %s",
                           dek_item->kek_oid,
                           dek_item->kek_name ? dek_item->kek_name : "(unknown)", dek_name);
        tecc_adapter_dek_mark_failed(adapter, dek_item->dek_oid);
        tecc_key_set_last_error(adapter, TECC_CRYPTO_KEY_ERROR);
        return NULL;
    }

    /* 4. 使用tecc_key_get_decrypted_dek解密DEK */
    decrypted_dek = tecc_key_get_decrypted_dek(tecc_state, dek_name, key_len);
    if (!decrypted_dek)
    {
        /* ReqID:TECC4.0 — 同缓存路径：只报一次简洁错误 */
        if (tecc_adapter_dek_mark_failed(adapter, dek_item->dek_oid))
        {
            tecc_processor_debug(TECC_LOG_ERROR,
                               "Failed to decrypt DEK '%s': the KEK '%s' key file "
                               "no longer matches this DEK (key material was changed/"
                               "replaced after the DEK was created). Restore the "
                               "original KEK key file and reconnect, or DROP and "
                               "recreate the DEK.",
                               dek_name,
                               dek_item->kek_name ? dek_item->kek_name : "(unknown)");
        }
        tecc_key_secure_erase(kek, kek_len);
        pg_free(kek);
        tecc_key_set_last_error(adapter, TECC_CRYPTO_KEY_ERROR);
        return NULL;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                       "Successfully decrypted DEK: %s (len=%zu)",
                       dek_name, *key_len);

    /* 安全清理KEK */
    tecc_key_secure_erase(kek, kek_len);
    pg_free(kek);

    /* Add to cache if caching is enabled */
    if (adapter && adapter->cache_enabled)
    {
        tecc_key_cache_add(adapter, dek_item->dek_oid,
                           dek_name, decrypted_dek, *key_len);
    }

    return decrypted_dek;
}


/*
 * Validate key
 */
bool
tecc_key_validate(const char *key_name, TeccKeyType key_type)
{
    if (!key_name || strlen(key_name) == 0)
        return false;

    /* Call actual key validation functions based on key type */
    switch (key_type)
    {
        case TECC_KEY_KEK:
            return tecc_key_validate_kek(key_name);
        case TECC_KEY_DEK:
        case TECC_KEY_DEK_ENCRYPTED:
            return tecc_key_validate_dek(key_name);
        default:
            tecc_processor_debug(TECC_LOG_WARNING,
                               "Unknown key type for validation: %d", key_type);
            return false;
    }
}

/*
 * Check if key is expired
 */
bool
tecc_key_is_expired(const char *key_name)
{
    KeyManager *manager = NULL;
    PgTeccDekInfo *dek_info = NULL;
    bool is_expired = false;

    if (!key_name)
        return true;

    /* For KEK keys, they never expire in current implementation */
    /* For DEK keys, check if they are active */
    manager = key_manager_create();
    if (manager == NULL)
        return true;  /* If we can't create manager, treat as expired */

    dek_info = key_manager_get_dek_info(manager, key_name);
    if (dek_info != NULL)
    {
        /* Check if DEK is active */
        if (strcmp(dek_info->status, "active") != 0)
            is_expired = true;

        key_manager_free_dek_info(dek_info);
    }
    else
    {
        /* Key not found, treat as expired */
        is_expired = true;
    }

    key_manager_destroy(manager);
    return is_expired;
}

/*
 * Get key information
 */
TeccKeyInfo *
tecc_key_get_info(const char *key_name)
{
    TeccKeyInfo *info;
    time_t now;

    if (!key_name)
        return NULL;

    info = pg_malloc0(sizeof(TeccKeyInfo));
    if (!info)
        return NULL;

    strlcpy(info->key_name, key_name, NAMEDATALEN);
    info->is_valid = tecc_key_validate(key_name, TECC_KEY_DEK);
    info->created_time = time(&now);
    info->expires_time = info->created_time + (365 * 24 * 3600);  /* 1 year */

    return info;
}

/*
 * Enable/disable key cache
 */
bool
tecc_key_cache_enable(TeccKeyManagerAdapter *adapter, bool enable)
{
    if (!adapter)
        return false;

    adapter->cache_enabled = enable;

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Key cache %s", enable ? "enabled" : "disabled");

    return true;
}

/*
 * Add key to cache
 */
bool
tecc_key_cache_add(TeccKeyManagerAdapter *adapter, Oid key_oid,
                    const char *key_name,
                    const unsigned char *key_data, size_t key_len)
{
    int i;
    time_t oldest_time = 0;
    int oldest_idx = -1;
    TeccKeyCacheEntry *entry;
    unsigned char *key_copy;

    if (!adapter || !key_name || !key_data || key_len == 0)
        return false;

    if (!adapter->cache_enabled || !adapter->key_cache)
        return false;

    /* Check if OID already exists in cache (should not happen normally) */
    for (i = 0; i < adapter->max_cache_size; i++)
    {
        if (adapter->key_cache[i].key_data != NULL &&
            adapter->key_cache[i].key_info.key_oid == key_oid)
        {
            /* Same OID already cached — update content */
            entry = &adapter->key_cache[i];
            memset(entry->key_data, 0, entry->key_length);
            pg_free(entry->key_data);
            goto fill_entry;
        }
    }

    /* Find free slot or oldest entry */
    entry = NULL;

    for (i = 0; i < adapter->max_cache_size; i++)
    {
        if (adapter->key_cache[i].key_data == NULL)
        {
            entry = &adapter->key_cache[i];
            break;
        }

        if (oldest_idx == -1 || adapter->key_cache[i].last_access < oldest_time)
        {
            oldest_time = adapter->key_cache[i].last_access;
            oldest_idx = i;
        }
    }

    /* Use oldest slot if no free slot */
    if (!entry && oldest_idx >= 0)
    {
        entry = &adapter->key_cache[oldest_idx];
        /* Clear old entry */
        if (entry->key_data)
        {
            memset(entry->key_data, 0, entry->key_length);
            pg_free(entry->key_data);
        }
    }

    if (!entry)
        return false;

fill_entry:
    /* Copy key data */
    key_copy = pg_malloc(key_len);
    if (!key_copy)
    {
        tecc_key_set_last_error(adapter, TECC_CRYPTO_CACHE_ERROR);
        return false;
    }
    memcpy(key_copy, key_data, key_len);

    /* Fill entry */
    entry->key_info.key_oid = key_oid;
    strlcpy(entry->key_info.key_name, key_name, NAMEDATALEN);
    entry->key_info.key_type = TECC_KEY_DEK;
    entry->key_info.is_valid = true;
    entry->key_data = key_copy;
    entry->key_length = key_len;
    entry->ref_count = 1;
    time(&entry->last_access);

    /* ReqID:TECC4.0 — pre-derive SM4-RND keys from DEK to avoid per-row HKDF.
     * Must use TECC_SM4_KEY_SIZE (16), not key_len: encrypt/decrypt always pass
     * TECC_SM4_KEY_SIZE to derive_keys regardless of actual DEK length. */
    if (key_data && key_len >= TECC_SM4_KEY_SIZE)
    {
        tecc_sm4_rnd_derive_keys(key_data, TECC_SM4_KEY_SIZE,
                                 entry->derived_enc_key,
                                 entry->derived_auth_key);
        entry->derived_keys_valid = true;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Added DEK to cache: %s (oid=%u, len=%zu)", key_name, key_oid, key_len);

    return true;
}

/*
 * Get key from cache
 */
unsigned char *
tecc_key_cache_get(TeccKeyManagerAdapter *adapter, Oid key_oid,
                   size_t *key_len)
{
    int i;
    unsigned char *key_copy;

    if (!adapter || !key_len || key_oid == InvalidOid)
        return NULL;

    *key_len = 0;

    if (!adapter->cache_enabled || !adapter->key_cache)
        return NULL;

    /* Find key in cache by OID */
    for (i = 0; i < adapter->max_cache_size; i++)
    {
        if (adapter->key_cache[i].key_data &&
            adapter->key_cache[i].key_info.key_oid == key_oid)
        {
            /* Found - increment ref count and update access time */
            adapter->key_cache[i].ref_count++;
            time(&adapter->key_cache[i].last_access);

            /* Return copy of key */
            *key_len = adapter->key_cache[i].key_length;
            key_copy = pg_malloc(*key_len);
            if (key_copy)
            {
                memcpy(key_copy, adapter->key_cache[i].key_data, *key_len);
                tecc_processor_debug(TECC_LOG_DEBUG1,
                                   "DEK found in cache: oid=%u", key_oid);
                return key_copy;
            }
            else
            {
                tecc_key_set_last_error(adapter, TECC_CRYPTO_CACHE_ERROR);
                return NULL;
            }
        }
    }

    return NULL;
}

/*
 * Clear key cache
 */
void
tecc_key_cache_clear(TeccKeyManagerAdapter *adapter)
{
    int i;

    if (!adapter || !adapter->key_cache)
        return;

    for (i = 0; i < adapter->max_cache_size; i++)
    {
        if (adapter->key_cache[i].key_data)
        {
            /* Securely wipe key data */
            memset(adapter->key_cache[i].key_data,
                   0,
                   adapter->key_cache[i].key_length);
            pg_free(adapter->key_cache[i].key_data);
            adapter->key_cache[i].key_data = NULL;
            adapter->key_cache[i].key_length = 0;
            adapter->key_cache[i].ref_count = 0;
        }
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
                        "Key cache cleared");
}

/*
 * Expire key from cache
 */
void
tecc_key_cache_expire(TeccKeyManagerAdapter *adapter, Oid key_oid)
{
    int i;

    if (!adapter || key_oid == InvalidOid || !adapter->key_cache)
        return;

    for (i = 0; i < adapter->max_cache_size; i++)
    {
        if (adapter->key_cache[i].key_data &&
            adapter->key_cache[i].key_info.key_oid == key_oid)
        {
            /* Clear entry */
            memset(adapter->key_cache[i].key_data, 0,
                   adapter->key_cache[i].key_length);
            pg_free(adapter->key_cache[i].key_data);
            adapter->key_cache[i].key_data = NULL;
            adapter->key_cache[i].key_length = 0;
            adapter->key_cache[i].ref_count = 0;
            adapter->key_cache[i].derived_keys_valid = false;

            tecc_processor_debug(TECC_LOG_DEBUG1,
                               "Expired DEK from cache: oid=%u", key_oid);
            break;
        }
    }
}

/*
 * Convert key type to string
 */
const char *
tecc_key_type_to_string(TeccKeyType key_type)
{
    switch (key_type)
    {
        case TECC_KEY_KEK:
            return "KEK";
        case TECC_KEY_DEK:
            return "DEK";
        case TECC_KEY_DEK_ENCRYPTED:
            return "DEK_ENCRYPTED";
        default:
            return "UNKNOWN";
    }
}

/*
 * Validate key name
 */
bool
tecc_key_name_is_valid(const char *key_name)
{
    int i;
    size_t len;

    if (!key_name)
        return false;

    len = strlen(key_name);
    if (len == 0 || len >= NAMEDATALEN)
        return false;

    /* Check characters */
    for (i = 0; i < len; i++)
    {
        char c = key_name[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '-'))
            return false;
    }

    return true;
}

/*
 * Generate default key name
 */
char *
tecc_key_generate_default_name(TeccKeyType key_type)
{
    char *name;
    time_t now;
    struct tm *tm_info;
    char time_str[32];

    name = pg_malloc(NAMEDATALEN);
    if (!name)
        return NULL;

    time(&now);
    tm_info = localtime(&now);
    strftime(time_str, sizeof(time_str), "%Y%m%d_%H%M%S", tm_info);

    switch (key_type)
    {
        case TECC_KEY_KEK:
            snprintf(name, NAMEDATALEN, "kek_default_%s", time_str);
            break;
        case TECC_KEY_DEK:
            snprintf(name, NAMEDATALEN, "dek_default_%s", time_str);
            break;
        default:
            snprintf(name, NAMEDATALEN, "key_default_%s", time_str);
            break;
    }

    return name;
}

/*
 * Set last error for adapter
 */
void
tecc_key_set_last_error(TeccKeyManagerAdapter *adapter, int error_code)
{
    if (adapter)
    {
        adapter->last_error = error_code;
    }
    else
    {
        /* Note: Global variable g_last_error removed for thread safety */
        /* If no adapter provided, error is not stored */
    }
}

/*
 * Get last error for adapter
 */
const char *
tecc_key_get_last_error_with_adapter(TeccKeyManagerAdapter *adapter)
{
    int error_code;

    if (adapter)
    {
        error_code = adapter->last_error;
    }
    else
    {
        /* Note: Global variable g_last_error removed for thread safety */
        /* If no adapter provided, return default error message */
        error_code = TECC_CRYPTO_SYSTEM_ERROR;
    }

    /* Return error message based on error code */
    switch (error_code)
    {
        case TECC_CRYPTO_SUCCESS:
            return "Success";
        case TECC_CRYPTO_INVALID_PARAM:
            return "Invalid parameter";
        case TECC_CRYPTO_KEY_ERROR:
            return "Key error";
        case TECC_CRYPTO_ALGO_ERROR:
            return "Algorithm error";
        case TECC_CRYPTO_MEMORY_ERROR:
            return "Memory error";
        case TECC_CRYPTO_DATA_ERROR:
            return "Data error";
        case TECC_CRYPTO_SCHEDULE_ERROR:
            return "Schedule error";
        case TECC_CRYPTO_SYSTEM_ERROR:
            return "System error";
        case TECC_CRYPTO_COLLATION_ERROR:
            return "Collation error";
        case TECC_CRYPTO_FORMAT_ERROR:
            return "Format error";
        case TECC_CRYPTO_CACHE_ERROR:
            return "Cache error";
        default:
            return "Unknown error";
    }
}

/*
 * Get error message
 */
const char *
tecc_key_get_error_message(int error_code)
{
    switch (error_code)
    {
        case TECC_CRYPTO_SUCCESS:
            return "Success";
        case TECC_CRYPTO_INVALID_PARAM:
            return "Invalid parameter";
        case TECC_CRYPTO_KEY_ERROR:
            return "Key error";
        case TECC_CRYPTO_MEMORY_ERROR:
            return "Memory allocation error";
        case TECC_CRYPTO_DATA_ERROR:
            return "Data error";
        default:
            return "Unknown error";
    }
}

/*
 * Get last error (backward compatibility version)
 */
const char *
tecc_key_adapter_get_last_error(void)
{
    /* Call thread-safe version with NULL adapter for backward compatibility */
    return tecc_key_get_last_error_with_adapter(NULL);
}

/*
 * Get DEK key through adapter
 */
int
tecc_key_manager_adapter_get_dek_key(TeccKeyManagerAdapter *adapter,
                                     const char *dek_name,
                                     unsigned char **dek_key,
                                     size_t *dek_key_len)
{
    if (!adapter || !dek_name || !dek_key || !dek_key_len)
    {
        tecc_key_set_last_error(adapter, TECC_CRYPTO_INVALID_PARAM);
        return TECC_CRYPTO_INVALID_PARAM;
    }

    /* Note: tecc_key_manager_adapter_get_dek_key is an adapter interface
     * for crypto scheduler. It should have access to tecc_state through
     * the adapter structure. We need to check if adapter->tecc_state is available.
     * adapter is guaranteed to be non-NULL here due to the check above.
     */
    if (adapter->tecc_state)
    {
        *dek_key = tecc_key_get_dek_with_adapter(adapter, dek_name, dek_key_len, adapter->tecc_state);
    }
    else
    {
        /* tecc_state is required for real key management operations */
        tecc_processor_debug(TECC_LOG_ERROR,
                           "tecc_state is NULL in adapter, cannot get DEK: %s", dek_name);
        tecc_key_set_last_error(adapter, TECC_CRYPTO_INVALID_PARAM);
        return TECC_CRYPTO_INVALID_PARAM;
    }
    if (!*dek_key)
    {
        /* Note: Global variable g_last_error removed for thread safety */
        /* adapter should not be NULL here because we checked it earlier */
        if (adapter)
        {
            return adapter->last_error;
        }
        else
        {
            /* This should not happen, but return system error as fallback */
            return TECC_CRYPTO_SYSTEM_ERROR;
        }
    }

    return TECC_CRYPTO_SUCCESS;
}

/* ReqID:TECC4.0 — extended version that also returns pre-derived enc/auth keys */
int
tecc_key_manager_adapter_get_dek_key_ext(TeccKeyManagerAdapter *adapter,
                                         const char *dek_name,
                                         unsigned char **dek_key,
                                         size_t *dek_key_len,
                                         unsigned char **derived_enc_key,
                                         unsigned char **derived_auth_key)
{
    int ret;
    int i;

    if (derived_enc_key) *derived_enc_key = NULL;
    if (derived_auth_key) *derived_auth_key = NULL;

    /* Get DEK key via existing path (cached or loaded) */
    ret = tecc_key_manager_adapter_get_dek_key(adapter, dek_name, dek_key, dek_key_len);
    if (ret != TECC_CRYPTO_SUCCESS)
        return ret;

    /* Look up derived keys from cache.
     *
     * ReqID:TECC4.0 — Bug 9254: 派生密钥必须与本次实际取到的 DEK 是同一份密钥材料。
     * 旧实现按 key_info.key_name 匹配，而 DEK 名并不唯一：
     *   - DROP KEK ... CASCADE 删除 DEK 后重建同名 DEK（本 bug 的复现路径），
     *   - 或不同 schema 下存在同名 DEK（Bug 9256 的跨 schema 场景）。
     * 此时缓存中会同时存在新旧两条同名记录，扫描必然命中先插入的旧记录，
     * 于是返回的 derived_enc/auth_key 来自已被删除的旧 DEK：写入用旧密钥加密，
     * 却按新 DEK 记账，后续任意新连接解密时 HMAC-SM3 校验不过，报
     * "Batch decrypt: item N failed"，数据不可读。
     * 改为按 DEK 的 OID（全局唯一，与 tecc_key_cache_get 的语义一致）匹配；
     * 解析不到时返回 NULL，由调用方从 DEK 明文现场派生（正确但略慢）。 */
    if (adapter && adapter->key_cache && dek_name && adapter->tecc_state)
    {
        TeccDekCacheItem *dek_item;

        dek_item = tecc_cache_get_dek_by_name_search_path((TeccState *)adapter->tecc_state,
                                                         dek_name);
        if (dek_item && dek_item->dek_oid != InvalidOid)
        {
            for (i = 0; i < adapter->max_cache_size; i++)
            {
                if (adapter->key_cache[i].key_data &&
                    adapter->key_cache[i].derived_keys_valid &&
                    adapter->key_cache[i].key_info.key_oid == dek_item->dek_oid)
                {
                    if (derived_enc_key)
                        *derived_enc_key = adapter->key_cache[i].derived_enc_key;
                    if (derived_auth_key)
                        *derived_auth_key = adapter->key_cache[i].derived_auth_key;
                    return TECC_CRYPTO_SUCCESS;
                }
            }
        }
    }

    /* Derived keys not in cache yet — fall back to caller deriving internally */
    if (derived_enc_key) *derived_enc_key = NULL;
    if (derived_auth_key) *derived_auth_key = NULL;
    return TECC_CRYPTO_SUCCESS;
}

/*
 * Release key (for crypto scheduler)
 */
void
tecc_key_manager_adapter_release_key(TeccKeyManagerAdapter *adapter,
                                    unsigned char *key,
                                    size_t key_len)
{
    if (!key || key_len == 0)
        return;

    /* Securely wipe key from memory */
    memset(key, 0, key_len);
    pg_free(key);
}

/* End - ReqID:TECC4.0 */

/* ==================== HE Key Stub Functions (ReqID:TECC4.0) ==================== */
int
tecc_key_manager_adapter_get_he_pubkey(TeccKeyManagerAdapter *adapter,
    Oid dek_oid, int algo_type,
    unsigned char **pubkey, size_t *pubkey_len)
{
    size_t key_size;
    if (adapter == NULL || pubkey == NULL || pubkey_len == NULL)
        return TECC_CRYPTO_INVALID_PARAM;
    if (algo_type == TECC_ALGO_EC_ELGAMAL)
        key_size = 65;
    else if (algo_type == TECC_ALGO_ELGAMAL)
        key_size = 256;
    else
        return TECC_CRYPTO_INVALID_PARAM;
    *pubkey = (unsigned char *)pg_malloc0(key_size);
    *pubkey_len = key_size;
    return TECC_CRYPTO_SUCCESS;
}

int
tecc_key_manager_adapter_get_he_privkey(TeccKeyManagerAdapter *adapter,
    Oid dek_oid, int algo_type,
    unsigned char **privkey, size_t *privkey_len)
{
    size_t key_size;
    if (adapter == NULL || privkey == NULL || privkey_len == NULL)
        return TECC_CRYPTO_INVALID_PARAM;
    if (algo_type == TECC_ALGO_EC_ELGAMAL)
        key_size = 32;
    else if (algo_type == TECC_ALGO_ELGAMAL)
        key_size = 256;
    else
        return TECC_CRYPTO_INVALID_PARAM;
    *privkey = (unsigned char *)pg_malloc0(key_size);
    *privkey_len = key_size;
    return TECC_CRYPTO_SUCCESS;
}

void
tecc_key_manager_adapter_release_he_key(TeccKeyManagerAdapter *adapter,
    unsigned char *key, size_t key_len)
{
    if (key != NULL && key_len > 0)
    {
        memset(key, 0, key_len);
        pg_free(key);
    }
}
