/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_crypto_scheduler.c
 *
 * Abstract:
 *   Main implementation of unified crypto scheduler for TECC module.
 *   Provides unified encryption/decryption interfaces with zero-copy support.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_scheduler/tecc_crypto_scheduler.c
 */

/* ReqID:TECC4.0 - HE 批量解密并发：pthread 与 sysconf（核数探测）。
 * unistd.h/sysconf 为 POSIX 专用，Windows MSVC 不可用（仅并行路径需要）。 */
#include <pthread.h>
#ifndef WIN32
#include <unistd.h>
#endif

#include "tecc_crypto_scheduler.h"
#include "tecc_algorithm_scheduler.h"
#include "tecc_key_manager_adapter.h"
#include "../tecc_processor/internal/tecc_processor_internal.h"
#include "../tecc_formatter/tecc_formatter.h"  /* Data formatter interface */
#include "../tecc_formatter/tecc_formatter_utils.h"  /* Formatter utilities */
#include "../tecc_formatter/tecc_data_validator.h"  /* Data validator interface */
#include "../tecc_cache/tecc_metadata_cache.h"  /* Metadata cache interface */
#include "../tecc_data_fetcher/include/tecc_data_fetcher.h"  /* For tecc_free_encrypted_column_metadata */
#include "../../../common/tecc_types.h"  /* Formatter error code definitions */
#include "common/fe_memutils.h"  /* Frontend memory management */
#include "mb/pg_wchar.h"  /* ReqID:TECC4.0 - 客户端编码（字符数校验用） */
#include "../tecc_libpq_integration/fe-tecc-integration.h"  /* TECC state management */
#include "../tecc_common/tecc_logging.h"  /* TECC logging system */
#include "../tecc_formatter/tecc_type_output.h"  /* Numeric text-to-binary conversion */
#include "../../../common/tecc_key/high_level_api.h"  /* tecc_key_get_last_error */
/* ReqID:TECC4.0 - forward declarations (no include to avoid bool conflict) */
extern int tecc_sm4_rnd_decrypt_with_keys(const unsigned char *key, size_t key_len,
    const unsigned char *ciphertext, size_t cipher_len,
    unsigned char *output_buffer, size_t buffer_size, size_t *output_len,
    const unsigned char *derived_enc_key, const unsigned char *derived_auth_key);
extern int tecc_sm4_rnd_encrypt_with_keys(const unsigned char *key, size_t key_len,
    const unsigned char *plaintext, size_t plain_len,
    unsigned char *output_buffer, size_t buffer_size, size_t *output_len,
    const unsigned char *derived_enc_key, const unsigned char *derived_auth_key);
extern int tecc_sm4_rnd_batch_decrypt_with_keys(
    const unsigned char *enc_key, const unsigned char *auth_key,
    const unsigned char **ciphertexts, const size_t *ciphertext_lens,
    size_t count, unsigned char **plaintexts, size_t *plaintext_lens);
#include "../tecc_hsm_adapter/fe-hsm-adapter.h"  /* HsmAdapterInterface, HsmDevice */
#include <string.h>
#include <stdlib.h>  /* For free() function */
#include <stdio.h>   /* For fprintf */
#include <sys/time.h> /* For gettimeofday */

/* Forward declaration from stmt_processor.h for error propagation */
extern void tecc_record_error(struct TeccState *state, int error_code,
                              const char *error_msg, const char *function_name,
                              int line_number, bool recoverable);

/*
 * tecc_split_qualified_name
 *    ReqID:TECC4.0 - Split a possibly schema-qualified name (schema.obj) into
 *    its schema part and bare object name.  Mirrors the backend get_dek_oid()
 *    semantics so frontend hg_dek/hg_kek lookups can filter by key_namespace.
 *
 *    Returns 1 and fills *schema_out / *name_out when the name contains a dot
 *    (with both parts non-empty); returns 0 if no dot (name stays bare).
 */
static int
tecc_split_qualified_name(const char *name,
                          char *schema_out, size_t schema_sz,
                          char *name_out, size_t name_sz)
{
    const char *dot;

    if (name == NULL)
        return 0;
    dot = strchr(name, '.');
    if (dot == NULL || dot == name || dot[1] == '\0')
        return 0;

    {
        size_t schema_len = (size_t) (dot - name);

        if (schema_len >= schema_sz)
            schema_len = schema_sz - 1;
        memcpy(schema_out, name, schema_len);
        schema_out[schema_len] = '\0';
        strlcpy(name_out, dot + 1, name_sz);
        return 1;
    }
}

/* ReqID:TECC4.0 - Check DEK USAGE against the exact schema-qualified key. */
static bool
tecc_dek_has_usage_permission(TeccState *state, const char *dek_name)
{
	TeccDekCacheItem *dek_item;
	const char *check_name;
	const char *check_schema;
	char		schema_name[NAMEDATALEN];
	char		bare_name[NAMEDATALEN];

	if (state == NULL || dek_name == NULL || dek_name[0] == '\0')
		return false;

	check_name = dek_name;
	check_schema = NULL;
	dek_item = tecc_cache_get_dek_by_name_search_path(state, dek_name);
	if (dek_item != NULL && dek_item->key_name != NULL)
	{
		check_name = dek_item->key_name;
		check_schema = dek_item->key_namespace;
	}
	else if (tecc_split_qualified_name(dek_name,
									   schema_name, sizeof(schema_name),
									   bare_name, sizeof(bare_name)))
	{
		check_name = bare_name;
		check_schema = schema_name;
	}

	return tecc_check_dek_permission(state, check_name, check_schema) ==
		TECC_FETCHER_OK;
}

/*
 * ReqID:TECC4.0 - Load HE key material (he_pubkey / he_privkey) for a DEK
 * from hg_dek_args. Returns 0 on success (output hex allocated), -1 on error.
 *
 * @param state   TECC state (has conn for query)
 * @param dek_name DEK name (may be schema-qualified, e.g. public.dek_x)
 * @param key     "he_pubkey" or "he_privkey"
 * @param hex_out Output: hex string (pg_malloc'd), or NULL if not found
 */

/* ==================== HE 元数据查询缓存 ====================
 * ReqID:TECC4.0 - tecc_he_load_dek_key_hex（DEK 公钥）和
 * tecc_load_column_he_dek_name（列 HE DEK 名）每次直接用主连接 PQexec。
 * 字段级并行 worker 并发用主连接（libpq 非线程安全）会卡死，且每次查询
 * 是串行路径的主要固定开销。加固定小缓存（线性覆盖）：字段级并行前主线程
 * 预热填充，worker 内命中缓存不碰主连接；无值（-1）也缓存，避免 worker
 * 重复查询。HE DEK/列数量通常极少，16 项足够。 */
#define TECC_HE_META_CACHE_SIZE 16

typedef struct TeccHeMetaCacheEntry
{
    bool valid;
    Oid table_oid;              /* 列 HE DEK 名缓存键 */
    int16 column_attnum;
    char key[32];               /* he_add_dek / he_mul_dek / he_ec_pubkey / he_eg_pubkey */
    char dek_name[NAMEDATALEN]; /* DEK 公钥缓存键 */
    char value[1024];           /* 公钥 hex（未压缩 EC 点可达 130 hex）或 DEK 名 */
} TeccHeMetaCacheEntry;

static TeccHeMetaCacheEntry tecc_he_meta_cache[TECC_HE_META_CACHE_SIZE];
static int tecc_he_meta_cache_next = 0;
static pthread_mutex_t tecc_he_meta_cache_lock = PTHREAD_MUTEX_INITIALIZER;

/* ReqID:TECC4.0 - 写入缓存（线性覆盖；线程安全） */
static void
tecc_he_meta_cache_put(const TeccHeMetaCacheEntry *entry)
{
    int idx;

    pthread_mutex_lock(&tecc_he_meta_cache_lock);
    idx = tecc_he_meta_cache_next;
    tecc_he_meta_cache[idx] = *entry;
    tecc_he_meta_cache_next = (idx + 1) % TECC_HE_META_CACHE_SIZE;
    pthread_mutex_unlock(&tecc_he_meta_cache_lock);
}

/*
 * ReqID:TECC4.0 - Bug 9263: 清空 HE 元数据缓存（线程安全）。
 *
 * 该缓存缓存的是 hg_dek_args 里 he_ec_privkey/he_eg_privkey 等字段的**原始 hex**；
 * 对 HE 私钥而言，那就是【KEK 加密后的信封】。ALTER KEK 轮换会用新 KEK 公钥重新
 * 封装这些信封，但本缓存既不在 tecc_cache_clear()（KEK/DEK/列元数据）的清理范围，
 * 也不在 tecc_key_cache_clear()（key_manager 的 DEK 明文）的清理范围，于是执行过
 * 轮换的客户端会继续拿【旧信封】配【新 KEK】去解封：SM2 信封不带完整性校验，
 * 解不出错而是静默产出一把错误的私钥，HE 解密结果变成垃圾值，表现为
 * "HE result out of range for column ..."。另起一个进程的客户端没有这份缓存，
 * 一切正常（注意：缓存是进程级的，同进程内 \connect 重连也清不掉）。
 *
 * 缓存是进程内全局的（字段级并行 worker 要复用，故不能用连接级缓存替代），
 * 这里整体清空：轮换是 DDL，代价可忽略；且其他连接缓存的同样是已经过期的旧
 * 信封，一并失效反而更正确。
 */
void
tecc_he_meta_cache_clear(void)
{
    pthread_mutex_lock(&tecc_he_meta_cache_lock);
    memset(tecc_he_meta_cache, 0, sizeof(tecc_he_meta_cache));
    tecc_he_meta_cache_next = 0;
    pthread_mutex_unlock(&tecc_he_meta_cache_lock);
}

/* ReqID:TECC4.0 - 按 DEK 公钥键查缓存（线程安全） */
static bool
tecc_he_meta_cache_get_dek_hex(const char *dek_name, const char *key,
                               char *out, size_t out_size)
{
    int i;
    bool found = false;

    if (dek_name == NULL || key == NULL || out == NULL)
        return false;

    pthread_mutex_lock(&tecc_he_meta_cache_lock);
    for (i = 0; i < TECC_HE_META_CACHE_SIZE && !found; i++)
    {
        if (tecc_he_meta_cache[i].valid &&
            tecc_he_meta_cache[i].dek_name[0] != '\0' &&
            strcmp(tecc_he_meta_cache[i].dek_name, dek_name) == 0 &&
            strcmp(tecc_he_meta_cache[i].key, key) == 0)
        {
            strlcpy(out, tecc_he_meta_cache[i].value, out_size);
            found = true;
        }
    }
    pthread_mutex_unlock(&tecc_he_meta_cache_lock);
    return found;
}

/* ReqID:TECC4.0 - 按列 HE DEK 名键查缓存（线程安全） */
static bool
tecc_he_meta_cache_get_col_dek(Oid table_oid, int16 column_attnum,
                               const char *key, char *out, size_t out_size)
{
    int i;
    bool found = false;

    if (key == NULL || out == NULL)
        return false;

    pthread_mutex_lock(&tecc_he_meta_cache_lock);
    for (i = 0; i < TECC_HE_META_CACHE_SIZE && !found; i++)
    {
        if (tecc_he_meta_cache[i].valid &&
            tecc_he_meta_cache[i].table_oid == table_oid &&
            tecc_he_meta_cache[i].column_attnum == column_attnum &&
            strcmp(tecc_he_meta_cache[i].key, key) == 0)
        {
            strlcpy(out, tecc_he_meta_cache[i].value, out_size);
            found = true;
        }
    }
    pthread_mutex_unlock(&tecc_he_meta_cache_lock);
    return found;
}

int
tecc_he_load_dek_key_hex(void *state_arg, const char *dek_name,
                         const char *key, char **hex_out)
{
    TeccState  *state = (TeccState *)state_arg;
    PGconn     *conn;
    PGresult   *res;
    char        esc[NAMEDATALEN * 2];
    char        query[1024];
    const char *val;
    bool        saved_disable;
    char        cached[1024];
    TeccHeMetaCacheEntry entry;

    *hex_out = NULL;
    if (state == NULL || state->conn == NULL || dek_name == NULL || key == NULL)
        return -1;

    /* ReqID:TECC4.0 - 先查缓存（字段级并行 worker 内命中，不碰主连接） */
    if (tecc_he_meta_cache_get_dek_hex(dek_name, key, cached, sizeof(cached)))
    {
        if (cached[0] == '\0')
            return -1;      /* 已缓存：该 DEK 无此 key */
        *hex_out = pg_strdup(cached);
        return 0;
    }

    conn = state->conn;

    /* ReqID:TECC4.0 - Escape DEK name; handle schema-qualified names
     * (public.dek_x) by splitting schema + name and filtering by
     * key_namespace (backend get_dek_oid semantics).  Otherwise the exact
     * dek_name match below fails for qualified names (hg_dek.dek_name has
     * no schema prefix). */
    {
        char schema_buf[NAMEDATALEN];
        char name_buf[NAMEDATALEN];

        if (tecc_split_qualified_name(dek_name, schema_buf, sizeof(schema_buf),
                                      name_buf, sizeof(name_buf)))
        {
            char esc_schema[NAMEDATALEN * 2];
            char esc_name[NAMEDATALEN * 2];

            PQescapeStringConn(conn, esc_schema, schema_buf, strlen(schema_buf), NULL);
            PQescapeStringConn(conn, esc_name, name_buf, strlen(name_buf), NULL);

            snprintf(query, sizeof(query),
                "SELECT encode(value, 'escape') FROM hg_dek_args "
                "WHERE dek_id = (SELECT d.oid FROM hg_dek d "
                "  JOIN pg_namespace n ON d.key_namespace = n.oid "
                "  WHERE d.dek_name = '%s' AND n.nspname = '%s') "
                "AND key = '%s'",
                esc_name, esc_schema, key);
        }
        else
        {
            /* Bare name: look up by dek_name alone (keeps existing behavior) */
            PQescapeStringConn(conn, esc, dek_name, strlen(dek_name), NULL);

            snprintf(query, sizeof(query),
                "SELECT encode(value, 'escape') FROM hg_dek_args "
                "WHERE dek_id = (SELECT oid FROM hg_dek WHERE dek_name = '%s') "
                "AND key = '%s'",
                esc, key);
        }
    }

    /* Temporarily disable TECC to avoid recursive processing */
    saved_disable = state->disable_once;
    state->disable_once = true;
    res = PQexec(conn, query);
    state->disable_once = saved_disable;

    if (res == NULL || PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
    {
        if (res)
            PQclear(res);
        return -1;
    }

    val = PQgetvalue(res, 0, 0);

    /* ReqID:TECC4.0 - 写缓存（无论有无值，避免 worker 重复查询主连接） */
    memset(&entry, 0, sizeof(entry));
    entry.valid = true;
    strlcpy(entry.dek_name, dek_name, sizeof(entry.dek_name));
    strlcpy(entry.key, key, sizeof(entry.key));
    if (val != NULL && val[0] != '\0')
        strlcpy(entry.value, val, sizeof(entry.value));
    tecc_he_meta_cache_put(&entry);

    if (val != NULL && val[0] != '\0')
        *hex_out = pg_strdup(val);
    PQclear(res);

    return (*hex_out != NULL) ? 0 : -1;
}

/* ReqID:TECC4.0 - P0-2 一列多 DEK：查询列的 he-add/he-mul DEK 名称。
 * 通过 table_oid + column_attnum 精确匹配 hg_encrypted_columns_info 视图，
 * 返回 he_add_dek/he_mul_dek 字段（DEK 名，无则失败）。
 * ReqID:TECC4.0 - 加元数据缓存（同 tecc_he_load_dek_key_hex），字段级
 * 并行前主线程预热，worker 内命中缓存不碰主连接。 */
int
tecc_load_column_he_dek_name(void *state_arg, Oid table_oid, int16 column_attnum,
                             const char *key, char *name_out, size_t name_out_size)
{
    TeccState  *state = (TeccState *)state_arg;
    PGconn     *conn;
    PGresult   *res;
    char        query[512];
    const char *col_field;
    const char *val;
    bool        saved_disable;
    char        cached[NAMEDATALEN];
    TeccHeMetaCacheEntry entry;

    if (name_out == NULL || name_out_size == 0)
        return -1;
    name_out[0] = '\0';
    if (state == NULL || state->conn == NULL || key == NULL)
        return -1;

    if (strcmp(key, "he_add_dek") == 0)
        col_field = "he_add_dek";
    else if (strcmp(key, "he_mul_dek") == 0)
        col_field = "he_mul_dek";
    else
        return -1;

    /* ReqID:TECC4.0 - 先查缓存（字段级并行 worker 内命中，不碰主连接） */
    if (tecc_he_meta_cache_get_col_dek(table_oid, column_attnum, key,
                                       cached, sizeof(cached)))
    {
        if (cached[0] == '\0')
            return -1;      /* 已缓存：该列无此 DEK */
        strlcpy(name_out, cached, name_out_size);
        return 0;
    }

    conn = state->conn;
    snprintf(query, sizeof(query),
             "SELECT %s FROM hg_encrypted_columns_info "
             "WHERE table_oid = %u AND column_attnum = %d",
             col_field, table_oid, (int) column_attnum);

    /* Temporarily disable TECC to avoid recursive processing */
    saved_disable = state->disable_once;
    state->disable_once = true;
    res = PQexec(conn, query);
    state->disable_once = saved_disable;

    if (res == NULL || PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
    {
        if (res)
            PQclear(res);
        return -1;
    }

    val = PQgetvalue(res, 0, 0);

    /* ReqID:TECC4.0 - 写缓存（无论有无值，避免 worker 重复查询主连接） */
    memset(&entry, 0, sizeof(entry));
    entry.valid = true;
    entry.table_oid = table_oid;
    entry.column_attnum = column_attnum;
    strlcpy(entry.key, key, sizeof(entry.key));
    if (val != NULL && val[0] != '\0')
        strlcpy(entry.value, val, sizeof(entry.value));
    tecc_he_meta_cache_put(&entry);

    if (val != NULL && val[0] != '\0')
        strlcpy(name_out, val, name_out_size);
    PQclear(res);

    return (name_out[0] != '\0') ? 0 : -1;
}

/*
 * ReqID:TECC4.0 - Load a KEK arg (key_algo / public_key) from hg_kek_args.
 * Returns 0 on success with *hex_out allocated, -1 on error.
 */
int
tecc_he_load_kek_arg_hex(void *state_arg, const char *kek_name,
                         const char *key, char **hex_out)
{
    TeccState  *state = (TeccState *)state_arg;
    PGconn     *conn;
    PGresult   *res;
    char        query[1024];
    const char *val;
    bool        saved_disable;

    *hex_out = NULL;
    if (state == NULL || state->conn == NULL || kek_name == NULL || key == NULL)
        return -1;

    conn = state->conn;

    /* ReqID:TECC4.0 - Escape KEK name; support schema-qualified names
     * (schema.kek) by splitting schema + name and filtering by
     * key_namespace, consistent with tecc_he_load_dek_key_hex. */
    {
        char schema_buf[NAMEDATALEN];
        char name_buf[NAMEDATALEN];
        char esc_schema[NAMEDATALEN * 2];
        char esc_name[NAMEDATALEN * 2];
        char ns_join[80];
        char ns_where[NAMEDATALEN + 64];
        bool qualified;

        qualified = tecc_split_qualified_name(kek_name,
                                              schema_buf, sizeof(schema_buf),
                                              name_buf, sizeof(name_buf));
        if (qualified)
        {
            PQescapeStringConn(conn, esc_schema, schema_buf, strlen(schema_buf), NULL);
            PQescapeStringConn(conn, esc_name, name_buf, strlen(name_buf), NULL);
            snprintf(ns_join, sizeof(ns_join),
                     " JOIN pg_namespace n ON k.key_namespace = n.oid");
            snprintf(ns_where, sizeof(ns_where),
                     " AND n.nspname = '%s'", esc_schema);
        }
        else
        {
            PQescapeStringConn(conn, esc_name, kek_name, strlen(kek_name), NULL);
            ns_join[0] = '\0';
            ns_where[0] = '\0';
        }

        snprintf(query, sizeof(query),
            "SELECT encode(value, 'hex') FROM hg_kek_args "
            "WHERE kek_id = (SELECT k.oid FROM hg_kek k%s "
            "  WHERE k.kek_name = '%s'%s) "
            "AND upper(key) = upper('%s')",
            ns_join, esc_name, ns_where, key);
    }

    /* Temporarily disable TECC to avoid recursive processing */
    saved_disable = state->disable_once;
    state->disable_once = true;
    res = PQexec(conn, query);
    state->disable_once = saved_disable;

    if (res == NULL || PQresultStatus(res) != PGRES_TUPLES_OK || PQntuples(res) == 0)
    {
        if (res)
            PQclear(res);
        return -1;
    }

    val = PQgetvalue(res, 0, 0);
    if (val != NULL && val[0] != '\0')
        *hex_out = pg_strdup(val);
    PQclear(res);

    return (*hex_out != NULL) ? 0 : -1;
}

/* Utility macros */
#ifndef MAX
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#endif

/* Frontend error handling macros (replace ereport) */
/* Use tecc_processor_debug instead of direct stderr access for shared library compatibility */
#define TECC_SCHEDULER_ERROR(format, ...) \
    do { \
        tecc_processor_debug(TECC_LOG_ERROR, "TECC SCHEDULER ERROR: " format, ##__VA_ARGS__); \
    } while (0)

#define TECC_SCHEDULER_WARNING(format, ...) \
    do { \
        tecc_processor_debug(TECC_LOG_WARNING, "TECC SCHEDULER WARNING: " format, ##__VA_ARGS__); \
    } while (0)

#define TECC_SCHEDULER_WARN TECC_SCHEDULER_WARNING

#define TECC_SCHEDULER_DEBUG(format, ...) \
    do { \
        TECC_LOG(TECC_LOG_DEBUG1, "SCHEDULER: " format, ##__VA_ARGS__); \
    } while (0)

/* Performance measurement helper macros */
#define TECC_START_TIMER(start_tv) gettimeofday(&(start_tv), NULL)
#define TECC_END_TIMER(start_tv, end_tv, elapsed_ms) \
    do { \
        gettimeofday(&(end_tv), NULL); \
        (elapsed_ms) = ((end_tv).tv_sec - (start_tv).tv_sec) * 1000.0 + \
                      ((end_tv).tv_usec - (start_tv).tv_usec) / 1000.0; \
    } while (0)

/* Forward declarations for incomplete types */
typedef struct TeccMetadataCache TeccMetadataCache;
typedef struct TeccKeyManager TeccKeyManager;

/*
 * Map encrypted type OID to original PostgreSQL type OID.
 * E.g., NUMERIC_ENC_OID(50005) -> NUMERICOID(1700).
 * If the OID is not an encrypted type, returns it unchanged.
 * ReqID:TECC4.0
 */
static inline Oid
tecc_map_to_original_type_oid(Oid enc_oid)
{
    switch (enc_oid)
    {
        case INT2_ENC_OID:    return INT2OID;
        case INT4_ENC_OID:    return INT4OID;
        case INT8_ENC_OID:    return INT8OID;
        case FLOAT4_ENC_OID:  return FLOAT4OID;
        case FLOAT8_ENC_OID:  return FLOAT8OID;
        case NUMERIC_ENC_OID: return NUMERICOID;
        case VARCHAR_ENC_OID: return VARCHAROID;
        case BPCHAR_ENC_OID:  return BPCHAROID;
        case TEXT_ENC_OID:    return TEXTOID;
        default:              return enc_oid;
    }
}

/* Global scheduler instance - REMOVED for thread safety */
/* Connection-specific scheduler should be used instead, stored in TeccState structure */

/* ReqID:TECC4.0 - Begin：连接级 HSM 初始化锁（说明见 fe-tecc-integration.h 的
 * TeccState.hsm_init_lock）。仅在 tecc_setup_hsm_context 的首次初始化段使用，
 * 软加密（localkms）列在 KEK 路由阶段即早退，不经过该段。 */

/*
 * tecc_hsm_init_lock_create - 创建连接级 HSM 初始化锁
 *
 * 在连接级状态创建时调用（此时还没有任何 worker 线程，故创建本身无竞争）。
 * 返回 0 成功；内存或互斥量不可用时返回 -1，由调用方降级为不加锁。
 */
int
tecc_hsm_init_lock_create(void *tecc_state)
{
    TeccState *state = (TeccState *)tecc_state;
    pthread_mutex_t *lock;

    if (state == NULL || state->hsm_init_lock != NULL)
        return 0;

    lock = (pthread_mutex_t *)pg_malloc(sizeof(pthread_mutex_t));
    if (lock == NULL)
        return -1;

    if (pthread_mutex_init(lock, NULL) != 0)
    {
        pg_free(lock);
        return -1;
    }

    state->hsm_init_lock = (void *)lock;
    return 0;
}

/* tecc_hsm_init_lock_destroy - 释放连接级 HSM 初始化锁（幂等） */
void
tecc_hsm_init_lock_destroy(void *tecc_state)
{
    TeccState *state = (TeccState *)tecc_state;

    if (state == NULL || state->hsm_init_lock == NULL)
        return;

    pthread_mutex_destroy((pthread_mutex_t *)state->hsm_init_lock);
    pg_free(state->hsm_init_lock);
    state->hsm_init_lock = NULL;
}

/*
 * tecc_hsm_init_device - 初始化 HSM 适配器并写入连接级缓存
 *
 * 调用方须持有 TeccState.hsm_init_lock（冷缓存的首次初始化必须串行，见
 * tecc_setup_hsm_context 内说明）。
 * 返回 0 成功（*cached_tried = 1，adapter/device 就绪）；返回 -1 失败
 * （*cached_tried = -1 且 adapter/device 已清空，失败原因已记录日志）。
 */
static int
tecc_hsm_init_device(HsmAdapterInterface **cached_adapter,
                     HsmDevice **cached_device,
                     int *cached_tried,
                     bool is_swxa,
                     const char *kek_name)
{
    /* Tianzheng adapter uses init(NULL)/connect; SWXA uses init(id)/connect.
     * For column data encryption we pass NULL config - adapter defaults work. */
    if (is_swxa)
        *cached_adapter = get_swxa_adapter();
    else
        *cached_adapter = get_tzhsm_adapter();
    if (!*cached_adapter)
    {
        *cached_tried = -1;
        TECC_LOG(TECC_LOG_ERROR, "TECC_ROUTE: Failed to get %s adapter for KEK '%s', refusing software fallback",
                 is_swxa ? "SWXA" : "HSM", kek_name);
        return -1;
    }

    if ((*cached_adapter)->init(cached_device, NULL) != HSM_SUCCESS)
    {
        *cached_tried = -1;
        *cached_adapter = NULL;
        TECC_LOG(TECC_LOG_ERROR, "TECC_ROUTE: %s init failed for KEK '%s', refusing software fallback",
                 is_swxa ? "SWXA" : "HSM", kek_name);
        return -1;
    }

    if ((*cached_adapter)->connect(*cached_device) != HSM_SUCCESS)
    {
        (*cached_adapter)->cleanup(*cached_device);
        *cached_device = NULL;
        *cached_tried = -1;
        *cached_adapter = NULL;
        TECC_LOG(TECC_LOG_ERROR, "TECC_ROUTE: %s connect failed for KEK '%s', refusing software fallback",
                 is_swxa ? "SWXA" : "HSM", kek_name);
        return -1;
    }

    /* ReqID:TECC4.0 - 发布顺序：adapter/device 先写好（上面的 init/connect 已写），
     * tried 最后写。无锁快路径以 tried 判断缓存是否可用，故 tried 必须最后发布。 */
    *cached_tried = 1;
    return 0;
}

/* ReqID:TECC4.0 - End：连接级 HSM 初始化锁 */

/*
 * Set up HSM SM4 encryption on context when the KEK's KMS type is HSM.
 * Checks the KEK cache item's kms_type field (2 = hsmkms, 4 = swxakms)
 * and selects the appropriate HSM adapter (Tianzheng or SWXA).
 *
 * HSM initialization is attempted once per adapter type and cached statically.
 * The decision to USE HSM is per-call based on the current column's KEK type,
 * so soft (localkms) and hard (hsmkms/swxakms) columns can coexist.
 * If no HSM info or not an HSM KEK, leaves context fields NULL (software SM4).
 */
static int
tecc_setup_hsm_context(TeccCryptoContextExtended *context,
                       TeccEncryptedColumnMetadata *col_meta,
                       void *tecc_state)
{
    /* ReqID:TECC4.0 - 从连接级 TeccState 获取 HSM 缓存（替代进程级 static 变量） */
    TeccState *state = (TeccState *)tecc_state;

    HsmAdapterInterface **cached_adapter;
    HsmDevice **cached_device;
    int *cached_tried;
    bool is_swxa;

    if (!context)
        return 0;

    /* Need KEK info and tecc_state to look up KEK cache */
    if (!col_meta || !state)
    {
        TECC_LOG(TECC_LOG_DEBUG1, "TECC_ROUTE: no col_meta/tecc_state, using SOFTWARE encryption");
        context->hsm_kms_type = 0;
        return 0;
    }

    /* Resolve KEK name: from col_meta->kek_name directly, or via DEK cache if NULL */
    if (!col_meta->kek_name || col_meta->kek_name[0] == '\0')
    {
        /* Try to resolve KEK name from DEK cache using col_meta->dek_name */
        if (col_meta->dek_name && col_meta->dek_name[0] != '\0')
        {
            TeccDekCacheItem *dek_item = tecc_cache_get_dek_by_name_search_path(
                (const TeccState *)tecc_state, col_meta->dek_name);
            if (dek_item && dek_item->kek_oid != InvalidOid)
            {
                TeccKekCacheItem *kek_item = tecc_cache_get_kek_by_oid(
                    (const TeccState *)tecc_state, dek_item->kek_oid);
                if (kek_item && kek_item->key_name)
                {
                    col_meta->kek_name = pg_strdup(kek_item->key_name);
                }
            }
        }
        /* Still no kek_name, cannot determine HSM type */
        if (!col_meta->kek_name || col_meta->kek_name[0] == '\0')
        {
            TECC_LOG(TECC_LOG_DEBUG1, "TECC_ROUTE: no kek_name, using SOFTWARE encryption");
            context->hsm_kms_type = 0;
            return 0;
        }
    }

    /* Look up KEK cache to check KMS type for THIS column */
    {
        TeccKekCacheItem *kek_item = tecc_cache_get_kek_by_name_search_path(
            (const TeccState *)tecc_state, col_meta->kek_name);

        /* KEK kms_type: 1=localkms, 2=hsmkms, 3=cloudkms, 4=swxakms */
        if (!kek_item)
        {
            TECC_LOG(TECC_LOG_DEBUG1, "TECC_ROUTE: KEK '%s' not found in cache, using SOFTWARE encryption", col_meta->kek_name);
            context->hsm_kms_type = 0;
            return 0;
        }
        if (kek_item->kms_type == 2)
        {
            is_swxa = false;
        }
        else if (kek_item->kms_type == 4)
        {
            is_swxa = true;
        }
        else
        {
            TECC_LOG(TECC_LOG_DEBUG1, "TECC_ROUTE: KEK '%s' kms_type=%d, using SOFTWARE encryption", col_meta->kek_name, kek_item->kms_type);
            context->hsm_kms_type = 0;
            return 0;  /* Not an HSM KEK, use software encryption */
        }
    }

    /* ReqID:TECC4.0 - 从连接级 TeccState 获取 HSM 缓存（替代进程级 static 变量） */
    if (is_swxa)
    {
        cached_adapter = (HsmAdapterInterface **)&state->hsm_swxa_adapter;
        cached_device = (HsmDevice **)&state->hsm_swxa_device;
        cached_tried = &state->hsm_swxa_tried;
    }
    else
    {
        cached_adapter = (HsmAdapterInterface **)&state->hsm_tz_adapter;
        cached_device = (HsmDevice **)&state->hsm_tz_device;
        cached_tried = &state->hsm_tz_tried;
    }

    /* If we already tried and failed, don't retry */
    if (*cached_tried < 0)
    {
        TECC_LOG(TECC_LOG_ERROR, "TECC_ROUTE: %s init previously failed for KEK '%s', refusing software fallback",
                 is_swxa ? "SWXA" : "HSM", col_meta->kek_name);
        context->hsm_kms_type = 0;
        return -1;
    }

    /* If already initialized, reuse */
    if (*cached_tried > 0 && *cached_adapter != NULL && *cached_device != NULL)
    {
        context->hsm_adapter = (void *)*cached_adapter;
        context->hsm_device = (void *)*cached_device;
        context->hsm_kms_type = is_swxa ? 4 : 2;
        TECC_LOG(TECC_LOG_DEBUG1, "TECC_ROUTE: KEK '%s' kms_type=%s, reusing %s adapter, using HARDWARE encryption",
                 col_meta->kek_name, is_swxa ? "SWXA" : "HSM",
                 is_swxa ? "SWXA" : "HSM");
        return 0;
    }

    /* ReqID:TECC4.0 - 首次初始化必须串行执行：同态字段级并行的 worker 线程会并发
     * 到达这里，无锁时各自 init + SDF_OpenSession，而上面的缓存槽只有一份、只留
     * 最后一个写入者，其余 HsmDevice（含会话句柄与各自的会话密钥句柄缓存）无人
     * disconnect/cleanup，一直挂到进程退出；交错还可能把 tried 写成 -1，使同一
     * 连接后续语句非确定性地报"拒绝软件回退"。
     * 因此：上面的 -1/复用分支是快路径、不加锁（本函数每值都会被调用，锁不能进
     * 热路径），只有冷缓存这一次进锁，等锁期间别人做完了就走下面的双检。
     * 锁未创建（state->hsm_init_lock == NULL）时降级为不加锁，行为与改动前一致。 */
    if (state->hsm_init_lock != NULL)
        pthread_mutex_lock((pthread_mutex_t *)state->hsm_init_lock);

    /* 双检：等锁期间可能已有其它线程完成初始化（成功或失败） */
    if (*cached_tried == 0 &&
        tecc_hsm_init_device(cached_adapter, cached_device, cached_tried,
                             is_swxa, col_meta->kek_name) != 0)
    {
        if (state->hsm_init_lock != NULL)
            pthread_mutex_unlock((pthread_mutex_t *)state->hsm_init_lock);
        context->hsm_kms_type = 0;
        return -1;
    }

    if (state->hsm_init_lock != NULL)
        pthread_mutex_unlock((pthread_mutex_t *)state->hsm_init_lock);

    /* 初始化未就绪（本线程失败，或等锁期间别的线程失败）：与上面的拒绝语义一致 */
    if (*cached_tried < 0 || *cached_adapter == NULL || *cached_device == NULL)
    {
        TECC_LOG(TECC_LOG_ERROR, "TECC_ROUTE: %s init failed for KEK '%s', refusing software fallback",
                 is_swxa ? "SWXA" : "HSM", col_meta->kek_name);
        context->hsm_kms_type = 0;
        return -1;
    }

    context->hsm_adapter = (void *)*cached_adapter;
    context->hsm_device = (void *)*cached_device;
    context->hsm_kms_type = is_swxa ? 4 : 2;
    TECC_LOG(TECC_LOG_DEBUG1, "TECC_ROUTE: %s initialized for KEK '%s', using HARDWARE encryption",
             is_swxa ? "SWXA" : "HSM", col_meta->kek_name);
    return 0;
}

/*
 * ReqID:TECC4.0 - Bug 9384: 结果列背后没有真实加密列时（纯加密常量表达式，如
 * coalesce(10::int2_enc encrypted with (dek = d), NULL) 或裸加密常量），预处理
 * 阶段不会记录 pending_expr_col_*，上游按表名 "unknown" 查不到列元数据，
 * col_meta 为 NULL；但密文头 dek_dist_oid 仍能解析出 DEK 名。
 *
 * HSM 路由必须按"密文实际使用的 DEK"判定：否则 tecc_setup_hsm_context 整段被
 * 跳过，hsm_kms_type 保持 0，硬加密的裸 [IV(16)][SM4-CBC] 密文会被交给软件
 * 批量解密（期望 [version][auth_tag][IV][ct_len][ct] 包装），格式校验失败，
 * 报 "Batch decrypt: item N failed"。
 *
 * 这里合成一份只含 dek_name 的最小元数据：tecc_setup_hsm_context 内部会用
 * dek_name 反查 DEK/KEK 缓存补齐 kek_name，因此无需真实列元数据。
 * 调用方用 tecc_free_encrypted_column_metadata 释放（dek_name 与补齐的 kek_name
 * 都会一并释放）。DEK 名为空时返回 NULL，调用方保持原有的软件路由行为。
 */
static TeccEncryptedColumnMetadata *
tecc_hsm_meta_synth_from_dek(const char *dek_name)
{
    TeccEncryptedColumnMetadata *meta;

    if (dek_name == NULL || dek_name[0] == '\0')
        return NULL;

    meta = pg_malloc0(sizeof(TeccEncryptedColumnMetadata));
    meta->dek_name = pg_strdup(dek_name);

    return meta;
}

/*
 * Get crypto scheduler from connection state
 * Returns NULL if connection or scheduler is not available
 */
static TeccCryptoScheduler *
tecc_get_scheduler_from_conn(PGconn *conn)
{
    TeccState *state;

    if (conn == NULL)
    {
        TECC_SCHEDULER_DEBUG("NULL connection passed to tecc_get_scheduler_from_conn");
        return NULL;
    }

    state = tecc_get_state(conn);
    if (state == NULL)
    {
        TECC_SCHEDULER_DEBUG("Failed to get TECC state from connection");
        return NULL;
    }

    if (!state->scheduler_initialized || state->crypto_scheduler == NULL)
    {
        TECC_SCHEDULER_DEBUG("Crypto scheduler not initialized for connection");
        return NULL;
    }

    return state->crypto_scheduler;
}


/* Begin - ReqID:TECC4.0 - Decryption validation functions */
static bool tecc_validate_hex_string(const char *hex_string, size_t *valid_length);
static bool tecc_validate_cipherdata_integrity(const CipherDataType *cipherdata, size_t cipher_len);
static bool tecc_detect_tampering(const CipherDataType *cipherdata, size_t cipher_len);
/* End - ReqID:TECC4.0 - Decryption validation functions */

/* Begin - ReqID:TECC4.0 - Batch optimization functions */
static TeccUnifiedEncryptResult *tecc_unified_encrypt_string_with_preloaded_key(
    TeccCryptoScheduler *scheduler,
    const char *plaintext,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name,
    const unsigned char *preloaded_dek_key,
    size_t preloaded_dek_key_len,
    const unsigned char *preloaded_derived_enc_key,
    const unsigned char *preloaded_derived_auth_key);
/* End - ReqID:TECC4.0 - Batch optimization functions */

static TeccAlgorithmSupport tecc_unified_get_algorithm_support_internal_with_scheduler(
    PGconn *conn,
    TeccCryptoScheduler *scheduler,
    const char *table_name,
    const char *column_name);

/* ReqID:TECC4.0 - HE 批量解密并发：并行阈值（行数低于此走串行，避免线程开销） */
#define TECC_HE_PARALLEL_MIN_ROWS 256

/* ReqID:TECC4.0 - HE 批量解密并发优化：worker 线程参数。
 * Windows 下 libpq 的 pthread-win32 仅实现 mutex/thread-local，无
 * pthread_create，故 worker 结构与函数仅非 Windows 平台编译。 */
#ifndef WIN32
typedef struct TeccHeWorkerArg
{
    int             start;
    int             end;
    char          **hex_strings;
    const BIGNUM  *ec_priv;     /* 共享只读 EC 私钥（NULL = 硬编码 fallback） */
    const BIGNUM  *ff_priv;     /* 共享只读 FF 私钥 */
    TeccUnifiedDecryptResult **results;
} TeccHeWorkerArg;

/* ReqID:TECC4.0 - HE 批量解密 worker 线程：独立 BN_CTX，处理 [start, end) 行。
 * 私钥为共享只读 BIGNUM*（OpenSSL BN 只读访问线程安全），结果按行号写入
 * 各自区间（无竞争），pg_malloc/pg_free 直接映射 malloc/free（线程安全）。 */
static void *
tecc_he_decrypt_worker(void *arg)
{
    TeccHeWorkerArg *wa = (TeccHeWorkerArg *)arg;
    BN_CTX *ctx = BN_CTX_new();
    int     i;

    for (i = wa->start; i < wa->end; i++)
    {
        unsigned char *bin = NULL;
        size_t         bin_len = 0;
        TeccParsedCipherdata pd;

        if (wa->hex_strings[i] == NULL)
            continue;

        if (tecc_formatter_hex_decode(wa->hex_strings[i], &bin, &bin_len) == 0 &&
            bin != NULL && bin_len >= 24 &&
            tecc_formatter_parse_cipherdata((CipherDataType *)bin, bin_len, &pd) == 0 &&
            pd.rnd_data_len == 0 &&
            (pd.eg_add_data_len > 0 || pd.eg_mul_data_len > 0))
        {
            char        *he_str = NULL;
            size_t       he_str_len = 0;
            const BIGNUM *row_priv = (pd.eg_add_data_len > 0) ? wa->ec_priv : wa->ff_priv;

            if (ctx != NULL &&
                tecc_he_decrypt_cipherdata_ex(pd.eg_add_data, pd.eg_add_data_len,
                                              pd.eg_mul_data, pd.eg_mul_data_len,
                                              row_priv, ctx,
                                              &he_str, &he_str_len) == 0 &&
                he_str != NULL)
            {
                wa->results[i] = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
                if (wa->results[i] != NULL)
                {
                    wa->results[i]->plaintext = he_str;
                    wa->results[i]->plain_len = he_str_len;
                    wa->results[i]->skipped_decrypt = true;
                    wa->results[i]->error_code = TECC_CRYPTO_SUCCESS;
                }
                else
                    pg_free(he_str);
            }
        }
        if (bin)
            pg_free(bin);
    }

    if (ctx)
        BN_CTX_free(ctx);
    return NULL;
}
#endif /* !WIN32 */

/* ReqID:TECC4.0 - HE 并行 worker 数：TECC_HE_THREADS 环境变量（0/1=串行），
 * 默认 8，实际取 clamp(N, 1, min(核数, 行数))。Windows 下无 pthread_create，
 * 恒返回 1（串行）。 */
static int
tecc_he_get_thread_count(int row_count)
{
#ifdef WIN32
    /* ReqID:TECC4.0 - Windows 禁用并行（pthread-win32 无 pthread_create） */
    return 1;
#else
    const char *env;
    long        cores;
    int         n = 8;

    env = getenv("TECC_HE_THREADS");
    if (env != NULL && env[0] != '\0')
    {
        n = atoi(env);
        if (n <= 1)
            return 1;
    }

    cores = sysconf(_SC_NPROCESSORS_ONLN);
    if (cores < 1)
        cores = 1;
    if (cores > 256)
        cores = 256;
    if (n > (int)cores)
        n = (int)cores;
    if (n > row_count)
        n = row_count;
    if (n < 1)
        n = 1;
    return n;
#endif
}

static TeccUnifiedDecryptResult *tecc_unified_decrypt_hex_internal_with_scheduler(
    PGconn *conn,
    TeccCryptoScheduler *scheduler,
    const char *hex_string,
    const char *table_name,
    const char *column_name,
    TeccCryptoContext *external_context,
    bool supports_he_add,
    bool supports_he_mul);

static TeccUnifiedEncryptResult *tecc_encrypt_search_with_rnd(
    PGconn *conn,
    TeccCryptoScheduler *scheduler,
    const char *plaintext,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name,
    TeccOperatorType operator_type);

static TeccUnifiedEncryptResult *tecc_unified_encrypt_string_internal_with_scheduler(
    TeccCryptoScheduler *scheduler,
    const char *plaintext,
    const char *table_name,
    const char *column_name,
    const char *schema_name,
    const char *kek_name,
    const char *dek_name,
    Oid collation_oid,
    void *tecc_state,
    TeccEncryptedColumnMetadata *col_meta_in,
    TeccCryptoContext *external_context,
    bool supports_he_add,
    bool supports_he_mul);

/* Thread-safe encryption function with connection parameter */
TeccUnifiedEncryptResult *tecc_unified_encrypt_string_with_conn(
    PGconn *conn,
    const char *plaintext,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name);

/* Stream crypto context creation with explicit scheduler */
static TeccStreamCryptoContext *tecc_create_stream_crypto_context_internal(
    TeccCryptoScheduler *scheduler,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name);

/* Note: Mock metadata functions have been removed. Real cache should be used instead. */

/* Helper function to get KEK name from column key ID - REMOVED */
/* Use tecc_convert_column_cache_item_to_metadata instead, which returns KEK/DEK names from cache */

/* Helper function to get DEK name from column key ID - REMOVED */
/* Use tecc_convert_column_cache_item_to_metadata instead, which returns KEK/DEK names from cache */

/*
 * Initialize crypto scheduler
 */
TeccCryptoScheduler *
tecc_crypto_scheduler_init(TeccMetadataCache *cache, TeccKeyManagerAdapter *key_mgr)
{
    TeccCryptoScheduler *scheduler;

    /* Allocate scheduler structure */
    scheduler = pg_malloc0(sizeof(TeccCryptoScheduler));
    if (!scheduler)
    {
        TECC_SCHEDULER_ERROR(
                           "Failed to allocate crypto scheduler");
        return NULL;
    }

    /* Set module references */
    scheduler->metadata_cache = cache;
    scheduler->key_manager = key_mgr;

    /* Default configuration */
    scheduler->max_retry_count = 3;

    /* Initialize GUC integration */
    scheduler->guc_params = NULL;
    scheduler->guc_enabled = true;  /* Enable GUC processing by default */

    /* Initialize statistics */
    scheduler->encryption_count = 0;
    scheduler->decryption_count = 0;
    scheduler->error_count = 0;

    /* Initialize batch processing statistics */
    scheduler->batch_encryption_count = 0;
    scheduler->batch_decryption_count = 0;
    scheduler->batch_error_count = 0;
    scheduler->total_batch_items_processed = 0;
    scheduler->successful_batch_items = 0;

    /* Use the provided key manager as key adapter */
    scheduler->key_adapter = key_mgr;
    /* Note: key_mgr should already be initialized by the caller */

    /* Global scheduler removed for thread safety - use connection-specific scheduler */

    /* Temporarily disable debug log to avoid segmentation fault */
    /* TECC_SCHEDULER_DEBUG("Initialized crypto scheduler"); */

    return scheduler;
}

/*
 * Cleanup crypto scheduler
 */
void
tecc_crypto_scheduler_cleanup(TeccCryptoScheduler *scheduler)
{
    if (!scheduler)
        return;

    TECC_SCHEDULER_DEBUG(
                        "Cleaning up crypto scheduler (enc=%d, dec=%d, err=%d)",
                        scheduler->encryption_count,
                        scheduler->decryption_count,
                        scheduler->error_count);

    /* Cleanup key manager adapter */
    if (scheduler->key_adapter)
    {
        tecc_key_manager_adapter_destroy(scheduler->key_adapter);
        scheduler->key_adapter = NULL;
    }

    /* Clear scheduler */
    memset(scheduler, 0, sizeof(TeccCryptoScheduler));
    pg_free(scheduler);

    /* Global scheduler reference removed for thread safety */
}

/*
 * Set debug mode
 */
void
tecc_crypto_scheduler_set_debug(TeccCryptoScheduler *scheduler, bool enable)
{
    if (!scheduler)
        return;

    scheduler->debug_mode = enable;

    TECC_SCHEDULER_DEBUG(
                        "Crypto scheduler debug mode: %s",
                        enable ? "enabled" : "disabled");
}

/*
 * Get statistics
 */
void
tecc_crypto_scheduler_get_stats(TeccCryptoScheduler *scheduler,
                               int *enc_count, int *dec_count, int *err_count)
{
    if (!scheduler)
        return;

    if (enc_count)
        *enc_count = scheduler->encryption_count;
    if (dec_count)
        *dec_count = scheduler->decryption_count;
    if (err_count)
        *err_count = scheduler->error_count;
}

/*
 * Get batch processing statistics
 */
void
tecc_crypto_scheduler_get_batch_stats(TeccCryptoScheduler *scheduler,
                                    int *batch_enc_count,
                                    int *batch_dec_count,
                                    int *batch_error_count,
                                    size_t *total_batch_items,
                                    size_t *successful_batch_items)
{
    if (!scheduler)
        return;

    if (batch_enc_count)
        *batch_enc_count = scheduler->batch_encryption_count;
    if (batch_dec_count)
        *batch_dec_count = scheduler->batch_decryption_count;
    if (batch_error_count)
        *batch_error_count = scheduler->batch_error_count;
    if (total_batch_items)
        *total_batch_items = scheduler->total_batch_items_processed;
    if (successful_batch_items)
        *successful_batch_items = scheduler->successful_batch_items;
}

/*
 * Ensure crypto scheduler is initialized (lazy initialization)
 * DEPRECATED: Use connection-specific scheduler instead
 */
bool
tecc_crypto_scheduler_ensure_initialized(void)
{
    TECC_SCHEDULER_ERROR("tecc_crypto_scheduler_ensure_initialized is deprecated, use connection-specific scheduler");
    return false;

    /* Check if already initialized */
    /* if (g_crypto_scheduler != NULL) */
    /*     return true; */

    /* Initialize formatter module first */
    /* tecc_formatter_initialize(); */

    /* Initialize with NULL cache and key manager */
    /* g_crypto_scheduler = tecc_crypto_scheduler_init(NULL, NULL); */
    /* if (!g_crypto_scheduler) */
    /* { */
    /*     TECC_SCHEDULER_ERROR("Failed to initialize crypto scheduler"); */
    /*     return false; */
    /* } */

    /* TECC_SCHEDULER_DEBUG("Lazy initialized crypto scheduler"); */
    /* return true; */
}

/*
 * Cleanup global scheduler instance
 * DEPRECATED: Use connection-specific scheduler instead
 */
void
tecc_crypto_scheduler_cleanup_global(void)
{
    TECC_SCHEDULER_ERROR("tecc_crypto_scheduler_cleanup_global is deprecated, use connection-specific scheduler");

    /* if (g_crypto_scheduler != NULL) */
    /* { */
    /*     TECC_SCHEDULER_DEBUG("Cleaning up global crypto scheduler"); */
    /*     tecc_crypto_scheduler_cleanup(g_crypto_scheduler); */
    /*     g_crypto_scheduler = NULL; */
    /* } */

    /* Cleanup formatter module */
    /* tecc_formatter_cleanup(); */
}

/*
 * Unified encryption interface
 */
TeccUnifiedEncryptResult *
tecc_unified_encrypt_string(PGconn *conn,
                           const char *plaintext,
                           const char *table_name,
                           const char *column_name,
                           const char *kek_name,
                           const char *dek_name,
                           TeccCryptoContext *external_context)
{
    TeccCryptoScheduler *scheduler;

    /* Validate parameters */
    if (conn == NULL || plaintext == NULL || table_name == NULL || column_name == NULL)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_unified_encrypt_string");
        return NULL;
    }

    /* Get scheduler from connection state */
    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to get crypto scheduler from connection");
        return NULL;
    }

    /* Call internal implementation with scheduler */
    return tecc_unified_encrypt_string_internal_with_scheduler(scheduler,
                                                              plaintext,
                                                              table_name,
                                                              column_name,
                                                              NULL, /* schema_name - 未限定，按 search_path 查找 */
                                                              kek_name,
                                                              dek_name,
                                                              0, /* No collation */
                                                              tecc_get_state(conn),
                                                              NULL,
                                                              external_context,
                                                              false,   /* supports_he_add */
                                                              false);  /* supports_he_mul */
}

/*
 * Schema-aware unified encryption.
 * For DML literal encryption on schema-qualified tables (e.g. sch1.tb1):
 * the plain tecc_unified_encrypt_string() has no schema parameter and its
 * metadata lookup falls back to search_path, which misses tables outside
 * the current search_path. Pass the target table's real schema here so the
 * scheduler can resolve column metadata (and DEK) for that table.
 */
TeccUnifiedEncryptResult *
tecc_unified_encrypt_string_with_schema(PGconn *conn,
                                        const char *schema_name,
                                        const char *plaintext,
                                        const char *table_name,
                                        const char *column_name,
                                        const char *kek_name,
                                        const char *dek_name,
                                        TeccCryptoContext *external_context)
{
    TeccCryptoScheduler *scheduler;

    /* Validate parameters */
    if (conn == NULL || plaintext == NULL || table_name == NULL || column_name == NULL)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_unified_encrypt_string_with_schema");
        return NULL;
    }

    /* Get scheduler from connection state */
    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to get crypto scheduler from connection");
        return NULL;
    }

    /* Call internal implementation with scheduler */
    return tecc_unified_encrypt_string_internal_with_scheduler(scheduler,
                                                              plaintext,
                                                              table_name,
                                                              column_name,
                                                              schema_name,
                                                              kek_name,
                                                              dek_name,
                                                              0, /* No collation */
                                                              tecc_get_state(conn),
                                                              NULL,
                                                              external_context,
                                                              false,   /* supports_he_add */
                                                              false);  /* supports_he_mul */
}

/*
 * ReqID:TECC4.0 - Unified encryption with explicit algorithm options.
 * For scenarios where column metadata is not in the cache (e.g. CREATE TABLE
 * DEFAULT encryption). The caller specifies algorithm support directly.
 *
 * collation_locale is an optional override used only when the metadata cache
 * lookup below finds nothing (see tecc_unified_encrypt_string_with_options_and_collation).
 */
static TeccUnifiedEncryptResult *
tecc_unified_encrypt_string_with_options_internal(
    PGconn *conn,
    const char *plaintext,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name,
    Oid data_type_oid,
    int32 type_mod,
    bool supports_det,
    bool supports_ope,
    bool supports_search,
    bool supports_he_add,
    bool supports_he_mul,
    const char *collation_locale,
    const char *he_add_dek_name,
    const char *he_mul_dek_name,
    TeccCryptoContext *external_context)
{
    TeccCryptoScheduler *scheduler;
    TeccEncryptedColumnMetadata col_meta_local;

    if (conn == NULL || plaintext == NULL || table_name == NULL || column_name == NULL)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_unified_encrypt_string_with_options");
        return NULL;
    }

    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to get crypto scheduler from connection");
        return NULL;
    }

    /* Build temporary metadata with caller-supplied options */
    memset(&col_meta_local, 0, sizeof(col_meta_local));
    col_meta_local.table_name = (char *)table_name;
    col_meta_local.column_name = (char *)column_name;
    col_meta_local.data_type_original_oid = data_type_oid;
    col_meta_local.data_type_original_mod = type_mod;
    col_meta_local.enable_det = supports_det;
    col_meta_local.enable_ope = supports_ope;
    col_meta_local.enable_search = supports_search;
    col_meta_local.dek_name = (char *)dek_name;
    col_meta_local.enable_he_add = supports_he_add;
    col_meta_local.enable_he_mul = supports_he_mul;
    col_meta_local.kek_name = (char *)kek_name;
    /* ReqID:TECC4.0 - Look up collation from cache so DET/OPE normalization
     * produces the same ICU sort key as the INSERT VALUES path. Without this,
     * constants in INSERT...SELECT get encrypted with collation=InvalidOid,
     * producing different DET ciphertext than the VALUES path → equality fails.
     *
     * ReqID:TECC4.0 - 建表路径（列还没入库，缓存必然 miss）由调用方显式传入
     * collation_locale，否则同一情景会在建表与查询之间产生两套密文。 */
    col_meta_local.collation_oid = InvalidOid;
    col_meta_local.collation_locale = NULL;
    {
        TeccState *state = tecc_get_state(conn);
        if (state != NULL && table_name != NULL && column_name != NULL)
        {
            const char *db_name = PQdb(conn);
            TeccColumnCacheItem *cache_item = NULL;
            if (db_name == NULL || db_name[0] == '\0')
                db_name = "postgres";
            cache_item =
                tecc_cache_get_encrypted_column_by_name(
                    state, db_name, "public", table_name, column_name);
            if (cache_item == NULL)
                cache_item =
                    tecc_cache_get_encrypted_column_by_name(
                        state, db_name, NULL, table_name, column_name);
            if (cache_item != NULL)
            {
                col_meta_local.collation_oid = cache_item->collation_oid;
                col_meta_local.collation_locale = cache_item->collation_locale;
                /* ReqID:TECC4.0 - 带上 table_oid/column_attnum，使调度器能按
                 * hg_encrypted_column_args 解析列真实的 HE_ADD_DEK/HE_MUL_DEK
                 * （一列多 DEK）。缺失时该解析被跳过，加密器回退到列级 DEK
                 * 找 HE 公钥：纯 HE 列还能靠传入的 HE DEK 名兜住，但
                 * det+he-add 这类混合列会拿对称 DEK 去找 EC 公钥而失败。 */
                col_meta_local.table_oid = cache_item->table_oid;
                col_meta_local.column_attnum = (int16) cache_item->column_attnum;
            }
        }
    }

    /* Cache miss (or a column the cache knows nothing about yet): fall back to
     * the caller-supplied locale so normalization matches the query paths. */
    if (col_meta_local.collation_locale == NULL &&
        collation_locale != NULL && collation_locale[0] != '\0')
        col_meta_local.collation_locale = (char *)collation_locale;

    /* ReqID:TECC4.0 - 同一情形下的同态 DEK 名：目标列还不存在的写入路径
     * （CTAS、视图/物化视图定义里的 cast、加密列 DEFAULT）拿不到
     * hg_encrypted_column_args，同态 DEK 只能由调用方从 ENCRYPTED WITH 里解析后
     * 传入。语义与 TeccEncryptedColumnMetadata.he_add_dek/he_mul_dek 一致
     * （见 tecc_column_cache.c 里从缓存项构造该结构处）。 */
    col_meta_local.he_add_dek = (char *)he_add_dek_name;
    col_meta_local.he_mul_dek = (char *)he_mul_dek_name;

    return tecc_unified_encrypt_string_internal_with_scheduler(
        scheduler,
        plaintext,
        table_name,
        column_name,
        NULL, /* schema_name - 未限定，按 search_path 查找 */
        kek_name,
        dek_name,
        0, /* No collation */
        tecc_get_state(conn),
        &col_meta_local,
        external_context,
        supports_he_add,
        supports_he_mul);
}

/*
 * ReqID:TECC4.0 - Public entry: collation comes from the metadata cache only.
 */
TeccUnifiedEncryptResult *
tecc_unified_encrypt_string_with_options(
    PGconn *conn,
    const char *plaintext,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name,
    Oid data_type_oid,
    int32 type_mod,
    bool supports_det,
    bool supports_ope,
    bool supports_search,
    bool supports_he_add,
    bool supports_he_mul,
    TeccCryptoContext *external_context)
{
    return tecc_unified_encrypt_string_with_options_internal(
        conn, plaintext, table_name, column_name, kek_name, dek_name,
        data_type_oid, type_mod, supports_det, supports_ope, supports_search,
        supports_he_add, supports_he_mul, NULL, NULL, NULL, external_context);
}

/*
 * ReqID:TECC4.0 - Public entry for callers that know the collation the column
 * will end up with (CREATE TABLE ... DEFAULT, before the table exists), and,
 * for the same reason, the homomorphic DEK names the column will be bound to
 * (he_add_dek_name / he_mul_dek_name, from the ENCRYPTED WITH clause).
 */
TeccUnifiedEncryptResult *
tecc_unified_encrypt_string_with_options_and_collation(
    PGconn *conn,
    const char *plaintext,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name,
    Oid data_type_oid,
    int32 type_mod,
    bool supports_det,
    bool supports_ope,
    bool supports_search,
    bool supports_he_add,
    bool supports_he_mul,
    const char *collation_locale,
    const char *he_add_dek_name,
    const char *he_mul_dek_name,
    TeccCryptoContext *external_context)
{
    return tecc_unified_encrypt_string_with_options_internal(
        conn, plaintext, table_name, column_name, kek_name, dek_name,
        data_type_oid, type_mod, supports_det, supports_ope, supports_search,
        supports_he_add, supports_he_mul, collation_locale,
        he_add_dek_name, he_mul_dek_name, external_context);
}


/*
 * SEARCH algorithm encryption with RND ciphertext generation for storage.
 * Used by streaming encryption path (tecc_stream_encrypt_data).
 */
static TeccUnifiedEncryptResult *
tecc_encrypt_search_with_rnd(
    PGconn *conn,
    TeccCryptoScheduler *scheduler,
    const char *plaintext,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name,
    TeccOperatorType operator_type)
{
    TeccCryptoContextExtended *context = NULL;
    TeccUnifiedEncryptResult *result = NULL;
    TeccEncryptedColumnMetadata *col_meta = NULL;
    unsigned char *dek_key = NULL;
    size_t dek_key_len = 0;
    unsigned char *derived_enc_key = NULL;    /* ReqID:TECC4.0 */
    unsigned char *derived_auth_key = NULL;   /* ReqID:TECC4.0 */
    int ret;
    CipherDataType *cipherdata = NULL;
    size_t cipherdata_len = 0;
    TeccFormatterErrorCode fmt_ret;
    unsigned char *rnd_ciphertext;
    size_t rnd_cipher_len;
    unsigned char *search_ciphertext;
    size_t search_cipher_len;
    TeccCryptoBuffer *cipherdata_buffer;

    /* Validate parameters */
    if (plaintext == NULL || table_name == NULL || column_name == NULL)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_encrypt_search_with_rnd");
        return NULL;
    }

    /* Get metadata for the column */
    if (conn != NULL)
    {
        /* Use real cache with connection */
        TeccState *state = tecc_get_state(conn);
        TeccColumnCacheItem *cache_item = NULL;

        if (state != NULL)
        {
            /* 从连接中获取实际数据库名，替代硬编码的"postgres" */
            const char *db_name = PQdb(conn);
            if (db_name == NULL || db_name[0] == '\0')
                db_name = "postgres";
            cache_item = tecc_cache_get_encrypted_column_by_name(state, db_name, NULL, table_name, column_name);
            if (cache_item != NULL)
            {
                col_meta = tecc_convert_column_cache_item_to_metadata(state, cache_item);
            }
        }
    }

    if (col_meta == NULL)
    {
        TECC_SCHEDULER_ERROR("Column %s.%s not found in metadata cache", table_name, column_name);
        return NULL;
    }

    /* Check if SEARCH algorithm is enabled for this column */
    if (!col_meta->enable_search)
    {
        TECC_SCHEDULER_ERROR("SEARCH algorithm not enabled for column %s.%s", table_name, column_name);
        /* Free metadata if it was allocated */
        if (conn != NULL && col_meta != NULL)
        {
            tecc_free_encrypted_column_metadata(col_meta);
        }
        return NULL;
    }

    /* Allocate context */
    context = (TeccCryptoContextExtended *)pg_malloc(sizeof(TeccCryptoContextExtended));
    if (context == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate crypto context");
        /* Use appropriate free function based on source */
        if (conn != NULL)
        {
            tecc_free_encrypted_column_metadata(col_meta);
        }
        /* Note: If conn is NULL, col_meta should also be NULL since we don't use mock data anymore */
        return NULL;
    }

    /* Initialize context */
    memset(context, 0, sizeof(TeccCryptoContextExtended));
    context->core.data_type = tecc_map_to_original_type_oid(col_meta->data_type_original_oid);
    context->core.type_mod = col_meta->data_type_original_mod;
    context->core.collation_oid = col_meta->collation_oid;
    if (col_meta->collation_locale != NULL)
        strlcpy(context->core.collation_locale, col_meta->collation_locale, NAMEDATALEN);
    context->core.algo_type = TECC_ALGO_SEARCH;
    /* Initialize algo_support structure */
    context->core.algo_support.supports_rnd = false;
    context->core.algo_support.supports_det = false;
    context->core.algo_support.supports_ope = false;
    context->core.algo_support.supports_search = false;
    context->core.selected_algorithms = 0;
    context->core.buffer_manager = NULL;

    /* Set extended identification fields */
    strlcpy(context->table_name, table_name, NAMEDATALEN);
    strlcpy(context->column_name, column_name, NAMEDATALEN);
    if (kek_name != NULL)
        strlcpy(context->kek_name, kek_name, NAMEDATALEN);
    if (dek_name != NULL)
        strlcpy(context->dek_name, dek_name, NAMEDATALEN);

    /* Set input data */
    TECC_SCHEDULER_DEBUG("DEBUG: Setting plaintext: plaintext=%p, strlen=%zu",
                        plaintext, plaintext ? strlen(plaintext) : 0);
    context->plaintext = (const unsigned char *)plaintext;
    context->plaintext_len = plaintext ? strlen(plaintext) : 0;

    /* Set operator type for SEARCH algorithm */
    context->operator_type = operator_type;

    /* Check if this is a LIKE pattern */
    if (operator_type == TECC_OP_LIKE || operator_type == TECC_OP_NOT_LIKE ||
        operator_type == TECC_OP_ILIKE || operator_type == TECC_OP_NOT_ILIKE)
    {
        context->is_like_pattern = true;
        TECC_SCHEDULER_DEBUG("Processing LIKE pattern: %s", plaintext);
    }
    else
    {
        context->is_like_pattern = false;
    }

    /* Set algorithm support based on metadata */
    context->core.algo_support.supports_rnd = true;  /* RND is always supported */
    context->core.algo_support.supports_det = col_meta->enable_det;
    context->core.algo_support.supports_ope = col_meta->enable_ope;
    context->core.algo_support.supports_search = col_meta->enable_search;
        context->core.algo_support.supports_he_add = col_meta->enable_he_add;
        context->core.algo_support.supports_he_mul = col_meta->enable_he_mul;
    /* Begin - ReqID:TECC4.0 - HE algorithm support */
    context->core.algo_support.supports_he_add = col_meta->enable_he_add;
    context->core.algo_support.supports_he_mul = col_meta->enable_he_mul;
    /* End - ReqID:TECC4.0 */

    /* Set selected algorithms.
     * ReqID:TECC4.0 - HE-only columns (he-add/he-mul, no symmetric algo) must
     * NOT select RND, otherwise the RND slot gets filled and the HE decrypt
     * path (which requires rnd_data_len == 0) is skipped.  For HE-only, only
     * the HE algorithm flags are set; selected_algorithms stays 0 (RND slot
     * empty).  Mixed columns keep RND. */
    if (col_meta != NULL &&
        (col_meta->enable_he_add || col_meta->enable_he_mul) &&
        !col_meta->enable_det && !col_meta->enable_ope && !col_meta->enable_search)
        context->core.selected_algorithms = 0;
    else
        context->core.selected_algorithms = TECC_ALGO_RND;

    /* Get DEK key with pre-derived SM4-RND keys */
    ret = tecc_key_manager_adapter_get_dek_key_ext(scheduler->key_adapter,
                                              context->dek_name,
                                              &dek_key,
                                              &dek_key_len,
                                              &derived_enc_key,
                                              &derived_auth_key);
    if (ret != TECC_CRYPTO_SUCCESS)
    {
        TECC_SCHEDULER_ERROR("Failed to get DEK key for %s", context->dek_name);
        pg_free(context);
        /* Use appropriate free function based on source */
        if (conn != NULL)
        {
            tecc_free_encrypted_column_metadata(col_meta);
        }
        /* Note: If conn is NULL, col_meta should also be NULL since we don't use mock data anymore */
        return NULL;
    }

    context->dek_key = dek_key;
    context->dek_key_len = dek_key_len;

    /* Apply GUC parameters if enabled */
    if (scheduler->guc_enabled && scheduler->guc_params != NULL)
    {
        context->guc_params = scheduler->guc_params;
        context->guc_processing_enabled = true;

        ret = tecc_crypto_scheduler_apply_guc_to_data(scheduler, context);
        if (ret != TECC_CRYPTO_SUCCESS)
        {
            TECC_SCHEDULER_ERROR("Failed to apply GUC parameters to data");
            tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
            /* Clear dangling pointer after key release */
            context->dek_key = NULL;
            context->dek_key_len = 0;
            pg_free(context);
            /* Use appropriate free function based on source */
        if (conn != NULL)
        {
            tecc_free_encrypted_column_metadata(col_meta);
        }
        /* Note: If conn is NULL, col_meta should also be NULL since we don't use mock data anymore */
            return NULL;
        }
    }

    /* Allocate result structure */
    result = (TeccUnifiedEncryptResult *)pg_malloc(sizeof(TeccUnifiedEncryptResult));
    if (result == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate result structure");
        tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
        /* Clear dangling pointer after key release */
        context->dek_key = NULL;
        context->dek_key_len = 0;
        pg_free(context);
        /* Use appropriate free function based on source */
        if (conn != NULL)
        {
            tecc_free_encrypted_column_metadata(col_meta);
        }
        /* Note: If conn is NULL, col_meta should also be NULL since we don't use mock data anymore */
        return NULL;
    }

    memset(result, 0, sizeof(TeccUnifiedEncryptResult));

    /* Call algorithm scheduler for SEARCH algorithm with operator type */
    ret = tecc_algorithm_scheduler_encrypt_search_with_operator(context,
                                                               operator_type,
                                                               &search_ciphertext,
                                                               &search_cipher_len);
    if (ret != TECC_CRYPTO_SUCCESS)
    {
        TECC_SCHEDULER_ERROR("SEARCH algorithm encryption failed with error %d", ret);
        strlcpy(result->error_msg, "SEARCH algorithm encryption failed", sizeof(result->error_msg));
        result->error_code = ret;
        tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
        /* Clear dangling pointer after key release */
        context->dek_key = NULL;
        context->dek_key_len = 0;
        pg_free(context);
        pg_free(result);
        /* Use appropriate free function based on source */
        if (conn != NULL)
        {
            tecc_free_encrypted_column_metadata(col_meta);
        }
        /* Note: If conn is NULL, col_meta should also be NULL since we don't use mock data anymore */
        return NULL;
    }

    /* Store SEARCH ciphertext in context */
    context->search_ciphertext = search_ciphertext;
    context->search_cipher_len = search_cipher_len;

    /* For SEARCH algorithm, we also need RND ciphertext for storage */
    ret = tecc_algorithm_scheduler_encrypt_rnd(context,
                                              &rnd_ciphertext,
                                              &rnd_cipher_len,
                                              derived_enc_key,
                                              derived_auth_key);
    if (ret != TECC_CRYPTO_SUCCESS)
    {
        TECC_SCHEDULER_ERROR("RND algorithm encryption failed with error %d", ret);
        strlcpy(result->error_msg, "RND algorithm encryption failed", sizeof(result->error_msg));
        result->error_code = ret;
        tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
        /* Clear dangling pointer after key release */
        context->dek_key = NULL;
        context->dek_key_len = 0;
        pg_free(search_ciphertext);
        pg_free(context);
        pg_free(result);
        /* Use appropriate free function based on source */
        if (conn != NULL)
        {
            tecc_free_encrypted_column_metadata(col_meta);
        }
        /* Note: If conn is NULL, col_meta should also be NULL since we don't use mock data anymore */
        return NULL;
    }

    context->rnd_ciphertext = rnd_ciphertext;
    context->rnd_cipher_len = rnd_cipher_len;

    /* Format cipherdata for transmission */
    fmt_ret = tecc_formatter_format_cipherdata(context, &cipherdata, &cipherdata_len);
    if (fmt_ret != TECC_FORMATTER_SUCCESS)
    {
        TECC_SCHEDULER_ERROR("Failed to format cipherdata with error %d", fmt_ret);
        strlcpy(result->error_msg, "Failed to format cipherdata", sizeof(result->error_msg));
        result->error_code = tecc_formatter_error_to_scheduler_error(fmt_ret);
        tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
        /* Clear dangling pointer after key release */
        context->dek_key = NULL;
        context->dek_key_len = 0;
        pg_free(search_ciphertext);
        pg_free(rnd_ciphertext);
        pg_free(context);
        pg_free(result);
        /* Use appropriate free function based on source */
        if (conn != NULL)
        {
            tecc_free_encrypted_column_metadata(col_meta);
        }
        /* Note: If conn is NULL, col_meta should also be NULL since we don't use mock data anymore */
        return NULL;
    }

    /* Create cipherdata buffer */
    cipherdata_buffer = (TeccCryptoBuffer *)pg_malloc(sizeof(TeccCryptoBuffer));
    if (cipherdata_buffer == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate cipherdata buffer");
        tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
        /* Clear dangling pointer after key release */
        context->dek_key = NULL;
        context->dek_key_len = 0;
        pg_free(search_ciphertext);
        pg_free(rnd_ciphertext);
        pg_free(cipherdata);
        pg_free(context);
        pg_free(result);
        /* Use appropriate free function based on source */
        if (conn != NULL)
        {
            tecc_free_encrypted_column_metadata(col_meta);
        }
        /* Note: If conn is NULL, col_meta should also be NULL since we don't use mock data anymore */
        return NULL;
    }

    cipherdata_buffer->data = cipherdata;
    cipherdata_buffer->size = cipherdata_len;
    cipherdata_buffer->used = cipherdata_len;
    cipherdata_buffer->owned_by_caller = true;
    cipherdata_buffer->next = NULL;

    /* Fill result structure */
    result->cipherdata_buffer = cipherdata_buffer;
    result->error_code = TECC_CRYPTO_SUCCESS;
    strlcpy(result->error_msg, "Success", sizeof(result->error_msg));

    /* Debug: print cipherdata before hex encode */
    TECC_SCHEDULER_DEBUG("cipherdata payload hex dump (%d bytes)",
                         (int)(cipherdata_len - sizeof(CipherDataType)));

    /* Convert to hex string for compatibility */
    fmt_ret = tecc_formatter_hex_encode((const unsigned char *)cipherdata, cipherdata_len, &result->hex_string);
            if (fmt_ret != TECC_FORMATTER_SUCCESS)
    {
        TECC_SCHEDULER_ERROR("Failed to convert cipherdata to hex");
        tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
        /* Clear dangling pointer after key release */
        context->dek_key = NULL;
        context->dek_key_len = 0;
        pg_free(search_ciphertext);
        pg_free(rnd_ciphertext);
        pg_free(cipherdata);
        pg_free(cipherdata_buffer);
        pg_free(context);
        pg_free(result);
        /* Use appropriate free function based on source */
        if (conn != NULL)
        {
            tecc_free_encrypted_column_metadata(col_meta);
        }
        /* Note: If conn is NULL, col_meta should also be NULL since we don't use mock data anymore */
        return NULL;
    }

    result->hex_len = strlen(result->hex_string);

    /* Update statistics */
    scheduler->encryption_count++;

    /* Cleanup */
    tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
    /* Clear dangling pointer after key release */
    context->dek_key = NULL;
    context->dek_key_len = 0;
    pg_free(search_ciphertext);
    pg_free(rnd_ciphertext);
    pg_free(context);
    /* Free metadata if it was allocated from real cache */
    if (conn != NULL && col_meta != NULL)
    {
        tecc_free_encrypted_column_metadata(col_meta);
    }
    /* Note: If conn is NULL, col_meta should also be NULL since we don't use mock data anymore */

    TECC_SCHEDULER_DEBUG("SEARCH algorithm encryption with operator type %d completed successfully", operator_type);
    return result;
}


/*
 * Unified decryption interface
 * Note: This function now includes connection parameter for thread safety
 */
TeccUnifiedDecryptResult *
tecc_unified_decrypt_hex(PGconn *conn,
                        const char *hex_string,
                        const char *table_name,
                        const char *column_name,
                        TeccCryptoContext *external_context)
{
    TeccCryptoScheduler *scheduler;

    /* Validate parameters */
    if (conn == NULL || hex_string == NULL || table_name == NULL || column_name == NULL)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_unified_decrypt_hex");
        return NULL;
    }

    /* Get scheduler from connection state */
    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to get crypto scheduler from connection");
        return NULL;
    }

    /* Call internal implementation with scheduler */
    return tecc_unified_decrypt_hex_internal_with_scheduler(conn,
                                                           scheduler,
                                                           hex_string,
                                                           table_name,
                                                           column_name,
                                                           external_context,
                                                           false,   /* supports_he_add */
                                                           false);  /* supports_he_mul */
}

/* ReqID:TECC4.0 - Decrypt raw algorithm segment hex for tecc_decrypt_raw(). */
TeccUnifiedDecryptResult *
tecc_decrypt_raw_segment_hex(PGconn *conn,
							 const char *hex_string,
							 const char *algorithm,
							 const char *schema_name,
							 const char *table_name,
							 const char *column_name,
							 TeccCryptoContext *external_context)
{
	TeccCryptoScheduler *scheduler;
	TeccUnifiedDecryptResult *result;
	TeccCryptoContextExtended *context;
	bool		context_is_external;
	TeccState  *state;
	TeccColumnCacheItem *cache_item;
	TeccEncryptedColumnMetadata *col_meta;
	TeccAlgorithmType algo_type;
	bool		is_he_add;
	bool		is_he_mul;
	unsigned char *cipher_data;
	size_t		cipher_len;
	unsigned char *dek_key;
	size_t		dek_key_len;
	unsigned char *derived_enc_key;
	unsigned char *derived_auth_key;
	unsigned char *plaintext;
	size_t		plain_len;
	size_t		buffer_size;
	const unsigned char *decrypt_cipher_data;
	size_t		decrypt_cipher_len;
	uint32_t	text_space_count;
	int			ret;
	TeccFormatterErrorCode fmt_ret;

	if (conn == NULL || hex_string == NULL || algorithm == NULL ||
		table_name == NULL || column_name == NULL)
		return NULL;

	scheduler = tecc_get_scheduler_from_conn(conn);
	state = tecc_get_state(conn);
	if (scheduler == NULL || state == NULL)
		return NULL;

	result = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
	if (result == NULL)
		return NULL;
	result->error_code = TECC_CRYPTO_SUCCESS;
	is_he_add = false;
	is_he_mul = false;

	if (pg_strcasecmp(algorithm, "rnd") == 0)
		algo_type = TECC_ALGO_RND;
	else if (pg_strcasecmp(algorithm, "ope") == 0)
		algo_type = TECC_ALGO_OPE;
	else if (pg_strcasecmp(algorithm, "det") == 0)
		algo_type = TECC_ALGO_DET;
	else if (pg_strcasecmp(algorithm, "he_add") == 0 ||
			 pg_strcasecmp(algorithm, "he-add") == 0)
	{
		algo_type = TECC_ALGO_RND;
		is_he_add = true;
	}
	else if (pg_strcasecmp(algorithm, "he_mul") == 0 ||
			 pg_strcasecmp(algorithm, "he-mul") == 0)
	{
		algo_type = TECC_ALGO_RND;
		is_he_mul = true;
	}
	else if (pg_strcasecmp(algorithm, "search") == 0)
	{
		result->error_code = TECC_CRYPTO_ALGO_ERROR;
		snprintf(result->error_msg, sizeof(result->error_msg),
				 "SEARCH raw segment decryption is not supported");
		return result;
	}
	else
	{
		result->error_code = TECC_CRYPTO_ALGO_ERROR;
		snprintf(result->error_msg, sizeof(result->error_msg),
				 "raw segment decryption is not supported for algorithm \"%s\"",
				 algorithm);
		return result;
	}

	cache_item = tecc_cache_get_encrypted_column_by_name(state, PQdb(conn),
														 schema_name,
														 table_name,
														 column_name);
	if (cache_item == NULL)
	{
		result->error_code = TECC_CRYPTO_KEY_ERROR;
		snprintf(result->error_msg, sizeof(result->error_msg),
				 "encrypted column metadata not found for %s.%s",
				 table_name, column_name);
		return result;
	}

	col_meta = tecc_convert_column_cache_item_to_metadata(state, cache_item);
	if (col_meta == NULL ||
		(!is_he_add && !is_he_mul &&
		 (col_meta->dek_name == NULL || col_meta->dek_name[0] == '\0')))
	{
		result->error_code = TECC_CRYPTO_KEY_ERROR;
		snprintf(result->error_msg, sizeof(result->error_msg),
				 "DEK metadata not found for %s.%s", table_name, column_name);
		if (col_meta != NULL)
			tecc_free_encrypted_column_metadata(col_meta);
		return result;
	}

	/* ReqID:TECC4.0 - Check DEK USAGE before decoding ciphertext or loading
	 * key material.  HE algorithms use their algorithm-specific DEK below. */
	if (!is_he_add && !is_he_mul &&
		!tecc_dek_has_usage_permission(state, col_meta->dek_name))
	{
		result->error_code = TECC_CRYPTO_KEY_ERROR;
		snprintf(result->error_msg, sizeof(result->error_msg),
				 "permission denied for DEK \"%s\"", col_meta->dek_name);
		tecc_free_encrypted_column_metadata(col_meta);
		return result;
	}

	fmt_ret = tecc_formatter_hex_decode(hex_string, &cipher_data, &cipher_len);
	if (fmt_ret != TECC_FORMATTER_SUCCESS)
	{
		result->error_code = TECC_CRYPTO_FORMAT_ERROR;
		snprintf(result->error_msg, sizeof(result->error_msg),
				 "Failed to decode raw ciphertext segment");
		tecc_free_encrypted_column_metadata(col_meta);
		return result;
	}

	if (is_he_add || is_he_mul)
	{
		const char *he_dek_name;
		char	   *he_plain;
		size_t		he_plain_len;
		Oid			seg_dek_id;
		int			he_ret;

		he_dek_name = NULL;
		he_plain = NULL;
		he_plain_len = 0;
		seg_dek_id = InvalidOid;
		if (cipher_len >= CIPHERDATA_HE_DEK_ID_SIZE)
			seg_dek_id = *(Oid *)cipher_data;
		if (OidIsValid(seg_dek_id))
			he_dek_name = tecc_cache_get_dek_name_by_distributed_id(state,
																	seg_dek_id);
		if (he_dek_name == NULL && col_meta != NULL)
		{
			if (is_he_add)
				he_dek_name = col_meta->he_add_dek;
			else
				he_dek_name = col_meta->he_mul_dek;
			if (he_dek_name == NULL || he_dek_name[0] == '\0')
				he_dek_name = col_meta->dek_name;
		}

		if (he_dek_name == NULL || he_dek_name[0] == '\0')
		{
			result->error_code = TECC_CRYPTO_KEY_ERROR;
			snprintf(result->error_msg, sizeof(result->error_msg),
					 "HE DEK metadata not found for %s.%s",
					 table_name, column_name);
			pg_free(cipher_data);
			tecc_free_encrypted_column_metadata(col_meta);
			return result;
		}

		/* ReqID:TECC4.0 - HE raw decryption requires USAGE on the DEK
		 * selected by the ciphertext segment, not the column's base DEK. */
		if (!tecc_dek_has_usage_permission(state, he_dek_name))
		{
			result->error_code = TECC_CRYPTO_KEY_ERROR;
			snprintf(result->error_msg, sizeof(result->error_msg),
					 "permission denied for DEK \"%s\"", he_dek_name);
			pg_free(cipher_data);
			tecc_free_encrypted_column_metadata(col_meta);
			return result;
		}

		he_ret = tecc_he_decrypt_cipherdata(is_he_add ? cipher_data : NULL,
											is_he_add ? cipher_len : 0,
											is_he_mul ? cipher_data : NULL,
											is_he_mul ? cipher_len : 0,
											he_dek_name, state,
											&he_plain, &he_plain_len);
		if (he_ret != TECC_ALGO_SUCCESS || he_plain == NULL)
		{
			result->error_code = TECC_CRYPTO_DATA_ERROR;
			snprintf(result->error_msg, sizeof(result->error_msg),
					 "%s raw segment decryption failed",
					 is_he_add ? "HE_ADD" : "HE_MUL");
			if (he_plain != NULL)
				pg_free(he_plain);
		}
		else
		{
			result->plaintext = he_plain;
			result->plain_len = he_plain_len;
			scheduler->decryption_count++;
		}

		pg_free(cipher_data);
		tecc_free_encrypted_column_metadata(col_meta);
		return result;
	}

	context_is_external = (external_context != NULL);
	if (context_is_external)
	{
		context = external_context;
		tecc_crypto_context_reset(context);
		strlcpy(context->table_name, table_name, NAMEDATALEN);
		strlcpy(context->column_name, column_name, NAMEDATALEN);
	}
	else
	{
		context = tecc_create_crypto_context_with_pool(table_name, column_name,
													  NULL, NULL);
		if (context == NULL)
		{
			result->error_code = TECC_CRYPTO_MEMORY_ERROR;
			snprintf(result->error_msg, sizeof(result->error_msg),
					 "Failed to create crypto context");
			pg_free(cipher_data);
			tecc_free_encrypted_column_metadata(col_meta);
			return result;
		}
	}

	context->core.algo_support.supports_det = col_meta->enable_det;
	context->core.algo_support.supports_ope = col_meta->enable_ope;
	context->core.algo_support.supports_search = col_meta->enable_search;
	context->core.algo_support.supports_he_add = col_meta->enable_he_add;
	context->core.algo_support.supports_he_mul = col_meta->enable_he_mul;
	context->core.data_type = tecc_map_to_original_type_oid(col_meta->data_type_original_oid);
	context->core.type_mod = col_meta->data_type_original_mod;
	context->core.collation_oid = col_meta->collation_oid;
	if (col_meta->collation_locale != NULL)
		strlcpy(context->core.collation_locale, col_meta->collation_locale,
				NAMEDATALEN);

	dek_key = NULL;
	dek_key_len = 0;
	derived_enc_key = NULL;
	derived_auth_key = NULL;
	ret = tecc_key_manager_adapter_get_dek_key_ext(scheduler->key_adapter,
												   col_meta->dek_name,
												   &dek_key, &dek_key_len,
												   &derived_enc_key,
												   &derived_auth_key);
	if (ret != TECC_CRYPTO_SUCCESS)
	{
		result->error_code = TECC_CRYPTO_KEY_ERROR;
		snprintf(result->error_msg, sizeof(result->error_msg),
				 "Failed to load DEK for raw segment decryption");
		pg_free(cipher_data);
		if (!context_is_external)
			pg_free(context);
		tecc_free_encrypted_column_metadata(col_meta);
		return result;
	}

	context->dek_key = dek_key;
	context->dek_key_len = dek_key_len;
	decrypt_cipher_data = cipher_data;
	decrypt_cipher_len = cipher_len;
	text_space_count = 0;

	/* ReqID:TECC4.0 - Text DET/OPE segments wrap the encrypted core as
	 * [format byte][ciphertext][trailing-space count:4].  The comparison
	 * format is part of CipherDataType, not input to the DET/OPE decryptor.
	 * OPE returns the normalized input only; trailing spaces stay in the
	 * wrapper and are not re-appended. */
	if ((algo_type == TECC_ALGO_DET || algo_type == TECC_ALGO_OPE) &&
		TECC_IS_TEXT_TYPE(context->core.data_type))
	{
		unsigned char format_byte;

		if (cipher_len < 5)
		{
			result->error_code = TECC_CRYPTO_FORMAT_ERROR;
			snprintf(result->error_msg, sizeof(result->error_msg),
					 "invalid text ciphertext wrapper");
			tecc_key_manager_adapter_release_key(scheduler->key_adapter,
											 dek_key, dek_key_len);
			pg_free(cipher_data);
			if (!context_is_external)
				pg_free(context);
			tecc_free_encrypted_column_metadata(col_meta);
			return result;
		}

		format_byte = cipher_data[0];
		text_space_count = ((uint32_t)cipher_data[cipher_len - 4] << 24) |
			((uint32_t)cipher_data[cipher_len - 3] << 16) |
			((uint32_t)cipher_data[cipher_len - 2] << 8) |
			(uint32_t)cipher_data[cipher_len - 1];
		if (format_byte > 1 ||
			(format_byte == 0 && text_space_count != 0) ||
			(format_byte == 1 && text_space_count == 0))
		{
			result->error_code = TECC_CRYPTO_FORMAT_ERROR;
			snprintf(result->error_msg, sizeof(result->error_msg),
					 "invalid text DET ciphertext wrapper");
			tecc_key_manager_adapter_release_key(scheduler->key_adapter,
											 dek_key, dek_key_len);
			pg_free(cipher_data);
			if (!context_is_external)
				pg_free(context);
			tecc_free_encrypted_column_metadata(col_meta);
			return result;
		}

		decrypt_cipher_data = cipher_data + 1;
		decrypt_cipher_len = cipher_len - 5;
	}

	if (algo_type == TECC_ALGO_RND && col_meta != NULL)
	{
		if (tecc_setup_hsm_context(context, col_meta, state) < 0)
		{
			result->error_code = TECC_CRYPTO_KEY_ERROR;
			snprintf(result->error_msg, sizeof(result->error_msg),
					 "HSM hardware required but initialization failed");
			tecc_key_manager_adapter_release_key(scheduler->key_adapter,
												 dek_key, dek_key_len);
			pg_free(cipher_data);
			if (!context_is_external)
				pg_free(context);
			tecc_free_encrypted_column_metadata(col_meta);
			return result;
		}
	}

	if (algo_type == TECC_ALGO_RND && context->hsm_adapter != NULL &&
		context->hsm_device != NULL)
		buffer_size = decrypt_cipher_len;
	else
		buffer_size = tecc_algo_calc_decrypt_buffer_size(algo_type,
												 decrypt_cipher_len);
	if (buffer_size == 0)
		buffer_size = decrypt_cipher_len;
	/* ReqID:TECC4.0 - Only DET re-appends trailing spaces after decrypt;
	 * OPE buffer stays the normalized plaintext size. */
	if (algo_type == TECC_ALGO_DET &&
		text_space_count > SIZE_MAX - buffer_size)
	{
		result->error_code = TECC_CRYPTO_FORMAT_ERROR;
		snprintf(result->error_msg, sizeof(result->error_msg),
				 "text trailing-space count is too large");
		tecc_key_manager_adapter_release_key(scheduler->key_adapter,
										 dek_key, dek_key_len);
		pg_free(cipher_data);
		if (!context_is_external)
			pg_free(context);
		tecc_free_encrypted_column_metadata(col_meta);
		return result;
	}
	if (algo_type == TECC_ALGO_DET)
		buffer_size += text_space_count;

	plaintext = pg_malloc(buffer_size);
	if (plaintext == NULL)
	{
		result->error_code = TECC_CRYPTO_MEMORY_ERROR;
		snprintf(result->error_msg, sizeof(result->error_msg),
				 "Failed to allocate raw segment plaintext buffer");
		tecc_key_manager_adapter_release_key(scheduler->key_adapter,
											 dek_key, dek_key_len);
		pg_free(cipher_data);
		if (!context_is_external)
			pg_free(context);
		tecc_free_encrypted_column_metadata(col_meta);
		return result;
	}

	plain_len = 0;
	if (algo_type == TECC_ALGO_RND && context->hsm_adapter != NULL &&
		context->hsm_device != NULL)
	{
		HsmAdapterInterface *hsm;

		hsm = (HsmAdapterInterface *)context->hsm_adapter;
		ret = TECC_CRYPTO_KEY_ERROR;
		if (hsm->sm4_decrypt != NULL && cipher_len > 16)
		{
			unsigned char sm4_key[TECC_SM4_KEY_SIZE];
			size_t		actual_key_len;
			size_t		hsm_plain_len;
			HsmErrorCode hsm_ret;

			actual_key_len = (dek_key_len >= TECC_SM4_KEY_SIZE) ?
				TECC_SM4_KEY_SIZE : dek_key_len;
			memcpy(sm4_key, dek_key, actual_key_len);
			hsm_plain_len = buffer_size;
			hsm_ret = hsm->sm4_decrypt((HsmDevice *)context->hsm_device,
									   sm4_key, actual_key_len,
									   cipher_data,
									   cipher_data + 16,
									   cipher_len - 16,
									   plaintext, &hsm_plain_len);
			memset(sm4_key, 0, sizeof(sm4_key));
			if (hsm_ret == HSM_SUCCESS)
			{
				plain_len = hsm_plain_len;
				ret = TECC_CRYPTO_SUCCESS;
			}
		}
	}
	else if (algo_type == TECC_ALGO_RND &&
			 derived_enc_key != NULL && derived_auth_key != NULL)
	{
		ret = tecc_sm4_rnd_decrypt_with_keys(dek_key, TECC_SM4_KEY_SIZE,
											 decrypt_cipher_data,
											 decrypt_cipher_len,
											 plaintext, buffer_size,
											 &plain_len,
											 derived_enc_key,
											 derived_auth_key);
	}
	else
	{
		ret = tecc_algo_decrypt_zero_copy(algo_type,
										  dek_key, dek_key_len,
										  decrypt_cipher_data,
										  decrypt_cipher_len,
										  plaintext, buffer_size,
										  &plain_len);
	}

	if (ret != TECC_CRYPTO_SUCCESS)
	{
		result->error_code = TECC_CRYPTO_DATA_ERROR;
		snprintf(result->error_msg, sizeof(result->error_msg),
				 "%s raw segment decryption failed",
				 tecc_get_algorithm_name(algo_type));
		memset(plaintext, 0, buffer_size);
		pg_free(plaintext);
	}
	else
	{
		/* ReqID:TECC4.0 - DET restores trailing spaces to rebuild the
		 * original text; OPE returns the normalized input as encrypted. */
		if (text_space_count > 0 && algo_type == TECC_ALGO_DET)
		{
			memset(plaintext + plain_len, ' ', text_space_count);
			plain_len += text_space_count;
		}
		result->plaintext = (char *)plaintext;
		result->plain_len = plain_len;
		scheduler->decryption_count++;
	}

	tecc_key_manager_adapter_release_key(scheduler->key_adapter,
										 dek_key, dek_key_len);
	context->dek_key = NULL;
	context->dek_key_len = 0;
	pg_free(cipher_data);
	if (!context_is_external)
		pg_free(context);
	tecc_free_encrypted_column_metadata(col_meta);

	return result;
}



/*
 * Get algorithm support
 * Note: This function now requires connection parameter for thread safety
 */
TeccAlgorithmSupport
tecc_unified_get_algorithm_support(PGconn *conn,
                                  const char *table_name, const char *column_name)
{
    TeccCryptoScheduler *scheduler;
    TeccAlgorithmSupport support = {0};

    /* Validate parameters */
    if (conn == NULL || !table_name || !column_name)
    {
        return support;
    }

    /* Get scheduler from connection state */
    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_WARNING("Failed to get scheduler for algorithm support");
        /* Return default support */
        support.supports_rnd = true;
        support.supports_det = true;
        support.supports_ope = true;
        support.supports_search = false;
        return support;
    }

    /* Call internal implementation with scheduler */
    return tecc_unified_get_algorithm_support_internal_with_scheduler(conn,
                                                                     scheduler,
                                                                     table_name,
                                                                     column_name);
}

/*
 * Internal algorithm support function with scheduler parameter
 */
static TeccAlgorithmSupport tecc_unified_get_algorithm_support_internal_with_scheduler(
    PGconn *conn,
    TeccCryptoScheduler *scheduler,
    const char *table_name,
    const char *column_name)
{
    TeccAlgorithmSupport support = {0};
    TeccEncryptedColumnMetadata *col_meta = NULL;

    if (!table_name || !column_name)
        return support;

    TECC_SCHEDULER_DEBUG(
                        "Getting algorithm support for %s.%s with scheduler",
                        table_name, column_name);

    /* Get from cache */
    if (conn != NULL)
    {
        /* Use real cache with connection */
        TeccState *state = tecc_get_state(conn);
        TeccColumnCacheItem *cache_item = NULL;

        if (state != NULL)
        {
            const char *db_name = PQdb(conn);
            if (db_name == NULL || db_name[0] == '\0')
                db_name = "postgres";
            cache_item = tecc_cache_get_encrypted_column_by_name(state, db_name, NULL, table_name, column_name);
            if (cache_item != NULL)
            {
                col_meta = tecc_convert_column_cache_item_to_metadata(state, cache_item);
            }
        }
    }

    if (col_meta)
    {
        support.supports_rnd = true;  /* Always supported */
        support.supports_det = col_meta->enable_det;
        support.supports_ope = col_meta->enable_ope;
        support.supports_search = col_meta->enable_search;
        support.supports_he_add = col_meta->enable_he_add;
        support.supports_he_mul = col_meta->enable_he_mul;

        /* Free metadata - only if it came from real cache (conn != NULL) */
        if (conn != NULL)
        {
            tecc_free_encrypted_column_metadata(col_meta);
        }
    }
    else
    {
        /* If not in cache, return default support */
        support.supports_rnd = true;
        support.supports_det = true;
        support.supports_ope = true;
        support.supports_search = false;  /* Not implemented yet */
    }

    /* Note: col_meta is already freed in the if block above */

    return support;
}

/*
 * Free encrypt result
 */
void
tecc_unified_free_encrypt_result(TeccUnifiedEncryptResult *result)
{
    if (!result)
        return;

    /* Free cipherdata buffer if present */
    if (result->cipherdata_buffer)
    {
        tecc_processor_release_buffer(result->cipherdata_buffer);
        result->cipherdata_buffer = NULL;
    }

    if (result->hex_string)
    {
        /* Securely wipe sensitive data */
        memset(result->hex_string, 0, result->hex_len);
        pg_free(result->hex_string);
        result->hex_string = NULL;
    }

    memset(result, 0, sizeof(TeccUnifiedEncryptResult));
    pg_free(result);
}

/*
 * Free decrypt result
 */
void
tecc_unified_free_decrypt_result(TeccUnifiedDecryptResult *result)
{
    if (!result)
        return;

    if (result->plaintext)
    {
        /* Securely wipe sensitive data */
        memset(result->plaintext, 0, result->plain_len);
        pg_free(result->plaintext);
    }

    memset(result, 0, sizeof(TeccUnifiedDecryptResult));
    pg_free(result);
}

/*
 * Data validation interface
 *
 * Validates input data for encryption/decryption operations.
 * This function provides direct access to data validation logic
 * without requiring full normalization.
 */
int
tecc_crypto_scheduler_validate_input_data(
    const unsigned char *data,
    size_t data_len,
    Oid data_type,
    TeccAlgorithmType algo_type,
    char *error_msg,
    size_t error_msg_size)
{
    TeccFormatterErrorCode fmt_result;

    /* Parameter validation */
    if (!data || data_len == 0) {
        if (error_msg && error_msg_size > 0) {
            snprintf(error_msg, error_msg_size, "Invalid input data: null pointer or zero length");
        }
        return TECC_CRYPTO_INVALID_PARAM;
    }

    if (!error_msg || error_msg_size == 0) {
        /* Caller must provide error message buffer */
        return TECC_CRYPTO_INVALID_PARAM;
    }

    /* Call formatter data validation function */
    fmt_result = tecc_validate_input_data(data, data_len, data_type, algo_type);

    if (fmt_result == TECC_FORMATTER_SUCCESS) {
        snprintf(error_msg, error_msg_size, "Data validation successful");
        return TECC_CRYPTO_SUCCESS;
    }

    /* Convert formatter error to scheduler error */
    switch (fmt_result) {
        /* Parameter errors (maps to TECC_CRYPTO_INVALID_PARAM) */
        case TECC_FORMATTER_ERROR_INVALID_PARAM:
            snprintf(error_msg, error_msg_size, "Invalid parameters for data validation");
            return TECC_CRYPTO_INVALID_PARAM;

        case TECC_FORMATTER_ERROR_NULL_POINTER:
            snprintf(error_msg, error_msg_size, "Null pointer parameter");
            return TECC_CRYPTO_INVALID_PARAM;

        case TECC_FORMATTER_ERROR_INVALID_TYPE:
            snprintf(error_msg, error_msg_size, "Invalid data type: %u", data_type);
            return TECC_CRYPTO_INVALID_PARAM;

        case TECC_FORMATTER_ERROR_INVALID_INPUT:
            snprintf(error_msg, error_msg_size, "Invalid input data for type %u", data_type);
            return TECC_CRYPTO_DATA_ERROR;

        case TECC_FORMATTER_ERROR_UNSUPPORTED_TYPE:
            snprintf(error_msg, error_msg_size, "Unsupported data type: %u", data_type);
            return TECC_CRYPTO_INVALID_PARAM;

        case TECC_FORMATTER_ERROR_DATA_TOO_LARGE:
            snprintf(error_msg, error_msg_size, "Data too large: %zu bytes", data_len);
            return TECC_CRYPTO_DATA_ERROR;

        /* Memory errors (maps to TECC_CRYPTO_MEMORY_ERROR) */
        case TECC_FORMATTER_ERROR_MEMORY_ALLOC:
            snprintf(error_msg, error_msg_size, "Memory allocation error during validation");
            return TECC_CRYPTO_MEMORY_ERROR;

        case TECC_FORMATTER_ERROR_BUFFER_TOO_SMALL:
            snprintf(error_msg, error_msg_size, "Buffer too small for data validation");
            return TECC_CRYPTO_MEMORY_ERROR;

        /* Data errors (maps to TECC_CRYPTO_DATA_ERROR) */
        case TECC_FORMATTER_ERROR_DATA_FORMAT:
            snprintf(error_msg, error_msg_size, "Invalid data format for type %u", data_type);
            return TECC_CRYPTO_DATA_ERROR;

        case TECC_FORMATTER_ERROR_ENCODING_FAILED:
            snprintf(error_msg, error_msg_size, "Encoding failed for data validation");
            return TECC_CRYPTO_DATA_ERROR;

        case TECC_FORMATTER_ERROR_DECODING_FAILED:
            snprintf(error_msg, error_msg_size, "Decoding failed for data validation");
            return TECC_CRYPTO_DATA_ERROR;

        case TECC_FORMATTER_ERROR_CONVERSION_FAILED:
            snprintf(error_msg, error_msg_size, "Data conversion failed for type %u", data_type);
            return TECC_CRYPTO_DATA_ERROR;

        case TECC_FORMATTER_ERROR_INVALID_NUMERIC:
            snprintf(error_msg, error_msg_size, "Invalid numeric data for type %u", data_type);
            return TECC_CRYPTO_DATA_ERROR;

        case TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW:
            snprintf(error_msg, error_msg_size, "Numeric overflow for type %u", data_type);
            return TECC_CRYPTO_DATA_ERROR;

        /* Format errors (maps to TECC_CRYPTO_FORMAT_ERROR) */
        case TECC_FORMATTER_ERROR_INVALID_HEADER:
            snprintf(error_msg, error_msg_size, "Invalid data header");
            return TECC_CRYPTO_FORMAT_ERROR;

        case TECC_FORMATTER_ERROR_VERSION_MISMATCH:
            snprintf(error_msg, error_msg_size, "Version mismatch in data");
            return TECC_CRYPTO_FORMAT_ERROR;

        case TECC_FORMATTER_ERROR_BUILD_FAILED:
            snprintf(error_msg, error_msg_size, "Data structure build failed");
            return TECC_CRYPTO_FORMAT_ERROR;

        /* ICU collation errors (maps to TECC_CRYPTO_COLLATION_ERROR) */
        case TECC_FORMATTER_ERROR_ICU_NOT_AVAILABLE:
            snprintf(error_msg, error_msg_size, "ICU collation not available");
            return TECC_CRYPTO_COLLATION_ERROR;

        case TECC_FORMATTER_ERROR_COLLATION_INVALID:
            snprintf(error_msg, error_msg_size, "Invalid collation for data type %u", data_type);
            return TECC_CRYPTO_COLLATION_ERROR;

        case TECC_FORMATTER_ERROR_SORTKEY_FAILED:
            snprintf(error_msg, error_msg_size, "Sort key generation failed");
            return TECC_CRYPTO_COLLATION_ERROR;

        /* System errors (maps to TECC_CRYPTO_SYSTEM_ERROR) */
        case TECC_FORMATTER_ERROR_SYSTEM:
            snprintf(error_msg, error_msg_size, "System error during data validation");
            return TECC_CRYPTO_SYSTEM_ERROR;

        /* Success case (should not reach here) */
        case TECC_FORMATTER_SUCCESS:
        default:
            snprintf(error_msg, error_msg_size, "Data validation failed with error code: %d", fmt_result);
            return TECC_CRYPTO_DATA_ERROR;
    }
}

/*
 * Self test
 */
int
tecc_crypto_scheduler_self_test(void)
{
    TeccUnifiedEncryptResult *enc_result;
    TeccUnifiedDecryptResult *dec_result;
    const char *test_plain = "test_encryption_data";
    const char *test_table = "test_table";
    const char *test_column = "test_column";

    TECC_SCHEDULER_DEBUG(
                        "Running crypto scheduler self test");

    /* Test encryption */
    /* Note: Self test uses NULL connection - this is for testing only */
    enc_result = tecc_unified_encrypt_string(NULL, test_plain, test_table, test_column, NULL, NULL, NULL);
    if (!enc_result)
    {
        TECC_SCHEDULER_ERROR(
                           "Self test: encryption failed");
        return -1;
    }

    if (enc_result->error_code != TECC_CRYPTO_SUCCESS)
    {
        TECC_SCHEDULER_ERROR(
                           "Self test: encryption returned error: %s",
                           enc_result->error_msg);
        tecc_unified_free_encrypt_result(enc_result);
        return -1;
    }

    /* Test decryption */
    /* Note: Self test uses NULL connection - this is for testing only */
    dec_result = tecc_unified_decrypt_hex(NULL, enc_result->hex_string, test_table, test_column, NULL);
    if (!dec_result)
    {
        TECC_SCHEDULER_ERROR(
                           "Self test: decryption failed");
        tecc_unified_free_encrypt_result(enc_result);
        return -1;
    }

    if (dec_result->error_code != TECC_CRYPTO_SUCCESS)
    {
        TECC_SCHEDULER_ERROR(
                           "Self test: decryption returned error: %s",
                           dec_result->error_msg);
        tecc_unified_free_encrypt_result(enc_result);
        tecc_unified_free_decrypt_result(dec_result);
        return -1;
    }

    /* Verify round-trip */
    if (strlen(test_plain) != dec_result->plain_len ||
        memcmp(test_plain, dec_result->plaintext, dec_result->plain_len) != 0)
    {
        TECC_SCHEDULER_ERROR(
                           "Self test: round-trip verification failed");
        tecc_unified_free_encrypt_result(enc_result);
        tecc_unified_free_decrypt_result(dec_result);
        return -1;
    }

    TECC_SCHEDULER_DEBUG(
                        "Self test passed successfully");

    tecc_unified_free_encrypt_result(enc_result);
    tecc_unified_free_decrypt_result(dec_result);

    return 0;
}

/* ==================== GUC Parameter Integration ==================== */

/*
 * Set GUC parameters for scheduler
 */
void
tecc_crypto_scheduler_set_guc_params(TeccCryptoScheduler *scheduler,
                                    const TeccGucParams *guc_params)
{
    if (!scheduler)
    {
        TECC_SCHEDULER_ERROR("Cannot set GUC parameters: scheduler is NULL");
        return;
    }

    scheduler->guc_params = guc_params;
    if (guc_params)
    {
        TECC_SCHEDULER_DEBUG("GUC parameters set for scheduler");
    }
    else
    {
        TECC_SCHEDULER_DEBUG("GUC parameters cleared for scheduler");
    }
}

/*
 * Get current GUC parameters
 */
const TeccGucParams *
tecc_crypto_scheduler_get_guc_params(const TeccCryptoScheduler *scheduler)
{
    if (!scheduler)
    {
        TECC_SCHEDULER_ERROR("Cannot get GUC parameters: scheduler is NULL");
        return NULL;
    }

    return scheduler->guc_params;
}

/*
 * Enable/disable GUC processing
 */
void
tecc_crypto_scheduler_set_guc_enabled(TeccCryptoScheduler *scheduler,
                                     bool enabled)
{
    if (!scheduler)
    {
        TECC_SCHEDULER_ERROR("Cannot set GUC enabled: scheduler is NULL");
        return;
    }

    scheduler->guc_enabled = enabled;
    TECC_SCHEDULER_DEBUG("GUC processing %s", enabled ? "enabled" : "disabled");
}

/*
 * Check if GUC processing is enabled
 */
bool
tecc_crypto_scheduler_is_guc_enabled(const TeccCryptoScheduler *scheduler)
{
    if (!scheduler)
    {
        TECC_SCHEDULER_ERROR("Cannot check GUC enabled: scheduler is NULL");
        return false;
    }

    return scheduler->guc_enabled;
}


/*
 * Apply GUC parameters to data before encryption
 */
int
tecc_crypto_scheduler_apply_guc_to_data(TeccCryptoScheduler *scheduler,
                                       TeccCryptoContext *context)
{
    if (!scheduler || !context)
    {
        TECC_SCHEDULER_ERROR("Cannot apply GUC to data: invalid parameters");
        return TECC_CRYPTO_INVALID_PARAM;
    }

    /* Check if GUC processing is enabled */
    if (!scheduler->guc_enabled)
    {
        TECC_SCHEDULER_DEBUG("GUC processing disabled, skipping GUC application");
        return TECC_CRYPTO_SUCCESS;
    }

    /* Check if we have GUC parameters */
    if (!scheduler->guc_params)
    {
        TECC_SCHEDULER_WARNING("No GUC parameters available, skipping GUC application");
        return TECC_CRYPTO_SUCCESS;
    }

    /* Apply GUC parameters based on data type */
    if (context->core.data_type == VARCHAROID ||
        context->core.data_type == BPCHAROID ||
        context->core.data_type == TEXTOID)
    {
        /* Text data - note: client_encoding parameter has been removed */
        TECC_SCHEDULER_DEBUG("Applying GUC to text data");

        /* Note: client_encoding parameter has been removed from TeccGucParams structure */
        /* Character encoding conversion is not part of TECC GUC parameter processing */

        /* Check if collation is specified */
        if (context->core.collation_oid != 0)
        {
            TECC_SCHEDULER_DEBUG("Text data has collation OID %u", context->core.collation_oid);
        }
    }
    else if (context->core.data_type == NUMERICOID ||
             context->core.data_type == INT2OID ||
             context->core.data_type == INT4OID ||
             context->core.data_type == INT8OID ||
             context->core.data_type == FLOAT4OID ||
             context->core.data_type == FLOAT8OID)
    {
        /* Numeric data - note: lc_numeric parameter has been removed */
        TECC_SCHEDULER_DEBUG("Applying GUC to numeric data");

        /* Note: lc_numeric parameter has been removed from TeccGucParams structure */
        /* Numeric locale formatting is not part of TECC GUC parameter processing */
    }
    else
    {
        /* Other data types - minimal GUC processing */
        TECC_SCHEDULER_DEBUG("Applying GUC to data type OID %u", context->core.data_type);
    }

    TECC_SCHEDULER_DEBUG("GUC parameters applied to data successfully");
    return TECC_CRYPTO_SUCCESS;
}

/* ==================== Thread-Safe Functions with Connection Parameter ==================== */

/*
 * ReqID:TECC4.0 - 释放加密列元数据，仅当元数据由本函数自行从缓存构造时。
 *
 * col_meta_in 非空表示元数据由调用方提供（例如
 * tecc_unified_encrypt_string_with_options 传入的栈对象，其字符串字段还
 * 指向缓存私有内存）。这类内存本函数无权释放 —— 释放会 free 栈地址并
 * double free 缓存字段，触发 glibc "free(): invalid pointer" 使进程 abort，
 * 表现为客户端连接被断开。错误路径必须与成功路径采用同一所有权判据。
 */
static void
tecc_free_internal_col_meta(TeccEncryptedColumnMetadata *col_meta,
                            const TeccEncryptedColumnMetadata *col_meta_in)
{
    if (col_meta != NULL && col_meta_in == NULL)
        tecc_free_encrypted_column_metadata(col_meta);
}

/*
 * ReqID:TECC4.0 - 写入侧长度校验（Bug 9327）。
 *
 * 判据本身在 tecc_type_output.c 的 tecc_char_typmod_effective_len()（读取侧
 * 定型用的是同一个函数，保证两侧一致）：按字符计数，超过列声明的 N 个字符时，
 * 只有超出部分全是空格才被服务端接受（服务端会截掉后再存）。
 *
 * 返回 true  —— *effective_len 是应采用的长度，可能小于明文长度（表示超出部分
 *               是空格、应截断后加密，写入侧据此与服务端存同一份值）；
 * 返回 false —— 超出部分含非空格字符，err_msg 带上服务端同款错误文本，调用方
 *               必须拒绝本次写入。
 */
static bool
tecc_check_char_typmod(Oid data_type, int32 type_mod,
                       const char *plaintext, size_t plaintext_len,
                       int encoding, size_t *effective_len,
                       char *err_msg, size_t err_msg_size)
{
    if (tecc_char_typmod_effective_len(data_type, type_mod, plaintext,
                                       plaintext_len, encoding, effective_len))
        return true;

    snprintf(err_msg, err_msg_size, "value too long for type %s(%d)",
             (data_type == BPCHAROID) ? "character" : "character varying",
             (int) (type_mod - (int32) sizeof(int32)));
    return false;
}

/*
 * Internal encryption implementation with explicit scheduler parameter
 */
static TeccUnifiedEncryptResult *
tecc_unified_encrypt_string_internal_with_scheduler(
    TeccCryptoScheduler *scheduler,
    const char *plaintext,
    const char *table_name,
    const char *column_name,
    const char *schema_name,
    const char *kek_name,
    const char *dek_name,
    Oid collation_oid,
    void *tecc_state,
    TeccEncryptedColumnMetadata *col_meta_in,
    TeccCryptoContext *external_context,
    bool supports_he_add,
    bool supports_he_mul)
{
    TeccCryptoContextExtended *context = NULL;
    bool context_is_external = (external_context != NULL);
    TeccUnifiedEncryptResult *result = NULL;
    TeccEncryptedColumnMetadata *col_meta = NULL;
    unsigned char *dek_key = NULL;
    size_t dek_key_len = 0;
    unsigned char *derived_enc_key = NULL;    /* ReqID:TECC4.0 */
    unsigned char *derived_auth_key = NULL;   /* ReqID:TECC4.0 */
    int ret;
    CipherDataType *cipherdata = NULL;
    size_t cipherdata_len = 0;
    TeccFormatterErrorCode fmt_ret;
    TeccCryptoBuffer *cipherdata_buffer;
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    const char *dek_name_to_use;
    /* ReqID:TECC4.0 - 字符型列按声明长度截断后的明文长度（0 = 未声明长度/非字符型） */
    size_t char_typmod_len = 0;

    /* Validate parameters */
    if (!scheduler || !plaintext || !table_name || !column_name)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to unified encrypt with scheduler");
        return NULL;
    }

    TECC_SCHEDULER_DEBUG("Unified encrypt with scheduler: %s.%s (len=%zu), plaintext=%p",
                        table_name, column_name, strlen(plaintext), plaintext);

    /* Debug: Check scheduler and key adapter */
    TECC_SCHEDULER_DEBUG("DEBUG: scheduler=%p, key_adapter=%p, dek_name=%s",
                        scheduler, scheduler ? scheduler->key_adapter : NULL,
                        dek_name ? dek_name : "NULL");

    /* Create or reuse encryption context */
    if (context_is_external)
    {
        context = external_context;
        tecc_crypto_context_reset(context);
        strlcpy(context->table_name, table_name, NAMEDATALEN);
        strlcpy(context->column_name, column_name, NAMEDATALEN);
        if (kek_name)
            strlcpy(context->kek_name, kek_name, NAMEDATALEN);
        if (dek_name)
            strlcpy(context->dek_name, dek_name, NAMEDATALEN);
    }
    else
    {
        context = tecc_create_crypto_context_with_collation(table_name, column_name,
                                                           kek_name, dek_name,
                                                           collation_oid);
        if (!context)
        {
            TECC_SCHEDULER_ERROR("Failed to create crypto context");
            return NULL;
        }
    }

    /* Debug: Check context creation */
    TECC_SCHEDULER_DEBUG("DEBUG: Created context=%p", context);

    /* Set plaintext data in context */
    context->plaintext = (const unsigned char *)plaintext;
    context->plaintext_len = plaintext ? strlen(plaintext) : 0;

    TECC_SCHEDULER_DEBUG("DEBUG: Set plaintext in context: plaintext=%p, len=%zu",
                        context->plaintext, context->plaintext_len);

    /* Get column metadata: use caller-supplied metadata if available,
     * otherwise look up from cache. No fallback — metadata is required. */
    if (col_meta_in != NULL)
    {
        col_meta = col_meta_in;
        TECC_SCHEDULER_DEBUG("DEBUG: Using caller-supplied column metadata: col_meta=%p", col_meta);
    }
    else
    {
        TECC_SCHEDULER_DEBUG("DEBUG: Checking metadata cache: scheduler->metadata_cache=%p", scheduler->metadata_cache);

        /* Try to get metadata from real cache if tecc_state is available */
        if (tecc_state != NULL)
        {
            TeccState *state = (TeccState *)tecc_state;
            TeccColumnCacheItem *cache_item = NULL;

            /* 从连接中获取实际数据库名，替代硬编码的"postgres" */
            {
                const char *db_name = (state->conn != NULL) ? PQdb(state->conn) : NULL;
                if (db_name == NULL || db_name[0] == '\0')
                    db_name = "postgres";
                /* 使用调用方传入的 schema（限定表名如 sch1.tb1），
                 * NULL 时缓存层按 search_path 查找（与解密路径一致） */
                cache_item = tecc_cache_get_encrypted_column_by_name(state, db_name, schema_name, table_name, column_name);
            }
            if (cache_item != NULL)
            {
                col_meta = tecc_convert_column_cache_item_to_metadata(state, cache_item);
                TECC_SCHEDULER_DEBUG("DEBUG: Got column metadata from real cache: col_meta=%p", col_meta);
            }
        }

        if (col_meta == NULL)
        {
            TECC_SCHEDULER_ERROR("Column %s.%s not found in metadata cache and no metadata provided",
                                table_name, column_name);
            if (!context_is_external) pg_free(context);
            return NULL;
        }
    }

    /* Update algorithm support from metadata if available */
    if (col_meta)
    {
        /* Update algorithm support from metadata */
        context->core.algo_support.supports_det = col_meta->enable_det;
        context->core.algo_support.supports_ope = col_meta->enable_ope;
        context->core.algo_support.supports_search = col_meta->enable_search;
        context->core.algo_support.supports_he_add = col_meta->enable_he_add;
        context->core.algo_support.supports_he_mul = col_meta->enable_he_mul;
        context->core.data_type = tecc_map_to_original_type_oid(col_meta->data_type_original_oid);
        context->core.type_mod = col_meta->data_type_original_mod;
        context->core.collation_oid = col_meta->collation_oid;

        /*
         * ReqID:TECC4.0 - 字符型列的声明长度在写入点生效（Bug 9327）。
         *
         * 值还是明文，只有这里能判断长度；服务端拿到的是密文，无从校验。
         * 覆盖全部写入路径（INSERT/UPDATE 字面量、EXECUTE 参数、COPY 等都
         * 经本函数加密）。WHERE 比较走 tecc_unified_encrypt_for_where，不受
         * 影响 —— 与服务端行为一致（超长常量比较返回空集，不报错）。
         *
         * 超出部分是空格时不是拒绝而是截断：服务端也是这么存的，前端必须存
         * 同一份值，否则回读会比服务端多出这些空格（char_typmod_len 记下
         * 截断后的长度，供本函数后面构造 RND 密文与比较核心值时使用）。
         */
        {
            char        len_err[128];
            int         client_encoding = PG_UTF8;
            TeccState  *len_state = (TeccState *) tecc_state;
            size_t      effective_len = 0;

            if (len_state != NULL && len_state->conn != NULL)
                client_encoding = PQclientEncoding(len_state->conn);

            if (!tecc_check_char_typmod(context->core.data_type,
                                        context->core.type_mod,
                                        (const char *) context->plaintext,
                                        context->plaintext_len,
                                        client_encoding, &effective_len,
                                        len_err, sizeof(len_err)))
            {
                TECC_SCHEDULER_ERROR("%s", len_err);
                if (tecc_state != NULL)
                    tecc_record_error((TeccState *) tecc_state, TECC_ERR_ENCRYPT,
                                      len_err, __func__, __LINE__, false);
                tecc_free_internal_col_meta(col_meta, col_meta_in);
                if (!context_is_external) pg_free(context);
                return NULL;
            }

            /* 只有字符型列会给出长度；未声明长度/非字符型保持 0（用原长） */
            if (effective_len > 0)
                char_typmod_len = effective_len;
        }

                if (col_meta->collation_locale != NULL)
            strlcpy(context->core.collation_locale, col_meta->collation_locale, NAMEDATALEN);

        /* Use metadata KEK/DEK if not specified */
        if (!kek_name && col_meta->kek_name != NULL && strlen(col_meta->kek_name) > 0)
            strlcpy(context->kek_name, col_meta->kek_name, NAMEDATALEN);
        if (!dek_name && col_meta->dek_name != NULL && strlen(col_meta->dek_name) > 0)
            strlcpy(context->dek_name, col_meta->dek_name, NAMEDATALEN);

        /*
         * ReqID:TECC4.0 - Prepare RND plaintext.
         *
         * Numeric types (INT2/4/8, FLOAT4/8, NUMERIC):
         *   Normalize once via tecc_formatter_normalize_data to produce the
         *   shared normalized binary format (sign+weight+reserved+digits).
         *   RND, DET, and OPE all encrypt the same normalized binary for
         *   cross-type ciphertext compatibility.
         *
         * Text types (VARCHAR, BPCHAR, TEXT):
         *   Use tecc_formatter_prepare_plaintext to add 4-byte length prefix
         *   for RND. DET/SEARCH normalization is applied separately below.
         */
        {
            bool is_numeric_type = (context->core.data_type == INT2OID ||
                                    context->core.data_type == INT4OID ||
                                    context->core.data_type == INT8OID ||
                                    context->core.data_type == FLOAT4OID ||
                                    context->core.data_type == FLOAT8OID ||
                                    context->core.data_type == NUMERICOID);

            if (is_numeric_type)
            {
                /* ReqID:TECC4.0 - RND normalize (no OPE padding).  DET and
                 * OPE each call normalize_data_for_algorithm independently
                 * in their tecc_execute_algo_* paths. */
                TeccFormatterErrorCode norm_ret;
                size_t req_size;
                TeccCryptoContextCore norm_core;

                memset(&norm_core, 0, sizeof(norm_core));
                norm_core.data_type = context->core.data_type;
                norm_core.type_mod = context->core.type_mod;
                norm_core.collation_oid = context->core.collation_oid;
                norm_core.algo_support = context->core.algo_support;
                norm_core.algo_support.supports_ope = false;
                norm_core.selected_algorithms = context->core.selected_algorithms;
                if (col_meta->collation_locale != NULL)
                    strlcpy(norm_core.collation_locale, col_meta->collation_locale, NAMEDATALEN);
                norm_core.algo_type = TECC_ALGO_DET;

                req_size = tecc_formatter_calculate_normalized_size(&norm_core,
                                                                     strlen(plaintext));
                if (req_size == 0)
                {
                    TECC_SCHEDULER_ERROR("Failed to calculate normalized size");
                    if (tecc_state != NULL)
                    {
                        TeccState *err_state;
                        err_state = (TeccState *)tecc_state;
                        tecc_record_error(err_state, TECC_ERR_ENCRYPT,
                                        "Failed to calculate normalized size",
                                        __func__, __LINE__, false);
                    }
                    tecc_free_internal_col_meta(col_meta, col_meta_in);
                    if (!context_is_external) pg_free(context);
                    return NULL;
                }

                context->normalized_plaintext = pg_malloc(req_size + 1);
                if (context->normalized_plaintext == NULL)
                {
                    TECC_SCHEDULER_ERROR("Failed to allocate normalized buffer");
                    tecc_free_internal_col_meta(col_meta, col_meta_in);
                    if (!context_is_external) pg_free(context);
                    return NULL;
                }

                {
                    TeccCryptoBuffer norm_buf;
                    size_t plaintext_len;
                    plaintext_len = strlen(plaintext);
                    if (plaintext_len > req_size)
                        plaintext_len = req_size;
                    norm_buf.data = context->normalized_plaintext;
                    norm_buf.size = req_size;
                    norm_buf.used = plaintext_len;
                    norm_buf.owned_by_caller = true;
                    memcpy(norm_buf.data, plaintext, plaintext_len);
                    ((unsigned char *)norm_buf.data)[plaintext_len] = '\0';
                    norm_ret = tecc_formatter_normalize_data(&norm_core,
                                                             norm_buf.data,
                                                             norm_buf.used,
                                                             &norm_buf);
                    if (norm_ret != TECC_FORMATTER_SUCCESS)
                    {
                        {
                            TeccState *err_state;
                            const char *err_desc;
                            err_state = (tecc_state != NULL) ? (TeccState *)tecc_state : NULL;
                            switch (norm_ret)
                            {
                                case TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW:
                                    err_desc = "numeric field overflow"; break;
                                case TECC_FORMATTER_ERROR_INVALID_INPUT:
                                    err_desc = "invalid input syntax for type"; break;
                                default:
                                    err_desc = "data normalization error"; break;
                            }
                            TECC_SCHEDULER_ERROR("RND normalize failed: %s (code=%d)", err_desc, (int)norm_ret);
                            if (err_state != NULL)
                                tecc_record_error(err_state, TECC_ERR_ENCRYPT,
                                                err_desc, __func__, __LINE__, false);
                        }
                        pg_free(context->normalized_plaintext);
                        context->normalized_plaintext = NULL;
                        tecc_free_internal_col_meta(col_meta, col_meta_in);
                        if (!context_is_external) pg_free(context);
                        return NULL;
                    }
                    context->normalized_plaintext_len = norm_buf.used;
                }
                /* RND reads from normalized_plaintext; plaintext is
                 * kept as original string for DET/OPE normalize. */
            }
            else
            {
                /* Text types: use prepare_plaintext for RND format */
                TeccFormatterErrorCode prep_ret;
                unsigned char *rnd_binary = NULL;
                size_t rnd_binary_len = 0;

                /* ReqID:TECC4.0 - 列声明长度内的文本才进密文（Bug 9327 写入侧） */
                prep_ret = tecc_formatter_prepare_plaintext(
                    context->core.data_type, context->core.type_mod,
                    plaintext,
                    (char_typmod_len > 0) ? char_typmod_len : strlen(plaintext),
                    &rnd_binary, &rnd_binary_len);

                if (prep_ret != TECC_FORMATTER_SUCCESS)
                {
                    TECC_SCHEDULER_ERROR("Failed to prepare RND plaintext (code=%d)",
                                        (int)prep_ret);
                    /* Record error for propagation to user */
                    if (tecc_state != NULL)
                    {
                        TeccState *err_state;
                        const char *err_desc;
                        err_state = (TeccState *)tecc_state;
                        switch (prep_ret)
                        {
                            case TECC_FORMATTER_ERROR_DATA_TOO_LARGE:
                                err_desc = "input value too large for encryption";
                                break;
                            default:
                                err_desc = "input data format error";
                                break;
                        }
                        tecc_record_error(err_state, TECC_ERR_ENCRYPT,
                                        err_desc, __func__, __LINE__, false);
                    }
                    tecc_free_internal_col_meta(col_meta, col_meta_in);
                    if (!context_is_external) pg_free(context);
                    return NULL;
                }

                context->plaintext = rnd_binary;
                context->plaintext_len = rnd_binary_len;
            }
        }
    }

    /* Allocate result structure */
    result = pg_malloc0(sizeof(TeccUnifiedEncryptResult));
    if (!result)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate encrypt result");
        if (!context_is_external) pg_free(context);
        return NULL;
    }

    /* Initialize result */
    result->error_code = TECC_CRYPTO_SUCCESS;
    result->error_msg[0] = '\0';

    /* Get DEK key from key manager */
    TECC_SCHEDULER_DEBUG("DEBUG: Before getting DEK key: dek_name=%s, context->dek_name=%s",
                        dek_name ? dek_name : "NULL",
                        context->dek_name);

    /* Use context->dek_name if dek_name is NULL */
    /* ReqID:TECC4.0 - dek_name_to_use declared at function scope beginning */

    dek_name_to_use = dek_name ? dek_name : context->dek_name;

    /*
     * ReqID:TECC4.0 - For HE-only columns (he-add / he-mul, no RND/DET/OPE/
     * SEARCH), encryption uses the HE public key, not the SM4-DEK. Skipping
     * the DEK key fetch avoids loading the non-asymmetric KEK plaintext
     * (which would fail). Traditional columns still need the SM4-DEK key.
     */
    if (col_meta != NULL &&
        (col_meta->enable_he_add || col_meta->enable_he_mul) &&
        !col_meta->enable_det && !col_meta->enable_ope && !col_meta->enable_search)
    {
        dek_key = NULL;
        dek_key_len = 0;
        derived_enc_key = NULL;
        derived_auth_key = NULL;
        ret = TECC_CRYPTO_SUCCESS;
    }
    else
        ret = tecc_key_manager_adapter_get_dek_key_ext(scheduler->key_adapter,
                                                  dek_name_to_use,
                                                  &dek_key, &dek_key_len,
                                                  &derived_enc_key, &derived_auth_key);
    if (ret != TECC_CRYPTO_SUCCESS)
    {
        TECC_SCHEDULER_ERROR("Failed to get DEK key for encryption");
        result->error_code = ret;
        /* ReqID:TECC4.0 - Include specific key error message from key layer */
        {
            const char *key_err = tecc_key_get_last_error();
            if (key_err != NULL && key_err[0] != '\0')
            {
                snprintf(result->error_msg, sizeof(result->error_msg),
                         "Cannot encrypt: %s", key_err);
            }
            else
            {
                snprintf(result->error_msg, sizeof(result->error_msg),
                         "Failed to get DEK key '%s'", dek_name_to_use);
            }
        }
        if (!context_is_external) pg_free(context);
        /* ReqID:TECC4.0 - Return result with error instead of NULL,
         * so caller can propagate the specific error message */
        return result;
    }

    /* Apply GUC parameters if enabled */
    if (scheduler->guc_enabled && scheduler->guc_params)
    {
        ret = tecc_crypto_scheduler_apply_guc_to_data(scheduler, (TeccCryptoContext *)context);
        if (ret != TECC_CRYPTO_SUCCESS)
        {
            TECC_SCHEDULER_ERROR("Failed to apply GUC parameters to data");
            result->error_code = ret;
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "Failed to apply GUC parameters");
            tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
            /* Clear dangling pointer after key release */
            context->dek_key = NULL;
            context->dek_key_len = 0;
            if (!context_is_external) pg_free(context);
            pg_free(result);
            return NULL;
        }
    }

    /* Algorithm support already set from metadata above */

    /* ReqID:TECC4.0 - 修复HSM路由：在DEK/KEK加载完成后补填col_meta->kek_name。
     * col_meta在DEK/KEK被缓存之前就已创建，导致kek_name为NULL，
     * 进而tecc_setup_hsm_context无法检测到HSM KEK，使硬加密错误地走了软件加密。 */
    if (col_meta && (col_meta->kek_name == NULL || col_meta->kek_name[0] == '\0') &&
        tecc_state != NULL && dek_name_to_use != NULL)
    {
        TeccDekCacheItem *dek_item = tecc_cache_get_dek_by_name_search_path(
            (const TeccState *)tecc_state, dek_name_to_use);
        if (dek_item != NULL && dek_item->kek_name != NULL && dek_item->kek_name[0] != '\0')
        {
            col_meta->kek_name = pg_strdup(dek_item->kek_name);
            strlcpy(context->kek_name, dek_item->kek_name, NAMEDATALEN);
        }
    }

    /* ReqID:TECC4.0 - Multi-algorithm INSERT encryption.
     * Always encrypt with RND, plus all declared optional algorithms (DET/OPE/SEARCH).
     * For numeric types, normalization is done once and shared across RND/DET/OPE
     * for cross-type ciphertext compatibility. All ciphertexts are packed into
     * a single CipherDataType. */

    /* RND: always required for INSERT */
    context->dek_key = dek_key;
    context->dek_key_len = dek_key_len;

    /* Set up HSM SM4 encryption based on KEK's KMS type.
     * If HSM is configured (kms_type=2/4) but init failed, abort — no software fallback. */
    if (tecc_setup_hsm_context(context, col_meta, tecc_state) < 0)
    {
        TECC_SCHEDULER_ERROR("HSM init failed for KEK, refusing software fallback");
        result->error_code = TECC_CRYPTO_KEY_ERROR;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "HSM hardware required but initialization failed");
        context->dek_key = NULL;
        context->dek_key_len = 0;
        if (!context_is_external) pg_free(context);
        pg_free(result);
        return NULL;
    }

    /* ReqID:TECC4.0 - HE-only columns skip the RND slot (no SM4-DEK). */
    if (col_meta != NULL &&
        (col_meta->enable_he_add || col_meta->enable_he_mul) &&
        !col_meta->enable_det && !col_meta->enable_ope && !col_meta->enable_search)
    {
        ret = TECC_CRYPTO_SUCCESS;
    }
    else
        ret = tecc_execute_algo_rnd((TeccCryptoContextExtended *)context,
                                    derived_enc_key, derived_auth_key);
    if (ret != TECC_CRYPTO_SUCCESS)
    {
        TECC_SCHEDULER_ERROR("RND algorithm encryption failed");
        result->error_code = ret;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "RND algorithm encryption failed");
        context->dek_key = NULL;
        context->dek_key_len = 0;
        if (!context_is_external) pg_free(context);
        pg_free(result);
        return NULL;
    }

    /* ReqID:TECC4.0 - Post-RND cleanup and DET/OPE normalization.
     *
     * For numeric types: normalization was already done before RND.
     *   plaintext == normalized_plaintext (shared), so DO NOT free it here.
     *   normalized_plaintext is already set and valid.
     *
     * For text types: free the RND binary, restore original text pointer,
     *   then normalize for DET/SEARCH as before. */
    {
        bool is_numeric_type = (context->core.data_type == INT2OID ||
                                context->core.data_type == INT4OID ||
                                context->core.data_type == INT8OID ||
                                context->core.data_type == FLOAT4OID ||
                                context->core.data_type == FLOAT8OID ||
                                context->core.data_type == NUMERICOID);

        if (is_numeric_type)
        {
            /* ReqID:TECC4.0 - Restore original raw string so DET/OPE
             * normalize_data_for_algorithm gets the text, not binary. */
            context->plaintext = (const unsigned char *)plaintext;
            context->plaintext_len = strlen(plaintext);
        }
        else
        {
            /* Text types: free RND binary, restore original text */
            if (context->plaintext)
            {
                pg_free((void *)context->plaintext);
                context->plaintext = (const unsigned char *)plaintext;
                /* ReqID:TECC4.0 - 比较核心值也按列声明长度取文本（Bug 9327
                 * 写入侧）：与服务端存储的值一致，超出部分的空格不再参与，
                 * 否则 bpchar 的补齐分支会看到超出声明长度而不补齐。 */
                context->plaintext_len = (char_typmod_len > 0) ?
                    char_typmod_len : strlen(plaintext);
            }

            /* ReqID:TECC4.0 - BPCHAR blank-padding: pad to declared length
             * so trailing spaces are consistently stored before rtrim.
             * PG backend pads bpchar(N) values to N bytes with spaces,
             * and the frontend must match this before encryption. */
            if (context->core.data_type == BPCHAROID && context->core.type_mod >= (int32)sizeof(int32))
            {
                /* PG stores bpchar typmod as (declared_length + 4), e.g. bpchar(20)→24 */
                int32 bpchar_len = context->core.type_mod - 4;
                size_t orig_len = context->plaintext_len;

                if (bpchar_len > 0 && (size_t)bpchar_len > orig_len)
                {
                    const char *orig_text;
                    unsigned char *padded;
                    size_t pad_count;
                    size_t i;

                    orig_text = (const char *)context->plaintext;
                    padded = (unsigned char *)pg_malloc((size_t)bpchar_len + 1);
                    if (padded != NULL)
                    {
                        memcpy(padded, orig_text, orig_len);
                        pad_count = (size_t)bpchar_len - orig_len;
                        for (i = 0; i < pad_count; i++)
                            padded[orig_len + i] = ' ';
                        padded[bpchar_len] = '\0';
                        context->plaintext = padded;
                        context->plaintext_len = (size_t)bpchar_len;
                    }
                }
            }

            /* ReqID:TECC4.0 - Universal rtrim for text types: strip trailing
             * spaces before DET/OPE normalization. The space_count is stored
             * in the context for post-encryption format encoding. This ensures
             * all text types produce the same encrypted_core for semantically
             * identical values (e.g., varchar 'hello' and bpchar 'hello' pad
             * to 20 bytes have the same 'hello' core after rtrim). */
            context->text_space_count = 0;
            if (TECC_IS_TEXT_TYPE(context->core.data_type) &&
                context->plaintext_len > 0)
            {
                const unsigned char *pd = context->plaintext;
                size_t used = context->plaintext_len;

                while (used > 0 && pd[used - 1] == ' ')
                    used--;
                context->text_space_count = (uint32_t)(context->plaintext_len - used);
                {
                }
            }

            /* Normalize for DET/SEARCH algorithms */
            if (context->core.algo_support.supports_det ||
                context->core.algo_support.supports_ope)
            {
                TeccFormatterErrorCode norm_ret;
                TeccCryptoContextCore norm_core;

                memset(&norm_core, 0, sizeof(norm_core));
                norm_core.data_type = context->core.data_type;
                norm_core.type_mod = context->core.type_mod;
                norm_core.collation_oid = context->core.collation_oid;
                norm_core.algo_support = context->core.algo_support;
                norm_core.selected_algorithms = context->core.selected_algorithms;
                if (col_meta->collation_locale != NULL)
                    strlcpy(norm_core.collation_locale, col_meta->collation_locale, NAMEDATALEN);
                norm_core.algo_type = TECC_ALGO_DET;

                {
                    size_t norm_buf_used_val = context->plaintext_len - context->text_space_count;
                    size_t req_size = tecc_formatter_calculate_normalized_size(&norm_core,
                                                                              context->plaintext_len);
                    /* ReqID:TECC4.0 - When norm_buf_used is 0 (pure trailing-space
                     * values rtrimmed to empty), ensure req_size can hold the
                     * canonical 16-byte zero block for empty-string cores.
                     * tecc_normalize_text_to_buffer needs buffer->size >= 16
                     * to write the zero block; a req_size based on the full
                     * plaintext_len (not the rtrim'd length) may be too small. */
                    if (norm_buf_used_val == 0 && req_size < 16)
                        req_size = 16;
                    if (req_size > 0)
                    {
                        context->normalized_plaintext = pg_malloc(req_size);
                        if (context->normalized_plaintext)
                        {
                            TeccCryptoBuffer norm_buf;
                            norm_buf.data = context->normalized_plaintext;
                            norm_buf.size = req_size;
                            /* ReqID:TECC4.0 - Use rtrim'd length from universal rtrim above.
                             * context->plaintext_len is the full (possibly padded) length,
                             * context->text_space_count was computed by the rtrim logic. */
                            norm_buf.used = norm_buf_used_val;
                            norm_buf.owned_by_caller = true;

                            memcpy(norm_buf.data, context->plaintext, norm_buf.used);
                            /* ReqID:TECC4.0 - null-terminate for strtod validation */
                            if (norm_buf.size > norm_buf.used)
                                ((unsigned char *)norm_buf.data)[norm_buf.used] = '\0';
                            norm_ret = tecc_formatter_normalize_data(&norm_core,
                                                                     norm_buf.data,
                                                                     norm_buf.used,
                                                                     &norm_buf);
                            if (norm_ret == TECC_FORMATTER_SUCCESS)
                            {
                                context->normalized_plaintext_len = norm_buf.used;
                                context->data_pre_normalized = true;
                                TECC_SCHEDULER_DEBUG("DET/OPE normalization: input=%zu bytes -> output=%zu bytes",
                                                    strlen(plaintext), norm_buf.used);
                            }
                            else
                            {
                                const char *err_desc;
                                pg_free(context->normalized_plaintext);
                                context->normalized_plaintext = NULL;
                                switch (norm_ret)
                                {
                                    case TECC_FORMATTER_ERROR_NUMERIC_OVERFLOW:
                                        err_desc = "numeric field overflow"; break;
                                    default:
                                        err_desc = "DET/OPE normalization failed"; break;
                                }
                                TECC_SCHEDULER_ERROR("DET/OPE normalize failed: %s (code=%d)",
                                                     err_desc, (int)norm_ret);
                                if (tecc_state != NULL)
                                    tecc_record_error((TeccState *)tecc_state, TECC_ERR_ENCRYPT,
                                                     err_desc, __func__, __LINE__, false);
                                tecc_free_internal_col_meta(col_meta, col_meta_in);
                                if (!context_is_external) pg_free(context);
                                return NULL;
                            }
                        }
                    }
                }
            }
        }
    }

    /* DET: conditional — failure is fatal since DET is required for equality queries */
    if (context->core.algo_support.supports_det)
    {
        ret = tecc_execute_algo_det((TeccCryptoContextExtended *)context);
        if (ret != TECC_CRYPTO_SUCCESS)
        {
            TECC_SCHEDULER_ERROR("DET algorithm encryption failed for column with support_algo='det': %d", ret);
            result->error_code = TECC_CRYPTO_ALGO_ERROR;
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "DET encryption failed");
            tecc_free_internal_col_meta(col_meta, col_meta_in);
            if (!context_is_external) pg_free(context);
            return NULL;
        }
    }

    /* OPE: conditional */
    if (context->core.algo_support.supports_ope)
    {
        ret = tecc_execute_algo_ope((TeccCryptoContextExtended *)context);
        if (ret != TECC_CRYPTO_SUCCESS)
        {
            TECC_SCHEDULER_ERROR("OPE algorithm encryption failed for column with support_algo='ope': %d", ret);
            result->error_code = TECC_CRYPTO_ALGO_ERROR;
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "OPE encryption failed");
            tecc_free_internal_col_meta(col_meta, col_meta_in);
            if (!context_is_external) pg_free(context);
            return NULL;
        }
    }

    /* SEARCH: conditional */
    if (context->core.algo_support.supports_search)
    {
        ret = tecc_execute_algo_search((TeccCryptoContextExtended *)context);
        if (ret != TECC_CRYPTO_SUCCESS)
        {
            TECC_SCHEDULER_ERROR("SEARCH algorithm encryption failed for column with support_algo='search': %d", ret);
            result->error_code = TECC_CRYPTO_ALGO_ERROR;
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "SEARCH encryption failed");
            tecc_free_internal_col_meta(col_meta, col_meta_in);
            if (!context_is_external) pg_free(context);
            return NULL;
        }
    }
    /* ReqID:TECC4.0 - P0-2 一列多 DEK：解析 he-add/he-mul DEK 名。
     * 优先用 hg_encrypted_column_args 绑定的 HE DEK（多 DEK），否则用
     * 列级 DEK（单 DEK，V4.0.0 兼容）。公钥与段内分布式 ID 均按此 DEK 解析。 */
    {
        char he_add_dek_name[NAMEDATALEN];
        char he_mul_dek_name[NAMEDATALEN];

        he_add_dek_name[0] = '\0';
        he_mul_dek_name[0] = '\0';

        if (tecc_state != NULL && col_meta != NULL && OidIsValid(col_meta->table_oid) &&
            col_meta->column_attnum > 0)
        {
            if (context->core.algo_support.supports_he_add)
                tecc_load_column_he_dek_name(tecc_state, col_meta->table_oid,
                                             col_meta->column_attnum, "he_add_dek",
                                             he_add_dek_name, sizeof(he_add_dek_name));
            if (context->core.algo_support.supports_he_mul)
                tecc_load_column_he_dek_name(tecc_state, col_meta->table_oid,
                                             col_meta->column_attnum, "he_mul_dek",
                                             he_mul_dek_name, sizeof(he_mul_dek_name));
        }

        /* ReqID:TECC4.0 - 上一步查不到（列还不存在、没有 hg_encrypted_column_args）
         * 时用调用方给的同态 DEK 名：CTAS / 视图定义里的 cast / 加密列 DEFAULT
         * 由 ENCRYPTED WITH 解析后传入。缺了这一步，det+he-add 这类混合列会拿
         * 对称 DEK 去找 EC 公钥而失败（报 per-DEK EC public key not loaded）。 */
        if (he_add_dek_name[0] == '\0' && col_meta != NULL &&
            col_meta->he_add_dek != NULL && col_meta->he_add_dek[0] != '\0')
            strlcpy(he_add_dek_name, col_meta->he_add_dek, sizeof(he_add_dek_name));
        if (he_mul_dek_name[0] == '\0' && col_meta != NULL &&
            col_meta->he_mul_dek != NULL && col_meta->he_mul_dek[0] != '\0')
            strlcpy(he_mul_dek_name, col_meta->he_mul_dek, sizeof(he_mul_dek_name));

        /* Load HE public keys + resolve per-segment DEK distributed IDs.
         * ReqID:TECC4.0 - 纯 HE 列（对称 DEK 名为空，但解析出 he_add_dek/he_mul_dek）
         * 也应执行 HE 加密，条件由"dek_name 非空"放宽为"任一 HE DEK 名非空"。 */
        if ((context->core.algo_support.supports_he_add ||
             context->core.algo_support.supports_he_mul) &&
            tecc_state != NULL &&
            (context->dek_name[0] != '\0' ||
             he_add_dek_name[0] != '\0' || he_mul_dek_name[0] != '\0'))
        {
            const char *ec_dek_name = (he_add_dek_name[0] != '\0') ?
                he_add_dek_name : context->dek_name;
            const char *eg_dek_name = (he_mul_dek_name[0] != '\0') ?
                he_mul_dek_name : context->dek_name;
            char *he_pub_hex = NULL;
            TeccDekCacheItem *dek_item;

            /* Load EC-ElGamal public key (for he-add) */
            if (context->core.algo_support.supports_he_add &&
                tecc_he_load_dek_key_hex(tecc_state, ec_dek_name,
                                         "he_ec_pubkey", &he_pub_hex) == 0 &&
                he_pub_hex != NULL)
            {
                if (context->he_ec_pubkey_hex)
                    pg_free(context->he_ec_pubkey_hex);
                context->he_ec_pubkey_hex = he_pub_hex;
                TECC_SCHEDULER_DEBUG("Loaded EC pubkey for DEK '%s' (len=%zu)",
                                     ec_dek_name, strlen(he_pub_hex));
            }
            else if (he_pub_hex)
                pg_free(he_pub_hex);

            /* Load FF ElGamal public key (for he-mul) */
            he_pub_hex = NULL;
            if (context->core.algo_support.supports_he_mul &&
                tecc_he_load_dek_key_hex(tecc_state, eg_dek_name,
                                         "he_eg_pubkey", &he_pub_hex) == 0 &&
                he_pub_hex != NULL)
            {
                if (context->he_eg_pubkey_hex)
                    pg_free(context->he_eg_pubkey_hex);
                context->he_eg_pubkey_hex = he_pub_hex;
                TECC_SCHEDULER_DEBUG("Loaded EG pubkey for DEK '%s' (len=%zu)",
                                     eg_dek_name, strlen(he_pub_hex));
            }
            else if (he_pub_hex)
                pg_free(he_pub_hex);

            /* Resolve per-segment DEK distributed IDs */
            dek_item = tecc_cache_get_dek_by_name_search_path(tecc_state, ec_dek_name);
            if (dek_item != NULL && context->core.algo_support.supports_he_add)
                context->he_add_dek_id = dek_item->column_key_distributed_id;

            dek_item = tecc_cache_get_dek_by_name_search_path(tecc_state, eg_dek_name);
            if (dek_item != NULL && context->core.algo_support.supports_he_mul)
                context->he_mul_dek_id = dek_item->column_key_distributed_id;
        }
    }

    /* Begin - ReqID:TECC4.0 - Homomorphic encryption algorithm execution */
    if (context->core.algo_support.supports_he_add)
    {
        ret = tecc_execute_algo_he_add((TeccCryptoContextExtended *)context);
        if (ret != TECC_CRYPTO_SUCCESS)
        {
            TECC_SCHEDULER_ERROR("HE_ADD algorithm encryption failed for column with support_algo=.he-add.: %d", ret);
            result->error_code = TECC_CRYPTO_ALGO_ERROR;
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "HE_ADD encryption failed");
            tecc_free_internal_col_meta(col_meta, col_meta_in);
            if (!context_is_external) pg_free(context);
            return NULL;
        }
    }

    if (context->core.algo_support.supports_he_mul)
    {
        ret = tecc_execute_algo_he_mul((TeccCryptoContextExtended *)context);
        if (ret != TECC_CRYPTO_SUCCESS)
        {
            TECC_SCHEDULER_ERROR("HE_MUL algorithm encryption failed for column with support_algo=.he-mul.: %d", ret);
            result->error_code = TECC_CRYPTO_ALGO_ERROR;
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "HE_MUL encryption failed");
            tecc_free_internal_col_meta(col_meta, col_meta_in);
            if (!context_is_external) pg_free(context);
            return NULL;
        }
    }
    /* End - ReqID:TECC4.0 */


    /* ReqID:TECC4.0 - Build CipherDataType via formatter.
     * tecc_formatter_format_cipherdata correctly packs all 4 algorithm
     * ciphertexts (RND, DET, OPE, SEARCH) with proper length fields. */
    fmt_ret = tecc_formatter_format_cipherdata(context, &cipherdata, &cipherdata_len);
    if (fmt_ret != TECC_FORMATTER_SUCCESS)
    {
        TECC_SCHEDULER_ERROR("Failed to format cipherdata");
        result->error_code = TECC_CRYPTO_MEMORY_ERROR;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "Failed to format cipherdata");
        context->dek_key = NULL;
        context->dek_key_len = 0;
        if (!context_is_external) pg_free(context);
        pg_free(result);
        return NULL;
    }

    /* Set DEK distributed ID from cache if available */
    if (tecc_state != NULL)
    {
        TeccState *state = (TeccState *)tecc_state;
        const char *dek_name_lookup = col_meta ? col_meta->dek_name : NULL;
        if (dek_name_lookup != NULL)
        {
            TeccDekCacheItem *dek_item = tecc_cache_get_dek_by_name_search_path(
                state, dek_name_lookup);
            if (dek_item != NULL && OidIsValid(dek_item->column_key_distributed_id))
                cipherdata->dek_dist_oid = dek_item->column_key_distributed_id;
        }
    }

    /* Build cipherdata_buffer wrapper for ownership tracking */
    cipherdata_buffer = pg_malloc0(sizeof(TeccCryptoBuffer));
    if (cipherdata_buffer)
    {
        cipherdata_buffer->data = cipherdata;
        cipherdata_buffer->size = cipherdata_len;
        cipherdata_buffer->used = cipherdata_len;
    }

    /* Convert to hex string for compatibility */
    fmt_ret = tecc_formatter_hex_encode((const unsigned char *)cipherdata, cipherdata_len, &result->hex_string);
    TECC_SCHEDULER_DEBUG("cipherdata hex dump (first 80 payload bytes)");
        if (fmt_ret != TECC_FORMATTER_SUCCESS)
    {
        TECC_SCHEDULER_ERROR("Failed to convert cipherdata to hex");
        result->error_code = TECC_CRYPTO_FORMAT_ERROR;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "Failed to convert cipherdata to hex");
        tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
        /* Clear dangling pointer after key release */
        context->dek_key = NULL;
        context->dek_key_len = 0;
        pg_free(cipherdata);
        pg_free(cipherdata_buffer);
        if (!context_is_external) pg_free(context);
        pg_free(result);
        return NULL;
    }

    result->hex_len = strlen(result->hex_string);
    result->cipherdata_buffer = cipherdata_buffer;

    /* Update statistics */
    scheduler->encryption_count++;

    /* Cleanup */
    tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
    /* Clear dangling pointer after key release */
    context->dek_key = NULL;
    context->dek_key_len = 0;
    if (!context_is_external) pg_free(context);

    /* Free metadata if it was allocated from real cache (not caller-supplied) */
    tecc_free_internal_col_meta(col_meta, col_meta_in);

    TECC_SCHEDULER_DEBUG("Unified encryption completed successfully");
    return result;
}

/*
 * Unified encryption with preloaded key for batch optimization
 */
static TeccUnifiedEncryptResult *
tecc_unified_encrypt_string_with_preloaded_key(
    TeccCryptoScheduler *scheduler,
    const char *plaintext,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name,
    const unsigned char *preloaded_dek_key,
    size_t preloaded_dek_key_len,
    const unsigned char *preloaded_derived_enc_key,
    const unsigned char *preloaded_derived_auth_key)
{
    TeccCryptoContextExtended *context = NULL;
    TeccUnifiedEncryptResult *result = NULL;
    TeccEncryptedColumnMetadata *col_meta = NULL;
    int ret;
    CipherDataType *cipherdata = NULL;
    size_t cipherdata_len = 0;
    TeccFormatterErrorCode fmt_ret;
    unsigned char *primary_ciphertext;
    size_t primary_cipher_len;
    TeccCryptoBuffer *cipherdata_buffer;
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    const char *dek_name_to_use = NULL;
    /* ReqID:TECC4.0 - local pointers to preloaded or fallback-derived keys */
    const unsigned char *effective_derived_enc_key = preloaded_derived_enc_key;
    const unsigned char *effective_derived_auth_key = preloaded_derived_auth_key;

    /* Validate parameters */
    if (!scheduler || !plaintext || !table_name || !column_name)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to unified encrypt with preloaded key");
        return NULL;
    }

    TECC_SCHEDULER_DEBUG("Unified encrypt with preloaded key: %s.%s (len=%zu, key_len=%zu)",
                        table_name, column_name, strlen(plaintext), preloaded_dek_key_len);

    /* Create encryption context with memory pool */
    context = tecc_create_crypto_context_with_collation(table_name, column_name,
                                                       kek_name, dek_name,
                                                       0); /* No collation */
    if (!context)
    {
        TECC_SCHEDULER_ERROR("Failed to create crypto context");
        return NULL;
    }

    /* Note: This function is used for batch optimization and doesn't have connection parameter */
    /* Without connection, we cannot access real cache, so use default values */
    TECC_SCHEDULER_DEBUG("DEBUG: Using default values for batch optimization for column %s.%s (no connection available)", table_name, column_name);

    /* Set default algorithm support */
    context->core.algo_support.supports_det = true;
    context->core.algo_support.supports_ope = true;
    context->core.algo_support.supports_search = false;
    context->core.data_type = TEXTOID;  /* Default to text type */

    /* Allocate result structure */
    result = pg_malloc0(sizeof(TeccUnifiedEncryptResult));
    if (!result)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate encrypt result");
        pg_free(context);
        return NULL;
    }

    /* Initialize result */
    result->error_code = TECC_CRYPTO_SUCCESS;
    result->error_msg[0] = '\0';

    /* Use preloaded DEK key */
    if (preloaded_dek_key && preloaded_dek_key_len > 0)
    {
        /* Copy preloaded key to context */
        context->dek_key = (unsigned char *)pg_malloc(preloaded_dek_key_len);
        if (!context->dek_key)
        {
            TECC_SCHEDULER_ERROR("Failed to allocate memory for preloaded key copy");
            result->error_code = TECC_CRYPTO_MEMORY_ERROR;
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "Failed to copy preloaded DEK key");
            pg_free(context);
            pg_free(result);
            return NULL;
        }
        memcpy(context->dek_key, preloaded_dek_key, preloaded_dek_key_len);
        context->dek_key_len = preloaded_dek_key_len;
    }
    else
    {
        /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
        unsigned char *dek_key = NULL;
        size_t dek_key_len = 0;
        unsigned char *derived_enc_fb = NULL;
        unsigned char *derived_auth_fb = NULL;

        /* Fall back to regular key loading */
        TECC_SCHEDULER_WARN("No preloaded key available, falling back to regular key loading");

        /* Use context->dek_name if dek_name is NULL */
        dek_name_to_use = dek_name ? dek_name : context->dek_name;
        TECC_SCHEDULER_DEBUG("DEBUG: Using DEK name: %s", dek_name_to_use);

        ret = tecc_key_manager_adapter_get_dek_key_ext(scheduler->key_adapter,
                                                  dek_name_to_use,
                                                  &dek_key, &dek_key_len,
                                                  &derived_enc_fb, &derived_auth_fb);
        if (ret != TECC_CRYPTO_SUCCESS)
        {
            TECC_SCHEDULER_ERROR("Failed to get DEK key for encryption");
            result->error_code = ret;
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "Failed to get DEK key '%s'", dek_name_to_use);
            pg_free(context);
            pg_free(result);
            return NULL;
        }

        context->dek_key = dek_key;
        context->dek_key_len = dek_key_len;
        effective_derived_enc_key = derived_enc_fb;
        effective_derived_auth_key = derived_auth_fb;
    }

    if (scheduler->guc_enabled && scheduler->guc_params)
    {
        ret = tecc_crypto_scheduler_apply_guc_to_data(scheduler, (TeccCryptoContext *)context);
        if (ret != TECC_CRYPTO_SUCCESS)
        {
            TECC_SCHEDULER_ERROR("Failed to apply GUC parameters to data");
            result->error_code = ret;
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "Failed to apply GUC parameters");
            /* Note: Don't release preloaded key - it's managed by caller */
            if (!preloaded_dek_key || preloaded_dek_key_len == 0)
            {
                /* Only release if we loaded it ourselves */
                tecc_key_manager_adapter_release_key(scheduler->key_adapter, context->dek_key, context->dek_key_len);
            }
            /* Clear dangling pointer after key release */
            context->dek_key = NULL;
            context->dek_key_len = 0;
            pg_free(context);
            pg_free(result);
            return NULL;
        }
    }

    /* Determine algorithm support */
    if (col_meta)
    {
        /* Use metadata from cache - already set in algo_support */
        /* No need to set again */
    }
    else
    {
        /* Default to RND only if no metadata */
        context->core.algo_support.supports_det = false;
        context->core.algo_support.supports_ope = false;
        context->core.algo_support.supports_search = false;
    }

    /* Execute encryption based on algorithm support */
    if (context->core.algo_support.supports_search)
    {
        /* SEARCH algorithm - use default operator type */
        ret = tecc_execute_algo_search((TeccCryptoContextExtended *)context);
        if (ret != TECC_CRYPTO_SUCCESS)
        {
            TECC_SCHEDULER_ERROR("SEARCH algorithm execution failed");
            result->error_code = ret;
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "SEARCH algorithm failed");
            goto cleanup;
        }
    }
    else if (context->core.algo_support.supports_ope)
    {
        /* OPE algorithm */
        ret = tecc_execute_algo_ope((TeccCryptoContextExtended *)context);
        if (ret != TECC_CRYPTO_SUCCESS)
        {
            TECC_SCHEDULER_ERROR("OPE algorithm execution failed");
            result->error_code = ret;
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "OPE algorithm failed");
            goto cleanup;
        }
    }
    else if (context->core.algo_support.supports_det)
    {
        /* DET algorithm */
        ret = tecc_execute_algo_det((TeccCryptoContextExtended *)context);
        if (ret != TECC_CRYPTO_SUCCESS)
        {
            TECC_SCHEDULER_ERROR("DET algorithm execution failed");
            result->error_code = ret;
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "DET algorithm failed");
            goto cleanup;
        }
    }
    else
    {
        /* RND algorithm (default) */
        {
            /* HSM setup based on KEK's KMS type via key_adapter.
             * If HSM is configured but init failed, abort — no software fallback. */
            void *hsm_state = (scheduler && scheduler->key_adapter) ?
                              scheduler->key_adapter->tecc_state : NULL;
            if (tecc_setup_hsm_context(context, col_meta, hsm_state) < 0)
            {
                TECC_SCHEDULER_ERROR("HSM init failed for KEK, refusing software fallback");
                result->error_code = TECC_CRYPTO_KEY_ERROR;
                snprintf(result->error_msg, sizeof(result->error_msg),
                         "HSM hardware required but initialization failed");
                ret = TECC_CRYPTO_KEY_ERROR;
                goto cleanup;
            }
        }
        ret = tecc_execute_algo_rnd((TeccCryptoContextExtended *)context,
                                    effective_derived_enc_key,
                                    effective_derived_auth_key);
        if (ret != TECC_CRYPTO_SUCCESS)
        {
            TECC_SCHEDULER_ERROR("RND algorithm execution failed");
            result->error_code = ret;
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "RND algorithm failed");
            goto cleanup;
        }
    }

    /* Get primary ciphertext from context based on algorithm support */
    if (context->core.algo_support.supports_search && context->search_ciphertext)
    {
        primary_ciphertext = context->search_ciphertext;
        primary_cipher_len = context->search_cipher_len;
    }
    else if (context->core.algo_support.supports_ope && context->ope_ciphertext)
    {
        primary_ciphertext = context->ope_ciphertext;
        primary_cipher_len = context->ope_cipher_len;
    }
    else if (context->core.algo_support.supports_det && context->det_ciphertext)
    {
        primary_ciphertext = context->det_ciphertext;
        primary_cipher_len = context->det_cipher_len;
    }
    else if (context->rnd_ciphertext)
    {
        primary_ciphertext = context->rnd_ciphertext;
        primary_cipher_len = context->rnd_cipher_len;
    }
    else
    {
        primary_ciphertext = NULL;
        primary_cipher_len = 0;
    }

    if (!primary_ciphertext || primary_cipher_len == 0)
    {
        TECC_SCHEDULER_ERROR("No primary ciphertext generated");
        result->error_code = TECC_CRYPTO_SYSTEM_ERROR;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "No ciphertext generated");
        goto cleanup;
    }

    /* Format cipherdata */
    cipherdata = (CipherDataType *)context->cipherdata;
    /* Get cipherdata length from varlena header */
    if (cipherdata)
    {
        /* VARSIZE returns the total length of the varlena structure */
        cipherdata_len = VARSIZE(cipherdata);
    }
    else
    {
        cipherdata_len = 0;
    }

    if (!cipherdata || cipherdata_len == 0)
    {
        TECC_SCHEDULER_ERROR("No cipherdata generated");
        result->error_code = TECC_CRYPTO_SYSTEM_ERROR;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "No cipherdata generated");
        goto cleanup;
    }

    /* Convert to hex string */
    fmt_ret = tecc_formatter_hex_encode((const unsigned char *)cipherdata, cipherdata_len,
                                              &result->hex_string);
    if (fmt_ret != TECC_FORMATTER_SUCCESS)
    {
        TECC_SCHEDULER_ERROR("Failed to convert cipherdata to hex");
        result->error_code = TECC_CRYPTO_FORMAT_ERROR;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "Failed to format cipherdata as hex");
        goto cleanup;
    }

    /* Create cipherdata buffer for result */
    cipherdata_buffer = pg_malloc0(sizeof(TeccCryptoBuffer));
    if (!cipherdata_buffer)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate cipherdata buffer");
        result->error_code = TECC_CRYPTO_MEMORY_ERROR;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "Failed to allocate cipherdata buffer");
        goto cleanup;
    }

    cipherdata_buffer->data = (unsigned char *)cipherdata;
    cipherdata_buffer->size = cipherdata_len;
    cipherdata_buffer->used = cipherdata_len;
    cipherdata_buffer->owned_by_caller = false; /* Owned by context */

    /* Transfer ownership of cipherdata from context to result */
    context->cipherdata = NULL;

    result->cipherdata_buffer = cipherdata_buffer;

    /* Update statistics */
    scheduler->encryption_count++;

cleanup:
    /* Note: col_meta is not used in this function anymore, so no cleanup needed */

    /* Clean up context */
    if (context)
    {
        /* Note: Don't release preloaded key - it's managed by caller */
        if (!preloaded_dek_key || preloaded_dek_key_len == 0)
        {
            /* Only release if we loaded it ourselves */
            if (context->dek_key && context->dek_key_len > 0)
            {
                tecc_key_manager_adapter_release_key(scheduler->key_adapter, context->dek_key, context->dek_key_len);
                context->dek_key = NULL;
                context->dek_key_len = 0;
            }
        }
        else
        {
            /* Free the copy of preloaded key */
            if (context->dek_key)
            {
                pg_free(context->dek_key);
                context->dek_key = NULL;
                context->dek_key_len = 0;
            }
        }

        tecc_destroy_crypto_context(context);
    }

    if (result->error_code != TECC_CRYPTO_SUCCESS)
    {
        /* Free any allocated resources on error */
        if (result->hex_string)
        {
            pg_free(result->hex_string);
            result->hex_string = NULL;
        }
        if (result->cipherdata_buffer)
        {
            if (result->cipherdata_buffer->data)
                pg_free(result->cipherdata_buffer->data);
            pg_free(result->cipherdata_buffer);
            result->cipherdata_buffer = NULL;
        }
    }

    if (result->error_code == TECC_CRYPTO_SUCCESS)
    {
        TECC_SCHEDULER_DEBUG("Unified encryption with preloaded key completed successfully");
    }
    else
    {
        TECC_SCHEDULER_ERROR("Unified encryption with preloaded key failed: %s", result->error_msg);
    }

    return result;
}

/*
 * Unified encryption interface with connection parameter (thread-safe version)
 */
TeccUnifiedEncryptResult *
tecc_unified_encrypt_string_with_conn(
    PGconn *conn,
    const char *plaintext,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name)
{
    TeccCryptoScheduler *scheduler;

    /* Validate parameters */
    if (conn == NULL || plaintext == NULL || table_name == NULL || column_name == NULL)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_unified_encrypt_string_with_conn");
        return NULL;
    }

    /* Get scheduler from connection state */
    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to get crypto scheduler from connection");
        return NULL;
    }

    /* Call internal implementation with scheduler */
    return tecc_unified_encrypt_string_internal_with_scheduler(scheduler,
                                                              plaintext,
                                                              table_name,
                                                              column_name,
                                                              NULL, /* schema_name - 未限定，按 search_path 查找 */
                                                              kek_name,
                                                              dek_name,
                                                              0, /* No collation */
                                                              tecc_get_state(conn),
                                                              NULL,
                                                              NULL,
                                                              false,   /* supports_he_add */
                                                              false);  /* supports_he_mul */
}


/* Begin - ReqID:TECC4.0 - Decryption validation functions implementation */

/*
 * tecc_validate_hex_string - Validate hex string format
 *
 * Validates that the hex string contains only valid hex characters
 * (0-9, a-f, A-F) and has an even length.
 *
 * Returns true if valid, false otherwise.
 * If valid_length is not NULL, stores the length of valid hex string.
 */
static bool
tecc_validate_hex_string(const char *hex_string, size_t *valid_length)
{
    size_t i;
    size_t len;

    if (hex_string == NULL)
        return false;

    len = strlen(hex_string);

    /* Hex string must have even length */
    if (len == 0 || (len % 2) != 0)
        return false;

    /* Validate each character */
    for (i = 0; i < len; i++)
    {
        char c = hex_string[i];
        if (!((c >= '0' && c <= '9') ||
              (c >= 'a' && c <= 'f') ||
              (c >= 'A' && c <= 'F')))
        {
            return false;
        }
    }

    if (valid_length != NULL)
        *valid_length = len;

    return true;
}

/*
 * tecc_validate_cipherdata_integrity - Validate cipherdata integrity
 *
 * Validates cipherdata structure integrity including:
 * 1. Minimum size check
 * 2. Version validation
 * 3. Length consistency
 * 4. Data range validation
 *
 * Returns true if valid, false otherwise.
 */
static bool
tecc_validate_cipherdata_integrity(const CipherDataType *cipherdata, size_t cipher_len)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    size_t calculated_len;
    size_t header_len;

    /* ReqID:TECC4.0 - pg_upgrade V4.0.0 兼容：v1.0 头部 28 字节（无 EG 字段），
     * v2.0 头部 36 字节。最小长度先按 v1.0 头部（28）放行，版本判定后再
     * 精确核对总长；v1.0 密文不得读取 EG 长度字段（位于数据段内）。 */

    /* Check minimum size — enough for version field */
    if (cipher_len < 28)
    {
                return false;
    }

    /* Check version */
    if (!CIPHERDATA_VERSION_VALID(cipherdata->version))
    {
                return false;
    }

    /* Check individual data lengths don't exceed total.
     * ReqID:TECC4.0 - v2.0 compact format: only present segments have
     * length fields; absent segments read as 0 via mask-aware macros. */
    header_len = CIPHERDATA_HEADER_SIZE(cipherdata->version);
    if (cipher_len < header_len)
    {
                return false;
    }

    calculated_len = header_len +
                           CIPHERDATA_DATUM_RND_LEN(cipherdata) +
                           CIPHERDATA_DATUM_DET_LEN(cipherdata) +
                           CIPHERDATA_DATUM_OPE_LEN(cipherdata) +
                           CIPHERDATA_DATUM_SEARCH_LEN(cipherdata) +
                           CIPHERDATA_DATUM_EG_ADD_LEN(cipherdata) +
                           CIPHERDATA_DATUM_EG_MUL_LEN(cipherdata);

    if (calculated_len != cipher_len)
    {
                return false;
    }

    /* ReqID:TECC4.0 - Negative lengths caught by cipherdata_validate */
    return true;
}

/*
 * tecc_detect_tampering - Detect cipherdata tampering
 *
 * Attempts to detect if cipherdata has been tampered with.
 * Currently implements basic checks - can be enhanced with HMAC verification.
 *
 * Returns true if tampering detected, false otherwise.
 */
static bool
tecc_detect_tampering(const CipherDataType *cipherdata, size_t cipher_len)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    size_t max_safe_len;

    /* Basic tampering detection:
     * 1. Check for unexpected zeros in critical fields
     * 2. Verify data ranges are reasonable
     * 3. Check for overflow/underflow in calculations
     * 4. Check for suspicious patterns
     */


    /* Check for unexpected zeros in all data length fields */
    if (CIPHERDATA_DATUM_RND_LEN(cipherdata) == 0 &&
        CIPHERDATA_DATUM_DET_LEN(cipherdata) == 0 &&
        CIPHERDATA_DATUM_OPE_LEN(cipherdata) == 0 &&
        CIPHERDATA_DATUM_SEARCH_LEN(cipherdata) == 0 &&
        CIPHERDATA_DATUM_EG_ADD_LEN(cipherdata) == 0 &&
        CIPHERDATA_DATUM_EG_MUL_LEN(cipherdata) == 0)
    {
        TECC_SCHEDULER_WARNING("All ciphertext lengths are zero - suspicious");
        return true; /* All data lengths are zero - suspicious */
    }

    /* Check for overflow in total length calculation */
    max_safe_len = SIZE_MAX - sizeof(CipherDataType);
    if (CIPHERDATA_DATUM_RND_LEN(cipherdata) > max_safe_len ||
        CIPHERDATA_DATUM_DET_LEN(cipherdata) > max_safe_len ||
        CIPHERDATA_DATUM_OPE_LEN(cipherdata) > max_safe_len ||
        CIPHERDATA_DATUM_SEARCH_LEN(cipherdata) > max_safe_len ||
        CIPHERDATA_DATUM_EG_ADD_LEN(cipherdata) > max_safe_len ||
        CIPHERDATA_DATUM_EG_MUL_LEN(cipherdata) > max_safe_len)
    {
        TECC_SCHEDULER_WARNING("Potential overflow detected");
        return true; /* Potential overflow */
    }

    /* Check for suspicious patterns in data lengths */
    /* All lengths being exactly the same might indicate tampering */
    if (CIPHERDATA_DATUM_RND_LEN(cipherdata) > 0 &&
        CIPHERDATA_DATUM_DET_LEN(cipherdata) > 0 &&
        CIPHERDATA_DATUM_RND_LEN(cipherdata) == CIPHERDATA_DATUM_DET_LEN(cipherdata) &&
        CIPHERDATA_DATUM_DET_LEN(cipherdata) == CIPHERDATA_DATUM_OPE_LEN(cipherdata) &&
        CIPHERDATA_DATUM_OPE_LEN(cipherdata) == CIPHERDATA_DATUM_SEARCH_LEN(cipherdata))
    {
        /* This could be legitimate, but worth logging as suspicious */
        TECC_SCHEDULER_WARNING("All ciphertext lengths are identical - possible tampering");
    }

    return false;
}

/* End - ReqID:TECC4.0 - Decryption validation functions implementation */

/*
 * Internal decryption implementation with explicit scheduler parameter
 */
static TeccUnifiedDecryptResult *
tecc_unified_decrypt_hex_internal_with_scheduler(
    PGconn *conn,
    TeccCryptoScheduler *scheduler,
    const char *hex_string,
    const char *table_name,
    const char *column_name,
    TeccCryptoContext *external_context,
    bool supports_he_add,
    bool supports_he_mul)
{
    /* Begin - ReqID:TECC4.0 */
    TeccCryptoContextExtended *context = NULL;
    bool context_is_external = (external_context != NULL);
    TeccUnifiedDecryptResult *result = NULL;
    TeccEncryptedColumnMetadata *col_meta = NULL;
    unsigned char *dek_key = NULL;
    size_t dek_key_len = 0;
    unsigned char *derived_enc_key = NULL;    /* ReqID:TECC4.0 - cached derived */
    unsigned char *derived_auth_key = NULL;   /* ReqID:TECC4.0 - cached derived */
    unsigned char *cipher_data = NULL;
    size_t cipher_len = 0;
    TeccFormatterErrorCode fmt_ret;
    CipherDataType *cipherdata = NULL;
    TeccParsedCipherdata parsed_data;
    int ret;
    unsigned char *plaintext = NULL;
    size_t plain_len = 0;
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    size_t valid_hex_len = 0;
    TeccAlgorithmType algo_type = TECC_ALGO_RND;
    const unsigned char *ciphertext = NULL;
    size_t ciphertext_len = 0;
    /* ReqID:TECC4.0 - Bug 9384: 提升到函数作用域 —— HSM 路由判定（下方
     * tecc_setup_hsm_context 处）需要在取 DEK 的块之外复用该 DEK 名。 */
    const char *dek_name_to_use = NULL;

    /* Validate parameters */
    if (!scheduler || !hex_string || !table_name || !column_name)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to unified decrypt with scheduler");
        return NULL;
    }

    TECC_SCHEDULER_DEBUG("Unified decrypt with scheduler: %s.%s (hex_len=%zu)",
                        table_name, column_name, strlen(hex_string));

    /* Begin - ReqID:TECC4.0 - Enhanced decryption validation */
    /* Validate hex string format */
    valid_hex_len = 0;
    if (!tecc_validate_hex_string(hex_string, &valid_hex_len))
    {
        TECC_SCHEDULER_ERROR("Invalid hex string format for decryption");
        return NULL;
    }

    TECC_SCHEDULER_DEBUG("Hex string validation passed (length=%zu)", valid_hex_len);
    /* End - ReqID:TECC4.0 - Enhanced decryption validation */

    /* Create or reuse decryption context */
    if (context_is_external)
    {
        context = external_context;
        tecc_crypto_context_reset(context);
        strlcpy(context->table_name, table_name, NAMEDATALEN);
        strlcpy(context->column_name, column_name, NAMEDATALEN);
    }
    else
    {
        context = tecc_create_crypto_context_with_pool(table_name, column_name,
                                                      NULL, NULL);
        if (!context)
        {
            TECC_SCHEDULER_ERROR("Failed to create crypto context");
            return NULL;
        }
    }

    /* Get column metadata from cache */
    /* Try to get metadata from real cache if connection is available */
    if (conn != NULL)
    {
        TeccState *state = tecc_get_state(conn);
        TeccColumnCacheItem *cache_item = NULL;

        if (state != NULL)
        {
            /* 从连接中获取实际数据库名，替代硬编码的"postgres" */
            const char *db_name = PQdb(conn);
            if (db_name == NULL || db_name[0] == '\0')
                db_name = "postgres";
            cache_item = tecc_cache_get_encrypted_column_by_name(state, db_name, NULL, table_name, column_name);
            if (cache_item != NULL)
            {
                col_meta = tecc_convert_column_cache_item_to_metadata(state, cache_item);
                TECC_SCHEDULER_DEBUG("DEBUG: Got column metadata from real cache for decryption: col_meta=%p", col_meta);
            }
        }
    }

    /* ReqID:TECC4.0 - Check DEK permission before decryption */
    /* ReqID:TECC4.0 - 放宽条件：无论dek_name是否为空，都尝试权限检查 */
    /* ReqID:TECC4.0 - 纯 HE 列（dek_name 为空，仅 he-add/he-mul 而无对称算法）
     * 没有对称 DEK，USAGE 权限检查无从谈起；解密由 per-DEK HE 私钥（字段级缓存）
     * 承载。此处跳过该门禁，与批量解密
     * tecc_unified_batch_decrypt_hex_internal_with_scheduler 的 HE 语义一致，
     * 否则 COPY 导出等单值解密路径会把纯 HE 密文当无权限→返回十六进制→\N。 */
    if (conn != NULL && col_meta != NULL &&
        !((col_meta->enable_he_add || col_meta->enable_he_mul) &&
          !col_meta->enable_det && !col_meta->enable_ope && !col_meta->enable_search))
    {
        TeccState *decrypt_state;
        bool has_permission;

        decrypt_state = tecc_get_state(conn);
        if (decrypt_state != NULL)
        {
            has_permission = tecc_cache_check_column_dek_permission(
                decrypt_state, NULL, table_name, column_name);
            if (!has_permission)
            {
                /* No DEK permission: skip decrypt, return hex ciphertext */
                result = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
                if (result != NULL)
                {
                    size_t hex_len;
                    hex_len = strlen(hex_string);
                    result->plaintext = (char *)pg_malloc(hex_len + 3);
                    if (result->plaintext != NULL)
                    {
                        result->plaintext[0] = '\\';
                        result->plaintext[1] = 'x';
                        memcpy(result->plaintext + 2, hex_string, hex_len);
                        result->plaintext[hex_len + 2] = '\0';
                        result->plain_len = hex_len + 2;
                    }
                    result->skipped_decrypt = true;
                    result->error_code = TECC_CRYPTO_SUCCESS;
                    result->error_msg[0] = '\0';
                }
                if (!context_is_external) pg_free(context);
                if (col_meta != NULL)
                    tecc_free_encrypted_column_metadata(col_meta);
                return result;
            }
        }
    }

    if (col_meta)
    {
        /* Update algorithm support from metadata */
        context->core.algo_support.supports_det = col_meta->enable_det;
        context->core.algo_support.supports_ope = col_meta->enable_ope;
        context->core.algo_support.supports_search = col_meta->enable_search;
        context->core.algo_support.supports_he_add = col_meta->enable_he_add;
        context->core.algo_support.supports_he_mul = col_meta->enable_he_mul;
        context->core.data_type = tecc_map_to_original_type_oid(col_meta->data_type_original_oid);
        context->core.type_mod = col_meta->data_type_original_mod;
        context->core.collation_oid = col_meta->collation_oid;
        if (col_meta->collation_locale != NULL)
            strlcpy(context->core.collation_locale, col_meta->collation_locale, NAMEDATALEN);
    }

    /* Allocate result structure */
    result = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
    if (!result)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate decrypt result");
        if (!context_is_external) pg_free(context);
        return NULL;
    }

    /* Initialize result */
    result->error_code = TECC_CRYPTO_SUCCESS;
    result->error_msg[0] = '\0';

    /* Debug: print hex string before decode */
    TECC_SCHEDULER_DEBUG("hex string to decode (first 20 chars): %.*s", 20, hex_string);

    /* Convert hex string to binary */
    fmt_ret = tecc_formatter_hex_decode(hex_string, &cipher_data, &cipher_len);
    if (fmt_ret != TECC_FORMATTER_SUCCESS)
    {
        TECC_SCHEDULER_ERROR("Failed to decode hex string");
        result->error_code = TECC_CRYPTO_FORMAT_ERROR;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "Failed to decode hex string");
        if (!context_is_external) pg_free(context);
        pg_free(result);
        return NULL;
    }

    /* Parse cipherdata to determine algorithm */
    cipherdata = (CipherDataType *)cipher_data;

    /* Debug: print full decoded data with field markers */
    if (cipher_len >= 24) {
        int version = *(int32*)(cipher_data+4);
        int rnd_len = *(int32*)(cipher_data+8);
        int det_len = *(int32*)(cipher_data+12);
        int ope_len = *(int32*)(cipher_data+16);
        int srch_len = *(int32*)(cipher_data+20);
        TECC_SCHEDULER_DEBUG("CipherDataType: version=0x%08x rnd_len=%d det_len=%d ope_len=%d srch_len=%d payload=%zu bytes",
                             version, rnd_len, det_len, ope_len, srch_len, cipher_len-24);
    }

    /* Begin - ReqID:TECC4.0 - Enhanced cipherdata validation */
    /* Validate cipherdata integrity */
    if (!tecc_validate_cipherdata_integrity(cipherdata, cipher_len))
    {
        TECC_SCHEDULER_ERROR("Cipherdata integrity validation failed");
        result->error_code = TECC_CRYPTO_FORMAT_ERROR;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "Cipherdata integrity validation failed");
        pg_free(cipher_data);
        if (!context_is_external) pg_free(context);
        pg_free(result);
        return NULL;
    }

    TECC_SCHEDULER_DEBUG("Cipherdata integrity validation passed");
    /* End - ReqID:TECC4.0 - Enhanced cipherdata validation */

    fmt_ret = tecc_formatter_parse_cipherdata(cipherdata, cipher_len, &parsed_data);
    if (fmt_ret != TECC_FORMATTER_SUCCESS)
    {
        TECC_SCHEDULER_ERROR("Failed to parse cipherdata");
        result->error_code = TECC_CRYPTO_FORMAT_ERROR;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "Failed to parse cipherdata");
        pg_free(cipher_data);
        if (!context_is_external) pg_free(context);
        pg_free(result);
        return NULL;
    }

    /* ReqID:TECC4.0 - 纯 HE 值解密（无 RND 槽，密文在 EG_ADD/EG_MUL 段）。
     * 镜像批量解密 tecc_unified_batch_decrypt_hex_internal_with_scheduler 的
     * 串行 HE 回退：先按段内 DEK 分布式 ID 解析 HE DEK（保证一列多 DEK 正确），
     * 再用该 DEK 的 EC/FF 私钥做 ElGamal 解密，得到十进制文本明文。
     * COPY 导出（pg_dump 数据段）等在连接忙态下依赖预热好的字段级缓存取私钥，
     * tecc_he_load_dek_key_hex 命中缓存即不碰主连接。 */
    if (parsed_data.rnd_data_len == 0 &&
        (parsed_data.eg_add_data_len > 0 || parsed_data.eg_mul_data_len > 0))
    {
        TeccState *hst = NULL;
        const char *hek = NULL;
        char *he_plain = NULL;
        size_t he_plain_len = 0;
        int he_ret;

        if (conn != NULL)
            hst = tecc_get_state(conn);

        if (hst != NULL)
        {
            Oid seg_dek;

            seg_dek = (parsed_data.eg_add_dek_id != InvalidOid) ?
                parsed_data.eg_add_dek_id : parsed_data.eg_mul_dek_id;
            if (OidIsValid(seg_dek))
                hek = tecc_cache_get_dek_name_by_distributed_id(hst, seg_dek);
        }
        if (hek == NULL && col_meta != NULL)
        {
            if (parsed_data.eg_add_data_len > 0)
                hek = col_meta->he_add_dek;
            if (hek == NULL && parsed_data.eg_mul_data_len > 0)
                hek = col_meta->he_mul_dek;
            if (hek == NULL)
                hek = col_meta->dek_name;
        }

        he_ret = -1;
        if (hst != NULL && hek != NULL && hek[0] != '\0')
            he_ret = tecc_he_decrypt_cipherdata(parsed_data.eg_add_data,
                                                parsed_data.eg_add_data_len,
                                                parsed_data.eg_mul_data,
                                                parsed_data.eg_mul_data_len,
                                                hek, hst, &he_plain, &he_plain_len);

        if (he_ret == 0 && he_plain != NULL)
        {
            /* 与批量解密一致：HE 明文为十进制文本，skipped_decrypt=true 表示
             * 调用方应直接当作文本使用，勿再做二进制→类型输出。 */
            result->plaintext = he_plain;
            result->plain_len = he_plain_len;
            result->skipped_decrypt = true;
            result->error_code = TECC_CRYPTO_SUCCESS;
            result->error_msg[0] = '\0';
            pg_free(cipher_data);
            if (!context_is_external) pg_free(context);
            if (col_meta != NULL)
                tecc_free_encrypted_column_metadata(col_meta);
            return result;
        }
        if (he_plain != NULL)
            pg_free(he_plain);
        /* DEK 解析/私钥加载失败时回落到下方 no-RND 分支，保留原有报错语义。 */
    }

    /* ReqID:TECC4.0 - SELECT decryption always uses RND ciphertext.
     * DET/OPE/SEARCH ciphertexts are used for backend comparison operators,
     * not for decryption. Only RND can reconstruct the original plaintext. */
    algo_type = TECC_ALGO_RND;
    if (parsed_data.rnd_data_len > 0)
    {
        context->rnd_ciphertext = (unsigned char *)parsed_data.rnd_data;
        context->rnd_cipher_len = parsed_data.rnd_data_len;
    }
    else
    {
        TECC_SCHEDULER_ERROR("No RND ciphertext found in cipherdata for decryption");
        result->error_code = TECC_CRYPTO_FORMAT_ERROR;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "No RND ciphertext found in cipherdata");
        pg_free(cipher_data);
        if (!context_is_external) pg_free(context);
        pg_free(result);
        return NULL;
    }

    /* Get DEK key from key manager */
    /* Use actual DEK name from column metadata if available */
    {
        dek_name_to_use = NULL;
        if (col_meta != NULL && col_meta->dek_name != NULL && col_meta->dek_name[0] != '\0')
        {
            dek_name_to_use = col_meta->dek_name;
        }
        else
        {
            /* ReqID:TECC4.0 - col_meta unavailable (e.g. expression result),
             * try to find DEK by distributed_id from CipherDataType */
            Oid dist_id;
            const char *found_name;

            dist_id = cipherdata->dek_dist_oid;
            if (OidIsValid(dist_id) && conn != NULL)
            {
                TeccState *lookup_state;
                lookup_state = tecc_get_state(conn);
                if (lookup_state != NULL)
                {
                    found_name = tecc_cache_get_dek_name_by_distributed_id(
                        lookup_state, dist_id);
                    if (found_name != NULL)
                        dek_name_to_use = found_name;
                }
            }
        }

        /* ReqID:TECC4.0 - If still no DEK name, user lacks permission.
         * Return hex ciphertext instead of error for SELECT/decrypt path. */
        if (dek_name_to_use == NULL)
        {
            TECC_SCHEDULER_DEBUG("No DEK available for decrypt — returning hex ciphertext");
            result->error_code = TECC_CRYPTO_SUCCESS;
            result->error_msg[0] = '\0';
            {
                size_t hex_len;
                hex_len = strlen(hex_string);
                result->plaintext = (char *)pg_malloc(hex_len + 3);
                if (result->plaintext != NULL)
                {
                    result->plaintext[0] = '\\';
                    result->plaintext[1] = 'x';
                    memcpy(result->plaintext + 2, hex_string, hex_len);
                    result->plaintext[hex_len + 2] = '\0';
                    result->plain_len = hex_len + 2;
                }
                result->skipped_decrypt = true;
            }
            pg_free(cipher_data);
            if (!context_is_external) pg_free(context);
            return result;
        }

        ret = tecc_key_manager_adapter_get_dek_key_ext(scheduler->key_adapter,
                                                  dek_name_to_use,
                                                  &dek_key, &dek_key_len,
                                                  &derived_enc_key,
                                                  &derived_auth_key);
        if (ret != TECC_CRYPTO_SUCCESS)
        {
            /* ReqID:TECC4.0 - DEK不可用（无权限或缓存未命中）：
             * 返回密文替代报错。覆盖权限检查被绕过（如tecc_get_state返回NULL）
             * 但DEK名称仍被解析出的边界情况。 */
            TECC_SCHEDULER_DEBUG("DEK key retrieval failed (%d) — returning hex ciphertext", ret);
            result->error_code = TECC_CRYPTO_SUCCESS;
            result->error_msg[0] = '\0';
            {
                size_t hex_len;
                hex_len = strlen(hex_string);
                result->plaintext = (char *)pg_malloc(hex_len + 3);
                if (result->plaintext != NULL)
                {
                    result->plaintext[0] = '\\';
                    result->plaintext[1] = 'x';
                    memcpy(result->plaintext + 2, hex_string, hex_len);
                    result->plaintext[hex_len + 2] = '\0';
                    result->plain_len = hex_len + 2;
                }
                result->skipped_decrypt = true;
            }
            pg_free(cipher_data);
            if (!context_is_external) pg_free(context);
            return result;
        }
    }

    /* Set key in context - note: key will be released by tecc_key_manager_adapter_release_key */
    context->dek_key = dek_key;
    context->dek_key_len = dek_key_len;

    /* Set up HSM SM4 decryption if column uses HSM KEK.
     * If HSM is configured but init failed, abort — no software fallback.
     *
     * ReqID:TECC4.0 - Bug 9384: 与批量解密路径同源修复——无列元数据（纯加密常量
     * 表达式）时用密文内解析出的 DEK 名合成最小元数据判定 HSM 路由，否则硬加密的
     * 裸 [IV][SM4-CBC] 密文会被软件解密按软件包装格式解析而失败。 */
    {
        TeccState *hsm_state = (conn != NULL) ? tecc_get_state(conn) : NULL;
        TeccEncryptedColumnMetadata *hsm_meta = col_meta;

        if (hsm_meta == NULL)
            hsm_meta = tecc_hsm_meta_synth_from_dek(dek_name_to_use);

        if (hsm_state != NULL && hsm_meta != NULL)
        {
            if (tecc_setup_hsm_context(context, hsm_meta, hsm_state) < 0)
            {
                TECC_SCHEDULER_ERROR("HSM init failed for KEK, refusing software fallback");
                result->error_code = TECC_CRYPTO_KEY_ERROR;
                snprintf(result->error_msg, sizeof(result->error_msg),
                         "HSM hardware required but initialization failed");
                if (hsm_meta != col_meta)
                    tecc_free_encrypted_column_metadata(hsm_meta);
                tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
                pg_free(cipher_data);
                if (!context_is_external) pg_free(context);
                pg_free(result);
                return NULL;
            }
        }

        if (hsm_meta != NULL && hsm_meta != col_meta)
            tecc_free_encrypted_column_metadata(hsm_meta);
    }

    /* Begin - ReqID:TECC4.0 - Tampering detection */
    /* Check for potential tampering in cipherdata */
    if (tecc_detect_tampering(cipherdata, cipher_len))
    {
        TECC_SCHEDULER_ERROR("Potential tampering detected in cipherdata");
        result->error_code = TECC_CRYPTO_DATA_ERROR;
        snprintf(result->error_msg, sizeof(result->error_msg),
                 "Potential tampering detected in cipherdata");
        tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
        pg_free(cipher_data);
        if (!context_is_external) pg_free(context);
        pg_free(result);
        return NULL;
    }

    TECC_SCHEDULER_DEBUG("Tampering detection passed");
    /* End - ReqID:TECC4.0 - Tampering detection */

    /* Execute decryption using zero-copy algorithm interface */
    {
        size_t buffer_size;

        /*
         * For HSM-decrypted RND data, the stored format is
         * [16-byte IV][SM4-CBC ciphertext]. The software-format
         * function returns 0 for this layout (expects a larger
         * software-specific header), so we calculate the buffer
         * size directly from ciphertext length.
         */
        if (algo_type == TECC_ALGO_RND && context->hsm_kms_type != 0)
            buffer_size = context->rnd_cipher_len;
        else
            buffer_size = tecc_algo_calc_decrypt_buffer_size(algo_type,
                                                           (algo_type == TECC_ALGO_SEARCH) ? context->search_cipher_len :
                                                           (algo_type == TECC_ALGO_OPE) ? context->ope_cipher_len :
                                                           (algo_type == TECC_ALGO_DET) ? context->det_cipher_len :
                                                           context->rnd_cipher_len);

        plaintext = pg_malloc(buffer_size);
        if (!plaintext)
        {
            TECC_SCHEDULER_ERROR("Failed to allocate plaintext buffer");
            result->error_code = TECC_CRYPTO_MEMORY_ERROR;
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "Failed to allocate plaintext buffer");
            tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
            pg_free(cipher_data);
            if (!context_is_external) pg_free(context);
            pg_free(result);
            return NULL;
        }

        ciphertext = NULL;
        ciphertext_len = 0;

        if (algo_type == TECC_ALGO_SEARCH)
        {
            ciphertext = context->search_ciphertext;
            ciphertext_len = context->search_cipher_len;
        }
        else if (algo_type == TECC_ALGO_OPE)
        {
            ciphertext = context->ope_ciphertext;
            ciphertext_len = context->ope_cipher_len;
        }
        else if (algo_type == TECC_ALGO_DET)
        {
            ciphertext = context->det_ciphertext;
            ciphertext_len = context->det_cipher_len;
        }
        else /* TECC_ALGO_RND */
        {
            ciphertext = context->rnd_ciphertext;
            ciphertext_len = context->rnd_cipher_len;
        }

        /* Try HSM SM4 decryption (for RND with HSM-enabled columns).
         * If HSM is configured, it MUST be used — no software fallback. */
        if (algo_type == TECC_ALGO_RND &&
            context->hsm_adapter != NULL && context->hsm_device != NULL)
        {
            HsmAdapterInterface *hsm = (HsmAdapterInterface *)context->hsm_adapter;

            if (hsm->sm4_decrypt == NULL)
            {
                TECC_SCHEDULER_ERROR("HSM adapter configured but sm4_decrypt is NULL");
                result->error_code = TECC_CRYPTO_KEY_ERROR;
                snprintf(result->error_msg, sizeof(result->error_msg),
                         "HSM adapter configured but sm4_decrypt is NULL");
                tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
                memset(plaintext, 0, buffer_size);
                pg_free(plaintext);
                pg_free(cipher_data);
                if (!context_is_external) pg_free(context);
                pg_free(result);
                return NULL;
            }

            if (ciphertext_len > 16)
            {
                unsigned char sm4_key[TECC_SM4_KEY_SIZE];
                size_t actual_key_len = (dek_key_len >= TECC_SM4_KEY_SIZE) ?
                                        TECC_SM4_KEY_SIZE : dek_key_len;
                HsmErrorCode hsm_ret;
                size_t hsm_plain_len = buffer_size;

                memcpy(sm4_key, dek_key, actual_key_len);
                TECC_LOG(TECC_LOG_DEBUG1, "TECC_ROUTE: executing RND decrypt via HSM SM4 (HARDWARE)");
                hsm_ret = hsm->sm4_decrypt(
                    (HsmDevice *)context->hsm_device,
                    sm4_key, actual_key_len,
                    ciphertext,           /* IV = first 16 bytes */
                    ciphertext + 16,      /* ciphertext after IV */
                    ciphertext_len - 16,
                    plaintext, &hsm_plain_len);
                memset(sm4_key, 0, sizeof(sm4_key));

                if (hsm_ret == HSM_SUCCESS)
                {
                    /* Strip PKCS#7 padding from HSM decrypt result.
                     * SWXA adapter already strips padding internally, so skip
                     * the safety net for SWXA to avoid double-stripping. */
                    if (context->hsm_kms_type != 4 && hsm_plain_len > 0)
                    {
                        unsigned char pkcs7_pad = plaintext[hsm_plain_len - 1];
                        if (pkcs7_pad > 0 && pkcs7_pad <= 16 &&
                            hsm_plain_len >= pkcs7_pad)
                        {
                            bool pad_valid = true;
                            size_t p;
                            for (p = hsm_plain_len - pkcs7_pad; p < hsm_plain_len; p++)
                            {
                                if ((unsigned char)plaintext[p] != pkcs7_pad)
                                {
                                    pad_valid = false;
                                    break;
                                }
                            }
                            if (pad_valid)
                                hsm_plain_len -= pkcs7_pad;
                        }
                    }
                    plain_len = hsm_plain_len;
                    ret = TECC_ALGO_SUCCESS;
                }
                else
                {
                    TECC_SCHEDULER_ERROR("HSM SM4 decrypt failed: %d", (int)hsm_ret);
                    result->error_code = TECC_CRYPTO_KEY_ERROR;
                    snprintf(result->error_msg, sizeof(result->error_msg),
                             "HSM SM4 decrypt failed: %d", (int)hsm_ret);
                    tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
                    memset(plaintext, 0, buffer_size);
                    pg_free(plaintext);
                    pg_free(cipher_data);
                    if (!context_is_external) pg_free(context);
                    pg_free(result);
                    return NULL;
                }
            }
            else
            {
                TECC_SCHEDULER_ERROR("Ciphertext too short for HSM SM4 decrypt: %zu bytes",
                                     ciphertext_len);
                result->error_code = TECC_CRYPTO_DATA_ERROR;
                snprintf(result->error_msg, sizeof(result->error_msg),
                         "Ciphertext too short for HSM SM4 decrypt: %zu bytes",
                         ciphertext_len);
                tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
                memset(plaintext, 0, buffer_size);
                pg_free(plaintext);
                pg_free(cipher_data);
                if (!context_is_external) pg_free(context);
                pg_free(result);
                return NULL;
            }
        }
        else
        {
            /* No HSM configured, use software SM4 */
            ret = TECC_ALGO_OPERATION_NOT_SUPPORTED;
        }

        /* Software SM4 (when no HSM configured) */
        if (ret != TECC_ALGO_SUCCESS)
        {
            TECC_LOG(TECC_LOG_DEBUG1, "TECC_ROUTE: executing RND decrypt via SOFTWARE SM4");
            if (algo_type == TECC_ALGO_RND && derived_enc_key != NULL && derived_auth_key != NULL)
            {
                ret = tecc_sm4_rnd_decrypt_with_keys(dek_key, TECC_SM4_KEY_SIZE,
                                             ciphertext, ciphertext_len,
                                             plaintext, buffer_size,
                                             &plain_len,
                                             derived_enc_key, derived_auth_key);
            }
            else
            {
                ret = tecc_algo_decrypt_zero_copy(algo_type,
                                             dek_key, dek_key_len,
                                             ciphertext, ciphertext_len,
                                             plaintext, buffer_size,
                                             &plain_len);
            }
        }
        if (ret != TECC_CRYPTO_SUCCESS)
        {
            TECC_SCHEDULER_ERROR("%s algorithm decryption failed",
                               tecc_get_algorithm_name(algo_type));
            result->error_code = ret;
            snprintf(result->error_msg, sizeof(result->error_msg),
                     "%s algorithm decryption failed",
                     tecc_get_algorithm_name(algo_type));
            tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
            /* Securely wipe plaintext data before freeing */
            if (plaintext)
                memset(plaintext, 0, buffer_size);
            pg_free(plaintext);
            pg_free(cipher_data);
            if (!context_is_external) pg_free(context);
            pg_free(result);
            return NULL;
        }
    }

    result->plaintext = (char *)plaintext;
    result->plain_len = plain_len;

    /* Update statistics */
    scheduler->decryption_count++;

    /* Cleanup */
    tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
    /* Clear dangling pointer after key release */
    context->dek_key = NULL;
    context->dek_key_len = 0;
    pg_free(cipher_data);
    if (context_is_external)
    {
        /* External context: only clear per-use pointers, keep buffers */
        context->rnd_ciphertext = NULL;
        context->det_ciphertext = NULL;
        context->ope_ciphertext = NULL;
        context->search_ciphertext = NULL;
    }
    else
    {
        if (!context_is_external) pg_free(context);
    }

    /* Free metadata if it was allocated from real cache */
    if (conn != NULL && col_meta != NULL)
    {
        tecc_free_encrypted_column_metadata(col_meta);
    }
    /* Note: If conn is NULL, col_meta should also be NULL since we don't use mock data anymore */

    TECC_SCHEDULER_DEBUG("Unified decryption completed successfully (plain_len=%zu)",
                        result->plain_len);
    return result;
    /* End - ReqID:TECC4.0 */
}

/* Begin - ReqID:TECC4.0 */
/*
 * Batch hex decrypt — decrypts multiple hex strings for the same column
 * at once. Shares one SM4 context (no per-row key expansion) and one
 * DEK lookup across all items. localkms RND path only; HSM deferred.
 *
 * Returns an array of count result pointers, or NULL on fatal error.
 * On item-level failure (e.g. one corrupt row), that item's result has
 * error_code set but the rest succeed.
 * Caller frees each result[i] via tecc_unified_free_decrypt_result().
 */
/*
 * ReqID:TECC4.0 - 硬加密列批量解密（拼接流）
 *
 * 背景：硬加密列的 RND 段就是 [16B IV][SM4-CBC 密文]，逐值调用加密机时每值一次
 * TCP 往返（真机实测约 1.14ms/值），全表返回 1 万行要 18 秒；perf 采样显示 97% 的
 * 时间在 libswsds 的 SDF_Decrypt/SocketCommunication 里，即纯往返延迟。
 *
 * 手法：CBC 解密每块只依赖自己与前一块，故把每个值自己的 IV 当作前置块插进流里
 * （IV1||CT1||IV2||CT2||...），整段用**一次** SDF_Decrypt 即可让各值独立解对：
 * 值 i 的明文就是它 CT 长度对应的那几块，IV 块的输出是垃圾、直接丢弃。
 * 真机探针（1KB~16MB 各档）验证：任意长度一次调用都正确还原，吞吐约 5~10MB/s；
 * 4096 个值从 4656ms 降到 25ms。
 *
 * 与逐值路径的差异：设备不做 PKCS#7（libpq 适配器自己补/剥），所以拼接后中间各值
 * 自己的 padding 会留在输出里，必须按值剥；SWXA（kms_type=4）的适配器会顺带剥掉
 * 整段末尾的 padding，故末尾值不再重复剥。
 *
 * 开关：TECC_HSM_BATCH=0 关闭拼接（退回逐值，排障/对照用）；
 *       TECC_HSM_BATCH_BYTES 指定单次调用字节数（默认 256KB）。
 */
static bool
tecc_hsm_batch_decrypt_enabled(void)
{
    const char *v = getenv("TECC_HSM_BATCH");

    if (v == NULL || *v == '\0')
        return true;
    return !(v[0] == '0' && v[1] == '\0');
}

static size_t
tecc_hsm_batch_call_limit(void)
{
    const char *v = getenv("TECC_HSM_BATCH_BYTES");
    long n;

    if (v != NULL && *v != '\0')
    {
        n = strtol(v, NULL, 10);
        if (n >= 4096 && n <= (long) (64 * 1024 * 1024))
            return (size_t) n;
    }
    return (size_t) (256 * 1024);
}

/* 剥掉一个值的 PKCS#7 padding；校验不通过则保持原长度（与逐值路径行为一致） */
static size_t
tecc_hsm_strip_pkcs7(unsigned char *buf, size_t len)
{
    unsigned char pad;
    size_t p;

    if (len == 0)
        return 0;
    pad = buf[len - 1];
    if (pad == 0 || pad > 16 || (size_t) pad > len)
        return len;
    for (p = len - pad; p < len; p++)
    {
        if (buf[p] != pad)
            return len;
    }
    return len - pad;
}

/*
 * 单个硬加密值的解密（逐值兜底路径，语义与改动前完全一致）。
 * rnd_data 为 [16B IV][SM4-CBC 密文]，长度 rnd_len。
 */
static void
tecc_hsm_decrypt_item(HsmAdapterInterface *hsm, HsmDevice *device,
                      const unsigned char *key, size_t key_len,
                      int hsm_kms_type,
                      const unsigned char *rnd_data, size_t rnd_len,
                      size_t item_index,
                      TeccUnifiedDecryptResult **slot)
{
    unsigned char sm4_key[TECC_SM4_KEY_SIZE];
    size_t actual_key_len;
    size_t iv_len = 16;
    unsigned char *ct_buf;
    size_t plain_len;
    HsmErrorCode hsm_ret;

    if (hsm == NULL || hsm->sm4_decrypt == NULL || device == NULL)
    {
        *slot = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
        if (*slot)
        {
            (*slot)->error_code = TECC_CRYPTO_KEY_ERROR;
            snprintf((*slot)->error_msg, sizeof((*slot)->error_msg),
                     "Batch decrypt: item %zu HSM not available", item_index);
        }
        return;
    }

    if (rnd_data == NULL || rnd_len <= iv_len)
    {
        *slot = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
        if (*slot)
        {
            (*slot)->error_code = TECC_CRYPTO_DATA_ERROR;
            snprintf((*slot)->error_msg, sizeof((*slot)->error_msg),
                     "Batch decrypt: item %zu HSM data too short", item_index);
        }
        return;
    }

    plain_len = rnd_len - iv_len;
    ct_buf = (unsigned char *)pg_malloc(plain_len);
    if (!ct_buf)
    {
        *slot = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
        if (*slot)
        {
            (*slot)->error_code = TECC_CRYPTO_MEMORY_ERROR;
            snprintf((*slot)->error_msg, sizeof((*slot)->error_msg),
                     "Batch decrypt: item %zu OOM", item_index);
        }
        return;
    }

    actual_key_len = (key_len >= TECC_SM4_KEY_SIZE) ? TECC_SM4_KEY_SIZE : key_len;
    memcpy(sm4_key, key, actual_key_len);

    hsm_ret = hsm->sm4_decrypt(device, sm4_key, actual_key_len,
                               rnd_data,                    /* IV = first 16 bytes */
                               rnd_data + iv_len,           /* ciphertext after IV */
                               rnd_len - iv_len,
                               ct_buf, &plain_len);
    memset(sm4_key, 0, sizeof(sm4_key));

    if (hsm_ret == HSM_SUCCESS)
    {
        /* Strip PKCS#7 padding (except SWXA which does it internally) */
        if (hsm_kms_type != 4)
            plain_len = tecc_hsm_strip_pkcs7(ct_buf, plain_len);

        *slot = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
        if (*slot)
        {
            (*slot)->plaintext = (char *)ct_buf;
            (*slot)->plain_len = plain_len;
            (*slot)->error_code = TECC_CRYPTO_SUCCESS;
        }
        else
        {
            pg_free(ct_buf);
        }
    }
    else
    {
        pg_free(ct_buf);
        *slot = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
        if (*slot)
        {
            (*slot)->error_code = TECC_CRYPTO_DATA_ERROR;
            snprintf((*slot)->error_msg, sizeof((*slot)->error_msg),
                     "Batch decrypt: item %zu HSM decrypt failed", item_index);
        }
    }
}

/*
 * 把拼接流一次交给加密机解密，再按值切分回填 results。
 * offsets[k] 为值在流中的起始偏移（即它的 IV 位置），seg_lens[k] 为该值
 * [IV][CT] 的总长，pending_idx[k] 为它在 results 中的下标。
 * 返回 false 表示整批失败，调用方须退回逐值路径。
 */
static bool
tecc_hsm_decrypt_stream(HsmAdapterInterface *hsm, HsmDevice *device,
                        const unsigned char *key, size_t key_len,
                        int hsm_kms_type,
                        const unsigned char *stream, size_t stream_len,
                        const size_t *offsets, const size_t *seg_lens,
                        const size_t *pending_idx, size_t npending,
                        TeccUnifiedDecryptResult **results)
{
    unsigned char sm4_key[TECC_SM4_KEY_SIZE];
    size_t actual_key_len;
    unsigned char dummy_iv[16];
    unsigned char *out;
    size_t out_len;
    size_t k;
    HsmErrorCode hsm_ret;

    if (hsm == NULL || hsm->sm4_decrypt == NULL || device == NULL || npending == 0)
        return false;

    out = (unsigned char *)pg_malloc(stream_len);
    if (out == NULL)
        return false;

    actual_key_len = (key_len >= TECC_SM4_KEY_SIZE) ? TECC_SM4_KEY_SIZE : key_len;
    memcpy(sm4_key, key, actual_key_len);
    /* 外层 IV 只影响流首块（该块是被丢弃的 IV 块），取值无关 */
    memset(dummy_iv, 0, sizeof(dummy_iv));

    out_len = stream_len;
    hsm_ret = hsm->sm4_decrypt(device, sm4_key, actual_key_len, dummy_iv,
                               stream, stream_len, out, &out_len);
    memset(sm4_key, 0, sizeof(sm4_key));

    if (hsm_ret != HSM_SUCCESS)
    {
        pg_free(out);
        return false;
    }

    /* ReqID:TECC4.0 - TECC_HSM_BATCH_DEBUG=1 时打印每个冲刷块的字节数/值个数，
     * 用于估算设备流量（每值字节数）与调块大小。 */
    {
        const char *dbg = getenv("TECC_HSM_BATCH_DEBUG");

        if (dbg != NULL && dbg[0] == '1')
            fprintf(stderr, "[tecc] HSM 拼接块: %zu 字节 / %zu 个值 (每值 %.1f 字节)\n",
                    stream_len, npending,
                    npending > 0 ? (double) stream_len / (double) npending : 0.0);
    }

    for (k = 0; k < npending; k++)
    {
        size_t seg_off = offsets[k] + 16;
        size_t seg_len;
        size_t plain_len;
        unsigned char *buf;

        if (k == npending - 1)
        {
            /* 末尾值：SWXA 适配器已剥掉整段末尾的 padding，长度以返回值为准 */
            if (out_len <= seg_off)
            {
                pg_free(out);
                return false;
            }
            seg_len = out_len - seg_off;
        }
        else
        {
            if (seg_lens[k] <= 16)
            {
                pg_free(out);
                return false;
            }
            seg_len = seg_lens[k] - 16;
        }

        buf = (unsigned char *)pg_malloc(seg_len + 1);
        if (buf == NULL)
        {
            pg_free(out);
            return false;
        }
        memcpy(buf, out + seg_off, seg_len);
        buf[seg_len] = '\0';

        plain_len = seg_len;
        if (hsm_kms_type != 4 || k != npending - 1)
            plain_len = tecc_hsm_strip_pkcs7(buf, seg_len);

        results[pending_idx[k]] = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
        if (results[pending_idx[k]] == NULL)
        {
            pg_free(buf);
            pg_free(out);
            return false;
        }
        results[pending_idx[k]]->plaintext = (char *)buf;
        results[pending_idx[k]]->plain_len = plain_len;
        results[pending_idx[k]]->error_code = TECC_CRYPTO_SUCCESS;
    }

    pg_free(out);
    return true;
}

static TeccUnifiedDecryptResult **
tecc_unified_batch_decrypt_hex_internal_with_scheduler(
    PGconn *conn,
    TeccCryptoScheduler *scheduler,
    const char **hex_strings,
    size_t count,
    const char *schema_name,
    const char *table_name,
    const char *column_name,
    TeccCryptoContext *external_context)
{
    TeccUnifiedDecryptResult **results;
    TeccCryptoContextExtended *context;
    bool context_is_external;
    TeccEncryptedColumnMetadata *col_meta;
    unsigned char *dek_key;
    size_t dek_key_len;
    unsigned char *derived_enc_key;
    unsigned char *derived_auth_key;
    const char *dek_name_to_use;
    unsigned char *batch_binary_base;
    unsigned char **binary_data;
    size_t *binary_lens;
    const unsigned char **rnd_ciphertexts;
    size_t *rnd_ciphertext_lens;
    TeccFormatterErrorCode fmt_ret;
    bool all_skipped;
    int ret;
    size_t i;

    /* Validate parameters */
    if (!scheduler || !hex_strings || count == 0 || !table_name || !column_name)
        return NULL;

    /* Allocate results array */
    results = pg_malloc0(sizeof(TeccUnifiedDecryptResult *) * count);
    if (!results)
        return NULL;

    /* Allocate per-item working arrays */
    binary_data = pg_malloc(sizeof(unsigned char *) * count);
    binary_lens = pg_malloc(sizeof(size_t) * count);
    rnd_ciphertexts = pg_malloc(sizeof(const unsigned char *) * count);
    rnd_ciphertext_lens = pg_malloc(sizeof(size_t) * count);
    if (!binary_data || !binary_lens || !rnd_ciphertexts || !rnd_ciphertext_lens)
    {
        pg_free(results);
        if (binary_data) pg_free(binary_data);
        if (binary_lens) pg_free(binary_lens);
        if (rnd_ciphertexts) pg_free(rnd_ciphertexts);
        if (rnd_ciphertext_lens) pg_free(rnd_ciphertext_lens);
        return NULL;
    }

    /* Create or reuse context */
    context_is_external = (external_context != NULL);
    if (context_is_external)
    {
        context = external_context;
        tecc_crypto_context_reset(context);
        strlcpy(context->table_name, table_name, NAMEDATALEN);
        strlcpy(context->column_name, column_name, NAMEDATALEN);
    }
    else
    {
        context = tecc_create_crypto_context_with_pool(table_name, column_name,
                                                       NULL, NULL);
        if (!context)
        {
            pg_free(results);
            pg_free(binary_data);
            pg_free(binary_lens);
            pg_free(rnd_ciphertexts);
            pg_free(rnd_ciphertext_lens);
            return NULL;
        }
    }

    /* Get column metadata from cache (once for all items) */
    col_meta = NULL;
    if (conn != NULL)
    {
        TeccState *state;
        TeccColumnCacheItem *cache_item;
        const char *db_name;

        state = tecc_get_state(conn);
        if (state != NULL)
        {
            db_name = PQdb(conn);
            if (db_name == NULL || db_name[0] == '\0')
                db_name = "postgres";
            cache_item = tecc_cache_get_encrypted_column_by_name(state, db_name,
                                                                 schema_name, table_name,
                                                                 column_name);
            if (cache_item != NULL)
                col_meta = tecc_convert_column_cache_item_to_metadata(state, cache_item);
        }
    }

    /* Permission check (once for all items) */
    all_skipped = false;
    if (conn != NULL && col_meta != NULL)
    {
        TeccState *perm_state;

        perm_state = tecc_get_state(conn);
        if (perm_state != NULL)
        {
            if (!tecc_cache_check_column_dek_permission(perm_state,
                                                        schema_name,
                                                        table_name, column_name))
            {
                all_skipped = true;
                TECC_SCHEDULER_DEBUG("Batch decrypt: no DEK permission, skipping all");
            }
        }
    }

    /* Determine DEK name */
    dek_name_to_use = NULL;
    if (!all_skipped)
    {
        if (col_meta != NULL && col_meta->dek_name != NULL &&
            col_meta->dek_name[0] != '\0')
        {
            dek_name_to_use = col_meta->dek_name;
        }
        else
        {
            /* For batch, we can't peek at individual cipherdata for dist_id
             * without decoding first. We'll do it per-item below if needed. */
        }
    }

    /* Resolve DEK name by decoding first item if needed */
    if (!all_skipped && dek_name_to_use == NULL && count > 0 && hex_strings[0])
    {
        unsigned char *first_binary;
        size_t first_binary_len;

        fmt_ret = tecc_formatter_hex_decode(hex_strings[0],
                                             &first_binary, &first_binary_len);
        if (fmt_ret == TECC_FORMATTER_SUCCESS)
        {
            CipherDataType *cd;
            Oid dist_id;

            cd = (CipherDataType *)first_binary;
            dist_id = cd->dek_dist_oid;
            if (OidIsValid(dist_id) && conn != NULL)
            {
                TeccState *lookup_state;

                lookup_state = tecc_get_state(conn);
                if (lookup_state != NULL)
                {
                    const char *found_name;

                    found_name = tecc_cache_get_dek_name_by_distributed_id(
                        lookup_state, dist_id);
                    if (found_name != NULL)
                        dek_name_to_use = found_name;
                }
            }
            pg_free(first_binary);
        }
    }

    if (!all_skipped && dek_name_to_use == NULL)
        all_skipped = true;

    /* ReqID:TECC4.0 - HE-only columns (he-add/he-mul, no RND/DET/OPE/SEARCH)
     * have no SM4-DEK, so skip the symmetric DEK fetch entirely.  Trying it
     * would fail and emit a spurious ERROR log before the HE decrypt path
     * below handles the column. */
    if (!all_skipped && col_meta != NULL &&
        (col_meta->enable_he_add || col_meta->enable_he_mul) &&
        !col_meta->enable_det && !col_meta->enable_ope && !col_meta->enable_search)
        all_skipped = true;

    /* ReqID:TECC4.0 - Bug 9150: 视图聚合结果列（如 sum(col_headd)）在
     * 后处理阶段无法通过表 OID/attnum 查到 col_meta（视图 OID 不在加密列
     * 缓存中），HE-only 检查被跳过，导致对纯 HE 密文尝试解密对称 DEK 并
     * 报错。此处对 col_meta == NULL 的情况，直接解析首条密文的段结构判断
     * 是否为纯 HE 密文（无 RND/DET/OPE/SEARCH 段），提前跳过对称 DEK 获取。 */
    if (!all_skipped && col_meta == NULL && dek_name_to_use != NULL &&
        count > 0 && hex_strings[0] != NULL)
    {
        unsigned char *peek_bin = NULL;
        size_t peek_len = 0;

        if (tecc_formatter_hex_decode(hex_strings[0], &peek_bin, &peek_len) == 0 &&
            peek_bin != NULL && peek_len >= 24)
        {
            CipherDataType *peek_cd = (CipherDataType *)peek_bin;

            /* ReqID:TECC4.0 - mask-aware: absent segments read as 0 */
            if (CIPHERDATA_DATUM_RND_LEN(peek_cd) == 0 &&
                CIPHERDATA_DATUM_DET_LEN(peek_cd) == 0 &&
                CIPHERDATA_DATUM_OPE_LEN(peek_cd) == 0 &&
                CIPHERDATA_DATUM_SEARCH_LEN(peek_cd) == 0 &&
                (CIPHERDATA_DATUM_EG_ADD_LEN(peek_cd) > 0 ||
                 CIPHERDATA_DATUM_EG_MUL_LEN(peek_cd) > 0))
            {
                all_skipped = true;
            }
            pg_free(peek_bin);
        }
    }

    /* Get DEK key (once for all items) */
    dek_key = NULL;
    dek_key_len = 0;
    derived_enc_key = NULL;
    derived_auth_key = NULL;

    if (!all_skipped)
    {
        ret = tecc_key_manager_adapter_get_dek_key_ext(
            scheduler->key_adapter, dek_name_to_use,
            &dek_key, &dek_key_len, &derived_enc_key, &derived_auth_key);
        if (ret != TECC_CRYPTO_SUCCESS)
            all_skipped = true;
    }

    if (all_skipped)
    {
        /* ReqID:TECC4.0 - HE 批量解密：惰性缓存段内 DEK 私钥 + 复用 BN_CTX，
         * 消除每行 SM2 信封解密与 BN_CTX 创建开销。按段内 DEK 名缓存（strcmp），
         * 多 DEK 列 DEK 变化时重新加载。 */
        BIGNUM *cached_ec_priv = NULL;
        BIGNUM *cached_ff_priv = NULL;
        BN_CTX *cached_ctx = NULL;
        char    cached_dek_name[NAMEDATALEN];

        cached_dek_name[0] = '\0';

#ifndef WIN32
        /* ReqID:TECC4.0 - HE 并行快速路径：预取第一行段内 DEK 私钥，行数足够大时
         * 临时创建线程分片并行。预取失败或多 DEK 差异回退下方串行惰性缓存。
         * fork 安全：worker 为函数内临时线程，函数返回前已全部 join，fork 时
         * 无活跃线程；he_crypto_init/he_eg_init 用 pthread_once 且初始化后状态
         * 只读，子进程继承安全。 */
        {
            int nthreads = tecc_he_get_thread_count(count);

            if (nthreads > 1 && count >= TECC_HE_PARALLEL_MIN_ROWS && hex_strings[0] != NULL)
            {
                TeccState *pst = tecc_get_state(conn);

                if (pst != NULL)
                {
                    unsigned char *first_bin = NULL;
                    size_t         first_bin_len = 0;
                    TeccParsedCipherdata first_pd;
                    BIGNUM *batch_ec_priv = NULL;
                    BIGNUM *batch_ff_priv = NULL;

                    if (tecc_formatter_hex_decode(hex_strings[0], &first_bin, &first_bin_len) == 0 &&
                        first_bin != NULL && first_bin_len >= 24 &&
                        tecc_formatter_parse_cipherdata((CipherDataType *)first_bin,
                                                        first_bin_len, &first_pd) == 0 &&
                        first_pd.rnd_data_len == 0 &&
                        (first_pd.eg_add_data_len > 0 || first_pd.eg_mul_data_len > 0))
                    {
                        Oid         seg_dek = (first_pd.eg_add_dek_id != InvalidOid) ?
                            first_pd.eg_add_dek_id : first_pd.eg_mul_dek_id;
                        const char *nm = NULL;

                        if (OidIsValid(seg_dek))
                            nm = tecc_cache_get_dek_name_by_distributed_id(pst, seg_dek);
                        if (nm == NULL)
                            nm = dek_name_to_use;
                        if (nm != NULL)
                        {
                            if (first_pd.eg_add_data_len > 0)
                                tecc_he_load_dek_privkey(pst, nm, 1, &batch_ec_priv);
                            if (first_pd.eg_mul_data_len > 0)
                                tecc_he_load_dek_privkey(pst, nm, 0, &batch_ff_priv);
                        }
                    }
                    if (first_bin)
                        pg_free(first_bin);

                    /* ReqID:TECC4.0 - 预取私钥可能为 NULL（段内 DEK 无 HE 私钥时
                     * 硬编码 fallback），仍分片并行（worker 内 ex 用硬编码私钥）。 */
                    {
                        pthread_t       *threads;
                        TeccHeWorkerArg *wargs;
                        int              wi;

                        threads = pg_malloc0(sizeof(pthread_t) * nthreads);
                        wargs = pg_malloc0(sizeof(TeccHeWorkerArg) * nthreads);
                        if (threads != NULL && wargs != NULL)
                        {
                            for (wi = 0; wi < nthreads; wi++)
                            {
                                wargs[wi].start = (int)((long long)count * wi / nthreads);
                                wargs[wi].end = (int)((long long)count * (wi + 1) / nthreads);
                                wargs[wi].hex_strings = (char **)hex_strings;
                                wargs[wi].ec_priv = batch_ec_priv;
                                wargs[wi].ff_priv = batch_ff_priv;
                                wargs[wi].results = results;
                            }
                            for (wi = 0; wi < nthreads; wi++)
                            {
                                if (pthread_create(&threads[wi], NULL,
                                                   tecc_he_decrypt_worker, &wargs[wi]) != 0)
                                {
                                    TECC_LOG(TECC_LOG_INFO,
                                             "HE batch: pthread_create failed for thread %d, "
                                             "items [%d,%d) will be processed serially",
                                             wi, wargs[wi].start, wargs[wi].end);
                                    threads[wi] = 0;
                                }
                            }
                            for (wi = 0; wi < nthreads; wi++)
                            {
                                if (threads[wi] != 0)
                                    pthread_join(threads[wi], NULL);
                            }
                            pg_free(threads);
                            pg_free(wargs);
                        }
                        else
                        {
                            TECC_SCHEDULER_DEBUG("HE batch parallel allocation failed (threads=%p, wargs=%p), falling back to serial",
                                                 (void *)threads, (void *)wargs);
                            if (threads)
                                pg_free(threads);
                            if (wargs)
                                pg_free(wargs);
                        }
                    }

                    if (batch_ec_priv)
                        BN_free(batch_ec_priv);
                    if (batch_ff_priv)
                        BN_free(batch_ff_priv);
                }
            }
        }
#endif /* !WIN32 */

        /* ReqID:TECC4.0 - Try HE decrypt before falling back to hex */
        for (i = 0; i < count; i++)
        {
            bool he_done = false;

            /* ReqID:TECC4.0 - 并行路径已解密该行（results[i] 非 NULL），跳过 */
            if (results[i] != NULL)
                continue;

            if (hex_strings[i] != NULL)
            {
                unsigned char *bin = NULL;
                size_t bin_len = 0;
                TeccParsedCipherdata pd;

                if (tecc_formatter_hex_decode(hex_strings[i], &bin, &bin_len) == 0 &&
                    bin != NULL && bin_len >= 24 &&
                    tecc_formatter_parse_cipherdata(
                        (CipherDataType *)bin, bin_len, &pd) == 0 &&
                    pd.rnd_data_len == 0 &&
                    (pd.eg_add_data_len > 0 || pd.eg_mul_data_len > 0))
                {
                    char   *he_str = NULL;
                    size_t  he_str_len = 0;
                    /* ReqID:TECC4.0 - P2-1: prefer the per-segment DEK
                     * identity over the column-level DEK name.  This keeps
                     * decryption correct once a column uses multiple DEKs
                     * (P0-2); single-DEK columns resolve to the same name. */
                    const char *dek_to_use = dek_name_to_use;
                    TeccState *st = tecc_get_state(conn);

                    if (st != NULL &&
                        (pd.eg_add_dek_id != InvalidOid ||
                         pd.eg_mul_dek_id != InvalidOid))
                    {
                        Oid     seg_dek = (pd.eg_add_dek_id != InvalidOid) ?
                            pd.eg_add_dek_id : pd.eg_mul_dek_id;
                        const char *nm =
                            tecc_cache_get_dek_name_by_distributed_id(st, seg_dek);

                        if (nm != NULL)
                            dek_to_use = nm;
                    }

                        /* ReqID:TECC4.0 - 惰性加载段内 DEK 私钥（DEK 变化时重载） */
                        if (st != NULL &&
                            (cached_dek_name[0] == '\0' ||
                             strcmp(cached_dek_name, dek_to_use) != 0))
                        {
                            strlcpy(cached_dek_name, dek_to_use, NAMEDATALEN);
                            if (cached_ec_priv)
                            { BN_free(cached_ec_priv); cached_ec_priv = NULL; }
                            if (cached_ff_priv)
                            { BN_free(cached_ff_priv); cached_ff_priv = NULL; }
                            if (pd.eg_add_data_len > 0)
                                tecc_he_load_dek_privkey(st, dek_to_use, 1, &cached_ec_priv);
                            if (pd.eg_mul_data_len > 0)
                                tecc_he_load_dek_privkey(st, dek_to_use, 0, &cached_ff_priv);
                        }
                        if (cached_ctx == NULL)
                            cached_ctx = BN_CTX_new();

                        {
                            const BIGNUM *row_priv = (pd.eg_add_data_len > 0) ?
                                cached_ec_priv : cached_ff_priv;

                            if (tecc_he_decrypt_cipherdata_ex(
                                    pd.eg_add_data, pd.eg_add_data_len,
                                    pd.eg_mul_data, pd.eg_mul_data_len,
                                    row_priv, cached_ctx,
                                    &he_str, &he_str_len) == 0 && he_str != NULL)
                            {
                                results[i] = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
                                if (results[i])
                                {
                                    results[i]->plaintext = he_str;
                                    results[i]->plain_len = he_str_len;
                                    results[i]->skipped_decrypt = true;
                                    results[i]->error_code = TECC_CRYPTO_SUCCESS;
                                }
                                else
                                {
                                    pg_free(he_str);
                                }
                                he_done = true;
                                pg_free(bin);
                            }
                            else
                            {
                                pg_free(bin);
                            }
                        }
                }
            }
            else
            {
                /* hex_strings[i] is NULL — handled in fallback */
            }

            if (!he_done)
            {
                results[i] = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
                if (results[i] != NULL)
                {
                    size_t hex_len;

                    hex_len = (hex_strings[i] != NULL) ? strlen(hex_strings[i]) : 0;
                    results[i]->plaintext = (char *)pg_malloc(hex_len + 3);
                    if (results[i]->plaintext != NULL)
                    {
                        if (hex_strings[i] != NULL)
                        {
                            results[i]->plaintext[0] = '\\';
                            results[i]->plaintext[1] = 'x';
                            memcpy(results[i]->plaintext + 2, hex_strings[i], hex_len);
                            results[i]->plaintext[hex_len + 2] = '\0';
                            results[i]->plain_len = hex_len + 2;
                        }
                        else
                        {
                            results[i]->plaintext[0] = '\0';
                            results[i]->plain_len = 0;
                        }
                    }
                    results[i]->skipped_decrypt = true;
                    results[i]->error_code = TECC_CRYPTO_SUCCESS;
                }
            }
        }

        /* ReqID:TECC4.0 - 释放惰性缓存的私钥与复用的 BN_CTX */
        if (cached_ec_priv)
            BN_free(cached_ec_priv);
        if (cached_ff_priv)
            BN_free(cached_ff_priv);
        if (cached_ctx)
            BN_CTX_free(cached_ctx);

        if (dek_key)
            tecc_key_manager_adapter_release_key(scheduler->key_adapter,
                                                 dek_key, dek_key_len);
        pg_free(binary_data);
        pg_free(binary_lens);
        pg_free(rnd_ciphertexts);
        pg_free(rnd_ciphertext_lens);
        if (col_meta)
            tecc_free_encrypted_column_metadata(col_meta);
        if (!context_is_external) pg_free(context);
        return results;
    }

    /* Update algorithm support from metadata */
    if (col_meta)
    {
        context->core.algo_support.supports_det = col_meta->enable_det;
        context->core.algo_support.supports_ope = col_meta->enable_ope;
        context->core.algo_support.supports_search = col_meta->enable_search;
        context->core.algo_support.supports_he_add = col_meta->enable_he_add;
        context->core.algo_support.supports_he_mul = col_meta->enable_he_mul;
        context->core.data_type = tecc_map_to_original_type_oid(col_meta->data_type_original_oid);
        context->core.type_mod = col_meta->data_type_original_mod;
        context->core.collation_oid = col_meta->collation_oid;
        if (col_meta->collation_locale != NULL)
            strlcpy(context->core.collation_locale, col_meta->collation_locale, NAMEDATALEN);
    }

    /* Detect HSM SM4 encryption for this column.
     * If HSM is configured, the batch software RND decrypt path cannot be used
     * because HSM encrypts data as raw SM4-CBC ([16-byte IV][ciphertext]) without
     * the software Sm4RndDataFormat wrapper (version + auth_tag + IV + ct_len + ct).
     * We fall through to per-item HSM decrypt below.
     *
     * ReqID:TECC4.0 - Bug 9384: 无列元数据（纯加密常量表达式）时不能就此判定为
     * 软件路由——用上面按 dek_dist_oid 解析出的 DEK 名合成最小元数据来判定。
     * 否则 HSM 密文会走软件批量解密，报 "Batch decrypt: item N failed"。 */
    {
        TeccState *local_state = tecc_get_state(conn);
        TeccEncryptedColumnMetadata *hsm_meta = col_meta;

        if (hsm_meta == NULL)
            hsm_meta = tecc_hsm_meta_synth_from_dek(dek_name_to_use);

        if (local_state != NULL && hsm_meta != NULL)
        {
            if (tecc_setup_hsm_context(context, hsm_meta, local_state) < 0)
            {
                TECC_SCHEDULER_ERROR("Batch decrypt: HSM init failed for column %s.%s",
                                     table_name, column_name);
                /* HSM init failure is fatal — no software fallback.
                 * Mark all items as errors and let caller handle. */
                for (i = 0; i < count; i++)
                {
                    results[i] = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
                    if (results[i])
                    {
                        results[i]->error_code = TECC_CRYPTO_KEY_ERROR;
                        snprintf(results[i]->error_msg, sizeof(results[i]->error_msg),
                                 "Batch decrypt: HSM init failed for column %s.%s",
                                 table_name, column_name);
                    }
                }
                tecc_key_manager_adapter_release_key(scheduler->key_adapter,
                                                     dek_key, dek_key_len);
                pg_free(binary_data);
                pg_free(binary_lens);
                pg_free(rnd_ciphertexts);
                pg_free(rnd_ciphertext_lens);
                /* hsm_meta 为 col_meta 或本次合成的临时元数据（后者时 col_meta 为 NULL） */
                if (hsm_meta)
                    tecc_free_encrypted_column_metadata(hsm_meta);
                if (!context_is_external) pg_free(context);
                return results;
            }
        }

        /* 合成的临时元数据仅用于本次 HSM 判定，col_meta 本身仍由下方统一释放 */
        if (hsm_meta != NULL && hsm_meta != col_meta)
            tecc_free_encrypted_column_metadata(hsm_meta);
    }

    /* Batch hex decode: single allocation for all binary data */
    fmt_ret = tecc_formatter_batch_hex_decode(hex_strings, count,
                                               binary_data, binary_lens);
    if (fmt_ret != TECC_FORMATTER_SUCCESS)
    {
        for (i = 0; i < count; i++)
            results[i] = NULL;
        tecc_key_manager_adapter_release_key(scheduler->key_adapter,
                                             dek_key, dek_key_len);
        pg_free(binary_data);
        pg_free(binary_lens);
        pg_free(rnd_ciphertexts);
        pg_free(rnd_ciphertext_lens);
        if (col_meta)
            tecc_free_encrypted_column_metadata(col_meta);
        if (!context_is_external) pg_free(context);
        return results;
    }
    batch_binary_base = binary_data[0];

    /* Per-item: parse CipherDataType, extract RND ciphertext */
    for (i = 0; i < count; i++)
    {
        CipherDataType *cipherdata;
        TeccParsedCipherdata parsed_data;

        results[i] = NULL;
        rnd_ciphertexts[i] = NULL;
        rnd_ciphertext_lens[i] = 0;

        if (!binary_data[i] || binary_lens[i] < 24)
        {
            results[i] = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
            if (results[i])
            {
                results[i]->error_code = TECC_CRYPTO_FORMAT_ERROR;
                snprintf(results[i]->error_msg, sizeof(results[i]->error_msg),
                         "Batch decrypt: item %zu too short", i);
            }
            continue;
        }

        cipherdata = (CipherDataType *)binary_data[i];

        if (!tecc_validate_cipherdata_integrity(cipherdata, binary_lens[i]))
        {
            results[i] = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
            if (results[i])
            {
                results[i]->error_code = TECC_CRYPTO_FORMAT_ERROR;
                snprintf(results[i]->error_msg, sizeof(results[i]->error_msg),
                         "Batch decrypt: item %zu integrity check failed", i);
            }
            continue;
        }

        fmt_ret = tecc_formatter_parse_cipherdata(cipherdata, binary_lens[i],
                                                   &parsed_data);
        if (fmt_ret != TECC_FORMATTER_SUCCESS)
        {
            results[i] = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
            if (results[i])
            {
                results[i]->error_code = TECC_CRYPTO_FORMAT_ERROR;
                snprintf(results[i]->error_msg, sizeof(results[i]->error_msg),
                         "Batch decrypt: item %zu parse failed", i);
            }
            continue;
        }

        if (parsed_data.rnd_data_len == 0)
        {
            /*
             * ReqID:TECC4.0 - Handle HE-only results: no RND data but may have
             * EG_ADD/EG_MUL (homomorphic computation results).  Try HE decrypt
             * with per-DEK private key before falling back to hex pass-through. */
            if (parsed_data.eg_add_data_len > 0 || parsed_data.eg_mul_data_len > 0)
            {
                char *he_str = NULL;
                size_t he_str_len = 0;
                const char *he_dek_name = NULL;
                void *he_state = NULL;

                /* Try to resolve DEK name from cipherdata.  ReqID:TECC4.0
                 * P2-1: prefer the per-segment DEK identity (EG_ADD/EG_MUL
                 * prefix) which reflects the DEK the result was computed
                 * with; fall back to the header dek_dist_oid for legacy
                 * single-DEK ciphertexts. */
                if (conn != NULL)
                {
                    TeccState *st = tecc_get_state(conn);
                    if (st != NULL)
                    {
                        Oid     dist_id;
                        const char *nm;

                        if (parsed_data.eg_add_dek_id != InvalidOid)
                            dist_id = parsed_data.eg_add_dek_id;
                        else if (parsed_data.eg_mul_dek_id != InvalidOid)
                            dist_id = parsed_data.eg_mul_dek_id;
                        else
                            dist_id = CIPHERDATA_DEK_DIST_OID(cipherdata);

                        nm = tecc_cache_get_dek_name_by_distributed_id(st, dist_id);
                        if (nm != NULL)
                            he_dek_name = nm;
                        he_state = st;
                    }
                }

                if (tecc_he_decrypt_cipherdata(
                        parsed_data.eg_add_data, parsed_data.eg_add_data_len,
                        parsed_data.eg_mul_data, parsed_data.eg_mul_data_len,
                        he_dek_name, he_state,
                        &he_str, &he_str_len) == 0 && he_str != NULL)
                {
                    results[i] = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
                    if (results[i])
                    {
                        results[i]->plaintext = he_str;
                        results[i]->plain_len = he_str_len;
                        results[i]->skipped_decrypt = true;
                        results[i]->error_code = TECC_CRYPTO_SUCCESS;
                    }
                    else
                    {
                        pg_free(he_str);
                    }
                    continue;
                }
            }
            /* Fallback: pass through hex string (only meaningful for HE
             * results that could not be decrypted client-side; e.g. server
             * homomorphic results carried as hex).  For a symmetric-only
             * column with no RND data, there is nothing to decrypt — report
             * the error rather than silently emitting the raw hex. */
            if (parsed_data.eg_add_data_len == 0 && parsed_data.eg_mul_data_len == 0)
            {
                results[i] = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
                if (results[i])
                {
                    results[i]->error_code = TECC_CRYPTO_FORMAT_ERROR;
                    snprintf(results[i]->error_msg, sizeof(results[i]->error_msg),
                             "Batch decrypt: item %zu has no RND data", i);
                }
                continue;
            }
            results[i] = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
            if (results[i])
            {
                size_t hex_len;

                results[i]->skipped_decrypt = true;
                results[i]->error_code = TECC_CRYPTO_SUCCESS;
                /* Begin - ReqID:TECC4.0 - Bug 9334 */
                /* ReqID:TECC4.0 - 未解密的密文必须以 "\x" 前缀回传，与上面
                 * all_skipped（纯同态列）分支的兜底保持一致：结果处理阶段
                 * 只把 "\x" 开头的值识别为密文原样输出（tecc_processor.c 的
                 * is_hex_ct），否则会拿十六进制密文去跑整数范围校验，报出
                 * 与事实不符的 "HE result out of range for column"，把真正的
                 * 失败原因（HE 私钥不可用）掩盖掉。plain_len 与其它分支一致
                 * 计入前缀两字节。 */
                hex_len = (hex_strings != NULL && hex_strings[i] != NULL) ?
                    strlen(hex_strings[i]) : 0;
                if (hex_len > 0)
                {
                    results[i]->plaintext = (char *)pg_malloc(hex_len + 3);
                    if (results[i]->plaintext != NULL)
                    {
                        results[i]->plaintext[0] = '\\';
                        results[i]->plaintext[1] = 'x';
                        memcpy(results[i]->plaintext + 2, hex_strings[i], hex_len);
                        results[i]->plaintext[hex_len + 2] = '\0';
                        results[i]->plain_len = hex_len + 2;
                    }
                }
                /* End - ReqID:TECC4.0 - Bug 9334 */
            }
            continue;
        }

        rnd_ciphertexts[i] = parsed_data.rnd_data;
        rnd_ciphertext_lens[i] = parsed_data.rnd_data_len;
        results[i] = NULL; /* Will be filled by batch decrypt below */
    }

    /* Batch SM4-RND decrypt: shared SM4 context, one key expansion */
    {
        unsigned char **plaintexts;
        size_t *plaintext_lens;
        const unsigned char **valid_ct;
        size_t *valid_ct_lens;
        size_t *idx_map;
        size_t valid_count;
        bool batch_ok;
        /* ReqID:TECC4.0 - 硬加密列拼接流状态（说明见 tecc_hsm_decrypt_stream） */
        bool hsm_batch;
        size_t hsm_call_limit;
        unsigned char *hsm_stream;
        size_t hsm_stream_len;
        size_t *hsm_off;
        size_t *hsm_seg;
        size_t *hsm_pend;
        size_t hsm_n;

        plaintexts = pg_malloc(sizeof(unsigned char *) * count);
        plaintext_lens = pg_malloc(sizeof(size_t) * count);
        valid_ct = pg_malloc(sizeof(const unsigned char *) * count);
        valid_ct_lens = pg_malloc(sizeof(size_t) * count);
        idx_map = pg_malloc(sizeof(size_t) * count);
        if (!plaintexts || !plaintext_lens || !valid_ct || !valid_ct_lens || !idx_map)
        {
            pg_free(batch_binary_base);
            tecc_key_manager_adapter_release_key(scheduler->key_adapter,
                                                 dek_key, dek_key_len);
            pg_free(results);
            pg_free(binary_data);
            pg_free(binary_lens);
            pg_free(rnd_ciphertexts);
            pg_free(rnd_ciphertext_lens);
            if (col_meta) tecc_free_encrypted_column_metadata(col_meta);
            if (!context_is_external) pg_free(context);
            return NULL;
        }

        /* Build list of valid items for batch decrypt.
         * If HSM is active for this column, skip software batch decrypt —
         * HSM-encrypted RND data uses raw [IV + SM4-CBC ciphertext] format,
         * which is incompatible with the software Sm4RndDataFormat wrapper
         * (version + auth_tag + IV + ct_len + ct) expected by the batch API.
         * HSM items are instead concatenated into one stream and decrypted with
         * a single device call per chunk (see tecc_hsm_decrypt_stream);
         * TECC_HSM_BATCH=0 falls back to per-item device calls. */
        hsm_batch = (context->hsm_kms_type != 0) && tecc_hsm_batch_decrypt_enabled();
        hsm_call_limit = hsm_batch ? tecc_hsm_batch_call_limit() : 0;
        hsm_stream = NULL;
        hsm_stream_len = 0;
        hsm_off = NULL;
        hsm_seg = NULL;
        hsm_pend = NULL;
        hsm_n = 0;

        if (hsm_batch)
        {
            hsm_stream = (unsigned char *)pg_malloc(hsm_call_limit);
            hsm_off = pg_malloc(sizeof(size_t) * count);
            hsm_seg = pg_malloc(sizeof(size_t) * count);
            hsm_pend = pg_malloc(sizeof(size_t) * count);
            if (hsm_stream == NULL || hsm_off == NULL || hsm_seg == NULL ||
                hsm_pend == NULL)
            {
                /* 内存不足：退回逐值路径（下面分支按 hsm_batch=false 处理） */
                if (hsm_stream) pg_free(hsm_stream);
                if (hsm_off) pg_free(hsm_off);
                if (hsm_seg) pg_free(hsm_seg);
                if (hsm_pend) pg_free(hsm_pend);
                hsm_stream = NULL;
                hsm_off = NULL;
                hsm_seg = NULL;
                hsm_pend = NULL;
                hsm_batch = false;
                hsm_call_limit = 0;
            }
        }

        valid_count = 0;
        for (i = 0; i < count; i++)
        {
            if (rnd_ciphertexts[i] != NULL && rnd_ciphertext_lens[i] > 0 &&
                results[i] == NULL)
            {
                if (context->hsm_kms_type != 0)
                {
                    /* HSM column: ciphertext layout is [16-byte IV][SM4-CBC ciphertext].
                     * 拼接流可用时攒进流里，达到块上限再冲刷；否则逐值调用。 */
                    HsmAdapterInterface *hsm = (HsmAdapterInterface *)context->hsm_adapter;
                    HsmDevice *hsm_dev = (HsmDevice *)context->hsm_device;
                    bool need_single;

                    need_single = !hsm_batch || hsm == NULL || hsm_dev == NULL ||
                                  hsm->sm4_decrypt == NULL ||
                                  rnd_ciphertext_lens[i] <= 16 ||
                                  rnd_ciphertext_lens[i] > hsm_call_limit;

                    if (need_single)
                    {
                        tecc_hsm_decrypt_item(hsm, hsm_dev, dek_key, dek_key_len,
                                              context->hsm_kms_type,
                                              rnd_ciphertexts[i],
                                              rnd_ciphertext_lens[i],
                                              i, &results[i]);
                        continue;
                    }

                    /* 流将满：先冲刷已攒的值（整批失败则退回逐值，行为不退化） */
                    if (hsm_stream_len > 0 &&
                        hsm_stream_len + rnd_ciphertext_lens[i] > hsm_call_limit)
                    {
                        size_t flush_k;

                        if (!tecc_hsm_decrypt_stream(hsm, hsm_dev, dek_key, dek_key_len,
                                                     context->hsm_kms_type,
                                                     hsm_stream, hsm_stream_len,
                                                     hsm_off, hsm_seg, hsm_pend,
                                                     hsm_n, results))
                        {
                            TECC_SCHEDULER_DEBUG("HSM batch stream failed, "
                                                 "falling back to per-item decrypt");
                            for (flush_k = 0; flush_k < hsm_n; flush_k++)
                            {
                                size_t idx = hsm_pend[flush_k];

                                tecc_hsm_decrypt_item(hsm, hsm_dev, dek_key, dek_key_len,
                                                      context->hsm_kms_type,
                                                      rnd_ciphertexts[idx],
                                                      rnd_ciphertext_lens[idx],
                                                      idx, &results[idx]);
                            }
                        }
                        hsm_stream_len = 0;
                        hsm_n = 0;
                    }

                    /* 追加本值：[IV][CT] 整段原样进流（IV 必须是未被改写的原值） */
                    hsm_off[hsm_n] = hsm_stream_len;
                    hsm_seg[hsm_n] = rnd_ciphertext_lens[i];
                    hsm_pend[hsm_n] = i;
                    hsm_n++;
                    memcpy(hsm_stream + hsm_stream_len, rnd_ciphertexts[i],
                           rnd_ciphertext_lens[i]);
                    hsm_stream_len += rnd_ciphertext_lens[i];
                }
                else
                {
                    /* Software column: add to batch for shared key expansion */
                    valid_ct[valid_count] = rnd_ciphertexts[i];
                    valid_ct_lens[valid_count] = rnd_ciphertext_lens[i];
                    idx_map[valid_count] = i;
                    valid_count++;
                }
            }
        }

        /* 收尾：冲刷剩余拼接流（整批失败同样退回逐值） */
        if (hsm_n > 0)
        {
            HsmAdapterInterface *hsm_tail = (HsmAdapterInterface *)context->hsm_adapter;
            HsmDevice *hsm_dev_tail = (HsmDevice *)context->hsm_device;
            size_t flush_k;

            if (!tecc_hsm_decrypt_stream(hsm_tail, hsm_dev_tail, dek_key, dek_key_len,
                                         context->hsm_kms_type,
                                         hsm_stream, hsm_stream_len,
                                         hsm_off, hsm_seg, hsm_pend,
                                         hsm_n, results))
            {
                TECC_SCHEDULER_DEBUG("HSM batch stream failed (tail), "
                                     "falling back to per-item decrypt");
                for (flush_k = 0; flush_k < hsm_n; flush_k++)
                {
                    size_t idx = hsm_pend[flush_k];

                    tecc_hsm_decrypt_item(hsm_tail, hsm_dev_tail, dek_key, dek_key_len,
                                          context->hsm_kms_type,
                                          rnd_ciphertexts[idx],
                                          rnd_ciphertext_lens[idx],
                                          idx, &results[idx]);
                }
            }
        }
        if (hsm_stream) pg_free(hsm_stream);
        if (hsm_off) pg_free(hsm_off);
        if (hsm_seg) pg_free(hsm_seg);
        if (hsm_pend) pg_free(hsm_pend);

        if (valid_count > 0)
        {
            ret = tecc_sm4_rnd_batch_decrypt_with_keys(
                derived_enc_key, derived_auth_key,
                valid_ct, valid_ct_lens, valid_count,
                plaintexts, plaintext_lens);
            batch_ok = (ret == 0); /* SM4_RND_SUCCESS == 0 */

            /* Build results from batch decrypt output */
            for (i = 0; i < valid_count; i++)
            {
                size_t orig_idx;

                orig_idx = idx_map[i];
                results[orig_idx] = pg_malloc0(sizeof(TeccUnifiedDecryptResult));
                if (results[orig_idx] != NULL)
                {
                    if (batch_ok && plaintexts[i] != NULL)
                    {
                        results[orig_idx]->plaintext = (char *)plaintexts[i];
                        results[orig_idx]->plain_len = plaintext_lens[i];
                        results[orig_idx]->error_code = TECC_CRYPTO_SUCCESS;
                    }
                    else
                    {
                        results[orig_idx]->error_code = TECC_CRYPTO_DATA_ERROR;
                        snprintf(results[orig_idx]->error_msg,
                                 sizeof(results[orig_idx]->error_msg),
                                 "Batch decrypt: item %zu failed", orig_idx);
                    }
                }
                else if (batch_ok && plaintexts[i] != NULL)
                {
                    free(plaintexts[i]);
                }
            }
        }

        pg_free(plaintexts);
        pg_free(plaintext_lens);
        pg_free(valid_ct);
        pg_free(valid_ct_lens);
        pg_free(idx_map);
    }

    /* Cleanup */
    tecc_key_manager_adapter_release_key(scheduler->key_adapter,
                                         dek_key, dek_key_len);
    context->dek_key = NULL;
    context->dek_key_len = 0;
    pg_free(batch_binary_base);
    if (context_is_external)
    {
        context->rnd_ciphertext = NULL;
        context->det_ciphertext = NULL;
        context->ope_ciphertext = NULL;
        context->search_ciphertext = NULL;
    }
    else
    {
        if (!context_is_external) pg_free(context);
    }
    if (col_meta)
        tecc_free_encrypted_column_metadata(col_meta);
    pg_free(binary_data);
    pg_free(binary_lens);
    pg_free(rnd_ciphertexts);
    pg_free(rnd_ciphertext_lens);

    /* Update statistics */
    scheduler->decryption_count += count;

    return results;
}

/*
 * Public batch decrypt hex wrapper
 */
TeccUnifiedDecryptResult **
tecc_unified_batch_decrypt_hex(PGconn *conn,
                               const char **hex_strings,
                               size_t count,
                               const char *schema_name,
                               const char *table_name,
                               const char *column_name,
                               TeccCryptoContext *external_context)
{
    TeccCryptoScheduler *scheduler;

    if (conn == NULL || hex_strings == NULL || count == 0 ||
        table_name == NULL || column_name == NULL)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_unified_batch_decrypt_hex");
        return NULL;
    }

    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to get crypto scheduler from connection");
        return NULL;
    }

    return tecc_unified_batch_decrypt_hex_internal_with_scheduler(
        conn, scheduler, hex_strings, count,
        schema_name, table_name, column_name, external_context);
}
/* End - ReqID:TECC4.0 */

/*
 * Unified encryption with collation support and connection parameter (thread-safe version)
 */
TeccUnifiedEncryptResult *
tecc_unified_encrypt_string_with_collation_and_conn(
    PGconn *conn,
    const char *plaintext,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name,
    Oid collation_oid,
    TeccCryptoContext *external_context)
{
    TeccCryptoScheduler *scheduler;

    /* Validate parameters */
    if (conn == NULL || plaintext == NULL || table_name == NULL || column_name == NULL)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_unified_encrypt_string_with_collation_and_conn");
        return NULL;
    }

    /* Get scheduler from connection state */
    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to get crypto scheduler from connection");
        return NULL;
    }

    /* Call internal implementation with scheduler */
    return tecc_unified_encrypt_string_internal_with_scheduler(scheduler,
                                                              plaintext,
                                                              table_name,
                                                              column_name,
                                                              NULL, /* schema_name - 未限定，按 search_path 查找 */
                                                              kek_name,
                                                              dek_name,
                                                              collation_oid,
                                                              tecc_get_state(conn),
                                                              NULL,
                                                              external_context,
                                                              false,   /* supports_he_add */
                                                              false);  /* supports_he_mul */
}

/* ==================== Batch Processing Implementation ==================== */
/* Begin - ReqID:TECC4.0 - Batch processing implementation for import/export module */


/*
 * Batch encryption interface with connection parameter (thread-safe version)
 */
TeccBatchEncryptResult *
tecc_unified_batch_encrypt_strings_with_conn(
    PGconn *conn,
    const char **plaintexts,
    const size_t *plaintext_lengths,
    size_t count,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    TeccCryptoScheduler *scheduler;
    TeccBatchEncryptResult *batch_result = NULL;
    size_t i;
    unsigned char *dek_key = NULL;
    size_t dek_key_len = 0;
    unsigned char *derived_enc_key = NULL;    /* ReqID:TECC4.0 */
    unsigned char *derived_auth_key = NULL;   /* ReqID:TECC4.0 */
    int key_ret = TECC_CRYPTO_SUCCESS;
    size_t success_count = 0;
    size_t error_count = 0;

    /* Validate parameters */
    if (conn == NULL || !plaintexts || !plaintext_lengths || count == 0 || !table_name || !column_name)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_unified_batch_encrypt_strings_with_conn");
        return NULL;
    }

    /* Get scheduler from connection state */
    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to get crypto scheduler from connection");
        return NULL;
    }

    TECC_SCHEDULER_DEBUG("Batch encryption: %s.%s (count=%zu)", table_name, column_name, count);

    /* Allocate batch result structure */
    batch_result = pg_malloc0(sizeof(TeccBatchEncryptResult));
    if (!batch_result)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate batch encrypt result");
        return NULL;
    }

    batch_result->count = count;

    /* Allocate arrays for individual results */
    batch_result->cipherdata_buffers = pg_malloc0(sizeof(TeccCryptoBuffer *) * count);
    batch_result->hex_strings = pg_malloc0(sizeof(char *) * count);
    batch_result->error_codes = pg_malloc0(sizeof(int) * count);

    if (!batch_result->cipherdata_buffers || !batch_result->hex_strings || !batch_result->error_codes)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate arrays for batch encrypt result");
        tecc_unified_free_batch_encrypt_result(batch_result);
        return NULL;
    }

    /* Initialize error codes to success */
    for (i = 0; i < count; i++)
    {
        batch_result->error_codes[i] = TECC_CRYPTO_SUCCESS;
    }

    /* Preload DEK key for batch optimization */
    dek_key = NULL;
    dek_key_len = 0;
    key_ret = TECC_CRYPTO_SUCCESS;

    if (dek_name)
    {
        key_ret = tecc_key_manager_adapter_get_dek_key_ext(scheduler->key_adapter,
                                                      dek_name,
                                                      &dek_key, &dek_key_len,
                                                      &derived_enc_key,
                                                      &derived_auth_key);
        if (key_ret != TECC_CRYPTO_SUCCESS)
        {
            TECC_SCHEDULER_WARN("Failed to preload DEK key '%s' for batch encryption, will load individually", dek_name);
            dek_key = NULL;
            dek_key_len = 0;
            derived_enc_key = NULL;
            derived_auth_key = NULL;
        }
        else
        {
            TECC_SCHEDULER_DEBUG("Preloaded DEK key for batch encryption: %s (len=%zu)", dek_name, dek_key_len);
        }
    }

    /* Process each item in the batch */
    for (i = 0; i < count; i++)
    {
        TeccUnifiedEncryptResult *single_result = NULL;
        const char *plaintext = plaintexts[i];

        /* Validate current item */
        if (!plaintext)
        {
            batch_result->error_codes[i] = TECC_CRYPTO_INVALID_PARAM;
            if (batch_result->overall_error_code == TECC_CRYPTO_SUCCESS)
            {
                snprintf(batch_result->overall_error_msg, sizeof(batch_result->overall_error_msg),
                         "Item %zu: NULL plaintext", i);
                batch_result->overall_error_code = TECC_CRYPTO_INVALID_PARAM;
            }
            continue;
        }

        /* Perform single encryption using batch-optimized function */
        single_result = tecc_unified_encrypt_string_with_preloaded_key(scheduler,
            plaintext, table_name, column_name, kek_name, dek_name,
            dek_key, dek_key_len,
            derived_enc_key, derived_auth_key);

        if (single_result && single_result->error_code == TECC_CRYPTO_SUCCESS)
        {
            /* Store successful result */
            batch_result->cipherdata_buffers[i] = single_result->cipherdata_buffer;
            batch_result->hex_strings[i] = single_result->hex_string;
            batch_result->error_codes[i] = TECC_CRYPTO_SUCCESS;

            /* Transfer ownership of buffer and hex string to batch result */
            single_result->cipherdata_buffer = NULL;
            single_result->hex_string = NULL;
        }
        else
        {
            /* Record error */
            if (single_result)
            {
                batch_result->error_codes[i] = single_result->error_code;
                if (batch_result->overall_error_code == TECC_CRYPTO_SUCCESS)
                {
                    snprintf(batch_result->overall_error_msg, sizeof(batch_result->overall_error_msg),
                             "Item %zu: %s", i, single_result->error_msg);
                    batch_result->overall_error_code = single_result->error_code;
                }

                /* Free resources if encryption failed */
                if (single_result->cipherdata_buffer)
                {
                    pg_free(single_result->cipherdata_buffer);
                    single_result->cipherdata_buffer = NULL;
                }
                if (single_result->hex_string)
                {
                    pg_free(single_result->hex_string);
                    single_result->hex_string = NULL;
                }
            }
            else
            {
                batch_result->error_codes[i] = TECC_CRYPTO_SYSTEM_ERROR;
                if (batch_result->overall_error_code == TECC_CRYPTO_SUCCESS)
                {
                    snprintf(batch_result->overall_error_msg, sizeof(batch_result->overall_error_msg),
                             "Item %zu: Encryption failed", i);
                    batch_result->overall_error_code = TECC_CRYPTO_SYSTEM_ERROR;
                }
            }
        }

        /* Free single result structure (but not the transferred buffers) */
        if (single_result)
        {
            pg_free(single_result);
        }
    }

    /* Calculate success and error counts for statistics and logging */
    success_count = 0;
    error_count = 0;
    for (i = 0; i < count; i++)
    {
        if (batch_result->error_codes[i] == TECC_CRYPTO_SUCCESS)
        {
            success_count++;
        }
        else
        {
            error_count++;
        }
    }

    /* Update statistics */
    if (scheduler)
    {
        scheduler->encryption_count += success_count;
        scheduler->error_count += error_count;

        /* Update batch-specific statistics */
        scheduler->batch_encryption_count++;
        scheduler->total_batch_items_processed += count;
        scheduler->successful_batch_items += success_count;
        scheduler->batch_error_count += error_count;
    }

    TECC_SCHEDULER_DEBUG("Batch encryption completed (success=%zu/%zu)",
                         success_count, count);

    /* Release preloaded DEK key if it was loaded */
    if (dek_key && dek_key_len > 0)
    {
        tecc_key_manager_adapter_release_key(scheduler->key_adapter, dek_key, dek_key_len);
        dek_key = NULL;
        dek_key_len = 0;
    }

    return batch_result;
}

/*
 * Internal batch decryption implementation with explicit scheduler parameter
 */
static TeccBatchDecryptResult *
tecc_unified_batch_decrypt_hex_strings_internal(
    TeccCryptoScheduler *scheduler,
    const char **hex_strings,
    const size_t *hex_lengths,
    size_t count,
    const char *table_name,
    const char *column_name)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    TeccBatchDecryptResult *batch_result = NULL;
    size_t i;
    size_t success_count = 0;
    size_t error_count = 0;

    /* Validate parameters */
    if (!scheduler || !hex_strings || !hex_lengths || count == 0 || !table_name || !column_name)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_unified_batch_decrypt_hex_strings_internal");
        return NULL;
    }

    TECC_SCHEDULER_DEBUG("Batch decryption internal: %s.%s (count=%zu)", table_name, column_name, count);

    /* Allocate batch result structure */
    batch_result = pg_malloc0(sizeof(TeccBatchDecryptResult));
    if (!batch_result)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate batch decrypt result");
        return NULL;
    }

    batch_result->count = count;
    batch_result->overall_error_code = TECC_CRYPTO_SUCCESS;
    batch_result->overall_error_msg[0] = '\0';

    /* Allocate arrays for individual results */
    batch_result->plaintexts = pg_malloc0(sizeof(char *) * count);
    batch_result->plaintext_lengths = pg_malloc0(sizeof(size_t) * count);
    batch_result->error_codes = pg_malloc0(sizeof(int) * count);

    if (!batch_result->plaintexts || !batch_result->plaintext_lengths || !batch_result->error_codes)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate arrays for batch decrypt result");
        tecc_unified_free_batch_decrypt_result(batch_result);
        return NULL;
    }

    /* Initialize error codes to success */
    for (i = 0; i < count; i++)
    {
        batch_result->error_codes[i] = TECC_CRYPTO_SUCCESS;
        batch_result->plaintext_lengths[i] = 0;
    }

    /* Process each item in the batch */
    for (i = 0; i < count; i++)
    {
        TeccUnifiedDecryptResult *single_result = NULL;
        const char *hex_string = hex_strings[i];
        /* Validate current item */
        if (!hex_string)
        {
            batch_result->error_codes[i] = TECC_CRYPTO_INVALID_PARAM;
            if (batch_result->overall_error_code == TECC_CRYPTO_SUCCESS)
            {
                snprintf(batch_result->overall_error_msg, sizeof(batch_result->overall_error_msg),
                         "Item %zu: NULL hex string", i);
                batch_result->overall_error_code = TECC_CRYPTO_INVALID_PARAM;
            }
            continue;
        }

        /* Optional: validate length if provided */
        if (hex_lengths[i] > 0)
        {
            size_t actual_len = strlen(hex_string);
            if (actual_len != hex_lengths[i])
            {
                batch_result->error_codes[i] = TECC_CRYPTO_INVALID_PARAM;
                if (batch_result->overall_error_code == TECC_CRYPTO_SUCCESS)
                {
                    snprintf(batch_result->overall_error_msg, sizeof(batch_result->overall_error_msg),
                             "Item %zu: Hex length mismatch (expected %zu, got %zu)",
                             i, hex_lengths[i], actual_len);
                    batch_result->overall_error_code = TECC_CRYPTO_INVALID_PARAM;
                }
                continue;
            }
        }

        /* Perform single decryption with scheduler */
        /* Note: conn is NULL for internal batch processing */
        single_result = tecc_unified_decrypt_hex_internal_with_scheduler(NULL,
                                                                        scheduler,
                                                                        hex_string,
                                                                        table_name,
                                                                        column_name,
                                                                        NULL,
                                                                        false,   /* supports_he_add */
                                                                        false);  /* supports_he_mul */

        if (single_result && single_result->error_code == TECC_CRYPTO_SUCCESS)
        {
            /* Store successful result */
            batch_result->plaintexts[i] = single_result->plaintext;
            batch_result->plaintext_lengths[i] = single_result->plain_len;
            batch_result->error_codes[i] = TECC_CRYPTO_SUCCESS;

            /* Transfer ownership of plaintext to batch result */
            single_result->plaintext = NULL;

            /* Success will be counted in batch statistics at the end */
        }
        else
        {
            /* Record error */
            if (single_result)
            {
                batch_result->error_codes[i] = single_result->error_code;
                if (batch_result->overall_error_code == TECC_CRYPTO_SUCCESS)
                {
                    snprintf(batch_result->overall_error_msg, sizeof(batch_result->overall_error_msg),
                             "Item %zu: %s", i, single_result->error_msg);
                    batch_result->overall_error_code = single_result->error_code;
                }

                /* Free plaintext if it was allocated but decryption failed */
                if (single_result->plaintext)
                {
                    /* Securely wipe plaintext data before freeing */
                    if (single_result->plain_len > 0 && single_result->plain_len <= 65536)
                        memset(single_result->plaintext, 0, single_result->plain_len);
                    pg_free(single_result->plaintext);
                    single_result->plaintext = NULL;
                }
            }
            else
            {
                batch_result->error_codes[i] = TECC_CRYPTO_BATCH_ERROR;
                if (batch_result->overall_error_code == TECC_CRYPTO_SUCCESS)
                {
                    snprintf(batch_result->overall_error_msg, sizeof(batch_result->overall_error_msg),
                             "Item %zu: Decryption failed", i);
                    batch_result->overall_error_code = TECC_CRYPTO_BATCH_ERROR;
                }
            }
        }

        /* Free single result structure */
        if (single_result)
        {
            pg_free(single_result);
        }
    }

    /* Calculate actual success count */
    success_count = 0;
    error_count = 0;
    for (i = 0; i < count; i++)
    {
        if (batch_result->error_codes[i] == TECC_CRYPTO_SUCCESS)
        {
            success_count++;
        }
        else
        {
            error_count++;
        }
    }

    /* Update statistics */
    scheduler->decryption_count += success_count;
    scheduler->error_count += error_count;

    /* Update batch-specific statistics */
    scheduler->batch_decryption_count++;
    scheduler->total_batch_items_processed += count;
    scheduler->successful_batch_items += success_count;
    scheduler->batch_error_count += error_count;

    TECC_SCHEDULER_DEBUG("Batch decryption completed (success=%zu/%zu)",
                         success_count, count);

    return batch_result;
}

/*
 * Batch decryption interface
 */
TeccBatchDecryptResult *
tecc_unified_batch_decrypt_hex_strings(
    const char **hex_strings,
    const size_t *hex_lengths,
    size_t count,
    const char *table_name,
    const char *column_name)
{
    /* This function is deprecated - use tecc_unified_batch_decrypt_hex_strings_with_conn instead */
    TECC_SCHEDULER_ERROR("tecc_unified_batch_decrypt_hex_strings is deprecated, use tecc_unified_batch_decrypt_hex_strings_with_conn with connection parameter");
    return NULL;
}
TeccBatchDecryptResult *
tecc_unified_batch_decrypt_hex_strings_with_conn(
    PGconn *conn,
    const char **hex_strings,
    const size_t *hex_lengths,
    size_t count,
    const char *table_name,
    const char *column_name)
{
    TeccCryptoScheduler *scheduler;

    /* Validate parameters */
    if (conn == NULL || !hex_strings || !hex_lengths || count == 0 || !table_name || !column_name)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_unified_batch_decrypt_hex_strings_with_conn");
        return NULL;
    }

    /* Get scheduler from connection state */
    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to get crypto scheduler from connection");
        return NULL;
    }

    /* Use the internal implementation with scheduler */
    return tecc_unified_batch_decrypt_hex_strings_internal(scheduler, hex_strings, hex_lengths, count,
                                                          table_name, column_name);
}

/*
 * Free batch encryption result
 */
void
tecc_unified_free_batch_encrypt_result(TeccBatchEncryptResult *result)
{
    size_t i;

    if (!result)
        return;

    /* Free individual cipherdata buffers and hex strings */
    if (result->cipherdata_buffers)
    {
        for (i = 0; i < result->count; i++)
        {
            if (result->cipherdata_buffers[i])
            {
                /* Free the buffer data if it exists */
                if (result->cipherdata_buffers[i]->data)
                {
                    pg_free(result->cipherdata_buffers[i]->data);
                }
                pg_free(result->cipherdata_buffers[i]);
            }
        }
        pg_free(result->cipherdata_buffers);
    }

    if (result->hex_strings)
    {
        for (i = 0; i < result->count; i++)
        {
            if (result->hex_strings[i])
            {
                pg_free(result->hex_strings[i]);
            }
        }
        pg_free(result->hex_strings);
    }

    if (result->error_codes)
    {
        pg_free(result->error_codes);
    }

    /* Free the result structure itself */
    pg_free(result);
}

/*
 * Free batch decryption result
 */
void
tecc_unified_free_batch_decrypt_result(TeccBatchDecryptResult *result)
{
    size_t i;

    if (!result)
        return;

    /* Free individual plaintexts */
    if (result->plaintexts)
    {
        for (i = 0; i < result->count; i++)
        {
            if (result->plaintexts[i])
            {
                /* Securely wipe plaintext data before freeing */
                if (result->plaintext_lengths && result->plaintext_lengths[i] > 0)
                    memset(result->plaintexts[i], 0, result->plaintext_lengths[i]);
                pg_free(result->plaintexts[i]);
            }
        }
        pg_free(result->plaintexts);
    }

    if (result->plaintext_lengths)
    {
        pg_free(result->plaintext_lengths);
    }

    if (result->error_codes)
    {
        pg_free(result->error_codes);
    }

    /* Free the result structure itself */
    pg_free(result);
}

/*
 * Get batch processing statistics (deprecated - use thread-safe version)
 */
int
tecc_get_batch_processing_stats(
    size_t *total_processed,
    size_t *success_count,
    size_t *error_count,
    double *average_time_per_item)
{
    /* This function is deprecated - it cannot get scheduler without connection */
    TECC_SCHEDULER_ERROR("tecc_get_batch_processing_stats is deprecated, use tecc_get_batch_processing_stats_with_conn with connection parameter");

    /* Return default values */
    if (total_processed)
        *total_processed = 0;
    if (success_count)
        *success_count = 0;
    if (error_count)
        *error_count = 0;
    if (average_time_per_item)
        *average_time_per_item = 0.0;

    return TECC_CRYPTO_SYSTEM_ERROR;
}

/*
 * Get batch processing statistics with connection parameter (thread-safe version)
 */
int
tecc_get_batch_processing_stats_with_conn(
    PGconn *conn,
    size_t *total_processed,
    size_t *success_count,
    size_t *error_count,
    double *average_time_per_item)
{
    TeccCryptoScheduler *scheduler;

    /* Validate parameters */
    if (conn == NULL)
    {
        TECC_SCHEDULER_ERROR("NULL connection passed to tecc_get_batch_processing_stats_with_conn");
        return TECC_CRYPTO_INVALID_PARAM;
    }

    /* Get scheduler from connection */
    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to get crypto scheduler from connection");
        return TECC_CRYPTO_SYSTEM_ERROR;
    }

    /* Return batch-specific statistics from scheduler */
    if (total_processed)
        *total_processed = scheduler->total_batch_items_processed;
    if (success_count)
        *success_count = scheduler->successful_batch_items;
    if (error_count)
        *error_count = scheduler->batch_error_count;
    if (average_time_per_item)
        *average_time_per_item = 0.0;  /* Not implemented yet - would require timing data */

    TECC_SCHEDULER_DEBUG("Batch statistics: total=%zu, success=%zu, errors=%d, batch_ops=%d",
                         scheduler->total_batch_items_processed,
                         scheduler->successful_batch_items,
                         scheduler->batch_error_count,
                         scheduler->batch_encryption_count + scheduler->batch_decryption_count);

    return TECC_CRYPTO_SUCCESS;
}

/*
 * Get detailed batch error statistics
 */
int
tecc_get_batch_error_details(
    PGconn *conn,
    int *total_errors,
    int *invalid_param_errors,
    int *memory_errors,
    int *format_errors,
    int *system_errors,
    int *batch_errors)
{
    TeccCryptoScheduler *scheduler;

    /* Validate parameters */
    if (conn == NULL)
    {
        TECC_SCHEDULER_ERROR("NULL connection passed to tecc_get_batch_error_details");
        return TECC_CRYPTO_INVALID_PARAM;
    }

    /* Get scheduler from connection */
    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to get crypto scheduler from connection");
        return TECC_CRYPTO_SYSTEM_ERROR;
    }

    /* Return batch-specific error statistics */
    if (total_errors)
        *total_errors = scheduler->batch_error_count;
    if (invalid_param_errors)
        *invalid_param_errors = 0;  /* Not tracked separately yet */
    if (memory_errors)
        *memory_errors = 0;         /* Not tracked separately yet */
    if (format_errors)
        *format_errors = 0;         /* Not tracked separately yet */
    if (system_errors)
        *system_errors = 0;         /* Not tracked separately yet */
    if (batch_errors)
        *batch_errors = scheduler->batch_error_count;  /* Same as total for now */

    TECC_SCHEDULER_DEBUG("Batch error details: total=%d, batch_ops=%d",
                         scheduler->batch_error_count,
                         scheduler->batch_encryption_count + scheduler->batch_decryption_count);

    return TECC_CRYPTO_SUCCESS;
}

/*
 * Optimize batch size based on performance metrics
 */
size_t
tecc_optimize_batch_size(
    size_t data_size,
    int algorithm_type,
    size_t available_memory)
{
    size_t optimal_batch_size;
    size_t estimated_memory_per_item = 0;

    /* Basic optimization logic */
    if (data_size == 0)
        return 100;  /* Default batch size for small data */

    /* Estimate memory usage per item */

    switch (algorithm_type)
    {
        case TECC_ALGO_RND:
            estimated_memory_per_item = data_size + 64;  /* SM4-CBC with padding and HMAC */
            break;
        case TECC_ALGO_DET:
            estimated_memory_per_item = data_size + 16;  /* Deterministic encryption */
            break;
        case TECC_ALGO_OPE:
            estimated_memory_per_item = data_size + 32;  /* Order-preserving encryption */
            break;
        case TECC_ALGO_SEARCH:
            estimated_memory_per_item = data_size * 2;   /* Searchable encryption */
            break;
        default:
            estimated_memory_per_item = data_size + 128; /* Conservative estimate */
    }

    /* Calculate optimal batch size based on available memory */
    if (estimated_memory_per_item == 0)
        return 100;

    optimal_batch_size = available_memory / estimated_memory_per_item;

    /* Apply limits */
    if (optimal_batch_size < 10)
        optimal_batch_size = 10;
    if (optimal_batch_size > 1000)
        optimal_batch_size = 1000;

    return optimal_batch_size;
}

/*
 * Smart batch size optimization with performance history
 */
size_t
tecc_smart_optimize_batch_size(
    PGconn *conn,
    size_t data_size,
    int algorithm_type,
    size_t available_memory)
{
    TeccCryptoScheduler *scheduler;
    size_t base_batch_size;
    size_t adjusted_batch_size;

    /* Validate parameters */
    if (conn == NULL)
    {
        TECC_SCHEDULER_ERROR("NULL connection passed to tecc_smart_optimize_batch_size");
        return tecc_optimize_batch_size(data_size, algorithm_type, available_memory);
    }

    /* Get scheduler from connection */
    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to get crypto scheduler from connection");
        return tecc_optimize_batch_size(data_size, algorithm_type, available_memory);
    }

    /* Get base batch size from basic optimization */
    base_batch_size = tecc_optimize_batch_size(data_size, algorithm_type, available_memory);

    /* Adjust based on batch error rate if we have enough batch statistics */
    if (scheduler->total_batch_items_processed > 100)
    {
        double batch_error_rate = (double)scheduler->batch_error_count / scheduler->total_batch_items_processed;

        /* Reduce batch size if batch error rate is high */
        if (batch_error_rate > 0.1)  /* 10% error rate */
        {
            adjusted_batch_size = base_batch_size / 2;
            TECC_SCHEDULER_DEBUG("High batch error rate (%.2f%%), reducing batch size from %zu to %zu",
                                batch_error_rate * 100, base_batch_size, adjusted_batch_size);
        }
        else if (batch_error_rate > 0.05)  /* 5% error rate */
        {
            adjusted_batch_size = base_batch_size * 3 / 4;
            TECC_SCHEDULER_DEBUG("Moderate batch error rate (%.2f%%), reducing batch size from %zu to %zu",
                                batch_error_rate * 100, base_batch_size, adjusted_batch_size);
        }
        else
        {
            /* Low batch error rate, can increase batch size */
            adjusted_batch_size = base_batch_size * 5 / 4;
            TECC_SCHEDULER_DEBUG("Low batch error rate (%.2f%%), increasing batch size from %zu to %zu",
                                batch_error_rate * 100, base_batch_size, adjusted_batch_size);
        }
    }
    else
    {
        /* Not enough batch statistics, use base batch size */
        adjusted_batch_size = base_batch_size;
        TECC_SCHEDULER_DEBUG("Insufficient batch statistics, using base batch size: %zu", base_batch_size);
    }

    /* Apply limits */
    if (adjusted_batch_size < 10)
        adjusted_batch_size = 10;
    if (adjusted_batch_size > 1000)
        adjusted_batch_size = 1000;

    TECC_SCHEDULER_DEBUG("Smart batch size optimization: data_size=%zu, algo=%d, mem=%zu -> batch=%zu",
                        data_size, algorithm_type, available_memory, adjusted_batch_size);

    return adjusted_batch_size;
}

/* End - ReqID:TECC4.0 - Batch processing implementation for import/export module */
/* End - ReqID:TECC4.0 */
/* ==================== Stream Processing Implementation ==================== */
/* Begin - ReqID:TECC4.0 - Stream processing implementation for import/export module */

/*
 * Create stream crypto context
 */
TeccStreamCryptoContext *
tecc_create_stream_crypto_context(
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name)
{
    TeccCryptoScheduler *scheduler = NULL;

    /* Validate parameters */
    if (!table_name || !column_name || !kek_name || !dek_name)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_create_stream_crypto_context");
        return NULL;
    }

    /* Check if scheduler is available */
    if (!scheduler)
    {
        TECC_SCHEDULER_ERROR("Crypto scheduler not initialized");
        return NULL;
    }

    /* Call internal implementation with global scheduler */
    return tecc_create_stream_crypto_context_internal(scheduler, table_name, column_name, kek_name, dek_name);
}

/*
 * Internal stream crypto context creation with explicit scheduler
 */
static TeccStreamCryptoContext *
tecc_create_stream_crypto_context_internal(
    TeccCryptoScheduler *scheduler,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name)
{
    TeccStreamCryptoContext *context = NULL;

    /* Validate parameters */
    if (!scheduler || !table_name || !column_name || !kek_name || !dek_name)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_create_stream_crypto_context_internal");
        return NULL;
    }

    /* Allocate context */
    context = (TeccStreamCryptoContext *)pg_malloc(sizeof(TeccStreamCryptoContext));
    if (!context)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate stream crypto context");
        return NULL;
    }

    /* Initialize context */
    memset(context, 0, sizeof(TeccStreamCryptoContext));

    /* Store scheduler reference */
    context->scheduler = scheduler;

    /* Create base crypto context */
    context->base_context = tecc_create_crypto_context_with_pool(
        table_name, column_name, kek_name, dek_name);
    if (!context->base_context)
    {
        pg_free(context);
        TECC_SCHEDULER_ERROR("Failed to create base crypto context");
        return NULL;
    }

    /* Initialize other fields */
    context->processed_count = 0;
    context->initialized = true;
    context->algorithm_type = TECC_ALGO_RND; /* Default algorithm: RND (SM4-CBC with HMAC) */
    context->collation_oid = InvalidOid; /* Default: no collation */
    context->operator_type = TECC_OP_EQUAL; /* Default operator */

    /* Initialize performance statistics */
    gettimeofday(&context->start_time, NULL);
    gettimeofday(&context->last_operation_time, NULL);
    context->total_encrypted_bytes = 0;
    context->total_decrypted_bytes = 0;
    context->encryption_count = 0;
    context->decryption_count = 0;
    context->batch_operations = 0;
    context->single_operations = 0;
    context->avg_encryption_time = 0.0;
    context->avg_decryption_time = 0.0;
    context->memory_saved = 0;

    /* Create memory pool for stream processing */
    context->stream_pool = tecc_stream_memory_pool_create(64); /* Default: 64 buffers */
    if (!context->stream_pool)
    {
        TECC_SCHEDULER_WARNING("Failed to create memory pool for stream context, continuing without pool");
        /* Continue without memory pool - not a fatal error */
    }

    /* Allocate key ID */
    if (dek_name)
    {
        context->key_id = pg_malloc(strlen(dek_name) + 1);
        if (context->key_id)
        {
            strcpy(context->key_id, dek_name);
        }
    }

    TECC_SCHEDULER_DEBUG("Created stream crypto context for table '%s', column '%s' with scheduler %p",
                        table_name, column_name, scheduler);

    return context;
}

/*
 * Create stream crypto context with connection parameter (thread-safe version)
 */
TeccStreamCryptoContext *
tecc_create_stream_crypto_context_with_conn(
    PGconn *conn,
    const char *table_name,
    const char *column_name,
    const char *kek_name,
    const char *dek_name)
{
    TeccCryptoScheduler *scheduler;

    /* Validate parameters */
    if (conn == NULL || !table_name || !column_name || !kek_name || !dek_name)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_create_stream_crypto_context_with_conn");
        return NULL;
    }

    /* Get scheduler from connection state */
    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to get crypto scheduler from connection");
        return NULL;
    }

    /* Create context with connection-specific scheduler */
    return tecc_create_stream_crypto_context_internal(scheduler, table_name, column_name, kek_name, dek_name);
}

/*
 * Stream encryption of single data item
 */
TeccUnifiedEncryptResult *
tecc_stream_encrypt_data(
    TeccStreamCryptoContext *context,
    const char *plaintext,
    size_t plaintext_len)
{
    TeccUnifiedEncryptResult *result = NULL;
    TeccCryptoScheduler *scheduler = NULL;
    struct timeval start_time, end_time;
    double elapsed_ms = 0.0;

    /* Validate parameters */
    if (!context || !context->initialized || !plaintext)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_stream_encrypt_data");
        return NULL;
    }

    /* Start timer for performance measurement */
    TECC_START_TIMER(start_time);

    /* Check if base context is available */
    if (!context->base_context)
    {
        TECC_SCHEDULER_ERROR("Base crypto context not available in stream context");
        return NULL;
    }

    /* Get scheduler from context, fall back to global scheduler if not set */
    scheduler = context->scheduler;
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Stream encryption requires connection-specific scheduler, global scheduler not available");
        return NULL;
    }

    /* Perform encryption based on algorithm type */
    switch (context->algorithm_type)
    {
        case TECC_ALGO_RND:
        case TECC_ALGO_DET:
        case TECC_ALGO_OPE:
            /* Use standard encryption with scheduler */
            result = tecc_unified_encrypt_string_internal_with_scheduler(
                scheduler,
                plaintext,
                context->base_context->table_name,
                context->base_context->column_name,
                NULL, /* schema_name - stream 上下文无 schema */
                context->base_context->kek_name,
                context->base_context->dek_name,
                context->collation_oid,
                NULL, NULL, NULL, /* tecc_state, col_meta_in, external_context not available in stream context */
                false, false); /* supports_he_add, supports_he_mul */
            break;

        case TECC_ALGO_SEARCH:
            /* SEARCH encryption + RND for storage; stream context has no connection */
            result = tecc_encrypt_search_with_rnd(
                NULL,  /* No connection available in stream context */
                scheduler,
                plaintext,
                context->base_context->table_name,
                context->base_context->column_name,
                context->base_context->kek_name,
                context->base_context->dek_name,
                context->operator_type);
            break;

        default:
            TECC_SCHEDULER_ERROR("Unsupported algorithm type %d in stream encryption",
                               context->algorithm_type);
            return NULL;
    }

    /* End timer and update performance statistics */
    TECC_END_TIMER(start_time, end_time, elapsed_ms);

    /* Update statistics */
    if (result && result->error_code == TECC_CRYPTO_SUCCESS)
    {
        context->processed_count++;

        /* Update performance statistics */
        tecc_stream_update_performance_stats(context, true, plaintext_len, elapsed_ms, false);
    }

    return result;
}

/*
 * Stream decryption of single data item
 */
TeccUnifiedDecryptResult *
tecc_stream_decrypt_data(
    TeccStreamCryptoContext *context,
    const char *hex_string,
    size_t hex_len)
{
    TeccUnifiedDecryptResult *result = NULL;
    TeccCryptoScheduler *scheduler = NULL;
    struct timeval start_time, end_time;
    double elapsed_ms = 0.0;

    /* Validate parameters */
    if (!context || !context->initialized || !hex_string)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_stream_decrypt_data");
        return NULL;
    }

    /* Start timer for performance measurement */
    TECC_START_TIMER(start_time);

    /* Check if base context is available */
    if (!context->base_context)
    {
        TECC_SCHEDULER_ERROR("Base crypto context not available in stream context");
        return NULL;
    }

    /* Get scheduler from context, fall back to global scheduler if not set */
    scheduler = context->scheduler;
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Stream decryption requires connection-specific scheduler, global scheduler not available");
        return NULL;
    }

    /* Perform decryption with scheduler */
    /* Note: conn is NULL for stream processing */
    result = tecc_unified_decrypt_hex_internal_with_scheduler(
        NULL,
        scheduler,
        hex_string,
        context->base_context->table_name,
        context->base_context->column_name,
        NULL,
        false,   /* supports_he_add */
        false);  /* supports_he_mul */

    /* End timer and update performance statistics */
    TECC_END_TIMER(start_time, end_time, elapsed_ms);

    /* Update statistics */
    if (result && result->error_code == TECC_CRYPTO_SUCCESS)
    {
        context->processed_count++;

        /* Update performance statistics */
        tecc_stream_update_performance_stats(context, false, hex_len, elapsed_ms, false);
    }

    return result;
}

/*
 * Batch encryption using stream context
 */
TeccBatchEncryptResult *
tecc_stream_batch_encrypt_data(
    TeccStreamCryptoContext *context,
    const char **plaintexts,
    const size_t *plaintext_lengths,
    size_t count)
{
    TeccBatchEncryptResult *batch_result = NULL;
    size_t i;
    struct timeval start_time, end_time;
    double elapsed_ms = 0.0;
    size_t total_data_size = 0;

    /* Validate parameters */
    if (!context || !context->initialized || !plaintexts || !plaintext_lengths || count == 0)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_stream_batch_encrypt_data");
        return NULL;
    }

    /* Start timer for performance measurement */
    TECC_START_TIMER(start_time);

    /* Calculate total data size for performance statistics */
    for (i = 0; i < count; i++)
    {
        total_data_size += plaintext_lengths[i];
    }

    /* Check if base context is available */
    if (!context->base_context)
    {
        TECC_SCHEDULER_ERROR("Base crypto context not available in stream context");
        return NULL;
    }

    /* Allocate batch result */
    batch_result = (TeccBatchEncryptResult *)pg_malloc(sizeof(TeccBatchEncryptResult));
    if (!batch_result)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate batch encrypt result");
        return NULL;
    }

    memset(batch_result, 0, sizeof(TeccBatchEncryptResult));
    batch_result->count = count;
    batch_result->overall_error_code = TECC_CRYPTO_SUCCESS;

    /* Allocate and initialize arrays using memory pool if available */
    if (context->stream_pool)
    {
        /* Try to allocate arrays from memory pool */
        batch_result->cipherdata_buffers = (TeccCryptoBuffer **)
            tecc_stream_memory_pool_alloc(context->stream_pool, count * sizeof(TeccCryptoBuffer *));
        batch_result->hex_strings = (char **)
            tecc_stream_memory_pool_alloc(context->stream_pool, count * sizeof(char *));
        batch_result->error_codes = (int *)
            tecc_stream_memory_pool_alloc(context->stream_pool, count * sizeof(int));
    }
    else
    {
        /* Fall back to regular allocation */
        batch_result->cipherdata_buffers = (TeccCryptoBuffer **)pg_malloc0(count * sizeof(TeccCryptoBuffer *));
        batch_result->hex_strings = (char **)pg_malloc0(count * sizeof(char *));
        batch_result->error_codes = (int *)pg_malloc0(count * sizeof(int));
    }

    if (!batch_result->cipherdata_buffers || !batch_result->hex_strings || !batch_result->error_codes)
    {
        tecc_unified_free_batch_encrypt_result(batch_result);
        TECC_SCHEDULER_ERROR("Failed to allocate arrays for batch encrypt result");
        return NULL;
    }
    memset(batch_result->error_codes, 0, count * sizeof(int));

    /* Process each item */
    for (i = 0; i < count; i++)
    {
        TeccUnifiedEncryptResult *single_result;

        single_result = tecc_stream_encrypt_data(context, plaintexts[i], plaintext_lengths[i]);

        if (single_result)
        {
            batch_result->cipherdata_buffers[i] = single_result->cipherdata_buffer;
            batch_result->hex_strings[i] = single_result->hex_string;
            batch_result->error_codes[i] = single_result->error_code;

            /* Free result structure but keep buffers */
            single_result->cipherdata_buffer = NULL;
            single_result->hex_string = NULL;
            tecc_unified_free_encrypt_result(single_result);

            if (batch_result->error_codes[i] != TECC_CRYPTO_SUCCESS)
            {
                batch_result->overall_error_code = TECC_CRYPTO_BATCH_ERROR;
                snprintf(batch_result->overall_error_msg, sizeof(batch_result->overall_error_msg),
                        "Batch encryption failed at item %zu", i);
            }
        }
        else
        {
            batch_result->error_codes[i] = TECC_CRYPTO_BATCH_ERROR;
            batch_result->overall_error_code = TECC_CRYPTO_BATCH_ERROR;
            snprintf(batch_result->overall_error_msg, sizeof(batch_result->overall_error_msg),
                    "Batch encryption failed at item %zu (null result)", i);
        }
    }

    /* End timer and update performance statistics */
    TECC_END_TIMER(start_time, end_time, elapsed_ms);

    /* Update processed count */
    context->processed_count += count;

    /* Update performance statistics */
    tecc_stream_update_performance_stats(context, true, total_data_size, elapsed_ms, true);

    return batch_result;
}

/*
 * Batch decryption using stream context
 */
TeccBatchDecryptResult *
tecc_stream_batch_decrypt_data(
    TeccStreamCryptoContext *context,
    const char **hex_strings,
    const size_t *hex_lengths,
    size_t count)
{
    TeccBatchDecryptResult *batch_result = NULL;
    size_t i;
    struct timeval start_time, end_time;
    double elapsed_ms = 0.0;
    size_t total_data_size = 0;

    /* Validate parameters */
    if (!context || !context->initialized || !hex_strings || !hex_lengths || count == 0)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_stream_batch_decrypt_data");
        return NULL;
    }

    /* Start timer for performance measurement */
    TECC_START_TIMER(start_time);

    /* Calculate total data size for performance statistics */
    for (i = 0; i < count; i++)
    {
        total_data_size += hex_lengths[i];
    }

    /* Check if base context is available */
    if (!context->base_context)
    {
        TECC_SCHEDULER_ERROR("Base crypto context not available in stream context");
        return NULL;
    }

    /* Allocate batch result */
    batch_result = (TeccBatchDecryptResult *)pg_malloc(sizeof(TeccBatchDecryptResult));
    if (!batch_result)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate batch decrypt result");
        return NULL;
    }

    memset(batch_result, 0, sizeof(TeccBatchDecryptResult));
    batch_result->count = count;
    batch_result->overall_error_code = TECC_CRYPTO_SUCCESS;

    /* Allocate and initialize arrays */
    batch_result->plaintexts = (char **)pg_malloc0(count * sizeof(char *));
    batch_result->plaintext_lengths = (size_t *)pg_malloc0(count * sizeof(size_t));
    batch_result->error_codes = (int *)pg_malloc0(count * sizeof(int));

    if (!batch_result->plaintexts || !batch_result->plaintext_lengths || !batch_result->error_codes)
    {
        tecc_unified_free_batch_decrypt_result(batch_result);
        TECC_SCHEDULER_ERROR("Failed to allocate arrays for batch decrypt result");
        return NULL;
    }
    memset(batch_result->error_codes, 0, count * sizeof(int));

    /* Process each item */
    for (i = 0; i < count; i++)
    {
        TeccUnifiedDecryptResult *single_result;

        single_result = tecc_stream_decrypt_data(context, hex_strings[i], hex_lengths[i]);

        if (single_result)
        {
            batch_result->plaintexts[i] = single_result->plaintext;
            batch_result->plaintext_lengths[i] = single_result->plain_len;
            batch_result->error_codes[i] = single_result->error_code;

            /* Free result structure but keep plaintext */
            single_result->plaintext = NULL;
            tecc_unified_free_decrypt_result(single_result);

            if (batch_result->error_codes[i] != TECC_CRYPTO_SUCCESS)
            {
                batch_result->overall_error_code = TECC_CRYPTO_BATCH_ERROR;
                snprintf(batch_result->overall_error_msg, sizeof(batch_result->overall_error_msg),
                        "Batch decryption failed at item %zu", i);
            }
        }
        else
        {
            batch_result->error_codes[i] = TECC_CRYPTO_BATCH_ERROR;
            batch_result->overall_error_code = TECC_CRYPTO_BATCH_ERROR;
            snprintf(batch_result->overall_error_msg, sizeof(batch_result->overall_error_msg),
                    "Batch decryption failed at item %zu (null result)", i);
        }
    }

    /* End timer and update performance statistics */
    TECC_END_TIMER(start_time, end_time, elapsed_ms);

    /* Update processed count */
    context->processed_count += count;

    /* Update performance statistics */
    tecc_stream_update_performance_stats(context, false, total_data_size, elapsed_ms, true);

    return batch_result;
}

/*
 * Set algorithm type for stream context
 */
int
tecc_stream_set_algorithm_type(
    TeccStreamCryptoContext *context,
    int algorithm_type)
{
    /* Validate parameters */
    if (!context || !context->initialized)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_stream_set_algorithm_type");
        return TECC_CRYPTO_INVALID_PARAM;
    }

    /* Validate algorithm type */
    if (algorithm_type < TECC_ALGO_RND || algorithm_type > TECC_ALGO_SEARCH)
    {
        TECC_SCHEDULER_ERROR("Invalid algorithm type: %d", algorithm_type);
        return TECC_CRYPTO_INVALID_PARAM;
    }

    context->algorithm_type = algorithm_type;
    return TECC_CRYPTO_SUCCESS;
}

/*
 * Set collation for stream context
 */
int
tecc_stream_set_collation(
    TeccStreamCryptoContext *context,
    Oid collation_oid)
{
    /* Validate parameters */
    if (!context || !context->initialized)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_stream_set_collation");
        return TECC_CRYPTO_INVALID_PARAM;
    }

    context->collation_oid = collation_oid;
    return TECC_CRYPTO_SUCCESS;
}

/*
 * Set operator type for stream context (SEARCH algorithm)
 */
int
tecc_stream_set_operator_type(
    TeccStreamCryptoContext *context,
    TeccOperatorType operator_type)
{
    /* Validate parameters */
    if (!context || !context->initialized)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_stream_set_operator_type");
        return TECC_CRYPTO_INVALID_PARAM;
    }

    /* Validate operator type */
    if (operator_type < TECC_OP_EQUAL || operator_type > TECC_OP_LIKE)
    {
        TECC_SCHEDULER_ERROR("Invalid operator type: %d", operator_type);
        return TECC_CRYPTO_INVALID_PARAM;
    }

    context->operator_type = operator_type;
    return TECC_CRYPTO_SUCCESS;
}

/*
 * Reset stream context (for reuse)
 */
int
tecc_stream_reset_context(
    TeccStreamCryptoContext *context)
{
    /* Validate parameters */
    if (!context || !context->initialized)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_stream_reset_context");
        return TECC_CRYPTO_INVALID_PARAM;
    }

    /* Reset statistics */
    context->processed_count = 0;

    /* Reset algorithm settings to defaults */
    context->algorithm_type = TECC_ALGO_RND;
    context->collation_oid = InvalidOid;
    context->operator_type = TECC_OP_EQUAL;

    return TECC_CRYPTO_SUCCESS;
}

/*
 * Create stream memory pool
 */
TeccStreamMemoryPool *
tecc_stream_memory_pool_create(int max_buffers)
{
    TeccStreamMemoryPool *pool;

    if (max_buffers <= 0)
    {
        TECC_SCHEDULER_ERROR("Invalid max_buffers parameter: %d", max_buffers);
        return NULL;
    }

    pool = (TeccStreamMemoryPool *)pg_malloc(sizeof(TeccStreamMemoryPool));
    if (!pool)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate memory pool");
        return NULL;
    }

    memset(pool, 0, sizeof(TeccStreamMemoryPool));
    pool->max_buffers = max_buffers;

    /* Allocate arrays */
    pool->buffers = (char **)pg_malloc0(max_buffers * sizeof(char *));
    pool->buffer_sizes = (size_t *)pg_malloc0(max_buffers * sizeof(size_t));

    if (!pool->buffers || !pool->buffer_sizes)
    {
        if (pool->buffers) pg_free(pool->buffers);
        if (pool->buffer_sizes) pg_free(pool->buffer_sizes);
        pg_free(pool);
        TECC_SCHEDULER_ERROR("Failed to allocate memory pool arrays");
        return NULL;
    }

    /* Initialize performance statistics */
    gettimeofday(&pool->creation_time, NULL);
    pool->total_allocations = 0;
    pool->total_allocation_size = 0;
    pool->peak_allocated = 0;
    pool->cache_hits = 0;
    pool->cache_misses = 0;

    TECC_SCHEDULER_DEBUG("Created stream memory pool with max_buffers=%d", max_buffers);
    return pool;
}

/*
 * Allocate buffer from stream memory pool
 */
char *
tecc_stream_memory_pool_alloc(TeccStreamMemoryPool *pool, size_t size)
{
    char *buffer;

    if (!pool || size == 0)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_stream_memory_pool_alloc");
        return NULL;
    }

    /* Check if we have space in the pool */
    if (pool->buffer_count >= pool->max_buffers)
    {
        /* Pool is full, allocate directly */
        buffer = (char *)pg_malloc(size);
        if (buffer)
        {
            TECC_SCHEDULER_DEBUG("Pool full, allocated buffer directly: size=%zu", size);
            /* Update statistics for direct allocation */
            pool->total_allocations++;
            pool->total_allocation_size += size;
            pool->cache_misses++;
        }
        return buffer;
    }

    /* Allocate buffer and add to pool */
    buffer = (char *)pg_malloc(size);
    if (!buffer)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate buffer of size %zu", size);
        return NULL;
    }

    /* Add to pool */
    pool->buffers[pool->buffer_count] = buffer;
    pool->buffer_sizes[pool->buffer_count] = size;
    pool->buffer_count++;
    pool->total_allocated += size;

    /* Update performance statistics */
    pool->total_allocations++;
    pool->total_allocation_size += size;
    pool->cache_hits++;

    /* Update peak allocated memory */
    if (pool->total_allocated > pool->peak_allocated)
    {
        pool->peak_allocated = pool->total_allocated;
    }

    TECC_SCHEDULER_DEBUG("Allocated buffer from pool: size=%zu, pool_count=%d, total_allocated=%zu",
                        size, pool->buffer_count, pool->total_allocated);
    return buffer;
}

/*
 * Destroy stream memory pool
 */
void
tecc_stream_memory_pool_destroy(TeccStreamMemoryPool *pool)
{
    int i;

    if (!pool)
        return;

    /* Free all buffers in the pool */
    for (i = 0; i < pool->buffer_count; i++)
    {
        if (pool->buffers[i])
        {
            pg_free(pool->buffers[i]);
        }
    }

    /* Free arrays */
    if (pool->buffers)
        pg_free(pool->buffers);
    if (pool->buffer_sizes)
        pg_free(pool->buffer_sizes);

    /* Free pool structure */
    pg_free(pool);

    TECC_SCHEDULER_DEBUG("Destroyed stream memory pool");
}

/*
 * Destroy stream context
 */
void
tecc_stream_destroy_context(
    TeccStreamCryptoContext *context)
{
    if (!context)
        return;

    /* Destroy base context */
    if (context->base_context)
    {
        tecc_destroy_crypto_context(context->base_context);
    }

    /* Free key ID */
    if (context->key_id)
    {
        pg_free(context->key_id);
    }

    /* Destroy stream pool if allocated */
    if (context->stream_pool)
    {
        tecc_stream_memory_pool_destroy(context->stream_pool);
    }

    /* Free context itself */
    pg_free(context);
}

/* ==================== Stream Performance Statistics Implementation ==================== */
/* Begin - ReqID:TECC4.0 - Stream performance statistics implementation for import/export module */

/*
 * tecc_stream_get_performance_stats - Get stream context performance statistics
 */
int
tecc_stream_get_performance_stats(
    TeccStreamCryptoContext *context,
    size_t *total_processed,
    size_t *encryption_count,
    size_t *decryption_count,
    size_t *batch_operations,
    size_t *single_operations,
    double *avg_encryption_time,
    double *avg_decryption_time,
    size_t *memory_saved)
{
    if (!context || !total_processed || !encryption_count || !decryption_count ||
        !batch_operations || !single_operations || !avg_encryption_time ||
        !avg_decryption_time || !memory_saved)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_stream_get_performance_stats");
        return -1;
    }

    *total_processed = context->processed_count;
    *encryption_count = context->encryption_count;
    *decryption_count = context->decryption_count;
    *batch_operations = context->batch_operations;
    *single_operations = context->single_operations;
    *avg_encryption_time = context->avg_encryption_time;
    *avg_decryption_time = context->avg_decryption_time;
    *memory_saved = context->memory_saved;

    return 0;
}

/*
 * tecc_stream_get_memory_pool_stats - Get memory pool statistics
 */
int
tecc_stream_get_memory_pool_stats(
    TeccStreamCryptoContext *context,
    size_t *total_allocations,
    size_t *total_allocation_size,
    size_t *peak_allocated,
    size_t *cache_hits,
    size_t *cache_misses,
    double *cache_hit_rate)
{
    if (!context || !total_allocations || !total_allocation_size || !peak_allocated ||
        !cache_hits || !cache_misses || !cache_hit_rate)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_stream_get_memory_pool_stats");
        return -1;
    }

    if (!context->stream_pool)
    {
        /* No memory pool available */
        *total_allocations = 0;
        *total_allocation_size = 0;
        *peak_allocated = 0;
        *cache_hits = 0;
        *cache_misses = 0;
        *cache_hit_rate = 0.0;
        return 0;
    }

    *total_allocations = context->stream_pool->total_allocations;
    *total_allocation_size = context->stream_pool->total_allocation_size;
    *peak_allocated = context->stream_pool->peak_allocated;
    *cache_hits = context->stream_pool->cache_hits;
    *cache_misses = context->stream_pool->cache_misses;

    /* Calculate cache hit rate */
    if (context->stream_pool->cache_hits + context->stream_pool->cache_misses > 0)
    {
        *cache_hit_rate = (double)context->stream_pool->cache_hits /
                         (context->stream_pool->cache_hits + context->stream_pool->cache_misses) * 100.0;
    }
    else
    {
        *cache_hit_rate = 0.0;
    }

    return 0;
}

/*
 * tecc_stream_reset_statistics - Reset stream context statistics
 */
int
tecc_stream_reset_statistics(
    TeccStreamCryptoContext *context)
{
    if (!context)
    {
        TECC_SCHEDULER_ERROR("Invalid parameter to tecc_stream_reset_statistics");
        return -1;
    }

    /* Reset performance statistics */
    gettimeofday(&context->start_time, NULL);
    gettimeofday(&context->last_operation_time, NULL);
    context->total_encrypted_bytes = 0;
    context->total_decrypted_bytes = 0;
    context->encryption_count = 0;
    context->decryption_count = 0;
    context->batch_operations = 0;
    context->single_operations = 0;
    context->avg_encryption_time = 0.0;
    context->avg_decryption_time = 0.0;
    context->memory_saved = 0;

    /* Reset memory pool statistics if available */
    if (context->stream_pool)
    {
        context->stream_pool->total_allocations = 0;
        context->stream_pool->total_allocation_size = 0;
        context->stream_pool->peak_allocated = 0;
        context->stream_pool->cache_hits = 0;
        context->stream_pool->cache_misses = 0;
    }

    TECC_SCHEDULER_DEBUG("Reset performance statistics for stream context");
    return 0;
}

/*
 * tecc_stream_update_performance_stats - Update stream context performance statistics after operation
 */
int
tecc_stream_update_performance_stats(
    TeccStreamCryptoContext *context,
    bool is_encryption,
    size_t data_size,
    double operation_time_ms,
    bool is_batch)
{
    if (!context)
    {
        TECC_SCHEDULER_ERROR("Invalid parameter to tecc_stream_update_performance_stats");
        return -1;
    }

    /* Update last operation time */
    gettimeofday(&context->last_operation_time, NULL);

    /* Update processed count */
    context->processed_count++;

    /* Update encryption/decryption statistics */
    if (is_encryption)
    {
        context->encryption_count++;
        context->total_encrypted_bytes += data_size;

        /* Update average encryption time */
        if (context->encryption_count == 1)
        {
            context->avg_encryption_time = operation_time_ms;
        }
        else
        {
            context->avg_encryption_time = (context->avg_encryption_time * (context->encryption_count - 1) +
                                           operation_time_ms) / context->encryption_count;
        }
    }
    else
    {
        context->decryption_count++;
        context->total_decrypted_bytes += data_size;

        /* Update average decryption time */
        if (context->decryption_count == 1)
        {
            context->avg_decryption_time = operation_time_ms;
        }
        else
        {
            context->avg_decryption_time = (context->avg_decryption_time * (context->decryption_count - 1) +
                                           operation_time_ms) / context->decryption_count;
        }
    }

    /* Update operation type statistics */
    if (is_batch)
    {
        context->batch_operations++;
    }
    else
    {
        context->single_operations++;
    }

    /* Update memory saved estimation (simplified: assume 20% memory saving with stream optimization) */
    if (context->stream_pool && context->stream_pool->cache_hits > 0)
    {
        /* Estimate memory saved by reusing buffers from pool */
        size_t estimated_saved = context->stream_pool->total_allocation_size * 20 / 100;
        if (estimated_saved > context->memory_saved)
        {
            context->memory_saved = estimated_saved;
        }
    }

    TECC_SCHEDULER_DEBUG("Updated performance stats: %s, size=%zu, time=%.2fms, batch=%s",
                        is_encryption ? "encryption" : "decryption",
                        data_size, operation_time_ms, is_batch ? "yes" : "no");
    return 0;
}

/*
 * tecc_stream_compare_performance - Compare stream vs standard performance
 */
int
tecc_stream_compare_performance(
    TeccStreamCryptoContext *stream_context,
    size_t standard_operation_count,
    double standard_total_time_ms,
    double *performance_improvement,
    double *memory_saving_ratio)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    size_t stream_total_operations;
    double stream_avg_time;
    double standard_avg_time;

    if (!stream_context || !performance_improvement || !memory_saving_ratio)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_stream_compare_performance");
        return -1;
    }

    /* Calculate total operations in stream context */
    stream_total_operations = stream_context->encryption_count + stream_context->decryption_count;

    if (stream_total_operations == 0 || standard_operation_count == 0)
    {
        *performance_improvement = 0.0;
        *memory_saving_ratio = 0.0;
        return 0;
    }

    /* Calculate average time per operation for stream context */
    stream_avg_time = 0.0;
    if (stream_context->encryption_count > 0)
    {
        stream_avg_time += stream_context->avg_encryption_time * stream_context->encryption_count;
    }
    if (stream_context->decryption_count > 0)
    {
        stream_avg_time += stream_context->avg_decryption_time * stream_context->decryption_count;
    }
    stream_avg_time /= stream_total_operations;

    /* Calculate average time per operation for standard processing */
    standard_avg_time = standard_total_time_ms / standard_operation_count;

    /* Calculate performance improvement (positive means stream is faster) */
    if (standard_avg_time > 0)
    {
        *performance_improvement = ((standard_avg_time - stream_avg_time) / standard_avg_time) * 100.0;
    }
    else
    {
        *performance_improvement = 0.0;
    }

    /* Calculate memory saving ratio */
    if (stream_context->stream_pool && stream_context->stream_pool->total_allocation_size > 0)
    {
        /* Simplified: assume standard processing would allocate 30% more memory */
        size_t estimated_standard_memory = stream_context->stream_pool->total_allocation_size * 130 / 100;
        *memory_saving_ratio = ((double)(estimated_standard_memory - stream_context->stream_pool->total_allocated) /
                               estimated_standard_memory) * 100.0;
    }
    else
    {
        *memory_saving_ratio = 0.0;
    }

    TECC_SCHEDULER_DEBUG("Performance comparison: stream_avg=%.2fms, standard_avg=%.2fms, improvement=%.2f%%, memory_saving=%.2f%%",
                        stream_avg_time, standard_avg_time, *performance_improvement, *memory_saving_ratio);
    return 0;
}

/*
 * tecc_stream_generate_performance_report - Generate performance report
 */
char *
tecc_stream_generate_performance_report(
    TeccStreamCryptoContext *context)
{
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    struct timeval current_time;
    double total_time_sec;
    size_t total_processed, encryption_count, decryption_count, batch_ops, single_ops, memory_saved;
    double avg_enc_time, avg_dec_time;
    size_t total_allocations, total_allocation_size, peak_allocated, cache_hits, cache_misses;
    double cache_hit_rate;
    double bytes_per_second = 0.0;
    double rows_per_second = 0.0;

    if (!context)
    {
        TECC_SCHEDULER_ERROR("Invalid parameter to tecc_stream_generate_performance_report");
        return NULL;
    }

    /* Calculate total processing time */
    gettimeofday(&current_time, NULL);
    total_time_sec = (current_time.tv_sec - context->start_time.tv_sec) +
                     (current_time.tv_usec - context->start_time.tv_usec) / 1000000.0;

    /* Get performance statistics */
    tecc_stream_get_performance_stats(context, &total_processed, &encryption_count,
                                     &decryption_count, &batch_ops, &single_ops,
                                     &avg_enc_time, &avg_dec_time, &memory_saved);

    /* Get memory pool statistics */
    tecc_stream_get_memory_pool_stats(context, &total_allocations, &total_allocation_size,
                                     &peak_allocated, &cache_hits, &cache_misses, &cache_hit_rate);

    /* Calculate throughput */
    if (total_time_sec > 0)
    {
        bytes_per_second = (context->total_encrypted_bytes + context->total_decrypted_bytes) / total_time_sec;
        rows_per_second = total_processed / total_time_sec;
    }

    /* Generate report string */
    {
        char *report = pg_malloc(2048); /* Allocate enough space for report */
    if (!report)
    {
        TECC_SCHEDULER_ERROR("Failed to allocate memory for performance report");
        return NULL;
    }

    snprintf(report, 2048,
        "TECC Stream Optimization Performance Report\n"
        "===========================================\n"
        "Processing Time: %.2f seconds\n"
        "Total Processed: %zu items\n"
        "  - Encryptions: %zu (%.2f ms avg)\n"
        "  - Decryptions: %zu (%.2f ms avg)\n"
        "Operation Types:\n"
        "  - Batch Operations: %zu\n"
        "  - Single Operations: %zu\n"
        "Throughput:\n"
        "  - Bytes/Second: %.2f\n"
        "  - Rows/Second: %.2f\n"
        "Memory Pool Statistics:\n"
        "  - Total Allocations: %zu\n"
        "  - Total Allocation Size: %zu bytes\n"
        "  - Peak Allocated: %zu bytes\n"
        "  - Cache Hits: %zu\n"
        "  - Cache Misses: %zu\n"
        "  - Cache Hit Rate: %.2f%%\n"
        "  - Estimated Memory Saved: %zu bytes\n",
        total_time_sec,
        total_processed,
        encryption_count, avg_enc_time,
        decryption_count, avg_dec_time,
        batch_ops,
        single_ops,
        bytes_per_second,
        rows_per_second,
        total_allocations,
        total_allocation_size,
        peak_allocated,
        cache_hits,
        cache_misses,
        cache_hit_rate,
        memory_saved);

        return report;
    }
}

/* End - ReqID:TECC4.0 - Stream performance statistics implementation for import/export module */
/* End - ReqID:TECC4.0 - Stream processing implementation for import/export module */

/*
 * Unified encryption for WHERE clause comparisons.
 * Encrypts plaintext using only the algorithm needed for the operator:
 *   = / <> -> DET
 *   < / > / <= / >= -> OPE
 *   LIKE / ILIKE / NOT LIKE / NOT ILIKE -> SEARCH
 */
TeccUnifiedEncryptResult *
tecc_unified_encrypt_for_where(PGconn *conn,
                              const char *plaintext,
                              const char *table_name,
                              const char *column_name,
                              const char *schema_name,
                              const char *kek_name,
                              const char *dek_name,
                              TeccOperatorType operator_type,
                              char like_escape_char,
                              TeccCryptoContext *external_context)
{
    TeccCryptoScheduler *scheduler;
    TeccUnifiedEncryptResult *result = NULL;
    int algo_type;
    const char *algo_name;
    bool algo_fallback_allowed;

    if (conn == NULL || plaintext == NULL || table_name == NULL || column_name == NULL)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_unified_encrypt_for_where");
        return NULL;
    }

    /* Map operator type to algorithm */
    algo_type = TECC_ALGO_DET;  /* default for equality */
    algo_name = "DET";
    algo_fallback_allowed = false; /* may switch to OPE for = */

    switch (operator_type)
    {
        case TECC_OP_EQUAL:
        case TECC_OP_NOT_EQUAL:
        case TECC_OP_IS_NOT_DISTINCT_FROM:
        case TECC_OP_IS_DISTINCT_FROM:
            algo_type = TECC_ALGO_DET;
            algo_name = "DET";
            algo_fallback_allowed = true; /* OPE also works for equality */
            break;
        case TECC_OP_LESS_THAN:
        case TECC_OP_GREATER_THAN:
        case TECC_OP_LESS_EQUAL:
        case TECC_OP_GREATER_EQUAL:
            algo_type = TECC_ALGO_OPE;
            algo_name = "OPE";
            break;
        case TECC_OP_LIKE:
        case TECC_OP_NOT_LIKE:
        case TECC_OP_ILIKE:
        case TECC_OP_NOT_ILIKE:
            algo_type = TECC_ALGO_SEARCH;
            algo_name = "SEARCH";
            break;
        default:
            TECC_SCHEDULER_ERROR("Unsupported operator type for WHERE encryption: %d", operator_type);
            return NULL;
    }

    TECC_SCHEDULER_DEBUG("WHERE encrypt: %s on %s.%s using %s",
                         plaintext, table_name, column_name, algo_name);

    scheduler = tecc_get_scheduler_from_conn(conn);
    if (scheduler == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to get crypto scheduler from connection");
        return NULL;
    }

    /* Build context and encrypt with the selected algorithm only */
    {
        TeccState *state = tecc_get_state(conn);
        TeccColumnCacheItem *cache_item = NULL;
        TeccEncryptedColumnMetadata *col_meta = NULL;

        if (state != NULL)
        {
            const char *db_name = PQdb(conn);
            if (db_name == NULL || db_name[0] == '\0')
                db_name = "postgres";
            /* 使用调用方传入的 schema（限定表名如 sch1.t_det），
             * 原硬编码 "public" 导致非 public 模式表的 WHERE 加密元数据漏检。 */
            cache_item = tecc_cache_get_encrypted_column_by_name(
                state, db_name, schema_name, table_name, column_name);
            if (cache_item != NULL)
                col_meta = tecc_convert_column_cache_item_to_metadata(state, cache_item);
        }

        if (col_meta == NULL)
        {
            TECC_SCHEDULER_ERROR("Column %s.%s not found in metadata cache", table_name, column_name);
            return NULL;
        }

        /* ReqID:TECC4.0 - For =/<> operators, if the column lacks DET
         * but has OPE, fall back to OPE. OPE also supports equality. */
        if (algo_fallback_allowed &&
            algo_type == TECC_ALGO_DET &&
            !col_meta->enable_det &&
            col_meta->enable_ope)
        {
            algo_type = TECC_ALGO_OPE;
            algo_name = "OPE";
            TECC_SCHEDULER_DEBUG("WHERE encrypt: column has OPE but not DET, "
                               "using OPE for equality comparison");
        }

        {
            TeccCryptoContextExtended *context;
            unsigned char *dek_key = NULL;
            unsigned char *bpchar_trim_buf = NULL;
            size_t dek_key_len = 0;
            int ret;
            CipherDataType *cipherdata = NULL;
            size_t cipherdata_len = 0;
            TeccFormatterErrorCode fmt_ret;
            unsigned char *primary_ct = NULL;
            size_t primary_ct_len = 0;
            const char *dek_name_to_use;
            TeccCryptoBuffer *cipherdata_buffer;

            context = tecc_create_crypto_context_with_collation(table_name, column_name,
                                                                kek_name, dek_name, 0);
            if (context == NULL)
            {
                tecc_free_encrypted_column_metadata(col_meta);
                return NULL;
            }

            context->core.data_type = tecc_map_to_original_type_oid(col_meta->data_type_original_oid);
            /* ReqID:TECC4.0 - MUST set type_mod BEFORE bpchar padding and rtrim.
             * type_mod is initialized to 0 by pg_malloc0 in context creation.
             * The bpchar padding check (core.type_mod >= 4) and padding length
             * calculation need the correct typmod from column metadata.
             * See also: collation_locale and supports_ope which are needed by
             * normalize_data_for_algorithm inside tecc_execute_algo_ope. */
            context->core.type_mod = col_meta->data_type_original_mod;
            if (col_meta->collation_locale != NULL)
                strlcpy(context->core.collation_locale, col_meta->collation_locale, NAMEDATALEN);
            context->core.algo_support.supports_det = col_meta->enable_det;
            context->core.algo_support.supports_ope = col_meta->enable_ope;
            context->core.algo_support.supports_search = col_meta->enable_search;
        context->core.algo_support.supports_he_add = col_meta->enable_he_add;
        context->core.algo_support.supports_he_mul = col_meta->enable_he_mul;

            /* ReqID:TECC4.0 - Unified text preparation for WHERE encryption:
             * 1. BPCHAR: pad to declared length (PG backend pads bpchar(N) to N bytes)
             * 2. Universal rtrim: compute space_count (but do NOT reduce plaintext_len)
             * This matches the INSERT path (tecc_unified_encrypt_string_internal_with_scheduler)
             * exactly: INSERT also keeps plaintext_len at the padded length and computes
             * text_space_count separately. normalize_data_for_algorithm inside
             * tecc_execute_algo_ope reads context->plaintext_len bytes for ICU sort key
             * generation, so the padded (not rtrimmed) length is critical. */
            {
                const unsigned char *pt = (const unsigned char *)plaintext;

                context->plaintext = pt;
                context->plaintext_len = strlen(plaintext);

                /* 1. BPCHAR blank-padding to declared length */
                if (context->core.data_type == BPCHAROID && context->core.type_mod >= (int32)sizeof(int32))
                {
                    int32 bpchar_len = context->core.type_mod - 4;
                    size_t orig_len = context->plaintext_len;

                    if (bpchar_len > 0 && (size_t)bpchar_len > orig_len)
                    {
                        const unsigned char *orig_text = context->plaintext;
                        unsigned char *padded;
                        size_t pad_count, i;

                        padded = (unsigned char *)pg_malloc((size_t)bpchar_len + 1);
                        if (padded != NULL)
                        {
                            memcpy(padded, orig_text, orig_len);
                            pad_count = (size_t)bpchar_len - orig_len;
                            for (i = 0; i < pad_count; i++)
                                padded[orig_len + i] = ' ';
                            padded[bpchar_len] = '\0';
                            context->plaintext = padded;
                            context->plaintext_len = (size_t)bpchar_len;
                            bpchar_trim_buf = padded;  /* track for cleanup */
                        }
                    }
                }

                /* 2. Universal rtrim for text types: compute space_count
                 * and reduce plaintext_len so DET/OPE always encrypt the
                 * rtrimmed data. space_count is stored for ciphertext format
                 * encoding; the server-side bpchar_enc_eq strips it and
                 * compares only the core ciphertext. */
                context->text_space_count = 0;
                if (TECC_IS_TEXT_TYPE(context->core.data_type) &&
                    context->plaintext_len > 0)
                {
                    const unsigned char *pd = context->plaintext;
                    size_t used = context->plaintext_len;

                    while (used > 0 && pd[used - 1] == ' ')
                        used--;
                    context->text_space_count = (uint32_t)(context->plaintext_len - used);
                    context->plaintext_len = used;
                }
            }

            /* ReqID:TECC4.0 - Set operator type for SEARCH LIKE/NOT LIKE pattern encryption */
            context->operator_type = operator_type;
            if (operator_type == TECC_OP_LIKE || operator_type == TECC_OP_NOT_LIKE ||
                operator_type == TECC_OP_ILIKE || operator_type == TECC_OP_NOT_ILIKE)
            {
                context->is_like_pattern = true;
                context->like_escape_char = like_escape_char;
            }

            /* ReqID:TECC4.0 - Set selected algorithms based on operator type. */
            if (algo_type == TECC_ALGO_SEARCH)
                context->core.selected_algorithms = (1 << TECC_ALGO_RND) | (1 << TECC_ALGO_SEARCH);
            else
                context->core.selected_algorithms = (1 << TECC_ALGO_RND) | (1 << algo_type);

            dek_name_to_use = dek_name ? dek_name : col_meta->dek_name;
            ret = tecc_key_manager_adapter_get_dek_key(scheduler->key_adapter,
                                                      dek_name_to_use,
                                                      &dek_key, &dek_key_len);
            if (ret != TECC_CRYPTO_SUCCESS)
            {
                TECC_SCHEDULER_ERROR("Failed to get DEK key for WHERE encryption");
                tecc_free_encrypted_column_metadata(col_meta);
                if (bpchar_trim_buf) pg_free(bpchar_trim_buf);
                pg_free(context);
                return NULL;
            }

            context->dek_key = dek_key;
            context->dek_key_len = dek_key_len;

            /* ReqID:TECC4.0 - HSM路由支持：补填col_meta->kek_name并设置HSM上下文
             * 同INSERT路径的修复模式，确保col_meta->kek_name不为NULL，
             * 使tecc_setup_hsm_context能检测到HSM KEK。
             * DET/OPE算法不使用HSM做数据加密，但正确的上下文状态对后续处理至关重要。 */
            if (col_meta && (col_meta->kek_name == NULL || col_meta->kek_name[0] == '\0') &&
                state != NULL && dek_name_to_use != NULL)
            {
                TeccDekCacheItem *dek_item = tecc_cache_get_dek_by_name_search_path(
                    (const TeccState *)state, dek_name_to_use);
                if (dek_item != NULL && dek_item->kek_name != NULL && dek_item->kek_name[0] != '\0')
                {
                    col_meta->kek_name = pg_strdup(dek_item->kek_name);
                    strlcpy(context->kek_name, dek_item->kek_name, NAMEDATALEN);
                }
            }

            /* Set up HSM SM4 encryption context based on KEK's KMS type.
             * If HSM is configured but init failed, abort — no software fallback. */
            if (state != NULL)
            {
                if (tecc_setup_hsm_context(context, col_meta, state) < 0)
                {
                    TECC_SCHEDULER_ERROR("HSM init failed for KEK, refusing software fallback");
                    tecc_free_encrypted_column_metadata(col_meta);
                    if (bpchar_trim_buf) pg_free(bpchar_trim_buf);
                    pg_free(context);
                    return NULL;
                }
            }

            /* Execute the selected algorithm */
            switch (algo_type)
            {
                case TECC_ALGO_DET:
                    ret = tecc_execute_algo_det(context);
                    primary_ct = context->det_ciphertext;
                    primary_ct_len = context->det_cipher_len;
                    break;
                case TECC_ALGO_OPE:
                    ret = tecc_execute_algo_ope(context);
                    primary_ct = context->ope_ciphertext;
                    primary_ct_len = context->ope_cipher_len;
                    break;
                case TECC_ALGO_SEARCH:
                    /* ReqID:TECC4.0 - Use operator-aware encryption for LIKE patterns */
                    if (operator_type == TECC_OP_LIKE || operator_type == TECC_OP_NOT_LIKE ||
                        operator_type == TECC_OP_ILIKE || operator_type == TECC_OP_NOT_ILIKE)
                    {
                        ret = tecc_algorithm_scheduler_encrypt_search_with_operator(
                            (TeccCryptoContext *)context, operator_type,
                            &primary_ct, &primary_ct_len);
                    }
                    else
                    {
                        ret = tecc_execute_algo_search(context);
                        primary_ct = context->search_ciphertext;
                        primary_ct_len = context->search_cipher_len;
                    }
                    break;
                default:
                    tecc_free_encrypted_column_metadata(col_meta);
                    if (bpchar_trim_buf) pg_free(bpchar_trim_buf);
                    pg_free(context);
                    return NULL;
            }

            if (ret != TECC_CRYPTO_SUCCESS || primary_ct == NULL)
            {
                TECC_SCHEDULER_ERROR("%s algorithm encryption failed for WHERE clause", algo_name);
                tecc_free_encrypted_column_metadata(col_meta);
                if (bpchar_trim_buf) pg_free(bpchar_trim_buf);
                pg_free(context);
                return NULL;
            }

            /* Build a minimal CipherDataType carrying only the needed
             * algorithm's segment (v2.0 compact header). */
            {
                uint16 seg_bit;
                uint16 mask;

                switch (algo_type)
                {
                    case TECC_ALGO_RND:    seg_bit = CIPHERDATA_SEG_RND;    break;
                    case TECC_ALGO_DET:    seg_bit = CIPHERDATA_SEG_DET;    break;
                    case TECC_ALGO_OPE:    seg_bit = CIPHERDATA_SEG_OPE;    break;
                    case TECC_ALGO_SEARCH: seg_bit = CIPHERDATA_SEG_SEARCH; break;
                    default:               seg_bit = 0;                     break;
                }
                mask = seg_bit;

                cipherdata_len = CIPHERDATA_TOTAL_SIZE(mask, 0, 0, 0, 0, 0, 0)
                                 + primary_ct_len;
                cipherdata = pg_malloc0(cipherdata_len);
                if (cipherdata == NULL)
                {
                    tecc_free_encrypted_column_metadata(col_meta);
                    if (bpchar_trim_buf) pg_free(bpchar_trim_buf);
                    pg_free(context);
                    return NULL;
                }

                {
                    char *cd_ptr;
                    int32 version;
                    Oid dek_dist_oid;

                    SET_VARSIZE(cipherdata, cipherdata_len);
                    cd_ptr = (char *)cipherdata;
                    cd_ptr += sizeof(int32);       /* skip vl_len_ */

                    /* version packs the major version and the segment mask */
                    version = CIPHERDATA_MAKE_VERSION(CIPHERDATA_VERSION_2_0, mask);
                    memcpy(cd_ptr, &version, sizeof(int32));
                    cd_ptr += sizeof(int32);

                    /* dek_dist_oid -- not known at this level */
                    dek_dist_oid = InvalidOid;
                    memcpy(cd_ptr, &dek_dist_oid, sizeof(Oid));
                    cd_ptr += sizeof(Oid);

                    /* Single length field for the selected segment */
                    {
                        int32 algo_len_val = (int32)primary_ct_len;

                        memcpy(cd_ptr, &algo_len_val, sizeof(int32));
                        cd_ptr += sizeof(int32);
                    }

                    /* Copy ciphertext */
                    memcpy(cd_ptr, primary_ct, primary_ct_len);
                }
            }

            /* Set DEK distributed ID from cache if available */
            if (state != NULL && col_meta != NULL && col_meta->dek_name != NULL)
            {
                TeccDekCacheItem *dek_item = tecc_cache_get_dek_by_name_search_path(
                    state, col_meta->dek_name);
                if (dek_item != NULL && OidIsValid(dek_item->column_key_distributed_id))
                    cipherdata->dek_dist_oid = dek_item->column_key_distributed_id;
            }

            cipherdata_buffer = pg_malloc0(sizeof(TeccCryptoBuffer));
            if (cipherdata_buffer)
            {
                cipherdata_buffer->data = cipherdata;
                cipherdata_buffer->size = cipherdata_len;
                cipherdata_buffer->used = cipherdata_len;
            }

            result = pg_malloc0(sizeof(TeccUnifiedEncryptResult));
            if (result)
            {
                result->error_code = TECC_CRYPTO_SUCCESS;
                fmt_ret = tecc_formatter_hex_encode((const unsigned char *)cipherdata, cipherdata_len,
                                                    &result->hex_string);
                if (fmt_ret == TECC_FORMATTER_SUCCESS)
                    result->hex_len = strlen(result->hex_string);
                result->cipherdata_buffer = cipherdata_buffer;
            }

            tecc_free_encrypted_column_metadata(col_meta);
            if (bpchar_trim_buf) pg_free(bpchar_trim_buf);
            pg_free(context);

            return result;
        }
    }
}

/* Begin - ReqID:TECC4.0 - HE 标量运算常量加密（客户端加密常量 → 服务端全密文同态） */
/*
 * tecc_unified_encrypt_he_constant
 *
 * Encrypts an integer constant with the column's HE public key and returns a
 * minimal CipherDataType (hex) carrying ONLY the EG_ADD slot (use_he_add=true,
 * EC-ElGamal) or the EG_MUL slot (use_he_add=false, finite field ElGamal),
 * including the per-segment DEK distributed ID prefix.
 *
 * The caller rewrites `enc + k` / `enc - k` / `enc * k` into
 * `enc + 'hex'::intN_enc`, so the server resolves the generic ciphertext×
 * ciphertext HE operators (he_add_generic / he_sub_generic / he_mul_generic)
 * and the plaintext constant never leaves the client.  Returns NULL on
 * failure (column not an HE column, HE DEK unresolvable, encryption error).
 */
TeccUnifiedEncryptResult *
tecc_unified_encrypt_he_constant(PGconn *conn,
                                 const char *plaintext,
                                 const char *table_name,
                                 const char *column_name,
                                 const char *schema_name,
                                 bool use_he_add)
{
    TeccUnifiedEncryptResult *result = NULL;
    TeccState *state = NULL;
    TeccColumnCacheItem *cache_item = NULL;
    TeccEncryptedColumnMetadata *col_meta = NULL;
    TeccCryptoContextExtended *context = NULL;
    CipherDataType *cipherdata = NULL;
    size_t cipherdata_len = 0;
    TeccFormatterErrorCode fmt_ret;
    char he_dek_name[NAMEDATALEN];
    const char *resolved_dek_name = NULL;
    char *he_pub_hex = NULL;
    TeccDekCacheItem *dek_item = NULL;
    Oid he_dek_dist_id = InvalidOid;
    int ret;
    const char *db_name;

    if (conn == NULL || plaintext == NULL || table_name == NULL || column_name == NULL)
    {
        TECC_SCHEDULER_ERROR("Invalid parameters to tecc_unified_encrypt_he_constant");
        return NULL;
    }

    state = tecc_get_state(conn);
    if (state == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to get TECC state for HE constant encryption");
        return NULL;
    }

    db_name = PQdb(conn);
    if (db_name == NULL || db_name[0] == '\0')
        db_name = "postgres";

    cache_item = tecc_cache_get_encrypted_column_by_name(
        state, db_name, schema_name, table_name, column_name);
    if (cache_item == NULL)
    {
        TECC_SCHEDULER_ERROR("Column %s.%s not found in metadata cache", table_name, column_name);
        return NULL;
    }
    col_meta = tecc_convert_column_cache_item_to_metadata(state, cache_item);
    if (col_meta == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to convert column metadata for %s.%s", table_name, column_name);
        return NULL;
    }

    /* The required HE algorithm must be enabled on the column */
    if (use_he_add && !col_meta->enable_he_add)
    {
        TECC_SCHEDULER_ERROR("Column %s.%s has no he-add support", table_name, column_name);
        tecc_free_encrypted_column_metadata(col_meta);
        return NULL;
    }
    if (!use_he_add && !col_meta->enable_he_mul)
    {
        TECC_SCHEDULER_ERROR("Column %s.%s has no he-mul support", table_name, column_name);
        tecc_free_encrypted_column_metadata(col_meta);
        return NULL;
    }

    /* Resolve HE DEK name: column-level he_add_dek/he_mul_dek binding
     * (hg_encrypted_column_args, P0-2 multi-DEK) first, fall back to the
     * column's DEK (single-DEK V4.0.0 compatibility). */
    he_dek_name[0] = '\0';
    if (OidIsValid(col_meta->table_oid) && col_meta->column_attnum > 0)
    {
        tecc_load_column_he_dek_name(state, col_meta->table_oid, col_meta->column_attnum,
                                     use_he_add ? "he_add_dek" : "he_mul_dek",
                                     he_dek_name, sizeof(he_dek_name));
    }
    if (he_dek_name[0] != '\0')
        resolved_dek_name = he_dek_name;
    else
        resolved_dek_name = col_meta->dek_name;
    if (resolved_dek_name == NULL || resolved_dek_name[0] == '\0')
    {
        TECC_SCHEDULER_ERROR("HE DEK name unresolvable for column %s.%s", table_name, column_name);
        tecc_free_encrypted_column_metadata(col_meta);
        return NULL;
    }

    /* Build a minimal crypto context with only the required HE algorithm */
    context = (TeccCryptoContextExtended *)tecc_create_crypto_context(
        table_name, column_name, NULL, resolved_dek_name);
    if (context == NULL)
    {
        TECC_SCHEDULER_ERROR("Failed to create crypto context for HE constant encryption");
        tecc_free_encrypted_column_metadata(col_meta);
        return NULL;
    }

    context->core.data_type = tecc_map_to_original_type_oid(col_meta->data_type_original_oid);
    context->core.algo_support.supports_he_add = use_he_add;
    context->core.algo_support.supports_he_mul = !use_he_add;
    context->plaintext = (const unsigned char *)plaintext;
    context->plaintext_len = strlen(plaintext);

    /* Load per-DEK HE public key */
    if (tecc_he_load_dek_key_hex(state, resolved_dek_name,
                                 use_he_add ? "he_ec_pubkey" : "he_eg_pubkey",
                                 &he_pub_hex) != 0 || he_pub_hex == NULL)
    {
        TECC_SCHEDULER_ERROR("HE constant encrypt: %s public key not loaded for DEK '%s'",
                             use_he_add ? "EC" : "EG", resolved_dek_name);
        if (he_pub_hex)
            pg_free(he_pub_hex);
        tecc_destroy_crypto_context((TeccCryptoContext *)context);
        tecc_free_encrypted_column_metadata(col_meta);
        return NULL;
    }
    if (use_he_add)
        context->he_ec_pubkey_hex = he_pub_hex;
    else
        context->he_eg_pubkey_hex = he_pub_hex;

    /* Resolve per-segment DEK distributed ID (must match the column's
     * EG_ADD/EG_MUL slot DEK ID for the server-side DEK match check) */
    dek_item = tecc_cache_get_dek_by_name_search_path(state, resolved_dek_name);
    if (dek_item != NULL && OidIsValid(dek_item->column_key_distributed_id))
    {
        he_dek_dist_id = dek_item->column_key_distributed_id;
        if (use_he_add)
            context->he_add_dek_id = he_dek_dist_id;
        else
            context->he_mul_dek_id = he_dek_dist_id;
    }

    /* Execute the HE algorithm */
    if (use_he_add)
        ret = tecc_execute_algo_he_add((TeccCryptoContext *)context);
    else
        ret = tecc_execute_algo_he_mul((TeccCryptoContext *)context);

    if (ret != TECC_ALGO_SUCCESS ||
        (use_he_add && (context->he_add_ciphertext == NULL || context->he_add_cipher_len == 0)) ||
        (!use_he_add && (context->he_mul_ciphertext == NULL || context->he_mul_cipher_len == 0)))
    {
        TECC_SCHEDULER_ERROR("HE constant encrypt: %s encryption failed: %d",
                             use_he_add ? "HE_ADD" : "HE_MUL", ret);
        tecc_destroy_crypto_context((TeccCryptoContext *)context);
        tecc_free_encrypted_column_metadata(col_meta);
        return NULL;
    }

    /* Format a minimal CipherDataType carrying only the HE slot
     * (tecc_formatter_format_cipherdata writes the 4B DEK ID prefix) */
    fmt_ret = tecc_formatter_format_cipherdata(context, &cipherdata, &cipherdata_len);
    if (fmt_ret != TECC_FORMATTER_SUCCESS || cipherdata == NULL)
    {
        TECC_SCHEDULER_ERROR("HE constant encrypt: format cipherdata failed: %d", fmt_ret);
        if (cipherdata)
            pg_free(cipherdata);
        tecc_destroy_crypto_context((TeccCryptoContext *)context);
        tecc_free_encrypted_column_metadata(col_meta);
        return NULL;
    }

    /* ReqID:TECC4.0 - 头部 DEK 分布式 ID：formatter 置 InvalidOid，但后端
     * cipherdata_validate 要求头部 dek_dist_oid 有效（INSERT 路径在调度器
     * 中补填），此处同样补填，保证 'hex'::intN_enc 输入合法。 */
    if (OidIsValid(he_dek_dist_id))
        cipherdata->dek_dist_oid = he_dek_dist_id;

    result = (TeccUnifiedEncryptResult *)pg_malloc0(sizeof(TeccUnifiedEncryptResult));
    if (result == NULL)
    {
        pg_free(cipherdata);
        tecc_destroy_crypto_context((TeccCryptoContext *)context);
        tecc_free_encrypted_column_metadata(col_meta);
        return NULL;
    }

    result->error_code = TECC_CRYPTO_SUCCESS;
    fmt_ret = tecc_formatter_hex_encode((const unsigned char *)cipherdata,
                                        cipherdata_len, &result->hex_string);
    if (fmt_ret != TECC_FORMATTER_SUCCESS || result->hex_string == NULL)
    {
        TECC_SCHEDULER_ERROR("HE constant encrypt: hex encode failed: %d", fmt_ret);
        tecc_unified_free_encrypt_result(result);
        tecc_destroy_crypto_context((TeccCryptoContext *)context);
        tecc_free_encrypted_column_metadata(col_meta);
        return NULL;
    }
    result->hex_len = strlen(result->hex_string);
    {
        TeccCryptoBuffer *cb = (TeccCryptoBuffer *)pg_malloc0(sizeof(TeccCryptoBuffer));
        if (cb != NULL)
        {
            cb->data = cipherdata;
            cb->size = cipherdata_len;
            cb->used = cipherdata_len;
            result->cipherdata_buffer = cb;
        }
        else
            pg_free(cipherdata);
    }

    tecc_destroy_crypto_context((TeccCryptoContext *)context);
    tecc_free_encrypted_column_metadata(col_meta);

    return result;
}
/* End - ReqID:TECC4.0 */

