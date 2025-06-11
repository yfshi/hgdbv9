/*------------------------------------------------------------------
 * 
 * File: mysint2.c
 *
 * Abstract: 
 * 		Compatible with MySQL's INT2 data type.
 *
 * Authored by liyonghao@highgo.com, 20250409.
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_my/src/datatype/mysint2.c
 *
 * ReqID:SRS-MYSQL-NUMBERICTYPE-INT2
 * 
 *------------------------------------------------------------------
 */

#include "postgres.h"
#include "common/int.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/numeric.h"
#include "../include/ivorysql_my.h"

PG_FUNCTION_INFO_V1(myint2_bit_and);
PG_FUNCTION_INFO_V1(myint2_bit_or);
PG_FUNCTION_INFO_V1(myint2_pl);
PG_FUNCTION_INFO_V1(myint2_mi);
PG_FUNCTION_INFO_V1(myint2_mul);
PG_FUNCTION_INFO_V1(myint2_div);
PG_FUNCTION_INFO_V1(myint2_exact_div);
PG_FUNCTION_INFO_V1(myint2_mod);
PG_FUNCTION_INFO_V1(myint2_bit_xor);
PG_FUNCTION_INFO_V1(myint2_or);
PG_FUNCTION_INFO_V1(myint2_and);
PG_FUNCTION_INFO_V1(myint2_safeeq);
PG_FUNCTION_INFO_V1(myint2_not);
PG_FUNCTION_INFO_V1(myint2_bit_neg);
PG_FUNCTION_INFO_V1(myint2_positive);
PG_FUNCTION_INFO_V1(myint2_negative);
PG_FUNCTION_INFO_V1(myint2_boolean);

int16 
my_atoi_int2(const char *s)
{
	long l;
	char *badp;
	int save_errno = 0;
	int flag = SIGNED_FLAG;

	/* skip leading spaces */
	while (likely(*s) && isspace((unsigned char) *s))
		s++;

	/*
	 * Some versions of strtol treat the empty string as an error, but some
	 * seem not to.  Make an explicit test to be sure we catch it.
	 */
	if (s == NULL)
		ereport(ERROR,
			(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				errmsg("invalid input syntax for type smallint")));
	if (*s == 0)
	{
		if (!is_strict_mode())
		{
			ereport(WARNING,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input syntax for type %s: \"%s\"",
						"smallint", s)));

			return 0;
		}
		else
		{
			ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input syntax for type %s: \"%s\"",
						"smallint", s)));
		}
	}

	errno = 0;
	l = my_strntoull10rnd_8bit(s, strlen(s), flag, &badp, &save_errno);

	/* We made no progress parsing the string, so bail out */
	if (s == badp)
	{
		if (!is_strict_mode())
		{
			ereport(WARNING,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input syntax for type %s: \"%s\"",
						"smallint", s)));

			return 0;
		}
		else
		{
			ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input syntax for type %s: \"%s\"",
						"smallint", s)));
		}
	}

	if (save_errno == ERANGE || l < PG_INT16_MIN || l > PG_INT16_MAX)
	{
		if(!is_strict_mode())
		{
			if(l < PG_INT16_MIN)
			{
				l = PG_INT16_MIN;
			}
			else
			{
				l = PG_INT16_MAX;
			}

			ereport(WARNING,
				(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
					errmsg("value \"%s\" is out of range for type smallint", s)));
		}
		else
		{
			ereport(ERROR,
				(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
					errmsg("value \"%s\" is out of range for type smallint", s)));
		}
	}

	/*
	 * Skip any trailing whitespace; if anything but whitespace remains before
	 * the terminating character, bail out
	 */
	while (*badp && *badp != '\0' && isspace((unsigned char)*badp))
		badp++;

	if (*badp && *badp != '\0')
	{
		if(!is_strict_mode())
		{
			ereport(WARNING,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
					errmsg("invalid input syntax for type smallint: \"%s\"", s)));
		}
		else
		{
			ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
					errmsg("invalid input syntax for type smallint: \"%s\"", s)));
		}
	}

	return (int16)l;
}

/* operator int2 & int2*/
Datum 
myint2_bit_and(PG_FUNCTION_ARGS)
{
	uint64 arg1 = PG_GETARG_UINT64(0);
	uint64 arg2 = PG_GETARG_UINT64(1);

	PG_RETURN_UINT64(arg1 & arg2);
}

/* operator int2 | int2*/
Datum 
myint2_bit_or(PG_FUNCTION_ARGS)
{
	uint64 arg1 = PG_GETARG_UINT64(0);
	uint64 arg2 = PG_GETARG_UINT64(1);

	PG_RETURN_UINT64(arg1 | arg2);
}

/* operator int2 + int2*/
Datum 
myint2_pl(PG_FUNCTION_ARGS)
{
	int64 arg0 = PG_GETARG_INT64(0);
	int64 arg1 = PG_GETARG_INT64(1);
	int64 result;

	result = arg0 + arg1;

	PG_RETURN_INT32(result);
}

/* operator int2 - int2 */
Datum 
myint2_mi(PG_FUNCTION_ARGS)
{
	int64 arg0 = PG_GETARG_INT64(0);
	int64 arg1 = PG_GETARG_INT64(1);
	int64 result = 0;
	result = arg0 - arg1;

	PG_RETURN_INT32(result);
}

/* operator int2 * int2 */
Datum 
myint2_mul(PG_FUNCTION_ARGS)
{
	int64 arg0 = PG_GETARG_INT64(0);
	int64 arg1 = PG_GETARG_INT64(1);
	int64 result = 0;
	
	result = DatumGetInt32(DirectFunctionCall2(int4mul, Int32GetDatum(arg0),
							Int32GetDatum(arg1)));

	PG_RETURN_INT32(result);
}

/* operator int2 / int2 */
Datum
myint2_div(PG_FUNCTION_ARGS)
{
	int64 arg0 = PG_GETARG_INT64(0);
	int64 arg1 = PG_GETARG_INT64(1);
	Numeric		res;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		PG_RETURN_NULL();

	if(arg1 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	res = DatumGetNumeric(DirectFunctionCall2(numeric_div, NumericGetDatum(int64_to_numeric(arg0)),
							NumericGetDatum(int64_to_numeric(arg1))));

	PG_RETURN_NUMERIC(res);
}

/* operator int2 div int2 */
Datum 
myint2_exact_div(PG_FUNCTION_ARGS)
{
	int64 arg0 = 0;
	int64 arg1 = 0;
	bool arg0_negative, arg1_negative, res_negative;
	uint64 uarg0 = 0;
	uint64 uarg1 = 0;
	uint64 res = 0;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		PG_RETURN_NULL();

	arg0 = PG_GETARG_INT64(0);
	arg1 = PG_GETARG_INT64(1);

	if(arg1 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	arg0_negative = (arg0 < 0);
	arg1_negative = (arg1 < 0);
	res_negative = (arg0_negative != arg1_negative);
	uarg0 = (uint64)(arg0_negative ? -arg0 : arg0);
	uarg1 = (uint64)(arg1_negative ? -arg1 : arg1);
	res = uarg0 / uarg1;
	if (res_negative)
	{
		res = (uint64)(-(int64)res);
	}
	PG_RETURN_INT32(res);
}

/* operator int2 % int2 */
Datum 
myint2_mod(PG_FUNCTION_ARGS)
{
	int64 arg0 = 0;
	int64 arg1 = 0;
	uint64 uarg0 = 0;
	uint64 uarg1 = 0;
	uint64 res = 0;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		PG_RETURN_NULL();
	
	arg0 = PG_GETARG_INT64(0);
	arg1 = PG_GETARG_INT64(1);

	if (arg1 == 0)
	{
		ereport(WARNING,
			(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
				errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	uarg0= (uint64) (arg0 < 0 ? -arg0 : arg0);
	uarg1= (uint64) (arg1 < 0 ? -arg1 : arg1);
	res= uarg0 % uarg1;
	PG_RETURN_INT32(arg0 < 0 ? -(int64)res : res);
}

/* operator int2 ^ int2*/
Datum 
myint2_bit_xor(PG_FUNCTION_ARGS)
{
	uint64 arg1 = PG_GETARG_UINT64(0);
	uint64 arg2 = PG_GETARG_UINT64(1);

	PG_RETURN_UINT64(arg1 ^ arg2);
}

/* operator int2 || int2 */
Datum 
myint2_or(PG_FUNCTION_ARGS)
{
	int64 arg0 = 0;
	int64 arg1 = 0;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
	{
		PG_RETURN_NULL();
	}

	arg0 = PG_GETARG_INT64(0);
	arg1 = PG_GETARG_INT64(1);

	if ((arg0 != 0) || (arg1 != 0))
	{
		PG_RETURN_INT32(1);
	}
	PG_RETURN_INT32(0);
}

/* operator int2 && int2 */
Datum 
myint2_and(PG_FUNCTION_ARGS)
{
	int64 arg0 = 0;
	int64 arg1 = 0;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
	{
		PG_RETURN_NULL();
	}

	arg0 = PG_GETARG_INT64(0);
	arg1 = PG_GETARG_INT64(1);

	if ((arg0 != 0) && (arg1 != 0))
	{
		PG_RETURN_INT32(1);
	}
	PG_RETURN_INT32(0);
}

/* operator int2 <=> int2 */
Datum 
myint2_safeeq(PG_FUNCTION_ARGS)
{
	int64 arg0 = 0;
	int64 arg1 = 0;

	bool is_null_a = PG_ARGISNULL(0);
	bool is_null_b = PG_ARGISNULL(1);

	if (is_null_a && is_null_b)
		PG_RETURN_INT32(1);
	if (is_null_a || is_null_b)
		PG_RETURN_INT32(0);

	arg0 = PG_GETARG_INT64(0);
	arg1 = PG_GETARG_INT64(1);

	PG_RETURN_INT32(arg0 == arg1 ? 1 : 0);
}

/* operator !int2 */
Datum 
myint2_not(PG_FUNCTION_ARGS)
{
	int64 arg1 = PG_GETARG_INT64(0);

	if (PG_ARGISNULL(0))
	{
		PG_RETURN_NULL();
	}

	PG_RETURN_INT32(arg1 == 0 ? 1 : 0);
}

Datum myint2_bit_neg(PG_FUNCTION_ARGS)
{
	uint64 arg1 = PG_GETARG_UINT64(0);

	PG_RETURN_UINT64(~arg1);
}

/* operator +int2 */
Datum 
myint2_positive(PG_FUNCTION_ARGS)
{
	int64 num = PG_GETARG_INT64(0);

	PG_RETURN_INT16(num);
}

/* operator -int2 */
Datum 
myint2_negative(PG_FUNCTION_ARGS)
{
	int64 num = PG_GETARG_INT32(0);

	PG_RETURN_INT64(-num);
}

/* cast to boolean */
Datum 
myint2_boolean(PG_FUNCTION_ARGS)
{
	int64	val1 = PG_GETARG_INT64(0);

	PG_RETURN_BOOL(val1 != 0);
}