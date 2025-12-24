/*-------------------------------------------------------------------------
 *
 * File:ora_trigger_stack.c
 *
 * Abstract:
 *		TriggerData stack for support implement the Oracle trigger's
 *		predicate.
 *
 *
 * ReqID:SRS-SQL-TRIGGER
 *
 * Authored by zhaoshengbin@highgo.com,20251217.
 *
 * Copyright:
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  src/backend/commands/ora_trigger_stack.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "commands/trigger.h"
#include "utils/memutils.h"

typedef struct TriggerStackEntry
{
	TriggerData *trigdata;
	struct TriggerStackEntry *prev;
} TriggerStackEntry;

static TriggerStackEntry *trigger_stack_top = NULL;

void
push_triggerdata(TriggerData *trigdata)
{
	TriggerStackEntry *entry;

	entry = MemoryContextAlloc(TopMemoryContext,
								sizeof(TriggerStackEntry));
	entry->trigdata = trigdata;
	entry->prev = trigger_stack_top;
	trigger_stack_top = entry;
}

void
pop_triggerdata(void)
{
	TriggerStackEntry *entry = trigger_stack_top;

	if (entry)
	{
		trigger_stack_top = entry->prev;
		pfree(entry);
	}
}

TriggerData *
peek_triggerdata(void)
{
	return trigger_stack_top ? trigger_stack_top->trigdata : NULL;
}
