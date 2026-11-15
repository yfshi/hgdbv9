/*-------------------------------------------------------------------------
 *
 * tecc_sm2_algo.c
 *	  SM2 envelope crypto primitives (cross-version, no OpenSSL 3.x dependency)
 *
 * Self-contained SM2 public-key encryption / decryption (GM/T 0003.4-2012,
 * GM/T 0003.5-2012), byte-compatible with OpenSSL's ossl_sm2_encrypt /
 * ossl_sm2_decrypt ASN.1 DER envelope.  EC arithmetic uses OpenSSL's
 * low-level EC_KEY / EC_GROUP / EC_POINT / BN primitives, available in
 * OpenSSL 1.0.2, 1.1.x and 3.x; no OpenSSL 3.x provider API is used.
 * SM3 + the SM2 KDF are self-implemented via tecc_sm3_internal (pure C).
 *
 * The private key container (ECPrivateKey / PKCS#8 DER) is also parsed by
 * hand -- see sm2_privkey_parse_der -- so nothing here goes through the
 * EVP_PKEY layer, whose legacy EC_KEY bridge is unusable for SM2 keys on
 * OpenSSL 3.0.
 *
 * The sm2p256v1 curve is constructed explicitly (explicit field params +
 * generator + cofactor), because NID_sm2 is absent in OpenSSL 1.0.2.
 *
 * References:
 *   - OpenSSL crypto/sm2/sm2_crypt.c (ossl_sm2_encrypt / ossl_sm2_decrypt),
 *     for the exact envelope format and hash/KDF ordering we must remain
 *     byte-compatible with.  Local copy: repro/sm2_crypt_ref.c.
 *   - GM/T 0003.3-2012 (SM2 KDF), GM/T 0003.4-2012 (encrypt), GM/T 0003.5
 *     (dense ciphertext format / SM2_Ciphertext ASN.1).
 *
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd.
 *
 * src/common/tecc_algorithm/tecc_sm2_algo.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
#include "c.h"
#include "common/fe_memutils.h"

#include <string.h>
#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/err.h>
#include <openssl/rand.h>

#include "common/tecc_algorithm/tecc_sm2_algo.h"
#include "common/tecc_algorithm/tecc_sm3_internal.h"

/* ==================== sm2p256v1 explicit curve params ====================
 * Verified against `openssl ecparam -name SM2 -param_enc explicit -text`.
 */
#define SM2_P  \
	"FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFF"
#define SM2_A  \
	"FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFC"
#define SM2_B  \
	"28E9FA9E9D9F5E344D5A9E4BCF6509A7F39789F515AB8F92DDBCBD414D940E93"
#define SM2_N  \
	"FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFF7203DF6B21C6052B53BBF40939D54123"
#define SM2_GX \
	"32C4AE2C1F1981195F9904466A39C9948FE30BBFF2660BE1715A4589334C74C7"
#define SM2_GY \
	"BC3736A2F4F6779C59BDCEE36B692153D0A9877CC62A474002DF32E52139F0A0"

#define SM2_FIELD_BYTES  32			/* 256-bit field size in bytes */

/* ==================== minimal DER helpers ====================
 * Only what the SM2_Ciphertext envelope needs:
 *   SEQUENCE { INTEGER C1x, INTEGER C1y, OCTET STRING C3, OCTET STRING C2 }
 * DER INTEGER: big-endian magnitude; if high bit set, prepend 0x00.
 */

/* Encode a length; returns byte count. buf must hold up to 5 bytes. */
static int
der_len_encode(size_t n, unsigned char *buf)
{
	if (n < 0x80)
	{
		buf[0] = (unsigned char) n;
		return 1;
	}
	else if (n <= 0xff)
	{
		buf[0] = 0x81;
		buf[1] = (unsigned char) n;
		return 2;
	}
	else if (n <= 0xffff)
	{
		buf[0] = 0x82;
		buf[1] = (unsigned char) (n >> 8);
		buf[2] = (unsigned char) n;
		return 3;
	}
	else if (n <= 0xffffff)
	{
		buf[0] = 0x83;
		buf[1] = (unsigned char) (n >> 16);
		buf[2] = (unsigned char) (n >> 8);
		buf[3] = (unsigned char) n;
		return 4;
	}
	else
	{
		buf[0] = 0x84;
		buf[1] = (unsigned char) (n >> 24);
		buf[2] = (unsigned char) (n >> 16);
		buf[3] = (unsigned char) (n >> 8);
		buf[4] = (unsigned char) n;
		return 5;
	}
}

/*
 * DER-encode a positive BIGNUM as a DER INTEGER.  Writes tag/length into
 * tl (needs 3 bytes) and value into val (needs BN_num_bytes+1).  Returns
 * the full encoded length (tl + value), or -1 on error.
 */
static int
der_encode_integer(const BIGNUM *bn, unsigned char *tl, unsigned char *val)
{
	int			nbytes;
	int			lead;

	if (bn == NULL)
		return -1;
	nbytes = BN_num_bytes(bn);
	if (nbytes < 0 || nbytes > 32)
		return -1;
	/* BN_bn2bin writes exactly BN_num_bytes(bn) big-endian bytes into val
	 * (which is sized to nbytes), avoiding BN_bn2binpad (not in 1.0.2). */
	if (nbytes > 0 && BN_bn2bin(bn, val) != nbytes)
		return -1;
	lead = 0;
	if ((val[0] & 0x80) != 0)
	{
		memmove(val + 1, val, nbytes);
		val[0] = 0x00;
		lead = 1;
	}
	tl[0] = 0x02;
	tl[1] = (unsigned char) (nbytes + lead);
	return nbytes + lead + 2;
}

/* ==================== SM2 private key container parsing ====================
 * ReqID:TECC4.0 - 自解析 SM2 私钥容器，不使用 d2i_PrivateKey() +
 * EVP_PKEY_get1_EC_KEY()。
 *
 * 原因（2026-09-28 实测）：OpenSSL 3.0 把 SM2 私钥解码成 provider 托管对象
 * （其 EVP_PKEY 类型为 EVP_PKEY_KEYMGMT，即 -1，而不是 EVP_PKEY_EC=408），
 * 而 deprecated 的 legacy 桥 EVP_PKEY_get1_EC_KEY() 对 provider 托管对象一律
 * 返回 NULL —— 对任意 SM2 私钥、两种容器形态都一样（1.0.2k / 1.1.1f 下同一
 * 调用正常）。于是 OpenSSL 3.0 构建的 libpq 永远解封不了 HE 的 DEK 私钥，
 * 同态列（he-add / he-mul）查询静默回落到返回密文。
 *
 * 这里只从 DER 里取私钥标量；曲线恒为 sm2p256v1，由本文件的常量重建
 * （见 sm2_build_group），所以不再依赖任何 OpenSSL 版本对曲线的命名/编码。
 * 两种实际出现过的容器都支持：
 *
 *   ECPrivateKey (SEC1/RFC 5915，tecc_ktool 写出的形态)：
 *     SEQUENCE { INTEGER 1, OCTET STRING privateKey,
 *                [0] parameters OPTIONAL, [1] publicKey OPTIONAL }
 *
 *   PrivateKeyInfo (PKCS#8)：其 privateKey OCTET STRING 内包上面这个结构。
 */

/* 定义在文件后部，解析器先用到 */
static int	der_parse_element(const unsigned char **p, const unsigned char *end,
							  int *tag, const unsigned char **content,
							  size_t *content_len);

/*
 * sm2_privkey_parse_der - 从 DER 私钥里定位私钥标量与可选公钥点。
 *
 * 返回的指针指向 priv_der 内部，调用方使用期间 priv_der 必须保持有效。
 * 成功返回 1；容器不是可识别的 SM2 私钥时返回 0。
 */
static int
sm2_privkey_parse_der(const unsigned char *priv_der, size_t priv_len,
					  const unsigned char **scalar, size_t *scalar_len,
					  const unsigned char **pub_oct, size_t *pub_oct_len)
{
	const unsigned char *p;
	const unsigned char *end;
	const unsigned char *content;
	size_t		clen;
	int			tag;

	if (priv_der == NULL || priv_len == 0 ||
		scalar == NULL || scalar_len == NULL ||
		pub_oct == NULL || pub_oct_len == NULL)
		return 0;
	*scalar = NULL;
	*scalar_len = 0;
	*pub_oct = NULL;
	*pub_oct_len = 0;

	p = priv_der;
	end = priv_der + priv_len;

	/* 外层 SEQUENCE */
	if (!der_parse_element(&p, end, &tag, &content, &clen) || tag != 0x30)
		return 0;
	p = content;
	end = content + clen;

	for (;;)
	{
		/* 版本 INTEGER：PKCS#8 为 0，ECPrivateKey 为 1 */
		if (!der_parse_element(&p, end, &tag, &content, &clen) || tag != 0x02)
			return 0;

		if (!der_parse_element(&p, end, &tag, &content, &clen))
			return 0;

		if (tag == 0x30)
		{
			/* PKCS#8：AlgorithmIdentifier 之后是 privateKey OCTET STRING，
			 * 其内容又是完整的 ECPrivateKey SEQUENCE，故要剥两层。 */
			if (!der_parse_element(&p, end, &tag, &content, &clen) || tag != 0x04)
				return 0;
			p = content;
			end = content + clen;
			if (!der_parse_element(&p, end, &tag, &content, &clen) || tag != 0x30)
				return 0;
			p = content;
			end = content + clen;
			continue;
		}
		if (tag != 0x04 || clen == 0)
			return 0;

		*scalar = content;
		*scalar_len = clen;
		break;
	}

	/* 可选 [0] parameters（忽略：曲线固定）与 [1] publicKey
	 * （BIT STRING，首字节为未用位数）。 */
	while (p < end)
	{
		const unsigned char *q;
		const unsigned char *qend;

		if (!der_parse_element(&p, end, &tag, &content, &clen))
			break;
		if (tag != 0xA1)
			continue;

		q = content;
		qend = content + clen;
		if (der_parse_element(&q, qend, &tag, &content, &clen) &&
			tag == 0x03 && clen > 1 && content[0] == 0x00)
		{
			*pub_oct = content + 1;
			*pub_oct_len = clen - 1;
		}
	}
	return 1;
}

/* ==================== sm2p256v1 EC_GROUP construction ==================== */

/*
 * Build a new EC_GROUP for sm2p256v1 using explicit parameters so NID_sm2
 * (absent in OpenSSL 1.0.2) is never required.  Caller frees it.
 */
static EC_GROUP *
sm2_build_group(void)
{
	BN_CTX    *ctx = NULL;
	BIGNUM    *p = NULL;
	BIGNUM    *a = NULL;
	BIGNUM    *b = NULL;
	BIGNUM    *n = NULL;
	BIGNUM    *gx = NULL;
	BIGNUM    *gy = NULL;
	BIGNUM    *cofactor = NULL;
	EC_GROUP  *group = NULL;
	EC_POINT  *g = NULL;
	int			ok = 0;

	ctx = BN_CTX_new();
	if (ctx == NULL)
		return NULL;
	BN_CTX_start(ctx);
	p = BN_CTX_get(ctx);
	a = BN_CTX_get(ctx);
	b = BN_CTX_get(ctx);
	n = BN_CTX_get(ctx);
	gx = BN_CTX_get(ctx);
	gy = BN_CTX_get(ctx);
	cofactor = BN_CTX_get(ctx);

	if (cofactor == NULL ||
		!BN_hex2bn(&p, SM2_P) ||
		!BN_hex2bn(&a, SM2_A) ||
		!BN_hex2bn(&b, SM2_B) ||
		!BN_hex2bn(&n, SM2_N) ||
		!BN_hex2bn(&gx, SM2_GX) ||
		!BN_hex2bn(&gy, SM2_GY) ||
		!BN_set_word(cofactor, 1))
		goto done;

	group = EC_GROUP_new_curve_GFp(p, a, b, ctx);
	if (group == NULL)
		goto done;
	g = EC_POINT_new(group);
	if (g == NULL)
		goto done;
	if (!EC_POINT_set_affine_coordinates_GFp(group, g, gx, gy, ctx))
		goto done;
	if (!EC_GROUP_set_generator(group, g, n, cofactor))
		goto done;
	ok = 1;

done:
	EC_POINT_free(g);
	if (!ok && group != NULL)
	{
		EC_GROUP_free(group);
		group = NULL;
	}
	BN_CTX_end(ctx);
	BN_CTX_free(ctx);
	return group;
}

/* ==================== key construction ==================== */

EC_KEY *
tecc_sm2_algo_pubkey_from_point(const unsigned char *pub_point, size_t pub_len)
{
	EC_GROUP  *group = NULL;
	EC_KEY    *ec = NULL;
	EC_POINT  *pt = NULL;
	BN_CTX    *ctx = NULL;
	int			ok = 0;

	if (pub_point == NULL || pub_len != TECC_SM2_POINT_UNCOMP ||
		pub_point[0] != 0x04)
		return NULL;

	group = sm2_build_group();
	if (group == NULL)
		return NULL;
	ctx = BN_CTX_new();
	if (ctx == NULL)
	{
		EC_GROUP_free(group);
		return NULL;
	}
	ec = EC_KEY_new();
	if (ec == NULL)
		goto done;
	if (EC_KEY_set_group(ec, group) != 1)
		goto done;
	pt = EC_POINT_new(group);
	if (pt == NULL)
		goto done;
	if (EC_POINT_oct2point(group, pt, pub_point, pub_len, ctx) != 1)
		goto done;
	if (EC_KEY_set_public_key(ec, pt) != 1)
		goto done;
	ok = 1;

done:
	EC_POINT_free(pt);
	BN_CTX_free(ctx);
	EC_GROUP_free(group);
	if (!ok && ec != NULL)
	{
		EC_KEY_free(ec);
		ec = NULL;
	}
	return ec;
}

EC_KEY *
tecc_sm2_algo_privkey_from_der(const unsigned char *priv_der, size_t priv_len)
{
	const unsigned char *scalar = NULL;
	const unsigned char *pub_oct = NULL;
	size_t		scalar_len = 0;
	size_t		pub_oct_len = 0;
	EC_GROUP  *group = NULL;
	EC_KEY    *ec = NULL;
	EC_POINT  *pub = NULL;
	BIGNUM    *d = NULL;
	BIGNUM    *order = NULL;
	BN_CTX    *ctx = NULL;
	int			ok = 0;

	if (priv_der == NULL || priv_len == 0)
		return NULL;

	/* ReqID:TECC4.0 - 自解析容器取标量，替代 d2i_PrivateKey() +
	 * EVP_PKEY_get1_EC_KEY()（后者在 OpenSSL 3.0 下对 SM2 私钥恒返回 NULL，
	 * 详见 sm2_privkey_parse_der 的说明）。 */
	if (!sm2_privkey_parse_der(priv_der, priv_len, &scalar, &scalar_len,
							   &pub_oct, &pub_oct_len))
		return NULL;

	group = sm2_build_group();
	if (group == NULL)
		return NULL;
	ctx = BN_CTX_new();
	if (ctx == NULL)
	{
		EC_GROUP_free(group);
		return NULL;
	}

	BN_CTX_start(ctx);
	order = BN_CTX_get(ctx);
	d = BN_bin2bn(scalar, (int) scalar_len, NULL);
	if (d == NULL || order == NULL)
		goto done;

	/* 标量须落在 [1, n-1] */
	if (EC_GROUP_get_order(group, order, ctx) != 1 ||
		BN_is_zero(d) || BN_cmp(d, order) >= 0)
		goto done;

	ec = EC_KEY_new();
	if (ec == NULL)
		goto done;
	if (EC_KEY_set_group(ec, group) != 1)
		goto done;
	if (EC_KEY_set_private_key(ec, d) != 1)
		goto done;

	/* 公钥：优先取 DER 里的 [1] publicKey；缺失时由 d*G 现场推导
	 * （tecc_sm2_algo_derive_pubkey 依赖它非空）。 */
	pub = EC_POINT_new(group);
	if (pub == NULL)
		goto done;
	if (pub_oct != NULL && pub_oct_len > 0)
	{
		if (EC_POINT_oct2point(group, pub, pub_oct, pub_oct_len, ctx) != 1)
			goto done;
	}
	else if (EC_POINT_mul(group, pub, d, NULL, NULL, ctx) != 1)
		goto done;
	if (EC_KEY_set_public_key(ec, pub) != 1)
		goto done;

	/* 校验公钥在曲线上、且等于 d*G */
	if (EC_KEY_check_key(ec) != 1)
		goto done;
	ok = 1;

done:
	EC_POINT_free(pub);
	if (d != NULL)
		BN_free(d);
	BN_CTX_end(ctx);
	BN_CTX_free(ctx);
	EC_GROUP_free(group);
	if (!ok && ec != NULL)
	{
		EC_KEY_free(ec);
		ec = NULL;
	}
	return ec;
}

/*
 * bn_to_padded - write a BIGNUM into a fixed-size, zero-padded big-endian
 * buffer.  Uses only BN_bn2bin (available in all OpenSSL versions; avoids
 * BN_bn2binpad which is not present in OpenSSL 1.0.2).  Returns 1 on
 * success (bn fits in size), 0 if the number needs more than size bytes.
 */
static int
bn_to_padded(const BIGNUM *bn, unsigned char *buf, size_t size)
{
	int			nbytes = BN_num_bytes(bn);
	int			off;

	if (bn == NULL || nbytes < 0 || (size_t) nbytes > size)
		return 0;
	memset(buf, 0, size);
	off = (int) size - nbytes;
	if (nbytes > 0 && BN_bn2bin(bn, buf + off) != nbytes)
		return 0;
	return 1;
}

/* ==================== SM2 KDF (X9.63, no shared-info, SM3) ====================
 * KDF(Z, klen) = concat( SM3(Z || ctr_be32) for ctr = 1, 2, ... ), truncated
 * to klen.  Here Z = x2 || y2 (2 * 32 bytes).  Exactly matches
 * OpenSSL's ossl_ecdh_kdf_X9_63 as used by ossl_sm2_encrypt.  Returns 1 on
 * success, 0 on failure.
 */
static int
sm2_kdf(const unsigned char *z, size_t zlen,
		unsigned char *out, size_t outlen)
{
	unsigned char block[SM2_FIELD_BYTES * 2 + 4];	/* Z || ctr(4) */
	unsigned char dg[TECC_SM3_DIGEST_LENGTH];
	size_t		got = 0;
	uint32_t	ct = 1;

	if (z == NULL || out == NULL || zlen > SM2_FIELD_BYTES * 2)
		return 0;

	if (outlen == 0)
		return 1;

	memcpy(block, z, zlen);

	while (got < outlen)
	{
		block[zlen + 0] = (unsigned char) (ct >> 24);
		block[zlen + 1] = (unsigned char) (ct >> 16);
		block[zlen + 2] = (unsigned char) (ct >> 8);
		block[zlen + 3] = (unsigned char) ct;

		if (tecc_sm3_hash(block, zlen + 4, dg) != 1)
			return 0;

		memcpy(out + got, dg, Min(TECC_SM3_DIGEST_LENGTH, outlen - got));
		got += Min(TECC_SM3_DIGEST_LENGTH, outlen - got);
		ct++;
		if (ct == 0)
			return 0;			/* requested output exceeds KDF capacity */
	}
	return 1;
}

static int
sm2_is_all_zero(const unsigned char *buf, size_t len)
{
	size_t	i;

	for (i = 0; i < len; i++)
	{
		if (buf[i] != 0)
			return 0;
	}
	return 1;
}

/* ==================== SM2 encrypt ==================== */
int
tecc_sm2_algo_encrypt(const unsigned char *pub_point, size_t pub_len,
					  const unsigned char *plain, size_t plain_len,
					  unsigned char **envelope, size_t *env_len)
{
	EC_KEY    *pub_ec = NULL;
	const EC_GROUP *group;
	BN_CTX    *ctx = NULL;
	BIGNUM    *k = NULL;
	BIGNUM    *x1 = NULL;
	BIGNUM    *y1 = NULL;
	BIGNUM    *x2 = NULL;
	BIGNUM    *y2 = NULL;
	BIGNUM    *order = NULL;
	EC_POINT  *kG = NULL;
	EC_POINT  *kP = NULL;
	const EC_POINT *P;
	unsigned char *x2y2 = NULL;
	unsigned char *mask = NULL;
	unsigned char *hash_buf = NULL; /* x2||M||y2 */
	unsigned char c3[TECC_SM2_C3_LEN];
	unsigned char *der = NULL;
	unsigned char *out = NULL;
	size_t		der_len = 0;
	int			rc = TECC_SM2_ALGO_ERR_CRYPTO;

	if (pub_point == NULL || plain == NULL || envelope == NULL || env_len == NULL)
		return TECC_SM2_ALGO_ERR_NULL;
	*envelope = NULL;
	*env_len = 0;

	pub_ec = tecc_sm2_algo_pubkey_from_point(pub_point, pub_len);
	if (pub_ec == NULL)
		return TECC_SM2_ALGO_ERR_INVALID;
	group = EC_KEY_get0_group(pub_ec);
	P = EC_KEY_get0_public_key(pub_ec);

	ctx = BN_CTX_new();
	if (ctx == NULL)
		goto done;
	BN_CTX_start(ctx);
	k = BN_CTX_get(ctx);
	x1 = BN_CTX_get(ctx);
	y1 = BN_CTX_get(ctx);
	x2 = BN_CTX_get(ctx);
	y2 = BN_CTX_get(ctx);
	order = BN_CTX_get(ctx);
	if (y2 == NULL || order == NULL)
		goto done;
	/* ReqID:TECC4.0 - OpenSSL 1.0.2 兼容：EC_GROUP_get0_order 需 1.1+，
	 * 改用 EC_GROUP_get_order(group, order, ctx)（1.0.2 即有）。需在
	 * BN_CTX 创建后调用，故这里才取 order。 */
	if (EC_GROUP_get_order(group, order, ctx) != 1)
		goto done;

	kG = EC_POINT_new(group);
	kP = EC_POINT_new(group);
	if (kG == NULL || kP == NULL)
		goto done;

	x2y2 = (unsigned char *) palloc(SM2_FIELD_BYTES * 2);
	mask = (unsigned char *) palloc(plain_len ? plain_len : 1);
	hash_buf = (unsigned char *) palloc(SM2_FIELD_BYTES + plain_len + SM2_FIELD_BYTES);

again:
	/* random k in [1, n-1] (BN_rand_range returns [0,n-1]) */
	if (BN_rand_range(k, order) != 1)
		goto done;
	if (BN_is_zero(k))
		goto again;

	/* C1 = kG ; shared point kP ; coordinates x2,y2 */
	if (EC_POINT_mul(group, kG, k, NULL, NULL, ctx) != 1 ||
		EC_POINT_get_affine_coordinates_GFp(group, kG, x1, y1, ctx) != 1 ||
		EC_POINT_mul(group, kP, NULL, P, k, ctx) != 1 ||
		EC_POINT_get_affine_coordinates_GFp(group, kP, x2, y2, ctx) != 1)
		goto done;

	/* Z = x2 || y2, big-endian padded to 32 bytes each */
	if (!bn_to_padded(x2, x2y2, SM2_FIELD_BYTES) ||
		!bn_to_padded(y2, x2y2 + SM2_FIELD_BYTES, SM2_FIELD_BYTES))
		goto done;

	/* KDF -> mask ; re-roll k if mask is all zero (GM/T 0003.4) */
	if (sm2_kdf(x2y2, SM2_FIELD_BYTES * 2, mask, plain_len) != 1)
		goto done;
	if (sm2_is_all_zero(mask, plain_len))
		goto again;

	/* C3 = SM3(x2 || M || y2) */
	memcpy(hash_buf, x2y2, SM2_FIELD_BYTES);
	memcpy(hash_buf + SM2_FIELD_BYTES, plain, plain_len);
	memcpy(hash_buf + SM2_FIELD_BYTES + plain_len, x2y2 + SM2_FIELD_BYTES,
		   SM2_FIELD_BYTES);
	if (tecc_sm3_hash(hash_buf, SM2_FIELD_BYTES + plain_len + SM2_FIELD_BYTES,
					  c3) != 1)
		goto done;

	/* Encode SEQUENCE envelope */
	{
		unsigned char tlx[3];
		unsigned char tly[3];
		unsigned char px[33];
		unsigned char py[33];
		unsigned char len_c3[5];
		unsigned char len_c2[5];
		int			c1x_e, c1y_e;
		size_t		content_len;
		size_t		lenbuf[5];
		int			lenbuf_n;
		size_t		pos = 0;
		size_t		i;

		c1x_e = der_encode_integer(x1, tlx, px);
		c1y_e = der_encode_integer(y1, tly, py);
		if (c1x_e < 0 || c1y_e < 0)
			goto done;

		/* OCTET STRING length-field sizes.  DER length encoding:
		 *   C3 len is always 32 (< 128)  -> 1 length byte.
		 *   C2 len == plaintext length    -> may exceed 127 bytes when the
		 *   plaintext is e.g. an FF-ElGamal (2048-bit) private key, in which
		 *   case the length field is 0x81 <len> (2 bytes).  Use the generic
		 *   length encoder so the envelope stays valid DER for any size. */
		{
			size_t		c3_hdr = 1 + der_len_encode(TECC_SM2_C3_LEN, len_c3);
			size_t		c2_hdr = 1 + der_len_encode(plain_len, len_c2);

			/* content = C1x + C1y + C3(HDR|data) + C2(HDR|data) */
			content_len = (size_t) c1x_e + (size_t) c1y_e
				+ (c3_hdr + TECC_SM2_C3_LEN)
				+ (c2_hdr + plain_len);

			lenbuf_n = der_len_encode(content_len, (unsigned char *) lenbuf);
			der_len = 1 + (size_t) lenbuf_n + content_len;
			der = (unsigned char *) palloc(der_len);

			der[pos++] = 0x30;	/* SEQUENCE */
			memcpy(der + pos, lenbuf, lenbuf_n);
			pos += lenbuf_n;

			/* C1x INTEGER */
			memcpy(der + pos, tlx, 2);
			memcpy(der + pos + 2, px, (size_t) c1x_e - 2);
			pos += (size_t) c1x_e;
			/* C1y INTEGER */
			memcpy(der + pos, tly, 2);
			memcpy(der + pos + 2, py, (size_t) c1y_e - 2);
			pos += (size_t) c1y_e;
			/* C3 */
			der[pos++] = 0x04;
			pos += der_len_encode(TECC_SM2_C3_LEN, der + pos);
			memcpy(der + pos, c3, TECC_SM2_C3_LEN);
			pos += TECC_SM2_C3_LEN;
			/* C2 = M XOR mask */
			der[pos++] = 0x04;
			pos += der_len_encode(plain_len, der + pos);
			for (i = 0; i < plain_len; i++)
				der[pos + i] = mask[i] ^ plain[i];
			pos += plain_len;
			Assert(pos == der_len);
		}
	}

	/* Return via OPENSSL_malloc to match current callee expectations. */
	out = OPENSSL_malloc(der_len);
	if (out == NULL)
	{
		rc = TECC_SM2_ALGO_ERR_MEMORY;
		goto done;
	}
	memcpy(out, der, der_len);
	*envelope = out;
	*env_len = der_len;
	rc = TECC_SM2_ALGO_OK;

done:
	if (rc != TECC_SM2_ALGO_OK && *envelope != NULL)
	{
		OPENSSL_free(*envelope);
		*envelope = NULL;
		*env_len = 0;
	}
	if (x2y2) pfree(x2y2);
	if (mask) pfree(mask);
	if (hash_buf) pfree(hash_buf);
	if (der) pfree(der);
	EC_POINT_free(kG);
	EC_POINT_free(kP);
	if (ctx != NULL)
	{
		BN_CTX_end(ctx);
		BN_CTX_free(ctx);
	}
	EC_KEY_free(pub_ec);
	return rc;
}

/* ==================== DER parser (defensive) ==================== */

/*
 * Parse a single DER element starting at *p.  On success sets *content to
 * point at content, *content_len to its length, and advances *p to just past
 * the element.  Returns 1 on success, 0 on error.  *end is the buffer limit.
 */
static int
der_parse_element(const unsigned char **p, const unsigned char *end,
				  int *tag, const unsigned char **content, size_t *content_len)
{
	const unsigned char *q = *p;
	size_t		len;
	size_t		nbytes;
	size_t		i;

	if (q >= end)
		return 0;
	*tag = *q++;
	/* minimal: only definite long/short forms; refuse indefinite (0x80) */
	if (q >= end)
		return 0;
	if (*q == 0x80)
		return 0;
	if ((*q & 0x80) == 0)
	{
		len = *q++;
	}
	else
	{
		nbytes = *q & 0x7f;
		if (nbytes == 0 || nbytes > 4)
			return 0;
		q++;
		len = 0;
		for (i = 0; i < nbytes; i++)
		{
			if (q >= end)
				return 0;
			len = (len << 8) | *q++;
		}
	}
	if (len > (size_t) (end - q))
		return 0;
	*content = q;
	*content_len = len;
	*p = q + len;
	return 1;
}

/* ==================== SM2 decrypt ==================== */
int
tecc_sm2_algo_decrypt(const unsigned char *priv_der, size_t priv_len,
					  const unsigned char *envelope, size_t env_len,
					  unsigned char **plain, size_t *plain_len)
{
	EC_KEY    *priv_ec = NULL;
	const EC_GROUP *group;
	const BIGNUM *priv_bn;
	BN_CTX    *ctx = NULL;
	BIGNUM    *x2 = NULL;
	BIGNUM    *y2 = NULL;
	BIGNUM    *c1x = NULL;
	BIGNUM    *c1y = NULL;
	EC_POINT  *C1 = NULL;
	EC_POINT  *kC1 = NULL;
	const unsigned char *p;
	const unsigned char *end;
	const unsigned char *content;
	int			tag;
	size_t		clen;
	const unsigned char *c1x_c, *c1y_c, *c3_c, *c2_c;
	size_t		c1x_n, c1y_n, c3_n, c2_n;
	unsigned char *x2y2 = NULL;
	unsigned char *mask = NULL;
	unsigned char *hash_buf = NULL;
	unsigned char *pt = NULL;
	size_t		pt_len = 0;
	unsigned char c3_calc[TECC_SM2_C3_LEN];
	size_t		i;
	int			rc = TECC_SM2_ALGO_ERR_CRYPTO;

	if (priv_der == NULL || envelope == NULL || plain == NULL || plain_len == NULL)
		return TECC_SM2_ALGO_ERR_NULL;
	*plain = NULL;
	*plain_len = 0;

	priv_ec = tecc_sm2_algo_privkey_from_der(priv_der, priv_len);
	if (priv_ec == NULL)
		return TECC_SM2_ALGO_ERR_INVALID;
	group = EC_KEY_get0_group(priv_ec);
	priv_bn = EC_KEY_get0_private_key(priv_ec);
	if (priv_bn == NULL)
		goto done;

	/* --- parse SEQUENCE envelope --- */
	p = envelope;
	end = envelope + env_len;
	if (!der_parse_element(&p, end, &tag, &content, &clen) || tag != 0x30)
		goto done;
	end = content + clen;		/* sequence content limit */
	p = content;

	if (!der_parse_element(&p, end, &tag, &c1x_c, &c1x_n) || tag != 0x02)
		goto done;
	if (!der_parse_element(&p, end, &tag, &c1y_c, &c1y_n) || tag != 0x02)
		goto done;
	if (!der_parse_element(&p, end, &tag, &c3_c, &c3_n) || tag != 0x04)
		goto done;
	if (!der_parse_element(&p, end, &tag, &c2_c, &c2_n) || tag != 0x04)
		goto done;
	if (c3_n != TECC_SM2_C3_LEN)
		goto done;

	ctx = BN_CTX_new();
	if (ctx == NULL)
		goto done;
	BN_CTX_start(ctx);
	x2 = BN_CTX_get(ctx);
	y2 = BN_CTX_get(ctx);
	c1x = BN_CTX_get(ctx);
	c1y = BN_CTX_get(ctx);
	if (c1y == NULL)
		goto done;

	/* Reconstruct C1 point from INTEGER coords */
	if (BN_bin2bn(c1x_c, c1x_n, c1x) == NULL ||
		BN_bin2bn(c1y_c, c1y_n, c1y) == NULL)
		goto done;
	C1 = EC_POINT_new(group);
	if (C1 == NULL)
		goto done;
	if (!EC_POINT_set_affine_coordinates_GFp(group, C1, c1x, c1y, ctx))
		goto done;

	/* kC1 = priv * C1 -> (x2, y2) */
	kC1 = EC_POINT_new(group);
	if (kC1 == NULL)
		goto done;
	if (!EC_POINT_mul(group, kC1, NULL, C1, priv_bn, ctx))
		goto done;
	if (!EC_POINT_get_affine_coordinates_GFp(group, kC1, x2, y2, ctx))
		goto done;

	x2y2 = (unsigned char *) palloc(SM2_FIELD_BYTES * 2);
	if (!bn_to_padded(x2, x2y2, SM2_FIELD_BYTES) ||
		!bn_to_padded(y2, x2y2 + SM2_FIELD_BYTES, SM2_FIELD_BYTES))
		goto done;

	/* KDF -> mask, plaintext = C2 XOR mask */
	mask = (unsigned char *) palloc(c2_n ? c2_n : 1);
	if (c2_n > 0 && sm2_kdf(x2y2, SM2_FIELD_BYTES * 2, mask, c2_n) != 1)
		goto done;
	pt = (unsigned char *) palloc(c2_n ? c2_n : 1);
	for (i = 0; i < c2_n; i++)
		pt[i] = mask[i] ^ c2_c[i];
	pt_len = c2_n;

	/* Verify C3 = SM3(x2 || pt || y2) */
	hash_buf = (unsigned char *) palloc(SM2_FIELD_BYTES + pt_len + SM2_FIELD_BYTES);
	memcpy(hash_buf, x2y2, SM2_FIELD_BYTES);
	memcpy(hash_buf + SM2_FIELD_BYTES, pt, pt_len);
	memcpy(hash_buf + SM2_FIELD_BYTES + pt_len, x2y2 + SM2_FIELD_BYTES,
		   SM2_FIELD_BYTES);
	if (tecc_sm3_hash(hash_buf, SM2_FIELD_BYTES + pt_len + SM2_FIELD_BYTES,
					  c3_calc) != 1)
		goto done;
	if (CRYPTO_memcmp(c3_calc, c3_c, TECC_SM2_C3_LEN) != 0)
		goto done;				/* authentication failed */

	*plain = OPENSSL_malloc(pt_len ? pt_len : 1);
	if (*plain == NULL)
	{
		rc = TECC_SM2_ALGO_ERR_MEMORY;
		goto done;
	}
	memcpy(*plain, pt, pt_len);
	*plain_len = pt_len;
	rc = TECC_SM2_ALGO_OK;

done:
	if (rc != TECC_SM2_ALGO_OK && *plain != NULL)
	{
		OPENSSL_free(*plain);
		*plain = NULL;
		*plain_len = 0;
	}
	if (x2y2) pfree(x2y2);
	if (mask) pfree(mask);
	if (hash_buf) pfree(hash_buf);
	if (pt) pfree(pt);
	EC_POINT_free(C1);
	EC_POINT_free(kC1);
	if (ctx != NULL)
	{
		BN_CTX_end(ctx);
		BN_CTX_free(ctx);
	}
	EC_KEY_free(priv_ec);
	return rc;
}

/* ==================== derive public point from private key ==================== */
int
tecc_sm2_algo_derive_pubkey(const unsigned char *priv_der, size_t priv_len,
							unsigned char **pub_point, size_t *pub_len)
{
	EC_KEY    *priv_ec = NULL;
	const EC_GROUP *group;
	const EC_POINT *P;
	BN_CTX    *ctx = NULL;
	unsigned char *out = NULL;
	size_t		out_len;
	int			rc = TECC_SM2_ALGO_ERR_CRYPTO;

	if (priv_der == NULL || priv_len == 0 || pub_point == NULL || pub_len == NULL)
		return TECC_SM2_ALGO_ERR_NULL;
	*pub_point = NULL;
	*pub_len = 0;

	priv_ec = tecc_sm2_algo_privkey_from_der(priv_der, priv_len);
	if (priv_ec == NULL)
		return TECC_SM2_ALGO_ERR_INVALID;

	group = EC_KEY_get0_group(priv_ec);
	P = EC_KEY_get0_public_key(priv_ec);
	ctx = BN_CTX_new();
	if (ctx == NULL)
		goto done;

	out_len = EC_POINT_point2oct(group, P, POINT_CONVERSION_UNCOMPRESSED,
								 NULL, 0, ctx);
	if (out_len == 0)
		goto done;
	out = OPENSSL_malloc(out_len);
	if (out == NULL)
	{
		rc = TECC_SM2_ALGO_ERR_MEMORY;
		goto done;
	}
	if (EC_POINT_point2oct(group, P, POINT_CONVERSION_UNCOMPRESSED,
						   out, out_len, ctx) != out_len)
	{
		OPENSSL_free(out);
		goto done;
	}
	*pub_point = out;
	*pub_len = out_len;
	rc = TECC_SM2_ALGO_OK;

done:
	if (ctx) BN_CTX_free(ctx);
	EC_KEY_free(priv_ec);
	return rc;
}
