/*-------------------------------------------------------------------------
 *
 * sql_firewall.c
 *		Prevent query execution which is not allowd by the rules.
 *
 * Copyright (c) 2015, Uptime Technologies, LLC
 *
 * sql_firewall is built on the top of pg_stat_statements.
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "access/genam.h"
#include "access/hash.h"
#include "access/heapam.h"
#include "access/parallel.h"
#include "access/stratnum.h"
#include "access/table.h"
#include "access/tableam.h"
#include "access/xact.h"
#include "catalog/indexing.h"
#include "catalog/namespace.h"
#include "catalog/pg_namespace.h"
#include "executor/executor.h"
#include "executor/instrument.h"
#include "executor/spi.h"
#include "funcapi.h"
#include "lib/stringinfo.h"
#include "mb/pg_wchar.h"
#include "miscadmin.h"
#include "nodes/nodeFuncs.h"
#include "parser/analyze.h"
#include "parser/parsetree.h"
#include "parser/scanner.h"
#include "parser/scansup.h"
#include "pgstat.h"
#include "storage/fd.h"
#include "storage/ipc.h"
#include "storage/lwlock.h"
#include "storage/shmem.h"
#include "storage/spin.h"
#include "tcop/cmdtag.h"
#include "tcop/tcopprot.h"
#include "tcop/utility.h"
#include "utils/builtins.h"
#include "utils/acl.h"
#include "utils/fmgroids.h"
#include "utils/guc.h"
#include "utils/memutils.h"
#include "utils/queryjumble.h"
#include "utils/lsyscache.h"
#include "utils/rel.h"
#include "utils/relcache.h"
#include "se_features_interface.h"
#include "pg_contrib_audit.h"


PG_MODULE_MAGIC;
/* tables, views and index name */
#define RULE_RELATION_NAME			"sql_firewall_rec"
#define RULE_INDEX_NAME				"sql_firewall_rec_key_idx"
#define RULE_STATEMENTS_VIEW_NAME	"sql_firewall_statements"
#define RULE_WHITE_LIST_VIEW_NAME	"sql_firewall_white_list"
#define RULE_BLACK_LIST_VIEW_NAME	"sql_firewall_black_list"
#define RULE_STAT_VIEW_NAME			"sql_firewall_stat"
#define STAT_REC_RELATION_NAME	"sql_firewall_stat_rec"
#define HIGH_RISK_RELATION_NAME		"sql_high_risk_rec"
#define HIGH_RISK_INDEX_NAME		"sql_high_risk_rec_statement_idx"

/* table sql_firewall_rec column define */
#define Natts_sql_firewall_rec			9
#define Anum_sql_firewall_rec_userid		1
#define Anum_sql_firewall_rec_queryid		2
#define Anum_sql_firewall_rec_query		3
#define Anum_sql_firewall_rec_original_query	4
#define Anum_sql_firewall_rec_calls		5
#define Anum_sql_firewall_rec_encoding		6
#define Anum_sql_firewall_rec_statement_type	7
#define Anum_sql_firewall_rec_warning_count	8
#define Anum_sql_firewall_rec_error_count	9

/* table sql_high_risk_rec column define */
#define Natts_sql_high_risk_rec			4
#define Anum_sql_high_risk_rec_statement	1
#define Anum_sql_high_risk_rec_description	2
#define Anum_sql_high_risk_rec_warning_count	3
#define Anum_sql_high_risk_rec_error_count	4

/* table sql_firewall_stat_rec column define */
#define Natts_sql_firewall_stat_rec		2
#define Anum_sql_firewall_stat_rec_warning	1
#define Anum_sql_firewall_stat_rec_error	2

/*
 * Hashtable key that defines the identity of
 * a hashtable entry.  We separate queries by
 * user and by database even if they are otherwise
 * identical.
 */
typedef struct sqlfirewallPgssHashKey
{
	Oid			dbid;			/* database OID */
	Oid			userid;			/* user OID */
	uint64		queryid;		/* query identifier */
	int			statement_type;	/* white statement or black statement */
} sqlfirewallPgssHashKey;

/*
 * The actual stats counters kept within sqlfirewallPgssEntry.
 */
typedef struct SqlFirewallCounters
{
	int64		calls;			/* # of times executed */
	int64		warning_count;	/* # of blacklist warnings */
	int64		error_count;	/* # of blacklist errors */
	uint64		change_count;	/* counter changes since creation */
	uint64		persisted_count; /* changes written to the table */
} SqlFirewallCounters;

/* black and white list. */
typedef enum
{
	SQL_FIREWALL_STATEMENT_ALL = 0,
	SQL_FIREWALL_STATEMENT_WHITE = 1,
	SQL_FIREWALL_STATEMENT_BLACK = 2
} SqlFirewallStatementType;

/* Statistics per statement. */
typedef struct sqlfirewallPgssEntry
{
	sqlfirewallPgssHashKey key;		/* hash key of entry - MUST BE FIRST */
	SqlFirewallCounters	counters;	/* the statistics for this query */
	int			statement_type;		/* white statement or black statement */
	slock_t		mutex;				/* protects the counters only */
} sqlfirewallPgssEntry;

/* sql_firewall query id */
typedef struct SqlFirewallQueryId
{
	uint64		queryid;
	char	   *query;
	char	   *original_query;
	bool		is_management;
} SqlFirewallQueryId;

typedef struct SqlFirewallPersistedRule
{
	Oid			userid;
	uint64		queryid;
	char	   *query;
	char	   *original_query;
	int64		calls;
	int64		warning_count;
	int64		error_count;
	int			encoding;
	int			statement_type;
} SqlFirewallPersistedRule;

typedef enum HighRiskStatementKind
{
	HIGH_RISK_NONE = 0,
	HIGH_RISK_ALTER,
	HIGH_RISK_UPDATE_WITHOUT_WHERE,
	HIGH_RISK_DELETE_WITHOUT_WHERE,
	HIGH_RISK_DROP_TABLE,
	HIGH_RISK_TRUNCATE_TABLE,
	HIGH_RISK_DROP_DATABASE,
	HIGH_RISK_DROP_SCHEMA
} HighRiskStatementKind;

typedef struct HighRiskHashKey
{
	Oid			dbid;
	int			kind;
} HighRiskHashKey;

typedef struct HighRiskHashEntry
{
	HighRiskHashKey key;
	slock_t		mutex;
	int64		warning_count;
	int64		error_count;
	uint64		change_count;
	uint64		persisted_count;
} HighRiskHashEntry;

typedef struct SqlFirewallStatCountHashKey
{
	Oid			dbid;
} SqlFirewallStatCountHashKey;

typedef struct SqlFirewallStatCountHashEntry
{
	SqlFirewallStatCountHashKey key;
	slock_t		mutex;
	int64		warning_count;
	int64		error_count;
	uint64		change_count;
	uint64		persisted_count;
} SqlFirewallStatCountHashEntry;

/*
 * Global shared state
 */
typedef struct sqlfirewallPgssSharedState
{
	LWLock	   *lock;			/* protects hashtable search/modification */
} sqlfirewallPgssSharedState;

/*
 * Struct for tracking locations/lengths of constants during normalization
 */
typedef LocationLen pgssLocationLen;
typedef JumbleState pgssJumbleState;

/*---- Local variables ----*/

/* Current nesting depth of ExecutorRun+ProcessUtility calls */
static int	nested_level = 0;

/* Current PREPARE or EXECUTE utility context; excluded from the firewall. */
static int	prepared_statement_level = 0;

/* Saved hook values in case of unload */
static shmem_startup_hook_type prev_shmem_startup_hook = NULL;
static post_parse_analyze_hook_type prev_post_parse_analyze_hook = NULL;
static ExecutorStart_hook_type prev_ExecutorStart = NULL;
static ExecutorRun_hook_type prev_ExecutorRun = NULL;
static ExecutorFinish_hook_type prev_ExecutorFinish = NULL;
static ExecutorEnd_hook_type prev_ExecutorEnd = NULL;
static ExecutorCheckPerms_hook_type prev_ExecutorCheckPerms = NULL;
static ProcessUtility_hook_type prev_ProcessUtility = NULL;

/* Links to shared memory state */
static sqlfirewallPgssSharedState *pgss = NULL;
static HTAB *pgss_hash = NULL;
static HTAB *high_risk_hash = NULL;
static HTAB *stat_count_hash = NULL;
static SqlFirewallStatCountHashEntry *current_stat_count = NULL;
static bool rule_hash_loaded = false;
static bool high_risk_hash_loaded = false;

/*---- GUC variables ----*/

typedef enum
{
	PGSS_TRACK_NONE,			/* track no statements */
	PGSS_TRACK_TOP,				/* only top level statements */
	PGSS_TRACK_ALL				/* all statements, including nested ones */
}	SqlFirewallPGSSTrackLevel;

typedef enum
{
	PGFW_MODE_DISABLED,
	PGFW_MODE_LEARNING,
	PGFW_MODE_PERMISSIVE,
	PGFW_MODE_ENFORCING
}	SqlFirewallPGFWMode;

static const struct config_enum_entry mode_options[] =
{
	{"disabled", PGFW_MODE_DISABLED, false},
	{"learning", PGFW_MODE_LEARNING, false},
	{"permissive", PGFW_MODE_PERMISSIVE, false},
	{"enforcing", PGFW_MODE_ENFORCING, false},
	{NULL, 0, false}
};

static int	pgfw_mode;			/* firewall mode */

static int	pgss_max;			/* max # statements to track */
static int	pgss_track;			/* tracking level */
static bool pgss_save;			/* whether to save stats across shutdown */


#define pgss_enabled() \
	(pgss_track == PGSS_TRACK_ALL || \
	(pgss_track == PGSS_TRACK_TOP && nested_level == 0))

/*---- Function declarations ----*/

void		_PG_init(void);
void		_PG_fini(void);

PG_FUNCTION_INFO_V1(sql_firewall_reset);
PG_FUNCTION_INFO_V1(sql_firewall_statements);
PG_FUNCTION_INFO_V1(sql_firewall_stat_error_count);
PG_FUNCTION_INFO_V1(sql_firewall_stat_warning_count);
PG_FUNCTION_INFO_V1(sql_firewall_stat_reset);
PG_FUNCTION_INFO_V1(sql_firewall_export_rule);
PG_FUNCTION_INFO_V1(sql_firewall_import_rule);
PG_FUNCTION_INFO_V1(sql_firewall_white_list);
PG_FUNCTION_INFO_V1(sql_firewall_black_list);
PG_FUNCTION_INFO_V1(import_white_list);
PG_FUNCTION_INFO_V1(import_white_list_for_user);
PG_FUNCTION_INFO_V1(import_white_list_batch);
PG_FUNCTION_INFO_V1(import_white_list_batch_for_user);
PG_FUNCTION_INFO_V1(export_white_list);
PG_FUNCTION_INFO_V1(export_white_list_for_user);
PG_FUNCTION_INFO_V1(export_white_list_sql);
PG_FUNCTION_INFO_V1(export_white_list_sql_for_user);
PG_FUNCTION_INFO_V1(delete_all_white_list);
PG_FUNCTION_INFO_V1(delete_all_white_list_for_user);
PG_FUNCTION_INFO_V1(delete_white_list);
PG_FUNCTION_INFO_V1(delete_white_list_for_user);
PG_FUNCTION_INFO_V1(delete_white_list_batch);
PG_FUNCTION_INFO_V1(delete_white_list_batch_for_user);
PG_FUNCTION_INFO_V1(import_black_list);
PG_FUNCTION_INFO_V1(import_black_list_for_user);
PG_FUNCTION_INFO_V1(import_black_list_batch);
PG_FUNCTION_INFO_V1(import_black_list_batch_for_user);
PG_FUNCTION_INFO_V1(export_black_list);
PG_FUNCTION_INFO_V1(export_black_list_for_user);
PG_FUNCTION_INFO_V1(export_black_list_sql);
PG_FUNCTION_INFO_V1(export_black_list_sql_for_user);
PG_FUNCTION_INFO_V1(export_all_sql);
PG_FUNCTION_INFO_V1(dispaly_single_sql);
PG_FUNCTION_INFO_V1(import_high_risk_statement);
PG_FUNCTION_INFO_V1(remove_high_risk_statement);
PG_FUNCTION_INFO_V1(delete_all_black_list);
PG_FUNCTION_INFO_V1(delete_all_black_list_for_user);
PG_FUNCTION_INFO_V1(delete_black_list);
PG_FUNCTION_INFO_V1(delete_black_list_for_user);
PG_FUNCTION_INFO_V1(delete_black_list_batch);
PG_FUNCTION_INFO_V1(delete_black_list_batch_for_user);

static void pgss_shmem_startup(void);
static void check_sql_firewall_loaded(void);
static void check_sql_firewall_function_permission(const char *funcname);
static void check_sql_firewall_relation_permission(const char *relname);
static Oid sql_firewall_role_name_to_oid(Name username);
static void pgss_post_parse_analyze(ParseState *pstate, Query *query,
					JumbleState *jstate);
static void pgss_ExecutorStart(QueryDesc *queryDesc, int eflags);
static void pgss_ExecutorRun(QueryDesc *queryDesc,
				 ScanDirection direction,
				 uint64 count,
				 bool execute_once);
static void pgss_ExecutorFinish(QueryDesc *queryDesc);
static void pgss_ExecutorEnd(QueryDesc *queryDesc);
static bool pgss_ExecutorCheckPerms(List *rangeTable,
									bool flag);
static void pgss_ProcessUtility(PlannedStmt *pstmt, const char *queryString,
					bool readOnlyTree,
					ProcessUtilityContext context, ParamListInfo params,
					QueryEnvironment *queryEnv,
					DestReceiver *dest, QueryCompletion *qc);
static uint32 pgss_hash_fn(const void *key, Size keysize);
static int	pgss_match_fn(const void *key1, const void *key2, Size keysize);
static uint64 pgss_hash_string(const char *str);
static uint64 pgss_hash_normalized_query(const char *query,
					 pgssJumbleState *jstate);
static char *pgss_canonicalize_query(const char *query);
static bool pgss_canonical_space_required(unsigned char left,
										  unsigned char right);
static void pgss_store(const char *query, uint64 queryId,
		   double total_time, uint64 rows,
		   pgssJumbleState *jstate,
		   HighRiskStatementKind high_risk,
		   bool parse_only,
		   bool check_firewall,
		   bool count_call,
		   Oid userid);
static void pg_stat_statements_internal(FunctionCallInfo fcinfo,
							bool showtext, int statement_type);
static Size pgss_memsize(void);
static sqlfirewallPgssEntry *entry_alloc(sqlfirewallPgssHashKey *key,
			int statement_type);
static bool pgss_restore(Oid userid, uint64 queryid, const char *query,
				 const char *original_query, int64 calls, int64 warning_count,
				 int64 error_count, int statement_type, bool persist);
static bool get_rule_relation_oids(Oid *relid, Oid *indexid);
static Oid get_stat_count_relation_oid(void);
static void ensure_stat_count_loaded(void);
static void persist_stat_count(void);
static List *load_persisted_rules(void);
static List *append_runtime_only_rules(List *rules);
static void free_persisted_rules(List *rules);
static int persisted_rule_cmp(const ListCell *a, const ListCell *b);
static void ensure_rule_hash_loaded(void);
static void persist_rule(Oid userid, uint64 queryid, const char *query,
						 const char *original_query, int64 calls,
						 int64 warning_count, int64 error_count, int encoding,
						 int statement_type);
static void persist_hash_entry(const sqlfirewallPgssHashKey *key);
static void persist_changed_rule_counts(void);
static void delete_persisted_rule(const sqlfirewallPgssHashKey *key);
static void delete_all_persisted_rules(void);
static List *sql_firewall_compute_queryid(const char *sqltext,
				 bool normalize_query);
static uint64 sql_firewall_compute_single_queryid(const char *sqltext,
					 uint64 fallback_queryid, bool *is_management);
static void sql_firewall_free_queryids(List *queryids);
static void reject_prepare_in_list_import(const char *sqltext);
static bool import_list_sql(Oid userid, const char *sqltext, int statement_type);
static bool import_list_file(Oid userid, const char *filepath, int statement_type);
static char *read_sql_file(const char *filepath);
static void require_list_sql_terminator(const char *sqltext);
static bool add_list_rule(Oid userid, uint64 queryid, const char *query,
				 const char *original_query, int statement_type, int64 calls);
static bool export_list_file(const char *filepath, Oid userid, bool filter_user,
				 int statement_type);
static bool export_list_sql_file(const char *filepath, Oid userid,
				 bool filter_user, int statement_type);
static bool delete_list_sql(Oid userid, bool filter_user, const char *sqltext,
				 int statement_type);
static bool delete_list_file(Oid userid, const char *filepath, int statement_type);
static bool delete_list_entries(Oid userid, bool filter_user, uint64 queryid,
				 bool filter_queryid, int statement_type);
static sqlfirewallPgssEntry *find_entry_for_user(Oid userid, uint64 queryid,
				 int statement_type);
static sqlfirewallPgssEntry *find_effective_entry(Oid userid, uint64 queryid,
				 int statement_type);
static HighRiskStatementKind is_high_risk_query(Query *query);
static bool check_high_risk_query(HighRiskStatementKind kind, const char *query);
static bool was_high_risk_checked(PlannedStmt *plannedstmt, const char *query);
static HighRiskStatementKind is_high_risk_utility(Node *parsetree);
static void ensure_high_risk_hash_loaded(void);
static void set_high_risk_hash(HighRiskStatementKind kind, int64 warning_count,
							   int64 error_count);
static void remove_high_risk_hash(HighRiskStatementKind kind);
static bool is_high_risk_hash(HighRiskStatementKind kind);
static void high_risk_stat_increment(HighRiskStatementKind kind, bool error);
static void persist_changed_high_risk_counts(void);
static HighRiskStatementKind high_risk_kind_from_name(const char *name);
static const char *high_risk_kind_name(HighRiskStatementKind kind);
static const char *high_risk_kind_description(HighRiskStatementKind kind);
static void get_high_risk_relation_oids(Oid *relid, Oid *indexid);
static bool is_sql_firewall_management_function(Oid funcid);
static bool is_sql_firewall_list_function(Oid funcid);
static bool is_sql_firewall_management_utility(Node *parsetree);
static bool is_sql_firewall_relation_access(List *rtable);
static bool is_sql_firewall_function_access(List *rtable);
static bool is_sql_firewall_management_query(Query *query);
static bool is_sql_firewall_management_plan(PlannedStmt *plannedstmt);
static bool is_inside_trigger(void);
static bool sql_firewall_xact_read_only(void);
static void entry_reset(void);
static char *generate_normalized_query(pgssJumbleState *jstate, const char *query,
						  int *query_len_p, int encoding);
static void fill_in_constant_lengths(pgssJumbleState *jstate, const char *query);
static int	comp_location(const void *a, const void *b);
static bool Exclude_internal_process(const char *str);


/*
 * _PG_init
 *
 * Module load callback.
 */
void
_PG_init(void)
{
	/*
	 * In order to create our shared memory area, we have to be loaded via
	 * shared_preload_libraries.  If not, fall out without hooking into any of
	 * the main system.  (We don't throw error here because it seems useful to
	 * allow the pg_stat_statements functions to be created even when the
	 * module isn't active.  The functions must protect themselves against
	 * being called then, however.)
	 */
	if (!process_shared_preload_libraries_in_progress)
		return;

	/*
	 * Define (or redefine) custom GUC variables.
	 */
	DefineCustomIntVariable("sql_firewall.max",
	  "Sets the maximum number of statements tracked by sql_firewall.",
							NULL,
							&pgss_max,
							5000,
							100,
							INT_MAX,
							PGC_POSTMASTER,
							0,
							NULL,
							NULL,
							NULL);

	pgss_track = PGSS_TRACK_ALL;
	pgss_save = true;

	DefineCustomEnumVariable("sql_firewall.firewall",
			   "Enable SQL Firewall feature.",
							 NULL,
							 &pgfw_mode,
							 PGFW_MODE_DISABLED,
							 mode_options,
							 PGC_SIGHUP,
							 0,
							 NULL,
							 NULL,
							 NULL);

	EmitWarningsOnPlaceholders("sql_firewall");

	/*
	 * Request additional shared resources.  (These are no-ops if we're not in
	 * the postmaster process.)  We'll allocate or attach to the shared
	 * resources in pgss_shmem_startup().
	 */
	RequestAddinShmemSpace(pgss_memsize());
	RequestNamedLWLockTranche("sql_firewall", 1);
	EnableQueryId();

	/*
	 * Install hooks.
	 */
	prev_shmem_startup_hook = shmem_startup_hook;
	shmem_startup_hook = pgss_shmem_startup;
	prev_post_parse_analyze_hook = post_parse_analyze_hook;
	post_parse_analyze_hook = pgss_post_parse_analyze;
	prev_ExecutorStart = ExecutorStart_hook;
	ExecutorStart_hook = pgss_ExecutorStart;
	prev_ExecutorRun = ExecutorRun_hook;
	ExecutorRun_hook = pgss_ExecutorRun;
	prev_ExecutorFinish = ExecutorFinish_hook;
	ExecutorFinish_hook = pgss_ExecutorFinish;
	prev_ExecutorEnd = ExecutorEnd_hook;
	ExecutorEnd_hook = pgss_ExecutorEnd;
	prev_ExecutorCheckPerms = ExecutorCheckPerms_hook;
	ExecutorCheckPerms_hook = pgss_ExecutorCheckPerms;
	prev_ProcessUtility = ProcessUtility_hook;
	ProcessUtility_hook = pgss_ProcessUtility;
}


/*
 * _PG_fini
 *
 * Module unload callback.
 */
void
_PG_fini(void)
{
	/* Uninstall hooks. */
	shmem_startup_hook = prev_shmem_startup_hook;
	post_parse_analyze_hook = prev_post_parse_analyze_hook;
	ExecutorStart_hook = prev_ExecutorStart;
	ExecutorRun_hook = prev_ExecutorRun;
	ExecutorFinish_hook = prev_ExecutorFinish;
	ExecutorEnd_hook = prev_ExecutorEnd;
	ExecutorCheckPerms_hook = prev_ExecutorCheckPerms;
	ProcessUtility_hook = prev_ProcessUtility;
}

/*
 * pgss_shmem_startup
 *
 * shmem_startup hook: allocate or attach to shared memory.  Persistent rules
 * are loaded from hg_security.sql_firewall_rec later, when a backend has a
 * database session.
 */
static void
pgss_shmem_startup(void)
{
	bool		found;
	HASHCTL		info;

	if (prev_shmem_startup_hook)
		prev_shmem_startup_hook();

	/* reset in case this is a restart within the postmaster */
	pgss = NULL;
	pgss_hash = NULL;
	high_risk_hash = NULL;
	stat_count_hash = NULL;
	current_stat_count = NULL;
	rule_hash_loaded = false;
	high_risk_hash_loaded = false;

	/*
	 * Create or attach to the shared memory state, including hash table
	 */
	LWLockAcquire(AddinShmemInitLock, LW_EXCLUSIVE);

	pgss = ShmemInitStruct("sql_firewall",
						   sizeof(sqlfirewallPgssSharedState),
						   &found);

	if (!found)
	{
		/* First time through ... */
		pgss->lock = &(GetNamedLWLockTranche("sql_firewall"))->lock;
	}

	memset(&info, 0, sizeof(info));
	info.keysize = sizeof(sqlfirewallPgssHashKey);
	info.entrysize = sizeof(sqlfirewallPgssEntry);
	info.hash = pgss_hash_fn;
	info.match = pgss_match_fn;
	pgss_hash = ShmemInitHash("sql_firewall hash",
							  pgss_max, pgss_max,
							  &info,
							  HASH_ELEM | HASH_FUNCTION | HASH_COMPARE);

	memset(&info, 0, sizeof(info));
	info.keysize = sizeof(HighRiskHashKey);
	info.entrysize = sizeof(HighRiskHashEntry);
	high_risk_hash = ShmemInitHash("sql_firewall high risk hash",
								  20, pgss_max,
								  &info, HASH_ELEM | HASH_BLOBS);

	memset(&info, 0, sizeof(info));
	info.keysize = sizeof(SqlFirewallStatCountHashKey);
	info.entrysize = sizeof(SqlFirewallStatCountHashEntry);
	stat_count_hash = ShmemInitHash("sql_firewall stat count hash",
								 10, pgss_max,
								 &info, HASH_ELEM | HASH_BLOBS);

	LWLockRelease(AddinShmemInitLock);
}

/*
 * pgss_post_parse_analyze
 *
 * Post-parse-analysis hook: mark query with a queryId.
 */
static void
pgss_post_parse_analyze(ParseState *pstate, Query *query, JumbleState *jstate)
{
	HighRiskStatementKind high_risk_kind;
	uint64		firewall_queryid;

	if (prev_post_parse_analyze_hook)
		prev_post_parse_analyze_hook(pstate, query, jstate);


	if (prepared_statement_level > 0)
		return;

	if (Exclude_internal_process(pstate->p_sourcetext))
		return;

	/*
	 * SQL executed by a trigger is an implementation detail
	 * of its caller.
	 */
	if (is_inside_trigger() && pgfw_mode != PGFW_MODE_LEARNING)
		return;

	/* Safety check... */
	if (!pgss || !pgss_hash)
		return;

	ensure_stat_count_loaded();
	persist_stat_count();
	ensure_rule_hash_loaded();
	ensure_high_risk_hash_loaded();
	persist_changed_rule_counts();
	persist_changed_high_risk_counts();

	/*
	 * Nested parse analysis is an intermediate step of
	 * an enclosing query or utility command.  Retain its
	 * normalized form while learning, but never enforce
	 * it separately: only SQL submitted by the user is
	 * checked in the permissive and enforcing modes.
	 */
	if (nested_level > 0 && pgfw_mode != PGFW_MODE_LEARNING)
		return;

	/*
	 * Utility statements get queryId zero. We do this even
	 * in cases where the statement contains an optimizable
	 * statement for which a queryId could be derived (such
	 * as EXPLAIN or DECLARE CURSOR). For such cases, runtime
	 * control will first go through ProcessUtility and then the
	 * executor, and we don't want the executor hooks to do
	 * anything, since we are already measuring the statement's
	 * costs at the utility level.
	 */
	if (query->utilityStmt || query->queryId == UINT64CONST(0))
		return;

	/* The management function for sql_firewall was accessed. */
	if (is_sql_firewall_management_query(query))
		return;

	/*
	 * Compute the firewall's OID-independent identity without changing the
	 * PostgreSQL-owned Query.queryId seen by other extensions and facilities.
	 */
	if (nested_level == 0)
		firewall_queryid = pgss_hash_normalized_query(pstate->p_sourcetext, jstate);
	else
		firewall_queryid = query->queryId;

	/* there check without where clause in
	 * update and delete statement.
	 */
	high_risk_kind = is_high_risk_query(query);
	check_high_risk_query(high_risk_kind, pstate->p_sourcetext);

	/*
	 * If we were able to identify any ignorable constants,
	 * we immediately create a hash table entry for the query,
	 * so that we can record the normalized form of the query string.
	 * If there were no such constants, the normalized string
	 * would be the same as the query text anyway, so there's
	 * no need for an early entry.
	 */
	if (jstate && jstate->clocations_count > 0)
		pgss_store(pstate->p_sourcetext,
			   firewall_queryid,
			   0,
			   0,
			   jstate,
			   false,
			   true,
			   false,
			   false,
			   GetUserId());
}

/*
 * pgss_ExecutorStart
 *
 * ExecutorStart hook: start up tracking if needed
 */
static void
pgss_ExecutorStart(QueryDesc *queryDesc, int eflags)
{
	uint64		queryId = UINT64CONST(0);
	bool		high_risk_checked = false;
	bool		is_management = false;
	bool		specified_process = false;

	/* Don't deal with specified process. */
	specified_process = Exclude_internal_process(queryDesc->sourceText);

	/*
	 * In learning mode, if the statement
	 * does not involve `sql_firewall functions
	 * or views, it is recorded for learning.
	 */
	if (prepared_statement_level == 0 &&
		(!is_inside_trigger() || pgfw_mode == PGFW_MODE_LEARNING) &&
		(nested_level == 0 || pgfw_mode == PGFW_MODE_LEARNING) &&
		queryDesc->plannedstmt->queryId != 0 &&
		queryDesc->plannedstmt->canSetTag && pgss_enabled() &&
		!is_sql_firewall_management_plan(queryDesc->plannedstmt) &&
		!specified_process)
	{
		if (nested_level == 0 && !is_inside_trigger())
			queryId = sql_firewall_compute_single_queryid(queryDesc->sourceText,
												 queryDesc->plannedstmt->queryId,
												 &is_management);
		else
			queryId = queryDesc->plannedstmt->queryId;

		if (!is_management)
		{
			high_risk_checked = was_high_risk_checked(queryDesc->plannedstmt,
											  queryDesc->sourceText);
			pgss_store(queryDesc->sourceText, queryId,
					   0, 0, NULL, false, true,
					   !high_risk_checked, false, GetUserId());
		}
	}

	if (prev_ExecutorStart)
		prev_ExecutorStart(queryDesc, eflags);
	else
		standard_ExecutorStart(queryDesc, eflags);

	/*
	 * If query has queryId zero, don't track it.  This prevents double
	 * counting of optimizable statements that are directly contained in
	 * utility statements.
	 */
	if (prepared_statement_level == 0 && pgss_enabled() &&
		queryDesc->plannedstmt->queryId != 0)
	{
		/*
		 * Set up to track total elapsed time in ExecutorRun.  Make sure the
		 * space is allocated in the per-query context so it will go away at
		 * ExecutorEnd.
		 */
		if (queryDesc->totaltime == NULL)
		{
			MemoryContext oldcxt;

			oldcxt = MemoryContextSwitchTo(queryDesc->estate->es_query_cxt);
			queryDesc->totaltime = InstrAlloc(1, INSTRUMENT_ALL, false);
			MemoryContextSwitchTo(oldcxt);
		}
	}
}

/*
 * pgss_ExecutorRun
 *
 * ExecutorRun hook: all we need do is track nesting depth.
 */
static void
pgss_ExecutorRun(QueryDesc *queryDesc, ScanDirection direction, uint64 count,
				 bool execute_once)
{
	nested_level++;
	PG_TRY();
	{
		if (prev_ExecutorRun)
			prev_ExecutorRun(queryDesc, direction, count, execute_once);
		else
			standard_ExecutorRun(queryDesc, direction, count, execute_once);
		nested_level--;
	}
	PG_CATCH();
	{
		nested_level--;
		PG_RE_THROW();
	}
	PG_END_TRY();
}

/*
 * pgss_ExecutorFinish
 *
 * ExecutorFinish hook: all we need do is track nesting depth.
 */
static void
pgss_ExecutorFinish(QueryDesc *queryDesc)
{
	nested_level++;
	PG_TRY();
	{
		if (prev_ExecutorFinish)
			prev_ExecutorFinish(queryDesc);
		else
			standard_ExecutorFinish(queryDesc);
		nested_level--;
	}
	PG_CATCH();
	{
		nested_level--;
		PG_RE_THROW();
	}
	PG_END_TRY();
}

/*
 * pgss_ExecutorEnd
 *
 * ExecutorEnd hook: store results if needed.
 */
static void
pgss_ExecutorEnd(QueryDesc *queryDesc)
{
	uint64		queryId;
	bool		is_management = false;
	bool		specified_process = false;

	/* Don't deal with specified process. */
	specified_process = Exclude_internal_process(queryDesc->sourceText);

	if (prepared_statement_level > 0)
	{
		if (prev_ExecutorEnd)
			prev_ExecutorEnd(queryDesc);
		else
			standard_ExecutorEnd(queryDesc);
		return;
	}

	if (nested_level == 0 && !is_inside_trigger())
		queryId = sql_firewall_compute_single_queryid(queryDesc->sourceText,
											 queryDesc->plannedstmt->queryId,
											 &is_management);
	else
		queryId = queryDesc->plannedstmt->queryId;

	/*
	 * Only check the top-level executable statement here.  Queries executed
	 * by triggers, foreign-key checks, rules, and SQL-language functions are
	 * implementation details of that statement; checking them separately
	 * produces duplicate warnings and can reject work that the user did not
	 * submit independently.
	 */
	if ((!is_inside_trigger() || pgfw_mode == PGFW_MODE_LEARNING) &&
		(nested_level == 0 || pgfw_mode == PGFW_MODE_LEARNING) &&
		queryId != 0 && queryDesc->totaltime && pgss_enabled() &&
		!is_management &&
		!is_sql_firewall_management_plan(queryDesc->plannedstmt) &&
		!specified_process)
	{
		/*
		 * Make sure stats accumulation is done.  (Note: it's okay if several
		 * levels of hook all do this.)
		 */
		InstrEndLoop(queryDesc->totaltime);

		pgss_store(queryDesc->sourceText,
				   queryId,
				   queryDesc->totaltime->total * 1000.0,		/* convert to msec */
				   queryDesc->estate->es_processed,
				   NULL,
				   false,
				   false,
				   false,
				   true,
				   GetUserId());
	}

	if (prev_ExecutorEnd)
		prev_ExecutorEnd(queryDesc);
	else
		standard_ExecutorEnd(queryDesc);
}

/*
 * pgss_ExecutorCheckPerms
 *
 * Restrict direct access to sql_firewall's catalog relations and public
 * monitoring views to superusers.  Keep this check in the executor permission
 * hook so it also applies when one of the protected views is referenced from
 * a larger query.
 */
static bool
pgss_ExecutorCheckPerms(List *rangeTable, bool flag)
{
	static const char *const protected_relations[] =
	{
		RULE_RELATION_NAME,
		STAT_REC_RELATION_NAME,
		HIGH_RISK_RELATION_NAME,
		RULE_BLACK_LIST_VIEW_NAME,
		RULE_STAT_VIEW_NAME,
		RULE_STATEMENTS_VIEW_NAME,
		RULE_WHITE_LIST_VIEW_NAME
	};
	Oid			protected_oids[7];
	ListCell   *lc;
	int			i;
	int			arrlength = lengthof(protected_relations);

	/* Preserve the result of any previously installed permission hook. */
	if (prev_ExecutorCheckPerms &&
		!prev_ExecutorCheckPerms(rangeTable, flag))
		return false;

	/* Resolve every protected relation once for this permission check. */
	for (i = 0; i < arrlength; i++)
	{
		protected_oids[i] = get_relname_relid(protected_relations[i],
										 SECURITY_SCHEMA_NAMESPACE);
	}

	foreach(lc, rangeTable)
	{
		RangeTblEntry *rte = lfirst_node(RangeTblEntry, lc);

		if (rte->rtekind != RTE_RELATION)
			continue;

		for (i = 0; i < arrlength; i++)
		{
			if (!OidIsValid(protected_oids[i]) ||
				rte->relid != protected_oids[i])
				continue;

			check_sql_firewall_relation_permission(protected_relations[i]);

			return false;
		}
	}

	return true;
}

/*
 * pgss_ProcessUtility
 *
 * ProcessUtility hook.
 */
static void
pgss_ProcessUtility(PlannedStmt *pstmt, const char *queryString,
					bool readOnlyTree,
					ProcessUtilityContext context, ParamListInfo params,
					QueryEnvironment *queryEnv,
					DestReceiver *dest, QueryCompletion *qc)
{
	Node	   *parsetree = pstmt->utilityStmt;
	HighRiskStatementKind high_risk_kind;
	bool		builtin_high_risk;

	/*
	 * Do not inspect, learn, enforce, or collect statistics for prepared
	 * statement lifecycle commands.  The context counter also suppresses the
	 * child parse hook of PREPARE and executor hooks reached by EXECUTE.
	 */
	if (IsA(parsetree, PrepareStmt) || IsA(parsetree, ExecuteStmt) ||
		IsA(parsetree, DeallocateStmt))
	{
		bool		needs_context = !IsA(parsetree, DeallocateStmt);

		if (needs_context)
			prepared_statement_level++;
		PG_TRY();
		{
			if (prev_ProcessUtility)
				prev_ProcessUtility(pstmt, queryString, readOnlyTree,
								context, params, queryEnv, dest, qc);
			else
				standard_ProcessUtility(pstmt, queryString, readOnlyTree,
								context, params, queryEnv, dest, qc);
			if (needs_context)
				prepared_statement_level--;
		}
		PG_CATCH();
		{
			if (needs_context)
				prepared_statement_level--;
			PG_RE_THROW();
		}
		PG_END_TRY();
		return;
	}

	ensure_stat_count_loaded();
	persist_stat_count();
	ensure_rule_hash_loaded();
	ensure_high_risk_hash_loaded();
	persist_changed_rule_counts();
	persist_changed_high_risk_counts();
	high_risk_kind = is_high_risk_utility(parsetree);
	builtin_high_risk = is_high_risk_hash(high_risk_kind);

	/*
	 * If it's an EXECUTE statement, we don't track it and don't increment the
	 * nesting level.  This allows the cycles to be charged to the underlying
	 * PREPARE instead (by the Executor hooks), which is much more useful.
	 *
	 * We also don't track execution of PREPARE.  If we did, we would get one
	 * hash table entry for the PREPARE (with hash calculated from the query
	 * string), and then a different one with the same query string (but hash
	 * calculated from the query tree) would be used to accumulate costs of
	 * ensuing EXECUTEs.  This would be confusing, and inconsistent with other
	 * cases where planning time is not included at all.
	 *
	 * Likewise, we don't track execution of DEALLOCATE.
	 */
	if ((!is_inside_trigger() || pgfw_mode == PGFW_MODE_LEARNING) &&
		(nested_level == 0 || pgfw_mode == PGFW_MODE_LEARNING) &&
		pgss_enabled() &&
		!(nested_level > 0 && IsA(parsetree, VacuumStmt)) &&
		!(is_sql_firewall_management_utility(parsetree) &&
		  !builtin_high_risk) &&
		!IsA(parsetree, ExecuteStmt) &&
		!IsA(parsetree, PrepareStmt) &&
		!IsA(parsetree, DeallocateStmt))
	{
		instr_time	start;
		instr_time	duration;
		uint64		queryId;
		Oid			userid = GetUserId();
		bool		specified_process = false;

		/* Don't deal with specified process. */
		specified_process = Exclude_internal_process(queryString);

		/*
		 * SET/RESET ROLE changes the effective user during ProcessUtility.
		 * Attribute these commands to the stable session user so a recreated
		 * role OID cannot invalidate a rule learned by the same session.
		 */
		if (IsA(parsetree, VariableSetStmt))
		{
			VariableSetStmt *setstmt = (VariableSetStmt *) parsetree;

			if (setstmt->name != NULL &&
				(strcmp(setstmt->name, "role") == 0 ||
				 strcmp(setstmt->name, "session_authorization") == 0))
				userid = GetSessionUserId();
		}

		INSTR_TIME_SET_CURRENT(start);

		nested_level++;
		PG_TRY();
		{
			if (prev_ProcessUtility)
				prev_ProcessUtility(pstmt, queryString, readOnlyTree,
									context, params, queryEnv,
									dest, qc);
			else
				standard_ProcessUtility(pstmt, queryString, readOnlyTree,
										context, params, queryEnv,
										dest, qc);
			nested_level--;
		}
		PG_CATCH();
		{
			nested_level--;
			PG_RE_THROW();
		}
		PG_END_TRY();

		/* Don't deal with specified process. */
		if (specified_process)
			return;

		INSTR_TIME_SET_CURRENT(duration);
		INSTR_TIME_SUBTRACT(duration, start);

		/* Apply the same formatting normalization used by imported rules. */
		queryId = pgss_hash_normalized_query(queryString, NULL);
		pgss_store(queryString,
				   queryId,
				   INSTR_TIME_GET_MILLISEC(duration),
				   0,
				   NULL,
				   builtin_high_risk ? high_risk_kind : HIGH_RISK_NONE,
				   false,
				   true,
				   true,
				   userid);
	}
	else
	{
		if (prev_ProcessUtility)
			prev_ProcessUtility(pstmt, queryString, readOnlyTree,
								context, params, queryEnv,
								dest, qc);
		else
			standard_ProcessUtility(pstmt, queryString, readOnlyTree,
									context, params, queryEnv,
									dest, qc);
	}
}

/*
 * pgss_hash_fn
 *
 * Calculate hash value for a key.
 */
static uint32
pgss_hash_fn(const void *key, Size keysize)
{
	const sqlfirewallPgssHashKey *k = (const sqlfirewallPgssHashKey *) key;
	uint32		lohalf = (uint32) k->queryid;
	uint32		hihalf = (uint32) (k->queryid >> 32);

	return DatumGetUInt32(hash_uint32((uint32) k->dbid)) ^
		DatumGetUInt32(hash_uint32((uint32) k->userid)) ^
		DatumGetUInt32(hash_uint32(lohalf)) ^
		DatumGetUInt32(hash_uint32(hihalf)) ^
		DatumGetUInt32(hash_uint32((uint32) k->statement_type));
}

/*
 * pgss_match_fn
 *
 * Compare two keys - zero means match
 */
static int
pgss_match_fn(const void *key1, const void *key2, Size keysize)
{
	const sqlfirewallPgssHashKey *k1 = (const sqlfirewallPgssHashKey *) key1;
	const sqlfirewallPgssHashKey *k2 = (const sqlfirewallPgssHashKey *) key2;

	if (k1->dbid == k2->dbid &&
		k1->userid == k2->userid &&
		k1->queryid == k2->queryid &&
		k1->statement_type == k2->statement_type)
		return 0;
	else
		return 1;
}

/*
 * pgss_hash_string
 *
 * Given an arbitrarily long query string, produce a
 * hash for the purposes of identifying the query,
 * without normalizing constants.  Used when hashing
 * utility statements.
 */
static uint64
pgss_hash_string(const char *str)
{
	return DatumGetUInt64(hash_any_extended((const unsigned char *) str,
											strlen(str), 0));
}

/*
 * pgss_hash_normalized_query
 *
 * Hash normalized SQL for optimizable statements,
 * independently of OIDs.
 */
static uint64
pgss_hash_normalized_query(const char *query, pgssJumbleState *jstate)
{
	char	   *normalized_query = NULL;
	const char *query_to_hash = query;
	char	   *canonical_query;
	uint64		queryid;

	/* Generate a normalized query statement. */
	if (jstate && jstate->clocations_count > 0)
	{
		int			query_len = strlen(query);

		normalized_query = generate_normalized_query(jstate, query,
											 &query_len,
											 GetDatabaseEncoding());
		query_to_hash = normalized_query;
	}

	/*
	 * Remove formatting-only whitespace
	 * while preserving quoted identifiers.
	 */
	canonical_query = pgss_canonicalize_query(query_to_hash);

	/* Generate a query ID using a hash. */
	queryid = pgss_hash_string(canonical_query);
	if (queryid == UINT64CONST(0))
	{
		queryid = UINT64CONST(1);
	}

	/* free variable. */
	pfree(canonical_query);
	if (normalized_query)
	{
		pfree(normalized_query);
	}

	return queryid;
}

/*
 * pgss_canonicalize_query
 *
 * Remove formatting-only whitespace while preserving quoted identifiers.
 */
static char *
pgss_canonicalize_query(const char *query)
{
	StringInfoData result;
	bool		in_double_quote = false;
	bool		pending_space = false;
	const unsigned char *p;

	initStringInfo(&result);

	/* Iterate through the entire string. */
	for (p = (const unsigned char *) query; *p != '\0'; p++)
	{
		/*
		 * If enclosed in double quotes, preserve
		 * the original format of the string within
		 * the double quotes.
		 */
		if (*p == '"')
		{
			if (pending_space && result.len > 0 &&
				pgss_canonical_space_required(result.data[result.len - 1], *p))
			{
				appendStringInfoChar(&result, ' ');
			}

			pending_space = false;
			appendStringInfoChar(&result, *p);
			if (in_double_quote && p[1] == '"')
			{
				appendStringInfoChar(&result, p[1]);
				p++;
			}
			else
			{
				in_double_quote = !in_double_quote;
			}

			continue;
		}

		/* Spaces within double quotes must be preserved,
		 * set a flag,Skip if not within double quotes.
		 */
		if (!in_double_quote && isspace(*p))
		{
			pending_space = true;
			continue;
		}

		/* A statement terminator does not affect the query identity. */
		if (!in_double_quote && *p == ';')
		{
			pending_space = false;
			continue;
		}

		/* Spaces within double quotes must be preserved. */
		if (pending_space && result.len > 0 &&
			pgss_canonical_space_required(result.data[result.len - 1], *p))
		{
			appendStringInfoChar(&result, ' ');
		}

		pending_space = false;

		appendStringInfoChar(&result, *p);
	}

	return result.data;
}

/*
 * pgss_canonical_space_required
 *
 * Preserve whitespace only when removing it could join two word-like tokens
 * or two operator characters into a different token.  Whitespace between a
 * word and punctuation/operator is formatting-only.
 */
static bool
pgss_canonical_space_required(unsigned char left, unsigned char right)
{
	bool		left_word;
	bool		right_word;
	const char *operator_chars = "+-*/<>=~!@#%^&|`";

	left_word = isalnum(left) || left == '_' || left == '$' ||
		left == '"' || left == '\'' || left >= 0x80;
	right_word = isalnum(right) || right == '_' || right == '$' ||
		right == '"' || right == '\'' || right >= 0x80;

	if (left_word && right_word)
		return true;

	return strchr(operator_chars, left) != NULL &&
		strchr(operator_chars, right) != NULL;
}

/*
 * statement_type_name
 *
 * Return black list or white list name.
 */
static const char *
statement_type_name(int statement_type)
{
	return statement_type == SQL_FIREWALL_STATEMENT_BLACK ? "black" : "white";
}

/*
 * parse_statement_type
 *
 * Determine blacklist/whitelist type based on the string.
 */
static int
parse_statement_type(const char *value)
{
	if (pg_strcasecmp(value, "black") == 0 ||
		pg_strcasecmp(value, "b") == 0 ||
		strcmp(value, "2") == 0)
		return SQL_FIREWALL_STATEMENT_BLACK;

	return SQL_FIREWALL_STATEMENT_WHITE;
}

/*
 * stat_warning_increment
 *
 * warning increment.
 */
static void
stat_warning_increment(void)
{
	if (current_stat_count == NULL)
		return;

	SpinLockAcquire(&current_stat_count->mutex);
	current_stat_count->warning_count++;
	current_stat_count->change_count++;
	SpinLockRelease(&current_stat_count->mutex);
}

/*
 * stat_error_increment
 *
 * error increment.
 */
static void
stat_error_increment(void)
{
	if (current_stat_count == NULL)
		return;

	SpinLockAcquire(&current_stat_count->mutex);
	current_stat_count->error_count++;
	current_stat_count->change_count++;
	SpinLockRelease(&current_stat_count->mutex);
}

/*
 * find_entry_for_user
 *
 * Retrieve the allowlist and blocklist for a specified user.
 */
static sqlfirewallPgssEntry *
find_entry_for_user(Oid userid, uint64 queryid, int statement_type)
{
	sqlfirewallPgssHashKey key;
	sqlfirewallPgssEntry  *entry;

	key.dbid = MyDatabaseId;
	key.userid = userid;
	key.queryid = queryid;
	key.statement_type = statement_type;

	entry = (sqlfirewallPgssEntry *) hash_search(pgss_hash, &key, HASH_FIND, NULL);
	if (entry && entry->statement_type == statement_type)
		return entry;

	return NULL;
}

/*
 * find_effective_entry
 *
 * Priority is given to searching the global
 * allowlist/blocklist; the specific user's list
 * is checked only if no match is found there.
 */
static sqlfirewallPgssEntry *
find_effective_entry(Oid userid, uint64 queryid, int statement_type)
{
	sqlfirewallPgssEntry  *entry;

	entry = find_entry_for_user(InvalidOid, queryid, statement_type);
	if (entry)
		return entry;

	return find_entry_for_user(userid, queryid, statement_type);
}

/*
 * is_high_risk_query
 *
 * Determine whether an UPDATE or DELETE statement
 * is a high-risk operation lacking a WHERE clause.
 */
static HighRiskStatementKind
is_high_risk_query(Query *query)
{
	if (query == NULL)
		return HIGH_RISK_NONE;

	if (query->jointree != NULL && query->jointree->quals == NULL)
	{
		if (query->commandType == CMD_UPDATE)
		{
			return HIGH_RISK_UPDATE_WITHOUT_WHERE;
		}

		if (query->commandType == CMD_DELETE)
		{
			return HIGH_RISK_DELETE_WITHOUT_WHERE;
		}
	}
	return HIGH_RISK_NONE;
}

/*
 * check_high_risk_query
 *
 * Check for high-risk statements and trigger errors
 * or alerts based on firewall policies.
 */
static bool
check_high_risk_query(HighRiskStatementKind kind, const char *query)
{
	if (kind == HIGH_RISK_NONE || !is_high_risk_hash(kind) ||
		pgfw_mode == PGFW_MODE_DISABLED || pgfw_mode == PGFW_MODE_LEARNING)
		return false;

	if (pgfw_mode == PGFW_MODE_ENFORCING)
	{
		high_risk_stat_increment(kind, true);
		stat_error_increment();
		ereport(ERROR,
				(errcode(ERRCODE_S_R_E_PROHIBITED_SQL_STATEMENT_ATTEMPTED),
				 errmsg("Prohibited high-risk SQL statement"),
				 errdetail("SQL statement: %s", query)));
	}
	ereport(WARNING,
			(errmsg("Prohibited high-risk SQL statement, it is being executed in permissive mode."),
			 errdetail("SQL statement: %s", query)));

	high_risk_stat_increment(kind, false);
	stat_warning_increment();
	return true;
}

/*
 * was_high_risk_checked
 *
 * ExecutorStart has PlannedStmt rather than Query.  Reparse only the current
 * top-level UPDATE/DELETE source text to determine whether the parse hook has
 * already handled it as a configured high-risk statement.  This avoids
 * carrying query state from one execution to the next.
 */
static bool
was_high_risk_checked(PlannedStmt *plannedstmt, const char *query)
{
	HighRiskStatementKind kind;
	List	   *raw_list;
	RawStmt    *rawstmt;
	Node	   *stmt;

	if (plannedstmt == NULL || query == NULL ||
		(pgfw_mode != PGFW_MODE_PERMISSIVE &&
		 pgfw_mode != PGFW_MODE_ENFORCING))
		return false;

	if (plannedstmt->commandType == CMD_UPDATE)
		kind = HIGH_RISK_UPDATE_WITHOUT_WHERE;
	else if (plannedstmt->commandType == CMD_DELETE)
		kind = HIGH_RISK_DELETE_WITHOUT_WHERE;
	else
		return false;

	if (!is_high_risk_hash(kind))
		return false;

	raw_list = pg_parse_query(query);
	if (list_length(raw_list) != 1)
	{
		return false;
	}

	rawstmt = linitial_node(RawStmt, raw_list);
	stmt = rawstmt->stmt;

	if (kind == HIGH_RISK_UPDATE_WITHOUT_WHERE && IsA(stmt, UpdateStmt))
	{
		return ((UpdateStmt *) stmt)->whereClause == NULL;
	}

	if (kind == HIGH_RISK_DELETE_WITHOUT_WHERE && IsA(stmt, DeleteStmt))
	{
		return ((DeleteStmt *) stmt)->whereClause == NULL;
	}

	return false;
}

/*
 * is_high_risk_utility
 *
 * Identify high-risk statements.
 */
static HighRiskStatementKind
is_high_risk_utility(Node *parsetree)
{
	DropStmt  *drop;

	if (parsetree == NULL)
		return HIGH_RISK_NONE;

	if (IsA(parsetree, TruncateStmt))
		return HIGH_RISK_TRUNCATE_TABLE;
	if (IsA(parsetree, DropdbStmt))
		return HIGH_RISK_DROP_DATABASE;
	if (IsA(parsetree, DropStmt))
	{
		drop = (DropStmt *) parsetree;
		if (drop->removeType == OBJECT_TABLE)
			return HIGH_RISK_DROP_TABLE;
		if (drop->removeType == OBJECT_DATABASE)
			return HIGH_RISK_DROP_DATABASE;
		if (drop->removeType == OBJECT_SCHEMA)
			return HIGH_RISK_DROP_SCHEMA;
		return HIGH_RISK_NONE;
	}

	/* ALTER commands whose raw parse nodes do not all share one node type. */
	if (IsA(parsetree, AlterTableStmt) ||
		IsA(parsetree, AlterCollationStmt) ||
		IsA(parsetree, AlterDomainStmt) ||
		IsA(parsetree, AlterDefaultPrivilegesStmt) ||
		IsA(parsetree, AlterTableSpaceOptionsStmt) ||
		IsA(parsetree, AlterTableMoveAllStmt) ||
		IsA(parsetree, AlterExtensionStmt) ||
		IsA(parsetree, AlterExtensionContentsStmt) ||
		IsA(parsetree, AlterFdwStmt) ||
		IsA(parsetree, AlterForeignServerStmt) ||
		IsA(parsetree, AlterUserMappingStmt) ||
		IsA(parsetree, AlterPolicyStmt) ||
		IsA(parsetree, AlterEventTrigStmt) ||
		IsA(parsetree, AlterRoleStmt) ||
		IsA(parsetree, AlterRoleSetStmt) ||
		IsA(parsetree, AlterSeqStmt) ||
		IsA(parsetree, AlterOpFamilyStmt) ||
		IsA(parsetree, AlterStatsStmt) ||
		IsA(parsetree, AlterFunctionStmt) ||
		IsA(parsetree, RenameStmt) ||
		IsA(parsetree, AlterObjectDependsStmt) ||
		IsA(parsetree, AlterObjectSchemaStmt) ||
		IsA(parsetree, AlterOwnerStmt) ||
		IsA(parsetree, AlterOperatorStmt) ||
		IsA(parsetree, AlterTypeStmt) ||
		IsA(parsetree, AlterEnumStmt) ||
		IsA(parsetree, AlterDatabaseStmt) ||
		IsA(parsetree, AlterDatabaseSetStmt) ||
		IsA(parsetree, AlterSystemStmt) ||
		IsA(parsetree, AlterTSDictionaryStmt) ||
		IsA(parsetree, AlterTSConfigurationStmt) ||
		IsA(parsetree, AlterPublicationStmt) ||
		IsA(parsetree, AlterSubscriptionStmt))
		return HIGH_RISK_ALTER;

	return HIGH_RISK_NONE;
}

/*
 * set_high_risk_hash
 *
 * Add high-risk statements to the hash table.
 */
static void
set_high_risk_hash(HighRiskStatementKind kind, int64 warning_count,
				   int64 error_count)
{
	HighRiskHashKey key;
	HighRiskHashEntry *entry;
	bool		found;

	/* Prerequisites must be met. */
	if (pgss == NULL || high_risk_hash == NULL ||
		kind == HIGH_RISK_NONE)
	{
		return;
	}

	/* prepare the key of hash high_risk_hash. */
	memset(&key, 0, sizeof(key));
	key.dbid = MyDatabaseId;
	key.kind = kind;
	LWLockAcquire(pgss->lock, LW_EXCLUSIVE);

	/* High-risk statement detection. */
	entry = hash_search(high_risk_hash, &key, HASH_ENTER_NULL, &found);
	if (entry == NULL)
	{
		LWLockRelease(pgss->lock);
		ereport(ERROR, (errmsg("sql_firewall high-risk hash table is full")));
	}

	/*
	 * If it is not found, it is a new record,
	 * and a value is assigned to the new record.
	 */
	if (!found)
	{
		SpinLockInit(&entry->mutex);
		entry->warning_count = warning_count;
		entry->error_count = error_count;
		entry->change_count = 0;
		entry->persisted_count = 0;
	}

	LWLockRelease(pgss->lock);
}

/*
 * remove_high_risk_hash
 *
 * Remove high-risk statements from the hash table.
 */
static void
remove_high_risk_hash(HighRiskStatementKind kind)
{
	HighRiskHashKey key;

	/* Prerequisites must be met. */
	if (pgss == NULL || high_risk_hash == NULL ||
		kind == HIGH_RISK_NONE)
		return;

	memset(&key, 0, sizeof(key));
	key.dbid = MyDatabaseId;
	key.kind = kind;

	LWLockAcquire(pgss->lock, LW_EXCLUSIVE);
	hash_search(high_risk_hash, &key, HASH_REMOVE, NULL);
	LWLockRelease(pgss->lock);
}

/*
 * is_high_risk_hash
 *
 * Check the hash for the presence of specified high-risk types.
 */
static bool
is_high_risk_hash(HighRiskStatementKind kind)
{
	HighRiskHashKey key;
	bool		found;

	/* Prerequisites must be met. */
	if (pgss == NULL || high_risk_hash == NULL ||
		kind == HIGH_RISK_NONE)
		return false;

	/* Prepare the key value. */
	memset(&key, 0, sizeof(key));
	key.dbid = MyDatabaseId;
	key.kind = kind;

	/*
	 * Check the hash to determine if the current type
	 * is classified as a high-risk statement.
	 */
	LWLockAcquire(pgss->lock, LW_SHARED);
	found = hash_search(high_risk_hash, &key, HASH_FIND, NULL) != NULL;
	LWLockRelease(pgss->lock);

	return found;
}

/*
 * high_risk_stat_increment
 *
 * Increment the warning or error count for one configured high-risk type.
 */
static void
high_risk_stat_increment(HighRiskStatementKind kind, bool error)
{
	HighRiskHashKey key;
	HighRiskHashEntry *entry;

	/* Prerequisites must be met. */
	if (pgss == NULL || high_risk_hash == NULL ||
		kind == HIGH_RISK_NONE)
		return;

	/* Find records from hash high_risk_hash. */
	memset(&key, 0, sizeof(key));
	key.dbid = MyDatabaseId;
	key.kind = kind;
	LWLockAcquire(pgss->lock, LW_SHARED);
	entry = hash_search(high_risk_hash, &key, HASH_FIND, NULL);

	/* update the data of hash high_risk_hash. */
	if (entry != NULL)
	{
		SpinLockAcquire(&entry->mutex);
		if (error)
			entry->error_count++;
		else
			entry->warning_count++;
		entry->change_count++;
		SpinLockRelease(&entry->mutex);
	}
	LWLockRelease(pgss->lock);
}

/*
 * persist_changed_high_risk_counts
 *
 * Persist dirty high-risk counters without writing on the rejection path.
 */
static void
persist_changed_high_risk_counts(void)
{
	HASH_SEQ_STATUS seq;
	HighRiskHashEntry *entry;
	List	   *keys = NIL;
	ListCell   *lc;

	/*
	 * Updating the relation sql_high_risk_rec does
	 * not support execution within a transaction
	 * or parallel mode, and the existence of the
	 * relation sql_high_risk_rec must be verified.
	 */
	if (!IsTransactionState() || sql_firewall_xact_read_only() || IsParallelWorker() ||
		IsInParallelMode() || RecoveryInProgress() || high_risk_hash == NULL)
		return;

	/* Sequentially scan the hash to retrieve
	 * entries where the data has changed.
	 */
	LWLockAcquire(pgss->lock, LW_SHARED);
	hash_seq_init(&seq, high_risk_hash);
	while ((entry = hash_seq_search(&seq)) != NULL)
	{
		bool dirty;

		/* Search only for valid records in the current library. */
		if (entry->key.dbid != MyDatabaseId ||
			entry->key.kind == HIGH_RISK_NONE)
			continue;

		/* Locate the records that have changed
		 * and temporarily record the key value.
		 */
		SpinLockAcquire(&entry->mutex);
		dirty = entry->change_count != entry->persisted_count;
		SpinLockRelease(&entry->mutex);
		if (dirty)
		{
			HighRiskHashKey *key = palloc(sizeof(HighRiskHashKey));

			*key = entry->key;
			keys = lappend(keys, key);
		}
	}
	LWLockRelease(pgss->lock);

	/*
	 * Sequentially store the previously recorded
	 * entries that meet the requirements into the
	 * relation sql_high_risk_rec.
	 */
	foreach(lc, keys)
	{
		HighRiskHashKey *key = lfirst(lc);
		Oid			relid;
		Oid			indexid;
		Relation	rel;
		SysScanDesc scan;
		ScanKeyData scankey;
		HeapTuple	tuple;
		HeapTuple	newtuple;
		Datum		values[Natts_sql_high_risk_rec];
		bool		nulls[Natts_sql_high_risk_rec];
		bool		replaces[Natts_sql_high_risk_rec];
		int64		warnings;
		int64		errors;
		uint64		change_count;

		/* Find the complete record based on the key value. */
		LWLockAcquire(pgss->lock, LW_SHARED);
		entry = hash_search(high_risk_hash, key, HASH_FIND, NULL);
		if (entry == NULL)
		{
			LWLockRelease(pgss->lock);
			continue;
		}

		/* get the warnings and errors. */
		SpinLockAcquire(&entry->mutex);
		warnings = entry->warning_count;
		errors = entry->error_count;
		change_count = entry->change_count;
		SpinLockRelease(&entry->mutex);
		LWLockRelease(pgss->lock);

		/*
		 * open the relation sql_high_risk_rec,
		 * and search the record.
		 */
		get_high_risk_relation_oids(&relid, &indexid);
		rel = table_open(relid, RowExclusiveLock);
		ScanKeyInit(&scankey, Anum_sql_high_risk_rec_statement,
					BTEqualStrategyNumber, F_TEXTEQ,
					CStringGetTextDatum(high_risk_kind_name(key->kind)));
		scan = systable_beginscan(rel, indexid, true, NULL, 1, &scankey);
		tuple = systable_getnext(scan);

		/*
		 * If the record is found in the relation
		 * sql_high_risk_rec, update that data.
		 */
		if (HeapTupleIsValid(tuple))
		{
			MemSet(values, 0, sizeof(values));
			MemSet(nulls, false, sizeof(nulls));
			MemSet(replaces, false, sizeof(replaces));
			values[Anum_sql_high_risk_rec_warning_count - 1] = Int64GetDatum(warnings);
			values[Anum_sql_high_risk_rec_error_count - 1] = Int64GetDatum(errors);
			replaces[Anum_sql_high_risk_rec_warning_count - 1] = true;
			replaces[Anum_sql_high_risk_rec_error_count - 1] = true;
			newtuple = heap_modify_tuple(tuple, RelationGetDescr(rel),
									 values, nulls, replaces);
			CatalogTupleUpdate(rel, &tuple->t_self, newtuple);
			heap_freetuple(newtuple);
		}
		systable_endscan(scan);
		table_close(rel, RowExclusiveLock);

		/*
		 * update the flag of hash, avoid repeat update
		 * relation sql_high_risk_rec.
		 */
		LWLockAcquire(pgss->lock, LW_SHARED);
		entry = hash_search(high_risk_hash, key, HASH_FIND, NULL);
		if (entry != NULL)
		{
			SpinLockAcquire(&entry->mutex);
			if (entry->persisted_count < change_count)
				entry->persisted_count = change_count;
			SpinLockRelease(&entry->mutex);
		}
		LWLockRelease(pgss->lock);
	}
	list_free_deep(keys);
}

/*
 * high_risk_kind_from_name
 *
 * Determine the type of high-risk statement
 * based on the provided name.
 */
static HighRiskStatementKind
high_risk_kind_from_name(const char *name)
{
	const char *start;
	const char *end;
	char	   *trimmed_name;
	HighRiskStatementKind kind = HIGH_RISK_NONE;

	if (name == NULL)
		return HIGH_RISK_NONE;

	/* Remove leading and trailing whitespace from a string. */
	start = name;
	while (*start != '\0' && isspace((unsigned char) *start))
		start++;
	end = start + strlen(start);
	while (end > start && isspace((unsigned char) end[-1]))
		end--;
	if (end == start)
		return HIGH_RISK_NONE;

	trimmed_name = pnstrdup(start, end - start);
	if (pg_strcasecmp(trimmed_name, "unsafealter") == 0)
		kind = HIGH_RISK_ALTER;
	else if (pg_strcasecmp(trimmed_name, "unsafeupdate") == 0)
		kind = HIGH_RISK_UPDATE_WITHOUT_WHERE;
	else if (pg_strcasecmp(trimmed_name, "unsafedelete") == 0)
		kind = HIGH_RISK_DELETE_WITHOUT_WHERE;
	else if (pg_strcasecmp(trimmed_name, "unsafedroptable") == 0)
		kind = HIGH_RISK_DROP_TABLE;
	else if (pg_strcasecmp(trimmed_name, "unsafetruncate") == 0)
		kind = HIGH_RISK_TRUNCATE_TABLE;
	else if (pg_strcasecmp(trimmed_name, "unsafedropdatabase") == 0)
		kind = HIGH_RISK_DROP_DATABASE;
	else if (pg_strcasecmp(trimmed_name, "unsafedropschema") == 0)
		kind = HIGH_RISK_DROP_SCHEMA;
	else
	{
		pfree(trimmed_name);
		ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("Invalid high-risk type"),
					 errdetail("Invalid high-risk typet: %s", name)));
	}
	pfree(trimmed_name);

	return kind;
}

/*
 * high_risk_kind_name
 *
 * Return the name of the high-risk type
 * based on the high-risk type.
 */
static const char *
high_risk_kind_name(HighRiskStatementKind kind)
{
	switch (kind)
	{
		case HIGH_RISK_ALTER:
			return "unsafealter";
		case HIGH_RISK_UPDATE_WITHOUT_WHERE:
			return "unsafeupdate";
		case HIGH_RISK_DELETE_WITHOUT_WHERE:
			return "unsafedelete";
		case HIGH_RISK_DROP_TABLE:
			return "unsafedroptable";
		case HIGH_RISK_TRUNCATE_TABLE:
			return "unsafetruncate";
		case HIGH_RISK_DROP_DATABASE:
			return "unsafedropdatabase";
		case HIGH_RISK_DROP_SCHEMA:
			return "unsafedropschema";
		default:
			return NULL;
	}
}

/*
 * high_risk_kind_description
 *
 * Description of high-risk types.
 */
static const char *
high_risk_kind_description(HighRiskStatementKind kind)
{
	switch (kind)
	{
		case HIGH_RISK_ALTER:
			return "all ALTER statements";
		case HIGH_RISK_UPDATE_WITHOUT_WHERE:
			return "UPDATE without a WHERE clause";
		case HIGH_RISK_DELETE_WITHOUT_WHERE:
			return "DELETE without a WHERE clause";
		case HIGH_RISK_DROP_TABLE:
			return "DROP TABLE statements";
		case HIGH_RISK_TRUNCATE_TABLE:
			return "TRUNCATE TABLE statements";
		case HIGH_RISK_DROP_DATABASE:
			return "DROP DATABASE statements";
		case HIGH_RISK_DROP_SCHEMA:
			return "DROP SCHEMA statements";
		default:
			return NULL;
	}
}

/*
 * get_rule_relation_oids
 *
 * get the relation oid of sql_firewall_rec and it index.
 */
static bool
get_rule_relation_oids(Oid *relid, Oid *indexid)
{
	Oid			nspoid;

	nspoid = SECURITY_SCHEMA_NAMESPACE;
	*relid = get_relname_relid(RULE_RELATION_NAME, nspoid);
	if (!OidIsValid(*relid))
		return false;

	*indexid = get_relname_relid(RULE_INDEX_NAME, nspoid);

	return OidIsValid(*indexid);
}

/*
 * get_stat_count_relation_oid
 *
 * get the relation oid of sql_firewall_stat_rec.
 */
static Oid
get_stat_count_relation_oid(void)
{
	return get_relname_relid(STAT_REC_RELATION_NAME, SECURITY_SCHEMA_NAMESPACE);
}

/*
 * ensure_stat_count_loaded
 *
 * ensure the relation sql_firewall_stat_rec was loaded.
 */
static void
ensure_stat_count_loaded(void)
{
	Oid			relid;
	Relation	rel;
	TableScanDesc scan;
	HeapTuple	tuple;
	int64		warnings = 0;
	int64		errors = 0;
	SqlFirewallStatCountHashKey key;
	SqlFirewallStatCountHashEntry *entry;
	bool		found;

	if (current_stat_count != NULL || stat_count_hash == NULL ||
		!IsTransactionState())
		return;

	/*
	 * Another backend may already have loaded the counters for this
	 * database.  Check the shared hash before reading the catalog table.
	 */
	key.dbid = MyDatabaseId;
	LWLockAcquire(pgss->lock, LW_SHARED);
	entry = hash_search(stat_count_hash, &key, HASH_FIND, NULL);
	if (entry != NULL)
		current_stat_count = entry;
	LWLockRelease(pgss->lock);

	if (entry != NULL)
		return;

	relid = get_stat_count_relation_oid();
	if (!OidIsValid(relid))
		return;

	rel = table_open(relid, AccessShareLock);
	scan = table_beginscan_catalog(rel, 0, NULL);
	tuple = heap_getnext(scan, ForwardScanDirection);
	if (HeapTupleIsValid(tuple))
	{
		bool		isnull;
		Datum		value;

		value = heap_getattr(tuple, Anum_sql_firewall_stat_rec_warning,
						 RelationGetDescr(rel), &isnull);
		if (!isnull)
			warnings = DatumGetInt64(value);
		value = heap_getattr(tuple, Anum_sql_firewall_stat_rec_error,
						 RelationGetDescr(rel), &isnull);
		if (!isnull)
			errors = DatumGetInt64(value);
	}
	table_endscan(scan);
	table_close(rel, AccessShareLock);

	LWLockAcquire(pgss->lock, LW_EXCLUSIVE);

	/* Prevent execution by other processes. */
	entry = hash_search(stat_count_hash, &key, HASH_ENTER, &found);
	if (!found)
	{
		SpinLockInit(&entry->mutex);
		entry->warning_count = warnings;
		entry->error_count = errors;
		entry->change_count = 0;
		entry->persisted_count = 0;
	}

	current_stat_count = entry;
	LWLockRelease(pgss->lock);
}

/*
 * persist_stat_count
 *
 * Store the number of errors and warnings
 * in the table sql_firewall_stat_rec.
 */
static void
persist_stat_count(void)
{
	Oid			relid;
	Relation	rel;
	TableScanDesc scan;
	HeapTuple	oldtuple;
	HeapTuple	newtuple;
	Datum		values[Natts_sql_firewall_stat_rec];
	bool		nulls[Natts_sql_firewall_stat_rec];
	bool		replaces[Natts_sql_firewall_stat_rec];
	int64		warnings;
	int64		errors;
	uint64		change_count;

	if (current_stat_count == NULL || !IsTransactionState() ||
		sql_firewall_xact_read_only() ||
		IsParallelWorker() || IsInParallelMode() || RecoveryInProgress())
		return;

	SpinLockAcquire(&current_stat_count->mutex);
	change_count = current_stat_count->change_count;
	if (change_count == current_stat_count->persisted_count)
	{
		SpinLockRelease(&current_stat_count->mutex);
		return;
	}
	warnings = current_stat_count->warning_count;
	errors = current_stat_count->error_count;
	SpinLockRelease(&current_stat_count->mutex);

	relid = get_stat_count_relation_oid();
	if (!OidIsValid(relid))
		return;
	rel = table_open(relid, RowExclusiveLock);
	scan = table_beginscan_catalog(rel, 0, NULL);
	oldtuple = heap_getnext(scan, ForwardScanDirection);
	MemSet(values, 0, sizeof(values));
	MemSet(nulls, false, sizeof(nulls));
	MemSet(replaces, false, sizeof(replaces));
	values[Anum_sql_firewall_stat_rec_warning - 1] = Int64GetDatum(warnings);
	values[Anum_sql_firewall_stat_rec_error - 1] = Int64GetDatum(errors);
	if (HeapTupleIsValid(oldtuple))
	{
		replaces[Anum_sql_firewall_stat_rec_warning - 1] = true;
		replaces[Anum_sql_firewall_stat_rec_error - 1] = true;
		newtuple = heap_modify_tuple(oldtuple, RelationGetDescr(rel),
								 values, nulls, replaces);
		CatalogTupleUpdate(rel, &oldtuple->t_self, newtuple);
	}
	else
	{
		newtuple = heap_form_tuple(RelationGetDescr(rel), values, nulls);
		CatalogTupleInsert(rel, newtuple);
	}
	heap_freetuple(newtuple);
	table_endscan(scan);
	table_close(rel, RowExclusiveLock);
	CommandCounterIncrement();

	SpinLockAcquire(&current_stat_count->mutex);
	if (current_stat_count->persisted_count < change_count)
		current_stat_count->persisted_count = change_count;
	SpinLockRelease(&current_stat_count->mutex);
}

/*
 * load_persisted_rules
 *
 * store the black list and white list in relation sql_firewall_rec.
 */
static List *
load_persisted_rules(void)
{
	Oid			relid;
	Oid			indexid;
	Relation	rel;
	TableScanDesc scan;
	HeapTuple	tuple;
	TupleDesc	tupdesc;
	List	   *rules = NIL;

	if (!get_rule_relation_oids(&relid, &indexid))
		return NIL;

	rel = table_open(relid, AccessShareLock);
	tupdesc = RelationGetDescr(rel);
	scan = table_beginscan_catalog(rel, 0, NULL);
	while (HeapTupleIsValid(tuple = heap_getnext(scan, ForwardScanDirection)))
	{
		Datum		values[Natts_sql_firewall_rec];
		bool		nulls[Natts_sql_firewall_rec];
		SqlFirewallPersistedRule *rule;
		char	   *encoding_name;
		char	   *type_name;

		heap_deform_tuple(tuple, tupdesc, values, nulls);
		if (nulls[Anum_sql_firewall_rec_userid - 1] ||
			nulls[Anum_sql_firewall_rec_queryid - 1] ||
			nulls[Anum_sql_firewall_rec_query - 1] ||
			nulls[Anum_sql_firewall_rec_original_query - 1] ||
			nulls[Anum_sql_firewall_rec_calls - 1] ||
			nulls[Anum_sql_firewall_rec_encoding - 1] ||
			nulls[Anum_sql_firewall_rec_statement_type - 1] ||
			nulls[Anum_sql_firewall_rec_warning_count - 1] ||
			nulls[Anum_sql_firewall_rec_error_count - 1])
			continue;

		rule = palloc0(sizeof(SqlFirewallPersistedRule));
		rule->userid = DatumGetObjectId(values[Anum_sql_firewall_rec_userid - 1]);
		rule->queryid = (uint64) DatumGetInt64(values[Anum_sql_firewall_rec_queryid - 1]);
		rule->query = TextDatumGetCString(values[Anum_sql_firewall_rec_query - 1]);
		rule->original_query = TextDatumGetCString(values[Anum_sql_firewall_rec_original_query - 1]);
		rule->calls = DatumGetInt64(values[Anum_sql_firewall_rec_calls - 1]);
		rule->warning_count =
			DatumGetInt64(values[Anum_sql_firewall_rec_warning_count - 1]);
		rule->error_count =
			DatumGetInt64(values[Anum_sql_firewall_rec_error_count - 1]);
		encoding_name = TextDatumGetCString(values[Anum_sql_firewall_rec_encoding - 1]);
		type_name = TextDatumGetCString(values[Anum_sql_firewall_rec_statement_type - 1]);
		rule->encoding = pg_char_to_encoding(encoding_name);
		if (pg_strcasecmp(type_name, "white") == 0)
			rule->statement_type = SQL_FIREWALL_STATEMENT_WHITE;
		else if (pg_strcasecmp(type_name, "black") == 0)
			rule->statement_type = SQL_FIREWALL_STATEMENT_BLACK;
		else
		{
			pfree(rule->query);
			pfree(rule->original_query);
			pfree(rule);
			pfree(encoding_name);
			pfree(type_name);
			continue;
		}
		pfree(encoding_name);
		pfree(type_name);
		if (rule->encoding < 0)
		{
			pfree(rule->query);
			pfree(rule->original_query);
			pfree(rule);
			continue;
		}
		rules = lappend(rules, rule);
	}
	table_endscan(scan);
	table_close(rel, AccessShareLock);
	list_sort(rules, persisted_rule_cmp);
	return rules;
}

/*
 * persisted_rule_cmp
 *
 * sort the entries.
 */
static int
persisted_rule_cmp(const ListCell *a, const ListCell *b)
{
	const SqlFirewallPersistedRule *ra = lfirst(a);
	const SqlFirewallPersistedRule *rb = lfirst(b);

	if (ra->userid != rb->userid)
		return ra->userid < rb->userid ? -1 : 1;
	if (ra->queryid != rb->queryid)
		return ra->queryid < rb->queryid ? -1 : 1;
	if (ra->statement_type != rb->statement_type)
		return ra->statement_type < rb->statement_type ? -1 : 1;
	return 0;
}

/*
 * append_runtime_only_rules
 *
 * Include entries learned before the extension table became available.
 */
static List *
append_runtime_only_rules(List *rules)
{
	HASH_SEQ_STATUS hash_seq;
	sqlfirewallPgssEntry  *entry;

	LWLockAcquire(pgss->lock, LW_SHARED);
	hash_seq_init(&hash_seq, pgss_hash);
	while ((entry = hash_seq_search(&hash_seq)) != NULL)
	{
		ListCell   *lc;
		bool		found = false;

		/* Search only for records in the current database. */
		if (entry->key.dbid != MyDatabaseId)
			continue;

		/*
		 * When loading data from the table into
		 * the hash map, an entry labeled SQL_FIREWALL_STATEMENT_ALL
		 * is inserted; this entry merely indicates that the data
		 * has been loaded and is not used as actual data.
		 */
		if (entry->statement_type != SQL_FIREWALL_STATEMENT_WHITE &&
			entry->statement_type != SQL_FIREWALL_STATEMENT_BLACK)
			continue;

		/*
		 * Check whether the record from the hash exists
		 * in the table; if it does, proceed to search for
		 * the next one.
		 */
		foreach(lc, rules)
		{
			SqlFirewallPersistedRule *rule = lfirst(lc);

			if (rule->userid == entry->key.userid &&
				rule->queryid == entry->key.queryid &&
				rule->statement_type == entry->key.statement_type)
			{
				found = true;
				break;
			}
		}

		/*
		 * Add items present in the hash but
		 * absent from the table to the list rules.
		 */
		if (!found)
		{
			SqlFirewallPersistedRule *rule = palloc0(sizeof(SqlFirewallPersistedRule));

			/*
			 * Add records that are not in the table but are present
			 * in the hash to the linked list rules.
			 */
			rule->userid = entry->key.userid;
			rule->queryid = entry->key.queryid;
			rule->encoding = GetDatabaseEncoding();
			rule->statement_type = entry->statement_type;
			SpinLockAcquire(&entry->mutex);
			rule->calls = entry->counters.calls;
			rule->warning_count = entry->counters.warning_count;
			rule->error_count = entry->counters.error_count;
			SpinLockRelease(&entry->mutex);
			rules = lappend(rules, rule);
		}
	}
	LWLockRelease(pgss->lock);
	list_sort(rules, persisted_rule_cmp);
	return rules;
}

/*
 * free_persisted_rules
 *
 * free persisted rules.
 */
static void
free_persisted_rules(List *rules)
{
	ListCell   *lc;

	foreach(lc, rules)
	{
		SqlFirewallPersistedRule *rule = lfirst(lc);

		if (rule->query)
			pfree(rule->query);
		if (rule->original_query)
			pfree(rule->original_query);
		pfree(rule);
	}
	list_free(rules);
}

/*
 * persist_rule
 *
 * store the black list and white list
 * in relation sql_firewall_rec.
 */
static void
persist_rule(Oid userid, uint64 queryid, const char *query,
			 const char *original_query, int64 calls, int64 warning_count,
			 int64 error_count, int encoding, int statement_type)
{
	Oid			relid;
	Oid			indexid;
	Relation	rel;
	SysScanDesc scan;
	ScanKeyData keys[3];
	HeapTuple	oldtuple;
	HeapTuple	newtuple;
	Datum		values[Natts_sql_firewall_rec];
	bool		nulls[Natts_sql_firewall_rec] ;
	bool		replaces[Natts_sql_firewall_rec] ;

	if (!IsTransactionState() || sql_firewall_xact_read_only() || IsParallelWorker() ||
		IsInParallelMode() || RecoveryInProgress() ||
		!get_rule_relation_oids(&relid, &indexid))
		return;

	MemSet(values, 0, sizeof(values));
	MemSet(nulls, false, sizeof(nulls));
	MemSet(replaces, false, sizeof(replaces));

	rel = table_open(relid, RowExclusiveLock);
	ScanKeyInit(&keys[0], Anum_sql_firewall_rec_userid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(userid));
	ScanKeyInit(&keys[1], Anum_sql_firewall_rec_queryid,
				BTEqualStrategyNumber, F_INT8EQ,
				Int64GetDatum((int64) queryid));
	ScanKeyInit(&keys[2], Anum_sql_firewall_rec_statement_type,
				BTEqualStrategyNumber, F_TEXTEQ,
				CStringGetTextDatum(statement_type_name(statement_type)));
	scan = systable_beginscan(rel, indexid, true, NULL, 3, keys);
	oldtuple = systable_getnext(scan);

	values[Anum_sql_firewall_rec_userid - 1] = ObjectIdGetDatum(userid);
	values[Anum_sql_firewall_rec_queryid - 1] = Int64GetDatum((int64) queryid);
	values[Anum_sql_firewall_rec_query - 1] = CStringGetTextDatum(query);
	values[Anum_sql_firewall_rec_original_query - 1] =
		CStringGetTextDatum(original_query);
	values[Anum_sql_firewall_rec_calls - 1] = Int64GetDatum(calls);
	values[Anum_sql_firewall_rec_encoding - 1] =
		CStringGetTextDatum(pg_encoding_to_char(encoding));
	values[Anum_sql_firewall_rec_statement_type - 1] =
		CStringGetTextDatum(statement_type_name(statement_type));
	values[Anum_sql_firewall_rec_warning_count - 1] =
		Int64GetDatum(warning_count);
	values[Anum_sql_firewall_rec_error_count - 1] = Int64GetDatum(error_count);

	if (HeapTupleIsValid(oldtuple))
	{
		replaces[Anum_sql_firewall_rec_query - 1] = true;
		replaces[Anum_sql_firewall_rec_original_query - 1] = true;
		replaces[Anum_sql_firewall_rec_calls - 1] = true;
		replaces[Anum_sql_firewall_rec_encoding - 1] = true;
		replaces[Anum_sql_firewall_rec_statement_type - 1] = true;
		replaces[Anum_sql_firewall_rec_warning_count - 1] = true;
		replaces[Anum_sql_firewall_rec_error_count - 1] = true;

		newtuple = heap_modify_tuple(oldtuple, RelationGetDescr(rel),
								 values, nulls, replaces);
		CatalogTupleUpdate(rel, &oldtuple->t_self, newtuple);
	}
	else
	{
		newtuple = heap_form_tuple(RelationGetDescr(rel), values, nulls);
		CatalogTupleInsert(rel, newtuple);
	}

	heap_freetuple(newtuple);
	systable_endscan(scan);
	table_close(rel, RowExclusiveLock);
	CommandCounterIncrement();
}

/*
 * delete_persisted_rule
 *
 * delete the black list or white list from the
 * relation sql_firewall_rec.
 */
static void
delete_persisted_rule(const sqlfirewallPgssHashKey *key)
{
	Oid			relid;
	Oid			indexid;
	Relation	rel;
	SysScanDesc scan;
	ScanKeyData keys[3];
	HeapTuple	tuple;

	/*
	 * Deleting sql_firewall records is not
	 * supported in transaction or parallel modes.
	 */
	if (!IsTransactionState() || sql_firewall_xact_read_only() || IsParallelWorker() ||
		IsInParallelMode() || RecoveryInProgress() ||
		!get_rule_relation_oids(&relid, &indexid))
		return;

	/*
	 * Deleting sql_firewall records from relateion
	 * sql_firewall_rec.
	 */
	rel = table_open(relid, RowExclusiveLock);
	ScanKeyInit(&keys[0], Anum_sql_firewall_rec_userid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(key->userid));
	ScanKeyInit(&keys[1], Anum_sql_firewall_rec_queryid,
				BTEqualStrategyNumber, F_INT8EQ,
				Int64GetDatum((int64) key->queryid));
	ScanKeyInit(&keys[2], Anum_sql_firewall_rec_statement_type,
				BTEqualStrategyNumber, F_TEXTEQ,
				CStringGetTextDatum(statement_type_name(key->statement_type)));
	scan = systable_beginscan(rel, indexid, true, NULL, 3, keys);
	tuple = systable_getnext(scan);
	/* Delete the record if found; otherwise, terminate. */
	if (HeapTupleIsValid(tuple))
	{
		CatalogTupleDelete(rel, &tuple->t_self);
	}
	systable_endscan(scan);
	table_close(rel, RowExclusiveLock);
	CommandCounterIncrement();
}

/*
 * delete_all_persisted_rules
 *
 * delete all black list and white list
 * form the relation sql_firewall_rec.
 */
static void
delete_all_persisted_rules(void)
{
	Oid			relid;
	Oid			indexid;
	Relation	rel;
	TableScanDesc scan;
	HeapTuple	tuple;

	/*
	 * Deleting sql_firewall records is not
	 * supported in transaction or parallel modes.
	 */
	if (!IsTransactionState() || sql_firewall_xact_read_only() || IsParallelWorker() ||
		IsInParallelMode() || RecoveryInProgress() ||
		!get_rule_relation_oids(&relid, &indexid))
		return;

	rel = table_open(relid, RowExclusiveLock);
	scan = table_beginscan_catalog(rel, 0, NULL);

	/* Delete all records from the sql_firewall_rec
	 * table in a loop.
	 */
	while (HeapTupleIsValid(tuple = heap_getnext(scan, ForwardScanDirection)))
	{
		CatalogTupleDelete(rel, &tuple->t_self);
	}

	table_endscan(scan);
	table_close(rel, RowExclusiveLock);
	CommandCounterIncrement();
}

/*
 * ensure_rule_hash_loaded
 *
 * ensure all rule from relation sql_firewall_rec
 * reload to hash pgss_hash.
 */
static void
ensure_rule_hash_loaded(void)
{
	Oid			relid;
	Oid			indexid;
	Relation	rel;
	TableScanDesc scan;
	HeapTuple	tuple;
	TupleDesc	tupdesc;
	sqlfirewallPgssHashKey marker_key;
	sqlfirewallPgssEntry  *marker_entry;
	bool		found;

	/*
	 * It must be ensured that the hash table
	 * pgss_hash and the user table sql_firewall_rec
	 * exist.
	 */
	if (pgss_hash == NULL || rule_hash_loaded)
		return;

	/*
	 * A zero-valued rule key is a per-database
	 * load-complete marker.  It is stored only
	 * in the shared hash and is never exposed
	 * as a firewall rule.
	 */
	marker_key.dbid = MyDatabaseId;
	marker_key.userid = InvalidOid;
	marker_key.queryid = UINT64CONST(0);
	marker_key.statement_type = SQL_FIREWALL_STATEMENT_ALL;

	LWLockAcquire(pgss->lock, LW_SHARED);
	marker_entry = hash_search(pgss_hash, &marker_key, HASH_FIND, NULL);
	LWLockRelease(pgss->lock);
	if (marker_entry != NULL)
	{
		rule_hash_loaded = true;
		return;
	}

	if (!get_rule_relation_oids(&relid, &indexid))
		return;

	/*
	 * Open the relation sql_firewall_rec and
	 * scan the data sequentially.
	 */
	rel = table_open(relid, AccessShareLock);
	tupdesc = RelationGetDescr(rel);
	scan = table_beginscan_catalog(rel, 0, NULL);

	/* Sequentially scan data. */
	while (HeapTupleIsValid(tuple = heap_getnext(scan, ForwardScanDirection)))
	{
		Datum		values[Natts_sql_firewall_rec];
		bool		nulls[Natts_sql_firewall_rec];
		char	   *query;
		char	   *original_query;
		char	   *encoding_name;
		char	   *statement_type_name;
		int			encoding;
		int			statement_type;
		Oid			userid = InvalidOid;
		uint64		queryid;
		int64		calls = 0;
		int64		warning_count = 0;
		int64		error_count = 0;

		heap_deform_tuple(tuple, tupdesc, values, nulls);
		if (nulls[Anum_sql_firewall_rec_userid - 1] ||
			nulls[Anum_sql_firewall_rec_queryid - 1] ||
			nulls[Anum_sql_firewall_rec_query - 1] ||
			nulls[Anum_sql_firewall_rec_original_query - 1] ||
			nulls[Anum_sql_firewall_rec_calls - 1] ||
			nulls[Anum_sql_firewall_rec_encoding - 1] ||
			nulls[Anum_sql_firewall_rec_statement_type - 1] ||
			nulls[Anum_sql_firewall_rec_warning_count - 1] ||
			nulls[Anum_sql_firewall_rec_error_count - 1])
			continue;

		/* Parse data. */
		userid = DatumGetObjectId(values[Anum_sql_firewall_rec_userid - 1]);
		queryid = (uint64) DatumGetInt64(values[Anum_sql_firewall_rec_queryid - 1]);
		encoding_name = TextDatumGetCString(values[Anum_sql_firewall_rec_encoding - 1]);
		statement_type_name = TextDatumGetCString(values[Anum_sql_firewall_rec_statement_type - 1]);
		encoding = pg_char_to_encoding(encoding_name);
		calls = DatumGetInt64(values[Anum_sql_firewall_rec_calls - 1]);
		warning_count = DatumGetInt64(values[Anum_sql_firewall_rec_warning_count - 1]);
		error_count = DatumGetInt64(values[Anum_sql_firewall_rec_error_count - 1]);
		if (pg_strcasecmp(statement_type_name, "white") == 0)
			statement_type = SQL_FIREWALL_STATEMENT_WHITE;
		else if (pg_strcasecmp(statement_type_name, "black") == 0)
			statement_type = SQL_FIREWALL_STATEMENT_BLACK;
		else
			ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("statement_type has only white or black")));

		pfree(encoding_name);
		pfree(statement_type_name);

		if (encoding < 0)
			continue;

		query = TextDatumGetCString(values[Anum_sql_firewall_rec_query - 1]);
		original_query = TextDatumGetCString(values[Anum_sql_firewall_rec_original_query - 1]);

		/* Write data to the hash table pgss_hash. */
		pgss_restore(userid,
					 queryid,
					 query, original_query,
					 calls,
					 warning_count, error_count,
					 statement_type, false);
		pfree(query);
		pfree(original_query);
	}

	table_endscan(scan);
	table_close(rel, AccessShareLock);

	/*
	 * Recheck under the exclusive lock in case another
	 * backend completed the load while this backend was
	 * scanning the table.
	 */
	LWLockAcquire(pgss->lock, LW_EXCLUSIVE);
	marker_entry = hash_search(pgss_hash, &marker_key, HASH_FIND, NULL);
	if (marker_entry == NULL)
	{
		/*
		 * After loading data from the table into the hash
		 * structure, a flag entry must be inserted.
		 */
		marker_entry = hash_search(pgss_hash, &marker_key, HASH_ENTER, &found);
		if (!found)
		{
			SpinLockInit(&marker_entry->mutex);
			memset(&marker_entry->counters, 0, sizeof(SqlFirewallCounters));
			marker_entry->statement_type = SQL_FIREWALL_STATEMENT_ALL;
		}
	}
	LWLockRelease(pgss->lock);
}

/*
 * persist_hash_entry
 *
 * update reltaion sql_firewall_rec, the data
 * from the hash pgss_hash.
 */
static void
persist_hash_entry(const sqlfirewallPgssHashKey *key)
{
	Oid			relid;
	Oid			indexid;
	Relation	rel;
	SysScanDesc scan;
	ScanKeyData keys[3];
	HeapTuple	oldtuple;
	HeapTuple	newtuple;
	Datum		values[Natts_sql_firewall_rec];
	bool		nulls[Natts_sql_firewall_rec];
	bool		replaces[Natts_sql_firewall_rec];
	sqlfirewallPgssEntry  *entry;
	int64		calls = 0;
	int64		warning_count = 0;
	int64		error_count = 0;
	uint64		change_count = 0;

	/*
	 * Updating the relation sql_firewall_rec does
	 * not support execution within a transaction
	 * or parallel mode, and the existence of the
	 * relation sql_firewall_rec must be verified.
	 */
	if (!IsTransactionState() || sql_firewall_xact_read_only() || IsParallelWorker() ||
		IsInParallelMode() || RecoveryInProgress() ||
		!get_rule_relation_oids(&relid, &indexid))
		return;

	/* get the value from hash pgss_hash. */
	LWLockAcquire(pgss->lock, LW_SHARED);
	entry = hash_search(pgss_hash, key, HASH_FIND, NULL);
	if (entry != NULL)
	{
		SpinLockAcquire(&entry->mutex);
		calls = entry->counters.calls;
		warning_count = entry->counters.warning_count;
		error_count = entry->counters.error_count;
		change_count = entry->counters.change_count;
		SpinLockRelease(&entry->mutex);
	}
	LWLockRelease(pgss->lock);

	if (entry == NULL)
		return;

	/*
	 * Open the relation sql_firewall_rec
	 * and search for the record.
	 */
	rel = table_open(relid, RowExclusiveLock);
	ScanKeyInit(&keys[0], Anum_sql_firewall_rec_userid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(key->userid));
	ScanKeyInit(&keys[1], Anum_sql_firewall_rec_queryid,
				BTEqualStrategyNumber, F_INT8EQ,
				Int64GetDatum((int64) key->queryid));
	ScanKeyInit(&keys[2], Anum_sql_firewall_rec_statement_type,
				BTEqualStrategyNumber, F_TEXTEQ,
				CStringGetTextDatum(statement_type_name(key->statement_type)));
	scan = systable_beginscan(rel, indexid, true, NULL, 3, keys);
	oldtuple = systable_getnext(scan);

	/* Update the call count if the record is found in the table. */
	if (HeapTupleIsValid(oldtuple))
	{
		/* Initialize variables. */
		MemSet(values, 0, sizeof(values));
		MemSet(nulls, false, sizeof(nulls));
		MemSet(replaces, false, sizeof(replaces));

		/* Prepare data and update records. */
		values[Anum_sql_firewall_rec_calls - 1] = Int64GetDatum(calls);
		values[Anum_sql_firewall_rec_warning_count - 1] =
			Int64GetDatum(warning_count);
		values[Anum_sql_firewall_rec_error_count - 1] = Int64GetDatum(error_count);
		replaces[Anum_sql_firewall_rec_calls - 1] = true;
		replaces[Anum_sql_firewall_rec_warning_count - 1] = true;
		replaces[Anum_sql_firewall_rec_error_count - 1] = true;
		newtuple = heap_modify_tuple(oldtuple, RelationGetDescr(rel),
								 values, nulls, replaces);
		CatalogTupleUpdate(rel, &oldtuple->t_self, newtuple);
		heap_freetuple(newtuple);
	}

	/* Finish scanning and close the relation. */
	systable_endscan(scan);
	table_close(rel, RowExclusiveLock);
	CommandCounterIncrement();

	/* Reset the flag in the hash after updating the table. */
	LWLockAcquire(pgss->lock, LW_SHARED);
	entry = hash_search(pgss_hash, key, HASH_FIND, NULL);
	if (entry != NULL)
	{
		SpinLockAcquire(&entry->mutex);
		if (entry->counters.persisted_count < change_count)
			entry->counters.persisted_count = change_count;
		SpinLockRelease(&entry->mutex);
	}
	LWLockRelease(pgss->lock);
}

/*
 * persist_changed_rule_counts
 *
 * Persist dirty blacklist counters on the next safe statement boundary.
 */
static void
persist_changed_rule_counts(void)
{
	HASH_SEQ_STATUS seq;
	sqlfirewallPgssEntry *entry;
	List	   *keys = NIL;
	ListCell   *lc;

	/*
	 * Updating the relation sql_firewall_rec does
	 * not support execution within a transaction
	 * or parallel mode, and the existence of the
	 * relation sql_firewall_rec must be verified.
	 */
	if (!IsTransactionState() || sql_firewall_xact_read_only() || IsParallelWorker() ||
		IsInParallelMode() || RecoveryInProgress() || pgss_hash == NULL)
		return;

	/*
	 * Sequentially scan all entries in the blacklist
	 * and update the alarm or error counts in the table.
	 */
	LWLockAcquire(pgss->lock, LW_SHARED);
	hash_seq_init(&seq, pgss_hash);
	while ((entry = hash_seq_search(&seq)) != NULL)
	{
		bool dirty;

		if (entry->key.dbid != MyDatabaseId ||
			entry->statement_type != SQL_FIREWALL_STATEMENT_BLACK)
			continue;

		SpinLockAcquire(&entry->mutex);
		dirty = entry->counters.change_count != entry->counters.persisted_count;
		SpinLockRelease(&entry->mutex);

		/* Temporarily store the record first. */
		if (dirty)
		{
			sqlfirewallPgssHashKey *key = palloc(sizeof(sqlfirewallPgssHashKey));

			*key = entry->key;
			keys = lappend(keys, key);
		}
	}
	LWLockRelease(pgss->lock);

	/* Update the record in the relation sql_firewall_rec. */
	foreach(lc, keys)
	{
		persist_hash_entry((sqlfirewallPgssHashKey *) lfirst(lc));
	}

	/* Destroy the list keys. */
	list_free_deep(keys);
}

/*
 * get_high_risk_relation_oids
 *
 * get the oid of relationsql_high_risk_rec.
 */
static void
get_high_risk_relation_oids(Oid *relid, Oid *indexid)
{
	Oid			nspoid;

	nspoid = SECURITY_SCHEMA_NAMESPACE;

	*relid = get_relname_relid(HIGH_RISK_RELATION_NAME, nspoid);
	if (!OidIsValid(*relid))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_TABLE),
				 errmsg("relation with OID %u and name \"%s\" does not exist",
						SECURITY_SCHEMA_NAMESPACE, HIGH_RISK_RELATION_NAME)));

	*indexid = get_relname_relid(HIGH_RISK_INDEX_NAME, nspoid);
	if (!OidIsValid(*indexid))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("index \"%s\" does not exist",
						HIGH_RISK_INDEX_NAME)));
}

/*
 * ensure_high_risk_hash_loaded
 *
 *  ensure high risk record to hash high_risk_hash.
 */
static void
ensure_high_risk_hash_loaded(void)
{
	Oid			relid;
	Relation	rel;
	TableScanDesc scan;
	HeapTuple	tuple;
	AttrNumber	statement_attnum;
	HighRiskHashKey marker_key;
	HighRiskHashEntry *marker_entry;
	bool		found;

	/*
	 * Return immediately if high_risk_hash
	 * does not exist or has already been loaded.
	 */
	if (high_risk_hash == NULL || high_risk_hash_loaded)
		return;

	/*
	 * A HIGH_RISK_NONE entry is a per-database load-complete marker.  It is
	 * kept only in the shared hash and is not a real high-risk category.
	 */
	memset(&marker_key, 0, sizeof(marker_key));
	marker_key.dbid = MyDatabaseId;
	marker_key.kind = HIGH_RISK_NONE;

	LWLockAcquire(pgss->lock, LW_SHARED);
	marker_entry = hash_search(high_risk_hash, &marker_key, HASH_FIND, NULL);
	LWLockRelease(pgss->lock);
	if (marker_entry != NULL)
	{
		high_risk_hash_loaded = true;
		return;
	}

	/* ensure the high risk relation sql_high_risk_rec must be exists. */
	relid = get_relname_relid(HIGH_RISK_RELATION_NAME, SECURITY_SCHEMA_NAMESPACE);
	if (!OidIsValid(relid))
		return;

	/* open the table and prepare the data. */
	rel = table_open(relid, AccessShareLock);
	statement_attnum = get_attnum(relid, "statement");
	if (statement_attnum == InvalidAttrNumber)
	{
		table_close(rel, AccessShareLock);
		return;
	}

	/* Sequentially scan data. */
	scan = table_beginscan_catalog(rel, 0, NULL);
	while ((tuple = heap_getnext(scan, ForwardScanDirection)) != NULL)
	{
		bool		isnull;
		Datum		value;
		int64		warning_count = 0;
		int64		error_count = 0;

		value = heap_getattr(tuple, statement_attnum, RelationGetDescr(rel),
						 &isnull);
		if (!isnull)
		{
			char *name = TextDatumGetCString(value);
			HighRiskStatementKind kind = high_risk_kind_from_name(name);

			value = heap_getattr(tuple, Anum_sql_high_risk_rec_warning_count,
							 RelationGetDescr(rel), &isnull);
			if (!isnull)
				warning_count = DatumGetInt64(value);
			value = heap_getattr(tuple, Anum_sql_high_risk_rec_error_count,
							 RelationGetDescr(rel), &isnull);
			if (!isnull)
				error_count = DatumGetInt64(value);

			/* Add high-risk statements to the hash table. */
			if (kind != HIGH_RISK_NONE)
				set_high_risk_hash(kind, warning_count, error_count);

			pfree(name);
		}
	}

	/* Finish scanning and close the relation. */
	table_endscan(scan);
	table_close(rel, AccessShareLock);

	/*
	 * Recheck under the exclusive lock in case another backend completed the
	 * load while this backend was scanning the table.  Only the backend that
	 * still cannot find the marker inserts and initializes it.
	 */
	LWLockAcquire(pgss->lock, LW_EXCLUSIVE);
	marker_entry = hash_search(high_risk_hash, &marker_key, HASH_FIND, NULL);
	if (marker_entry == NULL)
	{
		marker_entry = hash_search(high_risk_hash, &marker_key,
								 HASH_ENTER_NULL, &found);
		if (marker_entry == NULL)
		{
			LWLockRelease(pgss->lock);
			ereport(ERROR,
					(errmsg("sql_firewall high-risk hash table is full")));
		}
		if (!found)
		{
			SpinLockInit(&marker_entry->mutex);
			marker_entry->warning_count = 0;
			marker_entry->error_count = 0;
			marker_entry->change_count = 0;
			marker_entry->persisted_count = 0;
		}
	}
	LWLockRelease(pgss->lock);

	/* Cache the shared marker lookup in this backend. */
	high_risk_hash_loaded = true;
}

/*
 * is_inside_trigger
 *
 * Determine whether a trigger is currently being invoked.
 */
static bool
is_inside_trigger(void)
{
	return DatumGetInt32(OidFunctionCall0(F_PG_TRIGGER_DEPTH)) > 0;
}

/*
 * sql_firewall_xact_read_only
 *
 * XactReadOnly is not declared PGDLLIMPORT in PostgreSQL 14, so an extension
 * cannot portably reference that backend global from a Windows DLL.  The GUC
 * exposes the same current transaction state through a public backend API.
 */
static bool
sql_firewall_xact_read_only(void)
{
	return strcmp(GetConfigOption("transaction_read_only", false, false),
				  "on") == 0;
}

/*
 * is_sql_firewall_management_function
 *
 * Determine whether the current function is
 * an administrative function of sql_firewall.
 */
static bool
is_sql_firewall_management_function(Oid funcid)
{
	FmgrInfo	flinfo;
	PGFunction	fn_addr;

	fmgr_info(funcid, &flinfo);
	fn_addr = flinfo.fn_addr;

	return fn_addr == import_white_list ||
		fn_addr == import_white_list_for_user ||
		fn_addr == import_white_list_batch ||
		fn_addr == import_white_list_batch_for_user ||
		fn_addr == export_white_list ||
		fn_addr == export_white_list_for_user ||
		fn_addr == export_white_list_sql ||
		fn_addr == export_white_list_sql_for_user ||
		fn_addr == delete_all_white_list ||
		fn_addr == delete_all_white_list_for_user ||
		fn_addr == delete_white_list ||
		fn_addr == delete_white_list_for_user ||
		fn_addr == delete_white_list_batch ||
		fn_addr == delete_white_list_batch_for_user ||
		fn_addr == import_black_list ||
		fn_addr == import_black_list_for_user ||
		fn_addr == import_black_list_batch ||
		fn_addr == import_black_list_batch_for_user ||
		fn_addr == export_black_list ||
		fn_addr == export_black_list_for_user ||
		fn_addr == export_black_list_sql ||
		fn_addr == export_black_list_sql_for_user ||
		fn_addr == export_all_sql ||
		fn_addr == dispaly_single_sql ||
		fn_addr == import_high_risk_statement ||
		fn_addr == remove_high_risk_statement ||
		fn_addr == delete_all_black_list ||
		fn_addr == delete_all_black_list_for_user ||
		fn_addr == delete_black_list ||
		fn_addr == delete_black_list_for_user ||
		fn_addr == delete_black_list_batch ||
		fn_addr == delete_black_list_batch_for_user ||
		fn_addr == sql_firewall_export_rule ||
		fn_addr == sql_firewall_import_rule ||
		fn_addr == sql_firewall_stat_warning_count ||
		fn_addr == sql_firewall_stat_error_count ||
		fn_addr == sql_firewall_stat_reset ||
		fn_addr == sql_firewall_statements ||
		fn_addr == sql_firewall_white_list ||
		fn_addr == sql_firewall_black_list ||
		fn_addr == sql_firewall_reset;
}

/*
 * is_sql_firewall_list_function
 *
 * These functions can return results as a table structure.
 */
static bool
is_sql_firewall_list_function(Oid funcid)
{
	FmgrInfo	flinfo;
	PGFunction	fn_addr;

	fmgr_info(funcid, &flinfo);
	fn_addr = flinfo.fn_addr;

	return fn_addr == sql_firewall_statements ||
		fn_addr == sql_firewall_white_list ||
		fn_addr == sql_firewall_black_list;
}

/*
 * is_sql_firewall_management_utility
 *
 * when execute alter, allow alter system these GUC paramter,
 * these paramter is sql_firewall GUC paramter.
 */
static bool
is_sql_firewall_management_utility(Node *parsetree)
{
	AlterSystemStmt *stmt;
	const char *name;

	if (!superuser() || parsetree == NULL || !IsA(parsetree, AlterSystemStmt))
		return false;

	stmt = (AlterSystemStmt *) parsetree;
	if (stmt->setstmt == NULL || stmt->setstmt->name == NULL)
		return false;

	name = stmt->setstmt->name;

	return strcmp(name, "sql_firewall.max") == 0 ||
		strcmp(name, "sql_firewall.firewall") == 0;
}

/*
 * is_sql_firewall_relation_access
 *
 * whether mode in sql_firewall.firewall, allow these table
 * and table accessed.
 */
static bool
is_sql_firewall_relation_access(List *rtable)
{
	bool		found_management_relation = false;
	static const char *const management_relations[] =
	{
		RULE_RELATION_NAME,
		HIGH_RISK_RELATION_NAME,
		RULE_STATEMENTS_VIEW_NAME,
		RULE_WHITE_LIST_VIEW_NAME,
		RULE_BLACK_LIST_VIEW_NAME,
		RULE_STAT_VIEW_NAME,
		STAT_REC_RELATION_NAME
	};
	ListCell   *lc;

	/*
	 * Access permissions are enforced by PostgreSQL.  Firewall management
	 * relations must not become firewall rules for either privileged or
	 * unprivileged users, including while the firewall is learning.
	 */
	if (rtable == NIL)
		return false;

	foreach(lc, rtable)
	{
		RangeTblEntry *rte = lfirst_node(RangeTblEntry, lc);
		bool		is_management_relation = false;
		int			i;

		/*
		 * JOIN and RESULT entries do not introduce an independent data
		 * source.  Every other entry must be proved to be a firewall-owned
		 * relation, function, or subquery; unknown entries make the complete
		 * statement a normal firewall query.
		 */
		if (rte->rtekind == RTE_JOIN || rte->rtekind == RTE_RESULT)
			continue;
		if (rte->rtekind == RTE_SUBQUERY)
		{
			if (rte->subquery == NULL ||
				!is_sql_firewall_management_query(rte->subquery))
				return false;
			found_management_relation = true;
			continue;
		}
		if (rte->rtekind == RTE_FUNCTION)
		{
			ListCell   *fc;

			if (rte->functions == NIL)
				return false;
			foreach(fc, rte->functions)
			{
				RangeTblFunction *rtfunc = lfirst_node(RangeTblFunction, fc);
				Node	   *expr = strip_implicit_coercions(rtfunc->funcexpr);

				if (!IsA(expr, FuncExpr) ||
					!is_sql_firewall_management_function(((FuncExpr *) expr)->funcid))
					return false;
			}
			found_management_relation = true;
			continue;
		}
		if (rte->rtekind != RTE_RELATION)
			return false;

		for (i = 0; i < lengthof(management_relations); i++)
		{
			Oid			relid;

			relid = get_relname_relid(management_relations[i], SECURITY_SCHEMA_NAMESPACE);
			if (OidIsValid(relid) && rte->relid == relid)
			{
				is_management_relation = true;
				break;
			}
		}

		/* A mixed query must still be checked for its non-management table. */
		if (!is_management_relation)
			return false;
		found_management_relation = true;
	}

	return found_management_relation;
}

/*
 * is_sql_firewall_function_access
 *
 * Return true only when every FROM item is a firewall management function.
 * This keeps a query which mixes such a function with a business relation
 * subject to the normal firewall checks.
 */
static bool
is_sql_firewall_function_access(List *rtable)
{
	bool		found_management_function = false;
	ListCell   *lc;

	if (rtable == NIL)
		return false;

	/* sql_firewall function returning a table structure. */
	foreach(lc, rtable)
	{
		RangeTblEntry *rte = lfirst_node(RangeTblEntry, lc);
		RangeTblFunction *rtfunc;
		Node	   *expr;

		if (rte->rtekind != RTE_FUNCTION || list_length(rte->functions) != 1)
			return false;

		/*
		 * Determine whether it is a function that
		 * can return a table structure.
		 */
		rtfunc = linitial_node(RangeTblFunction, rte->functions);
		expr = strip_implicit_coercions(rtfunc->funcexpr);
		if (!IsA(expr, FuncExpr) ||
			!is_sql_firewall_list_function(((FuncExpr *) expr)->funcid))
			return false;

		found_management_function = true;
	}

	return found_management_function;
}

/*
 * is_sql_firewall_management_query
 *
 * The management function for sql_firewall was accessed.
 */
static bool
is_sql_firewall_management_query(Query *query)
{
	TargetEntry *tle;
	Node	   *expr;

	if (query == NULL)
		return false;

	/* sql_firewall tables and views. */
	if (is_sql_firewall_relation_access(query->rtable))
		return true;

	if (query->commandType == CMD_SELECT &&
		is_sql_firewall_function_access(query->rtable))
		return true;

	/* sql_firewall function returning a table structure. */
	if (query->commandType != CMD_SELECT ||
		query->utilityStmt != NULL || list_length(query->targetList) != 1 ||
		query->rtable != NIL || query->jointree == NULL ||
		query->jointree->fromlist != NIL || query->jointree->quals != NULL)
		return false;

	tle = linitial_node(TargetEntry, query->targetList);
	if (tle->resjunk)
		return false;

	expr = strip_implicit_coercions((Node *) tle->expr);
	if (!IsA(expr, FuncExpr))
		return false;

	/* sql_firewall management functions. */
	return is_sql_firewall_management_function(((FuncExpr *) expr)->funcid);
}

/*
 * is_sql_firewall_management_plan
 *
 * The management function for sql_firewall was accessed.
 */
static bool
is_sql_firewall_management_plan(PlannedStmt *plannedstmt)
{
	Plan	   *plan;
	TargetEntry *tle;
	Node	   *expr;

	if (plannedstmt == NULL)
		return false;

	/* sql_firewall tables and views. */
	if (is_sql_firewall_relation_access(plannedstmt->rtable))
		return true;

	if (plannedstmt->commandType == CMD_SELECT &&
		is_sql_firewall_function_access(plannedstmt->rtable))
		return true;

	if (plannedstmt->commandType != CMD_SELECT)
		return false;

	plan = plannedstmt->planTree;
	if (plan == NULL || !IsA(plan, Result) ||
		list_length(plan->targetlist) != 1 || plan->qual != NIL)
		return false;

	tle = linitial_node(TargetEntry, plan->targetlist);
	if (tle->resjunk)
		return false;

	expr = strip_implicit_coercions((Node *) tle->expr);
	if (!IsA(expr, FuncExpr))
		return false;

	/* sql_firewall management functions. */
	return is_sql_firewall_management_function(((FuncExpr *) expr)->funcid);
}

/*
 * pgss_store
 *
 * If jstate is not NULL then we're trying to create
 * an entry for which we have no statistics as yet;
 * we just want to record the normalized query string,
 * total_time, rows, bufusage are ignored in this case.
 */
static void
pgss_store(const char *query, uint64 queryId,
		   double total_time, uint64 rows,
		   pgssJumbleState *jstate,
		   HighRiskStatementKind high_risk, bool parse_only,
		   bool check_firewall, bool count_call, Oid userid)
{
	sqlfirewallPgssHashKey key;
	sqlfirewallPgssEntry  *entry;
	sqlfirewallPgssEntry  *black_entry;
	char	   *norm_query = NULL;
	int			encoding = GetDatabaseEncoding();
	int			query_len;
	sqlfirewallPgssHashKey persist_key;
	bool		persist_entry = false;
	bool		persist_new_entry = false;
	HighRiskStatementKind high_risk_warning = HIGH_RISK_NONE;

	Assert(query != NULL);

	/* Safety check... */
	if (!pgss || !pgss_hash)
		return;

	query_len = strlen(query);

	/* Set up key for hashtable search */
	key.dbid = MyDatabaseId;
	key.userid = userid;
	key.queryid = queryId;
	key.statement_type = SQL_FIREWALL_STATEMENT_WHITE;

	/* Lookup the hash table entry with shared lock. */
	LWLockAcquire(pgss->lock, LW_SHARED);

	/* The system checks the black list first,
	 * within the black list, the global black list
	 * takes precedence over the user-specific black list.
	 */
	black_entry = find_effective_entry(userid, queryId,
								   SQL_FIREWALL_STATEMENT_BLACK);
	/* When reporting errors, prioritize high-risk errors,
	 * report blockages related to whitelists/blocklists
	 * only if there are no high-risk errors.
	 */
	if (check_firewall && black_entry && pgfw_mode != PGFW_MODE_DISABLED)
	{
		if (pgfw_mode == PGFW_MODE_ENFORCING)
		{
			SpinLockAcquire(&black_entry->mutex);
			black_entry->counters.calls++;
			black_entry->counters.error_count++;
			black_entry->counters.change_count++;
			SpinLockRelease(&black_entry->mutex);
			LWLockRelease(pgss->lock);
			if (high_risk != HIGH_RISK_NONE)
			{
				/* Update the count of high-risk error. */
				high_risk_stat_increment(high_risk, true);
			}
			stat_error_increment();
			if (high_risk)
				ereport(ERROR,
					(errcode(ERRCODE_S_R_E_PROHIBITED_SQL_STATEMENT_ATTEMPTED),
					 errmsg("Prohibited high-risk SQL statement"),
					 errdetail("SQL statement: %s", query)));
			else
				ereport(ERROR,
					(errcode(ERRCODE_S_R_E_PROHIBITED_SQL_STATEMENT_ATTEMPTED),
					 errmsg("Prohibited black list SQL statement"),
					 errdetail("SQL statement: %s", query)));
		}
		else if (pgfw_mode == PGFW_MODE_PERMISSIVE)
		{
			SpinLockAcquire(&black_entry->mutex);
			black_entry->counters.calls++;
			black_entry->counters.warning_count++;
			black_entry->counters.change_count++;
			SpinLockRelease(&black_entry->mutex);
			high_risk_warning = high_risk;
			if (high_risk)
				ereport(WARNING,
						(errmsg("A high-risk statement is currently being executed, it is being executed in permissive mode."),
						 errdetail("SQL statement: %s", query)));
			else
				ereport(WARNING,
						(errmsg("The current statement is a blacklisted statement, it is being executed in permissive mode."),
						 errdetail("SQL statement: %s", query)));
			stat_warning_increment();
			goto done;
		}
	}

	/* Then the system checks the white list first,
	 * within the white list, the global white list
	 * takes precedence over the user-specific white list.
	 */
	entry = find_effective_entry(userid, queryId,
							 SQL_FIREWALL_STATEMENT_WHITE);

	if (!entry)
		entry = (sqlfirewallPgssEntry *) hash_search(pgss_hash, &key, HASH_FIND, NULL);

	if (entry && entry->statement_type != SQL_FIREWALL_STATEMENT_WHITE)
		entry = NULL;

	/* When reporting errors, prioritize high-risk errors,
	 * report blockages related to white lists only if there
	 * are no high-risk errors.
	 */
	if (check_firewall && !entry)
	{
		if (pgfw_mode == PGFW_MODE_ENFORCING)
		{
			LWLockRelease(pgss->lock);
			if (high_risk != HIGH_RISK_NONE)
				high_risk_stat_increment(high_risk, true);
			stat_error_increment();
			if (high_risk)
				ereport(ERROR,
					(errcode(ERRCODE_S_R_E_PROHIBITED_SQL_STATEMENT_ATTEMPTED),
					 errmsg("Prohibited high-risk SQL statement"),
					 errdetail("SQL statement: %s", query)));
			else
				ereport(ERROR,
					(errcode(ERRCODE_S_R_E_PROHIBITED_SQL_STATEMENT_ATTEMPTED),
					 errmsg("Prohibited SQL statement"),
					 errdetail("SQL statement: %s", query)));
		}
		else if (pgfw_mode == PGFW_MODE_PERMISSIVE)
		{
			high_risk_warning = high_risk;
			if (high_risk)
				ereport(WARNING,
						(errmsg("A high-risk statement is currently being executed, it is being executed in permissive mode."),
						 errdetail("SQL statement: %s", query)));
			else
				ereport(WARNING,
						(errmsg("The current statement is neither a whitelist nor a blacklist statement, it is being executed in permissive mode."),
						 errdetail("SQL statement: %s", query)));
			stat_warning_increment();
			goto done;
		}
	}

	/* Create new entry, if not present */
	if (!entry && pgfw_mode == PGFW_MODE_LEARNING)
	{
		/*
		 * Create a new, normalized query string if caller asked.
		 * We don't need to hold the lock while doing this work.
		 * Another backend can create a matching entry while the lock is
		 * released, so recheck the hash after acquiring the exclusive lock.
		 */
		if (jstate)
		{
			LWLockRelease(pgss->lock);
			norm_query = generate_normalized_query(jstate, query,
												   &query_len,
												   encoding);
			LWLockAcquire(pgss->lock, LW_SHARED);
		}

		/* Need exclusive lock to make a new hashtable entry - promote */
		LWLockRelease(pgss->lock);
		LWLockAcquire(pgss->lock, LW_EXCLUSIVE);

		/*
		 * Recheck while holding the exclusive lock.  The shared hash may
		 * have been changed by another backend while we promoted the lock.
		 * Use the effective lookup so a newly inserted global rule is also
		 * honored.
		 */
		entry = find_effective_entry(userid, queryId,
								 SQL_FIREWALL_STATEMENT_WHITE);

		if (!entry)
		{
			entry = entry_alloc(&key, SQL_FIREWALL_STATEMENT_WHITE);
			persist_new_entry = entry != NULL;
		}
	}

	/* Count a matching rule once when this call represents execution. */
	if (!parse_only && count_call)
	{
		/*
		 * Grab the spinlock while updating the counters
		 * (see comment about locking rules at the head of
		 * the file)
		 */
		volatile sqlfirewallPgssEntry *e = (volatile sqlfirewallPgssEntry *) entry;

		/*
		 * If the entry is not found, it means the statement
		 * is not in the firewall list.
		 */
		if (!e)
		  goto done;

		SpinLockAcquire(&e->mutex);

		e->counters.calls += 1;

		SpinLockRelease(&e->mutex);
		persist_key = entry->key;
		persist_entry = true;
	}

done:
	LWLockRelease(pgss->lock);
	if (high_risk_warning != HIGH_RISK_NONE)
		high_risk_stat_increment(high_risk_warning, false);
	if (persist_new_entry)
		persist_rule(key.userid, key.queryid,
					 norm_query ? norm_query : query, query,
					 count_call ? 1 : 0, 0, 0, encoding,
					 SQL_FIREWALL_STATEMENT_WHITE);
	if (persist_entry)
		persist_hash_entry(&persist_key);

	/* We postpone this clean-up until we're out of the lock */
	if (norm_query)
		pfree(norm_query);
}

/*
 * sql_firewall_reset
 *
 * Reset all statement statistics.
 */
Datum
sql_firewall_reset(PG_FUNCTION_ARGS)
{
	/* Permission check. */
	check_sql_firewall_function_permission("sql_firewall_reset");

	if (!pgss || !pgss_hash)
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("sql_firewall must be loaded via shared_preload_libraries")));

	/* clear all entries from hash. */
	entry_reset();

	/* clear all entries from relation sql_firewall_rec. */
	delete_all_persisted_rules();

	PG_RETURN_VOID();
}

/* Number of output arguments (columns) for the SQL-facing functions. */
#define SQL_FIREWALL_COLS			7		/* maximum of above */
#define SQL_FIREWALL_WHITE_COLS		5
#define SQL_FIREWALL_CSV_COLS			8

/*
 * sql_firewall_statements
 *
 * The SQL API of this function has changed multiple times, and will likely
 * do so again in future.  To support the case where a newer version of this
 * loadable module is being used with an old SQL declaration of the function,
 * we continue to support the older API versions.  For 1.2 and later, the
 * expected API version is identified by embedding it in the C name of the
 * function.  Unfortunately we weren't bright enough to do that for 1.1.
 */
Datum
sql_firewall_statements(PG_FUNCTION_ARGS)
{
	bool		showtext = PG_GETARG_BOOL(0);

	/* Permission check. */
	check_sql_firewall_function_permission("sql_firewall_statements");

	pg_stat_statements_internal(fcinfo, showtext,
								SQL_FIREWALL_STATEMENT_ALL);

	return (Datum) 0;
}

/*
 * sql_firewall_white_list
 *
 * query all white list.
 */
Datum
sql_firewall_white_list(PG_FUNCTION_ARGS)
{
	bool		showtext = PG_GETARG_BOOL(0);

	/* Permission check. */
	check_sql_firewall_function_permission("sql_firewall_white_list");

	pg_stat_statements_internal(fcinfo, showtext,
								SQL_FIREWALL_STATEMENT_WHITE);

	return (Datum) 0;
}

/*
 * sql_firewall_black_list
 *
 * query all black list.
 */
Datum
sql_firewall_black_list(PG_FUNCTION_ARGS)
{
	bool		showtext = PG_GETARG_BOOL(0);

	/* Permission check. */
	check_sql_firewall_function_permission("sql_firewall_black_list");

	pg_stat_statements_internal(fcinfo, showtext,
								SQL_FIREWALL_STATEMENT_BLACK);

	return (Datum) 0;
}

/*
 * pg_stat_statements_internal
 *
 * Common code for all versions of pg_stat_statements().
 */
static void
pg_stat_statements_internal(FunctionCallInfo fcinfo,
							bool showtext, int statement_type)
{
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
	TupleDesc	tupdesc;
	Tuplestorestate *tupstore;
	MemoryContext per_query_ctx;
	MemoryContext oldcontext;
	Oid			userid = GetUserId();
	bool		is_superuser = superuser();
	List	   *rules;
	ListCell   *lc;

	/* hash table must exist already */
	if (!pgss || !pgss_hash)
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("sql_firewall must be loaded via shared_preload_libraries")));

	/* check to see if caller supports us returning a tuplestore */
	if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("set-valued function called in context that cannot accept a set")));
	if (!(rsinfo->allowedModes & SFRM_Materialize))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("materialize mode required, but it is not " \
						"allowed in this context")));

	/* Switch into long-lived context to construct returned data structures */
	per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
	oldcontext = MemoryContextSwitchTo(per_query_ctx);

	/* Build a tuple descriptor for our result type */
	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");

	/*
	 * Check we have the expected number of output arguments.  Aside from
	 * being a good safety check, we need a kluge here to detect API version
	 * 1.1, which was wedged into the code in an ill-considered way.
	 */
	Assert(tupdesc->natts ==
		   (statement_type == SQL_FIREWALL_STATEMENT_WHITE ?
			SQL_FIREWALL_WHITE_COLS : SQL_FIREWALL_COLS));

	tupstore = tuplestore_begin_heap(true, false, work_mem);
	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = tupstore;
	rsinfo->setDesc = tupdesc;

	MemoryContextSwitchTo(oldcontext);

	persist_changed_rule_counts();
	rules = append_runtime_only_rules(load_persisted_rules());
	foreach(lc, rules)
	{
		SqlFirewallPersistedRule *rule = lfirst(lc);
		Datum		values[SQL_FIREWALL_COLS];
		bool		nulls[SQL_FIREWALL_COLS];
		int			i = 0;
		int64		queryid = rule->queryid;

		if (statement_type != SQL_FIREWALL_STATEMENT_ALL &&
			rule->statement_type != statement_type)
			continue;

		memset(values, 0, sizeof(values));
		memset(nulls, 0, sizeof(nulls));

		values[i++] = ObjectIdGetDatum(rule->userid);

		if (is_superuser || rule->userid == userid ||
			(statement_type != SQL_FIREWALL_STATEMENT_ALL &&
			 rule->userid == InvalidOid))
		{
			values[i++] = Int64GetDatumFast(queryid);

			if (showtext)
			{
				if (rule->query)
				{
					char	   *enc;

					enc = pg_any_to_server(rule->query,
										   strlen(rule->query),
										   rule->encoding);

					values[i++] = CStringGetTextDatum(enc);

					if (enc != rule->query)
						pfree(enc);
				}
				else
				{
					/* Just return a null if we fail to find the text */
					nulls[i++] = true;
				}
			}
			else
			{
				/* Query text not requested */
				nulls[i++] = true;
			}
		}
		else
		{
			/* Don't show queryid */
			nulls[i++] = true;

			/*
			 * Don't show query text, but hint as to the reason for not doing
			 * so if it was requested
			 */
			if (showtext)
				values[i++] = CStringGetTextDatum("<insufficient privilege>");
			else
				nulls[i++] = true;
		}

		/* The statistics view omits unexecuted entries; list views do not. */
		if (statement_type == SQL_FIREWALL_STATEMENT_ALL && rule->calls == 0)
			continue;

		values[i++] = CStringGetTextDatum(statement_type_name(rule->statement_type));
		values[i++] = Int64GetDatumFast(rule->calls);
		/* All 7-column APIs expose the persisted warning/error counters. */
		if (statement_type != SQL_FIREWALL_STATEMENT_WHITE)
		{
			values[i++] = Int64GetDatumFast(rule->warning_count);
			values[i++] = Int64GetDatumFast(rule->error_count);
		}

		tuplestore_putvalues(tupstore, tupdesc, values, nulls);
	}

	/* clean up and return the tuplestore */
	free_persisted_rules(rules);

	tuplestore_donestoring(tupstore);
}

/*
 * sql_firewall_stat_warning_count
 *
 * query the number of warnings.
 */
Datum
sql_firewall_stat_warning_count(PG_FUNCTION_ARGS)
{
	int64		count = 0;

	/* Permission check. */
	check_sql_firewall_function_permission("sql_firewall_stat_warning_count");

	ensure_stat_count_loaded();
	persist_stat_count();
	if (current_stat_count == NULL)
		PG_RETURN_INT64(0);

	SpinLockAcquire(&current_stat_count->mutex);
	count = current_stat_count->warning_count;
	SpinLockRelease(&current_stat_count->mutex);

	PG_RETURN_INT64(count);
}

/*
 * sql_firewall_stat_error_count
 *
 * query the number of all error.
 */
Datum
sql_firewall_stat_error_count(PG_FUNCTION_ARGS)
{
	int64		count = 0;

	/* Permission check. */
	check_sql_firewall_function_permission("sql_firewall_stat_error_count");

	ensure_stat_count_loaded();
	persist_stat_count();
	if (current_stat_count == NULL)
		PG_RETURN_INT64(0);

	SpinLockAcquire(&current_stat_count->mutex);
	count = current_stat_count->error_count;
	SpinLockRelease(&current_stat_count->mutex);

	PG_RETURN_INT64(count);
}

/*
 * sql_firewall_stat_reset
 *
 * delete the number of warnings and errors.
 */
Datum
sql_firewall_stat_reset(PG_FUNCTION_ARGS)
{
	/* Permission check. */
	check_sql_firewall_function_permission("sql_firewall_stat_reset");

	ensure_stat_count_loaded();
	if (current_stat_count == NULL)
		PG_RETURN_VOID();

	SpinLockAcquire(&current_stat_count->mutex);
	current_stat_count->warning_count = 0;
	current_stat_count->error_count = 0;
	current_stat_count->change_count++;
	SpinLockRelease(&current_stat_count->mutex);
	persist_stat_count();

	PG_RETURN_VOID();
}

/*
 * write_csv_field
 *
 * write the data of csv format to file.
 */
static void
write_csv_field(FILE *filep, const char *value)
{
	const char *p;
	bool		need_quote;

	need_quote = strpbrk(value, "\n\r,\"") != NULL;
	if (need_quote)
		fputc('"', filep);
	for (p = value; *p; p++)
	{
		if (*p == '"')
			fputc('"', filep);
		fputc(*p, filep);
	}
	if (need_quote)
		fputc('"', filep);
}

/*
 * sql_firewall_export_rule
 *
 * sql_firewall_export_rule() exports only part of sqlfirewallPgssEntry members
 * (userid, queryid, query string, and number of calls) in CSV format.
 *
 * Query text is read directly from hg_security.sql_firewall_rec.
 */
Datum
sql_firewall_export_rule(PG_FUNCTION_ARGS)
{
	char	   *rule_file = text_to_cstring(PG_GETARG_TEXT_P(0));

	/* Permission check. */
	check_sql_firewall_function_permission("sql_firewall_export_rule");

	/* hash table must exist already */
	if (!pgss || !pgss_hash)
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("sql_firewall must be loaded via shared_preload_libraries")));

	PG_RETURN_BOOL(export_list_file(rule_file, InvalidOid, false,
								 SQL_FIREWALL_STATEMENT_ALL));
}

/*
 * pgss_restore
 *
 * when import black or white list,
 * store the record to hash.
 */
static bool
pgss_restore(Oid userid, uint64 queryid, const char *query,
			 const char *original_query, int64 calls, int64 warning_count,
			 int64 error_count, int statement_type, bool persist)
{
	sqlfirewallPgssHashKey key;
	sqlfirewallPgssEntry  *entry;
	int			encoding = GetDatabaseEncoding();
	bool ret = false;

	key.dbid = MyDatabaseId;
	key.userid = userid;
	key.queryid = queryid;
	key.statement_type = statement_type;

	/*
	 * Check the entry in the hash table.
	 */
	LWLockAcquire(pgss->lock, LW_SHARED);

	entry = (sqlfirewallPgssEntry *) hash_search(pgss_hash, &key, HASH_FIND, NULL);
	if (entry)
	{
		SpinLockAcquire(&entry->mutex);
		entry->counters.calls = calls;
		entry->counters.warning_count = warning_count;
		entry->counters.error_count = error_count;
		entry->counters.change_count = 0;
		entry->counters.persisted_count = 0;
		SpinLockRelease(&entry->mutex);
		elog(DEBUG1, "userid %d, queryid " UINT64_FORMAT " already exists.", key.userid, key.queryid);
		ret = true;
		goto done;
	}

	LWLockRelease(pgss->lock);

	/*
	 * Restore the entry
	 */
	LWLockAcquire(pgss->lock, LW_EXCLUSIVE);

	entry = entry_alloc(&key, statement_type);
	if (!entry)
	{
		elog(ERROR, "Could not allocate an entry in the hash table.");
		goto done;
	}

	SpinLockAcquire(&entry->mutex);
	entry->counters.calls = calls;
	entry->counters.warning_count = warning_count;
	entry->counters.error_count = error_count;
	entry->counters.change_count = 0;
	entry->counters.persisted_count = 0;
	SpinLockRelease(&entry->mutex);

	ret = true;

done:
	LWLockRelease(pgss->lock);
	if (ret && persist)
		persist_rule(userid, queryid, query, original_query, calls,
					 warning_count, error_count, encoding, statement_type);
	return ret;
}

#define CSV_DEFAULT     1
#define CSV_SEPARATOR   2
#define CSV_NON_QUOTED  3
#define CSV_QUOTED      4

/*
 * parse_csv_values
 *
 * parse csv data to values.
 */
static int
parse_csv_values(const char *buf, char **values)
{
	int i;
	int cols = 1;
	int off = 0;
	int buflen = strlen(buf);
	int len = 0;
	int mode = CSV_DEFAULT;

	int buf2len = 512;
	char *buf2 = palloc(buf2len);
	memset(buf2, 0, buf2len);

	for (i = 0 ; i < buflen ; i++)
	{
		int field_end = false;

		switch (buf[i])
		{
			case '"':
				switch (mode)
				{
					case CSV_DEFAULT:
					case CSV_SEPARATOR:
						mode = CSV_QUOTED;
						off++;
						/* len should still be zero. */
						break;

					case CSV_NON_QUOTED:
						/* it's not correct csv format, but ok. */
						buf2[len++] = buf[i];
						break;

					case CSV_QUOTED:
						if (i < buflen - 1 && buf[i+1] == '"')
						{
							/* skip next char, and continue reading */
							i++;
							buf2[len++] = buf[i];
						}
						else
						{
							field_end = true;
							mode = CSV_DEFAULT;
						}
						break;
				}
				break;

			case ',':
				switch (mode)
				{
					case CSV_DEFAULT:
						break;

					case CSV_SEPARATOR:
					case CSV_NON_QUOTED:
						/* end of the field */
						field_end = true;
						mode = CSV_SEPARATOR;
						break;

					case CSV_QUOTED:
						buf2[len++] = buf[i];
						break;
				}
				break;

			default:
				switch (mode)
				{
					case CSV_DEFAULT:
					case CSV_SEPARATOR:
						off = i;
						mode = CSV_NON_QUOTED;
						break;
				}
				buf2[len++] = buf[i];
				break;
		}
		if (field_end == true)
		{
			/* end of the field */
			if (cols > SQL_FIREWALL_CSV_COLS)
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
						 errmsg("too many fields in sql_firewall CSV record")));
			values[cols-1] = palloc(len + 1);
			memset(values[cols-1], 0, len + 1);
			strncpy(values[cols-1], buf2, len);

			memset(buf2, 0, buf2len);
			off = i + 1;
			len = 0;
			cols++;

			field_end = false;
		}

		if (strlen(buf2) == buf2len - 1)
		{
			buf2 = repalloc(buf2, buf2len * 2);
			memset(buf2 + buf2len, 0, buf2len);
			buf2len = buf2len * 2;
		}
	}
	/* A quoted field spanning physical lines is not a complete CSV record. */
	if (mode == CSV_QUOTED)
	{
		for (i = 0; i < cols - 1; i++)
			pfree(values[i]);
		pfree(buf2);
		return 0;
	}
	if (off > 0)
	{
		/* end of the field */
		if (cols > SQL_FIREWALL_CSV_COLS)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
					 errmsg("too many fields in sql_firewall CSV record")));
		values[cols-1] = palloc(len + 1);
		memset(values[cols-1], 0, len + 1);
		strncpy(values[cols-1], buf2, len);

		memset(buf2, 0, buf2len);

		cols++;
	}

	pfree(buf2);

	return cols - 1;
}

/*
 * restore_csv_record
 *
 * Parse and restore one complete CSV record.  Return false when a quoted
 * field continues on a following physical line.
 */
static bool
restore_csv_record(const char *record, bool *restored)
{
	char	   *values[SQL_FIREWALL_CSV_COLS];
	int			nvalues;
	int			i;

	nvalues = parse_csv_values(record, values);
	if (nvalues == 0)
		return false;

	if (nvalues != SQL_FIREWALL_CSV_COLS)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid field count in sql_firewall CSV record: expected %d, got %d",
						SQL_FIREWALL_CSV_COLS, nvalues)));

	for (i = 0; i < nvalues; i++)
	{
		char	*str = values[i];

		if (values[i] == NULL)
		{
			ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("CSV-formatted data does not allow empty columns.")));
		}

		if (strlen(values[i]) == 0)
		{
			ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("CSV-formatted data does not allow empty columns.")));
		}

		while (*str && isspace((unsigned char) *str))
			str++;

		if (*str == '\0')
		{
			ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("CSV-formatted data does not allow empty columns.")));
		}
	}

	*restored = pgss_restore(atoi(values[0]),
							 strtoull(values[1], NULL, 10),
							 values[2], values[3], atol(values[5]),
							 strtoll(values[6], NULL, 10),
							 strtoll(values[7], NULL, 10),
							 parse_statement_type(values[4]), true);

	for (i = 0; i < nvalues; i++)
	{
		elog(DEBUG1, "sql_firewall_import_rule: values[%d] = %s",
			 i, values[i]);
		pfree(values[i]);
	}

	return true;
}

/*
 * sql_firewall_import_rule
 *
 * import data from the csv file.
 */
Datum
sql_firewall_import_rule(PG_FUNCTION_ARGS)
{
	char	   *rule_file = text_to_cstring(PG_GETARG_TEXT_P(0));
	FILE	   *filep;
	bool		ret = true;
	char		buf[8192];
	StringInfoData record;
	struct stat st;

	/* Permission check. */
	check_sql_firewall_function_permission("sql_firewall_import_rule");

	/* hash table must exist already */
	if (!pgss || !pgss_hash)
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("sql_firewall must be loaded via shared_preload_libraries")));

	if (stat(rule_file, &st) != 0)
	{
		ereport(ERROR,
			(errmsg("could not stat file \"%s\": %m",
				rule_file)));
	}

	if (!S_ISREG(st.st_mode))
	{
		ereport(ERROR,
			(errmsg("\"%s\" is not a regular file",
				rule_file)));
	}

	filep = AllocateFile(rule_file, PG_BINARY_R);
	if (filep == NULL)
		ereport(ERROR,
			(errmsg("could not open file \"%s\": %m",
				rule_file)));

	elog(DEBUG1, "sql_firewall_import_rule: file open, %s", rule_file);

	initStringInfo(&record);
	while (fgets(buf, sizeof(buf), filep) != NULL)
	{
		bool		restored;

		/*
		 * StringInfo grows as needed, so a physical line or a quoted CSV
		 * record can be larger than the fixed-size input buffer.
		 */
		appendStringInfoString(&record, buf);
		if (record.len == 0 || record.data[record.len - 1] != '\n')
			continue;

		if (restore_csv_record(record.data, &restored))
		{
			ret = ret && restored;
			resetStringInfo(&record);
		}
	}

	if (ferror(filep))
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not read file \"%s\": %m", rule_file)));

	/* Process a valid final record even when the file has no trailing newline. */
	if (record.len > 0)
	{
		bool		restored;

		if (!restore_csv_record(record.data, &restored))
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
					 errmsg("unterminated quoted field in sql_firewall CSV file")));
		ret = ret && restored;
	}
	pfree(record.data);

	if (FreeFile(filep))
		ereport(ERROR,
			(errcode_for_file_access(),
			 errmsg("could not close file \"%s\": %m",
				rule_file)));

	elog(DEBUG1, "sql_firewall_import_rule: file close, %s", rule_file);

	PG_RETURN_BOOL(ret);
}

/*
 * check_sql_firewall_loaded
 *
 * check the hash pgss_hash and shared memory state.
 */
static void
check_sql_firewall_loaded(void)
{
	if (!pgss || !pgss_hash)
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("sql_firewall must be loaded via shared_preload_libraries")));
}

/*
 * check_sql_firewall_function_permission
 *
 * Permission check.
 */
static void
check_sql_firewall_function_permission(const char *funcname)
{
	/*When the xxxx is enabled, only the syssso user group can use function `xx`.*/
	if (se_feature_mode)
	{
		if (!IS_SYSSSO(GetUserId()))
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					 errmsg("When the se_feature_mode are enabled, only the syssso user group can use function %s.", funcname)));
		}
	}
	else
	{
		if (!superuser())
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					 errmsg("When the se_feature_mode are disabled, only the superuser can use function %s.", funcname)));
		}
	}
}

/*
 * sql_firewall_role_name_to_oid
 *
 * Resolve the role name supplied by a list-management SQL function.
 */
static Oid
sql_firewall_role_name_to_oid(Name username)
{
	return get_role_oid(NameStr(*username), false);
}

/*
 * sql_firewall_compute_queryid
 *
 * Calculation of the query number.
 */
static List *
sql_firewall_compute_queryid(const char *sqltext, bool normalize_query)
{
	List	   *raw_parsetree_list;
	List	   *queryids = NIL;
	ListCell   *lc;

	raw_parsetree_list = pg_parse_query(sqltext);

	foreach(lc, raw_parsetree_list)
	{
		RawStmt    *rawstmt = lfirst_node(RawStmt, lc);
		SqlFirewallQueryId *result = palloc0(sizeof(SqlFirewallQueryId));
		int			query_location = rawstmt->stmt_location;
		int			query_len;
		char	   *statement;
		List	   *statement_raw_list;
		RawStmt    *statement_rawstmt;
		Query	   *query;
		pgssJumbleState *jstate = NULL;
		int			save_pgfw_mode = pgfw_mode;

		/*
		 * When the raw parser knows the statement boundary, it records the byte
		 * length in stmt_len.  That length stops immediately before the
		 * boundary semicolon, so include the semicolon when it is present.
		 */
		if (rawstmt->stmt_len > 0)
		{
			query_len = rawstmt->stmt_len;
			if (sqltext[query_location + query_len] == ';')
				query_len++;
		}
		/*
		 * A zero stmt_len means "through the end of the source string", so
		 * derive the length from the remaining text.
		 */
		else
			query_len = strlen(sqltext + query_location);

		/*
		 * A RawStmt after a semicolon can start at the whitespace preceding its
		 * first token.  Advance the start and reduce the length together so the
		 * isolated statement has no leading whitespace and the same endpoint.
		 */
		while (query_len > 0 &&
			   isspace((unsigned char) sqltext[query_location]))
		{
			query_location++;
			query_len--;
		}

		statement = pnstrdup(sqltext + query_location, query_len);
		statement_raw_list = pg_parse_query(statement);
		Assert(list_length(statement_raw_list) == 1);
		statement_rawstmt = linitial_node(RawStmt, statement_raw_list);

		/*
		 * Parse analysis is the authoritative way to classify a statement.
		 * Raw parse node tags are not a stable substitute for Query.commandType;
		 * for example, SELECT INTO is transformed into a utility Query here.
		 */
		pgfw_mode = PGFW_MODE_DISABLED;
		PG_TRY();
		{
			query = parse_analyze(statement_rawstmt, statement, NULL, 0, NULL);
		}
		PG_CATCH();
		{
			pgfw_mode = save_pgfw_mode;
			PG_RE_THROW();
		}
		PG_END_TRY();
		pgfw_mode = save_pgfw_mode;
		result->is_management = is_sql_firewall_management_query(query);

		/*
		 * ProcessUtility identifies utility statements from their source text.
		 * Apply the same canonicalization here even if parse analysis assigned a
		 * nonzero queryId (CREATE FUNCTION can do so because of its SQL body),
		 * otherwise an imported rule could never match the executed statement.
		 */
		if (query->utilityStmt != NULL || query->commandType == CMD_UTILITY)
		{
			result->queryid = pgss_hash_normalized_query(statement, NULL);
		}
		else if (query->queryId != UINT64CONST(0))
		{
			jstate = JumbleQuery(query, statement);
			result->queryid = pgss_hash_normalized_query(statement, jstate);

			if (normalize_query)
			{
				if (jstate && jstate->clocations_count > 0)
				{
					int			normalized_len = query_len;

					result->query = generate_normalized_query(jstate, statement,
														 &normalized_len,
														 GetDatabaseEncoding());
				}
			}
		}
		else
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("could not compute a query identifier"),
					 errdetail("A non-utility statement cannot be imported when its queryId is zero."),
					 errhint("Enable compute_query_id before importing the statement.")));

		if (result->query == NULL)
			result->query = pstrdup(statement);
		result->original_query = statement;
		queryids = lappend(queryids, result);
	}

	return queryids;
}

/*
 * sql_firewall_compute_single_queryid
 *
 * Calculation of the query number.
 */
static uint64
sql_firewall_compute_single_queryid(const char *sqltext,
									uint64 fallback_queryid,
									bool *is_management)
{
	List	   *raw_list;
	List	   *queryids;
	SqlFirewallQueryId *result;
	uint64		queryid = fallback_queryid;
	const char *p;

	/* if the sql statement is null, return null. */
	if (sqltext == NULL)
		return fallback_queryid;

	if (is_management != NULL)
		*is_management = false;

	/*
	 * Internal executor SQL commonly contains
	 * unbound ParamRefs such as $1.
	 */
	for (p = sqltext; *p != '\0'; p++)
	{
		if (*p == '$' && isdigit((unsigned char) p[1]))
		{
			return pgss_hash_normalized_query(sqltext, NULL);
		}
	}

	/*
	 * Re-parse and execute the SQL statement;
	 * if multiple query trees are generated,
	 * return the original query ID.
	 */
	raw_list = pg_parse_query(sqltext);
	if (list_length(raw_list) != 1)
	{
		return fallback_queryid;
	}

	/*
	 * Re-parse the SQL statement without generating
	 * a new query tree.
	 */
	queryids = sql_firewall_compute_queryid(sqltext, false);
	if (list_length(queryids) == 1)
	{
		result = linitial(queryids);
		queryid = result->queryid;

		/*
		 * Determine whether the current call is to
		 * a management function of sql_firewall.
		 */
		if (is_management != NULL)
		{
			*is_management = result->is_management;
		}
	}

	/* Clear intermediate variables. */
	sql_firewall_free_queryids(queryids);

	return queryid;
}

/*
 * sql_firewall_free_queryids
 *
 * Delete the intermediate temporary linked list.
 */
static void
sql_firewall_free_queryids(List *queryids)
{
	ListCell   *lc;

	foreach(lc, queryids)
	{
		SqlFirewallQueryId *result = lfirst(lc);

		pfree(result->query);
		pfree(result->original_query);
		pfree(result);
	}
	list_free(queryids);
}

/*
 * add_list_rule
 *
 * Load data from the temporary linked list
 * into the hash table and the user table.
 */
static bool
add_list_rule(Oid userid, uint64 queryid, const char *query,
			  const char *original_query, int statement_type, int64 calls)
{
	sqlfirewallPgssHashKey key;
	sqlfirewallPgssEntry  *entry;
	int			encoding = GetDatabaseEncoding();

	key.dbid = MyDatabaseId;
	key.userid = userid;
	key.queryid = queryid;
	key.statement_type = statement_type;
	LWLockAcquire(pgss->lock, LW_EXCLUSIVE);

	entry = (sqlfirewallPgssEntry *) hash_search(pgss_hash, &key, HASH_FIND, NULL);
	if (entry)
	{
		if (entry->statement_type != statement_type)
		{
			LWLockRelease(pgss->lock);
			ereport(ERROR,
					(errcode(ERRCODE_DUPLICATE_OBJECT),
					 errmsg("SQL rule already exists in %s statement",
							statement_type_name(entry->statement_type))));
		}

		SpinLockAcquire(&entry->mutex);
		entry->counters.calls = calls;
		SpinLockRelease(&entry->mutex);

		LWLockRelease(pgss->lock);
		persist_rule(userid, queryid, query, original_query, calls, 0, 0,
					 encoding, statement_type);
		return true;
	}

	entry = entry_alloc(&key, statement_type);
	if (!entry)
	{
		LWLockRelease(pgss->lock);
		ereport(ERROR,
				(errmsg("could not allocate an entry in the hash table")));
	}

	SpinLockAcquire(&entry->mutex);
	entry->counters.calls = calls;
	SpinLockRelease(&entry->mutex);

	LWLockRelease(pgss->lock);
	persist_rule(userid, queryid, query, original_query, calls, 0, 0,
				 encoding, statement_type);
	return true;
}

/*
 * reject_prepare_in_list_import
 *
 * don't deal with prepare statement.
 */
static void
reject_prepare_in_list_import(const char *sqltext)
{
	List	   *raw_parsetree_list;
	ListCell   *lc;

	raw_parsetree_list = pg_parse_query(sqltext);
	foreach(lc, raw_parsetree_list)
	{
		RawStmt    *rawstmt = lfirst_node(RawStmt, lc);

		if (IsA(rawstmt->stmt, PrepareStmt) ||
			IsA(rawstmt->stmt, ExecuteStmt) ||
			IsA(rawstmt->stmt, DeallocateStmt))
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("prepared statement commands are not supported in SQL firewall list imports")));
	}
}

/*
 * require_list_sql_terminator
 *
 * List imports and deletions deliberately require an explicit statement
 * terminator.  The parser accepts end-of-input as an implicit terminator, but
 * list-management files and function arguments use an unambiguous format.
 */
static void
require_list_sql_terminator(const char *sqltext)
{
	const char *end = sqltext + strlen(sqltext);

	while (end > sqltext && isspace((unsigned char) end[-1]))
		end--;

	if (end == sqltext || end[-1] != ';')
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("SQL firewall list statement must end with a semicolon")));
}

/*
 * import_list_sql
 *
 * import black list or white list.
 */
static bool
import_list_sql(Oid userid, const char *sqltext, int statement_type)
{
	List	   *queryids;
	ListCell   *lc;

	/* check the hash pgss_hash was loaded. */
	check_sql_firewall_loaded();
	require_list_sql_terminator(sqltext);

	/* Don't allow import prepare statement. */
	reject_prepare_in_list_import(sqltext);

	/* Calculate the SQL information to be imported. */
	queryids = sql_firewall_compute_queryid(sqltext, true);
	foreach(lc, queryids)
	{
		SqlFirewallQueryId *result = lfirst(lc);

		/*
		 * store this entries to hash pgss_hash
		 * and relation sql_firewall_rec.
		 */
		add_list_rule(userid, result->queryid, result->query,
					  result->original_query,
					  statement_type, 1);
	}

	/* free list queryids. */
	sql_firewall_free_queryids(queryids);

	return true;
}

/*
 * read_sql_file_internal
 *
 * read all sql statement from specified file.
 */
static void
read_sql_file_internal(FILE *filep, const char *filepath, StringInfo contents)
{
	char		buffer[8192];
	size_t		nread;

	/* Read all sql statement from specified file. */
	while ((nread = fread(buffer, 1, sizeof(buffer), filep)) > 0)
		appendBinaryStringInfo(contents, buffer, nread);

	/* Check for errors while reading the file. */
	if (ferror(filep))
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not read file \"%s\": %m", filepath)));
}

/*
 * read_sql_file
 *
 * Read one complete SQL file; statement boundaries
 * belong to the parser.
 */
static char *
read_sql_file(const char *filepath)
{
	FILE	   *filep = NULL;
	StringInfoData contents;
	struct stat st;

	/* Check if a file exists. */
	if (stat(filepath, &st) != 0)
		ereport(ERROR,
				(errmsg("could not stat file \"%s\": %m", filepath)));

	/*
	 * Determine whether the file is
	 * a standard text file.
	 */
	if (!S_ISREG(st.st_mode))
		ereport(ERROR,
				(errmsg("\"%s\" is not a regular file", filepath)));

	initStringInfo(&contents);

	/* Open the file, and read all sql statement. */
	filep = AllocateFile(filepath, PG_BINARY_R);
	if (filep == NULL)
		ereport(ERROR,
				(errmsg("could not open file \"%s\": %m", filepath)));

	PG_TRY();
	{
		/* Read all sql statement. */
		read_sql_file_internal(filep, filepath, &contents);
		if (FreeFile(filep))
			ereport(ERROR,
					(errcode_for_file_access(),
					 errmsg("could not close file \"%s\": %m", filepath)));
		filep = NULL;
	}
	PG_CATCH();
	{
		/*
		 * If read file failure. close the
		 * file and free data.
		 */
		if (filep != NULL)
			FreeFile(filep);
		pfree(contents.data);
		PG_RE_THROW();
	}
	PG_END_TRY();

	return contents.data;
}

/*
 * import_list_file
 *
 * batch black list or white list. there read sql file, and
 * add the rule to hash and relation.
 */
static bool
import_list_file(Oid userid, const char *filepath, int statement_type)
{
	char	   *sqltext;
	List	   *queryids;
	ListCell   *lc;

	/* Check if the hash has been loaded. */
	check_sql_firewall_loaded();

	/*
	 * Compute every query identifier before changing the rule store.  Thus a
	 * syntax or analysis error anywhere in the file prevents the batch from
	 * being applied, without requiring a separate temporary hash table.
	 */
	sqltext = read_sql_file(filepath);
	require_list_sql_terminator(sqltext);

	/* Don't allow import prepare statement. */
	reject_prepare_in_list_import(sqltext);

	/* Calculate the SQL information to be imported. */
	queryids = sql_firewall_compute_queryid(sqltext, true);
	pfree(sqltext);

	/*
	 * Import data from the list queryids
	 * one by one into the hash and table.
	 */
	foreach(lc, queryids)
	{
		SqlFirewallQueryId *result = lfirst(lc);

		add_list_rule(userid, result->queryid, result->query,
					  result->original_query, statement_type, 1);
	}

	/* free queryids. */
	sql_firewall_free_queryids(queryids);

	return true;
}

/*
 * export_list_file
 *
 * export the black list or white list to file,the file store
 * the data formate is csv.
 */
static bool
export_list_file(const char *filepath, Oid userid, bool filter_user,
				 int statement_type)
{
	FILE	   *filep;
	List	   *rules;
	ListCell   *lc;

	/* Check if the hash has been loaded. */
	check_sql_firewall_loaded();

	/* Open the file. */
	filep = AllocateFile(filepath, PG_BINARY_W);
	if (filep == NULL)
		ereport(ERROR,
				(errmsg("could not open file \"%s\": %m", filepath)));

	/* get relative rules. */
	rules = load_persisted_rules();

	/* Write the rules into the file one by one. */
	foreach(lc, rules)
	{
		SqlFirewallPersistedRule *rule = lfirst(lc);

		if (statement_type != SQL_FIREWALL_STATEMENT_ALL &&
			rule->statement_type != statement_type)
			continue;
		if (filter_user && rule->userid != userid)
			continue;

		/*
		 * Record format: userid, queryid, query, original_query,
		 * statement_type, calls, warning_count, error_count.
		 */
		fprintf(filep, "%u," UINT64_FORMAT ",", rule->userid,
				rule->queryid);
		write_csv_field(filep, rule->query);
		fputc(',', filep);
		write_csv_field(filep, rule->original_query);
		fprintf(filep, ",%s," INT64_FORMAT "," INT64_FORMAT "," INT64_FORMAT "\n",
				statement_type_name(rule->statement_type), rule->calls,
				rule->warning_count, rule->error_count);
	}

	/* Release the list rules. */
	free_persisted_rules(rules);

	/* Close the file. */
	if (FreeFile(filep))
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not close file \"%s\": %m", filepath)));

	return true;
}

/*
 * export_list_sql_file
 *
 * Export saved original SQL text,
 * one complete statement after another.
 */
static bool
export_list_sql_file(const char *filepath, Oid userid, bool filter_user,
					 int statement_type)
{
	FILE	   *filep;
	List	   *rules;
	ListCell   *lc;

	/* Check if the hash has been loaded. */
	check_sql_firewall_loaded();

	/* Open the file. */
	filep = AllocateFile(filepath, PG_BINARY_W);
	if (filep == NULL)
		ereport(ERROR,
				(errmsg("could not open file \"%s\": %m", filepath)));

	/* get relative rules. */
	rules = load_persisted_rules();

	/* Write the original sql into the file one by one. */
	foreach(lc, rules)
	{
		SqlFirewallPersistedRule *rule = lfirst(lc);
		int			original_query_len;

		if (statement_type != SQL_FIREWALL_STATEMENT_ALL &&
			rule->statement_type != statement_type)
			continue;
		if (filter_user && rule->userid != userid)
			continue;

		/* the record format is:riginal_query\n */
		original_query_len = strlen(rule->original_query);
		if (fwrite(rule->original_query, 1, original_query_len, filep) !=
			original_query_len)
		{
			free_persisted_rules(rules);
			FreeFile(filep);
			ereport(ERROR,
					(errcode_for_file_access(),
					 errmsg("could not write file \"%s\": %m", filepath)));
		}
		if (original_query_len == 0 ||
			rule->original_query[original_query_len - 1] != '\n')
			fputc('\n', filep);
	}

	/* Release the list rules. */
	free_persisted_rules(rules);

	/* Close the file. */
	if (FreeFile(filep))
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not close file \"%s\": %m", filepath)));

	return true;
}

/*
 * delete_list_entries
 *
 * delete the rule from the hash pgss_hash.
 */
static bool
delete_list_entries(Oid userid, bool filter_user, uint64 queryid,
					bool filter_queryid, int statement_type)
{
	HASH_SEQ_STATUS hash_seq;
	sqlfirewallPgssEntry  *entry;
	List	   *keys = NIL;
	ListCell   *lc;

	check_sql_firewall_loaded();

	LWLockAcquire(pgss->lock, LW_EXCLUSIVE);

	/* Locate the record to be deleted
	 * in the hash and make a temporary note of it.
	 */
	hash_seq_init(&hash_seq, pgss_hash);
	while ((entry = hash_seq_search(&hash_seq)) != NULL)
	{
		sqlfirewallPgssHashKey *keycopy;

		/* Only look for those that meet the requirements. */
		if (entry->key.dbid != MyDatabaseId)
			continue;
		if (entry->statement_type != statement_type)
			continue;
		if (filter_user && entry->key.userid != userid)
			continue;
		if (filter_queryid && entry->key.queryid != queryid)
			continue;

		keycopy = palloc(sizeof(sqlfirewallPgssHashKey));
		*keycopy = entry->key;
		keys = lappend(keys, keycopy);
	}

	/* Remove from hash pgss_hash. */
	foreach(lc, keys)
	{
		sqlfirewallPgssHashKey *key = (sqlfirewallPgssHashKey *) lfirst(lc);
		hash_search(pgss_hash, key, HASH_REMOVE, NULL);
	}

	LWLockRelease(pgss->lock);

	/* Remove from hash relation sql_firewall_rec. */
	foreach(lc, keys)
	{
		sqlfirewallPgssHashKey *key = (sqlfirewallPgssHashKey *) lfirst(lc);

		delete_persisted_rule(key);
		pfree(key);
	}
	list_free(keys);

	return true;
}

/*
 * delete_list_sql
 *
 * delete the rule from the hash pgss_hash and relation sql_firewall_rec.
 */
static bool
delete_list_sql(Oid userid, bool filter_user, const char *sqltext,
				int statement_type)
{
	List	   *queryids;
	ListCell   *lc;

	require_list_sql_terminator(sqltext);

	/* Retrieve the SQL information to be deleted. */
	queryids = sql_firewall_compute_queryid(sqltext, false);

	/* Delete one by one. */
	foreach(lc, queryids)
	{
		SqlFirewallQueryId *result = lfirst(lc);

		delete_list_entries(userid, filter_user, result->queryid, true,
							statement_type);
	}

	/* release list queryids. */
	sql_firewall_free_queryids(queryids);

	return true;
}

/*
 * delete_list_file
 *
 * batch delete the rule from the hash pgss_hash.
 */
static bool
delete_list_file(Oid userid, const char *filepath, int statement_type)
{
	char	   *sqltext;
	List	   *queryids;
	ListCell   *lc;

	check_sql_firewall_loaded();

	/* Read the entire content of the file.. */
	sqltext = read_sql_file(filepath);
	require_list_sql_terminator(sqltext);

	/* Calculate information for each SQL statement. */
	queryids = sql_firewall_compute_queryid(sqltext, false);

	/* free variable. */
	pfree(sqltext);

	/* Delete records one by one. */
	foreach(lc, queryids)
	{
		SqlFirewallQueryId *result = lfirst(lc);

		delete_list_entries(userid, userid != InvalidOid, result->queryid, true,
							statement_type);
	}

	/* release list queryids. */
	sql_firewall_free_queryids(queryids);

	return true;
}

/*
 * import_white_list
 *
 * import white list for all user.
 */
Datum
import_white_list(PG_FUNCTION_ARGS)
{
	check_sql_firewall_function_permission("import_white_list");
	PG_RETURN_BOOL(import_list_sql(InvalidOid,
								   text_to_cstring(PG_GETARG_TEXT_P(0)),
								   SQL_FIREWALL_STATEMENT_WHITE));
}

/*
 * import_white_list_for_user
 *
 * import white list for specified user.
 */
Datum
import_white_list_for_user(PG_FUNCTION_ARGS)
{
	check_sql_firewall_function_permission("import_white_list_for_user");
	PG_RETURN_BOOL(import_list_sql(sql_firewall_role_name_to_oid(PG_GETARG_NAME(0)),
								   text_to_cstring(PG_GETARG_TEXT_P(1)),
								   SQL_FIREWALL_STATEMENT_WHITE));
}

/*
 * import_white_list_batch
 *
 * batch import white list for all user.
 */
Datum
import_white_list_batch(PG_FUNCTION_ARGS)
{
	check_sql_firewall_function_permission("import_white_list_batch");
	PG_RETURN_BOOL(import_list_file(InvalidOid,
									text_to_cstring(PG_GETARG_TEXT_P(0)),
									SQL_FIREWALL_STATEMENT_WHITE));
}

/*
 * import_white_list_batch_for_user
 *
 * batch import white list for specified user.
 */
Datum
import_white_list_batch_for_user(PG_FUNCTION_ARGS)
{
	check_sql_firewall_function_permission("import_white_list_batch_for_user");
	PG_RETURN_BOOL(import_list_file(sql_firewall_role_name_to_oid(PG_GETARG_NAME(0)),
									text_to_cstring(PG_GETARG_TEXT_P(1)),
									SQL_FIREWALL_STATEMENT_WHITE));
}

/*
 * export_white_list
 *
 * export all white list data to csv format file.
 */
Datum
export_white_list(PG_FUNCTION_ARGS)
{
	check_sql_firewall_function_permission("export_white_list");
	PG_RETURN_BOOL(export_list_file(text_to_cstring(PG_GETARG_TEXT_P(0)),
									InvalidOid, false,
									SQL_FIREWALL_STATEMENT_WHITE));
}

/*
 * export_white_list_for_user
 *
 * export specified user's white list data to csv format file.
 */
Datum
export_white_list_for_user(PG_FUNCTION_ARGS)
{
	check_sql_firewall_function_permission("export_white_list_for_user");
	PG_RETURN_BOOL(export_list_file(text_to_cstring(PG_GETARG_TEXT_P(1)),
									sql_firewall_role_name_to_oid(PG_GETARG_NAME(0)), true,
									SQL_FIREWALL_STATEMENT_WHITE));
}

/*
 * export_white_list_sql
 *
 * export all white list sql to file.
 */
Datum
export_white_list_sql(PG_FUNCTION_ARGS)
{
	check_sql_firewall_function_permission("export_white_list_sql");
	PG_RETURN_BOOL(export_list_sql_file(text_to_cstring(PG_GETARG_TEXT_P(0)),
									  InvalidOid, false,
									  SQL_FIREWALL_STATEMENT_WHITE));
}

/*
 * export_white_list_sql_for_user
 *
 * export specified user's white list sql to file.
 */
Datum
export_white_list_sql_for_user(PG_FUNCTION_ARGS)
{
	check_sql_firewall_function_permission("export_white_list_sql_for_user");
	PG_RETURN_BOOL(export_list_sql_file(text_to_cstring(PG_GETARG_TEXT_P(1)),
									  sql_firewall_role_name_to_oid(PG_GETARG_NAME(0)), true,
									  SQL_FIREWALL_STATEMENT_WHITE));
}

/*
 * delete_all_white_list
 *
 * delete all white list from hash and table.
 */
Datum
delete_all_white_list(PG_FUNCTION_ARGS)
{
	check_sql_firewall_function_permission("delete_white_list");
	PG_RETURN_BOOL(delete_list_entries(InvalidOid, false, 0, false,
									   SQL_FIREWALL_STATEMENT_WHITE));
}

/*
 * delete_all_white_list_for_user
 *
 * delete specified user's white list from hash and table.
 */
Datum
delete_all_white_list_for_user(PG_FUNCTION_ARGS)
{
	check_sql_firewall_function_permission("delete_all_white_list_for_user");
	PG_RETURN_BOOL(delete_list_entries(sql_firewall_role_name_to_oid(PG_GETARG_NAME(0)),
									 true, 0, false,
									   SQL_FIREWALL_STATEMENT_WHITE));
}

/*
 * delete_white_list
 *
 * delete white list according to sql statement.
 */
Datum
delete_white_list(PG_FUNCTION_ARGS)
{
	check_sql_firewall_function_permission("delete_white_list");
	PG_RETURN_BOOL(delete_list_sql(InvalidOid, false,
								   text_to_cstring(PG_GETARG_TEXT_P(0)),
								   SQL_FIREWALL_STATEMENT_WHITE));
}

/*
 * delete_white_list_for_user
 *
 * delete specified user's white list according to sql statement.
 */
Datum
delete_white_list_for_user(PG_FUNCTION_ARGS)
{
	check_sql_firewall_function_permission("delete_white_list_for_user");
	PG_RETURN_BOOL(delete_list_sql(sql_firewall_role_name_to_oid(PG_GETARG_NAME(0)), true,
								   text_to_cstring(PG_GETARG_TEXT_P(1)),
								   SQL_FIREWALL_STATEMENT_WHITE));
}

/*
 * delete_white_list_batch
 *
 * batch delete white list according to the file.
 */
Datum
delete_white_list_batch(PG_FUNCTION_ARGS)
{
	check_sql_firewall_function_permission("delete_white_list_batch");
	PG_RETURN_BOOL(delete_list_file(InvalidOid,
									text_to_cstring(PG_GETARG_TEXT_P(0)),
									SQL_FIREWALL_STATEMENT_WHITE));
}

/*
 * delete_white_list_batch_for_user
 *
 * batch delete specified user's white list according to the file.
 */
Datum
delete_white_list_batch_for_user(PG_FUNCTION_ARGS)
{
	check_sql_firewall_function_permission("delete_white_list_batch_for_user");
	PG_RETURN_BOOL(delete_list_file(sql_firewall_role_name_to_oid(PG_GETARG_NAME(0)),
									text_to_cstring(PG_GETARG_TEXT_P(1)),
									SQL_FIREWALL_STATEMENT_WHITE));
}

/*
 * import_black_list
 *
 * import black list for all user.
 */
Datum
import_black_list(PG_FUNCTION_ARGS)
{
	check_sql_firewall_function_permission("import_black_list");
	PG_RETURN_BOOL(import_list_sql(InvalidOid,
								   text_to_cstring(PG_GETARG_TEXT_P(0)),
								   SQL_FIREWALL_STATEMENT_BLACK));
}

/*
 * import_black_list_for_user
 *
 * import black list for specified user.
 */
Datum
import_black_list_for_user(PG_FUNCTION_ARGS)
{
	check_sql_firewall_function_permission("import_black_list_for_user");
	PG_RETURN_BOOL(import_list_sql(sql_firewall_role_name_to_oid(PG_GETARG_NAME(0)),
								   text_to_cstring(PG_GETARG_TEXT_P(1)),
								   SQL_FIREWALL_STATEMENT_BLACK));
}

/*
 * import_black_list_batch
 *
 * batch import white list for all user.
 */
Datum
import_black_list_batch(PG_FUNCTION_ARGS)
{
	check_sql_firewall_function_permission("import_black_list_batch");
	PG_RETURN_BOOL(import_list_file(InvalidOid,
									text_to_cstring(PG_GETARG_TEXT_P(0)),
									SQL_FIREWALL_STATEMENT_BLACK));
}

/*
 * import_black_list_batch_for_user
 *
 * batch import black list for specified user.
 */
Datum
import_black_list_batch_for_user(PG_FUNCTION_ARGS)
{
	check_sql_firewall_function_permission("import_black_list_batch_for_user");
	PG_RETURN_BOOL(import_list_file(sql_firewall_role_name_to_oid(PG_GETARG_NAME(0)),
									text_to_cstring(PG_GETARG_TEXT_P(1)),
									SQL_FIREWALL_STATEMENT_BLACK));
}

/*
 * export_black_list
 *
 * export all black list data to csv format file.
 */
Datum
export_black_list(PG_FUNCTION_ARGS)
{
	check_sql_firewall_function_permission("export_black_list");
	PG_RETURN_BOOL(export_list_file(text_to_cstring(PG_GETARG_TEXT_P(0)),
									InvalidOid, false,
									SQL_FIREWALL_STATEMENT_BLACK));
}

/*
 * export_black_list_for_user
 *
 * export specified user's black list data to csv format file.
 */
Datum
export_black_list_for_user(PG_FUNCTION_ARGS)
{
	check_sql_firewall_function_permission("export_black_list_for_user");
	PG_RETURN_BOOL(export_list_file(text_to_cstring(PG_GETARG_TEXT_P(1)),
									sql_firewall_role_name_to_oid(PG_GETARG_NAME(0)), true,
									SQL_FIREWALL_STATEMENT_BLACK));
}

/*
 * export_black_list_sql
 *
 * export all black list sql to file.
 */
Datum
export_black_list_sql(PG_FUNCTION_ARGS)
{
	check_sql_firewall_function_permission("export_black_list_sql");
	PG_RETURN_BOOL(export_list_sql_file(text_to_cstring(PG_GETARG_TEXT_P(0)),
									  InvalidOid, false,
									  SQL_FIREWALL_STATEMENT_BLACK));
}

/*
 * export_black_list_sql_for_user
 *
 * export specified user's black list sql to file.
 */
Datum
export_black_list_sql_for_user(PG_FUNCTION_ARGS)
{
	check_sql_firewall_function_permission("export_black_list_sql_for_user");
	PG_RETURN_BOOL(export_list_sql_file(text_to_cstring(PG_GETARG_TEXT_P(1)),
									  sql_firewall_role_name_to_oid(PG_GETARG_NAME(0)), true,
									  SQL_FIREWALL_STATEMENT_BLACK));
}

/*
 * export_all_sql
 *
 * export all rules sql statement.
 */
Datum
export_all_sql(PG_FUNCTION_ARGS)
{
	check_sql_firewall_function_permission("export_all_sql");
	PG_RETURN_BOOL(export_list_sql_file(text_to_cstring(PG_GETARG_TEXT_P(0)),
									  InvalidOid, false,
									  SQL_FIREWALL_STATEMENT_ALL));
}

/* Return all original SQL texts identified by a query ID. */
/*
 * dispaly_single_sql
 *
 * dispaly single sql statement accroding to query id.
 */
Datum
dispaly_single_sql(PG_FUNCTION_ARGS)
{
	int64		requested_queryid = PG_GETARG_INT64(0);
	uint64		queryid = (uint64) requested_queryid;
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
	MemoryContext per_query_ctx;
	MemoryContext oldcontext;
	TupleDesc	tupdesc;
	Tuplestorestate *tupstore;
	Oid			userid = GetUserId();
	bool		is_superuser = superuser();
	List	   *rules;
	ListCell   *lc;
	bool		found = false;

	/* Permission check. */
	check_sql_firewall_function_permission("dispaly_single_sql");

	check_sql_firewall_loaded();

	if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo) ||
		!(rsinfo->allowedModes & SFRM_Materialize))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("set-valued function called in context that cannot accept a set")));

	per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
	oldcontext = MemoryContextSwitchTo(per_query_ctx);
	tupdesc = CreateTemplateTupleDesc(1);
	TupleDescInitEntry(tupdesc, (AttrNumber) 1, "dispaly_single_sql",
					   TEXTOID, -1, 0);
	tupdesc = BlessTupleDesc(tupdesc);
	tupstore = tuplestore_begin_heap(true, false, work_mem);
	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = tupstore;
	rsinfo->setDesc = tupdesc;
	MemoryContextSwitchTo(oldcontext);

	/* Search across all records. */
	rules = load_persisted_rules();
	foreach(lc, rules)
	{
		SqlFirewallPersistedRule *rule = lfirst(lc);
		char	   *server_query;
		Datum		value;
		bool		isnull = false;

		if (rule->queryid != queryid)
			continue;
		if (!is_superuser && rule->userid != InvalidOid &&
			rule->userid != userid)
			continue;

		/* Once found, record the original SQL statement. */
		server_query = pg_any_to_server(rule->original_query,
									strlen(rule->original_query),
									rule->encoding);
		value = CStringGetTextDatum(server_query);
		tuplestore_putvalues(tupstore, tupdesc, &value, &isnull);

		/* Set the flag as soon as it is found once. */
		found = true;
		if (server_query != rule->original_query)
			pfree(server_query);
	}

	/* Release the list rules. */
	free_persisted_rules(rules);

	/* Return the record if found. */
	if (!found)
	{
		Datum		value = (Datum) 0;
		bool		isnull = true;

		ereport(WARNING,
				(errmsg("query ID " INT64_FORMAT " has no record",
						requested_queryid)));
		tuplestore_putvalues(tupstore, tupdesc, &value, &isnull);
	}

	tuplestore_donestoring(tupstore);
	PG_RETURN_NULL();
}

/*
 * import_high_risk_statement
 *
 * import the type of high risk statement.
 */
Datum
import_high_risk_statement(PG_FUNCTION_ARGS)
{
	char	   *name = text_to_cstring(PG_GETARG_TEXT_P(0));
	HighRiskStatementKind kind = high_risk_kind_from_name(name);
	Oid			relid;
	Oid			indexid;
	Relation	rel;
	SysScanDesc scan;
	ScanKeyData key;
	HeapTuple	oldtuple;
	HeapTuple	newtuple;
	Datum		values[Natts_sql_high_risk_rec];
	bool		nulls[Natts_sql_high_risk_rec];
	bool		replaces[Natts_sql_high_risk_rec];
	Datum		statement;

	/* Permission check. */
	check_sql_firewall_function_permission("import_high_risk_statement");

	/* Initialize variables. */
	MemSet(values, 0, sizeof(values));
	MemSet(nulls, false, sizeof(nulls));
	MemSet(replaces, false, sizeof(replaces));

	if (kind == HIGH_RISK_NONE)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("unknown high-risk statement category: %s", name)));

	/* Resolve both objects before opening the table. */
	get_high_risk_relation_oids(&relid, &indexid);
	rel = table_open(relid, RowExclusiveLock);
	statement = CStringGetTextDatum(high_risk_kind_name(kind));
	ScanKeyInit(&key,
				(AttrNumber) 1,
				BTEqualStrategyNumber, F_TEXTEQ,
				statement);
	scan = systable_beginscan(rel, indexid, true, NULL, 1, &key);
	oldtuple = systable_getnext(scan);
	if (HeapTupleIsValid(oldtuple))
	{
		values[1] = CStringGetTextDatum(high_risk_kind_description(kind));
		replaces[1] = true;
		newtuple = heap_modify_tuple(oldtuple, RelationGetDescr(rel),
								 values, nulls, replaces);
		CatalogTupleUpdate(rel, &oldtuple->t_self, newtuple);
	}
	else
	{
		values[0] = statement;
		values[1] = CStringGetTextDatum(high_risk_kind_description(kind));
		newtuple = heap_form_tuple(RelationGetDescr(rel), values, nulls);
		CatalogTupleInsert(rel, newtuple);
	}
	heap_freetuple(newtuple);
	systable_endscan(scan);
	table_close(rel, RowExclusiveLock);

	set_high_risk_hash(kind, 0, 0);

	PG_RETURN_BOOL(true);
}

/*
 * remove_high_risk_statement
 *
 * remove the type of high risk statement.
 */
Datum
remove_high_risk_statement(PG_FUNCTION_ARGS)
{
	char	   *name = text_to_cstring(PG_GETARG_TEXT_P(0));
	HighRiskStatementKind kind = high_risk_kind_from_name(name);
	Oid			relid;
	Oid			indexid;
	Relation	rel;
	SysScanDesc scan;
	ScanKeyData key;
	HeapTuple	tuple;
	Datum		statement;

	check_sql_firewall_function_permission("remove_high_risk_statement");
	if (kind == HIGH_RISK_NONE)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("unknown high-risk statement category: %s", name)));

	/* Resolve both objects before opening the table. */
	get_high_risk_relation_oids(&relid, &indexid);
	rel = table_open(relid, RowExclusiveLock);
	statement = CStringGetTextDatum(high_risk_kind_name(kind));
	ScanKeyInit(&key,
				(AttrNumber) 1,
				BTEqualStrategyNumber, F_TEXTEQ,
				statement);
	scan = systable_beginscan(rel, indexid, true, NULL, 1, &key);
	tuple = systable_getnext(scan);

	/*
	 * If the record is found,
	 * delete it from the table.
	 */
	if (HeapTupleIsValid(tuple))
	{
		CatalogTupleDelete(rel, &tuple->t_self);
	}
	systable_endscan(scan);
	table_close(rel, RowExclusiveLock);

	/* Delete a record from the hash. */
	remove_high_risk_hash(kind);

	PG_RETURN_BOOL(true);
}

/*
 * delete_all_black_list
 *
 * delete all black list from hash and table.
 */
Datum
delete_all_black_list(PG_FUNCTION_ARGS)
{
	check_sql_firewall_function_permission("delete_all_black_list");
	PG_RETURN_BOOL(delete_list_entries(InvalidOid, false, 0, false,
									   SQL_FIREWALL_STATEMENT_BLACK));
}

/*
 * delete_all_black_list_for_user
 *
 *  delete all black list for specified user.
 */
Datum
delete_all_black_list_for_user(PG_FUNCTION_ARGS)
{
	check_sql_firewall_function_permission("delete_all_black_list_for_user");
	PG_RETURN_BOOL(delete_list_entries(sql_firewall_role_name_to_oid(PG_GETARG_NAME(0)),
									 true, 0, false,
									   SQL_FIREWALL_STATEMENT_BLACK));
}

/*
 * delete_black_list
 *
 * delete a record form black list.
 */
Datum
delete_black_list(PG_FUNCTION_ARGS)
{
	check_sql_firewall_function_permission("delete_black_list");
	PG_RETURN_BOOL(delete_list_sql(InvalidOid, false,
								   text_to_cstring(PG_GETARG_TEXT_P(0)),
								   SQL_FIREWALL_STATEMENT_BLACK));
}

/*
 * delete_black_list_for_user
 *
 * delete specified user's black list according to sql statement.
 */
Datum
delete_black_list_for_user(PG_FUNCTION_ARGS)
{
	check_sql_firewall_function_permission("delete_black_list_for_user");
	PG_RETURN_BOOL(delete_list_sql(sql_firewall_role_name_to_oid(PG_GETARG_NAME(0)), true,
								   text_to_cstring(PG_GETARG_TEXT_P(1)),
								   SQL_FIREWALL_STATEMENT_BLACK));
}

/*
 * delete_black_list_batch
 *
 * batch delete white list according to the file.
 */
Datum
delete_black_list_batch(PG_FUNCTION_ARGS)
{
	check_sql_firewall_function_permission("delete_black_list_batch");
	PG_RETURN_BOOL(delete_list_file(InvalidOid,
									text_to_cstring(PG_GETARG_TEXT_P(0)),
									SQL_FIREWALL_STATEMENT_BLACK));
}

/*
 * delete_black_list_batch_for_user
 *
 * batch delete specified user's white list according to the file.
 */
Datum
delete_black_list_batch_for_user(PG_FUNCTION_ARGS)
{
	check_sql_firewall_function_permission("delete_black_list_batch_for_user");
	PG_RETURN_BOOL(delete_list_file(sql_firewall_role_name_to_oid(PG_GETARG_NAME(0)),
									text_to_cstring(PG_GETARG_TEXT_P(1)),
									SQL_FIREWALL_STATEMENT_BLACK));
}

/*
 * pgss_memsize
 *
 * Estimate shared memory space needed.
 */
static Size
pgss_memsize(void)
{
	Size		size;

	size = MAXALIGN(sizeof(sqlfirewallPgssSharedState));
	size = add_size(size, hash_estimate_size(pgss_max, sizeof(sqlfirewallPgssEntry)));
	size = add_size(size, hash_estimate_size(pgss_max,
										 sizeof(HighRiskHashEntry)));
	size = add_size(size, hash_estimate_size(pgss_max,
										 sizeof(SqlFirewallStatCountHashEntry)));

	return size;
}

/*
 * entry_alloc
 *
 * Allocate a new hashtable entry.
 * caller must hold an exclusive lock on pgss->lock
 *
 * Note: despite needing exclusive lock, it's not an error for the target
 * entry to already exist.  This is because pgss_store releases and
 * reacquires lock after failing to find a match; so someone else could
 * have made the entry while we waited to get exclusive lock.
 */
static sqlfirewallPgssEntry *
entry_alloc(sqlfirewallPgssHashKey *key, int statement_type)
{
	sqlfirewallPgssEntry  *entry;
	bool		found;

	entry = (sqlfirewallPgssEntry *) hash_search(pgss_hash, key, HASH_FIND, NULL);
	if (entry != NULL)
	{
		if (entry->statement_type != statement_type)
			ereport(ERROR,
					(errcode(ERRCODE_DUPLICATE_OBJECT),
					 errmsg("SQL rule already exists in %s statement",
							statement_type_name(entry->statement_type))));
		return entry;
	}

	/* Ignore queries which exceed the max limit of the learning table. */
	if (hash_get_num_entries(pgss_hash) >= pgss_max)
	{
		ereport(WARNING,(errmsg("Number of queries exceeded the <sql_firewall.max> limit.")));
		return NULL;
	}

	/* Find or create an entry with desired hash code */
	entry = (sqlfirewallPgssEntry *) hash_search(pgss_hash, key, HASH_ENTER, &found);

	if (!found)
	{
		/* New entry, initialize it */

		/* reset the statistics */
		memset(&entry->counters, 0, sizeof(SqlFirewallCounters));
		/* re-initialize the mutex each time ... we assume no one using it */
		SpinLockInit(&entry->mutex);
		entry->statement_type = statement_type;
	}
	else if (entry->statement_type != statement_type)
		ereport(ERROR,
				(errcode(ERRCODE_DUPLICATE_OBJECT),
				 errmsg("SQL rule already exists in %s statement",
						statement_type_name(entry->statement_type))));

	return entry;
}

/*
 * entry_reset
 *
 * Release all entries.
 */
static void
entry_reset(void)
{
	HASH_SEQ_STATUS hash_seq;
	sqlfirewallPgssEntry  *entry;

	LWLockAcquire(pgss->lock, LW_EXCLUSIVE);

	hash_seq_init(&hash_seq, pgss_hash);

	/* Delete all records in the current database one by one. */
	while ((entry = hash_seq_search(&hash_seq)) != NULL)
	{
		if (entry->key.dbid == MyDatabaseId)
			hash_search(pgss_hash, &entry->key, HASH_REMOVE, NULL);
	}

	LWLockRelease(pgss->lock);
}

/*
 * generate_normalized_query
 *
 * Generate a normalized version of the query string that will be used to
 * represent all similar queries.
 *
 * Note that the normalized representation may well vary depending on
 * just which "equivalent" query is used to create the hashtable entry.
 * We assume this is OK.
 *
 * *query_len_p contains the input string length, and is updated with
 * the result string length (which cannot be longer) on exit.
 *
 * Returns a palloc'd string.
 */
static char *
generate_normalized_query(pgssJumbleState *jstate, const char *query,
						  int *query_len_p, int encoding)
{
	char	   *norm_query;
	int			query_len = *query_len_p;
	int			i,
				len_to_wrt,		/* Length (in bytes) to write */
				quer_loc = 0,	/* Source query byte location */
				n_quer_loc = 0, /* Normalized query byte location */
				last_off = 0,	/* Offset from start for previous tok */
				last_tok_len = 0;		/* Length (in bytes) of that tok */

	/*
	 * Get constants' lengths (core system only gives us locations).  Note
	 * this also ensures the items are sorted by location.
	 */
	fill_in_constant_lengths(jstate, query);

	/* Allocate result buffer */
	norm_query = palloc(query_len + 1);

	for (i = 0; i < jstate->clocations_count; i++)
	{
		int			off,		/* Offset from start for cur tok */
					tok_len;	/* Length (in bytes) of that tok */

		off = jstate->clocations[i].location;
		tok_len = jstate->clocations[i].length;

		if (tok_len < 0)
			continue;			/* ignore any duplicates */

		/* Copy next chunk (what precedes the next constant) */
		len_to_wrt = off - last_off;
		len_to_wrt -= last_tok_len;

		Assert(len_to_wrt >= 0);
		memcpy(norm_query + n_quer_loc, query + quer_loc, len_to_wrt);
		n_quer_loc += len_to_wrt;

		/* And insert a '?' in place of the constant token */
		norm_query[n_quer_loc++] = '?';

		quer_loc = off + tok_len;
		last_off = off;
		last_tok_len = tok_len;
	}

	/*
	 * We've copied up until the last ignorable constant.  Copy over the
	 * remaining bytes of the original query string.
	 */
	len_to_wrt = query_len - quer_loc;

	Assert(len_to_wrt >= 0);
	memcpy(norm_query + n_quer_loc, query + quer_loc, len_to_wrt);
	n_quer_loc += len_to_wrt;

	Assert(n_quer_loc <= query_len);
	norm_query[n_quer_loc] = '\0';

	*query_len_p = n_quer_loc;

	return norm_query;
}

/*
 * fill_in_constant_lengths
 *
 * Given a valid SQL string and an array of constant-location records,
 * fill in the textual lengths of those constants.
 *
 * The constants may use any allowed constant syntax, such as float literals,
 * bit-strings, single-quoted strings and dollar-quoted strings.  This is
 * accomplished by using the public API for the core scanner.
 *
 * It is the caller's job to ensure that the string is a valid SQL statement
 * with constants at the indicated locations.  Since in practice the string
 * has already been parsed, and the locations that the caller provides will
 * have originated from within the authoritative parser, this should not be
 * a problem.
 *
 * Duplicate constant pointers are possible, and will have their lengths
 * marked as '-1', so that they are later ignored.  (Actually, we assume the
 * lengths were initialized as -1 to start with, and don't change them here.)
 *
 * N.B. There is an assumption that a '-' character at a Const location begins
 * a negative numeric constant.  This precludes there ever being another
 * reason for a constant to start with a '-'.
 */
static void
fill_in_constant_lengths(pgssJumbleState *jstate, const char *query)
{
	pgssLocationLen *locs;
	core_yyscan_t yyscanner;
	core_yy_extra_type yyextra;
	core_YYSTYPE yylval;
	YYLTYPE		yylloc;
	int			last_loc = -1;
	int			i;

	/*
	 * Sort the records by location so that we can process them in order while
	 * scanning the query text.
	 */
	if (jstate->clocations_count > 1)
		qsort(jstate->clocations, jstate->clocations_count,
			  sizeof(pgssLocationLen), comp_location);
	locs = jstate->clocations;

	/* initialize the flex scanner --- should match raw_parser() */
	yyscanner = scanner_init(query,
							 &yyextra,
							 &ScanKeywords,
							 ScanKeywordTokens);

	/* we don't want to re-emit any escape string warnings */
	yyextra.escape_string_warning = false;

	/* Search for each constant, in sequence */
	for (i = 0; i < jstate->clocations_count; i++)
	{
		int			loc = locs[i].location;
		int			tok;

		Assert(loc >= 0);

		if (loc <= last_loc)
			continue;			/* Duplicate constant, ignore */

		/* Lex tokens until we find the desired constant */
		for (;;)
		{
			tok = core_yylex(&yylval, &yylloc, yyscanner);

			/* We should not hit end-of-string, but if we do, behave sanely */
			if (tok == 0)
				break;			/* out of inner for-loop */

			/*
			 * We should find the token position exactly, but if we somehow
			 * run past it, work with that.
			 */
			if (yylloc >= loc)
			{
				if (query[loc] == '-')
				{
					/*
					 * It's a negative value - this is the one and only case
					 * where we replace more than a single token.
					 *
					 * Do not compensate for the core system's special-case
					 * adjustment of location to that of the leading '-'
					 * operator in the event of a negative constant.  It is
					 * also useful for our purposes to start from the minus
					 * symbol.  In this way, queries like "select * from foo
					 * where bar = 1" and "select * from foo where bar = -2"
					 * will have identical normalized query strings.
					 */
					tok = core_yylex(&yylval, &yylloc, yyscanner);
					if (tok == 0)
						break;	/* out of inner for-loop */
				}

				/*
				 * We now rely on the assumption that flex has placed a zero
				 * byte after the text of the current token in scanbuf.
				 */
				locs[i].length = strlen(yyextra.scanbuf + loc);
				break;			/* out of inner for-loop */
			}
		}

		/* If we hit end-of-string, give up, leaving remaining lengths -1 */
		if (tok == 0)
			break;

		last_loc = loc;
	}

	scanner_finish(yyscanner);
}

/*
 * comp_location
 *
 * comparator for qsorting pgssLocationLen structs by location.
 */
static int
comp_location(const void *a, const void *b)
{
	int			l = ((const pgssLocationLen *) a)->location;
	int			r = ((const pgssLocationLen *) b)->location;

	if (l < r)
		return -1;
	else if (l > r)
		return +1;
	else
		return 0;
}

/*
 * check_sql_firewall_function_permission
 *
 * Permission check.
 */
static void
check_sql_firewall_relation_permission(const char *relname)
{
	/*When the xxxx is enabled, only the syssso user group can use function `xx`.*/
	if (se_feature_mode)
	{
		if (!IS_SYSSSO(GetUserId()))
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					 errmsg("When the se_feature_mode are enabled, only the syssso user group can access %s.", relname)));
		}
	}
	else
	{
		if (!superuser())
		{
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					 errmsg("When the se_feature_mode are disabled, only the superuser can access %s.", relname)));
		}
	}
}

/*
 * Exclude_internal_process
 *
 * Exclude specified processes from restrictions.
 */
static bool
Exclude_internal_process(const char *str)
{
	char *credchecklogindisplay =
		"DO $$\n"
		"BEGIN\n"
		"  PERFORM hg_security.login_message_display();\n"
		"EXCEPTION WHEN undefined_function THEN\n"
		"  \n"
		"  NULL;\n"
		"END;\n"
		"$$;\n";

	char *credchecklogindisplayexec =
		"SELECT hg_security.login_message_display()";

	char *credcheckaccountexpire =
		"DO $$\n" \
		"DECLARE\n" \
		"	user_validity TEXT DEFAULT NULL;\n" \
		"BEGIN\n" \
		"	SELECT (hg_security.user_expiration_info('%s')).user_validity INTO user_validity;\n" \
		"	IF user_validity IS NOT NULL THEN\n" \
		"		RAISE NOTICE 'Your account will expire in: %%', user_validity;\n" \
		"	END IF;\n" \
		"EXCEPTION\n" \
		"	WHEN OTHERS THEN\n" \
		"		NULL;\n" \
		"END $$;";
	char *credcheckaccountexpirestr = NULL;

	char *credcheckaccountexpireexec =
		"SELECT (hg_security.user_expiration_info('%s')).user_validity";
	char *credcheckaccountexpireexecstr =  NULL;
	char *username =  NULL;

	char *credcheckgrace_query =
		"DO $$ " \
		"DECLARE msg text; " \
		"BEGIN " \
		"  BEGIN " \
		"    msg := hg_security.pg_password_grace_notice(); " \
		"  EXCEPTION WHEN undefined_function THEN " \
		"    msg := NULL; " \
		"  END; " \
		"  IF msg IS NOT NULL THEN " \
		"    RAISE NOTICE '%', msg; " \
		"  END IF; " \
		"END $$";

	char *showcompatible_mode =
		"show ivorysql.compatible_mode;";

	if (MyBgworkerEntry != NULL)
	{
		if (strlen(MyBgworkerEntry->bgw_name) > 0)
		{
			if (strncmp(MyBgworkerEntry->bgw_name, AUDIT_LAUNCHER_S, strlen(AUDIT_LAUNCHER_S)) == 0)
			{
				return true;
			}
		}
	}

	if (SPI_get_connected() >= 0)
	{
		return true;
	}

	username = GetUserNameFromId(GetUserId(), true);
	credcheckaccountexpirestr = psprintf(credcheckaccountexpire, username);
	credcheckaccountexpireexecstr = psprintf(credcheckaccountexpireexec, username);

	if (str != NULL &&
		(!strcmp(str, credchecklogindisplay) ||
		(!strcmp(str, credcheckaccountexpirestr)) ||
		(!strcmp(str, credcheckaccountexpireexecstr)) ||
		(!strcmp(str, credcheckgrace_query)) ||
		(!strcmp(str, showcompatible_mode)) ||
		(!strcmp(str, credchecklogindisplayexec)) ||
		(!strcmp(str, showcompatible_mode)) ||
		(!strcmp(str, "NULL"))))
	{
		return true;
	}

	return false;
}
