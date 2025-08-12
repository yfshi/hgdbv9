/* ---------------------------------------------------------------------
 *
 * File: mfa.c
 *
 * Abstract:
 *		This file contains the two-factor authentication function.
 *
 * Author: suzy@highgo.com, 20241220.
 *
 * Copyright:
 * Copyright (c) 2024, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *		src/backend/libpq/mfa.c
 *
 * Add files that require "SRS-SECURITY-IDENTITYAUTH"
 *
 * ---------------------------------------------------------------------
 */

#include "postgres.h"

#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#ifdef _WIN32
#include <string.h> // _strnicmp
#else
#include <strings.h> // strncasecmp
#endif

#include "libpq/mfa.h"
#include "common/hmac.h"
#include "common/sha1.h"
#include "common/base64.h"
#include "access/htup.h"
#include "utils/syscache.h"
#include "catalog/pg_authid_d.h"
#include "catalog/pg_authid.h"
#include "utils/ora_compatible.h"
#include "utils/guc.h"
#include "utils/relcache.h"
#include "miscadmin.h"
#include "storage/lockdefs.h"
#include "utils/builtins.h"
#include "utils/rel.h"
#include "access/table.h"
#include "catalog/indexing.h"
#include "catalog/pg_object.h"
#include "catalog/pg_object_d.h"
#include "catalog/pg_mfaid.h"
#include "catalog/pg_mfaid_d.h"
#include "utils/jsonb.h"
#include "utils/acl.h"
#include "catalog/objectaddress.h"
#include "utils/timestamp.h"
#include "access/heapam.h"
#include "se_features_interface.h"

#ifdef USE_OPENSSL
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#endif

#ifdef USE_GMSSL
#include <gmssl/gm_sm3.h>
#include <gmssl/gm_sm2.h>
#include <gmssl/base64.h>
#include <gmssl/x509.h>
#include <gmssl/pem.h>
#endif

#ifdef _WIN32
#define _strncasecmp(str1, str2, count) _strnicmp(str1, str2, count)
#else
#define _strncasecmp(str1, str2, count) strncasecmp(str1, str2, count)
#endif

#define OTP_LENGTH 6
#define TIME_STEP 30
#define BASE32_LENGTH 64

/* Base32*/
static const char base32_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567=";

/* Base32*/
static void
base32_decode(const char *base32, unsigned char *output, size_t *output_len)
{
	size_t bit_index = 0;
	*output_len = 0;

	for (const char *ptr = base32; *ptr; ptr++)
	{
		char value;
		if (*ptr >= 'A' && *ptr <= 'Z')
		{
			value = *ptr - 'A';
		}
		else if (*ptr >= '2' && *ptr <= '7')
		{
			value = *ptr - '2' + 26;
		}
		else
		{
			continue;
		}

		bit_index += 5;
		if (bit_index >= 8)
		{
			output[*output_len] |= (value >> (bit_index - 8));
			(*output_len)++;
			output[*output_len] = 0;
			bit_index -= 8;
		}

		output[*output_len] |= (value << (8 - bit_index));
	}
}

/* Base32 */
static void
base32_encode(const unsigned char *input, size_t input_len, char *base32, size_t *base32_len)
{
	size_t bit_index = 0;
	int value = 0;
	*base32_len = 0;

	for (size_t i = 0; i < input_len; i++)
	{
		value = (value << 8) | input[i];
		bit_index += 8;

		while (bit_index >= 5)
		{
			base32[*base32_len] = base32_chars[(value >> (bit_index - 5)) & 0x1F];
			(*base32_len)++;
			bit_index -= 5;
		}
	}

	if (bit_index > 0)
	{
		base32[*base32_len] = base32_chars[(value << (5 - bit_index)) & 0x1F];
		(*base32_len)++;
	}

	base32[*base32_len] = '\0';
}

#if defined(USE_OPENSSL) || defined(USE_GMSSL)
static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";
#endif

static long
get_time_interval()
{
	return time(NULL) / TIME_STEP;
}

/* HMAC-SHA1 */
static unsigned char *
hmac_sha1(const unsigned char *key, int key_len,
								const unsigned char *data, int data_len,
								unsigned char *result, unsigned int result_len)
{
	pg_hmac_ctx *hmac_ctx = pg_hmac_create(PG_SHA1);

	if (hmac_ctx == NULL)
	{
		elog(ERROR, gettext_noop("TOTP could not initialize checksum context"));
		return NULL;
	}

	if (pg_hmac_init(hmac_ctx, key, key_len) < 0 ||
		pg_hmac_update(hmac_ctx, data, data_len) < 0 ||
		pg_hmac_final(hmac_ctx, result, result_len) < 0)
	{
		pg_hmac_free(hmac_ctx);
		elog(ERROR, gettext_noop("TOTP could not initialize checksum"));
		return NULL;
	}

	pg_hmac_free(hmac_ctx);
	return result;
}

/*
 * mfa_totp_generate
 *
 * Based on the current time and key, generate a one-time TOTP key
 * to verify whether the one-time key sent by the client is correct.
 * The one-time key changes every 30 seconds.
 */
static int
mfa_totp_generate(const unsigned char *key, int key_len)
{
	long time_interval;
	unsigned char time_bytes[8] = {0};
	int i;
	unsigned char hmac_result[SHA1_DIGEST_LENGTH];
	unsigned int hmac_len = sizeof(hmac_result);
	int offset;
	int otp;

	time_interval = get_time_interval();

	for (i = 0; i < 8; i++)
	{
		time_bytes[7 - i] = time_interval & 0xFF;
		time_interval >>= 8;
	}

	/* HMAC-SHA1 */
	hmac_sha1(key, key_len, time_bytes, sizeof(time_bytes), hmac_result, hmac_len);

	offset = hmac_result[hmac_len - 1] & 0x0F;
	otp = (hmac_result[offset] & 0x7F) << 24 |
		  (hmac_result[offset + 1] & 0xFF) << 16 |
		  (hmac_result[offset + 2] & 0xFF) << 8 |
		  (hmac_result[offset + 3] & 0xFF);

	otp %= (int)pow(10, OTP_LENGTH);
	return otp;
}

/*
 * mfa_totp_verify
 *
 * Generate a temporary one-time key using the locally
 * stored basic key and verify whether the user is a
 * legitimate one by constantly checking the one-time
 * key sent from the client.
 */
int
mfa_totp_verify(const char *role, const char *show_otp,
					int user_totp,
					const char *opt_salt, int otp_salt_len,
					char **logdetail)
{
	const char *secret;
	unsigned char key[BASE32_LENGTH] = {0};
	size_t key_len;
	int generated_otp;

	secret = show_otp;

	base32_decode(secret, key, &key_len);

	generated_otp = mfa_totp_generate(key, key_len);

	if (user_totp == generated_otp)
		return STATUS_OK;
	
	*logdetail = psprintf(_("the client password is incorrect"));

	return STATUS_ERROR;
}

/*
 * mfa_totp_genkey
 *
 * Generate the totp base key using random numbers.
 */
int
mfa_totp_genkey(char *out)
{
	unsigned char key[32] = {0};
	size_t key_len;

	if (!pg_strong_random(key, 12))
	{
		ereport(LOG, (errmsg("could not generate random OTP key")));
		return -1;
	}

	base32_encode(key, 12, out, &key_len);
	out[key_len] = '\0';

	return 1;
}

/*
 * mfa_get_keytype
 *
 * By checking the secondary authentication type and key set
 * in the system table pg_mfaid, determine which secondary
 * authentication method it is.
 */
static MFAType
mfa_get_keytype(const char *type, const char *key)
{
	if (strlen(type) >= 4)
	{
		if (strncmp(type, "totp", 4) == 0 &&
			strspn(key, base32_chars) > 8)
		{
			return MFA_TYPE_TOTP;
#if defined(USE_OPENSSL) || defined(USE_GMSSL)
		}
		else if (strncmp(type, "pkey", 4) == 0 &&
				 strspn(key, base64_chars) > 120)
		{
			return MFA_TYPE_PKEY;
#endif
		}
	}
#if defined(USE_OPENSSL) || defined(USE_GMSSL)
	if (strncmp(type, "cert", 4) == 0)
	{
		return MFA_TYPE_CERT;
	}
#endif
	return MFA_TYPE_UNKNOWN;
}

/*
 * mfa_get_roleid
 *
 * Search for the user OID from the system table pg_mfaid.
 */
Oid
mfa_get_roleid(const char *rolename)
{
	HeapTuple roleTup;
	Form_pg_authid rform;
	Oid roleid;

	if (rolename != NULL)
	{
		roleTup = SearchSysCache1(AUTHNAME, PointerGetDatum(rolename));
		if (!HeapTupleIsValid(roleTup))
			ereport(FATAL,
					(errcode(ERRCODE_INVALID_AUTHORIZATION_SPECIFICATION),
					 errmsg("role \"%s\" does not exist", rolename)));
	}
	else
	{
		return InvalidOid;
	}

	rform = (Form_pg_authid)GETSTRUCT(roleTup);
	roleid = rform->oid;
	ReleaseSysCache(roleTup);
	return roleid;
}

/*
 * mfa_get_key
 *
 * Search for the secondary authentication key from the
 * system table pg_mfaid based on the user OID.
 */
char *
mfa_get_key(Port *port, Oid roleid, MFAType *otp_type, char **logdetail)
{
	HeapTuple roleTup = NULL;
	Datum datum;
	bool isnull;
	char *key = NULL;
	char *type = NULL;

	*otp_type = MFA_TYPE_UNKNOWN;

	roleTup = SearchSysCache1(MFAAUTHOID,
							  ObjectIdGetDatum(roleid));

	if (!HeapTupleIsValid(roleTup))
	{
		*logdetail = psprintf(_("Role \"%d\" does not exist."),
							  roleid);
		return NULL; /* no such user */
	}

	datum = SysCacheGetAttr(MFAAUTHOID, roleTup,
							Anum_pg_mfaid_mfa_key, &isnull);
	if (isnull)
	{
		ReleaseSysCache(roleTup);
		*logdetail = psprintf(_("User \"%d\" has no multifactor authentication key assigned."),
							  roleid);
		return NULL; /* user has no password */
	}
	key = TextDatumGetCString(datum);

	datum = SysCacheGetAttr(MFAAUTHOID, roleTup,
							Anum_pg_mfaid_mfa_type, &isnull);
	if (!isnull)
		type = TextDatumGetCString(datum);

	ReleaseSysCache(roleTup);

	if (NULL != type)
	{
		*otp_type = mfa_get_keytype(type, key);
		pfree(type);
	}

	return key;
}

#ifdef USE_OPENSSL
/*
 * pkey_signature_verify
 *
 * Verify the random number data signed by the client with the private key
 * using the saved public key.
 */
static int
pkey_signature_verify_openssl(const char *publicKeyBase64, const char *message,
								const unsigned char *sig, size_t sig_len)
{
	int publicKeyDER_len;
	BIO *bio_mem = NULL;
	EVP_PKEY *pkey = NULL;
	EVP_MD_CTX *mctx = NULL;
	int ret = 0;
	int pkey_type = EVP_PKEY_NONE;

	int inlen = strlen(publicKeyBase64);
	unsigned char *publicKeyDER = palloc0(inlen);

	if (!publicKeyDER)
		return 0;

	publicKeyDER_len = pg_b64_decode(publicKeyBase64, inlen,
									(char *)publicKeyDER, inlen);

	if (publicKeyDER_len <= 1)
	{
		pfree(publicKeyDER);
		return 0;
	}

	bio_mem = BIO_new(BIO_s_mem());
	BIO_write(bio_mem, publicKeyDER, publicKeyDER_len);
	pkey = d2i_PUBKEY_bio(bio_mem, NULL);
	BIO_free_all(bio_mem);
	pfree(publicKeyDER);

	if (!pkey)
		goto end;

	mctx = EVP_MD_CTX_create();
	if (!mctx)
		goto end;

	pkey_type = EVP_PKEY_id(pkey);
	if (pkey_type == EVP_PKEY_RSA)
		EVP_DigestVerifyInit(mctx, NULL, EVP_sha256(), NULL, pkey);
	else
		goto end;

	EVP_DigestVerifyUpdate(mctx, message, 16);

	ret = EVP_DigestVerifyFinal(mctx, (unsigned char *)sig, sig_len);

end:
	if (mctx)
		EVP_MD_CTX_destroy(mctx);
	if (pkey)
		EVP_PKEY_free(pkey);

	return ret;
}
#endif

#ifdef USE_GMSSL

#define MAX_DER_SIZE 512

static int
pkey_signature_verify_gmssl(const char *publicKeyBase64,
							const char *message, const unsigned char *sig,
							size_t sig_len)
{
	int ret = 0;
	SM2_KEY verify_key;
	uint8_t pub_der[MAX_DER_SIZE];
	const uint8_t *p = pub_der;
	int priv_derlen = 0;
	size_t pub_derlen  = sizeof(pub_der);
	size_t plen = pub_derlen;
	uint8_t dgst[SM3_DIGEST_SIZE];
	SM3_CTX sm3;

	priv_derlen = base64_decode_block(pub_der, (uint8_t *)publicKeyBase64,
										strlen(publicKeyBase64));
	if (priv_derlen <= 1)
		goto end;

	gm_sm3_init(&sm3);
	gm_sm3_update(&sm3, (const uint8_t *)message, 16);
	gm_sm3_finish(&sm3, dgst);

	if (sm2_public_key_info_from_der(&verify_key, &p, &plen) != 1)
		goto end;

	if (gm_sm2_verify(&verify_key, dgst, sig, sig_len) != 1)
		goto end;

	ret = 1;
end:
	return ret;
}
#endif

/*
 * mfa_pkey_verify
 *
 * Public-private key authentication: Verify the client signature.
 */
int
mfa_pkey_verify(const char *role, const char *show_otp,
					char *user_otp,
					const char *opt_salt, int otp_salt_len,
					char **logdetail)
{
	int sig_len = 0;
	char *pkey = NULL;
	int user_otp_len = strlen(user_otp);
	unsigned char *sig = palloc0(user_otp_len);
	if (!sig)
	{
		*logdetail = psprintf(_("There is not enough memory allocated for the signature"));
		return STATUS_ERROR;
	}

	sig_len = pg_b64_decode(user_otp, user_otp_len, (char *)sig, user_otp_len);
	if (sig_len <= 0)
	{
		pfree(sig);
		*logdetail = psprintf(_("Client signature decoding error"));
		return STATUS_ERROR;
	}

#ifdef USE_OPENSSL
	pkey = strdup(show_otp);
	if (1 == pkey_signature_verify_openssl(pkey, opt_salt, sig, sig_len))
	{
		pfree(sig);
		return STATUS_OK;
	}
#endif
#ifdef USE_GMSSL
	pkey = strdup(show_otp);
	if (1 == pkey_signature_verify_gmssl(pkey, opt_salt, sig, sig_len))
	{
		pfree(sig);
		return STATUS_OK;
	}
#endif

#if !defined(USE_OPENSSL) && !defined(USE_GMSSL)
	(void)pkey;
#endif

	pfree(sig);
	*logdetail = psprintf(_("Signature verification failed."));

	return STATUS_ERROR;
}

#ifdef USE_OPENSSL
/*
 * cert_signature_verify_openssl
 *
 * Certificate authentication: Use the public key in the saved certificate
 * (rsa)to verify the signature data sent by the client.
 */
static int
cert_signature_verify_openssl(const char *cert_data, size_t cert_data_len,
										 const unsigned char *sig, size_t sig_len,
										 const unsigned char *data, size_t data_len)
{
	int ret = 0;
	EVP_PKEY *pkey = NULL;
	BIO *bio_mem = NULL;
	X509 *cert = NULL;
	EVP_MD_CTX *mctx = NULL;
	int pkey_type = -1;

	bio_mem = BIO_new(BIO_s_mem());
	BIO_write(bio_mem, cert_data, cert_data_len);
	cert = PEM_read_bio_X509(bio_mem, NULL, NULL, NULL);
	BIO_free_all(bio_mem);
	if (cert == NULL)
		return -1;

	pkey = X509_get_pubkey(cert);
	if (pkey == NULL)
	{
		X509_free(cert);
		return -1;
	}

	mctx = EVP_MD_CTX_create();
	if (!mctx)
		goto cleanup;

	pkey_type = EVP_PKEY_id(pkey);

	if (pkey_type == EVP_PKEY_RSA)
	{
		if (EVP_DigestVerifyInit(mctx, NULL, EVP_sha256(), NULL, pkey) <= 0)
		{
			ERR_print_errors_fp(stderr);
			goto cleanup;
		}
	}
	else
	{
		goto cleanup;
	}

	ret = EVP_DigestVerifyUpdate(mctx, data, data_len);
	if (ret != 1)
		goto cleanup;

	ret = EVP_DigestVerifyFinal(mctx, sig, sig_len);

cleanup:

	if (mctx)
		EVP_MD_CTX_destroy(mctx);
	if (pkey)
		EVP_PKEY_free(pkey);

	if (cert)
		X509_free(cert);

	return ret;
}
#endif

#ifdef USE_GMSSL
static int load_cert_from_pem_string(const char *pem_str, uint8_t *cert, size_t *cert_len, size_t maxlen) {
	int ret = -1;
	FILE *fp = fmemopen((void *)pem_str, strlen(pem_str), "r");
	if (!fp) 
		return -1;

	ret = x509_cert_from_pem(cert, cert_len, maxlen, fp);

	fclose(fp);
	return ret;
}

/*
 * cert_signature_verify_gmssl
 *
 * Certificate authentication: Use the public key in the saved certificate
 * (sm2)to verify the signature data sent by the client.
 */
static int
cert_signature_verify_gmssl(const char *cert_data, size_t cert_data_len,
										 const unsigned char *sig, size_t sig_len,
										 const unsigned char *data, size_t data_len)
{
	uint8_t  client_cert[4096];
	size_t client_cert_len = 0;
	SM2_KEY client_pubkey;
	uint8_t dgst[32];
	int ret = -1;

	if (load_cert_from_pem_string(cert_data, client_cert,
									&client_cert_len, sizeof(client_cert)) != 1)
		goto end;

	sm3_digest(data, data_len, dgst);

	if (x509_cert_get_subject_public_key(client_cert, client_cert_len, &client_pubkey) != 1)
		goto end;

	if (gm_sm2_verify(&client_pubkey, dgst, sig, sig_len) != 1)
		goto end;

	ret = 1;

end:
	return ret;
}
#endif

#ifdef USE_OPENSSL

static void
init_openssl(void)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	static bool isfirst = true;
	if (isfirst)
		OpenSSL_add_all_algorithms();
	isfirst = false;
#endif
}

/*
 * cert_cert_verify
 *
 * Certificate authentication: Use the local root certificate
 * to verify whether the client certificate is valid and has expired.
 */
static int
cert_verify_openssl(const char *cert_str, const char *root_cert_path, char **logdetail)
{
	int ret = 0;
	X509 *cert = NULL;
	X509 *root_cert = NULL;
	X509_STORE *cert_store = NULL;
	X509_STORE_CTX *ctx = NULL;

	BIO *bio = BIO_new_mem_buf(cert_str, -1);
	if (!bio)
		goto end;

	cert = PEM_read_bio_X509(bio, NULL, NULL, NULL);
	BIO_free(bio);
	bio = NULL;
	if (!cert)
		goto end;

	/* validity check */
	{
		ASN1_TIME *notBefore = X509_get_notBefore(cert);
		ASN1_TIME *notAfter = X509_get_notAfter(cert);
		time_t current_time = time(NULL);

		if (X509_cmp_time(notBefore, &current_time) > 0 || X509_cmp_time(notAfter, &current_time) < 0)
		{
			*logdetail = psprintf(_("Certificate verification failed(RSA certificate is expired or not yet valid)."));
			goto end;
		}
	}

	bio = BIO_new_file(root_cert_path, "r");
	if (!bio)
		goto end;

	root_cert = PEM_read_bio_X509(bio, NULL, NULL, NULL);
	if (!root_cert)
		goto end;

	cert_store = X509_STORE_new();
	if (!cert_store)
		goto end;

	if (X509_STORE_add_cert(cert_store, root_cert) != 1)
		goto end;

	ctx = X509_STORE_CTX_new();
	if (!ctx)
		goto end;

	if (X509_STORE_CTX_init(ctx, cert_store, cert, NULL) != 1)
		goto end;

	ret = X509_verify_cert(ctx);
	if (ret != 1)
		*logdetail = psprintf(_("Certificate verification failed(RSA)."));

end:
	if (bio)
		BIO_free(bio);
	if (ctx)
		X509_STORE_CTX_free(ctx);
	if (cert_store)
		X509_STORE_free(cert_store);
	if (cert)
		X509_free(cert);
	if (root_cert)
		X509_free(root_cert);

	return ret;
}

static int
cert_get_cert_type(const char *client_crt)
{
    BIO *bio = NULL;
    X509 *cert = NULL;
    EVP_PKEY *pkey = NULL;
    int result = 0;

    bio = BIO_new_mem_buf(client_crt, (int)strlen(client_crt));
    if (!bio) goto err;

    cert = PEM_read_bio_X509(bio, NULL, 0, NULL);
    if (!cert) goto err;

    pkey = X509_get_pubkey(cert);
    if (!pkey) goto err;

    switch (EVP_PKEY_base_id(pkey)) {
        case EVP_PKEY_RSA:
            result = 1;
            break;
        default:
            result = 0;
			break;
    }

    EVP_PKEY_free(pkey);
    X509_free(cert);
    BIO_free(bio);
    return result;

err:
    if (pkey) EVP_PKEY_free(pkey);
    if (cert) X509_free(cert);
    if (bio) BIO_free(bio);
    return result;
}

#endif

#ifdef USE_GMSSL
static int
cert_verify_gmssl(const char *cert_str, const char *root_cert_path, char **logdetail)
{
	uint8_t root_cert[4096], client_cert[4096];
	size_t root_cert_len = 0, client_cert_len = 0;
	int ret = -1;

	FILE *fp = fopen(root_cert_path, "r");
	if (!fp) 
		goto end;

	if (x509_cert_from_pem(root_cert, &root_cert_len, sizeof(root_cert), fp) != 1) {
		fclose(fp);
		goto end;
	}
	fclose(fp);

	if (load_cert_from_pem_string(cert_str, client_cert,
									&client_cert_len, sizeof(client_cert)) != 1)
		goto end;

	{
		time_t   not_before, not_after;
		time_t now;
		int    ok  = 0;

		if (x509_cert_get_details(
				client_cert, client_cert_len,
				NULL,
				NULL, NULL,
				NULL,
				NULL, NULL,
				&not_before, &not_after,
				NULL, NULL,
				NULL,
				NULL, NULL,
				NULL, NULL,
				NULL, NULL,
				NULL,
				NULL, NULL) != 1) {
			goto end;
		}

		now = time(NULL);
		ok  = x509_validity_check(not_before, not_after, now,
										X509_VALIDITY_MAX_SECONDS);
		if (ok != 1)
		{
			*logdetail = psprintf(_("Certificate verification failed(SM2 certificate is expired or not yet valid)."));
			goto end;
		}
	}

	if (x509_cert_verify_by_ca_cert(client_cert, client_cert_len, root_cert,
						root_cert_len, SM2_DEFAULT_ID, SM2_DEFAULT_ID_LENGTH) != 1)
	{
		*logdetail = psprintf(_("Certificate verification failed(SM2)."));
		goto end;
	}

	ret = 1;
end:

	return ret;
}
#endif

/*
 * mfa_cert_verify
 *
 * Certificate authentication: Load the local root certificate,
 * verify the client certificate and the client signature.
 */
int
mfa_cert_verify(const char *role, const char *cert, const char *sign,
					char *shadow_otp, const char *opt_salt, int otp_salt_len,
					char **logdetail)
{
#if defined(USE_OPENSSL) || defined(USE_GMSSL)
	int cert_type = 0;	//0: SM2 1: RSA
	int sig_len = 0;
	unsigned char sig[512] = {0x00};
	char root_cert_file[128] = {0};

	if (shadow_otp && strlen(shadow_otp) > 0)
	{
		snprintf(root_cert_file, sizeof(root_cert_file), "%s_cert_root.crt", shadow_otp);

		if (access(root_cert_file, F_OK) != 0)
		{
			strncpy(root_cert_file, "cert_root.crt", sizeof(root_cert_file) - 1);
			root_cert_file[sizeof(root_cert_file) - 1] = '\0';
		}
	}
	else
	{
		strncpy(root_cert_file, "cert_root.crt", sizeof(root_cert_file) - 1);
		root_cert_file[sizeof(root_cert_file) - 1] = '\0';
	}

	if (access(root_cert_file, F_OK) != 0)
	{
		strncpy(root_cert_file, "root.crt", sizeof(root_cert_file) - 1);
		root_cert_file[sizeof(root_cert_file) - 1] = '\0';
	}

	if (access(root_cert_file, F_OK) != 0)
	{
		*logdetail = psprintf(_("The root certificate does not exist.(%s)"), shadow_otp);
		return STATUS_ERROR;
	}

#ifdef USE_OPENSSL
	init_openssl();
	cert_type = cert_get_cert_type(cert);
	if (cert_type == 1)
	{
		if (cert_verify_openssl(cert, root_cert_file, logdetail) == 1)
		{
			sig_len = pg_b64_decode(sign, strlen(sign), (char *)sig, sizeof(sig));

			if (cert_signature_verify_openssl(cert, strlen(cert), sig, sig_len, (const unsigned char *)opt_salt, otp_salt_len) == 1)
				return STATUS_OK;
			else
				*logdetail = psprintf(_("Signature verification failed(RSA)."));
		}
	}
#endif

#ifdef USE_GMSSL
	if (cert_type == 0)
	{
		if (cert_verify_gmssl(cert, root_cert_file, logdetail) == 1)
		{
			sig_len = pg_b64_decode(sign, strlen(sign), (char *)sig, sizeof(sig));

			if (cert_signature_verify_gmssl(cert, strlen(cert), sig, sig_len, (const unsigned char *)opt_salt, otp_salt_len) == 1)
				return STATUS_OK;
			else
				*logdetail = psprintf(_("Signature verification failed(SM2)."));
		}
	}
#endif

#else
	*logdetail = psprintf(_("Certificate authentication requries the use of" \
								"--with-openssl or --with-gmssl for compilation."));
#endif

	return STATUS_ERROR;
}

/*
 * modify_mfa_param
 *
 * Modify or add the secondary authentication configuration for users.
 */
static bool
modify_mfa_param(const char *rolename, const char *otp, const char *param, bool isdel)
{
	Relation rel;
	HeapTuple oldtuple;
	HeapTuple newtup;
	bool nulls[Natts_pg_mfaid];
	Datum values[Natts_pg_mfaid];
	bool replaces[Natts_pg_mfaid];
	NameData rolname;
	TupleDesc tupDesc;
	Oid useroid;

	if (!isdel && param == NULL)
		return false;

	/* Initialize inserted values */
	MemSet(values, 0, sizeof(values));
	MemSet(nulls, false, sizeof(nulls));
	MemSet(replaces, true, sizeof(replaces));

	if (rolename == NULL)
		return false;

	namestrcpy(&rolname, pstrdup(rolename));
	useroid = get_role_oid(rolename, false);

	rel = table_open(MfaIdRelationId, RowExclusiveLock);
	tupDesc = RelationGetDescr(rel);

	oldtuple = SearchSysCache1(MFAAUTHOID,
							   ObjectIdGetDatum(useroid));

	/* if the object exists, update is executed,
	 * if it does not exist, insert is executed.
	 */
	if (HeapTupleIsValid(oldtuple))
	{
		if(isdel)
		{
			simple_heap_delete(rel, &oldtuple->t_self);
		}
		else
		{
			/* fill in the values and then update */
			values[Anum_pg_mfaid_oid - 1] = ObjectIdGetDatum(useroid);
			if (otp)
				values[Anum_pg_mfaid_mfa_type - 1] = CStringGetTextDatum(otp);
			else
				nulls[Anum_pg_mfaid_mfa_type - 1] = true;

			if (param)
				values[Anum_pg_mfaid_mfa_key - 1] = CStringGetTextDatum(param);
			else
				nulls[Anum_pg_mfaid_mfa_key - 1] = true;

			newtup = heap_modify_tuple(oldtuple, tupDesc, values, nulls, replaces);
			CatalogTupleUpdate(rel, &newtup->t_self, newtup);
		}
		ReleaseSysCache(oldtuple);
	}
	else
	{
		if (!isdel)
		{
			/* fill in the values and then update */
			values[Anum_pg_mfaid_oid - 1] = ObjectIdGetDatum(useroid);
			if (otp)
				values[Anum_pg_mfaid_mfa_type - 1] = CStringGetTextDatum(otp);
			else
				nulls[Anum_pg_mfaid_mfa_type - 1] = true;

			if (param)
				values[Anum_pg_mfaid_mfa_key - 1] = CStringGetTextDatum(param);
			else
				nulls[Anum_pg_mfaid_mfa_key - 1] = true;

			newtup = heap_form_tuple(tupDesc, values, nulls);
			CatalogTupleInsert(rel, newtup);
		}
	}

	table_close(rel, NoLock);

	return true;
}

static bool is_manager(void)
{
	if (se_feature.mode)
		return IS_SYSSSO(GetUserId());
	else
		return IS_SYSDBA(GetUserId());
}

/*
 * pg_set_2fa
 *
 * Modify or add the secondary authentication configuration for users.
 */
Datum pg_set_2fa(PG_FUNCTION_ARGS)
{
	char out_str[64];
	Datum role = PG_GETARG_DATUM(0);
	const char *role_p = TextDatumGetCString(role);

	Datum type = PG_GETARG_DATUM(1);
	const char *type_p = TextDatumGetCString(type);

	Datum param = PG_GETARG_DATUM(2);
	const char *param_p = TextDatumGetCString(param);

	if (!is_manager())
		elog(ERROR, "Permission denied.");

	/* Prepare the output message */
	snprintf(out_str, sizeof(out_str), "Set two-factor authentication to %s for %s: success!", type_p, role_p);

	/* Check the type and prepare rolotp accordingly */
	if (_strncasecmp(type_p, "TOTP", 4) == 0)
	{
		if (strlen(param_p) == 0 || param_p[0] == '\0')
		{
			char out[64];
			if (mfa_totp_genkey(out) == 1)
				modify_mfa_param(role_p, "totp", out, false);
		}
		else if ((strlen(param_p) <= BASE32_LENGTH) &&
			strspn(param_p, base32_chars) == strlen(param_p))
		{
			modify_mfa_param(role_p, "totp", param_p, false);
		}
		else
		{
			snprintf(out_str, sizeof(out_str), "Invalid TOTP key.");
		}
	}
#if defined(USE_OPENSSL) || defined(USE_GMSSL)
	else if (_strncasecmp(type_p, "PKEY", 4) == 0)
	{
		modify_mfa_param(role_p, "pkey", param_p, false);
	}
	else if (_strncasecmp(type_p, "CERT", 4) == 0)
	{
		if (param_p == NULL || strlen(param_p) == 0 || param_p[0] == '\0')
			modify_mfa_param(role_p, "cert", "rsa", false);
		else
			modify_mfa_param(role_p, "cert", param_p, false);
	}
#endif
	else
	{
		snprintf(out_str, sizeof(out_str), "Unknown type or wrong key.");
	}

	PG_RETURN_TEXT_P(cstring_to_text(out_str));
}

/*
 * pg_unset_2fa
 *
 * Delete the secondary authentication configuration for users.
 */
Datum pg_unset_2fa(PG_FUNCTION_ARGS)
{
	char out_str[64];
	Datum role = PG_GETARG_DATUM(0);
	const char *role_p = TextDatumGetCString(role);

	if (!is_manager())
		elog(ERROR, "Permission denied.");

	/* Prepare the output message */
	snprintf(out_str, sizeof(out_str), "Unset two-factor authentication for %s: success!", role_p);

	modify_mfa_param(role_p, NULL, NULL, true);

	PG_RETURN_TEXT_P(cstring_to_text(out_str));
}
