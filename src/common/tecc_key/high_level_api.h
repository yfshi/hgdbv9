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