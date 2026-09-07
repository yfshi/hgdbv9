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

#ifdef WIN32
#pragma execution_character_set("utf-8")
#endif
 
#include "postgres.h"
#include "fmgr.h"

#include "access/detoast.h"
#include "common/base64.h"
#include "common/hex.h"
#include "common/int.h"
#include "catalog/namespace.h"
#include "catalog/pg_namespace.h"
#include "catalog/pg_collation.h"
#include "funcapi.h"
#include "lib/stringinfo.h"
#include "mb/pg_wchar.h"
#include "miscadmin.h"
#include "parser/parse_coerce.h"
#include "utils/builtins.h"
#include "utils/date.h"
#include "utils/datetime.h"
#include "utils/float.h"
#include "utils/formatting.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"
#include "utils/numeric.h"
#include "utils/pg_crc.h"
#include "utils/syscache.h"
#include "utils/timestamp.h"
#include "utils/varbit.h"
#include "utils/varlena.h"

#include "../charset/ctype.h"
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

static const double log_10[] = {
  1e000, 1e001, 1e002, 1e003, 1e004, 1e005, 1e006, 1e007, 1e008, 1e009,
  1e010, 1e011, 1e012, 1e013, 1e014, 1e015, 1e016, 1e017, 1e018, 1e019,
  1e020, 1e021, 1e022, 1e023, 1e024, 1e025, 1e026, 1e027, 1e028, 1e029,
  1e030, 1e031, 1e032, 1e033, 1e034, 1e035, 1e036, 1e037, 1e038, 1e039,
  1e040, 1e041, 1e042, 1e043, 1e044, 1e045, 1e046, 1e047, 1e048, 1e049,
  1e050, 1e051, 1e052, 1e053, 1e054, 1e055, 1e056, 1e057, 1e058, 1e059,
  1e060, 1e061, 1e062, 1e063, 1e064, 1e065, 1e066, 1e067, 1e068, 1e069,
  1e070, 1e071, 1e072, 1e073, 1e074, 1e075, 1e076, 1e077, 1e078, 1e079,
  1e080, 1e081, 1e082, 1e083, 1e084, 1e085, 1e086, 1e087, 1e088, 1e089,
  1e090, 1e091, 1e092, 1e093, 1e094, 1e095, 1e096, 1e097, 1e098, 1e099,
  1e100, 1e101, 1e102, 1e103, 1e104, 1e105, 1e106, 1e107, 1e108, 1e109,
  1e110, 1e111, 1e112, 1e113, 1e114, 1e115, 1e116, 1e117, 1e118, 1e119,
  1e120, 1e121, 1e122, 1e123, 1e124, 1e125, 1e126, 1e127, 1e128, 1e129,
  1e130, 1e131, 1e132, 1e133, 1e134, 1e135, 1e136, 1e137, 1e138, 1e139,
  1e140, 1e141, 1e142, 1e143, 1e144, 1e145, 1e146, 1e147, 1e148, 1e149,
  1e150, 1e151, 1e152, 1e153, 1e154, 1e155, 1e156, 1e157, 1e158, 1e159,
  1e160, 1e161, 1e162, 1e163, 1e164, 1e165, 1e166, 1e167, 1e168, 1e169,
  1e170, 1e171, 1e172, 1e173, 1e174, 1e175, 1e176, 1e177, 1e178, 1e179,
  1e180, 1e181, 1e182, 1e183, 1e184, 1e185, 1e186, 1e187, 1e188, 1e189,
  1e190, 1e191, 1e192, 1e193, 1e194, 1e195, 1e196, 1e197, 1e198, 1e199,
  1e200, 1e201, 1e202, 1e203, 1e204, 1e205, 1e206, 1e207, 1e208, 1e209,
  1e210, 1e211, 1e212, 1e213, 1e214, 1e215, 1e216, 1e217, 1e218, 1e219,
  1e220, 1e221, 1e222, 1e223, 1e224, 1e225, 1e226, 1e227, 1e228, 1e229,
  1e230, 1e231, 1e232, 1e233, 1e234, 1e235, 1e236, 1e237, 1e238, 1e239,
  1e240, 1e241, 1e242, 1e243, 1e244, 1e245, 1e246, 1e247, 1e248, 1e249,
  1e250, 1e251, 1e252, 1e253, 1e254, 1e255, 1e256, 1e257, 1e258, 1e259,
  1e260, 1e261, 1e262, 1e263, 1e264, 1e265, 1e266, 1e267, 1e268, 1e269,
  1e270, 1e271, 1e272, 1e273, 1e274, 1e275, 1e276, 1e277, 1e278, 1e279,
  1e280, 1e281, 1e282, 1e283, 1e284, 1e285, 1e286, 1e287, 1e288, 1e289,
  1e290, 1e291, 1e292, 1e293, 1e294, 1e295, 1e296, 1e297, 1e298, 1e299,
  1e300, 1e301, 1e302, 1e303, 1e304, 1e305, 1e306, 1e307, 1e308
};

#define SOUNDEX_LEN 4
#define SOUND_THRESHOLD 4

/*
 * Maximum number of digits required to represent a 64-bit unsigned integer
 * in binary (base 2), which is the longest possible representation needed
 * for base conversions. For example, UINT64_MAX in base 2 has 64 digits.
 */
#define MAX_BASE_CONV_DIGITS 64

				/* ABCDEFGHIJKLMNOPQRSTUVWXYZ */
				/* :::::::::::::::::::::::::: */
const char *soundex_map = "01230120022455012623010202";


PG_FUNCTION_INFO_V1(locate);
PG_FUNCTION_INFO_V1(insert);
PG_FUNCTION_INFO_V1(elt);
PG_FUNCTION_INFO_V1(field);
PG_FUNCTION_INFO_V1(field_any);  /*  BUG#Z5516 */
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
PG_FUNCTION_INFO_V1(conv);
PG_FUNCTION_INFO_V1(unhex);
PG_FUNCTION_INFO_V1(weight_string);
PG_FUNCTION_INFO_V1(weight_string_boolean);
PG_FUNCTION_INFO_V1(mys_strcmp);
PG_FUNCTION_INFO_V1(database);
PG_FUNCTION_INFO_V1(char_length_varlena); /* bug#z6050 */
PG_FUNCTION_INFO_V1(oct);
PG_FUNCTION_INFO_V1(number_to_sql_mode);
PG_FUNCTION_INFO_V1(double_truncate);
PG_FUNCTION_INFO_V1(numeric_truncate);
PG_FUNCTION_INFO_V1(int_truncate);
PG_FUNCTION_INFO_V1(crc32);
PG_FUNCTION_INFO_V1(double_sqrt);
PG_FUNCTION_INFO_V1(mys_sleep);

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


static bytea *
b64_decode_internal(const char *data, int dataLength)
{
	int		resultLength, res = 0;
	bytea  *result;

	resultLength = pg_b64_dec_len(dataLength);
	result = (bytea *)palloc(VARHDRSZ + resultLength);
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

Datum
bin_text(PG_FUNCTION_ARGS)
{
	text *result;
	Datum newval = PG_GETARG_DATUM(0);

	result = DatumGetTextP(DirectFunctionCall3(conv, newval, 10, 2));
	if (result)
		PG_RETURN_TEXT_P(result);
	else
		PG_RETURN_TEXT_P(cstring_to_text("0"));
}

Datum
elt(PG_FUNCTION_ARGS)
{
	int64	num = PG_GETARG_INT64(0);
	Datum	*args;
	Oid		*types;
	bool	*nulls;
	int		nargs;

	if (num < 1)
		PG_RETURN_NULL();

	nargs = extract_variadic_args(fcinfo, 1, false, &args, &types, &nulls);

	/* Begin - BUG#Z5513 */
	if (num > nargs)
		PG_RETURN_NULL();
	/* End - BUG#Z5513 */

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
	/* Begin - BUG#Z5516 */
	char 	*lower_left = NULL,*lower_right = NULL;
	/* End - BUG#Z5516 */
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

	/* Begin - BUG#Z5516 */
	if (nulls[0])
		PG_RETURN_INT32(0);
	/* End - BUG#Z5516 */

	raw = DatumGetTextP(args[0]);
	first_arg = VARDATA_ANY(raw);
	first_len = VARSIZE_ANY_EXHDR(raw);

	/* Begin - BUG#Z5516 */
	lower_left = asc_tolower(first_arg, first_len);
	/* End - BUG#Z5516 */

	for (int i = 1; i < nargs; i++)
	{
		/* Begin - BUG#Z5516 */
		if (nulls[i])
			continue;
		/* End - BUG#Z5516 */

		raw = DatumGetTextP(args[i]);
		match_arg = VARDATA_ANY(raw);
		match_len = VARSIZE_ANY_EXHDR(raw);

		if (match_arg == NULL)
			continue;

		/* Begin - BUG#Z5516 */
		lower_right = asc_tolower(match_arg, match_len);

		if (varstr_cmp(lower_left, first_len, lower_right, match_len, PG_GET_COLLATION()) == 0)
			PG_RETURN_INT64(i);
		/* End - BUG#Z5516 */
	}

	PG_RETURN_INT32(0);
}

/* Begin - BUG#Z5516 */
Datum
field_any(PG_FUNCTION_ARGS)
{
	Datum	*args;
	Oid		*types;
	bool	*nulls;
	int		 nargs;
	double 	arg0 = 0.0;
	double 	argi = 0.0;
	char 	*str = NULL;
	char 	*endptr = NULL;
	Oid		typeOutput;
	bool	typIsVarlena;

	if (PG_NARGS() < 2)
	{
		ereport(ERROR, (errcode(ERRCODE_UNDEFINED_PARAMETER),
			errmsg("Incorrect parameter count in the call to native function 'field'")));
	}

	nargs = extract_variadic_args(fcinfo, 0, false, &args, &types, &nulls);

	if (nulls[0])
		PG_RETURN_INT32(0);

	for (int i = 0; i < nargs;i++)
	{
		endptr = NULL;

		if (nulls[i])
			continue;

		switch(types[i])
		{
			case BOOLOID:
				if (i == 0)
					arg0 = args[i] ? 1 : 0;
				else
					argi = args[i] ? 1 : 0;
				break;

			default:
				getTypeOutputInfo(types[i], &typeOutput, &typIsVarlena);
				str = OidOutputFunctionCall(typeOutput, args[i]);

				if (i == 0)
				{
					arg0 = strtod(str, &endptr);
				}
				else
				{
					argi = strtod(str, &endptr);
				}

				/* The string contains only spaces */
				if (endptr == str && *endptr == ' ')
					break;

				if (*endptr != '\0')
				{
					ereport(WARNING,
						errmsg("Truncated incorrect DOUBLE value: %s", str));
				}
				break;
		}

		if (i >= 1 && arg0 == argi)
		{
			PG_RETURN_INT32(i);
		}
	}

	PG_RETURN_INT32(0);
}
/* End - BUG#Z5516 */

Datum
find_in_set(PG_FUNCTION_ARGS)
{
	char 		*str,
				*strlist,
				*lower_str,
				*lower_strlist;
	ArrayType 	*arr;
	Datum  		*values;
	int 		 nvalues;
	int			 i = 0,
				 len_str,
				 len_strlist;
	text		*raw;

	raw = PG_GETARG_TEXT_PP(0);
	str = VARDATA_ANY(raw);
	len_str = VARSIZE_ANY_EXHDR(raw);

	/* Begin - BUG#Z5520 */
	if (strchr(str, ',') != NULL)
		PG_RETURN_INT32(0);
	/* End - BUG#Z5520 */

	raw = PG_GETARG_TEXT_PP(1);
	strlist = VARDATA_ANY(raw);
	/* Begin - BUG#Z5519 */
	len_strlist = VARSIZE_ANY_EXHDR(raw);
	if (len_strlist == 0)
		PG_RETURN_INT32(0);
	/* End - BUG#Z5519 */

	lower_str = asc_tolower(str, len_str);
	lower_strlist = asc_tolower(strlist, len_strlist);
	arr = DatumGetArrayTypeP(
			DirectFunctionCall2Coll(text_to_array,
									PG_GET_COLLATION(),
									CStringGetTextDatum(lower_strlist),
									CStringGetTextDatum(",")));

	deconstruct_array(arr, TEXTOID, -1, false, TYPALIGN_INT, 
					  &values, NULL, &nvalues);

	for (i = 0; i < nvalues; i++)
	{
		char	*target = VARDATA_ANY(values[i]);
		int		 len_target = VARSIZE_ANY_EXHDR(values[i]);

		if (varstr_cmp(lower_str, len_str, target, len_target, PG_GET_COLLATION()) == 0)
		{
			pfree(lower_str);
			pfree(lower_strlist);
			PG_RETURN_INT32(i + 1);
		}
	}

	pfree(lower_str);
	pfree(lower_strlist);
	PG_RETURN_INT32(0);
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
	
	snprintf(buf, sizeof(buf), "%llX", (long long unsigned int) PG_GETARG_UINT64(0));

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
		/* Begin - BUG#Z5536 */
		uint32 uchar_code;
		
		if (nulls[i])
			continue;
	
		uchar_code = (uint32) DatumGetInt64(args[i]);

		if (uchar_code > 0xffffff)
			appendStringInfoChar(&buf, (char)((uchar_code >> 24) & 0xFF));

		if (uchar_code > 0xffff)
			appendStringInfoChar(&buf, (char)((uchar_code >> 16) & 0xFF));

		if (uchar_code > 0xff)
			appendStringInfoChar(&buf, (char)((uchar_code >> 8) & 0xFF));

		appendStringInfoChar(&buf, (char)(uchar_code & 0xFF));
		/* End - BUG#Z5536 */
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
 * If alpha, map input letter to soundex code.
 * If not alpha and remove_garbage is set then skip to next char
 * else return 0
 */
static int
soundex_toupper(int ch)
{
	return (ch >= 'a' && ch <= 'z') ? ch - 'a' + 'A' : ch;
}

static char
get_scode(int wc)
{
	int ch= soundex_toupper(wc);
	if (ch < 'A' || ch > 'Z')
	{
				// Thread extended alfa (country spec)
		return '0';				// as vokal
	}
	return(soundex_map[ch-'A']);
}

static bool
my_uni_isalpha(int wc)
{
	/*
	 * Return true for all Basic Latin letters: a..z A..Z.
	 * Return true for all Unicode characters with code higher than U+00C0:
	 * - characters between 'z' and U+00C0 are controls and punctuations.
	 * - "U+00C0 LATIN CAPITAL LETTER A WITH GRAVE" is the first letter after 'z'.
	 */
	return (wc >= 'a' && wc <= 'z') ||
		   (wc >= 'A' && wc <= 'Z') ||
		   (wc >= 0xC0);
}

/*
 * Soundex implementation using PostgreSQL multibyte APIs.
 *
 * input	   : UTF-8 (or database encoding) string
 * input_len : length in bytes
 * out_len   : length of result string (excluding NUL terminator)
 *
 * Returns a malloc'ed, NUL-terminated string (caller must free).
 */
static char *
_soundex_valstr(const char *input, size_t input_len, size_t *out_len)
{
	int			rc;
	my_wc_t		wc;
	char		last_ch = 0, ch = 0;
	uint32		nchars = 0;
	size_t		alloc_len = 0;
	size_t		result_len = 0;
	char		*buf = NULL;
	unsigned char *to;
	const unsigned char *from, *end;

	if (out_len)
		*out_len = 0;
	if (!input || input_len == 0)
		return NULL;

	/* Allocate output buffer: at least 4 characters (worst case multibyte) */
	alloc_len = input_len > (size_t)(4 * MAX_MULTIBYTE_CHAR_LEN)
				   ? input_len
				   : (size_t)(4 * MAX_MULTIBYTE_CHAR_LEN);
	buf = (char *) palloc(alloc_len + 1);
	if (!buf)
		return NULL;

	to = (unsigned char *) buf;

	from = (const unsigned char *) input;
	end = from + input_len;

	/* --- Find the first alphabetic character --- */
	for (;;)
	{
		pg_wchar tmp_wc;

		if (from >= end)
		{
			buf[0] = '\0';
			if (out_len)
				*out_len = 0;
			return buf;
		}

		rc = pg_mblen((const char *)from);  /* number of bytes for this char */
		pg_mb2wchar_with_len((const char *)from, &tmp_wc, rc);
		wc = (my_wc_t) tmp_wc;

		if (rc == 1 && isalpha(*from))
		{
			/* single-byte alphabetic */
			last_ch = get_scode((my_wc_t)(unsigned char)(*from));
			*to++ = (unsigned char) (char) soundex_toupper((my_wc_t)(unsigned char)(*from));
			from++;
			nchars = 1;
			break;
		}
		else
		{
			from += rc;
			if (my_uni_isalpha(wc))
			{
				pg_wchar tmp_wc2;
				int wrote;

				wc = soundex_toupper(wc);
				last_ch = get_scode(wc);
				tmp_wc2 = (pg_wchar) wc;
				wrote = pg_wchar2mb_with_len(&tmp_wc2, (char *)to, 1);
				if (wrote <= 0)
				{
					pfree(buf);
					return NULL;
				}
				to += wrote;
				nchars = 1;
				break;
			}
		}
	}

	/* --- Process remaining characters --- */
	for (;;)
	{
		pg_wchar tmp_wc;

		if (from >= end)
			break;

		rc = pg_mblen((const char *)from);
		pg_mb2wchar_with_len((const char *)from, &tmp_wc, rc);
		wc = (my_wc_t) tmp_wc;

		if (rc == 1 && isalpha(*from))
		{
			from++;
		}
		else
		{
			from += rc;
			if (!my_uni_isalpha(wc))
				continue;
		}

		ch = get_scode(wc);

		if ((ch != '0') && (ch != last_ch))
		{
			pg_wchar tmp_wc2 = (pg_wchar) ch;
			int wrote = pg_wchar2mb_with_len(&tmp_wc2, (char *)to, 1);
			if (wrote <= 0)
				break;
			to += wrote;
			nchars++;
			last_ch = ch;
		}
	}

	/* --- Pad with '0' if less than 4 characters --- */
	if (nchars < 4)
	{
		uint32 need = 4 - nchars;
		memset(to, '0', need);  /* always ASCII '0' */
		to += need;
	}

	result_len = (size_t)(to - (unsigned char *) buf);
	buf[result_len] = '\0';
	if (out_len)
		*out_len = result_len;

	return buf;
}

/*
 * soundex
 */
Datum
soundex(PG_FUNCTION_ARGS)
{
	text		*raw = PG_GETARG_TEXT_PP(0);
	char		*input = VARDATA_ANY(raw);
	int			 input_len = VARSIZE_ANY_EXHDR(raw) + 1;
	char		*outstr = NULL;
	text		*ret;
	size_t		 size = 0;

	outstr = _soundex_valstr(input, input_len, &size);
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


/**
 * Convert a character to its corresponding digit value.
 *
 * @param c Character to convert
 * @return Digit value (0-35) or -1 if invalid
 */
static int
char_to_digit(char c)
{
	if (c >= '0' && c <= '9') return c - '0';
	if (c >= 'a' && c <= 'z') return c - 'a' + 10;
	if (c >= 'A' && c <= 'Z') return c - 'A' + 10;
	return -1;
}

/**
 * Convert a digit value to its corresponding character representation.
 *
 * @param d Digit value (0-35)
 * @return Character representation or '?' if invalid
 */
static char
digit_to_char(int d)
{
	if (d >= 0 && d <= 9) return '0' + d;
	if (d >= 10 && d < 36) return 'A' + (d - 10);
	return '?';
}

/**
 * Convert numbers between different bases (2-36), with MySQL-compatible behavior.
 *
 * Function signature: conv(n text, from_base integer, to_base integer) returns text
 *
 * Features:
 * - Supports both signed (negative base) and unsigned (positive base) conversions
 * - Handles leading/trailing whitespace and explicit +/- signs
 * - Stops conversion at first invalid digit for given base
 * - Implements MySQL's uint64 wraparound behavior for overflow
 * - Returns NULL for invalid bases (outside 2-36 range) or NULL inputs
 *
 * Special cases:
 * - Negative bases indicate signed number conversion
 * - Overflowing values wrap to UINT64_MAX
 * - Empty/non-convertible strings return '0'
 * - INT64_MIN is handled specially to avoid overflow during conversion
 *
 * @param PG_FUNCTION_ARGS Function arguments structure containing:
 *   n_text: Input number as text
 *   from_base: Source base (absolute value 2-36, sign indicates signed/unsigned)
 *   to_base: Target base (absolute value 2-36, sign indicates signed/unsigned)
 * @return Converted number as text, or NULL for invalid inputs
 */
Datum
conv(PG_FUNCTION_ARGS)
{
	text *n_text;
	int from_base;
	int to_base;
	int base_abs = 0;
	int i;
	char *n_cstr;
	char *p;
	char *first_digit;
	uint64_t value = 0;
	uint64_t uval;
	uint64_t outval;
	bool negative = false;
	bool has_digits = false;
	bool overflow_to_zero = false;
	bool output_negative = false;
	bool overflowed = false;
	char buf[MAX_BASE_CONV_DIGITS+1];

	/* Return NULL if any input is NULL */
	if (PG_ARGISNULL(0) || PG_ARGISNULL(1) || PG_ARGISNULL(2))
		PG_RETURN_NULL();

	n_text = PG_GETARG_TEXT_PP(0);
	from_base = PG_GETARG_INT32(1);
	to_base = PG_GETARG_INT32(2);

	/* Validate base ranges (absolute value 2-36) */
	if (abs(from_base) < 2 || abs(from_base) > 36 ||
		abs(to_base) < 2 || abs(to_base) > 36)
		PG_RETURN_NULL();

	n_cstr = text_to_cstring(n_text);
	p = n_cstr;

	/* Begin - BUG#Z5506 */
	/* Return NULL if any input is empty string */
	if (*n_cstr == '\0')
		PG_RETURN_NULL();
	/* End - BUG#Z5506 */

	/* Skip leading whitespace */
	while (isspace((unsigned char)*p)) p++;

	/* Process sign character */
	negative = false;
	if (*p == '+' || *p == '-')
	{
		if (*p == '-') negative = true;
		p++;
	}

	/* Remember first digit position for overflow detection */
	first_digit = p;

	/* Parse the numeric value */
	base_abs = abs(from_base);  /* Absolute value of base */
	while (*p)
	{
		uint64_t new_value;
		int d = char_to_digit(*p);
		/* Stop at first invalid digit */
		if (d < 0 || d >= base_abs)
		{
			break;
		}
		has_digits = true;

		/* Check for multiplication overflow */
		if (value > UINT64_MAX / base_abs)
		{
			overflowed = true;
			break;
		}
		new_value = value * base_abs;

		/* Check for addition overflow */
		if (new_value > UINT64_MAX - d)
		{
			overflowed = true;
			break;
		}
		value = new_value + d;

		p++;
	}

	if (overflowed)
	{
		value = UINT64_MAX;  /* Set to max value */

		/* Skip remaining valid digits */
		while (*p)
		{
			int d = char_to_digit(*p);
			if (d < 0 || d >= base_abs)
			{
				break;
			}
			p++;
		}
	}

	/* Return '0' for strings with no valid digits */
	if (!has_digits)
	{
		pfree(n_cstr);
		PG_RETURN_VARCHAR_P(cstring_to_text("0"));
	}

	/* Detect overflow-to-zero: non-zero input parsed as zero */
	overflow_to_zero = (has_digits && value == 0 && *first_digit != '0');

	/* Unsigned input (from_base > 0) */
	if (from_base > 0)
	{
		if (overflow_to_zero || overflowed)
		{
			/* Set to max value on overflow */
			uval = UINT64_MAX;
		}
		else
		{
			if (negative)
			{
				/* Two's complement representation for negatives */
				uval = 0 - value;
			}
			else
			{
				/* Direct value for positives */
				uval = value;
			}
		}
	}
	/* Signed input (from_base < 0) */
	else
	{
		if (negative)
		{
			/* Two's complement for negatives */
			uval = 0 - value;
		}
		else
		{
			/* Direct value for positives */
			uval = value;
		}
	}

	/* Prepare output value */
	if (to_base < 0)
	{
		int64_t sval = (int64_t)uval;
		if (sval < 0)
		{
			output_negative = true;
			/* Special handling for INT64_MIN to avoid overflow */
			if (sval == INT64_MIN)
				outval = (uint64_t)INT64_MAX + 1;
			else
				outval = (uint64_t)(-sval);
		}
		else
		{
			outval = (uint64_t)sval;
		}
	}
	else
	{
		/* Unsigned output */
		outval = uval;
	}

	/* Short-circuit for zero output */
	if (outval == 0)
	{
		pfree(n_cstr);
		PG_RETURN_VARCHAR_P(cstring_to_text("0"));
	}

	/* Convert to target base */
	i = MAX_BASE_CONV_DIGITS;
	buf[i] = '\0';

	while (outval > 0 && i > 0)
	{
		int rem = outval % abs(to_base);
		buf[--i] = digit_to_char(rem);
		outval /= abs(to_base);
	}

	/* Add negative sign if needed */
	if (output_negative && i > 0)
		buf[--i] = '-';

	pfree(n_cstr);
	PG_RETURN_VARCHAR_P(cstring_to_text(&buf[i]));
}

/*
 * Function: unhex
 *
 * Description:
 *   Converts a hexadecimal-encoded string into its binary representation.
 *   For a given input string `str`, UNHEX(str) interprets each consecutive
 *   pair of hexadecimal characters as a byte and returns the resulting
 *   binary string.
 *
 * Behavior:
 *   - The input string must consist of valid hexadecimal digits:
 *     characters '0'..'9', 'A'..'F', or 'a'..'f'.
 *   - The input length must be even; each pair of characters is decoded
 *     into one byte.
 *   - If the input contains any non-hexadecimal characters or the length
 *     is odd, the function returns NULL.
 *
 * Parameters:
 *   - str: A text string containing an even number of valid hexadecimal digits.
 *
 * Returns:
 *   - A binary string (bytea) representing the decoded bytes,
 *     or NULL if the input is invalid.
 */
Datum
unhex(PG_FUNCTION_ARGS)
{
	text       *input_text;
	char       *input_str;
	size_t      input_len;
	bytea      *result;
	char       *result_data;
	char       *p;
	size_t      result_len;
	size_t      i = 0;
	bool        odd_length = false;

	if (PG_ARGISNULL(0))
	PG_RETURN_NULL();

	input_text = PG_GETARG_TEXT_PP(0);
	input_str = text_to_cstring(input_text);
	input_len = strlen(input_str);

	/* Empty string handling */
	if (input_len == 0)
	{
		pfree(input_str);
		result = (bytea *) palloc(VARHDRSZ);
		SET_VARSIZE(result, VARHDRSZ);
		PG_RETURN_BYTEA_P(result);
	}

	/* Handle illegal characters */
	for (i = 0; i < input_len; i++)
	{
		if (!isxdigit((unsigned char) input_str[i]))
		{
			elog(WARNING, "Incorrect string value: ''%s'' for function unhex", input_str);
			pfree(input_str);
			PG_RETURN_NULL();
		}
	}

	odd_length = input_len % 2;

	/* Allocate result memory */
	result_len = odd_length ? ((input_len + 1)/2) : (input_len / 2);
	result = (bytea *) palloc(VARHDRSZ + result_len);
	SET_VARSIZE(result, VARHDRSZ + result_len);
	result_data = VARDATA(result);

	p = input_str;
	if (odd_length)
	{
		*result_data++ = mys_get_hex(p++);
		input_len--;
	}
	mys_hex_decode(p, input_len, result_data);

	pfree(input_str);
	PG_RETURN_BYTEA_P(result);
}

/*
 * Function: weight_string
 *
 * Description:
 *   This function implements the core logic for the MySQL-compatible
 *   WEIGHT_STRING() expression, which is defined in the parser (mys_gram.y)
 *   as:
 *     WEIGHT_STRING(str [AS {CHAR|BINARY}(N)] [LEVEL levels] [flags])
 *
 *   The parameters are passed as:
 *     - str:      the input string expression
 *     - levels:   weight level or -1
 *     - type:     target type after AS (CHAR or BINARY)
 *     - length:   length N specified after CHAR(N) or BINARY(N)
 *
 * Behavior:
 *   - If type is BINARY:
 *     - Validate the string length and weight level compatibility.
 *     - Output the raw byte representation of the input string (as bytea).
 *
 *   - If type is CHAR:
 *     - Validate the string length and weight level compatibility.
 *     - Determine the maximum output length based on collation, string length, and weight levels.
 *     - Process the input string according to collation rules and the specified flags.
 *     - Output the processed string as bytea.
 */
Datum
weight_string(PG_FUNCTION_ARGS)
{
	#define SINGLE_BYTE_LENGTH 1

	bytea *result;
	text *input;
	char * input_str;
	size_t input_len;
	size_t flags;
	size_t type;
	size_t nweights;
	int err_level;

	/* Begin - bug#z6009 */
	if (PG_ARGISNULL(0))
	{
		result = (bytea *)palloc0(VARHDRSZ);
		SET_VARSIZE(result, VARHDRSZ);
		PG_RETURN_BYTEA_P(result);
	}
	/* End - bug#z6009 */

	input = PG_GETARG_TEXT_PP(0);
	input_str = VARDATA_ANY(input);
	input_len = VARSIZE_ANY_EXHDR(input);

	flags = PG_GETARG_UINT32(1);
	type = PG_GETARG_UINT32(2);
	nweights = PG_GETARG_UINT32(3);
	err_level = is_strict_mode() ? ERROR : WARNING;

	if (type == WEIGHT_STRING_AS_UNUSE)
		nweights = input_len;

	if (type == WEIGHT_STRING_AS_BINARY)
	{
		if (Max(nweights, input_len) > max_allowed_packet)
		{
			ereport(err_level, (errmsg("weight_string output too large")));
			PG_RETURN_NULL();
		}

		if (input_len > nweights)
			ereport(WARNING,
					errmsg("Truncated incorrect BINARY(%lu) value: '%.*s'", nweights, (int32) input_len, input_str));

		result = (bytea *)palloc0(VARHDRSZ + nweights + SINGLE_BYTE_LENGTH);
		memcpy((char *)VARDATA(result), input_str, input_len);
		SET_VARSIZE(result, VARHDRSZ + nweights);
		PG_RETURN_BYTEA_P(result);
	}
	else if (type == WEIGHT_STRING_AS_CHAR ||
			 type == WEIGHT_STRING_AS_UNUSE)
	{
		char *result_data;
		size_t actual_len, max_output_len;

		max_output_len = do_mysql_strnxfrmlen(input_len, nweights, PG_GET_COLLATION());
		if (max_output_len > max_allowed_packet)
		{
			ereport(err_level, (errmsg("weight_string output too large")));
			PG_RETURN_NULL();
		}

		result = (bytea *)palloc0(VARHDRSZ + max_output_len);
		result_data = (char *)VARDATA(result);
		/* Begin - bug#z6010 */
		if (type == WEIGHT_STRING_AS_UNUSE)
			actual_len = do_mysql_strnxfrm((unsigned char*)result_data, max_output_len, nweights,
										   (unsigned char*)input_str, input_len,
										   flags, PG_GET_COLLATION());
		else
			actual_len = do_mysql_strnxfrm((unsigned char*)result_data, max_output_len, nweights,
										   (unsigned char*)input_str, input_len,
										   flags | MY_STRXFRM_PAD_WITH_SPACE, PG_GET_COLLATION());
		/* End - bug#z6010 */
		SET_VARSIZE(result, VARHDRSZ + actual_len);

		PG_RETURN_BYTEA_P(result);
	}
	else
	{
		ereport(err_level,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					errmsg("weight_string function does not support syntax other than 'AS CHAR' and 'AS BINARY'.")));
	}

	PG_RETURN_NULL();
}

/*
 * Function: weight_string_boolean
 *
 * Description:
 *   This function implements the core logic for the MySQL-compatible
 *   WEIGHT_STRING() expression, which is defined in the parser (mys_gram.y)
 *   as:
 *     WEIGHT_STRING(str [AS {CHAR|BINARY}(N)] [LEVEL levels] [flags])
 *
 *   The parameters are passed as:
 *     - str:      the input string expression
 *     - levels:   weight level or -1
 *     - type:     target type after AS (CHAR or BINARY)
 *     - length:   length N specified after CHAR(N) or BINARY(N)
 *
 * Behavior:
 *   - If type is BINARY:
 *     - Validate the string length and weight level compatibility.
 *     - Output the raw byte representation of the input string (as bytea).
 *
 *   - If type is Others:
 *     - Return NULL.
 */
Datum
weight_string_boolean(PG_FUNCTION_ARGS)
{
	size_t type = PG_GETARG_UINT32(2);

	if (type == WEIGHT_STRING_AS_BINARY)
	{
		bool istrue = PG_GETARG_BOOL(0);
		size_t data_len = PG_GETARG_UINT32(3);
		bytea *result = (bytea *)palloc0(VARHDRSZ + data_len);
		*((char *)VARDATA(result)) = istrue ? '1' : '0';
		SET_VARSIZE(result, VARHDRSZ + data_len);
		PG_RETURN_BYTEA_P(result);
	}
	else
	{
		PG_RETURN_NULL();
	}
}

/*
 * Function: mys_strcmp
 *
 * Description:
 *   Compares two string expressions `expr1` and `expr2` according to their
 *   collation rules.
 *
 * Behavior:
 *   - Returns 0 if the two strings are equal.
 *   - Returns -1 if expr1 is less than expr2 according to the current collation.
 *   - Returns 1 if expr1 is greater than expr2 according to the current collation.
 *
 * Notes:
 *   - The comparison respects the collation of the input arguments, which may
 *     affect case sensitivity, accent sensitivity, and language-specific rules.
 *   - NULL inputs are not allowed and should be handled separately if needed.
 */
Datum
mys_strcmp(PG_FUNCTION_ARGS)
{
	int32 ret = 0;
	text *dt0 = PG_GETARG_TEXT_PP(0);
	text *dt1 = PG_GETARG_TEXT_PP(1);
	char *str0 = VARDATA_ANY(dt0);
	char *str1 = VARDATA_ANY(dt1);
	size_t len0 = VARSIZE_ANY_EXHDR(dt0);
	size_t len1 = VARSIZE_ANY_EXHDR(dt1);

	ret = varstr_cmp(str0, len0, str1, len1, PG_GET_COLLATION());

	if (ret > 0)
		ret = 1;
	else if (ret < 0)
		ret = -1;
	else
		ret = 0;

	PG_RETURN_INT32(ret);
}

/* Begin - Req: SRS-MYSQL-PROTOCOL */
Datum
database(PG_FUNCTION_ARGS)
{
	List	   *search_path = fetch_search_path(false);
	char	   *nspname;

	if (search_path == NIL)
		PG_RETURN_NULL();

	nspname = get_namespace_name(linitial_oid(search_path));

	list_free(search_path);

	if (!nspname)
		PG_RETURN_NULL();		/* recently-deleted namespace? */

	PG_RETURN_DATUM(CStringGetTextDatum(nspname));
}
/* End - Req: SRS-MYSQL-PROTOCOL */

/* Begin - bug#z6050 */
Datum
char_length_varlena(PG_FUNCTION_ARGS)
{
	text *src = PG_GETARG_TEXT_PP(0);

	PG_RETURN_INT32(VARSIZE_ANY_EXHDR(src));
}
/* End - bug#z6050 */

Datum
oct(PG_FUNCTION_ARGS)
{
	text *result;
	text *input;
	Datum newval;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	input = PG_GETARG_TEXT_PP(0);
	if (VARSIZE_ANY_EXHDR(input) == 0)
		PG_RETURN_TEXT_P(cstring_to_text("0"));

	newval = PointerGetDatum(input);
	result = DatumGetTextPP(DirectFunctionCall3(conv, newval, 10, 8));
	if (result)
		PG_RETURN_TEXT_P(result);
	else
		PG_RETURN_TEXT_P(cstring_to_text("0"));
}

/* Begin - ReqID: SRS-MYSQL-PROC */
Datum
number_to_sql_mode(PG_FUNCTION_ARGS)
{
	sql_mode_t 		input = PG_GETARG_INT64(0);
	sql_mode_t 		it;
	const char 		**name;
	StringInfoData 	buf;

	initStringInfo(&buf);

	for (it = MODE_VALUES_FIRST, name = sql_mode_names;
		 *name != NULL; it = NEXT_MODE_VALUE(it), name++)
	{
		if (input & it)
		{
			if (buf.len != 0)
				appendStringInfoChar(&buf, ',');

			appendStringInfoString(&buf, *name);

			input &= ~it;
		}
	}

	if (input)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("illegal argument for function")));
	}

	PG_RETURN_TEXT_P(cstring_to_text(buf.data));
}
/* End - ReqID: SRS-MYSQL-PROC */

/* Begin - ReqID: SRS-MYSQL-BUILDFUNCTION */
static double
compute_pow10(int32 abs_dec)
{
	if (abs_dec < (int32) lengthof(log_10))
		return log_10[abs_dec];
	else
		return pow(10.0, (double) abs_dec);
}

Datum
double_truncate(PG_FUNCTION_ARGS)
{
	double		value = PG_GETARG_FLOAT8(0);
	int32		dec = PG_GETARG_INT32(1);
	double		res;
	int32		abs_dec;
	bool		dec_negative;
	double		tmp;
	double		value_div_tmp;
	double		value_mul_tmp;

	dec_negative = (dec < 0);
	abs_dec		 = dec_negative ? -dec : dec;

	tmp = compute_pow10(abs_dec);
	value_div_tmp = value / tmp;
	value_mul_tmp = value * tmp;

	if (dec_negative && isinf(tmp))
		res = 0.0;
	else if (!dec_negative && (isinf(value_mul_tmp) || isnan(value_mul_tmp)))
		res = value;
	else
	{
		if (value >= 0.0)
		{
			if (dec < 0)
				res = floor(value_div_tmp) * tmp;
			else
				res = floor(value_mul_tmp) / tmp;
		}
		else
		{
			if (dec < 0)
				res = ceil(value_div_tmp) * tmp;
			else 
				res = ceil(value_mul_tmp) / tmp;
		}
	}

	PG_RETURN_FLOAT8(res);
}

Datum
numeric_truncate(PG_FUNCTION_ARGS)
{
	Numeric		num = PG_GETARG_NUMERIC(0);
	int32		scale = PG_GETARG_INT32(1);

	/*
	 * Handle NaN and infinities
	 */
	if (numeric_is_inf(num) || numeric_is_nan(num))
		PG_RETURN_NUMERIC(num);

	scale = Min(scale, 30);

	return DirectFunctionCall2(numeric_trunc,
							   PG_GETARG_DATUM(0),
							   Int32GetDatum(scale));
}

Datum
int_truncate(PG_FUNCTION_ARGS)
{
	int64	val = PG_GETARG_INT64(0);
	int32	scale = PG_GETARG_INT32(1);
	int64	res;

	if (scale >= 0)
		PG_RETURN_INT64(val);
	else
	{
		int32 n = -scale;
		int64 pow10 = 1;

		for (int i = 0; i < n; i++)
		{
			if(pow10 > PG_INT64_MAX / 10)
				PG_RETURN_INT64(0);
			else
				pow10 *= 10;
		}
	
		res = (val / pow10) * pow10;
		PG_RETURN_INT64(res);
	}
}

Datum
crc32(PG_FUNCTION_ARGS)
{
    bytea       *in = PG_GETARG_BYTEA_PP(0);
    pg_crc32    crc;

    INIT_TRADITIONAL_CRC32(crc);
    COMP_TRADITIONAL_CRC32(crc, VARDATA_ANY(in), VARSIZE_ANY_EXHDR(in));
    FIN_TRADITIONAL_CRC32(crc);

	PG_RETURN_UINT32(crc);
}

Datum
double_sqrt(PG_FUNCTION_ARGS)
{
	float8		arg1 = PG_GETARG_FLOAT8(0);
	float8		result;

	if (arg1 < 0)
		PG_RETURN_NULL();

	if (arg1 >= 1e82)
	{	
		ereport(WARNING, (errmsg("Truncated incorrect DECIMAL value.")));
		arg1 = 1e65;
	}

	result = sqrt(arg1);

	if (unlikely(isinf(result)) && !isinf(arg1))
		float_overflow_error();
	if (unlikely(result == 0.0) && arg1 != 0.0)
		float_underflow_error();

	PG_RETURN_FLOAT8(result);
}

Datum
mys_sleep(PG_FUNCTION_ARGS)
{
	float8		secs;
	bool		interrupted = false;

	if (PG_ARGISNULL(0) ||
		(secs = PG_GETARG_FLOAT8(0)) < 0)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("Incorrect arguments to sleep.")));

#define MYS_SLEEP_SECS_MAX ((float8) 18446744073709551616.0 / 1e9)

	if (secs >= MYS_SLEEP_SECS_MAX)
		PG_RETURN_INT64(0);

	PG_TRY();
	{
		DirectFunctionCall1(pg_sleep, Float8GetDatum(secs));
	}
	PG_CATCH();
	{
		interrupted = true;
		FlushErrorState();
	}
	PG_END_TRY();

	PG_RETURN_INT64(interrupted ? 1 : 0);
}
/* End - ReqID: SRS-MYSQL-BUILDFUNCTION */