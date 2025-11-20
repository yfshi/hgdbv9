/*--------------------------------------------------------------------
 * guc.h
 *
 * External declarations pertaining to backend/utils/misc/guc.c and
 * backend/utils/misc/guc-file.l
 *
 * Copyright (c) 2000-2021, PostgreSQL Global Development Group
 * Written by Peter Eisentraut <peter_e@gmx.net>.
 *
 * src/include/utils/guc.h
 *--------------------------------------------------------------------
 */
#ifndef GUC_H
#define GUC_H

#include "nodes/parsenodes.h"
#include "tcop/dest.h"
#include "utils/array.h"


/* upper limit for GUC variables measured in kilobytes of memory */
/* note that various places assume the byte size fits in a "long" variable */
#if SIZEOF_SIZE_T > 4 && SIZEOF_LONG > 4
#define MAX_KILOBYTES	INT_MAX
#else
#define MAX_KILOBYTES	(INT_MAX / 1024)
#endif

/*
 * Automatic configuration file name for ALTER SYSTEM.
 * This file will be used to store values of configuration parameters
 * set by ALTER SYSTEM command.
 */
#define PG_AUTOCONF_FILENAME		"postgresql.auto.conf"

/*
 * Certain options can only be set at certain times. The rules are
 * like this:
 *
 * INTERNAL options cannot be set by the user at all, but only through
 * internal processes ("server_version" is an example).  These are GUC
 * variables only so they can be shown by SHOW, etc.
 *
 * POSTMASTER options can only be set when the postmaster starts,
 * either from the configuration file or the command line.
 *
 * SIGHUP options can only be set at postmaster startup or by changing
 * the configuration file and sending the HUP signal to the postmaster
 * or a backend process. (Notice that the signal receipt will not be
 * evaluated immediately. The postmaster and the backend check it at a
 * certain point in their main loop. It's safer to wait than to read a
 * file asynchronously.)
 *
 * BACKEND and SU_BACKEND options can only be set at postmaster startup,
 * from the configuration file, or by client request in the connection
 * startup packet (e.g., from libpq's PGOPTIONS variable).  SU_BACKEND
 * options can be set from the startup packet only when the user is a
 * superuser.  Furthermore, an already-started backend will ignore changes
 * to such an option in the configuration file.  The idea is that these
 * options are fixed for a given backend once it's started, but they can
 * vary across backends.
 *
 * SUSET options can be set at postmaster startup, with the SIGHUP
 * mechanism, or from the startup packet or SQL if you're a superuser.
 *
 * USERSET options can be set by anyone any time.
 */
typedef enum
{
	PGC_INTERNAL,
	PGC_POSTMASTER,
	PGC_SIGHUP,
	PGC_SU_BACKEND,
	PGC_BACKEND,
	PGC_SUSET,
	PGC_USERSET
} GucContext;

/*
 * The following type records the source of the current setting.  A
 * new setting can only take effect if the previous setting had the
 * same or lower level.  (E.g, changing the config file doesn't
 * override the postmaster command line.)  Tracking the source allows us
 * to process sources in any convenient order without affecting results.
 * Sources <= PGC_S_OVERRIDE will set the default used by RESET, as well
 * as the current value.  Note that source == PGC_S_OVERRIDE should be
 * used when setting a PGC_INTERNAL option.
 *
 * PGC_S_INTERACTIVE isn't actually a source value, but is the
 * dividing line between "interactive" and "non-interactive" sources for
 * error reporting purposes.
 *
 * PGC_S_TEST is used when testing values to be used later.  For example,
 * ALTER DATABASE/ROLE tests proposed per-database or per-user defaults this
 * way, and CREATE FUNCTION tests proposed function SET clauses this way.
 * This is an interactive case, but it needs its own source value because
 * some assign hooks need to make different validity checks in this case.
 * In particular, references to nonexistent database objects generally
 * shouldn't throw hard errors in this case, at most NOTICEs, since the
 * objects might exist by the time the setting is used for real.
 *
 * NB: see GucSource_Names in guc.c if you change this.
 */
typedef enum
{
	PGC_S_DEFAULT,				/* hard-wired default ("boot_val") */
	PGC_S_DYNAMIC_DEFAULT,		/* default computed during initialization */
	PGC_S_ENV_VAR,				/* postmaster environment variable */
	PGC_S_FILE,					/* postgresql.conf */
	PGC_S_ARGV,					/* postmaster command line */
	PGC_S_GLOBAL,				/* global in-database setting */
	PGC_S_DATABASE,				/* per-database setting */
	PGC_S_USER,					/* per-user setting */
	PGC_S_DATABASE_USER,		/* per-user-and-database setting */
	PGC_S_CLIENT,				/* from client connection request */
	PGC_S_OVERRIDE,				/* special case to forcibly set default */
	PGC_S_INTERACTIVE,			/* dividing line for error reporting */
	PGC_S_TEST,					/* test per-database or per-user setting */
	PGC_S_SESSION				/* SET command */
} GucSource;

/*
 * Parsing the configuration file(s) will return a list of name-value pairs
 * with source location info.  We also abuse this data structure to carry
 * error reports about the config files.  An entry reporting an error will
 * have errmsg != NULL, and might have NULLs for name, value, and/or filename.
 *
 * If "ignore" is true, don't attempt to apply the item (it might be an error
 * report, or an item we determined to be duplicate).  "applied" is set true
 * if we successfully applied, or could have applied, the setting.
 */
typedef struct ConfigVariable
{
	char	   *name;
	char	   *value;
	char	   *errmsg;
	char	   *filename;
	int			sourceline;
	bool		ignore;
	bool		applied;
	struct ConfigVariable *next;
} ConfigVariable;

extern bool ParseConfigFile(const char *config_file, bool strict,
							const char *calling_file, int calling_lineno,
							int depth, int elevel,
							ConfigVariable **head_p, ConfigVariable **tail_p);
extern bool ParseConfigFp(FILE *fp, const char *config_file,
						  int depth, int elevel,
						  ConfigVariable **head_p, ConfigVariable **tail_p);
extern bool ParseConfigDirectory(const char *includedir,
								 const char *calling_file, int calling_lineno,
								 int depth, int elevel,
								 ConfigVariable **head_p,
								 ConfigVariable **tail_p);
extern void FreeConfigVariables(ConfigVariable *list);
extern char *DeescapeQuotedString(const char *s);

/*
 * The possible values of an enum variable are specified by an array of
 * name-value pairs.  The "hidden" flag means the value is accepted but
 * won't be displayed when guc.c is asked for a list of acceptable values.
 */
struct config_enum_entry
{
	const char *name;
	int			val;
	bool		hidden;
};

/*
 * Signatures for per-variable check/assign/show hook functions
 */
typedef bool (*GucBoolCheckHook) (bool *newval, void **extra, GucSource source);
typedef bool (*GucIntCheckHook) (int *newval, void **extra, GucSource source);
typedef bool (*GucRealCheckHook) (double *newval, void **extra, GucSource source);
typedef bool (*GucStringCheckHook) (char **newval, void **extra, GucSource source);
typedef bool (*GucEnumCheckHook) (int *newval, void **extra, GucSource source);

typedef void (*GucBoolAssignHook) (bool newval, void *extra);
typedef void (*GucIntAssignHook) (int newval, void *extra);
typedef void (*GucRealAssignHook) (double newval, void *extra);
typedef void (*GucStringAssignHook) (const char *newval, void *extra);
typedef void (*GucEnumAssignHook) (int newval, void *extra);

typedef const char *(*GucShowHook) (void);

/*
 * Miscellaneous
 */
typedef enum
{
	/* Types of set_config_option actions */
	GUC_ACTION_SET,				/* regular SET command */
	GUC_ACTION_LOCAL,			/* SET LOCAL command */
	GUC_ACTION_SAVE				/* function SET option, or temp assignment */
} GucAction;

#define GUC_QUALIFIER_SEPARATOR '.'

/*
 * bit values in "flags" of a GUC variable
 */
#define GUC_LIST_INPUT			0x0001	/* input can be list format */
#define GUC_LIST_QUOTE			0x0002	/* double-quote list elements */
#define GUC_NO_SHOW_ALL			0x0004	/* exclude from SHOW ALL */
#define GUC_NO_RESET_ALL		0x0008	/* exclude from RESET ALL */
#define GUC_REPORT				0x0010	/* auto-report changes to client */
#define GUC_NOT_IN_SAMPLE		0x0020	/* not in postgresql.conf.sample */
#define GUC_DISALLOW_IN_FILE	0x0040	/* can't set in postgresql.conf */
#define GUC_CUSTOM_PLACEHOLDER	0x0080	/* placeholder for custom variable */
#define GUC_SUPERUSER_ONLY		0x0100	/* show only to superusers */
#define GUC_IS_NAME				0x0200	/* limit string to NAMEDATALEN-1 */
#define GUC_NOT_WHILE_SEC_REST	0x0400	/* can't set if security restricted */
#define GUC_DISALLOW_IN_AUTO_FILE 0x0800	/* can't set in
											 * PG_AUTOCONF_FILENAME */

#define GUC_UNIT_KB				0x1000	/* value is in kilobytes */
#define GUC_UNIT_BLOCKS			0x2000	/* value is in blocks */
#define GUC_UNIT_XBLOCKS		0x3000	/* value is in xlog blocks */
#define GUC_UNIT_MB				0x4000	/* value is in megabytes */
#define GUC_UNIT_BYTE			0x8000	/* value is in bytes */
#define GUC_UNIT_MEMORY			0xF000	/* mask for size-related units */

#define GUC_UNIT_MS			   0x10000	/* value is in milliseconds */
#define GUC_UNIT_S			   0x20000	/* value is in seconds */
#define GUC_UNIT_MIN		   0x30000	/* value is in minutes */
#define GUC_UNIT_TIME		   0xF0000	/* mask for time-related units */

#define GUC_EXPLAIN			  0x100000	/* include in explain */
#define GUC_ALLOW_IN_PARALLEL 0x200000	/* allow setting in parallel mode */

#define GUC_UNIT				(GUC_UNIT_MEMORY | GUC_UNIT_TIME)


/* GUC vars that are actually declared in guc.c, rather than elsewhere */
extern bool Debug_print_plan;
extern bool Debug_print_parse;
extern bool Debug_print_rewritten;
extern bool Debug_pretty_print;

extern bool log_parser_stats;
extern bool log_planner_stats;
extern bool log_executor_stats;
extern bool log_statement_stats;
extern bool log_btree_build_stats;

extern PGDLLIMPORT bool check_function_bodies;
extern bool session_auth_is_superuser;

extern bool log_duration;
extern int	log_parameter_max_length;
extern int	log_parameter_max_length_on_error;
extern int	log_min_error_statement;
extern PGDLLIMPORT int log_min_messages;
extern PGDLLIMPORT int client_min_messages;
extern int	log_min_duration_sample;
extern int	log_min_duration_statement;
extern int	log_temp_files;
extern double log_statement_sample_rate;
extern double log_xact_sample_rate;
extern char *backtrace_functions;
extern char *backtrace_symbol_list;

extern int	temp_file_limit;

extern int	num_temp_buffers;

extern char *cluster_name;
extern PGDLLIMPORT char *ConfigFileName;
extern char *HbaFileName;
extern char *IdentFileName;
extern char *external_pid_file;

extern PGDLLIMPORT char *application_name;
extern PGDLLIMPORT char *osname; /* ReqID: SRS-SESSION-VIEWS */


extern int	tcp_keepalives_idle;
extern int	tcp_keepalives_interval;
extern int	tcp_keepalives_count;
extern int	tcp_user_timeout;

#ifdef TRACE_SORT
extern PGDLLIMPORT bool trace_sort;	/* SRS-MISC-COMPATDM-SQL,win32 compile needs this to be declared as PGDLLIMPORT */
#endif

/* The following are the guc variables of ivorysql */
/* BEGIN - case sensitive indentify */
extern PGDLLIMPORT int identifier_case_switch;
extern PGDLLIMPORT bool identifier_case_from_pg_dump;
extern PGDLLIMPORT bool enable_case_switch;
/* END - case sensitive indentify */

/* BEGIN - SQL PARSER */
extern PGDLLIMPORT int  database_mode;	/* SRS-MISC-COMPATDM-SQL */
extern PGDLLIMPORT int  compatible_mode;
/* END - SQL PARSER */

/* BEGIN - ReqID:SRS-MISC-USER */
extern PGDLLIMPORT int  dumppgtype;
/* END - ReqID:SRS-MISC-USER */
/* Begin - BUG#z476 */
extern PGDLLIMPORT bool numerictonumber;
/* End - BUG#z476 */

/* Begin - ReqID:EMPTY_STIRNG_TO_NULL */
extern PGDLLIMPORT bool	enable_emptystring_to_NULL;
/* End - ReqID:EMPTY_STIRNG_TO_NULL */

/* Begin - BUG#6021 */
extern PGDLLIMPORT bool	enable_emptystring_is_NULL;
/* End - BUG#6021 */
/* Begin - BUG:6022 */
extern PGDLLIMPORT bool enable_standard_datetime_parsing_mode;
/* End - BUG:6022 */

/* Begin - BUG:M0000068 */
extern PGDLLIMPORT bool convert_plisql_int_to_number;
extern PGDLLIMPORT bool convert_plisql_anonymous_int_to_number;
/* End - BUG:M0000068 */

/* Begin - ReqID:SRS-MYSQL-NUMBERICTYPE */
extern PGDLLIMPORT int div_precision_increment;
/* End - ReqID:SRS-MYSQL-NUMBERICTYPE */
/* Begin - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
extern PGDLLIMPORT int	auto_increment_increment;
extern PGDLLIMPORT int	auto_increment_offset;
/* End - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
/* Begin - ReqID:SRS-GKC-DATATYPE_001 */
extern PGDLLIMPORT int	nls_length_semantics;	/* BUG#Z091 */
extern PGDLLIMPORT char *nls_date_format;
extern PGDLLIMPORT char *nls_timestamp_format;
extern PGDLLIMPORT char *nls_timestamp_tz_format;
extern PGDLLIMPORT int	datetime_ignore_nls_mask;
/* Begin - ReqID:COMPATIBLE_GROUP */
extern PGDLLIMPORT bool compatible_group;
/* End - ReqID:COMPATIBLE_GROUP */
/* Begin - ReqID:SRS-SQL-SYSFUNC */
extern PGDLLIMPORT char *nls_territory;
extern PGDLLIMPORT char *nls_currency;
extern PGDLLIMPORT char *nls_iso_currency;
extern PGDLLIMPORT char *nls_dual_currency;
extern PGDLLIMPORT char *nls_numeric_characters;
/* End - ReqID:SRS-SQL-SYSFUNC */

/* Begin - ReqID:SRS-SQL-INSERT */
extern PGDLLIMPORT bool insert_enable_alias;
/* End - ReqID:SRS-SQL-INSERT */

/* Begin - ReqID:SRS-SQL-MERGE */
extern PGDLLIMPORT int enable_ora_merge_tuning;
/* End - ReqID:SRS-SQL-MERGE */

/* Begin - ReqID:SRS-SQL-FUNCOUT */
extern PGDLLIMPORT bool	allow_out_parameter_const;
extern PGDLLIMPORT bool result_first;
/* End - ReqID:SRS-SQL-FUNCOUT */

/* Begin - ReqID:SRS-SQL-SEQ */
extern PGDLLIMPORT bool	seq_scale_fixed;
/* End - ReqID:SRS-SQL-SEQ */
extern PGDLLIMPORT bool internal_warning; /* BUG#Z230 */


extern PGDLLIMPORT bool	visibility_switch;	/* ReqID:SRS-SQL-INVISIBLECOL */
/* Begin - ReqID:SRS-SQL-GTT */
extern int max_active_gtt;
extern int vacuum_gtt_defer_check_age;
/* End - ReqID:SRS-SQL-GTT */

/* Begin - ReqID:SRS-SQL-ROWID */
extern PGDLLIMPORT bool default_with_rowids;
extern PGDLLIMPORT int rowid_seq_cache;
/* End - ReqID:SRS-SQL-ROWID */

/* Begin - BUG#M0000201: support question marks as binding variables */
extern PGDLLIMPORT int where_from;
/* End - BUG#M0000201: support question marks as binding variables */

extern PGDLLIMPORT bool sort_desc_nulls_last;	/* BUG#M0000197 */
extern PGDLLIMPORT bool cast_int_like_oracle;	/* BUG#M0000194 */
extern PGDLLIMPORT bool ora_views_index_scan;	/* BUG#M0000302 */

/* Begin - BUG#M0000183 */
extern PGDLLIMPORT bool check_package_invalid_by_tuple;
/* End - BUG#M0000183 */

/* Begin - BUG#M0000227 */
extern PGDLLIMPORT bool compile_plisql_use_owner;
/* End - BUG#M0000227 */

/* Begin - SRS-MISC-COMPATDM-SQL */
extern PGDLLIMPORT bool enable_blankspace_compare;
/* End - SRS-MISC-COMPATDM-SQL */
/* Begin - SRS-SQL-NLSSORT */
enum
{
	NLS_COMP_BINARY,
	NLS_COMP_LINGUISTIC,
	NLS_COMP_ANSI
};

extern PGDLLIMPORT bool enable_ora_default_collation;
extern PGDLLIMPORT bool subquery_with_raw_collation;
extern PGDLLIMPORT bool default_collation_is_c;
extern PGDLLIMPORT int	max_string_size;
extern PGDLLIMPORT int	nls_comp_option;
extern PGDLLIMPORT char	*nls_sort_str;
extern PGDLLIMPORT Oid	nls_sort_oid;
extern PGDLLIMPORT Oid	default_collation_oid;
/* End - SRS-SQL-NLSSORT */


/* Begin - ReqID:SRS-PLSQL-CURSOR */
extern PGDLLIMPORT bool check_strong_cursor_assign;
extern int	open_cursors;
extern bool enable_cursor_as_column_type;
/* End - ReqID:SRS-PLSQL-CURSOR */

/* Begin - BUG#M0000376 */
extern PGDLLIMPORT bool enable_get_oraparam_mode;
/* End - BUG#M0000376 */

/* Begin - ReqID:SRS-PLSQL-EXCEPTION */
extern PGDLLIMPORT char *plsql_warnings;
/* End - ReqID:SRS-PLSQL-EXCEPTION */

/* Begin - BUG#Z0001269 */
extern PGDLLIMPORT bool convert_utf8_to_gbk_question_mark;
/* End - BUG#Z00001269 */

/* Begin - ReqID:SRS-SQL-MULTICOLUMNUPDATE */
extern PGDLLIMPORT bool enable_update_multicol;
/* End - ReqID:SRS-SQL-MULTICOLUMNUPDATE */

/* Begin - BUG#M000411 */
extern bool enable_compatible_const_integer_operator;
/* End - BUG#M0000411 */

/* Begin - BUG#Z0001452 */
extern bool enable_compatible_numeric_division_result;
/* End - BUG#Z00001452 */


/* Begin - BUG#M0000470: SELECT FOR UPDATE */
extern PGDLLIMPORT bool allow_forupdate_in_subquery;
/* End - BUG#M0000470: SELECT FOR UPDATE */

extern PGDLLIMPORT bool enable_pg_interval_datatype; /* BUG#M0000475 */
extern PGDLLIMPORT bool	alter_database_set_special;  /* BUG#M0000475 */

extern bool package_security_define_compare_user;	/* BUG#M496 */

extern PGDLLIMPORT bool enable_ora_user_schema;		/* ReqID:SRS-SQL-SCHEMA */
extern PGDLLIMPORT int dbms_output_discard_buffer_limit; /* ReqID:SRS-PLSQL-DBMS_OUTPUT */


/* Begin - BUG#M0000414 */
extern bool enable_store_nullterminator_char;
/* End - BUG#M0000414 */

/* Begin - BUG#z1616 */
extern PGDLLIMPORT bool convert_gbk_to_utf8_question_mark;
/* End - BUG#z1616 */

/* Begin - ReqID:SRS-SQL-HierarchicalQueries */
extern bool connect_by_rownum_use_func;
/* End - ReqID:SRS-SQL-HierarchicalQueries */

/* Begin - OA#1475 */
extern bool compatible_ora_bytea_concat_operator;
/* End - OA#1475 */

extern PGDLLIMPORT bool enable_strict_view_def; /* OA-REQ-1558 */

/* Begin - OA#00001554 */
extern PGDLLIMPORT bool enable_compatible_type_shippable;
/* End - OA#00001554 */
extern bool enable_flashback_recycle_bin;	/* ReqID:SRS-RECOVERY-FLASHBACK */

extern bool	enable_coverwritten_ora_trigger;	/* ReqID:SRS-SQL-TRIGGER */

/* Begin - RSR-SQL-Partition-interval */
extern PGDLLIMPORT int max_interval_partition_failures;
/* End - RSR-SQL-Partition-interval */

#define ORADATE_MASK			0x01
#define ORATIMESTAMP_MASK		0x02
#define ORATIMESTAMPTZ_MASK		0x04
#define ORATIMESTAMPLTZ_MASK	0x08
#define DATETIME_IGNORE_NLS(i, m) (((i) & (m)) != 0)
/* End - ReqID:SRS-GKC-DATATYPE_001 */
/* Begin - ReqID:SRS-SQL-LIBPQOUT */
extern PGDLLIMPORT bool out_parameter_column_position;
/* End - ReqID:SRS-SQL-LIBPQOUT */

/* Begin - ReqID:SRS-SQL-DBLINK */
extern PGDLLIMPORT char *default_dblink_fdw;
extern PGDLLIMPORT int ora_open_links;
extern PGDLLIMPORT int32 ora_open_links_per_instance;	/*BUG#Z1325 */
extern PGDLLIMPORT bool check_lnk_metadata_changed;
extern PGDLLIMPORT bool ora_global_names;
extern PGDLLIMPORT bool dblinkrel_schema_use_user;	/* BUG#Z4433 */
/* End - ReqID:SRS-SQL-DBLINK */

/* Begin - wai BUG#Z00001678 */
extern int format_max_string_size_len;
/* End - wai BUG#Z00001678 */


/* Begin - ReqID:SRS-MYSQL-UPDATE */
extern PGDLLIMPORT bool enable_indexhint;
/* End - ReqID:SRS-MYSQL-UPDATE */

/* Begin - BUG#Z1434 */
extern PGDLLIMPORT bool enable_ignore_columns_list;
/* End - BUG#Z1434 */

/* Begin - ReqID:SRS-MYSQL-CASE */
extern PGDLLIMPORT int lower_case_table_names;
extern PGDLLIMPORT bool myenable_case_switch;
/* End - ReqID:SRS-MYSQL-CASE */

extern PGDLLIMPORT bool not_mode_pad_char_to_full;	/* ReqID: SRS-MYSQL-CHARACTERTYPE-CHAR(n) */
extern PGDLLIMPORT bool mysql_convert;	/* ReqID: SRS-MYSQL-CHARACTERTYPE-CONVERT */
/* Begin - BUG#M533 */
extern PGDLLIMPORT bool regress_test_default_value;
extern PGDLLIMPORT bool cast_to_char;
/* End - BUG#M533 */
extern PGDLLIMPORT bool use_mysql_bool;

extern PGDLLIMPORT bool enable_mysql_information_schema;

/* Begin - ReqID:SRS-MYSQL-BUILDINFUNCTION */
extern PGDLLIMPORT char *lc_time_names;
extern PGDLLIMPORT int default_week_format;
/* End - ReqID:SRS-MYSQL-BUILDINFUNCTION */

extern PGDLLIMPORT bool at_or_atat;		/* ReqID:SRS-MYSQL-SET */
extern PGDLLIMPORT bool sql_quote_show_create;	/* ReqID:SRS-MYSQL-SHOW */
extern PGDLLIMPORT bool use_mysql_do;	/* ReqID:SRS-MYSQL-SHOW */

/* Begin - ReqID:SRS-SECURITY-DATADESENSITIZATION
 * support data redaction policy
 */
extern PGDLLIMPORT bool hg_redaction_data;
/* End - ReqID:SRS-SECURITY-DATADESENSITIZATION */

/* Begin - Req: SRS-MYSQL-PROTOCOL */
extern PGDLLIMPORT char     *sha256_password_public_key_path;
extern PGDLLIMPORT char     *sha256_password_private_key_path;
extern PGDLLIMPORT char 	*version_comment_str;
/* End - Req: SRS-MYSQL-PROTOCOL */

extern PGDLLIMPORT bool use_mysql_bit_literal;	/* SRS-MYSQL-HEX-LITERAL */

/*
 * Functions exported by guc.c
 */
extern void SetConfigOption(const char *name, const char *value,
							GucContext context, GucSource source);

extern void DefineCustomBoolVariable(const char *name,
									 const char *short_desc,
									 const char *long_desc,
									 bool *valueAddr,
									 bool bootValue,
									 GucContext context,
									 int flags,
									 GucBoolCheckHook check_hook,
									 GucBoolAssignHook assign_hook,
									 GucShowHook show_hook);

extern void DefineCustomIntVariable(const char *name,
									const char *short_desc,
									const char *long_desc,
									int *valueAddr,
									int bootValue,
									int minValue,
									int maxValue,
									GucContext context,
									int flags,
									GucIntCheckHook check_hook,
									GucIntAssignHook assign_hook,
									GucShowHook show_hook);

extern void DefineCustomRealVariable(const char *name,
									 const char *short_desc,
									 const char *long_desc,
									 double *valueAddr,
									 double bootValue,
									 double minValue,
									 double maxValue,
									 GucContext context,
									 int flags,
									 GucRealCheckHook check_hook,
									 GucRealAssignHook assign_hook,
									 GucShowHook show_hook);

extern void DefineCustomStringVariable(const char *name,
									   const char *short_desc,
									   const char *long_desc,
									   char **valueAddr,
									   const char *bootValue,
									   GucContext context,
									   int flags,
									   GucStringCheckHook check_hook,
									   GucStringAssignHook assign_hook,
									   GucShowHook show_hook);

extern void DefineCustomEnumVariable(const char *name,
									 const char *short_desc,
									 const char *long_desc,
									 int *valueAddr,
									 int bootValue,
									 const struct config_enum_entry *options,
									 GucContext context,
									 int flags,
									 GucEnumCheckHook check_hook,
									 GucEnumAssignHook assign_hook,
									 GucShowHook show_hook);

extern void EmitWarningsOnPlaceholders(const char *className);

extern const char *GetConfigOption(const char *name, bool missing_ok,
								   bool restrict_privileged);
extern const char *GetConfigOptionResetString(const char *name);
extern int	GetConfigOptionFlags(const char *name, bool missing_ok);
extern void ProcessConfigFile(GucContext context);
extern void InitializeGUCOptions(void);
extern bool SelectConfigFiles(const char *userDoption, const char *progname);
extern void ResetAllOptions(void);
extern void AtStart_GUC(void);
extern int	NewGUCNestLevel(void);
extern void AtEOXact_GUC(bool isCommit, int nestLevel);
extern void BeginReportingGUCOptions(void);
extern void ReportChangedGUCOptions(void);
extern void ParseLongOption(const char *string, char **name, char **value);
extern bool parse_int(const char *value, int *result, int flags,
					  const char **hintmsg);
extern bool parse_real(const char *value, double *result, int flags,
					   const char **hintmsg);
extern int	set_config_option(const char *name, const char *value,
							  GucContext context, GucSource source,
							  GucAction action, bool changeVal, int elevel,
							  bool is_reload);
extern void AlterSystemSetConfigFile(AlterSystemStmt *altersysstmt);
extern char *GetConfigOptionByName(const char *name, const char **varname,
								   bool missing_ok);
extern void GetConfigOptionByNum(int varnum, const char **values, bool *noshow);
extern int	GetNumConfigOptions(void);

extern void SetPGVariable(const char *name, List *args, bool is_local);
extern void GetPGVariable(const char *name, DestReceiver *dest);
extern TupleDesc GetPGVariableResultDesc(const char *name);

extern void ExecSetVariableStmt(VariableSetStmt *stmt, bool isTopLevel);
extern char *ExtractSetVariableArgs(VariableSetStmt *stmt);

extern void ProcessGUCArray(ArrayType *array,
							GucContext context, GucSource source, GucAction action);
extern ArrayType *GUCArrayAdd(ArrayType *array, const char *name, const char *value);
extern ArrayType *GUCArrayDelete(ArrayType *array, const char *name);
extern ArrayType *GUCArrayReset(ArrayType *array);

#ifdef EXEC_BACKEND
extern void write_nondefault_variables(GucContext context);
extern void read_nondefault_variables(void);
#endif

/* GUC serialization */
extern Size EstimateGUCStateSpace(void);
extern void SerializeGUCState(Size maxsize, char *start_address);
extern void RestoreGUCState(void *gucstate);

/* Support for messages reported from GUC check hooks */

#ifdef MISC_INTERNAL /* Begin - ReqID:SRS-Performance-GUC */
extern void SetConfigFile(char *name, char *value);
#endif /* End - ReqID:SRS-Performance-GUC */

extern PGDLLIMPORT char *GUC_check_errmsg_string;
extern PGDLLIMPORT char *GUC_check_errdetail_string;
extern PGDLLIMPORT char *GUC_check_errhint_string;

extern PGDLLIMPORT char *iptimelimite;

extern void GUC_check_errcode(int sqlerrcode);

#define GUC_check_errmsg \
	pre_format_elog_string(errno, TEXTDOMAIN), \
	GUC_check_errmsg_string = format_elog_string

#define GUC_check_errdetail \
	pre_format_elog_string(errno, TEXTDOMAIN), \
	GUC_check_errdetail_string = format_elog_string

#define GUC_check_errhint \
	pre_format_elog_string(errno, TEXTDOMAIN), \
	GUC_check_errhint_string = format_elog_string


/*
 * The following functions are not in guc.c, but are declared here to avoid
 * having to include guc.h in some widely used headers that it really doesn't
 * belong in.
 */

/* in commands/tablespace.c */
extern bool check_default_tablespace(char **newval, void **extra, GucSource source);
extern bool check_temp_tablespaces(char **newval, void **extra, GucSource source);
extern void assign_temp_tablespaces(const char *newval, void *extra);

/* in catalog/namespace.c */
extern bool check_search_path(char **newval, void **extra, GucSource source);
extern void assign_search_path(const char *newval, void *extra);

/* in access/transam/xlog.c */
extern bool check_wal_buffers(int *newval, void **extra, GucSource source);
extern void assign_xlog_sync_method(int new_sync_method, void *extra);

/* Begin - ReqID:SRS-MYSQL-SET */
extern bool guc_list_input_check(const char *varname);
/* End - ReqID:SRS-MYSQL-SET */

/* Begin - ReqID:SRS-MISC-MYSQL-FRAME */
#define MODE_REAL_AS_FLOAT              ((1ULL << 0))
#define MODE_PIPES_AS_CONCAT            ((1ULL << 1))
#define MODE_ANSI_QUOTES                ((1ULL << 2))
#define MODE_IGNORE_SPACE               ((1ULL << 3))
#define MODE_NOT_USED                   ((1ULL << 4))
#define MODE_ONLY_FULL_GROUP_BY         ((1ULL << 5))
#define MODE_NO_UNSIGNED_SUBTRACTION    ((1ULL << 6))
#define MODE_NO_DIR_IN_CREATE           ((1ULL << 7))
#define MODE_POSTGRESQL                 ((1ULL << 8))
#define MODE_ORACLE                     ((1ULL << 9))
#define MODE_MSSQL                      ((1ULL << 10))
#define MODE_DB2                        ((1ULL << 11))
#define MODE_MAXDB                      ((1ULL << 12))
#define MODE_NO_KEY_OPTIONS             ((1ULL << 13))
#define MODE_NO_TABLE_OPTIONS           ((1ULL << 14))
#define MODE_NO_FIELD_OPTIONS           ((1ULL << 15))
#define MODE_MYSQL323                   ((1ULL << 16))
#define MODE_MYSQL40                    ((1ULL << 17))
#define MODE_ANSI                       ((1ULL << 18))
#define MODE_NO_AUTO_VALUE_ON_ZERO      ((1ULL << 19))
#define MODE_NO_BACKSLASH_ESCAPES       ((1ULL << 20))
#define MODE_STRICT_TRANS_TABLES        ((1ULL << 21))
#define MODE_STRICT_ALL_TABLES          ((1ULL << 22))
#define MODE_NO_ZERO_IN_DATE            ((1ULL << 23))
#define MODE_NO_ZERO_DATE               ((1ULL << 24))
#define MODE_INVALID_DATES              ((1ULL << 25))
#define MODE_ERROR_FOR_DIVISION_BY_ZERO ((1ULL << 26))
#define MODE_TRADITIONAL                ((1ULL << 27))
#define MODE_NO_AUTO_CREATE_USER        ((1ULL << 28))
#define MODE_HIGH_NOT_PRECEDENCE        ((1ULL << 29))
#define MODE_NO_ENGINE_SUBSTITUTION     ((1ULL << 30))
#define MODE_PAD_CHAR_TO_FULL_LENGTH    ((1ULL << 31))

#define NEXT_MODE_VALUE(cur) 			((cur << 1))
#define MODE_VALUES_FIRST				MODE_REAL_AS_FLOAT
#define MODE_VALUES_LAST				MODE_PAD_CHAR_TO_FULL_LENGTH

#define has_sql_mode(t)		((sql_mode) & (t))
#define is_strict_mode() 	(has_sql_mode(MODE_STRICT_TRANS_TABLES | MODE_STRICT_ALL_TABLES))
#define mode_pad_char_to_full()	(has_sql_mode(MODE_PAD_CHAR_TO_FULL_LENGTH))

typedef unsigned long long sql_mode_t;

extern PGDLLIMPORT sql_mode_t  sql_mode;
/* End - ReqID:SRS-MISC-MYSQL-FRAME */

/* Begin - ReqID:SRS-MYSQL-BINARYTYPE */
extern PGDLLIMPORT int 	 max_allowed_packet;
extern PGDLLIMPORT int 	 max_sort_length;
extern PGDLLIMPORT bool  enable_character_blob;
/* End - ReqID:SRS-MYSQL-BINARYTYPE */

/* Begin - BUG#Z4610 */
extern PGDLLIMPORT bool aggregates_without_group_by;
/* End - BUG#Z4610 */

/* BEGIN - SRS-MYSQL-BUILDINFUNCTION */
typedef enum WeightStringAS
{
	WEIGHT_STRING_AS_UNUSE,
	WEIGHT_STRING_AS_CHAR,
	WEIGHT_STRING_AS_BINARY
} WeightStringAS;
/* End - SRS-MYSQL-BUILDINFUNCTION */

/* Begin - Req: SRS-MYSQL-PROTOCOL */
extern PGDLLIMPORT int	OraPortNumber;
extern PGDLLIMPORT char *OraListenAddresses;

enum protocol_type
{
	PROTOCOL_PG,
	PROTOCOL_MYSQL,
};

extern PGDLLIMPORT int	mys_port;
extern PGDLLIMPORT char *mys_listen_address;
extern PGDLLIMPORT int 	mys_protocol_type;
extern PGDLLIMPORT char *default_database;
extern PGDLLIMPORT bool	mys_autocommit;
/* End - Req: SRS-MYSQL-PROTOCOL */

#endif							/* GUC_H */
