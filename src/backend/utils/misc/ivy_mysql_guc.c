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
#ifdef IVY_MY_GUC_VAR_DEFINE
/* Begin - ReqID:SRS-MYSQL-UPDATE */
bool		enable_indexhint = false;
/* End - ReqID:SRS-MYSQL-UPDATE */

#endif
#ifdef IVY_MY_GUC_BOOL_PARAMS

	/* Begin - ReqID:SRS-MYSQL-UPDATE */
	{
		{"mysql.enable_indexhint", PGC_USERSET, COMPAT_OPTIONS_PREVIOUS,
			gettext_noop("whether enable index hint feature in MySQL compatible mode."),
			NULL
		},
		&enable_indexhint,
		false,
		NULL, NULL, NULL
	},
	/* End - ReqID:SRS-MYSQL-UPDATE */
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
/* End - ReqID:SRS-MISC-MYSQL-FRAME */
#endif
