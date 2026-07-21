/*--------------------------------------------------------------------
 *
 * File: hg_profiler.c
 *
 * Abstract:
 *		Per-query SQL execution profiler that uses the SQL Profiler
 *		on-disk format.
 *
 * Copyright:
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *		contrib/hg_profiler/hg_profiler.c
 *
 * add the file for requirement "SRS-TOOLS-HG_PROFILER
 *
 *--------------------------------------------------------------------
 */

/*-------------------------------------------------------------------------
 *
 * The module installs the standard set of executor / shared-memory hooks:
 *
 *		shmem_request_hook		reserves the shared-memory area used to
 *								track active traces.
 *		shmem_startup_hook		initialises that area on cluster boot.
 *		ExecutorStart_hook		decides whether the current top-level
 *								query should be tracked, and arms node
 *								instrumentation accordingly.
 *		ExecutorRun_hook		maintains the nesting-level counter so
 *								we can ignore nested SPI sub-queries.
 *		ExecutorEnd_hook		captures the per-query profile row and
 *								appends it to the trace's CSV file.
 *
 * On-disk layout (relative to $PGDATA):
 *
 *		hg_profiler/<trace_id>/trace.ini
 *		hg_profiler/<trace_id>/trace-<pid>.csv
 *
 * The .ini file holds per-trace metadata (status, owner, filters, ...).
 * Each backend that contributes rows opens its own append-only CSV file
 * named after its PID; this avoids any cross-backend serialisation in
 * the hot path.  hg_p_load_trace() materialises those CSV files into the
 * temporary table _hg_tmp_tbl_p_profiler so SQL Profiler tooling can
 * read the captured data unchanged.
 *
 * IDENTIFICATION
 *		contrib/hg_profiler/hg_profiler.c
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#else
#include "rusagestub.h"
#endif
#include <unistd.h>

/*
 * Native <sys/resource.h> exposes the full rusage counters we want.
 * On Windows (and other platforms that use rusagestub.h) only ru_utime /
 * ru_stime exist, so the ten rusage CSV columns are written as 0 to keep
 * the column shape stable across platforms.
 */
#ifdef HAVE_SYS_RESOURCE_H
#define HGP_RU_DELTA(end_, start_, member)	\
	((int64) ((end_)->member - (start_)->member))
#else
#define HGP_RU_DELTA(end_, start_, member)      ((int64) 0)
#endif

#include "access/xact.h"
#include "access/parallel.h"
#include "catalog/pg_authid.h"
#include "catalog/pg_type.h"
#include "commands/explain.h"
#include "executor/executor.h"
#include "executor/instrument.h"
#include "executor/spi.h"
#include "fmgr.h"
#include "funcapi.h"
#include "miscadmin.h"
#include "nodes/execnodes.h"
#include "pgtime.h"
#include "port.h"
#include "port/atomics.h"
#include "storage/fd.h"
#include "storage/ipc.h"
#include "storage/lwlock.h"
#include "storage/proc.h"
#include "storage/shmem.h"
#include "utils/acl.h"
#include "utils/builtins.h"
#include "utils/datetime.h"
#include "utils/datum.h"
#include "utils/guc.h"
#include "utils/memutils.h"
#include "utils/timestamp.h"

PG_MODULE_MAGIC;

/* Maximum number of concurrently-active traces. */
#define HGP_MAX_TRACES				10
/* Maximum number of OIDs that can be specified in users / databases filters. */
#define HGP_MAX_FILTER_OIDS			5
/* Length of trace_id (YYYYMMDDhhmmss + 6-digit counter) plus NUL. */
#define HGP_TRACE_ID_LEN			20
#define HGP_TRACE_ID_BUFSZ			(HGP_TRACE_ID_LEN + 1)
/* Cached copy of trace comments kept in shared memory for active traces. */
#define HGP_COMMENTS_BUFSZ			1024

/*
 * The PostgreSQL extension version (declared in hg_profiler.control) is
 * a separate, unrelated number.
 */
#define HGP_PROFILER_VERSION			1.0

/* Trace status values stored in trace.ini's Status= line. */
#define HGP_STATUS_SERVER_RESTART	0
#define HGP_STATUS_ACTIVE			1
#define HGP_STATUS_STOPPED			2
#define HGP_STATUS_MAX_SIZE			3
#define HGP_STATUS_TIME_LIMIT		4

/* Default hg_p_activate arguments when NULL. */
#define HGP_DEFAULT_MAX_LOG_LIMIT_MB		100
#define HGP_DEFAULT_LOG_MIN_DURATION_SEC	5
#define HGP_DEFAULT_TIME_LIMIT_USEC		(10 * USECS_PER_MINUTE)
#define HGP_TIME_LIMIT_MIN_USEC			((int64) USECS_PER_MINUTE)
#define HGP_TIME_LIMIT_MAX_USEC			((int64) 180 * USECS_PER_MINUTE)

/* On-disk root directory name (relative to DataDir). */
#define HGP_ROOT_DIR				"hg_profiler"
#define HGP_INI_FILE				"trace.ini"
#define HGP_CSV_PREFIX				"trace-"
#define HGP_CSV_SUFFIX				".csv"

/* Number of fields in a CSV record (must match hg_p_emit_query_sample). */
#define HGP_CSV_NCOLS				29

/* IO buffer size used for trace-<pid>.csv files. */
#define HGP_CSV_IOBUF_SIZE			(64 * 1024)

/*
 * Shared state
 */
typedef struct HgpTraceShm
{
	bool		in_use;			/* slot in use*/
	uint32		generation;		/* bumped every hg_p_activate / finalise so
						 * backend-local file handles can detect
						 * stale slots
						 */
	char		trace_id[HGP_TRACE_ID_BUFSZ];
	char		comments[HGP_COMMENTS_BUFSZ];
	Oid		owner;			/* role that ran hg_p_activate() */
	TimestampTz start_time;
	TimestampTz end_time;		/* scheduled stop time */
	int32		max_log_size_mb;
	int32		log_min_duration_sec;
	int32		n_users;
	int32		n_databases;
	Oid		users[HGP_MAX_FILTER_OIDS];
	Oid		databases[HGP_MAX_FILTER_OIDS];
	pg_atomic_uint32 running_queries;
	pg_atomic_uint64 curr_log_size;	/* bytes written to all CSV files */
	int32		final_status;	/* HGP_STATUS_* set when slot is being torn
					 * down, before in_use flips back to false
					 */
} HgpTraceShm;

typedef struct HgpSharedState
{
	LWLock	   *lock;
	pg_atomic_uint32 active_count;	/* number of in_use slots */
	uint32		next_counter;	/* trace_id 6-digit suffix counter */
	HgpTraceShm	traces[HGP_MAX_TRACES];
} HgpSharedState;

static HgpSharedState *hgp_shared = NULL;

/*
 * Per-backend cache: one open trace-<pid>.csv FILE * per active slot.
 *
 * Indexed by slot number.  generation/trace_id let us detect a slot that
 * was reused by another hg_p_activate() while we were holding a stale fp,
 * in which case we drop our handle and re-open against the new trace.
 */
typedef struct HgpLocalFile
{
	FILE	*fp;
	uint32	generation;
	char	trace_id[HGP_TRACE_ID_BUFSZ];
} HgpLocalFile;

static HgpLocalFile hgp_local_files[HGP_MAX_TRACES];
static bool hgp_atexit_registered = false;

/*
 * GUCs
 */
static bool hgp_enabled = false;

/*
 * Saved hooks
 */
#if PG_VERSION_NUM >= 150000
static shmem_request_hook_type	prev_shmem_request_hook = NULL;
#endif
static shmem_startup_hook_type	prev_shmem_startup_hook = NULL;
static ExecutorStart_hook_type	prev_ExecutorStart = NULL;
static ExecutorRun_hook_type	prev_ExecutorRun = NULL;
static ExecutorEnd_hook_type	prev_ExecutorEnd = NULL;

/*
 * Per-backend state for the in-flight top-level query
 */
static int	hgp_nesting_level = 0;
static bool	hgp_current_tracked = false;
static int	hgp_current_trace_count = 0;
static int	hgp_current_trace_idxs[HGP_MAX_TRACES];
static struct rusage	hgp_current_rusage_start;
static BufferUsage	hgp_current_buf_start;
static TimestampTz	hgp_current_start_ts;

/*
 * trace.ini struct (parsed contents)
 */
typedef struct HgpTraceIni
{
	double	version;
	int	status;
	char	*comments;		/* palloc'd; possibly empty string */
	TimestampTz start_time;
	Oid	owner;
	Oid	users[HGP_MAX_FILTER_OIDS];
	int	n_users;
	Oid	databases[HGP_MAX_FILTER_OIDS];
	int	n_databases;
	int32	max_log_size_mb;
	int32	log_min_duration_sec;
	TimestampTz end_time;
	TimestampTz finish_time;
	bool	end_time_set;
	bool	finish_time_set;
} HgpTraceIni;

/*
 * Forward declarations
 */
void		_PG_init(void);

PG_FUNCTION_INFO_V1(hg_p_activate);
PG_FUNCTION_INFO_V1(hg_p_deactivate);
PG_FUNCTION_INFO_V1(hg_p_active_traces);
PG_FUNCTION_INFO_V1(hg_p_traces_list);
PG_FUNCTION_INFO_V1(hg_p_load_trace);
PG_FUNCTION_INFO_V1(hg_p_cleanup);
PG_FUNCTION_INFO_V1(hg_p_version);

static bool hg_p_read_trace_ini(const char *trace_id, HgpTraceIni *ini);
static void hg_p_free_trace_ini(HgpTraceIni *ini);

#if PG_VERSION_NUM >= 150000
static void hg_p_shmem_request(void);
#endif
static void hg_p_shmem_startup(void);
static void hg_p_restore_traces_from_disk(void);
static void hg_p_ExecutorStart_hook(QueryDesc *queryDesc, int eflags);
static void hg_p_ExecutorRun_hook(QueryDesc *queryDesc, ScanDirection direction,
								  uint64 count, bool execute_once);
static void hg_p_ExecutorEnd_hook(QueryDesc *queryDesc);
static void hg_p_xact_callback(XactEvent event, void *arg);
static void hg_p_before_shmem_exit(int code, Datum arg);
static int hg_p_match_active_traces(Oid user_id, Oid db_id,
									int *matched_idxs, int max_matches);

static void hg_p_release_query_slot(int trace_idx);
static void hg_p_check_limits(int trace_idx, TimestampTz now);
static void hg_p_prune_finished_traces(TimestampTz now);
static void hg_p_emit_query_sample(int trace_idx,
								   QueryDesc *queryDesc, double duration_ms,
								   const struct rusage *ru_end,
								   const BufferUsage *buf_end);
static char *hg_p_format_explain(QueryDesc *queryDesc);

static void hg_p_close_local_file(int slot);
static void hg_p_close_all_local_files(void);
static FILE *hg_p_get_local_file(int slot);

static void hg_p_csv_append_quoted(StringInfo dst, const char *src);

static void hg_p_make_trace_root(void);
static void hg_p_make_trace_dir(const char *trace_id);
static uint64 hg_p_dir_size(const char *path);
static bool hg_p_parse_trace_csv_filename(const char *name, int *pid_out);
static void hg_p_path_root(char *buf, size_t bufsz);
static void hg_p_path_trace_dir(char *buf, size_t bufsz, const char *trace_id);
static void hg_p_path_trace_ini(char *buf, size_t bufsz, const char *trace_id);
static void hg_p_path_trace_csv(char *buf, size_t bufsz, const char *trace_id, int pid);

static void hg_p_write_trace_ini(const char *trace_id,
								 const char *comments,
								 Oid owner,
								 TimestampTz start_time,
								 TimestampTz end_time,
								 int n_users, const Oid *users,
								 int n_dbs, const Oid *dbs,
								 int32 max_log_size_mb,
								 int32 log_min_duration_sec,
								 int status,
								 TimestampTz finish_time);

static bool hg_p_finalise_trace_in_shm_inner(int trace_idx, int status,
											 uint32 expected_gen,
											 char trace_id_out[HGP_TRACE_ID_BUFSZ]);
static void hg_p_finalise_trace(int trace_idx, int status,
								TimestampTz finish_time);
static bool hg_p_temp_relation_exists(const char *relname);
static void hg_p_ensure_load_tables(void);
static void hg_p_reset_load_table(void);

static bool hg_p_is_valid_trace_id(const char *trace_id);

/*
 * Module init
 */
void
_PG_init(void)
{
	if (!process_shared_preload_libraries_in_progress)
	{
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("hg_profiler must be loaded via shared_preload_libraries")));
	}

	DefineCustomBoolVariable("hg_profiler.enabled",
							 "Master switch for the hg_profiler module.",
							 NULL,
							 &hgp_enabled,
							 false,
							 PGC_SUSET,
							 0,
							 NULL, NULL, NULL);
#if PG_VERSION_NUM >= 150000
	MarkGUCPrefixReserved("hg_profiler");
#else
	EmitWarningsOnPlaceholders("hg_profiler");
#endif

	/* Hook installation */
#if PG_VERSION_NUM >= 150000
	prev_shmem_request_hook = shmem_request_hook;
	shmem_request_hook = hg_p_shmem_request;
#else
	RequestAddinShmemSpace(MAXALIGN(sizeof(HgpSharedState)));
	RequestNamedLWLockTranche("hg_profiler", 1);
#endif

	/* Hook installation */
	prev_shmem_startup_hook = shmem_startup_hook;
	shmem_startup_hook = hg_p_shmem_startup;

	prev_ExecutorStart = ExecutorStart_hook;
	ExecutorStart_hook = hg_p_ExecutorStart_hook;

	prev_ExecutorRun = ExecutorRun_hook;
	ExecutorRun_hook = hg_p_ExecutorRun_hook;

	prev_ExecutorEnd = ExecutorEnd_hook;
	ExecutorEnd_hook = hg_p_ExecutorEnd_hook;

	/*
	 * Abort callback so a failed user query does not leak the in-flight
	 * running_queries counter on its trace slot.
	 */
	RegisterXactCallback(hg_p_xact_callback, NULL);
}

/*
 * Backend-shutdown cleanup -- close any per-backend trace CSV file
 * handles cleanly, even if the backend is exiting because of a FATAL.
 */
static void
hg_p_before_shmem_exit(int code, Datum arg)
{
	(void) code;
	(void) arg;
	hg_p_close_all_local_files();
}

/*
 * xact callback -- if the transaction containing a tracked query rolls
 * back before ExecutorEnd had a chance to run, release the slot and
 * reset our in-flight bookkeeping.
 */
static void
hg_p_xact_callback(XactEvent event, void *arg)
{
	(void) arg;

	switch (event)
	{
		case XACT_EVENT_ABORT:
		case XACT_EVENT_PARALLEL_ABORT:
			if (hgp_current_tracked)
			{
				for (int i = 0; i < hgp_current_trace_count; i++)
				{
					hg_p_release_query_slot(hgp_current_trace_idxs[i]);
					hg_p_close_local_file(hgp_current_trace_idxs[i]);
				}
			}
			hgp_current_tracked = false;
			hgp_current_trace_count = 0;
			break;

		case XACT_EVENT_COMMIT:
		case XACT_EVENT_PARALLEL_COMMIT:
		case XACT_EVENT_PREPARE:
			/*
			 * hg_p_close_all_local_files() iterates HGP_MAX_TRACES slots
			 * and is a no-op for slots whose fp is already NULL, so the
			 * cost is negligible.
			 */
			hg_p_close_all_local_files();
			break;

		default:
			break;
	}
}

/*
 * shmem hooks
 */
#if PG_VERSION_NUM >= 150000
static void
hg_p_shmem_request(void)
{
	if (prev_shmem_request_hook)
		prev_shmem_request_hook();

	RequestAddinShmemSpace(MAXALIGN(sizeof(HgpSharedState)));
	RequestNamedLWLockTranche("hg_profiler", 1);
}
#endif

static void
hg_p_shmem_startup(void)
{
	bool		found;

	if (prev_shmem_startup_hook)
		prev_shmem_startup_hook();

	hgp_shared = ShmemInitStruct("hg_profiler",
								sizeof(HgpSharedState),
								&found);
	if (!found)
	{
		MemSet(hgp_shared, 0, sizeof(HgpSharedState));
		hgp_shared->lock = &(GetNamedLWLockTranche("hg_profiler"))->lock;
		pg_atomic_init_u32(&hgp_shared->active_count, 0);
		hgp_shared->next_counter = 0;

		for (int i = 0; i < HGP_MAX_TRACES; i++)
		{
			pg_atomic_init_u32(&hgp_shared->traces[i].running_queries, 0);
			pg_atomic_init_u64(&hgp_shared->traces[i].curr_log_size, 0);
		}

		/*
		 * Only the postmaster (or the single backend that races to create
		 * the shared-memory segment under EXEC_BACKEND) restores the
		 * trace.ini files left over from a previous server run.  Doing
		 * this unconditionally would re-scan $PGDATA/hg_profiler on every
		 * EXEC_BACKEND child startup and clobber the shared slots that
		 * other backends have already populated.
		 */
		hg_p_restore_traces_from_disk();
	}
}

/*
 * Trace-matching helper
 */
static void
hg_p_restore_traces_from_disk(void)
{
	char		root[MAXPGPATH];
	DIR		   *dir;
	struct dirent *ent;
	TimestampTz now = GetCurrentTimestamp();

	hg_p_path_root(root, sizeof(root));

	dir = AllocateDir(root);
	if (dir == NULL)
	{
		if (errno == ENOENT)
			return;
		ereport(WARNING,
				(errcode_for_file_access(),
				 errmsg("hg_profiler: could not open \"%s\": %m", root)));
		return;
	}

	while ((ent = ReadDir(dir, root)) != NULL)
	{
		HgpTraceIni	ini;
		int			slot_idx = -1;
		uint64		disk_size = 0;

		if (ent->d_name[0] == '.')
			continue;
		if (!hg_p_is_valid_trace_id(ent->d_name))
			continue;

		if (!hg_p_read_trace_ini(ent->d_name, &ini))
			continue;

		if (ini.status != HGP_STATUS_ACTIVE)
		{
			hg_p_free_trace_ini(&ini);
			continue;
		}

		/*
		 * time_limit is mandatory, so an ACTIVE trace whose scheduled end is
		 * already in the past should not be restored after restart.
		 */
		if (ini.end_time <= now)
		{
			hg_p_write_trace_ini(ent->d_name,
							 (ini.comments != NULL) ? ini.comments : "",
							 ini.owner, ini.start_time, ini.end_time,
							 ini.n_users, ini.users,
							 ini.n_databases, ini.databases,
							 ini.max_log_size_mb, ini.log_min_duration_sec,
							 HGP_STATUS_TIME_LIMIT,
							 now);
			hg_p_free_trace_ini(&ini);
			continue;
		}

		if (ini.max_log_size_mb > 0)
		{
			char		dir_path[MAXPGPATH];

			hg_p_path_trace_dir(dir_path, sizeof(dir_path), ent->d_name);
			disk_size = hg_p_dir_size(dir_path);
		}

		LWLockAcquire(hgp_shared->lock, LW_EXCLUSIVE);

		for (int i = 0; i < HGP_MAX_TRACES; i++)
		{
			if (!hgp_shared->traces[i].in_use)
			{
				slot_idx = i;
				break;
			}
		}

		if (slot_idx >= 0)
		{
			HgpTraceShm *t = &hgp_shared->traces[slot_idx];

			memset(t, 0, sizeof(HgpTraceShm));
			t->in_use = true;
			t->generation = ++hgp_shared->next_counter;
			strlcpy(t->trace_id, ent->d_name, HGP_TRACE_ID_BUFSZ);
			strlcpy(t->comments, (ini.comments != NULL) ? ini.comments : "", HGP_COMMENTS_BUFSZ);
			t->owner = ini.owner;
			t->start_time = ini.start_time;
			t->end_time = ini.end_time;
			t->max_log_size_mb = ini.max_log_size_mb;
			t->log_min_duration_sec = ini.log_min_duration_sec;
			t->n_users = ini.n_users;
			memcpy(t->users, ini.users, sizeof(Oid) * ini.n_users);
			t->n_databases = ini.n_databases;
			memcpy(t->databases, ini.databases, sizeof(Oid) * ini.n_databases);

			pg_atomic_init_u32(&t->running_queries, 0);
			pg_atomic_init_u64(&t->curr_log_size, disk_size);

			pg_atomic_fetch_add_u32(&hgp_shared->active_count, 1);
		}

		LWLockRelease(hgp_shared->lock);

		/*
		 * A trace that was ACTIVE on disk but could not be resumed after
		 * restart should be marked SERVER_RESTART.  Successfully restored
		 * traces remain ACTIVE on disk because they continue running.
		 */
		if (slot_idx < 0)
		{
			hg_p_write_trace_ini(ent->d_name,
							 (ini.comments != NULL) ? ini.comments : "",
							 ini.owner, ini.start_time, ini.end_time,
							 ini.n_users, ini.users,
							 ini.n_databases, ini.databases,
							 ini.max_log_size_mb, ini.log_min_duration_sec,
							 HGP_STATUS_SERVER_RESTART,
							 now);
		}

		hg_p_free_trace_ini(&ini);
	}

	FreeDir(dir);
}

static int
hg_p_match_active_traces(Oid user_id, Oid db_id, int *matched_idxs, int max_matches)
{
	int	nmatched = 0;
	TimestampTz now;

	if (hgp_shared == NULL || pg_atomic_read_u32(&hgp_shared->active_count) == 0)
		return 0;

	now = GetCurrentTimestamp();
	hg_p_prune_finished_traces(now);
	LWLockAcquire(hgp_shared->lock, LW_SHARED);

	for (int i = 0; i < HGP_MAX_TRACES; i++)
	{
		HgpTraceShm *t = &hgp_shared->traces[i];
		bool		user_match;
		bool		db_match;

		if (!t->in_use)
			continue;

		if (t->start_time != 0 && now < t->start_time)
			continue;

		if (t->end_time != 0 && now >= t->end_time)
			continue;

		if (t->max_log_size_mb > 0 &&
			pg_atomic_read_u64(&t->curr_log_size) >=
			((uint64) t->max_log_size_mb) * 1024UL * 1024UL)
			continue;

		user_match = (t->n_users == 0);
		if (!user_match)
		{
			for (int j = 0; j < t->n_users; j++)
				if (t->users[j] == user_id)
				{
					user_match = true;
					break;
				}
		}
		if (!user_match)
			continue;

		db_match = (t->n_databases == 0);
		if (!db_match)
		{
			for (int j = 0; j < t->n_databases; j++)
				if (t->databases[j] == db_id)
				{
					db_match = true;
					break;
				}
		}

		if (!db_match)
			continue;

		/*
		 * Only bump running_queries for slots we will actually report to
		 * the caller; bumping unconditionally and then dropping past
		 * max_matches would leak the counter.  Today max_matches is
		 * always HGP_MAX_TRACES so the early check never fires, but
		 * keeping the order right means future callers can pass a
		 * smaller cap safely.
		 */
		if (nmatched >= max_matches)
			break;
		pg_atomic_fetch_add_u32(&t->running_queries, 1);
		matched_idxs[nmatched++] = i;
	}

	LWLockRelease(hgp_shared->lock);
	return nmatched;
}

static void
hg_p_release_query_slot(int trace_idx)
{
	if (trace_idx < 0 || trace_idx >= HGP_MAX_TRACES)
		return;
	if (hgp_shared == NULL)
		return;
	pg_atomic_fetch_sub_u32(&hgp_shared->traces[trace_idx].running_queries, 1);
}

/*
 * Lazily evaluate per-trace limits.  Caller holds no lock.
 * Marks the slot's final_status (3=size, 4=time) and frees the slot when a
 * limit is breached, then rewrites trace.ini with the new status.
 */
static void
hg_p_check_limits(int trace_idx, TimestampTz now)
{
	HgpTraceShm *t;
	int		status_to_set = 0;
	bool		in_use;
	TimestampTz	end_time;
	int32		max_log_size_mb;

	if (trace_idx < 0 || trace_idx >= HGP_MAX_TRACES || hgp_shared == NULL)
		return;

	t = &hgp_shared->traces[trace_idx];

	/*
	 * Snapshot the slot's metadata under SHARED lock so we don't observe
	 * a torn 64-bit end_time on 32-bit platforms or race against
	 * hg_p_activate reusing the slot underneath us.  curr_log_size is
	 * atomic so reading it after we drop the lock is safe.
	 */
	LWLockAcquire(hgp_shared->lock, LW_SHARED);
	in_use = t->in_use;
	end_time = t->end_time;
	max_log_size_mb = t->max_log_size_mb;
	LWLockRelease(hgp_shared->lock);

	if (!in_use)
		return;

	if (end_time != 0 && now >= end_time)
		status_to_set = HGP_STATUS_TIME_LIMIT;
	else if (max_log_size_mb > 0 &&
			 pg_atomic_read_u64(&t->curr_log_size) >=
			 ((uint64) max_log_size_mb) * 1024UL * 1024UL)
		status_to_set = HGP_STATUS_MAX_SIZE;

	if (status_to_set != 0)
		hg_p_finalise_trace(trace_idx, status_to_set, now);
}

/*
 * Sweep active slots and finalise any trace that has already hit its
 * time-limit or size-limit.  This keeps trace.ini in sync even when the
 * trace expires between two matching queries.
 */
static void
hg_p_prune_finished_traces(TimestampTz now)
{
	int	candidates[HGP_MAX_TRACES];
	int	ncandidates = 0;

	if (hgp_shared == NULL ||
		pg_atomic_read_u32(&hgp_shared->active_count) == 0)
		return;

	LWLockAcquire(hgp_shared->lock, LW_SHARED);
	for (int i = 0; i < HGP_MAX_TRACES; i++)
	{
		HgpTraceShm *t = &hgp_shared->traces[i];

		if (!t->in_use)
			continue;

		if ((t->end_time != 0 && now >= t->end_time) ||
			(t->max_log_size_mb > 0 &&
			 pg_atomic_read_u64(&t->curr_log_size) >=
			 ((uint64) t->max_log_size_mb) * 1024UL * 1024UL))
			candidates[ncandidates++] = i;
	}
	LWLockRelease(hgp_shared->lock);

	for (int i = 0; i < ncandidates; i++)
		hg_p_check_limits(candidates[i], now);
}

/*
 * Free a shared-memory trace slot.  Returns true if the caller is the
 * one that actually freed it (the caller can then write trace.ini).
 *
 * The expected_gen argument guards against a slot that was finalised and
 * reused for a different trace between the time the caller snapshotted
 * its contents and the time it grabbed the exclusive lock.  Pass 0 to
 * skip the check (used for the cleanup-on-error path in hg_p_activate).
 *
 * We MUST bump generation here so any backend that still holds an open
 * file handle for this slot will detect the stale state at next use.
 */
static bool
hg_p_finalise_trace_in_shm_inner(int trace_idx, int status,
								 uint32 expected_gen,
								 char trace_id_out[HGP_TRACE_ID_BUFSZ])
{
	HgpTraceShm *t;
	bool		freed = false;

	t = &hgp_shared->traces[trace_idx];
	LWLockAcquire(hgp_shared->lock, LW_EXCLUSIVE);
	if (t->in_use && (expected_gen == 0 || t->generation == expected_gen))
	{
		memcpy(trace_id_out, t->trace_id, HGP_TRACE_ID_BUFSZ);
		t->final_status = status;
		t->in_use = false;
		t->generation++;
		pg_atomic_fetch_sub_u32(&hgp_shared->active_count, 1);
		freed = true;
	}
	LWLockRelease(hgp_shared->lock);
	return freed;
}

/*
 * Mark a trace finished and update its trace.ini on disk.
 */
static void
hg_p_finalise_trace(int trace_idx, int status, TimestampTz finish_time)
{
	HgpTraceShm	saved;
	uint32		saved_gen;
	char		trace_id[HGP_TRACE_ID_BUFSZ];

	if (trace_idx < 0 || trace_idx >= HGP_MAX_TRACES)
		return;

	/*
	 * Snapshot the slot under the lock so we can write trace.ini after we
	 * release it.  We capture the generation as well so we can refuse to
	 * tear down a slot that has been re-claimed by hg_p_activate after our
	 * lock release.
	 */
	LWLockAcquire(hgp_shared->lock, LW_SHARED);
	if (!hgp_shared->traces[trace_idx].in_use)
	{
		LWLockRelease(hgp_shared->lock);
		return;
	}
	saved = hgp_shared->traces[trace_idx];
	saved_gen = saved.generation;
	memcpy(trace_id, saved.trace_id, HGP_TRACE_ID_BUFSZ);
	LWLockRelease(hgp_shared->lock);

	if (!hg_p_finalise_trace_in_shm_inner(trace_idx, status, saved_gen, trace_id))
		return;					/* lost the race or slot was reused */

	hg_p_write_trace_ini(trace_id, saved.comments,
					   saved.owner, saved.start_time, saved.end_time,
					   saved.n_users, saved.users,
					   saved.n_databases, saved.databases,
					   saved.max_log_size_mb, saved.log_min_duration_sec,
					   status, finish_time);
}

/*
 * ExecutorStart hook
 */
static void
hg_p_ExecutorStart_hook(QueryDesc *queryDesc, int eflags)
{
	bool		want_track = false;
	int			trace_idxs[HGP_MAX_TRACES];
	int			ntrace_idxs = 0;

	if (hgp_enabled &&
		hgp_nesting_level == 0 &&
		!hgp_current_tracked &&
		!IsParallelWorker() &&
		(eflags & EXEC_FLAG_EXPLAIN_ONLY) == 0)
	{
		ntrace_idxs = hg_p_match_active_traces(GetUserId(), MyDatabaseId,
											 trace_idxs, HGP_MAX_TRACES);

		if (ntrace_idxs > 0)
		{
			want_track = true;
			queryDesc->instrument_options |= INSTRUMENT_TIMER;
			queryDesc->instrument_options |= INSTRUMENT_BUFFERS;
			queryDesc->instrument_options |= INSTRUMENT_ROWS;

			hgp_current_tracked = true;
			hgp_current_trace_count = ntrace_idxs;
			memcpy(hgp_current_trace_idxs, trace_idxs,
				   ntrace_idxs * sizeof(int));
			hgp_current_start_ts = GetCurrentTimestamp();
		}
	}

	if (prev_ExecutorStart)
		prev_ExecutorStart(queryDesc, eflags);
	else
		standard_ExecutorStart(queryDesc, eflags);

	/*
	 * Allocate the per-query timing accumulator (mirrors auto_explain) and
	 * snapshot the resource counters AFTER standard_ExecutorStart so the
	 * delta excludes plan-state initialisation costs.
	 */
	if (want_track)
	{
		if (queryDesc->totaltime == NULL)
		{
			MemoryContext oldcxt;

			oldcxt = MemoryContextSwitchTo(queryDesc->estate->es_query_cxt);
			queryDesc->totaltime = InstrAlloc(1, INSTRUMENT_ALL, false);
			MemoryContextSwitchTo(oldcxt);
		}

		/* system call to get the resource data*/
		getrusage(RUSAGE_SELF, &hgp_current_rusage_start);
		hgp_current_buf_start = pgBufferUsage;
	}
}

/*
 * ExecutorRun hook -- track nesting level so we only profile top-level queries
 */
static void
hg_p_ExecutorRun_hook(QueryDesc *queryDesc, ScanDirection direction,
					  uint64 count, bool execute_once)
{
	hgp_nesting_level++;
	PG_TRY();
	{
		if (prev_ExecutorRun)
			prev_ExecutorRun(queryDesc, direction, count, execute_once);
		else
			standard_ExecutorRun(queryDesc, direction, count, execute_once);
	}
	PG_FINALLY();
	{
		hgp_nesting_level--;
	}
	PG_END_TRY();
}

/*
 * ExecutorEnd hook -- emit one CSV row if the query was tracked
 */
static void
hg_p_ExecutorEnd_hook(QueryDesc *queryDesc)
{

	if (hgp_nesting_level == 0 && hgp_current_tracked)
	{
		int			trace_idxs[HGP_MAX_TRACES];
		int			ntrace_idxs = hgp_current_trace_count;

		/*
		 * Reset bookkeeping early so any unexpected re-entry sees a clean
		 * state.  Capture the trace indexes in locals first. 
		 */
		memcpy(trace_idxs, hgp_current_trace_idxs, ntrace_idxs * sizeof(int));
		hgp_current_tracked = false;
		hgp_current_trace_count = 0;

		if (queryDesc->totaltime != NULL && ntrace_idxs > 0)
		{
			struct rusage ru_end;
			BufferUsage buf_end;
			double		duration_ms;

			InstrEndLoop(queryDesc->totaltime);
			duration_ms = queryDesc->totaltime->total * 1000.0;
			getrusage(RUSAGE_SELF, &ru_end);
			buf_end = pgBufferUsage;

			for (int i = 0; i < ntrace_idxs; i++)
			{
				HgpTraceShm *t = &hgp_shared->traces[trace_idxs[i]];
				int			min_dur_sec = t->log_min_duration_sec;

				if (min_dur_sec < 0 || duration_ms >= (double) min_dur_sec * 1000.0)
				{
					hg_p_emit_query_sample(trace_idxs[i], queryDesc, duration_ms,
										 &ru_end, &buf_end);
				}
			}
		}

		for (int i = 0; i < ntrace_idxs; i++)
		{
			hg_p_release_query_slot(trace_idxs[i]);
			hg_p_check_limits(trace_idxs[i], GetCurrentTimestamp());
		}
	}

	if (prev_ExecutorEnd)
		prev_ExecutorEnd(queryDesc);
	else
		standard_ExecutorEnd(queryDesc);
}

/*
 * Build the EXPLAIN output for a finished query.  Runs in the executor's
 * per-query context and returns a palloc'd C string.
 */
static char *
hg_p_format_explain(QueryDesc *queryDesc)
{
	ExplainState *es;
	char	   *result;

	es = NewExplainState();
	es->analyze = (queryDesc->instrument_options & INSTRUMENT_TIMER) != 0;
	es->wal = false;
	es->timing = es->analyze;
	es->summary = es->analyze;
	es->buffers = es->analyze;
	es->format = EXPLAIN_FORMAT_TEXT;

	ExplainBeginOutput(es);
	ExplainQueryText(es, queryDesc);
	ExplainPrintPlan(es, queryDesc);
	ExplainEndOutput(es);

	if (es->str->len > 0 && es->str->data[es->str->len - 1] == '\n')
	{
		es->str->data[--es->str->len] = '\0';
	}

	result = pstrdup(es->str->data);
	pfree(es->str->data);
	pfree(es->str);
	pfree(es);
	return result;
}

/*
 * A trace_id is generated by hg_p_activate as exactly HGP_TRACE_ID_LEN
 * decimal digits: 14 chars of YYYYMMDDhhmmss followed by a 6-digit
 * counter.
 */
static bool
hg_p_is_valid_trace_id(const char *trace_id)
{
	if (trace_id == NULL)
		return false;
	if (strlen(trace_id) != HGP_TRACE_ID_LEN)
		return false;
	for (int i = 0; i < HGP_TRACE_ID_LEN; i++)
	{
		unsigned char c = (unsigned char) trace_id[i];

		if (c < '0' || c > '9')
			return false;
	}
	return true;
}

static void
hg_p_path_root(char *buf, size_t bufsz)
{
	if (DataDir != NULL)
		snprintf(buf, bufsz, "%s/%s", DataDir, HGP_ROOT_DIR);
	else
		snprintf(buf, bufsz, "%s", HGP_ROOT_DIR);
}

static void
hg_p_path_trace_dir(char *buf, size_t bufsz, const char *trace_id)
{
	if (DataDir != NULL)
		snprintf(buf, bufsz, "%s/%s/%s", DataDir, HGP_ROOT_DIR, trace_id);
	else
		snprintf(buf, bufsz, "%s/%s", HGP_ROOT_DIR, trace_id);
}

static void
hg_p_path_trace_ini(char *buf, size_t bufsz, const char *trace_id)
{
	if (DataDir != NULL)
		snprintf(buf, bufsz, "%s/%s/%s/%s",
				 DataDir, HGP_ROOT_DIR, trace_id, HGP_INI_FILE);
	else
		snprintf(buf, bufsz, "%s/%s/%s",
				 HGP_ROOT_DIR, trace_id, HGP_INI_FILE);
}

static void
hg_p_path_trace_csv(char *buf, size_t bufsz, const char *trace_id, int pid)
{
	if (DataDir != NULL)
		snprintf(buf, bufsz, "%s/%s/%s/%s%d%s",
				 DataDir, HGP_ROOT_DIR, trace_id,
				 HGP_CSV_PREFIX, pid, HGP_CSV_SUFFIX);
	else
		snprintf(buf, bufsz, "%s/%s/%s%d%s",
				 HGP_ROOT_DIR, trace_id,
				 HGP_CSV_PREFIX, pid, HGP_CSV_SUFFIX);
}

/*
 * Ensure $PGDATA/hg_profiler exists.
 */
static void
hg_p_make_trace_root(void)
{
	char		path[MAXPGPATH];

	hg_p_path_root(path, sizeof(path));
	if (MakePGDirectory(path) != 0 && errno != EEXIST)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("hg_profiler: could not create directory \"%s\": %m",
						path)));
}

/*
 * Ensure $PGDATA/hg_profiler/<trace_id> exists.
 * Errors if the directory is already there.
 */
static void
hg_p_make_trace_dir(const char *trace_id)
{
	char		path[MAXPGPATH];

	hg_p_path_trace_dir(path, sizeof(path), trace_id);
	if (MakePGDirectory(path) != 0)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("hg_profiler: could not create directory \"%s\": %m",
						path)));
}

static uint64
hg_p_dir_size(const char *path)
{
	uint64		dirsize = 0;
	DIR		   *dirdesc;
	struct dirent *direntry;
	char		filename[MAXPGPATH * 2];

	dirdesc = AllocateDir(path);
	if (dirdesc == NULL)
	{
		if (errno == ENOENT)
			return 0;
		ereport(WARNING,
				(errcode_for_file_access(),
				 errmsg("hg_profiler: could not open directory \"%s\": %m",
						path)));
		return (uint64) -1;
	}

	while ((direntry = ReadDir(dirdesc, path)) != NULL)
	{
		struct stat fst;

		CHECK_FOR_INTERRUPTS();

		if (strcmp(direntry->d_name, ".") == 0 ||
			strcmp(direntry->d_name, "..") == 0)
			continue;

		snprintf(filename, sizeof(filename), "%s/%s", path, direntry->d_name);
		if (stat(filename, &fst) < 0)
		{
			if (errno == ENOENT)
				continue;
			ereport(WARNING,
					(errcode_for_file_access(),
					 errmsg("hg_profiler: could not stat file \"%s\": %m",
							filename)));
			FreeDir(dirdesc);
			return (uint64) -1;
		}

		if (S_ISREG(fst.st_mode))
			dirsize += (uint64) fst.st_size;
	}

	FreeDir(dirdesc);
	return dirsize;
}

static bool
hg_p_parse_trace_csv_filename(const char *name, int *pid_out)
{
	size_t		nlen;
	size_t		prefix_len = strlen(HGP_CSV_PREFIX);
	size_t		suffix_len = strlen(HGP_CSV_SUFFIX);
	const char *digits;
	size_t		ndigits;
	long		pid;

	if (name == NULL || pid_out == NULL)
		return false;

	nlen = strlen(name);
	if (nlen <= prefix_len + suffix_len)
		return false;
	if (strncmp(name, HGP_CSV_PREFIX, prefix_len) != 0)
		return false;
	if (strcmp(name + nlen - suffix_len, HGP_CSV_SUFFIX) != 0)
		return false;

	digits = name + prefix_len;
	ndigits = nlen - prefix_len - suffix_len;
	if (ndigits == 0)
		return false;
	for (size_t i = 0; i < ndigits; i++)
	{
		unsigned char c = (unsigned char) digits[i];

		if (c < '0' || c > '9')
			return false;
	}

	errno = 0;
	pid = strtol(digits, NULL, 10);
	if (errno != 0 || pid <= 0 || pid > INT_MAX)
		return false;

	*pid_out = (int) pid;
	return true;
}

/*
 * String-quoting helpers
 *
 * hg_p_csv_append_quoted writes src into dst as a CSV-escaped, double-quoted
 * field.  Embedded double quotes are doubled (RFC 4180).  Embedded
 * newlines are preserved so multi-line EXPLAIN plans keep their original
 * formatting when loaded back into _hg_tmp_tbl_p_profiler.
 */
static void
hg_p_csv_append_quoted(StringInfo dst, const char *src)
{
	const char *chunk_start;
	const char *p;

	appendStringInfoChar(dst, '"');

	if (src == NULL)
	{
		appendStringInfoChar(dst, '"');
		return;
	}

	/*
	 * Scan for the next double-quote character.  Everything between
	 * chunk_start and p is a run of plain characters that can be
	 * appended in bulk, which is much faster than character-by-character
	 * for long strings like EXPLAIN output.
	 */
	chunk_start = src;
	p = src;
	while (*p)
	{
		if (*p == '"')
		{
			/* Flush the plain-character run before the quote. */
			if (p > chunk_start)
				appendBinaryStringInfo(dst, chunk_start, (int) (p - chunk_start));
			appendStringInfoString(dst, "\"\"");
			p++;
			chunk_start = p;
		}
		else
			p++;
	}

	/* Flush any remaining plain characters. */
	if (p > chunk_start)
		appendBinaryStringInfo(dst, chunk_start, (int) (p - chunk_start));

	appendStringInfoChar(dst, '"');
}

/*
 * Per-backend file-handle cache
 */
static FILE *
hg_p_get_local_file(int slot)
{
	HgpLocalFile *lf;
	HgpTraceShm *t;
	uint32		gen;
	char		trace_id[HGP_TRACE_ID_BUFSZ];
	char		path[MAXPGPATH];

	if (slot < 0 || slot >= HGP_MAX_TRACES || hgp_shared == NULL)
		return NULL;

	if (!hgp_atexit_registered)
	{
		before_shmem_exit(hg_p_before_shmem_exit, (Datum) 0);
		hgp_atexit_registered = true;
	}

	t = &hgp_shared->traces[slot];

	/*
	 * Snapshot the current generation/trace_id under the lock.  We don't
	 * need an exclusive lock; the slot may flip in_use=false at any time
	 * but the generation counter advances monotonically so a stale read
	 * is safe.
	 */
	LWLockAcquire(hgp_shared->lock, LW_SHARED);
	if (!t->in_use)
	{
		LWLockRelease(hgp_shared->lock);
		hg_p_close_local_file(slot);
		return NULL;
	}
	gen = t->generation;
	memcpy(trace_id, t->trace_id, HGP_TRACE_ID_BUFSZ);
	LWLockRelease(hgp_shared->lock);

	lf = &hgp_local_files[slot];
	if (lf->fp != NULL)
	{
		if (lf->generation == gen &&
			strcmp(lf->trace_id, trace_id) == 0)
			return lf->fp;
		hg_p_close_local_file(slot);
	}

	/* Open afresh. */
	hg_p_path_trace_csv(path, sizeof(path), trace_id, MyProcPid);
	lf->fp = AllocateFile(path, "a");
	if (lf->fp == NULL)
	{
		ereport(WARNING,
				(errcode_for_file_access(),
				 errmsg("hg_profiler: could not open trace file \"%s\": %m",
						path)));
		return NULL;
	}
	setvbuf(lf->fp, NULL, _IOFBF, HGP_CSV_IOBUF_SIZE);
	memcpy(lf->trace_id, trace_id, HGP_TRACE_ID_BUFSZ);
	lf->generation = gen;
	return lf->fp;
}

static void
hg_p_close_local_file(int slot)
{
	HgpLocalFile *lf;

	if (slot < 0 || slot >= HGP_MAX_TRACES)
		return;
	lf = &hgp_local_files[slot];
	if (lf->fp != NULL)
	{
		(void) FreeFile(lf->fp);
		lf->fp = NULL;
	}
	lf->generation = 0;
	lf->trace_id[0] = '\0';
}

static void
hg_p_close_all_local_files(void)
{
	for (int i = 0; i < HGP_MAX_TRACES; i++)
		hg_p_close_local_file(i);
}

/*
 * Persist one captured query as a CSV row.
 */
static void
hg_p_emit_query_sample(int trace_idx,
					   QueryDesc *queryDesc,
					   double duration_ms,
					   const struct rusage *ru_end,
					   const BufferUsage *buf_end)
{
	HgpTraceShm *t;
	StringInfoData csv;
	const char *appname;
	char	   *explain_text = NULL;
	int64		rows;
	const char *start_str;
	const char *end_str;
	const struct rusage *ru0 = &hgp_current_rusage_start;
	const BufferUsage *buf0 = &hgp_current_buf_start;
	FILE	   *fp;
	size_t		written;
	TimestampTz now;
	int32		max_log_size_mb;
	char		trace_id[HGP_TRACE_ID_BUFSZ];

	if (trace_idx < 0 || trace_idx >= HGP_MAX_TRACES)
		return;
	t = &hgp_shared->traces[trace_idx];

	now = GetCurrentTimestamp();

	LWLockAcquire(hgp_shared->lock, LW_SHARED);
	if (!t->in_use)
	{
		LWLockRelease(hgp_shared->lock);
		return;
	}
	max_log_size_mb = t->max_log_size_mb;
	memcpy(trace_id, t->trace_id, HGP_TRACE_ID_BUFSZ);
	LWLockRelease(hgp_shared->lock);

	if (max_log_size_mb > 0)
	{
		uint64		limit_bytes = ((uint64) max_log_size_mb) * 1024UL * 1024UL;
		char		dir_path[MAXPGPATH];
		uint64		dir_size;

		hg_p_path_trace_dir(dir_path, sizeof(dir_path), trace_id);
		dir_size = hg_p_dir_size(dir_path);

		if (dir_size == (uint64) -1 || dir_size >= limit_bytes)
		{
			hg_p_finalise_trace(trace_idx, HGP_STATUS_MAX_SIZE, now);
			return;
		}
	}

	start_str = timestamptz_to_str(hgp_current_start_ts);

	/* timestamptz_to_str uses a static buffer, so we have to copy out. */
	{
		static char start_buf[MAXDATELEN + 1];

		strlcpy(start_buf, start_str, sizeof(start_buf));
		start_str = start_buf;
	}
	end_str = timestamptz_to_str(now);

	initStringInfo(&csv);

	/* 1, 2: start/end timestamps (the file format keeps them unquoted). */
	appendStringInfo(&csv, "%s,%s,", start_str, end_str);

	/* 3: appl_name */
	appname = application_name;
	hg_p_csv_append_quoted(&csv, (appname == NULL) ? "" : appname);
	appendStringInfoChar(&csv, ',');

	/* 4, 5: user_id, db_id */
	appendStringInfo(&csv, "%u,%u,",
					 (unsigned) GetUserId(),
					 (unsigned) MyDatabaseId);

	/* 6: query_type */
	appendStringInfo(&csv, "%d,", (int) queryDesc->operation);

	/* 7: query text -- field order follows the CSV layout, except
	 * that we intentionally do not emit query_id. */
	hg_p_csv_append_quoted(&csv, queryDesc->sourceText);
	appendStringInfoChar(&csv, ',');

	/* 8: explain output */
	explain_text = hg_p_format_explain(queryDesc);
	hg_p_csv_append_quoted(&csv, explain_text);
	appendStringInfoChar(&csv, ',');

	/* 9: duration (ms) */
	appendStringInfo(&csv, "%.6f,", duration_ms);

	/* 10: rows_updated */
	rows = (queryDesc->estate != NULL) ?
		(int64) queryDesc->estate->es_processed : 0;
	appendStringInfo(&csv, INT64_FORMAT ",", rows);

	/* 11-20: getrusage(RUSAGE_SELF) deltas */
	appendStringInfo(&csv, INT64_FORMAT ",", HGP_RU_DELTA(ru_end, ru0, ru_inblock));
	appendStringInfo(&csv, INT64_FORMAT ",", HGP_RU_DELTA(ru_end, ru0, ru_oublock));
	appendStringInfo(&csv, INT64_FORMAT ",", HGP_RU_DELTA(ru_end, ru0, ru_majflt));
	appendStringInfo(&csv, INT64_FORMAT ",", HGP_RU_DELTA(ru_end, ru0, ru_minflt));
	appendStringInfo(&csv, INT64_FORMAT ",", HGP_RU_DELTA(ru_end, ru0, ru_nswap));
	appendStringInfo(&csv, INT64_FORMAT ",", HGP_RU_DELTA(ru_end, ru0, ru_nsignals));
	appendStringInfo(&csv, INT64_FORMAT ",", HGP_RU_DELTA(ru_end, ru0, ru_msgrcv));
	appendStringInfo(&csv, INT64_FORMAT ",", HGP_RU_DELTA(ru_end, ru0, ru_msgsnd));
	appendStringInfo(&csv, INT64_FORMAT ",", HGP_RU_DELTA(ru_end, ru0, ru_nvcsw));
	appendStringInfo(&csv, INT64_FORMAT ",", HGP_RU_DELTA(ru_end, ru0, ru_nivcsw));

	/* 21-28: pgBufferUsage deltas */
	appendStringInfo(&csv, INT64_FORMAT ",",
					 (int64) (buf_end->shared_blks_read    - buf0->shared_blks_read));
	appendStringInfo(&csv, INT64_FORMAT ",",
					 (int64) (buf_end->shared_blks_written - buf0->shared_blks_written));
	appendStringInfo(&csv, INT64_FORMAT ",",
					 (int64) (buf_end->shared_blks_hit     - buf0->shared_blks_hit));
	appendStringInfo(&csv, INT64_FORMAT ",",
					 (int64) (buf_end->local_blks_read     - buf0->local_blks_read));
	appendStringInfo(&csv, INT64_FORMAT ",",
					 (int64) (buf_end->local_blks_written  - buf0->local_blks_written));
	appendStringInfo(&csv, INT64_FORMAT ",",
					 (int64) (buf_end->local_blks_hit      - buf0->local_blks_hit));
	appendStringInfo(&csv, INT64_FORMAT ",",
					 (int64) (buf_end->temp_blks_read      - buf0->temp_blks_read));
	appendStringInfo(&csv, INT64_FORMAT ",",
					 (int64) (buf_end->temp_blks_written   - buf0->temp_blks_written));

	/* 29: pid */
	appendStringInfo(&csv, "%d\n", (int) MyProcPid);

	fp = hg_p_get_local_file(trace_idx);

	if (fp == NULL)
	{
		pfree(csv.data);
		if (explain_text != NULL)
			pfree(explain_text);
		return;
	}

	written = fwrite(csv.data, 1, (size_t) csv.len, fp);
	if (written != (size_t) csv.len)
	{
		ereport(WARNING,
				(errcode_for_file_access(),
				 errmsg("hg_profiler: short write to trace CSV (%zu of %d): %m",
						written, csv.len)));
		hg_p_close_local_file(trace_idx);
	}
	else
	{
		/*
		 * Account for the row in curr_log_size.  This includes only the
		 * bytes we just appended, not pre-existing file size.
		 */
		pg_atomic_fetch_add_u64(&t->curr_log_size, (uint64) csv.len);

		/*
		 * Flush so that data is visible to hg_p_load_trace() which may
		 * be called from another backend while this trace is still
		 * active.  The 64 KB IO buffer means we only actually hit the
		 * OS when the buffer is full or on this explicit flush.
		 */
		fflush(fp);
	}

	pfree(csv.data);
	if (explain_text != NULL)
		pfree(explain_text);

	/*
	 * Intentionally do NOT close the cached file handle here: the whole
	 * point of hgp_local_files[] is to amortise the open() cost across
	 * many writes from the same backend.  fflush() above already pushes
	 * the row into the OS buffer so concurrent hg_p_load_trace() readers
	 * can see it.  The handle is closed when:
	 *
	 *   - the slot's generation/trace_id moves under our feet (detected
	 *     by hg_p_get_local_file at next call),
	 *   - the transaction aborts mid-query (xact callback), or
	 *   - the backend exits (before_shmem_exit).
	 */
}

/*
 * trace.ini I/O
 */

/*
 * Append KeyName=value with backslash-escaped value, but values are
 * straightforward CSV-style quoted (we write Comments="..." quoted, the
 * rest as plain value to match the file format)
 */
static void
hg_p_ini_emit_quoted(StringInfo dst, const char *key, const char *val)
{
	appendStringInfo(dst, "%s=", key);
	hg_p_csv_append_quoted(dst, val);
	appendStringInfoChar(dst, '\n');
}

static void
hg_p_ini_emit_oid_list(StringInfo dst, const char *key,
					 const Oid *oids, int n)
{
	appendStringInfo(dst, "%s=", key);
	for (int i = 0; i < n; i++)
		appendStringInfo(dst, "%s%u", (i == 0) ? "" : ",",
						 (unsigned) oids[i]);
	appendStringInfoChar(dst, '\n');
}

/*
 * Format a TimestampTz using timestamptz_to_str, copying the result into
 * a caller-provided buffer (timestamptz_to_str's static buffer would
 * otherwise be clobbered by subsequent calls).
 */
static void
hg_p_format_timestamptz(char *buf, size_t bufsz, TimestampTz ts)
{
	const char *s = timestamptz_to_str(ts);

	strlcpy(buf, s, bufsz);
}

/*
 * Atomically write trace.ini and make the change crash-safe.
 *
 * Sequence (mirrors how PostgreSQL persists pg_control etc.):
 *
 *   1. Build the full file body in memory.
 *   2. Write it to "<trace.ini>.tmp" and fclose() to flush stdio buffers.
 *   3. durable_rename() fsync's the temp file, atomically renames it
 *      onto trace.ini, and fsync's the parent directory so the rename
 *      itself survives a crash.
 *
 * This way, after a crash the on-disk trace.ini is either the previous
 * contents or the new contents -- never a half-written file.  Without
 * the temp-file + rename dance, a crash mid-fwrite could leave a
 * truncated trace.ini that hg_p_read_trace_ini would silently skip,
 * "losing" the most recent state transition (e.g. STOPPED / TIME_LIMIT).
 *
 * Cost: one extra fsync per state transition (activate, deactivate,
 * finalise, startup-restore).  Per-row CSV writes are NOT affected --
 * those go through hg_p_emit_query_sample, which only fflush()es.
 */
static void
hg_p_write_trace_ini(const char *trace_id,
					 const char *comments,
					 Oid owner,
					 TimestampTz start_time,
					 TimestampTz end_time,
					 int n_users, const Oid *users,
					 int n_dbs, const Oid *dbs,
					 int32 max_log_size_mb,
					 int32 log_min_duration_sec,
					 int status,
					 TimestampTz finish_time)
{
	StringInfoData buf;
	char		path[MAXPGPATH];
	char		tmp_path[MAXPGPATH];
	char		ts_buf[MAXDATELEN + 1];
	FILE	   *fp;
	size_t		written;
	int			save_errno;

	initStringInfo(&buf);

	appendStringInfo(&buf, "Version=%.1f\n", HGP_PROFILER_VERSION);
	appendStringInfo(&buf, "TraceId=%s\n", trace_id);
	hg_p_ini_emit_quoted(&buf, "Comments", (comments != NULL) ? comments : "");
	appendStringInfo(&buf, "Owner=%u\n", (unsigned) owner);

	hg_p_format_timestamptz(ts_buf, sizeof(ts_buf), start_time);
	appendStringInfo(&buf, "StartTime=%s\n", ts_buf);

	if (end_time != 0)
	{
		hg_p_format_timestamptz(ts_buf, sizeof(ts_buf), end_time);
		appendStringInfo(&buf, "EndTime=%s\n", ts_buf);
	}

	appendStringInfo(&buf, "NUsers=%d\n", n_users);
	hg_p_ini_emit_oid_list(&buf, "Users", users, n_users);
	appendStringInfo(&buf, "NDatabases=%d\n", n_dbs);
	hg_p_ini_emit_oid_list(&buf, "Databases", dbs, n_dbs);
	appendStringInfo(&buf, "MaxLogLimit=%d\n", max_log_size_mb);
	appendStringInfo(&buf, "LogMinDuration=%d\n", log_min_duration_sec);
	appendStringInfo(&buf, "Status=%d\n", status);

	if (finish_time != 0)
	{
		hg_p_format_timestamptz(ts_buf, sizeof(ts_buf), finish_time);
		appendStringInfo(&buf, "FinishTime=%s\n", ts_buf);
	}

	hg_p_path_trace_ini(path, sizeof(path), trace_id);
	if (snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", path) >=
		(int) sizeof(tmp_path))
	{
		pfree(buf.data);
		ereport(ERROR,
				(errcode(ERRCODE_NAME_TOO_LONG),
				 errmsg("hg_profiler: trace.ini path too long: \"%s\"",
						path)));
	}

	fp = AllocateFile(tmp_path, "w");
	if (fp == NULL)
	{
		pfree(buf.data);
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("hg_profiler: could not open \"%s\" for writing: %m",
						tmp_path)));
	}

	written = fwrite(buf.data, 1, (size_t) buf.len, fp);
	if (written != (size_t) buf.len)
	{
		save_errno = errno ? errno : EIO;
		(void) FreeFile(fp);
		(void) unlink(tmp_path);
		pfree(buf.data);
		errno = save_errno;
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("hg_profiler: short write to \"%s\": %m", tmp_path)));
	}

	/*
	 * fclose() flushes stdio buffers into the OS.  Capture its result --
	 * if it fails the temp file's contents are unreliable, so we must
	 * not promote it to trace.ini.
	 */
	if (FreeFile(fp) != 0)
	{
		save_errno = errno;
		(void) unlink(tmp_path);
		pfree(buf.data);
		errno = save_errno;
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("hg_profiler: could not close \"%s\": %m", tmp_path)));
	}

	pfree(buf.data);

	/*
	 * Now make the new contents durable: durable_rename() fsync's the
	 * tmp file, renames it onto the live path, then fsync's the parent
	 * directory so the rename itself survives a crash.  ERROR-elevel
	 * means any failure jumps out -- the .tmp file may or may not still
	 * be there, but trace.ini retains its prior (consistent) contents.
	 */
	durable_rename(tmp_path, path, ERROR);
}

/*
 * trace.ini parser (struct defined near the top of the file).
 *
 * The .ini file uses a single Key=Value pair per line, with the Comments
 * value double-quoted in CSV style.
 */

/*
 * Strip a single layer of CSV-style quoting from value.  Returns a freshly
 * palloc'd string.
 *
 * Rules (mirror what hg_p_csv_append_quoted writes):
 *   - NULL or empty input     -> ""
 *   - "...."                  -> properly unquoted, "" -> "
 *   - bare value (no quotes)  -> verbatim copy
 *   - leading " but no
 *     matching trailing " (truncated trace.ini, manual edit, ...) ->
 *     accept what we have, treating the leading/trailing " as the quoting
 *     boundary.  Better than silently embedding stray quotes.
 */
static char *
hg_p_unquote_csv(const char *value)
{
	size_t		len;
	size_t		end;
	StringInfoData out;

	if (value == NULL)
		return pstrdup("");

	len = strlen(value);
	if (len == 0 || value[0] != '"')
		return pstrdup(value);

	/*
	 * Walk the string character-by-character so we know where the matching
	 * closing quote is.  Doubled quotes ("") are escaped literals and do
	 * not terminate the field.
	 */
	end = len;					/* default: no closing quote found */
	for (size_t i = 1; i < len; i++)
	{
		if (value[i] == '"')
		{
			if (i + 1 < len && value[i + 1] == '"')
			{
				i++;			/* skip escaped quote */
				continue;
			}
			end = i;			/* matched closing quote */
			break;
		}
	}

	initStringInfo(&out);
	for (size_t i = 1; i < end; i++)
	{
		char		c = value[i];

		if (c == '"' && i + 1 < end && value[i + 1] == '"')
		{
			appendStringInfoChar(&out, '"');
			i++;
		}
		else
			appendStringInfoChar(&out, c);
	}
	return out.data;
}

static void
hg_p_parse_oid_list(const char *value, Oid *out, int *n_out, int max_n)
{
	const char *p = value;
	int			n = 0;

	*n_out = 0;
	if (value == NULL)
		return;
	while (*p)
	{
		char	   *endp;
		unsigned long v;

		while (*p == ' ' || *p == '\t' || *p == ',')
			p++;
		if (*p == '\0')
			break;
		errno = 0;
		v = strtoul(p, &endp, 10);
		if (errno != 0 || endp == p)
			break;
		if (n < max_n)
			out[n++] = (Oid) v;
		p = endp;
	}
	*n_out = n;
}

static TimestampTz
hg_p_parse_timestamptz(const char *s)
{
	Datum		d;

	if (s == NULL || *s == '\0')
		return 0;

	d = DirectFunctionCall3(timestamptz_in,
							CStringGetDatum(s),
							ObjectIdGetDatum(InvalidOid),
							Int32GetDatum(-1));
	return DatumGetTimestampTz(d);
}

/*
 * Read an entire trace.ini file and populate ini.  Returns true on
 * success, false if the file does not exist; raises ERROR on any other
 * I/O or parse failure.
 */
static bool
hg_p_read_trace_ini(const char *trace_id, HgpTraceIni *ini)
{
	char		path[MAXPGPATH];
	FILE	   *fp;
	char		linebuf[4096];

	memset(ini, 0, sizeof(*ini));
	ini->comments = pstrdup("");
	ini->max_log_size_mb = -1;

	hg_p_path_trace_ini(path, sizeof(path), trace_id);
	fp = AllocateFile(path, "r");
	if (fp == NULL)
	{
		if (errno == ENOENT)
			return false;
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("hg_profiler: could not open \"%s\": %m", path)));
	}

	while (fgets(linebuf, sizeof(linebuf), fp) != NULL)
	{
		char	   *eq;
		char	   *key;
		char	   *value;
		size_t		llen;

		llen = strlen(linebuf);
		while (llen > 0 && (linebuf[llen - 1] == '\n' ||
							linebuf[llen - 1] == '\r'))
			linebuf[--llen] = '\0';
		if (llen == 0)
			continue;

		eq = strchr(linebuf, '=');
		if (eq == NULL)
			continue;
		*eq = '\0';
		key = linebuf;
		value = eq + 1;

		if (strcmp(key, "Version") == 0)
			ini->version = atof(value);
		else if (strcmp(key, "Status") == 0)
			ini->status = atoi(value);
		else if (strcmp(key, "Comments") == 0)
		{
			pfree(ini->comments);
			ini->comments = hg_p_unquote_csv(value);
		}
		else if (strcmp(key, "StartTime") == 0)
			ini->start_time = hg_p_parse_timestamptz(value);
		else if (strcmp(key, "Owner") == 0)
			ini->owner = (Oid) strtoul(value, NULL, 10);
		else if (strcmp(key, "NUsers") == 0)
			ini->n_users = Min(Max(atoi(value), 0), HGP_MAX_FILTER_OIDS);
		else if (strcmp(key, "Users") == 0)
			hg_p_parse_oid_list(value, ini->users, &ini->n_users, HGP_MAX_FILTER_OIDS);
		else if (strcmp(key, "NDatabases") == 0)
			ini->n_databases = Min(Max(atoi(value), 0), HGP_MAX_FILTER_OIDS);
		else if (strcmp(key, "Databases") == 0)
			hg_p_parse_oid_list(value, ini->databases, &ini->n_databases, HGP_MAX_FILTER_OIDS);
		else if (strcmp(key, "MaxLogLimit") == 0)
			ini->max_log_size_mb = (int32) strtol(value, NULL, 10);
		else if (strcmp(key, "LogMinDuration") == 0)
			ini->log_min_duration_sec = (int32) strtol(value, NULL, 10);
		else if (strcmp(key, "EndTime") == 0)
		{
			ini->end_time = hg_p_parse_timestamptz(value);
			ini->end_time_set = true;
		}
		else if (strcmp(key, "FinishTime") == 0)
		{
			ini->finish_time = hg_p_parse_timestamptz(value);
			ini->finish_time_set = true;
		}
	}

	(void) FreeFile(fp);

	/*
	 * Validate Version against the format we know how to read.  We accept
	 * 0 (Version line missing -- treat as "ancient" but parseable) or
	 * exactly HGP_PROFILER_VERSION (4.0).  A different value means the
	 * file was written by an incompatible profiler build -- skip it
	 * rather than silently mis-interpret the field layout.
	 */
	if (ini->version != 0.0 && ini->version != HGP_PROFILER_VERSION)
	{
		ereport(WARNING,
				(errmsg("hg_profiler: ignoring trace \"%s\": "
						"on-disk Version=%.1f, expected %.1f",
						trace_id, ini->version,
						(double) HGP_PROFILER_VERSION)));
		hg_p_free_trace_ini(ini);
		/*
		 * Leave ini->comments == NULL; hg_p_free_trace_ini is a no-op on
		 * NULL so callers that defensively call it again will still be
		 * happy.
		 */
		return false;
	}

	return true;
}

static void
hg_p_free_trace_ini(HgpTraceIni *ini)
{
	if (ini->comments != NULL)
	{
		pfree(ini->comments);
		ini->comments = NULL;
	}
}

/*
 * helper: parse an oidvector arg (NULL-friendly).
 */
static int
hg_p_extract_oidvector(oidvector *vec, Oid *out, int max_n,
					   const char *fieldname)
{
	int	n;

	if (vec == NULL)
		return 0;
	n = vec->dim1;
	if (n == 0)
		return 0;
	if (n > max_n)
		ereport(ERROR,
				(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
				 errmsg("hg_profiler: too many %s OIDs in filter "
						"(got %d, max %d)", fieldname, n, max_n)));
	for (int i = 0; i < n; i++)
	{
		out[i] = vec->values[i];
		if (!OidIsValid(out[i]))
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("hg_profiler: %s OID at position %d must be > 0",
							fieldname, i + 1)));
	}
	return n;
}

/*
 * Resolve optional users/databases filters and enforce caller privileges.
 */
static void
hg_p_resolve_activate_filters(oidvector *users_arg, oidvector *dbs_arg,
							  Oid *users_buf, int *n_users,
							  Oid *dbs_buf, int *n_dbs)
{
	*n_users = hg_p_extract_oidvector(users_arg, users_buf,
									  HGP_MAX_FILTER_OIDS, "users");
	*n_dbs = hg_p_extract_oidvector(dbs_arg, dbs_buf,
									HGP_MAX_FILTER_OIDS, "databases");

	/* Both unset: always current user + current database (any role). */
	if (*n_users == 0 && *n_dbs == 0)
	{
		users_buf[0] = GetUserId();
		*n_users = 1;
		dbs_buf[0] = MyDatabaseId;
		*n_dbs = 1;
		return;
	}

	if (*n_dbs == 0)
	{
		dbs_buf[0] = MyDatabaseId;
		*n_dbs = 1;
	}

	if (!superuser() && *n_users > 0)
	{
		Oid			me = GetUserId();

		for (int i = 0; i < *n_users; i++)
		{
			if (users_buf[i] != me)
				ereport(ERROR,
						(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
						 errmsg("non-superuser %u may only trace its own role, "
								"not role %u", me, users_buf[i])));
		}
	}
}

/*
 * hg_p_activate(comments, users oidvector, databases oidvector,
 *             max_log_limit_MB integer, log_min_duration_sec integer,
 *             time_limit interval)  RETURNS text
 */
Datum
hg_p_activate(PG_FUNCTION_ARGS)
{
	text	   *comments_arg;
	oidvector  *users_arg;
	oidvector  *dbs_arg;
	int32	max_log_limit_mb;
	int32	log_min_duration_sec;
	Interval   *time_limit_arg = NULL;
	Interval   default_time_limit;

	Oid	users_buf[HGP_MAX_FILTER_OIDS];
	Oid	dbs_buf[HGP_MAX_FILTER_OIDS];
	int	n_users;
	int	n_dbs;

	TimestampTz now;
	TimestampTz scheduled_end = 0;
	char	trace_id[HGP_TRACE_ID_BUFSZ];
	int	free_slot = -1;
	uint32	counter;
	struct pg_tm *tm;
	pg_time_t	tt;
	char	ts_buf[16];
	char   *comments_cstr = NULL;

	if (hgp_shared == NULL)
	{
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("hg_profiler is not loaded via shared_preload_libraries")));
	}

	/* Argument extraction */
	comments_arg = PG_ARGISNULL(0) ? NULL : PG_GETARG_TEXT_PP(0);
	users_arg = PG_ARGISNULL(1) ? NULL : (oidvector *) PG_GETARG_POINTER(1);
	dbs_arg = PG_ARGISNULL(2) ? NULL : (oidvector *) PG_GETARG_POINTER(2);
	max_log_limit_mb = PG_ARGISNULL(3) ? HGP_DEFAULT_MAX_LOG_LIMIT_MB
		: PG_GETARG_INT32(3);
	log_min_duration_sec = PG_ARGISNULL(4) ? HGP_DEFAULT_LOG_MIN_DURATION_SEC
		: PG_GETARG_INT32(4);
	if (PG_ARGISNULL(5))
	{
		default_time_limit.time = HGP_DEFAULT_TIME_LIMIT_USEC;
		default_time_limit.day = 0;
		default_time_limit.month = 0;
		time_limit_arg = &default_time_limit;
	}
	else
		time_limit_arg = PG_GETARG_INTERVAL_P(5);

	now = GetCurrentTimestamp();
	scheduled_end = DatumGetTimestampTz(DirectFunctionCall2(timestamptz_pl_interval,
							TimestampTzGetDatum(now),
							IntervalPGetDatum(time_limit_arg)));
	if (scheduled_end <= now)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("time_limit must be positive")));
	}
	else
	{
		int64	duration_usec = scheduled_end - now;

		if (duration_usec < HGP_TIME_LIMIT_MIN_USEC)
		{
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("time_limit must be at least 1 minute")));
		}
		if (duration_usec > HGP_TIME_LIMIT_MAX_USEC)
		{
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("time_limit must not exceed 180 minutes")));
		}
	}

	if (log_min_duration_sec < 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("log_min_duration must be a non-negative value")));
	}

	/*
	 * max_log_limit is in MB. It must be a positive number.
	 */
	if (max_log_limit_mb <= 0)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("max_log_limit_mb must be a positive value")));
	}
	else if (max_log_limit_mb > 300)
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("max_log_limit_mb must not exceed 300 MB")));
	}

	hg_p_resolve_activate_filters(users_arg, dbs_arg,
								  users_buf, &n_users,
								  dbs_buf, &n_dbs);

	if (comments_arg != NULL)
		comments_cstr = text_to_cstring(comments_arg);

	if (comments_cstr != NULL)
	{
		size_t len = strlen(comments_cstr);
		if (len == 0)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("comments must not be empty")));
		if (len > 100)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("comments must not exceed 100 characters")));
		if (strchr(comments_cstr, '\n') != NULL ||
			strchr(comments_cstr, '\r') != NULL)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("comments must not contain newline characters")));
	}

	/*
	 * Allocate a free shared-memory slot.  We require both !in_use AND
	 * running_queries == 0 -- a slot that has been logically freed but
	 * still has an in-flight backend draining it must not be re-purposed
	 * yet.
	 */
	LWLockAcquire(hgp_shared->lock, LW_EXCLUSIVE);
	for (int i = 0; i < HGP_MAX_TRACES; i++)
	{
		HgpTraceShm *cand = &hgp_shared->traces[i];

		if (!cand->in_use &&
			pg_atomic_read_u32(&cand->running_queries) == 0)
		{
			free_slot = i;
			break;
		}
	}
	if (free_slot < 0)
	{
		LWLockRelease(hgp_shared->lock);
		ereport(ERROR,
				(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
				 errmsg("hg_profiler: too many active traces "
						"(maximum is %d)", HGP_MAX_TRACES)));
	}

	hgp_shared->next_counter++;
	if (hgp_shared->next_counter > 999999)
		hgp_shared->next_counter = 1;
	counter = hgp_shared->next_counter;

	tt = timestamptz_to_time_t(now);
	tm = pg_localtime(&tt, log_timezone);
	pg_strftime(ts_buf, sizeof(ts_buf), "%Y%m%d%H%M%S", tm);
	snprintf(trace_id, sizeof(trace_id), "%14s%06u", ts_buf, counter);

	{
		HgpTraceShm *t = &hgp_shared->traces[free_slot];

		memcpy(t->trace_id, trace_id, HGP_TRACE_ID_BUFSZ);
		strlcpy(t->comments,
				(comments_cstr != NULL) ? comments_cstr : "",
				HGP_COMMENTS_BUFSZ);
		t->owner = GetUserId();
		t->start_time = now;
		t->end_time = scheduled_end;
		t->max_log_size_mb = max_log_limit_mb;
		t->log_min_duration_sec = log_min_duration_sec;
		t->n_users = n_users;
		t->n_databases = n_dbs;
		memcpy(t->users, users_buf, n_users * sizeof(Oid));
		memcpy(t->databases, dbs_buf, n_dbs * sizeof(Oid));
		pg_atomic_write_u32(&t->running_queries, 0);
		pg_atomic_write_u64(&t->curr_log_size, 0);
		t->final_status = 0;
		t->generation++;
		t->in_use = true;
	}

	pg_atomic_fetch_add_u32(&hgp_shared->active_count, 1);
	LWLockRelease(hgp_shared->lock);

	/*
	 * Create the on-disk trace directory and write the initial trace.ini.
	 * If anything below fails, roll back the slot we just claimed.
	 */
	PG_TRY();
	{
		hg_p_make_trace_root();
		hg_p_make_trace_dir(trace_id);

		hg_p_write_trace_ini(trace_id,
						   (comments_cstr != NULL) ? comments_cstr : "",
						   GetUserId(), now, scheduled_end,
						   n_users, users_buf,
						   n_dbs, dbs_buf,
						   max_log_limit_mb, log_min_duration_sec,
						   HGP_STATUS_ACTIVE, 0);
	}
	PG_CATCH();
	{
		char		dummy_id[HGP_TRACE_ID_BUFSZ];

		/* expected_gen=0 -> unconditional release (our slot, just claimed) */
		hg_p_finalise_trace_in_shm_inner(free_slot, HGP_STATUS_STOPPED, 0,
									   dummy_id);
		PG_RE_THROW();
	}
	PG_END_TRY();

	if (comments_cstr != NULL)
		pfree(comments_cstr);

	PG_RETURN_TEXT_P(cstring_to_text(trace_id));
}

/*
 * hg_p_deactivate(trace_id text) RETURNS bool
 */
Datum
hg_p_deactivate(PG_FUNCTION_ARGS)
{
	text	   *trace_id_arg = PG_GETARG_TEXT_PP(0);
	char	   *trace_id = text_to_cstring(trace_id_arg);
	int			slot = -1;
	Oid			slot_owner = InvalidOid;
	HgpTraceShm	saved;
	uint32		saved_gen = 0;
	char		freed_id[HGP_TRACE_ID_BUFSZ] = {0};
	bool		freed;
	TimestampTz finish_time = 0;

	if (hgp_shared == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("hg_profiler is not loaded via shared_preload_libraries")));

	memset(&saved, 0, sizeof(saved));
	LWLockAcquire(hgp_shared->lock, LW_SHARED);
	for (int i = 0; i < HGP_MAX_TRACES; i++)
	{
		HgpTraceShm *t = &hgp_shared->traces[i];

		if (!t->in_use)
			continue;
		if (strcmp(t->trace_id, trace_id) == 0)
		{
			slot = i;
			slot_owner = t->owner;
			saved = *t;
			saved_gen = t->generation;
			break;
		}
	}
	LWLockRelease(hgp_shared->lock);

	if (slot < 0)
		ereport(ERROR,
				(errcode(ERRCODE_NO_DATA_FOUND),
				 errmsg("trace \"%s\" is not currently active", trace_id)));

	if (!superuser() && slot_owner != GetUserId())
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("only the owner or a superuser may stop trace \"%s\"",
						trace_id)));

	freed = hg_p_finalise_trace_in_shm_inner(slot, HGP_STATUS_STOPPED,
										   saved_gen, freed_id);
	if (freed)
	{
		finish_time = GetCurrentTimestamp();

		hg_p_write_trace_ini(trace_id, saved.comments,
						   saved.owner, saved.start_time, saved.end_time,
						   saved.n_users, saved.users,
						   saved.n_databases, saved.databases,
						   saved.max_log_size_mb, saved.log_min_duration_sec,
						   HGP_STATUS_STOPPED, finish_time);
	}

	PG_RETURN_BOOL(true);
}

static void
hg_p_init_materialized_srf(FunctionCallInfo fcinfo)
{

#if PG_VERSION_NUM >= 160000
	InitMaterializedSRF(fcinfo, 0);

#elif PG_VERSION_NUM >= 150000
	SetSingleFuncCall(fcinfo, 0);

#else
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
	Tuplestorestate *tupstore;
	MemoryContext old_context;
	MemoryContext per_query_ctx;
	TupleDesc    stored_tupdesc;
	bool        random_access;

	/* check to see if caller supports returning a tuplestore */
	if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
 		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("set-valued function called in context that cannot accept a set")));
	if (!(rsinfo->allowedModes & SFRM_Materialize))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("materialize mode required, but it is not allowed in this context")));

	/*
	 * Store the tuplestore and the tuple descriptor in ReturnSetInfo; this
	 * must be done in the per-query memory context.
	 */
	per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
	old_context = MemoryContextSwitchTo(per_query_ctx);

	if (get_call_result_type(fcinfo, NULL, &stored_tupdesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");

	random_access = (rsinfo->allowedModes & SFRM_Materialize_Random) != 0;

	tupstore = tuplestore_begin_heap(random_access, false, work_mem);
	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = tupstore;
	rsinfo->setDesc = stored_tupdesc;
	MemoryContextSwitchTo(old_context);
#endif			/* PG_VERSION_NUM < 150000 */
}

/*
 * hg_p_active_traces() -- list shared-memory active slots.
 *
 * Returned column order (must stay in sync with hg_p_traces_list() and
 * hg_profiler--1.0.sql):
 *   0 trace_id
 *   1 comments
 *   2 owner
 *   3 start_time
 *   4 end_time
 *   5 max_log_limit
 *   6 log_min_duration
 *   7 users
 *   8 databases
 *   9 final_status
 */

#define HGP_ACTIVE_NCOLS 10

Datum
hg_p_active_traces(PG_FUNCTION_ARGS)
{
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
	bool		see_all;
	Oid			me = GetUserId();
	TimestampTz now;
	HgpTraceShm	rows[HGP_MAX_TRACES];
	int			nrows = 0;

	hg_p_init_materialized_srf(fcinfo);

	see_all = superuser();
	now = GetCurrentTimestamp();

	if (hgp_shared == NULL)
		return (Datum) 0;

	hg_p_prune_finished_traces(now);

	LWLockAcquire(hgp_shared->lock, LW_SHARED);
	for (int i = 0; i < HGP_MAX_TRACES; i++)
	{
		HgpTraceShm *t = &hgp_shared->traces[i];

		if (!t->in_use)
			continue;
		if (!see_all && t->owner != me)
			continue;
		if (t->end_time != 0 && now >= t->end_time)
			continue;
		rows[nrows++] = *t;
	}
	LWLockRelease(hgp_shared->lock);

	for (int i = 0; i < nrows; i++)
	{
		HgpTraceShm *t = &rows[i];
		Datum		values[HGP_ACTIVE_NCOLS];
		bool		nulls[HGP_ACTIVE_NCOLS];

		memset(nulls, false, sizeof(nulls));

		values[0] = CStringGetTextDatum(t->trace_id);

		if (t->comments[0] != '\0')
		{
			values[1] = CStringGetTextDatum(t->comments);
		}
		else
		{
			nulls[1] = true;
			values[1] = (Datum) 0;
		}
		values[2] = ObjectIdGetDatum(t->owner);

		values[3] = TimestampTzGetDatum(t->start_time);
		if (t->end_time != 0)
			values[4] = TimestampTzGetDatum(t->end_time);
		else
		{
			nulls[4] = true;
			values[4] = (Datum) 0;
		}
		values[5] = Int32GetDatum(t->max_log_size_mb);
		values[6] = Int32GetDatum(t->log_min_duration_sec);
		values[7] = PointerGetDatum(buildoidvector(t->users, t->n_users));
		values[8] = PointerGetDatum(buildoidvector(t->databases, t->n_databases));
		values[9] = Int32GetDatum(HGP_STATUS_ACTIVE);

		tuplestore_putvalues(rsinfo->setResult, rsinfo->setDesc, values, nulls);
	}

	return (Datum) 0;
}

/*
 * hg_p_traces_list() -- enumerate $PGDATA/hg_profiler/ and emit one row
 * per trace.ini found.  This is the "all traces" view and reports
 * statuses exactly as stored on disk.
 */
#define HGP_LIST_NCOLS 10

Datum
hg_p_traces_list(PG_FUNCTION_ARGS)
{
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
	bool		see_all;
	Oid			me = GetUserId();
	char		root[MAXPGPATH];
	DIR		   *dir;
	struct dirent *ent;

	hg_p_init_materialized_srf(fcinfo);

	see_all = superuser();
	hg_p_prune_finished_traces(GetCurrentTimestamp());

	hg_p_path_root(root, sizeof(root));

	dir = AllocateDir(root);
	if (dir == NULL)
	{
		if (errno == ENOENT)
			return (Datum) 0;
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("hg_profiler: could not open \"%s\": %m", root)));
	}

	while ((ent = ReadDir(dir, root)) != NULL)
	{
		HgpTraceIni	ini;
		Datum		values[HGP_LIST_NCOLS];
		bool		nulls[HGP_LIST_NCOLS];
		int			final_status;

		if (ent->d_name[0] == '.')
			continue;
		if (!hg_p_is_valid_trace_id(ent->d_name))
			continue;

		if (!hg_p_read_trace_ini(ent->d_name, &ini))
			continue;

		if (!see_all && ini.owner != me)
		{
			hg_p_free_trace_ini(&ini);
			continue;
		}

		final_status = ini.status;

		memset(nulls, false, sizeof(nulls));
		values[0] = CStringGetTextDatum(ent->d_name);
		if (ini.comments == NULL || *ini.comments == '\0')
		{
			nulls[1] = true;
			values[1] = (Datum) 0;
		}
		else
			values[1] = CStringGetTextDatum(ini.comments);
		values[2] = ObjectIdGetDatum(ini.owner);

		if (ini.start_time != 0)
			values[3] = TimestampTzGetDatum(ini.start_time);
		else
		{
			nulls[3] = true;
			values[3] = (Datum) 0;
		}

		if (ini.end_time_set)
			values[4] = TimestampTzGetDatum(ini.end_time);
		else
		{
			nulls[4] = true;
			values[4] = (Datum) 0;
		}

		values[5] = Int32GetDatum(ini.max_log_size_mb);
		values[6] = Int32GetDatum(ini.log_min_duration_sec);
		values[7] = PointerGetDatum(buildoidvector(ini.users, ini.n_users));
		values[8] = PointerGetDatum(buildoidvector(ini.databases, ini.n_databases));
		values[9] = Int32GetDatum(final_status);

		tuplestore_putvalues(rsinfo->setResult, rsinfo->setDesc,
							 values, nulls);
		hg_p_free_trace_ini(&ini);
	}

	FreeDir(dir);
	return (Datum) 0;
}

/*
 * CSV record parser -- splits a single line into HGP_CSV_NCOLS palloc'd
 * strings.  Returns true on success, false if the record is malformed.
 *
 * This matches what hg_p_csv_append_quoted writes:
 *   - Optional "..." quoted strings with embedded "" -> " un-escaping
 *   - Bare numeric / unquoted values
 *   - Comma-separated, ends with newline
 */
static bool
hg_p_parse_csv_line(const char *line, char **fields, int n_fields)
{
	const char *p = line;
	int			f;

	for (f = 0; f < n_fields; f++)
		fields[f] = NULL;

	for (f = 0; f < n_fields; f++)
	{
		StringInfoData val;

		initStringInfo(&val);

		if (*p == '"')
		{
			p++;
			while (*p != '\0')
			{
				if (*p == '"')
				{
					if (*(p + 1) == '"')
					{
						appendStringInfoChar(&val, '"');
						p += 2;
						continue;
					}
					p++;
					break;
				}
				appendStringInfoChar(&val, *p);
				p++;
			}
		}
		else
		{
			while (*p != '\0' && *p != ',' && *p != '\n' && *p != '\r')
			{
				appendStringInfoChar(&val, *p);
				p++;
			}
		}

		fields[f] = val.data;

		if (f == n_fields - 1)
		{
			while (*p == '\n' || *p == '\r' || *p == ' ' || *p == '\t')
				p++;
			if (*p != '\0')
			{
				/* trailing junk; tolerate */
			}
			break;
		}

		if (*p != ',')
		{
			/* Truncated record. */
			for (int j = 0; j <= f; j++)
				if (fields[j] != NULL)
				{
					pfree(fields[j]);
					fields[j] = NULL;
				}
			return false;
		}
		p++;
	}

	return true;
}

/*
 * hg_p_load_trace(trace_id text) RETURNS bool
 *
 * Scans $PGDATA/hg_profiler/<trace_id>/trace-*.csv files, parses each
 * row, and inserts it into the temporary table _hg_tmp_tbl_p_profiler.
 * The temp table is created on first call, then truncated and reloaded
 * from scratch on every invocation.
 */

#define HGP_LEGACY_TEMP_TABLE	"_hg_tmp_tbl_p_profiler"

/*
 * Temp-table layout.  This intentionally mirrors the column set documented
 * in hg_profiler--4.1.sql (the "non-conventional" block at the bottom of
 * that file), except that:
 *   - trace_id is kept as text rather than int8, because hg_p_activate now
 *     returns a 20-character text id; tools should treat trace_id as the
 *     value they passed in to hg_p_load_trace().
 *   - we add a "query" column so callers can see the SQL text directly
 *     instead of having to extract it from the EXPLAIN output.
 * All other columns and ordering match the expected 4.x temp table.
 */
static const char hgp_temp_create_sql[] =
"CREATE TEMP TABLE IF NOT EXISTS " HGP_LEGACY_TEMP_TABLE " ("
"  trace_id            text,"
"  start_time          timestamptz,"
"  end_time            timestamptz,"
"  appl_name           text,"
"  user_id             oid,"
"  db_id               oid,"
"  query_type          int4,"
"  query               text,"
"  \"explain\"         text,"
"  duration            float8,"
"  rows_updated        int8,"
"  fs_in               int8,"
"  fs_out              int8,"
"  page_faults         int8,"
"  page_reclaims       int8,"
"  swaps               int8,"
"  sign_recv           int8,"
"  msg_recv            int8,"
"  msg_snd             int8,"
"  vol_contx_switch    int8,"
"  invol_contx_switch  int8,"
"  shared_blk_read     int8,"
"  shared_blk_written  int8,"
"  shared_blk_hit      int8,"
"  local_blk_read      int8,"
"  local_blk_written   int8,"
"  local_blk_hit       int8,"
"  tmp_blk_read        int8,"
"  tmp_blk_written     int8,"
"  pid                 int4"
") ON COMMIT PRESERVE ROWS";

#define HGP_LOAD_NPARAMS	30

static const char hgp_load_insert_sql[] =
"INSERT INTO " HGP_LEGACY_TEMP_TABLE " ("
"  trace_id, start_time, end_time, appl_name, user_id, db_id,"
"  query_type, query, \"explain\", duration, rows_updated,"
"  fs_in, fs_out, page_faults, page_reclaims, swaps,"
"  sign_recv, msg_recv, msg_snd, vol_contx_switch, invol_contx_switch,"
"  shared_blk_read, shared_blk_written, shared_blk_hit,"
"  local_blk_read, local_blk_written, local_blk_hit,"
"  tmp_blk_read, tmp_blk_written, pid)"
" VALUES ("
"  $1,$2,$3,$4,$5,$6,"
"  $7,$8,$9,$10,$11,$12,"
"  $13,$14,$15,$16,$17,"
"  $18,$19,$20,$21,$22,"
"  $23,$24,$25,$26,$27,$28,"
"  $29,$30)";

static bool
hg_p_temp_relation_exists(const char *relname)
{
	Oid			argtypes[1] = {TEXTOID};
	Datum		argvalues[1];
	int			ret;

	argvalues[0] = CStringGetTextDatum(relname);

	ret = SPI_execute_with_args(
		"SELECT 1 "
		"FROM pg_catalog.pg_class c "
		"JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace "
		"WHERE n.oid = pg_my_temp_schema() AND c.relname = $1",
		1, argtypes, argvalues, NULL, true, 1);
	if (ret != SPI_OK_SELECT)
		elog(ERROR, "hg_profiler: failed to probe temp table %s", relname);

	return SPI_processed > 0;
}

static void
hg_p_ensure_load_tables(void)
{
	if (!hg_p_temp_relation_exists(HGP_LEGACY_TEMP_TABLE))
	{
		if (SPI_execute(hgp_temp_create_sql, false, 0) != SPI_OK_UTILITY)
			elog(ERROR, "hg_profiler: failed to create %s",
				 HGP_LEGACY_TEMP_TABLE);
	}
}

static void
hg_p_reset_load_table(void)
{
	if (SPI_execute("TRUNCATE TABLE " HGP_LEGACY_TEMP_TABLE,
					false, 0) != SPI_OK_UTILITY)
		elog(ERROR, "hg_profiler: failed to truncate %s",
			 HGP_LEGACY_TEMP_TABLE);
}

static SPIPlanPtr
hg_p_prepare_load_plan(void)
{
	Oid			argtypes[HGP_LOAD_NPARAMS] = {
		TEXTOID,				/* trace_id */
		TIMESTAMPTZOID,			/* start_time */
		TIMESTAMPTZOID,			/* end_time */
		TEXTOID,				/* appl_name */
		OIDOID,					/* user_id */
		OIDOID,					/* db_id */
		INT4OID,				/* query_type */
		TEXTOID,				/* query */
		TEXTOID,				/* explain */
		FLOAT8OID,				/* duration */
		INT8OID,				/* rows_updated */
		INT8OID, INT8OID, INT8OID, INT8OID, INT8OID,	/* rusage 1..5 */
		INT8OID, INT8OID, INT8OID, INT8OID, INT8OID,	/* rusage 6..10 */
		INT8OID, INT8OID, INT8OID,	/* shared 3 */
		INT8OID, INT8OID, INT8OID,	/* local 3 */
		INT8OID, INT8OID,		/* temp 2 */
		INT4OID					/* pid */
	};
	SPIPlanPtr	plan;

	StaticAssertStmt(lengthof(argtypes) == HGP_LOAD_NPARAMS,
					 "hgp_load_insert argtypes count mismatch");

	plan = SPI_prepare(hgp_load_insert_sql, HGP_LOAD_NPARAMS, argtypes);
	if (plan == NULL)
		elog(ERROR, "hg_profiler: SPI_prepare failed: %s",
			 SPI_result_code_string(SPI_result));
	return plan;
}

/*
 * Insert a single CSV-parsed record into the temp table.
 */
static void
hg_p_insert_csv_record(SPIPlanPtr plan, const char *trace_id,
					 char **fields)
{
	Datum		values[HGP_LOAD_NPARAMS];
	char		nulls[HGP_LOAD_NPARAMS];
	int			ret;
	int			i = 0;

	memset(nulls, ' ', sizeof(nulls));

#define HGP_SET_TEXT_OR_NULL(idx, src)							\
	do {														\
		if ((src) == NULL || *(src) == '\0')					\
		{														\
			nulls[(idx)] = 'n';									\
			values[(idx)] = (Datum) 0;							\
		}														\
		else													\
			values[(idx)] = CStringGetTextDatum(src);			\
	} while (0)

	values[i++] = CStringGetTextDatum(trace_id);

	values[i++] = TimestampTzGetDatum(hg_p_parse_timestamptz(fields[0]));
	values[i++] = TimestampTzGetDatum(hg_p_parse_timestamptz(fields[1]));

	HGP_SET_TEXT_OR_NULL(i, fields[2]);
	i++;

	values[i++] = ObjectIdGetDatum((Oid) strtoul(fields[3], NULL, 10));
	values[i++] = ObjectIdGetDatum((Oid) strtoul(fields[4], NULL, 10));

	values[i++] = Int32GetDatum((int32) atoi(fields[5]));

	HGP_SET_TEXT_OR_NULL(i, fields[6]);
	i++;
	HGP_SET_TEXT_OR_NULL(i, fields[7]);
	i++;

	values[i++] = Float8GetDatum(strtod(fields[8], NULL));
	values[i++] = Int64GetDatum((int64) strtoll(fields[9], NULL, 10));

	for (int k = 10; k <= 27; k++)
		values[i++] = Int64GetDatum((int64) strtoll(fields[k], NULL, 10));

	values[i++] = Int32GetDatum((int32) atoi(fields[28]));

#undef HGP_SET_TEXT_OR_NULL

	Assert(i == HGP_LOAD_NPARAMS);

	ret = SPI_execute_plan(plan, values, nulls, false, 0);
	if (ret != SPI_OK_INSERT)
		ereport(WARNING,
				(errmsg("hg_profiler: SPI_execute_plan(load) failed: %s",
						SPI_result_code_string(ret))));
}

/*
 * Read a single CSV record from fp, possibly spanning multiple lines if
 * a quoted field embeds a newline (for example, multi-line EXPLAIN text).
 *
 * Returns palloc'd line including trailing newline.  *complete_out is
 * set to true if the record ended on a newline (i.e. was fully on disk
 * at the time we read it), or false if we hit EOF mid-record -- the
 * latter happens when hg_p_load_trace runs concurrently with a backend
 * that is still appending to the CSV file.  Callers must not advance
 * the saved byte offset for incomplete records.
 *
 * Returns NULL at clean EOF.
 */
static char *
hg_p_read_csv_record(FILE *fp, bool *complete_out)
{
	StringInfoData buf;
	char		linebuf[8192];
	bool		in_quotes = false;
	bool		complete = false;

	initStringInfo(&buf);

	/*
	 * Read whole lines with fgets (much faster than fgetc per character)
	 * and append them to buf.  A record is complete when we encounter a
	 * newline outside of a quoted field.
	 */
	while (fgets(linebuf, sizeof(linebuf), fp) != NULL)
	{
		const char *p;

		appendStringInfoString(&buf, linebuf);

		/*
		 * Scan this chunk to track quote state.  Inside a quoted field,
		 * doubled quotes ("") represent a literal quote and do NOT
		 * change the in_quotes state.
		 */
		for (p = linebuf; *p; p++)
		{
			if (*p == '"')
			{
				if (in_quotes && *(p + 1) == '"')
					p++;		/* skip escaped quote */
				else
					in_quotes = !in_quotes;
			}
		}

		/*
		 * If the chunk ends with a newline and we're outside quotes,
		 * the record is complete.
		 */
		if (buf.len > 0 && buf.data[buf.len - 1] == '\n' && !in_quotes)
		{
			complete = true;
			break;
		}

		/*
		 * Otherwise: either the buffer was full (no trailing '\n') so
		 * we need to read more of the same line, or we're inside a
		 * quoted multi-line field and need to continue.
		 */
	}

	*complete_out = complete;

	if (buf.len == 0)
	{
		pfree(buf.data);
		return NULL;
	}
	return buf.data;
}

static void
hg_p_load_one_pid_file(SPIPlanPtr plan, const char *trace_id,
					   int pid, const char *path)
{
	FILE	   *fp;
	char	   *line;
	char	   *fields[HGP_CSV_NCOLS];

	fp = AllocateFile(path, "r");
	if (fp == NULL)
	{
		if (errno == ENOENT)
			return;
		ereport(WARNING,
				(errcode_for_file_access(),
				 errmsg("hg_profiler: could not open \"%s\": %m", path)));
		return;
	}

	for (;;)
	{
		bool		complete;

		line = hg_p_read_csv_record(fp, &complete);
		if (line == NULL)
			break;

		if (!complete)
		{
			pfree(line);
			break;
		}

		if (hg_p_parse_csv_line(line, fields, HGP_CSV_NCOLS))
		{
			hg_p_insert_csv_record(plan, trace_id, fields);
			for (int k = 0; k < HGP_CSV_NCOLS; k++)
				if (fields[k] != NULL)
				{
					pfree(fields[k]);
					fields[k] = NULL;
				}
		}
		pfree(line);
	}

	(void) FreeFile(fp);
}

Datum
hg_p_load_trace(PG_FUNCTION_ARGS)
{
	text	   *trace_id_arg = PG_GETARG_TEXT_PP(0);
	char	   *trace_id = text_to_cstring(trace_id_arg);
	HgpTraceIni	ini;
	bool		ini_ok;
	char		dir_path[MAXPGPATH];
	DIR		   *dir;
	struct dirent *ent;
	SPIPlanPtr	plan;

	if (!hg_p_is_valid_trace_id(trace_id))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("invalid trace_id \"%s\": must be %d decimal digits",
						trace_id, HGP_TRACE_ID_LEN)));

	ini_ok = hg_p_read_trace_ini(trace_id, &ini);
	if (!ini_ok)
	{
		hg_p_free_trace_ini(&ini);
		ereport(ERROR,
				(errcode(ERRCODE_NO_DATA_FOUND),
				 errmsg("trace \"%s\" not found", trace_id)));
	}

	if (!superuser() && ini.owner != GetUserId())
	{
		hg_p_free_trace_ini(&ini);
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("only the owner or a superuser may load trace \"%s\"",
						trace_id)));
	}

	hg_p_free_trace_ini(&ini);

	if (SPI_connect() != SPI_OK_CONNECT)
		elog(ERROR, "hg_profiler: SPI_connect failed");

	hg_p_ensure_load_tables();
	hg_p_reset_load_table();

	plan = hg_p_prepare_load_plan();

	hg_p_path_trace_dir(dir_path, sizeof(dir_path), trace_id);
	dir = AllocateDir(dir_path);
	if (dir == NULL)
	{
		SPI_finish();
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("hg_profiler: could not open \"%s\": %m", dir_path)));
	}

	while ((ent = ReadDir(dir, dir_path)) != NULL)
	{
		const char *name = ent->d_name;
		int			pid;
		char		path[MAXPGPATH];
		struct stat fst;

		if (!hg_p_parse_trace_csv_filename(name, &pid))
			continue;

		snprintf(path, sizeof(path), "%s/%s", dir_path, name);
		if (stat(path, &fst) != 0)
		{
			if (errno == ENOENT)
				continue;
			ereport(WARNING,
					(errcode_for_file_access(),
					 errmsg("hg_profiler: could not stat file \"%s\": %m",
							path)));
			continue;
		}
		if (!S_ISREG(fst.st_mode))
			continue;
		hg_p_load_one_pid_file(plan, trace_id, pid, path);
	}

	FreeDir(dir);

	SPI_finish();
	PG_RETURN_BOOL(true);
}

/*
 * hg_p_cleanup(trace_id text, remove_active bool) RETURNS bool
 *
 * Permanently delete the trace's on-disk directory.
 * If the trace is still active and remove_active is true, stop it first;
 * If it's active and remove_active is false, raise an error.
 */
Datum
hg_p_cleanup(PG_FUNCTION_ARGS)
{
	text	*trace_id_arg = PG_GETARG_TEXT_PP(0);
	bool	remove_active = PG_GETARG_BOOL(1);
	char	*trace_id = text_to_cstring(trace_id_arg);
	HgpTraceIni	ini;
	char	dir_path[MAXPGPATH];
	int	active_slot = -1;
	uint32	active_gen = 0;

	if (!hg_p_is_valid_trace_id(trace_id))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("invalid trace_id \"%s\": must be %d decimal digits",
						trace_id, HGP_TRACE_ID_LEN)));

	if (!hg_p_read_trace_ini(trace_id, &ini))
	{
		hg_p_free_trace_ini(&ini);
		ereport(ERROR,
				(errcode(ERRCODE_NO_DATA_FOUND),
				 errmsg("trace \"%s\" not found", trace_id)));
	}

	if (!superuser() && ini.owner != GetUserId())
	{
		hg_p_free_trace_ini(&ini);
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("only the owner or a superuser may remove trace \"%s\"",
						trace_id)));
	}

	hg_p_free_trace_ini(&ini);

	/* Is the trace still active in shmem? */
	if (hgp_shared != NULL)
	{
		LWLockAcquire(hgp_shared->lock, LW_SHARED);
		for (int i = 0; i < HGP_MAX_TRACES; i++)
		{
			HgpTraceShm *t = &hgp_shared->traces[i];

			if (t->in_use && strcmp(t->trace_id, trace_id) == 0)
			{
				active_slot = i;
				active_gen = t->generation;
				break;
			}
		}
		LWLockRelease(hgp_shared->lock);
	}

	if (active_slot >= 0)
	{
		char		dummy[HGP_TRACE_ID_BUFSZ];

		if (!remove_active)
			ereport(ERROR,
					(errcode(ERRCODE_OBJECT_IN_USE),
					 errmsg("trace \"%s\" is still active; pass "
							"remove_active=true to force removal",
							trace_id)));
		(void) hg_p_finalise_trace_in_shm_inner(active_slot,
											  HGP_STATUS_STOPPED,
											  active_gen, dummy);
	}

	hg_p_path_trace_dir(dir_path, sizeof(dir_path), trace_id);
	if (!rmtree(dir_path, true))
		ereport(WARNING,
				(errcode_for_file_access(),
				 errmsg("hg_profiler: could not fully remove \"%s\"",
						dir_path)));

	PG_RETURN_BOOL(true);
}

Datum
hg_p_version(PG_FUNCTION_ARGS)
{
	PG_RETURN_FLOAT8(HGP_PROFILER_VERSION);
}
