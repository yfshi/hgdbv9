/*-------------------------------------------------------------------------
 *
 * ivy_mysql_guc.c
 *
 * Support for grand unified configuration schema(for mysql compatible), including SET
 * command, configuration file, and command line options
 *
 * Authored by jinhuajian@highgo.com, 20230510.
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *        src/backend/utils/misc/ivy_mysql_guc.c
 *
 * add the file for requirement "SRS-MISC-MYSQL-FRAME", include
 * IVY_MY_GUC_VAR_DEFINE, IVY_MY_GUC_INT_PARAMS and IVY_MY_GUC_STRING_PARAMS
 *-------------------------------------pg_resetwal.c------------------------------------
 */
#ifdef IVY_MY_GUC_FUNC_DEFINE
	static void assign_sql_mode(const char *newval, void *extra);
	static bool check_sql_mode(char **newval, void **extra, GucSource source);
	static const char *show_sql_mode(void);
	/* Begin - ReqID:SRS-MYSQL-CASE */
	bool check_lower_case_table_names(int *newval, void **extra, GucSource source);
	void assign_lower_case_table_names(int newval, void *extra);
	/* End - ReqID:SRS-MYSQL-CASE */
#endif

#ifdef IVY_MY_GUC_VAR_DEFINE
/* Begin - ReqID:SRS-MYSQL-UPDATE */
bool		enable_indexhint = false;
/* End - ReqID:SRS-MYSQL-UPDATE */

/* Begin - ReqID:SRS-MISC-MYSQL-FRAME */
sql_mode_t  sql_mode;
static char *sql_mode_str = NULL;
/* End - ReqID:SRS-MISC-MYSQL-FRAME */

/* Begin - BUG#Z1434 */
bool		enable_ignore_columns_list = false;
/* End - BUG#Z1434 */
int			lower_case_table_names = 0;		/* ReqID:SRS-MYSQL-CASE */
bool		myenable_case_switch = true;	/* ReqID:SRS-MYSQL-CASE */
bool		not_mode_pad_char_to_full = true;	/* ReqID: SRS-MYSQL-CHARACTERTYPE-CHAR(n) */
int			div_prec_increment = 4;			/* ReqID: SRS-MYSQL-NUMBERICTYPE */

/* Begin - ReqID:SRS-MYSQL-CASE */
#define MIN_LOWER_NUM 0
#define MAX_LOWER_NUM 2
#ifdef WIN32
	#define DEF_LOWER_NUM 1
#else
	#define DEF_LOWER_NUM 0
#endif
/* End - ReqID:SRS-MYSQL-CASE */

#endif
#ifdef IVY_MY_GUC_BOOL_PARAMS

	/* Begin - ReqID:SRS-MYSQL-UPDATE */
	{
		{"mysql.enable_indexhint", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("whether enable index hint feature in MySQL compatible mode."),
			NULL
		},
		&enable_indexhint,
		false,
		NULL, NULL, NULL
	},
	/* End - ReqID:SRS-MYSQL-UPDATE */
	/* Begin - BUG#Z1434 */
	{
		{"mysql.enable_ignore_columns_list", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("whether enable ignore cloumns list feature in MySQL compatible mode."),
			NULL
		},
		&enable_ignore_columns_list,
		false,
		NULL, NULL, NULL
	},
	/* End - BUG#Z1434 */
	/* Begin - ReqID:SRS-MYSQL-CASE */
	{
		{"mysql.enable_case_switch", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("whether enable case conversion feature in oracle compatible mode."),
			NULL
		},
		&myenable_case_switch,
		true,
		NULL, NULL, NULL
	},
	/* End - ReqID:SRS-MYSQL-CASE */
	/* Begin - ReqID: SRS-MYSQL-CHARACTERTYPE-CHAR(n) */
	{
		{"mysql.mode_pad_char_to_full", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("mode_pad_char_to_full_length control char(n) output."),
			NULL
		},
		&not_mode_pad_char_to_full,
		true,
		NULL, NULL, NULL
	},
	/* End - ReqID: SRS-MYSQL-CHARACTERTYPE-CHAR(n) */
#endif

#ifdef IVY_MY_GUC_INT_PARAMS

	/* Begin - ReqID:SRS-MISC-MYSQL-FRAME */
	{
		{"mysql.port", PGC_POSTMASTER, CONN_AUTH_SETTINGS,
			gettext_noop("Sets the MySQL TCP port the server listens on."),
			NULL
		},
		&MysPortNumber,
		3306, 1, 65535,
		NULL, NULL, NULL
	},
	/* End - ReqID:SRS-MISC-MYSQL-FRAME */
	/* Begin - ReqID:SRS-MYSQL-CASE */
	{
		{"lower_case_table_names", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("MySQL case sensitive parameter."),
			NULL
		},
		&lower_case_table_names,
		DEF_LOWER_NUM, MIN_LOWER_NUM, MAX_LOWER_NUM,
		check_lower_case_table_names, assign_lower_case_table_names, NULL
	},
	/* End - ReqID:SRS-MYSQL-CASE */
	/* Begin - ReqID: SRS-MYSQL-NUMBERICTYPE */
	{
		{"div_prec_increment", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("Use mysql code in datatype function."),
			NULL
		},
		&div_prec_increment,
		4, 0, 30,
		NULL, NULL, NULL
	},
	/* End - ReqID: SRS-MYSQL-NUMBERICTYPE */
#endif

#ifdef IVY_MY_GUC_STRING_PARAMS

	/* Begin - ReqID:SRS-MISC-MYSQL-FRAME */
	{
		{"mysql.listen_addresses", PGC_POSTMASTER, CONN_AUTH_SETTINGS,
			gettext_noop("Sets MySQL host name or IP address(es) to listen to."),
			NULL,
			GUC_LIST_INPUT
		},
		&MysListenAddresses,
		"localhost",
		NULL, NULL, NULL
	},
	{
		{
			"sql_mode",
			PGC_USERSET,
			COMPAT_MYSQL_OPTIONS,
			gettext_noop("Sets mysql sql_mode."),
			NULL,
			GUC_LIST_INPUT
		},
		&sql_mode_str,
		"ANSI_QUOTES,ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,"
		"ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION,REAL_AS_FLOAT",
		check_sql_mode, assign_sql_mode, show_sql_mode
	},
#endif

#ifdef IVY_MY_GUC_FUNC_DEFINE

#define REPORT_SQL_MODE_VALUE_ERROR(val) do { \
	ereport(ERROR, \
			(errcode(ERRCODE_SYSTEM_ERROR), \
			errmsg("Variable 'sql_mode' can't be set to the value of '%s'", val))); \
} while(0);

static const char *sql_mode_names[] =
{
	"REAL_AS_FLOAT",
  	"PIPES_AS_CONCAT",
  	"ANSI_QUOTES",
  	"IGNORE_SPACE",
  	",",
  	"ONLY_FULL_GROUP_BY",
  	"NO_UNSIGNED_SUBTRACTION",
  	"NO_DIR_IN_CREATE",
  	"POSTGRESQL",
  	"ORACLE",
  	"MSSQL",
  	"DB2",
  	"MAXDB",
  	"NO_KEY_OPTIONS",
  	"NO_TABLE_OPTIONS",
  	"NO_FIELD_OPTIONS",
  	"MYSQL323",
  	"MYSQL40",
  	"ANSI",
  	"NO_AUTO_VALUE_ON_ZERO",
  	"NO_BACKSLASH_ESCAPES",
  	"STRICT_TRANS_TABLES",
  	"STRICT_ALL_TABLES",
  	"NO_ZERO_IN_DATE",
  	"NO_ZERO_DATE",
  	"ALLOW_INVALID_DATES",
  	"ERROR_FOR_DIVISION_BY_ZERO",
  	"TRADITIONAL",
  	"NO_AUTO_CREATE_USER",
  	"HIGH_NOT_PRECEDENCE",
  	"NO_ENGINE_SUBSTITUTION",
  	"PAD_CHAR_TO_FULL_LENGTH",
  	0
};

static void
check_sub_modes_of_strict_mode(sql_mode_t sql_mode)
{
	const sql_mode_t strict_modes = MODE_STRICT_TRANS_TABLES |
									MODE_STRICT_ALL_TABLES;

	const sql_mode_t new_strict_submodes = MODE_NO_ZERO_IN_DATE |
										   MODE_NO_ZERO_DATE 	|
										   MODE_ERROR_FOR_DIVISION_BY_ZERO;

	const sql_mode_t strict_modes_set = sql_mode & strict_modes;
	const sql_mode_t new_strict_submodes_set = sql_mode & new_strict_submodes;

	if (((strict_modes_set | new_strict_submodes_set) != 0) &&
		((new_strict_submodes_set != new_strict_submodes) || (strict_modes_set == 0)))
	{
		ereport(WARNING,
				(errcode(ERRCODE_WARNING),
				 errmsg("'NO_ZERO_DATE', 'NO_ZERO_IN_DATE' and 'ERROR_FOR_DIVISION_BY_ZERO' sql modes "
				 		"should be used with strict mode. They will be merged with strict mode in a future release.")));
	}
}

static sql_mode_t
expand_sql_mode(sql_mode_t sql_mode)
{
	if (sql_mode & MODE_ANSI)
	{
		sql_mode |= (MODE_REAL_AS_FLOAT 	| MODE_PIPES_AS_CONCAT 	|
					 MODE_ANSI_QUOTES 		| MODE_IGNORE_SPACE 	|
					 MODE_ONLY_FULL_GROUP_BY);
	}

	if (sql_mode & MODE_ORACLE)
	{
		sql_mode |= (MODE_PIPES_AS_CONCAT 	| MODE_ANSI_QUOTES 		|
					 MODE_IGNORE_SPACE 		| MODE_NO_KEY_OPTIONS 	|
					 MODE_NO_TABLE_OPTIONS 	| MODE_NO_FIELD_OPTIONS |
					 MODE_NO_AUTO_CREATE_USER);
	}

	if (sql_mode & MODE_MSSQL)
	{
		sql_mode |= (MODE_PIPES_AS_CONCAT 	| MODE_ANSI_QUOTES 		|
					 MODE_IGNORE_SPACE 		| MODE_NO_KEY_OPTIONS 	|
					 MODE_NO_TABLE_OPTIONS 	| MODE_NO_FIELD_OPTIONS);
	}

	if (sql_mode & MODE_POSTGRESQL)
	{
		sql_mode |= (MODE_PIPES_AS_CONCAT 	| MODE_ANSI_QUOTES 		|
					 MODE_IGNORE_SPACE 		| MODE_NO_KEY_OPTIONS 	|
					 MODE_NO_TABLE_OPTIONS 	| MODE_NO_FIELD_OPTIONS);
	}

	if (sql_mode & MODE_DB2)
	{
		sql_mode |= (MODE_PIPES_AS_CONCAT 	| MODE_ANSI_QUOTES 		|
					 MODE_IGNORE_SPACE 		| MODE_NO_KEY_OPTIONS 	|
					 MODE_NO_TABLE_OPTIONS 	| MODE_NO_FIELD_OPTIONS);
	}

	if (sql_mode & MODE_MAXDB)
	{
		sql_mode |= (MODE_PIPES_AS_CONCAT 	| MODE_ANSI_QUOTES 		|
					 MODE_IGNORE_SPACE 		| MODE_NO_KEY_OPTIONS 	|
					 MODE_NO_TABLE_OPTIONS 	| MODE_NO_FIELD_OPTIONS |
					 MODE_NO_AUTO_CREATE_USER);
	}

	if (sql_mode & MODE_MYSQL40)
		sql_mode |= MODE_HIGH_NOT_PRECEDENCE;

	if (sql_mode & MODE_MYSQL323)
		sql_mode |= MODE_HIGH_NOT_PRECEDENCE;

	if (sql_mode & MODE_TRADITIONAL)
	{
		sql_mode |= (MODE_STRICT_TRANS_TABLES 		 | MODE_STRICT_ALL_TABLES 	|
					 MODE_NO_ZERO_IN_DATE 			 | MODE_NO_ZERO_DATE 		|
					 MODE_ERROR_FOR_DIVISION_BY_ZERO | MODE_NO_AUTO_CREATE_USER |
					 MODE_NO_ENGINE_SUBSTITUTION);
	}

	check_sub_modes_of_strict_mode(sql_mode);

	return sql_mode;
}

static sql_mode_t
trans_sql_mode(const char *val)
{
	List *inputs = NULL;
	ListCell *it = NULL;

	sql_mode_t cur;
	sql_mode_t sql_mode = 0;

	const char **name;
	char *rawstring = pstrdup(val);

	if (!SplitIdentifierString(rawstring, ',', &inputs))
		REPORT_SQL_MODE_VALUE_ERROR(rawstring);

	foreach (it, inputs)
	{
		char *input = (char *) lfirst(it);

		for (cur = MODE_VALUES_FIRST, name = sql_mode_names;
			*name != NULL; cur = NEXT_MODE_VALUE(cur), name++)
		{
			if (!pg_strcasecmp(*name, input))
				break;
		}

		// illegal value
		if (*name == NULL)
			REPORT_SQL_MODE_VALUE_ERROR(input);

		sql_mode |= cur;
	}

	return expand_sql_mode(sql_mode);
}

static void
sql_mode_to_cstring(sql_mode_t sql_mode, StringInfo str)
{
	sql_mode_t cur;
	const char **name;

	for(cur = MODE_VALUES_FIRST, name = sql_mode_names;
		*name != NULL; cur = NEXT_MODE_VALUE(cur), name++)
	{
		if (sql_mode & cur)
		{
			if (str->len)
				appendStringInfoChar(str, ',');

			appendStringInfoString(str, *name);
		}
	}
}

static void
assign_sql_mode(const char *newval, void *extra)
{
	sql_mode_t deprecated_modes;
	sql_mode_t old_sql_mode = sql_mode;
	sql_mode_t new_sql_mode = trans_sql_mode(newval);
	sql_mode_t deprecated_mask = MODE_DB2 | MODE_MAXDB | MODE_MSSQL | MODE_MYSQL323 |
								 MODE_MYSQL40 | MODE_ORACLE | MODE_POSTGRESQL |
								 MODE_NO_FIELD_OPTIONS | MODE_NO_KEY_OPTIONS | MODE_NO_TABLE_OPTIONS;

	sql_mode = new_sql_mode;

	if (sql_mode_str == NULL)
		return;

	if ((old_sql_mode ^ new_sql_mode) & MODE_NO_AUTO_CREATE_USER)
	{
		ereport(WARNING,
			(errcode(ERRCODE_WARNING),
			 errmsg("Changing sql mode 'NO_AUTO_CREATE_USER' is deprecated. It will be removed in a future release.")));
	}

	deprecated_modes = new_sql_mode & deprecated_mask;

	if (deprecated_modes != 0)
	{
		StringInfo str = makeStringInfo();
		sql_mode_to_cstring(deprecated_modes, str);

		ereport(WARNING,
				(errcode(ERRCODE_WARNING),
				 errmsg("Changing sql mode '%s' is deprecated. It will be removed in a future release.", str->data)));
	}
}

static bool
check_sql_mode(char **newval, void **extra, GucSource source)
{
	char *it = *newval;
	while (*it)
	{
		if (isspace(*it))
			REPORT_SQL_MODE_VALUE_ERROR(*newval);

		it++;
	}

	return true;
}

static const char *
show_sql_mode(void)
{
	StringInfo str = makeStringInfo();

	sql_mode_to_cstring(sql_mode, str);

	return str->data;
}
/* End - ReqID:SRS-MISC-MYSQL-FRAME */

/* Begin - ReqID:SRS-MYSQL-CASE */
/*
 * check_lower_case_table_names
 *   guc parameter value calculation
 *
 * Different environments have different values.
 *
 */
bool
check_lower_case_table_names(int *newval, void **extra, GucSource source)
{
#ifdef WIN32
	*newval = *newval;
#else
	*newval = *newval % 2;
#endif

	return true;
}

/*
 * assign_lower_case_table_names
 *   assign value for guc parameter.
 *
 * Assign a value to lower_case_table_names.
 *
 */
void
assign_lower_case_table_names(int newval, void *extra)
{
	lower_case_table_names = newval;
}
/* End - ReqID:SRS-MYSQL-CASE */
#endif
