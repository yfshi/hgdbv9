/*-------------------------------------------------------------------------
 *
 * cipherdatatype.c
 *	  Functions for encrypted data types using CipherDataType structure
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/backend/utils/tecc/cipherdatatype.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include <ctype.h>
#include <string.h>

#include "catalog/pg_type.h"
#include "catalog/pg_type_enc.h"
#include "libpq/pqformat.h"
#include "utils/builtins.h"
#include "utils/cipherdatatype.h"
#include "utils/tecc/cipherdata_hex.h"

/*
 * Ensure CipherDataType uses 4-byte varlena header
 *
 * PG may store short varlena data using 1-byte header format.
 * This function converts it to 4-byte header format so that
 * CipherDataType structure field offsets are correct.
 * Returns a new palloc'd copy if conversion was needed.
 */
static CipherDataType *
cipherdata_convert_to_4b_header(const CipherDataType *input)
{
	Size		total_size;
	CipherDataType *result;
	const char *raw = (const char *) input;

	if (VARATT_IS_1B(input))
	{
		/* 1-byte header: 1 byte header + data starts at offset 1 */
		Size		data_size = VARSIZE_1B(input) - 1;	/* minus 1-byte header */
		Size		hdr_size_4b = sizeof(int32);

		/* Allocate with 4-byte header */
		total_size = hdr_size_4b + data_size;
		result = (CipherDataType *) palloc(total_size);
		SET_VARSIZE_4B_C(result, total_size);
		memcpy((char *) result + hdr_size_4b, raw + 1, data_size);
		return result;
	}
	else
	{
		/* Already 4-byte header (or compressed 4-byte, but not expected) */
		return (CipherDataType *) input;
	}
}

/*
 * cipherdata_seg_mask_from_lengths - Segment mask implied by the lengths
 *
 * ReqID:TECC4.0 - Only the compact layout reserves one length field per
 * non-empty segment, so this is the mask a freshly built compact value uses.
 */
static uint16
cipherdata_seg_mask_from_lengths(int32 rnd_len, int32 det_len, int32 ope_len,
								 int32 search_len, int32 eg_add_len, int32 eg_mul_len)
{
	uint16		mask = 0;

	if (rnd_len > 0)		mask |= CIPHERDATA_SEG_RND;
	if (det_len > 0)		mask |= CIPHERDATA_SEG_DET;
	if (ope_len > 0)		mask |= CIPHERDATA_SEG_OPE;
	if (search_len > 0)		mask |= CIPHERDATA_SEG_SEARCH;
	if (eg_add_len > 0)		mask |= CIPHERDATA_SEG_EG_ADD;
	if (eg_mul_len > 0)		mask |= CIPHERDATA_SEG_EG_MUL;

	return mask;
}

/*
 * cipherdata_alloc_layout - Allocate a CipherDataType with an explicit layout
 *
 * Allocates memory for a CipherDataType structure holding the given segment
 * lengths, reserving one length field per segment set in `layout_mask` (the
 * version stored in the structure carries the same mask).  The structure
 * includes the varlena header.
 *
 * ReqID:TECC4.0 - The caller must pass the mask implied by the version it is
 * going to keep, because every CIPHERDATA_*_PTR macro locates the payload from
 * that mask.  Reserving fields only for the non-empty segments makes the
 * payload copies of a value whose version declares more segments run past the
 * end of the structure.
 */
static CipherDataType *
cipherdata_alloc_layout(int32 rnd_len, int32 det_len, int32 ope_len,
						int32 search_len, int32 eg_add_len, int32 eg_mul_len,
						Oid dek_dist_oid, uint16 layout_mask)
{
	Size		total_size;
	CipherDataType *result;
	int32		version;
	char	   *hdr_ptr;
	int64		data_total;

	version = (int32)CIPHERDATA_CURRENT_VERSION | (int32)layout_mask;

	/* ReqID:TECC4.0 - Sum in int64: an int32 sum can wrap onto a small value
	 * and hand a huge segment length to the memcpy that fills the structure. */
	data_total = (int64) rnd_len + (int64) det_len + (int64) ope_len +
				 (int64) search_len + (int64) eg_add_len + (int64) eg_mul_len;

	/* Calculate total size including varlena header.  A negative sum (caller
	 * bug) turns into a huge Size and is refused by palloc. */
	total_size = (Size)CIPHERDATA_FIELDS_BASE_SIZE +
				 CIPHERDATA_POPCOUNT16(layout_mask) * sizeof(int32) +
				 sizeof(int32) +	/* vl_len_ */
				 (Size) data_total;

	/* Allocate memory */
	result = (CipherDataType *) palloc(total_size);

	/* Set varlena header - always use 4-byte */
	SET_VARSIZE_4B(result, total_size);

	/* Write base header */
	result->version = version;
	result->dek_dist_oid = dek_dist_oid;

	/* Write length fields at mask-computed offsets */
	hdr_ptr = (char *) VARDATA_ANY(result) + CIPHERDATA_FIELDS_BASE_SIZE;
	if (layout_mask & CIPHERDATA_SEG_RND)	{ *(int32 *)hdr_ptr = rnd_len;	hdr_ptr += sizeof(int32); }
	if (layout_mask & CIPHERDATA_SEG_DET)	{ *(int32 *)hdr_ptr = det_len;	hdr_ptr += sizeof(int32); }
	if (layout_mask & CIPHERDATA_SEG_OPE)	{ *(int32 *)hdr_ptr = ope_len;	hdr_ptr += sizeof(int32); }
	if (layout_mask & CIPHERDATA_SEG_SEARCH) { *(int32 *)hdr_ptr = search_len; hdr_ptr += sizeof(int32); }
	if (layout_mask & CIPHERDATA_SEG_EG_ADD) { *(int32 *)hdr_ptr = eg_add_len; hdr_ptr += sizeof(int32); }
	if (layout_mask & CIPHERDATA_SEG_EG_MUL) { *(int32 *)hdr_ptr = eg_mul_len; hdr_ptr += sizeof(int32); }

	/* Zero out the data area */
	if (data_total > 0)
		memset(hdr_ptr, 0, (Size) data_total);

	return result;
}

/*
 * cipherdata_alloc - Allocate a CipherDataType structure (v2.0 compact format)
 *
 * Allocates memory for a CipherDataType structure with the specified
 * ciphertext lengths.  Only length fields for non-zero segments are stored
 * (mask-based compact header).  The structure includes the varlena header.
 */
CipherDataType *
cipherdata_alloc(int32 rnd_len, int32 det_len, int32 ope_len, int32 search_len, int32 eg_add_len, int32 eg_mul_len, Oid dek_dist_oid)
{
	return cipherdata_alloc_layout(rnd_len, det_len, ope_len, search_len,
								   eg_add_len, eg_mul_len, dek_dist_oid,
								   cipherdata_seg_mask_from_lengths(rnd_len,
																	det_len,
																	ope_len,
																	search_len,
																	eg_add_len,
																	eg_mul_len));
}

/*
 * cipherdata_alloc_for_version - Allocate for a value that keeps `version`
 *
 * ReqID:TECC4.0 - Reserves exactly the length fields the kept version implies:
 * v1.0 keeps its fixed 28-byte header (RND/DET/OPE/SEARCH length fields are
 * always present, even when a segment is empty), legacy fixed V2 is re-packed
 * as compact by the callers, and a compact value carries the mask it declares.
 */
CipherDataType *
cipherdata_alloc_for_version(int32 rnd_len, int32 det_len, int32 ope_len,
							 int32 search_len, int32 eg_add_len, int32 eg_mul_len,
							 Oid dek_dist_oid, int32 version)
{
	uint16		layout_mask;

	if (CIPHERDATA_MAJOR_VERSION(version) == CIPHERDATA_VERSION_1_0)
		layout_mask = CIPHERDATA_SEG_RND | CIPHERDATA_SEG_DET |
					  CIPHERDATA_SEG_OPE | CIPHERDATA_SEG_SEARCH;
	else if (CIPHERDATA_IS_LEGACY_FIXED_V2(version))
		layout_mask = cipherdata_seg_mask_from_lengths(rnd_len, det_len, ope_len,
													   search_len, eg_add_len,
													   eg_mul_len);
	else
		layout_mask = CIPHERDATA_SEG_MASK(version);

	return cipherdata_alloc_layout(rnd_len, det_len, ope_len, search_len,
								   eg_add_len, eg_mul_len, dek_dist_oid,
								   layout_mask);
}

/*
 * cipherdata_parsed_layout_ok - Segment mask agrees with the parsed lengths
 *
 * ReqID:TECC4.0 - A parsed segment header is only safe to materialise when the
 * mask packed in its version selects the same length fields that the parsed
 * lengths produce.  The payload offsets come from the mask while the payload
 * copies consume the lengths, so a disagreement shifts one against the other
 * and the copies run past the end of the structure.  cipherdata_validate()
 * rejects such a value as well, but only after the copies have happened.
 *
 * v1.0 and legacy fixed V2 always carry a fixed set of length fields, so any
 * (non-negative) lengths are consistent with their layout.
 */
bool
cipherdata_parsed_layout_ok(int32 version, int32 rnd_len, int32 det_len,
							int32 ope_len, int32 search_len,
							int32 eg_add_len, int32 eg_mul_len)
{
	if (CIPHERDATA_MAJOR_VERSION(version) == CIPHERDATA_VERSION_1_0 ||
		CIPHERDATA_IS_LEGACY_FIXED_V2(version))
		return true;

	return CIPHERDATA_SEG_MASK(version) ==
		cipherdata_seg_mask_from_lengths(rnd_len, det_len, ope_len,
										 search_len, eg_add_len, eg_mul_len);
}

/*
 * cipherdata_free - Free a CipherDataType structure
 */
void
cipherdata_free(CipherDataType *cipherdata)
{
	pfree(cipherdata);
}

/*
 * cipherdata_validate - Validate a CipherDataType structure
 *
 * Returns true if the structure is valid, false otherwise.
 */
bool
cipherdata_validate(const CipherDataType *cipherdata)
{
	Size		expected_size;
	Size		vlen;
	int32		raw_version;
	int32		version;
	int32		rnd_len;
	int32		det_len;
	int32		ope_len;
	int32		search_len;
	/* ReqID:TECC4.0 */
	int32		eg_add_len;
	int32		eg_mul_len;

	if (cipherdata == NULL)
		return false;

	/* Get varlena size, handling both 1-byte and 4-byte headers */
	if (VARATT_IS_1B(cipherdata))
		vlen = VARSIZE_1B(cipherdata);
	else
		vlen = VARSIZE_4B(cipherdata);

	/* Minimum: vl_len_ + version + dek_dist_oid */
	if (vlen < CIPHERDATA_MIN_SIZE)
		return false;

	/* Use unified field-area macros from cipherdatatype.h */
	raw_version = CIPHERDATA_DATUM_RAW_VERSION(cipherdata);
	version    = CIPHERDATA_MAJOR_VERSION(raw_version);

	/* Reject unknown segment bits so a corrupt mask cannot be trusted */
	if ((CIPHERDATA_SEG_MASK(raw_version) & ~CIPHERDATA_SEG_ALL) != 0)
		return false;

	/* Validate the complete version-specific header before reading any
	 * mask-dependent length field. */
	if (!CIPHERDATA_VERSION_VALID(raw_version) ||
		vlen < CIPHERDATA_HEADER_SIZE(raw_version))
		return false;

	rnd_len    = CIPHERDATA_DATUM_RND_LEN(cipherdata);
	det_len    = CIPHERDATA_DATUM_DET_LEN(cipherdata);
	ope_len    = CIPHERDATA_DATUM_OPE_LEN(cipherdata);
	search_len = CIPHERDATA_DATUM_SEARCH_LEN(cipherdata);
	/* ReqID:TECC4.0 - Read EG lengths (mask-aware, returns 0 for v1.0) */
	eg_add_len = CIPHERDATA_DATUM_EG_ADD_LEN(cipherdata);
	eg_mul_len = CIPHERDATA_DATUM_EG_MUL_LEN(cipherdata);

	/* Check version (major only — low 16 bits hold the segment mask) */
	/* ReqID:TECC4.0 - Validate DEK distributed ID.
	 * Float sentinels (Infinity/-Infinity partition boundaries) use InvalidOid
	 * since they are not encrypted by any DEK. */
	if (version != CIPHERDATA_VERSION_FLOAT_SENTINEL &&
		!OidIsValid(CIPHERDATA_DATUM_DEK_DIST_OID(cipherdata)))
		return false;

	/* ReqID:TECC4.0 - Reject negative lengths (prevent unsigned overflow in size calc) */
	if (rnd_len < 0 || det_len < 0 || ope_len < 0 || search_len < 0 ||
		eg_add_len < 0 || eg_mul_len < 0)
		return false;

	/* Expected size: varlena header + version-aware cipherdata header + data.
	 * The header size already accounts for the mask-dependent number of
	 * length fields; subtract the 4-byte vl_len_ it assumes and add the real
	 * one, which may be 1 byte for a short-header Datum. */
	{
		Size		hdr_sz = VARATT_IS_1B(cipherdata) ? 1 : sizeof(int32);
		Size		cd_hdr = CIPHERDATA_HEADER_SIZE(raw_version);

		expected_size = hdr_sz + (cd_hdr - sizeof(int32)) +
				rnd_len + det_len + ope_len + search_len +
				eg_add_len + eg_mul_len;
	}

	/* Check total size */
	if (vlen != expected_size)
		return false;

	return true;
}

/*
 * cipherdata_copy - Copy a CipherDataType structure
 *
 * Returns a newly allocated copy of the source structure.
 * Returns NULL if the source is invalid.
 */
CipherDataType *
cipherdata_copy(const CipherDataType *src)
{
	CipherDataType *dst;
	Size total_size;

	if (src == NULL || !cipherdata_validate(src))
		return NULL;

	/* Calculate total size */
	total_size = VARSIZE(src);

	/* Allocate memory */
	dst = (CipherDataType *) palloc(total_size);

	/* Copy entire structure */
	memcpy(dst, src, total_size);

	return dst;
}

/*
 * cipherdata_equal - Compare two CipherDataType structures for equality
 *
 * Returns true if the structures are identical, false otherwise.
 * Comparison includes all header fields and ciphertext data.
 */
bool
cipherdata_equal(const CipherDataType *a, const CipherDataType *b)
{
	Size size_a, size_b;

	if (a == b)
		return true;

	if (a == NULL || b == NULL)
		return false;

	/* Quick size check */
	size_a = VARSIZE(a);
	size_b = VARSIZE(b);
	if (size_a != size_b)
		return false;

	/* Compare entire memory */
	return (memcmp(a, b, size_a) == 0);
}

/*
 * cipherdata_hash - Compute hash value for a CipherDataType structure
 *
 * Returns a 32-bit hash value suitable for hash table operations.
 */
uint32
cipherdata_hash(const CipherDataType *cipherdata)
{
	uint32 hash = 0;
	const unsigned char *data;
	Size size;
	int i;

	if (cipherdata == NULL)
		return 0;

	/* Use VARDATA_ANY / VARSIZE_ANY_EXHDR for varlena-header independence */
	data = (const unsigned char *) VARDATA_ANY(cipherdata);
	size = VARSIZE_ANY_EXHDR(cipherdata);

	for (i = 0; i < size; i++)
	{
		hash ^= data[i];
		hash *= 16777619;
	}

	return hash;
}

/*
 * Generic input function for encrypted types
 *
 * Input format: Base64 encoded binary data containing:
 * - Version (4 bytes)
 * - RND ciphertext length (4 bytes)
 * - DET ciphertext length (4 bytes)
 * - OPE ciphertext length (4 bytes)
 * - SEARCH ciphertext length (4 bytes)
 * - RND ciphertext data
 * - DET ciphertext data
 * - OPE ciphertext data
 * - SEARCH ciphertext data
 */
static Datum
cipherdata_in(PG_FUNCTION_ARGS, Oid type_oid)
{
	char	   *input_str = PG_GETARG_CSTRING(0);
	size_t		decoded_len;
	unsigned char *decoded_data;
	CipherDataType *result;
	Oid			dek_dist_oid;
	int32		rnd_len;
	int32		det_len;
	int32		ope_len;
	int32		search_len;
	/* ReqID:TECC4.0 */
	int32		version;
	int32		eg_add_len;
	int32		eg_mul_len;
	/* End - ReqID:TECC4.0 */
	unsigned char *ptr;

	/*
	 * ReqID:TECC4.0 - Handle IEEE 754 Infinity/-Infinity sentinel values
	 * for float4_enc and float8_enc types.
	 *
	 * These are used as RANGE partition boundary sentinels and are not
	 * hex-encoded.  The client-side tecc_encrypt_boundary_value() skips
	 * encryption for these values (patch 0014), sending them as plain
	 * text.  We construct a CipherDataType that wraps the raw IEEE 754
	 * float bytes so the value compares correctly against real encrypted
	 * data: -Infinity < any encrypted value < +Infinity.
	 */
	if (type_oid == FLOAT4_ENCOID || type_oid == FLOAT8_ENCOID)
	{
		const char *val = input_str;
		bool	is_negative;
		bool	is_sentinel = false;

		/* Strip single quotes if present (parser may leave them).
		 * We use a small stack buffer — Infinity strings are short. */
		if (val[0] == '\'')
		{
			size_t vlen = strlen(val);
			if (vlen > 2 && val[vlen - 1] == '\'')
			{
				char *stripped = (char *) palloc(vlen - 1);
				memcpy(stripped, val + 1, vlen - 2);
				stripped[vlen - 2] = '\0';
				val = stripped;
			}
		}

		if (strcmp(val, "Infinity") == 0 || strcmp(val, "+Infinity") == 0)
		{
			is_negative = false;
			is_sentinel = true;
		}
		else if (strcmp(val, "-Infinity") == 0)
		{
			is_negative = true;
			is_sentinel = true;
		}

		if (is_sentinel)
		{
			int32		ope_len;
			union { float f; uint32 i; } f4;
			union { double d; uint64 i; } f8;

			ope_len = (type_oid == FLOAT4_ENCOID) ? sizeof(float) : sizeof(double);

			result = cipherdata_alloc(0, 0, ope_len, 0, 0, 0, InvalidOid);
			/* ReqID:TECC4.0 - Keep the segment mask (OPE) while switching to
			 * the sentinel major version; otherwise the raw IEEE bytes stored
			 * in the OPE slot would be unreachable. */
			result->version = CIPHERDATA_MAKE_VERSION(CIPHERDATA_VERSION_FLOAT_SENTINEL,
													  CIPHERDATA_SEG_OPE);

			/* Store IEEE 754 Infinity bytes in the OPE data area.
			 * float4 -Infinity = 0xFF800000, +Infinity = 0x7F800000
			 * float8 -Infinity = 0xFFF0000000000000, +Infinity = 0x7FF0000000000000 */
			if (type_oid == FLOAT4_ENCOID)
			{
				f4.i = is_negative ? 0xFF800000UL : 0x7F800000UL;
				memcpy(CIPHERDATA_OPE_PTR(result), &f4.f, sizeof(float));
			}
			else
			{
				f8.i = is_negative ? 0xFFF0000000000000ULL : 0x7FF0000000000000ULL;
				memcpy(CIPHERDATA_OPE_PTR(result), &f8.d, sizeof(double));
			}

			if (val != input_str)
				pfree((void *) val);

			PG_RETURN_POINTER(result);
		}

		if (val != input_str)
			pfree((void *) val);
	}

	/* Validate hex string format */
	if (!is_valid_hex_string(input_str))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid hex string: \"%s\"", input_str),
				 errdetail("Hex string must contain only 0-9, a-f, A-F characters and may have \\x prefix.")));

	/* Decode hex input */
	decoded_data = hex_to_binary(input_str, &decoded_len);
	if (decoded_data == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid hex data for encrypted type")));

	/* Check minimum size: vl_len_ + version + dek_dist_oid */
	if (decoded_len < CIPHERDATA_MIN_SIZE)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid encrypted data: too short")));

	/* Parse base header (decoded_data includes vl_len_ at offset 0) */
	ptr = decoded_data;
	ptr += sizeof(int32);	/* skip vl_len_ */
	version = *(int32 *) ptr;	/* packed: major|mask */
	ptr += sizeof(int32);
	dek_dist_oid = *(Oid *) ptr;
	ptr += sizeof(Oid);

	/* ReqID:TECC4.0 - The length fields live in the header the version
	 * describes, so the buffer must be able to hold that header before we read
	 * them; the minimum-size test above only covers version + DEK id. */
	if (decoded_len < CIPHERDATA_HEADER_SIZE(version))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid encrypted data: truncated header")));

	/* ReqID:TECC4.0 - Read length fields.
	 * v1.0: always four base lengths, never EG.
	 * Legacy fixed v2.0: all six lengths, with no segment mask.
	 * Compact v2.0: one length per segment whose mask bit is set. */
	if (CIPHERDATA_MAJOR_VERSION(version) == CIPHERDATA_VERSION_1_0)
	{
		rnd_len = *(int32 *) ptr;	ptr += sizeof(int32);
		det_len = *(int32 *) ptr;	ptr += sizeof(int32);
		ope_len = *(int32 *) ptr;	ptr += sizeof(int32);
		search_len = *(int32 *) ptr;	ptr += sizeof(int32);
		eg_add_len = 0;
		eg_mul_len = 0;
	}
	else if (CIPHERDATA_IS_LEGACY_FIXED_V2(version))
	{
		if (decoded_len < CIPHERDATA_HEADER_SIZE_V2_FIXED)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
					 errmsg("invalid encrypted data: legacy V2 header is truncated")));
		rnd_len = *(int32 *) ptr;	ptr += sizeof(int32);
		det_len = *(int32 *) ptr;	ptr += sizeof(int32);
		ope_len = *(int32 *) ptr;	ptr += sizeof(int32);
		search_len = *(int32 *) ptr;	ptr += sizeof(int32);
		eg_add_len = *(int32 *) ptr;	ptr += sizeof(int32);
		eg_mul_len = *(int32 *) ptr;	ptr += sizeof(int32);
	}
	else
	{
		uint16	mask = CIPHERDATA_SEG_MASK(version);

		rnd_len = (mask & CIPHERDATA_SEG_RND) ? *(int32 *) ptr : 0;
		if (mask & CIPHERDATA_SEG_RND)		ptr += sizeof(int32);
		det_len = (mask & CIPHERDATA_SEG_DET) ? *(int32 *) ptr : 0;
		if (mask & CIPHERDATA_SEG_DET)		ptr += sizeof(int32);
		ope_len = (mask & CIPHERDATA_SEG_OPE) ? *(int32 *) ptr : 0;
		if (mask & CIPHERDATA_SEG_OPE)		ptr += sizeof(int32);
		search_len = (mask & CIPHERDATA_SEG_SEARCH) ? *(int32 *) ptr : 0;
		if (mask & CIPHERDATA_SEG_SEARCH)	ptr += sizeof(int32);
		eg_add_len = (mask & CIPHERDATA_SEG_EG_ADD) ? *(int32 *) ptr : 0;
		if (mask & CIPHERDATA_SEG_EG_ADD)	ptr += sizeof(int32);
		eg_mul_len = (mask & CIPHERDATA_SEG_EG_MUL) ? *(int32 *) ptr : 0;
		if (mask & CIPHERDATA_SEG_EG_MUL)	ptr += sizeof(int32);
	}

	/* ReqID:TECC4.0 - Reject negative segment lengths: they drive both the
	 * allocation size and the payload offsets, so a negative value would shift
	 * every later segment out of the structure. */
	if (rnd_len < 0 || det_len < 0 || ope_len < 0 || search_len < 0 ||
		eg_add_len < 0 || eg_mul_len < 0)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid encrypted data: negative segment length")));

	/* Validate total size (pre-allocation sanity check).  ReqID:TECC4.0 - The
	 * sum is computed in int64: in int32 an overflowing sum wraps onto the
	 * buffer length and lets an absurd segment length reach the copies. */
	{
		int64		expected = (int64) CIPHERDATA_HEADER_SIZE(version) +
			(int64) rnd_len + (int64) det_len + (int64) ope_len +
			(int64) search_len + (int64) eg_add_len + (int64) eg_mul_len;

		if ((int64) decoded_len != expected)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
					 errmsg("invalid encrypted data: size mismatch")));
	}

	/* ReqID:TECC4.0 - A mask that disagrees with the lengths actually parsed
	 * would send the copies below past the end of the structure. */
	if (!cipherdata_parsed_layout_ok(version, rnd_len, det_len, ope_len,
									 search_len, eg_add_len, eg_mul_len))
		ereport(ERROR,
				(errcode(ERRCODE_TECC_CIPHERDATA_INVALID),
				 errmsg("invalid CipherDataType structure")));

	/* Allocate CipherDataType.  ReqID:TECC4.0 - Reserve the length fields the
	 * kept version implies, not only those of the non-empty segments. */
	result = cipherdata_alloc_for_version(rnd_len, det_len, ope_len, search_len,
										  eg_add_len, eg_mul_len,
										  dek_dist_oid, version);
	/* Modify - ReqID:TECC4.0 - cipherdata_alloc() normalizes legacy fixed V2
	 * input to compact V2.  Restoring its zero-mask version here would make
	 * the pointer macros treat the compact allocation as a 36-byte header. */
	if (!CIPHERDATA_IS_LEGACY_FIXED_V2(version))
		result->version = version;

	/* Copy ciphertexts */
	if (rnd_len > 0)
		memcpy(CIPHERDATA_RND_PTR(result), ptr, rnd_len);
	ptr += rnd_len;

	if (det_len > 0)
		memcpy(CIPHERDATA_DET_PTR(result), ptr, det_len);
	ptr += det_len;

	if (ope_len > 0)
		memcpy(CIPHERDATA_OPE_PTR(result), ptr, ope_len);
	ptr += ope_len;

	if (search_len > 0)
		memcpy(CIPHERDATA_SEARCH_PTR(result), ptr, search_len);

	/* Begin - ReqID:TECC4.0 - Copy EG ciphertexts (v2.0 slots) */
	ptr += search_len;
	if (eg_add_len > 0)
		memcpy(CIPHERDATA_EG_ADD_PTR(result), ptr, eg_add_len);
	ptr += eg_add_len;
	if (eg_mul_len > 0)
		memcpy(CIPHERDATA_EG_MUL_PTR(result), ptr, eg_mul_len);
	/* End - ReqID:TECC4.0 */

	/* Free decoded data */
	pfree(decoded_data);

	/* ReqID:TECC4.0 - Unified validation (version, DEK identity, size) */
	if (!cipherdata_validate(result))
		ereport(ERROR,
				(errcode(ERRCODE_TECC_CIPHERDATA_INVALID),
				 errmsg("invalid CipherDataType structure")));

	/* Return as Datum */
	PG_RETURN_POINTER(result);
}

/*
 * Generic output function for encrypted types
 *
 * Output format: Hexadecimal encoded binary data with PostgreSQL \x prefix
 */
static Datum
cipherdata_out(PG_FUNCTION_ARGS)
{
	CipherDataType *orig = (CipherDataType *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	CipherDataType *cipherdata = cipherdata_convert_to_4b_header(orig);
	bool		need_free = (cipherdata != orig);
	size_t		total_size;
	char	   *result_str;
	unsigned char *ptr;

	/* Validate input */
	if (!cipherdata_validate(cipherdata))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_BINARY_REPRESENTATION),
				 errmsg("invalid CipherDataType structure")));

	/*
	 * ReqID:TECC4.0 - Float sentinels output plain Infinity text, not hex.
	 * Round-trip: cipherdata_in("Infinity") → sentinel → cipherdata_out() → "Infinity"
	 */
	if (CIPHERDATA_VERSION(cipherdata) == CIPHERDATA_VERSION_FLOAT_SENTINEL)
	{
		bool is_negative = CIPHERDATA_SENTINEL_IS_NEGATIVE(cipherdata);

		if (need_free)
			pfree(cipherdata);
		PG_RETURN_CSTRING(pstrdup(is_negative ? "-Infinity" : "Infinity"));
	}

	/*
	 * ReqID:TECC4.0 - pg_upgrade V4.0.0 兼容：v1.0 密文原样输出。
	 * 下面的重建路径按 v2.0 紧凑布局（mask 决定长度字段个数）重建，会把
	 * v1.0（28 字节固定头）密文重排、破坏字节流——升级后旧密文经前端解
	 * 十六进制后头部长度校验失败（integrity check failed）。
	 * v1.0 直接输出原始 varlena 字节，与 V4.0.0 输出格式一致。
	 */
	if (CIPHERDATA_VERSION(cipherdata) == CIPHERDATA_VERSION_1_0)
	{
		Size		vlen = (VARATT_IS_1B(cipherdata)) ?
			VARSIZE_1B(cipherdata) : VARSIZE_4B(cipherdata);

		result_str = binary_to_hex((const unsigned char *) cipherdata, vlen);
		if (need_free)
			pfree(cipherdata);
		if (result_str == NULL)
			ereport(ERROR,
					(errcode(ERRCODE_OUT_OF_MEMORY),
					 errmsg("out of memory")));
		PG_RETURN_CSTRING(result_str);
	}

	/* ReqID:TECC4.0 - v2.0 compact format output: only write present segments */
	{
		int32 rnd_len32 = CIPHERDATA_DATUM_RND_LEN(cipherdata);
		int32 det_len32 = CIPHERDATA_DATUM_DET_LEN(cipherdata);
		int32 ope_len32 = CIPHERDATA_DATUM_OPE_LEN(cipherdata);
		int32 search_len32 = CIPHERDATA_DATUM_SEARCH_LEN(cipherdata);
		int32 eg_add_len32 = CIPHERDATA_DATUM_EG_ADD_LEN(cipherdata);
		int32 eg_mul_len32 = CIPHERDATA_DATUM_EG_MUL_LEN(cipherdata);
		uint16 mask;
		unsigned char *buf;
		char *hdr_ptr;

		/* Compute mask from actual lengths */
		mask = 0;
		if (rnd_len32 > 0)		mask |= CIPHERDATA_SEG_RND;
		if (det_len32 > 0)		mask |= CIPHERDATA_SEG_DET;
		if (ope_len32 > 0)		mask |= CIPHERDATA_SEG_OPE;
		if (search_len32 > 0)	mask |= CIPHERDATA_SEG_SEARCH;
		if (eg_add_len32 > 0)	mask |= CIPHERDATA_SEG_EG_ADD;
		if (eg_mul_len32 > 0)	mask |= CIPHERDATA_SEG_EG_MUL;

		/* Calculate total size with compact header */
		total_size = sizeof(int32) + CIPHERDATA_FIELDS_BASE_SIZE +
					 CIPHERDATA_POPCOUNT16(mask) * (Size)sizeof(int32) +
					 rnd_len32 + det_len32 + ope_len32 + search_len32 +
					 eg_add_len32 + eg_mul_len32;

		buf = (unsigned char *) palloc(total_size);
		ptr = buf;

		/* Write varlena header */
		SET_VARSIZE(ptr, total_size);
		ptr += sizeof(int32);

		/* Write version with mask in low 16 bits */
		*(int32 *) ptr = CIPHERDATA_DATUM_VERSION(cipherdata) | (int32)mask;
		ptr += sizeof(int32);

		/* Write dek_dist_oid */
		*(Oid *) ptr = CIPHERDATA_DATUM_DEK_DIST_OID(cipherdata);
		ptr += sizeof(Oid);

		/* Write length fields only for present segments */
		hdr_ptr = (char *) ptr;
		if (mask & CIPHERDATA_SEG_RND)		{ *(int32 *)hdr_ptr = rnd_len32;	hdr_ptr += sizeof(int32); }
		if (mask & CIPHERDATA_SEG_DET)		{ *(int32 *)hdr_ptr = det_len32;	hdr_ptr += sizeof(int32); }
		if (mask & CIPHERDATA_SEG_OPE)		{ *(int32 *)hdr_ptr = ope_len32;	hdr_ptr += sizeof(int32); }
		if (mask & CIPHERDATA_SEG_SEARCH)	{ *(int32 *)hdr_ptr = search_len32; hdr_ptr += sizeof(int32); }
		if (mask & CIPHERDATA_SEG_EG_ADD)	{ *(int32 *)hdr_ptr = eg_add_len32; hdr_ptr += sizeof(int32); }
		if (mask & CIPHERDATA_SEG_EG_MUL)	{ *(int32 *)hdr_ptr = eg_mul_len32; hdr_ptr += sizeof(int32); }
		ptr = (unsigned char *) hdr_ptr;

		/* Write ciphertext data */
		if (rnd_len32 > 0)
		{
			memcpy(ptr, CIPHERDATA_DATUM_RND_PTR(cipherdata), rnd_len32);
			ptr += rnd_len32;
		}
		if (det_len32 > 0)
		{
			memcpy(ptr, CIPHERDATA_DATUM_DET_PTR(cipherdata), det_len32);
			ptr += det_len32;
		}
		if (ope_len32 > 0)
		{
			memcpy(ptr, CIPHERDATA_DATUM_OPE_PTR(cipherdata), ope_len32);
			ptr += ope_len32;
		}
		if (search_len32 > 0)
		{
			memcpy(ptr, CIPHERDATA_DATUM_SEARCH_PTR(cipherdata), search_len32);
			ptr += search_len32;
		}
		if (eg_add_len32 > 0)
		{
			memcpy(ptr, CIPHERDATA_DATUM_EG_ADD_PTR(cipherdata), eg_add_len32);
			ptr += eg_add_len32;
		}
		if (eg_mul_len32 > 0)
		{
			memcpy(ptr, CIPHERDATA_DATUM_EG_MUL_PTR(cipherdata), eg_mul_len32);
			ptr += eg_mul_len32;
		}

		ptr = buf;
	}

	/* Convert to hex string */
	result_str = binary_to_hex(ptr, total_size);
	pfree(ptr);				/* 释放临时密文二进制缓冲区 buf */
	if (need_free)
		pfree(cipherdata);	/* 释放 1-byte header 转换副本 */

	if (result_str == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_OUT_OF_MEMORY),
				 errmsg("out of memory")));
	PG_RETURN_CSTRING(result_str);
}

/*
 * Generic receive function for encrypted types
 */
static Datum
cipherdata_recv(PG_FUNCTION_ARGS, Oid type_oid)
{
	StringInfo	buf = (StringInfo) PG_GETARG_POINTER(0);
	CipherDataType *result;
	Oid			dek_dist_oid;
	int32		rnd_len;
	int32		det_len;
	int32		ope_len;
	int32		search_len;
	/* ReqID:TECC4.0 */
	int32		eg_add_len;
	int32		eg_mul_len;
	/* End - ReqID:TECC4.0 */
	int32		version;
	int32		major;
	uint16		mask;

	/* Read base header from binary stream */
	version = pq_getmsgint(buf, sizeof(int32));	/* packed: major|mask */
	major = CIPHERDATA_MAJOR_VERSION(version);
	mask = CIPHERDATA_SEG_MASK(version);
	dek_dist_oid = pq_getmsgint(buf, sizeof(Oid));

	/* ReqID:TECC4.0 - Read length fields.
	 * v1.0 always carries four base lengths, legacy fixed V2 carries all six,
	 * and compact V2 carries only lengths selected by the mask. */
	if (major == CIPHERDATA_VERSION_1_0)
	{
		rnd_len = pq_getmsgint(buf, sizeof(int32));
		det_len = pq_getmsgint(buf, sizeof(int32));
		ope_len = pq_getmsgint(buf, sizeof(int32));
		search_len = pq_getmsgint(buf, sizeof(int32));
		eg_add_len = 0;
		eg_mul_len = 0;
	}
	else if (CIPHERDATA_IS_LEGACY_FIXED_V2(version))
	{
		rnd_len = pq_getmsgint(buf, sizeof(int32));
		det_len = pq_getmsgint(buf, sizeof(int32));
		ope_len = pq_getmsgint(buf, sizeof(int32));
		search_len = pq_getmsgint(buf, sizeof(int32));
		eg_add_len = pq_getmsgint(buf, sizeof(int32));
		eg_mul_len = pq_getmsgint(buf, sizeof(int32));
	}
	else
	{
		rnd_len = (mask & CIPHERDATA_SEG_RND) ? pq_getmsgint(buf, sizeof(int32)) : 0;
		det_len = (mask & CIPHERDATA_SEG_DET) ? pq_getmsgint(buf, sizeof(int32)) : 0;
		ope_len = (mask & CIPHERDATA_SEG_OPE) ? pq_getmsgint(buf, sizeof(int32)) : 0;
		search_len = (mask & CIPHERDATA_SEG_SEARCH) ? pq_getmsgint(buf, sizeof(int32)) : 0;
		eg_add_len = (mask & CIPHERDATA_SEG_EG_ADD) ? pq_getmsgint(buf, sizeof(int32)) : 0;
		eg_mul_len = (mask & CIPHERDATA_SEG_EG_MUL) ? pq_getmsgint(buf, sizeof(int32)) : 0;
	}

	/* ReqID:TECC4.0 - Same guards as the text input path: a malformed segment
	 * header must not be able to steer the payload copies (nor the allocation)
	 * outside the structure. */
	if (rnd_len < 0 || det_len < 0 || ope_len < 0 || search_len < 0 ||
		eg_add_len < 0 || eg_mul_len < 0 ||
		!cipherdata_parsed_layout_ok(version, rnd_len, det_len, ope_len,
									 search_len, eg_add_len, eg_mul_len))
		ereport(ERROR,
				(errcode(ERRCODE_TECC_CIPHERDATA_INVALID),
				 errmsg("invalid CipherDataType structure")));

	/* ReqID:TECC4.0 - The header has been consumed by the reads above, so the
	 * bytes left in the message must cover the payload the lengths promise. */
	{
		int64		payload = (int64) rnd_len + (int64) det_len + (int64) ope_len +
			(int64) search_len + (int64) eg_add_len + (int64) eg_mul_len;

		if ((int64) (buf->len - buf->cursor) < payload)
			ereport(ERROR,
					(errcode(ERRCODE_TECC_CIPHERDATA_INVALID),
					 errmsg("insufficient data left in message")));
	}

	/* Allocate CipherDataType (version/DEK validated in cipherdata_validate below).
	 * ReqID:TECC4.0 - The reserved length fields follow the kept version. */
	result = cipherdata_alloc_for_version(rnd_len, det_len, ope_len, search_len,
										  eg_add_len, eg_mul_len,
										  dek_dist_oid, version);
	/* Preserve ordinary packed versions and float sentinels.  Legacy fixed V2
	 * is normalized by the allocator so its pointer layout stays valid. */
	if (!CIPHERDATA_IS_LEGACY_FIXED_V2(version))
		result->version = version;

	/* Read ciphertexts */
	if (rnd_len > 0)
		pq_copymsgbytes(buf, CIPHERDATA_RND_PTR(result), rnd_len);
	if (det_len > 0)
		pq_copymsgbytes(buf, CIPHERDATA_DET_PTR(result), det_len);
	if (ope_len > 0)
		pq_copymsgbytes(buf, CIPHERDATA_OPE_PTR(result), ope_len);
	if (search_len > 0)
		pq_copymsgbytes(buf, CIPHERDATA_SEARCH_PTR(result), search_len);

	/* ReqID:TECC4.0 - Read EG ciphertexts */
	if (eg_add_len > 0)
		pq_copymsgbytes(buf, CIPHERDATA_EG_ADD_PTR(result), eg_add_len);
	if (eg_mul_len > 0)
		pq_copymsgbytes(buf, CIPHERDATA_EG_MUL_PTR(result), eg_mul_len);

	/* ReqID:TECC4.0 - Unified validation (version, DEK identity, size) */
	if (!cipherdata_validate(result))
		ereport(ERROR,
				(errcode(ERRCODE_TECC_CIPHERDATA_INVALID),
				 errmsg("invalid CipherDataType structure")));

	PG_RETURN_POINTER(result);
}

/*
 * Generic send function for encrypted types
 */
static Datum
cipherdata_send(PG_FUNCTION_ARGS)
{
	CipherDataType *orig = (CipherDataType *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));
	CipherDataType *cipherdata = cipherdata_convert_to_4b_header(orig);
	bool		need_free = (cipherdata != orig);
	StringInfoData buf;
	int32		send_version;
	uint16		send_mask;

	/* Validate input */
	if (!cipherdata_validate(cipherdata))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_BINARY_REPRESENTATION),
				 errmsg("invalid CipherDataType structure")));

	/* Initialize buffer */
	pq_begintypsend(&buf);

	/* Modify - ReqID:TECC4.0 - Normalize legacy fixed V2 to compact V2 on
	 * output so receivers can determine which length fields follow. */
	send_version = CIPHERDATA_RAW_VERSION(cipherdata);
	send_mask = CIPHERDATA_DATUM_SEG_MASK(cipherdata);
	if (CIPHERDATA_IS_LEGACY_FIXED_V2(send_version))
	{
		send_mask = 0;
		if (CIPHERDATA_DATUM_RND_LEN(cipherdata) > 0)
			send_mask |= CIPHERDATA_SEG_RND;
		if (CIPHERDATA_DATUM_DET_LEN(cipherdata) > 0)
			send_mask |= CIPHERDATA_SEG_DET;
		if (CIPHERDATA_DATUM_OPE_LEN(cipherdata) > 0)
			send_mask |= CIPHERDATA_SEG_OPE;
		if (CIPHERDATA_DATUM_SEARCH_LEN(cipherdata) > 0)
			send_mask |= CIPHERDATA_SEG_SEARCH;
		if (CIPHERDATA_DATUM_EG_ADD_LEN(cipherdata) > 0)
			send_mask |= CIPHERDATA_SEG_EG_ADD;
		if (CIPHERDATA_DATUM_EG_MUL_LEN(cipherdata) > 0)
			send_mask |= CIPHERDATA_SEG_EG_MUL;
		send_version = CIPHERDATA_MAKE_VERSION(CIPHERDATA_VERSION_2_0,
										   send_mask);
	}

	/* Write base header (version carries the segment mask) */
	pq_sendint32(&buf, send_version);
	pq_sendint32(&buf, CIPHERDATA_DATUM_DEK_DIST_OID(cipherdata));

	/* ReqID:TECC4.0 - Write length fields.
	 * v1.0 carries the four base lengths unconditionally and has no EG slots.
	 * v2.0 writes a length only for segments present in the mask — matching
	 * cipherdata_recv exactly (the old code always wrote two extra zero EG
	 * lengths for v1.0, which desynchronised the read). */
	if (CIPHERDATA_VERSION(cipherdata) == CIPHERDATA_VERSION_1_0)
	{
		pq_sendint32(&buf, CIPHERDATA_DATUM_RND_LEN(cipherdata));
		pq_sendint32(&buf, CIPHERDATA_DATUM_DET_LEN(cipherdata));
		pq_sendint32(&buf, CIPHERDATA_DATUM_OPE_LEN(cipherdata));
		pq_sendint32(&buf, CIPHERDATA_DATUM_SEARCH_LEN(cipherdata));
	}
	else
	{
		if (send_mask & CIPHERDATA_SEG_RND)
			pq_sendint32(&buf, CIPHERDATA_DATUM_RND_LEN(cipherdata));
		if (send_mask & CIPHERDATA_SEG_DET)
			pq_sendint32(&buf, CIPHERDATA_DATUM_DET_LEN(cipherdata));
		if (send_mask & CIPHERDATA_SEG_OPE)
			pq_sendint32(&buf, CIPHERDATA_DATUM_OPE_LEN(cipherdata));
		if (send_mask & CIPHERDATA_SEG_SEARCH)
			pq_sendint32(&buf, CIPHERDATA_DATUM_SEARCH_LEN(cipherdata));
		if (send_mask & CIPHERDATA_SEG_EG_ADD)
			pq_sendint32(&buf, CIPHERDATA_DATUM_EG_ADD_LEN(cipherdata));
		if (send_mask & CIPHERDATA_SEG_EG_MUL)
			pq_sendint32(&buf, CIPHERDATA_DATUM_EG_MUL_LEN(cipherdata));
	}

	/* Write ciphertexts */
	if (CIPHERDATA_DATUM_RND_LEN(cipherdata) > 0)
		pq_sendbytes(&buf, CIPHERDATA_DATUM_RND_PTR(cipherdata),
					 CIPHERDATA_DATUM_RND_LEN(cipherdata));
	if (CIPHERDATA_DATUM_DET_LEN(cipherdata) > 0)
		pq_sendbytes(&buf, CIPHERDATA_DATUM_DET_PTR(cipherdata),
					 CIPHERDATA_DATUM_DET_LEN(cipherdata));
	if (CIPHERDATA_DATUM_OPE_LEN(cipherdata) > 0)
		pq_sendbytes(&buf, CIPHERDATA_DATUM_OPE_PTR(cipherdata),
					 CIPHERDATA_DATUM_OPE_LEN(cipherdata));
	if (CIPHERDATA_DATUM_SEARCH_LEN(cipherdata) > 0)
		pq_sendbytes(&buf, CIPHERDATA_DATUM_SEARCH_PTR(cipherdata),
					 CIPHERDATA_DATUM_SEARCH_LEN(cipherdata));
	/* ReqID:TECC4.0 - Write EG ciphertexts (v1.0 has no EG slots; its
	 * EG length macros already return 0 / NULL) */
	if (CIPHERDATA_DATUM_EG_ADD_LEN(cipherdata) > 0)
		pq_sendbytes(&buf, CIPHERDATA_DATUM_EG_ADD_PTR(cipherdata),
					 CIPHERDATA_DATUM_EG_ADD_LEN(cipherdata));
	if (CIPHERDATA_DATUM_EG_MUL_LEN(cipherdata) > 0)
		pq_sendbytes(&buf, CIPHERDATA_DATUM_EG_MUL_PTR(cipherdata),
					 CIPHERDATA_DATUM_EG_MUL_LEN(cipherdata));

	if (need_free)
		pfree(cipherdata);	/* 释放 1-byte header 转换副本 */

	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

/*
 * Type-specific I/O functions
 * These are thin wrappers around the generic cipherdata functions
 */

/* int2_enc functions */
PG_FUNCTION_INFO_V1(int2_enc_in);
Datum
int2_enc_in(PG_FUNCTION_ARGS)
{
	return cipherdata_in(fcinfo, INT2_ENCOID);
}

PG_FUNCTION_INFO_V1(int2_enc_out);
Datum
int2_enc_out(PG_FUNCTION_ARGS)
{
	return cipherdata_out(fcinfo);
}

PG_FUNCTION_INFO_V1(int2_enc_recv);
Datum
int2_enc_recv(PG_FUNCTION_ARGS)
{
	return cipherdata_recv(fcinfo, INT2_ENCOID);
}

PG_FUNCTION_INFO_V1(int2_enc_send);
Datum
int2_enc_send(PG_FUNCTION_ARGS)
{
	return cipherdata_send(fcinfo);
}

/* int4_enc functions */
PG_FUNCTION_INFO_V1(int4_enc_in);
Datum
int4_enc_in(PG_FUNCTION_ARGS)
{
	return cipherdata_in(fcinfo, INT4_ENCOID);
}

PG_FUNCTION_INFO_V1(int4_enc_out);
Datum
int4_enc_out(PG_FUNCTION_ARGS)
{
	return cipherdata_out(fcinfo);
}

PG_FUNCTION_INFO_V1(int4_enc_recv);
Datum
int4_enc_recv(PG_FUNCTION_ARGS)
{
	return cipherdata_recv(fcinfo, INT4_ENCOID);
}

PG_FUNCTION_INFO_V1(int4_enc_send);
Datum
int4_enc_send(PG_FUNCTION_ARGS)
{
	return cipherdata_send(fcinfo);
}

/* int8_enc functions */
PG_FUNCTION_INFO_V1(int8_enc_in);
Datum
int8_enc_in(PG_FUNCTION_ARGS)
{
	return cipherdata_in(fcinfo, INT8_ENCOID);
}

PG_FUNCTION_INFO_V1(int8_enc_out);
Datum
int8_enc_out(PG_FUNCTION_ARGS)
{
	return cipherdata_out(fcinfo);
}

PG_FUNCTION_INFO_V1(int8_enc_recv);
Datum
int8_enc_recv(PG_FUNCTION_ARGS)
{
	return cipherdata_recv(fcinfo, INT8_ENCOID);
}

PG_FUNCTION_INFO_V1(int8_enc_send);
Datum
int8_enc_send(PG_FUNCTION_ARGS)
{
	return cipherdata_send(fcinfo);
}

/* float4_enc functions */
PG_FUNCTION_INFO_V1(float4_enc_in);
Datum
float4_enc_in(PG_FUNCTION_ARGS)
{
	return cipherdata_in(fcinfo, FLOAT4_ENCOID);
}

PG_FUNCTION_INFO_V1(float4_enc_out);
Datum
float4_enc_out(PG_FUNCTION_ARGS)
{
	return cipherdata_out(fcinfo);
}

PG_FUNCTION_INFO_V1(float4_enc_recv);
Datum
float4_enc_recv(PG_FUNCTION_ARGS)
{
	return cipherdata_recv(fcinfo, FLOAT4_ENCOID);
}

PG_FUNCTION_INFO_V1(float4_enc_send);
Datum
float4_enc_send(PG_FUNCTION_ARGS)
{
	return cipherdata_send(fcinfo);
}

/* float8_enc functions */
PG_FUNCTION_INFO_V1(float8_enc_in);
Datum
float8_enc_in(PG_FUNCTION_ARGS)
{
	return cipherdata_in(fcinfo, FLOAT8_ENCOID);
}

PG_FUNCTION_INFO_V1(float8_enc_out);
Datum
float8_enc_out(PG_FUNCTION_ARGS)
{
	return cipherdata_out(fcinfo);
}

PG_FUNCTION_INFO_V1(float8_enc_recv);
Datum
float8_enc_recv(PG_FUNCTION_ARGS)
{
	return cipherdata_recv(fcinfo, FLOAT8_ENCOID);
}

PG_FUNCTION_INFO_V1(float8_enc_send);
Datum
float8_enc_send(PG_FUNCTION_ARGS)
{
	return cipherdata_send(fcinfo);
}

/* numeric_enc functions */
PG_FUNCTION_INFO_V1(numeric_enc_in);
Datum
numeric_enc_in(PG_FUNCTION_ARGS)
{
	return cipherdata_in(fcinfo, NUMERIC_ENCOID);
}

PG_FUNCTION_INFO_V1(numeric_enc_out);
Datum
numeric_enc_out(PG_FUNCTION_ARGS)
{
	return cipherdata_out(fcinfo);
}

PG_FUNCTION_INFO_V1(numeric_enc_recv);
Datum
numeric_enc_recv(PG_FUNCTION_ARGS)
{
	return cipherdata_recv(fcinfo, NUMERIC_ENCOID);
}

PG_FUNCTION_INFO_V1(numeric_enc_send);
Datum
numeric_enc_send(PG_FUNCTION_ARGS)
{
	return cipherdata_send(fcinfo);
}

/* varchar_enc functions */
PG_FUNCTION_INFO_V1(varchar_enc_in);
Datum
varchar_enc_in(PG_FUNCTION_ARGS)
{
	return cipherdata_in(fcinfo, VARCHAR_ENCOID);
}

PG_FUNCTION_INFO_V1(varchar_enc_out);
Datum
varchar_enc_out(PG_FUNCTION_ARGS)
{
	return cipherdata_out(fcinfo);
}

PG_FUNCTION_INFO_V1(varchar_enc_recv);
Datum
varchar_enc_recv(PG_FUNCTION_ARGS)
{
	return cipherdata_recv(fcinfo, VARCHAR_ENCOID);
}

PG_FUNCTION_INFO_V1(varchar_enc_send);
Datum
varchar_enc_send(PG_FUNCTION_ARGS)
{
	return cipherdata_send(fcinfo);
}

/* bpchar_enc functions */
PG_FUNCTION_INFO_V1(bpchar_enc_in);
Datum
bpchar_enc_in(PG_FUNCTION_ARGS)
{
	return cipherdata_in(fcinfo, BPCHAR_ENCOID);
}

PG_FUNCTION_INFO_V1(bpchar_enc_out);
Datum
bpchar_enc_out(PG_FUNCTION_ARGS)
{
	return cipherdata_out(fcinfo);
}

PG_FUNCTION_INFO_V1(bpchar_enc_recv);
Datum
bpchar_enc_recv(PG_FUNCTION_ARGS)
{
	return cipherdata_recv(fcinfo, BPCHAR_ENCOID);
}

PG_FUNCTION_INFO_V1(bpchar_enc_send);
Datum
bpchar_enc_send(PG_FUNCTION_ARGS)
{
	return cipherdata_send(fcinfo);
}

/* text_enc functions */
PG_FUNCTION_INFO_V1(text_enc_in);
Datum
text_enc_in(PG_FUNCTION_ARGS)
{
	return cipherdata_in(fcinfo, TEXT_ENCOID);
}

PG_FUNCTION_INFO_V1(text_enc_out);
Datum
text_enc_out(PG_FUNCTION_ARGS)
{
	return cipherdata_out(fcinfo);
}

PG_FUNCTION_INFO_V1(text_enc_recv);
Datum
text_enc_recv(PG_FUNCTION_ARGS)
{
	return cipherdata_recv(fcinfo, TEXT_ENCOID);
}

PG_FUNCTION_INFO_V1(text_enc_send);
Datum
text_enc_send(PG_FUNCTION_ARGS)
{
	return cipherdata_send(fcinfo);
}
