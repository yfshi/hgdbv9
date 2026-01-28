/* -------------------------------------------------------------------------
 *
 * pdr_report.c
 *
 * Using this tool to connect to the database
 *     get data form four tables
 *     generate pdr report as html format
 *
 *
 * Copyright:
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *		src/bin/scripts/pdr_report.c
 *
 * -------------------------------------------------------------------------
 */
#include <stdbool.h>  /* ReqID:SRS-MISC-PDR */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "libpq-fe.h"
#include "postgres.h"
#include "utils/timestamp.h"
#include "utils/int8.h"

#include "common.h"
#ifdef WIN32
#include <windows.h>
#include <conio.h>
#else
#include <linux/kernel.h>
#include <sys/sysinfo.h>
#include <pwd.h>
#endif

#include "access/xlog.h"


#define PGPDR_SQL_MAX 10
#define PGPDR_SEG_MAX 5
#define PGPDR_SNAP_SEC (dbbuf.naptime*60)
#define PGPDR_TAB_HASH_SIZE	512
#define PGPDR_WAIT_EVENT_SIZE	242
#define OIDS_LENGTH 5120

#define PAGELEN 24
#define LINELEN 512

int sum_size  = 0;
typedef int64 PgPdr_Counter;

/*
 * The actual stats counters kept within pgssEntry.
 */
typedef struct Counters
{
	Oid		userid;			/* user OID */
	Oid		dbid;			/* database OID */
	uint64		queryid;		/* query identifier */
	char*		query;			/* query string */
	uint64		calls;			/* # of times executed */
	double		total_time;		/* total execution time, in msec */
	double		min_time;		/* minimum execution time in msec */
	double		max_time;		/* maximum execution time in msec */
	double		mean_time;		/* mean execution time in msec */
	double		sum_var_time;		/* sum of variances in execution time in msec */
	uint64		rows;			/* total # of retrieved or affected rows */
	uint64		shared_blks_hit;	/* # of shared buffer hits */
	uint64		shared_blks_read;	/* # of shared disk blocks read */
	uint64		shared_blks_dirtied;	/* # of shared disk blocks dirtied */
	uint64		shared_blks_written;	/* # of shared disk blocks written */
	uint64		local_blks_hit;		/* # of local buffer hits */
	uint64		local_blks_read;	/* # of local disk blocks read */
	uint64		local_blks_dirtied; 	/* # of local disk blocks dirtied */
	uint64		local_blks_written; 	/* # of local disk blocks written */
	uint64		temp_blks_read; 	/* # of temp blocks read */
	uint64		temp_blks_written;	/* # of temp blocks written */
	double		blk_read_time;		/* time spent reading, in msec */
	double		blk_write_time; 	/* time spent writing, in msec */
	int 		query_slot;		/* the position in queryid_all */
	double		usage;			/* usage factor */
} Counters;

typedef struct  seg_counters
{
	Oid 	dbid;
	Oid 	tbid;
	uint64 	reads;
}seg_counters;

typedef struct io_counters
{
	uint64 	node_read_data;
	uint64	node_write_data;
	uint64	node_write_reqs;

	uint64 	db_read_data;
	uint64	db_write_data;
	uint64	db_write_reqs;

	uint64 	buf_read_data;
	uint64	buf_write_data;
	uint64	buf_write_reqs;

	uint64 	catalog_read_data;
	uint64	catalog_write_data;
	uint64	catalog_write_reqs;

	uint64 	rel_read_data;
	uint64	rel_write_data;
	uint64	rel_write_reqs;

	uint64 	temp_read_data;
	uint64	temp_write_data;
	uint64	temp_write_reqs;

	uint64	io_rel_read;
	uint64	io_rel_buf;
	uint64	io_rel_disk;

	uint64	io_index_read;
	uint64	io_index_buf;
	uint64 	io_index_disk;

	uint64 	io_seq_read;
	uint64 	io_seq_buf;
	uint64 	io_seq_disk;

	uint64 	io_view_read;
	uint64 	io_view_buf;
	uint64 	io_view_disk;

	uint64 	io_mv_read;
	uint64  io_mv_buf;
	uint64  io_mv_disk;

	uint64  io_comb_read;
	uint64  io_comb_buf;
	uint64  io_comb_disk;

	uint64  io_toast_read;
	uint64  io_toast_buf;
	uint64  io_toast_disk;

	uint64  io_foreign_read;
	uint64  io_foreign_buf;
	uint64  io_foreign_disk;

	uint64  io_total_read;
	uint64  io_total_buf;
	uint64  io_total_disk;
}io_counters;

typedef struct wait_counters
{
	char  name[64];
	uint64 count;
	char  type[64];
}wait_counters;

typedef struct snap_info
{
	int64 	snapid;
	char 	snaptime[32];
}snap_info;

snap_info allSanp[260000];

typedef struct pdrdb_grouping_info
{
	TimestampTz	dbstart_ts;
	int64 		snapcount;
	int64 		minsnapid;
	int64 		maxsnapid;
}pdrdb_grouping_info;

pdrdb_grouping_info dbGpInfo[102400];
wait_counters wait_stats_begin[PGPDR_WAIT_EVENT_SIZE];


/* ----------
 * PgPdr_StatDBEntry	The collector's data per database
 * ----------
 */
typedef struct PgPdr_StatDBEntry
{
	Oid		databaseid;
	char 		dbname[64];
	char 		dbtime[32];
	char 		dbver[64];
	char 		dbhost[256];
	char 		platform[32];
	int 		cpus;
	int 		cpucore;
	uint64  	dbmem;
	char 		begintime[32];
	char 		endtime[32];
	double 		run_tv;
	char 		str_run_tv[32];
	char		elapsed[32];
	int 		naptime;

	/* SQL Statistics */
	double 		db_totaltime;
	uint64 		db_totalgets;
	uint64 		db_totalreads;
	double		db_totaliotime;
	Counters 	sql_elapsed[PGPDR_SQL_MAX];
	Counters 	sql_cpu[PGPDR_SQL_MAX];
	Counters 	sql_io[PGPDR_SQL_MAX];
	Counters 	sql_gets[PGPDR_SQL_MAX];
	Counters 	sql_reads[PGPDR_SQL_MAX];
	Counters 	sql_exec[PGPDR_SQL_MAX];

	/* Segment Statistics */
	uint64 		db_totallogicalreads;
	uint64 		db_totalphysicalreads;
	uint64 		db_totaltablescans;

	seg_counters seg_logical_reads[PGPDR_SQL_MAX];
	seg_counters seg_physical_reads[PGPDR_SQL_MAX];
	seg_counters seg_table_scans[PGPDR_SQL_MAX];

	/* IO Stats */ 
	io_counters	io_stats;

	/* wait_event */
	uint64 		wait_total;
	uint64 		wait_activity;
	wait_counters 	wait_stats[PGPDR_WAIT_EVENT_SIZE];
	wait_counters 	wait_top10[10];

	PgPdr_Counter n_xact_commit;
	PgPdr_Counter n_xact_rollback;
	PgPdr_Counter n_blocks_fetched;
	PgPdr_Counter n_blocks_hit;
	PgPdr_Counter n_tuples_returned;
	PgPdr_Counter n_tuples_fetched;
	PgPdr_Counter n_tuples_inserted;
	PgPdr_Counter n_tuples_updated;
	PgPdr_Counter n_tuples_deleted;
	TimestampTz last_autovac_time;
	PgPdr_Counter n_conflict_tablespace;
	PgPdr_Counter n_conflict_lock;
	PgPdr_Counter n_conflict_snapshot;
	PgPdr_Counter n_conflict_bufferpin;
	PgPdr_Counter n_conflict_startup_deadlock;
	PgPdr_Counter n_temp_files;
	PgPdr_Counter n_temp_bytes;
	PgPdr_Counter n_deadlocks;
	PgPdr_Counter n_block_read_time;	/* times in microseconds */
	PgPdr_Counter n_block_write_time;

	TimestampTz stat_reset_timestamp;
	TimestampTz stats_timestamp;		/* time of db stats file update */
} PgPdr_StatDBEntry;

Oid 	relr[PGPDR_TAB_HASH_SIZE]={0};	/* relation */
Oid 	reli[PGPDR_TAB_HASH_SIZE]={0};	/* index */
Oid 	rels[PGPDR_TAB_HASH_SIZE]={0};	/* sequnce */
Oid 	relv[PGPDR_TAB_HASH_SIZE]={0};	/* view */
Oid 	relm[PGPDR_TAB_HASH_SIZE]={0};	/* matieral view */
Oid 	relc[PGPDR_TAB_HASH_SIZE]={0};	/* combine */
Oid 	relt[PGPDR_TAB_HASH_SIZE]={0};	/* toast */
Oid 	relf[PGPDR_TAB_HASH_SIZE]={0};	/* foreign */

char* 	wait_ev[PGPDR_WAIT_EVENT_SIZE]={
	"ShmemIndexLock",
	"OidGenLock",
	"XidGenLock",
	"ProcArrayLock",
	"SInvalReadLock",
	"SInvalWriteLock",
	"WALBufMappingLock",
	"WALWriteLock",
	"ControlFileLock",
	"XactSLRULock",
	"SubtransSLRULock",
	"MultiXactGenLock",
	"MultiXactOffsetSLRULock",
	"MultiXactMemberSLRULock",
	"RelCacheInitLock",
	"CheckpointerCommLock",
	"TwoPhaseStateLock",
	"TablespaceCreateLock",
	"BtreeVacuumLock",
	"AddinShmemInitLock",
	"AutovacuumLock",
	"AutovacuumScheduleLock",
	"SyncScanLock",
	"RelationMappingLock",
	"NotifySLRULock",
	"NotifyQueueLock",
	"SerializableXactHashLock",
	"SerializableFinishedListLock",
	"SerializablePredicateLockListLock",
	"SerialSLRULock",
	"SyncRepLock",
	"BackgroundWorkerLock",
	"DynamicSharedMemoryControlLock",
	"AutoFileLock",
	"ReplicationSlotAllocationLock",
	"ReplicationSlotControlLock",
	"CommitTsSLRULock",
	"CommitTsLock",
	"ReplicationOriginLock",
	"MultiXactTruncationLock",
	"OldSnapshotTimeMapLock",
	"LogicalRepWorkerLock",
	"XactTruncationLock",
	"WrapLimitsVacuumLock",
	"NotifyQueueTailLock",
	"FdwXactLock",
	"FdwXactResolverLock",
	"BadfileWriteLock",
	"ResGroupLock",
	"SessionStateLock",
	"XactBuffer",
	"CommitTsBuffer",
	"SubtransBuffer",
	"MultiXactOffsetBuffer",
	"MultiXactMemberBuffer",
	"NotifyBuffer",
	"SerialBuffer",
	"WALInsert",
	"BufferContent",
	"ReplicationOriginState",
	"ReplicationSlotIO",
	"LockFastPath",
	"BufferMapping",
	"LockManager",
	"PredicateLockManager",
	"ParallelHashJoin",
	"ParallelQueryDSA",
	"PerSessionDSA",
	"PerSessionRecordType",
	"PerSessionRecordTypmod",
	"SharedTupleStore",
	"SharedTidBitmap",
	"ParallelAppend",
	"PerXactPredicateList",
	"GlobalTempTableControl",
	"relation",
	"extend",
	"frozenid",
	"page",
	"tuple",
	"transactionid",
	"virtualxid",
	"spectoken",
	"object",
	"userlock",
	"advisory",
	"BufferPin",
	"ArchiverMain",
	"AutoVacuumMain",
	"BgWriterHibernate",
	"BgWriterMain",
	"CheckpointerMain",
	"LogicalApplyMain",
	"LogicalLauncherMain",
	"PgStatMain",
	"RecoveryWalStream",
	"SysLoggerMain",
	"WalReceiverMain",
	"WalSenderMain",
	"WalWriterMain",
	"ClientRead",
	"ClientWrite",
	"GSSOpenServer",
	"LibPQWalReceiverConnect",
	"LibPQWalReceiverReceive",
	"SSLOpenServer",
	"WalSenderWaitForWAL",
	"WalSenderWriteData",
	"Extension",
	"AppendReady",
	"BackendTermination",
	"BackupWaitWalArchive",
	"BgWorkerShutdown",
	"BgWorkerStartup",
	"BtreePage",
	"BufferIO",
	"CheckpointDone",
	"CheckpointStart",
	"ExecuteGather",
	"HashBatchAllocate",
	"HashBatchElect",
	"HashBatchLoad",
	"HashBuildAllocate",
	"HashBuildElect",
	"HashBuildHashInner",
	"HashBuildHashOuter",
	"HashGrowBatchesAllocate",
	"HashGrowBatchesDecide",
	"HashGrowBatchesElect",
	"HashGrowBatchesFinish",
	"HashGrowBatchesRepartition",
	"HashGrowBucketsAllocate",
	"HashGrowBucketsElect",
	"HashGrowBucketsReinsert",
	"LogicalSyncData",
	"LogicalSyncStateChange",
	"MessageQueueInternal",
	"MessageQueuePutMessage",
	"MessageQueueReceive",
	"MessageQueueSend",
	"ParallelBitmapScan",
	"ParallelCreateIndexScan",
	"ParallelFinish",
	"ProcArrayGroupUpdate",
	"ProcSignalBarrier",
	"Promote",
	"RecoveryConflictSnapshot",
	"RecoveryConflictTablespace",
	"RecoveryPause",
	"ReplicationOriginDrop",
	"ReplicationSlotDrop",
	"SafeSnapshot",
	"SyncRep",
	"WalReceiverExit",
	"WalReceiverWaitStart",
	"XactGroupUpdate",
	"BaseBackupThrottle",
	"CheckpointWriteDelay",
	"PgSleep",
	"RecoveryApplyDelay",
	"RecoveryRetrieveRetryInterval",
	"RegisterSyncRequest",
	"VacuumDelay",
	"BaseBackupRead",
	"BufFileRead",
	"BufFileWrite",
	"BufFileTruncate",
	"ControlFileRead",
	"ControlFileSync",
	"ControlFileSyncUpdate",
	"ControlFileWrite",
	"ControlFileWriteUpdate",
	"CopyFileRead",
	"CopyFileWrite",
	"DataFileExtend",
	"DataFileFlush",
	"DataFileImmediateSync",
	"DataFilePrefetch",
	"DataFileRead",
	"DataFileSync",
	"DataFileTruncate",
	"DataFileWrite",
	"DSMFillZeroWrite",
	"LockFileAddToDataDirRead",
	"LockFileAddToDataDirSync",
	"LockFileAddToDataDirWrite",
	"LockFileCreateRead",
	"LockFileCreateSync",
	"LockFileCreateWrite",
	"LockFileReCheckDataDirRead",
	"LogicalRewriteCheckpointSync",
	"LogicalRewriteMappingSync",
	"LogicalRewriteMappingWrite",
	"LogicalRewriteSync",
	"LogicalRewriteTruncate",
	"LogicalRewriteWrite",
	"RelationMapRead",
	"RelationMapSync",
	"RelationMapWrite",
	"ReorderBufferRead",
	"ReorderBufferWrite",
	"ReorderLogicalMappingRead",
	"ReplicationSlotRead",
	"ReplicationSlotRestoreSync",
	"ReplicationSlotSync",
	"ReplicationSlotWrite",
	"SLRUFlushSync",
	"SLRURead",
	"SLRUSync",
	"SLRUWrite",
	"SnapbuildRead",
	"SnapbuildSync",
	"SnapbuildWrite",
	"TimelineHistoryFileSync",
	"TimelineHistoryFileWrite",
	"TimelineHistoryRead",
	"TimelineHistorySync",
	"TimelineHistoryWrite",
	"TwophaseFileRead",
	"TwophaseFileSync",
	"TwophaseFileWrite",
	"WALSenderTimelineHistoryRead",
	"WALBootstrapSync",
	"WALBootstrapWrite",
	"WALCopyRead",
	"WALCopySync",
	"WALCopyWrite",
	"WALInitSync",
	"WALInitWrite",
	"WALRead",
	"WALSync",
	"WALSyncMethodAssign",
	"WALWrite",
	"LogicalChangesRead",
	"LogicalChangesWrite",
	"LogicalSubxactRead",
	"LogicalSubxactWrite",
	"FdwXactFileRead",
	"FdwXactFileSync",
	"FdwXactFileWrite",
	"FdwXactLauncherMain",
	"FdwXactResolverMain"
};

PgPdr_StatDBEntry dbbuf;
uint64	queryid_all[PGPDR_SQL_MAX*6]={0};
char 	*sqltext=NULL;
int64 	snap_begin,snap_end;

int find_replace_queryid(uint64 id);
int get_queryid_len(void);

void replace_string(char *buf, char* s1, char *s2);
void replace_sql_elapsed(char* buf);
void replace_sql_cpu(char* buf);
void replace_sql_io(char* buf);
void replace_sql_gets(char* buf);
void replace_sql_reads(char* buf);
void replace_sql_exec(char* buf);
void replace_sql_text(char* buf);
void replace_seg_logical_reads(char* buf);
void replace_seg_physical_reads(char* buf);
void replace_seg_table_scans(char* buf);

char* data2kmgtp(char* tmp,uint64 data);
char* datapersec(char* tmp,uint64 data);

void replace_io_stats(char* buf);
void read_pg_class(char* dbname);

char* oid2str(char* tmp,char kind);

void replace_summary(char* buf);
void replace_wait_top10(char* buf,int maxevent);
void replace_wait_stats(char* buf);
int find_wait_ev(char *name);
char* table_type(Oid id);

#ifdef WIN32
int see_more_win32(int per);
int control_line_win32(HANDLE myhandle,int mode);
#else
int see_more(FILE* cmd,int per);
#endif

int find_index_wait_ev(char *name);
int find_index_wait_ev_begin(char *name);
static void pdr_usage();

static void
exit_nicely(PGconn *conn)
{
	PQfinish(conn);
	exit(1);
}

int find_replace_queryid(uint64 id)
{
	int i;

	if (id == 0)
		return 0;

	for (i = 0; i < PGPDR_SQL_MAX*6; i++)
	{
		if (!queryid_all[i])
			break;
		if (queryid_all[i] == id)
			return i;	 
	}

	queryid_all[i] = id;

	return -1;
}

int get_queryid_len()
{
	int i;

	for (i = 0; i < PGPDR_SQL_MAX*6; i++)
	{
		if (!queryid_all[i])
			break;
	}

	return i;
}

/* string replace */
void replace_string(char *buf, char* s1, char *s2)
{
	char 	*p;
	int 	i;
	int 	len;

	len = strlen(s2);
	if (strlen(s1) == len)
	{
		/* if s1 is as same as s2,return */
		if (!strcmp(s1,s2))
		{
			return;
		}

		while ((p = strstr(buf,s1)) != NULL)
		{
			for (i = 0; i < len; i++)
			{
				buf[p-buf+i] = s2[i];
			}
		}
	}
	else if (strlen(s1) > strlen(s2))
	{
		while ((p = strstr(buf,s1)) != NULL)
		{
			memmove(p,s2,len);
			memmove(p+len,p+strlen(s1),strlen(buf)-(p-buf)-strlen(s1));
		}
	}
	else if (strlen(s1) < strlen(s2))
	{
		if (!strncmp(s1,s2,strlen(s1)))
		{
			return;
		}
		while ((p = strstr(buf,s1)) != NULL)
		{
			//we assume that buf has enough space
			memmove(p+len,p+strlen(s1),strlen(buf)-(p-buf)-strlen(s1));
			memmove(p,s2,len);
		}
	}   
}

void replace_sql_elapsed(char* buf)
{
	int  	i;
	char 	tmp[256];
	char  	name[64];
	char 	*tmp2=NULL;

	for (i = 0; i < PGPDR_SQL_MAX; i++)
	{		
		//elapsed time
		sprintf(name,"$sql_total_time_%d$",i+1);
		sprintf(tmp,"%f",dbbuf.sql_elapsed[i].total_time/1000);
		replace_string(buf,name,tmp);

		//executions
		sprintf(name,"$sql_calls_%d$",i+1);
		sprintf(tmp,"%lu",dbbuf.sql_elapsed[i].calls);
		replace_string(buf,name,tmp);

		//elapsed time per exec
		sprintf(name,"$sql_mean_time_%d$",i+1);
		sprintf(tmp,"%f",dbbuf.sql_elapsed[i].calls>0?dbbuf.sql_elapsed[i].total_time/dbbuf.sql_elapsed[i].calls/1000:0.00);
		replace_string(buf,name,tmp);

		//%total
		sprintf(name,"$sql_total_percent_%d$",i+1);
		sprintf(tmp,"%f",dbbuf.sql_elapsed[i].total_time/(fabs(dbbuf.db_totaltime)<1e-15?1:dbbuf.db_totaltime)*100);
		replace_string(buf,name,tmp);

		//%CPU
		sprintf(name,"$sql_cpu_percent_%d$",i+1);
		sprintf(tmp,"%f",(1-(dbbuf.sql_elapsed[i].blk_read_time+dbbuf.sql_elapsed[i].blk_write_time)/(fabs(dbbuf.db_totaltime)<1e-15?1:dbbuf.db_totaltime))*100);
		replace_string(buf,name,tmp);

		//%IO
		sprintf(name,"$sql_io_percent_%d$",i+1);
		sprintf(tmp,"%f",(dbbuf.sql_elapsed[i].blk_read_time+dbbuf.sql_elapsed[i].blk_write_time)/(fabs(dbbuf.db_totaltime)<1e-15?1:dbbuf.db_totaltime)*100);
		replace_string(buf,name,tmp);

		//sql id
		sprintf(name,"$sql_queryid_%d$",i+1);
		sprintf(tmp,"%lu",dbbuf.sql_elapsed[i].queryid);	
		replace_string(buf,name,tmp);

		//sql text
		sprintf(name,"$sql_time_sql_text_%d$",i+1);
		if (dbbuf.sql_elapsed[i].query)
		{	
			if (strlen(dbbuf.sql_elapsed[i].query) < 30)
			{
				replace_string(buf,name,dbbuf.sql_elapsed[i].query);
			}
			else
			{
				memset(tmp,0,sizeof(tmp));
				strncpy(tmp,dbbuf.sql_elapsed[i].query,30);
				strcat(tmp, "...");
				replace_string(buf,name,tmp);
			}
		}
		else
		{
			replace_string(buf,name," ");
		}

		//list of sql text
		if (find_replace_queryid(dbbuf.sql_elapsed[i].queryid)<0)
		{
			size_t tmp2_len = 0; /* ReqID:SRS-MISC-PDR */
			tmp2 = (char*)malloc(strlen(dbbuf.sql_elapsed[i].query)+300);

			/* Begin - ReqID:SRS-MISC-PDR */
			/*
			 * need check the return value for malloc(), realloc()
			 */
			if (tmp2 == NULL)
			{
				fprintf(stderr, _("Allocate memory error\n"));
				exit(1);
			}
			/* End - ReqID:SRS-MISC-PDR */

			if (get_queryid_len()%2)
				sprintf(tmp2,"<tr><td scope=\"row\" class='awrnc'><a class=\"awr\" name=\"%lu\"></a>%lu</td><td class='awrnc'>%s</td></tr>\n",
					dbbuf.sql_elapsed[i].queryid,dbbuf.sql_elapsed[i].queryid,dbbuf.sql_elapsed[i].query);
			else
				sprintf(tmp2,"<tr><td scope=\"row\" class='awrc'><a class=\"awr\" name=\"%lu\"></a>%lu</td><td class='awrc'>%s</td></tr>\n",
					dbbuf.sql_elapsed[i].queryid,dbbuf.sql_elapsed[i].queryid,dbbuf.sql_elapsed[i].query);

			if (sqltext)
			{
				sqltext = (char*)realloc(sqltext,strlen(sqltext)+strlen(tmp2)+1);
				tmp2_len = strlen(tmp2); /* ReqID:SRS-MISC-PDR */
			}
			else
			{
				sqltext = (char*)calloc(strlen(tmp2)+1,1);
				tmp2_len = strlen(tmp2); /* ReqID:SRS-MISC-PDR */
			}

			/* Begin - ReqID:SRS-MISC-PDR */
			if (sqltext == NULL)
			{
				fprintf(stderr, _("Allocate memory error\n"));
				exit(1);
			}
			/* End - ReqID:SRS-MISC-PDR */
			
			strncat(sqltext,tmp2,tmp2_len); /* ReqID:SRS-MISC-PDR */

			free(tmp2);

		}
	}
}

void replace_sql_cpu(char* buf)
{ 
	int	i;
	char 	tmp[256];
	char  	name[64];
	char 	*tmp2=NULL;

	for (i = 0; i < PGPDR_SQL_MAX; i++)
	{
		//cpu time
		sprintf(name,"$sql_cpu_time_%d$",i+1);
		sprintf(tmp,"%f",(dbbuf.sql_cpu[i].total_time-dbbuf.sql_cpu[i].blk_read_time-dbbuf.sql_cpu[i].blk_write_time)/1000);
		replace_string(buf,name,tmp);

		//executions
		sprintf(name,"$sql_cpu_calls_%d$",i+1);
		sprintf(tmp,"%lu",dbbuf.sql_cpu[i].calls);
		replace_string(buf,name,tmp);

		//cpu per exec
		sprintf(name,"$sql_cpu_mean_time_%d$",i+1);
		sprintf(tmp,"%f",dbbuf.sql_cpu[i].calls>0?(dbbuf.sql_cpu[i].total_time-dbbuf.sql_cpu[i].blk_read_time-dbbuf.sql_cpu[i].blk_write_time)/dbbuf.sql_cpu[i].calls/1000:0.00);
		replace_string(buf,name,tmp);

		//%total
		sprintf(name,"$sql_cpu_total_percent_%d$",i+1);
		sprintf(tmp,"%f",(dbbuf.sql_cpu[i].total_time-dbbuf.sql_cpu[i].blk_read_time-dbbuf.sql_cpu[i].blk_write_time)/(fabs(dbbuf.db_totaltime)<1e-15?1:dbbuf.db_totaltime)*100);
		replace_string(buf,name,tmp);

		//elapsed time
		sprintf(name,"$sql_cpu_total_time_%d$",i+1);
		sprintf(tmp,"%f",dbbuf.sql_cpu[i].total_time/1000);
		replace_string(buf,name,tmp);

		//%CPU
		sprintf(name,"$sql_cpu_cpu_percent_%d$",i+1);
		sprintf(tmp,"%f",(1-(dbbuf.sql_cpu[i].blk_read_time+dbbuf.sql_cpu[i].blk_write_time)/(fabs(dbbuf.db_totaltime)<1e-15?1:dbbuf.db_totaltime))*100);
		replace_string(buf,name,tmp);

		//%IO
		sprintf(name,"$sql_cpu_io_percent_%d$",i+1);
		sprintf(tmp,"%f",(dbbuf.sql_cpu[i].blk_read_time+dbbuf.sql_cpu[i].blk_write_time)/(fabs(dbbuf.db_totaltime)<1e-15?1:dbbuf.db_totaltime)*100);
		replace_string(buf,name,tmp);
		
		//sql id
		sprintf(name,"$sql_cpu_queryid_%d$",i+1);
		sprintf(tmp,"%lu",dbbuf.sql_cpu[i].queryid);	
		replace_string(buf,name,tmp);

		//sql text
		sprintf(name,"$sql_cpu_sql_text_%d$",i+1);	
		if (dbbuf.sql_cpu[i].query)
		{	
			if (strlen(dbbuf.sql_cpu[i].query) < 30)
			{
				replace_string(buf,name,dbbuf.sql_cpu[i].query);
			}
			else
			{
				memset(tmp,0,sizeof(tmp));
				strncpy(tmp,dbbuf.sql_cpu[i].query,30);
				strcat(tmp, "...");
				replace_string(buf,name,tmp);
			}
		}
		else
		{
			replace_string(buf,name," ");
		}

		//list of sql text
		if (find_replace_queryid(dbbuf.sql_cpu[i].queryid) < 0)
		{
			size_t tmp2_len = 0; /* ReqID:SRS-MISC-PDR */
			tmp2 = (char*)malloc(strlen(dbbuf.sql_cpu[i].query)+300);

			/* Begin - ReqID:SRS-MISC-PDR */
			/*
			 * need check the return value for malloc(), realloc()
			 */
			if (tmp2 == NULL)
			{
				fprintf(stderr, _("Allocate memory error\n"));
				exit(1);
			}
			/* End - ReqID:SRS-MISC-PDR */

			if (get_queryid_len()%2)
			{
				sprintf(tmp2,"<tr><td scope=\"row\" class='awrnc'><a class=\"awr\" name=\"%lu\"></a>%lu</td><td class='awrnc'>%s</td></tr>\n",
					dbbuf.sql_cpu[i].queryid,dbbuf.sql_cpu[i].queryid,dbbuf.sql_cpu[i].query);
			}
			else
			{
				sprintf(tmp2,"<tr><td scope=\"row\" class='awrc'><a class=\"awr\" name=\"%lu\"></a>%lu</td><td class='awrc'>%s</td></tr>\n",
					dbbuf.sql_cpu[i].queryid,dbbuf.sql_cpu[i].queryid,dbbuf.sql_cpu[i].query);
			}

			if (sqltext)
			{
				sqltext = (char*)realloc(sqltext,strlen(sqltext)+strlen(tmp2)+1);
				tmp2_len = strlen(tmp2); /* ReqID:SRS-MISC-PDR */
			}
			else
			{
				sqltext = (char*)calloc(strlen(tmp2)+1,1);
				tmp2_len = strlen(tmp2); /* ReqID:SRS-MISC-PDR */
			}
			/* Begin - ReqID:SRS-MISC-PDR */
			if (sqltext == NULL)
			{
				fprintf(stderr, _("Allocate memory error\n"));
				exit(1);
			}
			/* End - ReqID:SRS-MISC-PDR */

			strncat(sqltext,tmp2,tmp2_len); /* ReqID:SRS-MISC-PDR */

			free(tmp2);

		}
	}
}

void replace_sql_io(char* buf)
{
	int	i;
	char 	tmp[256];
	char  	name[64];
	char 	*tmp2=NULL;

	for (i = 0; i < PGPDR_SQL_MAX; i++)
	{
		//io time
		sprintf(name,"$sql_io_time_%d$",i+1);
		sprintf(tmp,"%f",(dbbuf.sql_io[i].blk_read_time+dbbuf.sql_io[i].blk_write_time)/1000);
		replace_string(buf,name,tmp);

		//executions
		sprintf(name,"$sql_io_calls_%d$",i+1);
		sprintf(tmp,"%lu",dbbuf.sql_io[i].calls);
		replace_string(buf,name,tmp);

		//io per exec
		sprintf(name,"$sql_io_mean_time_%d$",i+1);
		sprintf(tmp,"%f",dbbuf.sql_io[i].calls>0?(dbbuf.sql_io[i].blk_read_time+dbbuf.sql_io[i].blk_write_time)/dbbuf.sql_io[i].calls/1000:0.00);
		replace_string(buf,name,tmp);

		//%total
		sprintf(name,"$sql_io_total_percent_%d$",i+1);
		sprintf(tmp,"%f",(dbbuf.sql_io[i].blk_read_time+dbbuf.sql_io[i].blk_write_time)/(fabs(dbbuf.db_totaltime)<1e-15?1:dbbuf.db_totaltime)*100);
		replace_string(buf,name,tmp);

		//elapsed time
		sprintf(name,"$sql_io_total_time_%d$",i+1);
		sprintf(tmp,"%f",dbbuf.sql_io[i].total_time/1000);
		replace_string(buf,name,tmp);

		//%CPU
		sprintf(name,"$sql_io_cpu_percent_%d$",i+1);
		sprintf(tmp,"%f",(1-(dbbuf.sql_io[i].blk_read_time+dbbuf.sql_io[i].blk_write_time)/(fabs(dbbuf.db_totaltime)<1e-15?1:dbbuf.db_totaltime))*100);
		replace_string(buf,name,tmp);

		//%IO
		sprintf(name,"$sql_io_io_percent_%d$",i+1);
		sprintf(tmp,"%f",(dbbuf.sql_io[i].blk_read_time+dbbuf.sql_io[i].blk_write_time)/(fabs(dbbuf.db_totaltime)<1e-15?1:dbbuf.db_totaltime)*100);
		replace_string(buf,name,tmp);
		
		//sql id
		sprintf(name,"$sql_io_queryid_%d$",i+1);
		sprintf(tmp,"%lu",dbbuf.sql_io[i].queryid);	
		replace_string(buf,name,tmp);

		//sql text
		sprintf(name,"$sql_io_sql_text_%d$",i+1);	
		if (dbbuf.sql_io[i].query)
		{	
			if (strlen(dbbuf.sql_io[i].query) < 30)
			{
				replace_string(buf,name,dbbuf.sql_io[i].query);
			}
			else
			{
				memset(tmp,0,sizeof(tmp));
				strncpy(tmp,dbbuf.sql_io[i].query,30);
				strcat(tmp, "...");
				replace_string(buf,name,tmp);
			}
		}
		else
		{
			replace_string(buf,name," ");
		}

		//list of sql text
		if (find_replace_queryid(dbbuf.sql_io[i].queryid)<0)
		{
			size_t tmp2_len = 0; /* ReqID:SRS-MISC-PDR */
			tmp2 = (char*)malloc(strlen(dbbuf.sql_io[i].query)+300);

			/* Begin - ReqID:SRS-MISC-PDR */
			/*
			 * need check the return value for malloc(), realloc()
			 */
			if (tmp2 == NULL)
			{
				fprintf(stderr, _("Allocate memory error\n"));
				exit(1);
			}
			/* End - ReqID:SRS-MISC-PDR */

			if (get_queryid_len()%2)
				sprintf(tmp2,"<tr><td scope=\"row\" class='awrnc'><a class=\"awr\" name=\"%lu\"></a>%lu</td><td class='awrnc'>%s</td></tr>\n",
					dbbuf.sql_io[i].queryid,dbbuf.sql_io[i].queryid,dbbuf.sql_io[i].query);
			else
				sprintf(tmp2,"<tr><td scope=\"row\" class='awrc'><a class=\"awr\" name=\"%lu\"></a>%lu</td><td class='awrc'>%s</td></tr>\n",
					dbbuf.sql_io[i].queryid,dbbuf.sql_io[i].queryid,dbbuf.sql_io[i].query);

			if (sqltext)
			{
				sqltext = (char*)realloc(sqltext,strlen(sqltext)+strlen(tmp2)+1);
				tmp2_len = strlen(tmp2); /* ReqID:SRS-MISC-PDR */
			}
			else
			{
				sqltext = (char*)calloc(strlen(tmp2)+1,1);
				tmp2_len = strlen(tmp2); /* ReqID:SRS-MISC-PDR */
			}

			/* Begin - ReqID:SRS-MISC-PDR */
			if (sqltext == NULL)
			{
				fprintf(stderr, _("Allocate memory error\n"));
				exit(1);
			}
			/* End - ReqID:SRS-MISC-PDR */

			strncat(sqltext,tmp2,tmp2_len); /* ReqID:SRS-MISC-PDR */

			free(tmp2);

		}
	}
}

void replace_sql_gets(char* buf)
{
	int	i;
	char 	tmp[256];
	char  	name[64];
	char 	*tmp2=NULL;

	for (i = 0; i < PGPDR_SQL_MAX; i++)
	{
		//buffer gets
		sprintf(name,"$sql_buffer_gets_%d$",i+1);
		sprintf(tmp,"%lu",dbbuf.sql_gets[i].shared_blks_hit+dbbuf.sql_gets[i].local_blks_hit);
		replace_string(buf,name,tmp);

		//executions
		sprintf(name,"$sql_gets_calls_%d$",i+1);
		sprintf(tmp,"%lu",dbbuf.sql_gets[i].calls);
		replace_string(buf,name,tmp);

		//gets per exec
		sprintf(name,"$sql_gets_mean_%d$",i+1);
		sprintf(tmp,"%f",(float8)(dbbuf.sql_gets[i].shared_blks_hit+dbbuf.sql_gets[i].local_blks_hit)/(dbbuf.sql_gets[i].calls==0?1:dbbuf.sql_gets[i].calls));
		replace_string(buf,name,tmp);

		//%total
		sprintf(name,"$sql_gets_total_percent_%d$",i+1);
		sprintf(tmp,"%f",(float8)(dbbuf.sql_gets[i].shared_blks_hit+dbbuf.sql_gets[i].local_blks_hit)/(dbbuf.db_totalgets==0?1:dbbuf.db_totalgets)*100);
		replace_string(buf,name,tmp);

		//elapsed time
		sprintf(name,"$sql_gets_total_time_%d$",i+1);
		sprintf(tmp,"%f",dbbuf.sql_gets[i].total_time/1000);
		replace_string(buf,name,tmp);

		//%CPU
		sprintf(name,"$sql_gets_cpu_percent_%d$",i+1);
		sprintf(tmp,"%f",(1-(dbbuf.sql_gets[i].blk_read_time+dbbuf.sql_gets[i].blk_write_time)/(fabs(dbbuf.db_totaltime)<1e-15?1:dbbuf.db_totaltime))*100);
		replace_string(buf,name,tmp);

		//%IO
		sprintf(name,"$sql_gets_io_percent_%d$",i+1);
		sprintf(tmp,"%f",(dbbuf.sql_gets[i].blk_read_time+dbbuf.sql_gets[i].blk_write_time)/(fabs(dbbuf.db_totaltime)<1e-15?1:dbbuf.db_totaltime)*100);
		replace_string(buf,name,tmp);

		//sql id
		sprintf(name,"$sql_gets_queryid_%d$",i+1);
		sprintf(tmp,"%lu",dbbuf.sql_gets[i].queryid);	
		replace_string(buf,name,tmp);

		//sql text
		sprintf(name,"$sql_buffer_sql_text_%d$",i+1);	
		if (dbbuf.sql_gets[i].query)
		{	
			if (strlen(dbbuf.sql_gets[i].query) < 30)
			{
				replace_string(buf,name,dbbuf.sql_gets[i].query);
			}
			else
			{
				memset(tmp,0,sizeof(tmp));
				strncpy(tmp,dbbuf.sql_gets[i].query,30);
				strcat(tmp, "...");
				replace_string(buf,name,tmp);
			}
		}
		else
		{
			replace_string(buf,name," ");
		}

		//list of sql text
		if (find_replace_queryid(dbbuf.sql_gets[i].queryid) < 0)
		{
			size_t tmp2_len = 0; /* ReqID:SRS-MISC-PDR */
			tmp2 = (char*)malloc(strlen(dbbuf.sql_gets[i].query)+300);

			/* Begin - ReqID:SRS-MISC-PDR */
			/*
			 * need check the return value for malloc(), realloc()
			 */
			if (tmp2 == NULL)
			{
				fprintf(stderr, _("Allocate memory error\n"));
				exit(1);
			}
			/* End - ReqID:SRS-MISC-PDR */

			if (get_queryid_len()%2)
			{
				sprintf(tmp2,"<tr><td scope=\"row\" class='awrnc'><a class=\"awr\" name=\"%lu\"></a>%lu</td><td class='awrnc'>%s</td></tr>\n",
					dbbuf.sql_gets[i].queryid,dbbuf.sql_gets[i].queryid,dbbuf.sql_gets[i].query);
			}
			else
			{
				sprintf(tmp2,"<tr><td scope=\"row\" class='awrc'><a class=\"awr\" name=\"%lu\"></a>%lu</td><td class='awrc'>%s</td></tr>\n",
					dbbuf.sql_gets[i].queryid,dbbuf.sql_gets[i].queryid,dbbuf.sql_gets[i].query);
			}

			if (sqltext)
			{
				sqltext = (char*)realloc(sqltext,strlen(sqltext)+strlen(tmp2)+1);
				tmp2_len = strlen(tmp2); /* ReqID:SRS-MISC-PDR */
			}
			else
			{
				sqltext = (char*)calloc(strlen(tmp2)+1,1);
				tmp2_len = strlen(tmp2); /* ReqID:SRS-MISC-PDR */
			}

			/* Begin - ReqID:SRS-MISC-PDR */
			if (sqltext == NULL)
			{
				fprintf(stderr, _("Allocate memory error\n"));
				exit(1);
			}
			/* End - ReqID:SRS-MISC-PDR */

			strncat(sqltext,tmp2,tmp2_len);	/* ReqID:SRS-MISC-PDR */

			free(tmp2);
		}
	}
}

void replace_sql_reads(char* buf)
{
	int	i;
	char 	tmp[256];
	char  	name[64];
	char 	*tmp2=NULL;

	for ( i =0; i < PGPDR_SQL_MAX; i++)
	{
		//disk reads
		sprintf(name,"$sql_disk_reads_%d$",i+1);
		sprintf(tmp,"%lu",dbbuf.sql_reads[i].shared_blks_read+dbbuf.sql_reads[i].local_blks_read + dbbuf.sql_reads[i].temp_blks_read);
		replace_string(buf,name,tmp);

		//executions
		sprintf(name,"$sql_reads_calls_%d$",i+1);
		sprintf(tmp,"%lu",dbbuf.sql_reads[i].calls);
		replace_string(buf,name,tmp);

		//reads per exec
		sprintf(name,"$sql_reads_mean_%d$",i+1);
		sprintf(tmp,"%f",(float8)(dbbuf.sql_reads[i].shared_blks_read+dbbuf.sql_reads[i].local_blks_read + dbbuf.sql_reads[i].temp_blks_read)/(dbbuf.sql_reads[i].calls==0?1:dbbuf.sql_reads[i].calls));
		replace_string(buf,name,tmp);

		//%total
		sprintf(name,"$sql_reads_total_percent_%d$",i+1);
		sprintf(tmp,"%f",(float8)(dbbuf.sql_reads[i].shared_blks_read+dbbuf.sql_reads[i].local_blks_read + dbbuf.sql_reads[i].temp_blks_read)/(dbbuf.db_totalreads==0?1:dbbuf.db_totalreads)*100);
		replace_string(buf,name,tmp);

		//elapsed time
		sprintf(name,"$sql_reads_total_time_%d$",i+1);
		sprintf(tmp,"%f",dbbuf.sql_reads[i].total_time/1000);
		replace_string(buf,name,tmp);

		//%CPU
		sprintf(name,"$sql_reads_cpu_percent_%d$",i+1);
		sprintf(tmp,"%f",(1-(dbbuf.sql_reads[i].blk_read_time+dbbuf.sql_reads[i].blk_write_time)/(fabs(dbbuf.db_totaltime)<1e-15?1:dbbuf.db_totaltime))*100);
		replace_string(buf,name,tmp);

		//%IO
		sprintf(name,"$sql_reads_io_percent_%d$",i+1);
		sprintf(tmp,"%f",(dbbuf.sql_reads[i].blk_read_time+dbbuf.sql_reads[i].blk_write_time)/(fabs(dbbuf.db_totaltime)<1e-15?1:dbbuf.db_totaltime)*100);
		replace_string(buf,name,tmp);

		//sql id
		sprintf(name,"$sql_reads_queryid_%d$",i+1);
		sprintf(tmp,"%lu",dbbuf.sql_reads[i].queryid);	
		replace_string(buf,name,tmp);

		//sql text
		sprintf(name,"$sql_disk_sql_text_%d$",i+1);	
		if (dbbuf.sql_reads[i].query)
		{	
			if (strlen(dbbuf.sql_reads[i].query) < 30)
			{
				replace_string(buf,name,dbbuf.sql_reads[i].query);
			}
			else
			{
				memset(tmp,0,sizeof(tmp));
				strncpy(tmp,dbbuf.sql_reads[i].query,30);
				strcat(tmp, "...");
				replace_string(buf,name,tmp);
			}
		}
		else
		{
			replace_string(buf,name," ");
		}

		//list of sql text
		if (find_replace_queryid(dbbuf.sql_reads[i].queryid)<0)
		{
			size_t tmp2_len = 0; /* ReqID:SRS-MISC-PDR */
			tmp2 = (char*)malloc(strlen(dbbuf.sql_reads[i].query)+300);

			/* Begin - ReqID:SRS-MISC-PDR */
			/*
			 * need check the return value for malloc(), realloc()
			 */
			if (tmp2 == NULL)
			{
				fprintf(stderr, _("Allocate memory error\n"));
				exit(1);
			}
			/* End - ReqID:SRS-MISC-PDR */

			if (get_queryid_len()%2)
			{
				sprintf(tmp2,"<tr><td scope=\"row\" class='awrnc'><a class=\"awr\" name=\"%lu\"></a>%lu</td><td class='awrnc'>%s</td></tr>\n",
					dbbuf.sql_reads[i].queryid,dbbuf.sql_reads[i].queryid,dbbuf.sql_reads[i].query);
			}
			else
			{
				sprintf(tmp2,"<tr><td scope=\"row\" class='awrc'><a class=\"awr\" name=\"%lu\"></a>%lu</td><td class='awrc'>%s</td></tr>\n",
					dbbuf.sql_reads[i].queryid,dbbuf.sql_reads[i].queryid,dbbuf.sql_reads[i].query);
			}

			if (sqltext)
			{
				sqltext = (char*)realloc(sqltext,strlen(sqltext)+strlen(tmp2)+1);
				tmp2_len = strlen(tmp2); /* ReqID:SRS-MISC-PDR */
			}
			else
			{
				sqltext = (char*)calloc(strlen(tmp2)+1,1);
				tmp2_len = strlen(tmp2); /* ReqID:SRS-MISC-PDR */
			}
			/* Begin - ReqID:SRS-MISC-PDR */
			if (sqltext == NULL)
			{
				fprintf(stderr, _("Allocate memory error\n"));
				exit(1);
			}
			/* End - ReqID:SRS-MISC-PDR */

			strncat(sqltext,tmp2,tmp2_len); /* ReqID:SRS-MISC-PDR */

			free(tmp2);

		}
	}
}

void replace_sql_exec(char* buf)
{
	int	i;
	char 	tmp[256];
	char  	name[64];
	char 	*tmp2=NULL;

	for (i = 0; i < PGPDR_SQL_MAX; i++)
	{
		//executions
		sprintf(name,"$sql_executions_%d$",i+1);
		sprintf(tmp,"%lu",dbbuf.sql_exec[i].calls);
		replace_string(buf,name,tmp);

		//rows processed
		sprintf(name,"$sql_exec_rows_%d$",i+1);
		sprintf(tmp,"%lu",dbbuf.sql_exec[i].rows);
		replace_string(buf,name,tmp);

		//rows per exec
		sprintf(name,"$sql_exec_mean_row_%d$",i+1);
		sprintf(tmp,"%f",(float)dbbuf.sql_exec[i].rows/(dbbuf.sql_exec[i].calls==0?1:dbbuf.sql_exec[i].calls));
		replace_string(buf,name,tmp);

		//elapsed time
		sprintf(name,"$sql_exec_total_time_%d$",i+1);
		sprintf(tmp,"%f",dbbuf.sql_exec[i].total_time/1000);
		replace_string(buf,name,tmp);

		//%CPU
		sprintf(name,"$sql_exec_cpu_percent_%d$",i+1);
		sprintf(tmp,"%f",(1-(dbbuf.sql_exec[i].blk_read_time+dbbuf.sql_exec[i].blk_write_time)/(fabs(dbbuf.db_totaltime)<1e-15?1:dbbuf.db_totaltime))*100);
		replace_string(buf,name,tmp);

		//%IO
		sprintf(name,"$sql_exec_io_percent_%d$",i+1);
		sprintf(tmp,"%f",(dbbuf.sql_exec[i].blk_read_time+dbbuf.sql_exec[i].blk_write_time)/(fabs(dbbuf.db_totaltime)<1e-15?1:dbbuf.db_totaltime)*100);
		replace_string(buf,name,tmp);

		//sql id
		sprintf(name,"$sql_exec_queryid_%d$",i+1);
		sprintf(tmp,"%lu",dbbuf.sql_exec[i].queryid);	
		replace_string(buf,name,tmp);

		//sql text
		sprintf(name,"$sql_exec_sql_text_%d$",i+1);	
		if (dbbuf.sql_exec[i].query)
		{	
			if (strlen(dbbuf.sql_exec[i].query) < 30)
			{
				replace_string(buf,name,dbbuf.sql_exec[i].query);
			}
			else
			{
				memset(tmp,0,sizeof(tmp));
				strncpy(tmp,dbbuf.sql_exec[i].query,30);
				strcat(tmp, "...");
				replace_string(buf,name,tmp);
			}
		}
		else
		{
			replace_string(buf,name," ");
		}

		//list of sql text
		if (find_replace_queryid(dbbuf.sql_exec[i].queryid)<0)
		{
			size_t tmp2_len = 0; /* ReqID:SRS-MISC-PDR */
			tmp2 = (char*)malloc(strlen(dbbuf.sql_exec[i].query)+300);

			/* Begin - ReqID:SRS-MISC-PDR */
			/*
			 * need check the return value for malloc(), realloc()
			 */
			if (tmp2 == NULL)
			{
				fprintf(stderr, _("Allocate memory error\n"));
				exit(1);
			}
			/* End - ReqID:SRS-MISC-PDR */

			if (get_queryid_len()%2)
			{
				sprintf(tmp2,"<tr><td scope=\"row\" class='awrnc'><a class=\"awr\" name=\"%lu\"></a>%lu</td><td class='awrnc'>%s</td></tr>\n",
					dbbuf.sql_exec[i].queryid,dbbuf.sql_exec[i].queryid,dbbuf.sql_exec[i].query);
			}
			else
			{
				sprintf(tmp2,"<tr><td scope=\"row\" class='awrc'><a class=\"awr\" name=\"%lu\"></a>%lu</td><td class='awrc'>%s</td></tr>\n",
					dbbuf.sql_exec[i].queryid,dbbuf.sql_exec[i].queryid,dbbuf.sql_exec[i].query);
			}

			if (sqltext)
			{
				sqltext = (char*)realloc(sqltext,strlen(sqltext)+strlen(tmp2)+1);
				tmp2_len = strlen(tmp2); /* ReqID:SRS-MISC-PDR */
			}
			else
			{
				sqltext = (char*)calloc(strlen(tmp2)+1,1);
				tmp2_len = strlen(tmp2); /* ReqID:SRS-MISC-PDR */
			}
			/* Begin - ReqID:SRS-MISC-PDR */
			if (sqltext == NULL)
			{
				fprintf(stderr, _("Allocate memory error\n"));
				exit(1);
			}
			/* End - ReqID:SRS-MISC-PDR */

			strncat(sqltext,tmp2,tmp2_len);  /* ReqID:SRS-MISC-PDR */

			free(tmp2);

		}
	}
}

void replace_sql_text(char* buf)
{
	if (sqltext)
		replace_string(buf,"$sql_all_sql_text_list$",sqltext);
	else
		replace_string(buf,"$sql_all_sql_text_list$","No sql data.");

	if (sqltext)	
		free(sqltext);	
}

char* table_type(Oid id)
{
	int i;

	for (i = 0; i < PGPDR_TAB_HASH_SIZE; i++)
	{
		if (id == relr[i])
		{
			return "TABLE";
		}

		if (id == reli[i])
		{
			return "INDEX";
		}

		if (id == rels[i])
		{
			return "SEGMENT";
		}

		if (id == relv[i])
		{
			return "VIEW";
		}

		if (id == relm[i])
		{
			return "MV";
		}

		if (id == relc[i])
		{
			return "COMB";
		}

		if (id == relt[i])
		{
			return "TOAST";
		}

		if (id == relf[i])
		{
			return "FOREIGN";
		}
	}

	return "";
}

void replace_seg_logical_reads(char* buf)
{
	int i;
	char 		tmp[256];
	char  		name[64];

	for (i = 0; i < PGPDR_SQL_MAX; i++)
	{
		//dbid
		sprintf(name,"$seg_db_oid_%d$",i+1);
		sprintf(tmp,"%u",dbbuf.seg_logical_reads[i].dbid);
		replace_string(buf,name,tmp);

		//dbname
		sprintf(name,"$seg_db_name_%d$",i+1);
		sprintf(tmp,"%s",dbbuf.dbname);
		replace_string(buf,name,tmp);

		//tbid
		sprintf(name,"$seg_table_oid_%d$",i+1);
		sprintf(tmp,"%u",dbbuf.seg_logical_reads[i].tbid);
		replace_string(buf,name,tmp);

		//tbtype
		sprintf(name,"$seg_logical_table_type_%d$",i+1);
		sprintf(tmp,"%s",table_type(dbbuf.seg_logical_reads[i].tbid));
		replace_string(buf,name,tmp);

		//logical reads
		sprintf(name,"$seg_logical_reads_%d$",i+1);
		sprintf(tmp,"%lu",dbbuf.seg_logical_reads[i].reads);
		replace_string(buf,name,tmp);

		//%Total
		sprintf(name,"$seg_logical_reads_total_%d$",i+1);
		sprintf(tmp,"%f",(float)dbbuf.seg_logical_reads[i].reads/(dbbuf.db_totallogicalreads==0?1:dbbuf.db_totallogicalreads)*100);
		replace_string(buf,name,tmp);
	}
}

void replace_seg_physical_reads(char* buf)
{
	int i;
	char 		tmp[256];
	char  		name[64];

	for (i = 0; i < PGPDR_SQL_MAX; i++)
	{
		//dbid
		sprintf(name,"$seg_physical_db_oid_%d$",i+1);
		sprintf(tmp,"%u",dbbuf.seg_physical_reads[i].dbid);
		replace_string(buf,name,tmp);

		//dbname
		sprintf(name,"$seg_physical_db_name_%d$",i+1);
		sprintf(tmp,"%s",dbbuf.dbname);
		replace_string(buf,name,tmp);

		//tbid
		sprintf(name,"$seg_physical_table_oid_%d$",i+1);
		sprintf(tmp,"%u",dbbuf.seg_physical_reads[i].tbid);
		replace_string(buf,name,tmp);

		//tbtype
		sprintf(name,"$seg_physical_table_type_%d$",i+1);
		sprintf(tmp,"%s",table_type(dbbuf.seg_physical_reads[i].tbid));
		replace_string(buf,name,tmp);

		//logical reads
		sprintf(name,"$seg_physical_reads_%d$",i+1);
		sprintf(tmp,"%lu",dbbuf.seg_physical_reads[i].reads);
		//	(float)dbbuf.sql_exec[i].rows/(dbbuf.sql_exec[i].calls==0?1:dbbuf.sql_exec[i].calls));
		replace_string(buf,name,tmp);

		//%Total
		sprintf(name,"$seg_physical_reads_total_%d$",i+1);
		sprintf(tmp,"%f",(float)dbbuf.seg_physical_reads[i].reads/(dbbuf.db_totalphysicalreads==0?1:dbbuf.db_totalphysicalreads)*100);
		replace_string(buf,name,tmp);
	}
}

void replace_seg_table_scans(char* buf)
{
	int i;
	char 		tmp[256];
	char  		name[64];

	for (i = 0; i < PGPDR_SQL_MAX; i++)
	{
		//dbid
		sprintf(name,"$seg_scans_db_oid_%d$",i+1);
		sprintf(tmp,"%u",dbbuf.seg_table_scans[i].dbid);
		replace_string(buf,name,tmp);

		//dbname
		sprintf(name,"$seg_scans_db_name_%d$",i+1);
		sprintf(tmp,"%s",dbbuf.dbname);
		replace_string(buf,name,tmp);

		//tbid
		sprintf(name,"$seg_scans_table_oid_%d$",i+1);
		sprintf(tmp,"%u",dbbuf.seg_table_scans[i].tbid);
		replace_string(buf,name,tmp);

		//tbtype
		sprintf(name,"$seg_scans_table_type_%d$",i+1);
		sprintf(tmp,"%s",table_type(dbbuf.seg_table_scans[i].tbid));
		replace_string(buf,name,tmp);

		//logical reads
		sprintf(name,"$seg_scans_num_%d$",i+1);
		sprintf(tmp,"%lu",dbbuf.seg_table_scans[i].reads);
		replace_string(buf,name,tmp);

		//%Total
		sprintf(name,"$seg_scans_total_%d$",i+1);
		sprintf(tmp,"%f",(float)dbbuf.seg_table_scans[i].reads/(dbbuf.db_totaltablescans==0?1:dbbuf.db_totaltablescans)*100);
		replace_string(buf,name,tmp);
	}
}

char* data2kmgtp(char* tmp,uint64 data)
{
	if ((uint64)data*8 > (uint64)1024*1024*1024*1024)
		sprintf(tmp,"%.3f P",(double)data*8/1024/1024/1024/1024);
	else if ((uint64)data*8 > (uint64)1024*1024*1024)
		sprintf(tmp,"%.3f T",(double)data*8/1024/1024/1024);
	else if (data*8 > 1024*1024)
		sprintf(tmp,"%.3f G",(double)data*8/1024/1024);
	else if (data*8 > 1024)
		sprintf(tmp,"%.3f M",(double)data*8/1024);
	else if (data*8 >= 0)
		sprintf(tmp,"%lu K",data*8);

	return tmp;
}

char* datapersec(char* tmp,uint64 data)
{
	if ((uint64)data*8 > (uint64)1024*1024*1024*(dbbuf.run_tv))
		sprintf(tmp,"%.3f T",(double)data*8/1024/1024/1024/(dbbuf.run_tv));
	else if ((uint64)data*8 > (uint64)1024*1024*(dbbuf.run_tv))
		sprintf(tmp,"%.3f G",(double)data*8/1024/1024/(dbbuf.run_tv));
	else if (data*8 > 1024*(dbbuf.run_tv*60))
		sprintf(tmp,"%.3f M",(double)data*8/1024/(dbbuf.run_tv));
	else if (data*8 >= 0)
		sprintf(tmp,"%.3f K",(double)data*8/(dbbuf.run_tv));

	return tmp;
}

void replace_io_stats(char* buf)
{
	char 		tmp[256];
	char  		name[64];


	//node_read_data
	sprintf(name,"$node_read_data$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.node_read_data));

	//node_write_data
	sprintf(name,"$node_write_data$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.node_write_data));

	//node_write_reqs
	sprintf(name,"$node_write_reqs$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.node_write_reqs));

	//buf_read_data
	sprintf(name,"$buf_read_data$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.buf_read_data));

	//buf_write_data
	sprintf(name,"$buf_write_data$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.buf_write_data));

	//buf_write_reqs
	sprintf(name,"$buf_write_reqs$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.buf_write_reqs));

	//total_read_data
	sprintf(name,"$total_read_data$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.node_read_data+dbbuf.io_stats.buf_read_data));

	//total_write_data
	sprintf(name,"$total_write_data$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.node_write_data+dbbuf.io_stats.buf_write_data));
	
	//total_write_reqs
	sprintf(name,"$total_write_reqs$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.node_write_reqs+dbbuf.io_stats.buf_write_reqs));

	//read per sec
	sprintf(name,"$node_read_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.node_read_data));

	//node_write_data per sec
	sprintf(name,"$node_write_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.node_write_data));

	//buf_read_data per sec
	sprintf(name,"$buf_read_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.buf_read_data));

	//buf_write_data per sec
	sprintf(name,"$buf_write_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.buf_write_data));

	//total_read_data per sec
	sprintf(name,"$total_read_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.node_read_data+dbbuf.io_stats.buf_read_data));

	//total_write_data per sec
	sprintf(name,"$total_write_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.node_write_data+dbbuf.io_stats.buf_write_data));

	//node read total%
	sprintf(name,"$node_read_total$");
	sprintf(tmp,"%.3f",(float)dbbuf.io_stats.node_read_data/((dbbuf.io_stats.node_read_data+dbbuf.io_stats.buf_read_data)==0?1:(dbbuf.io_stats.node_read_data+dbbuf.io_stats.buf_read_data))*100);
	replace_string(buf,name,tmp);

	//buf read total%
	sprintf(name,"$buf_read_total$");
	sprintf(tmp,"%.3f",(float)dbbuf.io_stats.buf_read_data/((dbbuf.io_stats.node_read_data+dbbuf.io_stats.buf_read_data)==0?1:(dbbuf.io_stats.node_read_data+dbbuf.io_stats.buf_read_data))*100);
	replace_string(buf,name,tmp);

	//total read total%
	sprintf(name,"$total_read_total$");
	sprintf(tmp,"%.3f",100.0);
	replace_string(buf,name,tmp);

	//node write total%
	sprintf(name,"$node_write_total$");
	sprintf(tmp,"%.3f",(float)dbbuf.io_stats.node_write_data/((dbbuf.io_stats.node_write_data+dbbuf.io_stats.buf_write_data)==0?1:(dbbuf.io_stats.node_write_data+dbbuf.io_stats.buf_write_data))*100);
	replace_string(buf,name,tmp);

	//buf write total%
	sprintf(name,"$buf_write_total$");
	sprintf(tmp,"%.3f",(float)dbbuf.io_stats.buf_write_data/((dbbuf.io_stats.node_write_data+dbbuf.io_stats.buf_write_data)==0?1:(dbbuf.io_stats.node_write_data+dbbuf.io_stats.buf_write_data))*100);
	replace_string(buf,name,tmp);

	//total write total%
	sprintf(name,"$total_write_total$");
	sprintf(tmp,"%.3f",100.0);
	replace_string(buf,name,tmp);

	//catalog_read_data
	sprintf(name,"$catalog_read_data$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.catalog_read_data));

	//rel_read_data
	sprintf(name,"$relation_read_data$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.rel_read_data));

	//temp_read_data
	sprintf(name,"$temp_read_data$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.temp_read_data));

	//rel_total_read_data
	sprintf(name,"$rel_total_read_data$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.catalog_read_data+dbbuf.io_stats.rel_read_data+dbbuf.io_stats.temp_read_data));

	//catalog_write_data
	sprintf(name,"$catalog_write_data$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.catalog_write_data));

	//rel_write_data
	sprintf(name,"$relation_write_data$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.rel_write_data));

	//temp_write_data
	sprintf(name,"$temp_write_data$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.temp_write_data));

	//rel_total_write_data
	sprintf(name,"$rel_total_write_data$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.catalog_write_data+dbbuf.io_stats.rel_write_data+dbbuf.io_stats.temp_write_data));

	//catalog_write_reqs
	sprintf(name,"$catalog_write_reqs$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.catalog_write_reqs));

	//rel_write_reqs
	sprintf(name,"$relation_write_reqs$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.rel_write_reqs));

	//temp_write_reqs
	sprintf(name,"$temp_write_reqs$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.temp_write_reqs));

	//rel_total_write_reqs
	sprintf(name,"$rel_total_write_reqs$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.catalog_write_reqs+dbbuf.io_stats.rel_write_reqs+dbbuf.io_stats.temp_write_reqs));

	//catalog read per sec
	sprintf(name,"$catalog_read_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.catalog_read_data));

	//catalog_write_data per sec
	sprintf(name,"$catalog_write_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.catalog_write_data));

	//rel_read_data per sec
	sprintf(name,"$relation_read_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.rel_read_data));

	//rel_write_data per sec
	sprintf(name,"$relation_write_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.rel_write_data));

	//temp_read_data per sec
	sprintf(name,"$temp_read_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.temp_read_data));

	//temp_write_data per sec
	sprintf(name,"$temp_write_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.temp_write_data));

	//rel_total_read_data per sec
	sprintf(name,"$rel_total_read_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.catalog_read_data+dbbuf.io_stats.rel_read_data+dbbuf.io_stats.temp_read_data));

	//rel_total_write_data per sec
	sprintf(name,"$rel_total_write_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.catalog_write_data+dbbuf.io_stats.rel_write_data+dbbuf.io_stats.temp_write_data));

	//catalog read total%
	sprintf(name,"$catalog_read_total$");
	sprintf(tmp,"%.3f",(float)dbbuf.io_stats.catalog_read_data/((dbbuf.io_stats.catalog_read_data+dbbuf.io_stats.rel_read_data+dbbuf.io_stats.temp_read_data)==0?1:(dbbuf.io_stats.catalog_read_data+dbbuf.io_stats.rel_read_data+dbbuf.io_stats.temp_read_data))*100);
	replace_string(buf,name,tmp);

	//rel read total%
	sprintf(name,"$relation_read_total$");
	sprintf(tmp,"%.3f",(float)dbbuf.io_stats.rel_read_data/((dbbuf.io_stats.catalog_read_data+dbbuf.io_stats.rel_read_data+dbbuf.io_stats.temp_read_data)==0?1:(dbbuf.io_stats.catalog_read_data+dbbuf.io_stats.rel_read_data+dbbuf.io_stats.temp_read_data))*100);
	replace_string(buf,name,tmp);

	//temp read total%
	sprintf(name,"$temp_read_total$");
	sprintf(tmp,"%.3f",(float)dbbuf.io_stats.temp_read_data/((dbbuf.io_stats.catalog_read_data+dbbuf.io_stats.rel_read_data+dbbuf.io_stats.temp_read_data)==0?1:(dbbuf.io_stats.catalog_read_data+dbbuf.io_stats.rel_read_data+dbbuf.io_stats.temp_read_data))*100);
	replace_string(buf,name,tmp);

	//total read total%
	sprintf(name,"$rel_total_read_total$");
	sprintf(tmp,"%.3f",100.0);
	replace_string(buf,name,tmp);

	//catalog node write total%
	sprintf(name,"$catalog_write_total$");
	sprintf(tmp,"%.3f",(float)dbbuf.io_stats.catalog_write_data/((dbbuf.io_stats.catalog_write_data+dbbuf.io_stats.rel_write_data+dbbuf.io_stats.temp_write_data)==0?1:(dbbuf.io_stats.catalog_write_data+dbbuf.io_stats.rel_write_data+dbbuf.io_stats.temp_write_data))*100);
	replace_string(buf,name,tmp);

	//rel write total%
	sprintf(name,"$relation_write_total$");
	sprintf(tmp,"%.3f",(float)dbbuf.io_stats.rel_write_data/((dbbuf.io_stats.catalog_write_data+dbbuf.io_stats.rel_write_data+dbbuf.io_stats.temp_write_data)==0?1:(dbbuf.io_stats.catalog_write_data+dbbuf.io_stats.rel_write_data+dbbuf.io_stats.temp_write_data))*100);
	replace_string(buf,name,tmp);

	//temp write total%
	sprintf(name,"$temp_write_total$");
	sprintf(tmp,"%.3f",(float)dbbuf.io_stats.temp_write_data/((dbbuf.io_stats.catalog_write_data+dbbuf.io_stats.rel_write_data+dbbuf.io_stats.temp_write_data)==0?1:(dbbuf.io_stats.catalog_write_data+dbbuf.io_stats.rel_write_data+dbbuf.io_stats.temp_write_data))*100);
	replace_string(buf,name,tmp);

	//total write total%
	sprintf(name,"$rel_total_write_total$");
	sprintf(tmp,"%.3f",100.0);
	replace_string(buf,name,tmp);

	/////////////////////////////
	//IOStat by Reltype summary//
	/////////////////////////////

	//io_rel_read
	sprintf(name,"$io_rel_read$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.io_rel_read));

	//io_index_read
	sprintf(name,"$io_index_read$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.io_index_read));

	//io_seq_read
	sprintf(name,"$io_seq_read$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.io_seq_read));

	//io_view_read
	sprintf(name,"$io_view_read$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.io_view_read));

	//io_mv_read
	sprintf(name,"$io_mv_read$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.io_mv_read));

	//io_comb_read
	sprintf(name,"$io_comb_read$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.io_comb_read));

	//io_toast_read
	sprintf(name,"$io_toast_read$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.io_toast_read));

	//io_foreign_read
	sprintf(name,"$io_foreign_read$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.io_foreign_read));

	//io_total_read
	sprintf(name,"$io_total_read$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.io_total_read));

	//io_rel_read_per_sec
	sprintf(name,"$io_rel_read_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.io_rel_read));

	//io_index_read_per_sec
	sprintf(name,"$io_index_read_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.io_index_read));

	//io_seq_read_per_sec
	sprintf(name,"$io_seq_read_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.io_seq_read));

	//io_view_read_per_sec
	sprintf(name,"$io_view_read_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.io_view_read));

	//io_mv_read_per_sec
	sprintf(name,"$io_mv_read_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.io_mv_read));

	//io_comb_read_per_sec
	sprintf(name,"$io_comb_read_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.io_comb_read));

	//io_toast_read_per_sec
	sprintf(name,"$io_toast_read_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.io_toast_read));

	//io_foreign_read_per_sec
	sprintf(name,"$io_foreign_read_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.io_foreign_read));

	//io_total_read_per_sec
	sprintf(name,"$io_total_read_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.io_total_read));

	//io_rel read total%
	sprintf(name,"$io_rel_read_total$");
	sprintf(tmp,"%.3f",(float)dbbuf.io_stats.io_rel_read/(dbbuf.io_stats.io_total_read==0?1:dbbuf.io_stats.io_total_read)*100);
	replace_string(buf,name,tmp);

	//io_index read total%
	sprintf(name,"$io_index_read_total$");
	sprintf(tmp,"%.3f",(float)dbbuf.io_stats.io_index_read/(dbbuf.io_stats.io_total_read==0?1:dbbuf.io_stats.io_total_read)*100);
	replace_string(buf,name,tmp);

	//io_seq_read total%
	sprintf(name,"$io_seq_read_total$");
	sprintf(tmp,"%.3f",(float)dbbuf.io_stats.io_seq_read/(dbbuf.io_stats.io_total_read==0?1:dbbuf.io_stats.io_total_read)*100);
	replace_string(buf,name,tmp);

	//io_view read total%
	sprintf(name,"$io_view_read_total$");
	sprintf(tmp,"%.3f",(float)dbbuf.io_stats.io_view_read/(dbbuf.io_stats.io_total_read==0?1:dbbuf.io_stats.io_total_read)*100);
	replace_string(buf,name,tmp);

	//io_mv read total%
	sprintf(name,"$io_mv_read_total$");
	sprintf(tmp,"%.3f",(float)dbbuf.io_stats.io_mv_read/(dbbuf.io_stats.io_total_read==0?1:dbbuf.io_stats.io_total_read)*100);
	replace_string(buf,name,tmp);

	//io_comb read total%
	sprintf(name,"$io_comb_read_total$");
	sprintf(tmp,"%.3f",(float)dbbuf.io_stats.io_comb_read/(dbbuf.io_stats.io_total_read==0?1:dbbuf.io_stats.io_total_read)*100);
	replace_string(buf,name,tmp);

	//io_toast read total%
	sprintf(name,"$io_toast_read_total$");
	sprintf(tmp,"%.3f",(float)dbbuf.io_stats.io_toast_read/(dbbuf.io_stats.io_total_read==0?1:dbbuf.io_stats.io_total_read)*100);
	replace_string(buf,name,tmp);

	//io_foreign read total%
	sprintf(name,"$io_foreign_read_total$");
	sprintf(tmp,"%.3f",(float)dbbuf.io_stats.io_foreign_read/(dbbuf.io_stats.io_total_read==0?1:dbbuf.io_stats.io_total_read)*100);
	replace_string(buf,name,tmp);

	//io_total read total%
	sprintf(name,"$io_total_read_total$");
	sprintf(tmp,"%.3f",100.0);
	replace_string(buf,name,tmp);

	//io_rel_read_buf
	sprintf(name,"$io_rel_read_buf$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.io_rel_buf));

	//io_index_read_buf
	sprintf(name,"$io_index_read_buf$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.io_index_buf));

	//io_seq_read_buf
	sprintf(name,"$io_seq_read_buf$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.io_seq_buf));

	//io_view_read_buf
	sprintf(name,"$io_view_read_buf$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.io_view_buf));

	//io_mv_read_buf
	sprintf(name,"$io_mv_read_buf$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.io_mv_buf));

	//io_comb_read_buf
	sprintf(name,"$io_comb_read_buf$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.io_comb_buf));

	//io_toast_read_buf
	sprintf(name,"$io_toast_read_buf$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.io_toast_buf));

	//io_foreign_read_buf
	sprintf(name,"$io_foreign_read_buf$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.io_foreign_buf));

	//io_total_read_buf
	sprintf(name,"$io_total_read_buf$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.io_total_buf));

	//io_rel_read_buf_per_sec
	sprintf(name,"$io_rel_read_buf_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.io_rel_buf));

	//io_index_read_buf_per_sec
	sprintf(name,"$io_index_read_buf_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.io_index_buf));

	//io_seq_read_buf_per_sec
	sprintf(name,"$io_seq_read_buf_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.io_seq_buf));

	//io_view_read_buf_per_sec
	sprintf(name,"$io_view_read_buf_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.io_view_buf));

	//io_mv_read_buf_per_sec
	sprintf(name,"$io_mv_read_buf_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.io_mv_buf));

	//io_comb_read_buf_per_sec
	sprintf(name,"$io_comb_read_buf_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.io_comb_buf));

	//io_toast_read_buf_per_sec
	sprintf(name,"$io_toast_read_buf_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.io_toast_buf));

	//io_foreign_read_buf_per_sec
	sprintf(name,"$io_foreign_read_buf_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.io_foreign_buf));

	//io_total_read_buf_per_sec
	sprintf(name,"$io_total_read_buf_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.io_total_buf));

	//io_rel_read_disk
	sprintf(name,"$io_rel_read_disk$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.io_rel_disk));

	//io_index_read_disk
	sprintf(name,"$io_index_read_disk$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.io_index_disk));

	//io_seq_read_disk
	sprintf(name,"$io_seq_read_disk$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.io_seq_disk));

	//io_view_read_disk
	sprintf(name,"$io_view_read_disk$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.io_view_disk));

	//io_mv_read_disk
	sprintf(name,"$io_mv_read_disk$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.io_mv_disk));

	//io_comb_read_disk
	sprintf(name,"$io_comb_read_disk$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.io_comb_disk));

	//io_toast_read_disk
	sprintf(name,"$io_toast_read_disk$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.io_toast_disk));

	//io_foreign_read_disk
	sprintf(name,"$io_foreign_read_disk$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.io_foreign_disk));

	//io_total_read_disk
	sprintf(name,"$io_total_read_disk$");
	replace_string(buf,name,data2kmgtp(tmp,dbbuf.io_stats.io_total_disk));

	//io_rel_read_disk_per_sec
	sprintf(name,"$io_rel_read_disk_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.io_rel_disk));

	//io_index_read_disk_per_sec
	sprintf(name,"$io_index_read_disk_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.io_index_disk));

	//io_seq_read_disk_per_sec
	sprintf(name,"$io_seq_read_disk_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.io_seq_disk));

	//io_view_read_disk_per_sec
	sprintf(name,"$io_view_read_disk_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.io_view_disk));

	//io_mv_read_disk_per_sec
	sprintf(name,"$io_mv_read_disk_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.io_mv_disk));

	//io_comb_read_disk_per_sec
	sprintf(name,"$io_comb_read_disk_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.io_comb_disk));

	//io_toast_read_disk_per_sec
	sprintf(name,"$io_toast_read_disk_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.io_toast_disk));

	//io_foreign_read_disk_per_sec
	sprintf(name,"$io_foreign_read_disk_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.io_foreign_disk));

	//io_total_read_disk_per_sec
	sprintf(name,"$io_total_read_disk_per_sec$");
	replace_string(buf,name,datapersec(tmp,dbbuf.io_stats.io_total_disk));
}

int find_wait_ev(char *name)
{
	int i;

	for (i = 0; i < PGPDR_WAIT_EVENT_SIZE; i++)
	{
		if (!strcmp(name,dbbuf.wait_stats[i].name))
		{
			return dbbuf.wait_stats[i].count;
		}
	}

	return 0;
}

int find_index_wait_ev(char *name)
{
	int i;

	for (i = 0; i < PGPDR_WAIT_EVENT_SIZE; i++)
	{
		if (!strcmp(name,dbbuf.wait_stats[i].name))
		{
			return i;
		}
	}

	return -1;
}

int find_index_wait_ev_begin(char *name)
{
	int i;

	for (i = 0; i < PGPDR_WAIT_EVENT_SIZE; i++)
	{
		if (!strcmp(name,wait_stats_begin[i].name))
		{
			return i;
		}
	}

	return -1;
}


void replace_wait_stats(char* buf)
{
	char 	tmp[256];
	char  	name[128];
	int  	i;

	for (i = 0; i < PGPDR_WAIT_EVENT_SIZE; i++)
	{
		if (i >= 87 && i <= 99)
		{
			sprintf(name,"$%s_time$", wait_ev[i]);
			sprintf(tmp,"%.3f",(float)find_wait_ev(wait_ev[i])/100);
			replace_string(buf,name,tmp);
			sprintf(name,"$%s_total$", wait_ev[i]);
			sprintf(tmp,"%.3f",(float)find_wait_ev(wait_ev[i])/dbbuf.wait_activity*100);
			replace_string(buf,name,tmp);
		}
		else
		{
			sprintf(name,"$%s_time$", wait_ev[i]);
			sprintf(tmp,"%.3f",(float)find_wait_ev(wait_ev[i])/100);
			replace_string(buf,name,tmp);
			sprintf(name,"$%s_total$", wait_ev[i]);
			sprintf(tmp,"%.3f",(float)find_wait_ev(wait_ev[i])/dbbuf.wait_total*100);
			replace_string(buf,name,tmp);
		}
	}
}

void replace_wait_top10(char* buf,int maxevent)
{
	char 	tmp[256];
	char  	name[128];
	int  	i;
	
	for (i = 0; i < 10; i++)
	{

		if (i >= maxevent)
		{
			sprintf(name,"$wait_top10_name_%d$", i+1);
			sprintf(tmp,"%s","&nbsp;");
			replace_string(buf,name,tmp);

			sprintf(name,"$wait_top10_time_%d$", i+1);
			sprintf(tmp,"%s","&nbsp;");
			replace_string(buf,name,tmp);
			
			sprintf(name,"$wait_top10_percent_%d$", i+1);
			sprintf(tmp,"%s","&nbsp;");
			replace_string(buf,name,tmp);
			
			sprintf(name,"$wait_top10_type_%d$", i+1);
			sprintf(tmp,"%s","&nbsp;");
			replace_string(buf,name,tmp);

		}
		else 
		{
			sprintf(name,"$wait_top10_name_%d$", i+1);
			sprintf(tmp,"%s",dbbuf.wait_top10[i].name);
			replace_string(buf,name,tmp);

			sprintf(name,"$wait_top10_time_%d$", i+1);
			sprintf(tmp,"%.3f",(float)dbbuf.wait_top10[i].count/100);
			replace_string(buf,name,tmp);
			
			sprintf(name,"$wait_top10_percent_%d$", i+1);
			sprintf(tmp,"%.3f",(float)dbbuf.wait_top10[i].count/dbbuf.wait_total*100);
			replace_string(buf,name,tmp);

			sprintf(name,"$wait_top10_type_%d$", i+1);
			sprintf(tmp,"%s",dbbuf.wait_top10[i].type);
			replace_string(buf,name,tmp);

		}
	}
}

void replace_summary(char* buf)
{
	char 		tmp[256];

	//dbname
	replace_string(buf,"$dbname$",dbbuf.dbname);
	//dbid
	sprintf(tmp,"%u",dbbuf.databaseid);	
	replace_string(buf,"$dboid$",tmp);
	//snapbegin
	sprintf(tmp,"%ld",snap_begin);	
	replace_string(buf,"$snapbegin$",tmp);
	//snapend
	sprintf(tmp,"%ld",snap_end);	
	replace_string(buf,"$snapend$",tmp);

	//startup time
	replace_string(buf,"$databasestarttime$",dbbuf.dbtime);
	//pg version
	replace_string(buf,"$databaseversion$",dbbuf.dbver);
	//host name
	replace_string(buf,"$databasehost$",dbbuf.dbhost);
	//platform
	replace_string(buf,"$databaseplatform$",dbbuf.platform);
	//cpu 
	sprintf(tmp,"%u",dbbuf.cpus);	
	replace_string(buf,"$dbcpus$",tmp);
	//cpu core
	sprintf(tmp,"%u",dbbuf.cpucore);	
	replace_string(buf,"$dbcpucore$",tmp);
	//memery
	sprintf(tmp, "%.3f", (float)dbbuf.dbmem/1024/1024/1024);
	replace_string(buf,"$dbmemory$",tmp);

	//begin time
	replace_string(buf,"$snapbegintime$",dbbuf.begintime);
	//end time
	replace_string(buf,"$snapendtime$",dbbuf.endtime);

	//elapsed
	replace_string(buf,"$dbelapsed$",dbbuf.elapsed);
	//db time
	//sprintf(tmp, "%.2f", (float)(snap_end-snap_begin+1)*dbbuf.naptime);
	sprintf(tmp, "%s", dbbuf.str_run_tv);
	replace_string(buf,"$dbdbtime$",tmp);

	//Transactions
	if (dbbuf.n_xact_commit < 0)
		dbbuf.n_xact_commit = 0;
	
	sprintf(tmp, "%lu", dbbuf.n_xact_commit);
	replace_string(buf,"$Transactions_cnt$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_xact_commit/dbbuf.run_tv);
	replace_string(buf,"$Transactions_sec$",tmp);

	if (dbbuf.n_xact_commit == 0)
		dbbuf.n_xact_commit = 1;
	sprintf(tmp, "%.2f", (float)dbbuf.n_xact_commit/dbbuf.n_xact_commit);
	replace_string(buf,"$Transactions_xac$",tmp);

	//Rollback
	if (dbbuf.n_xact_rollback < 0)
		dbbuf.n_xact_rollback = 0;
	sprintf(tmp, "%lu", dbbuf.n_xact_rollback);
	replace_string(buf,"$Rollback_cnt$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_xact_rollback/dbbuf.run_tv);
	replace_string(buf,"$Rollback_sec$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_xact_rollback/dbbuf.n_xact_commit);
	replace_string(buf,"$Rollback_xac$",tmp);

	//Blocks fetched
	if (dbbuf.n_blocks_fetched < 0)
		dbbuf.n_blocks_fetched=0;
	sprintf(tmp, "%lu", dbbuf.n_blocks_fetched);
	replace_string(buf,"$Blocks_fetched_cnt$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_blocks_fetched/dbbuf.run_tv);
	replace_string(buf,"$Blocks_fetched_sec$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_blocks_fetched/dbbuf.n_xact_commit);
	replace_string(buf,"$Blocks_fetched_xac$",tmp);

	//Blocks hit
	if (dbbuf.n_blocks_hit < 0)
		dbbuf.n_blocks_hit=0;
	sprintf(tmp, "%lu", dbbuf.n_blocks_hit);
	replace_string(buf,"$Blocks_hit_cnt$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_blocks_hit/dbbuf.run_tv);
	replace_string(buf,"$Blocks_hit_sec$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_blocks_hit/dbbuf.n_xact_commit);
	replace_string(buf,"$Blocks_hit_xac$",tmp);

	//Tuples returned
	if (dbbuf.n_tuples_returned < 0)
		dbbuf.n_tuples_returned=0;
	sprintf(tmp, "%lu", dbbuf.n_tuples_returned);
	replace_string(buf,"$Tuples_returned_cnt$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_tuples_returned/dbbuf.run_tv);
	replace_string(buf,"$Tuples_returned_sec$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_tuples_returned/dbbuf.n_xact_commit);
	replace_string(buf,"$Tuples_returned_xac$",tmp);

	//Tuples fetched
	if (dbbuf.n_tuples_fetched < 0)
		dbbuf.n_tuples_fetched=0;
	sprintf(tmp, "%lu", dbbuf.n_tuples_fetched);
	replace_string(buf,"$Tuples_fetched_cnt$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_tuples_fetched/dbbuf.run_tv);
	replace_string(buf,"$Tuples_fetched_sec$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_tuples_fetched/dbbuf.n_xact_commit);
	replace_string(buf,"$Tuples_fetched_xac$",tmp);

	//Tuples inserted
	if (dbbuf.n_tuples_inserted < 0)
		dbbuf.n_tuples_inserted=0;
	sprintf(tmp, "%lu", dbbuf.n_tuples_inserted);
	replace_string(buf,"$Tuples_inserted_cnt$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_tuples_inserted/dbbuf.run_tv);
	replace_string(buf,"$Tuples_inserted_sec$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_tuples_inserted/dbbuf.n_xact_commit);
	replace_string(buf,"$Tuples_inserted_xac$",tmp);

	//Tuples updated
	if (dbbuf.n_tuples_updated < 0)
		dbbuf.n_tuples_updated=0;
	sprintf(tmp, "%lu", dbbuf.n_tuples_updated);
	replace_string(buf,"$Tuples_updated_cnt$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_tuples_updated/dbbuf.run_tv);
	replace_string(buf,"$Tuples_updated_sec$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_tuples_updated/dbbuf.n_xact_commit);
	replace_string(buf,"$Tuples_updated_xac$",tmp);

	//Tuples deleted
	if (dbbuf.n_tuples_deleted < 0)
		dbbuf.n_tuples_deleted=0;
	sprintf(tmp, "%lu", dbbuf.n_tuples_deleted);
	replace_string(buf,"$Tuples_deleted_cnt$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_tuples_deleted/dbbuf.run_tv);
	replace_string(buf,"$Tuples_deleted_sec$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_tuples_deleted/dbbuf.n_xact_commit);
	replace_string(buf,"$Tuples_deleted_xac$",tmp);

	//Conflict tablespace
	if (dbbuf.n_conflict_tablespace < 0)
		dbbuf.n_conflict_tablespace=0;
	sprintf(tmp, "%lu", dbbuf.n_conflict_tablespace);
	replace_string(buf,"$Conflict_tablespace_cnt$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_conflict_tablespace/dbbuf.run_tv);
	replace_string(buf,"$Conflict_tablespace_sec$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_conflict_tablespace/dbbuf.n_xact_commit);
	replace_string(buf,"$Conflict_tablespace_xac$",tmp);

	//Conflict lock
	if (dbbuf.n_conflict_lock < 0)
		dbbuf.n_conflict_lock=0;
	sprintf(tmp, "%lu", dbbuf.n_conflict_lock);
	replace_string(buf,"$Conflict_lock_cnt$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_conflict_lock/dbbuf.run_tv);
	replace_string(buf,"$Conflict_lock_sec$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_conflict_lock/dbbuf.n_xact_commit);
	replace_string(buf,"$Conflict_lock_xac$",tmp);

	//Conflict snapshot
	if (dbbuf.n_conflict_snapshot < 0)
		dbbuf.n_conflict_snapshot=0;
	sprintf(tmp, "%lu", dbbuf.n_conflict_snapshot);
	replace_string(buf,"$Conflict_snapshot_cnt$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_conflict_snapshot/dbbuf.run_tv);
	replace_string(buf,"$Conflict_snapshot_sec$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_conflict_snapshot/dbbuf.n_xact_commit);
	replace_string(buf,"$Conflict_snapshot_xac$",tmp);

	//Conflict bufferpin
	if (dbbuf.n_conflict_bufferpin < 0)
		dbbuf.n_conflict_bufferpin=0;
	sprintf(tmp, "%lu", dbbuf.n_conflict_bufferpin);
	replace_string(buf,"$Conflict_bufferpin_cnt$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_conflict_bufferpin/dbbuf.run_tv);
	replace_string(buf,"$Conflict_bufferpin_sec$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_conflict_bufferpin/dbbuf.n_xact_commit);
	replace_string(buf,"$Conflict_bufferpin_xac$",tmp);

	//Conflict deadlock
	if (dbbuf.n_conflict_startup_deadlock < 0)
		dbbuf.n_conflict_startup_deadlock=0;
	sprintf(tmp, "%lu", dbbuf.n_conflict_startup_deadlock);
	replace_string(buf,"$Conflict_deadlock_cnt$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_conflict_startup_deadlock/dbbuf.run_tv);
	replace_string(buf,"$Conflict_deadlock_sec$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_conflict_startup_deadlock/dbbuf.n_xact_commit);
	replace_string(buf,"$Conflict_deadlock_xac$",tmp);

	//Temp files
	if (dbbuf.n_temp_files < 0)
		dbbuf.n_temp_files=0;
	sprintf(tmp, "%lu", dbbuf.n_temp_files);
	replace_string(buf,"$Temp_files_cnt$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_temp_files/dbbuf.run_tv);
	replace_string(buf,"$Temp_files_sec$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_temp_files/dbbuf.n_xact_commit);
	replace_string(buf,"$Temp_files_xac$",tmp);

	//Temp bytes
	if (dbbuf.n_temp_bytes < 0)
		dbbuf.n_temp_bytes=0;
	sprintf(tmp, "%lu", dbbuf.n_temp_bytes);
	replace_string(buf,"$Temp_bytes_cnt$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_temp_bytes/dbbuf.run_tv);
	replace_string(buf,"$Temp_bytes_sec$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_temp_bytes/dbbuf.n_xact_commit);
	replace_string(buf,"$Temp_bytes_xac$",tmp);

	//Deadlocks
	if (dbbuf.n_deadlocks < 0)
		dbbuf.n_deadlocks=0;
	sprintf(tmp, "%lu", dbbuf.n_deadlocks);
	replace_string(buf,"$Deadlocks_cnt$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_deadlocks/dbbuf.run_tv);
	replace_string(buf,"$Deadlocks_sec$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_deadlocks/dbbuf.n_xact_commit);
	replace_string(buf,"$Deadlocks_xac$",tmp);

	//Block read time
	if (dbbuf.n_block_read_time < 0)
		dbbuf.n_block_read_time=0;
	sprintf(tmp, "%.2f", (float)dbbuf.n_block_read_time/1000);
	replace_string(buf,"$Block_read_time_cnt$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_block_read_time/dbbuf.run_tv);
	replace_string(buf,"$Block_read_time_sec$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_block_read_time/dbbuf.n_xact_commit);
	replace_string(buf,"$Block_read_time_xac$",tmp);

	//Block write time
	if (dbbuf.n_block_write_time < 0)
		dbbuf.n_block_write_time=0;
	sprintf(tmp, "%lu", dbbuf.n_block_write_time);
	replace_string(buf,"$Block_write_time_cnt$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_block_write_time/dbbuf.run_tv);
	replace_string(buf,"$Block_write_time_sec$",tmp);
	sprintf(tmp, "%.2f", (float)dbbuf.n_block_write_time/dbbuf.n_xact_commit);
	replace_string(buf,"$Block_write_time_xac$",tmp);
}

void read_pg_class(char* conninfo)
{
	PGconn	   *conn;
	PGresult   *res;
	int  		i;

	conn = PQconnectdb(conninfo);
	if (PQstatus(conn) != CONNECTION_OK)
	{
		fprintf(stderr, "Connection to database failed: %s",
				PQerrorMessage(conn));
		exit_nicely(conn);
	}

	//relation
	res = PQexec(conn, "select oid from pg_class where relkind='r';");
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	for (i = 0; i < PQntuples(res) && i < PGPDR_TAB_HASH_SIZE; i++)
	{
		relr[i] = strtoull(PQgetvalue(res, i, 0),NULL,10);
	}
	PQclear(res);

	//index
	res = PQexec(conn, "select oid from pg_class where relkind='i';");
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	for (i = 0; i < PQntuples(res) && i < PGPDR_TAB_HASH_SIZE; i++)
	{
		reli[i] = strtoull(PQgetvalue(res, i, 0),NULL,10);
	}
	PQclear(res);

	//sequnce
	res = PQexec(conn, "select oid from pg_class where relkind='s';");
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	for (i = 0; i < PQntuples(res) && i < PGPDR_TAB_HASH_SIZE; i++)
	{
		rels[i] = strtoull(PQgetvalue(res, i, 0),NULL,10);
	}
	PQclear(res);

	//view
	res = PQexec(conn, "select oid from pg_class where relkind='v';");
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	for (i = 0; i < PQntuples(res) && i < PGPDR_TAB_HASH_SIZE; i++)
	{
		relv[i] = strtoull(PQgetvalue(res, i, 0),NULL,10);
	}
	PQclear(res);

	//matierial view
	res = PQexec(conn, "select oid from pg_class where relkind='m';");
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	for (i = 0; i < PQntuples(res) && i < PGPDR_TAB_HASH_SIZE; i++)
	{
		relm[i] = strtoull(PQgetvalue(res, i, 0),NULL,10);
	}
	PQclear(res);

	//combine
	res = PQexec(conn, "select oid from pg_class where relkind='c';");
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	for (i = 0; i < PQntuples(res) && i < PGPDR_TAB_HASH_SIZE; i++)
	{
		relc[i] = strtoull(PQgetvalue(res, i, 0),NULL,10);
	}
	PQclear(res);

	//toast
	res = PQexec(conn, "select oid from pg_class where relkind='t';");
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	for (i = 0; i < PQntuples(res) && i < PGPDR_TAB_HASH_SIZE; i++)
	{
		relt[i] = strtoull(PQgetvalue(res, i, 0),NULL,10);
	}
	PQclear(res);

	//foreign
	res = PQexec(conn, "select oid from pg_class where relkind='f';");
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	for (i = 0; i < PQntuples(res) && i < PGPDR_TAB_HASH_SIZE; i++)
	{
		relf[i] = strtoull(PQgetvalue(res, i, 0),NULL,10);
	}
	PQclear(res);

	/* close the connection to the database and cleanup */
	PQfinish(conn);
}

char* oid2str(char* tmp,char kind)
{
	int	i;
	char	temp_str[OIDS_LENGTH];

	memset(temp_str,0x00,OIDS_LENGTH);
	
	switch (kind)
	{
		case 'r':
			for (i = 0; i < PGPDR_TAB_HASH_SIZE && relr[i] != 0; i++)
			{
				if (tmp[0])
				{
					memset(temp_str,0x00,OIDS_LENGTH);
					strcpy(temp_str,tmp);
					memset(tmp,0x00,OIDS_LENGTH);
					sprintf(tmp,"%s,%u",temp_str,relr[i]);
				}
				else
					sprintf(tmp,"%u",relr[i]);
			}
			break;
		case 'i':
			for (i = 0; i < PGPDR_TAB_HASH_SIZE && reli[i] != 0; i++)
			{
				if (tmp[0])
				{
					memset(temp_str,0x00,OIDS_LENGTH);
					strcpy(temp_str,tmp);
					memset(tmp,0x00,OIDS_LENGTH);
					sprintf(tmp,"%s,%u",temp_str,reli[i]);
				}
				else
					sprintf(tmp,"%u",reli[i]);
			}
			break;
		case 's':
			for (i = 0; i < PGPDR_TAB_HASH_SIZE && rels[i] != 0; i++)
			{
				if (tmp[0])
				{
					memset(temp_str,0x00,OIDS_LENGTH);
					strcpy(temp_str,tmp);
					memset(tmp,0x00,OIDS_LENGTH);
					sprintf(tmp,"%s,%u",temp_str,rels[i]);
				}
				else
					sprintf(tmp,"%u",rels[i]);
			}
			break;
		case 'v':
			for (i = 0; i < PGPDR_TAB_HASH_SIZE && relv[i] != 0; i++)
			{
				if (tmp[0])
				{
					memset(temp_str,0x00,OIDS_LENGTH);
					strcpy(temp_str,tmp);
					memset(tmp,0x00,OIDS_LENGTH);
					sprintf(tmp,"%s,%u",temp_str,relv[i]);
				}
				else
					sprintf(tmp,"%u",relv[i]);
			}
			break;
		case 'm':
			for (i = 0; i < PGPDR_TAB_HASH_SIZE && relm[i] != 0; i++)
			{
				if (tmp[0])
				{
					memset(temp_str,0x00,OIDS_LENGTH);
					strcpy(temp_str,tmp);
					memset(tmp,0x00,OIDS_LENGTH);
					sprintf(tmp,"%s,%u",temp_str,relm[i]);
				}
				else
					sprintf(tmp,"%u",relm[i]);
			}
			break;
		case 'c':
			for (i = 0; i < PGPDR_TAB_HASH_SIZE && relc[i] != 0; i++)
			{
				if (tmp[0])
				{
					memset(temp_str,0x00,OIDS_LENGTH);
					strcpy(temp_str,tmp);
					memset(tmp,0x00,OIDS_LENGTH);
					sprintf(tmp,"%s,%u",temp_str,relc[i]);
				}
				else
					sprintf(tmp,"%u",relc[i]);
			}
			break;
		case 't':
			for (i = 0; i < PGPDR_TAB_HASH_SIZE && relt[i] != 0; i++)
			{
				if (tmp[0])
				{
					memset(temp_str,0x00,OIDS_LENGTH);
					strcpy(temp_str,tmp);
					memset(tmp,0x00,OIDS_LENGTH);
					sprintf(tmp,"%s,%u",temp_str,relt[i]);
				}
				else
					sprintf(tmp,"%u",relt[i]);
			}
			break;
		case 'f':
			for (i = 0; i < PGPDR_TAB_HASH_SIZE && relf[i] != 0; i++)
			{
				if (tmp[0])
				{
					memset(temp_str,0x00,OIDS_LENGTH);
					strcpy(temp_str,tmp);
					memset(tmp,0x00,OIDS_LENGTH);
					sprintf(tmp,"%s,%u",temp_str,relf[i]);
				}
				else
					sprintf(tmp,"%u",relf[i]);
			}
			break;				
	}

	if (!tmp[0])
		sprintf(tmp,"%s","null");

	return tmp;
}


#ifdef WIN32
int see_more_win32(int per)
{
	/*
	 * print message, wait for response, return # of lines to advance
	 * q means no, space means yes CRmeans one line
	 */
	int c;

	HANDLE myhandle = GetStdHandle(STD_OUTPUT_HANDLE);
	WORD wOldColorAttrs;
	CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
	GetConsoleScreenBufferInfo(myhandle,&csbiInfo);
	wOldColorAttrs = csbiInfo.wAttributes;
	
	SetConsoleTextAttribute(myhandle,0x00 | 0xf0);
	
	printf("more?--q for quit--%d%--",per);
	SetConsoleTextAttribute(myhandle,wOldColorAttrs);
	
	while ((c=getch())!=EOF)
	{
		if (c == 'q')
			return 0;
		if (c == ' ')
			return PAGELEN;
		if (c == '\r')
			return 1;
	}
	return 0;
}

int control_line_win32(HANDLE myhandle,int mode)
{
	
	COORD coord;
	CONSOLE_SCREEN_BUFFER_INFO csbiInfo;
	GetConsoleScreenBufferInfo(myhandle,&csbiInfo);

	if (mode == 1)
	{
		coord.X = csbiInfo.dwCursorPosition.X;
		coord.Y = csbiInfo.dwCursorPosition.Y;
		coord.X = 0;
		coord.Y--;
	}
	SetConsoleCursorPosition(myhandle,coord);
	
	return 0;
}

#else
int see_more(FILE* cmd,int per)
{
	/*
	 * print message, wait for response, return # of lines to advance
	 * q means no, space means yes CRmeans one line
	 */
	int c;
	int ret = system("stty -icanon");
	if (ret != 0)
	{
		fprintf(stderr, "call system error:  %d\n", ret);
	}

	printf("\033[7m more?--q for quit--%d%%--\033[m",per);

	while ((c=getc(cmd))!=EOF)
	{
		if (c == 'q')
			return 0;
		if (c == ' ')
			return PAGELEN;
		if (c == '\n')
			return 1;
	}
	return 0;
}

#endif

/*
 * print help text
 */
static void
pdr_usage()
{
	printf(_("pdr_report is a utility to generate the performance diagnosis report of HighGo Database server.\n\n"));
	printf(_("Usage:\n"));
	printf(_("  pdr_report [OPTION] [PARA]\n"));
	printf(_("\nOptions:\n"));
	printf(_("  -p                          database server port (default: \"%s\")\n"),"5866");
	printf(_("  -h, --help                  show this help, then exit\n"));
}

static int64
strtoint64(const char *str)
{
	const char *ptr = str;
	int64		result = 0;
	int			sign = 1;

	/*
	 * Do our own scan, rather than relying on sscanf which might be broken
	 * for long long.
	 */

	/* skip leading spaces */
	while (*ptr && isspace((unsigned char) *ptr))
		ptr++;

	/* handle sign */
	if (*ptr == '-')
	{
		ptr++;

		/*
		 * Do an explicit check for INT64_MIN.  Ugly though this is, it's
		 * cleaner than trying to get the loop below to handle it portably.
		 */
		if (strncmp(ptr, "9223372036854775808", 19) == 0)
		{
			result = PG_INT64_MIN;
			ptr += 19;
			goto gotdigits;
		}
		sign = -1;
	}
	else if (*ptr == '+')
		ptr++;

	/* require at least one digit */
	if (!isdigit((unsigned char) *ptr))
		fprintf(stderr, "invalid input syntax for integer: \"%s\"\n", str);

	/* process digits */
	while (*ptr && isdigit((unsigned char) *ptr))
	{
		int64		tmp = result * 10 + (*ptr++ - '0');

		if ((tmp / 10) != result)	/* overflow? */
			fprintf(stderr, "value \"%s\" is out of range for type bigint\n", str);
		result = tmp;
	}

gotdigits:

	/* allow trailing whitespace, but not other trailing chars */
	while (*ptr != '\0' && isspace((unsigned char) *ptr))
		ptr++;

	if (*ptr != '\0')
		fprintf(stderr, "invalid input syntax for integer: \"%s\"\n", str);

	return ((sign < 0) ? -result : result);
}

int
main(int argc, char **argv)
{
	char	conninfo[256];
	PGconn	*conn;
	PGresult *res;

	int	i;
	FILE	*fpout;
	FILE	*fpin;
	char	buf[512000]={0};
	int  	len;

	char  	sql[51200];
	char 	*p;
	char  	oids[OIDS_LENGTH]={0};
	char 	*lastsep;
	char	my_exec_path[MAXPGPATH];
	char	path[MAXPGPATH];
	int  	port=0;

	char 	username[32];
	char	outpath[256]={0};
	char 	password[64]={0};
	char 	database[64]={0};
	char 	db_store[64]={0};
	char	cScanf;

	int64 	minid=0;
	int64 	maxid=0;
	char 	mintime[32]={0};
	char 	maxtime[32]={0};
	char 	outfile[64]={0};
#ifdef WIN32
	unsigned long uname_length = 0;
#else
	struct passwd *pwd;
#endif

	int	iScanfCount = 0;
	int	iAllSnapCount = 0;
	char	line[LINELEN];
	int	num_of_lines = 0;
	int	reply;

	FILE	*fp_tty = NULL;
	int	per = 0;
	char 	inputNum[12];
	int	iDbStartCount = 0;
	bool	bExist_begin = false;
	bool	bExist_end = false;

	int	wait_ev_index = 0;
	char	wait_ev_name[64];
	int	wait_stat_count = 0;
	int	j;
	int	maxevent = 0;
	char  	subsql[512];

	int	wait_ev_index_subbegin = 0;
	bool	dbstflag = false;
	int	printf_pageline = 0;
	char 	dbsysinfo[512];
	char	*tmppasswd = NULL;
	char	title_pwd[128]={0};

	bool	isdumped = false;
	char	conninfo_report[256];

	/* only for data check */
	double	snap_data_early;
	double	snap_data_late;
	int64	snap_check;
	int64	result_count = 0;
	
	/* find the exec path */
	if (find_my_exec(argv[0], my_exec_path) < 0)
	{
		fprintf(stderr, _("%s: could not find own program executable\n"), "pdr_report");
		exit(1);
	}

	/* we will use this to find pg_pdr.html	*/
	lastsep = strrchr(my_exec_path, '/');
	if (lastsep)
		*lastsep = '\0';

	/* pdr_report parameter */
	if (argc == 2)
	{
		if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)
		{
			pdr_usage();
			exit(0);
		}
		else 
		{
			/* initdb: invalid option -- 'a' Try "initdb --help" for more information.*/
			printf("pdr_report: invalid option -- '%s'\n",argv[1]);
			printf("Try \"pdr_report --help\" for more information.\n");
			return false;
		}
	}
	else if (argc == 3)
	{
		if (argv[1][0] != '-')
		{
			printf("pdr_report: invalid option -- '%s'\n",argv[1]);
			printf("Try \"pdr_report --help\" for more information.\n");

			return false;
		}
		while ((j = getopt(argc, argv, "p:")) != -1) 
		{
			switch (j) 
			{
		        	case 'p':
		            		port = strtoint64(optarg);
		            		break;
		        	case '?':
		            		return false;
			}
		}
	}
	else if (argc > 3 )
	{
		printf("pdr_report: invalid para.\n");
		return false;
	}
	if (port < 0 || port > 61000)
	{
		printf("Input port out of range %d\n",port);
		return false;
	}

#ifdef WIN32
	GetUserName(username,&uname_length);
	if (uname_length > 31)
	{
		/**todo username length check**/
	}
#else
	pwd = getpwuid(getuid());
	strcpy(username,pwd->pw_name);
#endif

	printf("================================================================================\n");
	printf("-----------------------------welcome to pdr report------------------------------\n");
	printf("================================================================================\n");
	
	while (1)
	{
		printf("please input system user name:\n");
		dbstflag = false;
		iScanfCount = 0;
		while (1)
		{
			if (scanf("%c",&cScanf) != 1)
			{
				/* do nothing */
			}

			if (iScanfCount > 30)
			{
				/*
 				 * Ignore bytes more than 31
				 * break;
				 * if break here and you input more than 32 bytes,those bytes will be the input of password.
				 */
			}
			if (cScanf == '\n')
			{
				if (iScanfCount == 0)
					break;
				else if (iScanfCount < 31)
				{
					username[iScanfCount] = 0x00;
					dbstflag = true;
					break;
				}
				else 
				{
					dbstflag = true;
					break;
				}
			}
			if (iScanfCount == 0)
			{
				memset(username,0x00,sizeof(username));
			}
			if (iScanfCount == 31)
			{
				username[iScanfCount] = 0x00;
			}
			else if (iScanfCount < 31)
			{
				username[iScanfCount] = cScanf;
			}
			iScanfCount++;
		}
		if (dbstflag)
			break;
	}
	
#ifdef WIN32
	memset(password,0x00,sizeof(password));
	memset(title_pwd,0x00,sizeof(title_pwd));
	while (1)
	{
		i = 0;
		memset(password,0x00,sizeof(password));
		printf("please input password for %s:\n",username);
		do
		{
			password[i] = getch();
			if (password[i] == '\r')
				break;
			if (password[i] == '\b')
			{
				if (password == 0)
				{
					printf("\a");
					continue;
				}
				i = i - 1;
				printf("\b \b");
			}
			else
			{
				i = i + 1;
				printf("*");
			}
		}while (password[i] != '\n' && i < 63);
		/* password */
		printf("\n");
		if (strlen(password) >= 2)
			break;
	}
#else	
	
	memset(password,0x00,sizeof(password));
	memset(title_pwd,0x00,sizeof(title_pwd));
	while (1)
	{
		iScanfCount = 0;
		dbstflag = false;
		sprintf(title_pwd,"please input password for %s:\n",username);
		tmppasswd = getpass(title_pwd);

		/* deal backspace of tmppasswd */
		/* test */
		for (i = 0; i < strlen(tmppasswd); i++)
		{
			if (tmppasswd[i] != '\b' && tmppasswd[i] != '\n')
			{
				password[iScanfCount] = tmppasswd[i];
				iScanfCount++;
			}
			else if (tmppasswd[i] != '\n')
			{
				if (iScanfCount == 0)
					continue;
				password[iScanfCount] = 0x00;
				iScanfCount--;
			}
			else 
			{
				password[iScanfCount] = 0x00;
			}
			if (iScanfCount >= 62)
			{
				dbstflag = false;
				break;
			}
		}
		if (iScanfCount > 0)
			dbstflag = true;
		if (dbstflag)
			break;
	}
#endif
	
	/* db_store */
	while (1)
	{
		printf("please input the database where the snapshot is stored:\n");
		dbstflag = false;
		iScanfCount = 0;
		while (1)
		{
			if (scanf("%c",&cScanf) != 1)
			{
				/* do nothing */
			}

			if (iScanfCount > 62)
			{
				/* break; */
			}
			if (cScanf == '\n')
			{
				if (iScanfCount == 0)
					break;
				else if (iScanfCount < 63)
				{
					db_store[iScanfCount] = 0x00;
					dbstflag = true;
					break;
				}
				else
				{
					dbstflag = true;
					break;
				}
			}
			if (iScanfCount == 0)
			{
				memset(db_store,0x00,sizeof(db_store));
			}
			if (iScanfCount == 63)
			{
				db_store[iScanfCount] = 0x00;
			}
			else if (iScanfCount < 63)
			{
				db_store[iScanfCount] = cScanf;
			}
			iScanfCount++;
		}
		if (dbstflag)
			break;
		
	}
	
	while (1)
	{
		printf("please input which database you want to be reported:\n");
		dbstflag = false;
		iScanfCount = 0;
		while (1)
		{
			if (scanf("%c",&cScanf) != 1)
			{
				/* do nothing */
			}
				
			if (iScanfCount > 62)
			{
				/* break; */
			}
			if (cScanf == '\n')
			{
				if (iScanfCount == 0)
					break;
				else if (iScanfCount < 63)
				{
					dbstflag = true;
					database[iScanfCount] = 0x00;
					break;
				}
				else
				{
					dbstflag = true;
					break;
				}
			}
			if (iScanfCount == 0)
			{
				memset(database,0x00,sizeof(database));
			}
			if (iScanfCount == 63)
			{
				database[iScanfCount] = 0x00;
			}
			else if (iScanfCount < 63)
			{
				database[iScanfCount] = cScanf;
			}
			iScanfCount++;
		}
		if (dbstflag)
			break;

	}

	if (port > 0)
	{
		sprintf(conninfo,"dbname=%s user=%s password=%s port=%d",db_store, username, password, port);
		sprintf(conninfo_report,"dbname=%s user=%s password=%s port=%d",database, username, password, port);
	}
	else
	{
		sprintf(conninfo,"dbname=%s user=%s password=%s",db_store, username, password);
		sprintf(conninfo_report,"dbname=%s user=%s password=%s",database, username, password);
	}

	/* 
	 * conninfo using for _pg_pdr_
	 * and we need another conninfo using for connect to database you want to be reported
	 * this connect will search pg_class only
	 */
	
	/* Make a connection to the database */
	conn = PQconnectdb(conninfo);

	/* Check to see that the backend connection was successfully made */
	if (PQstatus(conn) != CONNECTION_OK)
	{
		fprintf(stderr, "Connection to database failed: %s",
		PQerrorMessage(conn));
		exit_nicely(conn);
	}

	//get dbid from pg_database
	sprintf(sql,"select oid from pg_database where datname='%s';",database);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s\n", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.databaseid = strtoul(PQgetvalue(res, 0, 0),NULL,10);
	else
	{
		printf("database %s doesn't exist.\n",database);
		PQclear(res);
		exit_nicely(conn);
	}
	PQclear(res);

#ifdef WIN32
	if (isdumped)
	{
		sprintf(path,"%s/%s",my_exec_path,"../share/extension/pg_pdr_dump.html");	
	}
	else 
	{
		sprintf(path,"%s/%s",my_exec_path,"../share/extension/pg_pdr.html");	
	}
#else
	if (isdumped)
	{
		sprintf(path,"%s/%s",my_exec_path,"../share/postgresql/extension/pg_pdr_dump.html");	
	}
	else 
	{
		sprintf(path,"%s/%s",my_exec_path,"../share/postgresql/extension/pg_pdr.html"); 
	}

#endif
	if ((fpin = fopen(path, "rb")) == NULL)	
	{
		fprintf(stderr, _("%s: could not open file.\n"), path);
		return false;
	}

	fseek(fpin,0L,SEEK_END);
	len = ftell(fpin);
	fseek(fpin,0L,SEEK_SET);
	if (fread(buf, 1, len, fpin) <= 0)
	{
		fclose(fpin);
		return false;
	}

	//get snaps and time
	sprintf(sql,"select min(snap_id),min(snap_ts),max(snap_id),max(snap_ts) from _pg_pdr_.pdrdb where databaseid=%u and snap_id > 0;",dbbuf.databaseid);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	for (i = 0; i < PQntuples(res); i++)
	{
		minid = strtold(PQgetvalue(res, i, 0),NULL);
		strncpy(mintime, PQgetvalue(res, i, 1), 19);
		maxid = strtold(PQgetvalue(res, i, 2),NULL);
		strncpy(maxtime, PQgetvalue(res, i, 3), 19);
	}
	PQclear(res);

	//get all snap_id and snap_ts
	iAllSnapCount = 0;
	sprintf(sql,"select snap_id,snap_ts from _pg_pdr_.pdrdb where databaseid=%u and snap_id > 0 order by snap_id limit 260000;",dbbuf.databaseid);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	for (i = 0; i < PQntuples(res); i++)
	{
		allSanp[i].snapid = strtold(PQgetvalue(res, i, 0),NULL);
		strncpy(allSanp[i].snaptime, PQgetvalue(res, i, 1), 19);
		iAllSnapCount++;
	}
	PQclear(res);
	
	if (maxid <= 0)
	{
		printf("there is not enough snap infomation,please wait more time or check your pg_pdr.naptime argument.\n");
		return false;
	}
	if (minid <= 0)
		minid = 1;

	printf("--------------------------------------------------------------------------------\n");
#ifdef WIN32
	printf_pageline = 0;
	HANDLE myHandle = GetStdHandle(STD_OUTPUT_HANDLE);

	for (i = 0; i < iAllSnapCount; i++)
	{
		if (num_of_lines >= PAGELEN)
		{
			if (printf_pageline > 1)
			{
				sprintf(line,"snap_id = %ld, snap_ts = %s\n",allSanp[i].snapid, allSanp[i].snaptime);
				num_of_lines++;
				printf_pageline--;
			}
			else
			{
				per= (int)100* num_of_lines/iAllSnapCount;
				reply = see_more_win32(per);
				if (reply == 0 )
					break;
				else if (reply == PAGELEN)
				{
					num_of_lines++;
					printf_pageline = PAGELEN;
					control_line_win32(myHandle,1);
					sprintf(line,"\nsnap_id = %ld, snap_ts = %s\n",allSanp[i].snapid, allSanp[i].snaptime);
				}
				else 
				{
					control_line_win32(myHandle,1);
					sprintf(line,"\nsnap_id = %ld, snap_ts = %s\n",allSanp[i].snapid, allSanp[i].snaptime);
					num_of_lines += reply;
				}
			}
		}
		else 
		{
			sprintf(line,"snap_id = %ld, snap_ts = %s\n",allSanp[i].snapid, allSanp[i].snaptime);
			num_of_lines++;
		}
		if (fputs(line,stdout) == EOF)
		{
			fprintf(stderr,"print error\n");
			exit_nicely(conn);

		}
	}
#else
	fp_tty = fopen("/dev/tty","r");
	if (fp_tty == NULL)
	{
		fprintf(stderr,"open file error.\n");
		exit_nicely(conn);

	}
	printf_pageline = 0;
	for (i = 0; i < iAllSnapCount; i++)
	{
		if (num_of_lines >= PAGELEN)
		{
			if (printf_pageline > 1)
			{
				sprintf(line,"snap_id =%4ld, snap_ts = %s\n",allSanp[i].snapid, allSanp[i].snaptime);
				num_of_lines++;
				printf_pageline--;
			}
			else
			{
				
				per= (int)100* num_of_lines/iAllSnapCount;
				reply =see_more(fp_tty,per);
				if (reply == 0 )
					break;
				else if (reply == PAGELEN)
				{
					num_of_lines++;
					printf_pageline = PAGELEN;
					sprintf(line,"\033[25D\033[Ksnap_id =% 4ld, snap_ts = %s\n",allSanp[i].snapid, allSanp[i].snaptime);
				}
				else 
				{
					sprintf(line,"\033[1A\033[Ksnap_id =% 4ld, snap_ts = %s\n",allSanp[i].snapid, allSanp[i].snaptime);
					num_of_lines += reply;
				}
			}
		}
		else 
		{
			sprintf(line,"snap_id =%4ld, snap_ts = %s\n",allSanp[i].snapid, allSanp[i].snaptime);
			num_of_lines++;
		}
		if (fputs(line,stdout) == EOF)
		{
			fprintf(stderr,"print error\n");
			exit_nicely(conn);

		}
		
	}
#endif

	printf("you can make report from %ld(%s) to %ld(%s)\n",minid, mintime, maxid, maxtime);
	printf("--------------------------------------------------------------------------------\n");
	
	memset(inputNum,0x00,sizeof(inputNum));
	
	while (1)
	{
		printf("please input begin snapid(from %ld):\n", minid);
		dbstflag = false;
		iScanfCount = 0;
		while (1)
		{
			if (scanf("%c",&cScanf) != 1)
			{
				/* do nothing */
			}

			if (iScanfCount > 8)
			{
				/* break; */
			}
			if (cScanf == '\n')
			{
				if (iScanfCount == 0)
					break;
				else if (iScanfCount < 9)
				{
					inputNum[iScanfCount] = 0x00;
					dbstflag = true;
					break;
				}
				else
				{
					dbstflag = true;
					break;
				}
			}
			if (iScanfCount == 0)
			{
				memset(inputNum,0x00,sizeof(inputNum));
			}
			if (iScanfCount == 9)
			{
				inputNum[iScanfCount] = 0x00;
			}
			else if (iScanfCount < 9)
			{
				inputNum[iScanfCount] = cScanf;
			}
			iScanfCount++;
		}
		if (dbstflag)
			break;
	}

	snap_begin = strtoint64(inputNum);
	if (snap_begin >= 1)
	{
		printf("snap_begin=[%ld]\n",snap_begin);
	}
	else
	{
		fprintf(stderr,"inputNum=[%s] is not a valid number\n",inputNum);
		exit_nicely(conn);
	}
	
	memset(inputNum,0x00,sizeof(inputNum));
	
	while (1)
	{
		printf("please input end snapid(up to %ld):\n", maxid);
		dbstflag = false;
		iScanfCount = 0;
		while (1)
		{
			if (scanf("%c",&cScanf) != 1)
			{
				// do nothing
			}
			if (iScanfCount > 8)
			{
				//break;
			}
			if (cScanf == '\n')
			{
				if (iScanfCount == 0)
					break;
				else if (iScanfCount < 9)
				{
					inputNum[iScanfCount] = 0x00;
					dbstflag = true;
					break;
				}
				else
				{
					dbstflag = true;
					break;
				}
			}
			if (iScanfCount == 0)
			{
				memset(inputNum,0x00,sizeof(inputNum));
			}
			if (iScanfCount == 9)
			{
				inputNum[iScanfCount] = 0x00;
			}
			else if (iScanfCount < 9)
			{
				inputNum[iScanfCount] = cScanf;
			}
			iScanfCount++;
		}
		if (dbstflag)
			break;
	}

	snap_end = strtoint64(inputNum);
	if (snap_end >= 1 && snap_end <= maxid && snap_end > snap_begin)
	{
		printf("snap_end=[%ld]\n",snap_end);
	}
	else
	{
		fprintf(stderr,"snap_end=[%s] is not a valid number\n",inputNum);
		exit_nicely(conn);
	}
	iScanfCount = snap_end;

	for (i = 0; i < iAllSnapCount; i++)
	{
		if (allSanp[i].snapid == snap_end)
			bExist_end = true;
		if (allSanp[i].snapid == snap_begin)
			bExist_begin = true;
	}
	if (!bExist_begin)
	{
		fprintf(stderr,"snap_begin=[%ld] is not in pdrdb\n",snap_begin);
		exit_nicely(conn);
	}
	if (!bExist_end)
	{
		fprintf(stderr,"snap_end=[%ld] is not in pdrdb\n",snap_end);
		exit_nicely(conn);
	}

	strcpy(dbbuf.dbname,database);

	read_pg_class(conninfo_report);
	
	//naptime
	sprintf(sql,"show pg_pdr.naptime;");
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.naptime = strtod(PQgetvalue(res, 0, 0),NULL);
	PQclear(res);

	/* when snap_begin and snap_end are not all exist in pdrsql,there must be a check*/
	/* pdrsql must exist at least 1 data at snap_begin and snap_end */
	result_count = 0;
	sprintf(sql,"select count(*) from _pg_pdr_.pdrsql where snap_id=%ld and dbid=%u;", snap_end, dbbuf.databaseid);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		result_count = strtoll(PQgetvalue(res, 0, 0),NULL,10);
	
	if (result_count < 1)
	{
		fprintf(stderr, "pdrsql snap_end data error：\nNo connection has occurred on the database or data was abnormal.\n");
		PQclear(res);
		exit_nicely(conn);
	}
	PQclear(res);
	
	result_count = 0;
	sprintf(sql,"select count(*) from _pg_pdr_.pdrsql where snap_id=%ld and dbid=%u;", snap_begin, dbbuf.databaseid);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		result_count = strtoll(PQgetvalue(res, 0, 0),NULL,10);
	
	if (result_count < 1)
	{
		fprintf(stderr, "pdrsql snap_begin data error：\nNo connection has occurred on the database or data was abnormal.\n");
		PQclear(res);
		exit_nicely(conn);
	}
	PQclear(res);

	/* pdrtable must exist at least 1 data at snap_begin and snap_end */
	result_count = 0;
	sprintf(sql,"select count(*) from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u;", snap_end, dbbuf.databaseid);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		result_count = strtoll(PQgetvalue(res, 0, 0),NULL,10);
	
	if (result_count < 1)
	{
		fprintf(stderr, "pdrtable snap_end data error：\nNo connection has occurred on the database or data was abnormal.\n");
		PQclear(res);
		exit_nicely(conn);
	}
	PQclear(res);
	
	result_count = 0;
	sprintf(sql,"select count(*) from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u;", snap_begin, dbbuf.databaseid);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		result_count = strtoll(PQgetvalue(res, 0, 0),NULL,10);
	
	if (result_count < 1)
	{
		fprintf(stderr, "pdrtable snap_begin data error：\nNo connection has occurred on the database or data was abnormal.\n");
		PQclear(res);
		exit_nicely(conn);
	}
	PQclear(res);

	/* pdrwait must exist at least 1 data at snap_begin and snap_end */
	result_count = 0;
	sprintf(sql,"select count(*) from _pg_pdr_.pdrwait where snap_id=%ld;", snap_end);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		result_count = strtoll(PQgetvalue(res, 0, 0),NULL,10);
	
	if (result_count < 1)
	{
		fprintf(stderr, "pdrwait snap_end data error：\nNo connection has occurred on the database or data was abnormal.\n");
		PQclear(res);
		exit_nicely(conn);
	}
	PQclear(res);

	result_count = 0;
	sprintf(sql,"select count(*) from _pg_pdr_.pdrwait where snap_id=%ld;", snap_begin);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		result_count = strtoll(PQgetvalue(res, 0, 0),NULL,10);
	
	if (result_count < 1)
	{
		fprintf(stderr, "pdrwait snap_begin data error：\nNo connection has occurred on the database or data was abnormal.\n");
		PQclear(res);
		exit_nicely(conn);
	}
	PQclear(res);

	/* get sum(total_time) */
	sprintf(sql,"select a.x1-b.x1,a.x2-b.x2,a.x3-b.x3,a.x4-b.x4 from (select sum(total_exec_time) x1, \
		sum(shared_blks_hit::bigint+local_blks_hit::bigint) x2,\
		sum(shared_blks_read::bigint+local_blks_read::bigint+temp_blks_read::bigint) x3, \
		sum(coalesce(blk_read_time,0)+coalesce(blk_write_time,0)) x4 \
		from _pg_pdr_.pdrsql where snap_id=%ld and dbid=%u) a,\
		(select sum(total_exec_time) x1,sum(shared_blks_hit::bigint+local_blks_hit::bigint) x2, \
		sum(shared_blks_read::bigint+local_blks_read::bigint+temp_blks_read::bigint) x3, \
		sum(coalesce(blk_read_time,0)+coalesce(blk_write_time,0)) x4 \
		from _pg_pdr_.pdrsql where snap_id=%ld and dbid=%u) b;", snap_end, dbbuf.databaseid, snap_begin, dbbuf.databaseid);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{ 
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	result_count = PQntuples(res);
	if (result_count < 1)
	{
		fprintf(stderr, "pdrsql data error：\nNo connection has occurred on the database or data was abnormal.\n");
		PQclear(res);
		exit_nicely(conn);
	}
	for (i = 0; i < result_count; i++)
	{
		dbbuf.db_totaltime = strtod(PQgetvalue(res, i, 0),NULL);
		dbbuf.db_totalgets = strtoull(PQgetvalue(res, i, 1),NULL,10);
		dbbuf.db_totalreads = strtoull(PQgetvalue(res, i, 2),NULL,10);
		dbbuf.db_totaliotime = strtoull(PQgetvalue(res, i, 3),NULL,10);
	}
	
	/* Indicator check */
	if (dbbuf.db_totaltime < 0 || dbbuf.db_totalgets < 0 || dbbuf.db_totalreads < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}
	PQclear(res);

	/* get sql_elapsed */
	sprintf(sql,"select a.queryid,a.query, \
		a.calls-coalesce(b.calls,0), \
		a.total_exec_time-coalesce(b.total_exec_time,0), \
		a.min_exec_time,a.max_exec_time, \
		case when a.calls<=coalesce(b.calls,0) then a.total_exec_time \
		else (a.total_exec_time-coalesce(b.total_exec_time,0))/(a.calls-coalesce(b.calls,0)) end, \
		a.stddev_exec_time,a.rows-coalesce(b.rows,0), \
		a.shared_blks_hit-coalesce(b.shared_blks_hit,0), \
		a.shared_blks_read-coalesce(b.shared_blks_read,0), \
		a.shared_blks_dirtied-coalesce(b.shared_blks_dirtied,0), \
		a.shared_blks_written-coalesce(b.shared_blks_written,0), \
		a.local_blks_hit-coalesce(b.local_blks_hit,0), \
		a.local_blks_read-coalesce(b.local_blks_read,0), \
		a.local_blks_dirtied-coalesce(b.local_blks_dirtied,0), \
		a.local_blks_written-coalesce(b.local_blks_written,0), \
		a.temp_blks_read-coalesce(b.temp_blks_read,0), \
		a.temp_blks_written-coalesce(b.temp_blks_written,0), \
		a.blk_read_time-coalesce(b.blk_read_time,0), \
		a.blk_write_time-coalesce(b.blk_write_time,0) \
		from \
		(select * from _pg_pdr_.pdrsql where snap_id=%ld and dbid=%u) a left join \
		(select * from _pg_pdr_.pdrsql where snap_id=%ld and dbid=%u) b on a.queryid  = b.queryid \
		order by (a.total_exec_time-coalesce(b.total_exec_time,0)) \
		desc limit %d;",
		snap_end, dbbuf.databaseid, snap_begin, dbbuf.databaseid, PGPDR_SQL_MAX);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	memset((void*)(dbbuf.sql_elapsed), 0, sizeof(dbbuf.sql_elapsed));
	result_count = PQntuples(res);
	if (result_count < 1)
	{
		fprintf(stderr, "pdrsql data error：\nNo connection has occurred on the database or data was abnormal.\n");
		PQclear(res);
		exit_nicely(conn);
	}
	for (i = 0; i < result_count; i++)
	{	
		dbbuf.sql_elapsed[i].queryid = strtoull(PQgetvalue(res, i, 0),NULL,10);
		dbbuf.sql_elapsed[i].query = PQgetvalue(res, i, 1);		
		dbbuf.sql_elapsed[i].calls = strtoull(PQgetvalue(res, i, 2),NULL,10);	
		dbbuf.sql_elapsed[i].total_time = strtod(PQgetvalue(res, i, 3),NULL);
		dbbuf.sql_elapsed[i].min_time = strtod(PQgetvalue(res, i, 4),NULL);	
		dbbuf.sql_elapsed[i].max_time = strtod(PQgetvalue(res, i, 5),NULL);	
		dbbuf.sql_elapsed[i].mean_time = strtod(PQgetvalue(res, i, 6),NULL);	

		dbbuf.sql_elapsed[i].sum_var_time = strtod(PQgetvalue(res, i, 7),NULL);
		dbbuf.sql_elapsed[i].rows = strtoull(PQgetvalue(res, i, 8),NULL,10);	

		dbbuf.sql_elapsed[i].shared_blks_hit = strtoull(PQgetvalue(res, i, 9),NULL,10);
		dbbuf.sql_elapsed[i].shared_blks_read = strtoull(PQgetvalue(res, i, 10),NULL,10);
		dbbuf.sql_elapsed[i].shared_blks_dirtied = strtoull(PQgetvalue(res, i, 11),NULL,10);
		dbbuf.sql_elapsed[i].shared_blks_written = strtoull(PQgetvalue(res, i, 12),NULL,10);

		dbbuf.sql_elapsed[i].local_blks_hit = strtoull(PQgetvalue(res, i, 13),NULL,10);
		dbbuf.sql_elapsed[i].local_blks_read = strtoull(PQgetvalue(res, i, 14),NULL,10);
		dbbuf.sql_elapsed[i].local_blks_dirtied = strtoull(PQgetvalue(res, i, 15),NULL,10);
		dbbuf.sql_elapsed[i].local_blks_written = strtoull(PQgetvalue(res, i, 16),NULL,10);

		dbbuf.sql_elapsed[i].temp_blks_read = strtoull(PQgetvalue(res, i, 17),NULL,10);
		dbbuf.sql_elapsed[i].temp_blks_written = strtoull(PQgetvalue(res, i, 18),NULL,10);	

		dbbuf.sql_elapsed[i].blk_read_time = strtod(PQgetvalue(res, i, 19),NULL);
		dbbuf.sql_elapsed[i].blk_write_time = strtod(PQgetvalue(res, i, 20),NULL);

		/* Indicator check */
		if (dbbuf.sql_elapsed[i].calls < 0 || dbbuf.sql_elapsed[i].total_time < 0 || dbbuf.sql_elapsed[i].rows < 0 )
		{
			fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
			PQclear(res);
			exit_nicely(conn);
		}

	}
	
	replace_sql_elapsed(buf);
	PQclear(res);

	/* get sql_cpu */
	sprintf(sql,"select a.queryid,a.query, \
		a.calls-coalesce(b.calls,0), \
		a.total_exec_time-coalesce(b.total_exec_time,0), \
		a.min_exec_time,a.max_exec_time, \
		case when a.calls<=coalesce(b.calls,0) then a.total_exec_time \
		else (a.total_exec_time-coalesce(b.total_exec_time,0))/(a.calls-coalesce(b.calls,0)) end, \
		a.stddev_exec_time,a.rows-coalesce(b.rows,0), \
		a.shared_blks_hit-coalesce(b.shared_blks_hit,0), \
		a.shared_blks_read-coalesce(b.shared_blks_read,0), \
		a.shared_blks_dirtied-coalesce(b.shared_blks_dirtied,0), \
		a.shared_blks_written-coalesce(b.shared_blks_written,0), \
		a.local_blks_hit-coalesce(b.local_blks_hit,0), \
		a.local_blks_read-coalesce(b.local_blks_read,0), \
		a.local_blks_dirtied-coalesce(b.local_blks_dirtied,0), \
		a.local_blks_written-coalesce(b.local_blks_written,0), \
		a.temp_blks_read-coalesce(b.temp_blks_read,0), \
		a.temp_blks_written-coalesce(b.temp_blks_written,0), \
		a.blk_read_time-coalesce(b.blk_read_time,0), \
		a.blk_write_time-coalesce(b.blk_write_time,0) \
		from \
		(select * from _pg_pdr_.pdrsql where snap_id=%ld and dbid=%u) a left join \
		(select * from _pg_pdr_.pdrsql where snap_id=%ld and dbid=%u) b on a.queryid  = b.queryid \
		order by ((a.total_exec_time-coalesce(a.blk_read_time,0)-coalesce(a.blk_write_time,0))-(coalesce(b.total_exec_time,0)-coalesce(b.blk_read_time,0)-coalesce(b.blk_write_time,0))) \
		desc limit %d;",
		snap_end, dbbuf.databaseid, snap_begin, dbbuf.databaseid, PGPDR_SQL_MAX);

	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	memset((void*)(dbbuf.sql_cpu), 0, sizeof(dbbuf.sql_cpu));
	result_count = PQntuples(res);
	if (result_count < 1)
	{
		fprintf(stderr, "pdrsql data error：\nNo connection has occurred on the database or data was abnormal.\n");
		PQclear(res);
		exit_nicely(conn);
	}
	for (i = 0; i < result_count; i++)
	{
		dbbuf.sql_cpu[i].queryid = strtoull(PQgetvalue(res, i, 0),NULL,10);
		dbbuf.sql_cpu[i].query = PQgetvalue(res, i, 1);		
		dbbuf.sql_cpu[i].calls = strtoull(PQgetvalue(res, i, 2),NULL,10);	
		dbbuf.sql_cpu[i].total_time = strtod(PQgetvalue(res, i, 3),NULL);
		dbbuf.sql_cpu[i].min_time = strtod(PQgetvalue(res, i, 4),NULL);	
		dbbuf.sql_cpu[i].max_time = strtod(PQgetvalue(res, i, 5),NULL);	
		dbbuf.sql_cpu[i].mean_time = strtod(PQgetvalue(res, i, 6),NULL);	
		dbbuf.sql_cpu[i].sum_var_time = strtod(PQgetvalue(res, i, 7),NULL);
		dbbuf.sql_cpu[i].rows = strtoull(PQgetvalue(res, i, 8),NULL,10);	
		dbbuf.sql_cpu[i].shared_blks_hit = strtoull(PQgetvalue(res, i, 9),NULL,10);
		dbbuf.sql_cpu[i].shared_blks_read = strtoull(PQgetvalue(res, i, 10),NULL,10);
		dbbuf.sql_cpu[i].shared_blks_dirtied = strtoull(PQgetvalue(res, i, 11),NULL,10);
		dbbuf.sql_cpu[i].shared_blks_written = strtoull(PQgetvalue(res, i, 12),NULL,10);
		dbbuf.sql_cpu[i].local_blks_hit = strtoull(PQgetvalue(res, i, 13),NULL,10);
		dbbuf.sql_cpu[i].local_blks_read = strtoull(PQgetvalue(res, i, 14),NULL,10);
		dbbuf.sql_cpu[i].local_blks_dirtied = strtoull(PQgetvalue(res, i, 15),NULL,10);
		dbbuf.sql_cpu[i].local_blks_written = strtoull(PQgetvalue(res, i, 16),NULL,10);
		dbbuf.sql_cpu[i].temp_blks_read = strtoull(PQgetvalue(res, i, 17),NULL,10);
		dbbuf.sql_cpu[i].temp_blks_written = strtoull(PQgetvalue(res, i, 18),NULL,10);	
		dbbuf.sql_cpu[i].blk_read_time = strtod(PQgetvalue(res, i, 19),NULL);
		dbbuf.sql_cpu[i].blk_write_time = strtod(PQgetvalue(res, i, 20),NULL);

		/* Indicator check */
		if (dbbuf.sql_cpu[i].calls < 0 || dbbuf.sql_cpu[i].total_time < 0 || dbbuf.sql_cpu[i].rows < 0 )
		{
			fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
			PQclear(res);
			exit_nicely(conn);
		}

	}
	replace_sql_cpu(buf);
	PQclear(res);

	/* get sql_io */
	sprintf(sql,"select a.queryid,a.query, \
		a.calls-coalesce(b.calls,0), \
		a.total_exec_time-coalesce(b.total_exec_time,0), \
		a.min_exec_time,a.max_exec_time, \
		case when a.calls<=coalesce(b.calls,0) then a.total_exec_time \
		else (a.total_exec_time-coalesce(b.total_exec_time,0))/(a.calls-coalesce(b.calls,0)) end, \
		a.stddev_exec_time,a.rows-coalesce(b.rows,0), \
		a.shared_blks_hit-coalesce(b.shared_blks_hit,0), \
		a.shared_blks_read-coalesce(b.shared_blks_read,0), \
		a.shared_blks_dirtied-coalesce(b.shared_blks_dirtied,0), \
		a.shared_blks_written-coalesce(b.shared_blks_written,0), \
		a.local_blks_hit-coalesce(b.local_blks_hit,0), \
		a.local_blks_read-coalesce(b.local_blks_read,0), \
		a.local_blks_dirtied-coalesce(b.local_blks_dirtied,0), \
		a.local_blks_written-coalesce(b.local_blks_written,0), \
		a.temp_blks_read-coalesce(b.temp_blks_read,0), \
		a.temp_blks_written-coalesce(b.temp_blks_written,0), \
		a.blk_read_time-coalesce(b.blk_read_time,0), \
		a.blk_write_time-coalesce(b.blk_write_time,0) \
		from \
		(select * from _pg_pdr_.pdrsql where snap_id=%ld and dbid=%u) a left join \
		(select * from _pg_pdr_.pdrsql where snap_id=%ld and dbid=%u) b on a.queryid  = b.queryid \
		order by ((coalesce(a.blk_read_time,0)+coalesce(a.blk_write_time,0))-(coalesce(b.blk_read_time,0)+coalesce(b.blk_write_time,0))) \
		desc limit %d;",
		snap_end, dbbuf.databaseid, snap_begin, dbbuf.databaseid, PGPDR_SQL_MAX);

	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	memset((void*)(dbbuf.sql_io), 0, sizeof(dbbuf.sql_io));
	result_count = PQntuples(res);
	if (result_count < 1)
	{
		fprintf(stderr, "pdrsql data error：\nNo connection has occurred on the database or data was abnormal.\n");
		PQclear(res);
		exit_nicely(conn);
	}
	for (i = 0; i < result_count; i++)
	{
		dbbuf.sql_io[i].queryid = strtoull(PQgetvalue(res, i, 0),NULL,10);
		dbbuf.sql_io[i].query = PQgetvalue(res, i, 1);		
		dbbuf.sql_io[i].calls = strtoull(PQgetvalue(res, i, 2),NULL,10);	
		dbbuf.sql_io[i].total_time = strtod(PQgetvalue(res, i, 3),NULL);
		dbbuf.sql_io[i].min_time = strtod(PQgetvalue(res, i, 4),NULL);	
		dbbuf.sql_io[i].max_time = strtod(PQgetvalue(res, i, 5),NULL);	
		dbbuf.sql_io[i].mean_time = strtod(PQgetvalue(res, i, 6),NULL);	
		dbbuf.sql_io[i].sum_var_time = strtod(PQgetvalue(res, i, 7),NULL);
		dbbuf.sql_io[i].rows = strtoull(PQgetvalue(res, i, 8),NULL,10);	
		dbbuf.sql_io[i].shared_blks_hit = strtoull(PQgetvalue(res, i, 9),NULL,10);
		dbbuf.sql_io[i].shared_blks_read = strtoull(PQgetvalue(res, i, 10),NULL,10);
		dbbuf.sql_io[i].shared_blks_dirtied = strtoull(PQgetvalue(res, i, 11),NULL,10);
		dbbuf.sql_io[i].shared_blks_written = strtoull(PQgetvalue(res, i, 12),NULL,10);
		dbbuf.sql_io[i].local_blks_hit = strtoull(PQgetvalue(res, i, 13),NULL,10);
		dbbuf.sql_io[i].local_blks_read = strtoull(PQgetvalue(res, i, 14),NULL,10);
		dbbuf.sql_io[i].local_blks_dirtied = strtoull(PQgetvalue(res, i, 15),NULL,10);
		dbbuf.sql_io[i].local_blks_written = strtoull(PQgetvalue(res, i, 16),NULL,10);
		dbbuf.sql_io[i].temp_blks_read = strtoull(PQgetvalue(res, i, 17),NULL,10);
		dbbuf.sql_io[i].temp_blks_written = strtoull(PQgetvalue(res, i, 18),NULL,10);	
		dbbuf.sql_io[i].blk_read_time = strtod(PQgetvalue(res, i, 19),NULL);
		dbbuf.sql_io[i].blk_write_time = strtod(PQgetvalue(res, i, 20),NULL);

		/* Indicator check */
		if (dbbuf.sql_io[i].calls < 0 || dbbuf.sql_io[i].total_time < 0 || dbbuf.sql_io[i].rows < 0 )
		{
			fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
			PQclear(res);
			exit_nicely(conn);
		}

	}
	replace_sql_io(buf);
	PQclear(res);

	/* get sql_gets */
	sprintf(sql,"select a.queryid,a.query, \
		a.calls-coalesce(b.calls,0), \
		a.total_exec_time-coalesce(b.total_exec_time,0), \
		a.min_exec_time,a.max_exec_time, \
		case when a.calls<=coalesce(b.calls,0) then a.total_exec_time \
		else (a.total_exec_time-coalesce(b.total_exec_time,0))/(a.calls-coalesce(b.calls,0)) end, \
		a.stddev_exec_time,a.rows-coalesce(b.rows,0), \
		a.shared_blks_hit-coalesce(b.shared_blks_hit,0), \
		a.shared_blks_read-coalesce(b.shared_blks_read,0), \
		a.shared_blks_dirtied-coalesce(b.shared_blks_dirtied,0), \
		a.shared_blks_written-coalesce(b.shared_blks_written,0), \
		a.local_blks_hit-coalesce(b.local_blks_hit,0), \
		a.local_blks_read-coalesce(b.local_blks_read,0), \
		a.local_blks_dirtied-coalesce(b.local_blks_dirtied,0), \
		a.local_blks_written-coalesce(b.local_blks_written,0), \
		a.temp_blks_read-coalesce(b.temp_blks_read,0), \
		a.temp_blks_written-coalesce(b.temp_blks_written,0), \
		a.blk_read_time-coalesce(b.blk_read_time,0), \
		a.blk_write_time-coalesce(b.blk_write_time,0) \
		from \
		(select * from _pg_pdr_.pdrsql where snap_id=%ld and dbid=%u) a left join \
		(select * from _pg_pdr_.pdrsql where snap_id=%ld and dbid=%u) b on a.queryid  = b.queryid \
		order by ((coalesce(a.shared_blks_hit,0)+coalesce(a.local_blks_hit,0))-(coalesce(b.shared_blks_hit,0)+coalesce(b.local_blks_hit,0)))  \
		desc limit %d;",
		snap_end, dbbuf.databaseid, snap_begin, dbbuf.databaseid, PGPDR_SQL_MAX);

	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	memset((void*)(dbbuf.sql_gets), 0, sizeof(dbbuf.sql_gets));
	result_count = PQntuples(res);
	if (result_count < 1)
	{
		fprintf(stderr, "pdrsql data error：\nNo connection has occurred on the database or data was abnormal.\n");
		PQclear(res);
		exit_nicely(conn);
	}
	for (i = 0; i < result_count; i++)
	{
		dbbuf.sql_gets[i].queryid = strtoull(PQgetvalue(res, i, 0),NULL,10);
		dbbuf.sql_gets[i].query = PQgetvalue(res, i, 1);		
		dbbuf.sql_gets[i].calls = strtoull(PQgetvalue(res, i, 2),NULL,10);	
		dbbuf.sql_gets[i].total_time = strtod(PQgetvalue(res, i, 3),NULL);
		dbbuf.sql_gets[i].min_time = strtod(PQgetvalue(res, i, 4),NULL);	
		dbbuf.sql_gets[i].max_time = strtod(PQgetvalue(res, i, 5),NULL);	
		dbbuf.sql_gets[i].mean_time = strtod(PQgetvalue(res, i, 6),NULL);	
		dbbuf.sql_gets[i].sum_var_time = strtod(PQgetvalue(res, i, 7),NULL);
		dbbuf.sql_gets[i].rows = strtoull(PQgetvalue(res, i, 8),NULL,10);	
		dbbuf.sql_gets[i].shared_blks_hit = strtoull(PQgetvalue(res, i, 9),NULL,10);
		dbbuf.sql_gets[i].shared_blks_read = strtoull(PQgetvalue(res, i, 10),NULL,10);
		dbbuf.sql_gets[i].shared_blks_dirtied = strtoull(PQgetvalue(res, i, 11),NULL,10);
		dbbuf.sql_gets[i].shared_blks_written = strtoull(PQgetvalue(res, i, 12),NULL,10);
		dbbuf.sql_gets[i].local_blks_hit = strtoull(PQgetvalue(res, i, 13),NULL,10);
		dbbuf.sql_gets[i].local_blks_read = strtoull(PQgetvalue(res, i, 14),NULL,10);
		dbbuf.sql_gets[i].local_blks_dirtied = strtoull(PQgetvalue(res, i, 15),NULL,10);
		dbbuf.sql_gets[i].local_blks_written = strtoull(PQgetvalue(res, i, 16),NULL,10);
		dbbuf.sql_gets[i].temp_blks_read = strtoull(PQgetvalue(res, i, 17),NULL,10);
		dbbuf.sql_gets[i].temp_blks_written = strtoull(PQgetvalue(res, i, 18),NULL,10);	
		dbbuf.sql_gets[i].blk_read_time = strtod(PQgetvalue(res, i, 19),NULL);
		dbbuf.sql_gets[i].blk_write_time = strtod(PQgetvalue(res, i, 20),NULL);

		/* Indicator check */
		if (dbbuf.sql_gets[i].calls < 0 || dbbuf.sql_gets[i].total_time < 0 || dbbuf.sql_gets[i].rows < 0 )
		{
			fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
			PQclear(res);
			exit_nicely(conn);
		}

	}
	replace_sql_gets(buf);
	PQclear(res);


	/* get sql_reads */
	sprintf(sql,"select a.queryid,a.query, \
		a.calls-coalesce(b.calls,0), \
		a.total_exec_time-coalesce(b.total_exec_time,0), \
		a.min_exec_time,a.max_exec_time, \
		case when a.calls<=coalesce(b.calls,0) then a.total_exec_time \
		else (a.total_exec_time-coalesce(b.total_exec_time,0))/(a.calls-coalesce(b.calls,0)) end, \
		a.stddev_exec_time,a.rows-coalesce(b.rows,0), \
		a.shared_blks_hit-coalesce(b.shared_blks_hit,0), \
		a.shared_blks_read-coalesce(b.shared_blks_read,0), \
		a.shared_blks_dirtied-coalesce(b.shared_blks_dirtied,0), \
		a.shared_blks_written-coalesce(b.shared_blks_written,0), \
		a.local_blks_hit-coalesce(b.local_blks_hit,0), \
		a.local_blks_read-coalesce(b.local_blks_read,0), \
		a.local_blks_dirtied-coalesce(b.local_blks_dirtied,0), \
		a.local_blks_written-coalesce(b.local_blks_written,0), \
		a.temp_blks_read-coalesce(b.temp_blks_read,0), \
		a.temp_blks_written-coalesce(b.temp_blks_written,0), \
		a.blk_read_time-coalesce(b.blk_read_time,0), \
		a.blk_write_time-coalesce(b.blk_write_time,0) \
		from \
		(select * from _pg_pdr_.pdrsql where snap_id=%ld and dbid=%u) a left join \
		(select * from _pg_pdr_.pdrsql where snap_id=%ld and dbid=%u) b on a.queryid  = b.queryid \
		order by ((coalesce(a.shared_blks_read,0)+coalesce(a.local_blks_read,0)+coalesce(a.temp_blks_read,0))-(coalesce(b.shared_blks_read,0)+coalesce(b.local_blks_read,0)+coalesce(b.temp_blks_read,0)))  \
		desc limit %d;",
		snap_end, dbbuf.databaseid, snap_begin, dbbuf.databaseid, PGPDR_SQL_MAX);

	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	memset((void*)(dbbuf.sql_reads), 0, sizeof(dbbuf.sql_reads));
	result_count = PQntuples(res);
	if (result_count < 1)
	{
		fprintf(stderr, "pdrsql data error：\nNo connection has occurred on the database or data was abnormal.\n");
		PQclear(res);
		exit_nicely(conn);
	}
	for (i = 0; i < result_count; i++)
	{
		dbbuf.sql_reads[i].queryid = strtoull(PQgetvalue(res, i, 0),NULL,10);
		dbbuf.sql_reads[i].query = PQgetvalue(res, i, 1);		
		dbbuf.sql_reads[i].calls = strtoull(PQgetvalue(res, i, 2),NULL,10);	
		dbbuf.sql_reads[i].total_time = strtod(PQgetvalue(res, i, 3),NULL);
		dbbuf.sql_reads[i].min_time = strtod(PQgetvalue(res, i, 4),NULL);	
		dbbuf.sql_reads[i].max_time = strtod(PQgetvalue(res, i, 5),NULL);	
		dbbuf.sql_reads[i].mean_time = strtod(PQgetvalue(res, i, 6),NULL);	
		dbbuf.sql_reads[i].sum_var_time = strtod(PQgetvalue(res, i, 7),NULL);
		dbbuf.sql_reads[i].rows = strtoull(PQgetvalue(res, i, 8),NULL,10);	
		dbbuf.sql_reads[i].shared_blks_hit = strtoull(PQgetvalue(res, i, 9),NULL,10);
		dbbuf.sql_reads[i].shared_blks_read = strtoull(PQgetvalue(res, i, 10),NULL,10);
		dbbuf.sql_reads[i].shared_blks_dirtied = strtoull(PQgetvalue(res, i, 11),NULL,10);
		dbbuf.sql_reads[i].shared_blks_written = strtoull(PQgetvalue(res, i, 12),NULL,10);
		dbbuf.sql_reads[i].local_blks_hit = strtoull(PQgetvalue(res, i, 13),NULL,10);
		dbbuf.sql_reads[i].local_blks_read = strtoull(PQgetvalue(res, i, 14),NULL,10);
		dbbuf.sql_reads[i].local_blks_dirtied = strtoull(PQgetvalue(res, i, 15),NULL,10);
		dbbuf.sql_reads[i].local_blks_written = strtoull(PQgetvalue(res, i, 16),NULL,10);
		dbbuf.sql_reads[i].temp_blks_read = strtoull(PQgetvalue(res, i, 17),NULL,10);
		dbbuf.sql_reads[i].temp_blks_written = strtoull(PQgetvalue(res, i, 18),NULL,10);	
		dbbuf.sql_reads[i].blk_read_time = strtod(PQgetvalue(res, i, 19),NULL);
		dbbuf.sql_reads[i].blk_write_time = strtod(PQgetvalue(res, i, 20),NULL);

		/* Indicator check */
		if (dbbuf.sql_reads[i].calls < 0 || dbbuf.sql_reads[i].total_time < 0 || dbbuf.sql_reads[i].rows < 0 )
		{
			fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
			PQclear(res);
			exit_nicely(conn);
		}

	}
	replace_sql_reads(buf);
	PQclear(res);


	/* get sql_exec */
	sprintf(sql,"select a.queryid,a.query, \
		a.calls-coalesce(b.calls,0), \
		a.total_exec_time-coalesce(b.total_exec_time,0), \
		a.min_exec_time,a.max_exec_time, \
		case when a.calls<=coalesce(b.calls,0) then a.total_exec_time \
		else (a.total_exec_time-coalesce(b.total_exec_time,0))/(a.calls-coalesce(b.calls,0)) end, \
		a.stddev_exec_time,a.rows-coalesce(b.rows,0), \
		a.shared_blks_hit-coalesce(b.shared_blks_hit,0), \
		a.shared_blks_read-coalesce(b.shared_blks_read,0), \
		a.shared_blks_dirtied-coalesce(b.shared_blks_dirtied,0), \
		a.shared_blks_written-coalesce(b.shared_blks_written,0), \
		a.local_blks_hit-coalesce(b.local_blks_hit,0), \
		a.local_blks_read-coalesce(b.local_blks_read,0), \
		a.local_blks_dirtied-coalesce(b.local_blks_dirtied,0), \
		a.local_blks_written-coalesce(b.local_blks_written,0), \
		a.temp_blks_read-coalesce(b.temp_blks_read,0), \
		a.temp_blks_written-coalesce(b.temp_blks_written,0), \
		a.blk_read_time-coalesce(b.blk_read_time,0), \
		a.blk_write_time-coalesce(b.blk_write_time,0) \
		from \
		(select * from _pg_pdr_.pdrsql where snap_id=%ld and dbid=%u) a left join \
		(select * from _pg_pdr_.pdrsql where snap_id=%ld and dbid=%u) b on a.queryid  = b.queryid \
		order by ((coalesce(a.calls,0))-(coalesce(b.calls,0)))   \
		desc limit %d;",
		snap_end, dbbuf.databaseid, snap_begin, dbbuf.databaseid, PGPDR_SQL_MAX);

	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	memset((void*)(dbbuf.sql_exec), 0, sizeof(dbbuf.sql_exec));
	result_count = PQntuples(res);
	if (result_count < 1)
	{
		fprintf(stderr, "pdrsql data error：\nNo connection has occurred on the database or data was abnormal.\n");
		PQclear(res);
		exit_nicely(conn);
	}
	for (i = 0; i < result_count; i++)
	{
		dbbuf.sql_exec[i].queryid = strtoull(PQgetvalue(res, i, 0),NULL,10);
		dbbuf.sql_exec[i].query = PQgetvalue(res, i, 1);		
		dbbuf.sql_exec[i].calls = strtoull(PQgetvalue(res, i, 2),NULL,10);	
		dbbuf.sql_exec[i].total_time = strtod(PQgetvalue(res, i, 3),NULL);
		dbbuf.sql_exec[i].min_time = strtod(PQgetvalue(res, i, 4),NULL);	
		dbbuf.sql_exec[i].max_time = strtod(PQgetvalue(res, i, 5),NULL);	
		dbbuf.sql_exec[i].mean_time = strtod(PQgetvalue(res, i, 6),NULL);	
		dbbuf.sql_exec[i].sum_var_time = strtod(PQgetvalue(res, i, 7),NULL);
		dbbuf.sql_exec[i].rows = strtoull(PQgetvalue(res, i, 8),NULL,10);	
		dbbuf.sql_exec[i].shared_blks_hit = strtoull(PQgetvalue(res, i, 9),NULL,10);
		dbbuf.sql_exec[i].shared_blks_read = strtoull(PQgetvalue(res, i, 10),NULL,10);
		dbbuf.sql_exec[i].shared_blks_dirtied = strtoull(PQgetvalue(res, i, 11),NULL,10);
		dbbuf.sql_exec[i].shared_blks_written = strtoull(PQgetvalue(res, i, 12),NULL,10);
		dbbuf.sql_exec[i].local_blks_hit = strtoull(PQgetvalue(res, i, 13),NULL,10);
		dbbuf.sql_exec[i].local_blks_read = strtoull(PQgetvalue(res, i, 14),NULL,10);
		dbbuf.sql_exec[i].local_blks_dirtied = strtoull(PQgetvalue(res, i, 15),NULL,10);
		dbbuf.sql_exec[i].local_blks_written = strtoull(PQgetvalue(res, i, 16),NULL,10);
		dbbuf.sql_exec[i].temp_blks_read = strtoull(PQgetvalue(res, i, 17),NULL,10);
		dbbuf.sql_exec[i].temp_blks_written = strtoull(PQgetvalue(res, i, 18),NULL,10);	
		dbbuf.sql_exec[i].blk_read_time = strtod(PQgetvalue(res, i, 19),NULL);
		dbbuf.sql_exec[i].blk_write_time = strtod(PQgetvalue(res, i, 20),NULL);

		/* Indicator check */
		if (dbbuf.sql_exec[i].calls < 0 || dbbuf.sql_exec[i].total_time < 0 || dbbuf.sql_exec[i].rows < 0 )
		{
			fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
			PQclear(res);
			exit_nicely(conn);
		}

	}
	replace_sql_exec(buf);	
	replace_sql_text(buf);
	PQclear(res);


	/* get seg_logical */
	/* get total logical reads,total physical reads,total table scans */
	sprintf(sql,"select a.x1-b.x1,a.x2-b.x2,a.x3-b.x3 from (select sum(blocks_fetched) x1,sum(blocks_fetched-blocks_hit) x2,\
		sum(numscans) x3 from _pg_pdr_.pdrtable where databaseid=%u and snap_id=%ld) as a,\
		(select sum(blocks_fetched) x1,sum(blocks_fetched-blocks_hit) x2,sum(numscans) x3 from _pg_pdr_.pdrtable where databaseid=%u\
		 and snap_id=%ld) as b;", 
		 dbbuf.databaseid, snap_end, dbbuf.databaseid, snap_begin);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	result_count = PQntuples(res);
	if (result_count < 1)
	{
		fprintf(stderr, "pdrtable data error：\nNo connection has occurred on the database or data was abnormal.\n");
		PQclear(res);
		exit_nicely(conn);
	}
	for (i = 0; i < result_count; i++)
	{
		dbbuf.db_totallogicalreads = strtoull(PQgetvalue(res, i, 0),NULL,10);
		dbbuf.db_totalphysicalreads = strtoull(PQgetvalue(res, i, 1),NULL,10);
		dbbuf.db_totaltablescans = strtoull(PQgetvalue(res, i, 2),NULL,10);
	}

	/* Indicator check */
	if (dbbuf.db_totallogicalreads < 0 || dbbuf.db_totalphysicalreads < 0 || dbbuf.db_totaltablescans < 0 )
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	/* Segments by Logical Reads */
	sprintf(sql,"select databaseid,tableid,max(blocks_fetched)-min(blocks_fetched) \
	 		from _pg_pdr_.pdrtable where snap_id >=%ld and snap_id <= %ld \
	 		and databaseid=%u \
	 		group by databaseid,tableid order by max(blocks_fetched)-min(blocks_fetched) desc limit %d;",
	 		snap_begin,snap_end,dbbuf.databaseid,PGPDR_SQL_MAX);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	memset((void*)(dbbuf.seg_logical_reads), 0, sizeof(dbbuf.seg_logical_reads));
	for (i = 0; i < PQntuples(res); i++)
	{
		dbbuf.seg_logical_reads[i].dbid = strtoull(PQgetvalue(res, i, 0),NULL,10);
		dbbuf.seg_logical_reads[i].tbid = strtoull(PQgetvalue(res, i, 1),NULL,10);
		dbbuf.seg_logical_reads[i].reads = strtoull(PQgetvalue(res, i, 2),NULL,10);
	}
	if (!isdumped)
		replace_seg_logical_reads(buf);	
	PQclear(res);

	/* Segments by Physical Reads */
	sprintf(sql,"select databaseid,tableid,max(blocks_fetched-blocks_hit)-min(blocks_fetched-blocks_hit) \
		from _pg_pdr_.pdrtable where \
		snap_id >=%ld and snap_id <= %ld and databaseid=%u \
		group by databaseid,tableid order by max(blocks_fetched-blocks_hit)-min(blocks_fetched-blocks_hit) \
		desc limit %d;",
	 	snap_begin,snap_end,dbbuf.databaseid,PGPDR_SQL_MAX);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	memset((void*)(dbbuf.seg_logical_reads), 0, sizeof(dbbuf.seg_logical_reads));
	for (i = 0; i < PQntuples(res); i++)
	{
		dbbuf.seg_physical_reads[i].dbid = strtoull(PQgetvalue(res, i, 0),NULL,10);
		dbbuf.seg_physical_reads[i].tbid = strtoull(PQgetvalue(res, i, 1),NULL,10);
		dbbuf.seg_physical_reads[i].reads = strtoull(PQgetvalue(res, i, 2),NULL,10);
	}
	if (!isdumped)
		replace_seg_physical_reads(buf);
	PQclear(res);

	/* Segments by Table Scans */
	sprintf(sql,"select databaseid,tableid,max(numscans)-min(numscans) \
	 		from _pg_pdr_.pdrtable where snap_id >=%ld and snap_id <= %ld \
	 		and databaseid=%u \
	 		group by databaseid,tableid order by max(numscans)-min(numscans) desc limit %d;",
	 		snap_begin,snap_end,dbbuf.databaseid,PGPDR_SQL_MAX);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	memset((void*)(dbbuf.seg_table_scans), 0, sizeof(dbbuf.seg_table_scans));
	for (i = 0; i < PQntuples(res); i++)
	{
		dbbuf.seg_table_scans[i].dbid = strtoull(PQgetvalue(res, i, 0),NULL,10);
		dbbuf.seg_table_scans[i].tbid = strtoull(PQgetvalue(res, i, 1),NULL,10);
		dbbuf.seg_table_scans[i].reads = strtoull(PQgetvalue(res, i, 2),NULL,10);
	}
	if (!isdumped)
		replace_seg_table_scans(buf);
	PQclear(res);


	/* IO Stats  */
	//IOStat by Function summary
	//node_read_data------2
	sprintf(sql,"select a.x1-b.x1 from (select sum(shared_blks_read::bigint+local_blks_read::bigint+temp_blks_read::bigint) x1 \
		from _pg_pdr_.pdrsql where snap_id=%ld and dbid=%u) a,\
		(select sum(shared_blks_read::bigint+local_blks_read::bigint+temp_blks_read::bigint) x1 from _pg_pdr_.pdrsql where snap_id=%ld \
		and dbid=%u) b;",
		snap_end, dbbuf.databaseid, snap_begin, dbbuf.databaseid);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	memset((void*)(&dbbuf.io_stats), 0, sizeof(dbbuf.io_stats));
	if (PQntuples(res))
		dbbuf.io_stats.node_read_data = strtoull(PQgetvalue(res, 0, 0),NULL,10);

	/* Indicator check */
	if (dbbuf.io_stats.node_read_data < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//node_write_data,db_write_data,buf_write_data-----3
	sprintf(sql,"select a.x1-b.x1 from (select sum(shared_blks_written::bigint+local_blks_written::bigint+temp_blks_written::bigint) x1 \
		from _pg_pdr_.pdrsql where snap_id=%ld and dbid=%u) a,\
		(select sum(shared_blks_written::bigint+local_blks_written::bigint+temp_blks_written::bigint) x1 from _pg_pdr_.pdrsql where snap_id=%ld \
		and dbid=%u) b;",
		snap_end, dbbuf.databaseid, snap_begin, dbbuf.databaseid);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
	{
		dbbuf.io_stats.node_write_data = strtoull(PQgetvalue(res, 0, 0),NULL,10);
		
		dbbuf.io_stats.buf_write_data = dbbuf.io_stats.node_write_data;
	}
	/* Indicator check */
	if (dbbuf.io_stats.node_write_data < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//node_write_reqs,db_write_reqs,buf_write_reqs-----14
	sprintf(sql,"select a.x1-b.x1 from (select sum(shared_blks_dirtied::bigint+local_blks_dirtied::bigint+temp_blks_written::bigint) x1 \
		from _pg_pdr_.pdrsql where snap_id=%ld and dbid=%u) a,\
		(select sum(shared_blks_dirtied::bigint+local_blks_dirtied::bigint+temp_blks_written::bigint) x1 from _pg_pdr_.pdrsql where snap_id=%ld \
		and dbid=%u) b;",
		snap_end, dbbuf.databaseid, snap_begin, dbbuf.databaseid);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
	{
		dbbuf.io_stats.node_write_reqs = strtoull(PQgetvalue(res, 0, 0),NULL,10);
		
		dbbuf.io_stats.buf_write_reqs = dbbuf.io_stats.node_write_reqs;
	}
	/* Indicator check */
	if (dbbuf.io_stats.node_write_reqs < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}
	PQclear(res);

	//buf_read_data---1
	sprintf(sql,"select a.x1-b.x1 from (select sum(shared_blks_hit::bigint+local_blks_hit::bigint) x1 \
		from _pg_pdr_.pdrsql where snap_id=%ld and dbid=%u) a,\
		(select sum(shared_blks_hit::bigint+local_blks_hit::bigint) x1 from _pg_pdr_.pdrsql where snap_id=%ld \
		and dbid=%u) b;",
		snap_end, dbbuf.databaseid, snap_begin, dbbuf.databaseid);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.buf_read_data = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.buf_read_data < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	/* IOStat by Reltype summary */
	//catalog_read_data---5
	sprintf(sql,"select a.x1-b.x1 from (select sum(shared_blks_hit::bigint+shared_blks_read::bigint) x1 \
		from _pg_pdr_.pdrsql where snap_id=%ld and dbid=%u) a,\
		(select sum(shared_blks_hit::bigint+shared_blks_read::bigint) x1 from _pg_pdr_.pdrsql where snap_id=%ld \
		and dbid=%u) b;",
		snap_end, dbbuf.databaseid, snap_begin, dbbuf.databaseid);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.catalog_read_data = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.catalog_read_data < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//catalog_write_data---6
	sprintf(sql,"select a.x1-b.x1 from (select sum(shared_blks_written::bigint) x1 \
		from _pg_pdr_.pdrsql where snap_id=%ld and dbid=%u) a,\
		(select sum(shared_blks_written::bigint) x1 from _pg_pdr_.pdrsql where snap_id=%ld \
		and dbid=%u) b;",
		snap_end, dbbuf.databaseid, snap_begin, dbbuf.databaseid);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.catalog_write_data = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.catalog_write_data < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//rel_read_data---7
	sprintf(sql,"select a.x1-b.x1 from (select sum(local_blks_hit::bigint+local_blks_read::bigint) x1 \
		from _pg_pdr_.pdrsql where snap_id=%ld and dbid=%u) a,\
		(select sum(local_blks_hit::bigint+local_blks_read::bigint) x1 from _pg_pdr_.pdrsql where snap_id=%ld \
		and dbid=%u) b;",
		snap_end, dbbuf.databaseid, snap_begin, dbbuf.databaseid);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.rel_read_data = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.rel_read_data < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//rel_write_data---8
	sprintf(sql,"select a.x1-b.x1 from (select sum(local_blks_written::bigint) x1 \
		from _pg_pdr_.pdrsql where snap_id=%ld and dbid=%u) a,\
		(select sum(local_blks_written::bigint) x1 from _pg_pdr_.pdrsql where snap_id=%ld \
		and dbid=%u) b;",
		snap_end, dbbuf.databaseid, snap_begin, dbbuf.databaseid);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.rel_write_data = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.rel_write_data < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//temp_read_data---9
	sprintf(sql,"select a.x1-b.x1 from (select sum(temp_blks_read::bigint) x1 \
		from _pg_pdr_.pdrsql where snap_id=%ld and dbid=%u) a,\
		(select sum(temp_blks_read::bigint) x1 from _pg_pdr_.pdrsql where snap_id=%ld \
		and dbid=%u) b;",
		snap_end, dbbuf.databaseid, snap_begin, dbbuf.databaseid);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.temp_read_data = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.temp_read_data < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//temp_write_data---10
	sprintf(sql,"select a.x1-b.x1 from (select sum(temp_blks_written::bigint) x1 \
		from _pg_pdr_.pdrsql where snap_id=%ld and dbid=%u) a,\
		(select sum(temp_blks_written::bigint) x1 from _pg_pdr_.pdrsql where snap_id=%ld \
		and dbid=%u) b;",
		snap_end, dbbuf.databaseid, snap_begin, dbbuf.databaseid);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.temp_write_data = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.temp_write_data < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//catalog_write_reqs---11
	sprintf(sql,"select a.x1-b.x1 from (select sum(shared_blks_dirtied::bigint) x1 \
		from _pg_pdr_.pdrsql where snap_id=%ld and dbid=%u) a,\
		(select sum(shared_blks_dirtied::bigint) x1 from _pg_pdr_.pdrsql where snap_id=%ld \
		and dbid=%u) b;",
		snap_end, dbbuf.databaseid, snap_begin, dbbuf.databaseid);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.catalog_write_reqs = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.catalog_write_reqs < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//rel_write_reqs---12
	sprintf(sql,"select a.x1-b.x1 from (select sum(local_blks_dirtied::bigint) x1 \
		from _pg_pdr_.pdrsql where snap_id=%ld and dbid=%u) a,\
		(select sum(local_blks_dirtied::bigint) x1 from _pg_pdr_.pdrsql where snap_id=%ld \
		and dbid=%u) b;",
		snap_end, dbbuf.databaseid, snap_begin, dbbuf.databaseid);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.rel_write_reqs = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.rel_write_reqs < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//temp_write_reqs---13
	sprintf(sql,"select a.x1-b.x1 from (select sum(temp_blks_written::bigint) x1 \
		from _pg_pdr_.pdrsql where snap_id=%ld and dbid=%u) a,\
		(select sum(temp_blks_written::bigint) x1 from _pg_pdr_.pdrsql where snap_id=%ld \
		and dbid=%u) b;",
		snap_end, dbbuf.databaseid, snap_begin, dbbuf.databaseid);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.temp_write_reqs = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.temp_write_reqs < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	// io_rel_read 
	memset(oids,0,sizeof(oids));
	oid2str(oids,'r');
	sprintf(sql,"select a.x1-b.x1 from \
		(select sum(blocks_fetched) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) a,\
		(select sum(blocks_fetched) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) b;",
		snap_end, dbbuf.databaseid, oids, snap_begin, dbbuf.databaseid, oids);	
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.io_rel_read = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.io_rel_read < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//io_rel_buf
	sprintf(sql,"select a.x1-b.x1 from \
		(select sum(blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) a,\
		(select sum(blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) b;",
		snap_end, dbbuf.databaseid, oids, snap_begin, dbbuf.databaseid, oids);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.io_rel_buf = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.io_rel_buf < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//io_rel_disk
	sprintf(sql,"select a.x1-b.x1 from \
		(select sum(blocks_fetched-blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) a,\
		(select sum(blocks_fetched-blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) b;",
		snap_end, dbbuf.databaseid, oids, snap_begin, dbbuf.databaseid, oids);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.io_rel_disk = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.io_rel_disk < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//io_index_read
	memset(oids,0,sizeof(oids));
	oid2str(oids,'i');
	sprintf(sql,"select a.x1-b.x1 from \
		(select sum(blocks_fetched) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) a,\
		(select sum(blocks_fetched) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) b;",
		snap_end, dbbuf.databaseid, oids, snap_begin, dbbuf.databaseid, oids);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.io_index_read = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.io_index_read < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//io_index_buf
	sprintf(sql,"select a.x1-b.x1 from \
		(select sum(blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) a,\
		(select sum(blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) b;",
		snap_end, dbbuf.databaseid, oids, snap_begin, dbbuf.databaseid, oids);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.io_index_buf = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.io_index_buf < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//io_index_disk
	sprintf(sql,"select a.x1-b.x1 from \
		(select sum(blocks_fetched-blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) a,\
		(select sum(blocks_fetched-blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) b;",
		snap_end, dbbuf.databaseid, oids, snap_begin, dbbuf.databaseid, oids);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.io_index_disk = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.io_index_disk < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//io_seq_read
	memset(oids,0,sizeof(oids));
	oid2str(oids,'s');
	sprintf(sql,"select a.x1-b.x1 from \
		(select sum(blocks_fetched) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) a,\
		(select sum(blocks_fetched) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) b;",
		snap_end, dbbuf.databaseid, oids, snap_begin, dbbuf.databaseid, oids);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.io_seq_read = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.io_seq_read < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//io_seq_buf
	sprintf(sql,"select a.x1-b.x1 from \
		(select sum(blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) a,\
		(select sum(blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) b;",
		snap_end, dbbuf.databaseid, oids, snap_begin, dbbuf.databaseid, oids);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.io_seq_buf = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.io_seq_buf < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//io_seq_disk
	sprintf(sql,"select a.x1-b.x1 from \
		(select sum(blocks_fetched-blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) a,\
		(select sum(blocks_fetched-blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) b;",
		snap_end, dbbuf.databaseid, oids, snap_begin, dbbuf.databaseid, oids);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.io_seq_disk = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.io_seq_disk < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//io_view_read
	memset(oids,0,sizeof(oids));
	oid2str(oids,'v');
	sprintf(sql,"select a.x1-b.x1 from \
		(select sum(blocks_fetched) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) a,\
		(select sum(blocks_fetched) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) b;",
		snap_end, dbbuf.databaseid, oids, snap_begin, dbbuf.databaseid, oids);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.io_view_read = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.io_view_read < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//io_view_buf
	sprintf(sql,"select a.x1-b.x1 from \
		(select sum(blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) a,\
		(select sum(blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) b;",
		snap_end, dbbuf.databaseid, oids, snap_begin, dbbuf.databaseid, oids);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.io_view_buf = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.io_view_buf < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//io_view_disk
	sprintf(sql,"select a.x1-b.x1 from \
		(select sum(blocks_fetched-blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) a,\
		(select sum(blocks_fetched-blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) b;",
		snap_end, dbbuf.databaseid, oids, snap_begin, dbbuf.databaseid, oids);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.io_view_disk = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.io_view_disk < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//io_mv_read
	memset(oids,0,sizeof(oids));
	oid2str(oids,'m');
	sprintf(sql,"select a.x1-b.x1 from \
		(select sum(blocks_fetched) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) a,\
		(select sum(blocks_fetched) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) b;",
		snap_end, dbbuf.databaseid, oids, snap_begin, dbbuf.databaseid, oids);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.io_mv_read = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.io_mv_read < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//io_mv_buf
	sprintf(sql,"select a.x1-b.x1 from \
		(select sum(blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) a,\
		(select sum(blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) b;",
		snap_end, dbbuf.databaseid, oids, snap_begin, dbbuf.databaseid, oids);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.io_mv_buf = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.io_mv_buf < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//io_mv_disk
	sprintf(sql,"select a.x1-b.x1 from \
		(select sum(blocks_fetched-blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) a,\
		(select sum(blocks_fetched-blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) b;",
		snap_end, dbbuf.databaseid, oids, snap_begin, dbbuf.databaseid, oids);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.io_mv_disk = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.io_mv_disk < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//io_comb_read
	memset(oids,0,sizeof(oids));
	oid2str(oids,'c');
	sprintf(sql,"select a.x1-b.x1 from \
		(select sum(blocks_fetched) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) a,\
		(select sum(blocks_fetched) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) b;",
		snap_end, dbbuf.databaseid, oids, snap_begin, dbbuf.databaseid, oids);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.io_comb_read = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.io_comb_read < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//io_comb_buf
	sprintf(sql,"select a.x1-b.x1 from \
		(select sum(blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) a,\
		(select sum(blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) b;",
		snap_end, dbbuf.databaseid, oids, snap_begin, dbbuf.databaseid, oids);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.io_comb_buf = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.io_comb_buf < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//io_comb_disk
	sprintf(sql,"select a.x1-b.x1 from \
		(select sum(blocks_fetched-blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) a,\
		(select sum(blocks_fetched-blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) b;",
		snap_end, dbbuf.databaseid, oids, snap_begin, dbbuf.databaseid, oids);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.io_comb_disk = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.io_comb_disk < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}


	PQclear(res);

	//io_toast_read
	memset(oids,0,sizeof(oids));
	oid2str(oids,'t');
	sprintf(sql,"select a.x1-b.x1 from \
		(select sum(blocks_fetched) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) a,\
		(select sum(blocks_fetched) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) b;",
		snap_end, dbbuf.databaseid, oids, snap_begin, dbbuf.databaseid, oids);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.io_toast_read = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.io_toast_read < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//io_toast_buf
	sprintf(sql,"select a.x1-b.x1 from \
		(select sum(blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) a,\
		(select sum(blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) b;",
		snap_end, dbbuf.databaseid, oids, snap_begin, dbbuf.databaseid, oids);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.io_toast_buf = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.io_toast_buf < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//io_toast_disk
	sprintf(sql,"select a.x1-b.x1 from \
		(select sum(blocks_fetched-blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) a,\
		(select sum(blocks_fetched-blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) b;",
		snap_end, dbbuf.databaseid, oids, snap_begin, dbbuf.databaseid, oids);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.io_toast_disk = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.io_toast_disk < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//io_foreign_read
	memset(oids,0,sizeof(oids));
	oid2str(oids,'f');
	sprintf(sql,"select a.x1-b.x1 from \
		(select sum(blocks_fetched) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) a,\
		(select sum(blocks_fetched) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) b;",
		snap_end, dbbuf.databaseid, oids, snap_begin, dbbuf.databaseid, oids);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.io_foreign_read = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.io_foreign_read < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//io_foreign_buf
	sprintf(sql,"select a.x1-b.x1 from \
		(select sum(blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) a,\
		(select sum(blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) b;",
		snap_end, dbbuf.databaseid, oids, snap_begin, dbbuf.databaseid, oids);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.io_foreign_buf = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.io_foreign_buf < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//io_foreign_disk
	sprintf(sql,"select a.x1-b.x1 from \
		(select sum(blocks_fetched-blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) a,\
		(select sum(blocks_fetched-blocks_hit) x1 from _pg_pdr_.pdrtable where snap_id=%ld and databaseid=%u and tableid in (%s)) b;",
		snap_end, dbbuf.databaseid, oids, snap_begin, dbbuf.databaseid, oids);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.io_stats.io_foreign_disk = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	/* Indicator check */
	if (dbbuf.io_stats.io_foreign_disk < 0)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was closed unexpected.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	//io_total_read
	dbbuf.io_stats.io_total_read = dbbuf.io_stats.io_rel_read
								 + dbbuf.io_stats.io_index_read
								 + dbbuf.io_stats.io_seq_read
								 + dbbuf.io_stats.io_view_read
								 + dbbuf.io_stats.io_mv_read
								 + dbbuf.io_stats.io_comb_read
								 + dbbuf.io_stats.io_toast_read
								 + dbbuf.io_stats.io_foreign_read;

	//io_total_read_buf
	dbbuf.io_stats.io_total_buf = dbbuf.io_stats.io_rel_buf
	   							+ dbbuf.io_stats.io_index_buf
	   							+ dbbuf.io_stats.io_seq_buf
	   							+ dbbuf.io_stats.io_view_buf
	   							+ dbbuf.io_stats.io_mv_buf
	   							+ dbbuf.io_stats.io_comb_buf
	   							+ dbbuf.io_stats.io_toast_buf	
	   							+ dbbuf.io_stats.io_foreign_buf;

	//io_total_read_disk
	dbbuf.io_stats.io_total_disk = dbbuf.io_stats.io_rel_disk
								 + dbbuf.io_stats.io_index_disk
								 + dbbuf.io_stats.io_seq_disk
								 + dbbuf.io_stats.io_view_disk
								 + dbbuf.io_stats.io_mv_disk
								 + dbbuf.io_stats.io_comb_disk
								 + dbbuf.io_stats.io_toast_disk
								 + dbbuf.io_stats.io_foreign_disk;  													 

	/* Indicator check */
	//_pg_pdr_.pdrdb grouping by dbstart_ts
	sprintf(sql,"select dbstart_ts,count(*),min(snap_id),max(snap_id),extract(epoch FROM max(snap_ts))-extract(epoch FROM dbstart_ts) - (extract(epoch FROM min(snap_ts))-extract(epoch FROM dbstart_ts)) from _pg_pdr_.pdrdb \
		where databaseid=%u and snap_id >= %ld and snap_id <= %ld group by dbstart_ts order by dbstart_ts;",
		dbbuf.databaseid,snap_begin,snap_end);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	for (i = 0; i < PQntuples(res); i++)
	{
		dbGpInfo[i].dbstart_ts = strtoll(PQgetvalue(res, i, 0),NULL,10);
		dbGpInfo[i].snapcount = strtoll(PQgetvalue(res, i, 1),NULL,10);
		dbGpInfo[i].minsnapid = strtoll(PQgetvalue(res, i, 2),NULL,10);
		dbGpInfo[i].maxsnapid = strtoll(PQgetvalue(res, i, 3),NULL,10);
		dbbuf.run_tv += strtod(PQgetvalue(res, i, 4),NULL);
		iDbStartCount++;
		if (iDbStartCount >= 102400)
		{
			fprintf(stderr, "Number of database startup out of range.");
			PQclear(res);
			exit_nicely(conn);
		}
		
	}
	/* pdrwait data is invalid if database have a restart */
	if (iDbStartCount > 1)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was restarted.\n");
		PQclear(res);
		exit_nicely(conn);
	}
	dbbuf.run_tv = round(dbbuf.run_tv);
	
	if (fabs(dbbuf.run_tv)<1e-15)
	{
		fprintf(stderr, "pdr_report generation error：\nTime is too short between begin_snapid and end_snapid.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	PQclear(res);

	/* add more check on pdrdb */
	sprintf(sql,"select snap_id,extract(epoch FROM snap_ts) from _pg_pdr_.pdrdb \
		where databaseid=%u and snap_id >= %ld and snap_id <= %ld order by snap_id;",
		dbbuf.databaseid,snap_begin,snap_end);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	i = 0;
	snap_check = strtoll(PQgetvalue(res, i, 0),NULL,10);
	snap_data_early = strtod(PQgetvalue(res, i, 1),NULL);
	for (i = 1; i < PQntuples(res); i++)
	{
		snap_check = strtoll(PQgetvalue(res, i, 0),NULL,10);
		if (i == 0)
		{
			continue;
		}
		else
		{
			snap_data_late = strtod(PQgetvalue(res, i, 1),NULL);
		}
		if (snap_data_early > snap_data_late)
		{
			fprintf(stderr, "pdr_report generation error：\nwhen we compute the data of pdr_report,some data are negative.\n");
			PQclear(res);
			exit_nicely(conn);
		}
		
		snap_data_early = snap_data_late;
	}
	
	PQclear(res);

	/* add more check on pdrsql */
	sprintf(sql,"select snap_id,sum(total_exec_time) from _pg_pdr_.pdrsql \
		where dbid=%u and snap_id >= %ld and snap_id <= %ld group by snap_id order by snap_id;",
		dbbuf.databaseid,snap_begin,snap_end);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	i = 0;
	snap_check = strtoll(PQgetvalue(res, i, 0),NULL,10);
	snap_data_early = strtod(PQgetvalue(res, i, 1),NULL);
	for (i = 1; i < PQntuples(res); i++)
	{
		snap_check = strtoll(PQgetvalue(res, i, 0),NULL,10);
		if (i == 0)
		{
			continue;
		}
		else
		{
			snap_data_late = strtod(PQgetvalue(res, i, 1),NULL);
		}
		if (snap_data_early > snap_data_late)
		{
			fprintf(stderr, "pdr_report generation error：\nwhen we compute the data of pdr_report,some data are negative.\n");
			PQclear(res);
			exit_nicely(conn);
		}
		
		snap_data_early = snap_data_late;
	}
	
	PQclear(res);

	/* get dbbuf.run_tv and then replace_io_stats */
	replace_io_stats(buf);

	
	/* run_tv transform like -- 1 day 06:10:13 */
	sprintf(sql,"select date_trunc('second',age(timestamp 'epoch' + %.2f  * INTERVAL '1 second','1970-01-01'));", dbbuf.run_tv);

	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		strcpy(dbbuf.elapsed,PQgetvalue(res, 0, 0));
	PQclear(res);

	//wait event init all wait_stats.name and wait_stats.type
	memset((void*)(dbbuf.wait_stats), 0, sizeof(dbbuf.wait_stats));
	memset((void*)(wait_stats_begin), 0, sizeof(wait_stats_begin));
	sprintf(sql,"select event,event_type,count(*) from _pg_pdr_.pdrwait where snap_id >= %ld and snap_id <= %ld and event is not null group by event,event_type ;",
		snap_begin, snap_end);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	result_count = PQntuples(res);
	if (result_count < 1)
	{
		fprintf(stderr, "pdrwait data error：\nNo connection has occurred on the database or data was abnormal.\n");
		PQclear(res);
		exit_nicely(conn);
	}
	for (i = 0; i < result_count; i++)
	{
		strcpy(dbbuf.wait_stats[i].name, PQgetvalue(res, i, 0));
		strcpy(dbbuf.wait_stats[i].type, PQgetvalue(res, i, 1));
		strcpy(wait_stats_begin[i].name, dbbuf.wait_stats[i].name);
		strcpy(wait_stats_begin[i].type, dbbuf.wait_stats[i].type);
		wait_stat_count++;
	}
	PQclear(res);

	for (j = 0; j < wait_stat_count; j++)
	{
		wait_stats_begin[j].count = 0;
	}
	sprintf(sql,"select event,sum(count) from _pg_pdr_.pdrwait where snap_id=%ld  group by event;",
		snap_begin);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	for (j = 0; j < PQntuples(res); j++)
	{
		memset(wait_ev_name,0x00,sizeof(wait_ev_name));
		strcpy(wait_ev_name, PQgetvalue(res, j, 0));
		wait_ev_index = find_index_wait_ev_begin(wait_ev_name);
		if (wait_ev_index == -1)
		{
			fprintf(stderr, "pdr_report error\n");
			PQclear(res);
			exit_nicely(conn);
		}
		wait_stats_begin[wait_ev_index].count = strtoull(PQgetvalue(res, j, 1),NULL,10);
	}
	PQclear(res);
	
	sprintf(sql,"select event,sum(count) from _pg_pdr_.pdrwait where snap_id=%ld group by event;",
		snap_end);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	for (j = 0; j < PQntuples(res); j++)
	{
		memset(wait_ev_name,0x00,sizeof(wait_ev_name));
		strcpy(wait_ev_name, PQgetvalue(res, j, 0));
		wait_ev_index = find_index_wait_ev(wait_ev_name);
		if (wait_ev_index == -1)
		{
			fprintf(stderr, "pdr_report error\n");
			PQclear(res);
			exit_nicely(conn);
		}
		wait_ev_index_subbegin = find_index_wait_ev_begin(wait_ev_name);
		if (wait_ev_index == -1)
		{
			fprintf(stderr, "pdr_report error\n");
			PQclear(res);
			exit_nicely(conn);
		}
		snap_check = ( strtoull(PQgetvalue(res, j, 1),NULL,10) - wait_stats_begin[wait_ev_index_subbegin].count );
		if (snap_check < 0)
		{
			fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was restarted.\n");
			PQclear(res);
			exit_nicely(conn);
		}
		dbbuf.wait_stats[wait_ev_index].count += snap_check;
		
	}
	PQclear(res);

	sprintf(sql,"select a.x1 - coalesce(b.x1,0) from (select sum(count) x1 from _pg_pdr_.pdrwait where snap_id=%ld and event_type<>'Activity' and event_type is not null) as a, \
		(select sum(count) x1 from _pg_pdr_.pdrwait where snap_id=%ld and event_type<>'Activity' and event_type is not null) as b;",
		snap_end, snap_begin);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		dbbuf.wait_total = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	PQclear(res);
	
	
	//activity wait total
	sprintf(sql,"select a.x1 - coalesce(b.x1,0) from (select sum(count) x1 from _pg_pdr_.pdrwait where snap_id=%ld and event_type = 'Activity') as a, \
		(select sum(count) x1 from _pg_pdr_.pdrwait where snap_id=%ld and event_type = 'Activity') as b;",
		snap_end, snap_begin);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	result_count = PQntuples(res);
	if (result_count < 1)
	{
		fprintf(stderr, "pdrwait data error：\nNo connection has occurred on the database or data was abnormal.\n");
		PQclear(res);
		exit_nicely(conn);
	}
	if (result_count)
		dbbuf.wait_activity = strtoull(PQgetvalue(res, 0, 0),NULL,10);
	PQclear(res);

	/* find top10 of dbbuf.wait_stats */
	memset((void*)(dbbuf.wait_top10), 0, sizeof(dbbuf.wait_top10));
	j = 0;
	sprintf(sql,"select aa,count,cc from ( ");
	for (i = 0; i < wait_stat_count; i++)
	{
		memset(subsql,0x00,sizeof(subsql));
		if (i == 0)
		{
			sprintf(subsql," (select '%s' aa, %ld count, '%s' cc ) ",dbbuf.wait_stats[i].name,dbbuf.wait_stats[i].count,dbbuf.wait_stats[i].type);
			strcat(sql,subsql);
		}
		else
		{
			sprintf(subsql," union (select '%s' aa, %ld count, '%s' cc ) ",dbbuf.wait_stats[i].name,dbbuf.wait_stats[i].count,dbbuf.wait_stats[i].type);
			strcat(sql,subsql);
			j++;
		}
		
	}
	strcat(sql," ) X1 where X1.cc <> 'Activity' and X1.cc is not null order by X1.count desc limit 10 ");
	if (j < 1)
	{
		fprintf(stderr, "pdr_report generation error：\nwhen collect snapshot between begin_snapid and end_snapid, Highgo database was restarted.\n");
		PQclear(res);
		exit_nicely(conn);
	}

	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	
	for (i = 0; i < PQntuples(res); i++)
	{
		strcpy(dbbuf.wait_top10[i].name, PQgetvalue(res, i, 0));
		dbbuf.wait_top10[i].count = strtoull(PQgetvalue(res, i, 1),NULL,10);
		strcpy(dbbuf.wait_top10[i].type, PQgetvalue(res, i, 2));
		if (dbbuf.wait_top10[i].count > 0)
			maxevent++;
	}
	PQclear(res);


	replace_wait_stats(buf);
	replace_wait_top10(buf,maxevent);

	/*
	 * summary
	 */
	sprintf(sql,"select dbsysinfo from _pg_pdr_.pdrdb where snap_id=%ld and databaseid=%u;", snap_end, dbbuf.databaseid);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	memset(dbsysinfo,0x00,sizeof(dbsysinfo));
	if (PQntuples(res))
	{
		int notspace = 0;
		/**
		char		dbtime[32];
		char		dbver[32];
		char		dbhost[256];
		char		platform[32];
		int 		cpus;
		uint64		dbmem;
		**/
		strncpy(dbsysinfo,PQgetvalue(res, 0, 0),32);
		for (notspace = 0; notspace < 32; notspace++)
		{
			if (dbsysinfo[notspace] != ' ')
				break;
		}
		strncpy(dbbuf.dbtime,dbsysinfo+notspace,32-notspace);

		
		memset(dbsysinfo,0x00,sizeof(dbsysinfo));
		strncpy(dbsysinfo,PQgetvalue(res, 0, 0)+32,64);
		for (notspace = 0; notspace < 64; notspace++)
		{
			if (dbsysinfo[notspace] != ' ')
				break;
		}
		strncpy(dbbuf.dbver,dbsysinfo+notspace,64-notspace);

		
		memset(dbsysinfo,0x00,sizeof(dbsysinfo));
		strncpy(dbsysinfo,PQgetvalue(res, 0, 0)+96,256);
		for (notspace = 0; notspace < 256; notspace++)
		{
			if (dbsysinfo[notspace] != ' ')
				break;
		}
		strncpy(dbbuf.dbhost,dbsysinfo+notspace,256-notspace);
		
		memset(dbsysinfo,0x00,sizeof(dbsysinfo));
		strncpy(dbsysinfo,PQgetvalue(res, 0, 0)+352,32);
		for (notspace = 0; notspace < 32; notspace++)
		{
			if (dbsysinfo[notspace] != ' ')
				break;
		}
		strncpy(dbbuf.platform,dbsysinfo+notspace,32-notspace);

		memset(dbsysinfo,0x00,sizeof(dbsysinfo));
		strncpy(dbsysinfo,PQgetvalue(res, 0, 0)+384,16);
		for (notspace = 0; notspace < 16; notspace++)
		{
			if (dbsysinfo[notspace] != ' ')
				break;
		}
		dbbuf.cpus = strtoint64(dbsysinfo+notspace);

		memset(dbsysinfo,0x00,sizeof(dbsysinfo));
		strncpy(dbsysinfo,PQgetvalue(res, 0, 0)+400,16);
		for (notspace = 0; notspace < 16; notspace++)
		{
			if (dbsysinfo[notspace] != ' ')
				break;
		}
		dbbuf.dbmem = strtoint64(dbsysinfo+notspace);
	}

	PQclear(res);

	//begin time
	sprintf(sql,"select date_trunc('second',snap_ts) from _pg_pdr_.pdrsql where snap_id = %ld;", snap_begin);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		strcpy(dbbuf.begintime,PQgetvalue(res, 0, 0));
	PQclear(res);

	//end time
	sprintf(sql,"select date_trunc('second',snap_ts) from _pg_pdr_.pdrsql where snap_id = %ld;", snap_end);
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		strcpy(dbbuf.endtime,PQgetvalue(res, 0, 0));
	PQclear(res);

	//str_run_tv
	/* str_run_tv db_totaltime includes both db_cpu and db_wait..round(dbbuf.db_totaliotime) db_totaliotime*/
	/*
	 * BUG#Z5777
	 * "DB Time" is the all time for database running
	 * its not the same as "Elapsed" time
	 * for multi cores system, DB Time = sum(every core's DB time)
	 * so it maybe longer than "Elapsed" time, but must smaller than "Elapsed" time * cores
	 * dbbuf.db_totaltime is millisecond, DB time is second 
	 * */
	sprintf(sql,"select date_trunc('second',age(timestamp 'epoch' + %.2f  * INTERVAL '1 second','1970-01-01'));", round(dbbuf.db_totaltime/1000.0)); /* BUG#Z5777 */
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	if (PQntuples(res))
		strcpy(dbbuf.str_run_tv,PQgetvalue(res, 0, 0));
	PQclear(res);

	//load profile
	sprintf(sql,"select a.x1-b.x1,a.x2-b.x2,a.x3-b.x3,a.x4-b.x4,a.x5-b.x5,a.x6-b.x6,a.x7-b.x7,a.x8-b.x8,a.x9-b.x9,a.x10-b.x10,\
		a.x11-b.x11,a.x12-b.x12,a.x13-b.x13,a.x14-b.x14,a.x15-a.x15,a.x16-b.x16,a.x17-b.x17,a.x18-b.x18,a.x19-b.x19 \
		from (select sum(n_xact_commit) x1,sum(n_xact_rollback) x2,sum(n_blocks_fetched) x3,sum(n_blocks_hit) x4,\
		sum(n_tuples_returned) x5,sum(n_tuples_fetched) x6,sum(n_tuples_inserted) x7,sum(n_tuples_updated) x8, \
		sum(n_tuples_deleted) x9,sum(n_conflict_tablespace) x10,sum(n_conflict_lock) x11,sum(n_conflict_snapshot) x12, \
		sum(n_conflict_bufferpin) x13,sum(n_conflict_startup_deadlock) x14,sum(n_temp_files) x15,sum(n_temp_bytes) x16, \
		sum(n_deadlocks) x17,sum(n_block_read_time) x18,sum(n_block_write_time) x19 from _pg_pdr_.pdrdb where snap_id=%ld and databaseid=%u) as a,\
		(select sum(n_xact_commit) x1,sum(n_xact_rollback) x2,sum(n_blocks_fetched) x3,sum(n_blocks_hit) x4,\
		sum(n_tuples_returned) x5,sum(n_tuples_fetched) x6,sum(n_tuples_inserted) x7,sum(n_tuples_updated) x8, \
		sum(n_tuples_deleted) x9,sum(n_conflict_tablespace) x10,sum(n_conflict_lock) x11,sum(n_conflict_snapshot) x12, \
		sum(n_conflict_bufferpin) x13,sum(n_conflict_startup_deadlock) x14,sum(n_temp_files) x15,sum(n_temp_bytes) x16, \
		sum(n_deadlocks) x17,sum(n_block_read_time) x18,sum(n_block_write_time) x19 from _pg_pdr_.pdrdb where snap_id=%ld and databaseid=%u) as b;",
		snap_end, dbbuf.databaseid, snap_begin, dbbuf.databaseid);	
	res = PQexec(conn, sql);
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		fprintf(stderr, "select failed: %s", PQerrorMessage(conn));
		PQclear(res);
		exit_nicely(conn);
	}
	for (i = 0; i < PQntuples(res); i++)
	{
		dbbuf.n_xact_commit = strtoull(PQgetvalue(res, i, 0),NULL,10);
		dbbuf.n_xact_rollback = strtoull(PQgetvalue(res, i, 1),NULL,10);
		dbbuf.n_blocks_fetched = strtoull(PQgetvalue(res, i, 2),NULL,10);
		dbbuf.n_blocks_hit = strtoull(PQgetvalue(res, i, 3),NULL,10);
		dbbuf.n_tuples_returned = strtoull(PQgetvalue(res, i, 4),NULL,10);
		dbbuf.n_tuples_fetched = strtoull(PQgetvalue(res, i, 5),NULL,10);
		dbbuf.n_tuples_inserted = strtoull(PQgetvalue(res, i, 6),NULL,10);
		dbbuf.n_tuples_updated = strtoull(PQgetvalue(res, i, 7),NULL,10);
		dbbuf.n_tuples_deleted = strtoull(PQgetvalue(res, i, 8),NULL,10);
		dbbuf.n_conflict_tablespace = strtoull(PQgetvalue(res, i, 9),NULL,10);
		dbbuf.n_conflict_lock = strtoull(PQgetvalue(res, i, 10),NULL,10);
		dbbuf.n_conflict_snapshot = strtoull(PQgetvalue(res, i, 11),NULL,10);
		dbbuf.n_conflict_bufferpin = strtoull(PQgetvalue(res, i, 12),NULL,10);
		dbbuf.n_conflict_startup_deadlock = strtoull(PQgetvalue(res, i, 13),NULL,10);
		dbbuf.n_temp_files = strtoull(PQgetvalue(res, i, 14),NULL,10);
		dbbuf.n_temp_bytes = strtoull(PQgetvalue(res, i, 15),NULL,10);
		dbbuf.n_deadlocks = strtoull(PQgetvalue(res, i, 16),NULL,10);
		dbbuf.n_block_read_time = strtoull(PQgetvalue(res, i, 17),NULL,10);
		dbbuf.n_block_write_time = strtoull(PQgetvalue(res, i, 18),NULL,10);
	}
	PQclear(res);

	replace_summary(buf);

	while(1)
	{
		printf("please input html file name for this report(***.html):\n");
		dbstflag = false;
		iScanfCount = 0;
		while(1)
		{
			if (scanf("%c",&cScanf) != 1)
			{
				// do nothing
			}

			if (iScanfCount > 62)
			{
				//Ignore bytes more than 63
				//break;
			}
			if (cScanf == '\n')
			{
				if (iScanfCount == 0)
					break;
				else if (iScanfCount < 63)
				{
					outfile[iScanfCount] = 0x00;
					dbstflag = true;
					break;
				}
				else
				{
					dbstflag = true;
					break;
				}
			}
			if (iScanfCount == 0)
			{
				memset(outfile,0x00,sizeof(outfile));
			}
			if (iScanfCount == 63)
			{
				outfile[iScanfCount] = 0x00;
			}
			else if (iScanfCount < 63)
			{
				outfile[iScanfCount] = cScanf;
			}
			iScanfCount++;
		}

		if (dbstflag)
		{
			memset(outpath,0,sizeof(outpath));

#ifdef WIN32
			sprintf(outpath,"%s",outfile);
#else
			if (outfile[0] == '/')
				sprintf(outpath,"%s",outfile);
			else 
				sprintf(outpath,"./%s",outfile);
#endif

			fpout = fopen(outpath, "w");
			
			if (fpout == NULL)
			{
				/* only for linux */
				if (errno == 2)
				{
					printf("No such file or directory:%s\n",outpath);
				}
				else if (errno == 13)
				{
					printf("Permission denied:%s\n",outpath);
				}
				else 
				{
					printf("can not write file %s errno=%d\n",outpath,errno);
				}
				
			}
			else 
			{
				break;
			}
		}
		else
		{

		}
	}

	p = strstr(buf,"End of Report");
	
	if (p)	
		fwrite(buf, p-buf+14, 1, fpout);

	printf("report %s has been successfully created.\n", outpath);

	//close the connection to the database and cleanup 
	PQfinish(conn);

	return 0;
}
