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
	bool		update_tecc_mode;			/* Update TECC result decryption mode */

	/* Values to update (only valid if corresponding flag is true) */
	char		new_role[64];				/* New role value */
	bool		new_standard_conforming_strings; /* New standard_conforming_strings value */
	int			new_backslash_quote;		/* New backslash_quote value */
	bool		new_escape_string_warning;	/* New escape_string_warning value */
	char		new_searchpath[1024];		/* New searchpath value */
	bool		new_tecc_mode;				/* New TECC result decryption mode */

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

	/*
	 * ReqID:TECC4.0 - DDL-only mode（分布式内部连接）：
	 * 只做 DDL 解析与 ENCRYPTED WITH 子句合成，不做结果解密/常量加密/密钥处理。
	 * 见 plan/2026-10-28-hgdde分布式适配可行性分析.md §0.3。
	 */
	bool		ddl_only;

	/*
	 * ReqID:TECC4.0 - 取数目标覆盖（DDL-only 专用）。
	 * 内部连接连的是 worker，而逻辑表的加密声明在协调节点上；本字段非 NULL 时
	 * 取数器改用它建临时连接（见 tecc_create_temp_connection）。
	 */
	char	   *fetch_conninfo_override;

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

	/* ReqID:TECC4.0 - 密钥文件生命周期由 tecc_ktool 管理（generate/delete）：
	 * SQL DROP KEK/DEK 只清理目录元数据 + 客户端元数据缓存，不再触碰磁盘
	 * 密钥文件（.key/.priv/.pub），因此无需 last_kek_path/延迟队列/CASCADE 快照。 */
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

	/* ReqID:TECC4.0 - SELECT 表达式结果列（聚合/运算）的底层加密列元数据。
	 * PG 协议对表达式结果（如 sum(enc_col)）不返回 table_oid/attnum，
	 * 客户端预处理（tecc_stmt_processor_process_select）解析表达式参数中
	 * 的加密列，按名保存（schema/table/column），结果解密时
	 * （tecc_processor_run_post_query）按名查缓存恢复列元数据，保证解密
	 * 使用正确的 DEK 与算法（HE-only 走 HE 路径，对称列走对称路径）。 */
#define TECC_MAX_EXPR_RESULT_COLS 64
	int			pending_expr_col_count;
	char		pending_expr_col_schemas[TECC_MAX_EXPR_RESULT_COLS][NAMEDATALEN];
	char		pending_expr_col_tables[TECC_MAX_EXPR_RESULT_COLS][NAMEDATALEN];
	char		pending_expr_col_columns[TECC_MAX_EXPR_RESULT_COLS][NAMEDATALEN];
	bool		pending_decrypt_raw_call;
	bool		pending_expr_col_raw_hex[TECC_MAX_EXPR_RESULT_COLS];
	int			pending_expr_col_field_indexes[TECC_MAX_EXPR_RESULT_COLS];
	char		pending_expr_col_raw_algorithms[TECC_MAX_EXPR_RESULT_COLS][16];

	/* ReqID:TECC4.0 - 会话级临时表清单。
	 * 记录本会话 CREATE TEMP TABLE 创建的表名（PG 语义：未限定表名解析时临时表
	 * 优先于 search_path）。临时表在数据获取器池连接中不可见（会话私有），
	 * 前端必须靠此清单识别，避免把未限定名误解析到 search_path 中的同名表。 */
#define TECC_MAX_TEMP_TABLES 64
	int			temp_table_count;		/* 临时表数量 */
	char		temp_table_names[TECC_MAX_TEMP_TABLES][NAMEDATALEN];	/* 临时表名 */

	/* ReqID:TECC4.0 - 连接级 HSM 设备缓存（替代 tecc_setup_hsm_context 中的 static 变量）
	 * 每个 PGconn 独立缓存 HSM adapter/device 句柄，连接关闭时自动清理。
	 * adapter 是进程级单例 vtable（pthread_once 保护），不需要释放；
	 * device 是 init() 分配的，需要在 cleanup 时 disconnect + free。 */
	void	   *hsm_tz_adapter;		/* 天正 HsmAdapterInterface*（单例，不释放） */
	void	   *hsm_tz_device;			/* 天正 HsmDevice*（需 cleanup 释放） */
	int			hsm_tz_tried;			/* 0=未尝试, 1=成功, -1=失败 */
	void	   *hsm_swxa_adapter;		/* 三未 HsmAdapterInterface*（单例，不释放） */
	void	   *hsm_swxa_device;		/* 三未 HsmDevice*（需 cleanup 释放） */
	int			hsm_swxa_tried;			/* 0=未尝试, 1=成功, -1=失败 */

	/* ReqID:TECC4.0 - 连接级 HSM 初始化锁（pthread_mutex_t*，由
	 * tecc_hsm_init_lock_create/tecc_hsm_init_lock_destroy 管理）。
	 * 同态字段级并行的多个 worker 线程会并发进入 tecc_setup_hsm_context 的
	 * 首次初始化段，无锁时各自 init + SDF_OpenSession，而上面的缓存槽只有
	 * 一份、只留最后一个，其余会话（含各自的会话密钥句柄缓存）无人回收、
	 * 一直挂到进程退出。本锁只串行化这**一次性**的初始化段，不进每值热路径。
	 * 软加密（localkms）列在 KEK 路由阶段即早退，不经过该段，不受本锁影响。
	 * NULL 表示未创建（此时降级为不加锁，行为与加锁前一致）。 */
	void	   *hsm_init_lock;

	/* ReqID:TECC4.0 - pg_upgrade V4.0.0 兼容：V4.0.0 的
	 * hg_encrypted_columns_info 视图没有 he_add_dek/he_mul_dek 两列
	 * （V4.0.1 新增 HE 功能才引入）。首次探测后缓存，缺列时数据获取器
	 * 查询不再引用这两列，保证 V4.0.1 前端工具可连接 V4.0.0 库。 */
	bool		he_cols_probed;			/* 是否已完成探测 */
	bool		has_he_cols;			/* 视图是否含 he_add_dek/he_mul_dek */

	/* ReqID:TECC4.0 - 库默认排序规则的 locale 文本（ICU 归一化用），每连接查一次后缓存。
	 * 用于"加密时目标列还不存在"的路径（加密列 DEFAULT、CTAS、视图/物化视图
	 * 定义里的密态 cast）：这类列未写 COLLATE 时继承的就是库默认排序规则。
	 * 一条 PG 连接绑定一个库（换库必须重连），因此该值连接内恒定。
	 * NULL 表示尚未成功解析过（解析失败不缓存，下次重试并告警）。 */
	char	   *default_collation_locale;
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
/* ReqID:TECC4.0 - server-process detection for the internal-connection fallback */
extern bool tecc_running_in_server_process(void);

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
/* ReqID:TECC4.0 - Detect tecc_decrypt_raw() in query text outside literals */
extern bool tecc_query_calls_decrypt_raw(const char *query);

/* ReqID:TECC4.0 - 会话级临时表清单管理（CREATE TEMP TABLE 记录，DROP 移除，
 * 未限定表名解析时优先命中，语义与 PG 的 pg_temp 优先一致） */
extern bool tecc_state_is_temp_table(const TeccState *state, const char *relname);
extern void tecc_state_add_temp_table(TeccState *state, const char *relname);
extern void tecc_state_remove_temp_table(TeccState *state, const char *relname);

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

/* ==================== New State Management Interfaces (借鉴openGauss PGClientLogic) ==================== */

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
