/*-------------------------------------------------------------------------
 *
 * tecc_typeicu.h
 *	  ICU collation support for encrypted data types
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/utils/tecc_typeicu.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_TYPEICU_H
#define TECC_TYPEICU_H

#include "c.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "utils/cipherdatatype.h"

/* Begin - ReqID:TECC4.0 */

/*
 * cipherdata_compare_with_collation
 *		Compare two encrypted values with collation support
 *
 * Design principle:
 * - ICU collation processing is done on the client side
 * - Client converts plaintext strings to ICU sort keys, then encrypts them with OPE
 * - Server only needs to validate collation and compare OPE ciphertexts
 *
 * Returns: -1 if a < b, 0 if equal, 1 if a > b
 */
extern int cipherdata_compare_with_collation(const CipherDataType *a,
											const CipherDataType *b,
											Oid collation_oid);

/* End - ReqID:TECC4.0 */

#ifdef __cplusplus
}
#endif

#endif	/* TECC_TYPEICU_H */