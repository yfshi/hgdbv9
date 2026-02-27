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
#include "libpq/pqformat.h"
#include "utils/builtins.h"
#include "../include/ivorysql_my.h"

PG_FUNCTION_INFO_V1(int3_in);
PG_FUNCTION_INFO_V1(int3_out);
PG_FUNCTION_INFO_V1(int3_recv);
PG_FUNCTION_INFO_V1(int3_send);
PG_FUNCTION_INFO_V1(int3_scale);
PG_FUNCTION_INFO_V1(int8_int3);
PG_FUNCTION_INFO_V1(int4_int3);
PG_FUNCTION_INFO_V1(int3_int8);
PG_FUNCTION_INFO_V1(int3_uint8);
PG_FUNCTION_INFO_V1(int3_boolean);
PG_FUNCTION_INFO_V1(int3_numeric);
PG_FUNCTION_INFO_V1(int3_positive);
PG_FUNCTION_INFO_V1(int3_negative);

/* Begin - ReqID: SRS-MYSQL-DATATYPE-INT3 */
Datum
int3_in(PG_FUNCTION_ARGS)
{
	int64 	flag = 0;
	char 	*num = PG_GETARG_CSTRING(0);
	int32 	ret;

	if (PG_NARGS() > 3)
		flag = PG_GETARG_INT64(3);

	ret = my_atoi_int24(num, flag & TRANSFORM_FLAG_IN_INSERT);

	PG_RETURN_INT32(ret);
}

Datum
int3_out(PG_FUNCTION_ARGS)
{
	int32 arg1 = PG_GETARG_INT32(0);
	char *str = (char *) palloc(12);

	pg_ltoa(arg1, str);

	PG_RETURN_CSTRING(str);
}

Datum
int3_recv(PG_FUNCTION_ARGS)
{
	StringInfo buf = (StringInfo)PG_GETARG_POINTER(0);

	PG_RETURN_INT32((int32)pq_getmsgint(buf, sizeof(int32)));
}

Datum
int3_send(PG_FUNCTION_ARGS)
{
	int32 arg1 = PG_GETARG_INT32(0);
	StringInfoData buf;

	pq_begintypsend(&buf);
	pq_sendint32(&buf, arg1);
	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

Datum
int3_scale(PG_FUNCTION_ARGS)
{
	int64		num = PG_GETARG_INT64(0);

	PG_RETURN_INT32(num);
}

Datum
int8_int3(PG_FUNCTION_ARGS)
{
	int64	val1 = PG_GETARG_INT64(0);
	int32	val2 = val1;
	bool	is_insert = CALL_IN_INSERT();
	
	if (is_insert)
	{
		if (val1 < PG_INT24_MIN || val1 > PG_INT24_MAX)
		{
			int EREPORT_LEVEL = (!is_strict_mode()) ? WARNING : ERROR;
			ereport(EREPORT_LEVEL, (errmsg("Out of range value for column")));
			val2 = val1 < 0 ? PG_INT24_MIN : PG_INT24_MAX;
		}
	}
	else
	{
		if (val1 < PG_INT24_MIN || val1 > PG_INT24_MAX)
		{
			ereport(WARNING, (errmsg("integer out of range")));
			val2 = val1 < 0 ? PG_INT24_MIN : PG_INT24_MAX;
		}
	}
	
	PG_RETURN_INT32(val2);
}

Datum
int4_int3(PG_FUNCTION_ARGS)
{
	int32	val1 = PG_GETARG_INT32(0);
	int32	val2 = val1;
	bool	is_insert = CALL_IN_INSERT();
	
	if (is_insert)
	{
		if (val1 < PG_INT24_MIN || val1 > PG_INT24_MAX)
		{
			int EREPORT_LEVEL = (!is_strict_mode()) ? WARNING : ERROR;
			ereport(EREPORT_LEVEL, (errmsg("Out of range value for column")));
			val2 = val1 < 0 ? PG_INT24_MIN : PG_INT24_MAX;
		}
	}
	else
	{
		if (val1 < PG_INT24_MIN || val1 > PG_INT24_MAX)
		{
			ereport(WARNING, (errmsg("integer out of range")));
			val2 = val1 < 0 ? PG_INT24_MIN : PG_INT24_MAX;
		}
	}
	
	PG_RETURN_INT32(val2);
}

/* Begin - BUG#Z7200 */
Datum
int3_int8(PG_FUNCTION_ARGS)
{
	int32	val1 = PG_GETARG_INT32(0);

	PG_RETURN_INT64(val1);
}

Datum
int3_uint8(PG_FUNCTION_ARGS)
{
	int32	val1 = PG_GETARG_INT32(0);
	uint64	val2 = val1;
	bool	is_insert = CALL_IN_INSERT();
	
	if (is_insert)
	{
		if (val1 < 0)
		{
			int EREPORT_LEVEL = (!is_strict_mode()) ? WARNING : ERROR;
			ereport(EREPORT_LEVEL, (errmsg("Out of range value for column")));
			val2 = 0;
		}
	}

	PG_RETURN_UINT64(val2);
}
/* End - BUG#Z7200 */

Datum
int3_numeric(PG_FUNCTION_ARGS)
{
	int32	val1 = PG_GETARG_INT32(0);

	PG_RETURN_NUMERIC(int64_to_numeric((int64) val1));
}

Datum
int3_boolean(PG_FUNCTION_ARGS)
{
	int32		val1 = PG_GETARG_INT32(0);

	PG_RETURN_BOOL(val1 != 0);
}
/* End - ReqID: SRS-MYSQL-DATATYPE-INT3 */

/* operator +int3 */
Datum
int3_positive(PG_FUNCTION_ARGS)
{
	int32 num = PG_GETARG_INT32(0);

	PG_RETURN_INT32(num);
}

/* operator -int3 */
Datum
int3_negative(PG_FUNCTION_ARGS)
{
	int32 num = PG_GETARG_INT32(0);

	PG_RETURN_INT32(-num);
}
