/* Begin ReqID: SRS-SESSION-VIEWS */
/*-------------------------------------------------------------------------
 *
 * hg_proc_data.c
 *	  Functions for accessing the session statistic data
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src\backend\utils\adt\hg_proc_data.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/xact.h"
#include "catalog/namespace.h"
#include "tcop/utility.h"
#include "utils/builtins.h"
#include "utils/hg_proc_data.h"
#include "storage/ipc.h"

extern int MaxConnections;
extern unsigned long g_busy_secs;
extern unsigned long g_busy_microsecs;
extern TimestampTz g_before_time;
/*extern TimestampTz MyStartTimestamp;*/

/* false means idle */
extern bool g_proc_is_busy; 
extern bool should_lock_current_session;

static HTAB *ProcPrivDataHash = NULL;

/* The shared process data lock. */
LWLock		*proc_data_lock;			/* mutual exclusion */

/* ReqID: SRS-SESSION-VIEWS  
 * get the share memory size of the proc data hash table */
Size
ProcDataShmemSize(void) 
{
	Size shmemSize;
	shmemSize = add_size(sizeof(LWLock), hash_estimate_size(MaxConnections, sizeof(ProcPrivEnt)));
	elog(DEBUG5, "invoking ProcDataShmemSize(shmemSize=%zu)", shmemSize);

	return shmemSize;
}

/*
 * Initialize shmem hash table for mapping buffers
 *		size is the desired hash table size (possibly more than NBuffers)
 */
void
ProcDataShmemInit(void)
{
	HASHCTL		info;
	bool found;

	proc_data_lock = ShmemInitStruct("Session Status Data Lock", sizeof(LWLock), &found);

	if (!found)
	{
		LWLockInitialize(proc_data_lock, LWLockNewTrancheId());
	}
	
	LWLockRegisterTranche(proc_data_lock->tranche, "Session Status data lock");

	/* BufferTag maps to Buffer */
	info.keysize = sizeof(int);
	info.entrysize = sizeof(ProcPrivEnt);

	ProcPrivDataHash = ShmemInitHash("Proc Priv Data Lookup Table",
								  MaxConnections, MaxConnections,
								  &info, HASH_ELEM | HASH_BLOBS);
	RegisterOnExithandler();
}

/* look up information in the hash table. */
unsigned int
ProcDataTableLookup(const int *tagPtr, u_int32_t *sid, char *idle_sum, 
	char *busy_num, char *trans_info, u_int32_t *tid, char* path_search, bool *islock)
{
	ProcPrivEnt *result;
	LWLockAcquire(proc_data_lock, LW_SHARED);

	result = (ProcPrivEnt *)
		hash_search(ProcPrivDataHash, (const void *) tagPtr, HASH_FIND, NULL);

	if (!result) {
		LWLockRelease(proc_data_lock);
		return -1;
	}

	*sid = result->data.sid;
	strcpy(idle_sum, result->data.idle_sum);
	strcpy(busy_num, result->data.busy_num);
	strcpy(trans_info, result->data.trans_info);
	if(namespace_search_path != NULL)
	{
		if(strlen(namespace_search_path) < (8 * PROC_INFO_STR_LEN - 1 ))
		{
			strcpy(path_search, namespace_search_path);
		}
		else
		{
			strncpy(path_search, namespace_search_path, 8 * PROC_INFO_STR_LEN - 2);
			path_search[8 * PROC_INFO_STR_LEN - 1] = '\0';
		}
	}
	else
	{
		strcpy(path_search, "public");
	}
	*tid = result->data.tid;
	*islock = result->data.is_lock;

	LWLockRelease(proc_data_lock);
	return *sid;
}

/* ReqID: SRS-SESSION-VIEWS 
 * ProcDataTableInsert
 * format time to string . */
static void format_time(unsigned long secs, char *delta_time)
{

	unsigned long		mins;
	unsigned long		hours;
	unsigned long		days;
	mins = secs / 60;
	if(mins)
	{
		secs = secs % 60;
		hours = mins / 60;
		if(hours)
		{
			mins = mins % 60;
			days = mins / 24;
			if(days)
			{
				sprintf(delta_time, "%ldd%ldh%ldm%lds", days, hours, mins, secs);
			}
			else
			{
				sprintf(delta_time, "%ldh%ldm%lds", hours, mins, secs);
			}
		}
		else
		{
			sprintf(delta_time, "%ldm%lds", mins, secs);
		}
	}
	else
	{
		sprintf(delta_time, "%lds", secs);
	}
}

/* ReqID: SRS-SESSION-VIEWS 
 * ProcDataTableInsert
 * insert proc data to hash table, update if exists. */
void
ProcDataTableInsert(const int *tagPtr, int sid, int trans_state, unsigned int tid)
{
	ProcPrivEnt	   *result;
	char			delta_time_idle[32] = {'\0'};
	char			delta_time_busy[32] = {'\0'};
	long			secs;
	unsigned long	idle_secs = 0;
	static long		old_idle_secs = 0;
	int				microsecs;

	unsigned long	tmp_busy_secs = 0;
	unsigned long	tmp_busy_microsecs = 0;
	unsigned long	tmp = 0;

	LWLockAcquire(proc_data_lock, LW_EXCLUSIVE);

	result = (ProcPrivEnt *)
		hash_search(ProcPrivDataHash,
									(void *) tagPtr,
									HASH_ENTER,
									NULL);
	result->data.sid = sid;

	/* get first idle time, there are two situations
	 * ont is busy, one is idle. if it is idle,
	 * we need to calculate the time from idle to the current time.
	 */
	if (count_idle_busy_time)
	{
		elog(DEBUG3, "Counting busy time, g_proc_is_busy:%d", g_proc_is_busy);
		if(g_proc_is_busy)
		{
			TimestampDifference(g_before_time, GetCurrentTimestamp(),
							&secs, &microsecs);
			tmp_busy_secs = (unsigned long)secs + g_busy_secs;
			tmp_busy_microsecs	= microsecs + g_busy_microsecs;
			tmp = tmp_busy_microsecs / USECS_PER_SEC;
			if(tmp > 0)
			{
				tmp_busy_secs = tmp_busy_secs + tmp;
				tmp_busy_microsecs = tmp_busy_microsecs %  USECS_PER_SEC;
			}
		}
		else
		{
			tmp_busy_secs = g_busy_secs;
			tmp_busy_microsecs = g_busy_microsecs;
		}

		format_time(tmp_busy_secs, delta_time_busy);

		/* get total time */
		TimestampDifference(MyStartTimestamp, GetCurrentTimestamp(),
							&secs, &microsecs);
		idle_secs = secs - tmp_busy_secs;

		/* eg:3s - 3s - 2s -3s - 4s - 3s */
		if(old_idle_secs == idle_secs + 1)
		{
			idle_secs = old_idle_secs;
		}
		else
		{
			old_idle_secs = idle_secs;
		}
		
		format_time(idle_secs, delta_time_idle);

		elog(DEBUG1, "Finish counting busy time, time_idle:%s, time_busy:%s",
				delta_time_idle, delta_time_busy);
		strcpy(result->data.idle_sum, delta_time_idle);
		strcpy(result->data.busy_num, delta_time_busy);
	}

	strcpy(result->data.trans_info, TransStateIntAsString(trans_state));
	result->data.tid = tid;
	result->data.is_lock = should_lock_current_session;

	elog(DEBUG1, "ProcDataTableInsert, pid:%d, sid:%d, trans_state:%d, tid:%d, is_lock:%d", 
			*tagPtr, sid, trans_state, tid, result->data.is_lock);

	LWLockRelease(proc_data_lock);
}

/* ReqID: SRS-SESSION-VIEWS 
 * ProcDataTableDelete
 * delete information from the hash table, if exists. */
void
ProcDataDelete(const int *tagPtr)
{
	LWLockAcquire(proc_data_lock, LW_EXCLUSIVE);

	hash_search(ProcPrivDataHash, (void *) tagPtr, HASH_REMOVE,	NULL);
	LWLockRelease(proc_data_lock);
}

/* ReqID: SESSION-VIEWS 
 * hook function to delete information from the hash table */
void ClearProcDataOnExit(int code, Datum arg)
{
	ProcDataDelete(&MyProcPid);
}

/* Register the hook function to delete information from the hash table 
 * when the backend process is exiting */
void
RegisterOnExithandler(void)
{
	static bool already_done = false;

	if (already_done)
		return;
	before_shmem_exit(ClearProcDataOnExit, 0);
	already_done = true;
}

/* End ReqID: SESSION-VIEWS */
