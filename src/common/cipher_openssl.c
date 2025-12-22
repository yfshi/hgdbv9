/*-------------------------------------------------------------------------
 * cipher_openssl.c
 *		Cryptographic function using OpenSSL
 *
 * This contains the common low-level functions needed in both frontend and
 * backend, for implement the database encryption.
 *
 * Portions Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  src/common/cipher_openssl.c
 *
 * add the file for requirement "SRS-SECURITY-TDE"
 *-------------------------------------------------------------------------
 */
#ifndef FRONTEND
#include "postgres.h"
#else
#include "postgres_fe.h"
#endif

#include "common/cipher.h"
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>

#ifdef USE_GMSSL
#include <gmssl/sm4.h>
#endif

/*
 * prototype for the EVP functions that return an algorithm, e.g.
 * EVP_aes_128_ctr().
 */
typedef const EVP_CIPHER *(*ossl_EVP_cipher_func) (void);

static EVP_CIPHER_CTX *ossl_cipher_ctx_create(int cipher, unsigned char *key,
											  int klen, bool enc);

#ifdef USE_GMSSL
/*
 * Referring to the sm4_ctr_encrypt_init in GmSSL-3.1.1,
 * split it into two functions.
 */
static void sm4_ctr_encrypt_init_key(SM4_CTR_CTX *ctx, const uint8_t key[SM4_BLOCK_SIZE])
{
	sm4_set_encrypt_key(&ctx->sm4_key, key);
	return;
}

static int sm4_ctr_encrypt_init_iv(SM4_CTR_CTX *ctx, const uint8_t ctr[SM4_BLOCK_SIZE])
{
	memcpy(ctx->ctr, ctr, SM4_BLOCK_SIZE);
	memset(ctx->block, 0, SM4_BLOCK_SIZE);
	ctx->block_nbytes = 0;
	return 1;
}

#define sm4_ctr_decrypt_init_iv(ctx, ctr) sm4_ctr_encrypt_init_iv(ctx, ctr)
#endif

/*
 * Return a newly created cipher context.  'cipher' specifies cipher algorithm
 * by identifier like PG_CIPHER_XXX.
 */
PgCipherCtx *
pg_cipher_ctx_create(int cipher, unsigned char *key, int klen, bool enc)
{
	PgCipherCtx *ctx = NULL;

	if (cipher > PG_MAX_CIPHER_ID)
		return NULL;

	ctx = malloc(sizeof(PgCipherCtx));
	ctx->cipher = cipher;

#ifdef USE_GMSSL
	if (cipher == PG_CIPHER_SM4_CTR)
	{
		SM4_CTR_CTX *sm4_ctx = malloc(sizeof(SM4_CTR_CTX));
		sm4_ctr_encrypt_init_key(sm4_ctx, (const uint8_t *)key);
		ctx->ctx = (void*)sm4_ctx;
	}
	else
	{
#endif
	ctx->ctx = ossl_cipher_ctx_create(cipher, key, klen, enc);
#ifdef USE_GMSSL
	}
#endif

	return ctx;
}

void
pg_cipher_ctx_free(PgCipherCtx *ctx)
{
#ifdef USE_GMSSL
	if (ctx->cipher == PG_CIPHER_SM4_CTR)
	{
		free(ctx->ctx);
	}
	else
	{
#endif		
	EVP_CIPHER_CTX_free((EVP_CIPHER_CTX *)ctx->ctx);
#ifdef USE_GMSSL
	}
#endif
	free(ctx);
}

int
pg_cipher_blocksize(PgCipherCtx *ctx)
{
	Assert(ctx && ctx->ctx);

#ifdef USE_GMSSL
	if (ctx->cipher == PG_CIPHER_SM4_CTR)
	{
		return SM4_BLOCK_SIZE;
	}
	else
	{
#endif
	return EVP_CIPHER_CTX_block_size((EVP_CIPHER_CTX *)ctx->ctx);
#ifdef USE_GMSSL
	}
#endif
}

/*
 * Encryption routine to encrypt data provided.
 *
 * ctx is the encryption context which must have been created previously.
 *
 * plaintext is the data we are going to encrypt
 * inlen is the length of the data to encrypt
 *
 * ciphertext is the encrypted result
 * outlen is the encrypted length
 *
 * iv is the IV to use.
 * ivlen is the IV length to use.
 *
 * outtag is the resulting tag.
 * taglen is the length of the tag.
 */
bool
pg_cipher_encrypt(PgCipherCtx *ctx, int cipher,
				  const unsigned char *plaintext, const int inlen,
				  unsigned char *ciphertext, int *outlen,
				  const unsigned char *iv, const int ivlen)
{
	size_t			len = 0;
	size_t			enclen = 0;
	unsigned char  *restext = ciphertext;

	Assert(ctx != NULL && ctx->ctx != NULL);

	if (ciphertext == plaintext)
		restext = malloc(inlen);

#ifdef USE_GMSSL
	if (ctx->cipher == PG_CIPHER_SM4_CTR)
	{
		if (1 != sm4_ctr_encrypt_init_iv((SM4_CTR_CTX *)ctx->ctx, (const uint8_t *)iv))
			return false;

		if (1 != sm4_ctr_encrypt_update((SM4_CTR_CTX *)ctx->ctx, 
									(const uint8_t *)plaintext, (const size_t)inlen, 
									(uint8_t *)restext, (size_t *)&len))
			return false;

		enclen = len;

		if (1 != sm4_ctr_encrypt_finish((SM4_CTR_CTX *)ctx->ctx, 
									(uint8_t *)restext + enclen, 
									(size_t *)&len))
			return false;

		*outlen = enclen + len;
		if (restext != ciphertext)
			memcpy(ciphertext, restext, *outlen);
	}
	else
	{
#endif
	/*
	 * Here we are setting the IV for the context which was passed in.  Note
	 * that we signal to OpenSSL that we are configuring a new value for the
	 * context by passing in 'NULL' for the 2nd ('type') parameter.
	 */

	/* Set the IV for this encryption. */
	if (!EVP_EncryptInit_ex((EVP_CIPHER_CTX *)ctx->ctx, NULL, NULL, NULL, iv))
		return false;

	/*
	 * This is the function which is actually performing the encryption for
	 * us.
	 */
	if (!EVP_EncryptUpdate((EVP_CIPHER_CTX *)ctx->ctx, restext, (int *)&len, plaintext, inlen))
		return false;

	enclen = len;

	/* Finalize the encryption, which could add more to output. */
	if (!EVP_EncryptFinal_ex((EVP_CIPHER_CTX *)ctx->ctx, restext + enclen, (int *)&len))
		return false;

	*outlen = enclen + len;
	if (restext != ciphertext)
		memcpy(ciphertext, restext, *outlen);
#ifdef USE_GMSSL
	}
#endif

	if (restext != ciphertext)
		free(restext);
	return true;
}

/*
 * Decryption routine
 *
 * ctx is the encryption context which must have been created previously.
 *
 * ciphertext is the data we are going to decrypt
 * inlen is the length of the data to decrypt
 *
 * plaintext is the decrypted result
 * outlen is the decrypted length
 *
 * iv is the IV to use.
 * ivlen is the length of the IV.
 *
 * intag is the tag to use to verify.
 * taglen is the length of the tag.
 */
bool
pg_cipher_decrypt(PgCipherCtx *ctx, const int cipher,
				  const unsigned char *ciphertext, const int inlen,
				  unsigned char *plaintext, int *outlen,
				  const unsigned char *iv, const int ivlen)
{
	size_t			declen = 0;
	size_t			len = 0;
	unsigned char  *restext = plaintext;

	Assert(ctx != NULL && ctx->ctx != NULL);

	if (ciphertext == plaintext)
		restext = malloc(inlen);

#ifdef USE_GMSSL
	if (ctx->cipher == PG_CIPHER_SM4_CTR)
	{
		if (1 != sm4_ctr_decrypt_init_iv((SM4_CTR_CTX *)ctx->ctx, (const uint8_t *)iv))
			return false;

		if (1 != sm4_ctr_decrypt_update((SM4_CTR_CTX *)ctx->ctx, 
									(const uint8_t *)ciphertext, (const size_t)inlen, 
									(uint8_t *)restext, (size_t *)&len))
			return false;

		declen = len;

		if (1 != sm4_ctr_decrypt_finish((SM4_CTR_CTX *)ctx->ctx, 
									(uint8_t *)restext + declen, 
									(size_t *)&len))
			return false;

		*outlen = declen + len;
		if (restext != plaintext)
			memcpy(plaintext, restext, *outlen);
	}
	else
	{
#endif

	/*
	 * Here we are setting the IV for the context which was passed in.  Note
	 * that we signal to OpenSSL that we are configuring a new value for the
	 * context by passing in 'NULL' for the 2nd ('type') parameter.
	 */

	/* Set the IV for this decryption. */
	if (!EVP_DecryptInit_ex((EVP_CIPHER_CTX *)ctx->ctx, NULL, NULL, NULL, iv))
		return false;

	/*
	 * This is the function which is actually performing the decryption for
	 * us.
	 */
	if (!EVP_DecryptUpdate((EVP_CIPHER_CTX *)ctx->ctx, restext, (int *)&len, ciphertext, inlen))
		return false;

	declen = len;

	/*
	 * Finalize the decryption, which could add more to output, this is also
	 * the step which checks the tag and we MUST fail if this indicates an
	 * invalid tag!
	 */
	if (!EVP_DecryptFinal_ex((EVP_CIPHER_CTX *)ctx->ctx, restext + declen, (int *)&len))
		return false;

	*outlen = declen + len;
	if (restext != plaintext)
		memcpy(plaintext, restext, *outlen);
#ifdef USE_GMSSL
	}
#endif

	if (restext != plaintext)
		free(restext);

	return true;
}

/*
 * Routine to perform key wrapping for data provided.
 *
 * ctx is the encryption context which must have been created previously.
 *
 * plaintext is the data/key we are going to encrypt/wrap
 * inlen is the length of the data
 *
 * ciphertext is the wrapped result
 * outlen is the encrypted length (will be larger than input!)
 */
bool
pg_cipher_keywrap(PgCipherCtx *ctx,
				  const unsigned char *plaintext, const int inlen,
				  unsigned char *ciphertext, int *outlen)
{
	int			len;
	int			enclen;

	Assert(ctx != NULL && ctx->ctx != NULL);

	/*
	 * This is the function which is actually performing the encryption for
	 * us.
	 */
	if (!EVP_EncryptUpdate((EVP_CIPHER_CTX *)ctx->ctx, ciphertext, &len, plaintext, inlen))
		return false;

	enclen = len;

	/* Finalize the encryption, which could add more to output. */
	if (!EVP_EncryptFinal_ex((EVP_CIPHER_CTX *)ctx->ctx, ciphertext + enclen, &len))
		return false;

	*outlen = enclen + len;

	return true;
}

/*
 * Routine to perform key unwrapping of the data provided.
 *
 * ctx is the encryption context which must have been created previously.
 *
 * ciphertext is the wrapped key we are going to unwrap
 * inlen is the length of the data to decrypt/unwrap
 *
 * plaintext is the decrypted result
 * outlen is the decrypted length (will be smaller than input!)
 */
bool
pg_cipher_keyunwrap(PgCipherCtx *ctx,
					const unsigned char *ciphertext, const int inlen,
					unsigned char *plaintext, int *outlen)
{
	unsigned char *in = palloc(inlen);
	int			declen;
	int			len;

	/*
	 * This is the function which is actually performing the decryption for
	 * us.
	 */
	if (!EVP_DecryptUpdate((EVP_CIPHER_CTX *)ctx->ctx, in, &len, ciphertext, inlen))
		return false;

	declen = len;

	/*
	 * Finalize the decryption, which could add more to output, this is also
	 * the step which checks the tag and we MUST fail if this indicates an
	 * invalid result!
	 */
	if (!EVP_DecryptFinal_ex((EVP_CIPHER_CTX *)ctx->ctx, in + declen, &len))
		return false;

	*outlen = declen + len;
	memcpy(plaintext, in, *outlen);

	return true;
}

/*
 * Returns the correct KWP cipher functions for OpenSSL based
 * on the key length requested.
 */
static ossl_EVP_cipher_func
get_evp_aes_kwp(int klen)
{
	switch (klen)
	{
		case PG_AES128_KEY_LEN:
			return EVP_aes_128_wrap_pad;
		case PG_AES192_KEY_LEN:
			return EVP_aes_192_wrap_pad;
		case PG_AES256_KEY_LEN:
			return EVP_aes_256_wrap_pad;
		default:
			return NULL;
	}
}

/*
 * Returns the correct CTR cipher functions for OpenSSL based
 * on the key length requested.
 */
static ossl_EVP_cipher_func
get_evp_aes_ctr(int klen)
{
	switch (klen)
	{
		case PG_AES128_KEY_LEN:
			return EVP_aes_128_ctr;
		case PG_AES192_KEY_LEN:
			return EVP_aes_192_ctr;
		case PG_AES256_KEY_LEN:
			return EVP_aes_256_ctr;
		default:
			return NULL;
	}
}

/*
 * Initialize and return an EVP_CIPHER_CTX. Returns NULL if the given
 * cipher algorithm is not supported or on failure.
 */
static EVP_CIPHER_CTX *
ossl_cipher_ctx_create(int cipher, unsigned char *key, int klen, bool enc)
{
	EVP_CIPHER_CTX *ctx;
	ossl_EVP_cipher_func func;
	int			ret;

	ctx = EVP_CIPHER_CTX_new();

	/*
	 * We currently only support AES SM4.
	 */
	switch (cipher)
	{
		case PG_CIPHER_AES_KWP:
			func = get_evp_aes_kwp(klen);

			/*
			 * Since wrapping will produce more output then input, and we have
			 * to be ready for that, OpenSSL requires that we explicitly
			 * enable wrapping for the context.
			 */
			EVP_CIPHER_CTX_set_flags(ctx, EVP_CIPHER_CTX_FLAG_WRAP_ALLOW);
			if (!func)
				goto failed;
			break;
		case PG_CIPHER_AES_CTR:
			func = get_evp_aes_ctr(klen);
			if (!func)
				goto failed;
			break;
		case PG_CIPHER_SM4_CTR:
			{
#ifdef HAVE_EVP_SM4_CTR
				func = EVP_sm4_ctr;
#else
				func = NULL;
#ifndef FRONTEND
				ereport(ERROR,
						(errcode(ERRCODE_CONFIG_FILE_ERROR),
						(errmsg("the SM4 encryption algorithm is requires the GmSSL or OpenSSL version 1.1.1w or higher"))));
#else
				fprintf(stderr, _("the SM4 encryption algorithm is requires the GmSSL or OpenSSL version 1.1.1w or higher"));
				exit(1);
#endif
#endif
				if (!func)
					goto failed;
				break;
			}
		default:
			goto failed;
	}

	/*
	 * We create the context here based on the cipher requested and the
	 * provided key.  Note that the IV will be provided in the actual
	 * encryption call through another EVP_EncryptInit_ex call- this is fine
	 * as long as 'type' is passed in as NULL!
	 */
	if (enc)
		ret = EVP_EncryptInit_ex(ctx, (const EVP_CIPHER *) func(), NULL, key, NULL);
	else
		ret = EVP_DecryptInit_ex(ctx, (const EVP_CIPHER *) func(), NULL, key, NULL);

	if (!ret)
		goto failed;

	/* Set the key length based on the key length requested. */
	if (!EVP_CIPHER_CTX_set_key_length(ctx, klen))
		goto failed;

	return ctx;

failed:
	EVP_CIPHER_CTX_free(ctx);
	return NULL;
}

