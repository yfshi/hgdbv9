/*-------------------------------------------------------------------------
 *
 * tecc_ec_elgamal.c
 *	  EC-ElGamal additive homomorphic encryption implementation
 *
 * Implements EC-ElGamal on P-256 curve with CRT optimization for
 * multi-precision integer support (up to 84 bits, 4x21-bit CRT splits).
 *
 * References:
 *   - lubux/ecelgamal (ETH Zurich) for algorithm structure
 *   - crtecelgamal for CRT optimization pattern
 *
 * Portions Copyright (c) 2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 2021, HighGo Software Co.,Ltd.
 *
 * src/common/tecc_algorithm/tecc_ec_elgamal.c
 *
 *-------------------------------------------------------------------------
 */

#ifndef TECC_STANDALONE_TEST
#include "postgres.h"  /* included for TECC integration */
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

/* ReqID:TECC4.0 - 集成模式(含 postgres.h)下也需显式包含 <pthread.h>：本文件
 * 用到 pthread_create/join/once 与 PTHREAD_ONCE_INIT。原实现仅在 standalone
 * 分支 include，导致内网(无 _REENTRANT/线程宏)构建时 PTHREAD_ONCE_INIT 未声明。 */
#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

#include "common/tecc_algorithm/tecc_ec_elgamal.h"
#ifndef TECC_STANDALONE_TEST
#include "tecc_bsgs_tables_data.h"	/* ReqID:TECC4.0 - 静态 BSGS 表数据（参数指纹 + extern） */
#endif

/* ==================== BSGS Table: Entry Comparison ==================== */

/*
 * Compare two BSGS entries by their point_key bytes.
 * Used for qsort/bsearch on the table.
 */
static int
bsgs_entry_compare(const void *a, const void *b)
{
	const TeccECBSGSEntry *ea = (const TeccECBSGSEntry *) a;
	const TeccECBSGSEntry *eb = (const TeccECBSGSEntry *) b;

	return memcmp(ea->point_key, eb->point_key, TECC_EC_POINT_SIZE);
}

/*
 * Compare a raw point key against a BSGS entry.
 * Used for bsearch with a key-only argument.
 */
static int
bsgs_key_compare(const void *key, const void *entry)
{
	const uint8_t *k = (const uint8_t *) key;
	const TeccECBSGSEntry *e = (const TeccECBSGSEntry *) entry;

	return memcmp(k, e->point_key, TECC_EC_POINT_SIZE);
}

/* ==================== BSGS Table Management ==================== */

int
tecc_ec_bsgs_table_build_runtime(TeccECBSGSTable *table, const EC_GROUP *group,
								 uint32_t size)
{
	uint32_t	i;
	EC_POINT   *cur;
	BN_CTX	   *ctx;
	BIGNUM	   *bn_size;

	if (table == NULL || group == NULL)
		return TECC_EC_ELGAMAL_ERR_NULL;

	/* Allocate entries array */
	table->entries = (TeccECBSGSEntry *) calloc(size, sizeof(TeccECBSGSEntry));
	if (table->entries == NULL)
		return TECC_EC_ELGAMAL_ERR_MEMORY;
	table->capacity = size;
	table->size = 0;
	table->group = group;
	table->entries_owned = 1;	/* ReqID:TECC4.0 - 运行时构建，free 时释放 */

	ctx = BN_CTX_new();
	if (ctx == NULL)
	{
		free(table->entries);
		return TECC_EC_ELGAMAL_ERR_MEMORY;
	}

	/* Compute giant step: size * G */
	bn_size = BN_new();
	if (bn_size == NULL)
	{
		BN_CTX_free(ctx);
		free(table->entries);
		return TECC_EC_ELGAMAL_ERR_MEMORY;
	}
	BN_set_word(bn_size, (BN_ULONG) size);

	table->mG = EC_POINT_new(group);
	table->mG_inv = EC_POINT_new(group);
	if (table->mG == NULL || table->mG_inv == NULL)
	{
		BN_free(bn_size);
		BN_CTX_free(ctx);
		free(table->entries);
		return TECC_EC_ELGAMAL_ERR_MEMORY;
	}

	/* mG = size * G */
	EC_POINT_mul(group, table->mG, bn_size, NULL, NULL, ctx);

	/* mG_inv = -mG (invert) */
	EC_POINT_copy(table->mG_inv, table->mG);
	EC_POINT_invert(group, table->mG_inv, ctx);

		/* Baby steps: compute i*G for i = 0..size-1 */
		cur = EC_POINT_new(group);
		if (cur == NULL)
		{
			BN_free(bn_size);
			BN_CTX_free(ctx);
			free(table->entries);
			return TECC_EC_ELGAMAL_ERR_MEMORY;
		}
		EC_POINT_set_to_infinity(group, cur);

		/*
		 * Handle i=0 specially: the point at infinity cannot be serialized
		 * in compressed form (EC_POINT_point2oct returns 0 bytes).
		 * Use all-zero key bytes as convention for value 0.
		 */
		memset(table->entries[0].point_key, 0, TECC_EC_POINT_SIZE);
		table->entries[0].value = 0;
		table->size++;

		/* Next: cur = 0*G + G = G */
		EC_POINT_add(group, cur, cur, EC_GROUP_get0_generator(group), ctx);

		for (i = 1; i < size; i++)
		{
			size_t		oct_len;

			oct_len = EC_POINT_point2oct(group, cur,
										 POINT_CONVERSION_COMPRESSED,
										 table->entries[i].point_key,
										 TECC_EC_POINT_SIZE, ctx);
			if (oct_len != TECC_EC_POINT_SIZE)
			{
				EC_POINT_free(cur);
				BN_free(bn_size);
				BN_CTX_free(ctx);
				free(table->entries);
				return TECC_EC_ELGAMAL_ERR_CRYPTO;
			}
			table->entries[i].value = i;
			table->size++;

			/* Next: cur = cur + G */
			EC_POINT_add(group, cur, cur, EC_GROUP_get0_generator(group), ctx);
		}

		/* Sort entries for binary search */
		qsort(table->entries, table->size, sizeof(TeccECBSGSEntry),
			  bsgs_entry_compare);

		EC_POINT_free(cur);
		BN_free(bn_size);
		BN_CTX_free(ctx);
		return TECC_EC_ELGAMAL_SUCCESS;
}

/* ReqID:TECC4.0 - BSGS 表初始化（静态表数据优先）：
 * 优先复用与核心算法同源预生成的静态表数据（tecc_bsgs_entries），
 * 避免每个进程重新构建（HE 客户端冷启动 0.6s → ~10ms）。
 * 参数指纹（曲线 NID + capacity）与静态数据一致时直接复用；
 * 不一致（算法参数变化但表数据未重新生成）时回退运行时构建，
 * 保证正确性优先。 */
int
tecc_ec_bsgs_table_init(TeccECBSGSTable *table, const EC_GROUP *group,
						uint32_t size)
{
	BIGNUM   *bn_size;
	BN_CTX   *ctx;

	if (table == NULL || group == NULL)
		return TECC_EC_ELGAMAL_ERR_NULL;

#ifndef TECC_STANDALONE_TEST
	if (size == TECC_BSGS_DATA_CAPACITY &&
		EC_GROUP_get_curve_name(group) == TECC_BSGS_DATA_GROUP_NID)
	{
		/* 静态表数据匹配：复用 tecc_bsgs_entries，仅需构建 giant-step 点 */
		ctx = BN_CTX_new();
		bn_size = BN_new();
		if (ctx == NULL || bn_size == NULL)
		{
			BN_CTX_free(ctx);
			BN_free(bn_size);
			return TECC_EC_ELGAMAL_ERR_MEMORY;
		}
		BN_set_word(bn_size, (BN_ULONG) size);
		table->entries = (TeccECBSGSEntry *) tecc_bsgs_entries;
		table->size = size;
		table->capacity = size;
		table->group = group;
		table->entries_owned = 0;	/* 静态数据，不 free */
		table->mG = EC_POINT_new(group);
		table->mG_inv = EC_POINT_new(group);
		if (table->mG == NULL || table->mG_inv == NULL)
		{
			EC_POINT_free(table->mG);
			EC_POINT_free(table->mG_inv);
			BN_free(bn_size);
			BN_CTX_free(ctx);
			return TECC_EC_ELGAMAL_ERR_MEMORY;
		}
		EC_POINT_mul(group, table->mG, bn_size, NULL, NULL, ctx);
		EC_POINT_copy(table->mG_inv, table->mG);
		EC_POINT_invert(group, table->mG_inv, ctx);
		BN_free(bn_size);
		BN_CTX_free(ctx);
		return TECC_EC_ELGAMAL_SUCCESS;
	}
#endif

	/* 参数指纹不匹配（静态表数据与当前算法参数不一致）：
	 * 回退运行时构建，保证正确性优先。 */
	return tecc_ec_bsgs_table_build_runtime(table, group, size);
}

void
tecc_ec_bsgs_table_free(TeccECBSGSTable *table)
{
	if (table == NULL)
		return;
	if (table->entries != NULL)
	{
		/* ReqID:TECC4.0 - 仅释放运行时构建的 entries；静态表数据不 free */
		if (table->entries_owned)
			free(table->entries);
		table->entries = NULL;
	}
	if (table->mG != NULL)
	{
		EC_POINT_free(table->mG);
		table->mG = NULL;
	}
	if (table->mG_inv != NULL)
	{
		EC_POINT_free(table->mG_inv);
		table->mG_inv = NULL;
	}
	table->size = 0;
	table->capacity = 0;
}

/* ==================== BSGS Table Parallel Initialization ==================== */

/* ReqID:TECC4.0 - 并行建表 worker 上下文：每个 worker 独立建一个 BSGS 表。
 * tecc_ec_bsgs_table_init 内部使用局部 BN_CTX，线程安全；
 * EC_GROUP 在点加时只读，多线程共享安全。 */
typedef struct BSGSInitWorkerCtx
{
	TeccECBSGSTable   *table;
	const EC_GROUP	   *group;
	uint32_t			size;
	int					ret;
} BSGSInitWorkerCtx;

#ifdef _WIN32
static DWORD WINAPI
bsgs_init_worker_thread(LPVOID arg)
#else
static void *
bsgs_init_worker_thread(void *arg)
#endif
{
	BSGSInitWorkerCtx *w = (BSGSInitWorkerCtx *) arg;

	w->ret = tecc_ec_bsgs_table_init(w->table, w->group, w->size);
#ifdef _WIN32
	return (DWORD) w->ret;
#else
	return NULL;
#endif
}

/* ReqID:TECC4.0 - 并行初始化 count 个 BSGS 表（每个素数一个表）。
 * 各表完全独立，可多线程并行，显著缩短一次性建表时间（8 表约 8 倍加速），
 * 从而控制 HE 客户端冷启动开销。 */
int
tecc_ec_bsgs_tables_init_parallel(TeccECBSGSTable **tables,
								  const EC_GROUP *group,
								  uint32_t size,
								  int count)
{
	int					i;
	int					ret;
	BSGSInitWorkerCtx  *workers;
#ifdef _WIN32
	HANDLE			  *threads;
#else
	pthread_t		  *threads;
#endif

	if (tables == NULL || group == NULL || count <= 0)
		return TECC_EC_ELGAMAL_ERR_NULL;

	workers = (BSGSInitWorkerCtx *) calloc((size_t) count, sizeof(BSGSInitWorkerCtx));
	threads =
#ifdef _WIN32
		(HANDLE *) calloc((size_t) count, sizeof(HANDLE));
#else
		(pthread_t *) calloc((size_t) count, sizeof(pthread_t));
#endif
	if (workers == NULL || threads == NULL)
	{
		free(workers);
		free(threads);
		return TECC_EC_ELGAMAL_ERR_MEMORY;
	}

	for (i = 0; i < count; i++)
	{
		workers[i].table = tables[i];
		workers[i].group = group;
		workers[i].size = size;
		workers[i].ret = TECC_EC_ELGAMAL_ERR_CRYPTO;
	}

	/* Launch worker threads */
	for (i = 0; i < count; i++)
	{
#ifdef _WIN32
		threads[i] = CreateThread(NULL, 0, bsgs_init_worker_thread,
								  &workers[i], 0, NULL);
		if (threads[i] == NULL)
			workers[i].ret = TECC_EC_ELGAMAL_ERR_MEMORY;
#else
		if (pthread_create(&threads[i], NULL,
						   bsgs_init_worker_thread, &workers[i]) != 0)
			workers[i].ret = TECC_EC_ELGAMAL_ERR_MEMORY;
#endif
	}

	/* Wait for all workers */
#ifdef _WIN32
	WaitForMultipleObjects((DWORD) count, threads, TRUE, INFINITE);
	for (i = 0; i < count; i++)
	{
		if (threads[i] != NULL)
			CloseHandle(threads[i]);
	}
#else
	for (i = 0; i < count; i++)
	{
		if (threads[i])
			pthread_join(threads[i], NULL);
	}
#endif

	/* Check for errors in any worker */
	ret = TECC_EC_ELGAMAL_SUCCESS;
	for (i = 0; i < count; i++)
	{
		if (workers[i].ret != TECC_EC_ELGAMAL_SUCCESS)
		{
			ret = workers[i].ret;
			break;
		}
	}

	free(workers);
	free(threads);
	return ret;
}

int
tecc_ec_bsgs_solve(uint64_t *result, TeccECBSGSTable *table,
				   const EC_GROUP *group, const EC_POINT *M,
				   uint64_t max_iter, BN_CTX *ctx, int *is_negative)
{
	uint64_t	i;
	TeccECBSGSEntry *found;
	uint8_t		point_key_pos[TECC_EC_POINT_SIZE];
	uint8_t		point_key_neg[TECC_EC_POINT_SIZE];
	EC_POINT   *cur_pos;	/* 正方向：M - i*mG（找 M = +m*G 的 m） */
	EC_POINT   *cur_neg;	/* 负方向：-M - i*mG（找 -M = +m*G 的 m，即 M = -m*G） */
	int			local_ctx = (ctx == NULL);

	if (result == NULL || table == NULL || group == NULL || M == NULL)
		return TECC_EC_ELGAMAL_ERR_NULL;

	if (local_ctx)
	{
		ctx = BN_CTX_new();
		if (ctx == NULL)
			return TECC_EC_ELGAMAL_ERR_MEMORY;
	}

	cur_pos = EC_POINT_dup(M, group);
	cur_neg = EC_POINT_dup(M, group);
	if (cur_pos == NULL || cur_neg == NULL)
	{
		EC_POINT_free(cur_pos);
		EC_POINT_free(cur_neg);
		if (local_ctx) BN_CTX_free(ctx);
		return TECC_EC_ELGAMAL_ERR_MEMORY;
	}
	/* cur_neg = -M */
	EC_POINT_invert(group, cur_neg, ctx);

	if (is_negative != NULL)
		*is_negative = 0;

	/* ReqID:TECC4.0 - 双向 giant step：同时解 M 和 -M，正负值都只需
	 * |m|/capacity 步，避免负值场景（M = -m*G）单方向遍历满 max_iter 的低效。 */
	for (i = 0; i <= max_iter; i++)
	{
		size_t oct_len;

		/* 正方向：查 cur_pos */
		if (EC_POINT_is_at_infinity(group, cur_pos))
		{
			*result = (uint64_t) i * table->capacity;
			EC_POINT_free(cur_pos);
			EC_POINT_free(cur_neg);
			if (local_ctx) BN_CTX_free(ctx);
			return TECC_EC_ELGAMAL_SUCCESS;
		}
		oct_len = EC_POINT_point2oct(group, cur_pos, POINT_CONVERSION_COMPRESSED,
									 point_key_pos, TECC_EC_POINT_SIZE, ctx);
		if (oct_len != TECC_EC_POINT_SIZE)
		{
			EC_POINT_free(cur_pos);
			EC_POINT_free(cur_neg);
			if (local_ctx) BN_CTX_free(ctx);
			return TECC_EC_ELGAMAL_ERR_CRYPTO;
		}
		found = (TeccECBSGSEntry *) bsearch(point_key_pos, table->entries,
											table->size, sizeof(TeccECBSGSEntry),
											bsgs_key_compare);
		if (found != NULL)
		{
			*result = (uint64_t) i * table->capacity + found->value;
			EC_POINT_free(cur_pos);
			EC_POINT_free(cur_neg);
			if (local_ctx) BN_CTX_free(ctx);
			return TECC_EC_ELGAMAL_SUCCESS;
		}

		/* 负方向：查 cur_neg */
		if (EC_POINT_is_at_infinity(group, cur_neg))
		{
			*result = (uint64_t) i * table->capacity;
			if (is_negative != NULL)
				*is_negative = 1;
			EC_POINT_free(cur_pos);
			EC_POINT_free(cur_neg);
			if (local_ctx) BN_CTX_free(ctx);
			return TECC_EC_ELGAMAL_SUCCESS;
		}
		oct_len = EC_POINT_point2oct(group, cur_neg, POINT_CONVERSION_COMPRESSED,
									 point_key_neg, TECC_EC_POINT_SIZE, ctx);
		if (oct_len != TECC_EC_POINT_SIZE)
		{
			EC_POINT_free(cur_pos);
			EC_POINT_free(cur_neg);
			if (local_ctx) BN_CTX_free(ctx);
			return TECC_EC_ELGAMAL_ERR_CRYPTO;
		}
		found = (TeccECBSGSEntry *) bsearch(point_key_neg, table->entries,
											table->size, sizeof(TeccECBSGSEntry),
											bsgs_key_compare);
		if (found != NULL)
		{
			*result = (uint64_t) i * table->capacity + found->value;
			if (is_negative != NULL)
				*is_negative = 1;
			EC_POINT_free(cur_pos);
			EC_POINT_free(cur_neg);
			if (local_ctx) BN_CTX_free(ctx);
			return TECC_EC_ELGAMAL_SUCCESS;
		}

		/* 推进：cur -= mG（两个方向同步） */
		EC_POINT_add(group, cur_pos, cur_pos, table->mG_inv, ctx);
		EC_POINT_add(group, cur_neg, cur_neg, table->mG_inv, ctx);
	}

	EC_POINT_free(cur_pos);
	EC_POINT_free(cur_neg);
	if (local_ctx) BN_CTX_free(ctx);
	return TECC_EC_ELGAMAL_ERR_DECRYPT;	/* Not found */
}

/* ==================== Key Generation ==================== */

int
tecc_ec_elgamal_generate_keypair(const EC_GROUP *group,
								  BIGNUM **priv_key,
								  EC_POINT **pub_key)
{
	BIGNUM	   *ord;
	BIGNUM	   *x;
	EC_POINT   *Q;
	BN_CTX	   *ctx;

	if (group == NULL || priv_key == NULL || pub_key == NULL)
		return TECC_EC_ELGAMAL_ERR_NULL;

	ctx = BN_CTX_new();
	if (ctx == NULL)
		return TECC_EC_ELGAMAL_ERR_MEMORY;

	ord = BN_new();
	x = BN_new();
	Q = EC_POINT_new(group);
	if (ord == NULL || x == NULL || Q == NULL)
	{
		BN_free(ord);
		BN_free(x);
		EC_POINT_free(Q);
		BN_CTX_free(ctx);
		return TECC_EC_ELGAMAL_ERR_MEMORY;
	}

	/* Get curve order */
	if (EC_GROUP_get_order(group, ord, ctx) != 1)
	{
		BN_free(ord);
		BN_free(x);
		EC_POINT_free(Q);
		BN_CTX_free(ctx);
		return TECC_EC_ELGAMAL_ERR_CRYPTO;
	}

	/* Generate random private key: 0 < x < ord */
	if (BN_rand_range(x, ord) != 1)
	{
		BN_free(ord);
		BN_free(x);
		EC_POINT_free(Q);
		BN_CTX_free(ctx);
		return TECC_EC_ELGAMAL_ERR_CRYPTO;
	}

	/* Compute public key: Q = x * G */
	if (EC_POINT_mul(group, Q, x, NULL, NULL, ctx) != 1)
	{
		BN_free(ord);
		BN_free(x);
		EC_POINT_free(Q);
		BN_CTX_free(ctx);
		return TECC_EC_ELGAMAL_ERR_CRYPTO;
	}

	*priv_key = x;
	*pub_key = Q;

	BN_free(ord);
	BN_CTX_free(ctx);
	return TECC_EC_ELGAMAL_SUCCESS;
}

/* ==================== Core Encryption / Decryption ==================== */

int
tecc_ec_elgamal_encrypt(const EC_GROUP *group, const EC_POINT *Q,
						const BIGNUM *m, EC_POINT *C1, EC_POINT *C2,
						BN_CTX *ctx)
{
	BIGNUM	   *ord;
	BIGNUM	   *r;
	int			local_ctx = (ctx == NULL);
	int			ret;

	if (group == NULL || Q == NULL || m == NULL || C1 == NULL || C2 == NULL)
		return TECC_EC_ELGAMAL_ERR_NULL;

	if (local_ctx)
	{
		ctx = BN_CTX_new();
		if (ctx == NULL)
			return TECC_EC_ELGAMAL_ERR_MEMORY;
	}

	ord = BN_new();
	r = BN_new();
	if (ord == NULL || r == NULL)
	{
		BN_free(ord);
		BN_free(r);
		if (local_ctx) BN_CTX_free(ctx);
		return TECC_EC_ELGAMAL_ERR_MEMORY;
	}

	/* Get curve order */
	if (EC_GROUP_get_order(group, ord, ctx) != 1)
	{
		BN_free(ord);
		BN_free(r);
		if (local_ctx) BN_CTX_free(ctx);
		return TECC_EC_ELGAMAL_ERR_CRYPTO;
	}

	/* Generate random nonce: 0 < r < ord */
	if (BN_rand_range(r, ord) != 1)
	{
		BN_free(ord);
		BN_free(r);
		if (local_ctx) BN_CTX_free(ctx);
		return TECC_EC_ELGAMAL_ERR_CRYPTO;
	}

	/* C1 = r * G */
	if (EC_POINT_mul(group, C1, NULL, EC_GROUP_get0_generator(group), r, ctx) != 1)
	{
		BN_free(ord);
		BN_free(r);
		if (local_ctx) BN_CTX_free(ctx);
		return TECC_EC_ELGAMAL_ERR_CRYPTO;
	}

	/* C2 = m * G + r * Q */
	{
		EC_POINT *mG = EC_POINT_new(group);
		EC_POINT *rQ = EC_POINT_new(group);

		if (mG == NULL || rQ == NULL)
		{
			EC_POINT_free(mG);
			EC_POINT_free(rQ);
			BN_free(ord);
			BN_free(r);
			if (local_ctx) BN_CTX_free(ctx);
			return TECC_EC_ELGAMAL_ERR_MEMORY;
		}

		/* mG = m * G */
		EC_POINT_mul(group, mG, m, NULL, NULL, ctx);
		/* rQ = r * Q */
		EC_POINT_mul(group, rQ, NULL, Q, r, ctx);
		/* C2 = mG + rQ */
		EC_POINT_add(group, C2, mG, rQ, ctx);

		EC_POINT_free(mG);
		EC_POINT_free(rQ);
	}

	ret = TECC_EC_ELGAMAL_SUCCESS;

	BN_free(ord);
	BN_free(r);
	if (local_ctx) BN_CTX_free(ctx);
	return ret;
}

int
tecc_ec_elgamal_decrypt(const EC_GROUP *group, const BIGNUM *x,
						const EC_POINT *C1, const EC_POINT *C2,
						BIGNUM *m, TeccECBSGSTable *bsgs_table,
						BN_CTX *ctx)
{
	EC_POINT   *M;
	uint64_t	plaintext = 0;
	int			local_ctx = (ctx == NULL);
	int			ret;

	if (group == NULL || x == NULL || C1 == NULL || C2 == NULL || m == NULL)
		return TECC_EC_ELGAMAL_ERR_NULL;

	if (local_ctx)
	{
		ctx = BN_CTX_new();
		if (ctx == NULL)
			return TECC_EC_ELGAMAL_ERR_MEMORY;
	}

	M = EC_POINT_new(group);
	if (M == NULL)
	{
		if (local_ctx) BN_CTX_free(ctx);
		return TECC_EC_ELGAMAL_ERR_MEMORY;
	}

	/* M = x * C1 */
	EC_POINT_mul(group, M, NULL, C1, x, ctx);
	/* M = -x * C1 (invert) */
	EC_POINT_invert(group, M, ctx);
	/* M = C2 - x*C1 */
	EC_POINT_add(group, M, C2, M, ctx);

	/* Solve DLP: find m such that M = m * G */
	if (bsgs_table != NULL)
		{
			/* ReqID:TECC4.0 - giant-step 迭代次数独立于 baby-step 表容量：
		 * 可解上限 = capacity × max_iter = 2^14 × 2^18 = 2^32。
		 * 单值解密（余数 < 2^12）giant step 仅需 <4 步，不受影响；
		 * 聚合解密（SUM 标量大）才需要更多迭代。 */
		uint64_t	max_iter = (uint64_t) TECC_EC_BSGS_MAX_ITER;
		int			negative = 0;

		/* ReqID:TECC4.0 - 双向 BSGS 解 M 和 -M（负值无需二次遍历满 max_iter）。 */
		ret = tecc_ec_bsgs_solve(&plaintext, bsgs_table, group, M,
								 max_iter, ctx, &negative);
		if (ret == TECC_EC_ELGAMAL_SUCCESS)
		{
			BN_set_word(m, (BN_ULONG) plaintext);
			if (negative)
				BN_set_negative(m, 1);
			EC_POINT_free(M);
			if (local_ctx) BN_CTX_free(ctx);
			return TECC_EC_ELGAMAL_SUCCESS;
		}
		EC_POINT_free(M);
		if (local_ctx) BN_CTX_free(ctx);
		return ret;
	}
	else
	{
		/* Brute force fallback (very limited) */
		EC_POINT   *cur;
		const EC_POINT *G = EC_GROUP_get0_generator(group);
		uint64_t	i;

		cur = EC_POINT_new(group);
		if (cur == NULL)
		{
			EC_POINT_free(M);
			if (local_ctx) BN_CTX_free(ctx);
			return TECC_EC_ELGAMAL_ERR_MEMORY;
		}
		EC_POINT_set_to_infinity(group, cur);

		if (EC_POINT_is_at_infinity(group, M))
		{
			plaintext = 0;
		}
		else
		{
			for (i = 1; i < (1ULL << 20); i++)
			{
				EC_POINT_add(group, cur, cur, G, ctx);
				if (EC_POINT_cmp(group, cur, M, ctx) == 0)
				{
					plaintext = i;
					break;
				}
			}
			if (i >= (1ULL << 20))
			{
				EC_POINT_free(cur);
				EC_POINT_free(M);
				if (local_ctx) BN_CTX_free(ctx);
				return TECC_EC_ELGAMAL_ERR_DECRYPT;
			}
		}
		EC_POINT_free(cur);
	}

	BN_set_word(m, (BN_ULONG) plaintext);
	EC_POINT_free(M);
	if (local_ctx) BN_CTX_free(ctx);
	return TECC_EC_ELGAMAL_SUCCESS;
}

/* ==================== Homomorphic Operations ==================== */

int
tecc_ec_elgamal_add(const EC_GROUP *group,
					EC_POINT *res_C1, EC_POINT *res_C2,
					const EC_POINT *a_C1, const EC_POINT *a_C2,
					const EC_POINT *b_C1, const EC_POINT *b_C2,
					BN_CTX *ctx)
{
	int	local_ctx = (ctx == NULL);

	if (group == NULL || res_C1 == NULL || res_C2 == NULL ||
		a_C1 == NULL || a_C2 == NULL || b_C1 == NULL || b_C2 == NULL)
		return TECC_EC_ELGAMAL_ERR_NULL;

	if (local_ctx)
	{
		ctx = BN_CTX_new();
		if (ctx == NULL)
			return TECC_EC_ELGAMAL_ERR_MEMORY;
	}

	/* C1 = C1_a + C1_b */
	EC_POINT_add(group, res_C1, a_C1, b_C1, ctx);
	/* C2 = C2_a + C2_b */
	EC_POINT_add(group, res_C2, a_C2, b_C2, ctx);

	if (local_ctx) BN_CTX_free(ctx);
	return TECC_EC_ELGAMAL_SUCCESS;
}

int
tecc_ec_elgamal_sub(const EC_GROUP *group,
					EC_POINT *res_C1, EC_POINT *res_C2,
					const EC_POINT *a_C1, const EC_POINT *a_C2,
					const EC_POINT *b_C1, const EC_POINT *b_C2,
					BN_CTX *ctx)
{
	int	local_ctx = (ctx == NULL);

	if (group == NULL || res_C1 == NULL || res_C2 == NULL ||
		a_C1 == NULL || a_C2 == NULL || b_C1 == NULL || b_C2 == NULL)
		return TECC_EC_ELGAMAL_ERR_NULL;

	if (local_ctx)
	{
		ctx = BN_CTX_new();
		if (ctx == NULL)
			return TECC_EC_ELGAMAL_ERR_MEMORY;
	}

	/* C1 = C1_a - C1_b = C1_a + (-C1_b), C2 = C2_a - C2_b = C2_a + (-C2_b) */
	{
		EC_POINT *tmp;

		tmp = EC_POINT_dup(b_C1, group);
		if (tmp == NULL)
		{
			if (local_ctx) BN_CTX_free(ctx);
			return TECC_EC_ELGAMAL_ERR_MEMORY;
		}
		EC_POINT_invert(group, tmp, ctx);
		EC_POINT_add(group, res_C1, a_C1, tmp, ctx);

		EC_POINT_copy(tmp, b_C2);
		EC_POINT_invert(group, tmp, ctx);
		EC_POINT_add(group, res_C2, a_C2, tmp, ctx);

		EC_POINT_free(tmp);
	}

	if (local_ctx) BN_CTX_free(ctx);
	return TECC_EC_ELGAMAL_SUCCESS;
}

int
tecc_ec_elgamal_scalar_mul(const EC_GROUP *group,
						   EC_POINT *res_C1, EC_POINT *res_C2,
						   const EC_POINT *C1, const EC_POINT *C2,
						   const BIGNUM *k, BN_CTX *ctx)
{
	int	local_ctx = (ctx == NULL);

	if (group == NULL || res_C1 == NULL || res_C2 == NULL ||
		C1 == NULL || C2 == NULL || k == NULL)
		return TECC_EC_ELGAMAL_ERR_NULL;

	if (local_ctx)
	{
		ctx = BN_CTX_new();
		if (ctx == NULL)
			return TECC_EC_ELGAMAL_ERR_MEMORY;
	}

	/* C1 = k * C1 */
	EC_POINT_mul(group, res_C1, NULL, C1, k, ctx);
	/* C2 = k * C2 */
	EC_POINT_mul(group, res_C2, NULL, C2, k, ctx);

	if (local_ctx) BN_CTX_free(ctx);
	return TECC_EC_ELGAMAL_SUCCESS;
}

/* ==================== CRT Parameter Management ==================== */

/* ReqID:TECC4.0 - Delegate to init_ex for dynamic prime count */
int
tecc_ec_crt_params_init(TeccECCRTParams *params)
{
	return tecc_ec_crt_params_init_ex(params, TECC_EC_CRT_NUM_PRIMES);
}

/* ReqID:TECC4.0 - 固定 CRT 素数表（跨进程一致）：
 * EC-ElGamal CRT 分块解密的素数必须加解密双方一致。原实现每次随机生成
 * （BN_generate_prime_ex）且不序列化，服务端加密（seed/存储）与客户端
 * 解密（运行时）跨进程素数不同，导致明文 ≥ 最小素数（~3100）时 BSGS
 * 解余数 + CRT 重建失败（大值解密错误，perf he_int8 的 id≥5001 行）。
 * 改为固定素数表：8 个 16 位素数，总模数 ≈ 2^128 > 2^63（int8 范围），
 * 明文正数上限 ≈ 最小素数（65422），远大于原随机 12 位素数（~3100）。
 * BSGS 表容量 16384、max_iter=16384，可解到 ~2^28，覆盖 16 位余数。 */
static const unsigned int tecc_crt_fixed_primes[TECC_EC_CRT_NUM_PRIMES] = {
	65521u, 65519u, 65497u, 65479u,
	65449u, 65447u, 65437u, 65423u
};

/*
 * Initialize CRT parameters with configurable partition count.
 * Uses the fixed prime table above so that ciphertext produced in one
 * process (server-side seed/encryption) can be decrypted in another
 * (client-side runtime decryption).
 */
int
tecc_ec_crt_params_init_ex(TeccECCRTParams *params, int num_primes)
{
	int			i;
	BN_CTX	   *ctx;

	if (params == NULL || num_primes < 2 || num_primes > TECC_EC_CRT_NUM_PRIMES)
		return TECC_EC_ELGAMAL_ERR_INVALID;

	params->num_primes = num_primes;
	params->bits_per_prime = 16;	/* 固定素数表为 16 位 */

	params->primes = (BIGNUM **) calloc(num_primes, sizeof(BIGNUM *));
	if (params->primes == NULL)
		return TECC_EC_ELGAMAL_ERR_MEMORY;

	params->modulus = BN_new();
	if (params->modulus == NULL)
	{
		free(params->primes);
		return TECC_EC_ELGAMAL_ERR_MEMORY;
	}
	BN_set_word(params->modulus, 1);

	ctx = BN_CTX_new();
	if (ctx == NULL)
	{
		BN_free(params->modulus);
		free(params->primes);
		return TECC_EC_ELGAMAL_ERR_MEMORY;
	}

	for (i = 0; i < num_primes; i++)
	{
		params->primes[i] = BN_new();
		if (params->primes[i] == NULL)
		{
			int j;

			for (j = 0; j < i; j++)
				BN_free(params->primes[j]);
			BN_free(params->modulus);
			free(params->primes);
			BN_CTX_free(ctx);
			return TECC_EC_ELGAMAL_ERR_MEMORY;
		}

		BN_set_word(params->primes[i], tecc_crt_fixed_primes[i]);

		/* modulus *= primes[i] */
		BN_mul(params->modulus, params->modulus, params->primes[i], ctx);
	}

	BN_CTX_free(ctx);
	return TECC_EC_ELGAMAL_SUCCESS;
}


void
tecc_ec_crt_params_free(TeccECCRTParams *params)
{
	int i;

	if (params == NULL)
		return;

	if (params->primes != NULL)
	{
		for (i = 0; i < params->num_primes; i++)
		{
			if (params->primes[i] != NULL)
				BN_free(params->primes[i]);
		}
		free(params->primes);
		params->primes = NULL;
	}
	if (params->modulus != NULL)
	{
		BN_free(params->modulus);
		params->modulus = NULL;
	}
	params->num_primes = 0;
}

/* ==================== CRT Ciphertext Management ==================== */

int
tecc_ec_crt_ciphertext_init(TeccECCRTCiphertext *ct,
							const EC_GROUP *group, int num_splits)
{
	int i;

	if (ct == NULL || group == NULL || num_splits <= 0)
		return TECC_EC_ELGAMAL_ERR_NULL;

	ct->C1 = (EC_POINT **) calloc(num_splits, sizeof(EC_POINT *));
	ct->C2 = (EC_POINT **) calloc(num_splits, sizeof(EC_POINT *));
	if (ct->C1 == NULL || ct->C2 == NULL)
	{
		free(ct->C1);
		free(ct->C2);
		return TECC_EC_ELGAMAL_ERR_MEMORY;
	}

	ct->num_ciphertexts = num_splits;

	for (i = 0; i < num_splits; i++)
	{
		ct->C1[i] = EC_POINT_new(group);
		ct->C2[i] = EC_POINT_new(group);
		if (ct->C1[i] == NULL || ct->C2[i] == NULL)
		{
			tecc_ec_crt_ciphertext_free(ct, group);
			return TECC_EC_ELGAMAL_ERR_MEMORY;
		}
	}

	return TECC_EC_ELGAMAL_SUCCESS;
}

void
tecc_ec_crt_ciphertext_free(TeccECCRTCiphertext *ct, const EC_GROUP *group)
{
	int i;

	if (ct == NULL)
		return;

	if (ct->C1 != NULL)
	{
		for (i = 0; i < ct->num_ciphertexts; i++)
		{
			if (ct->C1[i] != NULL)
				EC_POINT_free(ct->C1[i]);
		}
		free(ct->C1);
		ct->C1 = NULL;
	}
	if (ct->C2 != NULL)
	{
		for (i = 0; i < ct->num_ciphertexts; i++)
		{
			if (ct->C2[i] != NULL)
				EC_POINT_free(ct->C2[i]);
		}
		free(ct->C2);
		ct->C2 = NULL;
	}
	ct->num_ciphertexts = 0;
}

/* ==================== CRT Core Operations ==================== */

/*
 * Solve CRT: given residues r_i = val mod p_i, compute val mod P
 * where P = p_1 * p_2 * ... * p_n
 */
static int
solve_crt(BIGNUM *result, int count, BIGNUM **residues,
		  BIGNUM **primes, BIGNUM *modulus, BN_CTX *ctx)
{
	int			i;
	int			local_ctx = (ctx == NULL);
	BIGNUM	   *tmp1;
	BIGNUM	   *tmp2;

	if (local_ctx)
	{
		ctx = BN_CTX_new();
		if (ctx == NULL)
			return TECC_EC_ELGAMAL_ERR_MEMORY;
	}

	tmp1 = BN_new();
	tmp2 = BN_new();
	if (tmp1 == NULL || tmp2 == NULL)
	{
		BN_free(tmp1);
		BN_free(tmp2);
		if (local_ctx) BN_CTX_free(ctx);
		return TECC_EC_ELGAMAL_ERR_MEMORY;
	}

	BN_zero(result);
	for (i = 0; i < count; i++)
	{
		/* tmp1 = P / p_i */
		BN_div(tmp1, NULL, modulus, primes[i], ctx);
		/* tmp2 = r_i * (P/p_i) */
		BN_mul(tmp2, residues[i], tmp1, ctx);
		/* tmp1 = (P/p_i)^{-1} mod p_i */
		BN_mod_inverse(tmp1, tmp1, primes[i], ctx);
		/* tmp2 = r_i * (P/p_i) * inv(P/p_i, p_i) */
		BN_mul(tmp2, tmp2, tmp1, ctx);
		/* result += tmp2 */
		BN_add(result, result, tmp2);
	}

	BN_mod(result, result, modulus, ctx);

	BN_free(tmp1);
	BN_free(tmp2);
	if (local_ctx) BN_CTX_free(ctx);
	return TECC_EC_ELGAMAL_SUCCESS;
}

/* ==================== CRT Parallel Encryption Worker ==================== */

/* ReqID:TECC4.0 - 算法内部并行开关：环境变量 TECC_HE_ALGO_PARALLEL，默认启用
 * （值为 0/false/off 时关闭）。用于 he-add/he-mul 单次加密内部的固定小线程并行
 * （he-add 8 分块、he-mul 2 模幂）。
 * 线程局部覆盖（tecc_he_algo_parallel_tls）：字段级并行 worker 内设为 0，关闭
 * 算法内部并行避免叠加（N worker × M 内部线程超订阅）；-1 表示不覆盖。 */
static int tecc_he_algo_parallel_enabled = -1;
static pthread_once_t tecc_he_algo_parallel_once = PTHREAD_ONCE_INIT;

#if defined(__GNUC__) || defined(__clang__)
static __thread int tecc_he_algo_parallel_tls = -1;
#else
static int tecc_he_algo_parallel_tls = -1;
#endif

static void
tecc_he_algo_parallel_init(void)
{
	const char *env = getenv("TECC_HE_ALGO_PARALLEL");

	tecc_he_algo_parallel_enabled =
		(env == NULL || env[0] == '\0' ||
		 (env[0] != '0' && env[0] != 'f' && env[0] != 'F' &&
		  env[0] != 'n' && env[0] != 'N')) ? 1 : 0;
}

/* ReqID:TECC4.0 - 算法内部并行是否启用：优先 TLS 覆盖，否则读环境变量（缓存）。
 * 导出供 tecc_elgamal.c（he-mul）与字段级并行决策复用。 */
int
tecc_he_algo_parallel_check(void)
{
	if (tecc_he_algo_parallel_tls >= 0)
		return tecc_he_algo_parallel_tls;
	pthread_once(&tecc_he_algo_parallel_once, tecc_he_algo_parallel_init);
	return tecc_he_algo_parallel_enabled;
}

/* ReqID:TECC4.0 - 线程局部覆盖算法内部并行（字段级并行 worker 内关闭避免叠加）。
 * enabled=-1 恢复默认。导出供 values_processor 字段级并行调用。 */
void
tecc_he_set_algo_parallel_override(int enabled)
{
	tecc_he_algo_parallel_tls = enabled;
}

/* Worker context for parallel CRT encrypt */
typedef struct CRTEncryptWorkerCtx
{
	const EC_GROUP		*group;
	const EC_POINT		*Q;
	const BIGNUM		*plaintext;
	const BIGNUM		**primes;	/* slice of params->primes */
	EC_POINT			**C1;		/* slice of ct->C1 */
	EC_POINT			**C2;		/* slice of ct->C2 */
	int					count;		/* number of partitions for this worker */
	int					ret;
} CRTEncryptWorkerCtx;

#ifdef _WIN32
static DWORD WINAPI
crt_encrypt_worker_thread(LPVOID arg)
#else
static void *
crt_encrypt_worker_thread(void *arg)
#endif
{
	CRTEncryptWorkerCtx *w = (CRTEncryptWorkerCtx *) arg;
	BN_CTX				*local_ctx;
	BIGNUM				*residue;
	int					i;

	local_ctx = BN_CTX_new();
	residue = BN_new();
	if (local_ctx == NULL || residue == NULL)
	{
		w->ret = TECC_EC_ELGAMAL_ERR_MEMORY;
		if (local_ctx != NULL)
			BN_CTX_free(local_ctx);
		if (residue != NULL)
			BN_free(residue);
#ifdef _WIN32
		return (DWORD) w->ret;
#else
		return NULL;
#endif
	}

	w->ret = TECC_EC_ELGAMAL_SUCCESS;
	for (i = 0; i < w->count && w->ret == TECC_EC_ELGAMAL_SUCCESS; i++)
	{
		/* residue = plaintext mod primes[i] */
		BN_mod(residue, w->plaintext, w->primes[i], local_ctx);
		/* Encrypt the residue */
		w->ret = tecc_ec_elgamal_encrypt(w->group, w->Q, residue,
										 w->C1[i], w->C2[i], local_ctx);
	}

	BN_free(residue);
	BN_CTX_free(local_ctx);

#ifdef _WIN32
	return (DWORD) w->ret;
#else
	return NULL;
#endif
}

/*
 * ReqID:TECC4.0 - Parallel CRT-mode EC-ElGamal encryption.
 * Distributing the 8 CRT partitions across worker threads mirrors
 * tecc_ec_crt_decrypt_parallel: each worker encrypts its partition slice
 * independently (each residue is independent), no recombination needed.
 */
static int
tecc_ec_crt_encrypt_parallel(TeccECCRTCiphertext *ct,
							 const EC_GROUP *group, const EC_POINT *Q,
							 const BIGNUM *plaintext,
							 const TeccECCRTParams *params,
							 BN_CTX *ctx)
{
	int					nparts;
	int					nthreads;
	int					i;
	int					ret;
	CRTEncryptWorkerCtx *workers;
#ifdef _WIN32
	HANDLE			  *threads;
#else
	pthread_t		  *threads;
#endif

	if (ct == NULL || group == NULL || Q == NULL ||
		plaintext == NULL || params == NULL)
		return TECC_EC_ELGAMAL_ERR_NULL;

	nparts = params->num_primes;
	nthreads = nparts;
	if (nthreads > TECC_EC_CRT_MAX_THREADS)
		nthreads = TECC_EC_CRT_MAX_THREADS;
	if (nthreads <= 1)
	{
		/* Fallback to serial when only one effective thread */
		int local_ctx = (ctx == NULL);
		BIGNUM *residue;
		int sret;

		if (local_ctx)
		{
			ctx = BN_CTX_new();
			if (ctx == NULL)
				return TECC_EC_ELGAMAL_ERR_MEMORY;
		}
		residue = BN_new();
		if (residue == NULL)
		{
			if (local_ctx) BN_CTX_free(ctx);
			return TECC_EC_ELGAMAL_ERR_MEMORY;
		}
		sret = TECC_EC_ELGAMAL_SUCCESS;
		for (i = 0; i < nparts && sret == TECC_EC_ELGAMAL_SUCCESS; i++)
		{
			BN_mod(residue, plaintext, params->primes[i], ctx);
			sret = tecc_ec_elgamal_encrypt(group, Q, residue,
										   ct->C1[i], ct->C2[i], ctx);
		}
		BN_free(residue);
		if (local_ctx) BN_CTX_free(ctx);
		return sret;
	}

	workers = (CRTEncryptWorkerCtx *) calloc(nthreads, sizeof(CRTEncryptWorkerCtx));
	threads =
#ifdef _WIN32
		(HANDLE *) calloc(nthreads, sizeof(HANDLE));
#else
		(pthread_t *) calloc(nthreads, sizeof(pthread_t));
#endif
	if (workers == NULL || threads == NULL)
	{
		free(workers);
		free(threads);
		return TECC_EC_ELGAMAL_ERR_MEMORY;
	}

	/* Distribute partitions across workers (round-robin) */
	{
		int base = nparts / nthreads;
		int extra = nparts % nthreads;
		int offset = 0;

		for (i = 0; i < nthreads; i++)
		{
			int chunk = base + (i < extra ? 1 : 0);

			workers[i].group = group;
			workers[i].Q = Q;
			workers[i].plaintext = plaintext;
			workers[i].primes = (const BIGNUM **) &params->primes[offset];
			workers[i].C1 = (EC_POINT **) &ct->C1[offset];
			workers[i].C2 = (EC_POINT **) &ct->C2[offset];
			workers[i].count = chunk;
			workers[i].ret = TECC_EC_ELGAMAL_ERR_CRYPTO;
			offset += chunk;
		}
	}

	/* Launch worker threads */
	for (i = 0; i < nthreads; i++)
	{
#ifdef _WIN32
		threads[i] = CreateThread(NULL, 0, crt_encrypt_worker_thread,
								  &workers[i], 0, NULL);
		if (threads[i] == NULL)
			workers[i].ret = TECC_EC_ELGAMAL_ERR_MEMORY;
#else
		if (pthread_create(&threads[i], NULL,
						   crt_encrypt_worker_thread, &workers[i]) != 0)
			workers[i].ret = TECC_EC_ELGAMAL_ERR_MEMORY;
#endif
	}

	/* Wait for all workers */
#ifdef _WIN32
	WaitForMultipleObjects(nthreads, threads, TRUE, INFINITE);
	for (i = 0; i < nthreads; i++)
	{
		if (threads[i] != NULL)
			CloseHandle(threads[i]);
	}
#else
	for (i = 0; i < nthreads; i++)
	{
		if (threads[i])
			pthread_join(threads[i], NULL);
	}
#endif

	/* Check for errors in any worker */
	ret = TECC_EC_ELGAMAL_SUCCESS;
	for (i = 0; i < nthreads && ret == TECC_EC_ELGAMAL_SUCCESS; i++)
	{
		if (workers[i].ret != TECC_EC_ELGAMAL_SUCCESS)
			ret = workers[i].ret;
	}

	free(workers);
	free(threads);
	return ret;
}

int
tecc_ec_crt_encrypt(TeccECCRTCiphertext *ct,
					const EC_GROUP *group, const EC_POINT *Q,
					const BIGNUM *plaintext,
					const TeccECCRTParams *params,
					BN_CTX *ctx)
{
	int			i;
	int			local_ctx = (ctx == NULL);
	BIGNUM	   *residue;
	int			ret;

	if (ct == NULL || group == NULL || Q == NULL ||
		plaintext == NULL || params == NULL)
		return TECC_EC_ELGAMAL_ERR_NULL;

	/* Ensure ciphertext has the right number of partitions */
	if (ct->num_ciphertexts != params->num_primes)
		return TECC_EC_ELGAMAL_ERR_INVALID;

	/* ReqID:TECC4.0 - 算法内部并行：8 分块并行加密（默认启用，环境变量
	 * TECC_HE_ALGO_PARALLEL 或 TLS 覆盖可关闭）。 */
	if (tecc_he_algo_parallel_check() && params->num_primes > 1)
		return tecc_ec_crt_encrypt_parallel(ct, group, Q, plaintext, params, ctx);

	if (local_ctx)
	{
		ctx = BN_CTX_new();
		if (ctx == NULL)
			return TECC_EC_ELGAMAL_ERR_MEMORY;
	}

	residue = BN_new();
	if (residue == NULL)
	{
		if (local_ctx) BN_CTX_free(ctx);
		return TECC_EC_ELGAMAL_ERR_MEMORY;
	}

	ret = TECC_EC_ELGAMAL_SUCCESS;
	for (i = 0; i < params->num_primes && ret == TECC_EC_ELGAMAL_SUCCESS; i++)
	{
		/* residue = plaintext mod primes[i] */
		BN_mod(residue, plaintext, params->primes[i], ctx);

		/* Encrypt the residue */
		ret = tecc_ec_elgamal_encrypt(group, Q, residue,
									  ct->C1[i], ct->C2[i], ctx);
	}

	BN_free(residue);
	if (local_ctx) BN_CTX_free(ctx);
	return ret;
}

int
tecc_ec_crt_decrypt(BIGNUM *result,
					const EC_GROUP *group, const BIGNUM *x,
					const TeccECCRTCiphertext *ct,
					const TeccECCRTParams *params,
					TeccECBSGSTable **bsgs_tables,
					BN_CTX *ctx)
{
	int			i;
	int			local_ctx = (ctx == NULL);
	BIGNUM	  **residues;
	int			ret;

	if (result == NULL || group == NULL || x == NULL ||
		ct == NULL || params == NULL || bsgs_tables == NULL)
		return TECC_EC_ELGAMAL_ERR_NULL;

	if (ct->num_ciphertexts != params->num_primes)
		return TECC_EC_ELGAMAL_ERR_INVALID;

	if (local_ctx)
	{
		ctx = BN_CTX_new();
		if (ctx == NULL)
			return TECC_EC_ELGAMAL_ERR_MEMORY;
	}

	residues = (BIGNUM **) calloc(params->num_primes, sizeof(BIGNUM *));
	if (residues == NULL)
	{
		if (local_ctx) BN_CTX_free(ctx);
		return TECC_EC_ELGAMAL_ERR_MEMORY;
	}

	ret = TECC_EC_ELGAMAL_SUCCESS;
	for (i = 0; i < params->num_primes && ret == TECC_EC_ELGAMAL_SUCCESS; i++)
	{
		residues[i] = BN_new();
		if (residues[i] == NULL)
		{
			ret = TECC_EC_ELGAMAL_ERR_MEMORY;
			break;
		}
		ret = tecc_ec_elgamal_decrypt(group, x,
									  ct->C1[i], ct->C2[i],
									  residues[i], bsgs_tables[i], ctx);
	}

	if (ret == TECC_EC_ELGAMAL_SUCCESS)
	{
		/* Solve CRT to recombine */
		ret = solve_crt(result, params->num_primes, residues,
						params->primes, params->modulus, ctx);

		/* Handle negative values */
		if (ret == TECC_EC_ELGAMAL_SUCCESS)
			tecc_ec_adjust_negative(result, params->modulus);
	}

	for (i = 0; i < params->num_primes; i++)
	{
		if (residues[i] != NULL)
			BN_free(residues[i]);
	}
	free(residues);

	if (local_ctx) BN_CTX_free(ctx);
	return ret;
}

/* ==================== CRT Parallel Decryption Worker ==================== */

/* Worker context for parallel CRT decrypt */
typedef struct CRTDecryptWorkerCtx
{
	const EC_GROUP		*group;
	const BIGNUM		*x;
	const EC_POINT		**C1;		/* array of C1 pointers (our partition slice) */
	const EC_POINT		**C2;		/* array of C2 pointers */
	TeccECBSGSTable		**bsgs;		/* BSGS tables for our partitions */
	BIGNUM				**residues;	/* output residues */
	int					count;		/* number of partitions for this worker */
	int					ret;
} CRTDecryptWorkerCtx;

#ifdef _WIN32
static DWORD WINAPI
crt_decrypt_worker_thread(LPVOID arg)
#else
static void *
crt_decrypt_worker_thread(void *arg)
#endif
{
	CRTDecryptWorkerCtx *w = (CRTDecryptWorkerCtx *) arg;
	BN_CTX				*local_ctx;
	int					i;

	local_ctx = BN_CTX_new();
	if (local_ctx == NULL)
	{
		w->ret = TECC_EC_ELGAMAL_ERR_MEMORY;
#ifdef _WIN32
		return (DWORD) w->ret;
#else
		return NULL;
#endif
	}

	w->ret = TECC_EC_ELGAMAL_SUCCESS;
	for (i = 0; i < w->count && w->ret == TECC_EC_ELGAMAL_SUCCESS; i++)
	{
		w->residues[i] = BN_new();
		if (w->residues[i] == NULL)
		{
			w->ret = TECC_EC_ELGAMAL_ERR_MEMORY;
			break;
		}
		w->ret = tecc_ec_elgamal_decrypt(w->group, w->x,
										  w->C1[i], w->C2[i],
										  w->residues[i], w->bsgs[i], local_ctx);
	}

	BN_CTX_free(local_ctx);

#ifdef _WIN32
	return (DWORD) w->ret;
#else
	return NULL;
#endif
}

/* ==================== CRT Parallel Decrypt ==================== */

/*
 * Parallel CRT-mode EC-ElGamal decryption.
 *
 * Distributes CRT partitions across num_threads worker threads, each thread
 * decrypts its assigned partitions independently. The main thread then
 * recombines all residues via CRT.
 *
 * Uses the platform threading API (CreateThread on Windows, pthread on POSIX).
 * Each worker gets its own BN_CTX for thread safety.
 *
 * num_threads: number of worker threads. Clamped to [1, num_primes].
 *              Use 1 for serial fallback. Use 0 to auto-detect (= num_primes).
 */
int
tecc_ec_crt_decrypt_parallel(BIGNUM *result,
							 const EC_GROUP *group, const BIGNUM *x,
							 const TeccECCRTCiphertext *ct,
							 const TeccECCRTParams *params,
							 TeccECBSGSTable **bsgs_tables,
							 int num_threads,
							 BN_CTX *ctx)
{
	int					nparts;
	int					nthreads;
	int					i;
	int					local_ctx;
	int					ret;
	BIGNUM			  **residues_all;
	CRTDecryptWorkerCtx *workers;
#ifdef _WIN32
	HANDLE			  *threads;
#else
	pthread_t		  *threads;
#endif

	if (result == NULL || group == NULL || x == NULL ||
		ct == NULL || params == NULL || bsgs_tables == NULL)
		return TECC_EC_ELGAMAL_ERR_NULL;

	if (ct->num_ciphertexts != params->num_primes)
		return TECC_EC_ELGAMAL_ERR_INVALID;

	nparts = params->num_primes;
	nthreads = num_threads;
	if (nthreads <= 0 || nthreads > nparts)
		nthreads = nparts;
	if (nthreads > TECC_EC_CRT_MAX_THREADS)
		nthreads = TECC_EC_CRT_MAX_THREADS;

	/* If single thread, fall back to serial */
	if (nthreads <= 1)
		return tecc_ec_crt_decrypt(result, group, x, ct, params,
								   bsgs_tables, ctx);

	local_ctx = (ctx == NULL);
	if (local_ctx)
	{
		ctx = BN_CTX_new();
		if (ctx == NULL)
			return TECC_EC_ELGAMAL_ERR_MEMORY;
	}

	/* Allocate residue array for all partitions */
	residues_all = (BIGNUM **) calloc(nparts, sizeof(BIGNUM *));
	if (residues_all == NULL)
	{
		if (local_ctx) BN_CTX_free(ctx);
		return TECC_EC_ELGAMAL_ERR_MEMORY;
	}

	/* Allocate worker contexts */
	workers = (CRTDecryptWorkerCtx *) calloc(nthreads, sizeof(CRTDecryptWorkerCtx));
	threads = 
#ifdef _WIN32
		(HANDLE *) calloc(nthreads, sizeof(HANDLE));
#else
		(pthread_t *) calloc(nthreads, sizeof(pthread_t));
#endif
	if (workers == NULL || threads == NULL)
	{
		free(residues_all);
		free(workers);
		free(threads);
		if (local_ctx) BN_CTX_free(ctx);
		return TECC_EC_ELGAMAL_ERR_MEMORY;
	}

	/* Distribute partitions across workers (round-robin) */
	{
		int base = nparts / nthreads;
		int extra = nparts % nthreads;
		int offset = 0;

		for (i = 0; i < nthreads; i++)
		{
			int chunk = base + (i < extra ? 1 : 0);
			workers[i].group = group;
			workers[i].x = x;
			workers[i].C1 = (const EC_POINT **) &ct->C1[offset];
			workers[i].C2 = (const EC_POINT **) &ct->C2[offset];
			workers[i].bsgs = &bsgs_tables[offset];
			workers[i].residues = &residues_all[offset];
			workers[i].count = chunk;
			workers[i].ret = TECC_EC_ELGAMAL_ERR_DECRYPT;
			offset += chunk;
		}
	}

	/* Launch worker threads */
	for (i = 0; i < nthreads; i++)
	{
#ifdef _WIN32
		threads[i] = CreateThread(NULL, 0, crt_decrypt_worker_thread,
								  &workers[i], 0, NULL);
		if (threads[i] == NULL)
		{
			/* Mark this worker as failed */
			workers[i].ret = TECC_EC_ELGAMAL_ERR_MEMORY;
		}
#else
		if (pthread_create(&threads[i], NULL,
						   crt_decrypt_worker_thread, &workers[i]) != 0)
		{
			workers[i].ret = TECC_EC_ELGAMAL_ERR_MEMORY;
		}
#endif
	}

	/* Wait for all workers */
#ifdef _WIN32
	WaitForMultipleObjects(nthreads, threads, TRUE, INFINITE);
	for (i = 0; i < nthreads; i++)
	{
		if (threads[i] != NULL)
			CloseHandle(threads[i]);
	}
#else
	for (i = 0; i < nthreads; i++)
	{
		if (threads[i])
			pthread_join(threads[i], NULL);
	}
#endif

	/* Check for errors in any worker */
	ret = TECC_EC_ELGAMAL_SUCCESS;
	for (i = 0; i < nthreads && ret == TECC_EC_ELGAMAL_SUCCESS; i++)
	{
		if (workers[i].ret != TECC_EC_ELGAMAL_SUCCESS)
			ret = workers[i].ret;
	}

	/* CRT recombine (in main thread) */
	if (ret == TECC_EC_ELGAMAL_SUCCESS)
	{
		ret = solve_crt(result, nparts, residues_all,
						params->primes, params->modulus, ctx);
		if (ret == TECC_EC_ELGAMAL_SUCCESS)
			tecc_ec_adjust_negative(result, params->modulus);
	}

	/* Cleanup */
	for (i = 0; i < nparts; i++)
	{
		if (residues_all[i] != NULL)
			BN_free(residues_all[i]);
	}
	free(residues_all);
	free(workers);
	free(threads);

	if (local_ctx) BN_CTX_free(ctx);
	return ret;
}


/* ==================== CRT Homomorphic Operations ==================== */

int
tecc_ec_crt_add(const EC_GROUP *group,
				TeccECCRTCiphertext *res,
				const TeccECCRTCiphertext *a,
				const TeccECCRTCiphertext *b,
				BN_CTX *ctx)
{
	int i;
	int local_ctx = (ctx == NULL);

	if (group == NULL || res == NULL || a == NULL || b == NULL)
		return TECC_EC_ELGAMAL_ERR_NULL;

	if (a->num_ciphertexts != b->num_ciphertexts ||
		res->num_ciphertexts != a->num_ciphertexts)
		return TECC_EC_ELGAMAL_ERR_INVALID;

	if (local_ctx)
	{
		ctx = BN_CTX_new();
		if (ctx == NULL)
			return TECC_EC_ELGAMAL_ERR_MEMORY;
	}

	for (i = 0; i < a->num_ciphertexts; i++)
	{
		tecc_ec_elgamal_add(group, res->C1[i], res->C2[i],
							a->C1[i], a->C2[i],
							b->C1[i], b->C2[i], ctx);
	}

	if (local_ctx) BN_CTX_free(ctx);
	return TECC_EC_ELGAMAL_SUCCESS;
}

int
tecc_ec_crt_sub(const EC_GROUP *group,
				TeccECCRTCiphertext *res,
				const TeccECCRTCiphertext *a,
				const TeccECCRTCiphertext *b,
				BN_CTX *ctx)
{
	int i;
	int local_ctx = (ctx == NULL);

	if (group == NULL || res == NULL || a == NULL || b == NULL)
		return TECC_EC_ELGAMAL_ERR_NULL;

	if (a->num_ciphertexts != b->num_ciphertexts ||
		res->num_ciphertexts != a->num_ciphertexts)
		return TECC_EC_ELGAMAL_ERR_INVALID;

	if (local_ctx)
	{
		ctx = BN_CTX_new();
		if (ctx == NULL)
			return TECC_EC_ELGAMAL_ERR_MEMORY;
	}

	for (i = 0; i < a->num_ciphertexts; i++)
	{
		tecc_ec_elgamal_sub(group, res->C1[i], res->C2[i],
							a->C1[i], a->C2[i],
							b->C1[i], b->C2[i], ctx);
	}

	if (local_ctx) BN_CTX_free(ctx);
	return TECC_EC_ELGAMAL_SUCCESS;
}

int
tecc_ec_crt_scalar_mul(const EC_GROUP *group,
					   TeccECCRTCiphertext *res,
					   const TeccECCRTCiphertext *a,
					   const BIGNUM *k,
					   BN_CTX *ctx)
{
	int i;
	int local_ctx = (ctx == NULL);

	if (group == NULL || res == NULL || a == NULL || k == NULL)
		return TECC_EC_ELGAMAL_ERR_NULL;

	if (res->num_ciphertexts != a->num_ciphertexts)
		return TECC_EC_ELGAMAL_ERR_INVALID;

	if (local_ctx)
	{
		ctx = BN_CTX_new();
		if (ctx == NULL)
			return TECC_EC_ELGAMAL_ERR_MEMORY;
	}

	for (i = 0; i < a->num_ciphertexts; i++)
	{
		tecc_ec_elgamal_scalar_mul(group, res->C1[i], res->C2[i],
								   a->C1[i], a->C2[i], k, ctx);
	}

	if (local_ctx) BN_CTX_free(ctx);
	return TECC_EC_ELGAMAL_SUCCESS;
}

/* ==================== Serialization ==================== */

int
tecc_ec_crt_serialize_ex(uint8_t *buf, int buf_size,
						 const TeccECCRTCiphertext *ct,
						 const EC_GROUP *group, BN_CTX *ctx)
{
	int			i;
	int			offset;
	bool		local_ctx = (ctx == NULL);
	size_t		oct_len;

	if (buf == NULL || ct == NULL || group == NULL)
		return TECC_EC_ELGAMAL_ERR_NULL;

	if (buf_size < TECC_EC_CRT_MAX_CIPHERTEXT_SIZE)
		return TECC_EC_ELGAMAL_ERR_SIZE;

	if (local_ctx)
	{
		ctx = BN_CTX_new();
		if (ctx == NULL)
			return TECC_EC_ELGAMAL_ERR_MEMORY;
	}

	/* Write number of splits (1 byte) */
	buf[0] = (uint8_t) ct->num_ciphertexts;
	offset = 1;

	for (i = 0; i < ct->num_ciphertexts; i++)
	{
		/* Handle point at infinity (zero result from self-subtraction) */
		if (EC_POINT_is_at_infinity(group, ct->C1[i]))
		{
			/* Encode as all-zero block of TECC_EC_POINT_SIZE */
			memset(buf + offset, 0, TECC_EC_POINT_SIZE);
		}
		else
		{
			/* Serialize C1 (compressed point) */
			oct_len = EC_POINT_point2oct(group, ct->C1[i],
										 POINT_CONVERSION_COMPRESSED,
										 buf + offset, TECC_EC_POINT_SIZE, ctx);
			if (oct_len != TECC_EC_POINT_SIZE)
			{
				if (local_ctx) BN_CTX_free(ctx);
				return TECC_EC_ELGAMAL_ERR_CRYPTO;
			}
		}
		offset += TECC_EC_POINT_SIZE;

		/* Handle point at infinity for C2 */
		if (EC_POINT_is_at_infinity(group, ct->C2[i]))
		{
			memset(buf + offset, 0, TECC_EC_POINT_SIZE);
		}
		else
		{
			/* Serialize C2 (compressed point) */
			oct_len = EC_POINT_point2oct(group, ct->C2[i],
										 POINT_CONVERSION_COMPRESSED,
										 buf + offset, TECC_EC_POINT_SIZE, ctx);
			if (oct_len != TECC_EC_POINT_SIZE)
			{
				if (local_ctx) BN_CTX_free(ctx);
				return TECC_EC_ELGAMAL_ERR_CRYPTO;
			}
		}
		offset += TECC_EC_POINT_SIZE;
	}

	if (local_ctx) BN_CTX_free(ctx);
	return offset;  /* Return total bytes written */
}

/* ReqID:TECC4.0 - Backward-compatible wrapper allocating its own BN_CTX */
int
tecc_ec_crt_serialize(uint8_t *buf, int buf_size,
					  const TeccECCRTCiphertext *ct,
					  const EC_GROUP *group)
{
	return tecc_ec_crt_serialize_ex(buf, buf_size, ct, group, NULL);
}

int
tecc_ec_crt_deserialize_ex(TeccECCRTCiphertext *ct,
						   const uint8_t *buf, int buf_size,
						   const EC_GROUP *group, BN_CTX *ctx)
{
	int			num_splits;
	int			offset;
	int			i;
	bool		local_ctx = (ctx == NULL);
	int			min_size;

	if (ct == NULL || buf == NULL || group == NULL)
		return TECC_EC_ELGAMAL_ERR_NULL;

	if (buf_size < 1)
		return TECC_EC_ELGAMAL_ERR_SIZE;

	num_splits = (int) buf[0];
	if (num_splits <= 0 || num_splits > TECC_EC_CRT_NUM_PRIMES)
		return TECC_EC_ELGAMAL_ERR_INVALID;

	min_size = 1 + num_splits * TECC_EC_POINT_SIZE * 2;
	if (buf_size < min_size)
		return TECC_EC_ELGAMAL_ERR_SIZE;

	/* Initialize ciphertext */
	if (tecc_ec_crt_ciphertext_init(ct, group, num_splits) != 0)
		return TECC_EC_ELGAMAL_ERR_MEMORY;

	if (local_ctx)
	{
		ctx = BN_CTX_new();
		if (ctx == NULL)
		{
			tecc_ec_crt_ciphertext_free(ct, group);
			return TECC_EC_ELGAMAL_ERR_MEMORY;
		}
	}

	offset = 1;
	for (i = 0; i < num_splits; i++)
	{
		/* Check for point at infinity (all-zero block) */
		{
			int			j;
			bool		is_infinity = true;

			for (j = 0; j < TECC_EC_POINT_SIZE; j++)
			{
				if (buf[offset + j] != 0)
				{
					is_infinity = false;
					break;
				}
			}
			if (is_infinity)
			{
				if (!EC_POINT_set_to_infinity(group, ct->C1[i]))
				{
					if (local_ctx) BN_CTX_free(ctx);
					tecc_ec_crt_ciphertext_free(ct, group);
					return TECC_EC_ELGAMAL_ERR_CRYPTO;
				}
			}
			else
			{
				/* Deserialize C1 */
				if (EC_POINT_oct2point(group, ct->C1[i], buf + offset,
									   TECC_EC_POINT_SIZE, ctx) != 1)
				{
					if (local_ctx) BN_CTX_free(ctx);
					tecc_ec_crt_ciphertext_free(ct, group);
					return TECC_EC_ELGAMAL_ERR_CRYPTO;
				}
			}
		}
		offset += TECC_EC_POINT_SIZE;

		/* Deserialize C2 */
		{
			int			j;
			bool		is_infinity = true;

			for (j = 0; j < TECC_EC_POINT_SIZE; j++)
			{
				if (buf[offset + j] != 0)
				{
					is_infinity = false;
					break;
				}
			}
			if (is_infinity)
			{
				if (!EC_POINT_set_to_infinity(group, ct->C2[i]))
				{
					if (local_ctx) BN_CTX_free(ctx);
					tecc_ec_crt_ciphertext_free(ct, group);
					return TECC_EC_ELGAMAL_ERR_CRYPTO;
				}
			}
			else
			{
				if (EC_POINT_oct2point(group, ct->C2[i], buf + offset,
									   TECC_EC_POINT_SIZE, ctx) != 1)
				{
					if (local_ctx) BN_CTX_free(ctx);
					tecc_ec_crt_ciphertext_free(ct, group);
					return TECC_EC_ELGAMAL_ERR_CRYPTO;
				}
			}
		}
		offset += TECC_EC_POINT_SIZE;
	}

	if (local_ctx) BN_CTX_free(ctx);
	return TECC_EC_ELGAMAL_SUCCESS;
}

/* ReqID:TECC4.0 - Backward-compatible wrapper allocating its own BN_CTX */
int
tecc_ec_crt_deserialize(TeccECCRTCiphertext *ct,
						const uint8_t *buf, int buf_size,
						const EC_GROUP *group)
{
	return tecc_ec_crt_deserialize_ex(ct, buf, buf_size, group, NULL);
}

/* ==================== Negative Value Handling ==================== */

void
tecc_ec_adjust_negative(BIGNUM *m, const BIGNUM *modulus)
{
	BIGNUM *half;

	if (m == NULL || modulus == NULL)
		return;

	half = BN_new();
	if (half == NULL)
		return;

	BN_rshift1(half, modulus);		/* half = modulus / 2 */

	if (BN_cmp(m, half) > 0)
	{
		/* m > modulus/2 → negative: m = m - modulus */
		BN_sub(m, m, modulus);
	}

	BN_free(half);
}

/* ==================== Utility Functions ==================== */

EC_GROUP *
tecc_ec_elgamal_create_group(void)
{
	EC_GROUP *group;

	group = EC_GROUP_new_by_curve_name(TECC_EC_ELGAMAL_CURVE);
	return group;
}

int
tecc_ec_elgamal_get_point_size(void)
{
	return TECC_EC_POINT_SIZE;
}
