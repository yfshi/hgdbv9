#include "postgres.h"
#include "utils/builtins.h"
#include "utils/float.h"
#include "fmgr.h"
#include "../include/ivorysql_my.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

PG_FUNCTION_INFO_V1(mys_varchar2int2);
PG_FUNCTION_INFO_V1(mys_varchar2int4);
PG_FUNCTION_INFO_V1(mys_varchar2int8);
PG_FUNCTION_INFO_V1(mys_varchar2uint4);
PG_FUNCTION_INFO_V1(mys_varchar2uint8);

Datum
mys_varchar2int2(PG_FUNCTION_ARGS)
{
	char	   num[64];
	text	   *arg;
	int16	   i_arg = 0;

	if (PG_ARGISNULL(0))
		PG_RETURN_NULL();

	arg = PG_GETARG_TEXT_PP(0);

	extract_leading_integer_str(text_to_cstring(arg), num, sizeof(num));

	i_arg = DatumGetInt16(DirectFunctionCall1(int2in,
												CStringGetDatum(num)));

	PG_RETURN_INT16(i_arg);
}

Datum
mys_varchar2int4(PG_FUNCTION_ARGS)
{
	char	   *arg = text_to_cstring(PG_GETARG_VARCHAR_PP(0));
	char	   num[64];
	int32	   ret = 0;

	if (!extract_leading_integer_str(arg, num, sizeof(num)))
	{
		ereport(WARNING, 
				errmsg("Truncated incorrect INTEGER value: '%s'", arg));
	}

	ret = DatumGetInt32(DirectFunctionCall1(int4in,
											CStringGetDatum(num)));

	pfree(arg);
	PG_RETURN_INT32(ret);
}

Datum
mys_varchar2uint4(PG_FUNCTION_ARGS)
{
	char	   *arg = text_to_cstring(PG_GETARG_VARCHAR_PP(0));
	char	   num[64];
	uint32	   ret = 0;

	if (!extract_leading_integer_str(arg, num, sizeof(num)))
	{
		ereport(WARNING, 
				errmsg("Truncated incorrect INTEGER value: '%s'", arg));
	}

	ret = DatumGetInt32(DirectFunctionCall1(mysuint4_in,
											CStringGetDatum(num)));

	pfree(arg);
	PG_RETURN_UINT32(ret);
}

Datum
mys_varchar2int8(PG_FUNCTION_ARGS)
{
	char	   *arg = text_to_cstring(PG_GETARG_VARCHAR_PP(0));
	char	   num[64];
	int64	   ret = 0;

	if (!extract_leading_integer_str(arg, num, sizeof(num)))
	{
		ereport(WARNING, 
				errmsg("Truncated incorrect INTEGER value: '%s'", arg));
	}

	ret = DatumGetInt32(DirectFunctionCall1(int8in,
											CStringGetDatum(num)));

	pfree(arg);
	PG_RETURN_INT64(ret);
}

Datum
mys_varchar2uint8(PG_FUNCTION_ARGS)
{
	char	   *arg = text_to_cstring(PG_GETARG_VARCHAR_PP(0));
	char	   num[64];
	uint64	   ret = 0;

	if (!extract_leading_integer_str(arg, num, sizeof(num)))
	{
		ereport(WARNING, 
				errmsg("Truncated incorrect INTEGER value: '%s'", arg));
	}

	ret = DatumGetInt32(DirectFunctionCall1(mysuint8_in,
											CStringGetDatum(num)));

	pfree(arg);
	PG_RETURN_UINT64(ret);
}
