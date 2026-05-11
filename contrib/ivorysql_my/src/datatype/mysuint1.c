/*------------------------------------------------------------------
 * 
 * File: mysuint1.c
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
#include "libpq/pqformat.h"
#include "utils/builtins.h"
#include "../include/ivorysql_my.h"

/* Begin - ReqID:SRS-MYSQL-NUMBERICTYPE-uint1 */
typedef struct Int8TransTypeData
{
	int64		count;
	int64		sum;
} Int8TransTypeData;

PG_FUNCTION_INFO_V1(uint1_in);
PG_FUNCTION_INFO_V1(uint1_out);
PG_FUNCTION_INFO_V1(uint1_recv);
PG_FUNCTION_INFO_V1(uint1_send);
PG_FUNCTION_INFO_V1(uint1_typmodin);
PG_FUNCTION_INFO_V1(uint1_typmodout);
PG_FUNCTION_INFO_V1(uint1_scale);

PG_FUNCTION_INFO_V1(int8_uint1);
PG_FUNCTION_INFO_V1(int4_uint1);
PG_FUNCTION_INFO_V1(int2_uint1);
PG_FUNCTION_INFO_V1(boolean_uint1);
PG_FUNCTION_INFO_V1(uint1_int8);
PG_FUNCTION_INFO_V1(uint1_numeric);

PG_FUNCTION_INFO_V1(uint1_eq);
PG_FUNCTION_INFO_V1(uint1_eqint2);
PG_FUNCTION_INFO_V1(uint1_eqint4);
PG_FUNCTION_INFO_V1(uint1_eqint8);
PG_FUNCTION_INFO_V1(int2_equint1);
PG_FUNCTION_INFO_V1(int4_equint1);
PG_FUNCTION_INFO_V1(int8_equint1);

PG_FUNCTION_INFO_V1(uint1_ne);
PG_FUNCTION_INFO_V1(uint1_neint2);
PG_FUNCTION_INFO_V1(uint1_neint4);
PG_FUNCTION_INFO_V1(uint1_neint8);
PG_FUNCTION_INFO_V1(int2_neuint1);
PG_FUNCTION_INFO_V1(int4_neuint1);
PG_FUNCTION_INFO_V1(int8_neuint1);

PG_FUNCTION_INFO_V1(uint1_lt);
PG_FUNCTION_INFO_V1(uint1_ltint2);
PG_FUNCTION_INFO_V1(uint1_ltint4);
PG_FUNCTION_INFO_V1(uint1_ltint8);
PG_FUNCTION_INFO_V1(int2_ltuint1);
PG_FUNCTION_INFO_V1(int4_ltuint1);
PG_FUNCTION_INFO_V1(int8_ltuint1);

PG_FUNCTION_INFO_V1(uint1_gt);
PG_FUNCTION_INFO_V1(uint1_gtint2);
PG_FUNCTION_INFO_V1(uint1_gtint4);
PG_FUNCTION_INFO_V1(uint1_gtint8);
PG_FUNCTION_INFO_V1(int2_gtuint1);
PG_FUNCTION_INFO_V1(int4_gtuint1);
PG_FUNCTION_INFO_V1(int8_gtuint1);

PG_FUNCTION_INFO_V1(uint1_le);
PG_FUNCTION_INFO_V1(uint1_leint2);
PG_FUNCTION_INFO_V1(uint1_leint4);
PG_FUNCTION_INFO_V1(uint1_leint8);
PG_FUNCTION_INFO_V1(int2_leuint1);
PG_FUNCTION_INFO_V1(int4_leuint1);
PG_FUNCTION_INFO_V1(int8_leuint1);

PG_FUNCTION_INFO_V1(uint1_ge);
PG_FUNCTION_INFO_V1(uint1_geint2);
PG_FUNCTION_INFO_V1(uint1_geint4);
PG_FUNCTION_INFO_V1(uint1_geint8);
PG_FUNCTION_INFO_V1(int2_geuint1);
PG_FUNCTION_INFO_V1(int4_geuint1);
PG_FUNCTION_INFO_V1(int8_geuint1);

PG_FUNCTION_INFO_V1(uint1_seq);
PG_FUNCTION_INFO_V1(uint1_seqint2);
PG_FUNCTION_INFO_V1(uint1_seqint4);
PG_FUNCTION_INFO_V1(uint1_seqint8);
PG_FUNCTION_INFO_V1(int2_sequint1);
PG_FUNCTION_INFO_V1(int4_sequint1);
PG_FUNCTION_INFO_V1(int8_sequint1);

PG_FUNCTION_INFO_V1(uint1_adduint1);
PG_FUNCTION_INFO_V1(uint1_addint2);
PG_FUNCTION_INFO_V1(uint1_addint4);
PG_FUNCTION_INFO_V1(uint1_addint8);
PG_FUNCTION_INFO_V1(int2_adduint1);
PG_FUNCTION_INFO_V1(int4_adduint1);
PG_FUNCTION_INFO_V1(int8_adduint1);

PG_FUNCTION_INFO_V1(uint1_negative);

PG_FUNCTION_INFO_V1(uint1_subuint1);
PG_FUNCTION_INFO_V1(uint1_subint2);
PG_FUNCTION_INFO_V1(uint1_subint4);
PG_FUNCTION_INFO_V1(uint1_subint8);
PG_FUNCTION_INFO_V1(int2_subuint1);
PG_FUNCTION_INFO_V1(int4_subuint1);
PG_FUNCTION_INFO_V1(int8_subuint1);

PG_FUNCTION_INFO_V1(uint1_muluint1);
PG_FUNCTION_INFO_V1(uint1_mulint2);
PG_FUNCTION_INFO_V1(uint1_mulint4);
PG_FUNCTION_INFO_V1(uint1_mulint8);
PG_FUNCTION_INFO_V1(int2_muluint1);
PG_FUNCTION_INFO_V1(int4_muluint1);
PG_FUNCTION_INFO_V1(int8_muluint1);

PG_FUNCTION_INFO_V1(uint1_divuint1);
PG_FUNCTION_INFO_V1(uint1_divint2);
PG_FUNCTION_INFO_V1(uint1_divint4);
PG_FUNCTION_INFO_V1(uint1_divint8);
PG_FUNCTION_INFO_V1(int2_divuint1);
PG_FUNCTION_INFO_V1(int4_divuint1);
PG_FUNCTION_INFO_V1(int8_divuint1);

PG_FUNCTION_INFO_V1(uint1_exact_divuint1);
PG_FUNCTION_INFO_V1(uint1_exact_divint2);
PG_FUNCTION_INFO_V1(uint1_exact_divint4);
PG_FUNCTION_INFO_V1(uint1_exact_divint8);
PG_FUNCTION_INFO_V1(int2_exact_divuint1);
PG_FUNCTION_INFO_V1(int4_exact_divuint1);
PG_FUNCTION_INFO_V1(int8_exact_divuint1);

PG_FUNCTION_INFO_V1(uint1_moduint1);
PG_FUNCTION_INFO_V1(uint1_modint2);
PG_FUNCTION_INFO_V1(uint1_modint4);
PG_FUNCTION_INFO_V1(uint1_modint8);
PG_FUNCTION_INFO_V1(int2_moduint1);
PG_FUNCTION_INFO_V1(int4_moduint1);
PG_FUNCTION_INFO_V1(int8_moduint1);

PG_FUNCTION_INFO_V1(uint1_bitanduint1);
PG_FUNCTION_INFO_V1(uint1_bitandint2);
PG_FUNCTION_INFO_V1(uint1_bitandint4);
PG_FUNCTION_INFO_V1(uint1_bitandint8);
PG_FUNCTION_INFO_V1(int2_bitanduint1);
PG_FUNCTION_INFO_V1(int4_bitanduint1);
PG_FUNCTION_INFO_V1(int8_bitanduint1);

PG_FUNCTION_INFO_V1(uint1_bitoruint1);
PG_FUNCTION_INFO_V1(uint1_bitorint2);
PG_FUNCTION_INFO_V1(uint1_bitorint4);
PG_FUNCTION_INFO_V1(uint1_bitorint8);
PG_FUNCTION_INFO_V1(int2_bitoruint1);
PG_FUNCTION_INFO_V1(int4_bitoruint1);
PG_FUNCTION_INFO_V1(int8_bitoruint1);

PG_FUNCTION_INFO_V1(uint1_negauint1);

PG_FUNCTION_INFO_V1(uint1_bitxoruint1);
PG_FUNCTION_INFO_V1(uint1_bitxorint2);
PG_FUNCTION_INFO_V1(uint1_bitxorint4);
PG_FUNCTION_INFO_V1(uint1_bitxorint8);
PG_FUNCTION_INFO_V1(int2_bitxoruint1);
PG_FUNCTION_INFO_V1(int4_bitxoruint1);
PG_FUNCTION_INFO_V1(int8_bitxoruint1);

PG_FUNCTION_INFO_V1(uint1_rightmovuint1);
PG_FUNCTION_INFO_V1(uint1_rightmovint2);
PG_FUNCTION_INFO_V1(uint1_rightmovint4);
PG_FUNCTION_INFO_V1(uint1_rightmovint8);
PG_FUNCTION_INFO_V1(int2_rightmovuint1);
PG_FUNCTION_INFO_V1(int4_rightmovuint1);
PG_FUNCTION_INFO_V1(int8_rightmovuint1);

PG_FUNCTION_INFO_V1(uint1_leftmovuint1);
PG_FUNCTION_INFO_V1(uint1_leftmovint2);
PG_FUNCTION_INFO_V1(uint1_leftmovint4);
PG_FUNCTION_INFO_V1(uint1_leftmovint8);
PG_FUNCTION_INFO_V1(int2_leftmovuint1);
PG_FUNCTION_INFO_V1(int4_leftmovuint1);
PG_FUNCTION_INFO_V1(int8_leftmovuint1);

PG_FUNCTION_INFO_V1(uint1_bit_count);

PG_FUNCTION_INFO_V1(uint1_anduint1);
PG_FUNCTION_INFO_V1(uint1_andint2);
PG_FUNCTION_INFO_V1(uint1_andint4);
PG_FUNCTION_INFO_V1(uint1_andint8);
PG_FUNCTION_INFO_V1(int2_anduint1);
PG_FUNCTION_INFO_V1(int4_anduint1);
PG_FUNCTION_INFO_V1(int8_anduint1);

PG_FUNCTION_INFO_V1(uint1_invertuint1);

PG_FUNCTION_INFO_V1(uint1_boolean);

PG_FUNCTION_INFO_V1(uint1_like);
PG_FUNCTION_INFO_V1(uint1_int8_like);
PG_FUNCTION_INFO_V1(uint1_text_like);
PG_FUNCTION_INFO_V1(uint1_notlike);
PG_FUNCTION_INFO_V1(uint1_int8_notlike);
PG_FUNCTION_INFO_V1(uint1_text_notlike);

PG_FUNCTION_INFO_V1(uint1_larger);
PG_FUNCTION_INFO_V1(uint1_smaller);

PG_FUNCTION_INFO_V1(uint1_avg_accum);
PG_FUNCTION_INFO_V1(uint1_sum);

/*****************************************************************************
 *	 uint1 ADT
 *****************************************************************************/

/* 
 * uint1_in
 * Given uint1 text string, convert to internal uint1 format.
 */
Datum
uint1_in(PG_FUNCTION_ARGS)
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
 * uint1_out
 * Given internal format uint1, convert to text string.
 */
Datum
uint1_out(PG_FUNCTION_ARGS)
{
	uint8 arg1 = PG_GETARG_UINT32(0);
	char *str = (char *)palloc(256); /* sign, 11 digits, '\0' */

	pg_lltoa(arg1, str);

	PG_RETURN_CSTRING(str);
}

/*
 *uint1_recv			- converts external binary format to uint1
 */
Datum
uint1_recv(PG_FUNCTION_ARGS)
{
	StringInfo buf = (StringInfo)PG_GETARG_POINTER(0);

	PG_RETURN_UINT32((uint32)pq_getmsgint(buf, sizeof(uint32)));
}

/*
 *uint1_send			- converts uint1 to binary format
 */
Datum
uint1_send(PG_FUNCTION_ARGS)
{
	uint32 arg1 = PG_GETARG_UINT32(0);
	StringInfoData buf;

	pq_begintypsend(&buf);
	pq_sendint32(&buf, arg1);
	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

/* common code for mysdatetime_typmodin */
Datum
uint1_typmodin(PG_FUNCTION_ARGS)
{
	ArrayType  *ta = PG_GETARG_ARRAYTYPE_P(0);
	int32	   *tl = NULL;
	int			n = 0;

	tl = ArrayGetIntegerTypmods(ta, &n);

	PG_RETURN_INT32(*tl);
}


/* common code for mysdatetimetypmodout and mysdatetimetypmodout */
Datum
uint1_typmodout(PG_FUNCTION_ARGS)
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
uint1_scale(PG_FUNCTION_ARGS)
{
	int64		num = PG_GETARG_INT64(0);

	PG_RETURN_UINT32(num);
}

/* Begin - BUG#Z7196 */
Datum
int8_uint1(PG_FUNCTION_ARGS)
{
	int64	val1 = PG_GETARG_INT64(0);
	uint8	val2 = val1;
	bool	is_insert = CALL_IN_INSERT();

	if (is_insert)
	{
		if (val1 < 0 || val1 > PG_UINT8_MAX)
		{
			int EREPORT_LEVEL = (!is_strict_mode()) ? WARNING : ERROR;
			ereport(EREPORT_LEVEL, (errmsg("Out of range value for column")));
			val2 = val1 < 0 ? 0 : PG_UINT8_MAX;
		}
	}
	else
	{
		if (val1 < 0 || val1 > PG_UINT8_MAX)
		{
			ereport(WARNING, (errmsg("integer unsigned out of range")));
			val2 = val1 < 0 ? 0 : PG_UINT8_MAX;
		}
	}

	PG_RETURN_UINT8(val2);
}

Datum
int4_uint1(PG_FUNCTION_ARGS)
{
	int32	val1 = PG_GETARG_INT32(0);
	uint8	val2 = val1;
	bool	is_insert = CALL_IN_INSERT();

	if (is_insert)
	{
		if (val1 < 0 || val1 > PG_UINT8_MAX)
		{
			int EREPORT_LEVEL = (!is_strict_mode()) ? WARNING : ERROR;
			ereport(EREPORT_LEVEL, (errmsg("Out of range value for column")));
			val2 = val1 < 0 ? 0 : PG_UINT8_MAX;
		}
	}
	else
	{
		if (val1 < 0 || val1 > PG_UINT8_MAX)
		{
			ereport(WARNING, (errmsg("integer unsigned out of range")));
			val2 = val1 < 0 ? 0 : PG_UINT8_MAX;
		}
	}

	PG_RETURN_UINT8(val2);
}

Datum
int2_uint1(PG_FUNCTION_ARGS)
{
	int16	val1 = PG_GETARG_INT16(0);
	uint8	val2 = val1;
	bool	is_insert = CALL_IN_INSERT();

	if (is_insert)
	{
		if (val1 < 0 || val1 > PG_UINT8_MAX)
		{
			int EREPORT_LEVEL = (!is_strict_mode()) ? WARNING : ERROR;
			ereport(EREPORT_LEVEL, (errmsg("Out of range value for column")));
			val2 = val1 < 0 ? 0 : PG_UINT8_MAX;
		}
	}
	else
	{
		if (val1 < 0 || val1 > PG_UINT8_MAX)
		{
			ereport(WARNING, (errmsg("integer unsigned out of range")));
			val2 = val1 < 0 ? 0 : PG_UINT8_MAX;
		}
	}

	PG_RETURN_UINT8(val2);
}
/* End - BUG#Z7196 */

Datum
boolean_uint1(PG_FUNCTION_ARGS)
{
	PG_RETURN_UINT32(PG_GETARG_BOOL(0));
}

/* Begin - BUG#Z7200 */
Datum
uint1_int8(PG_FUNCTION_ARGS)
{
	uint8	val1 = PG_GETARG_UINT8(0);

	PG_RETURN_INT64(val1);
}
/* End - BUG#Z7200 */

/* Begin - BUG#Z7615 */
Datum
uint1_numeric(PG_FUNCTION_ARGS)
{
	uint8	val1 = PG_GETARG_UINT8(0);

	PG_RETURN_NUMERIC(uint32_to_numeric((uint32) val1));
}
/* End - BUG#Z7615 */

/*****************************************************************************
 *	 Operator Function														 *
 *****************************************************************************/
Datum
uint1_eq(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint1_eqint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint1_eqint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);
	
	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint1_eqint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);
	
	PG_RETURN_BOOL(val1 == val2);
}

Datum
int2_equint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_BOOL(val2 == val1);
}

Datum
int4_equint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);
	
	PG_RETURN_BOOL(val2 == val1);
}

Datum
int8_equint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_BOOL(val2 == val1);
}


Datum
uint1_ne(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
uint1_neint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_BOOL(val1 != val2);
}


Datum
uint1_neint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
uint1_neint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 != val2);
}


Datum
int2_neuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_BOOL(val2 != val1);
}

Datum
int4_neuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_BOOL(val2 != val1);
}

Datum
int8_neuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_BOOL(val2 != val1);
}


Datum
uint1_lt(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
uint1_ltint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);
	if (val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 < val2);
}


Datum
uint1_ltint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);
	if (val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 < val2);
}

Datum
uint1_ltint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);
	if (val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 < val2);
}


Datum
int2_ltuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);
	if (val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 < val1);
}

Datum
int4_ltuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);
	if (val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 < val1);
}

Datum
int8_ltuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);
	if (val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 < val1);
}

Datum
uint1_le(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
uint1_leint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);
	if (val2 < 0) PG_RETURN_BOOL(false);
	
	PG_RETURN_BOOL(val1 <= val2);
}

Datum
uint1_leint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);
	if (val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
uint1_leint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);
	if (val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
int2_leuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);
	if (val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 <= val1);
}

Datum
int4_leuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);
	if (val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 <= val1);
}

Datum
int8_leuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);
	if (val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 <= val1);
}


Datum
uint1_gt(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
uint1_gtint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);
	if (val2 < 0) PG_RETURN_BOOL(true);
	
	PG_RETURN_BOOL(val1 > val2);
}

Datum
uint1_gtint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);
	if (val2 < 0) PG_RETURN_BOOL(true);
	
	PG_RETURN_BOOL(val1 > val2);
}

Datum
uint1_gtint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);
	if (val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val1 > val2);
}

Datum
int2_gtuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);
	if (val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 > val1);
}

Datum
int4_gtuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);
	if (val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 > val1);
}

Datum
int8_gtuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);
	if (val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 > val1);
}


Datum
uint1_ge(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 >= val2);
}

Datum
uint1_geint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);
	if (val2 < 0) PG_RETURN_BOOL(true);
	
	PG_RETURN_BOOL(val1 >= val2);
}

Datum
uint1_geint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);
	if (val2 < 0) PG_RETURN_BOOL(true);
	
	PG_RETURN_BOOL(val1 >= val2);
}

Datum
uint1_geint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);
	if (val2 < 0) PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val1 >= val2);
}


Datum
int2_geuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);
	if (val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 >= val1);
}

Datum
int4_geuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);
	if (val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 >= val1);
}

Datum
int8_geuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);
	if (val2 < 0) PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 < val1);
}

Datum
uint1_seq(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint1_seqint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint1_seqint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint1_seqint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
int2_sequint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_BOOL(val2 == val1);
}

Datum
int4_sequint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_BOOL(val2 == val1);
}

Datum
int8_sequint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_BOOL(val2 == val1);
}

Datum
uint1_adduint1(PG_FUNCTION_ARGS)
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
uint1_addint2(PG_FUNCTION_ARGS)
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
uint1_addint4(PG_FUNCTION_ARGS)
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
uint1_addint8(PG_FUNCTION_ARGS)
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
int2_adduint1(PG_FUNCTION_ARGS)
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
int4_adduint1(PG_FUNCTION_ARGS)
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
int8_adduint1(PG_FUNCTION_ARGS)
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
uint1_negative(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		tempvalue = (int32)val1;
	PG_RETURN_INT32(-tempvalue);
}

Datum
uint1_subuint1(PG_FUNCTION_ARGS)
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
uint1_subint2(PG_FUNCTION_ARGS)
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
uint1_subint4(PG_FUNCTION_ARGS)
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
uint1_subint8(PG_FUNCTION_ARGS)
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
int2_subuint1(PG_FUNCTION_ARGS)
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
int4_subuint1(PG_FUNCTION_ARGS)
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
int8_subuint1(PG_FUNCTION_ARGS)
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
uint1_muluint1(PG_FUNCTION_ARGS)
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
uint1_mulint2(PG_FUNCTION_ARGS)
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
uint1_mulint4(PG_FUNCTION_ARGS)
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
uint1_mulint8(PG_FUNCTION_ARGS)
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
int2_muluint1(PG_FUNCTION_ARGS)
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
int4_muluint1(PG_FUNCTION_ARGS)
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
int8_muluint1(PG_FUNCTION_ARGS)
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
uint1_divuint1(PG_FUNCTION_ARGS)
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
uint1_divint2(PG_FUNCTION_ARGS)
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
uint1_divint4(PG_FUNCTION_ARGS)
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
uint1_divint8(PG_FUNCTION_ARGS)
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
int2_divuint1(PG_FUNCTION_ARGS)
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
int4_divuint1(PG_FUNCTION_ARGS)
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
int8_divuint1(PG_FUNCTION_ARGS)
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
uint1_exact_divuint1(PG_FUNCTION_ARGS)
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
uint1_exact_divint2(PG_FUNCTION_ARGS)
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
uint1_exact_divint4(PG_FUNCTION_ARGS)
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
uint1_exact_divint8(PG_FUNCTION_ARGS)
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
int2_exact_divuint1(PG_FUNCTION_ARGS)
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
int4_exact_divuint1(PG_FUNCTION_ARGS)
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
int8_exact_divuint1(PG_FUNCTION_ARGS)
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
uint1_moduint1(PG_FUNCTION_ARGS)
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
uint1_modint2(PG_FUNCTION_ARGS)
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
uint1_modint4(PG_FUNCTION_ARGS)
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
uint1_modint8(PG_FUNCTION_ARGS)
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
int2_moduint1(PG_FUNCTION_ARGS)
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
int4_moduint1(PG_FUNCTION_ARGS)
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
int8_moduint1(PG_FUNCTION_ARGS)
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
uint1_bitanduint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_UINT64(val1 & val2);
}
Datum
uint1_bitandint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);


	PG_RETURN_UINT64(val1 & val2);
}
Datum
uint1_bitandint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);

	PG_RETURN_UINT64(val1 & val2);
}
Datum
uint1_bitandint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 & val2);
}

Datum
int2_bitanduint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_UINT64(val2 & val1);
}
Datum
int4_bitanduint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_UINT64(val2 & val1);
}
Datum
int8_bitanduint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_UINT64(val2 & val1);
}

// bitor
Datum
uint1_bitoruint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_UINT64(val1 | val2);
}
Datum
uint1_bitorint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_UINT64(val1 | val2);
}
Datum
uint1_bitorint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);

	PG_RETURN_UINT64(val1 | val2);
}
Datum
uint1_bitorint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 | val2);
}

Datum
int2_bitoruint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_UINT64(val2 | val1);
}
Datum
int4_bitoruint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_UINT64(val2 | val1);
}
Datum
int8_bitoruint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_UINT64(val2 | val1);
}


// nega
Datum
uint1_negauint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint64		res;

	res = ~(uint64)val1;
	PG_RETURN_UINT64(res);
}

// bitxor
Datum
uint1_bitxoruint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_UINT64(val1 ^ val2);
}
Datum
uint1_bitxorint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_UINT64(val1 ^ val2);
}
Datum
uint1_bitxorint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);

	PG_RETURN_UINT64(val1 ^ val2);
}
Datum
uint1_bitxorint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 ^ val2);
}

Datum
int2_bitxoruint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_UINT64(val2 ^ val1);
}
Datum
int4_bitxoruint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_UINT64(val2 ^ val1);
}
Datum
int8_bitxoruint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_UINT64(val2 ^ val1);
}


// right move
Datum
uint1_rightmovuint1(PG_FUNCTION_ARGS)
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
uint1_rightmovint2(PG_FUNCTION_ARGS)
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
uint1_rightmovint4(PG_FUNCTION_ARGS)
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
uint1_rightmovint8(PG_FUNCTION_ARGS)
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
int2_rightmovuint1(PG_FUNCTION_ARGS)
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
int4_rightmovuint1(PG_FUNCTION_ARGS)
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
int8_rightmovuint1(PG_FUNCTION_ARGS)
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
uint1_leftmovuint1(PG_FUNCTION_ARGS)
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
uint1_leftmovint2(PG_FUNCTION_ARGS)
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
uint1_leftmovint4(PG_FUNCTION_ARGS)
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
uint1_leftmovint8(PG_FUNCTION_ARGS)
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
int2_leftmovuint1(PG_FUNCTION_ARGS)
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
int4_leftmovuint1(PG_FUNCTION_ARGS)
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
int8_leftmovuint1(PG_FUNCTION_ARGS)
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
uint1_bit_count(PG_FUNCTION_ARGS)
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
uint1_anduint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_UINT32(val1 && val2);
}
Datum
uint1_andint2(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int16		val2 = PG_GETARG_INT16(1);

	PG_RETURN_UINT32(val1 && val2);
}
Datum
uint1_andint4(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int32		val2 = PG_GETARG_INT32(1);

	PG_RETURN_UINT32(val1 && val2);
}
Datum
uint1_andint8(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	int64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT32(val1 && val2);
}

Datum
int2_anduint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int16		val2 = PG_GETARG_INT16(0);

	PG_RETURN_UINT32(val2 && val1);
}
Datum
int4_anduint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int32		val2 = PG_GETARG_INT32(0);

	PG_RETURN_UINT32(val2 && val1);
}
Datum
int8_anduint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(1);
	int64		val2 = PG_GETARG_INT64(0);

	PG_RETURN_UINT32(val2 && val1);
}

// invert ! 
Datum
uint1_invertuint1(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);

	PG_RETURN_UINT32(!val1);
}

// uint1 to boolean
Datum
uint1_boolean(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);

	PG_RETURN_BOOL(val1 != 0);
}

//like
Datum
uint1_like(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum
uint1_int8_like(PG_FUNCTION_ARGS)
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
uint1_text_like(PG_FUNCTION_ARGS)
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
uint1_notlike(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
uint1_int8_notlike(PG_FUNCTION_ARGS)
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
uint1_text_notlike(PG_FUNCTION_ARGS)
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
uint1_larger(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_UINT32(val1 > val2 ? val1 : val2);
}

Datum
uint1_smaller(PG_FUNCTION_ARGS)
{
	uint8		val1 = PG_GETARG_UINT32(0);
	uint8		val2 = PG_GETARG_UINT32(1);

	PG_RETURN_UINT32(val1 < val2 ? val1 : val2);
}

Datum
uint1_avg_accum(PG_FUNCTION_ARGS)
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
uint1_sum(PG_FUNCTION_ARGS)
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
