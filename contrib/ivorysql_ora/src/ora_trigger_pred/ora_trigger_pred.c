/*-------------------------------------------------------------------------
 *
 * File:ora_trigger_pred.c
 *
 * Abstract:
 *		Implementing the compatible with Oracle triggers predicate functions.
 *
 *
 * ReqID:SRS-SQL-TRIGGER
 *
 * Authored by zhaoshengbin@highgo.com,20251217.
 *
 * Copyright:
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  contrib/ivorysql_ora/src/ora_trigger_pred/ora_trigger_pred.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "fmgr.h"
#include "commands/trigger.h"
#include "catalog/pg_type.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"


/* INSERTING */
PG_FUNCTION_INFO_V1(inserting);
Datum
inserting(PG_FUNCTION_ARGS)
{
	TriggerData *trigdata = peek_triggerdata();

	if (!trigdata)
		PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(TRIGGER_FIRED_BY_INSERT(trigdata->tg_event));
}

/* DELETING */
PG_FUNCTION_INFO_V1(deleting);
Datum
deleting(PG_FUNCTION_ARGS)
{
	TriggerData *trigdata = peek_triggerdata();

	if (!trigdata)
		PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(TRIGGER_FIRED_BY_DELETE(trigdata->tg_event));
}

/* UPDATING */
PG_FUNCTION_INFO_V1(updating);
Datum
updating(PG_FUNCTION_ARGS)
{
	TriggerData *trigdata = peek_triggerdata();

	if (!trigdata)
		PG_RETURN_BOOL(false);

	PG_RETURN_BOOL(TRIGGER_FIRED_BY_UPDATE(trigdata->tg_event));
}

/* UPDATING('column') */
PG_FUNCTION_INFO_V1(updating_col);
Datum
updating_col(PG_FUNCTION_ARGS)
{
	TriggerData *trigdata;
	text 		*colname;
	char 		*col;
	AttrNumber 	attnum;

	if (PG_ARGISNULL(0))
		PG_RETURN_BOOL(false);

	trigdata = peek_triggerdata();
	if (!trigdata)
		PG_RETURN_BOOL(false);

	if (!TRIGGER_FIRED_BY_UPDATE(trigdata->tg_event))
		PG_RETURN_BOOL(false);

	colname = PG_GETARG_TEXT_PP(0);
	col = text_to_cstring(colname);

	attnum = get_attnum(RelationGetRelid(trigdata->tg_relation), col);
	if (attnum == InvalidAttrNumber)
		ereport(ERROR,
				(errmsg("column \"%s\" does not exist", col)));

	PG_RETURN_BOOL(
		bms_is_member(attnum - FirstLowInvalidHeapAttributeNumber,
				trigdata->tg_updatedcols)
	);
}
