/*-------------------------------------------------------------------------
 *
 * anybinary.c
 *   for mysql binary type
 *
 * Portions Copyright (c) 2025 - , Highgo MySQL Compatible Development Group
 *
 * IDENTIFICATION
 *   contrib/ivorysql_my/src/datatype/binary/anybinary.c
 *
 * add the file for requirement "SRS-MYSQL-BINARYTYPE"
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include <ctype.h>
#include <limits.h>
#include <math.h>

#include "anybinary.h"
#include "catalog/pg_collation.h"
#include "catalog/pg_type.h"
#include "fmgr.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/elog.h"
#include "utils/sortsupport.h"
#include "utils/fmgrprotos.h"
#include "utils/float.h"

#include "../include/ivorysql_my.h"

#define BINARY_MAX_LENGTH       255
#define VARBINARY_MAX_LENGTH    65532

PG_FUNCTION_INFO_V1(binary_typmodout);
PG_FUNCTION_INFO_V1(binary_typmodin);
PG_FUNCTION_INFO_V1(varbinary_typmodin);
PG_FUNCTION_INFO_V1(anybinary_eq);
PG_FUNCTION_INFO_V1(anybinary_ne);
PG_FUNCTION_INFO_V1(anybinary_lt);
PG_FUNCTION_INFO_V1(anybinary_le);
PG_FUNCTION_INFO_V1(anybinary_gt);
PG_FUNCTION_INFO_V1(anybinary_ge);
PG_FUNCTION_INFO_V1(anybinary_seq);
PG_FUNCTION_INFO_V1(anybinary_cmp);
PG_FUNCTION_INFO_V1(anybinary_hash);
PG_FUNCTION_INFO_V1(anybinary_hash_extended);
PG_FUNCTION_INFO_V1(anybinary_div);
PG_FUNCTION_INFO_V1(anybinary_div_floor);
PG_FUNCTION_INFO_V1(anybinary_plus);
PG_FUNCTION_INFO_V1(anybinary_sub);
PG_FUNCTION_INFO_V1(anybinary_minus);
PG_FUNCTION_INFO_V1(anybinary_mul);
PG_FUNCTION_INFO_V1(anybinary_mod);
/* Begin - bug#6051 */
PG_FUNCTION_INFO_V1(anybinary_smaller);
PG_FUNCTION_INFO_V1(anybinary_larger);
PG_FUNCTION_INFO_V1(anybinary_sum);
PG_FUNCTION_INFO_V1(binary_avg_accum);
/* End - bug#6051 */

/* common code for varbinarytypmodin and binarytypmodin */
int32 
anybinary_typmodin(ArrayType *ta, int64 max_length, const char *typname)
{
	int32		typmod;
	int32	   *tl;
	int			n;

	tl = ArrayGetIntegerTypmods(ta, &n);

	/*
	* we're not too tense about good error message here because grammar
	* shouldn't allow wrong number of modifiers for CHAR
	*/
	if (n != 1)
	{
		ereport(ERROR,
			(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("invalid type modifier")));
	}

	if (*tl < 0)
	{
		ereport(ERROR,
			(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				errmsg("length for type %s must be at least 0", typname)));
	}

	if (*tl > max_length)
	{
		ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			errmsg("length for type %s cannot exceed %ld", typname, max_length)));
	}

	typmod = VARHDRSZ + *tl;
	return typmod;
}

/* common code for binary typmodout and varbinary typmodout */
static char *
anybinary_typmodout(int32 typmod)
{
	const size_t buffer_len = 64;

	char* res = (char*)palloc(buffer_len);

	if (typmod >= VARHDRSZ)
	{
		snprintf(res, buffer_len, "(%d)", (int)(typmod - VARHDRSZ));
	}
	else
	{
		*res = '\0';
	}

	return res;
}

Datum
binary_typmodin(PG_FUNCTION_ARGS)
{
	ArrayType* ta = PG_GETARG_ARRAYTYPE_P(0);

	PG_RETURN_INT32(anybinary_typmodin(ta, BINARY_MAX_LENGTH, "binary"));
}

Datum
binary_typmodout(PG_FUNCTION_ARGS)
{
	int32 typmod = PG_GETARG_INT32(0);

	PG_RETURN_CSTRING(anybinary_typmodout(typmod));
}

Datum
varbinary_typmodin(PG_FUNCTION_ARGS)
{
	ArrayType *ta = PG_GETARG_ARRAYTYPE_P(0);

	PG_RETURN_INT32(anybinary_typmodin(ta, VARBINARY_MAX_LENGTH, "varbinary"));
}


/*****************************************************************************
 *	 Operator Function														 *
 *****************************************************************************/
Datum
anybinary_eq(PG_FUNCTION_ARGS)
{
	return DatumGetBool(DirectFunctionCall2(byteaeq,
											PG_GETARG_DATUM(0),
											PG_GETARG_DATUM(1)));
}

Datum
anybinary_ne(PG_FUNCTION_ARGS)
{
	return DatumGetBool(DirectFunctionCall2(byteane,
											PG_GETARG_DATUM(0),
											PG_GETARG_DATUM(1)));
}

Datum
anybinary_lt(PG_FUNCTION_ARGS)
{
	bytea	   *arg1 = PG_GETARG_BYTEA_PP(0);
	bytea	   *arg2 = PG_GETARG_BYTEA_PP(1);
	int			len1,
				len2;
	int			cmp;

	len1 = VARSIZE_ANY_EXHDR(arg1);
	len2 = VARSIZE_ANY_EXHDR(arg2);

	cmp = memcmp(VARDATA_ANY(arg1), VARDATA_ANY(arg2), Min(len1, len2));

	PG_FREE_IF_COPY(arg1, 0);
	PG_FREE_IF_COPY(arg2, 1);

	PG_RETURN_BOOL((cmp < 0) || ((cmp == 0) && (len1 < len2)));
}

Datum
anybinary_le(PG_FUNCTION_ARGS)
{
	bytea	   *arg1 = PG_GETARG_BYTEA_PP(0);
	bytea	   *arg2 = PG_GETARG_BYTEA_PP(1);
	int			len1,
				len2;
	int			cmp;

	len1 = VARSIZE_ANY_EXHDR(arg1);
	len2 = VARSIZE_ANY_EXHDR(arg2);

	cmp = memcmp(VARDATA_ANY(arg1), VARDATA_ANY(arg2), Min(len1, len2));

	PG_FREE_IF_COPY(arg1, 0);
	PG_FREE_IF_COPY(arg2, 1);

	PG_RETURN_BOOL((cmp < 0) || ((cmp == 0) && (len1 <= len2)));
}

Datum
anybinary_gt(PG_FUNCTION_ARGS)
{
	bytea	   *arg1 = PG_GETARG_BYTEA_PP(0);
	bytea	   *arg2 = PG_GETARG_BYTEA_PP(1);
	int			len1,
				len2;
	int			cmp;

	len1 = VARSIZE_ANY_EXHDR(arg1);
	len2 = VARSIZE_ANY_EXHDR(arg2);

	cmp = memcmp(VARDATA_ANY(arg1), VARDATA_ANY(arg2), Min(len1, len2));

	PG_FREE_IF_COPY(arg1, 0);
	PG_FREE_IF_COPY(arg2, 1);

	PG_RETURN_BOOL((cmp > 0) || ((cmp == 0) && (len1 > len2)));
}

Datum
anybinary_ge(PG_FUNCTION_ARGS)
{
	bytea	   *arg1 = PG_GETARG_BYTEA_PP(0);
	bytea	   *arg2 = PG_GETARG_BYTEA_PP(1);
	int			len1,
				len2;
	int			cmp;

	len1 = VARSIZE_ANY_EXHDR(arg1);
	len2 = VARSIZE_ANY_EXHDR(arg2);

	cmp = memcmp(VARDATA_ANY(arg1), VARDATA_ANY(arg2), Min(len1, len2));

	PG_FREE_IF_COPY(arg1, 0);
	PG_FREE_IF_COPY(arg2, 1);

	PG_RETURN_BOOL((cmp > 0) || ((cmp == 0) && (len1 >= len2)));
}

Datum
anybinary_seq(PG_FUNCTION_ARGS)
{
	int	null_cnt = 0;

	if (PG_ARGISNULL(0)) null_cnt++;
	if (PG_ARGISNULL(1)) null_cnt++;

	if (null_cnt == 1)
		PG_RETURN_BOOL(false);
	else if (null_cnt == 2)
		PG_RETURN_BOOL(true);

	return anybinary_eq(fcinfo);
}

/*****************************************************************************
 *	 Index Function														 *
 *****************************************************************************/

Datum
anybinary_cmp(PG_FUNCTION_ARGS)
{
	bytea	   *arg1 = PG_GETARG_BYTEA_PP(0);
	bytea	   *arg2 = PG_GETARG_BYTEA_PP(1);
	int			len1,
				len2;
	int			cmp;

	len1 = VARSIZE_ANY_EXHDR(arg1);
	len2 = VARSIZE_ANY_EXHDR(arg2);

	cmp = memcmp(VARDATA_ANY(arg1), VARDATA_ANY(arg2), Min(len1, len2));
	if ((cmp == 0) && (len1 != len2))
		cmp = (len1 < len2) ? -1 : 1;

	PG_FREE_IF_COPY(arg1, 0);
	PG_FREE_IF_COPY(arg2, 1);

	PG_RETURN_INT32(cmp);
}

extern Datum hashvarlena(PG_FUNCTION_ARGS);
extern Datum hashvarlenaextended(PG_FUNCTION_ARGS);

Datum
anybinary_hash(PG_FUNCTION_ARGS)
{
	return hashvarlena(fcinfo);
}

Datum
anybinary_hash_extended(PG_FUNCTION_ARGS)
{
	return hashvarlenaextended(fcinfo);
}

static int64
mys_bytea2int64(bytea *arg)
{
	char 	*str;
	char	*end;
	int		len;
	int64 	arg_val = 0;

	len = VARSIZE_ANY_EXHDR(arg);

	if (len == 0)
	{
		return 0;
	}

	str = VARDATA_ANY(arg);
	for (int i = 0; i < len; i++)
	{
		if (isspace((unsigned char)*str))
		{
			str++;
		}
		else
			goto gt; /* any one is not space, call 'strtol' to convert */
	}

	/* return 0 if all space in str */
	return 0;

gt:
	end = VARDATA_ANY(arg) + len;
	arg_val = (int64) strtol(VARDATA_ANY(arg), &end, 10);
	return arg_val;
}

static float8
mys_bytea2float8(bytea *arg)
{
	int		len;
	char	*end;
	char	*num;
	double 	arg_val = 0;
	bool have_error = false;

	num = VARDATA_ANY(arg);
	len = VARSIZE_ANY_EXHDR(arg);
	end = VARDATA_ANY(arg) + len;

	arg_val = float8in_internal_opt_error(num, &end, "double precision", num, &have_error);
	if (have_error)
	{
		arg_val = strtod(num, &end);
	}

	return arg_val;
}

Datum
anybinary_div(PG_FUNCTION_ARGS)
{
	char	*src1, *src2;
	int		len1, len2;
	char	*end1, *end2;
	double 	arg1_val = 0;
	double 	arg2_val = 0;
	double 	result = 0;
	bytea 	*arg1, *arg2;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		PG_RETURN_NULL();

	arg1 = PG_GETARG_BYTEA_PP(0);
	arg2 = PG_GETARG_BYTEA_PP(1);

	src1 = VARDATA_ANY(arg1);
	src2 = VARDATA_ANY(arg2);

	len1 = VARSIZE_ANY_EXHDR(arg1);
	len2 = VARSIZE_ANY_EXHDR(arg2);

	src1 = pnstrdup(src1, len1);
	src2 = pnstrdup(src2, len2);

	arg1_val = (double) strtod(src1, &end1);
	arg2_val = (double) strtod(src2, &end2);

	if (arg2_val == 0)
	{
		pfree(src1);
		pfree(src2);

		PG_RETURN_NULL();
	}

	result = arg1_val / arg2_val;

	pfree(src1);
	pfree(src2);

	if (isinf(result))
	{
		PG_RETURN_NULL();
	}

	PG_RETURN_FLOAT8(result);
}

Datum
anybinary_div_floor(PG_FUNCTION_ARGS)
{
	int		len1, len2;
	char	*end1, *end2;
	double 	arg1_val = 0;
	double 	arg2_val = 0;
	double 	result = 0;
	bytea 	*arg1, *arg2;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		PG_RETURN_NULL();

	arg1 = PG_GETARG_BYTEA_PP(0);
	arg2 = PG_GETARG_BYTEA_PP(1);

	len1 = VARSIZE_ANY_EXHDR(arg1);
	len2 = VARSIZE_ANY_EXHDR(arg2);

	end1 = VARDATA_ANY(arg1) + len1;
	end2 = VARDATA_ANY(arg2) + len2;

	arg1_val = (double) strtod(VARDATA_ANY(arg1), &end1);
	arg2_val = (double) strtod(VARDATA_ANY(arg2), &end2);

	if (arg2_val == 0)
	{
		PG_RETURN_NULL();
	}

	result = arg1_val / arg2_val;

	if (isinf(result))
	{
		PG_RETURN_NULL();
	}

	PG_RETURN_FLOAT8(floor(result));
}

Datum
anybinary_plus(PG_FUNCTION_ARGS)
{
	int64 arg1_val = 0;
	int64 arg2_val = 0;
	bytea *arg1, *arg2;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		PG_RETURN_NULL();

	arg1 = PG_GETARG_BYTEA_PP(0);
	arg2 = PG_GETARG_BYTEA_PP(1);

	arg1_val = mys_bytea2int64(arg1);
	arg2_val = mys_bytea2int64(arg2);

	return DatumGetInt64(DirectFunctionCall2(int8pl,
											Int64GetDatum(arg1_val),
											Int64GetDatum(arg2_val)));
}

Datum
anybinary_sub(PG_FUNCTION_ARGS)
{
	int64 arg1_val = 0;
	int64 arg2_val = 0;
	bytea *arg1, *arg2;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		PG_RETURN_NULL();

	arg1 = PG_GETARG_BYTEA_PP(0);
	arg2 = PG_GETARG_BYTEA_PP(1);

	arg1_val = mys_bytea2int64(arg1);
	arg2_val = mys_bytea2int64(arg2);

	return DatumGetInt64(DirectFunctionCall2(int8mi,
											Int64GetDatum(arg1_val),
											Int64GetDatum(arg2_val)));
}

Datum
anybinary_minus(PG_FUNCTION_ARGS)
{
	float8 arg1_val = 0;
	float8 result = 0;
	bytea *arg1;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	arg1 = PG_GETARG_BYTEA_PP(0);

	arg1_val = mys_bytea2int64(arg1);

	/* can't get same as mysql if use 'float8um' */
	result = -arg1_val;

	PG_RETURN_FLOAT8(result);
}

Datum
anybinary_mul(PG_FUNCTION_ARGS)
{
	int64 arg1_val = 0;
	int64 arg2_val = 0;
	bytea *arg1, *arg2;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		PG_RETURN_NULL();

	arg1 = PG_GETARG_BYTEA_PP(0);
	arg2 = PG_GETARG_BYTEA_PP(1);

	arg1_val = mys_bytea2int64(arg1);
	arg2_val = mys_bytea2int64(arg2);

	return DatumGetInt64(DirectFunctionCall2(int8mul,
											Int64GetDatum(arg1_val),
											Int64GetDatum(arg2_val)));
}

Datum
anybinary_mod(PG_FUNCTION_ARGS)
{
	bytea *arg1, *arg2;
	float8 arg1_val;
	float8 arg2_val;
	float8 result;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		PG_RETURN_NULL();

	arg1 = PG_GETARG_BYTEA_PP(0);
	arg2 = PG_GETARG_BYTEA_PP(1);
	arg1_val = mys_bytea2float8(arg1);
	arg2_val = mys_bytea2float8(arg2);

	if (arg2_val == 0)
		PG_RETURN_NULL();

	result = fmod(arg1_val, arg2_val);
	PG_RETURN_FLOAT8(result);
}

bytea *
truncate_bytea_with_len(const bytea *b, int len)
{
	bytea	   *result = (text *) palloc(len + VARHDRSZ);

	SET_VARSIZE(result, len + VARHDRSZ);
	memcpy(VARDATA(result), VARDATA(b), len);

	return result;
}

/* Begin - bug#6051 */
Datum
anybinary_smaller(PG_FUNCTION_ARGS)
{
	bytea	   *arg1 = PG_GETARG_BYTEA_PP(0);
	bytea	   *arg2 = PG_GETARG_BYTEA_PP(1);
	bytea 	   *result;
	int			len1,
				len2;
	int			cmp;

	len1 = VARSIZE_ANY_EXHDR(arg1);
	len2 = VARSIZE_ANY_EXHDR(arg2);

	cmp = memcmp(VARDATA_ANY(arg1), VARDATA_ANY(arg2), Min(len1, len2));
	if ((cmp == 0) && (len1 != len2))
		cmp = (len1 < len2) ? -1 : 1;

	PG_FREE_IF_COPY(arg1, 0);
	PG_FREE_IF_COPY(arg2, 1);

	result = cmp < 0 ? arg1 : arg2;

	PG_RETURN_BYTEA_P(result);
}

Datum
anybinary_larger(PG_FUNCTION_ARGS)
{
	bytea	   *arg1 = PG_GETARG_BYTEA_PP(0);
	bytea	   *arg2 = PG_GETARG_BYTEA_PP(1);
	bytea 	   *result;
	int			len1,
				len2;
	int			cmp;

	len1 = VARSIZE_ANY_EXHDR(arg1);
	len2 = VARSIZE_ANY_EXHDR(arg2);

	cmp = memcmp(VARDATA_ANY(arg1), VARDATA_ANY(arg2), Min(len1, len2));
	if ((cmp == 0) && (len1 != len2))
		cmp = (len1 < len2) ? -1 : 1;

	PG_FREE_IF_COPY(arg1, 0);
	PG_FREE_IF_COPY(arg2, 1);

	result = cmp > 0 ? arg1 : arg2;

	PG_RETURN_BYTEA_P(result);
}

Datum
anybinary_sum(PG_FUNCTION_ARGS)
{
	float8 	arg1 = 0,
			arg2 = 0,
			ret = 0;

	if (PG_ARGISNULL(0) && PG_ARGISNULL(1))
		PG_RETURN_NULL();

	if (!PG_ARGISNULL(0))
	{
		arg1 = PG_GETARG_FLOAT8(0);
	}

	if (!PG_ARGISNULL(1))
	{
		bytea 	*arg = PG_GETARG_BYTEA_PP(1);
		char	*src = (char *) text_to_cstring(arg);
		arg2 = my_atod(src, false);
	}
	
	ret = float8_pl(arg1, arg2);

	PG_RETURN_FLOAT8(ret);
}

Datum
binary_avg_accum(PG_FUNCTION_ARGS)
{
	Datum 	ret;
	float8 	val = 0;
	bytea 	*arg;
	char	*src;

	arg = PG_GETARG_BYTEA_PP(1);
	src = (char *) text_to_cstring(arg);

	val = my_atod(src, false);

	ret = DirectFunctionCall2(float8_accum, PG_GETARG_DATUM(0), Float8GetDatum(val));

	PG_RETURN_DATUM(ret);
}
/* End - bug#6051 */