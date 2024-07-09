/*-------------------------------------------------------------------------
 *
 * ivy_guc.c
 *
 * Support for grand unified configuration scheme(for ivorysql), including SET
 * command, configuration file, and command line options
 *
 * Authored by lanke@highgo.com, 20220819.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *        src/backend/utils/misc/ivy_guc.c
 *
 *-------------------------------------------------------------------------
 */
#ifdef IVY_GUC_VAR_DEFINE
#define ISLOADIVORYSQL_ORA pg_transform_merge_stmt_hook

/* Begin - ReqID:SRS-SQL-GTT */
/*
 * num = 0 means disable global temporary table feature.
 * table schema are still saved in catalog.
 *
 * num > 0 means allows the database to manage multiple active tables at the same time.
 */
#define	MIN_NUM_ACTIVE_GTT			0
#define	DEFAULT_NUM_ACTIVE_GTT			1000
#define	MAX_NUM_ACTIVE_GTT			1000000

int		max_active_gtt = MIN_NUM_ACTIVE_GTT;
int		vacuum_gtt_defer_check_age = 0;
/* End - ReqID:SRS-SQL-GTT */

/* BEGIN - case sensitive indentify */
int				identifier_case_switch = INTERCHANGE;
bool  			identifier_case_from_pg_dump = false;
bool			enable_case_switch = true;
/* END - case sensitive indentify */

/* Begin - ReqID:SRS-GKC-DATATYPE_001 */
int			nls_length_semantics = NLS_LENGTH_BYTE;	/* BUG#Z091 */
char	   *nls_date_format = "";
char	   *nls_timestamp_format = "";
char	   *nls_timestamp_tz_format = "";
int 		datetime_ignore_nls_mask = 0;
/* End - ReqID:SRS-GKC-DATATYPE_001 */

/* Begin - ReqID:SRS-SQL-SYSFUNC */
char	   *nls_territory = "";
char	   *nls_currency = "";
char	   *nls_iso_currency = "";
char	   *nls_dual_currency = "";
char	   *nls_numeric_characters = "";
/* End - ReqID:SRS-SQL-SYSFUNC */

/* Begin - SRS-SQL-NLSSORT */
bool		 enable_ora_default_collation = true;
bool		 enable_partitionwise_qual_force_pushdown = true;
bool		 default_collation_is_c = true;
char		*nls_sort_str;
char		*nls_comp_str;
char		*default_collation_str;

int			max_string_size;
int	 		nls_comp_option = NLS_COMP_BINARY;

Oid			nls_sort_oid = ORA_COLL_OID(BINARY);
Oid			default_collation_oid = InvalidOid;
/* End - SRS-SQL-NLSSORT */

/* Begin - ReqID:EMPTY_STIRNG_TO_NULL */
bool			enable_emptystring_to_NULL = false;
/* End - ReqID:EMPTY_STIRNG_TO_NULL */


/* Begin - ReqID:COMPATIBLE_GROUP */
bool          compatible_group = false;
/* End - ReqID:COMPATIBLE_GROUP */

/* Begin - ReqID:SRS-SQL-INSERT */
bool		insert_enable_alias = false;
/* End - ReqID:SRS-SQL-INSERT */

/* Begin - BUG:M0000068 */
bool	convert_plisql_int_to_number = false;
bool	convert_plisql_anonymous_int_to_number= false;
/* End - BUG:M0000068 */

/* Begin - ReqID:SRS-SQL-MERGE */
int		enable_ora_merge_tuning = OMERGE_TUNING_OFF;
/* End - ReqID:SRS-SQL-MERGE */

/* BEGIN - SQL PARSER */
int	database_mode = DB_PG;
int	compatible_db = PG_PARSER;
/* END - SQL PARSER */
/* Begin - ReqID:SRS-SQL-FUNCOUT */
bool		allow_out_parameter_const = false;
bool		result_first = false;
/* End - ReqID:SRS-SQL-FUNCOUT */

/* Begin - ReqID:SRS-SQL-LIBPQOUT */
bool		out_parameter_column_position = false;
/* End - ReqID:SRS-SQL-LIBPQOUT */

/* Begin - ReqID:SRS-SQL-SEQ */
bool	seq_scale_fixed = false;
/* End - ReqID:SRS-SQL-SEQ */
bool	internal_warning = false;	/* BUG#Z230 */


/* Begin - BUG#M0000201: support question marks as binding variables */
/* where the application is coming from */
int	where_from = FROM_UNKNOWN;
/* End - BUG#M0000201: support question marks as binding variables */

/* Begin - ReqID:SRS-SQL-ROWID */
bool   default_with_rowids = false;
int        rowid_seq_cache = 20;
/* End - ReqID:SRS-SQL-ROWID */

bool   visibility_switch = false;      /* ReqID:SRS-SQL-INVISIBLECOL */

bool sort_desc_nulls_last = false;		/* BUG#M0000197 */
bool cast_int_like_oracle = false;		/* BUG#M0000194 */
bool ora_views_index_scan = false;		/* BUG#M0000302 */

/* Begin - BUG#M0000183 */
bool check_package_invalid_by_tuple = true;
/* End - BUG#M0000183 */

/* Begin - BUG#M0000227 */
bool compile_plisql_use_owner = true;
/* End - BUG#M0000227 */


/* BEGIN - ReqID:SRS-MISC-USER */
int dumppgtype = PG_DUMP;
int grant_usertype = PG_GRANTUSER;
/* END - ReqID:SRS-MISC-USER */
/* Begin - BUG#z476 */
bool numerictonumber = false;
/* End - BUG#z476 */

/* Begin - ReqID:SRS-SQL-DBLINK */
int ora_open_lins = 4;
char *default_dblink_fdw = "";
bool check_lnk_metadata_changed = false;
bool ora_global_names = false;
/* End - ReqID:SRS-SQL-DBLINK */


/* Begin - SRS-MISC-COMPATDM-SQL, this GUC for compat DM SPACE_COMPARE_MODE,
 * if this is true,we  compare the blank padding in chracter type as same as oracle,
 * else, as same as DM(SPACE_COMPARE_MODE =0,BLANK_PAD_MODE=0) 
 */
bool enable_blankspace_compare = true;
/* End - SRS-MISC-COMPATDM-SQL */

/* Begin - ReqID:SRS-PLSQL-CURSOR */
bool	check_strong_cursor_assign = true;
int		open_cursors = 50;
bool	enable_cursor_as_column_type = false;
/* End - ReqID:SRS-PLSQL-CURSOR */

/* Begin - BUG#M0000376 */
bool	enable_get_oraparam_mode = false;
/* End - BUG#M0000376 */

#endif

#ifdef IVY_GUC_VAR_STRUCT
/* BEGIN - SQL PARSER */
/* The comments shown as blow define the
 * value range of guc parameters "database_mode"
 * and "compatible_db".
 */
static const struct config_enum_entry db_mode_options[] = {
	{"pg", DB_PG, false},
	{"oracle", DB_ORACLE, false},
	{"0", DB_PG, false},
	{"1", DB_ORACLE, false},
	{NULL, 0, false}
};

static const struct config_enum_entry db_parser_options[] = {
	{"pg", PG_PARSER, false},
	{"oracle", ORA_PARSER, false},
	{NULL, 0, false}
/* END - SQL PARSER */
};

/* Begin - BUG#M0000201: support question marks as binding variables */
/* where the application is coming from */
static const struct config_enum_entry where_from_options[] = {
	{"unknown", FROM_UNKNOWN, false},
	{"jdbc", FROM_JDBC, false},
	{"c", FROM_C, false},
	{"oci", FROM_OCI, false},
	{"libpq", FROM_LIBPQ, false},
	{"odbc", FROM_ODBC, false},	/* BUG#M0000346 */
	{"0", FROM_UNKNOWN, false},
	{"1", FROM_JDBC, false},
	{"2", FROM_C, false},
	{"3", FROM_OCI, false},
	{"4", FROM_LIBPQ, false},
	{"5", FROM_ODBC, false},	/* BUG#M0000346 */
	{NULL, 0, false}
};
/* End - BUG#M0000201: support question marks as binding variables */

/* BEGIN - case sensitive indentify */
static const struct config_enum_entry case_conversion_mode[] = {
	{"normal", NORMAL, false},
	{"interchange", INTERCHANGE, false},
	{"lowercase", LOWERCASE, false},
	{"0", NORMAL, false},
	{"1", INTERCHANGE, false},
	{"2", LOWERCASE, false},
	{NULL, 0, false}
};
/* END - case sensitive indentify */

/* BEGIN - ReqID:SRS-MISC-USER */
static const struct config_enum_entry dump_pgtype_options[] = {
	{"pg", PG_DUMP, false},
	{"oracle", ORA_DUMP, false},
	{"0", PG_DUMP, false},
	{"1", ORA_DUMP, false},
	{NULL, 0, false}
};

static const struct config_enum_entry grant_usertype_options[] = {
	{"pg", PG_GRANTUSER, false},
	{"oracle", ORA_GRANTUSER, false},
	{"0", PG_GRANTUSER, false},
	{"1", ORA_GRANTUSER, false},
	{NULL, 0, false}
};
/* END - ReqID:SRS-MISC-USER */

/* Begin - BUG#Z091 */
static const struct config_enum_entry nls_length_options[] = {
	{"byte", NLS_LENGTH_BYTE, false},
	{"char", NLS_LENGTH_CHAR, false},
	{NULL, 0, false}
};
/* End - BUG#Z091 */

/* Begin - ReqID:SRS-SQL-MERGE */
static const struct config_enum_entry ora_merge_tuning[] = {
	{"off", OMERGE_TUNING_OFF, false},
	{"on", OMERGE_TUNING_ON, false},
	{"force", OMERGE_TUNING_FORCE, false},
	{NULL, 0, false}
};
/* End - ReqID:SRS-SQL-MERGE */

/* Begin - SRS-SQL-NLSSORT */
static const struct config_enum_entry max_string_size_options[] = {
	{"standard", ORA_MAX_STRING_SIZE_STANDARD, false},
	{"extended", ORA_MAX_STRING_SIZE_EXTENDED, false},
	{NULL, 0, false}
};
static const struct config_enum_entry nls_comp_options[] = {
	{"binary", NLS_COMP_BINARY, false},
	{"linguistic", NLS_COMP_LINGUISTIC, false},
	{"ansi", NLS_COMP_ANSI, false},
	{NULL, 0, false}
};
/* End - SRS-SQL-NLSSORT */
#endif

#ifdef IVY_GUC_FUNC_DECLARE
/* Begin - ReqID:SRS-GKC-DATATYPE_001 */
static bool check_nls_length_semantics(int *newval, void **extra, GucSource source);	/* BUG#Z091 */
/* End - ReqID:SRS-GKC-DATATYPE_001 */
static bool check_compatible_mode(int *newval, void **extra, GucSource source);
static bool check_database_mode(int *newval, void **extra, GucSource source);
void assign_compatible_mode(int newval, void *extra);
/* Begin - ReqID:SRS-SQL-SYSFUNC */
static void nls_case_conversion(char **param, char type);
static bool nls_length_check(char **newval, void **extra, GucSource source);
static bool nls_territory_check(char **newval, void **extra, GucSource source);
static bool nls_numeric_characters_check(char **newval, void **extra,
										 GucSource source);
static bool check_dump_pgtype(int *newval, void **extra, GucSource source);
static bool check_grant_usertype(int *newval, void **extra, GucSource source);
/* End - ReqID:SRS-SQL-SYSFUNC */

/* Begin - SRS-SQL-NLSSORT */
static bool nls_comp_check(char **newval, void **extra, GucSource source);
static void nls_comp_assign(const char *newval, void *extra);
static bool nls_sort_check(char **newval, void **extra, GucSource source);
static void nls_sort_assign(const char *newval, void *extra);
static bool default_collation_check(char **newval, void **extra, GucSource source);
static void default_collation_assign(const char *newval, void *extra);
static bool max_string_size_check(int *newval, void **extra, GucSource source);
/* End - SRS-SQL-NLSSORT */
/* Begin - ReqID:SRS-SQL-DBLINK */
static bool dblink_using_fdw_check(char **newval, void **extra, GucSource source);
/* End - ReqID:SRS-SQL-DBLINK */
#endif

#if 0
static struct config_bool Ivy_ConfigureNamesBool[] =
{
#endif
#ifdef IVY_GUC_BOOL_PARAMS
	/* BEGIN - case sensitive indentify */
	{
		/* Not for general use --- used by pg_dump */
		{"ivorysql.identifier_case_from_pg_dump", PGC_USERSET, UNGROUPED,
			gettext_noop("Shows whether the identifer with quote is from pg dump."),
			NULL,
			GUC_REPORT | GUC_NO_SHOW_ALL | GUC_NO_RESET_ALL | GUC_NOT_IN_SAMPLE | GUC_DISALLOW_IN_FILE
		},
		&identifier_case_from_pg_dump,
		false,
		NULL, NULL, NULL
	},
	{
		{"ivorysql.enable_case_switch", PGC_USERSET, CLIENT_CONN_STATEMENT,
			gettext_noop("whether enable case conversion feature in oracle compatible mode."),
			NULL,
			GUC_NO_SHOW_ALL | GUC_NOT_IN_SAMPLE | GUC_DISALLOW_IN_FILE
		},
		&enable_case_switch,
		true,
		NULL, NULL, NULL
	},
	/* END - case sensitive indentify */

	/* Begin - ReqID:EMPTY_STIRNG_TO_NULL */
	{
		{"ivorysql.enable_emptystring_to_NULL", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("whether convert empty string to NULL."),
			NULL
		},
		&enable_emptystring_to_NULL,
		false,
		NULL, NULL, NULL
	},
	/* End - ReqID:EMPTY_STIRNG_TO_NULL */

    /* Begin - ReqID:COMPATIBLE_GROUP */	
	{
		{"ivorysql.compatible_group", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("whether fully consistent with the Oracle mode."),
			NULL
		},
		&compatible_group,
		false,
		NULL, NULL, NULL
	},
	/* End  - ReqID:COMPATIBLE_GROUP */

	/* Begin - ReqID:SRS-SQL-INSERT */
	{
		{"ivorysql.insert_enable_alias", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Whether to enable INSERT alias-qualified columns."),
			NULL
		},
		&insert_enable_alias,
		false,
		NULL, NULL, NULL
	},
	/* End - ReqID:SRS-SQL-INSERT */

	/* Begin - BUG:M0000068 */
	{
		{"ivorysql.convert_plisql_int_to_number", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Whether the created plisql function converts integer and smallint to number by default."),
			NULL
		},
		&convert_plisql_int_to_number,
		false,
		NULL, NULL, NULL
	},

	{
		{"ivorysql.convert_plisql_anonymous_int_to_number", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Whether converts integer and smallint to number within plisql anonymous block."),
			NULL
		},
		&convert_plisql_anonymous_int_to_number,
		false,
		NULL, NULL, NULL
	},
	/* End - BUG:M0000068 */

	/* Begin - ReqID:SRS-SQL-FUNCOUT */
	/*
	 * if ivorysql.allow_out_parameter_const is false, out parameter must be a var.
	 * if ivorysql.allow_out_parameter_const is true, out parameter can be a constant value
	 */
	{
		{"ivorysql.allow_out_parameter_const", PGC_USERSET, DEVELOPER_OPTIONS,
			gettext_noop("plisql function out parameter can be a constant value."),
			NULL,
			GUC_NO_SHOW_ALL | GUC_NOT_IN_SAMPLE
		},
		&allow_out_parameter_const,
		false,
		NULL, NULL, NULL
	},
	/*
	 * When a function has out parameters, and the return value is not VOID,
	 * combile the out parameter values and the return value to a tuple.
	 * If ivorysql.result_first is false, the first in tuple is out parameter values,
	 * followed by function result.
	 * If ivorysql.result_first is true, the first in tuple is the function result,
	 * followed by out parameter values.
	 */
	{
		{"ivorysql.result_first", PGC_USERSET, DEVELOPER_OPTIONS,
			gettext_noop("the function result precedes or follows the out parameter values."),
			NULL,
			GUC_NO_SHOW_ALL | GUC_NOT_IN_SAMPLE
		},
		&result_first,
		true,
		NULL, NULL, NULL
	},
	/* End - ReqID:SRS-SQL-FUNCOUT */
	/* Begin - ReqID:SRS-SQL-LIBPQOUT */
	{
		{"ivorysql.out_parameter_column_position", PGC_USERSET, DEVELOPER_OPTIONS,
			gettext_noop("plisql function construct out parameter column by its position."),
			NULL,
			GUC_NO_SHOW_ALL | GUC_NOT_IN_SAMPLE
		},
		&out_parameter_column_position,
		false,
		NULL, NULL, NULL
	},
	/* End - ReqID:SRS-SQL-LIBPQOUT */
	/* Begin - ReqID:SRS-SQL-SEQ */
	{
		{"ivorysql.enable_seq_scale_fixed", PGC_USERSET, DEVELOPER_OPTIONS,
			gettext_noop("Sequence scale value switch."),
			NULL,
			GUC_NO_SHOW_ALL | GUC_NOT_IN_SAMPLE
		},
		&seq_scale_fixed,
		false,
		NULL, NULL, NULL
	},
	/* End - ReqID:SRS-SQL-SEQ */
	/* Begin BUG#Z230 */
	{
		{"ivorysql.enable_internal_warning", PGC_USERSET, DEVELOPER_OPTIONS,
			gettext_noop("Whether to print internal warning information."),
			NULL,
			GUC_NO_SHOW_ALL | GUC_NOT_IN_SAMPLE
		},
		&internal_warning,
		false,
		NULL, NULL, NULL
	},
	/* End BUG#Z230 */

	/* Begin - ReqID:SRS-SQL-ROWID */
   	{
       	{"ivorysql.default_with_rowids", PGC_USERSET, DEVELOPER_OPTIONS,
           	gettext_noop("Auto add rowid column for create new table."),
           	NULL,
           	GUC_NO_SHOW_ALL | GUC_NOT_IN_SAMPLE
       	},
       	&default_with_rowids,
       	false,
       	NULL, NULL, NULL
   	},
   	/* End - ReqID:SRS-SQL-ROWID */

	/* Begin - ReqID:SRS-SQL-INVISIBLECOL */
	{
		{"colinvisible", PGC_USERSET, DEVELOPER_OPTIONS,
			gettext_noop("Used to control whether to view invisible column information."),
			NULL,
			GUC_NO_SHOW_ALL | GUC_NOT_IN_SAMPLE
		},
		&visibility_switch,
		false,
		NULL, NULL, NULL
	},
	/* End - ReqID:SRS-SQL-INVISIBLECOL */
	/* Begin - BUG#M0000197 */
	{
		{"ivorysql.sort_desc_nulls_last", PGC_USERSET, DEVELOPER_OPTIONS,
			gettext_noop("NULLS LAST for ORDER BY ... DESC"),
			NULL,
			GUC_NO_SHOW_ALL | GUC_NOT_IN_SAMPLE
		},
		&sort_desc_nulls_last,
		false,
		NULL, NULL, NULL
	},
	/* End - BUG#M0000197 */
	/* Begin - BUG#M0000194 */
	{
		{"ivorysql.cast_int_like_oracle", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("CAST(expr AS int) processing float and exponent like Oracle"),
			NULL,
			GUC_NO_SHOW_ALL | GUC_NOT_IN_SAMPLE
		},
		&cast_int_like_oracle,
		false,
		NULL, NULL, NULL
	},
	/* End - BUG#M0000194 */
	/* Begin - BUG#M0000183 */
	{
		{"ivorysql.check_package_invalid_by_tuple", PGC_POSTMASTER, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Wether check package invalid by tuple header."),
			NULL,
			GUC_NO_SHOW_ALL | GUC_NOT_IN_SAMPLE
		},
		&check_package_invalid_by_tuple,
		true,
		NULL, NULL, NULL
	},
	/* End - BUG#M0000183 */
	/* Begin - SRS-MISC-COMPATDM-SQL */
	{
		{"ivorysql.enable_blankspace_compare", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Enable character type blank space compare."),
			NULL,
			GUC_NO_SHOW_ALL | GUC_NOT_IN_SAMPLE
		},
		&enable_blankspace_compare,
		true,
		NULL, NULL, NULL
	},
	/* End - SRS-MISC-COMPATDM-SQL */

	/* Begin - BUG#z476 */
	{
		{"ivorysql.numerictonumber", PGC_USERSET, CLIENT_CONN_STATEMENT,
			gettext_noop("Set numeric to number(38)."),
			NULL
		},
		&numerictonumber,
		false, 
		NULL, NULL, NULL
	},
	/* End - BUG#z476 */
	/* Begin - BUG#M0000302 */
	{
		{"ivorysql.ora_views_index_scan", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Try to scan oracle compatible views using "
						 "pg_catalog indexes on specific column."),
			NULL,
			GUC_NO_SHOW_ALL | GUC_NOT_IN_SAMPLE
		},
		&ora_views_index_scan,
		true,
		NULL, NULL, NULL
	},
	/* End - BUG#M0000302 */
	/* Begin - ReqID:SRS-PLSQL-CURSOR */
	{
		{"ivorysql.check_strong_cursor_assign", PGC_USERSET, DEVELOPER_OPTIONS,
			gettext_noop("Whether to check assignment between strong cursor types."),
			NULL,
			GUC_NO_SHOW_ALL | GUC_NOT_IN_SAMPLE
		},
		&check_strong_cursor_assign,
		true,
		NULL, NULL, NULL
	},
	{
		{"ivorysql.enable_cursor_as_column_type", PGC_USERSET, DEVELOPER_OPTIONS,
			gettext_noop("Whether to enable cursor as a column type."),
			NULL,
			GUC_NO_SHOW_ALL | GUC_NOT_IN_SAMPLE
		},
		&enable_cursor_as_column_type,
		false,
		NULL, NULL, NULL
	},
	/* End - ReqID:SRS-PLSQL-CURSOR */
	/* BEGIN - SRS-SQL-NLSSORT */
	{
		{"ivorysql.enable_partitionwise_qual_force_pushdown", PGC_USERSET, CLIENT_CONN_STATEMENT,
			gettext_noop("whether allow to force push down qual when do wise partition aggerate in oracle compatible mode."),
			NULL,
			GUC_NO_SHOW_ALL | GUC_NOT_IN_SAMPLE | GUC_DISALLOW_IN_FILE
		},
		&enable_partitionwise_qual_force_pushdown,
		true,
		NULL, NULL, NULL
	},
	{
		{"ivorysql.enable_ora_default_collation", PGC_USERSET, CLIENT_CONN_STATEMENT,
			gettext_noop("whether allow to set default collation in oracle compatible mode."),
			NULL,
			GUC_NO_SHOW_ALL | GUC_NOT_IN_SAMPLE | GUC_DISALLOW_IN_FILE
		},
		&enable_ora_default_collation,
		true,
		NULL, NULL, NULL
	},
	{
		{"ivorysql.default_collation_is_c", PGC_USERSET, CLIENT_CONN_STATEMENT,
			gettext_noop("whether the default collation(100) is C collation in oracle compatible mode."),
			NULL,
			GUC_NO_SHOW_ALL | GUC_NOT_IN_SAMPLE | GUC_DISALLOW_IN_FILE
		},
		&default_collation_is_c,
		true,
		NULL, NULL, NULL
	},
	/* END - SRS-SQL-NLSSORT */
	/* Begin - ReqID:SRS-SQL-DBLINK */
	{
		{"ivorysql.check_lnk_metadata_changed", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Check whether the object metadata accessed by using dblink has changed."),
			NULL,
			GUC_NO_SHOW_ALL | GUC_NOT_IN_SAMPLE
		},
		&check_lnk_metadata_changed,
		false,
		NULL, NULL, NULL
	},
	{
		{"global_names", PGC_SUSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("enforce that database links have same name as remote database."),
			NULL,
			GUC_NO_SHOW_ALL | GUC_NOT_IN_SAMPLE
		},
		&ora_global_names,
		false,
		NULL, NULL, NULL
	},
	/* End - ReqID:SRS-SQL-DBLINK */
	/* Begin - BUG#M0000376 */
	{
		{"ivorysql.enable_get_oraparam_mode", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("get the oracle bind params' mode  in an anonymous block"),
			NULL,
			GUC_NO_SHOW_ALL | GUC_NOT_IN_SAMPLE
		},
		&enable_get_oraparam_mode,
		false,
		NULL, NULL, NULL
	},
	/* End - BUG#M0000376 */
#endif
#if 0
	/* End-of-list marker */
	{
		{NULL, 0, 0, NULL, NULL}, NULL, false, NULL, NULL, NULL
	}
};
#endif

#if 0
static struct config_int Ivy_ConfigureNamesInt[] =
{
#endif
#ifdef IVY_GUC_INT_PARAMS
	/* Begin - ReqID:LISTEN-MULTI-PORT */
	{
		{"ivorysql.port", PGC_POSTMASTER, CONN_AUTH_SETTINGS,
			gettext_noop("Sets the Oracle TCP port the server listens on."),
			NULL
		},
		&OraPortNumber,
		1521, 1, 65535,
		NULL, NULL, NULL
	},
	/* End - ReqID:LISTEN-MULTI-PORT */

	/* Begin - ReqID:SRS-GKC-DATATYPE_001 */
	{
		{"ivorysql.datetime_ignore_nls_mask", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Sets the datetime type input is not controlled by the NLS parameter."),
			NULL
		},
		&datetime_ignore_nls_mask,
		0, 0, 15,
		NULL, NULL, NULL
	},
	/* End - ReqID:SRS-GKC-DATATYPE_001 */

	/* Begin - ReqID:SRS-SQL-GTT */
	{
		{"ivorysql.max_active_gtt", PGC_POSTMASTER, UNGROUPED,
			gettext_noop("max active global temporary table."),
			NULL
		},
		&max_active_gtt,
		DEFAULT_NUM_ACTIVE_GTT, MIN_NUM_ACTIVE_GTT, MAX_NUM_ACTIVE_GTT,
		NULL, NULL, NULL
	},
	/* End - ReqID:SRS-SQL-GTT */

	/* Begin - ReqID:SRS-SQL-GTT */
	{
		{"ivorysql.vacuum_gtt_defer_check_age", PGC_USERSET, CLIENT_CONN_STATEMENT,
			gettext_noop("The defer check age of GTT, used to check expired data after vacuum."),
			NULL
		},
		&vacuum_gtt_defer_check_age,
		10000, 0, 1000000,
		NULL, NULL, NULL
	},
	/* End - ReqID:SRS-SQL-GTT */

	/* Begin - ReqID:SRS-SQL-ROWID */
	{
		{"ivorysql.rowid_seq_cache", PGC_USERSET, DEVELOPER_OPTIONS,
			gettext_noop("Sets rowid Sequence cache number."),
			NULL,
			GUC_NO_SHOW_ALL | GUC_NOT_IN_SAMPLE
		},
		&rowid_seq_cache,
		20, 1, 60000,
		NULL, NULL, NULL
	},
	/* End - ReqID:SRS-SQL-ROWID */
	/* Begin - ReqID:SRS-PLSQL-CURSOR */
	{
		{"open_cursors", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("specifies the maximum number of open cursors a session can have at once."),
			NULL
		},
		&open_cursors,
		50, 0, 65535,
		NULL, NULL, NULL
	},
	/* End - ReqID:SRS-PLSQL-CURSOR */
	/* Begin - ReqID:SRS-SQL-DBLINK */
	{
    	{"open_links", PGC_POSTMASTER, COMPAT_ORACLE_OPTIONS,
        	gettext_noop("Sets the ccurrent session maximum number of database links in use."),
        	NULL,
        	GUC_NO_SHOW_ALL | GUC_NOT_IN_SAMPLE
    	},
    	&ora_open_lins,
    	4, 1, 10,
    	NULL, NULL, NULL
	},
	/* End - ReqID:SRS-SQL-DBLINK */	
#endif
#if 0
	/* End-of-list marker */
	{
		{NULL, 0, 0, NULL, NULL}, NULL, 0, 0, 0, NULL, NULL, NULL
	}
};
#endif

#if 0
static struct config_string Ivy_ConfigureNamesString[] =
{
#endif
#ifdef IVY_GUC_STRING_PARAMS
	/* Begin - ReqID:LISTEN-MULTI-PORT */
	{
		{"ivorysql.listen_addresses", PGC_POSTMASTER, CONN_AUTH_SETTINGS,
			gettext_noop("Sets oracle host name or IP address(es) to listen to."),
			NULL,
			GUC_LIST_INPUT
		},
		&OraListenAddresses,
		"localhost",
		NULL, NULL, NULL
	},
	/* End - ReqID:LISTEN-MULTI-PORT */

	/* Begin - ReqID:SRS-GKC-DATATYPE_001 */
	{
		{"nls_date_format", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Compatible Oracle NLS parameter for date type."),
			NULL
		},
		&nls_date_format,
		"YYYY-MM-DD",
		NULL, NULL, NULL
	},

	{
		{"nls_timestamp_format", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Compatible Oracle NLS parameter for timestamp type."),
			NULL
		},
		&nls_timestamp_format,
		"YYYY-MM-DD HH24:MI:SS.FF6",
		NULL, NULL, NULL
	},

	{
		{"nls_timestamp_tz_format", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Compatible Oracle NLS parameter for timestamp with time zone type."),
			NULL
		},
		&nls_timestamp_tz_format,
		"YYYY-MM-DD HH24:MI:SS.FF6 TZH:TZM",
		NULL, NULL, NULL
	},

	/* Begin - ReqID:SRS-SQL-SYSFUNC */
	{
		{"nls_territory", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Compatible Oracle NLS parameter for NLS_TERRITORY."),
			NULL
		},
		&nls_territory,
		"AMERICA",
		nls_territory_check, NULL, NULL
	},

	{
		{"nls_currency", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Compatible Oracle NLS parameter for NLS_CURRENCY."),
			NULL
		},
		&nls_currency,
		"$",
		nls_length_check, NULL, NULL
	},

	{
		{"nls_iso_currency", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Compatible Oracle NLS parameter for NLS_ISO_CURRENCY."),
			NULL
		},
		&nls_iso_currency,
		"AMERICA",
		nls_territory_check, NULL, NULL
	},

	{
		{"nls_dual_currency", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Compatible Oracle NLS parameter for NLS_DUAL_CURRENCY."),
			NULL
		},
		&nls_dual_currency,
		"$",
		nls_length_check, NULL, NULL
	},

	{
		{"nls_numeric_characters", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Compatible Oracle NLS parameter for NLS_NUMERIC_CHARACTERS."     ),
			NULL
		},
		&nls_numeric_characters,
		".,",
		nls_numeric_characters_check, NULL, NULL
	},
	/* End - ReqID:SRS-SQL-SYSFUNC */
	
	/* Begin - SRS-SQL-NLSSORT */
	{
		{"nls_comp", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Compatible Oracle NLS parameter for NLS_COMP"),
			NULL
		},
		&nls_comp_str,
		"binary",
		nls_comp_check, nls_comp_assign, NULL
	},
	{
		{"nls_sort", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Compatible Oracle NLS parameter for NLS_SORT."),
			NULL
		},
		&nls_sort_str,
		"binary",
		nls_sort_check, nls_sort_assign, NULL
	},
	{
		{"default_collation", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Compatible Oracle Session parameter for DEFAULT_COLLATION."),
			NULL
		},
		&default_collation_str,
		"",
		default_collation_check, default_collation_assign, NULL
	},
	/* End - SRS-SQL-NLSSORT */
	/* Begin - ReqID:SRS-SQL-DBLINK */
	{
		{"ivorysql.default_dblink_fdw", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Compatible Oracle DBLINK using FDW name."),
			NULL,
			GUC_NO_SHOW_ALL | GUC_NOT_IN_SAMPLE
		},
		&default_dblink_fdw,
		"postgres_fdw",
		dblink_using_fdw_check, NULL, NULL
	},
	/* End - ReqID:SRS-SQL-DBLINK */
#endif
#if 0
	/* End-of-list marker */
	{
		{NULL, 0, 0, NULL, NULL}, NULL, NULL, NULL, NULL, NULL
	}
};
#endif

#if 0
static struct config_real Ivy_ConfigureNamesReal[] =
{
#endif
#ifdef IVY_GUC_REAL_PARAMS
	/* Add real guc param at here */

#endif
#if 0
	/* End-of-list marker */
	{
		{NULL, 0, 0, NULL, NULL}, NULL, 0.0, 0.0, 0.0, NULL, NULL, NULL
	}
};
#endif

#if 0
static struct config_enum Ivy_ConfigureNamesEnum[] =
{
#endif
#ifdef IVY_GUC_ENUM_PARAMS
	/* BEGIN - SQL PARSER */
	{
		{"ivorysql.database_mode", PGC_INTERNAL, PRESET_OPTIONS,
			gettext_noop("Set database mode"),
			NULL,
			GUC_NOT_IN_SAMPLE | GUC_DISALLOW_IN_FILE
		},
		&database_mode,
		DB_PG, db_mode_options,
		check_database_mode, NULL, NULL
	},

	{
		{"ivorysql.compatible_mode", PGC_USERSET, CLIENT_CONN_STATEMENT,
			gettext_noop("Set default sql parser compatibility mode"),
			NULL,
			GUC_NOT_IN_SAMPLE | GUC_DISALLOW_IN_FILE
		},
		&compatible_db,
		PG_PARSER, db_parser_options,
		check_compatible_mode, assign_compatible_mode, NULL
	},
	/* END - SQL PARSER */

	/* BEGIN - case sensitive indentify */
	{
		{"ivorysql.identifier_case_switch", PGC_USERSET, DEVELOPER_OPTIONS,
			gettext_noop("Set character case conversion mode."),
			NULL,
			GUC_NO_SHOW_ALL | GUC_NOT_IN_SAMPLE
		},
		&identifier_case_switch,
		INTERCHANGE,case_conversion_mode,
		NULL,NULL,NULL
	},
	/* END - case sensitive indentify */

	/* Begin - BUG#Z091 */
	{
		{"nls_length_semantics", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Set the length semantics of the character datatype."),
			NULL
		},
		&nls_length_semantics,
		NLS_LENGTH_BYTE, nls_length_options,
		check_nls_length_semantics, NULL, NULL
	},
	/* End - BUG#Z091 */

	/* Begin - ReqID:SRS-SQL-MERGE */
	{
		{"ivorysql.enable_ora_merge_tuning", PGC_USERSET, DEVELOPER_OPTIONS,
			gettext_noop("Enable the ora_merge tuning mode."),
			NULL,
			GUC_NO_SHOW_ALL | GUC_NOT_IN_SAMPLE
		},
		&enable_ora_merge_tuning,
		OMERGE_TUNING_OFF, ora_merge_tuning,
		NULL, NULL, NULL
	},
	/* End - ReqID:SRS-SQL-MERGE */

	/* Begin - BUG#M0000201: support question marks as binding variables */
	/* where the application is coming from */
	{
		{"ivorysql.where_from", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Set where the application comes from"),
			NULL,
			GUC_NOT_IN_SAMPLE | GUC_NO_SHOW_ALL
		},
		&where_from,
		FROM_UNKNOWN, where_from_options,
		NULL, NULL, NULL
	},
	/* End - BUG#M0000201: support question marks as binding variables */

	/* BEGIN - ReqID:SRS-MISC-USER */
	{
		{"ivorysql.dumppgtype", PGC_USERSET, CLIENT_CONN_STATEMENT,
			gettext_noop("Set pg type use oracle parser."),
			NULL
		},
		&dumppgtype,
		PG_DUMP, dump_pgtype_options,
		check_dump_pgtype, NULL, NULL
	},
	{
		{"ivorysql.grant_usertype", PGC_USERSET, CLIENT_CONN_STATEMENT,
			gettext_noop("Set user type use oracle parser."),
			NULL
		},
		&grant_usertype,
		PG_GRANTUSER, grant_usertype_options,
		check_grant_usertype, NULL, NULL
	},
	/* END - ReqID:SRS-MISC-USER */

	/* Begin - SRS-SQL-NLSSORT */
	{
		{"max_string_size", PGC_SU_BACKEND, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Compatible Oracle system parameter for MAX_STRING_SIZE."),
			NULL
		},
		&max_string_size,
		ORA_MAX_STRING_SIZE_EXTENDED, max_string_size_options,
		max_string_size_check, NULL, NULL
	},
	/* End - SRS-SQL-NLSSORT */
#endif
#if 0
	/* End-of-list marker */
	{
		{NULL, 0, 0, NULL, NULL}, NULL, 0, NULL, NULL, NULL, NULL
	}
};
#endif

#ifdef IVY_GUC_FUNC_DEFINE
/* Begin - ReqID:SRS-GKC-DATATYPE_001 */
/* Begin - BUG#Z091 */
/*
 * When the user initializes a PG-native cluster (-m pg), 
 * this GUC variable is meaningless, any attempt to use 
 * this GUC variable should throw a reasonable warning ,
 * perhaps a warning message is enough, since the error 
 * termination transaction, which is probably not what you want.
 */
static bool
check_nls_length_semantics(int *newval, void **extra, GucSource source)
{
	if (IsUnderPostmaster && DB_PG == database_mode)
		ereport(WARNING,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("Do not use this GUC variable in the current cluster (PG).")));
	return true;
}
/* End - BUG#Z091 */
/* End - ReqID:SRS-GKC-DATATYPE_001 */

static bool
check_compatible_mode(int *newval, void **extra, GucSource source)
{
	int		newmode = *newval;

	if (DB_PG == database_mode && newmode == DB_ORACLE)
			ereport(ERROR,
				(errcode(ERRCODE_CANT_CHANGE_RUNTIME_PARAM),
				errmsg("parameter ivorysql.compatible_mode cannot be changed in native PG mode.")));

	if(DB_ORACLE == database_mode
		&&  (IsNormalProcessingMode() || (IsUnderPostmaster && MyProcPort)))
	{
		if (newmode == DB_ORACLE)
		{
			if (ora_raw_parser == NULL)
				ereport(ERROR,
					(errcode(ERRCODE_SYSTEM_ERROR),
					errmsg("liboracle_parser not found!"),
					errhint("You must load liboracle_parser to use oracle parser.")));
			if (!ISLOADIVORYSQL_ORA)
				ereport(ERROR,
					(errcode(ERRCODE_SYSTEM_ERROR),
					errmsg("IVORYSQL_ORA library not found!"),
					errhint("You must load IVORYSQL_ORA to use oracle parser..")));
		}
	}
	return true;
}

static bool
check_database_mode(int *newval, void **extra, GucSource source)
{
	int		newmode = *newval;

	if (DB_PG == database_mode && DB_ORACLE == newmode)
			ereport(NOTICE,
				(errcode(ERRCODE_CANT_CHANGE_RUNTIME_PARAM),
				errmsg("parameter ivorysql.database_mode cannot be changed")));

	return true;
}

void
assign_compatible_mode(int newval, void *extra)
{
	if(DB_ORACLE == database_mode
		&&  (IsNormalProcessingMode() || (IsUnderPostmaster && MyProcPort)))
	{
		if (newval == DB_ORACLE)
		{
			sql_raw_parser = ora_raw_parser;

			/* Begin - ReqID:SRS-SQL-MERGE */
			pg_transform_merge_stmt_hook = ora_transform_merge_stmt_hook;
			pg_exec_merge_matched_hook = ora_exec_merge_matched_hook;
			/* End - ReqID:SRS-SQL-MERGE */

			/* Begin - ReqID:SRS-SQL-Object */
			pg_object_bulid_relcache = NULL;
			/* End - ReqID:SRS-SQL-Object */

			/* update the search_path */
			assign_search_path(NULL, NULL);
		}
		else if (newval == DB_PG)
		{
			sql_raw_parser = standard_raw_parser;

			/* Begin - ReqID:SRS-SQL-MERGE */
			pg_transform_merge_stmt_hook = transformMergeStmt;
			pg_exec_merge_matched_hook = ExecMergeMatched;
			/* End - ReqID:SRS-SQL-MERGE */

			/* Begin - ReqID:SRS-SQL-Object */
			pg_object_bulid_relcache = NULL;
			/* End - ReqID:SRS-SQL-Object */

			/* update the search_path */
			assign_search_path(NULL, NULL);
		}
	}
}

/* BEGIN - ReqID:SRS-MISC-USER */
static bool
check_dump_pgtype(int *newval, void **extra, GucSource source)
{
	int		newmode = *newval;

	if (DB_PG == database_mode && newmode == ORA_DUMP)
			ereport(ERROR,
				(errcode(ERRCODE_CANT_CHANGE_RUNTIME_PARAM),
				errmsg("parameter ivorysql.dumppgtype cannot be changed in native PG mode.")));

	if (PG_PARSER == compatible_db && newmode == ORA_DUMP)
			ereport(ERROR,
				(errcode(ERRCODE_CANT_CHANGE_RUNTIME_PARAM),
				errmsg("parameter ivorysql.dumppgtype cannot be changed in native PG parser.")));
	
	if(DB_ORACLE == database_mode && ORA_PARSER == compatible_db
		&&  (IsNormalProcessingMode() || (IsUnderPostmaster && MyProcPort)))
	{
		if (newmode == ORA_DUMP)
		{
			if (ora_raw_parser == NULL)
				ereport(ERROR,
					(errcode(ERRCODE_SYSTEM_ERROR),
					errmsg("liboracle_parser not found!"),
					errhint("You must load liboracle_parser to use oracle parser.")));
			if (!ISLOADIVORYSQL_ORA)
				ereport(ERROR,
					(errcode(ERRCODE_SYSTEM_ERROR),
					errmsg("IVORYSQL_ORA library not found!"),
					errhint("You must load IVORYSQL_ORA to use oracle parser..")));
		}
	}
	return true;
}

static bool
check_grant_usertype(int *newval, void **extra, GucSource source)
{
	int		newmode = *newval;

	if (DB_PG == database_mode && newmode == ORA_GRANTUSER)
			ereport(ERROR,
				(errcode(ERRCODE_CANT_CHANGE_RUNTIME_PARAM),
				errmsg("parameter ivorysql.dumppgtype cannot be changed in native PG mode.")));

	if (PG_PARSER == compatible_db && newmode == ORA_GRANTUSER)
			ereport(ERROR,
				(errcode(ERRCODE_CANT_CHANGE_RUNTIME_PARAM),
				errmsg("parameter ivorysql.dumppgtype cannot be changed in native PG parser.")));

	if(DB_ORACLE == database_mode && ORA_PARSER == compatible_db
		&&  (IsNormalProcessingMode() || (IsUnderPostmaster && MyProcPort)))
	{
		if (newmode == ORA_GRANTUSER)
		{
			if (ora_raw_parser == NULL)
				ereport(ERROR,
					(errcode(ERRCODE_SYSTEM_ERROR),
					errmsg("liboracle_parser not found!"),
					errhint("You must load liboracle_parser to use oracle parser.")));
			if (!ISLOADIVORYSQL_ORA)
				ereport(ERROR,
					(errcode(ERRCODE_SYSTEM_ERROR),
					errmsg("IVORYSQL_ORA library not found!"),
					errhint("You must load IVORYSQL_ORA to use oracle parser..")));
		}
	}
	return true;
}
/* END - ReqID:SRS-MISC-USER */


/* Begin - ReqID:SRS-SQL-SYSFUNC */
static void
nls_case_conversion(char **param, char type)
{
	char   *p;

CASE_CONVERSION:
	if (type == 'u')
	{
		for (p = *param; p < *param + strlen(*param); ++p)
			if (97 <= *p && *p <= 122)
				*p -= 32;
		*p = '\0';
	}
	else if (type == 'l')
	{
		for (p = *param; p < *param + strlen(*param); ++p)
			if (65 <= *p && *p <= 90)
				*p += 32;
		*p = '\0';
	}
	else if (type == 'b')
	{
		bool    has_upper = false,
				has_lower = false;

		for (p = *param; p < *param + strlen(*param); ++p)
		{
			if (65 <= *p && *p <= 90)
				has_upper = true;
			else if (97 <= *p && *p <= 122)
				has_lower = true;
			if (has_upper && has_lower)
				return;
		}

		if (has_upper && !has_lower)
		{
			type = 'l';
			goto CASE_CONVERSION;
		}
		else if (!has_upper && has_lower)
		{
			type = 'u';
			goto CASE_CONVERSION;
		}
	}
}

static bool
nls_length_check(char **newval, void **extra, GucSource source)
{
	if (DB_ORACLE == database_mode
			&&  (IsNormalProcessingMode() || (IsUnderPostmaster && MyProcPort)))
	{
		if (strlen(*newval) > 255)
			ereport(ERROR, (errmsg("parameter value longer than 255 characters")));
		else if (isdigit(**newval))
			ereport(ERROR, (errmsg("Cannot access NLS data files "
								   "or invalid environment specified")));
		else
			nls_case_conversion(newval, 'b');
	}

	return true;
}

static bool
nls_numeric_characters_check(char **newval, void **extra, GucSource source)
{
	if (DB_ORACLE == database_mode
			&&  (IsNormalProcessingMode() || (IsUnderPostmaster && MyProcPort)))
	{
		if (strlen(*newval) < 2)
			ereport(ERROR, (errmsg("Cannot access NLS data files "
								   "or invalid environment specified")));
		else if (strlen(*newval) > 255)
			ereport(ERROR, (errmsg("parameter value longer than 255 characters")));
		else if ((*newval)[0] == (*newval)[1])
			ereport(ERROR, (errmsg("Cannot access NLS data files "
								   "or invalid environment specified")));
		else
		{
			int i;
			for (i = 0; i < 2; ++i)
				if (isdigit((*newval)[i]) ||
					(*newval)[i] == '+' || (*newval)[i] == '-' ||
					(*newval)[i] == '<' || (*newval)[i] == '>' ||
					(*newval)[i] < 32 || (*newval)[i] > 126)
					ereport(ERROR, (errmsg("Cannot access NLS data files "
										   "or invalid environment specified")));
		}

		nls_case_conversion(newval, 'b');
	}

	return true;
}

static bool
nls_territory_check(char **newval, void **extra, GucSource source)
{
	if (DB_ORACLE == database_mode
			&&  (IsNormalProcessingMode() || (IsUnderPostmaster && MyProcPort)))
	{
		if (pg_strcasecmp(*newval, "CHINA") == 0)
			memcpy(*newval, "CHINA", 5);
		else if (pg_strcasecmp(*newval, "AMERICA") == 0)
			memcpy(*newval, "AMERICA", 7);
		else
			ereport(ERROR, (errmsg("Cannot access NLS data files "
								   "or invalid environment specified")));
	}

	return true;
}
/* End - ReqID:SRS-SQL-SYSFUNC */

/* Begin - SRS-SQL-NLSSORT */
static bool
nls_comp_check(char **newval, void **extra, GucSource source)
{
	return true;
}

static void
nls_comp_assign(const char *newval, void *extra)
{
	if (DB_ORACLE == database_mode && 
		(IsNormalProcessingMode() || (IsUnderPostmaster && MyProcPort)))
	{
		int i = 0;
		char *options[] = { "BINARY", "LINGUISTIC", "ANSI", NULL };

		for (i = 0; i < 3 && pg_strcasecmp(newval, options[i]); i++) {}

		if (i != 3)
			nls_comp_option = i;
		else
			ereport(ERROR, (errmsg("IVY-12705: Cannot access NLS data files or invalid environment specified")));
	}
}

static bool
nls_sort_check(char **newval, void **extra, GucSource source)
{
	if (DB_ORACLE == database_mode && 
	   (IsNormalProcessingMode() || (IsUnderPostmaster && MyProcPort)))
	{
		Oid coll = CollationGetCollid(*newval);
		if (!OidIsValid(coll) || 
			get_collation_provider(coll) != COLLPROVIDER_ORACLE ||
			IsPseudoCollation(coll))
		{
			ereport(ERROR, (errmsg("IVY-12705: Cannot access NLS data files or invalid environment specified")));
		}
	}

	return true;
}

static void
nls_sort_assign(const char *newval, void *extra)
{
	if (DB_ORACLE == database_mode && 
	   (IsNormalProcessingMode() || (IsUnderPostmaster && MyProcPort)))
	{
		nls_sort_oid = CollationGetCollid(newval);
	}
}

static bool
default_collation_check(char **newval, void **extra, GucSource source)
{
	if (DB_ORACLE == database_mode && 
		(IsNormalProcessingMode() || (IsUnderPostmaster && MyProcPort)))
	{
		Oid coll = InvalidOid;
		
		if (max_string_size != ORA_MAX_STRING_SIZE_EXTENDED)
			ereport(ERROR, (errmsg("IVY-43929: Collation cannot be specified if parameter MAX_STRING_SIZE=STANDARD is set.")));

		if (strlen(*newval))
		{
			coll = CollationGetCollid(*newval);
			if (!OidIsValid(coll) || get_collation_provider(coll) != COLLPROVIDER_ORACLE)
				ereport(ERROR, (errmsg("IVY-12705: Cannot access NLS data files or invalid environment specified")));
		}

		default_collation_oid = coll;
	}

	return true;
}

static void
default_collation_assign(const char *newval, void *extra)
{
	if (strlen(newval))
		default_collation_oid = CollationGetCollid(newval);
}

static bool
max_string_size_check(int *newval, void **extra, GucSource source)
{
	int new_val = *newval;

	if (new_val != ORA_MAX_STRING_SIZE_STANDARD && new_val != ORA_MAX_STRING_SIZE_EXTENDED)
		ereport(ERROR, (errmsg("IVY-00096: invalid value EXTENDDED for parameter max_string_size, must be from among EXTENDED, STANDARD")));
		
	if (max_string_size == ORA_MAX_STRING_SIZE_EXTENDED && new_val == ORA_MAX_STRING_SIZE_STANDARD)
		ereport(ERROR, (errmsg("cannot set MAX_STRING_SIZE from EXTENDED to STANDARD")));

	return true;
}
/* End - SRS-SQL-NLSSORT */

static bool
dblink_using_fdw_check(char **newval, void **extra, GucSource source)
{
   if (DB_ORACLE == database_mode)
   {
       if (pg_strcasecmp(*newval, "postgres_fdw") == 0)
           memcpy(*newval, "postgres_fdw", 12);
       else if (pg_strcasecmp(*newval, "oracle_fdw") == 0)
           memcpy(*newval, "oracle_fdw", 10);
       else
           ereport(ERROR, (errmsg("invalid fdw name specified")));
   }

   return true;
}
#endif
