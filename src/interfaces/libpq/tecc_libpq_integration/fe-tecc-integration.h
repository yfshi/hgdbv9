/*-------------------------------------------------------------------------
 *
 * fe-tecc-integration.h
 *	  TECC libpq integration module data structures and interfaces
 *
 * Portions Copyright (c) 1996-2026, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/interfaces/libpq/fe-tecc-integration.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef FE_TECC_INTEGRATION_H
#define FE_TECC_INTEGRATION_H

#include "postgres_fe.h"

#include "libpq-fe.h"
#include "libpq-int.h"
#include "../tecc_cache/tecc_metadata_cache.h"
#include "../tecc_common/tecc_common_types.h"
#include "../tecc_scheduler/tecc_crypto_context.h"  /* For TeccCryptoContext */
#include "../tecc_processor/prepared_statement.h"  /* For TeccPreparedStatement */

/* Forward declarations */
typedef struct TeccState TeccState;
typedef struct TeccQueryParams TeccQueryParams;

/* ReqID:TECC4.0 - Pending table entry for same-transaction DDL+DML support (REMOVED).
 * Replaced by PENDING cache sentinel + post-DDL main connection refresh.
 * See tecc_cache_insert_pending() and tecc_refresh_table_cache(). */
typedef struct TeccResultInfo TeccResultInfo;
typedef struct TeccErrorInfo TeccErrorInfo;
typedef struct TeccConfig TeccConfig;
typedef struct TeccStats TeccStats;
typedef struct TeccPreparedStatementContext TeccPreparedStatementContext;
typedef struct TeccAsyncState TeccAsyncState;
typedef struct TeccAsyncStats TeccAsyncStats;
typedef struct TeccCryptoScheduler TeccCryptoScheduler;
/* TeccStatementData is now defined in tecc_common_types.h */

/* Error codes */
typedef enum TeccErrorCode
{
	TECC_ERR_INIT = 1000,			/* Initialization error */
	TECC_ERR_CACHE_LOAD,			/* Cache loading error */
	TECC_ERR_PARSE,					/* Parsing error */
	TECC_ERR_ENCRYPT,				/* Encryption error */
	TECC_ERR_DECRYPT,				/* Decryption error */
	TECC_ERR_TYPE,					/* Type error */
	TECC_ERR_PARAM,					/* Parameter error */
	TECC_ERR_MEMORY,				/* Memory error */
	TECC_ERR_CACHE_REFRESH,			/* Cache refresh error */
	TECC_ERR_METADATA,				/* Metadata error */
	TECC_ERR_KEY_MANAGER,			/* Key manager error */
	TECC_ERR_DATA_FETCHER,			/* Data fetcher error */
	TECC_ERR_PROCESSOR				/* Processor error */
} TeccErrorCode;

/* Processor result enumeration - defined in tecc_processor_types.h */
typedef enum TeccProcessorResult TeccProcessorResult;

/* Asynchronous operation type enumeration */
typedef enum TeccAsyncOpType
{
	TECC_ASYNC_QUERY = 1,				/* Asynchronous query */
	TECC_ASYNC_QUERY_PARAMS,			/* Asynchronous parameterized query */
	TECC_ASYNC_PREPARE,					/* Asynchronous prepared statement creation */
	TECC_ASYNC_QUERY_PREPARED,			/* Asynchronous prepared statement execution */
	TECC_ASYNC_COPY_DATA,				/* Asynchronous COPY data sending */
	TECC_ASYNC_COPY_RESULT,				/* Asynchronous COPY result receiving */
	TECC_ASYNC_DESCRIBE,				/* Asynchronous describe operation */
	TECC_ASYNC_FLUSH					/* Asynchronous flush operation */
} TeccAsyncOpType;

/* Error information structure */
typedef struct TeccErrorInfo
{
	int			error_code;			/* Error code */
	char		error_msg[256];		/* Error message */
	time_t		timestamp;			/* Error timestamp */
	char		function_name[64];	/* Function name where error occurred */
	int			line_number;		/* Line number where error occurred */
	bool		recoverable;		/* Whether error is recoverable */
} TeccErrorInfo;

/* Statistics structure */
typedef struct TeccStats
{
	unsigned long encrypt_count;	/* Number of encryption operations */
	unsigned long decrypt_count;	/* Number of decryption operations */
	unsigned long cache_hits;		/* Number of cache hits */
	unsigned long cache_misses;		/* Number of cache misses */
	unsigned long query_count;		/* Number of processed queries */
	unsigned long error_count;		/* Number of errors */
	unsigned long parse_count;		/* Number of SQL parses */
	unsigned long rewrite_count;	/* Number of SQL rewrites */
} TeccStats;

/* Backslash quote mode enumeration */
typedef enum {
	TECC_BACKSLASH_QUOTE_OFF = 0,		/* Disable backslash quoting */
	TECC_BACKSLASH_QUOTE_ON,			/* Enable backslash quoting */
	TECC_BACKSLASH_QUOTE_SAFE_ENCODING	/* Safe encoding mode */
} TeccBackslashQuoteMode;

/* GUC parameters structure (简化版本，与openGauss GucParams保持一致) */
/* 定义在tecc_common_types.h中，这里使用前向声明 */
typedef struct TeccGucParams TeccGucParams;

/* Update GUC values structure */
typedef struct TeccUpdateGucValues
{
	/* Begin - ReqID:TECC4.0 - 简化GUC更新值结构体 */

	/* Flags indicating which fields need to be updated */
	bool		update_role;				/* Update role field */
	bool		update_standard_conforming_strings; /* Update standard_conforming_strings */
	bool		update_backslash_quote;		/* Update backslash_quote */
	bool		update_escape_string_warning; /* Update escape_string_warning */
	bool		update_searchpath;			/* Update searchpath */

	/* Values to update (only valid if corresponding flag is true) */
	char		new_role[64];				/* New role value */
	bool		new_standard_conforming_strings; /* New standard_conforming_strings value */
	int			new_backslash_quote;		/* New backslash_quote value */
	bool		new_escape_string_warning;	/* New escape_string_warning value */
	char		new_searchpath[1024];		/* New searchpath value */

	/* End - ReqID:TECC4.0 - 简化GUC更新值结构体 */
} TeccUpdateGucValues;

/* Configuration structure */
typedef struct TeccConfig
{
	bool		enabled;					/* Whether TECC is enabled */
	int			cache_size;					/* Metadata cache size */
	int			cache_refresh_interval;		/* Cache refresh interval in seconds */
	int			max_parallel_decrypt;		/* Maximum parallel decrypt operations */
	int			batch_size;					/* Batch processing size */
	int			memory_pool_size;			/* Memory pool size in bytes */
	int			debug_level;				/* Debug level */
	bool		log_queries;				/* Whether to log queries */
	bool		log_errors;					/* Whether to log errors */
} TeccConfig;

/* Pending statement list node structure (借鉴openGauss PGClientLogic设计) */
typedef struct TeccPendingStatementNode
{
	char	   *stmt_name;					/* Statement name */
	void	   *stmt_context;				/* Statement context pointer */
	time_t		added_time;					/* Time when added to pending list */
	struct TeccPendingStatementNode *next;	/* Next node in linked list */
} TeccPendingStatementNode;

/* ReqID:TECC4.0 - Block recycling pool with independent lifetime.
 * PGconn releases blocks when closed but the pool struct itself
 * persists. PGresult objects hold a pointer to this pool and can
 * safely call into it even after the owning PGconn is freed. */
typedef struct TeccBlockPool
{
	void	   *free_blocks;		/* PGresult_data block chain */
	int			free_count;			/* blocks in free chain */
} TeccBlockPool;

/* TECC connection state structure */
typedef struct TeccState
{
	/* Basic state */
	bool		enabled;					/* Whether TECC is enabled */
	bool		initialized;				/* Whether state is initialized */

	/* Cache management */
	void	   *metadata_cache;				/* Metadata cache pointer */
	void	   *key_cache;					/* Key cache pointer */

	/* Statistics */
	TeccStats	stats;						/* Statistics */

	/* Associated connection */
	PGconn	   *conn;						/* Associated libpq connection */

	/* Error handling */
	TeccErrorInfo last_error;				/* Last error information */

	/* Configuration */
	TeccConfig	config;						/* Configuration parameters */

	/* Processor context */
	void	   *processor_ctx;				/* Processor module context */

	/* Crypto context */
	TeccCryptoContext *crypto_ctx;			/* Per-operation crypto context */
	void	   *key_mgr;					/* Key manager adapter pointer */

	/* Raw values list (similar to openGauss rawValuesForReplace) */
	void	   *raw_values_list;			/* Raw values replacement list */

	/* Prepared statements management (借鉴openGauss PGClientLogic设计) */
	TeccPreparedStatement *prepared_statements_list;	/* Prepared statements linked list */
	int			prepared_statements_count;			/* Number of prepared statements */
	TeccPendingStatementNode *pending_statements;	/* Pending prepared statements linked list (新增) */

	/* Cache refresh state */
	bool		cache_refresh_needed;		/* Flag indicating cache needs refresh */
	time_t		last_cache_refresh;			/* Last cache refresh time */

	/* Error recovery state */
	int			error_recovery_state;		/* Error recovery state: 0-normal, 1-recovering, 2-needs cache refresh */
	int			consecutive_errors;			/* Consecutive error count for recovery triggering */

	/* Temporary disable flag (借鉴openGauss disable_once) */
	bool		disable_once;				/* Temporarily disable TECC once (新增) */

	/* Last statement name tracking (借鉴openGauss lastStmtName) */
	char		last_stmt_name[NAMEDATALEN]; /* Last processed statement name (新增) */

	int			last_stmt_type;				/* Last statement type for post-processing cache refresh */
	char		last_table_schema[NAMEDATALEN];	/* Last DDL table schema for post-DDL cache refresh */
	char		last_table_name[NAMEDATALEN];	/* Last DDL table name for post-DDL cache refresh */
	char		last_key_name[NAMEDATALEN];		/* Last CREATE KEK/DEK name for PENDING cleanup */
	char		last_key_namespace[NAMEDATALEN]; /* Last CREATE KEK/DEK namespace for PENDING cleanup */
	int		last_drop_remove_type;		/* Last DROP removeType for selective cache clear */
	int		last_drop_behavior;			/* Last DROP behavior (CASCADE/RESTRICT) */
	Oid		last_drop_key_oid;			/* ReqID:TECC4.0 - Last DROP KEK/DEK OID for adapter cache invalidation */
	char	last_kek_path[256];			/* ReqID:TECC4.0 - Last DROP KEK key_path for client-side file deletion */

	/* ReqID:TECC4.0 - CASCADE DROP KEK file cleanup.
	 * Pre-snapshot of all plaintext KEK paths (KEY_PATH in hg_kek_args)
	 * captured before a CASCADE DDL (DROP SCHEMA/OWNED CASCADE) executes.
	 * After the DDL succeeds, we diff against the surviving paths and
	 * unlink() files whose KEKs are gone (server can't reach client FS). */
#define TECC_MAX_CASCADE_KEK_PATHS 256
	int		cascade_kek_path_count;		/* Number of pre-snapshot KEK paths */
	char	cascade_kek_paths[TECC_MAX_CASCADE_KEK_PATHS][256];	/* Pre-snapshot KEK paths */
	/* Last result status (借鉴openGauss m_lastResultStatus) */
	ExecStatusType last_result_status;		/* Last query result status (新增) */

	/* GUC parameters (借鉴openGauss GucParams) */
	TeccGucParams guc_params;				/* GUC parameters structure (新增) */
	TeccGucParams tmp_guc_params;			/* Temporary GUC parameters (新增) */
	TeccUpdateGucValues val_to_update;		/* Values to update (新增) */

	/* Data fetcher management (连接级管理，避免全局变量) */
	void	   *data_fetcher_manager;		/* 数据获取器管理器实例 */
	bool		data_fetcher_initialized;	/* 数据获取器是否已初始化 */

	/* Processor state (连接级管理，避免全局变量) */
	bool		processor_initialized;		/* 处理器是否已初始化 */
	void	   *processor_context;			/* 处理器上下文（连接特定） */

	/* Crypto scheduler state (连接级管理，避免全局变量) */
	TeccCryptoScheduler *crypto_scheduler;	/* 加密调度器实例 */
	bool		scheduler_initialized;		/* 调度器是否已初始化 */

	/* Recursion prevention for tecc_accept_pending_statements */
	int			accept_pending_recursion_depth;	/* 递归深度计数器，防止无限循环 */

	/* Connection pool for data fetcher internal queries (独立连接，避免嵌套PQexec) */
	char	   *conninfo;						/* 连接信息字符串，用于创建临时连接 */

	/* ReqID:TECC4.0 - Pending tables bridge for same-transaction DDL+DML.
	 * Non-NULL during the window between DDL preprocessing and the next
	 * cache refresh. Cleared on cache refresh or connection cleanup. */
	/* pending_tables removed, replaced by PENDING cache sentinel */

	/* ReqID:TECC4.0 - PGresult block recycling pool.
	 * Pool lifetime is independent of PGconn: when a connection closes,
	 * blocks in the pool are freed but the pool struct (16 bytes) persists.
	 * This prevents dangling-pointer crashes when a PGresult outlives its
	 * originating PGconn (e.g. psql \c reconnect scenarios). */
	TeccBlockPool block_pool;		/* PGresult_data block recycling */

	/* ReqID:TECC4.0 - Prepared statement query text store */
	struct TeccStoredQuery *stored_queries;
} TeccState;

/* ReqID:TECC4.0 - Prepared statement query text store */
typedef struct TeccStoredQuery
{
	char	   *stmt_name;
	char	   *query_string;
	struct TeccStoredQuery *next;
} TeccStoredQuery;

extern void tecc_store_prepared_query(PGconn *conn, const char *stmt_name,
								  const char *query);
extern const char *tecc_get_prepared_query(PGconn *conn, const char *stmt_name);
extern void tecc_remove_prepared_query(PGconn *conn, const char *stmt_name);

/* ReqID:TECC4.0 - PGresult block recycling API */
extern void *tecc_pgresult_block_alloc(TeccBlockPool *pool);
extern void  tecc_pgresult_block_free(TeccBlockPool *pool, void *block);

/* Query parameter processing structure */
typedef struct TeccQueryParams
{
	/* Rewritten query */
	char	   *rewritten_query;			/* Rewritten query string */

	/* Adjusted parameters */
	const char **adjusted_values;			/* Adjusted parameter values array */
	int		   *adjusted_lengths;			/* Adjusted parameter lengths array */
	int		   *adjusted_formats;			/* Adjusted parameter formats array */

	/* Original parameter backup */
	const char **original_values;			/* Original parameter values array */
	int		   *original_lengths;			/* Original parameter lengths array */
	int		   *original_formats;			/* Original parameter formats array */

	/* Parameter information */
	int			nParams;					/* Number of parameters */
	bool		params_modified;			/* Whether parameters were modified */
	bool		query_modified;				/* Whether query was modified */

	/* Memory management */
	bool		owns_memory;				/* Whether owns memory */
} TeccQueryParams;

/* Query result processing structure */
typedef struct TeccResultInfo
{
	/* Associated result */
	PGresult   *result;						/* Associated PGresult pointer */

	/* Encrypted column information */
	int		   *encrypted_columns;			/* Encrypted column index array */
	int			encrypted_column_count;		/* Number of encrypted columns */

	/* Decryption state */
	bool		decryption_completed;		/* Whether decryption is completed */

	/* Decrypted values */
	char	  **decrypted_values;			/* Decrypted values array */
	int			decrypted_value_count;		/* Number of decrypted values */

	/* Memory management */
	void *mem_pool;							/* Memory pool pointer (for future use) */
} TeccResultInfo;

/* Connection management interfaces */
extern TeccState *tecc_state_init(PGconn *conn);
extern void tecc_state_cleanup(TeccState *state);
extern bool tecc_is_enabled(const TeccState *state);

/* ReqID:TECC4.0 - Pending table bridge for same-transaction DDL+DML */
/* pending_tables functions removed, replaced by tecc_cache_*_pending() */

/* Query preprocessing interfaces */
extern char *tecc_preprocess_query(PGconn *conn, const char *query);
extern TeccQueryParams *tecc_preprocess_params(PGconn *conn,
											   const char *command,
											   int nParams,
											   const Oid *paramTypes,
											   const char *const *paramValues,
											   const int *paramLengths,
											   const int *paramFormats);
extern void tecc_free_query_params(TeccQueryParams *params);

/* Result processing interfaces */
extern void tecc_process_result(PGconn *conn, PGresult *result);

/* Error handling interfaces */
extern void tecc_handle_error(PGconn *conn,
							  int error_code,
							  const char *error_msg);
extern bool tecc_refresh_metadata_cache(PGconn *conn);
extern int PQteccRefreshCache(PGconn *conn);
extern const char *tecc_get_last_error(const PGconn *conn);

/* Utility functions */
extern TeccState *tecc_get_state(const PGconn *conn);
extern bool tecc_is_initialized(const PGconn *conn);
extern int tecc_get_stats(const PGconn *conn, TeccStats *stats);

/* Unified state management interfaces */
extern bool tecc_state_check_cache_refresh(TeccState *state);
extern int tecc_state_refresh_cache(TeccState *state);
extern void tecc_state_record_error(TeccState *state, int error_code, const char *error_msg);
extern bool tecc_state_should_retry(TeccState *state);
extern void *tecc_state_get_raw_values_list(TeccState *state);
extern void *tecc_state_get_prepared_statements(TeccState *state);
extern void tecc_state_reset_error_recovery(TeccState *state);

/* Internal interfaces to other TECC modules */
/* Note: These are implemented in other modules */
extern List *tecc_parser_parse(const char *query);
extern void tecc_scanner_init(const char *query);
extern void tecc_parser_free_tree(List *parse_tree);

extern TeccProcessorResult tecc_processor_run_pre_query(TeccStatementData *statement_data);
extern TeccProcessorResult tecc_processor_run_pre_exec(TeccStatementData *statement_data);
extern TeccProcessorResult tecc_processor_run_post_query(PGconn *conn, PGresult *result);
extern int tecc_processor_has_encrypted_columns(TeccStatementData *statement_data);
extern List *tecc_processor_get_encrypted_columns(TeccStatementData *statement_data);



/* ==================== COPY Processing Interfaces ==================== */
/* See fe-tecc-copy.h for COPY and batch processing declarations */
#include "fe-tecc-copy.h"

/* ==================== Prepared Statement Processing Interfaces ==================== */

/* 准备语句预处理接口 */
extern TeccPreparedStatementContext *tecc_preprocess_prepared_statement(
    PGconn *conn, const char *stmt_name, const char *query,
    int nParams, const Oid *paramTypes);

/* 准备语句参数预处理接口 */
extern TeccQueryParams *tecc_preprocess_prepared_params(
    PGconn *conn, const char *stmt_name, int nParams,
    const char *const *paramValues, const int *paramLengths,
    const int *paramFormats);

/* 准备语句清理接口 */
extern void tecc_cleanup_prepared_statement(PGconn *conn, const char *stmt_name);

/* 检查准备语句是否存在 */
extern bool tecc_prepared_statement_exists(TeccPreparedStatementContext *context, const char *stmt_name);

/* ==================== Asynchronous API Processing Interfaces ==================== */

/* Asynchronous state management interfaces */
extern TeccAsyncState *tecc_async_state_create(PGconn *conn,
                                               TeccAsyncOpType op_type,
                                               const char *op_id);
extern int tecc_async_state_save(PGconn *conn, TeccAsyncState *async_state);
extern TeccAsyncState *tecc_async_state_get(PGconn *conn, const char *op_id);
extern void tecc_async_state_cleanup(PGconn *conn, const char *op_id);
extern void tecc_async_state_cleanup_all(PGconn *conn);

/* Asynchronous query preprocessing interfaces */
extern TeccAsyncState *tecc_async_preprocess_query(PGconn *conn,
                                                   const char *query,
                                                   const char *async_id);
extern TeccAsyncState *tecc_async_preprocess_params(PGconn *conn,
                                                    const char *command,
                                                    int nParams,
                                                    const Oid *paramTypes,
                                                    const char *const *paramValues,
                                                    const int *paramLengths,
                                                    const int *paramFormats,
                                                    const char *async_id);

/* Asynchronous prepared statement processing interfaces */
extern TeccAsyncState *tecc_async_preprocess_prepared_statement(
    PGconn *conn, const char *stmt_name, const char *query,
    int nParams, const Oid *paramTypes, const char *async_id);
extern TeccAsyncState *tecc_async_preprocess_prepared_params(
    PGconn *conn, const char *stmt_name, int nParams,
    const char *const *paramValues, const int *paramLengths,
    const int *paramFormats, const char *async_id);

/* Asynchronous result processing interfaces */
extern int tecc_async_process_result(PGconn *conn, PGresult *result,
                                     const char *async_id);
extern int tecc_async_process_copy_data(PGconn *conn, const char *data,
                                        int data_len, char **processed_data,
                                        int *processed_len, const char *async_id);
extern int tecc_async_process_copy_result(PGconn *conn,
                                          const char *encrypted_data,
                                          int encrypted_len,
                                          char **decrypted_data,
                                          int *decrypted_len,
                                          const char *async_id);

/* Asynchronous state query interfaces */
extern bool tecc_async_is_completed(PGconn *conn, const char *async_id);
extern const char *tecc_async_get_error(PGconn *conn, const char *async_id);
extern int tecc_async_cancel(PGconn *conn, const char *async_id);
extern int tecc_async_get_stats(PGconn *conn, TeccAsyncStats *stats);

/* ==================== New State Management Interfaces (借鉴openGauss PGClientLogic) ==================== */

/* Get pending statements list */
extern TeccPendingStatementNode *tecc_state_get_pending_statements(TeccState *state);

/* Disable TECC once (temporary disable) */
extern void tecc_disable_once(PGconn *conn);

/* Reset disable_once flag */
extern void tecc_reset_disable_once(PGconn *conn);

/* Get last statement name */
extern const char *tecc_get_last_stmt_name(PGconn *conn);

/* Accept pending statements */
extern int tecc_accept_pending_statements(PGconn *conn);

/* Cleanup pending statements */
extern void tecc_cleanup_pending_statements(PGconn *conn);

/* ==================== GUC Parameters Management Interfaces ==================== */

/* Begin - ReqID:TECC4.0 - GUC参数管理接口 */

/* Apply GUC parameter updates */
extern bool tecc_guc_apply_update(PGconn *conn, const TeccUpdateGucValues *updates);

/* Get current GUC parameters */
extern const TeccGucParams *tecc_guc_get_params(PGconn *conn);

/* Set GUC parameter value */
extern bool tecc_guc_set_param(PGconn *conn, const char *param_name, const char *param_value);

/* Reset GUC parameters to defaults */
extern void tecc_guc_reset_to_defaults(PGconn *conn);

/* Debug logging function */
extern void tecc_record_debug(const TeccState *state, const char *format, ...) pg_attribute_printf(2, 3);

/* End - ReqID:TECC4.0 - GUC参数管理接口 */

#endif /* FE_TECC_INTEGRATION_H */