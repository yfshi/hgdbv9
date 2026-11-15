/*-------------------------------------------------------------------------
 *
 * tecc_sm2_algo.h
 *	  SM2 envelope crypto primitives (cross-version, no OpenSSL 3.x dependency)
 *
 * Self-contained implementations of SM2 public-key encryption / decryption
 * (GM/T 0003.4-2012 + GM/T 0003.5-2012), byte-compatible with OpenSSL's
 * ossl_sm2_encrypt/ossl_sm2_decrypt envelope (ASN.1 DER SM2_Ciphertext).
 *
 * The EC arithmetic uses OpenSSL's low-level EC_KEY / EC_GROUP / EC_POINT /
 * BN primitives, which are present in OpenSSL 1.0.2, 1.1.x and 3.x alike.
 * No OpenSSL 3.x provider API (EVP_PKEY_fromdata / OSSL_PARAM) is used, so
 * the same source compiles against 1.0.2/1.1.1/3.0.  SM3 hashing and the
 * SM2 KDF are self-implemented via tecc_sm3_internal (pure C).
 *
 * Authored by <wangxp@highgo.com>, 2026.
 *
 * src/include/common/tecc_algorithm/tecc_sm2_algo.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_SM2_ALGO_H
#define TECC_SM2_ALGO_H

#include <stddef.h>
#include <openssl/ec.h>
#include <openssl/evp.h>

/* Status codes (mirror tecc_key/tecc_sm2.h so the wrapper can pass through) */
#define TECC_SM2_ALGO_OK          0
#define TECC_SM2_ALGO_ERR_NULL    1
#define TECC_SM2_ALGO_ERR_MEMORY  2
#define TECC_SM2_ALGO_ERR_CRYPTO  3
#define TECC_SM2_ALGO_ERR_INVALID 4

/* sm2p256v1 uncompressed point size (04 || X || Y) */
#define TECC_SM2_POINT_UNCOMP 65
/* SM3 digest size */
#define TECC_SM2_C3_LEN       32

/*
 * Build an OpenSSL EC_KEY holding the SM2 public key from a 65-byte
 * uncompressed point (04||X||Y) on sm2p256v1.  Uses explicit curve params,
 * so no NID_sm2 (absent in OpenSSL 1.0.2) is required.
 * Returns a new EC_KEY the caller must EC_KEY_free, or NULL on error.
 */
EC_KEY *tecc_sm2_algo_pubkey_from_point(const unsigned char *pub_point,
										size_t pub_len);

/*
 * Build an OpenSSL EC_KEY from a DER-encoded SM2 private key.  Both the
 * ECPrivateKey (SEC1/RFC 5915) and PKCS#8 containers are accepted, whichever
 * tecc_ktool produced; the key is rebuilt on the explicit sm2p256v1 params,
 * so no OpenSSL version's SM2 key handling is involved (EVP_PKEY_get1_EC_KEY
 * returns NULL for SM2 keys on OpenSSL 3.0, so it must not be used here).
 * Returns a new EC_KEY (with the public key derived from the container or
 * computed from the private scalar) the caller frees with EC_KEY_free,
 * or NULL on error.
 */
EC_KEY *tecc_sm2_algo_privkey_from_der(const unsigned char *priv_der,
									   size_t priv_len);

/*
 * SM2 encrypt: ciphertext envelope is allocated by the caller-supplied
 * OpenSSL style (OPENSSL_malloc).  Output DER:
 *   SEQUENCE { INTEGER C1x, INTEGER C1y,
 *              OCTET STRING C3(32), OCTET STRING C2(plain_len) }
 *
 * pub_point: 65-byte uncompressed public point.  user_id is unused for
 * plain C1C3C2 encryption (GM/T 0003.5), kept for interface symmetry.
 * Returns TECC_SM2_ALGO_OK on success.
 */
int tecc_sm2_algo_encrypt(const unsigned char *pub_point, size_t pub_len,
						  const unsigned char *plain, size_t plain_len,
						  unsigned char **envelope, size_t *env_len);

/*
 * SM2 decrypt: priv_der is the PKCS#8 DER private key.  envelope/ *env_len
 * is the ASN.1 DER SM2_Ciphertext above.  On success *plain is
 * OPENSSL_allocated and *plain_len set.  Returns TECC_SM2_ALGO_OK.
 */
int tecc_sm2_algo_decrypt(const unsigned char *priv_der, size_t priv_len,
						  const unsigned char *envelope, size_t env_len,
						  unsigned char **plain, size_t *plain_len);

/*
 * Derive the 65-byte uncompressed public point (04||X||Y) from a PKCS#8
 * DER SM2 private key.  On success *pub_point is OPENSSL_allocated (65B).
 */
int tecc_sm2_algo_derive_pubkey(const unsigned char *priv_der, size_t priv_len,
								unsigned char **pub_point, size_t *pub_len);

#endif							/* TECC_SM2_ALGO_H */
