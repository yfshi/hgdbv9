/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_crypto_context.h
 *
 * Abstract:
 *   Crypto context definitions for TECC crypto scheduler module.
 *   Defines structures for encryption context and memory management.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/interfaces/libpq/tecc_scheduler/tecc_crypto_context.h
 */

#ifndef TECC_CRYPTO_CONTEXT_H
#define TECC_CRYPTO_CONTEXT_H

#include "postgres_fe.h"
#include "c.h"
#include "../tecc_common/tecc_common_types.h"
/* Forward declarations */
typedef struct TeccBufferDesc TeccBufferDesc;

/* Extended crypto context with unified memory management */
typedef TeccCryptoContextExtended TeccCryptoContext;

/* Function prototypes */

/* Crypto context management */
extern TeccCryptoContext *tecc_create_crypto_context(const char *table_name,
                                                    const char *column_name,
                                                    const char *kek_name,
                                                    const char *dek_name);
extern TeccCryptoContext *tecc_create_crypto_context_with_pool(const char *table_name,
                                                              const char *column_name,
                                                              const char *kek_name,
                                                              const char *dek_name);
extern TeccCryptoContext *tecc_create_crypto_context_with_collation(const char *table_name,
                                                                   const char *column_name,
                                                                   const char *kek_name,
                                                                   const char *dek_name,
                                                                   Oid collation_oid);
extern void tecc_destroy_crypto_context(TeccCryptoContext *context);
extern void tecc_crypto_context_reset(TeccCryptoContext *context);

/* Buffer management */
extern bool tecc_buffer_prepare(TeccCryptoContext *context, TeccBufferType type, size_t size);
extern void *tecc_buffer_get(TeccCryptoContext *context, TeccBufferType type);
extern size_t tecc_context_buffer_get_size(TeccCryptoContext *context, TeccBufferType type);
extern bool tecc_buffer_set_size(TeccCryptoContext *context, TeccBufferType type, size_t size);
extern TeccBufferType tecc_get_reusable_buffer(TeccCryptoContext *context, size_t required_size,
                                              TeccBufferType preferred_type);

/* Utility functions */
extern void tecc_calculate_buffer_sizes(TeccCryptoContext *context, size_t input_size, size_t *sizes);
extern bool tecc_validate_collation(TeccCryptoContext *context);
extern int tecc_prepare_icu_normalization(TeccCryptoContext *context);

#endif /* TECC_CRYPTO_CONTEXT_H */
/* End - ReqID:TECC4.0 */