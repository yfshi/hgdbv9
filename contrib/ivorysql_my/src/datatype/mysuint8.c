/*------------------------------------------------------------------
 * 
 * File: mysuint8.c
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
#include <float.h>
#include "common/int.h"
#include "libpq/pqformat.h"
#include "utils/builtins.h"
#include "utils/float.h"
#include "../include/ivorysql_my.h"

/* Begin - ReqID:SRS-MYSQL-NUMBERICTYPE-UINT8 */
PG_FUNCTION_INFO_V1(uint8_in);
PG_FUNCTION_INFO_V1(uint8_out);
PG_FUNCTION_INFO_V1(uint8_recv);
PG_FUNCTION_INFO_V1(uint8_send);
PG_FUNCTION_INFO_V1(uint8_typmodin);
PG_FUNCTION_INFO_V1(uint8_typmodout);
PG_FUNCTION_INFO_V1(uint8_scale);

PG_FUNCTION_INFO_V1(int8_uint8);
PG_FUNCTION_INFO_V1(int4_uint8);
PG_FUNCTION_INFO_V1(int2_uint8);
PG_FUNCTION_INFO_V1(boolean_uint8);
PG_FUNCTION_INFO_V1(uint8_int2);
PG_FUNCTION_INFO_V1(uint8_int4);
PG_FUNCTION_INFO_V1(uint8_int8);
PG_FUNCTION_INFO_V1(uint8_uint1);
PG_FUNCTION_INFO_V1(uint8_uint2);
PG_FUNCTION_INFO_V1(uint8_uint4);
PG_FUNCTION_INFO_V1(uint8_float4);
PG_FUNCTION_INFO_V1(uint8_float8);

PG_FUNCTION_INFO_V1(uint8_eq);
PG_FUNCTION_INFO_V1(uint8_eqint2);
PG_FUNCTION_INFO_V1(uint8_eqint4);
PG_FUNCTION_INFO_V1(uint8_eqint8);
PG_FUNCTION_INFO_V1(int2_equint8);
PG_FUNCTION_INFO_V1(int4_equint8);
PG_FUNCTION_INFO_V1(int8_equint8);

PG_FUNCTION_INFO_V1(uint8_ne);
PG_FUNCTION_INFO_V1(uint8_neint2);
PG_FUNCTION_INFO_V1(uint8_neint4);
PG_FUNCTION_INFO_V1(uint8_neint8);
PG_FUNCTION_INFO_V1(int2_neuint8);
PG_FUNCTION_INFO_V1(int4_neuint8);
PG_FUNCTION_INFO_V1(int8_neuint8);

PG_FUNCTION_INFO_V1(uint8_lt);
PG_FUNCTION_INFO_V1(uint8_ltint2);
PG_FUNCTION_INFO_V1(uint8_ltint4);
PG_FUNCTION_INFO_V1(uint8_ltint8);
PG_FUNCTION_INFO_V1(int2_ltuint8);
PG_FUNCTION_INFO_V1(int4_ltuint8);
PG_FUNCTION_INFO_V1(int8_ltuint8);

PG_FUNCTION_INFO_V1(uint8_gt);
PG_FUNCTION_INFO_V1(uint8_gtint2);
PG_FUNCTION_INFO_V1(uint8_gtint4);
PG_FUNCTION_INFO_V1(uint8_gtint8);
PG_FUNCTION_INFO_V1(int2_gtuint8);
PG_FUNCTION_INFO_V1(int4_gtuint8);
PG_FUNCTION_INFO_V1(int8_gtuint8);

PG_FUNCTION_INFO_V1(uint8_le);
PG_FUNCTION_INFO_V1(uint8_leint2);
PG_FUNCTION_INFO_V1(uint8_leint4);
PG_FUNCTION_INFO_V1(uint8_leint8);
PG_FUNCTION_INFO_V1(int2_leuint8);
PG_FUNCTION_INFO_V1(int4_leuint8);
PG_FUNCTION_INFO_V1(int8_leuint8);

PG_FUNCTION_INFO_V1(uint8_ge);
PG_FUNCTION_INFO_V1(uint8_geint2);
PG_FUNCTION_INFO_V1(uint8_geint4);
PG_FUNCTION_INFO_V1(uint8_geint8);
PG_FUNCTION_INFO_V1(int2_geuint8);
PG_FUNCTION_INFO_V1(int4_geuint8);
PG_FUNCTION_INFO_V1(int8_geuint8);

PG_FUNCTION_INFO_V1(uint8_seq);
PG_FUNCTION_INFO_V1(uint8_seqint2);
PG_FUNCTION_INFO_V1(uint8_seqint4);
PG_FUNCTION_INFO_V1(uint8_seqint8);
PG_FUNCTION_INFO_V1(int2_sequint8);
PG_FUNCTION_INFO_V1(int4_sequint8);
PG_FUNCTION_INFO_V1(int8_sequint8);

PG_FUNCTION_INFO_V1(uint8_adduint8);
PG_FUNCTION_INFO_V1(uint8_addint2);
PG_FUNCTION_INFO_V1(uint8_addint4);
PG_FUNCTION_INFO_V1(uint8_addint8);
PG_FUNCTION_INFO_V1(int2_adduint8);
PG_FUNCTION_INFO_V1(int4_adduint8);
PG_FUNCTION_INFO_V1(int8_adduint8);

PG_FUNCTION_INFO_V1(uint8_negative);

PG_FUNCTION_INFO_V1(uint8_subuint8);
PG_FUNCTION_INFO_V1(uint8_subint2);
PG_FUNCTION_INFO_V1(uint8_subint4);
PG_FUNCTION_INFO_V1(uint8_subint8);
PG_FUNCTION_INFO_V1(int2_subuint8);
PG_FUNCTION_INFO_V1(int4_subuint8);
PG_FUNCTION_INFO_V1(int8_subuint8);

PG_FUNCTION_INFO_V1(uint8_muluint8);
PG_FUNCTION_INFO_V1(uint8_mulint2);
PG_FUNCTION_INFO_V1(uint8_mulint4);
PG_FUNCTION_INFO_V1(uint8_mulint8);
PG_FUNCTION_INFO_V1(int2_muluint8);
PG_FUNCTION_INFO_V1(int4_muluint8);
PG_FUNCTION_INFO_V1(int8_muluint8);
/* Begin - BUG#6039 */
PG_FUNCTION_INFO_V1(double_muluint8);
PG_FUNCTION_INFO_V1(uint8_muldouble);
/* End - BUG#6039 */

PG_FUNCTION_INFO_V1(uint8_divuint8);
PG_FUNCTION_INFO_V1(uint8_divint2);
PG_FUNCTION_INFO_V1(uint8_divint4);
PG_FUNCTION_INFO_V1(uint8_divint8);
PG_FUNCTION_INFO_V1(int2_divuint8);
PG_FUNCTION_INFO_V1(int4_divuint8);
PG_FUNCTION_INFO_V1(int8_divuint8);

PG_FUNCTION_INFO_V1(uint8_exact_divuint8);
PG_FUNCTION_INFO_V1(uint8_exact_divint2);
PG_FUNCTION_INFO_V1(uint8_exact_divint4);
PG_FUNCTION_INFO_V1(uint8_exact_divint8);
PG_FUNCTION_INFO_V1(int2_exact_divuint8);
PG_FUNCTION_INFO_V1(int4_exact_divuint8);
PG_FUNCTION_INFO_V1(int8_exact_divuint8);

PG_FUNCTION_INFO_V1(uint8_moduint8);
PG_FUNCTION_INFO_V1(uint8_modint2);
PG_FUNCTION_INFO_V1(uint8_modint4);
PG_FUNCTION_INFO_V1(uint8_modint8);
PG_FUNCTION_INFO_V1(int2_moduint8);
PG_FUNCTION_INFO_V1(int4_moduint8);
PG_FUNCTION_INFO_V1(int8_moduint8);

PG_FUNCTION_INFO_V1(uint8_bitanduint8);
PG_FUNCTION_INFO_V1(uint8_bitandint2);
PG_FUNCTION_INFO_V1(uint8_bitandint4);
PG_FUNCTION_INFO_V1(uint8_bitandint8);
PG_FUNCTION_INFO_V1(int2_bitanduint8);
PG_FUNCTION_INFO_V1(int4_bitanduint8);
PG_FUNCTION_INFO_V1(int8_bitanduint8);

PG_FUNCTION_INFO_V1(uint8_bitoruint8);
PG_FUNCTION_INFO_V1(uint8_bitorint2);
PG_FUNCTION_INFO_V1(uint8_bitorint4);
PG_FUNCTION_INFO_V1(uint8_bitorint8);
PG_FUNCTION_INFO_V1(int2_bitoruint8);
PG_FUNCTION_INFO_V1(int4_bitoruint8);
PG_FUNCTION_INFO_V1(int8_bitoruint8);

PG_FUNCTION_INFO_V1(uint8_negauint8);

PG_FUNCTION_INFO_V1(uint8_bitxoruint8);
PG_FUNCTION_INFO_V1(uint8_bitxorint2);
PG_FUNCTION_INFO_V1(uint8_bitxorint4);
PG_FUNCTION_INFO_V1(uint8_bitxorint8);
PG_FUNCTION_INFO_V1(int2_bitxoruint8);
PG_FUNCTION_INFO_V1(int4_bitxoruint8);
PG_FUNCTION_INFO_V1(int8_bitxoruint8);

PG_FUNCTION_INFO_V1(uint8_rightmovuint8);
PG_FUNCTION_INFO_V1(uint8_rightmovint2);
PG_FUNCTION_INFO_V1(uint8_rightmovint4);
PG_FUNCTION_INFO_V1(uint8_rightmovint8);
PG_FUNCTION_INFO_V1(int2_rightmovuint8);
PG_FUNCTION_INFO_V1(int4_rightmovuint8);
PG_FUNCTION_INFO_V1(int8_rightmovuint8);

PG_FUNCTION_INFO_V1(uint8_leftmovuint8);
PG_FUNCTION_INFO_V1(uint8_leftmovint2);
PG_FUNCTION_INFO_V1(uint8_leftmovint4);
PG_FUNCTION_INFO_V1(uint8_leftmovint8);
PG_FUNCTION_INFO_V1(int2_leftmovuint8);
PG_FUNCTION_INFO_V1(int4_leftmovuint8);
PG_FUNCTION_INFO_V1(int8_leftmovuint8);

PG_FUNCTION_INFO_V1(uint8_bit_count);

PG_FUNCTION_INFO_V1(uint8_anduint8);
PG_FUNCTION_INFO_V1(uint8_andint2);
PG_FUNCTION_INFO_V1(uint8_andint4);
PG_FUNCTION_INFO_V1(uint8_andint8);
PG_FUNCTION_INFO_V1(int2_anduint8);
PG_FUNCTION_INFO_V1(int4_anduint8);
PG_FUNCTION_INFO_V1(int8_anduint8);

PG_FUNCTION_INFO_V1(uint8_invertuint8);

PG_FUNCTION_INFO_V1(int8_boolean);
PG_FUNCTION_INFO_V1(uint8_boolean);
PG_FUNCTION_INFO_V1(uint8_numeric);

PG_FUNCTION_INFO_V1(uint8_like);
PG_FUNCTION_INFO_V1(uint8_int8_like);
PG_FUNCTION_INFO_V1(uint8_text_like);
PG_FUNCTION_INFO_V1(uint8_notlike);
PG_FUNCTION_INFO_V1(uint8_int8_notlike);
PG_FUNCTION_INFO_V1(uint8_text_notlike);

PG_FUNCTION_INFO_V1(uint8_larger);
PG_FUNCTION_INFO_V1(uint8_smaller);

PG_FUNCTION_INFO_V1(uint8_avg_accum);
PG_FUNCTION_INFO_V1(uint8_avg);
PG_FUNCTION_INFO_V1(uint8_sum);

/*****************************************************************************
 *	 uint8 ADT
 *****************************************************************************/

/* 
 * uint8_in
 * Given uint8 text string, convert to internal uint8 format.
 */
Datum
uint8_in(PG_FUNCTION_ARGS)
{
	char 	*num = PG_GETARG_CSTRING(0);
	bool 	is_insert = CALL_IN_INSERT();
	uint64 	ret = my_atoi_uint64(num, is_insert);

	PG_RETURN_UINT64(ret);
}

/* 
 * uint8_out
 * Given internal format uint8, convert to text string.
 */
Datum
uint8_out(PG_FUNCTION_ARGS)
{
	uint64 arg1 = PG_GETARG_INT64(0);

	char *res = NULL;
	char *str = (char *)palloc(256); /* sign, 11 digits, '\0' */

	snprintf(str,256,"%llu", (long long unsigned int)arg1);

	res = str;

	PG_RETURN_CSTRING(res);
}

/*
 *uint8_recv			- converts external binary format to uint8
 */
Datum
uint8_recv(PG_FUNCTION_ARGS)
{
	StringInfo buf = (StringInfo)PG_GETARG_POINTER(0);

	PG_RETURN_UINT64((uint64)pq_getmsgint(buf, sizeof(uint64)));
}

/*
 *uint8_send			- converts uint8 to binary format
 */
Datum
uint8_send(PG_FUNCTION_ARGS)
{
	uint64 arg1 = PG_GETARG_INT64(0);
	StringInfoData buf;

	pq_begintypsend(&buf);
	pq_sendint32(&buf, arg1);
	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

/* common code for mysdatetime_typmodin */
Datum
uint8_typmodin(PG_FUNCTION_ARGS)
{
	ArrayType  *ta = PG_GETARG_ARRAYTYPE_P(0);
	int32	   *tl;
	int			n;

	tl = ArrayGetIntegerTypmods(ta, &n);

	PG_RETURN_INT32(*tl);
}


/* common code for mysdatetimetypmodout and mysdatetimetypmodout */
Datum
uint8_typmodout(PG_FUNCTION_ARGS)
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
uint8_scale(PG_FUNCTION_ARGS)
{
	Numeric		num = PG_GETARG_NUMERIC(0);

	PG_RETURN_NUMERIC(num);
}

Datum
int8_uint8(PG_FUNCTION_ARGS)
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
int4_uint8(PG_FUNCTION_ARGS)
{
	int32		val1 = PG_GETARG_INT32(0);
	uint64		val2 = val1;
	bool		is_insert = CALL_IN_INSERT();

	if (is_insert)
	{
		if (val1 < 0)
		{
			ereport(is_strict_mode() ? ERROR : WARNING, (errmsg("Out of range value for column")));
			val2 = 0;
		}
	}

	PG_RETURN_UINT64(val2);
}

Datum
int2_uint8(PG_FUNCTION_ARGS)
{
	int16		val1 = PG_GETARG_INT16(0);
	uint64		val2 = val1;
	bool		is_insert = CALL_IN_INSERT();

	if (is_insert)
	{
		if (val1 < 0)
		{
			ereport(is_strict_mode() ? ERROR : WARNING, (errmsg("Out of range value for column")));
			val2 = 0;
		}
	}

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
uint8_eq(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint8_eqint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint8_eqint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);
	
	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint8_eqint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);
	
	PG_RETURN_BOOL(val1 == val2);
}

Datum
int2_equint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_BOOL(val2 == val1);
}

Datum
int4_equint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);
	
	PG_RETURN_BOOL(val2 == val1);
}

Datum
int8_equint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_BOOL(val2 == val1);
}


Datum
uint8_ne(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
uint8_neint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_BOOL(val1 != val2);
}


Datum
uint8_neint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
uint8_neint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 != val2);
}


Datum
int2_neuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_BOOL(val2 != val1);
}

Datum
int4_neuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_BOOL(val2 != val1);
}

Datum
int8_neuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_BOOL(val2 != val1);
}


Datum
uint8_lt(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
uint8_ltint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 < val2);
}


Datum
uint8_ltint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
uint8_ltint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 < val2);
}


Datum
int2_ltuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);
	if(val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 < val1);
}

Datum
int4_ltuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);
	if(val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 < val1);
}

Datum
int8_ltuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);
	if(val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 < val1);
}

Datum
uint8_le(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
uint8_leint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);
	if(val2 < 0) PG_RETURN_BOOL(false);
	
	PG_RETURN_BOOL(val1 <= val2);
}

Datum
uint8_leint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
uint8_leint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
int2_leuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);
	if(val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 <= val1);
}

Datum
int4_leuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);
	if(val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 <= val1);
}

Datum
int8_leuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);
	if(val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 <= val1);
}


Datum
uint8_gt(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
uint8_gtint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);
	if(val2 < 0) PG_RETURN_BOOL(true);
	
	PG_RETURN_BOOL(val1 > val2);
}

Datum
uint8_gtint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);
	if(val2 < 0) PG_RETURN_BOOL(true);
	
	PG_RETURN_BOOL(val1 > val2);
}

Datum
uint8_gtint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);
	if(val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
int2_gtuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 > val1);
}

Datum
int4_gtuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 > val1);
}

Datum
int8_gtuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 > val1);
}


Datum
uint8_ge(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 >= val2);
}

Datum
uint8_geint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);
	if(val2 < 0) PG_RETURN_BOOL(true);
	
	PG_RETURN_BOOL(val1 >= val2);
}

Datum
uint8_geint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);
	if(val2 < 0) PG_RETURN_BOOL(true);
	
	PG_RETURN_BOOL(val1 >= val2);
}

Datum
uint8_geint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);
	if(val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val1 >= val2);
}


Datum
int2_geuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 >= val1);
}

Datum
int4_geuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 >= val1);
}

Datum
int8_geuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);
	if(val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 < val1);
}

Datum
uint8_seq(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint8_seqint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint8_seqint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint8_seqint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
int2_sequint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_BOOL(val2 == val1);
}

Datum
int4_sequint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_BOOL(val2 == val1);
}

Datum
int8_sequint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_BOOL(val2 == val1);
}

Datum
uint8_adduint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint64		val2 = PG_GETARG_UINT64(1);
	uint64 		ret;

	if (unlikely(pg_add_u64_overflow(val1, val2, &ret))) /* bug#z6037 */
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val1 + val2);
}

Datum
uint8_addint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	int16		val2 = PG_GETARG_INT16(1);
	uint64 		ret;

	if ((val2 > 0 && unlikely(pg_add_u64_overflow(val1, val2, &ret))) ||
		(val2 < 0 && unlikely(pg_sub_u64_overflow(val1, -1 * val2, &ret)))) /* bug#z6037 */
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val1 + val2);
}

Datum
uint8_addint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	int32		val2 = PG_GETARG_INT32(1);
	uint64 		ret;

	if ((val2 > 0 && unlikely(pg_add_u64_overflow(val1, val2, &ret))) ||
		(val2 < 0 && unlikely(pg_sub_u64_overflow(val1, -1 * val2, &ret)))) /* bug#z6037 */
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val1 + val2);
}

Datum
uint8_addint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	int64		val2 = PG_GETARG_INT64(1);
	uint64 		ret;

	if ((val2 > 0 && unlikely(pg_add_u64_overflow(val1, val2, &ret))) ||
		(val2 < 0 && unlikely(pg_sub_u64_overflow(val1, -1 * val2, &ret)))) /* bug#z6037 */
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val1 + val2);
}

Datum
int2_adduint8(PG_FUNCTION_ARGS)
{
	int16		val1 = PG_GETARG_INT16(0);
	uint64		val2 = PG_GETARG_UINT64(1);
	uint64 		ret;

	if ((val1 > 0 && unlikely(pg_add_u64_overflow(val1, val2, &ret))) ||
		(val1 < 0 && unlikely(pg_sub_u64_overflow(val2, -1 * val1, &ret)))) /* bug#z6037 */
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val1 + val2);
}

Datum
int4_adduint8(PG_FUNCTION_ARGS)
{
	int32		val1 = PG_GETARG_INT32(0);
	uint64		val2 = PG_GETARG_UINT64(1);
	uint64 		ret;

	if ((val1 > 0 && unlikely(pg_add_u64_overflow(val1, val2, &ret))) ||
		(val1 < 0 && unlikely(pg_sub_u64_overflow(val2, -1 * val1, &ret)))) /* bug#z6037 */
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val1 + val2);
}

Datum
int8_adduint8(PG_FUNCTION_ARGS)
{
	int64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_UINT64(1);
	uint64 		ret;

	if ((val1 > 0 && unlikely(pg_add_u64_overflow(val1, val2, &ret))) ||
		(val1 < 0 && unlikely(pg_sub_u64_overflow(val2, -1 * val1, &ret)))) /* bug#z6037 */
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}	
	PG_RETURN_UINT64(val1 + val2);
}

Datum
uint8_negative(PG_FUNCTION_ARGS)
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
uint8_subuint8(PG_FUNCTION_ARGS)
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
uint8_subint2(PG_FUNCTION_ARGS)
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
uint8_subint4(PG_FUNCTION_ARGS)
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
uint8_subint8(PG_FUNCTION_ARGS)
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
int2_subuint8(PG_FUNCTION_ARGS)
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
int4_subuint8(PG_FUNCTION_ARGS)
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
int8_subuint8(PG_FUNCTION_ARGS)
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
uint8_muluint8(PG_FUNCTION_ARGS)
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
uint8_mulint2(PG_FUNCTION_ARGS)
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
uint8_mulint4(PG_FUNCTION_ARGS)
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
uint8_mulint8(PG_FUNCTION_ARGS)
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
int2_muluint8(PG_FUNCTION_ARGS)
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
int4_muluint8(PG_FUNCTION_ARGS)
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
int8_muluint8(PG_FUNCTION_ARGS)
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
uint8_divuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);
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
uint8_divint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
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
uint8_divint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
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
uint8_divint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
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
int2_divuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
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
int4_divuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
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
int8_divuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
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
uint8_exact_divuint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	uint64		val2 = PG_GETARG_UINT64(1);
	
	if (val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	
	PG_RETURN_UINT64(val1 / val2);
}

Datum
uint8_exact_divint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	int16		val2 = PG_GETARG_INT16(1);
	
	if (val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	if (val2 < 0)
	{
		if (val1 >= abs(val2))
			ereport(ERROR, (errmsg("integer unsigned out of range")));
		
		PG_RETURN_UINT64(0);
	}

	PG_RETURN_UINT64(val1 / val2);
}

Datum
uint8_exact_divint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	int32		val2 = PG_GETARG_INT32(1);

	if (val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	if (val2 < 0)
	{
		if (val1 >= abs(val2))
			ereport(ERROR, (errmsg("integer unsigned out of range")));
		
		PG_RETURN_UINT64(0);
	}

	PG_RETURN_UINT64(val1 / val2);
}

Datum
uint8_exact_divint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_UINT64(0);
	int64		val2 = PG_GETARG_INT64(1);

	if (val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	if (val2 < 0)
	{
		if (val1 >= abs(val2))
			ereport(ERROR, (errmsg("integer unsigned out of range")));
		
		PG_RETURN_UINT64(0);
	}

	PG_RETURN_UINT64(val1 / val2);
}

Datum
int2_exact_divuint8(PG_FUNCTION_ARGS)
{
	int16		val1 = PG_GETARG_INT16(0);
	uint64		val2 = PG_GETARG_INT64(1);

	if (val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	if (val1 < 0)
	{
		if (abs(val1) >= val2)
			ereport(ERROR, (errmsg("integer unsigned out of range")));

		PG_RETURN_UINT32(0);
	}

	PG_RETURN_UINT32(val1 / val2);
}

Datum
int4_exact_divuint8(PG_FUNCTION_ARGS)
{
	int32		val1 = PG_GETARG_INT32(0);
	uint64		val2 = PG_GETARG_UINT64(1);
	
	if (val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	if (val1 < 0)
	{
		if (abs(val1) >= val2)
			ereport(ERROR, (errmsg("integer unsigned out of range")));

		PG_RETURN_UINT32(0);
	}

	PG_RETURN_UINT32(val1 / val2);
}

Datum
int8_exact_divuint8(PG_FUNCTION_ARGS)
{
	int64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_UINT64(1);
	
	if (val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	if (val1 < 0)
	{
		if (abs(val1) >= val2)
			ereport(ERROR, (errmsg("integer unsigned out of range")));

		PG_RETURN_UINT64(0);
	}
	
	PG_RETURN_UINT64(val1 / val2);
}

Datum
uint8_moduint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);
	uint64 		ret = 0;
	
	if (val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}
	
	ret = val1 % val2;
	
	PG_RETURN_UINT64(ret);
}

Datum
uint8_modint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);
	uint64 		ret = 0;

	if (val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	if (val2 < 0)
		val2 *= -1;

	ret = val1 % val2;

	PG_RETURN_UINT64(ret);
}

Datum
uint8_modint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);
	uint64 		ret = 0;

	if (val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	if (val2 < 0)
		val2 *= -1;

	ret = val1 % val2;

	PG_RETURN_UINT64(ret);
}

Datum
uint8_modint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);
	uint64 		ret = 0;

	if (val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	if (val2 < 0)
		val2 *= -1;

	ret = val1 % val2;

	PG_RETURN_UINT64(ret);
}

Datum
int2_moduint8(PG_FUNCTION_ARGS)
{
	int16		val1 = PG_GETARG_INT16(0);
	uint64		val2 = PG_GETARG_INT64(1);
	int64 		ret = 1;

	if (val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	if (val1 < 0)
	{
		ret = -1;
		val1 *= -1;
	}

	ret *= (val1 % val2);

	PG_RETURN_INT64(ret);
}

Datum
int4_moduint8(PG_FUNCTION_ARGS)
{
	int32		val1 = PG_GETARG_INT32(0);
	uint64		val2 = PG_GETARG_INT64(1);
	int64 		ret = 1;

	if (val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	if (val1 < 0)
	{
		ret = -1;
		val1 *= -1;
	}

	ret *= (val1 % val2);

	PG_RETURN_INT64(ret);
}

Datum
int8_moduint8(PG_FUNCTION_ARGS)
{
	int64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);
	int64 		ret = 1;

	if (val2 == 0)
	{
		if (has_sql_mode(MODE_ERROR_FOR_DIVISION_BY_ZERO))
			ereport(WARNING, (errcode(ERRCODE_DIVISION_BY_ZERO), errmsg("Division by 0")));
		PG_RETURN_NULL();
	}

	if (val1 < 0)
	{
		ret = -1;
		val1 *= -1;
	}

	ret *= (val1 % val2);

	PG_RETURN_INT64(ret);
}

// bitand
Datum
uint8_bitanduint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 & val2);
}
Datum
uint8_bitandint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);


	PG_RETURN_UINT64(val1 & val2);
}
Datum
uint8_bitandint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);

	PG_RETURN_UINT64(val1 & val2);
}
Datum
uint8_bitandint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 & val2);
}

Datum
int2_bitanduint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_UINT64(val2 & val1);
}
Datum
int4_bitanduint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_UINT64(val2 & val1);
}
Datum
int8_bitanduint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_UINT64(val2 & val1);
}

// bitor
Datum
uint8_bitoruint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 | val2);
}
Datum
uint8_bitorint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_UINT64(val1 | val2);
}
Datum
uint8_bitorint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);
	uint64		temp = (uint64)val2;
	PG_RETURN_UINT64(val1 | temp);
}
Datum
uint8_bitorint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 | val2);
}

Datum
int2_bitoruint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_UINT64(val2 | val1);
}
Datum
int4_bitoruint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_UINT64(val2 | val1);
}
Datum
int8_bitoruint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_UINT64(val2 | val1);
}

// nega
Datum
uint8_negauint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		res;

	res = ~(uint64)val1;
	PG_RETURN_UINT64(res);
}

// bitxor
Datum
uint8_bitxoruint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 ^ val2);
}
Datum
uint8_bitxorint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_UINT64(val1 ^ val2);
}
Datum
uint8_bitxorint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);

	PG_RETURN_UINT64(val1 ^ val2);
}
Datum
uint8_bitxorint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 ^ val2);
}

Datum
int2_bitxoruint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_UINT64(val2 ^ val1);
}
Datum
int4_bitxoruint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_UINT64(val2 ^ val1);
}
Datum
int8_bitxoruint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_UINT64(val2 ^ val1);
}


// right move
Datum
uint8_rightmovuint8(PG_FUNCTION_ARGS)
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
uint8_rightmovint2(PG_FUNCTION_ARGS)
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
uint8_rightmovint4(PG_FUNCTION_ARGS)
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
uint8_rightmovint8(PG_FUNCTION_ARGS)
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
int2_rightmovuint8(PG_FUNCTION_ARGS)
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
int4_rightmovuint8(PG_FUNCTION_ARGS)
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
int8_rightmovuint8(PG_FUNCTION_ARGS)
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
uint8_leftmovuint8(PG_FUNCTION_ARGS)
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
uint8_leftmovint2(PG_FUNCTION_ARGS)
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
uint8_leftmovint4(PG_FUNCTION_ARGS)
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
uint8_leftmovint8(PG_FUNCTION_ARGS)
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
int2_leftmovuint8(PG_FUNCTION_ARGS)
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
int4_leftmovuint8(PG_FUNCTION_ARGS)
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
int8_leftmovuint8(PG_FUNCTION_ARGS)
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
uint8_bit_count(PG_FUNCTION_ARGS)
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
uint8_anduint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_INT32(val1 && val2);
}
Datum
uint8_andint2(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_UINT64(val1 && val2);
}
Datum
uint8_andint4(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int32		val2 = PG_GETARG_INT32(1);

	PG_RETURN_UINT64(val1 && val2);
}
Datum
uint8_andint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 && val2);
}

Datum
int2_anduint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_UINT64(val2 && val1);
}
Datum
int4_anduint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_UINT64(val2 && val1);
}
Datum
int8_anduint8(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_UINT64(val2 && val1);
}

// invert ! 
Datum
uint8_invertuint8(PG_FUNCTION_ARGS)
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

// uint8 to boolean
Datum
uint8_boolean(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);

	PG_RETURN_BOOL(val1 != 0);
}

// uint8 to numeric
Datum
uint8_numeric(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);

	PG_RETURN_NUMERIC(uint64_to_numeric(val1));
}

//like
Datum
uint8_like(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint8_int8_like(PG_FUNCTION_ARGS)
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
uint8_text_like(PG_FUNCTION_ARGS)
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
uint8_notlike(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
uint8_int8_notlike(PG_FUNCTION_ARGS)
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
uint8_text_notlike(PG_FUNCTION_ARGS)
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
uint8_larger(PG_FUNCTION_ARGS)
{
	uint64		val1 = PG_GETARG_INT64(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 > val2 ? val1 : val2);
}

Datum
uint8_smaller(PG_FUNCTION_ARGS)
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
uint8_avg_accum(PG_FUNCTION_ARGS)
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
uint8_avg(PG_FUNCTION_ARGS)
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
uint8_sum(PG_FUNCTION_ARGS)
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

/* Begin - BUG#6039 */
Datum
double_muluint8(PG_FUNCTION_ARGS)
{
	double arg1 = PG_GETARG_FLOAT8(0);
	uint64 arg2 = PG_GETARG_UINT64(1);

	PG_RETURN_FLOAT8(float8_mul(arg1, (double) arg2));
}

Datum
uint8_muldouble(PG_FUNCTION_ARGS)
{
	uint64 arg1 = PG_GETARG_UINT64(0);
	double arg2 = PG_GETARG_FLOAT8(1);

	PG_RETURN_FLOAT8(float8_mul((double) arg1, arg2));
}
/* End - BUG#6039 */