/*------------------------------------------------------------------
 * 
 * File: numa.c
 *
 * Abstract: 
 * 	 The file where the NUMA optimization support function resides.
 *
 * Authored by liyuchen@highgo.com, 20241219.
 *
 * Copyright:
 * Copyright (c) 2024, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 * 	 contrib/hg_perf_numa/numa.c
 *
 *------------------------------------------------------------------
 */

#ifndef WIN32
#ifndef _WIN64

#include <postgres.h>

#include "fmgr.h"
#include "storage/shmem.h"
#include "utils/guc.h"
#include "storage/proc.h"
#include <numa.h>
#include "storage/pg_shmem.h"
#include "storage/fd.h"
#include "miscadmin.h"
#include <unistd.h>
#include <sched.h>
#include "storage/s_lock.h"
#include "storage/spin.h"
#include "storage/ipc.h"
#include "access/xlog.h"
#include "storage/lwlock.h"

PG_MODULE_MAGIC;

/* Saved hooks' value in case of unload */
proc_global_shmem_size_hook_type	prev_proc_global_shmem_size_hook = NULL;
proc_global_shmem_init_hook_type	prev_proc_global_shmem_init_hook = NULL;
init_process_hook_type				prev_init_process_hook = NULL;
wal_hugepage_shmem_size_hook_type	prev_wal_hugepage_shmem_size_hook = NULL;
wal_insert_lock_size_hook_type		prev_wal_insert_lock_size_hook = NULL;
xlog_shmem_hook_type				prev_xlog_shmem_hook = NULL;
xlog_shmem_init_hook_type			prev_xlog_shmem_init_hook = NULL;
xlog_insert_record_hook_type		prev_xlog_insert_record_hook = NULL;
wal_insert_lock_release_hook_type	prev_wal_insert_lock_release_hook = NULL;
wal_insert_lock_acquire_hook_type	prev_wal_insert_lock_acquire_hook = NULL;
wal_insert_lock_acquire_ex_hook_type	prev_wal_insert_lock_acquire_ex_hook = NULL;
wal_insert_lock_uia_hook_type 			prev_wal_insert_lock_uia_hook = NULL;
wal_insert_lock_tofin_hook_type 		prev_wal_insert_lock_tofin_hook = NULL;
get_last_important_rec_ptr_hook_type	prev_get_last_important_rec_ptr_hook = NULL;
//_hook_type	prev__hook = NULL

/* GUC parameters */
extern bool enable_numa;
extern bool hugepage_debug;
extern bool sp_numa;
extern bool enable_bind_numa_core;
extern int num_xloginsert_locks;

/* int num_xloginsert_locks = 16; */

/* normal variable */
extern bool hugePageAvailable;
Size insertLockSize = 0;
int nNumaNodesGlobal = 0;
int num_locks_in_group_global = 0;


extern void _PG_init(void);
extern void _PG_fini(void);

extern PGShmemHeader *ShmemSegHdr;
extern slock_t *ShmemLock;
extern void *ShmemBase;

static void bind_cpu (unsigned int cpuNo);
static Size get_huge_page_size (void);
static bool SysHugePageAvailable (void);
static void* ShmemAllocPageSizeAligned (Size size);
static void* ShmemAllocPageSizeAlignedNoError (Size size);
static Size ProcGlobalShmemSizeNuma (Size TotalProcs);
static PGPROC** ProcGlobalShmemInitNuma (PGPROC **procs, uint32 TotalProcs);
static void InitPorcessNuma (PGPROC *MyProc);
static Size WalHugepageShmemSize (void);
static void WalInsertLockSize (Size lockSize);
static Size xlogShmemNuma (void);
void xlogShmemInitNuma (WALInsertLockPaddedType ***insertLockGroupPtr,
						WALInsertLockPaddedType **WALInsertLocks,
						WALInsertLockPaddedType ***XLOGWALInsertLocks,
						WALInsertLockPaddedType ***GlobalWALInsertLocks,
						WALInsertLockPaddedType **LocalGroupWALInsertLocks,
						char **allocptr);
void xlog_insert_record (int lockno,
						bool holdingAllLocks,
						WALInsertLockPaddedType **WALInsertLocks,
						WALInsertLockPaddedType ***GlobalWALInsertLocks,
						XLogRecPtr StartPos);
void wal_insert_lock_acquire (int *MyLockNo,
							WALInsertLockPaddedType **WALInsertLocks,
							WALInsertLockPaddedType ***GlobalWALInsertLocks);
void wal_insert_lock_acquire_ex (bool *holdingAllLocks,
							WALInsertLockPaddedType **WALInsertLocks,
							WALInsertLockPaddedType ***GlobalWALInsertLocks);
void wal_insert_lock_release (bool *holdingAllLocks,
							int MyLockNo,
							WALInsertLockPaddedType **WALInsertLocks,
							WALInsertLockPaddedType ***GlobalWALInsertLocks);
void wal_insert_lock_uia (XLogRecPtr insertingAt,
						bool holdingAllLocks,
						int MyLockNo,
						WALInsertLockPaddedType **WALInsertLocks,
						WALInsertLockPaddedType ***GlobalWALInsertLocks);
XLogRecPtr wal_insert_lock_tofin (XLogRecPtr upto,
								XLogRecPtr finishedUpto,
								WALInsertLockPaddedType **WALInsertLocks,
								WALInsertLockPaddedType ***GlobalWALInsertLocks);
void get_last_important_rec_ptr (XLogRecPtr *res,
								WALInsertLockPaddedType **WALInsertLocks,
								WALInsertLockPaddedType ***GlobalWALInsertLocks);

void get_last_important_rec_ptr (XLogRecPtr *res,
								WALInsertLockPaddedType **WALInsertLocks,
								WALInsertLockPaddedType ***GlobalWALInsertLocks)
{
	int 		processorIndex = 0;
	int			i = 0;

	if(enable_numa)
	{
		for (processorIndex = 0; processorIndex < nNumaNodesGlobal; processorIndex++)
		{
			for (i = 0; i < num_locks_in_group_global; i++)
			{
				XLogRecPtr	last_important;
				LWLockAcquire((void *)&(*GlobalWALInsertLocks)[processorIndex][i].l.lock, LW_EXCLUSIVE);
				last_important = (*GlobalWALInsertLocks)[processorIndex][i].l.lastImportantAt;
				LWLockRelease((void *)&(*GlobalWALInsertLocks)[processorIndex][i].l.lock);

				if (*res < last_important)
					*res = last_important;
			}
		}
	}
	else
	{
		for (i = 0; i < num_xloginsert_locks; i++)
		{
			XLogRecPtr	last_important;

			/*
			 * Need to take a lock to prevent torn reads of the LSN, which are
			 * possible on some of the supported platforms. WAL insert locks only
			 * support exclusive mode, so we have to use that.
			 */
			LWLockAcquire((void *)&(*WALInsertLocks)[i].l.lock, LW_EXCLUSIVE);
			last_important = (*WALInsertLocks)[i].l.lastImportantAt;
			LWLockRelease((void *)&(*WALInsertLocks)[i].l.lock);

			if (*res < last_important)
				*res = last_important;
		}
	}
}

XLogRecPtr wal_insert_lock_tofin (XLogRecPtr upto,
								XLogRecPtr finishedUpto,
								WALInsertLockPaddedType **WALInsertLocks,
								WALInsertLockPaddedType ***GlobalWALInsertLocks)
{
	int i = 0;
	if(enable_numa)
	{
		int processorIndex = 0;
		for (processorIndex = 0; processorIndex < nNumaNodesGlobal; processorIndex++)
		{
			for (i = 0; i < num_locks_in_group_global; i++)
			{
				XLogRecPtr	insertingat = InvalidXLogRecPtr;
				do
				{
					WALInsertLockType *pInsertLock = &(*GlobalWALInsertLocks)[processorIndex][i].l;
					if (LWLockWaitForVar((void *)&pInsertLock->lock, &pInsertLock->insertingAt, insertingat, &insertingat)) {
						/* the lock was free, so no insertion in progress */
						insertingat = InvalidXLogRecPtr;
						break;
					}
				} while (insertingat < upto);

				if (insertingat != InvalidXLogRecPtr && insertingat < finishedUpto)
				finishedUpto = insertingat;
			}
		}
		return finishedUpto;
	}
	else
	{
		for (i = 0; i < num_xloginsert_locks; i++)
		{
			XLogRecPtr	insertingat = InvalidXLogRecPtr;

			do
			{
				/*
				 * See if this insertion is in progress.  LWLockWaitForVar will
				 * wait for the lock to be released, or for the 'value' to be set
				 * by a LWLockUpdateVar call.  When a lock is initially acquired,
				 * its value is 0 (InvalidXLogRecPtr), which means that we don't
				 * know where it's inserting yet.  We will have to wait for it. If
				 * it's a small insertion, the record will most likely fit on the
				 * same page and the inserter will release the lock without ever
				 * calling LWLockUpdateVar.  But if it has to sleep, it will
				 * advertise the insertion point with LWLockUpdateVar before
				 * sleeping.
				 */
				if (LWLockWaitForVar((void *)&(*WALInsertLocks)[i].l.lock,
									&(*WALInsertLocks)[i].l.insertingAt,
									insertingat, &insertingat))
				{
					/* the lock was free, so no insertion in progress */
					insertingat = InvalidXLogRecPtr;
					break;
				}

				/*
				* This insertion is still in progress. Have to wait, unless the
				* inserter has proceeded past 'upto'.
				*/
			} while (insertingat < upto);

			if (insertingat != InvalidXLogRecPtr && insertingat < finishedUpto)
				finishedUpto = insertingat;
		}
		return finishedUpto;
	}
}

void wal_insert_lock_uia (XLogRecPtr insertingAt,
						bool holdingAllLocks,
						int MyLockNo,
						WALInsertLockPaddedType **WALInsertLocks,
						WALInsertLockPaddedType ***GlobalWALInsertLocks)
{
	if(enable_numa)
	{
		if (holdingAllLocks)
		{
			WALInsertLockType *pInsertLock =
				&(*GlobalWALInsertLocks)[nNumaNodesGlobal - 1][num_locks_in_group_global - 1].l;
			LWLockUpdateVar((void *)&pInsertLock->lock, &pInsertLock->insertingAt, insertingAt);
		}
		else
		{
			LWLockUpdateVar((void *)&(*GlobalWALInsertLocks)[MyProc->numaNode][MyLockNo].l.lock,
							&(*GlobalWALInsertLocks)[MyProc->numaNode][MyLockNo].l.insertingAt,
							insertingAt);
		}
	}
	else
	{
		if (holdingAllLocks)
		{
			/*
			* We use the last lock to mark our actual position, see comments in
			* WALInsertLockAcquireExclusive.
			*/
			LWLockUpdateVar((void *)&(*WALInsertLocks)[num_xloginsert_locks - 1].l.lock,
							&(*WALInsertLocks)[num_xloginsert_locks - 1].l.insertingAt,
							insertingAt);
		}
		else
			LWLockUpdateVar((void *)&(*WALInsertLocks)[MyLockNo].l.lock,
							&(*WALInsertLocks)[MyLockNo].l.insertingAt,
							insertingAt);
	}
}												

void wal_insert_lock_release (bool *holdingAllLocks,
							int MyLockNo,
							WALInsertLockPaddedType **WALInsertLocks,
							WALInsertLockPaddedType ***GlobalWALInsertLocks)
{
	if(enable_numa)
	{
		if (*holdingAllLocks)
		{
			int i;
			int processorIndex = 0;
			for (processorIndex = 0; processorIndex < nNumaNodesGlobal; processorIndex++)
			{
				for (i = 0; i < num_locks_in_group_global; i++)
				{
					LWLockReleaseClearVar((void *)&(*GlobalWALInsertLocks)[processorIndex][i].l.lock,
										&(*GlobalWALInsertLocks)[processorIndex][i].l.insertingAt, 0);
				}
			}
			*holdingAllLocks = false;
		}
		else
		{
			LWLockReleaseClearVar((void *)&(*GlobalWALInsertLocks)[MyProc->numaNode][MyLockNo].l.lock,
								&(*GlobalWALInsertLocks)[MyProc->numaNode][MyLockNo].l.insertingAt,
								0);
    	}
	}
	else
	{
		if (*holdingAllLocks)
		{
			int			i;

			for (i = 0; i < num_xloginsert_locks; i++)
				LWLockReleaseClearVar((void *)&(*WALInsertLocks)[i].l.lock,
									&(*WALInsertLocks)[i].l.insertingAt,
									0);

			*holdingAllLocks = false;
		}
		else
		{
			LWLockReleaseClearVar((void *)&(*WALInsertLocks)[MyLockNo].l.lock,
								&(*WALInsertLocks)[MyLockNo].l.insertingAt,
								0);
		}
	}
}

void wal_insert_lock_acquire_ex (bool *holdingAllLocks,
							WALInsertLockPaddedType **WALInsertLocks,
							WALInsertLockPaddedType ***GlobalWALInsertLocks)
{
	int i;
	int	processorIndex;
	
	if(enable_numa)
	{
		for (processorIndex = 0; processorIndex < nNumaNodesGlobal; processorIndex++)
		{
			int limit = ((processorIndex < (nNumaNodesGlobal - 1)) ? num_locks_in_group_global
																: (num_locks_in_group_global - 1));
			for (i = 0; i < limit; i++) {
				WALInsertLockType *pInsertLock = &(*GlobalWALInsertLocks)[processorIndex][i].l;
				LWLockAcquire((void *)&pInsertLock->lock, LW_EXCLUSIVE);
				LWLockUpdateVar((void *)&pInsertLock->lock, &pInsertLock->insertingAt, PG_UINT64_MAX);
			}
		}
		/* Variable value reset to 0 at release */
		LWLockAcquire(
			(void *)&(*GlobalWALInsertLocks)[nNumaNodesGlobal - 1][num_locks_in_group_global - 1].l.lock,
			LW_EXCLUSIVE);

		*holdingAllLocks = true;
	}
	else
	{
		for (i = 0; i < num_xloginsert_locks - 1; i++)
		{
			LWLockAcquire((void *)&(*WALInsertLocks)[i].l.lock, LW_EXCLUSIVE);
			LWLockUpdateVar((void *)&(*WALInsertLocks)[i].l.lock,
							&(*WALInsertLocks)[i].l.insertingAt,
							PG_UINT64_MAX);
		}
		/* Variable value reset to 0 at release */
		LWLockAcquire((void *)&(*WALInsertLocks)[i].l.lock, LW_EXCLUSIVE);



		*holdingAllLocks = true;
	}
}

void wal_insert_lock_acquire (int *MyLockNo,
							WALInsertLockPaddedType **WALInsertLocks,
							WALInsertLockPaddedType ***GlobalWALInsertLocks)
{
	if(enable_numa)
	{
		bool		immed;
		static int	lockToTry = -1;
		if(lockToTry == -1)
		{
			lockToTry = (MyProc->pgprocno / nNumaNodesGlobal) % (num_locks_in_group_global);
		}
		*MyLockNo = lockToTry;

		immed = LWLockAcquire((void *)&(*GlobalWALInsertLocks)[MyProc->numaNode][*MyLockNo].l.lock, LW_EXCLUSIVE);

		if(!immed)
		{
			lockToTry = (lockToTry + 1) % (num_locks_in_group_global);
		}
	}
	else
	{
		bool		immed;
		static int	lockToTry = -1;

		if (lockToTry == -1)
			lockToTry = MyProc->pgprocno % num_xloginsert_locks;
		*MyLockNo = lockToTry;
		immed = LWLockAcquire((void *)&(*WALInsertLocks)[*MyLockNo].l.lock, LW_EXCLUSIVE);
		if (!immed)
		{
			lockToTry = (lockToTry + 1) % num_xloginsert_locks;
		}
	}
}

void xlog_insert_record (int lockno,
						bool holdingAllLocks,
						WALInsertLockPaddedType **WALInsertLocks,
						WALInsertLockPaddedType ***GlobalWALInsertLocks,
						XLogRecPtr StartPos)
{
	if(enable_numa)
	{
		if(holdingAllLocks)
			(*GlobalWALInsertLocks)[0][0].l.lastImportantAt = StartPos;
		else
			(*GlobalWALInsertLocks)[MyProc->numaNode][lockno].l.lastImportantAt = StartPos;
	}
	else
		(*WALInsertLocks)[lockno].l.lastImportantAt = StartPos;
}

void xlogShmemInitNuma (WALInsertLockPaddedType ***insertLockGroupPtr,
						WALInsertLockPaddedType **WALInsertLocks,
						WALInsertLockPaddedType ***XLOGWALInsertLocks,
						WALInsertLockPaddedType ***GlobalWALInsertLocks,
						WALInsertLockPaddedType **LocalGroupWALInsertLocks,
						char **allocptr)
{
	int			processorIndex = 0;
	Size		locksSizePerNumaNode;
	Size		hugePageSize = 0;
	Size		pageSize;
	uint32		num_locks_in_group;
	uint32 		nNumaNodes;
	int			i = 0;

	int numa_fix_offset[16] = {0, 5, 3, 3, 0, 5, 6, 6, 8, 8, 11, 11, 13, 13, 14, 14};

	nNumaNodes = numa_max_node() + 1;

	num_locks_in_group = num_xloginsert_locks / nNumaNodes + 
						(num_xloginsert_locks % nNumaNodes ? 1 : 0);
	if(enable_numa)
	{
		*insertLockGroupPtr = (WALInsertLockPaddedType **)
        		ShmemAlloc(num_xloginsert_locks * sizeof(WALInsertLockPaddedType *));

		if(hugePageAvailable)
		{
			hugePageSize = get_huge_page_size();
			locksSizePerNumaNode = TYPEALIGN(hugePageSize, 
				mul_size(num_locks_in_group, sizeof(WALInsertLockPaddedType)));
		}
		else
		{
			pageSize = (Size)getpagesize();
			locksSizePerNumaNode = TYPEALIGN(pageSize, 
				mul_size(num_locks_in_group, sizeof(WALInsertLockPaddedType)));
		}

		for (i = 0; i < nNumaNodes; i++)
		{
			(*insertLockGroupPtr)[i] = (WALInsertLockPaddedType *)ShmemAllocPageSizeAligned(locksSizePerNumaNode);
			if(sp_numa)
				numa_tonode_memory((*insertLockGroupPtr)[i], locksSizePerNumaNode, numa_fix_offset[i]);
			else
				numa_tonode_memory((*insertLockGroupPtr)[i], locksSizePerNumaNode, i);

			memset((*insertLockGroupPtr)[i], 0, locksSizePerNumaNode);
			if(hugepage_debug)
			{
				if(hugePageAvailable)
					elog(WARNING, "(HugePage) insertLockGroupPtr[%d] --> %p SizePerNumaNode = %lu", 
														i, (*insertLockGroupPtr)[i], locksSizePerNumaNode);
				else
					elog(WARNING, "(No HugePage) insertLockGroupPtr[%d] --> %p SizePerNumaNode = %lu", 
														i, (*insertLockGroupPtr)[i], locksSizePerNumaNode);
			}
        }
		*XLOGWALInsertLocks = *insertLockGroupPtr;
		*GlobalWALInsertLocks = *insertLockGroupPtr;
		*LocalGroupWALInsertLocks = (*GlobalWALInsertLocks)[0];
		for (processorIndex = 0; processorIndex < nNumaNodes; processorIndex++)
		{
			for (i = 0; i < num_locks_in_group; i++)
			{
				LWLockInitialize((void *)&(*GlobalWALInsertLocks)[processorIndex][i].l.lock,
								LWTRANCHE_WAL_INSERT);
				(*GlobalWALInsertLocks)[processorIndex][i].l.insertingAt = InvalidXLogRecPtr;
				(*GlobalWALInsertLocks)[processorIndex][i].l.lastImportantAt = InvalidXLogRecPtr;
			}
		}
	} 
	else
	{
		/* WAL insertion locks. Ensure they're aligned to the full padded size */
		*allocptr += sizeof(WALInsertLockPaddedType) -
			((uintptr_t) *allocptr) % sizeof(WALInsertLockPaddedType);
		*XLOGWALInsertLocks = (WALInsertLockPaddedType **)(allocptr);
		*WALInsertLocks = (**XLOGWALInsertLocks);
		*allocptr += sizeof(WALInsertLockPaddedType) * num_xloginsert_locks;

		for (i = 0; i < num_xloginsert_locks; i++)
		{
			LWLockInitialize((void *)&(*WALInsertLocks)[i].l.lock, LWTRANCHE_WAL_INSERT);
			(*WALInsertLocks)[i].l.insertingAt = InvalidXLogRecPtr;
			(*WALInsertLocks)[i].l.lastImportantAt = InvalidXLogRecPtr;
		}
	}
}

static Size xlogShmemNuma(void)
{
	Size	size = 0;
	uint32	nNumaNodes;
	uint32	num_locks_in_group;
	Size	locksSizePerNumaNode;
	if(enable_numa)
	{
		nNumaNodes = numa_max_node() + 1;
		nNumaNodesGlobal = nNumaNodes;

		/* If the number of numa nodes is greater than the number of locks, 
		   the number of locks needs to be replenished to the number of nodes */
		if(nNumaNodes > num_xloginsert_locks)
			num_xloginsert_locks = nNumaNodes;
		
		num_locks_in_group = num_xloginsert_locks / nNumaNodes + 
						(num_xloginsert_locks % nNumaNodes ? 1 : 0);
		num_locks_in_group_global = num_locks_in_group;

		locksSizePerNumaNode = TYPEALIGN((Size)getpagesize(), 
					mul_size(num_locks_in_group, insertLockSize));

		size = add_size(size, mul_size(locksSizePerNumaNode, nNumaNodes + 1));
	}
	else
	{
		size = add_size(size, mul_size(insertLockSize, num_xloginsert_locks + 1));
	}
	return size;
}

static void bind_cpu(unsigned int cpuNo)
{
	cpu_set_t mask;
		
	CPU_ZERO(&mask);    
    	CPU_SET(cpuNo, &mask);
	if (sched_setaffinity(0, sizeof(mask), &mask) == -1) {
		ereport(WARNING,(errmsg("sched_setaffinity failed!")));
	}   
		
}

static Size get_huge_page_size(void)
{
	Size		default_hugepagesize = 0;
	Size		hugepagesize;

	/*
	 * System-dependent code to find out the default huge page size.
	 *
	 * On Linux, read /proc/meminfo looking for a line like "Hugepagesize:
	 * nnnn kB".  Ignore any failures, falling back to the preset default.
	 */
#ifdef __linux__

	{
		FILE	   *fp = AllocateFile("/proc/meminfo", "r");
		char		buf[128];
		unsigned int sz;
		char		ch;

		if (fp)
		{
			while (fgets(buf, sizeof(buf), fp))
			{
				if (sscanf(buf, "Hugepagesize: %u %c", &sz, &ch) == 2)
				{
					if (ch == 'k')
					{
						default_hugepagesize = sz * (Size) 1024;
						break;
					}
					/* We could accept other units besides kB, if needed */
				}
			}
			FreeFile(fp);
		}
	}
#endif							/* __linux__ */

	if (huge_page_size != 0)
	{
		/* If huge page size is requested explicitly, use that. */
		hugepagesize = (Size) huge_page_size * 1024;
	}
	else if (default_hugepagesize != 0)
	{
		/* Otherwise use the system default, if we have it. */
		hugepagesize = default_hugepagesize;
	}
	else
	{
		/*
		 * If we fail to find out the system's default huge page size, or no
		 * huge page size is requested explicitly, assume it is 2MB. This will
		 * work fine when the actual size is less.  If it's more, we might get
		 * mmap() or munmap() failures due to unaligned requests; but at this
		 * writing, there are no reports of any non-Linux systems being picky
		 * about that.
		 */
		hugepagesize = 2 * 1024 * 1024;
	}

	return hugepagesize;
}

/*
 * ShmemAllocPageSizeAligned -- allocate page-aligned(address and size) chunk from shared memory
 *
 * Throws error if request cannot be satisfied.
 *
 * Assumes ShmemLock and ShmemSegHdr are initialized.
 */

static void*
ShmemAllocPageSizeAligned(Size size)
{
	void 	   *newSpace;

	newSpace = ShmemAllocPageSizeAlignedNoError(size);
	if (!newSpace)
		ereport(ERROR,
				(errcode(ERRCODE_OUT_OF_MEMORY),
				 errmsg("out of shared memory (%zu bytes requested)",
						size)));
	return newSpace;
}

/*
 * ShmemAllocPageSizeAlignedNoError -- allocate page-aligned(address and size) chunk from shared memory
 *
 * As ShmemAllocPageSizeAligned, but returns NULL if out of space, rather than erroring.
 */

static void*
ShmemAllocPageSizeAlignedNoError(Size size)
{
	Size 		newStart;
	Size 		newFree;
	void   	   *newSpace;
	Size 		pageSize;

	if (hugePageAvailable)
		pageSize = get_huge_page_size();
	else
		pageSize = (Size)getpagesize();

	size = TYPEALIGN(pageSize,size);
	Assert(ShmemSegHdr != NULL);

	SpinLockAcquire(ShmemLock);

	newStart = ShmemSegHdr->freeoffset;

	newStart = TYPEALIGN(pageSize,newStart);

	newFree = newStart + size;
	if (newFree <= ShmemSegHdr->totalsize)
	{
		newSpace = (void *) ((char *) ShmemBase + newStart);
		ShmemSegHdr->freeoffset = newFree;
	}
	else
		newSpace = NULL;

	SpinLockRelease(ShmemLock);
	return newSpace;
}

static bool
SysHugePageAvailable(void)
{
	Size		HugePages_Total = 0;
	Size		Hugetlb = 0;

#ifdef __linux__
	{
		FILE	   *fp = AllocateFile("/proc/meminfo", "r");
		char		buf[128];
		int			scanCount = 0;

		if (fp)
		{
			while (fgets(buf, sizeof(buf), fp))
			{
				if (sscanf(buf, "HugePages_Total: %lu", &HugePages_Total) == 1)
					scanCount += 1;
				if (sscanf(buf, "Hugetlb: %lu", &Hugetlb) == 1)
					scanCount += 1;
				if (scanCount == 2)
					break;
			}
			FreeFile(fp);
		}
	}
#else
	return false;//hugepage conditions of other platforms are not supported for the time being
#endif

	if (Hugetlb && HugePages_Total)
	{
		if (hugepage_debug)
			elog(WARNING, "hugepage will be tried to use");
		return true;
	}

	if (hugepage_debug)
		elog(WARNING, "hugepage available is false, no hugetlb or no hugepages free");

	return false;
}

static Size
ProcGlobalShmemSizeNuma(Size TotalProcs)
{
    int			nNumaNode;
	Size		pageSize;
	Size		hugePageSize;
	int			nProcsPerNumaNode;
	Size		procsTotalSize = 0;
    Size        size = 0;

    pageSize = (Size)getpagesize();

	nNumaNode = numa_max_node() + 1;

    /* Calculates the pre-allocated memory size */
	if (enable_numa && huge_pages != HUGE_PAGES_ON)
	{
		/* How many proc structs are in a NUMA node, rounded up */
		nProcsPerNumaNode = TotalProcs / nNumaNode + (TotalProcs % nNumaNode ? 1 : 0);

		/* 
		 * If there are N NUMA nodes, then the request for shared memory will 
		 * require N memory. For each block of memory to be allocated to NUMA nodes, 
		 * the size of the block of memory needs to be aligned with pagesize. 
		 * The return value of the innermost mul_size function here is the 
		 * unaligned size of a piece of memory, and we apply for its aligned 
		 * size space here. In addition, when you need to request space for 
		 * another page for the first memory request, its size is aligned with pagesize.
		 */
		size = add_size(size, mul_size(nNumaNode, 
					TYPEALIGN(pageSize, mul_size(nProcsPerNumaNode, sizeof(PGPROC)))));
		
		/* 
		 * An additional page is requested for the first requested 
		 * space to align with the memory page
		 */
		size = add_size(size, pageSize);

		if (sizeof(PGPROC) > pageSize)
		{
			elog(NOTICE, "The size of PGPROC is larger than memory page, \
									NUMA optimization will not be used");
			enable_numa = false;
		}
	}
    /* As long as the large page is not turned on, here we try to pre-allocate memory for the large page */
    if (enable_numa && huge_pages != HUGE_PAGES_OFF)
	{
		hugePageSize = get_huge_page_size();//The use of this function requires MAP_HUGETLB to be defined

		if (hugePageSize)
		{
			nProcsPerNumaNode = TotalProcs / nNumaNode + (TotalProcs % nNumaNode ? 1 : 0);

			procsTotalSize += mul_size(nNumaNode, TYPEALIGN(hugePageSize, 
									mul_size(nProcsPerNumaNode, sizeof(PGPROC))));
			procsTotalSize += hugePageSize;			

			if (SysHugePageAvailable())
				size = add_size(size, procsTotalSize);

			if (sizeof(PGPROC) > pageSize)
			{
				elog(NOTICE, "The size of PGPROC is larger than memory page, \
										NUMA optimization will not be used");
				enable_numa = false;
			}
		}
	}

    return size;
}

static PGPROC**
ProcGlobalShmemInitNuma(PGPROC **procs, uint32 TotalProcs)
{
	procs = (PGPROC **)ShmemAlloc(TotalProcs * sizeof(PGPROC *));
	if (enable_numa)
	{
		int		nNumaNode;
		int		nProcsPerNumaNode;
		Size	hugePageSize;
		Size	pageSize;
		Size	procsSizePerNumaNode;
		PGPROC	  **numa_temp_alloced;
		unsigned int	i = 0;
		unsigned int	pos = 0;
		unsigned int	g_ncpus = 0;
		unsigned int	currNumaNode = 0;

		int numa_fix_offset[16] = {0, 5, 3, 3, 0, 5, 6, 6, 8, 8, 11, 11, 13, 13, 14, 14};

		nNumaNode = numa_max_node() + 1;
		numa_temp_alloced = (PGPROC **)ShmemAlloc(nNumaNode * sizeof(PGPROC *));
		g_ncpus = sysconf(_SC_NPROCESSORS_ONLN);
		nProcsPerNumaNode = TotalProcs / nNumaNode + (TotalProcs % nNumaNode ? 1 : 0);

		if (hugePageAvailable)
		{
			hugePageSize = get_huge_page_size();
			procsSizePerNumaNode = TYPEALIGN(hugePageSize, 
								mul_size(nProcsPerNumaNode, sizeof(PGPROC)));

			for (i = 0; i < nNumaNode; i++)
			{
				numa_temp_alloced[i] = (PGPROC *)ShmemAllocPageSizeAligned(procsSizePerNumaNode);
				if (hugepage_debug)
					elog(WARNING, "(HugePage)NUMA node:%d --> %p, allocated size(per Numa node) = %lu",
								i, numa_temp_alloced[i], procsSizePerNumaNode);
				if (sp_numa)//condition of feiteng 16 NUMA nodes
					numa_tonode_memory(numa_temp_alloced[i], procsSizePerNumaNode, numa_fix_offset[i]);
				else
					numa_tonode_memory(numa_temp_alloced[i], procsSizePerNumaNode, i);

				memset(numa_temp_alloced[i], 0, procsSizePerNumaNode);
			}
			for (i = 0; i < TotalProcs; i++)
			{	
				pos = i / nNumaNode;
				procs[i] = (PGPROC *)(numa_temp_alloced[currNumaNode] + pos);

				MemSet(procs[i], 0, sizeof(PGPROC));
				procs[i]->numaNode = currNumaNode;
				procs[i]->cpuNo = i % g_ncpus % nNumaNode * (g_ncpus / nNumaNode)
								 + i % g_ncpus / nNumaNode;
				currNumaNode = (currNumaNode + 1) % nNumaNode;
			}
		}
		else
		{
			pageSize = (Size)getpagesize();
			procsSizePerNumaNode = TYPEALIGN(pageSize, 
					mul_size(nProcsPerNumaNode, sizeof(PGPROC)));
			for ( i = 0; i < nNumaNode; i++)
			{
				numa_temp_alloced[i] = (PGPROC *)ShmemAllocPageSizeAligned(procsSizePerNumaNode);
				if (sp_numa)
					numa_tonode_memory(numa_temp_alloced[i], procsSizePerNumaNode, numa_fix_offset[i]);
				else
					numa_tonode_memory(numa_temp_alloced[i], procsSizePerNumaNode, i);

				memset(numa_temp_alloced[i], 0, procsSizePerNumaNode);

				if (hugepage_debug)
					elog(WARNING, "(No HugePage)NUMA node:%d --> %p, allocated size(per Numa node) = %lu",
									i, numa_temp_alloced[i], procsSizePerNumaNode);
			}

			for (i = 0; i < TotalProcs; i++)
			{
				pos = i / nNumaNode;
				procs[i] = (PGPROC *)(numa_temp_alloced[i % nNumaNode] + pos);

				MemSet(procs[i], 0, sizeof(PGPROC));
				procs[i]->numaNode = i % nNumaNode;
				procs[i]->cpuNo = i % g_ncpus % nNumaNode * (g_ncpus / nNumaNode)
								+ i % g_ncpus / nNumaNode;
			}
		}
	}
	else
	{
		unsigned int i = 0;
		for (i = 0; i < TotalProcs; i++)
		{
			procs[i] = (PGPROC *) ShmemAlloc(sizeof(PGPROC));
			MemSet(procs[i], 0, sizeof(PGPROC));
		}
	}

	return procs;
}

static void
InitPorcessNuma(PGPROC *MyProc)
{
	if (enable_numa)
	{
		if (enable_bind_numa_core)
			bind_cpu(MyProc->cpuNo);
		else
			numa_run_on_node(MyProc->numaNode);
		numa_set_localalloc();
	}
}

static Size
WalHugepageShmemSize(void)
{
	Size	size = 0;
	Size	locksSizePerNumaNode;
	Size	hugePageSize = 0;
	Size	xlogInsertLocksTotalSize;
	uint32	nNumaNodes;
	uint32	num_locks_in_group;

	nNumaNodes = numa_max_node() + 1;
	nNumaNodesGlobal = nNumaNodes;

	/* 
	 * If the number of numa nodes is greater than the number of locks, 
	 * the number of locks needs to be replenished to the number of nodes 
	 */
	if(nNumaNodes > num_xloginsert_locks)
		num_xloginsert_locks = nNumaNodes;

	if(huge_pages == HUGE_PAGES_OFF)
		return size;

	num_locks_in_group = num_xloginsert_locks / nNumaNodes + 
						(num_xloginsert_locks % nNumaNodes ? 1 : 0);
	num_locks_in_group_global = num_locks_in_group;

	hugePageSize = get_huge_page_size();

	if(!insertLockSize)
		elog(WARNING, "cannot get walinsertlocksize");
	locksSizePerNumaNode = TYPEALIGN(hugePageSize, 
				mul_size(num_locks_in_group, insertLockSize));
	xlogInsertLocksTotalSize = mul_size(locksSizePerNumaNode, (Size)(nNumaNodes + 1));
	if(SysHugePageAvailable())
		size = add_size(size, xlogInsertLocksTotalSize);
	
	return size;
}

static void WalInsertLockSize(Size lockSize)
{
	insertLockSize = lockSize;
}

/*
 * Module initialization function
 */
void
_PG_init(void)
{
    if (!process_shared_preload_libraries_in_progress)
	{
		elog(ERROR, "The module can only be loaded via shared_preload_libraries");
		return;
	}
    
    /* active hooks */
    prev_proc_global_shmem_size_hook = proc_global_shmem_size_hook;
    proc_global_shmem_size_hook = ProcGlobalShmemSizeNuma;

	prev_proc_global_shmem_init_hook = proc_global_shmem_init_hook;
    proc_global_shmem_init_hook = ProcGlobalShmemInitNuma;

	prev_init_process_hook = init_process_hook;
	init_process_hook = InitPorcessNuma;

	prev_wal_hugepage_shmem_size_hook = wal_hugepage_shmem_size_hook;
	wal_hugepage_shmem_size_hook = WalHugepageShmemSize;

	prev_wal_insert_lock_size_hook = wal_insert_lock_size_hook;
	wal_insert_lock_size_hook = WalInsertLockSize;

	prev_xlog_shmem_hook = xlog_shmem_hook;
	xlog_shmem_hook = xlogShmemNuma;

	prev_xlog_shmem_init_hook = xlog_shmem_init_hook;
	xlog_shmem_init_hook = xlogShmemInitNuma;

	prev_xlog_insert_record_hook = xlog_insert_record_hook;
	xlog_insert_record_hook = xlog_insert_record;

	prev_wal_insert_lock_release_hook = wal_insert_lock_release_hook;
	wal_insert_lock_release_hook = wal_insert_lock_release;

	prev_wal_insert_lock_acquire_hook = wal_insert_lock_acquire_hook;
	wal_insert_lock_acquire_hook = wal_insert_lock_acquire;

	prev_wal_insert_lock_acquire_ex_hook = wal_insert_lock_acquire_ex_hook;
	wal_insert_lock_acquire_ex_hook = wal_insert_lock_acquire_ex;

	prev_wal_insert_lock_uia_hook = wal_insert_lock_uia_hook;
	wal_insert_lock_uia_hook = wal_insert_lock_uia;

	prev_wal_insert_lock_tofin_hook = wal_insert_lock_tofin_hook;
	wal_insert_lock_tofin_hook = wal_insert_lock_tofin;

	prev_get_last_important_rec_ptr_hook = get_last_important_rec_ptr_hook;
	get_last_important_rec_ptr_hook = get_last_important_rec_ptr;

	/* GUC parameter. We discuss flag later */
     DefineCustomBoolVariable("hg_perf_numa.enable_numa",
	 						 "Enable the performance of NUMA.",
	 						 NULL,
	 						 &enable_numa,
	 						 false,
	 						 PGC_POSTMASTER,
	 						 0,
	 						 NULL,
	 						 NULL,
	 						 NULL);

     DefineCustomBoolVariable("hg_perf_numa.hugepage_debug",
	 						 "Enable show the debug info of hugepage.",
	 						 NULL,
	 						 &hugepage_debug,
	 						  false,
	 						 PGC_POSTMASTER,
	 						 0,
	 						 NULL,
	 						 NULL,
	 						 NULL);
	DefineCustomBoolVariable("hg_perf_numa.sp_numa",
	 						 "to fit feiteng2000 special numa memory.",
	 						 NULL,
	 						 &sp_numa,
	 						  false,
	 						 PGC_POSTMASTER,
	 						 0,
	 						 NULL,
	 						 NULL,
	 						 NULL);
	DefineCustomBoolVariable("hg_perf_numa.enable_bind_numa_core",
	 						 "enable bind numa core.",
	 						 NULL,
	 						 &enable_bind_numa_core,
	 						  false,
	 						 PGC_POSTMASTER,
	 						 0,
	 						 NULL,
	 						 NULL,
	 						 NULL);
	DefineCustomIntVariable("hg_perf_numa.num_xloginsert_locks",
							"num of xloginsert locks",
							NULL,
							&num_xloginsert_locks,
							16,
							8, 128,
							PGC_POSTMASTER,
							0,
							NULL,
							NULL,
							NULL);
}

/*
 * Module unload function
 */
void
_PG_fini(void)
{
    /* uninstall hooks */
    proc_global_shmem_size_hook = prev_proc_global_shmem_size_hook;
	proc_global_shmem_size_hook = prev_proc_global_shmem_size_hook;
	proc_global_shmem_init_hook = prev_proc_global_shmem_init_hook;
	init_process_hook = prev_init_process_hook;
	wal_hugepage_shmem_size_hook = prev_wal_hugepage_shmem_size_hook;
	wal_insert_lock_size_hook = prev_wal_insert_lock_size_hook;
	xlog_shmem_hook = prev_xlog_shmem_hook;
	xlog_shmem_init_hook = prev_xlog_shmem_init_hook;
	xlog_insert_record_hook = prev_xlog_insert_record_hook;
	wal_insert_lock_release_hook = prev_wal_insert_lock_release_hook;
	wal_insert_lock_acquire_hook = prev_wal_insert_lock_acquire_hook;
	wal_insert_lock_acquire_ex_hook = prev_wal_insert_lock_acquire_ex_hook;
	wal_insert_lock_uia_hook = prev_wal_insert_lock_uia_hook;
	wal_insert_lock_tofin_hook = prev_wal_insert_lock_tofin_hook;
	get_last_important_rec_ptr_hook = prev_get_last_important_rec_ptr_hook;

}

#endif /* _WIN64 */
#endif /* WIN32 */