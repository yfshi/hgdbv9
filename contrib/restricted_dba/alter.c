/* -------------------------------------------------------------------------
 *
 * File: alter.c
 *
 * Abstract: 
 * 		This file contains alter command permission check.
 * 		if any function in src/backend/commands/alter.c changed, you
 * 		better also change at here.
 *
 * Authored by sunchengxi@highgo.com, 20240510.
 *
 * Copyright:
 * 		Copyright (c) 2024, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/restricted_dba/alter.c
 * -------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/htup_details.h"
#include "access/relation.h"
#include "access/sysattr.h"
#include "access/table.h"
#include "catalog/dependency.h"
#include "catalog/indexing.h"
#include "catalog/namespace.h"
#include "catalog/objectaccess.h"
#include "catalog/pg_collation.h"
#include "catalog/pg_conversion.h"
#include "catalog/pg_event_trigger.h"
#include "catalog/pg_foreign_data_wrapper.h"
#include "catalog/pg_foreign_server.h"
#include "catalog/pg_language.h"
#include "catalog/pg_largeobject.h"
#include "catalog/pg_largeobject_metadata.h"
#include "catalog/pg_namespace.h"
#include "catalog/pg_opclass.h"
#include "catalog/pg_opfamily.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_statistic_ext.h"
#include "catalog/pg_subscription.h"
#include "catalog/pg_ts_config.h"
#include "catalog/pg_ts_dict.h"
#include "catalog/pg_ts_parser.h"
#include "catalog/pg_ts_template.h"
/* Begin - ReqID:SRS-SQL-PACKAGE */
#include "catalog/pg_package.h"
/* End - ReqID:SRS-SQL-PACKAGE */
#include "commands/alter.h"
#include "commands/collationcmds.h"
#include "commands/conversioncmds.h"
#include "commands/dbcommands.h"
#include "commands/defrem.h"
#include "commands/event_trigger.h"
#include "commands/extension.h"
#include "commands/policy.h"
#include "commands/proclang.h"
#include "commands/publicationcmds.h"
#include "commands/schemacmds.h"
#include "commands/subscriptioncmds.h"
#include "commands/tablecmds.h"
#include "commands/tablespace.h"
#include "commands/trigger.h"
#include "commands/typecmds.h"
#include "commands/user.h"
/* Begin - ReqID:SRS-SQL-PACKAGE */
#include "commands/packagecmds.h"
/* End - ReqID:SRS-SQL-PACKAGE */
#include "miscadmin.h"
#include "parser/parse_func.h"
#include "rewrite/rewriteDefine.h"
#include "tcop/utility.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "utils/rel.h"
#include "utils/syscache.h"
/* Begin - ReqID:SRS-MYSQL-CASE */
#include "mysql_parser/mys_case_sensitive.h"
#include "parser/pg_object_hook.h"
#include "restricted_dba.h"

/*
* Executes an ALTER OBJECT / RENAME TO statement.  Based on the object
* type, the function appropriate to that type is executed.
*
* Return value is the address of the renamed object.
*/
bool
rename_object_check(RenameStmt *stmt)
{
	switch (stmt->renameType)
	{
		case OBJECT_ROLE:
			return rename_role_check(stmt->subname, stmt->newname);
		case OBJECT_TABLESPACE:
		case OBJECT_DATABASE:
			break; /*do nothing, allow to rename by its owner*/ /* Bug#z4756 */
		case OBJECT_SCHEMA:
			return alter_schema_rename_check(stmt->subname, stmt->newname);
		case OBJECT_TABLE:
		case OBJECT_SEQUENCE:
		case OBJECT_VIEW:
		case OBJECT_MATVIEW:
		case OBJECT_INDEX:
		case OBJECT_FOREIGN_TABLE:
		{
			return alter_relation_rename_check(stmt);
		}
		case OBJECT_FUNCTION:
		case OBJECT_PROCEDURE:
		case OBJECT_ROUTINE:
		{
			return alter_proc_rename_check(stmt);
		}
		default:
			return true;
			/*currently do nothing*/

	}

	return true;
}

/*
* Executes an ALTER OBJECT / OWNER TO statement.  Based on the object
* type, the function appropriate to that type is executed.
*/
bool
alter_owner_check(AlterOwnerStmt *stmt)
{
	Oid			newowner = get_rolespec_oid(stmt->newowner, false);

	switch (stmt->objectType)
	{
		case OBJECT_TABLESPACE:
		case OBJECT_DATABASE:
		{
			if(!IS_SYSDBA(newowner))
			{	
				ereport(ERROR,
						(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
						errmsg("permission denied"),
						errdetail("only sysdba_role member can be owner")));
			}
		}
		break;
		case OBJECT_SCHEMA:
		{
			if (!alter_schema_owner_check(strVal((Value *) stmt->object), newowner))
			{	
				ereport(ERROR,
						(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
						 errmsg("permission denied")));
			}
		}
		break;
		case OBJECT_TABLE:
		case OBJECT_SEQUENCE:
		case OBJECT_VIEW:
		case OBJECT_MATVIEW:
		case OBJECT_INDEX:
		case OBJECT_FOREIGN_TABLE:
		{
			/*will never get here, alter table will do the actual work*/
		}
		case OBJECT_FUNCTION:
		case OBJECT_PROCEDURE:
		case OBJECT_ROUTINE:
		{		 
			if (!alter_proc_owner_check(stmt, strVal((Value *) stmt->object), newowner))
			{	
				ereport(ERROR,
						(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
						 errmsg("permission denied")));
			}
		}

		break;
		default:
			return true;/* nothing */
	}

	return true;
}


bool 
alter_object_schema_check(AlterObjectSchemaStmt *stmt,
						  ObjectAddress *oldSchemaAddr)
{
	bool 		call_next = true;
	Oid			oldNspOid;

	switch (stmt->objectType)
	{
		case OBJECT_EXTENSION:
			/*nothing*/
			break;
		/* meaningless to alter a index's schema*/
		case OBJECT_FOREIGN_TABLE:
		case OBJECT_SEQUENCE:
		case OBJECT_TABLE:
		case OBJECT_VIEW:
		case OBJECT_MATVIEW:
			call_next = alter_relation_schema_check(stmt, 
													oldSchemaAddr ? &oldNspOid : NULL);
			break;

		case OBJECT_DOMAIN:
		case OBJECT_TYPE:
			/*nothing*/
			break;

		case OBJECT_FUNCTION:
		case OBJECT_PROCEDURE:
		case OBJECT_ROUTINE:
		{
			call_next = alter_proc_schema_check(stmt, 
												oldSchemaAddr ? &oldNspOid : NULL);
		}

			/* generic code path */
		case OBJECT_AGGREGATE:
		case OBJECT_COLLATION:
		case OBJECT_CONVERSION:
		/* Begin - ReqID:SRS-SQL-PACKAGE */
		case OBJECT_PACKAGE:
		/* End - ReqID:SRS-SQL-PACKAGE */
		case OBJECT_OPERATOR:
		case OBJECT_OPCLASS:
		case OBJECT_OPFAMILY:
		case OBJECT_STATISTIC_EXT:
		case OBJECT_TSCONFIGURATION:
		case OBJECT_TSDICTIONARY:
		case OBJECT_TSPARSER:
		case OBJECT_TSTEMPLATE:
			/*nothing*/
			break;

		default:
			elog(ERROR, "unrecognized AlterObjectSchemaStmt type: %d",
				 (int) stmt->objectType);
	}


	return call_next;
}