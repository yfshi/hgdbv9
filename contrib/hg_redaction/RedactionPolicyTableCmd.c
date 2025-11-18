/*-------------------------------------------------------------------------
 *
 * File: RedactionPolicyCmd.c
 *
 * Abstract: support ALTER/DROP table, the table have redaction policy.
 *
 * Copyright:
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *			/contrib/hg_redaction/RedactionPolicyTableCmd.c
 *
 * ReqID:SRS-SECURITY-DATADESENSITIZATION
 *
 *-------------------------------------------------------------------------
 */

#include "hg_redaction.h"
#include "utils/regproc.h"

static Oid ddl_get_policytb_oid(char *tablename);
/*
 * get_policytb_oid
 *		Returns the oid of the relation.
 *
 * Returns the oid of the relation,
 * if the redaction extension exist.
 *
 * NOTE: if can't get the oid, need to check whether
 * the extension pg_redaction exists.or whether the
 * public schema exists.
 */
static Oid
ddl_get_policytb_oid(char *tablename)
{
	char	*rdpolicy;
	Oid		rdpolicyschemaoid = InvalidOid;
	Oid		policytboid = InvalidOid;

	rdpolicy = tablename;
	/*get the oid of public schema */
	rdpolicyschemaoid = SECURITY_SCHEMA_NAMESPACE;

	/* if public schema exists, the next step is get the oid of 
	 * hg_redaction_policy, otherwise report an error. 
	 */
	if (OidIsValid(rdpolicyschemaoid))
		policytboid = get_relname_relid(rdpolicy, rdpolicyschemaoid);

	return policytboid;
}

/*
 * CreateRedactionPolicy
 *		Returns the name of a data redaction policy.
 *
 * The return value is a string, the string is the
 * name of redaction.
 *
 * NOTE: Obtain redaction policy name according to redaction
 * policy oid.
 */
char *
Redacton_policyoid_get_policyname(Oid policy_id, Oid reloid, char *attname)
{
	SysScanDesc		sscan_p;
	bool			isNull;
	Datum			polnamedatum;
	char		   *polname = NULL;

	if (OidIsValid(policy_id))
	{
		Relation		policyrel;
		HeapTuple		policy_tuple;
		ScanKeyData		skey;
		
		policyrel = table_open(ddl_get_policytb_oid("hg_redaction_policy"), AccessShareLock);
		ScanKeyInit(&skey,
					Anum_hg_redaction_policy_polid,
					BTEqualStrategyNumber, F_OIDEQ,
					ObjectIdGetDatum(policy_id));

		sscan_p = systable_beginscan(policyrel,
									 get_policytbindex_oid(NULL, hg_redaciton_policy_oid_index),
									 true, NULL, 1, &skey);

		/* get the redaction policy name */
		policy_tuple = systable_getnext(sscan_p);
		if (HeapTupleIsValid(policy_tuple))
		{
			polnamedatum = heap_getattr(policy_tuple, Anum_hg_redaction_policy_polname,
										 RelationGetDescr(policyrel), &isNull);
			if (!isNull)
				polname = DatumGetCString(polnamedatum);
		}

		systable_endscan(sscan_p);
		table_close(policyrel, AccessShareLock);
	}
	else if (OidIsValid(reloid) && attname)
	{
		Relation		policycolrel;
		HeapTuple		policycol_tuple;
		ScanKeyData		skey[2];

		policycolrel = table_open(ddl_get_policytb_oid("hg_redaction_column"), AccessShareLock);

		ScanKeyInit(&skey[0],
					Anum_hg_redaction_column_tabrelid,
					BTEqualStrategyNumber, F_OIDEQ,
					ObjectIdGetDatum(reloid));
		ScanKeyInit(&skey[1],
				Anum_hg_redaction_column_attname,
				BTEqualStrategyNumber, F_NAMEEQ,
				CStringGetDatum(attname));
		sscan_p = systable_beginscan(policycolrel,
									 get_policytbindex_oid(NULL, hg_redaciton_column_tabrelid_attname_index),
									 true, NULL, 2, skey);

		policycol_tuple = systable_getnext(sscan_p);
		if (HeapTupleIsValid(policycol_tuple))
		{
			polnamedatum = heap_getattr(policycol_tuple, Anum_hg_redaction_column_polname,
										 RelationGetDescr(policycolrel), &isNull);
			if (!isNull)
				polname = DatumGetCString(polnamedatum);
		}

		systable_endscan(sscan_p);
		table_close(policycolrel, AccessShareLock);
	}
	else
		polname = NULL;

	return polname;
}

/*
 * RemoveRedactionPolicyById
 *		No values return.
 *
 * NOTE: Delete data redaction policy based on oid.
 */
void
RemoveRedactionPolicyById(Oid policy_id)
{
	Relation		policyrel, attrrel;
	HeapTuple		policy_tuple, attr_tuple;
	ScanKeyData		skey, skey2;
	SysScanDesc		sscan_p, sscan_a;

	policyrel = table_open(ddl_get_policytb_oid("hg_redaction_policy"), RowExclusiveLock);
	attrrel = table_open(get_policycoltb_oid(), RowExclusiveLock);

	/*
	 * Find related records in table hg_redaction_policy
	 * and delete.
	 */
	ScanKeyInit(&skey,
				Anum_hg_redaction_policy_polid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(policy_id));
	
	sscan_p = systable_beginscan(policyrel,
								 get_policytbindex_oid(NULL, hg_redaciton_policy_oid_index),
								 true, NULL, 1, &skey);

	policy_tuple = systable_getnext(sscan_p);
	if (HeapTupleIsValid(policy_tuple))
	{
		CatalogTupleDelete(policyrel, &policy_tuple->t_self);
	}
	else
	{
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				errmsg("could not find tuple for redaction policy %u",
						policy_id)));
	}

	systable_endscan(sscan_p);

	/*
	 * Find related records in table hg_redaction_column
	 * and delete.
	 */
	ScanKeyInit(&skey2,
				Anum_hg_redaction_column_polid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(policy_id));

	sscan_a = systable_beginscan(attrrel,
								 get_policytbindex_oid(NULL, hg_redaciton_column_polid_index), 
								 true, NULL, 1, &skey2);
	while (HeapTupleIsValid(attr_tuple = systable_getnext(sscan_a)))
	{
		CatalogTupleDelete(attrrel, &attr_tuple->t_self);
	}

	systable_endscan(sscan_a);
	table_close(policyrel, RowExclusiveLock);
	table_close(attrrel, RowExclusiveLock);

}

/*
 * redaction_getObjectDescription
 *		Returns a string, the string is the
 * prompt information.
 *
 * Return a string, which represents the prompt
 * information when cascading delete redaction
 * policy.
 *
 * NOTE: the string is a prompt information,
 * the information is 'drop cascades to
 * redaction policy policy_name'.
 */
char *
redaction_getObjectDescription(const ObjectAddress *object)
{
	Oid		policyoid = InvalidOid;

	policyoid = ddl_get_policytb_oid("hg_redaction_policy");
	if (OidIsValid(policyoid) && (policyoid == object->classId))
	{
		StringInfoData buffer;

		initStringInfo(&buffer);
		appendStringInfo(&buffer, _("redaction policy %s"),
							 Redacton_policyoid_get_policyname(object->objectId, InvalidOid, NULL));

		return buffer.data;
	}

	return NULL;
}

/*
 * redaction_do_deletion
 *		Returns a bool value.
 *
 * Return a bool value, if the deletion is successful,
 * return true, otherwise return false.
 *
 * NOTE: the function is a entry of delete record from
 * hg_redaction_policy and hg_redaction_column.
 */
bool
redaction_do_deletion(const ObjectAddress *object)
{
	Oid		policyoid = InvalidOid;

	policyoid = ddl_get_policytb_oid("hg_redaction_policy");
	if (OidIsValid(policyoid) && (policyoid == object->classId))
	{
		RemoveRedactionPolicyById(object->objectId);
		return true;
	}

	return false;
}

/*
 * Redaction_AlterObjectNamespace_oid
 *		Returns a bool value.
 *
 * If the dependency is table, return true,
 * otherwise return false.
 *
 * NOTE: Determin whether the dependent table
 * is a data redaction table.
 */
bool
Redaction_AlterObjectNamespace_oid(const ObjectAddress *object)
{
	Oid		policyoid = InvalidOid;

	policyoid = ddl_get_policytb_oid("hg_redaction_policy");

	if (object && OidIsValid(policyoid) && (policyoid == object->classId))
		return true;

	return false;
}

/*
 * Redaction_GetObjectTypeDescription
 *		Returns a string.
 *
 * The returned string is a prompt message.
 *
 * NOTE: If it depends on table hg_redaction_policy,
 * give a prompt message.
 */
char *
Redaction_GetObjectTypeDescription(const ObjectAddress *object)
{
	Oid		policyoid = InvalidOid;

	policyoid = ddl_get_policytb_oid("hg_redaction_policy");

	if (object && OidIsValid(policyoid) && (policyoid == object->classId))
	{
		StringInfoData buffer;

		initStringInfo(&buffer);
		appendStringInfoString(&buffer, "redactionpolicy");
		
		return buffer.data;
	}

	return NULL;
}

/*
 * Redaction_getObjectIdentityParts
 *		Returns a string.
 *
 * The returned string is a prompt message.
 *
 * NOTE: If it depends on table hg_redaction_policy,
 * give a prompt message.
 */
char *
Redaction_getObjectIdentityParts(const ObjectAddress *object, List **objname)
{
	Oid		policyoid = InvalidOid;

	policyoid = ddl_get_policytb_oid("hg_redaction_policy");

	if (object && OidIsValid(policyoid) && (policyoid == object->classId))
	{
		char		   *subname = "objaddress redaction policy";
		StringInfoData	buffer;

		if (objname)
			*objname = list_make1(pstrdup(subname));

		initStringInfo(&buffer);
		appendStringInfo(&buffer, "data redaction policy %s",
							 Redacton_policyoid_get_policyname(object->objectId, InvalidOid, NULL));

		if (objname && *objname == NIL)
			elog(ERROR, "requested object address for unsupported object class %d: text result \"%s\"",
					object->classId, buffer.data);

		if (buffer.len == 0)
			return NULL;

		return buffer.data;
	}

	return NULL;
}

/*
 * Redaction_ATExecAlterColumnType
 *		No returns value.
 *
 * NOTE: Modify message when modifying a data table
 * with data redaction policy.
 * type is 1:alter column type;
 * type is 2:rename column name;
 * type is 3: drop column.
 */
void 
Redaction_ATExecAlterTable(Oid tableoid, char *colName, int type)
{
	char	   *policyname = NULL;

	policyname = Redacton_policyoid_get_policyname(InvalidOid, tableoid, colName);
	if (policyname)
	{
		if (type == 1)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("cannot alter type of a column used in a redaction policy"),
					 errdetail("redaction policy %s depends on column \"%s\"",
							   policyname, colName)));
		if (type == 2)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("cannot rename column used in a redaction policy"),
					 errdetail("redaction policy %s depends on column \"%s\"",
							   policyname, colName)));
		if (type == 3)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("cannot drop column used in a redaction policy"),
					 errdetail("redaction policy %s depends on column \"%s\"",
							   policyname, colName)));
	}	
}

/*
 * Redaction_AlterRelationNamespaceInternal
 *		No returns value.
 *
 * NOTE: When modify table namespace,
 * need to modify redaction information.
 *
 */
void 
Redaction_AlterRelationNamespaceInternal(Oid relOid, Oid oldNspOid, Oid newNspOid)
{
	Relation		policyrel;
	ScanKeyData		skey[1];
	SysScanDesc 	sscan_p;
	HeapTuple		tuple, policy_newtuple;
	Datum			policy_val[Natts_hg_redaction_policy];
	bool			policy_null[Natts_hg_redaction_policy];
	bool			policy_repl[Natts_hg_redaction_policy];
	char		   *namespace = NULL;

	namespace = get_namespace_name(newNspOid);
	if (!namespace)
		return;

	policyrel = table_open(get_policytb_oid(), RowExclusiveLock);

	ScanKeyInit(&skey[0],
				Anum_hg_redaction_policy_tabrelid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(relOid));

	sscan_p = systable_beginscan(policyrel,
									get_policytbindex_oid(NULL, hg_redaciton_table_oid_index),
									true, NULL, 1, skey);
	
	while (HeapTupleIsValid(tuple = systable_getnext(sscan_p)))
	{
		memset(policy_null, false, sizeof(policy_null));
		memset(policy_repl, false, sizeof(policy_repl));
		memset(policy_val, false, sizeof(policy_val));

		policy_val[Anum_hg_redaction_policy_schema - 1] = DirectFunctionCall1(namein,
											CStringGetDatum(namespace));
		policy_null[Anum_hg_redaction_policy_schema - 1] = false;
		policy_repl[Anum_hg_redaction_policy_schema -1] = true;

		policy_newtuple = heap_modify_tuple(tuple, RelationGetDescr(policyrel),
								policy_val, policy_null, policy_repl);

		/* Update row record */
		CatalogTupleUpdate(policyrel, &policy_newtuple->t_self, policy_newtuple);

		heap_freetuple(policy_newtuple);
	}

	systable_endscan(sscan_p);
	table_close(policyrel, RowExclusiveLock);
}

bool
judge_redaction_tb(Oid oid)
{
	char *relname = NULL;

	if (OidIsValid(oid))
		relname = get_rel_name(oid);
	else
		return false;
	
	if (relname != NULL && strcmp(relname , "hg_redaction_policy") == 0)
		return true;

	return false;
}

/* hg_redataction_rel_access_restrictions
 *
 * With all three privileges enabled, only user syssso can
 * query tables and views related to hg_redaction, with all
 * three privileges disabled, only the supperuser can query
 * table and views related to hg_redaction.
 */
bool
hg_redataction_rel_access_restrictions(List *tbnames, bool flg)
{
	ListCell	*cell;
	Oid			redactionpoid = InvalidOid;
	Oid			redactioncoid = InvalidOid;
	Oid			redactionvoid = InvalidOid;
	Oid			redactionfoid = InvalidOid;

	if (prev_ExecutorCheckPerms_hook &&
		!(*prev_ExecutorCheckPerms_hook) (tbnames, flg))
		return false;

	/* get table and view oid */
	redactionpoid = ddl_get_policytb_oid("hg_redaction_policy");
	redactioncoid = ddl_get_policytb_oid("hg_redaction_column");
	redactionvoid = ddl_get_policytb_oid("hg_redaction");
	redactionfoid = ddl_get_policytb_oid("hg_redaction_function");

	if (!OidIsValid(redactionpoid) ||!OidIsValid(redactioncoid) ||
		!OidIsValid(redactionvoid) || !OidIsValid(redactionfoid))
		return true;

	foreach(cell, tbnames)
	{
		RangeTblEntry *rte = lfirst(cell);
		if (rte->rtekind == RTE_RELATION)
		{
			char 	*reltype = NULL;
			char	*relname = NULL;

			if (rte->relid == redactionpoid || rte->relid == redactioncoid)
			{
				reltype = "table";
				if (rte->relid == redactionpoid)
					relname = "hg_redaction_policy";
				else
					relname = "hg_redaction_column";
			}
			else if (rte->relid == redactionvoid || rte->relid == redactionfoid)
			{
				reltype = "view";
				if (rte->relid == redactionvoid)
					relname = "hg_redaction";
				else
					relname = "hg_redaction_function";
			}
			else
				return true;

			if (!IS_SYSSSO(GetUserId()))
			{
				if (rte->relid == redactionpoid || rte->relid == redactioncoid ||
					rte->relid == redactionvoid || rte->relid == redactionfoid)
				{
					ereport(ERROR,
							(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							 errmsg("permission denied for %s %s.",
							 reltype, relname)));
				}
			}
		}
	}

	return true;
}
