/*------------------------------------------------------------------
 *
 * File: mysint1.c
 *
 * Abstract:
 * 		Compatible with MySQL's int1 type.
 *
 * Authored by tanyang@highgo.com, 20250415.
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *		contrib/ivorysql_my/src/datatype/mysint1.c
 *
 *------------------------------------------------------------------
 */

#include "postgres.h"
#include "common/int.h"
#include "libpq/pqformat.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "../include/ivorysql_my.h"

PG_FUNCTION_INFO_V1(int1_in);
PG_FUNCTION_INFO_V1(int1_out);
PG_FUNCTION_INFO_V1(int1_recv);
PG_FUNCTION_INFO_V1(int1_send);
PG_FUNCTION_INFO_V1(int1_scale);
PG_FUNCTION_INFO_V1(int1_int2);
PG_FUNCTION_INFO_V1(int1_int4);	/* bug#z8479 */
PG_FUNCTION_INFO_V1(int1_int8);
PG_FUNCTION_INFO_V1(int1_uint8);
PG_FUNCTION_INFO_V1(int8_int1);
PG_FUNCTION_INFO_V1(int4_int1);
PG_FUNCTION_INFO_V1(int2_int1);
PG_FUNCTION_INFO_V1(int1_boolean);
PG_FUNCTION_INFO_V1(boolean_int1);
PG_FUNCTION_INFO_V1(int1_numeric);
PG_FUNCTION_INFO_V1(int1_left_shift);
PG_FUNCTION_INFO_V1(int1_right_shift);
PG_FUNCTION_INFO_V1(int1_positive);

/* Begin - ReqID: SRS-MYSQL-DATATYPE-INT1 */
Datum
int1_in(PG_FUNCTION_ARGS)
{
	int64	flag = 0;
	char	*num = PG_GETARG_CSTRING(0);
	int8	ret;

	if (PG_NARGS() > 3)
		flag = PG_GETARG_INT64(3);

	ret = my_atoi_int8(num, flag & TRANSFORM_FLAG_IN_INSERT);

	PG_RETURN_INT8(ret);
}

Datum
int1_out(PG_FUNCTION_ARGS)
{
	int8	arg1 = PG_GETARG_INT8(0);
	char	*result = (char *) palloc(7);

	pg_itoa(arg1, result);
	PG_RETURN_CSTRING(result);
}

Datum
int1_recv(PG_FUNCTION_ARGS)
{
	StringInfo	buf = (StringInfo) PG_GETARG_POINTER(0);

	PG_RETURN_INT8((int8) pq_getmsgint(buf, sizeof(int8)));
}

Datum
int1_send(PG_FUNCTION_ARGS)
{
	int8	arg1 = PG_GETARG_INT8(0);
	StringInfoData buf;

	pq_begintypsend(&buf);
	pq_sendint8(&buf, arg1);
	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

Datum
int1_scale(PG_FUNCTION_ARGS)
{
	int8	num = PG_GETARG_INT8(0);

	PG_RETURN_INT8(num);
}

Datum
int1_int2(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);

	PG_RETURN_INT16(val1);
}

/* Begin - bug#8479 */
Datum
int1_int4(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);

	PG_RETURN_INT32(val1);
}
/* End - bug#8479 */

/* Begin - BUG#Z7200 */
Datum
int1_int8(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);

	PG_RETURN_INT64(val1);
}

Datum
int1_uint8(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);
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
int8_int1(PG_FUNCTION_ARGS)
{
	int64	val1 = PG_GETARG_INT64(0);
	int8	val2 = val1;
	bool	is_insert = CALL_IN_INSERT();

	if (is_insert)
	{
		if (val1 < PG_INT8_MIN || val1 > PG_INT8_MAX)
		{
			int EREPORT_LEVEL = (!is_strict_mode()) ? WARNING : ERROR;
			ereport(EREPORT_LEVEL, (errmsg("Out of range value for column")));
			val2 = val1 < 0 ? PG_INT8_MIN : PG_INT8_MAX;
		}
	}
	else
	{
		if (val1 < PG_INT8_MIN || val1 > PG_INT8_MAX)
		{
			ereport(WARNING, (errmsg("integer out of range")));
			val2 = val1 < 0 ? PG_INT8_MIN : PG_INT8_MAX;
		}
	}

	PG_RETURN_INT8(val2);
}

Datum
int4_int1(PG_FUNCTION_ARGS)
{
	int32	val1 = PG_GETARG_INT32(0);
	int8	val2 = val1;
	bool	is_insert = CALL_IN_INSERT();
	
	if (is_insert)
	{
		if (val1 < PG_INT8_MIN || val1 > PG_INT8_MAX)
		{
			int EREPORT_LEVEL = (!is_strict_mode()) ? WARNING : ERROR;
			ereport(EREPORT_LEVEL, (errmsg("Out of range value for column")));
			val2 = val1 < 0 ? PG_INT8_MIN : PG_INT8_MAX;
		}
	}
	else
	{
		if (val1 < PG_INT8_MIN || val1 > PG_INT8_MAX)
		{
			ereport(WARNING, (errmsg("integer out of range")));
			val2 = val1 < 0 ? PG_INT8_MIN : PG_INT8_MAX;
		}
	}

	PG_RETURN_INT8(val2);
}

Datum
int2_int1(PG_FUNCTION_ARGS)
{
	int16	val1 = PG_GETARG_INT32(0);
	int8	val2 = val1;
	bool	is_insert = CALL_IN_INSERT();
	
	if (is_insert)
	{
		if (val1 < PG_INT8_MIN || val1 > PG_INT8_MAX)
		{
			int EREPORT_LEVEL = (!is_strict_mode()) ? WARNING : ERROR;
			ereport(EREPORT_LEVEL, (errmsg("Out of range value for column")));
			val2 = val1 < 0 ? PG_INT8_MIN : PG_INT8_MAX;
		}
	}
	else
	{
		if (val1 < PG_INT8_MIN || val1 > PG_INT8_MAX)
		{
			ereport(WARNING, (errmsg("integer out of range")));
			val2 = val1 < 0 ? PG_INT8_MIN : PG_INT8_MAX;
		}
	}
	
	PG_RETURN_INT8(val2);
}

Datum
int1_boolean(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);

	PG_RETURN_BOOL(val1 != 0);
}

Datum
boolean_int1(PG_FUNCTION_ARGS)
{
	PG_RETURN_INT8(PG_GETARG_BOOL(0));
}

Datum
int1_numeric(PG_FUNCTION_ARGS)
{
	int8	val1 = PG_GETARG_INT8(0);

	PG_RETURN_NUMERIC(int64_to_numeric((int64) val1));
}

Datum
int1_left_shift(PG_FUNCTION_ARGS)
{
	uint64 arg1 = PG_GETARG_UINT64(0);
	uint32 shift = PG_GETARG_UINT32(1);
	uint64 res = arg1 << shift;
	PG_RETURN_UINT64(shift < sizeof(int64) * 8 ? (int64)res : 0LL);
}

Datum
int1_right_shift(PG_FUNCTION_ARGS)
{
	uint64 arg1 = PG_GETARG_UINT64(0);
	uint32 shift = PG_GETARG_UINT32(1);
	uint64 res = arg1 >> shift;
	PG_RETURN_UINT64(shift < sizeof(int64) * 8 ? (int64)res : 0LL);
}
/* End - ReqID: SRS-MYSQL-DATATYPE-INT1 */

Datum
int1_positive(PG_FUNCTION_ARGS)
{
	int8 num = PG_GETARG_INT8(0);

	PG_RETURN_INT8(num);
}

