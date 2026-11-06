/*-------------------------------------------------------------------------
 *
 * cipherdatatype.h
 *	  Definition of the CipherDataType structure for encrypted data types
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/utils/cipherdatatype.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef CIPHERDATATYPE_H
#define CIPHERDATATYPE_H

#include "c.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * CipherDataType structure
 *
 * This structure is used by all encrypted data types (int2_enc, int4_enc, etc.)
 * to store encrypted data in a unified format. The structure contains:
 * - Standard varlena header (vl_len_)
 * - Version information
 * - Lengths of different ciphertext components
 * - The actual encrypted data
 *
 * The data layout in memory:
 * +----------------+----------------+----------------+----------------+----------------+----------------+
 * | vl_len_ (4B)   | version (4B)   | rnd_length (4B)| det_length (4B)| ope_length (4B)| search_len(4B) |
 * +----------------+----------------+----------------+----------------+----------------+----------------+
 * |                                                                                                    |
 * |                                data[] (variable length, contains all ciphertexts)                  |
 * |                                                                                                    |
 * +----------------------------------------------------------------------------------------------------+
 *
 * The data[] field contains ciphertexts in this order:
 * 1. RND ciphertext (length = rnd_length)
 * 2. DET ciphertext (length = det_length)
 * 3. OPE ciphertext (length = ope_length)
 * 4. SEARCH ciphertext (length = search_length)
 *
 * Total length calculation:
 * total_bytes = 20 + rnd_length + det_length + ope_length + search_length
 * (where 20 = 4 (vl_len_) + 4 (version) + 4 (rnd_length) + 4 (det_length) + 4 (ope_length) + 4 (search_length))
 */

typedef struct CipherDataType
{
	/* Standard varlena header - DO NOT TOUCH directly */
	int32		vl_len_;		/* varlena header (do not touch directly!) */

	/* CipherDataType specific header */
	int32		version;		/* version identifier for format compatibility */
	/* Begin - ReqID:TECC4.0 */
	Oid			dek_dist_oid;	/* DEK distributed ID that encrypted this value */
	/* End - ReqID:TECC4.0 */
	int32		rnd_length;		/* length of RND ciphertext in bytes */
	int32		det_length;		/* length of DET ciphertext in bytes */
	int32		ope_length;		/* length of OPE ciphertext in bytes */
	int32		search_length;	/* length of SEARCH ciphertext in bytes */

	/* Variable length data - ciphertexts stored contiguously */
	char		data[FLEXIBLE_ARRAY_MEMBER];
} CipherDataType;

/* Size of the fixed part of CipherDataType (excluding varlena header) */
#define CIPHERDATA_FIXED_SIZE	(sizeof(int32) * 6 + sizeof(Oid))	/* = 28 bytes */

/* Version constants */
#define CIPHERDATA_VERSION_1_0		0x00010000	/* Initial version: support RND/DET/OPE/SEARCH */
#define CIPHERDATA_CURRENT_VERSION	CIPHERDATA_VERSION_1_0

/* Version check macro */
#define CIPHERDATA_VERSION_VALID(version) \
	((version) == CIPHERDATA_VERSION_1_0)

/* Begin - ReqID:TECC4.0 */

/* TECC-specific error codes (51000-51999) */

/* 类型系统错误 (51000-51099) */
#define ERRCODE_TECC_TYPE_NOT_FOUND       MAKE_SQLSTATE('5', '1', '0', '0', '1')
#define ERRCODE_TECC_TYPE_INVALID         MAKE_SQLSTATE('5', '1', '0', '0', '2')
#define ERRCODE_TECC_TYPE_CONFLICT        MAKE_SQLSTATE('5', '1', '0', '0', '3')

/* I/O函数错误 (51100-51199) */
#define ERRCODE_TECC_INVALID_HEX_STRING   MAKE_SQLSTATE('5', '1', '1', '0', '1')
#define ERRCODE_TECC_CIPHERDATA_INVALID   MAKE_SQLSTATE('5', '1', '1', '0', '2')
#define ERRCODE_TECC_VERSION_UNSUPPORTED  MAKE_SQLSTATE('5', '1', '1', '0', '3')

/* 操作符错误 (51200-51299) */
#define ERRCODE_TECC_NO_DET_CIPHERTEXT    MAKE_SQLSTATE('5', '1', '2', '0', '1')
#define ERRCODE_TECC_NO_OPE_CIPHERTEXT    MAKE_SQLSTATE('5', '1', '2', '0', '2')
#define ERRCODE_TECC_COMPARE_UNSUPPORTED  MAKE_SQLSTATE('5', '1', '2', '0', '3')

/* 类型转换错误 (51300-51399) */
#define ERRCODE_TECC_DEK_MISMATCH         MAKE_SQLSTATE('5', '1', '3', '0', '1')
#define ERRCODE_TECC_DEK_PERMISSION_DENIED MAKE_SQLSTATE('5', '1', '3', '0', '2')
#define ERRCODE_TECC_ALGORITHM_INCOMPATIBLE MAKE_SQLSTATE('5', '1', '3', '0', '3')
#define ERRCODE_TECC_COLLATION_MISMATCH   MAKE_SQLSTATE('5', '1', '3', '0', '4')

/* 索引支持错误 (51400-51499) */
#define ERRCODE_TECC_NO_OPE_FOR_BTREE     MAKE_SQLSTATE('5', '1', '4', '0', '1')
#define ERRCODE_TECC_NO_DET_FOR_HASH      MAKE_SQLSTATE('5', '1', '4', '0', '2')
#define ERRCODE_TECC_COLLATION_UNSUPPORTED MAKE_SQLSTATE('5', '1', '4', '0', '3')

/* ICU排序规则错误 (51500-51599) */
#define ERRCODE_TECC_ICU_NOT_AVAILABLE    MAKE_SQLSTATE('5', '1', '5', '0', '1')
#define ERRCODE_TECC_NO_OPE_FOR_COLLATION MAKE_SQLSTATE('5', '1', '5', '0', '4')

/* End - ReqID:TECC4.0 */

/* Macros for accessing CipherDataType fields */
#define CIPHERDATA_VERSION(cipherptr)		(((CipherDataType *)(cipherptr))->version)
#define CIPHERDATA_DEK_DIST_OID(cipherptr)	(((CipherDataType *)(cipherptr))->dek_dist_oid)
#define CIPHERDATA_RND_LENGTH(cipherptr)	(((CipherDataType *)(cipherptr))->rnd_length)
#define CIPHERDATA_DET_LENGTH(cipherptr)	(((CipherDataType *)(cipherptr))->det_length)
#define CIPHERDATA_OPE_LENGTH(cipherptr)	(((CipherDataType *)(cipherptr))->ope_length)
#define CIPHERDATA_SEARCH_LENGTH(cipherptr)	(((CipherDataType *)(cipherptr))->search_length)

/* Macros for accessing ciphertext data */
#define CIPHERDATA_RND_PTR(cipherptr)		\
	((char *)(cipherptr) + sizeof(CipherDataType))

#define CIPHERDATA_DET_PTR(cipherptr)		\
	(CIPHERDATA_RND_PTR(cipherptr) + CIPHERDATA_RND_LENGTH(cipherptr))

#define CIPHERDATA_OPE_PTR(cipherptr)		\
	(CIPHERDATA_DET_PTR(cipherptr) + CIPHERDATA_DET_LENGTH(cipherptr))

#define CIPHERDATA_SEARCH_PTR(cipherptr)	\
	(CIPHERDATA_OPE_PTR(cipherptr) + CIPHERDATA_OPE_LENGTH(cipherptr))

/* Macro to calculate total data size (excluding varlena header) */
#define CIPHERDATA_CONTENT_SIZE(cipherptr)	\
	(CIPHERDATA_FIXED_SIZE + \
	 CIPHERDATA_RND_LENGTH(cipherptr) + \
	 CIPHERDATA_DET_LENGTH(cipherptr) + \
	 CIPHERDATA_OPE_LENGTH(cipherptr) + \
	 CIPHERDATA_SEARCH_LENGTH(cipherptr))

/* Validation macro (Datum-safe, handles 1-byte varlena headers) */
#define CIPHERDATA_IS_VALID(ptr) \
	(PointerIsValid(ptr) && \
	 CIPHERDATA_VERSION_VALID(CIPHERDATA_DATUM_VERSION(ptr)))

/*
 * VARDATA_ANY-safe access macros — for Datum values that may use 1-byte
 * varlena headers.  Use these instead of the CIPHERDATA_* struct-member
 * macros when the pointer is a Datum (PG_GETARG_DATUM / PG_DETOAST_DATUM).
 *
 * Layout after varlena header:
 *   [version:4] [dek_dist_oid:4] [rnd_len:4] [det_len:4] [ope_len:4] [search_len:4] [ct...]
 *    +0          +4              +8         +12        +16         +20             +24
 */
#define CIPHERDATA_DATUM_VERSION(ptr)		(*(int32 *)(VARDATA_ANY(ptr)))
#define CIPHERDATA_DATUM_DEK_DIST_OID(ptr)	(*(Oid *)(VARDATA_ANY(ptr) + 4))
#define CIPHERDATA_DATUM_RND_LEN(ptr)		(*(int32 *)(VARDATA_ANY(ptr) + 8))
#define CIPHERDATA_DATUM_DET_LEN(ptr)		(*(int32 *)(VARDATA_ANY(ptr) + 12))
#define CIPHERDATA_DATUM_OPE_LEN(ptr)		(*(int32 *)(VARDATA_ANY(ptr) + 16))
#define CIPHERDATA_DATUM_SEARCH_LEN(ptr)	(*(int32 *)(VARDATA_ANY(ptr) + 20))
#define	CIPHERDATA_DATUM_RND_PTR(ptr)		(VARDATA_ANY(ptr) + 24)
#define	CIPHERDATA_DATUM_DET_PTR(ptr)		(CIPHERDATA_DATUM_RND_PTR(ptr) + CIPHERDATA_DATUM_RND_LEN(ptr))
#define	CIPHERDATA_DATUM_OPE_PTR(ptr)		(CIPHERDATA_DATUM_DET_PTR(ptr) + CIPHERDATA_DATUM_DET_LEN(ptr))
#define	CIPHERDATA_DATUM_SEARCH_PTR(ptr)	(CIPHERDATA_DATUM_OPE_PTR(ptr) + CIPHERDATA_DATUM_OPE_LEN(ptr))

/*
 * Short-form aliases for cipherdata_operators.c compatibility.
 * These operate on Datum pointers (may have 1-byte varlena headers).
 */
#define CD_DET_LEN(ptr)		CIPHERDATA_DATUM_DET_LEN(ptr)
#define CD_DET_PTR(ptr)		CIPHERDATA_DATUM_DET_PTR(ptr)

/* Function prototypes */
extern CipherDataType *cipherdata_alloc(int32 rnd_len, int32 det_len, int32 ope_len, int32 search_len, Oid dek_dist_oid);
extern void cipherdata_free(CipherDataType *cipherdata);
extern bool cipherdata_validate(const CipherDataType *cipherdata);
extern CipherDataType *cipherdata_copy(const CipherDataType *src);
extern bool cipherdata_equal(const CipherDataType *a, const CipherDataType *b);
extern uint32 cipherdata_hash(const CipherDataType *cipherdata);

#ifdef __cplusplus
}
#endif

#endif	/* CIPHERDATATYPE_H */