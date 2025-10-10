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
 *		contrib/ivorysql_my/src/datatype/mysuint4.c
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

/* Begin - ReqID:SRS-MYSQL-NUMBERICTYPE-UINT4 */
typedef struct Int8TransTypeData
{
	int64		count;
	int64		sum;
} Int8TransTypeData;

PG_FUNCTION_INFO_V1(mysuint4_in);
PG_FUNCTION_INFO_V1(mysuint4_out);
PG_FUNCTION_INFO_V1(mysuint4_recv);
PG_FUNCTION_INFO_V1(mysuint4_send);
PG_FUNCTION_INFO_V1(mysuint4_typmodin);
PG_FUNCTION_INFO_V1(mysuint4_typmodout);
PG_FUNCTION_INFO_V1(mysuint4_scale);

PG_FUNCTION_INFO_V1(int_uint4);
PG_FUNCTION_INFO_V1(numeric_uint4);
PG_FUNCTION_INFO_V1(boolean_uint4);

PG_FUNCTION_INFO_V1(mysuint4_eq);
PG_FUNCTION_INFO_V1(mysuint4_eqint2);
PG_FUNCTION_INFO_V1(mysuint4_eqint4);
PG_FUNCTION_INFO_V1(mysuint4_eqint8);
PG_FUNCTION_INFO_V1(int2_eqmysuint4);
PG_FUNCTION_INFO_V1(int4_eqmysuint4);
PG_FUNCTION_INFO_V1(int8_eqmysuint4);

PG_FUNCTION_INFO_V1(mysuint4_ne);
PG_FUNCTION_INFO_V1(mysuint4_neint2);
PG_FUNCTION_INFO_V1(mysuint4_neint4);
PG_FUNCTION_INFO_V1(mysuint4_neint8);
PG_FUNCTION_INFO_V1(int2_nemysuint4);
PG_FUNCTION_INFO_V1(int4_nemysuint4);
PG_FUNCTION_INFO_V1(int8_nemysuint4);

PG_FUNCTION_INFO_V1(mysuint4_lt);
PG_FUNCTION_INFO_V1(mysuint4_ltint2);
PG_FUNCTION_INFO_V1(mysuint4_ltint4);
PG_FUNCTION_INFO_V1(mysuint4_ltint8);
PG_FUNCTION_INFO_V1(int2_ltmysuint4);
PG_FUNCTION_INFO_V1(int4_ltmysuint4);
PG_FUNCTION_INFO_V1(int8_ltmysuint4);

PG_FUNCTION_INFO_V1(mysuint4_gt);
PG_FUNCTION_INFO_V1(mysuint4_gtint2);
PG_FUNCTION_INFO_V1(mysuint4_gtint4);
PG_FUNCTION_INFO_V1(mysuint4_gtint8);
PG_FUNCTION_INFO_V1(int2_gtmysuint4);
PG_FUNCTION_INFO_V1(int4_gtmysuint4);
PG_FUNCTION_INFO_V1(int8_gtmysuint4);

PG_FUNCTION_INFO_V1(mysuint4_le);
PG_FUNCTION_INFO_V1(mysuint4_leint2);
PG_FUNCTION_INFO_V1(mysuint4_leint4);
PG_FUNCTION_INFO_V1(mysuint4_leint8);
PG_FUNCTION_INFO_V1(int2_lemysuint4);
PG_FUNCTION_INFO_V1(int4_lemysuint4);
PG_FUNCTION_INFO_V1(int8_lemysuint4);

PG_FUNCTION_INFO_V1(mysuint4_ge);
PG_FUNCTION_INFO_V1(mysuint4_geint2);
PG_FUNCTION_INFO_V1(mysuint4_geint4);
PG_FUNCTION_INFO_V1(mysuint4_geint8);
PG_FUNCTION_INFO_V1(int2_gemysuint4);
PG_FUNCTION_INFO_V1(int4_gemysuint4);
PG_FUNCTION_INFO_V1(int8_gemysuint4);

PG_FUNCTION_INFO_V1(mysuint4_seq);
PG_FUNCTION_INFO_V1(mysuint4_seqint2);
PG_FUNCTION_INFO_V1(mysuint4_seqint4);
PG_FUNCTION_INFO_V1(mysuint4_seqint8);
PG_FUNCTION_INFO_V1(int2_seqmysuint4);
PG_FUNCTION_INFO_V1(int4_seqmysuint4);
PG_FUNCTION_INFO_V1(int8_seqmysuint4);

PG_FUNCTION_INFO_V1(mysuint4_addmysuint4);
PG_FUNCTION_INFO_V1(mysuint4_addint2);
PG_FUNCTION_INFO_V1(mysuint4_addint4);
PG_FUNCTION_INFO_V1(mysuint4_addint8);
PG_FUNCTION_INFO_V1(int2_addmysuint4);
PG_FUNCTION_INFO_V1(int4_addmysuint4);
PG_FUNCTION_INFO_V1(int8_addmysuint4);

PG_FUNCTION_INFO_V1(mysuint4_negative);

PG_FUNCTION_INFO_V1(mysuint4_submysuint4);
PG_FUNCTION_INFO_V1(mysuint4_subint2);
PG_FUNCTION_INFO_V1(mysuint4_subint4);
PG_FUNCTION_INFO_V1(mysuint4_subint8);
PG_FUNCTION_INFO_V1(int2_submysuint4);
PG_FUNCTION_INFO_V1(int4_submysuint4);
PG_FUNCTION_INFO_V1(int8_submysuint4);

PG_FUNCTION_INFO_V1(mysuint4_mulmysuint4);
PG_FUNCTION_INFO_V1(mysuint4_mulint2);
PG_FUNCTION_INFO_V1(mysuint4_mulint4);
PG_FUNCTION_INFO_V1(mysuint4_mulint8);
PG_FUNCTION_INFO_V1(int2_mulmysuint4);
PG_FUNCTION_INFO_V1(int4_mulmysuint4);
PG_FUNCTION_INFO_V1(int8_mulmysuint4);

PG_FUNCTION_INFO_V1(mysuint4_divmysuint4);
PG_FUNCTION_INFO_V1(mysuint4_divint2);
PG_FUNCTION_INFO_V1(mysuint4_divint4);
PG_FUNCTION_INFO_V1(mysuint4_divint8);
PG_FUNCTION_INFO_V1(int2_divmysuint4);
PG_FUNCTION_INFO_V1(int4_divmysuint4);
PG_FUNCTION_INFO_V1(int8_divmysuint4);

PG_FUNCTION_INFO_V1(mysuint4_exact_divmysuint4);
PG_FUNCTION_INFO_V1(mysuint4_exact_divint2);
PG_FUNCTION_INFO_V1(mysuint4_exact_divint4);
PG_FUNCTION_INFO_V1(mysuint4_exact_divint8);
PG_FUNCTION_INFO_V1(int2_exact_divmysuint4);
PG_FUNCTION_INFO_V1(int4_exact_divmysuint4);
PG_FUNCTION_INFO_V1(int8_exact_divmysuint4);

PG_FUNCTION_INFO_V1(mysuint4_modmysuint4);
PG_FUNCTION_INFO_V1(mysuint4_modint2);
PG_FUNCTION_INFO_V1(mysuint4_modint4);
PG_FUNCTION_INFO_V1(mysuint4_modint8);
PG_FUNCTION_INFO_V1(int2_modmysuint4);
PG_FUNCTION_INFO_V1(int4_modmysuint4);
PG_FUNCTION_INFO_V1(int8_modmysuint4);

PG_FUNCTION_INFO_V1(mysuint4_bitandmysuint4);
PG_FUNCTION_INFO_V1(mysuint4_bitandint2);
PG_FUNCTION_INFO_V1(mysuint4_bitandint4);
PG_FUNCTION_INFO_V1(mysuint4_bitandint8);
PG_FUNCTION_INFO_V1(int2_bitandmysuint4);
PG_FUNCTION_INFO_V1(int4_bitandmysuint4);
PG_FUNCTION_INFO_V1(int8_bitandmysuint4);

PG_FUNCTION_INFO_V1(mysuint4_bitormysuint4);
PG_FUNCTION_INFO_V1(mysuint4_bitorint2);
PG_FUNCTION_INFO_V1(mysuint4_bitorint4);
PG_FUNCTION_INFO_V1(mysuint4_bitorint8);
PG_FUNCTION_INFO_V1(int2_bitormysuint4);
PG_FUNCTION_INFO_V1(int4_bitormysuint4);
PG_FUNCTION_INFO_V1(int8_bitormysuint4);

PG_FUNCTION_INFO_V1(mysuint4_negamysuint4);

PG_FUNCTION_INFO_V1(mysuint4_bitxormysuint4);
PG_FUNCTION_INFO_V1(mysuint4_bitxorint2);
PG_FUNCTION_INFO_V1(mysuint4_bitxorint4);
PG_FUNCTION_INFO_V1(mysuint4_bitxorint8);
PG_FUNCTION_INFO_V1(int2_bitxormysuint4);
PG_FUNCTION_INFO_V1(int4_bitxormysuint4);
PG_FUNCTION_INFO_V1(int8_bitxormysuint4);

PG_FUNCTION_INFO_V1(mysuint4_rightmovmysuint4);
PG_FUNCTION_INFO_V1(mysuint4_rightmovint2);
PG_FUNCTION_INFO_V1(mysuint4_rightmovint4);
PG_FUNCTION_INFO_V1(mysuint4_rightmovint8);
PG_FUNCTION_INFO_V1(int2_rightmovmysuint4);
PG_FUNCTION_INFO_V1(int4_rightmovmysuint4);
PG_FUNCTION_INFO_V1(int8_rightmovmysuint4);

PG_FUNCTION_INFO_V1(mysuint4_leftmovmysuint4);
PG_FUNCTION_INFO_V1(mysuint4_leftmovint2);
PG_FUNCTION_INFO_V1(mysuint4_leftmovint4);
PG_FUNCTION_INFO_V1(mysuint4_leftmovint8);
PG_FUNCTION_INFO_V1(int2_leftmovmysuint4);
PG_FUNCTION_INFO_V1(int4_leftmovmysuint4);
PG_FUNCTION_INFO_V1(int8_leftmovmysuint4);

PG_FUNCTION_INFO_V1(mysuint4_bit_count);

PG_FUNCTION_INFO_V1(mysuint4_andmysuint4);
PG_FUNCTION_INFO_V1(mysuint4_andint2);
PG_FUNCTION_INFO_V1(mysuint4_andint4);
PG_FUNCTION_INFO_V1(mysuint4_andint8);
PG_FUNCTION_INFO_V1(int2_andmysuint4);
PG_FUNCTION_INFO_V1(int4_andmysuint4);
PG_FUNCTION_INFO_V1(int8_andmysuint4);

PG_FUNCTION_INFO_V1(mysuint4_invertmysuint4);

PG_FUNCTION_INFO_V1(mysuint4_boolean);

PG_FUNCTION_INFO_V1(mysuint4_like);
PG_FUNCTION_INFO_V1(mysuint4_int8_like);
PG_FUNCTION_INFO_V1(mysuint4_notlike);
PG_FUNCTION_INFO_V1(mysuint4_int8_notlike);
PG_FUNCTION_INFO_V1(mysuint4_text_like);
PG_FUNCTION_INFO_V1(mysuint4_text_notlike);

PG_FUNCTION_INFO_V1(mysuint4_larger);
PG_FUNCTION_INFO_V1(mysuint4_smaller);

PG_FUNCTION_INFO_V1(mysuint4_avg_accum);
PG_FUNCTION_INFO_V1(mysuint4_sum);

/*****************************************************************************
 *	 uint4 ADT
 *****************************************************************************/

/* 
 * mysuint4_in
 * Given uint4 text string, convert to internal uint4 format.
 */
Datum
mysuint4_in(PG_FUNCTION_ARGS)
{
	char 	*num = PG_GETARG_CSTRING(0);
	bool	is_insert = CALL_IN_INSERT();
	uint32 	ret = my_atoi_uint32(num, is_insert);

	PG_RETURN_UINT32(ret);
}

/* 
 * mysuint4_out
 * Given internal format uint4, convert to text string.
 */
Datum
mysuint4_out(PG_FUNCTION_ARGS)
{
	uint32 arg1 = PG_GETARG_UINT32(0);
	char *str = (char *)palloc(256); /* sign, 11 digits, '\0' */

	pg_lltoa(arg1, str);

	PG_RETURN_CSTRING(str);
}

/*
 *mysuint4_recv			- converts external binary format to uint4
 */
Datum
mysuint4_recv(PG_FUNCTION_ARGS)
{
	StringInfo buf = (StringInfo)PG_GETARG_POINTER(0);

	PG_RETURN_UINT32((uint32)pq_getmsgint(buf, sizeof(uint32)));
}

/*
 *mysuint4_send			- converts uint4 to binary format
 */
Datum
mysuint4_send(PG_FUNCTION_ARGS)
{
	uint32 arg1 = PG_GETARG_UINT32(0);
	StringInfoData buf;

	pq_begintypsend(&buf);
	pq_sendint32(&buf, arg1);
	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

/* common code for mysdatetime_typmodin */
Datum
mysuint4_typmodin(PG_FUNCTION_ARGS)
{
	ArrayType  *ta = PG_GETARG_ARRAYTYPE_P(0);
	int32	   *tl = NULL;
	int			n = 0;

	tl = ArrayGetIntegerTypmods(ta, &n);

	PG_RETURN_INT32(*tl);
}


/* common code for mysdatetimetypmodout and mysdatetimetypmodout */
Datum
mysuint4_typmodout(PG_FUNCTION_ARGS)
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
mysuint4_scale(PG_FUNCTION_ARGS)
{
	int64		num = PG_GETARG_INT64(0);

	PG_RETURN_UINT32(num);
}

Datum
int_uint4(PG_FUNCTION_ARGS)
{
	int64		val1 = PG_GETARG_INT64(0);
	
	if (val1 < 0 || val1 > PG_UINT32_MAX)
	{
		int EREPORT_LEVEL = (!is_strict_mode()) ? WARNING : ERROR;
		ereport(EREPORT_LEVEL, (errmsg("integer unsigned out of range")));
		val1 = val1 < 0 ? 0 : PG_UINT32_MAX;
	}

	PG_RETURN_UINT32(val1);
}

Datum
numeric_uint4(PG_FUNCTION_ARGS)
{
	Numeric		num = PG_GETARG_NUMERIC(0);
	int64		val1 = 0;
	uint32		val2 = 0;
	/* Begin - SRS-MYSQL-HEX-LITERAL */
	bool		is_insert = CALL_IN_INSERT();

	val1 = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(num)));
	
	val2 = (uint32) val1;

	if (is_insert)
	{
		if (val1 > UINT32_MAX)
		{
			ereport(is_strict_mode() ? ERROR : WARNING, (errmsg("Out of range value for column")));
			val2 = UINT32_MAX;
		}

		if (val1 < 0)
		{
			ereport(is_strict_mode() ? ERROR : WARNING, (errmsg("Out of range value for column")));
			val2 = 0;
		}
	}
	/* End - SRS-MYSQL-HEX-LITERAL */

	PG_RETURN_UINT32(val2);
}

Datum
boolean_uint4(PG_FUNCTION_ARGS)
{
	PG_RETURN_UINT32(PG_GETARG_BOOL(0));
}
/*****************************************************************************
 *	 Operator Function														 *
 *****************************************************************************/
Datum
mysuint4_eq(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint4_eqint2(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint4_eqint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);
	
	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint4_eqint8(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);
	
	PG_RETURN_BOOL(val1 == val2);
}

Datum
int2_eqmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_BOOL(val2 == val1);
}

Datum
int4_eqmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);
	
	PG_RETURN_BOOL(val2 == val1);
}

Datum
int8_eqmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_BOOL(val2 == val1);
}


Datum
mysuint4_ne(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
mysuint4_neint2(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_BOOL(val1 != val2);
}


Datum
mysuint4_neint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
mysuint4_neint8(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 != val2);
}


Datum
int2_nemysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_BOOL(val2 != val1);
}

Datum
int4_nemysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_BOOL(val2 != val1);
}

Datum
int8_nemysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_BOOL(val2 != val1);
}


Datum
mysuint4_lt(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
mysuint4_ltint2(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 < val2);
}


Datum
mysuint4_ltint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
mysuint4_ltint8(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 < val2);
}


Datum
int2_ltmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);
	if(val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 < val1);
}

Datum
int4_ltmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);
	if(val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 < val1);
}

Datum
int8_ltmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);
	if(val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 < val1);
}

Datum
mysuint4_le(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
mysuint4_leint2(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);
	if(val2 < 0) PG_RETURN_BOOL(false);
	
	PG_RETURN_BOOL(val1 <= val2);
}

Datum
mysuint4_leint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
mysuint4_leint8(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
int2_lemysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);
	if(val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 <= val1);
}

Datum
int4_lemysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);
	if(val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 <= val1);
}

Datum
int8_lemysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);
	if(val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 <= val1);
}


Datum
mysuint4_gt(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
mysuint4_gtint2(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);
	if(val2 < 0) PG_RETURN_BOOL(true);
	
	PG_RETURN_BOOL(val1 > val2);
}

Datum
mysuint4_gtint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);
	if(val2 < 0) PG_RETURN_BOOL(true);
	
	PG_RETURN_BOOL(val1 > val2);
}

Datum
mysuint4_gtint8(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);
	if(val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
int2_gtmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 > val1);
}

Datum
int4_gtmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 > val1);
}

Datum
int8_gtmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 > val1);
}


Datum
mysuint4_ge(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 >= val2);
}

Datum
mysuint4_geint2(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);
	if(val2 < 0) PG_RETURN_BOOL(true);
	
	PG_RETURN_BOOL(val1 >= val2);
}

Datum
mysuint4_geint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);
	if(val2 < 0) PG_RETURN_BOOL(true);
	
	PG_RETURN_BOOL(val1 >= val2);
}

Datum
mysuint4_geint8(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);
	if(val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val1 >= val2);
}


Datum
int2_gemysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 >= val1);
}

Datum
int4_gemysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 >= val1);
}

Datum
int8_gemysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 < val1);
}

Datum
mysuint4_seq(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint4_seqint2(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint4_seqint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint4_seqint8(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
int2_seqmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_BOOL(val2 == val1);
}

Datum
int4_seqmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_BOOL(val2 == val1);
}

Datum
int8_seqmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_BOOL(val2 == val1);
}

Datum
mysuint4_addmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint32		val2 = PG_GETARG_UINT32(1);
	if ((val1 + val2) < 0 || (val1 + val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val1 + val2);
}

Datum
mysuint4_addint2(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);
	if ((val1 + val2) < 0 || (val1 + val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val1 + val2);
}

Datum
mysuint4_addint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);
	if ((val1 + val2) < 0 || (val1 + val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val1 + val2);
}

Datum
mysuint4_addint8(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);
	if ((val1 + val2) < 0 || (val1 + val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val1 + val2);
}

Datum
int2_addmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);
	if ((val1 + val2) < 0 || (val1 + val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val1 + val2);
}

Datum
int4_addmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);
	if ((val1 + val2) < 0 || (val1 + val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val1 + val2);
}

Datum
int8_addmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);
	if ((val1 + val2) < 0 || (val1 + val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}	
	PG_RETURN_UINT64(val1 + val2);
}

Datum
mysuint4_negative(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int64		tempvalue = (int64)val1;
	PG_RETURN_INT64(-tempvalue);
}

Datum
mysuint4_submysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint32		val2 = PG_GETARG_UINT32(1);
	if ((val1 < val2) || (val1 - val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}

	PG_RETURN_UINT32(val1 - val2);
}

Datum
mysuint4_subint2(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);
	if ((val1 - val2) < 0 || (val1 - val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT32(val1 - val2);
}

Datum
mysuint4_subint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);
	if ((val1 - val2) < 0 || (val1 - val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT32(val1 - val2);
}

Datum
mysuint4_subint8(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);
	if ((val1 - val2) < 0 || (val1 - val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT32(val1 - val2);
}

Datum
int2_submysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);
	if (val2 < 0 || (val2 - val1) < 0 || (val2 - val1) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT32(val2 - val1);
}

Datum
int4_submysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);
	if (val2 < 0 || (val2 - val1) < 0 || (val2 - val1) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT32(val2 - val1);
}

Datum
int8_submysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);
	if (val2 < 0 || (val2 - val1) < 0 || (val2 - val1) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT32(val2 - val1);
}

// *
Datum
mysuint4_mulmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint32		val2 = PG_GETARG_UINT32(1);
	uint32		res = val1*val2;
	if (val2 < 0 || val2 > (PG_UINT32_MAX / val1))
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(res);
}
Datum
mysuint4_mulint2(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);
	uint32		res = val1*val2;
	if (val2 < 0 || val2 > (PG_UINT32_MAX / val1))
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(res);
}
Datum
mysuint4_mulint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);
	uint32		res = val1*val2;
	if (val2 < 0 || val2 > (PG_UINT32_MAX / val1))
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(res);
}
Datum
mysuint4_mulint8(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);
	uint32		res = val1*val2;
	if (val2 < 0 || val2 > (PG_UINT32_MAX / val1))
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(res);
}

Datum
int2_mulmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);
	uint32		res = val2*val1;
	if (val2 < 0 || val2 > (PG_UINT32_MAX / val1))
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(res);
}
Datum
int4_mulmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);
	uint32		res = val2*val1;
	if (val2 < 0 || val2 > (PG_UINT32_MAX / val1))
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(res);
}
Datum
int8_mulmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);
	uint32		res = val2*val1;
	if (val2 < 0 || val2 > (PG_UINT32_MAX / val1))
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(res);
}

// div
Datum
mysuint4_divmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint32		val2 = PG_GETARG_UINT32(1);
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
mysuint4_divint2(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
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
mysuint4_divint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
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
mysuint4_divint8(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
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
int2_divmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
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
int4_divmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
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
int8_divmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
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
mysuint4_exact_divmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint32		val2 = PG_GETARG_UINT32(1);
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
	
	PG_RETURN_UINT32(val1 / val2);
}
Datum
mysuint4_exact_divint2(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
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
	PG_RETURN_UINT32(val1 / val2);
}
Datum
mysuint4_exact_divint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
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

	PG_RETURN_UINT32(val1 / val2);
}
Datum
mysuint4_exact_divint8(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
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
	PG_RETURN_UINT32(val1 / val2);
}

Datum
int2_exact_divmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
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

	PG_RETURN_UINT32(val2 / val1);
}
Datum
int4_exact_divmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
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
	
	PG_RETURN_UINT32(val2 / val1);
}
Datum
int8_exact_divmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
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
	
	PG_RETURN_UINT32(val2 / val1);
}


// mod
Datum
mysuint4_modmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint32		val2 = PG_GETARG_UINT32(1);
	if (val2 < 0 )
	{
		PG_RETURN_UINT32(0);
	}
	if(val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	PG_RETURN_UINT32(val1 % val2);
}
Datum
mysuint4_modint2(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);
	if (val2 < 0 )
	{
		PG_RETURN_UINT32(0);
	}
	if(val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	PG_RETURN_UINT32(val1 % val2);
}
Datum
mysuint4_modint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);
	if (val2 < 0 )
	{
		PG_RETURN_UINT32(0);
	}
	if(val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	PG_RETURN_UINT32(val1 % val2);
}
Datum
mysuint4_modint8(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);
	if (val2 < 0 )
	{
		PG_RETURN_UINT32(0);
	}
	if(val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	PG_RETURN_UINT32(val1 % val2);
}

Datum
int2_modmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);
	if (val2 < 0 )
	{
		PG_RETURN_UINT32(0);
	}
	if(val1 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	PG_RETURN_UINT32(val2 % val1);
}
Datum
int4_modmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);
	if (val2 < 0 )
	{
		PG_RETURN_UINT32(0);
	}
	if(val1 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	PG_RETURN_UINT32(val2 % val1);
}
Datum
int8_modmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);
	if (val2 < 0 )
	{
		PG_RETURN_UINT32(0);
	}
	if(val1 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	PG_RETURN_UINT32(val2 % val1);
}

// bitand
Datum
mysuint4_bitandmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_UINT64(val1 & val2);
}
Datum
mysuint4_bitandint2(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);


	PG_RETURN_UINT64(val1 & val2);
}
Datum
mysuint4_bitandint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);

	PG_RETURN_UINT64(val1 & val2);
}
Datum
mysuint4_bitandint8(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 & val2);
}

Datum
int2_bitandmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_UINT64(val2 & val1);
}
Datum
int4_bitandmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_UINT64(val2 & val1);
}
Datum
int8_bitandmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_UINT64(val2 & val1);
}

// bitor
Datum
mysuint4_bitormysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_UINT64(val1 | val2);
}
Datum
mysuint4_bitorint2(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_UINT64(val1 | val2);
}
Datum
mysuint4_bitorint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);

	PG_RETURN_UINT64(val1 | val2);
}
Datum
mysuint4_bitorint8(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 | val2);
}

Datum
int2_bitormysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_UINT64(val2 | val1);
}
Datum
int4_bitormysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_UINT64(val2 | val1);
}
Datum
int8_bitormysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_UINT64(val2 | val1);
}


// nega
Datum
mysuint4_negamysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint64		res;

	res = ~(uint64)val1;
	PG_RETURN_UINT64(res);
}

// bitxor
Datum
mysuint4_bitxormysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_UINT64(val1 ^ val2);
}
Datum
mysuint4_bitxorint2(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_UINT64(val1 ^ val2);
}
Datum
mysuint4_bitxorint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);

	PG_RETURN_UINT64(val1 ^ val2);
}
Datum
mysuint4_bitxorint8(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 ^ val2);
}

Datum
int2_bitxormysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_UINT64(val2 ^ val1);
}
Datum
int4_bitxormysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_UINT64(val2 ^ val1);
}
Datum
int8_bitxormysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_UINT64(val2 ^ val1);
}


// right move
Datum
mysuint4_rightmovmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	if(val2 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}

	PG_RETURN_UINT64(val1 >> val2);
}
Datum
mysuint4_rightmovint2(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);

	if(val2 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val1 >> val2);
}
Datum
mysuint4_rightmovint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);

	if(val2 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val1 >> val2);
}
Datum
mysuint4_rightmovint8(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);

	if(val2 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val1 >> val2);
}

Datum
int2_rightmovmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);

	if(val1 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val2 >> val1);
}
Datum
int4_rightmovmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);

	if(val1 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val2 >> val1);
}
Datum
int8_rightmovmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);

	if(val1 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val2 >> val1);
}


// left move
Datum
mysuint4_leftmovmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	if(val2 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val1 << val2);
}
Datum
mysuint4_leftmovint2(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);

	if(val2 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val1 << val2);
}
Datum
mysuint4_leftmovint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);

	if(val2 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val1 << val2);
}
Datum
mysuint4_leftmovint8(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);

	if(val2 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val1 << val2);
}

Datum
int2_leftmovmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);

	if(val1 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val2 << val1);
}
Datum
int4_leftmovmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);

	if(val1 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val2 << val1);
}
Datum
int8_leftmovmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);

	if(val1 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val2 << val1);
}

//BIT_COUNT
Datum
mysuint4_bit_count(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint32			count = 0;

	while (val1)
	{
		count += val1 & 1;
		val1 >>= 1;
	}

	PG_RETURN_UINT32(count);
}

// and &&
Datum
mysuint4_andmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_UINT32(val1 && val2);
}
Datum
mysuint4_andint2(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_UINT32(val1 && val2);
}
Datum
mysuint4_andint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);

	PG_RETURN_UINT32(val1 && val2);
}
Datum
mysuint4_andint8(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT32(val1 && val2);
}

Datum
int2_andmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_UINT32(val2 && val1);
}
Datum
int4_andmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_UINT32(val2 && val1);
}
Datum
int8_andmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_UINT32(val2 && val1);
}

// invert ! 
Datum
mysuint4_invertmysuint4(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);

	PG_RETURN_UINT32(!val1);
}

// uint4 to boolean
Datum
mysuint4_boolean(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);

	PG_RETURN_BOOL(val1 != 0);
}

//like
Datum
mysuint4_like(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint4_int8_like(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);
	if( val2 < 0)
	{
		PG_RETURN_BOOL(false);
	}
	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint4_text_like(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
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
mysuint4_notlike(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint32		val2 = PG_GETARG_UINT32(1);
	PG_RETURN_BOOL(val1 != val2);
}

Datum
mysuint4_int8_notlike(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);
	if( val2 < 0)
	{
		PG_RETURN_BOOL(true);
	}
	PG_RETURN_BOOL(val1 != val2);
}

Datum
mysuint4_text_notlike(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
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
mysuint4_larger(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_UINT32(val1 > val2 ? val1 : val2);
}

Datum
mysuint4_smaller(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	uint32		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_UINT32(val1 < val2 ? val1 : val2);
}

Datum
mysuint4_avg_accum(PG_FUNCTION_ARGS)
{
	ArrayType  *transarray;
	uint32		newval = 0;
	Int8TransTypeData *transdata = NULL;

	/*
	 * If we're invoked as an aggregate, we can cheat and modify our first
	 * parameter in-place to reduce palloc overhead. Otherwise we need to make
	 * a copy of it before scribbling on it.
	 */
	if (AggCheckCallContext(fcinfo, NULL))
		transarray = PG_GETARG_ARRAYTYPE_P(0);
	else
		transarray = PG_GETARG_ARRAYTYPE_P_COPY(0);

	if (PG_ARGISNULL(1))
	{
		PG_RETURN_ARRAYTYPE_P(transarray);
	}
	newval = PG_GETARG_UINT32(1);

	if (ARR_HASNULL(transarray) ||
		ARR_SIZE(transarray) != ARR_OVERHEAD_NONULLS(1) + sizeof(Int8TransTypeData))
		elog(ERROR, "expected 2-element int8 array");

	transdata = (Int8TransTypeData *) ARR_DATA_PTR(transarray);
	transdata->count++;
	transdata->sum += newval;

	PG_RETURN_ARRAYTYPE_P(transarray);
}

Datum
mysuint4_sum(PG_FUNCTION_ARGS)
{
	uint32		newval;

	if (PG_ARGISNULL(0))
	{
		/* No non-null input seen so far... */
		if (PG_ARGISNULL(1))
			PG_RETURN_NULL();	/* still no non-null */
		/* This is the first non-null input. */
		newval = PG_GETARG_UINT32(1);
		PG_RETURN_INT64(newval);
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
		int64		oldsum = PG_GETARG_INT64(0);

		/* Leave sum unchanged if new input is null. */
		if (PG_ARGISNULL(1))
			PG_RETURN_INT64(oldsum);

		/* OK to do the addition. */
		newval = PG_GETARG_UINT32(1);
		oldsum = oldsum + newval;

		PG_RETURN_INT64(oldsum);
	}
}
/* End - ReqID:SRS-MYSQL-NUMBERICTYPE-UINT4 */
