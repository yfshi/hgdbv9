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
/* BEGIN - case sensitive indentify */
int				identifier_case_switch = INTERCHANGE;
bool  			identifier_case_from_pg_dump = false;
bool			enable_case_switch = true;
/* END - case sensitive indentify */

/* Begin - ReqID:SRS-GKC-DATATYPE_001 */
char	   *nls_length_semantics = NULL;
char	   *nls_date_format = "";
char	   *nls_timestamp_format = "";
char	   *nls_timestamp_tz_format = "";
int 		datetime_ignore_nls_mask = 0;
/* End - ReqID:SRS-GKC-DATATYPE_001 */

/* Begin - ReqID:EMPTY_STIRNG_TO_NULL */
bool			enable_emptystring_to_NULL = false;
/* End - ReqID:EMPTY_STIRNG_TO_NULL */

/* Begin - ReqID:SRS-SQL-INSERT */
bool		insert_enable_alias = false;
/* End - ReqID:SRS-SQL-INSERT */

/* BEGIN - SQL PARSER */
int	database_mode = DB_PG;
int	compatible_db = PG_PARSER;
/* END - SQL PARSER */
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
#endif

#ifdef IVY_GUC_FUNC_DECLARE
/* Begin - ReqID:SRS-GKC-DATATYPE_001 */
static bool check_nls_length_semantics(char **newval, void **extra, GucSource source);
/* End - ReqID:SRS-GKC-DATATYPE_001 */
static bool check_compatible_mode(int *newval, void **extra, GucSource source);
static bool check_database_mode(int *newval, void **extra, GucSource source);
void assign_compatible_mode(int newval, void *extra);
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
		{"nls_length_semantics", PGC_USERSET, COMPAT_ORACLE_OPTIONS,
			gettext_noop("Compatible Oracle NLS parameter for charater data type."),
			gettext_noop("Valid values are CHAR, BYTE."),
			GUC_IS_NAME
		},
		&nls_length_semantics,
		"byte",
		check_nls_length_semantics, NULL, NULL
	},

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
static bool
check_nls_length_semantics(char **newval, void **extra, GucSource source)
{
	if (newval == NULL
			|| (newval != NULL && pg_strcasecmp(*newval, "CHAR")  && pg_strcasecmp(*newval, "BYTE")))
		return false;

	return true;
}
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
			assign_search_path(NULL, NULL);
		}
		else if (newval == DB_PG)
		{
			sql_raw_parser = standard_raw_parser;

			/* Begin - ReqID:SRS-SQL-MERGE */
			pg_transform_merge_stmt_hook = transformMergeStmt;
			pg_exec_merge_matched_hook = ExecMergeMatched;
			/* End - ReqID:SRS-SQL-MERGE */
			assign_search_path(NULL, NULL);
		}
	}
}
#endif
