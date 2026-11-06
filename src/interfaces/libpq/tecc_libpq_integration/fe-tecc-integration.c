#ifdef _MSC_VER
#define TECC_ATTR_UNUSED
#else
#define TECC_ATTR_UNUSED __attribute__((unused))
#endif

/*-------------------------------------------------------------------------
 *
 * fe-tecc-integration.c
 *	  TECC libpq integration module implementation
 *
 * Portions Copyright (c) 1996-2026, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/interfaces/libpq/fe-tecc-integration.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres_fe.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "common/fe_memutils.h"

#include "libpq-fe.h"
#include "libpq-int.h"
#include "fe-tecc-integration.h"
/* #include "fe-tecc-metadata.h" */  /* This header may have been replaced by tecc_metadata_cache.h */
/* #include "fe-tecc-key.h" */       /* This header may have been replaced by tecc_key_manager_adapter.h */
/* Note: fe-tecc-parser.h is not used - it's for the old frontend parser architecture */
/* #include "fe-tecc-parser.h" */

/* Include other TECC module headers */
#include "../tecc_cache/tecc_metadata_cache.h"
#include "../tecc_processor/tecc_processor.h"
#include "../tecc_processor/internal/tecc_processor_internal.h"  /* For tecc_processor_debug macro */
#include "../tecc_scheduler/tecc_crypto_scheduler.h"
#include "../tecc_scheduler/tecc_key_manager_adapter.h"
#include "../tecc_cache/tecc_metadata_cache.h"
#include "../tecc_data_fetcher/include/tecc_data_fetcher.h"
#include "../tecc_common/tecc_frontend_list.h"
#include "../tecc_cache/tecc_data_fetcher_adapter.h"

/* Include frontend parser headers */
#include "../frontend_parser/tecc_parser.h"
#include "../frontend_parser/tecc_nodes.h"
#include "../frontend_parser/tecc_error.h"

/* Include common types for algorithm type definitions */
#include "../../../common/tecc_types.h"

/* External declarations for key management functions from key_management.c */
extern int tecc_processor_get_kek_name_for_dek(PGconn *conn,
                                              const char *dek_name,
                                              char **kek_name);

/* Static function prototypes */
static TeccConfig tecc_integration_get_default_config(void);
static int tecc_load_metadata_cache(TeccState *state);
static void tecc_cleanup_metadata_cache(TeccState *state);
static void tecc_cleanup_key_cache(TeccState *state);
void tecc_record_error(TeccState *state, int error_code,
					   const char *error_msg, const char *function_name,
					   int line_number, bool recoverable);
/* tecc_record_debug function declaration removed - now defined in header file */
static char *tecc_strdup(const char *str);
static char *tecc_strdup_ex(TeccState *state, const char *str);
static char **tecc_duplicate_param_values(TeccState *state, const char *const *paramValues, int nParams);

/* Prepared statement helper functions */
static TeccPreparedStatementContext *tecc_create_prepared_statement_context(PGconn *conn);
static void tecc_free_prepared_statement_context(TeccPreparedStatementContext *context);

/* Pending statements management functions */
static void tecc_add_to_pending_statements(TeccState *state, const char *stmt_name, TeccPreparedStatementContext *context);
static void tecc_move_pending_to_prepared(TeccState *state);
static void tecc_cleanup_pending_list(TeccState *state);

/* GUC parameters processing functions */
static void tecc_apply_guc_params_to_query(TeccState *state, const char *query, char **processed_query);
static bool tecc_should_process_query_with_guc(const TeccState *state, const char *query);
/* GUC parameters processing functions */
static void tecc_apply_guc_params_to_query(TeccState *state, const char *query, char **processed_query);
static bool tecc_should_process_query_with_guc(const TeccState *state, const char *query);

/*
 * tecc_state_init - Initialize TECC state for a connection
 *
 * Returns: TeccState pointer on success, NULL on failure
 */
/* 连接池创建时通过设置 PGENABLETECC=false 环境变量阻止递归初始化，
 * 因此 tecc_state_init 不需要递归保护。 */

TeccState *
tecc_state_init(PGconn *conn)
{
	TeccState *state;

	if (conn == NULL)
	{
		return NULL;
	}

	/* Allocate state structure */
	state = (TeccState *) pg_malloc(sizeof(TeccState));
	if (state == NULL)
	{
		return NULL;
	}

	/* Initialize all fields to zero */
	memset(state, 0, sizeof(TeccState));

	/* Set basic state */
	state->enabled = conn->enable_tecc;
	state->initialized = false;
	state->conn = conn;

	/* Set connection's tecc_state pointer early to ensure it's always set
	 * even if function returns early due to errors or disabled state */
	conn->tecc_state = state;



	/* Set default configuration */
	state->config = tecc_integration_get_default_config();

	/* Initialize statistics */
	memset(&state->stats, 0, sizeof(TeccStats));

	/* Initialize error info */
	memset(&state->last_error, 0, sizeof(TeccErrorInfo));
	state->last_error.timestamp = time(NULL);

	/* Initialize new fields from openGauss PGClientLogic design */
	state->pending_statements = NULL;		/* Pending statements list */
	state->disable_once = false;			/* Temporary disable flag */
	memset(state->last_stmt_name, 0, sizeof(state->last_stmt_name)); /* Last statement name */
	state->last_result_status = PGRES_EMPTY_QUERY; /* Last result status */

	/* Initialize GUC parameters */
	memset(&state->guc_params, 0, sizeof(TeccGucParams));
	memset(&state->tmp_guc_params, 0, sizeof(TeccGucParams));
	memset(&state->val_to_update, 0, sizeof(TeccUpdateGucValues));

	/* Save conninfo for connection pool (数据获取器使用独立连接) */
	{
		/* 使用PGHOST、PGPORT等环境变量构建conninfo字符串 */
		/* 注意：不能在connectDBComplete过程中调用PQconninfo，因为连接尚未完全建立 */
		char host_buf[256], port_buf[32], db_buf[512], user_buf[256], pass_buf[256];
		char conninfo_buf[2048];
		const char *host, *port, *db, *user, *pass;

		/*
		 * Priority: actual connection parameters (PQhost/PQport/...)
		 * take precedence over environment variables.  The user
		 * explicitly connected with e.g. -p 5866, and the temp
		 * connection must use the same port — not whatever PGPORT
		 * happens to be set to in the environment.
		 */
		host = PQhost(conn);
		port = PQport(conn);
		db = PQdb(conn);
		user = PQuser(conn);
		pass = PQpass(conn);

		if (host == NULL || host[0] == '\0')
		{
			host = getenv("PGHOST");
			if (host == NULL || host[0] == '\0')
				host = "localhost";
		}
		if (port == NULL || port[0] == '\0')
		{
			port = getenv("PGPORT");
			if (port == NULL || port[0] == '\0')
				port = "5432";
		}
		if (db == NULL || db[0] == '\0')
		{
			db = getenv("PGDATABASE");
			if (db == NULL || db[0] == '\0')
				db = "postgres";
		}
		if (user == NULL || user[0] == '\0')
		{
			user = getenv("PGUSER");
			if (user == NULL || user[0] == '\0')
				user = "postgres";
		}
		if (pass == NULL || pass[0] == '\0')
		{
			pass = getenv("PGPASSWORD");
		}

		/* Escape special chars in conninfo values: \ → \\, ' → \' */
		{
			int i, j;
			for (i = 0, j = 0; host[i] && j < (int)sizeof(host_buf) - 4; i++, j++)
			{
				host_buf[j] = host[i];
				if (host[i] == '\\' || host[i] == '\'')
				{
					host_buf[j] = '\\';
					host_buf[++j] = host[i];
				}
			}
			host_buf[j] = '\0';
			for (i = 0, j = 0; port[i] && j < (int)sizeof(port_buf) - 4; i++, j++)
			{
				port_buf[j] = port[i];
				if (port[i] == '\\' || port[i] == '\'')
				{
					port_buf[j] = '\\';
					port_buf[++j] = port[i];
				}
			}
			port_buf[j] = '\0';
			for (i = 0, j = 0; db[i] && j < (int)sizeof(db_buf) - 4; i++, j++)
			{
				db_buf[j] = db[i];
				if (db[i] == '\\' || db[i] == '\'')
				{
					db_buf[j] = '\\';
					db_buf[++j] = db[i];
				}
			}
			db_buf[j] = '\0';
			for (i = 0, j = 0; user[i] && j < (int)sizeof(user_buf) - 4; i++, j++)
			{
				user_buf[j] = user[i];
				if (user[i] == '\\' || user[i] == '\'')
				{
					user_buf[j] = '\\';
					user_buf[++j] = user[i];
				}
			}
			user_buf[j] = '\0';
			for (i = 0, j = 0; pass && pass[i] && j < (int)sizeof(pass_buf) - 4; i++, j++)
			{
				pass_buf[j] = pass[i];
				if (pass[i] == '\\' || pass[i] == '\'')
				{
					pass_buf[j] = '\\';
					pass_buf[++j] = pass[i];
				}
			}
			if (pass && pass[0]) pass_buf[j] = '\0';
		}

		snprintf(conninfo_buf, sizeof(conninfo_buf),
				 "host='%s' port='%s' dbname='%s' user='%s'%s%s%s",
				 host_buf, port_buf, db_buf, user_buf,
				 (pass && pass[0]) ? " password='" : "",
				 (pass && pass[0]) ? pass_buf : "",
				 (pass && pass[0]) ? "'" : "");

		state->conninfo = pg_strdup(conninfo_buf);

	}
	/* Set default GUC values */
	/* Begin - ReqID:TECC4.0 - 设置简化GUC参数的默认值 */
	strcpy(state->guc_params.role, "");		/* Empty role by default */
	state->guc_params.standard_conforming_strings = true; /* Enable by default */
	state->guc_params.backslash_quote = TECC_BACKSLASH_QUOTE_SAFE_ENCODING; /* Safe encoding by default */
	state->guc_params.escape_string_warning = true; /* Enable warning by default */
	strcpy(state->guc_params.searchpath, "\"$user\", public"); /* Default search path */
	/* End - ReqID:TECC4.0 - 设置简化GUC参数的默认值 */

	/* Check if TECC is enabled */
	if (!state->enabled)
	{
		state->initialized = true;
		return state;
	}

	/* Initialize data fetcher FIRST - needed for cache loading */
	/* Begin - ReqID:TECC4.0 - 调整初始化顺序：先初始化数据获取器 */
	{
		/* 线程局部递归深度计数器，防止池连接创建时无限递归 */
		static TECC_THREAD_LOCAL int fetcher_init_depth = 0;

		if (fetcher_init_depth == 0)
		{
			fetcher_init_depth++;
			if (!tecc_data_fetcher_init_for_state(state))
			{
				/* Record error but continue - data fetcher is optional for basic operations */
				tecc_record_error(state, TECC_ERR_DATA_FETCHER,
							 "Failed to initialize data fetcher, cache loading from backend will be disabled",
							 __func__, __LINE__, true);
			}
			fetcher_init_depth--;
		}
		else
		{
			/* 递归调用（池连接PQconnectdb触发）：跳过数据获取器初始化 */
			TECC_LOG(TECC_LOG_DEBUG1, "TECC STATE INIT: skipping data fetcher init (recursion depth=%d)",
					fetcher_init_depth);
			state->data_fetcher_initialized = true;
		}
	}
	/* End - ReqID:TECC4.0 - 调整初始化顺序：先初始化数据获取器 */

	/* Load metadata cache */
	if (tecc_load_metadata_cache(state) < 0)
	{
		/* Record error but continue with disabled TECC */
		tecc_record_error(state, TECC_ERR_CACHE_LOAD,
						 "Failed to load metadata cache",
						 __func__, __LINE__, true);
		state->enabled = false;
		state->initialized = true;
		return state;
	}


	/* Initialize processor context */
	/* Begin - ReqID:TECC4.0 - 修复处理器初始化问题 */
	/* Always initialize processor even if metadata cache is NULL */
	/* tecc_state pointer already set at beginning of function */
	/* Begin - ReqID:TECC4.0 - 调试日志添加 */
	tecc_processor_debug(TECC_LOG_DEBUG1,
						"DEBUG: Before calling tecc_processor_init, state->conn=%p, state=%p",
						state->conn, state);
	/* End - ReqID:TECC4.0 - 调试日志添加 */
	tecc_processor_init(state->conn);
	/* Begin - ReqID:TECC4.0 - 调试日志添加 */
	tecc_processor_debug(TECC_LOG_DEBUG1,
						"DEBUG: After calling tecc_processor_init, state->processor_initialized=%d",
						state->processor_initialized);
	/* End - ReqID:TECC4.0 - 调试日志添加 */
	state->processor_ctx = state->conn;  /* Store connection pointer for reference */
	/* End - ReqID:TECC4.0 - 修复处理器初始化问题 */

	/* Initialize crypto context */
	/* Begin - ReqID:TECC4.0 - 修复加密调度器初始化问题 */
	/* Always try to initialize crypto context even if metadata cache is NULL */
	/* Initialize key manager adapter */
	state->key_mgr = tecc_key_manager_adapter_create_with_state(state);
	if (state->key_mgr == NULL)
	{
		tecc_record_error(state, TECC_ERR_KEY_MANAGER,
						 "Failed to create key manager adapter",
						 __func__, __LINE__, true);
		/* Continue without key manager - TECC will be limited */
		state->crypto_ctx = NULL;
	}
	else
	{
		/* Initialize crypto scheduler with key manager */
		state->crypto_scheduler = tecc_crypto_scheduler_init(state->metadata_cache, state->key_mgr);
		/* Check if crypto scheduler initialization failed */
		if (state->crypto_scheduler == NULL)
		{
			tecc_record_error(state, TECC_ERR_KEY_MANAGER,
							 "Failed to initialize crypto scheduler",
							 __func__, __LINE__, true);
			/* Clean up key manager but continue without crypto context */
			tecc_key_manager_adapter_destroy((TeccKeyManagerAdapter *)state->key_mgr);
			state->key_mgr = NULL;
		}
		else
		{
			state->scheduler_initialized = true;
			tecc_record_debug(state, "Crypto scheduler initialized successfully, crypto_scheduler=%p, key_mgr=%p",
							state->crypto_scheduler, state->key_mgr);
		}
	}

	/* Create per-operation crypto context (reusable across queries) */
	state->crypto_ctx = tecc_create_crypto_context_with_pool(NULL, NULL, NULL, NULL);
	if (state->crypto_ctx == NULL)
	{
		tecc_record_error(state, TECC_ERR_KEY_MANAGER,
					 "Failed to create crypto context",
					 __func__, __LINE__, true);
	}
	/* End - ReqID:TECC4.0 - 修复加密调度器初始化问题 */

	state->initialized = true;

	return state;
}

/* Begin - ReqID:TECC4.0 */
/*
 * PGresult block recycling — eliminates repeated malloc/free for
 * PGresult_data blocks within a connection's lifetime.
 *
 * Pool lifetime is independent of PGconn: when a connection closes,
 * blocks in the pool are freed but the pool struct itself (16 bytes)
 * persists.  PGresult objects reference the pool directly, not through
 * PGconn, so they can safely call free() even after the connection
 * is closed — the pool simply has an empty free list (see
 * tecc_state_cleanup).
 *
 * Only standard-sized blocks (8192 bytes) are recycled.  Large-object
 * blocks have different sizes and are freed directly.
 */
#include <malloc.h>

#define TECC_BLOCK_FREE_MAX  (256 * 1024 * 1024 / 8192)

void *
tecc_pgresult_block_alloc(TeccBlockPool *pool)
{
	PGresult_data *block;

	if (pool == NULL)
		return malloc(8192);

	if (pool->free_blocks)
	{
		block = (PGresult_data *)pool->free_blocks;
		pool->free_blocks = block->next;
		pool->free_count--;
		return (void *)block;
	}
	return malloc(8192);
}

void
tecc_pgresult_block_free(TeccBlockPool *pool, void *block_ptr)
{
	PGresult_data *block = (PGresult_data *)block_ptr;
	size_t usable;

	if (pool == NULL)
	{
		free(block_ptr);
		return;
	}

	/*
	 * Safety: if the pool's free list is empty AND free_count is 0,
	 * the pool may have been zeroed by tecc_state_cleanup's memset
	 * (which clears TeccState including the block_pool field).
	 * In that case the pool pointer itself may point into freed memory,
	 * so we must not write into it — just free the block directly.
	 *
	 * A freshly initialized pool also has (NULL, 0), but that path
	 * is indistinguishable from a cleaned-up one; both are safe to
	 * just free the block since there's nothing to recycle.
	 */
	if (pool->free_blocks == NULL && pool->free_count == 0)
	{
		free(block_ptr);
		return;
	}

#ifdef _WIN32
	usable = _msize(block_ptr);
#else
	usable = malloc_usable_size(block_ptr);
#endif
	if (usable < 8192 || usable > 8192 + 32)
	{
		free(block_ptr);
		return;
	}

	if (pool->free_count < TECC_BLOCK_FREE_MAX)
	{
		block->next = pool->free_blocks;
		pool->free_blocks = (void *)block;
		pool->free_count++;
	}
	else
	{
		free(block_ptr);
	}
}
/* End - ReqID:TECC4.0 */

/*
 * tecc_state_cleanup - Clean up TECC state for a connection
 *
 * 修复方案：添加空指针检查，完善清理顺序，安全擦除敏感数据
 */
void
tecc_state_cleanup(TeccState *state)
{
	PGconn *conn;

	/* 添加空指针检查 */
	if (state == NULL)
	{
		tecc_record_debug(NULL, "tecc_state_cleanup called with NULL state");
		return;
	}

	/* 记录清理开始（用于调试） */
	tecc_record_debug(state, "Starting TECC state cleanup");

	conn = state->conn;

	/* 清理数据获取器（新增字段） */
	if (state->data_fetcher_initialized)
	{
		tecc_data_fetcher_cleanup_for_state(state);
		state->data_fetcher_initialized = false;
	}

	/* 清理调度器（新增字段） */
	if (state->scheduler_initialized && state->crypto_scheduler != NULL)
	{
		tecc_crypto_scheduler_cleanup(state->crypto_scheduler);
		state->crypto_scheduler = NULL;
		state->scheduler_initialized = false;
	}

	/* 清理处理器（新增字段） */
	if (state->processor_initialized)
	{
		tecc_processor_cleanup(state->conn);
		state->processor_initialized = false;
	}

	/* Clean up metadata cache */
	tecc_cleanup_metadata_cache(state);

	/* Clean up key cache */
	tecc_cleanup_key_cache(state);

	/* Clean up processor context (旧字段，向后兼容) */
	if (state->processor_ctx != NULL)
	{
		tecc_processor_cleanup(state->conn);
		state->processor_ctx = NULL;
	}

	/* Clean up per-operation crypto context */
	if (state->crypto_ctx != NULL)
	{
		tecc_destroy_crypto_context(state->crypto_ctx);
		state->crypto_ctx = NULL;
	}

	/* Clean up key manager (only if not already cleaned up by crypto context) */
	if (state->key_mgr != NULL)
	{
		tecc_key_manager_adapter_destroy((TeccKeyManagerAdapter *)state->key_mgr);
		state->key_mgr = NULL;
	}

	/* Clean up pending statements (new field from openGauss design) */
	tecc_cleanup_pending_list(state);

	if (state->conninfo != NULL)
	{
		pg_free(state->conninfo);
		state->conninfo = NULL;
	}

	/* ReqID:TECC4.0 - Clean up pending tables bridge */
	/* pending_tables_clear removed - PENDING sentinel in name hash, cleaned up with cache */

	/* ReqID:TECC4.0 - Clean up block recycling pool.
	 * Free all blocks but keep the pool struct alive — PGresult objects
	 * may still reference it and will safely see an empty free list. */
	if (state->block_pool.free_blocks)
	{
		PGresult_data *b = (PGresult_data *)state->block_pool.free_blocks;

		while (b)
		{
			PGresult_data *next = b->next;

			free(b);
			b = next;
		}
		state->block_pool.free_blocks = NULL;
		state->block_pool.free_count = 0;
	}

	/* Reset state tracking fields */
	state->disable_once = false;
	state->last_stmt_name[0] = '\0';
	state->last_result_status = PGRES_EMPTY_QUERY; /* Default empty status */

	/*
	 * Clear block_pool in any PGresult that still references
	 * state->block_pool.  After we memset+free(state), those
	 * pointers become dangling — a subsequent tecc_pgresult_block_free
	 * would write into freed memory → heap corruption.
	 */
	if (conn != NULL)
	{
		/* Direct result pointers held by the connection */
		if (conn->result != NULL)
			((PGresult *)conn->result)->block_pool = NULL;
		if (conn->next_result != NULL)
			((PGresult *)conn->next_result)->block_pool = NULL;
	}

	/* Clear connection's tecc_state pointer */
	if (conn != NULL && conn->tecc_state == state)
	{
		conn->tecc_state = NULL;
	}

	/* 安全擦除敏感数据 */
	memset(state, 0, sizeof(TeccState));

	/* Free state structure */
	tecc_pfree(state);

	/* 记录清理完成 */
	tecc_record_debug(NULL, "TECC state cleanup completed");
}

/* tecc_pending_tables_clear removed */


/* tecc_remove_pending_table removed */


/* tecc_save_pending_table removed */


/*
 * tecc_refresh_table_cache - Refresh encrypted column cache for a DDL-created table
 *
 * Called in tecc_process_result after a successful CREATE TABLE/ALTER TABLE
 * DDL commits. Uses the main connection (with TECC temporarily disabled) to
 * query hg_encrypted_columns_info and load the complete metadata into the
 * formal cache, replacing the PENDING sentinel that was inserted during
 * preprocessing.
 *
 * Using the main connection ensures the query sees the DDL's effects even
 * within an uncommitted transaction (same-connection visibility).
 */
static void
tecc_refresh_table_cache(TeccState *state, PGconn *conn,
                          const char *db_name,
                          const char *schema_name,
                          const char *table_name)
{
    PGresult   *res;
    bool        saved_enable_tecc;
    char        query[8192];
    char       *esc_table, *esc_schema;
    int         ntuples;
    int         i;

    if (state == NULL || conn == NULL || db_name == NULL ||
        schema_name == NULL || table_name == NULL)
    {
        tecc_processor_debug(TECC_LOG_DEBUG1,
            "tecc_refresh_table_cache: skipped (null param)");
        return;
    }

    tecc_processor_debug(TECC_LOG_DEBUG1,
        "tecc_refresh_table_cache: refreshing %s.%s (db=%s)",
        schema_name, table_name, db_name);

    /* Escape identifiers for SQL */
    esc_table = PQescapeLiteral(conn, table_name, strlen(table_name));
    esc_schema = PQescapeLiteral(conn, schema_name, strlen(schema_name));
    if (esc_table == NULL || esc_schema == NULL)
    {
        if (esc_table) PQfreemem(esc_table);
        if (esc_schema) PQfreemem(esc_schema);
        return;
    }

    /* Build query matching the data fetcher's pattern */
    snprintf(query, sizeof(query),
        "SELECT cls.oid AS table_oid, column_attnum, column_name, "
        "data_type_original_oid, data_type_original_mod, dek_name, "
        "encryption_type, collation_locale, column_key_id, create_date, "
        "has_dek_privilege(column_key_id, 'USAGE') AS has_dek_permission "
        "FROM hg_encrypted_columns_info "
        "JOIN pg_class cls ON table_oid = cls.oid "
        "JOIN pg_namespace ns ON cls.relnamespace = ns.oid "
        "WHERE cls.relname = %s AND ns.nspname = %s "
        "AND current_database() = '%s' "
        "ORDER BY column_attnum",
        esc_table, esc_schema, db_name);

    PQfreemem(esc_table);
    PQfreemem(esc_schema);

    /* Disable TECC preprocessing to prevent recursion */
    saved_enable_tecc = conn->enable_tecc;
    conn->enable_tecc = false;
    res = PQexec(conn, query);
    conn->enable_tecc = saved_enable_tecc;

    if (res == NULL || PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        tecc_processor_debug(TECC_LOG_WARNING,
            "tecc_refresh_table_cache: query failed for %s.%s",
            schema_name, table_name);
        if (res) PQclear(res);
        return;
    }

    ntuples = PQntuples(res);
    if (ntuples == 0)
    {
        /* No encrypted columns found — remove PENDING sentinel only */
        tecc_cache_remove_pending(state, db_name, schema_name, table_name);
        PQclear(res);
        return;
    }

    /* Parse results and insert into formal cache */
    for (i = 0; i < ntuples; i++)
    {
        TeccColumnCacheItem *item = (TeccColumnCacheItem *)pg_malloc0(sizeof(TeccColumnCacheItem));
        if (item == NULL)
            continue;

        item->cache_status = TECC_CACHE_STATUS_ENCRYPTED;
        item->database_name = pg_strdup(db_name);
        item->schema_name = pg_strdup(schema_name);
        item->table_name = pg_strdup(table_name);

        /* table_oid */
        if (PQgetisnull(res, i, 0) == 0)
            item->table_oid = (Oid)atol(PQgetvalue(res, i, 0));

        /* column_attnum */
        if (PQgetisnull(res, i, 1) == 0)
            item->column_attnum = atoi(PQgetvalue(res, i, 1));

        /* column_name */
        if (PQgetisnull(res, i, 2) == 0)
            item->column_name = pg_strdup(PQgetvalue(res, i, 2));

        /* data_type_original_oid */
        if (PQgetisnull(res, i, 3) == 0)
            item->original_type_oid = atoi(PQgetvalue(res, i, 3));

        /* data_type_original_mod */
        if (PQgetisnull(res, i, 4) == 0)
            item->original_type_mod = atoi(PQgetvalue(res, i, 4));

        /* dek_name */
        if (PQgetisnull(res, i, 5) == 0)
            item->dek_name = pg_strdup(PQgetvalue(res, i, 5));

        /* encryption_type */
        if (PQgetisnull(res, i, 6) == 0)
            item->encryption_type = atoi(PQgetvalue(res, i, 6));

        /* collation_locale */
        if (PQgetisnull(res, i, 7) == 0)
        {
            const char *loc = PQgetvalue(res, i, 7);
            if (loc[0] != '\0')
                item->collation_locale = pg_strdup(loc);
        }

        /* column_key_id (dek_id) */
        if (PQgetisnull(res, i, 8) == 0)
            item->dek_id = atoi(PQgetvalue(res, i, 8));

        /* create_date */
        if (PQgetisnull(res, i, 9) == 0)
            item->create_date = atol(PQgetvalue(res, i, 9));

        /* has_dek_permission */
        if (PQgetisnull(res, i, 10) == 0)
        {
            const char *perm = PQgetvalue(res, i, 10);
            item->has_dek_permission = (strcmp(perm, "t") == 0);
        }
        item->dek_permission_checked = true;

        /* Insert into formal cache */
        {
            int insert_ret = tecc_cache_insert_column_item(state, item);
            if (insert_ret != 0)
            {
                tecc_processor_debug(TECC_LOG_WARNING,
                    "tecc_refresh_table_cache: insert_column_item failed ret=%d for %s.%s.%s",
                    insert_ret, schema_name, table_name,
                    item->column_name ? item->column_name : "?");
            }
        }
    }

    /* Insert name sentinel */
    {
        TeccColumnCacheItem *name_sentinel;

        name_sentinel = (TeccColumnCacheItem *)pg_malloc0(sizeof(TeccColumnCacheItem));
        if (name_sentinel != NULL)
        {
            name_sentinel->cache_status = TECC_CACHE_STATUS_TABLE_LOADED;
            name_sentinel->database_name = pg_strdup(db_name);
            name_sentinel->schema_name = pg_strdup(schema_name);
            name_sentinel->table_name = pg_strdup(table_name);
            name_sentinel->column_name = pg_strdup("");
            tecc_cache_insert_column_item(state, name_sentinel);
        }
    }

    /* Insert OID sentinel */
    if (ntuples > 0 && PQgetisnull(res, 0, 0) == 0)
    {
        Oid tbl_oid = (Oid)atol(PQgetvalue(res, 0, 0));
        TeccColumnCacheItem *oid_sentinel;

        oid_sentinel = (TeccColumnCacheItem *)pg_malloc0(sizeof(TeccColumnCacheItem));
        if (oid_sentinel != NULL)
        {
            oid_sentinel->cache_status = TECC_CACHE_STATUS_NOT_ENCRYPTED;
            oid_sentinel->table_oid = tbl_oid;
            oid_sentinel->column_attnum = 0;
            tecc_cache_insert_column_item(state, oid_sentinel);
        }
    }

    PQclear(res);

    /* Remove PENDING sentinel — now replaced by real cache entries */
    tecc_cache_remove_pending(state, db_name, schema_name, table_name);

    tecc_processor_debug(TECC_LOG_DEBUG1,
        "tecc_refresh_table_cache: loaded %d encrypted columns for %s.%s",
        ntuples, schema_name, table_name);
}

/*
 * tecc_refresh_kek_cache - Load committed KEK metadata into formal cache.
 *
 * Called after CREATE KEK succeeds. Uses the data fetcher with the main
 * connection (enable_tecc disabled) to load full KEK metadata and replace
 * the PENDING sentinel. Same pattern as tecc_refresh_table_cache.
 */
static void
tecc_refresh_kek_cache(TeccState *state, PGconn *conn,
                       const char *kek_name, const char *kek_namespace)
{
    TeccCacheQueryParams query;
    TeccKekCacheItem *item = NULL;
    int count = 0;
    TeccFetcherErrorCode ret;
    int insert_ret;

    if (state == NULL || conn == NULL || kek_name == NULL ||
        kek_name[0] == '\0' || kek_namespace == NULL)
        return;

    tecc_processor_debug(TECC_LOG_DEBUG1,
        "tecc_refresh_kek_cache: loading %s.%s", kek_namespace, kek_name);

    memset(&query, 0, sizeof(query));
    query.flags = TECC_QUERY_BY_NAME;
    query.params.by_name.name = kek_name;
    query.params.by_name.namespace = kek_namespace;
    query.external_conn = conn;

    {
        bool saved_enable_tecc = conn->enable_tecc;
        conn->enable_tecc = false;
        ret = tecc_fetch_kek_for_cache(state, &query, &item, &count);
        conn->enable_tecc = saved_enable_tecc;
    }

    if (ret == TECC_CACHE_OK && item != NULL)
    {
        tecc_cache_remove_pending_kek(state, kek_name, kek_namespace);
        insert_ret = tecc_cache_insert_kek_item(state, item);
        if (insert_ret != 0)
        {
            tecc_processor_debug(TECC_LOG_WARNING,
                "tecc_refresh_kek_cache: insert failed ret=%d for %s.%s",
                insert_ret, kek_namespace, kek_name);
        }
        tecc_processor_debug(TECC_LOG_DEBUG1,
            "tecc_refresh_kek_cache: loaded KEK %s.%s (oid=%u)",
            kek_namespace, kek_name, item->kek_oid);
    }
    else
    {
        tecc_processor_debug(TECC_LOG_WARNING,
            "tecc_refresh_kek_cache: fetch failed ret=%d for %s.%s",
            ret, kek_namespace, kek_name);
    }
}

/*
 * tecc_refresh_dek_cache - Load committed DEK metadata into formal cache.
 *
 * Called after CREATE DEK succeeds. Same pattern as tecc_refresh_kek_cache.
 */
static void
tecc_refresh_dek_cache(TeccState *state, PGconn *conn,
                       const char *dek_name, const char *dek_namespace)
{
    TeccCacheQueryParams query;
    TeccDekCacheItem *item = NULL;
    int count = 0;
    TeccFetcherErrorCode ret;
    int insert_ret;

    if (state == NULL || conn == NULL || dek_name == NULL ||
        dek_name[0] == '\0' || dek_namespace == NULL)
        return;


    memset(&query, 0, sizeof(query));
    query.flags = TECC_QUERY_BY_NAME;
    query.params.by_name.name = dek_name;
    query.params.by_name.namespace = dek_namespace;
    query.external_conn = conn;

    {
        bool saved_enable_tecc = conn->enable_tecc;
        conn->enable_tecc = false;
        ret = tecc_fetch_dek_for_cache(state, &query, &item, &count);
        conn->enable_tecc = saved_enable_tecc;
    }

    if (ret == TECC_CACHE_OK && item != NULL)
    {
        tecc_cache_remove_pending_dek(state, dek_name, dek_namespace);
        insert_ret = tecc_cache_insert_dek_item(state, item);
        if (insert_ret != 0)
        {
            tecc_processor_debug(TECC_LOG_WARNING,
                "tecc_refresh_dek_cache: insert failed ret=%d for %s.%s",
                insert_ret, dek_namespace, dek_name);
        }
        tecc_processor_debug(TECC_LOG_DEBUG1,
            "tecc_refresh_dek_cache: loaded DEK %s.%s (oid=%u)",
            dek_namespace, dek_name, item->dek_oid);
    }
    else
    {
        tecc_processor_debug(TECC_LOG_WARNING,
            "tecc_refresh_dek_cache: fetch failed ret=%d for %s.%s",
            ret, dek_namespace, dek_name);
    }
}

/*




/*
 * tecc_is_enabled - Check if TECC is enabled for a state
 *
 * This function checks if TECC is enabled, considering the temporary
 * disable_once flag. If disable_once is true, it returns false.
 * Note: The disable_once flag should be reset by the caller after
 * the operation that should be skipped.
 *
 * Returns: true if TECC is enabled and initialized, false otherwise
 */
bool
tecc_is_enabled(const TeccState *state)
{
	bool enabled;

	/* Safety check: ensure state pointer is valid */
	if (state == NULL)
	{
		tecc_processor_debug(TECC_LOG_DEBUG1,
							"tecc_is_enabled: state is NULL");
		return false;
	}

	/* Debug: log state pointer and check if it looks valid */
	tecc_processor_debug(TECC_LOG_DEBUG2,
						"tecc_is_enabled: state=%p, checking disable_once flag",
						state);

	/* Check if TECC is temporarily disabled */
	if (state->disable_once)
	{
		tecc_processor_debug(TECC_LOG_DEBUG1,
							"tecc_is_enabled: TECC temporarily disabled via disable_once flag");
		return false;
	}

	enabled = state->enabled && state->initialized;

	tecc_processor_debug(TECC_LOG_DEBUG1,
					   "DEBUG: tecc_is_enabled: state=%p, enabled=%d, initialized=%d, disable_once=%d, final enabled=%d",
					   state, state->enabled, state->initialized, state->disable_once, enabled);

	return enabled;
}

/* ReqID:TECC4.0 - CASCADE KEK file cleanup helper functions */

/*
 * tecc_is_cascade_kek_drop - Check if query is DROP SCHEMA/OWNED ... CASCADE
 *
 * Uses simple text matching since the TECC frontend parser does not
 * handle OBJECT_SCHEMA/OBJECT_OWNED. Returns true if the SQL text
 * matches "DROP {SCHEMA|OWNED} ... CASCADE".
 */
static bool
tecc_is_cascade_kek_drop(const char *query)
{
    const char *p;

    if (query == NULL || query[0] == '\0')
        return false;

    p = query;
    /* Skip leading whitespace */
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
        p++;

    /* Must start with DROP */
    if (strncasecmp(p, "DROP", 4) != 0)
        return false;
    p += 4;

    /* Skip whitespace after DROP */
    while (*p == ' ' || *p == '\t')
        p++;

    /* Must be SCHEMA or OWNED */
    if (strncasecmp(p, "SCHEMA", 6) != 0 &&
        strncasecmp(p, "OWNED", 5) != 0 &&
        strncasecmp(p, "SCHEMAS", 7) != 0)
        return false;
    p++; /* Advance past first char of the keyword, we'll skip the rest below */

    /* Skip the rest of the keyword (SCHEMA/OWNED/SCHEMAS) */
    while (*p != '\0' && *p != ' ' && *p != '\t')
        p++;

    /* Look for CASCADE anywhere after the keyword */
    while (*p != '\0')
    {
        if (strncasecmp(p, "CASCADE", 7) == 0)
            return true;
        p++;
    }

    return false;
}

/*
 * tecc_capture_all_kek_paths - Pre-snapshot all KEY_PATH values from server
 *
 * Uses a temp connection (TECC-disabled) to query hg_kek_args for all
 * plaintext KEY_PATH values. Stores them in state->cascade_kek_paths[]
 * for later diff-and-unlink after CASCADE DDL succeeds.
 *
 * This must be called BEFORE the DDL executes, because after CASCADE
 * the KEK rows are gone.
 */
static void
tecc_capture_all_kek_paths(TeccState *state)
{
    PGconn *temp_conn;
    PGresult *res;
    int nrows, i;

    if (state == NULL)
        return;

    /* Reset first */
    state->cascade_kek_path_count = 0;

    temp_conn = tecc_create_temp_connection(state, "cascade_capture");
    if (temp_conn == NULL)
    {
        tecc_record_debug(state,
            "CASCADE capture: temp connection failed, skipping");
        return;
    }

    res = PQexec(temp_conn,
        "SELECT DISTINCT encode(value, 'escape') "
        "FROM hg_kek_args "
        "WHERE function_name = 'kek_config' "
        "AND (key = 'KEY_PATH' OR key = 'key_path')");

    if (PQresultStatus(res) == PGRES_TUPLES_OK)
    {
        nrows = PQntuples(res);
        if (nrows > TECC_MAX_CASCADE_KEK_PATHS)
            nrows = TECC_MAX_CASCADE_KEK_PATHS;

        for (i = 0; i < nrows; i++)
        {
            const char *val = PQgetvalue(res, i, 0);
            if (val != NULL && val[0] != '\0')
            {
                strncpy(state->cascade_kek_paths[i], val,
                        sizeof(state->cascade_kek_paths[i]) - 1);
                state->cascade_kek_paths[i][sizeof(state->cascade_kek_paths[i]) - 1] = '\0';
            }
        }
        state->cascade_kek_path_count = nrows;

        tecc_record_debug(state,
            "CASCADE KEK capture: snapshot %d KEK paths", nrows);
    }
    else
    {
        tecc_record_debug(state,
            "CASCADE KEK capture: query failed: %s",
            PQerrorMessage(temp_conn));
    }

    PQclear(res);
    tecc_close_temp_connection(temp_conn, "cascade_capture");
}

/*
 * tecc_cleanup_cascade_kek_files - Diff pre-snapshot vs current, unlink deleted
 *
 * Called after a CASCADE DDL succeeds. Queries the current (post-DDL) set
 * of KEK paths via a temp connection, diffs against the pre-snapshot, and
 * unlinks() local KEK files whose server-side KEK is gone.
 */
static void
tecc_cleanup_cascade_kek_files(TeccState *state)
{
    PGconn *temp_conn;
    PGresult *res;
    int nrows, i;

    if (state == NULL || state->cascade_kek_path_count <= 0)
        return;

    tecc_record_debug(state,
        "CASCADE KEK cleanup: checking %d pre-snapshot paths",
        state->cascade_kek_path_count);

    temp_conn = tecc_create_temp_connection(state, "cascade_cleanup");
    if (temp_conn == NULL)
    {
        tecc_record_debug(state,
            "CASCADE KEK cleanup: temp connection failed, skipping");
        state->cascade_kek_path_count = 0;
        return;
    }

    /* Query current (post-DDL) set of KEK paths */
    res = PQexec(temp_conn,
        "SELECT encode(value, 'escape') "
        "FROM hg_kek_args "
        "WHERE function_name = 'kek_config' "
        "AND (key = 'KEY_PATH' OR key = 'key_path')");

    if (PQresultStatus(res) == PGRES_TUPLES_OK)
    {
        nrows = PQntuples(res);

        /* For each pre-snapshot path, check if still alive */
        for (i = 0; i < state->cascade_kek_path_count; i++)
        {
            bool still_alive = false;
            int j;

            if (state->cascade_kek_paths[i][0] == '\0')
                continue;

            for (j = 0; j < nrows; j++)
            {
                const char *cur = PQgetvalue(res, j, 0);
                if (cur != NULL &&
                    strcmp(state->cascade_kek_paths[i], cur) == 0)
                {
                    still_alive = true;
                    break;
                }
            }

            if (!still_alive)
            {
                /* KEK was deleted by CASCADE - unlink local file */
                if (unlink(state->cascade_kek_paths[i]) != 0 && errno != ENOENT)
                {
                    tecc_record_debug(state,
                        "CASCADE KEK: Failed to delete file '%s': %s",
                        state->cascade_kek_paths[i], strerror(errno));
                }
                else
                {
                    tecc_record_debug(state,
                        "CASCADE KEK: Deleted file '%s'",
                        state->cascade_kek_paths[i]);
                }
            }
        }
    }
    else
    {
        tecc_record_debug(state,
            "CASCADE KEK cleanup: query failed: %s",
            PQerrorMessage(temp_conn));
    }

    PQclear(res);
    tecc_close_temp_connection(temp_conn, "cascade_cleanup");

    /* Clear state */
    state->cascade_kek_path_count = 0;
}

/*
 * tecc_preprocess_query - Preprocess a regular query
 *
 * Returns: Rewritten query string on success, NULL if no rewrite needed
 */
char *
tecc_preprocess_query(PGconn *conn, const char *query)
{
	TeccState *state;
	char *rewritten_query = NULL;
	bool enabled;

	if (conn == NULL || query == NULL)
	{
		return NULL;
	}

	state = tecc_get_state(conn);

	/* already logged below via tecc_processor_debug */

	/* Debug: log state pointer */
	tecc_processor_debug(TECC_LOG_DEBUG1,
						"tecc_preprocess_query: state=%p, conn=%p, conn->tecc_state=%p, query='%.*s'",
						state, conn, conn ? conn->tecc_state : NULL,
						query ? (int)strnlen(query, 100) : 0, query ? query : "(null)");

	/* Clear any stale error from previous query processing */
	if (state != NULL)
	{
		memset(&state->last_error, 0, sizeof(TeccErrorInfo));
		state->last_error.timestamp = time(NULL);
	}

	/* DEBUG: Check if TECC is enabled */
	enabled = tecc_is_enabled(state);
	if (state != NULL)
	{
		tecc_processor_debug(TECC_LOG_DEBUG1,
							"DEBUG: tecc_preprocess_query: tecc_is_enabled returned %d (state->enabled=%d, state->initialized=%d, state->disable_once=%d)",
							enabled, state->enabled, state->initialized, state->disable_once);
	}



	/* ReqID:TECC4.0 - Deferred DDL cache refresh */
	if (state != NULL &&
	    state->last_stmt_type == TECC_STMT_TYPE_CREATE_TABLE &&
	    state->last_table_name[0] != '\0')
	{
	    const char *db_name = PQdb(conn);
	    if (db_name == NULL) db_name = "postgres";
	    tecc_refresh_table_cache(state, conn, db_name,
	        state->last_table_schema,
	        state->last_table_name);
	    state->last_stmt_type = TECC_STMT_TYPE_UNKNOWN;
	}

	/* Begin - ReqID:TECC4.0 - Intercept SET search_path to update GUC cache
	 * TECC server does not send search_path ParameterStatus (no GUC_REPORT),
	 * so PQparameterStatus cannot be used. We must intercept SET search_path
	 * at the preprocess stage and update the cache directly. */
	if (state != NULL && query != NULL)
	{
		const char *sp = query;
		/* Skip leading whitespace */
		while (*sp == ' ' || *sp == '\t' || *sp == '\n' || *sp == '\r')
			sp++;
		if (strncasecmp(sp, "SET", 3) == 0)
		{
			const char *p = sp + 3;
			/* Skip whitespace after SET */
			while (*p == ' ' || *p == '\t')
				p++;
			/* Check for search_path (case-insensitive) */
			if (strncasecmp(p, "search_path", 11) == 0)
			{
				const char *eq = p + 11;
				/* Skip whitespace before = or TO */
				while (*eq == ' ' || *eq == '\t')
					eq++;
				/* Handle "=" or "TO" syntax */
				if (*eq == '=')
				{
					eq++;
				}
				else if (strncasecmp(eq, "TO", 2) == 0)
				{
					eq += 2;
				}
				else
					eq = NULL;

				if (eq != NULL)
				{
					/* Skip whitespace before value */
					while (*eq == ' ' || *eq == '\t')
						eq++;
					if (*eq != '\0' && *eq != ';')
					{
						const char *end;
						char *value;
						/* Strip trailing semicolon and whitespace */
						end = eq + strlen(eq);
						while (end > eq && (end[-1] == ';' || end[-1] == ' ' || end[-1] == '\t'))
							end--;
						value = (char *)pg_malloc(end - eq + 1);
						if (value != NULL)
						{
							memcpy(value, eq, end - eq);
							value[end - eq] = '\0';
							tecc_processor_debug(TECC_LOG_DEBUG1,
								"tecc_preprocess_query: SET search_path -> '%s'", value);
							tecc_cache_update_search_path(state, value);
							pg_free(value);
						}
					}
				}
			}
		}
	}
	/* End - ReqID:TECC4.0 - Intercept SET search_path */
		/*
	 * ReqID:TECC4.0 - Special handling for CREATE KEK/DEK statements
	 * These key management statements need frontend processing even when
	 * TECC is not fully initialized, because the frontend processor generates
	 * encryption keys and rewrites the SQL for backend storage.
	 */
	if (!enabled && state && state->enabled)
	{
		const char *query_upper = query;
		/* Skip leading whitespace */
		while (*query_upper && (*query_upper == '\0' || *query_upper == '\t'))
			query_upper++;

		if (strncasecmp(query_upper, "CREATE KEK", 10) == 0 ||
			strncasecmp(query_upper, "CREATE DEK", 10) == 0)
		{
			tecc_processor_debug(TECC_LOG_DEBUG1,
								"tecc_preprocess_query: Detected key management statement, processing without full initialization");

			/* Force initialization for key management processing */
			if (!state->initialized)
			{
				state->initialized = true;
				tecc_processor_debug(TECC_LOG_DEBUG1,
									"tecc_preprocess_query: Forced initialization for key management statement");
			}

			enabled = true;
		}
	}

	/* ReqID:TECC4.0 - CASCADE KEK file cleanup: pre-snapshot paths.
	 *
	 * Before the DDL executes, capture all plaintext KEK paths so we
	 * can diff against the post-DDL set and unlink() files whose KEKs
	 * are gone. This runs regardless of the enabled flag because the
	 * temp connection bypasses TECC. Guarded by state != NULL. */
	if (state != NULL && query != NULL)
	{
		state->cascade_kek_path_count = 0;
		if (tecc_is_cascade_kek_drop(query))
		{
			tecc_processor_debug(TECC_LOG_DEBUG1,
				"tecc_preprocess_query: detected CASCADE DDL, "
				"capturing KEK paths");
			tecc_capture_all_kek_paths(state);
		}
	}

	if (!enabled)
	{
		tecc_processor_debug(TECC_LOG_DEBUG1,
							"tecc_preprocess_query: TECC not enabled or state invalid, returning NULL");
		return NULL;
	}

	/* Check if this is a COPY command */
	if (strncasecmp(query, "COPY", 4) == 0)
	{
		/* This is a COPY command, preprocess it */
		TeccCopyState *copy_state = tecc_preprocess_copy(conn, query);
		if (copy_state != NULL)
		{
			/* COPY preprocessing successful, no query rewrite needed */
			return NULL;
		}
		else
		{
			/* COPY preprocessing skipped (server-side file COPY) or failed.
			 * tecc_preprocess_copy does not record an error for parse failures
			 * since only client-side COPY (FROM STDIN / TO STDOUT, used by
			 * psql \copy) needs TECC processing. Let the query proceed. */
			return NULL;
		}
	}


	/* Update last statement name - for regular queries, set to empty string */
	if (state->last_stmt_name[0] != '\0')
	{
		state->last_stmt_name[0] = '\0';
	}

	/* Create statement data and call processor */
	{
		TeccStatementData stmt_data;
		TeccProcessorResult proc_result;
		char *guc_processed_query = NULL;

		/* Initialize statement data structure */
		memset(&stmt_data, 0, sizeof(TeccStatementData));

		/* Fill in basic fields */
		stmt_data.conn = conn;
		stmt_data.query = query;
		stmt_data.parse_tree = NIL;  /* Processor will parse SQL */
		stmt_data.nParams = 0;  /* No parameters for regular query */
		stmt_data.paramTypes = NULL;
		stmt_data.paramValues = NULL;
		stmt_data.paramLengths = NULL;
		stmt_data.paramFormats = NULL;

		/* Apply GUC parameters to query if needed */
		tecc_apply_guc_params_to_query(state, query, &guc_processed_query);

		if (guc_processed_query != NULL)
		{
			/* Update query in statement data with GUC-processed version */
			stmt_data.query = guc_processed_query;
		}

		/* Call processor */
		tecc_processor_debug(TECC_LOG_DEBUG1,
							"DEBUG: tecc_preprocess_query: Calling tecc_processor_run_pre_query");
		proc_result = tecc_processor_run_pre_query(&stmt_data);
		tecc_processor_debug(TECC_LOG_DEBUG1,
							"DEBUG: tecc_preprocess_query: tecc_processor_run_pre_query returned %d", proc_result);
			
		/* Clean up GUC-processed query if it was allocated */
		if (guc_processed_query != NULL && guc_processed_query != query)
		{
			tecc_pfree(guc_processed_query);
			guc_processed_query = NULL;
		}

		if (proc_result == TECC_PROCESSOR_NEEDS_REWRITE && stmt_data.rewritten_query != NULL)


		if (proc_result == TECC_PROCESSOR_NEEDS_REWRITE)
		{
			/* Query needs rewrite, get rewritten query */
			if (stmt_data.rewritten_query != NULL)
			{
				rewritten_query = tecc_strdup(stmt_data.rewritten_query);

				/* Update statistics */
				state->stats.rewrite_count++;
			}


		}
		if (proc_result != TECC_PROCESSOR_SUCCESS &&
				 proc_result != TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS &&
				 proc_result != TECC_PROCESSOR_NEEDS_REWRITE)
		{
			/* Processor error */
			tecc_processor_debug(TECC_LOG_DEBUG1,
								"DEBUG: tecc_preprocess_query: Recording error for processor result %d", proc_result);
			/* Use specific error message from processor if available */
			if (stmt_data.error_message[0] != '\0')
			{
				tecc_record_error(state, TECC_ERR_PARSE,
								stmt_data.error_message,
								__func__, __LINE__, false);
			}
			else
			{
				tecc_record_error(state, TECC_ERR_PARSE,
								"Processor failed to process query",
								__func__, __LINE__, false);
			}
		}

		/* ReqID:TECC4.0 - Final safety check: if processor marked needs_rewrite
		 * or where_needs_rewrite but produced no rewritten query, this means
		 * something went wrong (e.g., encryption failure).
		 * Abort to prevent sending plaintext SQL. */
		TECC_LOG_MSG(TECC_LOG_DEBUG1,
					"SAFETY CHECK: rewritten_query=%p, needs_rewrite=%d, where_needs_rewrite=%d, proc_result=%d, encrypted_columns=%p, query='%s'",
					rewritten_query, stmt_data.needs_rewrite, stmt_data.where_needs_rewrite,
					proc_result, stmt_data.encrypted_columns,
					stmt_data.query ? stmt_data.query : "(null)");
		if (rewritten_query == NULL &&
			(stmt_data.needs_rewrite || stmt_data.where_needs_rewrite))
		{
			TECC_LOG_MSG(TECC_LOG_DEBUG1,
						   "SAFETY CHECK: encrypted columns present but no rewritten query");
				/* Use specific error message from processor if available */
				if (stmt_data.error_message[0] != '\0')
				{
					tecc_record_error(state, TECC_ERR_ENCRYPT,
									stmt_data.error_message,
									__func__, __LINE__, false);
				}
				else if (state->last_error.error_msg[0] != '\0')
				{
					tecc_record_error(state, TECC_ERR_ENCRYPT,
									state->last_error.error_msg,
									__func__, __LINE__, false);
				}
				else
				{
					tecc_record_error(state, TECC_ERR_ENCRYPT,
									"TECC processor detected encrypted columns but failed to produce rewritten query",
									__func__, __LINE__, false);
				}
		}

		/* Note: Processor manages parse tree memory, don't tecc_pfree it here */
		stmt_data.parse_tree = NULL;  /* Prevent double tecc_pfree */
	}


	/* Update statistics */
	state->stats.query_count++;

	tecc_processor_debug(TECC_LOG_DEBUG1,
					   "DEBUG: tecc_preprocess_query: Completed, returning %s",
					   rewritten_query ? "rewritten query" : "NULL");

	return rewritten_query;
}
static TeccProcessorResult tecc_encrypt_params_fallback(TeccStatementData *stmt_data, TeccState *state);

/*
 * tecc_preprocess_params - Preprocess a parameterized query
 *
 * Returns: TeccQueryParams structure on success, NULL if no processing needed
 */
TeccQueryParams *
tecc_preprocess_params(PGconn *conn,
					   const char *command,
					   int nParams,
					   const Oid *paramTypes,
					   const char *const *paramValues,
					   const int *paramLengths,
					   const int *paramFormats)
{
	TeccState *state;
	TeccQueryParams *params;
	int i;

	if (conn == NULL || command == NULL)
	{
		return NULL;
	}

	state = tecc_get_state(conn);
	/* Clear any stale error from previous query processing */
	if (state != NULL)
	{
		memset(&state->last_error, 0, sizeof(TeccErrorInfo));
		state->last_error.timestamp = time(NULL);
	}

	if (!tecc_is_enabled(state))
	{
		return NULL;
	}

	/* Check if there are any parameters */
	if (nParams <= 0)
	{
		return NULL;
	}

	/* Update last statement name - for parameterized queries, set to empty string */
	if (state->last_stmt_name[0] != '\0')
	{
		state->last_stmt_name[0] = '\0';
	}

	/* Allocate query parameters structure */
	params = (TeccQueryParams *) pg_malloc(sizeof(TeccQueryParams));
	if (params == NULL)
	{
		tecc_record_error(state, TECC_ERR_MEMORY,
						 "Failed to allocate query parameters structure",
						 __func__, __LINE__, false);
		return NULL;
	}

	/* Initialize all fields to zero */
	memset(params, 0, sizeof(TeccQueryParams));

	params->nParams = nParams;
	params->params_modified = false;
	params->query_modified = false;
	params->owns_memory = true;

	/* Allocate arrays for adjusted parameters */
	params->adjusted_values = (const char **) pg_malloc(nParams * sizeof(char *));
	params->adjusted_lengths = (int *) pg_malloc(nParams * sizeof(int));
	params->adjusted_formats = (int *) pg_malloc(nParams * sizeof(int));

	/* Allocate arrays for original parameter backup */
	params->original_values = (const char **) pg_malloc(nParams * sizeof(char *));
	params->original_lengths = (int *) pg_malloc(nParams * sizeof(int));
	params->original_formats = (int *) pg_malloc(nParams * sizeof(int));

	if (params->adjusted_values == NULL ||
		params->adjusted_lengths == NULL ||
		params->adjusted_formats == NULL ||
		params->original_values == NULL ||
		params->original_lengths == NULL ||
		params->original_formats == NULL)
	{
		tecc_record_error(state, TECC_ERR_MEMORY,
						 "Failed to allocate parameter arrays",
						 __func__, __LINE__, false);
		tecc_free_query_params(params);
		return NULL;
	}

	/* Initialize arrays */
	for (i = 0; i < nParams; i++)
	{
		params->adjusted_values[i] = NULL;
		params->adjusted_lengths[i] = 0;
		params->adjusted_formats[i] = paramFormats != NULL ? paramFormats[i] : 0;

		params->original_values[i] = paramValues != NULL ? paramValues[i] : NULL;
		params->original_lengths[i] = paramLengths != NULL ? paramLengths[i] : 0;
		params->original_formats[i] = paramFormats != NULL ? paramFormats[i] : 0;
	}

	/* Process parameterized query using processor module */
	{
		TeccStatementData stmt_data;
		TeccProcessorResult proc_result;
		char *guc_processed_query = NULL;

		/* Initialize statement data structure */
		memset(&stmt_data, 0, sizeof(TeccStatementData));

		/* Fill in basic fields */
		stmt_data.conn = conn;
		stmt_data.query = command;
		stmt_data.parse_tree = NIL;  /* Processor will parse SQL */
		stmt_data.nParams = nParams;
		stmt_data.paramTypes = paramTypes;
		/* paramValues is const char *const *, stmt_data.paramValues is now also const char *const * */
		/* Processor should not modify paramValues directly, use other mechanism to return modified values */
		stmt_data.paramValues = (const char *const *)tecc_duplicate_param_values(state, paramValues, nParams);
		stmt_data.paramLengths = paramLengths;
		stmt_data.paramFormats = paramFormats;

		/* Apply GUC parameters to query if needed */
		tecc_apply_guc_params_to_query(state, command, &guc_processed_query);

		if (guc_processed_query != NULL)
		{
			/* Update query in statement data with GUC-processed version */
			stmt_data.query = guc_processed_query;
		}

		/* Step 1: Parse query to determine if encrypted columns are involved */
		proc_result = tecc_processor_run_pre_query(&stmt_data);

		/* Clean up GUC-processed query if it was allocated */
		if (guc_processed_query != NULL && guc_processed_query != command)
		{
			tecc_pfree(guc_processed_query);
			guc_processed_query = NULL;
		}

		if (proc_result == TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS ||
			proc_result == TECC_PROCESSOR_ERROR)
		{
			/* ReqID:TECC4.0 - Inline fallback for extended query ($N params).
			 * Extract table from INSERT, call tecc_refresh_table_cache,
			 * then encrypt matching params. */
			tecc_processor_debug(TECC_LOG_DEBUG1,
				"DEBUG: tecc_preprocess_params: trying inline fallback for $N params");
			proc_result = tecc_encrypt_params_fallback(&stmt_data, state);
			if (proc_result == TECC_PROCESSOR_NEEDS_REWRITE)
			{
				tecc_processor_debug(TECC_LOG_DEBUG1,
					"DEBUG: tecc_preprocess_params: fallback encrypted params OK");
				/* Skip Step 2 */
				stmt_data.fallback_did_encrypt = true;
				proc_result = TECC_PROCESSOR_NEEDS_REWRITE;
			}
			else
			{
				tecc_processor_debug(TECC_LOG_DEBUG1,
					"DEBUG: tecc_preprocess_params: fallback could not process, returning NULL");
				return NULL;
			}
		}

		if (proc_result != TECC_PROCESSOR_SUCCESS &&
			proc_result != TECC_PROCESSOR_NEEDS_REWRITE)
		{
			/* Processor error during query analysis */
			tecc_record_error(state, TECC_ERR_PROCESSOR,
				"Processor failed to analyze parameterized query",
				__func__, __LINE__, false);
			return NULL;
		}

		/* Step 2: Process parameters (encryption) - skip if fallback handled it */
		if (!stmt_data.fallback_did_encrypt)
			proc_result = tecc_processor_run_pre_exec(&stmt_data);

		if (proc_result == TECC_PROCESSOR_NEEDS_REWRITE ||
			proc_result == TECC_PROCESSOR_SUCCESS)
		{
			/* Query needs processing - parameters may have been encrypted */
			params->query_modified = true;
			params->params_modified = true;

			/* Copy processed parameter values from processor module */
			/* Note: Processor module should not modify paramValues directly (it's const) */
			for (i = 0; i < nParams; i++)
			{
				const char *pv = NULL;
				if (stmt_data.param_map != NULL &&
				    i < stmt_data.param_map_size &&
				    stmt_data.param_map[i].encrypted_value != NULL)
				    pv = stmt_data.param_map[i].encrypted_value;
				else if (stmt_data.paramValues != NULL &&
				         stmt_data.paramValues[i] != NULL)
				    pv = stmt_data.paramValues[i];
				if (pv != NULL)
				{
					params->adjusted_values[i] = tecc_strdup_ex(state, pv);
					if (params->adjusted_values[i] == NULL)
					{
						tecc_record_error(state, TECC_ERR_MEMORY,
										 "Failed to allocate processed parameter value",
										 __func__, __LINE__, false);
						tecc_free_query_params(params);
						return NULL;
					}

					/* Update parameter length if available from processor */
					if (stmt_data.paramLengths != NULL)
					{
						params->adjusted_lengths[i] = stmt_data.paramLengths[i];
					}
					else
					{
						params->adjusted_lengths[i] = -1;
					}
				}
			}
		}
		else if (proc_result != TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS)
		{
			/* Processor error */
			tecc_record_error(state, TECC_ERR_PROCESSOR,
							 "Processor failed to process parameterized query",
							 __func__, __LINE__, false);
		}

		/* Clean up duplicated parameter values array */
		if (stmt_data.paramValues != NULL)
		{
			char **values_to_free = (char **)stmt_data.paramValues;
			for (i = 0; i < nParams; i++)
			{
				if (values_to_free[i] != NULL)
				{
					tecc_pfree(values_to_free[i]);
				}
			}
			tecc_pfree(values_to_free);
			stmt_data.paramValues = NULL;
		}
	}

	/* Only copy original parameter values if no encryption was performed */
	if (!params->query_modified)
	{
		for (i = 0; i < nParams; i++)
		{
			if (paramValues != NULL && paramValues[i] != NULL)
			{
				params->adjusted_values[i] = tecc_strdup(paramValues[i]);
				if (params->adjusted_values[i] == NULL)
				{
					tecc_record_error(state, TECC_ERR_MEMORY,
									 "Failed to duplicate parameter value",
									 __func__, __LINE__, false);
					tecc_free_query_params(params);
					return NULL;
				}
				params->adjusted_lengths[i] = paramLengths != NULL ? paramLengths[i] : -1;
			}
		}
	}

	/* Update statistics */
	state->stats.parse_count++;
	state->stats.query_count++;

	return params;
}

/*
 * tecc_free_query_params - Free query parameters structure
 */
void
tecc_free_query_params(TeccQueryParams *params)
{
	int i;

	if (params == NULL)
	{
		return;
	}

	/* Free rewritten query */
	if (params->rewritten_query != NULL)
	{
		tecc_pfree(params->rewritten_query);
	}

	/* Free adjusted parameter values */
	if (params->adjusted_values != NULL && params->owns_memory)
	{
		for (i = 0; i < params->nParams; i++)
		{
			if (params->adjusted_values[i] != NULL)
			{
				tecc_pfree((void *) params->adjusted_values[i]);
			}
		}
		tecc_pfree(params->adjusted_values);
	}

	/* Free other arrays */
	if (params->adjusted_lengths != NULL)
	{
		tecc_pfree(params->adjusted_lengths);
	}
	if (params->adjusted_formats != NULL)
	{
		tecc_pfree(params->adjusted_formats);
	}
	if (params->original_values != NULL)
	{
		tecc_pfree(params->original_values);
	}
	if (params->original_lengths != NULL)
	{
		tecc_pfree(params->original_lengths);
	}
	if (params->original_formats != NULL)
	{
		tecc_pfree(params->original_formats);
	}

	/* Free the structure itself */
	tecc_pfree(params);
}

/*
 * tecc_process_result - Process query result for decryption
 */
void
tecc_process_result(PGconn *conn, PGresult *result)
{
	TeccState *state;
	TeccProcessorResult processor_result;


	if (conn == NULL || result == NULL)
	{
		return;
	}

	state = tecc_get_state(conn);

	if (!tecc_is_enabled(state))
	{
		return;
	}

	/* Update last result status from PGresult */
	state->last_result_status = PQresultStatus(result);

	processor_result = tecc_processor_run_post_query(conn, result);

	if (processor_result != TECC_PROCESSOR_SUCCESS)
	{
		/* ReqID:TECC4.0 - Use specific error from processor if available */
		if (state->last_error.error_msg[0] == '\0')
		{
			tecc_record_error(state, TECC_ERR_PROCESSOR,
							 "Processor failed to process query result",
							 __func__, __LINE__, false);
		}
		else
		{
			state->last_error.line_number = __LINE__;
		}
	}

	/* ReqID:TECC4.0 - Remove PENDING sentinel on DDL failure */
	if (state->last_result_status == PGRES_FATAL_ERROR)
	{
		if (state->last_stmt_type == TECC_STMT_TYPE_CREATE_TABLE ||
			state->last_stmt_type == TECC_STMT_TYPE_ALTER_TABLE)
		{
			const char *db = PQdb(conn);
			if (db == NULL) db = "postgres";
			tecc_cache_remove_pending(state, db,
										state->last_table_schema,
										state->last_table_name);
			tecc_record_debug(state, "Removed PENDING sentinel due to DDL failure");
		}
		else if (state->last_stmt_type == TECC_STMT_TYPE_CREATE_KEK)
		{
			if (state->last_key_name[0] != '\0')
			{
				tecc_cache_remove_pending_kek(state, state->last_key_name,
												state->last_key_namespace);
				tecc_record_debug(state, "Removed PENDING KEK due to CREATE KEK failure");
			}
		}
		else if (state->last_stmt_type == TECC_STMT_TYPE_CREATE_DEK)
		{
			if (state->last_key_name[0] != '\0')
			{
				tecc_cache_remove_pending_dek(state, state->last_key_name,
												state->last_key_namespace);
				tecc_record_debug(state, "Removed PENDING DEK due to CREATE DEK failure");
			}
		}
	}

	/* Update statistics */
	state->stats.query_count++;

	/* Check if this is a successful DDL command that might affect metadata */
	/* DDL commands typically return PGRES_COMMAND_OK status */
	if (state->last_result_status == PGRES_COMMAND_OK)
	{
		/* Accept pending statements after successful DDL execution */
		tecc_accept_pending_statements(conn);

		/* ReqID:TECC4.0 - Post-DDL cache refresh: load committed metadata
		 * into formal cache using the main connection. */
		/* Skip DDL cache refresh when called recursively from PQexec inside tecc_refresh_table_cache */
		/* ReqID:TECC4.0 - Remove PENDING sentinel; actual refresh
		 * happens in tecc_preprocess_query (where PQexec works). */
		if ((state->last_stmt_type == TECC_STMT_TYPE_CREATE_TABLE ||
		    state->last_stmt_type == TECC_STMT_TYPE_ALTER_TABLE) &&
		    state->last_table_name[0] != '\0')
		{
		    const char *db_name = PQdb(conn);
		    if (db_name == NULL) db_name = "postgres";
		    tecc_cache_remove_pending(state, db_name,
		        state->last_table_schema,
		        state->last_table_name);
		}

		if (state->last_stmt_type == TECC_STMT_TYPE_CREATE_KEK &&
			state->last_key_name[0] != '\0')
		{
			tecc_refresh_kek_cache(state, conn,
				state->last_key_name, state->last_key_namespace);
		}
		else if (state->last_stmt_type == TECC_STMT_TYPE_CREATE_DEK &&
				 state->last_key_name[0] != '\0')
		{
			tecc_refresh_dek_cache(state, conn,
				state->last_key_name, state->last_key_namespace);
		}

		/* ReqID:TECC4.0 - Client-side KEK file deletion on DROP KEK success.
		 * key_path is on the client filesystem; server cannot delete it.
		 * Save path in pre-processing, unlink after server confirms. */
		if (state->last_stmt_type == TECC_STMT_TYPE_DROP &&
			state->last_drop_remove_type == OBJECT_KEK &&
			state->last_kek_path[0] != '\0')
		{
			if (unlink(state->last_kek_path) != 0 && errno != ENOENT)
			{
				tecc_record_debug(state,
					"Failed to delete KEK file '%s': %s",
					state->last_kek_path, strerror(errno));
			}
			else
			{
				tecc_record_debug(state,
					"Deleted KEK file '%s'", state->last_kek_path);
			}
			state->last_kek_path[0] = '\0';
		}

		/* ReqID:TECC4.0 - CASCADE KEK file cleanup: diff & unlink.
		 * If we pre-snapshot KEK paths before this DDL, diff against the
		 * current (post-DDL) set and unlink() files whose KEKs are gone. */
		if (state->cascade_kek_path_count > 0)
		{
			tecc_cleanup_cascade_kek_files(state);
		}

		/* Begin - ReqID:TECC4.0 - 检测SET语句并更新GUC参数 */

		/* End - ReqID:TECC4.0 - 检测SET语句并更新GUC参数 */
	}



}



/*
 * tecc_handle_error - Handle TECC-related error
 */
void
tecc_handle_error(PGconn *conn,
				  int error_code,
				  const char *error_msg)
{
	TeccState *state;

	if (conn == NULL || error_msg == NULL)
	{
		return;
	}

	state = tecc_get_state(conn);
	if (state == NULL)
	{
		return;
	}

	tecc_record_error(state, error_code, error_msg, __func__, __LINE__, true);

	/* Update statistics */
	state->stats.error_count++;
}

/*
 * tecc_refresh_metadata_cache - Refresh metadata cache
 *
 * Returns: true on success, false on failure
 */
bool
tecc_refresh_metadata_cache(PGconn *conn)
{
	TeccState *state;
	int result;

	if (conn == NULL)
	{
		return false;
	}

	state = tecc_get_state(conn);
	if (!tecc_is_enabled(state))
	{
		return false;
	}

	/* Clean up existing cache */
	tecc_cleanup_metadata_cache(state);

	/* Load new cache */
	result = tecc_load_metadata_cache(state);
	if (result < 0)
	{
		tecc_record_error(state, TECC_ERR_CACHE_REFRESH,
						 "Failed to refresh metadata cache",
						 __func__, __LINE__, true);
		return false;
	}

	/* ReqID:TECC4.0 - Pending tables are no longer needed after cache refresh */
	/* pending_tables_clear removed - PENDING sentinel in name hash, cleaned up with cache */

	/* After cache refresh, accept any pending statements */
	/* This ensures consistency between cache state and prepared statements */
	tecc_accept_pending_statements(conn);

	return true;
}

/*
 * PQteccRefreshCache - User-callable function to manually refresh TECC metadata cache
 *
 * This is the public API counterpart to tecc_refresh_metadata_cache(),
 * exposed via libpq-fe.h so users can call it from client applications.
 *
 * Returns: 0 on success, -1 on failure
 */
int
PQteccRefreshCache(PGconn *conn)
{
	if (!tecc_refresh_metadata_cache(conn))
		return -1;

	return 0;
}

/*
 * tecc_get_last_error - Get last error message for a connection
 *
 * Returns: Error message string, NULL if no error
 */
const char *
tecc_get_last_error(const PGconn *conn)
{
	TeccState *state;

	if (conn == NULL)
	{
		return NULL;
	}

	state = tecc_get_state(conn);
	if (state == NULL)
	{
		return NULL;
	}

	if (state->last_error.error_code == 0)
	{
		return NULL;
	}

	/* DEBUG: Check if TECC is disabled via disable_once */
	if (state->disable_once)
	{
		tecc_processor_debug(TECC_LOG_DEBUG1,
							"DEBUG: tecc_get_last_error: TECC disabled via disable_once, returning NULL even though error exists");
		return NULL;
	}

	return state->last_error.error_msg;
}

/*
 * tecc_get_state - Get TECC state from connection
 *
 * Returns: TeccState pointer, NULL if not available
 */
TeccState *
tecc_get_state(const PGconn *conn)
{
	TeccState *state;

	if (conn == NULL)
	{
		return NULL;
	}

	state = (TeccState *) conn->tecc_state;

	/* Debug: log the pointer values */
	tecc_processor_debug(TECC_LOG_DEBUG2,
						"tecc_get_state: conn=%p, conn->tecc_state=%p, returning state=%p",
						conn, conn->tecc_state, state);

	return state;
}

/*
 * tecc_is_initialized - Check if TECC is initialized for a connection
 *
 * Returns: true if initialized, false otherwise
 */
bool
tecc_is_initialized(const PGconn *conn)
{
	TeccState *state;

	state = tecc_get_state(conn);
	if (state == NULL)
	{
		return false;
	}

	return state->initialized;
}

/*
 * tecc_get_stats - Get TECC statistics for a connection
 *
 * Returns: 0 on success, -1 on error
 */
int
tecc_get_stats(const PGconn *conn, TeccStats *stats)
{
	TeccState *state;

	if (conn == NULL || stats == NULL)
	{
		return -1;
	}

	state = tecc_get_state(conn);
	if (state == NULL)
	{
		return -1;
	}

	*stats = state->stats;
	return 0;
}

/* ========================================================================
 * Internal helper functions
 * ======================================================================== */

/*
 * tecc_integration_get_default_config - Get default configuration
 *
 * Returns: Default TeccConfig structure
 */
static TeccConfig
tecc_integration_get_default_config(void)
{
	TeccConfig config;

	memset(&config, 0, sizeof(TeccConfig));

	config.enabled = true;
	config.cache_size = 1000;
	config.cache_refresh_interval = 300; /* 5 minutes */
	config.max_parallel_decrypt = 4;
	config.batch_size = 100;
	config.memory_pool_size = 1024 * 1024; /* 1MB */
	config.debug_level = 0;
	config.log_queries = false;
	config.log_errors = true;

	return config;
}

/*
 * tecc_load_metadata_cache - Load metadata cache
 *
 * Returns: 0 on success, -1 on failure
 */
static int
tecc_load_metadata_cache(TeccState *state)
{
	int result;

	if (state == NULL || state->conn == NULL)
	{
		return -1;
	}

	/* Call metadata cache module */
	result = tecc_cache_init((TeccState *)state);
	if (result < 0)
	{
		return -1;
	}

	/* Cache pointer is already stored in state->metadata_cache by tecc_cache_init() */

	return 0;
}

/*
 * tecc_cleanup_metadata_cache - Clean up metadata cache
 */
static void
tecc_cleanup_metadata_cache(TeccState *state)
{
	if (state == NULL || state->conn == NULL)
	{
		return;
	}

	/* Check if cache has already been cleaned up */
	if (state->metadata_cache == NULL)
	{
		tecc_record_debug(state, "Metadata cache already cleaned up, skipping");
		return;
	}

	/* Call cache module cleanup function */
	tecc_cache_cleanup((TeccState *)state);

	/* Clear pointer */
	state->metadata_cache = NULL;
}

/*
 * tecc_cleanup_key_cache - Clean up key cache
 */
static void
tecc_cleanup_key_cache(TeccState *state)
{
	if (state == NULL)
	{
		return;
	}

	/* Key cache is managed by key module, just clear pointer */
	state->key_cache = NULL;
}

/*
 * tecc_record_error - Record error in TECC state
 */
void
tecc_record_error(TeccState *state, int error_code,
				  const char *error_msg, const char *function_name,
				  int line_number, bool recoverable)
{
	if (state == NULL || error_msg == NULL || function_name == NULL)
	{
		return;
	}

	state->last_error.error_code = error_code;
	strncpy(state->last_error.error_msg, error_msg,
			sizeof(state->last_error.error_msg) - 1);
	state->last_error.error_msg[sizeof(state->last_error.error_msg) - 1] = '\0';

	state->last_error.timestamp = time(NULL);

	strncpy(state->last_error.function_name, function_name,
			sizeof(state->last_error.function_name) - 1);
	state->last_error.function_name[sizeof(state->last_error.function_name) - 1] = '\0';

	state->last_error.line_number = line_number;
	state->last_error.recoverable = recoverable;
}

/*
 * tecc_strdup - Duplicate string with error checking
 *
 * Returns: Duplicated string, NULL on failure
 */
/*
 * tecc_strdup_ex - Duplicate string with error checking and state context
 *
 * Returns: Duplicated string, NULL on failure
 */
static char *
tecc_strdup_ex(TeccState *state, const char *str)
{
	char *new_str;
	size_t len;

	if (str == NULL)
	{
		return NULL;
	}

	len = strlen(str) + 1;
	new_str = (char *) pg_malloc(len);
	if (new_str == NULL)
	{
		if (state != NULL)
		{
			tecc_record_error(state, TECC_ERR_MEMORY,
							 "Failed to allocate memory for string duplication",
							 __func__, __LINE__, true);
		}
		return NULL;
	}

	memcpy(new_str, str, len);
	return new_str;
}

/*
 * tecc_strdup - Duplicate string with error checking
 * (Backward compatibility wrapper)
 *
 * Returns: Duplicated string, NULL on failure
 */
static char *
tecc_strdup(const char *str)
{
	/* Call the new version without state context */
	return tecc_strdup_ex(NULL, str);
}

/*
 * tecc_duplicate_param_values - Duplicate parameter values array
 *
 * Returns: Newly allocated array, NULL on failure
 */
static char **
tecc_duplicate_param_values(TeccState *state, const char *const *paramValues, int nParams)
{
	char **new_array;
	int i;

	if (paramValues == NULL || nParams <= 0)
	{
		return NULL;
	}

	new_array = (char **) pg_malloc(nParams * sizeof(char *));
	if (new_array == NULL)
	{
		if (state != NULL)
		{
			tecc_record_error(state, TECC_ERR_MEMORY,
							 "Failed to allocate parameter values array",
							 __func__, __LINE__, true);
		}
		return NULL;
	}

	/* Initialize array to NULL */
	for (i = 0; i < nParams; i++)
	{
		new_array[i] = NULL;
	}

	/* Duplicate each parameter value */
	for (i = 0; i < nParams; i++)
	{
		if (paramValues[i] != NULL)
		{
			new_array[i] = tecc_strdup_ex(state, paramValues[i]);
			if (new_array[i] == NULL)
			{
				/* Clean up on failure */
				int j;
				for (j = 0; j < i; j++)
				{
					if (new_array[j] != NULL)
					{
						tecc_pfree(new_array[j]);
					}
				}
				tecc_pfree(new_array);
				return NULL;
			}
		}
	}

	return new_array;
}


/* ==================== Prepared Statement Processing Functions ==================== */

/*
 * tecc_create_prepared_statement_context - Create prepared statement context (helper)
 *
 * Returns: Prepared statement context pointer on success, NULL on failure
 */
static TeccPreparedStatementContext *
tecc_create_prepared_statement_context(PGconn *conn)
{
	/* TODO: Call processor module for prepared statement context creation */
	/* Processor module should provide: tecc_prepared_statement_init() */

	/* For now, return NULL for interface compatibility */
	(void)conn; /* Unused parameter */
	return NULL;
}

/*
 * tecc_free_prepared_statement_context - Free prepared statement context (helper)
 */
static void TECC_ATTR_UNUSED
tecc_free_prepared_statement_context(TeccPreparedStatementContext *context)
{
	/* TODO: Call processor module for prepared statement context cleanup */
	/* Processor module should provide: tecc_prepared_statement_cleanup() */

	/* For now, do nothing for interface compatibility */
	(void)context; /* Unused parameter */
}

/*
 * tecc_preprocess_prepared_statement - Preprocess prepared statement
 *
 * Returns: Prepared statement context pointer on success, NULL if no processing needed
 */
TeccPreparedStatementContext *
tecc_preprocess_prepared_statement(PGconn *conn, const char *stmt_name,
                                   const char *query, int nParams TECC_ATTR_UNUSED,
                                   const Oid *paramTypes TECC_ATTR_UNUSED)
{
	TeccState *state;

	TeccPreparedStatementContext *context;
	if (conn == NULL || stmt_name == NULL || query == NULL)
	{
		return NULL;
	}

	state = tecc_get_state(conn);
	if (state == NULL || !state->enabled)
	{
		/* TECC not enabled, return NULL (no processing needed) */
		return NULL;
	}

	/* Update last statement name with the prepared statement name */
	/* Use strncpy to ensure we don't overflow the buffer */
	strncpy(state->last_stmt_name, stmt_name, NAMEDATALEN - 1);
	state->last_stmt_name[NAMEDATALEN - 1] = '\0';

	/* TODO: Call processor module for prepared statement preprocessing */
	/* Processor module should provide: tecc_prepared_statement_create() */

	/* For now, create minimal context for interface compatibility */
	context = tecc_create_prepared_statement_context(conn);

	/* Add to pending statements list */
	if (context != NULL)
	{
		tecc_add_to_pending_statements(state, stmt_name, context);
	}

	return context;
}

/*
 * tecc_preprocess_prepared_params - Preprocess prepared statement parameters
 *
 * Returns: TeccQueryParams pointer on success, NULL if no processing needed
 */
TeccQueryParams *
tecc_preprocess_prepared_params(PGconn *conn, const char *stmt_name,
                                int nParams TECC_ATTR_UNUSED, const char *const *paramValues TECC_ATTR_UNUSED,
                                const int *paramLengths TECC_ATTR_UNUSED, const int *paramFormats TECC_ATTR_UNUSED)
{
	TeccState *state;

	if (conn == NULL || stmt_name == NULL)
	{
		return NULL;
	}

	state = tecc_get_state(conn);
	if (state == NULL || !state->enabled)
	{
		/* TECC not enabled, return NULL (no processing needed) */
		return NULL;
	}

	/* TODO: Call processor module for prepared statement parameter processing */
	/* Processor module should provide: tecc_prepared_statement_process_params() */

	/* For now, return NULL for interface compatibility */
	return NULL;
}

/*
 * tecc_cleanup_prepared_statement - Clean up prepared statement
 */
void
tecc_cleanup_prepared_statement(PGconn *conn, const char *stmt_name)
{
	TeccState *state;

	if (conn == NULL || stmt_name == NULL)
	{
		return;
	}

	state = tecc_get_state(conn);
	if (state == NULL || !state->enabled)
	{
		/* TECC not enabled, nothing to clean up */
		return;
	}

	/* TODO: Call processor module for prepared statement cleanup */
	/* Processor module should provide: tecc_prepared_statement_deallocate() */

	/* Clear last statement name if it matches the statement being cleaned up */
	if (strcmp(state->last_stmt_name, stmt_name) == 0)
	{
		state->last_stmt_name[0] = '\0';
	}

	/* For now, do nothing for interface compatibility */
	(void)state; /* Unused parameter */
}



/* Begin - ReqID:TECC4.0 - Prepared statement query text store */
void
tecc_store_prepared_query(PGconn *conn, const char *stmt_name,
                          const char *query)
{
    TeccState *state;
    TeccStoredQuery *entry;
    if (conn == NULL || stmt_name == NULL || query == NULL) return;
    state = tecc_get_state(conn);
    if (state == NULL) return;
    for (entry = state->stored_queries; entry != NULL; entry = entry->next)
    {
        if (strcmp(entry->stmt_name, stmt_name) == 0)
        {
            pg_free(entry->query_string);
            entry->query_string = pg_strdup(query);
            return;
        }
    }
    entry = (TeccStoredQuery *)pg_malloc(sizeof(TeccStoredQuery));
    entry->stmt_name = pg_strdup(stmt_name);
    entry->query_string = pg_strdup(query);
    entry->next = state->stored_queries;
    state->stored_queries = entry;
}

const char *
tecc_get_prepared_query(PGconn *conn, const char *stmt_name)
{
    TeccState *state;
    TeccStoredQuery *entry;
    if (conn == NULL || stmt_name == NULL) return NULL;
    state = tecc_get_state(conn);
    if (state == NULL) return NULL;
    for (entry = state->stored_queries; entry != NULL; entry = entry->next)
    {
        if (strcmp(entry->stmt_name, stmt_name) == 0)
            return entry->query_string;
    }
    return NULL;
}

void
tecc_remove_prepared_query(PGconn *conn, const char *stmt_name)
{
    TeccState *state;
    TeccStoredQuery *entry, *prev;
    if (conn == NULL || stmt_name == NULL) return;
    state = tecc_get_state(conn);
    if (state == NULL) return;
    prev = NULL;
    for (entry = state->stored_queries; entry != NULL; prev = entry, entry = entry->next)
    {
        if (strcmp(entry->stmt_name, stmt_name) == 0)
        {
            if (prev) prev->next = entry->next;
            else state->stored_queries = entry->next;
            pg_free(entry->stmt_name);
            pg_free(entry->query_string);
            pg_free(entry);
            return;
        }
    }
}
/* ReqID:TECC4.0 - Prepared statement query text store end */

/*
 * ReqID:TECC4.0 - Fallback encrypt params for extended query protocol.
 * When preprocessor can't parse $N params, extract table name from SQL,
 * query data fetcher directly */

/* ==================== Asynchronous API Processing Functions ==================== */

/*
 * tecc_encrypt_params_fallback - Direct encrypt for extended query $N params.
 * Refreshes table cache, gets encrypted columns, encrypts positional params.
 */
static TeccProcessorResult
tecc_encrypt_params_fallback(TeccStatementData *stmt_data, TeccState *state)
{
    const char *query, *p;
    char table_name[256], schema_name[256], db_name[256];
    int i, enc_cnt = 0, param_idx;
    PGconn *conn = stmt_data ? stmt_data->conn : NULL;
    const char *val;
    TeccUnifiedEncryptResult *enc;

    if (!stmt_data || !state || !conn || !stmt_data->query || 
        stmt_data->nParams == 0 || !stmt_data->paramValues)
        return TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS;

    query = stmt_data->query;
    p = query;
    while (*p == ' ' || *p == '\t') p++;
    if (strncasecmp(p, "INSERT ", 7) != 0) return TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS;
    p += 7; while (*p == ' ' || *p == '\t') p++;
    if (strncasecmp(p, "INTO ", 5) != 0) return TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS;
    p += 5; while (*p == ' ' || *p == '\t') p++;

    /* Extract schema.table */
    table_name[0] = schema_name[0] = '\0';
    {
        int idx = 0;
        while (*p && *p != '.' && *p != ' ' && *p != '\t' && *p != '(' && idx < 255)
            schema_name[idx++] = *p++;
        schema_name[idx] = '\0';
        if (*p == '.') {
            p++; idx = 0;
            while (*p && *p != ' ' && *p != '\t' && *p != '(' && idx < 255)
                table_name[idx++] = *p++;
            table_name[idx] = '\0';
        } else {
            strncpy(table_name, schema_name, 255);
            table_name[255] = '\0';
            strcpy(schema_name, "public");
        }
    }
    if (table_name[0] == '\0') return TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS;

    /* Get db name */
    { const char *d = PQdb(conn); strncpy(db_name, d ? d : "postgres", 255); db_name[255] = '\0'; }

    /* Query encrypted columns directly from server (no cache) */
    {
        bool saved_tecc = conn->enable_tecc;
        char q[8192], *esc_t, *esc_s;
        PGresult *qr; int ri, nrows;
        
        conn->enable_tecc = false;
        esc_t = PQescapeLiteral(conn, table_name, strlen(table_name));
        esc_s = PQescapeLiteral(conn, schema_name, strlen(schema_name));
        if (!esc_t || !esc_s) { conn->enable_tecc = saved_tecc; return TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS; }
        snprintf(q, sizeof(q),
            "SELECT column_attnum, column_name FROM hg_encrypted_columns_info "
            "JOIN pg_class cls ON table_oid = cls.oid "
            "JOIN pg_namespace ns ON cls.relnamespace = ns.oid "
            "WHERE cls.relname = %s AND ns.nspname = %s "
            "AND current_database() = '%s' ORDER BY column_attnum",
            esc_t, esc_s, db_name);
        PQfreemem(esc_t); PQfreemem(esc_s);
        qr = PQexec(conn, q);
        conn->enable_tecc = saved_tecc;
        
        if (qr && PQresultStatus(qr) == PGRES_TUPLES_OK && (nrows = PQntuples(qr)) > 0)
        {
            /* Allocate param_map */
            if (!stmt_data->param_map && stmt_data->nParams > 0) {
                stmt_data->param_map = (TeccParamMapEntry *)pg_malloc0(
                    stmt_data->nParams * sizeof(TeccParamMapEntry));
                stmt_data->param_map_size = stmt_data->nParams;
            }
            
            for (ri = 0; ri < nrows; ri++) {
                int ca = atoi(PQgetvalue(qr, ri, 0));
                const char *cn = PQgetvalue(qr, ri, 1);
                int pi = ca - 1;
                if (pi < 0 || pi >= (int)stmt_data->nParams || !stmt_data->paramValues[pi])
                    continue;
                
                enc = tecc_unified_encrypt_string(conn, stmt_data->paramValues[pi],
                    table_name, cn, NULL, NULL, state->crypto_ctx);
                if (enc && enc->error_code == TECC_CRYPTO_SUCCESS) {
                    if (stmt_data->param_map && pi < (int)stmt_data->param_map_size) {
                        if (stmt_data->param_map[pi].encrypted_value)
                            pg_free(stmt_data->param_map[pi].encrypted_value);
                        stmt_data->param_map[pi].encrypted_value = pg_strdup(enc->hex_string);
                        stmt_data->param_map[pi].needs_encryption = true;
                    }
                    enc_cnt++;
                    tecc_unified_free_encrypt_result(enc);
                } else { if (enc) tecc_unified_free_encrypt_result(enc); }
            }
        }
        if (qr) PQclear(qr);
    }

    return enc_cnt > 0 ? TECC_PROCESSOR_NEEDS_REWRITE : TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS;
}

/* ==================== Asynchronous API Processing Functions ==================== */

/*
 * tecc_async_state_create - Create asynchronous state
 *
 * Returns: New TeccAsyncState pointer, or NULL on error
 */
TeccAsyncState *
tecc_async_state_create(PGconn *conn, TeccAsyncOpType op_type, const char *op_id)
{
	/* TODO: Allocate memory for async state */
	/* For now, return NULL for interface compatibility */
	(void)conn; /* Unused parameter */
	(void)op_type; /* Unused parameter */
	(void)op_id; /* Unused parameter */
	(void)op_type; /* Unused parameter */
	(void)op_id; /* Unused parameter */

	return NULL;
}

/*
 * tecc_async_state_save - Save asynchronous state to connection
 *
 * Returns: 0 on success, -1 on error
 */
int
tecc_async_state_save(PGconn *conn, TeccAsyncState *async_state)
{
	if (conn == NULL || async_state == NULL)
	{
		return -1;
	}

	/* TODO: Save async state to connection context */
	/* For now, return success for interface compatibility */
	(void)conn; /* Unused parameter */
	(void)async_state; /* Unused parameter */

	return 0;
}

/*
 * tecc_async_state_get - Get asynchronous state by operation ID
 *
 * Returns: TeccAsyncState pointer, or NULL if not found
 */
TeccAsyncState *
tecc_async_state_get(PGconn *conn, const char *op_id)
{
	if (conn == NULL || op_id == NULL)
	{
		return NULL;
	}

	/* TODO: Retrieve async state from connection context */
	/* For now, return NULL for interface compatibility */
	(void)conn; /* Unused parameter */
	(void)op_id; /* Unused parameter */

	return NULL;
}

/*
 * tecc_async_state_cleanup - Cleanup asynchronous state
 */
void
tecc_async_state_cleanup(PGconn *conn, const char *op_id)
{
	if (conn == NULL || op_id == NULL)
	{
		return;
	}

	/* TODO: Cleanup async state from connection context */
	/* For now, do nothing for interface compatibility */
	(void)conn; /* Unused parameter */
	(void)op_id; /* Unused parameter */
}

/*
 * tecc_async_state_cleanup_all - Cleanup all asynchronous states
 */
void
tecc_async_state_cleanup_all(PGconn *conn)
{
	if (conn == NULL)
	{
		return;
	}

	/* TODO: Cleanup all async states from connection context */
	/* For now, do nothing for interface compatibility */
	(void)conn; /* Unused parameter */
}

/*
 * tecc_async_preprocess_query - Asynchronous query preprocessing
 *
 * Returns: TeccAsyncState pointer, or NULL on error
 */
TeccAsyncState *
tecc_async_preprocess_query(PGconn *conn, const char *query, const char *async_id)
{
	TeccState *state;

	if (conn == NULL || query == NULL || async_id == NULL)
	{
		return NULL;
	}

	state = tecc_get_state(conn);
	if (state == NULL || !state->enabled)
	{
		return NULL;
	}

	/* TODO: Call processor module for asynchronous query preprocessing */
	/* Processor module should provide: tecc_async_preprocess_query() */

	/* For now, create minimal async state for interface compatibility */
	return tecc_async_state_create(conn, TECC_ASYNC_QUERY, async_id);
}

/*
 * tecc_async_preprocess_params - Asynchronous parameterized query preprocessing
 *
 * Returns: TeccAsyncState pointer, or NULL on error
 */
TeccAsyncState *
tecc_async_preprocess_params(PGconn *conn,
							 const char *command,
							 int nParams,
							 const Oid *paramTypes,
							 const char *const *paramValues,
							 const int *paramLengths,
							 const int *paramFormats,
							 const char *async_id)
{
	TeccState *state;

	/* Mark unused parameters to avoid compiler warnings */
	(void)nParams;
	(void)paramTypes;
	(void)paramValues;
	(void)paramLengths;
	(void)paramFormats;

	if (conn == NULL || command == NULL || async_id == NULL)
	{
		return NULL;
	}

	state = tecc_get_state(conn);
	if (state == NULL || !state->enabled)
	{
		return NULL;
	}

	/* TODO: Call processor module for asynchronous parameter preprocessing */
	/* Processor module should provide: tecc_async_preprocess_params() */

	/* For now, create minimal async state for interface compatibility */
	return tecc_async_state_create(conn, TECC_ASYNC_QUERY_PARAMS, async_id);
}

/*
 * tecc_async_preprocess_prepared_statement - Asynchronous prepared statement preprocessing
 *
 * Returns: TeccAsyncState pointer, or NULL on error
 */
TeccAsyncState *
tecc_async_preprocess_prepared_statement(PGconn *conn, const char *stmt_name,
										 const char *query, int nParams,
										 const Oid *paramTypes, const char *async_id)
{
	TeccState *state;

	/* Mark unused parameters to avoid compiler warnings */
	(void)nParams;
	(void)paramTypes;

	if (conn == NULL || stmt_name == NULL || query == NULL || async_id == NULL)
	{
		return NULL;
	}

	state = tecc_get_state(conn);
	if (state == NULL || !state->enabled)
	{
		return NULL;
	}

	/* TODO: Call processor module for asynchronous prepared statement preprocessing */
	/* Processor module should provide: tecc_async_preprocess_prepared_statement() */

	/* For now, create minimal async state for interface compatibility */
	return tecc_async_state_create(conn, TECC_ASYNC_PREPARE, async_id);
}

/*
 * tecc_async_preprocess_prepared_params - Asynchronous prepared statement parameter preprocessing
 *
 * Returns: TeccAsyncState pointer, or NULL on error
 */
TeccAsyncState *
tecc_async_preprocess_prepared_params(PGconn *conn, const char *stmt_name,
									  int nParams, const char *const *paramValues,
									  const int *paramLengths, const int *paramFormats,
									  const char *async_id)
{
	TeccState *state;

	/* Mark unused parameters to avoid compiler warnings */
	(void)nParams;
	(void)paramValues;
	(void)paramLengths;
	(void)paramFormats;

	if (conn == NULL || stmt_name == NULL || async_id == NULL)
	{
		return NULL;
	}

	state = tecc_get_state(conn);
	if (state == NULL || !state->enabled)
	{
		return NULL;
	}

	/* TODO: Call processor module for asynchronous prepared statement parameter preprocessing */
	/* Processor module should provide: tecc_async_preprocess_prepared_params() */

	/* For now, create minimal async state for interface compatibility */
	return tecc_async_state_create(conn, TECC_ASYNC_QUERY_PREPARED, async_id);
}

/*
 * tecc_async_process_result - Asynchronous result processing
 *
 * Returns: 0 on success, -1 on error
 */
int
tecc_async_process_result(PGconn *conn, PGresult *result, const char *async_id)
{
	TeccState *state;

	if (conn == NULL || result == NULL || async_id == NULL)
	{
		return -1;
	}

	state = tecc_get_state(conn);
	if (state == NULL || !state->enabled)
	{
		return -1;
	}

	/* TODO: Call processor module for asynchronous result processing */
	/* Processor module should provide: tecc_async_process_result() */

	/* For now, fall back to synchronous processing for interface compatibility */
	tecc_process_result(conn, result);
	return 0;
}

/*
 * tecc_async_process_copy_data - Asynchronous COPY data processing
 *
 * Returns: 0 on success, -1 on error
 */
int
tecc_async_process_copy_data(PGconn *conn, const char *data, int data_len,
							 char **processed_data, int *processed_len,
							 const char *async_id)
{
	TeccState *state;

	if (conn == NULL || data == NULL || processed_data == NULL ||
		processed_len == NULL || async_id == NULL)
	{
		return -1;
	}

	state = tecc_get_state(conn);
	if (state == NULL || !state->enabled)
	{
		return -1;
	}

	/* TODO: Call processor module for asynchronous COPY data processing */
	/* Processor module should provide: tecc_async_process_copy_data() */

	/* For now, fall back to synchronous processing for interface compatibility */
	return tecc_process_copy_data(conn, data, data_len, processed_data, processed_len);
}

/*
 * tecc_async_process_copy_result - Asynchronous COPY result processing
 *
 * Returns: 0 on success, -1 on error
 */
int
tecc_async_process_copy_result(PGconn *conn, const char *encrypted_data,
							   int encrypted_len, char **decrypted_data,
							   int *decrypted_len, const char *async_id)
{
	TeccState *state;

	if (conn == NULL || encrypted_data == NULL || decrypted_data == NULL ||
		decrypted_len == NULL || async_id == NULL)
	{
		return -1;
	}

	state = tecc_get_state(conn);
	if (state == NULL || !state->enabled)
	{
		return -1;
	}

	/* TODO: Call processor module for asynchronous COPY result processing */
	/* Processor module should provide: tecc_async_process_copy_result() */

	/* For now, fall back to synchronous processing for interface compatibility */
	return tecc_process_copy_result(conn, encrypted_data, encrypted_len,
									decrypted_data, decrypted_len);
}

/*
 * tecc_async_is_completed - Check if asynchronous operation is completed
 *
 * Returns: true if completed, false otherwise
 */
bool
tecc_async_is_completed(PGconn *conn, const char *async_id)
{
	if (conn == NULL || async_id == NULL)
	{
		return false;
	}

	/* TODO: Check async operation completion status */
	/* For now, return true for interface compatibility */
	(void)conn; /* Unused parameter */
	(void)async_id; /* Unused parameter */

	return true;
}

/*
 * tecc_async_get_error - Get asynchronous operation error
 *
 * Returns: Error message string, or NULL if no error
 */
const char *
tecc_async_get_error(PGconn *conn, const char *async_id)
{
	if (conn == NULL || async_id == NULL)
	{
		return NULL;
	}

	/* TODO: Get async operation error message */
	/* For now, return NULL for interface compatibility */
	(void)conn; /* Unused parameter */
	(void)async_id; /* Unused parameter */

	return NULL;
}

/*
 * tecc_async_cancel - Cancel asynchronous operation
 *
 * Returns: 0 on success, -1 on error
 */
int
tecc_async_cancel(PGconn *conn, const char *async_id)
{
	if (conn == NULL || async_id == NULL)
	{
		return -1;
	}

	/* TODO: Cancel async operation */
	/* For now, return success for interface compatibility */
	(void)conn; /* Unused parameter */
	(void)async_id; /* Unused parameter */

	return 0;
}

/*
 * tecc_async_get_stats - Get asynchronous operation statistics
 *
 * Returns: 0 on success, -1 on error
 */
int
tecc_async_get_stats(PGconn *conn, TeccAsyncStats *stats)
{
	if (conn == NULL || stats == NULL)
	{
		return -1;
	}

	/* TODO: Get async operation statistics */
	/* For now, return success for interface compatibility */
	(void)conn; /* Unused parameter */
	(void)stats; /* Unused parameter */

	return 0;
}


/* ==================== New State Management Interfaces (借鉴openGauss PGClientLogic) ==================== */

/*
 * tecc_state_get_pending_statements - Get pending statements list
 *
 * Returns: Pending statements list pointer, or NULL if not available
 */
TeccPendingStatementNode *
tecc_state_get_pending_statements(TeccState *state)
{
	if (state == NULL)
	{
		return NULL;
	}

	return state->pending_statements;
}

/*
 * tecc_disable_once - Disable TECC once (temporary disable)
 *
 * This function sets the disable_once flag to temporarily disable TECC
 * for the next operation, similar to openGauss PGClientLogic::disable_once
 *
 * TODO: This interface is designed to be used by data fetcher module
 *       (similar to openGauss lib_pq_cursor.cpp). Currently the data
 *       fetcher module is not implemented in TECC.
 */
void
tecc_disable_once(PGconn *conn)
{
	TeccState *state;

	if (conn == NULL)
	{
		tecc_processor_debug(TECC_LOG_DEBUG1,
							"tecc_disable_once: conn is NULL, returning");
		return;
	}

	tecc_processor_debug(TECC_LOG_DEBUG1,
						"tecc_disable_once: conn=%p, conn->tecc_state=%p",
						conn, conn->tecc_state);

	state = tecc_get_state(conn);
	if (state == NULL)
	{
		tecc_processor_debug(TECC_LOG_DEBUG1,
							"tecc_disable_once: state is NULL, returning without setting disable_once");
		return;
	}

	tecc_processor_debug(TECC_LOG_DEBUG1,
						"tecc_disable_once: setting disable_once=true for state=%p",
						state);
	state->disable_once = true;
}

/*
 * tecc_reset_disable_once - Reset the disable_once flag
 *
 * This function resets the disable_once flag, allowing TECC to be
 * enabled again for subsequent operations.
 *
 * TODO: This interface should be called by data fetcher module
 *       after the operation that should skip TECC processing.
 */
void
tecc_reset_disable_once(PGconn *conn)
{
	TeccState *state;

	if (conn == NULL)
	{
		tecc_processor_debug(TECC_LOG_DEBUG1,
							"tecc_reset_disable_once: conn is NULL, returning");
		return;
	}

	tecc_processor_debug(TECC_LOG_DEBUG1,
						"tecc_reset_disable_once: conn=%p, conn->tecc_state=%p",
						conn, conn->tecc_state);

	state = tecc_get_state(conn);
	if (state == NULL)
	{
		tecc_processor_debug(TECC_LOG_DEBUG1,
							"tecc_reset_disable_once: state is NULL, returning without resetting disable_once");
		return;
	}

	tecc_processor_debug(TECC_LOG_DEBUG1,
						"tecc_reset_disable_once: resetting disable_once=false for state=%p",
						state);
	state->disable_once = false;

	/* After resetting disable_once, accept any pending statements */
	/* This ensures that statements accumulated during disabled period are properly handled */
	/* But we need to be careful to avoid recursion - only process if we're not already in a disable_once cycle */
	/* Check if there are actually pending statements before processing */
	if (state->pending_statements != NULL)
	{
		tecc_processor_debug(TECC_LOG_DEBUG1,
							"tecc_reset_disable_once: Found pending statements, processing them");
		tecc_accept_pending_statements(conn);
	}
	else
	{
		tecc_processor_debug(TECC_LOG_DEBUG1,
							"tecc_reset_disable_once: No pending statements to process");
	}
}

/*
 * tecc_get_last_stmt_name - Get last statement name
 *
 * Returns: Last statement name string, or NULL if not available
 */
const char *
tecc_get_last_stmt_name(PGconn *conn)
{
	TeccState *state;

	if (conn == NULL)
	{
		return NULL;
	}

	state = tecc_get_state(conn);
	if (state == NULL)
	{
		return NULL;
	}

	/* Return empty string if last_stmt_name is empty */
	if (state->last_stmt_name[0] == '\0')
	{
		return "";
	}

	return state->last_stmt_name;
}

/*
 * tecc_accept_pending_statements - Accept pending statements
 *
 * This function accepts pending prepared statements, moving them from
 * pending_statements to prepared_statements list.
 *
 * Returns: 0 on success, -1 on error
 */
int
tecc_accept_pending_statements(PGconn *conn)
{
	TeccState *state;
	const int MAX_RECURSION_DEPTH = 10;

	tecc_processor_debug(TECC_LOG_DEBUG1,
						"DEBUG LOOP: tecc_accept_pending_statements: called, conn=%p",
						conn);

	if (conn == NULL)
	{
		tecc_processor_debug(TECC_LOG_DEBUG1,
							"DEBUG LOOP: tecc_accept_pending_statements: conn is NULL, returning -1");
		return -1;
	}

	state = tecc_get_state(conn);
	if (state == NULL)
	{
		tecc_processor_debug(TECC_LOG_DEBUG1,
							"DEBUG LOOP: tecc_accept_pending_statements: state is NULL, returning -1");
		return -1;
	}

	/* Check recursion depth to prevent infinite loops */
	tecc_processor_debug(TECC_LOG_DEBUG1,
						"DEBUG LOOP: tecc_accept_pending_statements: state=%p, pending_statements=%p, recursion_depth=%d",
						state, state->pending_statements, state->accept_pending_recursion_depth);

	if (state->accept_pending_recursion_depth >= MAX_RECURSION_DEPTH)
	{
		tecc_processor_debug(TECC_LOG_ERROR,
							"ERROR: tecc_accept_pending_statements: Maximum recursion depth (%d) exceeded, possible infinite loop",
							MAX_RECURSION_DEPTH);
		/* Reset recursion depth to break the loop */
		state->accept_pending_recursion_depth = 0;
		return -1;
	}

	state->accept_pending_recursion_depth++;

	/* Move pending statements to prepared statements list */
	tecc_move_pending_to_prepared(state);

	tecc_processor_debug(TECC_LOG_DEBUG1,
						"DEBUG LOOP: tecc_accept_pending_statements: completed successfully");

	state->accept_pending_recursion_depth--;
	return 0;
}

/*
 * tecc_cleanup_pending_statements - Cleanup pending statements
 *
 * This function cleans up the pending statements list, typically called
 * on connection close or error recovery.
 */
void
tecc_cleanup_pending_statements(PGconn *conn)
{
	TeccState *state;

	if (conn == NULL)
	{
		return;
	}

	state = tecc_get_state(conn);
	if (state == NULL)
	{
		return;
	}

	/* Clean up pending statements list */
	tecc_cleanup_pending_list(state);
}

/* ==================== Pending Statements Management Functions ==================== */

/*
 * tecc_add_to_pending_statements - Add a prepared statement to pending list
 *
 * This function adds a prepared statement context to the pending statements list.
 * The statement will remain in pending state until tecc_accept_pending_statements()
 * is called to accept it.
 */
static void
tecc_add_to_pending_statements(TeccState *state, const char *stmt_name, TeccPreparedStatementContext *context)
{
	TeccPendingStatementNode *new_node;
	TeccPendingStatementNode *current;

	/* Allocate new node */
	new_node = (TeccPendingStatementNode *)pg_malloc(sizeof(TeccPendingStatementNode));
	if (new_node == NULL)
	{
		/* Memory allocation failed */
		return;
	}

	/* Initialize new node */
	new_node->stmt_name = pg_malloc(strlen(stmt_name) + 1);
	if (new_node->stmt_name == NULL)
	{
		tecc_pfree(new_node);
		return;
	}
	strcpy(new_node->stmt_name, stmt_name);

	new_node->stmt_context = context;
	new_node->added_time = time(NULL);
	new_node->next = NULL;

	/* Add to linked list */
	if (state->pending_statements == NULL)
	{
		/* First node in list */
		state->pending_statements = new_node;
	}
	else
	{
		/* Find last node */
		current = state->pending_statements;
		while (current->next != NULL)
		{
			current = current->next;
		}
		/* Add new node at end */
		current->next = new_node;
	}
}

/*
 * tecc_move_pending_to_prepared - Move pending statements to prepared statements list
 *
 * This function moves all pending statements to the prepared statements list,
 * making them available for execution.
 */
static void
tecc_move_pending_to_prepared(TeccState *state)
{
	int node_count;
	TeccPendingStatementNode *current;
	TeccPendingStatementNode *next;

	/* Move all pending statements to prepared statements list */
	current = state->pending_statements;
	node_count = 0;

	tecc_processor_debug(TECC_LOG_DEBUG1,
						"DEBUG LOOP: tecc_move_pending_to_prepared: state=%p, pending_statements=%p, node_count=%d",
						state, current, node_count);

	while (current != NULL)
	{
		node_count++;

		tecc_processor_debug(TECC_LOG_DEBUG1,
							"DEBUG LOOP: tecc_move_pending_to_prepared: Processing node %d, current=%p, current->next=%p, stmt_name=%s",
							node_count, current, current->next, current->stmt_name ? current->stmt_name : "(null)");

		/* Safety check: prevent infinite loop */
		if (node_count > 1000) {
			tecc_processor_debug(TECC_LOG_ERROR,
								"ERROR: tecc_move_pending_to_prepared: Too many nodes (%d), possible infinite loop", node_count);
			break;
		}

		next = current->next;

		/* TODO: Actually move statement context to prepared statements list */
		/* For now, we just free the node and keep the context in prepared_statements */
		/* Note: prepared_statements is still void*, may need to be updated to linked list */

		/* Free node memory (but keep statement context) */
		if (current->stmt_name != NULL)
		{
			tecc_pfree(current->stmt_name);
		}
		tecc_pfree(current);

		current = next;
	}

	/* Clear pending statements list */
	state->pending_statements = NULL;

	tecc_processor_debug(TECC_LOG_DEBUG1,
						"DEBUG LOOP: tecc_move_pending_to_prepared: Completed, processed %d nodes", node_count);
}

/*
 * tecc_cleanup_pending_list - Clean up pending statements list
 *
 * This function properly cleans up the pending statements list,
 * freeing all allocated memory.
 */
static void
tecc_cleanup_pending_list(TeccState *state)
{
	TeccPendingStatementNode *current;
	TeccPendingStatementNode *next;

	/* 安全检查：如果state为NULL，直接返回 */
	if (state == NULL)
	{
		return;
	}

	current = state->pending_statements;
	while (current != NULL)
	{
		next = current->next;

		/* Free statement name */
		if (current->stmt_name != NULL)
		{
			tecc_pfree(current->stmt_name);
		}

		/* TODO: Free statement context if needed */
		/* For now, we don't free the context as it may be used elsewhere */

		/* Free node */
		tecc_pfree(current);

		current = next;
	}

	/* Clear list pointer */
	state->pending_statements = NULL;
}

/* ==================== GUC Parameters Processing Functions ==================== */

/* Begin - ReqID:TECC4.0 - GUC参数转换辅助函数 */

/*
 * tecc_guc_process_string_literal - 处理字符串字面量中的转义序列
 *
 * 根据standard_conforming_strings设置处理字符串字面量中的转义序列。
 * 当standard_conforming_strings = false时，需要处理E'...'字符串中的转义序列。
 *
 * 参数:
 *   src - 源字符串
 *   dst - 目标缓冲区
 *   dst_len - 目标缓冲区大小
 *   standard_conforming - standard_conforming_strings设置
 *
 * 返回: 处理后的字符串长度，或-1表示错误
 */
static int TECC_ATTR_UNUSED
tecc_guc_process_string_literal(const char *src, char *dst, size_t dst_len,
								bool standard_conforming)
{
	const char *s = src;
	char *d = dst;
	size_t remaining = dst_len - 1; /* 保留一个字节给null终止符 */

	if (dst_len < 1)
		return -1;

	/* 如果standard_conforming_strings = true，不需要特殊处理 */
	if (standard_conforming)
	{
		size_t src_len = strlen(src);
		if (src_len >= dst_len)
			return -1;
		strcpy(dst, src);
		return (int)src_len;
	}

	/* 处理转义序列 */
	while (*s != '\0' && remaining > 0)
	{
		if (*s == '\\')
		{
			/* 处理转义序列 */
			s++; /* 跳过反斜杠 */
			if (*s == '\0')
			{
				/* 反斜杠在字符串末尾，保持原样 */
				*d++ = '\\';
				remaining--;
				break;
			}

			/* 处理常见的转义序列 */
			switch (*s)
			{
				case 'b': *d++ = '\b'; break;
				case 'f': *d++ = '\f'; break;
				case 'n': *d++ = '\n'; break;
				case 'r': *d++ = '\r'; break;
				case 't': *d++ = '\t'; break;
				case 'v': *d++ = '\v'; break;
				case '\\': *d++ = '\\'; break;
				case '\'': *d++ = '\''; break;
				case '"': *d++ = '"'; break;
				case '0': case '1': case '2': case '3':
				case '4': case '5': case '6': case '7':
					/* 八进制转义序列 - 简化处理，只取第一个数字 */
					*d++ = *s - '0';
					break;
				case 'x':
					/* 十六进制转义序列 - 简化处理 */
					s++;
					if (*s >= '0' && *s <= '9')
						*d++ = *s - '0' + (*s >= 'A' && *s <= 'F' ? 10 : 0);
					else if (*s >= 'a' && *s <= 'f')
						*d++ = *s - 'a' + 10;
					else if (*s >= 'A' && *s <= 'F')
						*d++ = *s - 'A' + 10;
					else
					{
						/* 无效的十六进制转义，保持原样 */
						*d++ = '\\';
						*d++ = 'x';
						if (*s != '\0')
							*d++ = *s;
						remaining -= 2;
					}
					break;
				default:
					/* 未知转义序列，保持原样 */
					*d++ = '\\';
					*d++ = *s;
					remaining--;
					break;
			}
			s++;
			remaining--;
		}
		else
		{
			/* 普通字符 */
			*d++ = *s++;
			remaining--;
		}
	}

	/* 添加null终止符 */
	*d = '\0';

	return (int)(d - dst);
}

/*
 * tecc_guc_apply_backslash_quote - 根据backslash_quote模式处理反斜杠引用
 *
 * 处理查询中的反斜杠引用，根据backslash_quote模式：
 * - TECC_BACKSLASH_QUOTE_OFF: 禁用反斜杠引用
 * - TECC_BACKSLASH_QUOTE_ON: 启用反斜杠引用
 * - TECC_BACKSLASH_QUOTE_SAFE_ENCODING: 安全编码模式
 *
 * 参数:
 *   query - 原始查询
 *   processed_query - 处理后的查询缓冲区
 *   processed_len - 处理后的查询缓冲区大小
 *   backslash_quote_mode - backslash_quote模式
 *
 * 返回: 处理后的字符串长度，或-1表示错误
 */
static int
tecc_guc_apply_backslash_quote(const char *query, char *processed_query,
							   size_t processed_len, int backslash_quote_mode)
{
	const char *src = query;
	char *dst = processed_query;
	size_t remaining = processed_len - 1;
	bool in_string = false;
	bool in_escape = false;
	char string_quote = '\0';

	if (processed_len < 1)
		return -1;

	/* 如果backslash_quote = SAFE_ENCODING，不需要特殊处理 */
	if (backslash_quote_mode == TECC_BACKSLASH_QUOTE_SAFE_ENCODING)
	{
		size_t query_len = strlen(query);
	query_len = strlen(query);
			return -1;
		strcpy(processed_query, query);
		return (int)query_len;
	}

	while (*src != '\0' && remaining > 0)
	{
		if (!in_string)
		{
			/* 不在字符串中 */
			if (*src == '\'' || *src == '"')
			{
				/* 进入字符串 */
				in_string = true;
				string_quote = *src;
				*dst++ = *src++;
				remaining--;
			}
			else
			{
				/* 普通字符 */
				*dst++ = *src++;
				remaining--;
			}
		}
		else
		{
			/* 在字符串中 */
			if (in_escape)
			{
				/* 处理转义字符 */
				in_escape = false;
				*dst++ = *src++;
				remaining--;
			}
			else if (*src == '\\')
			{
				/* 反斜杠 */
				if (backslash_quote_mode == TECC_BACKSLASH_QUOTE_OFF)
				{
					/* 禁用反斜杠引用，反斜杠视为普通字符 */
					*dst++ = *src++;
					remaining--;
				}
				else if (backslash_quote_mode == TECC_BACKSLASH_QUOTE_ON)
				{
					/* 启用反斜杠引用，检查下一个字符 */
					src++; /* 跳过反斜杠 */
					if (*src == string_quote)
					{
						/* 反斜杠引用引号字符 */
						*dst++ = *src++;
						remaining--;
					}
					else
					{
						/* 不是引用引号，保持反斜杠 */
						*dst++ = '\\';
						if (*src != '\0')
						{
							*dst++ = *src++;
							remaining--;
						}
						remaining--;
					}
				}
			}
			else if (*src == string_quote)
			{
				/* 字符串结束 */
				in_string = false;
				*dst++ = *src++;
				remaining--;
			}
			else
			{
				/* 字符串中的普通字符 */
				*dst++ = *src++;
				remaining--;
			}
		}
	}

	/* 添加null终止符 */
	*dst = '\0';

	return (int)(dst - processed_query);
}

/*
 * tecc_guc_apply_search_path - 应用搜索路径到未限定的对象名
 *
 * 根据search_path设置，将未限定的对象名（表名、函数名等）转换为完全限定名。
 * 这是一个简化实现，实际实现需要完整的SQL解析。
 *
 * 参数:
 *   query - 原始查询
 *   processed_query - 处理后的查询缓冲区
 *   processed_len - 处理后的查询缓冲区大小
 *   search_path - 搜索路径字符串
 *
 * 返回: 处理后的字符串长度，或-1表示错误
 */
static int
tecc_guc_apply_search_path(const char *query, char *processed_query,
						   size_t processed_len, const char *search_path)
{
	/* 简化实现：目前只复制查询，不实际应用搜索路径 */
	/* TODO: 实现完整的搜索路径应用逻辑 */
	size_t query_len;
	(void)search_path; /* 未使用参数 */
	query_len = strlen(query);

	if (query_len >= processed_len)
		return -1;

	strcpy(processed_query, query);
	return (int)query_len;
}

/* End - ReqID:TECC4.0 - GUC参数转换辅助函数 */

/* Begin - ReqID:TECC4.0 - GUC参数同步函数 */

/*
 * tecc_guc_sync_from_server_internal - 从数据库服务端同步GUC参数（内部函数）
 *
 * 通过查询数据库系统视图获取当前GUC参数值，并更新客户端状态。
 *
 * 参数:
 *   state - TECC状态
 *   conn - 数据库连接
 *
 * 返回: 成功返回true，失败返回false
 */

/*
 * tecc_guc_apply_update_internal - 应用GUC参数更新（内部函数）
 *
 * 应用TeccUpdateGucValues中的更新到当前GUC参数。
 *
 * 参数:
 *   state - TECC状态
 *   updates - 要应用的更新
 *
 * 返回: 成功返回true，失败返回false
 */
static bool
tecc_guc_apply_update_internal(TeccState *state, const TeccUpdateGucValues *updates)
{
	if (state == NULL || updates == NULL)
		return false;

	/* 应用更新 */
	if (updates->update_role)
	{
		strncpy(state->guc_params.role, updates->new_role, sizeof(state->guc_params.role) - 1);
		state->guc_params.role[sizeof(state->guc_params.role) - 1] = '\0';
	}

	if (updates->update_standard_conforming_strings)
	{
		state->guc_params.standard_conforming_strings = updates->new_standard_conforming_strings;
	}

	if (updates->update_backslash_quote)
	{
		state->guc_params.backslash_quote = updates->new_backslash_quote;
	}

	if (updates->update_escape_string_warning)
	{
		state->guc_params.escape_string_warning = updates->new_escape_string_warning;
	}

	if (updates->update_searchpath)
	{
		strncpy(state->guc_params.searchpath, updates->new_searchpath,
				sizeof(state->guc_params.searchpath) - 1);
		state->guc_params.searchpath[sizeof(state->guc_params.searchpath) - 1] = '\0';
	}

	/* 可以继续添加其他GUC参数的更新处理 */

	return true;
}

/* End - ReqID:TECC4.0 - GUC参数同步函数 */

/*
 * tecc_apply_guc_params_to_query - Apply GUC parameters to query processing
 *
 * This function applies GUC parameters to adjust query processing behavior.
 * For example, it can modify string literals based on standard_conforming_strings
 * or backslash_quote settings.
 *
 * Note: This is a simplified implementation. Real implementation would need
 * to parse the query and apply GUC-specific transformations.
 */
static void
tecc_apply_guc_params_to_query(TeccState *state, const char *query, char **processed_query)
{
	size_t query_len;
	size_t buffer_size;
	char *new_query;
	char *temp_buffer;
	char *current_query;
	int processed_len;
	/* Initialize output */
	*processed_query = NULL;

	/* Check if we need to process the query based on GUC parameters */
	if (!tecc_should_process_query_with_guc(state, query))
	{
		return; /* No processing needed */
	}

	/* Begin - ReqID:TECC4.0 - 实现完整GUC参数转换逻辑 */

	/* 分配内存用于处理后的查询 */
	query_len = strlen(query);
	/* 为可能的扩展预留额外空间 */
	buffer_size = query_len * 2 + 1;
	new_query = (char *)pg_malloc(buffer_size);
	if (new_query == NULL)
	{
		/* Memory allocation failed */
		return;
	}

	/* 复制原始查询作为起点 */
	strcpy(new_query, query);

	/* 应用GUC参数转换 */
	temp_buffer = NULL;
	current_query = new_query;
	processed_len = 0;

	/* 1. 处理standard_conforming_strings */
	if (!state->guc_params.standard_conforming_strings)
	{
		/* 需要处理字符串字面量中的转义序列 */
		/* 简化实现：在实际实现中，需要解析SQL识别字符串字面量 */
		/* 这里我们只演示处理逻辑 */
		temp_buffer = (char *)pg_malloc(buffer_size);
		if (temp_buffer != NULL)
		{
			/* 在实际实现中，这里需要调用tecc_guc_process_string_literal处理每个字符串字面量 */
			/* 简化：直接复制 */
			strcpy(temp_buffer, current_query);
			current_query = temp_buffer;
		}
	}

	/* 2. 处理backslash_quote */
	if (state->guc_params.backslash_quote != TECC_BACKSLASH_QUOTE_SAFE_ENCODING)
	{
		/* 应用反斜杠引用处理 */
		char *backslash_buffer = (char *)pg_malloc(buffer_size);
		if (backslash_buffer != NULL)
		{
			processed_len = tecc_guc_apply_backslash_quote(
				current_query, backslash_buffer, buffer_size,
				state->guc_params.backslash_quote);

			if (processed_len > 0)
			{
				/* 释放之前的临时缓冲区 */
				if (temp_buffer != NULL && temp_buffer != new_query)
				{
					tecc_pfree(temp_buffer);
				}

				temp_buffer = backslash_buffer;
				current_query = temp_buffer;
			}
			else
			{
				/* 处理失败，释放缓冲区 */
				tecc_pfree(backslash_buffer);
			}
		}
	}

	/* 3. 处理searchpath */
	if (state->guc_params.searchpath[0] != '\0')
	{
		/* 应用搜索路径 */
		char *searchpath_buffer = (char *)pg_malloc(buffer_size);
		if (searchpath_buffer != NULL)
		{
			processed_len = tecc_guc_apply_search_path(
				current_query, searchpath_buffer, buffer_size,
				state->guc_params.searchpath);

			if (processed_len > 0)
			{
				/* 释放之前的临时缓冲区 */
				if (temp_buffer != NULL && temp_buffer != new_query)
				{
					tecc_pfree(temp_buffer);
				}

				temp_buffer = searchpath_buffer;
				current_query = temp_buffer;
			}
			else
			{
				/* 处理失败，释放缓冲区 */
				tecc_pfree(searchpath_buffer);
			}
		}
	}

	/* 如果进行了任何处理，更新返回的查询 */
	if (current_query != new_query)
	{
		/* 复制处理后的查询到new_query */
		strcpy(new_query, current_query);

		/* 释放临时缓冲区 */
		if (temp_buffer != NULL && temp_buffer != new_query)
		{
			tecc_pfree(temp_buffer);
		}
	}
	else
	{
		/* 如果没有进行处理，但分配了temp_buffer，需要释放它 */
		if (temp_buffer != NULL && temp_buffer != new_query)
		{
			tecc_pfree(temp_buffer);
		}
	}

	/* 设置返回的查询 */
	*processed_query = new_query;

	/* End - ReqID:TECC4.0 - 实现完整GUC参数转换逻辑 */

	/* Note: Caller is responsible for freeing the returned string */
}

/*
 * tecc_should_process_query_with_guc - Check if query should be processed with GUC parameters
 *
 * This function checks whether a query should be processed with GUC parameters
 * based on the current GUC settings.
 *
 * Returns: true if query should be processed with GUC parameters, false otherwise
 */
static bool
tecc_should_process_query_with_guc(const TeccState *state, const char *query)
{
	/* Begin - ReqID:TECC4.0 - 完善GUC处理检查逻辑 */

	if (query == NULL || query[0] == '\0')
	{
		return false;
	}

	/* 1. 检查standard_conforming_strings */
	if (!state->guc_params.standard_conforming_strings)
	{
		/* 当standard_conforming_strings禁用时，需要处理E'...'字符串 */
		/* 检查查询是否包含E'前缀的字符串字面量 */
		const char *p = query;
		while ((p = strchr(p, 'E')) != NULL)
		{
			if (p[1] == '\'')
			{
				/* 找到E'字符串字面量，需要处理 */
				return true;
			}
			p++;
		}

		/* 也检查普通字符串字面量中是否包含反斜杠 */
		p = query;
		while ((p = strchr(p, '\'')) != NULL)
		{
			/* 找到字符串开始 */
			const char *str_start TECC_ATTR_UNUSED = p;
			p++; /* 跳过开始的引号 */

			/* 查找字符串结束 */
			while (*p != '\0' && *p != '\'')
			{
				if (*p == '\\')
				{
					/* 字符串中包含反斜杠，需要处理 */
					return true;
				}
				p++;
			}

			if (*p == '\0')
			{
				/* 字符串未结束，查询不完整 */
				break;
			}

			p++; /* 跳过结束的引号 */
		}
	}

	/* 2. 检查backslash_quote */
	if (state->guc_params.backslash_quote != TECC_BACKSLASH_QUOTE_SAFE_ENCODING)
	{
		/* 检查查询是否包含反斜杠 */
		if (strchr(query, '\\') != NULL)
		{
			return true;
		}
	}

	/* 3. 检查escape_string_warning */
	if (state->guc_params.escape_string_warning)
	{
		/* 检查可能触发警告的转义序列 */
		/* 简化实现：检查常见的非标准转义序列 */
		const char *warning_patterns[] = {
			"\\x",  /* 十六进制转义 */
			"\\0", "\\1", "\\2", "\\3", "\\4", "\\5", "\\6", "\\7", /* 八进制转义 */
			"\\b", "\\f", "\\n", "\\r", "\\t", "\\v" /* 控制字符转义 */
		};

		size_t i;
		for (i = 0; i < sizeof(warning_patterns) / sizeof(warning_patterns[0]); i++)
		{
			if (strstr(query, warning_patterns[i]) != NULL)
			{
				return true;
			}
		}
	}

	/* 4. 检查searchpath */
	if (state->guc_params.searchpath[0] != '\0')
	{
		/* 当searchpath非空时，可能需要应用搜索路径 */
		/* 简化实现：检查查询是否包含可能未限定的对象名 */
		/* 在实际实现中，需要更复杂的解析 */

		/* 检查常见的SQL关键字后是否跟着标识符（可能未限定） */
		const char *keywords[] = {
			"FROM ", "JOIN ", "TABLE ", "VIEW ", "FUNCTION ", "PROCEDURE "
		};

		size_t i;
		for (i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++)
		{
			const char *pos = query;
			while ((pos = strstr(pos, keywords[i])) != NULL)
			{
				const char *ident_end;
				/* 检查关键字后的标识符是否包含点号（已限定） */
				const char *ident_start = pos + strlen(keywords[i]);
				while (*ident_start == '\0' || *ident_start == '\t' || *ident_start == '\n')
					ident_start++;

				/* 查找标识符结束 */
				ident_end = ident_start;
				while (*ident_end != '\0' &&
					   (*ident_end != '\0' && *ident_end != '\t' &&
						*ident_end != '\n' && *ident_end != ',' &&
						*ident_end != ';' && *ident_end != ')'))
					ident_end++;

				/* 检查标识符中是否包含点号 */
				if (memchr(ident_start, '.', ident_end - ident_start) == NULL)
				{
					/* 未限定的标识符，可能需要应用搜索路径 */
					return true;
				}

				pos = ident_end;
			}
		}
	}

	/* End - ReqID:TECC4.0 - 完善GUC处理检查逻辑 */

	return false;
}

/* ==================== GUC Parameters Management Public Interfaces ==================== */

/* Begin - ReqID:TECC4.0 - GUC参数管理公共接口实现 */

/*
 * tecc_guc_sync_from_server - 从数据库服务端同步GUC参数（公共接口）
 *
 * 参数:
 *   conn - 数据库连接
 */

/*
 * tecc_guc_apply_update - 应用GUC参数更新（公共接口）
 *
 * 参数:
 *   conn - 数据库连接
 *   updates - 要应用的更新
 *
 * 返回: 成功返回true，失败返回false
 */
bool
tecc_guc_apply_update(PGconn *conn, const TeccUpdateGucValues *updates)
{
	TeccState *state = tecc_get_state(conn);

	if (state == NULL || updates == NULL)
		return false;

	return tecc_guc_apply_update_internal(state, updates);
}

/*
 * tecc_guc_get_params - 获取当前GUC参数（公共接口）
 *
 * 参数:
 *   conn - 数据库连接
 *
 * 返回: 当前GUC参数指针，或NULL表示错误
 */
const TeccGucParams *
tecc_guc_get_params(PGconn *conn)
{
	TeccState *state = tecc_get_state(conn);

	if (state == NULL)
		return NULL;

	return &state->guc_params;
}

/*
 * tecc_guc_set_param - 设置GUC参数值（公共接口）
 *
 * 参数:
 *   conn - 数据库连接
 *   param_name - 参数名
 *   param_value - 参数值
 *
 * 返回: 成功返回true，失败返回false
 */
bool
tecc_guc_set_param(PGconn *conn, const char *param_name, const char *param_value)
{
	TeccState *state = tecc_get_state(conn);
	TeccUpdateGucValues updates;

	if (state == NULL || param_name == NULL || param_value == NULL)
		return false;

	/* 初始化更新结构 */
	memset(&updates, 0, sizeof(TeccUpdateGucValues));

	/* 根据参数名设置对应的更新标志和值 */
	if (strcmp(param_name, "role") == 0)
	{
		updates.update_role = true;
		strncpy(updates.new_role, param_value, sizeof(updates.new_role) - 1);
		updates.new_role[sizeof(updates.new_role) - 1] = '\0';
	}
	else if (strcmp(param_name, "standard_conforming_strings") == 0)
	{
		updates.update_standard_conforming_strings = true;
		updates.new_standard_conforming_strings = (strcmp(param_value, "on") == 0);
	}
	else if (strcmp(param_name, "backslash_quote") == 0)
	{
		updates.update_backslash_quote = true;
		if (strcmp(param_value, "safe_encoding") == 0)
			updates.new_backslash_quote = TECC_BACKSLASH_QUOTE_SAFE_ENCODING;
		else if (strcmp(param_value, "on") == 0)
			updates.new_backslash_quote = TECC_BACKSLASH_QUOTE_ON;
		else
			updates.new_backslash_quote = TECC_BACKSLASH_QUOTE_OFF;
	}
	else if (strcmp(param_name, "escape_string_warning") == 0)
	{
		updates.update_escape_string_warning = true;
		updates.new_escape_string_warning = (strcmp(param_value, "on") == 0);
	}
	else if (strcmp(param_name, "search_path") == 0)
	{
		updates.update_searchpath = true;
		strncpy(updates.new_searchpath, param_value, sizeof(updates.new_searchpath) - 1);
		updates.new_searchpath[sizeof(updates.new_searchpath) - 1] = '\0';
	}
	else
	{
		/* 不支持的参数名 */
		return false;
	}

	/* 应用更新 */
	return tecc_guc_apply_update_internal(state, &updates);
}

/*
 * tecc_guc_reset_to_defaults - 重置GUC参数为默认值（公共接口）
 *
 * 参数:
 *   conn - 数据库连接
 */
void
tecc_guc_reset_to_defaults(PGconn *conn)
{
	TeccState *state = tecc_get_state(conn);

	if (state == NULL)
		return;

	/* 重置为默认值 */
	strcpy(state->guc_params.role, "");
	state->guc_params.standard_conforming_strings = true;
	state->guc_params.backslash_quote = TECC_BACKSLASH_QUOTE_SAFE_ENCODING;
	state->guc_params.escape_string_warning = true;
	strcpy(state->guc_params.searchpath, "\"$user\", public");
}

/* End - ReqID:TECC4.0 - GUC参数管理公共接口实现 */

/*
 * tecc_record_debug - 记录调试信息（简化实现）
 *
 * 参数:
 *   state - TECC状态（可为NULL）
 *   format - 格式化字符串
 *   ... - 可变参数
 */
void
tecc_record_debug(const TeccState *state, const char *format, ...)
{
	/* 简化实现：目前只忽略调试消息 */
	(void)state;
	(void)format;
	/* TODO: 实现完整的调试日志记录 */
}

