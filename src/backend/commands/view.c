/*-------------------------------------------------------------------------
 *
 * view.c
 *	  use rewrite rules to construct views
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/commands/view.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/relation.h"
#include "access/table.h"	/* ReqID:SRS-SQL-FORCEVIEW */
#include "access/xact.h"
#include "catalog/heap.h"		/* ReqID:SRS-SQL-FORCEVIEW */
#include "catalog/indexing.h"	/* ReqID:SRS-SQL-FORCEVIEW */
#include "catalog/namespace.h"
#include "catalog/pg_force_view.h"	/* ReqID:SRS-SQL-FORCEVIEW */
#include "catalog/pg_rewrite.h"		/* ReqID:SRS-SQL-FORCEVIEW */
#include "commands/defrem.h"
#include "commands/encrypted_column.h"
#include "commands/event_trigger.h"	/* BUG#5707 */
#include "commands/tablecmds.h"
#include "commands/view.h"
#include "miscadmin.h"
#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"
#include "parser/analyze.h"
#include "parser/parser.h"	/* ReqID:SRS-SQL-FORCEVIEW */
#include "parser/parse_relation.h"
#include "rewrite/rewriteDefine.h"
#include "rewrite/rewriteHandler.h"
#include "rewrite/rewriteManip.h"
#include "rewrite/rewriteSupport.h"
#include "utils/acl.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/guc.h"				/* ReqID:SRS-SQL-FORCEVIEW */
#include "utils/ora_compatible.h"	/* ReqID:SRS-SQL-FORCEVIEW */
#include "utils/rel.h"
#include "utils/regproc.h"	/* ReqID:SRS-SQL-FORCEVIEW */
#include "utils/syscache.h"
/* BEGIN - BUGM354 */
#include "parser/pg_object_hook.h"
#include "catalog/pg_object.h"
/* END - BUGM354 */
/* Begin - OA-REQ-2513 */
#include "access/genam.h"
#include "catalog/pg_depend.h"
#include "utils/fmgroids.h"
/* End - OA-REQ-2513 */

static void checkViewTupleDesc(TupleDesc newdesc, TupleDesc olddesc);

/*---------------------------------------------------------------------
 * DefineVirtualRelation
 *
 * Create a view relation and use the rules system to store the query
 * for the view.
 *
 * EventTriggerAlterTableStart must have been called already.
 *---------------------------------------------------------------------
 */
static ObjectAddress
DefineVirtualRelation(RangeVar *relation, List *tlist, bool replace,
					  List *options, Query *viewParse)
{
	Oid			viewOid;
	LOCKMODE	lockmode;
	CreateStmt *createStmt = makeNode(CreateStmt);
	List	   *attrList;
	ListCell   *t;

	/*
	 * create a list of ColumnDef nodes based on the names and types of the
	 * (non-junk) targetlist items from the view's SELECT list.
	 */
	attrList = NIL;
	foreach(t, tlist)
	{
		TargetEntry *tle = (TargetEntry *) lfirst(t);

		if (!tle->resjunk)
		{
			ColumnDef  *def = makeColumnDef(tle->resname,
											exprType((Node *) tle->expr),
											exprTypmod((Node *) tle->expr),
											exprCollation((Node *) tle->expr));

			/*
			 * It's possible that the column is of a collatable type but the
			 * collation could not be resolved, so double-check.
			 */
			if (type_is_collatable(exprType((Node *) tle->expr)))
			{
				if (!OidIsValid(def->collOid))
					ereport(ERROR,
							(errcode(ERRCODE_INDETERMINATE_COLLATION),
							 errmsg("could not determine which collation to use for view column \"%s\"",
									def->colname),
							 errhint("Use the COLLATE clause to set the collation explicitly.")));
			}
			else
				Assert(!OidIsValid(def->collOid));

			attrList = lappend(attrList, def);
		}
	}

	/*
	 * Look up, check permissions on, and lock the creation namespace; also
	 * check for a preexisting view with the same name.  This will also set
	 * relation->relpersistence to RELPERSISTENCE_TEMP if the selected
	 * namespace is temporary.
	 */
	lockmode = replace ? AccessExclusiveLock : NoLock;
	(void) RangeVarGetAndCheckCreationNamespace(relation, lockmode, &viewOid);

	/* Begin - ReqID:SRS-SQL-FORCEVIEW */
	/*
	 * If the view is a force view, rebuild the view.
	 */
	if (ORA_MODE == compatible_mode &&
		OidIsValid(viewOid) &&
		rel_is_force_view(viewOid))
	{
		if (replace)
		{
			ObjectAddress address;

			make_view_valid(tlist, viewOid, options, viewParse);
			ObjectAddressSet(address, RelationRelationId, viewOid);
			return address;
		}
	}
	/* End - ReqID:SRS-SQL-FORCEVIEW */

	if (OidIsValid(viewOid) && replace)
	{
		Relation	rel;
		TupleDesc	descriptor;
		List	   *atcmds = NIL;
		AlterTableCmd *atcmd;
		ObjectAddress address;

		/* Relation is already locked, but we must build a relcache entry. */
		rel = relation_open(viewOid, NoLock);

		/* Make sure it *is* a view. */
		if (rel->rd_rel->relkind != RELKIND_VIEW)
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("\"%s\" is not a view",
							RelationGetRelationName(rel))));

		/* Also check it's not in use already */
		CheckTableNotInUse(rel, "CREATE OR REPLACE VIEW");

		/*
		 * Due to the namespace visibility rules for temporary objects, we
		 * should only end up replacing a temporary view with another
		 * temporary view, and similarly for permanent views.
		 */
		Assert(relation->relpersistence == rel->rd_rel->relpersistence);

		/*
		 * Create a tuple descriptor to compare against the existing view, and
		 * verify that the old column list is an initial prefix of the new
		 * column list.
		 */
		descriptor = BuildDescForRelation(attrList);
		checkViewTupleDesc(descriptor, rel->rd_att);

		/*
		 * If new attributes have been added, we must add pg_attribute entries
		 * for them.  It is convenient (although overkill) to use the ALTER
		 * TABLE ADD COLUMN infrastructure for this.
		 *
		 * Note that we must do this before updating the query for the view,
		 * since the rules system requires that the correct view columns be in
		 * place when defining the new rules.
		 *
		 * Also note that ALTER TABLE doesn't run parse transformation on
		 * AT_AddColumnToView commands.  The ColumnDef we supply must be ready
		 * to execute as-is.
		 */
		if (list_length(attrList) > rel->rd_att->natts)
		{
			ListCell   *c;
			int			skip = rel->rd_att->natts;

			foreach(c, attrList)
			{
				if (skip > 0)
				{
					skip--;
					continue;
				}
				atcmd = makeNode(AlterTableCmd);
				atcmd->subtype = AT_AddColumnToView;
				atcmd->def = (Node *) lfirst(c);
				atcmds = lappend(atcmds, atcmd);
			}

			/* EventTriggerAlterTableStart called by ProcessUtilitySlow */
			AlterTableInternal(viewOid, atcmds, true);

			/* Make the new view columns visible */
			CommandCounterIncrement();
		}

		/*
		 * Update the query for the view.
		 *
		 * Note that we must do this before updating the view options, because
		 * the new options may not be compatible with the old view query (for
		 * example if we attempt to add the WITH CHECK OPTION, we require that
		 * the new view be automatically updatable, but the old view may not
		 * have been).
		 */
		StoreViewQuery(viewOid, viewParse, replace);

		/* Make the new view query visible */
		CommandCounterIncrement();

		/*
		 * Update the view's options.
		 *
		 * The new options list replaces the existing options list, even if
		 * it's empty.
		 */
		atcmd = makeNode(AlterTableCmd);
		atcmd->subtype = AT_ReplaceRelOptions;
		atcmd->def = (Node *) options;
		atcmds = list_make1(atcmd);

		/* EventTriggerAlterTableStart called by ProcessUtilitySlow */
		AlterTableInternal(viewOid, atcmds, true);

		/*
		 * There is very little to do here to update the view's dependencies.
		 * Most view-level dependency relationships, such as those on the
		 * owner, schema, and associated composite type, aren't changing.
		 * Because we don't allow changing type or collation of an existing
		 * view column, those dependencies of the existing columns don't
		 * change either, while the AT_AddColumnToView machinery took care of
		 * adding such dependencies for new view columns.  The dependencies of
		 * the view's query could have changed arbitrarily, but that was dealt
		 * with inside StoreViewQuery.  What remains is only to check that
		 * view replacement is allowed when we're creating an extension.
		 */
		ObjectAddressSet(address, RelationRelationId, viewOid);

		recordDependencyOnCurrentExtension(&address, true);

		/*
		 * Seems okay, so return the OID of the pre-existing view.
		 */
		relation_close(rel, NoLock);	/* keep the lock! */

		return address;
	}
	else
	{
		ObjectAddress address;

		/*
		 * Set the parameters for keys/inheritance etc. All of these are
		 * uninteresting for views...
		 */
		createStmt->relation = relation;
		createStmt->tableElts = attrList;
		createStmt->inhRelations = NIL;
		createStmt->constraints = NIL;
		createStmt->options = options;
		createStmt->oncommit = ONCOMMIT_NOOP;
		createStmt->tablespacename = NULL;
		createStmt->if_not_exists = false;
		createStmt->nestdtableCols = NIL; /* ReqID:SRS-SQL-Collection */

		/*
		 * Create the relation (this will error out if there's an existing
		 * view, so we don't need more code to complain if "replace" is
		 * false).
		 */
		address = DefineRelation(createStmt, RELKIND_VIEW, InvalidOid, NULL,
								 NULL);
		Assert(address.objectId != InvalidOid);

		/* Make the new view relation visible */
		CommandCounterIncrement();

		/* Store the query for the view */
		StoreViewQuery(address.objectId, viewParse, replace);

		return address;
	}
}

/* Begin - ReqID:SRS-SQL-FORCEVIEW */
/*---------------------------------------------------------------------
 * CreateForceVirtualPlaceholder
 *
 * 		Create virtual relation when force view compilation fails.
 *---------------------------------------------------------------------
 */
static ObjectAddress
CreateForceVirtualPlaceholder(RangeVar *relation, bool replace, List *options, bool *need_update)
{
	Oid			viewOid;
	LOCKMODE	lockmode;
	CreateStmt *createStmt = makeNode(CreateStmt);
	List	   *attrList;

	/* the target list of force view is NULL */
	attrList = NIL;

	/*
	 * Look up, check permissions on, and lock the creation namespace; also
	 * check for a preexisting view with the same name.  This will also set
	 * relation->relpersistence to RELPERSISTENCE_TEMP if the selected
	 * namespace is temporary.
	 */
	lockmode = replace ? AccessExclusiveLock : NoLock;
	(void) RangeVarGetAndCheckCreationNamespace(relation, lockmode, &viewOid);

	/*
	 * If the old view is a force view, it is just a placeholder
	 * and does not need to be replaced.
	 * 
	 * If there is an 'or replace' option, the metadata needs to be
	 * updated in pg_force_view. Otherwise, nothing need to be done.
	 */
	if (OidIsValid(viewOid) && rel_is_force_view(viewOid))
	{
		ObjectAddress address;

		ObjectAddressSet(address, RelationRelationId, viewOid);
		if (replace)
			*need_update = true;
		else
			*need_update = false;
		return address;
	}

	/* old view is a valid view */
	if (OidIsValid(viewOid))
	{
		if (replace)
		{
			ObjectAddress address;

			make_view_invalid(viewOid);
			ObjectAddressSet(address, RelationRelationId, viewOid);
			*need_update = true;
			return address;
		}
		else
		{
			ObjectAddress address;

			/* do nothing */
			ObjectAddressSet(address, RelationRelationId, viewOid);
			*need_update = false;
			return address;
		}
	}
	else
	{
		ObjectAddress address;

		/*
		 * now set the parameters for keys/inheritance etc. All of these are
		 * uninteresting for views...
		 */
		createStmt->relation = relation;
		createStmt->tableElts = attrList;
		createStmt->inhRelations = NIL;
		createStmt->constraints = NIL;
		createStmt->options = options;
		createStmt->oncommit = ONCOMMIT_NOOP;
		createStmt->tablespacename = NULL;
		createStmt->if_not_exists = false;
		createStmt->nestdtableCols = NIL; /* ReqID:SRS-SQL-Collection */

		address = DefineRelation(createStmt, RELKIND_VIEW, InvalidOid, NULL, NULL);
		Assert(address.objectId != InvalidOid);
		*need_update = true;
		return address;
	}
}
/* End - ReqID:SRS-SQL-FORCEVIEW */

/*
 * Verify that tupledesc associated with proposed new view definition
 * matches tupledesc of old view.  This is basically a cut-down version
 * of equalTupleDescs(), with code added to generate specific complaints.
 * Also, we allow the new tupledesc to have more columns than the old.
 */
static void
checkViewTupleDesc(TupleDesc newdesc, TupleDesc olddesc)
{
	int			i;

	if (newdesc->natts < olddesc->natts)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TABLE_DEFINITION),
				 errmsg("cannot drop columns from view")));

	for (i = 0; i < olddesc->natts; i++)
	{
		Form_pg_attribute newattr = TupleDescAttr(newdesc, i);
		Form_pg_attribute oldattr = TupleDescAttr(olddesc, i);

		/* XXX msg not right, but we don't support DROP COL on view anyway */
		if (newattr->attisdropped != oldattr->attisdropped)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_TABLE_DEFINITION),
					 errmsg("cannot drop columns from view")));

		if (strcmp(NameStr(newattr->attname), NameStr(oldattr->attname)) != 0)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_TABLE_DEFINITION),
					 errmsg("cannot change name of view column \"%s\" to \"%s\"",
							NameStr(oldattr->attname),
							NameStr(newattr->attname)),
					 errhint("Use ALTER VIEW ... RENAME COLUMN ... to change name of view column instead.")));
		/* XXX would it be safe to allow atttypmod to change?  Not sure */
		if (newattr->atttypid != oldattr->atttypid ||
			newattr->atttypmod != oldattr->atttypmod)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_TABLE_DEFINITION),
					 errmsg("cannot change data type of view column \"%s\" from %s to %s",
							NameStr(oldattr->attname),
							format_type_with_typemod(oldattr->atttypid,
													 oldattr->atttypmod),
							format_type_with_typemod(newattr->atttypid,
													 newattr->atttypmod))));
		/* We can ignore the remaining attributes of an attribute... */
	}

	/*
	 * We ignore the constraint fields.  The new view desc can't have any
	 * constraints, and the only ones that could be on the old view are
	 * defaults, which we are happy to leave in place.
	 */
}

static void
DefineViewRules(Oid viewOid, Query *viewParse, bool replace)
{
	/*
	 * Set up the ON SELECT rule.  Since the query has already been through
	 * parse analysis, we use DefineQueryRewrite() directly.
	 */
	DefineQueryRewrite(pstrdup(ViewSelectRuleName),
					   viewOid,
					   NULL,
					   CMD_SELECT,
					   true,
					   replace,
					   list_make1(viewParse));

	/*
	 * Someday: automatic ON INSERT, etc
	 */
}

/*---------------------------------------------------------------
 * UpdateRangeTableOfViewParse
 *
 * Update the range table of the given parsetree.
 * This update consists of adding two new entries IN THE BEGINNING
 * of the range table (otherwise the rule system will die a slow,
 * horrible and painful death, and we do not want that now, do we?)
 * one for the OLD relation and one for the NEW one (both of
 * them refer in fact to the "view" relation).
 *
 * Of course we must also increase the 'varnos' of all the Var nodes
 * by 2...
 *
 * These extra RT entries are not actually used in the query,
 * except for run-time locking and permission checking.
 *---------------------------------------------------------------
 */
static Query *
UpdateRangeTableOfViewParse(Oid viewOid, Query *viewParse)
{
	Relation	viewRel;
	List	   *new_rt;
	ParseNamespaceItem *nsitem;
	RangeTblEntry *rt_entry1,
			   *rt_entry2;
	ParseState *pstate;

	/*
	 * Make a copy of the given parsetree.  It's not so much that we don't
	 * want to scribble on our input, it's that the parser has a bad habit of
	 * outputting multiple links to the same subtree for constructs like
	 * BETWEEN, and we mustn't have OffsetVarNodes increment the varno of a
	 * Var node twice.  copyObject will expand any multiply-referenced subtree
	 * into multiple copies.
	 */
	viewParse = copyObject(viewParse);

	/* Create a dummy ParseState for addRangeTableEntryForRelation */
	pstate = make_parsestate(NULL);

	/* need to open the rel for addRangeTableEntryForRelation */
	viewRel = relation_open(viewOid, AccessShareLock);

	/*
	 * Create the 2 new range table entries and form the new range table...
	 * OLD first, then NEW....
	 */
	nsitem = addRangeTableEntryForRelation(pstate, viewRel,
										   AccessShareLock,
										   makeAlias("old", NIL),
										   false, false);
	rt_entry1 = nsitem->p_rte;
	nsitem = addRangeTableEntryForRelation(pstate, viewRel,
										   AccessShareLock,
										   makeAlias("new", NIL),
										   false, false);
	rt_entry2 = nsitem->p_rte;

	/* Must override addRangeTableEntry's default access-check flags */
	rt_entry1->requiredPerms = 0;
	rt_entry2->requiredPerms = 0;

	new_rt = lcons(rt_entry1, lcons(rt_entry2, viewParse->rtable));

	viewParse->rtable = new_rt;

	/*
	 * Now offset all var nodes by 2, and jointree RT indexes too.
	 */
	OffsetVarNodes((Node *) viewParse, 2, 0);

	relation_close(viewRel, AccessShareLock);

	return viewParse;
}

/*
 * DefineView
 *		Execute a CREATE VIEW command.
 */
ObjectAddress
DefineView(ViewStmt *stmt, const char *queryString,
		   int stmt_location, int stmt_len)
{
	RawStmt    *rawstmt;
	Query	   *viewParse = NULL;
	RangeVar   *view;
	ListCell   *cell;
	bool		check_option;
	bool		compile_failed = false; /* ReqID:SRS-SQL-FORCEVIEW */
	ObjectAddress address;
	/* Begin - Bug#M459 */
	Oid			save_userid;
	int			save_sec_context;
	int			save_nestlevel = 0;
	bool		switch_role = false;
	/* End - Bug#M459 */

	/*
	 * Begin - Bug#M459
	 *
	 * Switch role ahead of  DefineRelation() so that objects in the target
	 * namespace can be used for sytax parsing.
	 */
	if (stmt->view->schemaname && ORA_MODE == compatible_mode && enable_ora_user_schema)
	{
		Oid	nspid = RangeVarGetAndCheckCreationNamespace(stmt->view, NoLock, NULL);
		Oid	nspowner = GetUserId();

		GetUserIdAndSecContext(&save_userid, &save_sec_context);
		(void) fit_namespace_owner(nspid, &nspowner);

		if (nspowner != save_userid)
		{
			SetUserIdAndSecContext(nspowner, save_sec_context | SECURITY_LOCAL_USERID_CHANGE);
			save_nestlevel = NewGUCNestLevel();
			switch_role = true;

			(void) set_config_option("search_path", stmt->view->schemaname,
									 PGC_USERSET, PGC_S_SESSION,
									 GUC_ACTION_SAVE, true, 0, false);
		}
	}
	/* End - Bug#M459 */

	/*
	 * Run parse analysis to convert the raw parse tree to a Query.  Note this
	 * also acquires sufficient locks on the source table(s).
	 */
	rawstmt = makeNode(RawStmt);
	rawstmt->stmt = stmt->query;
	rawstmt->stmt_location = stmt_location;
	rawstmt->stmt_len = stmt_len;

	/* Begin - ReqID:SRS-SQL-FORCEVIEW */
	PG_TRY();
	{
		viewParse = parse_analyze(rawstmt, queryString, NULL, 0, NULL);
	}
	PG_CATCH();
	{
		if (ORA_MODE == compatible_mode && stmt->force)
		{
			compile_failed = true;
			FlushErrorState();
		}
		else
			PG_RE_THROW();
	}
	PG_END_TRY();

	/* Begin - ReqID:SRS-MYSQL-SET
	 * Create a view cannot include user defined variable.
	 */
	if (MYS_MODE == compatible_mode && viewParse->hasuservar)
	{
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("View's SELECT contains a variable or parameter")));
	}
	/* End - ReqID:SRS-MYSQL-SET */

	/*
	 * There are three situations where force view compile fail:
	 *	1. If there is no view with the same name, we create a placeholder
	 *	   in pg_class, and store the metadata of the view that failed to
	 *	   compile in the pg_force_view catalog.
	 *	2. If there is a view with the same name, and the old view is a valid
	 *	   view, then do nothing if without or replace option, otherwise replace
	 *	   the valid old view, and new force view is invalid.
	 *	3. If there is a view with the same name and the old view is invalid,
	 *	   then do nothing if without or replace option, otherwise update the
	 *	   metadata of the new view in the pg_force_view catalog.
	 */
	if (stmt->force && compile_failed)
	{
		bool	need_store;

		/* TODO: Special case,the grammar can't be suppress SELECT INTO */
		view = copyObject(stmt->view);		/* don't corrupt original command */

		address = CreateForceVirtualPlaceholder(view, stmt->replace, stmt->options, &need_store);

		CommandCounterIncrement();

		/* insert or update on pg_force_view catalog */
		if (need_store)
		{
			char	ident_case;
			StringInfo	buf = makeStringInfo(); /* OA-REQ-1459 */

			if (identifier_case_switch == NORMAL)
				ident_case = IDENT_CASE_NORMAL;
			else if (identifier_case_switch == INTERCHANGE)
				ident_case = IDENT_CASE_INTERCHANGE;
			else if (identifier_case_switch == LOWERCASE)
				ident_case = IDENT_CASE_LOWERCASE;
			else
			{
				/* Normally this will not happen unless you increase the identifier_case_switch enum value */
				ident_case = IDENT_CASE_UNDEFINE;	/* keep compiler quiet */
				elog(ERROR, "Incorrect identifier_case_switch value");
			}

			/* Begin - OA-REQ-1459 */
			/* Shouldn't happen? */
			if (!stmt->stmt_literal)
				elog(ERROR, "Unable to obtain query for force view");

			if (stmt->aliases != NIL)
			{
				ListCell	*alist_item;
				int			attno = 0;

				appendStringInfoChar(buf, '(');
				foreach(alist_item, stmt->aliases)
				{
					if (attno != 0)
						appendStringInfoString(buf, ", ");
					appendStringInfoString(buf, quote_identifier(strVal(lfirst(alist_item))));
					attno++;
				}
				appendStringInfoChar(buf, ')');
			}
			/* End - OA-REQ-1459 */

			StoreForceViewQuery(address.objectId, stmt->replace, ident_case, stmt->stmt_literal, stmt->aliases ? buf->data : NULL /* OA-REQ-1459 */);
		}

		/* oracle always issues this warning */
		elog(WARNING, "View created with compilation errors");

		/* Begin - Bug#M459 */
		if (switch_role)
		{
			/* Roll back any GUC changes */
			AtEOXact_GUC(false, save_nestlevel);
			/* Restore userid and security context */
			SetUserIdAndSecContext(save_userid, save_sec_context);
		}
		/* End - Bug#M459 */
		return address;
	}
	/* End - ReqID:SRS-SQL-FORCEVIEW */

	/*
	 * The grammar should ensure that the result is a single SELECT Query.
	 * However, it doesn't forbid SELECT INTO, so we have to check for that.
	 */
	if (!IsA(viewParse, Query))
		elog(ERROR, "unexpected parse analysis result");
	if (viewParse->utilityStmt != NULL &&
		IsA(viewParse->utilityStmt, CreateTableAsStmt))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("views must not contain SELECT INTO")));
	if (viewParse->commandType != CMD_SELECT)
		elog(ERROR, "unexpected parse analysis result");

	/*
	 * Check for unsupported cases.  These tests are redundant with ones in
	 * DefineQueryRewrite(), but that function will complain about a bogus ON
	 * SELECT rule, and we'd rather the message complain about a view.
	 */
	if (viewParse->hasModifyingCTE)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("views must not contain data-modifying statements in WITH")));

	/*
	 * If the user specified the WITH CHECK OPTION, add it to the list of
	 * reloptions.
	 */
	if (stmt->withCheckOption == LOCAL_CHECK_OPTION)
		stmt->options = lappend(stmt->options,
								makeDefElem("check_option",
											(Node *) makeString("local"), -1));
	else if (stmt->withCheckOption == CASCADED_CHECK_OPTION)
		stmt->options = lappend(stmt->options,
								makeDefElem("check_option",
											(Node *) makeString("cascaded"), -1));
	/* Begin - OA-REQ-2248 */
	else if (stmt->withCheckOption == READ_ONLY_VIEW_OPTION)
		stmt->options = lappend(stmt->options,
								makeDefElem("read_only",
											(Node *) makeString("true"), -1));
	/* End - OA-REQ-2248 */

	/*
	 * Check that the view is auto-updatable if WITH CHECK OPTION was
	 * specified.
	 */
	check_option = false;

	foreach(cell, stmt->options)
	{
		DefElem    *defel = (DefElem *) lfirst(cell);

		if (strcmp(defel->defname, "check_option") == 0)
			check_option = true;
	}

	/*
	 * If the check option is specified, look to see if the view is actually
	 * auto-updatable or not.
	 */
	if (check_option)
	{
		const char *view_updatable_error =
		view_query_is_auto_updatable(viewParse, true);

		if (view_updatable_error)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("WITH CHECK OPTION is supported only on automatically updatable views"),
					 errhint("%s", _(view_updatable_error))));
	}

	/*
	 * If a list of column names was given, run through and insert these into
	 * the actual query tree. - thomas 2000-03-08
	 */
	if (stmt->aliases != NIL)
	{
		ListCell   *alist_item = list_head(stmt->aliases);
		ListCell   *targetList;

		foreach(targetList, viewParse->targetList)
		{
			TargetEntry *te = lfirst_node(TargetEntry, targetList);

			/* junk columns don't get aliases */
			if (te->resjunk)
				continue;
			te->resname = pstrdup(strVal(lfirst(alist_item)));
			alist_item = lnext(stmt->aliases, alist_item);
			if (alist_item == NULL)
				break;			/* done assigning aliases */
		}

		if (alist_item != NULL)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("CREATE VIEW specifies more column "
							"names than columns")));
	}

	/* Unlogged views are not sensible. */
	if (stmt->view->relpersistence == RELPERSISTENCE_UNLOGGED)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("views cannot be unlogged because they do not have storage")));

	/* Global temporary table are not sensible. */
	if (stmt->view->relpersistence == RELPERSISTENCE_GLOBAL_TEMP)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("views cannot be global temp because they do not have storage")));

	/*
	 * If the user didn't explicitly ask for a temporary view, check whether
	 * we need one implicitly.  We allow TEMP to be inserted automatically as
	 * long as the CREATE command is consistent with that --- no explicit
	 * schema name.
	 */
	view = copyObject(stmt->view);	/* don't corrupt original command */
	if (view->relpersistence == RELPERSISTENCE_PERMANENT
		&& isQueryUsingTempRelation(viewParse))
	{
		view->relpersistence = RELPERSISTENCE_TEMP;
		ereport(NOTICE,
				(errmsg("view \"%s\" will be a temporary view",
						view->relname)));
	}

	/*
	 * Create the view relation
	 *
	 * NOTE: if it already exists and replace is false, the xact will be
	 * aborted.
	 */
	address = DefineVirtualRelation(view, viewParse->targetList,
									stmt->replace, stmt->options, viewParse);
									
	/* ReqID:TECC4.0 - Register encrypted column metadata for VIEW columns
	 * that reference encrypted base table columns.  This enables the TECC
	 * client-side cache to recognize VIEW columns as encrypted. */
	if (OidIsValid(address.objectId) && viewParse->targetList != NIL)
	{
		populate_view_encryption_metadata(address.objectId,
										  viewParse->targetList,
										  viewParse->rtable);
	}

	/* Begin - Bug#M459 */
	if (switch_role)
	{
		/* Roll back any GUC changes */
		AtEOXact_GUC(false, save_nestlevel);
		/* Restore userid and security context */
		SetUserIdAndSecContext(save_userid, save_sec_context);
	}
	/* End - Bug#M459 */

	return address;
}

/*
 * Use the rules system to store the query for the view.
 */
void
StoreViewQuery(Oid viewOid, Query *viewParse, bool replace)
{
	/*
	 * The range table of 'viewParse' does not contain entries for the "OLD"
	 * and "NEW" relations. So... add them!
	 */
	viewParse = UpdateRangeTableOfViewParse(viewOid, viewParse);

	/*
	 * Now create the rules associated with the view.
	 */
	DefineViewRules(viewOid, viewParse, replace);
}

/* Begin - ReqID:SRS-SQL-FORCEVIEW */
void
StoreForceViewQuery(Oid viewOid, bool replace, char ident_case ,const char *queryString, const char *attlist /* OA-REQ-1459 */)
{
	Datum		values[Natts_pg_force_view];
	bool		nulls[Natts_pg_force_view];
	bool		replaces[Natts_pg_force_view];
	HeapTuple	tup,
				oldtup;
	Relation	pg_force_view_desc;

	MemSet(nulls, false, sizeof(nulls));

	values[Anum_pg_force_view_fvoid - 1] = ObjectIdGetDatum(viewOid);
	values[Anum_pg_force_view_ident_case - 1] = CharGetDatum(ident_case);
	values[Anum_pg_force_view_source - 1] = CStringGetTextDatum(queryString);

	/* Begin - OA-REQ-1459 */
	/*
	 * When the view changes from valid to invalid or no
	 * column name is specified attlist is null.
	 */
	if (attlist)
		values[Anum_pg_force_view_attlist - 1] = CStringGetTextDatum(attlist);
	else
		nulls[Anum_pg_force_view_attlist - 1] = true;
	/* End - OA-REQ-1459 */

	/*
	 * Ready to store new pg_force_view tuple
	 */
	pg_force_view_desc = table_open(ForceViewRelationId, RowExclusiveLock);

	/*
	 * Check to see if we are replacing an existing tuple
	 */
	oldtup = SearchSysCache1(FORCEVIEWOID, ObjectIdGetDatum(viewOid));

	if (HeapTupleIsValid(oldtup))
	{
		if (!replace)
			ereport(ERROR,
					(errcode(ERRCODE_DUPLICATE_OBJECT),
					 errmsg("force view \"%s\" already exists",
							get_rel_name(viewOid))));

		MemSet(replaces, false, sizeof(replaces));
		replaces[Anum_pg_force_view_source - 1] = true;
		replaces[Anum_pg_force_view_ident_case - 1] = true;
		replaces[Anum_pg_force_view_attlist - 1] = true; /* OA-REQ-1459 */

		tup = heap_modify_tuple(oldtup, RelationGetDescr(pg_force_view_desc),
								values, nulls, replaces);

		CatalogTupleUpdate(pg_force_view_desc, &tup->t_self, tup);

		ReleaseSysCache(oldtup);
	}
	else
	{
		tup = heap_form_tuple(pg_force_view_desc->rd_att, values, nulls);
		CatalogTupleInsert(pg_force_view_desc, tup);
	}

	heap_freetuple(tup);
	table_close(pg_force_view_desc, RowExclusiveLock);
}

/*
 * Delete the tuple of pg_force_view by specified oid.
 */
void
DeleteForceView(Oid forceviewid)
{
	Relation	force_view_rel;
	HeapTuple	tup;

	force_view_rel = table_open(ForceViewRelationId, RowExclusiveLock);

	tup = SearchSysCache1(FORCEVIEWOID, ObjectIdGetDatum(forceviewid));

	#if 0
	if (!HeapTupleIsValid(tup))
		elog(ERROR, "cache lookup failed for force view %u", forceviewid);
	#endif

	if (!HeapTupleIsValid(tup))
	{
		table_close(force_view_rel, RowExclusiveLock);
		return;	
	}

	CatalogTupleDelete(force_view_rel, &tup->t_self);
	ReleaseSysCache(tup);
	table_close(force_view_rel, RowExclusiveLock);
}

#if 0
bool
rel_is_force_view(Oid relid)
{
	bool		res = false;
	bool		isNull;
	HeapTuple	tp;

	tp = SearchSysCache1(RELOID, ObjectIdGetDatum(relid));

	if (HeapTupleIsValid(tp))
	{
		Datum		reloptions;

		reloptions = SysCacheGetAttr(RELOID, tp, Anum_pg_class_reloptions, &isNull);

		ReleaseSysCache(tp);

		if (!isNull)
		{
			if (PointerIsValid(DatumGetPointer(reloptions)))
			{
				Datum	   *options;
				int			count;
				int			i;

				deconstruct_array(DatumGetArrayTypeP(reloptions),
								TEXTOID, -1, false, TYPALIGN_INT,
								&options, NULL, &count);

				for (i = 0; i < count; i++)
				{
					char	   *option = TextDatumGetCString(options[i]);

					if (strstr(option, "force_view"))
					{
						if (strstr(option, "true"))
						{
							res = true;
							pfree(option);
							break;
						}
					}
					pfree(option);
				}
			}
			else
				res = false;
		}
		else
			res = false;
	}
	else
		res = false;

	return res;
}
#endif

/*
 * rel_is_force_view
 *
 *		Given an OID checks whether a relation is a force view.
 */
bool
rel_is_force_view(Oid relid)
{
	if (IsUnderPostmaster &&
		get_rel_relkind(relid) == RELKIND_VIEW)
	{
		HeapTuple	tuple;

		tuple = SearchSysCache2(RULERELNAME, ObjectIdGetDatum(relid), CStringGetDatum("_RETURN"));
		if (HeapTupleIsValid(tuple))
		{
			ReleaseSysCache(tuple);
			return false;
		}
		else
			return true;
	}
	else
		return false;
}

/*
 * Currently we store the sql statement of force view,
 * which may be parsed and generated from the raw parse
 * tree in the future.
 */
char *
get_force_view_def(Oid viewoid)
{
	HeapTuple	tup;
	Datum		view_source;
	bool		isnull;
	/* Begin - OA-REQ-1459 */
	Datum		view_attlist;
	Datum		view_ident_case;
	char		ident_case;
	int			save_nestlevel;
	StringInfo	buf = makeStringInfo();
	/* End - OA-REQ-1459 */

	tup = SearchSysCache1(FORCEVIEWOID, ObjectIdGetDatum(viewoid));
	if (!HeapTupleIsValid(tup))
		elog(ERROR, "cache lookup failed for force view %u", viewoid);

	/* Begin - OA-REQ-1459 */
	view_ident_case = SysCacheGetAttr(FORCEVIEWOID, tup,
										Anum_pg_force_view_ident_case,
										&isnull);
	if (isnull)
		elog(ERROR, "the ident case of force view %u is lost", viewoid);
	/* End - OA-REQ-1459 */

	view_source = SysCacheGetAttr(FORCEVIEWOID, tup,
									 Anum_pg_force_view_source,
									 &isnull);
	if (isnull)
		elog(ERROR, "the definition of force view %u is lost", viewoid);

	/* Begin - OA-REQ-1459 */
	view_attlist = SysCacheGetAttr(FORCEVIEWOID, tup,
									Anum_pg_force_view_attlist,
									&isnull);

	save_nestlevel = NewGUCNestLevel();
	ident_case = DatumGetChar(view_ident_case);
	if (ident_case == IDENT_CASE_NORMAL)
		(void) set_config_option("ivorysql.identifier_case_switch", "normal",
								 PGC_USERSET, PGC_S_SESSION,
								 GUC_ACTION_SAVE, true, 0, false);
	else if (ident_case == IDENT_CASE_INTERCHANGE)
		(void) set_config_option("ivorysql.identifier_case_switch", "interchange",
								 PGC_USERSET, PGC_S_SESSION,
								 GUC_ACTION_SAVE, true, 0, false);
	else if (ident_case == IDENT_CASE_LOWERCASE)
		(void) set_config_option("ivorysql.identifier_case_switch", "lowercase",
								 PGC_USERSET, PGC_S_SESSION,
								 GUC_ACTION_SAVE, true, 0, false);
	else if (ident_case == IDENT_CASE_UNDEFINE)
		/* Shouldn't happen, issue a warning? */
		elog(WARNING, "An unknown ident case was encountered");
	else
		elog(ERROR, "Invalid ident case for force view");

	appendStringInfoString(buf, "CREATE OR REPLACE FORCE VIEW ");
	appendStringInfoString(buf, quote_identifier(get_namespace_name(get_rel_namespace(viewoid))));
	appendStringInfoChar(buf, '.');
	appendStringInfoString(buf, quote_identifier(get_rel_name(viewoid)));

	if (!isnull)
	{
		appendStringInfoChar(buf, ' ');
		appendStringInfoString(buf, TextDatumGetCString(view_attlist));
	}

	appendStringInfoString(buf, " AS ");
	appendStringInfoString(buf, TextDatumGetCString(view_source));

	ReleaseSysCache(tup);
	AtEOXact_GUC(false, save_nestlevel);

	return buf->data;
	/* End - OA-REQ-1459 */
}

static bool
compile_force_view_internal(ViewStmt *stmt, const char *queryString, Oid viewoid)
{
	RawStmt    *rawstmt;
	Query	   *viewParse;
	RangeVar   *view;
	ListCell   *cell;
	bool		check_option;
	MemoryContext	oldcontext = CurrentMemoryContext; /* OA-REQ-1558 */

	/*
	 * Run parse analysis to convert the raw parse tree to a Query.  Note this
	 * also acquires sufficient locks on the source table(s).
	 */
	rawstmt = makeNode(RawStmt);
	rawstmt->stmt = stmt->query;
	rawstmt->stmt_location = 0;
	rawstmt->stmt_len = 0;

	PG_TRY();
	{
		viewParse = parse_analyze(rawstmt, queryString, NULL, 0, NULL);
	}
	PG_CATCH();
	{
		/* Begin - OA-REQ-1558 */
		ErrorData  *edata;
		MemoryContextSwitchTo(oldcontext);
		edata = CopyErrorData();
		FlushErrorState();
		if (edata->message)
			elog(LOG, "force view compile failed: %s", edata->message);
		FreeErrorData(edata);
		/* End - OA-REQ-1558 */
		return false;
	}
	PG_END_TRY();

	/*
	 * The grammar should ensure that the result is a single SELECT Query.
	 * However, it doesn't forbid SELECT INTO, so we have to check for that.
	 */
	if (!IsA(viewParse, Query))
		elog(ERROR, "unexpected parse analysis result");
	if (viewParse->utilityStmt != NULL &&
		IsA(viewParse->utilityStmt, CreateTableAsStmt))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("views must not contain SELECT INTO")));
	if (viewParse->commandType != CMD_SELECT)
		elog(ERROR, "unexpected parse analysis result");

	/*
	 * Check for unsupported cases.  These tests are redundant with ones in
	 * DefineQueryRewrite(), but that function will complain about a bogus ON
	 * SELECT rule, and we'd rather the message complain about a view.
	 */
	if (viewParse->hasModifyingCTE)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("views must not contain data-modifying statements in WITH")));

	/*
	 * If the user specified the WITH CHECK OPTION, add it to the list of
	 * reloptions.
	 */
	if (stmt->withCheckOption == LOCAL_CHECK_OPTION)
		stmt->options = lappend(stmt->options,
								makeDefElem("check_option",
											(Node *) makeString("local"), -1));
	else if (stmt->withCheckOption == CASCADED_CHECK_OPTION)
		stmt->options = lappend(stmt->options,
								makeDefElem("check_option",
											(Node *) makeString("cascaded"), -1));

	/*
	 * Check that the view is auto-updatable if WITH CHECK OPTION was
	 * specified.
	 */
	check_option = false;

	foreach(cell, stmt->options)
	{
		DefElem    *defel = (DefElem *) lfirst(cell);

		if (strcmp(defel->defname, "check_option") == 0)
			check_option = true;
	}

	/*
	 * If the check option is specified, look to see if the view is actually
	 * auto-updatable or not.
	 */
	if (check_option)
	{
		const char *view_updatable_error =
		view_query_is_auto_updatable(viewParse, true);

		if (view_updatable_error)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("WITH CHECK OPTION is supported only on automatically updatable views"),
					 errhint("%s", _(view_updatable_error))));
	}

	/*
	 * If a list of column names was given, run through and insert these into
	 * the actual query tree. - thomas 2000-03-08
	 */
	if (stmt->aliases != NIL)
	{
		ListCell   *alist_item = list_head(stmt->aliases);
		ListCell   *targetList;

		foreach(targetList, viewParse->targetList)
		{
			TargetEntry *te = lfirst_node(TargetEntry, targetList);

			/* junk columns don't get aliases */
			if (te->resjunk)
				continue;
			te->resname = pstrdup(strVal(lfirst(alist_item)));
			alist_item = lnext(stmt->aliases, alist_item);
			if (alist_item == NULL)
				break;			/* done assigning aliases */
		}

		if (alist_item != NULL)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("CREATE VIEW specifies more column "
							"names than columns")));
	}

	/* Unlogged views are not sensible. */
	if (stmt->view->relpersistence == RELPERSISTENCE_UNLOGGED)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("views cannot be unlogged because they do not have storage")));

	/* Global temporary table are not sensible. */
	if (stmt->view->relpersistence == RELPERSISTENCE_GLOBAL_TEMP)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("views cannot be global temp because they do not have storage")));

	/*
	 * If the user didn't explicitly ask for a temporary view, check whether
	 * we need one implicitly.  We allow TEMP to be inserted automatically as
	 * long as the CREATE command is consistent with that --- no explicit
	 * schema name.
	 */
	view = copyObject(stmt->view);	/* don't corrupt original command */
	if (view->relpersistence == RELPERSISTENCE_PERMANENT
		&& isQueryUsingTempRelation(viewParse))
	{
		view->relpersistence = RELPERSISTENCE_TEMP;
		ereport(NOTICE,
				(errmsg("view \"%s\" will be a temporary view",
						view->relname)));
	}

	make_view_valid(viewParse->targetList, viewoid, stmt->options, viewParse);
	return true;
}

/*
 * Compile a force view, return true if success, otherwise return false.
 */
bool
compile_force_view(Oid viewoid)
{
	bool		res = false;
	bool		isnull;
	char		*fview_querystring;
	/* Begin - OA-REQ-1459 */
	int			save_nestlevel;
	Datum		view_ident_case;
	char		ident_case;
	HeapTuple	tup;
	/* End - OA-REQ-1459 */
	List	   *raw_parsetree_list;
	RawStmt    *rs;
	Node		*node;

	if (!rel_is_force_view(viewoid))
		return true;

	/* first: fetch force view metadata */
	tup = SearchSysCache1(FORCEVIEWOID, ObjectIdGetDatum(viewoid));
	if (!HeapTupleIsValid(tup))
		elog(ERROR, "cache lookup failed for force view %u", viewoid);

	/* Begin - OA-REQ-1459 */
	view_ident_case = SysCacheGetAttr(FORCEVIEWOID, tup,
										Anum_pg_force_view_ident_case,
										&isnull);
	if (isnull)
		elog(ERROR, "the ident case of force view %u is lost", viewoid);

	ident_case = DatumGetChar(view_ident_case);
	ReleaseSysCache(tup);

	fview_querystring = get_force_view_def(viewoid);
	save_nestlevel = NewGUCNestLevel();

	if (ident_case == IDENT_CASE_NORMAL)
		(void) set_config_option("ivorysql.identifier_case_switch", "normal",
								 PGC_USERSET, PGC_S_SESSION,
								 GUC_ACTION_SAVE, true, 0, false);
	else if (ident_case == IDENT_CASE_INTERCHANGE)
		(void) set_config_option("ivorysql.identifier_case_switch", "interchange",
								 PGC_USERSET, PGC_S_SESSION,
								 GUC_ACTION_SAVE, true, 0, false);
	else if (ident_case == IDENT_CASE_LOWERCASE)
		(void) set_config_option("ivorysql.identifier_case_switch", "lowercase",
								 PGC_USERSET, PGC_S_SESSION,
								 GUC_ACTION_SAVE, true, 0, false);
	else if (ident_case == IDENT_CASE_UNDEFINE)
		/* Shouldn't happen, issue a warning? */
		elog(WARNING, "An unknown ident case was encountered");
	else
		elog(ERROR, "Invalid ident case for force view");
	/* End - OA-REQ-1459 */

	/* second: parser */
	raw_parsetree_list = raw_parser(fview_querystring, RAW_PARSE_DEFAULT);
	if (list_length(raw_parsetree_list) != 1)
		elog(ERROR, "The definition of force view is wrong");

	rs = linitial_node(RawStmt, raw_parsetree_list);
	node = rs->stmt;
	if (node->type != T_ViewStmt)
		elog(ERROR, "force view %u compile fail due to metadata wrong", viewoid);
	
	/*
	 * third: do compile
	 *
	 * We don't use PG_TRY() here for now, even though the function may emit an error message.
	 * We want to emit rather than ignore the error that the view creation check fails, which
	 * helps to analyze the problem for user.
	 *
	 * BUG#5707: collect SCT_AlterTable command for event trigger.
	 */
	EventTriggerAlterTableStart(node);
	res = compile_force_view_internal((ViewStmt *) node, fview_querystring, viewoid);
	EventTriggerAlterTableEnd();

	AtEOXact_GUC(false, save_nestlevel); /* OA-REQ-1459 */
	pfree(fview_querystring);

	return res;
}

/* 
 * make_view_invalid
 *
 *		Make a view to force view, which is an invalid state.
 *		Currently, the invalidation of a view cannot be issued explicitly. 
 *		In most cases, it is caused by the deletion of dependent objects.
 */
bool
make_view_invalid(Oid viewid)
{
	Relation	viewrel;
	Relation	relation;
	HeapTuple	tuple;
	Form_pg_class classForm;
	Datum		viewdef;
	char		ident_case = IDENT_CASE_NORMAL; /* OA-REQ-1459 */
	Oid			ruleoid = InvalidOid;
	bool		isnull;
	char		*viewdef_p; /* OA-REQ-1459 */
	int			save_nestlevel; /* OA-REQ-1558 */

	if (rel_is_force_view(viewid))
		return true;

	/* Begin - OA-REQ-1558 */
	save_nestlevel = NewGUCNestLevel();
	(void) set_config_option("ivorysql.enable_strict_view_def", "true",
							 PGC_USERSET, PGC_S_SESSION,
							 GUC_ACTION_SAVE, true, 0, false);
	viewdef = DirectFunctionCall1(pg_get_viewdef, ObjectIdGetDatum(viewid));
	AtEOXact_GUC(false, save_nestlevel);
	/* End - OA-REQ-1558 */

	/* Begin - OA-REQ-1459 */
	if (identifier_case_switch == INTERCHANGE)
		ident_case = IDENT_CASE_INTERCHANGE;
	else if (identifier_case_switch == NORMAL)
		ident_case = IDENT_CASE_NORMAL;
	else if (identifier_case_switch == LOWERCASE)
		ident_case = IDENT_CASE_LOWERCASE;
	/* End - OA-REQ-1459 */

	viewrel = relation_open(viewid, AccessExclusiveLock);
	CheckTableNotInUse(viewrel, "INVALID VIEW");

	/*
	 * 1.pg_attribute
	 * Create or replace force view maybe change targetlist of view.
	 */
	DeleteAttributeTuples(viewid);
	/* BEGIN - BUGM354 */
	if(pg_object_drop_obj_hook && compatible_mode == ORA_MODE)
		DeleteForceViewAttinPgobject(viewid);
	/* END - BUGM354 */
	/*
	 *  2.pg_class
	 * Update the relhasrules and relnatts of pg_class.
	 */
	relation = table_open(RelationRelationId, RowExclusiveLock);
	tuple = SearchSysCacheCopy1(RELOID, ObjectIdGetDatum(viewid));
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "cache lookup failed for relation %u", viewid);
	classForm = (Form_pg_class) GETSTRUCT(tuple);
	classForm->relhasrules = false;
	classForm->relnatts = 0;
	CatalogTupleUpdate(relation, &tuple->t_self, tuple);
	heap_freetuple(tuple);
	table_close(relation, RowExclusiveLock);

	/*
	 * Close relcache entry, but *keep* AccessExclusiveLock on the relation
	 * until transaction commit.  This ensures no one else will try to do
	 * something with the doomed relation.
	 */
	relation_close(viewrel, NoLock);

	/*
	 * 3.pg_rewrite
	 * Only delete the _RETURN rule of view.
	 * Are other non-SELECT type rules considered for deletion?
	 */
	relation = table_open(RewriteRelationId, RowExclusiveLock);
	tuple = SearchSysCache2(RULERELNAME, ObjectIdGetDatum(viewid), CStringGetDatum("_RETURN"));
	if (!HeapTupleIsValid(tuple)) /* should not happen */
		elog(ERROR, "Unable to get the view's rules when invalid the view");
	ruleoid = heap_getattr(tuple, Anum_pg_rewrite_oid, RelationGetDescr(relation), &isnull);
	if (isnull)
		elog(ERROR, "Unable to get the view rule's OID when invalid the view");
	CatalogTupleDelete(relation, &tuple->t_self);
	ReleaseSysCache(tuple);
	table_close(relation, RowExclusiveLock);

	/*
	 * 4.pg_depend
	 * Get rid of old dependencies for the rule of this force view.
	 */
	deleteDependencyRecordsFor(RewriteRelationId, ruleoid, false);

	/*
	 * 5.pg_force_view
	 * Store the query definition of force view.
	 */
	/* Begin - OA-REQ-1459 */
#if 0
	buf = makeStringInfo();
	appendStringInfoString(buf, "CREATE FORCE VIEW ");
	appendStringInfoString(buf, quote_identifier(get_namespace_name(get_rel_namespace(viewid))));
	appendStringInfoChar(buf, '.');
	appendStringInfoString(buf, quote_identifier(get_rel_name(viewid)));
	appendStringInfoString(buf, " AS \n");
	appendStringInfoString(buf, TextDatumGetCString(viewdef));
#endif
	viewdef_p = TextDatumGetCString(viewdef);
	while (*viewdef_p == ' ')
		viewdef_p++;
	/* End - OA-REQ-1459 */

	/*
	 * When invalidating the view, we do not obtain the view definition directly
	 * from the query text entered by the user, but obtain it through pg_get_viewdef.
	 * Therefore, The view column names are already reflected in the query alias, so
	 * there is no need to store attlist in pg_force_view. the correctness of the meaning
	 * of double quotes in obtaining the view definition should depend on pg_get_viewdef.
	 */
	StoreForceViewQuery(viewid, false, ident_case, viewdef_p, NULL); /* OA-REQ-1459 */

	/* Make change visible */
	CommandCounterIncrement();

	return true;
}


bool
make_view_valid(List *tlist, Oid viewOid, List *options, Query *viewParse)
{
	Relation	rel;
	Relation	relation;
	HeapTuple	tuple;
	Form_pg_class classForm;
	TupleDesc	descriptor;
	List	   *atcmds = NIL;
	AlterTableCmd *atcmd;
	List	   *attrList;
	ListCell   *t;

	if (!rel_is_force_view(viewOid))
		return true;

	/*
	 * create a list of ColumnDef nodes based on the names and types of the
	 * (non-junk) targetlist items from the view's SELECT list.
	 */
	attrList = NIL;
	foreach(t, tlist)
	{
		TargetEntry *tle = (TargetEntry *) lfirst(t);

		if (!tle->resjunk)
		{
			ColumnDef  *def = makeColumnDef(tle->resname,
											exprType((Node *) tle->expr),
											exprTypmod((Node *) tle->expr),
											exprCollation((Node *) tle->expr));

			/*
			 * It's possible that the column is of a collatable type but the
			 * collation could not be resolved, so double-check.
			 */
			if (type_is_collatable(exprType((Node *) tle->expr)))
			{
				if (!OidIsValid(def->collOid))
					ereport(ERROR,
							(errcode(ERRCODE_INDETERMINATE_COLLATION),
							 errmsg("could not determine which collation to use for view column \"%s\"",
									def->colname),
							 errhint("Use the COLLATE clause to set the collation explicitly.")));
			}
			else
				Assert(!OidIsValid(def->collOid));

			attrList = lappend(attrList, def);
		}
	}

	rel = relation_open(viewOid, AccessExclusiveLock);

	/* Make sure it *is* a view. */
	if (rel->rd_rel->relkind != RELKIND_VIEW)
		ereport(ERROR,
				(errcode(ERRCODE_WRONG_OBJECT_TYPE),
				 errmsg("\"%s\" is not a view",
						RelationGetRelationName(rel))));

	/* Check the view not in use */
	CheckTableNotInUse(rel, "COMPILE FORCE VIEW");

	/*
	 * Create a tuple descriptor to compare against the existing view, and
	 * verify that the old column list is an initial prefix of the new
	 * column list.
	 */
	descriptor = BuildDescForRelation(attrList);
	checkViewTupleDesc(descriptor, rel->rd_att);

	/*
	 * The target list of force view is null. Here, we construct
	 * ALTER TABLE ADD COLUMN infrastructure is for force view.
	 */
	if (list_length(attrList) > rel->rd_att->natts)
	{
		ListCell   *c;
		int			skip = rel->rd_att->natts;

		foreach(c, attrList)
		{
			if (skip > 0)
			{
				skip--;
				continue;
			}
			atcmd = makeNode(AlterTableCmd);
			atcmd->subtype = AT_AddColumnToView;
			atcmd->def = (Node *) lfirst(c);
			atcmds = lappend(atcmds, atcmd);
		}

		AlterTableInternal(viewOid, atcmds, true);

		/* Make the new view columns visible */
		CommandCounterIncrement();
	}

	/*
	 * Update the query for the view.
	 *
	 * Note that we must do this before updating the view options, because
	 * the new options may not be compatible with the old view query (for
	 * example if we attempt to add the WITH CHECK OPTION, we require that
	 * the new view be automatically updatable, but the old view may not
	 * have been).
	 */
	StoreViewQuery(viewOid, viewParse, false);

	/* Make the new view query visible */
	CommandCounterIncrement();

	/*
	 * Update the view's options.
	 *
	 * The new options list replaces the existing options list, even if
	 * it's empty.
	 */
	atcmd = makeNode(AlterTableCmd);
	atcmd->subtype = AT_ReplaceRelOptions;
	atcmd->def = (Node *) options;
	atcmds = list_make1(atcmd);

	AlterTableInternal(viewOid, atcmds, true);
	relation_close(rel, NoLock);	/* keep the lock! */

	/* Update the relhasrules of pg_class */
	relation = table_open(RelationRelationId, RowExclusiveLock);
	tuple = SearchSysCacheCopy1(RELOID, ObjectIdGetDatum(viewOid));
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "cache lookup failed for relation %u", viewOid);
	classForm = (Form_pg_class) GETSTRUCT(tuple);
	classForm->relhasrules = true;
	CatalogTupleUpdate(relation, &tuple->t_self, tuple);
	heap_freetuple(tuple);
	table_close(relation, RowExclusiveLock);

	DeleteForceView(viewOid);

	/* Make all change visible */
	CommandCounterIncrement();

	return true;
}
/* End - ReqID:SRS-SQL-FORCEVIEW */

/* Begin - OA-REQ-2513 */
/*
 * make_view_invalid_recursive
 *
 *		Recursively invalidate a view and all views that depend on it.
 */
void
make_view_invalid_recursive(Oid viewoid)
{
	Relation	depRel;
	ScanKeyData	key[2];
	SysScanDesc	scan;
	HeapTuple	depTup;
	HeapTuple	classTup;
	List		*views_to_invalidate = NIL;
	ListCell	*lc;
	Form_pg_class	classForm;

	/* prevent stack overflow */
	check_stack_depth();

	/*
	 * Check if the target view itself is a materialized view?
	 * TODO: force view not support materialized view.
	 */
	classTup = SearchSysCache1(RELOID, ObjectIdGetDatum(viewoid));
	if (!HeapTupleIsValid(classTup))
		elog(ERROR, "cache lookup failed for relation %u", viewoid);

	classForm = (Form_pg_class) GETSTRUCT(classTup);

	if (classForm->relkind == RELKIND_MATVIEW)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("cannot invalidate materialized view"),
				 errdetail("materialized views cannot be invalidated currently.")));

	ReleaseSysCache(classTup);

	/* already be invalidated */
	if (rel_is_force_view(viewoid))
		return;

	/*
	 * Scan pg_depend to find all views that depend on the current view
	 */
	depRel = table_open(DependRelationId, AccessShareLock);

	ScanKeyInit(&key[0],
				Anum_pg_depend_refclassid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(RelationRelationId));
	ScanKeyInit(&key[1],
				Anum_pg_depend_refobjid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(viewoid));

	scan = systable_beginscan(depRel, DependReferenceIndexId, true,
							  NULL, 2, key);

	while (HeapTupleIsValid(depTup = systable_getnext(scan)))
	{
		Relation	rewriteRel_local;
		SysScanDesc	rule_scan;
		ScanKeyData	rule_key[1];
		HeapTuple	rewriteTup;
		Form_pg_rewrite	rewriteForm;
		Form_pg_depend	depForm;
		Oid			dependent_view_oid;

		depForm = (Form_pg_depend) GETSTRUCT(depTup);

		/* not a view? */
		if (depForm->classid != RewriteRelationId || depForm->deptype != DEPENDENCY_NORMAL)
			continue;

		/*
		 * Scan pg_rewrite table directly by rule OID to get ev_class
		 * and rulename
		 */
		rewriteRel_local = table_open(RewriteRelationId, AccessShareLock);

		ScanKeyInit(&rule_key[0],
					Anum_pg_rewrite_oid,
					BTEqualStrategyNumber, F_OIDEQ,
					ObjectIdGetDatum(depForm->objid));

		rule_scan = systable_beginscan(rewriteRel_local, RewriteOidIndexId, true,
									   NULL, 1, rule_key);

		rewriteTup = systable_getnext(rule_scan);

		/* rule still valid? */
		if (!HeapTupleIsValid(rewriteTup))
		{
			systable_endscan(rule_scan);
			table_close(rewriteRel_local, AccessShareLock);
			continue;
		}

		rewriteForm = (Form_pg_rewrite) GETSTRUCT(rewriteTup);
		dependent_view_oid = rewriteForm->ev_class;

		/* not a view */
		if (strcmp(NameStr(rewriteForm->rulename), ViewSelectRuleName) != 0)
		{
			systable_endscan(rule_scan);
			table_close(rewriteRel_local, AccessShareLock);
			continue;
		}

		/* check if the dependent object is a materialized view */
		classTup = SearchSysCache1(RELOID, ObjectIdGetDatum(dependent_view_oid));
		if (!HeapTupleIsValid(classTup))
		{
			systable_endscan(rule_scan);
			table_close(rewriteRel_local, AccessShareLock);
			continue;
		}

		classForm = (Form_pg_class) GETSTRUCT(classTup);

		/* currently materialized view is not support */
		if (classForm->relkind == RELKIND_MATVIEW)
		{
			systable_endscan(rule_scan);
			table_close(rewriteRel_local, AccessShareLock);
			ReleaseSysCache(classTup);
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("cannot invalidate view because a materialized view depends on it"),
					 errdetail("materialized view \"%s\" depends on view \"%s\".",
							   get_rel_name(dependent_view_oid),
							   get_rel_name(viewoid))));
		}

		if (classForm->relkind != RELKIND_VIEW ||
			dependent_view_oid == viewoid)
		{
			systable_endscan(rule_scan);
			table_close(rewriteRel_local, AccessShareLock);
			ReleaseSysCache(classTup);
			continue;
		}

		views_to_invalidate = list_append_unique_oid(views_to_invalidate,
													 dependent_view_oid);

		systable_endscan(rule_scan);
		table_close(rewriteRel_local, AccessShareLock);
		ReleaseSysCache(classTup);
	}

	systable_endscan(scan);
	table_close(depRel, AccessShareLock);

	/*
	 * Recursively invalidate all dependent views
	 */
	foreach(lc, views_to_invalidate)
		make_view_invalid_recursive(lfirst_oid(lc));

	/*
	 * Finally invalidate the target view
	 */
	make_view_invalid(viewoid);
	list_free(views_to_invalidate);
}
/* End - OA-REQ-2513 */