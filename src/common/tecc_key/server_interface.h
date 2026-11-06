/*-------------------------------------------------------------------------
 *
 * server_interface.h
 *    Server interface layer for TECC key management module
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/common/tecc_key/server_interface.h
 *
 * ReqID:TECC4.0 - Key management server interface layer
 * Authored by <wanghaoyan@highgo.com>, 20260408.
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *-------------------------------------------------------------------------
 */
#ifndef TECC_KEY_SERVER_INTERFACE_H
#define TECC_KEY_SERVER_INTERFACE_H

#include "tecc_common.h"

/* Mock libpq types */
typedef struct pg_conn PGconn;
typedef struct pg_result PGresult;

#include "key_manager.h"

/* Database connection handle */
typedef struct DbConnection
{
    void *pg_conn;              /* PostgreSQL connection handle */
    char *connection_string;    /* Connection string */
    bool is_connected;          /* Whether connected */
    time_t connect_time;        /* Connection time */
    time_t last_used;           /* Last used time */
    int use_count;              /* Use count */
} DbConnection;

/* Connection pool */
typedef struct ConnectionPool
{
    DbConnection **connections; /* Array of connections */
    int pool_size;              /* Current pool size */
    int max_pool_size;          /* Maximum pool size */
    int min_pool_size;          /* Minimum pool size */
    time_t connection_timeout;  /* Connection timeout */
    time_t idle_timeout;        /* Idle timeout */
    char *connection_string;    /* Connection string for new connections */
    pthread_mutex_t lock;       /* Pool lock */
} ConnectionPool;

/* Query result */
typedef struct QueryResult
{
    int num_rows;               /* Number of rows */
    int num_cols;               /* Number of columns */
    char **column_names;        /* Column names */
    char ***rows;               /* Row data */
    void *internal_result;      /* Internal result handle */
} QueryResult;

/* System table operation types */
typedef enum SystemTableOperation
{
    TABLE_OP_INSERT,            /* Insert operation */
    TABLE_OP_UPDATE,            /* Update operation */
    TABLE_OP_DELETE,            /* Delete operation */
    TABLE_OP_SELECT,            /* Select operation */
    TABLE_OP_TRUNCATE           /* Truncate operation */
} SystemTableOperation;

/* Function prototypes */

/* Connection pool management */
extern ConnectionPool *connection_pool_create(int min_size, int max_size,
                                             time_t conn_timeout, time_t idle_timeout,
                                             const char *conn_str);
extern void connection_pool_destroy(ConnectionPool *pool);
extern DbConnection *connection_pool_get_connection(ConnectionPool *pool);
extern void connection_pool_release_connection(ConnectionPool *pool, DbConnection *conn);
extern void connection_pool_close_idle_connections(ConnectionPool *pool);
extern int connection_pool_get_stats(ConnectionPool *pool,
                                    int *total_conns, int *active_conns, int *idle_conns);

/* Database connection */
extern DbConnection *db_connection_create(const char *conn_str);
extern void db_connection_destroy(DbConnection *conn);
extern int db_connection_connect(DbConnection *conn);
extern void db_connection_disconnect(DbConnection *conn);
extern bool db_connection_is_valid(DbConnection *conn);
extern int db_connection_reconnect(DbConnection *conn);

/* System table operations - KEK */
extern int server_interface_create_kek(DbConnection *conn, PgTeccKekInfo *kek_info);
extern int server_interface_query_kek_by_oid(DbConnection *conn, Oid kek_oid,
                                            PgTeccKekInfo **kek_info);
extern int server_interface_update_kek(DbConnection *conn, PgTeccKekInfo *kek_info);
extern int server_interface_delete_kek(DbConnection *conn, const char *kek_name);
extern int server_interface_list_keks(DbConnection *conn, char ***kek_names,
                                     int *kek_count);

/* System table operations - DEK */
extern int server_interface_create_dek(DbConnection *conn, PgTeccDekInfo *dek_info);
extern int server_interface_query_dek(DbConnection *conn, const char *dek_name,
                                     PgTeccDekInfo **dek_info);
extern int server_interface_query_dek_by_oid(DbConnection *conn, Oid dek_oid,
                                            PgTeccDekInfo **dek_info);
extern int server_interface_query_deks_by_kek(DbConnection *conn, const char *kek_name,
                                             char ***dek_names, int *dek_count);
extern int server_interface_query_kek_for_dek(DbConnection *conn, const char *dek_name,
                                             char **kek_name);
extern int server_interface_update_dek(DbConnection *conn, PgTeccDekInfo *dek_info);
extern int server_interface_delete_dek(DbConnection *conn, const char *dek_name);
extern int server_interface_list_deks(DbConnection *conn, char ***dek_names,
                                     int *dek_count);

/* Permission checking */
extern bool server_interface_check_dek_permission(DbConnection *conn, Oid dek_oid,
                                                 Oid user_oid);
extern bool server_interface_check_kek_permission(DbConnection *conn, Oid kek_oid,
                                                 Oid user_oid);
extern int server_interface_grant_dek_permission(DbConnection *conn, Oid dek_oid,
                                                Oid user_oid, const char *privilege);
extern int server_interface_revoke_dek_permission(DbConnection *conn, Oid dek_oid,
                                                 Oid user_oid, const char *privilege);
extern int server_interface_get_dek_acl(DbConnection *conn, Oid dek_oid,
                                       char **acl_string);
extern int server_interface_get_kek_acl(DbConnection *conn, Oid kek_oid,
                                       char **acl_string);

/* Query execution */
extern QueryResult *server_interface_execute_query(DbConnection *conn,
                                                  const char *query,
                                                  int num_params,
                                                  const char **param_values);
extern void server_interface_free_query_result(QueryResult *result);
extern int server_interface_execute_update(DbConnection *conn,
                                          const char *query,
                                          int num_params,
                                          const char **param_values,
                                          int *rows_affected);

/* Transaction management */
extern int server_interface_begin_transaction(DbConnection *conn);
extern int server_interface_commit_transaction(DbConnection *conn);
extern int server_interface_rollback_transaction(DbConnection *conn);
extern bool server_interface_in_transaction(DbConnection *conn);

/* Batch operations */
extern int server_interface_batch_query_deks(DbConnection *conn,
                                            const char **dek_names,
                                            int num_deks,
                                            PgTeccDekInfo ***dek_infos);
extern int server_interface_batch_check_permissions(DbConnection *conn,
                                                   Oid *key_oids,
                                                   int num_keys,
                                                   Oid user_oid,
                                                   bool *permissions);

/* Error handling */
extern const char *server_interface_get_last_error(DbConnection *conn);
extern void server_interface_clear_last_error(DbConnection *conn);
extern int server_interface_get_last_error_code(DbConnection *conn);

/* Utility functions */
extern char *server_interface_escape_string(DbConnection *conn, const char *str);
extern char *server_interface_quote_identifier(DbConnection *conn, const char *ident);
extern int server_interface_ping(DbConnection *conn);

/* Conversion functions */
extern PgTeccKekInfo *server_interface_parse_kek_tuple(void *tuple, void *tupdesc);
extern PgTeccDekInfo *server_interface_parse_dek_tuple(void *tuple, void *tupdesc);
extern char **server_interface_build_kek_insert_values(PgTeccKekInfo *kek_info);
extern char **server_interface_build_dek_insert_values(PgTeccDekInfo *dek_info);

#endif /* TECC_KEY_SERVER_INTERFACE_H */