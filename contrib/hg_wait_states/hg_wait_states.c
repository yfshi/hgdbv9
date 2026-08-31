/*-------------------------------------------------------------------------
 *
 * hg_wait_states.c
 *		Background sampler of per-backend wait events.
 *
 * This module starts a single background worker that wakes up every
 * seconds and writes three kinds of
 * binary log files inside "hg_wait_states.directory":
 *
 *		hg_ws_sessions_<created>_<rotate>_<flag>
 *		hg_ws_queries_<created>_<rotate>_<flag>
 *		hg_ws_samples_<created>_<rotate>_<flag>
 *
 * The on-disk binary layout is intentionally compatible with the previous
 * implementation so the SQL reader functions (declared in
 * hg_wait_states--*.sql) keep working unchanged.  Files rotate daily for
 * sessions/queries and hourly for samples.
 *
 * The previous version of this module maintained a custom queryId
 * generator (JumbleExpr/getQueryId) and a shared-memory entry per backend
 * that was filled by post_parse_analyze / Executor* / ProcessUtility
 * hooks.  Starting with PostgreSQL 14 the core can be asked to compute
 * the queryId via EnableQueryId(), so we drop the custom jumbler
 * altogether and read everything we need straight from the PgBackendStatus
 * snapshot that the core already maintains.  The hooks listed above are
 * therefore no longer required:
 *
 *
 * A tiny shared-memory area is still kept so that we can detect a
 * duplicate background worker and remember its PID.
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <sys/stat.h>
#include <unistd.h>

#include "access/xact.h"
#include "commands/dbcommands.h"
#include "fmgr.h"
#include "funcapi.h"
#include "lib/stringinfo.h"
#include "miscadmin.h"
#include "pgstat.h"
#include "postmaster/bgworker.h"
#include "storage/fd.h"
#include "storage/ipc.h"
#include "storage/latch.h"
#include "storage/lwlock.h"
#include "storage/proc.h"
#include "storage/procarray.h"
#include "storage/shmem.h"
#include "utils/backend_status.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/hsearch.h"
#include "utils/memutils.h"
#if PG_VERSION_NUM >= 160000
#include "nodes/queryjumble.h"
#else
#include "utils/queryjumble.h"
#endif
#include "utils/timestamp.h"
#include "utils/wait_event.h"

PG_MODULE_MAGIC;

/*-------------------------------------------------------------------------
 * On-disk format constants
 *-------------------------------------------------------------------------
 */

/* Magic written at the start of every log file (kept for compatibility) */
#define HG_WS_FILE_MAGIC		0x67578C8BU
/* File-format version, value 1 means the v1.2 layout */
#define HG_WS_FILE_VERSION		1

/* File-name prefixes (must remain identical with the historic reader) */
#define HG_WS_SESSIONS_PREFIX	"hg_ws_sessions_"
#define HG_WS_QUERIES_PREFIX	"hg_ws_queries_"
#define HG_WS_SAMPLES_PREFIX	"hg_ws_samples_"

/* Rotation periods, in micro-seconds */
#define HG_WS_SAMPLES_ROTATE_US		INT64CONST(3600000000)	/* 1 hour */
#define HG_WS_SESSIONS_ROTATE_US	INT64CONST(86400000000) /* 1 day */
#define HG_WS_QUERIES_ROTATE_US		INT64CONST(86400000000) /* 1 day */
#define HG_WS_PURGE_PERIOD_US		INT64CONST(3600000000)	/* 1 hour */

/*
 * How often the worker forces buffered data down to durable storage.
 * Per-cycle fflush already pushes bytes into the kernel page cache so
 * readers see them immediately; pg_fsync() in addition guarantees that an
 * OS-level crash loses at most one fsync period worth of samples instead
 * of an unbounded amount.  30 s is small enough to bound the loss window
 * yet large enough that the extra syscall is negligible compared to the
 * per-cycle work (typically << 1 ms even on slow disks).
 */
#define HG_WS_FSYNC_PERIOD_US		INT64CONST(30000000)	/* 30 seconds */

/*
 * How often the worker collect the wait data
 */
#define HG_WS_SAMPLING_INTERVAL_SEC	1

/*
 * Hard cap on any in-file variable-length string (database/user/application
 * name, client hostname).  PostgreSQL bounds each of these to NAMEDATALEN
 * (64) in practice; we use a 16x margin so a corrupt source string can
 * never grow the log file by an unbounded amount and so the reader can
 * sanity-check the embedded length.
 */
#define HG_WS_MAX_STRING_LEN	(NAMEDATALEN * 16)

/*
 * Layout of a single sample record on disk (40 bytes, identical with the
 * historic encoding so older log files remain readable).
 */
typedef struct HgWsSampleRecord
{
	int64		query_id;			/* offset 0 */
	TimestampTz sample_time;		/* offset 8 */
	uint32		wait_event_info;	/* offset 16 */
	int32		session_id;			/* offset 20 */
	TimestampTz query_start_time;	/* offset 24 */
	int32		sampling_interval;	/* offset 32 */
	int32		pad;				/* offset 36 -> total = 40 */
}			HgWsSampleRecord;

StaticAssertDecl(sizeof(HgWsSampleRecord) == 40,
				 "hg_wait_states sample record must be 40 bytes");

/*-------------------------------------------------------------------------
 * GUC variables
 *-------------------------------------------------------------------------
 */
static char	*hg_ws_directory = NULL;
static int	hg_ws_retention_period = 7;	/* days */
static bool	hg_ws_enable_collection = false;
static char	*hg_ws_database = NULL;

/*-------------------------------------------------------------------------
 * Shared-memory state (minimal: just bgworker pid + lock).
 *-------------------------------------------------------------------------
 */
typedef struct HgWsSharedState
{
	LWLock	   *lock;			/* protects worker_pid */
	pid_t		worker_pid;		/* PID of running collector, or 0 */
}			HgWsSharedState;

static HgWsSharedState *hg_ws_state = NULL;

/* Hooks we chain */
#if PG_VERSION_NUM >= 150000
static shmem_request_hook_type	prev_shmem_request_hook = NULL;
#endif
static shmem_startup_hook_type prev_shmem_startup_hook = NULL;

/* Signal flags inside the bgworker */
static volatile sig_atomic_t got_sigterm = false;
static volatile sig_atomic_t got_sighup = false;

/*-------------------------------------------------------------------------
 * Per-log-file in-memory state used by the bgworker.
 *-------------------------------------------------------------------------
 */
typedef struct HgWsLogFile
{
	FILE	   *fp;					/* current file handle, or NULL */
	char		path[MAXPGPATH];	/* absolute path including dir */
	char		name[MAXPGPATH];	/* basename only */
	TimestampTz created_ts;			/* timestamp embedded in the name */
	TimestampTz rotate_at;			/* next rotation point (us) */
}			HgWsLogFile;

/*
 * Session hash key.
 *
 * Defined here (before the hash warm-up helpers that need it) and reused
 * by the bgworker.  The OS pid alone is not a stable identifier because a
 * pid can be recycled by a brand new backend; pairing it with
 * proc_start_timestamp gives a key that is unique for the lifetime of the
 * backend.  Padding inside the key is zeroed explicitly so HASH_BLOBS
 * hashes and compares only the meaningful bytes.
 */
typedef struct HgWsSessionKey
{
	int32		session_id;
	int32		key_pad;		/* explicit padding, kept zeroed */
	TimestampTz proc_start_ts;
}			HgWsSessionKey;

static inline void
hg_ws_init_session_key(HgWsSessionKey *key, int32 pid, TimestampTz proc_start_ts)
{
	memset(key, 0, sizeof(*key));
	key->session_id = pid;
	key->proc_start_ts = proc_start_ts;
}

/* Reader functions */
PG_FUNCTION_INFO_V1(hg_wait_states_samples);
PG_FUNCTION_INFO_V1(hg_wait_states_samples_internal);
PG_FUNCTION_INFO_V1(hg_wait_states_sessions);
PG_FUNCTION_INFO_V1(hg_wait_states_queries);
PG_FUNCTION_INFO_V1(hg_wait_states_purge);
PG_FUNCTION_INFO_V1(hg_wait_states_directory_size);

PGDLLEXPORT void hg_wait_states_main(Datum main_arg);
void		_PG_init(void);

/* Forward declarations of static helpers */
#if PG_VERSION_NUM >= 150000
static void hg_ws_shmem_request(void);
#endif
static void hg_ws_shmem_startup(void);
static Size hg_ws_shmem_size(void);
static void hg_ws_sigterm(SIGNAL_ARGS);
static void hg_ws_sighup(SIGNAL_ARGS);
static void hg_ws_detach_shmem(int code, Datum arg);
static void hg_ws_register_worker(void);
static void hg_ws_init_directory(void);
static bool hg_ws_is_log_file(const char *name, const char *prefix,
							TimestampTz *created_ts, TimestampTz *rotate_ts,
							int *flag);
static void hg_ws_create_log_file(HgWsLogFile *lf, const char *prefix,
								TimestampTz now);
static void hg_ws_open_or_create(HgWsLogFile *lf, const char *prefix,
							   TimestampTz now, int64 rotate_period_us);
static void hg_ws_seed_session_hash_from_file(const char *path,
											HTAB *session_hash);
static void hg_ws_seed_query_hash_from_active_files(const char *prefix,
												  TimestampTz now,
												  HTAB *query_hash);
static void hg_ws_seed_session_hash_from_active_files(const char *prefix,
													TimestampTz now,
													HTAB *session_hash);
static void hg_ws_seed_query_hash_from_file(const char *path,
										  HTAB *query_hash);
static void hg_ws_write_header(HgWsLogFile *lf);
static bool hg_ws_validate_header(FILE *fp, const char *path);
static void hg_ws_purge_files(TimestampTz start_ts, TimestampTz end_ts);
static void hg_ws_collect_once(TimestampTz now,
							 HgWsLogFile *sessions, HgWsLogFile *queries,
							 HgWsLogFile *samples,
							 HTAB *session_hash, HTAB *query_hash);

/* SRF helpers */
static void read_samples(ReturnSetInfo *rsinfo, TimestampTz start_ts,
						 TimestampTz end_ts, bool with_interval);
static void read_sessions(ReturnSetInfo *rsinfo, TimestampTz start_ts,
						  TimestampTz end_ts, bool extended);
static void read_queries(ReturnSetInfo *rsinfo, TimestampTz start_ts,
						 TimestampTz end_ts);
static int64 hg_ws_dir_size(TimestampTz start_ts, TimestampTz end_ts);

/*-------------------------------------------------------------------------
 * Module load
 *-------------------------------------------------------------------------
 */
void
_PG_init(void)
{
	if (!process_shared_preload_libraries_in_progress)
	{
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("hg_wait_states must be loaded via shared_preload_libraries")));
	}

	/*
	 * Ask the core to compute query_id for us (the equivalent of setting
	 * compute_query_id = auto).  This replaces the custom JumbleExpr/
	 * getQueryId logic from the previous version.
	 */
	EnableQueryId();

	DefineCustomStringVariable("hg_wait_states.directory",
							   "Location used to store hg_wait_states log files.",
							   "Defaults to $PGDATA/hg_wait_states.",
							   &hg_ws_directory,
							   "hg_wait_states",
							   PGC_POSTMASTER,
							   GUC_SUPERUSER_ONLY | GUC_NOT_IN_SAMPLE,
							   NULL, NULL, NULL);

	DefineCustomStringVariable("hg_wait_states.database",
							  "Database the hg_wait_states collector connects to.",
							  NULL,
							  &hg_ws_database,
							  "highgo",
							  PGC_POSTMASTER,
							  GUC_SUPERUSER_ONLY | GUC_NOT_IN_SAMPLE,
							  NULL, NULL, NULL);

	DefineCustomIntVariable("hg_wait_states.retention_period",
							"Log files older than this are removed automatically.",
							NULL,
							&hg_ws_retention_period,
							7,					/* default 7 days */
							1,					/* min 1 day, matches the historical limit */
							INT_MAX,
							PGC_SIGHUP,
							GUC_NOT_IN_SAMPLE,
							NULL, NULL, NULL);

	DefineCustomBoolVariable("hg_wait_states.enable_collection",
							 "Enable/disable wait-states data collection.",
							 NULL,
							 &hg_ws_enable_collection,
							 false,
							 PGC_SIGHUP,
							 GUC_NOT_IN_SAMPLE,
							 NULL, NULL, NULL);

#if PG_VERSION_NUM >= 160000
	MarkGUCPrefixReserved("hg_wait_states");

	prev_shmem_request_hook = shmem_request_hook;
	shmem_request_hook = hg_ws_shmem_request;
#else
	EmitWarningsOnPlaceholders("hg_wait_states");

	RequestAddinShmemSpace(hg_ws_shmem_size());
	RequestNamedLWLockTranche("hg_wait_states", 1);
#endif

	/* Hook shmem setup so we can place our coordination struct. */
	prev_shmem_startup_hook = shmem_startup_hook;
	shmem_startup_hook = hg_ws_shmem_startup;

	if (!IsUnderPostmaster)
		hg_ws_init_directory();

	if (strcmp(hg_ws_database,"") == 0)
		hg_ws_database = "highgo";

	hg_ws_register_worker();
}

/*-------------------------------------------------------------------------
 * Shared-memory helpers
 *-------------------------------------------------------------------------
 */
static Size
hg_ws_shmem_size(void)
{
	return MAXALIGN(sizeof(HgWsSharedState));
}

#if PG_VERSION_NUM >= 150000
static void
hg_ws_shmem_request(void)
{
	if (prev_shmem_request_hook)
		prev_shmem_request_hook();

	RequestAddinShmemSpace(hg_ws_shmem_size());
	RequestNamedLWLockTranche("hg_wait_states", 1);
}
#endif

static void
hg_ws_shmem_startup(void)
{
	bool		found;

	if (prev_shmem_startup_hook)
		prev_shmem_startup_hook();

	hg_ws_state = NULL;

	LWLockAcquire(AddinShmemInitLock, LW_EXCLUSIVE);
	hg_ws_state = ShmemInitStruct("hg_wait_states", hg_ws_shmem_size(), &found);
	if (!found)
	{
		hg_ws_state->lock = &(GetNamedLWLockTranche("hg_wait_states"))->lock;
		hg_ws_state->worker_pid = 0;
	}
	LWLockRelease(AddinShmemInitLock);
}

/*-------------------------------------------------------------------------
 * Signal handlers
 *-------------------------------------------------------------------------
 */
static void
hg_ws_sigterm(SIGNAL_ARGS)
{
	int			save_errno = errno;

	got_sigterm = true;
	if (MyProc)
		SetLatch(&MyProc->procLatch);

	errno = save_errno;
}

static void
hg_ws_sighup(SIGNAL_ARGS)
{
	int			save_errno = errno;

	got_sighup = true;
	if (MyProc)
		SetLatch(&MyProc->procLatch);

	errno = save_errno;
}

/*
 * proc_exit callback: clear the global worker PID so the next bgworker
 * restart isn't blocked by a stale value.
 */
static void
hg_ws_detach_shmem(int code, Datum arg)
{
	if (hg_ws_state == NULL)
		return;
	LWLockAcquire(hg_ws_state->lock, LW_EXCLUSIVE);
	if (hg_ws_state->worker_pid == MyProcPid)
		hg_ws_state->worker_pid = 0;
	LWLockRelease(hg_ws_state->lock);
}

/*-------------------------------------------------------------------------
 * Background worker registration
 *-------------------------------------------------------------------------
 */
static void
hg_ws_register_worker(void)
{
	BackgroundWorker worker;

	memset(&worker, 0, sizeof(worker));
	worker.bgw_flags = BGWORKER_SHMEM_ACCESS | BGWORKER_BACKEND_DATABASE_CONNECTION;
	worker.bgw_start_time = BgWorkerStart_RecoveryFinished;
	worker.bgw_restart_time = 60;
	snprintf(worker.bgw_library_name, BGW_MAXLEN, "hg_wait_states");
	snprintf(worker.bgw_function_name, BGW_MAXLEN, "hg_wait_states_main");
	snprintf(worker.bgw_name, BGW_MAXLEN, "hg_wait_states collector");
	snprintf(worker.bgw_type, BGW_MAXLEN, "hg_wait_states collector");
	worker.bgw_notify_pid = 0;
	worker.bgw_main_arg = (Datum) 0;

	RegisterBackgroundWorker(&worker);
}

/*-------------------------------------------------------------------------
 * Make sure hg_ws_directory exists and is a directory.  Errors are
 * downgraded to warning when the directory already exists.
 *-------------------------------------------------------------------------
 */
static void
hg_ws_init_directory(void)
{
	struct stat st;

	if (mkdir(hg_ws_directory, S_IRWXU) < 0)
	{
		if (errno != EEXIST)
			ereport(ERROR,
					(errcode_for_file_access(),
					 errmsg("could not create hg_wait_states directory \"%s\": %m",
							hg_ws_directory),
					 errhint("Make sure the server user can create this directory or that no file with the same name exists.")));
		if (stat(hg_ws_directory, &st) != 0 || !S_ISDIR(st.st_mode))
			ereport(ERROR,
					(errcode_for_file_access(),
					 errmsg("\"%s\" exists but is not a directory", hg_ws_directory)));
	}
}

/*-------------------------------------------------------------------------
 * Recognize a log file name and parse the embedded timestamps.
 *
 * A valid log file has the form
 *	   <prefix><created>_<rotate>[_<flag>]
 * with <created>/<rotate> non-negative decimal integers and an optional
 * decimal <flag> in the range [0, HG_WS_FILE_VERSION].  Anything else --
 * including trailing characters such as ".bak", whitespace or unrelated
 * suffixes -- is rejected so we never accidentally treat a backup or
 * temporary file as a log file (which would expose it to unlink during
 * purge).
 *-------------------------------------------------------------------------
 */
static bool
hg_ws_is_log_file(const char *name, const char *prefix,
				TimestampTz *created_ts, TimestampTz *rotate_ts, int *flag)
{
	size_t		plen = strlen(prefix);
	size_t		nlen;
	int64		c,
				r;
	int			f = 0;
	int			n;
	int			consumed = 0;

	if (strncmp(name, prefix, plen) != 0)
		return false;

	nlen = strlen(name);

	/*
	 * Try the v1.2 / v1.3 layout first (three numeric fields).  %n stores
	 * the number of characters consumed up to that point so we can verify
	 * that nothing else trails on the line.
	 */
	n = sscanf(name + plen,
			   INT64_FORMAT "_" INT64_FORMAT "_%d%n",
			   &c, &r, &f, &consumed);
	if (n == 3 && plen + (size_t) consumed == nlen)
	{
		/* Reject obviously-corrupt embedded values. */
		if (c < 0 || r < 0 || f < 0 || f > HG_WS_FILE_VERSION)
			return false;
		goto out;
	}

	/* Otherwise accept the historic v1.0 layout (two numeric fields). */
	consumed = 0;
	n = sscanf(name + plen,
			   INT64_FORMAT "_" INT64_FORMAT "%n",
			   &c, &r, &consumed);
	if (n == 2 && plen + (size_t) consumed == nlen)
	{
		if (c < 0 || r < 0)
			return false;
		f = 0;
		goto out;
	}

	return false;

out:
	if (created_ts)
		*created_ts = (TimestampTz) c;
	if (rotate_ts)
		*rotate_ts = (TimestampTz) r;
	if (flag)
		*flag = f;

	return true;
}

/*-------------------------------------------------------------------------
 * File creation / rotation
 *-------------------------------------------------------------------------
 */

/*
 * Flush userspace buffers and ask the kernel to push the data to durable
 * storage.  Called periodically from the bgworker main loop and right
 * before any rotation/shutdown that would close the FILE *, so a sudden
 * OS-level crash loses at most one fsync period (~30 s) worth of samples.
 *
 * Uses pg_fsync() rather than the bare fsync(2) so the cluster-wide
 * "fsync = off" GUC is respected -- the test harness relies on that to
 * stay fast.  Failures are logged at LOG level only: a partial fsync of
 * monitoring data is not worth dragging the collector down for.
 */
static void
hg_ws_fsync_file(HgWsLogFile *lf)
{
	if (lf == NULL || lf->fp == NULL)
		return;

	if (fflush(lf->fp) != 0)
	{
		ereport(LOG,
				(errcode_for_file_access(),
				 errmsg("could not flush hg_wait_states file \"%s\": %m",
						lf->path)));
		return;
	}

	if (pg_fsync(fileno(lf->fp)) != 0)
		ereport(LOG,
				(errcode_for_file_access(),
				 errmsg("could not fsync hg_wait_states file \"%s\": %m",
						lf->path)));
}

/*
 * Append a fully-assembled record to a log file as a single transaction.
 *
 * On any short or failed write the file is truncated back to its size
 * before the call, so a half-finished variable-length record can never
 * survive on disk and confuse the reader.  The function never returns on
 * failure -- it re-raises via ereport(ERROR), preserving the original
 * errno from fwrite.
 */
static void
hg_ws_atomic_write(HgWsLogFile *lf, const char *buf, size_t len)
{
	long		off_before;

	if (len == 0)
		return;

	off_before = ftell(lf->fp);
	if (off_before < 0)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not get current offset of hg_wait_states file \"%s\": %m",
						lf->path)));

	if (fwrite(buf, 1, len, lf->fp) == len)
		return;

	{
		int			save_errno = errno;

		clearerr(lf->fp);
		(void) fflush(lf->fp);
		if (ftruncate(fileno(lf->fp), (off_t) off_before) != 0)
			ereport(LOG,
					(errcode_for_file_access(),
					 errmsg("could not roll back partial write of hg_wait_states file \"%s\": %m",
							lf->path)));
		(void) fseek(lf->fp, 0, SEEK_END);
		errno = save_errno;
	}

	ereport(ERROR,
			(errcode_for_file_access(),
			 errmsg("could not write to hg_wait_states file \"%s\": %m",
					lf->path)));
}

static void
hg_ws_write_header(HgWsLogFile *lf)
{
	uint32		hdr[2];

	hdr[0] = HG_WS_FILE_MAGIC;
	hdr[1] = HG_WS_FILE_VERSION;
	if (fwrite(hdr, sizeof(hdr), 1, lf->fp) != 1)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not write header into hg_wait_states file \"%s\": %m",
						lf->path)));
	if (fflush(lf->fp) != 0)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not flush hg_wait_states file \"%s\": %m",
						lf->path)));
}

static bool
hg_ws_validate_header(FILE *fp, const char *path)
{
	uint32		hdr[2];

	if (fread(hdr, sizeof(hdr), 1, fp) != 1)
	{
		ereport(LOG,
				(errcode_for_file_access(),
				 errmsg("could not read header of hg_wait_states file \"%s\": %m",
						path)));
		return false;
	}
	if (hdr[0] != HG_WS_FILE_MAGIC)
	{
		ereport(LOG,
				(errmsg("invalid hg_wait_states magic in file \"%s\" (got 0x%08X, expected 0x%08X)",
						path, hdr[0], (unsigned int) HG_WS_FILE_MAGIC)));
		return false;
	}

	/*
	 * Refuse files written by a future, incompatible version.  We accept
	 * any version <= HG_WS_FILE_VERSION because older formats are read
	 * elsewhere via the per-file flag derived from the filename.
	 */
	if (hdr[1] > HG_WS_FILE_VERSION)
	{
		ereport(LOG,
				(errmsg("unsupported hg_wait_states file version in \"%s\" "
						"(got %u, expected <= %u)",
						path, hdr[1], (unsigned int) HG_WS_FILE_VERSION)));
		return false;
	}
	return true;
}

static void
hg_ws_create_log_file(HgWsLogFile *lf, const char *prefix, TimestampTz now)
{
	struct stat st;
	bool		pre_existing;
	int			n;

	if (lf->fp != NULL)
	{
		/*
		 * fclose() only flushes the userspace buffer; force the kernel to
		 * write our data to disk *before* we close the FILE * because the
		 * file we are about to rotate away may never be reopened.  Any
		 * fsync error is logged but does not abort the rotation: the
		 * kernel buffer will eventually be flushed, and the in-memory data
		 * is by definition non-essential.
		 */
		hg_ws_fsync_file(lf);
		if (fclose(lf->fp) != 0)
			ereport(WARNING,
					(errcode_for_file_access(),
					 errmsg("could not close hg_wait_states file \"%s\": %m",
							lf->path)));
		lf->fp = NULL;
	}

	n = snprintf(lf->name, sizeof(lf->name),
				 "%s" INT64_FORMAT "_" INT64_FORMAT "_%d",
				 prefix, (int64) now, (int64) lf->rotate_at, HG_WS_FILE_VERSION);
	if (n < 0 || (size_t) n >= sizeof(lf->name))
		ereport(ERROR,
				(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
				 errmsg("hg_wait_states file name is too long (prefix \"%s\")",
						prefix)));

	n = snprintf(lf->path, sizeof(lf->path), "%s/%s", hg_ws_directory, lf->name);
	if (n < 0 || (size_t) n >= sizeof(lf->path))
		ereport(ERROR,
				(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
				 errmsg("hg_wait_states file path is too long (directory \"%s\")",
						hg_ws_directory)));

	/*
	 * Detect an unlikely but real edge case: a file with the same fully-
	 * computed name already exists (e.g. left behind by a crash that
	 * happened in the same microsecond).  In append mode the new header
	 * would land in the middle of the existing file and corrupt readers.
	 * We log a clear warning and start fresh by truncating.
	 */
	pre_existing = (stat(lf->path, &st) == 0 && S_ISREG(st.st_mode) &&
					st.st_size > 0);
	if (pre_existing)
		ereport(WARNING,
				(errmsg("hg_wait_states file \"%s\" already exists; "
						"truncating to avoid mixing old and new records",
						lf->path)));

	/*
	 * "wb" truncates if the file exists.  This is intentionally different
	 * from the historic "a" mode: the timestamp embedded in the name is
	 * microsecond-precise, so a collision can only happen on a crash
	 * restart -- and in that case the safest behaviour is to start with a
	 * clean file rather than splice a new header into corrupt data.
	 */
	lf->fp = fopen(lf->path, "wb");
	if (lf->fp == NULL)
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not open hg_wait_states file \"%s\" for writing: %m",
						lf->path)));

	lf->created_ts = now;
	hg_ws_write_header(lf);
}

/*
 * Try to reopen the most recent file of the given kind so we can append to
 * it; otherwise create a brand new one.
 */
static void
hg_ws_open_or_create(HgWsLogFile *lf, const char *prefix, TimestampTz now,
				   int64 rotate_period_us)
{
	DIR		   *dir;
	struct dirent *de;
	TimestampTz best_created = PG_INT64_MIN;
	TimestampTz best_rotate = 0;
	char		best_name[MAXPGPATH] = "";
	bool		found = false;

	dir = AllocateDir(hg_ws_directory);
	while ((de = ReadDir(dir, hg_ws_directory)) != NULL)
	{
		TimestampTz cts,
					rts;

		if (de->d_name[0] == '.' &&
			(de->d_name[1] == '\0' ||
			 (de->d_name[1] == '.' && de->d_name[2] == '\0')))
			continue;

		if (!hg_ws_is_log_file(de->d_name, prefix, &cts, &rts, NULL))
			continue;

		if (timestamp_cmp_internal(cts, best_created) > 0)
		{
			best_created = cts;
			best_rotate = rts;
			strlcpy(best_name, de->d_name, sizeof(best_name));
			found = true;
		}
	}
	FreeDir(dir);

	if (found && timestamp_cmp_internal(best_rotate, now) > 0)
	{
		FILE	   *fp;
		char		path[MAXPGPATH];
		int			n;

		n = snprintf(path, sizeof(path), "%s/%s", hg_ws_directory, best_name);
		if (n < 0 || (size_t) n >= sizeof(path))
		{
			ereport(LOG,
					(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
					 errmsg("hg_wait_states file path is too long (\"%s/%s\")",
							hg_ws_directory, best_name)));
			goto fresh;
		}
		fp = fopen(path, "rb+");
		if (fp == NULL)
		{
			ereport(LOG,
					(errcode_for_file_access(),
					 errmsg("could not reopen hg_wait_states file \"%s\": %m",
							path)));
			found = false;
		}
		else if (!hg_ws_validate_header(fp, path))
		{
			fclose(fp);
			found = false;
		}
		else
		{
			/* Seek to end so we append new records. */
			if (fseek(fp, 0, SEEK_END) != 0)
			{
				ereport(LOG,
						(errcode_for_file_access(),
						 errmsg("could not seek hg_wait_states file \"%s\": %m",
								path)));
				fclose(fp);
				found = false;
			}
			else
			{
				lf->fp = fp;
				strlcpy(lf->path, path, sizeof(lf->path));
				strlcpy(lf->name, best_name, sizeof(lf->name));
				lf->created_ts = best_created;
				lf->rotate_at = best_rotate;
				return;
			}
		}
	}

fresh:
	/* Fall back to a fresh file. */
	lf->rotate_at = now + rotate_period_us;
	hg_ws_create_log_file(lf, prefix, now);
}

/*
 * Rebuild the in-memory deduplication hashes from an existing metadata file
 * so a collector restart can keep appending to that file without re-emitting
 * every session/query entry already stored in it.
 */
static void
hg_ws_seed_session_hash_from_file(const char *path, HTAB *session_hash)
{
	FILE	   *fp;
	int32		sid;

	fp = AllocateFile(path, "rb");
	if (fp == NULL)
	{
		ereport(LOG,
				(errcode_for_file_access(),
				 errmsg("could not open hg_wait_states sessions file \"%s\" for hash warm-up: %m",
						path)));
		return;
	}

	if (!hg_ws_validate_header(fp, path))
	{
		FreeFile(fp);
		return;
	}

	while (fread(&sid, sizeof(int32), 1, fp) == 1)
	{
		bool		found;
		int32		len;
		TimestampTz proc_start_ts = 0;
		HgWsSessionKey skey;

		if (fread(&len, sizeof(int32), 1, fp) != 1 ||
			len < 0 || len > HG_WS_MAX_STRING_LEN ||
			fseek(fp, len, SEEK_CUR) != 0 ||
			fread(&len, sizeof(int32), 1, fp) != 1 ||
			len < 0 || len > HG_WS_MAX_STRING_LEN ||
			fseek(fp, len, SEEK_CUR) != 0 ||
			fread(&len, sizeof(int32), 1, fp) != 1 ||
			len < 0 || len > HG_WS_MAX_STRING_LEN ||
			fseek(fp, len, SEEK_CUR) != 0 ||
			fread(&len, sizeof(int32), 1, fp) != 1 ||
			len < 0 || len > HG_WS_MAX_STRING_LEN ||
			fseek(fp, len, SEEK_CUR) != 0 ||
			fread(&proc_start_ts, sizeof(TimestampTz), 1, fp) != 1)
		{
			ereport(LOG,
					(errmsg("hg_wait_states sessions file \"%s\" appears truncated or corrupt during hash warm-up",
							path)));
			break;
		}

		hg_ws_init_session_key(&skey, sid, proc_start_ts);
		(void) hash_search(session_hash, &skey, HASH_ENTER, &found);
	}

	FreeFile(fp);
}

static void
hg_ws_seed_query_hash_from_file(const char *path, HTAB *query_hash)
{
	FILE	   *fp;
	int64		qid;

	fp = AllocateFile(path, "rb");
	if (fp == NULL)
	{
		ereport(LOG,
				(errcode_for_file_access(),
				 errmsg("could not open hg_wait_states queries file \"%s\" for hash warm-up: %m",
						path)));
		return;
	}

	if (!hg_ws_validate_header(fp, path))
	{
		FreeFile(fp);
		return;
	}

	while (fread(&qid, sizeof(int64), 1, fp) == 1)
	{
		bool		found;
		int32		qlen;

		(void) hash_search(query_hash, &qid, HASH_ENTER, &found);

		if (fread(&qlen, sizeof(int32), 1, fp) != 1 ||
			qlen < 0 || qlen > pgstat_track_activity_query_size ||
			fseek(fp, qlen, SEEK_CUR) != 0)
		{
			ereport(LOG,
					(errmsg("hg_wait_states queries file \"%s\" appears truncated or corrupt during hash warm-up",
							path)));
			break;
		}
	}

	FreeFile(fp);
}

static void
hg_ws_seed_session_hash_from_active_files(const char *prefix, TimestampTz now,
										HTAB *session_hash)
{
	DIR		   *dir;
	struct dirent *de;

	dir = AllocateDir(hg_ws_directory);
	while ((de = ReadDir(dir, hg_ws_directory)) != NULL)
	{
		TimestampTz cts,
					rts;
		char		path[MAXPGPATH];
		int			n;

		if (de->d_name[0] == '.' &&
			(de->d_name[1] == '\0' ||
			 (de->d_name[1] == '.' && de->d_name[2] == '\0')))
			continue;

		if (!hg_ws_is_log_file(de->d_name, prefix, &cts, &rts, NULL))
			continue;

		/* Only seed from metadata files that can still match future samples. */
		if (timestamp_cmp_internal(rts, now) <= 0)
			continue;

		n = snprintf(path, sizeof(path), "%s/%s", hg_ws_directory, de->d_name);
		if (n < 0 || (size_t) n >= sizeof(path))
		{
			ereport(LOG,
					(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
					 errmsg("hg_wait_states file path is too long during hash warm-up (\"%s/%s\")",
							hg_ws_directory, de->d_name)));
			continue;
		}

		hg_ws_seed_session_hash_from_file(path, session_hash);
	}
	FreeDir(dir);
}

static void
hg_ws_seed_query_hash_from_active_files(const char *prefix, TimestampTz now,
									  HTAB *query_hash)
{
	DIR		   *dir;
	struct dirent *de;

	dir = AllocateDir(hg_ws_directory);
	while ((de = ReadDir(dir, hg_ws_directory)) != NULL)
	{
		TimestampTz cts,
					rts;
		char		path[MAXPGPATH];
		int			n;

		if (de->d_name[0] == '.' &&
			(de->d_name[1] == '\0' ||
			 (de->d_name[1] == '.' && de->d_name[2] == '\0')))
			continue;

		if (!hg_ws_is_log_file(de->d_name, prefix, &cts, &rts, NULL))
			continue;

		/* Only seed from metadata files that can still match future samples. */
		if (timestamp_cmp_internal(rts, now) <= 0)
			continue;

		n = snprintf(path, sizeof(path), "%s/%s", hg_ws_directory, de->d_name);
		if (n < 0 || (size_t) n >= sizeof(path))
		{
			ereport(LOG,
					(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
					 errmsg("hg_wait_states file path is too long during hash warm-up (\"%s/%s\")",
							hg_ws_directory, de->d_name)));
			continue;
		}

		hg_ws_seed_query_hash_from_file(path, query_hash);
	}
	FreeDir(dir);
}

/*-------------------------------------------------------------------------
 * Purge old log files (used by the bgworker daily and by the
 * hg_wait_states_purge SQL function).
 *-------------------------------------------------------------------------
 */
static const char *const hg_ws_all_prefixes[] = {
	HG_WS_SESSIONS_PREFIX,
	HG_WS_QUERIES_PREFIX,
	HG_WS_SAMPLES_PREFIX,
	NULL
};

static void
hg_ws_purge_files(TimestampTz start_ts, TimestampTz end_ts)
{
	DIR		   *dir;
	struct dirent *de;

	dir = AllocateDir(hg_ws_directory);
	while ((de = ReadDir(dir, hg_ws_directory)) != NULL)
	{
		TimestampTz cts,
					rts;
		bool		match = false;

		if (de->d_name[0] == '.' &&
			(de->d_name[1] == '\0' ||
			 (de->d_name[1] == '.' && de->d_name[2] == '\0')))
			continue;

		for (int i = 0; hg_ws_all_prefixes[i] != NULL; i++)
		{
			if (hg_ws_is_log_file(de->d_name, hg_ws_all_prefixes[i],
								&cts, &rts, NULL))
			{
				match = true;
				break;
			}
		}
		if (!match)
			continue;

		/*
		 * The file is removable when its whole time window
		 * [created, rotate] sits inside [start_ts, end_ts].
		 */
		if (timestamp_cmp_internal(cts, start_ts) >= 0 &&
			timestamp_cmp_internal(rts, end_ts) <= 0)
		{
			char		path[MAXPGPATH];
			int			n;

			n = snprintf(path, sizeof(path), "%s/%s",
						 hg_ws_directory, de->d_name);
			if (n < 0 || (size_t) n >= sizeof(path))
			{
				ereport(WARNING,
						(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
						 errmsg("hg_wait_states file path is too long; skipping purge of \"%s\"",
								de->d_name)));
				continue;
			}
			if (unlink(path) != 0 && errno != ENOENT)
				ereport(WARNING,
						(errcode_for_file_access(),
						 errmsg("could not remove hg_wait_states file \"%s\": %m",
								path)));
		}
	}
	FreeDir(dir);
}

/*-------------------------------------------------------------------------
 * Bgworker main loop
 *-------------------------------------------------------------------------
 */

/* Hash entry types (HgWsSessionKey is declared next to the other on-disk
 * helpers because the warm-up routines also need it). */
typedef struct HgWsSessionHashEntry
{
	HgWsSessionKey key;
}			HgWsSessionHashEntry;

typedef struct HgWsQueryHashEntry
{
	int64		query_id;
}			HgWsQueryHashEntry;

static HTAB *
create_session_hash(MemoryContext cxt)
{
	HASHCTL		hctl;

	memset(&hctl, 0, sizeof(hctl));
	hctl.keysize = sizeof(HgWsSessionKey);
	hctl.entrysize = sizeof(HgWsSessionHashEntry);
	hctl.hcxt = cxt;

	return hash_create("hg_wait_states session-id hash", 1024, &hctl,
					   HASH_ELEM | HASH_BLOBS | HASH_CONTEXT);
}

static HTAB *
create_query_hash(MemoryContext cxt)
{
	HASHCTL		hctl;

	memset(&hctl, 0, sizeof(hctl));
	hctl.keysize = sizeof(int64);
	hctl.entrysize = sizeof(HgWsQueryHashEntry);
	hctl.hcxt = cxt;

	return hash_create("hg_wait_states query-id hash", 1024, &hctl,
					   HASH_ELEM | HASH_BLOBS | HASH_CONTEXT);
}

/*
 * One sampling pass: scan PgBackendStatus and append per-backend records
 * to the three "per-backend" log files.
 */
static void
hg_ws_collect_once(TimestampTz now,
				 HgWsLogFile *sessions, HgWsLogFile *queries,
				 HgWsLogFile *samples,
				 HTAB *session_hash, HTAB *query_hash)
{
	int			num_backends;

	num_backends = pgstat_fetch_stat_numbackends();

	for (int i = 1; i <= num_backends; i++)
	{
		LocalPgBackendStatus *local;
		PgBackendStatus *be;
		PGPROC	   *proc;
		HgWsSampleRecord rec;
		bool		found_session;
		bool		found_query;
		int64		qid;

		CHECK_FOR_INTERRUPTS();

#if PG_VERSION_NUM >= 150000
		local = pgstat_get_local_beentry_by_index(i);
#else
		local = pgstat_fetch_stat_local_beentry(i);;
#endif
		if (local == NULL)
			continue;
		be = &local->backendStatus;

		/* skip slots that are not in use */
		if (be->st_procpid == 0)
			continue;
		/* only consider regular client backends */
		if (be->st_backendType != B_BACKEND)
			continue;

		qid = (int64) be->st_query_id;

		/*
		 * Only sample a backend that is actually executing a query
		 * (state != idle) and has a non-zero query identifier.
		 */
		if (be->st_state == STATE_IDLE || qid == 0)
			continue;

		/*
		 * Look up the corresponding PGPROC entry only after we have decided
		 * that the backend is worth sampling.  BackendIdGetProc is O(1) and
		 * cheaper than BackendPidGetProc which scans the whole ProcArray.
		 */
		proc = BackendPidGetProc(be->st_procpid);
		if (proc == NULL)
			continue;

		/*
		 * Build the sample record (one row per active backend per cycle).
		 * Do this early because if found_session is false, we will start and
		 * commit a catalog transaction. Committing clears the pgstat snapshot
		 * and frees 'be', making it a dangling pointer.
		 */
		MemSet(&rec, 0, sizeof(rec));
		rec.query_id = qid;
		rec.sample_time = now;

		/*
		 * Read wait_event_info atomically with a volatile pointer.  The
		 * field is updated lock-free by other backends; on weak-memory
		 * architectures this prevents the compiler from issuing two
		 * separate loads that could observe inconsistent half-words.
		 */
		rec.wait_event_info = *((volatile uint32 *) &proc->wait_event_info);

		rec.session_id = (int32) be->st_procpid;
		rec.query_start_time = be->st_activity_start_timestamp;
		rec.sampling_interval = HG_WS_SAMPLING_INTERVAL_SEC;

		/*
		 * Remember (once) every fresh query string we see.  We probe with
		 * HASH_FIND and only insert the entry *after* the write succeeded,
		 * so a transient I/O failure cannot leave the in-memory hash and
		 * the on-disk queries file out of sync (otherwise the qid would be
		 * remembered as written even though the record never reached the
		 * file).
		 */
		if (be->st_activity_raw != NULL && be->st_activity_raw[0] != '\0')
		{
			(void) hash_search(query_hash, &qid, HASH_FIND, &found_query);
			if (!found_query)
			{
				int32		qlen = (int32) strlen(be->st_activity_raw);
				StringInfoData buf;

				if (qlen > pgstat_track_activity_query_size)
					qlen = pgstat_track_activity_query_size;

				/*
				 * Serialize the whole record into a single buffer and write
				 * it in one fwrite.  hg_ws_atomic_write rolls the file back
				 * to its prior size if the write fails partway through, so a
				 * half-record can never survive on disk.
				 */
				initStringInfo(&buf);
				appendBinaryStringInfoNT(&buf, (char *) &qid, sizeof(int64));
				appendBinaryStringInfoNT(&buf, (char *) &qlen, sizeof(int32));
				if (qlen > 0)
					appendBinaryStringInfoNT(&buf, be->st_activity_raw, qlen);

				hg_ws_atomic_write(queries, buf.data, buf.len);
				pfree(buf.data);

				(void) hash_search(query_hash, &qid, HASH_ENTER, &found_query);
			}
		}

		/*
		 * Remember (once) every session.  The hash key is (pid,
		 * proc_start_timestamp) so a recycled OS pid that belongs to a
		 * different backend is correctly emitted as a brand new session
		 * record rather than being silently aliased to the previous owner.
		 */
		{
			HgWsSessionKey skey;

			hg_ws_init_session_key(&skey, (int32) be->st_procpid,
								   be->st_proc_start_timestamp);

			(void) hash_search(session_hash, &skey, HASH_FIND, &found_session);
			if (!found_session)
			{
				char	   *dbname;
				char	   *uname;
				const char *appname = be->st_appname ? be->st_appname : "";
				const char *chost = be->st_clienthostname ? be->st_clienthostname : "";
				TimestampTz proc_start_ts = be->st_proc_start_timestamp;
				int32		sid_on_disk = skey.session_id;
				int32		dlen,
							ulen,
							alen,
							hlen;
				StringInfoData buf;

				StartTransactionCommand();
				dbname = be->st_databaseid != InvalidOid ?
					get_database_name(be->st_databaseid) : pstrdup("");
				if (dbname == NULL)
					dbname = pstrdup("");
				uname = be->st_userid != InvalidOid ?
					GetUserNameFromId(be->st_userid, true) : pstrdup("");
				if (uname == NULL)
					uname = pstrdup("");

				/*
				 * Clamp every variable-length field to a sane upper bound so
				 * that a corrupt source string can never grow the file by an
				 * unbounded amount and can never trigger the corrupt-data
				 * branch in the reader.
				 */
				dlen = (int32) strlen(dbname);
				ulen = (int32) strlen(uname);
				alen = (int32) strlen(appname);
				hlen = (int32) strlen(chost);
				if (dlen > HG_WS_MAX_STRING_LEN)
					dlen = HG_WS_MAX_STRING_LEN;
				if (ulen > HG_WS_MAX_STRING_LEN)
					ulen = HG_WS_MAX_STRING_LEN;
				if (alen > HG_WS_MAX_STRING_LEN)
					alen = HG_WS_MAX_STRING_LEN;
				if (hlen > HG_WS_MAX_STRING_LEN)
					hlen = HG_WS_MAX_STRING_LEN;

				/* Build the record once, write it atomically. */
				initStringInfo(&buf);
				appendBinaryStringInfoNT(&buf, (char *) &sid_on_disk, sizeof(int32));
				appendBinaryStringInfoNT(&buf, (char *) &dlen, sizeof(int32));
				if (dlen > 0)
					appendBinaryStringInfoNT(&buf, dbname, dlen);
				appendBinaryStringInfoNT(&buf, (char *) &ulen, sizeof(int32));
				if (ulen > 0)
					appendBinaryStringInfoNT(&buf, uname, ulen);
				appendBinaryStringInfoNT(&buf, (char *) &alen, sizeof(int32));
				if (alen > 0)
					appendBinaryStringInfoNT(&buf, appname, alen);
				appendBinaryStringInfoNT(&buf, (char *) &hlen, sizeof(int32));
				if (hlen > 0)
					appendBinaryStringInfoNT(&buf, chost, hlen);
				appendBinaryStringInfoNT(&buf, (char *) &proc_start_ts,
										 sizeof(TimestampTz));

				hg_ws_atomic_write(sessions, buf.data, buf.len);
				pfree(buf.data);

				CommitTransactionCommand();

				(void) hash_search(session_hash, &skey, HASH_ENTER,
								   &found_session);
			}
		}

		hg_ws_atomic_write(samples, (const char *) &rec, sizeof(rec));
	}

	fflush(samples->fp);
	fflush(sessions->fp);
	fflush(queries->fp);

	pgstat_clear_snapshot();
}

void
hg_wait_states_main(Datum main_arg)
{
	MemoryContext sample_ctx;
	HgWsLogFile	sessions = {0};
	HgWsLogFile	queries = {0};
	HgWsLogFile	samples = {0};
	HTAB	   *session_hash;
	HTAB	   *query_hash;
	TimestampTz now;
	TimestampTz next_sample;
	TimestampTz next_purge;
	TimestampTz next_fsync;

	pqsignal(SIGTERM, hg_ws_sigterm);
	pqsignal(SIGHUP, hg_ws_sighup);
	BackgroundWorkerUnblockSignals();

	if (hg_ws_state == NULL)
		ereport(ERROR,
				(errmsg("hg_wait_states shared state was not initialized; "
						"is the module listed in shared_preload_libraries?")));

	/*
	 * Ensure the worker_pid is reset whenever we exit (clean or otherwise),
	 * so that a future restart isn't blocked by a stale value.
	 */
	on_shmem_exit(hg_ws_detach_shmem, (Datum) 0);

	/* register ourselves, refuse to run if another collector is alive */
	LWLockAcquire(hg_ws_state->lock, LW_EXCLUSIVE);
	if (hg_ws_state->worker_pid != 0)
	{
		pid_t		other = hg_ws_state->worker_pid;

		LWLockRelease(hg_ws_state->lock);
		ereport(LOG,
				(errmsg("hg_wait_states collector already running under PID %d",
						(int) other)));
		return;
	}
	hg_ws_state->worker_pid = MyProcPid;
	LWLockRelease(hg_ws_state->lock);

	ereport(LOG, (errmsg("hg_wait_states collector started")));

	BackgroundWorkerInitializeConnection(hg_ws_database, NULL, 0);

	sample_ctx = AllocSetContextCreate(TopMemoryContext,
									   "hg_wait_states sampler",
									   ALLOCSET_DEFAULT_SIZES);

	session_hash = create_session_hash(sample_ctx);
	query_hash = create_query_hash(sample_ctx);

	now = GetCurrentTimestamp();
	hg_ws_open_or_create(&sessions, HG_WS_SESSIONS_PREFIX, now, HG_WS_SESSIONS_ROTATE_US);
	hg_ws_open_or_create(&queries, HG_WS_QUERIES_PREFIX, now, HG_WS_QUERIES_ROTATE_US);
	hg_ws_open_or_create(&samples, HG_WS_SAMPLES_PREFIX, now, HG_WS_SAMPLES_ROTATE_US);
	hg_ws_seed_session_hash_from_active_files(HG_WS_SESSIONS_PREFIX, now,
											session_hash);
	hg_ws_seed_query_hash_from_active_files(HG_WS_QUERIES_PREFIX, now,
										  query_hash);

	next_sample = now + (int64) HG_WS_SAMPLING_INTERVAL_SEC * USECS_PER_SEC;
	next_purge = now + HG_WS_PURGE_PERIOD_US;
	next_fsync = now + HG_WS_FSYNC_PERIOD_US;

	while (!got_sigterm)
	{
		long		wait_ms;
		int			rc;

		CHECK_FOR_INTERRUPTS();

		if (got_sighup)
		{
			got_sighup = false;
			ProcessConfigFile(PGC_SIGHUP);
		}

		now = GetCurrentTimestamp();

		/* Rotate any log file whose window has closed. */
		if (timestamp_cmp_internal(samples.rotate_at, now) <= 0)
		{
			samples.rotate_at = now + HG_WS_SAMPLES_ROTATE_US;
			hg_ws_create_log_file(&samples, HG_WS_SAMPLES_PREFIX, now);
		}
		if (timestamp_cmp_internal(sessions.rotate_at, now) <= 0)
		{
			/* drop the cache so we re-emit every session in the new file */
			hash_destroy(session_hash);
			session_hash = create_session_hash(sample_ctx);
			sessions.rotate_at = now + HG_WS_SESSIONS_ROTATE_US;
			hg_ws_create_log_file(&sessions, HG_WS_SESSIONS_PREFIX, now);
		}
		if (timestamp_cmp_internal(queries.rotate_at, now) <= 0)
		{
			hash_destroy(query_hash);
			query_hash = create_query_hash(sample_ctx);
			queries.rotate_at = now + HG_WS_QUERIES_ROTATE_US;
			hg_ws_create_log_file(&queries, HG_WS_QUERIES_PREFIX, now);
		}

		if (hg_ws_enable_collection &&
			timestamp_cmp_internal(next_sample, now) <= 0)
		{
			MemoryContext old = CurrentMemoryContext;
			TimestampTz cycle_start = now;
			TimestampTz cycle_end;
			int64		cycle_us;

			PG_TRY();
			{
				hg_ws_collect_once(now, &sessions, &queries, &samples,
								 session_hash, query_hash);
			}
			PG_CATCH();
			{
				/*
				 * Don't let a transient error tear the worker down -- log it
				 * and continue.  Note however that hg_ws_collect_once leaves
				 * its files open so we can keep using them.
				 */
				MemoryContextSwitchTo(old);
				EmitErrorReport();
				FlushErrorState();
				AbortCurrentTransaction();
			}
			PG_END_TRY();

			/*
			 * If the collection cycle itself took longer than the fixed
			 * 1-second sampling interval, warn the operator
			 */
			cycle_end = GetCurrentTimestamp();
			cycle_us = cycle_end - cycle_start;
			if (cycle_us > (int64) HG_WS_SAMPLING_INTERVAL_SEC * USECS_PER_SEC)
				ereport(LOG,
						(errmsg("hg_wait_states sampling cycle took %ld ms, "
								"which exceeds the fixed sampling_interval (%d s)",
								(long) (cycle_us / 1000), HG_WS_SAMPLING_INTERVAL_SEC)));

			/*
			 * Drift compensation: advance the next deadline from the *target*
			 * time we just fired at, not from the wall-clock time after the
			 * cycle finished.  This keeps sample_time on a stable cadence
			 * (every `sampling_interval` seconds since the worker started)
			 * even when a cycle takes tens of milliseconds.
			 *
			 * If a cycle was so long, or the worker was so slow to wake,
			 * that we ended up more than one full interval behind, snap to
			 * the next interval boundary after `now` instead of letting the
			 * next iteration fire immediately and accumulate burst debt --
			 * the operator already got the warning above.
			 */
			{
				int64		interval_us = (int64) HG_WS_SAMPLING_INTERVAL_SEC *
											USECS_PER_SEC;

				next_sample += interval_us;
				if (next_sample <= cycle_end)
					next_sample = cycle_end + interval_us;
			}
		}

		if (timestamp_cmp_internal(next_purge, now) <= 0)
		{
			TimestampTz purge_before =
				now - (int64) hg_ws_retention_period * SECS_PER_DAY * USECS_PER_SEC;

			hg_ws_purge_files(PG_INT64_MIN, purge_before);
			next_purge = now + HG_WS_PURGE_PERIOD_US;
		}

		/*
		 * Periodically push buffered data all the way down to durable
		 * storage so an OS-level crash loses at most one fsync period
		 * (HG_WS_FSYNC_PERIOD_US) worth of samples, while rotation and
		 * shutdown each trigger an additional fsync of their own.
		 */
		if (timestamp_cmp_internal(next_fsync, now) <= 0)
		{
			hg_ws_fsync_file(&samples);
			hg_ws_fsync_file(&sessions);
			hg_ws_fsync_file(&queries);
			next_fsync = now + HG_WS_FSYNC_PERIOD_US;
		}

		/*
		 * Sleep until the next interesting event.  When several deadlines
		 * are due we want to wake up as soon as possible, even if the
		 * requested wait drops to zero, so the loop spins through the
		 * pending work without artificial delay.  Refresh `now` here:
		 * collect_once() can itself burn several hundred milliseconds in
		 * pgstat snapshot and catalog lookups, so using the loop-top
		 * timestamp would over-sleep by the same amount and silently
		 * widen the sampling cadence.
		 */
		{
			TimestampTz wake = next_sample;
			TimestampTz wake_now;

			if (timestamp_cmp_internal(samples.rotate_at, wake) < 0)
				wake = samples.rotate_at;
			if (timestamp_cmp_internal(sessions.rotate_at, wake) < 0)
				wake = sessions.rotate_at;
			if (timestamp_cmp_internal(queries.rotate_at, wake) < 0)
				wake = queries.rotate_at;
			if (timestamp_cmp_internal(next_purge, wake) < 0)
				wake = next_purge;
			if (timestamp_cmp_internal(next_fsync, wake) < 0)
				wake = next_fsync;

			wake_now = GetCurrentTimestamp();
			if (wake <= wake_now)
				wait_ms = 0;
			else
				wait_ms = (long) ((wake - wake_now) / 1000);
			if (wait_ms < 0)
				wait_ms = 0;
		}

		rc = WaitLatch(MyLatch,
					   WL_LATCH_SET | WL_TIMEOUT | WL_POSTMASTER_DEATH,
					   wait_ms, PG_WAIT_EXTENSION);
		ResetLatch(MyLatch);

		if (rc & WL_POSTMASTER_DEATH)
			proc_exit(1);
	}

	/*
	 * Tidy up before exiting.  fsync first so a clean shutdown leaves no
	 * unflushed data on the host: tests that restart the cluster and look
	 * at log files immediately rely on this.
	 */
	if (samples.fp)
	{
		hg_ws_fsync_file(&samples);
		fclose(samples.fp);
	}
	if (sessions.fp)
	{
		hg_ws_fsync_file(&sessions);
		fclose(sessions.fp);
	}
	if (queries.fp)
	{
		hg_ws_fsync_file(&queries);
		fclose(queries.fp);
	}

	LWLockAcquire(hg_ws_state->lock, LW_EXCLUSIVE);
	hg_ws_state->worker_pid = 0;
	LWLockRelease(hg_ws_state->lock);

	proc_exit(0);
}

/*-------------------------------------------------------------------------
 * Reader helpers (used by the SQL-callable functions below)
 *-------------------------------------------------------------------------
 */

/*
 * Open a file in the log directory and skip its 8-byte header, returning
 * NULL on failure (logged via ereport(LOG)).
 */
static FILE *
hg_ws_open_for_read(const char *name, int has_header)
{
	char		path[MAXPGPATH];
	FILE	   *fp;
	int			n;

	n = snprintf(path, sizeof(path), "%s/%s", hg_ws_directory, name);
	if (n < 0 || (size_t) n >= sizeof(path))
	{
		ereport(LOG,
				(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
				 errmsg("hg_wait_states file path is too long; skipping \"%s\"",
						name)));
		return NULL;
	}
	fp = AllocateFile(path, "rb");
	if (fp == NULL)
	{
		ereport(LOG,
				(errcode_for_file_access(),
				 errmsg("could not open hg_wait_states file \"%s\": %m", path)));
		return NULL;
	}
	if (has_header && !hg_ws_validate_header(fp, path))
	{
		FreeFile(fp);
		return NULL;
	}
	return fp;
}

static void
read_samples(ReturnSetInfo *rsinfo, TimestampTz start_ts, TimestampTz end_ts,
			 bool with_interval)
{
	DIR		   *dir;
	struct dirent *de;

	dir = AllocateDir(hg_ws_directory);
	while ((de = ReadDir(dir, hg_ws_directory)) != NULL)
	{
		TimestampTz cts,
					rts;
		int			flag;
		FILE	   *fp;
		HgWsSampleRecord rec;

		if (de->d_name[0] == '.' &&
			(de->d_name[1] == '\0' ||
			 (de->d_name[1] == '.' && de->d_name[2] == '\0')))
			continue;
		if (!hg_ws_is_log_file(de->d_name, HG_WS_SAMPLES_PREFIX, &cts, &rts, &flag))
			continue;
		/*
		 * Skip files entirely outside [start_ts, end_ts).  Use >= for the
		 * upper bound (cts == end_ts means the file has nothing in window
		 * because rec.sample_time >= cts = end_ts is rejected below).
		 */
		if (timestamp_cmp_internal(cts, end_ts) >= 0 ||
			timestamp_cmp_internal(rts, start_ts) <= 0)
			continue;

		fp = hg_ws_open_for_read(de->d_name, true);
		if (fp == NULL)
			continue;

		while (fread(&rec, sizeof(rec), 1, fp) == 1)
		{
			Datum		values[7];
			bool		nulls[7];
			const char *wt;
			const char *we;

			CHECK_FOR_INTERRUPTS();

			/*
			 * Samples are appended in sampling-cycle order, so sample_time
			 * is monotonically non-decreasing within a single file.  Once
			 * we cross the upper bound we can stop reading this file
			 * altogether instead of scanning the rest to no purpose.  The
			 * SQLs in the PEM templates (sql-2..sql-6, sql-10..sql-12) hit
			 * read_samples with very narrow [t-1, t+1] windows where this
			 * early-break is the dominant cost win.
			 */
			if (timestamp_cmp_internal(rec.sample_time, end_ts) >= 0)
				break;
			if (timestamp_cmp_internal(rec.sample_time, start_ts) < 0)
				continue;

			MemSet(nulls, false, sizeof(nulls));
			values[0] = Int64GetDatum(rec.query_id);
			values[1] = Int32GetDatum(rec.session_id);
			values[2] = TimestampTzGetDatum(rec.query_start_time);
			values[3] = TimestampTzGetDatum(rec.sample_time);

			wt = pgstat_get_wait_event_type(rec.wait_event_info);
			we = pgstat_get_wait_event(rec.wait_event_info);
			if (wt == NULL)
				nulls[4] = true;
			else
				values[4] = CStringGetTextDatum(wt);
			if (we == NULL)
				nulls[5] = true;
			else
				values[5] = CStringGetTextDatum(we);

			if (with_interval)
				values[6] = Int32GetDatum(rec.sampling_interval);

			tuplestore_putvalues(rsinfo->setResult, rsinfo->setDesc,
								 values, nulls);
		}

		FreeFile(fp);
	}
	FreeDir(dir);
}

static void
read_sessions(ReturnSetInfo *rsinfo, TimestampTz start_ts, TimestampTz end_ts,
			  bool extended)
{
	DIR		   *dir;
	struct dirent *de;

	dir = AllocateDir(hg_ws_directory);
	while ((de = ReadDir(dir, hg_ws_directory)) != NULL)
	{
		TimestampTz cts,
					rts;
		int			flag;
		FILE	   *fp;
		int32		sid;
		bool		corrupt = false;

		if (de->d_name[0] == '.' &&
			(de->d_name[1] == '\0' ||
			 (de->d_name[1] == '.' && de->d_name[2] == '\0')))
			continue;
		if (!hg_ws_is_log_file(de->d_name, HG_WS_SESSIONS_PREFIX, &cts, &rts, &flag))
			continue;
		if (timestamp_cmp_internal(cts, end_ts) >= 0 ||
			timestamp_cmp_internal(rts, start_ts) <= 0)
			continue;

		fp = hg_ws_open_for_read(de->d_name, flag == HG_WS_FILE_VERSION);
		if (fp == NULL)
			continue;

		while (fread(&sid, sizeof(int32), 1, fp) == 1)
		{
			int32		dlen,
						ulen,
						alen = 0,
						hlen = 0;
			char	   *dbname = NULL;
			char	   *uname = NULL;
			char	   *appname = NULL;
			char	   *chost = NULL;
			TimestampTz session_start_ts = 0;
			Datum		values[8];
			bool		nulls[8];

			CHECK_FOR_INTERRUPTS();

			/* dbname */
			if (fread(&dlen, sizeof(int32), 1, fp) != 1)
				break;
			if (dlen < 0 || dlen > HG_WS_MAX_STRING_LEN)
			{
				corrupt = true;
				break;
			}
			dbname = palloc(dlen + 1);
			if (dlen > 0 && fread(dbname, 1, dlen, fp) != (size_t) dlen)
				break;
			dbname[dlen] = '\0';

			/* username */
			if (fread(&ulen, sizeof(int32), 1, fp) != 1)
				break;
			if (ulen < 0 || ulen > HG_WS_MAX_STRING_LEN)
			{
				corrupt = true;
				break;
			}
			uname = palloc(ulen + 1);
			if (ulen > 0 && fread(uname, 1, ulen, fp) != (size_t) ulen)
				break;
			uname[ulen] = '\0';

			if (flag == HG_WS_FILE_VERSION)
			{
				if (fread(&alen, sizeof(int32), 1, fp) != 1)
					break;
				if (alen < 0 || alen > HG_WS_MAX_STRING_LEN)
				{
					corrupt = true;
					break;
				}
				appname = palloc(alen + 1);
				if (alen > 0 && fread(appname, 1, alen, fp) != (size_t) alen)
					break;
				appname[alen] = '\0';

				if (fread(&hlen, sizeof(int32), 1, fp) != 1)
					break;
				if (hlen < 0 || hlen > HG_WS_MAX_STRING_LEN)
				{
					corrupt = true;
					break;
				}
				chost = palloc(hlen + 1);
				if (hlen > 0 && fread(chost, 1, hlen, fp) != (size_t) hlen)
					break;
				chost[hlen] = '\0';

				if (fread(&session_start_ts, sizeof(TimestampTz), 1, fp) != 1)
					break;
			}

			CHECK_FOR_INTERRUPTS();

			MemSet(nulls, false, sizeof(nulls));
			values[0] = Int32GetDatum(sid);
			values[1] = CStringGetTextDatum(dbname);
			values[2] = CStringGetTextDatum(uname);
			values[3] = TimestampTzGetDatum(cts);
			values[4] = TimestampTzGetDatum(rts);
			if (extended)
			{
				values[5] = CStringGetTextDatum(appname ? appname : "");
				values[6] = CStringGetTextDatum(chost ? chost : "");
				values[7] = TimestampTzGetDatum(session_start_ts);
			}
			tuplestore_putvalues(rsinfo->setResult, rsinfo->setDesc,
								 values, nulls);

			pfree(dbname);
			pfree(uname);
			if (appname)
				pfree(appname);
			if (chost)
				pfree(chost);
		}

		if (corrupt)
			ereport(LOG,
					(errmsg("hg_wait_states sessions file \"%s\" appears truncated or corrupt; ignoring the rest",
							de->d_name)));

		FreeFile(fp);
	}
	FreeDir(dir);
}

static void
read_queries(ReturnSetInfo *rsinfo, TimestampTz start_ts, TimestampTz end_ts)
{
	DIR		   *dir;
	struct dirent *de;

	dir = AllocateDir(hg_ws_directory);
	while ((de = ReadDir(dir, hg_ws_directory)) != NULL)
	{
		TimestampTz cts,
					rts;
		int			flag;
		FILE	   *fp;
		int64		qid;
		int32		qlen;
		char	   *qbuf;
		bool		corrupt = false;

		if (de->d_name[0] == '.' &&
			(de->d_name[1] == '\0' ||
			 (de->d_name[1] == '.' && de->d_name[2] == '\0')))
			continue;
		if (!hg_ws_is_log_file(de->d_name, HG_WS_QUERIES_PREFIX, &cts, &rts, &flag))
			continue;
		if (timestamp_cmp_internal(cts, end_ts) >= 0 ||
			timestamp_cmp_internal(rts, start_ts) <= 0)
			continue;

		fp = hg_ws_open_for_read(de->d_name, flag == HG_WS_FILE_VERSION);
		if (fp == NULL)
			continue;

		qbuf = palloc(pgstat_track_activity_query_size + 1);

		while (fread(&qid, sizeof(int64), 1, fp) == 1)
		{
			Datum		values[4];
			bool		nulls[4];

			if (fread(&qlen, sizeof(int32), 1, fp) != 1)
				break;
			if (qlen < 0 || qlen > pgstat_track_activity_query_size)
			{
				corrupt = true;
				break;
			}
			if (qlen > 0 && fread(qbuf, 1, qlen, fp) != (size_t) qlen)
				break;
			qbuf[qlen] = '\0';

			CHECK_FOR_INTERRUPTS();

			MemSet(nulls, false, sizeof(nulls));
			values[0] = Int64GetDatum(qid);
			values[1] = CStringGetTextDatum(qbuf);
			values[2] = TimestampTzGetDatum(cts);
			values[3] = TimestampTzGetDatum(rts);
			tuplestore_putvalues(rsinfo->setResult, rsinfo->setDesc,
								 values, nulls);
		}

		if (corrupt)
			ereport(LOG,
					(errmsg("hg_wait_states queries file \"%s\" appears truncated or corrupt; ignoring the rest",
							de->d_name)));

		pfree(qbuf);
		FreeFile(fp);
	}
	FreeDir(dir);
}

static int64
hg_ws_dir_size(TimestampTz start_ts, TimestampTz end_ts)
{
	DIR		   *dir;
	struct dirent *de;
	int64		total = 0;

	dir = AllocateDir(hg_ws_directory);
	while ((de = ReadDir(dir, hg_ws_directory)) != NULL)
	{
		TimestampTz cts,
					rts;
		bool		match = false;
		struct stat st;
		char		path[MAXPGPATH];

		if (de->d_name[0] == '.' &&
			(de->d_name[1] == '\0' ||
			 (de->d_name[1] == '.' && de->d_name[2] == '\0')))
			continue;
		for (int i = 0; hg_ws_all_prefixes[i] != NULL; i++)
		{
			if (hg_ws_is_log_file(de->d_name, hg_ws_all_prefixes[i],
								&cts, &rts, NULL))
			{
				match = true;
				break;
			}
		}
		if (!match)
			continue;
		if (timestamp_cmp_internal(cts, end_ts) >= 0 ||
			timestamp_cmp_internal(rts, start_ts) <= 0)
			continue;

		{
			int		n = snprintf(path, sizeof(path), "%s/%s",
								 hg_ws_directory, de->d_name);

			if (n < 0 || (size_t) n >= sizeof(path))
				continue;		/* path too long, skip silently */
		}
		if (stat(path, &st) == 0)
			total += (int64) st.st_size;
	}
	FreeDir(dir);
	return total;
}

/*-------------------------------------------------------------------------
 * SQL-callable wrappers
 *-------------------------------------------------------------------------
 */

/*
 * Make sure the module has been initialised via shared_preload_libraries
 * before we touch any of its global state.
 */
static void
hg_ws_ensure_loaded(void)
{
	if (hg_ws_directory == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("hg_wait_states must be loaded via shared_preload_libraries")));
}

static void
hg_ws_init_srf(FunctionCallInfo fcinfo, ReturnSetInfo **rsinfo_out)
{
	hg_ws_ensure_loaded();

	/*
	 * InitMaterializedSRF already performs the standard checks for SRFs in
	 * materialize mode and sets up rsinfo->setResult / setDesc.
	 */
#if PG_VERSION_NUM >= 160000
	InitMaterializedSRF(fcinfo, 0);
#elif PG_VERSION_NUM >= 150000
	SetSingleFunctionCall(fcinfo, 0);
#else
	{
		ReturnSetInfo	*rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
		MemoryContext	old_context;
		MemoryContext	per_query_ctx;
		TupleDesc	stored_tupdesc;
		bool		random_access;

		if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("set-valued function called in context that cannot accept a set")));
		if (!(rsinfo->allowedModes & SFRM_Materialize))
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("materialize mode required, but it is not allowed in this context")));

		per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
		old_context = MemoryContextSwitchTo(per_query_ctx);

		if (get_call_result_type(fcinfo, NULL, &stored_tupdesc) != TYPEFUNC_COMPOSITE)
			elog(ERROR, "return type must be a row type");

		random_access = (rsinfo->allowedModes & SFRM_Materialize_Random) != 0;

		rsinfo->returnMode = SFRM_Materialize;
		rsinfo->setResult = tuplestore_begin_heap(random_access, false, work_mem);
		rsinfo->setDesc = stored_tupdesc;
		MemoryContextSwitchTo(old_context);
	}
#endif
	*rsinfo_out = (ReturnSetInfo *) fcinfo->resultinfo;
}

Datum
hg_wait_states_samples(PG_FUNCTION_ARGS)
{
	TimestampTz start_ts = PG_GETARG_TIMESTAMPTZ(0);
	TimestampTz end_ts = PG_GETARG_TIMESTAMPTZ(1);
	ReturnSetInfo *rsinfo;

	hg_ws_init_srf(fcinfo, &rsinfo);
	read_samples(rsinfo, start_ts, end_ts, false);
	return (Datum) 0;
}

Datum
hg_wait_states_samples_internal(PG_FUNCTION_ARGS)
{
	TimestampTz start_ts = PG_GETARG_TIMESTAMPTZ(0);
	TimestampTz end_ts = PG_GETARG_TIMESTAMPTZ(1);
	ReturnSetInfo *rsinfo;

	hg_ws_init_srf(fcinfo, &rsinfo);
	read_samples(rsinfo, start_ts, end_ts, true);
	return (Datum) 0;
}

Datum
hg_wait_states_sessions(PG_FUNCTION_ARGS)
{
	TimestampTz start_ts = PG_GETARG_TIMESTAMPTZ(0);
	TimestampTz end_ts = PG_GETARG_TIMESTAMPTZ(1);
	ReturnSetInfo *rsinfo;

	hg_ws_init_srf(fcinfo, &rsinfo);
	read_sessions(rsinfo, start_ts, end_ts, true);
	return (Datum) 0;
}

Datum
hg_wait_states_queries(PG_FUNCTION_ARGS)
{
	TimestampTz start_ts = PG_GETARG_TIMESTAMPTZ(0);
	TimestampTz end_ts = PG_GETARG_TIMESTAMPTZ(1);
	ReturnSetInfo *rsinfo;

	hg_ws_init_srf(fcinfo, &rsinfo);
	read_queries(rsinfo, start_ts, end_ts);
	return (Datum) 0;
}

Datum
hg_wait_states_purge(PG_FUNCTION_ARGS)
{
	TimestampTz start_ts = PG_GETARG_TIMESTAMPTZ(0);
	TimestampTz end_ts = PG_GETARG_TIMESTAMPTZ(1);

	hg_ws_ensure_loaded();
	hg_ws_purge_files(start_ts, end_ts);
	PG_RETURN_VOID();
}

Datum
hg_wait_states_directory_size(PG_FUNCTION_ARGS)
{
	TimestampTz start_ts = PG_GETARG_TIMESTAMPTZ(0);
	TimestampTz end_ts = PG_GETARG_TIMESTAMPTZ(1);

	hg_ws_ensure_loaded();
	PG_RETURN_INT64(hg_ws_dir_size(start_ts, end_ts));
}
