/* -------------------------------------------------------------------------
 *
 * File: hsh.c
 *
 * Abstract:
 * This file is the specific implementation of session history 
 *
 * Copyright:
 * 	Copyright (c) 2025, HighGo Software Co.,Ltd. All right reserved
 *
 * Identification: 
 *	contrib/hsh/hsh.c
 *
 * -------------------------------------------------------------------------
 */
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include "postgres.h"

/* These are always necessary for a bgworker */
#include "miscadmin.h"
#include "funcapi.h"

#include "access/xlog.h"
#include "access/xact.h"
#include "access/table.h"
#include "access/htup.h"
#include "access/heapam.h"
#include "access/genam.h"
#include "catalog/namespace.h"
#include "catalog/index.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_database.h"
#include "catalog/pg_authid.h"
#include "common/ip.h"
#include "commands/dbcommands.h"
#include "datatype/timestamp.h"
#include "executor/spi.h"
#include "postmaster/bgworker.h"
#include "postmaster/interrupt.h"
#include "storage/lwlock.h"
#include "storage/ipc.h"
#include "storage/shmem.h"
#include "storage/procarray.h"
#include "storage/proc.h"
#include "storage/shm_toc.h"
#include "utils/backend_status.h"
#include "utils/builtins.h"
#include "utils/elog.h"
#include "utils/guc.h"
#include "utils/datetime.h"
#include "utils/wait_event.h"
#include "utils/resowner.h"
#include "utils/snapmgr.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"
#include "utils/fmgroids.h"

/*
 * as we will show the database version
 * so keep the consistent with license module
 */
#ifdef USE_LICENSE
#include "license.h"
#endif

/* these headers are used by this particular worker's code */

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

#define STR_LEN 			128
#define UINT32_ACCESS_ONCE(var)		((uint32)(*((volatile uint32 *)&(var))))
#define HSH_COLLECTOR_MAGIC		0x7FD88CAC
#define Natts_hsh_history		17

#define HSH_HISTORY_NAMESPACE		"_hg_perf_"
#define HSH_HISTORY_DATA		"hsh_history_data"
#define HSH_STATEMENTS			"hsh_statements"

/* temp table, using pg_temp_xx schema */
#define HSH_RESULT			"hsh_result"
#define WRITER_CHECK			10 * 1000
#define	INTERVAL			15 * 60

bool	hsh_collector = false;
int	hsh_history_size = 100000;
int	hsh_history_time = 7;
char	*hsh_database;
int	hsh_writer_stat_interval = 1;
int	hsh_writer_ringbuf_interval = 10;
int	hsh_writer_clean_interval	= 60;

static volatile sig_atomic_t got_sighup = false;
static volatile sig_atomic_t got_sigterm = false;


/*
 * Data source type
 * used to define how to get the session sample to cal the report
 */
typedef enum
{
	/* could not find which type should be used to cal the report */
	NONE,
	/* used the mem data to cal the report */
	MEM_ONLY,
	/* used the db data to cal the report */
	DB_ONLY,
	/* used the mem and the db data to cal the report */
	COMBINE
}DATA_TYPE;

/*
 * HSH ringbuf item
 * used to store the sessoin info
 * refer to PgBackendStatus
 */
typedef struct
{
	/* used for find the sample to stored */
	uint64			sample_id;
	/* Now */
	TimestampTz 		ts;
	/* PgBackendStatus */
	int                     st_procpid;
	/* client addr */
	SockAddr                st_clientaddr;
	/* app name */
	char                    st_appname[STR_LEN];

	/* database id and database name */
	Oid                     st_databaseid;
	char			st_databasename[STR_LEN];

	/* user id and user name */
	Oid                     st_userid;
	char			st_username[STR_LEN];

	/* Wait event */
	uint32			raw_wait_event;
	char			wait_event_type[STR_LEN];
	char			wait_event[STR_LEN];

	/* query id from pg_stat_statements */
	uint64                  st_query_id;

	/* command Type, refer global CmdType */
	CmdType			st_cmd_type;

	/* query phase */
	QueryPhaseKind		st_query_phase;

	/* backend type and state */
	BackendType 		st_backendType;
	BackendState 		st_state;

	/* if this item need to stored */
	bool                    is_flushed_sample;

	/* more info for report, if need */
	ProgressCommandType     st_progress_command;
	char			st_activity_raw[STR_LEN];

} SessionHistEntry;

/*
 * HSH ringbuf 
 * its a Circular Queue
 * A lw lock to keep consistency for reading and writing
 */
typedef struct
{
	LWLock		lock;		/* lock for consistency */
	bool		is_full;	/* is full or false */
	int		max_size;	/* max queue length */
	int		curr_index;	/* the next index to write */
	int		sample_index;	/* for writing sample: the latest index have written */
	int		sample_index_saved;
	SessionHistEntry	*buffer; /* buffer */

	TimestampTz	old_ts;	/* the oldest timestamp for update the buff */
	TimestampTz	new_ts;	/* the latest timestamp for update the buff */

} History;

/*
 * mapping location for SessionHistEntry
 * used when SessionHistEntry --> tuple
 */
typedef enum
{
	NATTS_TS = 0, /* 0 */
	NATTS_PID,
	NATTS_ADDR,
	NATTS_APPNAME,
	NATTS_DBID,
	NATTS_DBNAME,	/* 5 */
	NATTS_USEID,
	NATTS_USERNAME,
	NATTS_EVENTTYPE,
	NATTS_EVENT,
	NATTS_QUERYID, /* 10 */
	NATTS_CMDTYPE,
	NATTS_QUERYPHASE,
	NATTS_BACKTYPE,
	NATTS_SATATE,
	NATTS_ISSAMPLE,	/* 15 */
	NATTS_ACTIVITY_RAW,
	NATTS_PROGRESSCOMMAND
}NATTS_HSH;

/*
 * top event info
 * used for top event report part
 */
typedef struct top_event
{
	char  wait_type[STR_LEN];
	char  wait_event[STR_LEN];
	uint64 count;
	double  count_d_sample;
	double  count_d_time;
}top_event;

/*
 * top sql command type
 * used for top sql report part
 */
typedef struct top_sql
{
	char	name[STR_LEN];
	uint64	count;
	int	id_type;
	double	count_d_sample;
	double	count_d_time;
}top_sql;

/*
 * top sql with top event
 * top sql, then top event for this sql
 * used for top sql top event report part
 */
typedef struct topsql_with_topevent
{
	uint64	query_id;
	uint64	counts;
	double	counts_d_sample;
	char	wait_event[STR_LEN];
	uint64	wait_counts;
	double	wait_counts_d_counts;
}topsql_with_topevent;

/*
 * top session
 * cal group by pid
 * cal the top event for this pid
 */
typedef struct top_session
{
	int	pid;
	uint64	pid_counts;
	double  pid_d_sample;
	double	pid_d_ts;
	char	wait_event[STR_LEN];
	uint64 	event_counts;
	double	event_d_pid;
}top_session;

/*
 * Top phase
 * other, parse, plan, bind, exec
 */
typedef struct top_phase
{
	char	phase_name[STR_LEN];
	uint64	phase_count;
	double	count_d_sum;
	double  count_d_sample;
	double  count_d_ts;	
}top_phase;

/*
 * over time for interval
 * used for Activity over time report part
 */
typedef struct sample_slot
{
	Timestamp	begin;
	Timestamp	end;
	TimestampTz	begin_tz;
	TimestampTz	end_tz;
	char		begin_str[MAXDATELEN + 1];
	char            end_str[MAXDATELEN + 1];
	int		sample;
	char		wait_event[STR_LEN];
	int		count;
	double		count_d_sample;
}sample_slot;

/*
 * sql info
 * used for detail SQL report part
 */
typedef struct sql_slot
{
	uint64  query_id;
	char	text[STR_LEN * 2];
}sql_slot;

#define TOP_LEN		10
#define	SLOT_LEN	12


/*
 * the result for report
 * top_event
 * top_sql
 * topsql_with_topevent
 * top session
 * over_time
 * sql_list
 */
typedef struct hsh_entry
{
	Timestamp	start_tm;
	Timestamp	end_tm;
	TimestampTz	start_tmtz;
	TimestampTz	end_tmtz;

	DATA_TYPE	sample_source;
	int		sample_counts;
	int		sample_time;
	double		avg_sessions;

	int		top_event_counts;
	top_event	top_events[TOP_LEN];

	int		top_sql_counts;
	top_sql		top_sqls[TOP_LEN];

	int		topsql_with_topevent_counts;
	topsql_with_topevent	topsql_with_topevents[TOP_LEN];

	int		top_session_counts;
	top_session	top_sessions[TOP_LEN];

	int		top_phase_counts;
	top_phase	top_phases[TOP_LEN];

	int		sample_slot_counts;
	sample_slot	sample_slots[SLOT_LEN];

	int		sql_slot_counts;
	sql_slot	sql_slots[10];
}hsh_entry;

hsh_entry entry;

/*
 * Used to define writer style
 */
typedef enum
{
	/* save history sample to table */
	SAVE_HISTORY = 1,
	/* clean history sample in the table */
	CLEAN_HISTORY,
	/* save query_id and query_text to the table */
	SAVE_STATE,
	/* clean query_id and query_text in the table */
	CLEAN_STATE
}TIMER_TYPE;

/*
 * Used to define which type data will read from history buff
 */
typedef enum
{
	/* read all data in the history buff */
	READ_ALL = 1,
	/* just read history sample data in the history buff */
	READ_SAMPLE
}SAMPLE_TYPE;

static shmem_startup_hook_type prev_shmem_startup_hook = NULL;

static volatile sig_atomic_t shutdown_requested = false;
static void handle_sigterm(SIGNAL_ARGS);

/* Pointers to shared memory objects */
History   *ring_buf = NULL;
#define HISTORY_ENTRY_LEN sizeof(SessionHistEntry)

/* html report template */

/*
 * report style 
 */
const char *html_style = "<html><head><meta charset=\"UTF-8\"><title>Highgo Activity Session Report</title>";

const char *html_header = "<style> table {width: 1000;} table,th,td {border: 1px solid gray; border-collapse: collapse; padding: 1px;} table tr td.value,table tr td.mono{font-family: Monospace;text-align: right}table tr.parent td:not(.relhdr) {background-color: #D8E8C2;}table tr.child td {background-color: #BBDD97; border-top-style: hidden;}table tr:nth-child(even){background-color: #eee;}table tr:nth-child(odd){background-color: #fff;}table tr:hover td:not(.relhdr) {background-color:#d9ffcc}table th {color: white; background-color: #003366;}table tr:target td{background-color: #EBEDFF;}table tr:target td:first-of-type {font-weight: bold;}td.top_sql_query {text-align: left; text-overflow: ellipsis; white-space: nowrap; overflow: hidden;}</style> </head>";

/*
 * report version
 */
const char *html_version_h = "<H2></H2><table><tr><th width=\"200\">按库展示</th><th width=\"200\">发行版本</th><th width=\"100\">数据来源</th></tr>";

const char *html_version_b = "<tr class=\"parent\"><td class=\"value\" style=\"text-align: center\">{{DATABASE_NAME}}</td><td class=\"value\" style=\"text-align: center\">{{DATABASE_VERSION}}</td><td class=\"value\" style=\"text-align: center\">{{DATABASE_SOURCE}}</td></tr></table><H2></H2>";

/*
 * report sample info
 */
const char *html_sample_h = "<H2></H2><table><tr><th width=\"100\">起始时间</th><th width=\"100\">结束时间</th><th width=\"100\">计时时间</th><th width=\"100\">样本总数</th><th width=\"100\">平均会话数</th></tr>";

const char *html_sample_b = "<tr class=\"parent\"><td class=\"value\">{{START_TIME}}</td><td class=\"value\">{{END_TIME}}</td><td class=\"value\">{{DURATION}}</td><td class=\"value\">{{TOTAL_SAMPLE}}</td><td class=\"value\">{{AVG_SESSIONS}}</td></tr></table>";

/* Begin - BUG#z6128 */
const char *html_sample_d = "<ul><li>字段“样本总数”：采集时间内的样本总数</li><li>字段“平均会话数”：数据来源为 MEMORY 时， 样本总数除以计时时间；数据来源为 DB/COMBINE 时，(样本总数除以计时时间)*10。</li></ul>";
/* End - BUG#z6128 */

/*
 * report body 
 */
const char *html_sample_report_body = "<H2><a Name=report_main>报告主体</a></H2><ul><li><a HREF=#top_user_events>Top 用户事件</a></li><li><a HREF=#top_sql_command_type>Top SQL 命令类型</a></li><li><a HREF=#top_sql_with_top_events>等待事件高的 Top SQL</a></li><li><a HREF=#top_session>Top 会话</a></li><li><a HREF=#top_phase>Top 执行阶段</a></li><li><a HREF=#activity_over_time>分段时间等待事件统计</a></li><li> <a HREF=#complete_list_sql_text>完整 SQL 列表</a></li></ul>";

/*
 * top event
 */
const char *html_event_t = "<H3> <a NAME=top_user_events>Top 用户事件</a></H3>";

const char *html_event_d = "<ul><li>显示客户端进程中按采样数排名前 10 的等待事件</li><li>字段“样本数”：采集时间内出现的等待事件次数</li><li>字段“活动占比”：事件样本数除以总的采样次数</li><li>字段“平均会话数”：事件采样数除以总的采样时间</li></ul>";

const char *html_event_h = "<table><tr><th width=\"200\">事件类型</th><th width=\"200\">事件名称</th><th width=\"200\">样本数</th><th width=\"200\">活动占比</th><th width=\"200\">平均会话数</th></tr>";

const char *html_event_b = "<tr class=\"parent\"> <td class=\"relhdr\" >{{event_type}}</td> <td class=\"value\" style=\"text-align: left\">{{event_event}}</td> <td class=\"value\">{{event_sample_count}}</td> <td class=\"value\">{{event_percentage}}</td> <td class=\"value\">{{event_avg}}</td>";

const char *html_events_e = "</table><p><a HREF=#report_main>返回到报告主体</a>";

/*
 * top sql type
 */
const char *html_sql_type_t = "<H3><a NAME=top_sql_command_type>Top SQL 命令类型</a></H3>";

const char *html_sql_type_d = "<ul><li>显示不同类型 SQL 的信息</li><li>字段“占比”：SQL 类型相同的不同 queryid 数量除以总的采样次数</li><li>字段“平均活跃会话数”：SQL 类型相同的不同 queryid 数量除以总的采样时间</li></ul>";

const char *html_sql_type_h = "<table><tr><th width=\"200\">SQL 类型</th><th width=\"200\">不同的Query ID</th><th width=\"200\">占比</th><th width=\"200\">平均活跃会话数</th></tr>";

const char *html_sql_type_b = "<tr class=\"parent\"> <td class=\"relhdr\" >{{SQL_TYPE}}</td> <td class=\"value\">{{SQL_DIFF}}</td> <td class=\"value\">{{SQL_PERCENTAGE}}</td> <td class=\"value\">{{SQL_AVG}}</td>";

const char *html_sql_type_e = "</table><p><a HREF=#report_main>返回到报告主体</a>";

/*
 * top sql with top event
 */
const char *html_sql_event_t = "<H3><a NAME=top_sql_with_top_events>等待事件高的 Top SQL</a></H3>";

const char *html_sql_event_d = " <ul><li>显示出现次数最多的 10 条 SQL 语句，以及对应次数最高的等待事件</li><li>字段“活动占比”：SQL 语句采样数除以总的采样次数</li><li>字段“事件占比”：等待事件采样数除以 SQL 语句采样数</li></ul>";

const char *html_sql_event_h = "<table><tr><th width=\"200\">Query Id</th><th width=\"200\">采样数</th><th width=\"200\">活动占比</th><th width=\"200\">事件名称</th><th width=\"200\">事件次数</th><th width=\"200\">事件占比</th></tr>";

const char *html_sql_event_b = " <tr class=\"parent\"><td class=\"relhdr\" style=\"text-align: left\"><a href=\"#{{QUERY_REF}}\">{{QUERY_ID}}</a></td><td class=\"value\">{{QUERY_COUNTS}}</td><td class=\"value\">{{QUERY_PRE}}</td><td class=\"value\" style=\"text-align: left\">{{QUERY_EVENT}}</td><td class=\"value\">{{QUERY_EVENT_COUNTS}}</td><td class=\"value\">{{QUERY_EVENT_AVG}}</td></tr>";

const char *html_sql_event_e = "</table><p><a HREF=#report_main>返回到报告主体</a>";

/*
 * top session
 */
const char *html_session_t = "<H3><a NAME=top_session>Top 会话</a></H3>";

/* Begin - BUG#z6128 */
const char *html_session_d = "<ul><li>显示会话中出现次数最高的 event 信息</li><li>字段“会话占比”：会话采样数除以总的采样次数</li><li>字段“平均会话数”：会话采样数除以总的采样时间</li><li>字段“事件名称”：会话采样中，出现次数最多的时间</li><li>字段“事件占比”：当前事件的采样数除以当前会话的总采样数</li></ul>";
/* End - BUG#z6128 */

const char *html_session_h = "<table><tr><th width=\"200\">会话 ID</th><th width=\"200\">会话占比</th><th width=\"200\">平均会话数</th><th width=\"200\">事件名称</th><th width=\"200\">事件占比</th>";

const char *html_session_b = "<tr class=\"parent\"><td class=\"relhdr\">{{SESSION_ID}}</td><td class=\"value\">{{SESSION_PRE}}</td><td class=\"value\">{{SESSION_AVG}}</td><td class=\"value\" style=\"text-align:left\">{{SESSION_EVENT}}</td><td class=\"value\">{{SESSION_EVENT_PRE}}</td></tr>";

const char *html_session_e = "</table><p><a HREF=#report_main>返回到报告主体</a>";

/*
 * top phase
 */
const char *html_phase_t = "<H3><a NAME=top_phase>Top 执行阶段</a></H3>";

const char *html_phase_d = "<ul><li>显示各个阶段的信息</li><li>字段“阶段次数”：开始和结束时间内次数</li><li>字段“阶段占比”：不同阶段的采样数除以总的采样次数</li><li>字段“平均会话数”：不同阶段的采样数除以总的采样时间</li></ul>";

const char *html_phase_h = "<table><tr><th width=\"200\">执行阶段</th><th width=\"200\">阶段次数</th><th width=\"200\">阶段占比</th><th width=\"200\">平均会话数</th>";

const char *html_phase_b = "<tr class=\"parent\"><td class=\"relhdr\">{{PHASE_NAME}}</td><td class=\"value\">{{PHASE_COUNT}}</td><td class=\"value\">{{PHASE_AVG}}</td><td class=\"value\" style=\"text-align:left\">{{PHASE_PRE}}</td></tr>";

const char *html_phase_e = "</table><p><a HREF=#report_main>返回到报告主体</a>";

/*
 * over time info
 */
const char *html_over_time_t = "<H3><a NAME=activity_over_time>分段时间等待事件统计</a></H3>";

const char *html_over_time_d = " <ul><li>显示按时间段划分的等待事件信息</li><li>字段“采样数”：开始和结束时间内采样次数</li><li>字段“事件类型”：采样区间内事件次数排名前三的等待事件</li><li>字段“事件数”：采样区间内的等待事件次数</li><li>字段“事件占比”：事件次数除以采样区间内总的等待事件次数</li></ul>";

const char *html_over_time_h = "<table><tr><th width=\"200\">开始时间</th><th width=\"200\">结束时间</th><th width=\"200\">采样数</th><th width=\"200\">事件类型</th><th width=\"200\">事件数</th><th width=\"200\">事件占比</th></tr>";

const char *html_over_time_b = "<tr class=\"parent\"><td class=\"value\">{{ACTIVITY_START}}</td><td class=\"value\">{{ACTIVITY_END}}</td><td class=\"value\">{{ACTIVITY_COUNTS}}</td><td class=\"value\" style=\"text-align: left\">{{EVENT_NAME}}</td><td class=\"value\">{{EVENT_COUNT}}</td><td class=\"value\">{{EVENT_PRE}}</td></tr>";

const char *html_over_time_e = "</table><p><a HREF=#report_main>返回到报告主体</a>";

/*
 * sql list
 */
const char *html_sql_list_t = "<H3><a NAME=complete_list_sql_text>完整 SQL 列表</a></H3>";

const char *html_sql_list_d = "<ul><li>显示SQL对应的具体信息</li></ul>";

const char *html_sql_list_h = "<table><tr><th width=\"200\">SQL ID</th><th width=\"300\">SQL 语句</th></tr>";

const char *html_sql_list_b = "<tr id=\"{{SQL_TEXT_REF}}\" class=\"parent\"><td class=\"relhdr\" style=\"text-align: left\">{{SQL_TEXT_ID}}</td><td class=\"value\" style=\"text-align: left\">{{SQL_TEXT_TEXT}}</td></tr>";

const char *html_sql_list_e = "</table><p><a HREF=#report_main>返回到报告主体</a>";

/*
 * report end
 */
const char *html_end = "</body></html>";


/*---- Function declarations ----*/
void            _PG_init(void);

/* collector */
void hsh_register_collector(void);
void hsh_register_writer(void);
void hsh_collector_main(Datum);
void hsh_writer_main(Datum);

bool should_hsh_run(bool, char *, char *);
void str_strcpy(char *, const char *);
void hsh_ringbuf_init(History *his, SessionHistEntry *entry, int size);

void get_ringbuf_to_heap(History *heap_buf, int filter_flag);
long get_history_fill_ratio(void);
void set_ringbuf_history_saved(void);
void hsh_timer(int flag);
void cal_report(TimestampTz aa, TimestampTz bb);
void stringinfo_replace(StringInfo buf, const char *placeholder, const char *replacement);
void hsh_read_template(StringInfo tmp, const char *key);
char *pg_getUserbyid(Oid roleid);

static void hsh_shmem_startup(void);
static void probe_sessions(History *hist_hdr, uint64 sample_id);
static inline bool history_is_sample(SessionHistEntry *entry);
static Timestamp timestamptz2timestamp(TimestampTz timestamp);
static void save_session_info(void);
static void write_to_systable(SessionHistEntry *beentry, Relation rel);

static MemoryContext hshCollectorContext = NULL;
MemoryContext   old_context_collector;

static MemoryContext hshWriterContext = NULL;
MemoryContext   old_context_writer;

/* 
 * Function:
 * 	timestamptz2timestamp()
 *
 * Description:
 * 	Get the current time, which is used as the creation time
 * 	or modification time of the pg object.
 *
 * Input:
 * 	TimestampTz
 *
 * Output:
 * 	Timestamp
 */
static Timestamp
timestamptz2timestamp(TimestampTz timestamp)
{
	Timestamp	result;
	struct pg_tm	tt,
			*tm = &tt;
	fsec_t		fsec;
	int		tz;

	if (TIMESTAMP_NOT_FINITE(timestamp))
		result = timestamp;
	else
	{
		if (timestamp2tm(timestamp, &tz, tm, &fsec, NULL, NULL) != 0)
			ereport(ERROR,
					(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
					 errmsg("timestamp out of range")));
		if (tm2timestamp(tm, 0, NULL, &result) != 0)
			ereport(ERROR,
					(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
					 errmsg("timestamp out of range")));
	}
	return result;
}

/*
 * Function:
 * 	history_time_check_hook()
 *
 * Description:
 * 	Check the GUC variable hsh.history_time
 *
 * Input:
 * 	GUC parameter
 *
 * Output:
 *	Bool(true/false)
 */
static bool
history_time_check_hook(int *newval, void **extra, GucSource source)
{
	int n = *newval;
	if ((n > 10) || (n < 1))
	{
		return false;
	}
	return true;
}

/*
 * Function:
 *      history_size_check_hook()
 *
 * Description:
 *      Check the GUC variable hsh.history_size
 *
 * Input:
 *      GUC parameter
 *
 * Output:
 *      Bool(true/false)
 */
static bool
history_size_check_hook(int *newval, void **extra, GucSource source)
{
	int n = *newval;
	if (n < 10000)
	{
		return false;
	}
	return true;
}

/*
 * Function:
 * 	hsh_register_collector(void)
 *
 * Description:
 * 	Register background worker for collector
 */
void
hsh_register_collector(void)
{
	BackgroundWorker worker;

	/* Set up background worker parameters */
	memset(&worker, 0, sizeof(worker));
	worker.bgw_flags = BGWORKER_SHMEM_ACCESS;
	worker.bgw_start_time = BgWorkerStart_ConsistentState;
	worker.bgw_restart_time = 5;
	worker.bgw_notify_pid = 0;
	snprintf(worker.bgw_library_name, BGW_MAXLEN, "hsh");
	snprintf(worker.bgw_function_name, BGW_MAXLEN, CppAsString(hsh_collector_main));
	snprintf(worker.bgw_name, BGW_MAXLEN, "Highgo Session History Collector");
	worker.bgw_main_arg = (Datum) 0;
	RegisterBackgroundWorker(&worker);
}

/*
 * Function:
 * 	hsh_register_writer(void)
 *
 * Description:
 * 	Register background worker for writer 
 */
void
hsh_register_writer(void)
{
	BackgroundWorker worker;

	/* Set up background worker parameters */
	memset(&worker, 0, sizeof(worker));
	worker.bgw_flags = BGWORKER_SHMEM_ACCESS | BGWORKER_BACKEND_DATABASE_CONNECTION;
	worker.bgw_start_time = BgWorkerStart_ConsistentState;
	worker.bgw_restart_time = 5;
	worker.bgw_notify_pid = 0;
	snprintf(worker.bgw_library_name, BGW_MAXLEN, "hsh");
	snprintf(worker.bgw_function_name, BGW_MAXLEN, CppAsString(hsh_writer_main));
	snprintf(worker.bgw_name, BGW_MAXLEN, "Highgo Session History Writer");
	worker.bgw_main_arg = (Datum) 0;
	RegisterBackgroundWorker(&worker);
}

/*
 * Function:
 *      hsh_shmem_size(void)
 *
 * Description:
 *      Estimate amount of shared memory needed.
 *
 * Output:
 * 	Size(alloc mem when startup)
 */
static Size
hsh_shmem_size(void)
{
	shm_toc_estimator e;
	Size		size;

	shm_toc_initialize_estimator(&e);
	shm_toc_estimate_chunk(&e, sizeof(History));
	shm_toc_estimate_chunk(&e, HISTORY_ENTRY_LEN * hsh_history_size);
	shm_toc_estimate_keys(&e, 2);
	size = shm_toc_estimate(&e);

	return size;
}

/*
 * Function:
 *      hsh_ringbuf_init()
 *
 * Description:
 *      Initilize the Ring buffer
 *
 * Input:
 * 	History - Head
 * 	SessionHistEntry - queue
 * 	size - length
 *
 * Output:
 *      void()
 */
void
hsh_ringbuf_init(History *hist, SessionHistEntry *entry, int size)
{

	LWLockInitialize(&hist->lock, LWLockNewTrancheId());

	hist->is_full = false;
	hist->max_size = size;
	hist->curr_index = 0;
	hist->sample_index = 0;
	hist->buffer = entry;
}

/*
 * Function:
 *      hsh_ringbuf_init()
 *
 * Description:
 *      Distribute shared memory.
 *
 */
static void
hsh_shmem_startup(void)
{
	bool	found;
	Size	segsize = hsh_shmem_size();
	shm_toc	*toc;
	void	*hsh_shmem;
	void	*ring_buf_body;

	hsh_shmem = ShmemInitStruct("hsh_collector", segsize, &found);

	if (!found)
	{
		long len = hsh_history_size * sizeof(SessionHistEntry);

		/* Create shared objects */
		toc = shm_toc_create(HSH_COLLECTOR_MAGIC, hsh_shmem, segsize);

		/* alloc ring_buf header part */
		ring_buf = shm_toc_allocate(toc, sizeof(History));
		shm_toc_insert(toc, 0, ring_buf);

		/* init ring_buf head part */
		memset(ring_buf, 0 , sizeof(History));

		/* alloc buffer part */
		ring_buf_body = shm_toc_allocate(toc, len);
		shm_toc_insert(toc, 1, ring_buf_body);

		/* init ring_buf body part*/
		memset(ring_buf_body, 0, len);

		hsh_ringbuf_init(ring_buf, ring_buf_body, hsh_history_size);

	}
	else
	{
		/* Attach to existing shared objects */
		toc = shm_toc_attach(HSH_COLLECTOR_MAGIC, hsh_shmem);
		ring_buf = (History *)shm_toc_lookup(toc, 0, false);
		ring_buf->buffer = (SessionHistEntry *)shm_toc_lookup(toc, 1, false);
	}

	if (prev_shmem_startup_hook)
		prev_shmem_startup_hook();
}

/*
 * Function:
 *      _PG_init(void)
 *
 * Description:
 *      Module initialize function: initialize info about HSH relation options.
 *
 */
void
_PG_init(void)
{

	/*
	 * get the shared_preload_libraries string
	 * then check if there including r_pg_pdr
	 * if not, raise an error
	 */
	if (NULL == shared_preload_libraries_string
			|| NULL == strstr(shared_preload_libraries_string, "hsh"))
	{
		elog(ERROR, "cannot create hsh extension after startup");
		return;
	}

	if (NULL == strstr(shared_preload_libraries_string, "pg_stat_statements"))
	{
		elog(ERROR, "cannot using Highgo Session History Extension as no pg_stat_statements in shared_preload_libraries");
		return;
	}

	/*
	 * In order to create our shared memory area, we have to be loaded via
	 * shared_preload_libraries.  If not, fall out 
	 */
	if (!process_shared_preload_libraries_in_progress)
		return;

	if (IsBinaryUpgrade)
		return;

	/*
	 * Request additional shared resources.  (These are no-ops if we're not in
	 * the postmaster process.)  We'll allocate or attach to the shared
	 * resources in hsh_shmem_startup().
	 */
        RequestAddinShmemSpace(hsh_shmem_size());

	/*
	 * Define (or redefine) custom GUC variables.
	 */
	DefineCustomBoolVariable("hsh.collector",
					"Whether to enabel session history tracking",
					NULL,
					&hsh_collector,
					false,
					PGC_SUSET,
					0,
					NULL,
					NULL,
					NULL);

	DefineCustomIntVariable("hsh.history_size",
					"Sets size of session history.",
					NULL,
					&hsh_history_size,
					100000,
					10000,
					INT_MAX,
					PGC_POSTMASTER,
					0,
					history_size_check_hook,
					NULL,
					NULL);

	DefineCustomIntVariable("hsh.history_time",
					"session history duration (in day).",
					NULL,
					&hsh_history_time,
					7,
					1,
					10,
					PGC_POSTMASTER,
					0,
					history_time_check_hook,
					NULL,
					NULL);

	DefineCustomStringVariable("hsh.database",
					"the database name which save the session history data.", 		
					NULL,
					&hsh_database,
					"highgo",
				   	PGC_POSTMASTER,
					0,
					NULL,
					NULL,
					NULL);

	DefineCustomIntVariable("hsh.writer_stat_interval",
					"the interval to save the hsh_statements.",
					NULL,
					&hsh_writer_stat_interval,
					1,
					1,
					10,
					PGC_SIGHUP,
					0,
					NULL,
					NULL,
					NULL);

	DefineCustomIntVariable("hsh.writer_ringbuf_interval",
					"the interval to save the hsh_ringbuf to hsh_history_data.",
					NULL,
					&hsh_writer_ringbuf_interval,
					10,
					1,
					100,
					PGC_SIGHUP,
					0,
					NULL,
					NULL,
					NULL);

	DefineCustomIntVariable("hsh.writer_clean_interval",
					"the interval to clean the hsh table data.",
					NULL,
					&hsh_writer_clean_interval,
					60,
					10,
					120,
					PGC_SIGHUP,
					0,
					NULL,
					NULL,
					NULL);

	/*
	 * Install hooks.
 	 */
	prev_shmem_startup_hook = shmem_startup_hook;
	shmem_startup_hook = hsh_shmem_startup;

	if (strcmp(hsh_database,"") == 0 )
	{

		hsh_database = "highgo";
	}

	/*
	 * registor two backend worker
	 * collector:
	 * 	 used to collect BE status
	 * writer:
	 * 	 used to flush the ring buf to the table and clean the expired table data
	 */
        hsh_register_collector();
        hsh_register_writer();

}

/*
 * memory copy for string
 */
void
str_strcpy(char *dest, const char *source)
{
	/* NB: We need to zero-pad the destination. */
	strncpy(dest, source, STR_LEN);
	dest[STR_LEN - 1] = '\0';
}

/*
 * Function:
 *      handle_sigterm()
 *
 * Description:
 *     Handle the sige term
 *
 */
static void
handle_sigterm(SIGNAL_ARGS)
{
	int	save_errno = errno;

	shutdown_requested = true;
	if (MyProc)
		SetLatch(&MyProc->procLatch);
	errno = save_errno;
}

/*
 * Function:
 *      probe_sessions()
 *
 * Description:
 *	Main function to collect session info into ring buff
 *	Read current state activity from backends and write them to ring buff
 *
 * Input:
 *	Ring buffer
 *	sample_id: id for this collecting
 *
 */
static void
probe_sessions(History *hist_hdr, uint64 sample_id)
{
	SockAddr        zero_clientaddr;
	int		count = 0; /* BUG#z6049 */

	TimestampTz now = GetCurrentTimestamp();
	int number = pgstat_fetch_stat_numbackends();

	if (number <= 0 )
		return;

	for (int curr_backend = 0; curr_backend < number; curr_backend++)
	{

		PGPROC *proc;
		uint32 raw_wait_event;

		LocalPgBackendStatus *local_beentry;
		PgBackendStatus *beentry;
		SessionHistEntry *hsh_array_slot = ring_buf->buffer + ring_buf->curr_index;

		SessionHistEntry item;
		memset(&item, 0, sizeof(SessionHistEntry));

		local_beentry = pgstat_fetch_stat_local_beentry(curr_backend);

		if (!local_beentry)
			continue;

		beentry = &local_beentry->backendStatus;

		/*
		 * skip some BEs
		 * if pid = 0 or pid = MyProc
		 * skpped them
		 */
		if (!beentry || beentry->st_procpid == 0 || beentry->st_procpid == MyProcPid)
		{
			continue;
		}

		/*
		 * skip some state BE
		 * if state is not active or fastpath function call state
		 * skipped them
		 */
		if (beentry->st_state != STATE_RUNNING && beentry->st_state != STATE_FASTPATH)
		{
			continue;
		}

		/*
		 * PgBackendStatus data
		 *
		 * sample_id is used for check if this loop is for sample
		 */
		item.sample_id = sample_id;
		item.ts = now;
		item.st_procpid = beentry->st_procpid;

		memcpy(&item.st_clientaddr, &beentry->st_clientaddr, sizeof(zero_clientaddr));
		str_strcpy(item.st_appname, beentry->st_appname);

		item.st_databaseid = beentry->st_databaseid;
		item.st_userid = beentry->st_userid;

		/* PROC->event data */
		proc = BackendPidGetProc(beentry->st_procpid);
		if (proc == NULL && (beentry->st_backendType != B_BACKEND))
		{
			/*
			 * For an auxiliary process, retrieve process info from
			 * AuxiliaryProcs stored in shared-memory.
			 */
			proc = AuxiliaryPidGetProc(beentry->st_procpid);
		}

		if (proc != NULL)
		{

			raw_wait_event = UINT32_ACCESS_ONCE(proc->wait_event_info);
			item.raw_wait_event = raw_wait_event;

		}
		else
		{
			raw_wait_event = 0;
		}
		item.raw_wait_event = raw_wait_event;

		/* query id */
		item.st_query_id = beentry->st_query_id;

		/* query type */
		item.st_cmd_type =  beentry->st_cmd_type;

		/* query phase */
		item.st_query_phase = beentry->st_query_phase;

                item.st_backendType = beentry->st_backendType;
                item.st_state = beentry->st_state;

                /* VACUUM, ANALYZE, CLUSTER,CREATE_INDEX, BASEBACKUP,COPY, INVALID */
                item.st_progress_command = beentry->st_progress_command;

		/* pg_stat_activity query column */
		if (beentry->st_activity_raw != NULL)
		{
			//int len;
			//len = Min(strlen(beentry->st_activity_raw), STR_LEN - 1);
			char *pos;
			size_t len;

			pos = strchr(beentry->st_activity_raw, ' ');
			if (pos)
				len = pos - beentry->st_activity_raw;
			else
				len = Min(strlen(beentry->st_activity_raw), STR_LEN - 1);
			memcpy(item.st_activity_raw, beentry->st_activity_raw, len);
			item.st_activity_raw[len] = '\0';
		}
		else
		{
			item.st_activity_raw[0] = '\0';
		}

		/* set which item will insert into table as sample */
		if (sample_id % 10 == 1)
			item.is_flushed_sample = true;

		memcpy(hsh_array_slot, &item, sizeof(item));

		ring_buf->curr_index ++;
		count++;	/* BUG#z6049 */

		if (ring_buf->curr_index >= ring_buf->max_size)
                {
                        ring_buf->curr_index = 0;
                        ring_buf->is_full = true;
                }
	}

	/*
 	 * update the ts if we really collected data
 	 */
	if (count != 0)	/* BUG#z6049 */
	{
		/* update the old_ts */
		if (ring_buf->old_ts == 0)
		{
			ring_buf->old_ts = now;
		}
		else if (ring_buf->is_full)
		{
			int64 next =  ring_buf->curr_index;
			ring_buf->old_ts = ring_buf->buffer[next].ts;
		}
		else
		{
			/* do nothing */
		}

		/* update the new_ts */
		ring_buf->new_ts = now;
	}
}

/*
 * Function:
 *      millisecs_diff()
 *
 * Description:
 *     Delta between two timestamps in milliseconds.
 *
 */
static int64
millisecs_diff(TimestampTz tz1, TimestampTz tz2)
{
	long		secs;
	int			microsecs;

	TimestampDifference(tz1, tz2, &secs, &microsecs);

	return secs * 1000 + microsecs / 1000;
}

/*
 * Function:
 *      should_hsh_run()
 *
 * Description:
 *     Check if the HSH worker running or not
 *
 */
bool
should_hsh_run(bool is_writer, char *sch, char *t)
{
	Oid             relOid;
	Oid             namespaceid;

	const char *track_activities;

	if (!hsh_collector)
		return false;

	/* recovery mode */
	if (RecoveryInProgress())
		return false;

	/* check track_activities config */
	track_activities = GetConfigOptionByName("track_activities", false, false);

	if (strcmp(track_activities,"on") != 0)
	{
		return false;
	}

	/* check shared memory initilized */
	if (ring_buf == NULL)
		 return false;

	/* check the hsh_history_data exist */
	if (is_writer)
	{
		StartTransactionCommand();
        	if(!sch)
        	{
                	namespaceid = LookupNamespaceNoError(HSH_HISTORY_NAMESPACE);
                	relOid = get_relname_relid(HSH_HISTORY_DATA, namespaceid);
        	}
        	else
        	{
                	namespaceid = LookupNamespaceNoError(sch);
                	relOid = get_relname_relid(t, namespaceid);
        	}
		CommitTransactionCommand();

		if (!OidIsValid(relOid))
			return false;
	}

	return true;

}

/*
 * Function:
 *      hsh_collector_setup_memcxt()
 *
 * Description:
 *     Create pgStatLocalContext, if not already done
 *
 */
static void
hsh_collector_setup_memcxt(void)
{
	if (!hshCollectorContext)
		hshCollectorContext = AllocSetContextCreate(TopMemoryContext,
							 "hsh collector context",
							 ALLOCSET_DEFAULT_SIZES);
}

/*
 * Function:
 *      hsh_collector_exit()
 *
 * Description:
 *     Exit collector worker
 *
 */
static void
hsh_collector_exit(int mode)
{
	MemoryContextSwitchTo(old_context_collector);
	MemoryContextReset(hshCollectorContext);
	proc_exit(mode);
}

/*
 * Function:
 *      hsh_writer_setup_memcxt(void)
 *
 * Description:
 *     Create pgStatLocalContext, if not already done
 *
 */
static void
hsh_writer_setup_memcxt(void)
{
	if (!hshWriterContext)
		hshWriterContext = AllocSetContextCreate(TopMemoryContext,
							 "hsh writer context",
							 ALLOCSET_DEFAULT_SIZES);
}

/*
 * Function:
 *	hsh_writer_exit()
 *
 * Description:
 *	Exit writer worker
 *
 */
static void
hsh_writer_exit(int mode)
{
	MemoryContextSwitchTo(old_context_writer);
	MemoryContextReset(hshWriterContext);
	proc_exit(mode);
}

/*
 * Function:
 *	hsh_collector_main()
 *
 * Description:
 *	Main routine of history collector
 *
 */
void
hsh_collector_main(Datum main_arg)
{
	TimestampTz 	current_ts,
			history_ts,
			begin_collector,
			end_collector;

	bool state;
	uint64 sample_id = 0;

	if (IsBinaryUpgrade)
	{
		ereport(LOG, (errmsg("Exit Highgo Session History collector in upgrade mode")));
		return;
	}

	/*
	 * Establish signal handlers.
	 *
	 * We want to respond to the ProcSignal notifications.  This is done in
	 * the upstream provided procsignal_sigusr1_handler, which is
	 * automatically used if a bgworker connects to a database.  But since our
	 * worker doesn't connect to any database even though it calls
	 * InitPostgres, which will still initializze a new backend and thus
	 * partitipate to the ProcSignal infrastructure.
 	 */
	pqsignal(SIGTERM, handle_sigterm);
	pqsignal(SIGHUP, SignalHandlerForConfigReload);
	pqsignal(SIGUSR1, procsignal_sigusr1_handler);

	BackgroundWorkerUnblockSignals();

	InitPostgres(NULL, InvalidOid, NULL, InvalidOid, NULL, false);
	SetProcessingMode(NormalProcessing);
	//BackgroundWorkerInitializeConnection(hsh_database, NULL, 0);

	hsh_collector_setup_memcxt();
	old_context_collector = MemoryContextSwitchTo(hshCollectorContext);

	/* Make hsh_collector recognisable in pg_stat_activity */
	pgstat_report_appname("Highgo Session History collector");

	ereport(DEBUG2, (errmsg("Highgo Session History collector started")));

	/* Start counting time for history samples */
	history_ts = GetCurrentTimestamp();

	if (RecoveryInProgress())
		hsh_collector_exit(1);

	while (!got_sigterm)
	{
		int		rc;
		int64		history_diff = 0;

		/* We need an explicit call for at least ProcSignal notifications. */
		CHECK_FOR_INTERRUPTS();

		if (ConfigReloadPending)
		{
			ConfigReloadPending = false;
			ProcessConfigFile(PGC_SIGHUP);
		}
		
		/* Calculate time to next sample for history or profile */
		current_ts = GetCurrentTimestamp();

		history_diff = millisecs_diff(history_ts, current_ts);

		/* Begin - BUG#z6093 */
		/*
		 * for special case: 
		 * if probe_session_info() spent more than 1s(1000ms)
		 * reset history_diff to zero
		 * to prevent waiting forever as (1000 - history_diff) is negative 
		 * so we can continue waiting 1s to collecotr the session info
		 */
		if (history_diff > 1000 )
		{
			history_diff = 0;
		}
		/* End - BUG#z6093 */

		/*
		 * Wait until next sample time or request to do something through
		 * shared memory.
		 */
		rc = WaitLatch(&MyProc->procLatch, WL_LATCH_SET | WL_TIMEOUT | WL_POSTMASTER_DEATH,
							(1000 - history_diff),
							PG_WAIT_EXTENSION);

		if (rc & WL_POSTMASTER_DEATH)
			proc_exit(1);

		ResetLatch(&MyProc->procLatch);
			
		state = should_hsh_run(false, NULL, NULL);

		/* Begin - BUG#z6093 */
		/* 
		 * begin to collector the probe session info
		 * if hsh.collector set to off
		 * we also set this timer
		 */
		begin_collector = GetCurrentTimestamp();
		/* End - BUG#z6093 */

		if (state)
		{

			int64	dura = 0;

			/* Acquire the share buf lock */
			LWLockAcquire(&ring_buf->lock, LW_EXCLUSIVE);

			/*
			 * why start a txn?
			 * because need to clean snapshot after calling pgstat_fetch_stat_numbackends()
			 */
			StartTransactionCommand();

			/* 
			 * collector the session info
			 * put them into the ring_buf 
			 * sample_id is used for check if this loop is for sample data
			 * after startup, for every 1s loop
			 * sample_id will be, 0,1,2,3...10,11,12,13...,20,21,22...
			 * when write the ringbuf data to the table
			 * just write 1,11,21,31....to the table
			 */
			probe_sessions(ring_buf, sample_id);
			sample_id ++;

			CommitTransactionCommand();

	        	/* Release the lock */
       			LWLockRelease(&ring_buf->lock);

			end_collector = GetCurrentTimestamp();

			/* cal the time for collector */
			dura = millisecs_diff(begin_collector, end_collector);

			/* Begin - BUG#z6093 */
			/*
			 * if probe_sessions() using more than 1s(1000ms)
			 * we should to record it
			 */
			if (dura > 1000)
			{
				elog(LOG, "Highgo Session History collector: collecting session info using more than 1s(1000ms)");
			}
			/* End - BUG#z6093 */
		}

		/* Begin - BUG#z6093 */
		/*
		 * history_ts is the begin time when we start collect
		 * if probe_sessions() spent 10ms
		 * we will wait (1000 - 10) ms
		 */
		history_ts = begin_collector;
		/* End - BUG#z6093 */

		/* Shutdown if requested */
		if (shutdown_requested)
			break;
	}

	ereport(LOG, (errmsg("Highgo Session History collector shutting down")));
	proc_exit(0);
}

/*
 * Function:
 *      history_is_sample()
 *
 * Description:
 *      return if the item is the sample data
 *
 * Output:
 * 	true for yes
 * 	false for no
 */
static inline bool
history_is_sample(SessionHistEntry *his_item)
{
	if (his_item->is_flushed_sample)
		return true;
	else
		return false;
}

/*
 * Function:
 *      get_ringbuf_to_heap()
 *
 * Description:
 *	Copy ring buffer to the heap
 *	Get the lock
 *	Memcpy()
 *	Release the lock
 *
 * Input:
 * 	new heap mem for ringbuf
 *
 * Output:
 * 	ringbuf data snapshot
 */
void
get_ringbuf_to_heap(History *heap_buf, int filter_flag)
{
	SessionHistEntry *hsh_array_slot;
	int count = 0;

	/* get the lock */
	LWLockAcquire(&ring_buf->lock, LW_EXCLUSIVE);

	count = ring_buf->is_full ? ring_buf->max_size : ring_buf->curr_index;

	if (count == 0)
	{
		LWLockRelease(&ring_buf->lock);
		heap_buf->curr_index = 0;
		heap_buf->max_size = 0;
		return;		
	}

	if (filter_flag == READ_ALL)
	{
		/*
		 * Read the shared memory
		 * flowing time elapses
		 * if not full, read from 0 to curr_index-1
		 * if full, read from curr_index to max_size, then read 0 to curr_index-1
		 *
		 */
		if (!ring_buf->is_full)
		{
			for (int i = 0; i < count; i ++)
			{
				hsh_array_slot = ring_buf->buffer + i;
				memcpy(heap_buf->buffer + heap_buf->curr_index, hsh_array_slot, HISTORY_ENTRY_LEN);
				heap_buf->curr_index++;
			}
		}
		/* buffer is full */
		else
		{
			/* copy curr_index to max_size */
			for (int i = ring_buf->curr_index; i < ring_buf->max_size; i++)
			{
				hsh_array_slot = ring_buf->buffer + i;
				memcpy(heap_buf->buffer + heap_buf->curr_index, hsh_array_slot, HISTORY_ENTRY_LEN);
				heap_buf->curr_index++;
			}

			/* copy 0 to curr_index */
			for (int i = 0; i < ring_buf->curr_index; i++)
			{
				hsh_array_slot = ring_buf->buffer + i;
				memcpy(heap_buf->buffer + heap_buf->curr_index, hsh_array_slot, HISTORY_ENTRY_LEN); 

				heap_buf->curr_index++;
			}
		}

		heap_buf->old_ts = ring_buf->old_ts;
		heap_buf->new_ts = ring_buf->new_ts;
	}
	else if (filter_flag == READ_SAMPLE)
	{
		/*
		 * from sample_index_saved to max_size
		 * filter the sample snap
		 */
		heap_buf->sample_index = ring_buf->curr_index;

		/*
		 * include two case:
		 *    1) buf is not full
		 *    2) buf is full, and sample_index_saved has cycled
		 */
		if (ring_buf->sample_index_saved <= ring_buf->curr_index)	/* BUG#z6126 */
		{
			elog(DEBUG2,"ring_buf->sample_index_saved = %d", ring_buf->sample_index_saved);
			elog(DEBUG2, "ring_buf->curr_index = %d", ring_buf->curr_index);

			/* from sample_index_saved to curr_index */
			for (int i = ring_buf->sample_index_saved; i < ring_buf->curr_index; i++)
			{
				hsh_array_slot = ring_buf->buffer + i;
				if (hsh_array_slot->is_flushed_sample)
				{
					memcpy(heap_buf->buffer + heap_buf->curr_index, hsh_array_slot, HISTORY_ENTRY_LEN);
					heap_buf->curr_index ++;
				}
				else
				{
					/* not copy no-sample entry */;
				}
			}

		}
		/*
		 * buf is full, but sample_index_saved is not cycled */
		else
		{
			elog(DEBUG2, "ring_buf->sample_index_saved = %d, ring_buf->curr_index = %d", ring_buf->sample_index_saved, ring_buf->curr_index);

			for (int i = ring_buf->sample_index_saved; i < ring_buf->max_size; i++)
			{
				hsh_array_slot = ring_buf->buffer + i;
				if (hsh_array_slot->is_flushed_sample)
				{
					memcpy(heap_buf->buffer + heap_buf->curr_index, hsh_array_slot, HISTORY_ENTRY_LEN);
					heap_buf->curr_index ++;
				}
				else
				{
					/* not copy no-sample entry */;
				}
			}

			/* from 0 to curr_index */
			for (int i = 0; i < ring_buf->curr_index; i++)
			{
				hsh_array_slot = ring_buf->buffer + i;
				if (hsh_array_slot->is_flushed_sample)
				{
					memcpy(heap_buf->buffer + heap_buf->curr_index, hsh_array_slot, HISTORY_ENTRY_LEN);
					heap_buf->curr_index ++;
				}
				else
				{
					/* not copy no-sample entry */;
				}
			}
		}
	}

	LWLockRelease(&ring_buf->lock);
}

/*
 * Function:
 * 	pg_getUserbyid(Oid roleid)
 *
 * Description:
 * 	refer pg_get_userbyid(PG_FUNCTION_ARGS)
 */
char *
pg_getUserbyid(Oid roleid)
{
	HeapTuple	roletup;
	char       *result;

	roletup = SearchSysCache1(AUTHOID, ObjectIdGetDatum(roleid));
	if (HeapTupleIsValid(roletup))
	{
		result = pstrdup(NameStr(((Form_pg_authid) GETSTRUCT(roletup))->rolname));
		
		ReleaseSysCache(roletup);
	}
	else
		result = NULL;

	return result;

}

/*
 * Function:
 *      GetTuple()
 *
 * Description:
 *      format the SessionHistEntry struct to values[] and null[]
 *      hsh_ringbuf_raw()
 *      writer_to_systable()
 *
 * Input:
 * 	SessionHistEntry item
 *
 * Output:
 * 	values[] and nulls[]
 *
 */
static void
GetTuple(Datum* values, int val_len, bool* nulls, int null_len, SessionHistEntry *beentry)
{
	SockAddr	zero_clientaddr;
	const char	*wait_event_type = NULL;
	const char	*wait_event = NULL;

	Assert(val_len == Natts_hsh_history && null_len == Natts_hsh_history);

	values[NATTS_TS] = TimestampTzGetDatum(beentry->ts);
	values[NATTS_PID] = Int64GetDatum(beentry->st_procpid);

	/* clenit addr */
	memset(&zero_clientaddr, 0, sizeof(zero_clientaddr));
	if (memcmp(&(beentry->st_clientaddr), &zero_clientaddr,
                                           sizeof(zero_clientaddr)) == 0)
	{
		nulls[NATTS_ADDR] = true;
	}
	else
	{
		if (beentry->st_clientaddr.addr.ss_family == AF_INET
#ifdef HAVE_IPV6
			|| beentry->st_clientaddr.addr.ss_family == AF_INET6
#endif
		)
		{
			char	remote_host[NI_MAXHOST];
			char	remote_port[NI_MAXSERV];
			int	ret;

			remote_host[0] = '\0';
			remote_port[0] = '\0';
			ret = pg_getnameinfo_all(&beentry->st_clientaddr.addr,
						beentry->st_clientaddr.salen,
						remote_host, sizeof(remote_host),
						remote_port, sizeof(remote_port),
						NI_NUMERICHOST | NI_NUMERICSERV);

			if (ret == 0)
			{
				clean_ipv6_addr(beentry->st_clientaddr.addr.ss_family, remote_host);
				values[NATTS_ADDR] = DirectFunctionCall1(inet_in,
							CStringGetDatum(remote_host));
			}
			else
			{
				nulls[NATTS_ADDR] = true;
			}
		}
		else if (beentry->st_clientaddr.addr.ss_family == AF_UNIX)
		{
			/*
			 * Unix sockets always reports NULL for host and -1 for
			 * port, so it's possible to tell the difference to
			 * connections we have no permissions to view, or with
			 * errors.
			 */
			nulls[NATTS_ADDR] = true;
		}
		else
		{
			/* Unknown address type, should never happen */
			nulls[NATTS_ADDR] = true;
		}
	}

	/* application name */
	if (strlen(beentry->st_appname))
		values[NATTS_APPNAME] = CStringGetTextDatum(beentry->st_appname);
	else
		nulls[NATTS_APPNAME] = true;

	/* db id */
	if (beentry->st_databaseid != InvalidOid)
	{
		char	*dbname = NULL;

		values[NATTS_DBID] = ObjectIdGetDatum(beentry->st_databaseid);

		/* db name */
		dbname = get_database_name(beentry->st_databaseid);
		if (strlen(dbname) != 0 )
		{
			values[NATTS_DBNAME] = CStringGetTextDatum(dbname);
		}
		else
		{
			nulls[NATTS_DBNAME] = true;
		}
	}
	else
	{
		nulls[NATTS_DBID] = true;
		nulls[NATTS_DBNAME] = true;
	}

	/* user id */
	if (beentry->st_userid != InvalidOid)
	{
		char 	*rolename = NULL;

		values[NATTS_USEID] = ObjectIdGetDatum(beentry->st_userid);

		/* user name */
		rolename = pg_getUserbyid(beentry->st_userid);

		if (strlen(rolename) != 0)
		{
			values[NATTS_USERNAME] = CStringGetTextDatum(rolename);	
		}
		else
		{
			nulls[NATTS_USERNAME] = true;
		}	
	}
	else
	{
		nulls[NATTS_USEID] = true;
		nulls[NATTS_USERNAME] = true;
	}	


	/* event */
	wait_event_type = pgstat_get_wait_event_type(beentry->raw_wait_event);
	wait_event = pgstat_get_wait_event(beentry->raw_wait_event);
	if (wait_event_type)
		values[NATTS_EVENTTYPE] = CStringGetTextDatum(wait_event_type);

	if (wait_event)
		values[NATTS_EVENT] = CStringGetTextDatum(wait_event);

	if (!wait_event_type && !wait_event)
	{
		values[NATTS_EVENTTYPE] = CStringGetTextDatum("OnCPU");
		values[NATTS_EVENT] = CStringGetTextDatum("CPU or Wait On CPU");
	}

	/* query_id */
	if (beentry->st_query_id == 0)
		nulls[NATTS_QUERYID] = true;
	else
		values[NATTS_QUERYID] = UInt64GetDatum(beentry->st_query_id);

	/* cmd type */
	switch (beentry->st_cmd_type)
	{
		case CMD_UNKNOWN:
			values[NATTS_CMDTYPE] = CStringGetTextDatum("unkown");
			break;
		case CMD_SELECT:
			values[NATTS_CMDTYPE] = CStringGetTextDatum("select");
			break;
		case CMD_UPDATE:
			values[NATTS_CMDTYPE] = CStringGetTextDatum("update");
			break;
		case CMD_INSERT:
			values[NATTS_CMDTYPE] = CStringGetTextDatum("insert");
			break;
		case CMD_DELETE:
			values[NATTS_CMDTYPE] = CStringGetTextDatum("delete");
			break;
		case CMD_MERGE:
			values[NATTS_CMDTYPE] = CStringGetTextDatum("merge");
			break;
		case CMD_UTILITY:
			values[NATTS_CMDTYPE] = CStringGetTextDatum("utility");
			break;
		case CMD_NOTHING:
			values[NATTS_CMDTYPE] = CStringGetTextDatum("nothing");
			break;
	}

	/* query phase */
	switch (beentry->st_query_phase)
	{
		case QUERY_PHASE_UNKNOWN:
			values[NATTS_QUERYPHASE] = CStringGetTextDatum("other");
			break;
		case QUERY_PHASE_PARSER:
			values[NATTS_QUERYPHASE] = CStringGetTextDatum("parse");
			break;
		case QUERY_PHASE_BIND:
			values[NATTS_QUERYPHASE] = CStringGetTextDatum("bind");
			break;
		case QUERY_PHASE_PLAN:
			values[NATTS_QUERYPHASE] = CStringGetTextDatum("plan");
			break;
		case QUERY_PHASE_EXEC:
			values[NATTS_QUERYPHASE] = CStringGetTextDatum("exec");
			break;
	}

	/* st_backendType */
	if (beentry->st_backendType == B_BG_WORKER)
	{
		const char *bgw_type;
		bgw_type = GetBackgroundWorkerTypeByPid(beentry->st_procpid);
		if (bgw_type)
			values[NATTS_BACKTYPE] = CStringGetTextDatum(bgw_type);
		else
			nulls[NATTS_BACKTYPE] = true;
	}
	else
	{
		values[NATTS_BACKTYPE] = CStringGetTextDatum(GetBackendTypeDesc(beentry->st_backendType));
	}

	/* st_state */
	switch (beentry->st_state)
	{
		case STATE_IDLE:
			values[NATTS_SATATE] = CStringGetTextDatum("idle");
			break;
		case STATE_RUNNING:
			values[NATTS_SATATE] = CStringGetTextDatum("active");
			break;
		case STATE_IDLEINTRANSACTION:
			values[NATTS_SATATE] = CStringGetTextDatum("idle in transaction");
			break;
		case STATE_FASTPATH:
			values[NATTS_SATATE] = CStringGetTextDatum("fastpath function call");
			break;
		case STATE_IDLEINTRANSACTION_ABORTED:
			values[NATTS_SATATE] = CStringGetTextDatum("idle in transaction (aborted)");
			break;
		case STATE_DISABLED:
			values[NATTS_SATATE] = CStringGetTextDatum("disabled");
			break;
		case STATE_UNDEFINED:
			nulls[NATTS_SATATE] = true;
			break;
	}

	/* is_sample */
	values[NATTS_ISSAMPLE] = BoolGetDatum(beentry->is_flushed_sample);

	/* TEXT */
	if (strlen(beentry->st_activity_raw) != 0)
		values[NATTS_ACTIVITY_RAW] = CStringGetTextDatum(beentry->st_activity_raw);
	else
		nulls[NATTS_ACTIVITY_RAW] = true;
}

PG_FUNCTION_INFO_V1(hsh_ringbuf_raw);


/*
 * Function:
 *      hsh_ringbuf_raw()
 *
 * Description:
 *      Read the Ringbuf item 
 *      construct the tuple 
 *      return to the user
 *      Read the buf from oldest to the latest
 *
 * Output:
 * 	Tuple
 */
Datum
hsh_ringbuf_raw(PG_FUNCTION_ARGS)
{
	FuncCallContext	*funcctx;
	History		*history_heap;
	History		*params;

	if (ring_buf == NULL)
	{
		elog(ERROR, "hsh must be loaded via shared_preload_libraries");
	}

	if (SRF_IS_FIRSTCALL())
	{
		MemoryContext oldcontext;
		TupleDesc	tupdesc;

		funcctx = SRF_FIRSTCALL_INIT();
		oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

		/* copy the ring buffer to the heap */
		history_heap = (History *) palloc0(sizeof(History));
		history_heap->buffer = (SessionHistEntry *) palloc0( hsh_history_size * HISTORY_ENTRY_LEN);
		
		get_ringbuf_to_heap(history_heap, READ_ALL);

		funcctx->user_fctx = history_heap;
		funcctx->max_calls = history_heap->curr_index;

		/* Build a tuple descriptor for our result type */
		if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
			elog(ERROR, "return type must be a row type");

		funcctx->tuple_desc = BlessTupleDesc(tupdesc);

		MemoryContextSwitchTo(oldcontext);
	}

	/* stuff done on every call of the function */
	funcctx = SRF_PERCALL_SETUP();
	params = (History*) funcctx->user_fctx;
	
	if (funcctx->call_cntr < funcctx->max_calls)
	{
		/* for each row */
		Datum		values[Natts_hsh_history];
		bool		nulls[Natts_hsh_history];
		HeapTuple	tuple;
		SessionHistEntry	*item;

		item = params->buffer + funcctx->call_cntr;
		MemSet(values, 0, sizeof(values));
		MemSet(nulls, 0, sizeof(nulls));

		GetTuple(values, Natts_hsh_history, nulls, Natts_hsh_history, item);

		tuple = heap_form_tuple(funcctx->tuple_desc, values, nulls);

		SRF_RETURN_NEXT(funcctx, HeapTupleGetDatum(tuple));
	}
	else
	{
		/* nothing left */
		SRF_RETURN_DONE(funcctx);
	}
}

/*
 * Function:
 *      get_history_fill_ratio(void)
 *
 * Description:
 *      check the fill ratio of the ring buf 
 *	if ring is not full, real_count/max
 *	if ring is full, (smaple_reset/max)
 *
 * Output:
 *      ratio
 */
long
get_history_fill_ratio(void)
{
	/* if not full, return real_count / size */
	if (!ring_buf->is_full)
	{
		return (long)((ring_buf->curr_index  * 100) / hsh_history_size); /* BUG#z6126 */
	}
	/* if full, return (not sample size) / size */
	else
	{
		/* Begin - BUG#z6126 */
		int last_sample_to_end;

		/* sample_index_saved has cycled */
		if (ring_buf->sample_index_saved <= ring_buf->curr_index)
		{
			last_sample_to_end = ring_buf->curr_index - ring_buf->sample_index_saved;
		}
		/* sample_index_saved is not cycled, but curr_index has cycled */
		else
		{
			last_sample_to_end = hsh_history_size - ring_buf->sample_index_saved + ring_buf->curr_index;
		}

		return (long)((last_sample_to_end * 100) / hsh_history_size);
		/* End - BUG#Z6126 */
	}
	
}

/*
 * Function:
 *      write_to_systable()
 *
 * Description:
 *      insert the session entry into a the table
 *
 * Input:
 * 	SessionHistEntry item
 * 	Relation rel
 *
 * Output:
 *      put the data into table
 */
static void
write_to_systable(SessionHistEntry *beentry, Relation rel)
{
	Datum values[Natts_hsh_history];
	bool nulls[Natts_hsh_history];
	HeapTuple tuple;
	BlockNumber block;
	OffsetNumber offnum;

	/* Begin - BUG#z6093 */
	List *indexList;
	ListCell * index;
	/* End - BUG#z6093 */

	/* Insert a line to the table */
	memset(values, 0, sizeof(values));
	memset(nulls, 0, sizeof(nulls));

	/* get active session profile tuple */
	GetTuple(values, Natts_hsh_history, nulls, Natts_hsh_history, beentry);

	tuple = heap_form_tuple(RelationGetDescr(rel), values, nulls);
	simple_heap_insert(rel, tuple);

	/* Begin - BUG#z6093 */
	indexList = RelationGetIndexList(rel);
	foreach(index, indexList)
	{
		Datum		*index_values;
		bool		*index_nulls;
		TupleTableSlot	*slot;
		EState		*estate;
		ExprContext	*econtext;

		Oid indexOid = lfirst_oid(index);
		Relation idexRel = index_open(indexOid, RowExclusiveLock);
		IndexInfo *indexInfo = BuildIndexInfo(idexRel);

		estate = CreateExecutorState();
		slot = MakeSingleTupleTableSlot(RelationGetDescr(rel), &TTSOpsHeapTuple);

		ExecStoreHeapTuple(tuple, slot, false);

		index_values = palloc(sizeof(Datum) * indexInfo->ii_NumIndexAttrs);
		index_nulls = palloc(sizeof(bool) * indexInfo->ii_NumIndexAttrs);

		econtext = GetPerTupleExprContext(estate);
		econtext->ecxt_scantuple = slot;

		FormIndexDatum(indexInfo, slot, estate, index_values, index_nulls);

		index_insert(idexRel, index_values, index_nulls,
					&(tuple->t_self), rel, indexInfo->ii_Unique, false, NULL);

		ExecDropSingleTupleTableSlot(slot);
		FreeExecutorState(estate);

		index_close(idexRel, RowExclusiveLock);
		pfree(index_values);
		pfree(index_nulls);

	}
	list_free(indexList);
	/* End - BUG#z6093 */

	/* DEBUG log */
        block = ItemPointerGetBlockNumber(&tuple->t_self);
        offnum = ItemPointerGetOffsetNumber(&tuple->t_self);
	elog(DEBUG2, "insert tuple at block %u, offset %u", block, offnum);
	
        heap_freetuple(tuple);
}

/*
 * Function:
 *      save_session_info(void)
 *
 * Description:
 *      Save the session history buffer into the table
 *      copy the buffer into new heap memory
 *	call write_to_systable() to hsh_history_data table for sample item
 *
 * Input:
 *
 * Output:
 *      ratio
 */
static void
save_session_info(void)
{
	int		count;	/* BUG#z6126 */
	History		*history_heap;
	Oid		namespaceid;
	Oid		relOid;
	Relation	rel;
	
	/* copy the ring buffer to the heap */
	history_heap = (History *) palloc0(sizeof(History));
	history_heap->buffer = (SessionHistEntry *) palloc0( hsh_history_size * HISTORY_ENTRY_LEN);

	get_ringbuf_to_heap(history_heap, READ_SAMPLE);

	count = history_heap->curr_index;
	Assert(count>=0);

	PG_TRY();
	{
		namespaceid = LookupNamespaceNoError(HSH_HISTORY_NAMESPACE);
		relOid = get_relname_relid(HSH_HISTORY_DATA, namespaceid);
		rel = table_open(relOid, RowExclusiveLock);

		for (int i = 0; i < count; i++)
		{
			SessionHistEntry  *hsh_history_slot = history_heap->buffer + i;
			if (hsh_history_slot->is_flushed_sample)
			{
				write_to_systable(hsh_history_slot, rel);
			}
		}

		table_close(rel, RowExclusiveLock);

		/* update the ring buf sample_index_saved */
		LWLockAcquire(&ring_buf->lock, LW_EXCLUSIVE);
		ring_buf->sample_index_saved = history_heap->sample_index;
		LWLockRelease(&ring_buf->lock);

		pfree(history_heap->buffer);
		pfree(history_heap);

	}
	PG_CATCH();
	{
		pfree(history_heap->buffer);
		pfree(history_heap);

		elog(INFO,"Flush Session History Info into session_history_data table failed");
		PG_RE_THROW();
	}
	PG_END_TRY();
}

/*
 * Function:
 *      hsh_writer_main()
 *
 * Description:
 *      Main routine of history Writer
 *      Flush the ring buff into hsh_history_data
 *      Insert the SQL info into hsh_statements from pg_stat_statements
 *      Delete the expired data from hsh_history_data and hsh_statements table
 *
 */
void
hsh_writer_main(Datum main_arg)
{

	TimestampTz	last_history_saved = 0,
			last_history_clean = 0,
			last_stat_saved = 0,
			last_stat_clean = 0;
	TimestampTz	history_ts,
			current_ts;

	StringInfoData query;

	/*
 	 * call back functions for SIG
 	 */
	pqsignal(SIGTERM, handle_sigterm);
	pqsignal(SIGHUP, SignalHandlerForConfigReload);
	pqsignal(SIGUSR1, procsignal_sigusr1_handler);
	BackgroundWorkerUnblockSignals();

	/*
 	 * create a new memory context for backend
 	 */
	hsh_writer_setup_memcxt();
        old_context_writer = MemoryContextSwitchTo(hshWriterContext);

	/* 
	 * Connect to a database
	 */
	BackgroundWorkerInitializeConnection(hsh_database, NULL, 0);

	/* Make hsh writer recognisable in pg_stat_activity */
	pgstat_report_appname("Highgo Session History Writer");
	ereport(DEBUG2, (errmsg("Highgo Session History Writer started")));

	if (RecoveryInProgress())
                hsh_writer_exit(1);

	history_ts = GetCurrentTimestamp();

	while (!got_sigterm)
	{
		int             rc;
		int		ret;
		TimestampTz now;
		int history_diff;

		initStringInfo(&query);

                /* We need an explicit call for at least ProcSignal notifications. */
                CHECK_FOR_INTERRUPTS();

                if (ConfigReloadPending)
                {
                        ConfigReloadPending = false;
                        ProcessConfigFile(PGC_SIGHUP);
                }

		pgstat_report_activity(STATE_IDLE, NULL);

		current_ts = GetCurrentTimestamp();
		history_diff = millisecs_diff(history_ts, current_ts);

		/* Begin - BUG#z6093 */
		/* if something wrong, reset the loop for 10s */
		if (history_diff > WRITER_CHECK)
		{
			history_diff  = 0;
			elog(LOG,"Highgo Session History Writer,loop duration is less than zero, reset the loop to 10s");
		}
		/* End - BUG#z6093 */

		/*
		 * wakeup every 10s to check if have work to do
		 */
		rc = WaitLatch(MyLatch, WL_LATCH_SET | WL_TIMEOUT | WL_POSTMASTER_DEATH,
                                        (WRITER_CHECK - history_diff),
                                        PG_WAIT_EXTENSION);

		ResetLatch(MyLatch);

		if (rc & WL_POSTMASTER_DEATH)
			proc_exit(1);

		/* Begin - BUG#z6093 */
		/* we wakeup, refresh the history_ts */
		history_ts = GetCurrentTimestamp();
		/* End - BUG#z6093 */

		if (rc & WL_LATCH_SET)
			goto loop_continue;

		elog(DEBUG2,"Highgo Session History Writer, waitlatch return %d",rc);

		/* ring_buf->count != 0 */
		if (should_hsh_run(true, NULL, NULL) && ring_buf != NULL )
		{
			SetCurrentStatementStartTimestamp();
			StartTransactionCommand();
			if ((ret = SPI_connect()) < 0)
			{
				/* internal error */
				elog(ERROR, "SPI connect to failure - returned %d", ret);
			}
			pgstat_report_activity(STATE_RUNNING, NULL);
			PushActiveSnapshot(GetTransactionSnapshot());
			now = GetCurrentTimestamp();

			/*
			 * 10 min or 80%
			 * read the data from ring_buff, save to table
			 */
			if ((now - last_history_saved) >= (hsh_writer_ringbuf_interval * USECS_PER_MINUTE) || get_history_fill_ratio() >= 80)	/* BUG#z6126 */
			{
				save_session_info();
				last_history_saved = GetCurrentTimestamp();
			}

                        /*
                         * 1 min
                         * save query_id and query from pg_stat_statements to hsh_statements
                         */
                        if ((now - last_stat_saved) >= (hsh_writer_stat_interval * USECS_PER_MINUTE))
                        {
				resetStringInfo(&query);
				appendStringInfo(&query, "INSERT INTO %s.%s (query_id, query) ",
							HSH_HISTORY_NAMESPACE,
							HSH_STATEMENTS);
				appendStringInfo(&query, "SELECT queryid, query FROM public.pg_stat_statements ON conflict do nothing");

                                if (SPI_execute(query.data, false, 0) != SPI_OK_INSERT)
				{
					elog(ERROR, "HSH collector, SPI_exec failed: %s", query.data);
				}
                                last_stat_saved = GetCurrentTimestamp();
                        }

			/*
			 * for every hour, clean the history data table
			 * clean the data who has expired 7 days
			 */
			if ((now - last_history_clean) >= (hsh_writer_clean_interval * USECS_PER_MINUTE))
			{
				resetStringInfo(&query);
				appendStringInfo(&query, "DELETE FROM %s.%s WHERE ts < (CURRENT_TIMESTAMP - pg_catalog.concat(%d, 'days')::INTERVAL);",
						HSH_HISTORY_NAMESPACE,
						HSH_HISTORY_DATA,
						hsh_history_time);

				ret = SPI_execute(query.data, false, 0);
				if (ret != SPI_OK_DELETE)
				{
					elog(ERROR, "HSH collector, SPI_exec failed %d: %s",ret, query.data);
				}
				last_history_clean = GetCurrentTimestamp();
			}

			/*
			 * for every hour, check if need clean hsh_statments table
			 */
			if ((now - last_stat_clean) >= (hsh_writer_clean_interval * USECS_PER_MINUTE))
			{
				resetStringInfo(&query);
				appendStringInfo(&query, "DELETE FROM %s.%s s ",
								HSH_HISTORY_NAMESPACE,
								HSH_STATEMENTS);
				appendStringInfo(&query, "WHERE NOT EXISTS (SELECT 1 FROM %s.%s d WHERE s.query_id = d.query_id)",
								HSH_HISTORY_NAMESPACE,
								HSH_HISTORY_DATA);
				appendStringInfo(&query, " AND NOT EXISTS(SELECT 1 FROM %s.hsh_ringbuf_raw() m WHERE s.query_id = m.query_id);",
								HSH_HISTORY_NAMESPACE);

				ret = SPI_execute(query.data, false, 0);
				if (ret != SPI_OK_DELETE)
				{
					elog(ERROR, "HSH collector, SPI_exec failed %d : %s", ret, query.data);
				}
				last_stat_clean = GetCurrentTimestamp();
			}

			SPI_finish();
			PopActiveSnapshot();
			CommitTransactionCommand();
			
		}

loop_continue:

		/* Shutdown if requested */
		if (shutdown_requested)
			break;

	}

	MemoryContextSwitchTo(old_context_writer);
	MemoryContextReset(hshWriterContext);
	proc_exit(1);
}

/*
 * Function:
 *      hsh_get_ringbuf_start()
 *
 * Description:
 *      Get the timestamptz for the oldest item
 *
 */
static TimestampTz
hsh_get_ringbuf_start(void)
{
	TimestampTz tmp = 0;

	if (ring_buf != NULL)
	{
		LWLockAcquire(&ring_buf->lock,0);
		tmp = ring_buf->old_ts;
		LWLockRelease(&ring_buf->lock);

		return tmp;
	}

	elog(ERROR, "Highgo Session History: shared buff did not initailized");
	return tmp;
}

/*
 * Function:
 *      hsh_get_ringbuf_end()
 *
 * Description:
 *      Get the timestamptz for the latest item
 *
 */
static TimestampTz
hsh_get_ringbuf_end(void)
{
	TimestampTz tmp = 0;

	if (ring_buf != NULL)
	{
		LWLockAcquire(&ring_buf->lock,0);
		tmp = ring_buf->new_ts;
		LWLockRelease(&ring_buf->lock);

		return tmp;
	}

	elog(ERROR, "Highgo Session History: shared buff did not initailized");
	return tmp;
}

/*
 * Function:
 *      write_mem_to_tmp_table()
 *
 * Description:
 *      For cal report
 *      insert the related ring buff data into temporary table
 *      'the related ring buff data' is all item from a to b
 *
 * Output:
 * 	the data in the tmp table
 */
static void
write_mem_to_tmp_table(TimestampTz a, TimestampTz b, Relation rel)
{
	Size count;
	History *history_heap;

	/* copy the ring buffer to the heap */
	history_heap = (History *) palloc0(sizeof(History));
	history_heap->buffer = (SessionHistEntry *) palloc0( hsh_history_size * HISTORY_ENTRY_LEN);

	get_ringbuf_to_heap(history_heap, READ_ALL);

	if (history_heap->is_full)
	{
		count = history_heap->max_size - history_heap->sample_index + history_heap->curr_index;
	}
	else
	{
		count = history_heap->curr_index - history_heap->sample_index;
	}
	Assert(count>=0);

	PG_TRY();
	{
		for (int i = 0; i < count; i++)
		{
			SessionHistEntry  *hsh_history_slot = history_heap->buffer + history_heap->sample_index + i;
			if ( a <= hsh_history_slot->ts && hsh_history_slot->ts <= b )
			{
				write_to_systable(hsh_history_slot, rel);
			}
		}

	}
	PG_CATCH();
	{
		elog(INFO,"Flush Session History Info into table failed");
		pfree(history_heap->buffer);
		pfree(history_heap);
		PG_RE_THROW();
	}
	PG_END_TRY();
	pfree(history_heap->buffer);
	pfree(history_heap);
}

/*
 * Function:
 *      write_db_to_tmp_table()
 *
 * Description:
 *      For cal report
 *      insert the related data from hsh_history_data into temporary table
 *      using SQL insert into xxx select * from xxx where ts>=a and ts<b
 *
 * Output:
 * 	the data in the tmp table
 */
static void
write_db_to_tmp_table(TimestampTz a, TimestampTz b)
{
	StringInfoData	querybuf;
	int	ret;
	char	*a_str;
	char	*b_str;

	a_str = pstrdup(timestamptz_to_str(a));
	b_str = pstrdup(timestamptz_to_str(b));

	initStringInfo(&querybuf);
	appendStringInfo(&querybuf, "insert into %s select * from %s.%s where ts>='%s' and ts<='%s';",
					HSH_RESULT,
					HSH_HISTORY_NAMESPACE,
					HSH_HISTORY_DATA,
					a_str,
					b_str);
	ret = SPI_exec(querybuf.data, 0);
        if (ret != SPI_OK_INSERT)
	{
		elog(ERROR, "SPI_exec failed: error code %d, query: %s", ret, querybuf.data);
	}

	elog(DEBUG2,"write_db_to_tmp_table() insert %llu tuples", (long long unsigned int)SPI_processed);

	pfree(a_str);
	pfree(b_str);
	pfree(querybuf.data);
}

/*
 * Function:
 *      write_mem_db_to_tmp_table()
 *
 * Description:
 *      For cal report
 *      insert the related ring buff data and history table data into temporary table
 *      'the related ring buff data' is the item from a to a.xx, and just the sample item
 *      'history table data' is the data from history table from a.xx to b 
 *
 * Output:
 *      the data in the tmp table
 */
static void
write_mem_db_to_tmp_table(TimestampTz b, TimestampTz e, Relation rel)
{
	StringInfoData	querybuf;
        int		ret;
	char		*b_str;
	char		*mem_start_str;
	Size		count;

	TimestampTz	mem_start;
	History		*history_heap;

	/* get the ringbuff last timestamp */
	mem_start = hsh_get_ringbuf_start();
	Assert(e > b);

	/* read the shared buff to the heap */
        history_heap = (History *) palloc0(sizeof(History));
        history_heap->buffer = (SessionHistEntry *) palloc0( hsh_history_size * HISTORY_ENTRY_LEN);

        get_ringbuf_to_heap(history_heap, READ_ALL);

        if (history_heap->is_full)
        {
                count = history_heap->max_size - history_heap->sample_index + history_heap->curr_index;
        }
        else
        {
                count = history_heap->curr_index - history_heap->sample_index;
        }
        Assert(count>=0);

	/* write mem data to tmp table */
	PG_TRY();
        {
                for (int i = 0; i < count; i++)
                {
                        SessionHistEntry  *hsh_history_slot = history_heap->buffer + history_heap->sample_index + i;
                        if ( b <= hsh_history_slot->ts && hsh_history_slot->ts <= e && hsh_history_slot->is_flushed_sample)
                        {
                                write_to_systable(hsh_history_slot, rel);
                        }
                }

        }
        PG_CATCH();
        {
                elog(INFO,"Flush Session History Info into table failed");
                pfree(history_heap->buffer);
                pfree(history_heap);
                PopActiveSnapshot();
                AbortCurrentTransaction();
                PG_RE_THROW();
        }
        PG_END_TRY();
        pfree(history_heap->buffer);
        pfree(history_heap);

	/* write db data to tmp table */
	b_str = pstrdup(timestamptz_to_str(b));	
	mem_start_str = pstrdup(timestamptz_to_str(mem_start));
	initStringInfo(&querybuf);
        appendStringInfo(&querybuf, "insert into %s select * from %s.%s where ts>='%s' and ts<='%s';",
					HSH_RESULT,
					HSH_HISTORY_NAMESPACE,
					HSH_HISTORY_DATA,
					b_str,
					mem_start_str);

        ret = SPI_exec(querybuf.data, 0);
        if (ret != SPI_OK_INSERT)
        {
                elog(ERROR, "SPI_exec failed: error code %d, query: %s", ret, querybuf.data);
        }

	pfree(b_str);
	pfree(mem_start_str);
	pfree(querybuf.data);
}

/*
 * Function:
 *	stringinfo_replace()
 *
 * Description:
 *      Replace the place holder to special buffer
 *
 */
void
stringinfo_replace(StringInfo buf, const char *placeholder, const char *replacement)
{
	char *pos;
	StringInfoData newbuf;

	initStringInfo(&newbuf);

	pos = buf->data;
	while (true)
	{
		char *p = strstr(pos, placeholder);
		if (p == NULL)
		{
			/* append then rest part */
			appendStringInfoString(&newbuf, pos);
			break;
		}

		/* first append before placehelder */
		appendBinaryStringInfo(&newbuf, pos, p - pos);

		/* append the replacement */
		appendStringInfoString(&newbuf, replacement);

		/* move to the next point */
		pos = p + strlen(placeholder);
	}

	/* copy the buffer to the old one */
	resetStringInfo(buf);
	appendStringInfoString(buf, newbuf.data);

	pfree(newbuf.data);
}

/*
 * Function:
 *      hsh_read_template()
 *
 * Description:
 *      Read the template for for the report by index
 *
 */
void
hsh_read_template(StringInfo tmp, const char *key)
{
	appendStringInfo(tmp, "%s", key);
}

/*
 * Function:
 *      set_data_source_type()
 *
 * Description:
 *      Check the data source type from b_tmtz to e_tmtz
 *      1. check the memory ringbuf
 *      2. check the hsh_history_data table
 *
 * Input:
 * 	TimestampTz b_tmtz: begin time
 * 	TimestampTz e_tmtz: end time
 *
 * Output:
 * 	NONE:	no data source found(time error or no data found)
 * 	MEM_ONLY: the data is in ring buf
 * 	DB_ONLY:  the data is in history_data table
 * 	COMBINE:  some in ringbuf, some in history_data table
 *
 */
static DATA_TYPE
set_data_source_type(TimestampTz b_tmtz, TimestampTz e_tmtz)
{
	Oid     namespaceOid;
        Oid     relOid;

	/* need init those data */
	TimestampTz	now;
	TimestampTz	mem_start = 0,
			mem_end = 0;
	TimestampTz	db_start = 0,
			db_end = 0;

	DATA_TYPE       type = NONE;
	bool		memory_ready = false;
	bool		table_ready = false;

        if (TIMESTAMP_NOT_FINITE(b_tmtz) || TIMESTAMP_NOT_FINITE(e_tmtz))
        {
                ereport(ERROR,
                                (errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
                                 errmsg("cannot subtract infinite timestamps")));
        }

	/* 
	 * check the duration should not longer than 60 min
	 */
	if ( (e_tmtz - b_tmtz) > (60 * 60 * USECS_PER_SEC))
	{
		elog(ERROR, "The Duration should not longer than 60 mins");
	}

	/* check the time is fine */
	now = GetCurrentTimestamp();    /* current time */
	if (b_tmtz > now)
	{
		elog(ERROR, "Start Time ahead of NOW");
	}
	if (b_tmtz <= (now - 7*USECS_PER_DAY))
	{
		elog(ERROR, "Start Time later than (NOW - 7days)");
	}

	if (e_tmtz< (now - 7*USECS_PER_DAY))
	{
		elog(ERROR, "End Time is later than (NOW - 7days)");
	}
	if (e_tmtz > now)
	{
		elog(ERROR, "End Time ahead of NOW");
	}

	/* check the start time and the end time is invalid */
	if (b_tmtz >= e_tmtz)
	{
		elog(ERROR, "Start Time is ahead of End Time");
	}

	/* check the ring_buf is ready */
	if (ring_buf == NULL)
	{
		elog(ERROR, "The Ringbuf did not initialized for Highgo Session History collector");
	}
	else
	{
		memory_ready = true;
	}

	/* check if hsh table exist */
	namespaceOid = LookupNamespaceNoError(HSH_HISTORY_NAMESPACE);
	if (OidIsValid(namespaceOid))
	{
		relOid = get_relname_relid(HSH_HISTORY_DATA, namespaceOid);
		if (!OidIsValid(relOid))
		{
			elog(ERROR,"HSH table %s.%s is not exit", HSH_HISTORY_NAMESPACE, HSH_HISTORY_DATA);
		}
		else
		{
			table_ready = true;
		}
	}
	else
	{
		elog(ERROR,"HSH namespace is not exit");
	}

	/*
	 * get the memory MIN(ts) MAX(ts)
	 */
	if (memory_ready)
	{
		mem_start = hsh_get_ringbuf_start();
		mem_end = hsh_get_ringbuf_end();
	}
	else
	{
		//there has already report the error
	}

	/*
	 * get the db MIN(ts) MAX(ts)
	 */
	if (table_ready)
	{
		StringInfoData querybuf;
		int ret;
		bool isnull;

		initStringInfo(&querybuf);

		/* Open SPI context. */
		if (SPI_connect() != SPI_OK_CONNECT)
		{
			elog(ERROR, "SPI_connect failed");
		}

		/* SQL */
		appendStringInfo(&querybuf, "Select MIN(ts),MAX(ts) from %s.%s",
						HSH_HISTORY_NAMESPACE,
						HSH_HISTORY_DATA);
		ret = SPI_exec(querybuf.data, 0);
		if (ret != SPI_OK_SELECT)
		{
			elog(ERROR, "SPI_exec failed: error code %d, query: %s", ret, querybuf.data);
		}

		if (SPI_processed != 1)
		{
			elog(ERROR, "not a singleton result: %s", querybuf.data);
		}

		db_start = DatumGetTimestampTz(SPI_getbinval(SPI_tuptable->vals[0],
									SPI_tuptable->tupdesc,
									1,
									&isnull));
		db_end = DatumGetTimestampTz(SPI_getbinval(SPI_tuptable->vals[0],
									SPI_tuptable->tupdesc,
									2,
									&isnull));
         	SPI_finish();
        }
	else
	{
		// there has already report the error
	}

	/* Begin - BUG#z6049 */
        /*
	 * check the data source is memory, db or memory&db
	 */
	if (db_end == 0 && mem_end == 0)
	{
		/* there is no data in db or mem */
		type = NONE;
		return type;
	}

	/* there is no possible mem_end < db_end */
	if (mem_start == 0)        /* db have data */
	{
		/*
		 * NOTE:
		 *                       min(ts)                               max(ts)
		 * DB time:               |-------------|
		 * ME time:                                        |  nothing    |
		 * dura:     N -    (b,e)
		 *
		 *           D -    (b,         e)
		 *           D -    (b,                        e)
		 *           D -            (b, e)
		 *           D -            (b,                e)
		 *
		 *           N -                           (b, e)
		 *
		 */
		if ((e_tmtz < db_start) || (db_end < b_tmtz))
		{
			type = NONE;
		}
		else
		{
			type = DB_ONLY;
		}
	}
	else if ((mem_start != 0 && mem_start <= db_start) || db_end == 0)  /* BUG#z6416 */
	{
		/*
		 * As db did not have data or its just a part of memory
		 * just check the memory
		 */

		/*
		 * NOTE:
		 *                     min(ts)         max(ts)
		 * DB time:             |  ( nothing )   |
		 * DB time:             |  (only one record, min=max=1)
		 * DB time:             |----------------|
		 * DB time:    	            |------------|
		 * ME time:             |---------------------|
		 * dura:   N -  (b, e)
		 *
		 *         M -  (b,             e)
		 *         M -  (b,                                  e)
		 * 	   M -           (b,                e)
		 * 	   M -           (b,                         e)
		 *
		 * 	   N -                                   (b, e)
		 */
		if (e_tmtz < mem_start || b_tmtz > mem_end)
			type = NONE;
		else
			type = MEM_ONLY;

	}
        else if (db_start < mem_start && mem_start <= db_end)	/* BUG#z6146 */
	{
		/*
		 * As db have data, and it has overlapping data with memory
		 * need check db and memory
		 */

		/*
		 * NOTE:
		 *                     min(ts)                    max(ts)
		 * DB time                |---------|
		 * DB time:               |-----------------|
		 * ME time:                         |--------------------|
		 * dura:     N -  (b, e)
		 *
		 *           D -  (b,           e)
		 *           D -  (b,                     e)
		 *           D -            (b, e)
		 *           D -            (b,           e)
		 *
		 *           C -  (b,                               e)
		 *           C -  (b,                                          e)
		 *           C -             (b,                    e)
		 *           C -             (b,                               e)
		 *
		 *           M -                     (b, e)
		 *           M -                     (b,            e)
		 *           M -                     (b,                       e)
		 *           M -                               (b,  e)
		 *           M -                               (b,             e)  
		 *
		 *           N -                                            (b,e)
		 *
		 */
		if (e_tmtz < db_start)
		{
			type = NONE;
		}
		else if (b_tmtz < mem_start && e_tmtz <= db_end)	/* BUG#z6146 */
		{
			type = DB_ONLY;
		}
		else if (b_tmtz < mem_start &&  db_end < e_tmtz)
		{
			type = COMBINE;
		}
		else if (mem_start <= b_tmtz && b_tmtz <= mem_end)	/* BUG#z6146 */
		{
			type = MEM_ONLY;
		}
		else if (mem_end < b_tmtz)
		{
			type = NONE;
		}
		else
		{ 
			elog(ERROR, "Highgo Session History: should not trigged this case (overlap for DB and MEM)");
		}
		
	}
	else if (db_end < mem_start && mem_start != 0)
	{
		/*
		 * AS db have data, but have no overlapping data with memory
		 * check db and memory
		 * pay attension with gap between them
		 */

		/*
		 * NOTE:
		 *                       min(ts)                               max(ts)
		 * DB time:               |-------------|
		 * ME time:                                        |---------------|
		 * dura:     N -    (b,e)
		 *
		 *           D -    (b,         e)
		 *           D -    (b,                       e)
		 *           D -            (b, e)
		 *           D -            (b,               e)
		 *
		 *           C -    (b,                                  e)
		 *           C -    (b,                                                    e)
		 *           C -              (b,                        e)
		 *           C -              (b,                                          e)
		 *
		 *           N -                         (b, e)
		 *
		 *           M -                         (b,             e)
		 *           M -                         (b,                               e)
		 *           M -                                     (b, e)
		 *           M -                                     (b,                   e)
		 *
		 *           N -                                                       (b, e)   
		 */
		if ((e_tmtz < db_start) || (db_end < b_tmtz && e_tmtz < mem_start) || (mem_end < b_tmtz))
		{
			type = NONE;
		}
		else if (b_tmtz <= db_end && e_tmtz < mem_start)	/* BUG#z6146 */
		{
			type = DB_ONLY;
		}
		else if (b_tmtz <= db_end && e_tmtz >= mem_start)	/* BUG#z6146 */
		{
			type = COMBINE;
		}
		else if (b_tmtz <= mem_end)	/* BUG#z6146 */
		{
			type = MEM_ONLY;
		}
		else
		{
			elog(ERROR,"Highgo Session History: should not trigged this case(no-overlap for DB and MEM)");
		}

	}
	else
	{
		elog(ERROR, "Highgo Session History: This Scenario should not be trigged"); 
	}
	/* End - BUG#z6049 */

	return type;
}

/*
 * Function:
 *      hsh_report_timespec
 *
 * Description:
 *      Check the data source type from b_tmtz to e_tmtz
 *
 * Input:
 *      TimestampTz interval
 *
 * Output:
 *      NONE / MEMORY / DB / COMBINE
 *
 */
PG_FUNCTION_INFO_V1(hsh_report_timespec);
Datum
hsh_report_timespec(PG_FUNCTION_ARGS)
{
	TimestampTz   b_tmtz;
	TimestampTz   e_tmtz;

	DATA_TYPE       result;
	text       *volatile ret = NULL;

	b_tmtz = PG_GETARG_TIMESTAMPTZ(0);
	e_tmtz = PG_GETARG_TIMESTAMPTZ(1);

	result =  set_data_source_type(b_tmtz, e_tmtz);
	switch(result)
	{
		case NONE:
			ret = cstring_to_text("NONE");
			break;
		case MEM_ONLY:
			ret = cstring_to_text("MEMORY");
			break;
		case DB_ONLY:
			ret = cstring_to_text("DB");
			break;
		case COMBINE:
			ret= cstring_to_text("COMBINE");
			break;
	}

	PG_RETURN_TEXT_P(ret);
}

/*
 * Split a pathname as dirname(1) and basename(1) would.
 *
 * XXX this probably doesn't do very well on Windows.  We probably need to
 * apply canonicalize_path(), at the very least.
 */
static void
split_path(const char *path, char **dir, char **fname)
{
	char	   *sep;

	/* split filepath into directory & filename */
	sep = strrchr(path, '/');

	/* directory path */
	if (sep != NULL)
	{
		/* Begin - BUG#z6336 */
		/* '/xxx/yyy/' */
		if (strlen(path) == (sep - path + 1))
		{
			*dir = pnstrdup(path, sep - path + 1);
			*fname = NULL;
		}
		/* End - BUG#z6336 */
		else	/* '/xxx/yyy/zzz' */
		{
			*dir = pnstrdup(path, sep - path);
			*fname = pstrdup(sep + 1);
		}
	}
	/* local directory */
	else	/* 'zzz' */
	{
		*dir = NULL;
		*fname = pstrdup(path);
	}
}

/*
 * Check whether directory exists and whether we can open it. Keep errno set so
 * that the caller can report errors somewhat more accurately.
 */
static bool
verify_directory(const char *directory)
{
	DIR   *dir = opendir(directory);

	if (dir == NULL)
		return false;

	closedir(dir);
	return true;
}

static void
check_loc_validate(const char *file_path)
{
	size_t	len;
	char	*path;
	char	*filename;
	bool	ret;

	if (file_path == NULL)
		elog(ERROR, "Report file is NULL or more than %d: %s", MAXPGPATH, file_path);

	len = strlen(file_path);
	if (len == 0 || len >= MAXPGPATH)
		elog(ERROR, "Report file is NULL or more than %d: %s", MAXPGPATH, file_path);

	/*
	 * split /xxx/yyy/zzz.html to /xxx/yyy/ and zzz.html
	 * then check /xxx/yyy/ by verify_directory()
	 * check zzz.html ended by .html
	 */
	split_path(file_path, &path, &filename);
	/* Begin - BUG#z6636 */
	/*
	 * the right file_path is '/xxx/yyy/zz.html'
	 * '/xxx/yyy/' and 'zzz.html' is invalid
	 */
	if (path == NULL)
		elog(ERROR,"There is an invalid file path: %s", file_path);

	if (filename == NULL)
		elog(ERROR,"There is an invalid file path: %s", file_path);
	/* End - BUG#z6636 */

	ret = verify_directory(path);

	if (!ret)
		elog(ERROR,"could not open directory \"%s\": %m", file_path);

	/* check xxx.html */
	len = strlen(filename);
	if (strlen(filename) <= 5 || strcmp(filename + len - 5, ".html") != 0)
		elog(ERROR,"The Report file is not HTML format \"%s\" ", file_path);

}

/*
 * Function:
 *      hsh_report_to_file()
 *
 * Description:
 *      User function to generate the HSH report
 *
 * Input:
 *      TimestampTz,TimestampTz, file_to_path
 *
 * Output:
 *      xxx/xxx/hsh.html
 *
 */
PG_FUNCTION_INFO_V1(hsh_report_to_file);
Datum
hsh_report_to_file(PG_FUNCTION_ARGS)
{
	int     ret;

        TimestampTz     b_tmtz;
        TimestampTz     e_tmtz;
	text            *loc;
	StringInfoData  query;
	char		*file_path;

	StringInfoData  report;
	const char *hgdb_version;
	StringInfoData	buf_tmp;

	Oid             relOid;
	Relation        rel;
	Oid             namespaceid;
	char		*tmp_0;

	struct pg_tm tt, *tm = &tt;
	char		buf_a[MAXDATELEN + 1];
	fsec_t		fsec;

	struct pg_tm tt_b, *tm_b = &tt_b;
	char		buf_b[MAXDATELEN + 1];
	fsec_t		fsec_b;

	DATA_TYPE type;

	/* get the input arg values */
	b_tmtz = PG_GETARG_TIMESTAMPTZ(0);
	e_tmtz = PG_GETARG_TIMESTAMPTZ(1);
	loc = PG_GETARG_TEXT_PP(2);

	/* get the data source type */
	type = set_data_source_type(b_tmtz, e_tmtz);
	entry.sample_source = type;

	/* check the loc is fine */
	file_path = text_to_cstring(loc);
	check_loc_validate(file_path);

	PushActiveSnapshot(GetTransactionSnapshot());

	/* put the data into tmp table hsh_result */
	if (SPI_connect() != SPI_OK_CONNECT)
	{
		elog(ERROR, "SPI_connect failed");
	}

	initStringInfo(&query);

	/* CREATE TEMPORARY TABLE */
	appendStringInfo(&query, "CREATE TEMPORARY TABLE %s (like %s.%s) ON COMMIT DROP;",
						HSH_RESULT,
						HSH_HISTORY_NAMESPACE,
						HSH_HISTORY_DATA);
	ret = SPI_execute(query.data, false, 0);
	if (ret != SPI_OK_UTILITY)
	{
		elog(ERROR, "SPI_exec failed: error code %d, query: %s", ret, query.data);
	}
	CommandCounterIncrement();

	/* READ memory sample data into table */
	if (type == MEM_ONLY)
	{
		namespaceid = LookupNamespaceNoError("pg_temp");
		relOid = get_relname_relid(HSH_RESULT, namespaceid);
		rel = table_open(relOid, RowExclusiveLock);

		write_mem_to_tmp_table(b_tmtz, e_tmtz, rel);

		table_close(rel, RowExclusiveLock);

		CommandCounterIncrement();

	}
	else if ( type == DB_ONLY )
	{
		write_db_to_tmp_table(b_tmtz, e_tmtz);

		CommandCounterIncrement();
	}
	else if (type == COMBINE)
	{
		namespaceid = LookupNamespaceNoError("pg_temp");
		relOid = get_relname_relid(HSH_RESULT, namespaceid);
		rel = table_open(relOid, RowExclusiveLock);

		write_mem_db_to_tmp_table(b_tmtz, e_tmtz, rel);
		table_close(rel, RowExclusiveLock);

		CommandCounterIncrement();
	}
	else
	{
		elog(ERROR, "Hsh_report_to_file() did not find the requested data");  
	}

	/* the the database version */

#ifdef USE_LICENSE
	/* Begin - BUG#z6426 */
	/*
	 * in the same session, 
	 * if calling licence function get_lic_baseinfo()
	 * it will set the title format to true, so the hg_get_highgoVersion() will output 'Database version: xxxx'
	 * HSH did not need 'Database version:', so set the title to false
	 */
	hg_set_is_with_title(false);
	/* End - BUG#z6426 */
	hgdb_version = hg_get_highgoVersion();
#else
	hgdb_version = "Highgo database";
#endif
	
	/*
	 * cal the report
	 * put the result into entry struct
	 */
	cal_report(b_tmtz, e_tmtz);

	/* 
	 * read the template style and header
	 */
	initStringInfo(&report);
	
	hsh_read_template(&report, html_style);
	hsh_read_template(&report, html_header);
	hsh_read_template(&report, html_version_h);
	hsh_read_template(&report, html_version_b);
	hsh_read_template(&report, html_sample_h);
	hsh_read_template(&report, html_sample_b);
	hsh_read_template(&report, html_sample_d); /* BUG#z6128 */
	hsh_read_template(&report, html_sample_report_body);

	/*
	 * hsh report version part
	 */
        stringinfo_replace(&report, "{{DATABASE_NAME}}", "ALL");
        stringinfo_replace(&report, "{{DATABASE_VERSION}}", hgdb_version);
	if (entry.sample_source == MEM_ONLY)
	{
		stringinfo_replace(&report, "{{DATABASE_SOURCE}}", "MEMORY");
	}
	else if (entry.sample_source == DB_ONLY)
	{
		stringinfo_replace(&report, "{{DATABASE_SOURCE}}", "DB");
	}
	else if (entry.sample_source == COMBINE)
	{
		stringinfo_replace(&report, "{{DATABASE_SOURCE}}", "COMBINE");
	}
	else
	{
		elog(ERROR,"Invalid Data Source Type");
	}

	if (TIMESTAMP_NOT_FINITE(entry.start_tm))
	{
		EncodeSpecialTimestamp(entry.start_tm, buf_a);
	}
	else if (timestamp2tm(entry.start_tm, NULL, tm, &fsec, NULL, NULL) == 0)
	{
		EncodeDateTime(tm, fsec, false, 0, NULL, DateStyle, buf_a);
	}
	else
	{
		ereport(ERROR,
				(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
				 errmsg("timestamp out of range")));
	}

        if (TIMESTAMP_NOT_FINITE(entry.end_tm))
        {
                EncodeSpecialTimestamp(entry.end_tm, buf_b);
        }
        else if (timestamp2tm(entry.end_tm, NULL, tm_b, &fsec_b, NULL, NULL) == 0)
        {
                EncodeDateTime(tm_b, fsec_b, false, 0, NULL, DateStyle, buf_b);
        }
        else
        {
                ereport(ERROR,
                                (errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
                                 errmsg("timestamp out of range")));
        }

        stringinfo_replace(&report, "{{START_TIME}}", buf_a);
        stringinfo_replace(&report, "{{END_TIME}}", buf_b);

	tmp_0 = psprintf("%llu", (unsigned long long)entry.sample_time);
        stringinfo_replace(&report, "{{DURATION}}", tmp_0);

	tmp_0 = psprintf("%llu", (unsigned long long)entry.sample_counts);
        stringinfo_replace(&report, "{{TOTAL_SAMPLE}}", tmp_0);

	tmp_0 = psprintf("%.2f", entry.avg_sessions);
        stringinfo_replace(&report, "{{AVG_SESSIONS}}", tmp_0);

	/*
	 * init a tmp buff
	 * need reset it before use it
	 * free the memory in the end
	 */
	initStringInfo(&buf_tmp);

	/*
	 * report the top user events
	 * read the tempalte, then repalce in the place holder
	 * put the content to the report buffer
	 */
	hsh_read_template(&report, html_event_t);
	hsh_read_template(&report, html_event_d);
	hsh_read_template(&report, html_event_h);

	for (int i = 0; i < entry.top_event_counts; i++)
	{
		char	*tmp_1, *tmp_2, *tmp_3;

		resetStringInfo(&buf_tmp);

		hsh_read_template(&buf_tmp, html_event_b);

		stringinfo_replace(&buf_tmp, "{{event_type}}", entry.top_events[i].wait_type);
		stringinfo_replace(&buf_tmp, "{{event_event}}", entry.top_events[i].wait_event);

		tmp_1 = psprintf("%llu", (unsigned long long)entry.top_events[i].count);
		stringinfo_replace(&buf_tmp, "{{event_sample_count}}", tmp_1);

		tmp_2 = psprintf("%.2f", entry.top_events[i].count_d_sample);
		stringinfo_replace(&buf_tmp, "{{event_percentage}}", tmp_2);

		tmp_3 = psprintf("%.2f", entry.top_events[i].count_d_time);
		stringinfo_replace(&buf_tmp, "{{event_avg}}", tmp_3);

		/* append to the report */
		appendStringInfo(&report,"%s", buf_tmp.data);

	}
	hsh_read_template(&report, html_events_e);

	/* 
	 * hsh report top_sql_type
	 */ 
	hsh_read_template(&report, html_sql_type_t);
	hsh_read_template(&report, html_sql_type_d);
	hsh_read_template(&report, html_sql_type_h);

        for (int i = 0; i < entry.top_sql_counts; i++)
        {
		char	*tmp_1, *tmp_2, *tmp_3;

		resetStringInfo(&buf_tmp);

		hsh_read_template(&buf_tmp,html_sql_type_b);

		stringinfo_replace(&buf_tmp, "{{SQL_TYPE}}", entry.top_sqls[i].name);

		tmp_1 = psprintf("%d", entry.top_sqls[i].id_type);
		stringinfo_replace(&buf_tmp, "{{SQL_DIFF}}", tmp_1);

		tmp_2 = psprintf("%.2f", entry.top_sqls[i].count_d_sample);
		stringinfo_replace(&buf_tmp, "{{SQL_PERCENTAGE}}", tmp_2);

		tmp_3 = psprintf("%.2f", entry.top_sqls[i].count_d_time);
		stringinfo_replace(&buf_tmp, "{{SQL_AVG}}", tmp_3);

		/* append to the report */
		appendStringInfo(&report,"%s", buf_tmp.data);

	}
        hsh_read_template(&report, html_sql_type_e);

	/*
	 * hsh report top_sql_with_top_event
	 */
	hsh_read_template(&report, html_sql_event_t);
	hsh_read_template(&report, html_sql_event_d);
	hsh_read_template(&report, html_sql_event_h);

        for (int i = 0; i < entry.topsql_with_topevent_counts; i++)
        {
		char	int64_str[32];
		char    uint64_str[32];
		char	*tmp_1;
		char	*tmp_2;

		resetStringInfo(&buf_tmp);

                hsh_read_template(&buf_tmp, html_sql_event_b);

		snprintf(int64_str, sizeof(int64_str), INT64_FORMAT, entry.topsql_with_topevents[i].query_id);
		stringinfo_replace(&buf_tmp, "{{QUERY_REF}}", int64_str);
                stringinfo_replace(&buf_tmp, "{{QUERY_ID}}", int64_str);

		snprintf(uint64_str, sizeof(uint64_str), UINT64_FORMAT, entry.topsql_with_topevents[i].counts);
                stringinfo_replace(&buf_tmp, "{{QUERY_COUNTS}}", uint64_str);

                tmp_1 = psprintf("%.2f", entry.topsql_with_topevents[i].counts_d_sample);
                stringinfo_replace(&buf_tmp, "{{QUERY_PRE}}", tmp_1);

                stringinfo_replace(&buf_tmp, "{{QUERY_EVENT}}", entry.topsql_with_topevents[i].wait_event);

		snprintf(uint64_str, sizeof(uint64_str), UINT64_FORMAT, entry.topsql_with_topevents[i].wait_counts);
		stringinfo_replace(&buf_tmp, "{{QUERY_EVENT_COUNTS}}", uint64_str);

		tmp_2 = psprintf("%.2f", entry.topsql_with_topevents[i].wait_counts_d_counts);
		stringinfo_replace(&buf_tmp, "{{QUERY_EVENT_AVG}}", tmp_2);
		
		/* append to the report */
		appendStringInfo(&report,"%s", buf_tmp.data);

	}
	hsh_read_template(&report, html_sql_event_e);

	/*
	 * hsh report top_session
	 */
	hsh_read_template(&report, html_session_t);
	hsh_read_template(&report, html_session_d);
	hsh_read_template(&report, html_session_h);

	for (int i = 0; i < entry.top_session_counts; i++)
	{
		char	*tmp_1, *tmp_2;
		char    *tmp_3, *tmp_4;

		resetStringInfo(&buf_tmp);

                hsh_read_template(&buf_tmp, html_session_b);

		tmp_1 = psprintf("%d", entry.top_sessions[i].pid);
                stringinfo_replace(&buf_tmp, "{{SESSION_ID}}", tmp_1);

		tmp_2 = psprintf("%.2f", entry.top_sessions[i].pid_d_sample);
                stringinfo_replace(&buf_tmp, "{{SESSION_PRE}}", tmp_2);

		tmp_3 = psprintf("%.2f", entry.top_sessions[i].pid_d_ts);
		stringinfo_replace(&buf_tmp, "{{SESSION_AVG}}", tmp_3); 

                stringinfo_replace(&buf_tmp, "{{SESSION_EVENT}}", entry.top_sessions[i].wait_event);

                tmp_4 = psprintf("%.2f", entry.top_sessions[i].event_d_pid);
                stringinfo_replace(&buf_tmp, "{{SESSION_EVENT_PRE}}", tmp_4);

		/* append to the report */
		appendStringInfo(&report,"%s", buf_tmp.data);

	}
	hsh_read_template(&report, html_session_e);

	/*
	 * hsh_report_top_phase
 	 */
	hsh_read_template(&report, html_phase_t);
	hsh_read_template(&report, html_phase_d);
	hsh_read_template(&report, html_phase_h);
	for (int i = 0; i < entry.top_phase_counts; i++)
	{
		char	*tmp_1;
		char	*tmp_2;
		char	uint64_str[32];

		resetStringInfo(&buf_tmp);

		hsh_read_template(&buf_tmp, html_phase_b);

		stringinfo_replace(&buf_tmp, "{{PHASE_NAME}}", entry.top_phases[i].phase_name);

		snprintf(uint64_str, sizeof(uint64_str), UINT64_FORMAT, entry.top_phases[i].phase_count);
		stringinfo_replace(&buf_tmp, "{{PHASE_COUNT}}", uint64_str);

		tmp_1 = psprintf("%.2f", entry.top_phases[i].count_d_sum);
		stringinfo_replace(&buf_tmp, "{{PHASE_AVG}}", tmp_1);


		tmp_2 = psprintf("%.2f", entry.top_phases[i].count_d_ts);
		stringinfo_replace(&buf_tmp, "{{PHASE_PRE}}", tmp_2);

		/* append to the report */
		appendStringInfo(&report,"%s", buf_tmp.data);

	}
	hsh_read_template(&report, html_phase_e);

	/*
	 * hsh report activity time
	 */
	hsh_read_template(&report, html_over_time_t);
	hsh_read_template(&report, html_over_time_d);
	hsh_read_template(&report, html_over_time_h);

	for (int i = 0; i < entry.sample_slot_counts; i++)
	{
		char *tmp;
		int j = i * 3;

		resetStringInfo(&buf_tmp);

		hsh_read_template(&buf_tmp, html_over_time_b);

		/* first line */
		stringinfo_replace(&buf_tmp, "{{ACTIVITY_START}}", entry.sample_slots[j].begin_str);
		stringinfo_replace(&buf_tmp, "{{ACTIVITY_END}}", entry.sample_slots[j].end_str);

		tmp = psprintf("%d", entry.sample_slots[j].sample);
		stringinfo_replace(&buf_tmp, "{{ACTIVITY_COUNTS}}", tmp);

		stringinfo_replace(&buf_tmp, "{{EVENT_NAME}}",entry.sample_slots[j].wait_event);
		tmp = psprintf("%d", entry.sample_slots[j].count);
		stringinfo_replace(&buf_tmp, "{{EVENT_COUNT}}",tmp);

		tmp = psprintf("%.2f", entry.sample_slots[j].count_d_sample);
		stringinfo_replace(&buf_tmp, "{{EVENT_PRE}}", tmp);
		appendStringInfo(&report,"%s", buf_tmp.data);
		
		/* second line */
		resetStringInfo(&buf_tmp);
		hsh_read_template(&buf_tmp, html_over_time_b);

		if (entry.sample_slots[j+1].count != 0)
		{
			stringinfo_replace(&buf_tmp, "{{ACTIVITY_START}}","");
			stringinfo_replace(&buf_tmp, "{{ACTIVITY_END}}","");
			stringinfo_replace(&buf_tmp, "{{ACTIVITY_COUNTS}}","");

			stringinfo_replace(&buf_tmp, "{{EVENT_NAME}}",entry.sample_slots[j+1].wait_event);
			tmp = psprintf("%d", entry.sample_slots[j+1].count);
			stringinfo_replace(&buf_tmp, "{{EVENT_COUNT}}",tmp);

			tmp = psprintf("%.2f", entry.sample_slots[j+1].count_d_sample);
			stringinfo_replace(&buf_tmp, "{{EVENT_PRE}}", tmp);

			appendStringInfo(&report,"%s", buf_tmp.data);
			
		}

		/* third line */
		resetStringInfo(&buf_tmp);
		hsh_read_template(&buf_tmp, html_over_time_b);

		if (entry.sample_slots[j+2].count != 0)
		{
			stringinfo_replace(&buf_tmp, "{{ACTIVITY_START}}","");
			stringinfo_replace(&buf_tmp, "{{ACTIVITY_END}}","");
			stringinfo_replace(&buf_tmp, "{{ACTIVITY_COUNTS}}","");

			stringinfo_replace(&buf_tmp, "{{EVENT_NAME}}",entry.sample_slots[j+2].wait_event);
			tmp = psprintf("%d", entry.sample_slots[j+2].count);
			stringinfo_replace(&buf_tmp, "{{EVENT_COUNT}}",tmp);

			tmp = psprintf("%.2f", entry.sample_slots[j+2].count_d_sample);
			stringinfo_replace(&buf_tmp, "{{EVENT_PRE}}", tmp);

			appendStringInfo(&report,"%s", buf_tmp.data);
		}	
	}
	hsh_read_template(&report, html_over_time_e);
	
	/*
	 * hsh report complete SQL
	 */
	hsh_read_template(&report, html_sql_list_t);
	hsh_read_template(&report, html_sql_list_d);
	hsh_read_template(&report, html_sql_list_h);

        for (int i = 0; i < entry.sql_slot_counts; i++)
        {
		char    int64_str[32];

		resetStringInfo(&buf_tmp);

                hsh_read_template(&buf_tmp, html_sql_list_b);

		snprintf(int64_str, sizeof(int64_str), INT64_FORMAT, entry.sql_slots[i].query_id);
                stringinfo_replace(&buf_tmp, "{{SQL_TEXT_REF}}", int64_str);

		stringinfo_replace(&buf_tmp, "{{SQL_TEXT_ID}}", int64_str);

                stringinfo_replace(&buf_tmp, "{{SQL_TEXT_TEXT}}", entry.sql_slots[i].text);

		/* append to the report */
		appendStringInfo(&report,"%s", buf_tmp.data);
	}
	hsh_read_template(&report, html_sql_list_e);

	/*
 	 * read the template end
 	 */
	hsh_read_template(&report, html_end);

	pfree(buf_tmp.data);

	PopActiveSnapshot();
	
	/*
	 * write to the file 
	 */
	if(loc)
	{
		char	*outpath;
		FILE	*fpout = NULL;

		outpath = text_to_cstring(loc);

		fpout = AllocateFile(outpath, "w");
		if( fpout == NULL)
		{
			ereport(ERROR,
				(errcode_for_file_access(),
				errmsg("could not open file \"%s\": %m",
				outpath)));
		}

		if (fwrite(report.data, 1, report.len, fpout) != report.len)
		{
			ereport(ERROR,
				(errcode_for_file_access(),
				errmsg("could not write file \"%s\": %m",
				outpath)));
		}

		FreeFile(fpout);
	}
	else
	{
		elog(ERROR, "Location is not availiable");
	}

	PG_RETURN_BOOL(true);
}

/*
 * Function:
 *      div_uint64_round()
 *
 * Description:
 *      a / b = xx.xxa
 *      such as: 100/800 = 0.13
 *
 */
static double 
div_uint64_round(uint64 a, uint64 b)
{
	double res;

	if (b == 0)
	{
		return 0.0;
	}

	res = (double) a / (double) b;
	res = round(res * 100.0) / 100.0;

	return res;
}

/*
 * Function:
 *      div_uint64_round()
 *
 * Description:
 *      a / b = xx.xx%
 *      such as: 100/800 = 12.50%
 *
 */
static double
div_uint64_round_2(uint64 a, uint64 b)
{
        double res;
        if (b == 0)
        {
                return 0.0;
        }

        res = (double) a / (double) b * 100.0;
        res = round(res * 100.0) / 100.0;

        return res;
}

/*
 * Function:
 *      cal_report()
 *
 * Description:
 *      Read the data from tmp table hsh_result
 *      cal the percentage
 *      put the result into entry struct
 *
 * Input:
 * 	time interval
 *
 * Output:
 *	hsh_entry
 *
 */
void
cal_report(TimestampTz aa, TimestampTz bb)
{
	uint64	time_diff;
	int	sample_count;
	StringInfoData query;
	int	ret;
	bool	isnull;
	SPITupleTable	*spi_tuptable;
	HeapTuple	spi_tuple;
	TupleDesc	spi_tupdesc;

	char	*column_1;
	char	*column_2;
	char    *column_3;

	char	*wait_type;
        char	*wait_event;
        char	*cnt;

	int     inter;

	/* start and end time */
	entry.start_tmtz = aa;
	entry.end_tmtz = bb;

	entry.start_tm = timestamptz2timestamp(aa);
	entry.end_tm = timestamptz2timestamp(bb);

	/* cal eslape time */
	time_diff = bb - aa;
	entry.sample_time = time_diff / USECS_PER_SEC;

	initStringInfo(&query);
	appendStringInfo(&query, "SELECT count(*) FROM %s WHERE backend_type = 'client backend';",
						HSH_RESULT);
	ret = SPI_execute(query.data, false, 0);
	if (ret != SPI_OK_SELECT)
	{
		elog(ERROR, "SPI_execute failed: error code %d, query %s", ret, query.data);
	}
	if (SPI_processed != 1)
	{
		elog(ERROR, "not a singleton result for query: %s", query.data);
	}

	sample_count = DatumGetInt32(SPI_getbinval(SPI_tuptable->vals[0],
							   SPI_tuptable->tupdesc,
							   1, &isnull));
	if (isnull)
	{
		elog(ERROR, "null result for query: %s", query.data);
	}

	entry.sample_counts = sample_count;


	if (entry.sample_source == MEM_ONLY)
	{
		entry.avg_sessions = div_uint64_round(entry.sample_counts, entry.sample_time);
	}
	else if (entry.sample_source == DB_ONLY)
	{
		entry.avg_sessions = div_uint64_round(entry.sample_counts * 10, entry.sample_time);
	}
	else if (entry.sample_source == COMBINE)
	{
		entry.avg_sessions = div_uint64_round(entry.sample_counts * 10, entry.sample_time);
	}
	else
	{
		elog(ERROR,"Did not find data for time duration");
	}

	/*
 	 * cal top 10 user event
 	 * store the result into entry.top_events
 	 */
	memset((void*)(entry.top_events), 0, sizeof(entry.top_events));
	resetStringInfo(&query);
	appendStringInfo(&query, "SELECT event_type,event,count(*) AS cnt FROM %s WHERE backend_type = 'client backend' \
					GROUP BY event_type,event ORDER BY cnt DESC LIMIT 10;",
					HSH_RESULT);
	ret = SPI_execute(query.data, false, 0);

	if (ret != SPI_OK_SELECT)
	{
		elog(ERROR, "SPI_execute failed: error code %d, query: %s", ret, query.data);
	}
	entry.top_event_counts = SPI_processed;

	spi_tuptable = SPI_tuptable;
	spi_tupdesc = spi_tuptable->tupdesc;
	if (spi_tupdesc->natts != 3)
	{
		elog(ERROR, "More columns returned: %s", query.data);
	}

	for (int i = 0; i < entry.top_event_counts; i++)
	{
		spi_tuple = spi_tuptable->vals[i];
		wait_type = SPI_getvalue(spi_tuple, spi_tupdesc, 1);
		wait_event = SPI_getvalue(spi_tuple, spi_tupdesc, 2);
		cnt = SPI_getvalue(spi_tuple, spi_tupdesc, 3);

		strncpy(entry.top_events[i].wait_type, wait_type, STR_LEN - 1);
		strncpy(entry.top_events[i].wait_event, wait_event, STR_LEN - 1);
		entry.top_events[i].count = pg_strtouint64(cnt, NULL, 0);
	}

	for (int i = 0; i < entry.top_event_counts; i++)
	{
		entry.top_events[i].count_d_sample = div_uint64_round_2(entry.top_events[i].count, entry.sample_counts);

		if (entry.sample_source == MEM_ONLY)
		{
			entry.top_events[i].count_d_time = div_uint64_round(entry.top_events[i].count, entry.sample_time);
		}
		else if (entry.sample_source == DB_ONLY || entry.sample_source == COMBINE)
		{
			entry.top_events[i].count_d_time = div_uint64_round(entry.top_events[i].count * 10, entry.sample_time);
		}
		else
		{
			entry.top_events[i].count_d_time = 0;
		}
		
	}

	/*
	 * cal top sql 
	 * store in entry.top_sqls
	 */
	memset((void*)(entry.top_sqls), 0, sizeof(entry.top_sqls));

	resetStringInfo(&query);
	appendStringInfo(&query, "SELECT cmd_type,count(*) AS cnt,count(DISTINCT query_id) as uni_q FROM %s \
							WHERE backend_type = 'client backend' AND cmd_type != 'unkown' \
							GROUP BY cmd_type ORDER BY cnt DESC limit 10;",
							HSH_RESULT);
	ret = SPI_execute(query.data, false, 0);
	if (ret != SPI_OK_SELECT)
	{
                elog(ERROR, "SPI_execute failed: error code %d, query: %s", ret, query.data);
	}
	entry.top_sql_counts = SPI_processed;

	spi_tuptable = SPI_tuptable;
        spi_tupdesc = spi_tuptable->tupdesc;

        if (spi_tupdesc->natts != 3)
	{
                elog(ERROR, "More columns returned: %s", query.data);
	}

        for (int i = 0; i < SPI_processed; i++)
        {
                spi_tuple = spi_tuptable->vals[i];
                column_1= SPI_getvalue(spi_tuple, spi_tupdesc, 1);
                column_2 = SPI_getvalue(spi_tuple, spi_tupdesc, 2);
		column_3 = SPI_getvalue(spi_tuple, spi_tupdesc, 3);

                strncpy(entry.top_sqls[i].name, column_1, STR_LEN - 1);
                entry.top_sqls[i].count = pg_strtouint64(column_2, NULL, 0);
		entry.top_sqls[i].id_type = pg_strtoint32(column_3);
        }

        for (int i = 0; i < entry.top_sql_counts; i++)
        {
                entry.top_sqls[i].count_d_sample = div_uint64_round_2(entry.top_sqls[i].count, entry.sample_counts);

		if (entry.sample_source == MEM_ONLY)
		{
                	entry.top_sqls[i].count_d_time = div_uint64_round(entry.top_sqls[i].count, entry.sample_time);
		}
		else if (entry.sample_source == DB_ONLY || entry.sample_source == COMBINE)
		{
			entry.top_sqls[i].count_d_time = div_uint64_round(entry.top_sqls[i].count * 10, entry.sample_time);
		}
		else
		{
			entry.top_sqls[i].count_d_time = 0;
		}		
        }

	/*
	 * cal topsql_with_topevent
	 * first select the top sql from hsh_result
	 * then select the top event for every sql
	 */
	memset((void*)(entry.topsql_with_topevents), 0, sizeof(entry.topsql_with_topevents));
	resetStringInfo(&query);
	appendStringInfo(&query, "SELECT query_id, COUNT(*) AS count FROM %s WHERE backend_type = 'client backend' AND \
						query_id != 0 GROUP BY query_id \
						ORDER BY count DESC limit 10;",
						HSH_RESULT);
	ret = SPI_execute(query.data, false, 0);
	if (ret != SPI_OK_SELECT)
        {
                elog(ERROR, "SPI_execute failed: error code %d, query: %s", ret, query.data);
        }
	entry.topsql_with_topevent_counts = SPI_processed;
	
	spi_tuptable = SPI_tuptable;
	spi_tupdesc = spi_tuptable->tupdesc;
	if (spi_tupdesc->natts != 2)
        {
                elog(ERROR, "More columns returned for query: %s", query.data);
        }
	for (int i = 0; i < SPI_processed; i++)
	{
		spi_tuple = spi_tuptable->vals[i];
                column_2 = SPI_getvalue(spi_tuple, spi_tupdesc, 2);

		entry.topsql_with_topevents[i].query_id = DatumGetInt64(SPI_getbinval(spi_tuple, spi_tupdesc, 1, &isnull));
		entry.topsql_with_topevents[i].counts =  pg_strtoint32(column_2);
		entry.topsql_with_topevents[i].counts_d_sample = div_uint64_round_2(entry.topsql_with_topevents[i].counts, entry.sample_counts);
	}
	/* select the top wait event for the top sql */
	for (int i = 0; i < entry.topsql_with_topevent_counts; i++)
	{
		char    int64_str[32];
                Datum    datum;
                text    *txt;
                char    *cstr;

		resetStringInfo(&query);

		snprintf(int64_str, sizeof(int64_str), INT64_FORMAT, entry.topsql_with_topevents[i].query_id);

		appendStringInfo(&query, "SELECT event, COUNT(*) as cnt FROM %s where query_id = '%s' \
						GROUP BY event ORDER BY cnt DESC LIMIT 1;",
						HSH_RESULT,
						int64_str);

		ret = SPI_execute(query.data, false, 0);
		if (ret != SPI_OK_SELECT)
		{
			elog(ERROR, "SPI_execute failed: error code %d, query: %s", ret, query.data);
		}
		if (SPI_processed >= 2)
		{
			elog(ERROR, "More returned for query: %s", query.data);
		}
		spi_tuptable = SPI_tuptable;
		spi_tupdesc = spi_tuptable->tupdesc;
		if (spi_tupdesc->natts != 2)
		{
			elog(ERROR, "More columns returned for query: %s", query.data);
		}

		/*
 		 * get top 1 event and the count
 		 */
		spi_tuple = spi_tuptable->vals[0];
		datum = SPI_getbinval(spi_tuple, spi_tupdesc, 1, &isnull);
		txt = DatumGetTextPP(datum);
		cstr = text_to_cstring(txt);

                strncpy(entry.topsql_with_topevents[i].wait_event, cstr, STR_LEN-1);
                entry.topsql_with_topevents[i].wait_counts =  pg_strtoint32(SPI_getvalue(spi_tuple, spi_tupdesc, 2));

		entry.topsql_with_topevents[i].wait_counts_d_counts = div_uint64_round_2(entry.topsql_with_topevents[i].wait_counts, entry.topsql_with_topevents[i].counts);

		pfree(cstr);
	}


	/*
	 * cal top session
	 * select the top pid
	 * select the top event fot pid
	 * store the result into entry.top_sessions
	 */
	memset((void*)(entry.top_sessions), 0, sizeof(entry.top_sessions));

	/* pid , counts */
        resetStringInfo(&query);
        appendStringInfo(&query, "SELECT pid,count(*) AS cnt FROM %s WHERE backend_type = 'client backend' \
						GROUP BY pid ORDER BY cnt DESC limit 10;",
						HSH_RESULT);
        ret = SPI_execute(query.data, false, 0);
        if (ret != SPI_OK_SELECT)
        {
                elog(ERROR, "SPI_execute failed: error code %d, query: %s", ret, query.data);
        }
        entry.top_session_counts = SPI_processed;

        spi_tuptable = SPI_tuptable;
        spi_tupdesc = spi_tuptable->tupdesc;
        if (spi_tupdesc->natts != 2)
        {
                elog(ERROR, "More columns returned for query: %s", query.data);
        }

        for (int i = 0; i < SPI_processed; i++)
        {
                spi_tuple = spi_tuptable->vals[i];
		entry.top_sessions[i].pid = pg_strtoint32(SPI_getvalue(spi_tuple, spi_tupdesc, 1));
		column_2 = SPI_getvalue(spi_tuple, spi_tupdesc, 2);

                entry.top_sessions[i].pid_counts = pg_strtouint64(column_2, NULL, 0);
        }

        for (int i = 0; i < entry.top_session_counts; i++)
        {
                entry.top_sessions[i].pid_d_sample = div_uint64_round_2(entry.top_sessions[i].pid_counts, entry.sample_counts);

		if (entry.sample_source == MEM_ONLY)
		{
			entry.top_sessions[i].pid_d_ts = div_uint64_round(entry.top_sessions[i].pid_counts, entry.sample_time);
		}
		else if (entry.sample_source == DB_ONLY || entry.sample_source == COMBINE)
		{
			entry.top_sessions[i].pid_d_ts = div_uint64_round(entry.top_sessions[i].pid_counts * 10, entry.sample_time);
		}
		else
		{
			entry.top_sessions[i].pid_d_ts = 0;
		}
		
        }

	/* event, counts */
	for (int i = 0; i < entry.top_session_counts; i++)
	{

		Datum   datum;
                text    *txt;
                char    *cstr;

		resetStringInfo(&query);
		appendStringInfo(&query, "SELECT event, COUNT(*) as cnt FROM %s where pid = %d \
							GROUP BY event ORDER BY cnt DESC LIMIT 1;",
							HSH_RESULT,
							entry.top_sessions[i].pid);

		ret = SPI_execute(query.data, false, 0);
                if (ret != SPI_OK_SELECT)
                {
                        elog(ERROR, "SPI_execute failed: error code %d, query: %s", ret, query.data);
                }
                if (SPI_processed != 1)
		{
                        elog(ERROR, "More returned for query: %s", query.data);
		}

                spi_tuptable = SPI_tuptable;
                spi_tupdesc = spi_tuptable->tupdesc;
                if (spi_tupdesc->natts != 2)
                {
                        elog(ERROR, "More columns returned: %s", query.data);
                }

                /*
		 * get top 1 event and the count
		 */
                spi_tuple = spi_tuptable->vals[0];
                datum = SPI_getbinval(spi_tuple, spi_tupdesc, 1, &isnull);
                txt = DatumGetTextPP(datum);
                cstr = text_to_cstring(txt);

                strncpy(entry.top_sessions[i].wait_event, cstr, STR_LEN-1);
                entry.top_sessions[i].event_counts =  pg_strtoint32(SPI_getvalue(spi_tuple, spi_tupdesc, 2));

                entry.top_sessions[i].event_d_pid = div_uint64_round_2(entry.top_sessions[i].event_counts, entry.top_sessions[i].pid_counts);

		pfree(cstr);
	}

	/*
	 * top phase
	 */
	memset((void*)(entry.top_phases), 0, sizeof(entry.top_phases));
	resetStringInfo(&query);
	appendStringInfo(&query, "SELECT phase,count(*) AS cnt FROM %s WHERE backend_type = 'client backend' \
						GROUP BY phase ORDER BY cnt DESC limit 5;",
						HSH_RESULT);

	ret = SPI_execute(query.data, false, 0);
	if (ret != SPI_OK_SELECT)
	{
		elog(ERROR, "SPI_execute failed: error code %d, query: %s", ret, query.data);
	}

	entry.top_phase_counts = SPI_processed;
	spi_tuptable = SPI_tuptable;
	spi_tupdesc = spi_tuptable->tupdesc;

	if (spi_tupdesc->natts != 2)
	{
		elog(ERROR, "More columns returned for query: %s", query.data);
	}

	for (int i = 0; i < SPI_processed; i++)
	{
		Datum   datum;
                text    *txt;
                char    *cstr;
		char 	*count_str;
		spi_tuple = spi_tuptable->vals[i];

		/* phase name */
		datum = SPI_getbinval(spi_tuple, spi_tupdesc, 1, &isnull);
		txt = DatumGetTextPP(datum);
		cstr = text_to_cstring(txt);

		strncpy(entry.top_phases[i].phase_name, cstr, STR_LEN-1);

		/* phase count */
		count_str = SPI_getvalue(spi_tuple, spi_tupdesc, 2);
		entry.top_phases[i].phase_count = pg_strtouint64(count_str, NULL, 0);
	}

	/*
 	 * count / sum_count
 	 * count / time
 	 */
	if (entry.top_phase_counts != 0)
	{
		uint64 sum_count = 0;

		for (int i = 0; i < entry.top_phase_counts; i++)
		{
			sum_count +=  entry.top_phases[i].phase_count;
		}

		if (sum_count == 0)
		{	
			/* do nothing */
			goto cal_continue;
		}

		for (int i = 0; i < entry.top_phase_counts; i++)
		{
			entry.top_phases[i].count_d_sum = div_uint64_round_2(entry.top_phases[i].phase_count, sum_count);

			if (entry.sample_source == MEM_ONLY)
			{
				entry.top_phases[i].count_d_ts = div_uint64_round(entry.top_phases[i].phase_count, entry.sample_time);
			}
			else if (entry.sample_source == DB_ONLY || entry.sample_source == COMBINE)
			{
				entry.top_phases[i].count_d_ts = div_uint64_round(entry.top_phases[i].phase_count*10,entry.sample_time);
			}
			else
			{
				entry.top_phases[i].count_d_ts = 0;
			}

		}
	}

cal_continue:

#define OVERTIME_INTERVAL 900

	/* over time */
	memset(entry.sample_slots, 0, sizeof(entry.sample_slots)); /* BUG#z6130 */

	/* interval is 15 * 60s = 900s*/
	if ((entry.sample_time % OVERTIME_INTERVAL) == 0 )
	{
		inter = entry.sample_time / OVERTIME_INTERVAL;
	}
	else
	{
		inter = (entry.sample_time / OVERTIME_INTERVAL + 1);
	}
	entry.sample_slot_counts = inter;

	Assert( entry.sample_slot_counts < 5);

	/* cal begin_ts, end_ts */
	if ( inter  == 1 )
        {
                entry.sample_slots[0].begin = entry.start_tmtz;
                entry.sample_slots[0].end   = entry.end_tmtz;

        }
	else
	{

		/* internal  = 15 min */
		for (int i = 0; i < inter - 1; i++)
		{
			int j = i * 3;
			if ( i == 0 )
			{
				entry.sample_slots[j].begin = entry.start_tmtz;
				entry.sample_slots[j].end   = entry.start_tmtz + OVERTIME_INTERVAL * USECS_PER_SEC;

			}
			else
			{
				entry.sample_slots[j].begin = entry.sample_slots[(i-1) * 3].end;
				entry.sample_slots[j].end = entry.sample_slots[j].begin + OVERTIME_INTERVAL * USECS_PER_SEC;
			}

		}

		/* the last interval */
		entry.sample_slots[(inter-1)*3].begin = entry.sample_slots[(inter-2)*3 ].end;
		entry.sample_slots[(inter-1)*3].end = entry.end_tmtz;

	}

        for (int tmp = 0; tmp < inter; tmp++)
        {
		Timestamp b_ts;
		Timestamp e_ts;

		char	buf_b[MAXDATELEN + 1];
		char	buf_e[MAXDATELEN + 1];

		struct pg_tm tt_b, *tm_b = &tt_b;
		struct pg_tm tt_e, *tm_e = &tt_e;

		fsec_t          fsec_b;
		fsec_t          fsec_e;

		b_ts = timestamptz2timestamp(entry.sample_slots[tmp * 3].begin);
		e_ts = timestamptz2timestamp(entry.sample_slots[tmp * 3].end);

		if (timestamp2tm(b_ts, NULL, tm_b, &fsec_b, NULL, NULL) == 0)
		{
			EncodeDateTime(tm_b, fsec_b, false, 0, NULL, DateStyle, buf_b);
		}

		if (timestamp2tm(e_ts, NULL, tm_e, &fsec_e, NULL, NULL) == 0)
		{
			EncodeDateTime(tm_e, fsec_e, false, 0, NULL, DateStyle, buf_e);
		}

		strncpy(entry.sample_slots[tmp * 3].begin_str, buf_b, strlen(buf_b));
		strncpy(entry.sample_slots[tmp * 3].end_str, buf_e, strlen(buf_e));
        }

	/* cal sample_sl t.sample */
	for (int i = 0; i < inter; i++)
	{
		int counts;
		char *b_str;
		char *e_str;
		int j = i*3;

		b_str = pstrdup(timestamptz_to_str(entry.sample_slots[j].begin));
		e_str = pstrdup(timestamptz_to_str(entry.sample_slots[j].end));

		resetStringInfo(&query);

		/*
		 * BUG#z6145, NOTE:
		 * the last sample should be included in the last interval
		 * not    [),[),[)
		 * should [),[),[]
		 */
		if (i < inter - 1)
		{
			appendStringInfo(&query, "SELECT count(*) FROM hsh_result WHERE backend_type = 'client backend' \
							AND ts>='%s'::timestamptz AND ts<'%s'::timestamptz;",
							b_str,
							e_str);
		}
		/* Begin - BUG#z6145 */
		else
		{
			/*
 			 * i = inter -1, this is the last interval
 			 * ts<='%s'
 			 */
			appendStringInfo(&query, "SELECT count(*) FROM hsh_result WHERE backend_type = 'client backend' \
							AND ts>='%s'::timestamptz AND ts<='%s'::timestamptz;",
							b_str,
							e_str);
		}
		/* End - BUG#z6145 */

		ret = SPI_execute(query.data, false, 0);
        	if (ret != SPI_OK_SELECT)
        	{
              		SPI_finish();
              		elog(ERROR, "SPI_exec failed: %d, query : %s", ret, query.data);
        	}
		if (SPI_processed != 1)
        	{
                	elog(ERROR, "not a singleton result: %s", query.data);
        	}

        	counts = DatumGetInt32(SPI_getbinval(SPI_tuptable->vals[0],
                                                           SPI_tuptable->tupdesc,
                                                           1, &isnull));
       		if (isnull)
        	{
               		elog(ERROR, "null result for query: %s", query.data);
        	}
		entry.sample_slots[j].sample = counts;

	}

	/* cal sample_slot.event and count */
	for (int i = 0; i < inter; i++)
	{
		SPITupleTable   *spituptable;
		TupleDesc       spitupdesc;
		char *b_str;
                char *e_str;
               	int j = i*3;

                b_str = pstrdup(timestamptz_to_str(entry.sample_slots[j].begin));
                e_str = pstrdup(timestamptz_to_str(entry.sample_slots[j].end));

		resetStringInfo(&query);

		/*
		 * BUG#z6145, NOTE:
		 * the last sample should be included in the last interval
		 * not    [),[),[)
		 * should [),[),[]
		 */
		if (i < inter - 1)
		{
			appendStringInfo(&query, "SELECT event,count(*) AS cnt FROM %s WHERE backend_type = 'client backend' \
							AND ts>='%s' AND ts<'%s' \
							GROUP BY event ORDER BY cnt DESC LIMIT 3;",
							HSH_RESULT,
							b_str,
							e_str);
		}
		/* Begin - BUG#z6145 */
		else
		{
			/*
			 * i = inter -1, this is the last interval
			 * ts <='%s'
			 */
			appendStringInfo(&query, "SELECT event,count(*) AS cnt FROM %s WHERE backend_type = 'client backend' \
							AND ts>='%s' AND ts<='%s' \
							GROUP BY event ORDER BY cnt DESC LIMIT 3;",
							HSH_RESULT,
							b_str,
							e_str);
		}

		ret = SPI_execute(query.data, false, 0);
		spituptable = SPI_tuptable;
		spitupdesc = spituptable->tupdesc;
		if (spi_tupdesc->natts != 2)
	        {
        	        elog(ERROR, "More columns returned: %s", query.data);
        	}

	        for (int r = 0; r < SPI_processed; r++)
        	{
			HeapTuple       spituple;
			char		*event;

                	spituple = spituptable->vals[r];
                	event = SPI_getvalue(spituple, spitupdesc, 1);
                	cnt = SPI_getvalue(spituple, spitupdesc, 2);

                	strncpy(entry.sample_slots[j + r].wait_event, event, STR_LEN - 1);
                	entry.sample_slots[j + r].count = pg_strtouint64(cnt, NULL, 0);

			/* cal sample_slot.count_d_sample */
			entry.sample_slots[j + r].count_d_sample = div_uint64_round_2(entry.sample_slots[j + r].count, entry.sample_slots[j].sample);

        	}

	}

	/*
	 * SQL list
	 * select from pg_stat_statement
	 * select from hsh_statements
	 * store in entry.sql_slots
	 */
	memset(entry.sql_slots, 0, sizeof(entry.sql_slots));  /* BUG#z6130 */

	entry.sql_slot_counts = entry.topsql_with_topevent_counts;	
	for (int i = 0; i < entry.sql_slot_counts; i++)
	{

                Datum   datum;
                text    *txt;
                char    *cstr;
		char	int64_str[32];

		entry.sql_slots[i].query_id = entry.topsql_with_topevents[i].query_id;
		snprintf(int64_str, sizeof(int64_str), INT64_FORMAT, entry.sql_slots[i].query_id);
		
		resetStringInfo(&query);
		/* first: check sql text from pg_stat_statements */
		appendStringInfo(&query, "SELECT query FROM public.pg_stat_statements where queryid = '%s';",
						int64_str);
		ret = SPI_execute(query.data, false, 0);
                if (ret != SPI_OK_SELECT)
                {
                        elog(ERROR, "SPI_execute failed: error code %d, query: %s", ret, query.data);
                }

		if (SPI_processed == 1)
		{
			spi_tuptable = SPI_tuptable;
               		spi_tupdesc = spi_tuptable->tupdesc;

			if (spi_tupdesc->natts != 1)
                	{
                        	elog(ERROR, "More columns returned for query: %s",query.data);
                	}

                	spi_tuple = spi_tuptable->vals[0];
                	datum = SPI_getbinval(spi_tuple, spi_tupdesc, 1, &isnull);
			if (!isnull)
			{
                		txt = DatumGetTextPP(datum);
                		cstr = text_to_cstring(txt);

                		strncpy(entry.sql_slots[i].text, cstr, (STR_LEN * 2) - 1);

				pfree(cstr);
			}
		}
		else
		{
			/* second: check sql text from hsh_statements */
			resetStringInfo(&query);
			appendStringInfo(&query, "SELECT query FROM %s.%s where query_id = '%s';",
                                                HSH_HISTORY_NAMESPACE,
                                                HSH_STATEMENTS,
                                                int64_str);
			ret = SPI_execute(query.data, false, 0);
			if (ret != SPI_OK_SELECT)
			{
				elog(ERROR, "SPI_execute failed: error code %d, query: %s", ret, query.data);
	                }

			if (SPI_processed == 1)
	                {
        	                spi_tuptable = SPI_tuptable;
                	        spi_tupdesc = spi_tuptable->tupdesc;

                        	if (spi_tupdesc->natts != 1)
                        	{
                                	elog(ERROR, "More columns returned for query: %s",query.data);
                        	}

                        	spi_tuple = spi_tuptable->vals[0];
                        	datum = SPI_getbinval(spi_tuple, spi_tupdesc, 1, &isnull);
                        	if (!isnull)
                        	{
                                	txt = DatumGetTextPP(datum);
                                	cstr = text_to_cstring(txt);

                                	strncpy(entry.sql_slots[i].text, cstr, (STR_LEN * 2) - 1);

                                	pfree(cstr);
                        	}
			}
			else
			{
				memset(entry.sql_slots[i].text, 0, sizeof(entry.sql_slots[i].text));
			}
                }

	}

	SPI_finish();
}
