/* -------------------------------------------------------------------------
 *
 * File: tablespace.c
 *
 * Abstract: 
 * 		This file contains tablespace command permission check.
 * 		if any function in src/backend/commands/tablespace.c changed, you
 * 		better also keep change at here.
 *
 * Authored by sunchengxi@highgo.com, 20240510.
 *
 * Copyright:
 * 		Copyright (c) 2024, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/restricted_dba/tablespace.c
 * -------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/heapam.h"
#include "access/htup_details.h"
#include "access/reloptions.h"
#include "access/sysattr.h"
#include "access/tableam.h"
#include "access/xact.h"
#include "access/xlog.h"
#include "access/xloginsert.h"
#include "catalog/catalog.h"
#include "catalog/dependency.h"
#include "catalog/indexing.h"
#include "catalog/namespace.h"
#include "catalog/objectaccess.h"
#include "catalog/pg_namespace.h"
#include "catalog/pg_tablespace.h"
#include "commands/comment.h"
#include "commands/seclabel.h"
#include "commands/tablecmds.h"
#include "commands/tablespace.h"
#include "common/file_perm.h"
#include "miscadmin.h"
#include "postmaster/bgwriter.h"
#include "storage/fd.h"
#include "storage/lmgr.h"
#include "storage/standby.h"
#include "utils/acl.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"
#include "utils/memutils.h"
#include "utils/rel.h"
#include "utils/varlena.h"

#include "restricted_dba.h"

bool
create_tablespace_check(CreateTableSpaceStmt *stmt)
{
	Oid			ownerId = InvalidOid;

	/* Must be super user */
	if (!IS_SYSDBA(GetUserId()))
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("permission denied to create tablespace \"%s\"",
						stmt->tablespacename),
				 errhint("Must be member of sysdba_role to create a tablespace.")));
	

	/* However, the eventual owner of the tablespace need not be */
	if (stmt->owner)
		ownerId = get_rolespec_oid(stmt->owner, false);
	else
		ownerId = GetUserId();

    if (!IS_SYSDBA(ownerId))
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("permission denied to create tablespace \"%s\"",
						stmt->tablespacename),
				 errhint("Must be member of sysdba_role to own a tablespace.")));
    
    /*and let next utility funciton do finish other check*/
	return true;
}

// /*
//  * No need to check alter tablespace set/reset and drop table space
//  * since only owner can do that. alter owner will be processed in alter.c 
//  * And we allow owner to rename it.
//  */
// bool 
// drop_tablespace_check(ObjectAddress *object, char* auditname)
// {
// 	Oid 		roleid = GetUserId();
// 	Oid			spc_oid = object->objectId;
// 	bool		result = true;

// 	/*only owner allow to drop a tablespace so limit superuser to drop normal user's */
// 	if (IS_SYSDBA(roleid))
// 	{
// 		HeapTuple	spctuple;
// 		Oid			spcowner;

// 		/* Search syscache for pg_tablespace */
// 		spctuple = SearchSysCache1(TABLESPACEOID, ObjectIdGetDatum(spc_oid));
// 		if (!HeapTupleIsValid(spctuple))
// 			ereport(ERROR,
// 					(errcode(ERRCODE_UNDEFINED_OBJECT),
// 					errmsg("tablespace with OID %u does not exist", spc_oid)));

// 		spcowner = ((Form_pg_tablespace) GETSTRUCT(spctuple))->spcowner;

// 		ReleaseSysCache(spctuple);

// 		if (IS_SYSSSO(spcowner) || IS_SYSSAO(spcowner))
// 			result = false;
// 	}

// 	return result;
// }