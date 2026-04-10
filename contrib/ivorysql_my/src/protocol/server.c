#include "postgres.h"

#include "catalog/catalog.h"
#include "catalog/namespace.h"
#include "catalog/pg_type.h"
#include "common/ip.h"
#include "common/hashfn.h"
#include "commands/user.h"
#include "miscadmin.h"
#include "parser/parser.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/pidfile.h"
#include "utils/syscache.h"
#include "utils/varlena.h"

#include "../include/message.h"
#include "../include/auth.h"
#include "../include/printtup.h"
#include "../include/server.h"

/* Begin - Req: SRS-MYSQL-PROTOCOL */
#define COLLATE_BINARY         		63
#define MAX_TYPE_NAME_LEN      		64

int					current_packet_type;
mysql_conn_info_t 	mysql_conn_info;

static create_server_listen_hook_type pre_server_listen_hook = NULL;
static get_protocol_config_hook_type pre_get_protocol_config_hook = NULL; /* bug#6805 */

static void mysql_session_init(ProtocolExtensionConfig *);

static struct HTAB *typoid2MysqlTypeHash = NULL;

static ProtocolExtensionConfig mysql_protocol_config = {
	T_ProtocolMysql,
	mysql_session_init,
	mysql_conn_handshake,
	mysql_client_authentication,
	mysql_client_authentication_finish,
	mysql_send_error_message,
	NULL,
	NULL,
	mysql_send_ready_for_query,
	mysql_read_command,
	mysql_end_command,
	mysql_printtup_create_DR,
	mysql_process_command, /* fn_process_command */
	NULL,
	mysql_receive_copy_begin, /* ReqID:SRS-MYSQL-SQL-LOADDATA */
	mysql_receive_copy_data   /* ReqID:SRS-MYSQL-SQL-LOADDATA */
};

static ProtocolExtensionConfig mysql_pg_protocol_config = {
	T_ProtocolPg,
	mysql_session_init,
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
	NULL,	/* ReqID:SRS-MYSQL-SQL-LOADDATA */
	NULL	/* ReqID:SRS-MYSQL-SQL-LOADDATA */
};

static TypeItem type_items[] = {
	{"tinyint", 	MYSQL_TYPE_TINY, 		TINYINTOID, 	0, 				COLLATE_BINARY},
    {"int2", 		MYSQL_TYPE_SHORT, 		INT2OID, 		0, 				COLLATE_BINARY},
	{"mediumint", 	MYSQL_TYPE_INT24,		MEDIUMINTOID, 	0, 				COLLATE_BINARY},
	{"int4", 		MYSQL_TYPE_LONG, 		INT4OID, 		0, 				COLLATE_BINARY},
    {"int8", 		MYSQL_TYPE_LONGLONG, 	INT8OID, 		0, 				COLLATE_BINARY},
	{"uint1", 		MYSQL_TYPE_TINY, 		UINT1OID, 		UNSIGNED_FLAG, 	COLLATE_BINARY},
	{"uint2", 		MYSQL_TYPE_SHORT, 		UINT2OID, 		UNSIGNED_FLAG, 	COLLATE_BINARY},
	{"uint3", 		MYSQL_TYPE_INT24, 		UINT3OID, 		UNSIGNED_FLAG, 	COLLATE_BINARY},
	{"uint4", 		MYSQL_TYPE_LONG, 		UINT4OID, 		UNSIGNED_FLAG, 	COLLATE_BINARY},
    {"uint8", 		MYSQL_TYPE_LONGLONG, 	UINT8OID, 		UNSIGNED_FLAG, 	COLLATE_BINARY},
	{"oid", 		MYSQL_TYPE_LONG, 		OIDOID,			UNSIGNED_FLAG, 	COLLATE_BINARY},
	{"bool", 		MYSQL_TYPE_TINY, 		BOOLOID, 		0, 				COLLATE_BINARY},
    {"float4", 		MYSQL_TYPE_FLOAT, 		FLOAT4OID, 		0, 				COLLATE_BINARY},
    {"float8", 		MYSQL_TYPE_DOUBLE, 		FLOAT8OID, 		0, 				COLLATE_BINARY},
	{"numeric", 	MYSQL_TYPE_NEWDECIMAL, 	NUMERICOID, 	0, 				COLLATE_BINARY},
    
    {"char", 		MYSQL_TYPE_STRING, 		CHAROID, 		0, 				COLLATE_UTF8MB4_GENERAL_CI},
    {"varchar", 	MYSQL_TYPE_VAR_STRING, 	VARCHAROID, 	0, 				COLLATE_UTF8MB4_GENERAL_CI},
	{"text", 		MYSQL_TYPE_BLOB, 		TEXTOID, 		BLOB_FLAG, 		COLLATE_UTF8MB4_GENERAL_CI},
	{"tinytext", 	MYSQL_TYPE_BLOB, 		TINYTEXTOID, 	BLOB_FLAG, 		COLLATE_UTF8MB4_GENERAL_CI},
	{"mediumtext", 	MYSQL_TYPE_BLOB, 		MEDIUMTEXTOID, 	BLOB_FLAG, 		COLLATE_UTF8MB4_GENERAL_CI},
	{"longtext", 	MYSQL_TYPE_BLOB, 		LONGTEXTOID, 	BLOB_FLAG, 		COLLATE_UTF8MB4_GENERAL_CI},

	{"mysbit", 		MYSQL_TYPE_BIT, 		MYSBITOID, 		BINARY_FLAG, 	COLLATE_BINARY},
	{"binary", 		MYSQL_TYPE_STRING, 		BINARYOID, 		BINARY_FLAG, 	COLLATE_BINARY},
	{"varbinary", 	MYSQL_TYPE_VAR_STRING, 	VARBINARYOID, 	BINARY_FLAG, 	COLLATE_BINARY},
	{"ivy_hex", 	MYSQL_TYPE_STRING, 		IVY_HEXOID, 	BINARY_FLAG, 	COLLATE_BINARY},
	{"blob", 		MYSQL_TYPE_BLOB, 		MYBLOBOID, 		BLOB_FLAG | BINARY_FLAG, 		COLLATE_BINARY},
	{"tinyblob", 	MYSQL_TYPE_BLOB, 		TINYBLOBOID, 	BLOB_FLAG | BINARY_FLAG, 		COLLATE_BINARY},
	{"mediumblob",	MYSQL_TYPE_BLOB, 		MEDIUMBLOBOID, 	BLOB_FLAG | BINARY_FLAG, 		COLLATE_BINARY},
	{"longblob", 	MYSQL_TYPE_BLOB, 		LONGBLOBOID, 	BLOB_FLAG | BINARY_FLAG, 		COLLATE_BINARY},
	
	{"year", 		MYSQL_TYPE_YEAR, 		YEAROID, 		UNSIGNED_FLAG | ZEROFILL_FLAG, 	COLLATE_BINARY},
	{"mysdate", 	MYSQL_TYPE_DATE, 		MYSDATEOID, 	0, 					COLLATE_BINARY},
	{"mystime", 	MYSQL_TYPE_TIME, 		MYSTIMEOID, 	0, 					COLLATE_BINARY},
	{"mystimes", 	MYSQL_TYPE_TIME, 		MYSTIMESOID, 	0, 					COLLATE_BINARY},
	{"mysdatetime", MYSQL_TYPE_DATETIME, 	MYSDATETIMEOID, 0, 					COLLATE_BINARY},
	{"mysdatetimes", MYSQL_TYPE_DATETIME, 	MYSDATETIMESOID,  0, 				COLLATE_BINARY},
	{"mystimestamp", MYSQL_TYPE_TIMESTAMP, 	MYSTIMESTAMPOID,  ZEROFILL_FLAG, 	COLLATE_BINARY},
	{"mystimestamps", MYSQL_TYPE_TIMESTAMP, MYSTIMESTAMPSOID, ZEROFILL_FLAG, 	COLLATE_BINARY},

	// ONLY FOR PG
    {"name", 		MYSQL_TYPE_STRING, 		NAMEOID, 		0, 				COLLATE_UTF8MB4_GENERAL_CI},
	{"date", 		MYSQL_TYPE_STRING, 		DATEOID, 		0, 				COLLATE_BINARY},
    {"time", 		MYSQL_TYPE_STRING, 		TIMEOID, 		0, 				COLLATE_BINARY},
    {"timestamptz", MYSQL_TYPE_STRING, 		TIMESTAMPTZOID, 0, 				COLLATE_BINARY},
    {"timestamp", 	MYSQL_TYPE_STRING, 		TIMESTAMPOID, 	0, 				COLLATE_BINARY},
	{"bpchar", 		MYSQL_TYPE_VARCHAR, 	BPCHAROID, 		0, 				COLLATE_UTF8MB4_GENERAL_CI},
	{"bit", 		MYSQL_TYPE_VAR_STRING, 	BITOID, 		BINARY_FLAG, 	COLLATE_BINARY},
    {"varbit", 		MYSQL_TYPE_VAR_STRING, 	VARBITOID, 		0, 				COLLATE_BINARY},
	{"bytea", 		MYSQL_TYPE_VAR_STRING, 	BYTEAOID, 		0, 				COLLATE_BINARY},

	{"point", 		MYSQL_TYPE_GEOMETRY, 	POINTOID, 		0, 				COLLATE_BINARY},
    {"line", 		MYSQL_TYPE_GEOMETRY, 	LINEOID, 		0, 				COLLATE_BINARY},
    {"polygon", 	MYSQL_TYPE_GEOMETRY, 	POLYGONOID, 	0, 				COLLATE_BINARY},
    
    {"json", 		MYSQL_TYPE_JSON, 		JSONOID, 		0, 				COLLATE_BINARY},

	{"regproc", 	MYSQL_TYPE_STRING, 		REGPROCOID, 	0, 				COLLATE_BINARY},
	{"pg_node_tree", MYSQL_TYPE_STRING, 	PG_NODE_TREEOID, 0, 			COLLATE_BINARY},
	{"_aclitem", 	MYSQL_TYPE_STRING, 		0, 				 0, 			COLLATE_BINARY},
	{"regtype",		MYSQL_TYPE_LONG,		REGTYPEOID,		0,				COLLATE_BINARY},
	{"unknown",		MYSQL_TYPE_VAR_STRING,	UNKNOWNOID,		0,				COLLATE_UTF8MB4_GENERAL_CI},
};

static const int ntype_items = sizeof(type_items) / sizeof(TypeItem);

static void
init_server_capabilities()
{
	set_client_capabilities(CLIENT_BASIC_FLAGS);

	/*
	 * TO DO support compress in ivorysql.
	 */
	#if 0
	if (opt_using_transactions)
    	protocol->add_client_capability(CLIENT_TRANSACTIONS);

  	protocol->add_client_capability(CAN_CLIENT_COMPRESS);
	#endif

	if (EnableSSL)
	{
		add_client_capabilities(CLIENT_SSL);
    	add_client_capabilities(CLIENT_SSL_VERIFY_SERVER_CERT);
	}
}

static void
init_mysql_conn_info()
{
	mysql_conn_info.resultset_metadata = RESULTSET_METADATA_FULL;
}

static void
mysql_session_init(ProtocolExtensionConfig *self)
{
	SetConfigOption("ivorysql.compatible_mode", "mysql", PGC_USERSET, PGC_S_OVERRIDE);      /* ReqID:SRS-MISC-GUC */

	if (mys_raw_parser == NULL)
	{
		ereport(ERROR,
				(errmsg("Invalid MySQL compatibility mode syntax library.")));
	}

	init_server_capabilities();

	init_mysql_conn_info();
	
	pq_init();

	if (nodeTag(self) == T_ProtocolMysql)
	{
		SetConfigOption("password_encryption", "mysql_native_password", PGC_USERSET, PGC_S_OVERRIDE);
		SetConfigOption("mysql.at_or_atat", "true", PGC_USERSET, PGC_S_OVERRIDE);
		SetConfigOption("mysql.enable_mysql_information_schema", "true", PGC_USERSET, PGC_S_OVERRIDE);
		SetConfigOption("mysql.qm_as_param", "true", PGC_USERSET, PGC_S_OVERRIDE);
		SetConfigOption("mysql.use_mysql_do", "true", PGC_USERSET, PGC_S_OVERRIDE);
		SetConfigOption("mysql.use_mysql_bit_literal", "true", PGC_USERSET, PGC_S_OVERRIDE); 
		SetConfigOption("ivorysql.aggregates_without_group_by", "true", PGC_USERSET, PGC_S_OVERRIDE);
	}
}

static void
create_mysql_server_listen(void)
{
	int		status;
	bool	listen_addr_saved = false;
	char 	*address = mys_listen_address;
	int 	port = mys_port;
	
	if (address)
	{
		char	   *rawstring;
		List	   *elemlist;
		ListCell   *l;
		int 		success = 0;

		/* Need a modifiable copy of ListenAddresses */
		rawstring = pstrdup(address);

		/* Parse string into list of hostnames */
		if (!SplitGUCList(rawstring, ',', &elemlist))
		{
			/* syntax error in list */
			ereport(FATAL,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("invalid list syntax in parameter \"%s\"", "listen_addresses")));
		}

		foreach(l, elemlist)
		{
			char	   *curhost = (char *) lfirst(l);

			if (strcmp(curhost, "*") == 0)
				status = StreamServerPort(AF_UNSPEC, NULL,
										  (unsigned short)port,
										  NULL,
										  ListenSocket, MAXLISTEN);
			else
				status = StreamServerPort(AF_UNSPEC, curhost,
										  (unsigned short)port,
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
			{
				ereport(WARNING,
						(errmsg("could not create listen socket for \"%s\"", curhost)));
			}
		}

		if (!success && elemlist != NIL)
		{
			ereport(FATAL,
					(errmsg("could not create any Mysql TCP/IP sockets")));
		}

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

		foreach(l, elemlist)
		{
			char	   *socketdir = (char *) lfirst(l);

			status = StreamServerPort(AF_UNIX, NULL,
									  (unsigned short)port,
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
				ereport(WARNING,
						(errmsg("could not create mysql Unix-domain socket in directory \"%s\"",
								socketdir)));
		}

		if (!success && elemlist != NIL)
			ereport(FATAL,
					(errmsg("could not create any mysql Unix-domain sockets")));
	}
#endif

	// create hash cache for caching_sha2_password
#ifdef USE_OPENSSL
	create_mysql_auth_cache();
#endif

	if (pre_server_listen_hook)
		pre_server_listen_hook();
}

static void
InitMysqlMicroHashTable(int size)
{
    HASHCTL info = {0};
    info.keysize = sizeof(Oid);
    info.entrysize = sizeof(HashEntryTypoid2TypeItem);
    info.hash = oid_hash;
    info.hcxt = TopMemoryContext;
    typoid2MysqlTypeHash = hash_create("MySQL Micro Type Oid Lookup Table", size, &info,
                                    	HASH_ELEM | HASH_FUNCTION | HASH_CONTEXT);
}

static const TypeItem *
TypoidHashTableAccess(HASHACTION action, Oid oid, const TypeItem *item)
{
	bool found = false;
	HashEntryTypoid2TypeItem *result = NULL;

	result = (HashEntryTypoid2TypeItem *) hash_search(typoid2MysqlTypeHash, &oid, action, &found);

	if (action == HASH_ENTER)
	{
		Assert(!found);
		result->item = item;
		return item;
	}

	if (action == HASH_FIND && found)
		return result->item;

	return NULL;
}

static void
InitTypoid2MysqlMacroHtab()
{
    InitMysqlMicroHashTable(ntype_items);

    for (int i = 0; i < ntype_items; i++)
	{
        TypeItem *typeItem = &type_items[i];
        if (strlen(typeItem->typname) > MAX_TYPE_NAME_LEN)
		{
            ereport(ERROR,
					(errmsg("the type name length exceed the limit of %d", MAX_TYPE_NAME_LEN)));
		}

        if (!OidIsValid(typeItem->oid))
		{
			typeItem->oid = TypenameGetTypid(typeItem->typname);

			if (!OidIsValid(typeItem->oid))
			{
				ereport(ERROR,
						(errmsg("the oid of data type(%s) is invalid", typeItem->typname)));
			}
		}

        TypoidHashTableAccess(HASH_ENTER, typeItem->oid, typeItem);
    }
}

const TypeItem *
GetItemByTypeOid(Oid oid, bool report_warning)
{
	HeapTuple 		tuple;
	Form_pg_type 	typeForm;
	const TypeItem 	*typeItem;
	TypeItem		*result = NULL;

    if (typoid2MysqlTypeHash == NULL)
	    InitTypoid2MysqlMacroHtab();

    if (!OidIsValid(oid))
	{
        ereport(ERROR,
				(errmsg("the oid of data type is invalid")));
	}

    typeItem = TypoidHashTableAccess(HASH_FIND, oid, NULL);

    if (typeItem != NULL)
		return typeItem;

	tuple = SearchSysCache1(TYPEOID, ObjectIdGetDatum(oid));

	if (!HeapTupleIsValid(tuple))
	{
		ereport(ERROR,
				(errmsg("%d is not the object id of type", oid)));
	}

	typeForm = (Form_pg_type) GETSTRUCT(tuple);
	result = (TypeItem *) palloc(sizeof(TypeItem));
	result->typname = NameStr(typeForm->typname);
	result->oid = oid;
	result->flags = 0;

	switch (typeForm->typtype)
	{
		case 'e':
			result->mysql_type_id = MYSQL_TYPE_ENUM;
			break;
		/* TO DO set type
		case 's':
			result->mysql_type_id = MYSQL_TYPE_SET;
			break;
		*/
		default:
			result->mysql_type_id = MYSQL_TYPE_STRING;
			break;
	}

	ReleaseSysCache(tuple);

	return result;
}

bool
type_is_blob(Oid oid)
{
	const TypeItem *item = GetItemByTypeOid(oid, false);

	return (item->flags & BLOB_FLAG) | (item->flags & BINARY_FLAG);
}

/* Begin - bug#6805 */
static ProtocolExtensionConfig *
mys_get_protocol_config(int family, char *local_port)
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

			UNIXSOCK_PATH(unixSocketPath, mys_port, socketdir);

			if (!strcmp(local_port, unixSocketPath))
			{
				if (mys_protocol_type == PROTOCOL_PG)
					return &mysql_pg_protocol_config;

				if (mys_protocol_type == PROTOCOL_MYSQL)
					return &mysql_protocol_config;
			}
		}
	}
	else
	{
		int port = pg_atoi(local_port, sizeof(int32), ' ');

		if (port == mys_port)
		{
			if (mys_protocol_type == PROTOCOL_PG)
				return &mysql_pg_protocol_config;

			if (mys_protocol_type == PROTOCOL_MYSQL)
				return &mysql_protocol_config;
		}
	}

	// Check other extension
	if (pre_get_protocol_config_hook)
		return pre_get_protocol_config_hook(family, local_port);

	return NULL;
}
/* End - bug#6805 */

void
register_mysql_listener()
{
	pre_server_listen_hook = server_listen_hook;
	server_listen_hook = create_mysql_server_listen;

	/* Begin - bug#6805 */
	pre_get_protocol_config_hook = get_protocol_config_hook;
	get_protocol_config_hook = mys_get_protocol_config;
	/* End - bug#6805 */
}

void
unregister_mysql_listener(void)
{
	server_listen_hook = pre_server_listen_hook;

	/* Begin - bug#6805 */
	get_protocol_config_hook = pre_get_protocol_config_hook;
	/* End - bug#6805 */
}
/* End - Req: SRS-MYSQL-PROTOCOL */
