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

#endif	/* HG_KEY_UTILS_H */