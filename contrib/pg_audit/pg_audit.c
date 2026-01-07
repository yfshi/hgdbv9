/* -------------------------------------------------------------------------
 *
 * File: pg_audit.c
 *
 * Abstract:
 * This file is the specific implementation of security audit.
 *
 * Authored by wanghaoyan@highgo.com, 20240327
 *
 * Copyright:
 * 	Copyright (c) 2009-2025, HighGo Software Co.,Ltd. All right reserved
 *
 * Identification: 
 *	contrib/pg_audit/pg_audit.c
 *
 * -------------------------------------------------------------------------
 */
#include "postgres.h"
#include "pg_audit.h"
#include <assert.h>
#ifdef USE_SQLITE
#include <sqlite3.h>
#endif
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>

#include <math.h>
#include <dirent.h>

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#endif
#ifndef WIN32
#include <net/if.h>
#include <net/if_arp.h>
#include <sys/ioctl.h>
#include <sys/vfs.h>
#endif
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/statvfs.h>
#endif

#include "access/xlog.h"
#include "access/xlog_internal.h"
#include "access/htup_details.h"
#include "access/sysattr.h"
#include "access/xact.h"
#include "catalog/catalog.h"
#include "catalog/objectaccess.h"
#include "catalog/pg_attribute.h"
#include "catalog/pg_class.h"
#include "catalog/pg_database.h"
#include "catalog/pg_sequence.h"
#include "catalog/namespace.h"
#include "catalog/pg_namespace_d.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_auth_members.h"
#include "catalog/pg_audit_config.h"
#include "catalog/pg_secmon_rule.h"
#include "catalog/pg_secmon_threshold.h"
#include "catalog/indexing.h"
#include "access/table.h"
#include "access/relation.h"
#include "commands/event_trigger.h"
#include "commands/dbcommands.h"
#include "executor/executor.h"
#include "executor/spi.h"
#include "libpq/auth.h"
#include "libpq/hba.h"
#include "libpq/pqsignal.h"
#include "nodes/nodes.h"
#include "nodes/params.h"
//#include "miscadmin.h"
#include "pgstat.h"
#include "postmaster/bgwriter.h"
#include "postmaster/postmaster.h"
#include "replication/syncrep.h"
#include "tcop/utility.h"
#include "tcop/deparse_utility.h"
#include "storage/bufmgr.h"
#include "storage/condition_variable.h"
#include "storage/fd.h"
#include "storage/ipc.h"
#include "storage/lwlock.h"
#include "storage/proc.h"
#include "storage/shm_toc.h"
#include "storage/shm_mq.h"
#include "storage/shmem.h"
#include "storage/smgr.h"
#include "storage/spin.h"
#include "storage/procsignal.h"
#include "utils/guc.h"
#include "utils/acl.h"
#include "utils/date.h"
#include "utils/builtins.h"
#include "utils/memutils.h"
#include "utils/resowner.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/rel.h"
#include "utils/relcache.h"
#include "utils/syscache.h"
#include "utils/timestamp.h"
#include "utils/varlena.h"
#include "storage/pmsignal.h"
#include "commands/sequence.h"
#include "utils/catcache.h"
#include "commands/extension.h"
#include "common/ip.h"
#include "catalog/heap.h"
#include "utils/inet.h"
#include "access/heapam.h"
#include "catalog/pg_type.h"
#include "funcapi.h"
#include "commands/tablespace.h"
#include "commands/defrem.h"
#include "nodes/makefuncs.h"
#include "access/genam.h"
#include "nodes/parsenodes.h"
#include "postmaster/bgworker.h"
#include "postmaster/bgworker_internals.h"
#include "common/file_perm.h"
#include "utils/fmgroids.h"
#include "access/genam.h"
#include "postgres_ext.h"
#include "port.h"
#include "nodes/value.h"
#include "pg_config.h"
#include "storage/ipc.h"
#include "pg_contrib_audit.h"
#include "foreign/foreign.h"
#include "catalog/pg_foreign_server.h"
#include "se_features_interface.h"
#include "catalog/pg_authid.h"


#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

static const struct config_enum_entry audit_policy_options[] = {
	{"pause", AUDIT_PAUSE, false},
#if defined(USE_SQLITE) && defined(ENABLE_DKY_BETA_FEATURE)
	{"cycle", AUDIT_CYCLE, false},
#endif
	{NULL, 0, false}
};

static const struct config_enum_entry audit_turn_options[] = {
	{"off", AUDIT_OFF, false},
	{"on", AUDIT_ON, false},
	{NULL, 0, false}
};

static const struct config_enum_entry audit_retion_options[] = {
	{"time", RETEN_TIME, false},
	{"space", RETEN_SPACE, false},
	{NULL, 0, false}
};

static const struct config_enum_entry audit_opers_options[] = {
	{"off", AUDIT_OPER_OFF, false},
	{"succ", AUDIT_OPER_SUCC, false},
	{"fail", AUDIT_OPER_FAIL, false},
	{"all", AUDIT_OPER_ALL, false},
	{NULL, 0, false}
};

#define Int2VectorSize(n)	(offsetof(int2vector, values) + (n) * sizeof(int16))
#define OidVectorSize(n)	(offsetof(oidvector, values) + (n) * sizeof(Oid))


AuditSwitch pg_audit = AUDIT_OFF;


AduitSwitchGUC audit_GUC;
int audit_spacelimit = 10240; //G
char *audit_alarmemail = NULL;
char *audit_space_alarm = NULL;

/*
 * Role with sysdba permissions
 */
#define SYS_SYSDBA		"sysdba_role"
/*
 * Role with syssso permissions
 */
#define SYS_SYSSSO		"syssso_role"
/*
 * Role with syssao permissions
 */
#define SYS_SYSSAO		"syssao_role"
/* common user */
#define SYS_COMMON		"common"

#define CACHE_SPACE 8192

#define IsSpace(c)		(isspace((unsigned char)(c)))

#define AUDIT_LOG_NATTS 27
#define SECMON_RULE_NATTS 7
#define SECMON_THR_NATTS 18

#define CHECK_LIMIT_TIMES 200

/* Begin - BUG#Z6092 */
typedef enum AuditParameter
{
        AUDIT_ENABLE,
        AUDIT_RENTENTIONPOLICY,
        AUDIT_RENTENTIONIDAYS,
        AUDIT_DETECTIONTIME,
        AUDIT_SPACELIMIT,
        AUDIT_SPACEALARM,
        AUDIT_ARCHDIR,
        AUDIT_REMOTE_ARCHDIR,
        AUDIT_MONITORING,
        AUDIT_MAIL,
        AUDIT_PRIVIMODE,
        AUDIT_SYSTEMOPERS,
#ifdef USE_SQLITE
        AUDIT_SQLITEDBPATH,
#endif
        AUDIT_TRACE_SETTINGS,
        AUDIT_LOGPOLICY,
        NUM_AUDIT_PARAMS
} AuditParameter;
/* End - BUG#Z6092 */

/* audit using independent memory context */
static MemoryContext AuditContext = NULL;
/* auditEventStack head */
AuditEventStackItem *auditEventStack = NULL; 

/* get connection related information */
ClientInfo clientInfo;

static bool internalStatement = false;
static char	   *audit_refname = NULL;
static List *auditedrole_syssao;
static List *auditedrole_syssso;

/* list of audit events in the current transaction */
List   *auditlist = NIL;
List   *rulelist = NIL;

static int MAX_SECMON_SEQSIZE = 100;
static MemoryContext SecmonCacheMemoryContext = NULL;
static SecmonHisCache CachedSecmonHistory; //double-linked list 

static TimestampTz operstarttime;

static ExecutorStart_hook_type next_ExecutorStart_hook = NULL;
static ProcessUtility_hook_type next_ProcessUtility_hook = NULL;
static ExecutorCheckPerms_hook_type next_ExecutorCheckPerms_hook = NULL;
static object_access_hook_type next_object_access_hook = NULL;
static shmem_startup_hook_type prev_shmem_startup_hook = NULL;


static MemoryContext pgAuditLocalContext = NULL;
MemoryContext	old_context;

/* flags set by signal handlers */
static volatile sig_atomic_t got_sighup = false;
static volatile sig_atomic_t got_sigterm = false;
static int	worker_naptime = 10;


static int audit_check_dir(const char *dir);
static int audit_mkdir_p(char *path, int omode);
static int audit_mkdirs(char const *argname);
static bool hide_password(char * query_string);
static void audit_set_server_option(char *gucname, char *gucvalue);
static void audit_create_arch_job(List *parsetreeList);
static void audit_alter_set_guc(List *parsetreeList);
static void audit_register_br(void);
static void audit_define_GUC(void);
static void audit_change_GUC(void);
static void audit_append_hook(void);
static bool audit_real_check_hook(char **newvalue, void **extra, GucSource source);
static bool audit_logdir_check_hook(char **newvalue, void **extra, GucSource source);
#ifdef USE_SQLITE
static bool audit_sqlite_check_hook(char **newvalue, void **extra, GucSource source);
static void audit_sqlite_assign_hook(const char *newvalue, void *extra);
#endif
static void check_audit_sao_permission(Oid roid, bool istable);
static void check_audit_saosso_perm(Oid roid, bool istable);
static char *AuditResultToString(AuditResult res);
static char *AuditTypeToString(AuditType type);
static ObjectType audit_get_objtype(char *key);
static char *audit_get_objname(ObjectType type);
static int add_delete_audit_stmt_arg1(char *statments, char *usrnames,  Oid auditoroid, Oid classoid, char *objtype,
										AuditType audittype, AuditResult isSuccess, AuditChange enable_unable);
static List *create_statments_list(AuditType audittype, char *statments, bool is_obj, ObjectType objtype);
static bool trim_space(char *words, unsigned int len);
static int append_word(const char buffer[], char *word);
static const char *get_next_token(const char *src, char *dst);
static char *trim(char *s);
static const char *skip_space(const char *str);
static AuditStmtType audit_get_stmt_type(char *statment, bool is_obj, ObjectType objtype);
static AuditStmtType audit_get_privi_type(char *statment);
static List *get_userid_list(char *usernames);
static RoleSpec *makeRoleSpec(RoleSpecType type, int location);
static void audit_check_pg_track_settings(Node *parsetree);
static char * audit_stmttype_to_string(AuditStmtType type);
static List *audit_get_options_on_statement(AuditType audittype, List *optionlist);
static int audit_check_mode(char *audit_result_str, AuditResult *isSuccess);
static void audit_insert_stmt_tuple(Relation pg_audit, Name auditevent, Oid auditoroid, Name object, Oid classoid,
									Name objtype, AuditType audittype, AuditResult mode);
static void audit_deal_with_item(Relation pg_audit, AuditStmtType auditevent, Oid auditoroid, char *object, Oid classoid, 
							char *objtype, AuditType audittype, AuditResult auditmode, AuditChange stmtflg);
static void disable_stmt_tuple_user0(Relation pg_audit, AuditStmtType auditevent, AuditResult auditmode);
static AuditResult calculate_audit_mode(AuditResult old_result, AuditResult new_result, AuditChange old_enable, 
													AuditChange new_enable, AuditChange *final_enable);
static int add_delete_audit_stmt_args2(PG_FUNCTION_ARGS, Oid auditoroid, AuditChange enable_unable, AuditType audittype, bool is_all_user);
static AuditObjects *get_objlist(char *objects, AuditObjects *audit_obj, char *schema);
static List *audit_get_options_on_object(ObjectType objtype, List *optionlist);
static void add_delete_object_tuple(AuditObjects *objects, Oid auditoroid, List *statments_list,
							char *objtype, char *schema, AuditResult isSuccess, AuditChange enable_unable);
static ObjectInfo *audit_get_object_from_namelist(List *obj, ObjectType objtype, char *schema);
static int add_delete_audit_object3(PG_FUNCTION_ARGS, Oid auditoroid, char *schema, AuditResult isSuccess, AuditChange enable_unable);
static int add_delete_audit_object4(PG_FUNCTION_ARGS, Oid auditoroid, AuditResult isSuccess, AuditChange enable_unable);
static int add_delete_audit_object5(PG_FUNCTION_ARGS, Oid auditoroid, AuditChange enable_unable);
static int disable_audit_stmt_byoid(Oid confid, char mode);
static void audit_insert_log(Relation log, AuditEvent *event, AuditEvent_Obj *object, int i);
static void audit_insert_all_log(Relation log, AuditEvent * event, AuditEvent_Obj * object, int i);
static AuditEventStackItem *stack_push(void);
static AuditEventStackItem *stack_pop_to_list(AuditResult result);
static char *audit_get_actionname_DML(CmdType operation);
static void audit_ExecutorStart_hook(QueryDesc *queryDesc, int eflags);
static void alter_obj_type_auditstmt_actionname(ObjectType objtype, char *buf);
static void audit_get_actionname_from_tree(Node *parsetree, char *buf);
static void log_select_dml_col(RangeTblEntry *rte, Oid relOid, Relation rel);
static void log_select_dml(List *rangeTabls);
static void audit_object_access_hook(ObjectAccessType access,
					Oid classId,Oid objectId,int subId,void *arg);
static bool audit_check_repeat_obj(AuditEvent *auditEvent, char *schemaName, char *objectName, AttrNumber attnum);
static void log_function_execute(Oid objectId);
static void log_index_execute(Oid objectId);
static void log_seq_execute(Oid objectId);
static void audit_aborted(void);
static void audit_commited(void);
static void audit_write_record_list(void);
static void audit_secmon_threshold_check(void);
static void audit_write_record_system(AuditEvent *event);
static char *audit_get_privilege(char *type);
static char *get_auth_method(UserAuth method);
static char *get_remote_mac(Port *port_m);
static AuditEvent *generateSystemEvent(AuditSystemEvent event, AuditResult result);
static void audit_cmd(void);
static void execute_snapshot(void);
static void pg_audit_sighup(SIGNAL_ARGS);
static void pg_audit_sigterm(SIGNAL_ARGS);
static void pgaudit_setup_memcxt(void);
static void audit_postmaster_sighup_guc(char *context);
static void audit_arch_job(void);
#ifdef USE_SQLITE
static void audit_set_sqlite_path(void);
#endif
static void audit_stop_pgagent(void);
static Oid add_secmon_rule(char *rule, Oid confid, AuditResult isSuccess, AuditChange enable_unable);
static Oid audit_insert_secmon_tuple(Relation pg_secmon_rule, Name rulename, Oid confid, AuditResult isSuccess, AuditChange enable_unable);
static void alter_secmon_rule_arg2(int args, Oid oid, char *rule, AuditResult isSuccess, AuditChange enable_unable);
static void alter_secmon_rule_arg3(PG_FUNCTION_ARGS, Oid oid, char *rule);
static void alter_secmon_rule_arg4(PG_FUNCTION_ARGS, Oid oid, char *rule);
static void audit_check_secmon_rule(int confindex, Oid confid, AuditEvent *event);
static void audit_insert_secmon_history(Form_pg_secmon_rule rule, Relation history, AuditEvent *event);
static void cache_secmon_history(List *oids, TimestampTz audittime, Oid commandid);
static void do_insert_secmon_history(Form_pg_secmon_rule rule, Relation history, AuditEvent *event);
static bool check_onetime_threshold(TimestampTz starttime, char *basename, char *basevalue, TimestampTz impt_starttime,
												TimestampTz impt_endtime, int2vector *impt_columns,  int confindex, AuditEvent *event, char *msg);
static bool check_maxtimes_threshold(int32 interval, int32 maxtimes, TimestampTz starttime, Oid ruleoid, char *msg);
static bool check_sequence_threshold(int32 interval, int32 nsteps, TimestampTz starttime, oidvector *ruleseq, char *msg);
static void do_insert_secmon_thr_history(Form_pg_secmon_threshold thr, Relation thrhistory, SecmonRuleEntry *rule,
													char *basename, char *basevalue,
													TimestampTz starttime, TimestampTz impt_starttime, TimestampTz impt_endtime,
													int2vector *impt_cloumns, oidvector *ruleseq);
static void audit_insert_secmon_thr_history(Form_pg_secmon_threshold thr, Relation thrhistory, SecmonRuleEntry *rule,
												char *basename, char *basevalue,
												TimestampTz starttime, TimestampTz impt_starttime, TimestampTz impt_endtime,
												int2vector *impt_cloumns, oidvector *ruleseq);
static void audit_threshold_alarm(int32 level, char *msg);
static void audit_send_mail(char *msg);
static void audit_terminate_session(void);
static void audit_lock_user(void);
static Node *audit_change_where_clause(Node *clause);
static void audit_get_from_clause_name(Node *n);
static Node * make_NullTest(char *col);
static BoolExpr *Audit_make_BoolExprNode(BoolExprType boolop, char *expr1_left,char *expr1_equl, char *expr1_right,
											char *expr2_left,char *expr2_equl, char *expr2_right);
static Node *Audit_A_Expr(char *col, char *name, char *equl);
static ColumnRef *make_ColumnRef(char *name);
static Node *make_A_Const(char *name, int location);
static BoolExpr *make_BoolExprNode(BoolExprType boolop, Node *left_ExprNode, Node *right_ExprNode);
static void audit_create_usr_list_syssao(void);
static void	audit_create_usr_list_syssso(void);
static Node *audit_privi_mode(Node *n, bool iswhereclase);
void audit_ddl_command_end(PG_FUNCTION_ARGS, const char *query);
void pg_audit_launch(void);
void _PG_init(void);
void _PG_fini(void);
static void audit_choice_retentionpolicy_mode(void);
static void audit_delete_log(TimestampTz del_time);
static int64 audit_get_total_log_size(char *table);
static int is_valid_string(const char *str);
static void get_disk_space(const char *path, uint64 *total, uint64 *free);
static void audit_disk_full(void);
static void AuditShememInit(void);
static void audit_hghac_sync_guc(Relation	audit_log, AuditEvent *auditevent);
static bool is_exceeded_disklimit(char relkind);
#if defined(USE_SQLITE) && defined(ENABLE_DKY_BETA_FEATURE)
static void audit_drop_table_partition(void);
static void audit_die(SIGNAL_ARGS);
static void audit_register_all_db_work(AuditTable flag);
void pg_audit_launch_create_table(Datum main_arg);
void pg_audit_launch_drop_table(Datum main_arg);
static bool is_db_changes(List      *sublist);
static bool is_table_exits(TimestampTz time_cur);
static bool check_disk_size(void);
/* Begin - BUG#Z6360 */
static bool find_is_exist_create_table_proc(Oid dbid);
/* End - BUG#Z6360 */
#endif

static void send_signal_launch_proc(void);
static bool is_enable_audit(void);
static void audit_WriteThrHistory(void);
/* Begin BUG#z6137 */
static void check_base_line(const char *basename, const char *basevalue);
/* End BUG#z6137 */


PG_FUNCTION_INFO_V1(pg_audit_ddl_command_end);
PG_FUNCTION_INFO_V1(pg_audit_sql_drop);
PG_FUNCTION_INFO_V1(pg_audit_file_archive);
PG_FUNCTION_INFO_V1(pg_audit_privilege);
PG_FUNCTION_INFO_V1(pg_noaudit_privilege);
PG_FUNCTION_INFO_V1(pg_noaudit_privilege_byoid);
PG_FUNCTION_INFO_V1(pg_audit_stmt);
PG_FUNCTION_INFO_V1(pg_noaudit_stmt);
PG_FUNCTION_INFO_V1(pg_noaudit_stmt_byoid);
PG_FUNCTION_INFO_V1(pg_audit_user);
PG_FUNCTION_INFO_V1(pg_audit_all_user);
PG_FUNCTION_INFO_V1(pg_noaudit_user);
PG_FUNCTION_INFO_V1(pg_noaudit_all_user);
PG_FUNCTION_INFO_V1(pg_noaudit_user_byoid);
PG_FUNCTION_INFO_V1(pg_audit_object);
PG_FUNCTION_INFO_V1(pg_noaudit_object);
PG_FUNCTION_INFO_V1(pg_noaudit_object_byoid);
PG_FUNCTION_INFO_V1(pg_create_secmon_rule);
PG_FUNCTION_INFO_V1(pg_alter_secmon_rule);
PG_FUNCTION_INFO_V1(pg_drop_secmon_rule);
PG_FUNCTION_INFO_V1(pg_create_secmon_threshold);
PG_FUNCTION_INFO_V1(pg_alter_secmon_threshold);
PG_FUNCTION_INFO_V1(pg_drop_secmon_threshold);
PG_FUNCTION_INFO_V1(current_is_syssao);
PG_FUNCTION_INFO_V1(current_is_syssso);
PG_FUNCTION_INFO_V1(current_is_superuser);
PG_FUNCTION_INFO_V1(audit_show_diskinfo);
PG_FUNCTION_INFO_V1(copy_file_to_remote_srv);
PG_FUNCTION_INFO_V1(pg_audit_switch_primary);
PG_FUNCTION_INFO_V1(show_audit_param);

Datum
current_is_syssao(PG_FUNCTION_ARGS)
{
	Oid		role_id = GetUserId();
	bool	result = IS_SYSSAO(role_id);

	PG_RETURN_BOOL(result);
}

Datum
current_is_syssso(PG_FUNCTION_ARGS)
{
	Oid		role_id = GetUserId();
	bool	result = IS_SYSSSO(role_id);

	PG_RETURN_BOOL(result);
}

Datum
current_is_superuser(PG_FUNCTION_ARGS)
{
	bool	result = superuser();

	PG_RETURN_BOOL(result);
}




/*
 * Test to see if a directory exists and is empty or not.
 *
 * Returns:
 *		0 if nonexistent
 *		1 if exists and empty
 *		2 if exists and contains _only_ dot files
 *		3 if exists and contains a mount point
 *		4 if exists and not empty
 *		-1 if trouble accessing directory (errno reflects the error)
 */
static int
audit_check_dir(const char *dir)
{
	int			result = 1;
	DIR		   *chkdir;
	struct dirent *file;
	bool		dot_found = false;
	bool		mount_found = false;
	int			readdir_errno;

	chkdir = opendir(dir);
	if (chkdir == NULL)
		return (errno == ENOENT) ? 0 : -1;

	while (errno = 0, (file = readdir(chkdir)) != NULL)
	{
		if (strcmp(".", file->d_name) == 0 ||
			strcmp("..", file->d_name) == 0)
		{
			/* skip this and parent directory */
			continue;
		}
#ifndef WIN32
		/* file starts with "." */
		else if (file->d_name[0] == '.')
		{
			dot_found = true;
		}
		/* lost+found directory */
		else if (strcmp("lost+found", file->d_name) == 0)
		{
			mount_found = true;
		}
#endif
		else
		{
			result = 4;			/* not empty */
			break;
		}
	}

	if (errno)
		result = -1;			/* some kind of I/O error? */

	/* Close chkdir and avoid overwriting the readdir errno on success */
	readdir_errno = errno;
	if (closedir(chkdir))
		result = -1;			/* error executing closedir */
	else
		errno = readdir_errno;

	/* We report on mount point if we find a lost+found directory */
	if (result == 1 && mount_found)
		result = 3;

	/* We report on dot-files if we _only_ find dot files */
	if (result == 1 && dot_found)
		result = 2;

	return result;
}

/*
 * Function: audit_mkdir_p()
 *
 * Description:
 * 	 execute function for creating directory
 *
 * Input:
 *	path: directory name
 *	omode: directory permissions
 *
 */
static int
audit_mkdir_p(char *path, int omode)
{
	struct stat sb;
	mode_t		numask,
				oumask;
	int			last,
				retval;
	char	   *p;

	retval = 0;
	p = path;

#ifdef WIN32
	/* skip network and drive specifiers for win32 */
	if (strlen(p) >= 2)
	{
		if (p[0] == '/' && p[1] == '/')
		{
			/* network drive */
			p = strstr(p + 2, "/");
			if (p == NULL)
			{
				errno = EINVAL;
				return -1;
			}
		}
		else if (p[1] == ':' &&
				 ((p[0] >= 'a' && p[0] <= 'z') ||
				  (p[0] >= 'A' && p[0] <= 'Z')))
		{
			/* local drive */
			p += 2;
		}
	}
#endif

	/*
	 * POSIX 1003.2: For each dir operand that does not name an existing
	 * directory, effects equivalent to those caused by the following command
	 * shall occur:
	 *
	 * mkdir -p -m $(umask -S),u+wx $(dirname dir) && mkdir [-m mode] dir
	 *
	 * We change the user's umask and then restore it, instead of doing
	 * chmod's.  Note we assume umask() can't change errno.
	 */
	oumask = umask(0);
	numask = oumask & ~(S_IWUSR | S_IXUSR);
	(void) umask(numask);

	if (p[0] == '/')			/* Skip leading '/'. */
		++p;
	for (last = 0; !last; ++p)
	{
		if (p[0] == '\0')
			last = 1;
		else if (p[0] != '/')
			continue;
		*p = '\0';
		if (!last && p[1] == '\0')
			last = 1;

		if (last)
			(void) umask(oumask);

		/* check for pre-existing directory */
		if (stat(path, &sb) == 0)
		{
			if (!S_ISDIR(sb.st_mode))
			{
				if (last)
					errno = EEXIST;
				else
					errno = ENOTDIR;
				retval = -1;
				break;
			}
		}
		else if (mkdir(path, last ? omode : S_IRWXU | S_IRWXG | S_IRWXO) < 0)
		{
			retval = -1;
			break;
		}
		if (!last)
			*p = '/';
	}

	/* ensure we restored umask */
	(void) umask(oumask);

	return retval;
}

/*
 * Function: audit_mkdirs()
 *
 * Description:
 * 	 create Path
 *
 */
static int
audit_mkdirs(char const *argname)
{
	char	   *name;
	char	   *cp;
	int ret =0;

	if (argname == NULL || argname[0] == '\0')
		return 0;

	cp = name = strdup(argname);

	/*
	 * On MS-Windows systems, do not worry about drive letters or backslashes,
	 * as this should suffice in practice.  Time zone names do not use drive
	 * letters and backslashes.  If the -d option of zic does not name an
	 * already-existing directory, it can use slashes to separate the
	 * already-existing ancestor prefix from the to-be-created subdirectories.
	 */

	/* Do not mkdir a root directory, as it must exist.  */
	while (*cp == '/')
		cp++;

	while (cp && ((cp = strchr(cp, '/'))))
	{
		if (cp)
			*cp = '\0';

		/*
		 * Try to create it.  It's OK if creation fails because the directory
		 * already exists, perhaps because some other process just created it.
		 * For simplicity do not check first whether it already exists, as
		 * that is checked anyway if the mkdir fails.
		 */
		ret = audit_mkdir_p(name, S_IRWXU);
		if (ret < 0)
		{
			return ret;
		}

		if (cp)
			*cp++ = '/';
	}
	ret = audit_mkdir_p(name, S_IRWXU);
	free(name);

	return ret;
}

/*
 * hide passwords in audit records
 */
static bool 
hide_password(char * query_string)
{
	char *cp,*a,*b;

	if (!query_string)
		return false;
	for (cp = query_string; *cp!='\0'; cp++)
	{
		*cp = tolower(*cp);
	}
	
	if (strstr(query_string, "password"))
	{
		if (strstr(query_string, "alter") || strstr(query_string, "create"))
		{
			if(strstr(query_string,"user") || strstr(query_string ,"role"))
			{
				cp = strstr(query_string, "password");
				a = strchr(cp, '\'');
				b = strrchr(cp, '\'');

				cp = a + 1;
				for (int i = 0; i < b-a-1; i++)
					cp[i] = '*';

				return true;
			}
		}
	}
	return false;
}

/*
 * check audit archive path
 */
static bool
audit_logdir_check_hook(char **newvalue, void **extra, GucSource source)
{
	int 	ret = 0;
	bool	result;

	if (*newvalue == NULL || *newvalue[0] == '\0')
	{
		return true;
	}

	/* It must be an absolute path */
	if (*newvalue[0] != '/')
	{
		GUC_check_errdetail("Archive directory does not support related paths: %s :%m", *newvalue);
		return false;
	}

	switch ((ret = audit_check_dir(*newvalue)))
	{
		case 0:
			/* dir not exist */
			GUC_check_errdetail("could not access directory: %s :%m", *newvalue);
			result = false;
			break;
		case 1:
		case 2:
		case 3:
		case 4:
			if (access(*newvalue, W_OK) < 0)
			{
				GUC_check_errdetail("no permission for this directory:%s :%m", *newvalue);
				result = false;
			}
			else
			{
				result = true;
			}
			break;
		default:
			/* Trouble accessing directory */
			GUC_check_errdetail("could not access directory: %s :%m", *newvalue);
			result = false;
			break;
	}
	return result;
}


#ifdef USE_SQLITE
/*
 * check sqlite database file path
 */
static bool
audit_sqlite_check_hook(char **newvalue, void **extra, GucSource source)
{
	struct stat fileInfo;
	
	if (*newvalue == NULL || *newvalue[0] == '\0')
		return true;

	if (source == PGC_S_DEFAULT)
		return true;

	if (lstat(*newvalue, &fileInfo) == -1)
	{
		GUC_check_errdetail("could not stat file:%s :%m", *newvalue);
		return false;
	}

	if (S_ISDIR(fileInfo.st_mode))
	{
		GUC_check_errdetail("%s is a directory: %m", *newvalue);
		return false;
	}
	
	return true;

}

/*
 * check sqlite database file path
 */
static void 
audit_sqlite_assign_hook(const char *newvalue, void *extra)
{
	struct stat fileInfo;

	if (newvalue == NULL || newvalue[0] == '\0')
		return;
		
	if (lstat(newvalue, &fileInfo) == -1)
	{
		GUC_check_errdetail("could not stat file:%s :%m", newvalue);
	}

	if (S_ISDIR(fileInfo.st_mode))
	{
		GUC_check_errdetail("%s is a directory: %m", newvalue);
	}
	return;
}
#endif

/*
 * Function: audit_define_GUC()
 *
 * Description:
 * 	 get GUC parameters
 *
 */
static void audit_define_GUC(void)
{
	/* audit switch */
	DefineCustomEnumVariable(audit_parameters[AUDIT_ENABLE],
						 "turn on or off pg_audit",
						 NULL,
						 (int *)&(audit_GUC.audit_enable),
						 AUDIT_OFF,
						 audit_turn_options,
						 PGC_POSTMASTER,
						 0,
						 NULL,
						 NULL,
						 NULL);
	/* audit switch */
	DefineCustomEnumVariable(audit_parameters[AUDIT_RENTENTIONPOLICY],
							 "time or space",
							 NULL,
							 (int *)&(audit_GUC.audit_retentionpolicy_switch),
							 RETEN_SPACE,
							 audit_retion_options,
							 PGC_POSTMASTER,
							 0,
							 NULL,
							 NULL,
							 NULL);

	/* save logs by time or space */
	DefineCustomIntVariable(audit_parameters[AUDIT_RENTENTIONIDAYS],
				 "set the audit log save days",
				 "If set to 30, the log save 30 days.",
				 &(audit_GUC.audit_retentiondays),
				 180,
				 1, INT_MAX,
				 PGC_POSTMASTER,
				 0,
				 NULL,
				 NULL,
				 NULL);
	/* timed task detection time, delete logs that exceed the time limit */
	DefineCustomIntVariable(audit_parameters[AUDIT_DETECTIONTIME],
				 "set audit log deletion detection time",
				 "If set to 1, deletion detection time is 1 day.",
				 &(audit_GUC.audit_detectiontime),
				 1,
				 1, INT_MAX,
				 PGC_POSTMASTER,
				 0,
				 NULL,
				 NULL,
				 NULL);
	/* maximum allowable usage space */
	DefineCustomIntVariable(audit_parameters[AUDIT_SPACELIMIT],
					 "set MB",
					 "only support XXXG",
					 &audit_spacelimit,
					 10240,
					 1, INT_MAX,
					 PGC_POSTMASTER,
					 GUC_UNIT_MB,
					 NULL,
					 NULL,
					 NULL);
	/* space usage exceeding threshold alarm */
	 DefineCustomStringVariable(audit_parameters[AUDIT_SPACEALARM],
					  "set audit space alarm",
					  NULL,
					  &audit_space_alarm,
					  "0.8",
					  PGC_POSTMASTER,
					  0,
					  audit_real_check_hook,
					  NULL,
					  NULL);
	/* archive Path */
	DefineCustomStringVariable(audit_parameters[AUDIT_ARCHDIR],
					 "set audit arch path",
					 NULL,
					 &audit_GUC.audit_archdir,
					 "",
					 PGC_POSTMASTER,
					 0,
					 audit_logdir_check_hook,
					 NULL,
					 NULL);
	/* remote path */
	DefineCustomStringVariable(audit_parameters[AUDIT_REMOTE_ARCHDIR],
					 "set audit remote arch path",
					 NULL,
					 &audit_GUC.remote_archdir,
					 "",
					 PGC_POSTMASTER,
					 0,
					 NULL,
					 NULL,
					 NULL);
	/* safety monitoring switch */
	DefineCustomEnumVariable(audit_parameters[AUDIT_MONITORING],
						 "monitoring on/off",
						 NULL,
						 (int *)&(audit_GUC.audit_monitoring),
						 AUDIT_OFF,
						 audit_turn_options,
						 PGC_POSTMASTER,
						 0,
						 NULL,
						 NULL,
						 NULL);
	/* email address for sending threshold alarm */
	DefineCustomStringVariable(audit_parameters[AUDIT_MAIL],
					 "alarm email",
					 NULL,
					 &audit_GUC.audit_mail,
					 "",
					 PGC_POSTMASTER,
					 0,
					 NULL,
					 NULL,
					 NULL);
	 /* permission mode */
	 DefineCustomIntVariable(audit_parameters[AUDIT_PRIVIMODE],
						  "set 1 or 2",
						  "1 is the general mode, 2 is the dedicated mode",
						  &(audit_GUC.audit_privimode),
						  1,
						  1, 2,
						  PGC_POSTMASTER,
						  0,
						  NULL,
						  NULL,
						  NULL);
	/* permission mode */
	DefineCustomEnumVariable(audit_parameters[AUDIT_SYSTEMOPERS],
							  "system audit, support off/succ/fail/all",
							  NULL,
							  (int *)&(audit_GUC.audit_systemopers),
							  AUDIT_OPER_OFF,
							  audit_opers_options,
							  PGC_POSTMASTER,
							  0,
							  NULL,
							  NULL,
							  NULL);
#ifdef USE_SQLITE
	/* system audit switch */
	DefineCustomStringVariable(audit_parameters[AUDIT_SQLITEDBPATH],
					 "set sqlite database path",
					 NULL,
					 &audit_GUC.audit_sqlitedbpath,
					 "pgaudit/sqlite/sqlite.db",
					 PGC_POSTMASTER,
					 0,
					 audit_sqlite_check_hook,
					 audit_sqlite_assign_hook,
					 NULL);
#endif

	/* There may be issues when going to pg_reboot 
	 * during wal recovery test.
	 * This parameter is used to disable pg_track_settings feature 
	 * during wal recovery test.
	 */
	DefineCustomEnumVariable(audit_parameters[AUDIT_TRACE_SETTINGS],
						 "turn on or off",
						 NULL,
						 (int *)&(audit_GUC.track_settings),
						 AUDIT_ON,
						 audit_turn_options,
						 PGC_POSTMASTER,
						 0,
						 NULL,
						 NULL,
						 NULL);

	DefineCustomEnumVariable(audit_parameters[AUDIT_LOGPOLICY],
						  "choice audit log policy",
						  NULL,
						  (int *)&(audit_GUC.logpolicy),
						  AUDIT_PAUSE,
						  audit_policy_options,
						  PGC_POSTMASTER,
						  0,
						  NULL,
						  NULL,
						  NULL);
}

/*
 * Function: audit_change_GUC()
 *
 * Description:
 * 	 modify GUC parameters
 *
 */
static void 
audit_change_GUC(void)
{
	uint64 tmp = 0;

	tmp = audit_spacelimit;
	audit_GUC.audit_spacelimit = tmp * AUDIT_MB;
	//audit_change_auditpause();//TODO
}

/*
 * Function: stack_push()
 *
 * Description:
 * 	 apply for an AuditEvent corresponding to SQL
 *
 */
static AuditEventStackItem * 
stack_push(void)
{
	AuditEventStackItem *stackItem;
	MemoryContext  oldcontext;
	MemoryContext  itemcontext = NULL;

	/*
	 * AuditContext : The context in which all audit information is located.
	 * List and stack structures are also included in this context
	 */
	if(!AuditContext)
		AuditContext = AllocSetContextCreate(TopMemoryContext,
											 "pgaudit  context",
											 ALLOCSET_DEFAULT_SIZES);

	oldcontext = MemoryContextSwitchTo(AuditContext);

	stackItem = palloc0(sizeof(AuditEventStackItem));

	/*
	 * Create a memory context for each AuditEvent structure, 
	 * where each SQL corresponds to an AuditEvent
	 */
	itemcontext = AllocSetContextCreate(AuditContext,
										"a pgaudit item context",
										ALLOCSET_DEFAULT_SIZES);

	MemoryContextSwitchTo(itemcontext);
	stackItem->auditEvent = palloc0(sizeof(AuditEvent));
	stackItem->auditEvent->auditContext = itemcontext;

	/* This is the method of inserting the header of the chain, 
	 * concatenating the AuditEvent corresponding to each SQL statement
	 */
	if (auditEventStack)
		 stackItem->next = auditEventStack;
	else
		 stackItem->next = NULL;

	auditEventStack = stackItem;

	MemoryContextSwitchTo(oldcontext);

	return stackItem;
}


/*
 * Function: audit_get_type_DML()
 *
 * Description:
 * 	 Convert the command type of DML type SQL to a type recognized by pg_audit
 *
 * Input:
 *	SQL command type
 *
 */
static char *
audit_get_actionname_DML(CmdType operation)
{
	char	   *actionmane = INVALID_STMT_S;

	/* Initialize command using queryDesc->operation */
	switch (operation)
	{
		case CMD_SELECT:
			actionmane = SELECT_S;
			break;

		case CMD_INSERT:
			actionmane = INSERT_S;
			break;

		case CMD_UPDATE:
			actionmane = UPDATE_S;
			break;

		case CMD_DELETE:
			actionmane = DELETE_S;
			break;

		default:
			actionmane = INVALID_STMT_S;
			break;
	}

	return actionmane;
}

/*
 * Function: audit_get_privilege
 * Description: Obtain permission levels for audit statement types
 * Input: Audit statement type
 * Return: Permission level for audit statement types
 *
 */
static char *
audit_get_privilege(char *type)
{
	char *pri = malloc(100);

	if (pg_strcasecmp(type, CREATE_ROLE_S) == 0 ||
		pg_strcasecmp(type, CREATE_SCHEMA_S) == 0 ||
		pg_strcasecmp(type, CREATE_USER_S) == 0 ||
		pg_strcasecmp(type, ALTER_ROLE_S) == 0 ||
		pg_strcasecmp(type, ALTER_SYSTEM_S) == 0 ||
		pg_strcasecmp(type, ALTER_SCHEMA_S) == 0 ||
		/* Begin - BUG#00004719 */
		pg_strcasecmp(type, ALTER_USER_S) == 0 ||
		/* End - BUG#00004719 */
		pg_strcasecmp(type, DROP_ROLE_S) == 0 ||
		pg_strcasecmp(type, DROP_SCHEMA_S) == 0 ||
		pg_strcasecmp(type, DROP_USER_S) == 0 ||
		pg_strcasecmp(type, GRANT_S) == 0 ||
		pg_strcasecmp(type, REVOKE_S) == 0 ||
		pg_strcasecmp(type, LOGIN_S) == 0 ||
		pg_strcasecmp(type, LOGOUT_S) == 0 ||
		pg_strcasecmp(type, PG_DUMP_S) == 0 ||
		pg_strcasecmp(type, PG_DUMPALL_S) == 0 ||
		pg_strcasecmp(type, PG_RESTORE_S) == 0 ||
		pg_strcasecmp(type, PG_BASEBACKUP_S) == 0 ||
		pg_strcasecmp(type, PG_PERMISSION_S) == 0)
		strcpy(pri, AUDIT_PRIVILEGE_SYSTEM_S);
	else if (pg_strcasecmp(type, CREATE_EXTENSION_S) == 0 ||
		pg_strcasecmp(type, CREATE_DATABASE_S) == 0 ||
		pg_strcasecmp(type, ALTER_EXTENSION_S) == 0 ||
		pg_strcasecmp(type, ALTER_DATABASE_S) == 0 ||
		pg_strcasecmp(type, DROP_DATABASE_S) == 0 ||
		pg_strcasecmp(type, DROP_EXTENSION_S) == 0 ||
		pg_strcasecmp(type, IMPORT_S) == 0 ||
		pg_strcasecmp(type, EXPORT_S) == 0 ||
		/* Begin - BUG#Z6075 */
		pg_strcasecmp(type, CREATE_RESOURCE_GROUP_S) == 0 ||
		pg_strcasecmp(type, ALTER_RESOURCE_GROUP_S) == 0 ||
		pg_strcasecmp(type, DROP_RESOURCE_GROUP_S) == 0)
		/* End - BUG#Z6075 */
		strcpy(pri, AUDIT_PRIVILEGE_DATABASE_S);
	else if (pg_strcasecmp(type, CREATE_DOMAIN_S) == 0 ||
		pg_strcasecmp(type, CREATE_PROCEDURE_S) == 0 ||
		pg_strcasecmp(type, CREATE_INDEX_S) == 0 ||
		pg_strcasecmp(type, CREATE_SEQUENCE_S) == 0 ||
		pg_strcasecmp(type, CREATE_TABLE_S) == 0 ||
		pg_strcasecmp(type, CREATE_TABLE_AS_S) == 0 ||
		pg_strcasecmp(type, CREATE_TRIGGER_S) == 0 ||
		pg_strcasecmp(type, CREATE_VIEW_S) == 0 ||
		pg_strcasecmp(type, ALTER_DOMAIN_S) == 0 ||
		pg_strcasecmp(type, ALTER_TABLE_S) == 0 ||
		pg_strcasecmp(type, ALTER_VIEW_S) == 0 ||
		pg_strcasecmp(type, ALTER_PROCEDURE_S) == 0 ||
		pg_strcasecmp(type, ALTER_INDEX_S) == 0 ||
		pg_strcasecmp(type, ALTER_SEQUENCE_S) == 0 ||
		pg_strcasecmp(type, ALTER_TRIGGER_S) == 0 ||
		pg_strcasecmp(type, DROP_DOMAIN_S) == 0 ||
		pg_strcasecmp(type, DROP_PROCEDURE_S) == 0 ||
		pg_strcasecmp(type, DROP_INDEX_S) == 0 ||
		pg_strcasecmp(type, DROP_SEQUENCE_S) == 0 ||
		pg_strcasecmp(type, DROP_TABLE_S) == 0 ||
		pg_strcasecmp(type, DROP_TRIGGER_S) == 0 ||
		pg_strcasecmp(type, DROP_VIEW_S) == 0 ||
		pg_strcasecmp(type, SELECT_S) == 0 ||
		pg_strcasecmp(type, SELECT_INTO_S) == 0 ||
		pg_strcasecmp(type, INSERT_S) == 0 ||
		pg_strcasecmp(type, UPDATE_S) == 0 ||
		pg_strcasecmp(type, DELETE_S) == 0 ||
		pg_strcasecmp(type, COMMENT_S) == 0 ||
		pg_strcasecmp(type, RESET_S) == 0 ||
		pg_strcasecmp(type, SET_S) == 0 ||
		pg_strcasecmp(type, TRUNCATE_S) == 0 ||
		pg_strcasecmp(type, COPY_TO_S) == 0 ||
		pg_strcasecmp(type, COPY_FROM_S) == 0 ||
		pg_strcasecmp(type, REINDEX_S) == 0 ||
		pg_strcasecmp(type, LOCK_S) == 0 ||
		pg_strcasecmp(type, CALL_S) == 0)
		strcpy(pri, AUDIT_PRIVILEGE_SCHEMAOBJECT_S);
	else if (pg_strcasecmp(type, START_S) == 0 ||
		pg_strcasecmp(type, STOP_S) == 0 ||
		pg_strcasecmp(type, RELOAD_S) == 0 ||
		pg_strcasecmp(type, AUDIT_STORAGE_INVALID_S) == 0 ||
		pg_strcasecmp(type, ALTER_GUC_S) == 0 ||
		pg_strcasecmp(type, MANDATORY_AUDIT_ON_S) == 0 ||
		pg_strcasecmp(type, MANDATORY_AUDIT_OFF_S) == 0 ||
		pg_strcasecmp(type, SWITCHOVER_S) == 0)
		strcpy(pri, AUDIT_PRIVILEGE_INSTANCE_S);
	else
		strcpy(pri, AUDIT_PRIVILEGE_SCHEMAOBJECT_S);

	return pri;
}


/*
 * insert audit events into pg_audit_log or sqlite_audit_log
 */
static void
audit_insert_all_log(Relation log, AuditEvent *event, AuditEvent_Obj *object, int i)
{
	if (*audit_GUC.audit_pause)
		return ;

	if(is_exceeded_disklimit(log->rd_rel->relkind))
	{
		if (audit_GUC.logpolicy == AUDIT_PAUSE)
		{
			return ;
		}
	}

	PG_TRY();
	{
		/* insert audit events into pg_audit_log */
		audit_insert_log(log, event, object, i);
	}

	/* when encountering errors while inserting into pg_audit_log, 
	 * taht is, when audit storage becomes invalid, 
	 * insert into sqlite_audit_log */
	PG_CATCH();
	{
		ErrorData *edata;
		MemoryContext oldcontext = CurrentMemoryContext;

		MemoryContextSwitchTo(TopMemoryContext);
		
#ifdef USE_SQLITE
		if (sqlite_open_db_for_audit_hook)
			sqlite_open_db_for_audit_hook(SYSTEMAUDIT_AUDIT_STORAGE_INVALID, AUDIT_SUCCESS, NULL, NULL);
#endif

	    edata = CopyErrorData();
	    FlushErrorState();
	    MemoryContextSwitchTo(oldcontext);

	    if (edata->elevel >= ERROR)
	    {
	        edata->elevel = WARNING;
			ereport(WARNING, (errcode(edata->sqlerrcode),
                    errmsg_internal("%s", edata->message)));
	    }

	}
	PG_END_TRY();
	
}

/* insert tuple into pg_audit_log */
static void
audit_insert_log(Relation log, AuditEvent *event, AuditEvent_Obj *object, int i)
{
	Datum	   *values;
	bool	   *nulls;
	char	   *tmp = NULL;
	Oid			roleid = 0;
	CatCList   *catlist;
	HeapTuple	tuple;
	char *query_string_tmp = NULL;

	values = palloc(AUDIT_LOG_NATTS*sizeof(Datum));
	nulls = palloc(AUDIT_LOG_NATTS*sizeof(bool));

	memset(values, 0, AUDIT_LOG_NATTS*sizeof(Datum));
	memset(nulls, false, AUDIT_LOG_NATTS*sizeof(bool));

	if (i >= 0)
		values[0] = Int32GetDatum(event->confoids[i]);
	else
	{
		values[0] = (Datum)0;
		nulls[0] = true;
	}
	values[1] = Int32GetDatum(event->sessionid);
	values[2] = Int32GetDatum(event->useroid);
	tmp = GetUserNameFromId(event->useroid, true);
	if (tmp)
	{
		values[3] = PointerGetDatum(cstring_to_text(tmp));
	}
	else
	{
		values[3] = (Datum)0;
		nulls[3] = true;
	}

	if (event->useroid)
	{
		catlist = SearchSysCacheList1(AUTHMEMMEMROLE, ObjectIdGetDatum(event->useroid));
		if (catlist->n_members > 0)
		{
			int j = 0;
			for (j = 0; j < catlist->n_members; j++)
			{
				HeapTuple   auth_member_tuple = &catlist->members[j]->tuple;
				Form_pg_auth_members auth_member = (Form_pg_auth_members) GETSTRUCT(auth_member_tuple);

				roleid = auth_member->roleid;
				break;
			}
		}
		ReleaseSysCacheList(catlist);
	}

	if (roleid)
	{
		values[4] = Int32GetDatum(roleid);
		tmp = GetUserNameFromId(roleid, true);

		if (tmp)
		{
			values[5] = PointerGetDatum(cstring_to_text(tmp));

			
		}
		else
		{
			values[5] = (Datum)0;
			nulls[5] = true;
		}
	}
	else
	{
		values[4] = (Datum)0;
		nulls[4] = true;
		values[5] = (Datum)0;
		nulls[5] = true;
	}
		
	values[6] = TimestampTzGetDatum(event->audittime);
	//values[7] = CharGetDatum(event->result);
	values[7] = PointerGetDatum(cstring_to_text(AuditResultToString(event->result)));
	

	if (object && event->conftypes[i] == AUDIT_TYPE_OBJECT)
	{
		values[8] = Int32GetDatum(object->schemaId);
		if (object->schemaName)
		{
			values[9] = PointerGetDatum(cstring_to_text(object->schemaName));
		}
		else
		{
			values[9] = (Datum)0;
			nulls[9] = true;
		}
		values[10] = Int32GetDatum(object->objectId);
		if (object->objectName)
		{
			values[11] = PointerGetDatum(cstring_to_text(object->objectName));
		}
		else
		{
			values[11] = (Datum)0;
			nulls[11] = true;
		}
		if (object->colname)
		{
			values[12] = PointerGetDatum(cstring_to_text(object->colname));
		}
		else
		{
			values[12] = (Datum)0;
			nulls[12] = true;
		}
		values[13] = Int32GetDatum(object->classId);
		if (object->objectClassdesc)
		{
			values[14] = PointerGetDatum(cstring_to_text(object->objectClassdesc));
		}
		else
		{
			values[14] = (Datum)0;
			nulls[14] = true;
		}
	}
	else
	{
		values[8] = (Datum)0;
		nulls[8] = true;
		values[9] = (Datum)0;
		nulls[9] = true;
		values[10] = (Datum)0;
		nulls[10] = true;
		values[11] = (Datum)0;
		nulls[11] = true;
		values[12] = (Datum)0;
		nulls[12] = true;
		values[13] = (Datum)0;
		nulls[13] = true;
		values[14] = (Datum)0;
		nulls[14] = true;
	}

	if (i == -1)
		values[15] = PointerGetDatum(cstring_to_text(AUDIT_TYPE_SYSYEM_S)); /* BUG#z6149 */
		//values[15] = CharGetDatum(AUDIT_TYPE_SYSYEM);
	else if (i == -2)
		values[15] = PointerGetDatum(cstring_to_text(AUDIT_TYPE_MANDATORY_S));
		//values[15] = CharGetDatum(AUDIT_TYPE_MANDATORY);
	else
		values[15] = PointerGetDatum(cstring_to_text(AuditTypeToString(event->conftypes[i])));
		//values[15] = CharGetDatum(event->conftypes[i]);

	values[16] = Int32GetDatum(event->commandid);

	values[17] = PointerGetDatum(cstring_to_text(event->actionname));
	
	if (strlen(clientInfo.port) > 0)
	{
		values[18] = PointerGetDatum(cstring_to_text(clientInfo.port));
	}
	else
	{
		values[18] = (Datum)0;
		nulls[18] = true;
	}
	if (strlen(clientInfo.clientname) > 0)
	{
		values[19] = PointerGetDatum(cstring_to_text(clientInfo.clientname));
	}
	else
	{
		values[19] = (Datum)0;
		nulls[19] = true;
	}
	if (strlen(clientInfo.ip) > 0)
	{
		values[20] = PointerGetDatum(cstring_to_text(clientInfo.ip));
	}
	else
	{
		values[20] = (Datum)0;
		nulls[20] = true;
	}
	if (strlen(clientInfo.mac) > 0)
	{
		values[21] = PointerGetDatum(cstring_to_text(clientInfo.mac));
	}
	else
	{
		values[21] = (Datum)0;
		nulls[21] = true;
	}
	if (strlen(clientInfo.appname) > 0)
	{
		values[22] = PointerGetDatum(cstring_to_text(clientInfo.appname));
	}
	else
	{
		values[22] = (Datum)0;
		nulls[22] = true;
	}
	if (event->sqltext)
	{	
		query_string_tmp = pstrdup(event->sqltext);
		if (hide_password(query_string_tmp))
		{
			values[23] = PointerGetDatum(cstring_to_text(query_string_tmp));
		}
		else
		{
			values[23] = PointerGetDatum(cstring_to_text(event->sqltext));
		}
		pfree(query_string_tmp);
		
	}
	else
	{
		values[23] = (Datum)0;
		nulls[23] = true;
	}
#if !defined(_WIN32) && !defined(_WIN64)
	if (event->errtext)
	{
		values[24] = PointerGetDatum(cstring_to_text(event->errtext));
	}
	else if (strlen(audit_msg) != 0)
	{
		values[24] = PointerGetDatum(cstring_to_text(audit_msg));
	}
	else
#endif
	{
		values[24] = (Datum)0;
		nulls[24] = true;
	}
	if (strlen(clientInfo.authmethod) > 0)
	{
		values[25] = PointerGetDatum(cstring_to_text(clientInfo.authmethod));
	}

	else
	{
		values[25] = (Datum)0;
		nulls[25] = true;
	}
	values[26] = PointerGetDatum(cstring_to_text(audit_get_privilege(event->actionname)));

	tuple = heap_form_tuple(RelationGetDescr(log), values, nulls);

#if defined(USE_SQLITE) && defined(ENABLE_DKY_BETA_FEATURE)

	if (log->rd_rel->relkind == RELKIND_PARTITIONED_TABLE)
	{
		char   *date_str = NULL;
		Oid 		relOid;
		StringInfoData buf;
		Relation	audit_log;

		date_str = text_to_cstring(DatumGetTextP(DirectFunctionCall2(timestamptz_to_char,
													TimestampTzGetDatum(event->audittime),
													PointerGetDatum(cstring_to_text("YYYYMMDD")))));
		initStringInfo(&buf);
		appendStringInfo(&buf, "%s_%s", PG_AUDIT_LOG, date_str);

		relOid = get_relname_relid(buf.data, log->rd_rel->relnamespace);
		if (relOid == InvalidOid)
			elog(ERROR, " could not get oid of relation %s", buf.data);

		audit_log = table_open(relOid, RowExclusiveLock);

		simple_heap_insert(audit_log, tuple);
		table_close(audit_log, RowExclusiveLock);
		audit_log = NULL;
	}
	else
#endif
	{
		simple_heap_insert(log, tuple);
		//targetBlock = RelationGetTargetBlock(log);
	}

	if(values)
		pfree(values);
	if(nulls)
		pfree(nulls);

	return ;
}


/*
 * Function: audit_write_record_list()
 *
 * Description:
 * 	 Write record into list
 *
 */
static void 
audit_write_record_list(void)
{
	ListCell *listcell;
	Oid			relOid;
	Relation	audit_log = NULL;
	bool		need_check_alteruser = false;
	bool		need_check_dropuser = false;

#if defined(USE_SQLITE) && defined(ENABLE_DKY_BETA_FEATURE)
	if (!check_disk_size())
		return ;
#endif

	StartTransactionCommand();
	
	relOid = get_relname_relid(PG_AUDIT_LOG, SECURITY_SCHEMA_NAMESPACE);
	if (relOid == InvalidOid)
		elog(ERROR, " could not get oid of relation pg_audit_log");

	audit_log = table_open(relOid, RowExclusiveLock);

	foreach(listcell, auditlist)
	{
		AuditEvent *auditevent = lfirst(listcell);
		int		i;
		CatCList * catlist;
		Form_pg_audit_config audit_form;
		char	objectItem[NAMEDATALEN] = {0};
		int 	n_members = 0;

		#if defined(USE_SQLITE) && defined(ENABLE_DKY_BETA_FEATURE)
		if (pg_strcasecmp(auditevent->actionname, CREATE_DATABASE_S) == 0)
		{
			send_signal_launch_proc();
		}
		#endif

		audit_hghac_sync_guc(audit_log, auditevent);

		/* check pg_audit_config to see if this operation requeries auditing */
		catlist = SearchSysCacheList1(AUDITACTIONNAMEAUDITTYPEID, CStringGetDatum(auditevent->actionname));
		n_members = catlist->n_members;
		
		for (i = 0; i < catlist->n_members; i++)
		{
			
			HeapTuple	audit_tuple = &catlist->members[i]->tuple;
			
			audit_form = (Form_pg_audit_config) GETSTRUCT(audit_tuple);

			if (!audit_form->enable)
			{
				if (strcmp(auditevent->actionname, ALTER_ROLE_S) == 0)
				{
					need_check_alteruser = true;
				}
				else if (strcmp(auditevent->actionname, DROP_ROLE_S) == 0)
				{
					need_check_dropuser = true;
				}
				continue;
			}
			
			if (audit_form->auditmode != auditevent->result && audit_form->auditmode != AUDIT_ALL)
				continue;

			if (audit_form->audittype == AUDIT_TYPE_PRIVILEGE || audit_form->audittype == AUDIT_TYPE_STATEMENT)
			{
				auditevent->confoids[i] = audit_form->confid;
				auditevent->conftypes[i] = audit_form->audittype;

				audit_insert_all_log(audit_log, auditevent, NULL, i);
			}
			else if (audit_form->audittype == AUDIT_TYPE_USER)
			{
				snprintf(objectItem, NAMEDATALEN, "0.0.0");
				
				if (strcmp(audit_form->objoid.data, objectItem) == 0)
				{
					auditevent->confoids[i] = audit_form->confid;
					auditevent->conftypes[i] = audit_form->audittype;

					audit_insert_all_log(audit_log, auditevent, NULL, i);
				}
				else
				{
					char	objItem[NAMEDATALEN] = {0};
					snprintf(objItem, NAMEDATALEN, "%d.0.0", auditevent->useroid);

					if (strcmp(audit_form->objoid.data, objItem) == 0)
					{
						auditevent->confoids[i] = audit_form->confid;
						auditevent->conftypes[i] = audit_form->audittype;

						audit_insert_all_log(audit_log, auditevent, NULL, i);
					}
					
				}
			}
			else if (audit_form->audittype == AUDIT_TYPE_OBJECT)
			{
				ListCell *cell;
				
				foreach(cell, auditevent->objects)
				{
					char	objItem[NAMEDATALEN] = {0};
					AuditEvent_Obj *obj = lfirst(cell);

					if ((strcmp(auditevent->actionname, ALTER_INDEX_S) == 0 ||
						strcmp(auditevent->actionname, DROP_INDEX_S) == 0) &&
						obj->indrelId != InvalidOid)
					{
						snprintf(objItem, NAMEDATALEN, "%d.%d.%d", obj->schemaId, obj->indrelId, obj->attnum);
					}
					else
					{
						snprintf(objItem, NAMEDATALEN, "%d.%d.%d", obj->schemaId, obj->objectId, obj->attnum);
					}

					if (strcmp(audit_form->objoid.data, objItem) == 0)
					{
						auditevent->confoids[i] = audit_form->confid;
						auditevent->conftypes[i] = audit_form->audittype;

						audit_insert_all_log(audit_log, auditevent, obj, i);
					}

				}
			}

			if (audit_GUC.audit_monitoring == AUDIT_ON)
				audit_check_secmon_rule(i, audit_form->confid, auditevent);
		}

		/* For the same information on the parse tree as ALTER USER and ALTER ROLE, 
		 * further processing is required 
		 */
		if (((strcmp(auditevent->actionname, ALTER_ROLE_S) == 0) && n_members == 0) || need_check_alteruser)
		{
			CatCList   *catlistr;

			catlistr = SearchSysCacheList1(AUDITACTIONNAMEAUDITTYPEID, CStringGetDatum(ALTER_USER_S));
		
			for (i = 0; i < catlistr->n_members; i++)
			{
				HeapTuple	tuple = &catlistr->members[i]->tuple;
				
				audit_form = (Form_pg_audit_config) GETSTRUCT(tuple);
				
				if (!audit_form->enable)
					continue;
			
				if (audit_form->auditmode != auditevent->result && audit_form->auditmode != AUDIT_ALL)
					continue;

				strcpy(auditevent->actionname, ALTER_USER_S);
				
				auditevent->confoids[i] = audit_form->confid;
				auditevent->conftypes[i] = audit_form->audittype;
				
				audit_insert_all_log(audit_log, auditevent, NULL, i);

				if (audit_GUC.audit_monitoring == AUDIT_ON)
					audit_check_secmon_rule(i, audit_form->confid, auditevent);
			}
			ReleaseSysCacheList(catlistr);
		}
		/* For the same information on the parse tree as DROP USER and DROP ROLE, 
		 * further processing is required 
		 */
		else if (((strcmp(auditevent->actionname, DROP_ROLE_S) == 0) && n_members == 0) || need_check_dropuser)
		{
			CatCList * catlistr;

			catlistr = SearchSysCacheList1(AUDITACTIONNAMEAUDITTYPEID, CStringGetDatum(DROP_USER_S));

			for (i = 0; i < catlistr->n_members; i++)
			{
				HeapTuple	tuple = &catlistr->members[i]->tuple;
				
				audit_form = (Form_pg_audit_config) GETSTRUCT(tuple);

				audit_check_secmon_rule(i, audit_form->confid, auditevent);
				
				if (!audit_form->enable)
					continue;
							
				if (audit_form->auditmode != auditevent->result && audit_form->auditmode != AUDIT_ALL)
					continue;
				
				strcpy(auditevent->actionname, DROP_USER_S);

				auditevent->confoids[i] = audit_form->confid;
				auditevent->conftypes[i] = audit_form->audittype;
				
				audit_insert_all_log(audit_log, auditevent, NULL, i);

				if (audit_GUC.audit_monitoring == AUDIT_ON)
					audit_check_secmon_rule(i, audit_form->confid, auditevent);
			}

			ReleaseSysCacheList(catlistr);
		}
		
		ReleaseSysCacheList(catlist);

		MemoryContextDelete(auditevent->auditContext);	
	}
	
	table_close(audit_log, RowExclusiveLock);
	audit_log = NULL;

	CommitTransactionCommand();

	list_free(auditlist);
	auditlist = NIL;
}

/*
 * write system audit events into pg_audit_log
 */
static void 
audit_write_record_system(AuditEvent *event)
{
	Oid			relOid;
	Relation	audit_log;

#if defined(USE_SQLITE) && defined(ENABLE_DKY_BETA_FEATURE)
		if (!check_disk_size())
			return ;
#endif

	StartTransactionCommand();
	
	relOid = get_relname_relid(PG_AUDIT_LOG, SECURITY_SCHEMA_NAMESPACE);
	if (relOid == InvalidOid)
		elog(ERROR, " could not get oid of relation pg_audit_log");
	
	audit_log = table_open(relOid, RowExclusiveLock);
	
	audit_insert_all_log(audit_log, event, NULL, -1);
	
	table_close(audit_log, RowExclusiveLock);
	audit_log = NULL;

	CommitTransactionCommand();
}


/*
 * Function: audit_CommandStart()
 *
 * Description:
 * 	 Record the start time of SQL execution
 *
 */
static void 
audit_CommandStart(void)
{	
#if !defined(_WIN32) && !defined(_WIN64)
	memset(audit_msg, '\0', MAXPGPATH);
#endif
	
	if (audit_GUC.audit_enable != AUDIT_ON)
		return;

	if (internalStatement)
		return;

	/* SQL start execution time */
	operstarttime = GetCurrentTimestamp();

}

static void 
audit_WriteRecord(void)
{
	if (audit_GUC.audit_enable != AUDIT_ON)
		return;

	/* Read and send from auditlist */
	if (auditlist)
	{
		/* only write audit logs on the primary node */
		if (!RecoveryInProgress())
		{
			audit_write_record_list();
		}
		/* do not write audit logs on the standby node */
		else
		{
			list_free(auditlist);
			auditlist = NIL;
		}
	}
}


/*
 * Function: audit_list_append()
 *
 * Description:
 * 	 Write AuditEvent to the list
 *
 */
static void 
audit_list_append(AuditEvent *listItem)
{
	auditlist = lappend(auditlist,listItem);
}


/*
 * Function: stack_pop_to_list()
 *
 * Description:
 * 	 Add auditvent to list
 *
 */
static AuditEventStackItem * 
stack_pop_to_list(AuditResult result)
{
	MemoryContext oldContext;
	AuditEventStackItem *nextItem;

	if (!auditEventStack)
	{
		ereport(ERROR,(errmsg("stack is empty")));
	}
	oldContext = MemoryContextSwitchTo(AuditContext);
	nextItem = auditEventStack->next;

	auditEventStack->auditEvent->result = result;

#if !defined(_WIN32) && !defined(_WIN64)
	if (audit_msg[0] != '\0')
	{
		auditEventStack->auditEvent->errtext = pstrdup(audit_msg);
		memset(audit_msg, '\0', MAXPGPATH);
	}
#endif

	audit_list_append(auditEventStack->auditEvent);
	
	pfree(auditEventStack);

	MemoryContextSwitchTo(oldContext);

	auditEventStack = nextItem;

	return auditEventStack;
}

/*
 * Function: audit_ExecutorStart_hook()
 *
 * Description:
 * 	 audit_ExecutorStart_hook, create audit items
 *
 * Input:
 *	queryDesc: SQL information
 *
 */
static void 
audit_ExecutorStart_hook(QueryDesc *queryDesc, int eflags)
{
	MemoryContext contextOld;
	AuditEventStackItem *stackItem;

	if (audit_GUC.audit_enable == AUDIT_ON)
	{
		if (!internalStatement)
		{
			char *actionname;
			stackItem = stack_push();

			actionname = audit_get_actionname_DML(queryDesc->operation);

			strcpy(stackItem->auditEvent->actionname, actionname);
			stackItem->auditEvent->conftypes[0] = AUDIT_TYPE_INVALID;
			stackItem->auditEvent->sessionid = MyProcPid;
			stackItem->auditEvent->commandid = GetCurrentCommandId(false);
			stackItem->auditEvent->useroid = GetUserId();
			//stackItem->auditEvent->username = GetUserNameFromId(stackItem->auditEvent->useroid, true);
			stackItem->auditEvent->audittime = operstarttime;
			/* Initialize the audit event */
			contextOld = MemoryContextSwitchTo(stackItem->auditEvent->auditContext);
			stackItem->auditEvent->sqltext = pnstrdup(queryDesc->sourceText,1023);
			MemoryContextSwitchTo(contextOld);
		}
	}

	/* Call the previous hook or standard function */
	if (next_ExecutorStart_hook)
		next_ExecutorStart_hook(queryDesc, eflags);
	else
		standard_ExecutorStart(queryDesc, eflags);
}


static void
audit_check_pg_track_settings(Node *parsetree)
{
	if (audit_GUC.audit_enable != AUDIT_ON || audit_GUC.audit_systemopers == AUDIT_OPER_OFF)
		return;

	if (nodeTag(parsetree) == T_AlterDatabaseSetStmt || nodeTag(parsetree) == T_AlterRoleSetStmt)
	{
		Oid oid;
		
		oid = get_extension_oid("pg_track_settings", true);
		if (!OidIsValid(oid))
		{
			ereport(WARNING,(errmsg("The modification of GUC parameters for system audit requires create extension pg_track_settings")));
		}
	}
	return;
}

/*
 * Function: alter_obj_type_auditstmt_actionname
 * Description: 
 * 	 Determine the type of audit statement by querying the object types in the plan tree
 * Input: object type
 * Return: Audit statement type
 *
 */
static void
alter_obj_type_auditstmt_actionname(ObjectType objtype, char *buf)
{
	assert(buf);

	switch (objtype)
	{
		case OBJECT_AGGREGATE:
		//	type = ALTER_AGGREGATE;
			break;
		case OBJECT_ATTRIBUTE:
		//	type = ALTER TYPE;
			break;
		case OBJECT_CAST:
		//	type = ALTER CAST;
			break;
		case OBJECT_COLLATION:
		//	type = ALTER COLLATION;
			break;
		case OBJECT_COLUMN:
			strcpy(buf, ALTER_TABLE_S);
			break;
		case OBJECT_CONVERSION:
			//type = ALTER CONVERSION;
			break;
		case OBJECT_DATABASE:
			strcpy(buf, ALTER_DATABASE_S);
			break;
		case OBJECT_DOMAIN:
		case OBJECT_DOMCONSTRAINT:
			strcpy(buf, ALTER_DOMAIN_S);
			break;
		case OBJECT_EXTENSION:
			strcpy(buf, ALTER_EXTENSION_S);
			break;
		case OBJECT_FDW:
		//	type = ALTER FOREIGN DATA WRAPPER;
			break;
		case OBJECT_FOREIGN_SERVER:
		//	type = ALTER SERVER;
			break;
		case OBJECT_FOREIGN_TABLE:
		//	type = ALTER FOREIGN TABLE;
			break;
		case OBJECT_FUNCTION:
			strcpy(buf, ALTER_FUNCTION_S);
			break;
		case OBJECT_INDEX:
			strcpy(buf, ALTER_INDEX_S);
			break;
		case OBJECT_LANGUAGE:
		//	type = ALTER LANGUAGE;
			break;
		case OBJECT_LARGEOBJECT:
		//	type = ALTER LARGE OBJECT;
			break;
		case OBJECT_OPCLASS:
		//	type = ALTER OPERATOR CLASS;
			break;
		case OBJECT_OPERATOR:
		//	type = ALTER_OPERATOR;
			break;
		case OBJECT_OPFAMILY:
		//	type = ALTER OPERATOR FAMILY;
			break;
		case OBJECT_POLICY:
		//	type = ALTER POLICY;
			break;
		case OBJECT_PROCEDURE:
			strcpy(buf, ALTER_PROCEDURE_S);
			break;
		case OBJECT_ROLE:
			strcpy(buf, ALTER_ROLE_S);
			break;
		case OBJECT_ROUTINE:
		//	type = ALTER_ROUTINE;
			break;
		case OBJECT_RULE:
		//	type = ALTER RULE;
			break;
		case OBJECT_SCHEMA:
			strcpy(buf, ALTER_SCHEMA_S);
			break;
		case OBJECT_SEQUENCE:
			strcpy(buf, ALTER_SEQUENCE_S);
			break;
		case OBJECT_TABLE:
		case OBJECT_TABCONSTRAINT:
			strcpy(buf, ALTER_TABLE_S);
			break;
		case OBJECT_TABLESPACE:
			strcpy(buf, ALTER_TABLESPACE_S);
			break;
		case OBJECT_TRIGGER:
			strcpy(buf, ALTER_TRIGGER_S);
			break;
		case OBJECT_EVENT_TRIGGER:
		//	type = ALTER EVENT TRIGGER;
			break;
		case OBJECT_TSCONFIGURATION:
		//	type = ALTER TEXT SEARCH CONFIGURATION;
			break;
		case OBJECT_TSDICTIONARY:
		//	type = ALTER TEXT SEARCH DICTIONARY;
			break;
		case OBJECT_TSPARSER:
		//	type = ALTER TEXT SEARCH PARSER;
			break;
		case OBJECT_TSTEMPLATE:
		//	type = ALTER TEXT SEARCH TEMPLATE;
			break;
		case OBJECT_TYPE:
		//	type = ALTER TYPE;
			break;
		case OBJECT_VIEW:
			strcpy(buf, ALTER_VIEW_S);
			break;
		case OBJECT_MATVIEW:
			strcpy(buf, ALTER_MATLIZED_VIEW_S);
			break;
		case OBJECT_PUBLICATION:
		//	type = ALTER PUBLICATION;
			break;
		case OBJECT_SUBSCRIPTION:
		//	type = ALTER SUBSCRIPTION;
			break;
		case OBJECT_STATISTIC_EXT:
		//	type = ALTER STATISTICS;
			break;
		default:
			strcpy(buf, INVALID_STMT_S);
			break;
	}

	if (strlen(buf) == 0)
		strcpy(buf, INVALID_STMT_S);
}


/*
 * Function: audit_get_actionname_from_tree
 * Description: 
 *   Determine the type of audit statement by querying the plan tree
 * Input: Query tree nodes
 * Return: Audit statement type
 *
 */
static void
audit_get_actionname_from_tree(Node *parsetree, char *buf)
{
	assert(buf);
	
	switch (nodeTag(parsetree))
	{
			/* recurse if we're given a RawStmt */
		case T_RawStmt:
			audit_get_actionname_from_tree(((RawStmt *) parsetree)->stmt, buf);
			break;

			/* raw plannable queries */
		case T_InsertStmt:
			strcpy(buf, INSERT_S);
			break;

		case T_DeleteStmt:
			strcpy(buf, DELETE_S);
			break;

		case T_UpdateStmt:
			strcpy(buf, UPDATE_S);
			break;

		case T_SelectStmt:
			strcpy(buf, SELECT_S);
			break;

			/* utility statements --- same whether raw or cooked */
		case T_TransactionStmt:
			{
				TransactionStmt *stmt = (TransactionStmt *) parsetree;

				switch (stmt->kind)
				{
					case TRANS_STMT_BEGIN:
						strcpy(buf, BEGIN_S);
						break;

					case TRANS_STMT_START:
						strcpy(buf, START_TRANSACTION_S);
						break;

					case TRANS_STMT_COMMIT:
						strcpy(buf, COMMIT_S);
						break;

					case TRANS_STMT_ROLLBACK:
						strcpy(buf, ROLLBACK_S);
						break;
						
					case TRANS_STMT_ROLLBACK_TO:
						strcpy(buf, ROLLBACK_TO_S);
						break;

					case TRANS_STMT_SAVEPOINT:
						strcpy(buf, SAVEPOINT_S);
						break;

					case TRANS_STMT_RELEASE:
						strcpy(buf, RELEASE_S);
						break;

					case TRANS_STMT_PREPARE:
				//		type = PREPARE TRANSACTION;
						break;

					case TRANS_STMT_COMMIT_PREPARED:
				//		type = COMMIT PREPARED;
						break;

					case TRANS_STMT_ROLLBACK_PREPARED:
						strcpy(buf, ROLLBACK_PREPARED_S);
						break;

					default:
						strcpy(buf, INVALID_STMT_S);
						break;
				}
			}
			break;

		case T_DeclareCursorStmt:
		//	type = DECLARE CURSOR;
			break;

		case T_ClosePortalStmt:
			{
				ClosePortalStmt *stmt = (ClosePortalStmt *) parsetree;

				if (stmt->portalname == NULL)
				//	type = CLOSE CURSOR ALL;
				;
				else
					;
				//	type = CLOSE CURSOR;
			}
			break;

		case T_FetchStmt:
			{
				//FetchStmt  *stmt = (FetchStmt *) parsetree;

			//	type = (stmt->ismove) ? MOVE : FETCH;
			}
			break;

		case T_CreateDomainStmt:
			strcpy(buf, CREATE_DOMAIN_S);
			break;
		/* Begin - BUG#Z6075 */
		case T_CreateResourceGroupStmt:
			strcpy(buf, CREATE_RESOURCE_GROUP_S);
			break;

		case T_DropResourceGroupStmt:
			strcpy(buf, DROP_RESOURCE_GROUP_S);
			break;

		case T_AlterResourceGroupStmt:
			strcpy(buf, ALTER_RESOURCE_GROUP_S);
			break;
		/* End - BUG#Z6075 */
		case T_CreateSchemaStmt:
			strcpy(buf, CREATE_SCHEMA_S);
			break;

		case T_CreateStmt:
			strcpy(buf, CREATE_TABLE_S);
			break;

		case T_CreateTableSpaceStmt:
			strcpy(buf, CREATE_TABLESPACE_S);
			break;

		case T_DropTableSpaceStmt:
			strcpy(buf, DROP_TABLESPACE_S);
			break;

		case T_AlterTableSpaceOptionsStmt:
			strcpy(buf, ALTER_TABLESPACE_S);
			break;

		case T_CreateExtensionStmt:
			strcpy(buf, CREATE_EXTENSION_S);
			break;

		case T_AlterExtensionStmt:
			strcpy(buf, ALTER_EXTENSION_S);
			break;

		case T_AlterExtensionContentsStmt:
			strcpy(buf, ALTER_EXTENSION_S);
			break;

		case T_CreateFdwStmt:
			//type = CREATE_FOREIGN DATA WRAPPER;
			break;

		case T_AlterFdwStmt:
			//type = ALTER FOREIGN DATA WRAPPER;
			break;

		case T_CreateForeignServerStmt:
			//type = CREATE_SERVER;
			break;

		case T_AlterForeignServerStmt:
			//type = ALTER_SERVER;
			break;

		case T_CreateUserMappingStmt:
			//type = CREATE_USER_MAPPING;
			break;

		case T_AlterUserMappingStmt:
			//type = ALTER_USER_MAPPING;
			break;

		case T_DropUserMappingStmt:
			//type = DROP_USER_MAPPING;
			break;

		case T_CreateForeignTableStmt:
		//	type = CREATE_FOREIGN_TABLE;
			break;

		case T_ImportForeignSchemaStmt:
		//	type = IMPORT FOREIGN SCHEMA;
			break;

		case T_DropStmt:
			switch (((DropStmt *) parsetree)->removeType)
			{
				case OBJECT_TABLE:
					strcpy(buf, DROP_TABLE_S);
					break;
				case OBJECT_SEQUENCE:
					strcpy(buf, DROP_SEQUENCE_S);
					break;
				case OBJECT_VIEW:
					strcpy(buf, DROP_VIEW_S);
					break;
				case OBJECT_MATVIEW:
					strcpy(buf, DROP_MATLIZED_VIEW_S);
					break;
				case OBJECT_INDEX:
					strcpy(buf, DROP_INDEX_S);
					break;
				case OBJECT_TYPE:
				//	type = DROP_TYPE;
					break;
				case OBJECT_DOMAIN:
					strcpy(buf, DROP_DOMAIN_S);
					break;
				case OBJECT_COLLATION:
				//	type = DROP_COLLATION;
					break;
				case OBJECT_CONVERSION:
				//	type = DROP CONVERSION;
					break;
				case OBJECT_SCHEMA:
					strcpy(buf, DROP_SCHEMA_S);
					break;
				case OBJECT_TSPARSER:
				//	type = DROP TEXT SEARCH PARSER;
					break;
				case OBJECT_TSDICTIONARY:
				//	type = DROP TEXT SEARCH DICTIONARY;
					break;
				case OBJECT_TSTEMPLATE:
				//	type = DROP TEXT SEARCH TEMPLATE;
					break;
				case OBJECT_TSCONFIGURATION:
				//	type = DROP TEXT SEARCH CONFIGURATION;
					break;
				case OBJECT_FOREIGN_TABLE:
				//	type = DROP FOREIGN TABLE;
					break;
				case OBJECT_EXTENSION:
					strcpy(buf, DROP_EXTENSION_S);
					break;
				case OBJECT_FUNCTION:
					strcpy(buf, DROP_FUNCTION_S);
					break;
				case OBJECT_PROCEDURE:
					strcpy(buf, DROP_PROCEDURE_S);
					break;
				case OBJECT_ROUTINE:
				//	type = DROP_ROUTINE;
					break;
				case OBJECT_AGGREGATE:
					//type = DROP_AGGREGATE;
					break;
				case OBJECT_OPERATOR:
					//type = DROP_OPERATOR;
					break;
				case OBJECT_LANGUAGE:
				//	type = DROP LANGUAGE;
					break;
				case OBJECT_CAST:
				//	type = DROP CAST;
					break;
				case OBJECT_TRIGGER:
					strcpy(buf, DROP_TRIGGER_S);
					break;
				case OBJECT_EVENT_TRIGGER:
				//	type = DROP EVENT TRIGGER;
					break;
				case OBJECT_RULE:
				//	type = DROP RULE;
					break;
				case OBJECT_FDW:
				//	type = DROP FOREIGN DATA WRAPPER;
					break;
				case OBJECT_FOREIGN_SERVER:
				//	type = DROP SERVER;
					break;
				case OBJECT_OPCLASS:
				//	type = DROP OPERATOR CLASS;
					break;
				case OBJECT_OPFAMILY:
				//	type = DROP OPERATOR FAMILY;
					break;
				case OBJECT_POLICY:
				//	type = DROP POLICY;
					break;
				case OBJECT_TRANSFORM:
				//	type = DROP TRANSFORM;
					break;
				case OBJECT_ACCESS_METHOD:
				//	type = DROP ACCESS METHOD;
					break;
				case OBJECT_PUBLICATION:
				//	type = DROP PUBLICATION;
					break;
				case OBJECT_STATISTIC_EXT:
				//	type = DROP STATISTICS;
					break;
				default:
					strcpy(buf, INVALID_STMT_S);
					break;
			}
			break;

		case T_TruncateStmt:
			strcpy(buf, TRUNCATE_S);
			break;

		case T_CommentStmt:
			strcpy(buf, COMMENT_S);
			break;

		case T_SecLabelStmt:
		//	type = SECURITY_LABEL;
			break;

		case T_CopyStmt:
			{
				CopyStmt *stmt = (CopyStmt *) parsetree;
				if(stmt->is_from)
					strcpy(buf, COPY_FROM_S);
				else
					strcpy(buf, COPY_TO_S);
			}
			break;

		case T_RenameStmt:
			alter_obj_type_auditstmt_actionname(((RenameStmt *) parsetree)->renameType, buf);
			break;

		case T_AlterObjectDependsStmt:
			alter_obj_type_auditstmt_actionname(((AlterObjectDependsStmt *) parsetree)->objectType, buf);
			break;

		case T_AlterObjectSchemaStmt:
			alter_obj_type_auditstmt_actionname(((AlterObjectSchemaStmt *) parsetree)->objectType, buf);
			break;

		case T_AlterOwnerStmt:
			alter_obj_type_auditstmt_actionname(((AlterOwnerStmt *) parsetree)->objectType, buf);
			break;

		case T_AlterTableMoveAllStmt:
			alter_obj_type_auditstmt_actionname(((AlterTableMoveAllStmt *) parsetree)->objtype, buf);
			break;

		case T_AlterTableStmt:
			alter_obj_type_auditstmt_actionname(((AlterTableStmt *) parsetree)->objtype, buf);
			break;

		case T_AlterDomainStmt:
			strcpy(buf, ALTER_DOMAIN_S);
			break;

		case T_AlterFunctionStmt:
			switch (((AlterFunctionStmt *) parsetree)->objtype)
			{
				case OBJECT_FUNCTION:
					strcpy(buf, ALTER_FUNCTION_S);
					break;
				case OBJECT_PROCEDURE:
					strcpy(buf, ALTER_PROCEDURE_S);
					break;
				case OBJECT_ROUTINE:
				//	type = ALTER ROUTINE;
					break;
				default:
					strcpy(buf, INVALID_STMT_S);
					break;
			}
			break;

		case T_GrantStmt:
			{
				GrantStmt  *stmt = (GrantStmt *) parsetree;

				if (stmt->is_grant)
					strcpy(buf, GRANT_S);
				else
					strcpy(buf, REVOKE_S);

			}
			break;

		case T_GrantRoleStmt:
			{
				GrantRoleStmt *stmt = (GrantRoleStmt *) parsetree;

				if (stmt->is_grant)
					strcpy(buf, GRANT_S);
				else
					strcpy(buf, REVOKE_S);
			}
			break;

		case T_AlterDefaultPrivilegesStmt:
		//	type = ALTER DEFAULT PRIVILEGES;
			break;

		case T_DefineStmt:
			switch (((DefineStmt *) parsetree)->kind)
			{
				case OBJECT_AGGREGATE:
			//		type = CREATE_AGGREGATE;
					break;
				case OBJECT_OPERATOR:
			//		type = CREATE OPERATOR;
					break;
				case OBJECT_TYPE:
			//		type = CREATE TYPE;
					break;
				case OBJECT_TSPARSER:
				//	type = CREATE TEXT SEARCH PARSER;
					break;
				case OBJECT_TSDICTIONARY:
				//	type = CREATE TEXT SEARCH DICTIONARY;
					break;
				case OBJECT_TSTEMPLATE:
				//	type = CREATE TEXT SEARCH TEMPLATE;
					break;
				case OBJECT_TSCONFIGURATION:
				//	type = CREATE TEXT SEARCH CONFIGURATION;
					break;
				case OBJECT_COLLATION:
				//	type = CREATE COLLATION;
					break;
				case OBJECT_ACCESS_METHOD:
				//	type = CREATE ACCESS METHOD;
					break;
				default:
					strcpy(buf, INVALID_STMT_S);
					break;
			}
			break;

		case T_CompositeTypeStmt:
		//	type = CREATE_TYPE;
			break;

		case T_CreateEnumStmt:
		//	type = CREATE TYPE;
			break;

		case T_CreateRangeStmt:
		//	type = CREATE TYPE;
			break;

		case T_AlterEnumStmt:
		//	type = ALTER TYPE;
			break;

		case T_ViewStmt:
			strcpy(buf, CREATE_VIEW_S);
			break;

		case T_CreateFunctionStmt:
			if (((CreateFunctionStmt *) parsetree)->is_procedure)
				strcpy(buf, CREATE_PROCEDURE_S);
			else
				strcpy(buf, CREATE_FUNCTION_S);
			break;

		case T_IndexStmt:
			strcpy(buf, CREATE_INDEX_S);
			break;

		case T_RuleStmt:
			//type = CREATE_RULE;
			break;

		case T_CreateSeqStmt:
			strcpy(buf, CREATE_SEQUENCE_S);
			break;

		case T_AlterSeqStmt:
			strcpy(buf, ALTER_SEQUENCE_S);
			break;

		case T_DoStmt:
		//	type = DO;
			break;

		case T_CreatedbStmt:
			strcpy(buf, CREATE_DATABASE_S);
			break;

		case T_AlterDatabaseStmt:
			strcpy(buf, ALTER_DATABASE_S);
			break;

		case T_AlterDatabaseSetStmt:
			strcpy(buf, ALTER_DATABASE_S);
			break;

		case T_DropdbStmt:
			strcpy(buf, DROP_DATABASE_S);
			break;

		case T_NotifyStmt:
			//type = NOTIFY;
			break;

		case T_ListenStmt:
			//type = LISTEN;
			break;

		case T_UnlistenStmt:
			//type = UNLISTEN;
			break;

		case T_LoadStmt:
			//type = LOAD;
			break;

		case T_CallStmt:
			strcpy(buf, CALL_S);
			break;

		case T_ClusterStmt:
		//	type = CLUSTER;
			break;

		case T_VacuumStmt:
		//	if (((VacuumStmt *) parsetree)->is_vacuumcmd)
		//		type = VACUUM;
		//	else
		//		type = ANALYZE;
			break;

		case T_ExplainStmt:
			//type = EXPLAIN;
			break;

		case T_CreateTableAsStmt:
			switch (((CreateTableAsStmt *) parsetree)->objtype)
			{
				case OBJECT_TABLE:
					if (((CreateTableAsStmt *) parsetree)->is_select_into)
						strcpy(buf, SELECT_INTO_S);
					else
						strcpy(buf, CREATE_TABLE_AS_S);
					break;
				case OBJECT_MATVIEW:
					strcpy(buf, CREATE_MATLIZED_VIEW_S);
					break;
				default:
					strcpy(buf, INVALID_STMT_S);
					break;
			}
			break;

		case T_RefreshMatViewStmt:
		//	type = REFRESH MATERIALIZED VIEW;
			break;

		case T_AlterSystemStmt:
			strcpy(buf, ALTER_SYSTEM_S);
			break;

		case T_VariableSetStmt:
			switch (((VariableSetStmt *) parsetree)->kind)
			{
				case VAR_SET_VALUE:
				case VAR_SET_CURRENT:
				case VAR_SET_DEFAULT:
				case VAR_SET_MULTI:
					strcpy(buf, SET_S);
					break;
				case VAR_RESET:
				case VAR_RESET_ALL:
					strcpy(buf, RESET_S);
					break;
				default:
					strcpy(buf, INVALID_STMT_S);
					break;
			}
			break;

		case T_VariableShowStmt:
		//	type = SHOW;
			break;

		case T_DiscardStmt:
			switch (((DiscardStmt *) parsetree)->target)
			{
				case DISCARD_ALL:
				//	type = DISCARD ALL;
					break;
				case DISCARD_PLANS:
				//	type = DISCARD PLANS;
					break;
				case DISCARD_TEMP:
				//	type = DISCARD TEMP;
					break;
				case DISCARD_SEQUENCES:
				//	type = DISCARD SEQUENCES;
					break;
				default:
					strcpy(buf, INVALID_STMT_S);
					break;
			}
			break;

		case T_CreateTransformStmt:
		//	type = CREATE TRANSFORM;
			break;

		case T_CreateTrigStmt:
			strcpy(buf, CREATE_TRIGGER_S);
			break;

		case T_CreateEventTrigStmt:
		//	type = CREATE EVENT TRIGGER;
			break;

		case T_AlterEventTrigStmt:
		//	type = ALTER EVENT TRIGGER;
			break;

		case T_CreatePLangStmt:
		//	type = CREATE LANGUAGE;
			break;

		case T_CreateRoleStmt:
			if (((CreateRoleStmt *)parsetree)->stmt_type == ROLESTMT_ROLE)
				strcpy(buf, CREATE_ROLE_S);
			else
				strcpy(buf, CREATE_USER_S);
			break;

		case T_AlterRoleStmt:
			strcpy(buf, ALTER_ROLE_S);
			break;

		case T_AlterRoleSetStmt:
			strcpy(buf, ALTER_ROLE_S);
			break;

		case T_DropRoleStmt:
			strcpy(buf, DROP_ROLE_S);
			break;

		case T_DropOwnedStmt:
		//	type = DROP_OWNED;
			break;

		case T_ReassignOwnedStmt:
		//	type = REASSIGN OWNED;
			break;

		case T_LockStmt:
			strcpy(buf, LOCK_S);
			break;

		case T_ConstraintsSetStmt:
		//	type = SET CONSTRAINTS;
			break;

		case T_CheckPointStmt:
		//	type = CHECKPOINT;
			break;

		case T_ReindexStmt:
			strcpy(buf, REINDEX_S);
			break;

		case T_CreateConversionStmt:
		//	type = CREATE_CONVERSION;
			break;

		case T_CreateCastStmt:
		//	type = CREATE CAST;
			break;

		case T_CreateOpClassStmt:
		//	type = CREATE OPERATOR CLASS;
			break;

		case T_CreateOpFamilyStmt:
		//	type = CREATE OPERATOR FAMILY;
			break;

		case T_AlterOpFamilyStmt:
		//	type = ALTER OPERATOR FAMILY;
			break;

		case T_AlterOperatorStmt:
		//	type = ALTER OPERATOR;
			break;

		case T_AlterTSDictionaryStmt:
		//	type = ALTER TEXT SEARCH DICTIONARY;
			break;

		case T_AlterTSConfigurationStmt:
		//	type = ALTER TEXT SEARCH CONFIGURATION;
			break;

		case T_CreatePolicyStmt:
		//	type = CREATE POLICY;
			break;

		case T_AlterPolicyStmt:
		//	type = ALTER POLICY;
			break;

		case T_CreateAmStmt:
		//	type = CREATE ACCESS METHOD;
			break;

		case T_CreatePublicationStmt:
		//	type = CREATE PUBLICATION;
			break;

		case T_AlterPublicationStmt:
		//	type = ALTER PUBLICATION;
			break;

		case T_CreateSubscriptionStmt:
		//	type = CREATE SUBSCRIPTION;
			break;

		case T_AlterSubscriptionStmt:
		//	type = ALTER SUBSCRIPTION;
			break;

		case T_DropSubscriptionStmt:
		//	type = DROP SUBSCRIPTION;
			break;

		case T_AlterCollationStmt:
		//	type = ALTER COLLATION;
			break;

		case T_PrepareStmt:
		//	type = PREPARE;
			break;

		case T_ExecuteStmt:
		//	type = EXECUTE;
			break;

		case T_CreateStatsStmt:
		//	type = CREATE STATISTICS;
			break;

		case T_DeallocateStmt:
			{
				DeallocateStmt *stmt = (DeallocateStmt *) parsetree;

				if (stmt->name == NULL)
					;
			//		type = DEALLOCATE ALL;
				else
					;
			//		type = DEALLOCATE;
			}
			break;

			/* already-planned queries */
		case T_PlannedStmt:
			{
				PlannedStmt *stmt = (PlannedStmt *) parsetree;

				switch (stmt->commandType)
				{
					case CMD_SELECT:

						/*
						 * We take a little extra care here so that the result
						 * will be useful for complaints about read-only
						 * statements
						 */
						if (stmt->rowMarks != NIL)
						{
							/* not 100% but probably close enough */
							switch (((PlanRowMark *) linitial(stmt->rowMarks))->strength)
							{
//								case LCS_FORKEYSHARE:
//									type = SELECT FOR KEY SHARE;
//									break;
//								case LCS_FORSHARE:
//									type = SELECT FOR SHARE;
//									break;
//								case LCS_FORNOKEYUPDATE:
//									type = SELECT FOR NO KEY UPDATE;
//									break;
//								case LCS_FORUPDATE:
//									type = SELECT FOR UPDATE;
//									break;
								default:
									strcpy(buf, SELECT_S);
									break;
							}
						}
						else
							strcpy(buf, SELECT_S);
						break;
					case CMD_UPDATE:
						strcpy(buf, UPDATE_S);
						break;
					case CMD_INSERT:
						strcpy(buf, INSERT_S);
						break;
					case CMD_DELETE:
						strcpy(buf, DELETE_S);
						break;
					case CMD_UTILITY:
						audit_get_actionname_from_tree(stmt->utilityStmt, buf);
						break;
					default:
						elog(WARNING, "unrecognized commandType: %d",
							 (int) stmt->commandType);
						strcpy(buf, INVALID_STMT_S);
						break;
				}
			}
			break;

			/* parsed-and-rewritten-but-not-planned queries */
		case T_Query:
			{
				Query	   *stmt = (Query *) parsetree;

				switch (stmt->commandType)
				{
					case CMD_SELECT:

						/*
						 * We take a little extra care here so that the result
						 * will be useful for complaints about read-only
						 * statements
						 */
//						if (stmt->rowMarks != NIL)
//						{
//							/* not 100% but probably close enough */
//							switch (((RowMarkClause *) linitial(stmt->rowMarks))->strength)
//							{
//								case LCS_FORKEYSHARE:
//									type = SELECT FOR KEY SHARE;
//									break;
//								case LCS_FORSHARE:
//									type = SELECT FOR SHARE;
//									break;
//								case LCS_FORNOKEYUPDATE:
//									type = SELECT FOR NO KEY UPDATE;
//									break;
//								case LCS_FORUPDATE:
//									type = SELECT FOR UPDATE;
//									break;
//								default:
//									type = ???;
//									break;
//							}
//						}
//						else
							strcpy(buf, SELECT_S);
						break;
					case CMD_UPDATE:
						strcpy(buf, UPDATE_S);
						break;
					case CMD_INSERT:
						strcpy(buf, INSERT_S);
						break;
					case CMD_DELETE:
						strcpy(buf, DELETE_S);
						break;
					case CMD_UTILITY:
						audit_get_actionname_from_tree(stmt->utilityStmt, buf);
						break;
					default:
						elog(WARNING, "unrecognized commandType: %d",
							 (int) stmt->commandType);
						strcpy(buf, INVALID_STMT_S);
						break;
				}
			}
			break;
		case T_AlterTypeStmt:
			break;
		case T_AlterStatsStmt:
			break;

//		case T_AtVariableSet:	/* added by luotao on 2021/3/29 */
//			break;
//		case T_CreateRedactionPolicyStmt:
//		case T_DropRedactionPolicyStmt:
//		case T_AlterRedactionPolicyStmt:
//			break;
		/* Begin - BUG#Z5791 */
		case T_CreateSynonymStmt:
			break;
		case T_AlterSynonymStmt:
			break;
		case T_DropSynStmt:
			break;
		/* End - BUG#Z5791 */
		default:
			elog(WARNING, "unrecognized node type: %d",
				 (int) nodeTag(parsetree));
			strcpy(buf, INVALID_STMT_S);
			break;
	}

	if (strlen(buf) == 0)
		strcpy(buf, INVALID_STMT_S);
}


/*
 * Function: audit_ProcessUtility_hook
 * Description: Obtain DDL related information
 */
static void 
audit_ProcessUtility_hook(PlannedStmt *pstmt,
										const char *queryString,
										bool readOnlyTree,
										ProcessUtilityContext context,
										ParamListInfo params,
										QueryEnvironment *queryEnv,
										DestReceiver *dest,
										QueryCompletion *completionTag)
{
	AuditEventStackItem *stackItem = NULL;
	MemoryContext contextOld;
	char event[32] = {'\0'};

	audit_check_pg_track_settings(pstmt->utilityStmt);

	if (audit_GUC.audit_enable == AUDIT_ON)
	{
		audit_get_actionname_from_tree(pstmt->utilityStmt, event);

		if (!internalStatement
				&& (!IsAbortedTransactionBlockState()|| (IsAbortedTransactionBlockState()
				&& (pg_strcasecmp(event,COMMIT_S)== 0 || pg_strcasecmp(event,ROLLBACK_S)== 0))))
		{
			stackItem = stack_push();

			stackItem->auditEvent->useroid = GetUserId();
			stackItem->auditEvent->audittime = operstarttime;
			stackItem->auditEvent->sessionid = MyProcPid;
			stackItem->auditEvent->commandid = GetCurrentCommandId(false);
			audit_get_actionname_from_tree(pstmt->utilityStmt, stackItem->auditEvent->actionname);
			stackItem->auditEvent->conftypes[0] = AUDIT_TYPE_INVALID;
			
			if (nodeTag(pstmt->utilityStmt) == T_AlterSystemStmt)
			{
				AlterSystemStmt *stmt = (AlterSystemStmt *) pstmt->utilityStmt;
				
				if (stmt->setstmt->kind == VAR_SET_VALUE)
				{
					VariableSetStmt *set = stmt->setstmt;
					
					contextOld = MemoryContextSwitchTo(stackItem->auditEvent->auditContext);

					stackItem->auditEvent->setname = pnstrdup(set->name, strlen(set->name));

					if (set->args != NIL)
					{
						Node *arg = (Node *)linitial(set->args);

						if (IsA(arg, A_Const))
						{
							A_Const *con = (A_Const *)arg;

							if (con->val.type == T_String)
							{
								stackItem->auditEvent->setvalue = pnstrdup(con->val.val.str, strlen(con->val.val.str));
							}
							else if (con->val.type == T_Integer)
							{
								char	val[MAXPGPATH] = {0};
								
								sprintf(val, "%d", con->val.val.ival);
								stackItem->auditEvent->setvalue = pnstrdup(val, strlen(val)); /* BUG#Z6107 */
							}
							else if (con->val.type == T_Float)
							{
								stackItem->auditEvent->setvalue = pnstrdup(con->val.val.str, strlen(con->val.val.str));
							}
						}
					}

					MemoryContextSwitchTo(contextOld);
				}
			}

			contextOld = MemoryContextSwitchTo(stackItem->auditEvent->auditContext);

			/* For excessively long SQL statements, truncation processing is used, 
			 * with a maximum of the first 1K lengths saved per SQL statement
			 */
			stackItem->auditEvent->sqltext = pnstrdup(queryString, 1023);

			MemoryContextSwitchTo(contextOld);

			//audit_check_statement(AUDITACTIONNAMEAUDITTYPEID, stackItem);

		}
	}

	/* Call the standard process utility chain. */
	if (next_ProcessUtility_hook)
		(*next_ProcessUtility_hook) (pstmt, queryString, readOnlyTree,context, params,
									queryEnv, dest, completionTag);
	else
		standard_ProcessUtility(pstmt, queryString, readOnlyTree, context, params,
								queryEnv, dest, completionTag);

}


/*
 * Function: log_select_dml_col
 * Description: Get SELECT/DML column object information
 * Input: object list:rangeTabls
 *
 */
static void 
log_select_dml_col(RangeTblEntry *rte, Oid relOid, Relation rel)
{
	Bitmapset  *columns = NULL;
	Bitmapset  *columns_bak = NULL;
	AttrNumber	col;
	char	   *colname = NULL;
	AuditStmtType type = INVALID_STMT;

	//if (auditEventStack->auditEvent->conftypes[num] != AUDIT_TYPE_STATEMENT && auditEventStack->auditEvent->conftypes[num] != AUDIT_TYPE_PRIVILEGE
	//	 && auditEventStack->auditEvent->conftypes[num] != AUDIT_TYPE_USER)
	{
		type = audit_get_stmt_type(auditEventStack->auditEvent->actionname, false, -1);
		/* Get column names */
		switch (type)
		{
			case INSERT_T:
				columns = bms_copy(rte->insertedCols);
				break;
			case UPDATE_T:
				columns = bms_copy(rte->updatedCols);
				break;
			case SELECT_T:
				columns = bms_copy(rte->selectedCols);
				break;
			default:
				columns = NULL;
		}

		if (columns)
		{
			columns_bak = bms_copy(columns);
		}

		/* Retrieve the columns' position in which column */
		while ((col = bms_first_member(columns)) >= 0)
		{
			col += FirstLowInvalidHeapAttributeNumber;

			if (col != InvalidAttrNumber)
			{
				HeapTuple attTuple;

				attTuple = SearchSysCache2(ATTNUM,
											 ObjectIdGetDatum(relOid),
											 Int16GetDatum(col));
				if (HeapTupleIsValid(attTuple))
				{
					Form_pg_attribute attributeForm = (Form_pg_attribute) GETSTRUCT(attTuple);

					if (!attributeForm->attisdropped)
					{
						colname = pstrdup(NameStr(attributeForm->attname));
					}
					ReleaseSysCache(attTuple);
				}

				/* Save column audit */
				if (colname)
				{
					AuditEvent_Obj *obj1 = palloc0(sizeof(AuditEvent_Obj));

					obj1->classId = AttributeRelationId;
					obj1->objectId = relOid;
					obj1->schemaId = RelationGetNamespace(rel);
					obj1->schemaName = get_namespace_name(RelationGetNamespace(rel));
					obj1->objectName = pstrdup(RelationGetRelationName(rel));
					obj1->colname = colname;
					obj1->attnum = col;
					obj1->objectClassdesc = audit_get_objname(OBJECT_COLUMN);
					if (columns_bak)
					{
						obj1->columns = bms_copy(columns_bak);
					}
					
					auditEventStack->auditEvent->objects = lappend(auditEventStack->auditEvent->objects, obj1);
					colname = NULL;
				}
			}
		}
	}
}


/*
 * check for duplicate objects
 */
static bool 
audit_check_repeat_obj(AuditEvent *auditEvent, char *schemaName, char *objectName, AttrNumber attnum)
{
	List *objects = NULL;
	ListCell *cell;

	if (auditEvent == NULL || auditEvent->objects == NULL)
		return false;

	objects = auditEvent->objects;
	foreach(cell, objects)
	{
		AuditEvent_Obj *obj = lfirst(cell);

		if (attnum == InvalidAttrNumber)
		{
			if (strcmp(obj->schemaName, schemaName) == 0 && strcmp(obj->objectName, objectName) == 0)
				return true;
		}
	}
	return false;
}

/*
 * Function: log_select_dml
 * Description: Retrieve object information from SELECT/DML statements
 * Input: object list:rangeTabls
 *
 */
static void 
log_select_dml(List *rangeTabls)
{
	ListCell *lr;
	MemoryContext contextOld;

	/* Do not log if this is an internal statement */
	if (internalStatement)
		return;

	if (!auditEventStack)
		return;
	
	/* Check the objects contained in the current statement to obtain relevant information */
	foreach (lr, rangeTabls)
	{
		Oid relOid;
		Relation rel = NULL;
		RangeTblEntry *rte = lfirst(lr);
		AuditEvent_Obj *obj;
		Oid		obj_class;
		ObjectType objtype = InvalidObjType;

		/* We only care about tables/views, and can ignore subqueries etc. */
		if (rte->rtekind != RTE_RELATION)
			continue;

		relOid = rte->relid;
		rel = relation_open(relOid, AccessShareLock);

		if (PG_CATALOG_NAMESPACE == (RelationGetNamespace(rel)))
		{
			relation_close(rel, AccessShareLock);
			continue;
		}

		if (audit_check_repeat_obj(auditEventStack->auditEvent, get_namespace_name(RelationGetNamespace(rel)), 
				pstrdup(RelationGetRelationName(rel)), InvalidAttrNumber))
		{
			relation_close(rel, AccessShareLock);
			continue;
		}
			
		switch(rel->rd_rel->relkind)
		{
			case RELKIND_RELATION:
			case RELKIND_TOASTVALUE:
			case RELKIND_FOREIGN_TABLE:
			case RELKIND_PARTITIONED_TABLE:
				objtype = OBJECT_TABLE;
				obj_class = RelationRelationId;
				break;
			case RELKIND_VIEW:
			case RELKIND_MATVIEW:
				objtype = OBJECT_VIEW;
				obj_class = RelationRelationId;
				break;
			case RELKIND_SEQUENCE :
				objtype = OBJECT_SEQUENCE;
				obj_class = SequenceRelationId;
				break;
			default:
				objtype = InvalidObjType;
				obj_class = InvalidOid;
				break;
		}

		if (obj_class == InvalidOid)
			continue;

		contextOld = MemoryContextSwitchTo(auditEventStack->auditEvent->auditContext);

		obj = palloc0(sizeof(AuditEvent_Obj));
		obj->classId = obj_class;
		obj->objectId = relOid;
		obj->schemaId = RelationGetNamespace(rel);
		obj->schemaName = get_namespace_name(RelationGetNamespace(rel));
		obj->objectName = pstrdup(RelationGetRelationName(rel));
		obj->objectClassdesc = audit_get_objname(objtype);
		auditEventStack->auditEvent->objects = lappend(auditEventStack->auditEvent->objects, obj);
		
		/* Column Audit */
		log_select_dml_col(rte, relOid, rel);

		MemoryContextSwitchTo(contextOld);

		relation_close(rel, AccessShareLock);
		
	}

	return;
}

/*
 * register the backend precess to perform refresh pg_trach_settings_snapshot
 * and start pgagent tasks when the database starts
 */
static void
audit_register_br(void)
{
	BackgroundWorker worker;

	if (IsBinaryUpgrade)
		return;
#if 0
	if (audit_GUC.audit_enable != AUDIT_ON || 
		(audit_GUC.audit_systemopers == AUDIT_OPER_OFF &&
		 (audit_GUC.audit_archdir == NULL || strlen(audit_GUC.audit_archdir) <= 0)))
	{
		return;
	}
#endif
	
	memset(&worker, 0, sizeof(worker));
	worker.bgw_flags = BGWORKER_SHMEM_ACCESS | BGWORKER_BACKEND_DATABASE_CONNECTION;
	worker.bgw_start_time = BgWorkerStart_RecoveryFinished;
	/* BGW_NEVER_RESTART connect once */
	worker.bgw_restart_time = 5;
	snprintf(worker.bgw_library_name, BGW_MAXLEN, "pg_audit");
	snprintf(worker.bgw_function_name, BGW_MAXLEN, "pg_audit_launch");
	worker.bgw_notify_pid = 0;
	snprintf(worker.bgw_type, BGW_MAXLEN, "pg_audit_launch");
	
	/*
	 * Now fill in worker-specific data, and do the actual registrations.
	 */
	snprintf(worker.bgw_name, BGW_MAXLEN, AUDIT_LAUNCHER_S);
	RegisterBackgroundWorker(&worker);
}

/*
 * Execute the function pg_track_settings_snapshot
 * to refresh the modification of the guc parameter
 * to the relevant table
 */
static void
execute_snapshot(void)
{
	char	sqltext[MAXPGPATH] = {0};
	int		ret;

	memset(sqltext, 0, MAXPGPATH);
	sprintf(sqltext, "select * from hg_security.pg_track_settings_snapshot()");

	internalStatement = true;

	StartTransactionCommand();

	/* Connect to SPI manager */
	if ((ret = SPI_connect()) < 0)
	{
		/* internal error */
		elog(WARNING, "audit_alter_set_guc: SPI_connect returned %d", ret);
		return;
	}

	PushActiveSnapshot(GetTransactionSnapshot());
	
	/* Now retrieve the crosstab source rows */
	ret = SPI_execute(sqltext, false, 0);

	/* Check for qualifying tuples */
	if (ret != SPI_OK_SELECT)
	{
		elog(WARNING, "audit_alter_set_guc: SPI_execute returned %d", ret);
		return;
	}

	SPI_finish();
	PopActiveSnapshot();
	CommitTransactionCommand();

	internalStatement = false;

}

static void audit_hghac_sync_guc(Relation	audit_log, AuditEvent *auditevent)
{
	AuditEvent auditevent_guc;

	if (MyProcPort && MyProcPort->application_name && pg_strcasecmp(MyProcPort->application_name, "Patroni ctl") == 0
		&& pg_strcasecmp(auditevent->actionname, ALTER_SYSTEM_S) == 0)
	{
		memcpy(&auditevent_guc, auditevent, sizeof(AuditEvent));
		strcpy(auditevent_guc.actionname, SYSTEMAUDIT_HGHAC_SYNC_GUC_S);
		auditevent_guc.confoids[0] = 0;
		auditevent_guc.sqltext = pnstrdup(auditevent->sqltext, 1023);

		audit_insert_all_log(audit_log, &auditevent_guc, NULL, -1);
	}
}

/*
 * real time effective modification of guc parameters
 */
static void
audit_alter_set_guc(List *parsetreeList)
{
	ListCell   *parsetree_item;
	char	sqltext[MAXPGPATH] = {0};
	int		ret;
	uint64	proc;
	Oid		relOid;
	Relation	audit_log;
	TimestampTz 	audittime;
	const char	*time;
	char	timec[20];

	memset(sqltext, 0, MAXPGPATH);
	sprintf(sqltext, "select * from hg_security.pg_track_settings_snapshot()");

	foreach(parsetree_item, parsetreeList)
	{
		RawStmt    *parsetree = lfirst_node(RawStmt, parsetree_item);

#if 0
		if (IsA(parsetree->stmt, AlterSystemStmt))
		{
			AlterSystemStmt  *stmt = (AlterSystemStmt *) parsetree->stmt;
			
				if (stmt->setstmt->kind == VAR_SET_VALUE &&
					strcmp(stmt->setstmt->name, "audit.systemopers") == 0)
				{
					ListCell   *cell;
					VariableSetStmt *set_enable = stmt->setstmt;

					
					foreach(cell, set_enable->args)
					{
						A_Const *grantee = (A_Const *) lfirst(cell);
						
						if (pg_strcasecmp(grantee->val.val.str, "off") == 0)
						{
							continue;
			
						}
						else
						{
							execute_snapshot();
						}
					}
				}
		}
#endif

		if (audit_GUC.audit_enable != AUDIT_ON || 
			audit_GUC.audit_systemopers == AUDIT_OPER_OFF)
			return;
		
		if (parsetree->stmt->type != T_AlterDatabaseSetStmt && 
			parsetree->stmt->type != T_AlterRoleSetStmt)
			continue;
		
		execute_snapshot();
		audittime = GetCurrentTimestamp();
	    time = timestamptz_to_str(audittime);
		strncpy(timec, time, 19);
		timec[19] = '\0';

		internalStatement = true;

	#if defined(USE_SQLITE) && defined(ENABLE_DKY_BETA_FEATURE)
		if (!check_disk_size())
			return ;
	#endif

		StartTransactionCommand();

		/* Connect to SPI manager */
		if ((ret = SPI_connect()) < 0)
		{
			/* internal error */
			elog(WARNING, "audit_alter_set_guc: SPI_connect returned %d", ret);
			internalStatement = false;
			return;
		}

		memset(sqltext, 0, MAXPGPATH);
		
		sprintf(sqltext, "SELECT ts, name, setting "
	       				 "FROM hg_security.pg_track_db_role_settings_history");
		

		PushActiveSnapshot(GetTransactionSnapshot());
		
		ret = SPI_execute(sqltext, true, 0);
		
		/* Check for qualifying tuples */
		if (ret != SPI_OK_SELECT)
		{
			elog(WARNING, "audit_alter_set_guc: SPI_execute returned %d", ret);
			internalStatement = false;
			return;
		}

		proc = SPI_processed;

		if (proc > 0)
		{
			SPITupleTable *spi_tuptable = SPI_tuptable;
			TupleDesc	spi_tupdesc = spi_tuptable->tupdesc;
			uint64		i;
			
			relOid = get_relname_relid(PG_AUDIT_LOG, SECURITY_SCHEMA_NAMESPACE);
			if (relOid == InvalidOid)
			{
				elog(WARNING, "could not get oid of relation pg_audit_log");
				return;
			}

			audit_log = table_open(relOid, RowExclusiveLock);
			
			for (i = 0; i < proc; i++)
			{
				HeapTuple	spi_tuple;
				char	   *ts;
				char	    tsc[20];
				char	   *name;
				char	   *setting;
				AuditEvent *auditevent = NULL;
				int			len;
				char	   *sql = palloc0(MAXPGPATH);

				
				len =  INTALIGN(sizeof(AuditEvent));
				auditevent = palloc0(len);

				spi_tuple = spi_tuptable->vals[i];
				
				ts = SPI_getvalue(spi_tuple, spi_tupdesc, 1);

				memset(tsc, 0, 20);
				strncpy(tsc, ts, 19);
				tsc[19] = '\0';

				if (strcmp(timec, tsc) != 0)
					continue;
				
				name = SPI_getvalue(spi_tuple, spi_tupdesc, 2);
				setting = SPI_getvalue(spi_tuple, spi_tupdesc, 3);

				strcpy(auditevent->actionname, ALTER_GUC_S);
				auditevent->audittime = DatumGetTimestampTz(DirectFunctionCall3(timestamptz_in,
																	 CStringGetDatum(ts),
																	 ObjectIdGetDatum(InvalidOid),
																	 Int32GetDatum(-1)));
				auditevent->useroid = GetUserId_audit();
				auditevent->privilege = AUDIT_PRIVILEGE_INSTACE;
				auditevent->sessionid = MyProcPid;
				auditevent->commandid = GetCurrentCommandId(false);
				auditevent->confoids[0] = 0;

				sprintf(sql, "set %s='%s'", name, setting);
				auditevent->sqltext = pnstrdup(sql, 1023);
				auditevent->result = AUDIT_SUCCESS;

				audit_insert_all_log(audit_log, auditevent, NULL, -1);
			}

			table_close(audit_log, RowExclusiveLock);
			audit_log = NULL;
		}
		
		SPI_finish();

		PopActiveSnapshot();
		CommitTransactionCommand();

		internalStatement = false;
	}
	
	return;
}
	
/* Not used, can be deleted later */
static void
audit_set_server_option(char *gucname, char *gucvalue)
{
	Relation	rel;
	HeapTuple	tp;
	HeapTuple	newtp;
	Oid			serverid;
	Datum		repl_val[Natts_pg_foreign_server];
	bool		repl_null[Natts_pg_foreign_server];
	bool		repl_repl[Natts_pg_foreign_server];
	char       *option = NULL;
	char	   *defname = "database";
	int 		len;
	Datum	   *srvoptions = (Datum *)palloc(sizeof(Datum));
	ArrayType   *array;

	if (pg_strcasecmp(gucname, "audit.sqlitedbpath") != 0)
		return;

	if (RecoveryInProgress())
		return;
	
	serverid = get_foreign_server_oid("sqlite_server", true);

	if (!OidIsValid(serverid))
	{
		elog(WARNING, "server sqlite_server does not exist");
		return;
	}

	memset(repl_val, 0, sizeof(repl_val));
	memset(repl_null, false, sizeof(repl_null));
	memset(repl_repl, false, sizeof(repl_repl));

	rel = table_open(ForeignServerRelationId, RowExclusiveLock);

	tp = SearchSysCache1(FOREIGNSERVEROID, ObjectIdGetDatum(serverid));

	if (!HeapTupleIsValid(tp))
	{
		elog(WARNING, "cache lookup failed for foreign server %u", serverid);
		return;
	}


	len = strlen(defname) + 1 + strlen(gucvalue);
	option = palloc(len + 1);
	sprintf(option, "%s=%s", defname, gucvalue);

	srvoptions[0] = CStringGetTextDatum(option);
	array = construct_array(srvoptions, 1, TEXTOID, -1, false, 'i');
	
	repl_val[Anum_pg_foreign_server_srvoptions - 1] = PointerGetDatum(array);
	repl_null[Anum_pg_foreign_server_srvoptions - 1] = false;
	repl_repl[Anum_pg_foreign_server_srvoptions - 1] = true;
	
	newtp = heap_modify_tuple(tp, RelationGetDescr(rel),
						   repl_val, repl_null, repl_repl);

	CatalogTupleUpdate(rel, &tp->t_self, newtp);

	heap_freetuple(newtp);

	table_close(rel, RowExclusiveLock);

	if (HeapTupleIsValid(tp))
		ReleaseCatCache(tp);
	
	return;
	
}

/*
 * When the database stop, the precess of pgagent needs to be stopped
 */
static void
audit_stop_pgagent(void)
{
	char		cmd[MAXPGPATH] = {0};
	int			rc;

	if (audit_GUC.audit_archdir == NULL || strlen(audit_GUC.audit_archdir) <= 0)
		return;

/* Begin - BUG#Z6180 */
#ifdef USE_SQLITE
	/* Begin - ReqID:SRS-SECURITY-AUDIT */
	if (sqlite_open_db_for_audit_hook)
		sqlite_open_db_for_audit_hook(SYSTEMAUDIT_STOP_PGAGENT, AUDIT_SUCCESS, NULL, NULL);
	/* End - ReqID:SRS-SECURITY-AUDIT */
#endif
/* End - BUG#Z6180 */

	memset(cmd, 0, MAXPGPATH);
	sprintf(cmd, "ps -ef | grep pgagent | grep -v grep > /dev/null");
	rc = system(cmd);

	if (rc != 0)
		return;
	
	memset(cmd, 0, MAXPGPATH);
	sprintf(cmd, "kill -9 `ps -ef|grep pgagent|grep -v grep|awk '{print $2}'`");
	rc = system(cmd);
	if (rc != 0)
		return;
}

/* Not used, can be deleted later */
static void 
audit_create_arch_job(List *parsetreeList)
{
	ListCell   *parsetree_item;
	int			elemno;
	int			noids = 0;
	Oid 	   *oids;
	uint64		proc;
	char		sqltext[MAXPGPATH] = {0};
	int			ret;

	foreach(parsetree_item, parsetreeList)
	{
		RawStmt    *parsetree = lfirst_node(RawStmt, parsetree_item);

		if (IsA(parsetree->stmt, AlterSystemStmt))
		{
			AlterSystemStmt  *stmt = (AlterSystemStmt *) parsetree->stmt;
			
				if (stmt->setstmt->kind == VAR_SET_VALUE &&
					strcmp(stmt->setstmt->name, "audit.archdir") == 0)
				{
					char	archdir[MAXPGPATH];
					ListCell   *cell;
					VariableSetStmt *set_enable = stmt->setstmt;
					
					foreach(cell, set_enable->args)
					{
						A_Const *grantee = (A_Const *) lfirst(cell);

						if (strcmp(grantee->val.val.str, audit_GUC.audit_archdir) == 0)
							return;
						
						if (grantee->val.val.str != NULL)
						{
							memset(archdir, 0, MAXPGPATH);
							memcpy(archdir, grantee->val.val.str, strlen(grantee->val.val.str));
						}
					}

					internalStatement = true;
									
					StartTransactionCommand();

					/* Connect to SPI manager */
					if ((ret = SPI_connect()) < 0)
					{
						internalStatement = false;
						/* internal error */
						elog(ERROR, "audit_create_arch_job: SPI_connect returned %d", ret);
					}

					PushActiveSnapshot(GetTransactionSnapshot());

					memset(sqltext, 0, MAXPGPATH);
					sprintf(sqltext, "select oid from pg_database where datallowconn = 't' and datistemplate = 'f'");

					/* Now retrieve the crosstab source rows */
					ret = SPI_execute(sqltext, true, 0);

					/* Check for qualifying tuples */
					if (ret != SPI_OK_SELECT)
					{
						internalStatement = false;
						elog(ERROR, "audit_create_arch_job: SPI_execute returned %d", ret);
					}

					proc = SPI_processed;

					if (proc > 0)
					{
						SPITupleTable *spi_tuptable = SPI_tuptable;
						TupleDesc	spi_tupdesc = spi_tuptable->tupdesc;
						uint64		i;

						oids = (Oid *) palloc(proc * sizeof(Oid));
						
						for (i = 0; i < proc; i++)
						{
							HeapTuple	spi_tuple;
						
							spi_tuple = spi_tuptable->vals[i];

							oids[noids++] = atooid(SPI_getvalue(spi_tuple, spi_tupdesc, 1));
							
						}
					}

					for (elemno = 0; elemno < noids; elemno++)
					{
						char *dbname;
						TimestampTz time;
						char   *time_str;
						char	timec[20];
						char   *archpath;
						char	wsqltext[1500] = {0};
						char	copysql[MAXPGPATH] = {0};
						

						dbname = get_database_name(oids[elemno]);
						time = GetCurrentTimestamp();
						time_str = pstrdup(timestamptz_to_str(time));

						strncpy(timec, time_str, 19);
						timec[19] = '\0';

						archpath = palloc(MAXPGPATH);
						
						sprintf(archpath, "%s/%d", archdir, oids[elemno]);
						sprintf(copysql, "COPY (select * from hg_security.pg_audit_log) TO ''%s/%d.csv''", archpath, oids[elemno]);	/* BUG#Z5840 */
				
						
						ret = audit_mkdirs(archpath);
						if (ret < 0)
						{
							elog(ERROR, "could not create directory:%s :%m", archpath);
						}

						memset(wsqltext, 0, 1500);

						sprintf(wsqltext, "WITH id AS "
 										 "(INSERT INTO pgagent.pga_job (jobjclid, jobname, jobdesc, "
 										 "jobenabled, jobhostagent) "
 										 "SELECT jcl.jclid, 'Audit automatic archiving', "
 										 "'Audit automatic archiving every hour', false, '' "
   										 "FROM pgagent.pga_jobclass jcl WHERE jclname='Routine Maintenance' "
 									 	 "RETURNING jobid), "
 										 "insertstep AS "
 										 "(INSERT INTO pgagent.pga_jobstep (jstjobid, jstname, jstdesc, "
 										 "jstenabled, jstkind, jstonerror, jstcode, jstdbname, jstconnstr) "
  										 "SELECT id.jobid, 'Audit automatic archiving', "
  										 "'Audit automatic archiving every hour', false, 's', 'f', "
  										 "'%s', "
  										 "'%s', '' FROM id) "
  										 "INSERT INTO pgagent.pga_schedule (jscjobid, jscname, jscdesc, jscminutes, "
  										 "jschours, jscweekdays, jscmonthdays, jscmonths, jscenabled, jscstart) "
 										 "SELECT id.jobid, 'Audit automatic archiving', "
 										 "'Audit automatic archiving every hour', "
 										 "'{f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f}', "
 										 //"'{t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t}', "
 										 "'{t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t}', "
 										 "'{t,t,t,t,t,t,t}', "
 										 "'{t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t}', "
 										 "'{t,t,t,t,t,t,t,t,t,t,t,t}', false, '%s' FROM id", copysql, dbname, timec);

						SPI_finish();
						PopActiveSnapshot();
						CommitTransactionCommand();

						StartTransactionCommand();

						/* Connect to SPI manager */
						if ((ret = SPI_connect()) < 0)
						{
							/* internal error */
							elog(ERROR, "audit_create_arch_job: SPI_connect returned %d", ret);
						}

						PushActiveSnapshot(GetTransactionSnapshot());

						/* Now retrieve the crosstab source rows */
						ret = SPI_execute(wsqltext, false, 0);

						/* Check for qualifying tuples */
						if (ret != SPI_OK_INSERT)
						{
							elog(ERROR, "audit_create_arch_job: SPI_execute returned %d", ret);
						}
							
					}

					SPI_finish();
					PopActiveSnapshot();
					CommitTransactionCommand();
					
					internalStatement = false;
				}
		}
	}

}

/*
 * Function: check_audit_table
 * Description
 *	For audit related tables, only syssao users are granted permission, 
 *  and for other accessed tables, no permission is granted
 *
 * Input: object list:rangeTabls
 */
static void 
check_audit_table(List *rangeTabls)
{
	ListCell *lr;
	Oid		useroid;

	useroid = GetUserId();

	foreach(lr, rangeTabls)
	{
		Oid relOid;
		RangeTblEntry *rte = lfirst(lr);

		if (rte->rtekind != RTE_RELATION)
			continue;

		relOid = rte->relid;

		/*
		 * Check the system table audit related table
		 */
		if (AuditConfigRelationId == relOid || AuditRuleRelationId == relOid ||
			AuditThresholdRelationId == relOid)
		{
			check_audit_sao_permission(useroid, true);
		}
		else
		{
			Oid tbloid;
			tbloid = get_relname_relid(PG_AUDIT_LOG, SECURITY_SCHEMA_NAMESPACE);

			if (relOid == tbloid)
			{
				check_audit_saosso_perm(useroid, true);
			}
			else
			{
				Oid ftboid;
				ftboid = get_relname_relid(SQLITE_AUDIT_LOG, SECURITY_SCHEMA_NAMESPACE);
				if (relOid == ftboid)
				{
					check_audit_saosso_perm(useroid, true);
				}
			}
		}
	}
	return;
}

/*
 * Function: pgaudit_ExecutorCheckPerms_hook
 * Description: Obtain object information
 * Input:
 * 	rangeTabls: table name
 *
 */
static bool audit_ExecutorCheckPerms_hook(List *rangeTabls, bool abort)
{	
	if (audit_GUC.audit_enable == AUDIT_ON && !internalStatement && auditEventStack
		&& !IsAbortedTransactionBlockState())
	{
		log_select_dml(rangeTabls);
	}

	/* When the audit switch is turned on, 
	 * user access to audit related tables should be restricted
	 */
	
	if (audit_GUC.audit_enable == AUDIT_ON && !internalStatement)
	{
		check_audit_table(rangeTabls);
	}

	/* Call the next hook function */
	if (next_ExecutorCheckPerms_hook &&
		!(*next_ExecutorCheckPerms_hook) (rangeTabls, abort))
		return false;

	return true;
}


/*
 * Function: log_seq_execute
 * Description: Obtain the object information of the sequence
 * Input:
 *		objectId 
 *
 */
static void 
log_seq_execute(Oid objectId)
{
	HeapTuple proctup;
	Form_pg_class proc;
	MemoryContext contextOld;
	AuditEvent_Obj *obj;
	ObjectType objecttype = OBJECT_SEQUENCE;

	if (internalStatement || !auditEventStack)
	{
		return;
	}

	/* Get info about the function. */
	proctup = SearchSysCache1(RELOID, ObjectIdGetDatum(objectId));

	if (!proctup)
		//elog(ERROR, "cache lookup failed for function %u", objectId);
		return ;

	proc = (Form_pg_class) GETSTRUCT(proctup);

	if (proc->relkind == RELKIND_SEQUENCE)
	{
		if (!audit_check_repeat_obj(auditEventStack->auditEvent, get_namespace_name(proc->relnamespace), 
			NameStr(proc->relname), InvalidAttrNumber))
		{
			contextOld = MemoryContextSwitchTo(auditEventStack->auditEvent->auditContext);
			obj = palloc0(sizeof(AuditEvent_Obj));
			obj->classId = SequenceRelationId;
			obj->objectId = objectId;
			obj->schemaId = proc->relnamespace;
			obj->schemaName = get_namespace_name(proc->relnamespace);
			obj->objectName = pstrdup(NameStr(proc->relname));
			obj->objectClassdesc = audit_get_objname(objecttype);
			auditEventStack->auditEvent->objects = lappend(auditEventStack->auditEvent->objects, obj);
		
			MemoryContextSwitchTo(contextOld);
		}

	}

	ReleaseSysCache(proctup);

}


/*
 * Function: log_index_execute
 * Description: Obtain object information for alter index
 * Input:
 *		objectId
 *
 */
static void 
log_index_execute(Oid objectId)
{
	HeapTuple classtup;
	Form_pg_class tuple;
	MemoryContext contextOld;
	AuditEvent_Obj *obj;
	HeapTuple indextup;
	Form_pg_index in_tuple;
	ObjectType objecttype = OBJECT_TABLE;
	
	if (internalStatement || !auditEventStack)
	{
		return;
	}

	/* Get info about the pg_class. */
	classtup = SearchSysCache1(RELOID, ObjectIdGetDatum(objectId));

	if (!classtup)
	{
		/*
		fix bug:
		CREATE TABLE part_attmp (a int primary key) partition by range (a);
		CREATE TABLE part_attmp1 PARTITION OF part_attmp FOR VALUES FROM (0) TO (100);
		ALTER INDEX part_attmp_pkey RENAME TO part_attmp_index;
		*/
		//elog(ERROR, "cache lookup failed for index %u", objectId);
		return;
	}

	tuple = (Form_pg_class) GETSTRUCT(classtup);

	if (tuple->relkind == RELKIND_INDEX)
	{
		indextup = SearchSysCache1(INDEXRELID, ObjectIdGetDatum(objectId));
		in_tuple = (Form_pg_index) GETSTRUCT(indextup);
		
		contextOld = MemoryContextSwitchTo(auditEventStack->auditEvent->auditContext);

		obj = palloc0(sizeof(AuditEvent_Obj));
		obj->classId = RelationRelationId;
		obj->objectId = objectId;
		obj->schemaId = tuple->relnamespace;
		obj->schemaName = get_namespace_name(tuple->relnamespace);
		obj->objectName = pstrdup(NameStr(tuple->relname));
		obj->objectClassdesc = audit_get_objname(objecttype);
		obj->indrelId = in_tuple->indrelid;
		auditEventStack->auditEvent->objects = lappend(auditEventStack->auditEvent->objects, obj);
		
		MemoryContextSwitchTo(contextOld);

		ReleaseSysCache(indextup);
	}

	ReleaseSysCache(classtup);
}


/*
 * Function: log_function_execute
 * Description: Obtain object information for function/PROCESS
 * Input:
 *		objectId
 *
 */
static void 
log_function_execute(Oid objectId)
{
	HeapTuple proctup;
	Form_pg_proc proc;
	MemoryContext contextOld;
	AuditEvent_Obj *obj;
	ObjectType objecttype = OBJECT_FUNCTION;

	if (internalStatement || !auditEventStack)
	{
		return;
	}


	/* Get info about the function. */
	proctup = SearchSysCache1(PROCOID, ObjectIdGetDatum(objectId));

	if (!proctup)
		//elog(ERROR, "cache lookup failed for function %u", objectId);
		return ;

	proc = (Form_pg_proc) GETSTRUCT(proctup);

	if (proc->prokind == 'p')
	{
		objecttype = OBJECT_PROCEDURE;
	}

	contextOld = MemoryContextSwitchTo(auditEventStack->auditEvent->auditContext);

	obj = palloc0(sizeof(AuditEvent_Obj));
	obj->classId = ProcedureRelationId;
	obj->objectId = objectId;
	obj->schemaId = proc->pronamespace;
	obj->schemaName = get_namespace_name(proc->pronamespace);
	obj->objectName = pstrdup(NameStr(proc->proname));
	obj->objectClassdesc = audit_get_objname(objecttype);
	auditEventStack->auditEvent->objects = lappend(auditEventStack->auditEvent->objects, obj);
	
	MemoryContextSwitchTo(contextOld);
	

	ReleaseSysCache(proctup);
}



/*
 * Function: audit_object_access_hook()
 *
 * Description:
 * 	 Get the object information of the function hook function
 *
 */
static void 
audit_object_access_hook(ObjectAccessType access,
					Oid classId,
					Oid objectId,
					int subId,
					void *arg)
{
	AuditStmtType type = INVALID_STMT;
	
	if (audit_GUC.audit_enable == AUDIT_ON && !internalStatement && 
		auditEventStack && !IsAbortedTransactionBlockState())
	{
		switch (access)
		{
			case OAT_FUNCTION_EXECUTE:
				log_function_execute(objectId);
				break;
			case OAT_POST_ALTER:
				{
					type = audit_get_stmt_type(auditEventStack->auditEvent->actionname, false, -1);
					switch (type)
					{
						case ALTER_INDEX:
							log_index_execute(objectId);
							break;
						case ALTER_SEQUENCE:
							log_seq_execute(objectId);
							break;
						case ALTER_PROCEDURE:
						case ALTER_FUNCTION:
							log_function_execute(objectId);
							break;
						default:
							break;
					}
				}

				break;
			case OAT_DROP:
				{
					type = audit_get_stmt_type(auditEventStack->auditEvent->actionname, false, -1);
					switch (type)
					{
						case DROP_INDEX:
							log_index_execute(objectId);
							break;
						case DROP_SEQUENCE:
							log_seq_execute(objectId);
							break;
						case DROP_PROCEDURE:
						case DROP_FUNCTION:
							log_function_execute(objectId);
							break;
						default:
							break;
					}
				}
				break;
			case OAT_POST_CREATE:
				{
					type = audit_get_stmt_type(auditEventStack->auditEvent->actionname, false, -1);
					switch (type)
					{
						case CREATE_SEQUENCE:
							log_seq_execute(objectId);
							break;
						case CREATE_FUNCTION:
						case CREATE_PROCEDURE:
							log_function_execute(objectId);
							break;
						default:
							break;
					}
				}
				break;
			default:
				break;
		}
	}

	if (next_object_access_hook)
		(*next_object_access_hook) (access, classId, objectId, subId, arg);
}

/*
 * Function: audit_getobject
 * Description: Obtain object information (table or sequence)
 *
 */
static void 
audit_getobject(Oid relOid_in, Relation rel_in)
{
	MemoryContext contextOld;
	AuditEvent_Obj *obj;
	Relation rel;
	Oid 	relOid = relOid_in;
	ObjectType objecttype = OBJECT_TABLE;

	if (audit_GUC.audit_enable != AUDIT_ON)
		return;

	if (internalStatement || !auditEventStack)
		return;
	
	if (rel_in)
	{
		rel = rel_in;
		relOid = RelationGetRelid(rel);
	}
	else if (relOid)
	{
		rel = relation_open(relOid, AccessShareLock);
	}
	else
		elog(ERROR, "Invalid object info");

	
	if (!audit_check_repeat_obj(auditEventStack->auditEvent, get_namespace_name(RelationGetNamespace(rel)), 
				pstrdup(RelationGetRelationName(rel)), InvalidAttrNumber))
	{
		contextOld = MemoryContextSwitchTo(auditEventStack->auditEvent->auditContext);
		obj = palloc0(sizeof(AuditEvent_Obj));
		if(rel->rd_rel->relkind == RELKIND_SEQUENCE)
		{
			obj->classId = SequenceRelationId;
			objecttype = OBJECT_SEQUENCE;
		}
		else
		{
			obj->classId = RelationRelationId;
			objecttype = OBJECT_TABLE;
		}

		obj->objectId = relOid;
		obj->schemaId = RelationGetNamespace(rel);
		obj->schemaName = get_namespace_name(RelationGetNamespace(rel));
		obj->objectName = pstrdup(RelationGetRelationName(rel));
		obj->objectClassdesc = audit_get_objname(objecttype);

		auditEventStack->auditEvent->objects = lappend(auditEventStack->auditEvent->objects, obj);

		MemoryContextSwitchTo(contextOld);
	}

	if(!rel_in)
		relation_close(rel, AccessShareLock);

}

/*
 * obtain authentication method
 */
static char *
get_auth_method(UserAuth method)
{
	char	*result = NULL;

	result = (char *)malloc(AUTHMETHOD_LEN);

	if (result == NULL)
		return NULL;

	switch (method)
	{
		case uaReject:
			strcpy(result, "reject");
			break;
		case uaTrust:
			strcpy(result, "trust");
			break;
		case uaIdent:
			strcpy(result, "ident");
			break;
		case uaPassword:
			strcpy(result, "password");
			break;
		case uaMD5:
			strcpy(result, "md5");
			break;
#ifdef USE_GMSSL
		case uaSM3:
			strcpy(result, "sm3");
			break;
		case uaSCRAMsha256:
			strcpy(result, "scram-sha-256");
			break;
		case uaSCRAMsm3:
			strcpy(result, "scram-sm3");
			break;
#else
		case uaSCRAMsha256:
			strcpy(result, "scram-sha-256");
			break;
#endif
		case uaGSS:
			strcpy(result, "gss");
			break;
		case uaSSPI:
			strcpy(result, "sspi");
			break;
		case uaPAM:
			strcpy(result, "pam");
			break;
		case uaBSD:
			strcpy(result, "bsd");
			break;
		case uaLDAP:
			strcpy(result, "ldap");
			break;
		case uaCert:
			strcpy(result, "cert");
			break;
		case uaRADIUS:
			strcpy(result, "radius");
			break;
		case uaPeer:
			strcpy(result, "peer");
			break;
		default:
			break;
	}

	return result;
	
}


/* 
 * obtain the mac address of the client
 */
static char *
get_remote_mac(Port *port_m)
{
	if (port_m == NULL)
		return NULL;

	if (!IS_AF_UNIX(port_m->raddr.addr.ss_family))
	{
	#ifndef WIN32
		char	ip[100] = {0};
		char   *hwa = NULL;
		char	mask[100] = {0};
		char	line[200] = {0};
		char	dev[100] = {0};
		int		type, flags;
		FILE   *fp;
		int		num, entries = 0;
		int 	ret;
		char	remote_host[NI_MAXHOST] = {0};
		char	remote_port[NI_MAXSERV] = {0};
		bool	tag = false;

		remote_host[0] = '\0';
		remote_port[0] = '\0';

		hwa = (char *)malloc(MAC_LEN);

		if ((ret = pg_getnameinfo_all(&port_m->raddr.addr, port_m->raddr.salen,
										remote_host, sizeof(remote_host),
										remote_port, sizeof(remote_port),
										(tag ? 0 : NI_NUMERICHOST) | NI_NUMERICSERV)) != 0)
			ereport(WARNING,
						(errmsg_internal("pg_getnameinfo_all() failed: %s",
							gai_strerror(ret))));
		do
		{
			if ((fp = fopen("/proc/net/arp", "r")) == NULL)
			{
				elog(LOG, "fopen() failed: %m");
				break;
			}
			if (fgets(line, sizeof(line), fp) != (char *) NULL)
			{
				strncpy(mask, "-", sizeof(mask));
				strncpy(dev, "-", sizeof(dev));

				for (; fgets(line, sizeof(line), fp);)
				{
					num = sscanf(line, "%s 0x%x 0x%x %99s %99s %99s\n", ip, &type, &flags, hwa, mask, dev);
					if (num < 4)
						break;
					if (num == 5)
					{
						num = sscanf(line, "%s 0x%x 0x%x %99s %99s\n", ip, &type, &flags, mask, dev);
						hwa[0] = 0;
					}

					entries++;

					if (strcmp(ip, remote_host) == 0)
					{
						fclose(fp);
						return hwa;
					}
					else
					{
						continue;
					}
				}
			}
			(void) fclose(fp);
		} while (0);
										
	#endif
	}

	return NULL;

}

/*
 * save the authentication method to clientInfo structure
 */
static void
audit_Save_ConnectAuthMethod(Port *port)
{
	char	*auth_method = NULL;

	if (audit_GUC.audit_enable != AUDIT_ON)
		return;

	auth_method = get_auth_method(port->hba->auth_method);
	if (auth_method != NULL)
		memcpy(&(clientInfo.authmethod), auth_method, strlen(auth_method) + 1);
}


/*
 * When dropping audit extension, the audit switch must be
 * turned off, otherwise an error will be reported
 */
static void
audit_drop_pgaudit(char *name)
{
	if (audit_GUC.audit_enable != AUDIT_ON)
		return;

	if (strcmp(name, "pg_audit") == 0)
		ereport(ERROR,
				(errmsg("cannot drop extension \"%s\" when audit.enable is on", name)));
}

/*
 * restrict ALTER operations on pg_audit_log
 */
static void
audit_alter_pg_auditlog(char *relname)
{
	if (audit_GUC.audit_enable != AUDIT_ON || relname == NULL)
		return;
	
	if (pg_strcasecmp(relname, "pg_audit_log") == 0)
	{
		ereport(ERROR,
				(errmsg("cannot alter table \"%s\"", relname)));
	}
}

#ifdef USE_SQLITE
/*
 * Open remote sqlite database using specified database path
 * and flags of opened file descriptor mode.
 */
static void
sqlite_open_db_for_audit(AuditSystemEvent event, AuditResult result, char *username, char *errtext)
{
	sqlite3	   *conn = NULL;
	int			rc;
	char	   *err;
	char	   *actionname;
	TimestampTz audittime;
	char	   *privilege;
	Oid			useroid = 0;
	/* Begin - BUG#00004776 */
	Oid			roleoid = 0;
	char	   *rolename = NULL;
	/* End - BUG#00004776 */
	int32		sessionid;
	char		inserttext[MAXPGPATH] = {0};
	char	   *tmp = NULL;
	char	   *timestr = NULL;

	if ((audit_GUC.audit_enable != AUDIT_ON || audit_GUC.audit_systemopers == AUDIT_OPER_OFF)
			&& event < MANDATORY_AUDIT_ON)
	{
		return;
	}

	/*result == SYSTEMAUDIT_AUDIT_LOG_DISK_FULL is special.
	   when process "logger" does not use *audit_GUC.audit_pause*/
	if(event != SYSTEMAUDIT_AUDIT_LOG_DISK_FULL && *audit_GUC.audit_pause)
		return;

	if (audit_GUC.audit_systemopers == AUDIT_OPER_SUCC && result != AUDIT_SUCCESS)
	{
		return;
	}
	if (audit_GUC.audit_systemopers == AUDIT_OPER_FAIL && result != AUDIT_FAIL)
	{
		return;
	}

	if (audit_GUC.audit_sqlitedbpath == NULL || strlen(audit_GUC.audit_sqlitedbpath) <= 0)
	{
		ereport(WARNING,
			errmsg("The parameter audit_GUC.audit_sqlitedbpath is empty. Please set the path of the sqlite database first"));
		return;
	}

	/* make 'LIKE' of SQLite case sensitive like PostgreSQL */
	actionname = malloc(32);
	privilege = malloc(16);
	
	switch(event)
	{
		case SYSTEMAUDIT_START:
			strcpy(actionname, START_S);
			strcpy(privilege, AUDIT_PRIVILEGE_INSTANCE_S);
			break;

		case SYSTEMAUDIT_STOP:
			strcpy(actionname, STOP_S);
			strcpy(privilege, AUDIT_PRIVILEGE_INSTANCE_S);
			break;

		case SYSTEMAUDIT_LOGIN:
			strcpy(actionname, LOGIN_S);
			strcpy(privilege, AUDIT_PRIVILEGE_SYSTEM_S);
			break;

		case SYSTEMAUDIT_LOGOUT:
			strcpy(actionname, LOGOUT_S);
			strcpy(privilege, AUDIT_PRIVILEGE_SYSTEM_S);
			break;

		case SYSTEMAUDIT_IMPORT:
			strcpy(actionname, IMPORT_S);
			strcpy(privilege, AUDIT_PRIVILEGE_DATABASE_S);
			break;

		case SYSTEMAUDIT_EXPORT:
			strcpy(actionname, EXPORT_S);
			strcpy(privilege, AUDIT_PRIVILEGE_DATABASE_S);
			break;

		case SYSTEMAUDIT_RELOAD:
			strcpy(actionname, RELOAD_S);
			strcpy(privilege, AUDIT_PRIVILEGE_INSTANCE_S);
			break;
		case SYSTEMAUDIT_AUDIT_STORAGE_INVALID:
			strcpy(actionname, AUDIT_STORAGE_INVALID_S);
			strcpy(privilege, AUDIT_PRIVILEGE_INSTANCE_S);
			break;
		case SYSTEMAUDIT_ALTER_GUC:
			strcpy(actionname, ALTER_GUC_S);
			strcpy(privilege, AUDIT_PRIVILEGE_INSTANCE_S);
			break;

		case SYSTEMAUDIT_SWITCHOVER:
			strcpy(actionname, SWITCHOVER_S);
			strcpy(privilege, AUDIT_PRIVILEGE_INSTANCE_S);
			break;

		case MANDATORY_AUDIT_ON:
			strcpy(actionname, MANDATORY_AUDIT_ON_S);
			strcpy(privilege, AUDIT_PRIVILEGE_INSTANCE_S);
			break;

		case MANDATORY_AUDIT_OFF:
			strcpy(actionname, MANDATORY_AUDIT_OFF_S);
			strcpy(privilege, AUDIT_PRIVILEGE_INSTANCE_S);
			break;

		case SYSTEMAUDIT_PG_DUMP:
			strcpy(actionname, PG_DUMP_S);
			strcpy(privilege, AUDIT_PRIVILEGE_SYSTEM_S);
			break;

		case SYSTEMAUDIT_PG_DUMPALL:
			strcpy(actionname, PG_DUMPALL_S);
			strcpy(privilege, AUDIT_PRIVILEGE_SYSTEM_S);
			break;

		case SYSTEMAUDIT_PG_RESTORE:
			strcpy(actionname, PG_RESTORE_S);
			strcpy(privilege, AUDIT_PRIVILEGE_SYSTEM_S);
			break;

		case SYSTEMAUDIT_PG_BASEBACKUP:
			strcpy(actionname, PG_BASEBACKUP_S);
			strcpy(privilege, AUDIT_PRIVILEGE_SYSTEM_S);
			break;

		case SYSTEMAUDIT_PERMISSION:
			strcpy(actionname, PG_PERMISSION_S);
			strcpy(privilege, AUDIT_PRIVILEGE_SYSTEM_S);
			break;

		case SYSTEMAUDIT_SPACE_ALARM:
			strcpy(actionname, AUDIT_SPACE_ALARM_S);
			strcpy(privilege, AUDIT_PRIVILEGE_SYSTEM_S);
			break;

		case SYSTEMAUDIT_AUDIT_SPACE_LIMIT:
			strcpy(actionname, AUDIT_SPACE_LIMIT_S);
			strcpy(privilege, AUDIT_PRIVILEGE_SYSTEM_S);
			break;

		case SYSTEMAUDIT_AUDIT_DISK_FULL:
			strcpy(actionname, AUDIT_DISK_FULL_S);
			strcpy(privilege, AUDIT_PRIVILEGE_SYSTEM_S);
			break;

		case SYSTEMAUDIT_AUDIT_LOG_DISK_FULL:
			strcpy(actionname, AUDIT_LOG_DISK_FULL_S);
			strcpy(privilege, AUDIT_PRIVILEGE_SYSTEM_S);
			break;

		case SYSTEMAUDIT_EXEC_FAIL:
			strcpy(actionname, SYSTEMAUDIT_EXEC_FAIL_S);
			strcpy(privilege, AUDIT_PRIVILEGE_SYSTEM_S);
			break;

		case SYSTEMAUDIT_HGHAC_SYNC:
			strcpy(actionname, SYSTEMAUDIT_HGHAC_SYNC_GUC_S);
			strcpy(privilege, AUDIT_PRIVILEGE_SYSTEM_S);
			break;

		case SYSTEMAUDIT_AUDIT_WALSENDER_CONN:
			strcpy(actionname, SYSTEMAUDIT_WALSENDER_CONN_S);
			strcpy(privilege, AUDIT_PRIVILEGE_SYSTEM_S);
			break;

		case SYSTEMAUDIT_AUDIT_WALSENDER_EXIT:
			strcpy(actionname, SYSTEMAUDIT_WALSENDER_EXIT_S);
			strcpy(privilege, AUDIT_PRIVILEGE_SYSTEM_S);
			break;

		case SYSTEMAUDIT_DEAD_LOCK:
			strcpy(actionname, SYSTEMAUDIT_DEAD_LOCK_S);
			strcpy(privilege, AUDIT_PRIVILEGE_SYSTEM_S);
			break;

		case SYSTEMAUDIT_OPEN_FILE_FAIL:
			strcpy(actionname, OPEN_FIEL_FAIL_S);
			strcpy(privilege, AUDIT_PRIVILEGE_SYSTEM_S);
			break;

		case SYSTEMAUDIT_FAILOVER:
			strcpy(actionname, FAILOVER_S);
			strcpy(privilege, AUDIT_PRIVILEGE_INSTANCE_S);
			break;

		case SYSTEMAUDIT_PROMOTE:
			strcpy(actionname, PROMOTE_S);
			strcpy(privilege, AUDIT_PRIVILEGE_INSTANCE_S);
			break;

		case SYSTEMAUDIT_AUDIT_RECOVERY:
			strcpy(actionname, AUDIT_RECOVERY_S);
			strcpy(privilege, AUDIT_PRIVILEGE_SYSTEM_S);
			break;

		case SYSTEMAUDIT_AUDIT_LOCK_SESSION:
			strcpy(actionname, AUDIT_LOCK_SESSION);
			strcpy(privilege, AUDIT_PRIVILEGE_SYSTEM_S);
			break;

		case SYSTEMAUDIT_AUDIT_UNLOCK_SESSION:
			strcpy(actionname, AUDIT_UNLOCK_SESSION);
			strcpy(privilege, AUDIT_PRIVILEGE_SYSTEM_S);
			break;

		case SYSTEMAUDIT_AUDIT_RESOURCE_MONITOR:
			strcpy(actionname, AUDIT_RESOURCE_MONITOR);
			strcpy(privilege, AUDIT_PRIVILEGE_SYSTEM_S);
			break;

		case SYSTEMAUDIT_CHANNEL:
			strcpy(actionname, SYSTEMAUDIT_CHANNEL_S);
			strcpy(privilege, AUDIT_PRIVILEGE_SYSTEM_S);
			break;

		case SYSTEMAUDIT_START_PGAGENT:
			strcpy(actionname, START_PGAGENT);
			strcpy(privilege, AUDIT_PRIVILEGE_SYSTEM_S);
			break;

		case SYSTEMAUDIT_STOP_PGAGENT:
			strcpy(actionname, STOP_PGAGENT);
			strcpy(privilege, AUDIT_PRIVILEGE_SYSTEM_S);
			break;

		case SYSTEMAUDIT_LOCK_USER:
			strcpy(actionname, SYSTEMAUDIT_LOCK_USER_S);
			strcpy(privilege, AUDIT_PRIVILEGE_SYSTEM_S);
			break;

		case SYSTEMAUDIT_UNLOCK_USER:
			strcpy(actionname, SYSTEMAUDIT_UNLOCK_USER_S);
			strcpy(privilege, AUDIT_PRIVILEGE_SYSTEM_S);
			break;

		case SYSTEMAUDIT_CERT_VERIFY:
			strcpy(actionname, SYSTEMAUDIT_CERT_VERIFY_S);
			strcpy(privilege, AUDIT_PRIVILEGE_SYSTEM_S);
			break;
		default:
			break;
	}

	rc = sqlite3_open(audit_GUC.audit_sqlitedbpath, &conn);
	if (rc != SQLITE_OK)
		ereport(WARNING,
				(errcode(ERRCODE_FDW_UNABLE_TO_ESTABLISH_CONNECTION),
				 errmsg("Failed to open SQLite DB, file '%s', result code %d", audit_GUC.audit_sqlitedbpath, rc)));
	sqlite3_busy_timeout(conn, 5000);

	audittime = GetCurrentTimestamp();
	timestr = pstrdup(timestamptz_to_str(audittime));
	useroid = GetUserId_audit();
	if (IsTransactionState() && !OidIsValid(useroid) && strcmp(actionname, LOGIN_S) == 0 && result == AUDIT_FAIL)
	{
		useroid = get_role_oid(MyProcPort->user_name, true);
	}

	switch(event)
	{
		case SYSTEMAUDIT_START:
		case SYSTEMAUDIT_STOP:
		case SYSTEMAUDIT_SWITCHOVER:
		case SYSTEMAUDIT_PROMOTE:
		case SYSTEMAUDIT_FAILOVER:
		case MANDATORY_AUDIT_ON:
		case MANDATORY_AUDIT_OFF:
			sessionid = PostmasterPid;
			break;
		default:
			sessionid = MyProcPid;
			break;
	}

	if (username == NULL)
	{
		if (useroid != InvalidOid && event != SYSTEMAUDIT_AUDIT_LOCK_SESSION)
			tmp = GetUserNameFromId(useroid, true);
		else if (event == SYSTEMAUDIT_AUDIT_LOCK_SESSION)
			tmp = pstrdup("");
	}

	/* Begin - BUG#00004776 */
	if (event != SYSTEMAUDIT_OPEN_FILE_FAIL)
	{
		if (IsTransactionState() && OidIsValid(useroid))
		{
			CatCList   *catlist;

			catlist = SearchSysCacheList1(AUTHMEMMEMROLE, ObjectIdGetDatum(useroid));
			if (catlist->n_members > 0)
			{
				HeapTuple   auth_member_tuple = &catlist->members[0]->tuple;
				Form_pg_auth_members auth_member = (Form_pg_auth_members) GETSTRUCT(auth_member_tuple);

				roleoid = auth_member->roleid;
			}
			if (catlist)
				ReleaseSysCacheList(catlist);
		}

		if (OidIsValid(roleoid))
		{
			rolename = GetUserNameFromId(roleoid, true);
		}
	}
	/* End - BUG#00004776 */
	
	
	sprintf(inserttext, "insert into pg_sqlite_audit_log(sessionoid, useroid, username, roleoid, rolename, audittime, result, audittype, commandid, actionname, clientport, clientname, clientip, clientmac, clientappname, errtext, authmethod, privlevel) values(%d, %d, \'%s\', %d, \'%s\', \'%s\', \'%s\', \'%s\', 0, \'%s\', \'%s\', \'%s\', \'%s\', \'%s\', \'%s\', \'%s\', \'%s\', \'%s\')", 
						sessionid, useroid, username != NULL ? username : tmp, roleoid, rolename, timestr, AuditResultToString(result), AUDIT_TYPE_SYSYEM_S, actionname,
						clientInfo.port, clientInfo.clientname, clientInfo.ip, clientInfo.mac,
						clientInfo.appname, errtext, clientInfo.authmethod, privilege);
	rc = sqlite3_exec(conn, inserttext, NULL, NULL, &err);
	if (rc != SQLITE_OK)
	{
		ereport(WARNING,
						(errcode(ERRCODE_FDW_UNABLE_TO_ESTABLISH_CONNECTION),
						 errmsg("Failed to insert in SQLite DB, file '%s', SQLite error '%s', result code %d", audit_GUC.audit_sqlitedbpath, err, rc)));
		sqlite3_free(err);
	}

	sqlite3_close(conn);
	conn = NULL;
}
#endif


/*
 * Function: audit_SaveConnectInfo
 * Description: Obtain server and client information
 * Input: Client connection port
 * Note: Called after the client connects, the information obtained is 
 * used for auditing all sql executed in the connection
 *
 */
static void 
audit_save_connectinfo(Port *port)
{
	const SockAddr local_addr = port->laddr;
	char		local_addr_s[NI_MAXHOST] = {0};
	char		local_port[NI_MAXSERV] = {0};
	char		host_name[CLIENTNAME_LEN];
	char	   *remote_mac = NULL;

	if (audit_GUC.audit_enable != AUDIT_ON)
		return;

	memset(&clientInfo,0,sizeof(ClientInfo));
	memset(host_name, 0, CLIENTNAME_LEN);
	
	pg_getnameinfo_all(&local_addr.addr, local_addr.salen,
					   local_addr_s, sizeof(local_addr_s),
					   local_port, sizeof(local_port),
					   NI_NUMERICHOST | NI_NUMERICSERV);
#ifndef WIN32
	gethostname(host_name, sizeof(host_name));
#endif

	if (port->application_name != NULL)
	{
		memcpy(&(clientInfo.appname), port->application_name, strlen(port->application_name) + 1);
	}

	if (strlen(host_name) != 0)
	{
		memcpy(&(clientInfo.clientname), host_name, strlen(host_name) + 1);
	}

	remote_mac = get_remote_mac(port);

	if (remote_mac != NULL)
		memcpy(&(clientInfo.mac), remote_mac, strlen(remote_mac) + 1);
		
	memcpy(&(clientInfo.ip), port->remote_host, strlen(port->remote_host) + 1);
	memcpy(&(clientInfo.port), port->remote_port, strlen(port->remote_port) + 1);


	if (strcmp(local_addr_s, "[local]") == 0)
	{
		memcpy(&(clientInfo.ip), "127.0.0.1", strlen("127.0.0.1") + 1);
	}

	//audit_local_mac_address(serverInfo.mac, MAC_LEN);
}


/*
 * Function: audit_aborted
 * Description: When a transaction is aborted, write the SQL audit 
 *				information in the transaction into the audit buffer
 *
 */
static void 
audit_aborted(void)
{
	//TransactionId xid;

	if (audit_GUC.audit_enable != AUDIT_ON)
		return;

	//xid = GetCurrentTransactionIdIfAny();

	/* If there is still data in auditEventStack, it needs to be written to auditlist first */
	while (auditEventStack)
	{
		stack_pop_to_list(AUDIT_FAIL);
	}

	internalStatement = false;
}

/*
 * Functio: audit_commited
 * Description: When a transaction is commited, write the SQL audit 
 *				information in the transaction into the audit buffer
 *
 */
static void 
audit_commited(void)
{
	//TransactionId xid;

	if (audit_GUC.audit_enable != AUDIT_ON)
		return;

	/* If there is still data in auditEventStack, it needs to be written to auditlist first */
	while (auditEventStack)
	{
		stack_pop_to_list(AUDIT_SUCCESS);
	}

	internalStatement = false;
}


static AuditEvent *
generateSystemEvent(AuditSystemEvent event, AuditResult result)
{
	int			 record_len;
	AuditEvent *newAuditEvent;
	char	   actionname[32];
	AuditEvent *auditEvent = NULL;
	char *query_string_tmp = NULL;
	int cmdlen = 0;

	switch(event)
	{
		case SYSTEMAUDIT_START:
			strcpy(actionname, START_S);
			break;

		case SYSTEMAUDIT_STOP:
			strcpy(actionname, STOP_S);
			break;

		case SYSTEMAUDIT_LOGIN:
			strcpy(actionname, LOGIN_S);
			break;

		case SYSTEMAUDIT_LOGOUT:
			strcpy(actionname, LOGOUT_S);
			break;

		case SYSTEMAUDIT_IMPORT:
			strcpy(actionname, IMPORT_S);
			break;

		case SYSTEMAUDIT_EXPORT:
			strcpy(actionname, EXPORT_S);
			break;

		case SYSTEMAUDIT_RELOAD:
			strcpy(actionname, RELOAD_S);
			break;

		case SYSTEMAUDIT_AUDIT_STORAGE_INVALID:
			strcpy(actionname, AUDIT_STORAGE_INVALID_S);
			break;
			
		case SYSTEMAUDIT_ALTER_GUC:
			strcpy(actionname, ALTER_GUC_S);
			break;

		case SYSTEMAUDIT_LOCK_ACCCOUNT:
			strcpy(actionname, LOCK_ACCOUNT_S);
			break;

		case SYSTEMAUDIT_SWITCHOVER:
			strcpy(actionname, SWITCHOVER_S);
			break;

		case MANDATORY_AUDIT_ON:
			strcpy(actionname, MANDATORY_AUDIT_ON_S);
			break;

		case MANDATORY_AUDIT_OFF:
			strcpy(actionname, MANDATORY_AUDIT_OFF_S);
			break;

		case SYSTEMAUDIT_PG_DUMP:
			strcpy(actionname, PG_DUMP_S);
			break;

		case SYSTEMAUDIT_PG_DUMPALL:
			strcpy(actionname, PG_DUMPALL_S);
			break;

		case SYSTEMAUDIT_PG_RESTORE:
			strcpy(actionname, PG_RESTORE_S);
			break;

		case SYSTEMAUDIT_PG_BASEBACKUP:
			strcpy(actionname, PG_BASEBACKUP_S);
			break;

		case SYSTEMAUDIT_PERMISSION:
			strcpy(actionname, PG_PERMISSION_S);
			break;

		case SYSTEMAUDIT_SPACE_ALARM:
			strcpy(actionname, AUDIT_SPACE_ALARM_S);
			break;

		case SYSTEMAUDIT_AUDIT_SPACE_LIMIT:
			strcpy(actionname, AUDIT_SPACE_LIMIT_S);
			break;

		case SYSTEMAUDIT_AUDIT_DISK_FULL:
			strcpy(actionname, AUDIT_DISK_FULL_S);
			break;

		case SYSTEMAUDIT_AUDIT_LOG_DISK_FULL:
			strcpy(actionname, AUDIT_LOG_DISK_FULL_S);
			break;

		case SYSTEMAUDIT_EXEC_FAIL:
			strcpy(actionname, SYSTEMAUDIT_EXEC_FAIL_S);
			break;

		case SYSTEMAUDIT_HGHAC_SYNC:
			strcpy(actionname, SYSTEMAUDIT_HGHAC_SYNC_GUC_S);
			break;

		case SYSTEMAUDIT_AUDIT_WALSENDER_CONN:
			strcpy(actionname, SYSTEMAUDIT_WALSENDER_CONN_S);
			break;

		case SYSTEMAUDIT_AUDIT_WALSENDER_EXIT:
			strcpy(actionname, SYSTEMAUDIT_WALSENDER_EXIT_S);
			break;

		case SYSTEMAUDIT_DEAD_LOCK:
			strcpy(actionname, SYSTEMAUDIT_DEAD_LOCK_S);
			break;
		case SYSTEMAUDIT_FAILOVER:
			strcpy(actionname, FAILOVER_S);
			break;

		case SYSTEMAUDIT_PROMOTE:
			strcpy(actionname, PROMOTE_S);
			break;

		case SYSTEMAUDIT_AUDIT_RECOVERY:
			strcpy(actionname, AUDIT_RECOVERY_S);
			break;

		case SYSTEMAUDIT_AUDIT_LOCK_SESSION:
			strcpy(actionname, AUDIT_LOCK_SESSION);
			break;

		case SYSTEMAUDIT_AUDIT_UNLOCK_SESSION:
			strcpy(actionname, AUDIT_UNLOCK_SESSION);
			break;

		case SYSTEMAUDIT_AUDIT_RESOURCE_MONITOR:
			strcpy(actionname, AUDIT_RESOURCE_MONITOR);
			break;

		case SYSTEMAUDIT_CHANNEL:
			strcpy(actionname, SYSTEMAUDIT_CHANNEL_S);
			break;

		case SYSTEMAUDIT_LOCK_USER:
			strcpy(actionname, SYSTEMAUDIT_LOCK_USER_S);
			break;
		case SYSTEMAUDIT_UNLOCK_USER:
			strcpy(actionname, SYSTEMAUDIT_UNLOCK_USER_S);
			break;

		default:
			break;
	}

	if (auditEventStack && auditEventStack->auditEvent)
	{
		auditEvent = auditEventStack->auditEvent;
		cmdlen = strlen(auditEvent->sqltext);
		record_len =  INTALIGN(sizeof(AuditEvent) + cmdlen);
		newAuditEvent = palloc0(record_len);

		query_string_tmp = pstrdup(auditEvent->sqltext);
		if (hide_password(query_string_tmp))
		{
			newAuditEvent->sqltext = pnstrdup((const char *)(query_string_tmp), strlen(query_string_tmp));
		}
		else
		{
			newAuditEvent->sqltext = pnstrdup((const char *)(auditEvent->sqltext), cmdlen);
		}
	}
	else
	{
		record_len =  INTALIGN(sizeof(AuditEvent));
		newAuditEvent = palloc0(record_len);
	}

	newAuditEvent->audittime = GetCurrentTimestamp();
	strcpy(newAuditEvent->actionname, actionname);

	newAuditEvent->privilege = AUDIT_PRIVILEGE_SYSTEM;

	/* When a system audit occurs, there may not be a user ID */
	newAuditEvent->useroid = GetUserId_audit();
	/* When the user inputs the password incorrectly */
	if (!OidIsValid(newAuditEvent->useroid) && strcmp(newAuditEvent->actionname, LOGIN_S) == 0 && result == AUDIT_FAIL)
	{
		newAuditEvent->useroid = get_role_oid(MyProcPort->user_name, true);
	}

	switch(event)
	{
		case SYSTEMAUDIT_START:
		case SYSTEMAUDIT_STOP:
		case SYSTEMAUDIT_SWITCHOVER:
		case SYSTEMAUDIT_FAILOVER:
		case SYSTEMAUDIT_PROMOTE:
		case MANDATORY_AUDIT_ON:
		case MANDATORY_AUDIT_OFF:
			/* For the above events, the corresponding process ID should be PostmasterPid */
			newAuditEvent->sessionid = PostmasterPid;
			break;
		default:
			newAuditEvent->sessionid = MyProcPid;
			break;
	}
	
	newAuditEvent->result = result;

	return newAuditEvent;
}

/*
 * 
 * Call at the appropriate time
 *
 */
/*
 * Function: deal_with_system_event()
 *
 * Description:
 *	 Handling system audit events
 *
 */
static void 
deal_with_system_event(AuditSystemEvent event, AuditResult result)
{
	AuditEvent *auditevent = NULL;
	bool is_fail = false;

	if (audit_GUC.audit_enable != AUDIT_ON || *audit_GUC.audit_pause)
		return;

#ifdef USE_SQLITE
	if (!OidIsValid(MyDatabaseId) && sqlite_open_db_for_audit_hook)
	{
		if (MyProcPort && MyProcPort->user_name)
			sqlite_open_db_for_audit_hook(event, result, MyProcPort->user_name, NULL);
		else
			sqlite_open_db_for_audit_hook(event, result, NULL, NULL);

		return ;
	}
#endif

	/* Exclude mandatory audit */
	if (MANDATORY_AUDIT_ON != event && MANDATORY_AUDIT_OFF != event)
	{
		/* System audit only audits successfully */
		if (audit_GUC.audit_systemopers == AUDIT_OPER_SUCC && result != AUDIT_SUCCESS)
		{
			return;
		}
		/* System audit only audits failures */
		else if (audit_GUC.audit_systemopers == AUDIT_OPER_FAIL && result != AUDIT_FAIL)
		{
			if (event == SYSTEMAUDIT_LOGOUT && clientInfo.pg_dump_restore != CMD_COMMON)
			{
				if (clientInfo.pg_dump_restore == CMD_PG_DUMP_SUC
					|| clientInfo.pg_dump_restore == CMD_PG_RESTORE_SUC
					|| clientInfo.pg_dump_restore == CMD_PG_DUMPALL_SUC
					|| clientInfo.pg_dump_restore == CMD_PG_BASEBACK_SUC)
				{
					return ;
				}
			}
			else
			{
				return ;
			}
		}
	}

	if (event == SYSTEMAUDIT_LOGOUT)
	{
		switch(clientInfo.pg_dump_restore)
		{
			case CMD_PG_DUMP_SUC:
				if (audit_GUC.audit_systemopers != AUDIT_OPER_FAIL)
					auditevent = generateSystemEvent(SYSTEMAUDIT_PG_DUMP, AUDIT_SUCCESS);
				break;
			case CMD_PG_DUMP_FAIL:
				if (audit_GUC.audit_systemopers != AUDIT_OPER_SUCC)
				{
					auditevent = generateSystemEvent(SYSTEMAUDIT_PG_DUMP, AUDIT_FAIL);
					is_fail = true;
				}
				break;
			case CMD_PG_RESTORE_SUC:
				if (audit_GUC.audit_systemopers != AUDIT_OPER_FAIL)
					auditevent = generateSystemEvent(SYSTEMAUDIT_PG_RESTORE, AUDIT_SUCCESS);
				break;
			case CMD_PG_RESTORE_FAIL:
				if (audit_GUC.audit_systemopers != AUDIT_OPER_SUCC)
				{
					auditevent = generateSystemEvent(SYSTEMAUDIT_PG_RESTORE, AUDIT_FAIL);
					is_fail = true;
				}
				break;
			case CMD_PG_DUMPALL_SUC:
				if (audit_GUC.audit_systemopers != AUDIT_OPER_FAIL)
					auditevent = generateSystemEvent(SYSTEMAUDIT_PG_DUMPALL, AUDIT_SUCCESS);
				break;
			case CMD_PG_DUMPALL_FAIL:
				if (audit_GUC.audit_systemopers != AUDIT_OPER_SUCC)
				{
					auditevent = generateSystemEvent(SYSTEMAUDIT_PG_DUMPALL, AUDIT_FAIL);
					is_fail = true;
				}
				break;
			case CMD_PG_BASEBACK_SUC:
				if (audit_GUC.audit_systemopers != AUDIT_OPER_FAIL)
					auditevent = generateSystemEvent(SYSTEMAUDIT_PG_BASEBACKUP,AUDIT_SUCCESS);
				break;
			case CMD_PG_BASEBACK_FAIL:
				if (audit_GUC.audit_systemopers != AUDIT_OPER_SUCC)
				{
					auditevent = generateSystemEvent(SYSTEMAUDIT_PG_BASEBACKUP, AUDIT_FAIL);
					is_fail = true;
				}
				break;

			default:
				break;
		}

		if (auditevent)
		{
			if (!RecoveryInProgress())
				audit_write_record_system(auditevent);
			pfree(auditevent);
		}

		/* Only when auditing fails, cannot record more successful exit information */
		if (is_fail && (audit_GUC.audit_systemopers == AUDIT_OPER_FAIL))
			return ;
	}

	auditevent = NULL;
	
	auditevent = generateSystemEvent(event, result);
	if (auditevent)
	{
		if (!RecoveryInProgress())
			audit_write_record_system(auditevent);
		pfree(auditevent);
	}

	auditevent = NULL;
}



static void 
audit_cmd(void)
{
	switch(clientInfo.pg_dump_restore)
	{
		case CMD_PG_DUMP_SUC:
			clientInfo.pg_dump_restore = CMD_PG_DUMP_FAIL;
			break;
		case CMD_PG_RESTORE_SUC:
			clientInfo.pg_dump_restore = CMD_PG_RESTORE_FAIL;
			break;
		case CMD_PG_DUMPALL_SUC:
			clientInfo.pg_dump_restore = CMD_PG_DUMPALL_FAIL;
			break;
		case CMD_PG_BASEBACK_SUC:
			clientInfo.pg_dump_restore = CMD_PG_BASEBACK_FAIL;
			break;
		default:
			break;
	}
}



static void 
audit_system_event_info(bool flag, SystemEventType sqlerrcode)
{
	if (audit_GUC.audit_enable != AUDIT_ON || audit_GUC.audit_systemopers == AUDIT_OPER_OFF || *audit_GUC.audit_pause)
	{
		return;
	}

	if (flag)
	{
		switch(sqlerrcode)
		{
			case SYSTEM_EVENT_START_SUCC:
				deal_with_system_event(SYSTEMAUDIT_START, AUDIT_SUCCESS);
				break;

			case SYSTEM_EVENT_START_FAIL:
				deal_with_system_event(SYSTEMAUDIT_START, AUDIT_FAIL);
				break;

			case SYSTEM_EVENT_SWITCHOVER_SUCC:
				deal_with_system_event(SYSTEMAUDIT_SWITCHOVER, AUDIT_SUCCESS);
				break;

			case SYSTEM_EVENT_LOGIN_SUCC:
				deal_with_system_event(SYSTEMAUDIT_LOGIN, AUDIT_SUCCESS);
				break;

			case SYSTEM_EVENT_LOGOUT_SUCC:
				deal_with_system_event(SYSTEMAUDIT_LOGOUT, AUDIT_SUCCESS);
				break;

			case SYSTEM_EVENT_RELOAD_SUCC:
				deal_with_system_event(SYSTEMAUDIT_RELOAD, AUDIT_SUCCESS);
				break;

			case SYSTEM_EVENT_STOP_SUCC:
				deal_with_system_event(SYSTEMAUDIT_STOP, AUDIT_SUCCESS);
				break;
			case SYSTEM_EVENT_BASEBACKUP_FAIL:
				clientInfo.pg_dump_restore = CMD_PG_BASEBACK_FAIL;
				deal_with_system_event(SYSTEMAUDIT_PG_BASEBACKUP, AUDIT_FAIL);
				break;
			case SYSTEM_EVENT_ALTER_GUC:
				deal_with_system_event(SYSTEMAUDIT_ALTER_GUC, AUDIT_SUCCESS);
				break;

			case SYSTEM_EVENT_SPACE_ALARM:
				deal_with_system_event(SYSTEMAUDIT_SPACE_ALARM, AUDIT_SUCCESS);
				break;

			case SYSTEM_EVENT_RECOVERY_SUCC:
				deal_with_system_event(SYSTEMAUDIT_AUDIT_RECOVERY, AUDIT_SUCCESS);
				break;

			case SYSTEM_EVENT_LOCK_SESSION:
				deal_with_system_event(SYSTEMAUDIT_AUDIT_LOCK_SESSION, AUDIT_SUCCESS);
				break;

			case SYSTEM_EVENT_UNLOCK_SESSION:
				deal_with_system_event(SYSTEMAUDIT_AUDIT_UNLOCK_SESSION, AUDIT_SUCCESS);
				break;

			case SYSTEM_EVENT_WALSENDER_CONN:
				deal_with_system_event(SYSTEMAUDIT_AUDIT_WALSENDER_CONN, AUDIT_SUCCESS);
				break;

			case SYSTEM_EVENT_WALSENDER_EXIT:
				deal_with_system_event(SYSTEMAUDIT_AUDIT_WALSENDER_EXIT, AUDIT_SUCCESS);
				break;

			case SYSTEM_EVENT_FAILOVER_SUCC:
				deal_with_system_event(SYSTEMAUDIT_FAILOVER, AUDIT_SUCCESS);
				break;

			case SYSTEM_EVENT_PROMOTE_SUCC:
				deal_with_system_event(SYSTEMAUDIT_PROMOTE, AUDIT_SUCCESS);
				break;

			case SYSTEM_EVENT_CHANEL:
				deal_with_system_event(SYSTEMAUDIT_CHANNEL, AUDIT_FAIL);
				break;

			default:
				deal_with_system_event(SYSTEMAUDIT_LOGOUT, AUDIT_SUCCESS);
				break;
		}

		return;
	}
#if defined(USE_SQLITE) && !defined(_WIN32)
	else
	{
		if (sqlite_open_db_for_audit_hook && sqlerrcode == ERRCODE_TOO_MANY_CONNECTIONS)
			sqlite_open_db_for_audit_hook(SYSTEMAUDIT_LOGIN, AUDIT_FAIL, MyProcPort->user_name, audit_msg);
		else if (sqlite_open_db_for_audit_hook && sqlerrcode == ERRCODE_T_R_DEADLOCK_DETECTED)
			sqlite_open_db_for_audit_hook(SYSTEMAUDIT_DEAD_LOCK, AUDIT_FAIL, MyProcPort->user_name, audit_msg);
		else if (sqlite_open_db_for_audit_hook && (sqlerrcode == ERRCODE_UNDEFINED_FILE
				|| sqlerrcode == ERRCODE_INSUFFICIENT_PRIVILEGE
				|| sqlerrcode == ERRCODE_DUPLICATE_FILE
				|| sqlerrcode == ERRCODE_WRONG_OBJECT_TYPE
				|| sqlerrcode == ERRCODE_OUT_OF_MEMORY
				|| sqlerrcode == ERRCODE_INSUFFICIENT_RESOURCES
				|| sqlerrcode == ERRCODE_IO_ERROR
				|| sqlerrcode == ERRCODE_INTERNAL_ERROR))
		{
			sqlite_open_db_for_audit_hook(SYSTEMAUDIT_OPEN_FILE_FAIL, AUDIT_FAIL, MyProcPort?MyProcPort->user_name:NULL, audit_msg);
		}
	}
#endif

	if (!flag && clientInfo.pg_dump_restore > '0' && sqlerrcode != ERRCODE_SUCCESSFUL_COMPLETION)
	{
		audit_cmd();
	}
}



/*
 * Function: audit_append_hook()
 *
 * Description:
 *	 Hook function centralized processing
 *
 */
static void 
audit_append_hook(void)
{
	next_ExecutorStart_hook = ExecutorStart_hook;
	ExecutorStart_hook = audit_ExecutorStart_hook;

	next_ProcessUtility_hook = ProcessUtility_hook;
	ProcessUtility_hook = audit_ProcessUtility_hook;

	next_ExecutorCheckPerms_hook = ExecutorCheckPerms_hook;
	ExecutorCheckPerms_hook = audit_ExecutorCheckPerms_hook;

	next_object_access_hook = object_access_hook;
	object_access_hook = audit_object_access_hook;

	audit_object_hook = audit_getobject;
	
	audit_CommandStart_hook = audit_CommandStart;

	audit_SaveConnectInfo_hook = audit_save_connectinfo;
	audit_Save_ConnectAuthMethod_hook = audit_Save_ConnectAuthMethod;

	audit_system_event_hook = audit_system_event_info;

	audit_aborted_hook = audit_aborted;
	audit_commited_hook = audit_commited;
	audit_privi_mode_hook = audit_privi_mode;
	audit_WriteRecord_hook = audit_WriteRecord;
	audit_WriteThrHistory_hook = audit_WriteThrHistory;

	audit_SetServerOption_hook = audit_set_server_option;
	audit_CreateArchJob_hook = audit_create_arch_job;
	audit_AlterSetGuc_hook = audit_alter_set_guc;
	audit_StopPgagent_hook = audit_stop_pgagent;
	audit_drop_pg_audit_hook = audit_drop_pgaudit;
	audit_alter_pg_audit_log_hook = audit_alter_pg_auditlog;
#ifdef USE_SQLITE
	sqlite_open_db_for_audit_hook = sqlite_open_db_for_audit;
#endif

	prev_shmem_startup_hook = shmem_startup_hook;
	shmem_startup_hook = AuditShememInit;

	is_audit_enable = is_enable_audit;
}

static void
pg_audit_sighup(SIGNAL_ARGS)
{
	int			save_errno = errno;

	got_sighup = true;
	SetLatch(MyLatch);

	errno = save_errno;
}

static void
pg_audit_wakeup(SIGNAL_ARGS)
{
	int			save_errno = errno;

	SetLatch(MyLatch);

	errno = save_errno;
}


static void
pg_audit_sigterm(SIGNAL_ARGS)
{
	int			save_errno = errno;

	got_sigterm = true;
	SetLatch(MyLatch);

	errno = save_errno;
}



static void
pgaudit_setup_memcxt(void)
{
	if (!pgAuditLocalContext)
		pgAuditLocalContext = AllocSetContextCreate(TopMemoryContext,
														"pg_audit_once",
														ALLOCSET_DEFAULT_SIZES);
}

/*
 * Record of modification of guc parameters that
 * take effect upon restart or receipt of sighup
 * signal
 */
static void
audit_postmaster_sighup_guc(char *context)
{
	StringInfoData buf;
	int			ret;
	TimestampTz 	audittime;
	const char	*time;
	char	timec[20];
	uint64	proc;
	Oid		relOid;
	Relation	audit_log;

	if (RecoveryInProgress())
		return;

	internalStatement = true;

	audittime = GetCurrentTimestamp();
	time = timestamptz_to_str(audittime);
	strncpy(timec, time, 19);
	timec[19] = '\0';

	initStringInfo(&buf);
	/* refresh snapshot */
	appendStringInfo(&buf, "select * from hg_security.pg_track_settings_snapshot()");
	
	SetCurrentStatementStartTimestamp();
	StartTransactionCommand();
	
	SPI_connect();
	PushActiveSnapshot(GetTransactionSnapshot());

	pgstat_report_activity(STATE_RUNNING, buf.data);
	
	ret = SPI_execute(buf.data, false, 0);

	if (ret != SPI_OK_SELECT)
	{
		internalStatement = false;
		elog(WARNING, "cannot select from hg_security.pg_track_settings_snapshot: error code %d", ret);
		return;
	}

	SPI_finish();
	PopActiveSnapshot();
	CommitTransactionCommand();
	pgstat_report_stat(false);
	pgstat_report_activity(STATE_IDLE, NULL);

	initStringInfo(&buf);
	
	/* compare pg_track_settings_history and pg_settings 
	 * to find the modified GUC parameters 
	 */
	appendStringInfo(&buf, "SELECT a.ts, a.name, a.setting "
							"FROM hg_security.pg_track_settings_history a "
							"INNER JOIN pg_settings b ON a.name = b.name "
							"WHERE b.context = '%s'", context);

	SetCurrentStatementStartTimestamp();
	StartTransactionCommand();
	SPI_connect();
	PushActiveSnapshot(GetTransactionSnapshot());
	pgstat_report_activity(STATE_RUNNING, buf.data);

	ret = SPI_execute(buf.data, true, 0);

	if (ret != SPI_OK_SELECT)
	{
		internalStatement = false;
		elog(WARNING, "cannot select from hg_security.pg_track_settings_history: error code %d", ret);
		return;
	}

	proc = SPI_processed;

	if (proc > 0)
	{
		SPITupleTable *spi_tuptable = SPI_tuptable;
		TupleDesc	spi_tupdesc = spi_tuptable->tupdesc;
		uint64		i;
				
		relOid = get_relname_relid(PG_AUDIT_LOG, SECURITY_SCHEMA_NAMESPACE);
		if (relOid == InvalidOid)
		{
			internalStatement = false;
			elog(WARNING, "could not get oid of relation pg_audit_log");
			return;
		}

		audit_log = table_open(relOid, RowExclusiveLock);
		
		for (i = 0; i < proc; i++)
		{
			HeapTuple	spi_tuple;
			char	   *ts;
			char		tsc[20];
			char	   *name;
			char	   *setting;
			AuditEvent *auditevent = NULL;
			int 		len;
			int			ret;
			char	   *sql = palloc0(MAXPGPATH);

			
			len =  INTALIGN(sizeof(AuditEvent));
			auditevent = palloc0(len);

			spi_tuple = spi_tuptable->vals[i];
			
			ts = SPI_getvalue(spi_tuple, spi_tupdesc, 1);

			memset(tsc, 0, 20);
			strncpy(tsc, ts, 19);
			tsc[19] = '\0';

			/* only when the time is the latest, it means it is just a modification
			 * made earlier, otherwrise it is a historical modification and will
			 * not be recorded
			 */
			if (strcmp(timec, tsc) != 0)
				continue;
				
			
			name = SPI_getvalue(spi_tuple, spi_tupdesc, 2);
			
			setting = SPI_getvalue(spi_tuple, spi_tupdesc, 3);

			if (pg_strcasecmp(name, "audit.enable") == 0)
			{
				ret = -2;
				if (pg_strcasecmp(setting, "on") == 0)
				{
					strcpy(auditevent->actionname, MANDATORY_AUDIT_ON_S);
				}
				else
				{
					strcpy(auditevent->actionname, MANDATORY_AUDIT_OFF_S);
				}
			}
			else
			{
				if ((audit_GUC.audit_enable == AUDIT_ON) && 
					(audit_GUC.audit_systemopers == AUDIT_OPER_SUCC ||
                     audit_GUC.audit_systemopers == AUDIT_OPER_ALL))
				{
					ret = -1;
					strcpy(auditevent->actionname, ALTER_GUC_S);
				}
				else
					continue;
	
			}
			
			auditevent->audittime = DatumGetTimestampTz(DirectFunctionCall3(timestamptz_in,
																 CStringGetDatum(ts),
																 ObjectIdGetDatum(InvalidOid),
																 Int32GetDatum(-1)));
			//auditevent->useroid = GetUserId_audit();
			auditevent->privilege = AUDIT_PRIVILEGE_INSTACE;
			//auditevent->sessionid = MyProcPid;

			sprintf(sql, "set %s='%s'", name, setting);
			auditevent->sqltext = pnstrdup(sql, 1023);
			auditevent->result = AUDIT_SUCCESS;
			auditevent->confoids[0] = 0;

			audit_insert_all_log(audit_log, auditevent, NULL, ret);

		}

		table_close(audit_log, RowExclusiveLock);
		audit_log = NULL;	
	}
	
	SPI_finish();
	PopActiveSnapshot();
	CommitTransactionCommand();
	pgstat_report_stat(false);
	pgstat_report_activity(STATE_IDLE, NULL);
	internalStatement = false;
}

/* When configuring the archive path, after restarting,
 * it is necessary to enable pagent scheduled tasks to
 * archive audit logs to the archive path on the hour
 */
static void
audit_arch_job(void)
{
	StringInfoData buf;
	/* Begin - BUG#00004708 */
	char	   *username = NULL;
	char		user[MAXPGPATH] = {0};
	/* End - BUG#00004708 */
	int			ret;
	uint64		proc;
	int			elemno;
	int			noids = 0;
	Oid 	   *oids;
	char		ch = '%';

	if (RecoveryInProgress())
		return;

	internalStatement = true;
	
	SetCurrentStatementStartTimestamp();
	StartTransactionCommand();
	
	SPI_connect();
	PushActiveSnapshot(GetTransactionSnapshot());
	
	initStringInfo(&buf);
	pgstat_report_activity(STATE_RUNNING, buf.data);
	appendStringInfo(&buf, "delete from pgagent.pga_job where jobname = 'Audit automatic archiving'");
	ret = SPI_execute(buf.data, false, 0);

	if (ret != SPI_OK_DELETE)
	{
		internalStatement = false;
		elog(WARNING, "cannot delete pgagent.pga_job: error code %d", ret);
		return;
	}

	initStringInfo(&buf);
	pgstat_report_activity(STATE_RUNNING, buf.data);
	appendStringInfo(&buf, "delete from pgagent.pga_jobstep where jstname = 'Audit automatic archiving'");
	ret = SPI_execute(buf.data, false, 0);

	if (ret != SPI_OK_DELETE)
	{
		internalStatement = false;
		elog(WARNING, "cannot update pgagent.pga_jobstep: error code %d", ret);
		return;
	}

	initStringInfo(&buf);
	pgstat_report_activity(STATE_RUNNING, buf.data);
	appendStringInfo(&buf, "delete from pgagent.pga_schedule where jscname = 'Audit automatic archiving'");
	ret = SPI_execute(buf.data, false, 0);

	if (ret != SPI_OK_DELETE)
	{
		internalStatement = false;
		elog(WARNING, "cannot update pgagent.pga_schedule: error code %d", ret);
		return;
	}

	SPI_finish();
	PopActiveSnapshot();
	CommitTransactionCommand();

	StartTransactionCommand();
	SPI_connect();
	PushActiveSnapshot(GetTransactionSnapshot());

	initStringInfo(&buf);
	pgstat_report_activity(STATE_RUNNING, buf.data);
	appendStringInfo(&buf, "select oid from pg_database where datallowconn = 't' and datistemplate = 'f'");
	ret = SPI_execute(buf.data, true, 0);

	if (ret != SPI_OK_SELECT)
	{
	
		internalStatement = false;
		elog(WARNING, "cannot select pg_database: error code %d", ret);
		return;
	}

	
	proc = SPI_processed;
	
	if (proc > 0)
	{
		SPITupleTable *spi_tuptable = SPI_tuptable;
		TupleDesc	spi_tupdesc = spi_tuptable->tupdesc;
		uint64		i;

		oids = (Oid *) palloc(proc * sizeof(Oid));
		
		for (i = 0; i < proc; i++)
		{
			HeapTuple	spi_tuple;
		
			spi_tuple = spi_tuptable->vals[i];

			oids[noids++] = atooid(SPI_getvalue(spi_tuple, spi_tupdesc, 1));
			
		}
	}

	/* Begin - BUG#00004708 */
	username = GetUserNameFromId(10, true);
	/* the memory requested by username may be flushed out when it is used ageain later */
	sprintf(user, "%s", username);
	/* End - BUG#00004708 */

	for (elemno = 0; elemno < noids; elemno++)
	{
		char *dbname;
		TimestampTz time;
		char   *time_str;
		char	timec[20];
		char   *archpath;
		char   *archfile;
		char	copysql[MAXPGPATH] = {0};
		/* Begin - BUG#00004708 */
		char	connstr[MAXPGPATH] = {0};
		/* End - BUG#00004708 */
		

		dbname = get_database_name(oids[elemno]);
		time = GetCurrentTimestamp();
		time_str = pstrdup(timestamptz_to_str(time));

		strncpy(timec, time_str, 19);
		timec[19] = '\0';

		archpath = palloc(MAXPGPATH);
		archfile = palloc(MAXPGPATH);
		
		snprintf(archpath, MAXPGPATH, "%s/%d", audit_GUC.audit_archdir, oids[elemno]);	/* Bug#Z5840 */
		if ((audit_GUC.remote_archdir != NULL && strlen(audit_GUC.remote_archdir) > 0) &&
							(audit_GUC.audit_archdir != NULL && strlen(audit_GUC.audit_archdir)))
		{
			snprintf(archfile, MAXPGPATH, "%s/%d.csv", archpath, oids[elemno]);	/* Bug#Z5840 */
			snprintf(copysql, sizeof(copysql), "COPY (SELECT * FROM hg_security.pg_audit_log) TO ''%s/%d.csv'';select hg_security.copy_file_to_remote_srv(''%s'', ''%s'');", archpath, oids[elemno], archfile, audit_GUC.remote_archdir);	/* BUG#Z5840 */	/* BUG#Z6166 */
		}
		else if ((audit_GUC.remote_archdir != NULL && strlen(audit_GUC.remote_archdir) > 0) &&
								(audit_GUC.audit_archdir != NULL && strlen(audit_GUC.audit_archdir) == 0))
		{
			snprintf(archfile, MAXPGPATH, "%s/%d.csv", DataDir, oids[elemno]);	/* Bug#Z5840 */
			snprintf(copysql, sizeof(copysql), "COPY (SELECT * FROM hg_security.pg_audit_log) TO ''%s/%d.csv'';select hg_security.copy_file_to_remote_srv(''%s'', ''%s'');", DataDir, oids[elemno], archfile, audit_GUC.remote_archdir); /* BUG#Z5840 */	/* BUG#Z6166 */
		}
		else if(audit_GUC.audit_archdir != NULL && strlen(audit_GUC.audit_archdir))
			snprintf(copysql, sizeof(copysql), "COPY (SELECT * FROM hg_security.pg_audit_log) TO ''%s/%d.csv''", archpath, oids[elemno]);	/* BUG#Z5840 */

		if (audit_GUC.audit_archdir != NULL && strlen(audit_GUC.audit_archdir))
		{
			ret = audit_mkdirs(archpath);
			if (ret < 0)
			{
				internalStatement = false;
				elog(WARNING, "could not create directory:%s :%m", archpath);
				return;
			}
		}

		/* Begin - BUG#00004708 */
		memset(connstr, 0 , MAXPGPATH);
		if (se_feature_mode)
		{
			sprintf(connstr, "host=localhost dbname=%s port=%d user=syssao", dbname, PostPortNumber);
		}
		else
		{
			
			sprintf(connstr, "host=localhost dbname=%s port=%d user=%s", dbname, PostPortNumber, user);
		}
		/* End - BUG#00004708 */

		initStringInfo(&buf);
		pgstat_report_activity(STATE_RUNNING, buf.data);
		appendStringInfo(&buf, "WITH id AS "
										 "(INSERT INTO pgagent.pga_job (jobjclid, jobname, jobdesc, "
										 "jobenabled, jobhostagent) "
										 "SELECT jcl.jclid, 'Audit automatic archiving', "
										 "'Audit automatic archiving every hour', true, '' "
										 "FROM pgagent.pga_jobclass jcl WHERE jclname='Routine Maintenance' "
										 "RETURNING jobid), "
										 "insertstep AS "
										 "(INSERT INTO pgagent.pga_jobstep (jstjobid, jstname, jstdesc, "
										 "jstenabled, jstkind, jstonerror, jstcode, jstdbname, jstconnstr) "
										 "SELECT id.jobid, 'Audit automatic archiving', "
										 "'Audit automatic archiving every hour', true, 's', 'f', "
										 "'%s', "
										 "'%s', '%s' FROM id) "
										 "INSERT INTO pgagent.pga_schedule (jscjobid, jscname, jscdesc, jscminutes, "
										 "jschours, jscweekdays, jscmonthdays, jscmonths, jscenabled, jscstart) "
										 "SELECT id.jobid, 'Audit automatic archiving', "
										 "'Audit automatic archiving every hour', "
										 "'{t,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f,f}', "
										 //"'{t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t}', "
										 "'{t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t}', "
										 "'{t,t,t,t,t,t,t}', "
										 "'{t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t,t}', "
										 "'{t,t,t,t,t,t,t,t,t,t,t,t}', true, '%s' FROM id", copysql, dbname, connstr, timec);
		
		ret = SPI_execute(buf.data, false, 0);
		
		if (ret != SPI_OK_INSERT)
		{
			internalStatement = false;
			elog(WARNING, "audit_create_arch_job: SPI_execute returned %d", ret);
			return;
		}		
	}

	SPI_finish();
	PopActiveSnapshot();

	CommitTransactionCommand();

	StartTransactionCommand();

	initStringInfo(&buf);

	SPI_connect();
	PushActiveSnapshot(GetTransactionSnapshot());
	pgstat_report_activity(STATE_RUNNING, buf.data);

	appendStringInfo(&buf, "select jstdbname from pgagent.pga_jobstep where jstname = 'Audit automatic archiving' "
							"and jstcode like '%c%s%c'", ch, audit_GUC.audit_archdir, ch);
	ret = SPI_execute(buf.data, true, 0);

	if (ret != SPI_OK_SELECT)
	{
		internalStatement = false;
		elog(WARNING, "cannot select from pgagent.pga_jobstep: error code %d", ret);
		return;
	}

	proc = SPI_processed;

	if (proc > 0)
	{
		SPITupleTable *spi_tuptable = SPI_tuptable;
		TupleDesc	spi_tupdesc = spi_tuptable->tupdesc;
		uint64		i;
		int			rc;

		for (i = 0; i < proc; i++)
		{
			HeapTuple	spi_tuple;
			char	   *dbname;
			char		cmd[MAXPGPATH];

			spi_tuple = spi_tuptable->vals[i];
			
			dbname = SPI_getvalue(spi_tuple, spi_tupdesc, 1);

			memset(cmd, 0, MAXPGPATH);

			/* Begin - BUG#00004708 */
			sprintf(cmd, "pgagent host=localhost dbname=%s port=%d user=%s ", dbname, PostPortNumber, user);
			/* End - BUG#00004708 */
			
			rc = system(cmd);

			if (rc != 0)
			{
#ifdef USE_SQLITE
				if (sqlite_open_db_for_audit_hook)
					sqlite_open_db_for_audit_hook(SYSTEMAUDIT_START_PGAGENT, AUDIT_FAIL, NULL, NULL);
#endif
				elog(WARNING, "cannot start pgagent service");
				continue;
			}
#ifdef USE_SQLITE
			if (sqlite_open_db_for_audit_hook)
				sqlite_open_db_for_audit_hook(SYSTEMAUDIT_START_PGAGENT, AUDIT_SUCCESS, NULL, NULL);
#endif
		}
	}
	
	SPI_finish();
	PopActiveSnapshot();
	CommitTransactionCommand();
	pgstat_report_stat(false);
	pgstat_report_activity(STATE_IDLE, NULL);
	internalStatement = false;
}


#ifdef USE_SQLITE
/* If the path of sqlite is modified, the path also
 * needs to be modified in pg_foreign_server
 */
static void
audit_set_sqlite_path(void)
{
	StringInfoData buf;
	uint64		proc;
	int			ret;

	if (RecoveryInProgress())
		return;

	internalStatement = true;
	
	SetCurrentStatementStartTimestamp();
	StartTransactionCommand();
	
	SPI_connect();
	PushActiveSnapshot(GetTransactionSnapshot());

	initStringInfo(&buf);
	pgstat_report_activity(STATE_RUNNING, buf.data);
	appendStringInfo(&buf, "select srvoptions from pg_foreign_server where srvname='sqlite_server'");
	ret = SPI_execute(buf.data, true, 0);
	
	if (ret != SPI_OK_SELECT)
	{
		internalStatement = false;
		elog(WARNING, "cannot select srvoptions of sqlite_server: error code %d", ret);
		return;
	}
	else
	{ 	
		proc = SPI_processed;
		
		if (proc > 0)
		{
			SPITupleTable *spi_tuptable = SPI_tuptable;
			TupleDesc	spi_tupdesc = spi_tuptable->tupdesc;
			uint64		i;
			
			for (i = 0; i < proc; i++)
			{
				HeapTuple	spi_tuple;
				char	   *srvoptions;
				char	   *database;

				spi_tuple = spi_tuptable->vals[i];
				
				srvoptions = SPI_getvalue(spi_tuple, spi_tupdesc, 1);

				database = strstr(srvoptions, "=");
				database = database + 1;
				database[strlen(database) - 1] = '\0';

				if (strcmp(database, audit_GUC.audit_sqlitedbpath) == 0)
				{
					SPI_finish();
					PopActiveSnapshot();
					CommitTransactionCommand();
					pgstat_report_stat(false);
					pgstat_report_activity(STATE_IDLE, NULL);
					internalStatement = false;
					return;
				}
			}
		}
	}

	initStringInfo(&buf);
	pgstat_report_activity(STATE_RUNNING, buf.data);
	appendStringInfo(&buf, "update pg_foreign_server set srvoptions='{database=%s}' where srvname = 'sqlite_server'", audit_GUC.audit_sqlitedbpath);
	ret = SPI_execute(buf.data, false, 0);
	
	if (ret != SPI_OK_UPDATE)
	{
		internalStatement = false;
		elog(WARNING, "cannot update srvoptions of sqlite_server: error code %d", ret);
		return;
	}
	
	SPI_finish();
	PopActiveSnapshot();
	CommitTransactionCommand();
	pgstat_report_stat(false);
	pgstat_report_activity(STATE_IDLE, NULL);
	internalStatement = false;
}
#endif

static void audit_delete_log(TimestampTz del_time)
{
	StringInfoData query;
	char *time;

	Oid save_userid;
	Oid syssao_oid;
	int save_sec_context;

	if (SPI_connect() != SPI_OK_CONNECT)
		elog(ERROR, "SPI_connect failed");

	time = DatumGetCString(DirectFunctionCall1(timestamptz_out, TimestampTzGetDatum(del_time)));
	initStringInfo(&query);
	appendStringInfo(&query, "delete from hg_security.pg_audit_log where audittime <\' %s\'", time);

	syssao_oid = get_role_oid("syssao", false);
	GetUserIdAndSecContext(&save_userid, &save_sec_context);

	SetUserIdAndSecContext(syssao_oid,
						save_sec_context | SECURITY_RESTRICTED_OPERATION);

	if (SPI_execute(query.data, false, 0) != SPI_OK_DELETE)
	{
		/* Restore userid and security context */
		SetUserIdAndSecContext(save_userid, save_sec_context);
		ereport(ERROR,
				(errcode(ERRCODE_DATA_EXCEPTION),
				errmsg("invalid query")));
	}

	/* Restore userid and security context */
	SetUserIdAndSecContext(save_userid, save_sec_context);

	if (SPI_finish() != SPI_OK_FINISH)
		elog(ERROR, "SPI_finish failed");

	return;
}

static int64 audit_get_total_log_size(char *table)
{
	int64 res;

	res =  Int64GetDatum(DirectFunctionCall1(pg_total_relation_size, get_relname_relid(table, SECURITY_SCHEMA_NAMESPACE)));

	return res;
}

static void get_disk_space(const char *path, uint64 *total, uint64 *free)
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

static int
audit_get_tablespace_path(StringInfoData *space_path)
{
	StringInfoData buf;
	uint64          proc;
	int                     ret;
	int flag = -1;
	MemoryContext oldcontext = CurrentMemoryContext;

	if (RecoveryInProgress())
		return flag;

	internalStatement = true;

	if (SPI_connect() != SPI_OK_CONNECT)
		elog(ERROR, "SPI_connect failed");

	initStringInfo(&buf);
	appendStringInfo(&buf, "SELECT pg_tablespace_location(oid) AS location FROM pg_tablespace WHERE spcname = 'audtblspc';");
	ret = SPI_execute(buf.data, true, 0);

	if (ret != SPI_OK_SELECT)
	{
		internalStatement = false;
		elog(WARNING, "cannot select srvoptions of sqlite_server: error code %d", ret);
		return flag;
	}
	else
	{
		proc = SPI_processed;

		if (proc == 1)
		{
			SPITupleTable *spi_tuptable = SPI_tuptable;
			TupleDesc       spi_tupdesc = spi_tuptable->tupdesc;
			char       *srvoptions;

			srvoptions = SPI_getvalue(spi_tuptable->vals[0], spi_tupdesc, 1);

			MemoryContextSwitchTo(oldcontext);
			initStringInfo(space_path);
			appendStringInfo(space_path, "%s", srvoptions);
			flag = 1;
		}
		else if (proc == 0)
		{
			flag = 0;
		}
	}

	internalStatement = false;

	if (SPI_finish() != SPI_OK_FINISH)
		elog(ERROR, "SPI_finish failed");

	return flag;
}

static bool is_disk_full(void)
{
	uint64 totle = 0;
	uint64 free = 0;
	StringInfoData buf;
	int ret = -1;
	bool is_full = false;

	/* get custom tablespace path*/
	ret = audit_get_tablespace_path(&buf);
	if (ret >= 0)
	{
		if (ret == 1)
		{
			/*get size of disk where the custom tablespace is located.*/
			get_disk_space(buf.data, &totle, &free);
		}
		else
		{
			/* The tablespace is not defined, so get datadir path */
			get_disk_space(DataDir, &totle, &free);
		}

		if (free < CACHE_SPACE)
		{
			is_full = true;
		}
	}

	return is_full;
}

static void audit_disk_full(void)
{
	uint64 totle = 0;
	uint64 free = 0;
	StringInfoData buf;
	int ret = -1;
	static bool disk_full = false;

	/* get custom tablespace path*/
	ret = audit_get_tablespace_path(&buf);
	if (ret >= 0)
	{
		if (ret == 1)
		{
			/*get size of disk where the custom tablespace is located.*/
			get_disk_space(buf.data, &totle, &free);
		}
		else
		{
			/* The tablespace is not defined, so get datadir path */
			get_disk_space(DataDir, &totle, &free);
		}

		if (free < CACHE_SPACE && *audit_GUC.audit_pause == false)
		{

#ifdef USE_SQLITE
			if (sqlite_open_db_for_audit_hook && (audit_GUC.logpolicy== AUDIT_PAUSE || (!disk_full && audit_GUC.logpolicy== AUDIT_CYCLE)))
			{
				sqlite_open_db_for_audit_hook(SYSTEMAUDIT_AUDIT_DISK_FULL, AUDIT_SUCCESS, NULL, NULL);
			}
#endif
			if (audit_GUC.logpolicy == AUDIT_PAUSE)
				*audit_GUC.audit_pause = true;
		#if defined(USE_SQLITE) && defined(ENABLE_DKY_BETA_FEATURE)
			else if (audit_GUC.logpolicy == AUDIT_CYCLE)
			{
				audit_register_all_db_work(AUDIT_DROP);
			}
		#endif

			if (!disk_full)
			{
				disk_full = true;
				ereport(WARNING,(errcode(ERRCODE_DISK_FULL),errmsg("Disk space is almost full and pause audit")));
			}
		}
		else if (free >= CACHE_SPACE && *audit_GUC.audit_pause == true)
		{
				*audit_GUC.audit_pause = false;
				disk_full = false;
				ereport(INFO,(errmsg("Disk space is enough and recover audit")));
		}
	}
}

static void send_signal_launch_proc(void)
{
	slist_mutable_iter iter;

	if (!IsUnderPostmaster)
	{
		return;
	}

	slist_foreach_modify(iter, &BackgroundWorkerList)
	{
		RegisteredBgWorker *rw;

		rw = slist_container(RegisteredBgWorker, rw_lnode, iter.cur);

		if (rw->rw_pid != getpid() && strncmp(rw->rw_worker.bgw_name, AUDIT_LAUNCHER_S, strlen(AUDIT_LAUNCHER_S) + 1) == 0)
		{
			kill(rw->rw_pid, SIGUSR2);
			break;
		}
	}
	return ;
}

static uint64 get_partiton_table_size(char *table)
{
	int ret = 0;
	StringInfoData buf;
	uint64 proc;
	uint64 table_size = 0;

	internalStatement = true;

	if (SPI_connect() != SPI_OK_CONNECT)
		elog(ERROR, "SPI_connect failed");

	initStringInfo(&buf);
	appendStringInfo(&buf, "select SUM(pg_total_relation_size(child.oid)) from pg_inherits "
							"join pg_class parent on pg_inherits.inhparent= parent.oid "
							"join pg_class child on pg_inherits.inhrelid= child.oid where parent.relname='%s';", table);
	ret = SPI_execute(buf.data, false, 0);

	pfree(buf.data);

	if (ret != SPI_OK_SELECT)
	{
		internalStatement = false;
		elog(WARNING, "cannot select pg_database: error code %d", ret);
		SPI_finish();
		return 0;
	}
	proc = SPI_processed;

	if (proc == 1)
	{
		SPITupleTable *spi_tuptable = SPI_tuptable;
		TupleDesc		spi_tupdesc = spi_tuptable->tupdesc;
		char *str = SPI_getvalue(spi_tuptable->vals[0], spi_tupdesc, 1);

		if (str != NULL)
			table_size = strtoul(str, NULL, 10);
		else
			table_size = 0;
	}
	else if (proc != 1)
	{
		internalStatement = false;
		elog(ERROR, "get partition table:pg_audit_log size fail! ");
	}
	internalStatement = false;

	if (SPI_finish() != SPI_OK_FINISH)
		elog(ERROR, "SPI_finish failed");

	return table_size;
}

static bool is_exceeded_disklimit(char relkind)
{
	static bool is_alarmed = false;
	static bool is_limited = false;
	uint64 table_size = 0;
	bool audit_pause = *audit_GUC.audit_pause;

	if (*audit_GUC.audit_pause)
		return true;

	if (audit_GUC.audit_retentionpolicy_switch == RETEN_TIME)
	{
		PushActiveSnapshot(GetTransactionSnapshot());
		if (is_disk_full())
			audit_pause = true;
		PopActiveSnapshot();
	}
	else if (audit_GUC.audit_retentionpolicy_switch == RETEN_SPACE)
	{
		PushActiveSnapshot(GetTransactionSnapshot());
		/* get table 'hg_audit_log' total size */
		if (relkind == RELKIND_PARTITIONED_TABLE)
			table_size = get_partiton_table_size(PG_AUDIT_LOG);
		else
			table_size = audit_get_total_log_size(PG_AUDIT_LOG);

		/* threshold alarm */
		if (audit_GUC.audit_spacealarm > 0 && !is_alarmed && table_size >= audit_GUC.audit_spacealarm * audit_GUC.audit_spacelimit
			&& table_size < audit_GUC.audit_spacelimit)
		{
			is_alarmed = true;
			send_signal_launch_proc();
		}
		else if (!is_limited && table_size >= audit_GUC.audit_spacelimit)
		{
			is_limited = true;
			audit_pause = true;
		}
		else if (table_size < audit_GUC.audit_spacelimit && *audit_GUC.audit_pause == true)
		{
			audit_pause = false;
			is_limited = false;
		}
		else if (audit_GUC.logpolicy == AUDIT_CYCLE && table_size >= audit_GUC.audit_spacelimit)
		{
			is_limited = true;
			audit_pause = true;
		}

		if (is_disk_full())
		{
			audit_pause = true;
		}

		PopActiveSnapshot();

	}

	if (audit_pause)
	{
		send_signal_launch_proc();
		return true;
	}
	else
		return false;
}

/*
 * Description:
 *      Perform differentiated checks based on temporal and spatial patterns, then delete the logs
 *
 */
static void audit_choice_retentionpolicy_mode(void)
{
	static bool is_alarmed = false;
	static bool is_limited = false;
	static TimestampTz pre_check_time = 0;
	static char relkind = '\0';
	TimestampTz cur_time = 0;
	TimestampTz del_time = 0;
	uint64 table_size = 0;

	if (audit_GUC.audit_enable != AUDIT_ON)
		return;

	if (audit_GUC.audit_retentionpolicy_switch == RETEN_TIME)
	{
		cur_time = GetCurrentTimestamp();

		SetCurrentStatementStartTimestamp();
		StartTransactionCommand();
		PushActiveSnapshot(GetTransactionSnapshot());

		if ((pre_check_time == 0 || (cur_time - audit_GUC.audit_detectiontime * USECS_PER_DAY) > pre_check_time))
		{
			pre_check_time = cur_time;
			del_time = cur_time - audit_GUC.audit_retentiondays * USECS_PER_DAY;

			/* delete hg_audit_log */
			audit_delete_log(del_time);
		}
		audit_disk_full();

		PopActiveSnapshot();
		CommitTransactionCommand();
	}
	else if (audit_GUC.audit_retentionpolicy_switch == RETEN_SPACE)
	{
		SetCurrentStatementStartTimestamp();
		StartTransactionCommand();
		PushActiveSnapshot(GetTransactionSnapshot());

		/* get table 'hg_audit_log' total size */
		if (relkind == '\0')
		{
			Oid relOid = InvalidOid;

			relOid = get_relname_relid(PG_AUDIT_LOG, SECURITY_SCHEMA_NAMESPACE);
			if (relOid == InvalidOid)
				elog(ERROR, " could not get oid of relation pg_audit_log");

			relkind = get_rel_relkind(relOid);
		}

		if (relkind == RELKIND_PARTITIONED_TABLE)
			table_size = get_partiton_table_size(PG_AUDIT_LOG);
		else
			table_size = audit_get_total_log_size(PG_AUDIT_LOG);

		/* threshold alarm */
		if (audit_GUC.audit_spacealarm > 0 && !is_alarmed && table_size >= audit_GUC.audit_spacealarm * audit_GUC.audit_spacelimit
				&& table_size < audit_GUC.audit_spacelimit)
		{
			is_alarmed = true;
			ereport(WARNING,
					errmsg("The log totle size Execeed alarm"));

			#ifdef USE_SQLITE
			if (sqlite_open_db_for_audit_hook)
				sqlite_open_db_for_audit_hook(SYSTEMAUDIT_SPACE_ALARM, AUDIT_SUCCESS, NULL, NULL);
			#endif
		}
		else if (!is_limited && table_size >= audit_GUC.audit_spacelimit)
		{

			#ifdef USE_SQLITE
				if (sqlite_open_db_for_audit_hook)
				{
						sqlite_open_db_for_audit_hook(SYSTEMAUDIT_AUDIT_SPACE_LIMIT, AUDIT_SUCCESS, NULL, NULL);
				}
			#endif

			if (audit_GUC.logpolicy == AUDIT_PAUSE)
			{
				*audit_GUC.audit_pause = true;
				is_limited = true;
			}
		#if defined(USE_SQLITE) && defined(ENABLE_DKY_BETA_FEATURE)
			else if (audit_GUC.logpolicy == AUDIT_CYCLE)
			{
				audit_register_all_db_work(AUDIT_DROP);
			}
		#endif
			ereport(WARNING,(errcode(ERRCODE_DISK_FULL),
					errmsg("Disk space is almost full and pause audit, table_size:%lu >= table_limit:%lu", table_size, audit_GUC.audit_spacelimit)));
		}
		else if (table_size < audit_GUC.audit_spacelimit && *audit_GUC.audit_pause == true && !is_disk_full())
		{
			*audit_GUC.audit_pause = false;
			is_limited = false;
			ereport(WARNING,(errmsg("Disk space is enough and recover audit")));
		}

		if (!*audit_GUC.audit_pause)
		{
			audit_disk_full();
		}

		PopActiveSnapshot();
		CommitTransactionCommand();
	}
}

#define LOG_ATTR_NUMS 8

static TupleDesc create_diskinfo_tuple_desc()
{
	TupleDesc tupdesc;

	tupdesc = CreateTemplateTupleDesc(LOG_ATTR_NUMS, false);

	TupleDescInitEntry(tupdesc, (AttrNumber) 1, "disk_size",
				TEXTOID, -1, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 2, "disk_used",
				TEXTOID, -1, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 3, "disk_used%",
				TEXTOID, -1, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 4, "limit",
				TEXTOID, -1, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 5, "table_used",
				TEXTOID, -1, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 6, "avail",
				TEXTOID, -1, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 7, "table_use%",
				TEXTOID, -1, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 8, "alarm",
				TEXTOID, -1, 0);

	return tupdesc;
}

static void audit_get_disk_info(uint64 *totle, uint64 *free)
{
	StringInfoData buf;
	int ret = -1;

	/* get custom tablespace path*/
	ret = audit_get_tablespace_path(&buf);
	if (ret >= 0)
	{
		if (ret == 1)
		{
			/*get size of disk where the custom tablespace is located.*/
			get_disk_space(buf.data, totle, free);
		}
		else
		{
			/* The tablespace is not defined, so get datadir path */
			get_disk_space(DataDir, totle, free);
		}

		//pfree(buf.data);
	}
}

Datum
audit_show_diskinfo(PG_FUNCTION_ARGS)
{
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
	TupleDesc	   tupdesc;
	Tuplestorestate *tupstore;
	MemoryContext oldcontext;
	uint64 totle = 0;
	uint64 free = 0;
	char buffer[21];
	int64 table_size = 0;
	static char relkind = '\0';

	Datum	  values[LOG_ATTR_NUMS];
	bool 		   nulls[LOG_ATTR_NUMS];

	/* permission check */
	check_audit_sao_permission(GetUserId(), false);

	if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
		   ereport(ERROR,
						   (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							errmsg("set-valued function called in context that cannot accept a set")));

	if (!(rsinfo->allowedModes & SFRM_Materialize))
			ereport(ERROR,
							(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							 errmsg("materialize mode required, but it is not allowed in this context")));

	MemSet(values, 0, sizeof(values));
	MemSet(nulls, 0, sizeof(nulls));


	oldcontext = MemoryContextSwitchTo(rsinfo->econtext->ecxt_per_query_memory);
	tupdesc = create_diskinfo_tuple_desc();
	tupstore =
			tuplestore_begin_heap(rsinfo->allowedModes & SFRM_Materialize_Random,
											   false, work_mem);
	MemoryContextSwitchTo(oldcontext);

	audit_get_disk_info(&totle, &free);

	MemSet(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%lu", totle);
	values[0] = PointerGetDatum(cstring_to_text(buffer));

	MemSet(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%lu", totle - free);
	values[1] = PointerGetDatum(cstring_to_text(buffer));

	MemSet(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%.2f%%", (((double)(totle - free))/totle)*100);
	values[2] = PointerGetDatum(cstring_to_text(buffer));

	MemSet(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%lu", audit_GUC.audit_spacelimit);
	values[3] = PointerGetDatum(cstring_to_text(buffer));

	/* get table 'hg_audit_log' total size */
	if (relkind == '\0')
	{
		Oid relOid = InvalidOid;

		relOid = get_relname_relid(PG_AUDIT_LOG, SECURITY_SCHEMA_NAMESPACE);
		if (relOid == InvalidOid)
			elog(ERROR, " could not get oid of relation pg_audit_log");

		relkind = get_rel_relkind(relOid);
	}

	if (relkind == RELKIND_PARTITIONED_TABLE)
		table_size = get_partiton_table_size(PG_AUDIT_LOG);
	else
		table_size = audit_get_total_log_size(PG_AUDIT_LOG);

	MemSet(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%ld", table_size);
	values[4] = PointerGetDatum(cstring_to_text(buffer));

	MemSet(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%ld", audit_GUC.audit_spacelimit - table_size);
	values[5] = PointerGetDatum(cstring_to_text(buffer));

	MemSet(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%.2f%%", (((double)(table_size))/audit_GUC.audit_spacelimit)*100);
	values[6] = PointerGetDatum(cstring_to_text(buffer));

	MemSet(buffer, 0, sizeof(buffer));
	snprintf(buffer, sizeof(buffer), "%lu", (uint64)(audit_GUC.audit_spacealarm * audit_GUC.audit_spacelimit));
	values[7] = PointerGetDatum(cstring_to_text(buffer));

	tuplestore_putvalues(tupstore, tupdesc, values, nulls);
	tuplestore_donestoring(tupstore);

	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = tupstore;
	rsinfo->setDesc = tupdesc;

	PG_RETURN_NULL();
}

/* Functions of backend precess */
void
pg_audit_launch(void)
{
	/* Establish signal handlers before unblocking signals. */
	pqsignal(SIGHUP, pg_audit_sighup);//wakeup latch
	pqsignal(SIGTERM, pg_audit_sigterm);//backend
	pqsignal(SIGUSR1, procsignal_sigusr1_handler);
	pqsignal(SIGUSR2, pg_audit_wakeup);

	/* We're now ready to receive signals */
	BackgroundWorkerUnblockSignals();
	pgaudit_setup_memcxt();
	old_context = MemoryContextSwitchTo(pgAuditLocalContext);

	/* Connect to our database */
	BackgroundWorkerInitializeConnection(DatabaseName, NULL, 0);

#if defined(USE_SQLITE) && defined(ENABLE_DKY_BETA_FEATURE)

	/* create partition table first. mandatory audit must use table */
	StartTransactionCommand();
	PushActiveSnapshot(GetTransactionSnapshot());
	audit_register_all_db_work(AUDIT_CREATE);
	PopActiveSnapshot();
	CommitTransactionCommand();
#endif

	/* If audit_GUC.track_settings is off then pg_track_settings_snapshot
	 * function will not be called to prevent it from affecting regress
	 * test of wal.
	 */
	if (audit_GUC.track_settings == AUDIT_ON)
	{
		audit_postmaster_sighup_guc("postmaster");
	}

	/* When audit_enable is on and the archvie path is not empty, 
	 * pagent scheduled task needs to be initiated for archiving
	 */
	if (audit_GUC.audit_enable == AUDIT_ON && ((audit_GUC.audit_archdir != NULL && strlen(audit_GUC.audit_archdir) > 0) ||
						(audit_GUC.remote_archdir != NULL && strlen(audit_GUC.remote_archdir) > 0)))
	{
		audit_arch_job();
	}
#ifdef USE_SQLITE
	/* If the path of sqlite is modified, the path also
	 * needs to be modified in pg_foreign_server
	 */
	if (audit_GUC.audit_enable == AUDIT_ON && audit_GUC.audit_systemopers != AUDIT_OPER_OFF && 
		audit_GUC.audit_sqlitedbpath != NULL && strlen(audit_GUC.audit_sqlitedbpath) > 0)
	{
		audit_set_sqlite_path();
	}
#endif

	if (!RecoveryInProgress())
	{
		/*
		 * Main loop: do this until the SIGTERM handler tells us to terminate
		 */
		while (!got_sigterm)
		{
			/* check based on time or space*/
				audit_choice_retentionpolicy_mode();

			#if defined(USE_SQLITE) && defined(ENABLE_DKY_BETA_FEATURE)
				/* create partition table first. mandatory audit must use table */
				StartTransactionCommand();
				PushActiveSnapshot(GetTransactionSnapshot());
				audit_register_all_db_work(AUDIT_CREATE);
				PopActiveSnapshot();
				CommitTransactionCommand();
			#endif

			/*
			 * In case of a SIGHUP, just reload the configuration.
			 */
			if (got_sighup)
			{
				got_sighup = false;
				ProcessConfigFile(PGC_SIGHUP);

				/* record the modification of guc parameters
				 * that take effect when receiving sighup signals
				 */
				if (audit_GUC.audit_enable == AUDIT_ON && audit_GUC.audit_systemopers != AUDIT_OPER_OFF
					&& audit_GUC.audit_systemopers != AUDIT_OPER_FAIL)
				{
					audit_postmaster_sighup_guc("sighup");
				}
			}

			/*
			 * Background workers mustn't call usleep() or any direct equivalent:
			 * instead, they may wait on their process latch, which sleeps as
			 * necessary, but is awakened if postmaster dies.  That way the
			 * background process goes away immediately in an emergency.
			 */
			(void) WaitLatch(MyLatch,
							 WL_LATCH_SET | WL_TIMEOUT | WL_EXIT_ON_PM_DEATH,
							 worker_naptime * 1000L,
							 PG_WAIT_EXTENSION);
			ResetLatch(MyLatch);

			CHECK_FOR_INTERRUPTS();

		}
	}
	MemoryContextSwitchTo(old_context);
	MemoryContextReset(pgAuditLocalContext);
	proc_exit(1);
}

/**
 * Validates a string against the following rules:
 * - Composition: Only digits (0-9), at most one decimal point (.),
 *   and optionally one percent sign (%) at the end
 * - Percent Sign: If present, must be the last character and unique
 * - Decimal Point: Prohibited at the start, allowed only once
 */
static int is_valid_string(const char *str)
{
	bool has_percent = false;
	int dot_count = 0;

	int length = strlen(str);
	if (length == 0)
		return -1;

	// check the last char is %
	if (str[length - 1] == '%')
	{
		has_percent = true;
		length--;

		if (length == 0)
			return false;
	}

	for (int i = 0; i < length; i++)
	{
		char c = str[i];

		// check allow num
		if (c >= '0' && c <= '9')
		{
			continue;
		}
		// check dot
		else if (c == '.')
		{
			dot_count++;
			if (dot_count > 1 || i == 0 ||(i == length - 1))
				return -1;
		}
		// other char is invalid
		else
		{
			return -1;
		}
	}

	if (has_percent)
	{
		if (str[length] != '%')
			return -1;
		else
			return 1;
	}

	return 0;
}

static bool
audit_real_check_hook(char **newvalue, void **extra, GucSource source)
{
	double num = 0;
	int re = 0;
#if 0
	if (IsNormalProcessingMode() || (IsUnderPostmaster && MyProcPort))
	{
		if (RecoveryInProgress())
			return false;
	}
#endif
	if (*newvalue == NULL || *newvalue[0]== '\0')
	{
		audit_GUC.audit_spacealarm = 0;
		return true;
	}
	re = is_valid_string(*newvalue);

	if (re < 0)
	{
		return false;
	}

	num = atof(*newvalue);
	if (re == 0)
	{
		if (num < 0 || num >= 1)
			return false;

		audit_GUC.audit_spacealarm = num;
	}
	else
	{
		if (num < 0 || num >= 100)
			return false;

		audit_GUC.audit_spacealarm = num / 100;
	}

	return true;
}


/*
 * Function: check_audit_sao_permission()
 *
 * Description: 
 * 	 permission check
 *
 * Input:
 *	istable: Is it permission to check the table?
 *
 */
static void 
check_audit_sao_permission(Oid roid, bool istable)
{
	if (se_feature_mode)
	{
		/* Begin - BUG#00004872 */
		if (!IS_SYSSAO(roid))
		{
			if (istable)
				ereport(ERROR,
							(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							 errmsg("permission denied, only role=syssao_role can select this table")));
			else
				ereport(ERROR,
							(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							 errmsg("permission denied, only role=syssao_role can run this function")));
		}
		/* End - BUG#00004872 */
	}
	else
	{
		if (!superuser())
		{
			if (istable)
				ereport(ERROR,
							(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							 errmsg("permission denied, only superuser can select this table")));
			else
				ereport(ERROR,
							(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							 errmsg("permission denied, only superuser can run this function")));
		}
	}
	return;
}

static void
check_audit_saosso_perm(Oid roid, bool istable)
{	
	if (se_feature_mode)
	{
		/* Begin - BUG#00004872 */
		if (audit_GUC.audit_privimode == 1)
		{
			if (!IS_SYSSAO(roid))
			{
				if (istable)
					ereport(ERROR,
								(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
								 errmsg("permission denied, only role=syssao_role can select this table")));
				else
					ereport(ERROR,
								(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
								 errmsg("permission denied, only role=syssao_role can run this function")));
			}
		}
		else
		{
			if (!IS_SYSSAO(roid) && (!IS_SYSSSO(roid)))
			{
				if (istable)
					ereport(ERROR,
								(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
								 errmsg("permission denied, only role=syssao_role or role=syssso_role can select this table")));
				else
					ereport(ERROR,
								(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
								 errmsg("permission denied, only role=syssao_role or role=syssso_role can run this function")));

			}
		}
		/* End - BUG#00004872 */
	}
	else
	{
		if (!superuser())
		{
			if (istable)
				ereport(ERROR,
							(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							 errmsg("permission denied, only superuser can select this table")));
			else
				ereport(ERROR,
							(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							 errmsg("permission denied, only superuser can run this function")));
		}
	}
	return;
}


static void 
audit_create_usr_list_syssao(void)
{
	auditedrole_syssao = NIL;
	
	auditedrole_syssao = lcons(makeString(SYSSSO_ROLE), auditedrole_syssao);
	auditedrole_syssao = lappend(auditedrole_syssao, makeString(SYSDBA_ROLE));
}

static void 
audit_create_usr_list_syssso(void)
{
	auditedrole_syssso = NIL;
	
	auditedrole_syssso = lcons(makeString(SYS_COMMON), auditedrole_syssso);
	auditedrole_syssso = lappend(auditedrole_syssso, makeString(SYSSAO_ROLE));
}



/*
 * Function: audit_change_where_clause
 * Description: Add restrictions to Where Clause
 * Input:
 *	n: Analyze tree nodes
 * Return��
 *	Where parse tree
 */
static Node *
audit_change_where_clause(Node *clause)
{
	BoolExpr   *BoolExprNode = NULL;
	Node	*right_Node = NULL;
	Node	*left_Node = NULL;
	Node *NullTestNode = NULL;
	ListCell *listcell;
	int num = 0;
	bool flag = false;
	Oid		useroid;

	if (audit_GUC.audit_privimode == 1)
		return clause;

	if (audit_refname)
	{
		useroid = GetUserId();
		
		if (IS_SYSSAO(useroid))
		{
			flag = true;

			audit_create_usr_list_syssao();
			
			foreach(listcell, auditedrole_syssao)
			{
				Value *str = lfirst(listcell);

				num++;
				if (num == 1)
				{
					
					right_Node = (Node *)Audit_A_Expr("rolename", strVal(str), "=");
				}
				else
				{
					left_Node = (Node *)Audit_A_Expr("rolename", strVal(str), "=");
					right_Node = (Node *)make_BoolExprNode(OR_EXPR, left_Node, right_Node);
				}
			}
		}
		else if (IS_SYSSSO(useroid))
		{
			flag = true;

			audit_create_usr_list_syssso();
			
			foreach(listcell, auditedrole_syssso)
			{
				Value *str = lfirst(listcell);

				num++;
				if (num == 1)
				{
					/* common: Other users except sysdba syssao syssso */
					right_Node = (Node *)Audit_make_BoolExprNode(AND_EXPR, "rolename", "<>", SYSSSO_ROLE,
									"rolename",  "<>", SYSDBA_ROLE);
					left_Node = (Node *)Audit_A_Expr("rolename", SYSSAO_ROLE, "<>");
					right_Node = (Node *)make_BoolExprNode(AND_EXPR, left_Node, right_Node);

					/* The situation where the username column has no value*/
					NullTestNode = make_NullTest("rolename");
					right_Node = (Node *)make_BoolExprNode(OR_EXPR, NullTestNode, (Node *)right_Node);
	
				}
				else
				{
					left_Node = (Node *)Audit_A_Expr("rolename", strVal(str), "=");
					right_Node = (Node *)make_BoolExprNode(OR_EXPR, left_Node, right_Node);
				}
			}
		}
	
		if (flag)
		{
			audit_refname = NULL;
			
			if (clause != NULL)
			{
				BoolExprNode = make_BoolExprNode(AND_EXPR, clause, right_Node);
				return (Node *)BoolExprNode;
			}
			else
				return right_Node;
		}
	}

	return clause;
}


/* In dedicated mode, it is neccessary to rewrite the 'where'
 * clause to allow syssao and syssso to select pg_audit_log
 * with separate permissions
 */
static Node *
audit_privi_mode(Node *n, bool iswhereclase)
{
	if (audit_GUC.audit_enable != AUDIT_ON)
		return n;

	if (internalStatement)
		return n;

	if (iswhereclase)
	{
		return audit_change_where_clause(n);
	}
	else
	{
		audit_get_from_clause_name(n);
	}

	return n;
}

static bool
is_enable_audit(void)
{
	if (audit_GUC.audit_enable != AUDIT_ON)
		return false;
	else
		return true;
}

/* Determine whether is is an audit log table */
static void 
audit_get_from_clause_name(Node *n)
{
	if (audit_GUC.audit_enable != AUDIT_ON)
		return;

	if (n->type == T_RangeVar)
	{
		RangeVar   *rv = (RangeVar *) n;
		if (strcmp(rv->relname, PG_AUDIT_LOG) == 0 || strcmp(rv->relname, SQLITE_AUDIT_LOG) == 0)
		{
			audit_refname = rv->alias ? rv->alias->aliasname : rv->relname;
		}
	}

	return;
}

static Node * 
make_NullTest(char *col)
{
	ColumnRef *ColNode = make_ColumnRef(col);
	NullTest *NullTestNode = makeNode(NullTest);

	NullTestNode->nulltesttype = IS_NULL;
	NullTestNode->argisrow = false;
	NullTestNode->location = -1;
	NullTestNode->arg = (Expr*)ColNode;

	return (Node *)NullTestNode;
}

static Node *
make_A_Const(char *name, int location)
{
	A_Const *n = makeNode(A_Const);

	n->val.type = T_String;
	n->val.val.str = name;
	n->location = location;

	return (Node *)n;
}

static ColumnRef *
make_ColumnRef(char *name)
{
	ColumnRef *ColNode = NULL;
	if (name == NULL)
		return NULL;

	ColNode = makeNode(ColumnRef);
	ColNode->fields = list_make1(makeString(audit_refname));
	ColNode->fields = lappend(ColNode->fields, makeString(name));
	ColNode->location = -1;

	return ColNode;
}


static Node *
Audit_A_Expr(char *col, char *name, char *equl)
{
	List *equl_list= NULL;
	A_Expr	   *A_ExprNode = NULL;

	/* syssao */
	ColumnRef *ColNode = make_ColumnRef(col);
	Node *A_ConstNode = make_A_Const(name, -1);
	equl_list = list_make1(makeString(equl));

	A_ExprNode = makeA_Expr(AEXPR_OP, equl_list, (Node *)ColNode, A_ConstNode, -1);

	return (Node *)A_ExprNode;
}

static BoolExpr * 
make_BoolExprNode(BoolExprType boolop, Node *left_ExprNode, Node *right_ExprNode)
{
	List	   *BoolExprNode_Args = NULL;
	BoolExpr   *BoolExprNode = NULL;

	BoolExprNode_Args = list_make1(left_ExprNode);
	BoolExprNode_Args = lappend(BoolExprNode_Args,right_ExprNode);
	BoolExprNode = (BoolExpr *)makeBoolExpr(boolop, BoolExprNode_Args, false, -1);

	return BoolExprNode;
}


static BoolExpr *
Audit_make_BoolExprNode(BoolExprType boolop, char *expr1_left,char *expr1_equl, char *expr1_right,
											char *expr2_left,char *expr2_equl, char *expr2_right)
{
	Node *left_ExprNode = NULL;
	Node *right_ExprNode = NULL;
	BoolExpr   *BoolExprNode = NULL;

	left_ExprNode = Audit_A_Expr(expr1_left, expr1_right, expr1_equl);

	right_ExprNode = Audit_A_Expr(expr2_left, expr2_right,expr2_equl);

	BoolExprNode = make_BoolExprNode(boolop, left_ExprNode, right_ExprNode);

	return BoolExprNode;
}

/*
 * Function: skip_space()
 *
 * Description:
 * 	 Skip empty characters
 *
 */
static const char *
skip_space(const char *str)
{
	while (IsSpace(*str)) { str++; }
	return str;
}

/*
 * Function: get_next_token()
 *
 * Description:
 * 	 Get the next string
 *
 */
static const char *
get_next_token(const char *src, char *dst)
{
	const char   *s;
	int		i;
	int		j;

	if ((s = skip_space(src)) == NULL)
		return NULL;

	/* parse quoted string */
	if (*s == '\'')
	{
		s++;
		for (i = 0, j = 0; s[i] != '\0'; i++)
		{
			if (s[i] == '\\')
			{
				i++;
				switch (s[i])
				{
					case 'b':
						dst[j] = '\b';
						break;
					case 'f':
						dst[j] = '\f';
						break;
					case 'n':
						dst[j] = '\n';
						break;
					case 'r':
						dst[j] = '\r';
						break;
					case 't':
						dst[j] = '\t';
						break;
					case '0':
					case '1':
					case '2':
					case '3':
					case '4':
					case '5':
					case '6':
					case '7':
						{
							int			k;
							long		octVal = 0;

							for (k = 0;
								 s[i + k] >= '0' && s[i + k] <= '7' && k < 3;
									 k++)
								octVal = (octVal << 3) + (s[i + k] - '0');
							i += k - 1;
							dst[j] = ((char) octVal);
						}
						break;
					default:
						dst[j] = s[i];
						break;
				}
			}
			else if (s[i] == '\'')
			{
				i++;
				/* doubled quote becomes just one quote */
				if (s[i] == '\'')
					dst[j] = s[i];
				else
					break;
			}
			else
				dst[j] = s[i];
			j++;
		}
	}
	else
	{
		i = j = strcspn(s, "# \n\r\t\v");
		memcpy(dst, s, j);
	}

	dst[j] = '\0';
	return s + i;
}

/*
 * Function: trim()
 *
 * Description:
 * 	 Remove spaces before and after the string
 *
 */
static char *
trim(char *s)
{
	/* Initialize start, end pointers */
	char	   *s1 = s,
			   *s2 = &s[strlen(s) - 1];

	/* If string is empty, no action needed */
	if (s2 < s1)
		return s;

	/* Trim and delimit right side */
	while ((isspace(*s2)) && (s2 >= s1))
		--s2;
	*(s2 + 1) = '\0';

	/* String is all whitespace - no need for further processing */
	if (s2 + 1 == s1)
		return s;

	/* Trim left side */
	while ((isspace(*s1)) && (s1 < s2))
		++s1;

	/* Copy finished string */
	memmove(s, s1, (s2 - s1) + 1);
	s[s2 - s1 + 1] = '\0';

	return s;
}


/*
 * Function: append_word()
 *
 * Description:
 * 	 Splicing Word
 */
static int 
append_word(const char buffer[], char *word)
{
	const char *start = buffer;
	const char *end = NULL;
	unsigned int len = 0;	

	do 
	{
		/* Find the next word */
		end = get_next_token(start, word+len);
		if (end == NULL || *end == '\0')
			break;

		len = strlen(word);
		*(word + len) = ' ';
		len = len + 1;
		
		start = end;
	}while(end != NULL);
	len = strlen(word);
	*(word + len) = '\0';

	return len + 1;
}


/*
 * Function: trim_space()
 *
 * Description:
 * 	 Remove spaces before and after the string, 
 *   as well as excess spaces in the middle
 *
 */
static bool 
trim_space(char *words, unsigned int len)
{
	unsigned int num = 0;
	char *word = NULL;
	
	/* Remove spaces before and after */
	trim(words);
	if (*words == '\0')
	{
		return false;
	}
	word = palloc0(len + 1);

	/* Remove the middle space */
	num = append_word(words, word);
	memcpy(words, word, num);

	pfree(word);

	return true;
}

/*
 * Function: audit_stmttype_to_string()
 *
 * Description:
 * 	 Convert audit statement types to string format
 *
 * Input:
 *	type: Audit statement type
 *
 * Output:
 * Return:
 *	The string corresponding to the type of audit statement
 *
 */
static char * 
audit_stmttype_to_string(AuditStmtType type)
{
	char *tag = INVALID_STMT_S;

	switch(type)
	{
		case CREATE_ROLE:
			tag = CREATE_ROLE_S;
			break;
		case CREATE_DOMAIN:
			tag = CREATE_DOMAIN_S;
			break;
		case CREATE_PROCEDURE:
			tag = CREATE_PROCEDURE_S;
			break;
		case CREATE_INDEX:
			tag = CREATE_INDEX_S;
			break;
		case CREATE_SCHEMA:
			tag = CREATE_SCHEMA_S;
			break;
		case CREATE_SEQUENCE:
			tag = CREATE_SEQUENCE_S;
			break;
		/* Begin - BUG#Z6075 */
		case CREATE_RESOURCE_GROUP:
			tag = CREATE_RESOURCE_GROUP_S;
			break;
		case DROP_RESOURCE_GROUP:
			tag = DROP_RESOURCE_GROUP_S;
			break;
		case ALTER_RESOURCE_GROUP:
			tag = ALTER_RESOURCE_GROUP_S;
			break;
		/* End - BUG#Z6075 */
		case CREATE_TABLE:
			tag = CREATE_TABLE_S;
			break;
		case CREATE_TABLE_AS:
			tag = CREATE_TABLE_AS_S;
			break;
		case CREATE_TRIGGER:
			tag = CREATE_TRIGGER_S;
			break;
		case CREATE_USER:
			tag = CREATE_USER_S;
			break;
		case CREATE_VIEW:
			tag = CREATE_VIEW_S;
			break;
		case CREATE_EXTENSION:
			tag = CREATE_EXTENSION_S;
			break;
		case CREATE_DATABASE:
			tag = CREATE_DATABASE_S;
			break;
		case ALTER_ROLE:
			tag = ALTER_ROLE_S;
			break;
		case ALTER_DOMAIN:
			tag = ALTER_DOMAIN_S;
			break;
		case ALTER_PROCEDURE:
			tag = ALTER_PROCEDURE_S;
			break;
		case ALTER_INDEX:
			tag = ALTER_INDEX_S;
			break;
		case ALTER_SYSTEM:
			tag = ALTER_SYSTEM_S;
			break;
		case ALTER_SCHEMA:
			tag = ALTER_SCHEMA_S;
			break;
		case ALTER_SEQUENCE:
			tag = ALTER_SEQUENCE_S;
			break;
		case ALTER_TABLE:
			tag = ALTER_TABLE_S;
			break;
		case ALTER_TRIGGER:
			tag = ALTER_TRIGGER_S;
			break;
		case ALTER_USER:
			tag = ALTER_USER_S;
			break;
		case ALTER_VIEW:
			tag = ALTER_VIEW_S;
			break;
		case ALTER_EXTENSION:
			tag = ALTER_EXTENSION_S;
			break;
		case ALTER_DATABASE:
			tag = ALTER_DATABASE_S;
			break;
		case DROP_DOMAIN:
			tag = DROP_DOMAIN_S;
			break;
		case DROP_PROCEDURE:
			tag = DROP_PROCEDURE_S;
			break;
		case DROP_ROLE:
			tag = DROP_ROLE_S;
			break;
		case DROP_INDEX:
			tag = DROP_INDEX_S;
			break;
		case DROP_SCHEMA:
			tag = DROP_SCHEMA_S;
			break;
		case DROP_SEQUENCE:
			tag = DROP_SEQUENCE_S;
			break;
		case DROP_TABLE:
			tag = DROP_TABLE_S;
			break;
		case DROP_TRIGGER:
			tag = DROP_TRIGGER_S;
			break;
		case DROP_USER:
			tag = DROP_USER_S;
			break;
		case DROP_VIEW:
			tag = DROP_VIEW_S;
			break;
		case DROP_DATABASE:
			tag = DROP_DATABASE_S;
			break;
		case DROP_EXTENSION:
			tag = DROP_EXTENSION_S;
			break;
		case SELECT_T:
			tag = SELECT_S;
			break;
		case SELECT_INTO:
			tag = SELECT_INTO_S;
			break;
		case INSERT_T:
			tag = INSERT_S;
			break;
		case UPDATE_T:
			tag = UPDATE_S;
			break;
		case DELETE_T:
			tag = DELETE_S;
			break;
		case GRANT_T:
		case GRANT_ROLE:
			tag = GRANT_S;
			break;
		case REVOKE_T:
		case REVOKE_ROLE:
			tag = REVOKE_S;
			break;
		case COMMENT_T:
			tag = COMMENT_S;
			break;
		case RESET_T:
			tag = RESET_S;
			break;
		case SET_T:
			tag = SET_S;
			break;
		case TRUNCATE_T:
			tag = TRUNCATE_S;
			break;
		case COPY_TO:
			tag = COPY_TO_S;
			break;
		case COPY_FROM:
			tag = COPY_FROM_S;
			break;
		case REINDEX_T:
			tag = REINDEX_S;
			break;
		case LOCK_T:
			tag = LOCK_S;
			break;
		case CALL_T:
			tag = CALL_S;
			break;
		case START_T:
			tag = START_S;
			break;
		case STOP_T:
			tag = STOP_S;
			break;
		case LOGIN_T:
			tag = LOGIN_S;
			break;
		case LOGOUT_T:
			tag = LOGOUT_S;
			break;
		case RELOAD_T:
			tag = RELOAD_S;
			break;
		case AUDIT_STORAGE_INVALID_T:
			tag = AUDIT_STORAGE_INVALID_S;
			break;
		case ALTER_GUC_T:
			tag = ALTER_GUC_S;
			break;
		case IMPORT_T:
			tag = IMPORT_S;
			break;
		case EXPORT_T:
			tag = EXPORT_S;
			break;
		case CREATE_FUNCTION:
			tag = CREATE_FUNCTION_S;
			break;
		case DROP_FUNCTION:
			tag = DROP_FUNCTION_S;
			break;
		case ALTER_FUNCTION:
			tag = ALTER_FUNCTION_S;
			break;
		case CREATE_TABLESPACE:
			tag = CREATE_TABLESPACE_S;
			break;
		case ALTER_TABLESPACE:
			tag = ALTER_TABLESPACE_S;
			break;
		case DROP_TABLESPACE:
			tag = DROP_TABLESPACE_S;
			break;

		case CREATE_MATLIZED_VIEW:
			tag = CREATE_MATLIZED_VIEW_S;
			break;
		case ALTER_MATLIZED_VIEW:
			tag = ALTER_MATLIZED_VIEW_S;
			break;
		case DROP_MATLIZED_VIEW:
			tag = DROP_MATLIZED_VIEW_S;
			break;
		case BEGIN_T:
			tag = BEGIN_S;
			break;
		case COMMIT_T:
			tag = COMMIT_S;
			break;
		case START_TRANSACTION:
			tag = START_TRANSACTION_S;
			break;
		case ROLLBACK_T:
			tag = ROLLBACK_S;
			break;
		case SAVEPOINT_T:
			tag = SAVEPOINT_S;
			break;
		case ROLLBACK_TO_T:
			tag = ROLLBACK_TO_S;
			break;
		case ROLLBACK_PREPARED_T:
			tag = ROLLBACK_PREPARED_S;
			break;
		case RELEASE_T:
			tag = RELEASE_S;
			break;
		case MANDATORY_AUDIT_ON_T:
			tag = MANDATORY_AUDIT_ON_S;
			break;
		case MANDATORY_AUDIT_OFF_T:
			tag = MANDATORY_AUDIT_OFF_S;
			break;
		case PG_DUMP_T:
			tag = PG_DUMP_S;
			break;
		case PG_DUMPALL_T:
			tag = PG_DUMPALL_S;
			break;
		case PG_RESTORE_T:
			tag = PG_RESTORE_S;
			break;
		case PG_BASEBACKUP_T:
			tag = PG_BASEBACKUP_S;
			break;

		case PG_PERMISSION_T:
			tag = PG_PERMISSION_S;
			break;

		case LOCK_ACCCOUNT:
			tag = LOCK_ACCOUNT_S;
			break;
		case SWITCHOVER:
			tag = SWITCHOVER_S;
			break;
		default:
			break;
	}

	return tag;
}


/*
 * Function: audit_get_privi_type()
 *
 * Description:
 * 	 Obtain permission audit type
 *
 * Input:
 *	statment:User input type keywords 
 *
 * Output:
 * Return:
 *	The string corresponding to the audit permission type
 *
 */
static AuditStmtType
audit_get_privi_type(char *statment)
{
	AuditStmtType type = INVALID_STMT;

	if (pg_strcasecmp(statment, CREATE_ROLE_S) == 0)
		type = CREATE_ROLE;
	else if (pg_strcasecmp(statment, CREATE_USER_S) == 0)
		type = CREATE_USER;
	else if (pg_strcasecmp(statment, CREATE_DATABASE_S) == 0)
		type = CREATE_DATABASE;
	else if (pg_strcasecmp(statment, ALTER_ROLE_S) == 0)
		type = ALTER_ROLE;
	else if (pg_strcasecmp(statment, CREATE_SCHEMA_S) == 0)
		type = CREATE_SCHEMA;
	else if (pg_strcasecmp(statment, ALTER_USER_S) == 0)
		type = ALTER_USER;
	else if (pg_strcasecmp(statment, ALTER_DATABASE_S) == 0)
		type = ALTER_DATABASE;
	else if (pg_strcasecmp(statment, ALTER_SCHEMA_S) == 0)
		type = ALTER_SCHEMA;
	else if (pg_strcasecmp(statment, DROP_ROLE_S) == 0)
		type = DROP_ROLE;
	else if (pg_strcasecmp(statment, DROP_USER_S) == 0)
		type = DROP_USER;
	else if (pg_strcasecmp(statment, DROP_SCHEMA_S) == 0)
		type = DROP_SCHEMA;
	else if (pg_strcasecmp(statment, DROP_DATABASE_S) == 0)
		type = DROP_DATABASE;
	else if (pg_strcasecmp(statment, GRANT_ROLE_S) == 0)
		type = GRANT_ROLE;
	else if (pg_strcasecmp(statment, REVOKE_ROLE_S) == 0)
		type = REVOKE_ROLE;
	else if (pg_strcasecmp(statment, ALL_S) == 0)
		type = ALL_T;
	else
		type = INVALID_STMT;
	return type;
}

/*
 * Function: audit_get_stmt_type()
 *
 * Description:
 * 	 Obtain statement audit type
 *
 * Input:
 *	statment: User input type keywords
 *	is_obj: Is it an object audit?
 *	objtype: object type
 *
 * Output:
 * Return:
 *	The string corresponding to the type of audit statements
 *
 */
static AuditStmtType
audit_get_stmt_type(char *statment, bool is_obj, ObjectType objtype)
{
	AuditStmtType type = INVALID_STMT;

	if (pg_strcasecmp(statment, CREATE_DOMAIN_S) == 0)
		type = CREATE_DOMAIN;
	else if (pg_strcasecmp(statment, CREATE_PROCEDURE_S) == 0)
		type = CREATE_PROCEDURE;
	else if (pg_strcasecmp(statment, CREATE_INDEX_S) == 0)
		type = CREATE_INDEX;
	else if (pg_strcasecmp(statment, CREATE_SEQUENCE_S) == 0)
		type = CREATE_SEQUENCE;
	/* Begin - BUG#Z6075 */
	else if (pg_strcasecmp(statment, CREATE_RESOURCE_GROUP_S) == 0)
		type = CREATE_RESOURCE_GROUP;
	else if (pg_strcasecmp(statment, DROP_RESOURCE_GROUP_S) == 0)
		type = DROP_RESOURCE_GROUP;
	else if (pg_strcasecmp(statment, ALTER_RESOURCE_GROUP_S) == 0)
		type = ALTER_RESOURCE_GROUP;
	/* End - BUG#Z6075 */
	else if (pg_strcasecmp(statment, CREATE_TABLE_S) == 0)
		type = CREATE_TABLE;
	else if (pg_strcasecmp(statment, CREATE_TABLE_AS_S) == 0)
		type = CREATE_TABLE_AS;
	else if (pg_strcasecmp(statment, CREATE_TRIGGER_S) == 0)
		type = CREATE_TRIGGER;
	else if (pg_strcasecmp(statment, CREATE_VIEW_S) == 0)
		type = CREATE_VIEW;
	else if (pg_strcasecmp(statment, CREATE_EXTENSION_S) == 0)
		type = CREATE_EXTENSION;
	else if (pg_strcasecmp(statment, ALTER_DOMAIN_S) == 0)
		type = ALTER_DOMAIN;
	else if (pg_strcasecmp(statment, ALTER_PROCEDURE_S) == 0)
		type = ALTER_PROCEDURE;
	else if (pg_strcasecmp(statment, ALTER_INDEX_S) == 0)
		type = ALTER_INDEX;
	else if (pg_strcasecmp(statment, ALTER_SYSTEM_S) == 0)
		type = ALTER_SYSTEM;
	else if (pg_strcasecmp(statment, ALTER_SEQUENCE_S) == 0)
		type = ALTER_SEQUENCE;
	else if (pg_strcasecmp(statment, ALTER_TABLE_S) == 0)
		type = ALTER_TABLE;
	else if (pg_strcasecmp(statment, ALTER_TRIGGER_S) == 0)
		type = ALTER_TRIGGER;
	else if (pg_strcasecmp(statment, ALTER_VIEW_S) == 0)
		type = ALTER_VIEW;
	else if (pg_strcasecmp(statment, ALTER_EXTENSION_S) == 0)
		type = ALTER_EXTENSION;
	else if (pg_strcasecmp(statment, DROP_DOMAIN_S) == 0)
		type = DROP_DOMAIN;
	else if (pg_strcasecmp(statment, DROP_PROCEDURE_S) == 0)
		type = DROP_PROCEDURE;
	else if (pg_strcasecmp(statment, DROP_INDEX_S) == 0)
		type = DROP_INDEX;
	else if (pg_strcasecmp(statment, DROP_SEQUENCE_S) == 0)
		type = DROP_SEQUENCE;
	else if (pg_strcasecmp(statment, DROP_TABLE_S) == 0)
		type = DROP_TABLE;
	else if (pg_strcasecmp(statment, DROP_TRIGGER_S) == 0)
		type = DROP_TRIGGER;
	else if (pg_strcasecmp(statment, DROP_VIEW_S) == 0)
		type = DROP_VIEW;
	else if (pg_strcasecmp(statment, DROP_EXTENSION_S) == 0)
		type = DROP_EXTENSION;
	else if (pg_strcasecmp(statment, SELECT_S) == 0)
		type = SELECT_T;
	else if (pg_strcasecmp(statment, SELECT_INTO_S) == 0)
		type = SELECT_INTO;
	else if (pg_strcasecmp(statment, INSERT_S) == 0)
		type = INSERT_T;
	else if (pg_strcasecmp(statment, UPDATE_S) == 0)
		type = UPDATE_T;
	else if (pg_strcasecmp(statment, DELETE_S) == 0)
		type = DELETE_T;
	else if (pg_strcasecmp(statment, GRANT_S) == 0)
		type = GRANT_T;
	else if (pg_strcasecmp(statment, REVOKE_S) == 0)
		type = REVOKE_T;
	else if (pg_strcasecmp(statment, COMMENT_S) == 0)
		type = COMMENT_T;
	else if (pg_strcasecmp(statment, RESET_S) == 0)
		type = RESET_T;
	else if (pg_strcasecmp(statment, SET_S) == 0)
		type = SET_T;
	else if (pg_strcasecmp(statment, TRUNCATE_S) == 0)
		type = TRUNCATE_T;
	else if (pg_strcasecmp(statment, COPY_TO_S) == 0)
		type = COPY_TO;
	else if (pg_strcasecmp(statment, COPY_FROM_S) == 0)
		type = COPY_FROM;
	else if (pg_strcasecmp(statment, REINDEX_S) == 0)
		type = REINDEX_T;
	else if (pg_strcasecmp(statment, LOCK_S) == 0)
		type = LOCK_T;
	else if (pg_strcasecmp(statment, CALL_S) == 0)
		type = CALL_T;
	else if (pg_strcasecmp(statment, START_S) == 0)
		type = START_T;
	else if (pg_strcasecmp(statment, STOP_S) == 0)
		type = STOP_T;
	else if (pg_strcasecmp(statment, LOGIN_S) == 0)
		type = LOGIN_T;
	else if (pg_strcasecmp(statment, LOGOUT_S) == 0)
		type = LOGOUT_T;
	else if (pg_strcasecmp(statment, RELOAD_S) == 0)
		type = RELOAD_T;
	else if (pg_strcasecmp(statment, AUDIT_STORAGE_INVALID_S) == 0)
		type = AUDIT_STORAGE_INVALID_T;
	else if (pg_strcasecmp(statment, ALTER_GUC_S) == 0)
		type = ALTER_GUC_T;
	else if (pg_strcasecmp(statment, IMPORT_S) == 0)
		type = IMPORT_T;
	else if (pg_strcasecmp(statment, EXPORT_S) == 0)
		type = EXPORT_T;
	else if (pg_strcasecmp(statment, CREATE_FUNCTION_S) == 0)
		type = CREATE_FUNCTION;
	else if (pg_strcasecmp(statment, DROP_FUNCTION_S) == 0)
		type = DROP_FUNCTION;
	else if (pg_strcasecmp(statment, ALTER_FUNCTION_S) == 0)
		type = ALTER_FUNCTION;
	else if (pg_strcasecmp(statment, CREATE_TABLESPACE_S) == 0)
		type = CREATE_TABLESPACE;
	else if (pg_strcasecmp(statment, ALTER_TABLESPACE_S) == 0)
		type = ALTER_TABLESPACE;
	else if (pg_strcasecmp(statment, DROP_TABLESPACE_S) == 0)
		type = DROP_TABLESPACE;
	else if (pg_strcasecmp(statment, CREATE_MATLIZED_VIEW_S) == 0)
		type = CREATE_MATLIZED_VIEW;
	else if (pg_strcasecmp(statment, ALTER_MATLIZED_VIEW_S) == 0)
		type = ALTER_MATLIZED_VIEW;
	else if (pg_strcasecmp(statment, DROP_MATLIZED_VIEW_S) == 0)
		type = DROP_MATLIZED_VIEW;
	else if (pg_strcasecmp(statment, BEGIN_S) == 0)
		type = BEGIN_T;
	else if (pg_strcasecmp(statment, COMMIT_S) == 0)
		type = COMMIT_T;
	else if (pg_strcasecmp(statment, START_TRANSACTION_S) == 0)
		type = START_TRANSACTION;
	else if (pg_strcasecmp(statment, ROLLBACK_S) == 0)
		type = ROLLBACK_T;
	else if (pg_strcasecmp(statment, SAVEPOINT_S) == 0)
		type = SAVEPOINT_T;
	else if (pg_strcasecmp(statment, ROLLBACK_TO_S) == 0)
		type = ROLLBACK_TO_T;
	else if (pg_strcasecmp(statment, ROLLBACK_PREPARED_S) == 0)
		type = ROLLBACK_PREPARED_T;
	else if (pg_strcasecmp(statment, RELEASE_S) == 0 || pg_strcasecmp(statment, RELEASE_SAVEPOINT_S) == 0)
		type = RELEASE_T;
	else if (pg_strcasecmp(statment, MANDATORY_AUDIT_ON_S) == 0)
		type = MANDATORY_AUDIT_ON_T;
	else if (pg_strcasecmp(statment, MANDATORY_AUDIT_OFF_S) == 0)
		type = MANDATORY_AUDIT_OFF_T;
	else if (pg_strcasecmp(statment, LOCK_ACCOUNT_S) == 0)
		type = LOCK_ACCCOUNT;
	else if (pg_strcasecmp(statment, SWITCHOVER_S) == 0)
		type = SWITCHOVER;
	else if (pg_strcasecmp(statment, ALL_S) == 0)
		type = ALL_T;
	else
	{
		if (is_obj == true)
		{
			switch (objtype)
			{
				case OBJECT_TABLE:
					if (pg_strcasecmp(statment, ALTER_S) == 0)
						type = ALTER_TABLE;
					break;
				case OBJECT_SEQUENCE:
					{
						if (pg_strcasecmp(statment, ALTER_S) == 0)
							type = ALTER_SEQUENCE;
						else if (pg_strcasecmp(statment, DROP_S) == 0)
							type = DROP_SEQUENCE;
						else if (pg_strcasecmp(statment, CREATE_S) == 0)
							type = CREATE_SEQUENCE;
					}
					break;
				case OBJECT_FUNCTION:
					{
						if (pg_strcasecmp(statment, ALTER_S) == 0)
							type = ALTER_FUNCTION;
						else if (pg_strcasecmp(statment, DROP_S) == 0)
							type = DROP_FUNCTION;
						else if (pg_strcasecmp(statment, CREATE_S) == 0)
							type = CREATE_FUNCTION;
					}
					break;
				case OBJECT_PROCEDURE:
					{
						if (pg_strcasecmp(statment, ALTER_S) == 0)
							type = ALTER_PROCEDURE;
						else if (pg_strcasecmp(statment, DROP_S) == 0)
							type = DROP_PROCEDURE;
						else if (pg_strcasecmp(statment, CREATE_S) == 0)
							type = CREATE_PROCEDURE;
					}
					break;
				default:
					type = INVALID_STMT;
					break;
			}
		}
		else
		{
			type = INVALID_STMT;
		}
		
	}

	return type;
}


/*
 * Function: create_statments_list()
 *
 * Description:
 * 	 Create list of audit statements
 *
 */
static List *
create_statments_list(AuditType audittype, char *statments, bool is_obj, ObjectType objtype)
{
	char	*words = NULL;
	char	*nextstatment = NULL;
	char	*curstatment = statments;
	unsigned int len = 0;
	List	*statments_list = NULL;
	AuditStmtType type = INVALID_STMT;

	if (statments == NULL)
		return NULL;

	do {
		nextstatment = strchr(curstatment, ',');
		if (nextstatment != NULL)
		{
			len = abs(nextstatment - curstatment);
			words = palloc0(len + 1);
			memcpy(words, curstatment, len);
			curstatment = nextstatment + 1;
		}
		else
		{
			len = strlen(curstatment);
			words = palloc0(len + 1);
			memcpy(words, curstatment, len + 1);
		}

		if (trim_space(words, len) == false)
		{
			return NULL;
		}

		if (audittype == AUDIT_TYPE_PRIVILEGE)
			type = audit_get_privi_type(words);
		else
			type = audit_get_stmt_type(words, is_obj, objtype);
			
		if (type == INVALID_STMT)
		{
			return NULL;
		}

		if (statments_list == NULL)
		{
			statments_list = list_make1_int(type);
		}
		else
		{
			statments_list = list_append_unique_int(statments_list, type);
		}
		pfree(words);

	}while(nextstatment != NULL);

	return statments_list;
}

/*
 * Function: makeRoleSpec()
 *
 * Description:
 * 	Create role structure
 *
 */
static RoleSpec *
makeRoleSpec(RoleSpecType type, int location)
{
	RoleSpec *spec = makeNode(RoleSpec);

	spec->roletype = type;
	spec->location = location;

	return spec;
}

/*
 * Function: get_userid_list()
 *
 * Description:
 * 	 Obtain a list of user ID list based on the usernames entered by the user
 *
 */
static List *
get_userid_list(char *usernames)
{
	char	*curuser = usernames;
	char	*nextuser = NULL;
	List *users = NIL;
	unsigned int len = 0;
	char	*words = NULL;
	RoleSpec *role = NULL;
	Oid userid = 0;

#if 0
	if (strlen(usernames) == 0)
	{
		users = lappend_oid(users, 0);
		return users;
	}
#endif
	do{
		nextuser = strchr(curuser, ',');
		if (nextuser != NULL)
		{
			len = abs(nextuser - curuser);
			words = palloc0(len + 1);
			memcpy(words, curuser, len);
			curuser = nextuser + 1;
		}
		else
		{
			len = strlen(curuser);
			words = palloc0(len + 1);
			memcpy(words, curuser, len + 1);
		}

		if (trim_space(words, len) == false)
		{
			return NULL;
		}

		/* get user oid */
		if (pg_strcasecmp(words, "CURRENT_USER") == 0)
		{
			role = makeRoleSpec(ROLESPEC_CURRENT_USER, -1);
		}
		else if (pg_strcasecmp(words, "SESSION_USER") == 0)
		{
			role = makeRoleSpec(ROLESPEC_SESSION_USER, -1);
		}
		else
		{
			if (pg_strcasecmp(words, "public") == 0)
			{
				role = (RoleSpec *) makeRoleSpec(ROLESPEC_PUBLIC, -1);
			}
			else if (pg_strcasecmp(words, "none") == 0)
			{
				return NULL;
			}
			else
			{
				role = makeRoleSpec(ROLESPEC_CSTRING, -1);
				role->rolename = pstrdup(words);
			}
		}
		
		userid = get_rolespec_oid(role, false);

		users = lappend_oid(users, userid);
		
		pfree(words);

	}while(nextuser != NULL);

	return users;
}

/*
 * Function: audit_get_options_on_statement()
 *
 * Description:
 * 	 When AuditSTmtType is ALL, exhaustively search for all AuditSTmtTypes
 *
 * Input:
 *	optionlist: Statement audit type
 *
 * Output:
 *
 * Return:
 *	Rewritten AuditStmtType List
 *
 */
static List *
audit_get_options_on_statement(AuditType audittype, List *optionlist)
{
	AuditStmtType	auditevent;
	List	*options = optionlist;

	auditevent = lfirst_int(list_head(optionlist));
	
	/* Specify audit statements */
	if (auditevent != ALL_T)
		return options;

	if (audittype == AUDIT_TYPE_PRIVILEGE)
	{
		options = list_make5_int(ALTER_DATABASE, ALTER_ROLE, ALTER_SCHEMA, ALTER_USER, CREATE_DATABASE);

		for (auditevent = CREATE_ROLE; auditevent < CREATE_DOMAIN; auditevent++)
		{
			options = lappend_int(options,auditevent);
		}	
	}
	else
	{
		options = list_make5_int(CREATE_DOMAIN, CREATE_PROCEDURE, CREATE_INDEX, CREATE_SEQUENCE, CREATE_TABLE);

		for (auditevent = CREATE_TABLE_AS; auditevent < START_T;auditevent++)
		{
			options = lappend_int(options,auditevent);
		}
	}

	return options;
}

/*
 * Function: audit_insert_stmt_tuple()
 *
 * Description:
 * 	 Write a record to the system table pg_audid_config
 *
 * Input:
 *	pg_audit	: table relation
 *	auditevent	: audit event
 *	auditoroid	: audit role oid
 *	object		: object or user information
 *	classoid	: OID of the system table where the object or user is located
 *	objtype		: object type
 *	audittype	: adit type
 *	mode		: audit success or failure
 *
 */
static void
audit_insert_stmt_tuple(Relation pg_audit, Name auditevent, Oid auditoroid, Name object, Oid classoid,
									Name objtype, AuditType audittype, AuditResult mode)
{
	Oid  		confid;
	Datum		values[Natts_pg_audit_config];
	bool		nulls[Natts_pg_audit_config];
	HeapTuple	tup;
	
	memset(values, 0, sizeof(values));
	memset(nulls, false, sizeof(nulls));

	values[Anum_pg_audit_config_actionname - 1] = NameGetDatum(auditevent);
	values[Anum_pg_audit_config_auditoroid - 1] = ObjectIdGetDatum(auditoroid);
	values[Anum_pg_audit_config_objoid - 1] = NameGetDatum(object);
	values[Anum_pg_audit_config_classoid - 1] = ObjectIdGetDatum(classoid);
	values[Anum_pg_audit_config_objtype - 1] = NameGetDatum(objtype);
	values[Anum_pg_audit_config_audittype - 1] = CharGetDatum(audittype);
	values[Anum_pg_audit_config_auditmode - 1] = CharGetDatum(mode);
	values[Anum_pg_audit_config_enable - 1] = CharGetDatum(AUDIT_EANBLE);

	confid = GetNewOidWithIndex(pg_audit, AuditConfigconfIdIndexId, Anum_pg_audit_config_confid);
	values[Anum_pg_audit_config_confid - 1] = ObjectIdGetDatum(confid);

	tup = heap_form_tuple(RelationGetDescr(pg_audit), values, nulls);

	CatalogTupleInsert(pg_audit, tup);

	heap_freetuple(tup);

}

/*
 * Function: delete_stmt_tuple_user0()
 *
 * Description:
 * 	 When no user is specified =all, cancel the user audit call to this function
 *
 * Input:
 *	pg_audit: table relation
 *	auditevent: statement audit event
 *	auditmode: audit success or failure
 *
 */
static void 
disable_stmt_tuple_user0(Relation pg_audit, AuditStmtType auditevent, AuditResult auditmode)
{
	CatCList * catlist;
	int i = 0;
	Form_pg_audit_config audit_form;
	int cacheId = 0;

	cacheId = AUDITACTIONNAMEAUDITTYPEID;

	catlist = SearchSysCacheList1(cacheId, CharGetDatum(auditevent));

	if (catlist->n_members > 0)
	{
		for (i = 0; i < catlist->n_members; i++)
		{
			HeapTuple   tup = &catlist->members[i]->tuple;
			
			audit_form = (Form_pg_audit_config) GETSTRUCT(tup);

			if (audit_form->auditmode == auditmode && audit_form->audittype == '3')
			{
				if (audit_form->enable == AUDIT_UNABLE)
					continue;
				else	
				{
					HeapTuple newtuple;
					Datum values[Natts_pg_audit_config];
					bool nulls[Natts_pg_audit_config];
					bool replaces[Natts_pg_audit_config];

					MemSet(values, 0, sizeof(values));
					MemSet(nulls, false, sizeof(nulls));
					MemSet(replaces, false, sizeof(replaces));

					replaces[Anum_pg_audit_config_enable - 1] = true;
					values[Anum_pg_audit_config_enable - 1] = CharGetDatum(AUDIT_UNABLE);

					newtuple = heap_modify_tuple(tup, RelationGetDescr(pg_audit), values,
										 nulls, replaces);
					CatalogTupleUpdate(pg_audit, &newtuple->t_self, newtuple);

					heap_freetuple(newtuple);
				}
			}
		}
	}
	ReleaseSysCacheList(catlist);

	return;
}

/*
 * Function: calculate_audit_mode
 *
 * Description:
 * 	 When configuring different audit modes for the same operation, 
 *   a new audit mode will be calculated based on the existing audit mode.
 *
 * Input:
 *	old_result: mode value of old tuples
 *	new_result: value of the set mode
 *	old_enable: enable value of old tuples
 *	new_enable: enable of the set mode
 *	enable: set or cancel auditing
 *
 * Output:
 * Return: calculated value of mode
 */

static AuditResult
calculate_audit_mode(AuditResult old_result, AuditResult new_result, AuditChange old_enable, AuditChange new_enable, AuditChange *final_enable)
{
	AuditResult result = AUDIT_KEEP;
	
	if (new_enable == AUDIT_EANBLE)
	{	
		*final_enable = AUDIT_EANBLE;
		
		if (old_enable == AUDIT_EANBLE)
		{
			if (old_result == AUDIT_ALL && new_result == AUDIT_ALL)
			{
				result = AUDIT_KEEP;
			}
			else if (old_result == AUDIT_ALL && new_result == AUDIT_SUCCESS)
			{
				result = AUDIT_KEEP;
			}
			else if (old_result == AUDIT_ALL && new_result == AUDIT_FAIL)
			{
				result = AUDIT_KEEP;
			}
			else if (old_result == AUDIT_SUCCESS && new_result == AUDIT_ALL)
			{
				result = AUDIT_ALL;
			}
			else if (old_result == AUDIT_SUCCESS && new_result == AUDIT_SUCCESS)
			{
				result = AUDIT_KEEP;
			}
			else if (old_result == AUDIT_SUCCESS && new_result == AUDIT_FAIL)
			{
				result = AUDIT_ALL;	
			}
			else if (old_result == AUDIT_FAIL && new_result == AUDIT_ALL)
			{
				result = AUDIT_ALL;
			}
			else if (old_result == AUDIT_FAIL && new_result == AUDIT_SUCCESS)
			{
				result = AUDIT_ALL;
			}
			else if (old_result == AUDIT_FAIL && new_result == AUDIT_FAIL)
			{
				result = AUDIT_KEEP;
			}
		}
		else
		{
			if (old_result == AUDIT_ALL && new_result == AUDIT_ALL)
			{
				result = AUDIT_KEEP;
			
			}
			else if (old_result == AUDIT_ALL && new_result == AUDIT_SUCCESS)
			{
				result = AUDIT_SUCCESS;
				
			}
			else if (old_result == AUDIT_ALL && new_result == AUDIT_FAIL)
			{
				result = AUDIT_FAIL;

			}
			else if (old_result == AUDIT_SUCCESS && new_result == AUDIT_ALL)
			{
				result = AUDIT_ALL;
			}
			else if (old_result == AUDIT_SUCCESS && new_result == AUDIT_SUCCESS)
			{
				result = AUDIT_KEEP;
			}
			else if (old_result == AUDIT_SUCCESS && new_result == AUDIT_FAIL)
			{
				result = AUDIT_FAIL;	
			}
			else if (old_result == AUDIT_FAIL && new_result == AUDIT_ALL)
			{
				result = AUDIT_ALL;
			}
			else if (old_result == AUDIT_FAIL && new_result == AUDIT_SUCCESS)
			{
				result = AUDIT_SUCCESS;
			}
			else if (old_result == AUDIT_FAIL && new_result == AUDIT_FAIL)
			{
				result = AUDIT_KEEP;
			}
		}
	}
	else
	{
		*final_enable = AUDIT_UNABLE;
		
		if (old_enable == AUDIT_EANBLE)
		{
			if (old_result == AUDIT_ALL && new_result == AUDIT_ALL)
			{
				result = AUDIT_KEEP;
			}
			else if (old_result == AUDIT_ALL && new_result == AUDIT_SUCCESS)
			{
				result = AUDIT_FAIL;
				*final_enable = AUDIT_EANBLE;
			}
			else if (old_result == AUDIT_ALL && new_result == AUDIT_FAIL)
			{
				result = AUDIT_SUCCESS;
				*final_enable = AUDIT_EANBLE;
			}
			else if (old_result == AUDIT_SUCCESS && new_result == AUDIT_ALL)
			{
				result = AUDIT_ALL;
			}
			else if (old_result == AUDIT_SUCCESS && new_result == AUDIT_SUCCESS)
			{
				result = AUDIT_KEEP;
			}
			else if (old_result == AUDIT_SUCCESS && new_result == AUDIT_FAIL)
			{
				result = AUDIT_NONE;
			}
			else if (old_result == AUDIT_FAIL && new_result == AUDIT_ALL)
			{
				result = AUDIT_ALL;
			}
			else if (old_result == AUDIT_FAIL && new_result == AUDIT_SUCCESS)
			{
				result = AUDIT_NONE;
			}
			else if (old_result == AUDIT_FAIL && new_result == AUDIT_FAIL)
			{
				result = AUDIT_KEEP;
			}
		}
		else
		{
			if (old_result == AUDIT_ALL && new_result == AUDIT_ALL)
			{
				result = AUDIT_KEEP;
			}
			else if (old_result == AUDIT_ALL && new_result == AUDIT_SUCCESS)
			{
				result = AUDIT_KEEP;
			}
			else if (old_result == AUDIT_ALL && new_result == AUDIT_FAIL)
			{
				result = AUDIT_KEEP;
			}
			else if (old_result == AUDIT_SUCCESS && new_result == AUDIT_ALL)
			{
				result = AUDIT_ALL;
			}
			else if (old_result == AUDIT_SUCCESS && new_result == AUDIT_SUCCESS)
			{
				result = AUDIT_KEEP;
			}
			else if (old_result == AUDIT_SUCCESS && new_result == AUDIT_FAIL)
			{
				result = AUDIT_NONE;
			}
			else if (old_result == AUDIT_FAIL && new_result == AUDIT_ALL)
			{
				result = AUDIT_ALL;
			}
			else if (old_result == AUDIT_FAIL && new_result == AUDIT_SUCCESS)
			{
				result = AUDIT_NONE;
			}
			else if (old_result == AUDIT_FAIL && new_result == AUDIT_FAIL)
			{
				result = AUDIT_KEEP;
			}
		}
	}

	return result;
}

/*
 * Function: audit_deal_with_item()
 *
 * Description:
 *	deal with an audit item
 *
 */
static void 
audit_deal_with_item(Relation pg_audit, AuditStmtType auditevent, Oid auditoroid, char *object, Oid classoid, 
							char *objtype, AuditType audittype, AuditResult auditmode, AuditChange stmtflg)
{
	HeapTuple tup;
	Form_pg_audit_config audit_form;
	char	*event;
	Name	name_event;
	Name	name_objtype;
	Name	name_obj;
	AuditResult	mode;
	HeapTuple newtuple;
	Datum values[Natts_pg_audit_config];
	bool nulls[Natts_pg_audit_config];
	bool replaces[Natts_pg_audit_config];
	
	/* 用户为ALL */
	if (audittype == AUDIT_TYPE_USER && AUDIT_UNABLE == stmtflg)
	{
		if (strcmp(object, "0.0") == 0)
		{
			disable_stmt_tuple_user0(pg_audit, auditevent, auditmode);
			return;
		}
	}

	event = audit_stmttype_to_string(auditevent);

	name_event = (Name) palloc(NAMEDATALEN);
	name_objtype = (Name) palloc(NAMEDATALEN);
	name_obj = (Name) palloc(NAMEDATALEN);
	
	namestrcpy(name_event, event);
	namestrcpy(name_objtype, objtype);
	namestrcpy(name_obj, object);
	
	tup = SearchSysCache4(AUDITACTIONNAMEAUDITTYPEID, CStringGetDatum(event), CStringGetDatum(object), 
								ObjectIdGetDatum(classoid), CharGetDatum(audittype));

	/* The tuple was not found, and to open the policy, insert a tuple */
	if (!HeapTupleIsValid(tup) && AUDIT_EANBLE == stmtflg)
	{
		audit_insert_stmt_tuple(pg_audit, name_event, auditoroid, name_obj, classoid, name_objtype, audittype, auditmode);
	}
	/* No tuple found, and if the policy needs to be closed, return directly without processing */
	else if (!HeapTupleIsValid(tup) && AUDIT_UNABLE == stmtflg)
	{
		ereport(ERROR,
					(errmsg("There is no audit strategy available!")));
	}
	else 
	{
		AuditChange result;
		audit_form = (Form_pg_audit_config) GETSTRUCT(tup);
		
		mode = calculate_audit_mode(audit_form->auditmode, auditmode, audit_form->enable, stmtflg, &result);

		if (mode == AUDIT_NONE)
		{
			ereport(ERROR,
					(errmsg("There is no audit strategy available!")));
		}
		else if (mode == AUDIT_KEEP)
		{
			if (result != audit_form->enable)
			{
				MemSet(values, 0, sizeof(values));
	 			MemSet(nulls, false, sizeof(nulls));
				MemSet(replaces, false, sizeof(replaces));
						
				replaces[Anum_pg_audit_config_enable - 1] = true;
				values[Anum_pg_audit_config_enable - 1] = CharGetDatum(result);

				newtuple = heap_modify_tuple(tup, RelationGetDescr(pg_audit), values,
								 nulls, replaces);
				CatalogTupleUpdate(pg_audit, &newtuple->t_self, newtuple);

				heap_freetuple(newtuple);
			}
			ReleaseCatCache(tup);
			return;
		}
		else
		{
			MemSet(values, 0, sizeof(values));
 			MemSet(nulls, false, sizeof(nulls));
			MemSet(replaces, false, sizeof(replaces));
			
			if (result != audit_form->enable)
			{
				replaces[Anum_pg_audit_config_enable - 1] = true;
				values[Anum_pg_audit_config_enable - 1] = CharGetDatum(result);
			}

			replaces[Anum_pg_audit_config_auditmode - 1] = true;
			values[Anum_pg_audit_config_auditmode - 1] = CharGetDatum(mode);

			newtuple = heap_modify_tuple(tup, RelationGetDescr(pg_audit), values,
								 nulls, replaces);
			CatalogTupleUpdate(pg_audit, &newtuple->t_self, newtuple);

			heap_freetuple(newtuple);
			ReleaseCatCache(tup);
		}
	}

	return;
}

/*
 * Function: audit_check_mode()
 *
 * Description:
 *	   Check the success or failure information specified in the parameters
 *
 */
static int 
audit_check_mode(char *audit_result_str, AuditResult *isSuccess)
{
	char * str = NULL;
	unsigned int len = 0;
	char	*words = NULL;
	char	*nextstr = NULL;
	char	*curstr = NULL;
	bool flag_suc = false;

	if (audit_result_str == NULL)
		return -1;

	len = strlen(audit_result_str) + 1;
	str = palloc0(len);
	memcpy(str, audit_result_str, len);

	curstr = str;
	do{
		nextstr = strchr(curstr, ',');
		if (nextstr != NULL)
		{
			len = abs(nextstr - curstr);
			words = palloc0(len + 1);
			memcpy(words, curstr, len);
			curstr = nextstr + 1;
		}
		else
		{
			len = strlen(curstr);
			words = palloc0(len + 1);
			memcpy(words, curstr, len + 1);
		}

		trim(words);

		if (pg_strcasecmp(words, SUCCESSFUL_S) == 0)
		{
			if (flag_suc == true)
			{
				ereport(ERROR,
						(errmsg("input paramter '%s' format error!",audit_result_str)));
			}
			*isSuccess = AUDIT_SUCCESS;
			flag_suc = true;
		}
		else if (pg_strcasecmp(words, FAIL_S) == 0)
		{
			if (flag_suc == true)
			{
				ereport(ERROR,
						(errmsg("input paramter '%s' format error!",audit_result_str)));
			}
			*isSuccess = AUDIT_FAIL;
			flag_suc = true;
		}
		else if (pg_strcasecmp(words, ALL_S) == 0)
		{
			if (flag_suc == true)
			{
				ereport(ERROR,
						(errmsg("input paramter '%s' format error!",audit_result_str)));
			}
			*isSuccess = AUDIT_ALL;
			flag_suc = true;
		}
		else
		{
			return -1;
		}

	}while(nextstr != NULL);

	return 0;
}


/*
 * Function: add_delete_audit_stmt_args3()
 *
 * Description:
 * 	Add or cancel audits
 *
 * Input:
 *		PG_FUNCTION_ARGS	: function Parameters
 *		auditoroid			: audit Role OID
 *		enable_unable 		: add or remove audit tags
 *		audittype			: audit type
 *
 */
static int 
add_delete_audit_stmt_args2(PG_FUNCTION_ARGS, Oid auditoroid, AuditChange enable_unable, AuditType audittype, bool is_all_user)
{
	char	   *statments = NULL;
	char	   *usernames = NULL;
	char	   *audit_result_str = NULL;
	AuditResult		isSuccess = AUDIT_ALL;
	int ret = 0;

	if (audittype == AUDIT_TYPE_USER && !is_all_user)
	{
		usernames = text_to_cstring(PG_GETARG_TEXT_PP(1));
		audit_result_str = text_to_cstring(PG_GETARG_TEXT_PP(2));
	}
	else
	{
		audit_result_str = text_to_cstring(PG_GETARG_TEXT_PP(1));
	}
	ret = audit_check_mode(audit_result_str, &isSuccess);

	if (ret < 0)
	{
		ereport(ERROR,
				(errmsg("input paramter '%s' format error!", audit_result_str)));
	}

	statments = text_to_cstring(PG_GETARG_TEXT_PP(0));
	
	/* select pg_audit_privilege('all', 'all', 'SUCCESSFUL'); */
	return add_delete_audit_stmt_arg1(statments, usernames, auditoroid, InvalidOid, "none", audittype, isSuccess, enable_unable);

}

/*
 * Function: audit_get_objtype()
 *
 * Description:
 * 	get object type
 *
 */
static ObjectType 
audit_get_objtype(char *key)
{
	ObjectType type = -1;

	if (pg_strcasecmp(key, TABLE_S) == 0)
		type = OBJECT_TABLE;
	else if (pg_strcasecmp(key, VIEW_S) == 0)
		type = OBJECT_VIEW;
	else if (pg_strcasecmp(key, COLUMN_S) == 0)
		type = OBJECT_COLUMN;
	else if (pg_strcasecmp(key, SEQUENCE_S) == 0)
		type = OBJECT_SEQUENCE;
	else if (pg_strcasecmp(key, PROCEDURE_S) == 0)
		type = OBJECT_PROCEDURE;
	else if (pg_strcasecmp(key, FUNCTION_S) == 0)
		type = OBJECT_FUNCTION;
	else
		type = -1;

	return type;
}


static char * 
audit_get_objname(ObjectType type)
{
	char   *object = NULL;
	
	switch(type)
	{
		case OBJECT_TABLE:
			object = TABLE_S;
			break;
		case OBJECT_VIEW:
			object = VIEW_S;
			break;
		case OBJECT_COLUMN:
			object = COLUMN_S;
			break;
		case OBJECT_SEQUENCE:
			object = SEQUENCE_S;
			break;
		case OBJECT_PROCEDURE:
			object = PROCEDURE_S;
			break;
		case OBJECT_FUNCTION:
			object = FUNCTION_S;
			break;
		default:
			break;
	}

	return object;
}


/*
 * Function: add_delete_audit_stmt_args1()
 *
 * Description:
 * 	 Add or delete statement audit
 *
 * Input:
 *		statments		: audit statement
 *		usrnames		: audit users
 *		auditoroid		: audit role oid
 *		classoid		: OID of the system table where the object or user is located
 *		objtype			: object type
 *		audittype		: audit type
 *		isSuccess		: Only audit success or failure markers
 *		enable_unable	: Enable or disable this policy
 *
 */
static int 
add_delete_audit_stmt_arg1(char *statments, char *usrnames, Oid auditoroid, Oid classoid, 
										char *objtype, AuditType audittype, AuditResult isSuccess, AuditChange enable_unable)
{
	List	*statments_list = NULL;
	List	*useroids_list = NULL;
	Relation    pg_audit_config;
	List		*options = NIL;		// list for statement_opts and object_opts
	AuditStmtType	auditevent;
	ListCell   *auditeventcell;
	//ObjectType	objectyp;
	ListCell   *usercell;
	Oid  		userid;

	//objectyp = audit_get_objtype(objtype);
		
	statments_list = create_statments_list(audittype, statments, false, -1);
	if (statments_list == NULL)
	{
		return -1;
	}

	if (usrnames != NULL)
	{
		useroids_list = get_userid_list(usrnames);
		if (useroids_list == NULL)
		{
			return -2;
		}
	}
	
	/* Audit cannot support all audit types, and when encountering unsupported 
	 * audit types, it still cannot be audited.Therefore, we can only exhaustively 
	 * list all statements that can be audited here
	 */
	options = audit_get_options_on_statement(audittype, statments_list);

	pg_audit_config = table_open(AuditConfigRelationId, RowExclusiveLock);

	foreach(auditeventcell, options)
	{
		auditevent = lfirst_int(auditeventcell);

		if (useroids_list != NULL && audittype == AUDIT_TYPE_USER)
		{
			foreach(usercell, useroids_list)
			{
				
				char		objectItem[NAMEDATALEN] = {0};
				
				userid = lfirst_oid(usercell);

				snprintf(objectItem, NAMEDATALEN, "%d.0.0", userid);
				
   				audit_deal_with_item(pg_audit_config, auditevent, auditoroid, objectItem, classoid, objtype, audittype, isSuccess, enable_unable);
			}
		}
		else
		{
			char		objectItem[NAMEDATALEN] = {0};
			
			snprintf(objectItem, NAMEDATALEN, "0.0.0");
			
			audit_deal_with_item(pg_audit_config, auditevent, auditoroid, objectItem, classoid, objtype, audittype, isSuccess, enable_unable);
		}
	}
	
	table_close(pg_audit_config, RowExclusiveLock);

	return 0;
}

/* Write object info into list for future use */
static List *
create_object(char *obj, ObjectType type, char *schema)
{
	char	*words = NULL;
	char	*wordS = NULL;
	char	*curstatment = obj;
	char	*nextstatment = NULL;
	unsigned int len = 0;
	List	*statments_list = NULL;

	if (obj == NULL)
		return NULL;

	if (type == OBJECT_COLUMN)
	{	
		nextstatment = strrchr(curstatment, '.');
		if (nextstatment != NULL)
		{
			len = strlen(nextstatment);
			words = palloc0(len);
			memcpy(words, nextstatment + 1, len);

			*nextstatment = '\0';
			
			len = strlen(curstatment);
			wordS = palloc0(len + 1);
			memcpy(wordS, curstatment, len + 1);

			/* Remove spaces before and after */
			trim(wordS);
			statments_list = list_make1(makeString(wordS));

			/* Remove spaces before and after */
			trim(words);
			statments_list = lappend(statments_list, makeString(words));
		}
	}
	else if (type == OBJECT_FUNCTION || type == OBJECT_PROCEDURE)
	{
		statments_list = list_make1(makeString(schema));

		len = strlen(curstatment);
		words = palloc0(len + 1);
		memcpy(words, curstatment, len + 1);

		/* Remove spaces before and after */
		trim(words);
		statments_list = lappend(statments_list, makeString(words));
	}
	else
	{
		len = strlen(curstatment);
		words = palloc0(len + 1);
		memcpy(words, curstatment, len + 1);

		/* Remove spaces before and after */
		trim(words);
		statments_list = list_make1(makeString(words));
	}

	return statments_list;
}


static AuditObjects *
get_objlist(char *objects, AuditObjects *audit_obj, char *schema)
{
	char   *nextobjects = NULL;
	char   *curobjects = objects;
	char   *words;
	List   *objlist = NULL;
	List   *obj = NULL;
	unsigned int len = 0;

	if (audit_obj == NULL)
		return NULL;

	do{
		nextobjects = strchr(curobjects, ',');
		if (nextobjects != NULL)
		{
			len = abs(nextobjects - curobjects);
			words = palloc0(len + 1);
			memcpy(words, curobjects, len);
			curobjects = nextobjects + 1;
		}
		else
		{
			len = strlen(curobjects);
			words = palloc0(len + 1);
			memcpy(words, curobjects, len + 1);
		}

		obj = create_object(words, audit_obj->objtype, schema);

		if (objlist == NULL)
		{
			objlist = list_make1(obj);
		}
		else
		{
			objlist = lappend(objlist, obj);
		}
		pfree(words);

	}while(nextobjects != NULL);

	audit_obj->objlist = objlist;
	return audit_obj;
}

/*
 * Function: audit_get_objtype()
 *
 * Description:
 * 	 	Determine if there are any mismatches in the designated audit object
 * 		If the audit statement is all, expand the audit statement into all audit 
 *		operations corresponding to the corresponding object
 *
 * Input:
 *		objtype : object type
 *		optionlist: object audit list
 *
 * Output:
 * Return:
 *	Rewritten object audit list
 *
 */
static List *
audit_get_options_on_object(ObjectType objtype, List *optionlist)
{
	AuditStmtType	auditevent;
	ListCell   *auditeventcell;
	List	   *options = optionlist;

		switch(objtype)
		{
			case OBJECT_TABLE:
				auditevent = lfirst_int(list_head(optionlist));
				if(auditevent == ALL_T)
				{
					options = list_make5_int(INSERT_T, UPDATE_T, DELETE_T, SELECT_T, LOCK_T);
					options = lappend_int(options, ALTER_TABLE);
					options = lappend_int(options, COMMENT_T);
					options = lappend_int(options, TRUNCATE_T);
					options = lappend_int(options, ALTER_INDEX);
					options = lappend_int(options, DROP_INDEX);
					break;
				}

				options = optionlist;
				foreach(auditeventcell, options)
				{
					auditevent = lfirst_int(auditeventcell);
					switch(auditevent)
					{
						case INSERT_T:
						case UPDATE_T:
						case DELETE_T:
						case SELECT_T:
						case LOCK_T:
						case COMMENT_T:
						case TRUNCATE_T:
						case ALTER_TABLE:
						case ALTER_INDEX:
						case DROP_INDEX:
							break;
						default:
							ereport(ERROR,
									(errmsg("option %s on table is not supported to audit", audit_stmttype_to_string(auditevent))));
					}
				}
				break;
			case OBJECT_VIEW:
				auditevent = lfirst_int(list_head(optionlist));
				if(auditevent == ALL_T)
				{
					options = list_make5_int(SELECT_T, COMMENT_T, INSERT_T, UPDATE_T, DELETE_T);
					break;
				}

				options = optionlist;
				foreach(auditeventcell, options)
				{
					auditevent = lfirst_int(auditeventcell);
					switch(auditevent)
					{
						case SELECT_T:
						case COMMENT_T:
						case INSERT_T:
						case UPDATE_T:
						case DELETE_T:
							break;
						default:
							ereport(ERROR,
									(errmsg("option %s on view/sequence is not supported to audit", audit_stmttype_to_string(auditevent))));
					}
				}
				break;
			case OBJECT_SEQUENCE:
				auditevent = lfirst_int(list_head(optionlist));
				if(auditevent == ALL_T)
				{
					options = list_make4_int(SELECT_T,COMMENT_T, ALTER_SEQUENCE, DROP_SEQUENCE);
					//options = list_make5_int(SELECT_T,COMMENT_T, ALTER_SEQUENCE, DROP_SEQUENCE, CREATE_SEQUENCE);
					break;
				}

				options = optionlist;
				foreach(auditeventcell, options)
				{
					auditevent = lfirst_int(auditeventcell);
					switch(auditevent)
					{
						case SELECT_T:
						case COMMENT_T:
						case ALTER_SEQUENCE:
						case DROP_SEQUENCE:
						//case CREATE_SEQUENCE:
							break;
						default:
							ereport(ERROR,
									(errmsg("option %s on view/sequence is not supported to audit", audit_stmttype_to_string(auditevent))));
					}
				}
				break;
			case OBJECT_PROCEDURE:
				auditevent = lfirst_int(list_head(optionlist));
				if(auditevent == ALL_T)
				{
					options = list_make3_int(CALL_T, ALTER_PROCEDURE, DROP_PROCEDURE);
					//options = list_make4_int(CALL_T, ALTER_PROCEDURE, DROP_PROCEDURE, CREATE_PROCEDURE);
					break;
				}
				options = optionlist;
				foreach(auditeventcell, options)
				{
					auditevent = lfirst_int(auditeventcell);
					switch(auditevent)
					{
						case CALL_T:
						case ALTER_PROCEDURE:
						case DROP_PROCEDURE:
						//case CREATE_PROCEDURE:
							break;
						default:
							ereport(ERROR,
									(errmsg("option %s on produce is not supported to audit", audit_stmttype_to_string(auditevent))));
					}
				}
				break;

			case OBJECT_FUNCTION:
				auditevent = lfirst_int(list_head(optionlist));
				if(auditevent == ALL_T)
				{
					//options = list_make4_int(SELECT_T, ALTER_FUNCTION, DROP_FUNCTION, CREATE_FUNCTION);
					options = list_make3_int(SELECT_T, ALTER_FUNCTION, DROP_FUNCTION);
					break;
				}
				options = optionlist;
				foreach(auditeventcell, options)
				{
					auditevent = lfirst_int(auditeventcell);
					switch(auditevent)
					{
						case SELECT_T:
						case ALTER_FUNCTION:
						case DROP_FUNCTION:
						//case CREATE_FUNCTION:
							break;
						default:
							ereport(ERROR,
									(errmsg("option %s on function is not supported to audit", audit_stmttype_to_string(auditevent))));
					}
				}
				break;
			case OBJECT_COLUMN:
				auditevent = lfirst_int(list_head(optionlist));
				if(auditevent == ALL_T)
				{
					options = list_make3_int(INSERT_T, UPDATE_T, SELECT_T);
					break;
				}
				options = optionlist;
				foreach(auditeventcell, options)
				{
					auditevent = lfirst_int(auditeventcell);
					switch(auditevent)
					{
						case SELECT_T:
						case UPDATE_T:
						case INSERT_T:
							break;
						default:
							ereport(ERROR,
									(errmsg("option %s on column is not supported to audit", audit_stmttype_to_string(auditevent))));
					}
				}
				break;
			default:
				ereport(ERROR,
							 (errmsg("object %d is not supported to audit", objtype)));
	}

	return options;
}

/*
 * Function: audit_get_object_from_namelist()
 *
 * Description:
 * 		get object information from the list
 *
 * Input:
 *		objtype : object type
 *
 * Output:
 * Return:
 *	object information
 *
 */
static ObjectInfo *
audit_get_object_from_namelist(List *obj, ObjectType objtype, char *schema)
{
	ObjectInfo *res =  palloc0(sizeof(ObjectInfo));
	HeapTuple  tup = NULL;
	FuncCandidateList clist;

	switch (objtype)
	{
		case OBJECT_TABLE:
		case OBJECT_VIEW:
		case OBJECT_SEQUENCE:
			switch (list_length(obj))
			{
				case 1:
					res->schema = schema;
					res->object = strVal(linitial(obj));
					res->schemaid = get_namespace_oid(schema,false);
					res->objectid = get_relname_relid(res->object, res->schemaid);
					break;
				default:
					ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("improper relation name (too many dotted names): %s",
							NameListToString(obj))));
			}
			break;
		case OBJECT_COLUMN:
			switch (list_length(obj))
			{
				case 2:
					res->schema = schema;
					res->object = strVal(linitial(obj));
					res->objectid = RelnameGetRelid(res->object);
					res->attr = strVal(lsecond(obj));
					res->schemaid = get_namespace_oid(schema, false);
					res->objectid = get_relname_relid(res->object, res->schemaid);
					break;
				default:
					ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("improper relation name (too many or too few dotted names): %s",
							NameListToString(obj))));
			}

			/* get column information*/
			tup = SearchSysCache2(ATTNAME,
									   ObjectIdGetDatum(res->objectid),
									   CStringGetDatum(res->attr));

			res->attnum = InvalidAttrNumber;
			if (HeapTupleIsValid(tup))
			{
				Form_pg_attribute attributeForm;
			
				attributeForm = (Form_pg_attribute) GETSTRUCT(tup);
				/* We want to return NULL for dropped columns */
				if (!attributeForm->attisdropped)
					res->attnum = attributeForm->attnum;
				ReleaseSysCache(tup);
			}

			if(res->attnum == InvalidAttrNumber)
				ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("Invalid column %s.%s ",res->object,res->attr)));			
			break;

		case OBJECT_PROCEDURE:
		case OBJECT_FUNCTION:
			clist = FuncnameGetCandidates(obj, -1, NIL, false, false, false, false);
			if (clist == NULL)
			{
				char	   *schemaname;
				char	   *funcname;
				DeconstructQualifiedName(obj, &schemaname, &funcname);
				if (schemaname != NULL)
				{
					ereport(ERROR,
							(errcode(ERRCODE_UNDEFINED_FUNCTION),
							errmsg("function \"%s.%s\" does not exist", schemaname,funcname)));
				}
				else
				{
					ereport(ERROR,
							(errcode(ERRCODE_UNDEFINED_FUNCTION),
							errmsg("function \"%s\" does not exist", funcname)));
				}
			}
			else if (clist->next != NULL)
			{
				char	   *schemaname;
				char	   *funcname;
				DeconstructQualifiedName(obj, &schemaname, &funcname);
				if (schemaname != NULL)
				{
					ereport(ERROR,
							(errcode(ERRCODE_UNDEFINED_FUNCTION),
							errmsg("more than one function named \"%s.%s\"", schemaname,funcname)));
				}
				else
				{
					ereport(ERROR,
							(errcode(ERRCODE_UNDEFINED_FUNCTION),
							errmsg("more than one function named \"%s\"", funcname)));
				}
			}
			res->objectid = clist->oid;
			res->schema = schema;
			res->schemaid = get_namespace_oid(schema,false);
			break;

		default:
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("not supported type %d",objtype)));
			
	}

	if(res->objectid == InvalidOid)
		ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("Invalid object %s ",res->object)));	

	return res;
}

/*
 * Function: add_delete_object_tuple()
 *
 * Description:
 *	   Modify rows in the specified pg_audid_config table
 * Input:
 *		objects       : object information
 *		auditoroid    : audit role oid
 *		statments_list: statement list
 *		objtype		  :	object type
 *		schema		  : schame name
 *		isSuccess	  : Only audit success or failure markers
 *		enable_unable : Enable or disable this policy
 *
 */
static void 
add_delete_object_tuple(AuditObjects *objects, Oid auditoroid, List *statments_list,
									char *objtype, char *schema, AuditResult isSuccess, AuditChange enable_unable)
{
	Relation 	pg_audit_config;
	ListCell   *objectcell;
	ObjectInfo *object;
	ListCell   *auditeventcell;
	AuditStmtType	auditevent;
	Oid			obj_class;

	pg_audit_config = table_open(AuditConfigRelationId, RowExclusiveLock);
		
	foreach(objectcell, objects->objlist)
	{
		char		objectItem[NAMEDATALEN] = {0};
		
		object = audit_get_object_from_namelist(lfirst(objectcell), objects->objtype, schema);

		snprintf(objectItem, NAMEDATALEN, "%d.%d.%d", object->schemaid, object->objectid, object->attnum);

		switch (objects->objtype)
		{
			case OBJECT_TABLE:
			case OBJECT_VIEW:
				obj_class = RelationRelationId;
				break;
			case OBJECT_SEQUENCE:
				obj_class = SequenceRelationId;
				break;
			case OBJECT_COLUMN:
				obj_class = AttributeRelationId;
				break;
			case OBJECT_PROCEDURE:
			case OBJECT_FUNCTION:
				obj_class = ProcedureRelationId;
				break;
			default:
				obj_class = InvalidOid; // should't be here
				break;
					
		}
		
		foreach(auditeventcell, statments_list)
		{
			auditevent = lfirst_int(auditeventcell);
			
			audit_deal_with_item(pg_audit_config, auditevent, auditoroid, objectItem, obj_class, objtype, AUDIT_TYPE_OBJECT, isSuccess, enable_unable);
		}
		
		pfree(object);
	}
	table_close(pg_audit_config, RowExclusiveLock);

	return;
}

/*
 * Function: add_delete_audit_object3()
 *
 * Description:
 * 	 Analyze object audit information, configure or cancel object audit policies
 *
 * Input:
 *		PG_FUNCTION_ARGS	: function Parameters
 *		auditoroid			: audit Role OID
 *		enable_unable 		: add or remove audit tags
 *		audittype			: audit type
 *
 */
static int 
add_delete_audit_object3(PG_FUNCTION_ARGS, Oid auditoroid, char *schema, AuditResult isSuccess, AuditChange enable_unable)
{
	List	*statments_list = NULL;
	AuditObjects audit_obj;
	char	   *statments = NULL;
	char	   *key_value = NULL;
	char	   *obj_name = NULL;

	statments = text_to_cstring(PG_GETARG_TEXT_PP(0));
	key_value = text_to_cstring(PG_GETARG_TEXT_PP(1));
	obj_name = text_to_cstring(PG_GETARG_TEXT_PP(2));

	trim(key_value);
	trim(obj_name);

	audit_obj.objtype = audit_get_objtype(key_value);
	if (audit_obj.objtype == -1)
	{
		ereport(ERROR,
				(errmsg("object type '%s' is not supported", key_value)));
	}
	if (!OidIsValid(get_namespace_oid(schema, true)))
	{
		ereport(ERROR,
				(errmsg("schema \"%s\" does not exist", schema)));
	}


	get_objlist(obj_name, &audit_obj, schema);

	statments_list = create_statments_list(AUDIT_TYPE_OBJECT, statments, true, audit_obj.objtype);
	if (statments_list == NULL)
	{
		ereport(ERROR,
				(errmsg("input paramter '%s' format error!", statments)));
	}
	
	/* determine whether the statement is valid */
	statments_list = audit_get_options_on_object(audit_obj.objtype, statments_list);

	add_delete_object_tuple(&audit_obj, auditoroid, statments_list, key_value, schema, isSuccess, enable_unable);

	return 0;
}

/*
 * Function: add_delete_audit_object4()
 *
 * Description:
 * 	 enable or disable object audit
 *
  * Input:
 *		PG_FUNCTION_ARGS	: function Parameters
 *		auditoroid			: audit Role OID
 *		enable_unable 		: add or remove audit tags
 *		audittype			: audit type
 *
 */
static int 
add_delete_audit_object4(PG_FUNCTION_ARGS, Oid auditoroid, AuditResult isSuccess, AuditChange enable_unable)
{
	char *schema = text_to_cstring(PG_GETARG_TEXT_PP(3));

	return add_delete_audit_object3(fcinfo, auditoroid, schema, isSuccess, enable_unable);
}

static int 
add_delete_audit_object5(PG_FUNCTION_ARGS, Oid auditoroid, AuditChange enable_unable)
{
	char *schema = text_to_cstring(PG_GETARG_TEXT_PP(3));
	char *audit_result_str = text_to_cstring(PG_GETARG_TEXT_PP(4));
	AuditResult		isSuccess = AUDIT_ALL;
	int ret = 0;

	ret = audit_check_mode(audit_result_str, &isSuccess);
	if (ret != 0)
	{
		ereport(ERROR,
				(errmsg("input paramter '%s' format error!", audit_result_str)));
	}

	return add_delete_audit_object3(fcinfo, auditoroid, schema, isSuccess, enable_unable);
}


/*
 * Function: delete_audit_stmt_args1()
 *
 * Description:
 * 	 Cancel the line specified by the confid statement audit
 *
 * Input:
 *		confid			:  confid field of the table
 *
 */
static int 
disable_audit_stmt_byoid(Oid confid, char mode)
{
	Relation 	pg_audit_config;
	HeapTuple tup;
	Form_pg_audit_config audit_form;
	Datum values[Natts_pg_audit_config];
	bool nulls[Natts_pg_audit_config];
	bool replaces[Natts_pg_audit_config];
	HeapTuple newtuple;
	int flag = 0;

	pg_audit_config = table_open(AuditConfigRelationId, RowExclusiveLock);
	tup = SearchSysCache1(AUDITCONFIGID, ObjectIdGetDatum(confid));
	if (!HeapTupleIsValid(tup))
	{
		flag = 0;
	}
	else
	{
		audit_form = (Form_pg_audit_config) GETSTRUCT(tup);	

		if (audit_form->audittype != mode)
		{
			flag = 1;
		}

		else if (audit_form->enable == AUDIT_EANBLE)
		{
			MemSet(values, 0, sizeof(values));
			MemSet(nulls, false, sizeof(nulls));
			MemSet(replaces, false, sizeof(replaces));

			replaces[Anum_pg_audit_config_enable - 1] = true;
			values[Anum_pg_audit_config_enable - 1] = CharGetDatum(AUDIT_UNABLE);

			newtuple = heap_modify_tuple(tup, RelationGetDescr(pg_audit_config), values,
								 nulls, replaces);
			CatalogTupleUpdate(pg_audit_config, &newtuple->t_self, newtuple);

			heap_freetuple(newtuple);

			flag = 2;
		}
		/* when audit configuration is false, the operation can also be cancelled successfully */
		else
		{
			flag = 2;
		}
	}

	if (HeapTupleIsValid(tup))
			ReleaseCatCache(tup);
	
	table_close(pg_audit_config, RowExclusiveLock);

	return flag;
}


void
_PG_init(void)
{
	/* Must be loaded with shared_preload_libraries */
	if (!process_shared_preload_libraries_in_progress)
	{
		return;
	}
	
	audit_define_GUC();
	audit_change_GUC();

	EmitWarningsOnPlaceholders("pg_audit");

	audit_append_hook();
	audit_register_br();
}

/* Uninstall hooks */
void
_PG_fini(void)
{
	ProcessUtility_hook = next_ProcessUtility_hook ;
	ExecutorCheckPerms_hook = next_ExecutorCheckPerms_hook;
}

/*
 * Function: ddl_command_end()
 *
 * Description:
 * 	 The trigger function at the end of a DDL statement 
 *   is used to retrieve object information for SQL
 *
 * Input:
 *	PG_FUNCTION_ARGS:
 *	query: SQL statements that need to be executed
 *
 */
void audit_ddl_command_end(PG_FUNCTION_ARGS, const char *query)
{
	int result,
		row;
	TupleDesc spiTupDesc;
	MemoryContext contextQuery;
	MemoryContext contextOld;
	MemoryContext contexttmp;

	if(audit_GUC.audit_enable != AUDIT_ON)
		return;

	/* Be sure the module was loaded */
	if (!auditEventStack)
		return;

	/* This is an internal statement - do not log it */
	internalStatement = true;

	/* Make sure the fuction was fired as a trigger */
	if (!CALLED_AS_EVENT_TRIGGER(fcinfo))
		elog(ERROR, "not fired by event trigger manager");

	/* Switch memory context for query */
	contextQuery = AllocSetContextCreate(
										CurrentMemoryContext,
										"audit_ddl_command_end temporary context",
										ALLOCSET_DEFAULT_MINSIZE,
										ALLOCSET_DEFAULT_INITSIZE,
										ALLOCSET_DEFAULT_MAXSIZE);
	contextOld = MemoryContextSwitchTo(contextQuery);

	/* Attempt to connect */
	result = SPI_connect();
	if (result < 0)
		elog(ERROR, "audit_ddl_command_end: SPI_connect returned %d",
				result);

	/* Execute the query */
	result = SPI_execute(query, true, 0);
	if (result != SPI_OK_SELECT)
		elog(ERROR, "audit_ddl_command_end: SPI_execute returned %d",
				result);

	/* Iterate returned rows */
	spiTupDesc = SPI_tuptable->tupdesc;
	for (row = 0; row < SPI_processed; row++)
	{
		HeapTuple 	 spiTuple;

		char		*classid;
		char		*objid;
		char		*objsubid;
		char 		*schemaName;
		char 		*objectName;
		Oid			obj_class = InvalidOid;
		HeapTuple 	classtuple;
		HeapTuple	proctup;
		ObjectType	 objtype = InvalidObjType;
		AuditEvent_Obj *obj;
		Oid 		 Objid;
		AttrNumber	 attnum;
		char		*colname = NULL;

		spiTuple = SPI_tuptable->vals[row];
		classid = SPI_getvalue(spiTuple, spiTupDesc, 1);
		objid = SPI_getvalue(spiTuple, spiTupDesc, 2);
		objsubid = SPI_getvalue(spiTuple, spiTupDesc, 3);

		if (classid == NULL && objid == NULL && objsubid == NULL)
		{
			continue;
		}

		if (strcmp(auditEventStack->auditEvent->actionname, ALTER_SEQUENCE_S) == 0 ||
			strcmp(auditEventStack->auditEvent->actionname, ALTER_FUNCTION_S) == 0)
		{
			continue;
		}

		Objid = (Oid)atoi(objid);
		attnum = (AttrNumber)atoi(objsubid);

		if (attnum != InvalidAttrNumber)
			objtype = OBJECT_COLUMN;
		else
		{
			switch(atoi(classid))
			{
				case RelationRelationId:
					obj_class = RelationRelationId;
					classtuple = SearchSysCache1(RELOID,ObjectIdGetDatum(atoi(objid)));
					if(HeapTupleIsValid(classtuple))
					{
						switch(((Form_pg_class) GETSTRUCT(classtuple))->relkind)
						{
							case RELKIND_RELATION:
							case RELKIND_TOASTVALUE:
							case RELKIND_FOREIGN_TABLE :
							case RELKIND_PARTITIONED_TABLE :
								objtype = OBJECT_TABLE;
								break;
							case RELKIND_VIEW:
							case RELKIND_MATVIEW:
								objtype = OBJECT_VIEW;
								break;
							case RELKIND_SEQUENCE :
								objtype = OBJECT_SEQUENCE;
								break;
							default:
								break;
						}
						ReleaseCatCache(classtuple);
					}
					break;

				case ProcedureRelationId:
					obj_class = ProcedureRelationId;
					proctup = SearchSysCache1(PROCOID, ObjectIdGetDatum(atoi(objid)));
					if (HeapTupleIsValid(proctup))
					{
						switch(((Form_pg_proc) GETSTRUCT(proctup))->prokind)
						{
							case 'p':
								objtype = OBJECT_PROCEDURE;
								break;
							case 'f':
								objtype = OBJECT_FUNCTION;
								break;
							default:
								break;
						}
						ReleaseCatCache(proctup);
					}
					break;
			}
		}
		if (objtype == InvalidObjType)
			continue;

		contexttmp = MemoryContextSwitchTo(auditEventStack->auditEvent->auditContext);

		/* column audit judgment*/
		if (objtype == OBJECT_COLUMN)
		{
			HeapTuple attTuple;
			attTuple = SearchSysCache2(ATTNUM,
										 ObjectIdGetDatum(Objid),
										 Int16GetDatum(attnum));
			if (HeapTupleIsValid(attTuple))
			{
				Form_pg_attribute attributeForm  = (Form_pg_attribute) GETSTRUCT(attTuple);

				if(!attributeForm->attisdropped)
				{
					colname = pstrdup(NameStr(attributeForm->attname));
				}
				ReleaseSysCache(attTuple);
			}
		}
	
		schemaName = SPI_getvalue(spiTuple, spiTupDesc, 4);
		objectName = SPI_getvalue(spiTuple, spiTupDesc, 5);

		obj = palloc0(sizeof(AuditEvent_Obj));
		obj->classId = obj_class;
		obj->objectId = Objid;
		
		if (objectName)
		{
			char	*dot = strchr(objectName, '.');
			if (dot != NULL)
			{
				size_t len = strlen(dot + 1);
				char *result_obj = palloc0(len + 1);

				if (result_obj != NULL)
				{
					strcpy(result_obj, dot + 1);
					obj->objectName = result_obj;
				}
				
			}
			else
			{
				obj->objectName = pstrdup(objectName);
			}
		}
		
		if (schemaName)
		{
			obj->schemaId = get_namespace_oid(schemaName, true);
			obj->schemaName = pstrdup(schemaName);
		}
		
		if (colname)
		{
			obj->colname = pstrdup(colname);
			obj->attnum = attnum;
		}
		obj->objectClassdesc = audit_get_objname(objtype);

		auditEventStack->auditEvent->objects = lappend(auditEventStack->auditEvent->objects, obj);

		MemoryContextSwitchTo(contexttmp);
	}

	/* Complete the query */
	SPI_finish();

	MemoryContextSwitchTo(contextOld);
	MemoryContextDelete(contextQuery);

	/* No longer in an internal statement */
	internalStatement = false;

	return;
}


Datum
pg_audit_ddl_command_end(PG_FUNCTION_ARGS)
{
	const char *query;

	if (audit_GUC.audit_enable != AUDIT_ON)
		PG_RETURN_NULL();

	/* Return objects affected by the (non drop) DDL statement */
	query = "SELECT classid, objid, objsubid, schema_name,object_identity \n"
				"  FROM pg_catalog.pg_event_trigger_ddl_commands()";

	audit_ddl_command_end(fcinfo, query);

	PG_RETURN_NULL();
}

Datum
pg_audit_sql_drop(PG_FUNCTION_ARGS)
{
	const char *query;

	if (audit_GUC.audit_enable != AUDIT_ON)
		PG_RETURN_NULL();

	/* Return objects affected by the drop statement */
	query = "SELECT classid, objid, objsubid,schema_name,object_name \n"
					"  FROM pg_catalog.pg_event_trigger_dropped_objects()\n"
					" WHERE lower(object_type) <> 'type'\n"
					"   AND schema_name <> 'pg_toast'";

	audit_ddl_command_end(fcinfo, query);

	PG_RETURN_NULL();
}


Datum
pg_audit_file_archive(PG_FUNCTION_ARGS)
{
	int			ret;
	char		archcmd[MAXPGPATH] = {0};
	char	   *archpath;
	TimestampTz cur_time;
	char *time_str = NULL;
	char	   *username = NULL;
	Oid 		userid = GetUserId();

	/* permission check */
	check_audit_saosso_perm(userid, false);

	if (audit_GUC.audit_archdir[0] == '\0')
	{
		ereport(ERROR,
				(errmsg("audit archive directory is not configured, please configure it first!")));
	}

	archpath = palloc0(MAXPGPATH);

	sprintf(archpath, "%s/%d", audit_GUC.audit_archdir, MyDatabaseId);

	
	ret = audit_mkdirs(archpath);
	if (ret < 0)
	{
		ereport(ERROR,
				(errmsg("could not create directory \"%s\"", archpath)));
	}

	cur_time = GetCurrentTimestamp();
	time_str = pstrdup(timestamptz_to_str(cur_time));
	time_str[10] = '-';

	username = GetUserNameFromId(userid, true);

	if (se_feature_mode)
	{
		/* Begin - BUG#00004872 */
		if (audit_GUC.audit_privimode == 1)
		{
			if (IS_SYSSAO(userid))
			{
				snprintf(archcmd, MAXPGPATH, "copy (select * from hg_security.pg_audit_log) to '%s/%s-%d-%s.csv'", archpath, username, MyDatabaseId, time_str);	/* BUG#Z5840 */
			}
			
		}
		else
		{
			if (IS_SYSSAO(userid))
			{
				snprintf(archcmd, MAXPGPATH, "copy (select * from hg_security.pg_audit_log where rolename = 'sysdba_role' or rolename = 'syssso_role') to '%s/%s-%d-%s.csv'", 
							archpath, username, MyDatabaseId, time_str);
			}
		
			else
			{
				snprintf(archcmd, MAXPGPATH, "copy (select * from hg_security.pg_audit_log where rolename != 'sysdba_role' and rolename != 'syssso_role' or username IS NULL) to '%s/%s-%d-%s.csv'", 
							archpath, username, MyDatabaseId, time_str);
			}
		}
		/* End - BUG#00004872 */
	}
	else
	{
		if (superuser())
		{
			snprintf(archcmd, MAXPGPATH, "copy (select * from hg_security.pg_audit_log) to '%s/%s-%d-%s.csv'", archpath, username, MyDatabaseId, time_str);	/* BUG#Z5840 */
		}
	}

	internalStatement = true;
	
	/* Connect to SPI manager */
	if ((ret = SPI_connect()) < 0)
	{
		internalStatement = false;
		/* internal error */
		elog(WARNING, "pg_audit_file_archive: SPI_connect returned %d", ret);
		PG_RETURN_BOOL(false);
	}
	
	/* Now retrieve the crosstab source rows */
	ret = SPI_execute(archcmd, false, 0);

	/* Check for qualifying tuples */
	if (ret != SPI_OK_UTILITY)
	{
		internalStatement = false;
		elog(WARNING, "pg_audit_file_archive: SPI_execute returned %d", ret);
		PG_RETURN_BOOL(false);
	}

	SPI_finish();

	internalStatement = false;

	PG_RETURN_BOOL(true);
}

Datum
copy_file_to_remote_srv(PG_FUNCTION_ARGS)
{
	text    *sourcevar = PG_GETARG_TEXT_P(0);
	text    *destvar = PG_GETARG_TEXT_P(1);
	StringInfoData buf;
	int			rc;

	/* permission check */
	check_audit_sao_permission(GetUserId(), false);

	initStringInfo(&buf);

	appendStringInfo(&buf, "scp ");
	appendBinaryStringInfo(&buf, VARDATA_ANY(sourcevar), VARSIZE_ANY_EXHDR(sourcevar));
	appendStringInfoSpaces(&buf, 1);
	appendBinaryStringInfo(&buf, VARDATA_ANY(destvar), VARSIZE_ANY_EXHDR(destvar));

	rc = system(buf.data);
	if (rc == 127 || rc == -1 || rc ==256)
		elog(ERROR, "copy audit log to remote server failed");

	PG_RETURN_VOID();
}

Datum
pg_audit_privilege(PG_FUNCTION_ARGS)
{
	int 		nargs = PG_NARGS();
	char	   *statments = text_to_cstring(PG_GETARG_TEXT_PP(0));
	char		paramStr[MAXPGPATH] = {0};
	int			res = 0;
	Oid			userid = GetUserId();

	/* permission check */
	check_audit_sao_permission(userid, false);

	if (nargs == 1)
	{	
		res = add_delete_audit_stmt_arg1(statments, NULL, userid, InvalidOid, "none", AUDIT_TYPE_PRIVILEGE, AUDIT_ALL, AUDIT_EANBLE);
		if (res == -1)
		{
			ereport(ERROR,
				(errmsg("input paramter '%s' format error!", statments)));
		}
	}
	else if (nargs == 2)
	{
		res = add_delete_audit_stmt_args2(fcinfo, userid, AUDIT_EANBLE, AUDIT_TYPE_PRIVILEGE, false);
		if (res == -1)
		{
			ereport(ERROR,
					(errmsg("input paramter '%s' format error!", statments)));
		}
	}

	sprintf(paramStr, "Successfully configured privilege audit policy!");	
	PG_RETURN_TEXT_P(cstring_to_text(pstrdup(paramStr)));

}


Datum
pg_noaudit_privilege(PG_FUNCTION_ARGS)
{	
	int 		nargs = PG_NARGS();
	char	   *statments = text_to_cstring(PG_GETARG_TEXT_PP(0));
	char		paramStr[MAXPGPATH] = {0};
	int			res = 0;
	Oid			userid = GetUserId();

	/* permission check */
	check_audit_sao_permission(userid, false);

	if (nargs == 1)
	{	
		res = add_delete_audit_stmt_arg1(statments, NULL, userid, InvalidOid, "none", AUDIT_TYPE_PRIVILEGE, AUDIT_ALL, AUDIT_UNABLE);
		if (res == -1)
		{
			ereport(ERROR,
				(errmsg("input paramter '%s' format error!", statments)));
		}
	}
	else if (nargs == 2)
	{
		res = add_delete_audit_stmt_args2(fcinfo, userid, AUDIT_UNABLE, AUDIT_TYPE_PRIVILEGE, false);
		if (res == -1)
		{
			ereport(ERROR,
					(errmsg("input paramter '%s' format error!", statments)));
		}
	}

	sprintf(paramStr, "Successfully canceled privilege audit policy!");	
	PG_RETURN_TEXT_P(cstring_to_text(pstrdup(paramStr)));
}

Datum
pg_noaudit_privilege_byoid(PG_FUNCTION_ARGS)
{
	Oid			confid = PG_GETARG_OID(0);
	char		paramStr[MAXPGPATH] = {0};
	int			res = 0;
	Oid			userid = GetUserId();

	/* permission check */
	check_audit_sao_permission(userid, false);

	res = disable_audit_stmt_byoid(confid, 'p');

	if (res == 2)
		sprintf(paramStr, "Successfully canceled statement audit policy!");
	else
		ereport(ERROR,
					(errmsg("There is no audit strategy available!")));

	PG_RETURN_TEXT_P(cstring_to_text(pstrdup(paramStr)));
}


Datum
pg_audit_stmt(PG_FUNCTION_ARGS)
{
	int 		nargs = PG_NARGS();
	char	   *statments = text_to_cstring(PG_GETARG_TEXT_PP(0));
	char		paramStr[MAXPGPATH] = {0};
	int			res = 0;
	Oid			userid = GetUserId();

	/* permission check */
	check_audit_sao_permission(userid, false);

	if (nargs == 1)
	{	
		res = add_delete_audit_stmt_arg1(statments, NULL, userid, InvalidOid, "none", AUDIT_TYPE_STATEMENT, AUDIT_ALL, AUDIT_EANBLE);
		if (res == -1)
		{
			ereport(ERROR,
				(errmsg("input paramter '%s' format error!", statments)));
		}
	}
	else if (nargs == 2)
	{
		res = add_delete_audit_stmt_args2(fcinfo, userid, AUDIT_EANBLE, AUDIT_TYPE_STATEMENT, false);
		if (res == -1)
		{
			ereport(ERROR,
					(errmsg("input paramter '%s' format error!", statments)));
		}
	}

	sprintf(paramStr, "Successfully configured statement audit policy!");	
	PG_RETURN_TEXT_P(cstring_to_text(pstrdup(paramStr)));
}

Datum
pg_noaudit_stmt(PG_FUNCTION_ARGS)
{
	int 		nargs = PG_NARGS();
	char	   *statments = text_to_cstring(PG_GETARG_TEXT_PP(0));
	char		paramStr[MAXPGPATH] = {0};
	int			res = 0;
	Oid			userid = GetUserId();

	/* permission check */
	check_audit_sao_permission(userid, false);

	if (nargs == 1)
	{	
		res = add_delete_audit_stmt_arg1(statments, NULL, userid, InvalidOid, "none", AUDIT_TYPE_STATEMENT, AUDIT_ALL, AUDIT_UNABLE);
		if (res == -1)
		{
			ereport(ERROR,
				(errmsg("input paramter '%s' format error!", statments)));
		}
	}
	else if (nargs == 2)
	{
		res = add_delete_audit_stmt_args2(fcinfo, userid, AUDIT_UNABLE, AUDIT_TYPE_STATEMENT, false);
		if (res == -1)
		{
			ereport(ERROR,
					(errmsg("input paramter '%s' format error!", statments)));
		}
	}

	sprintf(paramStr, "Successfully canceled statement audit policy!");	
	PG_RETURN_TEXT_P(cstring_to_text(pstrdup(paramStr)));
}

Datum
pg_noaudit_stmt_byoid(PG_FUNCTION_ARGS)
{
	Oid			confid = PG_GETARG_OID(0);
	char		paramStr[MAXPGPATH] = {0};
	int			res = 0;
	Oid			userid = GetUserId();

	/* permission check */
	check_audit_sao_permission(userid, false);

	res = disable_audit_stmt_byoid(confid, 's');

	if (res == 2)
		sprintf(paramStr, "Successfully canceled statement audit policy!");	
	else
		ereport(ERROR,
					(errmsg("There is no audit strategy available!")));
	
	PG_RETURN_TEXT_P(cstring_to_text(pstrdup(paramStr)));
}


Datum
pg_audit_user(PG_FUNCTION_ARGS)
{
	int 		nargs = PG_NARGS();
	char	   *statments = text_to_cstring(PG_GETARG_TEXT_PP(0));
	char	   *usernames = text_to_cstring(PG_GETARG_TEXT_PP(1));
	char		paramStr[MAXPGPATH] = {0};
	int			res = 0;
	Oid			userid = GetUserId();
	
	/* permission check */
	check_audit_sao_permission(userid, false);

	if (nargs == 2)
	{	
		res = add_delete_audit_stmt_arg1(statments, usernames, userid, InvalidOid, "none", AUDIT_TYPE_USER, AUDIT_ALL, AUDIT_EANBLE);
		if (res == -1)
		{
			ereport(ERROR,
				(errmsg("input paramter '%s' format error!", statments)));
		}
		else if (res == -2)
		{
			ereport(ERROR,
				(errmsg("input paramter '%s' format error!", usernames)));
		}
	}
	else if (nargs == 3)
	{
		res = add_delete_audit_stmt_args2(fcinfo, userid, AUDIT_EANBLE, AUDIT_TYPE_USER, false);
		if (res == -1)
		{
			ereport(ERROR,
					(errmsg("input paramter '%s' format error!", statments)));
		}
		else if (res == -2)
		{
			ereport(ERROR,
					(errmsg("input paramter '%s' format error!", usernames)));

		}
		
	}

	sprintf(paramStr, "Successfully configured user audit policy!");	
	PG_RETURN_TEXT_P(cstring_to_text(pstrdup(paramStr)));
}

Datum
pg_audit_all_user(PG_FUNCTION_ARGS)
{
	int 		nargs = PG_NARGS();
	char	   *statments = text_to_cstring(PG_GETARG_TEXT_PP(0));
	char		paramStr[MAXPGPATH] = {0};
	int			res = 0;
	Oid			userid = GetUserId();

	/* permission check */
	check_audit_sao_permission(userid, false);

	if (nargs == 1)
	{	
		res = add_delete_audit_stmt_arg1(statments, NULL, userid, InvalidOid, "none", AUDIT_TYPE_USER, AUDIT_ALL, AUDIT_EANBLE);
		if (res == -1)
		{
			ereport(ERROR,
				(errmsg("input paramter '%s' format error!", statments)));
		}
	}
	else if (nargs == 2)
	{
		res = add_delete_audit_stmt_args2(fcinfo, userid, AUDIT_EANBLE, AUDIT_TYPE_USER, true);
		if (res == -1)
		{
			ereport(ERROR,
					(errmsg("input paramter '%s' format error!", statments)));
		}
		
	}

	sprintf(paramStr, "Successfully configured user audit policy!");	
	PG_RETURN_TEXT_P(cstring_to_text(pstrdup(paramStr)));
}


Datum
pg_noaudit_user(PG_FUNCTION_ARGS)
{
	int 		nargs = PG_NARGS();
	char	   *statments = text_to_cstring(PG_GETARG_TEXT_PP(0));
	char	   *usernames = text_to_cstring(PG_GETARG_TEXT_PP(1));
	char		paramStr[MAXPGPATH] = {0};
	int			res = 0;
	Oid			userid = GetUserId();

	/* permission check */
	check_audit_sao_permission(userid, false);

	if (nargs == 2)
	{	
		res = add_delete_audit_stmt_arg1(statments, usernames, userid, InvalidOid, "none", AUDIT_TYPE_USER, AUDIT_ALL, AUDIT_UNABLE);
		if (res == -1)
		{
			ereport(ERROR,
				(errmsg("input paramter '%s' format error!", statments)));
		}
		else if (res == -2)
		{
			ereport(ERROR,
				(errmsg("input paramter '%s' format error!", usernames)));
		}
	}
	else if (nargs == 3)
	{
		res = add_delete_audit_stmt_args2(fcinfo, userid, AUDIT_UNABLE, AUDIT_TYPE_USER, false);
		if (res == -1)
		{
			ereport(ERROR,
					(errmsg("input paramter '%s' format error!", statments)));
		}
		else if (res == -2)
		{
			ereport(ERROR,
					(errmsg("input paramter '%s' format error!", usernames)));

		}
		
	}

	sprintf(paramStr, "Successfully canceled user audit policy!");	
	PG_RETURN_TEXT_P(cstring_to_text(pstrdup(paramStr)));
}

Datum
pg_noaudit_all_user(PG_FUNCTION_ARGS)
{
	int 		nargs = PG_NARGS();
	char	   *statments = text_to_cstring(PG_GETARG_TEXT_PP(0));
	char		paramStr[MAXPGPATH] = {0};
	int			res = 0;
	Oid			userid = GetUserId();

	/* permission check */
	check_audit_sao_permission(userid, false);

	if (nargs == 1)
	{	
		res = add_delete_audit_stmt_arg1(statments, NULL, userid, InvalidOid, "none", AUDIT_TYPE_USER, AUDIT_ALL, AUDIT_UNABLE);
		if (res == -1)
		{
			ereport(ERROR,
				(errmsg("input paramter '%s' format error!", statments)));
		}
	}
	else if (nargs == 2)
	{
		res = add_delete_audit_stmt_args2(fcinfo, userid, AUDIT_UNABLE, AUDIT_TYPE_USER, true);
		if (res == -1)
		{
			ereport(ERROR,
					(errmsg("input paramter '%s' format error!", statments)));
		}
	}

	sprintf(paramStr, "Successfully canceled user audit policy!");	
	PG_RETURN_TEXT_P(cstring_to_text(pstrdup(paramStr)));
}

Datum
pg_noaudit_user_byoid(PG_FUNCTION_ARGS)
{
	Oid			confid = PG_GETARG_OID(0);
	char		paramStr[MAXPGPATH] = {0};
	int			res = 0;
	Oid			userid = GetUserId();

	/* permission check */
	check_audit_sao_permission(userid, false);

	res = disable_audit_stmt_byoid(confid, 'u');

	if (res == 2)
		sprintf(paramStr, "Successfully canceled statement audit policy!");	
	else
		ereport(ERROR,
					(errmsg("There is no audit strategy available!")));	

	PG_RETURN_TEXT_P(cstring_to_text(pstrdup(paramStr)));
}


Datum
pg_audit_object(PG_FUNCTION_ARGS)
{
	char	paramStr[MAXPGPATH] = {0};
	int 	nargs = PG_NARGS();
	Oid		userid = GetUserId();

	check_audit_sao_permission(userid, false);

	if (nargs == 3)
	{
		add_delete_audit_object3(fcinfo, userid, "public", AUDIT_ALL, AUDIT_EANBLE);
	}
	else if (nargs == 4)
	{
		add_delete_audit_object4(fcinfo, userid, AUDIT_ALL, AUDIT_EANBLE);
	}
	else if (nargs == 5)
	{
		add_delete_audit_object5(fcinfo, userid, AUDIT_EANBLE);
	}

	sprintf(paramStr,"Successfully configured object audit policy!");
	PG_RETURN_TEXT_P(cstring_to_text(pstrdup(paramStr)));
}

Datum
pg_noaudit_object(PG_FUNCTION_ARGS)
{
	char	paramStr[MAXPGPATH] = {0};
	int 	nargs = PG_NARGS();
	Oid		userid = GetUserId();

	check_audit_sao_permission(userid, false);

	if (nargs == 3)
	{
		add_delete_audit_object3(fcinfo, userid, "public", AUDIT_ALL, AUDIT_UNABLE);
	}
	else if (nargs == 4)
	{
		add_delete_audit_object4(fcinfo, userid, AUDIT_ALL, AUDIT_UNABLE);
	}
	else if (nargs == 5)
	{
		add_delete_audit_object5(fcinfo, userid, AUDIT_UNABLE);
	}
		
	sprintf(paramStr,"Successfully canceled object audit policy!");
	PG_RETURN_TEXT_P(cstring_to_text(pstrdup(paramStr)));
}

Datum
pg_noaudit_object_byoid(PG_FUNCTION_ARGS)
{
	Oid			confid = PG_GETARG_OID(0);
	char		paramStr[MAXPGPATH] = {0};
	int			res = 0;
	Oid			userid = GetUserId();

	check_audit_sao_permission(userid, false);
	
	res = disable_audit_stmt_byoid(confid, 'o');

	if (res == 2)
		sprintf(paramStr, "Successfully canceled statement audit policy!");	
	else
		ereport(ERROR,
					(errmsg("There is no audit strategy available!")));

	PG_RETURN_TEXT_P(cstring_to_text(pstrdup(paramStr)));
}

static char *
AuditResultToString(AuditResult res)
{
	char *result = SUCCESSFUL_S;

	switch (res)
	{
		case AUDIT_SUCCESS:
			result = SUCCESSFUL_S;
			break;
		case AUDIT_FAIL:
			result = FAIL_S;
			break;
		default:
			break;
	}

	return result;
}

static char * 
AuditTypeToString(AuditType type)
{
	char *tag = AUDIT_TYPE_STATEMENT_S;

	switch(type)
	{
		case AUDIT_TYPE_STATEMENT:
			tag = AUDIT_TYPE_STATEMENT_S;
			break;
		case AUDIT_TYPE_PRIVILEGE:
			tag = AUDIT_TYPE_PRIVILEGE_S;
			break;
		case AUDIT_TYPE_USER:
			tag = AUDIT_TYPE_USER_S;
			break;
		case AUDIT_TYPE_SYSYEM:
			tag = AUDIT_TYPE_SYSYEM_S;
			break;
		case AUDIT_TYPE_OBJECT:
			tag = AUDIT_TYPE_OBJECT_S;
			break;
		case AUDIT_TYPE_MANDATORY:
			tag = AUDIT_TYPE_MANDATORY_S;
			break;
		default:
			break;
	}

	return tag;
}

/*
 * insert tuple into pg_secmon_history
 */
static void
do_insert_secmon_history(Form_pg_secmon_rule rule, Relation history, AuditEvent *event)
{
	Datum	   *values;
	bool	   *nulls;
	HeapTuple	tuple;

	values = palloc(SECMON_RULE_NATTS*sizeof(Datum));
	nulls = palloc(SECMON_RULE_NATTS*sizeof(bool));

	memset(values, 0, SECMON_RULE_NATTS*sizeof(Datum));
	memset(nulls, false, SECMON_RULE_NATTS*sizeof(bool));

	values[0] = TimestampTzGetDatum(event->audittime);
	values[1] = Int32GetDatum(rule->ruleoid);
	values[2] = Int32GetDatum(rule->confid);
	values[3] = PointerGetDatum(cstring_to_text(event->actionname));
	values[4] = Int32GetDatum(event->sessionid);
	values[5] = Int32GetDatum(event->commandid);
	values[6] = PointerGetDatum(cstring_to_text(rule->rulename.data));

	tuple = heap_form_tuple(RelationGetDescr(history), values, nulls);

	simple_heap_insert(history, tuple);

	if(values)
		pfree(values);
	if(nulls)
		pfree(nulls);

	return ;
	
}

/*
 * insert into pg_secmon_history
 */
static void
audit_insert_secmon_history(Form_pg_secmon_rule rule, Relation history, AuditEvent *event)
{
	PG_TRY();
	{
		do_insert_secmon_history(rule, history, event);
	}

	PG_CATCH();
	{
		ErrorData *edata;
		MemoryContext oldcontext = CurrentMemoryContext;
		
		MemoryContextSwitchTo(TopMemoryContext);
		
	    edata = CopyErrorData();
	    FlushErrorState();
	    MemoryContextSwitchTo(oldcontext);

	    if (edata->elevel >= ERROR)
	    {
	        edata->elevel = WARNING;
			ereport(WARNING, (errcode(edata->sqlerrcode),
                    errmsg_internal("%s", edata->message)));
	    }

	}
	PG_END_TRY();

	return;
	
}

/*
 * for sequence threshold, cache_secmon_history function
 * is used to cache the ruleoids list
 */
static void
cache_secmon_history(List *oids, TimestampTz audittime, Oid commandid)
{
	SecmonHisEntry	   *re = NULL;
	MemoryContext		oldcontext;

	/* alloc cache space */
	if (SecmonCacheMemoryContext == NULL)
	{
		SecmonCacheMemoryContext = AllocSetContextCreate(TopMemoryContext,
														"security monitoring memory context",
														ALLOCSET_DEFAULT_MINSIZE,
														ALLOCSET_DEFAULT_INITSIZE,
														ALLOCSET_DEFAULT_MAXSIZE);
		dlist_init(&CachedSecmonHistory.cached_secmon_his);
		CachedSecmonHistory.n_cached_secmon_his = 0;
	}

	/* exceeding the maximum value of the sequence,
	 * remove the oldest entry
	 */
	if (CachedSecmonHistory.n_cached_secmon_his + 1 > MAX_SECMON_SEQSIZE)
	{
		re = dlist_container(SecmonHisEntry, node, dlist_pop_head_node(&CachedSecmonHistory.cached_secmon_his));
		list_free(re->ruleoids);
		re->ruleoids = NIL;
		CachedSecmonHistory.n_cached_secmon_his--;
		
	}

	oldcontext = MemoryContextSwitchTo(SecmonCacheMemoryContext);

	if (re == NULL)
		re = (SecmonHisEntry *) palloc0(sizeof(SecmonHisEntry));

	/* save information to the last node */
	re->ruleoids = list_copy(oids);
	re->audittime = audittime;
	re->commandid = commandid;
	dlist_push_tail(&CachedSecmonHistory.cached_secmon_his, &re->node);
	CachedSecmonHistory.n_cached_secmon_his++;

	MemoryContextSwitchTo(oldcontext);
}


/*
 * check if the current operation is configured
 * with security monitoring rules and if it
 * needs to be recorded
 */
static void
audit_check_secmon_rule(int confindex, Oid confid, AuditEvent *event)
{
	Oid			relOid;
	Relation	secmon_his;
	int			i;
	CatCList   *catlist;
	List	   *ruleoids = NIL;

	relOid = get_relname_relid(PG_SECMON_HISTORY, SECURITY_SCHEMA_NAMESPACE);
	if (relOid == InvalidOid)
		elog(ERROR, " could not get oid of relation pg_secmon_history");

	secmon_his = table_open(relOid, RowExclusiveLock);

	catlist = SearchSysCacheList1(AUDITCONFIDIDRULENAME, ObjectIdGetDatum(confid));

	for (i = 0; i < catlist->n_members; i++)
	{
		SecmonRuleEntry	*re;
		Oid			ruleoid;
		MemoryContext	oldcontext;
		Form_pg_secmon_rule rule_form;
		HeapTuple	rule_tuple = &catlist->members[i]->tuple;
		

		rule_form = (Form_pg_secmon_rule) GETSTRUCT(rule_tuple);

		if (!rule_form->enable)
			continue;

		if (rule_form->monmode != event->result && rule_form->monmode != AUDIT_ALL)
			continue;

		/* inert into pg_secmon_history */
		audit_insert_secmon_history(rule_form, secmon_his, event);	

		/*
		 * we need to write the information to list for the 
		 * purpose of checking the security monitoring
		 * threshold later on 
		 */
		oldcontext = MemoryContextSwitchTo(AuditContext);
		re = (SecmonRuleEntry *) palloc(sizeof(SecmonRuleEntry));
		re->confindex = confindex;
		re->event = (AuditEvent *) palloc(sizeof(AuditEvent));
		*re->event = *event;

		if (event->setname)
		{
			re->event->setname = pnstrdup((const char *)(event->setname), strlen(event->setname));
		}

		if (event->setvalue)
		{
			re->event->setvalue = pnstrdup((const char *)(event->setvalue), strlen(event->setvalue));
		}

		re->event->objects = NIL;

		if (event->conftypes[confindex] == AUDIT_TYPE_OBJECT)
		{
			ListCell *cell;
			
			foreach(cell, event->objects)
			{ 
				AuditEvent_Obj *src_obj = (AuditEvent_Obj *) lfirst(cell); 
				AuditEvent_Obj *new_obj = (AuditEvent_Obj *) palloc(sizeof(AuditEvent_Obj));

				*new_obj = *src_obj;

				new_obj->attnum = src_obj->attnum;
				new_obj->classId = src_obj->classId;
				if (src_obj->colname)
					new_obj->colname = pstrdup(src_obj->colname);
				if (src_obj->columns)
					new_obj->columns = bms_copy(src_obj->columns);
				if (src_obj->objectClassdesc)
					new_obj->objectClassdesc = pstrdup(src_obj->objectClassdesc);
				new_obj->objectId = src_obj->objectId;
				if (src_obj->objectName)
					new_obj->objectName = pstrdup(src_obj->objectName);
				new_obj->indrelId = src_obj->indrelId;
				new_obj->schemaId = src_obj->schemaId;
				if (src_obj->schemaName)
					new_obj->schemaName = pstrdup(src_obj->schemaName);

				re->event->objects = lappend(re->event->objects, new_obj);
			}
		}
		
		re->ruletup = heap_copytuple(rule_tuple);
		ruleoid = rule_form->ruleoid;

		rulelist = lappend(rulelist, re);
		MemoryContextSwitchTo(oldcontext);

		ruleoids = lappend_oid(ruleoids, ruleoid);
		
	}

	cache_secmon_history(ruleoids, event->audittime, event->commandid);

	table_close(secmon_his, RowExclusiveLock);
	secmon_his = NULL;

	ReleaseSysCacheList(catlist);

	return;
}

/*
 * the one-time threshold includes baseline threshold(alter system),
 * important moments and important columns 
 */
static bool
check_onetime_threshold(TimestampTz starttime, char *basename, char *basevalue, TimestampTz impt_starttime,
									TimestampTz impt_endtime, int2vector *impt_columns,  int confindex, AuditEvent *event, char *msg)
{
	/* if the occurrence time of the event is smaller than
	 * the start time in pg_secmon_threshold, it will not
	 * be recored
	 */
	if (timestamp_cmp_internal(event->audittime, starttime) < 0)
	{
		ereport(NOTICE,
				(errmsg("The occurrence time of the event is smaller than the start time in pg_secmon_threshold")));
		//return false; /* BUG#z6114*/
	}

	if (basename != NULL)
	{
		Assert(basevalue != NULL);

		/* Begin BUG#6570 */
		if (event->setname != NULL && event->setvalue != NULL)
		/* End BUG#6570 */
		{

			if (pg_strcasecmp(basename, event->setname) != 0 || 
				pg_strcasecmp(basevalue, event->setvalue) != 0)
			{
				ereport(NOTICE,
						(errmsg("It is not the set baseline threshold")));
				//return false; /* BUG#z6114*/
			}
			else
			{

				snprintf(msg, MAXPGPATH, "The baseline name \"%s\" has reached the baseline threshold \"%s\"", basename, basevalue);

				return true;
			}
			
		}
	}

	if (impt_columns != NULL)
	{	
		List *objects = NULL;
		ListCell *cell;
		
		if (event->conftypes[confindex] != AUDIT_TYPE_OBJECT || event->objects == NULL)
		{
			ereport(NOTICE,
						(errmsg("It is not the important column")));
		}
		else if (event->objects)
		{

			objects = event->objects;
			foreach(cell, objects)
			{
				int			i;
				AuditEvent_Obj *obj = lfirst(cell);

				if (obj->colname != NULL && strlen(obj->colname) > 0)
				{
					for (i = 0; i < impt_columns->dim1; i++)
					{
						if (!bms_is_member(impt_columns->values[i] - FirstLowInvalidHeapAttributeNumber, obj->columns))
						{
							ereport(NOTICE,
							(errmsg("It is not the important column")));
							break;
						}
						else
						{	
							char *attname;

							attname = get_attname(obj->objectId, impt_columns->values[i], false);
							
							snprintf(msg, MAXPGPATH, "Important column \"%s\" has reached the threshold", attname);

							return true;
						}
					}
				}
			}
		}
	}

	if (!TIMESTAMP_IS_NOBEGIN(impt_starttime))
	{
		Assert(!TIMESTAMP_IS_NOBEGIN(impt_endtime));

		if (timestamp_cmp_internal(event->audittime, impt_starttime) < 0 || 
			timestamp_cmp_internal(event->audittime, impt_endtime) > 0)
		{
			ereport(NOTICE,
					(errmsg("The event occurrence time is less than the start time or the event occurrence time is greater than the end time")));
			//return false; /* BUG#z6114*/
		}
		else
		{
			const char	*starttime = NULL;
			const char	*endtime = NULL;

			if (impt_starttime == DT_NOBEGIN)
				starttime = pstrdup("-infinity");
			else
				starttime = pstrdup(timestamptz_to_str(impt_starttime));

			if (impt_endtime == DT_NOEND)
				endtime = pstrdup("infinity");
			else
				endtime = pstrdup(timestamptz_to_str(impt_endtime));

			snprintf(msg, MAXPGPATH, "Reached the threshold of \"%s\" to \"%s\" important moments", starttime, endtime);

			return true;
		}
	}
	else
	{
		const char	*starttime = NULL;
		const char	*endtime = NULL;

		starttime = pstrdup(timestamptz_to_str(impt_starttime));

		if (impt_starttime == DT_NOBEGIN)
			starttime = pstrdup("-infinity");
		else
			starttime = pstrdup(timestamptz_to_str(impt_starttime));

		if (impt_endtime == DT_NOEND)
			endtime = pstrdup("infinity");
		else
			endtime = pstrdup(timestamptz_to_str(impt_endtime));

		snprintf(msg, MAXPGPATH, "Reached the threshold of \"%s\" to \"%s\" important moments", starttime, endtime);

		return true;
	}

	return false;
}

/*
 * the maximum threshold is the number of times that
 * a unified audit time occurs within a specified interval
 *
 */
static bool
check_maxtimes_threshold(int32 interval, int32 maxtimes, TimestampTz starttime, Oid ruleoid, char *msg)
{
	Relation	secmon_his_rel;
	Relation	secmon_his_dex;
	Oid			relOid;
	Oid			idxOid;
	TableScanDesc secmon_his_scan;
	HeapTuple	tup;
	ScanKeyData	skey[2];
	TimestampTz	search_start;
	TimestampTz	current_time;
	int16		attnum_audittime;
	int16		attnum_ruleoid;
	int			count = 0;
	bool		result = false;

	current_time = GetCurrentTimestamp();

	search_start = DatumGetTimestampTz(DirectFunctionCall2(timestamptz_mi_interval,
										TimestampTzGetDatum(current_time),
										DirectFunctionCall7(make_interval, 0, 0, 0, 0, 0, interval, 0)));
	
	if (timestamp_cmp_internal(search_start, starttime) < 0)
	{
		search_start = starttime;
	}
	
	relOid = get_relname_relid(PG_SECMON_HISTORY, SECURITY_SCHEMA_NAMESPACE);
	if (relOid == InvalidOid)
	{
		elog(ERROR, " could not get oid of relation pg_secmon_history");
	}

	idxOid = get_relname_relid(PG_SECMON_HISTORY_INDEX, SECURITY_SCHEMA_NAMESPACE);
	if (idxOid == InvalidOid)
	{
		elog(ERROR, " could not get oid of index pg_secmon_history_audttime_ruleoid_idx");
	}

	attnum_audittime = get_attnum(relOid, "audittime");
	attnum_ruleoid = get_attnum(relOid, "ruleoid");

	if (attnum_audittime < 0 || attnum_ruleoid < 0)
	{
		elog(ERROR, " table pg_secmon_history is missing audittime or ruleoid columns");
	}

	/* the first field of the table pg_secmon_history index is audittime */
	ScanKeyInit(&skey[0], attnum_audittime, BTGreaterEqualStrategyNumber, F_TIMESTAMPTZ_GE, TimestampTzGetDatum(search_start));
	/* the second field of the table pg_secmon_history index is ruleoid */
	ScanKeyInit(&skey[1], attnum_ruleoid, BTEqualStrategyNumber, F_OIDEQ, ObjectIdGetDatum(ruleoid));

	secmon_his_rel = table_open(relOid, AccessShareLock);
	secmon_his_dex = index_open(idxOid, AccessShareLock);

	secmon_his_scan = heap_beginscan(secmon_his_rel, GetTransactionSnapshot(), 2, skey, NULL, SO_ALLOW_PAGEMODE);

	if (secmon_his_scan == NULL)
	{
		elog(ERROR, " secmon_his_scan is null");
	}

	while ((tup = heap_getnext(secmon_his_scan, ForwardScanDirection)) != NULL)
	{
		count ++;
		if (count >= maxtimes)
		{
			result = true;
			break;
		}
	}

	if (secmon_his_scan)
		heap_endscan(secmon_his_scan);
	if (secmon_his_dex)
		index_close(secmon_his_dex, AccessShareLock);
	if (secmon_his_rel)
		table_close(secmon_his_rel, AccessShareLock);

	if (result)
	{
		sprintf(msg, "Reached the maximum threshold of %d times within %d minutes", maxtimes, interval);
	}

	return result;	
}

/*
 * check whether the sequence threshold is
 * met through a cached doubly linked list
 */
static bool
check_sequence_threshold(int32 interval, int32 nsteps, TimestampTz starttime, oidvector *ruleseq, char *msg)
{
	int					i;
	SecmonHisEntry	   *she;
	TimestampTz			search_end;
	StringInfoData		str;
	int					j;

	/* no history or not enough */
	if (SecmonCacheMemoryContext == NULL || CachedSecmonHistory.n_cached_secmon_his < nsteps)
		return false;

	/* find end point for search, Max(starttime, now() - interval * 60) */
	search_end = DatumGetTimestampTz(DirectFunctionCall2(timestamptz_mi_interval,
										TimestampTzGetDatum(GetCurrentTransactionStartTimestamp()),
										DirectFunctionCall7(make_interval, 0, 0, 0, 0, 0, interval, 0)));

	if (timestamp_cmp_internal(search_end, starttime) < 0)
		search_end = starttime;

	/* check cached history in reversed order */
	she = dlist_tail_element(SecmonHisEntry, node, &CachedSecmonHistory.cached_secmon_his);
	
	Assert(nsteps > 1);

	for (i = nsteps - 1;; i--)
	{
		if (!list_member_oid(she->ruleoids, ruleseq->values[i]) ||
			timestamp_cmp_internal(search_end, she->audittime) > 0)
			return false;

		if (i <= 0)
			break;

		she = dlist_container(SecmonHisEntry, node,
								dlist_prev_node(&CachedSecmonHistory.cached_secmon_his, &she->node));
	}

	initStringInfo(&str);

	for (j = 0; j < ruleseq->dim1; j++)
	{
		if (j > 0)
			appendStringInfoChar(&str, ',');
		appendStringInfo(&str, "%u", ruleseq->values[j]);
	}

	sprintf(msg, "The sequence of rule oid \"%s\" has reached the sequence threshold", str.data);
	
	return true;	
}

static void
do_insert_secmon_thr_history(Form_pg_secmon_threshold thr, Relation thrhistory, SecmonRuleEntry *rule,
										char *basename, char *basevalue,
										TimestampTz starttime, TimestampTz impt_starttime, TimestampTz impt_endtime,
										int2vector *impt_cloumns, oidvector *ruleseq)
{
	Datum	   *values;
	bool	   *nulls;
	HeapTuple	tuple;
	Form_pg_secmon_rule rule_form;
		
	rule_form = (Form_pg_secmon_rule) GETSTRUCT(rule->ruletup);

	values = palloc(SECMON_THR_NATTS*sizeof(Datum));
	nulls = palloc(SECMON_THR_NATTS*sizeof(bool));

	memset(values, 0, SECMON_THR_NATTS*sizeof(Datum));
	memset(nulls, false, SECMON_THR_NATTS*sizeof(bool));

	values[0] = Int32GetDatum(thr->throid);
	values[1] = Int32GetDatum(rule_form->confid);
	values[2] = Int32GetDatum(rule->event->sessionid);
	values[3] = TimestampTzGetDatum(rule->event->audittime);
	values[4] = Int32GetDatum(rule->event->commandid);
	values[5] = NameGetDatum(&thr->thrname); /* BUG#Z6091 */
	values[6] = Int32GetDatum(rule_form->ruleoid);
	values[7] = Int32GetDatum(thr->interval);
	values[8] = TimestampTzGetDatum(starttime);
	values[9] = Int32GetDatum(thr->level);
	values[10] = Int32GetDatum(thr->maxtimes);
	
	if (impt_starttime == DT_NOBEGIN)
		values[11] = TimestampTzGetDatum(DT_NOBEGIN);
	else
		values[11] = TimestampTzGetDatum(impt_starttime);
	if (impt_endtime == DT_NOEND)
		values[12] = TimestampTzGetDatum(DT_NOEND);
	else
		values[12] = TimestampTzGetDatum(impt_endtime);
	
	values[13] = Int32GetDatum(thr->nsteps);
	
	if (ruleseq != NULL)
		values[14] = PointerGetDatum(ruleseq);
	else
		nulls[14] = true;
	
	if (basename != NULL && strlen(basename) > 0)
		values[15] = PointerGetDatum(cstring_to_text(basename));
	else
		nulls[15] = true;

	if (basevalue != NULL  && strlen(basevalue) > 0)
		values[16] = PointerGetDatum(cstring_to_text(basevalue));
	else
		nulls[16] = true;
	
	if (impt_cloumns != NULL)
		values[17] = PointerGetDatum(impt_cloumns);
	else
		nulls[17] = true;
	
	tuple = heap_form_tuple(RelationGetDescr(thrhistory), values, nulls);

	simple_heap_insert(thrhistory, tuple);

	if(values)
		pfree(values);
	if(nulls)
		pfree(nulls);

	return;
}

static void
audit_insert_secmon_thr_history(Form_pg_secmon_threshold thr, Relation thrhistory, SecmonRuleEntry *rule, 
											char *basename, char *basevalue,
											TimestampTz starttime, TimestampTz impt_starttime, TimestampTz impt_endtime,
											int2vector *impt_cloumns, oidvector *ruleseq)
{
	PG_TRY();
	{
		do_insert_secmon_thr_history(thr, thrhistory, rule, basename, basevalue, starttime, impt_starttime, impt_endtime, impt_cloumns, ruleseq);
	}

	PG_CATCH();
	{
		ErrorData *edata;
		MemoryContext oldcontext = CurrentMemoryContext;
		
		MemoryContextSwitchTo(TopMemoryContext);
		
	    edata = CopyErrorData();
	    FlushErrorState();
	    MemoryContextSwitchTo(oldcontext);

	    if (edata->elevel >= ERROR)
	    {
	        edata->elevel = WARNING;
			ereport(WARNING, (errcode(edata->sqlerrcode),
                    errmsg_internal("%s", edata->message)));
	    }

	}
	PG_END_TRY();

	return;
	
}

static void
audit_send_mail(char *msg)
{
	char	sendmail[MAXPGPATH] = {0};
	int		rc = 0;

	if (audit_GUC.audit_mail != NULL && audit_GUC.audit_mail[0] != '\0')
	{
		snprintf(sendmail, 1024 , "mail -s \"Dangerous operation on database\" %s <<< \'%s\'",
					audit_GUC.audit_mail, msg);

		rc = system(sendmail);

		if	(-1 != rc && WIFEXITED(rc) && WEXITSTATUS(rc) == 0)
			elog(WARNING, "send a mail to %s", audit_GUC.audit_mail);
		else
			elog(WARNING, "failed to send mail to %s", audit_GUC.audit_mail);
	}

	return;
}

static void
audit_terminate_session(void)
{
	ProcDiePending = true;
	InterruptPending = true;
	QueryCancelPending = true;
}

static void
audit_lock_user(void)
{
	Oid			userid;
	Relation	pg_authid_rel;
	HeapTuple	authid_tuple,
				new_tuple;
	Datum		new_record[Natts_pg_authid];
	bool		new_record_nulls[Natts_pg_authid];
	bool		new_record_repl[Natts_pg_authid];

	userid = GetUserId();
	
	pg_authid_rel = table_open(AuthIdRelationId, AccessShareLock);

	authid_tuple = SearchSysCache1(AUTHOID, ObjectIdGetDatum(userid));

	if (!(HeapTupleIsValid(authid_tuple)))
		elog(ERROR, "user with OID %u does not exist", userid);

	MemSet(new_record, 0, sizeof(new_record));
	MemSet(new_record_nulls, false, sizeof(new_record_nulls));
	MemSet(new_record_repl, false, sizeof(new_record_repl));

	new_record_repl[Anum_pg_authid_rolcanlogin - 1] = true;
	new_record[Anum_pg_authid_rolcanlogin - 1] = BoolGetDatum(false);

	new_tuple = heap_modify_tuple(authid_tuple, 
									RelationGetDescr(pg_authid_rel),
									new_record,
									new_record_nulls,
									new_record_repl);

	CatalogTupleUpdate(pg_authid_rel, &authid_tuple->t_self, new_tuple);

	
	ReleaseSysCache(authid_tuple);
	heap_freetuple(new_tuple);

	table_close(pg_authid_rel, AccessShareLock);
}

static void
audit_threshold_alarm(int32 level, char *msg)
{	
	switch (level)
	{
		case 1: 
			ereport(WARNING,
				(errmsg("\"%s\"", msg)));
			break;
		case 2:
			ereport(WARNING,
				(errmsg("\"%s\"", msg)));
			audit_send_mail(msg);
			break;
		case 3:
			ereport(WARNING,
				(errmsg("\"%s\"", msg)));
			audit_send_mail(msg);
			audit_terminate_session();
			break;
		case 4:
			ereport(WARNING,
				(errmsg("\"%s\"", msg)));
			audit_send_mail(msg);
			audit_terminate_session();
			/* Begin BUG#z6135 */
			if(!IS_SYSDBA(GetUserId()))
				audit_lock_user();
			/* End BUG#z6135 */
			break;
		default:
			break;
	}
}

/*
 * check if the current operation is configured
 * with security monitoring threshold and if it
 * needs to be recorded
 */
static void 
audit_secmon_threshold_check(void)
{
	ListCell *listcell;
	Oid 		relOid;
	Relation	secmon_threshold_his;

	StartTransactionCommand();
	
	relOid = get_relname_relid(PG_SECMON_THRESHOLD_HISTORY, SECURITY_SCHEMA_NAMESPACE);
	if (relOid == InvalidOid)
		elog(ERROR, " could not get oid of relation pg_secmon_thrhis");

	secmon_threshold_his = table_open(relOid, RowExclusiveLock);


	/* traverse list */	
	foreach(listcell, rulelist)
	{
		SecmonRuleEntry *ruleevent = lfirst(listcell);
		int 	i;
		CatCList * catlist;
		Form_pg_secmon_rule rule_form;
		
		rule_form = (Form_pg_secmon_rule) GETSTRUCT(ruleevent->ruletup);

		
		catlist = SearchSysCacheList1(AUDITTHRESHOLDRULEIDRULESEQ, ObjectIdGetDatum(rule_form->ruleoid));

		for (i = 0; i < catlist->n_members; i++)
		{
			bool		needed = false;
			char	   *basename = NULL;
			char	   *basevalue = NULL;
			TimestampTz starttime = 0;
			TimestampTz impt_starttime = 0;
			TimestampTz impt_endtime = 0;
			int2vector *impt_cloumns = NULL;
			oidvector  *ruleseq = NULL;
			Datum		datum;
			bool		isnull;
			char		warning_msg[MAXPGPATH] = {0};
			HeapTuple	tup = (HeapTuple) &catlist->members[i]->tuple;
			Form_pg_secmon_threshold thr_form = (Form_pg_secmon_threshold) GETSTRUCT(tup);

			if (!thr_form->enable)
				continue;

			datum = SysCacheGetAttr(AUDITTHRESHOLDRULEIDRULESEQ, tup, Anum_pg_secmon_threshold_basename, &isnull);
			if (!isnull)
				basename = pstrdup(NameStr(*(DatumGetName(datum))));
			
			datum = SysCacheGetAttr(AUDITTHRESHOLDRULEIDRULESEQ, tup, Anum_pg_secmon_threshold_basevalue, &isnull);
			if (!isnull)
				basevalue = pstrdup(NameStr(*(DatumGetName(datum))));

			datum = SysCacheGetAttr(AUDITTHRESHOLDRULEIDRULESEQ, tup, Anum_pg_secmon_threshold_impt_columns, &isnull);
			if (!isnull)
			{
				int2vector *src;
				int dim;

				src = (int2vector *) DatumGetPointer(datum);
				dim = src->dim1;
				
				impt_cloumns = (int2vector *) palloc(Int2VectorSize(dim));
				memcpy(impt_cloumns, src, Int2VectorSize(dim));
			}

			datum = SysCacheGetAttr(AUDITTHRESHOLDRULEIDRULESEQ, tup, Anum_pg_secmon_threshold_ruleseq, &isnull);
			if (!isnull)
			{
				oidvector *src;
				int dim;
				
				src = (oidvector *) DatumGetPointer(datum);
				dim = src->dim1;
				
				ruleseq = (oidvector *) palloc(OidVectorSize(dim));
				memcpy(ruleseq, src, OidVectorSize(dim));
			}

			datum = SysCacheGetAttr(AUDITTHRESHOLDRULEIDRULESEQ, tup, Anum_pg_secmon_threshold_starttime, &isnull);
			if (!isnull)
				starttime = DatumGetTimestampTz(datum);
			
			datum = SysCacheGetAttr(AUDITTHRESHOLDRULEIDRULESEQ, tup, Anum_pg_secmon_threshold_impt_starttime, &isnull);
			if (!isnull)
				impt_starttime = DatumGetTimestampTz(datum);

			datum = SysCacheGetAttr(AUDITTHRESHOLDRULEIDRULESEQ, tup, Anum_pg_secmon_threshold_impt_endtime, &isnull);
			if (!isnull)
				impt_endtime = DatumGetTimestampTz(datum);

			/* 
			 * if nsteps is 1 in pg_secmon_threshold, it is either a one-time
			 * threshold or a maximum threshold 
			 */
			if (thr_form->nsteps == 1)
			{
				if (thr_form->maxtimes == 1)
				{
					needed = check_onetime_threshold(starttime, basename, basevalue, impt_starttime, impt_endtime,
														impt_cloumns, ruleevent->confindex, ruleevent->event, warning_msg);				
				}
				else
				{
					needed = check_maxtimes_threshold(thr_form->interval, thr_form->maxtimes, starttime, thr_form->ruleoid, warning_msg);
				}
			}
			else
			{
				needed = check_sequence_threshold(thr_form->interval, thr_form->nsteps, starttime, ruleseq, warning_msg);
			}

			if (needed)
			{
				audit_threshold_alarm(thr_form->level, warning_msg);
				audit_insert_secmon_thr_history(thr_form, secmon_threshold_his, ruleevent, basename, basevalue, starttime, impt_starttime, impt_endtime, impt_cloumns, ruleseq);
			}
		}

		ReleaseSysCacheList(catlist);

	}	

	table_close(secmon_threshold_his, RowExclusiveLock);
	secmon_threshold_his = NULL;

	CommitTransactionCommand();

	list_free(rulelist);
	rulelist = NIL;
}

static void
audit_WriteThrHistory(void)
{
	if (rulelist)
	{
		/* only write audit logs on the primary node */
		if (!RecoveryInProgress())
		{
			audit_secmon_threshold_check();
		}
		/* do not write audit logs on the standby node */
		else
		{
			list_free(rulelist);
			rulelist = NIL;
		}
	}
}

/* Begin BUG#z6137 */
static void
check_base_line(const char *basename, const char *basevalue)
{
	char 	   *guc_var = NULL;

	guc_var = GetConfigOptionByName(basename, NULL, false);

	if (guc_var == NULL)
	{
		ereport(ERROR,
				(errmsg("basename '%s' does not exist!", basename)));
	}
}
/* End BUG#z6137 */


Datum
pg_create_secmon_rule(PG_FUNCTION_ARGS)
{
	char	   *rulename = text_to_cstring(PG_GETARG_TEXT_PP(0));
	Oid			confid = PG_GETARG_OID(1);
	char	   *monmode = text_to_cstring(PG_GETARG_TEXT_PP(2));
	bool		enable = PG_GETARG_BOOL(3);
	HeapTuple 	tup;
	Oid			ruleoid;
	AuditResult		isSuccess = AUDIT_ALL;
	int 		ret = 0;

	Oid			userid = GetUserId();

	/* permission check */
	check_audit_sao_permission(userid, false);

	/* check if the input parameters are correct */
	ret = audit_check_mode(monmode, &isSuccess);
	if (ret != 0)
	{
		ereport(ERROR,
				(errmsg("input paramter '%s' format error!", monmode)));
	}

	/* check if confid exists in table pg_audit_config */
	tup = SearchSysCache1(AUDITCONFIGID, ObjectIdGetDatum(confid));
	if (!HeapTupleIsValid(tup))
	{
		ereport(ERROR,
						(errmsg("confid %d does not exist in pg_audit_config!", confid)));
		
	}

	ReleaseCatCache(tup);

	/* insert a tuple into table pg_secmon_rule */
	ruleoid = add_secmon_rule(rulename, confid, isSuccess, enable);

	PG_RETURN_OID(ruleoid);
}

Datum
pg_alter_secmon_rule(PG_FUNCTION_ARGS)
{
	int 		nargs = PG_NARGS();
	Oid			ruleoid = PG_GETARG_OID(0);
	char	   *rulename = text_to_cstring(PG_GETARG_TEXT_PP(1));

	Oid			userid = GetUserId();

	/* permission check */
	check_audit_sao_permission(userid, false);

	/*
	 * omitting the timing of the audit indicates
	 * that both successful and fail will be audited;
	 * omitting the enable switch to indicate
	 * that it is turned on
	 */
	if (nargs == 2)
	{	
		alter_secmon_rule_arg2(2, ruleoid, rulename, -1, false);
	}
	/*
	 * omitting the enable switch to indicate
	 * that it is turned on
	 */
	else if (nargs == 3)
	{
		alter_secmon_rule_arg3(fcinfo, ruleoid, rulename);
	}
	else
	{
		alter_secmon_rule_arg4(fcinfo, ruleoid, rulename);
	}

	PG_RETURN_OID(ruleoid);
}

Datum
pg_drop_secmon_rule(PG_FUNCTION_ARGS)
{
	HeapTuple 	tup;
	Relation    pg_secmon_rule;
	Oid		ruleoid = PG_GETARG_OID(0);

	Oid 		userid = GetUserId();

	/* permission check */
	check_audit_sao_permission(userid, false);

	
	if (!OidIsValid(ruleoid))
	{
		ereport(ERROR,
					(errmsg("invalid ruleoid!")));
	}

	pg_secmon_rule = table_open(AuditRuleRelationId, RowExclusiveLock);

	tup = SearchSysCache1(AUDITRULEID, ObjectIdGetDatum(ruleoid));
	if (!HeapTupleIsValid(tup))
	{
		ereport(ERROR,
						(errmsg("ruleoid %d does not exist in pg_secmon_rule!", ruleoid)));
		
	}
			
	CatalogTupleDelete(pg_secmon_rule, &tup->t_self);
	
	ReleaseSysCache(tup);
	
	table_close(pg_secmon_rule, RowExclusiveLock);
		
	PG_RETURN_OID(ruleoid);
}

Datum
pg_create_secmon_threshold(PG_FUNCTION_ARGS)
{
	char	   *thrname;
	Name		name_thrname;
	oidvector  *oids;
	int32		interval;
	int32		maxtimes;
	char	   *basename;
	char	   *basevalue;
	TimestampTz starttime;
	TimestampTz	endtime;
	int2vector *columns;
	int32		level;
	bool		enabled;
	int			i;
	HeapTuple	tp;
	HeapTuple 	tuple;
	Oid			throid;
	Oid			ruleoid = 0;
	int32		nsteps;
	bool		nulls[Natts_pg_secmon_threshold];
	Datum		values[Natts_pg_secmon_threshold];
	Relation    pg_secmon_threshold;
	
	Oid			userid = GetUserId();

	/* permission check */
	check_audit_sao_permission(userid, false);

	if (PG_ARGISNULL(0) || 
		PG_ARGISNULL(1) ||
		PG_ARGISNULL(2) ||
		PG_ARGISNULL(3) ||
		PG_ARGISNULL(9) ||
		PG_ARGISNULL(10))
	{
		ereport(ERROR,
						(errmsg("thrname, secmonseq, interval, maxtimes, level and enabled cannot be null!")));
	}

	thrname = text_to_cstring(PG_GETARG_TEXT_PP(0));
	oids = (oidvector *) PG_GETARG_POINTER(1);
	interval = PG_GETARG_INT32(2);
	maxtimes = PG_GETARG_INT32(3);
	level = PG_GETARG_INT32(9);
	enabled = PG_GETARG_BOOL(10);

	if (interval <= 0)
	{
		ereport(ERROR,
					(errmsg("interval must be at least 1!")));
	}

	if (maxtimes <= 0)
	{
		ereport(ERROR,
					(errmsg("maxtimes must be at least 1!")));
	}

	if (level < 1 || level > 4)
	{
		ereport(ERROR,
					(errmsg("the values of level are 1, 2, 3, 4")));
	}

	if (level > 1)
	{
		if (audit_GUC.audit_mail == NULL || audit_GUC.audit_mail[0] == '\0')
			ereport(ERROR,
					(errmsg("when the level of the security monitoring threshold is greater than 1, audit alarm email needs to be configured")));
	}

	nsteps = oids->dim1;

	if (nsteps == 0 || nsteps > MAX_SECMON_SEQSIZE)
	{
		ereport(ERROR,
					(errmsg("number of OIDs in secmonseq must be in the range 1-%d!", MAX_SECMON_SEQSIZE)));
	}

	if (nsteps > 1 && maxtimes > 1)
	{
		ereport(ERROR,
					(errmsg("maxtimes for sequence threshold must be 1!")));
	}
	
	for (i = 0; i < nsteps; i++)
	{
		HeapTuple 	tup;
			
		ruleoid = oids->values[i];

		if (!OidIsValid(ruleoid))
			ereport(ERROR,
					(errmsg("invalid security monitoring rule with OID %u", ruleoid)));
		
		tup = SearchSysCache1(AUDITRULEID, ruleoid);
		if (!HeapTupleIsValid(tup))
		{
			ereport(ERROR,
							(errmsg("ruleoid %d does not exist in pg_secmon_rule!", ruleoid)));
			
		}

		ReleaseCatCache(tup);
	}
	
	tp = SearchSysCache3(AUDITTHRESHOLDRULEIDRULESEQ,
						 ObjectIdGetDatum(ruleoid),
						 PointerGetDatum(thrname),
						 PointerGetDatum(oids));

	if (HeapTupleIsValid(tp))
	{
		ReleaseCatCache(tp);
		ereport(ERROR,
						(errmsg("security monitoring threshold for thrname %s, ruleoid %d already exist in pg_secmon_threshold!", thrname, ruleoid)));
	}

	MemSet(values, 0, sizeof(values));
	MemSet(nulls, false, sizeof(nulls));

	if (PG_ARGISNULL(4) && PG_ARGISNULL(5))
	{
		nulls[Anum_pg_secmon_threshold_basename - 1] = true;
		nulls[Anum_pg_secmon_threshold_basevalue - 1] = true;
	}
	else
	{	
		
		Name	name_basename;
		Name	name_basevalue;
		
		if (PG_ARGISNULL(4) || PG_ARGISNULL(5))
		{
			ereport(ERROR,
							(errmsg("basename and basevalue must be both null or both not null!")));
		}

		basename = text_to_cstring(PG_GETARG_TEXT_PP(4));
		basevalue = text_to_cstring(PG_GETARG_TEXT_PP(5));

		/* Begin BUG#z6137 */
		check_base_line(basename, basevalue);
		/* End BUG#z6137 */

		if (nsteps > 1 || maxtimes > 1)
		{
			ereport(ERROR,
							(errmsg("basename and basevalue cant not work with sequence threshold or maxtimes threshold!")));
		}

		name_basename = (Name) palloc(NAMEDATALEN);
		name_basevalue = (Name) palloc(NAMEDATALEN);

		namestrcpy(name_basename, basename);
		namestrcpy(name_basevalue, basevalue);
		
		values[Anum_pg_secmon_threshold_basename - 1] = NameGetDatum(name_basename);
		values[Anum_pg_secmon_threshold_basevalue - 1] = NameGetDatum(name_basevalue);
	}

	if (PG_ARGISNULL(6) && PG_ARGISNULL(7))
	{
		values[Anum_pg_secmon_threshold_impt_starttime - 1] = TimestampTzGetDatum(DT_NOBEGIN);
		values[Anum_pg_secmon_threshold_impt_endtime - 1] = TimestampTzGetDatum(DT_NOEND);
	}
	else
	{
		if (PG_ARGISNULL(6) || PG_ARGISNULL(7))
		{
			ereport(ERROR,
							(errmsg("start time and end time of important moment must be both null or both not null!")));
		}
		
		starttime = PG_GETARG_TIMEADT(6);
		endtime	= PG_GETARG_TIMEADT(7);

		if (starttime > endtime)
		{
			ereport(ERROR,
							(errmsg("value of start time or end time invalid!")));
		}

		if (nsteps > 1 || maxtimes > 1)
		{
			ereport(ERROR,
							(errmsg("start time and end time of important momment cant not work with sequence threshold or maxtimes threshold!")));
		}

		values[Anum_pg_secmon_threshold_impt_starttime - 1] = TimestampTzGetDatum(starttime);
		values[Anum_pg_secmon_threshold_impt_endtime - 1] = TimestampTzGetDatum(endtime);
	}

	if (PG_ARGISNULL(8))
	{
		nulls[Anum_pg_secmon_threshold_impt_columns - 1] = true;
	}
	else
	{
		/* Begin BUG#z6137 */
		HeapTuple 	rule_tup;
		HeapTuple 	config_tup;
		Form_pg_secmon_rule rule_form;
		Oid			config_oid;
		Form_pg_audit_config config_form;
		char	   *objinfo;
		
		rule_tup = SearchSysCache1(AUDITRULEID, ruleoid);
		if (!HeapTupleIsValid(rule_tup))
		{
			ereport(ERROR,
							(errmsg("ruleoid %d does not exist in pg_secmon_rule!", ruleoid)));
			
		}

		rule_form = (Form_pg_secmon_rule) GETSTRUCT(rule_tup);

		config_oid = rule_form->confid;

		ReleaseCatCache(rule_tup);

		config_tup = SearchSysCache1(AUDITCONFIGID, config_oid);
		if (!HeapTupleIsValid(config_tup))
		{
			ereport(ERROR,
							(errmsg("confid %d does not exist in pg_audit_config!", config_oid)));
			
		}

		config_form = (Form_pg_audit_config) GETSTRUCT(config_tup);

		if (strcmp(NameStr(config_form->objtype), "column") == 0)
		{
			objinfo = pstrdup(NameStr(config_form->objoid));
		}
		else
		{
			objinfo = pstrdup("");
		}
		
		/* End BUG#z6137 */
		
		columns	= (int2vector *) PG_GETARG_POINTER(8);

		for (i = 0; i < columns->dim1; i++)
		{
			if (columns->values[i] <= FirstLowInvalidHeapAttributeNumber ||
				columns->values[i] == InvalidAttrNumber ||
				columns->values[i] > MaxHeapAttributeNumber)
			{
				ereport(ERROR,
							(errmsg("invalid column number %d", columns->values[i])));
			}

			/* Begin BUG#z6137 */
			if (strlen(objinfo) == 0)
				ereport(ERROR,
							(errmsg("the audit configuration is not a column audit")));
			if (strlen(objinfo) > 0 && (objinfo[strlen(objinfo) - 1] - '0') != columns->values[i])
				ereport(ERROR,
							(errmsg("invalid column number %d", columns->values[i])));
			/* End BUG#z6137 */
		}

		if (nsteps > 1 || maxtimes > 1)
		{
			ereport(ERROR,
							(errmsg("important columns cant not work with sequence threshold or maxtimes threshold!")));
		}

		/* Begin BUG#z6137 */
		ReleaseCatCache(config_tup);
		/* End BUG#z6137 */
		
		values[Anum_pg_secmon_threshold_impt_columns - 1] = PointerGetDatum(columns);
	}
	
	pg_secmon_threshold = table_open(AuditThresholdRelationId, RowExclusiveLock);

	name_thrname = (Name) palloc(NAMEDATALEN);

	namestrcpy(name_thrname, thrname);

	values[Anum_pg_secmon_threshold_thrname - 1] = NameGetDatum(name_thrname);
	values[Anum_pg_secmon_threshold_ruleoid - 1] = ObjectIdGetDatum(ruleoid);
	values[Anum_pg_secmon_threshold_interval - 1] = Int32GetDatum(interval);
	values[Anum_pg_secmon_threshold_starttime - 1] = TimestampTzGetDatum(GetCurrentTransactionStartTimestamp());
	values[Anum_pg_secmon_threshold_level - 1] = Int32GetDatum(level);
	values[Anum_pg_secmon_threshold_enable - 1] = BoolGetDatum(enabled);
	values[Anum_pg_secmon_threshold_maxtimes - 1] = Int32GetDatum(maxtimes);
	values[Anum_pg_secmon_threshold_nsteps - 1] = Int32GetDatum(nsteps);
	values[Anum_pg_secmon_threshold_ruleseq - 1] = PointerGetDatum(oids);
	
	throid = GetNewOidWithIndex(pg_secmon_threshold, AuditThresholdIndexId, Anum_pg_secmon_threshold_throid);
	values[Anum_pg_secmon_threshold_throid - 1] = ObjectIdGetDatum(throid);

	tuple = heap_form_tuple(RelationGetDescr(pg_secmon_threshold), values, nulls);

	CatalogTupleInsert(pg_secmon_threshold, tuple);

	heap_freetuple(tuple);

	table_close(pg_secmon_threshold, RowExclusiveLock);
	
	PG_RETURN_OID(throid);
}

/*
 * construct tuple and insert it into pg_secmon_rule
 */
static Oid
audit_insert_secmon_tuple(Relation pg_secmon_rule, Name rulename, Oid confid, AuditResult isSuccess, AuditChange enable_unable)
{
	Oid  		ruleoid;
	Datum		values[Natts_pg_secmon_rule];
	bool		nulls[Natts_pg_secmon_rule];
	HeapTuple	tup;
	
	memset(values, 0, sizeof(values));
	memset(nulls, false, sizeof(nulls));

	values[Anum_pg_secmon_rule_rulename - 1] = NameGetDatum(rulename);
	values[Anum_pg_secmon_rule_confid - 1] = ObjectIdGetDatum(confid);
	values[Anum_pg_secmon_rule_monmode - 1] = CharGetDatum(isSuccess);
	values[Anum_pg_secmon_rule_enable - 1] = CharGetDatum(enable_unable);

	ruleoid = GetNewOidWithIndex(pg_secmon_rule, AuditRuleIndexId, Anum_pg_secmon_rule_ruleoid);
	values[Anum_pg_secmon_rule_ruleoid - 1] = ObjectIdGetDatum(ruleoid);

	tup = heap_form_tuple(RelationGetDescr(pg_secmon_rule), values, nulls);

	CatalogTupleInsert(pg_secmon_rule, tup);

	heap_freetuple(tup);

	return ruleoid;

}

/*
 * Insert a tuple into table pg_secmon_rule
 */
static Oid 
add_secmon_rule(char *rule, Oid confid, AuditResult isSuccess, AuditChange enable_unable)
{
	Oid			ruleoid;
	Relation    pg_secmon_rule;
	HeapTuple tup;
	Name	name_rule;

	pg_secmon_rule = table_open(AuditRuleRelationId, RowExclusiveLock);

	/* search whether the tuple already exists 
	 * based on index AuditConfidRulenameIndexId
	 */
	tup = SearchSysCache2(AUDITCONFIDIDRULENAME, ObjectIdGetDatum(confid), CStringGetDatum(rule));
	
	if (HeapTupleIsValid(tup))
	{
		ReleaseCatCache(tup);
		ereport(ERROR,
						(errmsg("rule with name %s corresponding to confid %d already exist in pg_secmon_rule!", rule, confid)));
	}

	name_rule = (Name) palloc(NAMEDATALEN);

	namestrcpy(name_rule, rule);

	ruleoid = audit_insert_secmon_tuple(pg_secmon_rule, name_rule, confid, isSuccess, enable_unable);
	
	table_close(pg_secmon_rule, RowExclusiveLock);

	return ruleoid;
}

/*
 * omitting the timing of the audit indicates
 * that both successful and fail will be audited;
 * omitting the enable switch to indicate
 * that it is turned on
 */

static void
alter_secmon_rule_arg2(int args, Oid oid, char *rule, AuditResult isSuccess, AuditChange enable_unable)
{
	HeapTuple 	tup;
	Relation    pg_secmon_rule;
	HeapTuple 	newtuple;
	Name		name_rule;
	Datum 		values[Natts_pg_secmon_rule];
	bool 		nulls[Natts_pg_secmon_rule];
	bool 		replaces[Natts_pg_secmon_rule];
	
	tup = SearchSysCache1(AUDITRULEID, ObjectIdGetDatum(oid));
	if (!HeapTupleIsValid(tup))
	{
		ereport(ERROR,
						(errmsg("ruleoid %d does not exist in pg_secmon_rule!", oid)));
		
	}

	name_rule = (Name) palloc(NAMEDATALEN);

	namestrcpy(name_rule, rule);

	pg_secmon_rule = table_open(AuditRuleRelationId, RowExclusiveLock);
		
	MemSet(values, 0, sizeof(values));
	MemSet(nulls, false, sizeof(nulls));
	MemSet(replaces, false, sizeof(replaces));
			
	replaces[Anum_pg_secmon_rule_rulename - 1] = true;
	values[Anum_pg_secmon_rule_rulename - 1] = NameGetDatum(name_rule);

	if (args == 3)
	{
		replaces[Anum_pg_secmon_rule_monmode - 1] = true;
		values[Anum_pg_secmon_rule_monmode - 1] = CharGetDatum(isSuccess);
	}

	if (args == 4)
	{
		replaces[Anum_pg_secmon_rule_monmode - 1] = true;
		values[Anum_pg_secmon_rule_monmode - 1] = CharGetDatum(isSuccess); /* BUG#Z6076 */
		
		replaces[Anum_pg_secmon_rule_enable - 1] = true;
		values[Anum_pg_secmon_rule_enable - 1] = CharGetDatum(enable_unable);
	}

	newtuple = heap_modify_tuple(tup, RelationGetDescr(pg_secmon_rule), values,
					 nulls, replaces);
	CatalogTupleUpdate(pg_secmon_rule, &newtuple->t_self, newtuple);

	heap_freetuple(newtuple);

	ReleaseCatCache(tup);

	table_close(pg_secmon_rule, RowExclusiveLock);
	
	return;
}

/*
 * omitting the enable switch to indicate
 * that it is turned on
 */
static void
alter_secmon_rule_arg3(PG_FUNCTION_ARGS, Oid oid, char *rule)
{
	int			ret;
	char	   *mode;
	AuditResult result;

	mode = text_to_cstring(PG_GETARG_TEXT_PP(2));

	ret = audit_check_mode(mode, &result);
	if (ret != 0)
	{
		ereport(ERROR,
				(errmsg("input paramter '%s' format error!", mode)));
	}

	alter_secmon_rule_arg2(3, oid, rule, result, false);

	return;
	
}

static void
alter_secmon_rule_arg4(PG_FUNCTION_ARGS, Oid oid, char *rule)
{
	int			ret;
	char	   *mode;
	bool		enable;
	AuditResult result;

	mode = text_to_cstring(PG_GETARG_TEXT_PP(2));
	enable = PG_GETARG_BOOL(3);
	
	ret = audit_check_mode(mode, &result);
	if (ret != 0)
	{
		ereport(ERROR,
				(errmsg("input paramter '%s' format error!", mode)));
	}

	alter_secmon_rule_arg2(4, oid, rule, result, enable);

	return;
	
}

Datum
pg_alter_secmon_threshold(PG_FUNCTION_ARGS)
{
	Oid			throid;
	Relation	pg_secmon_threshold;
	HeapTuple	tp, tup, newtup;
	Form_pg_secmon_threshold thrform;
	bool		nulls[Natts_pg_secmon_threshold];
	Datum		values[Natts_pg_secmon_threshold];
	bool		replaces[Natts_pg_secmon_threshold];
	int			nsteps;
	int			maxtimes;
	Oid			userid = GetUserId();

	/* permission check */
	check_audit_sao_permission(userid, false);

	/* modify throid */
	if (PG_ARGISNULL(0))
		ereport(ERROR,
					(errmsg("throid cannot be null!")));

	throid = PG_GETARG_OID(0);

	if (!OidIsValid(throid))
		ereport(ERROR,
					(errmsg("invalid throid!")));

	pg_secmon_threshold = table_open(AuditThresholdRelationId, RowExclusiveLock);

	tup = SearchSysCache1(AUDITTHRESHOLDID, ObjectIdGetDatum(throid));
	if (!HeapTupleIsValid(tup))
		ereport(ERROR,
					(errmsg("throid %d does not exist in pg_secmon_threshold!", throid)));
	
	thrform = (Form_pg_secmon_threshold) GETSTRUCT(tup);	

	nsteps = thrform->nsteps;
	maxtimes = thrform->maxtimes;

	MemSet(values, 0, sizeof(values));
	MemSet(nulls, false, sizeof(nulls));
	MemSet(replaces, false, sizeof(replaces));

	/* modify thrname */
	if (!PG_ARGISNULL(1))
	{
		char   *thrname;
		Name	name_thrname;
		Oid		ruleoid;
		oidvector   *ruleseq;

		thrname = text_to_cstring(PG_GETARG_TEXT_PP(1));
		name_thrname = (Name) palloc(NAMEDATALEN);

		namestrcpy(name_thrname, thrname);

		ruleoid = thrform->ruleoid;
		ruleseq = &thrform->ruleseq;

		if (strncmp(thrname, NameStr(thrform->thrname), NAMEDATALEN) != 0)
		{	
			tp = SearchSysCache3(AUDITTHRESHOLDRULEIDRULESEQ,
						 ObjectIdGetDatum(ruleoid),
						 PointerGetDatum(thrname),
						 PointerGetDatum(ruleseq));

			if (HeapTupleIsValid(tp))
			{
				ReleaseCatCache(tp);
				ereport(ERROR,
								(errmsg("security monitoring threshold for thrname %s, ruleoid %d already exist in pg_secmon_threshold!", thrname, ruleoid)));
			}

			values[Anum_pg_secmon_threshold_thrname - 1] = NameGetDatum(name_thrname);
			replaces[Anum_pg_secmon_threshold_thrname - 1] = true;
		}
	}

	/* modify interval */
	if (!PG_ARGISNULL(2))
	{
		int	interval = PG_GETARG_INT32(2);

		if (interval <= 0)
		{
			ereport(ERROR,
					(errmsg("interval must be at least 1!")));
		}

		values[Anum_pg_secmon_threshold_interval - 1] = Int32GetDatum(interval);
		replaces[Anum_pg_secmon_threshold_interval - 1] = true;
	}

	/* modify maxtimes */
	if (!PG_ARGISNULL(3))
	{
		int	new_maxtimes = PG_GETARG_INT32(3);

		if (new_maxtimes <= 0)
		{
			ereport(ERROR,
					(errmsg("maxtimes must be at least 1!")));
		}

		if (nsteps > 1 && new_maxtimes > 1)
		{
			ereport(ERROR,
					(errmsg("maxtimes for sequence threshold must be 1!")));
		}

		values[Anum_pg_secmon_threshold_maxtimes - 1] = Int32GetDatum(new_maxtimes);
		replaces[Anum_pg_secmon_threshold_maxtimes - 1] = true;

		maxtimes = new_maxtimes;
	}

	/* modify basename and basevalue */
	if (!PG_ARGISNULL(4) || !PG_ARGISNULL(5))
	{
		char   *basename;
		char   *basevalue;
		Name	name_basename;
		Name	name_basevalue;

		if (PG_ARGISNULL(4) || PG_ARGISNULL(5))
		{
			ereport(ERROR,
					(errmsg("basename and basevalue must be updated together!")));
		}
		
		if (nsteps > 1 || maxtimes > 1)
		{
			ereport(ERROR,
							(errmsg("basename and basevalue cant not work with sequence threshold or maxtimes threshold!")));
		}
		
		basename = text_to_cstring(PG_GETARG_TEXT_PP(4));
		basevalue = text_to_cstring(PG_GETARG_TEXT_PP(5));

		/* Begin BUG#z6137 */
		check_base_line(basename, basevalue);
		/* End BUG#z6137 */

		name_basename = (Name) palloc(NAMEDATALEN);
		name_basevalue = (Name) palloc(NAMEDATALEN);

		namestrcpy(name_basename, basename);
		namestrcpy(name_basevalue, basevalue);
		
		values[Anum_pg_secmon_threshold_basename - 1] = NameGetDatum(name_basename);
		values[Anum_pg_secmon_threshold_basevalue - 1] = NameGetDatum(name_basevalue);

		replaces[Anum_pg_secmon_threshold_basename - 1] = true;
		replaces[Anum_pg_secmon_threshold_basevalue - 1] = true;
	}

	/* modify imp_startime and imp_endtime */
	if (!PG_ARGISNULL(6) || !PG_ARGISNULL(7))
	{
		TimestampTz starttime;
		TimestampTz	endtime;

		if (PG_ARGISNULL(6) || PG_ARGISNULL(7))
		{
			ereport(ERROR,
					(errmsg("start time and end time of important moment must be updated together!")));
		}

		starttime = PG_GETARG_TIMEADT(6);
		endtime	= PG_GETARG_TIMEADT(7);

		if (starttime > endtime)
		{
			ereport(ERROR,
					(errmsg("value of start time or end time invalid!")));
		}

		if (nsteps > 1 || maxtimes > 1)
		{
			ereport(ERROR,
							(errmsg("start time and end time of important moment cant not work with sequence threshold or maxtimes threshold!")));
		}

		values[Anum_pg_secmon_threshold_impt_starttime - 1] = TimestampTzGetDatum(starttime);
		values[Anum_pg_secmon_threshold_impt_endtime - 1] = TimestampTzGetDatum(endtime);
		replaces[Anum_pg_secmon_threshold_impt_starttime - 1] = true;
		replaces[Anum_pg_secmon_threshold_impt_endtime - 1] = true;
	}

	/* modify imp_columns */
	if (!PG_ARGISNULL(8))
	{
		int	i;
		int2vector *columns = (int2vector *) PG_GETARG_POINTER(8);

		/* Begin BUG#z6137 */
		HeapTuple 	rule_tup;
		HeapTuple 	config_tup;
		Form_pg_secmon_rule rule_form;
		Oid			config_oid;
		Form_pg_audit_config config_form;
		char	   *objinfo;
		Oid		ruleoid;

		ruleoid = thrform->ruleoid;
		
		rule_tup = SearchSysCache1(AUDITRULEID, ruleoid);
		if (!HeapTupleIsValid(rule_tup))
		{
			ereport(ERROR,
							(errmsg("ruleoid %d does not exist in pg_secmon_rule!", ruleoid)));
			
		}

		rule_form = (Form_pg_secmon_rule) GETSTRUCT(rule_tup);

		config_oid = rule_form->confid;

		ReleaseCatCache(rule_tup);

		config_tup = SearchSysCache1(AUDITCONFIGID, config_oid);
		if (!HeapTupleIsValid(config_tup))
		{
			ereport(ERROR,
							(errmsg("confid %d does not exist in pg_audit_config!", config_oid)));
			
		}

		config_form = (Form_pg_audit_config) GETSTRUCT(config_tup);

		if (strcmp(NameStr(config_form->objtype), "column") == 0)
		{
			objinfo = pstrdup(NameStr(config_form->objoid));
		}
		else
		{
			objinfo = pstrdup("");
		}
		
		/* End BUG#z6137 */

		for (i = 0; i < columns->dim1; i++)
		{
			if (columns->values[i] <= FirstLowInvalidHeapAttributeNumber ||
				columns->values[i] == InvalidAttrNumber ||
				columns->values[i] > MaxHeapAttributeNumber)
			{
				ereport(ERROR,
							(errmsg("invalid column number %d", columns->values[i])));
			}

			/* Begin BUG#z6137 */
			if (strlen(objinfo) == 0)
				ereport(ERROR,
							(errmsg("the audit configuration is not a column audit")));
			if (strlen(objinfo) > 0 && (objinfo[strlen(objinfo) - 1] - '0') != columns->values[i])
				ereport(ERROR,
							(errmsg("invalid column number %d", columns->values[i])));
			/* End BUG#z6137 */
		}

		if (nsteps > 1 || maxtimes > 1)
		{
			ereport(ERROR,
							(errmsg("important columns cant not work with sequence threshold or maxtimes threshold!")));
		}

		/* Begin BUG#z6137 */
		ReleaseCatCache(config_tup);
		/* End BUG#z6137 */
		
		values[Anum_pg_secmon_threshold_impt_columns - 1] = PointerGetDatum(columns);
		replaces[Anum_pg_secmon_threshold_impt_columns - 1] = true;
	}

	/* modify level */
	if (!PG_ARGISNULL(9))
	{
		int		level = PG_GETARG_INT32(9);

		if (level < 1 || level > 4)
		{
			ereport(ERROR,
						(errmsg("the values of level are 1, 2, 3, 4")));
		}
		
		if (level > 1)
		{
			if (audit_GUC.audit_mail == NULL || audit_GUC.audit_mail[0] == '\0')
				ereport(ERROR,
						(errmsg("when the level of the security monitoring threshold is greater than 1, audit alarm email needs to be configured")));
		}

		values[Anum_pg_secmon_threshold_level - 1] = Int32GetDatum(level);
		replaces[Anum_pg_secmon_threshold_level - 1] = true;
	}

	/* modify enabled */
	if (!PG_ARGISNULL(10))
	{
		bool	enabled = PG_GETARG_BOOL(10);

		values[Anum_pg_secmon_threshold_enable - 1] = BoolGetDatum(enabled);
		replaces[Anum_pg_secmon_threshold_enable - 1] = true;
	}

	/*Begin BUG#z6147 */
	/* modify starttime */
	//values[Anum_pg_secmon_threshold_starttime - 1] = TimestampTzGetDatum(GetCurrentTransactionStartTimestamp());
	//replaces[Anum_pg_secmon_threshold_starttime - 1] = true;
	/*End BUG#z6147 */

	newtup = heap_modify_tuple(tup, RelationGetDescr(pg_secmon_threshold), values, nulls, replaces);

	CatalogTupleUpdate(pg_secmon_threshold, &tup->t_self, newtup);
	
	heap_freetuple(newtup);

	table_close(pg_secmon_threshold, RowExclusiveLock);

	if (HeapTupleIsValid(tup))
		ReleaseCatCache(tup);
	
	
	PG_RETURN_OID(throid);
}

Datum
pg_drop_secmon_threshold(PG_FUNCTION_ARGS)
{	
	Oid			throid = PG_GETARG_OID(0);
	Relation	pg_secmon_threshold;
	HeapTuple	tup;
	Oid			userid = GetUserId();

	/* permission check */
	check_audit_sao_permission(userid, false);

	if (!OidIsValid(throid))
	{
		ereport(ERROR,
					(errmsg("invalid throid!")));
	}

	pg_secmon_threshold = table_open(AuditThresholdRelationId, RowExclusiveLock);

	tup = SearchSysCache1(AUDITTHRESHOLDID, ObjectIdGetDatum(throid));
	if (!HeapTupleIsValid(tup))
		ereport(ERROR,
					(errmsg("throid %d does not exist in pg_secmon_threshold!", throid)));
	
	CatalogTupleDelete(pg_secmon_threshold, &tup->t_self);
	
	ReleaseSysCache(tup);
	
	table_close(pg_secmon_threshold, RowExclusiveLock);
		
	PG_RETURN_OID(throid);
}

static void AuditShememInit(void)
{
	bool found_ctl_buf;

	if (prev_shmem_startup_hook)
		prev_shmem_startup_hook();

	audit_GUC.audit_pause = (bool *)
		ShmemInitStruct("Audit signal parmater",
						sizeof(bool), &found_ctl_buf);

	if (found_ctl_buf)
	{
		Assert(found_ctl_buf);
	}

	*audit_GUC.audit_pause = false;

	return ;
}

/* Begin - BUG#Z6092 */
Datum
show_audit_param(PG_FUNCTION_ARGS)
{	
	FuncCallContext *funcctx;
    int            call_cntr;
    int            max_calls;
    TupleDesc    tupdesc;
    AttInMetadata *attinmeta;
    MemoryContext oldcontext;
	Oid			userid = GetUserId();

	/* permission check */
	check_audit_sao_permission(userid, false);
	
    /* stuff done only on the first call of the function */
    if (SRF_IS_FIRSTCALL())
    {
        /* create a function context for cross-call persistence */
        funcctx = SRF_FIRSTCALL_INIT();

        /* switch to memory context appropriate for multiple function calls */
        oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

        /* build tupdesc for result tuples */
        tupdesc = CreateTemplateTupleDesc(2, false);
        TupleDescInitEntry(tupdesc, (AttrNumber) 1, "parameter",
                          TEXTOID, -1, 0);
        TupleDescInitEntry(tupdesc, (AttrNumber) 2, "value",
                          TEXTOID, -1, 0);

        attinmeta = TupleDescGetAttInMetadata(tupdesc);
        funcctx->attinmeta = attinmeta;
        funcctx->max_calls = NUM_AUDIT_PARAMS;

        MemoryContextSwitchTo(oldcontext);
    }

    /* stuff done on every call of the function */
    funcctx = SRF_PERCALL_SETUP();
    call_cntr = funcctx->call_cntr;
    max_calls = funcctx->max_calls;
    attinmeta = funcctx->attinmeta;

    if (call_cntr < max_calls)
    {
        char       *values[2];
        HeapTuple    tuple;
        Datum        result;
        char       *param_value;

        /* get the parameter name */
        values[0] = (char *) audit_parameters[call_cntr];

        /* get the parameter value */
        param_value = GetConfigOptionByName(values[0], NULL, false);
        values[1] = param_value ? pstrdup(param_value) : "NULL";

        /* build the tuple */
        tuple = BuildTupleFromCStrings(attinmeta, values);

        /* make the tuple into a datum */
        result = HeapTupleGetDatum(tuple);

        /* free memory if needed */
        if (param_value)
            pfree(param_value);

        SRF_RETURN_NEXT(funcctx, result);
    }
    else
    {
        SRF_RETURN_DONE(funcctx);
    }
}
/* End - BUG#Z6092 */

Datum
pg_audit_switch_primary(PG_FUNCTION_ARGS)
{

	if (MyProcPort && MyProcPort->application_name
		&& pg_strcasecmp(MyProcPort->application_name, "psql") == 0)
		PG_RETURN_BOOL(false);

#ifdef USE_SQLITE
	if (sqlite_open_db_for_audit_hook)
	{
		int num = PG_GETARG_INT32(0);

		if (num == 1)
			sqlite_open_db_for_audit_hook(SYSTEMAUDIT_SWITCHOVER, AUDIT_SUCCESS, MyProcPort->user_name, NULL);
		else if (num == 2)
			sqlite_open_db_for_audit_hook(SYSTEMAUDIT_FAILOVER, AUDIT_SUCCESS, MyProcPort->user_name, NULL);
	}
#endif

	PG_RETURN_BOOL(true);
}

#if defined(USE_SQLITE) && defined(ENABLE_DKY_BETA_FEATURE)
static bool check_disk_size(void)
{
	StartTransactionCommand();
	if(is_exceeded_disklimit(RELKIND_PARTITIONED_TABLE))
	{
		if (audit_GUC.logpolicy == AUDIT_PAUSE)
		{
			CommitTransactionCommand();
			return false;
		}
	}
	CommitTransactionCommand();

	return true;
}

static void audit_drop_table_partition(void)
{
	uint64		proc;
	int			ret;
	char	   *table = NULL;
	StringInfoData buf;
	TimestampTz time_cur;
	char *date_str = NULL;
	Oid save_userid;
	Oid syssao_oid;
	int save_sec_context;

	SetCurrentStatementStartTimestamp();
	StartTransactionCommand();
	PushActiveSnapshot(GetTransactionSnapshot());

	syssao_oid = get_role_oid("syssao", false);
	GetUserIdAndSecContext(&save_userid, &save_sec_context);
	SetUserIdAndSecContext(syssao_oid,
						save_sec_context | SECURITY_RESTRICTED_OPERATION);

	if (SPI_connect() != SPI_OK_CONNECT)
	{
		ereport(ERROR, errmsg("SPI_connect failed"));
	}

	internalStatement = true;

	initStringInfo(&buf);
	pgstat_report_activity(STATE_RUNNING, buf.data);
	appendStringInfo(&buf, "select relid::regclass as child_table from pg_partition_tree('hg_security.pg_audit_log');");
	ret = SPI_execute(buf.data, false, 0);

	if (ret != SPI_OK_SELECT)
	{
		internalStatement = false;
		SPI_finish();
		ereport(ERROR, errmsg("cannot select pg_database: error code %d", ret));
	}

	proc = SPI_processed;
	if (proc > 0)
	{
		SPITupleTable *spi_tuptable = SPI_tuptable;
		TupleDesc	spi_tupdesc = spi_tuptable->tupdesc;
		int64		i;
		char date[9];
		int32 datenum=0;
		int64 del_num = -1;
		HeapTuple	spi_tuple;

		for (i = 0; i < proc; i++)
		{
			spi_tuple = spi_tuptable->vals[i];
			table = SPI_getvalue(spi_tuple, spi_tupdesc, 1);

			if (pg_strcasecmp(table, PG_AUDIT_LOG) == 0)
				continue;

			memset(date, 0, sizeof(date));
			memcpy(date, table + strlen(PG_AUDIT_LOG) + 1, 9 - 1);

			/* choice oldest date table */
			if (del_num == -1 || datenum > atoi(date))
			{
				datenum = atoi(date);
				del_num = i;
			}
		}

		/* get current date */
		time_cur = GetCurrentTimestamp();
		date_str = text_to_cstring(DatumGetTextP(DirectFunctionCall2(timestamptz_to_char,
													TimestampTzGetDatum(time_cur),
													PointerGetDatum(cstring_to_text("YYYYMMDD")))));
		spi_tuple = spi_tuptable->vals[del_num];
		table = SPI_getvalue(spi_tuple, spi_tupdesc, 1);

		if (datenum != atoi(date_str))
		{
			if (del_num != -1 && pg_strcasecmp(table, PG_AUDIT_LOG) != 0)
			{
				initStringInfo(&buf);
				appendStringInfo(&buf, "SELECT relowner FROM pg_class WHERE relname='%s' and relnamespace=%d;", table, SECURITY_SCHEMA_NAMESPACE);
				ret = SPI_execute(buf.data, false, 0);

				if (ret != SPI_OK_SELECT)
				{
					internalStatement = false;
					SPI_finish();
					ereport(ERROR, errmsg("cannot select pg_database: error code %d", ret));
				}

				proc = SPI_processed;
				if (proc == 1)
				{
					SPITupleTable *spi_tuptable = SPI_tuptable;
					TupleDesc	spi_tupdesc = spi_tuptable->tupdesc;
					Oid  owner_oid;

					owner_oid = atooid(SPI_getvalue(spi_tuptable->vals[0], spi_tupdesc, 1));

					if (!IS_SYSSAO(owner_oid))
					{
						char tablename[1024] = {'\0'};

						internalStatement = false;
						strncpy(tablename, table, 1024);
						SPI_finish();
						ereport(ERROR,
								 errmsg("permission denied:must be owner of table %s", tablename));
					}
				}

				initStringInfo(&buf);
				appendStringInfo(&buf, "DROP TABLE IF EXISTS hg_security.%s;", table);

				if (buf.len > 0)
				{
					bool old_mode = se_feature_mode;
					se_feature_mode = false;

					PG_TRY();
					{
						ret = SPI_execute(buf.data, false, 0);
					}
					PG_FINALLY();
					{
						se_feature_mode = old_mode;
					}
					PG_END_TRY();
				}
			}
		}
		else
			ereport(INFO, errmsg("Forbidden drop table %s, this is current partition table ", table));
	}

	if (SPI_finish() != SPI_OK_FINISH)
	{
		internalStatement = false;
		ereport(ERROR, errmsg("SPI_finish failed"));
	}
	/* Restore userid and security context */
	SetUserIdAndSecContext(save_userid, save_sec_context);

	PopActiveSnapshot();
	CommitTransactionCommand();
	internalStatement = false;

	return;
}

static bool table_count_changes(void)
{
	StringInfoData buf;
	uint64		proc;
	int			ret;
	int table_num = 0;
	static int pre_table_num = 0;
	bool flag = false;

	if (SPI_connect() != SPI_OK_CONNECT)
	{
		elog(ERROR, "SPI_connect failed");
	}

	initStringInfo(&buf);
	appendStringInfo(&buf, "select count(relid::regclass) as child_num from pg_partition_tree('hg_security.pg_audit_log');");
	ret = SPI_execute(buf.data, false, 0);

	if (ret != SPI_OK_SELECT)
	{
		SPI_finish();
		ereport(ERROR, errmsg("cannot select pg_database: error code %d", ret));
	}

	proc = SPI_processed;
	if (proc == 1)
	{
		SPITupleTable *spi_tuptable = SPI_tuptable;
		TupleDesc	spi_tupdesc = spi_tuptable->tupdesc;
		table_num = atoi(SPI_getvalue(spi_tuptable->vals[0], spi_tupdesc, 1));

		if (pre_table_num == 0)
			pre_table_num = table_num;

		if (pre_table_num != table_num)
		{
			flag = true;
			pre_table_num = table_num;
		}
		else
			flag = false;
	}

	if (SPI_finish() != SPI_OK_FINISH)
	{
		elog(ERROR, "SPI_finish failed");
	}

	return flag;
}

static bool is_table_exits(TimestampTz time_cur)
{
	bool is_exist = false;
	int			ret;
	char *date_cur1 = NULL;
	StringInfoData buf;
	uint64		proc;

	if (SPI_connect() != SPI_OK_CONNECT)
	{
		elog(ERROR, "SPI_connect failed");
	}

	/* current day */
	date_cur1 = text_to_cstring(DatumGetTextP(DirectFunctionCall2(timestamptz_to_char,
											TimestampTzGetDatum(time_cur),
											PointerGetDatum(cstring_to_text("YYYYMMDD")))));

	initStringInfo(&buf);
	appendStringInfo(&buf, "SELECT EXISTS (SELECT FROM pg_tables where schemaname='hg_security' and tablename='pg_audit_log_%s');", date_cur1);
	ret = SPI_execute(buf.data, false, 0);

	if (ret != SPI_OK_SELECT)
	{
		SPI_finish();
		ereport(ERROR, errmsg("cannot select pg_database: error code %d", ret));
	}

	proc = SPI_processed;
	if (proc == 1)
	{
		SPITupleTable *spi_tuptable = SPI_tuptable;
		TupleDesc	spi_tupdesc = spi_tuptable->tupdesc;
		char *c = SPI_getvalue(spi_tuptable->vals[0], spi_tupdesc, 1);

		if (c[0] == 't')
			is_exist = true;
	}

	if (SPI_finish() != SPI_OK_FINISH)
	{
		elog(ERROR, "SPI_finish failed");
	}

	return is_exist;
}

static void audit_create_table_partition(TimestampTz time_cur)
{
	char *date_next_str1 = NULL;
	char *date_cur1 = NULL;
	TimestampTz time_next;
	StringInfoData buf;

	if (RecoveryInProgress())
		return;

	internalStatement = true;

	StartTransactionCommand();
	PushActiveSnapshot(GetTransactionSnapshot());

	if (is_table_exits(time_cur))
	{
		internalStatement = false;
		PopActiveSnapshot();
		CommitTransactionCommand();
		return;
	}

	time_next = time_cur + USECS_PER_DAY;

	/* current day */
	date_cur1 = text_to_cstring(DatumGetTextP(DirectFunctionCall2(timestamptz_to_char,
											TimestampTzGetDatum(time_cur),
											PointerGetDatum(cstring_to_text("YYYYMMDD")))));

	date_next_str1 = text_to_cstring(DatumGetTextP(DirectFunctionCall2(timestamptz_to_char,
											TimestampTzGetDatum(time_next),
											PointerGetDatum(cstring_to_text("YYYYMMDD")))));

	initStringInfo(&buf);

	if (SPI_connect() != SPI_OK_CONNECT)
	{
		PopActiveSnapshot();
		CommitTransactionCommand();
		elog(ERROR, "SPI_connect failed");
	}
	appendStringInfo(&buf, "CREATE TABLE IF NOT EXISTS hg_security.pg_audit_log_%s PARTITION OF hg_security.pg_audit_log (audittime) FOR VALUES FROM ('%s') TO ('%s');"
						"ALTER TABLE hg_security.pg_audit_log_%s OWNER TO syssao;"
						"GRANT SELECT ON hg_security.pg_audit_log_%s TO syssso;"
						"GRANT SELECT ON hg_security.pg_audit_log_%s to syssao_role;"
						"GRANT SELECT ON hg_security.pg_audit_log_%s to syssso_role;",
						date_cur1, date_cur1, date_next_str1, date_cur1,
						date_cur1, date_cur1, date_cur1);

	if (buf.len > 0)
	{
		bool old_mode = se_feature_mode;
		se_feature_mode = false;
		PG_TRY();
		{
			SPI_execute(buf.data, false, 0);
		}
		PG_FINALLY();
		{
			se_feature_mode = old_mode;
		}
		PG_END_TRY();
	}

	if (SPI_finish() != SPI_OK_FINISH)
	{
		internalStatement = false;
		PopActiveSnapshot();
		CommitTransactionCommand();
		elog(ERROR, "SPI_finish failed");
	}

	internalStatement = false;

	PopActiveSnapshot();
	CommitTransactionCommand();
}

static List *
audit_get_database_list(void)
{
	List	*res = NIL;
	Relation	rel;
	TableScanDesc scan;
	HeapTuple	tup;
	MemoryContext resultcxt;

	/* This is the context that we will allocate our output data in */
	resultcxt = CurrentMemoryContext;

	/*
	 * Start a transaction so we can access pg_database, and get a snapshot.
	 * We don't have a use for the snapshot itself, but we're interested in
	 * the secondary effect that it sets RecentGlobalXmin.  (This is critical
	 * for anything that reads heap pages, because HOT may decide to prune
	 * them even if the process doesn't attempt to modify any tuples.)
	 *
	 * FIXME: This comment is inaccurate / the code buggy. A snapshot that is
	 * not pushed/active does not reliably prevent HOT pruning (->xmin could
	 * e.g. be cleared when cache invalidations are processed).
	 */
	//StartTransactionCommand();
	(void) GetTransactionSnapshot();

	rel = table_open(DatabaseRelationId, AccessShareLock);
	scan = table_beginscan_catalog(rel, 0, NULL);

	while (HeapTupleIsValid(tup = heap_getnext(scan, ForwardScanDirection)))
	{
		Form_pg_database subform = (Form_pg_database) GETSTRUCT(tup);
		dbinfo *sub;
		MemoryContext oldcxt;

		/*
		 * Allocate our results in the caller's context, not the
		 * transaction's. We do this inside the loop, and restore the original
		 * context at the end, so that leaky things like heap_getnext() are
		 * not called in a potentially long-lived context.
		 */
		oldcxt = MemoryContextSwitchTo(resultcxt);

		sub = (dbinfo *) palloc0(sizeof(dbinfo));
		sub->dbid = subform->oid;
		sub->datistemplate = subform->datistemplate;
		sub->datallowconn = subform->datallowconn;
		/* We don't fill fields we are not interested in. */

		res = lappend(res, sub);
		MemoryContextSwitchTo(oldcxt);
	}

	table_endscan(scan);
	table_close(rel, AccessShareLock);

	return res;
}


static void
audit_register_db_work(Oid dbid, AuditTable flag)
{
	BackgroundWorker worker;
	BackgroundWorkerHandle *bgw_handle;

	if (flag == AUDIT_CREATE)
	{
		memset(&worker, 0, sizeof(worker));
		worker.bgw_flags = BGWORKER_SHMEM_ACCESS | BGWORKER_BACKEND_DATABASE_CONNECTION;
		worker.bgw_start_time = BgWorkerStart_RecoveryFinished;
		/* BGW_NEVER_RESTART connect once */
		worker.bgw_restart_time = BGW_NEVER_RESTART;
		snprintf(worker.bgw_library_name, BGW_MAXLEN, "pg_audit");
		snprintf(worker.bgw_function_name, BGW_MAXLEN, "pg_audit_launch_create_table");
		worker.bgw_notify_pid = 0;
		snprintf(worker.bgw_type, BGW_MAXLEN, "pg_audit_launch_create_table");
		worker.bgw_main_arg = Int32GetDatum(dbid);
	}
	else if (flag == AUDIT_DROP)
	{
		memset(&worker, 0, sizeof(worker));
		worker.bgw_flags = BGWORKER_SHMEM_ACCESS | BGWORKER_BACKEND_DATABASE_CONNECTION;
		worker.bgw_start_time = BgWorkerStart_RecoveryFinished;
		/* BGW_NEVER_RESTART connect once */
		worker.bgw_restart_time = BGW_NEVER_RESTART;
		snprintf(worker.bgw_library_name, BGW_MAXLEN, "pg_audit");
		snprintf(worker.bgw_function_name, BGW_MAXLEN, "pg_audit_launch_drop_table");
		worker.bgw_notify_pid = 0;
		snprintf(worker.bgw_type, BGW_MAXLEN, "pg_audit_launch_drop_table");
		worker.bgw_main_arg = Int32GetDatum(dbid);
	}

	/*
	 * Now fill in worker-specific data, and do the actual registrations.
	 */
	snprintf(worker.bgw_name, BGW_MAXLEN, AUDIT_LAUNCHER_DB_S, dbid);
	if (!RegisterDynamicBackgroundWorker(&worker, &bgw_handle))
	{
		ereport(WARNING,
				(errcode(ERRCODE_CONFIGURATION_LIMIT_EXCEEDED),
				 errmsg("out of background worker slots"),
				 errhint("You might need to increase max_worker_processes.")));
		return;
	}
}

static bool is_new_day(void)
{
	TimestampTz time_cur = 0;
	static int day_pre = 0;
	int day = 0;
	char *date_cur1 = NULL;

	time_cur = GetCurrentTimestamp();

	/* current day */
	date_cur1 = text_to_cstring(DatumGetTextP(DirectFunctionCall2(timestamptz_to_char,
											TimestampTzGetDatum(time_cur),
											PointerGetDatum(cstring_to_text("YYYYMMDD")))));
	day = atoi(date_cur1);

	if (day == day_pre)
	{
		return false;
	}

	day_pre = day;
	return true;
}

static bool is_db_changes(List      *sublist)
{
	int i = 0;
	int eq_num = 0;
	static int length = 0;
	static Oid *db_oid = NULL;
	MemoryContext oldctx;
	ListCell   *lc;

	if (length == list_length(sublist))
	{
		if (length == 0 || db_oid == NULL)
			return false;

		/* check the database change or not */
		foreach(lc, sublist)
		{
			dbinfo *sub = (dbinfo *) lfirst(lc);
			for (i = 0; i < length; i++)
			{
				if (db_oid[i] == sub->dbid)
				{
					eq_num++;
					continue;
				}
			}
		}

		if (eq_num != length)
		{
			i = 0;
			/* save new db oid */
			foreach(lc, sublist)
			{
				dbinfo *sub = (dbinfo *) lfirst(lc);
				db_oid[i++] = sub->dbid;
			}
		}
		/* db oid not change */
		else
			return false;
	}
	/* save db oid */
	else
	{
		if (length != list_length(sublist))
		{
			if (db_oid != NULL)
				pfree(db_oid);
			db_oid = NULL;
		}

		oldctx = MemoryContextSwitchTo(pgAuditLocalContext);
		db_oid = palloc0(sizeof(Oid) * list_length(sublist));
		MemoryContextSwitchTo(oldctx);

		i = 0;
		foreach(lc, sublist)
		{
			dbinfo *sub = (dbinfo *) lfirst(lc);
			db_oid[i++] = sub->dbid;
		}
		length = list_length(sublist);
	}

	return true;
}

static void
audit_register_all_db_work(AuditTable flag)
{
	List	   *sublist;
	ListCell   *lc;
	MemoryContext subctx;
	MemoryContext oldctx;

	TimestampTz time_cur;
	static TimestampTz pre_time = 0;
	int times = 0;

	if (RecoveryInProgress())
		return;

	/* get current date */
	time_cur = GetCurrentTimestamp();

	if (flag == AUDIT_DROP)
	{
		if (pre_time != 0 && pre_time + USECS_PER_SEC > time_cur)
		{
			return;
		}
		pre_time = time_cur;
	}

	subctx = AllocSetContextCreate(TopMemoryContext,
								   "all database sublist",
								   ALLOCSET_DEFAULT_SIZES);
	oldctx = MemoryContextSwitchTo(subctx);

	sublist = audit_get_database_list();

	if ((flag == AUDIT_CREATE && (is_db_changes(sublist) || is_new_day()))
		|| (flag == AUDIT_DROP && !table_count_changes()))
	{
		foreach(lc, sublist)
		{
			dbinfo *sub = (dbinfo *) lfirst(lc);
			if (sub->datallowconn && !sub->datistemplate)
			{
				/* Begin - BUG#Z6360 */
				if (!find_is_exist_create_table_proc(sub->dbid))
				/* End - BUG#Z6360 */
					audit_register_db_work(sub->dbid, flag);
			}
		}
	}

	if (flag == AUDIT_CREATE)
	{
		do
		{
			if (is_table_exits(time_cur)
				&& is_table_exits(time_cur + USECS_PER_DAY)) /* BUG#Z6360 */
			{
				break;
			}

			pg_usleep(5000);
			times++;

		} while(times < CHECK_LIMIT_TIMES); /* BUG#Z6360 */
	}

	/* Switch back to original memory context. */
	MemoryContextSwitchTo(oldctx);
	/* Clean the temporary memory. */
	MemoryContextDelete(subctx);

	return;
}

static void
audit_die(SIGNAL_ARGS)
{
	exit(1);
}

void pg_audit_launch_create_table(Datum main_arg)
{
	Oid dbid = DatumGetInt32(main_arg);
	/* Establish signal handlers before unblocking signals. */
	pqsignal(SIGTERM, audit_die);

	BackgroundWorkerUnblockSignals();

	/* Connect to our database. */
	BackgroundWorkerInitializeConnectionByOid(dbid, InvalidOid, 0);

	/* create partition table */
	audit_create_table_partition(GetCurrentTimestamp());
	audit_create_table_partition(GetCurrentTimestamp() + USECS_PER_DAY);

	exit(0);
}

void pg_audit_launch_drop_table(Datum main_arg)
{
	Oid dbid = DatumGetInt32(main_arg);

	/* Establish signal handlers before unblocking signals. */
	pqsignal(SIGTERM, audit_die);

	BackgroundWorkerUnblockSignals();

	/* Connect to our database. */
	BackgroundWorkerInitializeConnectionByOid(dbid, InvalidOid, 0);

	/* drop partiton table */
	audit_drop_table_partition();

	exit(0);
}

/* Begin - BUG#Z6360 */
static bool
find_is_exist_create_table_proc(Oid dbid)
{
	slist_mutable_iter iter;
	char audit_pro_name[BGW_MAXLEN] = {'\0'};

	memset(audit_pro_name, '\0', BGW_MAXLEN);
	snprintf(audit_pro_name, BGW_MAXLEN, AUDIT_LAUNCHER_DB_S, dbid);

	slist_foreach_modify(iter, &BackgroundWorkerList)
	{
		RegisteredBgWorker *rw;

		rw = slist_container(RegisteredBgWorker, rw_lnode, iter.cur);

		if (rw->rw_pid != getpid()
			&& strncmp(rw->rw_worker.bgw_name, audit_pro_name, strlen(audit_pro_name) + 1) == 0)
		{
			return true;
		}
	}

	return false;
}
/* End - BUG#Z6360 */
#endif
