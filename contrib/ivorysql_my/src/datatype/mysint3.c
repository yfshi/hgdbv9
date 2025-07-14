/*------------------------------------------------------------------
 * 
 * File: mysint3.c
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
 *		contrib/ivorysql_my/src/datatype/mysint3.c
 *
 *------------------------------------------------------------------
 */
#include "postgres.h"
#include "common/int.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/numeric.h"

PG_FUNCTION_INFO_V1(myint3_positive);
PG_FUNCTION_INFO_V1(myint3_negative);

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
