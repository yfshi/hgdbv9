#include "postgres.h"

#include "fmgr.h"
#include "utils/fmgrprotos.h"
#include "utils/guc.h"
#include "utils/numeric.h"

PG_FUNCTION_INFO_V1(int8_div);
PG_FUNCTION_INFO_V1(int8_div_int4);
PG_FUNCTION_INFO_V1(int8_exact_div);
PG_FUNCTION_INFO_V1(int8_mod);
PG_FUNCTION_INFO_V1(int8_bitand);
PG_FUNCTION_INFO_V1(int8_bitxor);
PG_FUNCTION_INFO_V1(int8_rightmov);
PG_FUNCTION_INFO_V1(int8_leftmov);
PG_FUNCTION_INFO_V1(int8_and);

Datum
int8_div(PG_FUNCTION_ARGS)
{
	int64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);
	Datum		res;

	if(val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));

		PG_RETURN_NULL();
	}

	res = DirectFunctionCall2(numeric_div,
							  NumericGetDatum(int64_to_numeric(val1)),
							  NumericGetDatum(int64_to_numeric(val2)));

	return res;
}

Datum
int8_div_int4(PG_FUNCTION_ARGS)
{
	int64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT32(1);
	Datum		res;

	if(val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));

		PG_RETURN_NULL();
	}

	res = DirectFunctionCall2(numeric_div,
							  NumericGetDatum(int64_to_numeric(val1)),
							  NumericGetDatum(int64_to_numeric(val2)));

	return res;
}

Datum
int8_exact_div(PG_FUNCTION_ARGS)
{
	int64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);
	
	if (val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));

		PG_RETURN_NULL();
	}

	PG_RETURN_INT64(val1 / val2);
}

Datum
int8_mod(PG_FUNCTION_ARGS)
{
	int64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);

	if (val2 < 0)
		PG_RETURN_UINT64(0);

	if (val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	PG_RETURN_UINT64(val1 % val2);
}

Datum
int8_bitand(PG_FUNCTION_ARGS)
{
	int64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 & val2);
}

Datum
int8_bitxor(PG_FUNCTION_ARGS)
{
	int64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 ^ val2);
}

Datum
int8_rightmov(PG_FUNCTION_ARGS)
{
	int64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);

	if (val2 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}

	PG_RETURN_UINT64(val1 >> val2);
}

Datum
int8_leftmov(PG_FUNCTION_ARGS)
{
	int64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);

	if (val2 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}

	PG_RETURN_UINT64(val1 << val2);
}

Datum
int8_and(PG_FUNCTION_ARGS)
{
	int64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_INT32(val1 && val2);
}
