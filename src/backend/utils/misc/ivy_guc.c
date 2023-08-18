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

/* Begin - ReqID:EMPTY_STIRNG_TO_NULL */
bool			enable_emptystring_to_NULL = false;
/* End - ReqID:EMPTY_STIRNG_TO_NULL */

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

bool	sort_desc_nulls_last = false;	/* BUG#M0000197 */

bool	cast_int_like_oracle = false;	/* BUG#M0000194 */
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
	{"0", FROM_UNKNOWN, false},
	{"1", FROM_JDBC, false},
	{"2", FROM_C, false},
	{"3", FROM_OCI, false},
	{"4", FROM_LIBPQ, false},
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
/* End - ReqID:SRS-SQL-SYSFUNC */
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
			/* update the search_path */
			assign_search_path(NULL, NULL);
		}
	}
}

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
#endif
