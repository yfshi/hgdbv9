/*------------------------------------------------------------------
 * 
 * File: myuint4.c
 *
 * Abstract: 
 * 		Compatible with MySQL's UINT data type.
 *
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_my/src/datatype/mysuint8.c
 *
 *------------------------------------------------------------------
 */

#include "postgres.h"

#include <ctype.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <sys/time.h>

#include "access/hash.h"
#include "access/xact.h"
#include "catalog/pg_type.h"
#include "fmgr.h"
#include "funcapi.h"
#include "libpq/pqformat.h"
#include "miscadmin.h"
#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"
#include "parser/scansup.h"
#include "pgtime.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/datetime.h"
#include "utils/formatting.h"
#include "utils/guc.h"
#include "utils/numeric.h"
#include "utils/timestamp.h"
#include "utils/varlena.h"
#include "../include/ivorysql_my.h"

/* Begin - ReqID:SRS-MYSQL-NUMBERICTYPE-UINT8 */
PG_FUNCTION_INFO_V1(mysuint8_in);
PG_FUNCTION_INFO_V1(mysuint8_out);
PG_FUNCTION_INFO_V1(mysuint8_recv);
PG_FUNCTION_INFO_V1(mysuint8_send);
PG_FUNCTION_INFO_V1(mysuint8_typmodin);
PG_FUNCTION_INFO_V1(mysuint8_typmodout);
PG_FUNCTION_INFO_V1(mysuint8_scale);

PG_FUNCTION_INFO_V1(int_uint8);
PG_FUNCTION_INFO_V1(numeric_uint8);
PG_FUNCTION_INFO_V1(boolean_uint8);
PG_FUNCTION_INFO_V1(uint8_int2);
PG_FUNCTION_INFO_V1(uint8_int4);
PG_FUNCTION_INFO_V1(uint8_int8);
PG_FUNCTION_INFO_V1(uint8_uint1);
PG_FUNCTION_INFO_V1(uint8_uint2);
PG_FUNCTION_INFO_V1(uint8_uint4);
PG_FUNCTION_INFO_V1(uint8_float4);
PG_FUNCTION_INFO_V1(uint8_float8);

PG_FUNCTION_INFO_V1(mysuint8_eq);
PG_FUNCTION_INFO_V1(mysuint8_eqint2);
PG_FUNCTION_INFO_V1(mysuint8_eqint4);
PG_FUNCTION_INFO_V1(mysuint8_eqint8);
PG_FUNCTION_INFO_V1(int2_eqmysuint8);
PG_FUNCTION_INFO_V1(int4_eqmysuint8);
PG_FUNCTION_INFO_V1(int8_eqmysuint8);

PG_FUNCTION_INFO_V1(mysuint8_ne);
PG_FUNCTION_INFO_V1(mysuint8_neint2);
PG_FUNCTION_INFO_V1(mysuint8_neint4);
PG_FUNCTION_INFO_V1(mysuint8_neint8);
PG_FUNCTION_INFO_V1(int2_nemysuint8);
PG_FUNCTION_INFO_V1(int4_nemysuint8);
PG_FUNCTION_INFO_V1(int8_nemysuint8);

PG_FUNCTION_INFO_V1(mysuint8_lt);
PG_FUNCTION_INFO_V1(mysuint8_ltint2);
PG_FUNCTION_INFO_V1(mysuint8_ltint4);
PG_FUNCTION_INFO_V1(mysuint8_ltint8);
PG_FUNCTION_INFO_V1(int2_ltmysuint8);
PG_FUNCTION_INFO_V1(int4_ltmysuint8);
PG_FUNCTION_INFO_V1(int8_ltmysuint8);

PG_FUNCTION_INFO_V1(mysuint8_gt);
PG_FUNCTION_INFO_V1(mysuint8_gtint2);
PG_FUNCTION_INFO_V1(mysuint8_gtint4);
PG_FUNCTION_INFO_V1(mysuint8_gtint8);
PG_FUNCTION_INFO_V1(int2_gtmysuint8);
PG_FUNCTION_INFO_V1(int4_gtmysuint8);
PG_FUNCTION_INFO_V1(int8_gtmysuint8);

PG_FUNCTION_INFO_V1(mysuint8_le);
PG_FUNCTION_INFO_V1(mysuint8_leint2);
PG_FUNCTION_INFO_V1(mysuint8_leint4);
PG_FUNCTION_INFO_V1(mysuint8_leint8);
PG_FUNCTION_INFO_V1(int2_lemysuint8);
PG_FUNCTION_INFO_V1(int4_lemysuint8);
PG_FUNCTION_INFO_V1(int8_lemysuint8);

PG_FUNCTION_INFO_V1(mysuint8_ge);
PG_FUNCTION_INFO_V1(mysuint8_geint2);
PG_FUNCTION_INFO_V1(mysuint8_geint4);
PG_FUNCTION_INFO_V1(mysuint8_geint8);
PG_FUNCTION_INFO_V1(int2_gemysuint8);
PG_FUNCTION_INFO_V1(int4_gemysuint8);
PG_FUNCTION_INFO_V1(int8_gemysuint8);

PG_FUNCTION_INFO_V1(mysuint8_seq);
PG_FUNCTION_INFO_V1(mysuint8_seqint2);
PG_FUNCTION_INFO_V1(mysuint8_seqint4);
PG_FUNCTION_INFO_V1(mysuint8_seqint8);
PG_FUNCTION_INFO_V1(int2_seqmysuint8);
PG_FUNCTION_INFO_V1(int4_seqmysuint8);
PG_FUNCTION_INFO_V1(int8_seqmysuint8);

PG_FUNCTION_INFO_V1(mysuint8_addmysuint8);
PG_FUNCTION_INFO_V1(mysuint8_addint2);
PG_FUNCTION_INFO_V1(mysuint8_addint4);
PG_FUNCTION_INFO_V1(mysuint8_addint8);
PG_FUNCTION_INFO_V1(int2_addmysuint8);
PG_FUNCTION_INFO_V1(int4_addmysuint8);
PG_FUNCTION_INFO_V1(int8_addmysuint8);

PG_FUNCTION_INFO_V1(mysuint8_negative);

PG_FUNCTION_INFO_V1(mysuint8_submysuint8);
PG_FUNCTION_INFO_V1(mysuint8_subint2);
PG_FUNCTION_INFO_V1(mysuint8_subint4);
PG_FUNCTION_INFO_V1(mysuint8_subint8);
PG_FUNCTION_INFO_V1(int2_submysuint8);
PG_FUNCTION_INFO_V1(int4_submysuint8);
PG_FUNCTION_INFO_V1(int8_submysuint8);

PG_FUNCTION_INFO_V1(mysuint8_mulmysuint8);
PG_FUNCTION_INFO_V1(mysuint8_mulint2);
PG_FUNCTION_INFO_V1(mysuint8_mulint4);
PG_FUNCTION_INFO_V1(mysuint8_mulint8);
PG_FUNCTION_INFO_V1(int2_mulmysuint8);
PG_FUNCTION_INFO_V1(int4_mulmysuint8);
PG_FUNCTION_INFO_V1(int8_mulmysuint8);

PG_FUNCTION_INFO_V1(mysuint8_divmysuint8);
PG_FUNCTION_INFO_V1(mysuint8_divint2);
PG_FUNCTION_INFO_V1(mysuint8_divint4);
PG_FUNCTION_INFO_V1(mysuint8_divint8);
PG_FUNCTION_INFO_V1(int2_divmysuint8);
PG_FUNCTION_INFO_V1(int4_divmysuint8);
PG_FUNCTION_INFO_V1(int8_divmysuint8);

PG_FUNCTION_INFO_V1(mysuint8_exact_divmysuint8);
PG_FUNCTION_INFO_V1(mysuint8_exact_divint2);
PG_FUNCTION_INFO_V1(mysuint8_exact_divint4);
PG_FUNCTION_INFO_V1(mysuint8_exact_divint8);
PG_FUNCTION_INFO_V1(int2_exact_divmysuint8);
PG_FUNCTION_INFO_V1(int4_exact_divmysuint8);
PG_FUNCTION_INFO_V1(int8_exact_divmysuint8);

PG_FUNCTION_INFO_V1(mysuint8_modmysuint8);
PG_FUNCTION_INFO_V1(mysuint8_modint2);
PG_FUNCTION_INFO_V1(mysuint8_modint4);
PG_FUNCTION_INFO_V1(mysuint8_modint8);
PG_FUNCTION_INFO_V1(int2_modmysuint8);
PG_FUNCTION_INFO_V1(int4_modmysuint8);
PG_FUNCTION_INFO_V1(int8_modmysuint8);

PG_FUNCTION_INFO_V1(mysuint8_bitandmysuint8);
PG_FUNCTION_INFO_V1(mysuint8_bitandint2);
PG_FUNCTION_INFO_V1(mysuint8_bitandint4);
PG_FUNCTION_INFO_V1(mysuint8_bitandint8);
PG_FUNCTION_INFO_V1(int2_bitandmysuint8);
PG_FUNCTION_INFO_V1(int4_bitandmysuint8);
PG_FUNCTION_INFO_V1(int8_bitandmysuint8);

PG_FUNCTION_INFO_V1(mysuint8_bitormysuint8);
PG_FUNCTION_INFO_V1(mysuint8_bitorint2);
PG_FUNCTION_INFO_V1(mysuint8_bitorint4);
PG_FUNCTION_INFO_V1(mysuint8_bitorint8);
PG_FUNCTION_INFO_V1(int2_bitormysuint8);
PG_FUNCTION_INFO_V1(int4_bitormysuint8);
PG_FUNCTION_INFO_V1(int8_bitormysuint8);

PG_FUNCTION_INFO_V1(mysuint8_negamysuint8);

PG_FUNCTION_INFO_V1(mysuint8_bitxormysuint8);
PG_FUNCTION_INFO_V1(mysuint8_bitxorint2);
PG_FUNCTION_INFO_V1(mysuint8_bitxorint4);
PG_FUNCTION_INFO_V1(mysuint8_bitxorint8);
PG_FUNCTION_INFO_V1(int2_bitxormysuint8);
PG_FUNCTION_INFO_V1(int4_bitxormysuint8);
PG_FUNCTION_INFO_V1(int8_bitxormysuint8);

PG_FUNCTION_INFO_V1(mysuint8_rightmovmysuint8);
PG_FUNCTION_INFO_V1(mysuint8_rightmovint2);
PG_FUNCTION_INFO_V1(mysuint8_rightmovint4);
PG_FUNCTION_INFO_V1(mysuint8_rightmovint8);
PG_FUNCTION_INFO_V1(int2_rightmovmysuint8);
PG_FUNCTION_INFO_V1(int4_rightmovmysuint8);
PG_FUNCTION_INFO_V1(int8_rightmovmysuint8);

PG_FUNCTION_INFO_V1(mysuint8_leftmovmysuint8);
PG_FUNCTION_INFO_V1(mysuint8_leftmovint2);
PG_FUNCTION_INFO_V1(mysuint8_leftmovint4);
PG_FUNCTION_INFO_V1(mysuint8_leftmovint8);
PG_FUNCTION_INFO_V1(int2_leftmovmysuint8);
PG_FUNCTION_INFO_V1(int4_leftmovmysuint8);
PG_FUNCTION_INFO_V1(int8_leftmovmysuint8);

PG_FUNCTION_INFO_V1(mysuint8_bit_count);

PG_FUNCTION_INFO_V1(mysuint8_andmysuint8);
PG_FUNCTION_INFO_V1(mysuint8_andint2);
PG_FUNCTION_INFO_V1(mysuint8_andint4);
PG_FUNCTION_INFO_V1(mysuint8_andint8);
PG_FUNCTION_INFO_V1(int2_andmysuint8);
PG_FUNCTION_INFO_V1(int4_andmysuint8);
PG_FUNCTION_INFO_V1(int8_andmysuint8);

PG_FUNCTION_INFO_V1(mysuint8_invertmysuint8);

PG_FUNCTION_INFO_V1(int8_boolean);
PG_FUNCTION_INFO_V1(mysuint8_boolean);
PG_FUNCTION_INFO_V1(mysuint8_numeric);

PG_FUNCTION_INFO_V1(mysuint8_like);
PG_FUNCTION_INFO_V1(mysuint8_int8_like);
PG_FUNCTION_INFO_V1(mysuint8_text_like);
PG_FUNCTION_INFO_V1(mysuint8_notlike);
PG_FUNCTION_INFO_V1(mysuint8_int8_notlike);
PG_FUNCTION_INFO_V1(mysuint8_text_notlike);

PG_FUNCTION_INFO_V1(mysuint8_larger);
PG_FUNCTION_INFO_V1(mysuint8_smaller);

PG_FUNCTION_INFO_V1(mysuint8_avg_accum);
PG_FUNCTION_INFO_V1(mysuint8_avg);
PG_FUNCTION_INFO_V1(mysuint8_sum);

/*****************************************************************************
 *	 uint8 ADT
 *****************************************************************************/

/* 
 * mysuint8_in
 * Given uint8 text string, convert to internal uint8 format.
 */
Datum
mysuint8_in(PG_FUNCTION_ARGS)
{
	char 	*num = PG_GETARG_CSTRING(0);
	bool 	is_insert = CALL_IN_INSERT();
	uint64 	ret = my_atoi_uint64(num, is_insert);

	PG_RETURN_UINT64(ret);
}

/* 
 * mysuint8_out
 * Given internal format uint8, convert to text string.
 */
Datum
mysuint8_out(PG_FUNCTION_ARGS)
{
	uint64 arg1 = PG_GETARG_INT64(0);

	char *res = NULL;
	char *str = (char *)palloc(256); /* sign, 11 digits, '\0' */

	snprintf(str,256,"%llu", (long long unsigned int)arg1);

	res = str;

	PG_RETURN_CSTRING(res);
}

/*
 *mysuint8_recv			- converts external binary format to uint8
 */
Datum
mysuint8_recv(PG_FUNCTION_ARGS)
{
	StringInfo buf = (StringInfo)PG_GETARG_POINTER(0);

	PG_RETURN_UINT64((uint64)pq_getmsgint(buf, sizeof(uint64)));
}

/*
 *mysuint8_send			- converts uint8 to binary format
 */
Datum
mysuint8_send(PG_FUNCTION_ARGS)
{
	uint64 arg1 = PG_GETARG_INT64(0);
	StringInfoData buf;

	pq_begintypsend(&buf);
	pq_sendint32(&buf, arg1);
	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

/* common code for mysdatetime_typmodin */
Datum
mysuint8_typmodin(PG_FUNCTION_ARGS)
{
	ArrayType  *ta = PG_GETARG_ARRAYTYPE_P(0);
	int32	   *tl;
	int			n;

	tl = ArrayGetIntegerTypmods(ta, &n);

	PG_RETURN_INT32(*tl);
}


/* common code for mysdatetimetypmodout and mysdatetimetypmodout */
Datum
mysuint8_typmodout(PG_FUNCTION_ARGS)
{
	int32		typmod = PG_GETARG_INT32(0);
	char	   *res = (char *) palloc(64);

	if (typmod >= 0)
		snprintf(res, 64, "(%d)",typmod);
	else
		*res = '\0';

	PG_RETURN_CSTRING(res);
}

Datum
mysuint8_scale(PG_FUNCTION_ARGS)
{
	Numeric		num = PG_GETARG_NUMERIC(0);

	PG_RETURN_NUMERIC(num);
}

Datum
int_uint8(PG_FUNCTION_ARGS)
{
	int64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = val1;
	bool		is_insert = CALL_IN_INSERT();

	/* Begin - SRS-MYSQL-HEX-LITERAL */
	
	if (is_insert)
	{
		if (val1 < 0)
		{
			ereport(is_strict_mode() ? ERROR : WARNING, (errmsg("Out of range value for column")));
			val2 = 0;
		}
	}
	/* End - SRS-MYSQL-HEX-LITERAL */

	PG_RETURN_UINT64(val2);
}

Datum
numeric_uint8(PG_FUNCTION_ARGS)
{
	Numeric		num = PG_GETARG_NUMERIC(0);
	uint64		val2 = 0;
	uint64		val1 = 0;
	Numeric 	uint8maxnum;
	bool		is_insert = CALL_IN_INSERT();
	int 		level = WARNING;

	/* Begin - SRS-MYSQL-HEX-LITERAL */
	if (is_insert)
		level = !is_strict_mode() ? WARNING : ERROR;
	/* End - SRS-MYSQL-HEX-LITERAL */

	uint8maxnum = DatumGetNumeric(DirectFunctionCall3(numeric_in,
													  CStringGetDatum("18446744073709551615"),
													  ObjectIdGetDatum(InvalidOid),
													  Int32GetDatum(-1)));

	if (DatumGetBool(DirectFunctionCall2(numeric_gt,
										 NumericGetDatum(num),
										 NumericGetDatum(uint8maxnum))))
	{
		ereport(level, (errmsg("integer unsigned out of range")));
		PG_RETURN_UINT64(PG_UINT64_MAX);
	}

	if (DatumGetBool(DirectFunctionCall2(numeric_lt,
										 NumericGetDatum(num),
										 NumericGetDatum(int64_to_numeric(0)))))
	{
		ereport(level, (errmsg("integer unsigned out of range")));
		PG_RETURN_UINT64(0);
	}

	val1 = DatumGetInt64(DirectFunctionCall1(numeric_uint8, NumericGetDatum(num)));

	if (val1 < 0 || val1 > PG_UINT64_MAX)
	{
		ereport(level, (errmsg("integer unsigned out of range")));
		val1 = val1 < 0 ? 0 : PG_UINT64_MAX;
	}

	val2 = val1;

	PG_RETURN_UINT64(val2);
}

Datum
boolean_uint8(PG_FUNCTION_ARGS)
{
	PG_RETURN_UINT64(PG_GETARG_BOOL(0));
}
/*****************************************************************************
 *	 Operator Function														 *
 *****************************************************************************/
Datum
mysuint8_eq(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint8_eqint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint8_eqint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);
	
	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint8_eqint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);
	
	PG_RETURN_BOOL(val1 == val2);
}

Datum
int2_eqmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_BOOL(val2 == val1);
}

Datum
int4_eqmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);
	
	PG_RETURN_BOOL(val2 == val1);
}

Datum
int8_eqmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_BOOL(val2 == val1);
}


Datum
mysuint8_ne(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
mysuint8_neint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_BOOL(val1 != val2);
}


Datum
mysuint8_neint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
mysuint8_neint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 != val2);
}


Datum
int2_nemysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_BOOL(val2 != val1);
}

Datum
int4_nemysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_BOOL(val2 != val1);
}

Datum
int8_nemysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_BOOL(val2 != val1);
}


Datum
mysuint8_lt(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
mysuint8_ltint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 < val2);
}


Datum
mysuint8_ltint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
mysuint8_ltint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 < val2);
}


Datum
int2_ltmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);
	if(val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 < val1);
}

Datum
int4_ltmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);
	if(val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 < val1);
}

Datum
int8_ltmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);
	if(val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 < val1);
}

Datum
mysuint8_le(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
mysuint8_leint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);
	if(val2 < 0) PG_RETURN_BOOL(false);
	
	PG_RETURN_BOOL(val1 <= val2);
}

Datum
mysuint8_leint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
mysuint8_leint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
int2_lemysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);
	if(val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 <= val1);
}

Datum
int4_lemysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);
	if(val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 <= val1);
}

Datum
int8_lemysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);
	if(val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 <= val1);
}


Datum
mysuint8_gt(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
mysuint8_gtint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);
	if(val2 < 0) PG_RETURN_BOOL(true);
	
	PG_RETURN_BOOL(val1 > val2);
}

Datum
mysuint8_gtint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);
	if(val2 < 0) PG_RETURN_BOOL(true);
	
	PG_RETURN_BOOL(val1 > val2);
}

Datum
mysuint8_gtint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);
	if(val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
int2_gtmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 > val1);
}

Datum
int4_gtmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 > val1);
}

Datum
int8_gtmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 > val1);
}


Datum
mysuint8_ge(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 >= val2);
}

Datum
mysuint8_geint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);
	if(val2 < 0) PG_RETURN_BOOL(true);
	
	PG_RETURN_BOOL(val1 >= val2);
}

Datum
mysuint8_geint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);
	if(val2 < 0) PG_RETURN_BOOL(true);
	
	PG_RETURN_BOOL(val1 >= val2);
}

Datum
mysuint8_geint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);
	if(val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val1 >= val2);
}


Datum
int2_gemysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 >= val1);
}

Datum
int4_gemysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 >= val1);
}

Datum
int8_gemysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 < val1);
}

Datum
mysuint8_seq(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint8_seqint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint8_seqint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint8_seqint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
int2_seqmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_BOOL(val2 == val1);
}

Datum
int4_seqmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_BOOL(val2 == val1);
}

Datum
int8_seqmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_BOOL(val2 == val1);
}

Datum
mysuint8_addmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);
	if ((val1 + val2) < 0 || (val1 + val2) > PG_UINT64_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val1 + val2);
}

Datum
mysuint8_addint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);
	if ((val1 + val2) < 0 || (val1 + val2) > PG_UINT64_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val1 + val2);
}

Datum
mysuint8_addint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);
	if ((val1 + val2) < 0 || (val1 + val2) > PG_UINT64_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val1 + val2);
}

Datum
mysuint8_addint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);
	if ((val1 + val2) < 0 || (val1 + val2) > PG_UINT64_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val1 + val2);
}

Datum
int2_addmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);
	if ((val1 + val2) < 0 || (val1 + val2) > PG_UINT64_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val1 + val2);
}

Datum
int4_addmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);
	if ((val1 + val2) < 0 || (val1 + val2) > PG_UINT64_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val1 + val2);
}

Datum
int8_addmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);
	if ((val1 + val2) < 0 || (val1 + val2) > PG_UINT64_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}	
	PG_RETURN_UINT64(val1 + val2);
}

Datum
mysuint8_negative(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	int64		tempvalue = (int64)val1;

	/* detemine if the min value of int64 overflows by comparing with (max int64 + 1) 
	 *   ' -(PG_INT64_MAX+1) = PG_INT64_MIN '
	 */
	if (val1 > ((uint64)PG_INT64_MAX+1))
	{
		ereport(ERROR, (errmsg("BIGINT unsigned out of range")));
	}	
	PG_RETURN_INT64(-tempvalue);
}

Datum
mysuint8_submysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);
	if ((val1 < val2) || (val1 - val2) > PG_UINT64_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}

	PG_RETURN_UINT64(val1 - val2);
}

Datum
mysuint8_subint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);
	if ((val1 - val2) < 0 || (val1 - val2) > PG_UINT64_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val1 - val2);
}

Datum
mysuint8_subint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);
	if ((val1 - val2) < 0 || (val1 - val2) > PG_UINT64_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val1 - val2);
}

Datum
mysuint8_subint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);
	if ((val1 - val2) < 0 || (val1 - val2) > PG_UINT64_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val1 - val2);
}

Datum
int2_submysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);
	if (val2 < 0 || (val2 - val1) < 0 || (val2 - val1) > PG_UINT64_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val2 - val1);
}

Datum
int4_submysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);
	if (val2 < 0 || (val2 - val1) < 0 || (val2 - val1) > PG_UINT64_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val2 - val1);
}

Datum
int8_submysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);
	if (val2 < 0 || (val2 - val1) < 0 || (val2 - val1) > PG_UINT64_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val2 - val1);
}

// *
Datum
mysuint8_mulmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);
	uint64		res = val1*val2;
	if (val2 < 0 || val2 > (PG_UINT64_MAX / val1))
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(res);
}
Datum
mysuint8_mulint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);
	uint64		res = val1*val2;
	if (val2 < 0 || val2 > (PG_UINT64_MAX / val1))
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(res);
}
Datum
mysuint8_mulint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);
	uint64		res = val1*val2;
	if (val2 < 0 || val2 > (PG_UINT64_MAX / val1))
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(res);
}
Datum
mysuint8_mulint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);
	uint64		res = val1*val2;
	if (val2 < 0 || val2 > (PG_UINT64_MAX / val1))
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(res);
}

Datum
int2_mulmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);
	uint64		res = val2*val1;
	if (val2 < 0 || val2 > (PG_UINT64_MAX / val1))
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(res);
}
Datum
int4_mulmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);
	uint64		res = val2*val1;
	if (val2 < 0 || val2 > (PG_UINT64_MAX / val1))
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(res);
}
Datum
int8_mulmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);
	uint64		res = val2*val1;
	if (val2 < 0 || val2 > (PG_UINT64_MAX / val1))
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(res);
}

// div
Datum
mysuint8_divmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);
	Numeric		res;

	if(val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	res = DatumGetNumeric(DirectFunctionCall2(numeric_div, NumericGetDatum(int64_to_numeric(val1)),
						NumericGetDatum(int64_to_numeric(val2))));

	PG_RETURN_NUMERIC(res);
	
}
Datum
mysuint8_divint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);
	Numeric		res;

	if(val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	res = DatumGetNumeric(DirectFunctionCall2(numeric_div, NumericGetDatum(int64_to_numeric(val1)),
						NumericGetDatum(int64_to_numeric(val2))));

	PG_RETURN_NUMERIC(res);
}
Datum
mysuint8_divint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);
	Numeric		res;

	if(val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	res = DatumGetNumeric(DirectFunctionCall2(numeric_div, NumericGetDatum(int64_to_numeric(val1)),
						NumericGetDatum(int64_to_numeric(val2))));

	PG_RETURN_NUMERIC(res);
}
Datum
mysuint8_divint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);
	Numeric		res;

	if(val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	res = DatumGetNumeric(DirectFunctionCall2(numeric_div, NumericGetDatum(int64_to_numeric(val1)),
						NumericGetDatum(int64_to_numeric(val2))));

	PG_RETURN_NUMERIC(res);
}

Datum
int2_divmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);
	Numeric		res;

	if(val1 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	res = DatumGetNumeric(DirectFunctionCall2(numeric_div, NumericGetDatum(int64_to_numeric(val2)),
						NumericGetDatum(int64_to_numeric(val1))));

	PG_RETURN_NUMERIC(res);
}
Datum
int4_divmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);
	Numeric		res;

	if(val1 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	res = DatumGetNumeric(DirectFunctionCall2(numeric_div, NumericGetDatum(int64_to_numeric(val2)),
						NumericGetDatum(int64_to_numeric(val1))));

	PG_RETURN_NUMERIC(res);
}
Datum
int8_divmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);
	Numeric		res;

	if(val1 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	res = DatumGetNumeric(DirectFunctionCall2(numeric_div, NumericGetDatum(int64_to_numeric(val2)),
						NumericGetDatum(int64_to_numeric(val1))));

	PG_RETURN_NUMERIC(res);
}

// exact_div
Datum
mysuint8_exact_divmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);
	if (val2 < 0 )
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	if(val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	
	PG_RETURN_UINT64(val1 / val2);
}
Datum
mysuint8_exact_divint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);
	if (val2 < 0 )
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	if(val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}	
	PG_RETURN_UINT64(val1 / val2);
}
Datum
mysuint8_exact_divint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);
	if (val2 < 0 )
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	if(val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	PG_RETURN_UINT64(val1 / val2);
}
Datum
mysuint8_exact_divint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);
	if (val2 < 0 )
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	if(val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}	
	PG_RETURN_UINT64(val1 / val2);
}

Datum
int2_exact_divmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);
	if (val2 < 0 )
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	if(val1 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}	

	PG_RETURN_UINT64(val2 / val1);
}
Datum
int4_exact_divmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);
	if (val2 < 0 )
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	if(val1 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	
	PG_RETURN_UINT64(val2 / val1);
}
Datum
int8_exact_divmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);
	if (val2 < 0 )
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	if(val1 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	
	PG_RETURN_UINT64(val2 / val1);
}


// mod
Datum
mysuint8_modmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);
	if (val2 < 0 )
	{
		PG_RETURN_UINT64(0);
	}
	if(val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	PG_RETURN_UINT64(val1 % val2);
}
Datum
mysuint8_modint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);
	if (val2 < 0 )
	{
		PG_RETURN_UINT64(0);
	}
	if(val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	PG_RETURN_UINT64(val1 % val2);
}
Datum
mysuint8_modint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);
	if (val2 < 0 )
	{
		PG_RETURN_UINT64(0);
	}
	if(val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	PG_RETURN_UINT64(val1 % val2);
}
Datum
mysuint8_modint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);
	if (val2 < 0 )
	{
		PG_RETURN_UINT64(0);
	}
	if(val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	PG_RETURN_UINT64(val1 % val2);
}

Datum
int2_modmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);
	if (val2 < 0 )
	{
		PG_RETURN_UINT64(0);
	}
	if(val1 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	PG_RETURN_UINT64(val2 % val1);
}
Datum
int4_modmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);
	if (val2 < 0 )
	{
		PG_RETURN_UINT64(0);
	}
	if(val1 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	PG_RETURN_UINT64(val2 % val1);
}
Datum
int8_modmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);
	if (val2 < 0 )
	{
		PG_RETURN_UINT64(0);
	}
	if(val1 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	PG_RETURN_UINT64(val2 % val1);
}

// bitand
Datum
mysuint8_bitandmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 & val2);
}
Datum
mysuint8_bitandint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);


	PG_RETURN_UINT64(val1 & val2);
}
Datum
mysuint8_bitandint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);

	PG_RETURN_UINT64(val1 & val2);
}
Datum
mysuint8_bitandint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 & val2);
}

Datum
int2_bitandmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_UINT64(val2 & val1);
}
Datum
int4_bitandmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_UINT64(val2 & val1);
}
Datum
int8_bitandmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_UINT64(val2 & val1);
}

// bitor
Datum
mysuint8_bitormysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 | val2);
}
Datum
mysuint8_bitorint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_UINT64(val1 | val2);
}
Datum
mysuint8_bitorint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);
	uint64		temp = (uint64)val2;
	PG_RETURN_UINT64(val1 | temp);
}
Datum
mysuint8_bitorint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 | val2);
}

Datum
int2_bitormysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_UINT64(val2 | val1);
}
Datum
int4_bitormysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_UINT64(val2 | val1);
}
Datum
int8_bitormysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_UINT64(val2 | val1);
}

// nega
Datum
mysuint8_negamysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		res;

	res = ~(uint64)val1;
	PG_RETURN_UINT64(res);
}

// bitxor
Datum
mysuint8_bitxormysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 ^ val2);
}
Datum
mysuint8_bitxorint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_UINT64(val1 ^ val2);
}
Datum
mysuint8_bitxorint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);

	PG_RETURN_UINT64(val1 ^ val2);
}
Datum
mysuint8_bitxorint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 ^ val2);
}

Datum
int2_bitxormysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_UINT64(val2 ^ val1);
}
Datum
int4_bitxormysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_UINT64(val2 ^ val1);
}
Datum
int8_bitxormysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_UINT64(val2 ^ val1);
}


// right move
Datum
mysuint8_rightmovmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	if(val2 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}

	PG_RETURN_UINT64(val1 >> val2);
}
Datum
mysuint8_rightmovint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);

	if(val2 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val1 >> val2);
}
Datum
mysuint8_rightmovint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);

	if(val2 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val1 >> val2);
}
Datum
mysuint8_rightmovint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);

	if(val2 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val1 >> val2);
}

Datum
int2_rightmovmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);

	if(val1 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val2 >> val1);
}
Datum
int4_rightmovmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);

	if(val1 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val2 >> val1);
}
Datum
int8_rightmovmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);

	if(val1 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val2 >> val1);
}


// left move
Datum
mysuint8_leftmovmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	if(val2 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val1 << val2);
}
Datum
mysuint8_leftmovint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);

	if(val2 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val1 << val2);
}
Datum
mysuint8_leftmovint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);

	if(val2 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val1 << val2);
}
Datum
mysuint8_leftmovint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);

	if(val2 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val1 << val2);
}

Datum
int2_leftmovmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);

	if(val1 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val2 << val1);
}
Datum
int4_leftmovmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);

	if(val1 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val2 << val1);
}
Datum
int8_leftmovmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);

	if(val1 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val2 << val1);
}

//BIT_COUNT
Datum
mysuint8_bit_count(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		count = 0;

	while (val1)
	{
		count += val1 & 1;
		val1 >>= 1;
	}
	PG_RETURN_UINT64(count);
}

// and &&
Datum
mysuint8_andmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_INT32(val1 && val2);
}
Datum
mysuint8_andint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_UINT64(val1 && val2);
}
Datum
mysuint8_andint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);

	PG_RETURN_UINT64(val1 && val2);
}
Datum
mysuint8_andint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 && val2);
}

Datum
int2_andmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_UINT64(val2 && val1);
}
Datum
int4_andmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_UINT64(val2 && val1);
}
Datum
int8_andmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_UINT64(val2 && val1);
}

// invert ! 
Datum
mysuint8_invertmysuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);

	PG_RETURN_INT32(!val1);
}

// int8 to boolean
Datum
int8_boolean(PG_FUNCTION_ARGS)
{
	if (PG_GETARG_INT64(0) == 0)
		PG_RETURN_BOOL(false);
	else
		PG_RETURN_BOOL(true);
}

// mysuint8 to boolean
Datum
mysuint8_boolean(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);

	PG_RETURN_BOOL(val1 != 0);
}

// uint8 to numeric
Datum
mysuint8_numeric(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);

	PG_RETURN_NUMERIC(uint64_to_numeric(val1));
}

//like
Datum
mysuint8_like(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint8_int8_like(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);
	if( val2 < 0)
	{
		PG_RETURN_BOOL(false);
	}
	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint8_text_like(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT32(0);
	Datum		val2 = PG_GETARG_DATUM(1);
	text		*val1text = NULL;
	bool		result =false;
	char		*val1str = (char *)palloc(256); /* sign, 11 digits, '\0' */

	pg_lltoa(val1, val1str);

	val1text = cstring_to_text(val1str);
	result = DatumGetBool(DirectFunctionCall2(textlike, Int64GetDatum(val1text), val2));

	PG_RETURN_BOOL(result);
}

Datum
mysuint8_notlike(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
mysuint8_int8_notlike(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);
	if( val2 < 0)
	{
		PG_RETURN_BOOL(true);
	}
	PG_RETURN_BOOL(val1 != val2);
}

Datum
mysuint8_text_notlike(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT32(0);
	Datum		val2 = PG_GETARG_DATUM(1);
	text		*val1text = NULL;
	bool		result =false;
	char		*val1str = (char *)palloc(256); /* sign, 11 digits, '\0' */

	pg_lltoa(val1, val1str);

	val1text = cstring_to_text(val1str);
	result = DatumGetBool(DirectFunctionCall2(textnlike, Int64GetDatum(val1text), val2));

	PG_RETURN_BOOL(result);
}

/* AGGREGATE */
Datum
mysuint8_larger(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 > val2 ? val1 : val2);
}

Datum
mysuint8_smaller(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 < val2 ? val1 : val2);
}

typedef struct Int8TransTypeData
{
	int64		count;
	Numeric		sum;
	MemoryContext agg_context;
} Int8TransTypeData;

Datum
mysuint8_avg_accum(PG_FUNCTION_ARGS)
{
	Int8TransTypeData *state;
	MemoryContext agg_context;
	MemoryContext old_context;
	state = PG_ARGISNULL(0) ? NULL : (Int8TransTypeData *) PG_GETARG_POINTER(0);

	/* Create the state data on the first call */
	if (state == NULL)
	{
		if (!AggCheckCallContext(fcinfo, &agg_context))
			elog(ERROR, "aggregate function called in non-aggregate context");

		old_context = MemoryContextSwitchTo(agg_context);

		state = (Int8TransTypeData *) palloc0(sizeof(Int8TransTypeData));
		state->count = 0;
		state->sum = uint64_to_numeric(0);
		state->agg_context = agg_context;
		MemoryContextSwitchTo(old_context);
	}

	if (!PG_ARGISNULL(1))
	{
		MemoryContext old_context;
		uint64		newval = 0;
		newval = PG_GETARG_UINT64(1);
		old_context = MemoryContextSwitchTo(state->agg_context);
		state->count++;
		state->sum = DatumGetNumeric(DirectFunctionCall2(numeric_add, NumericGetDatum(state->sum), NumericGetDatum(uint64_to_numeric(newval))));
		MemoryContextSwitchTo(old_context);
	}

	PG_RETURN_POINTER(state);
}

Datum
mysuint8_avg(PG_FUNCTION_ARGS)
{
	Int8TransTypeData *state;
	Datum		N_datum;
	Datum		sumX_datum;
	state = PG_ARGISNULL(0) ? NULL : (Int8TransTypeData *) PG_GETARG_POINTER(0);

	/* If there were no non-null inputs, return NULL */
	if (state == NULL || state->count == 0)
		PG_RETURN_NULL();
	
	sumX_datum = NumericGetDatum(state->sum);
	N_datum = NumericGetDatum(int64_to_numeric(state->count));

	PG_RETURN_DATUM(DirectFunctionCall2(numeric_div, sumX_datum, N_datum));
}

Datum
mysuint8_sum(PG_FUNCTION_ARGS)
{
	uint64		newval;

	if (PG_ARGISNULL(0))
	{
		/* No non-null input seen so far... */
		if (PG_ARGISNULL(1))
			PG_RETURN_NULL();	/* still no non-null */
		/* This is the first non-null input. */
		newval = (uint64) PG_GETARG_UINT64(1);

		PG_RETURN_NUMERIC(uint64_to_numeric(newval));
	}

	/*
	 * If we're invoked as an aggregate, we can cheat and modify our first
	 * parameter in-place to avoid palloc overhead. If not, we need to return
	 * the new value of the transition variable. (If int8 is pass-by-value,
	 * then of course this is useless as well as incorrect, so just ifdef it
	 * out.)
	 */
#ifndef USE_FLOAT8_BYVAL		/* controls int8 too */
	if (AggCheckCallContext(fcinfo, NULL))
	{
		int64	   *oldsum = (int64 *) PG_GETARG_POINTER(0);

		/* Leave the running sum unchanged in the new input is null */
		if (!PG_ARGISNULL(1))
			*oldsum = *oldsum + (int64) PG_GETARG_INT32(1);

		PG_RETURN_POINTER(oldsum);
	}
	else
#endif
	{
		Numeric		oldsum = PG_GETARG_NUMERIC(0);
		/* Leave sum unchanged if new input is null. */
		if (PG_ARGISNULL(1))
			PG_RETURN_NUMERIC(oldsum);

		/* OK to do the addition. */
		newval = (uint64) PG_GETARG_UINT64(1);

		oldsum = DatumGetNumeric(DirectFunctionCall2(numeric_add, NumericGetDatum(oldsum), NumericGetDatum(uint64_to_numeric(newval))));
		PG_RETURN_NUMERIC(oldsum);
	}
}
/* End - ReqID:SRS-MYSQL-NUMBERICTYPE-UINT8 */

/* Begin BUG#5048 5070 */
Datum
uint8_int2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	int16		val2 = val1;
	bool		is_insert = CALL_IN_INSERT();

	/* Begin - SRS-MYSQL-HEX-LITERAL */
	if (is_insert)
	{
		if (val1 > INT16_MAX)
		{
			ereport(is_strict_mode() ? ERROR : WARNING, (errmsg("Out of range value for column")));
			val2 = INT16_MAX;
		}
	}
	/* End - SRS-MYSQL-HEX-LITERAL */

	PG_RETURN_INT16(val2);
}

Datum
uint8_int4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	int32		val2 = val1;
	bool		is_insert = CALL_IN_INSERT();

	/* Begin - SRS-MYSQL-HEX-LITERAL */
	if (is_insert)
	{
		if (val1 > INT32_MAX)
		{
			ereport(is_strict_mode() ? ERROR : WARNING, (errmsg("Out of range value for column")));
			val2 = INT32_MAX;
		}
	}
	/* End - SRS-MYSQL-HEX-LITERAL */
	
	PG_RETURN_INT32(val2);
}

Datum
uint8_int8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	int64		val2 = val1;
	bool		is_insert = CALL_IN_INSERT();

	/* Begin - SRS-MYSQL-HEX-LITERAL */
	if (is_insert)
	{
		if (val1 > INT64_MAX)
		{
			ereport(is_strict_mode() ? ERROR : WARNING, (errmsg("Out of range value for column")));
			val2 = INT64_MAX;
		}
	}
	/* End - SRS-MYSQL-HEX-LITERAL */

	PG_RETURN_INT64(val2);
}

Datum
uint8_uint1(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint8		val2 = val1;
	bool		is_insert = CALL_IN_INSERT();

	/* Begin - SRS-MYSQL-HEX-LITERAL */
	if (is_insert)
	{
		if (val1 > UINT8_MAX)
		{
			ereport(is_strict_mode() ? ERROR : WARNING, (errmsg("Out of range value for column")));
			val2 = UINT8_MAX;
		}
	}
	/* End - SRS-MYSQL-HEX-LITERAL */
	
	PG_RETURN_UINT8(val2);
}

Datum
uint8_uint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint16		val2 = val1;
	bool		is_insert = CALL_IN_INSERT();

	/* Begin - SRS-MYSQL-HEX-LITERAL */
	if (is_insert)
	{
		if (val1 > UINT16_MAX)
		{
			ereport(is_strict_mode() ? ERROR : WARNING, (errmsg("Out of range value for column")));
			val2 = UINT16_MAX;
		}
	}
	/* End - SRS-MYSQL-HEX-LITERAL */

	PG_RETURN_UINT16(val2);
}

Datum
uint8_uint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	int32		val2 = val1;
	bool		is_insert = CALL_IN_INSERT();

	/* Begin - SRS-MYSQL-HEX-LITERAL */
	if (is_insert)
	{
		if (val1 > UINT32_MAX)
		{
			ereport(is_strict_mode() ? ERROR : WARNING, (errmsg("Out of range value for column")));
			val2 = UINT32_MAX;
		}
	}
	/* End - SRS-MYSQL-HEX-LITERAL */
	
	PG_RETURN_UINT32(val2);
}

Datum
uint8_float4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	float		val2 = val1;
	bool		is_insert = CALL_IN_INSERT();

	/* Begin - SRS-MYSQL-HEX-LITERAL */
	if (is_insert)
	{
		if (val1 > FLT_MAX)
		{
			ereport(is_strict_mode() ? ERROR : WARNING, (errmsg("Out of range value for column")));
			val2 = FLT_MAX;
		}
	}
	/* End - SRS-MYSQL-HEX-LITERAL */
	
	PG_RETURN_FLOAT4(val2);
}

Datum
uint8_float8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	double		val2 = val1;
	bool		is_insert = CALL_IN_INSERT();

	/* Begin - SRS-MYSQL-HEX-LITERAL */
	if (is_insert)
	{
		if (val1 > DBL_MAX)
		{
			ereport(is_strict_mode() ? ERROR : WARNING, (errmsg("Out of range value for column")));
			val2 = DBL_MAX;
		}
	}
	/* End - SRS-MYSQL-HEX-LITERAL */

	PG_RETURN_FLOAT8(val2);
}
/* End BUG#5048 5070 */
