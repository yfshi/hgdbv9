/*-------------------------------------------------------------------------
 *
 * File:ora_trigger_order.c
 *
 * Abstract:
 *		Trigger sorting to be compatible with the follows
 *		clause of Oracle triggers.
 *
 *
 * ReqID:SRS-SQL-TRIGGER
 *
 * Authored by zhaoshengbin@highgo.com,20250709.
 *
 * Copyright:
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  src/backend/commands/ora_trigger_order.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "utils/hsearch.h"
#include "utils/elog.h"
#include "nodes/pg_list.h"
#include "catalog/pg_trigger.h"
#include "miscadmin.h"
#include "commands/trigger.h"
#include "utils/memutils.h"

#include "utils/jsonb.h"
#include "lib/binaryheap.h"
#include "parser/pg_object_hook.h"
#include "utils/ruleutils.h"


/*
 * adjacency list for trigger graph node
 */
typedef struct TriggerAdj
{
	int     nNeighbors;
	int     allocNeighbors;
	int     *neighbors;		/* indices of neighbor nodes (out edges) */
} TriggerAdj;

/*
 * add_trigger_edge
 *	add directed edge u -> v  (u before v)
 */
static void
add_trigger_edge(TriggerAdj *adj, int u, int v)
{
	if (adj[u].nNeighbors >= adj[u].allocNeighbors)
	{
		if (adj[u].allocNeighbors <= 0)
		{
			adj[u].allocNeighbors = 8;
			adj[u].neighbors = (int *) palloc(adj[u].allocNeighbors * sizeof(int));
		}
		else
		{
			adj[u].allocNeighbors *= 2;
			adj[u].neighbors = (int *) repalloc(adj[u].neighbors,
												adj[u].allocNeighbors * sizeof(int));
		}
	}
	adj[u].neighbors[adj[u].nNeighbors++] = v;
}

static int
find_trigger_index_by_name(Trigger *triggers, int numtrigs, char *name)
{
	int     i;
	if (name == NULL)
		return -1;

	for (i = 0; i < numtrigs; i++)
	{
		if (strcmp(triggers[i].tgname, name) == 0)
			return i;
	}

	return -1;
}

/*
 * orderTriggersByPrecedence
 *
 * Build directed graph based on trigger FOLLOWS and perform
 * topological sort. On success, produce an array 'ordered' of trigger
 * indices in execution order (from first to last).
 *
 * If cycles detected, emit WARNING and return false, caller should fall back
 * to default order.
 */
bool
orderTriggersByPrecedence(Trigger *triggers, int numtrigs, int **out_order, int *out_count)
{
	int         i, j;
	TriggerAdj *adj;
	int        *indegree = NULL;
	int        *queue = NULL;
	int         qhead = 0, qtail = 0;
	int        *ordered = NULL;
	int         ordered_count = 0;
	ObjectAddress   object;
	PgObjectInfo    objinfo;


	if (numtrigs <= 1)
	{
		/* trivial */
		*out_order = NULL;
		*out_count = 0;
		return true;
	}

	/* allocate adjacency list */
	adj = (TriggerAdj *) palloc0(sizeof(TriggerAdj) * numtrigs);
	indegree = (int *) palloc0(sizeof(int) * numtrigs);

	/* Iterate triggers again and add edges according to follows */
	for (i = 0; i < numtrigs; i++)
	{
		List        *tr_follows = NIL;
		ListCell	*l;

		ObjectAddressSet(object, TriggerRelationId, triggers[i].tgoid);
		objinfo =  pg_object_get_ori_infor_hook(object);

		if (objinfo && objinfo->oricompatible_mode == 'o' && objinfo->orioption)
		{
			JsonbValue  key;
			JsonbValue *followsValue = NULL;

			key.type = jbvString;
			key.val.string.val = "follows";
			key.val.string.len = strlen("follows");

			followsValue = findJsonbValueFromContainer(&objinfo->orioption->root, JB_FOBJECT, &key);
			if (followsValue && followsValue->type == jbvBinary)
			{
				tr_follows = ivy_extract_array_from_jbvBinary(followsValue);
			}
		}

		if (tr_follows)
		{
			foreach(l, tr_follows)
			{
				char    *other_name = lfirst(l);
				int other_idx = find_trigger_index_by_name(triggers, numtrigs, other_name);

				/*
				 * other_idx == -1 indicates that other_name not exist in the current trigger follows
				 */
				if (other_idx < 0)
				{
					ereport(ERROR,
							(errmsg("trigger \"%s\" depends on non-existent trigger \"%s\"",
							 triggers[i].tgname, other_name)));
				}
				else if (other_idx == i)
				{
					ereport(ERROR,
							(errmsg("trigger \"%s\" declares FOLLOWS itself",
							 triggers[i].tgname)));
				}
				else
				{
					add_trigger_edge(adj, other_idx, i);
					indegree[i]++;
				}
			}
		}
	}

	/* Kahn's algorithm: init queue with indegree 0 nodes */
	queue = (int *) palloc0(sizeof(int) * numtrigs);
	ordered = (int *) palloc0(sizeof(int) * numtrigs);

	for (i = 0; i < numtrigs; i++)
	{
		if (indegree[i] == 0)
		{
			queue[qtail++] = i;
		}
	}

	while (qhead < qtail)
	{
		int u = queue[qhead++];
		ordered[ordered_count++] = u;

		for (j = 0; j < adj[u].nNeighbors; j++)
		{
			int v = adj[u].neighbors[j];
			indegree[v]--;
			if (indegree[v] == 0)
				queue[qtail++] = v;
		}
	}

	if (ordered_count != numtrigs)
	{
		/* free adjacency */
		for (i = 0; i < numtrigs; i++)
		{
			if (adj[i].neighbors)
				pfree(adj[i].neighbors);
		}

		pfree(adj);
		pfree(indegree);
		pfree(queue);
		pfree(ordered);

		ereport(ERROR,
				(errmsg("cycle detected in trigger precedence chain")));
	}

	/* success: return ordered array */
	*out_order = ordered;
	*out_count = ordered_count;

	/* free adjacency arrays (keep ordered) */
	for (i = 0; i < numtrigs; i++)
	{
		if (adj[i].neighbors)
			pfree(adj[i].neighbors);
	}

	pfree(adj);
	pfree(indegree);
	pfree(queue);

	return true;
}
