/*-------------------------------------------------------------------------
 *
 * hg_key_utils.h
 *	  Utility functions for HG key management
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/utils/hg_key_utils.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef HG_KEY_UTILS_H
#define HG_KEY_UTILS_H

#include "postgres.h"
#include "utils/timestamp.h"
#include "catalog/hg_kek.h"
#include "catalog/hg_dek.h"

/* Function declarations */
extern Oid get_kek_oid(const char *kekname, bool missing_ok);
extern Oid get_kek_oid_ext(const char *kekname, Oid namespace_id, bool missing_ok);
extern Oid get_dek_oid(const char *dekname, bool missing_ok);
extern Oid get_dek_oid_ext(const char *dekname, Oid namespace_id, bool missing_ok);
extern char *get_kek_name(Oid kekid);
extern char *get_dek_name(Oid dekid);
extern bool kek_exists(Oid kekid);
extern bool dek_exists(Oid dekid);

/* Permission checking - use has_kek_privilege() and has_dek_privilege() functions instead */

/* Key validation */
extern bool is_valid_dek(Oid dekid);

/* Begin - ReqID:TECC4.0 */
/* Retrieve DEK argument value from hg_dek_args by (dek_id, key) */
extern char *get_dek_arg_value(Oid dek_id, const char *arg_key);
/* Retrieve DEK distributed identity (hg_dek.column_key_distributed_id) */
extern Oid get_dek_dist_id(Oid dek_id);
/* Find an encrypted column binding this DEK as its homomorphic key
 * (hg_encrypted_column_args he_add_dek/he_mul_dek entry).  All out
 * parameters may be NULL. */
extern bool find_he_dek_bound_column(Oid dek_dist_id, Oid *table_oid,
									 char **column_name, const char **arg_key);
/* End - ReqID:TECC4.0 */

#endif	/* HG_KEY_UTILS_H */