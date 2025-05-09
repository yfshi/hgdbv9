/* ---------------------------------------------------------------------
 *
 * File: mfa_common.c
 *
 * Abstract:
 *		This file contains the function definitions used by the front end
 *		for multi-factor authentication.
 *
 * Author: suzy@highgo.com, 20241220.
 *
 * Copyright:
 * Copyright (c) 2024, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *		src/common/mfa_common.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_OPENSSL
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/opensslv.h>

#include "common/mfa.h"

char *pg_base64_encode(const unsigned char *input, int length)
{
	BIO *bio, *b64;
	BUF_MEM *bufferPtr;
	char *output = NULL;

	b64 = BIO_new(BIO_f_base64());
	bio = BIO_new(BIO_s_mem());
	bio = BIO_push(b64, bio);
	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); // no new line

	BIO_write(bio, input, length);
	(void)BIO_flush(bio);
	BIO_get_mem_ptr(bio, &bufferPtr);

	output = (char *)malloc(bufferPtr->length + 1);
	memcpy(output, bufferPtr->data, bufferPtr->length);
	output[bufferPtr->length] = '\0'; // null-terminate the string

	BIO_free_all(bio);
	return output;
}

unsigned char *pg_base64_decode(const char *input, int *length)
{
	BIO *bio, *b64;
	int input_len = strlen(input);
	unsigned char *buffer = (unsigned char *)malloc(input_len);

	b64 = BIO_new(BIO_f_base64());
	bio = BIO_new_mem_buf(input, input_len);
	bio = BIO_push(b64, bio);
	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); // no new line

	*length = BIO_read(bio, buffer, input_len);
	BIO_free_all(bio);

	return buffer;
}

static char *SM2_sign(const char *privateKeyBase64, const char *message, size_t *sig_len)
{
	char *sig = NULL;
	BIO *bio_mem = NULL;
	EVP_PKEY *pkey = NULL;
	EVP_MD_CTX *mctx = NULL;
	int pkey_type = EVP_PKEY_NONE;
	int privateKeyDER_len;
	unsigned char *privateKeyDER = pg_base64_decode(privateKeyBase64, &privateKeyDER_len);

	bio_mem = BIO_new(BIO_s_mem());
	BIO_write(bio_mem, privateKeyDER, privateKeyDER_len);
	pkey = d2i_PrivateKey_bio(bio_mem, NULL);
	BIO_free_all(bio_mem);
	free(privateKeyDER);

	if (pkey == NULL)
	{
		return NULL;
	}

	mctx = EVP_MD_CTX_create();

	pkey_type = EVP_PKEY_id(pkey);
	if (pkey_type == EVP_PKEY_RSA)
	{
		EVP_DigestSignInit(mctx, NULL, EVP_sha256(), NULL, pkey);
	}
	else
	{
#ifdef OPENSSL_VERSION_NUMBER
#if OPENSSL_VERSION_NUMBER < 0x10100000L
		return NULL;
#elif OPENSSL_VERSION_NUMBER < 0x30000000L
		EVP_PKEY_set_alias_type(pkey, EVP_PKEY_SM2);
#endif
#if OPENSSL_VERSION_NUMBER > 0x10100000L
		EVP_DigestSignInit(mctx, NULL, EVP_sm3(), NULL, pkey);
#endif
#endif
	}

	EVP_DigestSignUpdate(mctx, message, 16);

	EVP_DigestSignFinal(mctx, NULL, sig_len);
	sig = (char *)malloc(*sig_len + 1);
	EVP_DigestSignFinal(mctx, (unsigned char *)sig, sig_len);

	EVP_MD_CTX_destroy(mctx);
	EVP_PKEY_free(pkey);
	return (char *)sig;
}

char *pg_pkey_sign(const char *prikey, char *opt_salt, size_t *sig_len)
{
	return SM2_sign(prikey, opt_salt, sig_len);
}

static char *cert_sign_sm2(const char *privateKeyFile, const char *optsalt, size_t *sig_len)
{

	char *sig = NULL;
	EVP_PKEY *pkey = NULL;
	EVP_PKEY_CTX *ctx = NULL;
	EVP_MD_CTX *mctx = NULL;
	int pkey_type = EVP_PKEY_RSA;
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	const char *sm2_id = "1234567812345678"; // SM2 ID
#endif
	FILE *privKeyFile = fopen(privateKeyFile, "r");
	if (!privKeyFile)
	{
		fprintf(stderr, "Error: Unable to open private key file %s\n", privateKeyFile);
		return NULL;
	}

	pkey = PEM_read_PrivateKey(privKeyFile, NULL, NULL, NULL);
	fclose(privKeyFile);
	if (pkey == NULL)
	{
		fprintf(stderr, "Error: Unable to read private key from file. %s\n", privateKeyFile);
		return NULL;
	}

	mctx = EVP_MD_CTX_create();
	if (!mctx)
	{
		fprintf(stderr, "Error: Failed to allocate EVP_MD_CTX.\n");
		goto cleanup;
	}

	pkey_type = EVP_PKEY_id(pkey);
#if defined(HAVE_X509_GET_SIGNATURE_INFO)
	if (pkey_type == EVP_PKEY_SM2)
	{
#if OPENSSL_VERSION_NUMBER < 0x30000000L
		EVP_PKEY_set_alias_type(pkey, EVP_PKEY_SM2);
#endif
		ctx = EVP_PKEY_CTX_new(pkey, NULL);

		if (EVP_PKEY_CTX_set1_id(ctx, sm2_id, strlen((char *)sm2_id)) <= 0)
		{
			fprintf(stderr, "Error: Failed to set SM2 ID.\n");
			// ERR_print_errors_fp(stderr);
			goto cleanup;
		}

		EVP_MD_CTX_set_pkey_ctx(mctx, ctx);
		if (EVP_DigestSignInit(mctx, NULL, EVP_sm3(), NULL, pkey) <= 0)
		{
			fprintf(stderr, "Error: EVP_DigestSignInit failed for SM2/EC.\n");
			goto cleanup;
		}
	}
	else
#endif
		if (pkey_type == EVP_PKEY_RSA || pkey_type == EVP_PKEY_EC)
	{
		if (EVP_DigestSignInit(mctx, NULL, EVP_sha256(), NULL, pkey) <= 0)
		{
			fprintf(stderr, "Error: EVP_DigestSignInit failed for RSA.\n");
			goto cleanup;
		}
	}
	else
	{
		fprintf(stderr, "Error: Unsupported private key type.\n");
		goto cleanup;
	}

	EVP_DigestSignUpdate(mctx, optsalt, 16);

	EVP_DigestSignFinal(mctx, NULL, sig_len);
	sig = (char *)malloc(*sig_len);
	EVP_DigestSignFinal(mctx, (unsigned char *)sig, sig_len);

cleanup:
	if (mctx)
#if OPENSSL_VERSION_NUMBER < 0x10100000L
		EVP_MD_CTX_cleanup(mctx);
#else
		EVP_MD_CTX_free(mctx);
#endif
	if (ctx)
		EVP_PKEY_CTX_free(ctx);
	if (pkey)
		EVP_PKEY_free(pkey);
	return sig;
}

static char *cert_sign(const char *privateKeyFile, const char *optsalt, size_t *sig_len)
{
	return cert_sign_sm2(privateKeyFile, optsalt, sig_len);
}
char *pg_get_cert(const char *file, char *opt_salt, size_t *cert_len)
{
	int n;
	char *buffer = NULL;

	FILE *srcFile = fopen(file, "rb");
	if (srcFile == NULL)
	{
		fprintf(stderr, "Error: Unable to open source file %s\n", file);
		return NULL;
	}
	*cert_len = 0;
	buffer = (char *)malloc(1024);
	memset(buffer, 0, 1024);
	while ((n = fread(buffer + *cert_len, 1, 1024, srcFile)) > 0)
	{
		*cert_len += n;
	}
	return buffer;
}

char *pg_cert_sign(const char *file, char *opt_salt, size_t *sig_len)
{
	char *sig = NULL;
	sig = cert_sign(file, opt_salt, sig_len);

	return sig;
}
#endif