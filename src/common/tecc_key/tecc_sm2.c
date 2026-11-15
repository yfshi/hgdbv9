/*-------------------------------------------------------------------------
 *
 * tecc_sm2.c
 *    SM2 envelope encryption for TECC key management
 *
 * Thin adapter over the SM2 algorithm primitives in
 * src/common/tecc_algorithm/tecc_sm2_algo.c.  Keeps only the public
 * tecc_sm2_* API (declared in src/include/common/tecc_key/tecc_sm2.h)
 * and delegates the actual SM2 crypto to the dedicated algorithm module,
 * so this file carries no cryptography of its own.  The algorithm module
 * is self-implemented (OpenSSL low-level EC/BN + self-contained SM3/KDF)
 * and compatible with OpenSSL 1.0.2 / 1.1.x / 3.x (no 3.x provider API).
 *
 * The KEK is an SM2 key pair: its public key (65-byte uncompressed point)
 * encrypts the HE private key, and its private key (PKCS#8 DER) decrypts it.
 * The envelope format is byte-compatible with OpenSSL's SM2 C1C3C2 cipher.
 *
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd.
 *
 * src/common/tecc_key/tecc_sm2.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
#include "c.h"
#include "common/fe_memutils.h"

#include "common/tecc_key/tecc_sm2.h"
#include "common/tecc_algorithm/tecc_sm2_algo.h"

/* Map algorithm-module status to the tecc_sm2 public status codes. */
static int
tecc_sm2_map_status(int algo_status)
{
	switch (algo_status)
	{
		case TECC_SM2_ALGO_OK:
			return TECC_SM2_SUCCESS;
		case TECC_SM2_ALGO_ERR_NULL:
			return TECC_SM2_ERR_NULL;
		case TECC_SM2_ALGO_ERR_MEMORY:
			return TECC_SM2_ERR_MEMORY;
		case TECC_SM2_ALGO_ERR_INVALID:
			return TECC_SM2_ERR_INVALID;
		case TECC_SM2_ALGO_ERR_CRYPTO:
		default:
			return TECC_SM2_ERR_CRYPTO;
	}
}

int
tecc_sm2_envelope_encrypt(const unsigned char *pub_point, size_t pub_len,
						  const unsigned char *plain, size_t plain_len,
						  unsigned char **envelope, size_t *env_len)
{
	int			rc;

	if (pub_point == NULL || plain == NULL || envelope == NULL || env_len == NULL)
		return TECC_SM2_ERR_NULL;

	*envelope = NULL;
	*env_len = 0;

	rc = tecc_sm2_algo_encrypt(pub_point, pub_len, plain, plain_len,
							   envelope, env_len);
	return tecc_sm2_map_status(rc);
}

int
tecc_sm2_envelope_decrypt(const unsigned char *priv_der, size_t priv_len,
						  const unsigned char *envelope, size_t env_len,
						  unsigned char **plain, size_t *plain_len)
{
	int			rc;

	if (priv_der == NULL || envelope == NULL || plain == NULL || plain_len == NULL)
		return TECC_SM2_ERR_NULL;

	*plain = NULL;
	*plain_len = 0;

	rc = tecc_sm2_algo_decrypt(priv_der, priv_len, envelope, env_len,
							   plain, plain_len);
	return tecc_sm2_map_status(rc);
}

int
tecc_sm2_derive_pubkey(const unsigned char *priv_der, size_t priv_len,
					   unsigned char **pub_point, size_t *pub_len)
{
	int			rc;

	if (priv_der == NULL || priv_len == 0 || pub_point == NULL || pub_len == NULL)
		return TECC_SM2_ERR_NULL;

	*pub_point = NULL;
	*pub_len = 0;

	rc = tecc_sm2_algo_derive_pubkey(priv_der, priv_len, pub_point, pub_len);
	return tecc_sm2_map_status(rc);
}

/* ==================== ReqID:TECC4.0 - 分包信封容器 ==================== */

static const char tecc_hex_digits[] = "0123456789abcdef";

/* 二进制转小写 hex（malloc 分配，含结尾 '\0'） */
static char *
tecc_sm2_hex_encode(const unsigned char *bin, size_t bin_len)
{
	char	   *hex;
	size_t		i;

	hex = (char *) malloc(bin_len * 2 + 1);
	if (hex == NULL)
		return NULL;

	for (i = 0; i < bin_len; i++)
	{
		hex[i * 2] = tecc_hex_digits[(bin[i] >> 4) & 0x0F];
		hex[i * 2 + 1] = tecc_hex_digits[bin[i] & 0x0F];
	}
	hex[bin_len * 2] = '\0';
	return hex;
}

int
tecc_sm2_env_is_chunked(const char *env_hex)
{
	if (env_hex == NULL)
		return 0;
	return strncmp(env_hex, TECC_HE_ENV_CHUNK_PREFIX,
				   strlen(TECC_HE_ENV_CHUNK_PREFIX)) == 0 ? 1 : 0;
}

int
tecc_sm2_env_chunk_count(const char *env_hex)
{
	const char *p;
	int			n = 0;

	if (env_hex == NULL || env_hex[0] == '\0')
		return 0;

	if (!tecc_sm2_env_is_chunked(env_hex))
		return 1;

	/* 非空块计一次；遇到空块（连续 ':'）视为非法 */
	p = env_hex + strlen(TECC_HE_ENV_CHUNK_PREFIX);
	for (;;)
	{
		const char *sep = strchr(p, ':');

		if (sep == NULL)
		{
			if (*p == '\0')
				return 0;
			return n + 1;
		}
		if (sep == p)
			return 0;			/* 空块 */
		n++;
		p = sep + 1;
	}
}

int
tecc_sm2_env_chunk_hex(const char *env_hex, int idx, char **out_hex)
{
	const char *start;
	const char *end;
	int			cur = 0;

	if (out_hex == NULL)
		return TECC_SM2_ERR_NULL;
	*out_hex = NULL;

	if (env_hex == NULL || idx < 0)
		return TECC_SM2_ERR_INVALID;

	if (!tecc_sm2_env_is_chunked(env_hex))
	{
		char	   *copy;

		if (idx != 0)
			return TECC_SM2_ERR_INVALID;
		copy = (char *) malloc(strlen(env_hex) + 1);
		if (copy == NULL)
			return TECC_SM2_ERR_MEMORY;
		strcpy(copy, env_hex);
		*out_hex = copy;
		return TECC_SM2_SUCCESS;
	}

	start = env_hex + strlen(TECC_HE_ENV_CHUNK_PREFIX);
	for (;;)
	{
		size_t		len;
		char	   *copy;

		end = strchr(start, ':');
		len = (end != NULL) ? (size_t) (end - start) : strlen(start);
		if (len == 0)
			return TECC_SM2_ERR_INVALID;
		if (cur == idx)
		{
			copy = (char *) malloc(len + 1);
			if (copy == NULL)
				return TECC_SM2_ERR_MEMORY;
			memcpy(copy, start, len);
			copy[len] = '\0';
			*out_hex = copy;
			return TECC_SM2_SUCCESS;
		}
		if (end == NULL)
			break;
		cur++;
		start = end + 1;
	}
	return TECC_SM2_ERR_INVALID;
}

int
tecc_sm2_envelope_encrypt_hex(const unsigned char *pub_point, size_t pub_len,
							  const unsigned char *plain, size_t plain_len,
							  size_t max_plain_per_chunk, char **out_hex)
{
	/* 单块信封的 hex 缓冲：块明文 128B 时信封约 235B，双倍余量足够 */
	char	   *hex = NULL;
	unsigned char *env = NULL;
	size_t		env_len = 0;
	size_t		off;
	int			rc;

	if (out_hex == NULL)
		return TECC_SM2_ERR_NULL;
	*out_hex = NULL;
	if (pub_point == NULL || plain == NULL || plain_len == 0)
		return TECC_SM2_ERR_NULL;

	if (max_plain_per_chunk == 0 || plain_len <= max_plain_per_chunk)
	{
		/* 单信封（与历史行为一致，不分块） */
		rc = tecc_sm2_envelope_encrypt(pub_point, pub_len, plain, plain_len,
									   &env, &env_len);
		if (rc != TECC_SM2_SUCCESS)
			return rc;
		hex = tecc_sm2_hex_encode(env, env_len);
		OPENSSL_free(env);
		if (hex == NULL)
			return TECC_SM2_ERR_MEMORY;
		*out_hex = hex;
		return TECC_SM2_SUCCESS;
	}

	/* 分块容器：'v2:' 前缀 + 各块信封 hex，以 ':' 串联 */
	{
		size_t		nchunks = (plain_len + max_plain_per_chunk - 1) / max_plain_per_chunk;
		size_t		hex_cap = strlen(TECC_HE_ENV_CHUNK_PREFIX) + 1;
		size_t		i;

		/* 逐块先算长度：信封 ≤ 明文 + 约 120B 头（x/y/M/DER 包装） */
		hex_cap += nchunks * ((max_plain_per_chunk + 160) * 2 + 1);

		hex = (char *) malloc(hex_cap);
		if (hex == NULL)
			return TECC_SM2_ERR_MEMORY;
		strcpy(hex, TECC_HE_ENV_CHUNK_PREFIX);
		off = strlen(hex);

		for (i = 0; i < nchunks; i++)
		{
			size_t		this_len = plain_len - i * max_plain_per_chunk;
			char	   *chunk_hex;

			if (this_len > max_plain_per_chunk)
				this_len = max_plain_per_chunk;

			rc = tecc_sm2_envelope_encrypt(pub_point, pub_len,
										   plain + i * max_plain_per_chunk, this_len,
										   &env, &env_len);
			if (rc != TECC_SM2_SUCCESS)
			{
				free(hex);
				return rc;
			}
			chunk_hex = tecc_sm2_hex_encode(env, env_len);
			OPENSSL_free(env);
			env = NULL;
			if (chunk_hex == NULL)
			{
				free(hex);
				return TECC_SM2_ERR_MEMORY;
			}
			if (i > 0)
				hex[off++] = ':';
			memcpy(hex + off, chunk_hex, strlen(chunk_hex));
			off += strlen(chunk_hex);
			hex[off] = '\0';
			free(chunk_hex);
		}
	}

	*out_hex = hex;
	return TECC_SM2_SUCCESS;
}
