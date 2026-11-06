/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_sm4_internal.h
 *
 * Abstract:
 *   Internal SM4 block cipher implementation header.
 *   Pure C implementation per GM/T 0002-2012 standard.
 *   No OpenSSL dependency - used as replacement for EVP_sm4_cbc/ecb.
 *
 * Authored by <wangxp@highgo.com>, 20260508.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/include/common/tecc_algorithm/tecc_sm4_internal.h
 */

#ifndef TECC_SM4_INTERNAL_H
#define TECC_SM4_INTERNAL_H

#include <stddef.h>

#define TECC_SM4_BLOCK_SIZE 16
#define TECC_SM4_KEY_SIZE   16

typedef struct TeccSM4Context TeccSM4Context;

TeccSM4Context *tecc_sm4_context_new(void);
void tecc_sm4_context_free(TeccSM4Context *ctx);

int tecc_sm4_set_key_encrypt(TeccSM4Context *ctx, const unsigned char key[16]);
int tecc_sm4_set_key_decrypt(TeccSM4Context *ctx, const unsigned char key[16]);

int tecc_sm4_ecb_encrypt(TeccSM4Context *ctx,
                         const unsigned char in[16],
                         unsigned char out[16]);
int tecc_sm4_ecb_decrypt(TeccSM4Context *ctx,
                         const unsigned char in[16],
                         unsigned char out[16]);

int tecc_sm4_cbc_encrypt(TeccSM4Context *ctx,
                         const unsigned char iv[16],
                         const unsigned char *in, size_t len,
                         unsigned char *out);
int tecc_sm4_cbc_decrypt(TeccSM4Context *ctx,
                         const unsigned char iv[16],
                         const unsigned char *in, size_t len,
                         unsigned char *out);

#endif /* TECC_SM4_INTERNAL_H */
/* End - ReqID:TECC4.0 */
