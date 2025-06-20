/*-------------------------------------------------------------------------
 *
 * varbinary.c
 *   for mysql varbinary type
 *
 * Portions Copyright (c) 2025 - , Highgo MySQL Compatible Development Group
 *
 * IDENTIFICATION
 *   contrib/ivorysql_my/src/datatype/binary/varbinary.c
 *
 * add the file for requirement "SRS-MYSQL-BINARYTYPE"
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "anybinary.h"
#include "catalog/pg_collation.h"
#include "catalog/pg_type.h"
#include "fmgr.h"
#include "parser/parse_oper.h"
#include "libpq/pqformat.h"
#include "lib/stringinfo.h"
#include <limits.h>
#include "miscadmin.h"
#include "strtod.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/bytea.h"
#include "utils/elog.h"
#include "utils/fmgrprotos.h"
#include "utils/guc.h"
#include "utils/numeric.h"
#include "utils/sortsupport.h"
#include "utils/varlena.h"
#include "../include/ivorysql_my.h"

PG_FUNCTION_INFO_V1(varbinary);
PG_FUNCTION_INFO_V1(varbinary2);
PG_FUNCTION_INFO_V1(varbinary2bool);
/* Begin BUG#5048 5070 */
PG_FUNCTION_INFO_V1(varbinary2int2);
PG_FUNCTION_INFO_V1(varbinary2int4);
PG_FUNCTION_INFO_V1(varbinary2int8);
/* End BUG#5048 5070 */
PG_FUNCTION_INFO_V1(varbinary_in);
PG_FUNCTION_INFO_V1(varbinary_recv);
PG_FUNCTION_INFO_V1(varbinary_and);
PG_FUNCTION_INFO_V1(varbinary_not);
PG_FUNCTION_INFO_V1(varbinary_or);
PG_FUNCTION_INFO_V1(varbinary_bitand);
PG_FUNCTION_INFO_V1(varbinary_bitor);
PG_FUNCTION_INFO_V1(varbinary_bitxor);
PG_FUNCTION_INFO_V1(varbinary_bitneg);
PG_FUNCTION_INFO_V1(varbinary_bitshiftright);
PG_FUNCTION_INFO_V1(varbinary_bitshiftleft);
PG_FUNCTION_INFO_V1(varbinary_sortsupport);

typedef long long int longlong;
typedef unsigned long long int ulonglong;

// reference from varchar.c function varchar()
static Datum
varbinary_internel(FunctionCallInfo fcinfo, bool isPg2My)
{
	int	 	len, maxlen;
	bytea 	*source = PG_GETARG_BYTEA_PP(0);
	int32 	typmod = PG_GETARG_INT32(1);
	bool 	isExplicit = PG_GETARG_BOOL(2);

	if (isExplicit && !isPg2My)
	{
		ereport(ERROR,
				(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
				 errmsg("ERROR 1064 (42000): You have an error in your SQL syntax; check the cast")));
	}

	len = VARSIZE_ANY_EXHDR(source);
	maxlen = typmod - VARHDRSZ;

	if (maxlen < 0 || len <= maxlen)
		PG_RETURN_BYTEA_P(source);

	if (is_strict_mode())
	{
		ereport(ERROR,
				(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
				 errmsg("value too long for type varbinary(%d)",
						maxlen)));
	}

	ereport(WARNING,
			(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
			 errmsg("value too long for type varbinary(%d), data truncated",
					maxlen)));

	PG_RETURN_BYTEA_P(truncate_bytea_with_len(source, maxlen));
}

Datum 
varbinary(PG_FUNCTION_ARGS)
{
	return varbinary_internel(fcinfo, false);
}

Datum 
varbinary2(PG_FUNCTION_ARGS)
{
	return varbinary_internel(fcinfo, true);
}

Datum 
varbinary2bool(PG_FUNCTION_ARGS)
{
	bytea 	*arg1;
	char 	*end1;
	int 	len1;
	int 	error;
	bool	ret;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	arg1 = PG_GETARG_BYTEA_PP(0);

	len1 = VARSIZE_ANY_EXHDR(arg1);
	end1 = VARDATA_ANY(arg1) + len1;

	ret = (my_strtod_8bit(VARDATA_ANY(arg1), len1, &end1, &error) != 0.0);

	PG_RETURN_BOOL(ret);
}

/* Begin BUG#5048 5070 */
Datum 
varbinary2int2(PG_FUNCTION_ARGS)
{
	bytea 	*arg;
	char	*data;
	int		len,i;
	uint64	value = 0;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	arg = PG_GETARG_BYTEA_PP(0);
	data = VARDATA_ANY(arg);
	len = VARSIZE_ANY_EXHDR(arg);

	if (len > sizeof(int16))
	{
		if (!is_strict_mode())
		{
			ereport(WARNING,
				(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
					errmsg("BINARY value is out of range for type smallint")));
			PG_RETURN_INT16(PG_INT16_MAX);
		}
		else
		{
			ereport(ERROR,
				(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
					errmsg("BINARY value is out of range for type smallint")));
		}
	}

	for (i = 0; i < len; i++)
	{
		value = (value << 8) + (uint64)(unsigned char)data[i];
	}

	if (value > PG_INT16_MAX)
	{
		if (!is_strict_mode())
		{
			ereport(WARNING,
				(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
					errmsg("BINARY value is out of range for type smallint")));
			PG_RETURN_INT16(PG_INT16_MAX);
		}
		else
		{
			ereport(ERROR,
				(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
					errmsg("BINARY value is out of range for type smallint")));
		}
	}

	PG_RETURN_INT16(value);
}

Datum 
varbinary2int4(PG_FUNCTION_ARGS)
{
	bytea	*arg;
	char	*data;
	int	len,i;
	uint64	value = 0;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	arg = PG_GETARG_BYTEA_PP(0);
	data = VARDATA_ANY(arg);
	len = VARSIZE_ANY_EXHDR(arg);

	if (len > sizeof(int32))
	{
		if (!is_strict_mode())
		{
			ereport(WARNING,
				(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
					errmsg("BINARY value is out of range for type integer")));
			PG_RETURN_INT32(PG_INT32_MAX);
		}
		else
		{
			ereport(ERROR,
				(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
					errmsg("BINARY value is out of range for type integer")));
		}
	}

	for (i = 0; i < len; i++)
	{
		value = (value << 8) + (uint64)(unsigned char)data[i];
	}

	if (value > PG_INT32_MAX)
	{
		if (!is_strict_mode())
		{
			ereport(WARNING,
				(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
					errmsg("BINARY value is out of range for type integer")));
			PG_RETURN_INT32(PG_INT32_MAX);
		}
		else
		{
			ereport(ERROR,
				(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
					errmsg("BINARY value is out of range for type integer")));
		}
	}

	PG_RETURN_INT32(value);
}

Datum 
varbinary2int8(PG_FUNCTION_ARGS)
{
	bytea 	*arg;
	unsigned char	*data;
	int		len,i;
	uint64	value = 0;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	arg = PG_GETARG_BYTEA_PP(0);
	data = VARDATA_ANY(arg);
	len = VARSIZE_ANY_EXHDR(arg);

	if (len > sizeof(int64))
	{
		if (!is_strict_mode())
		{
			ereport(WARNING,
				(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
					errmsg("BINARY value is out of range for type bigint")));
			PG_RETURN_INT64(PG_INT64_MAX);
		}
		else
		{
			ereport(ERROR,
				(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
					errmsg("BINARY value is out of range for type bigint")));
		}
	}

	for (i = 0; i < len; i++)
	{
		value = (value << 8) + (uint64)(unsigned char)data[i];
	}

	if (value > PG_INT64_MAX)
	{
		if (!is_strict_mode())
		{
			ereport(WARNING,
				(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
					errmsg("BINARY value is out of range for type bigint")));
			PG_RETURN_INT64(PG_INT64_MAX);
		}
		else
		{
			ereport(ERROR,
				(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
					errmsg("BINARY value is out of range for type bigint")));
		}
	}

	PG_RETURN_INT64(value);
}
/* End BUG#5048 5070 */

// reference from varchar.c function varcharin() and varlena.c function byteain()
Datum 
varbinary_in(PG_FUNCTION_ARGS)
{
	char 	*tp, 
			*rp,
			*inputText = PG_GETARG_CSTRING(0);
	int 	bc, 
			zero_cnt = 0,
			atttypmod = PG_GETARG_INT32(2);
	size_t 	len = strlen(inputText),
			maxlen = atttypmod - VARHDRSZ,
			maxmblen = pg_mbcharcliplen(inputText, len, maxlen); // actual save length
	bytea 	*result;
	
	if (atttypmod >= (int32)VARHDRSZ && len > maxlen)
	{
		if (is_strict_mode())
		{
			ereport(ERROR,
					(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
					 errmsg("value too long for type varbinary(%lu)",
							maxlen)));
			PG_RETURN_VOID();
		}
		else
		{
			ereport(WARNING,
					(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
					 errmsg("value too long for type varbinary(%ld), data truncated",
							maxlen)));
		}
	}

	result = cstring_to_text_with_len(inputText, maxmblen);

	/* Recognize hex input */
	if (inputText[0] == '\\' && inputText[1] == 'x')
	{
		bc = (len - 2) / 2 + VARHDRSZ; /* maximum possible length */
		result = palloc(bc);
		bc = hex_decode(inputText + 2, len - 2, VARDATA(result));
		SET_VARSIZE(result, bc + VARHDRSZ); /* actual length */

		PG_RETURN_BYTEA_P(result);
	}

	/* Else, it's the traditional escaped style */
	for (bc = 0, tp = inputText; *tp != '\0'; bc++)
	{
		if (tp[0] != '\\')
			tp++;
		else if ((tp[0] == '\\') &&
				 (tp[1] >= '0' && tp[1] <= '3') &&
				 (tp[2] >= '0' && tp[2] <= '7') &&
				 (tp[3] >= '0' && tp[3] <= '7'))
			tp += 4;
		else if ((tp[0] == '\\') &&
				 (tp[1] == '\\'))
			tp += 2;
		else if (enable_character_blob &&
				 (tp[0] == '\\') && (tp[1] == '0') &&
				 ((tp[2] == '\\') || (tp[2] == '\0')))
		{
			tp += 2;
			zero_cnt += 2;
		}
		else
		{
			/*
			 * one backslash, not followed by another or ### valid octal
			 */
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
					 errmsg("invalid input syntax for type %s", "bytea")));
		}
	}

	bc += VARHDRSZ;
	bc += zero_cnt;

	result = (bytea *)palloc(bc);
	SET_VARSIZE(result, bc - zero_cnt);

	tp = inputText;
	rp = VARDATA(result);
	while (*tp != '\0')
	{
		if (tp[0] != '\\')
			*rp++ = *tp++;
		else if ((tp[0] == '\\') &&
				 (tp[1] >= '0' && tp[1] <= '3') &&
				 (tp[2] >= '0' && tp[2] <= '7') &&
				 (tp[3] >= '0' && tp[3] <= '7'))
		{
			bc = VAL(tp[1]);
			bc <<= 3;
			bc += VAL(tp[2]);
			bc <<= 3;
			*rp++ = bc + VAL(tp[3]);

			tp += 4;
		}
		else if ((tp[0] == '\\') &&
				 (tp[1] == '\\'))
		{
			*rp++ = '\\';
			tp += 2;
		}
		else if (enable_character_blob &&
				 (tp[0] == '\\') && (tp[1] == '0') &&
				 ((tp[2] == '\\') || (tp[2] == '\0')))
		{
			bc = VAL(tp[1]);
			bc <<= 3;
			bc += VAL(tp[1]);
			bc <<= 3;
			*rp++ = bc + VAL(tp[1]);

			tp += 4;
		}
		else
		{
			/*
			 * We should never get here. The first pass should not allow it.
			 */
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
					 errmsg("invalid input syntax for type %s", "bytea")));
		}
	}

	PG_RETURN_BYTEA_P(result);
}

/* Begin BUG#5048 5070 */
bytea * 
hexstring_varbinary(const char *value)
{
	bytea		*result;
	char		*new_value;
	char		*rp;
	int 		bc;
	size_t 	len, maxlen, maxmblen;

	size_t new_length = strlen(value) + 3; // '\\' 'x' '\0'
	rp = new_value = palloc(new_length);
	
	if (new_value == NULL)
	{
		fprintf(stderr, "Memory allocation failed\n");
		exit(EXIT_FAILURE);
	}

	*rp++ = '\\';
	*rp++ = 'x';
	*rp++ = (uint64)value;

	len = strlen(value);
	maxlen = -1 - VARHDRSZ;
	maxmblen = pg_mbcharcliplen(new_value, len + 2, maxlen); // actual save length

	result = cstring_to_text_with_len(new_value, maxmblen);

	bc = len / 2 + VARHDRSZ; /* maximum possible length */
	result = palloc(bc);
	bc = hex_decode(value, len, VARDATA(result));
	SET_VARSIZE(result, bc + VARHDRSZ); /* actual length */

	return result;
}
/* End BUG#5048 5070 */

// reference from varchar.c function varcharrecv() and varlena.c function bytearecv()
Datum 
varbinary_recv(PG_FUNCTION_ARGS)
{
	StringInfo 		buf = (StringInfo) PG_GETARG_POINTER(0);
	int 			atttypmod = PG_GETARG_INT32(2);
	int 			len = buf->len - buf->cursor;
	size_t 			maxlen = atttypmod - VARHDRSZ;
	size_t 			mbmaxlen;
	const char 		*inputText = pq_getmsgbytes(buf, len);
	bytea 			*result;

	result = (bytea *)palloc(len + VARHDRSZ);
	SET_VARSIZE(result, len + VARHDRSZ);

	if (atttypmod >= (int32)VARHDRSZ && len > maxlen)
	{
		if (is_strict_mode())
		{
			ereport(ERROR,
					(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
					 errmsg("value too long for type varbinary(%lu)",
							maxlen)));
			PG_RETURN_VOID();
		}
		else
		{
			ereport(WARNING,
					(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
					 errmsg("value too long for type varbinary(%ld), data truncated",
							maxlen)));
		}
	}

	mbmaxlen = pg_mbcharcliplen(inputText, len, maxlen);
	result = cstring_to_text_with_len(inputText, mbmaxlen);

	PG_RETURN_BYTEA_P(result);
}

/*****************************************************************************
 *	 Operator Function														 *
 *****************************************************************************/

Datum 
varbinary_and(PG_FUNCTION_ARGS)
{
	int 	len1,
			len2,
			error;
	char 	*end1,
			*end2;
	bytea 	*arg1, *arg2;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		PG_RETURN_NULL();

	arg1 = PG_GETARG_BYTEA_PP(0);
	arg2 = PG_GETARG_BYTEA_PP(1);

	len1 = VARSIZE_ANY_EXHDR(arg1);
	len2 = VARSIZE_ANY_EXHDR(arg2);

	end1 = VARDATA_ANY(arg1) + len1;
	end2 = VARDATA_ANY(arg1) + len2;

	return ((my_strtod_8bit(VARDATA_ANY(arg1), len1, &end1, &error) != 0.0) && (my_strtod_8bit(VARDATA_ANY(arg2), len2, &end2, &error) != 0.0));
}

Datum 
varbinary_or(PG_FUNCTION_ARGS)
{
	int 	len1,
			len2,
			error;
	char 	*end1,
			*end2;
	bytea 	*arg1, *arg2;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		PG_RETURN_NULL();

	arg1 = PG_GETARG_BYTEA_PP(0);
	arg2 = PG_GETARG_BYTEA_PP(1);

	len1 = VARSIZE_ANY_EXHDR(arg1);
	len2 = VARSIZE_ANY_EXHDR(arg2);

	end1 = VARDATA_ANY(arg1) + len1;
	end2 = VARDATA_ANY(arg1) + len2;

	return ((my_strtod_8bit(VARDATA_ANY(arg1), len1, &end1, &error) != 0.0) || (my_strtod_8bit(VARDATA_ANY(arg2), len2, &end2, &error) != 0.0));
}

static bool 
mys_get_varbinary_bool(bytea *arg)
{
	int len;
	char *end;
	int error;

	len = VARSIZE_ANY_EXHDR(arg);
	end = VARDATA_ANY(arg) + len;
	return ((my_strtod_8bit(VARDATA_ANY(arg), len, &end, &error) == 0.0) ? 1 : 0);
}

Datum 
varbinary_not(PG_FUNCTION_ARGS)
{
	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	return mys_get_varbinary_bool(PG_GETARG_BYTEA_PP(0));
}

Datum 
varbinary_bitand(PG_FUNCTION_ARGS)
{
	bytea 		*arg1, *arg2;
	int 		len1, len2;
	char 		*end1, *end2;
	ulonglong 	long_arg1 = 0,
				long_arg2 = 0,
				result = 0;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		PG_RETURN_NULL();

	arg1 = PG_GETARG_BYTEA_PP(0);
	arg2 = PG_GETARG_BYTEA_PP(1);

	len1 = VARSIZE_ANY_EXHDR(arg1);
	len2 = VARSIZE_ANY_EXHDR(arg2);

	end1 = VARDATA_ANY(arg1) + len1;
	end2 = VARDATA_ANY(arg2) + len2;

	long_arg1 = (ulonglong)strtoll(VARDATA_ANY(arg1), &end1, 10);
	long_arg2 = (ulonglong)strtoll(VARDATA_ANY(arg2), &end2, 10);

	result = long_arg1 & long_arg2;

	PG_RETURN_INT64(result);
}

Datum 
varbinary_bitor(PG_FUNCTION_ARGS)
{
	bytea 		*arg1, *arg2;
	int 		len1, len2;
	char 		*end1, *end2;
	ulonglong 	long_arg1 = 0,
				long_arg2 = 0,
				result = 0;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		PG_RETURN_NULL();
	
	arg1 = PG_GETARG_BYTEA_PP(0);
	arg2 = PG_GETARG_BYTEA_PP(1);

	len1 = VARSIZE_ANY_EXHDR(arg1);
	len2 = VARSIZE_ANY_EXHDR(arg2);

	end1 = VARDATA_ANY(arg1) + len1;
	end2 = VARDATA_ANY(arg2) + len2;

	long_arg1 = (ulonglong)strtoll(VARDATA_ANY(arg1), &end1, 10);
	long_arg2 = (ulonglong)strtoll(VARDATA_ANY(arg2), &end2, 10);

	result = long_arg1 | long_arg2;

	PG_RETURN_UINT64(result);
}

Datum 
varbinary_bitxor(PG_FUNCTION_ARGS)
{
	bytea *arg1, *arg2;
	int len1, len2;
	char *end1, *end2;
	ulonglong long_arg1 = 0;
	ulonglong long_arg2 = 0;
	ulonglong result = 0;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		PG_RETURN_NULL();

	arg1 = PG_GETARG_BYTEA_PP(0);
	arg2 = PG_GETARG_BYTEA_PP(1);

	len1 = VARSIZE_ANY_EXHDR(arg1);
	len2 = VARSIZE_ANY_EXHDR(arg2);

	end1 = VARDATA_ANY(arg1) + len1;
	end2 = VARDATA_ANY(arg2) + len2;

	long_arg1 = (ulonglong)strtoll(VARDATA_ANY(arg1), &end1, 10);
	long_arg2 = (ulonglong)strtoll(VARDATA_ANY(arg2), &end2, 10);

	result = long_arg1 ^ long_arg2;

	PG_RETURN_UINT64(result);
}

Datum 
varbinary_bitneg(PG_FUNCTION_ARGS)
{
	bytea *arg;
	int len;
	char *end;
	ulonglong long_arg = 0;
	ulonglong result = 0;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	arg = PG_GETARG_BYTEA_PP(0);

	len = VARSIZE_ANY_EXHDR(arg);

	end = VARDATA_ANY(arg) + len;

	long_arg = (ulonglong)strtoll(VARDATA_ANY(arg), &end, 10);

	result = ~long_arg;

	PG_RETURN_UINT64(result);
}

Datum 
varbinary_bitshiftleft(PG_FUNCTION_ARGS)
{
	bytea *arg;
	uint32_t shift = PG_GETARG_INT32(1);
	int len;
	char *end;
	ulonglong long_arg = 0;
	ulonglong result = 0;
	ulonglong res = 0;

	arg = PG_GETARG_BYTEA_P(0);

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		PG_RETURN_NULL();

	len = VARSIZE_ANY_EXHDR(arg);
	end = VARDATA_ANY(arg) + len;

	long_arg = (ulonglong)strtoll(VARDATA_ANY(arg), &end, 10);

	res = long_arg << shift;

	result = shift < sizeof(longlong) * 8 ? (longlong)res : 0LL;

	PG_RETURN_UINT64(result);
}

Datum 
varbinary_bitshiftright(PG_FUNCTION_ARGS)
{
	bytea *arg;
	uint32_t shift = PG_GETARG_INT32(1);
	int len;
	char *end;
	ulonglong long_arg = 0;
	ulonglong result = 0;
	ulonglong res = 0;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		PG_RETURN_NULL();

	arg = PG_GETARG_BYTEA_P(0);

	len = VARSIZE_ANY_EXHDR(arg);
	end = VARDATA_ANY(arg) + len;

	long_arg = (ulonglong)strtoll(VARDATA_ANY(arg), &end, 10);

	res = long_arg >> shift;

	result = shift < sizeof(longlong) * 8 ? (longlong)res : 0LL;

	PG_RETURN_UINT64(result);
}

/* reference from 'Datum bytea_sortsupport(PG_FUNCTION_ARGS)' */
Datum 
varbinary_sortsupport(PG_FUNCTION_ARGS)
{
	SortSupport ssup = (SortSupport)PG_GETARG_POINTER(0);
	MemoryContext oldcontext;

	oldcontext = MemoryContextSwitchTo(ssup->ssup_cxt);

	/* Use generic string SortSupport, forcing "C" collation */
	varstr_sortsupport(ssup, VARBINARYOID, C_COLLATION_OID);

	MemoryContextSwitchTo(oldcontext);

	PG_RETURN_VOID();
}
