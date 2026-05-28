/* -------------------------------------------------------------------------
 *
 * enforcment.c
 *
 * This code registers enforcement hooks to cancle the query which exceeds 
 * the quota limit.
 *
 * Copyright (C) 2013, PostgreSQL Global Development Group
 *
 *
 * -------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/xact.h"
#include "catalog/catalog.h"
#include "catalog/pg_class.h"
#include "common/hashfn.h"
#include "executor/executor.h"
#include "miscadmin.h"
#include "storage/bufmgr.h"
#include "utils/hsearch.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/rel.h"
#include "utils/syscache.h"

#include "diskquota.h"
#include "activetable.h"

static bool quota_check_ExecCheckRTPerms(List *rangeTable, bool ereport_on_violation);
static bool quota_check_ReadBufferExtendCheckPerms(Relation reln, ForkNumber forkNum,
									   BlockNumber blockNum, ReadBufferMode mode,
									   BufferAccessStrategy strategy);
static bool relation_needs_quota_check(Relation reln);
static void reserve_relation_quota_space(Relation reln, int64 bytes);
static void reserve_quota_target(Oid targetoid, QuotaType type, int64 bytes, bool persist_on_abort);
static void remember_local_reservation(DiskQuotaStateKey *key, int64 bytes, int64 used_base, bool persist_on_abort);
static void diskquota_xact_callback(XactEvent event, void *arg);
static void diskquota_subxact_callback(SubXactEvent event,
										SubTransactionId mySubid,
										SubTransactionId parentSubid,
										void *arg);

static ExecutorCheckPerms_hook_type prev_ExecutorCheckPerms_hook = NULL;
static ReadBufferExtended_hook_type prev_ReadBufferExtended_hook = NULL;

/*
 * Backend-local reservation made while extending a relation.
 *
 * The shared quota state counts these bytes as reserved immediately, then the
 * transaction and subtransaction callbacks either materialize or release the
 * reservation when the write outcome is known.
 */
typedef struct LocalReservationEntry
{
	DiskQuotaStateKey key;
	int64		bytes; /* bytes reserved by this backend */
	int64		persistent_abort_bytes; /* bytes still charged after abort */
	int64		used_base; /* shared used_bytes observed at reserve time */
	SubTransactionId subid; /* subtransaction that owns this reservation */
	struct LocalReservationEntry *next;
} LocalReservationEntry;

static LocalReservationEntry *local_reservations = NULL;
static bool xact_callback_registered = false;

/*
 * Initialize enforcement hooks.
 */
void
init_disk_quota_enforcement(void)
{
	/* enforcement hook before query is loading data */
	prev_ExecutorCheckPerms_hook = ExecutorCheckPerms_hook;
	ExecutorCheckPerms_hook = quota_check_ExecCheckRTPerms;

	/* enforcement hook during query is loading data*/
	prev_ReadBufferExtended_hook = ReadBufferExtended_hook;
	ReadBufferExtended_hook = quota_check_ReadBufferExtendCheckPerms;

	if (!xact_callback_registered)
	{
		/*
		 * Reservations live in backend-local memory and must be reconciled when
		 * transactions or subtransactions finish.
		 */
		RegisterXactCallback(diskquota_xact_callback, NULL);
		RegisterSubXactCallback(diskquota_subxact_callback, NULL);
		xact_callback_registered = true;
	}
}

static bool
relation_needs_quota_check(Relation reln)
{
	if (reln == NULL || reln->rd_rel == NULL)
		return false;

	if (IsBackgroundWorker)
		return false;

	if (RelationGetRelid(reln) < FirstNormalObjectId)
		return false;

	if (!RELKIND_HAS_STORAGE(reln->rd_rel->relkind))
		return false;

	return true;
}

/*
 * Remember quota bytes reserved by the current subtransaction.
 *
 * Each entry tracks the shared-state usage snapshot that was visible when the
 * reservation started. That baseline lets later checks determine how much of
 * the local reservation has already been materialized into used_bytes by the
 * worker.
 */
static void
remember_local_reservation(DiskQuotaStateKey *key, int64 bytes,
						   int64 used_base, bool persist_on_abort)
{
	LocalReservationEntry *entry;
	MemoryContext oldcontext;
	SubTransactionId subid = GetCurrentSubTransactionId();

	for (entry = local_reservations; entry != NULL; entry = entry->next)
	{
		if (entry->subid == subid &&
			entry->key.targetoid == key->targetoid &&
			entry->key.databaseoid == key->databaseoid &&
			entry->key.targettype == key->targettype)
		{
			entry->bytes += bytes;
			if (persist_on_abort)
				entry->persistent_abort_bytes += bytes;
			return;
		}
	}

	oldcontext = MemoryContextSwitchTo(TopMemoryContext);
	entry = palloc0(sizeof(LocalReservationEntry));
	MemoryContextSwitchTo(oldcontext);

	entry->key = *key;
	entry->bytes = bytes;
	entry->persistent_abort_bytes = persist_on_abort ? bytes : 0;
	entry->used_base = used_base;
	entry->subid = subid;
	entry->next = local_reservations;
	local_reservations = entry;
}

/*
 * Return the current backend's aggregated reservation state for one quota
 * target.
 *
 * reserved_bytes is the total bytes reserved locally for the target, while
 * used_base is the earliest shared-state usage snapshot seen by those local
 * reservations. The earliest baseline is needed to de-duplicate worker
 * refreshes against in-flight local growth.
 */
static void
get_local_reservation_state(DiskQuotaStateKey *key,
							int64 *reserved_bytes,
							int64 *used_base)
{
	LocalReservationEntry *entry;
	bool		found = false;

	*reserved_bytes = 0;
	*used_base = 0;

	for (entry = local_reservations; entry != NULL; entry = entry->next)
	{
		if (entry->key.targetoid == key->targetoid &&
			entry->key.databaseoid == key->databaseoid &&
			entry->key.targettype == key->targettype)
		{
			*reserved_bytes += entry->bytes;
			if (!found || entry->used_base < *used_base)
				*used_base = entry->used_base;
			found = true;
		}
	}
}

/*
 * Check whether the current backend already holds any local reservation for
 * the relation's schema or owner quota target.
 *
 * This is used to suppress entry-point quota prechecks for follow-up write
 * statements in the same transaction. Once local reservations exist, the
 * reservation path is the authoritative source because it can de-duplicate the
 * backend's own in-flight growth against worker-refreshed shared usage.
 */
static bool
relation_has_local_quota_reservation(Oid relid)
{
	HeapTuple	tp;
	Form_pg_class reltup;
	DiskQuotaStateKey key;
	int64		reserved_bytes;
	int64		used_base;

	tp = SearchSysCache1(RELOID, ObjectIdGetDatum(relid));
	if (!HeapTupleIsValid(tp))
		return false;

	reltup = (Form_pg_class) GETSTRUCT(tp);

	memset(&key, 0, sizeof(key));
	key.targetoid = reltup->relnamespace;
	key.databaseoid = MyDatabaseId;
	key.targettype = NAMESPACE_QUOTA;
	get_local_reservation_state(&key, &reserved_bytes, &used_base);
	if (reserved_bytes > 0)
	{
		ReleaseSysCache(tp);
		return true;
	}

	memset(&key, 0, sizeof(key));
	key.targetoid = reltup->relowner;
	key.databaseoid = MyDatabaseId;
	key.targettype = ROLE_QUOTA;
	get_local_reservation_state(&key, &reserved_bytes, &used_base);
	ReleaseSysCache(tp);

	return reserved_bytes > 0;
}

/*
 * Reserve quota bytes for one schema or role target.
 *
 * The shared worker may refresh used_bytes while this backend is still
 * extending the relation. To avoid double counting, the limit check only adds
 * the portion of local reservations that has not yet been materialized into
 * shared used_bytes.
 */
static void
reserve_quota_target(Oid targetoid, QuotaType type, int64 bytes,
							  bool persist_on_abort)
{
	DiskQuotaStateKey key;
	DiskQuotaStateEntry *entry;
	int64 local_reserved;
	int64 local_used_base;
	int64 materialized_local_reserved;
	int64 outstanding_local_reserved;
	int64 external_reserved;
	int64 used_base;
	bool found;

	if (!OidIsValid(targetoid))
		return;

	memset(&key, 0, sizeof(key));
	key.targetoid = targetoid;
	key.databaseoid = MyDatabaseId;
	key.targettype = (uint32) type;

	LWLockAcquire(diskquota_locks.quota_state_lock, LW_EXCLUSIVE);
	entry = (DiskQuotaStateEntry *) hash_search(disk_quota_state_map,
												&key,
												HASH_FIND,
												&found);
	if (!found)
	{
		LWLockRelease(diskquota_locks.quota_state_lock);
		return;
	}

	used_base = entry->used_bytes;
	get_local_reservation_state(&key, &local_reserved, &local_used_base);
	if (entry->reserved_bytes > local_reserved)
		external_reserved = entry->reserved_bytes - local_reserved;
	else
		external_reserved = 0;

	/*
	 * The worker can refresh used_bytes while this backend is still extending
	 * the same relation. In that case, some of our local reservations are
	 * already reflected in used_bytes and must not be counted again here.
	 */
	if (entry->used_bytes > local_used_base)
		materialized_local_reserved =
			Min(entry->used_bytes - local_used_base, local_reserved);
	else
		materialized_local_reserved = 0;

	outstanding_local_reserved = local_reserved - materialized_local_reserved;
	/*
	 * Limit enforcement must include local reservations that are not yet
	 * reflected in used_bytes; otherwise a single session can keep extending
	 * the same relation one block at a time without tripping the quota check,
	 * while still avoiding double counting after worker refreshes.
	 */
	if (entry->limit_bytes > 0 &&
		(entry->used_bytes + outstanding_local_reserved + external_reserved + bytes >
		entry->limit_bytes))
	{
		LWLockRelease(diskquota_locks.quota_state_lock);
		ereport(ERROR,
				(errcode(ERRCODE_DISK_FULL),
				 errmsg("%s's disk space quota exceeded",
						type == NAMESPACE_QUOTA ? "schema" : "role")));
	}

	entry->reserved_bytes += bytes;
	LWLockRelease(diskquota_locks.quota_state_lock);

	/* Record the local reservation after the shared state is updated. */
	remember_local_reservation(&key, bytes, used_base, persist_on_abort);
}

/*
 * Reserve bytes for both quota dimensions enforced on a relation.
 *
 * persist_on_abort tells the transaction callbacks whether a failed write can
 * still leave physical space behind. Existing relations typically keep the
 * allocated blocks after an error, while brand new transient relfilenodes do
 * not.
 */
static void
reserve_relation_quota_space(Relation reln, int64 bytes)
{
	Oid namespaceoid;
	Oid owneroid;
	bool persist_on_abort;

	if (!relation_needs_quota_check(reln))
		return;

	namespaceoid = reln->rd_rel->relnamespace;
	owneroid = reln->rd_rel->relowner;
	persist_on_abort =
		(reln->rd_createSubid == InvalidSubTransactionId &&
		 reln->rd_newRelfilenodeSubid == InvalidSubTransactionId);

	reserve_quota_target(namespaceoid, NAMESPACE_QUOTA, bytes, persist_on_abort);
	reserve_quota_target(owneroid, ROLE_QUOTA, bytes, persist_on_abort);
}

/*
 * Release local quota reservations at top-level transaction end.
 *
 * Committed reservations are folded into used_bytes when the worker has not
 * refreshed them yet. Abort handling keeps only reservations for writes that
 * can leave physical blocks allocated after failure.
 */
static void
diskquota_xact_callback(XactEvent event, void *arg)
{
	LocalReservationEntry *entry;
	LocalReservationEntry *next;
	bool is_commit;

	if (local_reservations == NULL)
		return;

	if (event != XACT_EVENT_COMMIT &&
		event != XACT_EVENT_PARALLEL_COMMIT &&
		event != XACT_EVENT_ABORT &&
		event != XACT_EVENT_PARALLEL_ABORT)
		return;

	is_commit = (event == XACT_EVENT_COMMIT ||
				 event == XACT_EVENT_PARALLEL_COMMIT);

	LWLockAcquire(diskquota_locks.quota_state_lock, LW_EXCLUSIVE);
	for (entry = local_reservations; entry != NULL; entry = next)
	{
		DiskQuotaStateEntry *state;
		bool found;

		next = entry->next;
		state = (DiskQuotaStateEntry *) hash_search(disk_quota_state_map,
													&entry->key,
													HASH_FIND,
													&found);
		if (found)
		{
			if (state->reserved_bytes >= entry->bytes)
				state->reserved_bytes -= entry->bytes;
			else
				state->reserved_bytes = 0;

			if (is_commit)
			{
				int64 expected_used = entry->used_base + entry->bytes;

				/* Preserve usage growth until the worker refreshes it. */
				if (state->used_bytes < expected_used)
					state->used_bytes += expected_used - state->used_bytes;
			}
			else if (entry->persistent_abort_bytes > 0)
			{
				int64 expected_used = entry->used_base + entry->persistent_abort_bytes;

				/*
				 * Some failed writes can still leave allocated blocks on an
				 * existing relfilenode, so keep that growth visible to later
				 * quota checks until the worker recomputes exact usage.
				 */
				if (state->used_bytes < expected_used)
					state->used_bytes += expected_used - state->used_bytes;
			}
		}
		pfree(entry);
	}
	local_reservations = NULL;
	LWLockRelease(diskquota_locks.quota_state_lock);
}

/*
 * Move or release local quota reservations when a subtransaction ends.
 *
 * Successful subtransactions are merged into the parent. Aborted
 * subtransactions return reserved bytes to the shared quota state, preserving
 * only physical growth that may survive the abort.
 */
static void
diskquota_subxact_callback(SubXactEvent event, SubTransactionId mySubid,
						   SubTransactionId parentSubid, void *arg)
{
	LocalReservationEntry **prev;
	LocalReservationEntry *entry;

	if (local_reservations == NULL)
		return;

	if (event == SUBXACT_EVENT_COMMIT_SUB)
	{
		LocalReservationEntry *prev = NULL;

		for (entry = local_reservations; entry != NULL; )
		{
			if (entry->subid == mySubid)
			{
				LocalReservationEntry *parent;

				for (parent = local_reservations; parent != NULL; parent = parent->next)
				{
					if (parent == entry)
						continue;
					if (parent->subid == parentSubid &&
						parent->key.targetoid == entry->key.targetoid &&
						parent->key.databaseoid == entry->key.databaseoid &&
						parent->key.targettype == entry->key.targettype)
						break;
				}

				if (parent != NULL)
				{
					/* Coalesce child reservations into an existing parent entry. */
					parent->bytes += entry->bytes;
					parent->persistent_abort_bytes += entry->persistent_abort_bytes;
					if (parent->used_base > entry->used_base)
						parent->used_base = entry->used_base;

					if (prev != NULL)
						prev->next = entry->next;
					else
						local_reservations = entry->next;

					pfree(entry);
					entry = (prev != NULL) ? prev->next : local_reservations;
					continue;
				}

				/* No matching parent entry exists, so retag this entry. */
				entry->subid = parentSubid;
			}

			prev = entry;
			entry = entry->next;
		}
		return;
	}

	if (event != SUBXACT_EVENT_ABORT_SUB)
		return;

	LWLockAcquire(diskquota_locks.quota_state_lock, LW_EXCLUSIVE);
	prev = &local_reservations;
	while ((entry = *prev) != NULL)
	{
		if (entry->subid == mySubid)
		{
			DiskQuotaStateEntry *state;
			bool found;

			state = (DiskQuotaStateEntry *) hash_search(disk_quota_state_map,
														&entry->key,
														HASH_FIND,
														&found);
			if (found)
			{
				/* Return the aborted subtransaction's reserved bytes. */
				if (state->reserved_bytes >= entry->bytes)
					state->reserved_bytes -= entry->bytes;
				else
					state->reserved_bytes = 0;

				if (entry->persistent_abort_bytes > 0)
				{
					int64 expected_used = entry->used_base + entry->persistent_abort_bytes;

					/*
					 * A subtransaction abort releases the reservation, but not
					 * necessarily the already-allocated relation blocks.
					 */
					if (state->used_bytes < expected_used)
						state->used_bytes += expected_used - state->used_bytes;
				}
			}

			*prev = entry->next;
			pfree(entry);
			continue;
		}
		prev = &entry->next;
	}
	LWLockRelease(diskquota_locks.quota_state_lock);
}

/*
 * Enformcent hook function before query is loading data. Throws an error if 
 * you try to INSERT, UPDATE or COPY into a table, and the quota has been exceeded.
 */
static bool
quota_check_ExecCheckRTPerms(List *rangeTable, bool ereport_on_violation)
{
	ListCell   *l;

	if (diskquota_enable != AUOTA_ON ||
		MyProcPort == NULL) /* BUG#Z7841 */
		return true;

	foreach(l, rangeTable)
	{
		RangeTblEntry *rte = (RangeTblEntry *) lfirst(l);

		/* see ExecCheckRTEPerms() */
		if (rte->rtekind != RTE_RELATION)
			continue;

		/*
		 * Only check quota on inserts. UPDATEs may well increase
		 * space usage too, but we ignore that for now.
		 */
		if ((rte->requiredPerms & ACL_INSERT) == 0 && (rte->requiredPerms & ACL_UPDATE) == 0)
			continue;

		/*
		 * Once this backend has already reserved space for the relation's quota
		 * targets, rely on reservation accounting instead of shared-state
		 * prechecks. That avoids false positives in workloads such as DO blocks
		 * that execute many INSERT statements inside one transaction.
		 */
		if (relation_has_local_quota_reservation(rte->relid))
			continue;

		/* Perform the check as the relation's owner and namespace */
		quota_check_common_backlist_by_reloid(rte->relid);
	}

	return true;
}

/*
 * Enforcement hook for relation growth.
 *
 * This path is triggered for every page extension. It relies on reservation
 * accounting only, instead of worker-maintained black-map/shared-state
 * prechecks, so the current backend does not reject its own in-flight growth
 * before reservation de-duplication is applied.
 */
static bool
quota_check_ReadBufferExtendCheckPerms(Relation reln, pg_attribute_unused() ForkNumber forkNum,
							pg_attribute_unused() BlockNumber blockNum,
							pg_attribute_unused() ReadBufferMode mode,
							pg_attribute_unused() BufferAccessStrategy strategy)
{
	if (diskquota_enable != AUOTA_ON ||
		MyProcPort == NULL) /* BUG#Z7841 */
		return true;

	if (!relation_needs_quota_check(reln))
		return true;

	reserve_relation_quota_space(reln, BLCKSZ);

	/*
	 * The extra relation-aware reporting is only needed for relations that are
	 * still private to the current transaction, such as newly created tables or
	 * relations that received a new relfilenode in this xact. For ordinary
	 * committed tables, the smgr hooks are enough and reporting them here makes
	 * the worker observe growth too aggressively during a single statement.
	 */
	if (reln->rd_createSubid != InvalidSubTransactionId ||
		reln->rd_newRelfilenodeSubid != InvalidSubTransactionId)
		report_active_table_relation(reln, AT_EXTEND);

	return true;
}
