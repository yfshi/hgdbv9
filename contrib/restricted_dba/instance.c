/* -------------------------------------------------------------------------
 *
 * File: instance.c
 *
 * Abstract: 
 * 		This file contains functions cluster-level operation.
 *
 * Authored by sunchengxi@highgo.com, 20240510.
 *
 * Copyright:
 * 		Copyright (c) 2024, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/restricted_dba/instance.c
 * -------------------------------------------------------------------------
 */

#include "postgres.h"

#include "catalog/pg_authid.h"
#include "parser/parse_node.h"
#include "miscadmin.h"
#include "utils/guc.h"
#include "utils/elog.h"

#include "restricted_dba.h"

/*
* Execute ALTER SYSTEM statement.
*
*/
bool
alter_system_set_check(AlterSystemStmt *altersysstmt)
{
	char	   *name;
	bool		call_next = true;

	/*
	* Extract statement arguments
	*/
	name = altersysstmt->setstmt->name;

	/*Begin -- bug#z5555*/
	if (name == NULL)
		return true;
	/*End -- bug#z5555*/
	/*
	* if is a audit parameter and the user is syssao, or 
	* is a se_feature parameter and the user is syssso, then allow the change
	*/
	if ((pg_strncasecmp(name,"audit.",strlen("audit.")) == 0 && IS_SYSSAO(GetUserId())) ||
		(pg_strncasecmp(name,"credcheck.",strlen("credcheck.")) == 0 && IS_SYSSSO(GetUserId())) ||	/* Bug#z4191*/
		(pg_strncasecmp(name,"se_feature.",strlen("se_feature.")) == 0 && IS_SYSSSO(GetUserId())) ||	/* Bug#z1456 */
		(pg_strncasecmp(name,"hg_sysmac.",strlen("hg_sysmac.")) == 0 && IS_SYSSSO(GetUserId())) ||
		(pg_strncasecmp(name,"hg_wal_integrity.",strlen("hg_wal_integrity.")) == 0 && IS_SYSSSO(GetUserId())) ||
		(pg_strncasecmp(name,"conn_restrict.enable",strlen("conn_restrict.enable")) == 0 && IS_SYSSSO(GetUserId())) ||	/* Bug #6238 */
		(pg_strcasecmp(name,"count_idle_busy_time") == 0 && IS_SYSSSO(GetUserId())) ||	/* Bug #6183 */
		(pg_strcasecmp(name,"idle_session_timeout") == 0 && IS_SYSSSO(GetUserId())) ||
		(pg_strcasecmp(name,"hg_session_locking_timeout") == 0 && IS_SYSSSO(GetUserId()))) /* Bug #6046*/
	{

		PG_TRY();
		{
			// Set as superuser 
			tmp_superuser_oid = GetUserId();

			/*do the actural work*/
			AlterSystemSetConfigFile((AlterSystemStmt *) altersysstmt);
		}
		PG_CATCH();
		{
			// Restore the original user status on error
			tmp_superuser_oid = InvalidOid;

			// Re-throw the error
			PG_RE_THROW();
		}
		PG_END_TRY();
		// bug# z4206 --begin
		tmp_superuser_oid = InvalidOid;
		// bug# z4206 --end

		call_next = false;
	}

	/*Begin -- bug#z4705*/
	if (((pg_strncasecmp(name,"audit.",strlen("audit.")) == 0) ||
		(pg_strncasecmp(name,"credcheck.",strlen("credcheck.")) == 0 ) ||
		(pg_strncasecmp(name,"hg_sysmac.",strlen("hg_sysmac.")) == 0 )||
		(pg_strncasecmp(name,"hg_wal_integrity.",strlen("hg_wal_integrity.")) == 0 )||
		(pg_strncasecmp(name,"conn_restrict.enable",strlen("conn_restrict.enable")) == 0 )||	/* Bug #6238 */
		(pg_strncasecmp(name,"se_feature.",strlen("se_feature.")) == 0 )) && IS_SYSDBA(GetUserId()))	
	{
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("permission denied")));
	}
	/*End -- bug#z4705*/

	return call_next;
}