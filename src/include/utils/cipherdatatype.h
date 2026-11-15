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
 * CipherDataType structure (v2.0 compact format)
 *
 * All encrypted data types (int2_enc, int4_enc, ...) store their value in a
 * unified varlena format.  The version field packs two pieces of information:
 *
 *   bits 31..16 : major version (CIPHERDATA_VERSION_*)
 *   bits 15..0  : segment mask (CIPHERDATA_SEG_*)
 *
 * Only segments whose mask bit is set contribute a 4-byte length field and a
 * ciphertext payload.  This removes the ~5 always-zero length fields that a
 * fixed 6-length header wasted for typical column configurations.
 *
 * Layout (offsets measured from the "field area", i.e. the byte right after
 * the varlena header, which is &cd->version):
 *
 *   +0   version        (4B, major|mask)
 *   +4   dek_dist_oid   (4B)
 *   +8   length fields  (4B each, present segments only, in mask bit order)
 *   ...  ciphertext payloads (present segments only, same order)
 *
 * Segment order is fixed: RND, DET, OPE, SEARCH, EG_ADD, EG_MUL.
 *
 * Header sizes (offset from the struct base, i.e. including vl_len_):
 *   v1.0 legacy : 28 bytes (fixed: 4 lengths, no EG)
 *   v2.0 compact: 12 + 4*popcount(mask) bytes
 *
 *   Examples (v2.0):
 *     RND+DET            mask=0x0003 -> 20B total header
 *     RND+OPE            mask=0x0005 -> 20B
 *     RND+DET+SEARCH     mask=0x000B -> 24B
 *     RND+DET+EG_ADD+EG_MUL mask=0x0013 -> 28B
 *     (v1.0 fixed header was 28B, so int4_enc default saves 8B/row)
 */
typedef struct CipherDataType
{
	/* Standard varlena header - DO NOT TOUCH directly */
	int32		vl_len_;		/* varlena header (do not touch directly!) */

	/* CipherDataType specific header */
	int32		version;		/* high 16 bits: major version; low 16: segment mask */
	Oid			dek_dist_oid;	/* DEK distributed ID that encrypted this value */

	/* Variable length data - length fields + ciphertext payloads, contiguous */
	char		data[FLEXIBLE_ARRAY_MEMBER];
} CipherDataType;

/* ------------------------------------------------------------------------
 * Size conventions
 *
 * Two distinct bases are used deliberately; each macro name states which:
 *   FIELDS_BASE : offset from the field area (== &cd->version)
 *   struct base : offset from cd itself (== &cd->vl_len_)
 * ------------------------------------------------------------------------ */

/* Offset from field area to the first length field: version(4) + dek(4) */
#define CIPHERDATA_FIELDS_BASE_SIZE		(sizeof(int32) + sizeof(Oid))	/* 8 */

/* Offset from struct base to the field area */
#define CIPHERDATA_STRUCT_TO_FIELDS		(sizeof(int32))					/* 4 */

/* Minimum on-disk bytes: vl_len_ + version + dek_dist_oid */
#define CIPHERDATA_MIN_SIZE				(CIPHERDATA_STRUCT_TO_FIELDS + CIPHERDATA_FIELDS_BASE_SIZE)	/* 12 */

/* Version constants (major, stored in bits 31..16) */
#define CIPHERDATA_VERSION_1_0		0x00010000	/* Legacy fixed 28B header */
#define CIPHERDATA_VERSION_2_0		0x00020000	/* Compact mask-based header */
#define CIPHERDATA_CURRENT_VERSION	CIPHERDATA_VERSION_2_0

/* ReqID:TECC4.0 - V4.0.1 initially stored V2 values with a fixed 36-byte
 * header and no segment mask.  Compact V2 values always have at least one
 * segment bit, so a zero mask identifies the legacy fixed layout. */
#define CIPHERDATA_IS_LEGACY_FIXED_V2(v) \
	(CIPHERDATA_MAJOR_VERSION(v) == CIPHERDATA_VERSION_2_0 && \
	 CIPHERDATA_SEG_MASK(v) == 0)

/* Float sentinel version for IEEE 754 Infinity/-Infinity RANGE boundaries.
 * These are not encrypted values: the payload holds raw IEEE 754 bytes. */
#define CIPHERDATA_VERSION_FLOAT_SENTINEL  0x7FFF0000

/* Segment mask bit definitions (stored in bits 15..0) */
#define CIPHERDATA_SEG_RND			0x0001
#define CIPHERDATA_SEG_DET			0x0002
#define CIPHERDATA_SEG_OPE			0x0004
#define CIPHERDATA_SEG_SEARCH		0x0008
#define CIPHERDATA_SEG_EG_ADD		0x0010
#define CIPHERDATA_SEG_EG_MUL		0x0020

/* Mask of all defined segment bits (used to reject unknown bits) */
#define CIPHERDATA_SEG_ALL			0x003F

/* Major version extraction (bits 31..16) */
#define CIPHERDATA_MAJOR_VERSION(v)		((int32)((v) & 0xFFFF0000))

/* Segment mask extraction (bits 15..0) */
#define CIPHERDATA_SEG_MASK(v)			((uint16)((v) & 0xFFFF))

/* Combine major version and segment mask into the packed version field */
#define CIPHERDATA_MAKE_VERSION(major, mask) \
	((int32)((major) | (int32)((mask) & 0xFFFF)))

/*
 * Number of set bits in the segment mask.
 * Only 6 segment bits exist, so a direct sum is both correct and cheap.
 * (Do NOT replace with the 0x5555/0x3333 bit-trick: that computes an 8-bit
 *  popcount and yields wrong results for the upper byte.)
 */
#define CIPHERDATA_POPCOUNT16(x) \
	((((x) >> 0) & 1) + (((x) >> 1) & 1) + (((x) >> 2) & 1) + \
	 (((x) >> 3) & 1) + (((x) >> 4) & 1) + (((x) >> 5) & 1))

/*
 * Header size, measured from the STRUCT BASE (includes vl_len_), so that
 * `(char *)cd + CIPHERDATA_HEADER_SIZE(v)` lands on the first payload byte.
 *
 *   v1.0 : 4 + 8 + 4*4 = 28
 *   v2.0 : 4 + 8 + 4*popcount(mask) = 12 + 4*popcount(mask)
 */
#define CIPHERDATA_HEADER_SIZE_V1_0		28
#define CIPHERDATA_HEADER_SIZE_V2_FIXED	36

#define CIPHERDATA_HEADER_SIZE(v) \
	((CIPHERDATA_MAJOR_VERSION(v) == CIPHERDATA_VERSION_1_0) ? \
	 (Size) CIPHERDATA_HEADER_SIZE_V1_0 : \
	 (CIPHERDATA_IS_LEGACY_FIXED_V2(v) ? \
	 (Size) CIPHERDATA_HEADER_SIZE_V2_FIXED : \
	 (Size) (CIPHERDATA_STRUCT_TO_FIELDS + CIPHERDATA_FIELDS_BASE_SIZE + \
			 CIPHERDATA_POPCOUNT16(CIPHERDATA_SEG_MASK(v)) * sizeof(int32))))

/*
 * Version validity — compare the MAJOR version only, because the low 16 bits
 * carry the segment mask and would otherwise fail an equality test.
 */
#define CIPHERDATA_VERSION_VALID(v) \
	(CIPHERDATA_MAJOR_VERSION(v) == CIPHERDATA_VERSION_1_0 || \
	 CIPHERDATA_MAJOR_VERSION(v) == CIPHERDATA_VERSION_2_0 || \
	 CIPHERDATA_MAJOR_VERSION(v) == CIPHERDATA_VERSION_FLOAT_SENTINEL)

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

/*
 * ------------------------------------------------------------------------
 * Field-area accessors
 *
 * These work for both backend (Datum, possibly 1-byte varlena header) and
 * frontend (struct pointer, always 4-byte header written by SET_VARSIZE_4B).
 * ------------------------------------------------------------------------
 */
#ifdef FRONTEND
/*
 * Frontend has no VARDATA_ANY().  All frontend CipherDataType values carry a
 * 4-byte varlena header, so the field area begins sizeof(int32) past the
 * struct base — exactly where the version member sits.
 */
#define CIPHERDATA_FIELDS_BASE(ptr)		((char *)(ptr) + CIPHERDATA_STRUCT_TO_FIELDS)
#define CIPHERDATA_DATUM_RAW_VERSION(ptr)	((int32)((ptr)->version))
#else
/* Backend: VARDATA_ANY handles both 1-byte and 4-byte varlena headers. */
#define CIPHERDATA_FIELDS_BASE(ptr)		VARDATA_ANY(ptr)
#define CIPHERDATA_DATUM_RAW_VERSION(ptr)	(*(int32 *)(VARDATA_ANY(ptr)))
#endif

/* Major version and segment mask of an on-disk value */
#define CIPHERDATA_DATUM_VERSION(ptr)		CIPHERDATA_MAJOR_VERSION(CIPHERDATA_DATUM_RAW_VERSION(ptr))
#define CIPHERDATA_DATUM_SEG_MASK(ptr)		CIPHERDATA_SEG_MASK(CIPHERDATA_DATUM_RAW_VERSION(ptr))
#define CIPHERDATA_DATUM_DEK_DIST_OID(ptr)	(*(Oid *)(CIPHERDATA_FIELDS_BASE(ptr) + sizeof(int32)))

/*
 * Segment presence tests.
 *
 * v1.0 always carries RND/DET/OPE/SEARCH (its header has all four length
 * fields unconditionally) but never carries EG segments.  Legacy fixed V2
 * carries all six length fields.  Compact V2 is driven by the mask.
 */
#define CIPHERDATA_DATUM_HAS_SEG(ptr, seg) \
	((CIPHERDATA_DATUM_VERSION(ptr) == CIPHERDATA_VERSION_1_0) || \
	 CIPHERDATA_IS_LEGACY_FIXED_V2(CIPHERDATA_DATUM_RAW_VERSION(ptr)) || \
	 ((CIPHERDATA_DATUM_SEG_MASK(ptr) & (seg)) != 0))

#define CIPHERDATA_DATUM_HAS_EG_SEG(ptr, seg) \
	((CIPHERDATA_DATUM_VERSION(ptr) != CIPHERDATA_VERSION_1_0) && \
	 (CIPHERDATA_IS_LEGACY_FIXED_V2(CIPHERDATA_DATUM_RAW_VERSION(ptr)) || \
	  ((CIPHERDATA_DATUM_SEG_MASK(ptr) & (seg)) != 0)))

/*
 * Byte offset of each length field, relative to the FIELD AREA.
 *
 * v1.0 keeps its historical fixed layout (rnd@8, det@12, ope@16, search@20)
 * so that legacy ciphertexts stay readable.  v2.0 packs the length fields of
 * present segments only, in mask-bit order.
 */
#define CIPHERDATA_DATUM_RND_OFFSET(ptr) \
	((Size) CIPHERDATA_FIELDS_BASE_SIZE)

#define CIPHERDATA_DATUM_DET_OFFSET(ptr) \
	(CIPHERDATA_DATUM_RND_OFFSET(ptr) + \
	 (CIPHERDATA_DATUM_HAS_SEG(ptr, CIPHERDATA_SEG_RND) ? (Size) sizeof(int32) : (Size) 0))

#define CIPHERDATA_DATUM_OPE_OFFSET(ptr) \
	(CIPHERDATA_DATUM_DET_OFFSET(ptr) + \
	 (CIPHERDATA_DATUM_HAS_SEG(ptr, CIPHERDATA_SEG_DET) ? (Size) sizeof(int32) : (Size) 0))

#define CIPHERDATA_DATUM_SEARCH_OFFSET(ptr) \
	(CIPHERDATA_DATUM_OPE_OFFSET(ptr) + \
	 (CIPHERDATA_DATUM_HAS_SEG(ptr, CIPHERDATA_SEG_OPE) ? (Size) sizeof(int32) : (Size) 0))

#define CIPHERDATA_DATUM_EG_ADD_OFFSET(ptr) \
	(CIPHERDATA_DATUM_SEARCH_OFFSET(ptr) + \
	 (CIPHERDATA_DATUM_HAS_SEG(ptr, CIPHERDATA_SEG_SEARCH) ? (Size) sizeof(int32) : (Size) 0))

#define CIPHERDATA_DATUM_EG_MUL_OFFSET(ptr) \
	(CIPHERDATA_DATUM_EG_ADD_OFFSET(ptr) + \
	 (CIPHERDATA_DATUM_HAS_SEG(ptr, CIPHERDATA_SEG_EG_ADD) ? (Size) sizeof(int32) : (Size) 0))

/*
 * Segment length accessors.
 *
 * IMPORTANT: a segment absent from the mask has NO length field on disk, so
 * reading at its offset would return a neighbouring length.  Absent segments
 * must therefore report 0.  This also makes the pointer chain below correct
 * without extra branching: absent segments contribute zero bytes.
 */
#define CIPHERDATA_DATUM_RND_LEN(ptr) \
	(CIPHERDATA_DATUM_HAS_SEG(ptr, CIPHERDATA_SEG_RND) ? \
	 *(int32 *)(CIPHERDATA_FIELDS_BASE(ptr) + CIPHERDATA_DATUM_RND_OFFSET(ptr)) : 0)

#define CIPHERDATA_DATUM_DET_LEN(ptr) \
	(CIPHERDATA_DATUM_HAS_SEG(ptr, CIPHERDATA_SEG_DET) ? \
	 *(int32 *)(CIPHERDATA_FIELDS_BASE(ptr) + CIPHERDATA_DATUM_DET_OFFSET(ptr)) : 0)

#define CIPHERDATA_DATUM_OPE_LEN(ptr) \
	(CIPHERDATA_DATUM_HAS_SEG(ptr, CIPHERDATA_SEG_OPE) ? \
	 *(int32 *)(CIPHERDATA_FIELDS_BASE(ptr) + CIPHERDATA_DATUM_OPE_OFFSET(ptr)) : 0)

#define CIPHERDATA_DATUM_SEARCH_LEN(ptr) \
	(CIPHERDATA_DATUM_HAS_SEG(ptr, CIPHERDATA_SEG_SEARCH) ? \
	 *(int32 *)(CIPHERDATA_FIELDS_BASE(ptr) + CIPHERDATA_DATUM_SEARCH_OFFSET(ptr)) : 0)

/* v1.0 has no EG segments at all -> use the EG-specific presence test */
#define CIPHERDATA_DATUM_EG_ADD_LEN(ptr) \
	(CIPHERDATA_DATUM_HAS_EG_SEG(ptr, CIPHERDATA_SEG_EG_ADD) ? \
	 *(int32 *)(CIPHERDATA_FIELDS_BASE(ptr) + CIPHERDATA_DATUM_EG_ADD_OFFSET(ptr)) : 0)

#define CIPHERDATA_DATUM_EG_MUL_LEN(ptr) \
	(CIPHERDATA_DATUM_HAS_EG_SEG(ptr, CIPHERDATA_SEG_EG_MUL) ? \
	 *(int32 *)(CIPHERDATA_FIELDS_BASE(ptr) + CIPHERDATA_DATUM_EG_MUL_OFFSET(ptr)) : 0)

/*
 * Payload pointers, relative to the struct base.  Each chains from the end of
 * the previous segment; absent segments have length 0 and therefore shift
 * nothing, so the chain stays correct for any mask.
 */
#define CIPHERDATA_DATUM_DATA_BASE(ptr) \
	((char *)(ptr) + CIPHERDATA_HEADER_SIZE(CIPHERDATA_DATUM_RAW_VERSION(ptr)))
#define CIPHERDATA_DATUM_END_OF_SEARCH(ptr) \
	(CIPHERDATA_DATUM_SEARCH_PTR(ptr) + CIPHERDATA_DATUM_SEARCH_LEN(ptr))

#define CIPHERDATA_DATUM_RND_PTR(ptr)		CIPHERDATA_DATUM_DATA_BASE(ptr)
#define CIPHERDATA_DATUM_DET_PTR(ptr) \
	(CIPHERDATA_DATUM_RND_PTR(ptr) + CIPHERDATA_DATUM_RND_LEN(ptr))
#define CIPHERDATA_DATUM_OPE_PTR(ptr) \
	(CIPHERDATA_DATUM_DET_PTR(ptr) + CIPHERDATA_DATUM_DET_LEN(ptr))
#define CIPHERDATA_DATUM_SEARCH_PTR(ptr) \
	(CIPHERDATA_DATUM_OPE_PTR(ptr) + CIPHERDATA_DATUM_OPE_LEN(ptr))

/* Begin - ReqID:TECC4.0 */
/* EG segments carry a 4-byte DEK distributed ID prefix before the ciphertext.
 * Pointer/length macros below operate on the whole segment (prefix included). */
#define CIPHERDATA_HE_DEK_ID_SIZE		4

/* NULL when the segment is absent — callers must not dereference then. */
#define CIPHERDATA_DATUM_EG_ADD_PTR(ptr) \
	(CIPHERDATA_DATUM_HAS_EG_SEG(ptr, CIPHERDATA_SEG_EG_ADD) ? \
	 CIPHERDATA_DATUM_END_OF_SEARCH(ptr) : NULL)
#define CIPHERDATA_DATUM_EG_MUL_PTR(ptr) \
	(CIPHERDATA_DATUM_HAS_EG_SEG(ptr, CIPHERDATA_SEG_EG_MUL) ? \
	 (CIPHERDATA_DATUM_END_OF_SEARCH(ptr) + CIPHERDATA_DATUM_EG_ADD_LEN(ptr)) : NULL)

/* Per-segment DEK distributed ID (first 4 bytes of the segment) */
#define CIPHERDATA_DATUM_EG_ADD_DEK_ID(ptr)	\
	((CIPHERDATA_DATUM_EG_ADD_LEN(ptr) >= CIPHERDATA_HE_DEK_ID_SIZE) ? \
	 (*(Oid *)CIPHERDATA_DATUM_EG_ADD_PTR(ptr)) : InvalidOid)
#define CIPHERDATA_DATUM_EG_MUL_DEK_ID(ptr)	\
	((CIPHERDATA_DATUM_EG_MUL_LEN(ptr) >= CIPHERDATA_HE_DEK_ID_SIZE) ? \
	 (*(Oid *)CIPHERDATA_DATUM_EG_MUL_PTR(ptr)) : InvalidOid)

/* Actual HE ciphertext (past the 4-byte DEK ID prefix); NULL when absent */
#define CIPHERDATA_DATUM_EG_ADD_CT_PTR(ptr)	\
	((CIPHERDATA_DATUM_EG_ADD_LEN(ptr) > 0) ? \
	 (const uint8_t *)(CIPHERDATA_DATUM_EG_ADD_PTR(ptr) + CIPHERDATA_HE_DEK_ID_SIZE) : NULL)
#define CIPHERDATA_DATUM_EG_MUL_CT_PTR(ptr)	\
	((CIPHERDATA_DATUM_EG_MUL_LEN(ptr) > 0) ? \
	 (const uint8_t *)(CIPHERDATA_DATUM_EG_MUL_PTR(ptr) + CIPHERDATA_HE_DEK_ID_SIZE) : NULL)

/* HE ciphertext length (excludes the 4-byte DEK ID prefix) */
#define CIPHERDATA_DATUM_EG_ADD_CT_LEN(ptr)	\
	((CIPHERDATA_DATUM_EG_ADD_LEN(ptr) > 0) ? \
	 (CIPHERDATA_DATUM_EG_ADD_LEN(ptr) - CIPHERDATA_HE_DEK_ID_SIZE) : 0)
#define CIPHERDATA_DATUM_EG_MUL_CT_LEN(ptr)	\
	((CIPHERDATA_DATUM_EG_MUL_LEN(ptr) > 0) ? \
	 (CIPHERDATA_DATUM_EG_MUL_LEN(ptr) - CIPHERDATA_HE_DEK_ID_SIZE) : 0)
/* End - ReqID:TECC4.0 */

/* Check if a CipherDataType (Datum pointer) is a float sentinel */
#define CIPHERDATA_IS_FLOAT_SENTINEL(ptr) \
	(CIPHERDATA_DATUM_VERSION(ptr) == CIPHERDATA_VERSION_FLOAT_SENTINEL)

/* Check if a float sentinel is negative (-Infinity).
 * Uses float comparison (endian-agnostic at C level) rather than
 * raw byte inspection to work correctly on both LE and BE systems. */
#define CIPHERDATA_SENTINEL_IS_NEGATIVE(ptr) \
	((CIPHERDATA_DATUM_OPE_LEN(ptr) == sizeof(float)) \
	 ? ((*(const float *)CIPHERDATA_DATUM_OPE_PTR(ptr)) < 0.0f) \
	 : ((*(const double *)CIPHERDATA_DATUM_OPE_PTR(ptr)) < 0.0))

/* Validation macro (Datum-safe, handles 1-byte varlena headers) */
#define CIPHERDATA_IS_VALID(ptr) \
	(PointerIsValid(ptr) && \
	 CIPHERDATA_VERSION_VALID(CIPHERDATA_DATUM_RAW_VERSION(ptr)))

/*
 * Struct-member accessors.
 *
 * CIPHERDATA_VERSION()/CIPHERDATA_DEK_DIST_OID() read the fixed struct fields.
 * CIPHERDATA_VERSION() returns the MAJOR version so that existing equality
 * tests against CIPHERDATA_VERSION_* keep working; use
 * CIPHERDATA_DATUM_SEG_MASK() to inspect the mask.
 *
 * The remaining accessors delegate to the mask-aware field-area macros, which
 * is valid for freshly allocated values (4-byte header, non-const pointer).
 */
#define CIPHERDATA_VERSION(cipherptr) \
	CIPHERDATA_MAJOR_VERSION(((CipherDataType *)(cipherptr))->version)
#define CIPHERDATA_RAW_VERSION(cipherptr)	(((CipherDataType *)(cipherptr))->version)
#define CIPHERDATA_DEK_DIST_OID(cipherptr)	(((CipherDataType *)(cipherptr))->dek_dist_oid)

#define CIPHERDATA_RND_LENGTH(cipherptr)	CIPHERDATA_DATUM_RND_LEN(cipherptr)
#define CIPHERDATA_DET_LENGTH(cipherptr)	CIPHERDATA_DATUM_DET_LEN(cipherptr)
#define CIPHERDATA_OPE_LENGTH(cipherptr)	CIPHERDATA_DATUM_OPE_LEN(cipherptr)
#define CIPHERDATA_SEARCH_LENGTH(cipherptr)	CIPHERDATA_DATUM_SEARCH_LEN(cipherptr)
#define CIPHERDATA_EG_ADD_LENGTH(cipherptr)	CIPHERDATA_DATUM_EG_ADD_LEN(cipherptr)
#define CIPHERDATA_EG_MUL_LENGTH(cipherptr)	CIPHERDATA_DATUM_EG_MUL_LEN(cipherptr)

#define CIPHERDATA_RND_PTR(cipherptr)		CIPHERDATA_DATUM_RND_PTR(cipherptr)
#define CIPHERDATA_DET_PTR(cipherptr)		CIPHERDATA_DATUM_DET_PTR(cipherptr)
#define CIPHERDATA_OPE_PTR(cipherptr)		CIPHERDATA_DATUM_OPE_PTR(cipherptr)
#define CIPHERDATA_SEARCH_PTR(cipherptr)	CIPHERDATA_DATUM_SEARCH_PTR(cipherptr)
#define CIPHERDATA_EG_ADD_PTR(cipherptr)	CIPHERDATA_DATUM_EG_ADD_PTR(cipherptr)
#define CIPHERDATA_EG_MUL_PTR(cipherptr)	CIPHERDATA_DATUM_EG_MUL_PTR(cipherptr)

#define CIPHERDATA_EG_ADD_DEK_ID_PTR(cipherptr)	\
	((Oid *)CIPHERDATA_EG_ADD_PTR(cipherptr))
#define CIPHERDATA_EG_MUL_DEK_ID_PTR(cipherptr)	\
	((Oid *)CIPHERDATA_EG_MUL_PTR(cipherptr))

#define CIPHERDATA_EG_ADD_CT_PTR(cipherptr)	\
	((CIPHERDATA_EG_ADD_LENGTH(cipherptr) > 0) ? \
	 (CIPHERDATA_EG_ADD_PTR(cipherptr) + CIPHERDATA_HE_DEK_ID_SIZE) : NULL)
#define CIPHERDATA_EG_MUL_CT_PTR(cipherptr)	\
	((CIPHERDATA_EG_MUL_LENGTH(cipherptr) > 0) ? \
	 (CIPHERDATA_EG_MUL_PTR(cipherptr) + CIPHERDATA_HE_DEK_ID_SIZE) : NULL)

#define CIPHERDATA_EG_ADD_CT_LENGTH(cipherptr)	\
	((CIPHERDATA_EG_ADD_LENGTH(cipherptr) > 0) ? \
	 (CIPHERDATA_EG_ADD_LENGTH(cipherptr) - CIPHERDATA_HE_DEK_ID_SIZE) : 0)
#define CIPHERDATA_EG_MUL_CT_LENGTH(cipherptr)	\
	((CIPHERDATA_EG_MUL_LENGTH(cipherptr) > 0) ? \
	 (CIPHERDATA_EG_MUL_LENGTH(cipherptr) - CIPHERDATA_HE_DEK_ID_SIZE) : 0)

/* Total on-disk size from the struct base, given a mask and segment lengths */
#define CIPHERDATA_TOTAL_SIZE(mask, rnd_len, det_len, ope_len, search_len, eg_add_len, eg_mul_len) \
	(CIPHERDATA_STRUCT_TO_FIELDS + CIPHERDATA_FIELDS_BASE_SIZE + \
	 CIPHERDATA_POPCOUNT16(mask) * sizeof(int32) + \
	 (rnd_len) + (det_len) + (ope_len) + (search_len) + (eg_add_len) + (eg_mul_len))

/*
 * Short-form aliases used by cipherdata_operators.c.
 */
#define CD_DET_LEN(ptr)		CIPHERDATA_DATUM_DET_LEN(ptr)
#define CD_DET_PTR(ptr)		CIPHERDATA_DATUM_DET_PTR(ptr)

/* Function prototypes */
extern CipherDataType *cipherdata_alloc(int32 rnd_len, int32 det_len, int32 ope_len, int32 search_len, int32 eg_add_len, int32 eg_mul_len, Oid dek_dist_oid);
/* ReqID:TECC4.0 - Allocate a value that keeps `version`, reserving exactly the
 * length fields that version implies (v1.0 always carries its four base
 * lengths, even for segments that stay empty). */
extern CipherDataType *cipherdata_alloc_for_version(int32 rnd_len, int32 det_len, int32 ope_len, int32 search_len, int32 eg_add_len, int32 eg_mul_len, Oid dek_dist_oid, int32 version);
/* ReqID:TECC4.0 - True when the mask packed in `version` selects the same
 * length fields that the parsed lengths produce; false means materialising the
 * value would copy payload past the end of the structure. */
extern bool cipherdata_parsed_layout_ok(int32 version, int32 rnd_len, int32 det_len, int32 ope_len, int32 search_len, int32 eg_add_len, int32 eg_mul_len);
extern void cipherdata_free(CipherDataType *cipherdata);
extern bool cipherdata_validate(const CipherDataType *cipherdata);
extern CipherDataType *cipherdata_copy(const CipherDataType *src);
extern bool cipherdata_equal(const CipherDataType *a, const CipherDataType *b);
extern uint32 cipherdata_hash(const CipherDataType *cipherdata);

#ifdef __cplusplus
}
#endif

#endif	/* CIPHERDATATYPE_H */
