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

static int trigger_name_compare(Datum a, Datum b, void *arg);

/*
 *	trigger_name_compare
 *		compare function
 */
static int
trigger_name_compare(Datum a, Datum b, void *arg)
{
	Trigger 	*triggers;
	int 		ia, ib;

	triggers = (Trigger *) arg;
	ia = DatumGetInt32(a);
	ib = DatumGetInt32(b);

	return strcmp(triggers[ia].tgname, triggers[ib].tgname);
}

/*
 *	ExtractStringArray
 *		Extracts an array of strings from a Jsonb object
 */
List *
ivyExtractJsonbStringArray(JsonbValue *arrayValue)
{
	List		*result = NIL;
	int 		i;

	if (arrayValue == NULL || arrayValue->type != jbvArray)
		return NIL;

	for (i = 0; i < arrayValue->val.array.nElems; i++)
	{
		JsonbValue *elem = &arrayValue->val.array.elems[i];
		if (elem->type == jbvString)
		{
			char 	*str = palloc(elem->val.string.len + 1);

			memcpy(str, elem->val.string.val, elem->val.string.len);
			str[elem->val.string.len] = '\0';

			result = lappend(result, str);
		}
	}
	return result;
}


/*
 *	CreateTriggerNameMap
 *		Create a HTAB from trigger name to index
 */
HTAB *
CreateTriggerNameMap(Trigger *triggers, int numtrigs)
{
	HASHCTL		ctl;
	HTAB		*map;
	int			i;

	MemSet(&ctl, 0, sizeof(ctl));
	ctl.keysize = NAMEDATALEN;
	ctl.entrysize = sizeof(TriggerNameMapEntry);
	ctl.hcxt = CurrentMemoryContext;

	map = hash_create("trigger name map", numtrigs, &ctl,
					 HASH_ELEM | HASH_CONTEXT | HASH_STRINGS);

	for (i = 0; i < numtrigs; i++)
	{
		TriggerNameMapEntry *entry;
		bool		found;
		char	name[NAMEDATALEN];

		strncpy(name, triggers[i].tgname, NAMEDATALEN);
		name[NAMEDATALEN - 1] = '\0';

		entry = (TriggerNameMapEntry *) hash_search(map, name, HASH_ENTER, &found);
		if (found)
		{
			/* Trigger name should be unique on a table */
			ereport(ERROR,
					(errcode(ERRCODE_DUPLICATE_OBJECT),
					errmsg("duplicate trigger name \"%s\"", name)));
		}

		entry->index = i;
	}

	return map;
}

/*
 *	GetTriggerFollowsDependencies
 *		Get trigger follows dependency
 */
void
GetTriggerFollowsDependencies(Trigger *triggers, int numtrigs,
					TriggerDepInfo *depInfo, HTAB *nameToIndex)
{
	ObjectAddress	object;
	PgObjectInfo	objinfo;
	TriggerNameMapEntry *entry;
	int				i;

	for (i = 0; i < numtrigs; i++)
	{
		depInfo[i].follows = NIL;

		entry = (TriggerNameMapEntry *) hash_search(nameToIndex, triggers[i].tgname, HASH_FIND, NULL);
		if (entry == NULL)
			continue;

		object.classId = TriggerRelationId;
		object.objectId = triggers[i].tgoid;
		object.objectSubId = 0;

		objinfo =  pg_object_get_ori_infor_hook(object);

		if (objinfo && objinfo->oricompatible_mode == 'o' && objinfo->orioption)
		{
			JsonbValue  key;
			JsonbValue *followsValue = NULL;

			key.type = jbvString;
			key.val.string.val = "follows";
			key.val.string.len = strlen("follows");

			followsValue = findJsonbValueFromContainer(&objinfo->orioption->root, JB_FOBJECT, &key);
			if (followsValue && followsValue->type == jbvArray)
			{
				depInfo[entry->index].follows = ivyExtractJsonbStringArray(followsValue);
			}
		}
	}
}

/*
 *	TopologicalSortTriggers
 *		Topological sorting, using a priority queue, ensures
 *		order by name when there is no dependency
 */
int *
TopologicalSortTriggers(Trigger *triggers, int numtrigs,
					TriggerDepInfo *depInfo, HTAB *nameToIndex)
{
	int			*inDegree;
	List		**graph;
	int			*sortedIndexes;
	binaryheap	*queue;
	int			count = 0;
	int			i;

	/* allocating memory */
	inDegree = (int *) palloc0(numtrigs * sizeof(int));
	graph = (List **) palloc0(numtrigs * sizeof(List *));
	sortedIndexes = (int *) palloc(numtrigs * sizeof(int));

	/* Building the dependency graph */
	for (i = 0; i < numtrigs; i++)
	{
		ListCell	*lc;

		foreach(lc, depInfo[i].follows)
		{
			char *depName = (char *) lfirst(lc);
			TriggerNameMapEntry *entry;
			int		depIndex;

			entry = (TriggerNameMapEntry *) hash_search(nameToIndex, depName, HASH_FIND, NULL);
			if (entry == NULL)
			{
				/* depend trigger not exist */
				ereport(WARNING,
						(errmsg("trigger \"%s\" depends on non-existent trigger \"%s\"",
								triggers[i].tgname, depName)));
				continue;
			}

			depIndex = entry->index;

			/* Add an edge from the dependent trigger to the current trigger */
			graph[depIndex] = lappend_int(graph[depIndex], i);
			inDegree[i]++;
		}
	}

	/* Initialize the priority queue(sorted by name) */
	queue = binaryheap_allocate(numtrigs, trigger_name_compare, triggers);

	/* Add a trigger with in-degree 0 to the queue */
	for (i = 0; i < numtrigs; i++)
	{
		if (inDegree[i] == 0)
		{
			binaryheap_add(queue, Int32GetDatum(i));
		}
	}

	/* Topo sort */
	while (!binaryheap_empty(queue))
	{
		Datum		d;
		int			idx;
		ListCell	*lc;

		d = binaryheap_remove_first(queue);
		idx = DatumGetInt32(d);
		sortedIndexes[count++] = idx;

		/* Process all successor nodes */
		foreach(lc, graph[idx])
		{
			int succ = lfirst_int(lc);

			inDegree[succ]--;
			if (inDegree[succ] == 0)
			{
				binaryheap_add(queue, Int32GetDatum(succ));
			}
		}
	}

	/* Check circular dependencies */
	if (count != numtrigs)
	{
		/* Find unhandled triggers */
		for (i = 0; i < numtrigs; i++)
		{
			if (inDegree[i] > 0)
			{
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
						 errmsg("circular dependency detected involving trigger \"%s\"",
								triggers[i].tgname)));
			}
		}
		elog(ERROR, "circular dependency detected in trigger FOLLOWS");
	}

	/* Clean up*/
	for (i = 0; i < numtrigs; i++)
	{
		if (graph[i] != NIL)
			list_free(graph[i]);
	}

	pfree(graph);
	pfree(inDegree);
	binaryheap_free(queue);

	return sortedIndexes;
}
