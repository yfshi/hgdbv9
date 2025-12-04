/* -------------------------------------------------------------------------
 *
 * File: user.c
 *
 * Abstract: 
 * 		This file contains all the user-related command permission check.
 * 		most functions in this file are variation of functions in
 * 		src/backend/commands/user.c if they changed, you should also
 * 		change at here.
 *
 * Authored by sunchengxi@highgo.com, 20240510.
 *
 * Copyright:
 * 		Copyright (c) 2024, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/restricted_dba/user.c
 * -------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/genam.h"
#include "access/htup_details.h"
#include "access/table.h"
#include "access/xact.h"
#include "access/heapam.h"
#include "catalog/binary_upgrade.h"
#include "catalog/catalog.h"
#include "catalog/dependency.h"
#include "catalog/indexing.h"
#include "catalog/objectaccess.h"
#include "catalog/pg_auth_members.h"
#include "catalog/pg_authid.h"
#include "catalog/pg_database.h"
#include "catalog/pg_db_role_setting.h"
#include "commands/comment.h"
#include "commands/dbcommands.h"
#include "commands/seclabel.h"
#include "commands/user.h"
#include "libpq/crypt.h"
#include "miscadmin.h"
#include "storage/lmgr.h"
#include "utils/acl.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/syscache.h"
#include "utils/timestamp.h"
#include "utils/ora_compatible.h"
#include "libpq/libpq-be.h"
#include "utils/snapmgr.h"
#include "access/tableam.h"

#include "se_features_interface.h"
#include "restricted_dba.h"

static void 
change_member_check(const char* rolname, Oid roleid, Oid grantor,
					List *grantee_specs, List *grantee_ids,
					bool admin_opt, bool is_grant)
{
	ListCell   *id_item;
	ListCell   *spec_item;

	Assert(grantor != InvalidOid);

	if (OidIsValid(roleid) && (IS_SYSDBA(roleid) || IS_SYSSSO(roleid) || IS_SYSSAO(roleid)))
	{
		/* make sure it has admin of security role */
		if (!is_admin_of_role_nosuper(grantor,roleid))
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					 errmsg("permission denied"),
					 errdetail("must have admin option to operate security role \"%s\"",
					 rolname)));

		if (grantor == roleid)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_GRANT_OPERATION),
					 errmsg("permission denied"),
					 errdetail("a user of security role cannot operate itself in membership")));

		if (admin_opt)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_GRANT_OPERATION),
					 errmsg("permission denied"),
					 errdetail("security roles cannot be operated with admin option")));
	}

	if(grantee_ids && grantee_specs)
	{
		forboth(spec_item, grantee_specs, id_item, grantee_ids)
		{
			RoleSpec   *grantee_spec =lfirst_node(RoleSpec, spec_item);
			Oid 		grantee_id = lfirst_oid(id_item);
			char	   *grantee_name = get_rolespec_name(grantee_spec);

			Assert(grantee_id != InvalidOid);


			if (IS_SYSDBA(grantee_id) || 
				IS_SYSSSO(grantee_id) ||
				IS_SYSSAO(grantee_id))
			{
				if (is_grant)
				{
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_GRANT_OPERATION),
							 errmsg("permission denied"),
							 errdetail("cannot give other role to security role \"%s\"",
									 grantee_name)));
				}	
				else
				{
					if (grantor == grantee_id)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_GRANT_OPERATION),
								 errmsg("permission denied"),
								 errdetail("security role \"%s\" cannot do self revoke",
										 grantee_name)));
				}
			}
		}
	}
		
}

/*
 * The main purpose of is funciton is to pervent some possible privilege misuse
 * 1).we don't want a role with createrole privilege to grant a security role to
 * others. 2).we don't want a security role be granted with grant option. 3). we
 * don't want any role has more than one security role.
 */
bool
grant_role_check(GrantRoleStmt *stmt)
{
    Relation	pg_authid_rel;
    Oid         grantor = InvalidOid;
    List	   *grantee_ids;
    ListCell   *item;


    if (stmt->grantor)
        grantor = get_rolespec_oid(stmt->grantor, false);
    else
        grantor = GetUserId();

    grantee_ids = roleSpecsToIds(stmt->grantee_roles);

    /* AccessShareLock is enough since we aren't modifying pg_authid */
    pg_authid_rel = table_open(AuthIdRelationId, AccessShareLock);

    /*
     * Step through all of the granted roles and add/remove entries for the
     * grantees, or, if admin_opt is set, then just add/remove the admin
     * option.
     *
     * Note: Permissions checking is done by AddRoleMems/DelRoleMems
     */
    foreach(item, stmt->granted_roles)
    {
        AccessPriv *priv = (AccessPriv *) lfirst(item);
        char       *rolename = priv->priv_name;
        Oid         roleid = InvalidOid;

        /* Must reject priv(columns) and ALL PRIVILEGES(columns) */
		if (rolename == NULL || priv->cols != NIL)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_GRANT_OPERATION),
					 errmsg("column names cannot be included in GRANT/REVOKE ROLE")));


		roleid = get_role_oid(rolename, false);

		change_member_check(rolename, roleid, grantor,
			  				stmt->grantee_roles, grantee_ids,
							stmt->admin_opt,stmt->is_grant);

	}

	/*
	 * Close pg_authid, but keep lock till commit.
	 */
	table_close(pg_authid_rel, NoLock);

	return true;
}

bool 
alter_role_check(AlterRoleStmt *stmt)
{
	Relation	pg_authid_rel;
	HeapTuple	tuple;
	Form_pg_authid authform;
	ListCell   *option;
	char	   *rolename = NULL;
	int			issuper = -1;	/* Make the user a superuser? */
	int			createrole = -1;	/* Can this user create roles? */
	int			createdb = -1;	/* Can the user create databases? */
	List	   *rolemembers = NIL;	/* roles to be added/removed */
	DefElem    *dpassword = NULL;
	DefElem    *dissuper = NULL;
	DefElem    *dinherit = NULL;
	DefElem    *dcreaterole = NULL;
	DefElem    *dcreatedb = NULL;
	DefElem    *dcanlogin = NULL;
	DefElem    *disreplication = NULL;
	DefElem    *dconnlimit = NULL;
	DefElem    *drolemembers = NULL;
	DefElem    *dvalidUntil = NULL;
	DefElem    *dbypassRLS = NULL;
	DefElem    *dresgroup = NULL;
	Oid			roleid;

	if (strcmp(stmt->role->rolename, SYSDBA_ROLE) == 0 ||
		strcmp(stmt->role->rolename, SYSSSO_ROLE) == 0 ||
		strcmp(stmt->role->rolename, SYSSAO_ROLE) == 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_RESERVED_NAME),
				 errmsg("security role \"%s\" cannot be altered",stmt->role->rolename)));

	}

	/* Extract options from the statement node tree */
	/*Begin -- bug#z4288*/
	/*
	 * Since no one should modify security user's attribute
	 * we have to check all alter user option
	 */
	foreach(option, stmt->options)
	{
		DefElem    *defel = (DefElem *) lfirst(option);

		if (strcmp(defel->defname, "password") == 0)
		{
			if (dpassword)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options")));
			dpassword = defel;
		}
		else if (strcmp(defel->defname, "superuser") == 0)
		{
			if (dissuper)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options")));
			dissuper = defel;
		}
		else if (strcmp(defel->defname, "inherit") == 0)
		{
			if (dinherit)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options")));
			dinherit = defel;
		}
		else if (strcmp(defel->defname, "createrole") == 0)
		{
			if (dcreaterole)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options")));
			dcreaterole = defel;
		}
		else if (strcmp(defel->defname, "createdb") == 0)
		{
			if (dcreatedb)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options")));
			dcreatedb = defel;
		}
		else if (strcmp(defel->defname, "canlogin") == 0)
		{
			if (dcanlogin)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options")));
			dcanlogin = defel;
		}
		else if (strcmp(defel->defname, "isreplication") == 0)
		{
			if (disreplication)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options")));
			disreplication = defel;
		}
		else if (strcmp(defel->defname, "connectionlimit") == 0)
		{
			if (dconnlimit)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options")));
			dconnlimit = defel;
		}
		else if (strcmp(defel->defname, "rolemembers") == 0 &&
				 stmt->action != 0)
		{
			if (drolemembers)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options")));
			drolemembers = defel;
		}
		else if (strcmp(defel->defname, "validUntil") == 0)
		{
			if (dvalidUntil)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options")));
			dvalidUntil = defel;
		}
		else if (strcmp(defel->defname, "bypassrls") == 0)
		{
			if (dbypassRLS)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options")));
			dbypassRLS = defel;
		}
		/*Begin --bug#z4761*/
		/*
		 * For resource group we currently don't add any restriction, just
		 * make it pass alter user. Maybe it will follows alter user rule latter.
		 */
		else if (strcmp(defel->defname, "resourceGroup") == 0)
		{

			if (dresgroup)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options")));
			dresgroup = defel;
		}
		/*End --bug#z4761*/
		else
			elog(ERROR, "option \"%s\" not recognized",
				 defel->defname);
	}
	/*End -- bug#z4288*/

	if (dissuper)
		issuper = intVal(dissuper->arg);
	if (dcreaterole)
		createrole = intVal(dcreaterole->arg);
	if (dcreatedb)
		createdb = intVal(dcreatedb->arg);
	if (drolemembers)
		rolemembers = (List *) drolemembers->arg;

	/*no one allowed to have these attributes*/
	if (issuper > 0 || createrole > 0 || createdb > 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("permission denied")));
	}

	/*
	 * Scan the pg_authid relation to be certain the user exists.
	 */
	pg_authid_rel = table_open(AuthIdRelationId, AccessShareLock);

	tuple = get_rolespec_tuple(stmt->role);
	

	authform = (Form_pg_authid) GETSTRUCT(tuple);
	rolename = pstrdup(NameStr(authform->rolname));
	roleid = authform->oid;
	
	if (IS_SYSDBA(roleid) || IS_SYSSSO(roleid) || IS_SYSSAO(roleid))
	{
		/*allow alter password and validuntil*/
		if (dissuper ||
			dcreatedb ||
			dcreaterole ||
			dinherit ||
			dcanlogin || 
			disreplication || 
			dconnlimit || 
			dbypassRLS ||
			drolemembers)
		{
			ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("permission denied")));
		}

		/*security role member can only alter password for itself*/
		if (dpassword && roleid != GetUserId())
		{
			ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("permission denied")));
		}
	}
	else
	{
		/*only syssso member can alter password for normal user*/
		/*bug#z5238 -- normal users can alter password for themselves*/
		if (dpassword && roleid != GetUserId() && !IS_SYSSSO(GetUserId()))
		{
			ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("permission denied")));
		}
	}

	/* valid until can only alter by syssso member */
	if (dvalidUntil && !IS_SYSSSO(GetUserId()))
	{
		ereport(ERROR,
			(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				errmsg("permission denied")));
	}

	/*alter group*/
	if (rolemembers)
	{
		if(stmt->action == 1)
			change_member_check(rolename, roleid, GetUserId(),
								rolemembers, roleSpecsToIds(rolemembers),
								false,true);
		if(stmt->action == -1)
			change_member_check(rolename, roleid, GetUserId(),
								rolemembers, roleSpecsToIds(rolemembers),
								false,false);
	}

	ReleaseSysCache(tuple);
	/*
	 * Close pg_authid, but keep lock till commit.
	 */
	table_close(pg_authid_rel, AccessShareLock);

	/* if not error out, continue other process */
	return true;
}

/*
 * CREATE ROLE
 */
bool
create_role_check(ParseState *pstate, CreateRoleStmt *stmt)
{
	ListCell   *item;
	ListCell   *option;
	bool		issuper = false;	/* Make the user a superuser? */
	bool		createrole = false; /* Can this user create roles? */
	bool		createdb = false;	/* Can the user create databases? */
	List	   *addroleto = NIL;	/* roles to make this a member of */
	List	   *rolemembers = NIL;	/* roles to be members of this role */
	List	   *adminmembers = NIL; /* roles to be admins of this role */
	DefElem    *dissuper = NULL;
	DefElem    *dcreaterole = NULL;
	DefElem    *dcreatedb = NULL;
	DefElem    *daddroleto = NULL;
	DefElem    *drolemembers = NULL;
	DefElem    *dadminmembers = NULL;


	/* Extract options from the statement node tree */
	foreach(option, stmt->options)
	{
		DefElem    *defel = (DefElem *) lfirst(option);

		if (strcmp(defel->defname, "superuser") == 0)
		{
			if (dissuper)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options"),
						 parser_errposition(pstate, defel->location)));
			dissuper = defel;
		}
		else if (strcmp(defel->defname, "createrole") == 0)
		{
			if (dcreaterole)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options"),
						 parser_errposition(pstate, defel->location)));
			dcreaterole = defel;
		}
		else if (strcmp(defel->defname, "createdb") == 0)
		{
			if (dcreatedb)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options"),
						 parser_errposition(pstate, defel->location)));
			dcreatedb = defel;
		}
		else if (strcmp(defel->defname, "addroleto") == 0)
		{
			if (daddroleto)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options"),
						 parser_errposition(pstate, defel->location)));
			daddroleto = defel;
		}
		else if (strcmp(defel->defname, "rolemembers") == 0)
		{
			if (drolemembers)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options"),
						 parser_errposition(pstate, defel->location)));
			drolemembers = defel;
		}
		else if (strcmp(defel->defname, "adminmembers") == 0)
		{
			if (dadminmembers)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options"),
						 parser_errposition(pstate, defel->location)));
			dadminmembers = defel;
		}
	}


	if (dissuper)
		issuper = intVal(dissuper->arg) != 0;
	if (dcreaterole)
		createrole = intVal(dcreaterole->arg) != 0;
	if (dcreatedb)
		createdb = intVal(dcreatedb->arg) != 0;
	if (daddroleto)
		addroleto = (List *) daddroleto->arg;
	if (drolemembers)
		rolemembers = (List *) drolemembers->arg;
	if (dadminmembers)
		adminmembers = (List *) dadminmembers->arg;

	if (issuper || createrole || createdb )
	{
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				errmsg("permission denied")));
	}

	/*
	 * Add the new role to the specified existing roles.
	 */
	if (addroleto)
	{
		foreach(item, addroleto)
		{
			RoleSpec   *oldrole = lfirst(item);
			HeapTuple	oldroletup = get_rolespec_tuple(oldrole);
			Form_pg_authid oldroleform = (Form_pg_authid) GETSTRUCT(oldroletup);
			Oid			oldroleid = oldroleform->oid;

			if (IS_SYSSSO(oldroleid) || IS_SYSSAO(oldroleid) || IS_SYSDBA(oldroleid))
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_GRANT_OPERATION),
						 errmsg("permission denied"),
						 errdetail("cannot set security roles in 'IN ROLE/GROUP' clause.")));


			ReleaseSysCache(oldroletup);
		}
	}

	change_member_check(NULL, InvalidOid, GetUserId(),
						rolemembers, roleSpecsToIds(rolemembers),
						false, true);

	change_member_check(NULL, InvalidOid,GetUserId(),
						adminmembers, roleSpecsToIds(adminmembers),
						true, true);

	return true;
}

/*
 * DROP ROLE
 */
bool
drop_role_check(DropRoleStmt *stmt)
{
	Relation	pg_authid_rel;
	ListCell   *item;

	/*
	 * Scan the pg_authid relation to find the Oid of the role(s) to be
	 * deleted.
	 */
	pg_authid_rel = table_open(AuthIdRelationId, AccessShareLock);

	foreach(item, stmt->roles)
	{
		RoleSpec   *rolspec = lfirst(item);
		char	   *role;
		HeapTuple	tuple;
		Form_pg_authid roleform;
		Oid			roleid = InvalidOid;

		if (rolspec->roletype != ROLESPEC_CSTRING)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("cannot use special role specifier in DROP ROLE")));
		role = rolspec->rolename;

		tuple = SearchSysCache1(AUTHNAME, PointerGetDatum(role));
		if (!HeapTupleIsValid(tuple))
		{
			if (!stmt->missing_ok)
			{
				ereport(ERROR,
						(errcode(ERRCODE_UNDEFINED_OBJECT),
						 errmsg("role \"%s\" does not exist", role)));
			}
			else
			{
				ereport(NOTICE,
						(errmsg("role \"%s\" does not exist, skipping",
								role)));
			}

			continue;
		}

		roleform = (Form_pg_authid) GETSTRUCT(tuple);
		roleid = roleform->oid;

		if (IS_SYSDBA(roleid) || IS_SYSSSO(roleid) || IS_SYSSAO(roleid))
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					errmsg("permission denied")));
		}
		
		ReleaseSysCache(tuple);
	}

	/*
	 * Now we can clean up; but keep locks until commit.
	 */
	table_close(pg_authid_rel, AccessShareLock);

	return true;
}

bool
alter_role_set_check(AlterRoleSetStmt *stmt)
{
	HeapTuple	roletuple;
	Form_pg_authid roleform;
	Oid			databaseid = InvalidOid;
	Oid			roleid = InvalidOid;

	/*Begin -- bug#z5555*/
	if (stmt->setstmt->name == NULL)
		return true;
	/*End -- bug#z5555*/

	if (stmt->role)
	{
		check_rolespec_name(stmt->role,
							_("Cannot alter reserved roles."));

		roletuple = get_rolespec_tuple(stmt->role);
		roleform = (Form_pg_authid) GETSTRUCT(roletuple);
		roleid = roleform->oid;

		/*
		 * Obtain a lock on the role and make sure it didn't go away in the
		 * meantime.
		 */
		shdepLockAndCheckObject(AuthIdRelationId, roleid);

		ReleaseSysCache(roletuple);

		if (pg_strncasecmp(stmt->setstmt->name,"se_feature.",strlen("se_feature.")) == 0)
		{
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("permission denied")));

			/* we should never get here */
			return false;
		}
	}
	else if (stmt->database != NULL) /* set for all roles for datebase */
	{
		/*
		 * If no role is specified, then this is effectively the same as
		 * ALTER DATABASE ... SET, so use the same permission check.
		 */
		if (pg_strncasecmp(stmt->setstmt->name,"se_feature.",strlen("se_feature.")) == 0 ||
			pg_strncasecmp(stmt->setstmt->name,"credcheck.",strlen("credcheck.")) == 0)
		{

			/*Begin -- bug#z4455*/
			databaseid = get_database_oid(stmt->database, false);
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
			shdepLockAndCheckObject(DatabaseRelationId, databaseid);

			AlterSetting(databaseid, InvalidOid, stmt->setstmt);

			UnlockSharedObject(DatabaseRelationId, databaseid, 0, AccessShareLock);
		
			return false;
		}
	}

	return true;
}

/*
 * Rename role
 */
bool
rename_role_check(const char *oldname, const char *newname)
{
	HeapTuple	oldtuple;
	Relation	rel;
	Oid			roleid;
	Form_pg_authid authform;

	rel = table_open(AuthIdRelationId, AccessShareLock);
	
	oldtuple = SearchSysCache1(AUTHNAME, CStringGetDatum(oldname));
	if (!HeapTupleIsValid(oldtuple))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("role \"%s\" does not exist", oldname)));

	authform = (Form_pg_authid) GETSTRUCT(oldtuple);
	roleid = authform->oid;

	if (IS_SYSDBA(roleid) || IS_SYSSSO(roleid) || IS_SYSSAO(roleid))
	{
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				errmsg("permission denied")));
	}

	ReleaseSysCache(oldtuple);

	/*
	 * Close pg_authid, but keep lock till commit.
	 */
	table_close(rel, AccessShareLock);

	return true;
}


/*
 * sysdba_role member will implictly get or loss superuser, 
 * createrole, and createdb attribute when membership changing.
 */
void
grant_sysdba_priv(Oid roleid, bool is_grant)
{
	Datum		new_record[Natts_pg_authid];
	bool		new_record_nulls[Natts_pg_authid];
	bool		new_record_repl[Natts_pg_authid];
	HeapTuple	oldtuple;
	HeapTuple	newtuple;
	Relation	pg_authid_rel;
	TupleDesc	pg_authid_dsc;

	/*
	 * Scan the pg_authid relation to be certain the user exists.
	 */
	pg_authid_rel = table_open(AuthIdRelationId, RowExclusiveLock);
	pg_authid_dsc = RelationGetDescr(pg_authid_rel);
	
	oldtuple = SearchSysCache1(AUTHOID, ObjectIdGetDatum(roleid));
	if (!HeapTupleIsValid(oldtuple))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("role with oid\"%d\" does not exist", roleid)));

	/*
	 * Build an updated tuple, perusing the information just obtained
	 */
	MemSet(new_record, 0, sizeof(new_record));
	MemSet(new_record_nulls, false, sizeof(new_record_nulls));
	MemSet(new_record_repl, false, sizeof(new_record_repl));

	if(is_grant)
	{
		new_record[Anum_pg_authid_rolsuper - 1] = BoolGetDatum(true);
		new_record[Anum_pg_authid_rolcreaterole - 1] = BoolGetDatum(true);
		new_record[Anum_pg_authid_rolcreatedb - 1] = BoolGetDatum(true);
	}
	else
	{
		new_record[Anum_pg_authid_rolsuper - 1] = BoolGetDatum(false);
		new_record[Anum_pg_authid_rolcreaterole - 1] = BoolGetDatum(false);
		new_record[Anum_pg_authid_rolcreatedb - 1] = BoolGetDatum(false);
	}

	new_record_repl[Anum_pg_authid_rolsuper - 1] = true;
	new_record_repl[Anum_pg_authid_rolcreaterole - 1] = true;
	new_record_repl[Anum_pg_authid_rolcreatedb - 1] = true;

	newtuple = heap_modify_tuple(oldtuple, pg_authid_dsc, new_record,
								  new_record_nulls, new_record_repl);
	CatalogTupleUpdate(pg_authid_rel, &oldtuple->t_self, newtuple);

	InvokeObjectPostAlterHook(AuthIdRelationId, roleid, 0);

	ReleaseSysCache(oldtuple);
	heap_freetuple(newtuple);

	/*
	 * Close pg_authid.
	 */
	table_close(pg_authid_rel, RowExclusiveLock);
}