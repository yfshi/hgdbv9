/*------------------------------------------------------------------
 * 
 * File: mysuint3.c
 *
 * Abstract: 
 * 		Compatible with MySQL's UINT data type.
 *
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_my/src/datatype/mysuint3.c
 *
 *------------------------------------------------------------------
 */

#include "postgres.h"
#include "libpq/pqformat.h"
#include "utils/builtins.h"
#include "../include/ivorysql_my.h"

PG_FUNCTION_INFO_V1(uint3_in);
PG_FUNCTION_INFO_V1(uint3_out);
PG_FUNCTION_INFO_V1(uint3_recv);
PG_FUNCTION_INFO_V1(uint3_send);
PG_FUNCTION_INFO_V1(uint3_typmodin);
PG_FUNCTION_INFO_V1(uint3_typmodout);
PG_FUNCTION_INFO_V1(uint3_scale);
PG_FUNCTION_INFO_V1(int8_uint3);
PG_FUNCTION_INFO_V1(int4_uint3);
PG_FUNCTION_INFO_V1(int2_uint3);
PG_FUNCTION_INFO_V1(uint3_boolean);
PG_FUNCTION_INFO_V1(uint3_uint8);
PG_FUNCTION_INFO_V1(uint3_int8);

Datum
uint3_in(PG_FUNCTION_ARGS)
{
	int64	flag = 0;
	char	*num = PG_GETARG_CSTRING(0);
	uint32	ret;

	if (PG_NARGS() > 3)
		flag = PG_GETARG_INT64(3);

	ret = my_atoi_uint24(num, flag & TRANSFORM_FLAG_IN_INSERT);

	PG_RETURN_UINT32(ret);
}

Datum
uint3_out(PG_FUNCTION_ARGS)
{
	uint32 arg1 = PG_GETARG_UINT32(0);
	char *str = (char *) palloc(12);

	pg_ltoa(arg1, str);

	PG_RETURN_CSTRING(str);
}

Datum
uint3_recv(PG_FUNCTION_ARGS)
{
	StringInfo buf = (StringInfo)PG_GETARG_POINTER(0);

	PG_RETURN_UINT32((uint32)pq_getmsgint(buf, sizeof(uint32)));
}

Datum
uint3_send(PG_FUNCTION_ARGS)
{
	uint32 arg1 = PG_GETARG_UINT32(0);
	StringInfoData buf;

	pq_begintypsend(&buf);
	pq_sendint32(&buf, arg1);
	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

Datum
uint3_typmodin(PG_FUNCTION_ARGS)
{
	ArrayType	*ta = PG_GETARG_ARRAYTYPE_P(0);
	int32		*tl = NULL;
	int			n = 0;

	tl = ArrayGetIntegerTypmods(ta, &n);

	PG_RETURN_INT32(*tl);
}

Datum
uint3_typmodout(PG_FUNCTION_ARGS)
{
	int32	typmod = PG_GETARG_INT32(0);
	char	*res = (char *) palloc(64);

	if (typmod >= 0)
	{
		snprintf(res, 64, "(%d)",typmod);
	}
	else
	{
		*res = '\0';
	}

	PG_RETURN_CSTRING(res);
}

Datum
uint3_scale(PG_FUNCTION_ARGS)
{
	int64	num = PG_GETARG_INT64(0);

	PG_RETURN_UINT32(num);
}

/* Begin - BUG#Z7196 */
Datum
int8_uint3(PG_FUNCTION_ARGS)
{
	int64	val1 = PG_GETARG_INT64(0);
	uint32	val2 = val1;
	bool	is_insert = CALL_IN_INSERT();

	if (is_insert)
	{
		if (val1 < 0 || val1 > PG_UINT24_MAX)
		{
			int EREPORT_LEVEL = (!is_strict_mode()) ? WARNING : ERROR;
			ereport(EREPORT_LEVEL, (errmsg("Out of range value for column")));
			val2 = val1 < 0 ? 0 : PG_UINT24_MAX;
		}
	}
	else
	{
		if (val1 < 0 || val1 > PG_UINT24_MAX)
		{
			ereport(WARNING, (errmsg("integer unsigned out of range")));
			val2 = val1 < 0 ? 0 : PG_UINT24_MAX;
		}
	}

	PG_RETURN_UINT32(val2);
}

Datum
int4_uint3(PG_FUNCTION_ARGS)
{
	int32	val1 = PG_GETARG_INT32(0);
	uint32	val2 = val1;
	bool	is_insert = CALL_IN_INSERT();

	if (is_insert)
	{
		if (val1 < 0 || val1 > PG_UINT24_MAX)
		{
			int EREPORT_LEVEL = (!is_strict_mode()) ? WARNING : ERROR;
			ereport(EREPORT_LEVEL, (errmsg("Out of range value for column")));
			val2 = val1 < 0 ? 0 : PG_UINT24_MAX;
		}
	}
	else
	{
		if (val1 < 0 || val1 > PG_UINT24_MAX)
		{
			ereport(WARNING, (errmsg("integer unsigned out of range")));
			val2 = val1 < 0 ? 0 : PG_UINT24_MAX;
		}
	}

	PG_RETURN_UINT32(val2);
}

Datum
int2_uint3(PG_FUNCTION_ARGS)
{
	int16	val1 = PG_GETARG_INT16(0);
	uint32	val2 = val1;
	bool	is_insert = CALL_IN_INSERT();

	if (is_insert)
	{
		if (val1 < 0 || val1 > PG_UINT24_MAX)
		{
			int EREPORT_LEVEL = (!is_strict_mode()) ? WARNING : ERROR;
			ereport(EREPORT_LEVEL, (errmsg("Out of range value for column")));
			val2 = val1 < 0 ? 0 : PG_UINT24_MAX;
		}
	}
	else
	{
		if (val1 < 0 || val1 > PG_UINT24_MAX)
		{
			ereport(WARNING, (errmsg("integer unsigned out of range")));
			val2 = val1 < 0 ? 0 : PG_UINT24_MAX;
		}
	}

	PG_RETURN_UINT32(val2);
}
/* End - BUG#Z7196 */

Datum
uint3_boolean(PG_FUNCTION_ARGS)
{
	uint32	val1 = PG_GETARG_UINT32(0);

	PG_RETURN_BOOL(val1 != 0);
}

/* Begin - BUG#Z7200 */
Datum
uint3_uint8(PG_FUNCTION_ARGS)
{
	uint32	val1 = PG_GETARG_UINT32(0);

	PG_RETURN_UINT64(val1);
}

Datum
uint3_int8(PG_FUNCTION_ARGS)
{
	uint32	val1 = PG_GETARG_UINT32(0);

	PG_RETURN_INT64(val1);
}
/* End - BUG#Z7200 */