/*------------------------------------------------------------------
 * 
 * File: ivorysql_ora.c
 *
 * Abstract: 
 * 	 Ivorysql_ora main entrypoint.
 *
 * Authored by zhenmingyang@highgo.com, 20220509.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 * 	 contrib/ivorysql_ora/src/ivorysql_ora.c
 *
 *------------------------------------------------------------------
 */

#include "postgres.h"

#include "fmgr.h"
#include "miscadmin.h"
#include "parser/parse_oper.h"
/* Begin - ReqID:SRS-SQL-INSERT */
#include "parser/parse_target.h"
/* End - ReqID:SRS-SQL-INSERT */

/* Begin - ReqID:SRS-SQL-MERGE */
#include "access/heapam.h"
#include "parser/parse_merge.h"
/* End - ReqID:SRS-SQL-MERGE */

/* Begin - ReqID:SRS-SQL-XML */
#include "executor/execExpr.h"
/* End - ReqID:SRS-SQL-XML */

#include "funcapi.h"
/* Begin - ReqID:SRS-PLSQL-Collection */
#include "include/collection.h"
#include "tstl2cl/include/c_algorithm.h"
#include "tstl2cl/include/c_def.h"
#include "tstl2cl/include/c_functional.h"
#include "tstl2cl/include/c_iterator.h"
#include "tstl2cl/include/c_list.h"
#include "tstl2cl/include/c_map.h"
#include "tstl2cl/include/c_queue.h"
#include "tstl2cl/include/c_set.h"
#include "tstl2cl/include/c_stack.h"
#include "tstl2cl/include/c_utility.h"
#include "tstl2cl/include/c_vector.h"
#include "tstl2cl/c_algo.h"
#include "tstl2cl/c_function.h"
#include "tstl2cl/c_memory.h"
#include "tstl2cl/c_pair.h"
#include "tstl2cl/c_tree.h"
/* End - ReqID:SRS-PLSQL-Collection */
#include "include/ivorysql_ora.h"
#include "include/nlssort.h"
#include "include/orarefcursor.h"

/* Begin - Req: SRS-MYSQL-PROTOCOL */
#include "common/ip.h"
#include "parser/parser.h"
#include "postmaster/postmaster.h"
#include "utils/builtins.h"
#include "utils/pidfile.h"
#include "utils/varlena.h"
/* End - Req: SRS-MYSQL-PROTOCOL */

/* Only include it once in any C file */
PG_MODULE_MAGIC;

/* Saved hook value in case of unload */
static oracle_datatype_precedence_hook_type pre_oracle_datatype_precedence_hook = NULL;

/* Begin - ReqID:SRS-SQL-INSERT */
static OraGetAttrnoHooktype pre_ora_get_attrno_hook = NULL;
/* End - ReqID:SRS-SQL-INSERT */

/* Begin - ReqID:SRS-SQL-MERGE */
static exec_merge_matched_hook_type pre_exec_merge_matched_hook = NULL;
static transform_merge_stmt_hook_type pre_transform_merge_stmt_hook = NULL;
/* End - ReqID:SRS-SQL-MERGE */

/* Begin - ReqID:SRS-SQL-XML */
static ora_updatexml_hook_type pre_ora_updatexml_hook = NULL;
/* End - ReqID:SRS-SQL-XML */

/* Begin - Req: SRS-MYSQL-PROTOCOL */
static create_server_listen_hook_type pre_server_listen_hook = NULL;
static get_protocol_config_hook_type  pre_get_protocol_config_hook = NULL; /* bug#6805 */
static void ora_session_init(ProtocolExtensionConfig *);
/* End - Req: SRS-MYSQL-PROTOCOL */

void _PG_init(void);
void _PG_fini(void);

/* Begin - ReqID:SRS-PLSQL-DBMS_OUTPUT */
void _dbms_output_PG_init(void);
void _dbms_output_PG_fini(void);
/* End - ReqID:SRS-PLSQL-DBMS_OUTPUT */

/* Begin - Req: SRS-MYSQL-PROTOCOL */
static ProtocolExtensionConfig ora_protocol_config = {
	T_ProtocolPg,
	ora_session_init,
	StartupPacketInitialize,
	ClientAuthentication,
	NULL,
	send_message_to_frontend,
	send_cancel_key,
	pq_comm_reset,
	ReadyForQuery,
	SocketBackend,
	NULL,
	printtup_create_DR,
	NULL, /* fn_process_command */
	libpq_report_param_status,
	NULL, /* ReqID:SRS-MYSQL-SQL-LOADDATA */
	NULL  /* ReqID:SRS-MYSQL-SQL-LOADDATA */
};

static void
ora_session_init(ProtocolExtensionConfig *self)
{
	SetConfigOption("ivorysql.compatible_mode", "oracle", PGC_USERSET, PGC_S_OVERRIDE);      /* ReqID:SRS-MISC-GUC */
	
	if (NULL == ora_raw_parser)
	{
		ereport(ERROR,
				(errmsg("Invalid Oracle compatibility mode syntax library.")));
	}

	pq_init();
}

static void
create_oracle_server_listen()
{
	int			status;
	bool		listen_addr_saved = false;

	if (OraListenAddresses)
	{
		char	   *rawstring;
		List	   *elemlist;
		ListCell   *l;
		int 		success = 0;

		/* Need a modifiable copy of ListenAddresses */
		rawstring = pstrdup(OraListenAddresses);

		/* Parse string into list of hostnames */
		if (!SplitGUCList(rawstring, ',', &elemlist))
		{
			/* syntax error in list */
			ereport(FATAL,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("invalid list syntax in parameter \"%s\"",
							"listen_addresses")));
		}

		foreach (l, elemlist)
		{
			char	   *curhost = (char *) lfirst(l);

			if (strcmp(curhost, "*") == 0)
				status = StreamServerPort(AF_UNSPEC, NULL,
										  (unsigned short) OraPortNumber,
										  NULL,
										  ListenSocket, MAXLISTEN);
			else
				status = StreamServerPort(AF_UNSPEC, curhost,
										  (unsigned short) OraPortNumber,
										  NULL,
										  ListenSocket, MAXLISTEN);

			if (status == STATUS_OK)
			{
				success++;
				/* record the first successful host addr in lockfile */
				if (!listen_addr_saved)
				{
					AddToDataDirLockFile(LOCK_FILE_LINE_LISTEN_ADDR, curhost);
					listen_addr_saved = true;
				}
			}
			else
				ereport(WARNING,
						(errmsg("could not create listen socket for \"%s\"",
								curhost)));
		}

		if (!success && elemlist != NIL)
			ereport(FATAL,
					(errmsg("could not create any Oracle TCP/IP sockets")));

		list_free(elemlist);
		pfree(rawstring);
	}

#ifdef HAVE_UNIX_SOCKETS
	if (Unix_socket_directories)
	{
		char	   *rawstring;
		List	   *elemlist;
		ListCell   *l;
		int			success = 0;

		/* Need a modifiable copy of Unix_socket_directories */
		rawstring = pstrdup(Unix_socket_directories);

		/* Parse string into list of directories */
		if (!SplitDirectoriesString(rawstring, ',', &elemlist))
		{
			/* syntax error in list */
			ereport(FATAL,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("invalid list syntax in parameter \"%s\"",
							"unix_socket_directories")));
		}

		foreach (l, elemlist)
		{
			char	   *socketdir = (char *) lfirst(l);

			status = StreamServerPort(AF_UNIX, NULL,
									  (unsigned short) OraPortNumber,
									  socketdir,
									  ListenSocket, MAXLISTEN);

			if (status == STATUS_OK)
			{
				success++;
				/* record the first successful Unix socket in lockfile */
				if (success == 1)
					AddToDataDirLockFile(LOCK_FILE_LINE_SOCKET_DIR, socketdir);
			}
			else
			{
				ereport(WARNING,
						(errmsg("could not create Oracle Unix-domain socket in directory \"%s\"",
								socketdir)));
			}
		}

		if (!success && elemlist != NIL)
		{
			ereport(FATAL,
					(errmsg("could not create any Oracle Unix-domain sockets")));
		}
	}
#endif

	if (pre_server_listen_hook)
		pre_server_listen_hook();
}

/* Begin - bug#6805 */
static ProtocolExtensionConfig *
ora_get_protocol_config(int family, char *local_port)
{
	// Check unix domain socket
	if (IS_AF_UNIX(family))
	{
		char 		*rawstring;
		List	  	*elemlist;
		ListCell	*l;
		char		unixSocketPath[MAXPGPATH];

		rawstring = pstrdup(Unix_socket_directories);

		if (!SplitDirectoriesString(rawstring, ',', &elemlist))
		{
			/* syntax error in list */
			ereport(FATAL,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("invalid list syntax in parameter \"%s\"",
							"unix_socket_directories")));
		}

		foreach(l, elemlist)
		{
			char	   *socketdir = (char *) lfirst(l);

			UNIXSOCK_PATH(unixSocketPath, OraPortNumber, socketdir);

			if (!strcmp(local_port, unixSocketPath))
				return &ora_protocol_config;
		}
	}
	else
	{
		int port = pg_atoi(local_port, sizeof(int32), ' ');

		if (port == OraPortNumber)
			return &ora_protocol_config;
	}

	// Check other extension
	if (pre_get_protocol_config_hook)
		return pre_get_protocol_config_hook(family, local_port);

	return NULL;
}
/* End - bug#6805 */
/* End - Req: SRS-MYSQL-PROTOCOL */

/*
 * Module initialization function.
 */
void
_PG_init(void)
{
#if 0
	/* Must be loaded with shared_preload_libaries */
	if (!process_shared_preload_libraries_in_progress)
		ereport(ERROR, (errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				errmsg("ivorysql_ora must be loaded via shared_preload_libraries")));
#endif

	/* Define custom GUC variables */
	
	/* Install Hooks */
	pre_oracle_datatype_precedence_hook = oracle_datatype_precedence_hook;
	oracle_datatype_precedence_hook = pg_compatible_oracle_precedence;

	/* Begin - ReqID:SRS-SQL-INSERT */
	pre_ora_get_attrno_hook = OraGetAttrnoHook;
	OraGetAttrnoHook = IvyOraGetAttrno;
	/* End - ReqID:SRS-SQL-INSERT */

	/* Begin - ReqID:SRS-SQL-MERGE */
	ora_exec_merge_matched_hook = IvyExecMergeMatched;
	ora_transform_merge_stmt_hook = IvytransformMergeStmt;
	pre_exec_merge_matched_hook = pg_exec_merge_matched_hook;
	pg_exec_merge_matched_hook = ora_exec_merge_matched_hook;
	pre_transform_merge_stmt_hook = pg_transform_merge_stmt_hook;
	pg_transform_merge_stmt_hook = ora_transform_merge_stmt_hook;
	/* End - ReqID:SRS-SQL-MERGE */

	/* Begin - ReqID:SRS-SQL-XML */
	pre_ora_updatexml_hook = ora_updatexml_hook;
	ora_updatexml_hook = updatexml;
	/* End - ReqID:SRS-SQL-XML */
	/* Begin - SRS-SQL-NLSSORT */
	nlssort_init();
	/* End - SRS-SQL-NLSSORT */
	/* Begin - ReqID:SRS-PLSQL-CURSOR */
	orarefcursor_init_internal();
	/* End - ReqID:SRS-PLSQL-CURSOR */

	_dbms_output_PG_init(); /* ReqID:SRS-PLSQL-DBMS_OUTPUT */
	
	/* Begin - Req: SRS-MYSQL-PROTOCOL */
	pre_server_listen_hook = server_listen_hook;
	server_listen_hook = create_oracle_server_listen;

	/* Begin - bug#6805 */
	pre_get_protocol_config_hook = get_protocol_config_hook;
	get_protocol_config_hook = ora_get_protocol_config;
	/* End - bug#6805 */
	/* End - Req: SRS-MYSQL-PROTOCOL */

	/* Begin - ReqID:SRS-PLSQL-Collection */
	collection_init_internal();
	/* End - ReqID:SRS-PLSQL-Collection */
}

/*
 * Module finalization function.
 *
 * Note that _PG_fini will only be called during an unload of the module, 
 * not during process termination. Presently, unloads are disabled and
 * will never occur, but this may change in the future.
 */
void
_PG_fini(void)
{
	/* Uninstall Hooks */
	oracle_datatype_precedence_hook = pre_oracle_datatype_precedence_hook;

	/* Begin - ReqID:SRS-SQL-INSERT */
	OraGetAttrnoHook = pre_ora_get_attrno_hook;
	/* Begin - ReqID:SRS-SQL-INSERT */

	/* Begin - ReqID:SRS-SQL-MERGE */
	pg_exec_merge_matched_hook = pre_exec_merge_matched_hook;
	pg_transform_merge_stmt_hook = pre_transform_merge_stmt_hook;
	/* End - ReqID:SRS-SQL-MERGE */

	/* Begin - ReqID:SRS-SQL-XML */
	ora_updatexml_hook = pre_ora_updatexml_hook;
	/* End - ReqID:SRS-SQL-XML */
	/* Begin - SRS-SQL-NLSSORT */
	nlssort_fini();
	/* End - SRS-SQL-NLSSORT */
	/* Begin - ReqID:SRS-PLSQL-CURSOR */
	orarefcursor_funcs.loadstate = UNLOADED;
	/* End - ReqID:SRS-PLSQL-CURSOR */

	_dbms_output_PG_fini(); /* ReqID:SRS-PLSQL-DBMS_OUTPUT */

	/* Begin - Req: SRS-MYSQL-PROTOCOL */
	server_listen_hook = pre_server_listen_hook;
	get_protocol_config_hook = pre_get_protocol_config_hook; /* bug#6805 */
	/* End - Req: SRS-MYSQL-PROTOCOL */
	/* Begin - ReqID:SRS-PLSQL-Collection */
	collection_funcs.loadstate = UNLOADED;
	/* End - ReqID:SRS-PLSQL-Collection */
}
