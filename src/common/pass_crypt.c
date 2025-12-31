/* ---------------------------------------------------------------------
 *
 * File: pass_crypt.c
 *
 * Abstract:
 * 		This file contains the encryption and decryption functions of the pgpass file.
 * 		Use SM4 and SM3 encryption when using --with-gmssl
 * 		Use AES and SHA256 encryption when using --with-openssl
 * 		Neither or them uses XOR and SHA256 encryption when in use
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

#include <sys/param.h>
#include <unistd.h>

#ifdef USE_GMSSL
#include <gmssl/sm4.h>
#include "common/sm3.h"
#elif defined(USE_OPENSSL)
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include "common/md5.h"
#else
#include "common/md5.h"
#endif

#include "common/hmac.h"
#include "common/hkdf.h"
#include "common/mac.h"
#include "common/base64.h"
#include "common/pass_crypt.h"

#include "common/controldata_utils.h"
#include "storage/fd.h"

static const char _base64[] =
	"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

static bool pass_hmac(const unsigned char *msg, size_t msg_len, const unsigned char *key, unsigned char *digest)
{
#ifdef USE_GMSSL
	pg_hmac_ctx *ctx = pg_hmac_create(PG_SM3);
#else
	pg_hmac_ctx *ctx = pg_hmac_create(PG_SHA256);
#endif
	if (!ctx)
		return false;

	pg_hmac_init(ctx, key, 16);
	pg_hmac_update(ctx, msg, msg_len);
	pg_hmac_final(ctx, digest, PG_DIGEST_LENGTH);
	pg_hmac_free(ctx);

	return true;
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
	if (!ControlFile)
		return NULL;
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

static int
symm_encrypt(const uint8_t *key, const uint8_t *iv,
			 uint8_t *in, int inlen, uint8_t *out, size_t *outlen)
{
#ifdef USE_GMSSL
	{
		SM4_KEY ctx;
		sm4_set_encrypt_key(&ctx, key);
		sm4_cbc_padding_encrypt(&ctx, iv,
								in, inlen,
								out, outlen);
		return 1;
	}
#elif defined(USE_OPENSSL)
	{
		int outl = 0;
		EVP_CIPHER_CTX *ctx = NULL;
		int ciphertext_len = 0;
		int ret = -1;

		if (!(ctx = EVP_CIPHER_CTX_new()))
		{
			goto end;
		}

		if (1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv))
		{
			goto end;
		}

		if (1 != EVP_EncryptUpdate(ctx, out, &outl, in, inlen))
		{
			goto end;
		}
		ciphertext_len = outl;

		if (1 != EVP_EncryptFinal_ex(ctx, out + ciphertext_len, &outl))
		{
			goto end;
		}
		ciphertext_len += outl;

		*outlen = ciphertext_len;
		ret = ciphertext_len;

	end:
		if (ctx)
			EVP_CIPHER_CTX_free(ctx);
		return ret;
	}
#else
	{
		int i = 0;
		unsigned char pad = 0;

		pad = 15 - ((inlen + 15)%16);
		memcpy(out, in, inlen);
		memset(out + inlen, pad, pad);
		for (i = 0; i < inlen + pad; i++)
		{
			out[i] = out[i] ^ iv[i%16];
			out[i] = out[i] ^ key[i%16];
		}
		*outlen = inlen + pad;
		return 1;
	}
#endif
}

static int
symm_decrypt(const uint8_t *key, const uint8_t *iv,
			 uint8_t *in, int inlen, uint8_t *out, size_t *outlen)
{
#ifdef USE_GMSSL
	{
		SM4_KEY ctx;
		sm4_set_decrypt_key(&ctx, key);
		sm4_cbc_padding_decrypt(&ctx, iv,	  // key、iv
								in, inlen,	  // in、inlen
								out, outlen); // out、outlen
		return 1;
	}
#elif defined(USE_OPENSSL)
	{
		int outl = 0;
		EVP_CIPHER_CTX *ctx = NULL;
		int plaintext_len = 0;
		int ret = -1;

		if (!(ctx = EVP_CIPHER_CTX_new()))
		{
			goto end;
		}

		if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv))
		{
			goto end;
		}

		if (1 != EVP_DecryptUpdate(ctx, out, &outl, in, inlen))
		{
			goto end;
		}
		plaintext_len = outl;

		if (1 != EVP_DecryptFinal_ex(ctx, out + plaintext_len, &outl))
		{
			goto end;
		}
		plaintext_len += outl;

		*outlen = plaintext_len;
		ret = plaintext_len;

	end:
		if (ctx)
			EVP_CIPHER_CTX_free(ctx);
		return ret;
	}
#else
	{
		int i = 0;
		for (i = 0; i < inlen; i++)
		{
			out[i] = in[i] ^ key[i%16];
			out[i] = out[i] ^ iv[i%16];
		}
		*outlen = inlen - out[inlen - 1];

		return 1;
	}
#endif
}

/*
 * pass_is_encrypted
 *
 * If it is ciphertext, the last 16 bytes are
 * the first 16 bytes of the hmac values
 * of all the previous data; if not, it is plaintext.
 */
bool pass_is_encrypted(const char *passwd)
{
	unsigned char b64_dec[BUFFER_MAX_SIZE];
	int b64_dec_len = 0;
	unsigned char digst[PG_DIGEST_LENGTH];

	if (strspn(passwd, _base64) != strlen(passwd))
	{
		return false;
	}
	b64_dec_len = pg_b64_decode(passwd, strlen(passwd), (char *)b64_dec, sizeof(b64_dec));
	if (b64_dec_len <= 0 || b64_dec_len < 64 || b64_dec_len % 16 != 0)
	{
		return false;
	}

#ifdef USE_GMSSL
	pg_sm3_binary(b64_dec, b64_dec_len - 16, digst);
#else
	pg_md5_binary(b64_dec, b64_dec_len - 16, digst);
#endif

	if (memcmp(digst, b64_dec + b64_dec_len - 16, 16) == 0)
		return true;

	return false;
}

/*
 * pass_encrypt
 *
 * Encrypt the "password" field.
 */
bool pass_encrypt(const char *passwd, const unsigned char *key, const unsigned char *hmac_key, char *out)
{
	unsigned char buf_plain[BUFFER_MAX_SIZE];
	unsigned char out_enc[BUFFER_MAX_SIZE];
	size_t out_enc_len = 0;
	int out_len = 0;

	pg_strong_random(out_enc, SYMM_IV_SIZE);
	out_len = SYMM_IV_SIZE;

	memcpy(out_enc + SYMM_IV_SIZE, passwd, strlen(passwd));
	out_len += strlen(passwd) > PWD_MAX_SIZE ? PWD_MAX_SIZE : strlen(passwd);

	if(!pass_hmac(out_enc, out_len, hmac_key, out_enc + out_len))
	{
#ifndef FRONTEND
		elog(WARNING, _("Encrypt:Failed to generate the plaintext check code"));
#else
		printf(_("WARNING:Encrypt:Failed to generate the plaintext check code.\n"));
#endif
		return false;
	}
	out_len += 16;

	memcpy(buf_plain, out_enc + SYMM_IV_SIZE, out_len - SYMM_IV_SIZE);

	symm_encrypt(key, out_enc,
						buf_plain, out_len - SYMM_IV_SIZE,
						out_enc + SYMM_IV_SIZE, &out_enc_len);

	out_len = SYMM_IV_SIZE + out_enc_len;

#ifdef USE_GMSSL
	pg_sm3_binary(out_enc, out_len, out_enc + out_len);
#else
	pg_md5_binary(out_enc, out_len, out_enc + out_len);
#endif
	out_len += 16;

	pg_b64_encode((char *)out_enc, out_len, out, 256);
	return true;
}

/*
 * pass_decrypt
 *
 * Decrypt the "password" field.
 */
bool pass_decrypt(const char *passwd, const unsigned char *key, const unsigned char *hmac_key, char *out)
{
	unsigned char buf_cipher[BUFFER_MAX_SIZE];

	unsigned char pass_dec[BUFFER_MAX_SIZE];
	size_t pass_dec_len = 0;
	unsigned char digst[PG_DIGEST_LENGTH];

	pass_dec_len = pg_b64_decode(passwd, strlen(passwd), (char *)pass_dec, sizeof(pass_dec));
	if (pass_dec_len <= 0 || pass_dec_len < 64 || pass_dec_len % 16 != 0)
	{
#ifndef FRONTEND
		elog(WARNING, _("Decrypt:Non-base64 encoding"));
#else
		printf(_("WARNING:Decrypt:Non-base64 encoding.\n"));
#endif
		return false;
	}
	pass_dec_len -= 16;
	pass_dec_len -= SYMM_IV_SIZE;

	memcpy(buf_cipher, pass_dec + SYMM_IV_SIZE, pass_dec_len);

	symm_decrypt(key, pass_dec,							// key、iv
							buf_cipher, pass_dec_len,				// in、inlen
							pass_dec + SYMM_IV_SIZE, &pass_dec_len); // out、outlen

	pass_dec_len -= 16;
	pass_hmac(pass_dec, pass_dec_len + SYMM_IV_SIZE, hmac_key, digst);
	if (memcmp(digst, pass_dec + SYMM_IV_SIZE + pass_dec_len, 16) != 0)
	{
#ifndef FRONTEND
		elog(WARNING, _("Decrypt:The plaintext password verification failed(hmac)"));
#else
		printf(_("WARNING:Decrypt:The plaintext password verification failed(hmac).\n"));
#endif
		return false;
	}

	memcpy(out, pass_dec + SYMM_IV_SIZE, pass_dec_len);
	out[pass_dec_len] = '\0';
	return true;
}

/*
 * pass_gen_key
 *
 * Generate the encryption key based on the input database path,
 * network card name and custom password.
 */
bool pass_gen_key(char *DataDir, char *Interface, char *CustomKey, unsigned char *sm4_key)
{
	char ident[17] = {0};
	int ident_len = 0;
	char mac[13] = {0};
	int mac_len = 0;
	int customkey_len = 0;
	bool ret = false;
	ControlFileData *ControlFile;

	/* Require identifier*/
	if (DataDir && DataDir[0] != '\0')
	{
		bool crc_ok = true;
		/* get a copy of the control file */
		ControlFile = get_identifier(DataDir, &crc_ok);

		if (!ControlFile){
#ifndef FRONTEND
			elog(WARNING, _("GenKey:The identifier was requested, but failed to obtain it."));
#else
			printf(_("WARNING:GenKey:The identifier was requested, but failed to obtain it.\n"));
#endif
			return false;
		}
		if (!crc_ok){
#ifndef FRONTEND
			elog(WARNING, _("GenKey:The identifier was requested, but failed to obtain it(crc check failed)"));
#else
			printf(_("WARNING:GenKey:The identifier was requested, but failed to obtain it(crc check failed).\n"));
#endif
			pfree(ControlFile);
			return false;
		}

		snprintf(ident, sizeof(ident) - 1, "%llx",
				 (unsigned long long)ControlFile->system_identifier);
		ident_len = 11;
		pfree(ControlFile);
	}

	/* Require MAC */
	if (Interface && Interface[0] != '\0')
	{
		ret = pg_get_mac_address(Interface, mac);
		if(!ret){
#ifndef FRONTEND
			elog(WARNING, _("GenKey:The mac was requested, but failed to obtain it."));
#else
			printf(_("WARNING:GenKey:The mac was requested, but failed to obtain it.\n"));
#endif
			return false;
		}
		mac_len = 12;
	}

	/* Require custom key */
	if (CustomKey && CustomKey[0] != '\0')
		customkey_len = strlen(CustomKey);

	/* The fist 16 bytes are the key, and the last 16 bytes are the hamc key */
	ret = hkdf_expand_key(sm4_key,
						  (unsigned char *)ident, ident_len,
						  (unsigned char *)mac, mac_len,
						  (unsigned char *)CustomKey, customkey_len,
						  16);
	return ret;
}
