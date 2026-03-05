/*-------------------------------------------------------------------------------
 * 
 * File: misc_functions.c
 *
 * Abstract: 
 * 		This file contains the implementation of Oracle's
 *		datatype-independent built-in functions.
 *
 * Authored by zhenmingyang@highgo.com, 20220526.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_ora/src/builtin_functions/misc_functions.c
 *
 *-------------------------------------------------------------------------------
 */

#include "postgres.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "utils/formatting.h"
#include "utils/numeric.h"
/* Begin - Bug#Z1064 */
#include "utils/builtins.h"
#include "utils/ruleutils.h"	/* ReqID:RSR-SQL-Partition-interval */
#include "access/transam.h"
/* End - Bug#Z1064 */
#include "lib/stringinfo.h" /* BUG#M0000457 */


PG_FUNCTION_INFO_V1(uid);	/* bug0000459 */
PG_FUNCTION_INFO_V1(get_macro_value); /* Bug#Z1064 */
/* Begin - BUG#M0000457 */
PG_FUNCTION_INFO_V1(orafce_listagg1_transfn);
PG_FUNCTION_INFO_V1(orafce_listagg2_transfn);
PG_FUNCTION_INFO_V1(orafce_listagg_finalfn);
/* End - BUG#M0000457 */
PG_FUNCTION_INFO_V1(pg_get_partinterval_def);	/* ReqID:RSR-SQL-Partition-interval */
/* Begin - OA-REQ-2129 */
PG_FUNCTION_INFO_V1(wm_concat_transfn);
/* End - OA-REQ-2129 */

/* Begin - bug0000459 */
Datum
uid(PG_FUNCTION_ARGS)
{
	PG_RETURN_UINT32(GetUserId());
}
/* End - bug0000459 */

/* Begin - BUG#M0000457 */
/****************************************************************
 * listagg
 *
 * Concates values and returns string.
 *
 * Syntax:
 *     FUNCTION listagg(string varchar, delimiter varchar = '')
 *      RETURNS varchar;
 *
 * Note: any NULL value is ignored.
 *
 ****************************************************************/
/* subroutine to initialize state */
static StringInfo
makeStringAggState(FunctionCallInfo fcinfo)
{
	StringInfo	state;
	MemoryContext aggcontext;
	MemoryContext oldcontext;

	if (!AggCheckCallContext(fcinfo, &aggcontext))
	{
		/* cannot be called directly because of internal-type argument */
		elog(ERROR, "listagg_transfn called in non-aggregate context");
	}

	/*
	 * Create state in aggregate context.  It'll stay there across subsequent
	 * calls.
	 */
	oldcontext = MemoryContextSwitchTo(aggcontext);
	state = makeStringInfo();
	MemoryContextSwitchTo(oldcontext);

	return state;
}

static void
appendStringInfoText(StringInfo str, const text *t)
{
	appendBinaryStringInfo(str, VARDATA_ANY(t), VARSIZE_ANY_EXHDR(t));
}

Datum
orafce_listagg1_transfn(PG_FUNCTION_ARGS)
{
	StringInfo	state;

	state = PG_ARGISNULL(0) ? NULL : (StringInfo) PG_GETARG_POINTER(0);

	/* Append the element unless null. */
	if (!PG_ARGISNULL(1))
	{
		if (state == NULL)
			state = makeStringAggState(fcinfo);
		appendStringInfoText(state, PG_GETARG_TEXT_PP(1));		/* value */
	}

	/*
	 * The transition type for string_agg() is declared to be "internal",
	 * which is a pass-by-value type the same size as a pointer.
	 */
	PG_RETURN_POINTER(state);
}

Datum
orafce_listagg2_transfn(PG_FUNCTION_ARGS)
{
	return string_agg_transfn(fcinfo);
}

Datum
orafce_listagg_finalfn(PG_FUNCTION_ARGS)
{
	return string_agg_finalfn(fcinfo);
}
/* End - BUG#M0000457 */

/*
 * ReqID:RSR-SQL-Partition-interval
 *
 * pg_get_partinterval_def - get define of partition interval.
 */
Datum
pg_get_partinterval_def(PG_FUNCTION_ARGS)
{
	Oid		relid = PG_GETARG_OID(0);
	char   *res;

	res = pg_get_partinterval_def_string(relid);

	if (res == NULL)
		PG_RETURN_NULL();

	PG_RETURN_TEXT_P(CStringGetTextDatum(res));
}

/* Begin - OA-REQ-2129 */
Datum
wm_concat_transfn(PG_FUNCTION_ARGS)
{
	StringInfo	state;

	state = PG_ARGISNULL(0) ? NULL : (StringInfo) PG_GETARG_POINTER(0);

	/* Append the value unless null. */
	if (!PG_ARGISNULL(1))
	{
		if (state == NULL)
			state = makeStringAggState(fcinfo);
		else
			appendStringInfoText(state, cstring_to_text(","));	/* delimiter */

		appendStringInfoText(state, PG_GETARG_TEXT_PP(1));	/* value */
	}

	/*
	 * The transition type for string_agg() is declared to be "internal",
	 * which is a pass-by-value type the same size as a pointer.
	 */
	PG_RETURN_POINTER(state);
}
/* End - OA-REQ-2129 */
