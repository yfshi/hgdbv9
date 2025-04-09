/* -------------------------------------------------------------------------
 *
 * worker_pg_pdr.c
 *		Sample background worker code that demonstrates various coding
 *		patterns: establishing a database connection; starting and committing
 *		transactions; using GUC variables, and heeding SIGHUP to reread
 *		the configuration file; reporting to pg_stat_activity; using the
 *		process latch to sleep and exit in case of postmaster death.
 *
 * This code connects to a database, creates a schema and table, and summarizes
 * the numbers contained therein.  To see it working, insert an initial value
 * with "total" type and some initial value; then insert some other rows with
 * "delta" type.  Delta rows will be deleted by this worker and their values
 * aggregated into the total.
 *
 * Copyright:
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *		contrib/worker_pg_pdr/worker_pg_pdr.c
 *
 * -------------------------------------------------------------------------
 */
#include "postgres.h"

/* These are always necessary for a bgworker */
#include "miscadmin.h"
#include "postmaster/bgworker.h"
#include "postmaster/bgworker_internals.h"
#include "storage/ipc.h"
#include "storage/latch.h"
#include "storage/lwlock.h"
#include "storage/proc.h"
#include "storage/shmem.h"

/* these headers are used by this particular worker's code */
#include "access/xact.h"
#include "executor/spi.h"
#include "access/genam.h"
#include "access/table.h"
#include "fmgr.h"
#include "lib/stringinfo.h"
#include "pgstat.h"
#include "utils/builtins.h"
#include "utils/snapmgr.h"
#include "tcop/utility.h"
#include "utils/memutils.h"
#include "storage/fd.h"
#include "port.h"
#include "catalog/pg_class.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_database.h"
#include "catalog/indexing.h"
#include "utils/fmgroids.h"
#include "access/xlog.h"
#include "miscadmin.h"
#ifdef WIN32
#include <windows.h>
#include <process.h>
#include <time.h>
#else

#include <linux/kernel.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#endif


PG_MODULE_MAGIC;


/* ----------
 * The initial size hints for the hash tables used in the collector.
 * ----------
 */
#define PGPDR_DB_HASH_SIZE	512
#define PGPDR_TAB_HASH_SIZE	512
#define PGPDR_FUNCTION_HASH_SIZE	512
#define PGPDR_SQL_MAX    10

typedef int64 PgPdr_Counter;

static MemoryContext pgPdrLocalContext = NULL;
static MemoryContext pgPdrLocalContext_once = NULL;

void		_PG_init(void);
void		pg_pdr_main(Datum) pg_attribute_noreturn();
void		bgworker_conn4once(void);


int find_replace_queryid(uint64 id);
static bool insert_new_snap(void);

static void pg_dummy_handler(SIGNAL_ARGS);

extern PGDLLEXPORT char * get_pgstat_stat_directory();
extern PGDLLEXPORT pid_t get_BackgroundWorkerData_pid_pdr();
extern PGDLLEXPORT pid_t get_BackgroundWorkerData_pid_wait();

/* flags set by signal handlers */
static volatile sig_atomic_t got_sighup = false;
static volatile sig_atomic_t got_sigterm = false;

/* GUC variables */
static int	pg_pdr_naptime = 60;
static int	pg_pdr_naplife = 7;
static char	*pg_pdr_napdb;

bool	isSnaping = false;

typedef struct worktable
{
	const char *schema;
	const char *name;
} worktable;

/* Begin- ReqID:SRS-MISC-PDR */
/*
 * Global statistics for pg_stat_statements
 */
typedef struct pgssGlobalStats
{
        int64		dealloc;	/* # of times entries were deallocated */
        TimestampTz 	stats_reset;	/* timestamp with all stats reset */
} pgssGlobalStats;
/* End - ReqID:SRS-MISC-PDR */

typedef struct pgssSharedState
{
	LWLock	*lock;			/* protects hashtable search/modification */
	double	cur_median_usage;	/* current median usage in hashtable */
	Size	mean_query_len; 	/* current mean entry text length */
	slock_t	mutex;			/* protects following fields only: */
	Size	extent;			/* current extent of query file */
	int	n_writers;		/* number of active writers to query file */
	int	gc_count;		/* query file garbage collection cycle count */

	/* ReqID:SRS-MISC-PDR */
	pgssGlobalStats stats;          /* global statistics for pgss */

} pgssSharedState;

typedef struct pdr_new_snap_info
{
	int	flag;		/* 10 in pg_pdr_new_snap() before SIGHUP*/
				/* 11 when got_sighup is true after WaitLatch() in pg_pdr_main() */
				/* 12 after new snapshot was created in insert_new_snap() */
				/* 13 after pg_pdr_new_snap() get the new snap_id*/
	int64 	pdr_new_sid;	/* new snap_id */
} pdr_new_snap_info;


/* all of the queryid */
uint64	queryid_all[PGPDR_SQL_MAX*6]={0};

HTAB	*localtables = NULL;
HTAB	*localfuncs = NULL;

MemoryContext	old_context;


#ifdef WIN32

time_t SystemTimeToTimet(SYSTEMTIME st)
{
	struct tm gm = {st.wSecond, st.wMinute, st.wHour, st.wDay, st.wMonth-1, st.wYear-1900, st.wDayOfWeek, 0, 0};
	return mktime(&gm);
}

#endif

/*
 * Signal handler for SIGTERM
 *		Set a flag to let the main loop to terminate, and set our latch to wake
 *		it up.
 */
static void
pg_pdr_sigterm(SIGNAL_ARGS)
{
	int	save_errno = errno;

	if (!proc_exit_inprogress)
	{
		InterruptPending = true;
		ProcDiePending = true;
	}

	got_sigterm = true;

	SetLatch(MyLatch);

	errno = save_errno;
}

static void
pg_pdr_sigterm_once(SIGNAL_ARGS)
{
	int	save_errno = errno;

	if (!proc_exit_inprogress)
	{
		InterruptPending = true;
		ProcDiePending = true;
	}
	errno = save_errno;
}

/*
 * Signal handler for SIGHUP
 *		Set a flag to tell the main loop to reread the config file, and set
 *		our latch to wake it up.
 */
static void
pg_pdr_sighup(SIGNAL_ARGS)
{
	int	save_errno = errno;

	got_sighup = true;
	SetLatch(MyLatch);
	errno = save_errno;

	return;
}
static void
pg_pdr_sighup_once(SIGNAL_ARGS)
{
	int	save_errno = errno;
	errno = save_errno;

	return;
}

static void
pg_dummy_handler(SIGNAL_ARGS)
{
	;
}


/*
 * Initialize workspace for a worker process: create the schema if it doesn't
 * already exist.
 */
static void
initialize_worker_spi()
{
	int	ret;
	int	ntup;
	bool	isnull;
	StringInfoData buf;
	bool 	allok;
	
	Relation	catalog_rs = NULL;
	SysScanDesc 	scan = NULL;
	ScanKeyData 	skey;
	HeapTuple	tuple = NULL;

	SetCurrentStatementStartTimestamp();
	StartTransactionCommand();
	SPI_connect();
	PushActiveSnapshot(GetTransactionSnapshot());
	pgstat_report_activity(STATE_RUNNING, NULL);

	/* CREATE EXTENSION pg_stat_statements and pg_wait_sampling first */
	catalog_rs = table_open(RelationRelationId, AccessShareLock);  /* ReqID:SRS-MISC-PDR */
	ScanKeyInit(&skey,
	 			Anum_pg_class_relname,
				BTEqualStrategyNumber, F_NAMEEQ,
				CStringGetDatum("pg_stat_statements"));
	scan = systable_beginscan(catalog_rs, InvalidOid, false, NULL, 1, &skey);
	tuple = systable_getnext(scan);
	if (!HeapTupleIsValid(tuple))
	{
		initStringInfo(&buf);
		appendStringInfo(&buf, "CREATE EXTENSION %s", "pg_stat_statements schema public");
		ret = SPI_execute(buf.data, false, 0);
	}
	systable_endscan(scan);

	ScanKeyInit(&skey,
				Anum_pg_class_relname,
				BTEqualStrategyNumber, F_NAMEEQ,
				CStringGetDatum("pg_wait_sampling_current"));
	scan = systable_beginscan(catalog_rs, InvalidOid, false, NULL, 1, &skey);
	tuple = systable_getnext(scan);
	if (!HeapTupleIsValid(tuple))
	{
		initStringInfo(&buf);
		appendStringInfo(&buf, "CREATE EXTENSION %s", "pg_wait_sampling schema public");
		ret = SPI_execute(buf.data, false, 0);
	}
	systable_endscan(scan);

	table_close(catalog_rs, AccessShareLock);  /* ReqID:SRS-MISC-PDR */

	catalog_rs = table_open(ProcedureRelationId, AccessShareLock);  /* ReqID:SRS-MISC-PDR */
	ScanKeyInit(&skey,
				Anum_pg_proc_proname,
				BTEqualStrategyNumber, F_NAMEEQ,
				CStringGetDatum("pg_pdr_new_snap"));
	scan = systable_beginscan(catalog_rs, InvalidOid, false, NULL, 1, &skey);
	tuple = systable_getnext(scan);
	if (!HeapTupleIsValid(tuple))
	{
		initStringInfo(&buf);
		appendStringInfo(&buf, "CREATE EXTENSION %s", "worker_pg_pdr schema public");
		ret = SPI_execute(buf.data, false, 0);
	}

	systable_endscan(scan);
	table_close(catalog_rs, AccessShareLock);   /* ReqID:SRS-MISC-PDR */

	/* XXX could we use CREATE SCHEMA IF NOT EXISTS? */
	initStringInfo(&buf);
	appendStringInfo(&buf, "select count(*) from pg_namespace where nspname = '%s'", "_pg_pdr_");

	ret = SPI_execute(buf.data, true, 0);
	if (ret != SPI_OK_SELECT)
	{
		elog(FATAL, "SPI_execute failed, %s : error code %d", buf.data, ret);
	}

	if (SPI_processed != 1)
		elog(FATAL, "not a singleton result");

	ntup = DatumGetInt64(SPI_getbinval(SPI_tuptable->vals[0],
						   SPI_tuptable->tupdesc,
						   1, &isnull));
	if (isnull)
		elog(FATAL, "null result");

	if (ntup > 0)
	{

		resetStringInfo(&buf);
		appendStringInfo(&buf,
				"select to_regclass('%s.%s') is not null and to_regclass('%s.%s') is not null and \
				to_regclass('%s.%s') is not null and to_regclass('%s.%s') is not null;",
				"_pg_pdr_", "pdrwait", "_pg_pdr_", "pdrtable", "_pg_pdr_", "pdrsql", "_pg_pdr_", "pdrdb");

		ret = SPI_execute(buf.data, true, 0);
		if (ret != SPI_OK_SELECT)
		{
			elog(FATAL, "SPI_execute failed, %s : error code %d", buf.data, ret);
		}

		if (SPI_processed != 1)
			elog(FATAL, "not a singleton result");

		allok = DatumGetBool(SPI_getbinval(SPI_tuptable->vals[0],
							 SPI_tuptable->tupdesc,
							 1, &isnull));
		/* 
		 * if one or more than one table is not exist
		 * drop the schema and the tables
		 * then create the new schema and the tables again
		 */
		if (!allok)
		{
			resetStringInfo(&buf);
			appendStringInfo(&buf,"drop schema %s cascade;", "_pg_pdr_");
			ret = SPI_execute(buf.data, false, 0);
			if (ret != SPI_OK_UTILITY)
			{
				elog(FATAL, "SPI_execute failed, %s : error code %d", buf.data, ret);
			}

			ntup = 0;
		}
	}

	/* there is not a _pg_pdr_ schema */
	if (ntup == 0)
	{
		resetStringInfo(&buf);
		appendStringInfo(&buf,
				 "CREATE SCHEMA %s; "
				 "create table %s.%s as select 0::int8 snap_id,now() snap_ts,* from public.pg_stat_statements \
				 where 1=2", "_pg_pdr_","_pg_pdr_", "pdrsql");

		ret = SPI_execute(buf.data, false, 0);
		if (ret != SPI_OK_UTILITY)
		{
			elog(FATAL, "SPI_execute failed, %s : error code %d", buf.data, ret);
		}

 		/* pdrdb */
		resetStringInfo(&buf);
		appendStringInfo(&buf,
				"create table %s.%s (snap_id int8,snap_ts timestamptz,dbstart_ts timestamptz,databaseid oid, \
				dbname text,dbsysinfo text,dbidentifier int8,n_xact_commit int8,n_xact_rollback int8, \
				n_blocks_fetched int8,n_blocks_hit int8,n_tuples_returned int8,n_tuples_fetched int8, \
				n_tuples_inserted int8,n_tuples_updated int8,n_tuples_deleted int8,\
				last_autovac_time int8,n_conflict_tablespace int8,n_conflict_lock int8, \
				n_conflict_snapshot int8,n_conflict_bufferpin int8,\
				n_conflict_startup_deadlock int8,n_temp_files int8,n_temp_bytes int8, \
				n_deadlocks int8,n_block_read_time int8,n_block_write_time int8);",
				"_pg_pdr_", "pdrdb");

		ret = SPI_execute(buf.data, false, 0);
		if (ret != SPI_OK_UTILITY)
		{
			elog(FATAL, "SPI_execute failed, %s : error code %d", buf.data, ret);
		}

		/* pdrtable */
		resetStringInfo(&buf);
		appendStringInfo(&buf,
			"create table %s.%s (snap_id int8, snap_ts timestamptz, databaseid oid, tableid oid,numscans int8, \
			tuples_returned int8, tuples_fetched int8,tuples_inserted int8,tuples_updated int8, \
			tuples_deleted int8,tuples_hot_updated int8,n_live_tuples int8,\
			n_dead_tuples int8,changes_since_analyze int8,blocks_fetched int8,blocks_hit int8,vacuum_timestamp int8,\
			vacuum_count int8,autovac_vacuum_timestamp int8,autovac_vacuum_count int8,analyze_timestamp int8,\
			analyze_count int8,autovac_analyze_timestamp int8,autovac_analyze_count int8);",
			"_pg_pdr_", "pdrtable");

		ret = SPI_execute(buf.data, false, 0);
		if (ret != SPI_OK_UTILITY)
		{
			elog(FATAL, "SPI_execute failed, %s : error code %d", buf.data, ret);
		}

		/* pdrwait */
		resetStringInfo(&buf);
		appendStringInfo(&buf,
			"create table %s.%s (snap_id int8, snap_ts timestamptz, pid integer, event_type text, \
			event text, queryid bigint, count bigint);",
			"_pg_pdr_", "pdrwait");

		ret = SPI_execute(buf.data, false, 0);
		if (ret != SPI_OK_UTILITY)
		{
			elog(FATAL, "SPI_execute failed, %s : error code %d", buf.data, ret);
		}

	}

	SPI_finish();
	PopActiveSnapshot();
	CommitTransactionCommand();
	pgstat_report_activity(STATE_IDLE, NULL);
}

/* ----------
 * pgpdr_setup_memcxt() -
 *
 *	Create pgStatLocalContext, if not already done.
 * ----------
 */
static void
pgpdr_setup_memcxt(void)
{
	if (!pgPdrLocalContext)
		pgPdrLocalContext = AllocSetContextCreate(TopMemoryContext,
								   "pg_pdr snapshot",
								   ALLOCSET_DEFAULT_SIZES);
}
static void
pgpdr_setup_memcxt_once(void)
{
	if (!pgPdrLocalContext_once)
		pgPdrLocalContext_once = AllocSetContextCreate(TopMemoryContext,
								   "pg_pdr_once",
								   ALLOCSET_DEFAULT_SIZES);
}

static void
pgpdr_exit(int mode)
{
	MemoryContextSwitchTo(old_context);
	MemoryContextReset(pgPdrLocalContext);
	proc_exit(mode);
}


int find_replace_queryid(uint64 id)
{
	int i;

	for (i = 0; i < PGPDR_SQL_MAX*6; i++)
	{
		if (!queryid_all[i])
			break;
		if (queryid_all[i] == id)
			return i;	 
	}

	queryid_all[i] = id;

	return i;
}

/*
 * return the filename for a DB stat file; filename is the output buffer,
 * of length len.
 */
static void
get_dbstat_filename(bool permanent, bool tempname, Oid databaseid,
					char *filename, int len)
{
	int	printed;
	char	*mypgstat_stat_directory = get_pgstat_stat_directory();
	/* NB -- pgstat_reset_remove_files knows about the pattern this uses */
	printed = snprintf(filename, len, "%s/db_%u.%s",
					   permanent ? PGSTAT_STAT_PERMANENT_DIRECTORY :
					   mypgstat_stat_directory,
					   databaseid,
					   tempname ? "tmp" : "stat");
	if (printed > len)
		elog(ERROR, "overlength pgstat path");
}

void
pg_pdr_main(Datum main_arg)
{
#ifdef WIN32
	SYSTEMTIME	mySysTime;
	time_t		curtime = 0;
	time_t		last_snap_time = 0;
#else
	time_t		curtime = 0;
	time_t		last_snap_time = 0;
#endif
	long		sleep_time = pg_pdr_naptime * 60 * 1000L;
	HASHCTL		hash_ctl;
	FILE		*fpin;
	char		statfile[MAXPGPATH];
	pdr_new_snap_info *shmem_snapinfo=NULL;
	bool		newsnapid_shmem;

	/* Establish signal handlers before unblocking signals. */
	pqsignal(SIGHUP, pg_pdr_sighup);
	pqsignal(SIGTERM, pg_pdr_sigterm);
	pqsignal(SIGUSR1, pg_dummy_handler);
	pqsignal(SIGUSR2, pg_dummy_handler);

	/* We're now ready to receive signals */
	BackgroundWorkerUnblockSignals();

	/* Connect to our database */
	BackgroundWorkerInitializeConnection(pg_pdr_napdb, NULL, 0);

	pgpdr_setup_memcxt();
	old_context = MemoryContextSwitchTo(pgPdrLocalContext);
	
	/* init first */ 
	initialize_worker_spi();
	memset(&hash_ctl, 0, sizeof(hash_ctl));
	hash_ctl.keysize = sizeof(Oid);
	hash_ctl.entrysize = sizeof(PgStat_StatTabEntry);
	hash_ctl.hcxt = pgPdrLocalContext;
	localtables = hash_create("Per-database table",
						  512,
						  &hash_ctl,
						  HASH_ELEM | HASH_BLOBS | HASH_CONTEXT);
			
	hash_ctl.keysize = sizeof(Oid);
	hash_ctl.entrysize = sizeof(PgStat_StatFuncEntry);
	hash_ctl.hcxt = pgPdrLocalContext;
	localfuncs = hash_create("Per-database function",
						 512,
						 &hash_ctl,
						 HASH_ELEM | HASH_BLOBS | HASH_CONTEXT);
	
	get_dbstat_filename(false, false, MyDatabaseId, statfile, MAXPGPATH);
	while((fpin = AllocateFile(statfile, PG_BINARY_R)) == NULL)
	{
		if (got_sigterm)
			pgpdr_exit(1);
		pg_usleep(5000);
	}

	/* for pdr running warning */
	if (NULL != fpin)
	{
		FreeFile(fpin);
	}

#ifdef WIN32
	memset(&mySysTime,0x00,sizeof(SYSTEMTIME));
	GetSystemTime(&mySysTime);
	last_snap_time = SystemTimeToTimet(mySysTime);
#else
	last_snap_time = time(NULL);
#endif

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
		rc = WaitLatch(MyLatch, WL_LATCH_SET | WL_TIMEOUT | WL_POSTMASTER_DEATH, sleep_time, PG_WAIT_EXTENSION);
		ResetLatch(MyLatch);

		if (RecoveryInProgress())
			pgpdr_exit(1);

		/* emergency bailout if postmaster has died */
		if (rc & WL_POSTMASTER_DEATH)
			pgpdr_exit(1);

		if (got_sigterm)
			pgpdr_exit(1);

		CHECK_FOR_INTERRUPTS();

		/*
		 * In case of a SIGHUP
		 */
		sleep_time = pg_pdr_naptime * 60 * 1000L;
#ifdef WIN32
		memset(&mySysTime,0x00,sizeof(SYSTEMTIME));
		GetSystemTime(&mySysTime);
		curtime = SystemTimeToTimet(mySysTime);
#else
		curtime = time(NULL);
#endif
		if (got_sighup)
		{
			/* test shmem pdr_new_snap_info */
			shmem_snapinfo = (pdr_new_snap_info *) ShmemInitStruct("pdr_new_snap_zt",
							   sizeof(pdr_new_snap_info),
							   &newsnapid_shmem);
			if (shmem_snapinfo->flag == 10)
			{
				shmem_snapinfo->flag = 11;
			}
			else 
			{
			}

			sleep_time = (pg_pdr_naptime * 60 * 1000L)-((curtime - last_snap_time)*1000L);
			if (sleep_time < 0)
				sleep_time = 1;
		}
		else
		{
			last_snap_time = curtime;
		}
		/*
		 * Start a transaction on which we can run queries.  Note that each
		 * StartTransactionCommand() call should be preceded by a
		 * SetCurrentStatementStartTimestamp() call, which sets both the time
		 * for the statement we're about the run, and also the transaction
		 * start time.  Also, each other query sent to SPI should probably be
		 * preceded by SetCurrentStatementStartTimestamp(), so that statement
		 * start time is always up to date.
		 *
		 * The SPI_connect() call lets us run queries through the SPI manager,
		 * and the PushActiveSnapshot() call creates an "active" snapshot
		 * which is necessary for queries to have MVCC data to work on.
		 *
		 * The pgstat_report_activity() call makes our activity visible
		 * through the pgstat views.
		 */
		SetCurrentStatementStartTimestamp();
		StartTransactionCommand();
		if ( insert_new_snap() )
		{
			if (got_sighup)
			{
				got_sighup = false;
				if (shmem_snapinfo->flag == 11)
				{
					shmem_snapinfo->flag = 12;
				}
			}
		}
		else 
		{
			if (got_sighup)
			{
				got_sighup = false;
				if (shmem_snapinfo->flag == 11)
				{
					shmem_snapinfo->flag = -1;
				}
			}

		}
		CommitTransactionCommand();
		pgstat_report_stat(false);
		pgstat_report_activity(STATE_IDLE, NULL);
	}
	MemoryContextSwitchTo(old_context);
	MemoryContextReset(pgPdrLocalContext);
	proc_exit(1);
}

static bool
naptime_check_hook(int *newval, void **extra, GucSource source)
{
	int n = *newval;
	if (n < 0)
	{
		return false;
	}
	return true;
}

static bool
naplife_check_hook(int *newval, void **extra, GucSource source)
{
	int n = *newval;
	if ((n > 180) || (n < 1))
	{
		return false;
	}
	return true;
}


static bool 
insert_new_snap(void)
{
	int	ret;
	uint64 	j;
	StringInfoData buf;

	Oid 	dboid[PGPDR_DB_HASH_SIZE];
	char 	*pdr_dbname[PGPDR_DB_HASH_SIZE];
	int 	dbcnt=0;
	int64  	sid=0; 
	HASH_SEQ_STATUS tstat;
	PgStat_StatTabEntry *tabentry;
	PgStat_StatDBEntry *dbentry=NULL;

	bool	found;

	/* database system and hardware information */
	char 	tmpgetvalue[256];
	char 	dbsysinfo[512];
	char 	dbtime[32];
	char 	dbver[64];
	char	*dbver_noother = NULL;
	char 	dbhost[256];
	char 	platform[32];
	char	*platform_noother = NULL;
	char 	dbtime_tmp[33];
	char 	dbver_tmp[65];
	char 	dbhost_tmp[257];
	char 	platform_tmp[33];
	int 	cpus;
	uint64 	dbmem;
	char 	cpus_tmp[17];
	char 	dbmem_tmp[17];
	pid_t	pdrpid = 0;
	int	length_val = 0;
	
#ifdef WIN32
	SYSTEM_INFO sSysInfo;
	unsigned long uname_length = 256;
	MEMORYSTATUSEX statex;
#else
	struct	sysinfo s_info;
#endif
	pdrpid = get_BackgroundWorkerData_pid_wait();

	if (pdrpid == 0)
	{
		elog(ERROR, "The backend pg_wait_sampling collector exited unexpected!");
		return false;
	}
	else 
	{
		if (kill(pdrpid, 0) == -1)
		{
			elog(ERROR, "The backend pg_wait_sampling collector exited unexpected!");
			return false;
		}
	}
	
	ShmemInitStruct("pg_stat_statements", sizeof(pgssSharedState), &found);

	if (!found)
	{
		elog(ERROR,"The backend pg_stat_statements exited unexpected!");
		return false;
	}
	
	if (RecoveryInProgress())
		return false;

	if (isSnaping)
		return false;
	isSnaping = true;

	initStringInfo(&buf);

	SPI_connect();
	PushActiveSnapshot(GetTransactionSnapshot());
	pgstat_report_activity(STATE_RUNNING, buf.data);

	/* Does the physical file exist for 4 tables of pdr */
	resetStringInfo(&buf);
	appendStringInfo(&buf,"select pg_stat_file(pg_relation_filepath('_pg_pdr_.pdrdb'));");

	ret = SPI_execute(buf.data, true, 0);
	
	if (ret != SPI_OK_SELECT)
	{
		elog(FATAL, "SPI_execute failed, %s : error code %d", buf.data, ret);
	}

	resetStringInfo(&buf);
	appendStringInfo(&buf,"select pg_stat_file(pg_relation_filepath('_pg_pdr_.pdrsql'));");

	ret = SPI_execute(buf.data, true, 0);

	if (ret != SPI_OK_SELECT)
	{
		elog(FATAL, "SPI_execute failed, %s : error code %d", buf.data, ret);
	}

	resetStringInfo(&buf);
	appendStringInfo(&buf,"select pg_stat_file(pg_relation_filepath('_pg_pdr_.pdrwait'));");

	ret = SPI_execute(buf.data, true, 0);

	if (ret != SPI_OK_SELECT)
	{
		elog(FATAL, "SPI_execute failed, %s : error code %d", buf.data, ret);
	}

	resetStringInfo(&buf);
	appendStringInfo(&buf,"select pg_stat_file(pg_relation_filepath('_pg_pdr_.pdrtable'));");

	ret = SPI_execute(buf.data, true, 0);

	if (ret != SPI_OK_SELECT)
	{
		elog(FATAL, "SPI_execute failed, %s : error code %d", buf.data, ret);
	}

	resetStringInfo(&buf);
	appendStringInfo(&buf,"select (now()-(select max(snap_ts) from _pg_pdr_.pdrdb)) < interval '0 minutes' \
				or ((now()-(select max(snap_ts) from _pg_pdr_.pdrdb) > interval '%d minutes') and \
				((now()-pg_postmaster_start_time()) > interval '%d minutes'))",
				pg_pdr_naptime + 60, pg_pdr_naptime + 1);

	ret = SPI_execute(buf.data, true, 0);
	if (ret != SPI_OK_SELECT)
	{
		elog(FATAL, "SPI_execute failed, %s : error code %d", buf.data, ret);
	}

	if (SPI_processed > 0 && SPI_tuptable != NULL)
	{
		TupleDesc tupdesc = SPI_tuptable->tupdesc;
		bool isnull;
		if (DatumGetBool(SPI_getbinval(SPI_tuptable->vals[0],tupdesc,1,&isnull)))
		{
			elog(WARNING,"OS time error, please clean the _pg_pdr_ or reset OS time, then restart database");
			ResetLatch(MyLatch);
			SPI_finish();
			PopActiveSnapshot();
			isSnaping = false;
			return false;
		}
	}

	/* write the pg_stat_statements to pdrsql */
	resetStringInfo(&buf);
	appendStringInfo(&buf,"select coalesce(max(snap_id),-1) from _pg_pdr_.pdrdb");
	ret = SPI_execute(buf.data, true, 0);
	if (ret != SPI_OK_SELECT)
	{
		elog(FATAL, "SPI_execute failed, %s : error code %d", buf.data, ret);
	}

	if (SPI_processed > 0 && SPI_tuptable != NULL)
	{
		TupleDesc tupdesc = SPI_tuptable->tupdesc;
		bool isnull;
		 
		sid = DatumGetInt64(SPI_getbinval(SPI_tuptable->vals[0],tupdesc,1,&isnull));
		/* sid+1 < 0 error close pdr */
		if (sid + 1 < 0)
		{
			ResetLatch(MyLatch);
			SPI_finish();
			PopActiveSnapshot();
			elog(ERROR, "Pdr error,New snap_id is invalid.snap_id=%ld", sid + 1);
			return false;
		
		}
	}

	/* Check sid+1. Delete invalid data in three tables: pdrwait,pdrtable,pdrsql */
	if (sid + 1 > 0)
	{
		
		resetStringInfo(&buf);
		appendStringInfo(&buf,"delete from _pg_pdr_.pdrwait where snap_id = %ld ;", sid + 1);
		ret = SPI_execute(buf.data, false, 0);
		if (ret != SPI_OK_DELETE)
		{
			elog(FATAL, "SPI_execute failed, %s : error code %d", buf.data, ret);
		}

		resetStringInfo(&buf);
		appendStringInfo(&buf,"delete from _pg_pdr_.pdrtable where snap_id = %ld ;",sid + 1);
		ret = SPI_execute(buf.data, false, 0);
		if (ret != SPI_OK_DELETE)
		{
			elog(FATAL, "SPI_execute failed, %s : error code %d", buf.data, ret);
		}
	
		resetStringInfo(&buf);
		appendStringInfo(&buf,"delete from _pg_pdr_.pdrsql where snap_id = %ld ;",sid + 1);
		ret = SPI_execute(buf.data, false, 0);
		if (ret != SPI_OK_DELETE)
		{
			elog(FATAL, "SPI_execute failed, %s : error code %d", buf.data, ret);
		}
	}

	/* gathering database,system and hardware information */
	memset(dbtime,0x00,sizeof(dbtime));
	memset(platform,0x00,sizeof(platform));
	memset(dbver,0x00,sizeof(dbver));
	memset(dbhost,0x000,sizeof(dbhost));
	memset(tmpgetvalue,0x00,sizeof(tmpgetvalue));
	memset(dbsysinfo,0x00,sizeof(dbsysinfo));

	/* get Platform_for_pdr() length */
	resetStringInfo(&buf);
	appendStringInfo(&buf,"select octet_length(trim(Platform_for_pdr()));");
        ret = SPI_execute(buf.data, true, 0);
        if (ret != SPI_OK_SELECT)
	{
		elog(FATAL, "SPI_execute failed, %s : error code %d", buf.data, ret);
	}
        if (SPI_processed > 0 && SPI_tuptable != NULL)
        {
		bool	isnull;
                TupleDesc tupdesc = SPI_tuptable->tupdesc;
                length_val = DatumGetInt32(SPI_getbinval(SPI_tuptable->vals[0],tupdesc,1,&isnull));
        }
		
	resetStringInfo(&buf);
	appendStringInfo(&buf,"select Platform_for_pdr()");
	ret = SPI_execute(buf.data, true, 0);
	if (ret != SPI_OK_SELECT)
	{
		elog(FATAL, "SPI_execute failed, %s : error code %d", buf.data, ret);
	}

	if (SPI_processed > 0 && SPI_tuptable != NULL)
	{
		bool	isnull;
		TupleDesc tupdesc = SPI_tuptable->tupdesc;
		strncpy(tmpgetvalue,DatumGetCString(SPI_getbinval(SPI_tuptable->vals[0],tupdesc,1,&isnull))+1,length_val);	
	}
	strcpy(platform,tmpgetvalue);

	platform_noother = strtok(platform,"~");

	memset(tmpgetvalue,0x00,sizeof(tmpgetvalue));
	resetStringInfo(&buf);

	/* ReqID:SRS-MISC-PDR */
	/* get highgo_version() length */
	appendStringInfo(&buf,"select octet_length(trim(highgo_version()));");
	ret = SPI_execute(buf.data, true, 0);
	if (ret != SPI_OK_SELECT)
	{
		elog(FATAL, "SPI_execute failed, %s : error code %d", buf.data, ret);
	}
	if (SPI_processed > 0 && SPI_tuptable != NULL)
	{
		bool	isnull;
		TupleDesc tupdesc = SPI_tuptable->tupdesc;
		length_val = DatumGetInt32(SPI_getbinval(SPI_tuptable->vals[0],tupdesc,1,&isnull));
	}

	/* get the highgo_version() */
	resetStringInfo(&buf);
	appendStringInfo(&buf,"select highgo_version()");
	ret = SPI_execute(buf.data, true, 0);
	if (ret != SPI_OK_SELECT)
	{
		elog(FATAL, "SPI_execute failed, %s : error code %d", buf.data, ret);
	}

	if (SPI_processed > 0 && SPI_tuptable != NULL)
	{
		bool	isnull;
		TupleDesc tupdesc = SPI_tuptable->tupdesc;

		strncpy(tmpgetvalue,DatumGetCString(SPI_getbinval(SPI_tuptable->vals[0],tupdesc,1,&isnull))+1,length_val);
	}
	strcpy(dbver,tmpgetvalue);
		
	dbver_noother = strtok(dbver,"~");

	/* pg_postmaster_start_time() */
	memset(tmpgetvalue,0x00,sizeof(tmpgetvalue));
	resetStringInfo(&buf);
	appendStringInfo(&buf,"select to_char(date_trunc('second',pg_postmaster_start_time()),'YYYY-MM-DD HH24:MI:SS') ");
	ret = SPI_execute(buf.data, true, 0);
	if (ret != SPI_OK_SELECT)
	{
		elog(FATAL, "SPI_execute failed, %s : error code %d", buf.data, ret);
	}

	if (SPI_processed > 0 && SPI_tuptable != NULL)
	{
		bool	isnull;
		TupleDesc tupdesc = SPI_tuptable->tupdesc;

		strncpy(tmpgetvalue,DatumGetCString(SPI_getbinval(SPI_tuptable->vals[0],tupdesc,1,&isnull))+1,19);
	}
	strcpy(dbtime,tmpgetvalue);
		
	/* all system cpu num .displayed in report */
#ifdef WIN32
	GetSystemInfo(&sSysInfo); 
	cpus = sSysInfo.dwNumberOfProcessors;
	/* physical memory */
	statex.dwLength = sizeof(statex);
	GlobalMemoryStatusEx(&statex);
	dbmem = statex.ullTotalPhys;
	GetUserName(dbhost,&uname_length);
	if (uname_length > 31)
	{
		/* todo username length check windows */
	}
		
#else
	cpus = get_nprocs_conf();
	sysinfo(&s_info);
	dbmem = s_info.totalram;
	gethostname(dbhost,256);
#endif
	memset(dbtime_tmp,0x00,sizeof(dbtime_tmp));
	memset(dbver_tmp,0x00,sizeof(dbver_tmp));
	memset(dbhost_tmp,0x00,sizeof(dbhost_tmp));
	memset(platform_tmp,0x00,sizeof(platform_tmp));
	memset(cpus_tmp,0x00,sizeof(cpus_tmp));
	memset(dbmem_tmp,0x00,sizeof(dbmem_tmp));

	memset(dbtime_tmp,' ',sizeof(dbtime_tmp)-1);
	memset(dbver_tmp,' ',sizeof(dbver_tmp)-1);
	memset(dbhost_tmp,' ',sizeof(dbhost_tmp)-1);
	memset(platform_tmp,' ',sizeof(platform_tmp)-1);
	memset(cpus_tmp,' ',sizeof(cpus_tmp)-1);
	memset(dbmem_tmp,' ',sizeof(dbmem_tmp)-1);
	
	memcpy(dbtime_tmp + (sizeof(dbtime_tmp) - strlen(dbtime) - 1),dbtime,strlen(dbtime));
	memcpy(dbver_tmp + (sizeof(dbver_tmp) - strlen(dbver_noother) - 1),dbver_noother,strlen(dbver_noother));
	memcpy(dbhost_tmp + (sizeof(dbhost_tmp) - strlen(dbhost) - 1),dbhost,strlen(dbhost));
	memcpy(platform_tmp + (sizeof(platform_tmp) - strlen(platform_noother)-1),platform_noother,strlen(platform_noother));
	memset(tmpgetvalue,0x00,sizeof(tmpgetvalue));
	sprintf(tmpgetvalue,"%d",cpus);
	memcpy(cpus_tmp + (sizeof(cpus_tmp) - strlen(tmpgetvalue) - 1),tmpgetvalue,strlen(tmpgetvalue));
	memset(tmpgetvalue,0x00,sizeof(tmpgetvalue));
	sprintf(tmpgetvalue,"%lu",dbmem);
	memcpy(dbmem_tmp + (sizeof(dbmem_tmp) - strlen(tmpgetvalue) - 1),tmpgetvalue,strlen(tmpgetvalue));
	sprintf(dbsysinfo,"%s%s%s%s%s%s",dbtime_tmp,dbver_tmp,dbhost_tmp,platform_tmp,cpus_tmp,dbmem_tmp);
	
	//this is the first snap
	if (sid < 0)
	{
		resetStringInfo(&buf);

		/* Begin - ReqID:SRS-MISC-PDR */

		/*
  		 * as the pg_stat_statements table in 14 have more columnis than 12
  		 * store all of the columns
 		 */
		appendStringInfo(&buf,"insert into _pg_pdr_.pdrsql \
			select %d,now(),0 userid,statisticpgss.* from ( \
			select \
			dbid, toplevel, queryid, min(query) query, \
			sum(plans) plans,sum(total_plan_time) total_plan_time,min(min_plan_time) min_plan_time, \
			max(max_plan_time) max_plan_time, \
			coalesce(sum(total_plan_time) / NULLIF(sum(plans),0),0) mean_plan_time, \
			sum(stddev_plan_time) stddev_plan_time, \
			sum(calls) calls,sum(total_exec_time) total_exec_time,min(min_exec_time) min_exec_time, \
			max(max_exec_time) max_exec_time, \
			sum(total_exec_time)/sum(calls) mean_exec_time,sum(stddev_exec_time) stddev_exec_time, \
			sum(rows) rows, \
			sum(shared_blks_hit) shared_blks_hit,sum(shared_blks_read) shared_blks_read, \
			sum(shared_blks_dirtied) shared_blks_dirtied, sum(shared_blks_written) shared_blks_written, \
			sum(local_blks_hit) local_blks_hit, sum(local_blks_read) local_blks_read, \
			sum(local_blks_dirtied) local_blks_dirtied, sum(local_blks_written) local_blks_written, \
			sum(temp_blks_read) temp_blks_read, sum(temp_blks_written) temp_blks_written, \
			sum(blk_read_time) blk_read_time, sum(blk_write_time) blk_write_time \
			from public.pg_stat_statements \
			where calls > 0 group by dbid,queryid,toplevel \
			) statisticpgss;",0);

		/* End - ReqID:SRS-MISC-PDR */

		ret = SPI_execute(buf.data, false, 0);
		if (ret != SPI_OK_INSERT)
		{
			elog(FATAL, "SPI_execute failed, %s : error code %d", buf.data, ret);
		}
	}
	else
	{
		resetStringInfo(&buf);
		/*
		 *  stddev_time not used in report and don't know what it means
		 *  so use sum(stddev_time)
		 */
		/* Begin - ReqID:SRS-MISC-PDR */
		/*
		 * store all clomuns in pg_stat_statements
		 */
		appendStringInfo(&buf,"insert into _pg_pdr_.pdrsql \
				select %ld,now(),0 userid,statisticpgss.* from ( \
				select \
				dbid, toplevel, queryid, min(query) query, \
				sum(plans) plans,sum(total_plan_time) total_plan_time,min(min_plan_time) min_plan_time, \
				max(max_plan_time) max_plan_time, \
				coalesce(sum(total_plan_time) / NULLIF(sum(plans),0),0) mean_plan_time, \
				sum(stddev_plan_time) stddev_plan_time, \
				sum(calls) calls,sum(total_exec_time) total_exec_time,min(min_exec_time) min_exec_time, \
				max(max_exec_time) max_exec_time, \
				sum(total_exec_time)/sum(calls) mean_exec_time,sum(stddev_exec_time) stddev_exec_time, \
				sum(rows) rows, \
				sum(shared_blks_hit) shared_blks_hit,sum(shared_blks_read) shared_blks_read, \
				sum(shared_blks_dirtied) shared_blks_dirtied, sum(shared_blks_written) shared_blks_written, \
				sum(local_blks_hit) local_blks_hit, sum(local_blks_read) local_blks_read, \
				sum(local_blks_dirtied) local_blks_dirtied, sum(local_blks_written) local_blks_written, \
				sum(temp_blks_read) temp_blks_read, sum(temp_blks_written) temp_blks_written, \
				sum(blk_read_time) blk_read_time, sum(blk_write_time) blk_write_time \
				from public.pg_stat_statements \
				where calls > 0 group by dbid,queryid,toplevel \
				) statisticpgss;", sid+1);

		/* End - ReqID:SRS-MISC-PDR */

		ret = SPI_execute(buf.data, false, 0);
		if (ret != SPI_OK_INSERT)
		{
			elog(FATAL, "SPI_execute failed, %s : error code %d", buf.data, ret);
		}
	}
		
	resetStringInfo(&buf);
	appendStringInfo(&buf,"select oid from pg_database where datistemplate='f';");
	/* We can now execute queries via SPI */
	ret = SPI_execute(buf.data, true, 0);

	if (ret != SPI_OK_SELECT)
	{
		elog(FATAL, "SPI_execute failed, %s : error code %d", buf.data, ret);
	}

	/* get and save database oids */
	if (SPI_processed > 0 && SPI_tuptable != NULL)
	{
		TupleDesc tupdesc = SPI_tuptable->tupdesc;
		bool		isnull;

		dbcnt = 0;
		//get dboid and add to db hash
		for (j = 0; j < SPI_processed && j < PGPDR_DB_HASH_SIZE; j++)
		{
			dboid[j] = DatumGetObjectId(SPI_getbinval(SPI_tuptable->vals[j],tupdesc,1,&isnull));
			dbcnt++;
		}
	}

	/* pdr_dbname fetch databasename from pg_database */
	resetStringInfo(&buf);
	appendStringInfo(&buf,"select datname from pg_database where datistemplate='f';");
	/* We can now execute queries via SPI */
	ret = SPI_execute(buf.data, true, 0);

	if (ret != SPI_OK_SELECT)
	{
		elog(FATAL, "SPI_execute failed, %s : error code %d", buf.data ,ret);
	}

	/* get and save database oids */
	if (SPI_processed > 0 && SPI_tuptable != NULL)
	{
		TupleDesc tupdesc = SPI_tuptable->tupdesc;
		bool		isnull;
		for (j = 0; j < SPI_processed && j < PGPDR_DB_HASH_SIZE; j++)
		{
			pdr_dbname[j] = DatumGetCString(SPI_getbinval(SPI_tuptable->vals[j],tupdesc,1,&isnull));
		}
	}

	/* for each database get the stat information and save to pdrdb */
	for (j = 0; j < dbcnt; j++)
	{	
		dbentry = pgstat_fetch_stat_dbentry(dboid[j]);
		
		if (dbentry)
		{
			//this is the first snap
			if (sid < 0)
			{
				resetStringInfo(&buf);
				appendStringInfo(&buf,"insert into _pg_pdr_.pdrdb select %d, now(), pg_postmaster_start_time(), \
						%u, '%s', '%s', %ld, %ld, %ld,\
						%ld,%ld,%ld,%ld,%ld,\
						%ld,%ld,%ld,%ld,%ld,\
						%ld,%ld,%ld,%ld,%ld,\
						%ld,%ld,%ld;",
					0,dbentry->databaseid,pdr_dbname[j],dbsysinfo,GetSystemIdentifier(),
					dbentry->n_xact_commit,dbentry->n_xact_rollback, 
					dbentry->n_blocks_fetched, 
					dbentry->n_blocks_hit, 
					dbentry->n_tuples_returned, 
					dbentry->n_tuples_fetched, 
					dbentry->n_tuples_inserted, 
					dbentry->n_tuples_updated, dbentry->n_tuples_deleted, 
					dbentry->last_autovac_time, dbentry->n_conflict_tablespace, dbentry->n_conflict_lock,
					dbentry->n_conflict_snapshot, dbentry->n_conflict_bufferpin, 
					dbentry->n_conflict_startup_deadlock, dbentry->n_temp_files, dbentry->n_temp_bytes, 
					dbentry->n_deadlocks, dbentry->n_block_read_time, dbentry->n_block_write_time);
					
				ret = SPI_execute(buf.data, false, 0);
				if (ret != SPI_OK_INSERT)
				{
					elog(FATAL, "SPI_execute failed, %s: error code %d", buf.data, ret);
				}
			}
			else
			{
				resetStringInfo(&buf);
				appendStringInfo(&buf,"insert into _pg_pdr_.pdrdb select %ld, now(),pg_postmaster_start_time(), \
						%u, '%s', '%s', %ld, \
						%ld,%ld,\
						%ld,%ld,%ld,%ld,%ld,\
						%ld,%ld,%ld,%ld,%ld,\
						%ld,%ld,%ld,%ld,%ld,\
						%ld,%ld,%ld;",
					sid+1,dbentry->databaseid,pdr_dbname[j],dbsysinfo,GetSystemIdentifier(),
					dbentry->n_xact_commit,dbentry->n_xact_rollback, 
					dbentry->n_blocks_fetched, dbentry->n_blocks_hit, dbentry->n_tuples_returned, 
					dbentry->n_tuples_fetched, dbentry->n_tuples_inserted, 
					dbentry->n_tuples_updated, dbentry->n_tuples_deleted, dbentry->last_autovac_time, 
					dbentry->n_conflict_tablespace, dbentry->n_conflict_lock, 
					dbentry->n_conflict_snapshot, dbentry->n_conflict_bufferpin, 
					dbentry->n_conflict_startup_deadlock, dbentry->n_temp_files, dbentry->n_temp_bytes, 
					dbentry->n_deadlocks, dbentry->n_block_read_time, dbentry->n_block_write_time);
					
				ret = SPI_execute(buf.data, false, 0);
				if (ret != SPI_OK_INSERT)
				{
					elog(FATAL, "SPI_execute failed, %s: error code %d", buf.data, ret);
				}
			}
			/* save each table infomation to pdrtable */
			/* this is the first snap */
			if (sid < 0)
			{
				if (dbentry->databaseid != MyDatabaseId)
				{
					pdr_pgstat_read_db_statsfile(dbentry->databaseid, localtables, localfuncs, false);
					hash_seq_init(&tstat, localtables);
				}
				else 
				{
					hash_seq_init(&tstat, dbentry->tables);
				}

				while ((tabentry = (PgStat_StatTabEntry *) hash_seq_search(&tstat)) != NULL)
				{
					resetStringInfo(&buf);
					appendStringInfo(&buf,"insert into _pg_pdr_.pdrtable select %d, now(), %u,%u,%ld,\
						%ld,%ld,%ld,%ld,%ld,\
						%ld,%ld,%ld,%ld,%ld,\
						%ld,%ld,%ld,%ld,%ld,\
						%ld,%ld,%ld,%ld;",
						0,dbentry->databaseid,tabentry->tableid,tabentry->numscans,
						tabentry->tuples_returned,tabentry->tuples_fetched,
						tabentry->tuples_inserted,tabentry->tuples_updated,tabentry->tuples_deleted,
						tabentry->tuples_hot_updated,tabentry->n_live_tuples,
						tabentry->n_dead_tuples,tabentry->changes_since_analyze,tabentry->blocks_fetched,
						tabentry->blocks_hit,tabentry->vacuum_timestamp,tabentry->vacuum_count,
						tabentry->autovac_vacuum_timestamp,tabentry->autovac_vacuum_count,
						tabentry->analyze_timestamp,tabentry->analyze_count,
						tabentry->autovac_analyze_timestamp,tabentry->autovac_analyze_count);
						
					ret = SPI_execute(buf.data, false, 0);
					if (ret != SPI_OK_INSERT)
					{
						elog(FATAL, "SPI_execute failed, %s: error code %d", buf.data, ret);
					}
				}
			}
			else
			{					
				if (dbentry->databaseid != MyDatabaseId)
				{
					pdr_pgstat_read_db_statsfile(dbentry->databaseid, localtables, localfuncs, false);
					hash_seq_init(&tstat, localtables);
				}
				else
				{
					hash_seq_init(&tstat, dbentry->tables);
				}

				while ((tabentry = (PgStat_StatTabEntry *) hash_seq_search(&tstat)) != NULL)
				{

					resetStringInfo(&buf);
					appendStringInfo(&buf,"insert into _pg_pdr_.pdrtable select %ld, now(), %u,%u,%ld,\
						%ld,%ld,%ld,%ld,%ld,\
						%ld,%ld,%ld,%ld,%ld,\
						%ld,%ld,%ld,%ld,%ld,\
						%ld,%ld,%ld,%ld;",
						sid+1,dbentry->databaseid,tabentry->tableid,tabentry->numscans,
						tabentry->tuples_returned,tabentry->tuples_fetched,
						tabentry->tuples_inserted,tabentry->tuples_updated,tabentry->tuples_deleted,
						tabentry->tuples_hot_updated,tabentry->n_live_tuples,
						tabentry->n_dead_tuples,tabentry->changes_since_analyze,tabentry->blocks_fetched,
						tabentry->blocks_hit,tabentry->vacuum_timestamp,tabentry->vacuum_count,
						tabentry->autovac_vacuum_timestamp,tabentry->autovac_vacuum_count,
						tabentry->analyze_timestamp,tabentry->analyze_count,
						tabentry->autovac_analyze_timestamp,tabentry->autovac_analyze_count);
						
					ret = SPI_execute(buf.data, false, 0);
					if (ret != SPI_OK_INSERT)
					{
						elog(FATAL, "SPI_execute failed, %s: error code %d", buf.data, ret);
					}
				}
			}

		}
		else 
		{
			long int zero=0;
			int64 ssid=0;
			ssid = sid<0?0:sid+1;
			resetStringInfo(&buf);
			appendStringInfo(&buf,"insert into _pg_pdr_.pdrdb select %ld, now(),pg_postmaster_start_time(), \
					%u,'%s','%s',%ld,%ld,%ld,\
					%ld,%ld,%ld,%ld,%ld,\
					%ld,%ld,%ld,%ld,%ld,\
					%ld,%ld,%ld,%ld,%ld,\
					%ld,%ld,%ld;",
				ssid,dboid[j],pdr_dbname[j],dbsysinfo,GetSystemIdentifier(),zero,zero, 
				zero, zero, zero, zero, zero, 
				zero, zero, zero, zero, zero, 
				zero, zero, zero, zero, zero, 
				zero, zero, zero);
				
			ret = SPI_execute(buf.data, false, 0);
			if (ret != SPI_OK_INSERT)
				elog(FATAL, "SPI_execute failed, %s: error code %d", buf.data, ret);
		}

	}

	/* write pg_wait_sampling_profile to pdrwait */
	//this is the first snap
	if (sid < 0)
	{
		resetStringInfo(&buf);
		appendStringInfo(&buf,"insert into _pg_pdr_.pdrwait select %d,now(),* from public.pg_wait_sampling_profile;",0);
	
		ret = SPI_execute(buf.data, false, 0);
		if (ret != SPI_OK_INSERT)
			elog(FATAL, "SPI_execute failed, %s: error code %d", buf.data, ret);
	}
	else
	{
		resetStringInfo(&buf);
		appendStringInfo(&buf,"insert into _pg_pdr_.pdrwait select %ld,now(),* from public.pg_wait_sampling_profile;",sid+1);
				
		ret = SPI_execute(buf.data, false, 0);
		if (ret != SPI_OK_INSERT)
			elog(FATAL, "SPI_execute failed, %s: error code %d", buf.data, ret);
	}
	
	/* delete old snaps */ 
	if (true)
	{	
		bool need_delete = false;
		initStringInfo(&buf);
		appendStringInfo(&buf,"select (now()-make_interval(days=>%d))>(select min(snap_ts) from _pg_pdr_.pdrdb)",pg_pdr_naplife);
		ret = SPI_execute(buf.data, true, 0);
		if (ret != SPI_OK_SELECT)
			elog(FATAL, "SPI_execute failed, %s: error code %d", buf.data, ret);

		if (SPI_processed > 0 && SPI_tuptable != NULL)
		{
			TupleDesc tupdesc = SPI_tuptable->tupdesc;
			bool		isnull;
			 
			need_delete = DatumGetBool(SPI_getbinval(SPI_tuptable->vals[0],tupdesc,1,&isnull));
		}
		if (need_delete)
		{
			resetStringInfo(&buf);
			appendStringInfo(&buf,"delete from _pg_pdr_.pdrwait where snap_ts < (select now()-make_interval(days=>%d));",pg_pdr_naplife);
			ret = SPI_execute(buf.data, false, 0);
			if (ret != SPI_OK_DELETE)
				elog(FATAL, "SPI_execute failed, %s : error code %d", buf.data ,ret);

			resetStringInfo(&buf);
			appendStringInfo(&buf,"delete from _pg_pdr_.pdrtable where snap_ts < (select now()-make_interval(days=>%d));",pg_pdr_naplife);
			ret = SPI_execute(buf.data, false, 0);
			if (ret != SPI_OK_DELETE)
				elog(FATAL, "SPI_execute failed, %s : error code %d", buf.data ,ret);
			
			resetStringInfo(&buf);
			appendStringInfo(&buf,"delete from _pg_pdr_.pdrsql where snap_ts < (select now()-make_interval(days=>%d));",pg_pdr_naplife);
			ret = SPI_execute(buf.data, false, 0);
			if (ret != SPI_OK_DELETE)
				elog(FATAL, "SPI_execute failed, %s : error code %d", buf.data ,ret);

			resetStringInfo(&buf);
			appendStringInfo(&buf,"delete from _pg_pdr_.pdrdb where snap_ts < (select now()-make_interval(days=>%d));",pg_pdr_naplife);
			ret = SPI_execute(buf.data, false, 0);
			if (ret != SPI_OK_DELETE)
				elog(FATAL, "SPI_execute failed, %s : error code %d", buf.data ,ret);
		}
	}

	ResetLatch(MyLatch);
		
	/*
	 * And finish our transaction.
	 */
	SPI_finish();
	PopActiveSnapshot();
	isSnaping = false;

	return true;
}

/*
 * Entrypoint of this module.
 *
 * We register more than one worker process here, to demonstrate how that can
 * be done.
 */
void
_PG_init(void)
{
	BackgroundWorker worker;
	BackgroundWorker worker_conn4once;
	

#ifndef WIN32
	/* check can create worker_pg_pdr */
	/*
	 * get the shared_preload_libraries string
	 * then check if there including worker_pg_pdr
	 * if not, raise an error
	 */
	if (NULL == shared_preload_libraries_string || NULL == strstr(shared_preload_libraries_string, "worker_pg_pdr"))
	{
		elog(ERROR, "cannot create worker_pg_pdr extension after startup");
		return;
	}
#endif

	/* get the configuration */
	DefineCustomIntVariable("pg_pdr.naptime",
						"Duration between each check (in minute).",
						NULL,
						&pg_pdr_naptime,
						60,
						0,
						INT_MAX,
						PGC_POSTMASTER,
						0,
						naptime_check_hook,
						NULL,
						NULL);
	DefineCustomIntVariable("pg_pdr.naplife",
						"snap duration (in day).",
						NULL,
						&pg_pdr_naplife,
						7,
						1,
						180,
						PGC_POSTMASTER,
						0,
						naplife_check_hook,
						NULL,
						NULL);

	DefineCustomStringVariable("pg_pdr.napdb",
						"the database name whice save the snap data.", 		
						NULL,
						&pg_pdr_napdb,
						"highgo",
					   	PGC_POSTMASTER,
						0,
						NULL,
						NULL,
						NULL);

	if (!process_shared_preload_libraries_in_progress)
		return;

	if (pg_pdr_naptime == 0)
		return;
	if (strcmp(pg_pdr_napdb,"") == 0 )
	{
		elog(WARNING, "pg_pdr_napdb is null.");
		pg_pdr_napdb = "highgo";
	}
	/* set up common data for all our workers */
	memset(&worker, 0, sizeof(worker));
	worker.bgw_flags = BGWORKER_SHMEM_ACCESS | BGWORKER_BACKEND_DATABASE_CONNECTION;
	worker.bgw_start_time = BgWorkerStart_RecoveryFinished;

	/* BGW_NEVER_RESTART */
	worker.bgw_restart_time = BGW_NEVER_RESTART;
	sprintf(worker.bgw_library_name, "worker_pg_pdr");
	sprintf(worker.bgw_function_name, "pg_pdr_main");
	worker.bgw_notify_pid = 0;

	/*
	 * Now fill in worker-specific data, and do the actual registrations.
	 */
	snprintf(worker.bgw_name, BGW_MAXLEN, "performance diagnosis collector");
	RegisterBackgroundWorker(&worker);

	/* add a backgroundworker to connect to napdb only one time */
	memset(&worker_conn4once, 0, sizeof(worker_conn4once));
	worker_conn4once.bgw_flags = BGWORKER_SHMEM_ACCESS | BGWORKER_BACKEND_DATABASE_CONNECTION;
	worker_conn4once.bgw_start_time = BgWorkerStart_RecoveryFinished;
	/* BGW_NEVER_RESTART connect once */
	worker_conn4once.bgw_restart_time = BGW_NEVER_RESTART;
	sprintf(worker_conn4once.bgw_library_name, "worker_pg_pdr");
	sprintf(worker_conn4once.bgw_function_name, "bgworker_conn4once");
	worker_conn4once.bgw_notify_pid = 0;

	/*
	 * Now fill in worker-specific data, and do the actual registrations.
	 */
	snprintf(worker_conn4once.bgw_name, BGW_MAXLEN, "worker_conn4once");
	RegisterBackgroundWorker(&worker_conn4once);
}

void
bgworker_conn4once(void)
{
	StringInfoData buf;
	char 	tmpgetvalue[256];
	int	ret;
	/* Establish signal handlers before unblocking signals. */
	pqsignal(SIGHUP, pg_pdr_sighup_once);	/* wakeup latch */
	pqsignal(SIGTERM, pg_pdr_sigterm_once);	/* backend */

	/* ReqID:SRS-MISC-PDR */
	pqsignal(SIGUSR1, procsignal_sigusr1_handler);

	/* We're now ready to receive signals */
	BackgroundWorkerUnblockSignals();
	pgpdr_setup_memcxt_once();
	old_context = MemoryContextSwitchTo(pgPdrLocalContext_once);

	/* Connect to our database */
	BackgroundWorkerInitializeConnection(pg_pdr_napdb, NULL, 0);
	SetCurrentStatementStartTimestamp();
	StartTransactionCommand();
	
	initStringInfo(&buf);
	
	SPI_connect();
	PushActiveSnapshot(GetTransactionSnapshot());
	pgstat_report_activity(STATE_RUNNING, buf.data);
	
	appendStringInfo(&buf,"select Platform_for_pdr()");
	ret = SPI_execute(buf.data, true, 0);
	if (ret != SPI_OK_SELECT)
		elog(FATAL, "cannot run select Platform_for_pdr(): error code %d", ret);

	if (SPI_processed > 0 && SPI_tuptable != NULL)
	{
		TupleDesc tupdesc = SPI_tuptable->tupdesc;
		bool	isnull;
		strncpy(tmpgetvalue,DatumGetCString(SPI_getbinval(SPI_tuptable->vals[0],tupdesc,1,&isnull))+1, 10);
	}
	
	SPI_finish();
	PopActiveSnapshot();

	CommitTransactionCommand();
	pgstat_report_stat(false);
	pgstat_report_activity(STATE_IDLE, NULL);
	MemoryContextSwitchTo(old_context);
	MemoryContextReset(pgPdrLocalContext_once);
}


PG_FUNCTION_INFO_V1(worker_spi_launch);
/*
 * Dynamically launch an SPI worker.
 */
Datum
worker_spi_launch(PG_FUNCTION_ARGS)
{
	int32		i = PG_GETARG_INT32(0);
	BackgroundWorker worker;
	BackgroundWorkerHandle *handle;
	BgwHandleStatus status;
	pid_t		pid;

	memset(&worker, 0, sizeof(worker));
	worker.bgw_flags = BGWORKER_SHMEM_ACCESS |
		BGWORKER_BACKEND_DATABASE_CONNECTION;
	worker.bgw_start_time = BgWorkerStart_RecoveryFinished;
	worker.bgw_restart_time = BGW_NEVER_RESTART;
	sprintf(worker.bgw_library_name, "pg_pdr");
	sprintf(worker.bgw_function_name, "pg_pdr_main");
	snprintf(worker.bgw_name, BGW_MAXLEN, "worker %d", i);
	worker.bgw_main_arg = Int32GetDatum(i);

	/* set bgw_notify_pid so that we can use WaitForBackgroundWorkerStartup */
	worker.bgw_notify_pid = MyProcPid;

	if (!RegisterDynamicBackgroundWorker(&worker, &handle))
		PG_RETURN_NULL();

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

	PG_RETURN_INT32(pid);
}

PG_FUNCTION_INFO_V1(pg_pdr_new_snap);

Datum 
pg_pdr_new_snap(PG_FUNCTION_ARGS)
{
	pid_t	pdrpid = 0;
	pdr_new_snap_info *shmem_snapinfo = NULL;
	int	i = 0;
	int64	pdr_new_sid = -1;
	bool	newsnapid_shmem;
	if (!pg_pdr_naptime)
	{
		elog(ERROR, "Pdr is closed.You can't create new snapshot now.");
		return -1;
	}
	if (RecoveryInProgress())
	{
		elog(ERROR, "Cannot create new snapshot in a read-only transaction");
		return -1;
	}

	pdrpid = get_BackgroundWorkerData_pid_pdr();
	if (!pdrpid)
		elog(ERROR,"The backend performance diagnosis collector exited unexpected!");

	/* pdrpid may be not precise when you kill pdr backgroundworker after you open psql */
	/* Then pg_pdr_new_snap() will return false */
	/* Check pdr backgroundworker like following code is unnecessary ? */
	elog(LOG, " ");
	if (kill(pdrpid, 0) == -1)
	{
		elog(ERROR, "The backend performance diagnosis collector exited unexpected!");
		return -1;
	}

	/* get a precise result by shared memory */
	shmem_snapinfo = (pdr_new_snap_info *)
					ShmemInitStruct("pdr_new_snap_zt",
						sizeof(pdr_new_snap_info),
						&newsnapid_shmem);
	if (shmem_snapinfo->flag == 0)
	{
		shmem_snapinfo->flag = 10;
	}
	else 
	{
		elog(ERROR, "There are other pg_pdr_new_snap function running!");
		PG_RETURN_INT64(false);
	}
	kill(pdrpid, SIGHUP);

	for (i = 1; i < 100; i++)
	{
		/* test shmem pdr_new_snap_info */
		shmem_snapinfo = (pdr_new_snap_info *)
						ShmemInitStruct("pdr_new_snap_zt",
						sizeof(pdr_new_snap_info),
						&newsnapid_shmem);
		if (shmem_snapinfo->flag == 12)
		{
			shmem_snapinfo->flag = 0;
			pdr_new_sid = 31415926;
			break;
		}
		else if (shmem_snapinfo->flag == -1)
		{
			shmem_snapinfo->flag = 0;
			pdr_new_sid = -1;
			break;
		}
		pg_usleep(10000);
		
	}

	shmem_snapinfo->flag = 0;
	
	if (pdr_new_sid > 0)
		PG_RETURN_BOOL(true);
	else 
		PG_RETURN_BOOL(false);
}
