/*-------------------------------------------------------------------------
 *
 * mys_information_schema.c
 *	  Implenmentation of the MySQL SHOW functionality
 *
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 *
 * IDENTIFICATION
 *	  contrib/ivorysql_my/src/information_schema/mys_information_schema.c
 *
 * add the file for requirement fix bug
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"
#include "lib/stringinfo.h"
#include "utils/builtins.h"
#include "catalog/pg_index.h"
#include "access/htup_details.h"

/* Begin - BUG#Z5974 */
PG_FUNCTION_INFO_V1(mys_get_gistindex_column);
/* End - BUG#Z5974 */

/* Begin - BUG#Z5974 */
/*
 * mys_get_gistindex_column:
 *   Get the column name of the gist index.
 *
 * Returns:
 *   The column name under the effect of the gist index.
 */
Datum
mys_get_gistindex_column(PG_FUNCTION_ARGS)
{
	Oid				indexrelid = PG_GETARG_OID(0);
	Oid				indrelid;
	Form_pg_index	idxrec;
	HeapTuple		ht_idx;
	StringInfoData	buf;
	Datum			predDatum;
	char		   *predString;
	bool			isnull;
	List		   *indexPreds = NIL;

	ht_idx = SearchSysCache1(INDEXRELID, ObjectIdGetDatum(indexrelid));
	if (!HeapTupleIsValid(ht_idx))
	{
		PG_RETURN_NULL();
	}

	idxrec = (Form_pg_index) GETSTRUCT(ht_idx);
	indrelid = idxrec->indrelid;

	initStringInfo(&buf);

	predDatum = SysCacheGetAttr(INDEXRELID, ht_idx,
								Anum_pg_index_indexprs, &isnull);
	if (!isnull)
	{
		ListCell	   *cell;
		int				attnum = 0;

		predString = TextDatumGetCString(predDatum);
		indexPreds = (List *) stringToNode(predString);
		pfree(predString);

		if (list_length(indexPreds) == 1 && IsA(linitial(indexPreds), FuncExpr))
		{
			FuncExpr *fexpr = (FuncExpr *)linitial(indexPreds);

			foreach(cell, fexpr->args)
			{
				char	   *attname;

				if (IsA(lfirst(cell), Var))
				{
					Var *var = (Var *)lfirst(cell);

					attname = get_attname(indrelid, var->varattno, false);
					if (attnum > 0)
					{
						appendStringInfoString(&buf, ", ");
					}
					appendStringInfo(&buf, "%s", attname);
					attnum++;
				}
			}
		}
	}

	ReleaseSysCache(ht_idx);

	PG_RETURN_TEXT_P(cstring_to_text(buf.data));
}
/* End - BUG#Z5974 */
