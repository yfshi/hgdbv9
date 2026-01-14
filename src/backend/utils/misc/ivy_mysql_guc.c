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
	/* Begin - qID: SRS-MYSQL-NUMBERICTYPE */
	bool check_div_precision_increment(int *newval, void **extra, GucSource source);
	void assign_div_precision_increment(int newval, void *extra);
	/* End - qID: SRS-MYSQL-NUMBERICTYPE */
	/* Begin - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
	static bool check_auto_increment_increment(int *newval, void **extra, GucSource source);
	static void assign_auto_increment_increment(int newval, void *extra);
	static bool check_auto_increment_offset(int *newval, void **extra, GucSource source);
	static void assign_auto_increment_offset(int newval, void *extra);
	/* End - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
	static void assign_regress_test_default_value(bool newval, void *extra);
	/* Begin - ReqID:SRS-MYSQL-BUILDINFUNCTION */
	static bool check_lc_time_names(char **target, void **extra, GucSource source);
	static int lc_time_names_Search(const char *target);
	static bool check_default_week_format(int *newval, void **extra, GucSource source);
	static void assign_default_week_format(int newval, void *extra);
/* End - ReqID:SRS-MYSQL-BUILDINFUNCTION */
#endif

#ifdef IVY_MY_GUC_VAR_DEFINE
/* Begin - Req: SRS-MYSQL-PROTOCOL */
static const struct config_enum_entry proto_type_options[] = {
	{"pg", PROTOCOL_PG, false},
	{"mysql", PROTOCOL_MYSQL, false},
	{NULL, 0, false}
};

static const struct config_enum_entry encrytion_type_options[] = {
	{"aes-128-ecb", mys_aes_128_ecb, false},
	{"aes-192-ecb", mys_aes_192_ecb, false},
	{"aes-256-ecb", mys_aes_256_ecb, false},

	{"aes-128-cbc", mys_aes_128_cbc, false},
	{"aes-192-cbc", mys_aes_192_cbc, false},
	{"aes-256-cbc", mys_aes_256_cbc, false},

	{"aes-128-cfb1", mys_aes_128_cfb1, false},
	{"aes-192-cfb1", mys_aes_128_cfb1, false},
	{"aes-256-cfb1", mys_aes_128_cfb1, false},

	{"aes-128-cfb8", mys_aes_128_cfb8, false},
	{"aes-192-cfb8", mys_aes_192_cfb8, false},
	{"aes-256-cfb8", mys_aes_256_cfb8, false},

	{"aes-128-cfb128", mys_aes_128_cfb128, false},
	{"aes-192-cfb128", mys_aes_192_cfb128, false},
	{"aes-256-cfb128", mys_aes_256_cfb128, false},

	{"aes-128-ofb", mys_aes_128_ofb, false},
	{"aes-192-ofb", mys_aes_192_ofb, false},
	{"aes-256-ofb", mys_aes_256_ofb, false},
	{NULL, 0, false}
};

int 	mys_port;
char 	*mys_listen_address;
int	 	mys_protocol_type;
int		mys_block_encryption_mode;
char 	*default_database;
char    *sha256_password_public_key_path;
char    *sha256_password_private_key_path;
char 	*version_comment_str;
int     net_buffer_length;
int 	wait_timeout;
int 	interactive_timeout;
int 	net_write_timeout;
int     sql_select_limit;

char 	*character_set_client;
char 	*character_set_connection;
char 	*character_set_results;
char 	*character_set_server;
char 	*collation_server;
char 	*collation_connection;
char 	*init_connect;
char 	*license;
char 	*performance_schema;
char 	*system_time_zone;
char 	*time_zone;
bool	mys_autocommit = true;
/* End - Req: SRS-MYSQL-PROTOCOL */

/* Begin - ReqID:SRS-MYSQL-UPDATE */
bool		enable_indexhint = false;
/* End - ReqID:SRS-MYSQL-UPDATE */

/* Begin - ReqID:SRS-MISC-MYSQL-FRAME */
sql_mode_t  sql_mode;
static char *sql_mode_str = NULL;
/* End - ReqID:SRS-MISC-MYSQL-FRAME */
/* Begin - ReqID:SRS-MYSQL-BINARYTYPE */
int			max_allowed_packet = 4194304;
int			max_sort_length = 1024;
/* End - ReqID:SRS-MYSQL-BINARYTYPE */
/* Begin - BUG#Z1434 */
bool		enable_ignore_columns_list = false;
/* End - BUG#Z1434 */
bool		mysql_convert = false;		/* ReqID: SRS-MYSQL-CHARACTERTYPE-CONVERT */
int			lower_case_table_names = 0;		/* ReqID:SRS-MYSQL-CASE */
bool		myenable_case_switch = true;	/* ReqID:SRS-MYSQL-CASE */
bool		not_mode_pad_char_to_full = true;	/* ReqID: SRS-MYSQL-CHARACTERTYPE-CHAR(n) */
int			div_precision_increment = 4;			/* ReqID: SRS-MYSQL-NUMBERICTYPE */
int			auto_increment_increment = 1;			/* SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
int			auto_increment_offset = 1;				/* SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
/* Begin - BUG#M533 */
bool		regress_test_default_value = false;
bool		cast_to_char = true;
/* End - BUG#M533 */

bool		enable_mysql_information_schema = false;

/* Begin - ReqID:SRS-MYSQL-BUILDINFUNCTION */
char	   *lc_time_names = NULL;
int			default_week_format = 0;
/* End - ReqID:SRS-MYSQL-BUILDINFUNCTION */

bool		at_or_atat = false;		/* ReqID:SRS-MYSQL-SET */
bool		sql_quote_show_create = true;	/* ReqID:SRS-MYSQL-SHOW */
bool		use_mysql_do = false;	/* ReqID:SRS-MYSQL-DO */
bool		use_mysql_bit_literal = false;	/* SRS-MYSQL-HEX-LITERAL */

/* Begin - ReqID:SRS-MYSQL-CASE */
#define MIN_LOWER_NUM 0
#define MAX_LOWER_NUM 2
#ifdef WIN32
	#define DEF_LOWER_NUM 1
#else
	#define DEF_LOWER_NUM 0
#endif
/* End - ReqID:SRS-MYSQL-CASE */
/* Begin - ReqID: SRS-MYSQL-NUMBERICTYPE */
#define DIV_PREC_MAX_VALUE 30
#define DIV_PREC_MIN_VALUE 4
/* End - ReqID: SRS-MYSQL-NUMBERICTYPE */
/* Begin - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
#define AUTO_INCREMENT_MAX_VALUE 65535
/* End - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
/* Begin - ReqID:SRS-MYSQL-BUILDINFUNCTION */
#define DEF_WEEK_FORMAT 0
#define MAX_WEEK_FORMAT 7
/* End - ReqID:SRS-MYSQL-BUILDINFUNCTION */

#endif

#ifdef IVY_MY_GUC_BOOL_PARAMS
	/* Begin - ReqID:SRS-MYSQL-UPDATE */
	{
		{"mysql.enable_indexhint", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("whether enable index hint feature in MySQL compatible mode."),
			NULL,
			GUC_NOT_IN_SAMPLE | GUC_NO_SHOW_ALL
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
			NULL,
			GUC_NOT_IN_SAMPLE | GUC_NO_SHOW_ALL
		},
		&enable_ignore_columns_list,
		false,
		NULL, NULL, NULL
	},
	/* End - BUG#Z1434 */
	{
		{"mysql.enable_mysql_information_schema", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("whether enable information schema feature in mysql compatible mode."),
			NULL,
			GUC_NOT_IN_SAMPLE
		},
		&enable_mysql_information_schema,
		false,
		NULL, NULL, NULL
	},
	/* Begin - ReqID:SRS-MYSQL-CASE */
	{
		{"mysql.enable_case_switch", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("whether enable case conversion feature in oracle compatible mode."),
			NULL,
			GUC_NOT_IN_SAMPLE
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
			NULL,
			GUC_NOT_IN_SAMPLE
		},
		&not_mode_pad_char_to_full,
		true,
		NULL, NULL, NULL
	},
	/* End - ReqID: SRS-MYSQL-CHARACTERTYPE-CHAR(n) */
	/* Begin - ReqID: SRS-MYSQL-CHARACTERTYPE-CONVERT */
	{
		{"mysql.mysql_convert", PGC_USERSET, COMPAT_OPTIONS_PREVIOUS,
			gettext_noop("mysql and pg convert."),
			NULL,
			GUC_NOT_IN_SAMPLE | GUC_NO_SHOW_ALL
		},
		&mysql_convert,
		true,	/* BUG#Z5124 */
		NULL, NULL, NULL
	},
	/* End - ReqID: SRS-MYSQL-CHARACTERTYPE-CONVERT */
	/* Begin - BUG#M533 */
	{
		// Only for regress test
		{"mysql.regress_test_default_value", PGC_POSTMASTER, COMPAT_MYSQL_OPTIONS,
			gettext_noop("Controlling the functional difference between MySQL and Postgres."),
			NULL,
			GUC_NOT_IN_SAMPLE | GUC_NO_SHOW_ALL
		},
		&regress_test_default_value,
		false,
		NULL, assign_regress_test_default_value, NULL
	},
	{
		{"mysql.cast_to_char", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("MySQL and Postgres CAST(expr as char) is difference."),
			NULL,
			GUC_NOT_IN_SAMPLE
		},
		&cast_to_char,
		true,
		NULL, NULL, NULL
	},
	/* End - BUG#M533 */
	/* Begin - ReqID:SRS-MYSQL-SET */
	{
		{"mysql.at_or_atat", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("MySQL user/global/session variable switch."),
			NULL,
			GUC_NOT_IN_SAMPLE
		},
		&at_or_atat,
		false,
		NULL, NULL, NULL
	},
	/* End - ReqID:SRS-MYSQL-SET */
	/* Begin - ReqID:SRS-MYSQL-SHOW */
	{
		{"sql_quote_show_create", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
				gettext_noop("MySQL Parameter show create command."),
				NULL,
				GUC_NOT_IN_SAMPLE
		},
		&sql_quote_show_create,
		true,
		NULL, NULL, NULL
	},
	/* End - ReqID:SRS-MYSQL-SHOW */
	/* Begin - ReqID:SRS-MYSQL-DO */
	{
		{"mysql.use_mysql_do", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
				gettext_noop("MySQL Parameter use MySQL DO command."),
				NULL,
				GUC_NOT_IN_SAMPLE
		},
		&use_mysql_do,
		false,
		NULL, NULL, NULL
	},
	/* End - ReqID:SRS-MYSQL-DO */
	/* Begin - ReqID:SRS-MYSQL-PROTOCOL */
	{
		{"autocommit", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
				gettext_noop("This parameter is only used in mysql compatibility."),
				NULL,
				GUC_NOT_IN_SAMPLE
		},
		&mys_autocommit,
		true,
		NULL, NULL, NULL
	},
	/* End - ReqID:SRS-MYSQL-PROTOCOL */
	/* Begin - SRS-MYSQL-HEX-LITERAL */
	{
		{"mysql.use_mysql_bit_literal", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("Use bit-value literal with Mysql features in mysql compatible mode."),
			NULL,
			GUC_NOT_IN_SAMPLE | GUC_NO_SHOW_ALL
		},
		&use_mysql_bit_literal,
		false,
		NULL, NULL, NULL
	},
	/* End - SRS-MYSQL-HEX-LITERAL */
#endif

#ifdef IVY_MY_GUC_INT_PARAMS
	/* Begin - ReqID:SRS-MISC-MYSQL-FRAME */
	{
		{"mysql.port", PGC_POSTMASTER, CONN_AUTH_SETTINGS,
			gettext_noop("Sets the MySQL TCP port the server listens on with pg protocol."),
			NULL
		},
		&mys_port,
		3306, 1, 65535,
		NULL, NULL, NULL
	},
	/* End - ReqID:SRS-MISC-MYSQL-FRAME */
	/* Begin - ReqID:SRS-MYSQL-CASE */
	{
		{"lower_case_table_names", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("MySQL case sensitive parameter."),
			NULL,
			GUC_NOT_IN_SAMPLE
		},
		&lower_case_table_names,
		DEF_LOWER_NUM, MIN_LOWER_NUM, MAX_LOWER_NUM,
		check_lower_case_table_names, assign_lower_case_table_names, NULL
	},
	/* End - ReqID:SRS-MYSQL-CASE */
	/* Begin - ReqID: SRS-MYSQL-NUMBERICTYPE */
	{
		{"div_precision_increment", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("Use mysql code in datatype function."),
			NULL
		},
		&div_precision_increment,
		4, INT_MIN, INT_MAX,	/* BUG#5164 */
		check_div_precision_increment, assign_div_precision_increment, NULL
	},
	/* End - ReqID: SRS-MYSQL-NUMBERICTYPE */
	/* Begin - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
	{
		{"auto_increment_increment", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("Use mysql code in datatype function."),
			NULL,
			GUC_NOT_IN_SAMPLE
		},
		&auto_increment_increment,
		1, INT_MIN, INT_MAX,
		check_auto_increment_increment, assign_auto_increment_increment, NULL
	},
	{
		{"auto_increment_offset", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("Use mysql code in datatype function."),
			NULL,
			GUC_NOT_IN_SAMPLE
		},
		&auto_increment_offset,
		1, INT_MIN, INT_MAX,
		check_auto_increment_offset, assign_auto_increment_offset, NULL
	},
	/* End - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
	/* Begin - ReqID:SRS-MYSQL-BUILDINFUNCTION */
	{
		{"default_week_format", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("MySQL default_week_format parameter."),
			NULL
		},
		&default_week_format,
		DEF_WEEK_FORMAT, PG_INT32_MIN, PG_INT32_MAX,
		check_default_week_format, assign_default_week_format, NULL
	},
	/* End - ReqID:SRS-MYSQL-BUILDINFUNCTION */
	/* Begin - ReqID:SRS-MYSQL-BINARYTYPE */
	{
		{
			"max_allowed_packet", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("Sets mysql max_allowed_packet ."),
			NULL,
			GUC_NOT_IN_SAMPLE
		},
		&max_allowed_packet,
		4194304, 1024, 1073741824,
		NULL, NULL, NULL
	},
	{
		{
			"max_sort_length", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("Sets mysql max_sort_length."),
			NULL,
			GUC_NOT_IN_SAMPLE
		},
		&max_sort_length,
		1024, 4, 8388608,
		NULL, NULL, NULL
	},
	/* End - ReqID:SRS-MYSQL-BINARYTYPE */
	/* Begin - ReqID:SRS-MYSQL-PROTOCOL */
	#define DEFAULT_NET_BUFFER_LENGTH   16384
    #define MIN_NET_BUFFER_LENGTH       1024
    #define MAX_NET_BUFFER_LENGTH       1048576
	{
		{
			"net_buffer_length", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("The default size of the buffer."),
			NULL,
			GUC_NOT_IN_SAMPLE
		},
		&net_buffer_length,
		DEFAULT_NET_BUFFER_LENGTH,
        MIN_NET_BUFFER_LENGTH,
        MAX_NET_BUFFER_LENGTH,
		NULL, NULL, NULL
	},
	#define DEFAULT_WAIT_TIMEOUT 	28800
	#define MIN_WAIT_TIMEOUT 		1
	#define MAX_WAIT_TIMEOUT 		31536000
	{
		{
			"wait_timeout", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("After this number of seconds of inactivity, "
                         "the server will close the noninteractive connection."),
			NULL,
			GUC_NOT_IN_SAMPLE
		},
		&wait_timeout,
		DEFAULT_WAIT_TIMEOUT,
		MIN_WAIT_TIMEOUT,
		MAX_WAIT_TIMEOUT,
		NULL, NULL, NULL
	},
	#define DEFAULT_INTERACTIVE_TIMEOUT 28800
	#define MIN_INTERACTIVE_TIMEOUT 	1
	#define MAX_INTERACTIVE_TIMEOUT 	31536000
	{
		{
			"interactive_timeout", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("After this number of seconds of inactivity, "
                         "the server will close the interactive connection."),
			NULL,
			GUC_NOT_IN_SAMPLE
		},
		&interactive_timeout,
		DEFAULT_INTERACTIVE_TIMEOUT,
		MIN_INTERACTIVE_TIMEOUT,
		MAX_INTERACTIVE_TIMEOUT,
		NULL, NULL, NULL
	},
	#define DEFAULT_NET_WRITE_TIMEOUT 	60
	#define MIN_NET_WRITE_TIMEOUT 		1
	#define MAX_NET_WRITE_TIMEOUT 		31536000
	{
		{
			"net_write_timeout", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("After this number of seconds of waiting, "
                         "the server will abort the write."),
			NULL,
			GUC_NOT_IN_SAMPLE
		},
		&net_write_timeout,
		DEFAULT_NET_WRITE_TIMEOUT,
		MIN_NET_WRITE_TIMEOUT,
		MAX_NET_WRITE_TIMEOUT,
		NULL, NULL, NULL
	},
	{
		{
			"sql_select_limit", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("The maximum number of rows to return from SELECT statements."),
			NULL,
			GUC_NOT_IN_SAMPLE
		},
		&sql_select_limit,
		INT_MAX - 1,
		0,
		INT_MAX,
		NULL, NULL, NULL
	},
	/* End - ReqID:SRS-MYSQL-PROTOCOL */
#endif

#ifdef IVY_MY_GUC_STRING_PARAMS
	/* Begin - ReqID:SRS-MISC-MYSQL-FRAME */
	{
		{"mysql.listen_addresses", PGC_POSTMASTER, CONN_AUTH_SETTINGS,
			gettext_noop("Sets MySQL host name or IP address(es) to listen to with pg protocol."),
			NULL,
			GUC_LIST_INPUT
		},
		&mys_listen_address,
		"localhost",
		NULL, NULL, NULL
	},
	{
		{"mysql.default_database", PGC_POSTMASTER, CONN_AUTH_SETTINGS,
			gettext_noop("Sets mysql default database name."),
			NULL,
			GUC_LIST_INPUT
		},
		&default_database,
		"highgo",
		NULL, NULL, NULL
	},
	/* Begin - Req: SRS-MYSQL-PROTOCOL */
	{
		{"version_comment", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("Version_comment for MySQL."),
			NULL,
			GUC_NOT_IN_SAMPLE
		},
		&version_comment_str,
		"HighGo",
		check_read_only_var, NULL, NULL
	},
	{
		{"sha256_password_public_key_path", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("A fully qualified path to the public RSA key used for authentication"),
			NULL
		},
		&sha256_password_public_key_path,
		"",
		check_read_only_var, NULL, NULL
	},
	{
		{"sha256_password_private_key_path", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("A fully qualified path to the private RSA key used for authentication"),
			NULL
		},
		&sha256_password_private_key_path,
		"",
		check_read_only_var, NULL, NULL
	},
	{
		{"character_set_client", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("Client uses this character set."),
			NULL,
			GUC_NOT_IN_SAMPLE
		},
		&character_set_client,
		"utf8",
		NULL, NULL, NULL
	},
	{
		{"character_set_connection", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("Set character_set_connection."),
			NULL,
			GUC_NOT_IN_SAMPLE
		},
		&character_set_connection,
		"",
		NULL, NULL, NULL
	},
	{
		{"character_set_results", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("The server uses this character set "
                         "to return the query results to the client."),
			NULL,
			GUC_NOT_IN_SAMPLE
		},
		&character_set_results,
		"utf8",
		NULL, NULL, NULL
	},
	{
		{"character_set_server", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("Server uses this character set."),
			NULL,
			GUC_NOT_IN_SAMPLE
		},
		&character_set_server,
		"latin1",
		NULL, NULL, NULL
	},
	{
		{"collation_server", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("Server uses this collation."),
			NULL,
			GUC_NOT_IN_SAMPLE
		},
		&collation_server,
		"latin1_swedish_ci",
		NULL, NULL, NULL
	},
	{
		{"collation_connection", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("Set collation_connection."),
			NULL,
			GUC_NOT_IN_SAMPLE
		},
		&collation_connection,
		"",
		NULL, NULL, NULL
	},
	{
		{"init_connect", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("SQL statements executed when each connection is initialized."),
			NULL,
			GUC_NOT_IN_SAMPLE
		},
		&init_connect,
		"",
		NULL, NULL, NULL
	},
	{
		{"license", PGC_INTERNAL, COMPAT_MYSQL_OPTIONS,
			gettext_noop("The server uses this license."),
			NULL,
			GUC_NOT_IN_SAMPLE
		},
		&license,
		"",
		NULL, NULL, NULL
	},
	{
		{"performance_schema", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("CUSTOM_OPTIONS."),
			NULL,
			GUC_NOT_IN_SAMPLE
		},
		&performance_schema,
		"",
		NULL, NULL, NULL
	},
	{
		{"system_time_zone", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("The time zone set by server."),
			NULL,
			GUC_NOT_IN_SAMPLE
		},
		&system_time_zone,
		"",
		NULL, NULL, NULL
	},
	{
		{"time_zone", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("The time zone set currently."),
			NULL,
			GUC_NOT_IN_SAMPLE
		},
		&time_zone,
		"SYSTEM",
		NULL, NULL, NULL
	},
	/* End - Req: SRS-MYSQL-PROTOCOL */
	/* Begin - ReqID:SRS-MISC-MYSQL-FRAME */
	{
		{
			"sql_mode",
			PGC_USERSET,
			COMPAT_MYSQL_OPTIONS,
			gettext_noop("Sets mysql sql_mode."),
			NULL,
			GUC_LIST_INPUT | GUC_NOT_IN_SAMPLE
		},
		&sql_mode_str,
		// psql need PIPES_AS_CONCAT, so we add it to default value, which is different from MySQL's.
		"ANSI_QUOTES,ONLY_FULL_GROUP_BY,STRICT_TRANS_TABLES,NO_ZERO_IN_DATE,NO_ZERO_DATE,"
		"ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION,REAL_AS_FLOAT,PIPES_AS_CONCAT",
		check_sql_mode, assign_sql_mode, show_sql_mode
	},
	/* Begin - ReqID:SRS-MYSQL-BUILDINFUNCTION */
	{
		{
			"lc_time_names", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
			gettext_noop("Sets mysql lc_time_names."),
			NULL,
			GUC_LIST_INPUT
		},
		&lc_time_names,
		"en_US",
		check_lc_time_names, NULL, NULL
	},
	/* End - ReqID:SRS-MYSQL-BUILDINFUNCTION */

#endif

#ifdef IVY_MY_GUC_ENUM_PARAMS
{
	{"mysql.protocol_type", PGC_POSTMASTER, PRESET_OPTIONS,
		gettext_noop("Set mysql port protocol type"),
		NULL,
		GUC_DISALLOW_IN_FILE
	},
	&mys_protocol_type,
	PROTOCOL_MYSQL, proto_type_options,
	NULL, NULL, NULL
},
{
	{"block_encryption_mode", PGC_USERSET, COMPAT_MYSQL_OPTIONS,
		gettext_noop("Sets mysql block encryption mode."),
		NULL,
		GUC_DISALLOW_IN_FILE
	},
	&mys_block_encryption_mode,
	mys_aes_128_ecb, encrytion_type_options,
	NULL, NULL, NULL
},
#endif

#ifdef IVY_MY_GUC_FUNC_DEFINE
/* Begin - Req: SRS-MYSQL-PROTOCOL */
static bool
check_read_only_var(char **newval, void **extra, GucSource source)
{
    if (source == PGC_S_SESSION)
    {
        ereport(ERROR,
                (errmsg("Variable 'version_comment' is a read only variable")));
    }
    
    return true;
}
/* End - Req: SRS-MYSQL-PROTOCOL */

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
	sql_mode = trans_sql_mode(newval);;
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

/* Begin - ReqID: SRS-MYSQL-NUMBERICTYPE */
/*
 * div_precision_increment
 *   guc parameter must >=4 and =< 30
 *
 */
bool
check_div_precision_increment(int *newval, void **extra, GucSource source)
{
	if (*newval < DIV_PREC_MIN_VALUE)
	{

		ereport(WARNING,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("Truncated incorrect div_precision_increment value: \"%d\"", *newval)));
		*newval = DIV_PREC_MIN_VALUE;
	}

	if (*newval > DIV_PREC_MAX_VALUE)
	{
		ereport(WARNING,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					errmsg("Truncated incorrect div_precision_increment value: \"%d\"", *newval)));
		*newval = DIV_PREC_MAX_VALUE;
	}

	return true;
}

void
assign_div_precision_increment(int newval, void *extra)
{
	div_precision_increment = newval;
}
/* End - ReqID: SRS-MYSQL-NUMBERICTYPE */

/* Begin - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
/*
 * auto_increment_increment
 *   guc parameter must >=4 and =< 30
 *
 */
static bool
check_auto_increment_increment(int *newval, void **extra, GucSource source)
{
	if (*newval < 1)
	{
		ereport(WARNING,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("Truncated incorrect auto_increment_increment value: \"%u\"", *newval)));
		*newval = 1;
	}

	if (*newval > AUTO_INCREMENT_MAX_VALUE)
	{
		ereport(WARNING,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("Truncated incorrect auto_increment_increment value: \"%u\"", *newval)));
		*newval = AUTO_INCREMENT_MAX_VALUE;
	}

	return true;
}

static void
assign_auto_increment_increment(int newval, void *extra)
{
	auto_increment_increment = newval;
}

/*
 * auto_increment_offset
 *   guc parameter must >=4 and =< 30
 *
 */
static bool
check_auto_increment_offset(int *newval, void **extra, GucSource source)
{
	if (*newval < 1)
	{
		ereport(WARNING,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("Truncated incorrect auto_increment_offset value: \"%u\"", *newval)));
		*newval = 1;
	}

	if (*newval > AUTO_INCREMENT_MAX_VALUE)
	{
		ereport(WARNING,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("Truncated incorrect auto_increment_offset value: \"%u\"", *newval)));
		*newval = AUTO_INCREMENT_MAX_VALUE;
	}

	return true;
}

static void
assign_auto_increment_offset(int newval, void *extra)
{
	auto_increment_offset = newval;
}
/* End - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */

static void
assign_regress_test_default_value(bool newval, void *extra)
{
	regress_test_default_value = newval;

	if (newval)
	{
		SetConfigOption("mysql.cast_to_char", "off", PGC_USERSET, PGC_S_OVERRIDE);
		SetConfigOption("mysql.mode_pad_char_to_full", "off", PGC_USERSET, PGC_S_OVERRIDE);
	}
}

/* Begin - ReqID:SRS-MYSQL-BUILDINFUNCTION */
#define REPORT_LC_TIME_NAMES_VALUE_ERROR(val) do { \
	ereport(ERROR, \
			(errcode(ERRCODE_SYSTEM_ERROR), \
			errmsg("Variable 'lc_time_names' can't be set to the value of '%s'", val))); \
} while(0);

static char *mys_lc_time_names[] =
{
	"ar_AE",
	"ar_BH",
	"ar_DZ",
	"ar_EG",
	"ar_IN",
	"ar_IQ",
	"ar_JO",
	"ar_KW",
	"ar_LB",
	"ar_LY",
	"ar_MA",
	"ar_OM",
	"ar_QA",
	"ar_SA",
	"ar_SD",
	"ar_SY",
	"ar_TN",
	"ar_YE",
	"be_BY",
	"bg_BG",
	"ca_ES",
	"cs_CZ",
	"da_DK",
	"de_AT",
	"de_BE",
	"de_CH",
	"de_DE",
	"de_LU",
	"en_AU",
	"en_CA",
	"en_GB",
	"en_IN",
	"en_NZ",
	"en_PH",
	"en_US",
	"en_ZA",
	"en_ZW",
	"es_AR",
	"es_BO",
	"es_CL",
	"es_CO",
	"es_CR",
	"es_DO",
	"es_EC",
	"es_ES",
	"es_GT",
	"es_HN",
	"es_MX",
	"es_NI",
	"es_PA",
	"es_PE",
	"es_PR",
	"es_PY",
	"es_SV",
	"es_US",
	"es_UY",
	"es_VE",
	"el_GR",
	"et_EE",
	"eu_ES",
	"fi_FI",
	"fo_FO",
	"fr_BE",
	"fr_CA",
	"fr_CH",
	"fr_FR",
	"fr_LU",
	"gl_ES",
	"gu_IN",
	"he_IL",
	"hi_IN",
	"hr_HR",
	"hu_HU",
	"id_ID",
	"is_IS",
	"it_CH",
	"it_IT",
	"ja_JP",
	"ko_KR",
	"lt_LT",
	"lv_LV",
	"mk_MK",
	"mn_MN",
	"ms_MY",
	"nb_NO",
	"nl_BE",
	"nl_NL",
	"no_NO",
	"pl_PL",
	"pt_BR",
	"pt_PT",
	"rm_CH",
	"ro_RO",
	"ru_RU",
	"ru_UA",
	"sk_SK",
	"sl_SI",
	"sq_AL",
	"sr_RS",
	"sv_FI",
	"sv_SE",
	"ta_IN",
	"te_IN",
	"th_TH",
	"tr_TR",
	"uk_UA",
	"ur_PK",
	"vi_VN",
	"zh_CN",
	"zh_HK",
	"zh_TW"
};

int LC_TIME_NMAES_NUM = sizeof(mys_lc_time_names)/sizeof(mys_lc_time_names[0]);

/*
 * check_lc_time_names
 * Check if the value of the lc_time_names variable is correct.
 */
static bool
check_lc_time_names(char **target, void **extra, GucSource source)
{
	(void)lc_time_names_Search(*target);
	return true;
}

/*
 * lc_time_names_Search
 * Use binary search to find out whether the lc_time_names variable exists in the mys_lc_time_names array.
 */
static int
lc_time_names_Search(const char *target)
{
	int		start = 0;
	int		end = LC_TIME_NMAES_NUM - 1;
	int		result = -1;

	while (start <= end)
	{
		int		mid = (start + end) / 2;
		int		cmp_res = pg_strcasecmp(target, mys_lc_time_names[mid]);
		if (cmp_res > 0)
		{
			start = mid + 1;
		}
		else if (cmp_res < 0)
		{
			end = mid - 1;
		}
		else
		{
			result = mid;
			break;
		}
	}
	if (result < 0)
		REPORT_LC_TIME_NAMES_VALUE_ERROR(target);

	return result;
}


/*
 * check_default_week_format
 *   guc parameter value calculation.
 *
 */
static bool
check_default_week_format(int *newval, void **extra, GucSource source)
{
	if (*newval < DEF_WEEK_FORMAT || *newval > MAX_WEEK_FORMAT)
	{
		ereport(WARNING, \
			(errcode(ERRCODE_SYSTEM_ERROR), \
			errmsg("Truncated incorrect default_week_format value: '%d'", *newval)));

		if (*newval < DEF_WEEK_FORMAT)
			*newval = DEF_WEEK_FORMAT;
		else
			*newval = MAX_WEEK_FORMAT;
	}
	return true;
}

/*
 * assign_default_week_format
 *   assign value for guc parameter.
 *
 * Assign a value to default_week_format.
 *
 */
void
assign_default_week_format(int newval, void *extra)
{
	default_week_format = newval;
}
/* End - ReqID:SRS-MYSQL-BUILDINFUNCTION */

#endif
