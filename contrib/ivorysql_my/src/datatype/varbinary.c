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
#include "parser/scansup.h"  /* ReqID - BUG#8428 8441 */
#include "libpq/pqformat.h"
#include "lib/stringinfo.h"
#include "miscadmin.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/bytea.h"
#include "utils/elog.h"
#include "utils/fmgrprotos.h"
#include "utils/guc.h"
#include "utils/lsyscache.h" /* ReqID - BUG#8428 8441 */
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

/* Begin - BUG#8428 8441 */
PG_FUNCTION_INFO_V1(varbinary_concat);
PG_FUNCTION_INFO_V1(varbinary2json);
PG_FUNCTION_INFO_V1(bit2varbinary);
/* Begin - BUG#8428 8441 */

typedef long long int longlong;
typedef unsigned long long int ulonglong;

// reference from varchar.c function varchar()
static Datum
varbinary_internel(FunctionCallInfo fcinfo, bool isPg2My)
{
	int	 	len, maxlen;
	bytea 	*source = PG_GETARG_BYTEA_PP(0);
	int32 	typmod = PG_GETARG_INT32(1);

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
	char 		*src1, *src2;
	ulonglong 	long_arg1 = 0,
				long_arg2 = 0,
				result = 0;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		PG_RETURN_NULL();

	arg1 = PG_GETARG_BYTEA_PP(0);
	arg2 = PG_GETARG_BYTEA_PP(1);

	src1 = text_to_cstring((text *) arg1);
	src2 = text_to_cstring((text *) arg2);

	long_arg1 = (ulonglong)strtoll(src1, NULL, 10);
	long_arg2 = (ulonglong)strtoll(src2, NULL, 10);

	result = long_arg1 & long_arg2;

	pfree(src1);
	pfree(src2);

	PG_RETURN_INT64(result);
}

Datum 
varbinary_bitor(PG_FUNCTION_ARGS)
{
	bytea 		*arg1, *arg2;
	char 		*src1, *src2;
	ulonglong 	long_arg1 = 0,
				long_arg2 = 0,
				result = 0;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		PG_RETURN_NULL();
	
	arg1 = PG_GETARG_BYTEA_PP(0);
	arg2 = PG_GETARG_BYTEA_PP(1);

	src1 = text_to_cstring((text *) arg1);
	src2 = text_to_cstring((text *) arg2);

	long_arg1 = (ulonglong)strtoll(src1, NULL, 10);
	long_arg2 = (ulonglong)strtoll(src2, NULL, 10);

	result = long_arg1 | long_arg2;

	pfree(src1);
	pfree(src2);

	PG_RETURN_UINT64(result);
}

Datum 
varbinary_bitxor(PG_FUNCTION_ARGS)
{
	bytea 		*arg1, *arg2;
	char 		*src1, *src2;
	ulonglong 	long_arg1 = 0,
				long_arg2 = 0,
				result = 0;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		PG_RETURN_NULL();

	arg1 = PG_GETARG_BYTEA_PP(0);
	arg2 = PG_GETARG_BYTEA_PP(1);

	src1 = text_to_cstring((text *) arg1);
	src2 = text_to_cstring((text *) arg2);

	long_arg1 = (ulonglong)strtoll(src1, NULL, 10);
	long_arg2 = (ulonglong)strtoll(src2, NULL, 10);

	result = long_arg1 ^ long_arg2;

	pfree(src1);
	pfree(src2);

	PG_RETURN_UINT64(result);
}

Datum 
varbinary_bitneg(PG_FUNCTION_ARGS)
{
	bytea *arg;
	char *src;
	ulonglong long_arg = 0;
	ulonglong result = 0;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	arg = PG_GETARG_BYTEA_PP(0);

	src = text_to_cstring((text *) arg);

	long_arg = (ulonglong)strtoll(src, NULL, 10);

	result = ~long_arg;

	pfree(src);

	PG_RETURN_UINT64(result);
}

Datum 
varbinary_bitshiftleft(PG_FUNCTION_ARGS)
{
	bytea *arg;
	uint32_t shift = PG_GETARG_INT32(1);
	char *src;
	ulonglong long_arg = 0;
	ulonglong result = 0;
	ulonglong res = 0;

	arg = PG_GETARG_BYTEA_P(0);

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		PG_RETURN_NULL();

	src = text_to_cstring((text *) arg);

	long_arg = (ulonglong)strtoll(src, NULL, 10);

	res = long_arg << shift;

	result = shift < sizeof(longlong) * 8 ? (longlong)res : 0LL;

	pfree(src);

	PG_RETURN_UINT64(result);
}

Datum 
varbinary_bitshiftright(PG_FUNCTION_ARGS)
{
	bytea *arg;
	uint32_t shift = PG_GETARG_INT32(1);
	char *src;
	ulonglong long_arg = 0;
	ulonglong result = 0;
	ulonglong res = 0;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		PG_RETURN_NULL();

	arg = PG_GETARG_BYTEA_P(0);

	src = text_to_cstring((text *) arg);

	long_arg = (ulonglong)strtoll(src, NULL, 10);

	res = long_arg >> shift;

	result = shift < sizeof(longlong) * 8 ? (longlong)res : 0LL;

	pfree(src);

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

/* Begin - BUG#8428 8441 */
Datum
varbinary_concat(PG_FUNCTION_ARGS)
{
    bytea *arg1 = PG_GETARG_BYTEA_PP(0);
    bytea *arg2 = PG_GETARG_BYTEA_PP(1);

    int32 len1 = VARSIZE_ANY_EXHDR(arg1);
    int32 len2 = VARSIZE_ANY_EXHDR(arg2);

    text *result;

    result = (text *) palloc(VARHDRSZ + len1 + len2);

    SET_VARSIZE(result, VARHDRSZ + len1 + len2);

    memcpy(VARDATA(result),
           VARDATA_ANY(arg1),
           len1);

    memcpy(VARDATA(result) + len1,
           VARDATA_ANY(arg2),
           len2);

    PG_RETURN_TEXT_P(result);
}

Datum
varbinary2json(PG_FUNCTION_ARGS)
{
	bytea *arg = PG_GETARG_BYTEA_PP(0);
	const char *data = VARDATA_ANY(arg);
	int32 len = VARSIZE_ANY_EXHDR(arg);
	char *str;
	Oid typinput;
	Oid typioparam;
	FmgrInfo finfo;
	Datum result;

	if (memchr(data, '\0', len) != NULL)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input syntax for type json"),
				 errdetail("JSON text cannot contain zero bytes.")));

	str = pnstrdup(data, len);

	getTypeInputInfo(JSONOID, &typinput, &typioparam);
	fmgr_info(typinput, &finfo);

	result = InputFunctionCall(&finfo, str, typioparam, -1);

	PG_RETURN_DATUM(result);
}

Datum
bit2varbinary(PG_FUNCTION_ARGS)
{
	VarBit		   *bits = PG_GETARG_VARBIT_P(0);
	int				bitlen = VARBITLEN(bits);
	int				outlen = (bitlen + 7) / 8;
	int				shift = outlen * 8 - bitlen;
	unsigned char  *src = VARBITS(bits);
	bytea		   *result;
	unsigned char  *dst;
	int				i;

	result = (bytea *) palloc0(VARHDRSZ + outlen);
	SET_VARSIZE(result, VARHDRSZ + outlen);
	dst = (unsigned char *) VARDATA(result);

	/* Right-align the bit string into whole bytes, as MySQL does. */
	for (i = 0; i < bitlen; i++)
	{
		int			outbit;

		if ((src[i / 8] & (0x80 >> (i % 8))) == 0)
			continue;

		outbit = i + shift;
		dst[outbit / 8] |= 0x80 >> (outbit % 8);
	}

	PG_RETURN_BYTEA_P(result);
}
/* End - BUG#8428 8441 */
