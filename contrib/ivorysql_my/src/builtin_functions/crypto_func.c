/*-------------------------------------------------------------------------------
 *
 * File: character_func.c
 *
 * Abstract:
 * 		This file contains the implementation of MySQL's
 *		crypto related built-in functions.
 *
 * Authored by liyonghao@highgo.com, 202501230.
 *
 * Copyright:
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *		contrib/ivorysql_my/src/builtin_functions/crypto_func.c
 *
 *-------------------------------------------------------------------------------
 */

#include "postgres.h"
#include "c.h"
#include "catalog/pg_namespace_d.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include "utils/elog.h"
#include "utils/ora_compatible.h"
#include "utils/varlena.h"
#include "../include/ivorysql_my.h"
#if USE_OPENSSL
#include <openssl/err.h>
#include <openssl/evp.h>
#if OPENSSL_VERSION_NUMBER >= 0x10101000L
#include <openssl/kdf.h>
#endif
#endif /* USE_OPENSSL */

PG_FUNCTION_INFO_V1(ivymy_aes_encrypt);
PG_FUNCTION_INFO_V1(ivymy_aes_decrypt);
PG_FUNCTION_INFO_V1(bytea_aes_decrypt);	/* bug#z7500 */
PG_FUNCTION_INFO_V1(random_bytes);

#if USE_OPENSSL
#define AES_IV_SIZE 16
#define MAX_KDF_OPTION_SIZE 256
#define MAX_AES_KEY_LENGTH 256
#define MIN_KDF_ITERATIONS_SIZE 1000
#define MAX_KDF_ITERATIONS_SIZE 65535
#define AES_BAD_DATA  -1
#define MAX_RANDOM_BYTES_BUFFER 1024

typedef enum Kdf_Name
{
	HKDF,
	PBKDF2_HAMC
} Kdf_Name;

static int aes_opmode_key_sizes[] =
{
	128 /* aes-128-ecb */,
	192 /* aes-192-ecb */,
	256 /* aes-256-ecb */,
	128 /* aes-128-cbc */,
	192 /* aes-192-cbc */,
	256 /* aes-256-cbc */,
	128 /* aes-128-cfb1 */,
	192 /* aes-192-cfb1 */,
	256 /* aes-256-cfb1 */,
	128 /* aes-128-cfb8 */,
	192 /* aes-192-cfb8 */,
	256 /* aes-256-cfb8 */,
	128 /* aes-128-cfb128 */,
	192 /* aes-192-cfb128 */,
	256 /* aes-256-cfb128 */,
	128 /* aes-128-ofb */,
	192 /* aes-192-ofb */,
	256 /* aes-256-ofb */
};

typedef struct KDF_Option
{
	char *kdf_name;
	char *salt;
	char *info_or_iter;
	int	iterations;
} KDF_Option;

static int size_of_kdf_option(KDF_Option *kdf_options);
static const EVP_CIPHER *aes_evp_type(const int mys_block_encryption_mode);
static bool aes_needs_iv(int mys_block_encryption_mode);
static int aes_get_size(uint32 source_length, int mys_block_encryption_mode);
static KDF_Option retrieve_kdf_options(char *arg3, char *arg4, char *arg5);
static bool kdf_function_derive_key(char *kdf_name,
								   const unsigned char *key,
								   const unsigned int key_length,
								   unsigned char *rkey,
								   unsigned int key_size,
								   KDF_Option *kdf_options);
static bool create_kdf_key(const unsigned char *key, const unsigned int key_length,
						  unsigned char *rkey, unsigned int rkey_size,
						  KDF_Option *kdf_options);
static void aes_create_key(const unsigned char *key, uint32 key_length, uint8 *rkey, int mys_block_encryption_mode);
static bool create_key(unsigned char *rkey, const unsigned char *key, uint32 key_length,
					   int mys_block_encryption_mode, KDF_Option *kdf_options);
static int aes_encrypt(const unsigned char *source, uint32 source_length,
					   unsigned char *dest,
					   const unsigned char *key, uint32 key_length,
					   int mys_block_encryption_mode, const unsigned char *iv,
					   bool padding,
					   KDF_Option *kdf_options);
static int aes_decrypt(const unsigned char *source, uint32 source_length,
					   unsigned char *dest,
					   const unsigned char *key, uint32 key_length,
					   int mys_block_encryption_mode, const unsigned char *iv,
					   bool padding,
					   KDF_Option *kdf_options);

static int 
size_of_kdf_option(KDF_Option *kdf_options)
{
	int kdf_option_size = 0;

	if (strcmp(kdf_options->kdf_name, ""))
		kdf_option_size++;
	if (strcmp(kdf_options->salt, ""))
		kdf_option_size++;
	if (strcmp(kdf_options->info_or_iter, ""))
		kdf_option_size++;

	return kdf_option_size;
}

static const EVP_CIPHER *
aes_evp_type(const int mys_block_encryption_mode)
{
	switch (mys_block_encryption_mode)
	{
		case mys_aes_128_ecb:
			return EVP_aes_128_ecb();
		case mys_aes_128_cbc:
			return EVP_aes_128_cbc();
		case mys_aes_128_cfb1:
			return EVP_aes_128_cfb1();
		case mys_aes_128_cfb8:
			return EVP_aes_128_cfb8();
		case mys_aes_128_cfb128:
			return EVP_aes_128_cfb128();
		case mys_aes_128_ofb:
			return EVP_aes_128_ofb();
		case mys_aes_192_ecb:
			return EVP_aes_192_ecb();
		case mys_aes_192_cbc:
			return EVP_aes_192_cbc();
		case mys_aes_192_cfb1:
			return EVP_aes_192_cfb1();
		case mys_aes_192_cfb8:
			return EVP_aes_192_cfb8();
		case mys_aes_192_cfb128:
			return EVP_aes_192_cfb128();
		case mys_aes_192_ofb:
			return EVP_aes_192_ofb();
		case mys_aes_256_ecb:
			return EVP_aes_256_ecb();
		case mys_aes_256_cbc:
			return EVP_aes_256_cbc();
		case mys_aes_256_cfb1:
			return EVP_aes_256_cfb1();
		case mys_aes_256_cfb8:
			return EVP_aes_256_cfb8();
		case mys_aes_256_cfb128:
			return EVP_aes_256_cfb128();
		case mys_aes_256_ofb:
			return EVP_aes_256_ofb();
		default:
			return NULL;
	}
}

static bool 
aes_needs_iv(int mys_block_encryption_mode)
{
	int iv_length;	
	const EVP_CIPHER *cipher = aes_evp_type(mys_block_encryption_mode);
	if (!cipher)
		ereport(ERROR, (errmsg("The block_encryption_mode parameter is incorrect.")));

	iv_length = EVP_CIPHER_iv_length(cipher);
	if (iv_length != 0 && iv_length != AES_IV_SIZE)
		ereport(ERROR, (errmsg("iv_length error")));
	return iv_length != 0 ? true : false;
}

static int
aes_get_size(uint32 source_length, int mys_block_encryption_mode)
{
	size_t block_size;
	const EVP_CIPHER *cipher = aes_evp_type(mys_block_encryption_mode);
	if (!cipher)
		ereport(ERROR, (errmsg("The block_encryption_mode parameter is incorrect.")));

	block_size = EVP_CIPHER_block_size(cipher);

	return block_size > 1 ? block_size * (source_length / block_size) + block_size : source_length;
}

static KDF_Option 
retrieve_kdf_options(char *arg3, char *arg4, char *arg5)
{
	KDF_Option kdf_options = {"", "", "", -1};
	size_t max_size_allowed;

	if (!arg3 || !(*arg3))
		return kdf_options;

	if (strlen(arg3) > MAX_KDF_OPTION_SIZE - 1)
	{
		ereport(ERROR, (errmsg("kdf option value length too long")));
	}

	// KDF function name should be valid
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	if (strcmp(arg3, "pbkdf2_hmac") == 0)
	{
#else
	if (strcmp(arg3, "hkdf") == 0 || strcmp(arg3, "pbkdf2_hmac") == 0)
	{
#endif
		kdf_options.kdf_name = arg3;
	}
	else
	{
		ereport(ERROR, (errmsg("KDF method name is not valid. Please use hkdf or pbkdf2_hmac method name")));
	}

	if (!arg4 || !(*arg4))
		return kdf_options;

	if (strlen(arg4) > MAX_KDF_OPTION_SIZE - 1)
	{
		ereport(ERROR, (errmsg("kdf option value length too long")));
	}
	kdf_options.salt = arg4;

	if (!arg5 || !(*arg5))
		return kdf_options;

	// For hkdf option 2 is info
	// For pbkdf2_hmac option 2 is iterations
	max_size_allowed = MAX_KDF_OPTION_SIZE;
	if (strcmp(kdf_options.kdf_name, "pbkdf2_hmac") == 0)
	{
		// 4 bytes for integer (65535).
		max_size_allowed = 6;
	}

	if (strlen(arg5) > max_size_allowed - 1)
	{
		ereport(ERROR, (errmsg("kdf option value length too long")));
	}
	kdf_options.info_or_iter = arg5;

	if (strcmp(kdf_options.kdf_name, "pbkdf2_hmac") == 0 && kdf_options.kdf_name && kdf_options.salt)
	{
		int iter = atoi(kdf_options.info_or_iter);
		if (iter < MIN_KDF_ITERATIONS_SIZE || iter > MAX_KDF_ITERATIONS_SIZE)
		{
			ereport(ERROR, (errmsg("For KDF method pbkdf2_hmac iterations value less than 1000 or more than 65535 is not allowed due to security reasons. Please provide iterations >= 1000 and iterations < 65535")));
		}
	}
	return kdf_options;
}

static bool
kdf_function_derive_key(char *kdf_name,
						const unsigned char *key,
						const unsigned int key_length,
						unsigned char *rkey,
						unsigned int key_size,
						KDF_Option *kdf_options)
{
	int res = 0;
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	size_t len;
#endif
	if (strcmp(kdf_name, "pbkdf2_hmac") == 0)
	{
		res = PKCS5_PBKDF2_HMAC((const char *)key, key_length,
								(const unsigned char *)kdf_options->salt, 
								strlen(kdf_options->salt),
								kdf_options->iterations, EVP_sha512(), key_size, rkey);
		return res ? true : false;
	}
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
	else if (strcmp(kdf_name, "hkdf") == 0)
	{
		EVP_PKEY_CTX *pctx = NULL;

		/* Set initial key  */
		memset(rkey, 0, key_size);
		pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, NULL);
		if (!pctx)
		{
			return false;
		}
		if (EVP_PKEY_derive_init(pctx) <= 0)
		{
			EVP_PKEY_CTX_free(pctx);
			return false;
		}
#if OPENSSL_VERSION_NUMBER >= 0x30000020L || OPENSSL_VERSION_NUMBER < 0x10101000L
		return false;
#else
		if (EVP_PKEY_CTX_set_hkdf_md(pctx, EVP_sha512()) <= 0)
		{
			EVP_PKEY_CTX_free(pctx);
			return false;
		}
		if (kdf_options->salt)
		{
			if (EVP_PKEY_CTX_set1_hkdf_salt(pctx,
											(const unsigned char *)(kdf_options->salt),
											strlen(kdf_options->salt)) <= 0)
			{
				EVP_PKEY_CTX_free(pctx);
				return false;
			}
		}

		if (kdf_options->info_or_iter)
		{
			if (EVP_PKEY_CTX_add1_hkdf_info(pctx,
											(const unsigned char *)(kdf_options->info_or_iter),
											strlen(kdf_options->info_or_iter)) <= 0)
			{
				EVP_PKEY_CTX_free(pctx);
				return false;
			}
		}

		if (EVP_PKEY_CTX_set1_hkdf_key(pctx, key, key_length) <= 0)
		{
			EVP_PKEY_CTX_free(pctx);
			return false;
		}
#endif /* OPENSSL_VERSION_NUMBER >= 0x30000020L */		
		len = key_size;
		if (EVP_PKEY_derive(pctx, (unsigned char *)rkey,
							(size_t *)(&len)) <= 0)
		{
			EVP_PKEY_CTX_free(pctx);
			return false;
		}
		if (len != key_size)
		{
			EVP_PKEY_CTX_free(pctx);
			return false;
		}
		if (pctx)
		{
			EVP_PKEY_CTX_free(pctx);
		}
		return true;
	}
#endif /* OPENSSL_VERSION_NUMBER >= 0x10100000L */
	else
	{
		ereport(ERROR, (errmsg("kdf name error")));
	}

	/*
	 * never reached, but keep compiler quiet
	 */
	return true;
}

static bool 
create_kdf_key(const unsigned char *key, const unsigned int key_length,
				   unsigned char *rkey, unsigned int rkey_size,
				   KDF_Option *kdf_options)
{
	int nkdf_options;
	char *kdf_name, *kdf_function;
	bool ret;

	if (kdf_options == NULL)
	{
		return false;
	}
	nkdf_options = size_of_kdf_option(kdf_options);

	if (nkdf_options < 1)
	{
		return false;
	}

	kdf_name = kdf_options->kdf_name;
	kdf_function = NULL;

	if (strcmp(kdf_name, "hkdf") == 0)
	{
#if OPENSSL_VERSION_NUMBER >= 0x10100000L
		kdf_function = kdf_name;
		kdf_options->info_or_iter = 0;
#else
		return false;
#endif
	}
	if (strcmp(kdf_name, "pbkdf2_hmac") == 0)
	{
		kdf_options->iterations = MIN_KDF_ITERATIONS_SIZE;
		if (size_of_kdf_option(kdf_options) > 2)
		{
			kdf_options->iterations = atoi(kdf_options->info_or_iter);
			if (kdf_options->iterations < MIN_KDF_ITERATIONS_SIZE ||
				kdf_options->iterations > MAX_KDF_ITERATIONS_SIZE)
			{
				return false;
			}
		}
		kdf_function = kdf_name;
	}

	if (kdf_function == NULL)
	{
		return false;
	}
	ret = kdf_function_derive_key(kdf_name, key, key_length, rkey, rkey_size, kdf_options);

	return ret;
}

static void 
aes_create_key(const unsigned char *key, uint32 key_length, uint8 *rkey, int mys_block_encryption_mode)
{
	const uint32 key_size = aes_opmode_key_sizes[mys_block_encryption_mode] / 8;
	uint8 *rkey_end;							  /* Real key boundary */
	uint8 *ptr;									  /* Start of the real key*/
	uint8 *sptr;								  /* Start of the working key */
	uint8 *key_end = ((uint8 *)key) + key_length; /* Working key boundary*/

	rkey_end = rkey + key_size;

	memset(rkey, 0, key_size); /* Set initial key  */

	for (ptr = rkey, sptr = (uint8 *)key; sptr < key_end; ptr++, sptr++)
	{
		if (ptr == rkey_end)
			/*  Just loop over tmp_key until we used all key */
			ptr = rkey;
		*ptr ^= *sptr;
	}
}

static bool
create_key(unsigned char *rkey, const unsigned char *key,
			uint32 key_length, int mys_block_encryption_mode,
			KDF_Option *kdf_options)
{
	if (size_of_kdf_option(kdf_options))
	{
		const uint32 key_size = aes_opmode_key_sizes[mys_block_encryption_mode] / 8;
		return create_kdf_key(key, key_length, rkey, key_size, kdf_options);
	}

	aes_create_key(key, key_length, rkey, mys_block_encryption_mode);
	return true;
}

static int
aes_encrypt(const unsigned char *source, uint32 source_length,
			unsigned char *dest,
			const unsigned char *key, uint32 key_length,
			int mys_block_encryption_mode, const unsigned char *iv,
			bool padding,
			KDF_Option *kdf_options)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	EVP_CIPHER_CTX stack_ctx;
	EVP_CIPHER_CTX *ctx = &stack_ctx;
#else  /* OPENSSL_VERSION_NUMBER < 0x10100000L */
	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
#endif /* OPENSSL_VERSION_NUMBER < 0x10100000L */
	const EVP_CIPHER *cipher = aes_evp_type(mys_block_encryption_mode);
	int u_len, f_len;
	/* The real key to be used for encryption */
	unsigned char rkey[MAX_AES_KEY_LENGTH / 8];

	if (!create_key(rkey, key, key_length, mys_block_encryption_mode, kdf_options))
	{
		return AES_BAD_DATA;
	}
	if (!ctx || !cipher || (EVP_CIPHER_iv_length(cipher) > 0 && !iv))
		return AES_BAD_DATA;

#if OPENSSL_VERSION_NUMBER < 0x10100000L
	EVP_CIPHER_CTX_init(ctx);
#endif /* OPENSSL_VERSION_NUMBER < 0x10100000L */

	if (!EVP_EncryptInit(ctx, cipher, rkey, iv))
		goto aes_error;
	if (!EVP_CIPHER_CTX_set_padding(ctx, padding))
		goto aes_error;
	if (!EVP_EncryptUpdate(ctx, dest, &u_len, source, source_length))
		goto aes_error;

	if (!EVP_EncryptFinal(ctx, dest + u_len, &f_len))
		goto aes_error; 

#if OPENSSL_VERSION_NUMBER < 0x10100000L
	EVP_CIPHER_CTX_cleanup(ctx);
#else  /* OPENSSL_VERSION_NUMBER < 0x10100000L */
	EVP_CIPHER_CTX_free(ctx);
#endif /* OPENSSL_VERSION_NUMBER < 0x10100000L */
	return u_len + f_len;

aes_error:
	/* need to explicitly clean up the error if we want to ignore it */
	ERR_clear_error();
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	EVP_CIPHER_CTX_cleanup(ctx);
#else  /* OPENSSL_VERSION_NUMBER < 0x10100000L */
	EVP_CIPHER_CTX_free(ctx);
#endif /* OPENSSL_VERSION_NUMBER < 0x10100000L */
	return AES_BAD_DATA;
}

static int 
aes_decrypt(const unsigned char *source, uint32 source_length,
					   unsigned char *dest,
					   const unsigned char *key, uint32 key_length,
					   int mys_block_encryption_mode, const unsigned char *iv,
					   bool padding,
					   KDF_Option *kdf_options)
{
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	EVP_CIPHER_CTX stack_ctx;
	EVP_CIPHER_CTX *ctx = &stack_ctx;
#else  /* OPENSSL_VERSION_NUMBER < 0x10100000L */
	EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
#endif /* OPENSSL_VERSION_NUMBER < 0x10100000L */
	const EVP_CIPHER *cipher = aes_evp_type(mys_block_encryption_mode);
	int u_len, f_len;

	/* The real key to be used for decryption */
	unsigned char rkey[MAX_AES_KEY_LENGTH / 8];

	if (!create_key(rkey, key, key_length, mys_block_encryption_mode, kdf_options))
	{
		return AES_BAD_DATA;
	}
	if (!ctx || !cipher || (EVP_CIPHER_iv_length(cipher) > 0 && !iv))
		return AES_BAD_DATA;

#if OPENSSL_VERSION_NUMBER < 0x10100000L
	EVP_CIPHER_CTX_init(ctx);
#endif /* OPENSSL_VERSION_NUMBER < 0x10100000L */
	if (!EVP_DecryptInit(ctx, aes_evp_type(mys_block_encryption_mode), rkey, iv))
		goto aes_error; /* Error */
	if (!EVP_CIPHER_CTX_set_padding(ctx, padding))
		goto aes_error; /* Error */
	if (!EVP_DecryptUpdate(ctx, dest, &u_len, source, source_length))
		goto aes_error; /* Error */
	if (!EVP_DecryptFinal_ex(ctx, dest + u_len, &f_len))
		goto aes_error; /* Error */

#if OPENSSL_VERSION_NUMBER < 0x10100000L
	EVP_CIPHER_CTX_cleanup(ctx);
#else  /* OPENSSL_VERSION_NUMBER < 0x10100000L */
	EVP_CIPHER_CTX_free(ctx);
#endif /* OPENSSL_VERSION_NUMBER < 0x10100000L */

	return u_len + f_len;

aes_error:
	/* need to explicitly clean up the error if we want to ignore it */
	ERR_clear_error();
#if OPENSSL_VERSION_NUMBER < 0x10100000L
	EVP_CIPHER_CTX_cleanup(ctx);
#else  /* OPENSSL_VERSION_NUMBER < 0x10100000L */
	EVP_CIPHER_CTX_free(ctx);
#endif /* OPENSSL_VERSION_NUMBER < 0x10100000L */
	return AES_BAD_DATA;
}
#endif /* NOT USE_OPENSSL */

Datum
ivymy_aes_encrypt(PG_FUNCTION_ARGS)
{
#if USE_OPENSSL
	char *sptr, *key;
	char *arg3, *arg4, *arg5;
	unsigned char *iv_str;
	char *str_value;
	int rkey_size, key_size, aes_length;
	KDF_Option kdf_options;
	KDF_Option null_kdf_options = {"","", "", -1};
	text *t;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
	{
		PG_RETURN_NULL();
	}

	if (aes_needs_iv(mys_block_encryption_mode))
	{
		/* Begin - bug#8443 */
		if (PG_ARGISNULL(2))
		{
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("Incorrect parameter count in the call to native function 'aes_encrypt'")));
		}
		/* End - bug#8443 */

		t = PG_GETARG_TEXT_PP(2);

		if (VARSIZE_ANY_EXHDR(t) < 16)
		{
			ereport(ERROR,
					(errmsg("The initialization vector supplied to aes_encrypt is too short. Must be at least 16 bytes long")));
		}
	}

	sptr = text_to_cstring(PG_GETARG_TEXT_PP(0)); // String to encrypt
	key = text_to_cstring(PG_GETARG_TEXT_PP(1)); // key

	/* Begin - bug#8443 */
	if (PG_ARGISNULL(2))
		iv_str = NULL;
	else
		iv_str = (unsigned char *) text_to_cstring(PG_GETARG_TEXT_PP(2));
	/* End - bug#8443 */

	arg3 = text_to_cstring(PG_GETARG_TEXT_PP(3));
	arg4 = text_to_cstring(PG_GETARG_TEXT_PP(4));
	arg5 = text_to_cstring(PG_GETARG_TEXT_PP(5));

	kdf_options = retrieve_kdf_options(arg3, arg4, arg5);

	// Calculate result length
	aes_length = aes_get_size(strlen(sptr), mys_block_encryption_mode);
	rkey_size = aes_opmode_key_sizes[mys_block_encryption_mode] / 8;
	key_size = strlen(key);
	if ((key_size > rkey_size) && !size_of_kdf_option(&kdf_options))
	{
		ereport(WARNING, 
				(errmsg("AES key size should be %d bytes length or secure KDF methods hkdf or pbkdf2_hmac should be used, \
						please provide exact AES key size or use KDF methods for better security.", rkey_size)));
	}

	str_value = (char *) palloc(aes_length);
	// finally encrypt directly to allocated buffer.
	if (aes_encrypt((unsigned char *)sptr, strlen(sptr),
					(unsigned char *)str_value,
					(unsigned char *)key, strlen(key),
					mys_block_encryption_mode, iv_str, true,
					(size_of_kdf_option(&kdf_options) > 0) ? &kdf_options : &null_kdf_options) == aes_length)
	{
		PG_RETURN_TEXT_P(cstring_to_text_with_len(str_value, aes_length));
	}
#endif /* NOT USE_OPENSSL */
	PG_RETURN_NULL();
}

Datum 
ivymy_aes_decrypt(PG_FUNCTION_ARGS)
{
#if OPENSSL_VERSION_NUMBER >= 0x100020bfL
	char *sptr, *key;
	char *arg3, *arg4, *arg5;
	unsigned char *iv_str;
	char *str_value;
	int rkey_size, key_size, aes_length;
	KDF_Option kdf_options;
	KDF_Option null_kdf_options = {"","", "", -1};
	text *t;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
	{
		PG_RETURN_NULL();
	}

	if (aes_needs_iv(mys_block_encryption_mode))
	{
		t = PG_GETARG_TEXT_PP(2);

		if (PG_ARGISNULL(2) || (VARSIZE_ANY_EXHDR(t) < 16))
			ereport(ERROR, 
					(errmsg("The initialization vector supplied to aes_encrypt is too short. Must be at least 16 bytes long")));
	}

	sptr = text_to_cstring(PG_GETARG_TEXT_PP(0)); // String to encrypt
	key = text_to_cstring(PG_GETARG_TEXT_PP(1)); // key
	iv_str = (unsigned char*)text_to_cstring(PG_GETARG_TEXT_PP(2));
	arg3 = text_to_cstring(PG_GETARG_TEXT_PP(3));
	arg4 = text_to_cstring(PG_GETARG_TEXT_PP(4));
	arg5 = text_to_cstring(PG_GETARG_TEXT_PP(5));


	kdf_options = retrieve_kdf_options(arg3, arg4, arg5);

	rkey_size = aes_opmode_key_sizes[mys_block_encryption_mode] / 8;
	key_size = strlen(key);
	if ((key_size > rkey_size) && !size_of_kdf_option(&kdf_options))
	{
		ereport(WARNING, 
				(errmsg("AES key size should be %d bytes length or secure KDF methods hkdf or pbkdf2_hmac should be used, \
						please provide exact AES key size or use KDF methods for better security.", rkey_size)));
	}


	str_value = (char *) palloc(strlen(sptr));
	// finally encrypt directly to allocated buffer.


	aes_length = aes_decrypt((unsigned char *)sptr, strlen(sptr),
					   (unsigned char *)str_value,
					   (unsigned char *)key, strlen(key),
					   mys_block_encryption_mode, iv_str, true,
					   (size_of_kdf_option(&kdf_options) > 0) ? &kdf_options : &null_kdf_options);
	if (aes_length > 0)
		PG_RETURN_TEXT_P(cstring_to_text_with_len(str_value, aes_length));
#endif /* OPENSSL_VERSION_NUMBER >= 0x100020bfL */
	PG_RETURN_NULL();
}

/* Begin Bug#Z7500 */
Datum
bytea_aes_decrypt(PG_FUNCTION_ARGS)
{
#if OPENSSL_VERSION_NUMBER >= 0x100020bfL

	char *key;
	char *arg3, *arg4, *arg5;
	unsigned char *iv_str;
	char *str_value;
	int rkey_size, key_size, aes_length;
	KDF_Option kdf_options;
	KDF_Option null_kdf_options = {"","", "", -1};
	bytea *bytea_data = PG_GETARG_BYTEA_PP(0);
	size_t len = VARSIZE_ANY_EXHDR(bytea_data);
	char *str = VARDATA_ANY(bytea_data);
	char *sptr = palloc(len + 1);
	text *t;

	memcpy(sptr, str, len);
	sptr[len] = '\0';

	if (aes_needs_iv(mys_block_encryption_mode))
	{
		t = PG_GETARG_TEXT_PP(2);

		if (VARSIZE_ANY_EXHDR(t) < 16)
			ereport(ERROR, 
					(errmsg("The initialization vector supplied to aes_encrypt is too short. Must be at least 16 bytes long")));
	}

	key = text_to_cstring(PG_GETARG_TEXT_PP(1)); // key
	iv_str = (unsigned char*)text_to_cstring(PG_GETARG_TEXT_PP(2));
	arg3 = text_to_cstring(PG_GETARG_TEXT_PP(3));
	arg4 = text_to_cstring(PG_GETARG_TEXT_PP(4));
	arg5 = text_to_cstring(PG_GETARG_TEXT_PP(5));


	kdf_options = retrieve_kdf_options(arg3, arg4, arg5);

	rkey_size = aes_opmode_key_sizes[mys_block_encryption_mode] / 8;
	key_size = strlen(key);
	if ((key_size > rkey_size) && !size_of_kdf_option(&kdf_options))
	{
		ereport(WARNING,
				(errmsg("AES key size should be %d bytes length or secure KDF methods hkdf or pbkdf2_hmac should be used, \
						please provide exact AES key size or use KDF methods for better security.", rkey_size)));
	}


	str_value = (char *) palloc(len);
	// finally encrypt directly to allocated buffer.


	aes_length = aes_decrypt((unsigned char *)sptr, len,
							(unsigned char *)str_value,
							(unsigned char *)key, strlen(key),
							mys_block_encryption_mode, iv_str, true,
							(size_of_kdf_option(&kdf_options) > 0) ? &kdf_options : &null_kdf_options);
	if (aes_length > 0)
		PG_RETURN_TEXT_P(cstring_to_text_with_len(str_value, aes_length));
#endif /* OPENSSL_VERSION_NUMBER >= 0x100020bfL */
	PG_RETURN_NULL();
}
/* End Bug#Z7500 */

Datum
random_bytes(PG_FUNCTION_ARGS)
{
	int len = PG_GETARG_INT32(0);
	bytea *res;

	if (len < 1 || len > MAX_RANDOM_BYTES_BUFFER)
	{
		ereport(ERROR,
				(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
				 errmsg("length value is out of range in 'random_bytes'")));
	}

	res = palloc(VARHDRSZ + len);
	SET_VARSIZE(res, VARHDRSZ + len);

	if (!pg_strong_random(VARDATA(res), len))
	{
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("could not generate a random number")));
	}

	PG_RETURN_BYTEA_P(res);
}