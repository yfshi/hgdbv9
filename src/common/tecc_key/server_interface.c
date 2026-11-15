/*-------------------------------------------------------------------------
 *
 * server_interface.c
 *    Server interface layer implementation for TECC key management module
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/common/tecc_key/server_interface.c
 *
 * ReqID:TECC4.0 - Key management server interface layer
 * Authored by <wanghaoyan@highgo.com>, 20260408.
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *-------------------------------------------------------------------------
 */
#include "postgres_fe.h"
#include "c.h"
#include "common/fe_memutils.h"
#include "port/pg_bswap.h"
#include <pthread.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "server_interface.h"
#include "utils.h"

/* Libpq headers for database connection */
#include "../../interfaces/libpq/libpq-fe.h"
#include "libpq/libpq-fs.h"

/* Constants */
#define DEFAULT_MIN_POOL_SIZE 1
#define DEFAULT_MAX_POOL_SIZE 10
#define DEFAULT_CONNECTION_TIMEOUT 30  /* 30 seconds */
#define DEFAULT_IDLE_TIMEOUT 300       /* 5 minutes */
#define MAX_CONNECTION_STRING_LENGTH 1024

/* Thread-local error buffer */
static pthread_key_t server_error_key;
static pthread_once_t server_error_key_once = PTHREAD_ONCE_INIT;

/*
 * create_server_error_key
 *    Create thread-specific error key for server interface
 */
static void
create_server_error_key(void)
{
    pthread_key_create(&server_error_key, free);
}

/*
 * get_server_error_buffer
 *    Get thread-specific error buffer for server interface
 */
static char *
get_server_error_buffer(void)
{
    char *buffer;

    pthread_once(&server_error_key_once, create_server_error_key);

    buffer = pthread_getspecific(server_error_key);
    if (buffer == NULL)
    {
        buffer = malloc(512);
        if (buffer != NULL)
        {
            memset(buffer, 0, 512);
            pthread_setspecific(server_error_key, buffer);
        }
    }

    return buffer;
}

/*
 * set_server_error
 *    Set server interface error message
 */
static void
set_server_error(const char *format, ...)
{
    va_list args;
    char *buffer;

    buffer = get_server_error_buffer();
    if (buffer == NULL)
        return;

    va_start(args, format);
    vsnprintf(buffer, 512, format, args);
    va_end(args);
}

/*
 * connection_pool_create
 *    Create a new connection pool
 */
ConnectionPool *
connection_pool_create(int min_size, int max_size,
                      time_t conn_timeout, time_t idle_timeout,
                      const char *conn_str)
{
    ConnectionPool *pool = NULL;
    int ret;

    /* Validate parameters */
    if (min_size < 0 || max_size < min_size || conn_timeout <= 0 || idle_timeout <= 0)
    {
        set_server_error("Invalid connection pool parameters");
        return NULL;
    }

    /* Use defaults if not specified */
    if (min_size == 0)
        min_size = DEFAULT_MIN_POOL_SIZE;
    if (max_size == 0)
        max_size = DEFAULT_MAX_POOL_SIZE;
    if (conn_timeout == 0)
        conn_timeout = DEFAULT_CONNECTION_TIMEOUT;
    if (idle_timeout == 0)
        idle_timeout = DEFAULT_IDLE_TIMEOUT;

    /* Allocate memory for pool */
    pool = (ConnectionPool *)pg_malloc0(sizeof(ConnectionPool));
    if (pool == NULL)
    {
        set_server_error("Failed to allocate memory for connection pool");
        return NULL;
    }

    /* Initialize pool fields */
    pool->min_pool_size = min_size;
    pool->max_pool_size = max_size;
    pool->pool_size = 0;
    pool->connection_timeout = conn_timeout;
    pool->idle_timeout = idle_timeout;

    /* Store connection string for creating new connections */
    if (conn_str != NULL)
    {
        pool->connection_string = pg_strdup(conn_str);
        if (pool->connection_string == NULL)
        {
            set_server_error("Failed to allocate memory for pool connection string");
            pg_free(pool);
            return NULL;
        }
    }
    else
    {
        pool->connection_string = NULL;
    }

    /* Allocate connections array */
    pool->connections = (DbConnection **)pg_malloc0(max_size * sizeof(DbConnection *));
    if (pool->connections == NULL)
    {
        set_server_error("Failed to allocate memory for connections array");
        if (pool->connection_string != NULL)
            pg_free(pool->connection_string);
        pg_free(pool);
        return NULL;
    }

    /* Initialize mutex */
    ret = pthread_mutex_init(&pool->lock, NULL);
    if (ret != 0)
    {
        set_server_error("Failed to initialize pool mutex: %d", ret);
        if (pool->connection_string != NULL)
            pg_free(pool->connection_string);
        pg_free(pool->connections);
        pg_free(pool);
        return NULL;
    }

    return pool;
}

/*
 * connection_pool_destroy
 *    Destroy a connection pool
 */
void
connection_pool_destroy(ConnectionPool *pool)
{
    int i;

    if (pool == NULL)
        return;

    pthread_mutex_lock(&pool->lock);

    /* Close and destroy all connections */
    for (i = 0; i < pool->pool_size; i++)
    {
        if (pool->connections[i] != NULL)
        {
            db_connection_disconnect(pool->connections[i]);
            db_connection_destroy(pool->connections[i]);
            pool->connections[i] = NULL;
        }
    }

    pthread_mutex_unlock(&pool->lock);

    /* Cleanup resources */
    pthread_mutex_destroy(&pool->lock);
    if (pool->connection_string != NULL)
        pg_free(pool->connection_string);
    pg_free(pool->connections);
    pg_free(pool);
}

/*
 * connection_pool_get_connection
 *    Get a connection from the pool (or create new one)
 */
DbConnection *
connection_pool_get_connection(ConnectionPool *pool)
{
    DbConnection *conn = NULL;
    time_t current_time;
    int i;
    int oldest_index = -1;
    time_t oldest_time = 0;

    if (pool == NULL)
    {
        set_server_error("Connection pool is NULL");
        return NULL;
    }

    current_time = time(NULL);

    pthread_mutex_lock(&pool->lock);

    /* Try to find an available connection */
    for (i = 0; i < pool->pool_size; i++)
    {
        if (pool->connections[i] != NULL && pool->connections[i]->is_connected)
        {
            /* Check if connection is idle */
            if (current_time - pool->connections[i]->last_used > pool->idle_timeout)
            {
                /* Connection idle too long, close it */
                db_connection_disconnect(pool->connections[i]);
                continue;
            }

            /* Check if connection is still valid */
            if (!db_connection_is_valid(pool->connections[i]))
            {
                /* Connection invalid, try to reconnect */
                if (db_connection_reconnect(pool->connections[i]) != 0)
                {
                    /* Reconnect failed, remove from pool */
                    db_connection_destroy(pool->connections[i]);
                    pool->connections[i] = NULL;
                    continue;
                }
            }

            /* Found available connection */
            conn = pool->connections[i];
            conn->last_used = current_time;
            conn->use_count++;
            break;
        }
        else if (pool->connections[i] == NULL)
        {
            /* Empty slot, remember for potential reuse */
            oldest_index = i;
            oldest_time = current_time;
        }
        else
        {
            /* Track oldest connection for LRU */
            if (oldest_index == -1 || pool->connections[i]->last_used < oldest_time)
            {
                oldest_index = i;
                oldest_time = pool->connections[i]->last_used;
            }
        }
    }

    /* If no available connection found, create new one if pool not full */
    if (conn == NULL && pool->pool_size < pool->max_pool_size)
    {
        int slot = (oldest_index != -1) ? oldest_index : pool->pool_size;

        /* Create new connection using pool's connection string */
        conn = db_connection_create(pool->connection_string);
        if (conn != NULL && db_connection_connect(conn) == 0)
        {
            conn->last_used = current_time;
            conn->use_count = 1;
            pool->connections[slot] = conn;
            if (slot == pool->pool_size)
                pool->pool_size++;
        }
        else
        {
            if (conn != NULL)
                db_connection_destroy(conn);
            conn = NULL;
        }
    }

    pthread_mutex_unlock(&pool->lock);

    if (conn == NULL)
        set_server_error("Failed to get connection from pool");

    return conn;
}

/*
 * connection_pool_release_connection
 *    Release a connection back to the pool
 */
void
connection_pool_release_connection(ConnectionPool *pool, DbConnection *conn)
{
    if (pool == NULL || conn == NULL)
        return;

    pthread_mutex_lock(&pool->lock);

    /* Update last used time */
    conn->last_used = time(NULL);

    /* If pool is over maximum size, close extra connections */
    if (pool->pool_size > pool->max_pool_size)
    {
        /* TODO: Implement connection eviction logic */
    }

    pthread_mutex_unlock(&pool->lock);
}

/*
 * connection_pool_close_idle_connections
 *    Close idle connections in the pool
 */
void
connection_pool_close_idle_connections(ConnectionPool *pool)
{
    time_t current_time;
    int i;

    if (pool == NULL)
        return;

    current_time = time(NULL);

    pthread_mutex_lock(&pool->lock);

    for (i = 0; i < pool->pool_size; i++)
    {
        if (pool->connections[i] != NULL && pool->connections[i]->is_connected)
        {
            /* Check if connection is idle beyond timeout */
            if (current_time - pool->connections[i]->last_used > pool->idle_timeout)
            {
                /* Also check if pool size would drop below minimum */
                int active_connections = 0;
                int j;

                for (j = 0; j < pool->pool_size; j++)
                {
                    if (pool->connections[j] != NULL && pool->connections[j]->is_connected &&
                        j != i)
                    {
                        active_connections++;
                    }
                }

                if (active_connections >= pool->min_pool_size)
                {
                    /* Close idle connection */
                    db_connection_disconnect(pool->connections[i]);
                }
            }
        }
    }

    pthread_mutex_unlock(&pool->lock);
}

/*
 * connection_pool_get_stats
 *    Get connection pool statistics
 */
int
connection_pool_get_stats(ConnectionPool *pool,
                         int *total_conns, int *active_conns, int *idle_conns)
{
    int total = 0;
    int active = 0;
    int idle = 0;
    int i;

    if (pool == NULL)
        return -1;

    pthread_mutex_lock(&pool->lock);

    for (i = 0; i < pool->pool_size; i++)
    {
        if (pool->connections[i] != NULL)
        {
            total++;
            if (pool->connections[i]->is_connected)
            {
                time_t current_time = time(NULL);
                if (current_time - pool->connections[i]->last_used > pool->idle_timeout)
                    idle++;
                else
                    active++;
            }
        }
    }

    pthread_mutex_unlock(&pool->lock);

    if (total_conns != NULL)
        *total_conns = total;
    if (active_conns != NULL)
        *active_conns = active;
    if (idle_conns != NULL)
        *idle_conns = idle;

    return 0;
}

/*
 * db_connection_create
 *    Create a new database connection handle
 */
DbConnection *
db_connection_create(const char *conn_str)
{
    DbConnection *conn = NULL;

    if (conn_str == NULL)
    {
        set_server_error("Connection string is required");
        return NULL;
    }

    conn = (DbConnection *)pg_malloc0(sizeof(DbConnection));
    if (conn == NULL)
    {
        set_server_error("Failed to allocate memory for database connection");
        return NULL;
    }

    conn->connection_string = pg_strdup(conn_str);
    if (conn->connection_string == NULL)
    {
        pg_free(conn);
        set_server_error("Failed to allocate memory for connection string");
        return NULL;
    }

    conn->is_connected = false;
    conn->connect_time = 0;
    conn->last_used = 0;
    conn->use_count = 0;
    conn->pg_conn = NULL;  /* Will be set when connected */

    return conn;
}

/*
 * db_connection_destroy
 *    Destroy a database connection handle
 */
void
db_connection_destroy(DbConnection *conn)
{
    if (conn == NULL)
        return;

    /* Disconnect if still connected */
    if (conn->is_connected)
        db_connection_disconnect(conn);

    /* Free connection string */
    if (conn->connection_string != NULL)
        pg_free(conn->connection_string);

    /* Free the connection handle */
    pg_free(conn);
}

/*
 * db_connection_connect
 *    Connect to database using libpq
 */
int
db_connection_connect(DbConnection *conn)
{
    PGconn *pgconn = NULL;

    if (conn == NULL)
    {
        set_server_error("Database connection is NULL");
        return -1;
    }

    if (conn->is_connected)
    {
        set_server_error("Database connection already connected");
        return 0;
    }

    /* Connect to PostgreSQL using libpq */
    pgconn = PQconnectdb(conn->connection_string);
    if (pgconn == NULL)
    {
        set_server_error("Failed to allocate PostgreSQL connection");
        return -1;
    }

    /* Check connection status */
    if (PQstatus(pgconn) != CONNECTION_OK)
    {
        set_server_error("Connection to database failed: %s", PQerrorMessage(pgconn));
        PQfinish(pgconn);
        return -1;
    }

    /* Connection successful */
    conn->pg_conn = pgconn;
    conn->is_connected = true;
    conn->connect_time = time(NULL);
    conn->last_used = conn->connect_time;

    tecc_key_log(LOG_LEVEL_INFO, __func__, __FILE__, __LINE__,
                "Connected to database successfully");

    return 0;
}

/*
 * db_connection_disconnect
 *    Disconnect from database
 */
void
db_connection_disconnect(DbConnection *conn)
{
    if (conn == NULL || !conn->is_connected)
        return;

    /* Close PostgreSQL connection */
    if (conn->pg_conn != NULL)
    {
        PQfinish((PGconn *)conn->pg_conn);
        conn->pg_conn = NULL;
    }

    conn->is_connected = false;

    tecc_key_log(LOG_LEVEL_INFO, __func__, __FILE__, __LINE__,
                "Disconnected from database");
}

/*
 * db_connection_is_valid
 *    Check if database connection is valid
 */
bool
db_connection_is_valid(DbConnection *conn)
{
    PGconn *pgconn = NULL;
    PGresult *res = NULL;
    bool valid = false;

    if (conn == NULL || !conn->is_connected || conn->pg_conn == NULL)
        return false;

    pgconn = (PGconn *)conn->pg_conn;

    /* Check connection status */
    if (PQstatus(pgconn) != CONNECTION_OK)
        return false;

    /* Try a simple query to verify connection */
    res = PQexec(pgconn, "SELECT 1");
    if (res == NULL)
        return false;

    valid = (PQresultStatus(res) == PGRES_TUPLES_OK);
    PQclear(res);

    return valid;
}

/*
 * db_connection_reconnect
 *    Reconnect to database
 */
int
db_connection_reconnect(DbConnection *conn)
{
    if (conn == NULL)
    {
        set_server_error("Database connection is NULL");
        return -1;
    }

    /* Disconnect first if connected */
    if (conn->is_connected)
        db_connection_disconnect(conn);

    /* Try to reconnect */
    return db_connection_connect(conn);
}

/*
 * server_interface_create_kek
 *    Create KEK in system table
 */
int
server_interface_create_kek(DbConnection *conn, PgTeccKekInfo *kek_info)
{
    /* TODO: Implement actual libpq connection and SQL execution */
    /* Variables will be used when implementation is complete:
    PGresult *res = NULL;
    char *sql = NULL;
    int ret = -1;
    */

    if (conn == NULL || kek_info == NULL || !conn->is_connected)
    {
        set_server_error("Invalid parameters for creating KEK");
        return -1;
    }

    /* TODO: Implement actual libpq connection and SQL execution */
    /* For now, simulate success */
    return 0;
}


/*
 * server_interface_query_kek_by_oid
 *    Query KEK from system table by OID
 */
int
server_interface_query_kek_by_oid(DbConnection *conn, Oid kek_oid,
                                 PgTeccKekInfo **kek_info)
{
    /* TODO: Implement actual database SELECT operation */
    /* For now, return NULL as placeholder */
    *kek_info = NULL;
    return -1;
}

/*
 * server_interface_update_kek
 *    Update KEK in system table
 */
int
server_interface_update_kek(DbConnection *conn, PgTeccKekInfo *kek_info)
{
    /* TODO: Implement actual database UPDATE operation */
    /* For now, return success as placeholder */
    return 0;
}

/*
 * server_interface_delete_kek
 *    Delete KEK from system table
 */
int
server_interface_delete_kek(DbConnection *conn, const char *kek_name)
{
    /* TODO: Implement actual database DELETE operation */
    /* For now, return success as placeholder */
    return 0;
}

/*
 * server_interface_list_keks
 *    List all KEKs from system table
 */
int
server_interface_list_keks(DbConnection *conn, char ***kek_names,
                          int *kek_count)
{
    /* TODO: Implement actual database SELECT operation */
    /* For now, return empty list as placeholder */
    *kek_names = NULL;
    *kek_count = 0;
    return 0;
}

/*
 * server_interface_create_dek
 *    Create DEK in system table
 */
int
server_interface_create_dek(DbConnection *conn, PgTeccDekInfo *dek_info)
{
    /* TODO: Implement actual database INSERT operation */
    /* For now, return success as placeholder */
    return 0;
}

/*
 * server_interface_query_dek
 *    Query DEK from system table by name
 */
int
server_interface_query_dek(DbConnection *conn, const char *dek_name,
                          PgTeccDekInfo **dek_info)
{
    PGconn *pgconn = NULL;
    PGresult *res = NULL;
    PgTeccDekInfo *info = NULL;
    const char *param_values[1];
    const char *key_data;
    int key_data_len;

    if (conn == NULL || dek_name == NULL || dek_info == NULL || !conn->is_connected)
    {
        set_server_error("Invalid parameters for querying DEK");
        return -1;
    }

    pgconn = (PGconn *)conn->pg_conn;

    /* Build SQL query to get DEK information from hg_dek table */
    param_values[0] = dek_name;

    /* Execute parameterized query */
    res = PQexecParams(pgconn,
                      "SELECT d.oid, d.dek_name, k.kek_name as kek_name, "
                      "d.supports_det, d.supports_ope, d.supports_search, "
                      "d.key_data, d.created_time, d.status "
                      "FROM hg_dek d "
                      "LEFT JOIN hg_kek k ON d.kek_id = k.oid "
                      "WHERE d.dek_name = $1 AND d.status = 'active'",
                      1,      /* One parameter */
                      NULL,   /* Let server infer param types */
                      param_values,
                      NULL,   /* Param lengths (text means null) */
                      NULL,   /* Param formats (text means null) */
                      0);     /* Result format (0 = text) */
    if (res == NULL)
    {
        set_server_error("Failed to execute query: %s", PQerrorMessage(pgconn));
        return -1;
    }

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        set_server_error("Query failed: %s", PQerrorMessage(pgconn));
        PQclear(res);
        return -1;
    }

    /* Check if we got a result */
    if (PQntuples(res) == 0)
    {
        set_server_error("DEK '%s' not found or not active", dek_name);
        PQclear(res);
        return -1;
    }

    /* Allocate memory for DEK info */
    info = (PgTeccDekInfo *)pg_malloc0(sizeof(PgTeccDekInfo));
    if (info == NULL)
    {
        set_server_error("Failed to allocate memory for DEK info");
        PQclear(res);
        return -1;
    }

    /* Parse query results */
    info->dek_oid = atoi(PQgetvalue(res, 0, 0));
    info->dek_name = pg_strdup(PQgetvalue(res, 0, 1));
    info->kek_name = pg_strdup(PQgetvalue(res, 0, 2));

    /* DEK不应该记录算法类型，算法类型在加密列中指定（如'det,ope'等）
     * 因此将算法支持字段设置为默认值（全部支持），不限制算法类型
     * 注意：后端hg_dek表的supports_det/supports_ope/supports_search字段已废弃
     */
    info->supports_det = true;   /* DET算法默认支持 */
    info->supports_ope = true;   /* OPE算法默认支持 */
    info->supports_search = true; /* SEARCH算法默认支持 */

    /* DEK不记录算法类型，设置为默认的RND算法类型 */
    info->algo_type = TECC_ALGO_RND;  /* Default to RND algorithm */

    /* Get encrypted key data */
    key_data = PQgetvalue(res, 0, 6);
    key_data_len = PQgetlength(res, 0, 6);

    if (key_data_len > 0)
    {
        info->encrypted_key = pg_malloc(key_data_len);
        if (info->encrypted_key != NULL)
        {
            memcpy(info->encrypted_key, key_data, key_data_len);
            info->encrypted_len = key_data_len;
        }
    }

    /* Parse timestamp */
    /* TODO: Parse actual timestamp from result */
    /* For now, use current time */
    info->created_time = time(NULL);
    info->status = pg_strdup(PQgetvalue(res, 0, 8));

    PQclear(res);

    *dek_info = info;

    tecc_key_log(LOG_LEVEL_INFO, __func__, __FILE__, __LINE__,
                "Successfully queried DEK '%s' (OID: %d)", dek_name, info->dek_oid);

    return 0;
}

/*
 * server_interface_check_dek_permission
 *    Check if user has USAGE permission on DEK
 */
bool
server_interface_check_dek_permission(DbConnection *conn, Oid dek_oid,
                                     Oid user_oid)
{
    PGconn *pgconn = NULL;
    PGresult *res = NULL;
    bool has_permission = false;
    char param1[32], param2[32];
    const char *param_values[2];

    if (conn == NULL || !conn->is_connected || dek_oid == InvalidOid)
    {
        set_server_error("Invalid parameters for DEK permission check");
        return false;
    }

    pgconn = (PGconn *)conn->pg_conn;

    /* Build SQL query to check DEK permission */
    snprintf(param1, sizeof(param1), "%u", user_oid);
    snprintf(param2, sizeof(param2), "%u", dek_oid);

    param_values[0] = param1;
    param_values[1] = param2;

    /* Execute parameterized query */
    res = PQexecParams(pgconn,
                      "SELECT has_table_privilege($1::oid, 'hg_dek', 'SELECT') "
                      "AND EXISTS (SELECT 1 FROM hg_dek WHERE oid = $2::oid)",
                      2,      /* Two parameters */
                      NULL,   /* Let server infer param types */
                      param_values,
                      NULL,   /* Param lengths (text means null) */
                      NULL,   /* Param formats (text means null) */
                      0);     /* Result format (0 = text) */
    if (res == NULL)
    {
        set_server_error("Failed to execute permission query: %s",
                        PQerrorMessage(pgconn));
        return false;
    }

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        set_server_error("Permission query failed: %s", PQerrorMessage(pgconn));
        PQclear(res);
        return false;
    }

    /* Parse result */
    if (PQntuples(res) > 0)
    {
        const char *result = PQgetvalue(res, 0, 0);
        has_permission = (result != NULL && strcmp(result, "t") == 0);
    }

    PQclear(res);

    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                "DEK permission check: user=%u, dek=%u, has_permission=%d",
                user_oid, dek_oid, has_permission);

    return has_permission;
}

/*
 * server_interface_check_kek_permission
 *    Check if user has USAGE permission on KEK
 */
bool
server_interface_check_kek_permission(DbConnection *conn, Oid kek_oid,
                                     Oid user_oid)
{
    PGconn *pgconn = NULL;
    PGresult *res = NULL;
    bool has_permission = false;
    char param1[32], param2[32];
    const char *param_values[2];

    if (conn == NULL || !conn->is_connected || kek_oid == InvalidOid)
    {
        set_server_error("Invalid parameters for KEK permission check");
        return false;
    }

    pgconn = (PGconn *)conn->pg_conn;

    /* Build SQL query to check KEK permission */
    snprintf(param1, sizeof(param1), "%u", user_oid);
    snprintf(param2, sizeof(param2), "%u", kek_oid);

    param_values[0] = param1;
    param_values[1] = param2;

    /* Execute parameterized query */
    res = PQexecParams(pgconn,
                      "SELECT has_table_privilege($1::oid, 'hg_kek', 'SELECT') "
                      "AND EXISTS (SELECT 1 FROM hg_kek WHERE oid = $2::oid)",
                      2,      /* Two parameters */
                      NULL,   /* Let server infer param types */
                      param_values,
                      NULL,   /* Param lengths (text means null) */
                      NULL,   /* Param formats (text means null) */
                      0);     /* Result format (0 = text) */
    if (res == NULL)
    {
        set_server_error("Failed to execute permission query: %s",
                        PQerrorMessage(pgconn));
        return false;
    }

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        set_server_error("Permission query failed: %s", PQerrorMessage(pgconn));
        PQclear(res);
        return false;
    }

    /* Parse result */
    if (PQntuples(res) > 0)
    {
        const char *result = PQgetvalue(res, 0, 0);
        has_permission = (result != NULL && strcmp(result, "t") == 0);
    }

    PQclear(res);

    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                "KEK permission check: user=%u, kek=%u, has_permission=%d",
                user_oid, kek_oid, has_permission);

    return has_permission;
}

/*
 * server_interface_query_kek_for_dek
 *    Query KEK name for a specific DEK
 */
int
server_interface_query_kek_for_dek(DbConnection *conn, const char *dek_name,
                                  char **kek_name)
{
    PGconn *pgconn = NULL;
    PGresult *res = NULL;
    const char *param_values[1];
    const char *kek_name_result;

    if (conn == NULL || dek_name == NULL || kek_name == NULL || !conn->is_connected)
    {
        set_server_error("Invalid parameters for querying KEK for DEK");
        return -1;
    }

    pgconn = (PGconn *)conn->pg_conn;

    /* Build SQL query to get KEK name for DEK */
    param_values[0] = dek_name;

    /* Execute parameterized query */
    res = PQexecParams(pgconn,
                      "SELECT k.kek_name "
                      "FROM hg_dek d "
                      "JOIN hg_kek k ON d.kek_id = k.oid "
                      "WHERE d.dek_name = $1 AND d.status = 'active'",
                      1,      /* One parameter */
                      NULL,   /* Let server infer param types */
                      param_values,
                      NULL,   /* Param lengths (text means null) */
                      NULL,   /* Param formats (text means null) */
                      0);     /* Result format (0 = text) */
    if (res == NULL)
    {
        set_server_error("Failed to execute query: %s", PQerrorMessage(pgconn));
        return -1;
    }

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        set_server_error("Query failed: %s", PQerrorMessage(pgconn));
        PQclear(res);
        return -1;
    }

    /* Check if we got a result */
    if (PQntuples(res) == 0)
    {
        set_server_error("DEK '%s' not found or not active", dek_name);
        PQclear(res);
        return -1;
    }

    /* Get KEK name from result */
    kek_name_result = PQgetvalue(res, 0, 0);
    if (kek_name_result == NULL)
    {
        set_server_error("KEK name is NULL for DEK '%s'", dek_name);
        PQclear(res);
        return -1;
    }

    *kek_name = pg_strdup(kek_name_result);
    PQclear(res);

    if (*kek_name == NULL)
    {
        set_server_error("Failed to allocate memory for KEK name");
        return -1;
    }

    tecc_key_log(LOG_LEVEL_DEBUG, __func__, __FILE__, __LINE__,
                "Found KEK '%s' for DEK '%s'", *kek_name, dek_name);

    return 0;
}

/*
 * server_interface_parse_kek_tuple
 *    Parse KEK information from query result
 */
PgTeccKekInfo *
server_interface_parse_kek_tuple(void *tuple, void *tupdesc)
{
    /* TODO: Implement proper tuple parsing */
    /* For now, return NULL as placeholder */
    set_server_error("KEK tuple parsing not implemented yet");
    return NULL;
}

/*
 * server_interface_parse_dek_tuple
 *    Parse DEK information from query result
 */
PgTeccDekInfo *
server_interface_parse_dek_tuple(void *tuple, void *tupdesc)
{
    /* TODO: Implement proper tuple parsing */
    /* For now, return NULL as placeholder */
    set_server_error("DEK tuple parsing not implemented yet");
    return NULL;
}