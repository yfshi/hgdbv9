/*
 * sm2_pub.c — SM2 public-key (.pub) file validation
 *
 * tecc_ktool generate writes <key-name>.pub as a plain 65-byte uncompressed
 * EC point (0x04 || X || Y) — no TECCKEK envelope and no HMAC, since it is
 * public data (see cmd_generate.c).  validate/info must therefore not treat
 * it as a KEK envelope.  This module validates such a file: fixed length +
 * 0x04 prefix, then checks the point actually lies on sm2p256v1, so that
 * arbitrary tampering (even a single flipped byte inside X/Y) is detected.
 *
 * src/bin/tecc_ktool/sm2_pub.c
 */

#include "tecc_ktool_common.h"

#include <stdarg.h>
#include <openssl/bn.h>
#include <openssl/ec.h>

/* sm2p256v1 field equation y^2 = x^3 + a*x + b (explicit params; same values
 * as cmd_generate.c — no NID_sm2 needed, works on any OpenSSL). */
#define TECC_SM2_P "FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFF"
#define TECC_SM2_A "FFFFFFFEFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF00000000FFFFFFFFFFFFFFFC"
#define TECC_SM2_B "28E9FA9E9D9F5E344D5A9E4BCF6509A7F39789F515AB8F92DDBCBD414D940E93"

#define SM2_PUB_RAW_LEN 65		/* 0x04 prefix + X(32) + Y(32) */
#define SM2_PUB_POINT_TAG 0x04	/* uncompressed EC point marker */

static void
set_err(char *errbuf, size_t errsz, const char *fmt,...)
{
	va_list		args;

	if (errbuf == NULL || errsz == 0)
		return;
	va_start(args, fmt);
	vsnprintf(errbuf, errsz, fmt, args);
	va_end(args);
}

/*
 * Validate a plain SM2 public-key file (as written by generate for sm2 keys).
 *
 * Returns 0 if the file is a well-formed uncompressed point on sm2p256v1,
 * -1 otherwise.  errbuf (if non-NULL) receives a human-readable reason.
 */
int
tecc_sm2_pub_check(const char *path, char *errbuf, size_t errsz)
{
	FILE	   *fp;
	struct stat st;
	unsigned char buf[SM2_PUB_RAW_LEN];
	size_t		nread;
	BN_CTX    *ctx = NULL;
	BIGNUM    *p = NULL;
	BIGNUM    *a = NULL;
	BIGNUM    *b = NULL;
	BIGNUM    *x = NULL;
	BIGNUM    *y = NULL;
	EC_GROUP  *group = NULL;
	EC_POINT  *point = NULL;
	int			ret = -1;

	if (errbuf && errsz > 0)
		errbuf[0] = '\0';

	if (stat(path, &st) != 0)
	{
		set_err(errbuf, errsz, "cannot access %s: %s", path, strerror(errno));
		return -1;
	}

	if (st.st_size != SM2_PUB_RAW_LEN)
	{
		set_err(errbuf, errsz,
				"bad length (%lld bytes, expected %d-byte uncompressed SM2 point)",
				(long long) st.st_size, SM2_PUB_RAW_LEN);
		return -1;
	}

	fp = fopen(path, "rb");
	if (fp == NULL)
	{
		set_err(errbuf, errsz, "cannot open %s: %s", path, strerror(errno));
		return -1;
	}
	nread = fread(buf, 1, sizeof(buf), fp);
	fclose(fp);
	if (nread != sizeof(buf))
	{
		set_err(errbuf, errsz, "short read from %s", path);
		return -1;
	}

	if (buf[0] != SM2_PUB_POINT_TAG)
	{
		set_err(errbuf, errsz,
				"not an uncompressed EC point (first byte 0x%02x, expected 0x%02x)",
				buf[0], SM2_PUB_POINT_TAG);
		return -1;
	}

	/* Point-on-curve check: does (X, Y) satisfy y^2 = x^3 + a*x + b (mod p)? */
	ctx = BN_CTX_new();
	if (ctx == NULL)
	{
		set_err(errbuf, errsz, "out of memory");
		return -1;
	}
	BN_CTX_start(ctx);
	p = BN_CTX_get(ctx);
	a = BN_CTX_get(ctx);
	b = BN_CTX_get(ctx);
	x = BN_CTX_get(ctx);
	y = BN_CTX_get(ctx);
	if (y == NULL ||
		!BN_hex2bn(&p, TECC_SM2_P) ||
		!BN_hex2bn(&a, TECC_SM2_A) ||
		!BN_hex2bn(&b, TECC_SM2_B) ||
		(x = BN_bin2bn(buf + 1, 32, x)) == NULL ||
		(y = BN_bin2bn(buf + 33, 32, y)) == NULL)
	{
		set_err(errbuf, errsz, "failed to parse curve parameters");
		goto done;
	}

	group = EC_GROUP_new_curve_GFp(p, a, b, ctx);
	if (group == NULL)
	{
		set_err(errbuf, errsz, "failed to build sm2p256v1 group");
		goto done;
	}
	point = EC_POINT_new(group);
	if (point == NULL)
	{
		set_err(errbuf, errsz, "out of memory");
		goto done;
	}
	if (!EC_POINT_set_affine_coordinates_GFp(group, point, x, y, ctx) ||
		EC_POINT_is_on_curve(group, point, ctx) != 1)
	{
		set_err(errbuf, errsz,
				"point is not on the sm2p256v1 curve (file tampered?)");
		goto done;
	}

	ret = 0;

done:
	EC_POINT_free(point);
	EC_GROUP_free(group);
	BN_CTX_end(ctx);
	BN_CTX_free(ctx);
	return ret;
}
