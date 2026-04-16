/*---------------------------------------------------------------------------------
 *
 * File:dbms_lock.c
 *
 * Abstract:
 *		Internal implementation of DBMS_LOCK package
 *
 * ReqID: SRS-PLSQL-DBMS_LOCK
 *
 * Copyright:
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  contrib/ivorysql_ora/src/builtin_packages/dbms_lock/dbms_lock.c
 *
 *---------------------------------------------------------------------------------
 */

#include <stdlib.h>
#include "postgres.h"
#include "miscadmin.h"
#include "funcapi.h"
#include "storage/proc.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/palloc.h"
#include "utils/lsyscache.h"
#include "access/table.h"
#include "access/xact.h"
#include "storage/lockdefs.h"
#include "storage/lock.h"
#include "access/heapam.h"
#include "access/tableam.h"
#include "access/genam.h"
#include "access/htup_details.h"
#include "catalog/pg_namespace.h"

#define DBMSLOCK_RET_OK             0
#define DBMSLOCK_RET_TIMEOUT        1
#define DBMSLOCK_RET_DEADLOCK       2
#define DBMSLOCK_RET_INVALID_ARGS   3
#define DBMSLOCK_RET_ALREADY_OWNER  4
#define DBMSLOCK_RET_NOT_OWNER      4
#define DBMSLOCK_RET_NO_SUCH_LOCK   5		// BUG #z7621
#define MAXWAIT                     32767
#define DBMS_LOCK_REL_NAME          "dbms_lock_allocated"
#define DBMS_LOCK_IDX_NAME          "dbms_lock_allocated_pkey"
#define LENGTH_OF_LOCKHANDLE        16		// BUG #z7621

/* ************************************************************************
 * A hash table used by each backend, with each backend maintaining
 * its own copy, to record the mapping between locks requested by
 * the backend and release_on_commit.
 * When a lock is requested, a record is added to the hash table;
 * when the lock is released, removing from the hash table.
* *************************************************************************/
HASHCTL ctl;
HTAB    *session_lock_map;

/* value of the hash table */
typedef struct {
	LOCKTAG     key;
	bool        sessionLock;
} session_lock_entry_t;

PG_FUNCTION_INFO_V1(dbms_lock_request_by_id);
PG_FUNCTION_INFO_V1(dbms_lock_request_by_lockhandle);
PG_FUNCTION_INFO_V1(dbms_lock_release_by_id);
PG_FUNCTION_INFO_V1(dbms_lock_release_by_lockhandle);
PG_FUNCTION_INFO_V1(dbms_lock_convert_by_id);
PG_FUNCTION_INFO_V1(dbms_lock_convert_by_lockhandle);

static bool is_lock_exists(int lockId)
{
	Relation rel, idxRel;
	IndexScanDesc indexScan;
	ScanKeyData skey;
	TupleTableSlot *slot;
	bool exists = false;

	rel = table_open(get_relname_relid(DBMS_LOCK_REL_NAME, PG_SYS_NAMESPACE), AccessShareLock);
	idxRel = index_open(get_relname_relid(DBMS_LOCK_IDX_NAME, PG_SYS_NAMESPACE), AccessShareLock);
	ScanKeyInit(&skey, IndexRelationGetNumberOfKeyAttributes(idxRel), BTEqualStrategyNumber, F_INT8EQ, Int8GetDatum(lockId));
	indexScan = index_beginscan(rel, idxRel, GetActiveSnapshot(), IndexRelationGetNumberOfKeyAttributes(idxRel), 0);
	index_rescan(indexScan, &skey, 1, NULL, 0);
	slot = table_slot_create(rel, NULL);

	while (index_getnext_slot(indexScan, ForwardScanDirection, slot))
	{
		exists = true;
		break;
	}

	ExecDropSingleTupleTableSlot(slot);
	index_endscan(indexScan);
	index_close(idxRel, AccessShareLock);
	table_close(rel, AccessShareLock);
	return exists;
}

Datum dbms_lock_request_by_id(PG_FUNCTION_ARGS)
{
	int lockId = (int)PG_GETARG_INT32(0);
	int lockMode = PG_GETARG_INT32(1);
	float8 timeOut = DatumGetFloat8(DirectFunctionCall1(numeric_float8, PG_GETARG_DATUM(2)));
	bool releaseOnCommit = PG_GETARG_BOOL(3);
	bool found;
	LOCKTAG tag;
	session_lock_entry_t *entry;
	waitTime = timeOut > MAXWAIT ? MAXWAIT : timeOut;

	// 1. Validate the parameters
	if (lockMode < OraNullLock || lockMode > OraExecusiveLock || timeOut < 0 || timeOut > MAXWAIT || lockId < 0)		// BUG #z7619
	{
		PG_RETURN_DATUM(Int32GetDatum(DBMSLOCK_RET_INVALID_ARGS));
	}

	// 2. Detected whether lock object is exist
	if (!is_lock_exists(lockId))
	{
		PG_RETURN_DATUM(Int32GetDatum(DBMSLOCK_RET_NO_SUCH_LOCK));		// BUG #z7621
	}

	// 3. Verify whether the lock is already held by us
	SET_LOCKTAG_DBMS_LOCK(tag, MyProc->databaseId, lockId);
	for (int mode = OraNullLock; mode <= OraExecusiveLock; mode++)
	{
		if (LockHeldByMe(&tag, mode))
		{
			PG_RETURN_DATUM(Int32GetDatum(DBMSLOCK_RET_ALREADY_OWNER));
		}
	}

	// 4. All validations passed, trying to acquire lock
	MyProc->waitStatus = PROC_WAIT_STATUS_OK;
	LockAcquire(&tag, lockMode, !releaseOnCommit, false);
	if (MyProc->waitStatus == PROC_WAIT_STATUS_TIMEOUT) {
		PG_RETURN_DATUM(Int32GetDatum(DBMSLOCK_RET_TIMEOUT));
	} else if (MyProc->waitStatus == PROC_WAIT_STATUS_ERROR) {
		PG_RETURN_DATUM(Int32GetDatum(DBMSLOCK_RET_DEADLOCK));
	}

	// 5. request lock success, recording the release_on_commit flag in hash table
	if (session_lock_map == NULL)
	{
		ctl.keysize = sizeof(LOCKTAG);
		ctl.entrysize = sizeof(session_lock_entry_t);
		session_lock_map = hash_create("session dbms lock hash",
									100,
									&ctl,
									HASH_ELEM | HASH_BLOBS);
	}
	entry = (session_lock_entry_t*)hash_search(session_lock_map,
												&tag,
												HASH_ENTER,
												&found);
	if (!found)
	{
		entry->key = tag;
		entry->sessionLock = !releaseOnCommit;
	}
	PG_RETURN_DATUM(Int32GetDatum(DBMSLOCK_RET_OK));
}

Datum dbms_lock_request_by_lockhandle(PG_FUNCTION_ARGS)
{
	text* lockHandle = PG_GETARG_TEXT_PP(0);
	int lockMode = PG_GETARG_INT32(1);
	float8 timeOut = DatumGetFloat8(DirectFunctionCall1(numeric_float8, PG_GETARG_DATUM(2)));
	bool releaseOnCommit = PG_GETARG_BOOL(3);
	char* name = text_to_cstring(lockHandle);
	int len = strlen(name);
	char* lockName = (char *)palloc0(len / 2 + 1);
	int ret;
	int lockId;
	bool found;
	LOCKTAG tag;
	char *endPtr;
	session_lock_entry_t *entry;
	waitTime = timeOut > MAXWAIT ? MAXWAIT : timeOut;

	// 1. Validate the parameters
	if (lockMode < OraNullLock || lockMode > OraExecusiveLock || timeOut < 0 || timeOut > MAXWAIT || len != LENGTH_OF_LOCKHANDLE)		// BUG #z7619 #z7621
	{
		ret = DBMSLOCK_RET_INVALID_ARGS;
		goto exit;
	}
	/*
	* The construction rule of lockhandle is: lockid || lockid. Therefor, it's only
	* necessary to verify that the first and second halves are identical
	*/
	for (int i = 0; i < len / 2; i ++)
	{
		if ((name[i] != name[i + len / 2]))
		{
			ret = DBMSLOCK_RET_INVALID_ARGS;
			goto exit;
		}
		lockName[i] = name[i];
	}
	lockName[len / 2] = '\0';
	lockId  = (int)strtol(lockName, &endPtr, 16);
	if (endPtr == lockName || *endPtr != '\0')
	{
		ret = DBMSLOCK_RET_INVALID_ARGS;
		goto exit;
	}

	// 2. Detected whether lock object is exist
	if (!is_lock_exists(lockId))
	{
		ret = DBMSLOCK_RET_NO_SUCH_LOCK;
		goto exit;
	}

	// 3. Verify whether the lock is already held by us
	SET_LOCKTAG_DBMS_LOCK(tag, MyProc->databaseId, lockId);
	for (int mode = OraNullLock; mode <= OraExecusiveLock; mode++)
	{
		if (LockHeldByMe(&tag, mode))
		{
			ret = DBMSLOCK_RET_ALREADY_OWNER;
			goto exit;
		}
	}

	// 4. All validations passed, trying to acquire lock
	MyProc->waitStatus = PROC_WAIT_STATUS_OK;
	LockAcquire(&tag, lockMode, !releaseOnCommit, false);
	if (MyProc->waitStatus == PROC_WAIT_STATUS_TIMEOUT)
	{
		ret = DBMSLOCK_RET_TIMEOUT;
		goto exit;
	}
	else if (MyProc->waitStatus == PROC_WAIT_STATUS_ERROR)
	{
		ret = DBMSLOCK_RET_DEADLOCK;
		goto exit;
	}

	// 5. request lock success, recording the release_on_commit flag in hash map
	if (session_lock_map == NULL)
	{
		ctl.keysize = sizeof(LOCKTAG);
		ctl.entrysize = sizeof(session_lock_entry_t);
		session_lock_map = hash_create("session dbms lock hash",
									100,
									&ctl,
									HASH_ELEM | HASH_BLOBS);
	}
	entry = (session_lock_entry_t*)hash_search(session_lock_map,
												&tag,
												HASH_ENTER,
												&found);
	if (!found)
	{
		entry->key = tag;
		entry->sessionLock = !releaseOnCommit;
	}

	ret = DBMSLOCK_RET_OK;
exit:
	if (name != NULL)
		pfree(name);
	if (lockName != NULL)
		pfree(lockName);
	PG_RETURN_DATUM(Int32GetDatum(ret));
}

Datum dbms_lock_release_by_id(PG_FUNCTION_ARGS)
{
	int lockId = (int)PG_GETARG_INT32(0);
	bool found;
	LOCKTAG tag;
	session_lock_entry_t *entry;
	bool held = false;
	// 1. Validate the parameters
	if (lockId <= 0)
	{
		PG_RETURN_DATUM(Int32GetDatum(DBMSLOCK_RET_INVALID_ARGS));
	}
	// 2. Detected whether lock object is exist
	if (!is_lock_exists(lockId))
	{
		PG_RETURN_DATUM(Int32GetDatum(DBMSLOCK_RET_NO_SUCH_LOCK));		// BUG #z7619 #z7621
	}

	// 3. trying to release lock
	if (session_lock_map == NULL)		// session_lock_map is nullptr, meaning the current backend has not requested any lock yet!
	{
		PG_RETURN_DATUM(Int32GetDatum(DBMSLOCK_RET_NOT_OWNER));
	}
	SET_LOCKTAG_DBMS_LOCK(tag, MyProc->databaseId, lockId);
	entry = (session_lock_entry_t*)hash_search(session_lock_map,
												&tag,
												HASH_FIND,
												&found);
	if (!found)
	{
		PG_RETURN_DATUM(Int32GetDatum(DBMSLOCK_RET_NOT_OWNER));
	}
	for (int mode = OraNullLock; mode <= OraExecusiveLock; mode++)
	{
		if (LockHeldByMe(&tag, mode))
		{
			held = true;
			LockRelease(&tag, mode, entry->sessionLock);
		}
	}
	if (!held)
	{
		PG_RETURN_DATUM(Int32GetDatum(DBMSLOCK_RET_NOT_OWNER));
	}

	// 4. remove entry from session_lock_map
	hash_search(session_lock_map, &tag,	HASH_REMOVE, &found);
	PG_RETURN_DATUM(Int32GetDatum(DBMSLOCK_RET_OK));
}

Datum dbms_lock_release_by_lockhandle(PG_FUNCTION_ARGS)
{
	text* lockHandle = PG_GETARG_TEXT_PP(0);
	char* name = text_to_cstring(lockHandle);
	int len = strlen(name);
	char* lockName = (char *)palloc0(len / 2 + 1);
	int ret;
	bool found;
	int lockId;
	char *endPtr;
	LOCKTAG tag;
	session_lock_entry_t *entry;
	bool held = false;

	// 1. Validate the parameters
	if (len != LENGTH_OF_LOCKHANDLE)		// BUG #z7619 #z7621
	{
		ret = DBMSLOCK_RET_INVALID_ARGS;
		goto exit;
	}
	for (int i = 0; i < len / 2; i ++)
	{
		if ((name[i] != name[i + len / 2]))
		{
			ret = DBMSLOCK_RET_INVALID_ARGS;
			goto exit;
		}
		lockName[i] = name[i];
	}
	lockName[len / 2] = '\0';
	lockId  = (int)strtol(lockName, &endPtr, 16);
	if (endPtr == lockName || *endPtr != '\0')
	{
		ret = DBMSLOCK_RET_INVALID_ARGS;
		goto exit;
	}

	// 2. Detected whether lock object is exist
	if (!is_lock_exists(lockId))
	{
		ret = DBMSLOCK_RET_NO_SUCH_LOCK;
		goto exit;
	}

	// 3. trying to release lock
	if (session_lock_map == NULL)
	{
		ret = DBMSLOCK_RET_NOT_OWNER;
		goto exit;
	}
	SET_LOCKTAG_DBMS_LOCK(tag, MyProc->databaseId, lockId);
	entry = (session_lock_entry_t*)hash_search(session_lock_map,
												&tag,
												HASH_FIND,
												&found);
	if (!found)
	{
		ret = DBMSLOCK_RET_NOT_OWNER;
		goto exit;
	}
	for (int mode = OraNullLock; mode <= OraExecusiveLock; mode++)
	{
		if (LockHeldByMe(&tag, mode))
		{
			held = true;
			LockRelease(&tag, mode, entry->sessionLock);
		}
	}
	if (!held)
	{
		ret = DBMSLOCK_RET_NOT_OWNER;
		goto exit;
	}

	// 4. remove entry from session_lock_map
	hash_search(session_lock_map, &tag,	HASH_REMOVE, &found);
	ret = DBMSLOCK_RET_OK;
exit:
	if (name != NULL)
		pfree(name);
	if (lockName != NULL)
		pfree(lockName);
	PG_RETURN_DATUM(Int32GetDatum(ret));
}

Datum dbms_lock_convert_by_id(PG_FUNCTION_ARGS)
{
	int lockId = (int)PG_GETARG_INT32(0);
	int lockMode = PG_GETARG_INT32(1);
	float8 timeOut = DatumGetFloat8(DirectFunctionCall1(numeric_float8, PG_GETARG_DATUM(2)));
	bool found;
	int heldMode;
	LOCKTAG tag;
	session_lock_entry_t *entry;
	bool held = false;

	// 1. Validate the parameters
	if (lockMode < OraNullLock || lockMode > OraExecusiveLock || timeOut < 0 || timeOut > MAXWAIT || lockId < 0)		// BUG #z7619 #z7621
	{
		PG_RETURN_DATUM(Int32GetDatum(DBMSLOCK_RET_INVALID_ARGS));
	}

	// 2. Detected whether lock object is exist
	if (!is_lock_exists(lockId))
	{
		PG_RETURN_DATUM(Int32GetDatum(DBMSLOCK_RET_NO_SUCH_LOCK));
	}

	waitTime = timeOut > MAXWAIT ? MAXWAIT : timeOut;
	SET_LOCKTAG_DBMS_LOCK(tag, MyProc->databaseId, lockId);
	MyProc->waitStatus = PROC_WAIT_STATUS_OK;

	// 3. trying to convert lockmode
	if (session_lock_map == NULL)
	{
		PG_RETURN_DATUM(Int32GetDatum(DBMSLOCK_RET_NOT_OWNER));
	}
	entry = (session_lock_entry_t*)hash_search(session_lock_map,
												&tag,
												HASH_FIND,
												&found);
	if (!found)
	{
		PG_RETURN_DATUM(Int32GetDatum(DBMSLOCK_RET_NOT_OWNER));
	}
	for (int mode = OraNullLock; mode <= OraExecusiveLock; mode++)
	{
		if (LockHeldByMe(&tag, mode))
		{
			held = true;
			heldMode = mode;
			LockAcquire(&tag, lockMode, entry->sessionLock, false);
			break;
		}
	}
	if (!held)
	{
		PG_RETURN_DATUM(Int32GetDatum(DBMSLOCK_RET_NOT_OWNER));
	}
	if (MyProc->waitStatus == PROC_WAIT_STATUS_TIMEOUT)
	{
		PG_RETURN_DATUM(Int32GetDatum(DBMSLOCK_RET_TIMEOUT));
	}
	else if (MyProc->waitStatus == PROC_WAIT_STATUS_ERROR)
	{
		PG_RETURN_DATUM(Int32GetDatum(DBMSLOCK_RET_DEADLOCK));
	}
	LockRelease(&tag, heldMode, entry->sessionLock);
	PG_RETURN_DATUM(Int32GetDatum(DBMSLOCK_RET_OK));
}

Datum dbms_lock_convert_by_lockhandle(PG_FUNCTION_ARGS)
{
	text* lockHandle = PG_GETARG_TEXT_PP(0);
	int lockMode = PG_GETARG_INT32(1);
	float8 timeOut = DatumGetFloat8(DirectFunctionCall1(numeric_float8, PG_GETARG_DATUM(2)));
	char* name = text_to_cstring(lockHandle);
	int len = strlen(name);
	char* lockName = (char *)palloc0(len / 2 + 1);
	int ret;
	bool found;
	int lockId;
	char *endPtr;
	LOCKTAG tag;
	int heldMode;
	session_lock_entry_t *entry;
	bool held = false;

	// 1. Validate the parameters
	if (lockMode < OraNullLock || lockMode > OraExecusiveLock || timeOut < 0 || timeOut > MAXWAIT || len != LENGTH_OF_LOCKHANDLE)		// BUG #z7619 #z7621
	{
		ret = DBMSLOCK_RET_INVALID_ARGS;
		goto exit;
	}

	for (int i = 0; i < len / 2; i ++)
	{
		if ((name[i] != name[i + len / 2]))
		{
			ret = DBMSLOCK_RET_INVALID_ARGS;
			goto exit;
		}
		lockName[i] = name[i];
	}
	lockName[len / 2] = '\0';
	lockId  = (int)strtol(lockName, &endPtr, 16);
	if (endPtr == lockName || *endPtr != '\0')
	{
		ret = DBMSLOCK_RET_INVALID_ARGS;
		goto exit;
	}

	// 2. Detected whether lock object is exist
	if (!is_lock_exists(lockId))
	{
		ret = DBMSLOCK_RET_NO_SUCH_LOCK;
		goto exit;
	}

	waitTime = timeOut > MAXWAIT ? MAXWAIT : timeOut;
	SET_LOCKTAG_DBMS_LOCK(tag, MyProc->databaseId, lockId);
	MyProc->waitStatus = PROC_WAIT_STATUS_OK;

	// 3. trying to convert lockmode
	if (session_lock_map == NULL)
	{
		ret = DBMSLOCK_RET_NOT_OWNER;
		goto exit;
	}

	entry = (session_lock_entry_t*)hash_search(session_lock_map,
												&tag,
												HASH_FIND,
												&found);
	if (!found)
	{
		ret = DBMSLOCK_RET_NOT_OWNER;
		goto exit;
	}
	for (int mode = OraNullLock; mode <= OraExecusiveLock; mode++)
	{
		if (LockHeldByMe(&tag, mode))
		{
			held = true;
			heldMode = mode;
			LockAcquire(&tag, lockMode, entry->sessionLock, false);
			break;
		}
	}

	if (!held)
	{
		ret = DBMSLOCK_RET_NOT_OWNER;
		goto exit;
	}
	if (MyProc->waitStatus == PROC_WAIT_STATUS_TIMEOUT)
	{
		ret = DBMSLOCK_RET_TIMEOUT;
		goto exit;
	}
	else if (MyProc->waitStatus == PROC_WAIT_STATUS_ERROR)
	{
		ret = DBMSLOCK_RET_DEADLOCK;
		goto exit;
	}
	LockRelease(&tag, heldMode, entry->sessionLock);
	ret = DBMSLOCK_RET_OK;
exit:
	if (name != NULL)
		pfree(name);
	if (lockName != NULL)
		pfree(lockName);
	PG_RETURN_DATUM(Int32GetDatum(ret));
}
