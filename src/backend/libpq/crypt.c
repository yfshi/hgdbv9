/*-------------------------------------------------------------------------
 *
 * crypt.c
 *	  Functions for dealing with encrypted passwords stored in
 *	  pg_authid.rolpassword.
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/backend/libpq/crypt.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <unistd.h>

#include "catalog/pg_authid.h"
#include "common/md5.h"
#include "common/scram-common.h"
#include "libpq/crypt.h"
#include "libpq/scram.h"
#include "miscadmin.h"
#include "utils/builtins.h"
#include "utils/syscache.h"
#include "utils/timestamp.h"
#include "utils/ora_compatible.h"
#include "utils/guc.h"	
#include "commands/user.h"
#include "commands/dbcommands.h"

/* Begin - ReqID:SRS-SECURITY-IDENTITYAUTH */
#ifdef USE_GMSSL
#include "common/sm3.h"
#endif
/* End - ReqID:SRS-SECURITY-IDENTITYAUTH */

/*
 * Fetch stored password for a user, for authentication.
 *
 * On error, returns NULL, and stores a palloc'd string describing the reason,
 * for the postmaster log, in *logdetail.  The error reason should *not* be
 * sent to the client, to avoid giving away user information!
 */
char *
get_role_password(const char *role, char **logdetail)
{
	TimestampTz vuntil = 0;
	HeapTuple	roleTup;
	Datum		datum;
	bool		isnull;
	char	   *shadow_pass;

	roleTup = SearchSysCache1(AUTHNAME, CStringGetDatum(role));
	if (!HeapTupleIsValid(roleTup))
	{
		*logdetail = psprintf(_("Role \"%s\" does not exist."),
							  role);
		return NULL;			/* no such user */
	}

	datum = SysCacheGetAttr(AUTHNAME, roleTup,
							Anum_pg_authid_rolpassword, &isnull);
	if (isnull)
	{
		ReleaseSysCache(roleTup);
		*logdetail = psprintf(_("User \"%s\" has no password assigned."),
							  role);
		return NULL;			/* user has no password */
	}
	shadow_pass = TextDatumGetCString(datum);

	datum = SysCacheGetAttr(AUTHNAME, roleTup,
							Anum_pg_authid_rolvaliduntil, &isnull);
	if (!isnull)
		vuntil = DatumGetTimestampTz(datum);

	ReleaseSysCache(roleTup);

	/*
	 * Password OK, but check to be sure we are not past rolvaliduntil
	 */
	if (!isnull && vuntil < GetCurrentTimestamp())
	{
		*logdetail = psprintf(_("User \"%s\" has an expired password."),
							  role);
		return NULL;
	}

	return shadow_pass;
}

/*
 * What kind of a password type is 'shadow_pass'?
 */
PasswordType
get_password_type(const char *shadow_pass)
{
	char	   *encoded_salt;
	int			iterations;
	uint8		stored_key[SCRAM_KEY_LEN];
	uint8		server_key[SCRAM_KEY_LEN];

	if (strncmp(shadow_pass, "md5", 3) == 0 &&
		strlen(shadow_pass) == MD5_PASSWD_LEN &&
		strspn(shadow_pass + 3, MD5_PASSWD_CHARSET) == MD5_PASSWD_LEN - 3)
		return PASSWORD_TYPE_MD5;

	if (parse_scram_secret("SCRAM-SHA-256", shadow_pass, &iterations, &encoded_salt,
						   stored_key, server_key))
		return PASSWORD_TYPE_SCRAM_SHA_256;
	/* Begin - ReqID:SRS-SECURITY-IDENTITYAUTH */
#ifdef USE_GMSSL
	if (strncmp(shadow_pass, "sm3", 3) == 0 &&
		strlen(shadow_pass) == SM3_PASSWD_LEN &&
		strspn(shadow_pass + 3, SM3_PASSWD_CHARSET) == SM3_PASSWD_LEN - 3)
		return PASSWORD_TYPE_SM3;
	if (parse_scram_secret("SCRAM-SM3", shadow_pass, &iterations, &encoded_salt,
						   stored_key, server_key))
		return PASSWORD_TYPE_SCRAM_SM3;
#endif
	/* End - ReqID:SRS-SECURITY-IDENTITYAUTH */

	/* Begin - Req: SRS-MYSQL-PROTOCOL */
	if (strncmp(shadow_pass, "mysql_native_password", 21) == 0)
	{
		return PASSWORD_TYPE_MYS_NATIVE;
	}

#ifdef USE_OPENSSL
    if (strncmp(shadow_pass, "caching_sha2_password", 21) == 0)
    {
		return PASSWORD_TYPE_MYS_CACHING_SHA2;
	}
#endif
	/* End - Req: SRS-MYSQL-PROTOCOL */

	return PASSWORD_TYPE_PLAINTEXT;
}

/*
 * Given a user-supplied password, convert it into a secret of
 * 'target_type' kind.
 *
 * If the password is already in encrypted form, we cannot reverse the
 * hash, so it is stored as it is regardless of the requested type.
 */
char *
encrypt_password(PasswordType target_type, const char *role,
				 const char *password)
{
	PasswordType guessed_type = get_password_type(password);
	char	   *encrypted_password;

	if (guessed_type != PASSWORD_TYPE_PLAINTEXT)
	{
		/*
		 * Cannot convert an already-encrypted password from one format to
		 * another, so return it as it is.
		 */
		return pstrdup(password);
	}

	switch (target_type)
	{
		case PASSWORD_TYPE_MD5:
			encrypted_password = palloc(MD5_PASSWD_LEN + 1);

			if (!pg_md5_encrypt(password, role, strlen(role),
								encrypted_password))
				elog(ERROR, "password encryption failed");
			return encrypted_password;
#ifdef USE_GMSSL
		case PASSWORD_TYPE_SM3: // ReqID:SRS-SECURITY-IDENTITYAUTH
			encrypted_password = palloc(SM3_PASSWD_LEN + 1);

			if (!pg_sm3_encrypt(password, role, strlen(role),
								encrypted_password))
				elog(ERROR, "password encryption failed");
			return encrypted_password;

		case PASSWORD_TYPE_SCRAM_SM3: //  ReqID:SRS-SECURITY-IDENTITYAUTH
			return pg_be_scram_build_secret("SCRAM-SM3", password);
#endif
		case PASSWORD_TYPE_SCRAM_SHA_256:
			return pg_be_scram_build_secret("SCRAM-SHA-256", password);

		case PASSWORD_TYPE_PLAINTEXT:
			elog(ERROR, "cannot encrypt password with 'plaintext'");
			return NULL; /* make compile quiet */
			
		/* Begin - Req: SRS-MYSQL-PROTOCOL */
		case PASSWORD_TYPE_MYS_NATIVE:
			if (compatible_mode != MYS_MODE || mysql_encrypt_password_hook == NULL)
				elog(ERROR, "cannot encrypt password with 'mysql_native_password'");
			
			return mysql_encrypt_password_hook(target_type, role, password);

#ifdef USE_OPENSSL		
		case PASSWORD_TYPE_MYS_CACHING_SHA2:
			if (compatible_mode != MYS_MODE || mysql_encrypt_password_hook == NULL)
				elog(ERROR, "cannot encrypt password with 'caching_sha2_password'");

		return mysql_encrypt_password_hook(target_type, role, password);
#endif
		/* End - Req: SRS-MYSQL-PROTOCOL */
	}

	/*
	 * This shouldn't happen, because the above switch statements should
	 * handle every combination of source and target password types.
	 */
	elog(ERROR, "cannot encrypt password to requested type");
	return NULL;				/* keep compiler quiet */
}

/*
 * Check MD5 authentication response, and return STATUS_OK or STATUS_ERROR.
 *
 * 'shadow_pass' is the user's correct password or password hash, as stored
 * in pg_authid.rolpassword.
 * 'client_pass' is the response given by the remote user to the MD5 challenge.
 * 'md5_salt' is the salt used in the MD5 authentication challenge.
 *
 * In the error case, optionally store a palloc'd string at *logdetail
 * that will be sent to the postmaster log (but not the client).
 */
int
md5_crypt_verify(const char *role, const char *shadow_pass,
				 const char *client_pass,
				 const char *md5_salt, int md5_salt_len,
				 char **logdetail)
{
	int			retval;
	char		crypt_pwd[MD5_PASSWD_LEN + 1];

	Assert(md5_salt_len > 0);

	if (get_password_type(shadow_pass) != PASSWORD_TYPE_MD5)
	{
		/* incompatible password hash format. */
		*logdetail = psprintf(_("User \"%s\" has a password that cannot be used with MD5 authentication."),
							  role);
		return STATUS_ERROR;
	}

	/*
	 * Compute the correct answer for the MD5 challenge.
	 *
	 * We do not bother setting logdetail for any pg_md5_encrypt failure
	 * below: the only possible error is out-of-memory, which is unlikely, and
	 * if it did happen adding a psprintf call would only make things worse.
	 */
	/* stored password already encrypted, only do salt */
	if (!pg_md5_encrypt(shadow_pass + strlen("md5"),
						md5_salt, md5_salt_len,
						crypt_pwd))
	{
		return STATUS_ERROR;
	}

	if (strcmp(client_pass, crypt_pwd) == 0)
		retval = STATUS_OK;
	else
	{
		*logdetail = psprintf(_("Password does not match for user \"%s\"."),
							  role);
		retval = STATUS_ERROR;
	}

	return retval;
}

/* Begin - ReqID:SRS-SECURITY-IDENTITYAUTH */
#ifdef USE_GMSSL
int
sm3_crypt_verify(const char *role, const char *shadow_pass,
				 const char *client_pass,
				 const char *sm3_salt, int sm3_salt_len,
				 char **logdetail)
{
	int			retval;
	char		crypt_pwd[SM3_PASSWD_LEN + 1];

	Assert(sm3_salt_len > 0);

	if (get_password_type(shadow_pass) != PASSWORD_TYPE_SM3)
	{
		/* incompatible password hash format. */
		*logdetail = psprintf(_("User \"%s\" has a password that cannot be used with SM3 authentication."),
							  role);
		return STATUS_ERROR;
	}

	/*
	 * Compute the correct answer for the MD5 challenge.
	 *
	 * We do not bother setting logdetail for any pg_md5_encrypt failure
	 * below: the only possible error is out-of-memory, which is unlikely, and
	 * if it did happen adding a psprintf call would only make things worse.
	 */
	/* stored password already encrypted, only do salt */
	if (!pg_sm3_encrypt(shadow_pass + strlen("sm3"),
						sm3_salt, sm3_salt_len,
						crypt_pwd))
	{
		return STATUS_ERROR;
	}

	if (strcmp(client_pass, crypt_pwd) == 0)
		retval = STATUS_OK;
	else
	{
		*logdetail = psprintf(_("Password does not match for user \"%s\"."),
							  role);
		retval = STATUS_ERROR;
	}

	return retval;
}

int
md5_sm3_crypt_verify(const char *role, const char *shadow_pass,
				 const char *client_pass,
				 const char *md5_salt, int md5_salt_len,
				 char **logdetail)
{
	int			retval;
	char		crypt_pwd[SM3_PASSWD_LEN + 1];

	Assert(md5_salt_len > 0);

	if (get_password_type(shadow_pass) != PASSWORD_TYPE_MD5)
	{
		/* incompatible password hash format. */
		*logdetail = psprintf(_("User \"%s\" has a password that cannot be used with MD5 authentication."),
							  role);
		return STATUS_ERROR;
	}

	/*
	 * Compute the correct answer for the MD5 challenge.
	 *
	 * We do not bother setting logdetail for any pg_md5_encrypt failure
	 * below: the only possible error is out-of-memory, which is unlikely, and
	 * if it did happen adding a psprintf call would only make things worse.
	 */
	/* stored password already encrypted, only do salt */
	if (!pg_sm3_encrypt(shadow_pass + strlen("sm3"),
						md5_salt, md5_salt_len,
						crypt_pwd))
	{
		return STATUS_ERROR;
	}

	if (strcmp(client_pass, crypt_pwd) == 0)
		retval = STATUS_OK;
	else
	{
		*logdetail = psprintf(_("Password does not match for user \"%s\"."),
							  role);
		retval = STATUS_ERROR;
	}

	return retval;
}

int
sm3_md5_crypt_verify(const char *role, const char *shadow_pass,
				 const char *client_pass,
				 const char *md5_salt, int md5_salt_len,
				 char **logdetail)
{
	int			retval;
	char		crypt_pwd[MD5_PASSWD_LEN + 1];

	Assert(md5_salt_len > 0);

	if (get_password_type(shadow_pass) != PASSWORD_TYPE_SM3)
	{
		/* incompatible password hash format. */
		*logdetail = psprintf(_("User \"%s\" has a password that cannot be used with MD5 authentication."),
							  role);
		return STATUS_ERROR;
	}

	/*
	 * Compute the correct answer for the MD5 challenge.
	 *
	 * We do not bother setting logdetail for any pg_md5_encrypt failure
	 * below: the only possible error is out-of-memory, which is unlikely, and
	 * if it did happen adding a psprintf call would only make things worse.
	 */
	/* stored password already encrypted, only do salt */
	if (!pg_md5_encrypt(shadow_pass + strlen("md5"),
						md5_salt, md5_salt_len,
						crypt_pwd))
	{
		return STATUS_ERROR;
	}

	if (strcmp(client_pass, crypt_pwd) == 0)
		retval = STATUS_OK;
	else
	{
		*logdetail = psprintf(_("Password does not match for user \"%s\"."),
							  role);
		retval = STATUS_ERROR;
	}

	return retval;
}
#endif
/* End - ReqID:SRS-SECURITY-IDENTITYAUTH */

/*
 * Check given password for given user, and return STATUS_OK or STATUS_ERROR.
 *
 * 'shadow_pass' is the user's correct password hash, as stored in
 * pg_authid.rolpassword.
 * 'client_pass' is the password given by the remote user.
 *
 * In the error case, optionally store a palloc'd string at *logdetail
 * that will be sent to the postmaster log (but not the client).
 */
int
plain_crypt_verify(const char *role, const char *shadow_pass,
				   const char *client_pass,
				   char **logdetail)
{
#ifdef USE_GMSSL
	char		crypt_client_pass[SM3_PASSWD_LEN + 1];
#else
	char		crypt_client_pass[MD5_PASSWD_LEN + 1];
#endif
	/*
	 * Client sent password in plaintext.  If we have an MD5 hash stored, hash
	 * the password the client sent, and compare the hashes.  Otherwise
	 * compare the plaintext passwords directly.
	 */
	switch (get_password_type(shadow_pass))
	{
		case PASSWORD_TYPE_SCRAM_SHA_256:
			if (scram_verify_plain_password("SCRAM-SHA-256", role,
											client_pass,
											shadow_pass))
			{
				return STATUS_OK;
			}
			else
			{
				*logdetail = psprintf(_("Password does not match for user \"%s\"."),
									  role);
				return STATUS_ERROR;
			}
			break;
#ifdef USE_GMSSL
		case PASSWORD_TYPE_SCRAM_SM3: // ReqID:SRS-SECURITY-IDENTITYAUTH
			if (scram_verify_plain_password("SCRAM-SM3", role,
											client_pass,
											shadow_pass))
			{
				return STATUS_OK;
			}
			else
			{
				*logdetail = psprintf(_("Password does not match for user \"%s\"."),
									  role);
				return STATUS_ERROR;
			}
			break;
#endif
		case PASSWORD_TYPE_MD5:
			if (!pg_md5_encrypt(client_pass,
								role,
								strlen(role),
								crypt_client_pass))
			{
				/*
				 * We do not bother setting logdetail for pg_md5_encrypt
				 * failure: the only possible error is out-of-memory, which is
				 * unlikely, and if it did happen adding a psprintf call would
				 * only make things worse.
				 */
				return STATUS_ERROR;
			}
			if (strcmp(crypt_client_pass, shadow_pass) == 0)
				return STATUS_OK;
			else
			{
				*logdetail = psprintf(_("Password does not match for user \"%s\"."),
									  role);
				return STATUS_ERROR;
			}
			break;
#ifdef USE_GMSSL
		case PASSWORD_TYPE_SM3: // ReqID:SRS-SECURITY-IDENTITYAUTH
			if (!pg_sm3_encrypt(client_pass,
								role,
								strlen(role),
								crypt_client_pass))
			{
				/*
				 * We do not bother setting logdetail for pg_md5_encrypt
				 * failure: the only possible error is out-of-memory, which is
				 * unlikely, and if it did happen adding a psprintf call would
				 * only make things worse.
				 */
				return STATUS_ERROR;
			}
			if (strcmp(crypt_client_pass, shadow_pass) == 0)
				return STATUS_OK;
			else
			{
				*logdetail = psprintf(_("Password does not match for user \"%s\"."),
									  role);
				return STATUS_ERROR;
			}
			break;
#endif
		case PASSWORD_TYPE_PLAINTEXT:
		/* Begin - Req: SRS-MYSQL-PROTOCOL */
		case PASSWORD_TYPE_MYS_NATIVE:
#ifdef USE_OPENSSL
		case PASSWORD_TYPE_MYS_CACHING_SHA2:
#endif
		/* End - Req: SRS-MYSQL-PROTOCOL */
			/*
			 * We never store passwords in plaintext, so this shouldn't
			 * happen.
			 */
			break;
	}

	/*
	 * This shouldn't happen.  Plain "password" authentication is possible
	 * with any kind of stored password hash.
	 */
	*logdetail = psprintf(_("Password of user \"%s\" is in unrecognized format."),
						  role);
	return STATUS_ERROR;
}
