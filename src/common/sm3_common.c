/*-------------------------------------------------------------------------
 *
 * md5_common.c
 *	  Routines shared between all MD5 implementations used for encrypted
 *	  passwords.
 *
 * Sverre H. Huseby <sverrehu@online.no>
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/common/md5_common.c
 *
 *-------------------------------------------------------------------------
 */

#ifndef FRONTEND
#include "postgres.h"
#else
#include "postgres_fe.h"
#endif

#ifdef USE_GMSSL

#include "common/cryptohash.h"
#include "common/sm3.h"

static void
bytesToHex(uint8 b[32], char *s)
{
	static const char *hex = "0123456789abcdef";
	int			q,
				w;

	for (q = 0, w = 0; q < 32; q++)
	{
		s[w++] = hex[(b[q] >> 4) & 0x0F];
		s[w++] = hex[b[q] & 0x0F];
	}
	s[w] = '\0';
}

bool
pg_sm3_hash(const void *buff, size_t len, char *hexsum)
{
	uint8		sum[PG_SM3_DIGEST_LENGTH];
	pg_cryptohash_ctx *ctx;

	ctx = pg_cryptohash_create(PG_SM3);
	if (ctx == NULL)
		return false;

	if (pg_cryptohash_init(ctx) < 0 ||
		pg_cryptohash_update(ctx, buff, len) < 0 ||
		pg_cryptohash_final(ctx, sum, sizeof(sum)) < 0)
	{
		pg_cryptohash_free(ctx);
		return false;
	}

	bytesToHex(sum, hexsum);
	pg_cryptohash_free(ctx);
	return true;
}

bool
pg_sm3_binary(const void *buff, size_t len, void *outbuf)
{
	pg_cryptohash_ctx *ctx;

	ctx = pg_cryptohash_create(PG_SM3);
	if (ctx == NULL)
		return false;

	if (pg_cryptohash_init(ctx) < 0 ||
		pg_cryptohash_update(ctx, buff, len) < 0 ||
		pg_cryptohash_final(ctx, outbuf, PG_SM3_DIGEST_LENGTH) < 0)
	{
		pg_cryptohash_free(ctx);
		return false;
	}

	pg_cryptohash_free(ctx);
	return true;
}


/*
 * Computes SM3 checksum of "passwd" (a null-terminated string) followed
 * by "salt" (which need not be null-terminated).
 *
 * Output format is "sm3" followed by a 32-hex-digit SM3 checksum.
 * Hence, the output buffer "buf" must be at least 36 bytes long.
 *
 * Returns true if okay, false on error (out of memory).
 */
bool
pg_sm3_encrypt(const char *passwd, const char *salt, size_t salt_len,
			   char *buf)
{
	size_t		passwd_len = strlen(passwd);

	/* +1 here is just to avoid risk of unportable malloc(0) */
	char	   *crypt_buf = malloc(passwd_len + salt_len + 1);
	bool		ret;

	if (!crypt_buf)
		return false;

	/*
	 * Place salt at the end because it may be known by users trying to crack
	 * the MD5 output.
	 */
	memcpy(crypt_buf, passwd, passwd_len);
	memcpy(crypt_buf + passwd_len, salt, salt_len);

	strcpy(buf, "sm3");
	ret = pg_sm3_hash(crypt_buf, passwd_len + salt_len, buf + 3);

	free(crypt_buf);

	return ret;
}

#endif
