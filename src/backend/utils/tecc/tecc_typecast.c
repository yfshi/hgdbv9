/*-------------------------------------------------------------------------
 *
 * tecc_typecast.c
 *	  Type casting functions for TECC encrypted types
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/backend/utils/tecc/tecc_typecast.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "utils/tecc/tecc_typecast.h"
#include "utils/cipherdatatype.h"
#include "utils/tecc/tecc_type_mapping.h"
#include "catalog/pg_type_enc.h"
#include "catalog/pg_type.h"
#include "utils/builtins.h"

/* Begin - ReqID:TECC4.0 */

/*
 * is_type_conversion_allowed - Check if conversion between types is allowed
 *
 * Returns true if the conversion from source_type to target_type is allowed
 * according to the design document.
 */
bool
is_type_conversion_allowed(Oid source_type, Oid target_type)
{
	/* Same type conversion is always allowed */
	if (source_type == target_type)
		return true;

	/* Check int2_enc conversions */
	if (source_type == INT2_ENCOID)
	{
		return (target_type == INT4_ENCOID ||
				target_type == INT8_ENCOID ||
				target_type == FLOAT4_ENCOID ||
				target_type == FLOAT8_ENCOID ||
				target_type == NUMERIC_ENCOID);
	}

	/* Check int4_enc conversions */
	if (source_type == INT4_ENCOID)
	{
		return (target_type == INT8_ENCOID ||
				target_type == FLOAT8_ENCOID ||
				target_type == NUMERIC_ENCOID);
	}

	/* Check int8_enc conversions */
	if (source_type == INT8_ENCOID)
	{
		return (target_type == NUMERIC_ENCOID);
	}

	/* Check float4_enc conversions */
	if (source_type == FLOAT4_ENCOID)
	{
		return (target_type == FLOAT8_ENCOID ||
				target_type == NUMERIC_ENCOID);
	}

	/* Check float8_enc conversions */
	if (source_type == FLOAT8_ENCOID)
	{
		return (target_type == NUMERIC_ENCOID);
	}

	/* Check string type conversions */
	if (source_type == TEXT_ENCOID)
	{
		return (target_type == VARCHAR_ENCOID ||
				target_type == BPCHAR_ENCOID);
	}

	if (source_type == VARCHAR_ENCOID)
	{
		return (target_type == TEXT_ENCOID ||
				target_type == BPCHAR_ENCOID);
	}

	if (source_type == BPCHAR_ENCOID)
	{
		return (target_type == TEXT_ENCOID ||
				target_type == VARCHAR_ENCOID);
	}

	/* All other conversions are not allowed */
	return false;
}

/*
 * check_dek_consistency_for_conversion - Check source DEK identity for conversion
 *
 * Validates that the source CipherDataType has:
 *  1. A valid structure (version, field lengths)
 *  2. A valid dek_dist_oid (not InvalidOid)
 *
 * Note: Cross-DEK comparison (source vs target column) cannot be done at
 * cast function level because the cast function only receives the source
 * Datum. Target column DEK verification must be performed at the query
 * planning/analysis layer where column metadata is available.
 */
bool
check_dek_consistency_for_conversion(CipherDataType *source)
{
	/* Validate source structure */
	if (!cipherdata_validate(source))
	{
		ereport(ERROR,
				(errcode(ERRCODE_TECC_CIPHERDATA_INVALID),
				 errmsg("invalid cipherdata structure for type conversion")));
		return false;
	}

	/* Validate source DEK identity */
	if (!OidIsValid(CIPHERDATA_DEK_DIST_OID(source)))
	{
		ereport(ERROR,
				(errcode(ERRCODE_TECC_DEK_MISMATCH),
				 errmsg("cannot convert encrypted value without DEK identity"),
				 errdetail("The source cipherdata has no valid DEK distributed ID.")));
		return false;
	}

	return true;
}

/*
 * Generic type conversion function
 *
 * This is the common logic for all type conversion functions.
 * It validates the conversion and copies the CipherDataType structure.
 */
static Datum
tecc_type_convert(PG_FUNCTION_ARGS, Oid source_type, Oid target_type)
{
	CipherDataType *source;
	CipherDataType *result;

	/* Get source cipherdata — detoast first to handle TOASTed values
	 * (PG_GETARG_POINTER may return a TOAST pointer, not the actual data) */
	source = (CipherDataType *) PG_DETOAST_DATUM(PG_GETARG_DATUM(0));

	/* Validate source (use Datum-safe check for 1-byte varlena headers) */
	if (!PointerIsValid(source) ||
	    !CIPHERDATA_VERSION_VALID(CIPHERDATA_DATUM_VERSION(source)))
		ereport(ERROR,
				(errcode(ERRCODE_TECC_CIPHERDATA_INVALID),
				 errmsg("invalid cipherdata pointer")));

	/* Check if conversion is allowed */
	if (!is_type_conversion_allowed(source_type, target_type))
		ereport(ERROR,
				(errcode(ERRCODE_CANNOT_COERCE),
				 errmsg("cannot cast type %s to %s",
						tecc_type_name(source_type),
						tecc_type_name(target_type))));

	/* Check source DEK consistency and structure */
	check_dek_consistency_for_conversion(source);

	/* Copy the cipherdata structure (includes dek_dist_oid) */
	result = cipherdata_copy(source);
	if (result == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_OUT_OF_MEMORY),
				 errmsg("out of memory")));


	/* ReqID:TECC4.0 - Normalize text trailing-space format on cross-type cast.
	 * PG native: bpchar(20) 'hello'::varchar → 'hello' (strips padding).
	 * The DET/OPE sections use [format_byte][core][space_count:4], so set
	 * format_byte=0x00 and space_count=0 when casting away from BPCHAR. */
	if (source_type == BPCHAR_ENCOID &&
	    (target_type == VARCHAR_ENCOID || target_type == TEXT_ENCOID))
	{
		if (CIPHERDATA_DET_LENGTH(result) >= 5)
		{
			unsigned char *det_ptr = CIPHERDATA_DET_PTR(result);
			int32 det_len = CIPHERDATA_DET_LENGTH(result);
			det_ptr[0] = 0x00;
			det_ptr[det_len - 4] = 0;
			det_ptr[det_len - 3] = 0;
			det_ptr[det_len - 2] = 0;
			det_ptr[det_len - 1] = 0;
		}
		if (CIPHERDATA_OPE_LENGTH(result) >= 5)
		{
			unsigned char *ope_ptr = CIPHERDATA_OPE_PTR(result);
			int32 ope_len = CIPHERDATA_OPE_LENGTH(result);
			ope_ptr[0] = 0x00;
			ope_ptr[ope_len - 4] = 0;
			ope_ptr[ope_len - 3] = 0;
			ope_ptr[ope_len - 2] = 0;
			ope_ptr[ope_len - 1] = 0;
		}
	}
	PG_RETURN_POINTER(result);
}

/*
 * int2_enc conversion functions
 */

PG_FUNCTION_INFO_V1(int2_enc_to_int4_enc);
Datum
int2_enc_to_int4_enc(PG_FUNCTION_ARGS)
{
	return tecc_type_convert(fcinfo, INT2_ENCOID, INT4_ENCOID);
}

PG_FUNCTION_INFO_V1(int2_enc_to_int8_enc);
Datum
int2_enc_to_int8_enc(PG_FUNCTION_ARGS)
{
	return tecc_type_convert(fcinfo, INT2_ENCOID, INT8_ENCOID);
}

PG_FUNCTION_INFO_V1(int2_enc_to_float4_enc);
Datum
int2_enc_to_float4_enc(PG_FUNCTION_ARGS)
{
	return tecc_type_convert(fcinfo, INT2_ENCOID, FLOAT4_ENCOID);
}

PG_FUNCTION_INFO_V1(int2_enc_to_float8_enc);
Datum
int2_enc_to_float8_enc(PG_FUNCTION_ARGS)
{
	return tecc_type_convert(fcinfo, INT2_ENCOID, FLOAT8_ENCOID);
}

PG_FUNCTION_INFO_V1(int2_enc_to_numeric_enc);
Datum
int2_enc_to_numeric_enc(PG_FUNCTION_ARGS)
{
	return tecc_type_convert(fcinfo, INT2_ENCOID, NUMERIC_ENCOID);
}

/*
 * int4_enc conversion functions
 */

PG_FUNCTION_INFO_V1(int4_enc_to_int8_enc);
Datum
int4_enc_to_int8_enc(PG_FUNCTION_ARGS)
{
	return tecc_type_convert(fcinfo, INT4_ENCOID, INT8_ENCOID);
}

PG_FUNCTION_INFO_V1(int4_enc_to_float4_enc);
Datum
int4_enc_to_float4_enc(PG_FUNCTION_ARGS)
{
	return tecc_type_convert(fcinfo, INT4_ENCOID, FLOAT4_ENCOID);
}

PG_FUNCTION_INFO_V1(int4_enc_to_float8_enc);
Datum
int4_enc_to_float8_enc(PG_FUNCTION_ARGS)
{
	return tecc_type_convert(fcinfo, INT4_ENCOID, FLOAT8_ENCOID);
}

PG_FUNCTION_INFO_V1(int4_enc_to_numeric_enc);
Datum
int4_enc_to_numeric_enc(PG_FUNCTION_ARGS)
{
	return tecc_type_convert(fcinfo, INT4_ENCOID, NUMERIC_ENCOID);
}

/*
 * int8_enc conversion functions
 */

PG_FUNCTION_INFO_V1(int8_enc_to_numeric_enc);
Datum
int8_enc_to_numeric_enc(PG_FUNCTION_ARGS)
{
	return tecc_type_convert(fcinfo, INT8_ENCOID, NUMERIC_ENCOID);
}

/*
 * float4_enc conversion functions
 */

PG_FUNCTION_INFO_V1(float4_enc_to_float8_enc);
Datum
float4_enc_to_float8_enc(PG_FUNCTION_ARGS)
{
	return tecc_type_convert(fcinfo, FLOAT4_ENCOID, FLOAT8_ENCOID);
}

PG_FUNCTION_INFO_V1(float4_enc_to_numeric_enc);
Datum
float4_enc_to_numeric_enc(PG_FUNCTION_ARGS)
{
	return tecc_type_convert(fcinfo, FLOAT4_ENCOID, NUMERIC_ENCOID);
}

/*
 * float8_enc conversion functions
 */

PG_FUNCTION_INFO_V1(float8_enc_to_numeric_enc);
Datum
float8_enc_to_numeric_enc(PG_FUNCTION_ARGS)
{
	return tecc_type_convert(fcinfo, FLOAT8_ENCOID, NUMERIC_ENCOID);
}

/*
 * string type conversion functions
 */

PG_FUNCTION_INFO_V1(text_enc_to_varchar_enc);
Datum
text_enc_to_varchar_enc(PG_FUNCTION_ARGS)
{
	return tecc_type_convert(fcinfo, TEXT_ENCOID, VARCHAR_ENCOID);
}

PG_FUNCTION_INFO_V1(text_enc_to_bpchar_enc);
Datum
text_enc_to_bpchar_enc(PG_FUNCTION_ARGS)
{
	return tecc_type_convert(fcinfo, TEXT_ENCOID, BPCHAR_ENCOID);
}

PG_FUNCTION_INFO_V1(varchar_enc_to_text_enc);
Datum
varchar_enc_to_text_enc(PG_FUNCTION_ARGS)
{
	return tecc_type_convert(fcinfo, VARCHAR_ENCOID, TEXT_ENCOID);
}

PG_FUNCTION_INFO_V1(varchar_enc_to_bpchar_enc);
Datum
varchar_enc_to_bpchar_enc(PG_FUNCTION_ARGS)
{
	return tecc_type_convert(fcinfo, VARCHAR_ENCOID, BPCHAR_ENCOID);
}

PG_FUNCTION_INFO_V1(bpchar_enc_to_text_enc);
Datum
bpchar_enc_to_text_enc(PG_FUNCTION_ARGS)
{
	return tecc_type_convert(fcinfo, BPCHAR_ENCOID, TEXT_ENCOID);
}

PG_FUNCTION_INFO_V1(bpchar_enc_to_varchar_enc);
Datum
bpchar_enc_to_varchar_enc(PG_FUNCTION_ARGS)
{
	return tecc_type_convert(fcinfo, BPCHAR_ENCOID, VARCHAR_ENCOID);
}

/* End - ReqID:TECC4.0 */