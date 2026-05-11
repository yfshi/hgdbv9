/*------------------------------------------------------------------
 * 
 * File: mysuint2.c
 *
 * Abstract: 
 * 		Compatible with MySQL's UINT data type.
 *
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_my/src/datatype/mysuint2.c
 *
 *------------------------------------------------------------------
 */

#include "postgres.h"
#include "libpq/pqformat.h"
#include "utils/builtins.h"
#include "../include/ivorysql_my.h"

/* Begin - ReqID:SRS-MYSQL-NUMBERICTYPE-uint2 */
typedef struct Int8TransTypeData
{
	int64		count;
	int64		sum;
} Int8TransTypeData;
 
PG_FUNCTION_INFO_V1(uint2_in);
PG_FUNCTION_INFO_V1(uint2_out);
PG_FUNCTION_INFO_V1(uint2_recv);
PG_FUNCTION_INFO_V1(uint2_send);
PG_FUNCTION_INFO_V1(uint2_typmodin);
PG_FUNCTION_INFO_V1(uint2_typmodout);
PG_FUNCTION_INFO_V1(uint2_scale);

PG_FUNCTION_INFO_V1(int8_uint2);
PG_FUNCTION_INFO_V1(int4_uint2);
PG_FUNCTION_INFO_V1(int2_uint2);
PG_FUNCTION_INFO_V1(boolean_uint2);
PG_FUNCTION_INFO_V1(uint2_int8);
PG_FUNCTION_INFO_V1(uint2_numeric);

PG_FUNCTION_INFO_V1(uint2_eq);
PG_FUNCTION_INFO_V1(uint2_eqint2);
PG_FUNCTION_INFO_V1(uint2_eqint4);
PG_FUNCTION_INFO_V1(uint2_eqint8);
PG_FUNCTION_INFO_V1(int2_equint2);
PG_FUNCTION_INFO_V1(int4_equint2);
PG_FUNCTION_INFO_V1(int8_equint2);

PG_FUNCTION_INFO_V1(uint2_ne);
PG_FUNCTION_INFO_V1(uint2_neint2);
PG_FUNCTION_INFO_V1(uint2_neint4);
PG_FUNCTION_INFO_V1(uint2_neint8);
PG_FUNCTION_INFO_V1(int2_neuint2);
PG_FUNCTION_INFO_V1(int4_neuint2);
PG_FUNCTION_INFO_V1(int8_neuint2);

PG_FUNCTION_INFO_V1(uint2_lt);
PG_FUNCTION_INFO_V1(uint2_ltint2);
PG_FUNCTION_INFO_V1(uint2_ltint4);
PG_FUNCTION_INFO_V1(uint2_ltint8);
PG_FUNCTION_INFO_V1(int2_ltuint2);
PG_FUNCTION_INFO_V1(int4_ltuint2);
PG_FUNCTION_INFO_V1(int8_ltuint2);

PG_FUNCTION_INFO_V1(uint2_gt);
PG_FUNCTION_INFO_V1(uint2_gtint2);
PG_FUNCTION_INFO_V1(uint2_gtint4);
PG_FUNCTION_INFO_V1(uint2_gtint8);
PG_FUNCTION_INFO_V1(int2_gtuint2);
PG_FUNCTION_INFO_V1(int4_gtuint2);
PG_FUNCTION_INFO_V1(int8_gtuint2);

PG_FUNCTION_INFO_V1(uint2_le);
PG_FUNCTION_INFO_V1(uint2_leint2);
PG_FUNCTION_INFO_V1(uint2_leint4);
PG_FUNCTION_INFO_V1(uint2_leint8);
PG_FUNCTION_INFO_V1(int2_leuint2);
PG_FUNCTION_INFO_V1(int4_leuint2);
PG_FUNCTION_INFO_V1(int8_leuint2);

PG_FUNCTION_INFO_V1(uint2_ge);
PG_FUNCTION_INFO_V1(uint2_geint2);
PG_FUNCTION_INFO_V1(uint2_geint4);
PG_FUNCTION_INFO_V1(uint2_geint8);
PG_FUNCTION_INFO_V1(int2_geuint2);
PG_FUNCTION_INFO_V1(int4_geuint2);
PG_FUNCTION_INFO_V1(int8_geuint2);

PG_FUNCTION_INFO_V1(uint2_seq);
PG_FUNCTION_INFO_V1(uint2_seqint2);
PG_FUNCTION_INFO_V1(uint2_seqint4);
PG_FUNCTION_INFO_V1(uint2_seqint8);
PG_FUNCTION_INFO_V1(int2_sequint2);
PG_FUNCTION_INFO_V1(int4_sequint2);
PG_FUNCTION_INFO_V1(int8_sequint2);

PG_FUNCTION_INFO_V1(uint2_adduint2);
PG_FUNCTION_INFO_V1(uint2_addint2);
PG_FUNCTION_INFO_V1(uint2_addint4);
PG_FUNCTION_INFO_V1(uint2_addint8);
PG_FUNCTION_INFO_V1(int2_adduint2);
PG_FUNCTION_INFO_V1(int4_adduint2);
PG_FUNCTION_INFO_V1(int8_adduint2);

PG_FUNCTION_INFO_V1(uint2_negative);

PG_FUNCTION_INFO_V1(uint2_subuint2);
PG_FUNCTION_INFO_V1(uint2_subint2);
PG_FUNCTION_INFO_V1(uint2_subint4);
PG_FUNCTION_INFO_V1(uint2_subint8);
PG_FUNCTION_INFO_V1(int2_subuint2);
PG_FUNCTION_INFO_V1(int4_subuint2);
PG_FUNCTION_INFO_V1(int8_subuint2);

PG_FUNCTION_INFO_V1(uint2_muluint2);
PG_FUNCTION_INFO_V1(uint2_mulint2);
PG_FUNCTION_INFO_V1(uint2_mulint4);
PG_FUNCTION_INFO_V1(uint2_mulint8);
PG_FUNCTION_INFO_V1(int2_muluint2);
PG_FUNCTION_INFO_V1(int4_muluint2);
PG_FUNCTION_INFO_V1(int8_muluint2);

PG_FUNCTION_INFO_V1(uint2_divuint2);
PG_FUNCTION_INFO_V1(uint2_divint2);
PG_FUNCTION_INFO_V1(uint2_divint4);
PG_FUNCTION_INFO_V1(uint2_divint8);
PG_FUNCTION_INFO_V1(int2_divuint2);
PG_FUNCTION_INFO_V1(int4_divuint2);
PG_FUNCTION_INFO_V1(int8_divuint2);

PG_FUNCTION_INFO_V1(uint2_exact_divuint2);
PG_FUNCTION_INFO_V1(uint2_exact_divint2);
PG_FUNCTION_INFO_V1(uint2_exact_divint4);
PG_FUNCTION_INFO_V1(uint2_exact_divint8);
PG_FUNCTION_INFO_V1(int2_exact_divuint2);
PG_FUNCTION_INFO_V1(int4_exact_divuint2);
PG_FUNCTION_INFO_V1(int8_exact_divuint2);

PG_FUNCTION_INFO_V1(uint2_moduint2);
PG_FUNCTION_INFO_V1(uint2_modint2);
PG_FUNCTION_INFO_V1(uint2_modint4);
PG_FUNCTION_INFO_V1(uint2_modint8);
PG_FUNCTION_INFO_V1(int2_moduint2);
PG_FUNCTION_INFO_V1(int4_moduint2);
PG_FUNCTION_INFO_V1(int8_moduint2);

PG_FUNCTION_INFO_V1(uint2_bitanduint2);
PG_FUNCTION_INFO_V1(uint2_bitandint2);
PG_FUNCTION_INFO_V1(uint2_bitandint4);
PG_FUNCTION_INFO_V1(uint2_bitandint8);
PG_FUNCTION_INFO_V1(int2_bitanduint2);
PG_FUNCTION_INFO_V1(int4_bitanduint2);
PG_FUNCTION_INFO_V1(int8_bitanduint2);

PG_FUNCTION_INFO_V1(uint2_bitoruint2);
PG_FUNCTION_INFO_V1(uint2_bitorint2);
PG_FUNCTION_INFO_V1(uint2_bitorint4);
PG_FUNCTION_INFO_V1(uint2_bitorint8);
PG_FUNCTION_INFO_V1(int2_bitoruint2);
PG_FUNCTION_INFO_V1(int4_bitoruint2);
PG_FUNCTION_INFO_V1(int8_bitoruint2);

PG_FUNCTION_INFO_V1(uint2_negauint2);

PG_FUNCTION_INFO_V1(uint2_bitxoruint2);
PG_FUNCTION_INFO_V1(uint2_bitxorint2);
PG_FUNCTION_INFO_V1(uint2_bitxorint4);
PG_FUNCTION_INFO_V1(uint2_bitxorint8);
PG_FUNCTION_INFO_V1(int2_bitxoruint2);
PG_FUNCTION_INFO_V1(int4_bitxoruint2);
PG_FUNCTION_INFO_V1(int8_bitxoruint2);

PG_FUNCTION_INFO_V1(uint2_rightmovuint2);
PG_FUNCTION_INFO_V1(uint2_rightmovint2);
PG_FUNCTION_INFO_V1(uint2_rightmovint4);
PG_FUNCTION_INFO_V1(uint2_rightmovint8);
PG_FUNCTION_INFO_V1(int2_rightmovuint2);
PG_FUNCTION_INFO_V1(int4_rightmovuint2);
PG_FUNCTION_INFO_V1(int8_rightmovuint2);

PG_FUNCTION_INFO_V1(uint2_leftmovuint2);
PG_FUNCTION_INFO_V1(uint2_leftmovint2);
PG_FUNCTION_INFO_V1(uint2_leftmovint4);
PG_FUNCTION_INFO_V1(uint2_leftmovint8);
PG_FUNCTION_INFO_V1(int2_leftmovuint2);
PG_FUNCTION_INFO_V1(int4_leftmovuint2);
PG_FUNCTION_INFO_V1(int8_leftmovuint2);

PG_FUNCTION_INFO_V1(uint2_bit_count);

PG_FUNCTION_INFO_V1(uint2_anduint2);
PG_FUNCTION_INFO_V1(uint2_andint2);
PG_FUNCTION_INFO_V1(uint2_andint4);
PG_FUNCTION_INFO_V1(uint2_andint8);
PG_FUNCTION_INFO_V1(int2_anduint2);
PG_FUNCTION_INFO_V1(int4_anduint2);
PG_FUNCTION_INFO_V1(int8_anduint2);

PG_FUNCTION_INFO_V1(uint2_invertuint2);

PG_FUNCTION_INFO_V1(uint2_boolean);

PG_FUNCTION_INFO_V1(uint2_like);
PG_FUNCTION_INFO_V1(uint2_int8_like);
PG_FUNCTION_INFO_V1(uint2_text_like);
PG_FUNCTION_INFO_V1(uint2_notlike);
PG_FUNCTION_INFO_V1(uint2_int8_notlike);
PG_FUNCTION_INFO_V1(uint2_text_notlike);

PG_FUNCTION_INFO_V1(uint2_larger);
PG_FUNCTION_INFO_V1(uint2_smaller);

PG_FUNCTION_INFO_V1(uint2_avg_accum);
PG_FUNCTION_INFO_V1(uint2_sum);
 
/*****************************************************************************
 *	 uint2 ADT
 *****************************************************************************/
 
/* 
 * uint2_in
 * Given uint2 text string, convert to internal uint2 format.
 */
Datum
uint2_in(PG_FUNCTION_ARGS)
{
	int64 	flag = 0;
	char 	*num = PG_GETARG_CSTRING(0);
	uint16 	ret;

	if (PG_NARGS() > 3)
		flag = PG_GETARG_INT64(3);

	ret = my_atoi_uint16(num, flag & TRANSFORM_FLAG_IN_INSERT);

	PG_RETURN_UINT16(ret);
}
 
/* 
 * uint2_out
 * Given internal format uint2, convert to text string.
 */
Datum
uint2_out(PG_FUNCTION_ARGS)
{
	uint16 arg1 = PG_GETARG_UINT32(0);
	char *str = (char *)palloc(256); /* sign, 11 digits, '\0' */

	pg_lltoa(arg1, str);

	PG_RETURN_CSTRING(str);
}

/*
 *uint2_recv			- converts external binary format to uint2
 */
Datum uint2_recv(PG_FUNCTION_ARGS)
{
	StringInfo buf = (StringInfo)PG_GETARG_POINTER(0);

	PG_RETURN_UINT32((uint32)pq_getmsgint(buf, sizeof(uint32)));
}

/*
 *uint2_send			- converts uint2 to binary format
 */
Datum uint2_send(PG_FUNCTION_ARGS)
{
	uint32 arg1 = PG_GETARG_UINT32(0);
	StringInfoData buf;

	pq_begintypsend(&buf);
	pq_sendint32(&buf, arg1);
	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

/* common code for mysdatetime_typmodin */
Datum uint2_typmodin(PG_FUNCTION_ARGS)
{
	ArrayType *ta = PG_GETARG_ARRAYTYPE_P(0);
	int32 *tl = NULL;
	int n = 0;

	tl = ArrayGetIntegerTypmods(ta, &n);

	PG_RETURN_INT32(*tl);
}

/* common code for mysdatetimetypmodout and mysdatetimetypmodout */
Datum uint2_typmodout(PG_FUNCTION_ARGS)
{
	int32 typmod = PG_GETARG_INT32(0);
	char *res = (char *)palloc(64);

	if (typmod >= 0)
		snprintf(res, 64, "(%d)", typmod);
	else
		*res = '\0';

	PG_RETURN_CSTRING(res);
}

Datum uint2_scale(PG_FUNCTION_ARGS)
{
	int64 num = PG_GETARG_INT64(0);

	PG_RETURN_UINT32(num);
}

/* Begin - BUG#Z7196 */
Datum
int8_uint2(PG_FUNCTION_ARGS)
{
	int64	val1 = PG_GETARG_INT64(0);
	uint16	val2 = val1;
	bool	is_insert = CALL_IN_INSERT();

	if (is_insert)
	{
		if (val1 < 0 || val1 > PG_UINT16_MAX)
		{
			int EREPORT_LEVEL = (!is_strict_mode()) ? WARNING : ERROR;
			ereport(EREPORT_LEVEL, (errmsg("Out of range value for column")));
			val2 = val1 < 0 ? 0 : PG_UINT16_MAX;
		}
	}
	else
	{
		if (val1 < 0 || val1 > PG_UINT16_MAX)
		{
			ereport(WARNING, (errmsg("integer unsigned out of range")));
			val2 = val1 < 0 ? 0 : PG_UINT16_MAX;
		}
	}

	PG_RETURN_UINT16(val2);
}

Datum
int4_uint2(PG_FUNCTION_ARGS)
{
	int32	val1 = PG_GETARG_INT32(0);
	uint16	val2 = val1;
	bool	is_insert = CALL_IN_INSERT();

	if (is_insert)
	{
		if (val1 < 0 || val1 > PG_UINT16_MAX)
		{
			int EREPORT_LEVEL = (!is_strict_mode()) ? WARNING : ERROR;
			ereport(EREPORT_LEVEL, (errmsg("Out of range value for column")));
			val2 = val1 < 0 ? 0 : PG_UINT16_MAX;
		}
	}
	else
	{
		if (val1 < 0 || val1 > PG_UINT16_MAX)
		{
			ereport(WARNING, (errmsg("integer unsigned out of range")));
			val2 = val1 < 0 ? 0 : PG_UINT16_MAX;
		}
	}

	PG_RETURN_UINT16(val2);
}

Datum
int2_uint2(PG_FUNCTION_ARGS)
{
	int16	val1 = PG_GETARG_INT16(0);
	uint16	val2 = val1;
	bool	is_insert = CALL_IN_INSERT();

	if (is_insert)
	{
		if (val1 < 0 || val1 > PG_UINT16_MAX)
		{
			int EREPORT_LEVEL = (!is_strict_mode()) ? WARNING : ERROR;
			ereport(EREPORT_LEVEL, (errmsg("Out of range value for column")));
			val2 = val1 < 0 ? 0 : PG_UINT16_MAX;
		}
	}
	else
	{
		if (val1 < 0 || val1 > PG_UINT16_MAX)
		{
			ereport(WARNING, (errmsg("integer unsigned out of range")));
			val2 = val1 < 0 ? 0 : PG_UINT16_MAX;
		}
	}

	PG_RETURN_UINT16(val2);
}
/* End - BUG#Z7196 */

Datum boolean_uint2(PG_FUNCTION_ARGS)
{
	PG_RETURN_UINT32(PG_GETARG_BOOL(0));
}

/* Begin - BUG#Z7200 */
Datum
uint2_int8(PG_FUNCTION_ARGS)
{
	uint16	val1 = PG_GETARG_UINT16(0);

	PG_RETURN_INT64(val1);
}
/* End - BUG#Z7200 */

/* Begin - BUG#Z7615 */
Datum
uint2_numeric(PG_FUNCTION_ARGS)
{
	uint16	val1 = PG_GETARG_UINT16(0);

	PG_RETURN_NUMERIC(uint32_to_numeric((uint32) val1));
}
/* End - BUG#Z7615 */

/*****************************************************************************
 *	 Operator Function														 *
 *****************************************************************************/
Datum uint2_eq(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	uint16 val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum uint2_eqint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int16 val2 = PG_GETARG_INT16(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum uint2_eqint4(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int32 val2 = PG_GETARG_INT32(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum uint2_eqint8(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int64 val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum int2_equint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int16 val2 = PG_GETARG_INT16(0);

	PG_RETURN_BOOL(val2 == val1);
}

Datum int4_equint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int32 val2 = PG_GETARG_INT32(0);

	PG_RETURN_BOOL(val2 == val1);
}

Datum int8_equint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int64 val2 = PG_GETARG_INT64(0);

	PG_RETURN_BOOL(val2 == val1);
}

Datum uint2_ne(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	uint16 val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum uint2_neint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int16 val2 = PG_GETARG_INT16(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum uint2_neint4(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int32 val2 = PG_GETARG_INT32(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum uint2_neint8(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int64 val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum int2_neuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int16 val2 = PG_GETARG_INT16(0);

	PG_RETURN_BOOL(val2 != val1);
}

Datum int4_neuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int32 val2 = PG_GETARG_INT32(0);

	PG_RETURN_BOOL(val2 != val1);
}

Datum int8_neuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int64 val2 = PG_GETARG_INT64(0);

	PG_RETURN_BOOL(val2 != val1);
}

Datum uint2_lt(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	uint16 val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 < val2);
}

Datum uint2_ltint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int16 val2 = PG_GETARG_INT16(1);
	if (val2 < 0)
		PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 < val2);
}

Datum uint2_ltint4(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int32 val2 = PG_GETARG_INT32(1);
	if (val2 < 0)
		PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 < val2);
}

Datum uint2_ltint8(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int64 val2 = PG_GETARG_INT64(1);
	if (val2 < 0)
		PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 < val2);
}

Datum int2_ltuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int16 val2 = PG_GETARG_INT16(0);
	if (val2 < 0)
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 < val1);
}

Datum int4_ltuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int32 val2 = PG_GETARG_INT32(0);
	if (val2 < 0)
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 < val1);
}

Datum int8_ltuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int64 val2 = PG_GETARG_INT64(0);
	if (val2 < 0)
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 < val1);
}

Datum uint2_le(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	uint16 val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum uint2_leint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int16 val2 = PG_GETARG_INT16(1);
	if (val2 < 0)
		PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum uint2_leint4(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int32 val2 = PG_GETARG_INT32(1);
	if (val2 < 0)
		PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum uint2_leint8(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int64 val2 = PG_GETARG_INT64(1);
	if (val2 < 0)
		PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val1 <= val2);
}

Datum int2_leuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int16 val2 = PG_GETARG_INT16(0);
	if (val2 < 0)
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 <= val1);
}

Datum int4_leuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int32 val2 = PG_GETARG_INT32(0);
	if (val2 < 0)
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 <= val1);
}

Datum int8_leuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int64 val2 = PG_GETARG_INT64(0);
	if (val2 < 0)
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val2 <= val1);
}

Datum uint2_gt(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	uint16 val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 > val2);
}

Datum uint2_gtint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int16 val2 = PG_GETARG_INT16(1);
	if (val2 < 0)
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val1 > val2);
}

Datum uint2_gtint4(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int32 val2 = PG_GETARG_INT32(1);
	if (val2 < 0)
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val1 > val2);
}

Datum uint2_gtint8(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int64 val2 = PG_GETARG_INT64(1);
	if (val2 < 0)
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val1 > val2);
}

Datum int2_gtuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int16 val2 = PG_GETARG_INT16(0);
	if (val2 < 0)
		PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 > val1);
}

Datum int4_gtuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int32 val2 = PG_GETARG_INT32(0);
	if (val2 < 0)
		PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 > val1);
}

Datum int8_gtuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int64 val2 = PG_GETARG_INT64(0);
	if (val2 < 0)
		PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 > val1);
}

Datum uint2_ge(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	uint16 val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 >= val2);
}

Datum uint2_geint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int16 val2 = PG_GETARG_INT16(1);
	if (val2 < 0)
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val1 >= val2);
}

Datum uint2_geint4(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int32 val2 = PG_GETARG_INT32(1);
	if (val2 < 0)
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val1 >= val2);
}

Datum uint2_geint8(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int64 val2 = PG_GETARG_INT64(1);
	if (val2 < 0)
		PG_RETURN_BOOL(true);

	PG_RETURN_BOOL(val1 >= val2);
}

Datum int2_geuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int16 val2 = PG_GETARG_INT16(0);
	if (val2 < 0)
		PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 >= val1);
}

Datum int4_geuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int32 val2 = PG_GETARG_INT32(0);
	if (val2 < 0)
		PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 >= val1);
}

Datum int8_geuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int64 val2 = PG_GETARG_INT64(0);
	if (val2 < 0)
		PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(val2 < val1);
}

Datum uint2_seq(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	uint16 val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum uint2_seqint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int16 val2 = PG_GETARG_INT16(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum uint2_seqint4(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int32 val2 = PG_GETARG_INT32(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum uint2_seqint8(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int64 val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum int2_sequint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int16 val2 = PG_GETARG_INT16(0);

	PG_RETURN_BOOL(val2 == val1);
}

Datum int4_sequint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int32 val2 = PG_GETARG_INT32(0);

	PG_RETURN_BOOL(val2 == val1);
}

Datum int8_sequint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int64 val2 = PG_GETARG_INT64(0);

	PG_RETURN_BOOL(val2 == val1);
}

Datum uint2_adduint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	uint16 val2 = PG_GETARG_UINT32(1);
	if ((val1 + val2) < 0 || (val1 + val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT32(val1 + val2);
}

Datum uint2_addint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int16 val2 = PG_GETARG_INT16(1);
	if ((val1 + val2) < 0 || (val1 + val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT32(val1 + val2);
}

Datum uint2_addint4(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int32 val2 = PG_GETARG_INT32(1);
	if ((val1 + val2) < 0 || (val1 + val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT32(val1 + val2);
}

Datum uint2_addint8(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int64 val2 = PG_GETARG_INT64(1);
	if ((val1 + val2) < 0 || (val1 + val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val1 + val2);
}

Datum int2_adduint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int16 val2 = PG_GETARG_INT16(0);
	if ((val1 + val2) < 0 || (val1 + val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT32(val1 + val2);
}

Datum int4_adduint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int32 val2 = PG_GETARG_INT32(0);
	if ((val1 + val2) < 0 || (val1 + val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT32(val1 + val2);
}

Datum int8_adduint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int64 val2 = PG_GETARG_INT64(0);
	if ((val1 + val2) < 0 || (val1 + val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val1 + val2);
}

Datum uint2_negative(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int32 tempvalue = (int32)val1;
	PG_RETURN_INT32(-tempvalue);
}

Datum uint2_subuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	uint16 val2 = PG_GETARG_UINT32(1);
	if ((val1 < val2) || (val1 - val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}

	PG_RETURN_UINT32(val1 - val2);
}

Datum uint2_subint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int16 val2 = PG_GETARG_INT16(1);
	if ((val1 - val2) < 0 || (val1 - val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT32(val1 - val2);
}

Datum uint2_subint4(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int32 val2 = PG_GETARG_INT32(1);
	if ((val1 - val2) < 0 || (val1 - val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT32(val1 - val2);
}

Datum uint2_subint8(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int64 val2 = PG_GETARG_INT64(1);
	if ((val1 - val2) < 0 || (val1 - val2) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val1 - val2);
}

Datum int2_subuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int16 val2 = PG_GETARG_INT16(0);
	if (val2 < 0 || (val2 - val1) < 0 || (val2 - val1) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT32(val2 - val1);
}

Datum int4_subuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int32 val2 = PG_GETARG_INT32(0);
	if (val2 < 0 || (val2 - val1) < 0 || (val2 - val1) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT32(val2 - val1);
}

Datum int8_subuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int64 val2 = PG_GETARG_INT64(0);
	if (val2 < 0 || (val2 - val1) < 0 || (val2 - val1) > PG_UINT32_MAX)
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(val2 - val1);
}

// *
Datum uint2_muluint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	uint16 val2 = PG_GETARG_UINT32(1);
	uint32 res = val1 * val2;
	if (val2 < 0 || val2 > (PG_UINT32_MAX / val1))
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(res);
}
Datum uint2_mulint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int16 val2 = PG_GETARG_INT16(1);
	uint32 res = val1 * val2;
	if (val2 < 0 || val2 > (PG_UINT32_MAX / val1))
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(res);
}
Datum uint2_mulint4(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int32 val2 = PG_GETARG_INT32(1);
	uint32 res = val1 * val2;
	if (val2 < 0 || val2 > (PG_UINT32_MAX / val1))
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(res);
}
Datum uint2_mulint8(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int64 val2 = PG_GETARG_INT64(1);
	uint32 res = val1 * val2;
	if (val2 < 0 || val2 > (PG_UINT32_MAX / val1))
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(res);
}

Datum int2_muluint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int16 val2 = PG_GETARG_INT16(0);
	uint32 res = val2 * val1;
	if (val2 < 0 || val2 > (PG_UINT32_MAX / val1))
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(res);
}
Datum int4_muluint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int32 val2 = PG_GETARG_INT32(0);
	uint32 res = val2 * val1;
	if (val2 < 0 || val2 > (PG_UINT32_MAX / val1))
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(res);
}
Datum int8_muluint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int64 val2 = PG_GETARG_INT64(0);
	uint32 res = val2 * val1;
	if (val2 < 0 || val2 > (PG_UINT32_MAX / val1))
	{
		ereport(ERROR, (errmsg("integer unsigned out of range")));
	}
	PG_RETURN_UINT64(res);
}

// div
Datum uint2_divuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	uint16 val2 = PG_GETARG_UINT32(1);
	Numeric res;

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
Datum uint2_divint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int16 val2 = PG_GETARG_INT16(1);
	Numeric res;

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
Datum uint2_divint4(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int32 val2 = PG_GETARG_INT32(1);
	Numeric res;

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
Datum uint2_divint8(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int64 val2 = PG_GETARG_INT64(1);
	Numeric res;

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

Datum int2_divuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int16 val2 = PG_GETARG_INT16(0);
	Numeric res;

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
Datum int4_divuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int32 val2 = PG_GETARG_INT32(0);
	Numeric res;

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
Datum int8_divuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int64 val2 = PG_GETARG_INT64(0);
	Numeric res;

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
Datum uint2_exact_divuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	uint16 val2 = PG_GETARG_UINT32(1);
	if (val2 < 0)
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
Datum uint2_exact_divint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int16 val2 = PG_GETARG_INT16(1);
	if (val2 < 0)
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
Datum uint2_exact_divint4(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int32 val2 = PG_GETARG_INT32(1);
	if (val2 < 0)
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
Datum uint2_exact_divint8(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int64 val2 = PG_GETARG_INT64(1);
	if (val2 < 0)
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

Datum int2_exact_divuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int16 val2 = PG_GETARG_INT16(0);
	if (val2 < 0)
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
Datum int4_exact_divuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int32 val2 = PG_GETARG_INT32(0);
	if (val2 < 0)
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
Datum int8_exact_divuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int64 val2 = PG_GETARG_INT64(0);
	if (val2 < 0)
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
Datum uint2_moduint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	uint16 val2 = PG_GETARG_UINT32(1);
	if (val2 < 0)
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
Datum uint2_modint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int16 val2 = PG_GETARG_INT16(1);
	if (val2 < 0)
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
Datum uint2_modint4(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int32 val2 = PG_GETARG_INT32(1);
	if (val2 < 0)
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
Datum uint2_modint8(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int64 val2 = PG_GETARG_INT64(1);
	if (val2 < 0)
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

Datum int2_moduint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int16 val2 = PG_GETARG_INT16(0);
	if (val2 < 0)
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
Datum int4_moduint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int32 val2 = PG_GETARG_INT32(0);
	if (val2 < 0)
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
Datum int8_moduint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int64 val2 = PG_GETARG_INT64(0);
	if (val2 < 0)
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
Datum uint2_bitanduint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	uint16 val2 = PG_GETARG_UINT32(1);

	PG_RETURN_UINT64(val1 & val2);
}
Datum uint2_bitandint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int16 val2 = PG_GETARG_INT16(1);

	PG_RETURN_UINT64(val1 & val2);
}
Datum uint2_bitandint4(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int32 val2 = PG_GETARG_INT32(1);

	PG_RETURN_UINT64(val1 & val2);
}
Datum uint2_bitandint8(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int64 val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 & val2);
}

Datum int2_bitanduint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int16 val2 = PG_GETARG_INT16(0);

	PG_RETURN_UINT64(val2 & val1);
}
Datum int4_bitanduint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int32 val2 = PG_GETARG_INT32(0);

	PG_RETURN_UINT64(val2 & val1);
}
Datum int8_bitanduint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int64 val2 = PG_GETARG_INT64(0);

	PG_RETURN_UINT64(val2 & val1);
}

// bitor
Datum uint2_bitoruint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	uint16 val2 = PG_GETARG_UINT32(1);

	PG_RETURN_UINT64(val1 | val2);
}
Datum uint2_bitorint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int16 val2 = PG_GETARG_INT16(1);

	PG_RETURN_UINT64(val1 | val2);
}
Datum uint2_bitorint4(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int32 val2 = PG_GETARG_INT32(1);

	PG_RETURN_UINT64(val1 | val2);
}
Datum uint2_bitorint8(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int64 val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 | val2);
}

Datum int2_bitoruint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int16 val2 = PG_GETARG_INT16(0);

	PG_RETURN_UINT64(val2 | val1);
}
Datum int4_bitoruint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int32 val2 = PG_GETARG_INT32(0);

	PG_RETURN_UINT64(val2 | val1);
}
Datum int8_bitoruint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int64 val2 = PG_GETARG_INT64(0);

	PG_RETURN_UINT64(val2 | val1);
}

// nega
Datum uint2_negauint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	uint64 res;

	res = ~(uint64)val1;
	PG_RETURN_UINT64(res);
}

// bitxor
Datum uint2_bitxoruint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	uint16 val2 = PG_GETARG_UINT32(1);

	PG_RETURN_UINT64(val1 ^ val2);
}
Datum uint2_bitxorint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int16 val2 = PG_GETARG_INT16(1);

	PG_RETURN_UINT64(val1 ^ val2);
}
Datum uint2_bitxorint4(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int32 val2 = PG_GETARG_INT32(1);

	PG_RETURN_UINT64(val1 ^ val2);
}
Datum uint2_bitxorint8(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int64 val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT64(val1 ^ val2);
}

Datum int2_bitxoruint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int16 val2 = PG_GETARG_INT16(0);

	PG_RETURN_UINT64(val2 ^ val1);
}
Datum int4_bitxoruint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int32 val2 = PG_GETARG_INT32(0);

	PG_RETURN_UINT64(val2 ^ val1);
}
Datum int8_bitxoruint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int64 val2 = PG_GETARG_INT64(0);

	PG_RETURN_UINT64(val2 ^ val1);
}

// right move
Datum uint2_rightmovuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	uint16 val2 = PG_GETARG_UINT32(1);

	if (val2 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}

	PG_RETURN_UINT64((uint64)val1 >> val2);
}
Datum uint2_rightmovint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int16 val2 = PG_GETARG_INT16(1);

	if (val2 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64((uint64)val1 >> val2);
}
Datum uint2_rightmovint4(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int32 val2 = PG_GETARG_INT32(1);

	if (val2 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64((uint64)val1 >> val2);
}
Datum uint2_rightmovint8(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int64 val2 = PG_GETARG_INT64(1);

	if (val2 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64((uint64)val1 >> val2);
}

Datum int2_rightmovuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int16 val2 = PG_GETARG_INT16(0);

	if (val1 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val2 >> val1);
}
Datum int4_rightmovuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int32 val2 = PG_GETARG_INT32(0);

	if (val1 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val2 >> val1);
}
Datum int8_rightmovuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int64 val2 = PG_GETARG_INT64(0);

	if (val1 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val2 >> val1);
}

// left move
Datum uint2_leftmovuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	uint16 val2 = PG_GETARG_UINT32(1);

	if (val2 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64((uint64)val1 << val2);
}
Datum uint2_leftmovint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int16 val2 = PG_GETARG_INT16(1);

	if (val2 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64((uint64)val1 << val2);
}
Datum uint2_leftmovint4(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int32 val2 = PG_GETARG_INT32(1);

	if (val2 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64((uint64)val1 << val2);
}
Datum uint2_leftmovint8(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int64 val2 = PG_GETARG_INT64(1);

	if (val2 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64((uint64)val1 << val2);
}

Datum int2_leftmovuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int16 val2 = PG_GETARG_INT16(0);

	if (val1 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val2 << val1);
}
Datum int4_leftmovuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int32 val2 = PG_GETARG_INT32(0);

	if (val1 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val2 << val1);
}
Datum int8_leftmovuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int64 val2 = PG_GETARG_INT64(0);

	if (val1 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64(val2 << val1);
}

// BIT_COUNT
Datum uint2_bit_count(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	uint32 count = 0;

	while (val1)
	{
		count += val1 & 1;
		val1 >>= 1;
	}

	PG_RETURN_UINT32(count);
}

// and &&
Datum uint2_anduint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	uint16 val2 = PG_GETARG_UINT32(1);

	PG_RETURN_UINT32(val1 && val2);
}
Datum uint2_andint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int16 val2 = PG_GETARG_INT16(1);

	PG_RETURN_UINT32(val1 && val2);
}
Datum uint2_andint4(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int32 val2 = PG_GETARG_INT32(1);

	PG_RETURN_UINT32(val1 && val2);
}
Datum uint2_andint8(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int64 val2 = PG_GETARG_INT64(1);

	PG_RETURN_UINT32(val1 && val2);
}

Datum int2_anduint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int16 val2 = PG_GETARG_INT16(0);

	PG_RETURN_UINT32(val2 && val1);
}
Datum int4_anduint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int32 val2 = PG_GETARG_INT32(0);

	PG_RETURN_UINT32(val2 && val1);
}
Datum int8_anduint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(1);
	int64 val2 = PG_GETARG_INT64(0);

	PG_RETURN_UINT32(val2 && val1);
}

// invert !
Datum uint2_invertuint2(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);

	PG_RETURN_UINT32(!val1);
}

// uint2 to boolean
Datum uint2_boolean(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);

	PG_RETURN_BOOL(val1 != 0);
}

// like
Datum uint2_like(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	uint16 val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 == val2);
}

Datum uint2_int8_like(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int64 val2 = PG_GETARG_INT64(1);
	if (val2 < 0)
	{
		PG_RETURN_BOOL(false);
	}
	PG_RETURN_BOOL(val1 == val2);
}

Datum uint2_text_like(PG_FUNCTION_ARGS)
{
	uint32 val1 = PG_GETARG_UINT32(0);
	Datum val2 = PG_GETARG_DATUM(1);
	text *val1text = NULL;
	bool result = false;
	char *val1str = (char *)palloc(256); /* sign, 11 digits, '\0' */

	pg_lltoa(val1, val1str);

	val1text = cstring_to_text(val1str);
	result = DatumGetBool(DirectFunctionCall2(textlike, Int64GetDatum(val1text), val2));

	PG_RETURN_BOOL(result);
}

Datum uint2_notlike(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	uint16 val2 = PG_GETARG_UINT32(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum uint2_int8_notlike(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	int64 val2 = PG_GETARG_INT64(1);
	if (val2 < 0)
	{
		PG_RETURN_BOOL(true);
	}
	PG_RETURN_BOOL(val1 != val2);
}

Datum uint2_text_notlike(PG_FUNCTION_ARGS)
{
	uint32 val1 = PG_GETARG_UINT32(0);
	Datum val2 = PG_GETARG_DATUM(1);
	text *val1text = NULL;
	bool result = false;
	char *val1str = (char *)palloc(256); /* sign, 11 digits, '\0' */

	pg_lltoa(val1, val1str);

	val1text = cstring_to_text(val1str);
	result = DatumGetBool(DirectFunctionCall2(textnlike, Int64GetDatum(val1text), val2));

	PG_RETURN_BOOL(result);
}

/* AGGREGATE */
Datum uint2_larger(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	uint16 val2 = PG_GETARG_UINT32(1);

	PG_RETURN_UINT32(val1 > val2 ? val1 : val2);
}

Datum uint2_smaller(PG_FUNCTION_ARGS)
{
	uint16 val1 = PG_GETARG_UINT32(0);
	uint16 val2 = PG_GETARG_UINT32(1);

	PG_RETURN_UINT32(val1 < val2 ? val1 : val2);
}

Datum uint2_avg_accum(PG_FUNCTION_ARGS)
{
	ArrayType *transarray;
	uint16 newval = 0;
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

	transdata = (Int8TransTypeData *)ARR_DATA_PTR(transarray);
	transdata->count++;
	transdata->sum += newval;

	PG_RETURN_ARRAYTYPE_P(transarray);
}

Datum uint2_sum(PG_FUNCTION_ARGS)
{
	uint16 newval;

	if (PG_ARGISNULL(0))
	{
		/* No non-null input seen so far... */
		if (PG_ARGISNULL(1))
			PG_RETURN_NULL(); /* still no non-null */
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
#ifndef USE_FLOAT8_BYVAL /* controls int8 too */
	if (AggCheckCallContext(fcinfo, NULL))
	{
		int64 *oldsum = (int64 *)PG_GETARG_POINTER(0);

		/* Leave the running sum unchanged in the new input is null */
		if (!PG_ARGISNULL(1))
			*oldsum = *oldsum + PG_GETARG_UINT32(1);

		PG_RETURN_POINTER(oldsum);
	}
	else
#endif
	{
		int64 oldsum = PG_GETARG_INT64(0);

		/* Leave sum unchanged if new input is null. */
		if (PG_ARGISNULL(1))
			PG_RETURN_INT64(oldsum);

		/* OK to do the addition. */
		newval = PG_GETARG_UINT32(1);
		oldsum = oldsum + newval;

		PG_RETURN_INT64(oldsum);
	}
}
/* End - ReqID:SRS-MYSQL-NUMBERICTYPE-uint2 */
