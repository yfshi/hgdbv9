/* Begin - ReqID:TECC4.0 */
/*
 * File: tecc_sm3_internal.c
 *
 * Abstract:
 *   Internal SM3 hash function implementation (GM/T 0004-2012).
 *   256-bit output, 512-bit block size, Merkle-Damgard construction.
 *   Includes HMAC-SM3 per RFC 2104. Pure C, no OpenSSL dependency.
 *
 * Authored by <wangxp@highgo.com>, 20260508.
 *
 * Copyright:
 *   Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *   src/common/tecc_algorithm/tecc_sm3_internal.c
 */

#include "../../include/common/tecc_algorithm/tecc_sm3_internal.h"
#include <stdlib.h>
#include <string.h>

/* SM3 IV constants (GM/T 0004-2012, Section 5.2) */
#define SM3_IV0 0x7380166fU
#define SM3_IV1 0x4914b2b9U
#define SM3_IV2 0x172442d7U
#define SM3_IV3 0xda8a0600U
#define SM3_IV4 0xa96f30bcU
#define SM3_IV5 0x163138aaU
#define SM3_IV6 0xe38dee4dU
#define SM3_IV7 0xb0fb0e4eU

/* SM3 T constants */
#define SM3_T0 0x79cc4519U  /* j = 0..15 */
#define SM3_T1 0x7a879d8aU  /* j = 16..63 */

/* ---- 32-bit helpers ---- */

static unsigned int
rotl32(unsigned int x, int n)
{
    return (x << n) | (x >> (32 - n));
}

static unsigned int
get32_be(const unsigned char b[4])
{
    unsigned int v;

    /* Read as native (little-endian on x86) then swap: compiles to a single
     * BSWAP instruction instead of 4 shifts + 3 ORs (measured ~30% of SM3
     * compression was byte-order conversion in the message expansion). */
    memcpy(&v, b, sizeof(v));
    return __builtin_bswap32(v);
}

static void
put32_be(unsigned char b[4], unsigned int v)
{
    v = __builtin_bswap32(v);
    memcpy(b, &v, sizeof(v));
}

/* ---- Boolean / permutation functions (GM/T 0004-2012, Section 4) ---- */

static unsigned int
sm3_ff0(unsigned int x, unsigned int y, unsigned int z)
{
    return x ^ y ^ z;
}

static unsigned int
sm3_ff1(unsigned int x, unsigned int y, unsigned int z)
{
    /* Optimized form (OpenSSL-style): (x&y)|((x|y)&z) — 3 ops vs 5 */
    return (x & y) | ((x | y) & z);
}

static unsigned int
sm3_gg0(unsigned int x, unsigned int y, unsigned int z)
{
    return x ^ y ^ z;
}

static unsigned int
sm3_gg1(unsigned int x, unsigned int y, unsigned int z)
{
    /* Optimized form (OpenSSL-style): z^(x&(y^z)) — 3 ops vs 4 */
    return z ^ (x & (y ^ z));
}

static unsigned int
sm3_p0(unsigned int x)
{
    return x ^ rotl32(x, 9) ^ rotl32(x, 17);
}

static unsigned int
sm3_p1(unsigned int x)
{
    return x ^ rotl32(x, 15) ^ rotl32(x, 23);
}

/* ---- SM3 context ---- */

struct TeccSM3Context {
    unsigned int  state[8];
    unsigned long long count;       /* total bits processed */
    unsigned char buffer[64];
    unsigned int  buflen;
};

TeccSM3Context *
tecc_sm3_context_new(void)
{
    TeccSM3Context *ctx;

    ctx = (TeccSM3Context *)malloc(sizeof(TeccSM3Context));
    if (ctx == NULL)
        return NULL;
    tecc_sm3_init(ctx);
    return ctx;
}

void
tecc_sm3_context_free(TeccSM3Context *ctx)
{
    if (ctx != NULL)
    {
        memset(ctx, 0, sizeof(TeccSM3Context));
        free(ctx);
    }
}

int
tecc_sm3_init(TeccSM3Context *ctx)
{
    if (ctx == NULL)
        return 0;
    ctx->state[0] = SM3_IV0;
    ctx->state[1] = SM3_IV1;
    ctx->state[2] = SM3_IV2;
    ctx->state[3] = SM3_IV3;
    ctx->state[4] = SM3_IV4;
    ctx->state[5] = SM3_IV5;
    ctx->state[6] = SM3_IV6;
    ctx->state[7] = SM3_IV7;
    ctx->count = 0;
    ctx->buflen = 0;
    memset(ctx->buffer, 0, 64);
    return 1;
}

/* Fully-unrolled SM3 round macros with register variables (OpenSSL-style
 * technique, self-implemented): W00..W15 hold the current message words and
 * SM3_EXPAND updates them in place; the round macro writes to B/D/F/H and the
 * caller passes rotated argument order so the compiler keeps state in
 * registers with zero moves. */
#define SM3_EXPAND(W0, W7, W13, W3, W10) \
    (sm3_p1((W0) ^ (W7) ^ rotl32((W13), 15)) ^ rotl32((W3), 7) ^ (W10))

#define SM3_RND(A, B, C, D, E, F, G, H, TJ, Wi, Wj, FF, GG) \
    do { \
        unsigned int a12 = rotl32((A), 12); \
        unsigned int ss1 = rotl32(a12 + (E) + (TJ), 7); \
        unsigned int tt1 = FF(A, B, C) + (D) + (ss1 ^ a12) + (Wj); \
        unsigned int tt2 = GG(E, F, G) + (H) + ss1 + (Wi); \
        (B) = rotl32((B), 9); (D) = tt1; (F) = rotl32((F), 19); (H) = sm3_p0(tt2); \
    } while (0)

#define SM3_R1(A, B, C, D, E, F, G, H, TJ, Wi, Wj) \
    SM3_RND(A, B, C, D, E, F, G, H, TJ, Wi, Wj, sm3_ff0, sm3_gg0)

#define SM3_R2(A, B, C, D, E, F, G, H, TJ, Wi, Wj) \
    SM3_RND(A, B, C, D, E, F, G, H, TJ, Wi, Wj, sm3_ff1, sm3_gg1)

/*
 * Process one 512-bit block through the compression function
 */
static void
sm3_compress(unsigned int state[8], const unsigned char block[64])
{
    unsigned int A, B, C, D, E, F, G, H;
    unsigned int W00, W01, W02, W03, W04, W05, W06, W07,
        W08, W09, W10, W11, W12, W13, W14, W15;

    /* load W[0..15] (message words, big-endian) */
    W00 = get32_be(block);
    W01 = get32_be(block + 4);
    W02 = get32_be(block + 8);
    W03 = get32_be(block + 12);
    W04 = get32_be(block + 16);
    W05 = get32_be(block + 20);
    W06 = get32_be(block + 24);
    W07 = get32_be(block + 28);
    W08 = get32_be(block + 32);
    W09 = get32_be(block + 36);
    W10 = get32_be(block + 40);
    W11 = get32_be(block + 44);
    W12 = get32_be(block + 48);
    W13 = get32_be(block + 52);
    W14 = get32_be(block + 56);
    W15 = get32_be(block + 60);

    A = state[0]; B = state[1]; C = state[2]; D = state[3];
    E = state[4]; F = state[5]; G = state[6]; H = state[7];

    /* 64 rounds fully unrolled with register variables (OpenSSL-style):
     * W00..W15 hold W[j..j+15]; SM3_EXPAND updates them in place after each
     * round so the whole message schedule stays in registers. */
        SM3_R1(A, B, C, D, E, F, G, H, 0x79CC4519, W00, W00 ^ W04);
        W00 = SM3_EXPAND(W00, W07, W13, W03, W10);
        SM3_R1(D, A, B, C, H, E, F, G, 0xF3988A32, W01, W01 ^ W05);
        W01 = SM3_EXPAND(W01, W08, W14, W04, W11);
        SM3_R1(C, D, A, B, G, H, E, F, 0xE7311465, W02, W02 ^ W06);
        W02 = SM3_EXPAND(W02, W09, W15, W05, W12);
        SM3_R1(B, C, D, A, F, G, H, E, 0xCE6228CB, W03, W03 ^ W07);
        W03 = SM3_EXPAND(W03, W10, W00, W06, W13);
        SM3_R1(A, B, C, D, E, F, G, H, 0x9CC45197, W04, W04 ^ W08);
        W04 = SM3_EXPAND(W04, W11, W01, W07, W14);
        SM3_R1(D, A, B, C, H, E, F, G, 0x3988A32F, W05, W05 ^ W09);
        W05 = SM3_EXPAND(W05, W12, W02, W08, W15);
        SM3_R1(C, D, A, B, G, H, E, F, 0x7311465E, W06, W06 ^ W10);
        W06 = SM3_EXPAND(W06, W13, W03, W09, W00);
        SM3_R1(B, C, D, A, F, G, H, E, 0xE6228CBC, W07, W07 ^ W11);
        W07 = SM3_EXPAND(W07, W14, W04, W10, W01);
        SM3_R1(A, B, C, D, E, F, G, H, 0xCC451979, W08, W08 ^ W12);
        W08 = SM3_EXPAND(W08, W15, W05, W11, W02);
        SM3_R1(D, A, B, C, H, E, F, G, 0x988A32F3, W09, W09 ^ W13);
        W09 = SM3_EXPAND(W09, W00, W06, W12, W03);
        SM3_R1(C, D, A, B, G, H, E, F, 0x311465E7, W10, W10 ^ W14);
        W10 = SM3_EXPAND(W10, W01, W07, W13, W04);
        SM3_R1(B, C, D, A, F, G, H, E, 0x6228CBCE, W11, W11 ^ W15);
        W11 = SM3_EXPAND(W11, W02, W08, W14, W05);
        SM3_R1(A, B, C, D, E, F, G, H, 0xC451979C, W12, W12 ^ W00);
        W12 = SM3_EXPAND(W12, W03, W09, W15, W06);
        SM3_R1(D, A, B, C, H, E, F, G, 0x88A32F39, W13, W13 ^ W01);
        W13 = SM3_EXPAND(W13, W04, W10, W00, W07);
        SM3_R1(C, D, A, B, G, H, E, F, 0x11465E73, W14, W14 ^ W02);
        W14 = SM3_EXPAND(W14, W05, W11, W01, W08);
        SM3_R1(B, C, D, A, F, G, H, E, 0x228CBCE6, W15, W15 ^ W03);
        W15 = SM3_EXPAND(W15, W06, W12, W02, W09);
        SM3_R2(A, B, C, D, E, F, G, H, 0x9D8A7A87, W00, W00 ^ W04);
        W00 = SM3_EXPAND(W00, W07, W13, W03, W10);
        SM3_R2(D, A, B, C, H, E, F, G, 0x3B14F50F, W01, W01 ^ W05);
        W01 = SM3_EXPAND(W01, W08, W14, W04, W11);
        SM3_R2(C, D, A, B, G, H, E, F, 0x7629EA1E, W02, W02 ^ W06);
        W02 = SM3_EXPAND(W02, W09, W15, W05, W12);
        SM3_R2(B, C, D, A, F, G, H, E, 0xEC53D43C, W03, W03 ^ W07);
        W03 = SM3_EXPAND(W03, W10, W00, W06, W13);
        SM3_R2(A, B, C, D, E, F, G, H, 0xD8A7A879, W04, W04 ^ W08);
        W04 = SM3_EXPAND(W04, W11, W01, W07, W14);
        SM3_R2(D, A, B, C, H, E, F, G, 0xB14F50F3, W05, W05 ^ W09);
        W05 = SM3_EXPAND(W05, W12, W02, W08, W15);
        SM3_R2(C, D, A, B, G, H, E, F, 0x629EA1E7, W06, W06 ^ W10);
        W06 = SM3_EXPAND(W06, W13, W03, W09, W00);
        SM3_R2(B, C, D, A, F, G, H, E, 0xC53D43CE, W07, W07 ^ W11);
        W07 = SM3_EXPAND(W07, W14, W04, W10, W01);
        SM3_R2(A, B, C, D, E, F, G, H, 0x8A7A879D, W08, W08 ^ W12);
        W08 = SM3_EXPAND(W08, W15, W05, W11, W02);
        SM3_R2(D, A, B, C, H, E, F, G, 0x14F50F3B, W09, W09 ^ W13);
        W09 = SM3_EXPAND(W09, W00, W06, W12, W03);
        SM3_R2(C, D, A, B, G, H, E, F, 0x29EA1E76, W10, W10 ^ W14);
        W10 = SM3_EXPAND(W10, W01, W07, W13, W04);
        SM3_R2(B, C, D, A, F, G, H, E, 0x53D43CEC, W11, W11 ^ W15);
        W11 = SM3_EXPAND(W11, W02, W08, W14, W05);
        SM3_R2(A, B, C, D, E, F, G, H, 0xA7A879D8, W12, W12 ^ W00);
        W12 = SM3_EXPAND(W12, W03, W09, W15, W06);
        SM3_R2(D, A, B, C, H, E, F, G, 0x4F50F3B1, W13, W13 ^ W01);
        W13 = SM3_EXPAND(W13, W04, W10, W00, W07);
        SM3_R2(C, D, A, B, G, H, E, F, 0x9EA1E762, W14, W14 ^ W02);
        W14 = SM3_EXPAND(W14, W05, W11, W01, W08);
        SM3_R2(B, C, D, A, F, G, H, E, 0x3D43CEC5, W15, W15 ^ W03);
        W15 = SM3_EXPAND(W15, W06, W12, W02, W09);
        SM3_R2(A, B, C, D, E, F, G, H, 0x7A879D8A, W00, W00 ^ W04);
        W00 = SM3_EXPAND(W00, W07, W13, W03, W10);
        SM3_R2(D, A, B, C, H, E, F, G, 0xF50F3B14, W01, W01 ^ W05);
        W01 = SM3_EXPAND(W01, W08, W14, W04, W11);
        SM3_R2(C, D, A, B, G, H, E, F, 0xEA1E7629, W02, W02 ^ W06);
        W02 = SM3_EXPAND(W02, W09, W15, W05, W12);
        SM3_R2(B, C, D, A, F, G, H, E, 0xD43CEC53, W03, W03 ^ W07);
        W03 = SM3_EXPAND(W03, W10, W00, W06, W13);
        SM3_R2(A, B, C, D, E, F, G, H, 0xA879D8A7, W04, W04 ^ W08);
        W04 = SM3_EXPAND(W04, W11, W01, W07, W14);
        SM3_R2(D, A, B, C, H, E, F, G, 0x50F3B14F, W05, W05 ^ W09);
        W05 = SM3_EXPAND(W05, W12, W02, W08, W15);
        SM3_R2(C, D, A, B, G, H, E, F, 0xA1E7629E, W06, W06 ^ W10);
        W06 = SM3_EXPAND(W06, W13, W03, W09, W00);
        SM3_R2(B, C, D, A, F, G, H, E, 0x43CEC53D, W07, W07 ^ W11);
        W07 = SM3_EXPAND(W07, W14, W04, W10, W01);
        SM3_R2(A, B, C, D, E, F, G, H, 0x879D8A7A, W08, W08 ^ W12);
        W08 = SM3_EXPAND(W08, W15, W05, W11, W02);
        SM3_R2(D, A, B, C, H, E, F, G, 0x0F3B14F5, W09, W09 ^ W13);
        W09 = SM3_EXPAND(W09, W00, W06, W12, W03);
        SM3_R2(C, D, A, B, G, H, E, F, 0x1E7629EA, W10, W10 ^ W14);
        W10 = SM3_EXPAND(W10, W01, W07, W13, W04);
        SM3_R2(B, C, D, A, F, G, H, E, 0x3CEC53D4, W11, W11 ^ W15);
        W11 = SM3_EXPAND(W11, W02, W08, W14, W05);
        SM3_R2(A, B, C, D, E, F, G, H, 0x79D8A7A8, W12, W12 ^ W00);
        W12 = SM3_EXPAND(W12, W03, W09, W15, W06);
        SM3_R2(D, A, B, C, H, E, F, G, 0xF3B14F50, W13, W13 ^ W01);
        W13 = SM3_EXPAND(W13, W04, W10, W00, W07);
        SM3_R2(C, D, A, B, G, H, E, F, 0xE7629EA1, W14, W14 ^ W02);
        W14 = SM3_EXPAND(W14, W05, W11, W01, W08);
        SM3_R2(B, C, D, A, F, G, H, E, 0xCEC53D43, W15, W15 ^ W03);
        W15 = SM3_EXPAND(W15, W06, W12, W02, W09);
        SM3_R2(A, B, C, D, E, F, G, H, 0x9D8A7A87, W00, W00 ^ W04);
        W00 = SM3_EXPAND(W00, W07, W13, W03, W10);
        SM3_R2(D, A, B, C, H, E, F, G, 0x3B14F50F, W01, W01 ^ W05);
        W01 = SM3_EXPAND(W01, W08, W14, W04, W11);
        SM3_R2(C, D, A, B, G, H, E, F, 0x7629EA1E, W02, W02 ^ W06);
        W02 = SM3_EXPAND(W02, W09, W15, W05, W12);
        SM3_R2(B, C, D, A, F, G, H, E, 0xEC53D43C, W03, W03 ^ W07);
        W03 = SM3_EXPAND(W03, W10, W00, W06, W13);
        SM3_R2(A, B, C, D, E, F, G, H, 0xD8A7A879, W04, W04 ^ W08);
        SM3_R2(D, A, B, C, H, E, F, G, 0xB14F50F3, W05, W05 ^ W09);
        SM3_R2(C, D, A, B, G, H, E, F, 0x629EA1E7, W06, W06 ^ W10);
        SM3_R2(B, C, D, A, F, G, H, E, 0xC53D43CE, W07, W07 ^ W11);
        SM3_R2(A, B, C, D, E, F, G, H, 0x8A7A879D, W08, W08 ^ W12);
        SM3_R2(D, A, B, C, H, E, F, G, 0x14F50F3B, W09, W09 ^ W13);
        SM3_R2(C, D, A, B, G, H, E, F, 0x29EA1E76, W10, W10 ^ W14);
        SM3_R2(B, C, D, A, F, G, H, E, 0x53D43CEC, W11, W11 ^ W15);
        SM3_R2(A, B, C, D, E, F, G, H, 0xA7A879D8, W12, W12 ^ W00);
        SM3_R2(D, A, B, C, H, E, F, G, 0x4F50F3B1, W13, W13 ^ W01);
        SM3_R2(C, D, A, B, G, H, E, F, 0x9EA1E762, W14, W14 ^ W02);
        SM3_R2(B, C, D, A, F, G, H, E, 0x3D43CEC5, W15, W15 ^ W03);

    /* update state */
    state[0] ^= A; state[1] ^= B; state[2] ^= C; state[3] ^= D;
    state[4] ^= E; state[5] ^= F; state[6] ^= G; state[7] ^= H;
}

int
tecc_sm3_update(TeccSM3Context *ctx, const unsigned char *data, size_t len)
{
    size_t used;
    size_t free_space;

    if (ctx == NULL || data == NULL)
        return 0;
    if (len == 0)
        return 1;

    used = ctx->buflen;
    free_space = 64 - used;

    ctx->count += (unsigned long long)len * 8;

    if (len >= free_space)
    {
        /* fill current buffer and compress */
        memcpy(ctx->buffer + used, data, free_space);
        sm3_compress(ctx->state, ctx->buffer);

        /* process full blocks */
        {
            size_t remaining = len - free_space;
            size_t full_blocks = remaining / 64;
            size_t i;

            for (i = 0; i < full_blocks; i++)
                sm3_compress(ctx->state, data + free_space + i * 64);

            /* save remainder */
            ctx->buflen = remaining % 64;
            if (ctx->buflen > 0)
                memcpy(ctx->buffer,
                       data + free_space + full_blocks * 64,
                       ctx->buflen);
            else
                memset(ctx->buffer, 0, 64);
        }
    }
    else
    {
        memcpy(ctx->buffer + used, data, len);
        ctx->buflen += len;
    }

    return 1;
}

int
tecc_sm3_final(TeccSM3Context *ctx, unsigned char digest[32])
{
    unsigned long long total_bits;
    int i;

    if (ctx == NULL || digest == NULL)
        return 0;

    total_bits = ctx->count;

    /* append 0x80 */
    ctx->buffer[ctx->buflen++] = 0x80;

    if (ctx->buflen > 56)
    {
        /* pad current block with zeros and compress */
        memset(ctx->buffer + ctx->buflen, 0, 64 - ctx->buflen);
        sm3_compress(ctx->state, ctx->buffer);
        ctx->buflen = 0;
    }

    /* pad with zeros up to position 56 */
    memset(ctx->buffer + ctx->buflen, 0, 56 - ctx->buflen);

    /* append 64-bit length in big-endian */
    {
        unsigned char len_buf[8];
        int k;
        for (k = 0; k < 8; k++)
            len_buf[7 - k] = (unsigned char)(total_bits >> (k * 8));
        memcpy(ctx->buffer + 56, len_buf, 8);
    }

    /* final compression */
    sm3_compress(ctx->state, ctx->buffer);

    /* output digest */
    for (i = 0; i < 8; i++)
        put32_be(digest + i * 4, ctx->state[i]);

    return 1;
}

int
tecc_sm3_hash(const unsigned char *data, size_t len,
              unsigned char digest[32])
{
    TeccSM3Context ctx;

    if (data == NULL && len > 0)
        return 0;
    if (digest == NULL)
        return 0;

    tecc_sm3_init(&ctx);
    tecc_sm3_update(&ctx, data, len);
    tecc_sm3_final(&ctx, digest);

    return 1;
}

/* ---- HMAC-SM3 (RFC 2104) ---- */

struct TeccSM3HMACContext {
    TeccSM3Context inner_ctx;
    unsigned char ipad[64];
    unsigned char opad[64];
    int initialized;
};

TeccSM3HMACContext *
tecc_sm3_hmac_new(void)
{
    TeccSM3HMACContext *ctx;

    ctx = (TeccSM3HMACContext *)malloc(sizeof(TeccSM3HMACContext));
    if (ctx == NULL)
        return NULL;
    memset(ctx, 0, sizeof(TeccSM3HMACContext));
    ctx->initialized = 0;
    return ctx;
}

void
tecc_sm3_hmac_free(TeccSM3HMACContext *ctx)
{
    if (ctx != NULL)
    {
        memset(ctx, 0, sizeof(TeccSM3HMACContext));
        free(ctx);
    }
}

int
tecc_sm3_hmac_init(TeccSM3HMACContext *ctx,
                   const unsigned char *key, size_t key_len)
{
    unsigned char key_buf[64];
    unsigned char hash_key[32];
    int i;

    if (ctx == NULL || key == NULL || key_len == 0)
        return 0;

    memset(key_buf, 0, sizeof(key_buf));

    if (key_len > 64)
    {
        /* hash key if longer than block size */
        tecc_sm3_hash(key, key_len, hash_key);
        memcpy(key_buf, hash_key, 32);
    }
    else
    {
        memcpy(key_buf, key, key_len);
    }

    for (i = 0; i < 64; i++)
    {
        ctx->ipad[i] = key_buf[i] ^ 0x36;
        ctx->opad[i] = key_buf[i] ^ 0x5c;
    }

    /* inner SM3(ipad || ...) */
    tecc_sm3_init(&ctx->inner_ctx);
    tecc_sm3_update(&ctx->inner_ctx, ctx->ipad, 64);
    ctx->initialized = 1;

    /* clear key material */
    memset(key_buf, 0, sizeof(key_buf));
    memset(hash_key, 0, sizeof(hash_key));

    return 1;
}

int
tecc_sm3_hmac_update(TeccSM3HMACContext *ctx,
                     const unsigned char *data, size_t len)
{
    if (ctx == NULL || !ctx->initialized || data == NULL)
        return 0;
    if (len == 0)
        return 1;
    return tecc_sm3_update(&ctx->inner_ctx, data, len);
}

int
tecc_sm3_hmac_final(TeccSM3HMACContext *ctx,
                    unsigned char mac[32])
{
    TeccSM3Context outer_ctx;
    unsigned char inner_hash[32];

    if (ctx == NULL || !ctx->initialized || mac == NULL)
        return 0;

    /* finish inner hash */
    tecc_sm3_final(&ctx->inner_ctx, inner_hash);

    /* outer SM3(opad || inner_hash) */
    tecc_sm3_init(&outer_ctx);
    tecc_sm3_update(&outer_ctx, ctx->opad, 64);
    tecc_sm3_update(&outer_ctx, inner_hash, 32);
    tecc_sm3_final(&outer_ctx, mac);

    /* clear sensitive data */
    memset(inner_hash, 0, sizeof(inner_hash));
    memset(&outer_ctx, 0, sizeof(outer_ctx));
    ctx->initialized = 0;

    return 1;
}

int
tecc_sm3_hmac(const unsigned char *key, size_t key_len,
              const unsigned char *data, size_t data_len,
              unsigned char mac[32])
{
    TeccSM3HMACContext ctx;
    int ret;

    if (key == NULL || key_len == 0 || data == NULL || mac == NULL)
        return 0;

    /* use stack-allocated context for one-shot */
    memset(&ctx, 0, sizeof(ctx));
    ret = tecc_sm3_hmac_init(&ctx, key, key_len);
    if (!ret)
        return 0;
    ret = tecc_sm3_hmac_update(&ctx, data, data_len);
    if (!ret)
    {
        memset(&ctx, 0, sizeof(ctx));
        return 0;
    }
    ret = tecc_sm3_hmac_final(&ctx, mac);
    memset(&ctx, 0, sizeof(ctx));
    return ret;
}

/* Begin - ReqID:TECC4.0 */
/*
 * SM3 HMAC Snapshot — eliminates repeated hmac_init for batch processing.
 *
 * After hmac_init, the inner SM3 context holds the state after compressing
 * the ipad block.  The outer SM3 context (after compressing the opad block)
 * is also pre-computed.  Both are saved.  Each per-item computation then
 * restores the inner context, feeds the message data, finalises the inner
 * hash, restores the outer context, feeds the inner hash, and finalises.
 * This saves two sm3_init + two block compressions per item.
 */
struct TeccSM3HMACSnapshot {
    TeccSM3Context inner_ctx;
    TeccSM3Context outer_ctx;
};

TeccSM3HMACSnapshot *
tecc_sm3_hmac_snapshot_create(const unsigned char *key, size_t key_len)
{
    TeccSM3HMACSnapshot *snap;
    TeccSM3HMACContext ctx;
    TeccSM3Context outer;
    int ret;

    if (key == NULL || key_len == 0)
        return NULL;

    snap = (TeccSM3HMACSnapshot *)malloc(sizeof(TeccSM3HMACSnapshot));
    if (!snap)
        return NULL;

    memset(&ctx, 0, sizeof(ctx));
    ret = tecc_sm3_hmac_init(&ctx, key, key_len);
    if (!ret)
    {
        free(snap);
        return NULL;
    }

    memcpy(&snap->inner_ctx, &ctx.inner_ctx, sizeof(TeccSM3Context));

    tecc_sm3_init(&outer);
    tecc_sm3_update(&outer, ctx.opad, 64);
    memcpy(&snap->outer_ctx, &outer, sizeof(TeccSM3Context));

    memset(&ctx, 0, sizeof(ctx));
    memset(&outer, 0, sizeof(outer));

    return snap;
}

int
tecc_sm3_hmac_snapshot_compute(TeccSM3HMACSnapshot *snap,
                                const unsigned char *data, size_t data_len,
                                unsigned char mac[32])
{
    TeccSM3Context inner_ctx;
    TeccSM3Context outer_ctx;
    unsigned char inner_hash[32];

    if (snap == NULL || data == NULL || data_len == 0 || mac == NULL)
        return 0;

    memcpy(&inner_ctx, &snap->inner_ctx, sizeof(TeccSM3Context));
    if (!tecc_sm3_update(&inner_ctx, data, data_len))
    {
        memset(&inner_ctx, 0, sizeof(inner_ctx));
        return 0;
    }
    if (!tecc_sm3_final(&inner_ctx, inner_hash))
    {
        memset(&inner_ctx, 0, sizeof(inner_ctx));
        return 0;
    }
    memset(&inner_ctx, 0, sizeof(inner_ctx));

    memcpy(&outer_ctx, &snap->outer_ctx, sizeof(TeccSM3Context));
    if (!tecc_sm3_update(&outer_ctx, inner_hash, 32))
    {
        memset(&outer_ctx, 0, sizeof(outer_ctx));
        memset(inner_hash, 0, sizeof(inner_hash));
        return 0;
    }
    if (!tecc_sm3_final(&outer_ctx, mac))
    {
        memset(&outer_ctx, 0, sizeof(outer_ctx));
        memset(inner_hash, 0, sizeof(inner_hash));
        return 0;
    }

    memset(&outer_ctx, 0, sizeof(outer_ctx));
    memset(inner_hash, 0, sizeof(inner_hash));
    return 1;
}

void
tecc_sm3_hmac_snapshot_free(TeccSM3HMACSnapshot *snap)
{
    if (snap)
    {
        memset(snap, 0, sizeof(TeccSM3HMACSnapshot));
        free(snap);
    }
}
/* End - ReqID:TECC4.0 */
