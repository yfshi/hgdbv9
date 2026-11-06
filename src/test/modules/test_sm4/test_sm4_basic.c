/*
 * File: test_sm4_basic.c
 * Abstract: SM4算法基础测试程序
 * Authored by <wangxp@highgo.com>, 20260312.
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 * Identification: src/test/modules/test_sm4/test_sm4_basic.c
 */

#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include "../../../include/common/tecc_algorithm/tecc_sm4_rnd.h"
#include "../../../include/common/tecc_algorithm/tecc_sm3_internal.h"

#include <stdio.h>
#include <string.h>

PG_MODULE_MAGIC;

/* ReqID:TECC4.0 */

/* Begin - ReqID:TECC4.0 */

/* 测试SM3哈希函数 */
static void
test_sm3_hash(void)
{
    unsigned char hash[SM3_HASH_LEN];
    char hex_hash[SM3_HASH_LEN * 2 + 1];
    const char *test_data = "abc";
    const char *expected = "66c7f0f462eeedd9d1f2d46bdc10e4e24167c4875cf2f7a2297da02b8f4ba8e0";

    elog(INFO, "Testing SM3 hash function...");

    if (tecc_sm3_hash((const unsigned char *)test_data, strlen(test_data),
                     hash, sizeof(hash)) != 0)
    {
        elog(ERROR, "SM3 hash function failed");
    }

    /* 转换为十六进制字符串 */
    {
        size_t i;
        for (i = 0; i < sizeof(hash); i++)
        {
            /* Use snprintf to avoid buffer overrun */
            snprintf(hex_hash + i * 2, 3, "%02x", hash[i]);
        }
    }
    hex_hash[sizeof(hash) * 2] = '\0';

    elog(INFO, "SM3 hash of '%s': %s", test_data, hex_hash);
    elog(INFO, "Expected hash: %s", expected);

    if (strcasecmp(hex_hash, expected) != 0)
    {
        elog(ERROR, "SM3 hash test failed");
    }

    elog(INFO, "SM3 hash test passed");
}

/* 测试HMAC-SM3函数 */
static void
test_hmac_sm3(void)
{
    unsigned char key[] = "test-key-12345678";
    unsigned char data[] = "test-data-for-hmac";
    unsigned char mac[SM3_HASH_LEN];

    elog(INFO, "Testing HMAC-SM3 function...");

    if (tecc_sm3_hmac(key, strlen((char *)key),
                     data, strlen((char *)data),
                     mac, sizeof(mac)) != 0)
    {
        elog(ERROR, "HMAC-SM3 function failed");
    }

    elog(INFO, "HMAC-SM3 test passed");
}

/* 测试HKDF-SM3函数 */
static void
test_hkdf_sm3(void)
{
    unsigned char salt[] = "test-salt";
    unsigned char ikm[] = "test-input-key-material";
    unsigned char info[] = "test-info";
    unsigned char okm[32];

    elog(INFO, "Testing HKDF-SM3 function...");

    if (hkdf_sm3_derive(salt, strlen((char *)salt),
                     ikm, strlen((char *)ikm),
                     info, strlen((char *)info),
                     okm, sizeof(okm)) != 0)
    {
        elog(ERROR, "HKDF-SM3 function failed");
    }

    elog(INFO, "HKDF-SM3 test passed");
}

/* 测试SM4-RND加密解密 */
static void
test_sm4_rnd_encrypt_decrypt(void)
{
    unsigned char key[] = "1234567890123456"; /* 16字节密钥 */
    unsigned char plaintext[] = "Hello, SM4-RND encryption!";
    unsigned char *ciphertext = NULL;
    unsigned char *decrypted = NULL;
    size_t cipher_len = 0;
    size_t decrypted_len = 0;
    int result;

    elog(INFO, "Testing SM4-RND encrypt/decrypt...");
    elog(INFO, "Plaintext: %s", plaintext);
    elog(INFO, "Plaintext length: %zu", strlen((char *)plaintext));

    /* 加密 */
    {
        size_t buf_size = tecc_sm4_rnd_calc_encrypt_buffer_size(strlen((char *)plaintext));
        ciphertext = (unsigned char *)palloc(buf_size);
        result = tecc_sm4_rnd_encrypt(key, sizeof(key) - 1,
                                     plaintext, strlen((char *)plaintext),
                                     ciphertext, buf_size, &cipher_len);
    }
    if (result != SM4_RND_SUCCESS)
    {
        elog(ERROR, "SM4-RND encryption failed: %d", result);
    }

    elog(INFO, "Encryption successful, ciphertext length: %zu", cipher_len);

    /* 验证数据格式 */
    result = tecc_sm4_rnd_validate_format(ciphertext, cipher_len);
    if (result != SM4_RND_SUCCESS)
    {
        pfree(ciphertext);
        elog(ERROR, "SM4-RND format validation failed: %d", result);
    }

    elog(INFO, "Format validation passed");

    /* 解密 */
    {
        size_t buf_size = tecc_sm4_rnd_calc_decrypt_buffer_size(cipher_len);
        decrypted = (unsigned char *)palloc(buf_size);
        result = tecc_sm4_rnd_decrypt(key, sizeof(key) - 1,
                                     ciphertext, cipher_len,
                                     decrypted, buf_size, &decrypted_len);
    }
    if (result != SM4_RND_SUCCESS)
    {
        pfree(ciphertext);
        elog(ERROR, "SM4-RND decryption failed: %d", result);
    }

    elog(INFO, "Decryption successful, decrypted length: %zu", decrypted_len);

    /* 验证解密结果 */
    if (decrypted_len != strlen((char *)plaintext))
    {
        pfree(ciphertext);
        pfree(decrypted);
        elog(ERROR, "Decrypted length mismatch: expected %zu, got %zu",
             strlen((char *)plaintext), decrypted_len);
    }

    if (memcmp(plaintext, decrypted, decrypted_len) != 0)
    {
        pfree(ciphertext);
        pfree(decrypted);
        elog(ERROR, "Decrypted data does not match original");
    }

    elog(INFO, "Decrypted text: %.*s", (int)decrypted_len, decrypted);
    elog(INFO, "SM4-RND encrypt/decrypt test passed");

    /* 清理 */
    pfree(ciphertext);
    pfree(decrypted);
}

/* 测试SM4-RND不同数据长度 */
static void
test_sm4_rnd_various_lengths(void)
{
    unsigned char key[] = "test-key-16-bytes";
    int test_lengths[] = {0, 1, 15, 16, 17, 31, 32, 100, 1000};
    int num_tests = sizeof(test_lengths) / sizeof(test_lengths[0]);
    int i;

    elog(INFO, "Testing SM4-RND with various data lengths...");

    for (i = 0; i < num_tests; i++)
    {
        int len = test_lengths[i];
        unsigned char *plaintext = NULL;
        unsigned char *ciphertext = NULL;
        unsigned char *decrypted = NULL;
        size_t cipher_len = 0;
        size_t decrypted_len = 0;
        int result;

        /* 分配和初始化明文 */
        plaintext = (unsigned char *)palloc(len + 1);
        if (len > 0)
        {
            memset(plaintext, 'A' + (i % 26), len);
        }
        plaintext[len] = '\0';

        /* 加密 */
        {
            size_t buf_size = tecc_sm4_rnd_calc_encrypt_buffer_size(len);
            ciphertext = (unsigned char *)palloc(buf_size);
            result = tecc_sm4_rnd_encrypt(key, sizeof(key) - 1,
                                         plaintext, len,
                                         ciphertext, buf_size, &cipher_len);
        }
        if (result != SM4_RND_SUCCESS)
        {
            pfree(plaintext);
            elog(ERROR, "SM4-RND encryption failed for length %d: %d", len, result);
        }

        /* 解密 */
        {
            size_t buf_size = tecc_sm4_rnd_calc_decrypt_buffer_size(cipher_len);
            decrypted = (unsigned char *)palloc(buf_size);
            result = tecc_sm4_rnd_decrypt(key, sizeof(key) - 1,
                                         ciphertext, cipher_len,
                                         decrypted, buf_size, &decrypted_len);
        }
        if (result != SM4_RND_SUCCESS)
        {
            pfree(plaintext);
            pfree(ciphertext);
            elog(ERROR, "SM4-RND decryption failed for length %d: %d", len, result);
        }

        /* 验证 */
        if (decrypted_len != (size_t)len)
        {
            pfree(plaintext);
            pfree(ciphertext);
            pfree(decrypted);
            elog(ERROR, "Length mismatch for test %d: expected %d, got %zu",
                 i, len, decrypted_len);
        }

        if (len > 0 && memcmp(plaintext, decrypted, len) != 0)
        {
            pfree(plaintext);
            pfree(ciphertext);
            pfree(decrypted);
            elog(ERROR, "Data mismatch for test %d (length %d)", i, len);
        }

        /* 清理 */
        pfree(plaintext);
        pfree(ciphertext);
        pfree(decrypted);

        elog(INFO, "  Test passed for length %d", len);
    }

    elog(INFO, "All length tests passed");
}

/* 主测试函数 */
PG_FUNCTION_INFO_V1(test_sm4_all);
Datum
test_sm4_all(PG_FUNCTION_ARGS)
{
    elog(INFO, "Starting SM4 algorithm tests...");

    /* 测试SM3哈希 */
    test_sm3_hash();

    /* 测试HMAC-SM3 */
    test_hmac_sm3();

    /* 测试HKDF-SM3 */
    test_hkdf_sm3();

    /* 测试SM4-RND加密解密 */
    test_sm4_rnd_encrypt_decrypt();

    /* 测试不同数据长度 */
    test_sm4_rnd_various_lengths();

    elog(INFO, "All SM4 algorithm tests passed successfully!");

    PG_RETURN_VOID();
}

/* End - ReqID:TECC4.0 */