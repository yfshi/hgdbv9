/*------------------------------------------------------------------
 * 
 * File: mys_int3.c
 *
 * Abstract: 
 * 		Compatible with MySQL's int3 type.
 *
 * Authored by tanyang@highgo.com, 20250415.
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_my/src/datatype/mysigned.c
 *
 *------------------------------------------------------------------
 */
#include "postgres.h"
#include "utils/guc.h"
#include "common/int.h"
#include "utils/builtins.h"
#include "utils/numeric.h"

PG_FUNCTION_INFO_V1(myint3_div);
PG_FUNCTION_INFO_V1(myint3_positive);
PG_FUNCTION_INFO_V1(myint3_negative);

Datum
myint3_div(PG_FUNCTION_ARGS)
{
	int32 arg0 = PG_GETARG_INT32(0);
	int32 arg1 = PG_GETARG_INT32(1);
	static char *p_val = NULL;
	Numeric		val;
	Datum		res;

	if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
		PG_RETURN_NULL();

	val = DatumGetNumeric(DirectFunctionCall2(numeric_div, NumericGetDatum(int64_to_numeric(arg0)),
							NumericGetDatum(int64_to_numeric(arg1))));


	p_val = DatumGetCString(DirectFunctionCall1(numeric_out, NumericGetDatum(val)));

	res = DirectFunctionCall3(numeric_in,
	CStringGetDatum(p_val),
	ObjectIdGetDatum(InvalidOid),
	Int32GetDatum(((14 << 16) | 4) + VARHDRSZ));

	PG_RETURN_NUMERIC(NumericGetDatum(res));
}


/* operator +int3 */
Datum
myint3_positive(PG_FUNCTION_ARGS)
{
	int32 num = PG_GETARG_INT32(0);

	PG_RETURN_INT32(num);
}

/* operator -int3 */
Datum
myint3_negative(PG_FUNCTION_ARGS)
{
	int32 num = PG_GETARG_INT32(0);

	PG_RETURN_INT32(-num);
}

