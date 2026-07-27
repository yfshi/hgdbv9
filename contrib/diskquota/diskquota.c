/* -------------------------------------------------------------------------
 *
 * diskquota.c
 *
 * Diskquota is used to limit the amount of disk space that a schema or a role
 * can use. Diskquota is based on background worker framework. It contains a 
 * launcher process which is responsible for starting/refreshing the diskquota
 * worker processes which monitor given databases. 
 *
 * Copyright (C) 2013, PostgreSQL Global Development Group
 *
 *
 * -------------------------------------------------------------------------
 */
 
#include <unistd.h>
#include "postgres.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/statvfs.h>
#endif

#include "access/tupdesc.h"
#include "access/xact.h"
#include "access/table.h"
#include "catalog/indexing.h"
#include "catalog/namespace.h"
#include "catalog/objectaccess.h"
#include "catalog/pg_collation.h"
#include "catalog/pg_database.h"
#include "catalog/pg_extension.h"
#include "catalog/pg_type.h"
#include "commands/dbcommands.h"
#include "commands/extension.h"
#include "executor/executor.h"
#include "executor/spi.h"
#include "miscadmin.h"
#include "nodes/makefuncs.h"
#include "parser/parsetree.h"
#include "pgstat.h"
#include "postmaster/bgworker.h"
#include "storage/fd.h"
#include "storage/ipc.h"
#include "tcop/utility.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/formatting.h"
#include "utils/memutils.h"
#include "utils/numeric.h"
#include "utils/snapmgr.h"
#include "utils/syscache.h"

#include "activetable.h"
#include "diskquota.h"
#include "storage/proc.h"		/* for MyProc */
#include "common/hashfn.h"
#include "utils/acl.h"
#include "postmaster/postmaster.h"
#include "catalog/pg_authid.h"
#include "catalog/pg_namespace.h"
#include "access/skey.h"
//#include "access/table.h"
#include "access/heapam.h"
#include "utils/lsyscache.h"


PG_MODULE_MAGIC;

/* disk quota helper function */
PG_FUNCTION_INFO_V1(set_schema_quota);
PG_FUNCTION_INFO_V1(set_role_quota);
PG_FUNCTION_INFO_V1(diskquota_start_worker);

/* flags set by signal handlers */
static volatile sig_atomic_t got_sighup = false;
static volatile sig_atomic_t got_sigterm = false;
static volatile sig_atomic_t got_sigusr1 = false;

#define MAX_DISK_QUOTA_ACTIVE_ENTRIES (1 * 1024 * 1024)
#define QUOTA_CONFIG "quota_config"
/* GUC variables */
int	diskquota_naptime = 1;
char *diskquota_monitored_database_list = NULL;
int diskquota_max_active_tables = MAX_DISK_QUOTA_ACTIVE_ENTRIES;
int diskquota_min_quota_free_space = 1024;
quotaSwitch diskquota_enable = AUOTA_OFF;
quotaSwitch diskquota_strict = AUOTA_ON;

bool diskquota_create_extension = false;
LWLockPadded *disk_quota_lock_array = NULL;

DiskQuotaSessionPids *tail = NULL;
bool diskquotasql = false;

typedef struct DiskQuotaWorkerEntry DiskQuotaWorkerEntry;

/* disk quota worker info used by launcher to manage the worker processes. */
struct DiskQuotaWorkerEntry
{
	Oid dbid;
	pid_t pid; /* worker pid */
	BackgroundWorkerHandle *handle;
};

/* Begin - BUG#Z8288 */
typedef struct ActiveMinQuotaTarget
{
	/* Quota targets that should be allowed to borrow their min reserve. */
	Oid namespaceoid;
	Oid owneroid;
	/* Temp writes borrow reserve only while their temp files exist. */
	int64 namespace_temp_released;
	int64 role_temp_released;
	/* QueryDesc or utility parsetree that owns this stack frame. */
	void *owner;
	struct ActiveMinQuotaTarget *prev;
} ActiveMinQuotaTarget;

static ActiveMinQuotaTarget *active_min_quota_target = NULL;
/* End - BUG#Z8288 */

DiskQuotaLocks diskquota_locks;
DiskquotaMessageBox * message_box = NULL;
/* using hash table to support incremental update the table size entry.*/
static HTAB *disk_quota_worker_map = NULL;
static object_access_hook_type next_object_access_hook;
static ProcessUtility_hook_type prev_ProcessUtility_hook = NULL;
/* Begin - BUG#Z8288 */
static ExecutorRun_hook_type prev_ExecutorRun_hook = NULL;
static ExecutorEnd_hook_type prev_ExecutorEnd_hook = NULL;
static FileWrite_hook_type prev_FileWrite_hook = NULL;
/* End - BUG#Z8288 */
static int num_db = 0;

/* functions of disk quota*/
void _PG_init(void);
void _PG_fini(void);
void disk_quota_worker_main(Datum);
void disk_quota_launcher_main(Datum);

static void disk_quota_sigterm(SIGNAL_ARGS);
static void disk_quota_sighup(SIGNAL_ARGS);
static int64 get_size_in_mb(char *str);
static void set_quota_internal(Oid targetoid, int64 quota_limit_mb, QuotaType type);
static void set_min_quota_internal(Oid targetoid, int64 min_limit_mb, QuotaType type);
static int start_worker_by_dboid(Oid dbid);
static void create_monitor_db_table();
static inline void exec_simple_utility(const char *sql);
static void exec_simple_spi(const char *sql, int expected_code);
static bool add_db_to_config(Oid dbid);
static void del_db_from_config(Oid dbid);
static void process_message_box(void);
static void process_message_box_internal(DiskquotaMessageResult *code);
static void dq_object_access_hook(ObjectAccessType access, Oid classId,
				Oid objectId, int subId, void *arg);
static void dq_process_utility(PlannedStmt *pstmt, const char *queryString,
							   bool readOnlyTree, ProcessUtilityContext context,
							   ParamListInfo params, QueryEnvironment *queryEnv,
							   DestReceiver *dest, QueryCompletion *qc);
/* Begin - BUG#Z8288 */
static void dq_executor_run(QueryDesc *queryDesc, ScanDirection direction,
							uint64 count, bool execute_once);
static void dq_executor_end(QueryDesc *queryDesc);
static void dq_filewrite_hook(File file, int amount, off_t offset,
							  uint32 wait_event_info);
static void dq_xact_callback(XactEvent event, void *arg);
/* End - BUG#Z8288 */
static const char *err_code_to_err_message(DiskquotaMessageResult code);
extern void diskquota_invalidate_db(Oid dbid);

void refresh_box_message(void);
static void diskquota_send_sig_to_session(void);

static void diskquota_free_node(DiskQuotaSessionPids *node);
static void delete_role_schema_quota(Oid objectId, QuotaType quotatype);
static void init_box_message(void);

static const struct config_enum_entry quota_turn_options[] = {
	{"off", AUOTA_OFF, false},
	{"on", AUOTA_ON, false},
	{NULL, 0, false}
};

/*
 * Entrypoint of diskquota module.
 *
 * Init shared memory and hooks.
 * Define GUCs.
 * start diskquota launcher process.
 */
void
_PG_init(void)
{
	BackgroundWorker worker;

	memset(&worker, 0, sizeof(BackgroundWorker));

	/* diskquota.so must be in shared_preload_libraries to init SHM. */
	if (!process_shared_preload_libraries_in_progress)
		elog(ERROR, "It is too late to load diskquota.so :"
			   " please put diskquota into 'shared_preload_libraries' in GUC");

	init_disk_quota_shmem();
	init_disk_quota_enforcement();
	init_active_table_hook();

	/* get the configuration */
	DefineCustomIntVariable("diskquota.naptime",
							"Duration between each check (in seconds).",
							NULL,
							&diskquota_naptime,
							10,
							1,
							INT_MAX,
							PGC_SIGHUP,
							0,
							NULL,
							NULL,
							NULL);

	DefineCustomIntVariable("diskquota.max_active_tables",
							"max number of active tables monitored by disk-quota",
							NULL,
							&diskquota_max_active_tables,
							1024,
							1,
							INT_MAX,
							PGC_SIGHUP,
							0,
							NULL,
							NULL,
							NULL);

	DefineCustomEnumVariable("diskquota.enable",
						 "turn on or off",
						 NULL,
						 (int *)&diskquota_enable,
						 AUOTA_OFF,
						 quota_turn_options,
						 PGC_POSTMASTER,
						 0,
						 NULL,
						 NULL,
						 NULL);

	DefineCustomEnumVariable("diskquota.strict",
						 "turn on or off",
						 NULL,
						 (int *)&diskquota_strict,
						 AUOTA_ON,
						 quota_turn_options,
						 PGC_POSTMASTER,
						 0,
						 NULL,
						 NULL,
						 NULL);

	DefineCustomIntVariable("diskquota.min_quota_free_space",
							"Minimum free disk space to keep after min quota reserve files are expanded.",
							NULL,
							&diskquota_min_quota_free_space,
							1024,
							0,
							INT_MAX,
							PGC_SUSET,
							GUC_UNIT_MB,
							NULL,
							NULL,
							NULL);

	/* Add dq_object_access_hook to handle drop extension event.*/
	next_object_access_hook = object_access_hook;
	object_access_hook = dq_object_access_hook;

	prev_ProcessUtility_hook = ProcessUtility_hook;
	ProcessUtility_hook = dq_process_utility;

	/* Begin - BUG#Z8288 */
	prev_ExecutorRun_hook = ExecutorRun_hook;
	ExecutorRun_hook = dq_executor_run;

	prev_ExecutorEnd_hook = ExecutorEnd_hook;
	ExecutorEnd_hook = dq_executor_end;

	prev_FileWrite_hook = FileWrite_hook;
	FileWrite_hook = dq_filewrite_hook;

	RegisterXactCallback(dq_xact_callback, NULL);
	/* End - BUG#Z8288 */

	if (diskquota_enable != AUOTA_ON)
		return ;

	/* set up common data for diskquota launcher worker */
	worker.bgw_flags = BGWORKER_SHMEM_ACCESS |
		BGWORKER_BACKEND_DATABASE_CONNECTION;
	worker.bgw_start_time = BgWorkerStart_RecoveryFinished;
	worker.bgw_restart_time = BGW_NEVER_RESTART;
	sprintf(worker.bgw_library_name, "diskquota");
	sprintf(worker.bgw_function_name, "disk_quota_launcher_main");
	worker.bgw_notify_pid = 0;
	snprintf(worker.bgw_type, BGW_MAXLEN, "disk_quota_launcher_main");

	snprintf(worker.bgw_name, BGW_MAXLEN, "[diskquota] - launcher");

	RegisterBackgroundWorker(&worker);
}

void
_PG_fini(void)
{
}

/*
 * Signal handler for SIGTERM
 *		Set a flag to let the main loop to terminate, and set our latch to wake
 *		it up.
 */
static void
disk_quota_sigterm(SIGNAL_ARGS)
{
	int			save_errno = errno;

	got_sigterm = true;
	if (MyProc)
		SetLatch(&MyProc->procLatch);

	errno = save_errno;
}

/*
 * Signal handler for SIGHUP
 *		Set a flag to tell the main loop to reread the config file, and set
 *		our latch to wake it up.
 */
static void
disk_quota_sighup(SIGNAL_ARGS)
{
	int			save_errno = errno;

	got_sighup = true;
	if (MyProc)
		SetLatch(&MyProc->procLatch);

	errno = save_errno;
}

/*
 * Signal handler for SIGUSR1
 * 		Set a flag to tell the launcher to handle message box
 */
static void
disk_quota_sigusr1(SIGNAL_ARGS)
{
	int save_errno = errno;
	got_sigusr1 = true;

	if (MyProc)
		SetLatch(&MyProc->procLatch);

	errno = save_errno;
}

/* wakeup */
static void
disk_quota_sigusr2(SIGNAL_ARGS)
{
	int			save_errno = errno;

	SetLatch(MyLatch);

	errno = save_errno;
}


/* ---- Functions for disk quota worker process ---- */

/*
 * Disk quota worker process will refresh disk quota model periodically.
 * Refresh logic is defined in quotamodel.c
 */
void
disk_quota_worker_main(Datum main_arg)
{
	char *dbname = MyBgworkerEntry->bgw_extra;

	if (diskquota_enable != AUOTA_ON)
		proc_exit(1);

	elog(LOG,"[diskquota]:start disk quota worker process to monitor database:%s", dbname);

	/* Establish signal handlers before unblocking signals. */
	pqsignal(SIGHUP, disk_quota_sighup);
	pqsignal(SIGTERM, disk_quota_sigterm);
	pqsignal(SIGUSR1, disk_quota_sigusr1);

	pqsignal(SIGUSR2, disk_quota_sigusr2);

	/* We're now ready to receive signals */
	BackgroundWorkerUnblockSignals();

	/* Connect to our database */
	BackgroundWorkerInitializeConnection(dbname, NULL, 0);

	/* Initialize diskquota related local hash map and refresh model immediately*/
	init_disk_quota_model();
	refresh_disk_quota_model(true);
	refresh_box_message();

	/*
	 * Main loop: do this until the SIGTERM handler tells us to terminate
	 */
	while (!got_sigterm)
	{
		int			rc;

		/*
		 * Background workers mustn't call usleep() or any direct equivalent:
		 * instead, they may wait on their process latch, which sleeps as
		 * necessary, but is awakened if postmaster dies.  That way the
		 * background process goes away immediately in an emergency.
		 */
		rc = WaitLatch(&MyProc->procLatch,
					   WL_LATCH_SET | WL_TIMEOUT | WL_POSTMASTER_DEATH,
					   diskquota_naptime * 1000L, PG_WAIT_EXTENSION);
		ResetLatch(&MyProc->procLatch);

		/* Do the work */
		refresh_disk_quota_model(false);
		refresh_box_message();

		/* emergency bailout if postmaster has died */
		if (rc & WL_POSTMASTER_DEATH)
			proc_exit(1);

		/*
		 * In case of a SIGHUP, just reload the configuration.
		 */
		if (got_sighup)
		{
			got_sighup = false;
			ProcessConfigFile(PGC_SIGHUP);
		}
	}

	/* Release on-disk reservations before the worker exits. */
	clear_min_quota_reserve_files();
	diskquota_invalidate_db(MyDatabaseId);
	diskquota_send_sig_to_session();
	init_box_message();
	proc_exit(0);
}

/**
 * create table to record the list of monitored databases
 * we need a place to store the database with diskquota enabled
 * (via CREATE EXTENSION diskquota). Currently, we store them into
 * heap table in diskquota_namespace schema of postgres database.
 * When database restarted, diskquota laucher will start worker processes
 * for these databases.
 */
static void
create_monitor_db_table()
{
	const char *sql;
	sql = "create schema if not exists hg_security;"
		"create table if not exists hg_security.diskquota_database_list(dbid oid not null unique);";
	exec_simple_utility(sql);
}

static inline void
exec_simple_utility(const char *sql)
{
	StartTransactionCommand();
	exec_simple_spi(sql, SPI_OK_UTILITY);
	CommitTransactionCommand();
}

static void
exec_simple_spi(const char *sql, int expected_code)
{
	int ret;

	ret = SPI_connect();
	if (ret != SPI_OK_CONNECT)
		elog(ERROR, "connect error, code=%d", ret);
	PushActiveSnapshot(GetTransactionSnapshot());
	ret = SPI_execute(sql, false, 0);
	if (ret != expected_code)
		elog(ERROR, "sql:'%s', code %d", sql, ret);
	SPI_finish();
	PopActiveSnapshot();
}

static bool
is_valid_dbid(Oid dbid)
{
	HeapTuple       tuple;

	if (dbid == InvalidOid)
		return false;
	tuple = SearchSysCache1(DATABASEOID, ObjectIdGetDatum(dbid));
	if (!HeapTupleIsValid(tuple))
		return false;
	ReleaseSysCache(tuple);
	return true;
}
/*
 * in early stage, start all worker processes of diskquota-enabled databases
 * from hg_security.diskquota_database_list
 */
static void
start_workers_from_dblist()
{
	TupleDesc tupdesc;
	int num = 0;
	int ret;
	int i;
	StartTransactionCommand();
	PushActiveSnapshot(GetTransactionSnapshot());
	ret = SPI_connect();
	if (ret != SPI_OK_CONNECT)
		elog(ERROR, "connect error, code=%d", ret);
	ret = SPI_execute("select dbid from hg_security.diskquota_database_list;", false, 0);
	if (ret != SPI_OK_SELECT)
		elog(ERROR, "select hg_security.diskquota_database_list");
	tupdesc = SPI_tuptable->tupdesc;
	if (tupdesc->natts != 1 || tupdesc->attrs[0].atttypid != OIDOID)
		elog(ERROR, "[diskquota] table diskquota_database_list corrupt, laucher will exit");

	for (i = 0; num < SPI_processed; i++)
	{
		HeapTuple tup;
		Oid dbid;
		Datum dat;
		bool isnull;

	    tup	= SPI_tuptable->vals[i];
		dat = SPI_getbinval(tup, tupdesc, 1, &isnull);
		if (isnull)
		{
			elog(ERROR, "dbid cann't be null");
		}
		dbid = DatumGetObjectId(dat);
		if (!is_valid_dbid(dbid))
		{
			continue;
		}
		if (start_worker_by_dboid(dbid) < 1)
		{
			elog(WARNING, "[diskquota]: start worker process of database(%d) failed", dbid);
		}
		num++;
	}
	num_db = num;
	message_box->db_num = num_db;
	SPI_finish();
	PopActiveSnapshot();
	CommitTransactionCommand();

	/*  TODO: clean invalid database */
}

static bool
add_db_to_config(Oid dbid)
{
	StringInfoData str;

	initStringInfo(&str);
	/* Begin - BUG#Z7820 */
	appendStringInfo(&str, "insert into hg_security.diskquota_database_list values(%d) ON CONFLICT DO NOTHING;", dbid);
	/* End - BUG#Z7820 */
	exec_simple_spi(str.data, SPI_OK_INSERT);

	return true;
}

static void
del_db_from_config(Oid dbid)
{
	StringInfoData str;

	initStringInfo(&str);
	appendStringInfo(&str, "delete from hg_security.diskquota_database_list where dbid=%d;", dbid);
	exec_simple_spi(str.data, SPI_OK_DELETE);
}

/*
 * When drop exention database, diskquota laucher will receive a message
 * to kill the diskquota worker process which monitoring the target database. 
 */
static void
try_kill_db_worker(Oid dbid)
{
	DiskQuotaWorkerEntry *hash_entry;
	bool found;
	hash_entry = (DiskQuotaWorkerEntry *)hash_search(disk_quota_worker_map,
											(void *)&dbid,
											HASH_REMOVE, &found);
	if (found)
	{
		BackgroundWorkerHandle *handle;
		handle = hash_entry->handle;
		TerminateBackgroundWorker(handle);
		pfree(handle);
	}
}

/*
 * handle create extension diskquota
 * if we know the exact error which caused failure,
 * we set it, and error out
 */
static void
on_add_db(Oid dbid, DiskquotaMessageResult *code)
{
	if (num_db > QUOTA_MAX_DB_NUM)
	{
		*code = ERR_EXCEED;
		elog(ERROR, "[diskquota] too database to monitor");
	}
	if (!is_valid_dbid(dbid))
	{
		*code = ERR_INVALID_DBID;
		elog(ERROR, "[diskquota] invalid database oid");
	}

	/*
	 * add dbid to hg_security.diskquota_database_list
	 * set *code to ERR_ADD_TO_DB if any error occurs
	 */
	PG_TRY();
	{
		add_db_to_config(dbid);
	}
	PG_CATCH();
	{
		*code = ERR_ADD_TO_DB;
		PG_RE_THROW();
	}
	PG_END_TRY();

	/* Begin - BUG#Z7820 */
	/* Recreating after a failed CREATE may find a stale worker for this dbid. */
	try_kill_db_worker(dbid);
	/* End - BUG#Z7820 */

	if (start_worker_by_dboid(dbid) < 1)
	{
		*code = ERR_START_WORKER;
		elog(ERROR, "[diskquota] failed to start worker - dbid=%d", dbid);
	}
}

/*
 * handle message: drop extension diskquota
 * do our best to:
 * 1. kill the associated worker process
 * 2. delete dbid from hg_security.diskquota_database_list
 * 3. invalidate black-map entries from shared memory
 */
static void
on_del_db(Oid dbid)
{
	if (dbid == InvalidOid)
		return;
	try_kill_db_worker(dbid);
	del_db_from_config(dbid);
}

/* ---- Functions for lancher process ---- */
/*
 * Launcher process manages the worker processes based on
 * GUC diskquota.monitor_databases in configuration file.
 */
void
disk_quota_launcher_main(Datum main_arg)
{
	HASHCTL		hash_ctl;

	if (diskquota_enable != AUOTA_ON)
		proc_exit(1);

	/* Establish signal handlers before unblocking signals. */
	pqsignal(SIGHUP, disk_quota_sighup);
	pqsignal(SIGTERM, disk_quota_sigterm);
	pqsignal(SIGUSR1, disk_quota_sigusr1);

	/* We're now ready to receive signals */
	BackgroundWorkerUnblockSignals();

	message_box->launcher_pid = MyProcPid;
	/* Connect to our database */
	BackgroundWorkerInitializeConnection(DatabaseName, NULL, 0);
	create_monitor_db_table();

	memset(&hash_ctl, 0, sizeof(hash_ctl));
	hash_ctl.keysize = sizeof(Oid);
	hash_ctl.entrysize = sizeof(DiskQuotaWorkerEntry);
	hash_ctl.hash = oid_hash;

	disk_quota_worker_map = hash_create("disk quota worker map",
						  1024,
						  &hash_ctl,
						  HASH_ELEM | HASH_FUNCTION);

	start_workers_from_dblist();

	/*
	 * Main loop: do this until the SIGTERM handler tells us to terminate
	 */
	while (!got_sigterm)
	{
		int			rc;

		/*
		 * Background workers mustn't call usleep() or any direct equivalent:
		 * instead, they may wait on their process latch, which sleeps as
		 * necessary, but is awakened if postmaster dies.  That way the
		 * background process goes away immediately in an emergency.
		 */
		rc = WaitLatch(&MyProc->procLatch,
						WL_LATCH_SET | WL_TIMEOUT | WL_POSTMASTER_DEATH,
						diskquota_naptime * 1000L, PG_WAIT_EXTENSION);
		ResetLatch(&MyProc->procLatch);

		/* emergency bailout if postmaster has died */
		if (rc & WL_POSTMASTER_DEATH)
			proc_exit(1);
		/* process message box, now someone is holding message_box_lock */
		if (got_sigusr1)
		{
			got_sigusr1 = false;
			process_message_box();
		}
		/*
		 * In case of a SIGHUP, just reload the configuration.
		 */
		if (got_sighup)
		{
			got_sighup = false;
			ProcessConfigFile(PGC_SIGHUP);
		}

	}

	proc_exit(1);
}

/*
 * Dynamically launch an disk quota worker process.
 */
static int
start_worker_by_dboid(Oid dbid)
{
	BackgroundWorker worker;
	BackgroundWorkerHandle *handle;
	BgwHandleStatus status;
	MemoryContext old_ctx;
	char *dbname;
	pid_t pid;
	bool found;
	bool ok;
	DiskQuotaWorkerEntry* workerentry;

	memset(&worker, 0, sizeof(BackgroundWorker));
	worker.bgw_flags = BGWORKER_SHMEM_ACCESS |
		BGWORKER_BACKEND_DATABASE_CONNECTION;
	worker.bgw_start_time = BgWorkerStart_RecoveryFinished;
	worker.bgw_restart_time = BGW_NEVER_RESTART;
	sprintf(worker.bgw_library_name, "diskquota");
	sprintf(worker.bgw_function_name, "disk_quota_worker_main");
	snprintf(worker.bgw_type, BGW_MAXLEN, "disk_quota_worker_main");

	dbname = get_database_name(dbid);
	Assert(dbname != NULL);
	snprintf(worker.bgw_name, sizeof(worker.bgw_name), "[diskquota] %s", dbname);
	snprintf(worker.bgw_extra, sizeof(worker.bgw_extra), "%s", dbname);
	pfree(dbname);
	/* set bgw_notify_pid so that we can use WaitForBackgroundWorkerStartup */
	worker.bgw_notify_pid = MyProcPid;
	worker.bgw_main_arg = (Datum) 0;

	old_ctx = MemoryContextSwitchTo(TopMemoryContext);
	ok = RegisterDynamicBackgroundWorker(&worker, &handle);
	MemoryContextSwitchTo(old_ctx);
	if (!ok)
		return -1;
	status = WaitForBackgroundWorkerStartup(handle, &pid);
	if (status == BGWH_STOPPED)
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_RESOURCES),
				 errmsg("could not start background process"),
			   errhint("More details may be available in the server log.")));
	if (status == BGWH_POSTMASTER_DIED)
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_RESOURCES),
			  errmsg("cannot start background processes without postmaster"),
				 errhint("Kill all remaining database processes and restart the database.")));

	Assert(status == BGWH_STARTED);

	/* put the worker handle into the worker map */
	workerentry = (DiskQuotaWorkerEntry *)hash_search(disk_quota_worker_map,
														(void *)&dbid,
														HASH_ENTER, &found);
	if (!found)
	{
		workerentry->handle = handle;
		workerentry->pid = pid;
	}

	return pid;
}

/* ---- Help Functions to set quota limit. ---- */
/*
 * Set disk quota limit for role.
 */
Datum
set_role_quota(PG_FUNCTION_ARGS)
{
	Oid roleoid;
	char *rolname;
	char *sizestr;
	int64 quota_limit_mb;
	bool min_quota = false;

	if (!superuser())
	{
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("must be superuser to set disk quota limit")));
	}

	rolname = text_to_cstring(PG_GETARG_TEXT_PP(0));
	rolname = str_tolower(rolname, strlen(rolname), DEFAULT_COLLATION_OID);
	roleoid = get_role_oid(rolname, false);

	sizestr = text_to_cstring(PG_GETARG_TEXT_PP(1));
	sizestr = str_tolower(sizestr, strlen(sizestr), DEFAULT_COLLATION_OID);
	quota_limit_mb = get_size_in_mb(sizestr);

	if (PG_NARGS() == 3)
		min_quota = PG_GETARG_BOOL(2);

	diskquotasql = true;
	if (min_quota)
		set_min_quota_internal(roleoid, quota_limit_mb, ROLE_QUOTA);
	else
		set_quota_internal(roleoid, quota_limit_mb, ROLE_QUOTA);
	diskquotasql = false;
	PG_RETURN_VOID();
}

/*
 * Set disk quota limit for schema.
 */
Datum
set_schema_quota(PG_FUNCTION_ARGS)
{
	Oid namespaceoid;
	char *nspname;
	char *sizestr;
	int64 quota_limit_mb;
	bool min_quota = false;

	if (!superuser())
	{
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("must be superuser to set disk quota limit")));
	}

	nspname = text_to_cstring(PG_GETARG_TEXT_PP(0));
	nspname = str_tolower(nspname, strlen(nspname), DEFAULT_COLLATION_OID);
	namespaceoid = get_namespace_oid(nspname, false);

	sizestr = text_to_cstring(PG_GETARG_TEXT_PP(1));
	sizestr = str_tolower(sizestr, strlen(sizestr),  DEFAULT_COLLATION_OID);
	quota_limit_mb = get_size_in_mb(sizestr);

	if (PG_NARGS() == 3)
		min_quota = PG_GETARG_BOOL(2);

	diskquotasql = true;
	if (min_quota)
		set_min_quota_internal(namespaceoid, quota_limit_mb, NAMESPACE_QUOTA);
	else
		set_quota_internal(namespaceoid, quota_limit_mb, NAMESPACE_QUOTA);
	diskquotasql = false;

	PG_RETURN_VOID();
}

void get_disk_space(const char *path, uint64 *total, uint64 *free)
{
#ifdef _WIN32
	ULARGE_INTEGER free_bytes, total_bytes, total_free_bytes;
	if (path == NULL || total == NULL || free == NULL)
		return ;

	if (GetDiskFreeSpaceExA(path, &free_bytes, &total_bytes, &total_free_bytes))
	{
		*total = (uint64)total_bytes.QuadPart;
		*free = (uint64)total_free_bytes.QuadPart;
	}
	else
	{
		ereport(ERROR, errmsg("call system function \"statvfs\" fail:error code:%lu\n", GetLastError()));
	}
#else
	struct statvfs vfs;
	if (path == NULL || total == NULL || free == NULL)
		return ;

	if (statvfs(path, &vfs) == 0)
	{
		*total = (unsigned long long)vfs.f_blocks * vfs.f_frsize;
		*free = (unsigned long long)vfs.f_bfree * vfs.f_frsize;
	}
	else
	{
		ereport(ERROR, errmsg("call system function \"statvfs\" fail"));
	}
#endif
}

/*
 * Write the quota limit info into quota_config table under
 * 'diskquota' schema of the current database.
 */
static void
set_quota_internal(Oid targetoid, int64 quota_limit_mb, QuotaType type)
{
	int ret;
	int64 current_min_limit_mb = -1;
	StringInfoData buf;

	/* Begin - BUG#Z7633 */
	if (quota_limit_mb < 0)
		elog(ERROR, "quota limit must be >= (0 MB)");
	/* End - BUG#Z7633 */

	initStringInfo(&buf);
	appendStringInfo(&buf,
					"select * from hg_security.quota_config where targetoid = %u"
					" and quotatype =%d",
					targetoid, type);

	SPI_connect();

	ret = SPI_execute(buf.data, false, 0);
	if (ret != SPI_OK_SELECT)
		elog(ERROR, "cannot select quota setting table: error code %d", ret);

	if (SPI_processed > 0)
	{
		bool isnull;
		Datum dat;
		HeapTuple tup = SPI_tuptable->vals[0];
		TupleDesc tupdesc = SPI_tuptable->tupdesc;
		if (tupdesc->natts >= 4)
		{
			dat = SPI_getbinval(tup, tupdesc, 4, &isnull);
			if (!isnull)
				current_min_limit_mb = DatumGetInt64(dat);
		}
	}

	if (current_min_limit_mb > 0 && quota_limit_mb < current_min_limit_mb)
		elog(ERROR, "quota limit (%ld MB) must be >= min quota (%ld MB)", quota_limit_mb, current_min_limit_mb);

	/* if the schema or role's quota has been set before*/
	if (SPI_processed == 0 && quota_limit_mb > 0)
	{
		resetStringInfo(&buf);
		initStringInfo(&buf);
		appendStringInfo(&buf,
					"insert into hg_security.quota_config values(%u,%d,%ld);",
					targetoid, type, quota_limit_mb);
		ret = SPI_execute(buf.data, false, 0);
		if (ret != SPI_OK_INSERT)
			elog(ERROR, "cannot insert into quota setting table, error code %d", ret);
	}
	else if(SPI_processed > 0 && quota_limit_mb >= 0)
	{
		resetStringInfo(&buf);
		initStringInfo(&buf);
		appendStringInfo(&buf,
					"update hg_security.quota_config set quotalimitMB = %ld where targetoid=%u"
					" and quotatype=%d;",
					quota_limit_mb, targetoid, type);
		ret = SPI_execute(buf.data, false, 0);
		if (ret != SPI_OK_UPDATE)
			elog(ERROR, "cannot update quota setting table, error code %d", ret);
	}

	/* Keep enforcement state in sync with SQL-visible quota metadata. */
	if (quota_limit_mb > 0)
		update_shared_quota_state(targetoid, quota_limit_mb, type);
	else
		remove_shared_quota_state(targetoid, type);
	/*
	 * And finish our transaction.
	 */
	SPI_finish();
	return;
}

/*
 * Persist a min quota and resize its reserve file to match current usage.
 */
static void
set_min_quota_internal(Oid targetoid, int64 min_limit_mb, QuotaType type)
{
	int ret;
	int64 current_quota_limit_mb = -1;
	int64 used_bytes = 0;
	int64 reserve_bytes = 0;
	int64 old_reserve_bytes = 0; /* BUG#Z8153 */
	uint64 total = 0;
	uint64 free = 0;
	StringInfoData buf;

	/* Begin - BUG#Z7633 */
	if (min_limit_mb < 0)
		elog(ERROR, "min quota limit must be >= (0 MB)");
	/* End - BUG#Z7633 */

	get_disk_space(DataDir, &total, &free);
	/* Begin - BUG#Z8153 */
	old_reserve_bytes = get_min_quota_reserve_file_size(type, targetoid);
	if (min_limit_mb > 0 && /* Bug#z8703 */
		(uint64)(min_limit_mb + diskquota_min_quota_free_space) * 1024 * 1024 >
		(free + old_reserve_bytes))
		elog(ERROR,
			"The current available disk space is %ld MB,"
			"required remaining free space is %d MB,"
			"cannot set `min quota` to %ld MB.",
			(free + old_reserve_bytes) / (1024 * 1024), diskquota_min_quota_free_space, min_limit_mb);
	/* End - BUG#Z8153 */

	initStringInfo(&buf);
	appendStringInfo(&buf,
					"select * from hg_security.quota_config where targetoid = %u"
					" and quotatype =%d",
					targetoid, type);

	SPI_connect();
	ret = SPI_execute(buf.data, false, 0);
	if (ret != SPI_OK_SELECT)
		elog(ERROR, "cannot select quota setting table: error code %d", ret);

	if (SPI_processed > 0)
	{
		bool isnull;
		Datum dat;
		HeapTuple tup = SPI_tuptable->vals[0];
		TupleDesc tupdesc = SPI_tuptable->tupdesc;

		if (tupdesc->natts >= 3)
		{
			dat = SPI_getbinval(tup, tupdesc, 3, &isnull);
			if (!isnull)
				current_quota_limit_mb = DatumGetInt64(dat);
		}
	}

	if (current_quota_limit_mb > 0 && min_limit_mb > current_quota_limit_mb)
		elog(ERROR, "min quota (%ld MB) must be <= quota limit (%ld MB)", min_limit_mb, current_quota_limit_mb);

	if (SPI_processed == 0 && min_limit_mb > 0)
	{
		resetStringInfo(&buf);
		initStringInfo(&buf);
		appendStringInfo(&buf,
					"insert into hg_security.quota_config(targetoid, quotatype, quotalimitMB, minlimitMB)"
					" values(%u,%d,%d,%ld);",
					targetoid, type, 0, min_limit_mb);
		ret = SPI_execute(buf.data, false, 0);
		pfree(buf.data);
		if (ret != SPI_OK_INSERT)
			elog(ERROR, "cannot insert min quota setting table, error code %d", ret);
	}
	else if (SPI_processed > 0 && min_limit_mb >= 0)
	{
		resetStringInfo(&buf);
		initStringInfo(&buf);
		appendStringInfo(&buf,
					"update hg_security.quota_config set minlimitMB = %ld where targetoid=%u"
					" and quotatype=%d;",
					min_limit_mb > 0 ? min_limit_mb : 0, targetoid, type);
		ret = SPI_execute(buf.data, false, 0);
		pfree(buf.data);
		if (ret != SPI_OK_UPDATE)
			elog(ERROR, "cannot update min quota setting table, error code %d", ret);
	}

	if (min_limit_mb > 0)
	{
		/* Reserve only the bytes needed to reach the requested minimum. */
		used_bytes = get_current_usage_bytes(targetoid, type);
		reserve_bytes = min_limit_mb * 1024 * 1024 - used_bytes;
		if (reserve_bytes < 0)
			reserve_bytes = 0;
	}

	SPI_finish();

	resize_min_quota_reserve_file(type, targetoid, reserve_bytes);
}

/*
 * Calculate current disk usage for one schema or role quota target.
 */
int64
get_current_usage_bytes(Oid targetoid, QuotaType type)
{
	StringInfoData buf;
	int ret;
	int64 used_bytes = 0;
	bool isnull;

	initStringInfo(&buf);
	if (type == NAMESPACE_QUOTA)
		appendStringInfo(&buf,
						 "select coalesce(sum(pg_total_relation_size(oid)), 0)::bigint "
						 "from pg_class where relnamespace = %u and relkind in ('r', 'm')",
						 targetoid);
	else
		appendStringInfo(&buf,
						 "select coalesce(sum(pg_total_relation_size(oid)), 0)::bigint "
						 "from pg_class where relowner = %u and relkind in ('r', 'm')",
						 targetoid);

	ret = SPI_execute(buf.data, true, 1);
	pfree(buf.data);
	if (ret != SPI_OK_SELECT)
		elog(ERROR, "cannot calculate current quota usage: error code %d", ret);

	if (SPI_processed > 0)
	{
		Datum dat = SPI_getbinval(SPI_tuptable->vals[0],
								  SPI_tuptable->tupdesc,
								  1,
								  &isnull);
		if (!isnull)
			used_bytes = DatumGetInt64(dat);
	}

	return used_bytes;
}

/*
 * Convert a human-readable size to a size in MB.
 */
static int64
get_size_in_mb(char *str)
{
	char		*strptr, *endptr;
	char		saved_char;
	Numeric	num;
	int64	result;
	bool		have_digits = false;

	/* Skip leading whitespace */
	strptr = str;
	while (isspace((unsigned char) *strptr))
		strptr++;

	/* Check that we have a valid number and determine where it ends */
	endptr = strptr;

	/* Part (1): sign */
	if (*endptr == '-' || *endptr == '+')
		endptr++;

	/* Part (2): main digit string */
	if (isdigit((unsigned char) *endptr))
	{
		have_digits = true;
		do
			endptr++;
		while (isdigit((unsigned char) *endptr));
	}

	/* Part (3): optional decimal point and fractional digits */
	if (*endptr == '.')
	{
		endptr++;
		if (isdigit((unsigned char) *endptr))
		{
			have_digits = true;
			do
				endptr++;
			while (isdigit((unsigned char) *endptr));
		}
	}

	/* Complain if we don't have a valid number at this point */
	if (!have_digits)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("invalid size: \"%s\"", str)));

	/* Part (4): optional exponent */
	if (*endptr == 'e' || *endptr == 'E')
	{
		long		exponent;
		char		*cp;

		/*
		 * Note we might one day support EB units, so if what follows 'E'
		 * isn't a number, just treat it all as a unit to be parsed.
		 */
		exponent = strtol(endptr + 1, &cp, 10);
		(void) exponent;		/* Silence -Wunused-result warnings */
		if (cp > endptr + 1)
			endptr = cp;
	}

	/*
	 * Parse the number, saving the next character, which may be the first
	 * character of the unit string.
	 */
	saved_char = *endptr;
	*endptr = '\0';

	num = DatumGetNumeric(DirectFunctionCall3(numeric_in,
											  CStringGetDatum(strptr),
											  ObjectIdGetDatum(InvalidOid),
											  Int32GetDatum(-1)));

	*endptr = saved_char;

	/* Skip whitespace between number and unit */
	strptr = endptr;
	while (isspace((unsigned char) *strptr))
		strptr++;

	/* Handle possible unit */
	if (*strptr != '\0')
	{
		int64		multiplier = 0;

		/* Trim any trailing whitespace */
		endptr = str + strlen(str) - 1;

		while (isspace((unsigned char) *endptr))
			endptr--;

		endptr++;
		*endptr = '\0';

		/* Parse the unit case-insensitively */
		if (pg_strcasecmp(strptr, "mb") == 0)
			multiplier = ((int64) 1);

		else if (pg_strcasecmp(strptr, "gb") == 0)
			multiplier = ((int64) 1024);

		else if (pg_strcasecmp(strptr, "tb") == 0)
			multiplier = ((int64) 1024) * 1024 ;
		else if (pg_strcasecmp(strptr, "pb") == 0)
			multiplier = ((int64) 1024) * 1024 * 1024 ;
		else
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("invalid size: \"%s\"", str),
					 errdetail("Invalid size unit: \"%s\".", strptr),
					 errhint("Valid units are \"MB\", \"GB\", \"TB\", and \"PB\".")));

		if (multiplier > 1)
		{
			Numeric		mul_num;

			mul_num = DatumGetNumeric(DirectFunctionCall1(int8_numeric,
														  Int64GetDatum(multiplier)));

			num = DatumGetNumeric(DirectFunctionCall2(numeric_mul,
													  NumericGetDatum(mul_num),
													  NumericGetDatum(num)));
		}
	}

	result = DatumGetInt64(DirectFunctionCall1(numeric_int8,
											   NumericGetDatum(num)));

	return result;
}

/*
 * trigger start diskquota worker when create extension diskquota
 * This function is called at backend side, and will send message to 
 * diskquota launcher. Luacher process is responsible for starting the real
 * diskquota worker process.
 */
Datum
diskquota_start_worker(PG_FUNCTION_ARGS)
{
	int rc;

	/* Begin - BUG#Z7631 */
	if (diskquota_enable != AUOTA_ON)
		elog(ERROR, "diskquota extension creation requires diskquota switch to be on.");
	/* End - BUG#Z7631 */

	LWLockAcquire(diskquota_locks.message_box_lock, LW_EXCLUSIVE);
	message_box->req_pid = MyProcPid;
	message_box->cmd = CMD_CREATE_EXTENSION;
	message_box->result = ERR_PENDING;
	message_box->data[0] = MyDatabaseId;
	LWLockRelease(diskquota_locks.message_box_lock);
	/* setup sig handler to receive message */
	rc = kill(message_box->launcher_pid, SIGUSR1);
	if (rc == 0)
	{
		int count = WAIT_TIME_COUNT;
		while(count-- > 0)
		{
			rc = WaitLatch(&MyProc->procLatch,
						   WL_LATCH_SET | WL_TIMEOUT | WL_POSTMASTER_DEATH,
						   100L, PG_WAIT_EXTENSION);
			if (rc & WL_POSTMASTER_DEATH)
				break;
			ResetLatch(&MyProc->procLatch);
			LWLockAcquire(diskquota_locks.message_box_lock, LW_SHARED);
			if (message_box->result != ERR_PENDING)
			{
				LWLockRelease(diskquota_locks.message_box_lock);
				break;
			}
			LWLockRelease(diskquota_locks.message_box_lock);
		}
	}
	LWLockAcquire(diskquota_locks.message_box_lock, LW_EXCLUSIVE);
	message_box->req_pid = 0;
	LWLockRelease(diskquota_locks.message_box_lock);
	if (message_box->result != ERR_OK)
		elog(ERROR, "%s", err_code_to_err_message((DiskquotaMessageResult)message_box->result));
	PG_RETURN_VOID();
}

static void
process_message_box_internal(DiskquotaMessageResult *code)
{
	Assert(message_box->launcher_pid == MyProcPid);
	switch (message_box->cmd)
	{
		case CMD_CREATE_EXTENSION:
			on_add_db(message_box->data[0], code);
			num_db++;
			break;
		case CMD_DROP_EXTENSION:
			on_del_db(message_box->data[0]);
			num_db--;
			break;
		default:
			elog(LOG, "[diskquota]:unsupported message cmd=%d", message_box->cmd);
			*code = ERR_UNKNOWN;
			break;
	}
}

/*
 * this function is called by launcher process to handle message from other backend 
 * processes which call CREATE/DROP EXTENSION diskquota; It must be able to catch errors,
 * and return an error code back to the backend process.
 */
static void
process_message_box()
{
	DiskquotaMessageResult code = ERR_UNKNOWN;
	int old_num_db = num_db;

	if (message_box->req_pid == 0)
		return;

	elog(LOG, "[launcher]: received message");
	PG_TRY();
	{
		StartTransactionCommand();
		process_message_box_internal(&code);
		CommitTransactionCommand();
		code = ERR_OK;
	}
	PG_CATCH();
	{
		error_context_stack = NULL;
		HOLD_INTERRUPTS();
		AbortCurrentTransaction();
		FlushErrorState();
		RESUME_INTERRUPTS();
		num_db = old_num_db;
	}
	PG_END_TRY();

	message_box->result = (int)code;
	message_box->db_num = num_db;
}

/*
 * This hook is used to handle drop extension diskquota event
 * It will send CMD_DROP_EXTENSION message to diskquota laucher.
 * Laucher will terminate the corresponding worker process and
 * remove the dbOid from the diskquota_database_list table.
 */
static void
dq_object_access_hook(ObjectAccessType access, Oid classId,
			Oid objectId, int subId, void *arg)
{
	Oid oid;
	int rc;

	if (diskquota_enable != AUOTA_ON)
		goto out;

	if (access == OAT_DROP && classId == AuthIdRelationId)
	{
		delete_role_schema_quota(objectId, ROLE_QUOTA);
		goto out;
	}
	else if (access == OAT_DROP && classId == NamespaceRelationId)
	{
		delete_role_schema_quota(objectId, NAMESPACE_QUOTA);
		goto out;
	}
	else if (access != OAT_DROP || classId != ExtensionRelationId)
		goto out;

	oid = get_extension_oid("diskquota", true);
	if (oid != objectId)
		goto out;

	/*
	 * invoke drop extension diskquota
	 * 1. stop bgworker for MyDatabaseId
	 * 2. remove dbid from hg_security.diskquota_database_list in postgres
	 */
	LWLockAcquire(diskquota_locks.message_box_lock, LW_EXCLUSIVE);
	message_box->req_pid = MyProcPid;
	message_box->cmd = CMD_DROP_EXTENSION;
	message_box->result = ERR_PENDING;
	message_box->data[0] = MyDatabaseId;
	rc = kill(message_box->launcher_pid, SIGUSR1);
	if (rc == 0)
	{
		int count = WAIT_TIME_COUNT;
		while(count-- >0)
		{
			rc = WaitLatch(&MyProc->procLatch,
						   WL_LATCH_SET | WL_TIMEOUT | WL_POSTMASTER_DEATH,
						   100L, PG_WAIT_EXTENSION);
			if (rc & WL_POSTMASTER_DEATH)
				break;
			ResetLatch(&MyProc->procLatch);
			if (message_box->result != ERR_PENDING)
				break;
		}
	}
	message_box->req_pid = 0;
	LWLockRelease(diskquota_locks.message_box_lock);
	if (message_box->result != ERR_OK)
		elog(ERROR, "[diskquota] %s", err_code_to_err_message((DiskquotaMessageResult)message_box->result));
	elog(LOG, "[diskquota] DROP EXTENTION diskquota; OK");

out:
	if (next_object_access_hook)
		(*next_object_access_hook)(access, classId, objectId,
				subId, arg);
}

/* Begin - BUG#Z8288 */
/*
 * Restore reserve that was borrowed only for temp files under one active
 * target.  The counters hold the actual bytes released from reserve files, not
 * the requested write delta, because sparse reserve files may release less.
 */
static void
restore_active_min_quota_temp_release(ActiveMinQuotaTarget *target)
{
	if (target == NULL)
		return;

	/* No-op when this target did not borrow reserve for temp files. */
	restore_min_quota_reserve_space(NAMESPACE_QUOTA,
									target->namespaceoid,
									target->namespace_temp_released);
	restore_min_quota_reserve_space(ROLE_QUOTA,
									target->owneroid,
									target->role_temp_released);
	target->namespace_temp_released = 0;
	target->role_temp_released = 0;
}

/*
 * Track the min-quota target currently allowed to borrow reserve space.
 *
 * FileWrite() does not know which relation or schema a write belongs to, so
 * executor and utility hooks push the target before running code that may
 * extend files.  TopMemoryContext keeps the frame alive across executor memory
 * context resets until ExecutorEnd/ProcessUtility cleanup pops it.
 */
static void
push_active_min_quota_target(Oid namespaceoid, Oid owneroid, void *owner)
{
	ActiveMinQuotaTarget *target;
	MemoryContext oldcontext;

	if (!OidIsValid(namespaceoid) && !OidIsValid(owneroid))
		return;

	/* The frame may outlive executor-local memory contexts. */
	oldcontext = MemoryContextSwitchTo(TopMemoryContext);
	target = palloc(sizeof(ActiveMinQuotaTarget));
	MemoryContextSwitchTo(oldcontext);

	target->namespaceoid = namespaceoid;
	target->owneroid = owneroid;
	target->namespace_temp_released = 0;
	target->role_temp_released = 0;
	target->owner = owner;
	target->prev = active_min_quota_target;
	active_min_quota_target = target;
}

/*
 * Leave the active min-quota target and restore any reserve temporarily
 * borrowed for temp files.  Persistent relation writes keep the borrowed space,
 * because their new file blocks are now part of the target's real usage.
 */
static void
pop_active_min_quota_target(void *owner)
{
	ActiveMinQuotaTarget *target = active_min_quota_target;

	if (target == NULL)
		return;

	Assert(owner == NULL || target->owner == owner);

	/* Only temp-file borrows are reversible at statement cleanup time. */
	restore_active_min_quota_temp_release(target);
	active_min_quota_target = target->prev;
	pfree(target);
}

/*
 * Transaction-end safety net for active min-quota frames.
 *
 * Normal cleanup should still happen in ExecutorEnd or ProcessUtility.  This
 * callback prevents a missed path from leaving TopMemoryContext frames and stale
 * FileWrite attribution behind for later transactions in the same backend.
 */
static void
dq_xact_callback(XactEvent event, void *arg)
{
	switch (event)
	{
		case XACT_EVENT_COMMIT:
		case XACT_EVENT_PARALLEL_COMMIT:
		case XACT_EVENT_ABORT:
		case XACT_EVENT_PARALLEL_ABORT:
		case XACT_EVENT_PREPARE:
			while (active_min_quota_target != NULL)
				pop_active_min_quota_target(NULL);
			break;
		default:
			break;
	}
}

/*
 * Resolve the quota target for DML statements.
 *
 * The FileWrite hook only sees storage writes.  For INSERT/UPDATE/DELETE, the
 * result relation is the object whose namespace and owner should be allowed to
 * consume its min-quota reserve when the filesystem is otherwise full.
 */
static bool
get_query_min_quota_target(QueryDesc *queryDesc, Oid *namespaceoid, Oid *owneroid)
{
	PlannedStmt *plannedstmt;
	RangeTblEntry *rte;
	Relation	rel;
	int			rtindex;

	if (queryDesc == NULL || queryDesc->plannedstmt == NULL)
		return false;

	plannedstmt = queryDesc->plannedstmt;
	if (plannedstmt->commandType != CMD_INSERT &&
		plannedstmt->commandType != CMD_UPDATE &&
		plannedstmt->commandType != CMD_DELETE)
		return false;

	if (plannedstmt->resultRelations == NIL)
		return false;

	/* Use the first result relation as the storage owner for this DML. */
	rtindex = linitial_int(plannedstmt->resultRelations);
	rte = rt_fetch(rtindex, plannedstmt->rtable);
	if (rte == NULL || rte->rtekind != RTE_RELATION || !OidIsValid(rte->relid))
		return false;

	rel = table_open(rte->relid, AccessShareLock);
	*namespaceoid = rel->rd_rel->relnamespace;
	*owneroid = rel->rd_rel->relowner;
	table_close(rel, AccessShareLock);
	return true;
}

/*
 * Resolve the quota target for utility statements that create or extend
 * relation storage before regular quota refresh catches up.
 */
static bool
get_utility_min_quota_target(Node *parsetree, Oid *namespaceoid, Oid *owneroid)
{
	Relation rel;

	if (parsetree == NULL)
		return false;

	switch (nodeTag(parsetree))
	{
		case T_CreateStmt:
			{
				CreateStmt *stmt = (CreateStmt *) parsetree;

				*namespaceoid = RangeVarGetCreationNamespace(stmt->relation);
				*owneroid = GetUserId();
				return true;
			}
		case T_CreateSeqStmt:
			{
				CreateSeqStmt *stmt = (CreateSeqStmt *) parsetree;

				*namespaceoid = RangeVarGetCreationNamespace(stmt->sequence);
				*owneroid = stmt->ownerId ? stmt->ownerId : GetUserId();
				return true;
			}
		case T_IndexStmt:
			{
				IndexStmt *stmt = (IndexStmt *) parsetree;
				Oid		relid;

				relid = RangeVarGetRelid(stmt->relation, AccessShareLock, false);
				rel = table_open(relid, AccessShareLock);
				*namespaceoid = rel->rd_rel->relnamespace;
				*owneroid = rel->rd_rel->relowner;
				table_close(rel, AccessShareLock);
				return true;
			}
		case T_CreateTableAsStmt:
			{
				CreateTableAsStmt *stmt = (CreateTableAsStmt *) parsetree;

				if (stmt->into == NULL || stmt->into->rel == NULL)
					return false;

				/*
				 * CTAS inserts through DestIntoRel while the utility command is
				 * running, so the target must come from the IntoClause rather
				 * than ExecutorRun's SELECT plan.
				 */
				*namespaceoid = RangeVarGetCreationNamespace(stmt->into->rel);
				*owneroid = GetUserId();
				return true;
			}
		default:
			return false;
	}
}

/*
 * Identify PostgreSQL temp files by their path component.  fd.c does not expose
 * the VFD temp flag to extensions, so the plugin uses the stable pgsql_tmp
 * directory/prefix convention.
 */
static bool
is_query_temp_file(File file)
{
	const char *path = FilePathName(file);

	return path != NULL && strstr(path, PG_TEMP_FILE_PREFIX) != NULL;
}

/*
 * ExecutorRun hook for DML file growth.
 *
 * Pushes the target before executor writes can reach FileWrite().  Successful
 * execution is popped in ExecutorEnd so temp files can be cleaned up before
 * borrowed temp reserve is restored.
 */
static void
dq_executor_run(QueryDesc *queryDesc, ScanDirection direction,
				uint64 count, bool execute_once)
{
	Oid namespaceoid = InvalidOid;
	Oid owneroid = InvalidOid;
	bool pushed = false;

	if (diskquota_enable != AUOTA_ON)
	{
		if (prev_ExecutorRun_hook)
			prev_ExecutorRun_hook(queryDesc, direction, count, execute_once);
		else
			standard_ExecutorRun(queryDesc, direction, count, execute_once);
		return;
	}

	if (get_query_min_quota_target(queryDesc, &namespaceoid, &owneroid))
	{
		/* Keep the target active until ExecutorEnd on the success path. */
		push_active_min_quota_target(namespaceoid, owneroid, queryDesc);
		pushed = true;
	}

	PG_TRY();
	{
		if (prev_ExecutorRun_hook)
			prev_ExecutorRun_hook(queryDesc, direction, count, execute_once);
		else
			standard_ExecutorRun(queryDesc, direction, count, execute_once);
	}
	PG_CATCH();
	{
		/* Avoid leaving this backend attributing later writes to this query. */
		if (pushed && active_min_quota_target != NULL &&
			active_min_quota_target->owner == queryDesc)
			pop_active_min_quota_target(queryDesc);
		PG_RE_THROW();
	}
	PG_END_TRY();
}

/*
 * ExecutorEnd hook used as the normal cleanup point for executor targets.
 */
static void
dq_executor_end(QueryDesc *queryDesc)
{
	if (diskquota_enable != AUOTA_ON)
	{
		if (prev_ExecutorEnd_hook)
			prev_ExecutorEnd_hook(queryDesc);
		else
			standard_ExecutorEnd(queryDesc);
		return;
	}

	PG_TRY();
	{
		if (prev_ExecutorEnd_hook)
			prev_ExecutorEnd_hook(queryDesc);
		else
			standard_ExecutorEnd(queryDesc);
	}
	PG_FINALLY();
	{
		/* Run after standard ExecutorEnd so temp file cleanup has happened. */
		if (active_min_quota_target != NULL &&
			active_min_quota_target->owner == queryDesc)
			pop_active_min_quota_target(queryDesc);
	}
	PG_END_TRY();
}

/*
 * Storage write hook that releases min-quota reserve just before real file
 * extension writes consume filesystem blocks.
 *
 * For temp files, the amount actually released is recorded so it can be
 * restored after the temp file is removed.  For persistent files, reserve stays
 * released because the target's real usage has grown.
 */
static void
dq_filewrite_hook(File file, int amount, off_t offset,
				  uint32 wait_event_info)
{
	off_t		current_size;
	off_t		past_write;
	int64		delta;
	bool		is_temp;
	int64		released;
	const char *path;

	if (prev_FileWrite_hook)
		prev_FileWrite_hook(file, amount, offset, wait_event_info);

	if (diskquota_enable != AUOTA_ON || active_min_quota_target == NULL)
		return;

	/* Ignore diskquota's own reserve-file writes. */
	path = FilePathName(file);
	if (path && strstr(path, MIN_QUOTA_RESERVE_DIR))
		return;

	current_size = FileSize(file);
	if (current_size < 0)
		return;

	/* Reserve is needed only for writes that extend the file. */
	past_write = offset + amount;

	delta = past_write - current_size;
	if (delta <= 0)
		return;

	is_temp = is_query_temp_file(file);

	/* Namespace and role min quotas are independent reserve files. */
	released = release_min_quota_reserve_space(NAMESPACE_QUOTA,
											   active_min_quota_target->namespaceoid,
											   delta);
	if (is_temp)
		active_min_quota_target->namespace_temp_released += released;

	released = release_min_quota_reserve_space(ROLE_QUOTA,
											   active_min_quota_target->owneroid,
											   delta);
	if (is_temp)
		active_min_quota_target->role_temp_released += released;
}
/* End - BUG#Z8288 */

/*
 * ProcessUtility hook used to refresh quota state after utility commands that
 * can shrink or rewrite relation storage.
 */
static void
dq_process_utility(PlannedStmt *pstmt, const char *queryString,
				   bool readOnlyTree, ProcessUtilityContext context,
				   ParamListInfo params, QueryEnvironment *queryEnv,
				   DestReceiver *dest, QueryCompletion *qc)
{
	Node *parsetree = pstmt->utilityStmt;
	/* Begin - BUG#Z8288 */
	Oid namespaceoid = InvalidOid;
	Oid owneroid = InvalidOid;
	bool pushed = false;

	if (diskquota_enable != AUOTA_ON)
	{
		if (prev_ProcessUtility_hook)
			prev_ProcessUtility_hook(pstmt, queryString, readOnlyTree, context,
									 params, queryEnv, dest, qc);
		else
			standard_ProcessUtility(pstmt, queryString, readOnlyTree, context,
									params, queryEnv, dest, qc);
		return;
	}

	if (!IsBackgroundWorker &&
		!creating_extension &&
		get_utility_min_quota_target(parsetree, &namespaceoid, &owneroid))
	{
		/* Utility storage writes use the object being created or indexed. */
		push_active_min_quota_target(namespaceoid, owneroid, parsetree);
		pushed = true;
	}

	PG_TRY();
	{
		if (prev_ProcessUtility_hook)
			prev_ProcessUtility_hook(pstmt, queryString, readOnlyTree, context,
									 params, queryEnv, dest, qc);
		else
			standard_ProcessUtility(pstmt, queryString, readOnlyTree, context,
									params, queryEnv, dest, qc);
	}
	PG_FINALLY();
	{
		/* Utility commands are not paired with ExecutorEnd. */
		if (pushed)
			pop_active_min_quota_target(parsetree);
	}
	PG_END_TRY();
	/* End - BUG#Z8288 */

	if (IsBackgroundWorker || creating_extension || parsetree == NULL)
		return;

	/* Only utility commands with immediate storage-size effects are handled. */
	switch (nodeTag(parsetree))
	{
		case T_TruncateStmt:
			{
				TruncateStmt *stmt = (TruncateStmt *) parsetree;
				ListCell   *lc;

				foreach(lc, stmt->relations)
				{
					RangeVar *rv = lfirst_node(RangeVar, lc);
					Oid relid = RangeVarGetRelid(rv, AccessShareLock, true);

					if (OidIsValid(relid))
						refresh_relation_quota_state(relid);
				}
			}
			break;
		case T_VacuumStmt:
			{
				VacuumStmt *stmt = (VacuumStmt *) parsetree;
				ListCell   *lc;

				if (!stmt->is_vacuumcmd || stmt->rels == NIL)
					break;

				foreach(lc, stmt->rels)
				{
					VacuumRelation *vrel = lfirst_node(VacuumRelation, lc);
					Oid relid = vrel->oid;

					if (!OidIsValid(relid) && vrel->relation != NULL)
						relid = RangeVarGetRelid(vrel->relation, AccessShareLock, true);

					if (OidIsValid(relid))
						refresh_relation_quota_state(relid);
				}
			}
			break;
		default:
			break;
	}
}

static const char *err_code_to_err_message(DiskquotaMessageResult code)
{
	switch (code)
	{
		case ERR_PENDING: return "no response from launcher, or timeout";
		case ERR_OK: return "NO ERROR";
		case ERR_EXCEED: return "too many database to monitor";
		case ERR_ADD_TO_DB: return "add dbid to diskquota_database_list failed";
		case ERR_START_WORKER: return "start worker failed";
		case ERR_INVALID_DBID: return "invalid dbid";
		default: return "unknown error";
	}
}

/* [1]:(db1, pid),[2]:(db2, pid)*/
void refresh_box_message(void)
{
	Oid db_oid = MyDatabaseId;
	int i = 0;

	if (diskquota_strict == AUOTA_OFF)
		return ;

	LWLockAcquire(diskquota_locks.message_box_lock, LW_EXCLUSIVE);
	/* save every one dboid and pid */
	for (i = 0; i < message_box->db_num; i++)
	{
		/* new */
		if (message_box->dboid_pid_info[i].dboid == InvalidOid)
		{
			message_box->dboid_pid_info[i].dboid = db_oid;
			message_box->dboid_pid_info[i].pid = getpid();
			break;
		}
		/* saved */
		else if (message_box->dboid_pid_info[i].dboid == db_oid)
			break;
	}

	LWLockRelease(diskquota_locks.message_box_lock);
}

static void
init_box_message(void)
{
	Oid db_oid = MyDatabaseId;
	int i = 0;

	LWLockAcquire(diskquota_locks.message_box_lock, LW_EXCLUSIVE);
	/* save every one dboid and pid */
	for (i = 0; i < QUOTA_MAX_DB_NUM; i++)
	{
		if (message_box->dboid_pid_info[i].dboid == db_oid)
		{
			message_box->dboid_pid_info[i].pid = 0;
			message_box->dboid_pid_info[i].dboid = InvalidOid;
			break;
		}
	}

	LWLockRelease(diskquota_locks.message_box_lock);
}

static void diskquota_free_node(DiskQuotaSessionPids *node)
{
	DiskQuotaSessionPids *head_node = NULL;

	node->use_next = NULL;
	node->use_prev = NULL;
	node->pid = 0;
	node->procLatch = NULL;
	node->deal = false;
	node->dbid = InvalidOid;

	/* only one node */
	if (message_box->free_pid_list == NULL)
	{
		message_box->free_pid_list = node;
		node->free_next = node;
		node->free_prev = node;
	}
	else
	{
		head_node = message_box->free_pid_list;
		head_node->free_prev->free_next = node;
		node->free_prev = head_node->free_prev;
		head_node->free_prev = node;
		node->free_next = head_node;
	}

	return;
}

static void diskquota_send_sig_to_session(void)
{
	DiskQuotaSessionPids *session_cur = NULL;
	DiskQuotaSessionPids *session_next = NULL;

	if (diskquota_strict == AUOTA_OFF)
		return;

	if (tail == NULL || tail == message_box->session_pid_list)
		return;

	/* Begin - BUG#Z7634 */
	if (!message_box ||
		!message_box->session_pid_list ||
		!message_box->session_pid_list->use_next)
		return;
	/* End - BUG#Z7634 */

	session_cur = message_box->session_pid_list->use_next;

	while (session_cur && session_cur != tail->use_next) /* BUG#Z7634 */
	{
		session_next = session_cur->use_next;

		/* send sig */
		if (session_cur->procLatch != NULL && session_cur->deal == true && session_cur->dbid == MyDatabaseId)
		{
			SetLatch(session_cur->procLatch);
			delete_pid_from_queue(session_cur);
		}
		else if (session_cur->procLatch != NULL && session_cur->dbid == MyDatabaseId)
		{
			session_cur->deal = true;
			SetLatch(session_cur->procLatch);
		}
		else if (tail->procLatch== NULL)
		{
			break;
		}
		session_cur = session_next;
	}
	tail = NULL;
}

void delete_pid_from_queue(DiskQuotaSessionPids *node)
{
	DiskQuotaSessionPids *session_head = message_box->session_pid_list;

	LWLockAcquire(diskquota_locks.message_box_lock, LW_EXCLUSIVE);

	session_head->use_next = node->use_next;
	node->use_next->use_prev = node->use_prev;

	/* set node to free list */
	diskquota_free_node(node);

	LWLockRelease(diskquota_locks.message_box_lock);
}

static void
delete_role_schema_quota(Oid objectId, QuotaType quotatype)
{
	TableScanDesc	scandesc;
	ScanKeyData		scanKey[2];
	HeapTuple		tup;
	Relation		rel;
	bool isnull = false;
	Datum		target_oid_datum;
	Datum quotatype_datum;
	Oid diskquota_config = InvalidOid;

	diskquota_config = get_relname_relid(QUOTA_CONFIG, SECURITY_SCHEMA_NAMESPACE);
	if (diskquota_config == InvalidOid)
		return ;

	ScanKeyInit(&scanKey[0],
				1,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(objectId));

	ScanKeyInit(&scanKey[1],
				2,
				BTEqualStrategyNumber, F_INT4EQ,
				Int32GetDatum((int)quotatype));

	rel = table_open(diskquota_config, RowExclusiveLock);
	scandesc = table_beginscan(rel, SnapshotSelf, 2, scanKey);

	/* only one tuple will be found */
	while ((tup = heap_getnext(scandesc, ForwardScanDirection)) != NULL)
	{
		if(HeapTupleIsValid(tup))
		{
			quotatype_datum = heap_getattr(tup, 2, RelationGetDescr(rel), &isnull);
			target_oid_datum = heap_getattr(tup, 1, RelationGetDescr(rel), &isnull);
			if (isnull || ((QuotaType) DatumGetUInt32(quotatype_datum) == ROLE_QUOTA &&
					objectId == DatumGetObjectId(target_oid_datum)))
			{
				simple_heap_delete(rel, &tup->t_self);
			}
		}
	}

	table_endscan(scandesc);
	table_close(rel, RowExclusiveLock);

}
