/*---------------------------------------------------------------------------------
 *
 * File:utl_raw.c
 *
 * Abstract:
 *		Internal implementation of UTL_RAW package
 *
 * ReqID: SRS-PLSQL-UTL_RAW
 *
 * Copyright:
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  contrib/ivorysql_ora/src/builtin_packages/utl_raw/utl_raw.c
 *---------------------------------------------------------------------------------
 */

/* Begin - ReqID:SRS-PLSQL-UTL_RAW */
#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include "utils/numeric.h"
#include "catalog/pg_type.h"
#include "port/pg_bswap.h"
#include "miscadmin.h"
#include <ctype.h>
#include "common/hex.h"
#include "mb/pg_wchar.h"

#define MAX_RAW_LEN 32767
#define MAP_KEEP    -1
#define MAP_DELETE  256
#define MAP_ARRAY_SIZE 256
#define START_RANGE_BYTE 0x00
#define END_RANGE_BYTE 0xFF

static void
check_raw_length(bytea *raw)
{
	int current_raw_len;
	if (raw == NULL)
	{
		return;
	}

	current_raw_len = VARSIZE_ANY_EXHDR(raw);
	if (current_raw_len > MAX_RAW_LEN)
	{
		ereport(ERROR,
			(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("RAW length %d exceeds maximum allowed %d",
			current_raw_len, MAX_RAW_LEN)));
	}
}

static void
utl_raw_swap_raw_data(bytea* raw)
{
	unsigned char *result_data;
	int         result_length;
	result_data = (unsigned char*)VARDATA_ANY(raw);
	result_length = VARSIZE_ANY_EXHDR(raw);
	for (int i = 0; i < result_length / 2; i++)
	{
		unsigned char tmp = result_data[i];
		result_data[i] = result_data[result_length - 1 - i];
		result_data[result_length - 1 - i] = tmp;
	}
}

#define OP_AND         1
#define OP_COMPLEMENT  2
#define OP_OR          3
#define OP_XOR         4
#define BINARY_DOUBLE_GATE_SIZE  8
#define BINARY_FLOAT_GATE_SIZE   4
#define BINARY_INTEGER_GATE_SIZE 4

static bytea*
bit_operate_internal(bytea *src_bytea1, bytea* src_bytea2, int op)
{
	int     bytea1_length;
	int     bytea2_length;
	int     operate_length;
	int     result_length;
	bytea  *result;
	unsigned char *bytea1_data;
	unsigned char *bytea2_data;
	unsigned char *result_data;

	check_raw_length(src_bytea1);
	check_raw_length(src_bytea2);

	// check && execute complement
	if (op == OP_COMPLEMENT)
	{
		result_length = src_bytea1 == NULL ? 0 : VARSIZE_ANY_EXHDR(src_bytea1);
		if (result_length == 0)
		{
			return NULL;
		}
		result = (bytea *) palloc(result_length + VARHDRSZ);
		SET_VARSIZE(result, result_length + VARHDRSZ);
		bytea1_data = (unsigned char*)VARDATA_ANY(src_bytea1);
		result_data = (unsigned char*)VARDATA_ANY(result);
		for (int i = 0; i < result_length; i++)
			result_data[i] = ~bytea1_data[i];
		return result;
	}

	bytea1_length = src_bytea1 == NULL ? 0 : VARSIZE_ANY_EXHDR(src_bytea1);
	bytea2_length = src_bytea2 == NULL ? 0 : VARSIZE_ANY_EXHDR(src_bytea2);
	if (bytea1_length == 0 || bytea2_length == 0)
	{
		// param1 is '' or param2 is '', and lenght of another param is less than 32K
		return NULL;
	}

	result_length = (bytea1_length >= bytea2_length) ? bytea1_length : bytea2_length;
	operate_length = (bytea1_length <= bytea2_length) ? bytea1_length : bytea2_length;
	result = (bytea *) palloc(result_length + VARHDRSZ);
	SET_VARSIZE(result, result_length + VARHDRSZ);
	bytea1_data = (unsigned char*)VARDATA_ANY(src_bytea1);
	bytea2_data = (unsigned char*)VARDATA_ANY(src_bytea2);
	result_data = (unsigned char*)VARDATA_ANY(result);

	switch (op)
	{
		case OP_AND:
			for (int i = 0; i < operate_length; i++)
				result_data[i] = bytea1_data[i] & bytea2_data[i];
			break;
		case OP_OR:
			for (int i = 0; i < operate_length; i++)
				result_data[i] = bytea1_data[i] | bytea2_data[i];
			break;
		case OP_XOR:
			for (int i = 0; i < operate_length; i++)
				result_data[i] = bytea1_data[i] ^ bytea2_data[i];
			break;
		default:
			elog(ERROR, "invalid bit operation code");
	}

	// if bytea1_length == bytea2_length, no need to memcpy from src_data
	if (bytea1_length > bytea2_length)
		memcpy(result_data + operate_length, bytea1_data + operate_length,
			bytea1_length - bytea2_length);
	else if (bytea2_length > bytea1_length)
		memcpy(result_data + operate_length, bytea2_data + operate_length,
			bytea2_length - bytea1_length);

	return result;
}

// datum function
PG_FUNCTION_INFO_V1(utl_raw_bit_and);
Datum
utl_raw_bit_and(PG_FUNCTION_ARGS)
{
	bytea* src_bytea1;
	bytea* src_bytea2;
	bytea* result;
	src_bytea1 = PG_ARGISNULL(0) ? NULL : PG_GETARG_BYTEA_PP(0);
	src_bytea2 = PG_ARGISNULL(1) ? NULL : PG_GETARG_BYTEA_PP(1);
	result = bit_operate_internal(src_bytea1, src_bytea2, OP_AND);
	if (result == NULL)
	{
		PG_RETURN_NULL();
	}

	PG_RETURN_BYTEA_P(result);
}

PG_FUNCTION_INFO_V1(utl_raw_bit_complement);
Datum
utl_raw_bit_complement(PG_FUNCTION_ARGS)
{
	bytea* src_bytea;
	bytea* result;
	src_bytea = PG_ARGISNULL(0) ? NULL : PG_GETARG_BYTEA_PP(0);
	result = bit_operate_internal(src_bytea, NULL, OP_COMPLEMENT);
	if (result == NULL)
	{
		PG_RETURN_NULL();
	}

	PG_RETURN_BYTEA_P(result);
}

PG_FUNCTION_INFO_V1(utl_raw_bit_or);
Datum
utl_raw_bit_or(PG_FUNCTION_ARGS)
{
	bytea* src_bytea1;
	bytea* src_bytea2;
	bytea* result;
	src_bytea1 = PG_ARGISNULL(0) ? NULL : PG_GETARG_BYTEA_PP(0);
	src_bytea2 = PG_ARGISNULL(1) ? NULL : PG_GETARG_BYTEA_PP(1);
	result = bit_operate_internal(src_bytea1, src_bytea2, OP_OR);
	if (result == NULL)
	{
		PG_RETURN_NULL();
	}

	PG_RETURN_BYTEA_P(result);
}

PG_FUNCTION_INFO_V1(utl_raw_bit_xor);
Datum
utl_raw_bit_xor(PG_FUNCTION_ARGS)
{
	bytea* src_bytea1;
	bytea* src_bytea2;
	bytea* result;
	src_bytea1 = PG_ARGISNULL(0) ? NULL : PG_GETARG_BYTEA_PP(0);
	src_bytea2 = PG_ARGISNULL(1) ? NULL : PG_GETARG_BYTEA_PP(1);
	result = bit_operate_internal(src_bytea1, src_bytea2, OP_XOR);
	if (result == NULL)
	{
		PG_RETURN_NULL();
	}

	PG_RETURN_BYTEA_P(result);
}

PG_FUNCTION_INFO_V1(utl_raw_cast_from_binary_double);
Datum
utl_raw_cast_from_binary_double(PG_FUNCTION_ARGS)
{
	float8      src_data;
	int         endianess;
	bytea      *result;
	bool        need_swap;
	bool        is_machine_big_endian;
	src_data = PG_GETARG_FLOAT8(0);
	endianess = PG_GETARG_INT32(1);
	result = DatumGetByteaP(DirectFunctionCall1(float8send, Float8GetDatum(src_data)));
#ifdef WORDS_BIGENDIAN
	is_machine_big_endian = true;
#else
	is_machine_big_endian = false;
#endif
	need_swap = false;
	switch (endianess)
	{
		case 1: /* big_endian */
			break;
	    case 2: /* little_endian */
			need_swap = true;
			break;
		case 3: /* machine_endian */
			need_swap = is_machine_big_endian ? false : true;
			break;
		default:
			ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("invalid endianess value: %d", endianess)));
	}

	if (need_swap)
	{
		utl_raw_swap_raw_data(result);
	}

	PG_RETURN_BYTEA_P(result);
}

PG_FUNCTION_INFO_V1(utl_raw_cast_from_binary_float);
Datum
utl_raw_cast_from_binary_float(PG_FUNCTION_ARGS)
{
	float4      src_data;
	int         endianess;
	bytea      *result;
	bool        need_swap;
	bool        is_machine_big_endian;
	src_data = PG_GETARG_FLOAT4(0);
	endianess = PG_GETARG_INT32(1);
	result = DatumGetByteaP(DirectFunctionCall1(float4send, Float4GetDatum(src_data)));
#ifdef WORDS_BIGENDIAN
	is_machine_big_endian = true;
#else
	is_machine_big_endian = false;
#endif
	need_swap = false;
	switch (endianess)
	{
		case 1: /* big_endian */
			break;
	    case 2: /* little_endian */
			need_swap = true;
			break;
		case 3: /* machine_endian */
			need_swap = is_machine_big_endian ? false : true;
			break;
		default:
			ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("invalid endianess value: %d", endianess)));
	}

	if (need_swap)
	{
		utl_raw_swap_raw_data(result);
	}

	PG_RETURN_BYTEA_P(result);
}

PG_FUNCTION_INFO_V1(utl_raw_cast_from_binary_integer);
Datum
utl_raw_cast_from_binary_integer(PG_FUNCTION_ARGS)
{
	int         src_data;
	int         endianess;
	bytea      *result;
	bool        need_swap;
	bool        is_machine_big_endian;
	src_data = PG_GETARG_INT32(0);
	endianess = PG_GETARG_INT32(1);
	result = DatumGetByteaP(DirectFunctionCall1(int4send, Int32GetDatum(src_data)));
#ifdef WORDS_BIGENDIAN
	is_machine_big_endian = true;
#else
	is_machine_big_endian = false;
#endif
	need_swap = false;
	switch (endianess)
	{
		case 1: /* big_endian */
			break;
		case 2: /* little_endian */
			need_swap = true;
			break;
		case 3: /* machine_endian */
			need_swap = is_machine_big_endian ? false : true;
			break;
		default:
			ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("invalid endianess value: %d", endianess)));
	}

	if (need_swap)
	{
		utl_raw_swap_raw_data(result);
	}

	PG_RETURN_BYTEA_P(result);
}

PG_FUNCTION_INFO_V1(utl_raw_cast_from_number);
Datum
utl_raw_cast_from_number(PG_FUNCTION_ARGS)
{
	Datum       src_data;
	bytea      *result;
	src_data = PG_GETARG_DATUM(0);
	result = DatumGetByteaP(DirectFunctionCall1(numeric_send, src_data));
	PG_RETURN_BYTEA_P(result);
}

PG_FUNCTION_INFO_V1(utl_raw_cast_to_binary_double);
Datum
utl_raw_cast_to_binary_double(PG_FUNCTION_ARGS)
{
	uint64      raw_data;
	bytea      *src_bytea;
	int         endianess;
	int         src_bytea_length;
	unsigned char *src_bytea_data;
	StringInfoData buf;
	float8      result;
	bool        need_swap;
	bool        is_machine_big_endian;

	src_bytea = PG_ARGISNULL(0) ? NULL : PG_GETARG_BYTEA_PP(0);
	check_raw_length(src_bytea);
	src_bytea_length = src_bytea == NULL ? 0 : VARSIZE_ANY_EXHDR(src_bytea);
	if (src_bytea_length == 0 || PG_ARGISNULL(1))
	{
		PG_RETURN_NULL();
	}
	if (src_bytea_length < BINARY_DOUBLE_GATE_SIZE)
	{
		ereport(ERROR,
			(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("RAW length %d is less than 8 bytes required", src_bytea_length)));
	}

#ifdef WORDS_BIGENDIAN
	is_machine_big_endian = true;
#else
	is_machine_big_endian = false;
#endif

	need_swap = false;
	endianess = PG_GETARG_INT32(1);
	switch (endianess)
	{
		case 1: /* big_endian */
			break;
	    case 2: /* little_endian */
			need_swap = true;
			break;
		case 3: /* machine_endian */
			need_swap = is_machine_big_endian ? false : true;
			break;
		default:
			ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("invalid endianess value: %d", endianess)));
	}
	// get upper 8 bytes and swap with endinaess.
	src_bytea_data = (unsigned char*)VARDATA_ANY(src_bytea);
	memcpy(&raw_data, src_bytea_data, BINARY_DOUBLE_GATE_SIZE);
	if (need_swap)
	{
		raw_data = pg_bswap64(raw_data);
	}

	// use StringInfo and binary_double_recv get result_data;
	initStringInfo(&buf);
	appendBinaryStringInfo(&buf, (char*)&raw_data, BINARY_DOUBLE_GATE_SIZE);
	result = DatumGetFloat8(DirectFunctionCall1(float8recv, PointerGetDatum(&buf)));
	pfree(buf.data);

	PG_RETURN_FLOAT8(result);
}

PG_FUNCTION_INFO_V1(utl_raw_cast_to_binary_float);
Datum
utl_raw_cast_to_binary_float(PG_FUNCTION_ARGS)
{
	uint32      raw_data;
	bytea      *src_bytea;
	int         endianess;
	int         src_bytea_length;
	unsigned char *src_bytea_data;
	StringInfoData buf;
	float4      result;
	bool        need_swap;
	bool        is_machine_big_endian;

	src_bytea = PG_ARGISNULL(0) ? NULL : PG_GETARG_BYTEA_PP(0);
	check_raw_length(src_bytea);
	src_bytea_length = src_bytea == NULL ? 0 : VARSIZE_ANY_EXHDR(src_bytea);
	if (src_bytea_length == 0 || PG_ARGISNULL(1))
	{
		PG_RETURN_NULL();
	}
	if (src_bytea_length < BINARY_FLOAT_GATE_SIZE)
	{
		ereport(ERROR,
			(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("RAW length %d is less than 4 bytes required", src_bytea_length)));
	}

#ifdef WORDS_BIGENDIAN
	is_machine_big_endian = true;
#else
	is_machine_big_endian = false;
#endif

	need_swap = false;
	endianess = PG_GETARG_INT32(1);
	switch (endianess)
	{
		case 1: /* big_endian */
			break;
	    case 2: /* little_endian */
			need_swap = true;
			break;
		case 3: /* machine_endian */
			need_swap = is_machine_big_endian ? false : true;
			break;
		default:
			ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("invalid endianess value: %d", endianess)));
	}
	// get upper 4 bytes and swap with endinaess.
	src_bytea_data = (unsigned char*)VARDATA_ANY(src_bytea);
	memcpy(&raw_data, src_bytea_data, BINARY_FLOAT_GATE_SIZE);
	if (need_swap)
	{
		raw_data = pg_bswap32(raw_data);
	}

	// use StringInfo and binary_float_recv get result_data;
	initStringInfo(&buf);
	appendBinaryStringInfo(&buf, (char*)&raw_data, BINARY_FLOAT_GATE_SIZE);
	result = DatumGetFloat4(DirectFunctionCall1(float4recv, PointerGetDatum(&buf)));
	pfree(buf.data);

	PG_RETURN_FLOAT4(result);
}

/*
 * utl_raw_cast_to_binary_integer
 *
 * cast raw to binary_integer - 
 * direct concatenation is enough, no need to use recv
 */
PG_FUNCTION_INFO_V1(utl_raw_cast_to_binary_integer);
Datum
utl_raw_cast_to_binary_integer(PG_FUNCTION_ARGS)
{
	bytea      *src_bytea;
	int         endianess;
	int         src_bytea_length;
	unsigned char *src_bytea_data;
	uint32      value;
	int32       result;
	bool        need_swap;
	bool        is_machine_big_endian;

	src_bytea = PG_ARGISNULL(0) ? NULL : PG_GETARG_BYTEA_PP(0);
	check_raw_length(src_bytea);
	src_bytea_length = src_bytea == NULL ? 0 : VARSIZE_ANY_EXHDR(src_bytea);
	if (src_bytea_length == 0 || PG_ARGISNULL(1))
	{
		PG_RETURN_NULL();
	}
	if (src_bytea_length > BINARY_INTEGER_GATE_SIZE)
	{
		ereport(ERROR,
			(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("RAW length %d is more than 4 bytes required", src_bytea_length)));
	}

#ifdef WORDS_BIGENDIAN
	is_machine_big_endian = true;
#else
	is_machine_big_endian = false;
#endif
	need_swap = false;
	endianess = PG_GETARG_INT32(1);
	switch (endianess)
	{
		case 1: /* big_endian */
			break;
	    case 2: /* little_endian */
			need_swap = true;
			break;
		case 3: /* machine_endian */
			need_swap = is_machine_big_endian ? false : true;
			break;
		default:
			ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("invalid endianess value: %d", endianess)));
	}

	src_bytea_data = (unsigned char*)VARDATA_ANY(src_bytea);
	value = 0;
	for (int i = 0; i < src_bytea_length; i++)
	{
		if (need_swap)
		{
			value |= (uint32) src_bytea_data[i] << (8 * i);
		}
		else
		{
			value = (value << 8) | src_bytea_data[i];
		}
	}

	result = (int32)value;
	PG_RETURN_INT32((int32)result);
}

PG_FUNCTION_INFO_V1(utl_raw_cast_to_number);
Datum
utl_raw_cast_to_number(PG_FUNCTION_ARGS)
{
	bytea *src_bytea;
	Datum result;
	StringInfoData buf;
	src_bytea = PG_GETARG_BYTEA_P(0);
	initStringInfo(&buf);
	appendBinaryStringInfo(&buf, (char*)VARDATA_ANY(src_bytea), VARSIZE_ANY_EXHDR(src_bytea));
	result = DirectFunctionCall4(numeric_recv, PointerGetDatum(&buf),
								ObjectIdGetDatum(NUMERICOID),Int32GetDatum(-1),
								Int32GetDatum(-1));
	pfree(buf.data);
	PG_RETURN_DATUM(result);
}

PG_FUNCTION_INFO_V1(utl_raw_convert);
Datum
utl_raw_convert(PG_FUNCTION_ARGS)
{
	// r RAW, to_charset VARCHAR2, from_charset VARCHAR2
	bytea       *src_bytea;
	text        *from_charset;
	text        *to_charset;
	char  *from_charset_name;
	char  *to_charset_name;
	Datum  from_charset_datum;
	Datum  to_charset_datum;
	bytea       *conv_bytea;
	int         from_charset_id;
	int         to_charset_id;
	unsigned char *conv_data;
	int         conv_len;
	int         final_len;
	bytea       *result;
	unsigned char* result_data;

	// check r, from_charset, to_charset cannot be null or ''
	if (PG_ARGISNULL(0) || PG_ARGISNULL(1) || PG_ARGISNULL(2))
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("convert_raw, from_charset and to_charset cannot be NULL")));
	}
	src_bytea = PG_GETARG_BYTEA_P(0);
	from_charset = PG_GETARG_TEXT_PP(1);
	to_charset = PG_GETARG_TEXT_PP(2);
	check_raw_length(src_bytea);
	if (VARSIZE_ANY_EXHDR(src_bytea) == 0 || VARSIZE_ANY_EXHDR(from_charset) == 0 ||
		VARSIZE_ANY_EXHDR(to_charset) == 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("convert_raw, from_charset and to_charset cannot be NULL")));
	}

	// use charset param to get charset name, charset oid and convert bytea
	from_charset_name = text_to_cstring(from_charset);
	to_charset_name = text_to_cstring(to_charset);
	from_charset_id = pg_char_to_encoding(from_charset_name);
	to_charset_id = pg_char_to_encoding(to_charset_name);
	if (from_charset_id < 0 || to_charset_id < 0) {
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("invalid encoding name: %s",
						from_charset_id < 0 ? from_charset_name : to_charset_name)));
	}

	to_charset_datum = DirectFunctionCall1(namein, CStringGetDatum(to_charset_name));
	from_charset_datum = DirectFunctionCall1(namein, CStringGetDatum(from_charset_name));
	conv_bytea = DatumGetByteaP(DirectFunctionCall3(pg_convert, PointerGetDatum(src_bytea), from_charset_datum, to_charset_datum));
	conv_len = VARSIZE_ANY_EXHDR(conv_bytea);
	if (conv_len <= MAX_RAW_LEN)
	{
		pfree(DatumGetPointer(from_charset_datum));
		pfree(DatumGetPointer(to_charset_datum));
		pfree(from_charset_name);
		pfree(to_charset_name);

		PG_RETURN_BYTEA_P(conv_bytea);
	}

	conv_data = (unsigned char *) VARDATA_ANY(conv_bytea);
	final_len = pg_encoding_mbcliplen(to_charset_id,
									(const char *) conv_data,
									conv_len,
									MAX_RAW_LEN);
	result = (bytea *) palloc(final_len + VARHDRSZ);
	SET_VARSIZE(result, final_len + VARHDRSZ);
	result_data = (unsigned char*)VARDATA_ANY(result);
	memcpy(result_data, conv_data, final_len);
	pfree(conv_bytea);
	pfree(DatumGetPointer(from_charset_datum));
	pfree(DatumGetPointer(to_charset_datum));
	pfree(from_charset_name);
	pfree(to_charset_name);

	PG_RETURN_BYTEA_P(result);
}

PG_FUNCTION_INFO_V1(utl_raw_overlay);
Datum
utl_raw_overlay(PG_FUNCTION_ARGS)
{
	// overlay_str raw, target raw, pos binary_integer default 1, len binary_integer null, pad raw default null
	bytea *overlay_str;
	bytea *target;
	int32 pos;
	int32 len;
	bytea *pad;
	int overlay_len;
	int target_len;
	int pad_len;
	int final_len;
	bytea* result;
	unsigned char pad_byte;
	unsigned char* result_data;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("overlay_str and target cannot be NULL")));
	}
	overlay_str = PG_GETARG_BYTEA_PP(0);
	target = PG_GETARG_BYTEA_PP(1);
	overlay_len = VARSIZE_ANY_EXHDR(overlay_str);
	target_len = VARSIZE_ANY_EXHDR(target);
	if (overlay_len > MAX_RAW_LEN || overlay_len == 0 ||
		target_len > MAX_RAW_LEN || target_len == 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("invalid overlay_str or target")));
	}

	// check pos
	if (PG_ARGISNULL(2))
	{
		pos = 1;
	}
	else
	{
		pos = PG_GETARG_INT32(2);
		if (pos < 1 || pos > MAX_RAW_LEN)
		{
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("pos must be between 0 and MAX_RAW_LEN")));
		}
	}

	// check len
	if (PG_ARGISNULL(3))
	{
		len = overlay_len;
	}
	else
	{
		len = PG_GETARG_INT32(3);
		if (len < 0 || len > MAX_RAW_LEN)
		{
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("len must be between 0 and MAX_RAW_LEN")));
		}
	}
	// pos + len
	if (pos + len - 1 > MAX_RAW_LEN)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("pos + len - 1 exceeds maximun raw length")));
	}

	// pad raw
	if (PG_ARGISNULL(4))
	{
		pad_byte = 0x00;
	}
	else
	{
		pad = PG_GETARG_BYTEA_PP(4);
		pad_len = VARSIZE_ANY_EXHDR(pad);
		if (pad_len == 0)
		{
			pad_byte = 0x00;
		}
		else
		{
			// get the first byte
			pad_byte = ((unsigned char*) VARDATA_ANY(pad))[0];
		}
	}

	// get final len to set result
	final_len = target_len < pos + len - 1 ? pos + len - 1 : target_len;
	result = (bytea *) palloc(final_len + VARHDRSZ);
	SET_VARSIZE(result, final_len + VARHDRSZ);
	result_data = (unsigned char*) VARDATA_ANY(result);

	memcpy(result_data, (unsigned char*) VARDATA_ANY(target), target_len);
	memset(result_data + target_len, pad_byte, final_len - target_len);

	if (len > 0)
	{
		memset(result_data + pos - 1, pad_byte, len);
		memcpy(result_data + pos - 1, (unsigned char*)VARDATA_ANY(overlay_str),
			len > overlay_len ? overlay_len : len);
	}
	PG_RETURN_BYTEA_P(result);
}

PG_FUNCTION_INFO_V1(utl_raw_reverse);
Datum
utl_raw_reverse(PG_FUNCTION_ARGS)
{
	bytea       *src_bytea;
	bytea       *result;
	int         src_bytea_length;
	unsigned char *src_bytea_data;
	unsigned char *result_data;

	if (PG_ARGISNULL(0))
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("numeric or value error")));
	}
	src_bytea = PG_GETARG_BYTEA_PP(0);
	check_raw_length(src_bytea);
	src_bytea_length = VARSIZE_ANY_EXHDR(src_bytea);
	if (src_bytea_length == 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("numeric or value error")));
	}

	src_bytea_data = (unsigned char *)VARDATA_ANY(src_bytea);
	result = (bytea *) palloc(src_bytea_length + VARHDRSZ);
	SET_VARSIZE(result, src_bytea_length + VARHDRSZ);
	result_data = (unsigned char*)VARDATA_ANY(result);
	for (int i = 0; i < src_bytea_length; i++)
	{
		result_data[i] = src_bytea_data[src_bytea_length - 1 - i];
	}

	PG_RETURN_BYTEA_P(result);
}

PG_FUNCTION_INFO_V1(utl_raw_translate);
Datum
utl_raw_translate(PG_FUNCTION_ARGS)
{
	bytea *src_byte;
	bytea *from_set;
	bytea *to_set;
	int src_len;
	int from_len;
	int to_len;
	unsigned char *src_data;
	unsigned char *from_data;
	unsigned char *to_data;
	int translate_array[MAP_ARRAY_SIZE];
	bytea *result;
	unsigned char *result_data;
	int out_len;
	int out_idx;
	int count_delete_bytea;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1) || PG_ARGISNULL(2))
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("translate raw, from_set or to_set cannot be null")));
	}

	src_byte = PG_GETARG_BYTEA_PP(0);
	from_set = PG_GETARG_BYTEA_PP(1);
	to_set = PG_GETARG_BYTEA_PP(2);
	src_len = VARSIZE_ANY_EXHDR(src_byte);
	from_len = VARSIZE_ANY_EXHDR(from_set);
	to_len = VARSIZE_ANY_EXHDR(to_set);
	if (src_len == 0 || from_len == 0 || to_len == 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("translate raw, from_set or to_set cannot be null")));
	}
	check_raw_length(src_byte);
	check_raw_length(from_set);
	check_raw_length(to_set);

	src_data = (unsigned char*) VARDATA_ANY(src_byte);
	from_data = (unsigned char*) VARDATA_ANY(from_set);
	to_data = (unsigned char*) VARDATA_ANY(to_set);

	for (int i = 0; i < MAP_ARRAY_SIZE; i++)
	{
		translate_array[i] = MAP_KEEP;
	}

	count_delete_bytea = 0;
	for (int i = 0; i < from_len; i++)
	{
		unsigned char current_ch = from_data[i];
		if (translate_array[current_ch] != MAP_KEEP)
		{
			// use the first translate rule
			continue;
		}
		if (i < to_len)
		{
			// enough long to translate
			translate_array[current_ch] = to_data[i];
		}
		else
		{
			// from_set too long, remove that byte
			translate_array[current_ch] = MAP_DELETE;
			count_delete_bytea++;
		}
	}

	out_len = src_len - count_delete_bytea;
	out_idx = 0;
	result = (bytea *) palloc(out_len + VARHDRSZ);
	SET_VARSIZE(result, out_len + VARHDRSZ);
	result_data = (unsigned char*) VARDATA_ANY(result);
	for (int i = 0; i < src_len; i++)
	{
		unsigned char current_ch = src_data[i];
		if (translate_array[current_ch] == MAP_KEEP)
		{
			result_data[out_idx] = current_ch;
			out_idx++;
		}
		else if (translate_array[current_ch] == MAP_DELETE)
		{
			continue;
		}
		else
		{
			result_data[out_idx] = (unsigned char)translate_array[current_ch];
			out_idx++;
		}
	}
	PG_RETURN_BYTEA_P(result);
}

PG_FUNCTION_INFO_V1(utl_raw_transliterate);
Datum
utl_raw_transliterate(PG_FUNCTION_ARGS)
{
	bytea *src_byte;
	bytea *from_set;
	bytea *to_set;
	bytea *pad;
	int src_len;
	int from_len;
	int to_len;
	unsigned char *src_data;
	unsigned char *from_data;
	unsigned char *to_data;
	unsigned char pad_byte;
	int translate_array[MAP_ARRAY_SIZE];
	bytea *result;
	unsigned char *result_data;

	// get src byte len and data.
	if (PG_ARGISNULL(0))
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("translate raw, from_set or to_set cannot be null")));
	}
	src_byte = PG_GETARG_BYTEA_PP(0);
	src_len = VARSIZE_ANY_EXHDR(src_byte);
	if (src_len == 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("translate raw, from_set or to_set cannot be null")));
	}
	check_raw_length(src_byte);
	src_data = (unsigned char*)VARDATA_ANY(src_byte);

	// get from set len and data
	if (PG_ARGISNULL(1))
	{
		from_set = NULL;
		from_len = 0;
	}
	else
	{
		from_set = PG_GETARG_BYTEA_PP(1);
		from_len = VARSIZE_ANY_EXHDR(from_set);
		check_raw_length(from_set);
	}

	// get to set len and data
	if (PG_ARGISNULL(2))
	{
		to_set = NULL;
		to_len = 0;
	}
	else
	{
		to_set = PG_GETARG_BYTEA_PP(2);
		to_len = VARSIZE_ANY_EXHDR(to_set);
		check_raw_length(to_set);
	}

	// get pad len and data to set pad_byte
	if (PG_ARGISNULL(3))
	{
		pad_byte = 0x00;
	}
	else
	{
		pad = PG_GETARG_BYTEA_PP(3);
		check_raw_length(pad);
		pad_byte = ((unsigned char*)VARDATA_ANY(pad))[0];
	}

	// initial translate_array
	for (int i = 0; i < MAP_ARRAY_SIZE; i++)
	{
		translate_array[i] = MAP_KEEP;
	}
	if (from_len == 0 && to_len == 0)
	{
		// from_set and to_set are null or '', use pad_byte translate all bytes.
		result = (bytea*) palloc(src_len + VARHDRSZ);
		SET_VARSIZE(result, src_len + VARHDRSZ);
		result_data = (unsigned char*)VARDATA_ANY(result);
		for (int i = 0; i < src_len; i++)
		{
			result_data[i] = pad_byte;
		}
		PG_RETURN_BYTEA_P(result);
	}
	else if (from_len == 0 && to_len != 0)
	{
		// it equals to from_len == to_len, and from_set is subsequence of 00..FF
		for (int i = 0; i < MAP_ARRAY_SIZE; i++)
		{
			translate_array[i] = pad_byte;
		}
		to_data = (unsigned char*)VARDATA_ANY(to_set);
		for (int i = 0; i < to_len && i < sizeof(translate_array) / sizeof(translate_array[0]); i++)
		{
			translate_array[i] = to_data[i];
		}
	}
	else if (from_len != 0 && to_len == 0)
	{
		// if to_set is NULL, every byte in from_set translate to pad
		from_data = (unsigned char*)VARDATA_ANY(from_set);
		for (int i = 0; i < from_len; i++)
		{
			unsigned char modify_byte = from_data[i];
			translate_array[modify_byte] = pad_byte;
		}
	}
	else
	{
		// if to_set not NULL, every byte in from_set translate by to_set,
		// too long part in from_set translate to pad
		from_data = (unsigned char*)VARDATA_ANY(from_set);
		to_data = (unsigned char*)VARDATA_ANY(to_set);
		for (int i = 0; i < from_len; i++)
		{
			unsigned char modify_byte = from_data[i];
			if (translate_array[modify_byte] != MAP_KEEP)
			{
				// use the first byte translate
				continue;
			}
			if (i < to_len)
			{
				translate_array[modify_byte] = to_data[i];
			}
			else
			{
				translate_array[modify_byte] = pad_byte;
			}
		}
	}

	result = (bytea*) palloc(src_len + VARHDRSZ);
	SET_VARSIZE(result, src_len + VARHDRSZ);
	result_data = (unsigned char*)VARDATA_ANY(result);

	for (int i = 0; i < src_len; i++)
	{
		unsigned char translate_byte = src_data[i];
		if (translate_array[translate_byte] != MAP_KEEP)
		{
			result_data[i] = translate_array[translate_byte];
		}
		else
		{
			result_data[i] = translate_byte;
		}
	}

	PG_RETURN_BYTEA_P(result);
}

PG_FUNCTION_INFO_V1(utl_raw_xrange);
Datum
utl_raw_xrange(PG_FUNCTION_ARGS)
{
	// start_byte, end_byte
	bytea *start_bytea;
	bytea *end_bytea;
	unsigned char start_byte;
	unsigned char end_byte;
	int start_bytea_len;
	int end_bytea_len;
	int result_len;
	bytea *result;
	unsigned char *result_data;
	bool is_start_greater_than_end;

	if (PG_ARGISNULL(0))
	{
		// start with x'00'
		start_bytea = NULL;
		start_byte = 0x00;
	}
	else
	{
		start_bytea = PG_GETARG_BYTEA_PP(0);
		check_raw_length(start_bytea);
		start_bytea_len = VARSIZE_ANY_EXHDR(start_bytea);
		if (start_bytea_len == 0)
		{
			start_byte = 0x00;
		}
		else
		{
			start_byte = ((unsigned char*)VARDATA_ANY(start_bytea))[0];
		}
	}

	if (PG_ARGISNULL(1))
	{
		// end with x'FF'
		end_bytea = NULL;
		end_byte = 0xFF;
	}
	else
	{
		end_bytea = PG_GETARG_BYTEA_PP(1);
		check_raw_length(end_bytea);
		end_bytea_len = VARSIZE_ANY_EXHDR(end_bytea);
		if (end_bytea_len == 0)
		{
			end_byte = 0xFF;
		}
		else
		{
			end_byte = ((unsigned char*) VARDATA_ANY(end_bytea))[0];
		}
	}
	if (start_byte <= end_byte)
	{
		is_start_greater_than_end = false;
		result_len = end_byte - start_byte + 1;
	}
	else
	{
		is_start_greater_than_end = true;
		result_len = (END_RANGE_BYTE - start_byte + 1) + (end_byte - START_RANGE_BYTE + 1);
	}
	result = (bytea*) palloc(result_len + VARHDRSZ);
	SET_VARSIZE(result, result_len + VARHDRSZ);
	result_data = (unsigned char*)VARDATA_ANY(result);
	if (is_start_greater_than_end)
	{
		for (int i = 0; i < END_RANGE_BYTE - start_byte + 1; i++)
		{
			result_data[i] = (unsigned char)(start_byte + i);
		}
		for (int i = END_RANGE_BYTE - start_byte + 1; i < result_len; i++)
		{
			result_data[i] = (unsigned char)(START_RANGE_BYTE + i - (END_RANGE_BYTE - start_byte + 1));
		}
	}
	else
	{
		for (int i = 0; i < result_len; i++)
		{
			result_data[i] = (unsigned char)(start_byte + i);
		}
	}

	PG_RETURN_BYTEA_P(result);
}
/* End - ReqID:SRS-PLSQL-UTL_RAW */
