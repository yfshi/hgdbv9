/*-------------------------------------------------------------------------
 *
 * nodeCount.c
 *	  Routines to Count node.
 *
 * IDENTIFICATION
 *	  src/backend/executor/nodeCount.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "executor/executor.h"
#include "executor/nodeCount.h"
#include "utils/memutils.h"
#include "miscadmin.h"

/* ----------------------------------------------------------------
 * ExecCount
 *
 * Main executor of Count node.
 * ----------------------------------------------------------------
 */
static TupleTableSlot *
ExecCount(PlanState *pstate)
{
	CountState	*node = castNode(CountState, pstate);
	TupleTableSlot	*outerTupleSlot;
	TupleTableSlot	*resultSlot;
	ExprContext		*econtext;

	CHECK_FOR_INTERRUPTS();

	econtext = node->ps.ps_ExprContext;

	if (node->countDone)
		return NULL;

	/*
	 * Reset per-tuple memory context to free any expression evaluation
	 * storage allocated in the previous tuple cycle.  Note this can't happen
	 * until we're done projecting out tuples from a scan tuple.
	 */
	ResetExprContext(econtext);

	while (!node->countDone)
	{
		/*
		 * retrieve tuples from the outer plan until there are no more.
		 */
		outerTupleSlot = ExecProcNode(outerPlanState(node));

		if (TupIsNull(outerTupleSlot))
			return NULL;

		/*
		 * use outer tuple as scan tuple for projection
		 */
		econtext->ecxt_outertuple = outerTupleSlot;
		econtext->ecxt_scantuple = outerTupleSlot;
		econtext->count_rownum = *node->counter;

		/*
		 * Check STOPKEY condition
		 */
		if (node->stopkeys != NULL
			&& !ExecQual(node->stopkeys, econtext))
		{
			node->countDone = true;
			break;
		}

		if (node->ps.qual == NULL
			|| ExecQual(node->ps.qual, econtext))
		{
			/*
			 * form the result tuple using ExecProject(), and return it --- unless
			 * the projection produces an empty set, in which case we must loop
			 * back to see if there are more outerPlan tuples.
			 */
			resultSlot = ExecProject(node->ps.ps_ProjInfo);
			(*node->counter)++;

			return resultSlot;
		}
	}

	return NULL;
}

/* ----------------------------------------------------------------
 * ExecInitCount
 *
 * 	 This initializes the count node state structures and
 *		the node's subplan.
 * ----------------------------------------------------------------
 */
CountState *
ExecInitCount(Count *node, EState *estate, int eflags)
{
	CountState	*countstate;
	Plan		*outerPlan;

	/* check for unsupported flags */
	Assert(!(eflags & (EXEC_FLAG_MARK | EXEC_FLAG_BACKWARD)));

	/*
	 * create state structure
	 */
	countstate = makeNode(CountState);
	countstate->ps.plan = (Plan *) node;
	countstate->ps.state = estate;
	countstate->ps.ExecProcNode = ExecCount;
	countstate->countDone = false;

	/*
	 * init countstate->counter as a pointer to a member of estate->count
	 * according to the plan node's countnode id, and init its value as 1.
	 */
	countstate->counter = &estate->count[node->countnode_id];
	*(countstate->counter) = 1;

	/*
	 * Miscellaneous initialization
	 *
	 * create expression context for node
	 */
	ExecAssignExprContext(estate, &countstate->ps);

	/*
	 * Tuple table initialization
	 */
	ExecInitResultTupleSlotTL(&countstate->ps, &TTSOpsVirtual);
	ExecAssignProjectionInfo(&countstate->ps, NULL);

	/*
	 * initialize child expressions
	 */
	countstate->ps.qual =
		ExecInitQual(node->plan.qual, (PlanState *) countstate);
	countstate->stopkeys =
		ExecInitQual((List *) node->stopkeys, (PlanState *) countstate);

	/*
	 * then initialize outer plan
	 */
	outerPlan = outerPlan(node);
	outerPlanState(countstate) = ExecInitNode(outerPlan, estate, eflags);

	/*
	 * we don't use inner plan
	 */
	Assert(innerPlan(node) == NULL);

	/*
	 * initialize tuple type and projection info
	 */
	ExecInitResultTypeTL(&countstate->ps);
	ExecAssignProjectionInfo(&countstate->ps, NULL);

	return countstate;
}


/* ----------------------------------------------------------------
 *		ExecEndCount
 *
 *		This shuts down the subplan and frees resources allocated
 *		to this node.
 * ----------------------------------------------------------------
 */
void
ExecEndCount(CountState *node)
{
	/* Free the exprcontext */
	ExecFreeExprContext(&node->ps);

	/* clean up tuple table */
	ExecClearTuple(node->ps.ps_ResultTupleSlot);

	/* shut down subplans */
	ExecEndNode(outerPlanState(node));
}

void
ExecReScanCount(CountState *node)
{
	/* must clear result tuple so first input tuple is returned */
	ExecClearTuple(node->ps.ps_ResultTupleSlot);

	node->countDone = false;
	*node->counter = 1;

	/*
	 * if chgParam of subnode is not null then plan will be re-scanned by
	 * first ExecProcNode.
	 */
	if (node->ps.lefttree &&
		node->ps.lefttree->chgParam == NULL)
		ExecReScan(node->ps.lefttree);
}
