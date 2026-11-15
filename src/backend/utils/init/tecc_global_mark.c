/*-------------------------------------------------------------------------
 *
 * tecc_global_mark.c
 *	  TECC global metadata update mark system
 *
 * This module provides per-backend process-local boolean marks to indicate
 * when TECC metadata (hg_kek, hg_dek, hg_encrypted_columns) has been
 * updated. Three independent marks allow selective cache refresh:
 *
 *   - tecc_kek_updated_mark:       set when hg_kek table is modified
 *   - tecc_dek_updated_mark:       set when hg_dek table is modified
 *   - tecc_column_updated_mark:    set when hg_encrypted_columns modified
 *
 * Cross-backend synchronization:
 * These are process-local variables. When another backend modifies a TECC
 * system table, PostgreSQL's shared invalidation message system notifies
 * all other backends. We register syscache callbacks that fire when these
 * invalidation messages are processed, setting the local marks accordingly.
 *
 * Frontend connections check these marks via SQL functions (e.g.,
 * SELECT hg_is_kek_updated()) to determine if they need to refresh
 * their cache.
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 *
 * IDENTIFICATION
 *	  src/backend/utils/init/tecc_global_mark.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "fmgr.h"
#include "miscadmin.h"
#include "port/atomics.h"
#include "utils/inval.h"
#include "utils/syscache.h"
#include "utils/tecc_global_mark.h"
#include "catalog/hg_kek_d.h"
#include "catalog/hg_dek_d.h"
#include "catalog/hg_encrypted_columns_d.h"

/*
 * Per-backend process-local marks indicating TECC metadata has been updated.
 *
 * - volatile: ensures visibility across threads/processes
 * - Initialized to false (no updates since last check)
 */

volatile bool tecc_kek_updated_mark = false;
volatile bool tecc_dek_updated_mark = false;
volatile bool tecc_column_updated_mark = false;

/*
 * Forward declarations of syscache callbacks.
 */
static void TeccKekCacheCallback(Datum arg, int cacheid, uint32 hashvalue);
static void TeccDekCacheCallback(Datum arg, int cacheid, uint32 hashvalue);
static void TeccColumnCacheCallback(Datum arg, int cacheid, uint32 hashvalue);

/*
 * Initialize the TECC global mark system.
 *
 * Registers syscache callbacks for all five TECC system table cache IDs.
 * When another backend modifies a TECC system table, the shared invalidation
 * message system triggers these callbacks, which set the local marks.
 */
void
tecc_init_global_mark(void)
{
	CacheRegisterSyscacheCallback(HGKEKNAME,
								  TeccKekCacheCallback,
								  (Datum) 0);
	CacheRegisterSyscacheCallback(HGKEKOID,
								  TeccKekCacheCallback,
								  (Datum) 0);

	CacheRegisterSyscacheCallback(HGDEKNAME,
								  TeccDekCacheCallback,
								  (Datum) 0);
	CacheRegisterSyscacheCallback(HGDEKOID,
								  TeccDekCacheCallback,
								  (Datum) 0);

	CacheRegisterSyscacheCallback(HGENCRYPTEDCOLUMNSTABLEATTNUM,
								  TeccColumnCacheCallback,
								  (Datum) 0);
}

/*
 * Syscache callback for KEK table changes.
 */
static void
TeccKekCacheCallback(Datum arg, int cacheid, uint32 hashvalue)
{
	pg_memory_barrier();
	tecc_kek_updated_mark = true;
	pg_memory_barrier();
}

/*
 * Syscache callback for DEK table changes.
 */
static void
TeccDekCacheCallback(Datum arg, int cacheid, uint32 hashvalue)
{
	pg_memory_barrier();
	tecc_dek_updated_mark = true;
	pg_memory_barrier();
}

/*
 * Syscache callback for encrypted column table changes.
 */
static void
TeccColumnCacheCallback(Datum arg, int cacheid, uint32 hashvalue)
{
	pg_memory_barrier();
	tecc_column_updated_mark = true;
	pg_memory_barrier();
}

/*
 * tecc_mark_kek_updated - Explicitly mark KEK table as updated.
 *
 * Required for INSERT operations: CatalogTupleInsert sends invalidation
 * messages, but these messages are processed at command boundaries
 * (CommandCounterIncrement() or transaction end). This function ensures
 * the mark is set immediately for the local backend, allowing subsequent
 * operations in the same command to use updated metadata.
 *
 * For UPDATE/DELETE from other backends, the TeccKekCacheCallback
 * syscache callback handles setting the mark when invalidation
 * messages are processed.
 */
void
tecc_mark_kek_updated(void)
{
	/* Function disabled - syscache callbacks handle invalidation */
	/* elog(LOG, "tecc_mark_kek_updated: setting mark (pid=%d)", MyProcPid); */
	/* pg_memory_barrier(); */
	/* tecc_kek_updated_mark = true; */
	/* pg_memory_barrier(); */
}

/*
 * tecc_mark_dek_updated - Explicitly mark DEK table as updated.
 */
void
tecc_mark_dek_updated(void)
{
	/* Function disabled - syscache callbacks handle invalidation */
	/* pg_memory_barrier(); */
	/* tecc_dek_updated_mark = true; */
	/* pg_memory_barrier(); */
}

/*
 * tecc_mark_column_updated - Explicitly mark encrypted columns table as updated.
 */
void
tecc_mark_column_updated(void)
{
	/* Function disabled - syscache callbacks handle invalidation */
	/* pg_memory_barrier(); */
	/* tecc_column_updated_mark = true; */
	/* pg_memory_barrier(); */
}

/* ------------------------------------------------------------------ */
/*                    Mark Clear Functions                             */
/* ------------------------------------------------------------------ */

void
tecc_clear_kek_mark(void)
{
	elog(DEBUG1, "tecc_clear_kek_mark: clearing mark (pid=%d)", MyProcPid);
	pg_memory_barrier();
	tecc_kek_updated_mark = false;
	pg_memory_barrier();
}

void
tecc_clear_dek_mark(void)
{
	pg_memory_barrier();
	tecc_dek_updated_mark = false;
	pg_memory_barrier();
}

void
tecc_clear_column_mark(void)
{
	pg_memory_barrier();
	tecc_column_updated_mark = false;
	pg_memory_barrier();
}

/* ------------------------------------------------------------------ */
/*                    Mark Check Functions                             */
/* ------------------------------------------------------------------ */

bool
tecc_is_kek_updated(void)
{
	bool		result;

	pg_memory_barrier();
	result = tecc_kek_updated_mark;
	pg_memory_barrier();

	elog(DEBUG1, "tecc_is_kek_updated: returning %s (pid=%d)", result ? "true" : "false", MyProcPid);
	return result;
}

bool
tecc_is_dek_updated(void)
{
	bool		result;

	pg_memory_barrier();
	result = tecc_dek_updated_mark;
	pg_memory_barrier();

	return result;
}

bool
tecc_is_column_updated(void)
{
	bool		result;

	pg_memory_barrier();
	result = tecc_column_updated_mark;
	pg_memory_barrier();

	return result;
}

/* ------------------------------------------------------------------ */
/*                    SQL-callable Function Wrappers                   */
/* ------------------------------------------------------------------ */

Datum
hg_is_kek_updated(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(tecc_is_kek_updated());
}

Datum
hg_clear_kek_mark(PG_FUNCTION_ARGS)
{
	tecc_clear_kek_mark();
	PG_RETURN_VOID();
}

Datum
hg_is_dek_updated(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(tecc_is_dek_updated());
}

Datum
hg_clear_dek_mark(PG_FUNCTION_ARGS)
{
	tecc_clear_dek_mark();
	PG_RETURN_VOID();
}

Datum
hg_is_column_updated(PG_FUNCTION_ARGS)
{
	PG_RETURN_BOOL(tecc_is_column_updated());
}

Datum
hg_clear_column_mark(PG_FUNCTION_ARGS)
{
	tecc_clear_column_mark();
	PG_RETURN_VOID();
}

/* ------------------------------------------------------------------ */
/*                    Fastpath Functions                              */
/* ------------------------------------------------------------------ */

/*
 * tecc_get_cache_marks - return bitmask of all cache marks
 * Returns: bitmask (bit0=KEK, bit1=DEK, bit2=COLUMN)
 */
int
tecc_get_cache_marks(void)
{
	int marks = 0;

	pg_memory_barrier();
	if (tecc_kek_updated_mark) marks |= 0x01;
	if (tecc_dek_updated_mark) marks |= 0x02;
	if (tecc_column_updated_mark) marks |= 0x04;
	pg_memory_barrier();

	return marks;
}

/*
 * tecc_clear_cache_marks - clear multiple cache marks at once
 * mask: bitmask of marks to clear
 */
void
tecc_clear_cache_marks(int mask)
{
	if (mask & 0x01) tecc_clear_kek_mark();
	if (mask & 0x02) tecc_clear_dek_mark();
	if (mask & 0x04) tecc_clear_column_mark();
}

/*
 * SQL-callable wrapper for tecc_get_cache_marks
 */
Datum
tecc_get_cache_marks_wrapper(PG_FUNCTION_ARGS)
{
	PG_RETURN_INT32(tecc_get_cache_marks());
}

/*
 * SQL-callable wrapper for tecc_clear_cache_marks
 */
Datum
tecc_clear_cache_marks_wrapper(PG_FUNCTION_ARGS)
{
	int32 mask = PG_GETARG_INT32(0);

	tecc_clear_cache_marks(mask);
	PG_RETURN_VOID();
}

/*
 * tecc_update_meta - SQL-callable function to refresh TECC metadata cache
 *
 * Broadcasts catalog + relcache invalidation for hg_kek / hg_dek /
 * hg_encrypted_columns. Each backend's syscache callback (registered in
 * tecc_init_global_mark) sets its local updated-mark upon receiving the
 * invalidation message, so connected clients re-fetch metadata on their
 * next query; the relcache callback also invalidates the server-side HE
 * public key cache.
 *
 * Usage: SELECT update_meta();
 */
Datum
tecc_update_meta(PG_FUNCTION_ARGS)
{
	/* ReqID:TECC4.0 - 广播 catcache invalidation，触发各后端 syscache 回调
	 * 置 updated-mark=true，客户端下次查询检测到 mark 后刷新缓存。 */
	CacheInvalidateCatalog(HgKekRelationId);
	CacheInvalidateCatalog(HgDekRelationId);
	CacheInvalidateCatalog(HgEncryptedColumnsRelationId);

	/* ReqID:TECC4.0 - 广播 relcache invalidation，触发各后端 relcache 回调，
	 * 使服务端 HE 公钥缓存失效，保证服务端/客户端缓存刷新一致。 */
	CacheInvalidateRelcacheByRelid(HgKekRelationId);
	CacheInvalidateRelcacheByRelid(HgDekRelationId);
	CacheInvalidateRelcacheByRelid(HgEncryptedColumnsRelationId);

	PG_RETURN_VOID();
}
