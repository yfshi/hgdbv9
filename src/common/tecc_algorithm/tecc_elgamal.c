/*-------------------------------------------------------------------------
 *
 * tecc_elgamal.c
 *	  Finite field ElGamal multiplicative homomorphic encryption
 *
 * Implements standard ElGamal over 2048-bit prime field with
 * multiplicative homomorphism.  No DLP solving needed for decryption.
 *
 * Portions Copyright (c) 2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 2021, HighGo Software Co.,Ltd.
 *
 * src/common/tecc_algorithm/tecc_elgamal.c
 *
 *-------------------------------------------------------------------------
 */

#ifndef TECC_STANDALONE_TEST
#include "postgres.h"	/* for TECC integration; also compiles standalone */
#else
/* Standalone test mode: use libc only */
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#endif

#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include "common/tecc_algorithm/tecc_elgamal.h"
/* ReqID:TECC4.0 - 复用 tecc_ec_elgamal.c 的算法内部并行 check/setter */
#include "common/tecc_algorithm/tecc_ec_elgamal.h"

/* ReqID:TECC4.0 - 素数判定的 OpenSSL 版本兼容：
 * BN_is_prime_ex 在 OpenSSL 3.0 起标记 deprecated，推荐 BN_check_prime（1.1.1+）。
 * 为兼容 OpenSSL 1.1.x，按版本选择。nchecks 参数在 3.0 无意义（固定 64 轮）。 */
#if OPENSSL_VERSION_NUMBER >= 0x30000000L
#define TECC_BN_CHECK_PRIME(v, ctx)	BN_check_prime((v), (ctx), NULL)
#else
#define TECC_BN_CHECK_PRIME(v, ctx)	(BN_is_prime_ex((v), 64, (ctx), NULL) == 1)
#endif

/* ==================== Default Parameters ==================== */

/*
 * RFC 5114 section 2.3 — 2048-bit MODP Group with 256-bit Prime Order Subgroup.
 *
 * Verified properties (Miller-Rabin + subgroup check):
 *   p = (true RFC 5114 §2.3 prime) — a 2048-bit prime
 *   q = (p - 1) / h  — a 256-bit prime; h = (p-1)/q is the cofactor
 *   g = (true RFC 5114 §2.3 generator) — generates exactly the order-q subgroup:
 *       g != 1, g^q mod p == 1, and q is prime → ord_p(g) = q.
 *
 * NOTE: Do NOT replace these with a hand-transcribed value without
 * verifying primality of p/q and that g generates the order-q subgroup.
 * The previous value was a composite (non-prime) 2048-bit number, which
 * broke the discrete-log assumption of the finite-field ElGamal scheme.
 */
static const char *RFC5114_2048_P =
	"87A8E61DB4B6663CFFBBD19C651959998CEEF608660DD0F2"
	"5D2CEED4435E3B00E00DF8F1D61957D4FAF7DF4561B2AA30"
	"16C3D91134096FAA3BF4296D830E9A7C209E0C6497517ABD"
	"5A8A9D306BCF67ED91F9E6725B4758C022E0B1EF4275BF7B"
	"6C5BFC11D45F9088B941F54EB1E59BB8BC39A0BF12307F5C"
	"4FDB70C581B23F76B63ACAE1CAA6B7902D52526735488A0E"
	"F13C6D9A51BFA4AB3AD8347796524D8EF6A167B5A41825D9"
	"67E144E5140564251CCACB83E6B486F6B3CA3F7971506026"
	"C0B857F689962856DED4010ABD0BE621C3A3960A54E710C3"
	"75F26375D7014103A4B54330C198AF126116D2276E11715F"
	"693877FAD7EF09CADB094AE91E1A1597";

/* RFC 5114 §2.3 generator: generates the order-q (256-bit) subgroup. */
static const char *RFC5114_2048_G =
	"3FB32C9B73134D0B2E77506660EDBD484CA7B18F21EF2054"
	"07F4793A1A0BA12510DBC15077BE463FFF4FED4AAC0BB555"
	"BE3A6C1B0C6B47B1BC3773BF7E8C6F62901228F8C28CBB18"
	"A55AE31341000A650196F931C77A57F2DDF463E5E9EC144B"
	"777DE62AAAB8A8628AC376D282D6ED3864E67982428EBC83"
	"1D14348F6F2F9193B5045AF2767164E1DFC967C1FB3F2E55"
	"A4BD1BFFE83B9C80D052B985D182EA0ADB2A3B7313D3FE14"
	"C8484B1E052588B9B7D2BBD2DF016199ECD06E1557CD0915"
	"B3353BBB64E0EC377FD028370DF92B52C7891428CDC67EB6"
	"184B523D1DB246C32F63078490F00EF8D647D148D4795451"
	"5E2327CFEF98C582664B4C0F6CC41659";

/* RFC 5114 §2.3 subgroup order q (256-bit prime). */
static const char *RFC5114_2048_Q =
	"8CF83642A709A097B447997640129DA299B1A47D1EB3750B"
	"A308B0FE64F5FBD3";

/*
 * RFC 5114 1024-bit MODP Group (Section 2.1, safe prime).
 * Generator g = 2, order-q subgroup.
 */
static const char *RFC5114_1024_P =
	"B10B8F96A080E01DDE92DE5EAE5D54EC52C99FBCFB06A3C6"
	"9A6A9DCA52D23B616073E28675A23D189838EF1E2EE652C0"
	"13ECB4AEA906112324975C3CD49B83BFACCBDD7D90C4BD70"
	"98488E9C219A73724EFFD6FAE5644738FAA31A4FF55BCCC0"
	"A151AF5F0DC8B4BD45BF37DF365C1A65E68CFDA76D4DA708"
	"DF1FB2BC2E4A4371";

static const char *RFC5114_1024_G = "02";

/*
 * RFC 2409 Oakley Group 2 (1024-bit safe prime).
 * Well-known prime from IKE, generator g = 2.
 */
static const char *RFC2409_1024_P =
	"FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD1"
	"29024E088A67CC74020BBEA63B139B22514A08798E3404DD"
	"EF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245"
	"E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7ED"
	"EE386BFB5A899FA5AE9F24117C4B1FE649286651ECE65381"
	"FFFFFFFFFFFFFFFF";

static const char *RFC2409_1024_G = "02";

/* ==================== Default Parameters Management ==================== */

/*
 * tecc_elgamal_params_valid - Validate a finite-field ElGamal parameter set.
 *
 * ReqID:TECC4.0 - Guards against a mis-transcribed group (a composite modulus
 * would break the discrete-log assumption).  Checks:
 *   - p is a prime
 *   - q is a prime and q | (p - 1)  (q is the prime-order subgroup order)
 *   - g != 1 and g^q mod p == 1  (g generates the order-q subgroup)
 */
static int
tecc_elgamal_params_valid(const BIGNUM *p, const BIGNUM *q, const BIGNUM *g,
						  BN_CTX *ctx)
{
	BIGNUM *pm1;
	BIGNUM *tmp;
	int		ret = TECC_ELGAMAL_ERR_CRYPTO;

	if (p == NULL || q == NULL || g == NULL)
		return TECC_ELGAMAL_ERR_NULL;

	pm1 = BN_new();
	tmp = BN_new();
	if (pm1 == NULL || tmp == NULL)
	{
		BN_free(pm1);
		BN_free(tmp);
		return TECC_ELGAMAL_ERR_MEMORY;
	}

	/* p must be prime */
	if (TECC_BN_CHECK_PRIME(p, ctx) != 1)
		goto out;

	/* q must be prime */
	if (TECC_BN_CHECK_PRIME(q, ctx) != 1)
		goto out;

	/* q must divide p - 1 */
	if (BN_sub(pm1, p, BN_value_one()) != 1)
		goto out;
	if (BN_mod(tmp, pm1, q, ctx) != 1 || !BN_is_zero(tmp))
		goto out;

	/* g must be != 1 and satisfy g^q ≡ 1 (mod p) */
	if (BN_is_one(g))
		goto out;
	if (BN_mod_exp(tmp, g, q, p, ctx) != 1 || !BN_is_one(tmp))
		goto out;

	ret = TECC_ELGAMAL_SUCCESS;

out:
	BN_free(pm1);
	BN_free(tmp);
	return ret;
}

int
tecc_elgamal_get_default_params(BIGNUM **p, BIGNUM **q, BIGNUM **g)
{
	BIGNUM *bn_p;
	BIGNUM *bn_q;
	BIGNUM *bn_g;
	BN_CTX *ctx;
	int		ret;

	if (p == NULL || q == NULL || g == NULL)
		return TECC_ELGAMAL_ERR_NULL;

	bn_p = BN_new();
	bn_q = BN_new();
	bn_g = BN_new();
	ctx = BN_CTX_new();
	if (bn_p == NULL || bn_q == NULL || bn_g == NULL || ctx == NULL)
	{
		BN_free(bn_p);
		BN_free(bn_q);
		BN_free(bn_g);
		BN_CTX_free(ctx);
		return TECC_ELGAMAL_ERR_MEMORY;
	}

	/* Parse hex strings (p and its prime-order subgroup order q from RFC 5114) */
	if (BN_hex2bn(&bn_p, RFC5114_2048_P) == 0 ||
		BN_hex2bn(&bn_q, RFC5114_2048_Q) == 0 ||
		BN_hex2bn(&bn_g, RFC5114_2048_G) == 0)
	{
		BN_free(bn_p);
		BN_free(bn_q);
		BN_free(bn_g);
		BN_CTX_free(ctx);
		return TECC_ELGAMAL_ERR_CRYPTO;
	}

	/* ReqID:TECC4.0 - Reject a mis-transcribed parameter set (the original
	 * RFC5114_2048_P was composite).  Fail loudly rather than silently
	 * running ElGamal over a broken modulus. */
	ret = tecc_elgamal_params_valid(bn_p, bn_q, bn_g, ctx);
	if (ret != TECC_ELGAMAL_SUCCESS)
	{
		BN_free(bn_p);
		BN_free(bn_q);
		BN_free(bn_g);
		BN_CTX_free(ctx);
		return ret;
	}

	BN_CTX_free(ctx);

	*p = bn_p;
	*q = bn_q;
	*g = bn_g;
	return TECC_ELGAMAL_SUCCESS;
}

int
tecc_elgamal_generate_params(BIGNUM **p, BIGNUM **g)
{
	BIGNUM *bn_p;
	BIGNUM *bn_q;
	BIGNUM *bn_g;
	BN_CTX *ctx;
	int		ret;

	if (p == NULL || g == NULL)
		return TECC_ELGAMAL_ERR_NULL;

	bn_p = BN_new();
	bn_q = BN_new();
	bn_g = BN_new();
	ctx = BN_CTX_new();
	if (bn_p == NULL || bn_q == NULL || bn_g == NULL || ctx == NULL)
	{
		BN_free(bn_p);
		BN_free(bn_q);
		BN_free(bn_g);
		BN_CTX_free(ctx);
		return TECC_ELGAMAL_ERR_MEMORY;
	}

	if (BN_hex2bn(&bn_p, RFC5114_2048_P) == 0 ||
		BN_hex2bn(&bn_q, RFC5114_2048_Q) == 0 ||
		BN_hex2bn(&bn_g, RFC5114_2048_G) == 0)
	{
		BN_free(bn_p);
		BN_free(bn_q);
		BN_free(bn_g);
		BN_CTX_free(ctx);
		return TECC_ELGAMAL_ERR_CRYPTO;
	}

	/* ReqID:TECC4.0 - Reject a broken (mis-transcribed) parameter set. */
	ret = tecc_elgamal_params_valid(bn_p, bn_q, bn_g, ctx);
	if (ret != TECC_ELGAMAL_SUCCESS)
	{
		BN_free(bn_p);
		BN_free(bn_q);
		BN_free(bn_g);
		BN_CTX_free(ctx);
		return ret;
	}

	BN_CTX_free(ctx);

	*p = bn_p;
	*g = bn_g;
	return TECC_ELGAMAL_SUCCESS;
}

/* ==================== Key Generation ==================== */

int
tecc_elgamal_generate_keypair(const BIGNUM *p, const BIGNUM *q, const BIGNUM *g,
							  BIGNUM **priv_key, BIGNUM **pub_key)
{
	BIGNUM *x;
	BIGNUM *h;
	BIGNUM *q_range;
	BN_CTX *ctx;
	int		ret;

	if (p == NULL || q == NULL || g == NULL ||
		priv_key == NULL || pub_key == NULL)
		return TECC_ELGAMAL_ERR_NULL;

	ctx = BN_CTX_new();
	if (ctx == NULL)
		return TECC_ELGAMAL_ERR_MEMORY;

	x = BN_new();
	h = BN_new();
	q_range = BN_new();
	if (x == NULL || h == NULL || q_range == NULL)
	{
		BN_free(x);
		BN_free(h);
		BN_free(q_range);
		BN_CTX_free(ctx);
		return TECC_ELGAMAL_ERR_MEMORY;
	}

	/* ReqID:TECC4.0 - Draw private key uniformly from [1, q-1] (prime-order
	 * subgroup).  BN_rand_range(x, q_range) draws in [0, q_range); with
	 * q_range = q-1 this yields [0, q-2], then we force it into [1, q-1]. */
	BN_sub(q_range, q, BN_value_one());
	if (BN_rand_range(x, q_range) != 1)
	{
		BN_free(x);
		BN_free(h);
		BN_free(q_range);
		BN_CTX_free(ctx);
		return TECC_ELGAMAL_ERR_CRYPTO;
	}
	/* Ensure 1 <= x <= q-1 (x == 0 would yield a trivial public key h = 1;
	 * the range is [0, q-2] here post-BN_sub, so force out of 0). */
	if (BN_is_zero(x))
		BN_add(x, x, BN_value_one());

	/* Compute public key: h = g^x mod p */
	if (BN_mod_exp(h, g, x, p, ctx) != 1)
	{
		BN_free(x);
		BN_free(h);
		BN_free(q_range);
		BN_CTX_free(ctx);
		return TECC_ELGAMAL_ERR_CRYPTO;
	}

	*priv_key = x;
	*pub_key = h;

	ret = TECC_ELGAMAL_SUCCESS;

	BN_free(q_range);
	BN_CTX_free(ctx);
	return ret;
}

/* ==================== Core Encryption / Decryption ==================== */

/* ReqID:TECC4.0 - 算法内部并行开关复用 tecc_ec_elgamal.c 的公共实现
 * tecc_he_algo_parallel_check()（环境变量 TECC_HE_ALGO_PARALLEL + TLS 覆盖），
 * 用于 he-mul 的 2 次模幂并行。 */

/* Worker context for parallel ElGamal encrypt (two independent mod-exps) */
typedef struct ElGamalEncryptWorkerCtx
{
	const BIGNUM *base;		/* g or h */
	const BIGNUM *exp;		/* r */
	const BIGNUM *mod;		/* p */
	BIGNUM		 *out;		/* result: c1 or hr */
	int			  ret;
} ElGamalEncryptWorkerCtx;

#ifdef _WIN32
static DWORD WINAPI
elgamal_encrypt_worker_thread(LPVOID arg)
#else
static void *
elgamal_encrypt_worker_thread(void *arg)
#endif
{
	ElGamalEncryptWorkerCtx *w = (ElGamalEncryptWorkerCtx *) arg;
	BN_CTX *local_ctx;

	local_ctx = BN_CTX_new();
	if (local_ctx == NULL)
	{
		w->ret = TECC_ELGAMAL_ERR_MEMORY;
#ifdef _WIN32
		return (DWORD) w->ret;
#else
		return NULL;
#endif
	}

	w->ret = (BN_mod_exp(w->out, w->base, w->exp, w->mod, local_ctx) == 1)
		? TECC_ELGAMAL_SUCCESS : TECC_ELGAMAL_ERR_CRYPTO;

	BN_CTX_free(local_ctx);
#ifdef _WIN32
	return (DWORD) w->ret;
#else
	return NULL;
#endif
}

int
tecc_elgamal_encrypt(const BIGNUM *p, const BIGNUM *q,
					 const BIGNUM *g, const BIGNUM *h,
					 const BIGNUM *m, BIGNUM *c1, BIGNUM *c2,
					 BN_CTX *ctx)
{
	BIGNUM *r;
	BIGNUM *q_range;
	BIGNUM *hr;		/* h^r mod p */
	BIGNUM *m_padded;	/* normalized non-negative plaintext */
	int		local_ctx = (ctx == NULL);
	int		ret;

	if (p == NULL || q == NULL || g == NULL || h == NULL ||
		m == NULL || c1 == NULL || c2 == NULL)
		return TECC_ELGAMAL_ERR_NULL;

	if (local_ctx)
	{
		ctx = BN_CTX_new();
		if (ctx == NULL)
			return TECC_ELGAMAL_ERR_MEMORY;
	}

	r = BN_new();
	q_range = BN_new();
	hr = BN_new();
	m_padded = NULL;
	if (r == NULL || q_range == NULL || hr == NULL)
	{
		BN_free(r);
		BN_free(q_range);
		BN_free(hr);
		if (local_ctx) BN_CTX_free(ctx);
		return TECC_ELGAMAL_ERR_MEMORY;
	}

	/* Normalize negative plaintext: BN_mod_mul on a negative BIGNUM is
	 * undefined. Encode negative m as (p + m) so that c2 = (p+m)*hr mod p
	 * lands in [0,p); tecc_elgamal_decrypt() mirrors this by reducing
	 * results > p/2 back to the negative range. */
	if (BN_is_negative(m))
	{
		m_padded = BN_new();
		if (m_padded == NULL)
		{
			BN_free(r);
			BN_free(q_range);
			BN_free(hr);
			if (local_ctx) BN_CTX_free(ctx);
			return TECC_ELGAMAL_ERR_MEMORY;
		}
		/* m_padded = p + m, with |m| < p so result is in (0, p) */
		if (BN_add(m_padded, p, m) != 1)
		{
			BN_free(m_padded);
			BN_free(r);
			BN_free(q_range);
			BN_free(hr);
			if (local_ctx) BN_CTX_free(ctx);
			return TECC_ELGAMAL_ERR_MEMORY;
		}
		BN_set_negative(m_padded, 0);
	}

	/* ReqID:TECC4.0 - Generate ephemeral nonce r uniformly from [1, q-1]
	 * (prime-order subgroup), so c1 = g^r is never 1 (avoids leaking the
	 * plaintext in the ciphertext when r is a multiple of the subgroup
	 * order) and matches standard prime-order-subgroup ElGamal. */
	BN_sub(q_range, q, BN_value_one());
	if (BN_rand_range(r, q_range) != 1)
	{
		BN_free(r);
		BN_free(q_range);
		BN_free(hr);
		if (local_ctx) BN_CTX_free(ctx);
		return TECC_ELGAMAL_ERR_CRYPTO;
	}
	if (BN_is_zero(r))
		BN_add(r, r, BN_value_one());

	/* ReqID:TECC4.0 - 算法内部并行：2 次模幂 c1=g^r、hr=h^r 相互独立，
	 * 固定 2 线程并行（默认启用，TECC_HE_ALGO_PARALLEL 或 TLS 覆盖可关闭）。 */
	if (tecc_he_algo_parallel_check())
	{
		ElGamalEncryptWorkerCtx workers[2];
#ifdef _WIN32
		HANDLE threads[2];
#else
		pthread_t threads[2];
#endif
		int wi;

		workers[0].base = g; workers[0].exp = r; workers[0].mod = p; workers[0].out = c1;
		workers[1].base = h; workers[1].exp = r; workers[1].mod = p; workers[1].out = hr;
		workers[0].ret = TECC_ELGAMAL_ERR_CRYPTO;
		workers[1].ret = TECC_ELGAMAL_ERR_CRYPTO;

		for (wi = 0; wi < 2; wi++)
		{
#ifdef _WIN32
			threads[wi] = CreateThread(NULL, 0, elgamal_encrypt_worker_thread,
									   &workers[wi], 0, NULL);
			if (threads[wi] == NULL)
				workers[wi].ret = TECC_ELGAMAL_ERR_MEMORY;
#else
			if (pthread_create(&threads[wi], NULL,
							   elgamal_encrypt_worker_thread, &workers[wi]) != 0)
				workers[wi].ret = TECC_ELGAMAL_ERR_MEMORY;
#endif
		}

#ifdef _WIN32
		WaitForMultipleObjects(2, threads, TRUE, INFINITE);
		for (wi = 0; wi < 2; wi++)
		{
			if (threads[wi] != NULL)
				CloseHandle(threads[wi]);
		}
#else
		for (wi = 0; wi < 2; wi++)
		{
			if (threads[wi])
				pthread_join(threads[wi], NULL);
		}
#endif

		if (workers[0].ret != TECC_ELGAMAL_SUCCESS ||
			workers[1].ret != TECC_ELGAMAL_SUCCESS)
		{
			BN_free(r);
			BN_free(q_range);
			BN_free(hr);
			if (local_ctx) BN_CTX_free(ctx);
			return (workers[0].ret != TECC_ELGAMAL_SUCCESS) ?
				workers[0].ret : workers[1].ret;
		}
	}
	else
	{
		/* c1 = g^r mod p */
		if (BN_mod_exp(c1, g, r, p, ctx) != 1)
		{
			BN_free(r);
			BN_free(q_range);
			BN_free(hr);
			if (local_ctx) BN_CTX_free(ctx);
			return TECC_ELGAMAL_ERR_CRYPTO;
		}

		/* hr = h^r mod p */
		if (BN_mod_exp(hr, h, r, p, ctx) != 1)
		{
			BN_free(r);
			BN_free(q_range);
			BN_free(hr);
			if (local_ctx) BN_CTX_free(ctx);
			return TECC_ELGAMAL_ERR_CRYPTO;
		}
	}

	/* c2 = m * hr mod p (use normalized plaintext for negative m) */
	if (BN_mod_mul(c2, m_padded ? m_padded : m, hr, p, ctx) != 1)
	{
		BN_free(m_padded);
		BN_free(r);
		BN_free(q_range);
		BN_free(hr);
		if (local_ctx) BN_CTX_free(ctx);
		return TECC_ELGAMAL_ERR_CRYPTO;
	}

	ret = TECC_ELGAMAL_SUCCESS;

	BN_free(m_padded);
	BN_free(r);
	BN_free(q_range);
	BN_free(hr);
	if (local_ctx) BN_CTX_free(ctx);
	return ret;
}

int
tecc_elgamal_decrypt(const BIGNUM *p, const BIGNUM *x,
					 const BIGNUM *c1, const BIGNUM *c2,
					 BIGNUM *m, BN_CTX *ctx)
{
	BIGNUM *s;		/* s = c1^x mod p */
	BIGNUM *half;	/* p / 2, for negative-value reduction */
	int		local_ctx = (ctx == NULL);
	int		ret;

	if (p == NULL || x == NULL || c1 == NULL || c2 == NULL || m == NULL)
		return TECC_ELGAMAL_ERR_NULL;

	if (local_ctx)
	{
		ctx = BN_CTX_new();
		if (ctx == NULL)
			return TECC_ELGAMAL_ERR_MEMORY;
	}

	s = BN_new();
	half = BN_new();
	if (s == NULL || half == NULL)
	{
		BN_free(s);
		BN_free(half);
		if (local_ctx) BN_CTX_free(ctx);
		return TECC_ELGAMAL_ERR_MEMORY;
	}

	/* s = c1^x mod p (shared secret) */
	if (BN_mod_exp(s, c1, x, p, ctx) != 1)
	{
		BN_free(s);
		BN_free(half);
		if (local_ctx) BN_CTX_free(ctx);
		return TECC_ELGAMAL_ERR_CRYPTO;
	}

	/* m = c2 * s^(-1) mod p = c2 / s mod p */
	{
		BIGNUM *s_inv;

		s_inv = BN_new();
		if (s_inv == NULL)
		{
			BN_free(s);
			BN_free(half);
			if (local_ctx) BN_CTX_free(ctx);
			return TECC_ELGAMAL_ERR_MEMORY;
		}

		if (BN_mod_inverse(s_inv, s, p, ctx) == NULL)
		{
			BN_free(s);
			BN_free(s_inv);
			BN_free(half);
			if (local_ctx) BN_CTX_free(ctx);
			return TECC_ELGAMAL_ERR_CRYPTO;
		}

		if (BN_mod_mul(m, c2, s_inv, p, ctx) != 1)
		{
			BN_free(s);
			BN_free(s_inv);
			BN_free(half);
			if (local_ctx) BN_CTX_free(ctx);
			return TECC_ELGAMAL_ERR_CRYPTO;
		}
		BN_free(s_inv);
	}

	/* Recover negative plaintext: encryption encoded negative m as (p + m),
	 * so any decrypted value > p/2 represents a negative number. Mirror the
	 * EC-ElGamal convention (tecc_ec_adjust_negative). */
	BN_rshift1(half, p);		/* half = p / 2 */
	if (BN_cmp(m, half) > 0)
		BN_sub(m, m, p);		/* m -= p → back to negative */

	ret = TECC_ELGAMAL_SUCCESS;

	BN_free(s);
	BN_free(half);
	if (local_ctx) BN_CTX_free(ctx);
	return ret;
}

/* ==================== Homomorphic Operations ==================== */

int
tecc_elgamal_mul(const BIGNUM *p,
				 BIGNUM *res_c1, BIGNUM *res_c2,
				 const BIGNUM *a_c1, const BIGNUM *a_c2,
				 const BIGNUM *b_c1, const BIGNUM *b_c2,
				 BN_CTX *ctx)
{
	int local_ctx = (ctx == NULL);
	int ret;

	if (p == NULL || res_c1 == NULL || res_c2 == NULL ||
		a_c1 == NULL || a_c2 == NULL || b_c1 == NULL || b_c2 == NULL)
		return TECC_ELGAMAL_ERR_NULL;

	if (local_ctx)
	{
		ctx = BN_CTX_new();
		if (ctx == NULL)
			return TECC_ELGAMAL_ERR_MEMORY;
	}

	/* c1 = c1_a * c1_b mod p */
	ret = BN_mod_mul(res_c1, a_c1, b_c1, p, ctx);
	/* c2 = c2_a * c2_b mod p */
	if (ret == 1)
		ret = BN_mod_mul(res_c2, a_c2, b_c2, p, ctx);

	if (local_ctx) BN_CTX_free(ctx);

	if (ret != 1)
		return TECC_ELGAMAL_ERR_CRYPTO;
	return TECC_ELGAMAL_SUCCESS;
}

/*
 * ReqID:TECC4.0 - ElGamal homomorphic exponentiation: E(m)^n = E(m^n mod p)
 * (c1, c2) = (c1_a^n mod p, c2_a^n mod p)
 *
 * 用平方-乘（二进制快速幂）实现，输出与原来「重复相乘 n-1 次」逐位相同：
 * E(m) = (g^r, m*h^r) 时 (c1^n, c2^n) 恰好是随机数为 r*n 的合法密文，且两种
 * 算法都在 [0,p) 的规范剩余上结束，故密文格式与跨客户端解密行为不变。
 * 调用方负责校验 n 为正。
 */
int
tecc_elgamal_pow(const BIGNUM *p,
				 BIGNUM *res_c1, BIGNUM *res_c2,
				 const BIGNUM *a_c1, const BIGNUM *a_c2,
				 const BIGNUM *n, BN_CTX *ctx)
{
	BIGNUM	   *acc_c1;
	BIGNUM	   *acc_c2;
	BIGNUM	   *tmp_c1;
	BIGNUM	   *tmp_c2;
	int			local_ctx = (ctx == NULL);
	int			bit;
	int			ret;

	if (p == NULL || res_c1 == NULL || res_c2 == NULL ||
		a_c1 == NULL || a_c2 == NULL || n == NULL)
		return TECC_ELGAMAL_ERR_NULL;

	/* n ≤ 0 无定义（n = 0 需要对密文求逆元），由调用方保证为正 */
	if (BN_is_zero(n) || BN_is_negative(n))
		return TECC_ELGAMAL_ERR_INVALID;

	if (local_ctx)
	{
		ctx = BN_CTX_new();
		if (ctx == NULL)
			return TECC_ELGAMAL_ERR_MEMORY;
	}

	acc_c1 = BN_new();
	acc_c2 = BN_new();
	tmp_c1 = BN_new();
	tmp_c2 = BN_new();
	if (acc_c1 == NULL || acc_c2 == NULL || tmp_c1 == NULL || tmp_c2 == NULL)
	{
		BN_free(acc_c1); BN_free(acc_c2);
		BN_free(tmp_c1); BN_free(tmp_c2);
		if (local_ctx) BN_CTX_free(ctx);
		return TECC_ELGAMAL_ERR_MEMORY;
	}

	/* acc = E(m)：n = 1 时下面的循环体不执行，等价原实现直接复制密文 */
	BN_copy(acc_c1, a_c1);
	BN_copy(acc_c2, a_c2);

	/* 从次高位起逐位处理：每步先平方，本位为 1 再乘一次 E(m) */
	for (bit = BN_num_bits(n) - 2; bit >= 0; bit--)
	{
		/* acc = acc²（先落临时量，避免输出与两个输入互为别名） */
		if (tecc_elgamal_mul(p, tmp_c1, tmp_c2, acc_c1, acc_c2,
							 acc_c1, acc_c2, ctx) != 0)
		{
			ret = TECC_ELGAMAL_ERR_CRYPTO;
			goto done;
		}
		BN_copy(acc_c1, tmp_c1);
		BN_copy(acc_c2, tmp_c2);

		/* 本位为 1：acc = acc * E(m) */
		if (BN_is_bit_set(n, bit))
		{
			if (tecc_elgamal_mul(p, tmp_c1, tmp_c2, acc_c1, acc_c2,
								 a_c1, a_c2, ctx) != 0)
			{
				ret = TECC_ELGAMAL_ERR_CRYPTO;
				goto done;
			}
			BN_copy(acc_c1, tmp_c1);
			BN_copy(acc_c2, tmp_c2);
		}
	}

	BN_copy(res_c1, acc_c1);
	BN_copy(res_c2, acc_c2);
	ret = TECC_ELGAMAL_SUCCESS;

done:
	BN_free(acc_c1); BN_free(acc_c2);
	BN_free(tmp_c1); BN_free(tmp_c2);
	if (local_ctx) BN_CTX_free(ctx);
	return ret;
}

int
tecc_elgamal_mul_scalar(const BIGNUM *p, const BIGNUM *q,
						const BIGNUM *g, const BIGNUM *h,
						BIGNUM *res_c1, BIGNUM *res_c2,
						const BIGNUM *a_c1, const BIGNUM *a_c2,
						const BIGNUM *k, BN_CTX *ctx)
{
	BIGNUM *enc_k_c1;
	BIGNUM *enc_k_c2;
	int		local_ctx = (ctx == NULL);
	int		ret;

	if (p == NULL || q == NULL || g == NULL || h == NULL ||
		res_c1 == NULL || res_c2 == NULL ||
		a_c1 == NULL || a_c2 == NULL || k == NULL)
		return TECC_ELGAMAL_ERR_NULL;

	if (local_ctx)
	{
		ctx = BN_CTX_new();
		if (ctx == NULL)
			return TECC_ELGAMAL_ERR_MEMORY;
	}

	enc_k_c1 = BN_new();
	enc_k_c2 = BN_new();
	if (enc_k_c1 == NULL || enc_k_c2 == NULL)

	{
		BN_free(enc_k_c1);
		BN_free(enc_k_c2);
		if (local_ctx) BN_CTX_free(ctx);
		return TECC_ELGAMAL_ERR_MEMORY;
	}

	/* Encrypt k: E(k) = (g^r, k * h^r) */
	ret = tecc_elgamal_encrypt(p, q, g, h, k, enc_k_c1, enc_k_c2, ctx);

	/* Multiply: E(a) * E(k) = E(a*k) */
	if (ret == TECC_ELGAMAL_SUCCESS)
	{
		ret = tecc_elgamal_mul(p, res_c1, res_c2,
							  a_c1, a_c2,
							  enc_k_c1, enc_k_c2, ctx);
	}

	BN_free(enc_k_c1);
	BN_free(enc_k_c2);
	if (local_ctx) BN_CTX_free(ctx);
	return ret;
}

/* ==================== Serialization ==================== */

int
tecc_elgamal_serialize(uint8_t *buf, int buf_size,
					   const BIGNUM *c1, const BIGNUM *c2)
{
	int	padded_size;

	if (buf == NULL || c1 == NULL || c2 == NULL)
		return TECC_ELGAMAL_ERR_NULL;

	/* Each component is exactly 256 bytes in fixed-length encoding */
	padded_size = TECC_ELGAMAL_CIPHERTEXT_SIZE;
	if (buf_size < padded_size)
		return TECC_ELGAMAL_ERR_SIZE;

	/* Clear buffer */
	memset(buf, 0, (size_t) padded_size);

	/* Write c1 (right-aligned, big-endian) */
	BN_bn2bin(c1, buf + (256 - BN_num_bytes(c1)));

	/* Write c2 (right-aligned, big-endian) */
	BN_bn2bin(c2, buf + 256 + (256 - BN_num_bytes(c2)));

	return padded_size;
}

int
tecc_elgamal_deserialize(BIGNUM *c1, BIGNUM *c2,
						 const uint8_t *buf, int buf_size)
{
	if (c1 == NULL || c2 == NULL || buf == NULL)
		return TECC_ELGAMAL_ERR_NULL;

	if (buf_size < TECC_ELGAMAL_CIPHERTEXT_SIZE)
		return TECC_ELGAMAL_ERR_SIZE;

	/* Read c1 (256 bytes) */
	BN_bin2bn(buf, 256, c1);

	/* Read c2 (256 bytes) */
	BN_bin2bn(buf + 256, 256, c2);

	return TECC_ELGAMAL_SUCCESS;
}

/* ==================== CRT Mode Implementation ==================== */

/* ReqID:TECC4.0 */
int
tecc_elgamal_crt_params_init(TeccElGamalCRTParams *params)
{
	BIGNUM *p1, *p2, *g1, *g2, *N;
	BN_CTX *ctx;

	if (params == NULL)
		return TECC_ELGAMAL_ERR_NULL;

	p1 = BN_new();
	p2 = BN_new();
	g1 = BN_new();
	g2 = BN_new();
	N = BN_new();
	if (p1 == NULL || p2 == NULL || g1 == NULL || g2 == NULL || N == NULL)
	{
		BN_free(p1); BN_free(p2); BN_free(g1); BN_free(g2); BN_free(N);
		return TECC_ELGAMAL_ERR_MEMORY;
	}

	/* Load RFC 5114 1024-bit safe prime */
	if (BN_hex2bn(&p1, RFC5114_1024_P) == 0 ||
		BN_hex2bn(&g1, RFC5114_1024_G) == 0)
	{
		BN_free(p1); BN_free(p2); BN_free(g1); BN_free(g2); BN_free(N);
		return TECC_ELGAMAL_ERR_CRYPTO;
	}

	/* Load RFC 2409 (Oakley Group 2) 1024-bit safe prime */
	if (BN_hex2bn(&p2, RFC2409_1024_P) == 0 ||
		BN_hex2bn(&g2, RFC2409_1024_G) == 0)
	{
		BN_free(p1); BN_free(p2); BN_free(g1); BN_free(g2); BN_free(N);
		return TECC_ELGAMAL_ERR_CRYPTO;
	}

	/* Compute CRT modulus N = p1 * p2 */
	ctx = BN_CTX_new();
	if (ctx == NULL)
	{
		BN_free(p1); BN_free(p2); BN_free(g1); BN_free(g2); BN_free(N);
		return TECC_ELGAMAL_ERR_MEMORY;
	}
	BN_mul(N, p1, p2, ctx);
	BN_CTX_free(ctx);

	params->num_primes = TECC_ELGAMAL_CRT_NUM_PRIMES;
	params->primes[0] = p1;
	params->primes[1] = p2;
	params->generators[0] = g1;
	params->generators[1] = g2;
	params->modulus = N;

	return TECC_ELGAMAL_SUCCESS;
}

/* ReqID:TECC4.0 */
void
tecc_elgamal_crt_params_free(TeccElGamalCRTParams *params)
{
	int i;

	if (params == NULL)
		return;

	for (i = 0; i < TECC_ELGAMAL_CRT_NUM_PRIMES; i++)
	{
		BN_free(params->primes[i]);
		BN_free(params->generators[i]);
		params->primes[i] = NULL;
		params->generators[i] = NULL;
	}
	BN_free(params->modulus);
	params->modulus = NULL;
	params->num_primes = 0;
}

/* ReqID:TECC4.0 */
int
tecc_elgamal_crt_ciphertext_init(TeccElGamalCRTCiphertext *ct)
{
	int i;

	if (ct == NULL)
		return TECC_ELGAMAL_ERR_NULL;

	for (i = 0; i < TECC_ELGAMAL_CRT_NUM_PRIMES; i++)
	{
		ct->c1[i] = BN_new();
		ct->c2[i] = BN_new();
		if (ct->c1[i] == NULL || ct->c2[i] == NULL)
		{
			tecc_elgamal_crt_ciphertext_free(ct);
			return TECC_ELGAMAL_ERR_MEMORY;
		}
	}
	return TECC_ELGAMAL_SUCCESS;
}

/* ReqID:TECC4.0 */
void
tecc_elgamal_crt_ciphertext_free(TeccElGamalCRTCiphertext *ct)
{
	int i;

	if (ct == NULL)
		return;

	for (i = 0; i < TECC_ELGAMAL_CRT_NUM_PRIMES; i++)
	{
		BN_free(ct->c1[i]);
		BN_free(ct->c2[i]);
		ct->c1[i] = NULL;
		ct->c2[i] = NULL;
	}
}

/* ReqID:TECC4.0 */
int
tecc_elgamal_crt_generate_keypair(BIGNUM **x, BIGNUM **h1, BIGNUM **h2,
								   const TeccElGamalCRTParams *params)
{
	BIGNUM *priv_key;
	BIGNUM *pub1, *pub2;
	BIGNUM *pm1;
	BN_CTX *ctx;
	int		i;

	if (params == NULL || x == NULL || h1 == NULL || h2 == NULL)
		return TECC_ELGAMAL_ERR_NULL;

	ctx = BN_CTX_new();
	if (ctx == NULL)
		return TECC_ELGAMAL_ERR_MEMORY;

	priv_key = BN_new();
	pub1 = BN_new();
	pub2 = BN_new();
	pm1 = BN_new();
	if (priv_key == NULL || pub1 == NULL || pub2 == NULL || pm1 == NULL)
	{
		BN_free(priv_key); BN_free(pub1); BN_free(pub2); BN_free(pm1);
		BN_CTX_free(ctx);
		return TECC_ELGAMAL_ERR_MEMORY;
	}

	/* Use smaller subgroup order for key generation (p1-1 is fine as bound) */
	BN_sub(pm1, params->primes[0], BN_value_one());
	for (i = 1; i < params->num_primes; i++)
	{
		/* Use the smallest (p_i - 1) as key range for consistency */
		BIGNUM *other = BN_new();
		BN_sub(other, params->primes[i], BN_value_one());
		if (BN_cmp(other, pm1) < 0)
			BN_copy(pm1, other);
		BN_free(other);
	}

	/* Generate random private key: 1 < x < min(p_i - 1) */
	if (BN_rand_range(priv_key, pm1) != 1)
	{
		BN_free(priv_key); BN_free(pub1); BN_free(pub2); BN_free(pm1);
		BN_CTX_free(ctx);
		return TECC_ELGAMAL_ERR_CRYPTO;
	}
	if (BN_is_one(priv_key) || BN_is_zero(priv_key))
		BN_add(priv_key, priv_key, BN_value_one());

	/* Compute public key components: h_i = g_i^x mod p_i */
	if (BN_mod_exp(pub1, params->generators[0], priv_key, params->primes[0], ctx) != 1 ||
		BN_mod_exp(pub2, params->generators[1], priv_key, params->primes[1], ctx) != 1)
	{
		BN_free(priv_key); BN_free(pub1); BN_free(pub2); BN_free(pm1);
		BN_CTX_free(ctx);
		return TECC_ELGAMAL_ERR_CRYPTO;
	}

	*x = priv_key;
	*h1 = pub1;
	*h2 = pub2;

	BN_free(pm1);
	BN_CTX_free(ctx);
	return TECC_ELGAMAL_SUCCESS;
}

/* ReqID:TECC4.0 */
int
tecc_elgamal_crt_encrypt(TeccElGamalCRTCiphertext *ct,
						  const BIGNUM *m,
						  const TeccElGamalCRTParams *params,
						  const BIGNUM *h1, const BIGNUM *h2,
						  BN_CTX *ctx)
{
	BIGNUM *r;
	BIGNUM *m_i;
	BIGNUM *hr_i;
	BIGNUM *pm1;
	const BIGNUM *pub_keys[2];
	int		local_ctx = (ctx == NULL);
	int		i, ret;

	if (ct == NULL || m == NULL || params == NULL || h1 == NULL || h2 == NULL)
		return TECC_ELGAMAL_ERR_NULL;

	if (local_ctx)
	{
		ctx = BN_CTX_new();
		if (ctx == NULL)
			return TECC_ELGAMAL_ERR_MEMORY;
	}

	r = BN_new();
	m_i = BN_new();
	hr_i = BN_new();
	pm1 = BN_new();
	if (r == NULL || m_i == NULL || hr_i == NULL || pm1 == NULL)
	{
		BN_free(r); BN_free(m_i); BN_free(hr_i); BN_free(pm1);
		if (local_ctx) BN_CTX_free(ctx);
		return TECC_ELGAMAL_ERR_MEMORY;
	}

	pub_keys[0] = h1;
	pub_keys[1] = h2;

	/* Use p_i - 1 as range for random r */
	BN_sub(pm1, params->primes[0], BN_value_one());

	/* Generate random nonce (same r for both primes saves one BN_rand call) */
	if (BN_rand_range(r, pm1) != 1)
	{
		BN_free(r); BN_free(m_i); BN_free(hr_i); BN_free(pm1);
		if (local_ctx) BN_CTX_free(ctx);
		return TECC_ELGAMAL_ERR_CRYPTO;
	}

	ret = TECC_ELGAMAL_SUCCESS;
	for (i = 0; i < params->num_primes; i++)
	{
		/* m_i = m mod p_i */
		if (BN_mod(m_i, m, params->primes[i], ctx) != 1)
		{
			ret = TECC_ELGAMAL_ERR_CRYPTO;
			break;
		}

		/* c1_i = g_i^r mod p_i */
		if (BN_mod_exp(ct->c1[i], params->generators[i], r, params->primes[i], ctx) != 1)
		{
			ret = TECC_ELGAMAL_ERR_CRYPTO;
			break;
		}

		/* hr_i = h_i^r mod p_i */
		if (BN_mod_exp(hr_i, pub_keys[i], r, params->primes[i], ctx) != 1)
		{
			ret = TECC_ELGAMAL_ERR_CRYPTO;
			break;
		}

		/* c2_i = m_i * hr_i mod p_i */
		if (BN_mod_mul(ct->c2[i], m_i, hr_i, params->primes[i], ctx) != 1)
		{
			ret = TECC_ELGAMAL_ERR_CRYPTO;
			break;
		}
	}

	BN_free(r); BN_free(m_i); BN_free(hr_i); BN_free(pm1);
	if (local_ctx) BN_CTX_free(ctx);
	return ret;
}

/* ReqID:TECC4.0 */
int
tecc_elgamal_crt_decrypt(BIGNUM *m,
						  const TeccElGamalCRTCiphertext *ct,
						  const TeccElGamalCRTParams *params,
						  const BIGNUM *x,
						  BN_CTX *ctx)
{
	BIGNUM *residues[2];
	BIGNUM *s, *s_inv;
	int		local_ctx = (ctx == NULL);
	int		i, ret;

	if (m == NULL || ct == NULL || params == NULL || x == NULL)
		return TECC_ELGAMAL_ERR_NULL;

	if (local_ctx)
	{
		ctx = BN_CTX_new();
		if (ctx == NULL)
			return TECC_ELGAMAL_ERR_MEMORY;
	}

	s = BN_new();
	s_inv = BN_new();
	residues[0] = BN_new();
	residues[1] = BN_new();
	if (s == NULL || s_inv == NULL || residues[0] == NULL || residues[1] == NULL)
	{
		BN_free(s); BN_free(s_inv);
		BN_free(residues[0]); BN_free(residues[1]);
		if (local_ctx) BN_CTX_free(ctx);
		return TECC_ELGAMAL_ERR_MEMORY;
	}

	ret = TECC_ELGAMAL_SUCCESS;
	for (i = 0; i < params->num_primes; i++)
	{
		/* s = c1_i^x mod p_i */
		if (BN_mod_exp(s, ct->c1[i], x, params->primes[i], ctx) != 1)
		{
			ret = TECC_ELGAMAL_ERR_CRYPTO;
			break;
		}

		/* s_inv = s^(-1) mod p_i */
		if (BN_mod_inverse(s_inv, s, params->primes[i], ctx) == NULL)
		{
			ret = TECC_ELGAMAL_ERR_CRYPTO;
			break;
		}

		/* m_i = c2_i * s_inv mod p_i */
		if (BN_mod_mul(residues[i], ct->c2[i], s_inv, params->primes[i], ctx) != 1)
		{
			ret = TECC_ELGAMAL_ERR_CRYPTO;
			break;
		}
	}

	/* CRT recombination: m = m1 + p1 * ((m2 - m1) * p1^(-1) mod p2) */
	if (ret == TECC_ELGAMAL_SUCCESS)
	{
		BIGNUM *diff = BN_new();
		BIGNUM *p1_inv_mod_p2 = BN_new();
		BIGNUM *tmp = BN_new();

		if (diff == NULL || p1_inv_mod_p2 == NULL || tmp == NULL)
		{
			BN_free(diff); BN_free(p1_inv_mod_p2); BN_free(tmp);
			ret = TECC_ELGAMAL_ERR_MEMORY;
		}
		else
		{
			/* p1_inv = p1^(-1) mod p2 */
			if (BN_mod_inverse(p1_inv_mod_p2, params->primes[0], params->primes[1], ctx) == NULL)
			{
				ret = TECC_ELGAMAL_ERR_CRYPTO;
			}
			else
			{
				/* diff = (m2 - m1) mod p2 */
				BN_mod_sub(diff, residues[1], residues[0], params->primes[1], ctx);

				/* tmp = diff * p1_inv mod p2 */
				BN_mod_mul(tmp, diff, p1_inv_mod_p2, params->primes[1], ctx);

				/* tmp = tmp * p1 */
				BN_mul(tmp, tmp, params->primes[0], ctx);

				/* m = m1 + tmp */
				BN_add(m, residues[0], tmp);
			}
			BN_free(diff); BN_free(p1_inv_mod_p2); BN_free(tmp);
		}
	}

	BN_free(s); BN_free(s_inv);
	BN_free(residues[0]); BN_free(residues[1]);
	if (local_ctx) BN_CTX_free(ctx);
	return ret;
}

/* ReqID:TECC4.0 */
int
tecc_elgamal_crt_mul(TeccElGamalCRTCiphertext *res,
					  const TeccElGamalCRTCiphertext *a,
					  const TeccElGamalCRTCiphertext *b,
					  const TeccElGamalCRTParams *params,
					  BN_CTX *ctx)
{
	int local_ctx = (ctx == NULL);
	int i, ret;

	if (res == NULL || a == NULL || b == NULL || params == NULL)
		return TECC_ELGAMAL_ERR_NULL;

	if (local_ctx)
	{
		ctx = BN_CTX_new();
		if (ctx == NULL)
			return TECC_ELGAMAL_ERR_MEMORY;
	}

	for (i = 0; i < params->num_primes; i++)
	{
		/* c1_i = c1_a_i * c1_b_i mod p_i */
		ret = BN_mod_mul(res->c1[i], a->c1[i], b->c1[i], params->primes[i], ctx);
		if (ret != 1)
		{
			if (local_ctx) BN_CTX_free(ctx);
			return TECC_ELGAMAL_ERR_CRYPTO;
		}
		/* c2_i = c2_a_i * c2_b_i mod p_i */
		ret = BN_mod_mul(res->c2[i], a->c2[i], b->c2[i], params->primes[i], ctx);
		if (ret != 1)
		{
			if (local_ctx) BN_CTX_free(ctx);
			return TECC_ELGAMAL_ERR_CRYPTO;
		}
	}

	if (local_ctx) BN_CTX_free(ctx);
	return TECC_ELGAMAL_SUCCESS;
}

/* ReqID:TECC4.0 */
int
tecc_elgamal_crt_mul_scalar(TeccElGamalCRTCiphertext *res,
							 const TeccElGamalCRTCiphertext *a,
							 const BIGNUM *k,
							 const TeccElGamalCRTParams *params,
							 const BIGNUM *h1, const BIGNUM *h2,
							 BN_CTX *ctx)
{
	TeccElGamalCRTCiphertext enc_k;
	int local_ctx = (ctx == NULL);
	int ret;

	if (res == NULL || a == NULL || k == NULL || params == NULL ||
		h1 == NULL || h2 == NULL)
		return TECC_ELGAMAL_ERR_NULL;

	if (local_ctx)
	{
		ctx = BN_CTX_new();
		if (ctx == NULL)
			return TECC_ELGAMAL_ERR_MEMORY;
	}

	/* Init temporary ciphertext for E(k) */
	ret = tecc_elgamal_crt_ciphertext_init(&enc_k);
	if (ret != TECC_ELGAMAL_SUCCESS)
	{
		if (local_ctx) BN_CTX_free(ctx);
		return ret;
	}

	/* Encrypt k */
	ret = tecc_elgamal_crt_encrypt(&enc_k, k, params, h1, h2, ctx);
	if (ret == TECC_ELGAMAL_SUCCESS)
	{
		/* Multiply: E(a) * E(k) = E(a*k) */
		ret = tecc_elgamal_crt_mul(res, a, &enc_k, params, ctx);
	}

	tecc_elgamal_crt_ciphertext_free(&enc_k);
	if (local_ctx) BN_CTX_free(ctx);
	return ret;
}

/* ReqID:TECC4.0 */
int
tecc_elgamal_crt_serialize(uint8_t *buf, int buf_size,
							const TeccElGamalCRTCiphertext *ct)
{
	int i, comp_size;

	if (buf == NULL || ct == NULL)
		return TECC_ELGAMAL_ERR_NULL;

	comp_size = TECC_ELGAMAL_CRT_COMPONENT_SIZE;
	if (buf_size < TECC_ELGAMAL_CRT_CIPHERTEXT_SIZE)
		return TECC_ELGAMAL_ERR_SIZE;

	memset(buf, 0, (size_t) TECC_ELGAMAL_CRT_CIPHERTEXT_SIZE);

	for (i = 0; i < TECC_ELGAMAL_CRT_NUM_PRIMES; i++)
	{
		int c1_bytes, c2_bytes;
		/* c1_i: right-aligned 128 bytes */
		c1_bytes = BN_num_bytes(ct->c1[i]);
		BN_bn2bin(ct->c1[i], buf + (comp_size - c1_bytes));
		/* c2_i: right-aligned 128 bytes */
		c2_bytes = BN_num_bytes(ct->c2[i]);
		BN_bn2bin(ct->c2[i], buf + comp_size + (comp_size - c2_bytes));
		buf += comp_size * 2;  /* advance to next prime's slot */
	}

	return TECC_ELGAMAL_CRT_CIPHERTEXT_SIZE;
}

/* ReqID:TECC4.0 */
int
tecc_elgamal_crt_deserialize(TeccElGamalCRTCiphertext *ct,
							  const uint8_t *buf, int buf_size)
{
	int i, comp_size;

	if (ct == NULL || buf == NULL)
		return TECC_ELGAMAL_ERR_NULL;

	comp_size = TECC_ELGAMAL_CRT_COMPONENT_SIZE;
	if (buf_size < TECC_ELGAMAL_CRT_CIPHERTEXT_SIZE)
		return TECC_ELGAMAL_ERR_SIZE;

	for (i = 0; i < TECC_ELGAMAL_CRT_NUM_PRIMES; i++)
	{
		BN_bin2bn(buf, comp_size, ct->c1[i]);
		BN_bin2bn(buf + comp_size, comp_size, ct->c2[i]);
		buf += comp_size * 2;
	}

	return TECC_ELGAMAL_SUCCESS;
}
