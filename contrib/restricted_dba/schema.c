/* -------------------------------------------------------------------------
 *
 * File: schema.c
 *
 * Abstract: 
 * 		This file contains permission check of schema command.
 * 		if any change in src/backend/commands/schemacmds.c, it's better
 * 		also change here.
 *
 * Authored by sunchengxi@highgo.com, 20240510.
 *
 * Copyright:
 * 		Copyright (c) 2024, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/restricted_dba/schema.c
 * -------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/htup_details.h"
#include "access/table.h"
#include "access/xact.h"
#include "catalog/catalog.h"
#include "catalog/dependency.h"
#include "catalog/indexing.h"
#include "catalog/namespace.h"
#include "catalog/objectaccess.h"
#include "catalog/pg_authid.h"
#include "catalog/pg_namespace.h"
#include "commands/dbcommands.h"
#include "commands/event_trigger.h"
#include "commands/schemacmds.h"
#include "miscadmin.h"
#include "parser/parse_utilcmd.h"
#include "parser/pg_object_hook.h"
#include "parser/scansup.h"
#include "tcop/utility.h"
#include "utils/acl.h"
#include "utils/builtins.h"
#include "utils/jsonb.h"
#include "utils/lsyscache.h"
#include "utils/ora_compatible.h"
#include "utils/rel.h"
#include "utils/syscache.h"
/* Begin - ReqID:SRS-MYSQL-CASE */
#include "mysql_parser/mys_case_sensitive.h"
#include "parser/pg_object_hook.h"
#include "utils/guc.h"
#include "utils/ora_compatible.h"
/* End - ReqID:SRS-MYSQL-CASE */

#include "restricted_dba.h"

/*
 * CREATE SCHEMA
 *
 * Note: caller should pass in location information for the whole
 * CREATE SCHEMA statement, which in turn we pass down as the location
 * of the component commands.  This comports with our general plan of
 * reporting location/len for the whole command even when executing
 * a subquery.
 */
bool
create_schema_check(CreateSchemaStmt *stmt, const char *queryString,
					int stmt_location, int stmt_len)
{
	Oid			owner_uid;
	Oid			saved_uid;
	int			save_sec_context;

	GetUserIdAndSecContext(&saved_uid, &save_sec_context);

	/*
	 * Who is supposed to own the new schema?
	 */
	if (stmt->authrole)
		owner_uid = get_rolespec_oid(stmt->authrole, false);
	else
		owner_uid = saved_uid;

	if (IS_SYSSSO(GetUserId()) || IS_SYSSAO(GetUserId()) ||
		(IS_SYSSSO(owner_uid) || IS_SYSSAO(owner_uid)))
	{
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("permission denied"),
				 errdetail("member of syssso_role and syssao_role cannot own a schema")));
	}

	/*
	 * since create privilege on database can not granted to syssso_role
	 * or syssao_role, so we don't check the whewher user can create schema.
	 */

	return true;
}

bool
use_schema_check(Oid namespaceid, Oid roleid)
{
	bool result = true;

	if ((namespaceid == PG_CATALOG_NAMESPACE) ||
		(namespaceid == PG_PUBLIC_NAMESPACE) ||
		(isTempNamespace(namespaceid)))
	{
		return true;
	}
	else if (IS_SYSDBA(roleid))
	{
		HeapTuple	tp;
		Oid			nspowner;
		Form_pg_namespace nsptup;
		char 	   *nspname;
		bool		have_priv = false;

		/* Search syscache for pg_tablespace */
		tp = SearchSysCache1(NAMESPACEOID, ObjectIdGetDatum(namespaceid));
		if (!HeapTupleIsValid(tp))
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_SCHEMA),
					errmsg("schema with OID %u does not exist", namespaceid)));

		nsptup = (Form_pg_namespace) GETSTRUCT(tp);
		nspowner = nsptup->nspowner;
		nspname  = pstrdup(NameStr(nsptup->nspname));

		ReleaseSysCache(tp);

		have_priv = (pg_namespace_aclmask_nosuper(namespaceid, roleid, ACL_CREATE , ACLMASK_ANY) != 0);

		if (IS_SYSSSO(nspowner) || IS_SYSSAO(nspowner))
		{
			/* Bug#z4741*/
			if(!have_priv)
			{
				result = false;
				ereport(ERROR,
						(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
						 errmsg("permission denied for schema %s", nspname),
						 errdetail("sysdba_role member cannot use schema of "
								   "syssso_role/syssao_role member schema")));
			}
		}
		else if (restricted_dba && IS_SYSDBA(GetUserId()))
		{
			if(!have_priv)
			{
				result = false;
				ereport(ERROR,
						(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
						 errmsg("permission denied for schema %s", nspname),
						 errdetail("don't have enough privilege on schema %s", nspname)));
			}
		}

	}
	/*leave other non superuser case to acl*/

	return result;
}

bool
alter_schema_owner_check(const char *name, Oid newOwnerId)
{
	Oid			nspowner;
	HeapTuple	tup;
	Relation	rel;
	Form_pg_namespace nspform;
	bool		result = true;

	rel = table_open(NamespaceRelationId, RowExclusiveLock);

	tup = SearchSysCache1(NAMESPACENAME, CStringGetDatum(name));
	if (!HeapTupleIsValid(tup))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_SCHEMA),
				 errmsg("schema \"%s\" does not exist", name)));

	nspform = (Form_pg_namespace) GETSTRUCT(tup);
	nspowner = nspform->nspowner;

	if (IS_SYSDBA(GetUserId()) &&
	 	(IS_SYSSSO(nspowner) || IS_SYSSAO(nspowner)))
	{
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("permission denied"),
				 errdetail("cannot alter schema of syssso_role/syssao_role "
				 			"member")));
	}

	if(IS_SYSSSO(newOwnerId) || IS_SYSSAO(newOwnerId))
	{	
		result = false;
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("permission denied"),
				 errdetail("syssso_role/syssao_role member "
				 		   "cannot be owner of schema")));
	}

	if (restricted_dba && IS_SYSDBA(GetUserId()))
	{
		if (nspowner != GetUserId())
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
						errmsg("permission denied"),
						errdetail("sysdba_role member can only alter schema of "
								"itself")));
		}

		if (!IS_SYSDBA(newOwnerId))
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
						errmsg("permission denied"),
						errdetail("only sysdba_role member is allowed to own a "
								"schema of another sysdba_role member")));

	}

	ReleaseSysCache(tup);

	table_close(rel, RowExclusiveLock);

	return result;
}

bool
alter_schema_rename_check(const char *oldname, const char *newname)
{
	Oid			nspowner;
	HeapTuple	tup;
	Relation	rel;
	Form_pg_namespace nspform;

	rel = table_open(NamespaceRelationId, RowExclusiveLock);

	tup = SearchSysCache1(NAMESPACENAME, CStringGetDatum(oldname));
	if (!HeapTupleIsValid(tup))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_SCHEMA),
				 errmsg("schema \"%s\" does not exist", oldname)));

	nspform = (Form_pg_namespace) GETSTRUCT(tup);
	nspowner = nspform->nspowner;


	if(IS_SYSSSO(nspowner) || IS_SYSSAO(nspowner))
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				errmsg("permission denied"),
				errdetail("cannot alter schema of syssso_role/syssao_role "
							"member")));

	if (restricted_dba && IS_SYSDBA(GetUserId()))
	{
		if (nspowner != GetUserId())
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
						errmsg("permission denied"),
						errdetail("sysdba_role member can only alter schema of "
								  "itself")));
		}
	}


	ReleaseSysCache(tup);

	table_close(rel, RowExclusiveLock);

	return true;
}


bool 
drop_schema_check(ObjectAddress *object)
{
	Oid			nsp_oid = object->objectId;
	HeapTuple	nsptuple;
	Oid			nspowner;
	char	   *auditname;

	auditname = getObjectIdentity(object, false);

	/* Begin :Bug#z6280*/
	if(nsp_oid == PG_PUBLIC_NAMESPACE)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("cannot drop public schema")));
	}
	/* End: Bug#z6280*/

	/* Search syscache for pg_tablespace */
	nsptuple = SearchSysCache1(NAMESPACEOID, ObjectIdGetDatum(nsp_oid));
	if (!HeapTupleIsValid(nsptuple))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_SCHEMA),
				errmsg("schema with OID %u does not exist", nsp_oid)));

	nspowner = ((Form_pg_namespace) GETSTRUCT(nsptuple))->nspowner;

	ReleaseSysCache(nsptuple);

	if (IS_SYSSSO(nspowner) || IS_SYSSAO(nspowner))
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("permission denied for namespace %s",auditname),
				 errdetail("cannot drop schema of syssso_role/syssao_role "
						"member")));

	if (restricted_dba && IS_SYSDBA(GetUserId()))
	{
		if (nspowner != GetUserId())
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
						errmsg("permission denied"),
						errdetail("sysdba_role member can only drop schema of "
								  "itself")));
		}
	}

	return true;
}