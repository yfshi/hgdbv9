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

/* ---- Boolean / permutation functions (GM/T 0004-2012, Section 4) ---- */

static unsigned int
sm3_ff0(unsigned int x, unsigned int y, unsigned int z)
{
    return x ^ y ^ z;
}

static unsigned int
sm3_ff1(unsigned int x, unsigned int y, unsigned int z)
{
    return (x & y) | (x & z) | (y & z);
}

static unsigned int
sm3_gg0(unsigned int x, unsigned int y, unsigned int z)
{
    return x ^ y ^ z;
}

static unsigned int
sm3_gg1(unsigned int x, unsigned int y, unsigned int z)
{
    return (x & y) | (~x & z);
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

/*
 * Process one 512-bit block through the compression function
 */
static void
sm3_compress(unsigned int state[8], const unsigned char block[64])
{
    unsigned int w[68];
    unsigned int w1[64];
    unsigned int a, b, c, d, e, f, g, h;
    unsigned int ss1, ss2, tt1, tt2;
    int j;

    /* expand message block to W[0..67] */
    for (j = 0; j < 16; j++)
        w[j] = get32_be(block + j * 4);

    for (j = 16; j < 68; j++)
        w[j] = sm3_p1(w[j - 16] ^ w[j - 9] ^ rotl32(w[j - 3], 15))
               ^ rotl32(w[j - 13], 7) ^ w[j - 6];

    /* W'[0..63] */
    for (j = 0; j < 64; j++)
        w1[j] = w[j] ^ w[j + 4];

    /* init working variables */
    a = state[0]; b = state[1]; c = state[2]; d = state[3];
    e = state[4]; f = state[5]; g = state[6]; h = state[7];

    /* 64 rounds */
    for (j = 0; j < 64; j++)
    {
        if (j < 16)
        {
            ss1 = rotl32(rotl32(a, 12) + e + rotl32(SM3_T0, j), 7);
            ss2 = ss1 ^ rotl32(a, 12);
            tt1 = sm3_ff0(a, b, c) + d + ss2 + w1[j];
            tt2 = sm3_gg0(e, f, g) + h + ss1 + w[j];
        }
        else
        {
            ss1 = rotl32(rotl32(a, 12) + e + rotl32(SM3_T1, j), 7);
            ss2 = ss1 ^ rotl32(a, 12);
            tt1 = sm3_ff1(a, b, c) + d + ss2 + w1[j];
            tt2 = sm3_gg1(e, f, g) + h + ss1 + w[j];
        }

        d = c;
        c = rotl32(b, 9);
        b = a;
        a = tt1;
        h = g;
        g = rotl32(f, 19);
        f = e;
        e = sm3_p0(tt2);
    }

    /* update state */
    state[0] ^= a; state[1] ^= b; state[2] ^= c; state[3] ^= d;
    state[4] ^= e; state[5] ^= f; state[6] ^= g; state[7] ^= h;
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
