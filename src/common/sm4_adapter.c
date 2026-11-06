/*
 * File: sm4_adapter.c
 * Abstract: SM4算法适配器实现 - 为OPE算法提供兼容的SM4接口，基于内部SM4实现
 * Authored by <wangxp@highgo.com>, 20260312.
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 * Identification: src/common/crypto/sm4_adapter.c
 */

#include "postgres.h"
#include "c.h"
#include "utils/palloc.h"
#include "sm4_adapter.h"
#include "common/tecc_algorithm/tecc_sm4_internal.h"
#include <string.h>

/* ReqID:TECC4.0 */

/* Begin - ReqID:TECC4.0 */


/* SM4密钥初始化 */
int
SM4_KeyInit(uint8_t *key, SM4_Key *sm4_key)
{
    struct SM4_KeyContext *ctx;

    if (!key || !sm4_key)
    {
        return 0;
    }

    /* 分配密钥上下文 */
    ctx = (struct SM4_KeyContext *)palloc(sizeof(struct SM4_KeyContext));
    if (!ctx)
    {
        return 0;
    }

    memset(ctx, 0, sizeof(struct SM4_KeyContext));

    /* 保存密钥 */
    memcpy(ctx->key, key, SM4_KEY_SIZE);

    /* 创建加密上下文 */
    ctx->encrypt_ctx = tecc_sm4_context_new();
    if (!ctx->encrypt_ctx)
    {
        pfree(ctx);
        return 0;
    }

    /* 初始化SM4-ECB加密密钥 */
    if (tecc_sm4_set_key_encrypt(ctx->encrypt_ctx, key) != 1)
    {
        tecc_sm4_context_free(ctx->encrypt_ctx);
        pfree(ctx);
        return 0;
    }

    /* 创建解密上下文 */
    ctx->decrypt_ctx = tecc_sm4_context_new();
    if (!ctx->decrypt_ctx)
    {
        tecc_sm4_context_free(ctx->encrypt_ctx);
        pfree(ctx);
        return 0;
    }

    /* 初始化SM4-ECB解密密钥 */
    if (tecc_sm4_set_key_decrypt(ctx->decrypt_ctx, key) != 1)
    {
        tecc_sm4_context_free(ctx->encrypt_ctx);
        tecc_sm4_context_free(ctx->decrypt_ctx);
        pfree(ctx);
        return 0;
    }

    *sm4_key = ctx;
    return 1;
}

/* SM4加密（16字节块，ECB模式，无填充） */
void
SM4_Encrypt(const uint8_t *plaintext, uint8_t *ciphertext, const SM4_Key sm4_key)
{
    if (!plaintext || !ciphertext || !sm4_key || !sm4_key->encrypt_ctx)
    {
        return;
    }

    tecc_sm4_ecb_encrypt(sm4_key->encrypt_ctx, plaintext, ciphertext);
}

/* SM4解密（16字节块，ECB模式，无填充） */
void
SM4_Decrypt(const uint8_t *ciphertext, uint8_t *plaintext, const SM4_Key sm4_key)
{
    if (!ciphertext || !plaintext || !sm4_key || !sm4_key->decrypt_ctx)
    {
        return;
    }

    tecc_sm4_ecb_decrypt(sm4_key->decrypt_ctx, ciphertext, plaintext);
}

/* 删除SM4密钥 */
void
SM4_KeyDelete(SM4_Key sm4_key)
{
    if (!sm4_key)
    {
        return;
    }

    if (sm4_key->encrypt_ctx)
    {
        tecc_sm4_context_free(sm4_key->encrypt_ctx);
    }

    if (sm4_key->decrypt_ctx)
    {
        tecc_sm4_context_free(sm4_key->decrypt_ctx);
    }

    /* 清除敏感数据 */
    memset(sm4_key->key, 0, sizeof(sm4_key->key));

    pfree(sm4_key);
}

/* End - ReqID:TECC4.0 */
