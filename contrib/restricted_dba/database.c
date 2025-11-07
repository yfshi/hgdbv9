/* -------------------------------------------------------------------------
 *
 * File: database.c
 *
 * Abstract: 
 * 		This file contains database command permission check.
 * 		if any function in src/backend/commands/database.c changed, you
 * 		better also keep change at here.
 *
 * Authored by sunchengxi@highgo.com, 20240510.
 *
 * Copyright:
 * 		Copyright (c) 2024, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/restricted_dba/database.c
 * -------------------------------------------------------------------------
 */

#include "postgres.h"

#include "catalog/dependency.h"
#include "catalog/pg_database.h"
#include "catalog/pg_db_role_setting.h"
#include "commands/dbcommands.h"
#include "commands/defrem.h"
#include "miscadmin.h"
#include "storage/lmgr.h"
#include "utils/acl.h"

#include "restricted_dba.h"


bool
create_db_check(ParseState *pstate, const CreatedbStmt *stmt)
{
	Oid			datdba;
	ListCell   *option;
	DefElem    *downer = NULL;
	char	   *dbowner = NULL;


	/* Extract options from the statement node tree */
	foreach(option, stmt->options)
	{
		DefElem    *defel = (DefElem *) lfirst(option);

		if (strcmp(defel->defname, "owner") == 0)
		{
			if (downer)
			{
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options"),
						 parser_errposition(pstate, defel->location)));
			}
			downer = defel;
		}		
	}

	if (downer && downer->arg)
		dbowner = defGetString(downer);

	/* obtain OID of proposed owner */
	if (dbowner)
		datdba = get_role_oid(dbowner, false);
	else
		datdba = GetUserId();

	/*only member of sysdba_role can create database or be owner of database*/
	if(!IS_SYSDBA(GetUserId()))
	{
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("permission denied to create database")));
	}

	if(!IS_SYSDBA(datdba))
	{
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("only member of sysdba_role can own a database")));
	}

	return true;
}

/*
 * Same as TableSpace alter database and drop database can only be executed by its
 * onwner, so we do nothing here. leave them to standard function. alter database
 * owner/rename check is in alter.c
 */



/*
 * ALTER DATABASE name SET ...
 */
bool
alter_database_set_check(AlterDatabaseSetStmt *stmt)
{
	Oid			datid;

	/*Begin -- bug#z5555*/
	if (stmt->setstmt->name == NULL)
		return true;
	/*End -- bug#z5555*/

	if (pg_strncasecmp(stmt->setstmt->name,"se_feature.",strlen("se_feature.")) == 0 ||
		pg_strncasecmp(stmt->setstmt->name,"credcheck.",strlen("credcheck.")) == 0)
	{
		/*Begin -- bug#z4455*/
		datid = get_database_oid(stmt->dbname, false);
		/*Begin -- bug#z4455*/

		/*Begin -- bug#z4705*/
		if (!IS_SYSSSO(GetUserId()))	
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					errmsg("permission denied")));
		}
		/*End -- bug#z4705*/	
		
		/*
		* Obtain a lock on the database and make sure it didn't go away in the
		* meantime.
		*/
		shdepLockAndCheckObject(DatabaseRelationId, datid);

		AlterSetting(datid, InvalidOid, stmt->setstmt);

		UnlockSharedObject(DatabaseRelationId, datid, 0, AccessShareLock);
	
		return false;
	}

	return true;
}