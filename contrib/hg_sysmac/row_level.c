/*-------------------------------------------------------------------------
 *
 * row_level.c
 *	  Routines to handle mandatory access control
 *
 * Authored by wangjie@highgo.com, 20250917.
 *
 * Copyright:
 * Copyright (c) 2009-2025, HighGo Software Co.,Ltd. All right reserved.
 *
 * IDENTIFICATION
 *	  contrib/hg_sysmac/row_level.c
 *
 *-------------------------------------------------------------------------
 */

/* Begin - ReqID: SRS-SECURITY-LMAC */

#include "postgres.h"

#include "access/table.h"
#include "access/genam.h"
#include "catalog/pg_authid.h"
#include "catalog/pg_inherits.h"
#include "catalog/pg_namespace.h"
#include "executor/spi.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "miscadmin.h"
#include "parser/scansup.h"
#include "parser/pg_object_hook.h"

#include "hg_sysmac.h"

typedef enum PolicyActions
{
	APPLY,
	ENABLE,
	DISABLE,
	DROP,
}PolicyActions;

/*
 * relation_has_hgseclabel
 *
 * check whether the given Relation has a column named "__hg_seclabel__"
 */
static bool
relation_has_hgseclabel(Relation rel)
{
	TupleDesc desc = RelationGetDescr(rel);
	int i = 0;

	for (i = 0; i < desc->natts; i++)
	{
		Form_pg_attribute attr = TupleDescAttr(desc, i);

		/* skip dropped columns */
		if (attr->attisdropped)
			continue;

		if (strcmp(NameStr(attr->attname), "__hg_seclabel__") == 0)
			return true;
	}

	return false;
}

static void
mac_identifier_case_transform(Oid relid, Relation target_rel, char **schema, char **relname)
{
	if (compatible_mode == MYS_MODE)
	{
		ObjectAddress address;
		PgObjectInfo oriinf_schema = NULL;
		PgObjectInfo oriinf_relname = NULL;

		memset(&address, 0, sizeof(ObjectAddress));
		ObjectAddressSet(address, RelationRelationId, relid);
		if (pg_object_get_ori_infor_hook)
		{
			oriinf_relname = pg_object_get_ori_infor_hook (address);
			if (oriinf_relname)
				*relname = pstrdup(oriinf_relname->oriname);
			else
				*relname = pstrdup(RelationGetRelationName(target_rel));
		}

		memset(&address, 0, sizeof(ObjectAddress));
		ObjectAddressSet(address, NamespaceRelationId, oriinf_relname->orischema);
		if (pg_object_get_ori_infor_hook)
		{
			oriinf_schema = pg_object_get_ori_infor_hook (address);
			if (oriinf_schema)
				*schema = pstrdup(oriinf_schema->oriname);
			else
				*schema = get_namespace_name(target_rel->rd_rel->relnamespace);
		}
	}
	else if (compatible_mode == ORA_MODE)
	{
		char *old_relname = pstrdup(RelationGetRelationName(target_rel));
		char* old_schema = get_namespace_name(target_rel->rd_rel->relnamespace);

		/* if identifier_case_switch is INTERCHANGE, performs the transformation. */
		if (INTERCHANGE == identifier_case_switch)
		{
			*schema = identifier_case_transform(old_schema, strlen(old_schema));
			*relname = identifier_case_transform(old_relname, strlen(old_relname));
		}
		else
		{
			*schema = old_schema;
			*relname = old_relname;
		}
	}
	else
	{
		*relname = pstrdup(RelationGetRelationName(target_rel));
		*schema = get_namespace_name(target_rel->rd_rel->relnamespace);
	}

	if (*schema == NULL || *relname == NULL)
	{
		elog(ERROR, "Table name and schema for '%u' not found.", relid);
	}
}

/*
 * Row-level mandatory access control processing supports the following actions:
 * 	 - apply  : function - apply_table_policy
 * 	 - enable : function - enable_table_policy
 * 	 - disable: function - disable_table_policy
 * 	 - drop   : function - drop_table_policy
 */
static bool
apply_row_policy_on_table(Oid relid, PolicyActions action, bool dump)
{
	StringInfoData querybuf;
	Relation	target_rel;
	Oid			relowner;
	Oid			save_userid;
	int			save_sec_context;
	int			save_nestlevel;
	List		*tableIds;
	ListCell	*li;
	char		*schema = NULL;
	char		*relname = NULL;

	initStringInfo(&querybuf);
	target_rel = table_open(relid, AccessShareLock);
	relowner = target_rel->rd_rel->relowner;
	/*
	 * Obtain the case-transformed schema and relname according to
	 * the compatible modes of MySQL / Oracle / PG.
	 */
	mac_identifier_case_transform(relid, target_rel, &schema, &relname);
	table_close(target_rel, AccessShareLock);

	/*
	 * Switch to the owner's userid, so that any functions are run as that
	 * user.  Also lock down security-restricted operations and arrange to
	 * make GUC variable changes local to this command.
	 */
	GetUserIdAndSecContext(&save_userid, &save_sec_context);
	SetUserIdAndSecContext(relowner,
						   save_sec_context |
						   SECURITY_LOCAL_USERID_CHANGE |
						   SECURITY_RESTRICTED_OPERATION);
	save_nestlevel = NewGUCNestLevel();

	/* Open SPI context. */
	if (SPI_connect() != SPI_OK_CONNECT)
		elog(ERROR, "SPI_connect failed");

	tableIds = find_all_inheritors(relid, NoLock, NULL);

	switch (action)
	{
		case APPLY:
		{
			if (target_rel->rd_rel->relispartition)
			{
				if (!relation_has_hgseclabel(target_rel))
				{
					elog(ERROR, "Mandatory access control row-level - partitioned tables"
								" must be applied to the parent before the child tables.");
				}
				if (dump)
					appendStringInfo(&querybuf,
							 "select hg_security.apply_table_policy('%s', '%s', true, true) ",
							 schema, relname);
				else
					appendStringInfo(&querybuf,
							 "select hg_security.apply_table_policy('%s', '%s', true, false) ",
							 schema, relname);
			}
			else
			{
				if (dump)
					appendStringInfo(&querybuf,
							 "select hg_security.apply_table_policy('%s', '%s', false, true) ",
							 schema, relname);
				else
					appendStringInfo(&querybuf,
							 "select hg_security.apply_table_policy('%s', '%s', false, false) ",
							 schema, relname);
			}
			if (SPI_exec(querybuf.data, 0) != SPI_OK_SELECT)
				elog(ERROR, "SPI_exec failed: %s", querybuf.data);

			resetStringInfo(&querybuf);
			/*switch to user that can change pg_class */
			SetUserIdAndSecContext(BOOTSTRAP_SUPERUSERID,
								   save_sec_context |
								   SECURITY_LOCAL_USERID_CHANGE |
								   SECURITY_RESTRICTED_OPERATION);
			foreach(li, tableIds)
			{
				Oid			tableOid = lfirst_oid(li);
				appendStringInfo(&querybuf,
							 "update pg_attribute set attisvisible = false where attrelid = %u and attname = '__hg_seclabel__' ",
							 tableOid);
				if (SPI_exec(querybuf.data, 0) != SPI_OK_UPDATE)
					elog(ERROR, "SPI_exec failed: %s", querybuf.data);
				resetStringInfo(&querybuf);
			}
			break;
		}
		case ENABLE:
		{
			appendStringInfo(&querybuf,
							 "select hg_security.enable_table_policy('%s', '%s') ",
							 schema, relname);
			if (SPI_exec(querybuf.data, 0) != SPI_OK_SELECT)
				elog(ERROR, "SPI_exec failed: %s", querybuf.data);
			break;
		}
		case DISABLE:
		{
			appendStringInfo(&querybuf,
							 "select hg_security.disable_table_policy('%s', '%s') ",
							 schema, relname);
			if (SPI_exec(querybuf.data, 0) != SPI_OK_SELECT)
				elog(ERROR, "SPI_exec failed: %s", querybuf.data);
			break;
		}
		case DROP:
		{
			foreach(li, tableIds)
			{
				char		*inh_schema = NULL;
				char		*inh_relname = NULL;
				Oid			tableOid = lfirst_oid(li);
				Relation	rel = table_open(tableOid, AccessShareLock);
				mac_identifier_case_transform(tableOid, rel, &inh_schema, &inh_relname);
				table_close(rel, AccessShareLock);

				appendStringInfo(&querybuf,
							 "select hg_security.disable_table_policy('%s', '%s') ",
							 inh_schema, inh_relname);
				if (SPI_exec(querybuf.data, 0) != SPI_OK_SELECT)
					elog(ERROR, "SPI_exec failed: %s", querybuf.data);
				resetStringInfo(&querybuf);
			}

			appendStringInfo(&querybuf,
							 "select hg_security.drop_table_policy('%s', '%s') ",
							 schema, relname);
			if (SPI_exec(querybuf.data, 0) != SPI_OK_SELECT)
				elog(ERROR, "SPI_exec failed: %s", querybuf.data);
			break;
		}

		default:
			break;
	}

	/* Close SPI context. */
	if (SPI_finish() != SPI_OK_FINISH)
		elog(ERROR, "SPI_finish failed");

	/* Roll back any GUC changes */
	AtEOXact_GUC(false, save_nestlevel);

	/* Restore userid and security context */
	SetUserIdAndSecContext(save_userid, save_sec_context);

	pfree(schema);
	pfree(relname);

	return true;
}


PG_FUNCTION_INFO_V1(apply_row_policy);
Datum
apply_row_policy(PG_FUNCTION_ARGS)
{
	Oid   relid = PG_GETARG_OID(0);
	char *action_cmd = TextDatumGetCString(PG_GETARG_DATUM(1));
	char *result;
	bool  dump = false;
	PolicyActions action;

	if (PG_NARGS() == 3)
		dump = PG_GETARG_BOOL(2);

	is_security_roles();

	if (strcmp(action_cmd, "apply") == 0)
		action = APPLY;
	else if (strcmp(action_cmd, "enable") == 0)
		action = ENABLE;
	else if (strcmp(action_cmd, "disable") == 0)
		action = DISABLE;
	else if (strcmp(action_cmd, "drop") == 0)
	{
		Relation rel = table_open(relid, AccessShareLock);
		table_close(rel, AccessShareLock);
		action = DROP;
		if (rel->rd_rel->relispartition)
			elog(ERROR, "cannot %s row-level security policy to child tables of partition", action_cmd);
	}
	else
		elog(ERROR, "unsupported policy command type: %s", action_cmd);

	if (apply_row_policy_on_table(relid, action, dump))
		result = "set";
	else
		result = "failed";

	PG_RETURN_TEXT_P(cstring_to_text(result));
}

/*
 * When inserting data, save the current user's seclabel as
 * the row's seclabel into the `__hg_seclabel__` column.
 */
PG_FUNCTION_INFO_V1(mac_row_user_seclabel);
Datum
mac_row_user_seclabel(PG_FUNCTION_ARGS)
{
	char *label = get_obj_seclabel_internal(AuthIdRelationId, GetUserId(), InvalidOid, true);

	if (label)
		PG_RETURN_TEXT_P(cstring_to_text(label));

	PG_RETURN_NULL();
}

/*
 * Row-level MAC rules required for select operation:
 * 	- SELECT: user_seclabel >= row_seclabel
 */
PG_FUNCTION_INFO_V1(mac_row_select_policy);
Datum
mac_row_select_policy(PG_FUNCTION_ARGS)
{
	SecLabel *seclabel_l = NULL;
	SecLabel *seclabel_r = NULL;
	char	 *user_label = NULL;
	bool	  result = false;

	if (!global_on || !global_row_level) /* Bug - #z5940 */
		PG_RETURN_BOOL(true);

	user_label = get_obj_seclabel_internal(AuthIdRelationId, GetUserId(), InvalidOid, true);
	if (user_label)
		seclabel_l = string_to_seclabel(user_label, NULL, true);
	else
		seclabel_l = NULL;

	if (PG_ARGISNULL(0))
		seclabel_r = NULL;
	else
		seclabel_r = PG_GETARG_SECLABEL(0);

	result = (seclabel_compare(seclabel_l, seclabel_r) != LABEL_UNCOMPARABLE);

	if (seclabel_l)
		pfree(seclabel_l);
	if (seclabel_r)
		pfree(seclabel_r);

	PG_RETURN_BOOL(result);
}

/*
 * Row-level MAC rules required for delete operation:
 * 	- SELECT: user_seclabel = row_seclabel
 */
PG_FUNCTION_INFO_V1(mac_row_delete_policy);
Datum
mac_row_delete_policy(PG_FUNCTION_ARGS)
{
	SecLabel *seclabel_l = NULL;
	SecLabel *seclabel_r = NULL;
	char	 *user_label = NULL;
	bool	  result = false;

	if (!global_on || !global_row_level) /* Bug - #z5940 */
		PG_RETURN_BOOL(true);

	user_label = get_obj_seclabel_internal(AuthIdRelationId, GetUserId(), InvalidOid, true);
	if (user_label)
		seclabel_l = string_to_seclabel(user_label, NULL, true);
	else
		seclabel_l = NULL;

	if (PG_ARGISNULL(0))
		seclabel_r = NULL;
	else
		seclabel_r = PG_GETARG_SECLABEL(0);

	result = (seclabel_compare(seclabel_l, seclabel_r) == LABEL_EQUAL);

	if (seclabel_l)
		pfree(seclabel_l);
	if (seclabel_r)
		pfree(seclabel_r);

	PG_RETURN_BOOL(result);
}

/*
 * Row-level MAC rules required for update operation:
 * 	- SELECT: user_seclabel = row_seclabel
 */
PG_FUNCTION_INFO_V1(mac_row_update_policy);
Datum
mac_row_update_policy(PG_FUNCTION_ARGS)
{
	SecLabel *seclabel_l = NULL;
	SecLabel *seclabel_r = NULL;
	char	 *user_label = NULL;
	bool	  result = false;

	if (!global_on || !global_row_level) /* Bug - #z5940 */
		PG_RETURN_BOOL(true);

	user_label = get_obj_seclabel_internal(AuthIdRelationId, GetUserId(), InvalidOid, true);
	if (user_label)
		seclabel_l = string_to_seclabel(user_label, NULL, true);
	else
		seclabel_l = NULL;

	if (PG_ARGISNULL(0))
		seclabel_r = NULL;
	else
		seclabel_r = PG_GETARG_SECLABEL(0);

	result = (seclabel_compare(seclabel_l, seclabel_r) == LABEL_EQUAL);

	if (seclabel_l)
		pfree(seclabel_l);
	if (seclabel_r)
		pfree(seclabel_r);

	PG_RETURN_BOOL(result);
}

/*
 * Row-level MAC rules required for insert:
 * 	- INSERT: table_seclabel >= user_seclabel
 */
PG_FUNCTION_INFO_V1(mac_row_insert_policy);
Datum
mac_row_insert_policy(PG_FUNCTION_ARGS)
{
	SecLabel *seclabel_l = NULL;
	SecLabel *seclabel_r = NULL;
	char	 *user_label = NULL;
	char	 *table_label = NULL;
	bool	  result = false;

	if (!global_on || !global_row_level) /* Bug - #z5940 */
		PG_RETURN_BOOL(true);

	user_label = get_obj_seclabel_internal(AuthIdRelationId, GetUserId(), InvalidOid, true);
	if (user_label)
		seclabel_r = string_to_seclabel(user_label, NULL, true);
	else
		seclabel_r = NULL;

	if (PG_ARGISNULL(0))
		seclabel_l = NULL;
	else
	{
		table_label = get_obj_seclabel_internal(RelationRelationId, PG_GETARG_OID(0), InvalidOid, true);
		if (table_label)
			seclabel_l = string_to_seclabel(table_label, NULL, true);
		else
			seclabel_l = NULL;
	}

	result = (seclabel_compare(seclabel_l, seclabel_r) != LABEL_UNCOMPARABLE);

	if (seclabel_l)
		pfree(seclabel_l);
	if (seclabel_r)
		pfree(seclabel_r);

	PG_RETURN_BOOL(result);
}

/* End - ReqID: SRS-SECURITY-LMAC */
