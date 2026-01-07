/*-------------------------------------------------------------------------
 *
 * File: RedactionPolicyCmd.c 
 *
 * Abstract: support redaction policy create/alter/drop.
 *
 * Copyright:
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *			/contrib/hg_redaction/RedactionPolicyCmd.c
 *
 * ReqID:SRS-SECURITY-DATADESENSITIZATION
 *
 *-------------------------------------------------------------------------
 */
#include "hg_redaction.h"
#include "utils/regproc.h"
#include "commands/defrem.h"
#include "nodes/makefuncs.h"
#include "parser/parse_func.h"


PG_FUNCTION_INFO_V1(all_redaction_policy_num);

static void RangeVarCallbackForRedactionPolicy(const RangeVar *rv, Oid relid, Oid oldrelid,
															void *arg);
Oid get_procid_from_procname(char *procname);
static void DropRedactionPolicy(DropRedactionPolicyStmt *stmt);
static void RenameRedactionPolicy(AlterRedactionPolicyStmt *stmt);
static void RenameRedactioncolumn(AlterRedactionPolicyStmt *stmt);
static void FilterRedactionPolicy(AlterRedactionPolicyStmt *stmt);
static void StatusRedactionPolicy(AlterRedactionPolicyStmt *stmt);
static void AddRedactionColumn(AlterRedactionPolicyStmt *stmt);
static void ModifyRedactionPolicy(AlterRedactionPolicyStmt *stmt);
static void DropColumnRedactionColumn(AlterRedactionPolicyStmt *stmt);
static void AlterRedactionPolicy(AlterRedactionPolicyStmt *stmt);
static void CreateRedactionPolicy(CreateRedactionPolicyStmt *stmt);
static void define_hg_redaction_column(Oid policy_id, char *polname, Oid table_id,
												 Relation target_table, List *colfunc_list);
Oid hg_GetNewOidWithIndex(Relation relation, Oid indexId, AttrNumber oidcolumn);
Datum
redatcion_transformRelOptions(Datum oldOptions, List *defList, const char *namspace,
					char *validnsps[], bool acceptOidsOff, bool isReset);
DefElem *
redatcion_makeDefElem(char *name, Node *arg, int location);
static void check_table_col(Relation rel, char *colname);
static char * get_table_schema(Oid reloid);
static List * get_redaction_func_name(char *defname);	/* BUG#Z6485 */

/*
 * get_policytb_oid
 *		Returns the oid of the relation hg_redaction_policy.
 *
 * Returns the oid of the relation hg_redaction_policy,
 * if the redactiom redaction exist.
 *
 * NOTE: if can't get the oid, need to check whether
 * the extension pg_redaction exists.or whether the
 * hg_security schema exists.
 */
Oid
get_policytb_oid(void)
{
	char	*rdpolicy = "hg_redaction_policy";
	Oid		rdpolicyschemaoid = InvalidOid;
	Oid		policytboid = InvalidOid;

	/*get the oid of hg_security schema */
	rdpolicyschemaoid = SECURITY_SCHEMA_NAMESPACE;

	/* if hg_security schema exists, the next step is get the oid of 
	 * hg_redaction_policy, otherwise report an error. 
	 */
	if (OidIsValid(rdpolicyschemaoid))
		policytboid = GetSysCacheOid2(RELNAMENSP, Anum_pg_class_oid,
									  PointerGetDatum(rdpolicy),
									  ObjectIdGetDatum(rdpolicyschemaoid));
	else
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_SCHEMA),
				 errmsg("schema hg_security does not exist")));

	/* if the oid of the hg_redaction_policy is invalid, it 
	 * will report an error.
	 */
	if (!policytboid)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_TABLE),
				 errmsg("table \"%s\" does not exist, maybe you need to create extension hg_redaction", rdpolicy)));

	return policytboid;
}

/*
 * get_policycoltb_oid
 *		Returns the oid of the relation get_policycoltb_oid.
 *
 * Returns the oid of the relation hg_redaction_column,
 * if the redactiom redaction exist.
 *
 * NOTE: if can't get the oid, need to check whether
 * the extension pg_redaction exists. or whether the
 * hg_security schema exists.
 */
Oid
get_policycoltb_oid(void)
{
	char	*rdpolicycol = "hg_redaction_column";
	Oid		rdpolicyschemaoid = InvalidOid;
	Oid		policytbcoloid = InvalidOid;

	/*get the oid of hg_security schema */
	//rdpolicyschemaoid = LookupExplicitNamespace(rdpolicyschema, true);
	rdpolicyschemaoid = SECURITY_SCHEMA_NAMESPACE;

	/* if hg_security schema exists, the next step is get the oid
	 * of hg_redaction_policy, otherwise report an error.
	 */
	if (OidIsValid(rdpolicyschemaoid))
		policytbcoloid = get_relname_relid(rdpolicycol, rdpolicyschemaoid);
	else
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_SCHEMA),
				 errmsg("schema hg_security does not exist")));

	/* if the oid of the hg_redaction_policy is invalid,
	 * it will report an error.
	 */
	if (!policytbcoloid)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_TABLE),
				 errmsg("table \"%s\" does not exist, maybe you need to create extension hg_redaction",
					rdpolicycol)));

	return policytbcoloid;
}

/*
 * get_policycoltb_oid
 *		Returns the oid of the relation get_policycoltb_oid.
 *
 * Returns the oid of the relation hg_redaction_column,
 * if the redactiom redaction exist.
 *
 * NOTE: if can't get the oid, need to check whether the
 * extension pg_redaction exists. or whether the hg_security
 * schema exists.
 */
Oid
get_policytbindex_oid(char *indexschema, char *indexneam)
{
	Oid		rdindexschemaoid = InvalidOid;
	Oid		rdindexoid = InvalidOid;

	/* get the oid of hg_security schema or the name of indexschema */
	if (!indexschema)
		rdindexschemaoid = SECURITY_SCHEMA_NAMESPACE;
	else
		rdindexschemaoid = LookupExplicitNamespace(indexschema, true);

	/* if the schema exists, the next step is get the oid of indexneam,
	 * otherwise report an error.
	 */
	if (OidIsValid(rdindexschemaoid))
		rdindexoid = get_relname_relid(indexneam, rdindexschemaoid);
	else
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_SCHEMA),
				 errmsg("schema \"%s\" does not exist", indexschema)));

	/* if the oid of the index is invalid, 
	 * it will report an error.
	 */
	if (!OidIsValid(rdindexoid))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_TABLE),
				 errmsg("index \"%s\" does not exist", indexneam)));

	return rdindexoid;
}

/*
 * get_table_schema
 *		Return name of table namespace.
 *
 * Get the name of namespace through the oid of the table.
 *
 */
static char *
get_table_schema(Oid reloid)
{
	ScanKeyData 	skey[1];
	SysScanDesc 	sscan_p;
	HeapTuple		tuple;
	Relation		rel;
	bool			toptions_list_isnull;
	Datum			datum;
	Oid				schemaoid = InvalidOid;
	char		   *schemaname = NULL;

	rel = table_open(RelationRelationId, AccessShareLock);

	ScanKeyInit(&skey[0],
				Anum_pg_class_oid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(reloid));

	sscan_p = systable_beginscan(rel,
									ClassOidIndexId,
									true, NULL, 1, skey);
	tuple = systable_getnext(sscan_p);

	/*
	 * The redaction policy name of the same table is unique,
	 * and different tables can have the same redaction policy name.
	 */
	if (HeapTupleIsValid(tuple))
	{
		datum = heap_getattr(tuple, Anum_pg_class_relnamespace, 
								RelationGetDescr(rel), &toptions_list_isnull);
		schemaoid = DatumGetObjectId(datum);
	}

	if (OidIsValid(schemaoid))
		schemaname = get_namespace_name(schemaoid);

	systable_endscan(sscan_p);
	relation_close(rel, AccessShareLock);

	return schemaname;
}

/*
 * Transform a relation options list (list of DefElem) into the text array
 * format that is kept in pg_class.reloptions, including only those options
 * that are in the passed namespace.  The output values do not include the
 * namespace.
 *
 * This is used for three cases: CREATE TABLE/INDEX, ALTER TABLE SET, and
 * ALTER TABLE RESET.  In the ALTER cases, oldOptions is the existing
 * reloptions value (possibly NULL), and we replace or remove entries
 * as needed.
 *
 * If acceptOidsOff is true, then we allow oids = false, but throw error when
 * on. This is solely needed for backwards compatibility.
 *
 * Note that this is not responsible for determining whether the options
 * are valid, but it does check that namespaces for all the options given are
 * listed in validnsps.  The NULL namespace is always valid and need not be
 * explicitly listed.  Passing a NULL pointer means that only the NULL
 * namespace is valid.
 *
 * Both oldOptions and the result are text arrays (or NULL for "default"),
 * but we declare them as Datums to avoid including array.h in reloptions.h.
 */
Datum
redatcion_transformRelOptions(Datum oldOptions, List *defList, const char *namspace,
					char *validnsps[], bool acceptOidsOff, bool isReset)
{
	Datum		result;
	ArrayBuildState *astate;
	ListCell   *cell;

	/* no change if empty list */
	if (defList == NIL)
		return oldOptions;

	/* We build new array using accumArrayResult */
	astate = NULL;

	/* Copy any oldOptions that aren't to be replaced */
	if (PointerIsValid(DatumGetPointer(oldOptions)))
	{
		ArrayType  *array = DatumGetArrayTypeP(oldOptions);
		Datum	   *oldoptions;
		int			noldoptions;
		int			i;

		deconstruct_array(array, TEXTOID, -1, false, TYPALIGN_INT,
						  &oldoptions, NULL, &noldoptions);

		for (i = 0; i < noldoptions; i++)
		{
			char	   *text_str = VARDATA(oldoptions[i]);
			int			text_len = VARSIZE(oldoptions[i]) - VARHDRSZ;

			/* Search for a match in defList */
			foreach(cell, defList)
			{
				DefElem    *def = (DefElem *) lfirst(cell);
				int			kw_len;

				/* ignore if not in the same namespace */
				if (namspace == NULL)
				{
					if (def->defnamespace != NULL)
						continue;
				}
				else if (def->defnamespace == NULL)
					continue;
				else if (strcmp(def->defnamespace, namspace) != 0)
					continue;

				kw_len = strlen(def->defname);
				if (text_len > kw_len && text_str[kw_len] == '=' &&
					strncmp(text_str, def->defname, kw_len) == 0)
					break;
			}
			if (!cell)
			{
				/* No match, so keep old option */
				astate = accumArrayResult(astate, oldoptions[i],
										  false, TEXTOID,
										  CurrentMemoryContext);
			}
		}
	}

	/*
	 * If CREATE/SET, add new options to array; if RESET, just check that the
	 * user didn't say RESET (option=val).  (Must do this because the grammar
	 * doesn't enforce it.)
	 */
	foreach(cell, defList)
	{
		DefElem    *def = (DefElem *) lfirst(cell);

		if (isReset)
		{
			if (def->arg != NULL)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("RESET must not include values for parameters")));
		}
		else
		{
			text	   *t;
			const char *value;
			Size		len;

			/*
			 * Error out if the namespace is not valid.  A NULL namespace is
			 * always valid.
			 */
			if (def->defnamespace != NULL)
			{
				bool		valid = false;
				int			i;

				if (validnsps)
				{
					for (i = 0; validnsps[i]; i++)
					{
						if (strcmp(def->defnamespace, validnsps[i]) == 0)
						{
							valid = true;
							break;
						}
					}
				}

				if (!valid)
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
							 errmsg("unrecognized parameter namespace \"%s\"",
									def->defnamespace)));
			}

			/* ignore if not in the same namespace */
			if (namspace == NULL)
			{
				if (def->defnamespace != NULL)
					continue;
			}
			else if (def->defnamespace == NULL)
				continue;
			else if (strcmp(def->defnamespace, namspace) != 0)
				continue;

			/*
			 * Flatten the DefElem into a text string like "name=arg". If we
			 * have just "name", assume "name=true" is meant.  Note: the
			 * namespace is not output.
			 */
			if (def->arg != NULL)
				value = defGetString(def);
			else
				value = "true";

			/*
			 * This is not a great place for this test, but there's no other
			 * convenient place to filter the option out. As WITH (oids =
			 * false) will be removed someday, this seems like an acceptable
			 * amount of ugly.
			 */
			if (acceptOidsOff && def->defnamespace == NULL &&
				strcmp(def->defname, "oids") == 0)
			{
				if (defGetBoolean(def))
					ereport(ERROR,
							(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							 errmsg("tables declared WITH OIDS are not supported")));
				/* skip over option, reloptions machinery doesn't know it */
				continue;
			}

			len = VARHDRSZ + strlen(def->defname) + 1 + strlen(value);
			/* +1 leaves room for sprintf's trailing null */
			if (def->defaction == DEFELEM_SET)
			{
				t = (text *) palloc(len + 2);
				SET_VARSIZE(t, len+1);
				sprintf(VARDATA(t), "%s!=%s", def->defname, value);
			}
			else
			{
				t = (text *) palloc(len + 1);
				SET_VARSIZE(t, len);
				sprintf(VARDATA(t), "%s=%s", def->defname, value);
			}

			astate = accumArrayResult(astate, PointerGetDatum(t),
									  false, TEXTOID,
									  CurrentMemoryContext);
		}
	}

	if (astate)
		result = makeArrayResult(astate, CurrentMemoryContext);
	else
		result = (Datum) 0;

	return result;
}

/*
 * redatcion_makeDefElem -
 *	build a DefElem node
 *
 * This is sufficient for the "typical" case with an unqualified option name
 * and no special action.
 */
DefElem *
redatcion_makeDefElem(char *name, Node *arg, int location)
{
	DefElem    *res = makeNode(DefElem);

	res->defnamespace = NULL;
	res->defname = name;
	res->arg = arg;
	res->defaction = DEFELEM_SET;
	res->location = location;

	return res;
}

/*
 * Convert the text-array format of reloptions into a List of DefElem.
 * This is the inverse of transformRelOptions().
 */
List *
redatcion_untransformRelOptions(Datum options)
{
	List	   *result = NIL;
	ArrayType  *array;
	Datum	   *optiondatums;
	int			noptions;
	int			i;

	/* Nothing to do if no options */
	if (!PointerIsValid(DatumGetPointer(options)))
		return result;

	array = DatumGetArrayTypeP(options);

	deconstruct_array(array, TEXTOID, -1, false, TYPALIGN_INT,
					  &optiondatums, NULL, &noptions);

	for (i = 0; i < noptions; i++)
	{
		char	   *s;
		char	   *p;
		Node	   *val = NULL;
		char	   *news;

		s = TextDatumGetCString(optiondatums[i]);
		news = pstrdup(s);

		p = strstr(s, "!=");
		if (p)
		{
			*p++ = '\0';
			if (strchr(pstrdup(s), ',') || strchr(pstrdup(s), '='))
			{
				p = strchr(news, '=');
				if (p)
				{
					*p++ = '\0';
					val = (Node *) makeString(pstrdup(p));
				}
				result = lappend(result, makeDefElem(pstrdup(news), val, -1));
			}
			else
			{
				val = (Node *) makeString(pstrdup(p+1));
				result = lappend(result, redatcion_makeDefElem(pstrdup(s), val, -1));
			}
		}
		else
		{
			p = strchr(s, '=');
			if (p)
			{
				*p++ = '\0';
				val = (Node *) makeString(pstrdup(p));
			}
			result = lappend(result, makeDefElem(pstrdup(s), val, -1));
		}
		
	}

	return result;
}

/*
 * ProcessUtility hook
 */
/*
 * hg_redact_ProcessUtility
 *		no value return.
 *
 * NOTE: deal with ProcessUtility hook.
 */
bool
hg_redact_ProcessUtility(PlannedStmt *pstmt, const char *queryString,
					bool readOnlyTree,
					ProcessUtilityContext context,
					ParamListInfo params, QueryEnvironment *queryEnv,
					DestReceiver *dest, QueryCompletion *qc)
{
	Node	   *parsetree;
	bool		rdflg = false;

	/* This can recurse, so check for excessive recursion */
	check_stack_depth();

	/*
	 * If the given node tree is read-only, make a copy to ensure that parse
	 * transformations don't damage the original tree.	This could be
	 * refactored to avoid making unnecessary copies in more cases, but it's
	 * not clear that it's worth a great deal of trouble over.	Statements
	 * that are complex enough to be expensive to copy are exactly the ones
	 * we'd need to copy, so that only marginal savings seem possible.
	 */
	if (readOnlyTree)
		pstmt = copyObject(pstmt);
	parsetree = pstmt->utilityStmt;

	if (hg_prev_redaction_ProcessUtility)
		hg_prev_redaction_ProcessUtility(pstmt, queryString, readOnlyTree,
							context, params, queryEnv,
							dest, qc);

	/* create/alter/drop data redaction policy. */
	if (nodeTag(parsetree) == T_CreateRedactionPolicyStmt ||
		nodeTag(parsetree) == T_AlterRedactionPolicyStmt ||
		nodeTag(parsetree) == T_DropRedactionPolicyStmt)
	{
		/* if the GUC parameter hg_redaction_data is off,
		 * an error will be thrown.
		 */
		if (hg_redaction_data == true)
		{
			if (nodeTag(parsetree) == T_CreateRedactionPolicyStmt)
				CreateRedactionPolicy((CreateRedactionPolicyStmt *) parsetree);
			else if (nodeTag(parsetree) == T_AlterRedactionPolicyStmt)
				AlterRedactionPolicy((AlterRedactionPolicyStmt *) parsetree);
			else
				DropRedactionPolicy((DropRedactionPolicyStmt *) parsetree);
		}
		else
		{
			ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					errmsg("hg_redaction.enable already off"),
					errhint("hg_redaction.enable already off, maybe you should set hg_redaction.enable = on;")));
		}

		rdflg = true;
	}

	return rdflg;
}

/*
 * DropRedactionPolicy
 *		Returns the ObjectAddress of the drop redaction.
 *
 * drop data redaction.
 *
 */
static void
DropRedactionPolicy(DropRedactionPolicyStmt *stmt)
{
	Relation		policyrel, attrrel;
	HeapTuple		policy_tuple, attr_tuple;
	ScanKeyData 	skey[2], skey2[2];
	SysScanDesc 	sscan_p, sscan_a;
	ListCell		*item;
	Oid 			policyid = InvalidOid;
	bool			isNull;
	Datum			poloiddatum;
	Oid				table_id;

	/* Begin - BUG#Z6440 */
	if (!IS_SYSSSO(GetUserId()))
	{
		char	   *username = NULL;

		if (se_feature_mode)
		{
			username = "syssso";
		}
		else
		{
			username = "superuser";
		}

		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("permission denied to drop redacation policy."),
				 errhint("Must be %s to drop a redacation policy.", username)));
	}
	/* End - BUG#Z6440 */

	table_id = RangeVarGetRelidExtended(stmt->table_name, AccessShareLock,
										0,
										RangeVarCallbackForRedactionPolicy,
										(void *) stmt);
	policyrel = table_open(get_policytb_oid(), RowExclusiveLock);
	attrrel = table_open(get_policycoltb_oid(), RowExclusiveLock);

	foreach(item, stmt->policyname_list)
	{
		char			*polname;
		polname = strVal(lfirst(item));

		ScanKeyInit(&skey[0],
					Anum_hg_redaction_policy_tabrelid,
					BTEqualStrategyNumber, F_OIDEQ,
					ObjectIdGetDatum(table_id));
		ScanKeyInit(&skey[1],
					Anum_hg_redaction_policy_polname,
					BTEqualStrategyNumber, F_NAMEEQ,
					CStringGetDatum(polname));

		sscan_p = systable_beginscan(policyrel, get_policytbindex_oid(NULL, hg_redaciton_policy_tabrelid_polname_index), 
								true, NULL, 2, skey);
		policy_tuple = systable_getnext(sscan_p);
		if (HeapTupleIsValid(policy_tuple))
		{
			poloiddatum = heap_getattr(policy_tuple, Anum_hg_redaction_policy_polid, RelationGetDescr(policyrel), &isNull);
			policyid = (Oid)DatumGetObjectId(poloiddatum);

			CatalogTupleDelete(policyrel, &policy_tuple->t_self);
		}
		else
		{
			if (stmt->if_exists)
			{
				ereport(NOTICE,
					(errmsg("redaction policy \"%s\" does not exist, skipping",
							polname)));
			}
			else
				ereport(ERROR,
						(errcode(ERRCODE_UNDEFINED_OBJECT),
						errmsg("policy_name \"%s\" on %s.%s does not exist",
								polname, stmt->table_name->schemaname, stmt->table_name->relname)));
		}

		systable_endscan(sscan_p);

		ScanKeyInit(&skey2[0],
					Anum_hg_redaction_column_polname,
					BTEqualStrategyNumber, F_NAMEEQ,
					CStringGetDatum(polname));
		ScanKeyInit(&skey2[1],
					Anum_hg_redaction_column_tabrelid,
					BTEqualStrategyNumber, F_OIDEQ,
					CStringGetDatum(table_id));

		sscan_a = systable_beginscan(attrrel, get_policytbindex_oid(NULL, hg_redaciton_column_polname_attname_index), 
									 true, NULL, 2, skey2);
		while (HeapTupleIsValid(attr_tuple = systable_getnext(sscan_a)))
		{
			CatalogTupleDelete(attrrel, &attr_tuple->t_self);
		}

		systable_endscan(sscan_a);

		/* Drop the dependency of data redaction policy */
		deleteDependencyRecordsFor(get_policytb_oid(), policyid, true);
	}

	table_close(policyrel, RowExclusiveLock);
	table_close(attrrel, RowExclusiveLock);
}

/*
 * RenameRedactionPolicy
 *		no value return.
 *
 * NOTE: Modify the name of the data redaction policy
 * in table hg_redaction_policy.
 */
static void
RenameRedactionPolicy(AlterRedactionPolicyStmt *stmt)
{
	Relation		policyrel;
	HeapTuple		policy_tuple, policy_newtuple;
	ScanKeyData		skey[2];
	SysScanDesc		sscan;
	Oid				rel_id;
	char		   *polname = NULL;
	Datum			pol_val[Natts_hg_redaction_policy];
	bool			pol_null[Natts_hg_redaction_policy];
	bool			pol_repl[Natts_hg_redaction_policy];
	
	polname = stmt->policy_newname;

	policyrel = table_open(get_policytb_oid(), RowExclusiveLock);

	/*
	 * Find related records based on the oid of the
	 * table and the name of the data redaction pilicy.
	 */
	rel_id = RangeVarGetRelidExtended(stmt->table_name, AccessShareLock,
										0,
										RangeVarCallbackForRedactionPolicy,
										(void *) stmt);

	ScanKeyInit(&skey[0],
				Anum_hg_redaction_policy_tabrelid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(rel_id));
	ScanKeyInit(&skey[1],
				Anum_hg_redaction_policy_polname,
				BTEqualStrategyNumber, F_NAMEEQ,
				CStringGetDatum(stmt->policy_name));

	sscan = systable_beginscan(policyrel,
							   get_policytbindex_oid(NULL, hg_redaciton_policy_tabrelid_polname_index), true, NULL, 2, skey);

	policy_tuple = systable_getnext(sscan);

	/*
	 * If this data redaction policy does not exist on the
	 * hg_redaction_policy, an error will be thrown.
	 */
	if (!HeapTupleIsValid(policy_tuple))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				errmsg("policy_name \"%s\" does not exist",
						stmt->policy_name)));

	memset(pol_null, false, sizeof(pol_null));
	memset(pol_repl, false, sizeof(pol_repl));
	memset(pol_val, false, sizeof(pol_val));

	/* Fill modified value, create a modify tuple */
	if (polname != (Datum) 0)
		pol_val[Anum_hg_redaction_policy_polname - 1] = DirectFunctionCall1(namein,
													CStringGetDatum(polname));
	else
		pol_null[Anum_hg_redaction_policy_polname - 1] = true;	
	pol_repl[Anum_hg_redaction_policy_polname - 1] = true;

	policy_newtuple = heap_modify_tuple(policy_tuple, RelationGetDescr(policyrel),
							pol_val, pol_null, pol_repl);

	/* Update row record */
	CatalogTupleUpdate(policyrel, &policy_newtuple->t_self, policy_newtuple);

	heap_freetuple(policy_newtuple);

	systable_endscan(sscan);
	table_close(policyrel, RowExclusiveLock);
}

/*
 * RenameRedactioncolumn
 *		no value return.
 *
 * NOTE: Modify the name of the data redaction policy
 * in table hg_redaction_column.
 */
static void
RenameRedactioncolumn(AlterRedactionPolicyStmt *stmt)
{
	Relation		attrrel;
	HeapTuple		attr_tuple,attr_newtuple;
	ScanKeyData		skey[2];
	Oid				table_id;
	SysScanDesc		sscan;
	char		   *polname = NULL;
	Datum			attr_val[Natts_hg_redaction_column];
	bool			attr_null[Natts_hg_redaction_column];
	bool			attr_repl[Natts_hg_redaction_column];

	polname = stmt->policy_newname;

	attrrel = table_open(get_policycoltb_oid(), RowExclusiveLock);

	/*
	 * Find related records based on the oid of the
	 * table and the name of the data redaction pilicy.
	 */
	table_id = RangeVarGetRelidExtended(stmt->table_name, AccessShareLock,
										0,
										RangeVarCallbackForRedactionPolicy,
										(void *) stmt);

	ScanKeyInit(&skey[0],
				Anum_hg_redaction_column_polname,
				BTEqualStrategyNumber, F_NAMEEQ,
				CStringGetDatum(stmt->policy_name));
	ScanKeyInit(&skey[1],
				Anum_hg_redaction_policy_tabrelid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(table_id));

	sscan = systable_beginscan(attrrel,
							   get_policytbindex_oid(NULL, hg_redaciton_column_polname_attname_index), true, NULL, 2, skey);

	while (HeapTupleIsValid(attr_tuple = systable_getnext(sscan)))
	{
		memset(attr_null, false, sizeof(attr_null));
		memset(attr_repl, false, sizeof(attr_repl));
		memset(attr_val, false, sizeof(attr_val));

		/* Fill modified value, create a modify tuple */
		if (polname != (Datum) 0)
			attr_val[Anum_hg_redaction_column_polname - 1] = DirectFunctionCall1(namein,
													CStringGetDatum(polname));
		else
			attr_null[Anum_hg_redaction_column_polname - 1] = true;	
		attr_repl[Anum_hg_redaction_column_polname - 1] = true;

		attr_newtuple = heap_modify_tuple(attr_tuple, RelationGetDescr(attrrel),
								attr_val, attr_null, attr_repl);

		/* Update row record */
		CatalogTupleUpdate(attrrel, &attr_newtuple->t_self, attr_newtuple);

		heap_freetuple(attr_newtuple);
	}

	systable_endscan(sscan);
	table_close(attrrel, RowExclusiveLock);

}

/*
 * FilterRedactionPolicy
 *		no value return.
 *
 * NOTE: Modify the for information of the data redaction policy
 * in table hg_redaction_policy.
 */
static void
FilterRedactionPolicy(AlterRedactionPolicyStmt *stmt)
{
	Relation		policyrel;
	HeapTuple		policy_tuple, policy_newtuple;
	ScanKeyData		skey[2];
	SysScanDesc		sscan;
	Oid				rel_id;
	Datum			newOptions;
	Datum			pol_val[Natts_hg_redaction_policy];
	bool			pol_null[Natts_hg_redaction_policy];
	bool			pol_repl[Natts_hg_redaction_policy];
	
	policyrel = table_open(get_policytb_oid(), RowExclusiveLock);

	/*
	 * Find related records based on the oid of the
	 * table and the name of the data redaction pilicy.
	 */
	rel_id = RangeVarGetRelidExtended(stmt->table_name, AccessShareLock,
										0,
										RangeVarCallbackForRedactionPolicy,
										(void *) stmt);

	ScanKeyInit(&skey[0],
				Anum_hg_redaction_policy_tabrelid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(rel_id));
	ScanKeyInit(&skey[1],
				Anum_hg_redaction_policy_polname,
				BTEqualStrategyNumber, F_NAMEEQ,
				CStringGetDatum(stmt->policy_name));

	sscan = systable_beginscan(policyrel,
							   get_policytbindex_oid(NULL, hg_redaciton_policy_tabrelid_polname_index), true, NULL, 2, skey);

	policy_tuple = systable_getnext(sscan);

	/*
	 * If this data redaction policy does not exist on the
	 * hg_redaction_policy, an error will be thrown.
	 */
	if (!HeapTupleIsValid(policy_tuple))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				errmsg("policy_name \"%s\" does not exist",
						stmt->policy_name)));

	memset(pol_null, false, sizeof(pol_null));
	memset(pol_repl, false, sizeof(pol_repl));
	memset(pol_val, false, sizeof(pol_val));

	/* parser for clause */
	newOptions = redatcion_transformRelOptions((Datum) 0,
									 stmt->options,
									 NULL, NULL, false, false);

	/* Fill modified value, create a modify tuple */
	if (newOptions != (Datum) 0)
		pol_val[Anum_hg_redaction_policy_poloptions - 1] = newOptions;
	else
		pol_null[Anum_hg_redaction_policy_poloptions - 1] = true;

	pol_repl[Anum_hg_redaction_policy_poloptions - 1] = true;

	policy_newtuple = heap_modify_tuple(policy_tuple, RelationGetDescr(policyrel),
							pol_val, pol_null, pol_repl);

	/* Update row record */
	CatalogTupleUpdate(policyrel, &policy_newtuple->t_self, policy_newtuple);

	heap_freetuple(policy_newtuple);

	systable_endscan(sscan);
	table_close(policyrel, RowExclusiveLock);
}

/*
 * StatusRedactionPolicy
 *		no value return.
 *
 * NOTE: Modify the enable value of the data redaction policy
 * in table hg_redaction_policy.
 */
static void
StatusRedactionPolicy(AlterRedactionPolicyStmt *stmt)
{
	Relation		policyrel;
	HeapTuple		policy_tuple, policy_newtuple;
	ScanKeyData		skey[2];
	SysScanDesc		sscan;
	Oid				rel_id;
	Datum			pol_val[Natts_hg_redaction_policy];
	bool			pol_null[Natts_hg_redaction_policy];
	bool			pol_repl[Natts_hg_redaction_policy];
	
	policyrel = table_open(get_policytb_oid(), RowExclusiveLock);

	/*
	 * Find related records based on the oid of the
	 * table and the name of the data redaction pilicy.
	 */
	rel_id = RangeVarGetRelidExtended(stmt->table_name, AccessShareLock,
										0,
										RangeVarCallbackForRedactionPolicy,
										(void *) stmt);

	ScanKeyInit(&skey[0],
				Anum_hg_redaction_policy_tabrelid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(rel_id));
	ScanKeyInit(&skey[1],
				Anum_hg_redaction_policy_polname,
				BTEqualStrategyNumber, F_NAMEEQ,
				CStringGetDatum(stmt->policy_name));

	sscan = systable_beginscan(policyrel, get_policytbindex_oid(NULL, hg_redaciton_policy_tabrelid_polname_index), true,
								 NULL, 2, skey);

	policy_tuple = systable_getnext(sscan);

	/*
	 * If this data redaction policy does not exist on the
	 * hg_redaction_policy, an error will be thrown.
	 */
	if (!HeapTupleIsValid(policy_tuple))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				errmsg("policy_name \"%s\" does not exist",
						stmt->policy_name)));

	memset(pol_null, false, sizeof(pol_null));
	memset(pol_repl, false, sizeof(pol_repl));
	memset(pol_val, false, sizeof(pol_val));

	/* Fill modified value, create a modify tuple */
	pol_val[Anum_hg_redaction_policy_polpermissive - 1] = BoolGetDatum(stmt->policy_enabled);
	pol_repl[Anum_hg_redaction_policy_polpermissive - 1] = true;

	policy_newtuple = heap_modify_tuple(policy_tuple, RelationGetDescr(policyrel),
							pol_val, pol_null, pol_repl);

	/* Update row record */
	CatalogTupleUpdate(policyrel, &policy_newtuple->t_self, policy_newtuple);

	heap_freetuple(policy_newtuple);

	systable_endscan(sscan);
	table_close(policyrel, RowExclusiveLock);
}

/*
 * AddRedactionColumn
 *		no value return.
 *
 * NOTE: add new redaction column of a exist data redaction policy,
 * add new row in table hg_redaction_column.
 */
static void
AddRedactionColumn(AlterRedactionPolicyStmt *stmt)
{
	Relation		policyrel, target_table;
	SysScanDesc		sscan;
	HeapTuple		policy_tuple;
	ScanKeyData		skey[2];
	Oid				policy_id;
	Oid				table_id;
	Datum			poloid;
	Datum			tableid;
	bool			isNull;

	policyrel = table_open(get_policytb_oid(), RowExclusiveLock);

	/*
	 * Find related records based on the oid of the
	 * table and the name of the data redaction pilicy.
	 */
	table_id = RangeVarGetRelidExtended(stmt->table_name, AccessShareLock,
										0,
										RangeVarCallbackForRedactionPolicy,
										(void *) stmt);
	
	/* get the table oid and redaction policy name. */
	ScanKeyInit(&skey[0],
				Anum_hg_redaction_policy_tabrelid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(table_id));

	ScanKeyInit(&skey[1],
				Anum_hg_redaction_policy_polname,
				BTEqualStrategyNumber, F_NAMEEQ,
				CStringGetDatum(stmt->policy_name));

	sscan = systable_beginscan(policyrel, get_policytbindex_oid(NULL, hg_redaciton_policy_tabrelid_polname_index), true,
								 NULL, 2, skey);

	policy_tuple = systable_getnext(sscan);

	/*
	 * If this data redaction policy does not exist on the
	 * hg_redaction_policy, an error will be thrown.
	 */
	if (!HeapTupleIsValid(policy_tuple))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("policyname \"%s\" does not exist",
						stmt->policy_name)));

	/* get the unique oid of the data redaction policy */
	poloid = heap_getattr(policy_tuple, Anum_hg_redaction_policy_polid, RelationGetDescr(policyrel), &isNull);
	policy_id = (Oid)DatumGetObjectId(poloid);

	tableid = heap_getattr(policy_tuple, Anum_hg_redaction_policy_tabrelid, RelationGetDescr(policyrel), &isNull);
	table_id = (Oid)DatumGetObjectId(tableid);

	target_table = relation_open(table_id, AccessShareLock);

	/*
	 * Now we start to inert the data into
	 * the hg_redaction_column table.
	 */
	define_hg_redaction_column(policy_id, stmt->policy_name, table_id,
							target_table,stmt->add_columns);

	systable_endscan(sscan);
	table_close(target_table, AccessShareLock);
	table_close(policyrel, RowExclusiveLock);

}

/*
 * ModifyRedactionPolicy
 *		no value return.
 *
 * NOTE: Modify the cloumns information that exists on a 
 * redaction policy.
 */
static void
ModifyRedactionPolicy(AlterRedactionPolicyStmt *stmt)
{
	Relation		attrrel;
	HeapTuple		attr_tuple,attr_newtuple;
	ScanKeyData		skey[3];
	SysScanDesc		sscan;
	ListCell		*item;
	Datum			attr_val[Natts_hg_redaction_column];
	bool			attr_null[Natts_hg_redaction_column];
	bool			attr_repl[Natts_hg_redaction_column];
	int16			scope = 0;
	int16			exception = 0;
	ListCell		*withoption_lc;
	ListCell		*defcell;
	bool			scopeflg = false;
	bool			exceptionflg = false;

	attrrel = table_open(get_policycoltb_oid(), RowExclusiveLock);

	foreach (defcell, stmt->colname_list)
	{
		DefElem	   *defelem_cols;
		RedactionPolicyColumnStmt	*rdpolcol;
		List	   *with_options = NIL;
		List	   *colname_list = NIL;
		char	   *funcname = NULL;
		List	   *funcnamelist = NIL;		/* BUG#Z6485 */

		defelem_cols = (DefElem *) lfirst(defcell);
		rdpolcol = (RedactionPolicyColumnStmt *)defelem_cols->arg;
		with_options = rdpolcol->withops_list;
		colname_list = rdpolcol->colname_list;
		/* get function name */
		/* Begin - BUG#Z6485 */
		if (defelem_cols->defname != NULL)
		{
			funcnamelist = get_redaction_func_name(defelem_cols->defname);
			funcname = strVal(llast(funcnamelist));
		}
		/* End - BUG#Z6485 */

		if (rdpolcol->optype != 'M')
		{
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
						errmsg("syntax error at or near \"column\".")));
		}

		/*
		 * The with options option has at most two nodes.
		 * if exceeded, an error will be thrown.
		 */
		if (list_length(with_options) > 2)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
						errmsg("with options exceeds length")));

		scope = 0;
		exception = 0;
		scopeflg = false;
		exceptionflg = false;
		/* Start parser with options */
		foreach (withoption_lc, with_options)
		{
			DefElem		*withoptions = (DefElem *)lfirst(withoption_lc);

			/*
			 * If the with options is SCOPE, there are three
			 * possible values here, they are query, top_tlist,
			 * top_tlist_or_error, when the value is query, it
			 * takes effect in the top-level query, and in the
			 * subquery data redaction is not performed in, and
			 * when the value is top_tlist_or_error, an error
			 * will be thrown if appears in subquery.
			 */
			if (withoptions->defname && !strcmp(withoptions->defname, "scope"))
			{
				if (scopeflg)
					ereport(ERROR,
					(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
						errmsg("scope occurs multiple times")));
				
				if (withoptions->arg)
				{
					/*
					 * if the SCOPE value is query, it needs
					 * it needs to be converted to 1, if the
					 * SCOPE value is top_tlist, it needs
					 * to be converted to 2, if the SCOPE
					 * value is top_tlist_or_error,it needs
					 * to be converted to 4, these values
					 * will be stored in.the hg_redaction_column
					 * table.
					 */
					if (!strcmp(strVal(withoptions->arg), "query"))
						scope = 1;
					else if (!strcmp(strVal(withoptions->arg), "top_tlist"))
						scope = 2;
					else if (!strcmp(strVal(withoptions->arg), "top_tlist_or_error"))
						scope = 4;
					else
						ereport(ERROR,
							(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
								errmsg("scope attribute %s does not exist",
								 strVal(withoptions->arg))));
				}

				scopeflg = true;
			}

			/*
			 * If the with options is EXCEPTOPN, there are two
			 * possible values here, they are none and equal,
			 * when the value is none, data redaction is required
			 * everywhere, when the value is equal, appears in the
			 * equal expression without data redaction.
			 */
			if (withoptions->defname && !strcmp(withoptions->defname, "exception"))
			{
				if (exceptionflg)
					ereport(ERROR,
					(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
						errmsg("exception occurs multiple times")));
				
				if (withoptions->arg)
				{
					/*
					 * if the EXCEPTION value is none, it needs
					 * it needs to be converted to 8, if the
					 * EXCEPTION value is equal, it needs
					 * to be converted to 16, these values
					 * will be stored in.the hg_redaction_column
					 * table.
					 */
					if (!strcmp(strVal(withoptions->arg), "none"))
						exception = 8;
					else if (!strcmp(strVal(withoptions->arg), "equal"))
						exception = 16;
					else
						ereport(ERROR,
							(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
								errmsg("exception attribute %s does not exist",
								 strVal(withoptions->arg))));
				}
				exceptionflg = true;
			}
		}

		/* Now start parsing column informtion */
		foreach(item, colname_list)
		{
			Oid				procid;
			char		   *colname = strVal(lfirst(item));
			Oid				rel_id;

			/*
			 * Find related records based on the oid of the
			 * table and the name of the data redaction pilicy,
			 * and the column name the user table.
			 */
			rel_id = RangeVarGetRelidExtended(stmt->table_name, AccessShareLock,
											  0,
											  RangeVarCallbackForRedactionPolicy,
											  (void *) stmt);
			ScanKeyInit(&skey[0],
						Anum_hg_redaction_column_polname,
						BTEqualStrategyNumber, F_NAMEEQ,
						CStringGetDatum(stmt->policy_name));
			ScanKeyInit(&skey[1],
						Anum_hg_redaction_policy_tabrelid,
						BTEqualStrategyNumber, F_OIDEQ,
						ObjectIdGetDatum(rel_id));
			/* Set key - policy's name. */
			ScanKeyInit(&skey[2],
						Anum_hg_redaction_column_attname,
						BTEqualStrategyNumber, F_NAMEEQ,
						CStringGetDatum(colname));
			
			sscan = systable_beginscan(attrrel,
									   get_policytbindex_oid(NULL, hg_redaciton_column_polname_attname_index), true, NULL, 3, skey);

			attr_tuple = systable_getnext(sscan);

			if (!HeapTupleIsValid(attr_tuple))
					ereport(ERROR,
						(errcode(ERRCODE_DUPLICATE_OBJECT),
						 errmsg("column \"%s\" is not exists", colname)));

			memset(attr_null, false, sizeof(attr_null));
			memset(attr_repl, false, sizeof(attr_repl));
			memset(attr_val, false, sizeof(attr_val));

			if (funcname)
			{
				procid = get_procid_from_procname(defelem_cols->defname);	/* BUG#Z6485 */

				/* Fill modified value, create a modify tuple */
				attr_val[Anum_hg_redaction_column_procid -1] = ObjectIdGetDatum(procid);
				attr_val[Anum_hg_redaction_column_procname -1] = DirectFunctionCall1(namein,
																CStringGetDatum(funcname));
				attr_repl[Anum_hg_redaction_column_procid - 1] = true;
				attr_repl[Anum_hg_redaction_column_procname - 1] = true;
			}

			if (scope)
			{
				attr_val[Anum_hg_redaction_column_polscope -1] = Int16GetDatum(scope);
				attr_repl[Anum_hg_redaction_column_polscope - 1] = true;
			}
			if (exception)
			{
				attr_val[Anum_hg_redaction_column_polexception -1] = Int16GetDatum(exception);
				attr_repl[Anum_hg_redaction_column_polexception - 1] = true;
			}

			attr_newtuple = heap_modify_tuple(attr_tuple, RelationGetDescr(attrrel),
									attr_val, attr_null, attr_repl);

			/* Update row record */
			CatalogTupleUpdate(attrrel, &attr_newtuple->t_self, attr_newtuple);

			systable_endscan(sscan);
			heap_freetuple(attr_newtuple);
		}
	}
	table_close(attrrel, RowExclusiveLock);

}

/*
 * DropColumnRedactionColumn
 *		no value return.
 *
 * NOTE: drop the cloumns information that exists on a 
 * redaction policy.
 */
static void
DropColumnRedactionColumn(AlterRedactionPolicyStmt *stmt)
{
	Relation		attrrel;
	HeapTuple		attr_tuple;
	ScanKeyData		skey[2];
	SysScanDesc		sscan;
	Oid				rel_id;
	ListCell	   *item;
	char		   *attrname = NULL;

	attrrel = table_open(get_policycoltb_oid(), RowExclusiveLock);

	/* Get the table oid */
	rel_id = RangeVarGetRelidExtended(stmt->table_name, AccessShareLock,
										0,
										RangeVarCallbackForRedactionPolicy,
										(void *) stmt);

	foreach(item, stmt->colname_list)
	{
		attrname = strVal(lfirst(item));

		/*
		 * Find related records based on the oid of the
		 * table and the name of the data redaction pilicy.
		 */
		ScanKeyInit(&skey[0],
					Anum_hg_redaction_policy_tabrelid,
					BTEqualStrategyNumber, F_OIDEQ,
					ObjectIdGetDatum(rel_id));
		ScanKeyInit(&skey[1],
					Anum_hg_redaction_column_attname,
					BTEqualStrategyNumber, F_NAMEEQ,
					CStringGetDatum(attrname));

		sscan = systable_beginscan(attrrel, get_policytbindex_oid(NULL, hg_redaciton_column_tabrelid_attname_index), 
								true, NULL, 2, skey);

		/* delete related records */
		while (HeapTupleIsValid(attr_tuple = systable_getnext(sscan)))
		{
			CatalogTupleDelete(attrrel, &attr_tuple->t_self);
		}

		systable_endscan(sscan);

	}

	table_close(attrrel, RowExclusiveLock);

}

/*
 * AlterRedactionPolicy
 *		Returns the ObjectAddress for an already existing
 * redaction policy.
 *
 * Returns value include the oid of hg_redaction_policy table,
 * and an already existing oid the a redaction policy.
 *
 * NOTE: Every time a redaction policy is alter, data will
 * be inserted into one table or two tables of
 * hg_redaction_policy and hg_redaction_column.
 */
static void
AlterRedactionPolicy(AlterRedactionPolicyStmt *stmt)
{
	Oid 			table_id;
	Relation		target_table;
	SysScanDesc 	sscan_p;
	HeapTuple		policy_tuple;
	Relation		policyrel;
	ScanKeyData 	skey[2];

	/*
	 * Only supperusers and syssso users can alter
	 * data redaction policy.
	 */
	/* Begin - BUG#Z6440 */
	if (!IS_SYSSSO(GetUserId()))
	{
		char	   *username = NULL;
		if (se_feature_mode)
		{
			username = "syssso";
		}
		else
		{
			username = "superuser";
		}

		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("permission denied to alter redacation policy\"%s\"",
						stmt->policy_name),
				 errhint("Must be %s to alter a redacation policy.", username)));
	}
	/* End - BUG#Z6440 */

	/*
	 * It can only alter the existing data redaction policy.
	 * an error will be thrown if the data redaction policy
	 * does not exist.
	 */
	if (stmt->table_name)
	{
		table_id = RangeVarGetRelidExtended(stmt->table_name, AccessShareLock,
											0,
											RangeVarCallbackForRedactionPolicy,
											(void *) stmt);  

		target_table = relation_open(table_id, NoLock);

		policyrel = table_open(get_policytb_oid(), AccessShareLock);

		ScanKeyInit(&skey[0],
					Anum_hg_redaction_policy_tabrelid,
					BTEqualStrategyNumber, F_OIDEQ,
					ObjectIdGetDatum(table_id));

		ScanKeyInit(&skey[1],
					Anum_hg_redaction_policy_polname,
					BTEqualStrategyNumber, F_NAMEEQ,
					CStringGetDatum(stmt->policy_name));

		sscan_p = systable_beginscan(policyrel, get_policytbindex_oid(NULL, hg_redaciton_policy_tabrelid_polname_index), true,
									NULL, 2, skey);
		policy_tuple = systable_getnext(sscan_p);

		if (!HeapTupleIsValid(policy_tuple))
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_OBJECT),
					errmsg("policy \"%s\" for table \"%s\" does not exist",
							stmt->policy_name,
							RelationGetRelationName(target_table))));

		table_close(policyrel, AccessShareLock);
		systable_endscan(sscan_p);
		relation_close(target_table, NoLock);

	}

	/*
	 * Now modify the rows of related tables according
	 * to different alter types.
	 */
	switch (stmt->stmt_type)
	{
		case 'R':
			RenameRedactionPolicy(stmt);
			RenameRedactioncolumn(stmt);
			break;
		case 'F':
			FilterRedactionPolicy(stmt);
			break;
		case 'S':
			StatusRedactionPolicy(stmt);
			break;
		case 'A':
			AddRedactionColumn(stmt);
			break;
		case 'M':
			ModifyRedactionPolicy(stmt);
			break;
		case 'D':
			DropColumnRedactionColumn(stmt);
			break;
		default:
			elog(ERROR, "unrecognized alter domain type: %d",(int) stmt->stmt_type);
			break;
	}
}

/*
 * hg_GetNewOidWithIndex
 *		Returns oid of a new redaction policy.
 *
 * Returns oid of a new redaction policy, the oid is
 * generated based on the indexes and columns on the
 * hg_redaction_policy table.
 *
 * NOTE: Every time a redaction policy id created, 
 * an oid should be generated.
 */
Oid
hg_GetNewOidWithIndex(Relation relation, Oid indexId, AttrNumber oidcolumn)
{
	Oid			newOid;
	SysScanDesc scan;
	ScanKeyData key;
	bool		collides;
	uint64		retries = 0;
	uint64		retries_before_log = GETNEWOID_LOG_THRESHOLD;


	/* In bootstrap mode, we don't have any indexes to use */
	if (IsBootstrapProcessingMode())
		return GetNewObjectId();

	/*
	 * We should never be asked to generate a new pg_type OID during
	 * pg_upgrade; doing so would risk collisions with the OIDs it wants to
	 * assign.  Hitting this assert means there's some path where we failed to
	 * ensure that a type OID is determined by commands in the dump script.
	 */
	Assert(!IsBinaryUpgrade || RelationGetRelid(relation) != TypeRelationId);

	/* Generate new OIDs until we find one not in the table */
	do
	{
		CHECK_FOR_INTERRUPTS();

		newOid = GetNewObjectId();

		ScanKeyInit(&key,
					oidcolumn,
					BTEqualStrategyNumber, F_OIDEQ,
					ObjectIdGetDatum(newOid));

		/* see notes above about using SnapshotAny */
		scan = systable_beginscan(relation, indexId, true,
								  SnapshotAny, 1, &key);

		collides = HeapTupleIsValid(systable_getnext(scan));

		systable_endscan(scan);

		/*
		 * Log that we iterate more than GETNEWOID_LOG_THRESHOLD but have not
		 * yet found OID unused in the relation. Then repeat logging with
		 * exponentially increasing intervals until we iterate more than
		 * GETNEWOID_LOG_MAX_INTERVAL. Finally repeat logging every
		 * GETNEWOID_LOG_MAX_INTERVAL unless an unused OID is found. This
		 * logic is necessary not to fill up the server log with the similar
		 * messages.
		 */
		if (retries >= retries_before_log)
		{
			ereport(LOG,
					(errmsg("still searching for an unused OID in relation \"%s\"",
							RelationGetRelationName(relation)),
					 errdetail_plural("OID candidates have been checked %llu time, but no unused OID has been found yet.",
									  "OID candidates have been checked %llu times, but no unused OID has been found yet.",
									  retries,
									  (unsigned long long) retries)));

			/*
			 * Double the number of retries to do before logging next until it
			 * reaches GETNEWOID_LOG_MAX_INTERVAL.
			 */
			if (retries_before_log * 2 <= GETNEWOID_LOG_MAX_INTERVAL)
				retries_before_log *= 2;
			else
				retries_before_log += GETNEWOID_LOG_MAX_INTERVAL;
		}

		retries++;
	} while (collides);

	/*
	 * If at least one log message is emitted, also log
	 * the completion of OID assignment.
	 */
	if (retries > GETNEWOID_LOG_THRESHOLD)
	{
		ereport(LOG,
				(errmsg_plural("new OID has been assigned in relation \"%s\" after %llu retry",
							   "new OID has been assigned in relation \"%s\" after %llu retries",
							   retries,
							   RelationGetRelationName(relation), (unsigned long long) retries)));
	}

	return newOid;
}

/*
 * CreateRedactionPolicy
 *		Returns the ObjectAddress of a new redaction policy.
 *
 * Returns value include the oid of hg_redaction_policy, and
 * new oid the a redaction policy.
 *
 * NOTE: Every time a redaction policy is create, data will
 * be inserted into two tables of hg_redaction_policy and
 * hg_redaction_column.
 */
static void
CreateRedactionPolicy(CreateRedactionPolicyStmt *stmt)
{
	Relation		policyrel;
	Oid				policy_id;
	Oid				table_id;
	ScanKeyData 	skey[2];
	SysScanDesc 	sscan_p;
	HeapTuple		policy_tuple;
	Relation		target_table;
	Datum			values[Natts_hg_redaction_policy];
	bool			nulls[Natts_hg_redaction_policy];
	Datum			newOptions;
	ObjectAddress	target;
	ObjectAddress	myself;
	char		   *schemaname = NULL;

	/*
	 * Only supperusers and syssso users can create
	 * data redaction policy.
	 */
	/* Begin - BUG#Z6440 */
	if (!IS_SYSSSO(GetUserId()))
	{
		char	   *username = NULL;

		if (se_feature_mode)
		{
			username = "syssso";
		}
		else
		{
			username = "superuser";
		}

		ereport(ERROR,
			(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				errmsg("permission denied to create redacation policy \"%s\"",
					stmt->policy_name),
				errhint("Must be %s to create a redacation policy.", username)));
	}
	/* End - BUG#Z6440 */

	/*
	 * Get the oid of the table that need to create
	 * data redaction policy.
	 */
	table_id = RangeVarGetRelidExtended(stmt->table_name, AccessShareLock,
										0,
										RangeVarCallbackForRedactionPolicy,
										(void *) stmt);

	if (stmt->table_name->schemaname)
		schemaname = stmt->table_name->schemaname;
	else
		schemaname = get_table_schema(table_id);
	/*
	 * Open target table that need to create data redaction
	 * policy, No additional lock is necessary.
	 */
	target_table = relation_open(table_id, NoLock);

	/* Open hg_redaction_policy table */
	policyrel = table_open(get_policytb_oid(), RowExclusiveLock);

	/* Check whether there is an existing policy in the
	 * hg_redaction_policy table throuth the user table
	 * oid and redaction policy name.
	 */
	ScanKeyInit(&skey[0],
				Anum_hg_redaction_policy_tabrelid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(table_id));

	ScanKeyInit(&skey[1],
				Anum_hg_redaction_policy_polname,
				BTEqualStrategyNumber, F_NAMEEQ,
				CStringGetDatum(stmt->policy_name));

	sscan_p = systable_beginscan(policyrel,
									get_policytbindex_oid(NULL, hg_redaciton_policy_tabrelid_polname_index),
									true, NULL, 2, skey);
	policy_tuple = systable_getnext(sscan_p);

	/*
	 * The redaction policy name of the same table is unique,
	 * and different tables can have the same redaction policy name.
	 */
	if (HeapTupleIsValid(policy_tuple))
		ereport(ERROR,
				(errcode(ERRCODE_DUPLICATE_OBJECT),
				 errmsg("the policy \"%s\" already exist ",
				 stmt->policy_name)));

	/* zero-clear */
	memset(values, 0, sizeof(values));
	memset(nulls, 0, sizeof(nulls));

	/* get a new policy oid */
	policy_id = hg_GetNewOidWithIndex(policyrel,
									  get_policytbindex_oid(NULL, hg_redaciton_policy_oid_index),
									  Anum_hg_redaction_policy_polid);

	/* fill in the values of the hg_redaction_policy table */
	values[Anum_hg_redaction_policy_polid - 1] = ObjectIdGetDatum(policy_id);
	values[Anum_hg_redaction_policy_polname - 1] = DirectFunctionCall1(namein,
															 CStringGetDatum(stmt->policy_name));
	values[Anum_hg_redaction_policy_tabrelid - 1] = ObjectIdGetDatum(table_id);
	values[Anum_hg_redaction_policy_schema - 1] = DirectFunctionCall1(namein,
															 CStringGetDatum(schemaname));
	values[Anum_hg_redaction_policy_tabname - 1] = DirectFunctionCall1(namein,
															 CStringGetDatum(get_rel_name(table_id)));
	values[Anum_hg_redaction_policy_polpermissive - 1] = BoolGetDatum(stmt->policy_enabled);

	/* parse the options domain, the options is for clause */
	newOptions = redatcion_transformRelOptions((Datum) 0,
									 stmt->options,
									 NULL, NULL, false, false);

	if (newOptions != (Datum) 0)
		values[Anum_hg_redaction_policy_poloptions - 1] = newOptions;
	else
		nulls[Anum_hg_redaction_policy_poloptions - 1] = true;

	/* Now insert a new tuple */
	policy_tuple = heap_form_tuple(RelationGetDescr(policyrel), values, nulls);
	CatalogTupleInsert(policyrel, policy_tuple);

	/* Record Dependencies */
	target.classId = RelationRelationId;
	target.objectId = table_id;
	target.objectSubId = 0;

	myself.classId = get_policytb_oid();
	myself.objectId = policy_id;
	myself.objectSubId = 0;

	/* Add a dependency */
	recordDependencyOn(&myself, &target, DEPENDENCY_AUTO);

	InvokeObjectPostCreateHook(get_policytb_oid(), policy_id, 0);

	/*
	 * now that the insertion into the hg_redaction_policy table
	 * has been completed, we start to inert the data into
	 * the hg_redaction_column table.
	 */
	define_hg_redaction_column(policy_id, stmt->policy_name, table_id,
								target_table, stmt->add_columns);

	heap_freetuple(policy_tuple);
	table_close(policyrel, RowExclusiveLock);
	systable_endscan(sscan_p);
	relation_close(target_table, NoLock);
}

/*
 * Callback to RangeVarGetRelidExtended().
 *		no value return.
 *
 * Checks the following:
 *	- the relation specified is a table.
 *	- current user owns the table.
 *	- the table is not a system table.
 *
 * If any of these checks fails then an error is raised.
 */
static void
RangeVarCallbackForRedactionPolicy(const RangeVar *rv, Oid relid, Oid oldrelid,
						  void *arg)
{
	HeapTuple	tuple;
	Form_pg_class classform;
	char		relkind;

	tuple = SearchSysCache1(RELOID, ObjectIdGetDatum(relid));
	if (!HeapTupleIsValid(tuple))
		return;

	classform = (Form_pg_class) GETSTRUCT(tuple);
	relkind = classform->relkind;

	/* No system table modifications unless explicitly allowed. */
	if (!allowSystemTableMods && IsSystemClass(relid, classform))
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("permission denied: \"%s\" is a system catalog",
						rv->relname)));

	/* Relation type MUST be a table. */
	if (relkind != RELKIND_RELATION && relkind != RELKIND_PARTITIONED_TABLE)
		ereport(ERROR,
				(errcode(ERRCODE_WRONG_OBJECT_TYPE),
				 errmsg("\"%s\" is not a table", rv->relname)));

	ReleaseSysCache(tuple);
}

/*
 * get_procid_from_procname
 *	return function oid.
 *
 * NOTE: Return the oid of the function througth the name of the
 * function, if there are multiple functions with the same name,
 * need to find the specified data redaction function.
 */
Oid
get_procid_from_procname(char *procname)
{
	Oid				funcid = InvalidOid;
	List		   *procagrs = NIL;
	List		   *funcname = NIL;

	funcname = stringToQualifiedNameList(procname);
	if (funcname == NIL)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
				 errmsg("data redaction function \"%s\" does not exist", procname)));
	}

	/*
	 * Construct the parmater list of the data
	 * redaction function.
	 */
	procagrs = redaction_func_args(strVal(llast(funcname)));

	if (list_length(procagrs) == 2)
	{
		Oid		funcargtypes[2];

		funcargtypes[0] = linitial_oid(procagrs);
		funcargtypes[1] = lsecond_oid(procagrs);
		funcid = LookupFuncName(funcname, list_length(procagrs), funcargtypes, false);
	}
	else if (list_length(procagrs) == 1)
	{
		Oid			funcargtypes[1];

		funcargtypes[0] = linitial_oid(procagrs);
		funcid = LookupFuncName(funcname, list_length(procagrs), funcargtypes, false);
	}
	else
	{
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				  errmsg("data redaction function \"%s\" does not exist", procname)));
	}

	if (!check_redaction_func(funcid, procagrs))
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				  errmsg("data redaction function \"%s\" does not exist", procname)));

	return funcid;
}

/*
 * check_table_col
 *		no return value.
 *
 * NOTE: check is this table has this column.
 */
static void check_table_col(Relation rel, char *colname)
{
	TupleDesc	tupdesc = RelationGetDescr(rel);
	int			i;
	bool		resflg = false;

	for (i = 1; i <= tupdesc->natts; i++)
	{
		char			   *name = NULL;
		Form_pg_attribute   attr = TupleDescAttr(tupdesc, i - 1);

		if (attr->attisdropped)
			continue;
		else
		{
			name = pstrdup(NameStr(attr->attname));
			if (!strcmp(colname, name))
			{
				resflg = true;
				break;
			}
		}
	}

	if (!resflg)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
				 errmsg("in table %s column %s is not exist",
				 RelationGetRelationName(rel), colname)));
}

/*
 * define_hg_redaction_column
 *		no return value.
 *
 * NOTE: the main function of this function is to insert 
 * the column information of the data redaction policy
 * into the a table.
 */
static void
define_hg_redaction_column(Oid policy_id, char *polname, Oid table_id,
									 Relation target_table, List *colfunc_list)
{
	Relation		attrrel;
	Datum 			values[Natts_hg_redaction_column];
	bool			nulls[Natts_hg_redaction_column];
	ListCell	   *colfunc_lc;
	ListCell	   *col_lc;
	ListCell	   *withoption_lc;
	List		   *col_list = NULL;
	List		   *withoption_list = NULL;

	/* Open hg_redaction_column table */
	attrrel = table_open(get_policycoltb_oid(), RowExclusiveLock);

	/* and make the entries */
	memset(nulls, false, sizeof(nulls));

	/*
	 * There colfunc_list is column information.
	 */
	foreach(colfunc_lc, colfunc_list)
	{
		DefElem		   *func_cols = (DefElem *) lfirst(colfunc_lc);
		ScanKeyData		skey[2];
		Oid				procid;
		int16			scope = 0;
		int16			exception = 0;
		bool			scopeflg = false;
		bool			exceptionflg = false;
		List		   *funcnamelist = NIL;
		char		   *funcname = NULL;

		funcnamelist = get_redaction_func_name(func_cols->defname);	/* BUG#Z6485 */
		funcname = strVal(llast(funcnamelist));

		/*
		 * Parser the node, the node is a RedactionPolicyColumnStmt,
		 * including two lists, one list is the column information,
		 * and the other is the with options option.
		 */
		if (funcname != NULL)
		{
			RedactionPolicyColumnStmt *colelement = (RedactionPolicyColumnStmt *)func_cols->arg;
		
			col_list = colelement->colname_list;
			withoption_list = colelement->withops_list;
		}

		/*
		 * The with options option has at most two nodes.
		 * if exceeded, an error will be thrown.
		 */
		if (list_length(withoption_list) > 2)
			ereport(ERROR,
				(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
					errmsg("with options exceeds length")));

		/* Start parser with options */
		foreach (withoption_lc, withoption_list)
		{
			DefElem		*withoptions = (DefElem *)lfirst(withoption_lc);

			/*
			 * If the with options is SCOPE, there are three
			 * possible values here, they are query, top_tlist,
			 * top_tlist_or_error, when the value is query, it
			 * takes effect in the top-level query, and in the
			 * subquery data redaction is not performed in, and
			 * when the value is top_tlist_or_error, an error
			 * will be thrown if appears in subquery.
			 */
			if (withoptions->defname && !strcmp(withoptions->defname, "scope"))
			{
				/* An error will be thrown if scop appears twice */
				if (scopeflg)
					ereport(ERROR,
						(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
							errmsg("scope occurs multiple times")));
				
				if (withoptions->arg)
				{
					/*
					 * if the SCOPE value is query, it needs
					 * it needs to be converted to 1, if the
					 * SCOPE value is top_tlist, it needs
					 * to be converted to 2, if the SCOPE
					 * value is top_tlist_or_error,it needs
					 * to be converted to 4, these values
					 * will be stored in.the hg_redaction_column
					 * table.
					 */
					if (!strcmp(strVal(withoptions->arg), "query"))
						scope = 1;
					else if (!strcmp(strVal(withoptions->arg), "top_tlist"))
						scope = 2;
					else if (!strcmp(strVal(withoptions->arg), "top_tlist_or_error"))
						scope = 4;
					else
						ereport(ERROR,
							(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
								errmsg("scope attribute %s does not exist",
								 strVal(withoptions->arg))));
				}
				scopeflg = true;
			}

			/*
			 * If the with options is EXCEPTOPN, there are two
			 * possible values here, they are none and equal,
			 * when the value is none, data redaction is required
			 * everywhere, when the value is equal, appears in the
			 * equal expression without data redaction.
			 */
			if (withoptions->defname && !strcmp(withoptions->defname, "exception"))
			{
				/* An error will be thrown if exception appears twice */
				if (exceptionflg)
					ereport(ERROR,
						(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
							errmsg("exception occurs multiple times")));

				/*
				 * if the EXCEPTION value is none, it needs
				 * it needs to be converted to 8, if the
				 * EXCEPTION value is equal, it needs
				 * to be converted to 16, these values
				 * will be stored in.the hg_redaction_column
				 * table.
				 */
				if (withoptions->arg)
				{
					if (!strcmp(strVal(withoptions->arg), "none"))
						exception = 8;
					else if (!strcmp(strVal(withoptions->arg), "equal"))
						exception = 16;
					else
						ereport(ERROR,
							(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
								errmsg("exception attribute %s does not exist",
								 strVal(withoptions->arg))));
				}
				exceptionflg = true;
			}
		}

		/*
		 * if there is no with options option here, the
		 * default value of scope is 1, and the default value
		 * of exception is 16.
		 */
		if (scope == 0)
			scope = 1;
		if (exception == 0)
			exception = 16;

		/* Now start parsing column informtion */
		foreach(col_lc, col_list)
		{
			HeapTuple	attr_tuple;
			SysScanDesc	sscan;

			/* Must ensure that the column names is not null */
			char *colname = strVal(lfirst(col_lc));
			if (colname == NULL)
			{
				ereport(ERROR,
						(errcode(ERRCODE_DUPLICATE_OBJECT),
						  errmsg("column name can not be vacancy !")));
			}
			check_table_col(target_table, colname);

			/*
			 * Columns on the same table can only appear in
			 * one data redaction policy.if they already exist
			 * an error will be reported.
			 */
			ScanKeyInit(&skey[0],
						Anum_hg_redaction_column_tabrelid,
						BTEqualStrategyNumber, F_OIDEQ,
						ObjectIdGetDatum(table_id));
			ScanKeyInit(&skey[1],
						Anum_hg_redaction_column_attname,
						BTEqualStrategyNumber, F_NAMEEQ,
						CStringGetDatum(colname));
			sscan = systable_beginscan(attrrel, 
									   get_policytbindex_oid(NULL, hg_redaciton_column_tabrelid_attname_index), true, NULL, 2, skey);

			attr_tuple = systable_getnext(sscan);

			if (HeapTupleIsValid(attr_tuple))
				ereport(ERROR,
						(errcode(ERRCODE_DUPLICATE_OBJECT),
						  errmsg("column \"%s\" for table \"%s\" already have policy",
								 colname, RelationGetRelationName(target_table))));

			/* fill in the values of the hg_redaction_column table */
			values[Anum_hg_redaction_column_polid - 1]	= ObjectIdGetDatum(policy_id);
			values[Anum_hg_redaction_column_polname -1] = DirectFunctionCall1(namein,
																			   CStringGetDatum(polname));
			values[Anum_hg_redaction_column_tabrelid -1] = ObjectIdGetDatum(table_id);
			values[Anum_hg_redaction_column_attname - 1] = DirectFunctionCall1(namein,
																			   CStringGetDatum(colname));

			procid = get_procid_from_procname(func_cols->defname);

			values[Anum_hg_redaction_column_procid -1] = ObjectIdGetDatum(procid);
			values[Anum_hg_redaction_column_procname -1] = DirectFunctionCall1(namein,
																			   CStringGetDatum(funcname));

			values[Anum_hg_redaction_column_polscope - 1] = Int16GetDatum(scope);
			values[Anum_hg_redaction_column_polexception - 1] = Int16GetDatum(exception);

			attr_tuple = heap_form_tuple(RelationGetDescr(attrrel), values, nulls);

			/* Now insert a new tuple */
			CatalogTupleInsert(attrrel, attr_tuple);

			systable_endscan(sscan);
			heap_freetuple(attr_tuple);
		}
	}

	table_close(attrrel, RowExclusiveLock);
}

/*
 * redaction_func_args
 *  return a list of function argment.
 *
 * NOTE: The data redaction function is predefined,and 
 * the number and type of parameter are clear, create
 * a list of parameter oid, and return the list.
 */
List *
redaction_func_args(char *funcname)
{
	List	   *funcargs = NIL;
	int			functype;

	/*
	 * Obtain the data redaction function type, and
	 * create a corresponding parmeter list according
	 * to diffent types of functions.
	 */
	functype = get_function_behavious(funcname);
	switch(functype)
	{
		case REDACT_ALL:
		case REDACT_SHUFFLE:
		case REDACT_RANDOM:
			funcargs = list_make1_oid(TEXTOID);
			break;
		case REDACT_BANKCARD:
		case REDACT_EMAILNAME:
		case REDACT_EMAILFULL:
		case REDACT_DIGITS:
		case REDACT_IDCARD:
		case REDACT_NAME:
		case REDACT_PHONE:
			funcargs = list_make2_oid(TEXTOID, BPCHAROID);
			break;
		case REDACT_CASH:
			funcargs = list_make1_oid(CASHOID);
			break;
		default:
			break;
	}

	return funcargs;
}

/*
 * check_redaction_func
 *	return a bool.
 *
 * NOTE: if the number and type of parameter of
 * this function are consistent with redaction
 * function, and procofig is also a fixed value,
 * it means that the function has been foud.
 */
bool
check_redaction_func(Oid functionId, List *funcargs)
{
	Relation		rel;
	/*ScanKeyData		skey;
	SysScanDesc		sscan;*/
	HeapTuple		newtup;
	bool			isNull;
	Datum			proconfigdatum;
	bool			resflg = false;

	if (funcargs == NIL)
		return false;

	/*
	 * Get the record of the function from the pg_proc
	 * table by oid.
	 */
	rel = table_open(ProcedureRelationId, AccessShareLock);
	newtup =SearchSysCache1(PROCOID, ObjectIdGetDatum(functionId));

	if (HeapTupleIsValid(newtup))
	{
		ArrayType	   *proconfig;
		int				i;
		Form_pg_proc	procform;
		bool			nargseq = false;
		
		procform = (Form_pg_proc) GETSTRUCT(newtup);

		/*
		 * The number of parameters of the function 
		 * must be equal.
		 */
		if (procform && list_length(funcargs) == procform->pronargs)
		{
			ListCell   *oidcell;
			int			pos = 0;
			Oid			argoid = InvalidOid;

			/*
			 * The parameter types of the function
			 * must be same.
			 */
			foreach(oidcell, funcargs)
			{
				argoid = lfirst_oid(oidcell);
				if (argoid == procform->proargtypes.values[pos++])
					nargseq = true;
				else
				{
					nargseq = false;
					break;
				}
			}
		}

		/* get the proconfig from pg_proc */
		proconfigdatum = heap_getattr(newtup, Anum_pg_proc_proconfig,
										RelationGetDescr(rel), &isNull);
		proconfig = DatumGetArrayTypePCopy(proconfigdatum);

		if (!isNull && proconfigdatum && nargseq)
		{
			for (i = 1; i <= ARR_DIMS(proconfig)[0]; i++)
			{
				Datum		d;
				bool		isnull;
				char	   *s;

				d = array_ref(proconfig, 1, &i,
							  -1 /* varlenarray */ ,
							  -1 /* TEXT's typlen */ ,
							  false /* TEXT's typbyval */ ,
							  TYPALIGN_INT /* TEXT's typalign */ ,
							  &isnull);

				if (isnull)
					continue;

				s = TextDatumGetCString(d);
				/*
				 * the "pg_proc.redaction=1" is the
				 * fixed format of the data redaction
				 * function.
				 */
				if (!strcmp(s, "pg_proc.redaction=1"))
				{
					resflg = true;
					break;
				}
			}
		}

		ReleaseSysCache(newtup);
	}

	table_close(rel, AccessShareLock);

	return resflg;
}

/*
 * all_redaction_policy_num
 *		Returns a number of all redaction policy.
 *
 * Count the number of redactions.
 *
 */
Datum
all_redaction_policy_num(PG_FUNCTION_ARGS)
{
	Relation	rel;
	TableScanDesc scan;
	HeapTuple	tup;
	Snapshot	snapshot;
	int64		policynum = 0; 
	
	rel = table_open(get_policytb_oid(), AccessShareLock);
	snapshot = RegisterSnapshot(GetLatestSnapshot());
	scan = table_beginscan(rel, snapshot, 0, NULL);

	while ((tup = heap_getnext(scan, ForwardScanDirection)) != NULL)
	{
		policynum++;
	}
	
	table_endscan(scan);
	UnregisterSnapshot(snapshot);
	table_close(rel, AccessShareLock);

	return policynum;
}

/* Begin - BUG#Z6485
 *
 * get_redaction_func_name
 *		Returns a function name list.
 *
 * get function schema and function name.
 *
 */
static List *
get_redaction_func_name(char *defname)
{
	List		   *funcnamelist = NIL;

	funcnamelist = stringToQualifiedNameList(defname);
	if (funcnamelist == NIL)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
				 errmsg("data redaction function \"%s\" does not exist", defname)));
	}

	return funcnamelist;
}
/* End - BUG#Z6485 */
