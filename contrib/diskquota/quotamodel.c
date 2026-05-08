/* -------------------------------------------------------------------------
 *
 * quotamodel.c
 *
 * This code is responsible for init disk quota model and refresh disk quota 
 * model.
 *
 * Copyright (C) 2013, PostgreSQL Global Development Group
 *
 *
 * -------------------------------------------------------------------------
 */

#include "postgres.h"

#include <unistd.h>
#include <fcntl.h>

#include "access/heapam.h"
#include "access/htup_details.h"
#include "access/reloptions.h"
#include "access/transam.h"
#include "access/xact.h"
#include "catalog/namespace.h"
#include "catalog/pg_class.h"
#include "catalog/pg_database.h"
#include "catalog/pg_type.h"
#include "commands/dbcommands.h"
#include "executor/spi.h"
#include "funcapi.h"
#include "lib/stringinfo.h"
#include "miscadmin.h"
#include "nodes/makefuncs.h"
#include "storage/ipc.h"
#include "storage/latch.h"
#include "storage/lwlock.h"
#include "storage/shmem.h"
#include "storage/fd.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "utils/snapmgr.h"
#include "utils/syscache.h"
#include "utils/wait_event.h"

#include "activetable.h"
#include "diskquota.h"
#include "common/hashfn.h"
#include "catalog/heap.h"
#include "access/tableam.h"
#include "commands/extension.h"
#include "pg_contrib_audit.h"  // ReqID:SRS-SECURITY-AUDIT

/* cluster level max size of black list */
#define MAX_DISK_QUOTA_BLACK_ENTRIES (1024 * 1024)
/* cluster level init size of black list */
#define INIT_DISK_QUOTA_BLACK_ENTRIES 8192
/* per database level max size of black list */
#define MAX_LOCAL_DISK_QUOTA_BLACK_ENTRIES 8192

#define MIN_QUOTA_RESERVE_DIR "diskquota_min_reserve"

typedef struct TableSizeEntry TableSizeEntry;
typedef struct NamespaceSizeEntry NamespaceSizeEntry;
typedef struct RoleSizeEntry RoleSizeEntry;
typedef struct QuotaLimitEntry QuotaLimitEntry;
typedef struct BlackMapEntry BlackMapEntry;
typedef struct LocalBlackMapEntry LocalBlackMapEntry;

/* local cache of table disk size and corresponding schema and owner */
struct TableSizeEntry
{
	RelFileNode node;       /* hash table key */
	Oid			reloid;
	Oid			namespaceoid;
	Oid			owneroid;
	int64		totalsize;

};

/* local cache of namespace disk size */
struct NamespaceSizeEntry
{
	Oid			namespaceoid;
	int64		totalsize;
};

/* local cache of role disk size */
struct RoleSizeEntry
{
	Oid			owneroid;
	int64		totalsize;
};

/* local cache of disk quota limit */
struct QuotaLimitEntry
{
	Oid			targetoid;
	int64		limitsize;
	int64		minlimitsize;
};

/* global blacklist for which exceed their quota limit */
struct BlackMapEntry
{
	Oid			targetoid;
	Oid			databaseoid;
	uint32		targettype;
};

/* local blacklist for which exceed their quota limit */
struct LocalBlackMapEntry
{
	BlackMapEntry	keyitem;
	bool			isexceeded;
};

/* using hash table to support incremental update the table size entry.*/
static HTAB *table_size_map = NULL;
static HTAB *namespace_size_map = NULL;
static HTAB *role_size_map = NULL;
static HTAB *namespace_quota_limit_map = NULL;
static HTAB *role_quota_limit_map = NULL;

/* black list for database objects which exceed their quota limit */
static HTAB *disk_quota_black_map = NULL;
static HTAB *local_disk_quota_black_map = NULL;

static shmem_startup_hook_type prev_shmem_startup_hook = NULL;

/* functions to refresh disk quota model*/
static void calculate_table_disk_usage(bool force);
static void calculate_schema_disk_usage(void);
static void calculate_role_disk_usage(void);
static void flush_local_black_map(void);
static void check_disk_quota_by_oid(Oid targetOid, int64 current_usage, QuotaType type);
static void update_namespace_map(Oid namespaceoid, int64 updatesize);
static void update_role_map(Oid owneroid, int64 updatesize);
static void remove_namespace_map(Oid namespaceoid);
static void remove_role_map(Oid owneroid);
static bool load_quotas(void);

static Size DiskQuotaShmemSize(void);
static void disk_quota_shmem_startup(void);
static void init_sessionlist_pid(void);
static bool quota_check_common(Oid ownerOid, Oid nsOid);

static char *get_min_quota_reserve_dir(void);

/*
 * DiskQuotaShmemSize
 * Compute space needed for diskquota-related shared memory
 */
Size
DiskQuotaShmemSize(void)
{
	Size		size;

	size = sizeof(DiskquotaMessageBox);
	size = add_size(size, hash_estimate_size(MAX_DISK_QUOTA_BLACK_ENTRIES, sizeof(BlackMapEntry)));
	size = add_size(size, hash_estimate_size(diskquota_max_active_tables, sizeof(DiskQuotaActiveTableEntry)));
	return size;
}

static void
init_lwlocks(void)
{
	LWLockPadded *base;
	base = GetNamedLWLockTranche("diskquota_locks");
	diskquota_locks.active_table_lock = &base[0].lock;
	diskquota_locks.black_map_lock = &base[1].lock;
	diskquota_locks.message_box_lock = &base[2].lock;
}
/*
 * DiskQuotaShmemInit
 *		Allocate and initialize diskquota-related shared memory
 */
void
disk_quota_shmem_startup(void)
{
	bool		found;
	HASHCTL		hash_ctl;

	if (prev_shmem_startup_hook)
		(*prev_shmem_startup_hook)();

	disk_quota_black_map = NULL;

	LWLockAcquire(AddinShmemInitLock, LW_EXCLUSIVE);

	init_lwlocks();
	message_box = ShmemInitStruct("disk_quota_message_box",
								sizeof(DiskquotaMessageBox) +
								sizeof(DiskQuotaSessionPids) * MaxConnections,
								&found);
	if (!found)
	{
		memset((void*)message_box, 0, sizeof(DiskquotaMessageBox) + sizeof(DiskQuotaSessionPids) * MaxConnections);
		init_sessionlist_pid();
	}

	memset(&hash_ctl, 0, sizeof(hash_ctl));
	hash_ctl.keysize = sizeof(BlackMapEntry);
	hash_ctl.entrysize = sizeof(BlackMapEntry);
	hash_ctl.hash = tag_hash;

	disk_quota_black_map = ShmemInitHash("blackmap whose quota limitation is reached",
									INIT_DISK_QUOTA_BLACK_ENTRIES,
									MAX_DISK_QUOTA_BLACK_ENTRIES,
									&hash_ctl,
									HASH_ELEM | HASH_FUNCTION);

	init_shm_worker_active_tables();
	disk_quota_lock_array = ShmemInitStruct("disk_quota_partionlock",
								diskquota_max_active_tables ,
								&found);

	LWLockRelease(AddinShmemInitLock);
}

void
init_disk_quota_shmem(void)
{
	/*
	 * Request additional shared resources.  (These are no-ops if we're not in
	 * the postmaster process.)  We'll allocate or attach to the shared
	 * resources in pgss_shmem_startup().
	 */
	RequestAddinShmemSpace(DiskQuotaShmemSize());
	RequestNamedLWLockTranche("diskquota_locks", 3);

	/*
	 * Install startup hook to initialize our shared memory.
	 */
	prev_shmem_startup_hook = shmem_startup_hook;
	shmem_startup_hook = disk_quota_shmem_startup;
}

/*
 * Init disk quota model when the worker process firstly started.
 */
void
init_disk_quota_model(void)
{
	HASHCTL		hash_ctl;

	/* init hash table for table/schema/role etc.*/
	memset(&hash_ctl, 0, sizeof(hash_ctl));
	hash_ctl.keysize = sizeof(RelFileNode);
	hash_ctl.entrysize = sizeof(TableSizeEntry);
	hash_ctl.hcxt = CurrentMemoryContext;
	hash_ctl.hash = tag_hash;

	table_size_map = hash_create("TableSizeEntry map",
								1024 * 8,
								&hash_ctl,
								HASH_ELEM | HASH_CONTEXT | HASH_FUNCTION);

	memset(&hash_ctl, 0, sizeof(hash_ctl));
	hash_ctl.keysize = sizeof(Oid);
	hash_ctl.entrysize = sizeof(NamespaceSizeEntry);
	hash_ctl.hcxt = CurrentMemoryContext;
	hash_ctl.hash = oid_hash;

	namespace_size_map = hash_create("NamespaceSizeEntry map",
								1024,
								&hash_ctl,
								HASH_ELEM | HASH_CONTEXT | HASH_FUNCTION);

	memset(&hash_ctl, 0, sizeof(hash_ctl));
	hash_ctl.keysize = sizeof(Oid);
	hash_ctl.entrysize = sizeof(RoleSizeEntry);
	hash_ctl.hcxt = CurrentMemoryContext;
	hash_ctl.hash = oid_hash;

	role_size_map = hash_create("RoleSizeEntry map",
								1024,
								&hash_ctl,
								HASH_ELEM | HASH_CONTEXT | HASH_FUNCTION);

	memset(&hash_ctl, 0, sizeof(hash_ctl));
	hash_ctl.keysize = sizeof(Oid);
	hash_ctl.entrysize = sizeof(QuotaLimitEntry);
	hash_ctl.hcxt = CurrentMemoryContext;
	hash_ctl.hash = oid_hash;

	namespace_quota_limit_map = hash_create("Namespace QuotaLimitEntry map",
								1024,
								&hash_ctl,
								HASH_ELEM | HASH_CONTEXT | HASH_FUNCTION);

	role_quota_limit_map = hash_create("Role QuotaLimitEntry map",
								1024,
								&hash_ctl,
								HASH_ELEM | HASH_CONTEXT | HASH_FUNCTION);

	memset(&hash_ctl, 0, sizeof(hash_ctl));
	hash_ctl.keysize = sizeof(BlackMapEntry);
	hash_ctl.entrysize = sizeof(LocalBlackMapEntry);
	hash_ctl.hcxt = CurrentMemoryContext;
	hash_ctl.hash = tag_hash;

	local_disk_quota_black_map = hash_create("local blackmap whose quota limitation is reached",
									MAX_LOCAL_DISK_QUOTA_BLACK_ENTRIES,
									&hash_ctl,
									HASH_ELEM | HASH_CONTEXT | HASH_FUNCTION);
}

/*
 * diskquota worker will refresh disk quota model
 * periodically. It will reload quota setting and 
 * recalculate the changed disk usage.
 */
void
refresh_disk_quota_model(bool force)
{
	elog(DEBUG1,"check disk quota begin");
	StartTransactionCommand();
	SPI_connect();
	PushActiveSnapshot(GetTransactionSnapshot());
	/* skip refresh model when load_quotas failed */
	if (load_quotas())
	{
		refresh_disk_quota_usage(force);
	}
	SPI_finish();
	PopActiveSnapshot();
	CommitTransactionCommand();
	elog(DEBUG1,"check disk quota end");
}

/*
 * Update the disk usage of nameapsce and role.
 * Put the exceeded namespace and role into shared black map.
 */
void
refresh_disk_quota_usage(bool force)
{
	/* get tail node */
	if (diskquota_strict == AUOTA_ON && !force && message_box->session_pid_list != message_box->session_pid_list->use_prev)
		tail = message_box->session_pid_list->use_prev;

	/* recalculate the disk usage of table, schema and role */
	calculate_table_disk_usage(force);
	calculate_schema_disk_usage();
	calculate_role_disk_usage();
	reconcile_min_quota_reserve_files();
	/* copy local black map back to shared black map */
	flush_local_black_map();
}

/*
 * Generate the new shared blacklist from the local_black_list which
 * exceed the quota limit.
 * local_balck_list is used to reduce the lock race.
 */
static void
flush_local_black_map(void)
{
	HASH_SEQ_STATUS iter;
	LocalBlackMapEntry* localblackentry;
	BlackMapEntry* blackentry;
	bool found;

	LWLockAcquire(diskquota_locks.black_map_lock, LW_EXCLUSIVE);

	hash_seq_init(&iter, local_disk_quota_black_map);
	while ((localblackentry = hash_seq_search(&iter)) != NULL)
	{
		if (localblackentry->isexceeded)
		{
			blackentry = (BlackMapEntry*) hash_search(disk_quota_black_map,
							   (void *) &localblackentry->keyitem,
							   HASH_ENTER_NULL, &found);
			if (blackentry == NULL)
			{
				elog(WARNING, "shared disk quota black map size limit reached.");
			}
			else
			{
				/* new db objects which exceed quota limit */
				if (!found)
				{
					blackentry->targetoid = localblackentry->keyitem.targetoid;
					blackentry->databaseoid = MyDatabaseId;
					blackentry->targettype = localblackentry->keyitem.targettype;
				}
			}
			localblackentry->isexceeded = false;
		}
		else
		{
			/* db objects are removed or under quota limit in the new loop */
			(void) hash_search(disk_quota_black_map,
							   (void *) &localblackentry->keyitem,
							   HASH_REMOVE, NULL);
			(void) hash_search(local_disk_quota_black_map,
							   (void *) &localblackentry->keyitem,
							   HASH_REMOVE, NULL);

			if ((QuotaType)localblackentry->keyitem.targettype == NAMESPACE_QUOTA)
			{
				elog(WARNING, "namespace:remove object %u from blacklist.",
							localblackentry->keyitem.targetoid);
			}
			else
			{
				elog(WARNING, "role:remove object %u from blacklist.",
							localblackentry->keyitem.targetoid);
			}
		}
	}
	LWLockRelease(diskquota_locks.black_map_lock);
}

/*
 * Compare the disk quota limit and current usage of a database object.
 * Put them into local blacklist if quota limit is exceeded.
 */
static void check_disk_quota_by_oid(Oid targetOid, int64 current_usage, QuotaType type)
{
	bool					found;
	int32 					quota_limit_mb;
	int32 					current_usage_mb;
	LocalBlackMapEntry*		localblackentry;
	BlackMapEntry 			keyitem;

	QuotaLimitEntry* quota_entry;
	if (type == NAMESPACE_QUOTA)
	{
		quota_entry = (QuotaLimitEntry *)hash_search(namespace_quota_limit_map,
											&targetOid,
											HASH_FIND, &found);
	}
	else if (type == ROLE_QUOTA)
	{
		quota_entry = (QuotaLimitEntry *)hash_search(role_quota_limit_map,
											&targetOid,
											HASH_FIND, &found);
	}
	else
	{
		/* skip check if not namespace or role quota*/
		return;
	}

	if (!found)
	{
		/* default no limit */
		return;
	}

	quota_limit_mb = quota_entry->limitsize * (1024 *1024) - BLCKSZ;
	current_usage_mb = current_usage;
	if (quota_limit_mb > 0 && current_usage_mb >= quota_limit_mb)
	{
		memset(&keyitem, 0, sizeof(BlackMapEntry));
		keyitem.targetoid = targetOid;
		keyitem.databaseoid = MyDatabaseId;
		keyitem.targettype = (uint32)type;
		if (type == NAMESPACE_QUOTA)
		{
			elog(WARNING,"namespace:Put object %u to blacklist with quota limit:%d, current usage:%d",
					targetOid, quota_limit_mb, current_usage_mb);
		}
		else
		{
			elog(WARNING,"role:Put object %u to blacklist with quota limit:%d, current usage:%d",
					targetOid, quota_limit_mb, current_usage_mb);
		}

		localblackentry = (LocalBlackMapEntry*) hash_search(local_disk_quota_black_map,
					&keyitem,
					HASH_ENTER, &found);
		localblackentry->isexceeded = true;
	}
	else
	{
		memset(&keyitem, 0, sizeof(BlackMapEntry));
		keyitem.targetoid = targetOid;
		keyitem.databaseoid = MyDatabaseId;
		keyitem.targettype = (uint32)type;
		localblackentry = (LocalBlackMapEntry*) hash_search(local_disk_quota_black_map,
					&keyitem,
					HASH_ENTER, &found);
		localblackentry->isexceeded = false;
	}

}

/*
 *  Remove a namespace from local namespace_size_map
 */
static void
remove_namespace_map(Oid namespaceoid)
{
	hash_search(namespace_size_map,
			&namespaceoid,
			HASH_REMOVE, NULL);
}

/*
 * Update the current disk usage of a namespace in namespace_size_map.
 */
static void
update_namespace_map(Oid namespaceoid, int64 updatesize)
{
	bool found;
	NamespaceSizeEntry* nsentry;
	nsentry = (NamespaceSizeEntry *)hash_search(namespace_size_map,
			&namespaceoid,
			HASH_ENTER, &found);
	if (!found)
	{
		nsentry->namespaceoid = namespaceoid;
		nsentry->totalsize = updatesize;
	}
	else {
		nsentry->totalsize += updatesize;
	}
}

/*
 *  Remove a namespace from local role_size_map
 */
static void
remove_role_map(Oid owneroid)
{
	hash_search(role_size_map,
			&owneroid,
			HASH_REMOVE, NULL);
}

/*
 * Update the current disk usage of a namespace in role_size_map.
 */
static void
update_role_map(Oid owneroid, int64 updatesize)
{
	bool found;
	RoleSizeEntry* rolentry;
	rolentry = (RoleSizeEntry *)hash_search(role_size_map,
			&owneroid,
			HASH_ENTER, &found);
	if (!found)
	{
		rolentry->owneroid = owneroid;
		rolentry->totalsize = updatesize;
	}
	else
	{
		rolentry->totalsize += updatesize;
	}
}

/*
 *  Incremental way to update the disk quota of every database objects
 *  Recalculate the table's disk usage when it's a new table or active table.
 *  Detect the removed table if it's nolonger in pg_class.
 *  If change happens, no matter size change or owner change,
 *  update namespace_size_map and role_size_map correspondingly.
 *  Parameter 'force' set to true at initialization stage to recalculate 
 *  the file size of all the tables.
 *
 */
static void
calculate_table_disk_usage(bool force)
{
	bool found;
	bool active_tbl_found = false;
	Relation	classRel;
	HeapTuple	tuple;
	TableScanDesc relScan;
	RelFileNode node;
	TableSizeEntry *tsentry = NULL;
	Oid			relOid;
	HASH_SEQ_STATUS iter;
	HTAB *local_active_table_stat_map;
	DiskQuotaActiveTableEntry *active_table_entry;

	classRel = table_open(RelationRelationId, AccessShareLock);
	relScan = table_beginscan_catalog(classRel, 0, NULL);

	/* TODO: init process could be separated to speed up processing */
	local_active_table_stat_map = pg_fetch_active_tables(force);

	/*
	 * scan pg_class to detect table event: drop, reset schema, reset owenr.
	 * calculate the file size for active table and update namespace_size_map
	 * and role_size_map
	 */
	while ((tuple = heap_getnext(relScan, ForwardScanDirection)) != NULL)
	{
		Form_pg_class classForm = (Form_pg_class) GETSTRUCT(tuple);
		found = false;

		if (classForm->relkind != RELKIND_RELATION &&
			classForm->relkind != RELKIND_MATVIEW)
			continue;
		relOid = classForm->oid;

		/* ignore system table*/
		if(relOid < FirstNormalObjectId)
			continue;

		node.dbNode = MyDatabaseId;
		if (classForm->reltablespace == 0)
		{
			/* 
			 * relfilenode table space could not be zero, so using MyDatabaseTableSpace to record the correct
			 * default table space oid
			 */
			node.spcNode = MyDatabaseTableSpace;
		}
		else
		{
			node.spcNode = classForm->reltablespace;
		}
		node.relNode = classForm->relfilenode;

		tsentry = (TableSizeEntry *)hash_search(table_size_map,
											&node,
											HASH_ENTER, &found);

		/* We need to check whether such table is in local cache yet. If not, we init the entry firstly. */
		if(!found)
		{
			tsentry->node = node;
			tsentry->reloid = relOid;
			tsentry->namespaceoid = classForm->relnamespace;
			tsentry->owneroid = classForm->relowner;
			tsentry->totalsize = 0;
		}

		if (tsentry->reloid == InvalidOid)
			tsentry->reloid = relOid;

		if (tsentry->namespaceoid == InvalidOid)
			tsentry->namespaceoid = classForm->relnamespace;

		if (tsentry->owneroid == InvalidOid)
		{
			tsentry->owneroid = classForm->relowner;
		}

		/* The worker is single thread, so it should be safe when using HASH_REMOVE as there is no other thread
		 * on this hash table
		 * */
		active_table_entry = (DiskQuotaActiveTableEntry *) hash_search(local_active_table_stat_map, &node, HASH_REMOVE, &active_tbl_found);

		/* skip to recalculate the tables which are not in active list and not at initializatio stage*/
		if(active_tbl_found)
		{
			int64 oldtotalsize;

			if (active_table_entry->type == AT_UNLINK)
			{
				/* in case of the relate file has been removed, but the relfilenode is still existing in catalog */
				update_namespace_map(tsentry->namespaceoid, -1 * tsentry->totalsize);
				update_role_map(tsentry->owneroid, -1 * tsentry->totalsize);
				hash_search(table_size_map, &node, HASH_REMOVE, NULL);
			}
			else
			{
				oldtotalsize = tsentry->totalsize;
				tsentry->totalsize = active_table_entry->tablesize;
				update_namespace_map(tsentry->namespaceoid, tsentry->totalsize - oldtotalsize);
				update_role_map(tsentry->owneroid, tsentry->totalsize - oldtotalsize);
			}
		}

		/* if schema change, transfer the file size */
		if (tsentry->namespaceoid != classForm->relnamespace)
		{
			update_namespace_map(tsentry->namespaceoid, -1 * tsentry->totalsize);
			tsentry->namespaceoid = classForm->relnamespace;
			update_namespace_map(tsentry->namespaceoid, tsentry->totalsize);
		}
		/* if owner change, transfer the file size */
		if(tsentry->owneroid != classForm->relowner)
		{
			update_role_map(tsentry->owneroid, -1 * tsentry->totalsize);
			tsentry->owneroid = classForm->relowner;
			update_role_map(tsentry->owneroid, tsentry->totalsize);
		}
	}

	table_endscan(relScan);
	table_close(classRel, AccessShareLock);

	/* process table objects that are not (visible) in catalog yet */
	hash_seq_init(&iter, local_active_table_stat_map);
	while ((active_table_entry = (DiskQuotaActiveTableEntry *) hash_seq_search(&iter)) != NULL)
	{
		tsentry = (TableSizeEntry *)hash_search(table_size_map,
												&active_table_entry->node,
												HASH_ENTER, &found);

		/* A new invisible table object found, we need to init it firstly and then do update */
		if(!found && active_table_entry->type != AT_UNLINK)
		{
			tsentry->node = active_table_entry->node;
			tsentry->reloid = InvalidOid;
			tsentry->namespaceoid = active_table_entry->namespace;
			tsentry->owneroid = active_table_entry->owner;
			tsentry->totalsize = active_table_entry->tablesize;

			update_namespace_map(tsentry->namespaceoid, tsentry->totalsize);
			update_role_map(tsentry->owneroid, tsentry->totalsize);

			ereport(LOG, (errmsg("An active relfilenode %d:%d is added into local cache",
								active_table_entry->node.relNode, active_table_entry->node.spcNode)));
		}
		else if (found && active_table_entry->type != AT_UNLINK)
		{
			int64 oldtotalsize = tsentry->totalsize;
			tsentry->totalsize = active_table_entry->tablesize;
			update_namespace_map(tsentry->namespaceoid, tsentry->totalsize - oldtotalsize);
			update_role_map(tsentry->owneroid, tsentry->totalsize - oldtotalsize);
			ereport(LOG, (errmsg("An active relfilenode %d:%d is updated into local cache",
								active_table_entry->node.relNode, active_table_entry->node.spcNode)));
		}
		else if (found && active_table_entry->type == AT_UNLINK)
		{
			update_role_map(tsentry->owneroid, -1 * tsentry->totalsize);
			update_namespace_map(tsentry->namespaceoid, -1 * tsentry->totalsize);
			hash_search(table_size_map, &active_table_entry->node, HASH_REMOVE, NULL);

			ereport(LOG, (errmsg("An active relfilenode %d:%d is deleted into local cache",
								active_table_entry->node.relNode, active_table_entry->node.spcNode)));
		}
		else if (!found && active_table_entry->type == AT_UNLINK)
		{
			/* This case means the table has been created, the dropped out in one cycle of diskquota worker process */
			continue;
		}
		else
		{
			ereport(ERROR, (errmsg("An active relfilenode %d:%d is under incorrect status",
			                     active_table_entry->node.relNode, active_table_entry->node.spcNode)));
		}

	}
	hash_destroy(local_active_table_stat_map);
}

/*
 * Check the namespace quota limit and current usage
 * Remove dropped namespace from namespace_size_map
 */
static void calculate_schema_disk_usage(void)
{
	HeapTuple	tuple;
	HASH_SEQ_STATUS iter;
	NamespaceSizeEntry* nsentry;
	hash_seq_init(&iter, namespace_size_map);

	while ((nsentry = hash_seq_search(&iter)) != NULL)
	{
		/* check if namespace is already be deleted */
		tuple = SearchSysCache1(NAMESPACEOID, ObjectIdGetDatum(nsentry->namespaceoid));
		if (!HeapTupleIsValid(tuple))
		{
			remove_namespace_map(nsentry->namespaceoid);
			continue;
		}
		ReleaseSysCache(tuple);

		check_disk_quota_by_oid(nsentry->namespaceoid, nsentry->totalsize, NAMESPACE_QUOTA);
	}
}

/*
 * Check the role quota limit and current usage
 * Remove dropped role from roel_size_map
 */
static void calculate_role_disk_usage(void)
{
	HeapTuple	tuple;
	HASH_SEQ_STATUS iter;
	RoleSizeEntry* rolentry;
	hash_seq_init(&iter, role_size_map);

	while ((rolentry = hash_seq_search(&iter)) != NULL)
	{
		/* check if role is already be deleted */
		tuple = SearchSysCache1(AUTHOID, ObjectIdGetDatum(rolentry->owneroid));
		if (!HeapTupleIsValid(tuple))
		{
			remove_role_map(rolentry->owneroid);
			continue;
		}
		ReleaseSysCache(tuple);

		check_disk_quota_by_oid(rolentry->owneroid, rolentry->totalsize, ROLE_QUOTA);
	}
}

/*
 * Load quotas from diskquota configuration table(quota_config).
*/
static bool
load_quotas(void)
{
	int			ret;
	TupleDesc	tupdesc;
	int			i;
	bool		found;
	QuotaLimitEntry* quota_entry;
	HASH_SEQ_STATUS iter;

	RangeVar   *rv;
	Relation	rel;

	rv = makeRangeVar("hg_security", "quota_config", -1);
	rel = table_openrv_extended(rv, AccessShareLock, true);
	if (!rel)
	{
		/* configuration table is missing. */
		elog(LOG, "configuration table \"quota_config\" is missing in database \"%s\"," 
				" please recreate diskquota extension",
			 get_database_name(MyDatabaseId));
		return false;
	}
	table_close(rel, NoLock);

	/* clear entries in quota limit map*/
	hash_seq_init(&iter, namespace_quota_limit_map);
	while ((quota_entry = hash_seq_search(&iter)) != NULL)
	{
		(void) hash_search(namespace_quota_limit_map,
				(void *) &quota_entry->targetoid,
				HASH_REMOVE, NULL);
	}

	hash_seq_init(&iter, role_quota_limit_map);
	while ((quota_entry = hash_seq_search(&iter)) != NULL)
	{
		(void) hash_search(role_quota_limit_map,
				(void *) &quota_entry->targetoid,
				HASH_REMOVE, NULL);
	}

	ret = SPI_execute("select targetoid, quotatype, quotalimitMB, minlimitMB from hg_security.quota_config", true, 0);
	if (ret != SPI_OK_SELECT)
		elog(FATAL, "SPI_execute failed: error code %d", ret);

	tupdesc = SPI_tuptable->tupdesc;
	if (tupdesc->natts != 4 ||
		TupleDescAttr(tupdesc, 0)->atttypid != OIDOID ||
		TupleDescAttr(tupdesc, 1)->atttypid != INT4OID ||
		TupleDescAttr(tupdesc, 2)->atttypid != INT8OID ||
		TupleDescAttr(tupdesc, 3)->atttypid != INT8OID)
	{
		elog(LOG, "configuration table \"quota_config\" is corruptted in database \"%s\"," 
				" please recreate diskquota extension",
			 get_database_name(MyDatabaseId));
		return false;
	}

	for (i = 0; i < SPI_processed; i++)
	{
		HeapTuple	tup = SPI_tuptable->vals[i];
		Datum		dat;
		Oid			targetOid;
		int64		quota_limit_mb = 0;
		int64		min_limit_mb = 0;
		QuotaType	quotatype;
		bool		isnull;

		dat = SPI_getbinval(tup, tupdesc, 1, &isnull);
		if (isnull)
			continue;
		targetOid = DatumGetObjectId(dat);
		
		dat = SPI_getbinval(tup, tupdesc, 2, &isnull);
		if (isnull)
			continue;
		quotatype = (QuotaType)DatumGetInt32(dat);

		dat = SPI_getbinval(tup, tupdesc, 3, &isnull);
		if (!isnull)
			quota_limit_mb = DatumGetInt64(dat);

		dat = SPI_getbinval(tup, tupdesc, 4, &isnull);
		if (!isnull)
			min_limit_mb = DatumGetInt64(dat);

		if (quotatype == NAMESPACE_QUOTA)
		{
			quota_entry = (QuotaLimitEntry *)hash_search(namespace_quota_limit_map,
												&targetOid,
												HASH_ENTER, &found);
			quota_entry->limitsize = quota_limit_mb;
			quota_entry->minlimitsize = min_limit_mb;
		}
		else if (quotatype == ROLE_QUOTA)
		{
			quota_entry = (QuotaLimitEntry *)hash_search(role_quota_limit_map,
												&targetOid,
												HASH_ENTER, &found);
			quota_entry->limitsize = quota_limit_mb;
			quota_entry->minlimitsize = min_limit_mb;
		}
	}
	return true;
}

/*
 * Return the per-database directory that stores min quota reserve files.
 */
static char *
get_min_quota_reserve_dir(void)
{
	return psprintf("%s/%s", DataDir, MIN_QUOTA_RESERVE_DIR);
}

#if !(defined(HAVE_POSIX_FALLOCATE) && defined(__linux__))
/*
 * Fill a reserve file with zeroes up to target_bytes.
 *
 * This is the portable fallback for platforms without posix_fallocate(), such
 * as Windows.  FileTruncate() alone can create a sparse file, which would not
 * actually reserve filesystem space.
 */
static int
write_zeroes_to_reserve_file(File file, int64 target_bytes)
{
	#define		RESERVE_WRITE_CHUNK_SIZE (1024 * 1024)

	char	   *zerobuf;
	int64		offset = 0;

	if (target_bytes <= 0)
		return 0;

	zerobuf = (char *) palloc0(RESERVE_WRITE_CHUNK_SIZE);

	while (offset < target_bytes)
	{
		int			amount;
		int			written;

		if (target_bytes - offset > RESERVE_WRITE_CHUNK_SIZE)
			amount = RESERVE_WRITE_CHUNK_SIZE;
		else
			amount = (int) (target_bytes - offset);

		written = FileWrite(file,
							zerobuf,
							amount,
							(off_t) offset,
							WAIT_EVENT_DATA_FILE_WRITE);
		if (written != amount)
		{
			if (written >= 0)
				errno = ENOSPC;
			pfree(zerobuf);
			return -1;
		}

		offset += amount;
	}

	pfree(zerobuf);
	return 0;
}
#endif

/*
 * Resize the reserve file so it matches the remaining protected space.
 */
void
resize_min_quota_reserve_file(QuotaType type, Oid targetoid, int64 target_bytes)
{
	char	   *dir = get_min_quota_reserve_dir();
	char		path[MAXPGPATH];
	File		file;

	if (MakePGDirectory(dir) < 0 && errno != EEXIST)
	{
		ereport(WARNING,
				(errcode_for_file_access(),
				 errmsg("could not create reserve directory \"%s\": %m", dir)));
		pfree(dir);
		return;
	}

	snprintf(path, sizeof(path), "%s/%s_%u_%u.reserve",
			 dir,
			 type == NAMESPACE_QUOTA ? "nsp" : "rol",
			 MyDatabaseId,
			 targetoid);

	file = PathNameOpenFile(path, O_RDWR | O_CREAT | PG_BINARY);
	if (file < 0)
	{
		elog(WARNING, "cannot open reserve file \"%s\": %m", path);
		pfree(dir);
		return;
	}

	if (target_bytes <= 0)
	{
		if (FileTruncate(file, 0, WAIT_EVENT_DATA_FILE_TRUNCATE) != 0)
			elog(WARNING, "cannot truncate reserve file \"%s\": %m", path);
	}
	else
	{
#if defined(HAVE_POSIX_FALLOCATE) && defined(__linux__)
		/*
		 * Preallocate the reservation so the remaining free space cannot be
		 * consumed by objects that do not have a min quota guarantee.
		 */
		if (posix_fallocate(FileGetRawDesc(file), 0, target_bytes) != 0)
			elog(WARNING, "cannot preallocate reserve file \"%s\": %m", path);
#else
		/*
		 * Windows has no posix_fallocate() equivalent, so write zeroes
		 * to make the reserve file consume real space.
		 */
		if (write_zeroes_to_reserve_file(file, target_bytes) != 0)
			elog(WARNING, "cannot preallocate reserve file \"%s\": %m", path);
#endif
		if (FileTruncate(file, target_bytes, WAIT_EVENT_DATA_FILE_TRUNCATE) != 0)
			elog(WARNING, "cannot resize reserve file \"%s\": %m", path);
	}

	FileClose(file);
	pfree(dir);
}

/*
 * Remove the stale reserve file that belongs to the given quota owner.
 */
static void
remove_stale_min_quota_reserve_files(QuotaType type, Oid targetoid)
{
	char	   *dir = get_min_quota_reserve_dir();
	char		path[MAXPGPATH];

	/*
	 * When min quota is disabled for one object, clear only that object's
	 * reserve file. Other quota owners may still rely on their reservations.
	 */
	snprintf(path, sizeof(path), "%s/%s_%u_%u.reserve",
			 dir,
			 type == NAMESPACE_QUOTA ? "nsp" : "rol",
			 MyDatabaseId,
			 targetoid);

	if (unlink(path) != 0)
	{
		if (errno != ENOENT)
			elog(WARNING, "cannot remove stale reserve file \"%s\": %m", path);
	}

	pfree(dir);
}

/*
 * Rebuild reserve files from the latest quota map and usage snapshot.
 */
void
reconcile_min_quota_reserve_files(void)
{
	HASH_SEQ_STATUS iter;
	QuotaLimitEntry *quota_entry;

	hash_seq_init(&iter, namespace_quota_limit_map);
	while ((quota_entry = hash_seq_search(&iter)) != NULL)
	{
		NamespaceSizeEntry *nsentry;
		bool found;
		int64 used_bytes = 0;
		int64 reserve_bytes;

		if (quota_entry->minlimitsize <= 0)
		{
			/* Remove only this namespace's stale reservation. */
			remove_stale_min_quota_reserve_files(NAMESPACE_QUOTA,
												quota_entry->targetoid);
			continue;
		}

		nsentry = (NamespaceSizeEntry *) hash_search(namespace_size_map,
												 &quota_entry->targetoid,
												 HASH_FIND,
												 &found);
		if (found)
			used_bytes = nsentry->totalsize;
		reserve_bytes = (quota_entry->minlimitsize * 1024 * 1024) - used_bytes;
		if (reserve_bytes < 0)
			reserve_bytes = 0;

		resize_min_quota_reserve_file(NAMESPACE_QUOTA, quota_entry->targetoid, reserve_bytes);
	}

	hash_seq_init(&iter, role_quota_limit_map);
	while ((quota_entry = hash_seq_search(&iter)) != NULL)
	{
		RoleSizeEntry *rolentry;
		bool found;
		int64 used_bytes = 0;
		int64 reserve_bytes;

		if (quota_entry->minlimitsize <= 0)
		{
			/* Remove only this role's stale reservation. */
			remove_stale_min_quota_reserve_files(ROLE_QUOTA,
												quota_entry->targetoid);
			continue;
		}

		rolentry = (RoleSizeEntry *) hash_search(role_size_map,
											 &quota_entry->targetoid,
											 HASH_FIND,
											 &found);
		if (found)
			used_bytes = rolentry->totalsize;
		reserve_bytes = (quota_entry->minlimitsize * 1024 * 1024) - used_bytes;
		if (reserve_bytes < 0)
			reserve_bytes = 0;

		resize_min_quota_reserve_file(ROLE_QUOTA, quota_entry->targetoid, reserve_bytes);
	}
}

/*
 * Clear every reserve file when the worker exits so reservations can be
 * rebuilt from persisted quota metadata on the next refresh.
 */
void
clear_min_quota_reserve_files(void)
{
	char *dir = get_min_quota_reserve_dir();
	DIR *d = AllocateDir(dir);
	struct dirent *de;

	if (d == NULL)
	{
		pfree(dir);
		return;
	}

	while ((de = ReadDir(d, dir)) != NULL)
	{
		char path[MAXPGPATH];
		File file;

		if (de->d_name[0] == '.')
			continue;
		snprintf(path, sizeof(path), "%s/%s", dir, de->d_name);

		file = PathNameOpenFile(path, O_RDWR | PG_BINARY);
		if (file >= 0)
		{
			(void) FileTruncate(file, 0, WAIT_EVENT_DATA_FILE_TRUNCATE);
			FileClose(file);
		}
	}

	FreeDir(d);
	pfree(dir);
}

/*
 * Given table oid, search for namespace and owner.
 */
static void
get_rel_owner_schema(Oid relid, Oid *ownerOid, Oid *nsOid)
{
	HeapTuple	tp;

	tp = SearchSysCache1(RELOID, ObjectIdGetDatum(relid));
	if (HeapTupleIsValid(tp))
	{
		Form_pg_class reltup = (Form_pg_class) GETSTRUCT(tp);
		*ownerOid = reltup->relowner;
		*nsOid = reltup->relnamespace;
		ReleaseSysCache(tp);
	}
	return;
}

/*
 * Given table oid, check whether quota limit
 * of table's schema or table's owner are reached.
 * Do enforcemet if quota exceeds.
 */
static bool
quota_check_common(Oid ownerOid, Oid nsOid)
{
	bool found = false;
	BlackMapEntry keyitem;

	memset(&keyitem, 0, sizeof(BlackMapEntry));
	LWLockAcquire(diskquota_locks.black_map_lock, LW_SHARED);

	if (nsOid != InvalidOid)
	{
		keyitem.targetoid = nsOid;
		keyitem.databaseoid = MyDatabaseId;
		keyitem.targettype = NAMESPACE_QUOTA;
		hash_search(disk_quota_black_map,
				&keyitem,
				HASH_FIND, &found);
		if (found)
		{
			LWLockRelease(diskquota_locks.black_map_lock);
			/* Begin - ReqID:SRS-SECURITY-AUDIT */
			#ifdef USE_SQLITE
			if (sqlite_open_db_for_audit_hook)
				sqlite_open_db_for_audit_hook(SYSTEMAUDIT_EVENT_DISK_LIMIT_SCHEMA, AUDIT_SUCCESS, MyProcPort->user_name, NULL);
			#endif
			/* End - ReqID:SRS-SECURITY-AUDIT */
			ereport(ERROR,
					(errcode(ERRCODE_DISK_FULL),
					 errmsg("schema's disk space quota exceeded")));
			return false;
		}
	}

	if (ownerOid != InvalidOid)
	{
		keyitem.targetoid = ownerOid;
		keyitem.databaseoid = MyDatabaseId;
		keyitem.targettype = ROLE_QUOTA;
		hash_search(disk_quota_black_map,
				&keyitem,
				HASH_FIND, &found);
		if (found)
		{
			LWLockRelease(diskquota_locks.black_map_lock);

			/* Begin - ReqID:SRS-SECURITY-AUDIT */
			#ifdef USE_SQLITE
			if (sqlite_open_db_for_audit_hook)
				sqlite_open_db_for_audit_hook(SYSTEMAUDIT_EVENT_DISK_LIMIT_ROLE, AUDIT_SUCCESS, MyProcPort->user_name, NULL);
			#endif
			/* End - ReqID:SRS-SECURITY-AUDIT */
			ereport(ERROR,
					(errcode(ERRCODE_DISK_FULL),
					 errmsg("role's disk space quota exceeded")));
			return false;
		}
	}
	LWLockRelease(diskquota_locks.black_map_lock);
	return true;
}

bool
quota_check_common_backlist(Relation reln)
{
	Oid ownerOid = InvalidOid;
	Oid nsOid = InvalidOid;

	if (reln->rd_rel != NULL)
	{
		nsOid = reln->rd_rel->relnamespace;
		ownerOid = reln->rd_rel->relowner;
	}
	return quota_check_common(ownerOid, nsOid);
}

bool
quota_check_common_backlist_by_reloid(Oid reloid)
{
	Oid ownerOid = InvalidOid;
	Oid nsOid = InvalidOid;

	get_rel_owner_schema(reloid, &ownerOid, &nsOid);
	return quota_check_common(ownerOid, nsOid);
}

/*
 * invalidate all black entry with a specific dbid in SHM
 */
void
diskquota_invalidate_db(Oid dbid)
{
	BlackMapEntry * entry;
	HASH_SEQ_STATUS iter;
	LWLockAcquire(diskquota_locks.black_map_lock, LW_EXCLUSIVE);
	hash_seq_init(&iter, disk_quota_black_map);
	while ((entry = hash_seq_search(&iter)) != NULL)
	{
		if (entry->databaseoid == dbid)
		{
			hash_search(disk_quota_black_map, entry, HASH_REMOVE, NULL);
		}
	}
	LWLockRelease(diskquota_locks.black_map_lock);
}

static void init_sessionlist_pid(void)
{
	int i = 0;
	DiskQuotaSessionPids *free_head = &(message_box->session_pid_arry[1]);
	DiskQuotaSessionPids *cur_free_node = (DiskQuotaSessionPids *)free_head;

	/* arry[0] use to session_pid_list head forever */
	message_box->free_pid_list = &(message_box->session_pid_arry[1]);
	message_box->session_pid_list = &(message_box->session_pid_arry[0]);
	message_box->session_pid_list->use_next = (DiskQuotaSessionPids *)message_box->session_pid_list;
	message_box->session_pid_list->use_prev = (DiskQuotaSessionPids *)message_box->session_pid_list;
	message_box->session_pid_list->pid = -1;

	/* set head, tail node free_pid_list */
	cur_free_node->free_prev = &(message_box->session_pid_arry[MaxConnections - 1]);
	message_box->session_pid_arry[MaxConnections - 1].free_prev =
				&(message_box->session_pid_arry[MaxConnections - 2]);
	message_box->session_pid_arry[MaxConnections - 1].free_next = free_head;

	for (i = 1; i < MaxConnections - 1; i++)
	{
		cur_free_node = &(message_box->session_pid_arry[i]);
		cur_free_node->free_next = &(message_box->session_pid_arry[i + 1]);
		if (i > 1)
			cur_free_node->free_prev = &(message_box->session_pid_arry[i - 1]);
	}

	return;
}
