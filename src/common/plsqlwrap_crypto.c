/*-------------------------------------------------------------------------
 *
 * plsqlwrap_crypto.c
 *	 Shared encryption and decryption for wrapped PL/SQL source.
 *
 *-------------------------------------------------------------------------
 */
/* Begin - Reqid:SRS-PLPGSQL-HIDDEN*/
#ifndef FRONTEND
#include "postgres.h"
#else
#include "postgres_fe.h"
#endif

#include <limits.h>

#ifdef USE_OPENSSL
#include <openssl/evp.h>
#endif

#ifdef USE_GMSSL
#include <gmssl/aes.h>
#include <gmssl/sm4.h>
#endif

#include "common/base64.h"
#include "common/plsqlwrap_crypto.h"
#include "port.h"

#define PG_PLSQL_WRAP_AES_KEY_LEN 32
#define PG_PLSQL_WRAP_AES_IV_LEN 16
#define PG_PLSQL_WRAP_SM4_KEY_LEN 16
#define PG_PLSQL_WRAP_SM4_IV_LEN 16

#ifndef FRONTEND
#define PLSQL_WRAP_ALLOC(size) palloc(size)
#define PLSQL_WRAP_FREE(ptr) pfree(ptr)
#else
#define PLSQL_WRAP_ALLOC(size) pg_malloc(size)
#define PLSQL_WRAP_FREE(ptr) pg_free(ptr)
#endif

/* Use stable keys so wrapped source remains valid across dump and restore. */
static const unsigned char pg_plsql_wrap_aes_key[PG_PLSQL_WRAP_AES_KEY_LEN] = {
	0x69, 0x76, 0x6f, 0x72, 0x79, 0x73, 0x71, 0x6c,
	0x20, 0x70, 0x6c, 0x73, 0x71, 0x6c, 0x20, 0x77,
	0x72, 0x61, 0x70, 0x20, 0x61, 0x65, 0x73, 0x32,
	0x35, 0x36, 0x20, 0x6b, 0x65, 0x79, 0x21, 0x01
};

static const unsigned char pg_plsql_wrap_sm4_key[PG_PLSQL_WRAP_SM4_KEY_LEN] = {
	0x69, 0x76, 0x6f, 0x72, 0x79, 0x73, 0x71, 0x6c,
	0x20, 0x70, 0x6c, 0x73, 0x71, 0x6c, 0x20, 0x73
};

/* Select the envelope prefix and key for an explicit cipher type. */
static bool
pg_plsql_wrap_cipher_parameters(PgPlsqlWrapCipher cipher_type,
								const char **prefix,
								const unsigned char **key,
								int *iv_len)
{
	switch (cipher_type)
	{
		case PG_PLSQL_WRAP_CIPHER_AES:
			*prefix = PG_PLSQL_WRAP_AES_PREFIX;
			*key = pg_plsql_wrap_aes_key;
			*iv_len = PG_PLSQL_WRAP_AES_IV_LEN;
			return true;
		case PG_PLSQL_WRAP_CIPHER_SM4:
			*prefix = PG_PLSQL_WRAP_SM4_PREFIX;
			*key = pg_plsql_wrap_sm4_key;
			*iv_len = PG_PLSQL_WRAP_SM4_IV_LEN;
			return true;
	}

	return false;
}

#if defined(USE_OPENSSL) && !defined(USE_GMSSL)
/* Look up SM4-CTR without requiring a configure-time OpenSSL symbol. */
static const EVP_CIPHER *
pg_plsql_wrap_sm4_cipher(void)
{
	const EVP_CIPHER *cipher;

	cipher = EVP_get_cipherbyname("SM4-CTR");
	if (cipher == NULL)
		cipher = EVP_get_cipherbyname("sm4-ctr");

	return cipher;
}

/* Select the OpenSSL EVP cipher for the requested source wrapping cipher. */
static const EVP_CIPHER *
pg_plsql_wrap_evp_cipher(PgPlsqlWrapCipher cipher_type)
{
	if (cipher_type == PG_PLSQL_WRAP_CIPHER_AES)
		return EVP_aes_256_ctr();
	if (cipher_type == PG_PLSQL_WRAP_CIPHER_SM4)
		return pg_plsql_wrap_sm4_cipher();

	return NULL;
}
#endif

/* Encrypt source through the provider selected at configure time. */
char *
pg_plsql_wrap_encrypt(const char *src, PgPlsqlWrapCipher cipher_type,
					  const char **error_message)
{
	const char *prefix;
	const unsigned char *key;
	int			iv_len;
	size_t		src_len;
	size_t		prefix_len;
	size_t		magic_len = strlen(PG_PLSQL_WRAP_MAGIC);
	unsigned char iv[PG_PLSQL_WRAP_AES_IV_LEN];
	unsigned char *ciphertext;
	unsigned char *blob;
	size_t		cipher_len;
	size_t		blob_len;
	int			encoded_len;
	int			b64_len;
	char	   *result;

	*error_message = NULL;
	if (!pg_plsql_wrap_cipher_parameters(cipher_type, &prefix, &key, &iv_len))
	{
		*error_message = "invalid PL/SQL source wrapping cipher";
		return NULL;
	}

	src_len = strlen(src);
	prefix_len = strlen(prefix);
	if (src_len > INT32_MAX)
	{
		*error_message = "PL/SQL source is too large to wrap";
		return NULL;
	}
	if (!pg_strong_random(iv, iv_len))
	{
		*error_message = "could not generate random bytes for PL/SQL source wrapping";
		return NULL;
	}

	/* CTR preserves source length and therefore does not require padding. */
	ciphertext = (unsigned char *) PLSQL_WRAP_ALLOC(src_len == 0 ? 1 : src_len);
#ifdef USE_GMSSL
	{
		unsigned char ctr[PG_PLSQL_WRAP_AES_IV_LEN];

		memcpy(ctr, iv, sizeof(ctr));
		if (cipher_type == PG_PLSQL_WRAP_CIPHER_AES)
		{
			AES_KEY		aes_key;

			if (aes_set_encrypt_key(&aes_key, key, AES256_KEY_SIZE) != 1)
			{
				PLSQL_WRAP_FREE(ciphertext);
				*error_message = "could not initialize GmSSL AES key";
				return NULL;
			}
			aes_ctr_encrypt(&aes_key, ctr, (const uint8_t *) src, src_len,
							ciphertext);
		}
		else
		{
			SM4_KEY		sm4_key;

			sm4_set_encrypt_key(&sm4_key, key);
			sm4_ctr_encrypt(&sm4_key, ctr, (const uint8_t *) src, src_len,
							ciphertext);
		}
	}
#elif defined(USE_OPENSSL)
	{
		const EVP_CIPHER *cipher = pg_plsql_wrap_evp_cipher(cipher_type);
		EVP_CIPHER_CTX *ctx;
		int			out_len = 0;
		int			final_len = 0;

		if (cipher == NULL)
		{
			PLSQL_WRAP_FREE(ciphertext);
			*error_message = "SM4-CTR is not available in this OpenSSL build";
			return NULL;
		}
		ctx = EVP_CIPHER_CTX_new();
		if (ctx == NULL ||
			EVP_EncryptInit_ex(ctx, cipher, NULL, key, iv) != 1 ||
			EVP_EncryptUpdate(ctx, ciphertext, &out_len,
							  (const unsigned char *) src, (int) src_len) != 1 ||
			EVP_EncryptFinal_ex(ctx, ciphertext + out_len, &final_len) != 1)
		{
			if (ctx != NULL)
				EVP_CIPHER_CTX_free(ctx);
			PLSQL_WRAP_FREE(ciphertext);
			*error_message = "could not encrypt PL/SQL source";
			return NULL;
		}
		EVP_CIPHER_CTX_free(ctx);
		if ((size_t) (out_len + final_len) != src_len)
		{
			PLSQL_WRAP_FREE(ciphertext);
			*error_message = "unexpected PL/SQL CTR ciphertext length";
			return NULL;
		}
	}
#else
	PLSQL_WRAP_FREE(ciphertext);
	*error_message = "PL/SQL source wrapping requires GmSSL or OpenSSL";
	return NULL;
#endif

	cipher_len = src_len;
	blob_len = iv_len + cipher_len;
	blob = (unsigned char *) PLSQL_WRAP_ALLOC(blob_len);
	memcpy(blob, iv, iv_len);
	memcpy(blob + iv_len, ciphertext, cipher_len);
	b64_len = pg_b64_enc_len(blob_len);
	result = (char *) PLSQL_WRAP_ALLOC(magic_len + prefix_len + b64_len + 1);
	memcpy(result, PG_PLSQL_WRAP_MAGIC, magic_len);
	memcpy(result + magic_len, prefix, prefix_len);
	encoded_len = pg_b64_encode((const char *) blob, blob_len,
								result + magic_len + prefix_len, b64_len);
	PLSQL_WRAP_FREE(ciphertext);
	PLSQL_WRAP_FREE(blob);
	if (encoded_len < 0)
	{
		PLSQL_WRAP_FREE(result);
		*error_message = "could not encode wrapped PL/SQL source";
		return NULL;
	}
	result[magic_len + prefix_len + encoded_len] = '\0';

	return result;
}

/* Quote a wrapped payload so generated DDL cannot lex ciphertext as an identifier. */
char *
pg_plsql_wrap_quote_payload(const char *wrapped)
{
	Size		magic_len = strlen(PG_PLSQL_WRAP_MAGIC);
	const char *payload;
	Size		payload_len;
	char	   *quoted;

	if (strncmp(wrapped, PG_PLSQL_WRAP_MAGIC, magic_len) != 0)
		return NULL;

	/* Keep the magic header readable and quote only the opaque payload line. */
	payload = wrapped + magic_len;
	payload_len = strlen(payload);
	quoted = (char *) PLSQL_WRAP_ALLOC(magic_len + payload_len + 3);
	memcpy(quoted, PG_PLSQL_WRAP_MAGIC, magic_len);
	/* SQL quoting prevents long base64 text from being lexed as an identifier. */
	quoted[magic_len] = '\'';
	memcpy(quoted + magic_len + 1, payload, payload_len);
	quoted[magic_len + payload_len + 1] = '\'';
	quoted[magic_len + payload_len + 2] = '\0';
	return quoted;
}

/* Decrypt source with the cipher identified by the wrapped payload prefix. */
char *
pg_plsql_wrap_decrypt(const char *payload, size_t payload_len,
					  PgPlsqlWrapCipher cipher_type,
					  const char **error_message)
{
	const char *prefix;
	const unsigned char *key;
	int			iv_len;
	size_t		prefix_len;
	char	   *decoded;
	unsigned char *plaintext;
#if defined(USE_GMSSL) || defined(USE_OPENSSL)
	const unsigned char *iv;
	const unsigned char *ciphertext;
#endif
	int			decoded_len;
	int			cipher_len;

	*error_message = NULL;
	if (!pg_plsql_wrap_cipher_parameters(cipher_type, &prefix, &key, &iv_len))
	{
		*error_message = "invalid PL/SQL source wrapping cipher";
		return NULL;
	}

	prefix_len = strlen(prefix);
	if (payload_len >= prefix_len && strncmp(payload, prefix, prefix_len) == 0)
	{
		payload += prefix_len;
		payload_len -= prefix_len;
	}
	if (payload_len == 0 || payload_len > INT32_MAX)
	{
		*error_message = "invalid wrapped PL/SQL source";
		return NULL;
	}

	decoded = (char *) PLSQL_WRAP_ALLOC(pg_b64_dec_len((int) payload_len));
	decoded_len = pg_b64_decode(payload, (int) payload_len, decoded,
								 pg_b64_dec_len((int) payload_len));
	if (decoded_len <= iv_len)
	{
		PLSQL_WRAP_FREE(decoded);
		*error_message = "invalid wrapped PL/SQL source";
		return NULL;
	}

	cipher_len = decoded_len - iv_len;
	plaintext = (unsigned char *) PLSQL_WRAP_ALLOC(cipher_len + 1);
#if defined(USE_GMSSL) || defined(USE_OPENSSL)
	iv = (const unsigned char *) decoded;
	ciphertext = (const unsigned char *) decoded + iv_len;
#endif
#ifdef USE_GMSSL
	{
		unsigned char ctr[PG_PLSQL_WRAP_AES_IV_LEN];

		memcpy(ctr, iv, sizeof(ctr));
		if (cipher_type == PG_PLSQL_WRAP_CIPHER_AES)
		{
			AES_KEY		aes_key;

			/* CTR decrypts with the encryption key schedule. */
			if (aes_set_encrypt_key(&aes_key, key, AES256_KEY_SIZE) != 1)
			{
				PLSQL_WRAP_FREE(decoded);
				PLSQL_WRAP_FREE(plaintext);
				*error_message = "could not initialize GmSSL AES key";
				return NULL;
			}
			aes_ctr_decrypt(&aes_key, ctr, ciphertext, cipher_len, plaintext);
		}
		else
		{
			SM4_KEY		sm4_key;

			/* CTR decrypts with the encryption key schedule. */
			sm4_set_encrypt_key(&sm4_key, key);
			sm4_ctr_decrypt(&sm4_key, ctr, ciphertext, cipher_len, plaintext);
		}
	}
#elif defined(USE_OPENSSL)
	{
		const EVP_CIPHER *cipher = pg_plsql_wrap_evp_cipher(cipher_type);
		EVP_CIPHER_CTX *ctx;
		int			out_len = 0;
		int			final_len = 0;

		if (cipher == NULL)
		{
			PLSQL_WRAP_FREE(decoded);
			PLSQL_WRAP_FREE(plaintext);
			*error_message = "SM4-CTR is not available in this OpenSSL build";
			return NULL;
		}
		ctx = EVP_CIPHER_CTX_new();
		if (ctx == NULL ||
			EVP_DecryptInit_ex(ctx, cipher, NULL, key, iv) != 1 ||
			EVP_DecryptUpdate(ctx, plaintext, &out_len, ciphertext, cipher_len) != 1 ||
			EVP_DecryptFinal_ex(ctx, plaintext + out_len, &final_len) != 1 ||
			out_len + final_len != cipher_len)
		{
			if (ctx != NULL)
				EVP_CIPHER_CTX_free(ctx);
			PLSQL_WRAP_FREE(decoded);
			PLSQL_WRAP_FREE(plaintext);
			*error_message = "invalid wrapped PL/SQL source";
			return NULL;
		}
		EVP_CIPHER_CTX_free(ctx);
	}
#else
	PLSQL_WRAP_FREE(decoded);
	PLSQL_WRAP_FREE(plaintext);
	*error_message = "PL/SQL source wrapping requires GmSSL or OpenSSL";
	return NULL;
#endif

	PLSQL_WRAP_FREE(decoded);
	plaintext[cipher_len] = '\0';
	return (char *) plaintext;
}
/* End - Reqid:SRS-PLPGSQL-HIDDEN*/
