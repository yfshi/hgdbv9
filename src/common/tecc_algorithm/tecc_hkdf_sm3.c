/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_hkdf_sm3.c
 *
 * Abstract:
 *   HKDF-SM3 implementation for TECC algorithm module.
 *   HMAC-based Key Derivation Function using SM3 hash algorithm.
 *
 * Authored by <wangxp@highgo.com>, 20260406.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/common/tecc_algorithm/tecc_hkdf_sm3.c
 */

#include "../../include/common/tecc_algorithm/tecc_algorithm_api.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../../include/common/tecc_algorithm/tecc_sm3_internal.h"

/* HKDF-SM3 constants */
#define SM3_HASH_LEN 32
#define SM3_BLOCK_SIZE 64

/*
 * HMAC-SM3 implementation for HKDF
 */
static int
hmac_sm3_core(const unsigned char *key, size_t key_len,
              const unsigned char *data, size_t data_len,
              unsigned char *mac)
{
    if (!key || key_len == 0 || !data || data_len == 0 || !mac)
        return -1;

    if (tecc_sm3_hmac(key, key_len, data, data_len, mac) != 1)
        return -1;

    return 0;
}

/*
 * HKDF-SM3 extract step
 */
static int
hkdf_sm3_extract(const unsigned char *salt, size_t salt_len,
                 const unsigned char *ikm, size_t ikm_len,
                 unsigned char *prk)
{
    if (!salt || salt_len == 0)
    {
        /* Use zero salt if not provided */
        unsigned char zero_salt[SM3_HASH_LEN] = {0};
        return hmac_sm3_core(zero_salt, SM3_HASH_LEN, ikm, ikm_len, prk);
    }

    return hmac_sm3_core(salt, salt_len, ikm, ikm_len, prk);
}

/*
 * HKDF-SM3 expand step
 */
static int
hkdf_sm3_expand(const unsigned char *prk, size_t prk_len,
                const unsigned char *info, size_t info_len,
                unsigned char *okm, size_t okm_len)
{
    unsigned char t[SM3_HASH_LEN];
    unsigned char counter;
    size_t n;
    size_t remaining_len;
    size_t offset = 0;
    int result;
    unsigned char buf[SM3_BLOCK_SIZE];
    size_t buf_len;

    if (!prk || prk_len == 0 || !okm || okm_len == 0)
        return -1;

    n = (okm_len + SM3_HASH_LEN - 1) / SM3_HASH_LEN;
    if (n > 255)
        return -1;  /* Too much output requested */

    for (counter = 1; counter <= n; counter++)
    {
        /* Prepare T(i) input */
        if (counter == 1)
        {
            /* T(1) = HMAC(PRK, info || 0x01) */
            if (info_len > 0)
                memcpy(buf, info, info_len);
            buf[info_len] = counter;
            buf_len = info_len + 1;
        }
        else
        {
            /* T(i) = HMAC(PRK, T(i-1) || info || i) */
            memcpy(buf, t, SM3_HASH_LEN);
            if (info_len > 0)
                memcpy(buf + SM3_HASH_LEN, info, info_len);
            buf[SM3_HASH_LEN + info_len] = counter;
            buf_len = SM3_HASH_LEN + info_len + 1;
        }

        result = hmac_sm3_core(prk, prk_len, buf, buf_len, t);
        if (result != 0)
            return result;

        remaining_len = okm_len - offset;
        if (remaining_len > SM3_HASH_LEN)
            remaining_len = SM3_HASH_LEN;

        memcpy(okm + offset, t, remaining_len);
        offset += remaining_len;
    }

    return 0;
}

/*
 * HKDF-SM3 key derivation function
 * Implements HKDF as per RFC 5869 using SM3 hash
 */
int hkdf_sm3_derive(const unsigned char *salt, size_t salt_len,
                    const unsigned char *ikm, size_t ikm_len,
                    const unsigned char *info, size_t info_len,
                    unsigned char *okm, size_t okm_len);

int
hkdf_sm3_derive(const unsigned char *salt, size_t salt_len,
                const unsigned char *ikm, size_t ikm_len,
                const unsigned char *info, size_t info_len,
                unsigned char *okm, size_t okm_len)
{
    unsigned char prk[SM3_HASH_LEN];
    int result;

    if (!ikm || ikm_len == 0 || !okm || okm_len == 0)
        return -1;

    /* Extract phase */
    result = hkdf_sm3_extract(salt, salt_len, ikm, ikm_len, prk);
    if (result != 0)
        return result;

    /* Expand phase */
    result = hkdf_sm3_expand(prk, sizeof(prk), info, info_len, okm, okm_len);

    /* Clear intermediate PRK */
    memset(prk, 0, sizeof(prk));

    return result;
}

/* End - ReqID:TECC4.0 */