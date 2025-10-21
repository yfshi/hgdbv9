/*-------------------------------------------------------------------------
 *
 * autonxstack.c
 *	  autonomous transaction system support routines
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * Copyright:
 * Copyright (c) 2009-2025, HighGo Software Co.,Ltd. All right reserved.
 *
 * IDENTIFICATION
 *	  src/backend/access/transam/autonxstack.c
 *
 * ReqID:SRS-PLSQL-AUTONOMOUSTRANSACTION
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/xact.h"
#include "access/transam.h"
#include "miscadmin.h"
#include "nodes/pg_list.h"
#include "storage/proc.h"
#include "utils/elog.h"
#include "utils/memutils.h"
#include "utils/snapmgr.h"


#define RESNAME_LEN	32

/*
 * registered transaction var information
 */
typedef struct AutonxVarInfo
{
	char	name[RESNAME_LEN];				/* name of var */
	void   *address;						/* address */
	int		len;							/* length of var */
	AutoxIsoVarsHookType	post_backup;	/* called after backup */
	AutoxIsoVarsHookType	prev_restore;	/* called before restore */
	AutoxIsoVarsHookType	prev_backup;	/* called before backup */
	AutoxIsoVarsHookType	post_restore;	/* called after restore */
} AutonxVarInfo;

/*
 * vars stack entry
 */
typedef struct AutonxVarsEntry
{
	void					*data;		/* raw data of all vars */
	struct AutonxVarsEntry	*parent;	/* parent entry */
} AutonxVarsEntry;

/*
 * vars stack
 */
typedef struct AutonxVarsStack
{
	int				size;	/* stack size */
	AutonxVarsEntry *entry;	/* stack top */
} AutonxVarsStack;

/*
 * transaction isolation state structure
 */
typedef struct AutonxIsolationState
{
	List			   *vars;			/* vars list */
	int					varsTotalLen;	/* length of all vars */
	AutonxVarsStack	   *varStack;		/* vars stack */
	MemoryContext		context;		/* session-life memory context */
} AutonxIsolationState;


static AutonxIsolationState *CurrentAutoxIsolationState = NULL;

AutoxIsoVarsHookType RegisterAutonxIsoVars_PlisqlHook;
AutoxIsoVarsHookType RegisterAutonxIsoVars_PlpgsqlHook;

/*
 * InitAutoxIsolationState
 *   Initialize autonomous transaction isolation state data.
 */
void
InitAutoxIsolationState(void)
{
	/*
	 * CurrentAutoxIsolationState only need to be initilized once
	 * within the session.
	 */

	if (CurrentAutoxIsolationState == NULL)
	{
		CurrentAutoxIsolationState =
			MemoryContextAllocZero(TopMemoryContext, sizeof(AutonxIsolationState));

		CurrentAutoxIsolationState->context =
			AllocSetContextCreate(TopMemoryContext,
								"AutonTransIsoResContext",
								ALLOCSET_DEFAULT_SIZES);

		PG_TRY();
		{
			/*
			* register all transaction resources that required
			* isolation.
			*/

			RegisterAutonxIsoVars_Xact();
			RegisterAutonxIsoVars_Process();
			RegisterAutonxIsoVars_LWLock();
			RegisterAutonxIsoVars_CV();
			RegisterAutonxIsoVars_Lock();
			RegisterAutonxIsoVars_Lmgr();
			RegisterAutonxIsoVars_PredLock();
			RegisterAutonxIsoVars_HtabSeqScan();
			RegisterAutonxIsoVars_Trigger();
			RegisterAutonxIsoVars_Snapmgr();
			RegisterAutonxIsoVars_LargeObject();
			RegisterAutonxIsoVars_Portal();
			RegisterAutonxIsoVars_Notify();
			RegisterAutonxIsoVars_Smgr();
			RegisterAutonxIsoVars_Files();
			RegisterAutonxIsoVars_Combocid();
			RegisterAutonxIsoVars_PgStat();
			RegisterAutonxIsoVars_Enum();
			RegisterAutonxIsoVars_PendingStorage();
			RegisterAutonxIsoVars_Relmapper();
			RegisterAutonxIsoVars_Relcache();
			RegisterAutonxIsoVars_Inval();
			RegisterAutonxIsoVars_MultiXact();

			/*
			* hook only for plisql
			*/
			if (RegisterAutonxIsoVars_PlisqlHook != 0)
				RegisterAutonxIsoVars_PlisqlHook();
		}
		PG_CATCH();
		{
			MemoryContextDelete(CurrentAutoxIsolationState->context);
			pfree(CurrentAutoxIsolationState);
			CurrentAutoxIsolationState = NULL;

			PG_RE_THROW();
		}
		PG_END_TRY();
	}

	/*
	 * Hook for other extensions.
	 *
	 * Try to register vars of extensions every time when autonomous transaction
	 * starting to prevent the extension library being loaded after initialization.
	 *
	 * Additionally, please prevent duplicate registrations within in the hook function.
	 */
	if (RegisterAutonxIsoVars_PlpgsqlHook != 0)
		RegisterAutonxIsoVars_PlpgsqlHook();
}

/*
 * RegisterAutonxIsoVar
 *   Register a var, simplified of RegisterAutonxIsoVarExt.
 */
void
RegisterAutonxIsoVar(const char *name, void *address, int len,
					 AutoxIsoVarsHookType post_backup,
					 AutoxIsoVarsHookType prev_restore)
{
	RegisterAutonxIsoVarExt(name, address, len, post_backup, prev_restore, NULL, NULL);
}

/*
 * RegisterAutonxIsoVarExt
 *   Register a transaction global var.
 *
 * name - the name of transaction global var.
 * address - the address of var.
 * len - the length of var to be copied.
 * prev_backup - hook function before var backup
 * post_backup - hook function after var backup
 * prev_restore - hook function before var restore
 * post_restore - hook function after var restore
 */
void
RegisterAutonxIsoVarExt(const char *name, void *address, int len,
						AutoxIsoVarsHookType post_backup,
						AutoxIsoVarsHookType prev_restore,
						AutoxIsoVarsHookType prev_backup,
						AutoxIsoVarsHookType post_restore)
{
	MemoryContext oldcxt;
	AutonxVarInfo *info;

	Assert(CurrentAutoxIsolationState != NULL);

	oldcxt = MemoryContextSwitchTo(CurrentAutoxIsolationState->context);

	info = palloc0(sizeof(AutonxVarInfo));
	strncpy(info->name, name, RESNAME_LEN);
	info->address = address;
	info->len = len;
	info->post_backup = post_backup;
	info->prev_restore = prev_restore;
	info->prev_backup = prev_backup;
	info->post_restore = post_restore;

	CurrentAutoxIsolationState->vars =
		lappend(CurrentAutoxIsolationState->vars, info);
	CurrentAutoxIsolationState->varsTotalLen += len;

	MemoryContextSwitchTo(oldcxt);
}

/*
 * AutoxIsoStackPush
 *   Create transaction isolation state stack entry for a normal main transaction
 * or a autonomous main transaction.
 */
void
AutoxIsoStackPush(void)
{
	MemoryContext oldcxt;
	AutonxVarsEntry *entry;
	ListCell *lc;
	void *dest;
	char *pos;
	int num = 0;

	oldcxt = MemoryContextSwitchTo(CurrentAutoxIsolationState->context);

	/*
	 * Request all memory spaces firstly to prevent rollback below.
	 */
	if (CurrentAutoxIsolationState->varStack == NULL)
		CurrentAutoxIsolationState->varStack = palloc0(sizeof(AutonxVarsStack));
	entry = palloc0(sizeof(AutonxVarsEntry));

	dest = palloc(CurrentAutoxIsolationState->varsTotalLen);
	pos = (char *) dest;

	PG_TRY();
	{
		num = 0;

		/*
		 * backup outter transaction resource are as folows:
		 *   for every var in varList
		 *     call the pre function
		 *     copy
		 *     call the post function
		 */
		foreach (lc, CurrentAutoxIsolationState->vars)
		{
			AutonxVarInfo *info = (AutonxVarInfo *) lfirst(lc);

			if (info->prev_backup)
				info->prev_backup();

			if (info->len > 0)
			{
				memcpy(pos, info->address, info->len);
				pos += info->len;
			}

			elog(DEBUG1, "[AutonxIsolationState][push][%s] from 0x%p into stack data at 0x%p, length %d",
				 info->name, info->address ? info->address : 0, info->len > 0 ? pos : 0, info->len);

			if (info->post_backup != NULL)
				info->post_backup();

			num++;
		}
	}
	PG_CATCH();
	{
		/*
		 * If failed, origin vars may have already been lost, and end
		 * the current session directly.
		 */
		elog(FATAL, "push resources failed before starting autonomous transaction");

		PG_RE_THROW();
	}
	PG_END_TRY();

	entry->data = dest;
	entry->parent = CurrentAutoxIsolationState->varStack->entry;
	CurrentAutoxIsolationState->varStack->entry = entry;
	CurrentAutoxIsolationState->varStack->size++;

	elog(DEBUG1,
		 "push AutonxIsolationState successfully, total resources %d, current depth is %d",
		 num, CurrentAutoxIsolationState->varStack->size);

	MemoryContextSwitchTo(oldcxt);
}

/*
 * AutoxIsoStackPop
 *   Pop back to outter transaction isolation state.
 */
void
AutoxIsoStackPop(void)
{
	AutonxVarsEntry *entry;
	ListCell *lc;
	void *dest;
	char *pos;
	int num = 0;

	Assert(CurrentAutoxIsolationState != NULL &&
		   CurrentAutoxIsolationState->varStack != NULL &&
		   CurrentAutoxIsolationState->varStack->size > 0);

	entry = CurrentAutoxIsolationState->varStack->entry;
	dest = entry->data;
	pos = (char *) dest;

	PG_TRY();
	{
		/*
		 * restore ouuter transaction resource step are as folows:
		 *   for every var in varList
		 *     call the pre function
		 *     copy
		 *     call the post function
		 */
		foreach (lc, CurrentAutoxIsolationState->vars)
		{
			AutonxVarInfo *info = (AutonxVarInfo *)lfirst(lc);

			if (info->prev_restore != NULL)
				info->prev_restore();

			if (info->len > 0)
			{
				memcpy(info->address, pos, info->len);
				pos += info->len;
			}

			if (info->post_restore)
				info->post_restore();

			elog(DEBUG1, "[AutonxIsolationState][pop][%s] from stack data at 0x%p to 0x%p, length %d",
				 info->name, info->len > 0 ? pos : 0, info->address ? info->address : 0, info->len);
			num++;
		}

		elog(DEBUG1,
			 "pop AutonxIsolationState successfully, total resouces %d, current depth is %d",
			 num, CurrentAutoxIsolationState->varStack->size - 1);
	}
	PG_CATCH();
	{
		/*
		 * If failed, origin vars may have already been lost, and end
		 * the current session directly.
		 */
		elog(FATAL, "pop resources failed after commting autonomous transaction");

		PG_RE_THROW();;
	}
	PG_END_TRY();

	CurrentAutoxIsolationState->varStack->entry = entry->parent;
	CurrentAutoxIsolationState->varStack->size--;

	pfree(entry->data);
	pfree(entry);
}
