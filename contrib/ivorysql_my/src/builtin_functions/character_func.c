/*-------------------------------------------------------------------------------
 *
 * File: character_func.c
 *
 * Abstract:
 * 		This file contains the implementation of MySQL's
 *		character data type related built-in functions.
 *
 * Authored by jinhuajian@highgo.com, 20250205.
 *
 * Copyright:
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *		contrib/ivorysql_my/src/builtin_functions/character_func.c
 *
 *-------------------------------------------------------------------------------
 */
#include "postgres.h"
#include "fmgr.h"

#include "access/detoast.h"
#include "common/int.h"
#include "common/base64.h"
#include "catalog/pg_namespace.h"
#include "catalog/pg_collation.h"
#include "funcapi.h"
#include "lib/stringinfo.h"
#include "mb/pg_wchar.h"
#include "parser/parse_coerce.h"
#include "utils/builtins.h"
#include "utils/varlena.h"
#include "utils/guc.h"
#include "utils/varbit.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"
#include "utils/date.h"
#include "utils/timestamp.h"
#include "utils/float.h"
#include "utils/numeric.h"
#include "utils/datetime.h"

#include "../include/ivorysql_my.h"


/*
 * Base64 character encoding table.
 * Used for converting 6-bit integer values (0–63) to their corresponding Base64 character.
 * For example: 0 → 'A', 1 → 'B', ..., 62 → '+', 63 → '/'
 */
static const char _base64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/*
 * Base64 character decoding lookup table.
 * Maps ASCII character codes (0–127) to their corresponding Base64 values (0–63).
 * Characters not part of the Base64 alphabet are mapped to -1 (invalid).
 * For example: 'A' (65) → 0, 'B' (66) → 1, ..., '+' (43) → 62, '/' (47) → 63
 */
static const int8 b64lookup[128] = {
	-1, -1, -1, -1, -1, -1, -1, -1,     // 0-7   (control characters)
	-1, -1, -1, -1, -1, -1, -1, -1,     // 8-15
	-1, -1, -1, -1, -1, -1, -1, -1,     // 16-23
	-1, -1, -1, -1, -1, -1, -1, -1,     // 24-31
	-1, -1, -1, -1, -1, -1, -1, -1,     // 32-39
	-1, -1, -1, 62, -1, -1, -1, 63,     // 40-47 ('+', '/')
	52, 53, 54, 55, 56, 57, 58, 59,     // 48-55 ('0'-'7')
	60, 61, -1, -1, -1, -1, -1, -1,     // 56-63 ('8'-'9', '=' → -1)
	-1,  0,  1,  2,  3,  4,  5,  6,     // 64-71 ('A'-'G')
	 7,  8,  9, 10, 11, 12, 13, 14,     // 72-79 ('H'-'O')
	15, 16, 17, 18, 19, 20, 21, 22,     // 80-87 ('P'-'W')
	23, 24, 25, -1, -1, -1, -1, -1,     // 88-95 ('X'-'Z')
	-1, 26, 27, 28, 29, 30, 31, 32,     // 96-103 ('a'-'g')
	33, 34, 35, 36, 37, 38, 39, 40,     // 104-111 ('h'-'o')
	41, 42, 43, 44, 45, 46, 47, 48,     // 112-119 ('p'-'w')
	49, 50, 51, -1, -1, -1, -1, -1      // 120-127 ('x'-'z')
};

#define SOUNDEX_LEN 4
#define SOUND_THRESHOLD 4

/* ABCDEFGHIJKLMNOPQRSTUVWXYZ */
static const char *soundex_table = "01230120022455012623010202";

static void set_sound(const char *instr, char *outstr, int size);
static char soundex_code(char letter);


PG_FUNCTION_INFO_V1(locate);
PG_FUNCTION_INFO_V1(insert);
PG_FUNCTION_INFO_V1(elt);
PG_FUNCTION_INFO_V1(field);
PG_FUNCTION_INFO_V1(substring_index);
PG_FUNCTION_INFO_V1(ord);
PG_FUNCTION_INFO_V1(mys_text_length);
PG_FUNCTION_INFO_V1(find_in_set);
PG_FUNCTION_INFO_V1(space);
PG_FUNCTION_INFO_V1(myschar);
PG_FUNCTION_INFO_V1(hex_str);
PG_FUNCTION_INFO_V1(hex_num);
PG_FUNCTION_INFO_V1(hex_bytea);
PG_FUNCTION_INFO_V1(to_bin64);
PG_FUNCTION_INFO_V1(bin_num);
PG_FUNCTION_INFO_V1(bin_text);
PG_FUNCTION_INFO_V1(soundex);
PG_FUNCTION_INFO_V1(export_set);

PG_FUNCTION_INFO_V1(base64_decode);
PG_FUNCTION_INFO_V1(base64_encode);

static char
soundex_code(char letter)
{
	letter = toupper((unsigned char) letter);
	/* Defend against non-ASCII letters */
	if (letter >= 'A' && letter <= 'Z')
		return soundex_table[letter - 'A'];
	return letter;
}


static void
set_sound(const char *instr, char *outstr, int size)
{
	int			count;

	AssertArg(instr);
	AssertArg(outstr);

	/* Skip leading non-alphabetic characters */
	while (*instr && !isalpha((unsigned char) *instr))
		++instr;

	/* If no string left, return all-zeroes buffer */
	if (!*instr)
	{
		memset(outstr, '\0', size);
		return;
	}

	/* Take the first letter as is */
	*outstr++ = (char) toupper((unsigned char) *instr++);

	count = 1;
	while (*instr)
	{
		if (isalpha((unsigned char)*instr) &&
					  soundex_code(*instr) != soundex_code(*(instr - 1)) &&
					  soundex_code(*instr) != soundex_code(*(outstr - 1)))
		{
			if (soundex_code(instr[0]) != '0' && soundex_code(*instr) != *(outstr - 1))
			{
				*outstr = soundex_code(instr[0]);
				++outstr;
				++count;
			}
		}
		++instr;
	}

	/* Fill with 0's */
	while (count < SOUNDEX_LEN)
	{
		*outstr = '0';
		++outstr;
		++count;
	}

	/* And null-terminate */
	*outstr = '\0';
}

static unsigned
b64_decode(const char* src, unsigned len, char* dst, bool flag)
{
	const char *srcend = src + len, *s = src;
	char* p = dst;
	char c;
	int b = 0;
	uint32 buf = 0;
	int pos = 0, end = 0;

	while (s < srcend)
	{
		c = *s++;

#if 0
		/* when use psql: SELECT hex(FROM_BASE64('  A  \n  B  \r  C  \t D  '));
		   the SQL change to SELECT hex(FROM_BASE64('  A  \\n  B  \\r  C  \\t D  ')); */
		if (MyProcPort && MyProcPort->application_name && strcmp(MyProcPort->application_name, "psql") == 0)
		{
			char slash_c = '\0';
			/* this is bad way to solve :SELECT hex(FROM_BASE64('  A  \n  B  \r  C  \t D  ')); */
			if (c == '\\')
			{
				slash_c = *s++;
			}

			if (slash_c != '\0' && c == '\\')
			{
				if (slash_c == 't')
					c = '\t';
				else if (slash_c == 'n')
					c = '\n';
				else if (slash_c == 'r')
					c = '\r';
			}
		}
#endif

		if (c == ' ' || c == '\t' || c == '\n' || c == '\r')
			continue;

		if (c == '=')
		{
			/* end sequence */
			if (!end)
			{
				if (pos == 2)
					end = 1;
				else if (pos == 3)
					end = 2;
				else
				{
					if (flag)
						ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE), errmsg("unexpected \"=\"")));
					else
					{
						*dst = 0;
						return 0;
					}
				}
			}
			b = 0;
		}
		else
		{
			b = -1;
			if (c > 0 && c < 127)
				b = b64lookup[(unsigned char)c];
			if (b < 0)
			{
				if (flag)
					ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE), errmsg("invalid symbol")));
				else
				{
					*dst = 0;
					return 0;
				}
			}
		}
		/* add it to buffer */
		buf = (buf << 6) + b;
		pos++;
		if (pos == 4)
		{
			*p++ = (buf >> 16) & 255;
			if (end == 0 || end > 1)
				*p++ = (buf >> 8) & 255;
			if (end == 0 || end > 2)
				*p++ = buf & 255;
			buf = 0;
			pos = 0;
		}
	}

	if (pos != 0)
	{
		if (flag)
			ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE), errmsg("invalid end sequence")));
		else
		{
			*dst = 0;
			return 0;
		}
	}
	return p - dst;
}


static int
b64_encode(const char *src, unsigned len, char *dst)
{
	char   *p,   *lend = dst + 76;
	const char   *s;
	const char *end = src + len;
	int		pos = 2;
	unsigned long buf = 0;

	s = src;
	p = dst;

	while (s < end)
	{
		buf |= (unsigned char)(*s) << (pos << 3);
		pos--;
		s++;

		/*
		 * write it out
		 */
		if (pos < 0)
		{
			*p++ = _base64[(buf >> 18) & 0x3f];
			*p++ = _base64[(buf >> 12) & 0x3f];
			*p++ = _base64[(buf >> 6) & 0x3f];
			*p++ = _base64[buf & 0x3f];
			pos = 2;
			buf = 0;
		}
		if (p >= lend)
		{
			*p++ = '\n';
			lend = p + 76;
		}
	}
	if (pos != 2)
	{
		*p++ = _base64[(buf >> 18) & 0x3f];
		*p++ = _base64[(buf >> 12) & 0x3f];
		*p++ = (pos == 0) ? _base64[(buf >> 6) & 0x3f] : '=';
		*p++ = '=';
	}

	return p - dst;
}


static bytea*
b64_decode_internal(const char *data, int dataLength)
{
	int		resultLength, res = 0;
	bytea  *result;

	resultLength = pg_b64_dec_len(dataLength);
	result = (bytea*)palloc(VARHDRSZ + resultLength);
	if (resultLength > 0)
	{
		res = b64_decode(data, dataLength, VARDATA(result), false);
	}
	SET_VARSIZE(result, VARHDRSZ + res);
	return result;
}

static text*
b64_encode_internal(const char *data, int dataLength)
{
	int		resultLength, res;
	text   *result;

	resultLength = pg_b64_enc_len(dataLength);
	result = (text *)palloc(VARHDRSZ + resultLength);
	res = b64_encode(data, dataLength, VARDATA(result));
	SET_VARSIZE(result, VARHDRSZ + res);
	return result;
}

Datum
base64_decode(PG_FUNCTION_ARGS)
{
	text   *data = PG_GETARG_TEXT_P(0);
	int		dataLength = VARSIZE(data) - VARHDRSZ;
	bytea  *result ;
	int		resultLength;

	result = b64_decode_internal(VARDATA(data), dataLength);
	resultLength = VARSIZE(result) - VARHDRSZ;

	if (dataLength != 0 && resultLength == 0)
		PG_RETURN_NULL();
	PG_RETURN_TEXT_P(result);
}

Datum
base64_encode(PG_FUNCTION_ARGS)
{
	text   *data = PG_GETARG_TEXT_P(0);
	int		dataLength = VARSIZE(data) - VARHDRSZ;
	text   *result = b64_encode_internal(VARDATA(data), dataLength);
	PG_RETURN_TEXT_P(result);
}

/*
 * Workhorse for to_bin, to_oct, and to_hex.  Note that base must be > 1 and <=
 * 16.
 */
static inline text *
convert_to_base(uint64 value, int base)
{
	const char *digits = "0123456789abcdef";

	/* We size the buffer for to_bin's longest possible return value. */
	char		buf[sizeof(uint64) * BITS_PER_BYTE];
	char	   *const end = buf + sizeof(buf);
	char	   *ptr = end;

	Assert(base > 1);
	Assert(base <= 16);

	do
	{
		*--ptr = digits[value % base];
		value /= base;
	} while (ptr > buf && value);

	return cstring_to_text_with_len(ptr, end - ptr);
}

/*
 * Convert an integer to a string containing a base-2 (binary) representation
 * of the number.
 */
Datum
to_bin64(PG_FUNCTION_ARGS)
{
	uint64		value = (uint64) PG_GETARG_INT64(0);

	PG_RETURN_TEXT_P(convert_to_base(value, 2));
}

Datum
bin_num(PG_FUNCTION_ARGS)
{
	uint64  value;
	Numeric num;

	num = PG_GETARG_NUMERIC(0);
	
	// mysql will do round off in bin(numeric)
	value = conv_numeric_uint64(num, false);

	PG_RETURN_TEXT_P(convert_to_base(value, 2));
}

Datum
bin_text(PG_FUNCTION_ARGS)
{
	char 		*arg = text_to_cstring(PG_GETARG_TEXT_PP(0));
	char	  	num[64];
	uint64 		value;

	extract_leading_integer_str(arg, num, sizeof(num));

	value = DirectFunctionCall3(mysuint8_in,
								CStringGetDatum(num),
								ObjectIdGetDatum(InvalidOid),
								Int32GetDatum(-1));

	pfree(arg);

	return DirectFunctionCall1(to_bin64, UInt64GetDatum(value));
}

Datum
elt(PG_FUNCTION_ARGS)
{
	int64	num = PG_GETARG_INT64(0);
	Datum	*args;
	Oid		*types;
	bool	*nulls;

	if (num < 1 || num > PG_NARGS())
		PG_RETURN_NULL();

	extract_variadic_args(fcinfo, 1, false, &args, &types, &nulls);

	PG_RETURN_DATUM(args[num - 1]);
}

/**
 * Return a string such that for every bit set in the value bits, 
 * you get an on string and for every unset bit, you get an off string.
 */
Datum
export_set(PG_FUNCTION_ARGS)
{
	int64 	bits = PG_GETARG_INT64(0);
	text 	*on_text = PG_GETARG_TEXT_PP(1),
			*off_text = PG_GETARG_TEXT_PP(2),
			*separator = PG_NARGS() > 3 ? PG_GETARG_TEXT_PP(3) : cstring_to_text(",");
	int 	 nbits = PG_NARGS() > 4 ? PG_GETARG_INT32(4) : 64;
	char 	*on_cstr = NULL,
			*off_cstr = NULL,
			*sep_cstr = NULL;
	int		 on_len,
			 off_len,
			 sep_len;
	text 	*ret;
	StringInfoData result;

	if (nbits > 64 || nbits < 0)
		nbits = 64;

	on_cstr = VARDATA_ANY(on_text);
	on_len = VARSIZE_ANY_EXHDR(on_text);
	off_cstr = VARDATA_ANY(off_text);
	off_len = VARSIZE_ANY_EXHDR(off_text);
	sep_cstr = VARDATA_ANY(separator);
	sep_len = VARSIZE_ANY_EXHDR(separator);

	initStringInfo(&result);

	for (int i = 0; i < nbits; i++)
	{
		if (i > 0)
			appendBinaryStringInfo(&result, sep_cstr, sep_len);

		if ((bits >> i) & 1)
			appendBinaryStringInfo(&result, on_cstr, on_len);
		else
			appendBinaryStringInfo(&result, off_cstr, off_len);
	}

	ret = cstring_to_text_with_len(result.data, result.len);

	pfree(result.data);

	PG_RETURN_VARCHAR_P(ret);
}

Datum
field(PG_FUNCTION_ARGS)
{
	int		nargs;
	Datum	*args;
	Oid		*types;
	bool	*nulls;
	char 	*first_arg,
			*match_arg;
	int		 first_len,
			 match_len;
	text	*raw;

	/* fetch argument values to build the array */
	nargs = extract_variadic_args(fcinfo, 0, false, &args, &types, &nulls);

	if (nargs < 2)
	{
		ereport(ERROR, (errcode(ERRCODE_UNDEFINED_PARAMETER),
			errmsg("Incorrect parameter count in the call to native function 'field'")));
	}

	if (PG_ARGISNULL(0))
		PG_RETURN_INT32(0);

	if (args[0] == (Datum)0)
		PG_RETURN_INT32(0);

	raw = DatumGetTextP(args[0]);
	first_arg = VARDATA_ANY(raw);
	first_len = VARSIZE_ANY_EXHDR(raw);

	for (int i = 1; i < nargs; i++)
	{
		if (args[i] == (Datum)0)
			PG_RETURN_INT32(0);

		raw = DatumGetTextP(args[i]);
		match_arg = VARDATA_ANY(raw);
		match_len = VARSIZE_ANY_EXHDR(raw);

		if (match_arg == NULL)
			continue;

		if (varstr_cmp(first_arg, first_len, match_arg, match_len, PG_GET_COLLATION()) == 0)
			PG_RETURN_INT64(i);
	}

	PG_RETURN_INT32(0);
}

Datum
find_in_set(PG_FUNCTION_ARGS)
{
	char 		*str,
				*strlist;
	ArrayType 	*arr;
	Datum  		*values;
	int 		 nvalues;
	int			 ret = 0, 
				 len_str;
	text		*raw;

	raw = PG_GETARG_TEXT_PP(0);
	str = VARDATA_ANY(raw);
	len_str = VARSIZE_ANY_EXHDR(raw);

	raw = PG_GETARG_TEXT_PP(1);
	strlist = VARDATA_ANY(raw);

	arr = DatumGetArrayTypeP(
			DirectFunctionCall2Coll(text_to_array,
									PG_GET_COLLATION(),
									CStringGetTextDatum(strlist),
									CStringGetTextDatum(",")));

	deconstruct_array(arr, TEXTOID, -1, false, TYPALIGN_INT, 
					  &values, NULL, &nvalues);

	for (int i = 0; i < nvalues; i++)
	{
		char 	*target = VARDATA_ANY(values[i]);
		int 	 len_target = VARSIZE_ANY_EXHDR(values[i]);

		if (varstr_cmp(str, len_str, target, len_target, PG_GET_COLLATION()) == 0)
		{
			PG_RETURN_INT32(ret + 1);			
		}

		ret++;
	}
	
	PG_RETURN_INT32(ret);
}

Datum
hex_str(PG_FUNCTION_ARGS)
{
	text 	*raw = PG_GETARG_TEXT_PP(0);
	char	*str = VARDATA_ANY(raw);
	int 	 len = VARSIZE_ANY_EXHDR(raw);
	text 	*ret;
	StringInfoData buf;

	initStringInfo(&buf);

	for (int i = 0; i < len; i++)
	{
		appendStringInfo(&buf, "%02X", (uint8) str[i]);
	}
	
	ret = cstring_to_text(buf.data);

	pfree(buf.data);

	PG_RETURN_TEXT_P(ret);
}

Datum
hex_num(PG_FUNCTION_ARGS)
{
	text	*ret;
	char	 buf[64];
	
	snprintf(buf, sizeof(buf), "%lX", PG_GETARG_UINT64(0));

	ret = cstring_to_text(buf);

	PG_RETURN_TEXT_P(ret);
}

Datum
hex_bytea(PG_FUNCTION_ARGS)
{
	bytea 	*raw = PG_GETARG_BYTEA_PP(0);
	char	*data = VARDATA_ANY(raw);
	int 	 len = VARSIZE_ANY_EXHDR(raw);
	text 	*ret;
	StringInfoData buf;

	initStringInfo(&buf);

	for (int i = 0; i < len; i++)
	{
		appendStringInfo(&buf, "%02X", (uint8) data[i]);
	}
	
	ret = cstring_to_text(buf.data);

	pfree(buf.data);

	PG_RETURN_TEXT_P(ret);
}

Datum
locate(PG_FUNCTION_ARGS)
{	
	int		result = 0;
	int64	pos = 0;
	Datum 	substr = PG_GETARG_DATUM(0);
	text	*raw = PG_GETARG_TEXT_PP(1);
	char 	*str = VARDATA_ANY(raw);
	int 	 len = VARSIZE_ANY_EXHDR(raw);
	
	if (PG_NARGS() == 3)
	{
		pos = PG_GETARG_INT64(2);
		if (pos <= 0 || pos > len)
			PG_RETURN_INT32(0);

		// The third argument start from 1, we trans the position to array index
		pos--;
	}

	result = DatumGetInt32(DirectFunctionCall2Coll(textpos, 
												   PG_GET_COLLATION(), 
												   PointerGetDatum(cstring_to_text_with_len(str + pos, len - pos)), 
												   substr));

	// Add offset to the final result
	if (result != 0)
		result += pos;

	PG_RETURN_INT32(result);
}

Datum
myschar(PG_FUNCTION_ARGS)
{
	Datum	*args;
	Oid		*types;
	bool	*nulls;
	int		 nargs;
	StringInfoData buf;
	text 	*ret;

	initStringInfo(&buf);

	nargs = extract_variadic_args(fcinfo, 0, false, &args, &types, &nulls);

	for (int i = 0; i < nargs; ++i)
	{
		text *str;

		if (nulls[i])
			continue;
		
		str = DatumGetTextPP(DirectFunctionCall1(chr, args[i]));
		
		appendBinaryStringInfo(&buf, VARDATA_ANY(str), VARSIZE_ANY_EXHDR(str));
	}

	ret = cstring_to_text_with_len(buf.data, buf.len);
	pfree(buf.data);

	PG_RETURN_TEXT_P(ret);
}

/*
 * Return the code for the leftmost character if that character
 * is a multi-byte (sequence of one or more bytes) one
 */
Datum
ord(PG_FUNCTION_ARGS)
{
	text	   *str = PG_GETARG_TEXT_PP(0);
	const char *ptr = VARDATA_ANY(str);
	int			len = VARSIZE_ANY_EXHDR(str);
	uint64		currentNum = 1;
	uint64		result = 0;
	int			sz;

	if (len == 0)
		PG_RETURN_UINT64(0);

	sz = pg_mblen(ptr);
	ptr += sz - 1;
	for (int i = 0; i < sz; i++)
	{
		result += ((*ptr) & 0xff) * currentNum;
		currentNum <<= 8;
		ptr--;
	}

	PG_RETURN_UINT64(result);
}

Datum
space(PG_FUNCTION_ARGS)
{
	char   *str;
	text   *ret;
	int64   len = PG_GETARG_INT64(0);

	if (len <= 0)
		PG_RETURN_TEXT_P(cstring_to_text(""));

	if (len > max_allowed_packet)
	{
		ereport(WARNING,(errmsg("Result of space() was larger than max_allowed_packet (%d) - truncated.", max_allowed_packet)));
		PG_RETURN_NULL();
	}

	str = palloc0(len + 1);

	if (!str)
		PG_RETURN_NULL();

	memset(str, ' ', len);
	str[len] = '\0';
	ret = cstring_to_text(str);
	pfree(str);

	PG_RETURN_TEXT_P(ret);
}

/*
 * soundex
 */
Datum
soundex(PG_FUNCTION_ARGS)
{
	text 	   *raw = PG_GETARG_TEXT_PP(0);
	char	   *arg = VARDATA_ANY(raw);
	int			strLen = VARSIZE_ANY_EXHDR(raw) + 1;
	int			minSoundLen;
	char	   *outstr = NULL;
	text	   *ret;
	size_t		size;

	minSoundLen = pg_encoding_max_length(GetDatabaseEncoding()) + SOUND_THRESHOLD;
	
	size = Max(strLen, minSoundLen);

	outstr = (char *) palloc0(size);
	
	set_sound(arg, outstr, size);

	ret = cstring_to_text(outstr);

	pfree(outstr);
	
	PG_RETURN_TEXT_P(ret);
}


Datum
substring_index(PG_FUNCTION_ARGS)
{
	text 		*str = PG_GETARG_TEXT_PP(0);
	text 		*delim = PG_GETARG_TEXT_PP(1);
	int64		 count = PG_GETARG_INT64(2);
	int 		 nvalues, s, e;
	Datum  		*values;
	ArrayType 	*arr;
	StringInfoData  buf;
	text 	*ret;

	if (count == 0)
		PG_RETURN_TEXT_P(CStringGetTextDatum(""));

	if (count == INT64_MAX)
		count = -1;

	arr = DatumGetArrayTypeP(
			DirectFunctionCall2Coll(text_to_array,
									DEFAULT_COLLATION_OID, // substring_index is case-sensitive match when searching for delim.
									PointerGetDatum(str),
									PointerGetDatum(delim)));
	
	deconstruct_array(arr, TEXTOID, -1, false, TYPALIGN_INT,
					  &values, NULL, &nvalues);
	
	if (count > 0)
	{
		s = 0;
		e = count > nvalues ? nvalues : count;
	}
	else
	{
		s = nvalues + count;

		if(s < 0)
			s = 0;

		e = nvalues;
	}

	initStringInfo(&buf);
	
	for (int i = s; i < e; i++)
	{
		appendBinaryStringInfo(&buf, VARDATA_ANY(values[i]), VARSIZE_ANY_EXHDR(values[i]));
		if (i < e - 1)
			appendBinaryStringInfo(&buf, VARDATA_ANY(delim), VARSIZE_ANY_EXHDR(delim));
	}
												
	ret = cstring_to_text_with_len(buf.data, buf.len);
	pfree(buf.data);

	PG_RETURN_TEXT_P(ret);
}

Datum
insert(PG_FUNCTION_ARGS)
{
	text   *str = PG_GETARG_TEXT_PP(0);
	int64	pos = PG_GETARG_INT64(1); 	/* substring start position */
	int64	len = PG_GETARG_INT64(2);   /* substring length */
	text   *newstr = PG_GETARG_TEXT_PP(3);
	int		sp_pl_sl;
	text   *s1 = NULL;
	text   *s2 = NULL;
	text   *result = NULL;

	if (pos <= 0 || pos > text_length(PointerGetDatum(str)) || pg_add_s32_overflow(pos, len, &sp_pl_sl))
		PG_RETURN_TEXT_P(str);
	
	s1 = text_substring(PointerGetDatum(str), 1, pos - 1, false);
	s2 = text_substring(PointerGetDatum(str), sp_pl_sl, -1, true);
	result = text_catenate(s1, newstr);

	if (len >= 0)
		result = text_catenate(result, s2);

	PG_RETURN_TEXT_P(result);
}

Datum
mys_text_length(PG_FUNCTION_ARGS)
{
	Datum		str = PG_GETARG_DATUM(0);

	/* We need not detoast the input at all */
	PG_RETURN_INT32(toast_raw_datum_size(str) - VARHDRSZ);
}
