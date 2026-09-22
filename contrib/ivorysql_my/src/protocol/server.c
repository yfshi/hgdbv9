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

#include "../charset/ctype.h"
#include "../include/message.h"
#include "../include/auth.h"
#include "../include/printtup.h"
#include "../include/server.h"

/* Begin - Req: SRS-MYSQL-PROTOCOL */
#define COLLATE_BINARY         		63
#define MAX_TYPE_NAME_LEN      		64

int					current_packet_type;
mysql_conn_info_t 	mysql_conn_info;
/* Begin OA#2577 */
static MysqlInstanceEntry *mys_instances = NULL;
static int num_mys_instances = 0;
static void parse_mysql_instances(const char *port_list_str, const char *db_list_str);
/* End OA#2577 */

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
	{"tinyint", 	MYSQL_TYPE_TINY, 		TINYINTOID, 	0, 				true},
    {"int2", 		MYSQL_TYPE_SHORT, 		INT2OID, 		0, 				true},
	{"mediumint", 	MYSQL_TYPE_INT24,		MEDIUMINTOID, 	0, 				true},
	{"int4", 		MYSQL_TYPE_LONG, 		INT4OID, 		0, 				true},
    {"int8", 		MYSQL_TYPE_LONGLONG, 	INT8OID, 		0, 				true},
	{"uint1", 		MYSQL_TYPE_TINY, 		UINT1OID, 		UNSIGNED_FLAG, 	true},
	{"uint2", 		MYSQL_TYPE_SHORT, 		UINT2OID, 		UNSIGNED_FLAG, 	true},
	{"uint3", 		MYSQL_TYPE_INT24, 		UINT3OID, 		UNSIGNED_FLAG, 	true},
	{"uint4", 		MYSQL_TYPE_LONG, 		UINT4OID, 		UNSIGNED_FLAG, 	true},
    {"uint8", 		MYSQL_TYPE_LONGLONG, 	UINT8OID, 		UNSIGNED_FLAG, 	true},
	{"bool", 		MYSQL_TYPE_TINY, 		BOOLOID, 		BOOL_FLAG,		true},
    {"float4", 		MYSQL_TYPE_FLOAT, 		FLOAT4OID, 		0, 				true},
    {"float8", 		MYSQL_TYPE_DOUBLE, 		FLOAT8OID, 		0, 				true},
	{"numeric", 	MYSQL_TYPE_NEWDECIMAL, 	NUMERICOID, 	0, 				true},

    {"char", 		MYSQL_TYPE_STRING, 		CHAROID, 		0, 				false},
	{"bpchar", 		MYSQL_TYPE_STRING, 		BPCHAROID, 		0, 				false},
    {"varchar", 	MYSQL_TYPE_VAR_STRING, 	VARCHAROID, 	0, 				false},
	{"text", 		MYSQL_TYPE_BLOB, 		TEXTOID, 		BLOB_FLAG, 		false},
	{"tinytext", 	MYSQL_TYPE_BLOB, 		TINYTEXTOID, 	BLOB_FLAG, 		false},
	{"mediumtext", 	MYSQL_TYPE_BLOB, 		MEDIUMTEXTOID, 	BLOB_FLAG, 		false},
	{"longtext", 	MYSQL_TYPE_BLOB, 		LONGTEXTOID, 	BLOB_FLAG, 		false},

	{"mysbit", 		MYSQL_TYPE_BIT, 		MYSBITOID, 		BINARY_FLAG, 	true},
	{"binary", 		MYSQL_TYPE_STRING, 		BINARYOID, 		BINARY_FLAG, 	true},
	{"varbinary", 	MYSQL_TYPE_VAR_STRING, 	VARBINARYOID, 	BINARY_FLAG, 	true},
	{"ivy_hex", 	MYSQL_TYPE_STRING, 		IVY_HEXOID, 	BINARY_FLAG, 	true},
	{"blob", 		MYSQL_TYPE_BLOB, 		MYBLOBOID, 		BLOB_FLAG | BINARY_FLAG, 		true},
	{"tinyblob", 	MYSQL_TYPE_BLOB, 		TINYBLOBOID, 	BLOB_FLAG | BINARY_FLAG, 		true},
	{"mediumblob",	MYSQL_TYPE_BLOB, 		MEDIUMBLOBOID, 	BLOB_FLAG | BINARY_FLAG, 		true},
	{"longblob", 	MYSQL_TYPE_BLOB, 		LONGBLOBOID, 	BLOB_FLAG | BINARY_FLAG, 		true},

	{"year", 		MYSQL_TYPE_YEAR, 		YEAROID, 		UNSIGNED_FLAG | ZEROFILL_FLAG, 	true},
	{"mysdate", 	MYSQL_TYPE_DATE, 		MYSDATEOID, 	0, 					true},
	{"mystime", 	MYSQL_TYPE_TIME, 		MYSTIMEOID, 	0, 					true},
	{"mystimes", 	MYSQL_TYPE_TIME, 		MYSTIMESOID, 	0, 					true},
	{"mysdatetime", MYSQL_TYPE_DATETIME, 	MYSDATETIMEOID, 0, 					true},
	{"mysdatetimes", MYSQL_TYPE_DATETIME, 	MYSDATETIMESOID,  0, 				true},
	{"mystimestamp", MYSQL_TYPE_TIMESTAMP, 	MYSTIMESTAMPOID,  ZEROFILL_FLAG, 	true},
	{"mystimestamps", MYSQL_TYPE_TIMESTAMP, MYSTIMESTAMPSOID, ZEROFILL_FLAG, 	true},
	{"point", 		MYSQL_TYPE_GEOMETRY, 	POINTOID, 		0, 				true},


	/* Begin - BUG#9294 */
	/*
	* PostgreSQL-only types.
	*
	* These types do not participate in MySQL field type aggregation.
	* MYSQL_TYPE_UNKNOWN causes mysql_agg_field_type() to fall back to
	* PostgreSQL's native common-type resolution.
	*/
	// ONLY FOR PG
	{"bit", 		MYSQL_TYPE_UNKNOWN, 	BITOID, 		BINARY_FLAG, 	true},
	{"bytea", 		MYSQL_TYPE_UNKNOWN, 	BYTEAOID, 		0, 				true},
	{"date", 		MYSQL_TYPE_UNKNOWN, 	DATEOID, 		0, 				true},
    {"json", 		MYSQL_TYPE_UNKNOWN, 	JSONOID, 		0, 				true},
	{"jsonb", 		MYSQL_TYPE_JSON, 		JSONBOID, 		0, 				false},
    {"line", 		MYSQL_TYPE_UNKNOWN, 	LINEOID, 		0, 				true},
	{"money", 		MYSQL_TYPE_UNKNOWN, 	MONEYOID, 		0, 				true},
    {"name", 		MYSQL_TYPE_UNKNOWN, 	NAMEOID, 		0, 				false},
	{"oid", 		MYSQL_TYPE_UNKNOWN, 	OIDOID,			UNSIGNED_FLAG, 	true},
	{"pg_node_tree", MYSQL_TYPE_UNKNOWN, 	PG_NODE_TREEOID, 0, 			true},
    {"polygon", 	MYSQL_TYPE_UNKNOWN, 	POLYGONOID, 	0, 				true},
	{"regclass", 	MYSQL_TYPE_UNKNOWN, 	REGCLASSOID, 	0, 				true},
	{"regproc", 	MYSQL_TYPE_UNKNOWN, 	REGPROCOID, 	0, 				true},
	{"regtype",		MYSQL_TYPE_UNKNOWN,		REGTYPEOID,		0,				true},
    {"time", 		MYSQL_TYPE_UNKNOWN, 	TIMEOID, 		0, 				true},
    {"timestamptz", MYSQL_TYPE_UNKNOWN, 	TIMESTAMPTZOID, 0, 				true},
    {"timestamp", 	MYSQL_TYPE_UNKNOWN, 	TIMESTAMPOID, 	0, 				true},
	{"unknown",		MYSQL_TYPE_VAR_STRING,	UNKNOWNOID,		0,				false},
    {"varbit", 		MYSQL_TYPE_UNKNOWN, 	VARBITOID, 		0, 				true},

	{"_aclitem", 	MYSQL_TYPE_UNKNOWN, 	0, 				 0, 			true}
	/* End - BUG#9294 */
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
		/* Begin - ReqID:SRS-MYSQL-PROC */
		SetConfigOption("mysql.default_function_security_is_definer", "true", PGC_USERSET, PGC_S_OVERRIDE);
		SetConfigOption("mysql.create_function_with_sql_mode", "true", PGC_USERSET, PGC_S_OVERRIDE);
		/* End - ReqID:SRS-MYSQL-PROC */
		SetConfigOption("mysql.create_database_mode", "schema", PGC_USERSET, PGC_S_OVERRIDE);		/* BUG#Z8480 */
	}
}

static void
create_mysql_server_listen(void)
{
	bool	listen_addr_saved = false;
	char	*address = mys_listen_address;

	parse_mysql_instances(mys_port, default_database);

	for (int i = 0; i < num_mys_instances; i++)
	{
		int		status;
		int		port = mys_instances[i].port;

		if (address)
		{
			char	   *rawstring;
			List	   *elemlist;
			ListCell   *l;
			int			success = 0;

			rawstring = pstrdup(address);

			if (!SplitGUCList(rawstring, ',', &elemlist))
			{
				ereport(FATAL,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						 errmsg("invalid list syntax in parameter \"%s\"", "listen_addresses")));
			}

			foreach (l, elemlist)
			{
				char	   *curhost = (char *)lfirst(l);

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
					if (!listen_addr_saved)
					{
						AddToDataDirLockFile(LOCK_FILE_LINE_LISTEN_ADDR, curhost);
						listen_addr_saved = true;
					}
				}
				else
				{
					ereport(WARNING,
							(errmsg("could not create Mysql listen socket for \"%s\" on port %d",
									curhost, port)));
				}
			}

			if (!success && elemlist != NIL)
			{
				ereport(FATAL,
						(errmsg("could not create any Mysql TCP/IP sockets for port %d",
								port)));
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

			rawstring = pstrdup(Unix_socket_directories);

			if (!SplitDirectoriesString(rawstring, ',', &elemlist))
			{
				ereport(FATAL,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						 errmsg("invalid list syntax in parameter \"%s\"",
								"unix_socket_directories")));
			}

			foreach (l, elemlist)
			{
				char	   *socketdir = (char *)lfirst(l);

				status = StreamServerPort(AF_UNIX, NULL,
										  (unsigned short)port,
										  socketdir,
										  ListenSocket, MAXLISTEN);

				if (status == STATUS_OK)
				{
					success++;
					if (success == 1)
						AddToDataDirLockFile(LOCK_FILE_LINE_SOCKET_DIR, socketdir);
				}
				else
					ereport(WARNING,
							(errmsg("could not create mysql Unix-domain socket in directory \"%s\" for port %d",
									socketdir, port)));
			}

			if (!success && elemlist != NIL)
				ereport(FATAL,
						(errmsg("could not create any mysql Unix-domain sockets for port %d",
								port)));
		}
#endif
	}

	/* create hash cache for caching_sha2_password */
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
	result = (TypeItem *) palloc0(sizeof(TypeItem));
	result->typname = NameStr(typeForm->typname);
	result->oid = oid;
	result->flags = 0;
    result->is_binary_charset = false;


	switch (typeForm->typtype)
	{
		case TYPTYPE_ENUM:
			result->mysql_type_id = MYSQL_TYPE_STRING;
			result->flags |= ENUM_FLAG;
			break;
		/* TO DO set type
		case 's':
			result->mysql_type_id = MYSQL_TYPE_STRING;
			result->flags |= SET_FLAG;
			break;
		*/

		/*
		 * Composite \ domain \ pseudo types have no corresponding MySQL
		 * scalar type. Keep them unknown and fall back to
		 * PostgreSQL's native type resolution when necessary.
		 */
		case TYPTYPE_COMPOSITE:
		case TYPTYPE_PSEUDO:
		case TYPTYPE_DOMAIN:
			result->mysql_type_id = MYSQL_TYPE_UNKNOWN;
			break;

		default:
			/*
			 * PostgreSQL array types have no direct MySQL type mapping.
			 * Mark them as unknown so type resolution can fall back to
			 * PostgreSQL's native rules instead of treating them as strings.
			 */
			if (typeForm->typcategory == TYPCATEGORY_ARRAY)
				result->mysql_type_id = MYSQL_TYPE_UNKNOWN;
			else
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
	/* In the Windows environment (exec_backend), `num_mys_instances` was not inherited,
	 * resulting in a value of 0; therefore, parsing is performed again. */
	if (num_mys_instances == 0)
		parse_mysql_instances(mys_port, default_database);

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

		foreach (l, elemlist)
		{
			char	   *socketdir = (char *)lfirst(l);

			for (int i = 0; i < num_mys_instances; i++)
			{
				UNIXSOCK_PATH(unixSocketPath, mys_instances[i].port, socketdir);

				if (!strcmp(local_port, unixSocketPath))
				{
					if (mys_protocol_type == PROTOCOL_PG)
						return &mysql_pg_protocol_config;

					if (mys_protocol_type == PROTOCOL_MYSQL)
						return &mysql_protocol_config;
				}
			}
		}
	}
	else
	{
		int port = pg_atoi(local_port, sizeof(int32), ' ');

		if (lookup_mysql_instance(port) != NULL)
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

/* Begin OA#2577 */
void
parse_mysql_instances(const char *port_list_str, const char *db_list_str)
{
	List	   *port_elemlist = NIL;
	List	   *db_elemlist = NIL;
	ListCell   *pl, *dl;
	int			count = 0;
	int			i;

	/* Parse port list */
	if (port_list_str && port_list_str[0] != '\0')
	{
		if (!SplitGUCList(pstrdup(port_list_str), ',', &port_elemlist))
			ereport(FATAL,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("invalid list syntax in parameter \"mysql.port\"")));
	}

	/* Parse database list */
	if (db_list_str && db_list_str[0] != '\0')
	{
		if (!SplitGUCList(pstrdup(db_list_str), ',', &db_elemlist))
			ereport(FATAL,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("invalid list syntax in parameter \"mysql.default_database\"")));
	}

	/* Validate lengths match */
	{
		int nports = list_length(port_elemlist);
		int ndbs = list_length(db_elemlist);
		if (nports == 0)
			ereport(FATAL,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("mysql.port must be set when mysql.default_database has entries")));
		if (ndbs == 0)
			ereport(FATAL,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("mysql.default_database must be set when mysql.port has entries")));
		if (nports != ndbs)
			ereport(FATAL,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("mysql.port has %d entries but mysql.default_database has %d entries "
							"(must be equal)", nports, ndbs)));
		count = nports;
	}

	mys_instances = palloc0(sizeof(MysqlInstanceEntry) * count);
	num_mys_instances = count;

	pl = list_head(port_elemlist);
	dl = list_head(db_elemlist);
	for (i = 0; i < count; i++)
	{
		char	   *port_str = (char *) lfirst(pl);
		char	   *db_name = (char *) lfirst(dl);
		int			port;

		pl = lnext(port_elemlist, pl);
		dl = lnext(db_elemlist, dl);

		if (db_name[0] == '\0')
			ereport(FATAL,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("empty database name in mysql.default_database")));

		port = atoi(port_str);
		if (port == 0 || port < 1 || port > 65535)
			ereport(FATAL,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("invalid port number %d in mysql.port, "
							"must be between 1 and 65535", port)));

		mys_instances[i].port = port;
		mys_instances[i].database = pstrdup(db_name);
	}

	list_free(port_elemlist);
	list_free(db_elemlist);
}

MysqlInstanceEntry *
lookup_mysql_instance(int port)
{
	for (int i = 0; i < num_mys_instances; i++)
	{
		if (mys_instances[i].port == port)
			return &mys_instances[i];
	}
	return NULL;
}
/* End OA#2577 */

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

/* Begin - OA#2772 */
uint32
get_mysql_result_collation_id(const TypeItem *item)
{
    const char *encoding_name;

    /*
     * Binary types always use the MySQL binary charset/collation.
     */
    if (item->is_binary_charset)
        return my_charset_bin.number;

    /*
     * character_set_results being NULL or empty means that result data is
     * not converted, so the FIELD packet should describe the server
     * encoding.
     */
    if (CHARACTER_SET_RESULTS_IS_VALIDY())
        encoding_name = GetDatabaseEncodingName();
    else
        /*
         * Result data is converted to character_set_results, so the
         * FIELD packet charset must match the actual result encoding.
         */
        encoding_name = character_set_results;

    return get_mysql_collation_id_by_encoding_name(encoding_name);
}
/* End - OA#2772 */
