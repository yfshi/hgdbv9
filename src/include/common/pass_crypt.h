/* ---------------------------------------------------------------------
 *
 * File: pass_crypt.h
 *
 * Abstract:
 * 		This file contains the difinition of the encryption and decryption functions of the pgpass file.
 *
 * Author: suzy@highgo.com, 20241220.
 *
 * Copyright:
 * Copyright (c) 2024, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 * 		src/include/common/pass_crypt.h
 *
 * Add files that require "SRS-SECURITY-IDENTITYAUTH"
 *
 * ---------------------------------------------------------------------
 */

#ifndef PG_PASS_CRYPT_H
#define PG_PASS_CRYPT_H

#define PG_DIGEST_LENGTH 32
#define SYMM_IV_SIZE 16
#define PWD_MAX_SIZE 32
#define BUFFER_MAX_SIZE ((SYMM_IV_SIZE + PWD_MAX_SIZE + 16 + PG_DIGEST_LENGTH * 2) * 4 / 3)

bool pass_is_encrypted(const char *passwd);
bool pass_encrypt(const char *passwd, const unsigned char *key, const unsigned char *hmac_key, char *out);
bool pass_decrypt(const char *passwd, const unsigned char *key, const unsigned char *hmac_key, char *out);
bool pass_gen_key(char *DataDir, char *Interface, char *CustomKey, unsigned char *key);
#endif