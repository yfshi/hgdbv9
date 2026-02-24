/*------------------------------------------------------------------
 * 
 * File: mysint4.c
 *
 * Abstract: 
 * 		Compatible with MySQL's INT4 data type.
 *
 * Authored by liyonghao@highgo.com, 20250421.
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_my/src/datatype/mysint4.c
 *
 * ReqID:SRS-MYSQL-NUMBERICTYPE-INT4
 * 
 *------------------------------------------------------------------
 */

#include "postgres.h"
#include "common/int.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/numeric.h"
#include "../include/ivorysql_my.h"

PG_FUNCTION_INFO_V1(int4_bit_and);
PG_FUNCTION_INFO_V1(int4_bit_or);
PG_FUNCTION_INFO_V1(int4_pl);
PG_FUNCTION_INFO_V1(int4_mi);
PG_FUNCTION_INFO_V1(int4_mul);
PG_FUNCTION_INFO_V1(int4_div);
PG_FUNCTION_INFO_V1(int4_exact_div);
PG_FUNCTION_INFO_V1(int4_mod);
PG_FUNCTION_INFO_V1(int4_bit_xor);
PG_FUNCTION_INFO_V1(int4_or);
PG_FUNCTION_INFO_V1(int4_and);
PG_FUNCTION_INFO_V1(int4_left_shift);
PG_FUNCTION_INFO_V1(int4_right_shift);
PG_FUNCTION_INFO_V1(int4_safeeq);
PG_FUNCTION_INFO_V1(int4_not);
PG_FUNCTION_INFO_V1(int4_bit_neg);
PG_FUNCTION_INFO_V1(int4_positive);
PG_FUNCTION_INFO_V1(int4_negative);

/* operator int4 & int4 */
Datum 
int4_bit_and(PG_FUNCTION_ARGS)
{
	uint64 arg1 = PG_GETARG_UINT64(0);
	uint64 arg2 = PG_GETARG_UINT64(1);

	PG_RETURN_UINT64(arg1 & arg2);
}

/* operator int4 | int4 */
Datum 
int4_bit_or(PG_FUNCTION_ARGS)
{
	uint64 arg1 = PG_GETARG_UINT64(0);
	uint64 arg2 = PG_GETARG_UINT64(1);

	PG_RETURN_UINT64(arg1 | arg2);
}

/* operator int4 + int4 */
Datum 
int4_pl(PG_FUNCTION_ARGS)
{
	int64 arg0 = PG_GETARG_INT64(0);
	int64 arg1 = PG_GETARG_INT64(1);
	int64 result;

	result = arg0 + arg1;

	PG_RETURN_INT64(result);
}

/* operator int4 - int4 */
Datum 
int4_mi(PG_FUNCTION_ARGS)
{
	int64 arg0 = PG_GETARG_INT64(0);
	int64 arg1 = PG_GETARG_INT64(1);
	int64 result = 0;
	result = arg0 - arg1;

	PG_RETURN_INT64(result);
}

/* operator int4 * int4 */
Datum 
int4_mul(PG_FUNCTION_ARGS)
{
	int64 arg0 = PG_GETARG_INT64(0);
	int64 arg1 = PG_GETARG_INT64(1);
	int64 result = 0;
	
	result = DatumGetInt64(DirectFunctionCall2(int8mul, Int64GetDatum(arg0),	/* BUG#5091 */
							Int64GetDatum(arg1)));

	PG_RETURN_INT64(result);
}

/* operator int4 / int4 */
Datum
int4_div(PG_FUNCTION_ARGS)
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

/* operator int4 div int4 */
Datum 
int4_exact_div(PG_FUNCTION_ARGS)
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

	if (arg1 == 0)
	{
		ereport(WARNING,
				(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
				 errmsg("Division by 0")));
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

/* operator int4 % int4 */
Datum 
int4_mod(PG_FUNCTION_ARGS)
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

/* operator int4 ^ int4 */
Datum 
int4_bit_xor(PG_FUNCTION_ARGS)
{
	uint64 arg1 = PG_GETARG_UINT64(0);
	uint64 arg2 = PG_GETARG_UINT64(1);

	PG_RETURN_UINT64(arg1 ^ arg2);
}

/* operator int2 || int2 */
Datum 
int4_or(PG_FUNCTION_ARGS)
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


/* operator int4 && int4 */
Datum 
int4_and(PG_FUNCTION_ARGS)
{
	int64 arg0 = 0;
	int64 arg1 = 0;

	/* Begin - BUG#5050 */
	if (PG_ARGISNULL(0) && PG_ARGISNULL(1))
	{
		PG_RETURN_NULL();
	}
	else if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
	{
		PG_RETURN_INT32(0);
	}
	/* End - BUG#5050 */

	arg0 = PG_GETARG_INT64(0);
	arg1 = PG_GETARG_INT64(1);

	if ((arg0 != 0) && (arg1 != 0))
	{
		PG_RETURN_INT32(1);
	}
	PG_RETURN_INT32(0);
}

/* operator int4 << int4 */
Datum 
int4_left_shift(PG_FUNCTION_ARGS)
{
	uint64 arg1 = PG_GETARG_UINT64(0);
	uint32 shift = PG_GETARG_UINT32(1);
	uint64 res = arg1 << shift;
	PG_RETURN_UINT64(shift < sizeof(int64) * 8 ? (int64)res : 0LL);
}

/* operator int4 >> int4 */
Datum 
int4_right_shift(PG_FUNCTION_ARGS)
{
	uint64 arg1 = PG_GETARG_UINT64(0);
	uint32 shift = PG_GETARG_UINT32(1);
	uint64 res = arg1 >> shift;
	PG_RETURN_UINT64(shift < sizeof(int64) * 8 ? (int64)res : 0LL);
}

/* operator int4 <=> int4 */
Datum 
int4_safeeq(PG_FUNCTION_ARGS)
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
int4_not(PG_FUNCTION_ARGS)
{
	int64 arg1 = PG_GETARG_INT64(0);

	if (PG_ARGISNULL(0))
	{
		PG_RETURN_NULL();
	}

	PG_RETURN_INT32(arg1 == 0 ? 1 : 0);
}

Datum int4_bit_neg(PG_FUNCTION_ARGS)
{
	uint64 arg1 = PG_GETARG_UINT64(0);

	PG_RETURN_UINT64(~arg1);
}

/* operator +int4 */
Datum 
int4_positive(PG_FUNCTION_ARGS)
{
	int64 num = PG_GETARG_INT64(0);

	PG_RETURN_INT16(num);
}

/* operator -int4 */
Datum 
int4_negative(PG_FUNCTION_ARGS)
{
	int64 num = PG_GETARG_INT32(0);

	PG_RETURN_INT64(-num);
}
