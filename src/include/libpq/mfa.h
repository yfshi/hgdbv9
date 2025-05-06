/* ---------------------------------------------------------------------
 *
 * File: mfa.h
 *
 * Abstract:
 *		This file contains the definition of the two-factor authentication function.
 *
 * Author: suzy@highgo.com, 20241220.
 *
 * Copyright:
 *		Copyright (c) 2024, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 * 	src/include/libpq/mfa.h
 *
 * Add files that require "SRS-SECURITY-IDENTITYAUTH"
 *
 * ---------------------------------------------------------------------
 */

#ifndef PG_MFA_H
#define PG_MFA_H

#include "datatype/timestamp.h"
#include "libpq/libpq.h"

typedef enum MFAType
{
	MFA_TYPE_TOTP = 1,
	MFA_TYPE_PKEY,
	MFA_TYPE_CERT,
	MFA_TYPE_UNKNOWN
} MFAType;

extern int mfa_totp_verify(const char *role, const char *show_otp,
						   int user_otp,
						   const char *opt_salt, int otp_salt_len,
						   char **logdetail);

extern int mfa_pkey_verify(const char *role, const char *show_otp,
						   char *user_totp,
						   const char *opt_salt, int otp_salt_len,
						   char **logdetail);

int mfa_cert_verify(const char *role, const char *cert, const char *sign,
					char *shadow_otp, const char *opt_salt, int otp_salt_len,
					char **logdetail);

extern int mfa_totp_genkey(char *out);

extern char *mfa_get_key(Port *port, Oid roleid, MFAType *otp_type, char **logdetail);

extern Oid
mfa_get_roleid(const char *rolename);

extern char *mfa_base64_encode(const unsigned char *input, int length);
extern unsigned char *mfa_base64_decode(const char *input, int *length);

#endif