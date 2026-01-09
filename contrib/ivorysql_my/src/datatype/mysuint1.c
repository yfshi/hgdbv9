/*------------------------------------------------------------------
 * 
 * File: myuint1.c
 *
 * Abstract: 
 * 		Compatible with MySQL's UINT data type.
 *
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_my/src/datatype/mysuint1.c
 *
 *------------------------------------------------------------------
 */

#include "postgres.h"
#include <ctype.h>
#include <math.h>
#include <float.h>
#include "fmgr.h"
#include <limits.h>
#include <sys/time.h>
#include "access/hash.h"
#include "access/xact.h"
#include "catalog/pg_type.h"
#include "funcapi.h"
#include "libpq/pqformat.h"
#include "miscadmin.h"
#include "pgtime.h"
#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"
#include "parser/scansup.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/datetime.h"
#include "utils/formatting.h"
#include "utils/guc.h"
#include "utils/timestamp.h"
#include "utils/varlena.h"
#include "utils/numeric.h"
#include "../include/ivorysql_my.h"

/* Begin - ReqID:SRS-MYSQL-NUMBERICTYPE-uint1 */
typedef struct Int8TransTypeData
{
	int64		count;
	int64		sum;
} Int8TransTypeData;

PG_FUNCTION_INFO_V1(mysuint1_in);
PG_FUNCTION_INFO_V1(mysuint1_out);
PG_FUNCTION_INFO_V1(mysuint1_recv);
PG_FUNCTION_INFO_V1(mysuint1_send);
PG_FUNCTION_INFO_V1(mysuint1_typmodin);
PG_FUNCTION_INFO_V1(mysuint1_typmodout);
PG_FUNCTION_INFO_V1(mysuint1_scale);

PG_FUNCTION_INFO_V1(int8_uint1);
PG_FUNCTION_INFO_V1(int4_uint1);
PG_FUNCTION_INFO_V1(int2_uint1);
PG_FUNCTION_INFO_V1(boolean_mysuint1);

PG_FUNCTION_INFO_V1(mysuint1_eq);
PG_FUNCTION_INFO_V1(mysuint1_eqint2);
PG_FUNCTION_INFO_V1(mysuint1_eqint4);
PG_FUNCTION_INFO_V1(mysuint1_eqint8);
PG_FUNCTION_INFO_V1(int2_eqmysuint1);
PG_FUNCTION_INFO_V1(int4_eqmysuint1);
PG_FUNCTION_INFO_V1(int8_eqmysuint1);

PG_FUNCTION_INFO_V1(mysuint1_ne);
PG_FUNCTION_INFO_V1(mysuint1_neint2);
PG_FUNCTION_INFO_V1(mysuint1_neint4);
PG_FUNCTION_INFO_V1(mysuint1_neint8);
PG_FUNCTION_INFO_V1(int2_nemysuint1);
PG_FUNCTION_INFO_V1(int4_nemysuint1);
PG_FUNCTION_INFO_V1(int8_nemysuint1);

PG_FUNCTION_INFO_V1(mysuint1_lt);
PG_FUNCTION_INFO_V1(mysuint1_ltint2);
PG_FUNCTION_INFO_V1(mysuint1_ltint4);
PG_FUNCTION_INFO_V1(mysuint1_ltint8);
PG_FUNCTION_INFO_V1(int2_ltmysuint1);
PG_FUNCTION_INFO_V1(int4_ltmysuint1);
PG_FUNCTION_INFO_V1(int8_ltmysuint1);

PG_FUNCTION_INFO_V1(mysuint1_gt);
PG_FUNCTION_INFO_V1(mysuint1_gtint2);
PG_FUNCTION_INFO_V1(mysuint1_gtint4);
PG_FUNCTION_INFO_V1(mysuint1_gtint8);
PG_FUNCTION_INFO_V1(int2_gtmysuint1);
PG_FUNCTION_INFO_V1(int4_gtmysuint1);
PG_FUNCTION_INFO_V1(int8_gtmysuint1);

PG_FUNCTION_INFO_V1(mysuint1_le);
PG_FUNCTION_INFO_V1(mysuint1_leint2);
PG_FUNCTION_INFO_V1(mysuint1_leint4);
PG_FUNCTION_INFO_V1(mysuint1_leint8);
PG_FUNCTION_INFO_V1(int2_lemysuint1);
PG_FUNCTION_INFO_V1(int4_lemysuint1);
PG_FUNCTION_INFO_V1(int8_lemysuint1);

PG_FUNCTION_INFO_V1(mysuint1_ge);
PG_FUNCTION_INFO_V1(mysuint1_geint2);
PG_FUNCTION_INFO_V1(mysuint1_geint4);
PG_FUNCTION_INFO_V1(mysuint1_geint8);
PG_FUNCTION_INFO_V1(int2_gemysuint1);
PG_FUNCTION_INFO_V1(int4_gemysuint1);
PG_FUNCTION_INFO_V1(int8_gemysuint1);

PG_FUNCTION_INFO_V1(mysuint1_seq);
PG_FUNCTION_INFO_V1(mysuint1_seqint2);
PG_FUNCTION_INFO_V1(mysuint1_seqint4);
PG_FUNCTION_INFO_V1(mysuint1_seqint8);
PG_FUNCTION_INFO_V1(int2_seqmysuint1);
PG_FUNCTION_INFO_V1(int4_seqmysuint1);
PG_FUNCTION_INFO_V1(int8_seqmysuint1);

PG_FUNCTION_INFO_V1(mysuint1_addmysuint1);
PG_FUNCTION_INFO_V1(mysuint1_addint2);
PG_FUNCTION_INFO_V1(mysuint1_addint4);
PG_FUNCTION_INFO_V1(mysuint1_addint8);
PG_FUNCTION_INFO_V1(int2_addmysuint1);
PG_FUNCTION_INFO_V1(int4_addmysuint1);
PG_FUNCTION_INFO_V1(int8_addmysuint1);

PG_FUNCTION_INFO_V1(mysuint1_negative);

PG_FUNCTION_INFO_V1(mysuint1_submysuint1);
PG_FUNCTION_INFO_V1(mysuint1_subint2);
PG_FUNCTION_INFO_V1(mysuint1_subint4);
PG_FUNCTION_INFO_V1(mysuint1_subint8);
PG_FUNCTION_INFO_V1(int2_submysuint1);
PG_FUNCTION_INFO_V1(int4_submysuint1);
PG_FUNCTION_INFO_V1(int8_submysuint1);

PG_FUNCTION_INFO_V1(mysuint1_mulmysuint1);
PG_FUNCTION_INFO_V1(mysuint1_mulint2);
PG_FUNCTION_INFO_V1(mysuint1_mulint4);
PG_FUNCTION_INFO_V1(mysuint1_mulint8);
PG_FUNCTION_INFO_V1(int2_mulmysuint1);
PG_FUNCTION_INFO_V1(int4_mulmysuint1);
PG_FUNCTION_INFO_V1(int8_mulmysuint1);

PG_FUNCTION_INFO_V1(mysuint1_divmysuint1);
PG_FUNCTION_INFO_V1(mysuint1_divint2);
PG_FUNCTION_INFO_V1(mysuint1_divint4);
PG_FUNCTION_INFO_V1(mysuint1_divint8);
PG_FUNCTION_INFO_V1(int2_divmysuint1);
PG_FUNCTION_INFO_V1(int4_divmysuint1);
PG_FUNCTION_INFO_V1(int8_divmysuint1);

PG_FUNCTION_INFO_V1(mysuint1_exact_divmysuint1);
PG_FUNCTION_INFO_V1(mysuint1_exact_divint2);
PG_FUNCTION_INFO_V1(mysuint1_exact_divint4);
PG_FUNCTION_INFO_V1(mysuint1_exact_divint8);
PG_FUNCTION_INFO_V1(int2_exact_divmysuint1);
PG_FUNCTION_INFO_V1(int4_exact_divmysuint1);
PG_FUNCTION_INFO_V1(int8_exact_divmysuint1);

PG_FUNCTION_INFO_V1(mysuint1_modmysuint1);
PG_FUNCTION_INFO_V1(mysuint1_modint2);
PG_FUNCTION_INFO_V1(mysuint1_modint4);
PG_FUNCTION_INFO_V1(mysuint1_modint8);
PG_FUNCTION_INFO_V1(int2_modmysuint1);
PG_FUNCTION_INFO_V1(int4_modmysuint1);
PG_FUNCTION_INFO_V1(int8_modmysuint1);

PG_FUNCTION_INFO_V1(mysuint1_bitandmysuint1);
PG_FUNCTION_INFO_V1(mysuint1_bitandint2);
PG_FUNCTION_INFO_V1(mysuint1_bitandint4);
PG_FUNCTION_INFO_V1(mysuint1_bitandint8);
PG_FUNCTION_INFO_V1(int2_bitandmysuint1);
PG_FUNCTION_INFO_V1(int4_bitandmysuint1);
PG_FUNCTION_INFO_V1(int8_bitandmysuint1);

PG_FUNCTION_INFO_V1(mysuint1_bitormysuint1);
PG_FUNCTION_INFO_V1(mysuint1_bitorint2);
PG_FUNCTION_INFO_V1(mysuint1_bitorint4);
PG_FUNCTION_INFO_V1(mysuint1_bitorint8);
PG_FUNCTION_INFO_V1(int2_bitormysuint1);
PG_FUNCTION_INFO_V1(int4_bitormysuint1);
PG_FUNCTION_INFO_V1(int8_bitormysuint1);

PG_FUNCTION_INFO_V1(mysuint1_negamysuint1);

PG_FUNCTION_INFO_V1(mysuint1_bitxormysuint1);
PG_FUNCTION_INFO_V1(mysuint1_bitxorint2);
PG_FUNCTION_INFO_V1(mysuint1_bitxorint4);
PG_FUNCTION_INFO_V1(mysuint1_bitxorint8);
PG_FUNCTION_INFO_V1(int2_bitxormysuint1);
PG_FUNCTION_INFO_V1(int4_bitxormysuint1);
PG_FUNCTION_INFO_V1(int8_bitxormysuint1);

PG_FUNCTION_INFO_V1(mysuint1_rightmovmysuint1);
PG_FUNCTION_INFO_V1(mysuint1_rightmovint2);
PG_FUNCTION_INFO_V1(mysuint1_rightmovint4);
PG_FUNCTION_INFO_V1(mysuint1_rightmovint8);
PG_FUNCTION_INFO_V1(int2_rightmovmysuint1);
PG_FUNCTION_INFO_V1(int4_rightmovmysuint1);
PG_FUNCTION_INFO_V1(int8_rightmovmysuint1);

PG_FUNCTION_INFO_V1(mysuint1_leftmovmysuint1);
PG_FUNCTION_INFO_V1(mysuint1_leftmovint2);
PG_FUNCTION_INFO_V1(mysuint1_leftmovint4);
PG_FUNCTION_INFO_V1(mysuint1_leftmovint8);
PG_FUNCTION_INFO_V1(int2_leftmovmysuint1);
PG_FUNCTION_INFO_V1(int4_leftmovmysuint1);
PG_FUNCTION_INFO_V1(int8_leftmovmysuint1);

PG_FUNCTION_INFO_V1(mysuint1_bit_count);

PG_FUNCTION_INFO_V1(mysuint1_andmysuint1);
PG_FUNCTION_INFO_V1(mysuint1_andint2);
PG_FUNCTION_INFO_V1(mysuint1_andint4);
PG_FUNCTION_INFO_V1(mysuint1_andint8);
PG_FUNCTION_INFO_V1(int2_andmysuint1);
PG_FUNCTION_INFO_V1(int4_andmysuint1);
PG_FUNCTION_INFO_V1(int8_andmysuint1);

PG_FUNCTION_INFO_V1(mysuint1_invertmysuint1);

PG_FUNCTION_INFO_V1(mysuint1_boolean);

PG_FUNCTION_INFO_V1(mysuint1_like);
PG_FUNCTION_INFO_V1(mysuint1_int8_like);
PG_FUNCTION_INFO_V1(mysuint1_text_like);
PG_FUNCTION_INFO_V1(mysuint1_notlike);
PG_FUNCTION_INFO_V1(mysuint1_int8_notlike);
PG_FUNCTION_INFO_V1(mysuint1_text_notlike);

PG_FUNCTION_INFO_V1(mysuint1_larger);
PG_FUNCTION_INFO_V1(mysuint1_smaller);

PG_FUNCTION_INFO_V1(mysuint1_avg_accum);
PG_FUNCTION_INFO_V1(mysuint1_sum);

/*****************************************************************************
 *	 uint1 ADT
 *****************************************************************************/

/* 
 * mysuint1_in
 * Given uint1 text string, convert to internal uint1 format.
 */
Datum
mysuint1_in(PG_FUNCTION_ARGS)
{
	int64 	flag = 0;
	char 	*num = PG_GETARG_CSTRING(0);
	uint8 	ret;

	if (PG_NARGS() > 3)
		flag = PG_GETARG_INT64(3);

	ret = my_atoi_uint8(num, flag & TRANSFORM_FLAG_IN_INSERT);

	PG_RETURN_UINT8(ret);
}

/* 
 * mysuint1_out
 * Given internal format uint1, convert to text string.
 */
Datum
mysuint1_out(PG_FUNCTION_ARGS)
{
	uint8 arg1 = PG_GETARG_UINT32(0);
	char *str = (char *)palloc(256); /* sign, 11 digits, '\0' */

	pg_lltoa(arg1, str);

	PG_RETURN_CSTRING(str);
}

/*
 *mysuint1_recv			- converts external binary format to uint1
 */
Datum
mysuint1_recv(PG_FUNCTION_ARGS)
{
	StringInfo buf = (StringInfo)PG_GETARG_POINTER(0);

	PG_RETURN_UINT32((uint32)pq_getmsgint(buf, sizeof(uint32)));
}

/*
 *mysuint1_send			- converts uint1 to binary format
 */
Datum
mysuint1_send(PG_FUNCTION_ARGS)
{
	uint32 arg1 = PG_GETARG_UINT32(0);
	StringInfoData buf;

	pq_begintypsend(&buf);
	pq_sendint32(&buf, arg1);
	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

/* common code for mysdatetime_typmodin */
Datum
mysuint1_typmodin(PG_FUNCTION_ARGS)
{
	ArrayType  *ta = PG_GETARG_ARRAYTYPE_P(0);
	int32	   *tl = NULL;
	int			n = 0;

	tl = ArrayGetIntegerTypmods(ta, &n);

	PG_RETURN_INT32(*tl);
}


/* common code for mysdatetimetypmodout and mysdatetimetypmodout */
Datum
mysuint1_typmodout(PG_FUNCTION_ARGS)
{
	int32		typmod = PG_GETARG_INT32(0);
	char	   *res = (char *) palloc(64);

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
mysuint1_scale(PG_FUNCTION_ARGS)
{
	int64		num = PG_GETARG_INT64(0);

	PG_RETURN_UINT32(num);
}

Datum
int8_uint1(PG_FUNCTION_ARGS)
{
	int64		val1 = PG_GETARG_INT64(0);
	
	if (val1 < 0 || val1 > PG_UINT8_MAX)
	{
		int EREPORT_LEVEL = (!is_strict_mode()) ? WARNING : ERROR;
		ereport(EREPORT_LEVEL, (errmsg("integer unsigned out of range")));
		val1 = val1 < 0 ? 0 : PG_UINT8_MAX;
	}

	PG_RETURN_UINT32(val1);
}

Datum
int4_uint1(PG_FUNCTION_ARGS)
{
	int32		val1 = PG_GETARG_INT32(0);
	
	if (val1 < 0 || val1 > PG_UINT8_MAX)
	{
		int EREPORT_LEVEL = (!is_strict_mode()) ? WARNING : ERROR;
		ereport(EREPORT_LEVEL, (errmsg("integer unsigned out of range")));
		val1 = val1 < 0 ? 0 : PG_UINT8_MAX;
	}

	PG_RETURN_UINT32(val1);
}

Datum
int2_uint1(PG_FUNCTION_ARGS)
{
	int16		val1 = PG_GETARG_INT16(0);
	
	if (val1 < 0 || val1 > PG_UINT8_MAX)
	{
		int EREPORT_LEVEL = (!is_strict_mode()) ? WARNING : ERROR;
		ereport(EREPORT_LEVEL, (errmsg("integer unsigned out of range")));
		val1 = val1 < 0 ? 0 : PG_UINT8_MAX;
	}

	PG_RETURN_UINT32(val1);
}

Datum
boolean_mysuint1(PG_FUNCTION_ARGS)
{
	PG_RETURN_UINT32(PG_GETARG_BOOL(0));
}
/*****************************************************************************
 *	 Operator Function														 *
 *****************************************************************************/
Datum
mysuint1_eq(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint1_eqint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint1_eqint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);
	
	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint1_eqint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);
	
	PG_RETURN_BOOL(val1 == val2);
}

Datum
int2_eqmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_BOOL(val2 == val1);
}

Datum
int4_eqmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);
	
	PG_RETURN_BOOL(val2 == val1);
}

Datum
int8_eqmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_BOOL(val2 == val1);
}


Datum
mysuint1_ne(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
mysuint1_neint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_BOOL(val1 != val2);
}


Datum
mysuint1_neint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
mysuint1_neint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 != val2);
}


Datum
int2_nemysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_BOOL(val2 != val1);
}

Datum
int4_nemysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_BOOL(val2 != val1);
}

Datum
int8_nemysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_BOOL(val2 != val1);
}


Datum
mysuint1_lt(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
mysuint1_ltint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);
	if (val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 < val2);
}


Datum
mysuint1_ltint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);
	if (val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
mysuint1_ltint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);
	if (val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 < val2);
}


Datum
int2_ltmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);
	if (val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 < val1);
}

Datum
int4_ltmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);
	if (val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 < val1);
}

Datum
int8_ltmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);
	if (val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 < val1);
}

Datum
mysuint1_le(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
mysuint1_leint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);
	if (val2 < 0) PG_RETURN_BOOL(false);
	
	PG_RETURN_BOOL(val1 <= val2);
}

Datum
mysuint1_leint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);
	if (val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
mysuint1_leint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);
	if (val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
int2_lemysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);
	if (val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 <= val1);
}

Datum
int4_lemysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);
	if (val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 <= val1);
}

Datum
int8_lemysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);
	if (val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 <= val1);
}


Datum
mysuint1_gt(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
mysuint1_gtint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);
	if (val2 < 0) PG_RETURN_BOOL(true);
	
	PG_RETURN_BOOL(val1 > val2);
}

Datum
mysuint1_gtint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);
	if (val2 < 0) PG_RETURN_BOOL(true);
	
	PG_RETURN_BOOL(val1 > val2);
}

Datum
mysuint1_gtint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);
	if (val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
int2_gtmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);
	if (val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 > val1);
}

Datum
int4_gtmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);
	if (val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 > val1);
}

Datum
int8_gtmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);
	if (val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 > val1);
}


Datum
mysuint1_ge(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 >= val2);
}

Datum
mysuint1_geint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);
	if (val2 < 0) PG_RETURN_BOOL(true);
	
	PG_RETURN_BOOL(val1 >= val2);
}

Datum
mysuint1_geint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);
	if (val2 < 0) PG_RETURN_BOOL(true);
	
	PG_RETURN_BOOL(val1 >= val2);
}

Datum
mysuint1_geint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);
	if (val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val1 >= val2);
}


Datum
int2_gemysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);
	if (val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 >= val1);
}

Datum
int4_gemysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);
	if (val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 >= val1);
}

Datum
int8_gemysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);
	if (val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 < val1);
}

Datum
mysuint1_seq(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint1_seqint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint1_seqint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint1_seqint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
int2_seqmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_BOOL(val2 == val1);
}

Datum
int4_seqmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_BOOL(val2 == val1);
}

Datum
int8_seqmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_BOOL(val2 == val1);
}

Datum
mysuint1_addmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);
	if ((val1 + val2) < 0 || (val1 + val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT32(val1 + val2);
}

Datum
mysuint1_addint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);
	if ((val1 + val2) < 0 || (val1 + val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT32(val1 + val2);
}

Datum
mysuint1_addint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);
	if ((val1 + val2) < 0 || (val1 + val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT32(val1 + val2);
}

Datum
mysuint1_addint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);
	if ((val1 + val2) < 0 || (val1 + val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val1 + val2);
}

Datum
int2_addmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);
	if ((val1 + val2) < 0 || (val1 + val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT32(val1 + val2);
}

Datum
int4_addmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);
	if ((val1 + val2) < 0 || (val1 + val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT32(val1 + val2);
}

Datum
int8_addmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);
	if ((val1 + val2) < 0 || (val1 + val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}	
	PG_RETURN_UINT64(val1 + val2);
}

Datum
mysuint1_negative(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		tempvalue = (int32)val1;
	PG_RETURN_INT32(-tempvalue);
}

Datum
mysuint1_submysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);
	if ((val1 < val2) || (val1 - val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}

	PG_RETURN_UINT32(val1 - val2);
}

Datum
mysuint1_subint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);
	if ((val1 - val2) < 0 || (val1 - val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT32(val1 - val2);
}

Datum
mysuint1_subint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);
	if ((val1 - val2) < 0 || (val1 - val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT32(val1 - val2);
}

Datum
mysuint1_subint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);
	if ((val1 - val2) < 0 || (val1 - val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val1 - val2);
}

Datum
int2_submysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);
	if (val2 < 0 || (val2 - val1) < 0 || (val2 - val1) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT32(val2 - val1);
}

Datum
int4_submysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);
	if (val2 < 0 || (val2 - val1) < 0 || (val2 - val1) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT32(val2 - val1);
}

Datum
int8_submysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);
	if (val2 < 0 || (val2 - val1) < 0 || (val2 - val1) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val2 - val1);
}

// *
Datum
mysuint1_mulmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);
	uint32		res = val1*val2;
	if (val2 < 0 || val2 > (PG_UINT32_MAX / val1))
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(res);
}
Datum
mysuint1_mulint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);
	uint32		res = val1*val2;
	if (val2 < 0 || val2 > (PG_UINT32_MAX / val1))
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(res);
}
Datum
mysuint1_mulint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);
	uint32		res = val1*val2;
	if (val2 < 0 || val2 > (PG_UINT32_MAX / val1))
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(res);
}
Datum
mysuint1_mulint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);
	uint32		res = val1*val2;
	if (val2 < 0 || val2 > (PG_UINT32_MAX / val1))
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(res);
}

Datum
int2_mulmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);
	uint32		res = val2*val1;
	if (val2 < 0 || val2 > (PG_UINT32_MAX / val1))
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(res);
}
Datum
int4_mulmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);
	uint32		res = val2*val1;
	if (val2 < 0 || val2 > (PG_UINT32_MAX / val1))
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(res);
}
Datum
int8_mulmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
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
mysuint1_divmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);
	Numeric		res;

	if (val2 == 0)
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
mysuint1_divint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);
	Numeric		res;

	if (val2 == 0)
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
mysuint1_divint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);
	Numeric		res;

	if (val2 == 0)
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
mysuint1_divint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);
	Numeric		res;

	if (val2 == 0)
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
int2_divmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);
	Numeric		res;

	if (val1 == 0)
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
int4_divmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);
	Numeric		res;

	if (val1 == 0)
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
int8_divmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);
	Numeric		res;

	if (val1 == 0)
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
mysuint1_exact_divmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);
	if (val2 < 0 )
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	if (val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	
	PG_RETURN_UINT32(val1 / val2);
}
Datum
mysuint1_exact_divint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);
	if (val2 < 0 )
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	if (val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}	
	PG_RETURN_UINT32(val1 / val2);
}
Datum
mysuint1_exact_divint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);
	if (val2 < 0 )
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	if (val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	PG_RETURN_UINT32(val1 / val2);
}
Datum
mysuint1_exact_divint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);
	if (val2 < 0 )
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	if (val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}	
	PG_RETURN_UINT64(val1 / val2);
}

Datum
int2_exact_divmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);
	if (val2 < 0 )
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	if (val1 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}	

	PG_RETURN_UINT32(val2 / val1);
}
Datum
int4_exact_divmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);
	if (val2 < 0 )
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	if (val1 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	
	PG_RETURN_UINT32(val2 / val1);
}
Datum
int8_exact_divmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);
	if (val2 < 0 )
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	if (val1 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	
	PG_RETURN_UINT64(val2 / val1);
}


// mod
Datum
mysuint1_modmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);
	if (val2 < 0 )
	{
		PG_RETURN_UINT32(0);
	}
	if (val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	PG_RETURN_UINT32(val1 % val2);
}
Datum
mysuint1_modint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);
	if (val2 < 0 )
	{
		PG_RETURN_UINT32(0);
	}
	if (val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	PG_RETURN_UINT32(val1 % val2);
}
Datum
mysuint1_modint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);
	if (val2 < 0 )
	{
		PG_RETURN_UINT32(0);
	}
	if (val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	PG_RETURN_UINT32(val1 % val2);
}
Datum
mysuint1_modint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);
	if (val2 < 0 )
	{
		PG_RETURN_UINT64(0);
	}
	if (val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	PG_RETURN_UINT64(val1 % val2);
}

Datum
int2_modmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);
	if (val2 < 0 )
	{
		PG_RETURN_INT64(0);
	}
	if (val1 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	PG_RETURN_INT64(val2 % val1);
}
Datum
int4_modmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);
	if (val2 < 0 )
	{
		PG_RETURN_INT64(0);
	}
	if (val1 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	PG_RETURN_INT64(val2 % val1);
}
Datum
int8_modmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);
	if (val2 < 0 )
	{
		PG_RETURN_INT64(0);
	}
	if (val1 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	PG_RETURN_INT64(val2 % val1);
}

// bitand
Datum
mysuint1_bitandmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_UINT64(val1 & val2);
}
Datum
mysuint1_bitandint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);


	PG_RETURN_UINT64(val1 & val2);
}
Datum
mysuint1_bitandint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);

	PG_RETURN_UINT64(val1 & val2);
}
Datum
mysuint1_bitandint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 & val2);
}

Datum
int2_bitandmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_UINT64(val2 & val1);
}
Datum
int4_bitandmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_UINT64(val2 & val1);
}
Datum
int8_bitandmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_UINT64(val2 & val1);
}

// bitor
Datum
mysuint1_bitormysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_UINT64(val1 | val2);
}
Datum
mysuint1_bitorint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_UINT64(val1 | val2);
}
Datum
mysuint1_bitorint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);

	PG_RETURN_UINT64(val1 | val2);
}
Datum
mysuint1_bitorint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 | val2);
}

Datum
int2_bitormysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_UINT64(val2 | val1);
}
Datum
int4_bitormysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_UINT64(val2 | val1);
}
Datum
int8_bitormysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_UINT64(val2 | val1);
}


// nega
Datum
mysuint1_negamysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint64		res;

	res = ~(uint64)val1;
	PG_RETURN_UINT64(res);
}

// bitxor
Datum
mysuint1_bitxormysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_UINT64(val1 ^ val2);
}
Datum
mysuint1_bitxorint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_UINT64(val1 ^ val2);
}
Datum
mysuint1_bitxorint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);

	PG_RETURN_UINT64(val1 ^ val2);
}
Datum
mysuint1_bitxorint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 ^ val2);
}

Datum
int2_bitxormysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_UINT64(val2 ^ val1);
}
Datum
int4_bitxormysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_UINT64(val2 ^ val1);
}
Datum
int8_bitxormysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_UINT64(val2 ^ val1);
}


// right move
Datum
mysuint1_rightmovmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	if (val2 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}

	PG_RETURN_UINT64((uint64)val1 >> val2);
}
Datum
mysuint1_rightmovint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);

	if (val2 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64((uint64)val1 >> val2);
}
Datum
mysuint1_rightmovint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);

	if (val2 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64((uint64)val1 >> val2);
}
Datum
mysuint1_rightmovint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);

	if (val2 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64((uint64)val1 >> val2);
}

Datum
int2_rightmovmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);

	if (val1 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val2 >> val1);
}
Datum
int4_rightmovmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);

	if (val1 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val2 >> val1);
}
Datum
int8_rightmovmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);

	if (val1 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val2 >> val1);
}


// left move
Datum
mysuint1_leftmovmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	if (val2 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64((uint64)val1 << val2);
}
Datum
mysuint1_leftmovint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);

	if (val2 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64((uint64)val1 << val2);
}
Datum
mysuint1_leftmovint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);

	if (val2 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64((uint64)val1 << val2);
}
Datum
mysuint1_leftmovint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);

	if (val2 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64((uint64)val1 << val2);
}

Datum
int2_leftmovmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);

	if (val1 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val2 << val1);
}
Datum
int4_leftmovmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);

	if (val1 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val2 << val1);
}
Datum
int8_leftmovmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);

	if (val1 > sizeof(int64)*8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val2 << val1);
}

//BIT_COUNT
Datum
mysuint1_bit_count(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
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
mysuint1_andmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_UINT32(val1 && val2);
}
Datum
mysuint1_andint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_UINT32(val1 && val2);
}
Datum
mysuint1_andint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);

	PG_RETURN_UINT32(val1 && val2);
}
Datum
mysuint1_andint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT32(val1 && val2);
}

Datum
int2_andmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_UINT32(val2 && val1);
}
Datum
int4_andmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_UINT32(val2 && val1);
}
Datum
int8_andmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_UINT32(val2 && val1);
}

// invert ! 
Datum
mysuint1_invertmysuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);

	PG_RETURN_UINT32(!val1);
}

// uint1 to boolean
Datum
mysuint1_boolean(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);

	PG_RETURN_BOOL(val1 != 0);
}

//like
Datum
mysuint1_like(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint1_int8_like(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);
	if ( val2 < 0)
	{
		PG_RETURN_BOOL(false);
	}
	PG_RETURN_BOOL(val1 == val2);
}

Datum
mysuint1_text_like(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	Datum		val2 = PG_GETARG_DATUM(1);
	text		*val1text = NULL;
	bool		result =false;
	char		*val1str = (char *)palloc(32); /* sign, 11 digits, '\0' */

	pg_lltoa(val1, val1str);

	val1text = cstring_to_text(val1str);
	result = DatumGetBool(DirectFunctionCall2(textlike, Int64GetDatum(val1text), val2));

	PG_RETURN_BOOL(result);
}

Datum
mysuint1_notlike(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
mysuint1_int8_notlike(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);
	if ( val2 < 0)
	{
		PG_RETURN_BOOL(true);
	}
	PG_RETURN_BOOL(val1 != val2);
}

Datum
mysuint1_text_notlike(PG_FUNCTION_ARGS)
{
	uint32		val1 = PG_GETARG_UINT32(0);
	Datum		val2 = PG_GETARG_DATUM(1);
	text		*val1text = NULL;
	bool		result =false;
	char		*val1str = (char *)palloc(32); /* sign, 11 digits, '\0' */

	pg_lltoa(val1, val1str);

	val1text = cstring_to_text(val1str);
	result = DatumGetBool(DirectFunctionCall2(textnlike, Int64GetDatum(val1text), val2));

	PG_RETURN_BOOL(result);
}

/* AGGREGATE */
Datum
mysuint1_larger(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_UINT32(val1 > val2 ? val1 : val2);
}

Datum
mysuint1_smaller(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_UINT32(val1 < val2 ? val1 : val2);
}

Datum
mysuint1_avg_accum(PG_FUNCTION_ARGS)
{
	ArrayType  *transarray;
	uint8		newval = 0;
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
mysuint1_sum(PG_FUNCTION_ARGS)
{
	uint8		newval;

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
/* End - ReqID:SRS-MYSQL-NUMBERICTYPE-uint1 */
