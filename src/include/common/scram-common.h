/*-------------------------------------------------------------------------
 *
 * scram-common.h
 *		Declarations for helper functions used for SCRAM authentication
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/common/scram-common.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef SCRAM_COMMON_H
#define SCRAM_COMMON_H

#include "common/cryptohash.h"
#include "common/sha2.h"

/* Name of SCRAM mechanisms per IANA */
#define SCRAM_SHA_256_NAME "SCRAM-SHA-256"
#define SCRAM_SHA_256_PLUS_NAME "SCRAM-SHA-256-PLUS"	/* with channel binding */
/* Begin - ReqID:SRS-SECURITY-IDENTITYAUTH */
#ifdef USE_GMSSL
#define SCRAM_SM3_NAME "SCRAM-SM3"
#define SCRAM_SM3_PLUS_NAME "SCRAM-SM3-PLUS"	/* with channel binding */
#define SCRAM_SM3_SHA256_NAME "SCRAM-SM3-SHA256"
#define SCRAM_SHA256_SM3_NAME "SCRAM-SHA256-SM3"
#endif
/* End - ReqID:SRS-SECURITY-IDENTITYAUTH */
/* Length of SCRAM keys (client and server) */
#define SCRAM_KEY_LEN				PG_SHA256_DIGEST_LENGTH

/* length of HMAC */
#define SHA256_HMAC_B				PG_SHA256_BLOCK_LENGTH

/*
 * Size of random nonce generated in the authentication exchange.  This
 * is in "raw" number of bytes, the actual nonces sent over the wire are
 * encoded using only ASCII-printable characters.
 */
#define SCRAM_RAW_NONCE_LEN			18

/*
 * Length of salt when generating new secrets, in bytes.  (It will be stored
 * and sent over the wire encoded in Base64.)  16 bytes is what the example in
 * RFC 7677 uses.
 */
#define SCRAM_DEFAULT_SALT_LEN		16

/*
 * Default number of iterations when generating secret.  Should be at least
 * 4096 per RFC 7677.
 */
#define SCRAM_DEFAULT_ITERATIONS	4096
/* Begin - ReqID:SRS-SECURITY-IDENTITYAUTH */
extern int	scram_SaltedPassword(const char *scram_type, const char *password, const char *salt,
								 int saltlen, int iterations, uint8 *result);
extern int	scram_H(const char *scram_type, const uint8 *str, int len, uint8 *result);
extern int	scram_ClientKey(const char *scram_type, const uint8 *salted_password, uint8 *result);
extern int	scram_ServerKey(const char *scram_type, const uint8 *salted_password, uint8 *result);

extern char *scram_build_secret(const char *scram_type, const char *salt, int saltlen, int iterations,
								const char *password);
/* End - ReqID:SRS-SECURITY-IDENTITYAUTH */
#endif							/* SCRAM_COMMON_H */
