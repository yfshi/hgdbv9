/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_sm3_internal.h
 *
 * Abstract:
 *   Internal SM3 hash function implementation header.
 *   Pure C implementation per GM/T 0004-2012 standard.
 *   Includes HMAC-SM3 (RFC 2104) built on SM3 core.
 *   No OpenSSL dependency.
 *
 * Authored by <wangxp@highgo.com>, 20260508.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/include/common/tecc_algorithm/tecc_sm3_internal.h
 */

#ifndef TECC_SM3_INTERNAL_H
#define TECC_SM3_INTERNAL_H

#include <stddef.h>

#define TECC_SM3_DIGEST_LENGTH 32
#define TECC_SM3_BLOCK_SIZE    64
/* Compatibility alias used by tests and hkdf module */
#define SM3_HASH_LEN TECC_SM3_DIGEST_LENGTH

typedef struct TeccSM3Context TeccSM3Context;

TeccSM3Context *tecc_sm3_context_new(void);
void tecc_sm3_context_free(TeccSM3Context *ctx);

int tecc_sm3_init(TeccSM3Context *ctx);
int tecc_sm3_update(TeccSM3Context *ctx, const unsigned char *data, size_t len);
int tecc_sm3_final(TeccSM3Context *ctx, unsigned char digest[32]);

int tecc_sm3_hash(const unsigned char *data, size_t len,
                  unsigned char digest[32]);

/* HMAC-SM3 context (RFC 2104) */
typedef struct TeccSM3HMACContext TeccSM3HMACContext;

TeccSM3HMACContext *tecc_sm3_hmac_new(void);
void tecc_sm3_hmac_free(TeccSM3HMACContext *ctx);
int tecc_sm3_hmac_init(TeccSM3HMACContext *ctx,
                       const unsigned char *key, size_t key_len);
int tecc_sm3_hmac_update(TeccSM3HMACContext *ctx,
                         const unsigned char *data, size_t len);
int tecc_sm3_hmac_final(TeccSM3HMACContext *ctx,
                        unsigned char mac[32]);

int tecc_sm3_hmac(const unsigned char *key, size_t key_len,
                  const unsigned char *data, size_t data_len,
                  unsigned char mac[32]);

/* Begin - ReqID:TECC4.0 */
/*
 * SM3 HMAC Snapshot API — eliminates per-item hmac_init overhead
 * in batch processing.  Creates a snapshot of the intermediate
 * SM3 states after both ipad and opad blocks are compressed, then
 * restores those states for each per-item hmac computation, skipping
 * repeated key expansion and initial block compression.
 */
typedef struct TeccSM3HMACSnapshot TeccSM3HMACSnapshot;

TeccSM3HMACSnapshot *tecc_sm3_hmac_snapshot_create(const unsigned char *key, size_t key_len);
int tecc_sm3_hmac_snapshot_compute(TeccSM3HMACSnapshot *snap,
                                    const unsigned char *data, size_t data_len,
                                    unsigned char mac[32]);
void tecc_sm3_hmac_snapshot_free(TeccSM3HMACSnapshot *snap);
/* End - ReqID:TECC4.0 */

#endif /* TECC_SM3_INTERNAL_H */
/* End - ReqID:TECC4.0 */
