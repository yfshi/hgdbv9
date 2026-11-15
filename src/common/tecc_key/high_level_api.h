/*-------------------------------------------------------------------------
 *
 * high_level_api.h
 *    High-level API interface for TECC key management module
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/common/tecc_key/high_level_api.h
 *
 * ReqID:TECC4.0 - Key management high-level API
 * Authored by <wanghaoyan@highgo.com>, 20260408.
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_KEY_HIGH_LEVEL_API_H
#define TECC_KEY_HIGH_LEVEL_API_H

/*
 * IMPORTANT: This module is designed for FRONTEND use only.
 * It uses frontend memory allocators (pg_malloc/pg_free) and
 * should only be included by frontend code.
 *
 * If backend code needs key management functionality, a separate
 * backend-specific implementation should be created.
 */
#ifndef FRONTEND
#error "high_level_api.h should only be included by frontend code"
#endif

#include "postgres_fe.h"
#include "c.h"
#include "port/pg_bswap.h"

/* API version management */
#define TECC_KEY_API_VERSION_MAJOR 1
#define TECC_KEY_API_VERSION_MINOR 0
#define TECC_KEY_API_VERSION_PATCH 0

/* API information structure */
typedef struct TeccKeyApiInfo
{
    int api_version_major;
    int api_version_minor;
    int api_version_patch;
    char *module_name;
    char *build_date;
    char *git_hash;
    unsigned long long features;  /* Feature bitmap */
} TeccKeyApiInfo;

/* Key format types */
typedef enum KeyFormat
{
    KEY_FORMAT_RAW,          /* Raw key bytes */
    KEY_FORMAT_ENCRYPTED,    /* Encrypted key */
    KEY_FORMAT_WRAPPED       /* Wrapped key */
} KeyFormat;

/* Key security levels */
typedef enum KeySecurityLevel
{
    KEY_SECURITY_LOW,        /* Low security, plaintext in memory */
    KEY_SECURITY_MEDIUM,     /* Medium security, encrypted in memory */
    KEY_SECURITY_HIGH        /* High security, hardware protected */
} KeySecurityLevel;

/* Key manager key data structure */
typedef struct KeyManagerKeyData
{
    unsigned char *data;        /* Key data */
    size_t length;              /* Data length */
    KeyFormat format;           /* Data format: RAW/ENCRYPTED */
    KeySecurityLevel security;  /* Security level */
} KeyManagerKeyData;

/* Function prototypes */

/* API version checking */
extern int tecc_key_check_api_version(int required_major, int required_minor);
extern TeccKeyApiInfo *tecc_key_get_api_info(void);

/* High-level key operations */
/* tecc_key_get_dek removed - use tecc_cache_get_dek_by_name instead */
/* tecc_key_get_kek removed - use tecc_key_get_kek_with_state instead */
extern unsigned char *tecc_key_get_kek_with_state(void *tecc_state, const char *kek_name, size_t *key_len);

/* 绕过连接级明文缓存，强制重读存储（localkms 读 .priv/.key 文件）——供 CREATE DEK
 * 私钥可用性校验使用，保证同一连接内删除私钥文件后也能稳定报错。 */
extern unsigned char *tecc_key_get_kek_with_state_refresh(void *tecc_state, const char *kek_name, size_t *key_len);

/*
 * ReqID:TECC4.0 - Bug 9256: 由 DEK 侧信息取出其 KEK 明文。
 *
 * DEK 的目录记录里存的是 KEK 的 OID（kek_id），而 KEK 可以创建在任意 schema。
 * 若只拿 DEK 上那个裸 KEK 名去按 search_path 解析，KEK 不在 search_path 上时
 * 永远找不到（在 public 下使用 sch1.kek 建表，INSERT 直接失败）。本函数优先按
 * OID 精确解析，再以限定名 "schema.name" 取明文；仅当 OID 无效（异常/旧缓存项）
 * 时才退回按名解析。OID 有效但查不到时不回退 —— 同名 KEK 可能是另一个 schema
 * 里的另一把密钥，用错密钥只会得到难以定位的解密失败。
 */
extern unsigned char *tecc_key_get_kek_by_oid_or_name(void *tecc_state,
                                                      Oid kek_oid,
                                                      const char *kek_name,
                                                      size_t *key_len);
extern int tecc_key_decrypt_dek(const unsigned char *encrypted_dek,
                               size_t encrypted_len,
                               const unsigned char *kek,
                               size_t kek_len,
                               unsigned char **decrypted_dek,
                               size_t *decrypted_len);

/* Extended key operations */
extern unsigned char *tecc_key_get_decrypted_dek(const void *tecc_state, const char *dek_name, size_t *key_len);
extern unsigned char *tecc_key_get_kek_for_dek(const void *tecc_state, const char *dek_name, size_t *kek_len);

/* Key validation */
extern bool tecc_key_validate_dek(const char *dek_name);
extern bool tecc_key_validate_kek(const char *kek_name);

/* Memory management */
extern void tecc_key_free_key_data(unsigned char *key_data, size_t key_len);
extern void tecc_key_secure_erase(void *data, size_t length);

/* Error handling */
extern const char *tecc_key_get_last_error(void);
extern void tecc_key_clear_last_error(void);

#endif /* TECC_KEY_HIGH_LEVEL_API_H */