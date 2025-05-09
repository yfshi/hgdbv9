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

#include "se_features_interface.h"

#ifdef USE_OPENSSL
#include <openssl/ec.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <openssl/err.h>
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
static void base32_decode(const char *base32, unsigned char *output, size_t *output_len)
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
static void base32_encode(const unsigned char *input, size_t input_len, char *base32, size_t *base32_len)
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

#ifdef USE_OPENSSL
static const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/=";

char *mfa_base64_encode(const unsigned char *input, int length)
{
	char *output = NULL;
	BIO *bio, *b64;
	BUF_MEM *bufferPtr;

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

unsigned char *mfa_base64_decode(const char *input, int *length)
{
	BIO *bio, *b64;
	int input_len = strlen(input);
	unsigned char *buffer = (unsigned char *)malloc(input_len);

	b64 = BIO_new(BIO_f_base64());
	bio = BIO_new_mem_buf(input, input_len);
	bio = BIO_push(b64, bio);
	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL); /* no new line */

	*length = BIO_read(bio, buffer, input_len);
	BIO_free_all(bio);

	return buffer;
}
#endif

static long get_time_interval()
{
	return time(NULL) / TIME_STEP;
}

/* HMAC-SHA1 */
static unsigned char *hmac_sha1(const unsigned char *key, int key_len,
								const unsigned char *data, int data_len,
								unsigned char *result, unsigned int result_len)
{
	pg_hmac_ctx *hmac_ctx = pg_hmac_create(PG_SHA1);

	if (hmac_ctx == NULL)
	{
		elog(ERROR, gettext_noop("TOTP could not initialize checksum context"));
	}

	if (pg_hmac_init(hmac_ctx, key, key_len) < 0 ||
		pg_hmac_update(hmac_ctx, data, data_len) < 0 ||
		pg_hmac_final(hmac_ctx, result, result_len) < 0)
	{
		pg_hmac_free(hmac_ctx);
		elog(ERROR, gettext_noop("TOTP could not initialize checksum"));
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
	{
		return STATUS_OK;
	}

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

	return 0;
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
#ifdef USE_OPENSSL
		}
		else if (strncmp(type, "pkey", 4) == 0 &&
				 strspn(key, base64_chars) > 120)
		{
			return MFA_TYPE_PKEY;
#endif
		}
	}
#ifdef USE_OPENSSL
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
pkey_signature_verify(const char *publicKeyBase64, const char *message, const unsigned char *sig, size_t sig_len, char *user_id)
{
	int publicKeyDER_len;
	unsigned char *publicKeyDER;
	BIO *bio_mem;
	EVP_PKEY *pkey = NULL;
	EVP_MD_CTX *mctx;
	int ret;
	int pkey_type = EVP_PKEY_NONE;

	publicKeyDER = mfa_base64_decode(publicKeyBase64, &publicKeyDER_len);

	bio_mem = BIO_new(BIO_s_mem());
	BIO_write(bio_mem, publicKeyDER, publicKeyDER_len);
	pkey = d2i_PUBKEY_bio(bio_mem, NULL);
	BIO_free_all(bio_mem);
	free(publicKeyDER);

	if (pkey == NULL)
	{
		return -1;
	}

	mctx = EVP_MD_CTX_create();

	pkey_type = EVP_PKEY_id(pkey);
	if (pkey_type == EVP_PKEY_RSA)
	{
		EVP_DigestVerifyInit(mctx, NULL, EVP_sha256(), NULL, pkey);
	}
	else
	{
#ifdef OPENSSL_VERSION_NUMBER
#if OPENSSL_VERSION_NUMBER < 0x10100000L
		return -1;
#elif OPENSSL_VERSION_NUMBER < 0x30000000L
		EVP_PKEY_set_alias_type(pkey, EVP_PKEY_SM2);
#endif
#if OPENSSL_VERSION_NUMBER > 0x10100000L
		EVP_DigestVerifyInit(mctx, NULL, EVP_sm3(), NULL, pkey);
#endif
#endif
	}

	EVP_DigestVerifyUpdate(mctx, message, 16);

	ret = EVP_DigestVerifyFinal(mctx, (unsigned char *)sig, sig_len);

	EVP_MD_CTX_destroy(mctx);
	EVP_PKEY_free(pkey);

	return (ret == 1) ? 1 : 0;
}

/*
 * mfa_pkey_verify
 *
 * Public-private key authentication: Verify the client signature.
 */
int
mfa_pkey_verify(const char *role, const char *show_otp,
					char *user_totp,
					const char *opt_salt, int otp_salt_len,
					char **logdetail)
{
	int sig_len = 0;
	unsigned char *sig = mfa_base64_decode(user_totp, &sig_len);
	if (1 == pkey_signature_verify(show_otp, opt_salt, sig, sig_len, NULL))
	{
		return STATUS_OK;
	}
	return STATUS_ERROR;
}

static void cert_handleErrors(void)
{
	ERR_print_errors_fp(stderr);
	abort();
}

/*
 * cert_signature_verify_sm2_rsa
 *
 * Certificate authentication: Use the public key in the saved certificate
 * (sm2 or rsa)to verify the signature data sent by the client.
 */
static int
cert_signature_verify_sm2_rsa(const char *cert_data, size_t cert_data_len,
										 const unsigned char *sig, size_t sig_len,
										 const unsigned char *data, size_t data_len)
{
	int ret = 0;
	EVP_PKEY *pkey = NULL;
	BIO *bio_mem = NULL;
	X509 *cert = NULL;
	EVP_PKEY_CTX *ctx = NULL;
	EVP_MD_CTX *mctx = NULL;
	int pkey_type = -1;
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	const char *sm2_id = "1234567812345678";
#endif
	bio_mem = BIO_new(BIO_s_mem());
	BIO_write(bio_mem, cert_data, cert_data_len);
	cert = PEM_read_bio_X509(bio_mem, NULL, NULL, NULL);
	BIO_free_all(bio_mem);
	if (cert == NULL)
	{
		return -1;
	}

	pkey = X509_get_pubkey(cert);
	if (pkey == NULL)
	{
		X509_free(cert);
		return -1;
	}

	mctx = EVP_MD_CTX_create();
	if (!mctx)
	{
		goto cleanup;
	}

	pkey_type = EVP_PKEY_id(pkey);
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	if (pkey_type <= 0)
	{
		int algnid = 0;
		X509_PUBKEY *pubkey_info = X509_get_X509_PUBKEY(cert);
		X509_ALGOR *alg = NULL;
		const ASN1_OBJECT *obj = NULL;
		X509_PUBKEY_get0_param(NULL, NULL, NULL, &alg, pubkey_info);

		X509_ALGOR_get0(&obj, NULL, NULL, alg);
		algnid = OBJ_obj2nid(obj);

		if (algnid == NID_sm2)
		{
			pkey_type = EVP_PKEY_SM2;
		}
	}

	if (pkey_type <= 0)
	{
		pkey_type = EVP_PKEY_SM2;
	}
#endif
	if (pkey_type == EVP_PKEY_RSA || pkey_type == EVP_PKEY_EC)
	{
		if (EVP_DigestVerifyInit(mctx, NULL, EVP_sha256(), NULL, pkey) <= 0)
		{
			ERR_print_errors_fp(stderr);
			goto cleanup;
		}
	}
#if defined(HAVE_X509_GET_SIGNATURE_INFO)
	else if (pkey_type == EVP_PKEY_SM2)
	{
#if OPENSSL_VERSION_NUMBER < 0x30000000L
		if (!EVP_PKEY_set_alias_type(pkey, EVP_PKEY_SM2))
		{
			ERR_print_errors_fp(stderr);
			goto cleanup;
		}
#endif
		ctx = EVP_PKEY_CTX_new(pkey, NULL);

		if (EVP_PKEY_CTX_set1_id(ctx, sm2_id, strlen((char *)sm2_id)) <= 0)
		{
			ERR_print_errors_fp(stderr);
			goto cleanup;
		}
		EVP_MD_CTX_set_pkey_ctx(mctx, ctx);

		if (EVP_DigestVerifyInit(mctx, NULL, EVP_sm3(), NULL, pkey) <= 0)
		{
			ERR_print_errors_fp(stderr);
			goto cleanup;
		}
	}
#endif
	else
	{
		goto cleanup;
	}

	ret = EVP_DigestVerifyUpdate(mctx, data, data_len);
	if (!ret)
	{
		goto cleanup;
	}

	ret = (EVP_DigestVerifyFinal(mctx, sig, sig_len) == 1) ? 1 : 0;

	if (!ret)
	{
		goto cleanup;
	}

cleanup:

#if OPENSSL_VERSION_NUMBER < 0x10100000L
	if (mctx)
		EVP_MD_CTX_cleanup(mctx);
#else
	if (mctx)
	{
		EVP_MD_CTX_free(mctx);
	}
#endif
	if (ctx)
	{
		EVP_PKEY_CTX_free(ctx);
	}

	if (pkey)
	{
		EVP_PKEY_free(pkey);
	}
	if (cert)
	{
		X509_free(cert);
	}

	return ret;
}

static int
cert_signature_verify(const char *cert_data, size_t cert_data_len,
								 const unsigned char *sig, size_t sig_len,
								 const unsigned char *data, size_t data_len)
{
	return cert_signature_verify_sm2_rsa(cert_data, cert_data_len, sig, sig_len, data, data_len);
}

/*
 * cert_cert_verify
 *
 * Certificate authentication: Use the local root certificate
 * to verify whether the client certificate is valid and has expired.
 */
static int
cert_cert_verify(const char *cert_str, const char *root_cert_path)
{
	int ret = 0;
	X509 *cert = NULL;
	X509 *root_cert = NULL;
	X509_STORE *cert_store = NULL;
	X509_STORE_CTX *ctx = NULL;

	BIO *bio = BIO_new_mem_buf(cert_str, -1);
	if (!bio)
		cert_handleErrors();

	cert = PEM_read_bio_X509(bio, NULL, NULL, NULL);
	BIO_free(bio);
	if (!cert)
		cert_handleErrors();

	// validity check
	{
		ASN1_TIME *notBefore = X509_get_notBefore(cert);
		ASN1_TIME *notAfter = X509_get_notAfter(cert);
		time_t current_time = time(NULL);

		if (X509_cmp_time(notBefore, &current_time) > 0)
		{
			X509_free(cert);
			return 1;
		}

		if (X509_cmp_time(notAfter, &current_time) < 0)
		{
			X509_free(cert);
			return 1;
		}
	}

	bio = BIO_new_file(root_cert_path, "r");
	if (!bio)
		cert_handleErrors();

	root_cert = PEM_read_bio_X509(bio, NULL, NULL, NULL);
	BIO_free(bio);
	if (!root_cert)
		cert_handleErrors();

	cert_store = X509_STORE_new();
	if (!cert_store)
		cert_handleErrors();

	if (X509_STORE_add_cert(cert_store, root_cert) != 1)
	{
		X509_STORE_free(cert_store);
		cert_handleErrors();
	}

	ctx = X509_STORE_CTX_new();
	if (!ctx)
	{
		X509_STORE_free(cert_store);
		cert_handleErrors();
	}

	if (X509_STORE_CTX_init(ctx, cert_store, cert, NULL) != 1)
	{
		X509_STORE_CTX_free(ctx);
		X509_STORE_free(cert_store);
		cert_handleErrors();
	}

	ret = X509_verify_cert(ctx);
	if (ret != 1)
	{
		return 0;
	}

	X509_STORE_CTX_free(ctx);
	X509_STORE_free(cert_store);
	X509_free(cert);
	X509_free(root_cert);

	return (ret == 1) ? 0 : 1;
}

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

	if (cert_cert_verify(cert, root_cert_file) != 0)
	{
		return STATUS_ERROR;
	}
	sig_len = pg_b64_decode(sign, strlen(sign), (char *)sig, sizeof(sig));

	if (cert_signature_verify(cert, strlen(cert), sig, sig_len, (const unsigned char *)opt_salt, otp_salt_len) != 1)
	{
		return STATUS_ERROR;
	}

	return STATUS_OK;
}
#endif /* USE_OPENSSL */

/*
 * modify_mfa_param
 *
 * Modify or add the secondary authentication configuration for users.
 */
static bool
modify_mfa_param(const char *rolename, const char *otp, const char *param)
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

	if (param == NULL)
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

			ReleaseSysCache(oldtuple);
		}
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

		newtup = heap_form_tuple(tupDesc, values, nulls);
		CatalogTupleInsert(rel, newtup);
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
		elog(ERROR, "Permission denied. Only role=(syssso_role or sysdba_role) can execute this function.");

	/* Prepare the output message */
	snprintf(out_str, sizeof(out_str), "Set two-factor authentication to %s for %s: success!", type_p, role_p);

	/* Check the type and prepare rolotp accordingly */
	if ((_strncasecmp(type_p, "TOTP", 4) == 0) &&
		(strlen(param_p) <= BASE32_LENGTH) &&
		(strspn(param_p, base32_chars) == strlen(param_p)))
	{
		modify_mfa_param(role_p, "totp", param_p);
#ifdef USE_OPENSSL
	}
	else if (_strncasecmp(type_p, "PKEY", 4) == 0)
	{
		modify_mfa_param(role_p, "pkey", param_p);
	}
	else if (_strncasecmp(type_p, "CERT", 4) == 0)
	{
		if (param_p == NULL || strlen(param_p) == 0 || param_p[0] == '\0')
			modify_mfa_param(role_p, "cert", "rsa");
		else
			modify_mfa_param(role_p, "cert", param_p);
#endif
	}
	else
	{
		snprintf(out_str, sizeof(out_str), "Unknown type or wrong key.");
	}

	PG_RETURN_TEXT_P(cstring_to_text(out_str));
}
