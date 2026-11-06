/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_sm4_internal.c
 *
 * Abstract:
 *   Internal SM4 block cipher implementation (GM/T 0002-2012).
 *   128-bit block, 128-bit key, 32-round Feistel-like structure.
 *   Pure C, no external dependencies (no OpenSSL).
 *
 * Authored by <wangxp@highgo.com>, 20260508.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/common/tecc_algorithm/tecc_sm4_internal.c
 */

#include "../../include/common/tecc_algorithm/tecc_sm4_internal.h"
#include <stdlib.h>
#include <string.h>

/* SM4 S-box (GM/T 0002-2012, Section 6.6) */
static const unsigned char sm4_sbox[256] = {
    0xd6, 0x90, 0xe9, 0xfe, 0xcc, 0xe1, 0x3d, 0xb7,
    0x16, 0xb6, 0x14, 0xc2, 0x28, 0xfb, 0x2c, 0x05,
    0x2b, 0x67, 0x9a, 0x76, 0x2a, 0xbe, 0x04, 0xc3,
    0xaa, 0x44, 0x13, 0x26, 0x49, 0x86, 0x06, 0x99,
    0x9c, 0x42, 0x50, 0xf4, 0x91, 0xef, 0x98, 0x7a,
    0x33, 0x54, 0x0b, 0x43, 0xed, 0xcf, 0xac, 0x62,
    0xe4, 0xb3, 0x1c, 0xa9, 0xc9, 0x08, 0xe8, 0x95,
    0x80, 0xdf, 0x94, 0xfa, 0x75, 0x8f, 0x3f, 0xa6,
    0x47, 0x07, 0xa7, 0xfc, 0xf3, 0x73, 0x17, 0xba,
    0x83, 0x59, 0x3c, 0x19, 0xe6, 0x85, 0x4f, 0xa8,
    0x68, 0x6b, 0x81, 0xb2, 0x71, 0x64, 0xda, 0x8b,
    0xf8, 0xeb, 0x0f, 0x4b, 0x70, 0x56, 0x9d, 0x35,
    0x1e, 0x24, 0x0e, 0x5e, 0x63, 0x58, 0xd1, 0xa2,
    0x25, 0x22, 0x7c, 0x3b, 0x01, 0x21, 0x78, 0x87,
    0xd4, 0x00, 0x46, 0x57, 0x9f, 0xd3, 0x27, 0x52,
    0x4c, 0x36, 0x02, 0xe7, 0xa0, 0xc4, 0xc8, 0x9e,
    0xea, 0xbf, 0x8a, 0xd2, 0x40, 0xc7, 0x38, 0xb5,
    0xa3, 0xf7, 0xf2, 0xce, 0xf9, 0x61, 0x15, 0xa1,
    0xe0, 0xae, 0x5d, 0xa4, 0x9b, 0x34, 0x1a, 0x55,
    0xad, 0x93, 0x32, 0x30, 0xf5, 0x8c, 0xb1, 0xe3,
    0x1d, 0xf6, 0xe2, 0x2e, 0x82, 0x66, 0xca, 0x60,
    0xc0, 0x29, 0x23, 0xab, 0x0d, 0x53, 0x4e, 0x6f,
    0xd5, 0xdb, 0x37, 0x45, 0xde, 0xfd, 0x8e, 0x2f,
    0x03, 0xff, 0x6a, 0x72, 0x6d, 0x6c, 0x5b, 0x51,
    0x8d, 0x1b, 0xaf, 0x92, 0xbb, 0xdd, 0xbc, 0x7f,
    0x11, 0xd9, 0x5c, 0x41, 0x1f, 0x10, 0x5a, 0xd8,
    0x0a, 0xc1, 0x31, 0x88, 0xa5, 0xcd, 0x7b, 0xbd,
    0x2d, 0x74, 0xd0, 0x12, 0xb8, 0xe5, 0xb4, 0xb0,
    0x89, 0x69, 0x97, 0x4a, 0x0c, 0x96, 0x77, 0x7e,
    0x65, 0xb9, 0xf1, 0x09, 0xc5, 0x6e, 0xc6, 0x84,
    0x18, 0xf0, 0x7d, 0xec, 0x3a, 0xdc, 0x4d, 0x20,
    0x79, 0xee, 0x5f, 0x3e, 0xd7, 0xcb, 0x39, 0x48
};

/* FK constants for key expansion (GM/T 0002-2012) */
#define SM4_FK0 0xA3B1BAC6U
#define SM4_FK1 0x56AA3350U
#define SM4_FK2 0x677D9197U
#define SM4_FK3 0xB27022DCU

/* CK constants for key expansion (GM/T 0002-2012, Section 7.2) */
static const unsigned int sm4_ck[32] = {
    0x00070E15U, 0x1C232A31U, 0x383F464DU, 0x545B6269U,
    0x70777E85U, 0x8C939AA1U, 0xA8AFB6BDU, 0xC4CBD2D9U,
    0xE0E7EEF5U, 0xFC030A11U, 0x181F262DU, 0x343B4249U,
    0x50575E65U, 0x6C737A81U, 0x888F969DU, 0xA4ABB2B9U,
    0xC0C7CED5U, 0xDCE3EAF1U, 0xF8FF060DU, 0x141B2229U,
    0x30373E45U, 0x4C535A61U, 0x686F767DU, 0x848B9299U,
    0xA0A7AEB5U, 0xBCC3CAD1U, 0xD8DFE6EDU, 0xF4FB0209U,
    0x10171E25U, 0x2C333A41U, 0x484F565DU, 0x646B7279U
};

/* Internal context structure */
struct TeccSM4Context {
    unsigned int rk[32];
    int key_set;
};

/* ---- 32-bit helpers ---- */

static unsigned int
rotl32(unsigned int x, int n)
{
    return (x << n) | (x >> (32 - n));
}

static unsigned int
get32_be(const unsigned char b[4])
{
    return ((unsigned int)b[0] << 24) |
           ((unsigned int)b[1] << 16) |
           ((unsigned int)b[2] <<  8) |
           ((unsigned int)b[3]);
}

static void
put32_be(unsigned char b[4], unsigned int v)
{
    b[0] = (unsigned char)(v >> 24);
    b[1] = (unsigned char)(v >> 16);
    b[2] = (unsigned char)(v >>  8);
    b[3] = (unsigned char)(v);
}

/* τ: S-box substitution on 4 bytes in parallel */
static unsigned int
sm4_tau(unsigned int w)
{
    return ((unsigned int)sm4_sbox[(w >> 24) & 0xFF] << 24) |
           ((unsigned int)sm4_sbox[(w >> 16) & 0xFF] << 16) |
           ((unsigned int)sm4_sbox[(w >>  8) & 0xFF] <<  8) |
           ((unsigned int)sm4_sbox[ w        & 0xFF]);
}

/* L: linear transform for round function T */
static unsigned int
sm4_l(unsigned int b)
{
    return b ^ rotl32(b, 2) ^ rotl32(b, 10) ^ rotl32(b, 18) ^ rotl32(b, 24);
}

/* L': linear transform for key expansion T' */
static unsigned int
sm4_l_prime(unsigned int b)
{
    return b ^ rotl32(b, 13) ^ rotl32(b, 23);
}

/* T: round function composition = L ∘ τ */
static unsigned int
sm4_t(unsigned int x)
{
    return sm4_l(sm4_tau(x));
}

/* T': key expansion composition = L' ∘ τ */
static unsigned int
sm4_t_prime(unsigned int x)
{
    return sm4_l_prime(sm4_tau(x));
}

/*
 * Key expansion: generate 32 round keys
 */
static void
sm4_key_expand(const unsigned char key[16], unsigned int rk[32])
{
    unsigned int k[36];  /* K[0..35] */
    int i;

    k[0] = get32_be(key)      ^ SM4_FK0;
    k[1] = get32_be(key + 4)  ^ SM4_FK1;
    k[2] = get32_be(key + 8)  ^ SM4_FK2;
    k[3] = get32_be(key + 12) ^ SM4_FK3;

    for (i = 0; i < 32; i++)
    {
        k[i + 4] = k[i] ^ sm4_t_prime(k[i + 1] ^ k[i + 2] ^ k[i + 3] ^ sm4_ck[i]);
        rk[i] = k[i + 4];
    }
}

/*
 * SM4 round function: apply 32 rounds on X[0..3], write to out[0..3]
 */
static void
sm4_rounds(const unsigned int rk[32],
           const unsigned int x[4],
           unsigned int out[4])
{
    unsigned int xv[36];
    int i;

    xv[0] = x[0];
    xv[1] = x[1];
    xv[2] = x[2];
    xv[3] = x[3];

    for (i = 0; i < 32; i++)
        xv[i + 4] = xv[i] ^ sm4_t(xv[i + 1] ^ xv[i + 2] ^ xv[i + 3] ^ rk[i]);

    /* ciphertext in reverse order: (X[35], X[34], X[33], X[32]) */
    out[0] = xv[35];
    out[1] = xv[34];
    out[2] = xv[33];
    out[3] = xv[32];
}

/* ---- Public API ---- */

TeccSM4Context *
tecc_sm4_context_new(void)
{
    TeccSM4Context *ctx;

    ctx = (TeccSM4Context *)malloc(sizeof(TeccSM4Context));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(TeccSM4Context));
    ctx->key_set = 0;
    return ctx;
}

void
tecc_sm4_context_free(TeccSM4Context *ctx)
{
    if (ctx != NULL)
    {
        memset(ctx, 0, sizeof(TeccSM4Context));
        free(ctx);
    }
}

int
tecc_sm4_set_key_encrypt(TeccSM4Context *ctx, const unsigned char key[16])
{
    if (ctx == NULL || key == NULL)
        return 0;
    sm4_key_expand(key, ctx->rk);
    ctx->key_set = 1;
    return 1;
}

int
tecc_sm4_set_key_decrypt(TeccSM4Context *ctx, const unsigned char key[16])
{
    unsigned int enc_rk[32];
    int i;

    if (ctx == NULL || key == NULL)
        return 0;
    sm4_key_expand(key, enc_rk);
    /* reverse round key order for decryption */
    for (i = 0; i < 32; i++)
        ctx->rk[i] = enc_rk[31 - i];
    ctx->key_set = 1;
    return 1;
}

int
tecc_sm4_ecb_encrypt(TeccSM4Context *ctx,
                     const unsigned char in[16],
                     unsigned char out[16])
{
    unsigned int x[4];
    unsigned int y[4];

    if (ctx == NULL || !ctx->key_set || in == NULL || out == NULL)
        return 0;

    x[0] = get32_be(in);
    x[1] = get32_be(in + 4);
    x[2] = get32_be(in + 8);
    x[3] = get32_be(in + 12);

    sm4_rounds(ctx->rk, x, y);

    put32_be(out,      y[0]);
    put32_be(out + 4,  y[1]);
    put32_be(out + 8,  y[2]);
    put32_be(out + 12, y[3]);

    return 1;
}

int
tecc_sm4_ecb_decrypt(TeccSM4Context *ctx,
                     const unsigned char in[16],
                     unsigned char out[16])
{
    /* Same as encrypt because decryption uses reversed round keys */
    return tecc_sm4_ecb_encrypt(ctx, in, out);
}

int
tecc_sm4_cbc_encrypt(TeccSM4Context *ctx,
                     const unsigned char iv[16],
                     const unsigned char *in, size_t len,
                     unsigned char *out)
{
    unsigned char prev[16];
    size_t n;
    int i;

    if (ctx == NULL || !ctx->key_set || iv == NULL || in == NULL || out == NULL)
        return 0;
    if (len == 0 || len % 16 != 0)
        return 0;

    memcpy(prev, iv, 16);

    for (n = 0; n < len; n += 16)
    {
        unsigned char block[16];

        for (i = 0; i < 16; i++)
            block[i] = in[n + i] ^ prev[i];

        if (!tecc_sm4_ecb_encrypt(ctx, block, out + n))
            return 0;

        memcpy(prev, out + n, 16);
    }

    return 1;
}

int
tecc_sm4_cbc_decrypt(TeccSM4Context *ctx,
                     const unsigned char iv[16],
                     const unsigned char *in, size_t len,
                     unsigned char *out)
{
    unsigned char prev[16];
    unsigned char next_prev[16];
    size_t n;
    int i;

    if (ctx == NULL || !ctx->key_set || iv == NULL || in == NULL || out == NULL)
        return 0;
    if (len == 0 || len % 16 != 0)
        return 0;

    memcpy(prev, iv, 16);

    for (n = 0; n < len; n += 16)
    {
        unsigned char block[16];

        memcpy(next_prev, in + n, 16);

        if (!tecc_sm4_ecb_decrypt(ctx, in + n, block))
            return 0;

        for (i = 0; i < 16; i++)
            out[n + i] = block[i] ^ prev[i];

        memcpy(prev, next_prev, 16);
    }

    return 1;
}
/* End - ReqID:TECC4.0 */
