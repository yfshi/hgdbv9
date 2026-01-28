/*-------------------------------------------------------------------------
 *
 * nodeModifyTable.c
 *	  routines to handle ModifyTable nodes.
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/executor/nodeModifyTable.c
 *
 *-------------------------------------------------------------------------
 */
/* INTERFACE ROUTINES
 *		ExecInitModifyTable - initialize the ModifyTable node
 *		ExecModifyTable		- retrieve the next tuple from the node
 *		ExecEndModifyTable	- shut down the ModifyTable node
 *		ExecReScanModifyTable - rescan the ModifyTable node
 *
 *	 NOTES
 *		The ModifyTable node receives input from its outerPlan, which is
 *		the data to insert for INSERT cases, or the changed columns' new
 *		values plus row-locating info for UPDATE cases, or just the
 *		row-locating info for DELETE cases.
 *
 *		The relation to modify can be an ordinary table, a view having an
 *		INSTEAD OF trigger, or a foreign table.  Earlier processing already
 *		pointed ModifyTable to the underlying relations of any automatically
 *		updatable view not using an INSTEAD OF trigger, so code here can
 *		assume it won't have one as a modification target.  This node does
 *		process ri_WithCheckOptions, which may have expressions from those
 *		automatically updatable views.
 *
 *		If the query specifies RETURNING, then the ModifyTable returns a
 *		RETURNING tuple after completing each row insert, update, or delete.
 *		It must be called again to continue the operation.  Without RETURNING,
 *		we just loop within the node until all the work is done, then
 *		return NULL.  This avoids useless call/return overhead.
 */

#include "postgres.h"

#include "access/heapam.h"
#include "access/htup_details.h"
#include "access/tableam.h"
#include "access/xact.h"
#include "catalog/catalog.h"
#include "catalog/storage_gtt.h"
#include "commands/trigger.h"
#include "executor/execPartition.h"
#include "executor/executor.h"
#include "executor/nodeModifyTable.h"
#include "foreign/fdwapi.h"
#include "miscadmin.h"
#include "nodes/nodeFuncs.h"
#include "optimizer/optimizer.h"
#include "rewrite/rewriteHandler.h"
#include "storage/bufmgr.h"
#include "storage/lmgr.h"
#include "utils/builtins.h"
#include "utils/datum.h"
#include "utils/memutils.h"
#include "utils/rel.h"
#include "utils/ora_compatible.h"

/* Begin - ReqID:SRS-SQL-GTT */
#include "catalog/index.h"
#include "access/genam.h"
#include "catalog/storage.h"
/* End - ReqID:SRS-SQL-GTT */

#include "utils/lsyscache.h"

/* Begin - SRS-SQL-MULTITABLEINSERT */
#include "partitioning/partdesc.h"
/* End - SRS-SQL-MULTITABLEINSERT */


typedef struct MTTargetRelLookup
{
	Oid			relationOid;	/* hash key, must be first */
	int			relationIndex;	/* rel's index in resultRelInfo[] array */
} MTTargetRelLookup;


static void ExecBatchInsert(ModifyTableState *mtstate,
							ResultRelInfo *resultRelInfo,
							TupleTableSlot **slots,
							TupleTableSlot **planSlots,
							int numSlots,
							EState *estate,
							bool canSetTag);
static void ExecCrossPartitionUpdateForeignKey(ModifyTableContext *context,
											   ResultRelInfo *sourcePartInfo,
											   ResultRelInfo *destPartInfo,
											   ItemPointer tupleid,
											   TupleTableSlot *oldslot,
											   TupleTableSlot *newslot);
static void ExecPendingInserts(EState *estate);
static bool ExecOnConflictUpdate(ModifyTableContext *context,
										  ResultRelInfo *resultRelInfo,
										  ItemPointer conflictTid,
										  TupleTableSlot *excludedSlot,
										  bool canSetTag,
										  TupleTableSlot **returning);
static TupleTableSlot *ExecPrepareTupleRouting(ModifyTableState *mtstate,
											   EState *estate,
											   PartitionTupleRouting *proute,
											   ResultRelInfo *targetRelInfo,
											   TupleTableSlot *slot,
											   ResultRelInfo **partRelInfo);
static TupleTableSlot *internalGetUpdateNewTuple(ResultRelInfo *relinfo,
												 TupleTableSlot *planSlot,
												 TupleTableSlot *oldSlot,
												 MergeActionState *relaction);

static TupleTableSlot *ExecMerge(ModifyTableContext *context,
								 ResultRelInfo *resultRelInfo,
								 ItemPointer tupleid,
								 bool canSetTag);
static void ExecInitMerge(ModifyTableState *mtstate, EState *estate);
static void ExecMergeNotMatched(ModifyTableContext *context,
								ResultRelInfo *resultRelInfo,
								bool canSetTag);
/* Begin - SRS-SQL-MULTITABLEINSERT */
static void ExecInitMultiInsert(ModifyTableState *mtstate, EState *estate);
static TupleTableSlot *ExecMultiInsert(ModifyTableContext *context,
							ResultRelInfo *resultRelInfo,
							TupleTableSlot *slot,
							bool canSetTag);
/* End - SRS-SQL-MULTITABLEINSERT */


/* Begin - ReqID:SRS-SQL-MERGE */
exec_merge_matched_hook_type pg_exec_merge_matched_hook = ExecMergeMatched;
exec_merge_matched_hook_type ora_exec_merge_matched_hook = NULL;
/* End - ReqID:SRS-SQL-MERGE */

/* Begin - ReqID:SRS-MYSQL-UPDATE */
static void InitMultipleModify(ModifyTableState* node, int32 resultRelationNum);
/* End - ReqID:SRS-MYSQL-UPDATE */

/* Begin - ReqID:SRS-MYSQL-SQL-INSERT */
static bool
ExecConflictUpdate(ModifyTableContext *context,
			  ResultRelInfo *resultRelInfo,
			  ItemPointer conflictTid,
			  TupleTableSlot *excludedSlot,
			  bool canSetTag,
			  TupleTableSlot **returning);
static void
ExecUpsert(ModifyTableContext *context, ModifyTableState* state, TupleTableSlot* slot,
			   TupleTableSlot* planSlot, EState* estate,
			   bool canSetTag, TupleTableSlot** returning,
			   bool* updated, ResultRelInfo *resultRelInfo);

/* End - ReqID:SRS-MYSQL-SQL-INSERT */
/* Begin - BUG#Z4241 */
static void CheckSamepartionResultRelInfo(List *partresultRelInfo, ResultRelInfo *resultRelInfo);
static bool CheckPartitionRelationIsexist(ResultRelInfo *partRelInfo, List *partOids);
static ResultRelInfo * ExecInitCheckpartitionResultInfo(ModifyTableState *mtstate, ModifyTable *node, Index resultRelation);
static bool CheckSamerootResultRelInfo(List *partresultRelInfo, ResultRelInfo *resultRelInfo);
/* End - BUG#Z4241 */


/*
 * Verify that the tuples to be produced by INSERT match the
 * target relation's rowtype
 *
 * We do this to guard against stale plans.  If plan invalidation is
 * functioning properly then we should never get a failure here, but better
 * safe than sorry.  Note that this is called after we have obtained lock
 * on the target rel, so the rowtype can't change underneath us.
 *
 * The plan output is represented by its targetlist, because that makes
 * handling the dropped-column case easier.
 *
 * We used to use this for UPDATE as well, but now the equivalent checks
 * are done in ExecBuildUpdateProjection.
 */
static void
ExecCheckPlanOutput(Relation resultRel, List *targetList)
{
	TupleDesc	resultDesc = RelationGetDescr(resultRel);
	int			attno = 0;
	ListCell   *lc;

	foreach(lc, targetList)
	{
		TargetEntry *tle = (TargetEntry *) lfirst(lc);
		Form_pg_attribute attr;

		Assert(!tle->resjunk);	/* caller removed junk items already */

		if (attno >= resultDesc->natts)
			ereport(ERROR,
					(errcode(ERRCODE_DATATYPE_MISMATCH),
					 errmsg("table row type and query-specified row type do not match"),
					 errdetail("Query has too many columns.")));
		attr = TupleDescAttr(resultDesc, attno);
		attno++;

		/*
		 * Special cases here should match planner's expand_insert_targetlist.
		 */
		if (attr->attisdropped)
		{
			/*
			 * For a dropped column, we can't check atttypid (it's likely 0).
			 * In any case the planner has most likely inserted an INT4 null.
			 * What we insist on is just *some* NULL constant.
			 */
			if (!IsA(tle->expr, Const) ||
				!((Const *) tle->expr)->constisnull)
				ereport(ERROR,
						(errcode(ERRCODE_DATATYPE_MISMATCH),
						 errmsg("table row type and query-specified row type do not match"),
						 errdetail("Query provides a value for a dropped column at ordinal position %d.",
								   attno)));
		}
		else if (attr->attgenerated)
		{
			/*
			 * For a generated column, the planner will have inserted a null
			 * of the column's base type (to avoid possibly failing on domain
			 * not-null constraints).  It doesn't seem worth insisting on that
			 * exact type though, since a null value is type-independent.  As
			 * above, just insist on *some* NULL constant.
			 */
			if (!IsA(tle->expr, Const) ||
				!((Const *) tle->expr)->constisnull)
				ereport(ERROR,
						(errcode(ERRCODE_DATATYPE_MISMATCH),
						 errmsg("table row type and query-specified row type do not match"),
						 errdetail("Query provides a value for a generated column at ordinal position %d.",
								   attno)));
		}
		else
		{
			/* Normal case: demand type match */
			if (exprType((Node *) tle->expr) != attr->atttypid)
				ereport(ERROR,
						(errcode(ERRCODE_DATATYPE_MISMATCH),
						 errmsg("table row type and query-specified row type do not match"),
						 errdetail("Table has type %s at ordinal position %d, but query expects %s.",
								   format_type_be(attr->atttypid),
								   attno,
								   format_type_be(exprType((Node *) tle->expr)))));
		}
	}
	if (attno != resultDesc->natts)
		ereport(ERROR,
				(errcode(ERRCODE_DATATYPE_MISMATCH),
				 errmsg("table row type and query-specified row type do not match"),
				 errdetail("Query has too few columns.")));
}

/*
 * ExecProcessReturning --- evaluate a RETURNING list
 *
 * resultRelInfo: current result rel
 * tupleSlot: slot holding tuple actually inserted/updated/deleted
 * planSlot: slot holding tuple returned by top subplan node
 *
 * Note: If tupleSlot is NULL, the FDW should have already provided econtext's
 * scan tuple.
 *
 * Returns a slot holding the result tuple
 */
static TupleTableSlot *
ExecProcessReturning(ResultRelInfo *resultRelInfo,
					 TupleTableSlot *tupleSlot,
					 TupleTableSlot *planSlot)
{
	ProjectionInfo *projectReturning = resultRelInfo->ri_projectReturning;
	ExprContext *econtext = projectReturning->pi_exprContext;

	/* Make tuple and any needed join variables available to ExecProject */
	if (tupleSlot)
		econtext->ecxt_scantuple = tupleSlot;
	econtext->ecxt_outertuple = planSlot;

	/*
	 * RETURNING expressions might reference the tableoid column, so
	 * reinitialize tts_tableOid before evaluating them.
	 */
	econtext->ecxt_scantuple->tts_tableOid =
		RelationGetRelid(resultRelInfo->ri_RelationDesc);

	/* Compute the RETURNING expressions */
	return ExecProject(projectReturning);
}

/*
 * ExecCheckTupleVisible -- verify tuple is visible
 *
 * It would not be consistent with guarantees of the higher isolation levels to
 * proceed with avoiding insertion (taking speculative insertion's alternative
 * path) on the basis of another tuple that is not visible to MVCC snapshot.
 * Check for the need to raise a serialization failure, and do so as necessary.
 */
static void
ExecCheckTupleVisible(EState *estate,
					  Relation rel,
					  TupleTableSlot *slot)
{
	if (!IsolationUsesXactSnapshot())
		return;

	if (!table_tuple_satisfies_snapshot(rel, slot, estate->es_snapshot))
	{
		Datum		xminDatum;
		TransactionId xmin;
		bool		isnull;

		xminDatum = slot_getsysattr(slot, MinTransactionIdAttributeNumber, &isnull);
		Assert(!isnull);
		xmin = DatumGetTransactionId(xminDatum);

		/*
		 * We should not raise a serialization failure if the conflict is
		 * against a tuple inserted by our own transaction, even if it's not
		 * visible to our snapshot.  (This would happen, for example, if
		 * conflicting keys are proposed for insertion in a single command.)
		 */
		if (!TransactionIdIsCurrentTransactionId(xmin))
			ereport(ERROR,
					(errcode(ERRCODE_T_R_SERIALIZATION_FAILURE),
					 errmsg("could not serialize access due to concurrent update")));
	}
}

/*
 * ExecCheckTIDVisible -- convenience variant of ExecCheckTupleVisible()
 */
static void
ExecCheckTIDVisible(EState *estate,
					ResultRelInfo *relinfo,
					ItemPointer tid,
					TupleTableSlot *tempSlot)
{
	Relation	rel = relinfo->ri_RelationDesc;

	/* Redundantly check isolation level */
	if (!IsolationUsesXactSnapshot())
		return;

	if (!table_tuple_fetch_row_version(rel, tid, SnapshotAny, tempSlot))
		elog(ERROR, "failed to fetch conflicting tuple for ON CONFLICT");
	ExecCheckTupleVisible(estate, rel, tempSlot);
	ExecClearTuple(tempSlot);
}

/*
 * Initialize to compute stored generated columns for a tuple
 *
 * This fills the resultRelInfo's ri_GeneratedExprs field and makes an
 * associated ResultRelInfoExtra struct to hold ri_extraUpdatedCols.
 * (Currently, ri_extraUpdatedCols is consulted only in UPDATE, but we
 * must fill it in other cases too, since for example cmdtype might be
 * MERGE yet an UPDATE might happen later.)
 */
void
ExecInitStoredGenerated(ResultRelInfo *resultRelInfo,
						EState *estate,
						CmdType cmdtype)
{
	Relation	rel = resultRelInfo->ri_RelationDesc;
	TupleDesc	tupdesc = RelationGetDescr(rel);
	int			natts = tupdesc->natts;
	Bitmapset  *updatedCols;
	ResultRelInfoExtra *rextra;
	MemoryContext oldContext;

	/* Don't call twice */
	Assert(resultRelInfo->ri_GeneratedExprs == NULL);

	/* Nothing to do if no generated columns */
	if (!(tupdesc->constr && tupdesc->constr->has_generated_stored))
		return;

	/*
	 * In an UPDATE, we can skip computing any generated columns that do not
	 * depend on any UPDATE target column.  But if there is a BEFORE ROW
	 * UPDATE trigger, we cannot skip because the trigger might change more
	 * columns.
	 */
	if (cmdtype == CMD_UPDATE &&
		!(rel->trigdesc && rel->trigdesc->trig_update_before_row))
		updatedCols = ExecGetUpdatedCols(resultRelInfo, estate);
	else
		updatedCols = NULL;

	/*
	 * Make sure these data structures are built in the per-query memory
	 * context so they'll survive throughout the query.
	 */
	oldContext = MemoryContextSwitchTo(estate->es_query_cxt);

	resultRelInfo->ri_GeneratedExprs =
		(ExprState **) palloc0(natts * sizeof(ExprState *));
	resultRelInfo->ri_NumGeneratedNeeded = 0;

	rextra = palloc_object(ResultRelInfoExtra);
	rextra->rinfo = resultRelInfo;
	rextra->ri_extraUpdatedCols = NULL;
	estate->es_resultrelinfo_extra = lappend(estate->es_resultrelinfo_extra,
											 rextra);

	for (int i = 0; i < natts; i++)
	{
		if (TupleDescAttr(tupdesc, i)->attgenerated == ATTRIBUTE_GENERATED_STORED)
		{
			Expr	   *expr;

			/* Fetch the GENERATED AS expression tree */
			expr = (Expr *) build_column_default(rel, i + 1);
			if (expr == NULL)
				elog(ERROR, "no generation expression found for column number %d of table \"%s\"",
					 i + 1, RelationGetRelationName(rel));

			/*
			 * If it's an update with a known set of update target columns,
			 * see if we can skip the computation.
			 */
			if (updatedCols)
			{
				Bitmapset  *attrs_used = NULL;

				pull_varattnos((Node *) expr, 1, &attrs_used);

				if (!bms_overlap(updatedCols, attrs_used))
					continue;	/* need not update this column */
			}

			/* No luck, so prepare the expression for execution */
			resultRelInfo->ri_GeneratedExprs[i] = ExecPrepareExpr(expr, estate);
			resultRelInfo->ri_NumGeneratedNeeded++;

			/* And mark this column in rextra->ri_extraUpdatedCols */
			rextra->ri_extraUpdatedCols =
				bms_add_member(rextra->ri_extraUpdatedCols,
							   i + 1 - FirstLowInvalidHeapAttributeNumber);
		}
	}

	MemoryContextSwitchTo(oldContext);
}

/*
 * Compute stored generated columns for a tuple
 */
void
ExecComputeStoredGenerated(ResultRelInfo *resultRelInfo,
						   EState *estate, TupleTableSlot *slot,
						   CmdType cmdtype)
{
	Relation	rel = resultRelInfo->ri_RelationDesc;
	TupleDesc	tupdesc = RelationGetDescr(rel);
	int			natts = tupdesc->natts;
	ExprContext *econtext = GetPerTupleExprContext(estate);
	MemoryContext oldContext;
	Datum	   *values;
	bool	   *nulls;

	/* We should not be called unless this is true */
	Assert(tupdesc->constr && tupdesc->constr->has_generated_stored);

	/*
	 * For relations named directly in the query, ExecInitStoredGenerated
	 * should have been called already; but this might not have happened yet
	 * for a partition child rel.  Also, it's convenient for outside callers
	 * to not have to call ExecInitStoredGenerated explicitly.
	 */
	if (resultRelInfo->ri_GeneratedExprs == NULL)
		ExecInitStoredGenerated(resultRelInfo, estate, cmdtype);

	/*
	 * If no generated columns have been affected by this change, then skip
	 * the rest.
	 */
	if (resultRelInfo->ri_NumGeneratedNeeded == 0)
		return;

	oldContext = MemoryContextSwitchTo(GetPerTupleMemoryContext(estate));

	values = palloc(sizeof(*values) * natts);
	nulls = palloc(sizeof(*nulls) * natts);

	slot_getallattrs(slot);
	memcpy(nulls, slot->tts_isnull, sizeof(*nulls) * natts);

	for (int i = 0; i < natts; i++)
	{
		Form_pg_attribute attr = TupleDescAttr(tupdesc, i);

		if (resultRelInfo->ri_GeneratedExprs[i])
		{
			Datum		val;
			bool		isnull;

			Assert(attr->attgenerated == ATTRIBUTE_GENERATED_STORED);

			econtext->ecxt_scantuple = slot;

			val = ExecEvalExpr(resultRelInfo->ri_GeneratedExprs[i], econtext, &isnull);

			/*
			 * We must make a copy of val as we have no guarantees about where
			 * memory for a pass-by-reference Datum is located.
			 */
			if (!isnull)
				val = datumCopy(val, attr->attbyval, attr->attlen);

			values[i] = val;
			nulls[i] = isnull;
		}
		else
		{
			if (!nulls[i])
				values[i] = datumCopy(slot->tts_values[i], attr->attbyval, attr->attlen);
		}
	}

	ExecClearTuple(slot);
	memcpy(slot->tts_values, values, sizeof(*values) * natts);
	memcpy(slot->tts_isnull, nulls, sizeof(*nulls) * natts);
	ExecStoreVirtualTuple(slot);
	ExecMaterializeSlot(slot);

	MemoryContextSwitchTo(oldContext);
}

/*
 * ExecInitInsertProjection
 *		Do one-time initialization of projection data for INSERT tuples.
 *
 * INSERT queries may need a projection to filter out junk attrs in the tlist.
 *
 * This is also a convenient place to verify that the
 * output of an INSERT matches the target table.
 */
static void
ExecInitInsertProjection(ModifyTableState *mtstate,
						 ResultRelInfo *resultRelInfo)
{
	ModifyTable *node = (ModifyTable *) mtstate->ps.plan;
	Plan	   *subplan = outerPlan(node);
	EState	   *estate = mtstate->ps.state;
	List	   *insertTargetList = NIL;
	bool		need_projection = false;
	ListCell   *l;

	/* Extract non-junk columns of the subplan's result tlist. */
	foreach(l, subplan->targetlist)
	{
		TargetEntry *tle = (TargetEntry *) lfirst(l);

		if (!tle->resjunk)
			insertTargetList = lappend(insertTargetList, tle);
		else
			need_projection = true;
	}

	/*
	 * The junk-free list must produce a tuple suitable for the result
	 * relation.
	 */
	ExecCheckPlanOutput(resultRelInfo->ri_RelationDesc, insertTargetList);

	/* We'll need a slot matching the table's format. */
	resultRelInfo->ri_newTupleSlot =
		table_slot_create(resultRelInfo->ri_RelationDesc,
						  &estate->es_tupleTable);

	/* Build ProjectionInfo if needed (it probably isn't). */
	if (need_projection)
	{
		TupleDesc	relDesc = RelationGetDescr(resultRelInfo->ri_RelationDesc);

		/* need an expression context to do the projection */
		if (mtstate->ps.ps_ExprContext == NULL)
			ExecAssignExprContext(estate, &mtstate->ps);

		resultRelInfo->ri_projectNew =
			ExecBuildProjectionInfo(insertTargetList,
									mtstate->ps.ps_ExprContext,
									resultRelInfo->ri_newTupleSlot,
									&mtstate->ps,
									relDesc);
	}

	resultRelInfo->ri_projectNewInfoValid = true;
}

/*
 * ExecInitUpdateProjection
 *		Do one-time initialization of projection data for UPDATE tuples.
 *
 * UPDATE always needs a projection, because (1) there's always some junk
 * attrs, and (2) we may need to merge values of not-updated columns from
 * the old tuple into the final tuple.  In UPDATE, the tuple arriving from
 * the subplan contains only new values for the changed columns, plus row
 * identity info in the junk attrs.
 *
 * This is "one-time" for any given result rel, but we might touch more than
 * one result rel in the course of an inherited UPDATE, and each one needs
 * its own projection due to possible column order variation.
 *
 * This is also a convenient place to verify that the output of an UPDATE
 * matches the target table (ExecBuildUpdateProjection does that).
 */
static void
ExecInitUpdateProjection(ModifyTableState *mtstate,
						 ResultRelInfo *resultRelInfo)
{
	ModifyTable *node = (ModifyTable *) mtstate->ps.plan;
	Plan	   *subplan = outerPlan(node);
	EState	   *estate = mtstate->ps.state;
	TupleDesc	relDesc = RelationGetDescr(resultRelInfo->ri_RelationDesc);
	int			whichrel;
	List	   *updateColnos = NIL;	/* ReqID:SRS-MYSQL-UPDATE */

	/*
	 * Usually, mt_lastResultIndex matches the target rel.  If it happens not
	 * to, we can get the index the hard way with an integer division.
	 */
	whichrel = mtstate->mt_lastResultIndex;
	if (resultRelInfo != mtstate->resultRelInfo + whichrel)
	{
		whichrel = resultRelInfo - mtstate->resultRelInfo;
		Assert(whichrel >= 0 && whichrel < mtstate->mt_nrels);
	}

	updateColnos = (List *) list_nth(node->updateColnosLists, whichrel);

	/*
	 * For UPDATE, we use the old tuple to fill up missing values in the tuple
	 * produced by the subplan to get the new tuple.  We need two slots, both
	 * matching the table's desired format.
	 */
	resultRelInfo->ri_oldTupleSlot =
		table_slot_create(resultRelInfo->ri_RelationDesc,
						  &estate->es_tupleTable);
	resultRelInfo->ri_newTupleSlot =
		table_slot_create(resultRelInfo->ri_RelationDesc,
						  &estate->es_tupleTable);

	/* need an expression context to do the projection */
	if (mtstate->ps.ps_ExprContext == NULL)
		ExecAssignExprContext(estate, &mtstate->ps);

	resultRelInfo->ri_projectNew =
		ExecBuildUpdateProjection(subplan->targetlist,
								  false,	/* subplan did the evaluation */
								  updateColnos,
								  relDesc,
								  mtstate->ps.ps_ExprContext,
								  resultRelInfo->ri_newTupleSlot,
								  &mtstate->ps);

	resultRelInfo->ri_projectNewInfoValid = true;
}


/* Begin - ReqID:SRS-MYSQL-UPDATE */
/*
 * ExecInitMultilUpdateProjection
 *		Initialize the projection data of the UPDATE tuple once
 *		according to the target column of the corresponding relationship.
 */
static void
ExecInitMultilUpdateProjection(ModifyTableState *mtstate, List *targetList,
										  ResultRelInfo *resultRelInfo, int whichrel)
{
	ModifyTable *node = (ModifyTable *) mtstate->ps.plan;

	EState 	*estate = mtstate->ps.state;
	TupleDesc	 relDesc = RelationGetDescr(resultRelInfo->ri_RelationDesc);
	List	   *updateColnos = NIL;

	if (resultRelInfo->ri_RootResultRelInfo)
	{
		for(int i = 0; i < mtstate->mt_nrels; i++)
		{
			if (resultRelInfo == (mtstate->resultRelInfo + i))
			{
				whichrel = i;
				break;
			}
		}
	}

	updateColnos = (List *) list_nth(node->updateColnosLists, whichrel);

	/*
	 * For UPDATE, we use the old tuple to fill up missing values in the tuple
	 * produced by the subplan to get the new tuple.  We need two slots, both
	 * matching the table's desired format.
	 */
	resultRelInfo->ri_oldTupleSlot =
		table_slot_create(resultRelInfo->ri_RelationDesc,
						   &estate->es_tupleTable);
	resultRelInfo->ri_newTupleSlot =
		table_slot_create(resultRelInfo->ri_RelationDesc,
						   &estate->es_tupleTable);

	/* need an expression context to do the projection */
	if (mtstate->ps.ps_ExprContext == NULL)
		ExecAssignExprContext(estate, &mtstate->ps);

	if (resultRelInfo->ri_RootResultRelInfo)
	{
		resultRelInfo->ri_projectNew =
			ExecBuildMutilUpdateProjection(targetList,
										   false,	 /* subplan didthe evaluation */
										   updateColnos,
										   relDesc,
										   mtstate->ps.ps_ExprContext,
										   resultRelInfo->ri_newTupleSlot,
										   &mtstate->ps,
										   resultRelInfo->ri_RootResultRelInfo->ri_RangeTableIndex);
	}
	else
		resultRelInfo->ri_projectNew =
			ExecBuildMutilUpdateProjection(targetList,
										   false,	 /* subplan did the evaluation */
										   updateColnos,
										   relDesc,
										   mtstate->ps.ps_ExprContext,
										   resultRelInfo->ri_newTupleSlot,
										   &mtstate->ps,
										   resultRelInfo->ri_RangeTableIndex);

	resultRelInfo->ri_projectNewInfoValid = true;
}
/* End - ReqID:SRS-MYSQL-UPDATE */


/*
 * ExecGetInsertNewTuple
 *		This prepares a "new" tuple ready to be inserted into given result
 *		relation, by removing any junk columns of the plan's output tuple
 *		and (if necessary) coercing the tuple to the right tuple format.
 */
static TupleTableSlot *
ExecGetInsertNewTuple(ResultRelInfo *relinfo,
					  TupleTableSlot *planSlot)
{
	ProjectionInfo *newProj = relinfo->ri_projectNew;
	ExprContext *econtext;

	/*
	 * If there's no projection to be done, just make sure the slot is of the
	 * right type for the target rel.  If the planSlot is the right type we
	 * can use it as-is, else copy the data into ri_newTupleSlot.
	 */
	if (newProj == NULL)
	{
		if (relinfo->ri_newTupleSlot->tts_ops != planSlot->tts_ops)
		{
			ExecCopySlot(relinfo->ri_newTupleSlot, planSlot);
			return relinfo->ri_newTupleSlot;
		}
		else
			return planSlot;
	}

	/*
	 * Else project; since the projection output slot is ri_newTupleSlot, this
	 * will also fix any slot-type problem.
	 *
	 * Note: currently, this is dead code, because INSERT cases don't receive
	 * any junk columns so there's never a projection to be done.
	 */
	econtext = newProj->pi_exprContext;
	econtext->ecxt_outertuple = planSlot;
	return ExecProject(newProj);
}

/*
 * ExecGetUpdateNewTuple
 *		This prepares a "new" tuple by combining an UPDATE subplan's output
 *		tuple (which contains values of changed columns) with unchanged
 *		columns taken from the old tuple.
 *
 * The subplan tuple might also contain junk columns, which are ignored.
 * Note that the projection also ensures we have a slot of the right type.
 */
TupleTableSlot *
ExecGetUpdateNewTuple(ResultRelInfo *relinfo,
					  TupleTableSlot *planSlot,
					  TupleTableSlot *oldSlot)
{
	/* Use a few extra Asserts to protect against outside callers */
	Assert(relinfo->ri_projectNewInfoValid);
	Assert(planSlot != NULL && !TTS_EMPTY(planSlot));
	Assert(oldSlot != NULL && !TTS_EMPTY(oldSlot));

	return internalGetUpdateNewTuple(relinfo, planSlot, oldSlot, NULL);
}

/*
 * Callback for ModifyTableState->GetUpdateNewTuple for use by regular UPDATE.
 */
static TupleTableSlot *
internalGetUpdateNewTuple(ResultRelInfo *relinfo,
						  TupleTableSlot *planSlot,
						  TupleTableSlot *oldSlot,
						  MergeActionState *relaction)
{
	ProjectionInfo *newProj = relinfo->ri_projectNew;
	ExprContext *econtext;

	econtext = newProj->pi_exprContext;
	econtext->ecxt_outertuple = planSlot;
	econtext->ecxt_scantuple = oldSlot;
	return ExecProject(newProj);
}

/* Begin - ReqID:SRS-MYSQL-SQL-INSERT */
/*
 * ExecUpsert - execute insert ... on duplicate key update.
 */
static void
ExecUpsert(ModifyTableContext *context, ModifyTableState* state, TupleTableSlot* slot, TupleTableSlot* planSlot, EState* estate,
			   bool canSetTag, TupleTableSlot** returning, bool* updated, ResultRelInfo *resultRelInfo)
{
	bool			specConflict = false;
	List		   *recheckIndexes = NIL;
	Relation		resultRelationDesc = NULL;
	Relation		heaprel = NULL; /* actual relation to upsert index */
	Relation		targetrel = NULL; /* actual relation to upsert tuple */
	UpsertState	   *upsertState = state->mt_upsert;
	uint32			specToken;
//	RangeTblEntry  *rte;
	ItemPointerData conflictTid;

	*updated = false;

	/*
	 * get information on the (current) result relation
	 */
	resultRelationDesc = resultRelInfo->ri_RelationDesc;
	heaprel = resultRelationDesc;

//	ConstraintsForExecUpsert(resultRelationDesc);

//	rte = exec_rt_fetch(resultRelInfo->ri_RangeTableIndex, estate);
	vlock:
	targetrel = heaprel;

	specConflict = false;

	if (!ExecCheckIndexConstraints(resultRelInfo, slot, estate, &conflictTid, NIL))
	{

		/* committed conflict tuple found */
		if (upsertState->us_action == UPSERT_UPDATE)
		{
			/*
			 * In case of ON DUPLICATE KEY UPDATE, execute the UPDATE part.
			 * Be prepared to retry if the UPDATE fails because
			 * of another concurrent UPDATE/DELETE to the conflict tuple.
			 */
			*returning = NULL;

			if (ExecConflictUpdate(context, resultRelInfo, &conflictTid, slot, canSetTag, returning))
			{
				InstrCountFiltered2(&state->ps, 1);
				*updated = true;
				return;
			}
			else
			{
				goto vlock;
			}
		}
		else
		{
			/*
			 * In case of DUPLICATE UPDATE NOTHING, do nothing.
			 * However, verify that the tuple is visible to the
			 * executor's MVCC snapshot at higher isolation levels.
			 */
			Assert(upsertState->us_action == UPSERT_NOTHING);
			ExecCheckTIDVisible(estate, resultRelInfo, &conflictTid,
								ExecGetReturningSlot(estate, resultRelInfo));
			InstrCountFiltered2(&state->ps, 1);
			*updated = true;
			return;
		}
	}

	/* insert the tuple */
	//table_tuple_insert(targetrel, slot, estate->es_output_cid, 0, NULL);
	specToken = SpeculativeInsertionLockAcquire(GetCurrentTransactionId());

	/* insert the tuple, with the speculative token */
	table_tuple_insert_speculative(targetrel, slot,
								   estate->es_output_cid,
								   0,
								   NULL,
								   specToken);

	/* insert index entries for tuple */
	recheckIndexes = ExecInsertIndexTuples(resultRelInfo,
										   slot, estate, false, true,
										   &specConflict,
										   NULL);

	/* adjust the tuple's state accordingly */
	table_tuple_complete_speculative(resultRelationDesc, slot,
									 specToken, !specConflict);

	/*
	 * Wake up anyone waiting for our decision.  They will re-check
	 * the tuple, see that it's no longer speculative, and wait on our
	 * XID as if this was a regularly inserted tuple all along.  Or if
	 * we killed the tuple, they will see it's dead, and proceed as if
	 * the tuple never existed.
	 */
	SpeculativeInsertionLockRelease(GetCurrentTransactionId());
	/* other transaction commit index insertion before us,
	 * then abort the tuple and try to find the conflict tuple again
	 */
	if (specConflict)
	{
		list_free(recheckIndexes);
		goto vlock;
	}

	return;
}
/* End - ReqID:SRS-MYSQL-SQL-INSERT */

/* Begin - ReqID:SRS-MYSQL-SQL-REPLACE */
/*
 * CheckPartitionRelationIsexist
 * Check whether the partition is the specified partition.
 */
static bool
CheckPartitionRelationIsexist(ResultRelInfo *partRelInfo, List *partOids)
{
	bool		partflag = false;
	ListCell   *lc = NULL;
	Oid			partreloid = InvalidOid;

	/* Get the oid of the partition relation. */
	if (partRelInfo->ri_RelationDesc)
		partreloid = partRelInfo->ri_RelationDesc->rd_node.relNode;
	/* Traverse all specified partitions relation and report an error if not found. */
	foreach(lc, partOids)
	{
		Oid		partoid = lfirst_int(lc);
		if (partoid == partreloid)
		{
			partflag = true;
			break;
		}
	}
	return partflag;
}
/* End - ReqID:SRS-MYSQL-SQL-REPLACE */


/* ----------------------------------------------------------------
 *		ExecInsert
 *
 *		For INSERT, we have to insert the tuple into the target relation
 *		(or partition thereof) and insert appropriate tuples into the index
 *		relations.
 *
 *		slot contains the new tuple value to be stored.
 *		planSlot is the output of the ModifyTable's subplan; we use it
 *		to access "junk" columns that are not going to be stored.
 *
 *		Returns RETURNING result if any, otherwise NULL.
 *
 *		This may change the currently active tuple conversion map in
 *		mtstate->mt_transition_capture, so the callers must take care to
 *		save the previous value to avoid losing track of it.
 * ----------------------------------------------------------------
 */
static TupleTableSlot *
ExecInsert(ModifyTableContext *context,
		   ResultRelInfo *resultRelInfo,
		   TupleTableSlot *slot,
		   bool canSetTag,
		   TupleTableSlot **inserted_tuple,
		   ResultRelInfo **insert_destrel)
{
	ModifyTableState *mtstate = context->mtstate;
	EState	   *estate = context->estate;
	Relation	resultRelationDesc;
	List	   *recheckIndexes = NIL;
	TupleTableSlot *result = NULL;
	TupleTableSlot *planSlot = context->planSlot;
	TransitionCaptureState *ar_insert_trig_tcs;
	ModifyTable *node = (ModifyTable *) mtstate->ps.plan;
	OnConflictAction onconflict = node->onConflictAction;
	PartitionTupleRouting *proute = mtstate->mt_partition_tuple_routing;
	MemoryContext oldContext;

	UpsertAction upsertAction = node->upsertAction; /* ReqID:SRS-MYSQL-SQL-INSERT */
	/*
	 * If the input result relation is a partitioned table, find the leaf
	 * partition to insert the tuple into.
	 */
	if (proute)
	{
		ResultRelInfo *partRelInfo;

		slot = ExecPrepareTupleRouting(mtstate, estate, proute,
									   resultRelInfo, slot,
									   &partRelInfo);
		/* Begin - ReqID:SRS-MYSQL-SQL-REPLACE */
		if (MYS_MODE == compatible_mode && mtstate->mt_partOids) /* BUG#Z5121 */
		{
			if (!CheckPartitionRelationIsexist(partRelInfo, mtstate->mt_partOids))
				ereport(ERROR,
						(errcode(ERRCODE_UNDEFINED_TABLE),
						 errmsg("Found a row not matching the given partition set.")));
		}
		/* End - ReqID:SRS-MYSQL-SQL-REPLACE */
		resultRelInfo = partRelInfo;
	}

	ExecMaterializeSlot(slot);

	resultRelationDesc = resultRelInfo->ri_RelationDesc;

	/*
	 * Open the table's indexes, if we have not done so already, so that we
	 * can add new index entries for the inserted tuple.
	 */
	if (resultRelationDesc->rd_rel->relhasindex &&
		resultRelInfo->ri_IndexRelationDescs == NULL)
		ExecOpenIndices(resultRelInfo, onconflict != ONCONFLICT_NONE);

	/*
	 * BEFORE ROW INSERT Triggers.
	 *
	 * Note: We fire BEFORE ROW TRIGGERS for every attempted insertion in an
	 * INSERT ... ON CONFLICT statement.  We cannot check for constraint
	 * violations before firing these triggers, because they can change the
	 * values to insert.  Also, they can run arbitrary user-defined code with
	 * side-effects that we can't cancel by just not inserting the tuple.
	 */
	if (resultRelInfo->ri_TrigDesc &&
		resultRelInfo->ri_TrigDesc->trig_insert_before_row)
	{
		/* Flush any pending inserts, so rows are visible to the triggers */
		if (estate->es_insert_pending_result_relations != NIL)
			ExecPendingInserts(estate);

		if (!ExecBRInsertTriggers(estate, resultRelInfo, slot))
			return NULL;		/* "do nothing" */
	}

	/* INSTEAD OF ROW INSERT Triggers */
	if (resultRelInfo->ri_TrigDesc &&
		resultRelInfo->ri_TrigDesc->trig_insert_instead_row)
	{
		if (!ExecIRInsertTriggers(estate, resultRelInfo, slot))
			return NULL;		/* "do nothing" */
	}
	else if (resultRelInfo->ri_FdwRoutine)
	{
		/*
		 * GENERATED expressions might reference the tableoid column, so
		 * (re-)initialize tts_tableOid before evaluating them.
		 */
		slot->tts_tableOid = RelationGetRelid(resultRelInfo->ri_RelationDesc);

		/*
		 * Compute stored generated columns
		 */
		if (resultRelationDesc->rd_att->constr &&
			resultRelationDesc->rd_att->constr->has_generated_stored)
			ExecComputeStoredGenerated(resultRelInfo, estate, slot,
									   CMD_INSERT);

		/*
		 * If the FDW supports batching, and batching is requested, accumulate
		 * rows and insert them in batches. Otherwise use the per-row inserts.
		 */
		if (resultRelInfo->ri_BatchSize > 1)
		{
			bool		flushed = false;

			/*
			 * When we've reached the desired batch size, perform the
			 * insertion.
			 */
			if (resultRelInfo->ri_NumSlots == resultRelInfo->ri_BatchSize)
			{
				ExecBatchInsert(mtstate, resultRelInfo,
								resultRelInfo->ri_Slots,
								resultRelInfo->ri_PlanSlots,
								resultRelInfo->ri_NumSlots,
								estate, canSetTag);
				flushed = true;
			}

			oldContext = MemoryContextSwitchTo(estate->es_query_cxt);

			if (resultRelInfo->ri_Slots == NULL)
			{
				resultRelInfo->ri_Slots = palloc(sizeof(TupleTableSlot *) *
												 resultRelInfo->ri_BatchSize);
				resultRelInfo->ri_PlanSlots = palloc(sizeof(TupleTableSlot *) *
													 resultRelInfo->ri_BatchSize);
			}

			/*
			 * Initialize the batch slots. We don't know how many slots will
			 * be needed, so we initialize them as the batch grows, and we
			 * keep them across batches. To mitigate an inefficiency in how
			 * resource owner handles objects with many references (as with
			 * many slots all referencing the same tuple descriptor) we copy
			 * the appropriate tuple descriptor for each slot.
			 */
			if (resultRelInfo->ri_NumSlots >= resultRelInfo->ri_NumSlotsInitialized)
			{
				TupleDesc	tdesc = CreateTupleDescCopy(slot->tts_tupleDescriptor);
				TupleDesc	plan_tdesc =
					CreateTupleDescCopy(planSlot->tts_tupleDescriptor);

				resultRelInfo->ri_Slots[resultRelInfo->ri_NumSlots] =
					MakeSingleTupleTableSlot(tdesc, slot->tts_ops);

				resultRelInfo->ri_PlanSlots[resultRelInfo->ri_NumSlots] =
					MakeSingleTupleTableSlot(plan_tdesc, planSlot->tts_ops);

				/* remember how many batch slots we initialized */
				resultRelInfo->ri_NumSlotsInitialized++;
			}

			ExecCopySlot(resultRelInfo->ri_Slots[resultRelInfo->ri_NumSlots],
						 slot);

			ExecCopySlot(resultRelInfo->ri_PlanSlots[resultRelInfo->ri_NumSlots],
						 planSlot);

			/*
			 * If these are the first tuples stored in the buffers, add the
			 * target rel and the mtstate to the
			 * es_insert_pending_result_relations and
			 * es_insert_pending_modifytables lists respectively, execpt in
			 * the case where flushing was done above, in which case they
			 * would already have been added to the lists, so no need to do
			 * this.
			 */
			if (resultRelInfo->ri_NumSlots == 0 && !flushed)
			{
				Assert(!list_member_ptr(estate->es_insert_pending_result_relations,
										resultRelInfo));
				estate->es_insert_pending_result_relations =
					lappend(estate->es_insert_pending_result_relations,
							resultRelInfo);
				estate->es_insert_pending_modifytables =
					lappend(estate->es_insert_pending_modifytables, mtstate);
			}
			Assert(list_member_ptr(estate->es_insert_pending_result_relations,
								   resultRelInfo));

			resultRelInfo->ri_NumSlots++;

			MemoryContextSwitchTo(oldContext);

			return NULL;
		}

		/*
		 * insert into foreign table: let the FDW do it
		 */
		slot = resultRelInfo->ri_FdwRoutine->ExecForeignInsert(estate,
															   resultRelInfo,
															   slot,
															   planSlot);

		if (slot == NULL)		/* "do nothing" */
			return NULL;

		/*
		 * AFTER ROW Triggers or RETURNING expressions might reference the
		 * tableoid column, so (re-)initialize tts_tableOid before evaluating
		 * them.  (This covers the case where the FDW replaced the slot.)
		 */
		slot->tts_tableOid = RelationGetRelid(resultRelInfo->ri_RelationDesc);
	}
	else
	{
		WCOKind		wco_kind;

		/*
		 * Constraints and GENERATED expressions might reference the tableoid
		 * column, so (re-)initialize tts_tableOid before evaluating them.
		 */
		slot->tts_tableOid = RelationGetRelid(resultRelationDesc);

		/*
		 * Compute stored generated columns
		 */
		if (resultRelationDesc->rd_att->constr &&
			resultRelationDesc->rd_att->constr->has_generated_stored)
			ExecComputeStoredGenerated(resultRelInfo, estate, slot,
									   CMD_INSERT);

		/*
		 * Check any RLS WITH CHECK policies.
		 *
		 * Normally we should check INSERT policies. But if the insert is the
		 * result of a partition key update that moved the tuple to a new
		 * partition, we should instead check UPDATE policies, because we are
		 * executing policies defined on the target table, and not those
		 * defined on the child partitions.
		 *
		 * If we're running MERGE, we refer to the action that we're executing
		 * to know if we're doing an INSERT or UPDATE to a partition table.
		 */
		if (mtstate->operation == CMD_UPDATE)
			wco_kind = WCO_RLS_UPDATE_CHECK;
		else if (mtstate->operation == CMD_MERGE)
			wco_kind = (context->relaction->mas_action->commandType == CMD_UPDATE) ?
				WCO_RLS_UPDATE_CHECK : WCO_RLS_INSERT_CHECK;
		else
			wco_kind = WCO_RLS_INSERT_CHECK;

		/*
		 * ExecWithCheckOptions() will skip any WCOs which are not of the kind
		 * we are looking for at this point.
		 */
		if (resultRelInfo->ri_WithCheckOptions != NIL)
			ExecWithCheckOptions(wco_kind, resultRelInfo, slot, estate);

		/*
		 * Check the constraints of the tuple.
		 */
		if (resultRelationDesc->rd_att->constr)
			ExecConstraints(resultRelInfo, slot, estate);

		/*
		 * Also check the tuple against the partition constraint, if there is
		 * one; except that if we got here via tuple-routing, we don't need to
		 * if there's no BR trigger defined on the partition.
		 */
		if (resultRelationDesc->rd_rel->relispartition &&
			(resultRelInfo->ri_RootResultRelInfo == NULL ||
			 (resultRelInfo->ri_TrigDesc &&
			  resultRelInfo->ri_TrigDesc->trig_insert_before_row)))
			ExecPartitionCheck(resultRelInfo, slot, estate, true);

		if (onconflict != ONCONFLICT_NONE && resultRelInfo->ri_NumIndices > 0)
		{
			/* Perform a speculative insertion. */
			uint32		specToken;
			ItemPointerData conflictTid;
			bool		specConflict;
			List	   *arbiterIndexes;

			arbiterIndexes = resultRelInfo->ri_onConflictArbiterIndexes;

			/*
			 * Do a non-conclusive check for conflicts first.
			 *
			 * We're not holding any locks yet, so this doesn't guarantee that
			 * the later insert won't conflict.  But it avoids leaving behind
			 * a lot of canceled speculative insertions, if you run a lot of
			 * INSERT ON CONFLICT statements that do conflict.
			 *
			 * We loop back here if we find a conflict below, either during
			 * the pre-check, or when we re-check after inserting the tuple
			 * speculatively.  Better allow interrupts in case some bug makes
			 * this an infinite loop.
			 */
	vlock:
			CHECK_FOR_INTERRUPTS();
			specConflict = false;
			if (!ExecCheckIndexConstraints(resultRelInfo, slot, estate,
										   &conflictTid, arbiterIndexes))
			{
				/* committed conflict tuple found */
				if (onconflict == ONCONFLICT_UPDATE)
				{
					/*
					 * In case of ON CONFLICT DO UPDATE, execute the UPDATE
					 * part.  Be prepared to retry if the UPDATE fails because
					 * of another concurrent UPDATE/DELETE to the conflict
					 * tuple.
					 */
					TupleTableSlot *returning = NULL;

					if (ExecOnConflictUpdate(context, resultRelInfo,
											 &conflictTid, slot, canSetTag,
											 &returning))
					{
						InstrCountTuples2(&mtstate->ps, 1);
						return returning;
					}
					else
						goto vlock;
				}
				else
				{
					/*
					 * In case of ON CONFLICT DO NOTHING, do nothing. However,
					 * verify that the tuple is visible to the executor's MVCC
					 * snapshot at higher isolation levels.
					 *
					 * Using ExecGetReturningSlot() to store the tuple for the
					 * recheck isn't that pretty, but we can't trivially use
					 * the input slot, because it might not be of a compatible
					 * type. As there's no conflicting usage of
					 * ExecGetReturningSlot() in the DO NOTHING case...
					 */
					Assert(onconflict == ONCONFLICT_NOTHING);
					ExecCheckTIDVisible(estate, resultRelInfo, &conflictTid,
										ExecGetReturningSlot(estate, resultRelInfo));
					InstrCountTuples2(&mtstate->ps, 1);
					return NULL;
				}
			}

			/*
			 * Before we start insertion proper, acquire our "speculative
			 * insertion lock".  Others can use that to wait for us to decide
			 * if we're going to go ahead with the insertion, instead of
			 * waiting for the whole transaction to complete.
			 */
			specToken = SpeculativeInsertionLockAcquire(GetCurrentTransactionId());

			/* insert the tuple, with the speculative token */
			table_tuple_insert_speculative(resultRelationDesc, slot,
										   estate->es_output_cid,
										   0,
										   NULL,
										   specToken);

			/* insert index entries for tuple */
			recheckIndexes = ExecInsertIndexTuples(resultRelInfo,
												   slot, estate, false, true,
												   &specConflict,
												   arbiterIndexes);

			/* adjust the tuple's state accordingly */
			table_tuple_complete_speculative(resultRelationDesc, slot,
											 specToken, !specConflict);

			/*
			 * Wake up anyone waiting for our decision.  They will re-check
			 * the tuple, see that it's no longer speculative, and wait on our
			 * XID as if this was a regularly inserted tuple all along.  Or if
			 * we killed the tuple, they will see it's dead, and proceed as if
			 * the tuple never existed.
			 */
			SpeculativeInsertionLockRelease(GetCurrentTransactionId());

			/*
			 * If there was a conflict, start from the beginning.  We'll do
			 * the pre-check again, which will now find the conflicting tuple
			 * (unless it aborts before we get there).
			 */
			if (specConflict)
			{
				list_free(recheckIndexes);
				goto vlock;
			}

			/* Since there was no insertion conflict, we're done */
		}
		/* Begin - ReqID:SRS-MYSQL-SQL-INSERT */
		else if (upsertAction != UPSERT_NONE && resultRelInfo->ri_NumIndices > 0)
		{
			TupleTableSlot   *returning = NULL;
			bool			updated = false;

			/* Get project node information. */
			if (!resultRelInfo->ri_updateProj &&
				mtstate->resultRelInfo->ri_updateProj)
				resultRelInfo->ri_updateProj = mtstate->resultRelInfo->ri_updateProj;

			ExecUpsert(context, mtstate, slot, planSlot, estate, canSetTag, &returning, &updated, resultRelInfo);
			if (updated)
			{
				return returning;
			}
		}
		/* End - ReqID:SRS-MYSQL-SQL-INSERT */
		else
		{
			/* insert the tuple normally */
			table_tuple_insert(resultRelationDesc, slot,
							   estate->es_output_cid,
							   0, NULL);

			/* insert index entries for tuple */
			if (resultRelInfo->ri_NumIndices > 0)
				recheckIndexes = ExecInsertIndexTuples(resultRelInfo,
													   slot, estate, false,
													   false, NULL, NIL);
		}
	}

	/*
	 * there, multi table insert convert to insert + cte
	 * we should inclue cte insert rows
	 */
	if (canSetTag || estate->es_multi_insert_usecte) /* SRS-SQL-MULTITABLEINSERT */
		(estate->es_processed)++;

	/*
	 * If this insert is the result of a partition key update that moved the
	 * tuple to a new partition, put this row into the transition NEW TABLE,
	 * if there is one. We need to do this separately for DELETE and INSERT
	 * because they happen on different tables.
	 */
	ar_insert_trig_tcs = mtstate->mt_transition_capture;
	if (mtstate->operation == CMD_UPDATE && mtstate->mt_transition_capture
		&& mtstate->mt_transition_capture->tcs_update_new_table)
	{
		ExecARUpdateTriggers(estate, resultRelInfo,
							 NULL, NULL,
							 NULL,
							 NULL,
							 slot,
							 NULL,
							 mtstate->mt_transition_capture,
							 false);

		/*
		 * We've already captured the NEW TABLE row, so make sure any AR
		 * INSERT trigger fired below doesn't capture it again.
		 */
		ar_insert_trig_tcs = NULL;
	}

	/* AFTER ROW INSERT Triggers */
	ExecARInsertTriggers(estate, resultRelInfo, slot, recheckIndexes,
						 ar_insert_trig_tcs);

	list_free(recheckIndexes);

	/*
	 * Check any WITH CHECK OPTION constraints from parent views.  We are
	 * required to do this after testing all constraints and uniqueness
	 * violations per the SQL spec, so we do it after actually inserting the
	 * record into the heap and all indexes.
	 *
	 * ExecWithCheckOptions will elog(ERROR) if a violation is found, so the
	 * tuple will never be seen, if it violates the WITH CHECK OPTION.
	 *
	 * ExecWithCheckOptions() will skip any WCOs which are not of the kind we
	 * are looking for at this point.
	 */
	if (resultRelInfo->ri_WithCheckOptions != NIL)
		ExecWithCheckOptions(WCO_VIEW_CHECK, resultRelInfo, slot, estate);

	/* Process RETURNING if present */
	if (resultRelInfo->ri_projectReturning)
		result = ExecProcessReturning(resultRelInfo, slot, planSlot);

	if (inserted_tuple)
		*inserted_tuple = slot;
	if (insert_destrel)
		*insert_destrel = resultRelInfo;

	return result;
}


/* ----------------------------------------------------------------
 *		ExecBatchInsert
 *
 *		Insert multiple tuples in an efficient way.
 *		Currently, this handles inserting into a foreign table without
 *		RETURNING clause.
 * ----------------------------------------------------------------
 */
static void
ExecBatchInsert(ModifyTableState *mtstate,
				ResultRelInfo *resultRelInfo,
				TupleTableSlot **slots,
				TupleTableSlot **planSlots,
				int numSlots,
				EState *estate,
				bool canSetTag)
{
	int			i;
	int			numInserted = numSlots;
	TupleTableSlot *slot = NULL;
	TupleTableSlot **rslots;

	/*
	 * insert into foreign table: let the FDW do it
	 */
	rslots = resultRelInfo->ri_FdwRoutine->ExecForeignBatchInsert(estate,
																  resultRelInfo,
																  slots,
																  planSlots,
																  &numInserted);

	for (i = 0; i < numInserted; i++)
	{
		slot = rslots[i];

		/*
		 * AFTER ROW Triggers might reference the tableoid column, so
		 * (re-)initialize tts_tableOid before evaluating them.
		 */
		slot->tts_tableOid = RelationGetRelid(resultRelInfo->ri_RelationDesc);

		/* AFTER ROW INSERT Triggers */
		ExecARInsertTriggers(estate, resultRelInfo, slot, NIL,
							 mtstate->mt_transition_capture);

		/*
		 * Check any WITH CHECK OPTION constraints from parent views.  See the
		 * comment in ExecInsert.
		 */
		if (resultRelInfo->ri_WithCheckOptions != NIL)
			ExecWithCheckOptions(WCO_VIEW_CHECK, resultRelInfo, slot, estate);
	}

	if (canSetTag && numInserted > 0)
		estate->es_processed += numInserted;
		
	/* Clean up all the slots, ready for the next batch */
	for (i = 0; i < numSlots; i++)
	{
		ExecClearTuple(slots[i]);
		ExecClearTuple(planSlots[i]);
	}
	resultRelInfo->ri_NumSlots = 0;
}

/*
 * ExecDeletePrologue -- subroutine for ExecDelete
 *
 * Prepare executor state for DELETE.  Actually, the only thing we have to do
 * here is execute BEFORE ROW triggers.  We return false if one of them makes
 * the delete a no-op; otherwise, return true.
 */
bool
ExecDeletePrologue(ModifyTableContext *context, ResultRelInfo *resultRelInfo,
				   ItemPointer tupleid, HeapTuple oldtuple,
				   TupleTableSlot **epqreturnslot)
{
	/* BEGIN - merge patch refer to "Fix handling of pending inserts in nodeModifyTable.c" */
	EState *estate = context->estate;
	/* END - merge patch refer to "Fix handling of pending inserts in nodeModifyTable.c" */
	/* BEFORE ROW DELETE triggers */
	if (resultRelInfo->ri_TrigDesc &&
		resultRelInfo->ri_TrigDesc->trig_delete_before_row)
	{
		/* BEGIN - merge patch refer to "Fix handling of pending inserts in nodeModifyTable.c" */
		/* Flush any pending inserts, so rows are visible to the triggers */
		if (estate->es_insert_pending_result_relations != NIL)
			ExecPendingInserts(estate);
		/* END - merge patch refer to "Fix handling of pending inserts in nodeModifyTable.c" */
		return ExecBRDeleteTriggers(context->estate, context->epqstate,
									resultRelInfo, tupleid, oldtuple,
									epqreturnslot);
	}
	return true;
}

/*
 * ExecDeleteAct -- subroutine for ExecDelete
 *
 * Actually delete the tuple from a plain table.
 *
 * Caller is in charge of doing EvalPlanQual as necessary
 */
TM_Result
ExecDeleteAct(ModifyTableContext *context, ResultRelInfo *resultRelInfo,
			  ItemPointer tupleid, bool changingPart)
{
	EState	   *estate = context->estate;

	return table_tuple_delete(resultRelInfo->ri_RelationDesc, tupleid,
							  estate->es_output_cid,
							  estate->es_snapshot,
							  estate->es_crosscheck_snapshot,
							  true /* wait for commit */ ,
							  &context->tmfd,
							  changingPart);
}

/*
 * ExecDeleteEpilogue -- subroutine for ExecDelete
 *
 * Closing steps of tuple deletion; this invokes AFTER FOR EACH ROW triggers,
 * including the UPDATE triggers if the deletion is being done as part of a
 * cross-partition tuple move.
 */
void
ExecDeleteEpilogue(ModifyTableContext *context, ResultRelInfo *resultRelInfo,
				   ItemPointer tupleid, HeapTuple oldtuple, bool changingPart)
{
	ModifyTableState *mtstate = context->mtstate;
	EState	   *estate = context->estate;
	TransitionCaptureState *ar_delete_trig_tcs;

	/*
	 * If this delete is the result of a partition key update that moved the
	 * tuple to a new partition, put this row into the transition OLD TABLE,
	 * if there is one. We need to do this separately for DELETE and INSERT
	 * because they happen on different tables.
	 */
	ar_delete_trig_tcs = mtstate->mt_transition_capture;
	if (mtstate->operation == CMD_UPDATE && mtstate->mt_transition_capture &&
		mtstate->mt_transition_capture->tcs_update_old_table)
	{
		ExecARUpdateTriggers(estate, resultRelInfo,
							 NULL, NULL,
							 tupleid, oldtuple,
							 NULL, NULL, mtstate->mt_transition_capture,
							 false);

		/*
		 * We've already captured the OLD TABLE row, so make sure any AR
		 * DELETE trigger fired below doesn't capture it again.
		 */
		ar_delete_trig_tcs = NULL;
	}

	/* AFTER ROW DELETE Triggers */
	ExecARDeleteTriggers(estate, resultRelInfo, tupleid, oldtuple,
						 ar_delete_trig_tcs, changingPart);
}

/* ----------------------------------------------------------------
 *		ExecDelete
 *
 *		DELETE is like UPDATE, except that we delete the tuple and no
 *		index modifications are needed.
 *
 *		When deleting from a table, tupleid identifies the tuple to
 *		delete and oldtuple is NULL.  When deleting through a view
 *		INSTEAD OF trigger, oldtuple is passed to the triggers and identifies
 *		what to delete, and tupleid is invalid.  When deleting from a
 *		foreign table, tupleid is invalid; the FDW has to figure out
 *		which row to delete using data from the planSlot.  oldtuple is
 *		passed to foreign table triggers; it is NULL when the foreign
 *		table has no relevant triggers.  We use tupleDeleted to indicate
 *		whether the tuple is actually deleted, callers can use it to
 *		decide whether to continue the operation.  When this DELETE is a
 *		part of an UPDATE of partition-key, then the slot returned by
 *		EvalPlanQual() is passed back using output parameter epqslot.
 *
 *		Returns RETURNING result if any, otherwise NULL.
 * ----------------------------------------------------------------
 */
static TupleTableSlot *
ExecDelete(ModifyTableContext *context,
			ResultRelInfo *resultRelInfo,
			ItemPointer tupleid,
			HeapTuple oldtuple,
			bool processReturning,
			bool changingPart,
			bool canSetTag,
			bool *tupleDeleted,
			TupleTableSlot **epqreturnslot)

{
	EState	   *estate = context->estate;
	Relation	resultRelationDesc = resultRelInfo->ri_RelationDesc;
	TM_Result	result;
	TupleTableSlot *slot = NULL;

	if (tupleDeleted)
		*tupleDeleted = false;

	/*
	 * Prepare for the delete.  This includes BEFORE ROW triggers, so we're
	 * done if it says we are.
	 */
	if (!ExecDeletePrologue(context, resultRelInfo, tupleid, oldtuple,
							epqreturnslot))
		return NULL;


	/* INSTEAD OF ROW DELETE Triggers */
	if (resultRelInfo->ri_TrigDesc &&
		resultRelInfo->ri_TrigDesc->trig_delete_instead_row)
	{
		bool		dodelete;

		Assert(oldtuple != NULL);
		dodelete = ExecIRDeleteTriggers(estate, resultRelInfo, oldtuple);

		if (!dodelete)			/* "do nothing" */
			return NULL;
	}
	else if (resultRelInfo->ri_FdwRoutine)
	{
		/*
		 * delete from foreign table: let the FDW do it
		 *
		 * We offer the returning slot as a place to store RETURNING data,
		 * although the FDW can return some other slot if it wants.
		 */
		slot = ExecGetReturningSlot(estate, resultRelInfo);
		slot = resultRelInfo->ri_FdwRoutine->ExecForeignDelete(estate,
															   resultRelInfo,
															   slot,
															   context->planSlot);

		if (slot == NULL)		/* "do nothing" */
			return NULL;

		/*
		 * RETURNING expressions might reference the tableoid column, so
		 * (re)initialize tts_tableOid before evaluating them.
		 */
		if (TTS_EMPTY(slot))
			ExecStoreAllNullTuple(slot);

		slot->tts_tableOid = RelationGetRelid(resultRelationDesc);
	}
	else
	{
		/*
		 * delete the tuple
		 *
		 * Note: if context->estate->es_crosscheck_snapshot isn't
		 * InvalidSnapshot, we check that the row to be deleted is visible to
		 * that snapshot, and throw a can't-serialize error if not. This is a
		 * special-case behavior needed for referential integrity updates in
		 * transaction-snapshot mode transactions.
		 */
ldelete:;
		result = ExecDeleteAct(context, resultRelInfo, tupleid, changingPart);

		switch (result)
		{
			case TM_SelfModified:

				/*
				 * The target tuple was already updated or deleted by the
				 * current command, or by a later command in the current
				 * transaction.  The former case is possible in a join DELETE
				 * where multiple tuples join to the same target tuple. This
				 * is somewhat questionable, but Postgres has always allowed
				 * it: we just ignore additional deletion attempts.
				 *
				 * The latter case arises if the tuple is modified by a
				 * command in a BEFORE trigger, or perhaps by a command in a
				 * volatile function used in the query.  In such situations we
				 * should not ignore the deletion, but it is equally unsafe to
				 * proceed.  We don't want to discard the original DELETE
				 * while keeping the triggered actions based on its deletion;
				 * and it would be no better to allow the original DELETE
				 * while discarding updates that it triggered.  The row update
				 * carries some information that might be important according
				 * to business rules; so throwing an error is the only safe
				 * course.
				 *
				 * If a trigger actually intends this type of interaction, it
				 * can re-execute the DELETE and then return NULL to cancel
				 * the outer delete.
				 */
				if (context->tmfd.cmax != estate->es_output_cid)
					ereport(ERROR,
							(errcode(ERRCODE_TRIGGERED_DATA_CHANGE_VIOLATION),
							 errmsg("tuple to be deleted was already modified by an operation triggered by the current command"),
							 errhint("Consider using an AFTER trigger instead of a BEFORE trigger to propagate changes to other rows.")));

				/* Else, already deleted by self; nothing to do */
				return NULL;

			case TM_Ok:
				break;

			case TM_Updated:
				{
					TupleTableSlot *inputslot;
					TupleTableSlot *epqslot;

					if (IsolationUsesXactSnapshot())
						ereport(ERROR,
								(errcode(ERRCODE_T_R_SERIALIZATION_FAILURE),
								 errmsg("could not serialize access due to concurrent update")));

					/*
					 * Already know that we're going to need to do EPQ, so
					 * fetch tuple directly into the right slot.
					 */
					EvalPlanQualBegin(context->epqstate);
					inputslot = EvalPlanQualSlot(context->epqstate, resultRelationDesc,
												 resultRelInfo->ri_RangeTableIndex);
					result = table_tuple_lock(resultRelationDesc, tupleid,
											  estate->es_snapshot,
											  inputslot, estate->es_output_cid,
											  LockTupleExclusive, LockWaitBlock,
											  0,/* BUG#M0000470:FOR UPDATE WAIT integer */
											  TUPLE_LOCK_FLAG_FIND_LAST_VERSION,
											  &context->tmfd);

					switch (result)
					{
						case TM_Ok:
							Assert(context->tmfd.traversed);
							epqslot = EvalPlanQual(context->epqstate,
												   resultRelationDesc,
												   resultRelInfo->ri_RangeTableIndex,
												   inputslot);
							if (TupIsNull(epqslot))
								/* Tuple not passing quals anymore, exiting... */
								return NULL;

							/*
							 * If requested, skip delete and pass back the
							 * updated row.
							 */
							if (epqreturnslot)
							{
								*epqreturnslot = epqslot;
								return NULL;
							}
							else
								goto ldelete;

						case TM_SelfModified:

							/*
							 * This can be reached when following an update
							 * chain from a tuple updated by another session,
							 * reaching a tuple that was already updated in
							 * this transaction. If previously updated by this
							 * command, ignore the delete, otherwise error
							 * out.
							 *
							 * See also TM_SelfModified response to
							 * table_tuple_delete() above.
							 */
							if (context->tmfd.cmax != estate->es_output_cid)
								ereport(ERROR,
										(errcode(ERRCODE_TRIGGERED_DATA_CHANGE_VIOLATION),
										 errmsg("tuple to be deleted was already modified by an operation triggered by the current command"),
										 errhint("Consider using an AFTER trigger instead of a BEFORE trigger to propagate changes to other rows.")));
							return NULL;

						case TM_Deleted:
							/* tuple already deleted; nothing to do */
							return NULL;

						default:

							/*
							 * TM_Invisible should be impossible because we're
							 * waiting for updated row versions, and would
							 * already have errored out if the first version
							 * is invisible.
							 *
							 * TM_Updated should be impossible, because we're
							 * locking the latest version via
							 * TUPLE_LOCK_FLAG_FIND_LAST_VERSION.
							 */
							elog(ERROR, "unexpected table_tuple_lock status: %u",
								 result);
							return NULL;
					}

					Assert(false);
					break;
				}

			case TM_Deleted:
				if (IsolationUsesXactSnapshot())
					ereport(ERROR,
							(errcode(ERRCODE_T_R_SERIALIZATION_FAILURE),
							 errmsg("could not serialize access due to concurrent delete")));

				/*
				 * tuple already deleted; nothing to do. But MERGE might want
				 * to handle it differently. We've already filled-in
				 * actionInfo with sufficient information for MERGE to look
				 * at.
				 */
				return NULL;

			default:
				elog(ERROR, "unrecognized table_tuple_delete status: %u",
					 result);
				return NULL;
		}

		/*
		 * Note: Normally one would think that we have to delete index tuples
		 * associated with the heap tuple now...
		 *
		 * ... but in POSTGRES, we have no need to do this because VACUUM will
		 * take care of it later.  We can't delete index tuples immediately
		 * anyway, since the tuple is still visible to other transactions.
		 */
	}

	if (canSetTag)
		(estate->es_processed)++;

	/* Tell caller that the delete actually happened. */
	if (tupleDeleted)
		*tupleDeleted = true;

	ExecDeleteEpilogue(context, resultRelInfo, tupleid, oldtuple, changingPart);

	/* Process RETURNING if present and if requested */
	if (processReturning && resultRelInfo->ri_projectReturning)
	{
		/*
		 * We have to put the target tuple into a slot, which means first we
		 * gotta fetch it.  We can use the trigger tuple slot.
		 */
		TupleTableSlot *rslot;

		if (resultRelInfo->ri_FdwRoutine)
		{
			/* FDW must have provided a slot containing the deleted row */
			Assert(!TupIsNull(slot));
		}
		else
		{
			slot = ExecGetReturningSlot(estate, resultRelInfo);
			if (oldtuple != NULL)
			{
				ExecForceStoreHeapTuple(oldtuple, slot, false);
			}
			else
			{
				if (!table_tuple_fetch_row_version(resultRelationDesc, tupleid,
												   SnapshotAny, slot))
					elog(ERROR, "failed to fetch deleted tuple for DELETE RETURNING");
			}
		}

		rslot = ExecProcessReturning(resultRelInfo, slot, context->planSlot);

		/*
		 * Before releasing the target tuple again, make sure rslot has a
		 * local copy of any pass-by-reference values.
		 */
		ExecMaterializeSlot(rslot);

		ExecClearTuple(slot);

		return rslot;
	}

	return NULL;
}

/*
 * ExecPendingInserts -- flushes all pending inserts to the foreign tables
 */
static void
ExecPendingInserts(EState *estate)
{
	ListCell   *l1,
			   *l2;

	forboth(l1, estate->es_insert_pending_result_relations,
			l2, estate->es_insert_pending_modifytables)
	{
		ResultRelInfo *resultRelInfo = (ResultRelInfo *) lfirst(l1);
		ModifyTableState *mtstate = (ModifyTableState *) lfirst(l2);

		Assert(mtstate);
		ExecBatchInsert(mtstate, resultRelInfo,
						resultRelInfo->ri_Slots,
						resultRelInfo->ri_PlanSlots,
						resultRelInfo->ri_NumSlots,
						estate, mtstate->canSetTag);
	}

	list_free(estate->es_insert_pending_result_relations);
	list_free(estate->es_insert_pending_modifytables);
	estate->es_insert_pending_result_relations = NIL;
	estate->es_insert_pending_modifytables = NIL;
}

/*
 * ExecCrossPartitionUpdate --- Move an updated tuple to another partition.
 *
 * This works by first deleting the old tuple from the current partition,
 * followed by inserting the new tuple into the root parent table, that is,
 * mtstate->rootResultRelInfo.  It will be re-routed from there to the
 * correct partition.
 *
 * Returns true if the tuple has been successfully moved, or if it's found
 * that the tuple was concurrently deleted so there's nothing more to do
 * for the caller.
 *
 * False is returned if the tuple we're trying to move is found to have been
 * concurrently updated.  In that case, the caller must to check if the
 * updated tuple that's returned in *retry_slot still needs to be re-routed,
 * and call this function again or perform a regular update accordingly.
 */
static bool
ExecCrossPartitionUpdate(ModifyTableContext *context,
						 ResultRelInfo *resultRelInfo,
						 ItemPointer tupleid, HeapTuple oldtuple,
						 TupleTableSlot *slot,
						 bool canSetTag,
						 UpdateContext *updateCxt,
						 TupleTableSlot **inserted_tuple,
						 ResultRelInfo **insert_destrel)
{
	ModifyTableState *mtstate = context->mtstate;
	EState	   *estate = mtstate->ps.state;
	TupleConversionMap *tupconv_map;
	bool		tuple_deleted;
	TupleTableSlot *epqslot = NULL;

	/* Begin - ReqID:SRS-MYSQL-UPDATE */
	ResultRelInfo		   *originResultRelInfo = NULL;
	PartitionTupleRouting  *origin_partition_tuple_routing = NULL;
	TupleTableSlot		   *origin_root_tuple_slot = NULL;
	bool		is_replacerootrelinfo = false;
	/* End - ReqID:SRS-MYSQL-UPDATE */

	context->cpUpdateReturningSlot = NULL;
	context->cpUpdateRetrySlot = NULL;

	/* Begin - ReqID:SRS-MYSQL-UPDATE */
	if (MYS_MODE == compatible_mode && resultRelInfo->ri_RootResultRelInfo) /* BUG#Z5121 */
	{
		if (mtstate->rootResultRelInfo != resultRelInfo->ri_RootResultRelInfo)
		{
			originResultRelInfo = mtstate->rootResultRelInfo;
			mtstate->rootResultRelInfo = resultRelInfo->ri_RootResultRelInfo;
			origin_partition_tuple_routing = mtstate->mt_partition_tuple_routing;
			origin_root_tuple_slot = mtstate->mt_root_tuple_slot;
			if (context->estate->result_rel_index > 0 &&
				mtstate->mt_partition_tuple_routing &&
				mtstate->mt_root_tuple_slot)
			{
				mtstate->mt_partition_tuple_routing = NULL;
				mtstate->mt_root_tuple_slot = NULL;
			}
			is_replacerootrelinfo = true;
		}
	}
	/* End - ReqID:SRS-MYSQL-UPDATE */
	/*
	 * Disallow an INSERT ON CONFLICT DO UPDATE that causes the original row
	 * to migrate to a different partition.  Maybe this can be implemented
	 * some day, but it seems a fringe feature with little redeeming value.
	 */
	if (((ModifyTable *) mtstate->ps.plan)->onConflictAction == ONCONFLICT_UPDATE)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("invalid ON UPDATE specification"),
				 errdetail("The result tuple would appear in a different partition than the original tuple.")));

	/*
	 * When an UPDATE is run directly on a leaf partition, simply fail with a
	 * partition constraint violation error.
	 */
	if (resultRelInfo == mtstate->rootResultRelInfo)
		ExecPartitionCheckEmitError(resultRelInfo, slot, estate);

	/* Initialize tuple routing info if not already done. */
	if (mtstate->mt_partition_tuple_routing == NULL)
	{
		Relation	rootRel = mtstate->rootResultRelInfo->ri_RelationDesc;
		MemoryContext oldcxt;

		/* Things built here have to last for the query duration. */
		oldcxt = MemoryContextSwitchTo(estate->es_query_cxt);

		mtstate->mt_partition_tuple_routing =
			ExecSetupPartitionTupleRouting(estate, rootRel);

		/*
		 * Before a partition's tuple can be re-routed, it must first be
		 * converted to the root's format, so we'll need a slot for storing
		 * such tuples.
		 */
		Assert(mtstate->mt_root_tuple_slot == NULL);
		mtstate->mt_root_tuple_slot = table_slot_create(rootRel, NULL);

		MemoryContextSwitchTo(oldcxt);
	}

	/*
	 * Row movement, part 1.  Delete the tuple, but skip RETURNING processing.
	 * We want to return rows from INSERT.
	 */
	ExecDelete(context, resultRelInfo,
			   tupleid, oldtuple,
			   false,			/* processReturning */
			   true,			/* changingPart */
			   false,			/* canSetTag */
			   &tuple_deleted, &epqslot);

	/*
	 * For some reason if DELETE didn't happen (e.g. trigger prevented it, or
	 * it was already deleted by self, or it was concurrently deleted by
	 * another transaction), then we should skip the insert as well;
	 * otherwise, an UPDATE could cause an increase in the total number of
	 * rows across all partitions, which is clearly wrong.
	 *
	 * For a normal UPDATE, the case where the tuple has been the subject of a
	 * concurrent UPDATE or DELETE would be handled by the EvalPlanQual
	 * machinery, but for an UPDATE that we've translated into a DELETE from
	 * this partition and an INSERT into some other partition, that's not
	 * available, because CTID chains can't span relation boundaries.  We
	 * mimic the semantics to a limited extent by skipping the INSERT if the
	 * DELETE fails to find a tuple.  This ensures that two concurrent
	 * attempts to UPDATE the same tuple at the same time can't turn one tuple
	 * into two, and that an UPDATE of a just-deleted tuple can't resurrect
	 * it.
	 */
	if (!tuple_deleted)
	{
		/*
		 * epqslot will be typically NULL.  But when ExecDelete() finds that
		 * another transaction has concurrently updated the same row, it
		 * re-fetches the row, skips the delete, and epqslot is set to the
		 * re-fetched tuple slot.  In that case, we need to do all the checks
		 * again.
		 */
		if (TupIsNull(epqslot))
			return true;
		else
		{
			/* Fetch the most recent version of old tuple. */
			TupleTableSlot *oldSlot;

			/* ... but first, make sure ri_oldTupleSlot is initialized. */
			if (unlikely(!resultRelInfo->ri_projectNewInfoValid))
				ExecInitUpdateProjection(mtstate, resultRelInfo);
			oldSlot = resultRelInfo->ri_oldTupleSlot;
			if (!table_tuple_fetch_row_version(resultRelInfo->ri_RelationDesc,
											   tupleid,
											   SnapshotAny,
											   oldSlot))
				elog(ERROR, "failed to fetch tuple being updated");
			/* and project the new tuple to retry the UPDATE with */
			context->cpUpdateRetrySlot =
				context->GetUpdateNewTuple(resultRelInfo, epqslot, oldSlot,
													   context->relaction);
			return false;
		}
	}

	/*
	 * resultRelInfo is one of the per-relation resultRelInfos.  So we should
	 * convert the tuple into root's tuple descriptor if needed, since
	 * ExecInsert() starts the search from root.
	 */
	tupconv_map = ExecGetChildToRootMap(resultRelInfo);
	if (tupconv_map != NULL)
		slot = execute_attr_map_slot(tupconv_map->attrMap,
									 slot,
									 mtstate->mt_root_tuple_slot);

	/* Tuple routing starts from the root table. */
	context->cpUpdateReturningSlot =
		ExecInsert(context, mtstate->rootResultRelInfo, slot, canSetTag,
				   inserted_tuple, insert_destrel);

	/*
	 * Reset the transition state that may possibly have been written by
	 * INSERT.
	 */
	if (mtstate->mt_transition_capture)
		mtstate->mt_transition_capture->tcs_original_insert_tuple = NULL;

	/* Begin - ReqID:SRS-MYSQL-UPDATE */
	if (MYS_MODE == compatible_mode && is_replacerootrelinfo) /* BUG#Z5121 */
	{
		if (!list_member_ptr(mtstate->mt_partition_tuple_routing_list, mtstate->mt_partition_tuple_routing))
			mtstate->mt_partition_tuple_routing_list = lappend(mtstate->mt_partition_tuple_routing_list,
														   mtstate->mt_partition_tuple_routing);
		if (!list_member_ptr(mtstate->mt_root_tuple_slot_list, mtstate->mt_root_tuple_slot))
			mtstate->mt_root_tuple_slot_list = lappend(mtstate->mt_root_tuple_slot_list,
													   mtstate->mt_root_tuple_slot);
		mtstate->rootResultRelInfo = originResultRelInfo;
		mtstate->mt_partition_tuple_routing = origin_partition_tuple_routing;
		mtstate->mt_root_tuple_slot = origin_root_tuple_slot;
	}
	/* End - ReqID:SRS-MYSQL-UPDATE */

	/* We're done moving. */
	return true;
}


/*
 * ExecUpdateAct -- subroutine for ExecUpdate
 *
 * Actually update the tuple, when operating on a plain table.  If the
 * table is a partition, and the command was called referencing an ancestor
 * partitioned table, this routine migrates the resulting tuple to another
 * partition.
 *
 * The caller is in charge of keeping indexes current as necessary.  The
 * caller is also in charge of doing EvalPlanQual if the tuple is found to
 * be concurrently updated.  However, in case of a cross-partition update,
 * this routine does it.
 *
 * Caller is in charge of doing EvalPlanQual as necessary, and of keeping
 * indexes current for the update.
 */
TM_Result
ExecUpdateAct(ModifyTableContext *context, ResultRelInfo *resultRelInfo,
			  ItemPointer tupleid, HeapTuple oldtuple, TupleTableSlot *slot,
			  bool canSetTag, UpdateContext *updateCxt)
{
	EState	   *estate = context->estate;
	Relation	resultRelationDesc = resultRelInfo->ri_RelationDesc;
	bool		partition_constraint_failed;
	TM_Result	result;

	updateCxt->crossPartUpdate = false;

	/*
	 * If we move the tuple to a new partition, we loop back here to recompute
	 * GENERATED values (which are allowed to be different across partitions)
	 * and recheck any RLS policies and constraints.  We do not fire any
	 * BEFORE triggers of the new partition, however.
	 */
lreplace:
	/* Fill in GENERATEd columns */
	ExecUpdatePrepareSlot(resultRelInfo, slot, estate);

	/* ensure slot is independent, consider e.g. EPQ */
	ExecMaterializeSlot(slot);

	/*
	 * If partition constraint fails, this row might get moved to another
	 * partition, in which case we should check the RLS CHECK policy just
	 * before inserting into the new partition, rather than doing it here.
	 * This is because a trigger on that partition might again change the row.
	 * So skip the WCO checks if the partition constraint fails.
	 */
	partition_constraint_failed =
		resultRelationDesc->rd_rel->relispartition &&
		!ExecPartitionCheck(resultRelInfo, slot, estate, false);

	/* Check any RLS UPDATE WITH CHECK policies */
	if (!partition_constraint_failed &&
		resultRelInfo->ri_WithCheckOptions != NIL)
	{
		/*
		 * ExecWithCheckOptions() will skip any WCOs which are not of the kind
		 * we are looking for at this point.
		 */
		ExecWithCheckOptions(WCO_RLS_UPDATE_CHECK,
							 resultRelInfo, slot, estate);
	}

	/*
	 * If a partition check failed, try to move the row into the right
	 * partition.
	 */
	if (partition_constraint_failed)
	{
		TupleTableSlot *inserted_tuple;
		ResultRelInfo *insert_destrel = NULL;

		/*
		 * ExecCrossPartitionUpdate will first DELETE the row from the
		 * partition it's currently in and then insert it back into the root
		 * table, which will re-route it to the correct partition.  However,
		 * if the tuple has been concurrently updated, a retry is needed.
		 */
		if (ExecCrossPartitionUpdate(context, resultRelInfo,
									 tupleid, oldtuple, slot,
									 canSetTag, updateCxt,
									 &inserted_tuple,
									 &insert_destrel))
		{
			/* success! */
			updateCxt->updated = true;
			updateCxt->crossPartUpdate = true;

			/*
			 * If the partitioned table being updated is referenced in foreign
			 * keys, queue up trigger events to check that none of them were
			 * violated.  No special treatment is needed in
			 * non-cross-partition update situations, because the leaf
			 * partition's AR update triggers will take care of that.  During
			 * cross-partition updates implemented as delete on the source
			 * partition followed by insert on the destination partition,
			 * AR-UPDATE triggers of the root table (that is, the table
			 * mentioned in the query) must be fired.
			 *
			 * NULL insert_destrel means that the move failed to occur, that
			 * is, the update failed, so no need to anything in that case.
			 */
			if (insert_destrel &&
				resultRelInfo->ri_TrigDesc &&
				resultRelInfo->ri_TrigDesc->trig_update_after_row)
				ExecCrossPartitionUpdateForeignKey(context,
												   resultRelInfo,
												   insert_destrel,
												   tupleid, slot,
												   inserted_tuple);

			return TM_Ok;
		}

		/*
		 * No luck, a retry is needed.  If running MERGE, we do not do so
		 * here; instead let it handle that on its own rules.
		 */
		if (context->relaction != NULL)
			return TM_Updated;

		/*
		 * ExecCrossPartitionUpdate installed an updated version of the new
		 * tuple in the retry slot; start over.
		 */
		slot = context->cpUpdateRetrySlot;
		goto lreplace;
	}

	/*
	 * Check the constraints of the tuple.  We've already checked the
	 * partition constraint above; however, we must still ensure the tuple
	 * passes all other constraints, so we will call ExecConstraints() and
	 * have it validate all remaining checks.
	 */
	if (resultRelationDesc->rd_att->constr)
		ExecConstraints(resultRelInfo, slot, estate);

	/*
	 * replace the heap tuple
	 *
	 * Note: if es_crosscheck_snapshot isn't InvalidSnapshot, we check that
	 * the row to be updated is visible to that snapshot, and throw a
	 * can't-serialize error if not. This is a special-case behavior needed
	 * for referential integrity updates in transaction-snapshot mode
	 * transactions.
	 */
	result = table_tuple_update(resultRelationDesc, tupleid, slot,
								estate->es_output_cid,
								estate->es_snapshot,
								estate->es_crosscheck_snapshot,
								true /* wait for commit */ ,
								&context->tmfd, &context->lockmode,
								&updateCxt->updateIndexes);
	if (result == TM_Ok)
		updateCxt->updated = true;

	return result;
}

/*
 * ExecUpdateEpilogue -- subroutine for ExecUpdate
 *
 * Closing steps of updating a tuple.  Must be called if ExecUpdateAct
 * returns indicating that the tuple was updated.
 */
void
ExecUpdateEpilogue(ModifyTableContext *context, UpdateContext *updateCxt,
				   ResultRelInfo *resultRelInfo, ItemPointer tupleid,
				   HeapTuple oldtuple, TupleTableSlot *slot,
				   List *recheckIndexes)
{
	ModifyTableState *mtstate = context->mtstate;

	/* insert index entries for tuple if necessary */
	if (resultRelInfo->ri_NumIndices > 0 && updateCxt->updateIndexes)
		recheckIndexes = ExecInsertIndexTuples(resultRelInfo,
											   slot, context->estate,
											   true, false,
											   NULL, NIL);

	/* AFTER ROW UPDATE Triggers */
	ExecARUpdateTriggers(context->estate, resultRelInfo,
						 NULL, NULL,
						 tupleid, oldtuple, slot,
						 recheckIndexes,
						 mtstate->operation == CMD_INSERT ?
						 mtstate->mt_oc_transition_capture :
						 mtstate->mt_transition_capture,
						 false);

	/*
	 * Check any WITH CHECK OPTION constraints from parent views.  We are
	 * required to do this after testing all constraints and uniqueness
	 * violations per the SQL spec, so we do it after actually updating the
	 * record in the heap and all indexes.
	 *
	 * ExecWithCheckOptions() will skip any WCOs which are not of the kind we
	 * are looking for at this point.
	 */
	if (resultRelInfo->ri_WithCheckOptions != NIL)
		ExecWithCheckOptions(WCO_VIEW_CHECK, resultRelInfo,
							 slot, context->estate);
}

/*
 * ExecUpdatePrologue -- subroutine for ExecUpdate
 *
 * Prepare executor state for UPDATE.  This includes running BEFORE ROW
 * triggers.  We return false if one of them makes the update a no-op;
 * otherwise, return true.
 */
bool
ExecUpdatePrologue(ModifyTableContext *context, ResultRelInfo *resultRelInfo,
				   ItemPointer tupleid, HeapTuple oldtuple, TupleTableSlot *slot)
{
	Relation	resultRelationDesc = resultRelInfo->ri_RelationDesc;

	ExecMaterializeSlot(slot);

	/*
	 * Open the table's indexes, if we have not done so already, so that we
	 * can add new index entries for the updated tuple.
	 */
	if (resultRelationDesc->rd_rel->relhasindex &&
		resultRelInfo->ri_IndexRelationDescs == NULL)
		ExecOpenIndices(resultRelInfo, false);

	/* BEFORE ROW UPDATE triggers */
	if (resultRelInfo->ri_TrigDesc &&
		resultRelInfo->ri_TrigDesc->trig_update_before_row)
	{
		/* BEGIN - merge patch refer to "Fix handling of pending inserts in nodeModifyTable.c" */
		EState *estate = context->estate;
		
		/* Flush any pending inserts, so rows are visible to the triggers */
		if (estate->es_insert_pending_result_relations != NIL)
			ExecPendingInserts(estate);
		/* END - merge patch refer to "Fix handling of pending inserts in nodeModifyTable.c" */
		return ExecBRUpdateTriggers(context->estate, context->epqstate,
									resultRelInfo, tupleid, oldtuple, slot,
									&context->tmfd);
	}
	return true;
}

/*
 * ExecUpdatePrepareSlot -- subroutine for ExecUpdateAct
 *
 * Apply the final modifications to the tuple slot before the update.
 * (This is split out because we also need it in the foreign-table code path.)
 */
void
ExecUpdatePrepareSlot(ResultRelInfo *resultRelInfo,
					  TupleTableSlot *slot,
					  EState *estate)
{
	Relation	resultRelationDesc = resultRelInfo->ri_RelationDesc;

	/*
	 * Constraints and GENERATED expressions might reference the tableoid
	 * column, so (re-)initialize tts_tableOid before evaluating them.
	 */
	slot->tts_tableOid = RelationGetRelid(resultRelationDesc);

	/*
	 * Compute stored generated columns
	 */
	if (resultRelationDesc->rd_att->constr &&
		resultRelationDesc->rd_att->constr->has_generated_stored)
		ExecComputeStoredGenerated(resultRelInfo, estate, slot,
								   CMD_UPDATE);
}

/*
 * Queues up an update event using the target root partitioned table's
 * trigger to check that a cross-partition update hasn't broken any foreign
 * keys pointing into it.
 */
static void
ExecCrossPartitionUpdateForeignKey(ModifyTableContext *context,
								   ResultRelInfo *sourcePartInfo,
								   ResultRelInfo *destPartInfo,
								   ItemPointer tupleid,
								   TupleTableSlot *oldslot,
								   TupleTableSlot *newslot)
{
	ListCell   *lc;
	ResultRelInfo *rootRelInfo;
	List	   *ancestorRels;

	rootRelInfo = sourcePartInfo->ri_RootResultRelInfo;
	ancestorRels = ExecGetAncestorResultRels(context->estate, sourcePartInfo);

	/*
	 * For any foreign keys that point directly into a non-root ancestors of
	 * the source partition, we can in theory fire an update event to enforce
	 * those constraints using their triggers, if we could tell that both the
	 * source and the destination partitions are under the same ancestor. But
	 * for now, we simply report an error that those cannot be enforced.
	 */
	foreach(lc, ancestorRels)
	{
		ResultRelInfo *rInfo = lfirst(lc);
		TriggerDesc *trigdesc = rInfo->ri_TrigDesc;
		bool		has_noncloned_fkey = false;

		/* Root ancestor's triggers will be processed. */
		if (rInfo == rootRelInfo)
			continue;

		if (trigdesc && trigdesc->trig_update_after_row)
		{
			for (int i = 0; i < trigdesc->numtriggers; i++)
			{
				Trigger    *trig = &trigdesc->triggers[i];

				if (!trig->tgisclone &&
					RI_FKey_trigger_type(trig->tgfoid) == RI_TRIGGER_PK)
				{
					has_noncloned_fkey = true;
					break;
				}
			}
		}

		if (has_noncloned_fkey)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("cannot move tuple across partitions when a non-root ancestor of the source partition is directly referenced in a foreign key"),
					 errdetail("A foreign key points to ancestor \"%s\", but not the root ancestor \"%s\".",
							   RelationGetRelationName(rInfo->ri_RelationDesc),
							   RelationGetRelationName(rootRelInfo->ri_RelationDesc)),
					 errhint("Consider defining the foreign key on \"%s\".",
							 RelationGetRelationName(rootRelInfo->ri_RelationDesc))));
	}

	/* Perform the root table's triggers. */
	ExecARUpdateTriggers(context->estate,
						 rootRelInfo, sourcePartInfo, destPartInfo,
						 tupleid, NULL, newslot, NIL, NULL, true);
}


/* ----------------------------------------------------------------
 *		ExecUpdate
 *
 *		note: we can't run UPDATE queries with transactions
 *		off because UPDATEs are actually INSERTs and our
 *		scan will mistakenly loop forever, updating the tuple
 *		it just inserted..  This should be fixed but until it
 *		is, we don't want to get stuck in an infinite loop
 *		which corrupts your database..
 *
 *		When updating a table, tupleid identifies the tuple to
 *		update and oldtuple is NULL.  When updating through a view INSTEAD OF
 *		trigger, oldtuple is passed to the triggers and identifies what to
 *		update, and tupleid is invalid.  When updating a foreign table,
 *		tupleid is invalid; the FDW has to figure out which row to
 *		update using data from the planSlot.  oldtuple is passed to
 *		foreign table triggers; it is NULL when the foreign table has
 *		no relevant triggers.
 *
 *		slot contains the new tuple value to be stored.
 *		planSlot is the output of the ModifyTable's subplan; we use it
 *		to access values from other input tables (for RETURNING),
 *		row-ID junk columns, etc.
 *
 *		Returns RETURNING result if any, otherwise NULL.  On exit, if tupleid
 *		had identified the tuple to update, it will identify the tuple
 *		actually updated after EvalPlanQual.
 * ----------------------------------------------------------------
 */
static TupleTableSlot *
ExecUpdate(ModifyTableContext *context, ResultRelInfo *resultRelInfo,
		   ItemPointer tupleid, HeapTuple oldtuple, TupleTableSlot *slot,
		   bool canSetTag)
{
	EState	   *estate = context->estate;
	Relation	resultRelationDesc = resultRelInfo->ri_RelationDesc;
	UpdateContext updateCxt = {0};
	TM_Result	result;
	List	   *recheckIndexes = NIL;

	/*
	 * abort the operation if not running transactions
	 */
	if (IsBootstrapProcessingMode())
		elog(ERROR, "cannot UPDATE during bootstrap");

	/*
	 * Prepare for the update.  This includes BEFORE ROW triggers, so we're
	 * done if it says we are.
	 */
	if (!ExecUpdatePrologue(context, resultRelInfo, tupleid, oldtuple, slot))
		return NULL;

	/* INSTEAD OF ROW UPDATE Triggers */
	if (resultRelInfo->ri_TrigDesc &&
		resultRelInfo->ri_TrigDesc->trig_update_instead_row)
	{
		if (!ExecIRUpdateTriggers(estate, resultRelInfo,
								  oldtuple, slot))
			return NULL;		/* "do nothing" */
	}
	else if (resultRelInfo->ri_FdwRoutine)
	{
		/* Fill in GENERATEd columns */
		ExecUpdatePrepareSlot(resultRelInfo, slot, estate);

		/*
		 * update in foreign table: let the FDW do it
		 */
		slot = resultRelInfo->ri_FdwRoutine->ExecForeignUpdate(estate,
															   resultRelInfo,
															   slot,
															   context->planSlot);

		if (slot == NULL)		/* "do nothing" */
			return NULL;

		/*
		 * AFTER ROW Triggers or RETURNING expressions might reference the
		 * tableoid column, so (re-)initialize tts_tableOid before evaluating
		 * them.  (This covers the case where the FDW replaced the slot.)
		 */
		slot->tts_tableOid = RelationGetRelid(resultRelationDesc);
	}
	else
	{
		ItemPointerData lockedtid;
		/* Fill in the slot appropriately */

		/*
		 * If we generate a new candidate tuple after EvalPlanQual testing, we
		 * must loop back here to try again.  (We don't need to redo triggers,
		 * however.  If there are any BEFORE triggers then trigger.c will have
		 * done table_tuple_lock to lock the correct tuple, so there's no need
		 * to do them again.)
		 */
redo_act:
		lockedtid = *tupleid;
		result = ExecUpdateAct(context, resultRelInfo, tupleid, oldtuple, slot,
							   canSetTag, &updateCxt);
		/*
		 * If ExecUpdateAct reports that a cross-partition update was done,
		 * then the RETURNING tuple (if any) has been projected and there's
		 * nothing else for us to do.
		 */
		if (updateCxt.crossPartUpdate)
			return context->cpUpdateReturningSlot;

		switch (result)
		{
			case TM_SelfModified:

				/*
				 * The target tuple was already updated or deleted by the
				 * current command, or by a later command in the current
				 * transaction.  The former case is possible in a join UPDATE
				 * where multiple tuples join to the same target tuple. This
				 * is pretty questionable, but Postgres has always allowed it:
				 * we just execute the first update action and ignore
				 * additional update attempts.
				 *
				 * The latter case arises if the tuple is modified by a
				 * command in a BEFORE trigger, or perhaps by a command in a
				 * volatile function used in the query.  In such situations we
				 * should not ignore the update, but it is equally unsafe to
				 * proceed.  We don't want to discard the original UPDATE
				 * while keeping the triggered actions based on it; and we
				 * have no principled way to merge this update with the
				 * previous ones.  So throwing an error is the only safe
				 * course.
				 *
				 * If a trigger actually intends this type of interaction, it
				 * can re-execute the UPDATE (assuming it can figure out how)
				 * and then return NULL to cancel the outer update.
				 */
				if (context->tmfd.cmax != estate->es_output_cid)
					ereport(ERROR,
							(errcode(ERRCODE_TRIGGERED_DATA_CHANGE_VIOLATION),
							 errmsg("tuple to be updated was already modified by an operation triggered by the current command"),
							 errhint("Consider using an AFTER trigger instead of a BEFORE trigger to propagate changes to other rows.")));

				/* Else, already updated by self; nothing to do */
				return NULL;

			case TM_Ok:
				break;

			case TM_Updated:
				{
					TupleTableSlot *inputslot;
					TupleTableSlot *epqslot;
					TupleTableSlot *oldSlot;

					if (IsolationUsesXactSnapshot())
						ereport(ERROR,
								(errcode(ERRCODE_T_R_SERIALIZATION_FAILURE),
								 errmsg("could not serialize access due to concurrent update")));

					/*
					 * Already know that we're going to need to do EPQ, so
					 * fetch tuple directly into the right slot.
					 */
					inputslot = EvalPlanQualSlot(context->epqstate, resultRelationDesc,
												 resultRelInfo->ri_RangeTableIndex);

					result = table_tuple_lock(resultRelationDesc, tupleid,
											  estate->es_snapshot,
											  inputslot, estate->es_output_cid,
											  context->lockmode, LockWaitBlock,
											  0,/* BUG#M0000470:FOR UPDATE WAIT integer */
											  TUPLE_LOCK_FLAG_FIND_LAST_VERSION,
											  &context->tmfd);

					switch (result)
					{
						case TM_Ok:
							Assert(context->tmfd.traversed);

							epqslot = EvalPlanQual(context->epqstate,
												   resultRelationDesc,
												   resultRelInfo->ri_RangeTableIndex,
												   inputslot);
							if (TupIsNull(epqslot))
								/* Tuple not passing quals anymore, exiting... */
								return NULL;

							/* Make sure ri_oldTupleSlot is initialized. */
							if (unlikely(!resultRelInfo->ri_projectNewInfoValid))
								ExecInitUpdateProjection(context->mtstate,
														 resultRelInfo);

							if (resultRelInfo->ri_needLockTagTuple)
							{
								UnlockTuple(resultRelationDesc,
											&lockedtid, InplaceUpdateTupleLock);
								LockTuple(resultRelationDesc,
										  tupleid, InplaceUpdateTupleLock);
							}

							/* Fetch the most recent version of old tuple. */
							oldSlot = resultRelInfo->ri_oldTupleSlot;
							if (!table_tuple_fetch_row_version(resultRelationDesc,
															   tupleid,
															   SnapshotAny,
															   oldSlot))
								elog(ERROR, "failed to fetch tuple being updated");
							slot = ExecGetUpdateNewTuple(resultRelInfo,
														 epqslot, oldSlot);
							goto redo_act;

						case TM_Deleted:
							/* tuple already deleted; nothing to do */
							return NULL;

						case TM_SelfModified:

							/*
							 * This can be reached when following an update
							 * chain from a tuple updated by another session,
							 * reaching a tuple that was already updated in
							 * this transaction. If previously modified by
							 * this command, ignore the redundant update,
							 * otherwise error out.
							 *
							 * See also TM_SelfModified response to
							 * table_tuple_update() above.
							 */
							if (context->tmfd.cmax != estate->es_output_cid)
								ereport(ERROR,
										(errcode(ERRCODE_TRIGGERED_DATA_CHANGE_VIOLATION),
										 errmsg("tuple to be updated was already modified by an operation triggered by the current command"),
										 errhint("Consider using an AFTER trigger instead of a BEFORE trigger to propagate changes to other rows.")));
							return NULL;

						default:
							/* see table_tuple_lock call in ExecDelete() */
							elog(ERROR, "unexpected table_tuple_lock status: %u",
								 result);
							return NULL;
					}
				}

				break;

			case TM_Deleted:
				if (IsolationUsesXactSnapshot())
					ereport(ERROR,
							(errcode(ERRCODE_T_R_SERIALIZATION_FAILURE),
							 errmsg("could not serialize access due to concurrent delete")));
				/* tuple already deleted; nothing to do */
				return NULL;

			default:
				elog(ERROR, "unrecognized table_tuple_update status: %u",
					 result);
				return NULL;
		}

	}

	if (canSetTag)
		(estate->es_processed)++;

	ExecUpdateEpilogue(context, &updateCxt, resultRelInfo, tupleid, oldtuple,
					   slot, recheckIndexes);

	list_free(recheckIndexes);

	/* Process RETURNING if present */
	if (resultRelInfo->ri_projectReturning)
		return ExecProcessReturning(resultRelInfo, slot, context->planSlot);

	return NULL;
}

/*
 * ExecOnConflictUpdate --- execute UPDATE of INSERT ON CONFLICT DO UPDATE
 *
 * Try to lock tuple for update as part of speculative insertion.  If
 * a qual originating from ON CONFLICT DO UPDATE is satisfied, update
 * (but still lock row, even though it may not satisfy estate's
 * snapshot).
 *
 * Returns true if we're done (with or without an update), or false if
 * the caller must retry the INSERT from scratch.
 */
static bool
ExecOnConflictUpdate(ModifyTableContext *context,
					 ResultRelInfo *resultRelInfo,
					 ItemPointer conflictTid,
					 TupleTableSlot *excludedSlot,
					 bool canSetTag,
					 TupleTableSlot **returning)
{
	ModifyTableState *mtstate = context->mtstate;
	ExprContext *econtext = mtstate->ps.ps_ExprContext;
	Relation	relation = resultRelInfo->ri_RelationDesc;
	ExprState  *onConflictSetWhere = resultRelInfo->ri_onConflict->oc_WhereClause;
	TupleTableSlot *existing = resultRelInfo->ri_onConflict->oc_Existing;
	TM_FailureData tmfd;
	LockTupleMode lockmode;
	TM_Result	test;
	Datum		xminDatum;
	TransactionId xmin;
	bool		isnull;

	/*
	 * Parse analysis should have blocked ON CONFLICT for all system
	 * relations, which includes these.  There's no fundamental obstacle to
	 * supporting this; we'd just need to handle LOCKTAG_TUPLE like the other
	 * ExecUpdate() caller.
	 */
	Assert(!resultRelInfo->ri_needLockTagTuple);

	/* Determine lock mode to use */
	lockmode = ExecUpdateLockMode(context->estate, resultRelInfo);

	/*
	 * Lock tuple for update.  Don't follow updates when tuple cannot be
	 * locked without doing so.  A row locking conflict here means our
	 * previous conclusion that the tuple is conclusively committed is not
	 * true anymore.
	 */
	test = table_tuple_lock(relation, conflictTid,
							context->estate->es_snapshot,
							existing, context->estate->es_output_cid,
							lockmode, LockWaitBlock,
							0,/* BUG#M0000470:FOR UPDATE WAIT integer */
							0,
							&tmfd);
	switch (test)
	{
		case TM_Ok:
			/* success! */
			break;

		case TM_Invisible:

			/*
			 * This can occur when a just inserted tuple is updated again in
			 * the same command. E.g. because multiple rows with the same
			 * conflicting key values are inserted.
			 *
			 * This is somewhat similar to the ExecUpdate() TM_SelfModified
			 * case.  We do not want to proceed because it would lead to the
			 * same row being updated a second time in some unspecified order,
			 * and in contrast to plain UPDATEs there's no historical behavior
			 * to break.
			 *
			 * It is the user's responsibility to prevent this situation from
			 * occurring.  These problems are why the SQL standard similarly
			 * specifies that for SQL MERGE, an exception must be raised in
			 * the event of an attempt to update the same row twice.
			 */
			xminDatum = slot_getsysattr(existing,
										MinTransactionIdAttributeNumber,
										&isnull);
			Assert(!isnull);
			xmin = DatumGetTransactionId(xminDatum);

			if (TransactionIdIsCurrentTransactionId(xmin))
				ereport(ERROR,
						(errcode(ERRCODE_CARDINALITY_VIOLATION),
				/* translator: %s is a SQL command name */
						 errmsg("%s command cannot affect row a second time",
								"ON CONFLICT DO UPDATE"),
						 errhint("Ensure that no rows proposed for insertion within the same command have duplicate constrained values.")));

			/* This shouldn't happen */
			elog(ERROR, "attempted to lock invisible tuple");
			break;

		case TM_SelfModified:

			/*
			 * This state should never be reached. As a dirty snapshot is used
			 * to find conflicting tuples, speculative insertion wouldn't have
			 * seen this row to conflict with.
			 */
			elog(ERROR, "unexpected self-updated tuple");
			break;

		case TM_Updated:
			if (IsolationUsesXactSnapshot())
				ereport(ERROR,
						(errcode(ERRCODE_T_R_SERIALIZATION_FAILURE),
						 errmsg("could not serialize access due to concurrent update")));

			/*
			 * As long as we don't support an UPDATE of INSERT ON CONFLICT for
			 * a partitioned table we shouldn't reach to a case where tuple to
			 * be lock is moved to another partition due to concurrent update
			 * of the partition key.
			 */
			Assert(!ItemPointerIndicatesMovedPartitions(&tmfd.ctid));

			/*
			 * Tell caller to try again from the very start.
			 *
			 * It does not make sense to use the usual EvalPlanQual() style
			 * loop here, as the new version of the row might not conflict
			 * anymore, or the conflicting tuple has actually been deleted.
			 */
			ExecClearTuple(existing);
			return false;

		case TM_Deleted:
			if (IsolationUsesXactSnapshot())
				ereport(ERROR,
						(errcode(ERRCODE_T_R_SERIALIZATION_FAILURE),
						 errmsg("could not serialize access due to concurrent delete")));

			/* see TM_Updated case */
			Assert(!ItemPointerIndicatesMovedPartitions(&tmfd.ctid));
			ExecClearTuple(existing);
			return false;

		default:
			elog(ERROR, "unrecognized table_tuple_lock status: %u", test);
	}

	/* Success, the tuple is locked. */

	/*
	 * Verify that the tuple is visible to our MVCC snapshot if the current
	 * isolation level mandates that.
	 *
	 * It's not sufficient to rely on the check within ExecUpdate() as e.g.
	 * CONFLICT ... WHERE clause may prevent us from reaching that.
	 *
	 * This means we only ever continue when a new command in the current
	 * transaction could see the row, even though in READ COMMITTED mode the
	 * tuple will not be visible according to the current statement's
	 * snapshot.  This is in line with the way UPDATE deals with newer tuple
	 * versions.
	 */
	ExecCheckTupleVisible(context->estate, relation, existing);

	/*
	 * Make tuple and any needed join variables available to ExecQual and
	 * ExecProject.  The EXCLUDED tuple is installed in ecxt_innertuple, while
	 * the target's existing tuple is installed in the scantuple.  EXCLUDED
	 * has been made to reference INNER_VAR in setrefs.c, but there is no
	 * other redirection.
	 */
	econtext->ecxt_scantuple = existing;
	econtext->ecxt_innertuple = excludedSlot;
	econtext->ecxt_outertuple = NULL;

	if (!ExecQual(onConflictSetWhere, econtext))
	{
		ExecClearTuple(existing);	/* see return below */
		InstrCountFiltered1(&mtstate->ps, 1);
		return true;			/* done with the tuple */
	}

	if (resultRelInfo->ri_WithCheckOptions != NIL)
	{
		/*
		 * Check target's existing tuple against UPDATE-applicable USING
		 * security barrier quals (if any), enforced here as RLS checks/WCOs.
		 *
		 * The rewriter creates UPDATE RLS checks/WCOs for UPDATE security
		 * quals, and stores them as WCOs of "kind" WCO_RLS_CONFLICT_CHECK,
		 * but that's almost the extent of its special handling for ON
		 * CONFLICT DO UPDATE.
		 *
		 * The rewriter will also have associated UPDATE applicable straight
		 * RLS checks/WCOs for the benefit of the ExecUpdate() call that
		 * follows.  INSERTs and UPDATEs naturally have mutually exclusive WCO
		 * kinds, so there is no danger of spurious over-enforcement in the
		 * INSERT or UPDATE path.
		 */
		ExecWithCheckOptions(WCO_RLS_CONFLICT_CHECK, resultRelInfo,
							 existing,
							 mtstate->ps.state);
	}

	/* Project the new tuple version */
	ExecProject(resultRelInfo->ri_onConflict->oc_ProjInfo);

	/*
	 * Note that it is possible that the target tuple has been modified in
	 * this session, after the above table_tuple_lock. We choose to not error
	 * out in that case, in line with ExecUpdate's treatment of similar cases.
	 * This can happen if an UPDATE is triggered from within ExecQual(),
	 * ExecWithCheckOptions() or ExecProject() above, e.g. by selecting from a
	 * wCTE in the ON CONFLICT's SET.
	 */
	*returning = ExecUpdate(context, resultRelInfo,
							conflictTid, NULL,
							resultRelInfo->ri_onConflict->oc_ProjSlot,
							canSetTag);

	/*
	 * Clear out existing tuple, as there might not be another conflict among
	 * the next input rows. Don't want to hold resources till the end of the
	 * query.
	 */
	ExecClearTuple(existing);
	return true;
}

/* Begin - ReqID:SRS-MYSQL-SQL-INSERT */
/*
 * ExecConflictUpdate --- execute INSERT ON DUPLICATE KEY UPDATE
 */
static bool
ExecConflictUpdate(ModifyTableContext *context,
			  ResultRelInfo *resultRelInfo,
			  ItemPointer conflictTid,
			  TupleTableSlot *excludedSlot,
			  bool canSetTag,
			  TupleTableSlot **returning)
{
	ModifyTableState *mtstate = context->mtstate;
	ExprContext *econtext = mtstate->ps.ps_ExprContext;
	Relation	 relation = resultRelInfo->ri_RelationDesc;
//	ExprState	*onConflictSetWhere = resultRelInfo->ri_onConflict->oc_WhereClause;
	TupleTableSlot *existing = mtstate->mt_upsert->us_existing;
	TM_FailureData tmfd;
	LockTupleMode lockmode;
	TM_Result	 test;
	Datum		 xminDatum;
	TransactionId xmin;
	bool		 isnull;

	/* Determine lock mode to use */
	lockmode = ExecUpdateLockMode(context->estate, resultRelInfo);

	/*
	 * Lock tuple for update.	Don't follow updates when tuple cannot be
	 * locked without doing so.  A row locking conflict here means our
	 * previous conclusion that the tuple is conclusively committed is not
	 * true anymore.
	 */
	test = table_tuple_lock(relation, conflictTid,
							 context->estate->es_snapshot,
							 existing, context->estate->es_output_cid,
							 lockmode, LockWaitBlock,
							 0,/* BUG#M0000470:FOR UPDATE WAIT integer */
							 0,
							 &tmfd);
	if ( test != TM_Ok && MYS_MODE == compatible_mode) /* BUG#Z5121 */
	{
		if(ItemPointerEquals(conflictTid, &(context->mtstate->mt_upsert->us_updateproj->tts_tid)) &&
			resultRelInfo->ri_RelationDesc->rd_node.relNode == context->mtstate->mt_upsert->us_updateproj->tts_tableOid)
		{
			resultRelInfo->ri_RelationDesc->rd_conflictTid = conflictTid;
			test = TM_Ok;
		}
	}
	switch (test)
	{
		case TM_Ok:
			/* success! */
			break;

		case TM_Invisible:

			/*
			 * This can occur when a just inserted tuple is updated again in
			 * the same command. E.g. because multiple rows with the same
			 * conflicting key values are inserted.
			 *
			 * This is somewhat similar to the ExecUpdate() TM_SelfModified
			 * case.  We do not want to proceed because it would lead to the
			 * same row being updated a second time in some unspecified order,
			 * and in contrast to plain UPDATEs there's no historical behavior
			 * to break.
			 *
			 * It is the user's responsibility to prevent this situation from
			 * occurring.	These problems are why the SQL standard similarly
			 * specifies that for SQL MERGE, an exception must be raised in
			 * the event of an attempt to update the same row twice.
			 */
			xminDatum = slot_getsysattr(existing,
									 MinTransactionIdAttributeNumber,
									 &isnull);
			Assert(!isnull);
			xmin = DatumGetTransactionId(xminDatum);

			if (TransactionIdIsCurrentTransactionId(xmin))
				ereport(ERROR,
					 (errcode(ERRCODE_CARDINALITY_VIOLATION),
					/* translator: %s is a SQL command name */
					  errmsg("%s command cannot affect row a second time",
							 "ON CONFLICT DO UPDATE"),
					  errhint("Ensure that no rows proposed for insertion within the same command have duplicate constrained values.")));

				/* This shouldn't happen */
			elog(ERROR, "attempted to lock invisible tuple");
			break;

		case TM_SelfModified:

			/*
			 * This state should never be reached. As a dirty snapshot is used
			 * to find conflicting tuples, speculative insertion wouldn't have
			 * seen this row to conflict with.
			 */
			elog(ERROR, "unexpected self-updated tuple");
			break;

		case TM_Updated:
			if (IsolationUsesXactSnapshot())
				ereport(ERROR,
						(errcode(ERRCODE_T_R_SERIALIZATION_FAILURE),
						 errmsg("could not serialize access due to concurrent update")));

			/*
			 * As long as we don't support an UPDATE of INSERT ON CONFLICT for
			 * a partitioned table we shouldn't reach to a case where tuple to
			 * be lock is moved to another partition due to concurrent update
			 * of the partition key.
			 */
			Assert(!ItemPointerIndicatesMovedPartitions(&tmfd.ctid));

			/*
			 * Tell caller to try again from the very start.
			 *
			 * It does not make sense to use the usual EvalPlanQual() style
			 * loop here, as the new version of the row might not conflict
			 * anymore, or the conflicting tuple has actually been deleted.
			 */
			ExecClearTuple(existing);
			return false;

		case TM_Deleted:
			if (IsolationUsesXactSnapshot())
				ereport(ERROR,
						(errcode(ERRCODE_T_R_SERIALIZATION_FAILURE),
						 errmsg("could not serialize access due to concurrent delete")));

			/* see TM_Updated case */
			Assert(!ItemPointerIndicatesMovedPartitions(&tmfd.ctid));
			ExecClearTuple(existing);
			return false;

		default:
			elog(ERROR, "unrecognized table_tuple_lock status: %u", test);
	}

	/* Success, the tuple is locked. */

	/*
	* Verify that the tuple is visible to our MVCC snapshot if the current
	* isolation level mandates that.
	*
	* It's not sufficient to rely on the check within ExecUpdate() as e.g.
	* CONFLICT ... WHERE clause may prevent us from reaching that.
	*
	* This means we only ever continue when a new command in the current
	* transaction could see the row, even though in READ COMMITTED mode the
	* tuple will not be visible according to the current statement's
	* snapshot.  This is in line with the way UPDATE deals with newer tuple
	* versions.
	*/
	ExecCheckTupleVisible(context->estate, relation, existing);

	/*
	 * Make tuple and any needed join variables available to ExecQual and
	 * ExecProject.  The EXCLUDED tuple is installed in ecxt_innertuple, while
	 * the target's existing tuple is installed in the scantuple.	EXCLUDED
	 * has been made to reference INNER_VAR in setrefs.c, but there is no
	 * other redirection.
	 */
	econtext->ecxt_scantuple = mtstate->mt_upsert->us_existing;
	econtext->ecxt_innertuple = excludedSlot;
	econtext->ecxt_outertuple = NULL;

//	if (!ExecQual(onConflictSetWhere, econtext))
//	{
//		ExecClearTuple(existing);	 /* see return below */
//		InstrCountFiltered1(&mtstate->ps, 1);
//		return true;			 /* done with the tuple */
//	}

	if (resultRelInfo->ri_WithCheckOptions != NIL)
	{
		/*
		 * Check target's existing tuple against UPDATE-applicable USING
		 * security barrier quals (if any), enforced here as RLS checks/WCOs.
		 *
		 * The rewriter creates UPDATE RLS checks/WCOs for UPDATE security
		 * quals, and stores them as WCOs of "kind" WCO_RLS_CONFLICT_CHECK,
		 * but that's almost the extent of its special handling for ON
		 * CONFLICT DO UPDATE.
		 *
		 * The rewriter will also have associated UPDATE applicable straight
		 * RLS checks/WCOs for the benefit of the ExecUpdate() call that
		 * follows.  INSERTs and UPDATEs naturally have mutually exclusive WCO
		 * kinds, so there is no danger of spurious over-enforcement in the
		 * INSERT or UPDATE path.
		 */
		ExecWithCheckOptions(WCO_RLS_CONFLICT_CHECK, resultRelInfo,
							 existing,
							 mtstate->ps.state);
	}

	/* Project the new tuple version */
	ExecProject(resultRelInfo->ri_updateProj);

	/*
	* Note that it is possible that the target tuple has been modified in
	* this session, after the above table_tuple_lock. We choose to not error
	* out in that case, in line with ExecUpdate's treatment of similar cases.
	* This can happen if an UPDATE is triggered from within ExecQual(),
	* ExecWithCheckOptions() or ExecProject() above, e.g. by selecting from a
	* wCTE in the ON CONFLICT's SET.
	*/
	*returning = ExecUpdate(context, resultRelInfo,
							conflictTid, NULL,
							mtstate->mt_upsert->us_updateproj,
							canSetTag);

	/*
	* Clear out existing tuple, as there might not be another conflict among
	* the next input rows. Don't want to hold resources till the end of the
	* query.
	*/
	ExecClearTuple(existing);
	return true;
}
/* End - ReqID:SRS-MYSQL-SQL-INSERT */

/*
 * Perform MERGE.
 */
static TupleTableSlot *
ExecMerge(ModifyTableContext *context, ResultRelInfo *resultRelInfo,
		  ItemPointer tupleid, bool canSetTag)
{
	bool		matched;

	/*-----
	 * If we are dealing with a WHEN MATCHED case (tupleid is valid), we
	 * execute the first action for which the additional WHEN MATCHED AND
	 * quals pass.  If an action without quals is found, that action is
	 * executed.
	 *
	 * Similarly, if we are dealing with WHEN NOT MATCHED case, we look at
	 * the given WHEN NOT MATCHED actions in sequence until one passes.
	 *
	 * Things get interesting in case of concurrent update/delete of the
	 * target tuple. Such concurrent update/delete is detected while we are
	 * executing a WHEN MATCHED action.
	 *
	 * A concurrent update can:
	 *
	 * 1. modify the target tuple so that it no longer satisfies the
	 *    additional quals attached to the current WHEN MATCHED action
	 *
	 *    In this case, we are still dealing with a WHEN MATCHED case.
	 *    We recheck the list of WHEN MATCHED actions from the start and
	 *    choose the first one that satisfies the new target tuple.
	 *
	 * 2. modify the target tuple so that the join quals no longer pass and
	 *    hence the source tuple no longer has a match.
	 *
	 *    In this case, the source tuple no longer matches the target tuple,
	 *    so we now instead find a qualifying WHEN NOT MATCHED action to
	 *    execute.
	 *
	 * XXX Hmmm, what if the updated tuple would now match one that was
	 * considered NOT MATCHED so far?
	 *
	 * A concurrent delete changes a WHEN MATCHED case to WHEN NOT MATCHED.
	 *
	 * ExecMergeMatched takes care of following the update chain and
	 * re-finding the qualifying WHEN MATCHED action, as long as the updated
	 * target tuple still satisfies the join quals, i.e., it remains a WHEN
	 * MATCHED case. If the tuple gets deleted or the join quals fail, it
	 * returns and we try ExecMergeNotMatched. Given that ExecMergeMatched
	 * always make progress by following the update chain and we never switch
	 * from ExecMergeNotMatched to ExecMergeMatched, there is no risk of a
	 * livelock.
	 */
	matched = tupleid != NULL;
	if (matched)
		matched = (*pg_exec_merge_matched_hook)(context, resultRelInfo, tupleid, canSetTag);

	/*
	 * Either we were dealing with a NOT MATCHED tuple or ExecMergeMatched()
	 * returned "false", indicating the previously MATCHED tuple no longer
	 * matches.
	 */
	if (!matched)
		ExecMergeNotMatched(context, resultRelInfo, canSetTag);

	/* No RETURNING support yet */
	return NULL;
}

/*
 * Check and execute the first qualifying MATCHED action. The current target
 * tuple is identified by tupleid.
 *
 * We start from the first WHEN MATCHED action and check if the WHEN quals
 * pass, if any. If the WHEN quals for the first action do not pass, we
 * check the second, then the third and so on. If we reach to the end, no
 * action is taken and we return true, indicating that no further action is
 * required for this tuple.
 *
 * If we do find a qualifying action, then we attempt to execute the action.
 *
 * If the tuple is concurrently updated, EvalPlanQual is run with the updated
 * tuple to recheck the join quals. Note that the additional quals associated
 * with individual actions are evaluated by this routine via ExecQual, while
 * EvalPlanQual checks for the join quals. If EvalPlanQual tells us that the
 * updated tuple still passes the join quals, then we restart from the first
 * action to look for a qualifying action. Otherwise, we return false --
 * meaning that a NOT MATCHED action must now be executed for the current
 * source tuple.
 */
bool
ExecMergeMatched(ModifyTableContext *context, ResultRelInfo *resultRelInfo,
				 ItemPointer tupleid, bool canSetTag)
{
	ModifyTableState *mtstate = context->mtstate;
	ItemPointerData lockedtid;
	TupleTableSlot *newslot;
	EState	   *estate = context->estate;
	ExprContext *econtext = mtstate->ps.ps_ExprContext;
	bool		isNull;
	EPQState   *epqstate = &mtstate->mt_epqstate;
	ListCell   *l;
	bool		no_further_action = true;

	/*
	 * If there are no WHEN MATCHED actions, we are done.
	 */
	if (resultRelInfo->ri_matchedMergeAction == NIL)
		return no_further_action;

	/*
	 * Make tuple and any needed join variables available to ExecQual and
	 * ExecProject. The target's existing tuple is installed in the scantuple.
	 * Again, this target relation's slot is required only in the case of a
	 * MATCHED tuple and UPDATE/DELETE actions.
	 */
	econtext->ecxt_scantuple = resultRelInfo->ri_oldTupleSlot;
	econtext->ecxt_innertuple = context->planSlot;
	econtext->ecxt_outertuple = NULL;

lmerge_matched:;
	if (resultRelInfo->ri_needLockTagTuple)
	{
		/*
		 * This locks even for CMD_DELETE, for CMD_NOTHING, and for tuples
		 * that don't match mas_whenqual.  MERGE on system catalogs is a minor
		 * use case, so don't bother optimizing those.
		 */
		LockTuple(resultRelInfo->ri_RelationDesc, tupleid,
				  InplaceUpdateTupleLock);
		lockedtid = *tupleid;
	}
	else
		ItemPointerSetInvalid(&lockedtid);

	/*
	 * This routine is only invoked for matched rows, and we must have found
	 * the tupleid of the target row in that case; fetch that tuple.
	 *
	 * We use SnapshotAny for this because we might get called again after
	 * EvalPlanQual returns us a new tuple, which may not be visible to our
	 * MVCC snapshot.
	 */

	if (!table_tuple_fetch_row_version(resultRelInfo->ri_RelationDesc,
									   tupleid,
									   SnapshotAny,
									   resultRelInfo->ri_oldTupleSlot))
		elog(ERROR, "failed to fetch the target tuple");

	foreach(l, resultRelInfo->ri_matchedMergeAction)
	{
		MergeActionState *relaction = (MergeActionState *) lfirst(l);
		CmdType		commandType = relaction->mas_action->commandType;
		List	   *recheckIndexes = NIL;
		TM_Result	result;
		UpdateContext updateCxt = {0};

		/*
		 * Test condition, if any.
		 *
		 * In the absence of any condition, we perform the action
		 * unconditionally (no need to check separately since ExecQual() will
		 * return true if there are no conditions to evaluate).
		 */
		if (!ExecQual(relaction->mas_whenqual, econtext))
			continue;

		/*
		 * Check if the existing target tuple meets the USING checks of
		 * UPDATE/DELETE RLS policies. If those checks fail, we throw an
		 * error.
		 *
		 * The WITH CHECK quals are applied in ExecUpdate() and hence we need
		 * not do anything special to handle them.
		 *
		 * NOTE: We must do this after WHEN quals are evaluated, so that we
		 * check policies only when they matter.
		 */
		if (resultRelInfo->ri_WithCheckOptions)
		{
			ExecWithCheckOptions(commandType == CMD_UPDATE ?
								 WCO_RLS_MERGE_UPDATE_CHECK : WCO_RLS_MERGE_DELETE_CHECK,
								 resultRelInfo,
								 resultRelInfo->ri_oldTupleSlot,
								 context->mtstate->ps.state);
		}

		/* Perform stated action */
		switch (commandType)
		{
			case CMD_UPDATE:

				/*
				 * Project the output tuple, and use that to update the table.
				 * We don't need to filter out junk attributes, because the
				 * UPDATE action's targetlist doesn't have any.
				 */
				newslot = ExecProject(relaction->mas_proj);

				context->relaction = relaction;
				context->GetUpdateNewTuple = mergeGetUpdateNewTuple;
				context->cpUpdateRetrySlot = NULL;

				if (!ExecUpdatePrologue(context, resultRelInfo,
										tupleid, NULL, newslot))
				{
					result = TM_Ok;
					goto out;
				}
				result = ExecUpdateAct(context, resultRelInfo, tupleid, NULL,
									   newslot, mtstate->canSetTag, &updateCxt);
				
				/*
				 * As in ExecUpdate(), if ExecUpdateAct() reports that a
				 * cross-partition update was done, then there's nothing else
				 * for us to do --- the UPDATE has been turned into a DELETE
				 * and an INSERT, and we must not perform any of the usual
				 * post-update tasks.
				 */
				if (updateCxt.crossPartUpdate)
				{
					mtstate->mt_merge_updated += 1;
					goto out;
				}

				if (result == TM_Ok && updateCxt.updated)
				{
					ExecUpdateEpilogue(context, &updateCxt, resultRelInfo,
									   tupleid, NULL, newslot, recheckIndexes);
					mtstate->mt_merge_updated += 1;
				}

				break;

			case CMD_DELETE:
				context->relaction = relaction;
				if (!ExecDeletePrologue(context, resultRelInfo, tupleid,
										NULL, NULL))
				{
					result = TM_Ok;
					goto out;
				}
				result = ExecDeleteAct(context, resultRelInfo, tupleid, false);
				if (result == TM_Ok)
				{
					ExecDeleteEpilogue(context, resultRelInfo, tupleid, NULL,
									   false);
					mtstate->mt_merge_deleted += 1;
				}
				break;

			case CMD_NOTHING:
				/* Doing nothing is always OK */
				result = TM_Ok;
				break;

			default:
				elog(ERROR, "unknown action in MERGE WHEN MATCHED clause");
		}

		switch (result)
		{
			case TM_Ok:
				/* all good; perform final actions */
				if (canSetTag)
					(estate->es_processed)++;

				break;

			case TM_SelfModified:

				/*
				 * The SQL standard disallows this for MERGE.
				 */
				if (TransactionIdIsCurrentTransactionId(context->tmfd.xmax))
					ereport(ERROR,
							(errcode(ERRCODE_CARDINALITY_VIOLATION),
					/* translator: %s is a SQL command name */
							 errmsg("%s command cannot affect row a second time",
									"MERGE"),
							 errhint("Ensure that not more than one source row matches any one target row.")));
				/* This shouldn't happen */
				elog(ERROR, "attempted to update or delete invisible tuple");
				no_further_action = false;
				goto out;

			case TM_Deleted:
				if (IsolationUsesXactSnapshot())
					ereport(ERROR,
							(errcode(ERRCODE_T_R_SERIALIZATION_FAILURE),
							 errmsg("could not serialize access due to concurrent delete")));

				/*
				 * If the tuple was already deleted, return to let caller
				 * handle it under NOT MATCHED clauses.
				 */
				no_further_action = false;
				goto out;

			case TM_Updated:
				{
					Relation	resultRelationDesc;
					TupleTableSlot *epqslot,
							   *inputslot;
					LockTupleMode lockmode;

					/*
					 * The target tuple was concurrently updated by some other
					 * transaction.
					 */

					/*
					 * If cpUpdateRetrySlot is set, ExecCrossPartitionUpdate()
					 * must have detected that the tuple was concurrently
					 * updated, so we restart the search for an appropriate
					 * WHEN MATCHED clause to process the updated tuple.
					 *
					 * In this case, ExecDelete() would already have performed
					 * EvalPlanQual() on the latest version of the tuple,
					 * which in turn would already have been loaded into
					 * ri_oldTupleSlot, so no need to do either of those
					 * things.
					 *
					 * XXX why do we not check the WHEN NOT MATCHED list in
					 * this case?
					 */
					if (!TupIsNull(context->cpUpdateRetrySlot))
						goto lmerge_matched;

					/*
					 * Otherwise, we run the EvalPlanQual() with the new
					 * version of the tuple. If EvalPlanQual() does not return
					 * a tuple, then we switch to the NOT MATCHED list of
					 * actions. If it does return a tuple and the join qual is
					 * still satisfied, then we just need to recheck the
					 * MATCHED actions, starting from the top, and execute the
					 * first qualifying action.
					 */
					resultRelationDesc = resultRelInfo->ri_RelationDesc;
					lockmode = ExecUpdateLockMode(estate, resultRelInfo);

					inputslot = EvalPlanQualSlot(epqstate, resultRelationDesc,
												 resultRelInfo->ri_RangeTableIndex);

					result = table_tuple_lock(resultRelationDesc, tupleid,
											  estate->es_snapshot,
											  inputslot, estate->es_output_cid,
											  lockmode, LockWaitBlock,
											  0,/* BUG#M0000470:FOR UPDATE WAIT integer */
											  TUPLE_LOCK_FLAG_FIND_LAST_VERSION,
											  &context->tmfd);
					switch (result)
					{
						case TM_Ok:
							epqslot = EvalPlanQual(epqstate,
												   resultRelationDesc,
												   resultRelInfo->ri_RangeTableIndex,
												   inputslot);

							/*
							 * If we got no tuple, or the tuple we get has a
							 * NULL ctid, go back to caller: this one is not a
							 * MATCHED tuple anymore, so they can retry with
							 * NOT MATCHED actions.
							 */
							if (TupIsNull(epqslot))
							{
								no_further_action = false;
								goto out;
							}

							(void) ExecGetJunkAttribute(epqslot,
														resultRelInfo->ri_RowIdAttNo,
														&isNull);
							if (isNull)
							{
								no_further_action = false;
								goto out;
							}

							/*
							 * When a tuple was updated and migrated to
							 * another partition concurrently, the current
							 * MERGE implementation can't follow.  There's
							 * probably a better way to handle this case, but
							 * it'd require recognizing the relation to which
							 * the tuple moved, and setting our current
							 * resultRelInfo to that.
							 */
							if (ItemPointerIndicatesMovedPartitions(&context->tmfd.ctid))
								ereport(ERROR,
										(errcode(ERRCODE_T_R_SERIALIZATION_FAILURE),
										 errmsg("tuple to be deleted was already moved to another partition due to concurrent update")));

							/*
							 * A non-NULL ctid means that we are still dealing
							 * with MATCHED case. Restart the loop so that we
							 * apply all the MATCHED rules again, to ensure
							 * that the first qualifying WHEN MATCHED action
							 * is executed.
							 *
							 * Update tupleid to that of the new tuple, for
							 * the refetch we do at the top.
							 */
							if (resultRelInfo->ri_needLockTagTuple)
								UnlockTuple(resultRelInfo->ri_RelationDesc,
											&lockedtid,
											InplaceUpdateTupleLock);
							ItemPointerCopy(&context->tmfd.ctid, tupleid);
							goto lmerge_matched;

						case TM_Deleted:

							/*
							 * tuple already deleted; tell caller to run NOT
							 * MATCHED actions
							 */
							no_further_action = false;
							goto out;

						case TM_SelfModified:

							/*
							 * This can be reached when following an update
							 * chain from a tuple updated by another session,
							 * reaching a tuple that was already updated in
							 * this transaction. If previously modified by
							 * this command, ignore the redundant update,
							 * otherwise error out.
							 *
							 * See also response to TM_SelfModified in
							 * ExecUpdate().
							 */
							if (context->tmfd.cmax != estate->es_output_cid)
								ereport(ERROR,
										(errcode(ERRCODE_TRIGGERED_DATA_CHANGE_VIOLATION),
										 errmsg("tuple to be updated or deleted was already modified by an operation triggered by the current command"),
										 errhint("Consider using an AFTER trigger instead of a BEFORE trigger to propagate changes to other rows.")));
							return false;

						default:
							/* see table_tuple_lock call in ExecDelete() */
							elog(ERROR, "unexpected table_tuple_lock status: %u",
								 result);
							no_further_action = false;
							goto out;
					}
				}

			case TM_Invisible:
			case TM_WouldBlock:
			case TM_BeingModified:
				/* these should not occur */
				elog(ERROR, "unexpected tuple operation result: %d", result);
				break;
		}

		/*
		 * We've activated one of the WHEN clauses, so we don't search
		 * further. This is required behaviour, not an optimization.
		 */
		break;
	}

	/*
	 * Successfully executed an action or no qualifying action was found.
	 */
out:
	if (ItemPointerIsValid(&lockedtid))
		UnlockTuple(resultRelInfo->ri_RelationDesc, &lockedtid,
					InplaceUpdateTupleLock);
	return no_further_action;
}


/*
 * Execute the first qualifying NOT MATCHED action.
 */
static void
ExecMergeNotMatched(ModifyTableContext *context, ResultRelInfo *resultRelInfo,
					bool canSetTag)
{
	ModifyTableState *mtstate = context->mtstate;
	ExprContext *econtext = mtstate->ps.ps_ExprContext;
	List	   *actionStates = NIL;
	ListCell   *l;

	/*
	 * For INSERT actions, the root relation's merge action is OK since the
	 * INSERT's targetlist and the WHEN conditions can only refer to the
	 * source relation and hence it does not matter which result relation we
	 * work with.
	 *
	 * XXX does this mean that we can avoid creating copies of actionStates on
	 * partitioned tables, for not-matched actions?
	 */
	actionStates = resultRelInfo->ri_notMatchedMergeAction;

	/*
	 * Make source tuple available to ExecQual and ExecProject. We don't need
	 * the target tuple, since the WHEN quals and targetlist can't refer to
	 * the target columns.
	 */
	econtext->ecxt_scantuple = NULL;
	econtext->ecxt_innertuple = context->planSlot;
	econtext->ecxt_outertuple = NULL;

	foreach(l, actionStates)
	{
		MergeActionState *action = (MergeActionState *) lfirst(l);
		CmdType		commandType = action->mas_action->commandType;
		TupleTableSlot *newslot;

		/*
		 * Test condition, if any.
		 *
		 * In the absence of any condition, we perform the action
		 * unconditionally (no need to check separately since ExecQual() will
		 * return true if there are no conditions to evaluate).
		 */
		if (!ExecQual(action->mas_whenqual, econtext))
			continue;

		/* Perform stated action */
		switch (commandType)
		{
			case CMD_INSERT:

				/*
				 * Project the tuple.  In case of a partitioned table, the
				 * projection was already built to use the root's descriptor,
				 * so we don't need to map the tuple here.
				 */
				newslot = ExecProject(action->mas_proj);
				context->relaction = action;

				(void) ExecInsert(context, mtstate->rootResultRelInfo, newslot,
								  canSetTag, NULL, NULL);
				mtstate->mt_merge_inserted += 1;
				break;
			case CMD_NOTHING:
				/* Do nothing */
				break;
			default:
				elog(ERROR, "unknown action in MERGE WHEN NOT MATCHED clause");
		}

		/*
		 * We've activated one of the WHEN clauses, so we don't search
		 * further. This is required behaviour, not an optimization.
		 */
		break;
	}
}

/*
 * Initialize state for execution of MERGE.
 */
void
ExecInitMerge(ModifyTableState *mtstate, EState *estate)
{
	ModifyTable *node = (ModifyTable *) mtstate->ps.plan;
	ResultRelInfo *rootRelInfo = mtstate->rootResultRelInfo;
	ResultRelInfo *resultRelInfo;
	ExprContext *econtext;
	ListCell   *lc;
	int			i;

	if (node->mergeActionLists == NIL)
		return;

	mtstate->mt_merge_subcommands = 0;

	if (mtstate->ps.ps_ExprContext == NULL)
		ExecAssignExprContext(estate, &mtstate->ps);
	econtext = mtstate->ps.ps_ExprContext;

	/*
	 * Create a MergeActionState for each action on the mergeActionList and
	 * add it to either a list of matched actions or not-matched actions.
	 *
	 * Similar logic appears in ExecInitPartitionInfo(), so if changing
	 * anything here, do so there too.
	 */
	i = 0;
	foreach(lc, node->mergeActionLists)
	{
		List	   *mergeActionList = lfirst(lc);
		TupleDesc	relationDesc;
		ListCell   *l;

		resultRelInfo = mtstate->resultRelInfo + i;
		i++;
		relationDesc = RelationGetDescr(resultRelInfo->ri_RelationDesc);

		/* initialize slots for MERGE fetches from this rel */
		if (unlikely(!resultRelInfo->ri_projectNewInfoValid))
			ExecInitMergeTupleSlots(mtstate, resultRelInfo);

		foreach(l, mergeActionList)
		{
			MergeAction *action = (MergeAction *) lfirst(l);
			MergeActionState *action_state;
			TupleTableSlot *tgtslot;
			TupleDesc	tgtdesc;
			List	  **list;

			/*
			 * Build action merge state for this rel.  (For partitions,
			 * equivalent code exists in ExecInitPartitionInfo.)
			 */
			action_state = makeNode(MergeActionState);
			action_state->mas_action = action;
			action_state->mas_whenqual = ExecInitQual((List *) action->qual,
													  &mtstate->ps);
			/*
			 * We create two lists - one for WHEN MATCHED actions and one for
			 * WHEN NOT MATCHED actions - and stick the MergeActionState into
			 * the appropriate list.
			 */
			if (action_state->mas_action->matched)
				list = &resultRelInfo->ri_matchedMergeAction;
			else
				list = &resultRelInfo->ri_notMatchedMergeAction;
			*list = lappend(*list, action_state);

			switch (action->commandType)
			{
				case CMD_INSERT:
					ExecCheckPlanOutput(rootRelInfo->ri_RelationDesc,
										action->targetList);

					/*
					 * If the MERGE targets a partitioned table, any INSERT
					 * actions must be routed through it, not the child
					 * relations. Initialize the routing struct and the root
					 * table's "new" tuple slot for that, if not already done.
					 * The projection we prepare, for all relations, uses the
					 * root relation descriptor, and targets the plan's root
					 * slot.  (This is consistent with the fact that we
					 * checked the plan output to match the root relation,
					 * above.)
					 */
					if (rootRelInfo->ri_RelationDesc->rd_rel->relkind ==
						RELKIND_PARTITIONED_TABLE)
					{
						if (mtstate->mt_partition_tuple_routing == NULL)
						{
							/*
							 * Initialize planstate for routing if not already
							 * done.
							 *
							 * Note that the slot is managed as a standalone
							 * slot belonging to ModifyTableState, so we pass
							 * NULL for the 2nd argument.
							 */
							mtstate->mt_root_tuple_slot =
								table_slot_create(rootRelInfo->ri_RelationDesc,
												  NULL);
							mtstate->mt_partition_tuple_routing =
								ExecSetupPartitionTupleRouting(estate,
															   rootRelInfo->ri_RelationDesc);
						}
						tgtslot = mtstate->mt_root_tuple_slot;
						tgtdesc = RelationGetDescr(rootRelInfo->ri_RelationDesc);
					}
					else
					{
						/* not partitioned? use the stock relation and slot */
						tgtslot = resultRelInfo->ri_newTupleSlot;
						tgtdesc = RelationGetDescr(resultRelInfo->ri_RelationDesc);
					}

					action_state->mas_proj =
						ExecBuildProjectionInfo(action->targetList, econtext,
												tgtslot,
												&mtstate->ps,
												tgtdesc);

					mtstate->mt_merge_subcommands |= MERGE_INSERT;
					break;
				case CMD_UPDATE:
					action_state->mas_proj =
						ExecBuildUpdateProjection(action->targetList,
												  true,
												  action->updateColnos,
												  relationDesc,
												  econtext,
												  resultRelInfo->ri_newTupleSlot,
												  &mtstate->ps);
					mtstate->mt_merge_subcommands |= MERGE_UPDATE;
					break;
				case CMD_DELETE:
					mtstate->mt_merge_subcommands |= MERGE_DELETE;
					break;
				case CMD_NOTHING:
					break;
				default:
					elog(ERROR, "unknown operation");
					break;
			}
		}
	}
}

/*
 * Initializes the tuple slots in a ResultRelInfo for any MERGE action.
 *
 * We mark 'projectNewInfoValid' even though the projections themselves
 * are not initialized here.
 */
void
ExecInitMergeTupleSlots(ModifyTableState *mtstate,
						ResultRelInfo *resultRelInfo)
{
	EState	   *estate = mtstate->ps.state;

	Assert(!resultRelInfo->ri_projectNewInfoValid);

	resultRelInfo->ri_oldTupleSlot =
		table_slot_create(resultRelInfo->ri_RelationDesc,
						  &estate->es_tupleTable);
	resultRelInfo->ri_newTupleSlot =
		table_slot_create(resultRelInfo->ri_RelationDesc,
						  &estate->es_tupleTable);
	resultRelInfo->ri_projectNewInfoValid = true;
}

/*
 * Callback for ModifyTableContext->GetUpdateNewTuple for use by MERGE.  It
 * computes the updated tuple by projecting from the current merge action's
 * projection.
 */
TupleTableSlot *
mergeGetUpdateNewTuple(ResultRelInfo *relinfo,
					   TupleTableSlot *planSlot,
					   TupleTableSlot *oldSlot,
					   MergeActionState *relaction)
{
	ExprContext *econtext = relaction->mas_proj->pi_exprContext;

	econtext->ecxt_scantuple = oldSlot;
	econtext->ecxt_innertuple = planSlot;

	return ExecProject(relaction->mas_proj);
}

/* Begin - SRS-SQL-MULTITABLEINSERT */
/*
 * Initialize state for execution of insert all/first.
 */
static void
ExecInitMultiInsert(ModifyTableState *mtstate, EState *estate)
{
	ModifyTable *node = (ModifyTable *) mtstate->ps.plan;
	ResultRelInfo *resultRelInfo;
	ExprContext *econtext;
	ListCell   *lc;

	if (node->multiInsertActionLists == NIL)
		return;

	if (mtstate->ps.ps_ExprContext == NULL)
		ExecAssignExprContext(estate, &mtstate->ps);
	econtext = mtstate->ps.ps_ExprContext;

	/*
	 * Create a ConditionInsertActionState for each action on the mergeActionList and
	 * add it to either a list of matched actions or not-matched actions.
	 *
	 * Similar logic appears in ExecInitPartitionInfo(), so if changing
	 * anything here, do so there too.
	 */
	foreach(lc, node->multiInsertActionLists)
	{
		ConditionInsertAction *condinsert = (ConditionInsertAction *) lfirst(lc);
		ListCell   *llc;
		ConditionInsertState *cstate = makeNode(ConditionInsertState);

		cstate->whenqual = ExecInitQual((List *) condinsert->qual,
													  &mtstate->ps);
		cstate->is_else = condinsert->is_else;

		foreach (llc, condinsert->intos)
		{
			InsertIntoAction *intoa = (InsertIntoAction *) lfirst(llc);
			TupleTableSlot *tgtslot;
			TupleDesc	tgtdesc;

			resultRelInfo = mtstate->resultRelInfo + intoa->target_rte - 1;

			/* initialize slots for insert fetches from this rel */
			if (unlikely(!resultRelInfo->ri_projectNewInfoValid))
				ExecInitMergeTupleSlots(mtstate, resultRelInfo);

			ExecCheckPlanOutput(resultRelInfo->ri_RelationDesc,
										intoa->targetList);
			if (resultRelInfo->ri_RelationDesc->rd_rel->relkind ==
						RELKIND_PARTITIONED_TABLE &&
				resultRelInfo->ri_partition_tuple_routing == NULL)
			{
				PartitionDirectory save_direc = estate->es_partition_directory;

				resultRelInfo->ri_partition_tuple_routing =
								ExecSetupPartitionTupleRouting(estate,
															   resultRelInfo->ri_RelationDesc);
				resultRelInfo->ri_part_dir = estate->es_partition_directory;
				estate->es_partition_directory = save_direc;
			}
			tgtslot = resultRelInfo->ri_newTupleSlot;
			tgtdesc = RelationGetDescr(resultRelInfo->ri_RelationDesc);

			cstate->projs = lappend(cstate->projs,
							ExecBuildProjectionInfo(intoa->targetList, econtext,
												tgtslot,
												&mtstate->ps,
												tgtdesc));
		}
		cstate->intos = condinsert->intos;
		mtstate->mt_multi_inserts = lappend(mtstate->mt_multi_inserts,
								cstate);
	}
}


/*
 * handle Insert ALL/FIRST
 */
static TupleTableSlot *
ExecMultiInsert(ModifyTableContext *context,
						ResultRelInfo *resultRelInfo,
						TupleTableSlot *slot,
						bool canSetTag)
{
	ModifyTableState *mtstate = context->mtstate;
	TupleTableSlot *newslot;
	ExprContext *econtext = mtstate->ps.ps_ExprContext;
	ListCell   *l;
	bool	has_match = false;
	bool	is_multi_first = ((ModifyTable *) mtstate->ps.plan)->multiInsertFirst;

	/*
	 * Make source tuple available to ExecQual and ExecProject. We don't need
	 * the target tuple, since the WHEN quals and targetlist can't refer to
	 * the target columns.
	 */
	econtext->ecxt_scantuple = NULL;
	econtext->ecxt_innertuple = slot;
	econtext->ecxt_outertuple = NULL;

	foreach (l, mtstate->mt_multi_inserts)
	{
		ConditionInsertState *cstate = (ConditionInsertState *) lfirst(l);
		ListCell *lc;
		ListCell *lc1;

		/*
		 * Test condition, if any
		 */
		if (!ExecQual(cstate->whenqual, econtext))
			continue;

		/*
		 * ignore else stmt, which has match
		 */
		if (cstate->is_else && has_match)
			continue;

		forboth(lc, cstate->intos, lc1, cstate->projs)
		{
			ProjectionInfo *proj = (ProjectionInfo *) lfirst(lc1);
			InsertIntoAction *intoa = (InsertIntoAction *) lfirst(lc);
			TransitionCaptureState *save_catptrue = mtstate->mt_transition_capture;
			struct PartitionTupleRouting *save_routing = mtstate->mt_partition_tuple_routing;
			ResultRelInfo *save_rootinfo = mtstate->rootResultRelInfo;
			PartitionDirectory save_dir = context->estate->es_partition_directory;

			resultRelInfo = mtstate->resultRelInfo + intoa->target_rte - 1;
			mtstate->mt_partition_tuple_routing = resultRelInfo->ri_partition_tuple_routing;
			mtstate->rootResultRelInfo = resultRelInfo;
			Assert(intoa->target_rte <= mtstate->mt_nrels);
			mtstate->mt_transition_capture = mtstate->mt_transition_captures[intoa->target_rte - 1];
			context->estate->es_partition_directory = resultRelInfo->ri_part_dir;

			newslot = ExecProject(proj);
			(void ) ExecInsert(context, resultRelInfo, newslot, canSetTag,
								NULL, NULL);
			/*
			 * es_partition_directory mabye set in ExecFindPartition,
			 * so we save it in resultRenInfo
			 */
			resultRelInfo->ri_part_dir = context->estate->es_partition_directory;

			/* recover mtstate attributes */
			mtstate->mt_transition_capture = save_catptrue;
			mtstate->mt_partition_tuple_routing = save_routing;
			mtstate->rootResultRelInfo = save_rootinfo;
			context->estate->es_partition_directory = save_dir;

			mtstate->mt_multi_inserted += 1;
		}
		/* first,we can break */
		if (is_multi_first)
			break;
		has_match = true;
	}

	return NULL;
}

/* End - SRS-SQL-MULTITABLEINSERT */


/*
 * Process BEFORE EACH STATEMENT triggers
 */
static void
fireBSTriggers(ModifyTableState *node)
{
	ModifyTable *plan = (ModifyTable *) node->ps.plan;

	/* Begin - ReqID:SRS-MYSQL-UPDATE */
	ResultRelInfo   *resultRelInfo;
	int				resultRelationNum = 0;
	int				i = 0;
	bool			multirel = false;

	if (MYS_MODE == compatible_mode && node->mt_ResultTupleSlots) /* BUG#Z5121 */
	{
		resultRelInfo = node->resultRelInfo;
		resultRelationNum = node->mt_ResultTupleSlots ? list_length(node->mt_ResultTupleSlots) : 1;
		multirel = true;
	}
	else
		resultRelInfo = node->rootResultRelInfo;
	/* END - ReqID:SRS-MYSQL-UPDATE */

	switch (node->operation)
	{
		case CMD_INSERT:
			ExecBSInsertTriggers(node->ps.state, resultRelInfo);
			if (plan->onConflictAction == ONCONFLICT_UPDATE)
				ExecBSUpdateTriggers(node->ps.state,
									 resultRelInfo);
			
			/* Begin - ReqID:SRS-MYSQL-SQL-INSERT */
			if (plan->upsertAction == UPSERT_UPDATE)
				ExecBSUpdateTriggers(node->ps.state,
									 resultRelInfo);
			/* End - ReqID:SRS-MYSQL-SQL-INSERT */
			break;
		case CMD_UPDATE:
			/* Begin - ReqID:SRS-MYSQL-UPDATE */
			if (multirel)
			{
				for(i = 0; i < resultRelationNum; i++)
				{
					if (node->mt_partresultRelInfo)
					{
						ResultRelInfo   *curresultRelInfo;
						curresultRelInfo = list_nth(node->mt_partresultRelInfo, i);
						if (curresultRelInfo->ri_RootResultRelInfo)
							resultRelInfo = curresultRelInfo->ri_RootResultRelInfo;
						else
							resultRelInfo = curresultRelInfo;
						ExecBSUpdateTriggers(node->ps.state, resultRelInfo);
					}
					else
					{
						if (resultRelInfo->ri_RootResultRelInfo && 
							(node->mt_isexits_rootrel || !node->mt_partresultRelInfo))
							resultRelInfo->ri_RootResultRelInfo = NULL;
						ExecBSUpdateTriggers(node->ps.state, resultRelInfo);
						resultRelInfo++;
					}
				}
			}
			else
				ExecBSUpdateTriggers(node->ps.state, resultRelInfo);
			/* END - ReqID:SRS-MYSQL-UPDATE */
			break;
		case CMD_DELETE:
			/* Begin - ReqID:SRS-MYSQL-SQL-DELETE */
			if (multirel)
			{
				for(i = 0; i < resultRelationNum; i++)
				{
					if (node->mt_partresultRelInfo)
					{
						ResultRelInfo   *curresultRelInfo;
						curresultRelInfo = list_nth(node->mt_partresultRelInfo, i);
						if (curresultRelInfo->ri_RootResultRelInfo)
							resultRelInfo = curresultRelInfo->ri_RootResultRelInfo;
						else
							resultRelInfo = curresultRelInfo;
						ExecBSDeleteTriggers(node->ps.state, resultRelInfo);
					}
					else
					{
						ExecBSDeleteTriggers(node->ps.state, resultRelInfo);
						resultRelInfo++;
					}
				}
			}
			else
				ExecBSDeleteTriggers(node->ps.state, resultRelInfo);
			/* End - ReqID:SRS-MYSQL-SOL-DELETE */
			break;
		case CMD_MERGE:
			if (node->mt_merge_subcommands & MERGE_INSERT)
				ExecBSInsertTriggers(node->ps.state, resultRelInfo);
			if (node->mt_merge_subcommands & MERGE_UPDATE)
				ExecBSUpdateTriggers(node->ps.state, resultRelInfo);
			if (node->mt_merge_subcommands & MERGE_DELETE)
				ExecBSDeleteTriggers(node->ps.state, resultRelInfo);
			break;
		/* Begin - SRS-SQL-MULTITABLEINSERT */
		case CMD_MULTI_INSERT:
			{
				ListCell *lc;

				foreach (lc, node->mt_multi_inserts)
				{
					ConditionInsertState *cstate = (ConditionInsertState *) lfirst(lc);
					ListCell *llc;

					foreach (llc, cstate->intos)
					{
						InsertIntoAction *intoa = (InsertIntoAction *) lfirst(llc);

						resultRelInfo = node->resultRelInfo + intoa->target_rte - 1;

						ExecBSInsertTriggers(node->ps.state, resultRelInfo);
					}
				}
			}
			break;
		/* End - SRS-SQL-MULTITABLEINSERT */
		default:
			elog(ERROR, "unknown operation");
			break;
	}
}

/*
 * Process AFTER EACH STATEMENT triggers
 */
static void
fireASTriggers(ModifyTableState *node)
{
	ModifyTable *plan = (ModifyTable *) node->ps.plan;

	/* Begin - ReqID:SRS-MYSQL-UPDATE */
	ResultRelInfo   *resultRelInfo;
	int				resultRelationNum = 0;
	int				i = 0;
	bool			multirel = false;

	if (MYS_MODE == compatible_mode && node->mt_ResultTupleSlots) /* BUG#Z5121 */
	{
		resultRelInfo = node->resultRelInfo;
		resultRelationNum = node->mt_ResultTupleSlots ? list_length(node->mt_ResultTupleSlots) : 1;
		multirel = true;
	}
	else
		resultRelInfo = node->rootResultRelInfo;
	/* END - ReqID:SRS-MYSQL-UPDATE */

	switch (node->operation)
	{
		case CMD_INSERT:
			if (plan->onConflictAction == ONCONFLICT_UPDATE)
				ExecASUpdateTriggers(node->ps.state,
									 resultRelInfo,
									 node->mt_oc_transition_capture);
			
			/* Begin - ReqID:SRS-MYSQL-SQL-INSERT */
			if (plan->upsertAction == UPSERT_UPDATE)
				ExecASUpdateTriggers(node->ps.state,
									 resultRelInfo,
									 node->mt_oc_transition_capture);
			/* End - ReqID:SRS-MYSQL-SQL-INSERT */
			ExecASInsertTriggers(node->ps.state, resultRelInfo,
								 node->mt_transition_capture);
			break;
		case CMD_UPDATE:
			/* Begin - ReqID:SRS-MYSQL-UPDATE */
			if (multirel)
			{
				for(i = 0; i < resultRelationNum; i++)
				{
					if (node->mt_partresultRelInfo)
					{
						ResultRelInfo   *curresultRelInfo;
						curresultRelInfo = list_nth(node->mt_partresultRelInfo, i);
						if (curresultRelInfo->ri_RootResultRelInfo)
							resultRelInfo = curresultRelInfo->ri_RootResultRelInfo;
						else
							resultRelInfo = curresultRelInfo;
						ExecASUpdateTriggers(node->ps.state, resultRelInfo,
										 node->mt_transition_capture);
					}
					else
					{
						if (resultRelInfo->ri_RootResultRelInfo && node->mt_isexits_rootrel)
							resultRelInfo->ri_RootResultRelInfo = NULL;
						ExecASUpdateTriggers(node->ps.state, resultRelInfo,
										 node->mt_transition_capture);
						resultRelInfo++;
					}
				}
			}
			else
				ExecASUpdateTriggers(node->ps.state, resultRelInfo,
										 node->mt_transition_capture);
			/* END - ReqID:SRS-MYSQL-UPDATE */
			break;
		case CMD_DELETE:
			/* Begin - ReqID:SRS-MYSQL-SQL-DELETE */
			if (multirel)
			{
				for(i = 0; i < resultRelationNum; i++)
				{
					if (node->mt_partresultRelInfo)
					{
						ResultRelInfo   *curresultRelInfo;
						curresultRelInfo = list_nth(node->mt_partresultRelInfo, i);
						if (curresultRelInfo->ri_RootResultRelInfo)
							resultRelInfo = curresultRelInfo->ri_RootResultRelInfo;
						else
							resultRelInfo = curresultRelInfo;
						ExecASUpdateTriggers(node->ps.state, resultRelInfo,
										 node->mt_transition_capture);
					}
					else
					{
						ExecASDeleteTriggers(node->ps.state, resultRelInfo,
									 node->mt_transition_capture);
						resultRelInfo++;
					}
				}
			}
			else
				ExecASDeleteTriggers(node->ps.state, resultRelInfo,
								 node->mt_transition_capture);
			/* End - ReqID:SRS-MYSQL-SQL-DELETE */
			break;
		case CMD_MERGE:
			/* Begin - ReqID:SRS-SQL-MERGE */
			if (compatible_mode == ORA_MODE)
			{
				if (node->mt_merge_subcommands & MERGE_UPDATE)
					ExecASUpdateTriggers(node->ps.state, resultRelInfo,
										 node->mt_transition_capture);
				if (node->mt_merge_subcommands & MERGE_DELETE)
					ExecASDeleteTriggers(node->ps.state, resultRelInfo,
										 node->mt_transition_capture);
				if (node->mt_merge_subcommands & MERGE_INSERT)
					ExecASInsertTriggers(node->ps.state, resultRelInfo,
										 node->mt_transition_capture);
			}/* End - ReqID:SRS-SQL-MERGE */
			else
			{
				if (node->mt_merge_subcommands & MERGE_DELETE)
					ExecASDeleteTriggers(node->ps.state, resultRelInfo,
										 node->mt_transition_capture);
				if (node->mt_merge_subcommands & MERGE_UPDATE)
					ExecASUpdateTriggers(node->ps.state, resultRelInfo,
										 node->mt_transition_capture);
				if (node->mt_merge_subcommands & MERGE_INSERT)
					ExecASInsertTriggers(node->ps.state, resultRelInfo,
										 node->mt_transition_capture);
			}
			break;
		/* Begin - SRS-SQL-MULTITABLEINSERT */
		case CMD_MULTI_INSERT:
			{
				ListCell *lc;

				foreach (lc, node->mt_multi_inserts)
				{
					ConditionInsertState *cstate = (ConditionInsertState *) lfirst(lc);
					ListCell *llc;

					foreach (llc, cstate->intos)
					{
						InsertIntoAction *intoa = (InsertIntoAction *) lfirst(llc);

						resultRelInfo = node->resultRelInfo + intoa->target_rte - 1;

						ExecASInsertTriggers(node->ps.state, resultRelInfo,
								 node->mt_transition_captures[intoa->target_rte - 1]);
					}
				}
			}
			break;
		/* End - SRS-SQL-MULTITABLEINSERT */
		default:
			elog(ERROR, "unknown operation");
			break;
	}
}

/*
 * Set up the state needed for collecting transition tuples for AFTER
 * triggers.
 */
static void
ExecSetupTransitionCaptureState(ModifyTableState *mtstate, EState *estate)
{
	ModifyTable *plan = (ModifyTable *) mtstate->ps.plan;
	ResultRelInfo *targetRelInfo = mtstate->rootResultRelInfo;

	/* Check for transition tables on the directly targeted relation. */
	/* Begin - SRS-SQL-MULTITABLEINSERT */
	if (mtstate->operation == CMD_MULTI_INSERT)
	{
		int i = 0;
		ListCell *lc;
		ModifyTable *plan = (ModifyTable *) mtstate->ps.plan;

		mtstate->mt_transition_captures = (struct TransitionCaptureState **) palloc0(mtstate->mt_nrels *
			sizeof(struct TransitionCaptureState *));

		foreach (lc, plan->resultRelations)
		{
			int rt_index = lfirst_int(lc);
			ResultRelInfo *relinfo = mtstate->resultRelInfo + rt_index - 1;

			Assert(rt_index <= mtstate->mt_nrels);
			Assert(mtstate->mt_transition_captures[rt_index - 1] == NULL);
			mtstate->mt_transition_captures[rt_index - 1] =
				MakeTransitionCaptureState(relinfo->ri_TrigDesc,
								   RelationGetRelid(relinfo->ri_RelationDesc),
								   mtstate->operation);
			i++;
		}
		Assert(i == mtstate->mt_nrels);
	}
	else
		mtstate->mt_transition_capture =
			MakeTransitionCaptureState(targetRelInfo->ri_TrigDesc,
								   RelationGetRelid(targetRelInfo->ri_RelationDesc),
								   mtstate->operation);
	/* End - SRS-SQL-MULTITABLEINSERT */
	if (plan->operation == CMD_INSERT &&
		plan->onConflictAction == ONCONFLICT_UPDATE)
		mtstate->mt_oc_transition_capture =
			MakeTransitionCaptureState(targetRelInfo->ri_TrigDesc,
									   RelationGetRelid(targetRelInfo->ri_RelationDesc),
									   CMD_UPDATE);
}

/*
 * ExecPrepareTupleRouting --- prepare for routing one tuple
 *
 * Determine the partition in which the tuple in slot is to be inserted,
 * and return its ResultRelInfo in *partRelInfo.  The return value is
 * a slot holding the tuple of the partition rowtype.
 *
 * This also sets the transition table information in mtstate based on the
 * selected partition.
 */
static TupleTableSlot *
ExecPrepareTupleRouting(ModifyTableState *mtstate,
						EState *estate,
						PartitionTupleRouting *proute,
						ResultRelInfo *targetRelInfo,
						TupleTableSlot *slot,
						ResultRelInfo **partRelInfo)
{
	ResultRelInfo *partrel;
	TupleConversionMap *map;

	/*
	 * Lookup the target partition's ResultRelInfo.  If ExecFindPartition does
	 * not find a valid partition for the tuple in 'slot' then an error is
	 * raised.  An error may also be raised if the found partition is not a
	 * valid target for INSERTs.  This is required since a partitioned table
	 * UPDATE to another partition becomes a DELETE+INSERT.
	 */
	partrel = ExecFindPartition(mtstate, targetRelInfo, proute, slot, estate);

	/*
	 * If we're capturing transition tuples, we might need to convert from the
	 * partition rowtype to root partitioned table's rowtype.  But if there
	 * are no BEFORE triggers on the partition that could change the tuple, we
	 * can just remember the original unconverted tuple to avoid a needless
	 * round trip conversion.
	 */
	if (mtstate->mt_transition_capture != NULL)
	{
		bool		has_before_insert_row_trig;

		has_before_insert_row_trig = (partrel->ri_TrigDesc &&
									  partrel->ri_TrigDesc->trig_insert_before_row);

		mtstate->mt_transition_capture->tcs_original_insert_tuple =
			!has_before_insert_row_trig ? slot : NULL;
	}

	/*
	 * Convert the tuple, if necessary.
	 */
	map = partrel->ri_RootToPartitionMap;
	if (map != NULL)
	{
		TupleTableSlot *new_slot = partrel->ri_PartitionTupleSlot;

		slot = execute_attr_map_slot(map->attrMap, slot, new_slot);
	}

	*partRelInfo = partrel;
	return slot;
}


/* Begin - ReqID:SRS-MYSQL-SQL-REPLACE */
/*
 * For REPLACE, we have to insert the tuple into the target relation
 * (or partition thereof) and insert appropriate tuples into the index relations.
 * If the insertion conflicts with the primary key and unique constraints, 
 * delete first and then insert.
 */
static TupleTableSlot *
ExecReplace(ModifyTableContext *context,
		   ResultRelInfo *resultRelInfo,
		   TupleTableSlot *slot,
		   bool canSetTag,
		   TupleTableSlot **inserted_tuple,
		   ResultRelInfo **insert_destrel,
		   HeapTuple oldtuple)
{
	ModifyTableState *mtstate = context->mtstate;
	EState	   *estate = context->estate;
	ModifyTable *node = (ModifyTable *) mtstate->ps.plan;
	TupleTableSlot *result = NULL;
	PartitionTupleRouting *proute = mtstate->mt_partition_tuple_routing;
	bool			isConflict = false;
	ItemPointerData conflictTid;

	/*
	 * If the input result relation is a partitioned table, find the leaf
	 * partition to insert the tuple into.
	 */
	if (proute)
	{
		ResultRelInfo *partRelInfo;

		slot = ExecPrepareTupleRouting(mtstate, estate, proute,
									   resultRelInfo, slot,
									   &partRelInfo);
		if (MYS_MODE == compatible_mode && mtstate->mt_partOids) /* BUG#Z5121 */
		{
			if (!CheckPartitionRelationIsexist(partRelInfo, mtstate->mt_partOids))
				ereport(ERROR,
						(errcode(ERRCODE_UNDEFINED_TABLE),
						 errmsg("Found a row not matching the given partition set.")));
		}
		resultRelInfo = partRelInfo;
	}

	/* Begin - BUG#Z1622 */
	/* Find all conflicting columns and delete them,and finally insert new rows. */
	isConflict = true;
	while(isConflict)
	{
		if (!ExecCheckIndexConstraints(resultRelInfo, slot, estate, &conflictTid, NIL))
		{
			(void *)ExecDelete(context, resultRelInfo, &conflictTid, oldtuple,
							  true, false, node->canSetTag, NULL, NULL);
		}
		else
		{
			result = ExecInsert(context, resultRelInfo, slot,
								node->canSetTag, NULL, NULL);
			isConflict = false;
		}
	}
	/* End - BUG#Z1622 */

	return result;
}
/* End - ReqID:SRS-MYSQL-SQL-REPLACE */

/* ----------------------------------------------------------------
 *	   ExecModifyTable
 *
 *		Perform table modifications as required, and return RETURNING results
 *		if needed.
 * ----------------------------------------------------------------
 */
static TupleTableSlot *
ExecModifyTable(PlanState *pstate)
{
	ModifyTableState *node = castNode(ModifyTableState, pstate);
	ModifyTableContext context;
	EState	   *estate = node->ps.state;
	CmdType		operation = node->operation;
	ResultRelInfo *resultRelInfo;
	PlanState  *subplanstate;
	TupleTableSlot *slot;
	TupleTableSlot *planSlot;
	TupleTableSlot *oldSlot;
	ItemPointer tupleid;
	ItemPointerData tuple_ctid;
	HeapTupleData oldtupdata;
	HeapTuple	oldtuple;
	bool		tuplock;

	/* Begin - ReqID:SRS-MYSQL-UPDATE */
	int					resultRelationNum = 0;
	TupleTableSlot	   *tmpslot;
	int					res = 0, tblnum = 0;
	bool				first_fetch_slot;

	if (MYS_MODE == compatible_mode && node->mt_ResultTupleSlots) /* BUG#Z5121 */
	{
		tupleid = NULL;
		resultRelationNum = node->mt_ResultTupleSlots ? list_length(node->mt_ResultTupleSlots) : 1;
		first_fetch_slot = false;
	}
	/* End - ReqID:SRS-MYSQL-UPDATE */

	CHECK_FOR_INTERRUPTS();

	/*
	 * This should NOT get called during EvalPlanQual; we should have passed a
	 * subplan tree to EvalPlanQual, instead.  Use a runtime test not just
	 * Assert because this condition is easy to miss in testing.  (Note:
	 * although ModifyTable should not get executed within an EvalPlanQual
	 * operation, we do have to allow it to be initialized and shut down in
	 * case it is within a CTE subplan.  Hence this test must be here, not in
	 * ExecInitModifyTable.)
	 */
	if (estate->es_epq_active != NULL)
		elog(ERROR, "ModifyTable should not be called during EvalPlanQual");

	/*
	 * If we've already completed processing, don't try to do more.  We need
	 * this test because ExecPostprocessPlan might call us an extra time, and
	 * our subplan's nodes aren't necessarily robust against being called
	 * extra times.
	 */
	if (node->mt_done)
		return NULL;

	/*
	 * On first call, fire BEFORE STATEMENT triggers before proceeding.
	 */
	if (node->fireBSTriggers)
	{
		fireBSTriggers(node);
		node->fireBSTriggers = false;
	}

	/* Preload local variables */
	resultRelInfo = node->resultRelInfo + node->mt_lastResultIndex;
	subplanstate = outerPlanState(node);

	/* Set global context */
	context.mtstate = node;
	context.epqstate = &node->mt_epqstate;
	context.estate = estate;

	/* Begin - ReqID:SRS-SQL-GTT */
	if (RELATION_IS_GLOBAL_TEMP(resultRelInfo->ri_RelationDesc))
	{
		if (operation == CMD_INSERT ||
			(operation == CMD_MERGE && resultRelInfo->ri_notMatchedMergeAction != NIL))
			init_gtt_storage(operation, resultRelInfo);
	}
	/* End - ReqID:SRS-SQL-GTT */

	/*
	 * Fetch rows from subplan, and execute the required table modification
	 * for each row.
	 */
	for (;;)
	{
		/* Begin - ReqID:SRS-MYSQL-UPDATE */
		if (MYS_MODE == compatible_mode && resultRelationNum) /* BUG#Z5121 */
		{
			if (node->mt_partresultRelInfo)
			{
				resultRelInfo = list_nth(node->mt_partresultRelInfo, estate->result_rel_index);
				node->mt_isexits_rootrel = true;
			}
			else
				resultRelInfo = node->resultRelInfo + estate->result_rel_index;
//			node->mt_lastResultOid = InvalidOid;
		}
		/* End - ReqID:SRS-MYSQL-UPDATE */
		/*
		 * Reset the per-output-tuple exprcontext.  This is needed because
		 * triggers expect to use that context as workspace.  It's a bit ugly
		 * to do this below the top level of the plan, however.  We might need
		 * to rethink this later.
		 */
		ResetPerTupleExprContext(estate);

		/*
		 * Reset per-tuple memory context used for processing on conflict and
		 * returning clauses, to free any expression evaluation storage
		 * allocated in the previous cycle.
		 */
		if (pstate->ps_ExprContext)
			ResetExprContext(pstate->ps_ExprContext);

		/* Begin - SRS-MYSQL-UPDATE */
		if (MYS_MODE == compatible_mode && resultRelationNum) /* BUG#Z5121 */
		{
			planSlot = FetchPlanSlot(subplanstate, node, &res, &tblnum, &resultRelInfo,
												&tmpslot, estate, &first_fetch_slot);

			if(estate->result_rel_index && !node->mt_ProjInfos[estate->result_rel_index]->pi_fetch_slot)
			{
				if (estate->result_rel_index == resultRelationNum - 1)
				{
					estate->result_rel_index = 0;
					node->mt_lastResultOid = InvalidOid;
				}
				else
					estate->result_rel_index++;
				continue;
			}
		}
		else
			planSlot = ExecProcNode(subplanstate);
		/* End - ReqID:SRS-MYSQL-UPDATE */

		/* No more tuples to process? */
		if (TupIsNull(planSlot))
			break;

		/*
		 * When there are multiple result relations, each tuple contains a
		 * junk column that gives the OID of the rel from which it came.
		 * Extract it and select the correct result relation.
		 */
		if (AttributeNumberIsValid(node->mt_resultOidAttno))
		{
			Datum		datum;
			bool		isNull;
			Oid			resultoid;

			datum = ExecGetJunkAttribute(planSlot, node->mt_resultOidAttno,
										 &isNull);
			if (isNull)
			{
				/*
				 * For commands other than MERGE, any tuples having InvalidOid
				 * for tableoid are errors.  For MERGE, we may need to handle
				 * them as WHEN NOT MATCHED clauses if any, so do that.
				 *
				 * Note that we use the node's toplevel resultRelInfo, not any
				 * specific partition's.
				 */
				if (operation == CMD_MERGE)
				{
					EvalPlanQualSetSlot(&node->mt_epqstate, planSlot);

					context.planSlot = planSlot;
					context.lockmode = 0;

					ExecMerge(&context, node->resultRelInfo, NULL, node->canSetTag);
					continue;	/* no RETURNING support yet */
				}

				/* Begin - ReqID:SRS-MYSQL-UPDATE */
				if (MYS_MODE == compatible_mode) /* BUG#Z5121 */
					continue;
				else
					elog(ERROR, "tableoid is NULL");
				/* End - ReqID:SRS-MYSQL-UPDATE */
			}
			resultoid = DatumGetObjectId(datum);

			/* If it's not the same as last time, we need to locate the rel */
			if (resultoid != node->mt_lastResultOid)
			{
				/* Begin - ReqID:SRS-MYSQL-UPDATE */
				if (MYS_MODE == compatible_mode && !resultRelationNum) /* BUG#Z5121 */
				{
					/* If the matching partition is not the specified partition,
					 * no update is performed,and this cycle is skipped.
					 */
					bool		flag = false;
					for (int ndx = 0; ndx < node->mt_nrels; ndx++)
					{
						ResultRelInfo *rInfo = node->resultRelInfo + ndx;

						if (RelationGetRelid(rInfo->ri_RelationDesc) == resultoid)
						{
							flag = true;
							break;
						}
					}
					if (!flag)
						continue;
				}
				/* End - ReqID:SRS-MYSQL-UPDATE */
				resultRelInfo = ExecLookupResultRelByOid(node, resultoid,
														 false, true);
			}
		}

		/*
		 * If resultRelInfo->ri_usesFdwDirectModify is true, all we need to do
		 * here is compute the RETURNING expressions.
		 */
		if (resultRelInfo->ri_usesFdwDirectModify)
		{
			Assert(resultRelInfo->ri_projectReturning);

			/*
			 * A scan slot containing the data that was actually inserted,
			 * updated or deleted has already been made available to
			 * ExecProcessReturning by IterateDirectModify, so no need to
			 * provide it here.
			 */
			slot = ExecProcessReturning(resultRelInfo, NULL, planSlot);

			return slot;
		}

		EvalPlanQualSetSlot(&node->mt_epqstate, planSlot);
		slot = planSlot;

		tupleid = NULL;
		oldtuple = NULL;

		/*
		 * For UPDATE/DELETE/MERGE, fetch the row identity info for the tuple
		 * to be updated/deleted/merged.  For a heap relation, that's a TID;
		 * otherwise we may have a wholerow junk attr that carries the old
		 * tuple in toto.  Keep this in step with the part of
		 * ExecInitModifyTable that sets up ri_RowIdAttNo.
		 */
		if (operation == CMD_UPDATE || operation == CMD_DELETE ||
			operation == CMD_MERGE)
		{
			char		relkind;
			Datum		datum;
			bool		isNull;

			relkind = resultRelInfo->ri_RelationDesc->rd_rel->relkind;
			if (relkind == RELKIND_RELATION ||
				relkind == RELKIND_MATVIEW ||
				relkind == RELKIND_PARTITIONED_TABLE)
			{

				/* ri_RowIdAttNo refers to a ctid attribute */
				Assert(AttributeNumberIsValid(resultRelInfo->ri_RowIdAttNo));
				datum = ExecGetJunkAttribute(slot,
											 resultRelInfo->ri_RowIdAttNo,
											 &isNull);

				/*
				 * For commands other than MERGE, any tuples having a null row
				 * identifier are errors.  For MERGE, we may need to handle
				 * them as WHEN NOT MATCHED clauses if any, so do that.
				 *
				 * Note that we use the node's toplevel resultRelInfo, not any
				 * specific partition's.
				 */
				if (isNull)
				{
					if (operation == CMD_MERGE)
					{
						EvalPlanQualSetSlot(&node->mt_epqstate, planSlot);

						context.planSlot = planSlot;
						context.lockmode = 0;

						ExecMerge(&context, node->resultRelInfo, NULL, node->canSetTag);
						continue;	/* no RETURNING support yet */
					}

					/* Begin - ReqID:SRS-MYSQL-UPDATE */
					if (MYS_MODE == compatible_mode && resultRelationNum) /* BUG#Z5121 */
					{
						if (estate->result_rel_index == resultRelationNum - 1)
							estate->result_rel_index = 0;
						else
							estate->result_rel_index++;

						continue;
					}
					/* End - ReqID:SRS-MYSQL-UPDATE */
					if (MYS_MODE == compatible_mode) /* BUG#Z5121 */
						continue;
					else
						elog(ERROR, "ctid is NULL");
				}

				tupleid = (ItemPointer) DatumGetPointer(datum);
				tuple_ctid = *tupleid;	/* be sure we don't free ctid!! */
				tupleid = &tuple_ctid;
			}

			/*
			 * Use the wholerow attribute, when available, to reconstruct the
			 * old relation tuple.  The old tuple serves one or both of two
			 * purposes: 1) it serves as the OLD tuple for row triggers, 2) it
			 * provides values for any unchanged columns for the NEW tuple of
			 * an UPDATE, because the subplan does not produce all the columns
			 * of the target table.
			 *
			 * Note that the wholerow attribute does not carry system columns,
			 * so foreign table triggers miss seeing those, except that we
			 * know enough here to set t_tableOid.  Quite separately from
			 * this, the FDW may fetch its own junk attrs to identify the row.
			 *
			 * Other relevant relkinds, currently limited to views having
			 * INSTEAD OF triggers, always have a wholerow attribute.
			 */
			else if (AttributeNumberIsValid(resultRelInfo->ri_RowIdAttNo))
			{
				datum = ExecGetJunkAttribute(slot,
											 resultRelInfo->ri_RowIdAttNo,
											 &isNull);
				/* shouldn't ever get a null result... */
				if (isNull)
					elog(ERROR, "wholerow is NULL");

				oldtupdata.t_data = DatumGetHeapTupleHeader(datum);
				oldtupdata.t_len =
					HeapTupleHeaderGetDatumLength(oldtupdata.t_data);
				ItemPointerSetInvalid(&(oldtupdata.t_self));
				/* Historically, view triggers see invalid t_tableOid. */
				oldtupdata.t_tableOid =
					(relkind == RELKIND_VIEW) ? InvalidOid :
					RelationGetRelid(resultRelInfo->ri_RelationDesc);

				oldtuple = &oldtupdata;
			}
			else
			{
				/* Only foreign tables are allowed to omit a row-ID attr */
				Assert(relkind == RELKIND_FOREIGN_TABLE);
			}
		}

		/* complete context setup */
		context.planSlot = planSlot;
		context.lockmode = 0;

		switch (operation)
		{
			case CMD_INSERT:
				/* Initialize projection info if first time for this table */
				if (unlikely(!resultRelInfo->ri_projectNewInfoValid))
					ExecInitInsertProjection(node, resultRelInfo);
				slot = ExecGetInsertNewTuple(resultRelInfo, planSlot);
				/* Begin - ReqID:SRS-MYSQL-SQL-REPLACE */
				if (node->isReplace && MYS_MODE == compatible_mode) /* BUG#Z5121 */
					slot = ExecReplace(&context, resultRelInfo, slot,
									  node->canSetTag, NULL, NULL, oldtuple);
				else
					slot = ExecInsert(&context, resultRelInfo, slot,
									  node->canSetTag, NULL, NULL);
				/* End - ReqID:SRS-MYSQL-SQL-REPLACE */
				break;
			case CMD_UPDATE:
				{
					tuplock = false;
					/* Initialize projection info if first time for this table */
					/* Begin - ReqID:SRS-MYSQL-UPDATE */
					if (MYS_MODE == compatible_mode && resultRelationNum) /* BUG#Z5121 */
					{
						if(!estate->result_rel_index)
						{
							if (unlikely(!resultRelInfo->ri_projectNewInfoValid))
								ExecInitUpdateProjection(node, resultRelInfo);
						}
						else
						{
							List	   *targetlists = ((ModifyTable*)(node->ps.plan))->targetlists;
							List	   *targetlist = list_nth(targetlists, estate->result_rel_index);

							ExecInitMultilUpdateProjection(node, targetlist, resultRelInfo, estate->result_rel_index);
						}
					}
					else
					{
						if (unlikely(!resultRelInfo->ri_projectNewInfoValid))
							ExecInitUpdateProjection(node, resultRelInfo);
					}
					/* End - ReqID:SRS-MYSQL-UPDATE */

					/*
					 * Make the new tuple by combining plan's output tuple with
					 * the old tuple being updated.
					 */
					oldSlot = resultRelInfo->ri_oldTupleSlot;
					if (oldtuple != NULL)
					{
						Assert(!resultRelInfo->ri_needLockTagTuple);
						/* Use the wholerow junk attr as the old tuple. */
						ExecForceStoreHeapTuple(oldtuple, oldSlot, false);
					}
					else
					{
						/* Fetch the most recent version of old tuple. */
						Relation	relation = resultRelInfo->ri_RelationDesc;

						if (resultRelInfo->ri_needLockTagTuple)
						{
							LockTuple(relation, tupleid, InplaceUpdateTupleLock);
							tuplock = true;
						}

						if (!table_tuple_fetch_row_version(relation, tupleid,
														   SnapshotAny,
														   oldSlot))
							elog(ERROR, "failed to fetch tuple being updated");
					}
					slot = internalGetUpdateNewTuple(resultRelInfo, planSlot,
													 oldSlot, NULL);
					context.GetUpdateNewTuple = internalGetUpdateNewTuple;
					context.relaction = NULL;

					slot = ExecUpdate(&context, resultRelInfo, tupleid, oldtuple,
									  slot, node->canSetTag);
					
					if (tuplock)
					UnlockTuple(resultRelInfo->ri_RelationDesc, tupleid,
								InplaceUpdateTupleLock);
				}
				break;
			case CMD_DELETE:
				slot = ExecDelete(&context, resultRelInfo, tupleid, oldtuple,
								  true, false, node->canSetTag, NULL, NULL);
				break;

			case CMD_MERGE:
				slot = ExecMerge(&context, resultRelInfo, tupleid, node->canSetTag);
				break;

			case CMD_MULTI_INSERT:
				slot = ExecMultiInsert(&context, resultRelInfo, slot,
									  node->canSetTag);
				break;
			default:
				elog(ERROR, "unknown operation");
				break;
		}

		/* Begin - ReqID:SRS-MYSQL-UPDATE */
		if (MYS_MODE == compatible_mode && resultRelationNum) /* BUG#Z5121 */
		{
			if (estate->result_rel_index == resultRelationNum - 1)
				estate->result_rel_index = 0;
			else
				estate->result_rel_index++;
		}
		/* End - ReqID:SRS-MYSQL-UPDATE */

		/*
		 * If we got a RETURNING result, return it to caller.  We'll continue
		 * the work on next call.
		 */
		if (slot)
			return slot;
	}

	/*
	 * Insert remaining tuples for batch insert.
	 */
	if (estate->es_insert_pending_result_relations != NIL)
		ExecPendingInserts(estate);

	/*
	 * We're done, but fire AFTER STATEMENT triggers before exiting.
	 */
	fireASTriggers(node);

	node->mt_done = true;

	return NULL;
}


/* Begin - ReqID:SRS-MYSQL-UPDATE */
/*
 * InitMultipleModify - Construct a projInfo node based on the target columns.
 */
static void
InitMultipleModify(ModifyTableState* node, int32 resultRelationNum)
{
	ListCell	   *l1 = NULL;
	List		   *targetlists = ((ModifyTable*)(node->ps.plan))->targetlists;
	EState		   *estate = node->ps.state;
	int				i = 0;

	if (resultRelationNum == 1)
		return;

	node->mt_ProjInfos = (ProjectionInfo**)palloc0((resultRelationNum + 1) * sizeof(ProjectionInfo*));

	foreach (l1, targetlists)
	{
		TupleTableSlot *slot;
		TupleDesc tupDesc;
		ProjectionInfo* projInfo = NULL;
		List   *tmplist = NIL;

		List	*targetList = (List*)lfirst(l1);

		tmplist = list_concat(tmplist, estate->es_tupleTable);

		slot = ExecMutilAllocTableSlot(&tmplist,
												   &TTSOpsVirtual);

		tupDesc = ExecTypeFromTL(targetList, false);

		ExecSetSlotDescriptor(slot, tupDesc);

		node->mt_ResultTupleSlots = lappend(node->mt_ResultTupleSlots, slot);

		projInfo = ExecBuildProjectionInfo(targetList, node->ps.ps_ExprContext, slot, (PlanState*)node, NULL);


		/* Use the targetlist of the subplan to build mt_ProjInfos. It will be use for FetchMultipleModifySlot. */
		node->mt_ProjInfos[i] = projInfo;
		i++;
	}
}
/* End - ReqID:SRS-MYSQL-UPDATE */


/*
 * ExecLookupResultRelByOid
 * 		If the table with given OID is among the result relations to be
 * 		updated by the given ModifyTable node, return its ResultRelInfo.
 *
 * If not found, return NULL if missing_ok, else raise error.
 *
 * If update_cache is true, then upon successful lookup, update the node's
 * one-element cache.  ONLY ExecModifyTable may pass true for this.
 */
ResultRelInfo *
ExecLookupResultRelByOid(ModifyTableState *node, Oid resultoid,
						 bool missing_ok, bool update_cache)
{
	if (node->mt_resultOidHash)
	{
		/* Use the pre-built hash table to locate the rel */
		MTTargetRelLookup *mtlookup;

		mtlookup = (MTTargetRelLookup *)
			hash_search(node->mt_resultOidHash, &resultoid, HASH_FIND, NULL);
		if (mtlookup)
		{
			if (update_cache)
			{
				node->mt_lastResultOid = resultoid;
				node->mt_lastResultIndex = mtlookup->relationIndex;
			}
			return node->resultRelInfo + mtlookup->relationIndex;
		}
	}
	else
	{
		/* With few target rels, just search the ResultRelInfo array */
		for (int ndx = 0; ndx < node->mt_nrels; ndx++)
		{
			ResultRelInfo *rInfo = node->resultRelInfo + ndx;

			if (RelationGetRelid(rInfo->ri_RelationDesc) == resultoid)
			{
				if (update_cache)
				{
					node->mt_lastResultOid = resultoid;
					node->mt_lastResultIndex = ndx;
				}
				return rInfo;
			}
		}
	}

	if (!missing_ok)
		elog(ERROR, "incorrect result relation OID %u", resultoid);
	return NULL;
}

/*
 * ExecInitCheckpartitionResultInfo
 * Find the corresponding rootResultRelInfo for the partition.
 */
static ResultRelInfo *
ExecInitCheckpartitionResultInfo(ModifyTableState *mtstate, ModifyTable *node, Index resultRelation)
{
	ListCell	   *lc = NULL;
	ListCell	   *lc1 = NULL;
	int				i = 0;

	foreach(lc, node->partresultRelations)
	{
		List	   *partlist = (List *)lfirst(lc);
		foreach(lc1, partlist)
		{
			Index	relidx = lfirst_int(lc1);
			if (relidx == resultRelation)
			{
				return mtstate->mt_rootResultRelInfos + i;
			}
		}
		i++;
	}
	return NULL;
}


/*
 * CheckSamerootResultRelInfo
 * Check if the partitions have the same rootResultRelInfo.
 */
static bool
CheckSamerootResultRelInfo(List *partresultRelInfo, ResultRelInfo *resultRelInfo)
{
	ListCell   *lc = NULL;
	if (!partresultRelInfo)
		return true;
	else
	{
		foreach(lc, partresultRelInfo)
		{
			ResultRelInfo	*relinfo = lfirst(lc);
			if (relinfo->ri_RootResultRelInfo == resultRelInfo->ri_RootResultRelInfo)
				return false;
		}
	}
	return true;
}

/* Begin - BUG#Z4241 */
/*
 * CheckSamepartResultRelInfo
 * Check if the partitions is duplicated.
 */
static void
CheckSamepartionResultRelInfo(List *partresultRelInfo, ResultRelInfo *resultRelInfo)
{
	ListCell   *lc = NULL;
	if (!partresultRelInfo)
		return;
	else
	{
		foreach(lc, partresultRelInfo)
		{
			ResultRelInfo	*relinfo = lfirst(lc);
			if (relinfo->ri_RelationDesc == resultRelInfo->ri_RelationDesc)
				ereport(ERROR,
						(errcode(ERRCODE_DUPLICATE_TABLE),
						 errmsg("The partition relation %s is duplicated.", RelationGetRelationName(resultRelInfo->ri_RelationDesc))));
		}
	}
	return;
}
/* End - BUG#Z4241 */


/* ----------------------------------------------------------------
 *		ExecInitModifyTable
 * ----------------------------------------------------------------
 */
ModifyTableState *
ExecInitModifyTable(ModifyTable *node, EState *estate, int eflags)
{
	ModifyTableState *mtstate;
	Plan	   *subplan = outerPlan(node);
	CmdType		operation = node->operation;
	int			nrels = list_length(node->resultRelations);
	ResultRelInfo *resultRelInfo;
	List	   *arowmarks;
	ListCell   *l;
	int			i;
	Relation	rel;

	/* Begin - ReqID:SRS-MYSQL-SQL-INSERT */
	UpsertState		   *upsertState = NULL;
	ResultRelInfo	   *result_rel_info = NULL;
	/* End - ReqID:SRS-MYSQL-SQL-INSERT */

	/* check for unsupported flags */
	Assert(!(eflags & (EXEC_FLAG_BACKWARD | EXEC_FLAG_MARK)));

	/*
	 * create state structure
	 */
	mtstate = makeNode(ModifyTableState);
	mtstate->ps.plan = (Plan *) node;
	mtstate->ps.state = estate;
	mtstate->ps.ExecProcNode = ExecModifyTable;

	mtstate->operation = operation;
	mtstate->canSetTag = node->canSetTag;
	mtstate->mt_done = false;

	mtstate->mt_nrels = nrels;
	mtstate->resultRelInfo = (ResultRelInfo *)
		palloc(nrels * sizeof(ResultRelInfo));

	mtstate->mt_merge_inserted = 0;
	mtstate->mt_merge_updated = 0;
	mtstate->mt_merge_deleted = 0;
	/* Begin - SRS-SQL-MULTITABLEINSERT */
	mtstate->mt_multi_inserted = 0;
	/* End - SRS-SQL-MULTITABLEINSERT */

	/* Begin - ReqID:SRS-MYSQL-SQL-INSERT */
	upsertState = (UpsertState*)palloc0(sizeof(UpsertState));
	upsertState->us_action = node->upsertAction;
	upsertState->us_existing = NULL;
	upsertState->us_excludedtlist = NIL;
	upsertState->us_updateproj = NULL;
	mtstate->mt_upsert = upsertState;
	/* End - ReqID:SRS-MYSQL-SQL-INSERT */

	/* Begin - ReqID:SRS-MYSQL-SQL-REPLACE */
	mtstate->isReplace = node->isReplace;
	mtstate->mt_partOids = node->partOids;
	/* End - ReqID:SRS-MYSQL-SQL-REPLACE */
	/*----------
	 * Resolve the target relation. This is the same as:
	 *
	 * - the relation for which we will fire FOR STATEMENT triggers,
	 * - the relation into whose tuple format all captured transition tuples
	 *   must be converted, and
	 * - the root partitioned table used for tuple routing.
	 *
	 * If it's a partitioned or inherited table, the root partition or
	 * appendrel RTE doesn't appear elsewhere in the plan and its RT index is
	 * given explicitly in node->rootRelation.  Otherwise, the target relation
	 * is the sole relation in the node->resultRelations list.
	 *----------
	 */
	/* Begin - ReqID:SRS-MYSQL-UPDATE */
	if (MYS_MODE == compatible_mode && node->rootRelations) /* BUG#Z5121 */
	{
		ListCell		   *lc = NULL;
		ResultRelInfo	   *rootresultRelInfo = NULL;
		int					nrootrel = list_length(node->rootRelations);
		mtstate->mt_rootResultRelInfos= (ResultRelInfo *)
								palloc(nrootrel *sizeof(ResultRelInfo));
		rootresultRelInfo = mtstate->mt_rootResultRelInfos;
		foreach(lc, node->rootRelations)
		{
			int		rootrelation = lfirst_int(lc);
			if (rootrelation > 0)
			{
				ExecInitResultRelation(estate, rootresultRelInfo,
									   rootrelation);
			}
			rootresultRelInfo++;
		}
		mtstate->rootResultRelInfo = mtstate->mt_rootResultRelInfos + 0;
	}
	/* End - ReqID:SRS-MYSQL-UPDATE */
	else
	{
		if (node->rootRelation > 0)
		{
			mtstate->rootResultRelInfo = makeNode(ResultRelInfo);
			ExecInitResultRelation(estate, mtstate->rootResultRelInfo,
								   node->rootRelation);
		}
		else
		{
			/* Begin - ReqID:SRS-MYSQL-UPDATE */
			if(!(MYS_MODE == compatible_mode && node->parseresultrelations &&
				list_length(node->parseresultrelations) > 1)) /* BUG#Z5121 */
				Assert(operation == CMD_MULTI_INSERT || /* SRS-SQL-MULTITABLEINSERT */
				list_length(node->resultRelations) == 1);
			/* End - ReqID:SRS-MYSQL-UPDATE */
			mtstate->rootResultRelInfo = mtstate->resultRelInfo;
			ExecInitResultRelation(estate, mtstate->resultRelInfo,
								   linitial_int(node->resultRelations));
		}
	}

	/* set up epqstate with dummy subplan data for the moment */
	EvalPlanQualInitExt(&mtstate->mt_epqstate, estate, NULL, NIL,
						node->epqParam, node->resultRelations);
	mtstate->fireBSTriggers = true;

	/*
	 * Open all the result relations and initialize the ResultRelInfo structs.
	 * (But root relation was initialized above, if it's part of the array.)
	 * We must do this before initializing the subplan, because direct-modify
	 * FDWs expect their ResultRelInfos to be available.
	 */
	resultRelInfo = mtstate->resultRelInfo;
	i = 0;
	foreach(l, node->resultRelations)
	{
		Index		resultRelation = lfirst_int(l);

		if (resultRelInfo != mtstate->rootResultRelInfo)
		{
			ExecInitResultRelation(estate, resultRelInfo, resultRelation);

			/*
			 * For child result relations, store the root result relation
			 * pointer.  We do so for the convenience of places that want to
			 * look at the query's original target relation but don't have the
			 * mtstate handy.
			 */
			/* Begin - ReqID:SRS-MYSQL-UPDATE */
			if (MYS_MODE == compatible_mode && node->rootRelations) /* BUG#Z5121 */
			{
				/* check resultRelation is partition relation. */
				resultRelInfo->ri_RootResultRelInfo = ExecInitCheckpartitionResultInfo(mtstate, node, resultRelation);
				if (!resultRelInfo->ri_RootResultRelInfo)
					mtstate->mt_partresultRelInfo = lappend(mtstate->mt_partresultRelInfo, resultRelInfo);
				else
				{
					if (CheckSamerootResultRelInfo(mtstate->mt_partresultRelInfo, resultRelInfo))
					{
						/* Begin - BUG#Z4241 */
						CheckSamepartionResultRelInfo(mtstate->mt_partresultRelInfo, resultRelInfo);
						/* End - BUG#Z4241 */
						mtstate->mt_partresultRelInfo = lappend(mtstate->mt_partresultRelInfo, resultRelInfo);
					}
				}
			}
			else
			{
				if (!get_rel_relispartition(resultRelInfo->ri_RelationDesc->rd_id) && MYS_MODE == compatible_mode) /* BUG#Z5121 */
				{
					resultRelInfo->ri_RootResultRelInfo = NULL;
				}
				/* Begin - SRS-SQL-MULTITABLEINSERT */
				else if (operation == CMD_MULTI_INSERT)
					resultRelInfo->ri_RootResultRelInfo = NULL;
				/* End - SRS-SQL-MULTITABLEINSERT */
				else
					resultRelInfo->ri_RootResultRelInfo = mtstate->rootResultRelInfo;
			}
			/* End - ReqID:SRS-MYSQL-UPDATE */
		}

		/* Initialize the usesFdwDirectModify flag */
		resultRelInfo->ri_usesFdwDirectModify = bms_is_member(i,
															  node->fdwDirectModifyPlans);

		/*
		 * Verify result relation is a valid target for the current operation
		 */
		CheckValidResultRelNew(resultRelInfo, operation,
							   node->onConflictAction);

		resultRelInfo++;
		i++;
	}

	/* Begin - SRS-SQL-MULTITABLEINSERT */
	/*
	 * Build state for collecting transition tuples.  This requires having a
	 * valid trigger query context, so skip it in explain-only mode.
	 *
	 * multi insert should ExecInitResultRelation first
	 */
	if (!(eflags & EXEC_FLAG_EXPLAIN_ONLY))
		ExecSetupTransitionCaptureState(mtstate, estate);
	/* End - SRS-SQL-MULTITABLEINSERT */

	/*
	 * Now we may initialize the subplan.
	 */
	outerPlanState(mtstate) = ExecInitNode(subplan, estate, eflags);

	/*
	 * Do additional per-result-relation initialization.
	 */
	for (i = 0; i < nrels; i++)
	{
		resultRelInfo = &mtstate->resultRelInfo[i];

		/* Let FDWs init themselves for foreign-table result rels */
		if (!resultRelInfo->ri_usesFdwDirectModify &&
			resultRelInfo->ri_FdwRoutine != NULL &&
			resultRelInfo->ri_FdwRoutine->BeginForeignModify != NULL)
		{
			List	   *fdw_private = (List *) list_nth(node->fdwPrivLists, i);

			resultRelInfo->ri_FdwRoutine->BeginForeignModify(mtstate,
															 resultRelInfo,
															 fdw_private,
															 i,
															 eflags);
		}

		/*
		 * For UPDATE/DELETE/MERGE, find the appropriate junk attr now, either
		 * a 'ctid' or 'wholerow' attribute depending on relkind.  For foreign
		 * tables, the FDW might have created additional junk attr(s), but
		 * those are no concern of ours.
		 */
		if (operation == CMD_UPDATE || operation == CMD_DELETE ||
			operation == CMD_MERGE)
		{
			char		relkind;
			List	   *multilist;	/* ReqID:SRS-MYSQL-UPDATE */

			relkind = resultRelInfo->ri_RelationDesc->rd_rel->relkind;
			if (relkind == RELKIND_RELATION ||
				relkind == RELKIND_MATVIEW ||
				relkind == RELKIND_PARTITIONED_TABLE)
			{
				/* Begin - ReqID:SRS-MYSQL-UPDATE */
				if (node->targetlists && MYS_MODE == compatible_mode) /* BUG#Z5121 */
				{
					if (node->rootRelations &&
						resultRelInfo->ri_RootResultRelInfo)
					{
						ListCell   *lc = NULL;
						int			j = 0;
						foreach(lc, node->parseresultrelations)
						{
							Index	resultrelation = lfirst_int(lc);
							if (resultrelation == resultRelInfo->ri_RootResultRelInfo->ri_RangeTableIndex)
								break;
							j++;
						}
						multilist = list_nth(node->targetlists, j);
					}
					else
						multilist = list_nth(node->targetlists, i);
				}
				else
					multilist = subplan->targetlist;
				resultRelInfo->ri_RowIdAttNo =
					ExecFindJunkAttributeInTlist(multilist, "ctid");
				/* End - ReqID:SRS-MYSQL-UPDATE */

				if (!AttributeNumberIsValid(resultRelInfo->ri_RowIdAttNo))
					elog(ERROR, "could not find junk ctid column");
			}
			else if (relkind == RELKIND_FOREIGN_TABLE)
			{
				/*
				 * We don't support MERGE with foreign tables for now.  (It's
				 * problematic because the implementation uses CTID.)
				 */
				Assert(operation != CMD_MERGE);

				/*
				 * When there is a row-level trigger, there should be a
				 * wholerow attribute.  We also require it to be present in
				 * UPDATE and MERGE, so we can get the values of unchanged
				 * columns.
				 */
				resultRelInfo->ri_RowIdAttNo =
					ExecFindJunkAttributeInTlist(subplan->targetlist,
												 "wholerow");
				if ((mtstate->operation == CMD_UPDATE || mtstate->operation == CMD_MERGE) &&
					!AttributeNumberIsValid(resultRelInfo->ri_RowIdAttNo))
					elog(ERROR, "could not find junk wholerow column");
			}
			else
			{
				/* No support for MERGE */
				Assert(operation != CMD_MERGE);
				/* Other valid target relkinds must provide wholerow */
				resultRelInfo->ri_RowIdAttNo =
					ExecFindJunkAttributeInTlist(subplan->targetlist,
												 "wholerow");
				if (!AttributeNumberIsValid(resultRelInfo->ri_RowIdAttNo))
					elog(ERROR, "could not find junk wholerow column");
			}
		}

		/*
		 * For INSERT and UPDATE, prepare to evaluate any generated columns.
		 * (This is probably not necessary any longer, but we'll refrain from
		 * changing it in back branches, in case extension code expects it.)
		 */
		if (operation == CMD_INSERT ||
			operation == CMD_UPDATE ||
			operation == CMD_MULTI_INSERT)
			ExecInitStoredGenerated(resultRelInfo, estate, operation);
	}

	/*
	 * If this is an inherited update/delete/merge, there will be a junk
	 * attribute named "tableoid" present in the subplan's targetlist.  It
	 * will be used to identify the result relation for a given tuple to be
	 * updated/deleted/merged.
	 */
	mtstate->mt_resultOidAttno =
		ExecFindJunkAttributeInTlist(subplan->targetlist, "tableoid");

	/* Begin - ReqID:SRS-MYSQL-UPDATE */
	if (!(MYS_MODE == compatible_mode && node->parseresultrelations &&
		list_length(node->parseresultrelations) > 1)) /* BUG#Z5121 */
		Assert(AttributeNumberIsValid(mtstate->mt_resultOidAttno) ||
			nrels == 1 || operation == CMD_MULTI_INSERT); /* SRS-SQL-MULTITABLEINSERT */
	/* End - ReqID:SRS-MYSQL-UPDATE */

	mtstate->mt_lastResultOid = InvalidOid; /* force lookup at first tuple */
	mtstate->mt_lastResultIndex = 0;	/* must be zero if no such attr */

	/* Get the root target relation */
	rel = mtstate->rootResultRelInfo->ri_RelationDesc;

	/*
	 * Build state for tuple routing if it's a partitioned INSERT.  An UPDATE
	 * or MERGE might need this too, but only if it actually moves tuples
	 * between partitions; in that case setup is done by
	 * ExecCrossPartitionUpdate.
	 */
	if (rel->rd_rel->relkind == RELKIND_PARTITIONED_TABLE &&
		operation == CMD_INSERT)
		mtstate->mt_partition_tuple_routing =
			ExecSetupPartitionTupleRouting(estate, rel);

	/*
	 * Initialize any WITH CHECK OPTION constraints if needed.
	 */
	resultRelInfo = mtstate->resultRelInfo;
	foreach(l, node->withCheckOptionLists)
	{
		List	   *wcoList = (List *) lfirst(l);
		List	   *wcoExprs = NIL;
		ListCell   *ll;

		foreach(ll, wcoList)
		{
			WithCheckOption *wco = (WithCheckOption *) lfirst(ll);
			ExprState  *wcoExpr = ExecInitQual((List *) wco->qual,
											   &mtstate->ps);

			wcoExprs = lappend(wcoExprs, wcoExpr);
		}

		resultRelInfo->ri_WithCheckOptions = wcoList;
		resultRelInfo->ri_WithCheckOptionExprs = wcoExprs;
		resultRelInfo++;
	}

	/*
	 * Initialize RETURNING projections if needed.
	 */
	if (node->returningLists)
	{
		TupleTableSlot *slot;
		ExprContext *econtext;

		/*
		 * Initialize result tuple slot and assign its rowtype using the first
		 * RETURNING list.  We assume the rest will look the same.
		 */
		mtstate->ps.plan->targetlist = (List *) linitial(node->returningLists);

		/* Set up a slot for the output of the RETURNING projection(s) */
		ExecInitResultTupleSlotTL(&mtstate->ps, &TTSOpsVirtual);
		slot = mtstate->ps.ps_ResultTupleSlot;

		/* Need an econtext too */
		if (mtstate->ps.ps_ExprContext == NULL)
			ExecAssignExprContext(estate, &mtstate->ps);
		econtext = mtstate->ps.ps_ExprContext;

		/*
		 * Build a projection for each result rel.
		 */
		resultRelInfo = mtstate->resultRelInfo;
		foreach(l, node->returningLists)
		{
			List	   *rlist = (List *) lfirst(l);

			resultRelInfo->ri_returningList = rlist;
			resultRelInfo->ri_projectReturning =
				ExecBuildProjectionInfo(rlist, econtext, slot, &mtstate->ps,
										resultRelInfo->ri_RelationDesc->rd_att);
			resultRelInfo++;
		}
	}
	else
	{
		/*
		 * We still must construct a dummy result tuple type, because InitPlan
		 * expects one (maybe should change that?).
		 */
		mtstate->ps.plan->targetlist = NIL;
		ExecInitResultTypeTL(&mtstate->ps);

		mtstate->ps.ps_ExprContext = NULL;
	}

	/* Set the list of arbiter indexes if needed for ON CONFLICT */
	resultRelInfo = mtstate->resultRelInfo;
	if (node->onConflictAction != ONCONFLICT_NONE)
	{
		/* insert may only have one relation, inheritance is not expanded */
		Assert(nrels == 1);
		resultRelInfo->ri_onConflictArbiterIndexes = node->arbiterIndexes;
	}

	/*
	 * If needed, Initialize target list, projection and qual for ON CONFLICT
	 * DO UPDATE.
	 */
	if (node->onConflictAction == ONCONFLICT_UPDATE)
	{
		OnConflictSetState *onconfl = makeNode(OnConflictSetState);
		ExprContext *econtext;
		TupleDesc	relationDesc;

		/* already exists if created by RETURNING processing above */
		if (mtstate->ps.ps_ExprContext == NULL)
			ExecAssignExprContext(estate, &mtstate->ps);

		econtext = mtstate->ps.ps_ExprContext;
		relationDesc = resultRelInfo->ri_RelationDesc->rd_att;

		/* create state for DO UPDATE SET operation */
		resultRelInfo->ri_onConflict = onconfl;

		/* initialize slot for the existing tuple */
		onconfl->oc_Existing =
			table_slot_create(resultRelInfo->ri_RelationDesc,
							  &mtstate->ps.state->es_tupleTable);

		/*
		 * Create the tuple slot for the UPDATE SET projection. We want a slot
		 * of the table's type here, because the slot will be used to insert
		 * into the table, and for RETURNING processing - which may access
		 * system attributes.
		 */
		onconfl->oc_ProjSlot =
			table_slot_create(resultRelInfo->ri_RelationDesc,
							  &mtstate->ps.state->es_tupleTable);

		/* build UPDATE SET projection state */
		onconfl->oc_ProjInfo =
			ExecBuildUpdateProjection(node->onConflictSet,
									  true,
									  node->onConflictCols,
									  relationDesc,
									  econtext,
									  onconfl->oc_ProjSlot,
									  &mtstate->ps);

		/* initialize state to evaluate the WHERE clause, if any */
		if (node->onConflictWhere)
		{
			ExprState  *qualexpr;

			qualexpr = ExecInitQual((List *) node->onConflictWhere,
									&mtstate->ps);
			onconfl->oc_WhereClause = qualexpr;
		}
	}

	/*
	 * If needed, Initialize target list, projection for DUPLICATE KEY UPDATE
	 */
	/* Begin - ReqID:SRS-MYSQL-SQL-INSERT */
	result_rel_info = mtstate->resultRelInfo;
	if ((node->upsertAction == UPSERT_UPDATE || node->isReplace) && MYS_MODE == compatible_mode) /* BUG#Z5121 */
	{
		ExprContext	   *econtext = NULL;
		TupleDesc		tupDesc = NULL;

		resultRelInfo->ri_RelationDesc->rd_onduplicate = true;
		ExecOpenIndices(result_rel_info, true);
		/* already exists if created by RETURNING processing above */
		if (mtstate->ps.ps_ExprContext == NULL)
			ExecAssignExprContext(estate, &mtstate->ps);

		econtext = mtstate->ps.ps_ExprContext;
		tupDesc = result_rel_info->ri_RelationDesc->rd_att;

		/* initialize slot for the existing tuple */
		upsertState->us_existing =
			table_slot_create(resultRelInfo->ri_RelationDesc,
							  &mtstate->ps.state->es_tupleTable);

		upsertState->us_excludedtlist = node->upexclRelTlist;
		/* create target slot for UPDATE SET projection */
		upsertState->us_updateproj =
			table_slot_create(resultRelInfo->ri_RelationDesc,
							  &mtstate->ps.state->es_tupleTable);
		/* build UPDATE SET expression and projection state */
		result_rel_info->ri_updateProj =
			ExecBuildUpdateProjection(node->updateTlist,
									  true,
									  node->upsertCols,
									  tupDesc,
									  econtext,
									  upsertState->us_updateproj,
									  &mtstate->ps);
		resultRelInfo->ri_RelationDesc->rd_conflictTid = NULL;
	}
	/* End - ReqID:SRS-MYSQL-SQL-INSERT */
	/*
	 * If we have any secondary relations in an UPDATE or DELETE, they need to
	 * be treated like non-locked relations in SELECT FOR UPDATE, ie, the
	 * EvalPlanQual mechanism needs to be told about them.  Locate the
	 * relevant ExecRowMarks.
	 */
	arowmarks = NIL;
	foreach(l, node->rowMarks)
	{
		PlanRowMark *rc = lfirst_node(PlanRowMark, l);
		ExecRowMark *erm;
		ExecAuxRowMark *aerm;

		/* ignore "parent" rowmarks; they are irrelevant at runtime */
		if (rc->isParent)
			continue;

		/* Find ExecRowMark and build ExecAuxRowMark */
		erm = ExecFindRowMark(estate, rc->rti, false);
		aerm = ExecBuildAuxRowMark(erm, subplan->targetlist);
		arowmarks = lappend(arowmarks, aerm);
	}

	/* For a MERGE command, initialize its state */
	if (mtstate->operation == CMD_MERGE)
		ExecInitMerge(mtstate, estate);

	/* Begin - SRS-SQL-MULTITABLEINSERT */
	if (mtstate->operation == CMD_MULTI_INSERT)
		ExecInitMultiInsert(mtstate, estate);
	/* End - SRS-SQL-MULTITABLEINSERT */

	EvalPlanQualSetPlan(&mtstate->mt_epqstate, subplan, arowmarks);

	/*
	 * If there are a lot of result relations, use a hash table to speed the
	 * lookups.  If there are not a lot, a simple linear search is faster.
	 *
	 * It's not clear where the threshold is, but try 64 for starters.  In a
	 * debugging build, use a small threshold so that we get some test
	 * coverage of both code paths.
	 */
#ifdef USE_ASSERT_CHECKING
#define MT_NRELS_HASH 4
#else
#define MT_NRELS_HASH 64
#endif
	if (nrels >= MT_NRELS_HASH)
	{
		HASHCTL		hash_ctl;

		hash_ctl.keysize = sizeof(Oid);
		hash_ctl.entrysize = sizeof(MTTargetRelLookup);
		hash_ctl.hcxt = CurrentMemoryContext;
		mtstate->mt_resultOidHash =
			hash_create("ModifyTable target hash",
						nrels, &hash_ctl,
						HASH_ELEM | HASH_BLOBS | HASH_CONTEXT);
		for (i = 0; i < nrels; i++)
		{
			Oid			hashkey;
			MTTargetRelLookup *mtlookup;
			bool		found;

			resultRelInfo = &mtstate->resultRelInfo[i];
			hashkey = RelationGetRelid(resultRelInfo->ri_RelationDesc);
			mtlookup = (MTTargetRelLookup *)
				hash_search(mtstate->mt_resultOidHash, &hashkey,
							HASH_ENTER, &found);
			/* Begin - ReqID:SRS-MYSQL-UPDATE */
			/* check partition is duplicated. */
			if (MYS_MODE == compatible_mode && found) /* BUG#Z5121 */
				ereport(ERROR,
						(errcode(ERRCODE_DUPLICATE_TABLE),
						 errmsg("The partition relation %s is duplicated.", RelationGetRelationName(resultRelInfo->ri_RelationDesc))));
			else if (mtstate->operation != CMD_MULTI_INSERT) /* SRS-SQL-MULTITABLEINSERT */
				Assert(!found);
			/* End - ReqID:SRS-MYSQL-UPDATE */
			mtlookup->relationIndex = i;
		}
	}
	else
		mtstate->mt_resultOidHash = NULL;

	/*
	 * Determine if the FDW supports batch insert and determine the batch size
	 * (a FDW may support batching, but it may be disabled for the
	 * server/table).
	 *
	 * We only do this for INSERT, so that for UPDATE/DELETE the batch size
	 * remains set to 0.
	 */
	if (operation == CMD_INSERT)
	{
		/* insert may only have one relation, inheritance is not expanded */
		Assert(nrels == 1);
		resultRelInfo = mtstate->resultRelInfo;
		if (!resultRelInfo->ri_usesFdwDirectModify &&
			resultRelInfo->ri_FdwRoutine != NULL &&
			resultRelInfo->ri_FdwRoutine->GetForeignModifyBatchSize &&
			resultRelInfo->ri_FdwRoutine->ExecForeignBatchInsert)
		{
			resultRelInfo->ri_BatchSize =
				resultRelInfo->ri_FdwRoutine->GetForeignModifyBatchSize(resultRelInfo);
			Assert(resultRelInfo->ri_BatchSize >= 1);
		}
		else
			resultRelInfo->ri_BatchSize = 1;
	}

	/*
	 * Lastly, if this is not the primary (canSetTag) ModifyTable node, add it
	 * to estate->es_auxmodifytables so that it will be run to completion by
	 * ExecPostprocessPlan.  (It'd actually work fine to add the primary
	 * ModifyTable node too, but there's no need.)  Note the use of lcons not
	 * lappend: we need later-initialized ModifyTable nodes to be shut down
	 * before earlier ones.  This ensures that we don't throw away RETURNING
	 * rows that need to be seen by a later CTE subplan.
	 */
	if (!mtstate->canSetTag)
		estate->es_auxmodifytables = lcons(mtstate,
										   estate->es_auxmodifytables);
	/* Begin - ReqID:SRS-MYSQL-UPDATE */
	if (MYS_MODE == compatible_mode && (operation == CMD_UPDATE || operation == CMD_DELETE)
		&& node->parseresultrelations &&
		list_length(node->parseresultrelations) > 1) /* BUG#Z5121 */
		{
			if (mtstate->ps.ps_ExprContext == NULL)
				ExecAssignExprContext(estate, &mtstate->ps);
			InitMultipleModify(mtstate, list_length(node->resultRelations));
			mtstate->mt_parseresultrelations = node->parseresultrelations;
		}
	/* End - ReqID:SRS-MYSQL-UPDATE */

	return mtstate;
}


/* Begin - ReqID:SRS-MYSQL-UPDATE */
/*
 * ExecClearMultipleModifyTuple - Cleap up multiple relation slots.
 */
static void
ExecClearMultipleModifyTuple(List* slots)
{
	ListCell	   *l;
	foreach(l, slots)
	{
		TupleTableSlot* slot = (TupleTableSlot*)lfirst(l);
		(void)ExecClearTuple(slot);
	}
}
/* End - ReqID:SRS-MYSQL-UPDATE */

/* ----------------------------------------------------------------
 *		ExecEndModifyTable
 *
 *		Shuts down the plan.
 *
 *		Returns nothing of interest.
 * ----------------------------------------------------------------
 */
void
ExecEndModifyTable(ModifyTableState *node)
{
	int			i;

	/*
	 * Allow any FDWs to shut down
	 */
	for (i = 0; i < node->mt_nrels; i++)
	{
		int			j;
		ResultRelInfo *resultRelInfo = node->resultRelInfo + i;

		if (!resultRelInfo->ri_usesFdwDirectModify &&
			resultRelInfo->ri_FdwRoutine != NULL &&
			resultRelInfo->ri_FdwRoutine->EndForeignModify != NULL)
			resultRelInfo->ri_FdwRoutine->EndForeignModify(node->ps.state,
														   resultRelInfo);

		/*
		 * Cleanup the initialized batch slots. This only matters for FDWs
		 * with batching, but the other cases will have ri_NumSlotsInitialized
		 * == 0.
		 */
		for (j = 0; j < resultRelInfo->ri_NumSlotsInitialized; j++)
		{
			ExecDropSingleTupleTableSlot(resultRelInfo->ri_Slots[j]);
			ExecDropSingleTupleTableSlot(resultRelInfo->ri_PlanSlots[j]);
		}
		/* Begin - SRS-SQL-MULTITABLEINSERT */
		if (node->operation == CMD_MULTI_INSERT)
		{
			if (resultRelInfo->ri_partition_tuple_routing != NULL)
				ExecCleanupTupleRouting(node, resultRelInfo->ri_partition_tuple_routing);
			if (resultRelInfo->ri_part_dir != NULL)
			{
				DestroyPartitionDirectory(resultRelInfo->ri_part_dir);
				resultRelInfo->ri_part_dir = NULL;
			}
		}
		/* End - SRS-SQL-MULTITABLEINSERT */
	}

	/*
	 * Close all the partitioned tables, leaf partitions, and their indices
	 * and release the slot used for tuple routing, if set.
	 */
	if (node->mt_partition_tuple_routing)
	{
		ExecCleanupTupleRouting(node, node->mt_partition_tuple_routing);

		if (node->mt_root_tuple_slot)
			ExecDropSingleTupleTableSlot(node->mt_root_tuple_slot);
	}

	/* Begin - ReqID:SRS-MYSQL-UPDATE */
	if (MYS_MODE == compatible_mode && node->mt_partition_tuple_routing_list) /* BUG#Z5121 */
	{
		ListCell   *lc = NULL;
		ListCell   *lc1 = NULL;
		foreach(lc, node->mt_partition_tuple_routing_list)
		{
			PartitionTupleRouting	   *partition_tuple_routing = lfirst(lc);
			ExecCleanupTupleRouting(node, partition_tuple_routing);
		}
		if (node->mt_root_tuple_slot_list)
		{
			foreach(lc1, node->mt_root_tuple_slot_list)
			{
				TupleTableSlot			   *root_tuple_slot = lfirst(lc1);
				ExecDropSingleTupleTableSlot(root_tuple_slot);
			}
		}
	}
	/* End - ReqID:SRS-MYSQL-UPDATE */
	/*
	 * Free the exprcontext
	 */
	ExecFreeExprContext(&node->ps);

	/*
	 * clean out the tuple table
	 */
	if (node->ps.ps_ResultTupleSlot)
		ExecClearTuple(node->ps.ps_ResultTupleSlot);

	/* Begin - ReqID:SRS-MYSQL-UPDATE */
	ExecClearMultipleModifyTuple(node->mt_ResultTupleSlots);
	/* End - ReqID:SRS-MYSQL-UPDATE */

	/*
	 * Terminate EPQ execution if active
	 */
	EvalPlanQualEnd(&node->mt_epqstate);

	/*
	 * shut down subplan
	 */
	ExecEndNode(outerPlanState(node));
}

void
ExecReScanModifyTable(ModifyTableState *node)
{
	/*
	 * Currently, we don't need to support rescan on ModifyTable nodes. The
	 * semantics of that would be a bit debatable anyway.
	 */
	elog(ERROR, "ExecReScanModifyTable is not implemented");
}
