/*-------------------------------------------------------------------------
 *
 * crypt.h
 *	  Interface to libpq/crypt.c
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/libpq/crypt.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_CRYPT_H
#define PG_CRYPT_H

#include "datatype/timestamp.h"
#include "libpq/libpq.h"

/*
 * Types of password hashes or secrets.
 *
 * Plaintext passwords can be passed in by the user, in a CREATE/ALTER USER
 * command. They will be encrypted to MD5 or SCRAM-SHA-256 format, before
 * storing on-disk, so only MD5 and SCRAM-SHA-256 passwords should appear
 * in pg_authid.rolpassword. They are also the allowed values for the
 * password_encryption GUC.
 */
typedef enum PasswordType
{
	PASSWORD_TYPE_PLAINTEXT = 0,
	PASSWORD_TYPE_MD5,
	PASSWORD_TYPE_SCRAM_SHA_256,
#ifdef USE_GMSSL
	PASSWORD_TYPE_SM3, //ReqID:SRS-SECURITY-IDENTITYAUTH
	PASSWORD_TYPE_SCRAM_SM3, // ReqID:SRS-SECURITY-IDENTITYAUTH
#endif
	/* Begin - Req: SRS-MYSQL-PROTOCOL */
	PASSWORD_TYPE_MYS_NATIVE,       // mysql_native_password
	PASSWORD_TYPE_MYS_CACHING_SHA2  // caching_sha2_password
	/* End - Req: SRS-MYSQL-PROTOCOL */
} PasswordType;

extern PasswordType get_password_type(const char *shadow_pass);
extern char *encrypt_password(PasswordType target_type, const char *role,
							  const char *password);

extern char *get_role_password(const char *role, char **logdetail);

extern int	md5_crypt_verify(const char *role, const char *shadow_pass,
							 const char *client_pass, const char *md5_salt,
							 int md5_salt_len, char **logdetail);
/* Begin - ReqID:SRS-SECURITY-IDENTITYAUTH */
#ifdef USE_GMSSL
extern int sm3_crypt_verify(const char *role, const char *shadow_pass,
				 const char *client_pass,
				 const char *sm3_salt, int sm3_salt_len,
				 char **logdetail);
int
md5_sm3_crypt_verify(const char *role, const char *shadow_pass,
				 const char *client_pass,
				 const char *md5_salt, int md5_salt_len,
				 char **logdetail);

int
sm3_md5_crypt_verify(const char *role, const char *shadow_pass,
				 const char *client_pass,
				 const char *md5_salt, int md5_salt_len,
				 char **logdetail);
#endif
/* End - ReqID:SRS-SECURITY-IDENTITYAUTH */

extern int	plain_crypt_verify(const char *role, const char *shadow_pass,
							   const char *client_pass, char **logdetail);

#endif
