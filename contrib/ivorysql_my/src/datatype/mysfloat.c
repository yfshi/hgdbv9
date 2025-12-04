/*------------------------------------------------------------------
 * 
 * File: mysfloat.c
 *
 * Abstract: 
 * 		Compatible with MySQL's float data type operator.
 *
 * Authored by jinhuajian@highgo.com, 20250407.
 *
 * Copyright:
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_my/src/datatype/mysfloat.c
 *
 *------------------------------------------------------------------
 */
#include "postgres.h"
#include <ctype.h>
#include <math.h>
#include <float.h>
#include "fmgr.h"
#include "utils/varlena.h"
#include "utils/numeric.h"
#include "utils/float.h"
#include "utils/fmgrprotos.h"
#include "utils/guc.h"


PG_FUNCTION_INFO_V1(mysfloat8_not);
PG_FUNCTION_INFO_V1(mysfloat4um);
PG_FUNCTION_INFO_V1(mysfloat8_logic_not);
PG_FUNCTION_INFO_V1(mysfloat4safeeq);
PG_FUNCTION_INFO_V1(mysfloat8safeeq);
PG_FUNCTION_INFO_V1(mysfloat4_and);
PG_FUNCTION_INFO_V1(mysfloat8_and);
PG_FUNCTION_INFO_V1(mysfloat8_or);
PG_FUNCTION_INFO_V1(mysfloat8_shl);
PG_FUNCTION_INFO_V1(mysfloat8_shr);
PG_FUNCTION_INFO_V1(mysfloat8_xor);
PG_FUNCTION_INFO_V1(mysfloat4_pl);
PG_FUNCTION_INFO_V1(mysfloat4_mi);
PG_FUNCTION_INFO_V1(mysfloat4_mul);
PG_FUNCTION_INFO_V1(mysfloat48_div);
PG_FUNCTION_INFO_V1(mysfloat8_div);
PG_FUNCTION_INFO_V1(mysfloat84_div);
PG_FUNCTION_INFO_V1(mysfloat4_moddiv);
PG_FUNCTION_INFO_V1(mysfloat8_moddiv);
PG_FUNCTION_INFO_V1(mysfloat8_mod);
PG_FUNCTION_INFO_V1(mysfloat8_logic_and);
PG_FUNCTION_INFO_V1(mysfloat4_boolean);
PG_FUNCTION_INFO_V1(mysfloat8_boolean);
/* Begin - BUG#Z5204 */
PG_FUNCTION_INFO_V1(mysfloat4larger);
PG_FUNCTION_INFO_V1(mysfloat84_pl);
/* End - BUG#Z5204 */
/* Begin - bug#z6040 */
PG_FUNCTION_INFO_V1(mysfloat4_moddiv_uint8);
PG_FUNCTION_INFO_V1(mysfloat8_moddiv_uint8);
/* End - bug#z6040 */

/*
 * mysfloat8_not - bitwise inversion.
 */
Datum
mysfloat8_not(PG_FUNCTION_ARGS)
{
	int64	val1;

	val1 = DatumGetInt64(DirectFunctionCall1(dtoi8, PG_GETARG_DATUM(0)));
	PG_RETURN_UINT64(~val1);
}


/*
 * mysfloat4um - returns -arg1.
 */
Datum
mysfloat4um(PG_FUNCTION_ARGS)
{
	float4	arg1 = PG_GETARG_FLOAT4(0);
	float8	result = 0.0;

	result = -arg1;
	PG_RETURN_FLOAT8(result);
}


/*
 * mysfloat8_logic_not - logical negation, the condition is true,
 * logical negation is false.
 */
Datum
mysfloat8_logic_not(PG_FUNCTION_ARGS)
{
	float8	arg1 = PG_GETARG_FLOAT8(0);

	if (PG_ARGISNULL(0))
	{
		PG_RETURN_NULL();
	}

	PG_RETURN_BOOL(arg1 != 0 ? 0 : 1);
}

/*
 * mysfloat4safeeq - float4 <=> float4.
 */
Datum
mysfloat4safeeq(PG_FUNCTION_ARGS)
{
	float4	arg1;
	float4	arg2;

	bool is_null_a = PG_ARGISNULL(0);
	bool is_null_b = PG_ARGISNULL(1);

	if (is_null_a && is_null_b)
		PG_RETURN_BOOL(true);
	
	if (is_null_a || is_null_b)
		PG_RETURN_BOOL(false);

	arg1 = PG_GETARG_FLOAT4(0);
	arg2 = PG_GETARG_FLOAT4(1);

	PG_RETURN_BOOL(float4_eq(arg1, arg2));
}

/*
 * mysfloat8safeeq - float8 <=> float8.
 */
Datum
mysfloat8safeeq(PG_FUNCTION_ARGS)
{
	float8	arg1;
	float8	arg2;

	bool is_null_a = PG_ARGISNULL(0);
	bool is_null_b = PG_ARGISNULL(1);

	if (is_null_a && is_null_b)
		PG_RETURN_BOOL(true);
	
	if (is_null_a || is_null_b)
		PG_RETURN_BOOL(false);

	arg1 = PG_GETARG_FLOAT8(0);
	arg2 = PG_GETARG_FLOAT8(1);

	PG_RETURN_BOOL(float8_eq(arg1, arg2));
}

/* Begin - bug#5378 */
Datum
mysfloat4_and(PG_FUNCTION_ARGS)
{
	uint64	val1;
	uint64	val2;

	val1 = DatumGetUInt64(DirectFunctionCall1(ftoi8, PG_GETARG_DATUM(0)));
	val2 = DatumGetUInt64(DirectFunctionCall1(ftoi8, PG_GETARG_DATUM(1)));

	PG_RETURN_UINT64(val1 & val2);
}
/* End - bug#5378 */

/* Begin - BUG#Z5355 */
/*
 * mysfloat8_and - bitwise AND.
 */
Datum
mysfloat8_and(PG_FUNCTION_ARGS)
{
	uint64	val1;
	uint64	val2;

	val1 = DatumGetUInt64(DirectFunctionCall1(dtoi8, PG_GETARG_DATUM(0)));
	val2 = DatumGetUInt64(DirectFunctionCall1(dtoi8, PG_GETARG_DATUM(1)));

	PG_RETURN_UINT64(val1 & val2);
}

/*
 * mysfloat8_or - bitwise OR.
 */
Datum
mysfloat8_or(PG_FUNCTION_ARGS)
{
	uint64	val1;
	uint64	val2;

	val1 = DatumGetUInt64(DirectFunctionCall1(dtoi8, PG_GETARG_DATUM(0)));
	val2 = DatumGetUInt64(DirectFunctionCall1(dtoi8, PG_GETARG_DATUM(1)));

	PG_RETURN_UINT64(val1 | val2);
}

/*
 * mysfloat8_shl - bitwise left shift.
 */
Datum
mysfloat8_shl(PG_FUNCTION_ARGS)
{
	uint64	val1;
	uint64	val2;
	
	val1 = DatumGetUInt64(DirectFunctionCall1(dtoi8, PG_GETARG_DATUM(0)));
	val2 = DatumGetUInt64(DirectFunctionCall1(dtoi8, PG_GETARG_DATUM(1)));

	if(val2 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}

	PG_RETURN_UINT64(val1 << val2);
}

/*
 * mysfloat8_shr - bitwise right shift.
 */
Datum
mysfloat8_shr(PG_FUNCTION_ARGS)
{
	uint64	val1;
	uint64	val2;
	
	val1 = DatumGetUInt64(DirectFunctionCall1(dtoi8, PG_GETARG_DATUM(0)));
	val2 = DatumGetUInt64(DirectFunctionCall1(dtoi8, PG_GETARG_DATUM(1)));

	if(val2 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}

	PG_RETURN_UINT64(val1 >> val2);
}

/*
 * mysfloat8_xor - float8 ^ float8.
 */
Datum
mysfloat8_xor(PG_FUNCTION_ARGS)
{
	uint64	val1;
	uint64	val2;

	val1 = DatumGetUInt64(DirectFunctionCall1(dtoi8, PG_GETARG_DATUM(0)));
	val2 = DatumGetUInt64(DirectFunctionCall1(dtoi8, PG_GETARG_DATUM(1)));

	PG_RETURN_UINT64(val1 ^ val2);
}
/* End - BUG#Z5355 */

/*
 * mysfloat8_pl - float4 + float4.
 */
Datum
mysfloat4_pl(PG_FUNCTION_ARGS)
{
	float4	arg1 = PG_GETARG_FLOAT4(0);
	float4	arg2 = PG_GETARG_FLOAT4(1);

	float8	result = arg1 + arg2;
	if (unlikely(isinf(result)) && !isinf(arg1) && !isinf(arg2))
		float_overflow_error();

	PG_RETURN_FLOAT8(result);
}

/*
 * mysfloat8_mi - float4 - float4.
 */
Datum
mysfloat4_mi(PG_FUNCTION_ARGS)
{
	float4	arg1 = PG_GETARG_FLOAT4(0);
	float4	arg2 = PG_GETARG_FLOAT4(1);

	float8	result = arg1 - arg2;
	if (unlikely(isinf(result)) && !isinf(arg1) && !isinf(arg2))
		float_overflow_error();

	PG_RETURN_FLOAT8(result);
}

/*
 * mysfloat8_mul - float4 * float4.
 */
Datum
mysfloat4_mul(PG_FUNCTION_ARGS)
{
	float4	arg1 = PG_GETARG_FLOAT4(0);
	float4	arg2 = PG_GETARG_FLOAT4(1);

	float8	result = arg1 * arg2;
	if (unlikely(isinf(result)) && !isinf(arg1) && !isinf(arg2))
		float_overflow_error();
	if (unlikely(result == 0.0) && arg1 != 0.0 && arg2 != 0.0)
		float_underflow_error();

	PG_RETURN_FLOAT8(result);
}

/*
 * mysfloat4_div - float4 / float8.
 */
Datum
mysfloat48_div(PG_FUNCTION_ARGS)
{
	float4	arg1 = PG_GETARG_FLOAT4(0);
	float8	arg2 = PG_GETARG_FLOAT8(1);
	double result = 0.0;

	if (arg2 == 0.0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING,
				(errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	result = arg1 / arg2;

	if (unlikely(isinf(result)) && !isinf(arg1))
		float_overflow_error();
	if (unlikely(result == 0.0) && arg1 != 0.0 && !isinf(arg2))
		float_underflow_error();

	PG_RETURN_FLOAT8(result);
}

/*
 * mysfloat8_div    - float8 / float8.
 */
Datum
mysfloat8_div(PG_FUNCTION_ARGS)
{
	float8	arg1 = PG_GETARG_FLOAT8(0);
	float8	arg2 = PG_GETARG_FLOAT8(1);
	double	result = 0.0;

	if (arg2 == 0.0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING,
				(errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	result = arg1 / arg2;

	if (unlikely(isinf(result)) && !isinf(arg1))
		float_overflow_error();
	if (unlikely(result == 0.0) && arg1 != 0.0 && !isinf(arg2))
		float_underflow_error();

	PG_RETURN_FLOAT8(result);
}

/*
 * mysfloat84_div    - float8 / float4.
 */
Datum
mysfloat84_div(PG_FUNCTION_ARGS)
{
	float8	arg1 = PG_GETARG_FLOAT8(0);
	float4	arg2 = PG_GETARG_FLOAT4(1);
	double	result = 0.0;

	if (arg2 == 0.0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING,
				(errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	result = arg1 / arg2;

	if (unlikely(isinf(result)) && !isinf(arg1))
		float_overflow_error();
	if (unlikely(result == 0.0) && arg1 != 0.0 && !isinf(arg2))
		float_underflow_error();

	PG_RETURN_FLOAT8(result);
}

/*
 * mysfloat4_moddiv - float4 // float4, return the integer part.
 */
Datum
mysfloat4_moddiv(PG_FUNCTION_ARGS)
{
	int64	num = 0;
	float4	arg1 = PG_GETARG_FLOAT4(0);
	float4	arg2 = PG_GETARG_FLOAT4(1);

	if (arg2 == 0.0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING,
				(errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	num = arg1 / arg2;

	PG_RETURN_INT64(num);
}

/*
 * mysfloat8_moddiv - float8 // float8, return the integer part.
 */
Datum
mysfloat8_moddiv(PG_FUNCTION_ARGS)
{
	int64	num = 0;
	float8	arg1 = PG_GETARG_FLOAT8(0);
	float8	arg2 = PG_GETARG_FLOAT8(1);

	if (arg2 == 0.0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING,
				(errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	num = arg1 / arg2;

	PG_RETURN_INT64(num);
}

/*
 * mysfloat8_mod - float8 % float8.
 */
Datum
mysfloat8_mod(PG_FUNCTION_ARGS)
{
	double	arg1 = PG_GETARG_FLOAT8(0);
	double	arg2 = PG_GETARG_FLOAT8(1);

	if (arg2 == 0.0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING,
				(errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	PG_RETURN_FLOAT8(fmod(arg1, arg2));
}

/*
 * mysfloat8_logic_and - float8 && float8.
 */
Datum
mysfloat8_logic_and(PG_FUNCTION_ARGS)
{
	float8	arg1 = PG_GETARG_FLOAT8(0);
	float8	arg2 = PG_GETARG_FLOAT8(1);

	bool	bool1 = (arg1 != 0.0 ? true : false);
	bool	bool2 = (arg2 != 0.0 ? true : false);

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
	{
		PG_RETURN_NULL();
	}

	if (bool1 && bool2)
	{
		PG_RETURN_BOOL(true);
	}

	PG_RETURN_BOOL(false);
}

/*
 * mysfloat4_boolean - convert float4 to boolean.
 */
Datum
mysfloat4_boolean(PG_FUNCTION_ARGS)
{
	int64	val1;

	val1 = DatumGetInt64(DirectFunctionCall1(ftoi8, PG_GETARG_DATUM(0)));
	PG_RETURN_BOOL(val1);
}

/*
 * mysfloat8_boolean - convert float8 to boolean.
 */
Datum
mysfloat8_boolean(PG_FUNCTION_ARGS)
{
	int64	val1;

	val1 = DatumGetInt64(DirectFunctionCall1(dtoi8, PG_GETARG_DATUM(0)));
	PG_RETURN_BOOL(val1);
}

/* Begin - BUG#Z5204 */
/*
 * mysfloat4larger - float8 > float4
 */
Datum
mysfloat4larger(PG_FUNCTION_ARGS)
{
	float8	arg1 = PG_GETARG_FLOAT8(0);
	float4	arg2 = PG_GETARG_FLOAT4(1);
	float8	result;

	if (float8_gt(arg1, arg2))
		result = arg1;
	else
		result = arg2;
	PG_RETURN_FLOAT8(result);
}

/*
 * mysfloat84_pl - float8 + float4.
 */
Datum
mysfloat84_pl(PG_FUNCTION_ARGS)
{
	float8	arg1 = PG_GETARG_FLOAT8(0);
	float4	arg2 = PG_GETARG_FLOAT4(1);

	float8	result = arg1 + arg2;
	if (unlikely(isinf(result)) && !isinf(arg1) && !isinf(arg2))
		float_overflow_error();

	PG_RETURN_FLOAT8(result);
}
/* End - BUG#Z5204 */

/* Begin - bug#z6040 */
Datum
mysfloat4_moddiv_uint8(PG_FUNCTION_ARGS)
{
	uint64	num = 0;
	float4	arg1 = PG_GETARG_FLOAT4(0);
	uint64	arg2 = PG_GETARG_UINT64(1);

	if (arg2 == 0.0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING,
				(errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	num = arg1 / arg2;

	PG_RETURN_UINT64(num);
}

Datum
mysfloat8_moddiv_uint8(PG_FUNCTION_ARGS)
{
	uint64	num = 0;
	float8	arg1 = PG_GETARG_FLOAT8(0);
	uint64	arg2 = PG_GETARG_UINT64(1);

	if (arg2 == 0.0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING,
				(errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	num = arg1 / arg2;

	PG_RETURN_UINT64(num);
}
/* End - bug#z6040 */
