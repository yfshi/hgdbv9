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
#include <limits.h>

#include "postgres.h"

#include "anybinary.h"
#include "catalog/pg_collation.h"
#include "catalog/pg_type.h"
#include "fmgr.h"
#include "parser/parse_oper.h"
#include "libpq/pqformat.h"
#include "lib/stringinfo.h"
#include "miscadmin.h"
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
/* Begin BUG#5048 5070 */
PG_FUNCTION_INFO_V1(varbinary2text);
PG_FUNCTION_INFO_V1(varbinary2bpchar);
PG_FUNCTION_INFO_V1(varbinary2varchar);
PG_FUNCTION_INFO_V1(anybinary2date);
PG_FUNCTION_INFO_V1(anybinary2time);
PG_FUNCTION_INFO_V1(anybinary2datetime);
/* End BUG#5048 5070 */
PG_FUNCTION_INFO_V1(varbinary_in);
PG_FUNCTION_INFO_V1(varbinary_recv);
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
			maxlen = atttypmod - VARHDRSZ;
	size_t  hex_len = 0;
	bytea 	*result;

	/* Recognize hex input */
	/* Begin - bug#z5465 */ 
	if (inputText[0] == '\\' && inputText[1] == 'x')
	{
		hex_len = len - 2;
		if (HEX_LEN(len) > maxlen)
		{
			if (is_strict_mode())
			{
				ereport(ERROR, 
					(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
						errmsg("value too long for type varbinary(%d)", atttypmod - VARHDRSZ)));
			}
			else
			{
				hex_len = maxlen * 2;
				ereport(WARNING, 
					(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
						errmsg("value too long for type varbinary(%d)", atttypmod - VARHDRSZ)));
			}
		}
		result = (bytea *)palloc(HEX_LEN(len) + VARHDRSZ); /* */

		bc = hex_decode(inputText + 2, hex_len, VARDATA(result));
		SET_VARSIZE(result, HEX_LEN(len) > maxlen ? atttypmod : HEX_LEN(len) + VARHDRSZ);
		PG_RETURN_BYTEA_P(result);
	}
	/* End - bug#z5465 */ 

	/* Else, it's the traditional escaped style */
	if (atttypmod >= (int32)VARHDRSZ && len > maxlen)
	{
		if (is_strict_mode())
		{
			ereport(ERROR,
					(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
					 errmsg("value too long for type varbinary")));
			PG_RETURN_VOID();
		}
		else
		{
			ereport(WARNING,
					(errcode(ERRCODE_STRING_DATA_RIGHT_TRUNCATION),
					 errmsg("value too long for type varbinary, data truncated")));
		}
	}

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

/* Begin BUG#5048 5070 */
Datum 
anybinary2date(PG_FUNCTION_ARGS)
{
	bytea 	*arg = PG_GETARG_BYTEA_PP(0);
	int 	len = VARSIZE_ANY_EXHDR(arg);
	char 	*data = palloc0(len + 1);
	Datum 	result;

	memcpy(data, VARDATA_ANY(arg), len);
	
	result = DirectFunctionCall1(mysdate_in, 
							     CStringGetDatum(data));

	if (result == MYSQL_ZERO_DATE)
	{
		PG_RETURN_NULL();
	}

	pfree(data);
	
	return result;
}

Datum 
anybinary2time(PG_FUNCTION_ARGS)
{
	bytea 	*arg = PG_GETARG_BYTEA_PP(0);
	int 	len = VARSIZE_ANY_EXHDR(arg);
	char 	*data = palloc0(len + 1);
	Datum 	result;

	memcpy(data, VARDATA_ANY(arg), len);
	
	result = DirectFunctionCall1(mystime_in, 
							     CStringGetDatum(data));

	pfree(data);
	
	return result;
}

Datum 
anybinary2datetime(PG_FUNCTION_ARGS)
{
	bytea 	*arg = PG_GETARG_BYTEA_PP(0);
	int 	len = VARSIZE_ANY_EXHDR(arg);
	char 	*data = palloc0(len + 1);
	Datum 	result;

	memcpy(data, VARDATA_ANY(arg), len);
	
	result = DirectFunctionCall1(mysdatetime_in, 
							     CStringGetDatum(data));
	if (result == MYSQL_ZERO_DATE)
	{
		PG_RETURN_NULL();
	}

	pfree(data);
	
	return result;
}

Datum 
varbinary2text(PG_FUNCTION_ARGS)
{
	bytea 	*arg = PG_GETARG_BYTEA_PP(0);
	char 	*data = VARDATA_ANY(arg);
	int 	len = VARSIZE_ANY_EXHDR(arg);
	text	*ret = cstring_to_text_with_len(data, len);
	
	PG_RETURN_TEXT_P(ret);
}

Datum 
varbinary2bpchar(PG_FUNCTION_ARGS)
{
	bytea 		*arg = PG_GETARG_BYTEA_PP(0);
	int 		len = VARSIZE_ANY_EXHDR(arg);
	char 		*data = palloc0(len + 1);
	Datum 		result;

	memcpy(data, VARDATA_ANY(arg), len);
	
	result = DirectFunctionCall3(bpcharin, 
								 CStringGetDatum(data),
								 ObjectIdGetDatum(InvalidOid),
								 Int32GetDatum(-1));

	pfree(data);

	return result;
}

Datum 
varbinary2varchar(PG_FUNCTION_ARGS)
{
	bytea 	*arg = PG_GETARG_BYTEA_PP(0);
	int 	len = VARSIZE_ANY_EXHDR(arg);
	char 	*data = palloc0(len + 1);
	Datum 	result;

	memcpy(data, VARDATA_ANY(arg), len);
	
	result = DirectFunctionCall3(varcharin, 
								 CStringGetDatum(data),
								 ObjectIdGetDatum(InvalidOid),
								 Int32GetDatum(-1));
	

	pfree(data);
	
	return result;
}
/* End BUG#5048 5070 */
