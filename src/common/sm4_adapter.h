/*
 * File: sm4_adapter.h
 * Abstract: SM4算法适配器头文件 - 为OPE算法提供兼容的SM4接口
 * Authored by <wangxp@highgo.com>, 20260312.
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 * Identification: src/common/crypto/sm4_adapter.h
 */

#ifndef SM4_ADAPTER_H
#define SM4_ADAPTER_H

#include "postgres.h"
#include "c.h"
#include <stdint.h>

/* ReqID:TECC4.0 */

/* Begin - ReqID:TECC4.0 */

/* Forward declaration for internal SM4 context */
typedef struct TeccSM4Context TeccSM4Context;

/* SM4密钥上下文结构 */
struct SM4_KeyContext
{
    TeccSM4Context *encrypt_ctx;  /* 加密上下文 */
    TeccSM4Context *decrypt_ctx;  /* 解密上下文 */
    unsigned char key[16];        /* 原始密钥 (16字节) */
};

/* SM4密钥类型定义（兼容OPE算法） */
typedef struct SM4_KeyContext *SM4_Key;

/* SM4函数声明 */
extern int SM4_KeyInit(uint8_t *key, SM4_Key *sm4_key);
extern void SM4_Encrypt(const uint8_t *plaintext, uint8_t *ciphertext, const SM4_Key sm4_key);
extern void SM4_Decrypt(const uint8_t *ciphertext, uint8_t *plaintext, const SM4_Key sm4_key);
extern void SM4_KeyDelete(SM4_Key sm4_key);

/* SM4块大小常量 */
#define SM4_BLOCK_SIZE 16
#define SM4_KEY_SIZE 16

/* End - ReqID:TECC4.0 */

#endif /* SM4_ADAPTER_H */
