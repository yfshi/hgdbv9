/* Begin - ReqID:SRS-SECURITY-RESOURCEMANAGEMENT */
/*
 * walkers.c
 *
 *  Created on: Feb 8, 2011
 *      Author: siva
 *
 * Authored by zhangyilin@highgo.com
 * Portions Copyright (c) 2009-2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 */

#include "postgres.h"

#include "catalog/pg_collation.h"
#include "catalog/pg_type.h"
#include "miscadmin.h"
#include "nodes/nodeFuncs.h"
#include "optimizer/walkers.h"


/*
 * These are helpers to find node in queries
 */
typedef struct find_nodes_context
{
	List *nodeTags;
	int foundNode;
} find_nodes_context;

static bool find_nodes_walker(Node *node, find_nodes_context *context);

/*
 * Looks for nodes that belong to the given list.
 * Returns the index of the first such node that it encounters, or -1 if none
 */
int find_nodes(Node *node, List *nodeTags)
{
	find_nodes_context context;
	Assert(NULL != node);
	context.nodeTags = nodeTags;
	context.foundNode = -1;
	find_nodes_walker(node, &context);

	return context.foundNode;
}

static bool
find_nodes_walker(Node *node, find_nodes_context *context)
{
	ListCell *lc;
	int i = 0;

	if (NULL == node)
	{
		return false;
	}

	if (IsA(node, Query))
	{
		/* Recurse into subselects */
		return query_tree_walker((Query *) node, find_nodes_walker, (void *) context, 0 /* flags */);
	}
	foreach(lc, context->nodeTags)
	{
		NodeTag nodeTag = (NodeTag) lfirst_int(lc);
		if (nodeTag(node) == nodeTag)
		{
			context->foundNode = i;
			return true;
		}
		i++;
	}

	return expression_tree_walker(node, find_nodes_walker, (void *) context);
}
/* End - ReqID:SRS-SECURITY-RESOURCEMANAGEMENT */

