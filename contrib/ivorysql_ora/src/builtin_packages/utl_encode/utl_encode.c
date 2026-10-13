/*---------------------------------------------------------------------------------
 *
 * File:utl_encode.c
 *
 * Abstract:
 *		Internal implementation of UTL_ENCODE package
 *
 * ReqID: SRS-PLSQL-UTL_ENCODE
 *
 * Copyright:
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  contrib/ivorysql_ora/src/builtin_packages/utl_encode/utl_encode.c
 *---------------------------------------------------------------------------------
 */

/* Begin - ReqID:SRS-PLSQL-UTL_ENCODE */
#include "postgres.h"
#include "fmgr.h"
#include "mb/pg_wchar.h"
#include "utils/builtins.h"
#include "common/base64.h"

#define MAX_RAW_LEN 32767
#define UTL_ENCODE_BASE64_LINE_SIZE 64
#define UTL_ENCODE_CR 0x0D
#define UTL_ENCODE_LF 0x0A
#define UTL_ENCODE_PAD 0x3D

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
			errmsg("PL/SQL: numeric or value error: raw variable length too long")));
	}
}

static unsigned char *
utl_encode_convert_charset(unsigned char *src_data, int src_length,
								int from_encoding, int to_encoding, int *out_len)
{
	unsigned char  *converted_data = pg_do_encoding_conversion(src_data, src_length,
											   from_encoding, to_encoding);
	if (converted_data == src_data)
	{
		return NULL;
	}
	*out_len = strlen((char *) converted_data);
	return converted_data;
}

/*
 * utl_encode_base64_insert_crlf:
 * 		Insert 0x0D0A every 64 bytes except for the last line.
 * @base64_data: unsigned char array, src encoded text.
 * @base64_length: length of encoded text.
 * @dst_data: unsigned char array, text with 0x0D0A.
 * @return: length of dst_data
 * */
static int
utl_encode_base64_insert_crlf(unsigned char *base64_data, int base64_length,
                               unsigned char *dst_data)
{
	int src_pos;
	int dst_pos;
	int char_count;

	src_pos = 0;
	dst_pos = 0;
	char_count = 0;

	while (src_pos < base64_length)
	{
		dst_data[dst_pos++] = base64_data[src_pos++];
		char_count++;

		if (char_count == 64 && src_pos < base64_length)
		{
			dst_data[dst_pos++] = UTL_ENCODE_CR;
			dst_data[dst_pos++] = UTL_ENCODE_LF;
			char_count = 0;
		}
	}

	return dst_pos;
}

/*
 * utl_encode_base64_decode_prepare:
 *		Remove 0x0D or 0x0A, remove all subsequent bytes when scan first 0x3D.
 *		Pad the remain part with 0x3D to form complete 4bytes sequence.
 * @src_data: unsigned char array, src bytea data.
 * @src_data_length: length of src_data.
 * @dst_data: unsigned char array, bytea data after operation.
 * @return: length of dst_data
 * */
static int
utl_encode_base64_decode_prepare(unsigned char *src_data, int src_data_length,
									unsigned char *dst_data)
{
	int src_pos;
	int dst_pos;
	int remainder;
	unsigned char current;

	src_pos = 0;
	dst_pos = 0;

	// remove 0x0D, 0x0A, all subsequent bytes when scan first 0x3D
	while (src_pos < src_data_length)
	{
		current = src_data[src_pos];

		if (current == UTL_ENCODE_PAD)
		{
			break;
		}

		if (current == UTL_ENCODE_CR || current == UTL_ENCODE_LF)
		{
			src_pos++;
			continue;
		}

		dst_data[dst_pos++] = current;
		src_pos++;
	}

	if (dst_pos == 0)
	{
		return 0;
	}

	remainder = dst_pos % 4;
	if (remainder == 1)
	{
		// skip last one byte
		dst_pos -=1;
	}
	else if (remainder == 2)
	{
		dst_data[dst_pos++] = UTL_ENCODE_PAD;
		dst_data[dst_pos++] = UTL_ENCODE_PAD;
	}
	else if (remainder == 3)
	{
		dst_data[dst_pos++] = UTL_ENCODE_PAD;
	}

	return dst_pos;
}

/*
 * BASE64
 */
PG_FUNCTION_INFO_V1(utl_encode_base64_encode);
Datum
utl_encode_base64_encode(PG_FUNCTION_ARGS)
{
	bytea			*src_bytea;
	bytea			*result;
	int				src_bytea_length;
	int				encode_length;
	int				result_length;
	int				written_length;
	int				num_new_lines;
	unsigned char	*src_bytea_data;
	unsigned char	*encode_data;
	unsigned char	*result_data;

	if (PG_ARGISNULL(0))
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("bad argument: encode raw cannot be NULL")));
	}
	src_bytea = PG_GETARG_BYTEA_PP(0);
	src_bytea_length = VARSIZE_ANY_EXHDR(src_bytea);
	if (src_bytea_length == 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("bad argument: encode raw cannot be NULL")));
	}
	check_raw_length(src_bytea);

	// pg_b64_enc_len is error
	encode_length = (src_bytea_length + 2) / 3 * 4;
	num_new_lines = (encode_length - 1) / UTL_ENCODE_BASE64_LINE_SIZE;
	result_length = encode_length + 2 * num_new_lines;
	if (result_length > MAX_RAW_LEN)
	{
		ereport(ERROR,
			(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("PL/SQL: numeric or value error: raw variable length too long")));
	}
	encode_data = (unsigned char*)palloc(encode_length + 1);
	src_bytea_data = (unsigned char*)VARDATA_ANY(src_bytea);
	written_length = pg_b64_encode((const char *)src_bytea_data, src_bytea_length,
								(char *)encode_data, encode_length);
	if (written_length < 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INTERNAL_ERROR),
				 errmsg("base64 encoding failed")));
	}

	result = (bytea*) palloc(result_length + VARHDRSZ);
	SET_VARSIZE(result, result_length + VARHDRSZ);
	result_data = (unsigned char*)VARDATA(result);
	result_length = utl_encode_base64_insert_crlf(encode_data, written_length, result_data);
	pfree(encode_data);
	SET_VARSIZE(result, VARHDRSZ + result_length);
	PG_RETURN_BYTEA_P(result);
}

PG_FUNCTION_INFO_V1(utl_encode_base64_decode);
Datum
utl_encode_base64_decode(PG_FUNCTION_ARGS)
{
	bytea		   *src_bytea;
	bytea		   *result;
	int				src_bytea_length;
	int				clean_length;
	int				decode_length;
	int				decoded_length;
	unsigned char  *src_bytea_data;
	unsigned char  *clean_data;
	unsigned char  *result_data;

	if (PG_ARGISNULL(0))
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("bad argument: encode raw cannot be NULL")));
	}

	src_bytea = PG_GETARG_BYTEA_PP(0);
	src_bytea_length = VARSIZE_ANY_EXHDR(src_bytea);
	if (src_bytea_length == 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("bad argument: encode raw cannot be NULL")));
	}
	check_raw_length(src_bytea);
	src_bytea_data = (unsigned char *) VARDATA_ANY(src_bytea);

	// remove line breaks and decode may expand by up to 2 bytes with two '='
	clean_data = (unsigned char *) palloc(src_bytea_length + 2);
	clean_length = utl_encode_base64_decode_prepare(src_bytea_data, src_bytea_length, clean_data);

	if (clean_length == 0)
	{
		pfree(clean_data);
		PG_RETURN_NULL();
	}

	// get decode length
	decode_length = pg_b64_dec_len(clean_length);
	result = (bytea *) palloc(VARHDRSZ + decode_length);
	result_data = (unsigned char *) VARDATA(result);
	decoded_length = pg_b64_decode((const char *) clean_data,
								   clean_length,
								   (char *)result_data,
								   decode_length);
	pfree(clean_data);
	if (decoded_length < 0)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("invalid base64 input for BASE64_DECODE")));
	SET_VARSIZE(result, VARHDRSZ + decoded_length);
	PG_RETURN_BYTEA_P(result);
}

/*
 * QUOTED_PRINTABLE
 */
static inline int
qp_hex_value(unsigned char c)
{
	if (c >= '0' && c <= '9')
		return c - '0';
	if (c >= 'A' && c <= 'F')
		return c - 'A' + 10;
	return -1;
}

/*
 * utl_encode_qp_encode
 *		execute quoted_printable encode with out 0x3D0D0A
 *
 * @src_data: unsigned char array, src raw data
 * @src_length: length of src_data
 * @dst_data: unsigned char array, encoded raw data
 * @return: length of dst_data
 * */
static int
utl_encode_qp_encode(unsigned char *src_data, int src_length,
                          unsigned char *dst_data)
{
	int src_pos;
	int dst_pos;
	unsigned char cur;
	static const unsigned char hex_chars[] = "0123456789ABCDEF";

	src_pos = 0;
	dst_pos = 0;

	while (src_pos < src_length)
	{
		cur = src_data[src_pos];

		// 200D0A encode to =20 0D0A
		if (cur == 0x20 && src_pos + 2 < src_length &&
			src_data[src_pos + 1] == UTL_ENCODE_CR &&
			src_data[src_pos + 2] == UTL_ENCODE_LF)
		{
			dst_data[dst_pos++] = UTL_ENCODE_PAD;
			dst_data[dst_pos++] = 0x32;
			dst_data[dst_pos++] = 0x30;
			src_pos++;
			continue;
		}

		// 0x0D0A encode to 0x0D0A
		if (cur == UTL_ENCODE_CR && src_pos + 1 < src_length &&
			src_data[src_pos + 1] == UTL_ENCODE_LF)
		{
			dst_data[dst_pos++] = UTL_ENCODE_CR;
			dst_data[dst_pos++] = UTL_ENCODE_LF;
			src_pos += 2;
			continue;
		}

		// printable char(0x20 - 0x7E, without 0x3D) encode to itself, else encode to 3DXXXX
		if (cur >= 0x20 && cur <= 0x7E && cur != UTL_ENCODE_PAD)
		{
			dst_data[dst_pos++] = cur;
		}
		else
		{
			dst_data[dst_pos++] = UTL_ENCODE_PAD;
			dst_data[dst_pos++] = hex_chars[cur >> 4];
			dst_data[dst_pos++] = hex_chars[cur & 0x0F];
		}

		src_pos++;
	}

	return dst_pos;
}

/*
 * utl_encode_qp_insert_crlf
 *		Insert 0x3D0D0A every 75 bytes except for the last line.
 *
 * The input requirement is valid encoded text, than will not contain
 * individual 0x3D 0x0D 0x0A.
 * 
 * @qp_data: unsigned char array, src quote_printable encoded text.
 * @qp_length: length of qp_data
 * @dst_data: unsigned char array, text with 0x3D0D0A.
 * @return: length of dst_data
 * */
static int
utl_encode_qp_insert_crlf(unsigned char *qp_data, int qp_length,
                                unsigned char *dst_data)
{
	int src_pos;
	int dst_pos;
	int line_length;
	int enc_len;
	unsigned char cur;

	src_pos = 0;
	dst_pos = 0;
	line_length = 0;

	while (src_pos < qp_length)
	{
		cur = qp_data[src_pos];

		// 0x0D0A encoded to 0x0D0A, and reset line_length
		if (cur == UTL_ENCODE_CR && src_pos + 1 < qp_length &&
			qp_data[src_pos + 1] == UTL_ENCODE_LF)
		{
			dst_data[dst_pos++] = UTL_ENCODE_CR;
			dst_data[dst_pos++] = UTL_ENCODE_LF;
			line_length = 0;
			src_pos += 2;
			continue;
		}

		// 0x3D with other 2 bytes
		if (cur == UTL_ENCODE_PAD)
			enc_len = 3;
		else
			enc_len = 1;

		// insert 0x3D0D0A before 75 bytes
		if (line_length + enc_len > 75)
		{
			dst_data[dst_pos++] = UTL_ENCODE_PAD;
			dst_data[dst_pos++] = UTL_ENCODE_CR;
			dst_data[dst_pos++] = UTL_ENCODE_LF;
			line_length = 0;
		}

		dst_data[dst_pos++] = qp_data[src_pos++];
		if (enc_len == 3)
		{
			dst_data[dst_pos++] = qp_data[src_pos++];
			dst_data[dst_pos++] = qp_data[src_pos++];
		}
		line_length += enc_len;
	}

	return dst_pos;
}

/*
 * utl_encode_qp_decode
 *		Base quoted printable decode function, decode input raw in QP frame.
 *
 * @src_data: unsigned char array, src quote_printable encoded text.
 * @src_length: length of src_data
 * @dst_data: unsigned char array, decode result.
 * @return: length of dst_data
 * */
static int
utl_encode_qp_decode(unsigned char *src_data, int src_length, unsigned char *dst_data)
{
	int src_pos;
	int dst_pos;

	src_pos = 0;
	dst_pos = 0;

	while (src_pos < src_length)
	{
		int high_byte;
		int low_byte;
		unsigned char current = src_data[src_pos];
		unsigned char bytea1;
		unsigned char bytea2;

		// if 0x0D following consecutive 0x20 sequence, remove the sequence of 0x20
		if (current == 0x20)
		{
			int space_end = src_pos;
			while (space_end < src_length &&
				   src_data[space_end] == 0x20)
				space_end++;

			if (space_end < src_length &&
				src_data[space_end] == UTL_ENCODE_CR)
			{
				// remove sequence of 0x20
				src_pos = space_end;
				continue;
			}

			// print 0x20
			dst_data[dst_pos++] = current;
			src_pos++;
			continue;
		}

		// copy this byte is not 0x3D
		if (current != UTL_ENCODE_PAD)
		{
			dst_data[dst_pos++] = current;
			src_pos++;
			continue;
		}

		// 0x3D is the last byte, remove it
		if (src_pos + 1 >= src_length)
		{
			break;
		}

		bytea1 = src_data[src_pos + 1];

		// 0x3D with one byte:
		// 0x3D0D 0x3D0A decode to NULL, 0x3D00 decode to 0x00, others are error
		if (src_pos + 2 >= src_length)
		{
			if (bytea1 == 0x00)
			{
				dst_data[dst_pos++] = 0x00;
			}
			else if (bytea1 != UTL_ENCODE_CR && bytea1 != UTL_ENCODE_LF)
			{
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						 errmsg("invalid encoded string")));
			}

			src_pos += 2;
			continue;
		}

		bytea2 = src_data[src_pos + 2];

		// 0x3D00 with extra bytes, decode to 0x00 and others
		if (bytea1 == 0x00)
		{
			dst_data[dst_pos++] = 0x00;
			src_pos += 2;
			continue;
		}

		// 0x3D0D0D 0x3D0D0A 0x3D0A0A 0x3D0A0D decode to NULL
		if (bytea1 == UTL_ENCODE_CR || bytea1 == UTL_ENCODE_LF)
		{
			src_pos += (bytea2 == UTL_ENCODE_CR || bytea2 == UTL_ENCODE_LF) ? 3 : 2;
			continue;
		}

		// bytea1 and bytea2 are 0-F
		high_byte = qp_hex_value(bytea1);
		low_byte = qp_hex_value(bytea2);
		if (high_byte >= 0 && low_byte >= 0)
		{
			dst_data[dst_pos++] = (unsigned char) ((high_byte << 4) | low_byte);
			src_pos += 3;
			continue;
		}
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("invalid encoded string")));
	}
	return dst_pos;
}

PG_FUNCTION_INFO_V1(utl_encode_quoted_printable_encode);
Datum
utl_encode_quoted_printable_encode(PG_FUNCTION_ARGS)
{
	bytea		   *src_bytea;
	bytea		   *result;
	int				buf_size;
	int				result_length;
	int				src_bytea_length;
	int				encode_length;
	unsigned char  *src_bytea_data;
	unsigned char  *encode_bytea_data;
	unsigned char  *result_data;

	if (PG_ARGISNULL(0))
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("bad argument: encode raw cannot be NULL")));
	}
	src_bytea = PG_GETARG_BYTEA_PP(0);
	src_bytea_length = VARSIZE_ANY_EXHDR(src_bytea);
	if (src_bytea_length == 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("bad argument: encode raw cannot be NULL")));
	}
	check_raw_length(src_bytea);
	src_bytea_data = (unsigned char *) VARDATA_ANY(src_bytea);

	// set buffer size, maxize encoded raw is 3 * src_bytea_length
	// maxize size need add '=CRLF' every 73, 74 or 75 bytes.
	encode_length = 3 * src_bytea_length;
	encode_bytea_data = (unsigned char *)palloc(encode_length);
	encode_length = utl_encode_qp_encode(src_bytea_data, src_bytea_length, encode_bytea_data);
	if (encode_length > MAX_RAW_LEN)
	{
		ereport(ERROR,
			(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("PL/SQL: numeric or value error: raw variable length too long")));
	}
	buf_size = encode_length + (encode_length / 73 + 1) * 3;
	result = (bytea *) palloc(VARHDRSZ + buf_size);
	result_data = (unsigned char *) VARDATA(result);
	result_length = utl_encode_qp_insert_crlf(encode_bytea_data, encode_length, result_data);
	pfree(encode_bytea_data);
	if (result_length > MAX_RAW_LEN)
	{
		ereport(ERROR,
			(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("PL/SQL: numeric or value error: raw variable length too long")));
	}
	SET_VARSIZE(result, VARHDRSZ + result_length);

	PG_RETURN_BYTEA_P(result);
}

PG_FUNCTION_INFO_V1(utl_encode_quoted_printable_decode);
Datum
utl_encode_quoted_printable_decode(PG_FUNCTION_ARGS)
{
	bytea         *src_bytea;
	bytea         *result;
	int            src_bytea_length;
	int            result_length;
	unsigned char *src_bytea_data;
	unsigned char *result_data;

	// check param is NULL or '' or too long
	if (PG_ARGISNULL(0))
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("bad argument: decode raw cannot be NULL")));
	}
	src_bytea = PG_GETARG_BYTEA_PP(0);
	src_bytea_length = VARSIZE_ANY_EXHDR(src_bytea);
	if (src_bytea_length == 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("bad argument: decode raw cannot be NULL")));
	}
	check_raw_length(src_bytea);

	src_bytea_data = (unsigned char *) VARDATA_ANY(src_bytea);
	result = (bytea *) palloc(VARHDRSZ + src_bytea_length);
	result_data = (unsigned char *) VARDATA(result);
	result_length = utl_encode_qp_decode(src_bytea_data, src_bytea_length, result_data) ;

	// decode result is \x'', return NULL
	if (result_length == 0)
	{
		pfree(result);
		PG_RETURN_NULL();
	}
	SET_VARSIZE(result, VARHDRSZ + result_length);
	PG_RETURN_BYTEA_P(result);
}

/*
 * MIMEHEADER
 */

static int
utl_encode_mimeheader_qp_encode(unsigned char *src_data, int src_length,
                               unsigned char *dst_data)
{
	int src_pos;
	int dst_pos;
	int need_escape;
	unsigned char cur;
	static const unsigned char hex_chars[] = "0123456789ABCDEF";

	src_pos = 0;
	dst_pos = 0;

	while (src_pos < src_length)
	{
		cur = src_data[src_pos];

		// CRLF put ifself, and not reset calculate.
		if (cur == UTL_ENCODE_CR && src_pos + 1 < src_length &&
			src_data[src_pos + 1] == UTL_ENCODE_LF)
		{
			dst_data[dst_pos++] = UTL_ENCODE_CR;
			dst_data[dst_pos++] = UTL_ENCODE_LF;
			src_pos += 2;
			continue;
		}

		// 20-7E [except for: space, '?', '=', '_'] put itself, others translate
		// to the format of '=xx' according hexadecimalof ascii codes. 
		need_escape = 0;
		if (cur == 0x20 || cur == UTL_ENCODE_PAD || cur == 0x3F || cur == 0x5F
			|| cur < 0x20 || cur > 0x7E)
			need_escape = 1;

		if (need_escape)
		{
			dst_data[dst_pos++] = UTL_ENCODE_PAD;
			dst_data[dst_pos++] = hex_chars[cur >> 4];
			dst_data[dst_pos++] = hex_chars[cur & 0x0F];
		}
		else
			dst_data[dst_pos++] = cur;

		src_pos++;
	}

	return dst_pos;
}

/*
 * mimeheader_encode
 *		encode input text according RFC 2047 mime rule.
 *
 * encode-word: '=?<charset>?<encoding>?<encode_text>?='
 * An encode-word may not be more than 75bytes.
 * For <charset>: invaid encode_charset report error.
 * For <encoding>: Q is quoted_printable, B is base64.
 * For <encode_text>: maintain complete units, B maintains groups of 4 characters,
 * 	Q maintains format of '=xx'.
 * Multiple encode-words may be separated by CRLF.
 *
 * */
PG_FUNCTION_INFO_V1(utl_encode_mimeheader_encode);
Datum
utl_encode_mimeheader_encode(PG_FUNCTION_ARGS)
{
	text		   *src_text;
	text		   *encode_charset_text;
	text		   *result;
	int				src_length;
	int				work_length;
	int				encoded_length;
	int				charset_id;
	int				charset_name_length;
	int				encoding_value;
	int				encode_text_length;
	int				label_length;
	int				encode_text_pos;
	int				dst_pos;
	int				total_length;
	int				line_count;
	int				real_encode_text_len;
	char		   *charset_name;
	char			encoding_char;
	unsigned char  *src_data;
	unsigned char  *work_data;
	unsigned char  *converted_data;
	unsigned char  *encoded_data;
	unsigned char  *result_data;

	if (PG_ARGISNULL(0))
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("bad argument: encode string cannot be NULL")));
	}
	src_text = PG_GETARG_TEXT_PP(0);
	src_length = VARSIZE_ANY_EXHDR(src_text);
	if (src_length == 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("bad argument: encode string cannot be NULL")));
	}
	src_data = (unsigned char *) VARDATA_ANY(src_text);

	// check encode_charset
	if (PG_ARGISNULL(1) ||
		VARSIZE_ANY_EXHDR(PG_GETARG_TEXT_PP(1)) == 0)
	{
		charset_name = (char *)GetDatabaseEncodingName();
		charset_id = GetDatabaseEncoding();
	}
	else
	{
		encode_charset_text = PG_GETARG_TEXT_PP(1);
		charset_name = text_to_cstring(encode_charset_text);
		charset_id = pg_char_to_encoding(charset_name);
		if (charset_id < 0)
		{
			pfree(charset_name);
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("invalid Character set")));
		}
	}
	charset_name_length = strlen(charset_name);

	// check encoding value
	encoding_value = PG_ARGISNULL(2) ? 2 : PG_GETARG_INT32(2);

	// convert to target encode_charset if encode_charset is not null
	work_data = src_data;
	work_length = src_length;
	converted_data = NULL;
	if (charset_id != GetDatabaseEncoding())
	{
		converted_data = utl_encode_convert_charset(src_data, src_length,
						GetDatabaseEncoding(), charset_id, &work_length);
		if (converted_data != NULL)
			work_data = converted_data;
	}

	switch (encoding_value)
	{
		case 1:
			encoded_length = (work_length + 2) / 3 * 4;
			encoded_data = (unsigned char *) palloc(encoded_length);
			encoded_length = pg_b64_encode((const char *) work_data, work_length,
										   (char *) encoded_data,
										   encoded_length);
			if (encoded_length < 0)
			{
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						 errmsg("base64 encoding failed")));
			}
			encoding_char = 'B';
			break;

		case 2:
			encoded_data = (unsigned char *) palloc(3 * work_length);
			encoded_length = utl_encode_mimeheader_qp_encode(work_data, work_length,
															encoded_data);
			encoding_char = 'Q';
			break;
		default:
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						errmsg("bad argument: invalid encoding option")));
	}

	// get encode_text length, if base64, down to multiple of 4 bytes
	// label_length is '=?', charset_name, '?', 'B' or 'Q', '?' and '?='.
	label_length = 7 + charset_name_length;
	encode_text_length = 75 - label_length;
	if (encoding_value == 1)
		encode_text_length = (encode_text_length / 4) * 4;

	// calculate maximum size
	if (encoding_char == 'B')
	{
		line_count = (encoded_length + encode_text_length - 1) / encode_text_length;
		total_length = encoded_length + line_count * label_length +
			(line_count - 1) * 2;
	}
	else
	{
		// a character may be encoded to 3 bytes, calculate theoretical total length
		line_count = encoded_length / (encode_text_length - 2) + 1;
		total_length = encoded_length + line_count * label_length +
			(line_count - 1) * 2;
	}

	// set lable and encode_text to build encode result
	result = (text *) palloc(VARHDRSZ + total_length);
	result_data = (unsigned char *) VARDATA(result);
	dst_pos = 0;
	encode_text_pos = 0;

	while (encode_text_pos < encoded_length)
	{
		// '=?<encode_charset>?<encoding>?' label
		result_data[dst_pos++] = '=';
		result_data[dst_pos++] = '?';
		memcpy(result_data + dst_pos, charset_name, charset_name_length);
		dst_pos += charset_name_length;
		result_data[dst_pos++] = '?';
		result_data[dst_pos++] = encoding_char;
		result_data[dst_pos++] = '?';

		// calculate real encode_text length
		if (encoding_value == 1)
		{
			// maybe remain less than encode_text_length
			real_encode_text_len = Min(encode_text_length, encoded_length - encode_text_pos);
		}
		else
		{
			// add every character and check not more than encode_text_length
			real_encode_text_len = 0;
			while (encode_text_pos + real_encode_text_len < encoded_length)
			{
				int unit_len = encoded_data[encode_text_pos + real_encode_text_len] ==
					UTL_ENCODE_PAD ? 3 : 1;
				if (real_encode_text_len + unit_len > encode_text_length)
				{
					break;
				}
				real_encode_text_len += unit_len;
			}
		}

		// write data value
		memcpy(result_data + dst_pos, encoded_data + encode_text_pos, real_encode_text_len);
		dst_pos += real_encode_text_len;
		result_data[dst_pos++] = '?';
		result_data[dst_pos++] = '=';
		
		// check if need CRLF
		encode_text_pos += real_encode_text_len;
		if (encode_text_pos < encoded_length)
		{
			result_data[dst_pos++] = UTL_ENCODE_CR;
			result_data[dst_pos++] = UTL_ENCODE_LF;
		}
	}

	SET_VARSIZE(result, VARHDRSZ + dst_pos);

	pfree(encoded_data);
	if (converted_data != NULL)
		pfree(converted_data);

	PG_RETURN_TEXT_P(result);
}

static int
utl_encode_mimeheader_q_decode(unsigned char *src_data, int src_length,
                               unsigned char *dst_data)
{
	int				src_pos;
	int				dst_pos;
	unsigned char	cur;
	unsigned char	b1;
	unsigned char	b2;
	int				h1;
	int				h2;

	src_pos = 0;
	dst_pos = 0;

	while (src_pos < src_length)
	{
		cur = src_data[src_pos];

		if (cur == 0x3F)
		{
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("invalid mime header tag: encode_text contain '?'")));
		}

		if (cur == UTL_ENCODE_PAD)
		{
			if (src_pos + 1 >= src_length)
			{
				if (src_pos == 0)
				{
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
							 errmsg("invalid mime header tag")));
				}
				break;
			}

			if (src_pos + 2 >= src_length)
			{
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						 errmsg("invalid mime header tag: incomplete sequence")));
			}

			b1 = src_data[src_pos + 1];
			b2 = src_data[src_pos + 2];
			h1 = qp_hex_value(b1);
			h2 = qp_hex_value(b2);
			if (h1 < 0 || h2 < 0)
			{
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						 errmsg("invalid mime header tag: invalid hex sequence")));
			}

			dst_data[dst_pos++] = (unsigned char) ((h1 << 4) | h2);
			src_pos += 3;
			continue;
		}

		dst_data[dst_pos++] = cur;
		src_pos++;
	}

	return dst_pos;
}

PG_FUNCTION_INFO_V1(utl_encode_mimeheader_decode);
Datum
utl_encode_mimeheader_decode(PG_FUNCTION_ARGS)
{
	text			*buf_text;
	int				src_length;
	int				src_pos;
	int				charset_start;
	int				charset_len;
	int				decode_text_pos;
	int				decode_text_length;
	int				charset_id;
	int				decode_length;
	int				converted_len;
	int				prepared_length;
	char		   *charset_name;
	unsigned char	encoding_char;
	unsigned char  *src_data;
	unsigned char  *decode_data;
	unsigned char  *converted_data;
	unsigned char  *prepared_data;
	StringInfoData	buf;

	if (PG_ARGISNULL(0))
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("bad argument: decode string cannot be NULL")));
	}
	buf_text = PG_GETARG_TEXT_PP(0);
	src_length = VARSIZE_ANY_EXHDR(buf_text);
	if (src_length == 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("bad argument: decode string cannot be NULL")));
	}

	src_data = (unsigned char *) VARDATA_ANY(buf_text);
	src_pos = 0;
	initStringInfo(&buf);
	while (src_pos < src_length)
	{
		// '=?'
		if (src_pos + 1 >= src_length ||
			src_data[src_pos] != '=' || src_data[src_pos + 1] != '?')
		{
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("invalid mime header tag")));
		}
		src_pos += 2;

		// get encode charset
		charset_start = src_pos;
		while (src_pos < src_length && src_data[src_pos] != '?')
			src_pos++;
		if (src_pos >= src_length)
		{
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("invalid mime header tag")));
		}
		charset_len = src_pos - charset_start;
		charset_name = (char *) palloc(charset_len + 1);
		memcpy(charset_name, src_data + charset_start, charset_len);
		charset_name[charset_len] = '\0';
		charset_id = pg_char_to_encoding(charset_name);
		if (charset_id < 0)
		{
			pfree(charset_name);
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("invalid mime header tag: invalid Character set")));
		}
		pfree(charset_name);

		// skip second '?' and check encoding value
		src_pos++;
		if (src_pos >= src_length)
		{
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("invalid mime header tag: input string without encoding")));
		}
		encoding_char = src_data[src_pos];
		if (encoding_char != 'B' && encoding_char != 'Q')
		{
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("invalid mime header tag: invalid encoding")));
		}
		src_pos++;

		// skip third '?'
		if (src_pos >= src_length || src_data[src_pos] != '?')
		{
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("invalid mime header tag for MIMEHEADER_DECODE")));
		}
		src_pos++;

		// check encode_text, it cannot be null.
		decode_text_pos = src_pos;
		while (src_pos + 1 < src_length &&
			   !(src_data[src_pos] == '?' && src_data[src_pos + 1] == '='))
			src_pos++;
		if (src_pos + 1 >= src_length)
		{
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("invalid mime header tag")));
		}
		decode_text_length = src_pos - decode_text_pos;
		src_pos += 2;

		if (decode_text_length == 0)
		{
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("invalid mime header tag: decode_text is null")));
		}
		if (decode_text_length > 74)
		{
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("invalid mime header tag: decode_text is too long")));
		}

		if (encoding_char == 'Q')
		{
			decode_data = (unsigned char *)palloc(decode_text_length + 1);
			decode_length = utl_encode_mimeheader_q_decode(src_data + decode_text_pos,
														 decode_text_length, decode_data);
		}
		else
		{
			prepared_data = (unsigned char *)palloc(decode_text_length + 2);
			prepared_length = utl_encode_base64_decode_prepare(src_data + decode_text_pos,
				decode_text_length, prepared_data);
			if (prepared_length == 0)
			{
				ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("invalid mime header tag: invalid base64 decode text")));
			}
			decode_data = (unsigned char *)palloc(prepared_length);
			decode_length = pg_b64_decode((const char *)prepared_data, prepared_length,
				(char *)decode_data, prepared_length);
			if (decode_length < 0)
			{
				ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("invalid mime header tag: invalid base64 text")));
			}
		}

		// convert from target charset to database charset
		converted_data = NULL;
		if (charset_id != GetDatabaseEncoding())
		{
			converted_data = utl_encode_convert_charset(decode_data, decode_length,
														 charset_id, GetDatabaseEncoding(),
														 &converted_len);
			if (converted_data != NULL)
			{
				pfree(decode_data);
				decode_data = converted_data;
				decode_length = converted_len;
			}
		}

		appendBinaryStringInfo(&buf, (const char *)decode_data, decode_length);
		pfree(decode_data);

		if (src_pos >= src_length)
			break;

		// check separators
		if (src_data[src_pos] == '=')
		{
			// without separators, this '=' belongs to the next encode-word
			continue;
		}
		else if (src_data[src_pos] == UTL_ENCODE_CR)
		{
			// remove 0x0D or 0x0D0A
			src_pos++;
			if (src_pos < src_length && src_data[src_pos] == UTL_ENCODE_LF)
			{
				src_pos++;
			}
			continue;
		}
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("invalid mime header tag: invalid separators")));
	}

	// deal with 0x00
	for (int i = 0; i < buf.len; i++)
	{
		if (buf.data[i] == '\0')
		{
			if (i == 0)
			{
				// first byte is 0x00, error
				ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("invalid mime header tag: invalid decode text")));
			}
			buf.len = i;
			break;
		}
	}

	PG_RETURN_TEXT_P(cstring_to_text_with_len((const char *)buf.data, buf.len));
}

/*
 * TEXT
 */
PG_FUNCTION_INFO_V1(utl_encode_text_encode);
Datum
utl_encode_text_encode(PG_FUNCTION_ARGS)
{
	text		   *buf_text;
	text		   *encode_charset_text;
	text		   *result;
	int				src_length;
	int				src_enc_length;
	int				work_length;
	int				result_length;
	int				encoding_value;
	int				encode_length;
	int				encoding_id;
	char		   *encoding_name;
	unsigned char  *src_data;
	unsigned char  *work_data;
	unsigned char  *converted_data;
	unsigned char  *encode_data;
	unsigned char  *result_data;

	if (PG_ARGISNULL(0))
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("bad argument: encode raw cannot be NULL")));
	}
	buf_text = PG_GETARG_TEXT_PP(0);
	src_length = VARSIZE_ANY_EXHDR(buf_text);
	if (src_length == 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("bad argument: encode raw cannot be NULL")));
	}
	src_data = (unsigned char *) VARDATA_ANY(buf_text);

	work_data = src_data;
	work_length = src_length;
	converted_data = NULL;

	// encode_charset not null: convert from database charset to encode_charset, get raw data
	if (!PG_ARGISNULL(1))
	{
		encode_charset_text = PG_GETARG_TEXT_PP(1);
		if (VARSIZE_ANY_EXHDR(encode_charset_text) != 0)
		{
			encoding_name = text_to_cstring(encode_charset_text);
			encoding_id = pg_char_to_encoding((const char *)encoding_name);
			pfree(encoding_name);
			if (encoding_id < 0)
			{
				ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("invalid encoding name: %s", encoding_name)));
			}
			if (encoding_id != GetDatabaseEncoding())
			{
				converted_data = utl_encode_convert_charset(src_data,
					src_length, GetDatabaseEncoding(), encoding_id, &work_length);
			}
			if (converted_data != NULL)
			{
				work_data = converted_data;
			}
		}
	}

	// encoding: 1 is base64, 2 is quoted_printable, null is equal to 2
	encoding_value = PG_ARGISNULL(2) ? 2 : PG_GETARG_INT32(2);
	encode_data = NULL;
	result_data = NULL;
	encode_length = 0;
	result_length = 0;
	// use encode_charset and encoding, execute text_encode
	switch (encoding_value)
	{
		case 1:
		{
			src_enc_length = (work_length + 2) / 3 * 4;
			encode_data = (unsigned char *)palloc(src_enc_length);
			encode_length = pg_b64_encode((const char *)work_data, work_length,
				(char *)encode_data, src_enc_length);
			if (encode_length < 0)
			{
				ereport(ERROR,
					(errcode(ERRCODE_INTERNAL_ERROR),
					errmsg("base64 encoding failed")));
			}
			result_data = (unsigned char *)palloc(encode_length + (encode_length / 64) * 2);
			result_length = utl_encode_base64_insert_crlf(encode_data,
				encode_length, result_data);
			pfree(encode_data);
			break;
		}
		case 2:
		{
			src_enc_length = 3 * work_length;
			encode_data = (unsigned char *)palloc(src_enc_length);
			encode_length = utl_encode_qp_encode(work_data, work_length, encode_data);
			result_data = (unsigned char *)palloc(encode_length + 3 *(encode_length / 73 + 1));
			result_length = utl_encode_qp_insert_crlf(encode_data, encode_length, result_data);
			pfree(encode_data);
			break;
		}
		default:
			ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("bad argument: invalid encoding option")));
	}
	result = (text *) palloc(VARHDRSZ + result_length);
	SET_VARSIZE(result, VARHDRSZ + result_length);
	memcpy(VARDATA(result), result_data, result_length);
	pfree(result_data);
	if (converted_data != NULL)
		pfree(converted_data);

	PG_RETURN_TEXT_P(result);
}

PG_FUNCTION_INFO_V1(utl_encode_text_decode);
Datum
utl_encode_text_decode(PG_FUNCTION_ARGS)
{
	text		   *buf_text;
	text		   *encode_charset_text;
	text		   *result;
	int				src_length;
	int				decoded_length;
	int				output_length;
	int				prepare_length;
	int				capacity;
	int				encoding_value;
	int				encode_charset_id;
	unsigned char  *src_data;
	unsigned char  *prepare_data;
	unsigned char  *decoded_data;
	unsigned char  *converted_data;
	unsigned char  *output_data;

	if (PG_ARGISNULL(0))
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("bad argument: decode raw cannot be NULL")));
	}
	buf_text = PG_GETARG_TEXT_PP(0);
	src_length = VARSIZE_ANY_EXHDR(buf_text);
	if (src_length == 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("bad argument: decode raw cannot be NULL")));
	}
	src_data = (unsigned char *) VARDATA_ANY(buf_text);

	// get encode_charset
	encode_charset_id = GetDatabaseEncoding();
	if (!PG_ARGISNULL(1))
	{
		encode_charset_text = PG_GETARG_TEXT_PP(1);
		if (VARSIZE_ANY_EXHDR(encode_charset_text) != 0)
		{
			encode_charset_id = pg_char_to_encoding(text_to_cstring(encode_charset_text));
			if (encode_charset_id < 0)
			{
				ereport(ERROR,
						(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
						errmsg("bad argument: invalid encode_charset")));
			}
		}
	}

	// encoding: 1 is base64, 2 or NULL are quoted_printable
	if (PG_ARGISNULL(2))
		encoding_value = 2;
	else
	{
		encoding_value = PG_GETARG_INT32(2);
	}

	prepare_data = NULL;
	decoded_data = NULL;
	decoded_length = 0;

	switch (encoding_value)
	{
		case 1:
		{
			capacity = src_length + 2;
			prepare_data = (unsigned char *)palloc(capacity);
			prepare_length = utl_encode_base64_decode_prepare(src_data, src_length,
															prepare_data);
			if (prepare_length == 0)
			{
				pfree(prepare_data);
				PG_RETURN_NULL();
			}
			capacity = pg_b64_dec_len(prepare_length);
			decoded_data = (unsigned char *)palloc(capacity);
			decoded_length = pg_b64_decode((const char *)prepare_data, prepare_length,
											(char *)decoded_data, capacity);
			pfree(prepare_data);
			if (decoded_length < 0)
			{
				ereport(ERROR,
					(errcode(ERRCODE_INTERNAL_ERROR),
					errmsg("base64 decoding failed")));
			}
			break;
		}
		case 2:
		{
			capacity = src_length;
			decoded_data = (unsigned char *)palloc(capacity);
			decoded_length = utl_encode_qp_decode(src_data, src_length, decoded_data);
			if (decoded_length == 0)
			{
				pfree(decoded_data);
				PG_RETURN_NULL();
			}
			break;
		}
		default:
			ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("bad argument: invalid decoding option")));
	}

	output_data = decoded_data;
	output_length = decoded_length;
	converted_data = NULL;

	if (encode_charset_id != GetDatabaseEncoding())
	{
		converted_data = utl_encode_convert_charset(decoded_data,
			decoded_length, encode_charset_id, GetDatabaseEncoding(), &output_length);
		if (converted_data != NULL)
		{
			output_data = converted_data;
		}
	}

	result = (text *) palloc(VARHDRSZ + output_length);
	SET_VARSIZE(result, VARHDRSZ + output_length);
	memcpy(VARDATA(result), output_data, output_length);
	pfree(decoded_data);
	if (converted_data != NULL)
		pfree(converted_data);

	PG_RETURN_TEXT_P(result);
}

/*
 * Unix to Unix
 */
PG_FUNCTION_INFO_V1(utl_encode_uuencode);
Datum
utl_encode_uuencode(PG_FUNCTION_ARGS)
{
	bytea		   *src_bytea;
	bytea		   *result;
	text           *filename;
	text           *permission;
	int             chunk;
	int				result_pos;
	int				src_pos;
	int				file_type;
	int				src_bytea_length;
	int				filename_length;
	int				permission_length;
	int				result_length;
	int				header_len;
	int				data_len;
	int				trailer_len;
	int				full_lines;
	int				last_bytes;
	unsigned char  *src_bytea_data;
	unsigned char  *filename_data;
	unsigned char  *permission_data;
	unsigned char  *result_data;

	if (PG_ARGISNULL(0))
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("bad argument: encode raw cannot be NULL")));
	}
	src_bytea = PG_GETARG_BYTEA_PP(0);
	src_bytea_length = VARSIZE_ANY_EXHDR(src_bytea);
	if (src_bytea_length == 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("bad argument: encode raw cannot be NULL")));
	}
	check_raw_length(src_bytea);
	src_bytea_data = (unsigned char *) VARDATA_ANY(src_bytea);

	// get type
	if (PG_ARGISNULL(1))
		file_type = 1;
	else
	{
		file_type = PG_GETARG_INT32(1);
		if (file_type < 1 || file_type > 4)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("bad argument: invalid type value %d for UUENCODE", file_type)));
	}

	// get filename
	if (PG_ARGISNULL(2))
	{
		filename_data = NULL;
		filename_length = 0;
	}
	else
	{
		filename = PG_GETARG_TEXT_PP(2);
		filename_data = (unsigned char *) VARDATA_ANY(filename);
		filename_length = VARSIZE_ANY_EXHDR(filename);
	}

	// get permission
	if (PG_ARGISNULL(3))
	{
		permission_data = NULL;
		permission_length = 0;
	}
	else
	{
		permission = PG_GETARG_TEXT_PP(3);
		permission_data = (unsigned char *) VARDATA_ANY(permission);
		permission_length = VARSIZE_ANY_EXHDR(permission);
	}

	// calculate and check the length of result
	header_len = 0;
	data_len = 0;
	trailer_len = 0;

	// calculate header if exist
	if (file_type == 1 || file_type == 2)
	{
		header_len = 6 + permission_length + 1 + filename_length + 2;
	}

	// calculate data
	full_lines = src_bytea_length / 57;
	last_bytes = src_bytea_length % 57;
	data_len = full_lines * (1 + 19 * 4 + 2);
	if (last_bytes > 0)
	{
		data_len += 1 + ((last_bytes + 2) / 3) * 4 + 2;
	}

	// calculate end line if exist
	if (file_type == 1 || file_type == 4)
	{
		trailer_len = 4;
	}

	result_length = header_len + data_len + trailer_len;
	if (result_length > MAX_RAW_LEN)
	{
		ereport(ERROR,
			(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("PL/SQL: numeric or value error: raw variable length too long")));
	}

	result = (bytea *) palloc(VARHDRSZ + result_length);
	result_data = (unsigned char *) VARDATA(result);
	SET_VARSIZE(result, VARHDRSZ + result_length);
	result_pos = 0;

	// header line: 'begin <permission> <filename>' || CRLF
	if (file_type == 1 || file_type == 2)
	{
		memcpy(result_data + result_pos, "begin ", 6);
		result_pos += 6;

		if (permission_length > 0)
		{
			memcpy(result_data + result_pos, permission_data, permission_length);
			result_pos += permission_length;
		}
		result_data[result_pos++] = 0x20;

		if (filename_length > 0)
		{
			memcpy(result_data + result_pos, filename_data, filename_length);
			result_pos += filename_length;
		}

		result_data[result_pos++] = UTL_ENCODE_CR;
		result_data[result_pos++] = UTL_ENCODE_LF;
	}

	// data line: each group 57 bytes, length + encode result + CRLF
	src_pos = 0;
	while (src_pos < src_bytea_length)
	{
		chunk = (src_bytea_length - src_pos >= 57) ? 57 : (src_bytea_length - src_pos);

		// length = bytea_len + 32 + ceil(bytea_len / 3)
		result_data[result_pos++] =
			(unsigned char) (chunk + 32 + ((chunk + 2) / 3));

		// Encode rule
		// Each group 3 bytes with total 24bits.
		// Each group of 6 bits add 32 to 4 bytes.
		for (int j = 0; j < chunk; j += 3)
		{
			unsigned int b0 = src_bytea_data[src_pos + j];
			unsigned int b1 = (j + 1 < chunk) ? src_bytea_data[src_pos + j + 1] : 0;
			unsigned int b2 = (j + 2 < chunk) ? src_bytea_data[src_pos + j + 2] : 0;
			unsigned int triple = (b0 << 16) | (b1 << 8) | b2;

			result_data[result_pos++] =
				(unsigned char) (((triple >> 18) & 0x3F) + 32);
			result_data[result_pos++] =
				(unsigned char) (((triple >> 12) & 0x3F) + 32);
			result_data[result_pos++] =
				(unsigned char) (((triple >> 6) & 0x3F) + 32);
			result_data[result_pos++] =
				(unsigned char) ((triple & 0x3F) + 32);
		}

		result_data[result_pos++] = UTL_ENCODE_CR;
		result_data[result_pos++] = UTL_ENCODE_LF;

		src_pos += chunk;
	}

	// end line if exist: LF + 'end'
	if (file_type == 1 || file_type == 4)
	{
		result_data[result_pos++] = UTL_ENCODE_LF;
		memcpy(result_data + result_pos, "end", 3);
		result_pos += 3;
	}
	Assert(result_pos == result_length);

	PG_RETURN_BYTEA_P(result);
}

PG_FUNCTION_INFO_V1(utl_encode_uudecode);
Datum
utl_encode_uudecode(PG_FUNCTION_ARGS)
{
	bytea		   *src_bytea;
	bytea		   *result;
	int				src_length;
	int				result_pos;
	int				length_byte;
	int				length_value;
	int				decoded_length;
	int				encoded_length;
	int				full_decoded_length;
	int				newline_count;
	unsigned char  *src_data;
	unsigned char  *src_end;
	unsigned char  *pos;
	unsigned char  *result_data;
	unsigned char  *line_data;
	unsigned char  *scan;
	unsigned char	line_buffer[76];
	unsigned int	sextet0;
	unsigned int	sextet1;
	unsigned int	sextet2;
	unsigned int	sextet3;
	unsigned int	triple;

	if (PG_ARGISNULL(0))
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("bad argument: decode raw cannot be NULL")));
	}
	src_bytea = PG_GETARG_BYTEA_PP(0);
	src_length = VARSIZE_ANY_EXHDR(src_bytea);
	if (src_length == 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("bad argument: decode raw cannot be NULL")));
	}
	check_raw_length(src_bytea);
	src_data = (unsigned char *) VARDATA_ANY(src_bytea);
	src_end = src_data + src_length;
	pos = src_data;

	// remove header line if exist
	if (src_end - pos >= 5 && memcmp(pos, "begin", 5) == 0)
	{
		while (pos < src_end && *pos != UTL_ENCODE_CR && *pos != UTL_ENCODE_LF)
			pos++;

		if (pos < src_end)
		{
			if (*pos == UTL_ENCODE_CR)
			{
				pos++;
				if (pos < src_end && (*pos == UTL_ENCODE_CR || *pos == UTL_ENCODE_LF))
					pos++;
			}
			else
				pos++;
		}
	}

	// remove end line if exist
	if (src_end - pos >= 3 && memcmp(src_end - 3, "end", 3) == 0)
	{
		if (src_end - pos >= 4)
		{
			unsigned char *end_prev = src_end - 4;

			if (*end_prev == UTL_ENCODE_LF)
				src_end -= 4;
			else if (*end_prev == UTL_ENCODE_CR)
			{
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						 errmsg("invalid uuencode input")));
			}
			else
				src_end -= 3;
		}
		else
			src_end -= 3;
	}

	// remove last 0x0D or 0x0A in data part
	if (src_end > pos && (*(src_end - 1) == UTL_ENCODE_CR || *(src_end - 1) == UTL_ENCODE_LF))
		src_end--;

	result = (bytea *) palloc(VARHDRSZ + src_length);
	result_data = (unsigned char *) VARDATA(result);
	result_pos = 0;
	while (pos < src_end)
	{
		length_byte = *pos++;
		length_value = length_byte - 32;

		if (length_value < 0)
		{
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("invalid uuencode input")));
		}

		// calculate the length of decode result cannot more than 57 bytes.
		// maybe length_value is invalid, decode result rounded down,
		// excepted bytes to be read are rounded up
		decoded_length = (length_value * 3) / 4;
		if (decoded_length > 57)
		{
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("invalid uuencode input")));
		}

		// calculate number of data than need to be read for decoding
		encoded_length = 4 * ((length_value + 3) / 4);
		full_decoded_length = 0;
		if (decoded_length > 0)
		{
			if (src_end - pos < encoded_length)
			{
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						 errmsg("invalid uuencode input")));
			}

			line_data = pos;
			for (int group_offset = 0; group_offset < encoded_length; group_offset += 4)
			{
				sextet0 = line_data[group_offset] - 32;
				sextet1 = line_data[group_offset + 1] - 32;
				sextet2 = line_data[group_offset + 2] - 32;
				sextet3 = line_data[group_offset + 3] - 32;

				if (sextet0 > 63 || sextet1 > 63 ||
					sextet2 > 63 || sextet3 > 63)
				{
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
							 errmsg("invalid uuencode input")));
				}

				triple = (sextet0 << 18) | (sextet1 << 12) |
						 (sextet2 << 6) | sextet3;

				line_buffer[(group_offset / 4) * 3]     = (triple >> 16) & 0xFF;
				line_buffer[(group_offset / 4) * 3 + 1] = (triple >> 8) & 0xFF;
				line_buffer[(group_offset / 4) * 3 + 2] = triple & 0xFF;
			}

			// if current is not last line, decode result needs to be rounded up
			// to a multiple of 3.
			full_decoded_length = (encoded_length / 4) * 3;
		}

		// maybe decoded_length is 0, need skip 4 bytes
		pos += encoded_length;

		// remove extra bytes util 0x0D or 0x0A
		scan = pos;
		while (scan < src_end && *scan != UTL_ENCODE_CR && *scan != UTL_ENCODE_LF)
			scan++;

		// scan and remove 1 or 2 0x0D 0x0A
		newline_count = 0;
		while (scan < src_end && (*scan == UTL_ENCODE_CR || *scan == UTL_ENCODE_LF))
		{
			newline_count++;
			scan++;
		}

		if (newline_count >= 3)
		{
			ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("invalid uuencode input")));
		}

		// extra bytes with no 0x0D or 0x0A util end byte
		if (scan == src_end)
		{
			if (decoded_length > 0)
			{
				memcpy(result_data + result_pos, line_buffer, decoded_length);
				result_pos += decoded_length;
			}
			break;
		}

		if (full_decoded_length > 0)
		{
			memcpy(result_data + result_pos, line_buffer, full_decoded_length);
			result_pos += full_decoded_length;
		}
		pos = scan;
	}

	if (result_pos == 0)
	{
		pfree(result);
		PG_RETURN_NULL();
	}

	SET_VARSIZE(result, VARHDRSZ + result_pos);

	PG_RETURN_BYTEA_P(result);
}

/* End - ReqID:SRS-PLSQL-UTL_ENCODE */