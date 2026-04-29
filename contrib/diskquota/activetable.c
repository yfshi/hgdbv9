/* -------------------------------------------------------------------------
 *
 * activetable.c
 *
 * This code is responsible for detecting active table for databases
 *
 * Copyright (C) 2013, PostgreSQL Global Development Group
 *
 *
 * -------------------------------------------------------------------------
 */
#include <unistd.h>

#include "postgres.h"

#include "access/htup_details.h"
#include "catalog/indexing.h"
#include "catalog/pg_class.h"
#include "catalog/pg_type.h"
#include "executor/spi.h"
#include "funcapi.h"
#include "miscadmin.h"
#include "storage/shmem.h"
#include "storage/smgr.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/relfilenodemap.h"
#include "utils/syscache.h"

#include "activetable.h"
#include "pg_utils.h"
#include "common/hashfn.h"
#include "access/heapam.h"

#include "utils/lsyscache.h"
#include "commands/extension.h"
#include "storage/proc.h"		/* for MyProc */
#include "utils/wait_event.h"

DiskQuotaActiveOid diskquota_rel;

HTAB *active_tables_map = NULL;
static smgrcreate_hook_type prev_smgrcreate_hook = NULL;
static smgrextend_hook_type prev_smgrextend_hook = NULL;
static smgrtruncate_hook_type prev_smgrtruncate_hook = NULL;
static smgrdounlinkall_hook_type prev_smgrdounlinkall_hook = NULL;
static void active_table_hook_smgrcreate(SMgrRelation reln,
							  ForkNumber forknum,
							  bool isRedo);
static void active_table_hook_smgrextend(SMgrRelation reln,
							  ForkNumber forknum,
							  BlockNumber blocknum,
							  char *buffer,
							  bool skipFsync);
static void active_table_hook_smgrtruncate(SMgrRelation reln,
							  ForkNumber forknum,
							  BlockNumber blocknum);
static void active_table_hook_smgrunlink(SMgrRelation *reln,
                              int nrels,
                              bool isRedo);

static void report_active_table_SmgrStat(SMgrRelation reln, ActiveType at);
static HTAB* get_active_tables_stats(void);
static HTAB* get_all_tables_stats(void);

void init_active_table_hook(void);
void init_shm_worker_active_tables(void);
void init_lock_active_tables(void);
HTAB* pg_fetch_active_tables(bool);
void diskquota_send_sig_to_proc(void);
DiskQuotaSessionPids *save_pid_to_queue(int pid);
static DiskQuotaSessionPids *diskquota_alloc_node(void);
static void init_entry(DiskQuotaActiveTableFileEntry *entry, void *node);
static Oid get_rel_oid(Oid reltablespace, Oid relfilenode);

void save_relation(Relation rel);

/*
 * Register smgr hook to detect active table.
 */
void
init_active_table_hook(void)
{
	prev_smgrcreate_hook = smgrcreate_hook;
	smgrcreate_hook = active_table_hook_smgrcreate;

	prev_smgrextend_hook = smgrextend_hook;
	smgrextend_hook = active_table_hook_smgrextend;

	prev_smgrtruncate_hook = smgrtruncate_hook;
	smgrtruncate_hook = active_table_hook_smgrtruncate;

	prev_smgrdounlinkall_hook = smgrdounlinkall_hook;
	smgrdounlinkall_hook = active_table_hook_smgrunlink;

	save_relation_hook = save_relation;
}

static void
active_table_hook_smgrcreate(SMgrRelation reln,
							  pg_attribute_unused() ForkNumber forknum,
							  pg_attribute_unused() bool isRedo)
{
	report_active_table_SmgrStat(reln, AT_CREATE);
}

static void
active_table_hook_smgrextend(SMgrRelation reln,
							  pg_attribute_unused() ForkNumber forknum,
							  pg_attribute_unused() BlockNumber blocknum,
							  pg_attribute_unused() char *buffer,
							  pg_attribute_unused() bool skipFsync)
{
	report_active_table_SmgrStat(reln, AT_EXTEND);
}

static void
active_table_hook_smgrtruncate(SMgrRelation reln,
							  pg_attribute_unused() ForkNumber forknum,
							  pg_attribute_unused() BlockNumber blocknum)
{
	report_active_table_SmgrStat(reln, AT_TRUNCATE);
}

static void active_table_hook_smgrunlink(SMgrRelation *reln,
                                         pg_attribute_unused() int nrels,
                                         pg_attribute_unused() bool isRedo)
{
	int i;
	for (i = 0; i < nrels; i++)
	{
		report_active_table_SmgrStat(reln[i], AT_UNLINK);
	}
}

/*
 * Init active_tables_map shared memory
 */
void
init_shm_worker_active_tables(void)
{
	HASHCTL ctl;
	memset(&ctl, 0, sizeof(ctl));

	ctl.keysize = sizeof(RelFileNode);
	ctl.entrysize = sizeof(DiskQuotaActiveTableEntry);
	ctl.hash = tag_hash;
	ctl.num_partitions = NUM_BUFFER_PARTITIONS;

	active_tables_map = ShmemInitHash ("active_tables",
										diskquota_max_active_tables,
										diskquota_max_active_tables,
										&ctl,
										HASH_ELEM | HASH_FUNCTION | HASH_PARTITION);
}

/*
 * Fetch active table file size statistics.
 * If force is true, then fetch all the tables.
 */
HTAB* pg_fetch_active_tables(bool force)
{
	if (force)
	{
		return get_all_tables_stats();
	}
	else
	{
		return get_active_tables_stats();
	}
}

/*
 * Get the table size statistics for all the tables
 */
static HTAB* 
get_all_tables_stats()
{
	HTAB *local_table_stats_map = NULL;
	HASHCTL ctl;
	HeapTuple tuple;
	Relation classRel;
	TableScanDesc relScan;

	memset(&ctl, 0, sizeof(ctl));
	ctl.keysize = sizeof(RelFileNode);
	ctl.entrysize = sizeof(DiskQuotaActiveTableEntry);
	ctl.hcxt = CurrentMemoryContext;
	ctl.hash = tag_hash;

	local_table_stats_map = hash_create("local table map with table size info",
								1024,
								&ctl,
								HASH_ELEM | HASH_CONTEXT | HASH_FUNCTION);
	
	classRel = table_open(RelationRelationId, AccessShareLock);
    relScan = table_beginscan_catalog(classRel, 0, NULL);

	while ((tuple = heap_getnext(relScan, ForwardScanDirection)) != NULL)
	{
		Oid relOid;
		DiskQuotaActiveTableEntry *entry;
		RelFileNode node;

		Form_pg_class classForm = (Form_pg_class) GETSTRUCT(tuple);
		if (classForm->relkind != RELKIND_RELATION &&
				classForm->relkind != RELKIND_MATVIEW)
			continue;
		relOid = classForm->oid;

		/* ignore system table*/
		if (relOid < FirstNormalObjectId)
			continue;

		if (classForm->reltablespace == 0)
		{
			/* 
			 * reltablespace = 0 means it uses default table space, so assign it by MyDatabaseTableSpace
			 * as the relfilenode spcNode is not zero when it is in default table space.
			 */
			node.spcNode = MyDatabaseTableSpace;
		}
		else
		{
			node.spcNode = classForm->reltablespace;
		}
		node.dbNode = MyDatabaseId;
		node.relNode = classForm->relfilenode;

		entry = (DiskQuotaActiveTableEntry *) hash_search(local_table_stats_map, &node, HASH_ENTER, NULL);

		entry->node = node;
		entry->type = AT_EXTEND;
		entry->tablesize = diskquota_get_table_size_by_oid(relOid);
	}

	table_endscan(relScan);
	table_close(classRel, AccessShareLock);

    return local_table_stats_map;	
}
/**
 * Get local active table with table oid and table size info.
 * This function first copies active table map from shared memory 
 * to local active table map with refilenode info. Then traverses
 * the local map and find corresponding table oid and table file 
 * size. Finnaly stores them into local active table map and return.
 */
static HTAB* 
get_active_tables_stats()
{
	HASHCTL ctl;
	HTAB *local_active_table_file_map = NULL;
	HTAB *local_active_table_stats_map = NULL;
	HASH_SEQ_STATUS iter;
	DiskQuotaActiveTableFileEntry *active_table_file_entry;
	DiskQuotaActiveTableEntry *active_table_entry;

	Oid relOid;

	memset(&ctl, 0, sizeof(ctl));
	ctl.keysize = sizeof(RelFileNode);
	ctl.entrysize = sizeof(DiskQuotaActiveTableFileEntry);
	ctl.hcxt = CurrentMemoryContext;
	ctl.hash = tag_hash;

	local_active_table_file_map = hash_create("local active table map with relfilenode info",
								1024,
								&ctl,
								HASH_ELEM | HASH_CONTEXT | HASH_FUNCTION);

	/* Move active table from shared memory to local active table map */
	LWLockAcquire(diskquota_locks.active_table_lock, LW_EXCLUSIVE);

	hash_seq_init(&iter, active_tables_map);

	while ((active_table_file_entry = (DiskQuotaActiveTableFileEntry *) hash_seq_search(&iter)) != NULL)
	{
		bool  found;
		DiskQuotaActiveTableFileEntry *entry;

		if (active_table_file_entry->node.dbNode != MyDatabaseId)
		{
			continue;
		}

		/* Add the active table entry into local hash table*/
		entry = hash_search(local_active_table_file_map, &active_table_file_entry->node, HASH_ENTER, &found);
		*entry = *active_table_file_entry;
		hash_search(active_tables_map, &active_table_file_entry->node, HASH_REMOVE, &found);
	}

	LWLockRelease(diskquota_locks.active_table_lock);

	memset(&ctl, 0, sizeof(ctl));
	ctl.keysize = sizeof(RelFileNode);
	ctl.entrysize = sizeof(DiskQuotaActiveTableEntry);
	ctl.hcxt = CurrentMemoryContext;
	ctl.hash = tag_hash;

	local_active_table_stats_map = hash_create("local active table map with relfilenode info",
								1024,
								&ctl,
								HASH_ELEM | HASH_CONTEXT | HASH_FUNCTION);

	/* traverse local active table map and calculate their file size. */
	hash_seq_init(&iter, local_active_table_file_map);
	/* scan whole local map, get the oid of each table and calculate the size of them */
	while ((active_table_file_entry = (DiskQuotaActiveTableFileEntry *) hash_seq_search(&iter)) != NULL)
	{
		bool found;

		switch (active_table_file_entry->tablestatus)
		{
			case TABLE_COMMIT_CREATE:
			case TABLE_COMMIT_CHANGE:
				relOid = RelidByRelfilenode(active_table_file_entry->node.spcNode,
				                            active_table_file_entry->node.relNode);

				if (relOid == InvalidOid)
				{
					relOid = get_rel_oid(active_table_file_entry->node.spcNode, active_table_file_entry->node.relNode);
				}

				if (relOid != InvalidOid) {
					active_table_entry = hash_search(local_active_table_stats_map, &active_table_file_entry->node,
					                                 HASH_ENTER, &found);
					active_table_entry->node = active_table_file_entry->node;
					active_table_entry->tablesize = diskquota_get_table_size_by_oid(relOid);

					active_table_entry->type = AT_EXTEND;
					hash_search(local_active_table_file_map, &active_table_file_entry->node, HASH_REMOVE, NULL);
				} else {
					active_table_file_entry->ispushedback = true;
				}
				break;
			case TABLE_IN_TRANSX_CHANGE:
			case TABLE_IN_TRANSX_CREATE:
				relOid = RelidByRelfilenode(active_table_file_entry->node.spcNode,
											active_table_file_entry->node.relNode);

				if (relOid == InvalidOid)
				{
					relOid = get_rel_oid(active_table_file_entry->node.spcNode, active_table_file_entry->node.relNode);
				}

				active_table_entry = hash_search(local_active_table_stats_map, &active_table_file_entry->node,
				                                 HASH_ENTER, &found);
				active_table_entry->node = active_table_file_entry->node;
				if (relOid != InvalidOid)
					active_table_entry->tablesize = diskquota_get_table_size_by_oid(relOid);
				else
					active_table_entry->tablesize = diskquota_get_table_size_by_relfilenode(&active_table_entry->node);

				active_table_entry->namespace = active_table_file_entry->inXnamespace;
				active_table_entry->owner = active_table_file_entry->inXowner;
				active_table_entry->type = AT_EXTEND;
				hash_search(local_active_table_file_map, &active_table_file_entry->node, HASH_REMOVE, NULL);
				break;
			case TABLE_COMMIT_DELETE:
			case TABLE_IN_TRANSX_DELETE:
				active_table_entry = hash_search(local_active_table_stats_map, &active_table_file_entry->node,
				                                 HASH_ENTER, &found);
				active_table_entry->node = active_table_file_entry->node;
				active_table_entry->type = AT_UNLINK;
				/* no set table size */
				hash_search(local_active_table_file_map, &active_table_file_entry->node, HASH_REMOVE, NULL);
				break;
			default:
				/* Unknown status of entry, ignore it*/
				ereport(LOG, (errmsg("found an entry in unexpected status, table status is %d, relnode is %d", active_table_file_entry->tablestatus,
					 active_table_file_entry->node.relNode)));
				hash_search(local_active_table_file_map, &active_table_file_entry->node, HASH_REMOVE, NULL);
				break;

		}
	}

	/* If table is not found, it could be in transaction, so push back to share memory */

	if (hash_get_num_entries(local_active_table_file_map) > 0)
	{
		bool  found;
		DiskQuotaActiveTableFileEntry *entry;

		/* traverse local active table map and rewrite them into share memory */
		hash_seq_init(&iter, local_active_table_file_map);
		LWLockAcquire(diskquota_locks.active_table_lock, LW_EXCLUSIVE);

		while ((active_table_file_entry = (DiskQuotaActiveTableFileEntry *) hash_seq_search(&iter)) != NULL)
		{
			entry = hash_search(active_tables_map, &active_table_file_entry->node, HASH_ENTER_NULL, &found);
			if (entry)
			{
				HeapTuple tuple;
				Form_pg_class rel;
				Oid relOid;

				relOid = RelidByRelfilenode(entry->node.spcNode, entry->node.relNode);
				tuple = SearchSysCache1(RELOID, ObjectIdGetDatum(relOid));
				if (HeapTupleIsValid(tuple))
				{
					rel = (Form_pg_class) GETSTRUCT(tuple);
					active_table_file_entry->inXnamespace = rel->relnamespace;
					active_table_file_entry->inXowner =  rel->relowner;
					ReleaseSysCache(tuple);
				}
				*entry = *active_table_file_entry;
			}
		}
		LWLockRelease(diskquota_locks.active_table_lock);
	}

	hash_destroy(local_active_table_file_map);
	return local_active_table_stats_map;
}

/**
 *  Hook function in smgr to report the active table
 *  information and stroe them in active table shared memory
 *  diskquota worker will consuming these active tables and
 *  recalculate their file size to update diskquota model.
 */
static void
report_active_table_SmgrStat(SMgrRelation reln, ActiveType at)
{
	DiskQuotaActiveTableFileEntry *entry = NULL;
	bool found = false;
	LWLock	   *partitionLock = NULL;

	if (diskquota_enable != AUOTA_ON)
		return ;

	/* ignore the system table relfilenode */
	if (reln->smgr_rnode.node.relNode < FirstNormalObjectId)
		return;
	{
		entry = hash_search(active_tables_map, &reln->smgr_rnode.node, HASH_ENTER_NULL, &found);

		if (!found)
		{
			init_entry(entry, &reln->smgr_rnode.node);
		}

		partitionLock = QuotaHashPartitionLock(entry->hashcode);

		if (found)
			LWLockAcquire(partitionLock, LW_EXCLUSIVE);

		if (entry && !entry->ispushedback)
		{
			entry->node = reln->smgr_rnode.node;
			switch (at)
			{
				case AT_EXTEND:
				case AT_TRUNCATE:
					entry->tablestatus = TABLE_COMMIT_CHANGE;
					entry->inXnamespace = InvalidOid;
					entry->inXowner = InvalidOid;
					break;

				case AT_CREATE:
					entry->tablestatus = TABLE_COMMIT_CREATE;
					entry->inXnamespace = InvalidOid;
					entry->inXowner = InvalidOid;
					break;

				case AT_UNLINK:
					entry->tablestatus = TABLE_COMMIT_DELETE;
					entry->inXnamespace = InvalidOid;
					entry->inXowner = InvalidOid;
					break;

				default:
					entry->tablestatus = TABLE_UNKNOWN;
					break;
			}
		}
		/*
		 * if the entry is still in active list and marked as un-committed,
		 * this means the entry is only visible in transaction, and need to process here
		 * TODO: this is time consuming, may need to let user to choose enable or disable
		 */
		else if (entry && found && entry->ispushedback)
		{
			/*
			 * If the entry status is TABLE_NOT_FOUND, but its action type is AT_UNLINK. This means
			 * the transaction is under committing or aborting, hence we just need ignore this active
			 * entry and send its unlink status back to worker.
			 */
			if (at == AT_UNLINK)
			{
				entry->tablestatus = TABLE_IN_TRANSX_DELETE;
			}
			else
			{
				entry->inXnamespace = diskquota_rel.inXnamespace;
				entry->inXowner = diskquota_rel.inXowner;
				switch (at)
				{
					case AT_EXTEND:
					case AT_TRUNCATE:
						entry->tablestatus = TABLE_IN_TRANSX_CHANGE;
						break;
					case AT_CREATE:
						entry->tablestatus = TABLE_IN_TRANSX_CREATE;
						break;
					default:
						entry->tablestatus = TABLE_UNKNOWN;
						break;
				}
			}
		}
		else if (entry == NULL)
		{
			/* We may miss the file size change of this relation at current refresh interval.*/
			ereport(WARNING, (errmsg("Share memory is not enough for active tables.")));
		}

		if (found)
			LWLockRelease(partitionLock);
	}

	if ((entry && !entry->ispushedback) ||
		(entry && found && entry->ispushedback))
		/* send sig to launch, statistics of disk usage */
		diskquota_send_sig_to_proc();
}

void diskquota_send_sig_to_proc(void)
{
	int i = 0;
	int pid = 0;

	if (diskquota_strict == AUOTA_OFF)
		return;

	if (MyDatabaseId == InvalidOid)
		return;

	LWLockAcquire(diskquota_locks.message_box_lock, LW_SHARED);

	/* bgwork dboid and pid */
	for (i = 0; i < message_box->db_num; i++)
	{
		if (message_box->dboid_pid_info[i].dboid == MyDatabaseId)
		{
			pid = message_box->dboid_pid_info[i].pid;
			break;
		}
		else if (message_box->dboid_pid_info[i].dboid == InvalidOid)
		{
			break;
		}
	}

	LWLockRelease(diskquota_locks.message_box_lock);

	if (pid != 0 && !diskquotasql)
	{
		int rc = 0;
		DiskQuotaSessionPids *node = NULL;
		node = save_pid_to_queue(getpid());
		if (node != NULL)
		{
			rc = kill(pid, SIGUSR2);
			if (rc == 0)
			{
				int count = 120; /* 1.2 sec */

				while(count-- > 0)
				{
					rc = WaitLatch(&MyProc->procLatch,
								   WL_LATCH_SET | WL_TIMEOUT | WL_POSTMASTER_DEATH,
								   100L, PG_WAIT_EXTENSION);
					if (rc & WL_POSTMASTER_DEATH)
						break;

					ResetLatch(&MyProc->procLatch);

					if (node->deal == true)
					{
						delete_pid_from_queue(node);
						node = NULL;
						break;
					}
					else if (node->pid != getpid())
					{
						break;
					}
				}
			}

			ResetLatch(&MyProc->procLatch);
		}
		else
		{
			kill(pid, SIGUSR2);
			pg_usleep(1000000);
		}
	}
	else if (pid != 0 && diskquotasql)
	{
		kill(pid, SIGUSR2);
		pg_usleep(1000000);
	}

}

static DiskQuotaSessionPids *diskquota_alloc_node(void)
{
	DiskQuotaSessionPids *alloc_node = NULL;

	if (message_box->free_pid_list == NULL)
	{
		return NULL;
	}

	/* when there is only one node left, both prev and next point to itself,
		get secend node*/
	alloc_node = message_box->free_pid_list->free_next;
	alloc_node->free_next->free_prev = alloc_node->free_prev;
	alloc_node->free_prev->free_next = alloc_node->free_next;

	alloc_node->free_next = NULL;
	alloc_node->free_prev = NULL;

	/* last one node */
	if (message_box->free_pid_list->free_next == message_box->free_pid_list->free_prev)
		message_box->free_pid_list = NULL;

	return alloc_node;
}

DiskQuotaSessionPids * 
save_pid_to_queue(int pid)
{
	DiskQuotaSessionPids *alloc_node = NULL;
	DiskQuotaSessionPids *session_head = message_box->session_pid_list;

	if (pid <= 0)
		return NULL;

	LWLockAcquire(diskquota_locks.message_box_lock, LW_EXCLUSIVE);

	alloc_node = diskquota_alloc_node();
	if (alloc_node == NULL)
	{
		LWLockRelease(diskquota_locks.message_box_lock);
		return NULL;
	}

	session_head->use_prev->use_next = alloc_node;
	alloc_node->use_prev = session_head->use_prev;

	session_head->use_prev = alloc_node;
	alloc_node->use_next = session_head;
	alloc_node->pid = pid;
	alloc_node->dbid = MyDatabaseId;
	alloc_node->procLatch = &(MyProc->procLatch);
	alloc_node->deal = false;
	LWLockRelease(diskquota_locks.message_box_lock);

	return alloc_node;
}

static void init_entry(DiskQuotaActiveTableFileEntry *entry, void *node)
{
	if (entry == NULL)
		return;

	entry->inXnamespace = InvalidOid;
	entry->tablestatus = TABLE_UNKNOWN;
	entry->ispushedback = false;
	entry->inXowner = InvalidOid;
	entry->node.dbNode = InvalidOid;
	entry->node.relNode = InvalidOid;
	entry->node.spcNode = InvalidOid;
	entry->hashcode = get_hash_value(active_tables_map, node);

	return;
}

static Oid get_rel_oid(Oid reltablespace, Oid relfilenode)
{
	StringInfoData buf;
	int			ret;
	uint64		proc;
	Oid reloid = InvalidOid;

	initStringInfo(&buf);

	if (SPI_connect() != SPI_OK_CONNECT)
	{
		elog(WARNING, "SPI_connect failed");
	}

	if (reltablespace == 1663 || reltablespace == InvalidOid)
	{
		appendStringInfo(&buf, "select oid from pg_class where relfilenode=%d and (reltablespace=%d or reltablespace = 0);",
						relfilenode, reltablespace);
	}
	else
	{
		appendStringInfo(&buf, "select oid from pg_class where relfilenode=%d and reltablespace=%d;",
						relfilenode, reltablespace);
	}

	ret = SPI_execute(buf.data, false, 0);
	if (ret != SPI_OK_SELECT)
	{
		SPI_finish();
		ereport(WARNING, errmsg("cannot select pg_class: error code %d", ret));
	}

	proc = SPI_processed;

	if (proc == 1)
	{
		SPITupleTable *spi_tuptable = SPI_tuptable;
		TupleDesc	spi_tupdesc = spi_tuptable->tupdesc;
		reloid = atooid(SPI_getvalue(spi_tuptable->vals[0], spi_tupdesc, 1));
	}

	if (SPI_finish() != SPI_OK_FINISH)
	{
		elog(WARNING, "SPI_finish failed");
	}

	return reloid;
}

void save_relation(Relation rel)
{
	if (diskquota_enable != AUOTA_ON)
		return ;

	if (rel != NULL && rel->rd_rel != NULL)
	{
		diskquota_rel.inXnamespace = rel->rd_rel->relnamespace;
		diskquota_rel.inXowner = rel->rd_rel->relowner;
	}
	else
	{
		diskquota_rel.inXnamespace = InvalidOid;
		diskquota_rel.inXowner = InvalidOid;
	}

	return;
}
