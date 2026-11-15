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
#include "fe-auth.h"
#include "fe-tecc-integration.h"
/* #include "fe-tecc-metadata.h" */  /* This header may have been replaced by tecc_metadata_cache.h */
/* #include "fe-tecc-key.h" */       /* This header may have been replaced by tecc_key_manager_adapter.h */
#include "../tecc_hsm_adapter/fe-hsm-adapter.h"	/* ReqID:TECC4.0 - HSM cleanup in tecc_state_cleanup */
/* Note: fe-tecc-parser.h is not used - it's for the old frontend parser architecture */
/* #include "fe-tecc-parser.h" */

/* Include other TECC module headers */
#include "../tecc_cache/tecc_metadata_cache.h"
#include "../tecc_processor/tecc_processor.h"
#include "../tecc_processor/internal/tecc_processor_internal.h"  /* For tecc_processor_debug macro */
#include "../tecc_scheduler/tecc_crypto_scheduler.h"
#include "../tecc_scheduler/tecc_key_manager_adapter.h"
/* ReqID:TECC4.0 - Bug 9263: tecc_he_meta_cache_clear()（轮换后清 HE 元数据缓存） */
#include "../tecc_scheduler/tecc_algorithm_scheduler.h"
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
bool tecc_query_calls_decrypt_raw(const char *query);
static bool tecc_is_identifier_char(char ch);

/* Prepared statement helper functions */

/* Pending statements management functions */
static void tecc_add_to_pending_statements(TeccState *state, const char *stmt_name, TeccPreparedStatementContext *context);
static void tecc_move_pending_to_prepared(TeccState *state);
static void tecc_cleanup_pending_list(TeccState *state);

/*
 * tecc_is_identifier_char - Check SQL identifier body character.
 */
static bool
tecc_is_identifier_char(char ch)
{
	return ((ch >= 'a' && ch <= 'z') ||
			(ch >= 'A' && ch <= 'Z') ||
			(ch >= '0' && ch <= '9') ||
			ch == '_');
}

/*
 * tecc_query_calls_decrypt_raw - Detect tecc_decrypt_raw() outside literals.
 */
bool
tecc_query_calls_decrypt_raw(const char *query)
{
	const char *p;
	const char *name = "tecc_decrypt_raw";
	size_t		name_len;
	bool		in_single_quote = false;
	bool		in_double_quote = false;
	bool		in_line_comment = false;
	bool		in_block_comment = false;

	if (query == NULL)
		return false;

	p = query;
	name_len = strlen(name);
	while (*p != '\0')
	{
		if (in_line_comment)
		{
			if (*p == '\n' || *p == '\r')
				in_line_comment = false;
			p++;
			continue;
		}

		if (in_block_comment)
		{
			if (p[0] == '*' && p[1] == '/')
			{
				in_block_comment = false;
				p += 2;
			}
			else
				p++;
			continue;
		}

		if (in_single_quote)
		{
			if (*p == '\'')
			{
				if (p[1] == '\'')
					p += 2;
				else
				{
					in_single_quote = false;
					p++;
				}
			}
			else
				p++;
			continue;
		}

		if (in_double_quote)
		{
			if (*p == '"')
			{
				if (p[1] == '"')
					p += 2;
				else
				{
					in_double_quote = false;
					p++;
				}
			}
			else
				p++;
			continue;
		}

		if (p[0] == '-' && p[1] == '-')
		{
			in_line_comment = true;
			p += 2;
			continue;
		}

		if (p[0] == '/' && p[1] == '*')
		{
			in_block_comment = true;
			p += 2;
			continue;
		}

		if (*p == '\'')
		{
			in_single_quote = true;
			p++;
			continue;
		}

		if (*p == '"')
		{
			in_double_quote = true;
			p++;
			continue;
		}

		if (pg_strncasecmp(p, name, name_len) == 0 &&
			(p == query || !tecc_is_identifier_char(*(p - 1))) &&
			!tecc_is_identifier_char(p[name_len]))
			return true;

		p++;
	}

	return false;
}

/* Begin - ReqID:TECC4.0 */
/*
 * tecc_running_in_server_process
 *		Report whether this libpq instance is running inside a PostgreSQL
 *		server process rather than inside a separate client process.
 *
 * Why this must be answered: TECC的客户端处理是直接编进 libpq 的，而它的
 * enable_tecc 选项编译默认为 true。服务端自己从后端进程里开的 libpq 连接
 * （Citus 的内部连接、dblink、postgres_fdw、TECC 自己的元数据取数连接……）
 * 会因此把改写与结果解密跑在**后端进程内**：分片密文在后端就被解密、常量被
 * 二次加密，现象极难解释（实测：dblink 读加密列返回的是解密后的明文）。
 * 因此凡是"在服务端进程内部建立"的连接，一律不启用密态处理。
 *
 * 判定手法＝弱符号探测：PostmasterMain() 由服务端可执行文件定义，并随
 * `-rdynamic` 进到它的动态符号表；而 psql / 各驱动 .so 里没有它。libpq 里对
 * 它取一个**弱引用**，符号被解析到 ⇒ 本 libpq 被装进了服务端进程；保持 0 ⇒
 * 普通客户端。这个判定不需要任何配置、不依赖环境变量，也不会被后端进程
 * fork/exec 出来的客户端误继承。
 *
 * 非 ELF 弱符号的编译器（MSVC）下探测恒为 false，退化为"不隔离"——本项要防的
 * 分布式场景（Citus）只在类 Unix 上构建。
 */
#ifdef __GNUC__
extern int	PostmasterMain(int argc, char *argv[]) __attribute__((weak));

bool
tecc_running_in_server_process(void)
{
	return (&PostmasterMain != NULL);
}
#else
bool
tecc_running_in_server_process(void)
{
	return false;
}
#endif
/* End - ReqID:TECC4.0 */

/* Begin - ReqID:TECC4.0 - 同进程查询 ENCRYPTED WITH 子句（弱符号） */
/*
 * hg_get_encrypted_clause_by_name()
 *		PG 核心侧的实现（src/backend/commands/encrypted_column.c）。
 *
 *		只在"libpq 被装进服务端进程"时才能解析到；普通客户端进程里该符号为 NULL，
 *		自然不会走这条路（DDL-only 本身也只出现在服务端进程里）。
 *
 *		为什么优先用它：它读的是**本进程**的目录，因而
 *		  - 能看到本事务尚未提交的行（ALTER ADD COLUMN 刚写的那行）；
 *		  - 不依赖目标节点的 pg_dist_node（加节点建壳表时新节点上还是空的）。
 *		取数器（另开会话）做不到这两点。
 */
#ifdef __GNUC__
extern char *hg_get_encrypted_clause_by_name(const char *schema, const char *table,
											 const char *column)
	__attribute__((weak));
#endif
/* End - ReqID:TECC4.0 - 同进程查询 ENCRYPTED WITH 子句（弱符号） */

/*
 * tecc_state_init - Initialize TECC state for a connection
 *
 * Returns: TeccState pointer on success, NULL on failure
 */
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

	/* ReqID:TECC4.0 - 连接级 HSM 初始化锁：此时还没有 worker 线程，创建本身无竞争。
	 * 创建失败只降级为不加锁（hsm_init_lock 保持 NULL），不影响硬加密之外的任何
	 * 功能，也不影响软加密（localkms）路径——后者根本不经过该锁。 */
	if (tecc_hsm_init_lock_create(state) != 0)
		tecc_record_debug(NULL, "TECC HSM init lock unavailable; HSM first-init will not be serialized");

	/* Set basic state */
	state->enabled = conn->enable_tecc;
	/* ReqID:TECC4.0 - 三态：DDL-only（分布式内部连接）只做 DDL，不碰数据面 */
	state->ddl_only = conn->tecc_ddl_only;

	/* ReqID:TECC4.0 - 复制协议连接豁免：conninfo 含 replication 参数
	 * （replication=true/database，pg_basebackup/pg_receivewal/
	 * pg_recvlogical 等工具）走 walsender 复制协议，无普通 SQL 元数据
	 * 查询能力。TECC 预处理/权限检查/临时连接在这些连接上会反复失败
	 * （TEMP_CONN: CREATE FAILED）并干扰工具自身流程，且复制场景
	 * 不涉及密态数据处理，故整体禁用 TECC 处理。 */
	if (conn->replication != NULL && conn->replication[0] != '\0')
		state->enabled = false;

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
        /* ReqID:TECC4.0 - PQconninfo reads parsed PGconn options; it does not
         * open another connection or require a completed SSL handshake. */
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

		/*
		 * enable_tecc=false：取数连接自己绝不能再走密态处理，否则取元数据
		 * 这件事本身会递归地触发元数据查询。这里把选项显式写进取数连接自己的
		 * conninfo —— 只影响这一条连接，而不是像以前那样进程级改环境变量
		 * （那会把同一进程里之后建立的**所有**连接都关掉密态，后端进程内的
		 * 连接尤其危险）。
		 */
		snprintf(conninfo_buf, sizeof(conninfo_buf),
				 "host='%s' port='%s' dbname='%s' user='%s'%s%s%s enable_tecc=false",
				 host_buf, port_buf, db_buf, user_buf,
				 (pass && pass[0]) ? " password='" : "",
				 (pass && pass[0]) ? pass_buf : "",
				 (pass && pass[0]) ? "'" : "");

        /* ReqID:TECC4.0 - metadata queries must inherit the same transport
         * policy, including TLCP's dual certificates. Do not log this buffer:
         * sslpassword is a secret, like the existing database password. */
        {
            PQExpBufferData full_conninfo;
            PQconninfoOption *options = PQconninfo(conn);
            PQconninfoOption *option;

            if (options == NULL)
            {
                pg_free(state);
                return NULL;
            }
            initPQExpBuffer(&full_conninfo);
            appendPQExpBufferStr(&full_conninfo, conninfo_buf);
            for (option = options; option->keyword; option++)
            {
                const char *p;

                if (!option->val || /* Preserve explicit empty overrides too. */
                    (strncmp(option->keyword, "ssl", 3) != 0 &&
                     strcmp(option->keyword, "gssencmode") != 0 &&
                     strcmp(option->keyword, "channel_binding") != 0 &&
                     strcmp(option->keyword, "connect_timeout") != 0))
                    continue;
                appendPQExpBuffer(&full_conninfo, " %s='", option->keyword);
                for (p = option->val; *p; p++)
                {
                    if (*p == '\\' || *p == '\'')
                        appendPQExpBufferChar(&full_conninfo, '\\');
                    appendPQExpBufferChar(&full_conninfo, *p);
                }
                appendPQExpBufferChar(&full_conninfo, '\'');
            }
            PQconninfoFree(options);
            if (PQExpBufferDataBroken(full_conninfo))
            {
                termPQExpBuffer(&full_conninfo);
                pg_free(state);
                return NULL;
            }
            state->conninfo = pg_strdup(full_conninfo.data);
            termPQExpBuffer(&full_conninfo);
        }

	}
	/* Set default GUC values */
	/* Begin - ReqID:TECC4.0 - 设置简化GUC参数的默认值 */
	strcpy(state->guc_params.role, "");		/* Empty role by default */
	state->guc_params.standard_conforming_strings = true; /* Enable by default */
	state->guc_params.backslash_quote = TECC_BACKSLASH_QUOTE_SAFE_ENCODING; /* Safe encoding by default */
	state->guc_params.escape_string_warning = true; /* Enable warning by default */
	strcpy(state->guc_params.searchpath, "\"$user\", public"); /* Default search path */
	state->guc_params.tecc_mode = true; /* Result decryption enabled by default */
	/* End - ReqID:TECC4.0 - 设置简化GUC参数的默认值 */

	/* Check if TECC is enabled */
	if (state->ddl_only)
	{
		/*
		 * ReqID:TECC4.0 - DDL-only（分布式内部连接）。
		 *
		 * 数据面必须与改造前完全等价：改造前这里强行 enable_tecc=false，
		 * 所以 state->enabled 保持 false，所有数据闸门（常量加密、结果解密、
		 * COPY、密钥处理、加解密上下文）照旧全部关闭。
		 *
		 * 唯一多准备的是"取元数据"的能力：DDL 子句合成要读
		 * hg_encrypted_columns_info。密钥管理/加解密调度器一律不建（DDL 通路
		 * 不做任何加解密，内部连接不该因为拿不到密钥而起不来）。
		 *
		 * 取数器初始化失败不致命：查不到元数据就不补子句 → worker 侧照旧
		 * 报 requires ENCRYPTED WITH clause（失败关闭）。
		 */
		state->enabled = false;

		if (!tecc_data_fetcher_init_for_state(state))
		{
			tecc_record_error(state, TECC_ERR_DATA_FETCHER,
							  "Failed to initialize data fetcher in DDL-only mode; "
							  "ENCRYPTED WITH clauses cannot be synthesized",
							  __func__, __LINE__, true);
		}

		state->initialized = true;
		return state;
	}

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

	/*
	 * ReqID:TECC4.0 - DDL-only 模式到此为止。
	 *
	 * 下面三段分别初始化密钥管理适配器、加解密调度器与加解密上下文，它们会
	 * 触碰 KEK/DEK 与密码机（localkms 要读密钥文件、HSM 要建会话），而 DDL-only
	 * 只做"解析 DDL + 合成子句"，一次加解密都不做，这些初始化纯属多余且可能失败
	 * （内部连接不该因为拿不到密钥而起不来）。故整段跳过。
	 *
	 * 详见 plan/2026-10-28-hgdde分布式适配可行性分析.md §0.3.2。
	 */
	if (state->ddl_only)
	{
		state->initialized = true;
		return state;
	}

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

	/* ReqID:TECC4.0 - 释放连接级 HSM 初始化锁（必须在函数末尾的 memset 之前，
	 * 否则互斥量泄漏；destroy 幂等，锁未创建时是空操作）。 */
	tecc_hsm_init_lock_destroy(state);

	/* ReqID:TECC4.0 - 清理连接级 HSM 设备缓存 */
	if (state->hsm_tz_device != NULL)
	{
		HsmAdapterInterface *hsm = (HsmAdapterInterface *)state->hsm_tz_adapter;
		if (hsm != NULL)
		{
			hsm->disconnect((HsmDevice *)state->hsm_tz_device);
			hsm->cleanup((HsmDevice *)state->hsm_tz_device);
		}
		state->hsm_tz_device = NULL;
		state->hsm_tz_adapter = NULL;
		state->hsm_tz_tried = 0;
	}
	if (state->hsm_swxa_device != NULL)
	{
		HsmAdapterInterface *hsm = (HsmAdapterInterface *)state->hsm_swxa_adapter;
		if (hsm != NULL)
		{
			hsm->disconnect((HsmDevice *)state->hsm_swxa_device);
			hsm->cleanup((HsmDevice *)state->hsm_swxa_device);
		}
		state->hsm_swxa_device = NULL;
		state->hsm_swxa_adapter = NULL;
		state->hsm_swxa_tried = 0;
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

	/* ReqID:TECC4.0 - 释放库默认排序规则 locale 缓存（下面的 memset 会丢指针） */
	if (state->default_collation_locale != NULL)
	{
		pg_free(state->default_collation_locale);
		state->default_collation_locale = NULL;
	}

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
    bool        use_temp_conn = false;
    PGconn     *qconn = NULL;
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

    /* ReqID:TECC4.0 - 连接选择：非管道模式用主连接（保持同事务可见性，
     * 显式事务内 CREATE TABLE 后立即 DML 能看到未提交 DDL）；
     * 管道模式下主连接禁止同步 PQexec，改用 data_fetcher 独立连接。 */
    if (conn->pipelineStatus != PQ_PIPELINE_OFF)
    {
        qconn = tecc_data_fetcher_get_connection(state);
        if (qconn == NULL)
        {
            tecc_processor_debug(TECC_LOG_WARNING,
                "tecc_refresh_table_cache: temp connection failed for %s.%s (pipeline mode)",
                schema_name, table_name);
            tecc_record_error(state, TECC_ERR_DATA_FETCHER,
                              "TECC: cannot refresh table metadata in pipeline mode (temp connection failed)",
                              __func__, __LINE__, false);
            return;
        }
        use_temp_conn = true;
    }
    else
    {
        qconn = conn;
    }

    /* Escape identifiers for SQL (on the query connection) */
    esc_table = PQescapeLiteral(qconn, table_name, strlen(table_name));
    esc_schema = PQescapeLiteral(qconn, schema_name, strlen(schema_name));
    if (esc_table == NULL || esc_schema == NULL)
    {
        if (esc_table) PQfreemem(esc_table);
        if (esc_schema) PQfreemem(esc_schema);
        if (use_temp_conn)
            tecc_data_fetcher_release_connection(state, qconn);
        return;
    }

    /* Build query matching the data fetcher's pattern */
    snprintf(query, sizeof(query),
        "SELECT cls.oid AS table_oid, column_attnum, column_name, "
        "data_type_original_oid, data_type_original_mod, dek_name, "
        "encryption_type, collation_locale, column_key_id, create_date, "
        "has_dek_privilege(column_key_id, 'USAGE') AS has_dek_permission, "
        "he_add_dek, he_mul_dek "
        "FROM hg_encrypted_columns_info "
        "JOIN pg_class cls ON table_oid = cls.oid "
        "JOIN pg_namespace ns ON cls.relnamespace = ns.oid "
        "WHERE cls.relname = %s AND ns.nspname = %s "
        "AND current_database() = '%s' "
        "ORDER BY column_attnum",
        esc_table, esc_schema, db_name);

    PQfreemem(esc_table);
    PQfreemem(esc_schema);

    /* Disable TECC preprocessing to prevent recursion (only relevant on
     * the main connection; temp connections already have enable_tecc=false) */
    saved_enable_tecc = conn->enable_tecc;
    conn->enable_tecc = false;
    res = PQexec(qconn, query);
    conn->enable_tecc = saved_enable_tecc;

    if (res == NULL || PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        tecc_processor_debug(TECC_LOG_WARNING,
            "tecc_refresh_table_cache: query failed for %s.%s",
            schema_name, table_name);
        if (res) PQclear(res);
        /* ReqID:TECC4.0 - 管道模式下查询失败（独立连接看不到未提交 DDL 或
         * 连接错误）必须中止，不能静默当作"无加密列"，否则后续 DML 会明文
         * 发送到密态列。保留 PENDING 哨兵不变。 */
        if (use_temp_conn)
        {
            tecc_record_error(state, TECC_ERR_DATA_FETCHER,
                              "TECC: failed to load table metadata in pipeline mode",
                              __func__, __LINE__, false);
            tecc_data_fetcher_release_connection(state, qconn);
        }
        return;
    }

    ntuples = PQntuples(res);
    if (ntuples == 0)
    {
        /* No encrypted columns found — remove PENDING sentinel only */
        /* ReqID:TECC4.0 - 管道模式独立连接下 0 行可能意味着表刚在显式事务中
         * 创建尚未提交（独立连接不可见）。此时不能静默当作"无加密列"，
         * 应中止以避免明文发送。 */
        if (use_temp_conn)
        {
            tecc_processor_debug(TECC_LOG_WARNING,
                "tecc_refresh_table_cache: 0 rows via temp connection for %s.%s (uncommitted DDL?)",
                schema_name, table_name);
            tecc_record_error(state, TECC_ERR_DATA_FETCHER,
                              "TECC: table metadata not visible via temp connection in pipeline mode "
                              "(CREATE TABLE may be uncommitted)",
                              __func__, __LINE__, false);
            PQclear(res);
            tecc_data_fetcher_release_connection(state, qconn);
            return;
        }
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

        /* ReqID:TECC4.0 - 一列多 DEK：HE 加法/乘法 DEK 名（纯 HE 列 dek_name 为 NULL） */
        if (PQgetisnull(res, i, 11) == 0)
            item->he_add_dek_name = pg_strdup(PQgetvalue(res, i, 11));
        if (PQgetisnull(res, i, 12) == 0)
            item->he_mul_dek_name = pg_strdup(PQgetvalue(res, i, 12));

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

    /* ReqID:TECC4.0 - Remove stale name sentinel (e.g. NOT_ENCRYPTED from a
     * prior failed DDL or empty-cache query) before inserting TABLE_LOADED.
     * Without this, a stale NOT_ENCRYPTED sentinel blocks the refresh and
     * causes subsequent DML to skip encryption. */
    tecc_cache_remove_name_sentinel(state, db_name, schema_name, table_name);

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

    /* ReqID:TECC4.0 - 管道模式下释放临时连接 */
    if (use_temp_conn)
        tecc_data_fetcher_release_connection(state, qconn);

    tecc_processor_debug(TECC_LOG_DEBUG1,
        "tecc_refresh_table_cache: loaded %d encrypted columns for %s.%s",
        ntuples, schema_name, table_name);
}

/*
 * ReqID:TECC4.0 - Bug 9332 同族：按 search_path 顺序回退解析 KEK/DEK 的真实模式。
 *
 * 前端的 current_schema 是从 search_path **文本**推出来的
 * （tecc_cache_manager.c: parse_current_schema_from_search_path()，注释虽写
 * "按PG规则"，实现只是取第一个名字），既不校验该模式是否存在，也不问服务端
 * （PG 不把 current_schema 作为 GUC_REPORT 上报，前端手上没有权威值）。
 * 而后端的 recomputeNamespacePath() 会把不存在的模式从 activeSearchPath 滤掉，
 * 建对象用的是"列表中第一个**存在**的模式"，于是下面这个场景两边分叉：
 *
 *     SET search_path = sch;   -- 前端据此把 current_schema 记为 sch
 *     DROP SCHEMA sch;         -- 后端 current_schema() 变成 NULL，但前端没有任何
 *                              -- 事件可感知（search_path 文本没变，无 ParameterStatus）
 *     CREATE KEK k ...         -- 后端落在下一个存在的模式（如 public），stmt_processor
 *                              -- 却按 "sch" 插了一条 oid=0 的 PENDING 条目
 *     （post 阶段）tecc_refresh_kek_cache(state, "k", "sch")
 *                              -- 按 sch 回查必然取不到 → 正式条目没建，那条假条目
 *                              -- 又正好挂在 current_schema 名下
 *
 * 后果：同一会话里裸名 CREATE DEK → "KEK 'k' cache miss"、裸名 ALTER KEK →
 * "Failed to load old KEK"，而 KEK 明明存在；用限定名或重开会话即恢复。
 * 全部 fail-closed，不会静默错写，但会把用户挡在门外。
 *
 * 表路径早就有这层回退（tecc_fetch_resolve_table_schema()：按 search_path 顺序
 * 逐个查 pg_class 取第一个命中，见 tecc_cache_manager.c 调用处），KEK/DEK 此前
 * 没有。这里补上同一套规则：按 search_path 顺序逐个模式回查，第一个命中即采用。
 * 只在"按预测模式取不到"时触发（正常路径不增加往返），候选顺序与
 * tecc_cache_get_*_by_name_search_path() 第 2 步的查找顺序一致，保证"登记的
 * 身份"与"查找的身份"是同一个。
 *
 * 不走"全库裸名兜底"（namespace 传 NULL）的原因：tecc_parse_kek_metadata() 拿到
 * 行就收、遇到不同 kek_oid 才停，跨模式同名 KEK 会静默取到 oid 最小的那一个，
 * 属 Bug 9330 那类静默错绑，不能用在身份解析上。
 */
#define TECC_RESOLVE_MAX_ATTEMPTS 8

static TeccKekCacheItem *
tecc_resolve_kek_by_search_path(TeccState *state, const char *kek_name,
                                const char *guessed_namespace)
{
    const char **schemas;
    int nschemas = 0;
    int i;
    int attempted = 0;

    schemas = tecc_cache_get_search_path_schemas(state, &nschemas);
    if (schemas == NULL)
        return NULL;

    for (i = 0; i < nschemas && attempted < TECC_RESOLVE_MAX_ATTEMPTS; i++)
    {
        TeccCacheQueryParams query;
        TeccKekCacheItem *item = NULL;
        int count = 0;
        const char *ns = schemas[i];

        if (ns == NULL || ns[0] == '\0')
            continue;
        /* 预测模式刚刚已经试过且失败，不重复 */
        if (guessed_namespace != NULL && strcmp(ns, guessed_namespace) == 0)
            continue;

        memset(&query, 0, sizeof(query));
        query.flags = TECC_QUERY_BY_NAME;
        query.params.by_name.name = kek_name;
        query.params.by_name.namespace = ns;
        query.external_conn = NULL;

        attempted++;
        if ((int)tecc_fetch_kek_for_cache(state, &query, &item, &count) == (int)TECC_CACHE_OK &&
            item != NULL)
        {
            tecc_processor_debug(TECC_LOG_DEBUG1,
                "tecc_resolve_kek_by_search_path: KEK '%s' 回退命中模式 '%s'（预测模式 '%s' 已失效）",
                kek_name, ns,
                (guessed_namespace != NULL && guessed_namespace[0] != '\0') ?
                    guessed_namespace : "(none)");
            return item;
        }
    }

    return NULL;
}

static TeccDekCacheItem *
tecc_resolve_dek_by_search_path(TeccState *state, const char *dek_name,
                                const char *guessed_namespace)
{
    const char **schemas;
    int nschemas = 0;
    int i;
    int attempted = 0;

    schemas = tecc_cache_get_search_path_schemas(state, &nschemas);
    if (schemas == NULL)
        return NULL;

    for (i = 0; i < nschemas && attempted < TECC_RESOLVE_MAX_ATTEMPTS; i++)
    {
        TeccCacheQueryParams query;
        TeccDekCacheItem *item = NULL;
        int count = 0;
        const char *ns = schemas[i];

        if (ns == NULL || ns[0] == '\0')
            continue;
        if (guessed_namespace != NULL && strcmp(ns, guessed_namespace) == 0)
            continue;

        memset(&query, 0, sizeof(query));
        query.flags = TECC_QUERY_BY_NAME;
        query.params.by_name.name = dek_name;
        query.params.by_name.namespace = ns;
        query.external_conn = NULL;

        attempted++;
        if ((int)tecc_fetch_dek_for_cache(state, &query, &item, &count) == (int)TECC_CACHE_OK &&
            item != NULL)
        {
            tecc_processor_debug(TECC_LOG_DEBUG1,
                "tecc_resolve_dek_by_search_path: DEK '%s' 回退命中模式 '%s'（预测模式 '%s' 已失效）",
                dek_name, ns,
                (guessed_namespace != NULL && guessed_namespace[0] != '\0') ?
                    guessed_namespace : "(none)");
            return item;
        }
    }

    return NULL;
}

/*
 * tecc_refresh_kek_cache - Load committed KEK metadata into formal cache.
 *
 * Called after CREATE KEK succeeds. Uses the data fetcher interface
 * (tecc_fetch_kek_for_cache) which manages its own temporary connection,
 * loading full KEK metadata and replacing the PENDING sentinel.
 * Same pattern as tecc_refresh_table_cache.
 */
static void
tecc_refresh_kek_cache(TeccState *state, const char *kek_name, const char *kek_namespace)
{
    TeccCacheQueryParams query;
    TeccKekCacheItem *item = NULL;
    int count = 0;
    TeccFetcherErrorCode ret;
    int insert_ret;

    if (state == NULL || kek_name == NULL ||
        kek_name[0] == '\0' || kek_namespace == NULL)
        return;

    tecc_processor_debug(TECC_LOG_DEBUG1,
        "tecc_refresh_kek_cache: loading %s.%s", kek_namespace, kek_name);

    memset(&query, 0, sizeof(query));
    query.flags = TECC_QUERY_BY_NAME;
    query.params.by_name.name = kek_name;
    query.params.by_name.namespace = kek_namespace;
    /* ReqID:TECC4.0 - 用临时连接获取元数据，不用主连接 conn。否则 post 阶段
     * 在 conn 上执行 PQexec 会干扰当前语句（如 ALTER KEK 的多语句重写）的
     * 结果，导致 psql 显示错误的命令标签。与 tecc_data_fetcher.c 中"不能使用
     * 主连接，因为 PQexec 会触发 TECC hook"的原则一致。 */
    query.external_conn = NULL;

    ret = tecc_fetch_kek_for_cache(state, &query, &item, &count);

    /* ReqID:TECC4.0 - Bug 9332 同族：按预测模式取不到时，按 search_path 顺序回退到
     * 真正落库的模式（原因见 tecc_resolve_kek_by_search_path 上方注释）。拿到的条目
     * 自带服务端上报的 key_namespace，下面按预测模式清 PENDING，正是要清掉那条挂在
     * 失效模式名下、oid=0 的假条目。 */
    if ((int)ret != (int)TECC_CACHE_OK || item == NULL)
        item = tecc_resolve_kek_by_search_path(state, kek_name, kek_namespace);

    if (item != NULL)
    {
        tecc_cache_remove_pending_kek(state, kek_name, kek_namespace);
        insert_ret = tecc_cache_insert_kek_item(state, item);
        if (insert_ret != 0)
        {
            tecc_processor_debug(TECC_LOG_WARNING,
                "tecc_refresh_kek_cache: insert failed ret=%d for %s.%s",
                insert_ret,
                item->key_namespace ? item->key_namespace : kek_namespace,
                kek_name);
        }
        tecc_processor_debug(TECC_LOG_DEBUG1,
            "tecc_refresh_kek_cache: loaded KEK %s.%s (oid=%u)",
            item->key_namespace ? item->key_namespace : kek_namespace,
            kek_name, item->kek_oid);
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
 * Called after CREATE DEK succeeds. Uses the data fetcher interface
 * (tecc_fetch_dek_for_cache) which manages its own temporary connection.
 * Same pattern as tecc_refresh_kek_cache.
 */
static void
tecc_refresh_dek_cache(TeccState *state, const char *dek_name, const char *dek_namespace)
{
    TeccCacheQueryParams query;
    TeccDekCacheItem *item = NULL;
    int count = 0;
    TeccFetcherErrorCode ret;
    int insert_ret;

    if (state == NULL || dek_name == NULL ||
        dek_name[0] == '\0' || dek_namespace == NULL)
        return;


    memset(&query, 0, sizeof(query));
    query.flags = TECC_QUERY_BY_NAME;
    query.params.by_name.name = dek_name;
    query.params.by_name.namespace = dek_namespace;
    query.external_conn = NULL;

    ret = tecc_fetch_dek_for_cache(state, &query, &item, &count);

    /* ReqID:TECC4.0 - Bug 9332 同族：同 tecc_refresh_kek_cache，按 search_path
     * 顺序回退解析真实模式。 */
    if ((int)ret != (int)TECC_CACHE_OK || item == NULL)
        item = tecc_resolve_dek_by_search_path(state, dek_name, dek_namespace);

    if (item != NULL)
    {
        tecc_cache_remove_pending_dek(state, dek_name, dek_namespace);
        insert_ret = tecc_cache_insert_dek_item(state, item);
        if (insert_ret != 0)
        {
            tecc_processor_debug(TECC_LOG_WARNING,
                "tecc_refresh_dek_cache: insert failed ret=%d for %s.%s",
                insert_ret,
                item->key_namespace ? item->key_namespace : dek_namespace,
                dek_name);
        }
        tecc_processor_debug(TECC_LOG_DEBUG1,
            "tecc_refresh_dek_cache: loaded DEK %s.%s (oid=%u)",
            item->key_namespace ? item->key_namespace : dek_namespace,
            dek_name, item->dek_oid);
    }
    else
    {
        tecc_processor_debug(TECC_LOG_WARNING,
            "tecc_refresh_dek_cache: fetch failed ret=%d for %s.%s",
            ret, dek_namespace, dek_name);
    }
}

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

/* Begin - ReqID:TECC4.0 - DDL-only 通路（分布式内部连接） */
/*
 * 分布式内部连接上的 DDL 预处理：只做 ENCRYPTED WITH 子句合成。
 *
 * 为什么需要这条路：分布式插件从元组描述符反解析 worker 侧 DDL，而
 * ENCRYPTED WITH 只存在 hg_encrypted_columns、不在 pg_attribute 里，
 * 所以下发文本丢了这个子句；worker 重新解析时会报
 * `encrypted data type "..." requires ENCRYPTED WITH clause`。
 * 见 plan/2026-10-28-hgdde分布式适配可行性分析.md §0.3。
 *
 * 处理范围：只认 `CREATE TABLE ... (...)` 与 `ALTER TABLE ... ADD COLUMN ...`。
 * 其余语句一律原样透传（返回 NULL）。
 *
 * 失败关闭：取不到元数据就不补，worker 侧照旧报错 —— 不猜 DEK、不猜算法。
 */

/* 与后端 is_encrypted_data_type() 对应的九个密态类型名 */
static const char *const tecc_ddl_only_enc_type_names[] = {
	"int2_enc", "int4_enc", "int8_enc", "float4_enc", "float8_enc",
	"numeric_enc", "varchar_enc", "bpchar_enc", "text_enc"
};

/* 一条插入动作：在原 SQL 的 offset 处插入 text */
typedef struct TeccDdlClauseEdit
{
	int			offset;
	char	   *text;
} TeccDdlClauseEdit;

static bool tecc_ddl_only_is_encrypted_type(const char *type_name);
static int	tecc_ddl_only_skip_space(const char *sql, int pos);
static int	tecc_ddl_only_skip_ident(const char *sql, int pos);
static bool tecc_ddl_only_scan_type(const char *sql, int start, char *name_out,
									size_t name_size, int *end_out);
static char *tecc_ddl_only_rewrite_ddl(PGconn *conn, TeccState *state,
									   const char *query);
static char *tecc_ddl_only_rewrite_wrapped_call(PGconn *conn, TeccState *state,
												const char *query);
static char *tecc_ddl_only_strip_shard_suffix(const char *relname);
static void tecc_ddl_only_append_quoted(PQExpBuffer buf, const char *value);
static char *tecc_ddl_only_build_clause(const TeccEncryptedColumnMetadata *meta);
static bool tecc_ddl_only_switch_to_coordinator(TeccState *state);
static TeccEncryptedColumnMetadata *tecc_ddl_only_fetch_column(TeccState *state,
															   const char *dbname,
															   const char *schema,
															   const char *table,
															   const char *column);
static char *tecc_ddl_only_apply_edits(const char *query, List *edits);

/*
 * tecc_ddl_only_is_encrypted_type
 *		类型名（已去掉模式限定）是否密态类型。
 */
static bool
tecc_ddl_only_is_encrypted_type(const char *type_name)
{
	int			i;

	if (type_name == NULL)
		return false;

	for (i = 0; i < (int) lengthof(tecc_ddl_only_enc_type_names); i++)
	{
		if (pg_strcasecmp(type_name, tecc_ddl_only_enc_type_names[i]) == 0)
			return true;
	}

	return false;
}

static int
tecc_ddl_only_skip_space(const char *sql, int pos)
{
	while (sql[pos] == ' ' || sql[pos] == '\t' || sql[pos] == '\n' ||
		   sql[pos] == '\r' || sql[pos] == '\f')
		pos++;

	return pos;
}

/*
 * tecc_ddl_only_skip_ident
 *		跳过一个标识符（支持双引号形式与其中的 "" 转义），返回结束位置。
 */
static int
tecc_ddl_only_skip_ident(const char *sql, int pos)
{
	if (sql[pos] == '"')
	{
		pos++;
		while (sql[pos] != '\0')
		{
			if (sql[pos] == '"')
			{
				if (sql[pos + 1] == '"')
				{
					pos += 2;
					continue;
				}
				return pos + 1;
			}
			pos++;
		}
		return pos;
	}

	while (sql[pos] != '\0' &&
		   (isalnum((unsigned char) sql[pos]) || sql[pos] == '_' || sql[pos] == '$'))
		pos++;

	return pos;
}

/*
 * tecc_ddl_only_scan_type
 *		从类型的起始位置扫出（1）类型名最后一段（写入 name_out）与
 *		（2）类型规格结束的偏移（子句应插在这里）。
 *
 *		为什么不从解析树的 Value 节点取类型名：该前端解析器产出的 String 节点
 *		用的是标准 PG 节点标签（实测 111），而 `tecc_nodes.h` 把 `IsA(x, String)`
 *		重定义成 `FE_T_String`(228)，两者对不上；直接读 `val.str` 会在非 Value 节点
 *		上取到野指针。按位置扫文本没有这个问题，且与"插入点"用的是同一段扫描，
 *		天然一致。
 *
 *		覆盖：模式限定名（a.b）、typmod（varchar_enc(20)、numeric_enc(10,2)）、
 *		数组维度（int4_enc[]）、双引号标识符。typmod 内的括号与字符串会正确跳过。
 */
static bool
tecc_ddl_only_scan_type(const char *sql, int start, char *name_out,
						size_t name_size, int *end_out)
{
	int			pos;
	bool		have_name = false;

	if (start < 0 || name_out == NULL || name_size == 0)
		return false;

	name_out[0] = '\0';
	pos = tecc_ddl_only_skip_space(sql, start);

	/* 逐段读标识符（a.b.c），只保留最后一段 */
	while (sql[pos] != '\0')
	{
		size_t		k = 0;
		int			next;

		if (sql[pos] == '"')
		{
			pos++;
			while (sql[pos] != '\0')
			{
				if (sql[pos] == '"')
				{
					if (sql[pos + 1] == '"')
					{
						if (k + 1 < name_size)
							name_out[k++] = '"';
						pos += 2;
						continue;
					}
					pos++;
					break;
				}
				if (k + 1 < name_size)
					name_out[k++] = sql[pos];
				pos++;
			}
			name_out[k] = '\0';
			have_name = true;
		}
		else
		{
			while (sql[pos] != '\0' &&
				   (isalnum((unsigned char) sql[pos]) || sql[pos] == '_' ||
					sql[pos] == '$'))
			{
				if (k + 1 < name_size)
					name_out[k++] = sql[pos];
				pos++;
			}
			if (k == 0)
				break;			/* 不是标识符：类型段结束 */
			name_out[k] = '\0';
			have_name = true;
		}

		next = tecc_ddl_only_skip_space(sql, pos);
		if (sql[next] != '.')
			break;				/* 不是模式限定：pos 停在标识符末尾，不吃尾部空格 */
		pos = tecc_ddl_only_skip_space(sql, next + 1);
		/* 不在这里跳过：交给下一轮读取，并覆盖 name_out 为最后一段 */
	}

	if (!have_name)
		return false;

	/* typmod */
	{
		int			next = tecc_ddl_only_skip_space(sql, pos);

		if (sql[next] == '(')
		{
			int			depth = 0;

			pos = next;
			while (sql[pos] != '\0')
			{
				if (sql[pos] == '(')
					depth++;
				else if (sql[pos] == ')')
				{
					depth--;
					if (depth == 0)
					{
						pos++;
						break;
					}
				}
				else if (sql[pos] == '\'')
				{
					pos++;
					while (sql[pos] != '\0')
					{
						if (sql[pos] == '\'' && sql[pos + 1] == '\'')
						{
							pos += 2;
							continue;
						}
						if (sql[pos] == '\'')
							break;
						pos++;
					}
				}
				pos++;
			}
		}
	}

	/* 数组维度 */
	for (;;)
	{
		int			next = tecc_ddl_only_skip_space(sql, pos);

		if (sql[next] != '[')
			break;
		pos = next;
		while (sql[pos] != '\0' && sql[pos] != ']')
			pos++;
		if (sql[pos] == ']')
			pos++;
	}

	*end_out = pos;
	return true;
}

/*
 * tecc_ddl_only_strip_shard_suffix
 *		若表名形如 <前缀>_<十进制数字>，返回剥掉后缀的名字；否则返回 NULL。
 *
 *		注意：这里只做"像不像分片"的判断，是否真是分片由调用方用元数据自校验
 *		（剥出的名字必须能在元数据里查到）。
 */
static char *
tecc_ddl_only_strip_shard_suffix(const char *relname)
{
	const char *underscore;
	const char *p;
	char	   *result;

	if (relname == NULL)
		return NULL;

	underscore = strrchr(relname, '_');
	if (underscore == NULL || underscore == relname)
		return NULL;

	for (p = underscore + 1; *p != '\0'; p++)
	{
		if (*p < '0' || *p > '9')
			return NULL;
	}
	if (p == underscore + 1)
		return NULL;

	result = strdup(relname);
	if (result == NULL)
		return NULL;
	result[underscore - relname] = '\0';

	return result;
}

static void
tecc_ddl_only_append_quoted(PQExpBuffer buf, const char *value)
{
	const char *p;

	appendPQExpBufferChar(buf, '\'');
	for (p = value; *p != '\0'; p++)
	{
		if (*p == '\'')
			appendPQExpBufferChar(buf, '\'');
		appendPQExpBufferChar(buf, *p);
	}
	appendPQExpBufferChar(buf, '\'');
}

/*
 * tecc_ddl_only_build_clause
 *		把一列的加密元数据渲染成 ` ENCRYPTED WITH (...)`。
 *
 *		选项名与取值形态必须与后端 hg_get_encrypted_column_clause() /
 *		hg_get_encrypted_column_options() 一致：
 *		DEK / SUPPORT_ALGO / HE_ADD_DEK / HE_MUL_DEK；算法串用小写连字符形态。
 */
static char *
tecc_ddl_only_build_clause(const TeccEncryptedColumnMetadata *meta)
{
	PQExpBufferData buf;
	bool		first = true;
	int			i;
	static const int masks[] = {1, 2, 4, 8, 16};
	static const char *names[] = {"det", "ope", "search", "he-add", "he-mul"};

	if (meta == NULL)
		return NULL;

	initPQExpBuffer(&buf);
	appendPQExpBufferStr(&buf, " ENCRYPTED WITH (");

	if (meta->dek_name != NULL && meta->dek_name[0] != '\0')
	{
		appendPQExpBufferStr(&buf, "DEK = ");
		tecc_ddl_only_append_quoted(&buf, meta->dek_name);
		first = false;
	}

	if (meta->encryption_type != 0)
	{
		bool		first_algo = true;

		if (!first)
			appendPQExpBufferStr(&buf, ", ");
		appendPQExpBufferStr(&buf, "SUPPORT_ALGO = '");
		for (i = 0; i < (int) lengthof(masks); i++)
		{
			if ((meta->encryption_type & masks[i]) == 0)
				continue;
			if (!first_algo)
				appendPQExpBufferChar(&buf, ',');
			appendPQExpBufferStr(&buf, names[i]);
			first_algo = false;
		}
		appendPQExpBufferChar(&buf, '\'');
		first = false;
	}

	if (meta->he_add_dek != NULL && meta->he_add_dek[0] != '\0')
	{
		if (!first)
			appendPQExpBufferStr(&buf, ", ");
		appendPQExpBufferStr(&buf, "HE_ADD_DEK = ");
		tecc_ddl_only_append_quoted(&buf, meta->he_add_dek);
		first = false;
	}

	if (meta->he_mul_dek != NULL && meta->he_mul_dek[0] != '\0')
	{
		if (!first)
			appendPQExpBufferStr(&buf, ", ");
		appendPQExpBufferStr(&buf, "HE_MUL_DEK = ");
		tecc_ddl_only_append_quoted(&buf, meta->he_mul_dek);
		first = false;
	}

	appendPQExpBufferChar(&buf, ')');

	if (PQExpBufferDataBroken(buf))
	{
		termPQExpBuffer(&buf);
		return NULL;
	}

	return buf.data;
}

/*
 * tecc_ddl_only_switch_to_coordinator
 *		把取数目标切到协调节点（逻辑表层必需：worker 上此时还没有该表的声明）。
 *
 *		协调节点定位：在本连接上查 pg_dist_node 里 groupid = 0 的那一行。
 *		只在第一次需要时解析并缓存到 state->fetch_conninfo_override。
 *
 *		返回 false 表示解析失败 —— 调用方据此放弃补子句（失败关闭）。
 */
static bool
tecc_ddl_only_switch_to_coordinator(TeccState *state)
{
	TeccCursorInterface *cursor;
	char	   *host = NULL;
	char	   *port = NULL;
	PQconninfoOption *options;
	PQExpBufferData buf;
	PQconninfoOption *opt;

	if (state == NULL)
		return false;

	if (state->fetch_conninfo_override != NULL)
		return true;

	cursor = tecc_fetcher_execute_query(state,
										"SELECT nodename, nodeport::text "
										"FROM pg_catalog.pg_dist_node "
										"WHERE groupid = 0 AND isactive "
										"ORDER BY nodeid LIMIT 1");
	if (cursor == NULL)
	{
		TECC_LOG(TECC_LOG_DEBUG1,
				 "DDL-only: cannot query pg_dist_node for coordinator (%s)",
				 tecc_get_last_error(state->conn) ?
				 tecc_get_last_error(state->conn) : "no detail");
		return false;
	}

	if (cursor->next(cursor) != TECC_FETCHER_OK)
	{
		TECC_LOG(TECC_LOG_DEBUG1,
				 "DDL-only: pg_dist_node returned no coordinator row");
		cursor->destroy(cursor);
		return false;
	}

	{
		const char *h = cursor->get_column(cursor, 0);
		const char *p = cursor->get_column(cursor, 1);

		if (h != NULL)
			host = strdup(h);
		if (p != NULL)
			port = strdup(p);
	}
	cursor->destroy(cursor);

	if (host == NULL || port == NULL)
	{
		TECC_LOG(TECC_LOG_DEBUG1,
				 "DDL-only: pg_dist_node row has no usable nodename/nodeport");
		free(host);
		free(port);
		return false;
	}

	options = PQconninfoParse(state->conninfo, NULL);
	if (options == NULL)
	{
		TECC_LOG(TECC_LOG_DEBUG1,
				 "DDL-only: PQconninfoParse failed on internal conninfo");
		free(host);
		free(port);
		return false;
	}

	initPQExpBuffer(&buf);
	for (opt = options; opt->keyword != NULL; opt++)
	{
		const char *p;

		if (opt->val == NULL || opt->val[0] == '\0')
			continue;
		if (strcmp(opt->keyword, "host") == 0 ||
			strcmp(opt->keyword, "hostaddr") == 0 ||
			strcmp(opt->keyword, "port") == 0 ||
			strcmp(opt->keyword, "enable_tecc") == 0)
			continue;

		appendPQExpBuffer(&buf, "%s='", opt->keyword);
		for (p = opt->val; *p != '\0'; p++)
		{
			if (*p == '\\' || *p == '\'')
				appendPQExpBufferChar(&buf, '\\');
			appendPQExpBufferChar(&buf, *p);
		}
		appendPQExpBufferStr(&buf, "' ");
	}
	PQconninfoFree(options);

	appendPQExpBuffer(&buf, "host='%s' port='%s' enable_tecc=false", host, port);
	free(host);
	free(port);

	if (PQExpBufferDataBroken(buf))
	{
		termPQExpBuffer(&buf);
		return false;
	}

	state->fetch_conninfo_override = buf.data;

	TECC_LOG(TECC_LOG_DEBUG1, "DDL-only: fetch target -> coordinator (%s)",
			 state->fetch_conninfo_override);

	return true;
}

/*
 * tecc_ddl_only_fetch_column
 *		取一列的加密元数据。三级退让，任何一级取到即用：
 *		  1) 按语句里的表名直接取（分片 DDL 与逻辑表 DDL 都能命中时最省）；
 *		  2) 表名像分片则剥掉 `_<数字>` 再取（自校验：剥出的名字必须能查到）；
 *		  3) 仍取不到 → 把取数目标切到协调节点再按 1)/2) 取一次（逻辑表层）。
 *
 *		全部失败返回 NULL（调用方不补子句 → 失败关闭）。
 */
static TeccEncryptedColumnMetadata *
tecc_ddl_only_fetch_column(TeccState *state, const char *dbname,
						   const char *schema, const char *table,
						   const char *column)
{
	TeccEncryptedColumnMetadata *meta = NULL;
	char	   *logical;
	char		resolved_schema[256];
	char	   *resolved = NULL;

	/*
	 * 不带模式名（ALTER TABLE 实测就是这种形态：`ALTER TABLE probe_v3 ADD
	 * COLUMN c pg_catalog.int4_enc`）：按 "$user", public 解析真实 schema。
	 * 解析不出来就不补子句（失败关闭），绝不猜一个模式名去补。
	 */
	if (schema == NULL || schema[0] == '\0')
	{
		const char *candidates[2];
		const char *user = PQuser(state->conn);
		int			ncandidates = 0;

		if (user != NULL && user[0] != '\0')
			candidates[ncandidates++] = user;
		candidates[ncandidates++] = "public";

		if (tecc_fetch_resolve_table_schema(state, dbname, table,
											candidates, ncandidates,
											&resolved) != TECC_FETCHER_OK ||
			resolved == NULL)
		{
			TECC_LOG(TECC_LOG_DEBUG1,
					 "DDL-only: cannot resolve schema for unqualified table %s",
					 table);
			return NULL;
		}

		snprintf(resolved_schema, sizeof(resolved_schema), "%s", resolved);
		pg_free(resolved);
		schema = resolved_schema;
		TECC_LOG(TECC_LOG_DEBUG1, "DDL-only: resolved unqualified table %s -> schema '%s'",
				 table, schema);
	}

	TECC_LOG(TECC_LOG_DEBUG1, "DDL-only: fetch %s.%s.%s (override=%s)",
			 schema, table, column,
			 state->fetch_conninfo_override ? "coordinator" : "connected-node");

	if (tecc_fetch_encrypted_column_metadata_by_name(state, dbname, schema,
													 table, column, &meta) == TECC_FETCHER_OK &&
		meta != NULL)
		return meta;

	logical = tecc_ddl_only_strip_shard_suffix(table);
	if (logical != NULL)
	{
		if (tecc_fetch_encrypted_column_metadata_by_name(state, dbname, schema,
														 logical, column, &meta) == TECC_FETCHER_OK &&
			meta != NULL)
		{
			free(logical);
			return meta;
		}
	}

	if (!tecc_ddl_only_switch_to_coordinator(state))
	{
		TECC_LOG(TECC_LOG_DEBUG1, "DDL-only: could not switch to coordinator for %s.%s.%s",
				 schema, table, column);
		free(logical);
		return NULL;
	}

	if (tecc_fetch_encrypted_column_metadata_by_name(state, dbname, schema,
													 table, column, &meta) == TECC_FETCHER_OK &&
		meta != NULL)
	{
		TECC_LOG(TECC_LOG_DEBUG1, "DDL-only: coordinator has metadata for %s.%s.%s",
				 schema, table, column);
		free(logical);
		return meta;
	}
	TECC_LOG(TECC_LOG_DEBUG1, "DDL-only: coordinator has NO metadata for %s.%s.%s",
			 schema, table, column);

	if (logical != NULL)
	{
		if (tecc_fetch_encrypted_column_metadata_by_name(state, dbname, schema,
														 logical, column, &meta) == TECC_FETCHER_OK &&
			meta != NULL)
		{
			free(logical);
			return meta;
		}
	}

	free(logical);
	return NULL;
}

/*
 * tecc_ddl_only_apply_edits
 *		按偏移升序把插入文本拼进原 SQL。
 */
static char *
tecc_ddl_only_apply_edits(const char *query, List *edits)
{
	PQExpBufferData buf;
	ListCell   *lc;
	int			pos = 0;

	initPQExpBuffer(&buf);

	foreach(lc, edits)
	{
		TeccDdlClauseEdit *edit = (TeccDdlClauseEdit *) lfirst(lc);
		int			len = edit->offset - pos;

		if (len < 0)
			len = 0;
		appendBinaryPQExpBuffer(&buf, query + pos, len);
		appendPQExpBufferStr(&buf, edit->text);
		pos = edit->offset;
	}
	appendPQExpBufferStr(&buf, query + pos);

	if (PQExpBufferDataBroken(buf))
	{
		termPQExpBuffer(&buf);
		return NULL;
	}

	return buf.data;
}

/*
 * tecc_ddl_only_word_at
 *		判断 pos 处是否是一个独立关键字（大小写不敏感，前后非标识符字符）。
 */
static bool
tecc_ddl_only_word_at(const char *q, int pos, const char *word)
{
	size_t		len = strlen(word);

	if (pg_strncasecmp(q + pos, word, len) != 0)
		return false;
	if (isalnum((unsigned char) q[pos + len]) || q[pos + len] == '_')
		return false;

	return true;
}

/*
 * tecc_ddl_only_read_ident
 *		读一个标识符（支持双引号与 "" 转义）到 out，返回结束位置（失败返回 -1）。
 */
static int
tecc_ddl_only_read_ident(const char *q, int pos, char *out, size_t out_size)
{
	size_t		k = 0;

	if (out_size > 0)
		out[0] = '\0';

	if (q[pos] == '"')
	{
		pos++;
		while (q[pos] != '\0')
		{
			if (q[pos] == '"')
			{
				if (q[pos + 1] == '"')
				{
					if (k + 1 < out_size)
						out[k++] = '"';
					pos += 2;
					continue;
				}
				pos++;
				break;
			}
			if (k + 1 < out_size)
				out[k++] = q[pos];
			pos++;
		}
	}
	else
	{
		while (q[pos] != '\0' &&
			   (isalnum((unsigned char) q[pos]) || q[pos] == '_' || q[pos] == '$'))
		{
			if (k + 1 < out_size)
				out[k++] = q[pos];
			pos++;
		}
	}

	if (k == 0)
		return -1;

	out[k] = '\0';
	return pos;
}

/*
 * tecc_ddl_only_skip_qualified_name
 *		读 <name> / <schema>.<name> / <db>.<schema>.<name>，把最后两段分别写入
 *		schema_out / table_out（只有一段时 schema_out 为空）。返回结束位置。
 */
static int
tecc_ddl_only_skip_qualified_name(const char *q, int pos,
								  char *schema_out, size_t schema_size,
								  char *table_out, size_t table_size)
{
	char		seg[256];
	int			nseg = 0;
	int			end = -1;

	for (;;)
	{
		int			start = tecc_ddl_only_skip_space(q, pos);

		end = tecc_ddl_only_read_ident(q, start, seg, sizeof(seg));
		if (end < 0)
			break;

		nseg++;
		if (nseg == 1)
		{
			snprintf(table_out, table_size, "%s", seg);
			if (schema_size > 0)
				schema_out[0] = '\0';
		}
		else
		{
			snprintf(schema_out, schema_size, "%s", table_out);
			snprintf(table_out, table_size, "%s", seg);
		}

		{
			int			next = tecc_ddl_only_skip_space(q, end);

			if (q[next] == '.')
			{
				pos = next + 1;
				continue;
			}
			end = next;
			break;
		}
	}

	return (nseg > 0) ? end : -1;
}

/*
 * tecc_ddl_only_find_matching_paren
 *		open 指向 '('，返回与之配对的 ')' 的下标；不配对返回 -1。
 *		会正确跳过字符串字面量与双引号标识符。
 */
static int
tecc_ddl_only_find_matching_paren(const char *q, int open)
{
	int			depth = 0;
	int			pos;

	for (pos = open; q[pos] != '\0'; pos++)
	{
		if (q[pos] == '\'')
		{
			pos++;
			while (q[pos] != '\0')
			{
				if (q[pos] == '\'' && q[pos + 1] == '\'')
				{
					pos += 2;
					continue;
				}
				if (q[pos] == '\'')
					break;
				pos++;
			}
			continue;
		}
		if (q[pos] == '"')
		{
			pos++;
			while (q[pos] != '\0')
			{
				if (q[pos] == '"' && q[pos + 1] == '"')
				{
					pos += 2;
					continue;
				}
				if (q[pos] == '"')
					break;
				pos++;
			}
			continue;
		}
		if (q[pos] == '(')
			depth++;
		else if (q[pos] == ')')
		{
			depth--;
			if (depth == 0)
				return pos;
		}
	}

	return -1;
}

/*
 * tecc_ddl_only_is_table_constraint
 *		该列定义项是否是表级约束（不是列，跳过）。
 */
static bool
tecc_ddl_only_is_table_constraint(const char *q, int pos)
{
	static const char *const keywords[] = {
		"CONSTRAINT", "PRIMARY", "UNIQUE", "CHECK", "FOREIGN", "EXCLUDE", "LIKE"
	};
	int			i;

	pos = tecc_ddl_only_skip_space(q, pos);
	for (i = 0; i < (int) lengthof(keywords); i++)
	{
		if (tecc_ddl_only_word_at(q, pos, keywords[i]))
			return true;
	}

	return false;
}

/*
 * tecc_ddl_only_add_column_edit
 *		检查一列（名字 + 类型起点），必要时取元数据并登记一条插入动作。
 */
static void
tecc_ddl_only_add_column_edit(PGconn *conn, TeccState *state,
							  const char *query, const char *schema,
							  const char *table, const char *column,
							  int type_start, List **edits)
{
	char		type_name[64];
	int			offset;
	TeccEncryptedColumnMetadata *meta;
	char	   *clause;
	TeccDdlClauseEdit *edit;

	if (!tecc_ddl_only_scan_type(query, type_start, type_name,
								 sizeof(type_name), &offset))
	{
		TECC_LOG(TECC_LOG_DEBUG1, "DDL-only: column=%s scan_type failed (start=%d)",
				 column, type_start);
		return;
	}
	TECC_LOG(TECC_LOG_DEBUG1, "DDL-only: column=%s type='%s' end=%d",
			 column, type_name, offset);
	if (!tecc_ddl_only_is_encrypted_type(type_name))
		return;

	/* 已经带子句就以插件给的为准（在类型之后找 ENCRYPTED） */
	{
		const char *p;

		for (p = query + offset; *p != '\0' && *p != ',' && *p != ')'; p++)
		{
			if (pg_strncasecmp(p, "ENCRYPTED", 9) == 0)
				return;
		}
	}

	/*
	 * 优先问同进程的 PG 核心：它读本进程目录，能看到本事务未提交的行、
	 * 也不依赖目标节点的 pg_dist_node。取不到再退回取数器（另开会话）。
	 */
	clause = NULL;
#ifdef __GNUC__
	if (schema != NULL && hg_get_encrypted_clause_by_name != NULL)
		clause = hg_get_encrypted_clause_by_name(schema, table, column);
#endif

	if (clause == NULL)
	{
		meta = tecc_ddl_only_fetch_column(state, PQdb(conn), schema, table, column);
		if (meta == NULL)
		{
			TECC_LOG(TECC_LOG_DEBUG1,
					 "DDL-only: no metadata for %s.%s.%s, clause NOT synthesized",
					 schema, table, column);
			return;				/* 失败关闭：不猜 */
		}

		clause = tecc_ddl_only_build_clause(meta);
	}

	if (clause == NULL)
		return;

	edit = (TeccDdlClauseEdit *) malloc(sizeof(TeccDdlClauseEdit));
	if (edit == NULL)
		return;
	edit->offset = offset;
	edit->text = clause;
	*edits = lappend(*edits, edit);

	TECC_LOG(TECC_LOG_DEBUG1, "DDL-only: %s.%s.%s (%s) ->%s",
			 schema, table, column, type_name, clause);
}

/*
 * tecc_ddl_only_preprocess_one
 *		处理**单条**语句的 DDL-only 通路。返回改写后的 SQL（调用方负责 free），
 *		无需改写返回 NULL。
 *
 *		分布式插件下发的形态有三种，都要覆盖：
 *		  1. 独立语句：`CREATE TABLE ...`；
 *		  2. 包在函数调用里的内嵌文本（Citus 12 实测形态）：
 *		     `SELECT worker_apply_shard_ddl_command (<shardid>, '<DDL 文本>')`
 *		     内层写的是**逻辑表名**，worker 侧由 RelayEventExtendNames 扩成
 *		     `<逻辑表>_<shardid>` 再执行；
 *		  3. 多语句串（加节点时的元数据同步）—— 由上层切分后逐条调用本函数。
 */
static char *
tecc_ddl_only_preprocess_one(PGconn *conn, TeccState *state, const char *query)
{
	const char *p = query;

	while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')
		p++;

	if (pg_strncasecmp(p, "CREATE", 6) == 0 || pg_strncasecmp(p, "ALTER", 5) == 0)
		return tecc_ddl_only_rewrite_ddl(conn, state, query);

	if (pg_strncasecmp(p, "SELECT", 6) == 0)
		return tecc_ddl_only_rewrite_wrapped_call(conn, state, query);

	return NULL;
}

/*
 * tecc_ddl_only_preprocess_query
 *		DDL-only 通路入口：把查询串按**顶层分号**切成语句，逐条尝试改写，有改动
 *		才拼回；一条都没改则返回 NULL（原样发出）。
 *
 *		为什么要切分：加节点时插件的元数据同步把多条语句塞进一个查询串，
 *		实测形态是
 *		    `SELECT worker_drop_sequence_dependency(...);;DROP TABLE IF EXISTS ...;
 *		     CREATE TABLE public.t (... int4_enc ...) USING heap;ALTER TABLE ...;
 *		     SELECT worker_create_truncate_trigger(...)`
 *		不切分就只会看到第一条（SELECT），内嵌的 CREATE TABLE 永远补不上子句，
 *		新节点上建壳表直接失败（= 加节点失败）。
 *
 *		切分只需正确跳过单引号字符串（含 '' 转义）、双引号标识符（含 "" 转义）
 *		与美元引用（$$...$$ / $tag$...$tag$）：分号不会出现在这些结构之外。
 */
static char *
tecc_ddl_only_preprocess_query(PGconn *conn, TeccState *state, const char *query)
{
	const char *stmt_start;
	const char *p;
	PQExpBufferData buf;
	bool		changed = false;

	if (state == NULL || query == NULL)
		return NULL;

	/* 取数器没起来就没法判断"表里有没有密态列"，直接原样透传 */
	if (!state->data_fetcher_initialized)
		return NULL;

	initPQExpBuffer(&buf);
	stmt_start = query;

	for (p = query; ; p++)
	{
		if (*p == '\'')
		{
			p++;
			while (*p != '\0')
			{
				if (*p == '\'' && p[1] == '\'')
				{
					p += 2;
					continue;
				}
				if (*p == '\'')
					break;
				p++;
			}
			if (*p == '\0')
				break;
			continue;
		}

		if (*p == '"')
		{
			p++;
			while (*p != '\0')
			{
				if (*p == '"' && p[1] == '"')
				{
					p += 2;
					continue;
				}
				if (*p == '"')
					break;
				p++;
			}
			if (*p == '\0')
				break;
			continue;
		}

		/* 美元引用：$tag$ ... $tag$ */
		if (*p == '$' && (p[1] == '$' || isalpha((unsigned char) p[1]) ||
						  p[1] == '_'))
		{
			const char *tag_end = p + 1;
			const char *close_tag;
			size_t		tag_len;

			while (*tag_end != '\0' && *tag_end != '$' &&
				   (isalnum((unsigned char) *tag_end) || *tag_end == '_'))
				tag_end++;
			if (*tag_end != '$')
				continue;		/* 不是美元引用开头，当普通字符 */

			tag_len = (size_t) (tag_end + 1 - p);
			close_tag = p + tag_len;
			while (*close_tag != '\0' &&
				   strncmp(close_tag, p, tag_len) != 0)
				close_tag++;

			if (*close_tag == '\0')
				break;			/* 未闭合：交给下面整体处理 */
			p = close_tag + tag_len - 1;
			continue;
		}

		if (*p != ';' && *p != '\0')
			continue;

		/* 一条语句的边界：stmt_start .. p */
		{
			size_t		len = (size_t) (p - stmt_start);
			char	   *one = malloc(len + 1);
			char	   *rewritten = NULL;

			if (one == NULL)
			{
				termPQExpBuffer(&buf);
				return NULL;
			}
			memcpy(one, stmt_start, len);
			one[len] = '\0';

			rewritten = tecc_ddl_only_preprocess_one(conn, state, one);
			appendPQExpBufferStr(&buf, rewritten != NULL ? rewritten : one);
			if (rewritten != NULL)
			{
				changed = true;
				free(rewritten);
			}
			free(one);
		}

		if (*p == '\0')
			break;

		/* 保留原分号与空语句 */
		appendPQExpBufferChar(&buf, ';');
		stmt_start = p + 1;
	}

	if (!changed || PQExpBufferDataBroken(buf))
	{
		termPQExpBuffer(&buf);
		return NULL;
	}

	/* 调用方 free()；先摘下数据再释放缓冲区结构本身 */
	{
		char	   *result = buf.data;

		buf.data = NULL;
		termPQExpBuffer(&buf);
		return result;
	}
}

/*
 * tecc_ddl_only_rewrite_wrapped_call
 *		处理 `SELECT worker_apply_shard_ddl_command (<shardid>[, '<schema>'], '<DDL>')`
 *		这种"DDL 内嵌在函数参数里"的形态。
 *
 *		Citus 12 实测就只发这一种形态：内层写的是**逻辑表名**，worker 侧由
 *		RelayEventExtendNames 扩成 `<逻辑表>_<shardid>` 再执行。所以元数据要按
 *		内层的（模式名, 表名）去取 —— 通常只有协调节点上有（见 §0.4.3 的三级退让）。
 *
 *		做法：取出最内层那个"以 CREATE/ALTER 开头"的字符串字面量（解 '' 转义）→
 *		交给 tecc_ddl_only_rewrite_ddl() 改写 → 重新按 SQL 字面量转义拼回原语句。
 *		其余部分（shardid、外层 SELECT、函数名、USING heap 等）一字不动。
 */
static char *
tecc_ddl_only_rewrite_wrapped_call(PGconn *conn, TeccState *state, const char *query)
{
	static const char funcname[] = "worker_apply_shard_ddl_command";
	const char *func;
	const char *p;
	const char *inner_start = NULL;
	const char *inner_end = NULL;
	int			depth = 0;
	char	   *inner;
	char	   *rewritten;
	PQExpBufferData buf;
	size_t		len;
	char	   *dst;
	const char *s;

	/* 定位函数名（大小写不敏感） */
	for (func = query; *func != '\0'; func++)
	{
		if (pg_strncasecmp(func, funcname, sizeof(funcname) - 1) == 0)
			break;
	}
	if (*func == '\0')
		return NULL;

	/* 在参数列表（depth == 1）里找字符串字面量，记住最后一个像 DDL 的 */
	for (p = func; *p != '\0'; p++)
	{
		const char *content;
		const char *q;

		if (*p == '(')
		{
			depth++;
			continue;
		}
		if (*p == ')')
		{
			depth--;
			if (depth == 0)
				break;
			continue;
		}
		if (*p != '\'' || depth != 1)
			continue;

		content = p + 1;
		q = content;
		while (*q != '\0')
		{
			if (*q == '\'')
			{
				if (q[1] == '\'')
				{
					q += 2;
					continue;
				}
				break;
			}
			q++;
		}
		if (*q == '\0')
			return NULL;		/* 字面量没闭合：不动 */

		{
			const char *head = content;

			while (head < q && (*head == ' ' || *head == '\t' ||
								*head == '\n' || *head == '\r'))
				head++;
			if (((q - head) >= 6 && pg_strncasecmp(head, "CREATE", 6) == 0) ||
				((q - head) >= 5 && pg_strncasecmp(head, "ALTER", 5) == 0))
			{
				inner_start = content;
				inner_end = q;
			}
		}
		p = q;
	}

	if (inner_start == NULL || inner_end == NULL)
		return NULL;

	/* 去掉 SQL 字面量的 '' 转义，得到内层 DDL 原文 */
	len = (size_t) (inner_end - inner_start);
	inner = malloc(len + 1);
	if (inner == NULL)
		return NULL;
	dst = inner;
	for (s = inner_start; s < inner_end; s++)
	{
		if (*s == '\'' && (s + 1) < inner_end && s[1] == '\'')
		{
			*dst++ = '\'';
			s++;
			continue;
		}
		*dst++ = *s;
	}
	*dst = '\0';

	rewritten = tecc_ddl_only_rewrite_ddl(conn, state, inner);
	free(inner);
	if (rewritten == NULL)
		return NULL;

	/* 拼回：原前缀（已含开引号）+ 改写后的 DDL（重新转义）+ 原后缀 */
	initPQExpBuffer(&buf);
	appendBinaryPQExpBuffer(&buf, query, (int) (inner_start - query));
	for (s = rewritten; *s != '\0'; s++)
	{
		if (*s == '\'')
			appendPQExpBufferChar(&buf, '\'');
		appendPQExpBufferChar(&buf, *s);
	}
	appendPQExpBufferChar(&buf, '\'');
	appendPQExpBufferStr(&buf, inner_end + 1);

	free(rewritten);

	if (PQExpBufferDataBroken(buf))
	{
		termPQExpBuffer(&buf);
		return NULL;
	}

	return buf.data;
}

/*
 * tecc_ddl_only_rewrite_ddl
 *		独立 DDL 语句的改写：`CREATE TABLE ... (...)` 与
 *		`ALTER TABLE ... ADD [COLUMN] ...`。
 *
 *		完全不依赖前端解析器：实测该解析器认不了 `CREATE TABLE ... USING heap`
 *		（PG12+ 语法），而分布式插件下发的正是这种文本。改为按文本结构扫描
 *		——反而更抗插件输出差异，且"取类型名"与"定位插入点"共用同一段扫描。
 *
 *		ALTER 分支成立的前提是"同进程目录查询"（见 hg_get_encrypted_clause_by_name）：
 *		该列的声明是在同一个事务里刚写的，另开会话的取数器看不到。
 */
static char *
tecc_ddl_only_rewrite_ddl(PGconn *conn, TeccState *state, const char *query)
{
	int			pos;
	char		schema[256];
	char		table[256];
	List	   *edits = NIL;

	if (state == NULL || query == NULL)
		return NULL;

	TECC_LOG(TECC_LOG_DEBUG1, "DDL-only: rewrite_ddl enter %.48s", query);

	pos = tecc_ddl_only_skip_space(query, 0);

	if (tecc_ddl_only_word_at(query, pos, "CREATE"))
	{
		int			body_open;
		int			body_close;

		pos += 6;

		/* CREATE [GLOBAL|LOCAL] [TEMP|TEMPORARY|UNLOGGED] TABLE */
		for (;;)
		{
			int			next = tecc_ddl_only_skip_space(query, pos);

			if (tecc_ddl_only_word_at(query, next, "GLOBAL"))
				pos = next + 6;
			else if (tecc_ddl_only_word_at(query, next, "LOCAL"))
				pos = next + 5;
			else if (tecc_ddl_only_word_at(query, next, "TEMPORARY"))
				pos = next + 9;
			else if (tecc_ddl_only_word_at(query, next, "TEMP"))
				pos = next + 4;
			else if (tecc_ddl_only_word_at(query, next, "UNLOGGED"))
				pos = next + 8;
			else
				break;
		}

		pos = tecc_ddl_only_skip_space(query, pos);
		if (!tecc_ddl_only_word_at(query, pos, "TABLE"))
			return NULL;
		pos += 5;

		/* IF NOT EXISTS */
		{
			int			next = tecc_ddl_only_skip_space(query, pos);

			if (tecc_ddl_only_word_at(query, next, "IF"))
			{
				next = tecc_ddl_only_skip_space(query, next + 2);
				if (tecc_ddl_only_word_at(query, next, "NOT"))
				{
					next = tecc_ddl_only_skip_space(query, next + 3);
					if (tecc_ddl_only_word_at(query, next, "EXISTS"))
						pos = next + 6;
				}
			}
		}

		pos = tecc_ddl_only_skip_qualified_name(query, pos, schema, sizeof(schema),
												table, sizeof(table));
		if (pos < 0)
			return NULL;

		body_open = tecc_ddl_only_skip_space(query, pos);
		if (query[body_open] != '(')
			return NULL;
		body_close = tecc_ddl_only_find_matching_paren(query, body_open);
		if (body_close < 0)
			return NULL;

		/* 逐个顶层逗号切分列定义项 */
		{
			int			item_start = body_open + 1;
			int			p;
			int			depth = 0;

			for (p = body_open + 1; p <= body_close; p++)
			{
				if (query[p] == '(')
					depth++;
				else if (query[p] == ')')
					depth--;
				else if (query[p] == '\'' || query[p] == '"')
				{
					char		quote = query[p];

					p++;
					while (query[p] != '\0')
					{
						if (query[p] == quote)
						{
							if (query[p + 1] == quote)
							{
								p += 2;
								continue;
							}
							break;
						}
						p++;
					}
					continue;
				}

				if ((query[p] == ',' && depth == 0) || p == body_close)
				{
					int			col_start = tecc_ddl_only_skip_space(query, item_start);
					char		column[256];
					int			after_ident;

					item_start = p + 1;

					if (p == col_start)
						continue;
					if (tecc_ddl_only_is_table_constraint(query, col_start))
						continue;

					after_ident = tecc_ddl_only_read_ident(query, col_start,
														   column, sizeof(column));
					if (after_ident < 0)
						continue;

					tecc_ddl_only_add_column_edit(conn, state, query, schema, table,
												  column, after_ident, &edits);
				}
			}
		}
	}
	else if (tecc_ddl_only_word_at(query, pos, "ALTER"))
	{
		pos += 5;
		pos = tecc_ddl_only_skip_space(query, pos);
		if (!tecc_ddl_only_word_at(query, pos, "TABLE"))
			return NULL;
		pos += 5;

		/* [IF EXISTS] [ONLY] <name> */
		{
			int			next = tecc_ddl_only_skip_space(query, pos);

			if (tecc_ddl_only_word_at(query, next, "IF"))
			{
				next = tecc_ddl_only_skip_space(query, next + 2);
				if (tecc_ddl_only_word_at(query, next, "EXISTS"))
					pos = next + 6;
			}
			next = tecc_ddl_only_skip_space(query, pos);
			if (tecc_ddl_only_word_at(query, next, "ONLY"))
				pos = next + 4;
		}

		pos = tecc_ddl_only_skip_qualified_name(query, pos, schema, sizeof(schema),
												table, sizeof(table));
		if (pos < 0)
			return NULL;

		/*
		 * 找 ADD [COLUMN] [IF NOT EXISTS] <col> <type>。
		 *
		 * 这条路径能成立的关键是"同进程目录查询"：ALTER 的声明是在**同一个事务里**
		 * 刚写进 hg_encrypted_columns 的，另开会话的取数器看不到未提交行，而
		 * hg_get_encrypted_clause_by_name() 读的是本进程目录，看得到。
		 */
		for (;;)
		{
			int			next = tecc_ddl_only_skip_space(query, pos);

			if (query[next] == '\0')
				break;

			if (tecc_ddl_only_word_at(query, next, "ADD"))
			{
				int			col_pos = tecc_ddl_only_skip_space(query, next + 3);
				char		column[256];
				int			after_ident;

				if (tecc_ddl_only_word_at(query, col_pos, "COLUMN"))
					col_pos = tecc_ddl_only_skip_space(query, col_pos + 6);
				if (tecc_ddl_only_word_at(query, col_pos, "IF"))
				{
					col_pos = tecc_ddl_only_skip_space(query, col_pos + 2);
					if (tecc_ddl_only_word_at(query, col_pos, "NOT"))
						col_pos = tecc_ddl_only_skip_space(query, col_pos + 3);
					if (tecc_ddl_only_word_at(query, col_pos, "EXISTS"))
						col_pos = tecc_ddl_only_skip_space(query, col_pos + 6);
				}

				after_ident = tecc_ddl_only_read_ident(query, col_pos, column,
													   sizeof(column));
				if (after_ident >= 0)
					tecc_ddl_only_add_column_edit(conn, state, query, schema, table,
												  column, after_ident, &edits);

				pos = next + 3;
				continue;
			}

			pos = next + 1;
		}
	}
	else
		return NULL;

	if (edits == NIL)
		return NULL;

	return tecc_ddl_only_apply_edits(query, edits);
}
/* End - ReqID:TECC4.0 - DDL-only 通路 */

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

	/*
	 * ReqID:TECC4.0 - DDL-only（分布式内部连接）走独立通路：只合成
	 * ENCRYPTED WITH 子句，不做常量加密 / 结果解密 / 密钥 DDL 处理。
	 */
	if (conn->tecc_ddl_only)
	{
		TECC_LOG(TECC_LOG_DEBUG1,
				 "preprocess_query: ddl_only=1 has_state=%d", state != NULL);
	}

	if (state != NULL && state->ddl_only)
		return tecc_ddl_only_preprocess_query(conn, state, query);

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

		/* ReqID:TECC4.0 - 重置表达式结果列元数据（每次新查询清空，
		 * 由 tecc_stmt_processor_process_select 重新填充）。
		 * 注意：改写后的 SQL 重发（disable_once 置位，TECC 不重新处理）
		 * 时必须保留原查询收集的 pending_expr_col_*——否则表达式结果列
		 * （如 enc + k / enc * k 标量运算）在结果解密时无法恢复列元数据，
		 * 会走错误的 SM4-DEK 路径并产生 "Failed to decrypt DEK" 误报。 */
		if (!state->disable_once)
		{
			int			expr_idx;

			state->pending_expr_col_count = 0;
			state->pending_decrypt_raw_call = false;
			memset(state->pending_expr_col_raw_hex, 0,
				   sizeof(state->pending_expr_col_raw_hex));
			for (expr_idx = 0; expr_idx < TECC_MAX_EXPR_RESULT_COLS; expr_idx++)
				state->pending_expr_col_field_indexes[expr_idx] = -1;
			memset(state->pending_expr_col_raw_algorithms, 0,
				   sizeof(state->pending_expr_col_raw_algorithms));
		}

		/* ReqID:TECC4.0 - tecc_decrypt_raw() is a plaintext diagnostic helper.
		 * When session decryption is disabled, fail before sending the query
		 * to the server so raw ciphertext payloads are not exposed. */
		if (!state->guc_params.tecc_mode &&
			tecc_query_calls_decrypt_raw(query))
		{
			tecc_record_error(state, TECC_ERR_PROCESSOR,
							  "tecc_decrypt_raw requires enable hg_tecc_mode",
							  __func__, __LINE__, false);
			return NULL;
		}

		/* ReqID:TECC4.0 - Gap D: fail-closed reject tecc_decrypt_raw in PREPARE ... AS.
		 * pending_expr_col_raw_algorithms is per-statement: PREPARE registers it,
		 * but EXECUTE's per-statement memset clears it, so EXECUTE silently degrades
		 * to full-value decryption (wrong data, no error).  Detect at PREPARE time
		 * by scanning the raw query text for tecc_decrypt_raw, regardless of tecc_mode.
		 * Aligned with JDBC behaviour (SQLSTATE 0A000). */
		if (tecc_query_calls_decrypt_raw(query))
		{
			const char *q_start = query;
			while (*q_start == ' ' || *q_start == '\t' || *q_start == '\n' || *q_start == '\r')
				q_start++;
			if (strncasecmp(q_start, "PREPARE", 7) == 0)
			{
				tecc_record_error(state, TECC_ERR_PROCESSOR,
								  "tecc_decrypt_raw is not supported in PREPARE ... AS statements",
								  __func__, __LINE__, false);
				return NULL;
			}
		}
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
	    (state->last_stmt_type == TECC_STMT_TYPE_CREATE_TABLE ||
	     state->last_stmt_type == TECC_STMT_TYPE_ALTER_TABLE) &&
	    state->last_table_name[0] != '\0')
	{
	    const char *db_name = PQdb(conn);
	    if (db_name == NULL) db_name = "postgres";
	    tecc_refresh_table_cache(state, conn, db_name,
	        state->last_table_schema,
	        state->last_table_name);
	    state->last_stmt_type = TECC_STMT_TYPE_UNKNOWN;
	}

	/* ReqID:TECC4.0 - search_path 缓存更新机制变更说明（Review 问题2 修复）：
	 * 1. 后端 search_path GUC 已加 GUC_REPORT，后端在每个命令边界
	 *    （ReadyForQuery 前）上报 search_path 最终权威值，libpq 自动保存，
	 *    tecc_cache_manager 的 get_live_search_path / sync_guc_params_from_live_conn
	 *    惰性同步路径激活 —— 跨语句一致性的唯一权威来源；
	 * 2. 同串即时性（"SET search_path=x; SELECT 密态 ..." 同一查询串内，
	 *    ParameterStatus 尚未到达）由语句处理器对解析后的 SetStmt 做预测
	 *    预写缓存（tecc_processor.c FE_T_SetStmt 分支），执行后由权威值覆盖。
	 * 故此处不再需要（也不再允许）发送前的字符匹配拦截。 */

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
		const char *cp = query + 4;
		while (*cp == ' ' || *cp == '\t') cp++;
		TeccCopyState *copy_state = tecc_preprocess_copy(conn, query);
		if (copy_state == NULL)
		{
			/* COPY preprocessing skipped (server-side file COPY) or failed.
			 * tecc_preprocess_copy does not record an error for parse failures
			 * since only client-side COPY (FROM STDIN / TO STDOUT, used by
			 * psql \copy) needs TECC processing. Let the query proceed. */
			return NULL;
		}
		/* Client-side COPY: set up copy_state successfully.
		 * Only subquery COPY (COPY (SELECT ...) TO STDOUT) needs query
		 * rewriting — the inner SELECT must go through the same encrypted
		 * column preprocessing (WHERE/ORDER BY) as a standalone SELECT.
		 * Plain table COPY (\copy tab TO) needs no rewriting. */
		if (*cp != '(')
			return NULL;
		/* Fall through to the common query handler for subquery rewriting */
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

		/* Call processor */
		tecc_processor_debug(TECC_LOG_DEBUG1,
					"DEBUG: tecc_preprocess_query: Calling tecc_processor_run_pre_query");

		/* Begin - ReqID:TECC4.0 - Sync GUC params from cache to scanner thread-locals.
		 * The scanner needs standard_conforming_strings to decide how to handle
		 * backslash-quote sequences in regular string literals.  We read the
		 * authoritative value from the TECC cache (loaded during tecc_state_init,
		 * refreshed on ParameterStatus) and write it to thread-local variables
		 * that the flex scanner reads via getter functions. */
		{
			const char *scs_val = tecc_cache_get_guc_param(state,
															  "standard_conforming_strings");
			const char *bq_val  = tecc_cache_get_guc_param(state,
															 "backslash_quote");
			tecc_parser_set_standard_conforming_strings(
				scs_val == NULL || strcmp(scs_val, "on") == 0);
			tecc_parser_set_backslash_quote(
				(bq_val != NULL && bq_val[0] != '\0') ? bq_val[0] : 's');
		}
		/* End - ReqID:TECC4.0 - Sync GUC params to scanner */

		proc_result = tecc_processor_run_pre_query(&stmt_data);
		tecc_processor_debug(TECC_LOG_DEBUG1,
			"DEBUG: tecc_preprocess_query: tecc_processor_run_pre_query returned %d", proc_result);
		
	/* ReqID:TECC4.0 — After processor runs, copy column mapping
	 * from stmt_data to conn->tecc_copy_state for COPY subquery.
	 * NOTE: Only copy column_mapping (for BINARY mode). Do NOT copy
	 * column_names/column_count — the processor's allocations may reside
	 * in mmap'd memory that gets unmapped after processor return, causing
	 * use-after-free. Let the TEXT processor use attnum-based matching
	 * (column_count == 0) which works without column_names. */
	if (conn != NULL && conn->tecc_copy_state != NULL &&
		stmt_data.copy_column_mapping != NULL && stmt_data.copy_mapping_count > 0)
	{
		TeccEnhancedCopyState *cs = (TeccEnhancedCopyState *)conn->tecc_copy_state;
		/* 本次预处理前 state 上已有一份映射（identity 或上一轮 subquery），
		 * 先释放再接管 processor 的输出，避免每次 \copy 累积泄漏 */
		if (cs->column_mapping != NULL)
			tecc_pfree(cs->column_mapping);
		if (cs->column_table_names != NULL)
			tecc_pfree(cs->column_table_names);
		cs->column_mapping = stmt_data.copy_column_mapping;
		cs->mapping_count = stmt_data.copy_mapping_count;
		/* ReqID:TECC4.0 - Bug 907：多表 JOIN 子查询导出必需，逐列记录来源表 */
		cs->column_table_names = stmt_data.copy_column_table_names;
		/* NOT setting column_names/column_count — see note above */

		/* ReqID:TECC4.0 - Bug 907：来源表名到这一步才拿到，而加密列元数据
		 * 在 COPY 状态创建时就已加载（只含 FROM 首表），这里补载其余表的
		 * 密态列并重建上下文数组，否则 JOIN 右表的列匹配不到元数据、
		 * 导出仍是密文。 */
		if (cs->column_table_names != NULL)
			(void)tecc_copy_state_reload_mapped_columns(cs);
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

	/*
	 * ReqID:TECC4.0 - DDL-only（分布式内部连接）：不做常量加密。
	 *
	 * 必须放在 tecc_is_enabled() 之前：DDL-only 下 state->enabled 是 false
	 * （数据面整体关闭，与改造前等价），否则这里会直接返回、DDL 通路永远走不到。
	 *
	 * DDL 也可能走扩展协议（Citus 用无名 prepared statement 下发逻辑表 DDL，
	 * 实测 `execute <unnamed>: CREATE TABLE ...`），所以 0 参数时仍要落到下面的
	 * "0 参数走文本改写"分支；只有带参数（必然是 DML）才原样透传。
	 */
	if (state != NULL && state->ddl_only)
	{
		if (nParams > 0)
			return NULL;
	}
	else if (!tecc_is_enabled(state))
	{
		return NULL;
	}

	/* ReqID:TECC4.0 - 管道模式支持：0 参数扩展协议查询（PQsendQueryParams
	 * nParams=0，管道模式下简单协议文本不可用）必须走文本改写路径。
	 * 此时 tecc_preprocess_query 负责把加密常量改写进 SQL 文本，返回
	 * TeccQueryParams（仅 rewritten_query 有意义，无参数需调整）。
	 * 注意：non-pipeline 下 PQsendQueryParams(0 参数) 也走此路径，行为一致。 */
	if (nParams <= 0)
	{
		char	   *rewritten = NULL;

		/* 调用 tecc_preprocess_query 做文本改写（与简单协议路径相同） */
		rewritten = tecc_preprocess_query(conn, command);
		if (rewritten == NULL)
		{
			/* 无改写（无加密列 / disable_once 等）或错误——tecc_preprocess_query
			 * 内部已记录 last_error，调用方 fe-exec.c 会据此中止。返回 NULL。 */
			return NULL;
		}

		params = (TeccQueryParams *) pg_malloc(sizeof(TeccQueryParams));
		if (params == NULL)
		{
			tecc_record_error(state, TECC_ERR_MEMORY,
							 "Failed to allocate query parameters structure",
							 __func__, __LINE__, false);
			tecc_pfree(rewritten);
			return NULL;
		}
		memset(params, 0, sizeof(TeccQueryParams));
		params->nParams = 0;
		params->owns_memory = true;
		params->params_modified = false;
		params->query_modified = true;
		params->rewritten_query = rewritten;
		return params;
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

		/* Step 1: Parse query to determine if encrypted columns are involved */
		proc_result = tecc_processor_run_pre_query(&stmt_data);

		/* ReqID:TECC4.0 - 移除遗留 fallback：参数化 INSERT/SELECT/UPDATE/
		 * DELETE 的参数加密已由正常处理器路径（tecc_map_insert_params /
		 * tecc_map_update_params / tecc_map_comparison_params + process_parameters）
		 * 完整覆盖，不再需要 tecc_encrypt_params_fallback 的绕过方案。
		 * NO_ENCRYPTED_COLUMNS 时直接走 Step 2（无加密列则参数不加密，
		 * 行为与原先 fallback 空操作一致）。 */

		if (proc_result != TECC_PROCESSOR_SUCCESS &&
			proc_result != TECC_PROCESSOR_NEEDS_REWRITE &&
			proc_result != TECC_PROCESSOR_NO_ENCRYPTED_COLUMNS)
		{
			/* Processor error during query analysis */
			tecc_record_error(state, TECC_ERR_PROCESSOR,
				"Processor failed to analyze parameterized query",
				__func__, __LINE__, false);
			return NULL;
		}

		/* Step 2: Process parameters (encryption) */
		proc_result = tecc_processor_run_pre_exec(&stmt_data);

		if (proc_result == TECC_PROCESSOR_NEEDS_REWRITE ||
			proc_result == TECC_PROCESSOR_SUCCESS)
		{
			/* Query needs processing - parameters may have been encrypted */
			params->query_modified = true;
			params->params_modified = true;

			/* ReqID:TECC4.0 - 拷贝改写后的 SQL（字面量加密 + $N 占位保留），
			 * 供 fe-exec 发送。此前缺失导致含字面量的参数化 INSERT 发送原
			 * SQL（字面量明文，服务端报 "invalid hex string"）。 */
			if (stmt_data.rewritten_query != NULL && params->rewritten_query == NULL)
				params->rewritten_query = tecc_strdup_ex(state, stmt_data.rewritten_query);

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

	/*
	 * ReqID:TECC4.0 - DDL-only（分布式内部连接）：绝不解密任何结果。
	 * 这是 W1 的核心底线 —— 密文不能在服务端进程里变成明文。
	 */
	if (state->ddl_only)
	{
		return;
	}

	if (!state->guc_params.tecc_mode)
	{
		int			raw_idx;

		if (state->pending_decrypt_raw_call)
		{
			state->last_result_status = PQresultStatus(result);
			state->stats.query_count++;
			tecc_record_error(state, TECC_ERR_PROCESSOR,
							  "tecc_decrypt_raw requires enable hg_tecc_mode",
							  __func__, __LINE__, false);
			return;
		}

		for (raw_idx = 0; raw_idx < state->pending_expr_col_count; raw_idx++)
		{
			if (state->pending_expr_col_raw_hex[raw_idx])
			{
				state->last_result_status = PQresultStatus(result);
				state->stats.query_count++;
				tecc_record_error(state, TECC_ERR_PROCESSOR,
								  "tecc_decrypt_raw requires enable hg_tecc_mode",
								  __func__, __LINE__, false);
				return;
			}
		}

		state->last_result_status = PQresultStatus(result);
		state->stats.query_count++;
		tecc_processor_debug(TECC_LOG_DEBUG1,
							 "tecc_process_result: hg_tecc_mode=off, skip result decryption");
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
			tecc_refresh_kek_cache(state,
				state->last_key_name, state->last_key_namespace);
		}
		else if (state->last_stmt_type == TECC_STMT_TYPE_CREATE_DEK &&
				 state->last_key_name[0] != '\0')
		{
			tecc_refresh_dek_cache(state,
				state->last_key_name, state->last_key_namespace);
		}
	else if (state->last_stmt_type == TECC_STMT_TYPE_ALTER_KEK)
	{
		/*
		 * ALTER KEK rotated the KEK and re-encrypted all associated DEKs.
		 * Cached plaintext KEK/DEK values are now stale (the KEK file
		 * changed, and DEK encrypted_value rows were updated). Clear the
		 * caches so the next access reloads the new KEK plaintext and
		 * decrypts DEKs with it. Otherwise a long-lived connection would
		 * keep using the old KEK and fail HMAC on the re-encrypted DEKs.
		 */
		tecc_cache_clear(state);
		/* ReqID:TECC4.0 - 清除 key_manager 层的 DEK 明文缓存（key_cache）。
		 * tecc_cache_clear 只清 tecc_cache 的 KEK/DEK 元数据，不清 key_manager
		 * 的 DEK 明文缓存；不清理会导致 ALTER KEK 后同连接 INSERT 用旧 DEK
		 * 明文加密、SELECT 用新 DEK 明文解密而失败。 */
		if (state->key_mgr != NULL)
			tecc_key_cache_clear((TeccKeyManagerAdapter *)state->key_mgr);
		/* ReqID:TECC4.0 - Bug 9263: 轮换会用新 KEK 公钥重新封装 HE 私钥信封
		 * （he_ec_privkey/he_eg_privkey），而 crypto scheduler 里的进程级 HE 元
		 * 数据缓存存的正是旧信封 hex。上面两个清除函数都覆盖不到它，不清就会拿
		 * 旧信封配新 KEK 解封，静默得到错误私钥，HE 运算结果变成垃圾值并报
		 * "HE result out of range for column ..."。 */
		tecc_he_meta_cache_clear();
		/* ReqID:TECC4.0 - 清除后立即重新加载 KEK 元数据（kms_type/kms_path）。
		 * swxa 数据加解密路由（tecc_setup_hsm_context）按 KEK 缓存里的
		 * kms_type 判断走 HSM 还是软件路径；若只清除不重载，缓存 miss 会被
		 * 误判为软件路径，导致 HSM 密文按软件算法解密失败（重连才恢复）。 */
		if (state->last_key_name[0] != '\0')
			tecc_refresh_kek_cache(state,
				state->last_key_name, state->last_key_namespace);
		tecc_record_debug(state,
			"ALTER KEK: cleared KEK/DEK caches after rotation");
		state->last_key_name[0] = '\0';
	}

		/* Begin - ReqID:TECC4.0 - 检测SET语句并更新GUC参数 */

		/* End - ReqID:TECC4.0 - 检测SET语句并更新GUC参数 */
	}

	/* ReqID:TECC4.0 - 执行 update_meta() 的会话：语句成功执行后立即清空本地
	 * 元数据缓存，下次查询重新加载（不等 60s mark 节流）。update_meta 是 SELECT
	 * （返回 PGRES_TUPLES_OK），不走上面的 COMMAND_OK DDL 分支，故单独处理。 */
	if (state->last_stmt_type == TECC_STMT_TYPE_UPDATE_META &&
		state->last_result_status == PGRES_TUPLES_OK)
	{
		tecc_cache_clear(state);
		tecc_record_debug(state, "update_meta(): cleared local metadata cache");
		state->last_stmt_type = TECC_STMT_TYPE_UNKNOWN;
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

/* Begin - ReqID:TECC4.0 - Allow psql hg_tecc.mode to enable TECC lazily */
static bool
tecc_enable_connection_for_mode(PGconn *conn)
{
	TeccState  *old_state;
	TeccState  *new_state;

	if (conn == NULL)
		return false;

	if (conn->enable_tecc)
		return true;

	old_state = tecc_get_state(conn);
	if (old_state != NULL)
		tecc_state_cleanup(old_state);

	conn->enable_tecc = true;
	new_state = tecc_state_init(conn);
	if (new_state == NULL || !new_state->initialized || !new_state->enabled)
	{
		conn->enable_tecc = false;
		return false;
	}

	return true;
}
/* End - ReqID:TECC4.0 - Allow psql hg_tecc.mode to enable TECC lazily */

/*
 * PQteccSetMode - Set TECC result decryption mode for this connection
 *
 * Returns: 0 on success, -1 on failure
 */
int
PQteccSetMode(PGconn *conn, int enabled)
{
	TeccState *state;

	/* Begin - ReqID:TECC4.0 - Do not depend on startup TECC state */
	if (conn == NULL)
		return -1;

	if (!conn->enable_tecc)
	{
		if (!tecc_enable_connection_for_mode(conn))
			return -1;
	}

	state = tecc_get_state(conn);
	if (state == NULL || !state->initialized)
		return -1;

	state->guc_params.tecc_mode = (enabled != 0);
	state->enabled = conn->enable_tecc;
	/* End - ReqID:TECC4.0 - Do not depend on startup TECC state */
	return 0;
}

/*
 * PQteccGetMode - Get TECC result decryption mode for this connection
 *
 * Returns: 0 on success, -1 on failure
 */
int
PQteccGetMode(PGconn *conn, int *enabled)
{
	TeccState *state;

	/* Begin - ReqID:TECC4.0 - Report mode even when startup TECC is off */
	if (conn == NULL || enabled == NULL)
		return -1;

	if (!conn->enable_tecc)
	{
		*enabled = 0;
		return 0;
	}

	state = tecc_get_state(conn);
	if (state == NULL || !state->initialized)
		return -1;

	*enabled = state->guc_params.tecc_mode ? 1 : 0;
	/* End - ReqID:TECC4.0 - Report mode even when startup TECC is off */
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

/* ReqID:TECC4.0 - 补回返回类型（头文件声明为 void；缺省 int 会让 MSVC 报 C2371） */
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

	if (updates->update_tecc_mode)
	{
		state->guc_params.tecc_mode = updates->new_tecc_mode;
	}

	/* 可以继续添加其他GUC参数的更新处理 */

	return true;
}

/* End - ReqID:TECC4.0 - GUC参数同步函数 */


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
	else if (strcmp(param_name, "hg_tecc_mode") == 0)
	{
		if (strcmp(param_value, "on") != 0 && strcmp(param_value, "off") != 0)
			return false;
		updates.update_tecc_mode = true;
		updates.new_tecc_mode = (strcmp(param_value, "on") == 0);
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
	state->guc_params.tecc_mode = true;
}

/* End - ReqID:TECC4.0 - GUC参数管理公共接口实现 */

/* Begin - ReqID:TECC4.0 - 会话级临时表清单管理 */

/*
 * tecc_state_is_temp_table - 判断表名是否在本会话临时表清单中
 *
 * 参数:
 *   state - TECC状态
 *   relname - 表名
 *
 * 返回:
 *   true表示该表是本会话创建的临时表
 */
bool
tecc_state_is_temp_table(const TeccState *state, const char *relname)
{
	int			i;

	if (state == NULL || relname == NULL || relname[0] == '\0')
		return false;

	for (i = 0; i < state->temp_table_count; i++)
	{
		if (state->temp_table_names[i][0] != '\0' &&
			strcmp(state->temp_table_names[i], relname) == 0)
			return true;
	}
	return false;
}

/*
 * tecc_state_add_temp_table - 将表名加入会话级临时表清单
 *
 * CREATE TEMP TABLE 成功预解析时调用（语句在预处理阶段，尚未执行）。
 * 上限 TECC_MAX_TEMP_TABLES，超出时忽略（仅影响未限定名遮蔽识别，不致命）。
 */
void
tecc_state_add_temp_table(TeccState *state, const char *relname)
{
	if (state == NULL || relname == NULL || relname[0] == '\0')
		return;

	if (tecc_state_is_temp_table(state, relname))
		return;

	if (state->temp_table_count >= TECC_MAX_TEMP_TABLES)
		return;

	strncpy(state->temp_table_names[state->temp_table_count], relname,
			NAMEDATALEN - 1);
	state->temp_table_names[state->temp_table_count][NAMEDATALEN - 1] = '\0';
	state->temp_table_count++;
}

/*
 * tecc_state_remove_temp_table - 从会话级临时表清单移除表名
 *
 * DROP TABLE 预处理时调用。PG 语义下 DROP TABLE 未限定名同样优先解析临时表，
 * 因此清单命中即移除。
 */
void
tecc_state_remove_temp_table(TeccState *state, const char *relname)
{
	int			i;

	if (state == NULL || relname == NULL || relname[0] == '\0')
		return;

	for (i = 0; i < state->temp_table_count; i++)
	{
		if (state->temp_table_names[i][0] != '\0' &&
			strcmp(state->temp_table_names[i], relname) == 0)
		{
			/* 用最后一个元素填补，保持紧凑 */
			state->temp_table_count--;
			if (i < state->temp_table_count)
			{
				strcpy(state->temp_table_names[i],
					   state->temp_table_names[state->temp_table_count]);
			}
			state->temp_table_names[state->temp_table_count][0] = '\0';
			return;
		}
	}
}

/* End - ReqID:TECC4.0 - 会话级临时表清单管理 */

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
