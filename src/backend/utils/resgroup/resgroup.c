/* Begin - ReqID:SRS-SECURITY-RESOURCEMANAGEMENT */
/*-------------------------------------------------------------------------
 *
 * resgroup.c
 *	  GPDB resource group management code.
 *
 * Portions Copyright (c) 2006-2010, Greenplum inc.
 * Portions Copyright (c) 2012-Present VMware, Inc. or its affiliates.
 *
 * Authored by zhangyilin@highgo.com
 * Portions Copyright (c) 2009-2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	    src/backend/utils/resgroup/resgroup.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include <math.h>

#include "access/genam.h"
#include "access/table.h"
#include "access/xact.h"
#include "tcop/tcopprot.h"
#include "catalog/catalog.h"
#include "catalog/pg_authid.h"
#include "catalog/pg_resgroup.h"
#include "catalog/pg_resgroupcapability.h"
#include "commands/resgroupcmds.h"
#include "commands/tablespace.h"
#include "common/hashfn.h"
#include "funcapi.h"
#include "miscadmin.h"
#include "pgstat.h"
#include "port/atomics.h"
#include "postmaster/autovacuum.h"
#include "storage/ipc.h"
#include "storage/latch.h"
#include "storage/lmgr.h"
#include "storage/lock.h"
#include "storage/pg_shmem.h"
#include "storage/proc.h"
#include "storage/procarray.h"
#include "storage/procsignal.h"
#include "tcop/utility.h"
#include "utils/builtins.h"
#include "utils/memutils.h"
#include "utils/ps_status.h"
#include "utils/cgroup.h"
#include "utils/resgroup.h"
#include "utils/resource_manager.h"
#include "utils/cgroup-ops-v1.h"
#include "utils/cgroup-ops-dummy.h"
#include "utils/cgroup-ops-v2.h"
#include "utils/cgroup_io_limit.h"
#include "access/xact.h"
#include "nodes/nodeFuncs.h"
#include "replication/logicallauncher.h"
#include "optimizer/walkers.h"

#define INVALID_SESSION_PID -1
#define SHMEM_SESSION_STATE_ARRAY "Session State Array"

/* Pointer to SessionStateArray instance in shared memory */
volatile SessionStateArray *AllSessionStateEntries = NULL;

/*
 * A pointer to the entry in the array of SessionState that corresponds
 * to the SessionState of this session.
 */
volatile SessionState *MySessionState = NULL;
/* Have we initialized the sessionState pointer */
bool sessionStateInited = false;

#define InvalidSlotId	(-1)
#define RESGROUP_MAX_SLOTS	(MaxConnections)

/*
 * Data structures
 */
typedef struct ResGroupInfo				ResGroupInfo;
typedef struct ResGroupHashEntry		ResGroupHashEntry;
typedef struct ResGroupProcData			ResGroupProcData;
typedef struct ResGroupSlotData			ResGroupSlotData;
typedef struct ResGroupData				ResGroupData;
typedef struct ResGroupControl			ResGroupControl;

/*
 * Resource group info.
 *
 * This records the group and groupId for a transaction.
 * When group->groupId != groupId, it means the group
 * has been dropped.
 */
struct ResGroupInfo
{
	ResGroupData	*group;
	Oid				groupId;
};

struct ResGroupHashEntry
{
	Oid		groupId;
	int		index;
};

/*
 * Per proc resource group information.
 *
 * Config snapshot and runtime accounting information in current proc.
 */
struct ResGroupProcData
{
	Oid		groupId;
	ResGroupData		*group;
	ResGroupSlotData	*slot;
	ResGroupCaps	caps;
};

/*
 * Per slot resource group information.
 *
 * Resource group have 'concurrency' number of slots.
 * Each transaction acquires a slot on coordinator before running.
 * The information shared by QE processes on each segments are stored
 * in this structure.
 */
struct ResGroupSlotData
{
	Oid				groupId;
	ResGroupData	*group;		/* pointer to the group */
	int				nProcs;		/* number of procs in this slot */
	ResGroupSlotData	*next;
	ResGroupCaps	caps;
};

/*
 * Resource group information.
 */
struct ResGroupData
{
	Oid				groupId;			/* ID for this group */
	volatile int	nRunning;			/* number of running trans */
	volatile int	nRunningBypassed;	/* number of running trans in bypass mode */
	int64			totalExecuted;		/* total number of executed trans */
	int64			totalQueued;		/* total number of queued trans	*/
	int64			totalQueuedTimeMs;	/* total queue time, in milliseconds */
	PROC_QUEUE		waitProcs;			/* list of PGPROC objects waiting on this group */
	bool			lockedForDrop;  	/* true if resource group is dropped but not committed yet */
	ResGroupCaps	caps;				/* capabilities of this group */
};

struct ResGroupControl
{
	int 			segmentsOnCoordinator;
	ResGroupSlotData	*slots;		/* slot pool shared by all resource groups */
	ResGroupSlotData	*freeSlot;	/* head of the free list */
	HTAB			*htbl;

	/*
	 * The hash table for resource groups in shared memory should only be populated
	 * once, so we add a flag here to implement this requirement.
	 */
	bool			loaded;
	int				nGroups;
	ResGroupData	groups[1];
};

bool pg_resource_group_enable_cgroup_cpuset = false;
CGroupOpsRoutine *cgroupOpsRoutine = NULL;
CGroupSystemInfo *cgroupSystemInfo = NULL;

/* Number of entries in the SessionStateArray */
static int SessionStateArrayEntryCount = 0;

/* static variables */
static ResGroupControl *pResGroupControl = NULL;

static ResGroupProcData __self =
{
	InvalidOid,
};
static ResGroupProcData *self = &__self;

/* If we are waiting on a group, this points to the associated group */
static ResGroupData *groupAwaited = NULL;
static TimestampTz groupWaitStart;
static TimestampTz groupWaitEnd;
/* the resource group self is running in bypass mode */
static ResGroupData *bypassedGroup = NULL;
/* a fake slot used in bypass mode */
static ResGroupSlotData bypassedSlot;

/* static functions */
static void wakeupSlots(ResGroupData *group, bool grant);
static ResGroupData *groupHashNew(Oid groupId);
static ResGroupData *groupHashFind(Oid groupId, bool raise);
static ResGroupData *groupHashRemove(Oid groupId);
static void waitOnGroup(ResGroupData *group, bool isMoveQuery);
static ResGroupData *createGroup(Oid groupId, const ResGroupCaps *caps);
static void removeGroup(Oid groupId);
static void AtProcExit_ResGroup(int code, Datum arg);
static void groupWaitCancel(bool isMoveQuery);
static void initSlot(ResGroupSlotData *slot, ResGroupData *group);
static void selfAttachResGroup(ResGroupData *group, ResGroupSlotData *slot);
static void selfDetachResGroup(ResGroupData *group, ResGroupSlotData *slot);
static bool slotpoolInit(void);
static ResGroupSlotData *slotpoolAllocSlot(void);
static void slotpoolFreeSlot(ResGroupSlotData *slot);
static ResGroupSlotData *groupGetSlot(ResGroupData *group);
static void groupPutSlot(ResGroupData *group, ResGroupSlotData *slot);
static Oid decideResGroupId(void);
static void decideResGroup(ResGroupInfo *pGroupInfo);
static bool groupIncBypassedRef(ResGroupInfo *pGroupInfo);
static void groupDecBypassedRef(ResGroupData *group);
static ResGroupSlotData *groupAcquireSlot(ResGroupInfo *pGroupInfo, bool isMoveQuery);
static void groupReleaseSlot(ResGroupData *group, ResGroupSlotData *slot, bool isMoveQuery);
static void addTotalQueueDuration(ResGroupData *group);
static void selfValidateResGroupInfo(void);
static bool selfIsAssigned(void);
static void selfSetGroup(ResGroupData *group);
static void selfUnsetGroup(void);
static void selfSetSlot(ResGroupSlotData *slot);
static void selfUnsetSlot(void);
static bool procIsWaiting(const PGPROC *proc);
static void procWakeup(PGPROC *proc);
static void groupWaitQueueValidate(const ResGroupData *group);
static void groupWaitProcValidate(PGPROC *proc, PROC_QUEUE *head);
static void groupWaitQueuePush(ResGroupData *group, PGPROC *proc);
static PGPROC *groupWaitQueuePop(ResGroupData *group);
static void groupWaitQueueErase(ResGroupData *group, PGPROC *proc);
static bool groupWaitQueueIsEmpty(const ResGroupData *group);
static bool checkBypassWalker(Node *node, void *context);
static bool shouldBypassSelectQuery(Node *node);
static bool shouldBypassQuery(const char *query_string);
static void lockResGroupForDrop(ResGroupData *group);
static void unlockResGroupForDrop(ResGroupData *group);
static bool groupIsDropped(ResGroupInfo *pGroupInfo);
static void sessionSetSlot(ResGroupSlotData *slot);
static void sessionResetSlot(ResGroupSlotData *slot);
static void cpusetOperation(char *cpuset1,
							const char *cpuset2,
							int len,
							bool sub);
#ifdef USE_ASSERT_CHECKING
static bool selfHasGroup(void);
static bool selfHasSlot(void);
static void slotValidate(const ResGroupSlotData *slot);
static bool slotIsInFreelist(const ResGroupSlotData *slot);
static bool slotIsInUse(const ResGroupSlotData *slot);
static bool groupIsNotDropped(const ResGroupData *group);
static bool groupWaitQueueFind(ResGroupData *group, const PGPROC *proc);
#endif /* USE_ASSERT_CHECKING */

static bool is_pure_catalog_plan(PlannedStmt *stmt);
static bool can_bypass_based_on_plan_cost(PlannedStmt *stmt);


/*
 * Estimate size the resource group structures will need in
 * shared memory.
 *
 */
Size
ResGroupShmemSize(void)
{
	Size		size = 0;

	/* The hash of groups. */
	size = hash_estimate_size(MaxResourceGroups, sizeof(ResGroupHashEntry));
	/* The control structure. */
	size = add_size(size, sizeof(ResGroupControl) - sizeof(ResGroupData));
	/* The control structure. */
	size = add_size(size, mul_size(MaxResourceGroups, sizeof(ResGroupData)));
	/* The slot pool. */
	size = add_size(size, mul_size(RESGROUP_MAX_SLOTS, sizeof(ResGroupSlotData)));
	/* Add a safety margin */
	size = add_size(size, size / 10);
	return size;
}

void
ResManagerShmemInit(void)
{
	if (IsResGroupEnabled() && !IsUnderPostmaster)
	{
		ResGroupControlInit();
	}
}

void
InitResManager(void)
{
	if  (IsResGroupEnabled() &&
			 IsUnderPostmaster )
	{
		/*
		 * InitResManager() is called under PostgresMain(), so resource group is not
		 * initialized for auxiliary processes and other special backends. eg
		 * checkpointer, ftsprobe and filerep processes. Wal sender acts like a backend,
		 * so we also need to exclude it.
		 */
		InitResGroups();
		cgroupOpsRoutine->adjustgucs();
		ResGroupActivated = true;
	}
}

/*
 * Initialize the global ResGroupControl struct of resource groups.
 */
void
ResGroupControlInit(void)
{
	int			i;
    bool        found;
    HASHCTL     info;
    int         hash_flags;
	int			size;

	size = sizeof(*pResGroupControl) - sizeof(ResGroupData);
	size += mul_size(MaxResourceGroups, sizeof(ResGroupData));

    pResGroupControl = ShmemInitStruct("global resource group control",
                                       size, &found);
    if (found)
        return;
    if (pResGroupControl == NULL)
        goto error_out;
    /* Set key and entry sizes of hash table */
    MemSet(&info, 0, sizeof(info));
    info.keysize = sizeof(Oid);
    info.entrysize = sizeof(ResGroupHashEntry);
    info.hash = tag_hash;

    hash_flags = (HASH_ELEM | HASH_FUNCTION);
    elog(LOG, "creating hash table for %d resource groups", MaxResourceGroups);
    pResGroupControl->htbl = ShmemInitHash("Resource Group Hash Table",
                                           MaxResourceGroups,
                                           MaxResourceGroups,
                                           &info, hash_flags);

    if (!pResGroupControl->htbl)
        goto error_out;

    /*
     * No need to acquire LWLock here, since this is expected to be called by
     * postmaster only
     */
    pResGroupControl->loaded = false;
    pResGroupControl->nGroups = MaxResourceGroups;
	for (i = 0; i < MaxResourceGroups; i++)
		pResGroupControl->groups[i].groupId = InvalidOid;

	if (!slotpoolInit())
		goto error_out;

    return;

error_out:
	ereport(FATAL,
			(errcode(ERRCODE_OUT_OF_MEMORY),
			 errmsg("not enough shared memory for resource group control")));
}

/*
 * Allocate a resource group entry from a hash table
 */
void
AllocResGroupEntry(Oid groupId, const ResGroupCaps *caps)
{
	ResGroupData	*group;

	LWLockAcquire(ResGroupLock, LW_EXCLUSIVE);
	group = createGroup(groupId, caps);
	Assert(group != NULL);
	LWLockRelease(ResGroupLock);
}

void
initCgroup(void)
{
	bool probe_result = false;
#ifdef __linux__
	if (pg_resource_manager_policy == RESOURCE_MANAGER_POLICY_GROUP)
	{
		cgroupOpsRoutine = get_group_routine_v1();
		cgroupSystemInfo = get_cgroup_sysinfo_v1();
		ereport(LOG,
				(errmsg("get group routine.")));
	}
	else
	{
		cgroupOpsRoutine = get_group_routine_v2();
		cgroupSystemInfo = get_cgroup_sysinfo_v2();
	}
#else
	cgroupOpsRoutine = get_cgroup_routine_dummy();
	cgroupSystemInfo = get_cgroup_sysinfo_dummy();
#endif

	probe_result = cgroupOpsRoutine->probecgroup();
	if (!probe_result)
	{
		ereport(LOG,
				(errmsg("The control group is not well configured, please check your system configuration.")));
	}
	cgroupOpsRoutine->checkcgroup();
	cgroupOpsRoutine->initcgroup();
}

/*
 * Load the resource groups in shared memory. Note this
 * can only be done after enough setup has been done. This uses
 * heap_open etc which in turn requires shared memory to be set up.
 */
void
InitResGroups(void)
{
	HeapTuple	tuple;
	SysScanDesc	sscan;
	int			numGroups;
	ResGroupCaps		caps;
	Relation			relResGroup;
	Relation			relResGroupCapability;
	char		cpuset[MaxCpuSetLength] = {0};
	int			defaultCore = -1;
	Bitmapset	*bmsUnused = NULL;

	on_shmem_exit(AtProcExit_ResGroup, 0);

	if (pResGroupControl->loaded)
		return;

	/*
	 * The resgroup shared mem initialization must be serialized. Only the first session
	 * should do the init.
	 * Serialization is done by LW_EXCLUSIVE ResGroupLock. However, we must obtain all DB
	 * locks before obtaining LWlock to prevent deadlock.
	 */
	relResGroup = table_open(ResGroupRelationId, AccessShareLock);
	relResGroupCapability = table_open(ResGroupCapabilityRelationId, AccessShareLock);
	LWLockAcquire(ResGroupLock, LW_EXCLUSIVE);
	
	if (pResGroupControl->loaded)
		goto exit;

	if (pg_resource_group_enable_cgroup_cpuset)
	{
		/* Get cpuset from cpuset BASEDIR_HGDB, and transform it into bitset */
		cgroupOpsRoutine->getcpuset(CGROUP_ROOT_ID, cpuset, MaxCpuSetLength);
		bmsUnused = CpusetToBitset(cpuset, MaxCpuSetLength);
		/* get the minimum core number, in case of the zero core is not exist */
		defaultCore = bms_next_member(bmsUnused, -1);
		Assert(defaultCore >= 0);
	}
	
	numGroups = 0;
	sscan = systable_beginscan(relResGroup, InvalidOid, false, NULL, 0, NULL);
	while (HeapTupleIsValid(tuple = systable_getnext(sscan)))
	{
		Oid			groupId = ((Form_pg_resgroup) GETSTRUCT(tuple))->oid;
		ResGroupData	*group;

		Bitmapset *bmsCurrent;

		GetResGroupCapabilities(relResGroupCapability, groupId, &caps);
		group = createGroup(groupId, &caps);
		Assert(group != NULL);

		cgroupOpsRoutine->createcgroup(groupId);

		if (caps.io_limit != NIL)
		{
			cgroupOpsRoutine->setio(groupId, caps.io_limit);
			cgroupOpsRoutine->freeio(caps.io_limit);
		}

		if (CpusetIsEmpty(caps.cpuset))
		{
			cgroupOpsRoutine->setcpulimit(groupId, caps.cpuMaxPercent);
			cgroupOpsRoutine->setcpuweight(groupId, caps.cpuWeight);
		}
		else
		{
			Bitmapset *bmsCommon = NULL;
			Bitmapset *bmsMissing = NULL;
			char *cpuset = getCpuSetByRole(caps.cpuset);

			bmsCurrent = CpusetToBitset(cpuset, MaxCpuSetLength);
			bmsCommon = bms_intersect(bmsCurrent, bmsUnused);
			bmsMissing = bms_difference(bmsCurrent, bmsCommon);

			/*
			 * Do not call EnsureCpusetIsAvailable() here as resource group is
			 * not activated yet
			 */
			if (!pg_resource_group_enable_cgroup_cpuset)
			{
				ereport(WARNING,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						 errmsg("cgroup is not properly configured to use the cpuset feature"),
						 errhint("Extra cgroup configurations are required to enable this feature, "
								 "please refer to the Greenplum Documentations for details")));
			}

			Assert(caps.cpuMaxPercent == CPU_MAX_PERCENT_DISABLED);

			if (bms_is_empty(bmsMissing))
			{
				/*
				 * write cpus to corresponding file
				 * if all the cores are available
				 */
				char *cpuset= getCpuSetByRole(caps.cpuset);
				cgroupOpsRoutine->setcpuset(groupId, cpuset);
				bmsUnused = bms_del_members(bmsUnused, bmsCurrent);
			}
			else
			{
				char		cpusetMissing[MaxCpuSetLength] = {0};

				/*
				 * if some of the cores are unavailable, just set defaultCore
				 * to this group and send a warning message, so the system
				 * can startup, then DBA can fix it
				 */
				snprintf(cpuset, MaxCpuSetLength, "%d", defaultCore);
				cgroupOpsRoutine->setcpuset(groupId, cpuset);
				BitsetToCpuset(bmsMissing, cpusetMissing, MaxCpuSetLength);
				ereport(WARNING,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						 errmsg("cpu cores %s are unavailable on the system "
								"in resource group %s",
								cpusetMissing, GetResGroupNameForId(groupId)),
						 errhint("using core %d for this resource group, "
								 "please adjust the settings and restart",
								 defaultCore)));
			}
		}

		numGroups++;
		Assert(numGroups <= MaxResourceGroups);
	}
	systable_endscan(sscan);

	if (pg_resource_group_enable_cgroup_cpuset)
	{
		/*
		 * set default cpuset
		 */
		if (bms_is_empty(bmsUnused))
		{
			/* no unused core, assign default core to default group */
			snprintf(cpuset, MaxCpuSetLength, "%d", defaultCore);
		}
		else
		{
			/* assign all unused cores to default group */
			BitsetToCpuset(bmsUnused, cpuset, MaxCpuSetLength);
		}

		Assert(cpuset[0]);
		Assert(!CpusetIsEmpty(cpuset));

		cgroupOpsRoutine->setcpuset(DEFAULT_CPUSET_GROUP_ID, cpuset);
	}

	pResGroupControl->loaded = true;
	elog(LOG, "initialized %d resource groups", numGroups);

exit:
	LWLockRelease(ResGroupLock);

	/*
	 * release lock here to guarantee we have no lock held when acquiring
	 * resource group slot
	 */
	table_close(relResGroup, AccessShareLock);
	table_close(relResGroupCapability, AccessShareLock);
}

/* Allocates the shared memory SessionStateArray */
void
SessionState_ShmemInit()
{
	bool	found = false;

	Size shmemSize = SessionState_ShmemSize();
	AllSessionStateEntries = (SessionStateArray *)
				ShmemInitStruct(SHMEM_SESSION_STATE_ARRAY, shmemSize, &found);

	Assert(found || !IsUnderPostmaster);

	if (!IsUnderPostmaster)
	{
		SessionState *prev = NULL;
		MemSet(AllSessionStateEntries, 0, shmemSize);

		/*
		 * We're the first - initialize.
		 */
		AllSessionStateEntries->numSession = 0;
		AllSessionStateEntries->maxSession = SessionStateArrayEntryCount;

		/* Every entry of the array is free at this time */
		AllSessionStateEntries->freeList = (SessionState *) &AllSessionStateEntries->sessions[0];
		AllSessionStateEntries->usedList = NULL;

		/*
		 * Set all the entries' sessionId to invalid. Also, set the next pointer
		 * to point to the next entry in the array.
		 */
		prev = (SessionState *) &AllSessionStateEntries->sessions[0];
		prev->pid = INVALID_SESSION_PID;

		for (int i = 1; i < AllSessionStateEntries->maxSession; i++)
		{
			SessionState *cur = (SessionState *) &AllSessionStateEntries->sessions[i];

			cur->pid = INVALID_SESSION_PID;
			prev->next = cur;

			prev = cur;
		}

		prev->next = NULL;
	}
}

/* Returns the size of the SessionState array */
Size
SessionState_ShmemSize()
{
	Size size = offsetof(SessionStateArray, sessions);
	SessionStateArrayEntryCount = MaxBackends;
	size = add_size(size, mul_size(sizeof(SessionState),
			SessionStateArrayEntryCount));

	return size;
}

/* Returns the size of the shared memory to allocate SessionStateArray */
/*
 * Grabs one entry in the sessionStateArray for current session.
 * If the current session already has an entry, it just returns the
 * pointer to the previously grabbed entry.
 */
static SessionState*
SessionState_Acquire(int sessionPid)
{
	SessionState *cur = NULL;
	SessionState *acquired = NULL;
	LWLockAcquire(SessionStateLock, LW_EXCLUSIVE);
	cur = AllSessionStateEntries->usedList;
	
	while (cur != NULL && cur->pid != sessionPid)
	{
		Assert(INVALID_SESSION_PID != cur->pid);
		cur = cur->next;
	}
	if (NULL == cur && NULL == AllSessionStateEntries->freeList)
	{
		LWLockRelease(SessionStateLock);
		ereport(FATAL,
				(errcode(ERRCODE_TOO_MANY_CONNECTIONS),
						errmsg("too many sessions"),
						errdetail("Could not acquire resources for additional sessions."),
						errhint("Disconnect some sessions and try again.")));
	}

	acquired = cur;
	/*
	 * Nothing was acquired for this session from any other processes. Therefore,
	 * acquire a new entry, and reset its properties.
	 */
	if (NULL == acquired)
	{
		acquired = AllSessionStateEntries->freeList;

		AllSessionStateEntries->freeList = acquired->next;

		acquired->next = AllSessionStateEntries->usedList;
		AllSessionStateEntries->usedList = acquired;
		AllSessionStateEntries->numSession++;
		Assert(AllSessionStateEntries->numSession <= AllSessionStateEntries->maxSession);

		acquired->pid = sessionPid;
		acquired->resGroupSlot = NULL;
	}

	LWLockRelease(SessionStateLock);

	return acquired;
}

/*
 * Releases the pinCount of a SessionState entry. If the pinCount
 * drops to 0, it puts the entry back to the freeList for reuse.
 */
static void
SessionState_Release(SessionState *acquired)
{
	SessionState *cur = NULL;
	SessionState *prev = NULL;

	if (!sessionStateInited)
	{
		Assert(NULL == acquired);
		return;
	}
	Assert(NULL != acquired);
	LWLockAcquire(SessionStateLock, LW_EXCLUSIVE);
	acquired->pid = INVALID_SESSION_PID;
	acquired->resGroupSlot = NULL;
	cur = AllSessionStateEntries->usedList;

	while (cur != acquired && cur != NULL)
	{
		prev = cur;
		cur = cur->next;
	}

	Assert(cur == acquired);

	/* grabbed is at the head of used list */
	if (NULL == prev)
	{
		Assert(AllSessionStateEntries->usedList == acquired);
		AllSessionStateEntries->usedList = acquired->next;
	}
	else
	{
		prev->next = cur->next;
	}

	acquired->next = AllSessionStateEntries->freeList;
	AllSessionStateEntries->freeList = acquired;
	AllSessionStateEntries->numSession--;
	Assert(AllSessionStateEntries->numSession >= 0);
	
	LWLockRelease(SessionStateLock);
}
/* Initialize the SessionState for current session */
void
SessionState_Init()
{
	Assert(NULL == MySessionState);

	Assert(!sessionStateInited);

	MySessionState = SessionState_Acquire(MyProcPid);

	Assert(NULL != MySessionState);

	sessionStateInited = true;
}

/* Shutdown the SessionState for current session */
void
SessionState_Shutdown()
{
	Assert(INVALID_SESSION_PID == MyProcPid || NULL != MySessionState);

	Assert(INVALID_SESSION_PID == MyProcPid || sessionStateInited);

	SessionState_Release((SessionState *)MySessionState);
	MySessionState = NULL;
	sessionStateInited = false;
}
/*
 * get resource group id by session id
 */
Oid
ResGroupGetGroupIdBySessionPid(int sessionPid)
{
	Oid groupId = InvalidOid;
	SessionState *curSessionState;

	LWLockAcquire(SessionStateLock, LW_SHARED);
	curSessionState = AllSessionStateEntries->usedList;
	while (curSessionState != NULL)
	{
		if (curSessionState->pid == sessionPid)
		{
			ResGroupSlotData *slot = (ResGroupSlotData *)curSessionState->resGroupSlot;
			if (slot != NULL)
				groupId = slot->groupId;
			break;
		}
		curSessionState = curSessionState->next;
	}
	LWLockRelease(SessionStateLock);

	return groupId;
}


/*
 * Check resource group status when DROP RESOURCE GROUP
 *
 * Errors out if there're running transactions, otherwise lock the resource group.
 * New transactions will be queued if the resource group is locked.
 */
void
ResGroupCheckForDrop(Oid groupId, char *name)
{
	ResGroupData	*group;

	LWLockAcquire(ResGroupLock, LW_EXCLUSIVE);
	group = groupHashFind(groupId, true);
	if (group->nRunning + group->nRunningBypassed > 0)
	{
		int nQuery = group->nRunning + group->nRunningBypassed + group->waitProcs.size;

		Assert(name != NULL);
		ereport(ERROR,
				(errcode(ERRCODE_DEPENDENT_OBJECTS_STILL_EXIST),
				 errmsg("cannot drop resource group \"%s\"", name),
				 errhint(" The resource group is currently managing %d query(ies) and cannot be dropped.\n"
						 "\tTerminate the queries first or try dropping the group later.\n"
						 "\tThe view pg_stat_activity tracks the queries managed by resource groups.", nQuery)));
	}

	lockResGroupForDrop(group);
	LWLockRelease(ResGroupLock);
}

/*
 * Drop resource group call back function
 *
 * Wake up the backends in the wait queue when DROP RESOURCE GROUP finishes.
 * Unlock the resource group if the transaction is aborted.
 * Remove the resource group entry in shared memory if the transaction is committed.
 *
 * This function is called in the callback function of DROP RESOURCE GROUP.
 */
void
ResGroupDropFinish(const ResourceGroupCallbackContext *callbackCtx,
				   bool isCommit)
{
	ResGroupData	*group;
	volatile int	savedInterruptHoldoffCount;

	LWLockAcquire(ResGroupLock, LW_EXCLUSIVE);

	PG_TRY();
	{
		savedInterruptHoldoffCount = InterruptHoldoffCount;
		group = groupHashFind(callbackCtx->groupid, true);
		wakeupSlots(group, false);
		unlockResGroupForDrop(group);

		if (isCommit)
		{
			removeGroup(callbackCtx->groupid);
			if (!CpusetIsEmpty(group->caps.cpuset))
			{
				if (pg_resource_group_enable_cgroup_cpuset)
				{
					/* reset default group, add cpu cores to it */
					char cpuset[MaxCpuSetLength];
					cgroupOpsRoutine->getcpuset(DEFAULT_CPUSET_GROUP_ID,
										  cpuset, MaxCpuSetLength);
					CpusetUnion(cpuset, group->caps.cpuset, MaxCpuSetLength);
					cgroupOpsRoutine->setcpuset(DEFAULT_CPUSET_GROUP_ID, cpuset);
				}
			}

			cgroupOpsRoutine->destroycgroup(callbackCtx->groupid, true);
		}
	}
	PG_CATCH();
	{
		InterruptHoldoffCount = savedInterruptHoldoffCount;
		if (elog_demote(WARNING))
		{
			EmitErrorReport();
			FlushErrorState();
		}
		else
		{
			elog(LOG, "unable to demote error");
		}
	}
	PG_END_TRY();

	LWLockRelease(ResGroupLock);
}

/*
 * Remove the resource group entry in shared memory if the transaction is aborted.
 *
 * This function is called in the callback function of CREATE RESOURCE GROUP.
 */
void
ResGroupCreateOnAbort(const ResourceGroupCallbackContext *callbackCtx)
{
	volatile int savedInterruptHoldoffCount;

	LWLockAcquire(ResGroupLock, LW_EXCLUSIVE);
	PG_TRY();
	{
		savedInterruptHoldoffCount = InterruptHoldoffCount;
		removeGroup(callbackCtx->groupid);
		/* remove the os dependent part for this resource group */
		cgroupOpsRoutine->destroycgroup(callbackCtx->groupid, true);

		if (!CpusetIsEmpty(callbackCtx->caps.cpuset) &&
			pg_resource_group_enable_cgroup_cpuset)
		{
			/* return cpu cores to default group */
			char defaultGroupCpuset[MaxCpuSetLength];
			cgroupOpsRoutine->getcpuset(DEFAULT_CPUSET_GROUP_ID,
								  defaultGroupCpuset,
								  MaxCpuSetLength);
			CpusetUnion(defaultGroupCpuset,
						callbackCtx->caps.cpuset,
						MaxCpuSetLength);
			cgroupOpsRoutine->setcpuset(DEFAULT_CPUSET_GROUP_ID, defaultGroupCpuset);
		}
	}
	PG_CATCH();
	{
		InterruptHoldoffCount = savedInterruptHoldoffCount;
		if (elog_demote(WARNING))
		{
			EmitErrorReport();
			FlushErrorState();
		}
		else
		{
			elog(LOG, "unable to demote error");
		}
	}
	PG_END_TRY();
	LWLockRelease(ResGroupLock);
}

/*
 * Apply the new resgroup caps.
 */
void
ResGroupAlterOnCommit(const ResourceGroupCallbackContext *callbackCtx)
{
	ResGroupData	*group;
	volatile int	savedInterruptHoldoffCount;

	LWLockAcquire(ResGroupLock, LW_EXCLUSIVE);

	PG_TRY();
	{
		savedInterruptHoldoffCount = InterruptHoldoffCount;
		group = groupHashFind(callbackCtx->groupid, true);

		group->caps = callbackCtx->caps;

		if (callbackCtx->limittype == RESGROUP_LIMIT_TYPE_CPU)
		{
			char *cpuset = NULL;
			cgroupOpsRoutine->setcpulimit(callbackCtx->groupid,
										callbackCtx->caps.cpuMaxPercent);

			/* We should set cpuset to the default value */
			cpuset = (char *) palloc(MaxCpuSetLength);
			sprintf(cpuset, "0-%d", cgroupSystemInfo->ncores-1);
			cgroupOpsRoutine->setcpuset(callbackCtx->groupid, cpuset);
		}
		else if (callbackCtx->limittype == RESGROUP_LIMIT_TYPE_CPU_SHARES)
		{
			cgroupOpsRoutine->setcpuweight(callbackCtx->groupid,
										   callbackCtx->caps.cpuWeight);
		}
		else if (callbackCtx->limittype == RESGROUP_LIMIT_TYPE_CPUSET)
		{
			if (pg_resource_group_enable_cgroup_cpuset)
			{
				char *cpuset = getCpuSetByRole(callbackCtx->caps.cpuset);
				cgroupOpsRoutine->setcpuset(callbackCtx->groupid,
									        cpuset);
			}
		}
		else if (callbackCtx->limittype == RESGROUP_LIMIT_TYPE_CONCURRENCY)
		{
			wakeupSlots(group, true);
		}
		else if (callbackCtx->limittype == RESGROUP_LIMIT_TYPE_IO_LIMIT)
		{
			if (callbackCtx->caps.io_limit != NIL)
			{
				cgroupOpsRoutine->cleario(callbackCtx->groupid);
				cgroupOpsRoutine->setio(callbackCtx->groupid, callbackCtx->caps.io_limit);
			}
		}

		/* reset default group if cpuset has changed */
		if (strcmp(callbackCtx->oldCaps.cpuset, callbackCtx->caps.cpuset) &&
			pg_resource_group_enable_cgroup_cpuset)
		{
			char *cpuset = NULL;
			char *oldcpuset = NULL;
			char defaultCpusetGroup[MaxCpuSetLength];
			/* get current default group value */
			cgroupOpsRoutine->getcpuset(DEFAULT_CPUSET_GROUP_ID,
								  defaultCpusetGroup,
								  MaxCpuSetLength);
			/* Add old value to default group
			 * sub new value from default group */
			cpuset = getCpuSetByRole(callbackCtx->caps.cpuset);
			oldcpuset = getCpuSetByRole(callbackCtx->oldCaps.cpuset);
			CpusetUnion(defaultCpusetGroup,
						oldcpuset,
						MaxCpuSetLength);
			CpusetDifference(defaultCpusetGroup,
						cpuset,
						MaxCpuSetLength);
			cgroupOpsRoutine->setcpuset(DEFAULT_CPUSET_GROUP_ID, defaultCpusetGroup);
		}
	}
	PG_CATCH();
	{
		InterruptHoldoffCount = savedInterruptHoldoffCount;
		if (elog_demote(WARNING))
		{
			EmitErrorReport();
			FlushErrorState();
		}
		else
		{
			elog(LOG, "unable to demote error");
		}
	}
	PG_END_TRY();

	LWLockRelease(ResGroupLock);
}

bool
ResGroupIsAssigned(void)
{
	return selfIsAssigned();
}

/*
 * Get resource group id of my proc.
 *
 * Returns InvalidOid in any of below cases:
 * - resource group is not enabled;
 * - resource group is not activated (initialized);
 * - my proc is not running inside a transaction;
 * - my proc is not assigned a resource group yet;
 *
 * Otherwise a valid resource group id is returned.
 *
 */
Oid
GetMyResGroupId(void)
{
	return self->groupId;
}

/*
 *  Retrieve statistic information of type from resource group
 */
Datum
ResGroupGetStat(Oid groupId, ResGroupStatType type)
{
	ResGroupData *group;
	Interval   *interval;
	Datum		result;

	Assert(IsResGroupActivated());

	LWLockAcquire(ResGroupLock, LW_SHARED);
	group = groupHashFind(groupId, true);
	switch (type)
	{
		case RES_GROUP_STAT_NRUNNING:
			result = Int32GetDatum(group->nRunning + group->nRunningBypassed);
			break;
		case RES_GROUP_STAT_NQUEUEING:
			result = Int32GetDatum(group->waitProcs.size);
			break;
		case RES_GROUP_STAT_TOTAL_EXECUTED:
			result = Int64GetDatum(group->totalExecuted);
			break;
		case RES_GROUP_STAT_TOTAL_QUEUED:
			result = Int64GetDatum(group->totalQueued);
			break;
		case RES_GROUP_STAT_TOTAL_QUEUE_TIME:
			/*
			 * Turn milliseconds in totalQueuedTimeMs into an Interval.
			 *
			 * Note: we only use the 'time' field. The user can call
			 * justify_interval() if she wants.
			 */
			interval = (Interval *) palloc(sizeof(Interval));
			interval->time = group->totalQueuedTimeMs;
			interval->day = 0;
			interval->month = 0;
			result = IntervalPGetDatum(interval);
			break;
		default:
			ereport(ERROR,
					(errcode(ERRCODE_INTERNAL_ERROR),
					 errmsg("invalid stat type %d", type)));
	}
	LWLockRelease(ResGroupLock);
	return result;
}

/*
 * removeGroup -- remove resource group from share memory and
 * reclaim the group's memory back to MEM POOL.
 */
static void
removeGroup(Oid groupId)
{
	ResGroupData *group;

	Assert(LWLockHeldByMeInMode(ResGroupLock, LW_EXCLUSIVE));
	Assert(OidIsValid(groupId));

	group = groupHashRemove(groupId);
	group->groupId = InvalidOid;
}

/*
 * createGroup -- initialize the elements for a resource group.
 *
 * Notes:
 *	It is expected that the appropriate lightweight lock is held before
 *	calling this - unless we are the startup process.
 */
static ResGroupData *
createGroup(Oid groupId, const ResGroupCaps *caps)
{
	ResGroupData	*group;

	Assert(LWLockHeldByMeInMode(ResGroupLock, LW_EXCLUSIVE));
	Assert(OidIsValid(groupId));

	group = groupHashNew(groupId);
	Assert(group != NULL);

	group->groupId = groupId;
	group->caps = *caps;
	/* remove local pointers */
	group->caps.io_limit = NIL;
	group->nRunning = 0;
	group->nRunningBypassed = 0;
	ProcQueueInit(&group->waitProcs);
	group->totalExecuted = 0;
	group->totalQueued = 0;
	group->totalQueuedTimeMs = 0;
	group->lockedForDrop = false;

	return group;
}

/*
 * Attach a process to a slot.
 */
static void
selfAttachResGroup(ResGroupData *group, ResGroupSlotData *slot)
{
	selfSetGroup(group);
	selfSetSlot(slot);
	pg_atomic_add_fetch_u32((pg_atomic_uint32*) &slot->nProcs, 1);
}

/*
 * Detach a process from a slot.
 */
static void
selfDetachResGroup(ResGroupData *group, ResGroupSlotData *slot)
{
	pg_atomic_sub_fetch_u32((pg_atomic_uint32*) &slot->nProcs, 1);
	selfUnsetSlot();
	selfUnsetGroup();
}

/*
 * Initialize the members of a slot
 */
static void
initSlot(ResGroupSlotData *slot, ResGroupData *group)
{
	Assert(LWLockHeldByMeInMode(ResGroupLock, LW_EXCLUSIVE));
	Assert(!slotIsInUse(slot));
	Assert(group->groupId != InvalidOid);

	slot->group = group;
	slot->groupId = group->groupId;
	slot->caps = group->caps;
}

/*
 * Alloc and initialize slot pool
 */
static bool
slotpoolInit(void)
{
	ResGroupSlotData *slot;
	ResGroupSlotData *next;
	int numSlots;
	int memSize;
	int i;

	numSlots = RESGROUP_MAX_SLOTS;
	memSize = mul_size(numSlots, sizeof(ResGroupSlotData));
	pResGroupControl->slots = ShmemAlloc(memSize);
	if (!pResGroupControl->slots)
		return false;
	MemSet(pResGroupControl->slots, 0, memSize);
	/* push all the slots into the list */
	next = NULL;
	for (i = numSlots - 1; i >= 0; i--)
	{
		slot = &pResGroupControl->slots[i];

		slot->group = NULL;
		slot->groupId = InvalidOid;

		slot->next = next;
		next = slot;
	}
	pResGroupControl->freeSlot = next;

	return true;
}

/*
 * Alloc a slot from shared slot pool
 */
static ResGroupSlotData *
slotpoolAllocSlot(void)
{
	ResGroupSlotData *slot;

	Assert(LWLockHeldByMeInMode(ResGroupLock, LW_EXCLUSIVE));
	Assert(pResGroupControl->freeSlot != NULL);

	slot = pResGroupControl->freeSlot;
	pResGroupControl->freeSlot = slot->next;
	slot->next = NULL;
	return slot;
}

/*
 * Free a slot back to shared slot pool
 */
static void
slotpoolFreeSlot(ResGroupSlotData *slot)
{
	Assert(LWLockHeldByMeInMode(ResGroupLock, LW_EXCLUSIVE));
	Assert(slotIsInUse(slot));
	Assert(slot->nProcs == 0);

	slot->group = NULL;
	slot->groupId = InvalidOid;
	slot->next = pResGroupControl->freeSlot;
	pResGroupControl->freeSlot = slot;
}

/*
 * Get a slot.
 *
 * A slot can be got with this function the concurrency limit is not reached.
 *
 * On success the nRunning is increased and the slot's groupId is also set
 * accordingly, the slot id is returned.
 *
 * On failure nothing is changed and InvalidSlotId is returned.
 */
static ResGroupSlotData *
groupGetSlot(ResGroupData *group)
{
	ResGroupSlotData	*slot;
	ResGroupCaps		*caps;

	Assert(LWLockHeldByMeInMode(ResGroupLock, LW_EXCLUSIVE));

	Assert(groupIsNotDropped(group));

	caps = &group->caps;
	/* First check if the concurrency limit is reached */
	if (group->nRunning >= caps->concurrency)
		return NULL;
	/* Now actually get a free slot */
	slot = slotpoolAllocSlot();
	Assert(!slotIsInUse(slot));
	initSlot(slot, group);
	group->nRunning++;
	return slot;
}

/*
 * Put back the slot assigned to self.
 *
 * This will release a slot, its nRunning will be decreased.
 */
static void
groupPutSlot(ResGroupData *group, ResGroupSlotData *slot)
{
	Assert(LWLockHeldByMeInMode(ResGroupLock, LW_EXCLUSIVE));
	Assert(slotIsInUse(slot));

	/* Return the slot back to free list */
	slotpoolFreeSlot(slot);
	group->nRunning--;
}

/*
 * Pick a resource group for the current transaction.
 */
static Oid
decideResGroupId(void)
{
	Oid groupId = InvalidOid;

	if (groupId == InvalidOid)
		groupId = getResGroupIdByPgCapability(GetUserId());

	return groupId;
}

/*
 * Decide the proper resource group for current role.
 *
 * An exception is thrown if current role is invalid.
 */
static void
decideResGroup(ResGroupInfo *pGroupInfo)
{
	ResGroupData	*group;
	Oid				 groupId;
	/*
	Assert(pResGroupControl != NULL);
	Assert(pResGroupControl->segmentsOnCoordinator > 0);
	*/
	
	/* always find out the up-to-date resgroup id */
	groupId = decideResGroupId();
	LWLockAcquire(ResGroupLock, LW_SHARED);
	group = groupHashFind(groupId, false);
	if (!group)
	{
		groupId = superuser() ? ADMINRESGROUP_OID : DEFAULTRESGROUP_OID;
		group = groupHashFind(groupId, false);
	}

	Assert(group != NULL);
	LWLockRelease(ResGroupLock);
	pGroupInfo->group = group;
	pGroupInfo->groupId = groupId;
}

/*
 * Increase the bypassed ref count
 *
 * Return true if the operation is done, or false if the group is dropped.
 */
static bool
groupIncBypassedRef(ResGroupInfo *pGroupInfo)
{
	ResGroupData	*group = pGroupInfo->group;
	bool			result = false;

	LWLockAcquire(ResGroupLock, LW_EXCLUSIVE);
	/* Has the group been dropped? */
	if (groupIsDropped(pGroupInfo))
		goto end;
	/* Is the group locked for drop? */
	if (group->lockedForDrop)
		goto end;
	result = true;
	pg_atomic_add_fetch_u32((pg_atomic_uint32 *) &group->nRunningBypassed, 1);

end:
	LWLockRelease(ResGroupLock);
	return result;
}

/*
 * Decrease the bypassed ref count
 */
static void
groupDecBypassedRef(ResGroupData *group)
{
	pg_atomic_sub_fetch_u32((pg_atomic_uint32 *) &group->nRunningBypassed, 1);
}

/*
 * Acquire a resource group slot
 *
 * Call this function at the start of the transaction.
 * This function set current resource group in MyResGroupSharedInfo,
 * and current slot in MyProc->resSlot.
 */
static ResGroupSlotData *
groupAcquireSlot(ResGroupInfo *pGroupInfo, bool isMoveQuery)
{
	ResGroupSlotData *slot;
	ResGroupData	 *group;

	Assert(!selfIsAssigned() || isMoveQuery);
	group = pGroupInfo->group;
	LWLockAcquire(ResGroupLock, LW_EXCLUSIVE);
	/* Has the group been dropped? */
	if (groupIsDropped(pGroupInfo))
	{
		LWLockRelease(ResGroupLock);
		return NULL;
	}
	/* acquire a slot */
	if (!group->lockedForDrop)
	{
		/* try to get a slot directly */
		slot = groupGetSlot(group);
		if (slot != NULL)
		{
			/* got one, lucky */
			group->totalExecuted++;
			LWLockRelease(ResGroupLock);
			/* pgstat_report_resgroup(group->groupId); */
			return slot;
		}
	}

	/*
	 * Add into group wait queue (if not waiting yet).
	 */
	Assert(!proc_exit_inprogress);
	groupWaitQueuePush(group, MyProc);
	if (!group->lockedForDrop)
		group->totalQueued++;
	LWLockRelease(ResGroupLock);
	/*
	 * wait on the queue
	 * slot will be assigned by the proc wakes me up
	 * if i am waken up by DROP RESOURCE GROUP statement, the
	 * resSlot will be NULL.
	 */
	waitOnGroup(group, isMoveQuery);
	if (MyProc->resSlot == NULL)
		return NULL;
	/*
	 * The waking process has granted us a valid slot.
	 * Update the statistic information of the resource group.
	 */
	slot = (ResGroupSlotData *) MyProc->resSlot;
	MyProc->resSlot = NULL;
	LWLockAcquire(ResGroupLock, LW_EXCLUSIVE);
	addTotalQueueDuration(group);
	group->totalExecuted++;
	LWLockRelease(ResGroupLock);

	return slot;
}

/*
 * Attempt to wake up pending slots in the group.
 *
 * - grant indicates whether to grant the proc a slot;
 * - release indicates whether to wake up the proc with the LWLock
 *   temporarily released;
 *
 * When grant is true we'll give up once no slot can be get,
 * e.g. due to lack of free slot.
 *
 * When grant is false all the pending procs will be woken up.
 */
static void
wakeupSlots(ResGroupData *group, bool grant)
{
	Assert(LWLockHeldByMeInMode(ResGroupLock, LW_EXCLUSIVE));

	while (!groupWaitQueueIsEmpty(group))
	{
		PGPROC		*waitProc;
		ResGroupSlotData *slot = NULL;
		if (grant)
		{
			/* try to get a slot for that proc */
			slot = groupGetSlot(group);
			if (slot == NULL)
				/* if can't get one then give up */
				break;
		}
		/* wake up one process in the wait queue */
		waitProc = groupWaitQueuePop(group);
		waitProc->resSlot = slot;
		procWakeup(waitProc);
	}
}

/* Update the total queued time of this group */
static void
addTotalQueueDuration(ResGroupData *group)
{
	Assert(LWLockHeldByMeInMode(ResGroupLock, LW_EXCLUSIVE));
	if (group == NULL)
		return;
	group->totalQueuedTimeMs += (groupWaitEnd - groupWaitStart);
}

/*
 * Release the resource group slot
 *
 * Call this function at the end of the transaction.
 */
static void
groupReleaseSlot(ResGroupData *group, ResGroupSlotData *slot, bool isMoveQuery)
{
	Assert(LWLockHeldByMeInMode(ResGroupLock, LW_EXCLUSIVE));
	Assert(!selfIsAssigned() || isMoveQuery);

	groupPutSlot(group, slot);
	/*
	 * My slot is put back, then how many queuing queries should I wake up?
	 * Maybe zero, maybe one, maybe more, depends on how the resgroup's
	 * configuration were changed during our execution.
	 */
	wakeupSlots(group, true);
}

/*
 * Serialize the resource group information that need to dispatch to segment.
 */
void
SerializeResGroupInfo(StringInfo str)
{
	unsigned int cpuset_len;
	int32		itmp;
	ResGroupCaps empty_caps;
	ResGroupCaps *caps;

	if (selfIsAssigned())
		caps = &self->caps;
	else
	{
		ClearResGroupCaps(&empty_caps);
		caps = &empty_caps;
	}

	itmp = htonl(self->groupId);
	appendBinaryStringInfo(str, (char *) &itmp, sizeof(int32));
	itmp = htonl(caps->concurrency);
	appendBinaryStringInfo(str, (char *) &itmp, sizeof(int32));
	itmp = htonl(caps->cpuMaxPercent);
	appendBinaryStringInfo(str, (char *) &itmp, sizeof(int32));
	itmp = htonl(caps->cpuWeight);
	appendBinaryStringInfo(str, (char *) &itmp, sizeof(int32));
	itmp = htonl(caps->memory_limit);
	appendBinaryStringInfo(str, (char *) &itmp, sizeof(int32));
	itmp = htonl(caps->min_cost);
	appendBinaryStringInfo(str, (char *) &itmp, sizeof(int32));
	cpuset_len = strlen(caps->cpuset);
	itmp = htonl(cpuset_len);
	appendBinaryStringInfo(str, (char *) &itmp, sizeof(int32));
	appendBinaryStringInfo(str, caps->cpuset, cpuset_len);
	itmp = htonl(bypassedSlot.groupId);
	appendBinaryStringInfo(str, (char *) &itmp, sizeof(itmp));
}

/*
 * Deserialize the resource group information dispatched.
 * not used.
 */
void
DeserializeResGroupInfo(struct ResGroupCaps *capsOut,
						Oid *groupId,
						const char *buf,
						int len)
{
	int32		itmp;
	unsigned int cpuset_len;
	const char	*ptr = buf;

	Assert(len > 0);
	ClearResGroupCaps(capsOut);
	memcpy(&itmp, ptr, sizeof(int32)); ptr += sizeof(int32);
	*groupId = ntohl(itmp);
	memcpy(&itmp, ptr, sizeof(int32)); ptr += sizeof(int32);
	capsOut->concurrency = ntohl(itmp);
	memcpy(&itmp, ptr, sizeof(int32)); ptr += sizeof(int32);
	capsOut->cpuMaxPercent = ntohl(itmp);
	memcpy(&itmp, ptr, sizeof(int32)); ptr += sizeof(int32);
	capsOut->cpuWeight = ntohl(itmp);
	memcpy(&itmp, ptr, sizeof(int32)); ptr += sizeof(int32);
	capsOut->memory_limit = ntohl(itmp);
	memcpy(&itmp, ptr, sizeof(int32)); ptr += sizeof(int32);
	capsOut->min_cost = ntohl(itmp);
	memcpy(&itmp, ptr, sizeof(int32)); ptr += sizeof(int32);
	cpuset_len = ntohl(itmp);
	if (cpuset_len >= sizeof(capsOut->cpuset))
		elog(ERROR, "malformed serialized resource group info");
	memcpy(capsOut->cpuset, ptr, cpuset_len); ptr += cpuset_len;
	capsOut->cpuset[cpuset_len] = '\0';
	memcpy(&itmp, ptr, sizeof(int32)); ptr += sizeof(int32);
	bypassedSlot.groupId = ntohl(itmp);

	Assert(len == ptr - buf);
}

/*
 * Check whether resource group should be assigned.
 */
bool
ShouldAssignResGroup(void)
{
	/*
	 * Bypass resource group when it's waiting for a resource group slot. e.g.
	 * MyProc was interrupted by SIGTERM while waiting for resource group slot.
	 * Some callbacks - RemoveTempRelationsCallback for example - open new
	 * transactions on proc exit. It can cause a double add of MyProc to the
	 * waiting queue (and its corruption).
	 *
	 * Also bypass resource group when it's exiting.
	 * Also bypass resource group when it's vacuum worker process.
	 */
	return IsResGroupActivated() &&
		IsNormalProcessingMode() &&
		!IsAnyAutoVacuumProcess() &&
		!IsLogicalLauncher() &&
		!proc_exit_inprogress &&
		!procIsWaiting(MyProc) &&
		!IsAutoVacuumWorkerProcess();
}

/*
 * Check whether resource group should be un-assigned.
 * This will be called on both coordinator and segments.
 */
bool
ShouldUnassignResGroup(void)
{
	return IsResGroupActivated() && IsNormalProcessingMode();
}

/*
 * A new session is assigned to a resource group at the beginning of a transaction.
 * It will first acquire a slot from the resource group, and then, it will get the
 * current capability snapshot, update the memory usage information, and add to
 * the corresponding cgroup.
 */
void
AssignResGroup(void)
{
	ResGroupSlotData	*slot;
	ResGroupInfo		groupInfo;

	pg_resource_group_disable_notice_scansup = true;
	/*
	 * if query should be bypassed, do not assign a
	 * resource group, leave self unassigned
	 */
	if (shouldBypassQuery(debug_query_string))
	{
		/*
		 * If it's the first query in the connection (make sure tab completion
		 * is not triggered otherwise it will run some implicit query before
		 * you execute the SHOW command).
		 *
		 * Also need to increase a bypassed ref count to prevent the group
		 * being dropped concurrently.
		 */
		do {
			decideResGroup(&groupInfo);
		} while (!groupIncBypassedRef(&groupInfo));
		/* Record which resgroup we are running in */
		bypassedGroup = groupInfo.group;
		/* Update pg_stat_activity statistics */
		bypassedGroup->totalExecuted++;
		/* pgstat_report_resgroup(bypassedGroup->groupId); */
		/* Initialize the fake slot */
		bypassedSlot.group = groupInfo.group;
		bypassedSlot.groupId = groupInfo.groupId;
		/* Add into cgroup */
		cgroupOpsRoutine->attachcgroup(bypassedGroup->groupId, MyProcPid,
									   bypassedGroup->caps.cpuMaxPercent == CPU_MAX_PERCENT_DISABLED);
		pg_resource_group_disable_notice_scansup = false;
		return;
	}
	pg_resource_group_disable_notice_scansup = false;
	
	PG_TRY();
	{
		do {
			decideResGroup(&groupInfo);
			/* Acquire slot */
			slot = groupAcquireSlot(&groupInfo, false);
		} while (slot == NULL);

		/* Set resource group slot for current session */
		sessionSetSlot(slot);
		selfAttachResGroup(groupInfo.group, slot);
		/* Init self */
		self->caps = slot->caps;
		/* Add into cgroup */
		cgroupOpsRoutine->attachcgroup(self->groupId, MyProcPid,
									   self->caps.cpuMaxPercent == CPU_MAX_PERCENT_DISABLED);
	}
	PG_CATCH();
	{
		UnassignResGroup();
		PG_RE_THROW();
	}
	PG_END_TRY();
}

/*
 * Detach from a resource group at the end of the transaction.
 */
void
UnassignResGroup(void)
{
	ResGroupData		*group;
	ResGroupSlotData	*slot;

	if (bypassedGroup)
	{
		/* bypass mode ref count is only maintained on qd */
		if (1)
			groupDecBypassedRef(bypassedGroup);

		/* Reset the fake slot */
		bypassedSlot.group = NULL;
		bypassedSlot.groupId = InvalidOid;
		bypassedGroup = NULL;

		return;
	}
	if (!selfIsAssigned())
		return;
	LWLockAcquire(ResGroupLock, LW_EXCLUSIVE);
	group = self->group;
	slot = self->slot;
	/* Sub proc memory accounting info from group and slot */
	selfDetachResGroup(group, slot);
	/* Release the slot if no reference. */
	if (slot->nProcs == 0)
	{
		groupReleaseSlot(group, slot, false);

		/*
		 * Reset resource group slot for current session. Note MySessionState
		 * could be reset as NULL in shmem_exit() before.
		 */
		sessionResetSlot(slot);
	}
	LWLockRelease(ResGroupLock);
}


/*
 * Wait on the queue of resource group
 */
static void
waitOnGroup(ResGroupData *group, bool isMoveQuery)
{
	int64 timeout = -1;
	int64 curTime;
	const char *old_status;
	char *new_status = NULL;
	int len;
	PGPROC *proc = MyProc;
	const char *queueStr = " queuing";

	Assert(!LWLockHeldByMeInMode(ResGroupLock, LW_EXCLUSIVE));
	Assert(!selfIsAssigned() || isMoveQuery);

	/* set ps status to waiting */
	if (update_process_title)
	{
		old_status = get_ps_display(&len);
		new_status = (char *) palloc(len + strlen(queueStr) + 1);
		memcpy(new_status, old_status, len);
		strcpy(new_status + len, queueStr);
		set_ps_display(new_status);
		/* truncate off " queuing" */
		new_status[len] = '\0';
	}

	/*
	 * Mark that we are waiting on resource group
	 *
	 * This is used for interrupt cleanup, similar to lockAwaited in ProcSleep
	 */
	groupAwaited = group;
	groupWaitStart = GetCurrentTimestamp();

	/*
	 * Make sure we have released all locks before going to sleep, to eliminate
	 * deadlock situations
	 */
	PG_TRY();
	{
		for (;;)
		{
			ResetLatch(&proc->procLatch);

			CHECK_FOR_INTERRUPTS();

			if (!procIsWaiting(proc))
				break;

			if (pg_resource_group_queuing_timeout > 0)
			{
				curTime = GetCurrentTimestamp();
				timeout = pg_resource_group_queuing_timeout - (curTime - groupWaitStart) / 1000;
				if (timeout < 0)
					ereport(ERROR,
							(errcode(ERRCODE_QUERY_CANCELED),
							 errmsg("canceling statement due to resource group waiting timeout")));

				WaitLatch(&proc->procLatch, WL_LATCH_SET | WL_TIMEOUT | WL_POSTMASTER_DEATH,
						  (long) timeout, PG_WAIT_RESOURCE_GROUP);
			}
			else
			{
				WaitLatch(&proc->procLatch, WL_LATCH_SET | WL_POSTMASTER_DEATH, -1,
						  PG_WAIT_RESOURCE_GROUP);
			}
		}
	}
	PG_CATCH();
	{
		/* reset ps status */
		if (update_process_title)
		{
			set_ps_display(new_status);
			pfree(new_status);
		}

		groupWaitCancel(isMoveQuery);
		PG_RE_THROW();
	}
	PG_END_TRY();
	groupAwaited = NULL;
	groupWaitEnd = GetCurrentTimestamp();
	/* reset ps status */
	if (update_process_title)
	{
		set_ps_display(new_status);
		pfree(new_status);
	}
}

/*
 * groupHashNew -- return a new (empty) group object to initialize.
 *
 * Notes
 *	The resource group lightweight lock (ResGroupLock) *must* be held for
 *	this operation.
 */
static ResGroupData *
groupHashNew(Oid groupId)
{
	int			i;
	bool		found;
	ResGroupHashEntry *entry;

	Assert(LWLockHeldByMeInMode(ResGroupLock, LW_EXCLUSIVE));
	Assert(groupId != InvalidOid);

	for (i = 0; i < pResGroupControl->nGroups; i++)
	{
		if (pResGroupControl->groups[i].groupId == InvalidOid)
			break;
	}
	Assert(i < pResGroupControl->nGroups);

	entry = (ResGroupHashEntry *)
		hash_search(pResGroupControl->htbl, (void *) &groupId, HASH_ENTER, &found);
	/* caller should test that the group does not exist already */
	Assert(!found);
	entry->index = i;

	return &pResGroupControl->groups[i];
}

/*
 * groupHashFind -- return the group for a given oid.
 *
 * If the group cannot be found, then NULL is returned if 'raise' is false,
 * otherwise an exception is thrown.
 *
 * Notes
 *	The resource group lightweight lock (ResGroupLock) *must* be held for
 *	this operation.
 */
static ResGroupData *
groupHashFind(Oid groupId, bool raise)
{
	bool				found;
	ResGroupHashEntry	*entry;

	Assert(LWLockHeldByMe(ResGroupLock));

	entry = (ResGroupHashEntry *)
		hash_search(pResGroupControl->htbl, (void *) &groupId, HASH_FIND, &found);
	if (!found)
	{
		ereport(raise ? ERROR : LOG,
				(errcode(ERRCODE_DATA_CORRUPTED),
				 errmsg("cannot find resource group with Oid %d in shared memory",
						groupId)));
		return NULL;
	}

	Assert(entry->index < pResGroupControl->nGroups);
	return &pResGroupControl->groups[entry->index];
}

/*
 * groupHashRemove -- remove the group for a given oid.
 *
 * If the group cannot be found then an exception is thrown.
 *
 * Notes
 *	The resource group lightweight lock (ResGroupLock) *must* be held for
 *	this operation.
 */
static ResGroupData *
groupHashRemove(Oid groupId)
{
	bool		found;
	ResGroupHashEntry	*entry;
	ResGroupData		*group;

	Assert(LWLockHeldByMeInMode(ResGroupLock, LW_EXCLUSIVE));

	entry = (ResGroupHashEntry*)hash_search(pResGroupControl->htbl,
											(void *) &groupId,
											HASH_REMOVE,
											&found);
	if (!found)
		ereport(ERROR,
				(errcode(ERRCODE_DATA_CORRUPTED),
				 errmsg("cannot find resource group with Oid %d in shared memory to remove",
						groupId)));
	group = &pResGroupControl->groups[entry->index];

	return group;
}

/* Process exit without waiting for slot or received SIGTERM */
static void
AtProcExit_ResGroup(int code, Datum arg)
{
	groupWaitCancel(false);
}

/*
 * Handle the interrupt cases when waiting on the queue
 *
 * The proc may wait on the queue for a slot, or wait for the
 * DROP transaction to finish. In the first case, at the same time
 * we get interrupted (SIGINT or SIGTERM), we could have been
 * granted a slot or not. In the second case, there's no running
 * transaction in the group. If the DROP transaction is finished
 * (commit or abort) at the same time as we get interrupted,
 * MyProc should have been removed from the wait queue, and the
 * ResGroupData entry may have been removed if the DROP is committed.
 */
static void
groupWaitCancel(bool isMoveQuery)
{
	ResGroupData		*group;
	ResGroupSlotData	*slot;

	/* Nothing to do if we weren't waiting on a group */
	if (groupAwaited == NULL)
		return;

	pgstat_report_wait_end();
	groupWaitEnd = GetCurrentTimestamp();

	Assert(!selfIsAssigned() || isMoveQuery);

	group = groupAwaited;
	/* We are sure to be interrupted in the for loop of waitOnGroup now */
	LWLockAcquire(ResGroupLock, LW_EXCLUSIVE);
	if (procIsWaiting(MyProc))
	{
		/*
		 * Still waiting on the queue when get interrupted, remove
		 * myself from the queue
		 */
		Assert(!groupWaitQueueIsEmpty(group));
		groupWaitQueueErase(group, MyProc);
		addTotalQueueDuration(group);
	}
	else if (MyProc->resSlot != NULL)
	{
		/* Woken up by a slot holder */
		Assert(!procIsWaiting(MyProc));
		/* First complete the slot's transfer from MyProc to self */
		slot = MyProc->resSlot;
		MyProc->resSlot = NULL;
		/*
		 * Similar as groupReleaseSlot(), how many pending queries to
		 * wake up depends on how many slots we can get.
		 */
		groupReleaseSlot(group, slot, false);
		/*
		 * Reset resource group slot for current session. Note MySessionState
		 * could be reset as NULL in shmem_exit() before.
		 */
		sessionResetSlot(slot);
		group->totalExecuted++;
		addTotalQueueDuration(group);
	}
	else
	{
		/*
		 * The transaction of DROP RESOURCE GROUP is finished,
		 * groupAcquireSlot will do the retry.
		 *
		 * The resource group pointed by self->group may have
		 * already been removed by here.
		 */
		Assert(!procIsWaiting(MyProc));
	}
	LWLockRelease(ResGroupLock);
	groupAwaited = NULL;
}

/*
 * Validate the consistency of the resgroup information in self.
 *
 * This function checks the consistency of (group & groupId).
 */
static void
selfValidateResGroupInfo(void)
{
	Assert(self->groupId == InvalidOid || self->group != NULL);
	/*
	AssertImply(self->groupId != InvalidOid,
		self->group != NULL);
	*/
}

/*
 * Check whether self is assigned.
 *
 * This is mostly equal to (selfHasSlot() && selfHasGroup()),
 * however this function requires the slot and group to be in
 * a consistent status, they must both be set or unset,
 * so calling this function during the assign/unassign/switch process
 * might cause an error, use with caution.
 *
 * Even selfIsAssigned() is true it doesn't mean the assign/switch
 * process is completely done, for example the memory accounting
 * information might not been updated yet.
 *
 * This function doesn't check whether the assigned resgroup
 * is valid or dropped.
 */
static bool
selfIsAssigned(void)
{
	selfValidateResGroupInfo();
	/*
	AssertImply(self->group == NULL,
			self->slot == NULL);
	AssertImply(self->group != NULL,
			self->slot != NULL);
	*/
	Assert(self->groupId != InvalidOid || self->group == NULL);
	Assert(self->groupId == InvalidOid || self->group != NULL);
	
	return self->groupId != InvalidOid;
}

#ifdef USE_ASSERT_CHECKING
/*
 * Check whether self has been set a slot.
 *
 * We don't check whether a resgroup is set or not.
 */
static bool
selfHasSlot(void)
{
	return self->slot != NULL;
}

/*
 * Check whether self has been set a resgroup.
 *
 * Consistency will be checked on the groupId and group pointer.
 *
 * We don't check whether the resgroup is valid or dropped.
 *
 * We don't check whether a slot is set or not.
 */
static bool
selfHasGroup(void)
{
	return self->groupId != InvalidOid;
}
#endif /* USE_ASSERT_CHECKING */

/*
 * Set both the groupId and the group pointer in self.
 *
 * The group must not be dropped.
 *
 * Some over limitations are put to force the caller understand
 * what it's doing and what it wants:
 * - self must has not been set a resgroup;
 */
static void
selfSetGroup(ResGroupData *group)
{
	Assert(!selfIsAssigned());
	Assert(groupIsNotDropped(group));

	self->group = group;
	self->groupId = group->groupId;
}

/*
 * Unset both the groupId and the resgroup pointer in self.
 *
 * Some over limitations are put to force the caller understand
 * what it's doing and what it wants:
 * - self must has been set a resgroup;
 */
static void
selfUnsetGroup(void)
{
	Assert(selfHasGroup());
	Assert(!selfHasSlot());

	self->groupId = InvalidOid;
	self->group = NULL;
}

/*
 * Set the slot pointer in self.
 *
 * Some over limitations are put to force the caller understand
 * what it's doing and what it wants:
 * - self must has been set a resgroup;
 * - self must has not been set a slot before set;
 */
static void
selfSetSlot(ResGroupSlotData *slot)
{
	Assert(selfHasGroup());
	Assert(!selfHasSlot());
	Assert(slotIsInUse(slot));

	self->slot = slot;
}

/*
 * Unset the slot pointer in self.
 *
 * Some over limitations are put to force the caller understand
 * what it's doing and what it wants:
 * - self must has been set a resgroup;
 * - self must has been set a slot before unset;
 */
static void
selfUnsetSlot(void)
{
	Assert(selfHasGroup());
	Assert(selfHasSlot());

	self->slot = NULL;
}

/*
 * Check whether proc is in some resgroup's wait queue.
 *
 * The LWLock is not required.
 *
 * This function does not check whether proc is in a specific resgroup's
 * wait queue. To make this check use groupWaitQueueFind().
 */
static bool
procIsWaiting(const PGPROC *proc)
{
	/*------
	 * The typical asm instructions fow below C operation can be like this:
	 * ( gcc 4.8.5-11, x86_64-redhat-linux, -O0 )
	 *
     *     mov    -0x8(%rbp),%rax           ; load proc
     *     mov    0x8(%rax),%rax            ; load proc->links.next
     *     cmp    $0,%rax                   ; compare with NULL
     *     setne  %al                       ; store the result
	 *
	 * The operation is atomic, so a lock is not required here.
	 *------
	 */
	return proc->links.next != NULL;
}

/*
 * Notify a proc it's woken up.
 */
static void
procWakeup(PGPROC *proc)
{
	Assert(!procIsWaiting(proc));

	SetLatch(&proc->procLatch);
}

#ifdef USE_ASSERT_CHECKING

/*
 * Validate a slot's attributes.
 */
static void
slotValidate(const ResGroupSlotData *slot)
{
	Assert(slot != NULL);

	/* further checks whether the slot is freed or idle */
	if (slot->groupId == InvalidOid)
	{
		Assert(slot->nProcs == 0);
	}
	else
	{
		Assert(!slotIsInFreelist(slot));
		/* Assert(slot == sessionGetSlot()); */
	}
}

/*
 * A slot is in use if it has a valid groupId.
 */
static bool
slotIsInUse(const ResGroupSlotData *slot)
{
	slotValidate(slot);

	return slot->groupId != InvalidOid;
}

static bool
slotIsInFreelist(const ResGroupSlotData *slot)
{
	ResGroupSlotData *current;

	current = pResGroupControl->freeSlot;
	for ( ; current != NULL; current = current->next)
	{
		if (current == slot)
			return true;
	}

	return false;
}
#endif /* USE_ASSERT_CHECKING */

static void
lockResGroupForDrop(ResGroupData *group)
{
	if (group->lockedForDrop)
		return;

	Assert(LWLockHeldByMeInMode(ResGroupLock, LW_EXCLUSIVE));
	Assert(group->nRunning == 0);
	Assert(group->nRunningBypassed == 0);
	group->lockedForDrop = true;
}

static void
unlockResGroupForDrop(ResGroupData *group)
{
	if (!group->lockedForDrop)
		return;

	Assert(LWLockHeldByMeInMode(ResGroupLock, LW_EXCLUSIVE));
	Assert(group->nRunning == 0);
	Assert(group->nRunningBypassed == 0);
	group->lockedForDrop = false;
}

#ifdef USE_ASSERT_CHECKING
/*
 * Check whether a resgroup is dropped.
 *
 * A dropped resgroup has groupId == InvalidOid,
 * however there is also the case that the resgroup is first dropped
 * then the shm struct is reused by another newly created resgroup,
 * in such a case the groupId is not InvalidOid but the original
 * resgroup does is dropped.
 *
 * So this function is not always reliable, use with caution.
 */
static bool
groupIsNotDropped(const ResGroupData *group)
{
	return group
		&& group->groupId != InvalidOid;
}
#endif /* USE_ASSERT_CHECKING */

/*
 * Validate the consistency of the resgroup wait queue.
 */
static void
groupWaitQueueValidate(const ResGroupData *group)
{
	const PROC_QUEUE	*waitQueue;

	Assert(LWLockHeldByMeInMode(ResGroupLock, LW_EXCLUSIVE));

	waitQueue = &group->waitProcs;
	if (pg_resgroup_debug_wait_queue)
	{
		if (waitQueue->size == 0)
		{
			if (waitQueue->links.next != &waitQueue->links ||
				waitQueue->links.prev != &waitQueue->links)
				elog(PANIC, "resource group wait queue is corrupted");
		}
		else
		{
			PGPROC *nextProc = (PGPROC *)waitQueue->links.next;
			PGPROC *prevProc = (PGPROC *)waitQueue->links.prev;

			if (
				/*
				!nextProc->mppIsWriter ||
				!prevProc->mppIsWriter ||
				*/
				nextProc->links.prev != &waitQueue->links ||
				prevProc->links.next != &waitQueue->links)
				elog(PANIC, "resource group wait queue is corrupted");
		}
		return;
	}

}

static void
groupWaitProcValidate(PGPROC *proc, PROC_QUEUE *head)
{
	/* PGPROC *nextProc = (PGPROC *)proc->links.next; */
	/* PGPROC *prevProc = (PGPROC *)proc->links.prev; */

	Assert(LWLockHeldByMeInMode(ResGroupLock, LW_EXCLUSIVE));

	if (!pg_resgroup_debug_wait_queue)
		return;

	/*
	if (!proc->mppIsWriter ||
		((PROC_QUEUE *)nextProc != head && !nextProc->mppIsWriter) ||
		((PROC_QUEUE *)prevProc != head && !prevProc->mppIsWriter) ||
		nextProc->links.prev != &proc->links ||
		prevProc->links.next != &proc->links)
		elog(PANIC, "resource group wait queue is corrupted");
	*/
	
	return;
}

/*
 * Push a proc to the resgroup wait queue.
 */
static void
groupWaitQueuePush(ResGroupData *group, PGPROC *proc)
{
	PROC_QUEUE			*waitQueue;
	PGPROC				*headProc;

	Assert(LWLockHeldByMeInMode(ResGroupLock, LW_EXCLUSIVE));
	Assert(!procIsWaiting(proc));
	Assert(proc->resSlot == NULL);

	groupWaitQueueValidate(group);
	waitQueue = &group->waitProcs;
	headProc = (PGPROC *) &waitQueue->links;
	SHMQueueInsertBefore(&headProc->links, &proc->links);
	groupWaitProcValidate(proc, waitQueue);
	waitQueue->size++;

	Assert(groupWaitQueueFind(group, proc));
}

/*
 * Pop the top proc from the resgroup wait queue and return it.
 */
static PGPROC *
groupWaitQueuePop(ResGroupData *group)
{
	PROC_QUEUE			*waitQueue;
	PGPROC				*proc;

	Assert(LWLockHeldByMeInMode(ResGroupLock, LW_EXCLUSIVE));
	Assert(!groupWaitQueueIsEmpty(group));

	groupWaitQueueValidate(group);
	waitQueue = &group->waitProcs;
	proc = (PGPROC *) waitQueue->links.next;
	groupWaitProcValidate(proc, waitQueue);
	Assert(groupWaitQueueFind(group, proc));
	Assert(proc->resSlot == NULL);

	SHMQueueDelete(&proc->links);
	waitQueue->size--;

	return proc;
}

/*
 * Erase proc from the resgroup wait queue.
 */
static void
groupWaitQueueErase(ResGroupData *group, PGPROC *proc)
{
	PROC_QUEUE			*waitQueue;

	Assert(LWLockHeldByMeInMode(ResGroupLock, LW_EXCLUSIVE));
	Assert(!groupWaitQueueIsEmpty(group));
	Assert(groupWaitQueueFind(group, proc));
	Assert(proc->resSlot == NULL);

	groupWaitQueueValidate(group);
	waitQueue = &group->waitProcs;
	groupWaitProcValidate(proc, waitQueue);
	SHMQueueDelete(&proc->links);
	waitQueue->size--;
}

/*
 * Check whether the resgroup wait queue is empty.
 */
static bool
groupWaitQueueIsEmpty(const ResGroupData *group)
{
	const PROC_QUEUE	*waitQueue;

	Assert(LWLockHeldByMeInMode(ResGroupLock, LW_EXCLUSIVE));
	groupWaitQueueValidate(group);
	waitQueue = &group->waitProcs;
	return waitQueue->size == 0;
}

#ifdef USE_ASSERT_CHECKING
/*
 * Find proc in group's wait queue.
 *
 * Return true if found or false if not found.
 *
 * This functions is expensive so should only be used in debugging logic,
 * in most cases procIsWaiting() shall be used.
 */
static bool
groupWaitQueueFind(ResGroupData *group, const PGPROC *proc)
{
	PROC_QUEUE			*waitQueue;
	SHM_QUEUE			*head;
	PGPROC				*iter;
	Size				offset;

	Assert(LWLockHeldByMeInMode(ResGroupLock, LW_EXCLUSIVE));

	groupWaitQueueValidate(group);
	waitQueue = &group->waitProcs;
	head = &waitQueue->links;
	offset = offsetof(PGPROC, links);
	for (iter = (PGPROC *) SHMQueueNext(head, head, offset); iter;
		 iter = (PGPROC *) SHMQueueNext(head, &iter->links, offset))
	{
		if (iter == proc)
		{
			Assert(procIsWaiting(proc));
			return true;
		}
	}

	return false;
}
#endif/* USE_ASSERT_CHECKING */

/*
 * Walk through the raw expression tree, if there is a RangeVar without
 * `pg_catalog` prefix, terminate the process immediately to save the cpu
 * resource.
 */
static bool
checkBypassWalker(Node *node, void *context)
{
	bool *bypass = context;

	if (node == NULL)
		return false;

	if (IsA(node, RangeVar))
	{
		RangeVar *from = (RangeVar *) node;
		if (from->schemaname == NULL ||
			strcmp(from->schemaname, "pg_catalog") != 0)
		{
			*bypass = false;
			return true;
		}
		else
		{
			/*
			 * Make sure there is at least one RangeVar
			 */
			*bypass = true;
		}
	}

	return raw_expression_tree_walker(node, checkBypassWalker, context);
}

static bool
shouldBypassSelectQuery(Node *node)
{
	bool catalog_bypass = false;

	if (pg_resource_group_bypass_catalog_query)
		raw_expression_tree_walker(node, checkBypassWalker, &catalog_bypass);

	return catalog_bypass;
}

/*
 * Parse the query and check if this query should
 * bypass the management of resource group.
 *
 * Currently, only SET/RESET/SHOW command and SELECT with only catalog tables
 * can be bypassed
 */
static bool
shouldBypassQuery(const char *query_string)
{
	MemoryContext oldcontext = NULL;
	MemoryContext tmpcontext = NULL;
	List *parsetree_list; 
	ListCell *parsetree_item;
	Node *parsetree;
	bool		bypass;

	if (pg_resource_group_bypass)
		return true;

	if (!query_string)
		return false;

	/*
	 * Switch to appropriate context for constructing parsetrees.
	 *
	 * It is possible that MessageContext is NULL, for example in a bgworker:
	 *
	 *     debug_query_string = "select 1";
	 *     StartTransactionCommand();
	 *
	 * This is not the recommended order of setting debug_query_string, but we
	 * should not put a constraint on the order by resource group anyway.
	 */
	if (MessageContext)
		oldcontext = MemoryContextSwitchTo(MessageContext);
	else
	{
		/* Create a temp memory context to prevent memory leaks */
		tmpcontext = AllocSetContextCreate(CurrentMemoryContext,
										   "resgroup temporary context",
										   ALLOCSET_DEFAULT_MINSIZE,
										   ALLOCSET_DEFAULT_INITSIZE,
										   ALLOCSET_DEFAULT_MAXSIZE);
		oldcontext = MemoryContextSwitchTo(tmpcontext);
	}

	parsetree_list = pg_parse_query(query_string);
	MemoryContextSwitchTo(oldcontext);
	if (parsetree_list == NULL)
		return false;

	/* Only bypass SET/RESET/SHOW command and SELECT with only catalog tables
	 * for now */
	bypass = true;
	foreach(parsetree_item, parsetree_list)
	{
		parsetree = (Node *) lfirst(parsetree_item);
		if (nodeTag(parsetree) == T_RawStmt)
			parsetree = ((RawStmt *)parsetree)->stmt;
		if (IsA(parsetree, SelectStmt))
		{
			if (!shouldBypassSelectQuery(parsetree))
			{
				bypass = false;
				break;
			}
		}
		else if (nodeTag(parsetree) != T_VariableSetStmt &&
			nodeTag(parsetree) != T_VariableShowStmt &&
			nodeTag(parsetree) != T_AlterSystemStmt ) /* BUG#4773 */
		{
			bypass = false;
			break;
		}
	}
	list_free_deep(parsetree_list);
	if (tmpcontext)
		MemoryContextDelete(tmpcontext);

	return bypass;
}

/*
 * Check whether the resource group has been dropped.
 */
static bool
groupIsDropped(ResGroupInfo *pGroupInfo)
{
	Assert(pGroupInfo != NULL);
	Assert(pGroupInfo->group != NULL);

	return pGroupInfo->group->groupId != pGroupInfo->groupId;
}

/*
 * Set resource group slot for current session.
 */
static void
sessionSetSlot(ResGroupSlotData *slot)
{
	Assert(slot != NULL);
	/*
	 * Previously, we had an assertion, that MySessionState->resGroupSlot
	 * should be NULL here. There is a case, when we want to move processes
	 * from one group to another. We got assertion error on main process,
	 * if entrydb process not called UnassignResGroup() yet (and vice versa).
	 * Next call to UnassignResGroup() (by main or entrydb process) will free
	 * slot and it's OK, but here we want to set new slot to session, so we
	 * changed assertion.
	 */
	/* Assert(MySessionState->resGroupSlot == NULL); */
	
	/*
	 * SessionStateLock is required since runaway detector will traverse
	 * the current session array and check corresponding resGroupSlot with
	 * shared lock on SessionStateLock.
	 */
	LWLockAcquire(SessionStateLock, LW_EXCLUSIVE);
	MySessionState->pid = MyProcPid;
	MySessionState->resGroupSlot = (void *) slot;
	LWLockRelease(SessionStateLock);
}

/*
 * Reset resource group slot for current session to NULL, check we resetting
 * correct slot
 */
static void
sessionResetSlot(ResGroupSlotData *slot)
{
	/*
	 * SessionStateLock is required since runaway detector will traverse
	 * the current session array and check corresponding resGroupSlot with
	 * shared lock on SessionStateLock.
	 */
	/*
	if (MySessionState != NULL)
	{
		LWLockAcquire(SessionStateLock, LW_EXCLUSIVE);

		If the slot is ours, set resGroupSlot to NULL. 

		
		if (MySessionState->resGroupSlot == slot)
			MySessionState->resGroupSlot = NULL;

		LWLockRelease(SessionStateLock);
	}
	*/
}

/*
 * Parse cpuset to bitset
 * If cpuset is "1,3-5", Bitmapset 1,3,4,5 are set.
 */
Bitmapset *
CpusetToBitset(const char *cpuset, int len)
{
	int	pos = 0, num1 = 0, num2 = 0;
	enum Status
	{
		Initial,
		Begin,
		Number,
		Interval,
		Number2
	};
	enum Status	s = Initial;

	Bitmapset	*bms = NULL;
	if (cpuset == NULL || len <= 0)
		return bms;
	while (pos < len && cpuset[pos])
	{
		char c = cpuset[pos++];
		if (c == ',')
		{
			if (s == Initial || s == Begin)
			{
				continue;
			}
			else if (s == Interval)
			{
				goto error_logic;
			}
			else if (s == Number)
			{
				bms = bms_union(bms, bms_make_singleton(num1));
				num1 = 0;
				s = Begin;
			}
			else if (s == Number2)
			{
				if (num1 > num2)
				{
					goto error_logic;
				}
				for (int i = num1; i <= num2; ++i)
				{
					bms = bms_union(bms, bms_make_singleton(i));
				}
				num1 = num2 = 0;
				s = Begin;
			}
		}
		else if (c == '-')
		{
			if (s != Number)
			{
				goto error_logic;
			}
			s = Interval;
		}
		else if (isdigit(c))
		{
			if (s == Initial || s == Begin)
			{
				s = Number;
			}
			else if (s == Interval)
			{
				s = Number2;
			}
			if (s == Number)
			{
				num1 = num1 * 10 + (c - '0');
			}
			else if (s == Number2)
			{
				num2 = num2 * 10 + (c - '0');
			}
		}
		else if (c == '\n')
		{
			break;
		}
		else
		{
			goto error_logic;
		}
	}
	if (s == Number)
	{
		bms = bms_union(bms, bms_make_singleton(num1));
	}
	else if (s == Number2)
	{
		if (num1 > num2)
		{
			goto error_logic;
		}
		for (int i = num1; i <= num2; ++i)
		{
			bms = bms_union(bms, bms_make_singleton(i));
		}
	}
	else if (s == Initial || s == Interval)
	{
		goto error_logic;
	}
	return bms;
error_logic:
	return NULL;
}

/*
 * Check the value of cpuset is empty or not
 */
bool CpusetIsEmpty(const char *cpuset)
{
	return strcmp(cpuset, DefaultCpuset) == 0;
}

/*
 * Set cpuset value to default value -1.
 */
void SetCpusetEmpty(char *cpuset, int cpusetSize)
{
	RGStrNCpy(cpuset, DefaultCpuset, cpusetSize);
}

/*
 * Transform non-empty bitset to cpuset.
 *
 * This function does not check the cpu cores are available or not.
 */
void
BitsetToCpuset(const Bitmapset *bms,
			   char *cpuset,
			   int cpusetSize)
{
	int len = 0;
	int lastContinuousBit = -1;
	int	intervalStart = -1;
	int num;
	char buffer[32] = {0};

	Assert(!bms_is_empty(bms));

	cpuset[0] = '\0';
	num = -1;
	while ((num = bms_next_member(bms, num)) >= 0)
	{
		if (lastContinuousBit == -1)
		{
			intervalStart = lastContinuousBit = num;
		}
		else
		{
			if (num != lastContinuousBit + 1)
			{
				if (intervalStart == lastContinuousBit)
				{
					snprintf(buffer, sizeof(buffer), "%d,", intervalStart);
				}
				else
				{
					snprintf(buffer, sizeof(buffer), "%d-%d,", intervalStart, lastContinuousBit);
				}
				if (len + strlen(buffer) >= cpusetSize)
				{
					Assert(cpuset[0]);
					return ;
				}
				strcpy(cpuset + len, buffer);
				len += strlen(buffer);
				intervalStart = lastContinuousBit = num;
			}
			else
			{
				lastContinuousBit = num;
			}
		}
	}
	if (intervalStart != -1)
	{
		if (intervalStart == lastContinuousBit)
		{
			snprintf(buffer, sizeof(buffer), "%d", intervalStart);
		}
		else
		{
			snprintf(buffer, sizeof(buffer), "%d-%d", intervalStart, lastContinuousBit);
		}
		if (len + strlen(buffer) >= cpusetSize)
		{
			Assert(cpuset[0]);
			return ;
		}
		strcpy(cpuset + len, buffer);
		len += strlen(buffer);
	}
	else
	{
		/* bms is non-empty, so it should never reach here */
		pg_unreachable();
	}
}

/*
 * calculate the result of cpuset1 plus/minus cpuset2 and save in place
 * if sub is true, the operation is minus
 * if sub is false, the operation is plus
 */
void
cpusetOperation(char *cpuset1, const char *cpuset2,
							int len, bool sub)
{
	char cpuset[MaxCpuSetLength] = {0};
	int defaultCore = -1;
	Bitmapset *bms1 = CpusetToBitset(cpuset1, len);
	Bitmapset *bms2 = CpusetToBitset(cpuset2, len);
	Bitmapset *bmsDefault = NULL;
	if (sub)
	{
		bms1 = bms_del_members(bms1, bms2);
	}
	else
	{
		bms1 = bms_add_members(bms1, bms2);
	}
	if (!bms_is_empty(bms1))
	{
		BitsetToCpuset(bms1, cpuset1, len);
	}
	else
	{
		/* Get cpuset from cpuset of BASEDIR_HGDB, and transform it into bitset */
		cgroupOpsRoutine->getcpuset(CGROUP_ROOT_ID, cpuset, MaxCpuSetLength);
		bmsDefault = CpusetToBitset(cpuset, MaxCpuSetLength);
		/* get the minimum core number, in case of the zero core is not exist */
		defaultCore = bms_next_member(bmsDefault, -1);
		Assert(defaultCore >= 0);
		snprintf(cpuset1, MaxCpuSetLength, "%d", defaultCore);
	}
}

/*
 * union cpuset2 to cpuset1
 */
void
CpusetUnion(char *cpuset1, const char *cpuset2, int len)
{
	cpusetOperation(cpuset1, cpuset2, len, false);
}

/*
 * subtract cpuset2 from cpuset1
 */
void
CpusetDifference(char *cpuset1, const char *cpuset2, int len)
{
	cpusetOperation(cpuset1, cpuset2, len, true);
}

/*
 * ensure that cpuset is available.
 */
bool
EnsureCpusetIsAvailable(int elevel)
{
	if (!IsResGroupActivated())
	{
		ereport(elevel,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("resource group must be enabled to use cpuset feature")));

		return false;
	}
	if (!pg_resource_group_enable_cgroup_cpuset)
	{
		ereport(elevel,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("cgroup is not properly configured to use the cpuset feature"),
				 errhint("Extra cgroup configurations are required to enable this feature, "
						 "please refer to the Greenplum Documentations for details")));

		return false;
	}
	return true;
}

/*
 * move a proc to a resource group
 */
void
HandleMoveResourceGroup(void)
{
	ResGroupSlotData *slot;
	ResGroupData *group;
	Oid			groupId;
	pid_t		callerPid;

	/* transaction has finished */
	if (!IsTransactionState() || !selfIsAssigned())
	{
		SpinLockAcquire(&MyProc->movetoMutex);

		/*
		 * setting movetoGroupId to InvalidOid alone without setting
		 * movetoResSlot to NULL means target process tried to handle, but
		 * can't do anything with a command
		 */
		MyProc->movetoGroupId = InvalidOid;
		callerPid = MyProc->movetoCallerPid;
		SpinLockRelease(&MyProc->movetoMutex);

		/* notify initiator, current command is irrelevant */
		if (callerPid != InvalidPid)
			ResGroupMoveNotifyInitiator(callerPid);
		return;
	}

	SpinLockAcquire(&MyProc->movetoMutex);
	slot = (ResGroupSlotData *) MyProc->movetoResSlot;
	groupId = MyProc->movetoGroupId;
	callerPid = MyProc->movetoCallerPid;
	/* set to NULL to mark we got slot control */
	MyProc->movetoResSlot = NULL;
	/* set to InvalidOid to mark we handling the command */
	MyProc->movetoGroupId = InvalidOid;

	/*
	 * Don't clean movetoCallerPid. It guards us from another initiators,
	 * which may overwrite moveto* params.
	 */
	SpinLockRelease(&MyProc->movetoMutex);
	if (!slot)
	{
		/* moving command is irrelevant */
		return;
	}
	/*
	 * starting from this point, all control over slot should be done
	 * here, from target process
	 */

	Assert(groupId != InvalidOid);
	
	ResGroupMoveNotifyInitiator(callerPid);
	/* unassign the old resource group and release the old slot */
	UnassignResGroup();
	sessionSetSlot(slot);
	/* Add proc memory accounting info into group and slot */
	group = slot->group;
	selfAttachResGroup(group, slot);
	/* Init self */
	self->caps = slot->caps;

	/*
	 * You may say it's ugly to notify entrydb process here, but not in
	 * initiator process, but we want to be sure slot was actually
	 * assigned to session using sessionSetSlot(). We can't do much inside
	 * one spinlock. Especially, we can't work with multiple LWLocks
	 * inside of it. So, to keep the solution simple and plain, we decided
	 * to signal entrydb process here, inside of target process handler.
	 */

	/**
	(void) ResGroupMoveSignalTarget(MyProcPid,
									NULL, groupId, true);
	**/
	
	/*
	 * Add into cgroup. On any exception slot will be freed by the end of
	 * transaction.
	 */
	cgroupOpsRoutine->attachcgroup(self->groupId, MyProcPid,
								   self->caps.cpuMaxPercent == CPU_MAX_PERCENT_DISABLED);
}

/*
 * Try to give away all slot control to target process.
 */
static void
resGroupGiveSlotAway(int sessionId, ResGroupSlotData ** slot, Oid groupId)
{
	long		timeout;
	int64		curTime;
	int64		waitStart;
	int			latchRes;
	bool		clean = false;
	bool		res = false;
	/*
	SIMPLE_FAULT_INJECTOR("resource_group_give_away_begin");
	*/
	if (!ResGroupMoveSignalTarget(sessionId, *slot, groupId, false))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 (errmsg("cannot send signal to process"))));
	waitStart = GetCurrentTimestamp();
	for (;;)
	{
		/*
		 * In an infinite loop, call ResGroupMoveCheckTargetReady() to check whether
		 * all target processes have received signal of RG move.
		 * If we have hit pg_resource_group_move_timeout, try to cancel the move
		 * operation (no matter was target handled signal) and clean remained stuffs.
		 */
		curTime = GetCurrentTimestamp();
		timeout = pg_resource_group_move_timeout - (curTime - waitStart) / 1000;
		if (timeout > 0)
		{
			PG_TRY();
			{
				/*
				SIMPLE_FAULT_INJECTOR("resource_group_give_away_wait_latch");
				*/
				/*
				 * do check here to clean all target's moveto* params in case
				 * of interruption or any exception
				 */
				CHECK_FOR_INTERRUPTS();

				latchRes = WaitLatch(&MyProc->procLatch,
				   WL_LATCH_SET | WL_TIMEOUT | WL_POSTMASTER_DEATH,
				   timeout,
				   PG_WAIT_RESOURCE_GROUP);

				if (latchRes & WL_POSTMASTER_DEATH)
					elog(ERROR,
					 "got WL_POSTMASTER_DEATH waiting on latch; exiting...");
			}
			PG_CATCH();
			{
				clean = true;
				ResGroupMoveCheckTargetReady(sessionId, &clean, &res);
				if (res)
				{
					/*
					 * clean slot variable, because we don't need to touch it
					 * in current process as control is on the target side
					 */
					*slot = NULL;
					ereport(WARNING,
							(errmsg("got exception, but slot control is on the target process side"),
							 errhint("QEs weren't moved. They'll be moved by the next command dispatched in the target transaction, if any.")));
				}
				PG_RE_THROW();
			}
			PG_END_TRY();
		}
		else
			latchRes = WL_TIMEOUT;
		clean = (latchRes & WL_TIMEOUT);
		ResGroupMoveCheckTargetReady(sessionId, &clean, &res);
		if (clean)
			break;
		ResetLatch(&MyProc->procLatch);
	}

	if (!res)
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 (errmsg("target process failed to move to a new group"))));
}

void
ResGroupMoveQuery(int sessionId, Oid groupId, const char *groupName)
{
	ResGroupInfo groupInfo;
	ResGroupData *group;
	ResGroupSlotData *slot;

	Assert(pResGroupControl != NULL);
	/* Assert(pResGroupControl->segmentsOnCoordinator > 0); */

	LWLockAcquire(ResGroupLock, LW_SHARED);
	group = groupHashFind(groupId, false);
	LWLockRelease(ResGroupLock);
	if (!group)
	{
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 (errmsg("invalid resource group id: %d", groupId))));
	}
	groupInfo.group = group;
	groupInfo.groupId = groupId;
	slot = groupAcquireSlot(&groupInfo, true);
	if (slot == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_RESOURCES),
				 (errmsg("cannot get slot in resource group %d", groupId))));
	PG_TRY();
	{
		resGroupGiveSlotAway(sessionId, &slot, groupId);
	}
	PG_CATCH();
	{
		/*
		 * There can be exceptional situations, when slot is already on the
		 * target side. Release slot only if available.
		 */
		if (slot)
		{
			LWLockAcquire(ResGroupLock, LW_EXCLUSIVE);
			groupReleaseSlot(group, slot, true);
			LWLockRelease(ResGroupLock);
		}
		PG_RE_THROW();
	}
	PG_END_TRY();
}

/*
 * After getting the plan of a query, it must be inside
 * a transaction which means it must already hold a resgroup
 * slot. For some cases, we can unassign to save a concurrency
 * slot and other resources (just like bypass):
 *   - for explicit transaction block (begin; end), don't do it
 *     because for following SQLs it will not try to enter resgroup
 *   - pure catalog query or very simple query (no rangetable and
 *     no function)
 *   - if the total cost is smaller than resgroup configured mincost
 */
void
check_and_unassign_from_resgroup(PlannedStmt* stmt)
{
	bool         inFunction;
	ResGroupInfo groupInfo;

	if (!IsNormalProcessingMode() ||
		!IsResGroupActivated() ||
		bypassedGroup != NULL)
		return;
	/*
	 * Don't need to consider the sql commands inside the UDF, they will also
	 * be bypassed or use the same resgroup as the outer query.
	 */
	inFunction = already_under_executor_run() || utility_nested();
	if (IsInTransactionBlock(!inFunction))
		return;
	/*
	 * If none of the bypass(unassign) rule satisfy, return directly
	 */
	if (!can_bypass_based_on_plan_cost(stmt) &&
		!(pg_resource_group_bypass_catalog_query && is_pure_catalog_plan(stmt)))
		return;
	/* Unassign from resgroup and bypass */
	UnassignResGroup();
	do
	{
		decideResGroup(&groupInfo);
	} while (!groupIncBypassedRef(&groupInfo));
	bypassedGroup = groupInfo.group;
	bypassedGroup->totalExecuted++;
	/* pgstat_report_resgroup(bypassedGroup->groupId); */
	bypassedSlot.group = groupInfo.group;
	bypassedSlot.groupId = groupInfo.groupId;
	cgroupOpsRoutine->attachcgroup(bypassedGroup->groupId, MyProcPid,
								   bypassedGroup->caps.cpuMaxPercent == CPU_MAX_PERCENT_DISABLED);
}

/*
 * return ture if there is a resource group which io_limit contains tblspcid.
 * if errout is true, print warning, message. */
bool
checkTablespaceInIOlimit(Oid tblspcid, bool errout)
{
	Relation rel_resgroup_caps;
	SysScanDesc	sscan;
	HeapTuple	tuple;
	bool		contain = false;
	bool		print_header = false;
	StringInfo  log = makeStringInfo();

	rel_resgroup_caps = table_open(ResGroupCapabilityRelationId, AccessShareLock);
	/* get io limit string from catalog */
	sscan = systable_beginscan(rel_resgroup_caps, InvalidOid, false,
							   NULL, 0, NULL);
	while (HeapTupleIsValid(tuple = systable_getnext(sscan)))
	{
		Oid id;
		bool isNULL;
		Datum id_datum;
		Datum type_datum;
		Datum value_datum;
		ResGroupLimitType type;
		char *io_limit_str;
		List *limit_list;
		ListCell *cell;

		type_datum = heap_getattr(tuple, Anum_pg_resgroupcapability_reslimittype,
								  rel_resgroup_caps->rd_att, &isNULL);
		type = (ResGroupLimitType) DatumGetInt16(type_datum);
		if (type != RESGROUP_LIMIT_TYPE_IO_LIMIT)
			continue;
		id_datum = heap_getattr(tuple, Anum_pg_resgroupcapability_resgroupid,
								rel_resgroup_caps->rd_att, &isNULL);
		id = DatumGetObjectId(id_datum);
		value_datum = heap_getattr(tuple, Anum_pg_resgroupcapability_value,
								   rel_resgroup_caps->rd_att, &isNULL);
		io_limit_str = TextDatumGetCString(value_datum);
		if (strcmp(io_limit_str, DefaultIOLimit) == 0)
			continue;
		limit_list = cgroupOpsRoutine->parseio(io_limit_str);
		foreach(cell, limit_list)
		{
			TblSpcIOLimit *limit = (TblSpcIOLimit *) lfirst(cell);

			if (limit->tablespace_oid == tblspcid)
			{
				contain = true;

				if (!errout)
					break;
				if (!print_header)
				{
					print_header = true;
					appendStringInfo(log, "io limit: following resource groups depend on tablespace %s:", get_tablespace_name(tblspcid));
				}
				appendStringInfo(log, " %s", GetResGroupNameForId(id));
			}
		}
		cgroupOpsRoutine->freeio(limit_list);
		if (contain && !errout)
			break;
	}
	systable_endscan(sscan);
	table_close(rel_resgroup_caps, AccessShareLock);
	if (contain && errout)
		ereport(ERROR, (errcode(ERRCODE_DEPENDENT_OBJECTS_STILL_EXIST),
						errmsg("%s", log->data),
						errhint("you can remove those resource groups or remove tablespace %s from io_limit of those resource groups.", get_tablespace_name(tblspcid))));

	pfree(log->data);
	pfree(log);

	return contain;
}

/*
 * Get the max session memory limit for the user,
 * calculated as (memory_limit / concurrency).
 */
int64
get_group_memory_limit(void)
{
	ResGroupCaps   *caps;
	int64			queryMem = -1;

	LWLockAcquire(ResGroupLock, LW_SHARED);

	if (!self || !self->group || self->group->caps.concurrency <= 0)
	{
		LWLockRelease(ResGroupLock);
		return queryMem;
	}

	caps = &self->group->caps;
	if (caps->memory_limit > 0)
		queryMem = (int64)(caps->memory_limit *1024L *1024L / caps->concurrency);

	LWLockRelease(ResGroupLock);

	return queryMem;
}

/*
 * Given a planned statement, check if it is pure catalog query or a very simple query.
 * Return true only when:
 *   - there must be only one slice
 *   - there is no FuncExpr in target list
 *   - range table cannot contain FUNCTION or TABLEFUNC
 *   - range table must be catalog if it is RTE_RELATION
 */
static bool
is_pure_catalog_plan(PlannedStmt *stmt)
{
	ListCell *rtable;
	List     *func_tag;

	/* For catalog SQL, we only consider SELECT stmt. */
	if (stmt->commandType != CMD_SELECT)
		return false;
	if (stmt->planTree->targetlist != NIL)
	{
		int    pos;
		func_tag = list_make1_int(T_FuncExpr);
		pos = find_nodes((Node *) (stmt->planTree->targetlist), func_tag);
		list_free(func_tag);
		if (pos >= 0)
			return false;
	}
	foreach(rtable, stmt->rtable)
	{
		RangeTblEntry *rte = (RangeTblEntry *) lfirst(rtable);

		if (rte->rtekind == RTE_FUNCTION ||
			rte->rtekind == RTE_TABLEFUNC)
			return false;
		if (rte->rtekind != RTE_RELATION)
			continue;
		if (rte->relkind == RELKIND_MATVIEW)
			return false;
		if (rte->relkind == RELKIND_VIEW)
			continue;
		if(!IsCatalogRelationOid(rte->relid))
			return false;
	}

	return true;
}

static bool
can_bypass_based_on_plan_cost(PlannedStmt *stmt)
{
	ResGroupCaps *caps = &self->group->caps;
	int           min_cost;

	min_cost = (int) pg_atomic_read_u32((pg_atomic_uint32 *) &caps->min_cost);
	if(stmt->planTree)
	{
		return stmt->planTree->total_cost < min_cost;
	}
	else 
	{
		return true;
	}
}
/* End - ReqID:SRS-SECURITY-RESOURCEMANAGEMENT */
