/*-------------------------------------------------------------------------
 *
 * plsqlwrap_crypto.h
 *	 Shared PL/SQL source wrapping cryptography.
 *
 *-------------------------------------------------------------------------
 */
/* Begin - Reqid:SRS-PLPGSQL-HIDDEN*/
#ifndef PLSQLWRAP_CRYPTO_H
#define PLSQLWRAP_CRYPTO_H

#include <stddef.h>

#define PG_PLSQL_WRAP_MAGIC "ivorysql wrapped v1\n"
#define PG_PLSQL_WRAP_AES_PREFIX "aes:"
#define PG_PLSQL_WRAP_SM4_PREFIX "sm4:"

/* Identify the cipher encoded in a wrapped PL/SQL payload. */
typedef enum PgPlsqlWrapCipher
{
	PG_PLSQL_WRAP_CIPHER_AES,
	PG_PLSQL_WRAP_CIPHER_SM4
} PgPlsqlWrapCipher;

extern char *pg_plsql_wrap_encrypt(const char *src,
							  PgPlsqlWrapCipher cipher_type,
							  const char **error_message);
extern char *pg_plsql_wrap_quote_payload(const char *wrapped);
extern char *pg_plsql_wrap_decrypt(const char *payload, size_t payload_len,
							  PgPlsqlWrapCipher cipher_type,
							  const char **error_message);

#endif							/* PLSQLWRAP_CRYPTO_H */
/* End - Reqid:SRS-PLPGSQL-HIDDEN*/
