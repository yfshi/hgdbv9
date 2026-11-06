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
 * cipherdata_alloc - Allocate a CipherDataType structure
 *
 * Allocates memory for a CipherDataType structure with the specified
 * ciphertext lengths. The structure includes the varlena header.
 */
CipherDataType *
cipherdata_alloc(int32 rnd_len, int32 det_len, int32 ope_len, int32 search_len, Oid dek_dist_oid)
{
	Size		total_size;
	CipherDataType *result;

	/* Calculate total size including varlena header */
	total_size = sizeof(CipherDataType) + rnd_len + det_len + ope_len + search_len;

	/* Allocate memory */
	result = (CipherDataType *) palloc(total_size);

	/* Set varlena header - always use 4-byte.
	 * ReqID:TECC4.0 - CIPHERDATA_DATUM_* macros assume 4-byte
	 * header offsets.  1-byte short header would shift all
	 * field reads by 3 bytes. */
	SET_VARSIZE_4B(result, total_size);

	/* Initialize fields */
	result->version = CIPHERDATA_CURRENT_VERSION;
	result->dek_dist_oid = dek_dist_oid;
	result->rnd_length = rnd_len;
	result->det_length = det_len;
	result->ope_length = ope_len;
	result->search_length = search_len;

	/* Zero out the data area */
	if (total_size > sizeof(CipherDataType))
		memset(result->data, 0, rnd_len + det_len + ope_len + search_len);

	return result;
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
	int32		version;
	int32		rnd_len;
	int32		det_len;
	int32		ope_len;
	int32		search_len;

	if (cipherdata == NULL)
		return false;

	/* Get varlena size, handling both 1-byte and 4-byte headers */
	if (VARATT_IS_1B(cipherdata))
		vlen = VARSIZE_1B(cipherdata);
	else
		vlen = VARSIZE_4B(cipherdata);

	/* Minimum: must be at least the fixed-size header */
	if (vlen < sizeof(CipherDataType))
		return false;

	/* Use unified VARDATA_ANY-safe macros from cipherdatatype.h */
	version    = CIPHERDATA_DATUM_VERSION(cipherdata);
	rnd_len    = CIPHERDATA_DATUM_RND_LEN(cipherdata);
	det_len    = CIPHERDATA_DATUM_DET_LEN(cipherdata);
	ope_len    = CIPHERDATA_DATUM_OPE_LEN(cipherdata);
	search_len = CIPHERDATA_DATUM_SEARCH_LEN(cipherdata);

	/* Check version */
	if (!CIPHERDATA_VERSION_VALID(version))
		return false;

	/* ReqID:TECC4.0 - Validate DEK distributed ID */
	if (!OidIsValid(CIPHERDATA_DATUM_DEK_DIST_OID(cipherdata)))
		return false;

	/* ReqID:TECC4.0 - Reject negative lengths (prevent unsigned overflow in size calc) */
	if (rnd_len < 0 || det_len < 0 || ope_len < 0 || search_len < 0)
		return false;

	/* Calculate expected size — use CIPHERDATA_FIXED_SIZE minus varlena header */
	{
		Size hdr_sz = VARATT_IS_1B(cipherdata) ? 1 : sizeof(int32);
		expected_size = hdr_sz + (CIPHERDATA_FIXED_SIZE - sizeof(int32)) +
				rnd_len + det_len + ope_len + search_len;
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
	unsigned char *ptr;

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

	/* Check minimum size (CIPHERDATA_FIXED_SIZE = 28 bytes, includes vl_len_) */
	if (decoded_len < CIPHERDATA_FIXED_SIZE)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid encrypted data: too short")));

	/* Parse header (now includes vl_len_ at offset 0) */
	ptr = decoded_data;
	ptr += sizeof(int32);	/* skip vl_len_ */
	ptr += sizeof(int32);	/* skip version, validated in cipherdata_validate */
	dek_dist_oid = *(Oid *) ptr;
	ptr += sizeof(Oid);
	rnd_len = *(int32 *) ptr;
	ptr += sizeof(int32);
	det_len = *(int32 *) ptr;
	ptr += sizeof(int32);
	ope_len = *(int32 *) ptr;
	ptr += sizeof(int32);
	search_len = *(int32 *) ptr;
	ptr += sizeof(int32);

	/* Validate total size (pre-allocation sanity check) */
	if (decoded_len != CIPHERDATA_FIXED_SIZE + rnd_len + det_len + ope_len + search_len)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid encrypted data: size mismatch")));

	/* Allocate CipherDataType */
	result = cipherdata_alloc(rnd_len, det_len, ope_len, search_len, dek_dist_oid);

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

	/* ReqID:TECC4.0 - Output all ciphertexts (RND+DET+OPE+SEARCH) for
	 * export/import data consistency.  Previously only RND was output. */
	{
		int32 rnd_len32 = CIPHERDATA_RND_LENGTH(cipherdata);
		int32 det_len32 = CIPHERDATA_DET_LENGTH(cipherdata);
		int32 ope_len32 = CIPHERDATA_OPE_LENGTH(cipherdata);
		int32 search_len32 = CIPHERDATA_SEARCH_LENGTH(cipherdata);
		unsigned char *buf;		/* ReqID:TECC4.0 - fix ptr arithmetic bug */

		total_size = CIPHERDATA_FIXED_SIZE + rnd_len32 + det_len32 + ope_len32 + search_len32;

		buf = (unsigned char *) palloc(total_size);
		ptr = buf;

		SET_VARSIZE(ptr, total_size);
		ptr += sizeof(int32);
		*(int32 *) ptr = CIPHERDATA_VERSION_1_0;
		ptr += sizeof(int32);
		*(Oid *) ptr = CIPHERDATA_DEK_DIST_OID(cipherdata);
		ptr += sizeof(Oid);
		*(int32 *) ptr = rnd_len32;
		ptr += sizeof(int32);
		*(int32 *) ptr = det_len32;
		ptr += sizeof(int32);
		*(int32 *) ptr = ope_len32;
		ptr += sizeof(int32);
		*(int32 *) ptr = search_len32;
		ptr += sizeof(int32);

		if (rnd_len32 > 0)
		{
			memcpy(ptr, CIPHERDATA_RND_PTR(cipherdata), rnd_len32);
			ptr += rnd_len32;
		}
		if (det_len32 > 0)
		{
			memcpy(ptr, CIPHERDATA_DET_PTR(cipherdata), det_len32);
			ptr += det_len32;
		}
		if (ope_len32 > 0)
		{
			memcpy(ptr, CIPHERDATA_OPE_PTR(cipherdata), ope_len32);
			ptr += ope_len32;
		}
		if (search_len32 > 0)
		{
			memcpy(ptr, CIPHERDATA_SEARCH_PTR(cipherdata), search_len32);
			ptr += search_len32;
		}

		ptr = buf;
	}

	/* Convert to hex string */
	result_str = binary_to_hex(ptr, total_size);

	if (result_str == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_OUT_OF_MEMORY),
				 errmsg("out of memory")));
	PG_RETURN_CSTRING(result_str);
	if (need_free)
		pfree(cipherdata);
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

	/* Read header from binary stream */
	(void) pq_getmsgint(buf, sizeof(int32));	/* skip version, validated below */
	dek_dist_oid = pq_getmsgint(buf, sizeof(Oid));
	rnd_len = pq_getmsgint(buf, sizeof(int32));
	det_len = pq_getmsgint(buf, sizeof(int32));
	ope_len = pq_getmsgint(buf, sizeof(int32));
	search_len = pq_getmsgint(buf, sizeof(int32));

	/* Allocate CipherDataType (version/DEK validated in cipherdata_validate below) */
	result = cipherdata_alloc(rnd_len, det_len, ope_len, search_len, dek_dist_oid);

	/* Read ciphertexts */
	if (rnd_len > 0)
		pq_copymsgbytes(buf, CIPHERDATA_RND_PTR(result), rnd_len);
	if (det_len > 0)
		pq_copymsgbytes(buf, CIPHERDATA_DET_PTR(result), det_len);
	if (ope_len > 0)
		pq_copymsgbytes(buf, CIPHERDATA_OPE_PTR(result), ope_len);
	if (search_len > 0)
		pq_copymsgbytes(buf, CIPHERDATA_SEARCH_PTR(result), search_len);

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

	/* Validate input */
	if (!cipherdata_validate(cipherdata))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_BINARY_REPRESENTATION),
				 errmsg("invalid CipherDataType structure")));

	/* Initialize buffer */
	pq_begintypsend(&buf);

	/* Write header */
	pq_sendint32(&buf, cipherdata->version);
	pq_sendint32(&buf, CIPHERDATA_DEK_DIST_OID(cipherdata));
	pq_sendint32(&buf, CIPHERDATA_RND_LENGTH(cipherdata));
	pq_sendint32(&buf, CIPHERDATA_DET_LENGTH(cipherdata));
	pq_sendint32(&buf, CIPHERDATA_OPE_LENGTH(cipherdata));
	pq_sendint32(&buf, CIPHERDATA_SEARCH_LENGTH(cipherdata));

	/* Write ciphertexts */
	if (CIPHERDATA_RND_LENGTH(cipherdata) > 0)
		pq_sendbytes(&buf, CIPHERDATA_RND_PTR(cipherdata),
					 CIPHERDATA_RND_LENGTH(cipherdata));
	if (CIPHERDATA_DET_LENGTH(cipherdata) > 0)
		pq_sendbytes(&buf, CIPHERDATA_DET_PTR(cipherdata),
					 CIPHERDATA_DET_LENGTH(cipherdata));
	if (CIPHERDATA_OPE_LENGTH(cipherdata) > 0)
		pq_sendbytes(&buf, CIPHERDATA_OPE_PTR(cipherdata),
					 CIPHERDATA_OPE_LENGTH(cipherdata));
	if (CIPHERDATA_SEARCH_LENGTH(cipherdata) > 0)
		pq_sendbytes(&buf, CIPHERDATA_SEARCH_PTR(cipherdata),
					 CIPHERDATA_SEARCH_LENGTH(cipherdata));

	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
	if (need_free)
		pfree(cipherdata);
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