/*-------------------------------------------------------------------------
 *
 * resgroup.h
 *	  GPDB resource group definitions.
 *
 *
 * Portions Copyright (c) 2006-2017, Greenplum inc.
 * Portions Copyright (c) 2012-Present VMware, Inc. or its affiliates.
 *
 * Authored by zhangyilin@highgo.com
 * Portions Copyright (c) 2009-2024, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	    src/include/utils/resgroup.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef RES_GROUP_H
#define RES_GROUP_H



#include "postgres.h"

#include "catalog/pg_resgroup.h"
#include "utils/cgroup.h"

#include "utils/rel.h"
#include "nodes/plannodes.h"


/*
 * The max number of resource groups.
 */
#define MaxResourceGroups 100

/*
 * The max length of cpuset
 */
#define MaxCpuSetLength 1024

/*
 * Default value of cpuset
 */
#define DefaultCpuset "-1"

/*
 * Default value of io_limit
 */
#define DefaultIOLimit "-1"

/*
 * Default value of cpu weight
 */
#define DefaultCPUWeight 100

/*
 * Resource group capability.
 */
typedef int32 ResGroupCap;

/*
 * Resource group capabilities.
 *
 * These are usually a snapshot of the pg_resgroupcapability table
 * for a resource group.
 *
 * The properties must be in the same order as ResGroupLimitType.
 *
 * This struct can also be converted to an array of ResGroupCap so the fields
 * can be accessed via index and iterated with loop.
 *
 *     ResGroupCaps caps;
 *     ResGroupCap *array = (ResGroupCap *) &caps;
 *     caps.concurrency.value = 1;
 *     array[RESGROUP_LIMIT_TYPE_CONCURRENCY] = 2;
 *     Assert(caps.concurrency.value == 2);
 */
typedef struct ResGroupCaps
{
	ResGroupCap		__unknown;			/* placeholder, do not use it */
	ResGroupCap		concurrency;
	ResGroupCap		cpuMaxPercent;
	ResGroupCap		cpuWeight;
	ResGroupCap		memory_limit;
	volatile ResGroupCap	min_cost;

	/*
	 * io_limit is a pointer in TopMemoryContext,
	 * This cell of list should be converted to TblSpcIOLimit when use.
	 */
	List			*io_limit;

	char			cpuset[MaxCpuSetLength];
} ResGroupCaps;

/* Set 'cpuset' to an empty string, and reset all other fields to zero */
#define ClearResGroupCaps(caps) do { \
	MemSet((caps), 0, offsetof(ResGroupCaps, cpuset) + 1); \
} while(0)


/*
 * GUC variables.
 */
extern bool						pg_resgroup_debug_wait_queue;

extern double pg_resource_group_cpu_limit;
extern int pg_resource_group_cpu_priority;

extern bool pg_resource_group_bypass;
extern bool pg_resource_group_bypass_catalog_query;

extern int pg_resource_group_queuing_timeout;
extern int pg_resource_group_move_timeout;

extern bool pg_resource_group_disable_notice_scansup;

/*
 * Non-GUC global variables.
 */
extern bool pg_resource_group_enable_cgroup_cpuset;

typedef enum ResGroupLimitType
{
	RESGROUP_LIMIT_TYPE_UNKNOWN = 0,

	RESGROUP_LIMIT_TYPE_CONCURRENCY,	/* concurrency */
	RESGROUP_LIMIT_TYPE_CPU,			/* cpu_max_percent */
	RESGROUP_LIMIT_TYPE_CPU_SHARES,		/* cpu_weight */
	RESGROUP_LIMIT_TYPE_CPUSET,			/* cpuset */
	RESGROUP_LIMIT_TYPE_MEMORY_LIMIT,	/* memory_limit */
	RESGROUP_LIMIT_TYPE_MIN_COST,		/* min_cost */
	RESGROUP_LIMIT_TYPE_IO_LIMIT,		/* io_limit */
	RESGROUP_LIMIT_TYPE_ROLEID,			/* roleid */
	RESGROUP_LIMIT_TYPE_COUNT,
} ResGroupLimitType;

/* Type of statistic information */
typedef enum
{
	RES_GROUP_STAT_UNKNOWN = -1,

	RES_GROUP_STAT_NRUNNING = 0,
	RES_GROUP_STAT_NQUEUEING,
	RES_GROUP_STAT_TOTAL_EXECUTED,
	RES_GROUP_STAT_TOTAL_QUEUED,
	RES_GROUP_STAT_TOTAL_QUEUE_TIME,
	RES_GROUP_STAT_CPU_USAGE,
} ResGroupStatType;

/*
 * The context to pass to callback in CREATE/ALTER/DROP resource group
 */
typedef struct
{
	Oid					groupid;
	ResGroupLimitType	limittype;
	ResGroupCaps		caps;
	ResGroupCaps		oldCaps;	/* last config value, alter operation need to
 										* check last config for recycling */

	List				*ioLimit;
} ResourceGroupCallbackContext;


typedef struct SessionState
{
	int			pid;			/* Backend's process ID; 0 if prepared xact */

	/* Next in the list. If the current entry is in free list,
	 * the next would point to the next entry in the free list.
	 * If, however, the current entry is in the used list, the
	 * next entry would point to the next used entry */
	struct SessionState *next;

	/*
	 * Resource group per-session slot information.
	 */
	void *resGroupSlot;

} SessionState;

/*
 * Contains pointer to the shared memory array of SessionState entries.
 * Additionally, we save the number of used entries, and the total number
 * of entries, along with free list and used list for these entries.
 */
typedef struct SessionStateArray
{
	/* Number of allocated entries */
	int			numSession;
	/* Maximum number of entries */
	int			maxSession;

	/* Head of the list of free entries */
	SessionState *freeList;
	/* Head of the list of used entries */
	SessionState *usedList;

	/* Head of the entries */
	SessionState sessions[1];		/* VARIABLE LENGTH ARRAY */
} SessionStateArray;

extern volatile SessionState *MySessionState;
extern volatile SessionStateArray *AllSessionStateEntries;

extern Size SessionState_ShmemSize(void);
extern void SessionState_ShmemInit(void);
extern void SessionState_Init(void);
extern void SessionState_Shutdown(void);
extern bool SessionState_IsAcquired(SessionState *sessionState);


/* Shared memory and semaphores */
extern Size ResGroupShmemSize(void);
extern void ResGroupControlInit(void);
extern void CGroupOpsAndInfoInit(void);

extern void initCgroup(void);
/* Load resource group information from catalog */
extern void	InitResGroups(void);

extern void AllocResGroupEntry(Oid groupId, const ResGroupCaps *caps);

extern void SerializeResGroupInfo(StringInfo str);
extern void DeserializeResGroupInfo(struct ResGroupCaps *capsOut,
									Oid *groupId,
									const char *buf,
									int len);

extern bool ShouldAssignResGroup(void);
extern bool ShouldUnassignResGroup(void);
extern void AssignResGroup(void);
extern void UnassignResGroup(void);

extern bool ResGroupIsAssigned(void);

/* Retrieve statistic information of type from resource group */
extern Datum ResGroupGetStat(Oid groupId, ResGroupStatType type);

extern void ResGroupDropFinish(const ResourceGroupCallbackContext *callbackCtx,
							   bool isCommit);
extern void ResGroupCreateOnAbort(const ResourceGroupCallbackContext *callbackCtx);
extern void ResGroupAlterOnCommit(const ResourceGroupCallbackContext *callbackCtx);
extern void ResGroupCheckForDrop(Oid groupId, char *name);
extern void check_and_unassign_from_resgroup(PlannedStmt* stmt);
extern uint64 ResourceGroupGetQueryMemoryLimit(void);

/*
 * Get resource group id of my proc.
 *
 * This function is not dead code although there is no consumer in the hgdb
 * code tree.  Some extensions require this to get the internal resource group
 * information.
 */
extern Oid GetMyResGroupId(void);

extern void ResGroupDumpInfo(StringInfo str);

extern int ResGroupGetHostPrimaryCount(void);

extern Bitmapset *CpusetToBitset(const char *cpuset,
								 int len);
extern void BitsetToCpuset(const Bitmapset *bms,
							char *cpuset,
							int cpusetSize);
extern void CpusetUnion(char *cpuset1, const char *cpuset2, int len);
extern void CpusetDifference(char *cpuset1, const char *cpuset2, int len);
extern bool CpusetIsEmpty(const char *cpuset);
extern void SetCpusetEmpty(char *cpuset, int cpusetSize);
extern bool EnsureCpusetIsAvailable(int elevel);
extern void HandleMoveResourceGroup(void);
extern void ResGroupMoveQuery(int sessionId, Oid groupId, const char *groupName);
extern Oid ResGroupGetGroupIdBySessionPid(int sessionPid);
extern char *getCpuSetByRole(const char *cpuset);
extern void checkCpuSetByRole(const char *cpuset);
extern bool checkTablespaceInIOlimit(Oid tblspcid, bool errout);
extern int64 get_group_memory_limit(void);

#endif   /* RES_GROUP_H */
