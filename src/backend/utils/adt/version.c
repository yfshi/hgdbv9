/*-------------------------------------------------------------------------
 *
 * version.c
 *	 Returns the PostgreSQL version string
 *
 * Copyright (c) 1998-2021, PostgreSQL Global Development Group
 *
 * IDENTIFICATION
 *
 * src/backend/utils/adt/version.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "utils/builtins.h"
/* Begin - BUG#Z00004967 */
#include "lib/stringinfo.h"
/* End - BUG#Z00004967 */
/* Begin - BUG#Z00005371 */
#include "mb/pg_wchar.h"
/* End - BUG#Z00005371 */



Datum
pgsql_version(PG_FUNCTION_ARGS)
{
	PG_RETURN_TEXT_P(cstring_to_text(PG_VERSION_STR));
}

/* Begin - BUG#Z00004967 */
/*
 * return build_version str
 * like HG Database Server x86_64 V9 0-3-0-1034-20241220-206-f2344abcd-083b4fa P001
 */
Datum
hgdb_build_version(PG_FUNCTION_ARGS)
{
	StringInfoData data;
	text *ret;

	initStringInfo(&data);

	appendStringInfo(&data, "%s %s %s %s-%s-%s-%s-%s",
			gettext(HG_PRODUCT_NAME), HG_BUILD_CPU, HG_MAJOR_VERSION,
			HG_EXTRAL_VERSION, HG_BUILD_DATE, HG_PROJECT_ID,
			HG_BRANCH_HASH, HG_BRANCH_COMMIT_HASH);

	/* append patch str */
	if (HG_PATCH_STR != NULL &&
		strcmp(HG_PATCH_STR, "") != 0)
		appendStringInfo(&data, " %s", HG_PATCH_STR);

	ret = cstring_to_text(data.data);
	pfree(data.data);

	PG_RETURN_TEXT_P(ret);
}
/* End - BUG#Z00004967 */

/*
 * hgdb_version return HG Database Serve V9.0 + build_date
 */
Datum
hgdb_version(PG_FUNCTION_ARGS)
{
	StringInfoData data;
	text *ret;

	initStringInfo(&data);

	appendStringInfo(&data, "%s",
			gettext(HG_PRODUCT_NAME));

	/* English should add a space */
	if (pg_mblen(data.data) == 1)
		appendStringInfoChar(&data, ' ');

	appendStringInfo(&data, "%s %s",
					HG_HGDB_VERSION, HG_BUILD_DATE);

	ret = cstring_to_text(data.data);
	pfree(data.data);

	PG_RETURN_TEXT_P(ret);
}

