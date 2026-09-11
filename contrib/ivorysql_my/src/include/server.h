#ifndef MYS_SERVER_H
#define MYS_SERVER_H

#include "postgres.h"
#include "postmaster/postmaster.h"
#include "tcop/tcopprot.h"

#define SCRAMBLE_LENGTH                         20
#define CLIENT_PASSWORD_SHA256_TOKEN_LEN        32
#define AUTH_PLUGIN_DATA_LEN                    20

#define COLLATE_UTF8MB4_GENERAL_CI	            45

enum SERVER_STATUS_flags_enum {
    /**
     * Is raised when a multi-statement transaction
     * has been started, either explicitly, by means
     * of BEGIN or COMMIT AND CHAIN, or
     * implicitly, by the first transactional
     * statement, when autocommit=off.
     */
    SERVER_STATUS_IN_TRANS = 1,
    SERVER_STATUS_AUTOCOMMIT = 2,   /**< Server in auto_commit mode */
    SERVER_MORE_RESULTS_EXISTS = 8, /**< Multi query - next query exists */
    SERVER_QUERY_NO_GOOD_INDEX_USED = 16,
    SERVER_QUERY_NO_INDEX_USED = 32,
    /**
     * The server was able to fulfill the clients request and opened a
     * read-only non-scrollable cursor for a query. This flag comes
     * in reply to COM_STMT_EXECUTE and COM_STMT_FETCH commands.
     * Used by Binary Protocol Resultset to signal that COM_STMT_FETCH
     * must be used to fetch the row-data.
     * @todo Refify "Binary Protocol Resultset" and "COM_STMT_FETCH".
     */
    SERVER_STATUS_CURSOR_EXISTS = 64,
    /**
     * This flag is sent when a read-only cursor is exhausted, in reply to
     * COM_STMT_FETCH command.
     */
    SERVER_STATUS_LAST_ROW_SENT = 128,
    SERVER_STATUS_DB_DROPPED = 256, /**< A database was dropped */
    SERVER_STATUS_NO_BACKSLASH_ESCAPES = 512,
    /**
     * Sent to the client if after a prepared statement reprepare
     * we discovered that the new statement returns a different
     * number of result set columns.
     */
    SERVER_STATUS_METADATA_CHANGED = 1024,
    SERVER_QUERY_WAS_SLOW = 2048,
    /**
     * To mark ResultSet containing output parameter values.
     */
    SERVER_PS_OUT_PARAMS = 4096,

    /**
     * Set at the same time as SERVER_STATUS_IN_TRANS if the started
     * multi-statement transaction is a read-only transaction. Cleared
     * when the transaction commits or aborts. Since this flag is sent
     * to clients in OK and EOF packets, the flag indicates the
     * transaction status at the end of command execution.
     */
    SERVER_STATUS_IN_TRANS_READONLY = 8192,

    /**
     * This status flag, when on, implies that one of the state information has
     * changed on the server because of the execution of the last statement.
     */
    SERVER_SESSION_STATE_CHANGED = (1UL << 14)
};

#define ServerStatus() ( \
    (mys_autocommit ? SERVER_STATUS_AUTOCOMMIT : 0) | \
    (is_last_parsetree ? 0 : SERVER_MORE_RESULTS_EXISTS) /* OA-2499 */ \
)

// client capabilities
#define CLIENT_LONG_PASSWORD                      1
#define CLIENT_FOUND_ROWS                         (1UL << 1)
#define CLIENT_LONG_FLAG                          (1UL << 2)
#define CLIENT_CONNECT_WITH_DB                    (1UL << 3)
#define CLIENT_NO_SCHEMA                          (1UL << 4)
#define CLIENT_COMPRESS                           (1UL << 5)
#define CLIENT_ODBC                               (1UL << 6)
#define CLIENT_LOCAL_FILES                        (1UL << 7)
#define CLIENT_IGNORE_SPACE                       (1UL << 8)
#define CLIENT_PROTOCOL_41                        (1UL << 9)
#define CLIENT_INTERACTIVE                        (1UL << 10)
#define CLIENT_SSL                                (1UL << 11)
#define CLIENT_IGNORE_SIGPIPE                     (1UL << 12)
#define CLIENT_TRANSACTIONS                       (1UL << 13)
#define CLIENT_RESERVED                           (1UL << 14)
#define CLIENT_SECURE_CONNECTION                  (1UL << 15)
#define CLIENT_MULTI_STATEMENTS                   (1UL << 16)
#define CLIENT_MULTI_RESULTS                      (1UL << 17)
#define CLIENT_PS_MULTI_RESULTS                   (1UL << 18)
#define CLIENT_PLUGIN_AUTH                        (1UL << 19)
#define CLIENT_CONNECT_ATTRS                      (1UL << 20)
#define CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA     (1UL << 21)
#define CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS       (1UL << 22)
#define CLIENT_SESSION_TRACK                      (1UL << 23)
#define CLIENT_DEPRECATE_EOF                      (1UL << 24)
#define CLIENT_OPTIONAL_RESULTSET_METADATA        (1UL << 25)
#define CLIENT_ZSTD_COMPRESSION_ALGORITHM         (1UL << 26)
#define CLIENT_QUERY_ATTRIBUTES                   (1UL << 27)
#define MULTI_FACTOR_AUTHENTICATION               (1UL << 28)
#define CLIENT_CAPABILITY_EXTENSION               (1UL << 29)
#define CLIENT_SSL_VERIFY_SERVER_CERT             (1UL << 30)
#define CLIENT_REMEMBER_OPTIONS                   (1UL << 31)
#define CLIENT_RESERVED2                          (32768)       /**< DEPRECATED: Old flag for 4.1 authentication CLIENT_SECURE_CONNECTION */

#define CLIENT_ALL_FLAGS                                                       \
  (CLIENT_LONG_PASSWORD | CLIENT_FOUND_ROWS | CLIENT_LONG_FLAG |               \
   CLIENT_CONNECT_WITH_DB | CLIENT_NO_SCHEMA | CLIENT_COMPRESS | CLIENT_ODBC | \
   CLIENT_LOCAL_FILES | CLIENT_IGNORE_SPACE | CLIENT_PROTOCOL_41 |             \
   CLIENT_INTERACTIVE | CLIENT_SSL | CLIENT_IGNORE_SIGPIPE |                   \
   CLIENT_TRANSACTIONS | CLIENT_RESERVED | CLIENT_RESERVED2 |                  \
   CLIENT_MULTI_STATEMENTS | CLIENT_MULTI_RESULTS | CLIENT_PS_MULTI_RESULTS |  \
   CLIENT_SSL_VERIFY_SERVER_CERT | CLIENT_REMEMBER_OPTIONS |                   \
   CLIENT_PLUGIN_AUTH | CLIENT_CONNECT_ATTRS |                                 \
   CLIENT_PLUGIN_AUTH_LENENC_CLIENT_DATA |                                     \
   CLIENT_CAN_HANDLE_EXPIRED_PASSWORDS | CLIENT_SESSION_TRACK |                \
   CLIENT_DEPRECATE_EOF | CLIENT_OPTIONAL_RESULTSET_METADATA |                 \
   CLIENT_ZSTD_COMPRESSION_ALGORITHM | CLIENT_QUERY_ATTRIBUTES |               \
   MULTI_FACTOR_AUTHENTICATION)

#define CLIENT_BASIC_FLAGS \
		(CLIENT_ALL_FLAGS & ~(CLIENT_SSL | CLIENT_COMPRESS | CLIENT_SSL_VERIFY_SERVER_CERT | CLIENT_ZSTD_COMPRESSION_ALGORITHM))

/* Command phase definition*/
#define COM_SLEEP                   0x00
#define COM_QUIT                    0x01
#define COM_INIT_DB                 0x02
#define COM_QUERY                   0x03
#define COM_FIELD_LIST              0x04
#define COM_CREATE_DB               0x05
#define COM_DROP_DB                 0x06
#define COM_REFRESH                 0x07
#define COM_DEPRECATED_1            0x08
#define COM_STATISTICS              0x09
#define COM_PROCESS_INFO            0x0a
#define COM_CONNECT                 0x0b
#define COM_PROCESS_KILL            0x0c
#define COM_DEBUG                   0x0d
#define COM_PING                    0x0e
#define COM_TIME                    0x0f
#define COM_DELAYED_INSERT          0x10
#define COM_CHANGE_USER             0x11
#define COM_BINLOG_DUMP             0x12
#define COM_TABLE_DUMP              0x13
#define COM_CONNECT_OUT             0x14
#define COM_REGISTER_SLAVE          0x15
#define COM_STMT_PREPARE            0x16
#define COM_STMT_EXECUTE            0x17
#define COM_STMT_SEND_LONG_DATA     0x18
#define COM_STMT_CLOSE              0x19
#define COM_STMT_RESET              0x1a
#define COM_SET_OPTION              0x1b
#define COM_STMT_FETCH              0x1c
#define COM_DAEMON                  0x1d
#define COM_BINLOG_DUMP_GTID        0x1e
#define COM_RESET_CONNECTION        0x1f

// flag for column definition
#define NOT_NULL_FLAG				1
#define PRI_KEY_FLAG			    2
#define UNIQUE_KEY_FLAG				4
#define MULTIPLE_KEY_FLAG			8
#define BLOB_FLAG					16
#define UNSIGNED_FLAG          		32
#define ZEROFILL_FLAG				64
#define BINARY_FLAG                 128
#define ENUM_FLAG					256
#define AUTO_INCREMENT_FLAG			512
#define TIMESTAMP_FLAG				1024
#define SET_FLAG					2048
#define NO_DEFAULT_VALUE            4096
#define ON_UPDATE_NOW_FLAG          8192
#define PART_KEY_FLAG               16384
#define NUM_FLAG                    32768

#define DEFAULLT_DATA_FIELD_LEN     65535

/* Begin - OA#2772 */
#define CHARACTER_SET_RESULTS_IS_VALIDY() \
    (character_set_results == NULL || character_set_results[0] == '\0')
/* End - OA#2772 */

typedef struct {
    uint32 client_capabilities;
    uint32 max_packet_size;
    uint8  charset;
    char   *schema; /* bug#6321 */
    uint8  scramble[SCRAMBLE_LENGTH + 1];
    uint8  auth_response[CLIENT_PASSWORD_SHA256_TOKEN_LEN + 1];
    uint8  resultset_metadata;
    /* data */
} mysql_conn_info_t;

typedef struct {
    uint8   protocol_version;
    char    *server_version_str;
    uint32  server_version;
    uint32  thread_id;
    char    auth_plugin_data[AUTH_PLUGIN_DATA_LEN + 1];
    uint32  capabilities;
    uint8   charset;
    uint16  server_status;
    char    *auth_plugin_name;
} mysql_handshake_t;

typedef struct {
    char *key;
    char *value;
} mysql_conn_attr_t;

typedef struct {
    uint32  client_capabilities;
    uint32  max_packet_size;
    uint8   charset;
    char    *username;
    char    *auth_response;
    char    *database;
    // the Authentication Method used by the client to generate auth-response value in this packet. This is an UTF-8 string.
    char    *client_plugin_name;
    uint8   zstd_compression_level;
    uint32  nattrs;
    mysql_conn_attr_t attrs[128];
} mysql_handshake_response_t;

typedef struct {
    uint8   header;
    uint64  affected_rows;
    uint64  last_insert_id;
    uint16  status_flags;
    uint16  warnings;
    char    *info;
    char    *session_state_info;
} mysql_ok_packet_t;

typedef struct {
    const char  *errmsg;
    const char  *sqlstate;
    uint16      errcode;
} mysql_err_packet_t;

typedef struct {
    char    *typname;             // type name of ivorysql
    uint32  mysql_type_id;        // defined in enum_field_types from MySQL source code
    Oid     oid;                  // oid of pg type's, this is hash key
    uint32  flags;                // type flag, such as unsigned flag or others
    bool    is_binary_charset;    // whether type uses binary charset
} TypeItem;

typedef struct {
    Oid oid;
    const TypeItem* item;
} HashEntryTypoid2TypeItem;

typedef struct {
    char        *name;
    char        *org_name;
    char        *table;
    char        *org_table;
    char        *db;
    char        *catalog;
    char        *def;
    uint32      length;
    uint32      max_length;
    uint32      name_length;
    uint32      org_name_length;
    uint32      table_length;
    uint32      org_table_length;
    uint32      db_length;
    uint32      catalog_length;
    uint32      def_length;
    uint32      flags;
    uint32      decimals;
    uint32      charset;
    uint32      type;
    void        *extension;
    char        *default_value;
    bool        is_com_field_list; /* bug#6850 */
} mysql_column_definition_t;

typedef enum enum_field_types {
    MYSQL_TYPE_DECIMAL,
    MYSQL_TYPE_TINY,
    MYSQL_TYPE_SHORT,
    MYSQL_TYPE_LONG,
    MYSQL_TYPE_FLOAT,
    MYSQL_TYPE_DOUBLE,
    MYSQL_TYPE_NULL,
    MYSQL_TYPE_TIMESTAMP,
    MYSQL_TYPE_LONGLONG,
    MYSQL_TYPE_INT24,
    MYSQL_TYPE_DATE,
    MYSQL_TYPE_TIME,
    MYSQL_TYPE_DATETIME,
    MYSQL_TYPE_YEAR,
    MYSQL_TYPE_NEWDATE, /**< Internal to MySQL. Not used in protocol */
    MYSQL_TYPE_VARCHAR,
    MYSQL_TYPE_BIT,
    MYSQL_TYPE_TIMESTAMP2,
    MYSQL_TYPE_DATETIME2,   /**< Internal to MySQL. Not used in protocol */
    MYSQL_TYPE_TIME2,       /**< Internal to MySQL. Not used in protocol */
    MYSQL_TYPE_TYPED_ARRAY, /**< Used for replication only */
    MYSQL_TYPE_VECTOR = 242,
    MYSQL_TYPE_INVALID = 243,
    MYSQL_TYPE_BOOL = 244, /**< Currently just a placeholder */
    MYSQL_TYPE_JSON = 245,
    MYSQL_TYPE_NEWDECIMAL = 246,
    MYSQL_TYPE_ENUM = 247,
    MYSQL_TYPE_SET = 248,
    MYSQL_TYPE_TINY_BLOB = 249,
    MYSQL_TYPE_MEDIUM_BLOB = 250,
    MYSQL_TYPE_LONG_BLOB = 251,
    MYSQL_TYPE_BLOB = 252,
    MYSQL_TYPE_VAR_STRING = 253,
    MYSQL_TYPE_STRING = 254,
    MYSQL_TYPE_GEOMETRY = 255
} enum_field_types;

enum enum_cursor_type {
    CURSOR_TYPE_NO_CURSOR = 0,
    CURSOR_TYPE_READ_ONLY = 1,
    CURSOR_TYPE_FOR_UPDATE = 2,
    CURSOR_TYPE_SCROLLABLE = 4,
    /**
     * On when the client will send the parameter count
     * even for 0 parameters.
     */
    PARAMETER_COUNT_AVAILABLE = 8
};

enum enum_resultset_metadata {
    RESULTSET_METADATA_NONE,
    RESULTSET_METADATA_FULL
};

typedef struct {
    uint32  statement_id;
    uint8   flags;
    uint32  iteration_count;        // Number of times to execute the statement. Currently always 1.
    uint32  param_count;
    char    *null_bitmap;
    uint8   new_params_bind_flag;
    ParamListInfo paramLI;
    /**
     * 0 for CURSOR OPEN REQUEST COM_STMT_EXECUTE.
     * FETCH_ALL for normal COM_STMT_EXECUTE.
     * Other values for COM_STMT_FETCH.
     */
    int64   num_rows;
} com_stmt_exec_t;

#define IS_CURSOR_OPEN_REQUEST(R) ((R->flags & (CURSOR_TYPE_READ_ONLY | CURSOR_TYPE_FOR_UPDATE | CURSOR_TYPE_SCROLLABLE)))

extern int		current_packet_type;
extern mysql_conn_info_t    mysql_conn_info;

/* Begin OA#2577 */
typedef struct MysqlInstanceEntry {
	int		port;
	char   *database;
} MysqlInstanceEntry;

extern MysqlInstanceEntry *lookup_mysql_instance(int port);
/* End OA#2577 */
extern void register_mysql_listener(void);
extern void unregister_mysql_listener(void);
extern const TypeItem *GetItemByTypeOid(Oid oid, bool report_warning); /* Bug - #z6172 */
extern bool type_is_blob(Oid oid);

/* Begin - OA#2772 */
extern uint32 get_mysql_result_collation_id(const TypeItem *item);
/* End - OA#2772 */

static inline uint32
get_client_capabilities()
{
	return mysql_conn_info.client_capabilities;
}

static inline uint32
set_client_capabilities(uint32 capabilities)
{
	mysql_conn_info.client_capabilities = capabilities;
	return mysql_conn_info.client_capabilities;
}

static inline uint32
add_client_capabilities(uint32 capabilities)
{
	mysql_conn_info.client_capabilities |= capabilities;
	return mysql_conn_info.client_capabilities;
}

static inline uint32
del_client_capabilities(uint32 capabilities)
{
	mysql_conn_info.client_capabilities &= ~capabilities;
	return mysql_conn_info.client_capabilities;
}

static inline bool
has_client_capabilities(uint32 capabilities)
{
	return mysql_conn_info.client_capabilities & capabilities;
}
#endif
