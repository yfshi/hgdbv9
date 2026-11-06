/*-------------------------------------------------------------------------
 *
 * tecc_error.h
 *	  Error handling utilities for TECC (全密态数据库) module
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/include/utils/tecc/tecc_error.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_ERROR_H
#define TECC_ERROR_H

#include "postgres.h"
#include "utils/cipherdatatype.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Begin - ReqID:TECC4.0 */

/*
 * tecc_report_error - Unified error reporting function for TECC module
 *
 * This function provides consistent error reporting with detailed context
 * information for debugging and user guidance.
 *
 * Parameters:
 *   sqlstate: TECC-specific SQLSTATE error code
 *   message: Primary error message
 *   detail: Detailed error description (optional)
 *   hint: User guidance for resolving the error (optional)
 *   lineno: Source line number where error occurred
 *   funcname: Function name where error occurred
 */
extern void tecc_report_error(int sqlstate, const char *message,
							  const char *detail, const char *hint,
							  int lineno, const char *funcname);

/*
 * Error reporting macros for convenience
 */

/* General error reporting macro */
#define TECC_REPORT_ERROR(sqlstate, message, detail, hint) \
	tecc_report_error((sqlstate), (message), (detail), (hint), \
					  __LINE__, __func__)

/* Common error reporting macros */

/* I/O function errors */
#define TECC_ERROR_INVALID_HEX_STRING(input_str) \
	TECC_REPORT_ERROR(ERRCODE_TECC_INVALID_HEX_STRING, \
					 "invalid hex string", \
					 psprintf("Input: \"%s\"", (input_str)), \
					 "Hex string must contain only 0-9, a-f, A-F characters and may have \\x prefix.")

#define TECC_ERROR_CIPHERDATA_INVALID \
	TECC_REPORT_ERROR(ERRCODE_TECC_CIPHERDATA_INVALID, \
					 "invalid CipherDataType structure", \
					 NULL, \
					 "Verify the encrypted data format and version.")

#define TECC_ERROR_VERSION_UNSUPPORTED(version) \
	TECC_REPORT_ERROR(ERRCODE_TECC_VERSION_UNSUPPORTED, \
					 "unsupported encrypted data version", \
					 psprintf("Version: 0x%08x", (version)), \
					 "Ensure client and server use compatible TECC versions.")

/* Operator errors */
#define TECC_ERROR_NO_DET_CIPHERTEXT \
	TECC_REPORT_ERROR(ERRCODE_TECC_NO_DET_CIPHERTEXT, \
					 "cannot compare encrypted values without DET ciphertext", \
					 NULL, \
					 "Enable DET encryption for the column to support equality comparisons.")

#define TECC_ERROR_NO_OPE_CIPHERTEXT \
	TECC_REPORT_ERROR(ERRCODE_TECC_NO_OPE_CIPHERTEXT, \
					 "cannot compare encrypted values without OPE ciphertext", \
					 NULL, \
					 "Enable OPE encryption for the column to support range queries.")

#define TECC_ERROR_COMPARE_UNSUPPORTED \
	TECC_REPORT_ERROR(ERRCODE_TECC_COMPARE_UNSUPPORTED, \
					 "comparison operation not supported for encrypted values", \
					 NULL, \
					 "Check if both values have the required ciphertext types for the operation.")

/* Type conversion errors */
#define TECC_ERROR_DEK_MISMATCH(dek1_oid, dek2_oid) \
	TECC_REPORT_ERROR(ERRCODE_TECC_DEK_MISMATCH, \
					 "cannot convert between encrypted columns using different DEKs", \
					 psprintf("Source column uses DEK OID %u, target column uses DEK OID %u", \
							  (dek1_oid), (dek2_oid)), \
					 "Ensure both columns use the same DEK for conversion.")

#define TECC_ERROR_ALGORITHM_INCOMPATIBLE(alg1, alg2) \
	TECC_REPORT_ERROR(ERRCODE_TECC_ALGORITHM_INCOMPATIBLE, \
					 "cannot convert between encrypted columns using incompatible algorithms", \
					 psprintf("Source algorithm: %s, target algorithm: %s", \
							  (alg1), (alg2)), \
					 "Ensure both columns use compatible encryption algorithms.")

#define TECC_ERROR_COLLATION_MISMATCH(coll1_oid, coll2_oid) \
	TECC_REPORT_ERROR(ERRCODE_TECC_COLLATION_MISMATCH, \
					 "cannot convert between encrypted columns using different collations", \
					 psprintf("Source collation OID: %u, target collation OID: %u", \
							  (coll1_oid), (coll2_oid)), \
					 "Ensure both columns use the same collation for string type conversion.")

/* Index support errors */
#define TECC_ERROR_NO_OPE_FOR_BTREE \
	TECC_REPORT_ERROR(ERRCODE_TECC_NO_OPE_FOR_BTREE, \
					 "cannot create B-tree index on encrypted column without OPE ciphertext", \
					 NULL, \
					 "Enable OPE encryption for the column to support B-tree indexes.")

#define TECC_ERROR_NO_DET_FOR_HASH \
	TECC_REPORT_ERROR(ERRCODE_TECC_NO_DET_FOR_HASH, \
					 "cannot create Hash index on encrypted column without DET ciphertext", \
					 NULL, \
					 "Enable DET encryption for the column to support Hash indexes.")

#define TECC_ERROR_COLLATION_UNSUPPORTED(collation_oid) \
	TECC_REPORT_ERROR(ERRCODE_TECC_COLLATION_UNSUPPORTED, \
					 "collation not supported for encrypted column index", \
					 psprintf("Collation OID: %u", (collation_oid)), \
					 "Use default collation or ensure collation is compatible with encrypted types.")

/* ICU collation errors */
#define TECC_ERROR_ICU_NOT_AVAILABLE(collname) \
	TECC_REPORT_ERROR(ERRCODE_TECC_ICU_NOT_AVAILABLE, \
					 "ICU collation not available", \
					 psprintf("Collation: %s", (collname)), \
					 "Rebuild PostgreSQL with --with-icu or use a different collation.")

#define TECC_ERROR_NO_OPE_FOR_COLLATION \
	TECC_REPORT_ERROR(ERRCODE_TECC_NO_OPE_FOR_COLLATION, \
					 "cannot use collation-aware comparison without OPE ciphertext", \
					 NULL, \
					 "Enable OPE encryption for the column to support collation-aware comparisons.")

/* Validation helper macros */
#define TECC_VALIDATE_CIPHERDATA(cipherdata) \
	do { \
		if (!cipherdata_validate(cipherdata)) \
			TECC_ERROR_CIPHERDATA_INVALID; \
	} while (0)

#define TECC_VALIDATE_CIPHERDATA_PTR(cipherdata) \
	do { \
		if (!PointerIsValid(cipherdata)) \
			TECC_REPORT_ERROR(ERRCODE_TECC_CIPHERDATA_INVALID, \
							 "null cipherdata pointer", \
							 NULL, \
							 "Verify the encrypted data is properly initialized."); \
		TECC_VALIDATE_CIPHERDATA(cipherdata); \
	} while (0)

/* End - ReqID:TECC4.0 */

#ifdef __cplusplus
}
#endif

#endif	/* TECC_ERROR_H */