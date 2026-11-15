/*-------------------------------------------------------------------------
 *
 * he_operators.h
 *	  Homomorphic encryption operator functions
 *
 * Defines server-side operators for EC-ElGamal (additive) and
 * finite field ElGamal (multiplicative) homomorphic operations.
 *
 * All operators use the DEK public key stored in CipherDataType's
 * dek_dist_oid field to perform server-side constant encryption.
 *
 * Portions Copyright (c) 2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 2021, HighGo Software Co.,Ltd.
 *
 * src/include/utils/tecc/he_operators.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef HE_OPERATORS_H
#define HE_OPERATORS_H

#include "postgres.h"
#include "fmgr.h"
#include "utils/cipherdatatype.h"
#include <openssl/bn.h>

/* ==================== Operator Functions ==================== */

/* int4_enc operators */
extern Datum int4_enc_he_add(PG_FUNCTION_ARGS);
extern Datum int4_enc_he_add_int2(PG_FUNCTION_ARGS);
extern Datum int4_enc_he_sub(PG_FUNCTION_ARGS);
extern Datum int4_enc_he_mul(PG_FUNCTION_ARGS);
extern Datum int4_enc_he_mul_scalar(PG_FUNCTION_ARGS);
extern Datum int4_enc_he_add_scalar(PG_FUNCTION_ARGS);
extern Datum int4_enc_he_sub_scalar(PG_FUNCTION_ARGS);
extern Datum int4_he_add_scalar(PG_FUNCTION_ARGS);
extern Datum int4_he_mul_scalar(PG_FUNCTION_ARGS);

/* int8_enc operators */
extern Datum int8_enc_he_add(PG_FUNCTION_ARGS);
extern Datum int8_enc_he_sub(PG_FUNCTION_ARGS);
extern Datum int8_enc_he_mul(PG_FUNCTION_ARGS);
extern Datum int8_enc_he_mul_scalar(PG_FUNCTION_ARGS);
extern Datum int8_enc_he_add_scalar(PG_FUNCTION_ARGS);
extern Datum int8_enc_he_sub_scalar(PG_FUNCTION_ARGS);
extern Datum int8_he_add_scalar(PG_FUNCTION_ARGS);
extern Datum int8_he_mul_scalar(PG_FUNCTION_ARGS);

/* int2_enc operators */
extern Datum int2_enc_he_add(PG_FUNCTION_ARGS);
extern Datum int2_enc_he_sub(PG_FUNCTION_ARGS);
extern Datum int2_enc_he_mul(PG_FUNCTION_ARGS);
extern Datum int2_enc_he_mul_scalar(PG_FUNCTION_ARGS);
extern Datum int2_enc_he_add_scalar(PG_FUNCTION_ARGS);
extern Datum int2_enc_he_sub_scalar(PG_FUNCTION_ARGS);
extern Datum int2_he_add_scalar(PG_FUNCTION_ARGS);
extern Datum int2_he_mul_scalar(PG_FUNCTION_ARGS);

/* ReqID:TECC4.0 - int2_enc scalar operators with int4 (PG type promotion) */
extern Datum int2_enc_he_add_scalar4(PG_FUNCTION_ARGS);
extern Datum int2_enc_he_sub_scalar4(PG_FUNCTION_ARGS);
extern Datum int2_enc_he_mul_scalar4(PG_FUNCTION_ARGS);
extern Datum int4_he_add_scalar2e(PG_FUNCTION_ARGS);
extern Datum int4_he_sub_scalar2e(PG_FUNCTION_ARGS);
extern Datum int4_he_mul_scalar2e(PG_FUNCTION_ARGS);

/* ReqID:TECC4.0 - Unary negation and left-constant subtraction */
extern Datum int2_enc_he_neg(PG_FUNCTION_ARGS);
extern Datum int4_enc_he_neg(PG_FUNCTION_ARGS);
extern Datum int8_enc_he_neg(PG_FUNCTION_ARGS);
extern Datum int2_he_sub_scalar(PG_FUNCTION_ARGS);
extern Datum int4_he_sub_scalar(PG_FUNCTION_ARGS);
extern Datum int8_he_sub_scalar(PG_FUNCTION_ARGS);

/* ==================== Public Key Cache ==================== */

/*
 * Initialize the per-process HE public key cache.
 * The cache maps dek_dist_oid + key_algo → parsed key material.
 * Should be called once per process startup.
 */
extern void he_pubkey_cache_init(void);

/*
 * Look up a public key by DEK OID.
 * Returns a pointer to cached BIGNUM data (EC group, ElGamal p/g/h, etc.)
 * The caller must not free the returned data.
 *
 * @param dek_oid DEK OID to look up
 * @param prefer_ec true → prefer EC-ElGamal (for he-add/he-sub),
 *                  false → prefer finite field ElGamal (for he-mul)
 * @param is_ec_elgamal Output: true if EC-ElGamal key, false if finite field ElGamal
 * @param pub_key Output: public key pointer (EC_POINT* for EC-ElGamal, BIGNUM* for ElGamal)
 * @param group Output: EC_GROUP* for EC-ElGamal (NULL for ElGamal)
 * @param p Output: BIGNUM* prime modulus for ElGamal (NULL for EC-ElGamal)
 * @param g Output: BIGNUM* generator for ElGamal (NULL for EC-ElGamal)
 * @return 0 on success, negative on error
 */
extern int he_get_pubkey(Oid dek_oid,
						 bool prefer_ec,
						 bool *is_ec_elgamal,
						 void **pub_key,
						 void **group,
						 BIGNUM **p,
						 BIGNUM **q,
						 BIGNUM **g);

/*
 * Invalidate the public key cache.
 * Called when hg_dek or hg_dek_args entries change.
 */
extern void he_pubkey_cache_invalidate(void);

/* ==================== Public Key Fetching ==================== */

/*
 * Fetch HE public key from hg_dek by DEK OID.
 * The DEK entry contains the public key (as BIGNUM for EC-ElGamal
 * or (p, g, h) for finite field ElGamal).
 *
 * This is called on cache miss to populate the cache.
 *
 * @param dek_oid DEK OID
 * @param is_ec_elgamal Output: true if EC-ElGamal key
 * @param pub_key_bytes Output: allocated buffer with public key bytes
 * @param pub_key_len Output: length of public key bytes
 * @return 0 on success, negative on error
 */
extern int he_fetch_pubkey_from_dek(Oid dek_oid,
									bool *is_ec_elgamal,
									uint8_t **pub_key_bytes,
									int *pub_key_len);

/* ReqID:TECC4.0 - Homomorphic power (^ operator / power function) via
 * repeated FF-ElGamal multiplication. int2/int4/int8 encrypted types. */
extern Datum int2_enc_pow(PG_FUNCTION_ARGS);
extern Datum int2_enc_power(PG_FUNCTION_ARGS);
extern Datum int4_enc_pow(PG_FUNCTION_ARGS);
extern Datum int4_enc_power(PG_FUNCTION_ARGS);
extern Datum int8_enc_pow(PG_FUNCTION_ARGS);
extern Datum int8_enc_power(PG_FUNCTION_ARGS);

#endif /* HE_OPERATORS_H */
