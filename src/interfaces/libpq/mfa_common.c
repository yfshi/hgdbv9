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
 *		src/interfaces/mfa_common.c
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
#endif

#ifdef USE_GMSSL
#include <gmssl/gm_sm3.h>
#include <gmssl/gm_sm2.h>
#include <gmssl/base64.h>
#endif

#include "common/base64.h"
#include "common/string.h"
#include "mfa_common.h"

#if defined(USE_OPENSSL) || defined(USE_GMSSL)
static char *tmp_pass = "123456";
#endif

#ifdef USE_GMSSL
static int
load_encrypted_sm2_private_key(const char *filename, SM2_KEY *key) {
	FILE *fp = fopen(filename, "r");
	if (!fp)
		return -1;

	if (sm2_private_key_info_decrypt_from_pem(key, tmp_pass, fp) != 1)
	{
		char *client_pass = simple_prompt("Enter PEM pass phrase: ", false);
		if (!client_pass)
		{
			fclose(fp);
			return -1;
		}
		if (sm2_private_key_info_decrypt_from_pem(key, client_pass, fp) != 1)
		{
			fclose(fp);
			return -1;
		}
		free(client_pass);
	}

	fclose(fp);

	return 1;
}

static int
load_sm2_private_key_from_string(const uint8_t *pem_str, size_t *inlen, SM2_KEY *key) {
	const uint8_t *attrs = NULL;
	size_t attrs_len = 0;
	size_t inlen_b = *inlen;
	const uint8_t *p = pem_str;

	if(sm2_private_key_info_decrypt_from_der(key, &attrs, &attrs_len, tmp_pass, &p, inlen) != 1)
	{
		char *client_pass = simple_prompt("Enter PEM pass phrase: ", false);
		if (!client_pass)
		{
			return -1;
		}
		p = pem_str;
		if (sm2_private_key_info_decrypt_from_der(key, &attrs, &attrs_len, client_pass, &p, &inlen_b) != 1)
		{
			return -1;
		}
		free(client_pass);
	}

	return 1;
}
#endif

#ifdef USE_OPENSSL

/*
 * Sign the message using the client's private key(rsa)
 */
static unsigned char *
pkey_sign_openssl(const char *privateKeyBase64, const char *message, size_t *sig_len)
{
	unsigned char *sig = NULL;
	BIO *bio_mem = NULL;
	EVP_PKEY *pkey = NULL;
	EVP_MD_CTX *mctx = NULL;
	int pkey_type = EVP_PKEY_NONE;
	int privateKeyDER_len;

	int inlen = strlen(privateKeyBase64);
	unsigned char *privateKeyDER = palloc0(inlen);

	if (!privateKeyDER)
		return NULL;
	
	privateKeyDER_len = pg_b64_decode(privateKeyBase64, inlen,
										(char *)privateKeyDER, inlen);

	if (privateKeyDER_len <= 1)
	{
		pfree(privateKeyDER);
		return NULL;
	}

	bio_mem = BIO_new(BIO_s_mem());
	BIO_write(bio_mem, privateKeyDER, privateKeyDER_len);
	pkey = d2i_PrivateKey_bio(bio_mem, NULL);
	BIO_free_all(bio_mem);
	pfree(privateKeyDER);

	if (pkey == NULL)
		goto end;

	mctx = EVP_MD_CTX_create();
	if (!mctx)
		goto end;

	pkey_type = EVP_PKEY_id(pkey);
	if (pkey_type == EVP_PKEY_RSA)
		EVP_DigestSignInit(mctx, NULL, EVP_sha256(), NULL, pkey);
	else
		goto end;

	EVP_DigestSignUpdate(mctx, message, 16);

	EVP_DigestSignFinal(mctx, NULL, sig_len);
	sig = malloc(*sig_len + 1);
	if (!sig)
		goto end;

	EVP_DigestSignFinal(mctx, sig, sig_len);

	EVP_MD_CTX_destroy(mctx);
	EVP_PKEY_free(pkey);
	return sig;
end:
	if (mctx)
		EVP_MD_CTX_destroy(mctx);
	if (pkey)
		EVP_PKEY_free(pkey);
	if (sig)
		free(sig);
	return NULL;
}
#endif

#ifdef USE_GMSSL

#define MAX_DER_SIZE 512
/*
 * Sign the message using the client's private key(sm2)
 */
static unsigned char *
pkey_sign_gmssl(const char *privateKeyBase64,
								const char *message, size_t *sig_len)
{
	int ret = -1;
	SM2_KEY sign_key;
	uint8_t priv_der[MAX_DER_SIZE];
	size_t priv_derlen = sizeof(priv_der);

	uint8_t dgst[SM3_DIGEST_SIZE];
	SM3_CTX sm3;
	unsigned char *sig = NULL;

	const uint8_t *p = priv_der;
	size_t plen = 0;

	priv_derlen = base64_decode_block(priv_der, (uint8_t *)privateKeyBase64,
										strlen(privateKeyBase64));
	if (priv_derlen <= 1) {
		goto end;
	}

	plen = priv_derlen;
	ret = load_sm2_private_key_from_string(p, &plen, &sign_key);
	if (ret != 1) {
		goto end;
	}

	gm_sm3_init(&sm3);
	gm_sm3_update(&sm3, (const uint8_t *)message, 16);
	gm_sm3_finish(&sm3, dgst);

	sig = (unsigned char *)malloc(256);
	if (!sig)
		goto end;

	if (gm_sm2_sign(&sign_key, dgst, sig, sig_len) != 1)
		goto end;

	return sig;

end:
	if (sig)
		free(sig);

	return NULL;
}
#endif

/*
 * Select different signature functions based on the type(rsa or sm2) of private key
 */
unsigned char *
pq_pkey_sign(const char *prikey, char *opt_salt, size_t *sig_len)
{
	unsigned char *sig = NULL;
#ifdef USE_OPENSSL
	/* rsa */
		sig = pkey_sign_openssl(prikey, opt_salt, sig_len);
#endif
#ifdef USE_GMSSL
	/* sm2 */
	if (sig == NULL)
		sig = pkey_sign_gmssl(prikey, opt_salt, sig_len);
#endif
	return sig;
}

#ifdef USE_OPENSSL

static int
pem_passwd_cb(char *buf, int size, int rwflag, void *userdata)
{
    const char *password = (const char *)userdata;
    int len = (int)strlen(password);
    if (len > size) {
        len = size;
    }
    memcpy(buf, password, len);
    return len;
}

/*
 * Sign the message using the client's private key file(rsa)
 */
static unsigned char *
cert_sign_openssl(const char *privateKeyFile, const char *optsalt, size_t *sig_len)
{

	unsigned char *sig = NULL;
	EVP_PKEY *pkey = NULL;
	EVP_MD_CTX *mctx = NULL;
	int pkey_type = EVP_PKEY_RSA;

	FILE *privKeyFile = fopen(privateKeyFile, "r");
	if (!privKeyFile)
		goto cleanup;

	pkey = PEM_read_PrivateKey(privKeyFile, NULL, pem_passwd_cb, (void *)tmp_pass);
	fclose(privKeyFile);
	if (!pkey)
		goto cleanup;

	mctx = EVP_MD_CTX_create();
	if (!mctx)
		goto cleanup;

	pkey_type = EVP_PKEY_id(pkey);
	if (pkey_type == EVP_PKEY_RSA)
	{
		if (EVP_DigestSignInit(mctx, NULL, EVP_sha256(), NULL, pkey) <= 0)
			goto cleanup;
	}
	else
	{
		goto cleanup;
	}

	EVP_DigestSignUpdate(mctx, optsalt, 16);

	EVP_DigestSignFinal(mctx, NULL, sig_len);
	sig = malloc(*sig_len);
	if (!sig)
		goto cleanup;

	EVP_DigestSignFinal(mctx, sig, sig_len);

cleanup:
	if (mctx)
		EVP_MD_CTX_destroy(mctx);
	if (pkey)
		EVP_PKEY_free(pkey);
	return sig;
}
#endif

#ifdef USE_GMSSL

/*
 * Sign the message using the client's private key file(sm2)
 */
static unsigned char *
cert_sign_gmssl(const char *privateKeyFile, const char *optsalt, size_t *sig_len)
{
	SM2_KEY client_key;
	uint8_t dgst[32];
	unsigned char *sig = NULL;

	if (load_encrypted_sm2_private_key(privateKeyFile, &client_key) != 1)
		goto end;

	sm3_digest((const uint8_t *)optsalt, 16, dgst);

	sig = (unsigned char *)malloc(256);
	if (!sig)
		goto end;

	if (gm_sm2_sign(&client_key, dgst, sig, sig_len) != 1)
		goto end;

end:

	return sig;
}
#endif

unsigned char *
pq_cert_sign(const char *privateKeyFile, const char *optsalt, size_t *sig_len)
{
	unsigned char *sig = NULL;

#ifdef USE_OPENSSL
	sig = cert_sign_openssl(privateKeyFile, optsalt, sig_len);
#endif
#ifdef USE_GMSSL
	if (sig == NULL)
		sig = cert_sign_gmssl(privateKeyFile, optsalt, sig_len);
#endif
	return sig;
}
char *pq_get_cert(const char *file, char *opt_salt, size_t *cert_len)
{
	int n;
	char *buffer = NULL;

	FILE *srcFile = fopen(file, "rb");
	if (srcFile == NULL)
		return NULL;

	*cert_len = 0;
	buffer = (char *)malloc(2048);
	if(!buffer)
	{
		fclose(srcFile);
		return NULL;
	}

	memset(buffer, 0, 2048);
	n = fread(buffer + *cert_len, 1, 2048, srcFile);
	while (*cert_len  < 2048 && n > 0)
	{
		*cert_len += n;
		n = fread(buffer + *cert_len, 1, 2048, srcFile);
	}

	fclose(srcFile);
	return buffer;
}
