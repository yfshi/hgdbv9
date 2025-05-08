/* ---------------------------------------------------------------------
 *
 * File: pass_crypt.c
 *
 * Abstract:
 * 		This file contains the encryption and decryption functions of the pgpass file.
 *
 * Author: suzy@highgo.com, 20241220.
 *
 * Copyright:
 * Copyright (c) 2024, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 * 		src/common/pass_crypt.c
 *
 * Add files that require "SRS-SECURITY-IDENTITYAUTH"
 *
 * ---------------------------------------------------------------------
 */

#ifndef FRONTEND
#include "postgres.h"
#else
#include "postgres_fe.h"
#endif

#ifdef USE_GMSSL
#include <gmssl/sm4.h>

#include <sys/param.h>
#include <unistd.h>

#include "common/sm3.h"
#include "common/hmac.h"
#include "common/hkdf.h"
#include "common/mac.h"
#include "common/base64.h"
#include "common/pass_crypt.h"

#include "common/controldata_utils.h"
#include "storage/fd.h"

static const char _base64[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

static void pass_hmac(const unsigned char *msg, size_t msg_len, const unsigned char *key, unsigned char *digest)
{
	pg_hmac_ctx *ctx = pg_hmac_create(PG_SM3);
	pg_hmac_init(ctx, key, SM4_KEY_SIZE);
	pg_hmac_update(ctx, msg, msg_len);
	pg_hmac_final(ctx, digest, PG_SM3_DIGEST_LENGTH);
}

static ControlFileData *
get_identifier(const char *DataDir, bool *crc_ok_p)
{
	ControlFileData *ControlFile;
	int fd;
	char ControlFilePath[MAXPGPATH];
	pg_crc32c crc;
	int r;
#ifdef FRONTEND
	pg_crc32c last_crc;
	int retries = 0;
#endif

	AssertArg(crc_ok_p);

	ControlFile = palloc(sizeof(ControlFileData));
	snprintf(ControlFilePath, MAXPGPATH, "%s/global/pg_control", DataDir);

#ifdef FRONTEND
	INIT_CRC32C(last_crc);

retry:
#endif

#ifndef FRONTEND
	if ((fd = OpenTransientFile(ControlFilePath, O_RDONLY | PG_BINARY)) == -1)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not open file \"%s\" for reading: %m",
						ControlFilePath)));
#else
	if ((fd = open(ControlFilePath, O_RDONLY | PG_BINARY, 0)) == -1)
	{
		exit(EXIT_FAILURE);
	}
#endif

	r = read(fd, ControlFile, sizeof(ControlFileData));
	if (r != sizeof(ControlFileData))
	{
		if (r < 0)
#ifndef FRONTEND
			ereport(ERROR,
					(errcode_for_file_access(),
					 errmsg("could not read file \"%s\": %m", ControlFilePath)));
#else
		{
			exit(EXIT_FAILURE);
		}
#endif
		else
#ifndef FRONTEND
			ereport(ERROR,
					(errcode(ERRCODE_DATA_CORRUPTED),
					 errmsg("could not read file \"%s\": read %d of %zu",
							ControlFilePath, r, sizeof(ControlFileData))));
#else
		{
			exit(EXIT_FAILURE);
		}
#endif
	}

#ifndef FRONTEND
	if (CloseTransientFile(fd) != 0)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not close file \"%s\": %m",
						ControlFilePath)));
#else
	if (close(fd) != 0)
	{
		exit(EXIT_FAILURE);
	}
#endif

	/* Check the CRC. */
	INIT_CRC32C(crc);
	COMP_CRC32C(crc,
				(char *)ControlFile,
				offsetof(ControlFileData, crc));
	FIN_CRC32C(crc);

	*crc_ok_p = EQ_CRC32C(crc, ControlFile->crc);

#ifdef FRONTEND

	/*
	 * If the server was writing at the same time, it is possible that we read
	 * partially updated contents on some systems.  If the CRC doesn't match,
	 * retry a limited number of times until we compute the same bad CRC twice
	 * in a row with a short sleep in between.  Then the failure is unlikely
	 * to be due to a concurrent write.
	 */
	if (!*crc_ok_p &&
		(retries == 0 || !EQ_CRC32C(crc, last_crc)) &&
		retries < 10)
	{
		retries++;
		last_crc = crc;
		pg_usleep(10000);
		goto retry;
	}
#endif

	/* Make sure the control file is valid byte order. */
	if (ControlFile->pg_control_version % 65536 == 0 &&
		ControlFile->pg_control_version / 65536 != 0)
#ifndef FRONTEND
		elog(ERROR, _("byte ordering mismatch"));
#else
		;
#endif

	return ControlFile;
}

/*
 * pass_is_encrypted
 *
 * If it is ciphertext, the last 16 bytes are
 * the first 16 bytes of the sm3-hmac values
 * of all the previous data; if not, it is plaintext.
 */
bool
pass_is_encrypted(const char *passwd)
{
	unsigned char b64_dec[BUFFER_MAX_SIZE];
	int b64_dec_len = 0;
	unsigned char digst[PG_SM3_DIGEST_LENGTH];

	if (strspn(passwd, _base64) != strlen(passwd))
	{
		return false;
	}
	b64_dec_len = pg_b64_decode(passwd, strlen(passwd), (char *)b64_dec, sizeof(b64_dec));
	if (b64_dec_len <= 0 || b64_dec_len < 64 || b64_dec_len % 16 != 0)
	{
		return false;
	}

	pg_sm3_binary(b64_dec, b64_dec_len - 16, digst);

	if (memcmp(digst, b64_dec + b64_dec_len - 16, 16) == 0)
		return true;

	return false;
}

/*
 * pass_encrypt
 *
 * Encrypt the "password" field.
 */
bool
pass_encrypt(const char *passwd, const unsigned char *key, const unsigned char *hmac_key, char *out)
{
	unsigned char buf_plain[BUFFER_MAX_SIZE];
	SM4_KEY ctx;
	unsigned char out_enc[BUFFER_MAX_SIZE];
	size_t out_enc_len = 0;
	int out_len = 0;

	pg_strong_random(out_enc, SM4_IV_SIZE);
	out_len = SM4_IV_SIZE;

	memcpy(out_enc + SM4_IV_SIZE, passwd, strlen(passwd));
	out_len += strlen(passwd) > PWD_MAX_SIZE ? PWD_MAX_SIZE : strlen(passwd);

	pass_hmac(out_enc, out_len, hmac_key, out_enc + out_len);
	out_len += 16;

	sm4_set_encrypt_key(&ctx, key);
	memcpy(buf_plain, out_enc + SM4_IV_SIZE, out_len - SM4_IV_SIZE);
	sm4_cbc_padding_encrypt(&ctx, out_enc,
							buf_plain, out_len - SM4_IV_SIZE,
							out_enc + SM4_IV_SIZE, &out_enc_len);
	out_len = SM4_IV_SIZE + out_enc_len;

	pg_sm3_binary(out_enc, out_len, out_enc + out_len);

	out_len += 16;

	pg_b64_encode((char *)out_enc, out_len, out, 256);
	return true;
}

/*
 * pass_decrypt
 *
 * Decrypt the "password" field.
 */
bool
pass_decrypt(const char *passwd, const unsigned char *key, const unsigned char *hmac_key, char *out)
{
	unsigned char buf_cipher[BUFFER_MAX_SIZE];
	SM4_KEY ctx;
	unsigned char pass_dec[BUFFER_MAX_SIZE];
	size_t pass_dec_len = 0;
	unsigned char digst[PG_SM3_DIGEST_LENGTH];

	pass_dec_len = pg_b64_decode(passwd, strlen(passwd), (char *)pass_dec, sizeof(pass_dec));
	if (pass_dec_len <= 0 || pass_dec_len < 64 || pass_dec_len % 16 != 0)
	{
		return false;
	}
	pass_dec_len -= 16;
	pass_dec_len -= SM4_IV_SIZE;
	sm4_set_decrypt_key(&ctx, key);
	memcpy(buf_cipher, pass_dec + SM4_IV_SIZE, pass_dec_len);
	sm4_cbc_padding_decrypt(&ctx, pass_dec,							// key、iv
							buf_cipher, pass_dec_len,	// in、inlen
							pass_dec + SM4_IV_SIZE, &pass_dec_len); // out、outlen
	pass_dec_len -= 16;
	pass_hmac(pass_dec, pass_dec_len + SM4_IV_SIZE, hmac_key, digst);
	if (memcmp(digst, pass_dec + SM4_IV_SIZE + pass_dec_len, 16) != 0)
		return false;

	memcpy(out, pass_dec + SM4_IV_SIZE, pass_dec_len);
	out[pass_dec_len] = '\0';
	return true;
}

/*
 * pass_gen_key
 *
 * Generate the encryption key based on the input database path,
 * network card name and custom password.
 */
bool
pass_gen_key(char *DataDir, char *Interface, char *CustomKey, unsigned char *sm4_key)
{

	char ident[17] = {0};
	int ident_len = 0;
	char mac[13] = {0};
	int mac_len = 0;
	int customkey_len = 0;
	bool ret = false;
	bool crc_ok;
	ControlFileData *ControlFile;

	/* Require identifier*/
	if (DataDir && DataDir[0] != '\0')
	{
		/* get a copy of the control file */
		ControlFile = get_identifier(DataDir, &crc_ok);
		if (!crc_ok)
			printf(_("WARNING: The identifier was requested, but failed to obtain it.\n\n"));
		snprintf(ident, sizeof(ident) - 1, "%llx",
				 (unsigned long long)ControlFile->system_identifier);
		ident_len = 11;
	}

	/* Require MAC */
	if (Interface && Interface[0] != '\0')
	{
		ret = pg_get_mac_address(Interface, mac);
		mac_len = 12;
	}

	/* Require custom key */
	if (CustomKey && CustomKey[0] != '\0')
		customkey_len = strlen(CustomKey);

	ret = sm3_hkdf_expand(sm4_key,
						  (unsigned char *)ident, ident_len,
						  (unsigned char *)mac, mac_len,
						  (unsigned char *)CustomKey, customkey_len,
						  16);
	return ret;
}

#endif
