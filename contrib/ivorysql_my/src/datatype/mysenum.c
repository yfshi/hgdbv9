/*------------------------------------------------------------------
 * 
 * File: mysenum.c
 *
 * Abstract: 
 * 		Compatible with MySQL's enum data type.
 *
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_my/src/datatype/mysenum.c
 *
 *------------------------------------------------------------------
 */
#include "postgres.h"

#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <float.h>
#include <limits.h>
#include <sys/time.h>

#include "access/hash.h"
#include "access/xact.h"
#include "access/table.h"
#include "catalog/pg_collation.h"
#include "catalog/pg_enum.h"
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
#include "utils/catcache.h"
#include "utils/datetime.h"
#include "utils/formatting.h"
#include "utils/float.h"
#include "utils/fmgroids.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"
#include "utils/numeric.h"
#include "utils/syscache.h"
#include "utils/timestamp.h"
#include "utils/varlena.h"

#include "../include/ivorysql_my.h"

/* Begin - ReqID:SRS-MYSQL-ENUMTYPE */
PG_FUNCTION_INFO_V1(anyenum_eqint2);
PG_FUNCTION_INFO_V1(anyenum_eqint4);
PG_FUNCTION_INFO_V1(anyenum_eqint8);
PG_FUNCTION_INFO_V1(int2_eqanyenum);
PG_FUNCTION_INFO_V1(int4_eqanyenum);
PG_FUNCTION_INFO_V1(int8_eqanyenum);
PG_FUNCTION_INFO_V1(anyenum_eqtext);
PG_FUNCTION_INFO_V1(text_eqanyenum);
PG_FUNCTION_INFO_V1(enum_cmp_text);
PG_FUNCTION_INFO_V1(text_cmp_enum);

PG_FUNCTION_INFO_V1(anyenum_neint2);
PG_FUNCTION_INFO_V1(anyenum_neint4);
PG_FUNCTION_INFO_V1(anyenum_neint8);
PG_FUNCTION_INFO_V1(int2_neanyenum);
PG_FUNCTION_INFO_V1(int4_neanyenum);
PG_FUNCTION_INFO_V1(int8_neanyenum);
PG_FUNCTION_INFO_V1(anyenum_netext);
PG_FUNCTION_INFO_V1(text_neanyenum);

PG_FUNCTION_INFO_V1(anyenum_ltint2);
PG_FUNCTION_INFO_V1(anyenum_ltint4);
PG_FUNCTION_INFO_V1(anyenum_ltint8);
PG_FUNCTION_INFO_V1(int2_ltanyenum);
PG_FUNCTION_INFO_V1(int4_ltanyenum);
PG_FUNCTION_INFO_V1(int8_ltanyenum);
PG_FUNCTION_INFO_V1(anyenum_lttext);
PG_FUNCTION_INFO_V1(text_ltanyenum);

PG_FUNCTION_INFO_V1(anyenum_gtint2);
PG_FUNCTION_INFO_V1(anyenum_gtint4);
PG_FUNCTION_INFO_V1(anyenum_gtint8);
PG_FUNCTION_INFO_V1(int2_gtanyenum);
PG_FUNCTION_INFO_V1(int4_gtanyenum);
PG_FUNCTION_INFO_V1(int8_gtanyenum);
PG_FUNCTION_INFO_V1(anyenum_gttext);
PG_FUNCTION_INFO_V1(text_gtanyenum);

PG_FUNCTION_INFO_V1(anyenum_leint2);
PG_FUNCTION_INFO_V1(anyenum_leint4);
PG_FUNCTION_INFO_V1(anyenum_leint8);
PG_FUNCTION_INFO_V1(int2_leanyenum);
PG_FUNCTION_INFO_V1(int4_leanyenum);
PG_FUNCTION_INFO_V1(int8_leanyenum);
PG_FUNCTION_INFO_V1(anyenum_letext);
PG_FUNCTION_INFO_V1(text_leanyenum);

PG_FUNCTION_INFO_V1(anyenum_geint2);
PG_FUNCTION_INFO_V1(anyenum_geint4);
PG_FUNCTION_INFO_V1(anyenum_geint8);
PG_FUNCTION_INFO_V1(int2_geanyenum);
PG_FUNCTION_INFO_V1(int4_geanyenum);
PG_FUNCTION_INFO_V1(int8_geanyenum);
PG_FUNCTION_INFO_V1(anyenum_getext);
PG_FUNCTION_INFO_V1(text_geanyenum);

PG_FUNCTION_INFO_V1(anyenum_seqanyenum);
PG_FUNCTION_INFO_V1(anyenum_seqint2);
PG_FUNCTION_INFO_V1(anyenum_seqint4);
PG_FUNCTION_INFO_V1(anyenum_seqint8);
PG_FUNCTION_INFO_V1(int2_seqanyenum);
PG_FUNCTION_INFO_V1(int4_seqanyenum);
PG_FUNCTION_INFO_V1(int8_seqanyenum);

PG_FUNCTION_INFO_V1(anyenum_addanyenum);
PG_FUNCTION_INFO_V1(anyenum_addint2);
PG_FUNCTION_INFO_V1(anyenum_addint4);
PG_FUNCTION_INFO_V1(anyenum_addint8);
PG_FUNCTION_INFO_V1(int2_addanyenum);
PG_FUNCTION_INFO_V1(int4_addanyenum);
PG_FUNCTION_INFO_V1(int8_addanyenum);

PG_FUNCTION_INFO_V1(anyenum_negative);

PG_FUNCTION_INFO_V1(anyenum_subanyenum);
PG_FUNCTION_INFO_V1(anyenum_subint2);
PG_FUNCTION_INFO_V1(anyenum_subint4);
PG_FUNCTION_INFO_V1(anyenum_subint8);
PG_FUNCTION_INFO_V1(int2_subanyenum);
PG_FUNCTION_INFO_V1(int4_subanyenum);
PG_FUNCTION_INFO_V1(int8_subanyenum);

PG_FUNCTION_INFO_V1(anyenum_mulanyenum);
PG_FUNCTION_INFO_V1(anyenum_mulint2);
PG_FUNCTION_INFO_V1(anyenum_mulint4);
PG_FUNCTION_INFO_V1(anyenum_mulint8);
PG_FUNCTION_INFO_V1(int2_mulanyenum);
PG_FUNCTION_INFO_V1(int4_mulanyenum);
PG_FUNCTION_INFO_V1(int8_mulanyenum);

PG_FUNCTION_INFO_V1(anyenum_divanyenum);
PG_FUNCTION_INFO_V1(anyenum_divint2);
PG_FUNCTION_INFO_V1(anyenum_divint4);
PG_FUNCTION_INFO_V1(anyenum_divint8);
PG_FUNCTION_INFO_V1(int2_divanyenum);
PG_FUNCTION_INFO_V1(int4_divanyenum);
PG_FUNCTION_INFO_V1(int8_divanyenum);

PG_FUNCTION_INFO_V1(anyenum_exact_divanyenum);
PG_FUNCTION_INFO_V1(anyenum_exact_divint2);
PG_FUNCTION_INFO_V1(anyenum_exact_divint4);
PG_FUNCTION_INFO_V1(anyenum_exact_divint8);
PG_FUNCTION_INFO_V1(int2_exact_divanyenum);
PG_FUNCTION_INFO_V1(int4_exact_divanyenum);
PG_FUNCTION_INFO_V1(int8_exact_divanyenum);

PG_FUNCTION_INFO_V1(anyenum_modanyenum);
PG_FUNCTION_INFO_V1(anyenum_modint2);
PG_FUNCTION_INFO_V1(anyenum_modint4);
PG_FUNCTION_INFO_V1(anyenum_modint8);
PG_FUNCTION_INFO_V1(int2_modanyenum);
PG_FUNCTION_INFO_V1(int4_modanyenum);
PG_FUNCTION_INFO_V1(int8_modanyenum);

PG_FUNCTION_INFO_V1(anyenum_bitandanyenum);
PG_FUNCTION_INFO_V1(anyenum_bitandint2);
PG_FUNCTION_INFO_V1(anyenum_bitandint4);
PG_FUNCTION_INFO_V1(anyenum_bitandint8);
PG_FUNCTION_INFO_V1(int2_bitandanyenum);
PG_FUNCTION_INFO_V1(int4_bitandanyenum);
PG_FUNCTION_INFO_V1(int8_bitandanyenum);

PG_FUNCTION_INFO_V1(anyenum_bitoranyenum);
PG_FUNCTION_INFO_V1(anyenum_bitorint2);
PG_FUNCTION_INFO_V1(anyenum_bitorint4);
PG_FUNCTION_INFO_V1(anyenum_bitorint8);
PG_FUNCTION_INFO_V1(int2_bitoranyenum);
PG_FUNCTION_INFO_V1(int4_bitoranyenum);
PG_FUNCTION_INFO_V1(int8_bitoranyenum);

PG_FUNCTION_INFO_V1(anyenum_negaanyenum);

PG_FUNCTION_INFO_V1(anyenum_bitxoranyenum);
PG_FUNCTION_INFO_V1(anyenum_bitxorint2);
PG_FUNCTION_INFO_V1(anyenum_bitxorint4);
PG_FUNCTION_INFO_V1(anyenum_bitxorint8);
PG_FUNCTION_INFO_V1(int2_bitxoranyenum);
PG_FUNCTION_INFO_V1(int4_bitxoranyenum);
PG_FUNCTION_INFO_V1(int8_bitxoranyenum);

PG_FUNCTION_INFO_V1(anyenum_rightmovanyenum);
PG_FUNCTION_INFO_V1(anyenum_rightmovint2);
PG_FUNCTION_INFO_V1(anyenum_rightmovint4);
PG_FUNCTION_INFO_V1(anyenum_rightmovint8);
PG_FUNCTION_INFO_V1(int2_rightmovanyenum);
PG_FUNCTION_INFO_V1(int4_rightmovanyenum);
PG_FUNCTION_INFO_V1(int8_rightmovanyenum);

PG_FUNCTION_INFO_V1(anyenum_leftmovanyenum);
PG_FUNCTION_INFO_V1(anyenum_leftmovint2);
PG_FUNCTION_INFO_V1(anyenum_leftmovint4);
PG_FUNCTION_INFO_V1(anyenum_leftmovint8);
PG_FUNCTION_INFO_V1(int2_leftmovanyenum);
PG_FUNCTION_INFO_V1(int4_leftmovanyenum);
PG_FUNCTION_INFO_V1(int8_leftmovanyenum);

PG_FUNCTION_INFO_V1(anyenum_bit_count);

PG_FUNCTION_INFO_V1(anyenum_andanyenum);
PG_FUNCTION_INFO_V1(anyenum_andint2);
PG_FUNCTION_INFO_V1(anyenum_andint4);
PG_FUNCTION_INFO_V1(anyenum_andint8);
PG_FUNCTION_INFO_V1(int2_andanyenum);
PG_FUNCTION_INFO_V1(int4_andanyenum);
PG_FUNCTION_INFO_V1(int8_andanyenum);
PG_FUNCTION_INFO_V1(anyenum_andbool);
PG_FUNCTION_INFO_V1(bool_andanyenum);

PG_FUNCTION_INFO_V1(anyenum_oranyenum);
PG_FUNCTION_INFO_V1(anyenum_orint2);
PG_FUNCTION_INFO_V1(anyenum_orint4);
PG_FUNCTION_INFO_V1(anyenum_orint8);
PG_FUNCTION_INFO_V1(int2_oranyenum);
PG_FUNCTION_INFO_V1(int4_oranyenum);
PG_FUNCTION_INFO_V1(int8_oranyenum);
PG_FUNCTION_INFO_V1(anyenum_orbool);
PG_FUNCTION_INFO_V1(bool_oranyenum);

PG_FUNCTION_INFO_V1(anyenum_invertanyenum);

PG_FUNCTION_INFO_V1(anyenum_anyenum_like);
PG_FUNCTION_INFO_V1(anyenum_int8_like);
PG_FUNCTION_INFO_V1(anyenum_text_like);
PG_FUNCTION_INFO_V1(anyenum_anyenum_notlike);
PG_FUNCTION_INFO_V1(anyenum_int8_notlike);
PG_FUNCTION_INFO_V1(anyenum_text_notlike);

PG_FUNCTION_INFO_V1(anyenum_avg_accum);
PG_FUNCTION_INFO_V1(anyenum_avg);
PG_FUNCTION_INFO_V1(anyenum_sum);

PG_FUNCTION_INFO_V1(format_anonymous_enum);
PG_FUNCTION_INFO_V1(check_is_anonymous_enum);

inline static int
ENUM_STRCMP(char *str1, char *str2, Oid coll)
{
	char	   *trimstr1 = NULL;
	char	   *trimstr2 = NULL;
	Datum		trimtxt;

	trimtxt = DirectFunctionCall1(rtrim1, CStringGetTextDatum(str1));
	trimstr1 = TextDatumGetCString(trimtxt);

	trimtxt = DirectFunctionCall1(rtrim1, CStringGetTextDatum(str2));
	trimstr2 = TextDatumGetCString(trimtxt);

	return varstr_cmp(trimstr1, strlen(trimstr1), trimstr2, strlen(trimstr2), coll);
}

Datum
anyenum_eqint2(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int16		val2 = PG_GETARG_INT16(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val1 == val2);
}

Datum
anyenum_eqint4(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int32		val2 = PG_GETARG_INT32(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val1 == val2);
}

Datum
anyenum_eqint8(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int64		val2 = PG_GETARG_INT64(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val1 == val2);
}

Datum
int2_eqanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int16		val2 = PG_GETARG_INT16(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val2 == val1);
}

Datum
int4_eqanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int32		val2 = PG_GETARG_INT32(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val2 == val1);
}

Datum
int8_eqanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int64		val2 = PG_GETARG_INT64(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val2 == val1);
}

Datum
anyenum_eqtext(PG_FUNCTION_ARGS)
{
	Oid			oid = PG_GETARG_OID(0);
	char		*str1 = NULL;
	text	    *tvalue = PG_GETARG_TEXT_P(1);
	char		*str2 = text_to_cstring(tvalue);

	str1 = get_enum_label(oid);

	PG_RETURN_BOOL(ENUM_STRCMP(str1, str2, PG_GET_COLLATION()) == 0);
}

Datum
text_eqanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid = PG_GETARG_OID(1);
	char		*str1 = NULL;
	text	    *tvalue = PG_GETARG_TEXT_P(0);
	char		*str2 = text_to_cstring(tvalue);

	str1 = get_enum_label(oid);
	PG_RETURN_BOOL(ENUM_STRCMP(str2, str1, PG_GET_COLLATION()) == 0);
}

Datum
anyenum_neint2(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int16		val2 = PG_GETARG_INT16(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val1 != val2);
}

Datum
anyenum_neint4(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int32		val2 = PG_GETARG_INT32(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val1 != val2);
}

Datum
anyenum_neint8(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int64		val2 = PG_GETARG_INT64(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val1 != val2);
}

Datum
int2_neanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int16		val2 = PG_GETARG_INT16(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val2 != val1);
}

Datum
int4_neanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int32		val2 = PG_GETARG_INT32(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val2 != val1);
}

Datum
int8_neanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int64		val2 = PG_GETARG_INT64(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val2 != val1);
}

Datum
anyenum_netext(PG_FUNCTION_ARGS)
{
	Oid			oid = PG_GETARG_OID(0);
	char		*str1 = get_enum_label(oid);
	text	    *tvalue = PG_GETARG_TEXT_P(1);
	char		*str2 = text_to_cstring(tvalue);

	PG_RETURN_BOOL(ENUM_STRCMP(str1, str2, PG_GET_COLLATION()) != 0);
}

Datum
text_neanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid = PG_GETARG_OID(1);
	char		*str1 = get_enum_label(oid);
	text	  	*tvalue = PG_GETARG_TEXT_P(0);
	char		*str2 = text_to_cstring(tvalue);

	PG_RETURN_BOOL(ENUM_STRCMP(str2, str1, PG_GET_COLLATION()) != 0);
}

Datum
anyenum_ltint2(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int16		val2 = PG_GETARG_INT16(1);
	float8		val1;

	if (val2 < 0) 
		PG_RETURN_BOOL(false);

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val1 < val2);
}

Datum
anyenum_ltint4(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int32		val2 = PG_GETARG_INT32(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val1 < val2);
}

Datum
anyenum_ltint8(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int64		val2 = PG_GETARG_INT64(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val1 < val2);
}

Datum
int2_ltanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int16		val2 = PG_GETARG_INT16(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val2 < val1);
}

Datum
int4_ltanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int32		val2 = PG_GETARG_INT32(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val2 < val1);
}

Datum
int8_ltanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int64		val2 = PG_GETARG_INT64(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val2 < val1);
}

Datum
anyenum_lttext(PG_FUNCTION_ARGS)
{
	Oid			oid = PG_GETARG_OID(0);
	char		*str1 = get_enum_label(oid);
	text	   	*tvalue = PG_GETARG_TEXT_P(1);
	char		*str2 = text_to_cstring(tvalue);

	PG_RETURN_BOOL(ENUM_STRCMP(str1, str2, PG_GET_COLLATION()) < 0);
}

Datum
text_ltanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid = PG_GETARG_OID(1);
	char		*str1 = get_enum_label(oid);
	text	  	*tvalue = PG_GETARG_TEXT_P(0);
	char		*str2 = text_to_cstring(tvalue);

	PG_RETURN_BOOL(ENUM_STRCMP(str2, str1, PG_GET_COLLATION()) < 0);
}

Datum
anyenum_leint2(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int16		val2 = PG_GETARG_INT16(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
anyenum_leint4(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int32		val2 = PG_GETARG_INT32(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
anyenum_leint8(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int64		val2 = PG_GETARG_INT64(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val1 <= val2);
}

Datum
int2_leanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int16		val2 = PG_GETARG_INT16(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val2 <= val1);
}

Datum
int4_leanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int32		val2 = PG_GETARG_INT32(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val2 <= val1);
}

Datum
int8_leanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int64		val2 = PG_GETARG_INT64(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val2 <= val1);
}

Datum
anyenum_letext(PG_FUNCTION_ARGS)
{
	Oid			oid = PG_GETARG_OID(0);
	char		*str1 = get_enum_label(oid);
	text	  	*tvalue = PG_GETARG_TEXT_P(1);
	char		*str2 = text_to_cstring(tvalue);

	PG_RETURN_BOOL(ENUM_STRCMP(str1, str2, PG_GET_COLLATION()) <= 0);
}

Datum
text_leanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid = PG_GETARG_OID(1);
	char		*str1 = get_enum_label(oid);
	text	  	*tvalue = PG_GETARG_TEXT_P(0);
	char		*str2 = text_to_cstring(tvalue);

	PG_RETURN_BOOL(ENUM_STRCMP(str2, str1, PG_GET_COLLATION()) <= 0);
}

Datum
anyenum_gtint2(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int16		val2 = PG_GETARG_INT16(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val1 > val2);
}

Datum
anyenum_gtint4(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int32		val2 = PG_GETARG_INT32(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val1 > val2);
}

Datum
anyenum_gtint8(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int64		val2 = PG_GETARG_INT64(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val1 > val2);
}

Datum
int2_gtanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int16		val2 = PG_GETARG_INT16(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val2 > val1);
}

Datum
int4_gtanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int32		val2 = PG_GETARG_INT32(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val2 > val1);
}

Datum
int8_gtanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int64		val2 = PG_GETARG_INT64(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val2 > val1);
}

Datum
anyenum_gttext(PG_FUNCTION_ARGS)
{
	Oid			oid = PG_GETARG_OID(0);
	char		*str1 = get_enum_label(oid);
	text	  	*tvalue = PG_GETARG_TEXT_P(1);
	char		*str2 = text_to_cstring(tvalue);
	
	PG_RETURN_BOOL(ENUM_STRCMP(str1, str2, PG_GET_COLLATION()) > 0);
}

Datum
text_gtanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid = PG_GETARG_OID(1);
	char		*str1 = get_enum_label(oid);
	text	  	*tvalue = PG_GETARG_TEXT_P(0);
	char		*str2 = text_to_cstring(tvalue);

	PG_RETURN_BOOL(ENUM_STRCMP(str2, str1, PG_GET_COLLATION()) > 0);
}

Datum
anyenum_geint2(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int16		val2 = PG_GETARG_INT16(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val1 >= val2);
}

Datum
anyenum_geint4(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int32		val2 = PG_GETARG_INT32(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val1 >= val2);
}

Datum
anyenum_geint8(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int64		val2 = PG_GETARG_INT64(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val1 >= val2);
}

Datum
int2_geanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int16		val2 = PG_GETARG_INT16(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val2 >= val1);
}

Datum
int4_geanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int32		val2 = PG_GETARG_INT32(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val2 >= val1);
}

Datum
int8_geanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int64		val2 = PG_GETARG_INT64(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val2 >= val1);
}

Datum
anyenum_getext(PG_FUNCTION_ARGS)
{
	Oid			oid = PG_GETARG_OID(0);
	char		*str1 = get_enum_label(oid);
	text		*tvalue = PG_GETARG_TEXT_P(1);
	char		*str2 = text_to_cstring(tvalue);

	PG_RETURN_BOOL(ENUM_STRCMP(str1, str2, PG_GET_COLLATION()) >= 0);
}

Datum
text_geanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid = PG_GETARG_OID(1);
	char		*str1 = get_enum_label(oid);
	text	 	*tvalue = PG_GETARG_TEXT_P(0);
	char		*str2 = text_to_cstring(tvalue);
	
	PG_RETURN_BOOL(ENUM_STRCMP(str2, str1, PG_GET_COLLATION()) >= 0);
}

Datum
anyenum_seqanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	Oid			oid2 = PG_GETARG_OID(1);

	PG_RETURN_BOOL(compare_two_enum_value(oid1, oid2) == 0);
}

Datum
anyenum_seqint2(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int16		val2 = PG_GETARG_INT16(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val1 == val2);
}

Datum
anyenum_seqint4(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int32		val2 = PG_GETARG_INT32(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val1 == val2);
}

Datum
anyenum_seqint8(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int64		val2 = PG_GETARG_INT64(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val1 == val2);
}

Datum
int2_seqanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int16		val2 = PG_GETARG_INT16(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}
	PG_RETURN_BOOL(val2 == val1);
}

Datum
int4_seqanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int32		val2 = PG_GETARG_INT32(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val2 == val1);
}

Datum
int8_seqanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int64		val2 = PG_GETARG_INT64(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_BOOL(val2 == val1);
}

Datum
anyenum_addanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	Oid			oid2 = PG_GETARG_OID(1);
	float8		val1;
	float8		val2;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	if (oid2 == InvalidOid)
	{
		val2 = 0;
	}
	else
	{
		val2 = get_enum_sortorder(oid2);
	}

	PG_RETURN_DATUM(DirectFunctionCall2(float8pl, Float8GetDatum(val1), Float8GetDatum(val2)));
}

Datum
anyenum_addint2(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int16		val2 = PG_GETARG_INT16(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_DATUM(DirectFunctionCall2(float8pl, Float8GetDatum(val1), Float8GetDatum(val2)));
}

Datum
anyenum_addint4(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int32		val2 = PG_GETARG_INT32(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_FLOAT8(val1+val2);
}

Datum
anyenum_addint8(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int64		val2 = PG_GETARG_INT64(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_DATUM(DirectFunctionCall2(float8pl, Float8GetDatum(val1), Float8GetDatum(val2)));
}

Datum
int2_addanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int16		val2 = PG_GETARG_INT16(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_DATUM(DirectFunctionCall2(float8pl, Float8GetDatum(val1), Float8GetDatum(val2)));
}

Datum
int4_addanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int32		val2 = PG_GETARG_INT32(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_DATUM(DirectFunctionCall2(float8pl, Float8GetDatum(val1), Float8GetDatum(val2)));
}

Datum
int8_addanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int64		val2 = PG_GETARG_INT64(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_DATUM(DirectFunctionCall2(float8pl, Float8GetDatum(val1), Float8GetDatum(val2)));
}

Datum
anyenum_negative(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_FLOAT8(-val1);
}

Datum
anyenum_subanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	Oid			oid2 = PG_GETARG_OID(1);
	float8		val1;
	float8		val2;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	if (oid2 == InvalidOid)
	{
		val2 = 0;
	}
	else
	{
		val2 = get_enum_sortorder(oid2);
	}

	PG_RETURN_DATUM(DirectFunctionCall2(float8mi, Float8GetDatum(val1), Float8GetDatum(val2)));
}

Datum
anyenum_subint2(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int16		val2 = PG_GETARG_INT16(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_DATUM(DirectFunctionCall2(float8mi, Float8GetDatum(val1), Float8GetDatum(val2)));
}

Datum
anyenum_subint4(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int32		val2 = PG_GETARG_INT32(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_DATUM(DirectFunctionCall2(float8mi, Float8GetDatum(val1), Float8GetDatum(val2)));
}

Datum
anyenum_subint8(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int64		val2 = PG_GETARG_INT64(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_DATUM(DirectFunctionCall2(float8mi, Float8GetDatum(val1), Float8GetDatum(val2)));
}

Datum
int2_subanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int16		val2 = PG_GETARG_INT16(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_DATUM(DirectFunctionCall2(float8mi, Float8GetDatum(val2), Float8GetDatum(val1)));
}

Datum
int4_subanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int32		val2 = PG_GETARG_INT32(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_DATUM(DirectFunctionCall2(float8mi, Float8GetDatum(val2), Float8GetDatum(val1)));
}

Datum
int8_subanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int64		val2 = PG_GETARG_INT64(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_DATUM(DirectFunctionCall2(float8mi, Float8GetDatum(val2), Float8GetDatum(val1)));
}

Datum
anyenum_mulanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	Oid			oid2 = PG_GETARG_OID(1);
	float8		val1;
	float8		val2;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	if (oid2 == InvalidOid)
	{
		val2 = 0;
	}
	else
	{
		val2 = get_enum_sortorder(oid2);
	}

	PG_RETURN_DATUM(DirectFunctionCall2(float8mul, Float8GetDatum(val1), Float8GetDatum(val2)));
}

Datum
anyenum_mulint2(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int16		val2 = PG_GETARG_INT16(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_DATUM(DirectFunctionCall2(float8mul, Float8GetDatum(val1), Float8GetDatum(val2)));
}

Datum
anyenum_mulint4(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int32		val2 = PG_GETARG_INT32(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_DATUM(DirectFunctionCall2(float8mul, Float8GetDatum(val1), Float8GetDatum(val2)));
}

Datum
anyenum_mulint8(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int64		val2 = PG_GETARG_INT64(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_DATUM(DirectFunctionCall2(float8mul, Float8GetDatum(val1), Float8GetDatum(val2)));
}

Datum
int2_mulanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int16		val2 = PG_GETARG_INT16(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_DATUM(DirectFunctionCall2(float8mul, Float8GetDatum(val1), Float8GetDatum(val2)));
}

Datum
int4_mulanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int32		val2 = PG_GETARG_INT32(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_DATUM(DirectFunctionCall2(float8mul, Float8GetDatum(val1), Float8GetDatum(val2)));
}

Datum
int8_mulanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int64		val2 = PG_GETARG_INT64(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_DATUM(DirectFunctionCall2(float8mul, Float8GetDatum(val1), Float8GetDatum(val2)));
}

Datum
anyenum_divanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	Oid			oid2 = PG_GETARG_OID(1);
	float8		val1;
	float8		val2;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	if (oid2 == InvalidOid)
	{
		val2 = 0;
	}
	else
	{
		val2 = get_enum_sortorder(oid2);
	}

	MYSQL_CHECK_DIVISION_BY_ZERO(val2);

	PG_RETURN_DATUM(DirectFunctionCall2(float8div, Float8GetDatum(val1), Float8GetDatum(val2)));
}

Datum
anyenum_divint2(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int16		val2 = PG_GETARG_INT16(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	MYSQL_CHECK_DIVISION_BY_ZERO(val2);

	PG_RETURN_DATUM(DirectFunctionCall2(float8div, Float8GetDatum(val1), Float8GetDatum(val2)));
}

Datum
anyenum_divint4(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int32		val2 = PG_GETARG_INT32(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	MYSQL_CHECK_DIVISION_BY_ZERO(val2);

	PG_RETURN_DATUM(DirectFunctionCall2(float8div, Float8GetDatum(val1), Float8GetDatum(val2)));
}

Datum
anyenum_divint8(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int64		val2 = PG_GETARG_INT64(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	MYSQL_CHECK_DIVISION_BY_ZERO(val2);

	PG_RETURN_DATUM(DirectFunctionCall2(float8div, Float8GetDatum(val1), Float8GetDatum(val2)));
}

Datum
int2_divanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int16		val2 = PG_GETARG_INT16(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	MYSQL_CHECK_DIVISION_BY_ZERO(val1);
	
	PG_RETURN_DATUM(DirectFunctionCall2(float8div, Float8GetDatum(val2), Float8GetDatum(val1)));
}

Datum
int4_divanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int32		val2 = PG_GETARG_INT32(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	MYSQL_CHECK_DIVISION_BY_ZERO(val1);

	PG_RETURN_DATUM(DirectFunctionCall2(float8div, Float8GetDatum(val2), Float8GetDatum(val1)));
}

Datum
int8_divanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int64		val2 = PG_GETARG_INT64(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	MYSQL_CHECK_DIVISION_BY_ZERO(val1);

	PG_RETURN_DATUM(DirectFunctionCall2(float8div, Float8GetDatum(val2), Float8GetDatum(val1)));
}

Datum
anyenum_exact_divanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	Oid			oid2 = PG_GETARG_OID(1);
	float8		val1;
	float8		val2;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	if (oid2 == InvalidOid)
	{
		val2 = 0;
	}
	else
	{
		val2 = get_enum_sortorder(oid2);
	}

	MYSQL_CHECK_DIVISION_BY_ZERO(val2);
	
	PG_RETURN_INT32(val1 / val2);
}

Datum
anyenum_exact_divint2(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int16		val2 = PG_GETARG_INT16(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	MYSQL_CHECK_DIVISION_BY_ZERO(val2);

	PG_RETURN_INT32(val1 / val2);
}

Datum
anyenum_exact_divint4(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int32		val2 = PG_GETARG_INT32(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	MYSQL_CHECK_DIVISION_BY_ZERO(val2);

	PG_RETURN_INT32(val1 / val2);
}

Datum
anyenum_exact_divint8(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int64		val2 = PG_GETARG_INT64(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	MYSQL_CHECK_DIVISION_BY_ZERO(val2);

	PG_RETURN_INT32(val1 / val2);
}

Datum
int2_exact_divanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int16		val2 = PG_GETARG_INT16(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	MYSQL_CHECK_DIVISION_BY_ZERO(val1);	

	PG_RETURN_INT32(val2 / val1);
}

Datum
int4_exact_divanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int32		val2 = PG_GETARG_INT32(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	MYSQL_CHECK_DIVISION_BY_ZERO(val1);
	
	PG_RETURN_INT32(val2 / val1);
}

Datum
int8_exact_divanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int64		val2 = PG_GETARG_INT64(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	MYSQL_CHECK_DIVISION_BY_ZERO(val1);
	
	PG_RETURN_INT32(val2 / val1);
}

Datum
anyenum_modanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	Oid			oid2 = PG_GETARG_OID(1);
	float8		val1;
	float8		val2;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	if (oid2 == InvalidOid)
	{
		val2 = 0;
	}
	else
	{
		val2 = get_enum_sortorder(oid2);
	}

	MYSQL_CHECK_DIVISION_BY_ZERO(val2);

	PG_RETURN_FLOAT8(fmod(val1, val2));
}

Datum
anyenum_modint2(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int16		val2 = PG_GETARG_INT16(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	MYSQL_CHECK_DIVISION_BY_ZERO(val2);

	PG_RETURN_FLOAT8(fmod(val1, val2));
}

Datum
anyenum_modint4(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int32		val2 = PG_GETARG_INT32(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	MYSQL_CHECK_DIVISION_BY_ZERO(val2);

	PG_RETURN_FLOAT8(fmod(val1, val2));
}

Datum
anyenum_modint8(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int64		val2 = PG_GETARG_INT64(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	MYSQL_CHECK_DIVISION_BY_ZERO(val2);

	PG_RETURN_FLOAT8(fmod(val1, val2));
}

Datum
int2_modanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int16		val2 = PG_GETARG_INT16(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	MYSQL_CHECK_DIVISION_BY_ZERO(val1);

	PG_RETURN_FLOAT8(fmod(val2, val1));
}

Datum
int4_modanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int32		val2 = PG_GETARG_INT32(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	MYSQL_CHECK_DIVISION_BY_ZERO(val1);

	PG_RETURN_FLOAT8(fmod(val2, val1));
}

Datum
int8_modanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int64		val2 = PG_GETARG_INT64(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	MYSQL_CHECK_DIVISION_BY_ZERO(val1);

	PG_RETURN_FLOAT8(fmod(val2, val1));
}

Datum
anyenum_bitandanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	Oid			oid2 = PG_GETARG_OID(1);
	float8		val1;
	float8		val2;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	if (oid2 == InvalidOid)
	{
		val2 = 0;
	}
	else
	{
		val2 = get_enum_sortorder(oid2);
	}

	PG_RETURN_UINT64((uint64) val1 & (uint64) val2);
}

Datum
anyenum_bitandint2(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int16		val2 = PG_GETARG_INT16(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_UINT64((uint64) val1 & (uint64) val2);
}

Datum
anyenum_bitandint4(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int32		val2 = PG_GETARG_INT32(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_UINT64((uint64) val1 & (uint64) val2);
}

Datum
anyenum_bitandint8(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int64		val2 = PG_GETARG_INT64(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_UINT64((uint64) val1 & (uint64) val2);
}

Datum
int2_bitandanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int16		val2 = PG_GETARG_INT16(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_UINT64((uint64) val1 & (uint64) val2);
}

Datum
int4_bitandanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int32		val2 = PG_GETARG_INT32(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_UINT64((uint64) val1 & (uint64) val2);
}

Datum
int8_bitandanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int64		val2 = PG_GETARG_INT64(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_UINT64((uint64) val1 & (uint64) val2);
}

Datum
anyenum_bitoranyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	Oid			oid2 = PG_GETARG_OID(1);
	float8		val1;
	float8		val2;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	if (oid2 == InvalidOid)
	{
		val2 = 0;
	}
	else
	{
		val2 = get_enum_sortorder(oid2);
	}

	PG_RETURN_UINT64((uint64) val1 | (uint64) val2);
}

Datum
anyenum_bitorint2(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int16		val2 = PG_GETARG_INT16(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_UINT64((uint64) val1 | (uint64) val2);
}

Datum
anyenum_bitorint4(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int32		val2 = PG_GETARG_INT32(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_UINT64((uint64) val1 | (uint64) val2);
}

Datum
anyenum_bitorint8(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int64		val2 = PG_GETARG_INT64(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_UINT64((uint64) val1 | (uint64) val2);
}

Datum
int2_bitoranyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int16		val2 = PG_GETARG_INT16(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_UINT64((uint64) val1 | (uint64) val2);
}

Datum
int4_bitoranyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int32		val2 = PG_GETARG_INT32(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_UINT64((uint64) val1 | (uint64) val2);
}

Datum
int8_bitoranyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int64		val2 = PG_GETARG_INT64(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_UINT64((uint64) val1 | (uint64) val2);
}

Datum
anyenum_negaanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	uint64		res;
	float8		val1;

	if (PG_ARGISNULL(0))
	{
		PG_RETURN_NULL();
	}

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	res = ~(uint64) val1;
	PG_RETURN_UINT64(res);
}

Datum
anyenum_bitxoranyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	Oid			oid2 = PG_GETARG_OID(1);
	float8		val1;
	float8		val2;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	if (oid2 == InvalidOid)
	{
		val2 = 0;
	}
	else
	{
		val2 = get_enum_sortorder(oid2);
	}

	PG_RETURN_UINT64((uint64) val1 ^ (uint64) val2);
}

Datum
anyenum_bitxorint2(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int16		val2 = PG_GETARG_INT16(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_UINT64((uint64) val1 ^ (uint64) val2);
}

Datum
anyenum_bitxorint4(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int32		val2 = PG_GETARG_INT32(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_UINT64((uint64) val1 ^ (uint64) val2);
}

Datum
anyenum_bitxorint8(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int64		val2 = PG_GETARG_INT64(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_UINT64((uint64) val1 ^ (uint64) val2);
}

Datum
int2_bitxoranyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int16		val2 = PG_GETARG_INT16(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_UINT64((uint64) val1 ^ (uint64) val2);
}

Datum
int4_bitxoranyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int32		val2 = PG_GETARG_INT32(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_UINT64((uint64) val1 ^ (uint64) val2);
}

Datum
int8_bitxoranyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int64		val2 = PG_GETARG_INT64(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_UINT64((uint64) val1 ^ (uint64) val2);
}

Datum
anyenum_rightmovanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	Oid			oid2 = PG_GETARG_OID(1);
	float8		val1;
	float8		val2;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	if (oid2 == InvalidOid)
	{
		val2 = 0;
	}
	else
	{
		val2 = get_enum_sortorder(oid2);
	}

	/* the bit shift exceeds the 8 byte value */
	if (val2 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}

	PG_RETURN_UINT64((uint64) val1 >> (uint64) val2);
}

Datum
anyenum_rightmovint2(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int16		val2 = PG_GETARG_INT16(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	/* the bit shift exceeds the 8 byte value */
	if (val2 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}

	PG_RETURN_UINT64((uint64) val1 >> (uint64) val2);
}

Datum
anyenum_rightmovint4(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int32		val2 = PG_GETARG_INT32(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	/* the bit shift exceeds the 8 byte value */
	if (val2 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}

	PG_RETURN_UINT64((uint64) val1 >> (uint64) val2);
}

Datum
anyenum_rightmovint8(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int64		val2 = PG_GETARG_INT64(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	/* the bit shift exceeds the 8 byte value */
	if (val2 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}

	PG_RETURN_UINT64((uint64) val1 >> (uint64) val2);
}

Datum
int2_rightmovanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int16		val2 = PG_GETARG_INT16(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	if (val1 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}

	PG_RETURN_UINT64((uint64) val2 >> (uint64) val1);
}

Datum
int4_rightmovanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int32		val2 = PG_GETARG_INT32(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	if (val1 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}

	PG_RETURN_UINT64((uint64) val2 >> (uint64) val1);
}

Datum
int8_rightmovanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int64		val2 = PG_GETARG_INT64(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	if (val1 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}

	PG_RETURN_UINT64((uint64) val2 >> (uint64) val1);
}

Datum
anyenum_leftmovanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	Oid			oid2 = PG_GETARG_OID(1);
	float8		val1;
	float8		val2;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	if (oid2 == InvalidOid)
	{
		val2 = 0;
	}
	else
	{
		val2 = get_enum_sortorder(oid2);
	}

	/* the bit shift exceeds the 8 byte value */
	if (val2 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}

	PG_RETURN_UINT64((uint64) val1 << (uint64) val2);
}

Datum
anyenum_leftmovint2(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int16		val2 = PG_GETARG_INT16(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	/* the bit shift exceeds the 8 byte value */
	if (val2 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}

	PG_RETURN_UINT64((uint64) val1 << (uint64) val2);
}

Datum
anyenum_leftmovint4(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int32		val2 = PG_GETARG_INT32(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}
	/* the bit shift exceeds the 8 byte value */
	if (val2 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}

	PG_RETURN_UINT64((uint64) val1 << (uint64) val2);
}

Datum
anyenum_leftmovint8(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int64		val2 = PG_GETARG_INT64(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	/* the bit shift exceeds the 8 byte value */
	if (val2 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}

	PG_RETURN_UINT64((uint64) val1 << (uint64) val2);
}

Datum
int2_leftmovanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int16		val2 = PG_GETARG_INT16(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	if (val1 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}
	
	PG_RETURN_UINT64((uint64) val2 << (uint64) val1);
}

Datum
int4_leftmovanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int32		val2 = PG_GETARG_INT32(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	if (val1 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64((uint64) val2 << (uint64) val1);
}

Datum
int8_leftmovanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int64		val2 = PG_GETARG_INT64(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	if (val1 > sizeof(int64) * 8)
	{
		PG_RETURN_UINT64(0);
	}
	PG_RETURN_UINT64((uint64) val2 << (uint64) val1);
}

Datum
anyenum_bit_count(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	uint64		count = 0;
	float8		val1;
	int32		res;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	res = (int32) val1;
	while (res)
	{
		count += res & 1;
		res >>= 1;
	}
	PG_RETURN_UINT64(count);
}

Datum
anyenum_andanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	Oid			oid2 = PG_GETARG_OID(1);
	float8		val1;
	float8		val2;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	if (oid2 == InvalidOid)
	{
		val2 = 0;
	}
	else
	{
		val2 = get_enum_sortorder(oid2);
	}

	PG_RETURN_INT32((uint64) val1 && (uint64) val2);
}

Datum
anyenum_andint2(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int16		val2 = PG_GETARG_INT16(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_INT32((uint64) val1 && (uint64) val2);
}

Datum
anyenum_andint4(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int32		val2 = PG_GETARG_INT32(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_INT32((uint64) val1 && (uint64) val2);
}

Datum
anyenum_andint8(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int64		val2 = PG_GETARG_INT64(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_INT32((uint64) val1 && (uint64) val2);
}

Datum
int2_andanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int16		val2 = PG_GETARG_INT16(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_INT32((uint64) val1 && (uint64) val2);
}

Datum
int4_andanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int32		val2 = PG_GETARG_INT32(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_INT32((uint64) val1 && (uint64) val2);
}

Datum
int8_andanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int64		val2 = PG_GETARG_INT64(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_INT32((uint64) val1 && (uint64) val2);
}

Datum
anyenum_andbool(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	bool		val2 = PG_GETARG_BOOL(1);
	bool		val1;

	if (oid1 == InvalidOid)
	{
		val1 = false;
	}
	else
	{
		val1 = true;
	}

	PG_RETURN_BOOL(val1 && val2);
}

Datum
bool_andanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	bool		val2 = PG_GETARG_BOOL(0);
	bool		val1;

	if (oid1 == InvalidOid)
	{
		val1 = false;
	}
	else
	{
		val1 = true;
	}

	PG_RETURN_BOOL(val1 && val2);
}

Datum
anyenum_oranyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	uint64		val2 = PG_GETARG_INT64(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_INT32((uint64) val1 || (uint64) val2);
}

Datum
anyenum_orint2(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int16		val2 = PG_GETARG_INT16(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_INT32((uint64) val1 || (uint64) val2);
}

Datum
anyenum_orint4(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int32		val2 = PG_GETARG_INT32(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_INT32((uint64) val1 || (uint64) val2);
}

Datum
anyenum_orint8(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	int64		val2 = PG_GETARG_INT64(1);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_INT32((uint64) val1 || (uint64) val2);
}

Datum
int2_oranyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int16		val2 = PG_GETARG_INT16(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_INT32((uint64) val1 || (uint64) val2);
}

Datum
int4_oranyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int32		val2 = PG_GETARG_INT32(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_INT32((uint64) val1 || (uint64) val2);
}

Datum
int8_oranyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	int64		val2 = PG_GETARG_INT64(0);
	float8		val1;

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_INT32((uint64) val1 || (uint64) val2);
}

Datum
anyenum_orbool(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	bool		val2 = PG_GETARG_BOOL(1);
	bool		val1;

	if (oid1 == InvalidOid)
	{
		val1 = false;
	}
	else
	{
		val1 = true;
	}

	PG_RETURN_BOOL(val1 || val2);
}

Datum
bool_oranyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(1);
	bool		val2 = PG_GETARG_BOOL(0);
	bool		val1;

	if (oid1 == InvalidOid)
	{
		val1 = false;
	}
	else
	{
		val1 = true;
	}

	PG_RETURN_BOOL(val1 || val2);
}

Datum
anyenum_invertanyenum(PG_FUNCTION_ARGS)
{
	Oid			oid1 = PG_GETARG_OID(0);
	float8		val1;

	if (PG_ARGISNULL(0))
	{
		PG_RETURN_NULL();
	}

	if (oid1 == InvalidOid)
	{
		val1 = 0;
	}
	else
	{
		val1 = get_enum_sortorder(oid1);
	}

	PG_RETURN_INT32(!((int32) val1));
}

/* enum like enum : comparison value */
Datum
anyenum_anyenum_like(PG_FUNCTION_ARGS)
{
	Oid			val1 = PG_GETARG_OID(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 == val2);
}

/* enum like int : return false */
Datum
anyenum_int8_like(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(false);
}

/* enum like text : comparison strings(use textlike) */
Datum
anyenum_text_like(PG_FUNCTION_ARGS)
{
	Oid			oid = PG_GETARG_OID(0);
	char		*str1 = get_enum_label(oid);
	Datum 		val2 = PG_GETARG_DATUM(1);
	bool		result = false;

	result = DatumGetBool(DirectFunctionCall2(textlike, 
											  CStringGetTextDatum(str1),
											  val2));

	pfree(str1);
	PG_RETURN_BOOL(result);
}

Datum
anyenum_anyenum_notlike(PG_FUNCTION_ARGS)
{
	Oid			val1 = PG_GETARG_OID(0);
	uint64		val2 = PG_GETARG_INT64(1);

	PG_RETURN_BOOL(val1 != val2);
}

Datum
anyenum_int8_notlike(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(true);
}

Datum
anyenum_text_notlike(PG_FUNCTION_ARGS)
{
	Oid			oid = PG_GETARG_OID(0);
	char		*str1 = get_enum_label(oid);
	Datum 		val2 = PG_GETARG_DATUM(1);
	bool		result = false;

	result = DatumGetBool(DirectFunctionCall2(textnlike, 
											  CStringGetTextDatum(str1), 
											  val2));

	pfree(str1);
	PG_RETURN_BOOL(result);
}

Datum
anyenum_avg_accum(PG_FUNCTION_ARGS)
{
	ArrayType  *transarray = PG_GETARG_ARRAYTYPE_P(0);
	Oid			oid = PG_GETARG_OID(1);
	float8		newval = 0.0;

	float8	   *transvalues;
	float8		N,
				Sx,
				Sxx,
				tmp;

	if (oid == InvalidOid)
	{
		PG_RETURN_ARRAYTYPE_P(transarray);
	}
	newval = get_enum_sortorder(oid);

	transvalues = (float8 *) ARR_DATA_PTR(transarray);
	N = transvalues[0];
	Sx = transvalues[1];
	Sxx = transvalues[2];

	/*
	 * Use the Youngs-Cramer algorithm to incorporate the new value into the
	 * transition values.
	 */
	N += 1.0;
	Sx += newval;

	if (transvalues[0] > 0.0)
	{
		tmp = newval * N - Sx;
		Sxx += tmp * tmp / (N * transvalues[0]);

		/*
		 * Overflow check.  We only report an overflow error when finite
		 * inputs lead to infinite results.  Note also that Sxx should be NaN
		 * if any of the inputs are infinite, so we intentionally prevent Sxx
		 * from becoming infinite.
		 */
		if (isinf(Sx) || isinf(Sxx))
		{
			if (!isinf(transvalues[1]) && !isinf(newval))
				float_overflow_error();

			Sxx = get_float8_nan();
		}
	}
	else
	{
		/*
		 * At the first input, we normally can leave Sxx as 0.  However, if
		 * the first input is Inf or NaN, we'd better force Sxx to NaN;
		 * otherwise we will falsely report variance zero when there are no
		 * more inputs.
		 */
		if (isnan(newval) || isinf(newval))
			Sxx = get_float8_nan();
	}

	/*
	 * If we're invoked as an aggregate, we can cheat and modify our first
	 * parameter in-place to reduce palloc overhead. Otherwise we construct a
	 * new array with the updated transition data and return it.
	 */
	if (AggCheckCallContext(fcinfo, NULL))
	{
		transvalues[0] = N;
		transvalues[1] = Sx;
		transvalues[2] = Sxx;

		PG_RETURN_ARRAYTYPE_P(transarray);
	}
	else
	{
		Datum		transdatums[3];
		ArrayType  *result;

		transdatums[0] = Float8GetDatumFast(N);
		transdatums[1] = Float8GetDatumFast(Sx);
		transdatums[2] = Float8GetDatumFast(Sxx);

		result = construct_array(transdatums, 3,
								 FLOAT8OID,
								 sizeof(float8), FLOAT8PASSBYVAL, TYPALIGN_DOUBLE);

		PG_RETURN_ARRAYTYPE_P(result);
	}
}

Datum
anyenum_avg(PG_FUNCTION_ARGS)
{
	ArrayType  *transarray = PG_GETARG_ARRAYTYPE_P(0);
	float8	   *transvalues;
	float8		N,
				Sx;

	transvalues = (float8 *) ARR_DATA_PTR(transarray);
	N = transvalues[0];
	Sx = transvalues[1];
	/* ignore Sxx */

	/* SQL defines AVG of no values to be NULL */
	if (N == 0.0)
		PG_RETURN_NULL();

	PG_RETURN_FLOAT8(Sx / N);
}

Datum
anyenum_sum(PG_FUNCTION_ARGS)
{
	float8		newval;
	Oid			oid1;
	float8		oldsum;

	if (PG_ARGISNULL(0))
	{
		/* No non-null input seen so far... */
		if (PG_ARGISNULL(1))
			PG_RETURN_NULL();	/* still no non-null */

		/* This is the first non-null input. */
		oid1 = PG_GETARG_OID(1);
		newval = get_enum_sortorder(oid1);
		PG_RETURN_FLOAT8(newval);
	}

	oldsum = PG_GETARG_FLOAT8(0);

	/* Leave sum unchanged if new input is null. */
	if (PG_ARGISNULL(1))
		PG_RETURN_FLOAT8(oldsum);

	oid1 = PG_GETARG_OID(1);
	newval = get_enum_sortorder(oid1);

	/* OK to do the addition. */
	newval = oldsum + newval;

	PG_RETURN_FLOAT8(newval);
}

Datum
format_anonymous_enum(PG_FUNCTION_ARGS)
{
	Oid 		typoid = PG_GETARG_OID(0);
	int			nelems;
	CatCList   	*list;
	StringInfoData 	buf;
	HeapTuple  *existing;
	int			i;

	if (!type_is_enum(typoid) || !type_is_anonymous_enum(typoid))
		PG_RETURN_NULL();

	list = SearchSysCacheList1(ENUMTYPOIDNAME, ObjectIdGetDatum(typoid));
	nelems = list->n_members;
	/* Sort the existing members by enumsortorder */
	existing = (HeapTuple *) palloc(nelems * sizeof(HeapTuple));
	for (i = 0; i < nelems; i++)
		existing[i] = &(list->members[i]->tuple);

	qsort(existing, nelems, sizeof(HeapTuple), sort_order_cmp);

	initStringInfo(&buf);

	appendStringInfo(&buf, "enum(");

	for (i = 0; i < nelems; i++)
	{
		HeapTuple	exists_tup = existing[i];
		Form_pg_enum exists_en = (Form_pg_enum) GETSTRUCT(exists_tup);
		char 			*enumlabel = NameStr(exists_en->enumlabel);
		
		appendStringInfo(&buf, "\'%s\'", enumlabel);
		
		if (i != nelems - 1)
			appendStringInfo(&buf, ", ");	
	}

	appendStringInfo(&buf, ")");

	pfree(existing);
	ReleaseCatCacheList(list);

	PG_RETURN_TEXT_P(CStringGetTextDatum(buf.data));
}

Datum
check_is_anonymous_enum(PG_FUNCTION_ARGS)
{
	Oid 		typoid = PG_GETARG_OID(0);

	if (type_is_anonymous_enum(typoid))
		PG_RETURN_BOOL(true);
	PG_RETURN_BOOL(false);
}

Datum
enum_cmp_text(PG_FUNCTION_ARGS)
{
	Oid 		a = PG_GETARG_OID(0);
	Oid			b = InvalidOid;
	char		*label = text_to_cstring(PG_GETARG_TEXT_PP(1));
	Oid 		enumtypid = InvalidOid;

	enumtypid = get_enum_enumtypid(a);

	b = get_enum_oid_by_label(enumtypid, label, PG_GET_COLLATION());

	if (!OidIsValid(b))
		return -1;

	return enum_cmp_internal(a, b, fcinfo);
}
