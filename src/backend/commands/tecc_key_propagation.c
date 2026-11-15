/*-------------------------------------------------------------------------
 *
 * tecc_key.c
 *	  TECC 密钥元数据（KEK/DEK）的集群级传播。
 *
 * TECC 的 KEK/DEK 元数据（hg_kek / hg_kek_args / hg_dek / hg_dek_args）记在**每个**
 * 节点各自的系统表里，而分片建表会在 worker 上按名字解析 DEK
 * （record_encrypted_column_metadata 在 DEK 缺失时直接 ERROR），所以 coordinator 上
 * 建/改/删 KEK/DEK 之后必须让所有 worker 跟上，否则新加的 worker 上建不出加密分片表。
 *
 * 传播方式：把 coordinator 收到的**已处理语句原文**原样重放到每个 worker。
 * 这样做既是可行的、也是必须的：
 *
 *   1. libpq 侧的 TECC 处理器在把 CREATE KEK/DEK 发往服务端之前，已经把
 *      `_TECC_PROCESSED` 标记、`_KEK_FILE_CHECKSUM`、`public_key`，以及 DEK 的
 *      `ENCRYPTED_VALUE` / `HE_EC_PUBKEY` / `HE_EC_PRIVKEY` 等**密钥材料**注入进了
 *      语句文本（实测服务端 log_statement 记录到的原文：
 *        create dek w3_dek with values (kek=w3_kek, ENCRYPTED_VALUE = 'de6fd0...',
 *                                       _TECC_PROCESSED = 'yes');
 *        create kek w3_hekek with (key_store='localkms', key_algo='sm2',
 *                                  key_path='...', public_key = '04b124...',
 *                                  _tecc_processed = 'yes');
 *      ）。照搬这段文本重放，worker 上落的就是**同一份**密钥材料，不需要额外映射、
 *      也不会因为各自再生成一份而与 coordinator 不一致。
 *
 *   2. 服务端的 TECC DDL 校验只认语句里带的 `_TECC_PROCESSED` 标记
 *      （createkek.c / createdek.c），而 Citus 的内部连接被**故意**关掉了 TECC
 *      （W1：libpq 对"在服务端进程内打开的连接"强制 enable_tecc=false）。所以
 *      不能让 worker 自己走客户端处理：只发用户原文会被 worker 拒绝
 *      （ERROR: CREATE KEK requires TECC encryption mode to be enabled），
 *      而原样重放带标记的文本正好绕开这条限制。
 *
 *   3. 顺序天然正确：语句到达顺序 = KEK 先于 DEK；DEK 又必然先于引用它的分片建表。
 *
 *   4. **例外：ALTER KEK 轮换时，光重放 ALTER 语句本身不够**。前端把 ALTER KEK
 *      改写成一整条字符串里的**多条语句**：先是 ALTER KEK 原文（带
 *      `_tecc_processed`），紧跟它派生出来的密钥材料写回调用
 *      （stmt_processor.c 用 update_buf 拼出，见 hg_rotate_funcs.sql）：
 *        SELECT hg_kek_rotate_dek_arg(<dek oid>, '<arg>', '<hex>');
 *        SELECT hg_kek_rotate_kek_arg(<kek oid>, '<arg>', <bytea>);
 *      PG 把这条字符串按分号拆成多棵语法树逐个走 utility hook，而上面第 1、2 点
 *      的重放通道只认 `ALTER KEK` 本身（IsTeccKeyStmt），那两条 SELECT 不匹配
 *      ⇒ 轮换后 worker 会停在"key_path 已换新、DEK 密文与公钥仍是旧的"的半途状态。
 *      所以这两条也必须传播，见 TeccKeyRotationStmtText。
 *
 *      注意这里有个取巧但必要的落点：SELECT 根本不走 ProcessUtility（Citus 只
 *      hook 了 ProcessUtility_hook，SELECT 走 planner/executor），所以**单独一条
 *      `SELECT hg_kek_rotate_dek_arg(...)` 是拦不到的**。实现改为在 ALTER KEK 的
 *      postprocess 里把**同一条字符串的尾部**重新解析一遍取出这些调用
 *      （TeccKeyTrailingRotateCommands）。因此受支持的形态就是前端实际发射的
 *      "ALTER KEK 紧跟上写回调用"这一种；用户手写孤立的一条 rotate 调用不会被
 *      传播（该调用本身是内部的受控写入口，正常不这么用）。
 *
 * 前置条件（本模块不负责、运维需保证）：localkms 的密钥文件必须能被 worker 读到
 * —— 非对称 KEK 直接 stat `key_path`（不做 TECC_KEK_PATH 覆盖），对称 KEK 走
 * TECC_KEK_PATH 覆盖；worker 上重放 CREATE KEK 时仍会校验该文件。
 *
 * 落点（2026-10-29 外围化，本文件已搬出 Citus）：PG 核心的 utility hook
 * postprocess 段（本地命令成功之后）通过 **SQL 层**的
 * `run_command_on_workers(<语句文本>)` 下发 —— 按与 hgdde 团队的约定，插件的 SQL
 * 函数/目录表可按同名同签名依赖，因此这条路径对 Citus 与 hgdde 通用，不需要插件
 * 提供任何 C 层 API，也不再需要 DISABLE/ENABLE_DDL_PROPAGATION 包裹（worker 上
 * `citus_is_coordinator()` 为假，本钩子不会二次传播）。
 *
 * ReqID:TECC4.0
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/genam.h"
#include "access/htup_details.h"
#include "access/table.h"
#include "catalog/hg_dek.h"
#include "catalog/hg_kek.h"
#include "catalog/namespace.h"
#include "catalog/objectaccess.h"
#include "catalog/objectaddress.h"
#include "commands/tecc_dist_replay.h"
#include "commands/tecc_key_propagation.h"
#include "commands/tecc_key_recipe.h"
#include "executor/spi.h"
#include "miscadmin.h"
#include "nodes/parsenodes.h"
#include "nodes/pg_list.h"
#include "nodes/primnodes.h"
#include "tcop/tcopprot.h"
#include "tcop/utility.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/rel.h"


/*
 * 前端在 ALTER KEK 之后注入的两条密钥材料写回函数的名字（定义见
 * src/backend/catalog/hg_rotate_funcs.sql）。两个名字与前端
 * tecc_stmt_processor_process_alter_kek() 里的发射点一一对应，改名必须同步。
 */
#define TECC_ROTATE_DEK_ARG_FUNC "hg_kek_rotate_dek_arg"
#define TECC_ROTATE_KEK_ARG_FUNC "hg_kek_rotate_kek_arg"


/* 本文件对外只暴露 TeccKeyPropagateIfNeeded / 两个地址解析函数，其余为内部实现 */
static char * TeccKeyStmtText(const char *queryString, int stmt_location, int stmt_len);
static List * TeccKeyStmtNames(Node *parsetree);
static bool TeccKeyStmtTargetIdentity(Node *parsetree, Oid *catalogId,
									  Oid *namespaceId, char **objectName);
static List * TeccKeyStatementCommands(Node *parsetree, const char *queryString,
									   int stmt_location, int stmt_len);
static List * TeccKeyTrailingRotateCommands(const char *queryString, int stmt_location,
											int stmt_len, List **seqTracker);
static Oid TeccKeyRotateCallCatalog(Node *parsetree);
static char * TeccKeyRotationStmtText(Node *parsetree, const char *queryString,
									  int stmt_location, int stmt_len,
									  Oid *objectClass, Oid *namespaceId,
									  char **objectName);
static void TeccKeyRotationShapeError(const char *detail);
static void TeccKeyReplayOnWorkers(const char *replay);
static void TeccKeyCollectObjectIds(Oid catalogId, List **objectIds);
static void TeccKeyAppendBootstrapStep(StringInfo replay, const char *command);
static void TeccKeyMarkDistributed(Node *parsetree);
static void TeccKeyUnmarkDistributed(Oid classId, Oid objectId);
static void TeccKeyProviderUpsert(const char *objectType, const char *objectName,
								 const char *createDdl);


/*
 * TeccKeyProviderUpsert records the create command of a KEK/DEK into the generic
 * object-create provider table (`hgdde.distributed_object_create_ddl`, created by
 * the distributed extension), so that a node added later can have the object created
 * for it by the distributed metadata replay before any encrypted column's shell is
 * built there.
 *
 * The provider table only exists in a cluster that runs a distributed extension
 * implementing the generic provider pass (hgdde's `create_distributed_object_dependencies`);
 * clusters without it are unaffected.
 */
static void
TeccKeyProviderUpsert(const char *objectType, const char *objectName, const char *createDdl)
{
	int ret;

	if (createDdl == NULL || objectName == NULL || objectType == NULL)
	{
		return;
	}

	if (SPI_connect() != SPI_OK_CONNECT)
	{
		return;
	}

	ret = SPI_execute("SELECT pg_catalog.to_regclass('hgdde.distributed_object_create_ddl') IS NOT NULL",
					 true, 1);
	if (ret != SPI_OK_SELECT || SPI_processed != 1)
	{
		SPI_finish();
		return;
	}

	{
		bool		isnull = true;
		Datum	value = SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1, &isnull);

		if (isnull || !DatumGetBool(value))
		{
			SPI_finish();
			return;
		}
	}

	{
		char	   *typeLit;
		char	   *nameLit;
		char	   *ddlLit;
		StringInfoData insert;

		typeLit = quote_literal_cstr(objectType);
		nameLit = quote_literal_cstr(objectName);
		ddlLit = quote_literal_cstr(createDdl);

		initStringInfo(&insert);
		appendStringInfo(&insert, "INSERT INTO hgdde.distributed_object_create_ddl \
(object_type, object_name, create_ddl) VALUES (");
		appendStringInfo(&insert, "%s, %s, %s", typeLit, nameLit, ddlLit);
		appendStringInfo(&insert, ") ON CONFLICT DO NOTHING");
		ret = SPI_exec(insert.data, 0);
		pfree(insert.data);
		pfree(typeLit);
		pfree(nameLit);
		pfree(ddlLit);

		if (ret != SPI_OK_INSERT)
		{
			SPI_finish();
			elog(WARNING, "TeccKeyProviderUpsert failed: could not record the distributed object create command");
		}

		SPI_finish();
	}
}

/*
 * IsTeccKeyStmt returns whether the statement is one of TECC's key management
 * statements (CREATE KEK / ALTER KEK / CREATE DEK / DROP KEK / DROP DEK).
 */
static bool
IsTeccKeyStmt(Node *parsetree)
{
	if (parsetree == NULL)
	{
		return false;
	}

	if (IsA(parsetree, CreateKekStmt) || IsA(parsetree, AlterKekStmt) ||
		IsA(parsetree, CreateDekStmt))
	{
		return true;
	}

	if (IsA(parsetree, DropStmt))
	{
		DropStmt *dropStmt = castNode(DropStmt, parsetree);

		return dropStmt->removeType == OBJECT_KEK || dropStmt->removeType == OBJECT_DEK;
	}

	return false;
}


/*
 * TeccKeyStmtText extracts the text of the statement that is currently being
 * executed. queryString/stmt_location/stmt_len come from the utility hook; for
 * the simple query protocol the text is the statement as received from the
 * client -- which for TECC statements is the *processed* text (key material
 * already injected by the frontend).
 *
 * TeccKeyStmtText returns NULL when there is no text to extract.
 */
static char *
TeccKeyStmtText(const char *queryString, int stmt_location, int stmt_len)
{
	const char *statementStart = NULL;
	size_t		statementLength = 0;

	if (queryString == NULL || stmt_location < 0 ||
		(size_t) stmt_location > strlen(queryString))
	{
		return NULL;
	}

	statementStart = queryString + stmt_location;

	if (stmt_len > 0)
	{
		statementLength = (size_t) stmt_len;
	}
	else
	{
		statementLength = strlen(statementStart);
	}

	if (statementLength == 0)
	{
		return NULL;
	}

	return pnstrdup(statementStart, statementLength);
}


/*
 * TeccKeyPropagateIfNeeded replays a TECC key statement -- and, for ALTER KEK, the
 * material write-back calls the frontend appended right after it -- on every worker.
 *
 * Must be called from the utility hook's postprocess stage: the local statement has
 * succeeded by then, so the objects the write-backs address are known to exist.
 *
 * Replaying to the workers that are registered *right now* is only half the job:
 * the same command list is also persisted as each object's reconstruction recipe
 * (hg_tecc_ddl, see TeccKeyStatementCommands), so a worker added later can be
 * brought up to date. The two consumers therefore share one source of truth.
 *
 * No-op outside a cluster: when the plugin is absent (single node) the SQL-level
 * entry point does not resolve, and on a worker citus_is_coordinator() is false.
 */
void
TeccKeyPropagateIfNeeded(Node *parsetree, const char *queryString,
						 int stmt_location, int stmt_len)
{
	List	   *commands = NIL;
	ListCell   *lc;
	StringInfoData replay;

	if (!IsTeccKeyStmt(parsetree))
	{
		return;
	}

	/*
	 * 先判"是不是协调节点"：worker 上重放到达的 KEK DDL 同样是 utility 语句，若不
	 * 在这里提前返回，它会再去解析"尾部材料写回调用"并试图二次传播（实测报
	 * cannot propagate TECC key material write-back statement）。
	 */
	if (!TeccDistIsCoordinator())
	{
		return;
	}

	commands = TeccKeyStatementCommands(parsetree, queryString,
										stmt_location, stmt_len);

	if (commands == NIL)
	{
		return;
	}

	/*
	 * 多条语句合成一次下发：ALTER KEK 与其材料写回调用必须落在同一个隐式事务里，
	 * 不能让 worker 停在"key_path 换了、材料还是旧的"的半途状态。
	 */
	initStringInfo(&replay);
	foreach(lc, commands)
	{
		char	   *command = (char *) lfirst(lc);

		if (replay.len > 0)
		{
			appendStringInfoString(&replay, ";\n");
		}
		appendStringInfoString(&replay, command);
	}

	TeccKeyReplayOnWorkers(replay.data);

	pfree(replay.data);

	/*
	 * 再把这个对象登记为"分布式对象"（pg_dist_object）。
	 *
	 * 这一步是"新节点在创建 shell 表**之前**就拿到密钥"的唯一可行机制：加节点时
	 * shell 表是在 citus_add_node() **内部**建的，而插件的依赖机制会先按
	 * pg_dist_object 重放依赖对象的 DDL、再建依赖它的表。绕过插件、改成
	 * "语句成功之后我们再去补发"是来不及的 —— 实测新节点在那一刻缺 DEK，
	 * CREATE TABLE ... ENCRYPTED WITH 直接失败，整个 add_node 回滚。
	 *
	 * 这也是 W4a 目前唯一的 citus 侧残留（distribute_object_ops.c 的登记表 +
	 * dependencies.c / dependency.c / pg_get_object_address / multi_executor 的
	 * KEK/DEK 分支）。详情见 plan/2026-10-28-hgdde分布式适配可行性分析.md §0.5.4。
	 */
	TeccKeyMarkDistributed(parsetree);
}


/*
 * TeccKeyMarkDistributed records a freshly created KEK/DEK in pg_catalog.pg_dist_object,
 * which is what the distributed plugin scans when a new node joins. A no-op on a single
 * node (the catalog does not exist) and for statements other than CREATE.
 */
static void
TeccKeyMarkDistributed(Node *parsetree)
{
	List	   *addresses = NIL;
	ListCell   *lc;
	int			ret;
	bool		isnull = true;
	Datum		value;

	if (!IsA(parsetree, CreateKekStmt) && !IsA(parsetree, CreateDekStmt))
	{
		return;
	}

	SPI_connect();

	ret = SPI_execute("SELECT pg_catalog.to_regclass('pg_catalog.pg_dist_object') "
					  "IS NOT NULL", true, 1);
	if (ret != SPI_OK_SELECT || SPI_processed != 1)
	{
		SPI_finish();
		return;
	}

	value = SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1, &isnull);
	if (isnull || !DatumGetBool(value))
	{
		SPI_finish();
		return;
	}

	if (IsA(parsetree, CreateKekStmt))
	{
		addresses = CreateKekStmtObjectAddress(parsetree, false, true);
	}
	else
	{
		addresses = CreateDekStmtObjectAddress(parsetree, false, true);
	}

	foreach(lc, addresses)
	{
		ObjectAddress *address = (ObjectAddress *) lfirst(lc);
		Oid			paramTypes[3] = {OIDOID, OIDOID, INT4OID};
		Datum		paramValues[3];

		paramValues[0] = ObjectIdGetDatum(address->classId);
		paramValues[1] = ObjectIdGetDatum(address->objectId);
		paramValues[2] = Int32GetDatum(address->objectSubId);

		ret = SPI_execute_with_args("INSERT INTO pg_catalog.pg_dist_object "
									"(classid, objid, objsubid) VALUES ($1, $2, $3) "
									"ON CONFLICT DO NOTHING",
									3, paramTypes, paramValues, NULL, false, 0);
		if (ret != SPI_OK_INSERT)
		{
			SPI_finish();
			ereport(ERROR,
					(errcode(ERRCODE_INTERNAL_ERROR),
					 errmsg("could not register TECC key object as a distributed object")));
		}
	}

	SPI_finish();
}


/*
 * 判"是不是协调节点"以前在这里用 SPI 直接问 citus_is_coordinator()。这套探测已经
 * 收进 commands/tecc_dist_replay.c（TeccDistIsCoordinator）—— 表/列标记传播与新节点
 * 补发需要同一条判据，重复实现会漂移。
 */


/*
 * TeccKeyReplayOnWorkers sends one SQL text to every worker through
 * run_command_on_workers(), and fails the whole statement when any worker reports a
 * failure.
 *
 * The replay is wrapped in a DO block that tolerates the two "already converged"
 * errors (object already exists / already absent). That tolerance is required
 * because the per-node replay is not atomic across nodes: if one worker is down, the
 * workers already reached keep their change while the coordinator rolls back, so a
 * plain retry would die on "already exists" on those nodes and the cluster could
 * never converge. Both tolerated cases mean the worker is already where the
 * coordinator wants it, and each one raises a WARNING so it stays visible.
 *
 * Any other failure is fatal: the coordinator would otherwise keep a KEK/DEK the
 * workers do not have, and every later shard DDL referencing it would break with a
 * confusing "DEK does not exist" on some node.
 */
static void
TeccKeyReplayOnWorkers(const char *replay)
{
	StringInfoData wrapped;

	initStringInfo(&wrapped);
	appendStringInfo(&wrapped,
					 "DO $tecc_replay_%d$ BEGIN EXECUTE %s; "
					 "EXCEPTION "
					 "WHEN duplicate_object THEN "
					 "  RAISE WARNING 'tecc: key object already present, replay skipped'; "
					 "WHEN undefined_object THEN "
					 "  RAISE WARNING 'tecc: key object already absent, replay skipped'; "
					 "END $tecc_replay_%d$",
					 MyProcPid, quote_literal_cstr(replay), MyProcPid);

	TeccDistReplayOnWorkers(wrapped.data, "TECC key statement");

	pfree(wrapped.data);
}


/* ---------------------------------------------------------------------
 * 新节点补发：把所有 KEK/DEK 的重建配方整体重放
 * ---------------------------------------------------------------------
 */

/*
 * TeccKeyRegisteredAsDistributed reports whether this coordinator has registered any
 * KEK/DEK in pg_dist_object, i.e. whether the distributed plugin will replay the key
 * recipes itself when a node joins.
 *
 * Returns false for a single-node install (the catalog is absent) and on a worker.
 */
bool
TeccKeyRegisteredAsDistributed(void)
{
	bool		registered = false;
	int			ret;
	bool		isnull = true;
	Datum		value;

	if (!TeccDistIsCoordinator())
	{
		return false;
	}

	SPI_connect();

	ret = SPI_execute("SELECT pg_catalog.to_regclass('pg_catalog.pg_dist_object') "
					  "IS NOT NULL", true, 1);
	if (ret != SPI_OK_SELECT || SPI_processed != 1)
	{
		SPI_finish();
		return false;
	}

	value = SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1, &isnull);
	if (isnull || !DatumGetBool(value))
	{
		SPI_finish();
		return false;
	}

	ret = SPI_execute("SELECT EXISTS ("
					  "  SELECT 1 FROM pg_catalog.pg_dist_object d"
					  "  WHERE d.classid IN (SELECT c.oid FROM pg_catalog.pg_class c"
					  "                      WHERE c.relname IN ('hg_kek','hg_dek') AND"
					  "                            c.relnamespace = "
					  "                            'pg_catalog'::pg_catalog.regnamespace)"
					  ")", true, 1);
	if (ret == SPI_OK_SELECT && SPI_processed == 1)
	{
		isnull = true;
		value = SPI_getbinval(SPI_tuptable->vals[0], SPI_tuptable->tupdesc, 1, &isnull);
		registered = !isnull && DatumGetBool(value);
	}

	SPI_finish();

	return registered;
}


/*
 * TeccKeyReplayAllOnWorkers replays every KEK/DEK recipe on every worker.
 *
 * 触发点只有一处：tecc_dist_bootstrap.c 在 citus_add_node() 语句**成功之后**调用，
 * 且仅在插件没有把这些对象登记为分布式对象时调用（见
 * TeccKeyRegisteredAsDistributed）。
 *
 * 用途是"外围补强"：加节点那一刻**没有壳表要建**时（集群里只有密钥、还没有密态
 * 分布式表），插件在 add_node 内部不需要下发任何东西，密钥也就不会顺带送达 ——
 * 新节点于是静默地缺密钥，直到以后第一次建加密分布式表才炸。这一档补的就是这个缺口。
 *
 * 为什么登记了就必须让路：登记时插件会在 add_node **内部**重放同一批配方；此时若
 * 我们再在 ExecutorEnd 里重放一次，两份重放并发落在新节点的同一张 hg_kek/hg_dek 上，
 * 后发的那条要等先发的（尚未提交的）事务结束，而先发的事务又在等 add_node 提交
 * —— 实测 citus_add_node 就此挂死。两侧择一，谁管由"登记与否"决定，不靠猜。
 *
 * 为什么能在 add_node 成功之后工作：那时新节点已经登记进 pg_dist_node，
 * run_command_on_workers() 够得到它（同一位置的标记推送已实测在新节点落地）。
 * 它救不了"已有密态分布式表再扩容"的情形：那时依赖 DEK 的
 * CREATE TABLE ... ENCRYPTED WITH 早在 add_node 内部就执行过了（实测新节点缺 DEK
 * → 建壳表失败 → 整个 add_node 回滚）。
 *
 * 与 TeccKeyReplayOnWorkers 的区别只在容错粒度：这里是"很多个对象 × 很多条语句"，
 * 已经被别的节点执行过的语句必须各自跳过而不能让整段回滚 —— 所以**每条语句**包一层
 * BEGIN…EXCEPTION…END（PL/pgSQL 里每个带 EXCEPTION 的块是独立子事务），
 * 而不是把整段包一层。整段包一层的话，"KEK 已存在"会让同一对象后面的 DEK 永远建不出来。
 *
 * 容忍的两类错误都只记 DEBUG 而不是 WARNING：对已有节点来说"已经存在"是**正常**
 * 状态，每次补发都会重放一遍全部配方，用 WARNING 会把日志刷满；真正的失败
 * （权限、材料损坏、节点不可达…）仍然原样抛错，由调用方连带把 add_node 一起回滚。
 */
void
TeccKeyReplayAllOnWorkers(void)
{
	List	   *keys = NIL;
	ListCell   *lc;
	StringInfoData replay;
	bool		anyStep = false;

	/*
	 * worker 上不补发：重放到达的 CREATE KEK/DEK 同样是 utility 语句，若在 worker
	 * 上再走一遍，它会对着自己的 worker 列表再发一轮（实测报 cannot propagate
	 * TECC key material write-back statement）。单机库（插件不可用）由
	 * TeccDistReplayOnWorkers 自己退化为空操作，这里不必重复判断。
	 */
	if (!TeccDistIsCoordinator())
	{
		return;
	}

	/*
	 * KEK 必须先于 DEK：DEK 的材料是用 KEK 解开的，配方里 DEK 的 CREATE 也以
	 * KEK 已经存在为前提。
	 */
	TeccKeyCollectObjectIds(HgKekRelationId, &keys);
	TeccKeyCollectObjectIds(HgDekRelationId, &keys);

	if (keys == NIL)
	{
		return;
	}

	initStringInfo(&replay);
	appendStringInfo(&replay, "DO $tecc_key_bootstrap_%d$ BEGIN ", MyProcPid);

	foreach(lc, keys)
	{
		ObjectAddress *address = (ObjectAddress *) lfirst(lc);
		List	   *commands = TeccKeyRecipeCommands(address->classId,
													address->objectId);
		ListCell   *lc2;

		foreach(lc2, commands)
		{
			TeccKeyAppendBootstrapStep(&replay, (const char *) lfirst(lc2));
			anyStep = true;
		}
	}

	appendStringInfo(&replay, "END $tecc_key_bootstrap_%d$", MyProcPid);

	if (!anyStep)
	{
		/* 有对象却没有配方（历史遗留）时不必下发一个空 DO 块 */
		pfree(replay.data);
		return;
	}

	TeccDistReplayOnWorkers(replay.data, "TECC key recipe (new node bootstrap)");

	pfree(replay.data);
}


/*
 * TeccKeyCollectObjectIds appends the address of every object in a KEK/DEK catalog
 * to *objectIds, in catalog (OID) order.
 */
static void
TeccKeyCollectObjectIds(Oid catalogId, List **objectIds)
{
	Relation	rel;
	SysScanDesc scan;
	HeapTuple	tuple;
	AttrNumber	oidAttnum;

	Assert(catalogId == HgKekRelationId || catalogId == HgDekRelationId);

	oidAttnum = (catalogId == HgKekRelationId) ? Anum_hg_kek_oid : Anum_hg_dek_oid;

	rel = table_open(catalogId, AccessShareLock);
	scan = systable_beginscan(rel, InvalidOid, false, NULL, 0, NULL);

	while ((tuple = systable_getnext(scan)) != NULL)
	{
		ObjectAddress *address = palloc0(sizeof(ObjectAddress));
		bool		isNull = false;
		Datum		value = heap_getattr(tuple, oidAttnum,
										 RelationGetDescr(rel), &isNull);

		address->classId = catalogId;
		address->objectId = DatumGetObjectId(value);
		address->objectSubId = 0;

		*objectIds = lappend(*objectIds, address);
	}

	systable_endscan(scan);
	table_close(rel, AccessShareLock);
}


/*
 * TeccKeyAppendBootstrapStep appends one recipe statement as an independently
 * guarded PL/pgSQL step.
 */
static void
TeccKeyAppendBootstrapStep(StringInfo replay, const char *command)
{
	appendStringInfo(replay,
					 "BEGIN EXECUTE %s; "
					 "EXCEPTION "
					 "WHEN duplicate_object THEN "
					 "  RAISE DEBUG 'tecc: key object already present, recipe step skipped'; "
					 "WHEN undefined_object THEN "
					 "  RAISE DEBUG 'tecc: key object already absent, recipe step skipped'; "
					 "END; ",
					 quote_literal_cstr(command));
}


/* ---------------------------------------------------------------------
 * 钩子安装
 * ---------------------------------------------------------------------
 */

/*
 * 安装点：InitPostgres()（见 src/backend/utils/init/postinit.c）。
 *
 * 必须挂在 utility hook 的 **postprocess** 段：本地 CREATE/ALTER/DROP KEK/DEK
 * 成功之后才谈得上传播；传播失败则整个语句报错回滚，不会留下"协调节点有、worker
 * 没有"的半途状态。
 */
static ProcessUtility_hook_type prev_tecc_key_utility_hook = NULL;

/*
 * 删除 KEK/DEK 时两件事一起做：
 *   1) 清掉它在 hg_tecc_ddl 里的"重建配方" —— 配方是"重建当前状态所需的语句"，
 *      对象没了就必须一起清，否则新节点补发会把已删的密钥又建回来；
 *   2) 从 pg_dist_object 里摘掉 —— 否则它仍是"应同步对象"，加节点时会照样尝试。
 *
 * 挂在 object_access_hook 的 OAT_DROP 而不是 DROP 语句的 postprocess：
 *   · 级联删除路径（DROP SCHEMA ... CASCADE、DROP OWNED）根本不会出现
 *     "DROP KEK" 这条语句，挂在语句上会漏；
 *   · 该钩子由 deleteOneObject 在真正删除**之前**调用，对象还在，可以用
 *     （classId, objectId）精确定位，不需要事后按名字 + search_path 猜测。
 *
 * 这两件事原来在 contrib/citus 的 CitusObjectAccessHook 里；搬到核心后插件那边
 * 不必再挂 object_access_hook（UnmarkObjectDistributed 换成下面这条等价的本地
 * SPI DELETE —— 它本来就只删本地一行，不触达 worker）。
 */
static object_access_hook_type prev_tecc_key_object_access_hook = NULL;

static void
TeccKeyUnmarkDistributed(Oid classId, Oid objectId)
{
	int			ret;

	if (SPI_connect() != SPI_OK_CONNECT)
	{
		return;
	}

	ret = SPI_execute("SELECT pg_catalog.to_regclass('pg_catalog.pg_dist_object') "
					  "IS NOT NULL", true, 1);
	if (ret == SPI_OK_SELECT && SPI_processed == 1)
	{
		bool		isnull = true;
		Datum		value = SPI_getbinval(SPI_tuptable->vals[0],
										  SPI_tuptable->tupdesc, 1, &isnull);

		if (!isnull && DatumGetBool(value))
		{
			Oid			paramTypes[2] = {OIDOID, OIDOID};
			Datum		paramValues[2];

			paramValues[0] = ObjectIdGetDatum(classId);
			paramValues[1] = ObjectIdGetDatum(objectId);

			(void) SPI_execute_with_args("DELETE FROM pg_catalog.pg_dist_object "
										 " WHERE classid = $1 AND objid = $2",
										 2, paramTypes, paramValues, NULL,
										 false, 0);
		}
	}

	SPI_finish();
}

static void
tecc_key_object_access(ObjectAccessType access, Oid classId, Oid objectId,
					   int subId, void *arg)
{
	if (prev_tecc_key_object_access_hook != NULL)
	{
		prev_tecc_key_object_access_hook(access, classId, objectId, subId, arg);
	}

	if (access == OAT_DROP &&
		(classId == HgKekRelationId || classId == HgDekRelationId))
	{
		TeccKeyRecipeForget(classId, objectId);
		TeccKeyUnmarkDistributed(classId, objectId);
	}
}

static void
tecc_key_propagation_ProcessUtility(PlannedStmt *pstmt,
									const char *queryString,
									bool readOnlyTree,
									ProcessUtilityContext context,
									ParamListInfo params,
									QueryEnvironment *queryEnv,
									DestReceiver *dest,
									QueryCompletion *qc)
{
	if (prev_tecc_key_utility_hook != NULL)
	{
		prev_tecc_key_utility_hook(pstmt, queryString, readOnlyTree, context,
								   params, queryEnv, dest, qc);
	}
	else
	{
		standard_ProcessUtility(pstmt, queryString, readOnlyTree, context,
								params, queryEnv, dest, qc);
	}

	if (pstmt != NULL)
	{
		TeccKeyPropagateIfNeeded(pstmt->utilityStmt, queryString,
								 pstmt->stmt_location, pstmt->stmt_len);
	}
}


void
tecc_install_key_propagation_hooks(void)
{
	prev_tecc_key_utility_hook = ProcessUtility_hook;
	ProcessUtility_hook = tecc_key_propagation_ProcessUtility;

	prev_tecc_key_object_access_hook = object_access_hook;
	object_access_hook = tecc_key_object_access;
}


/*
 * TeccKeyStmtTargetIdentity resolves the object a CREATE/ALTER KEK or CREATE DEK
 * statement targets, as the (namespace, name) pair the recipe is keyed by.
 * Returns false when the statement is not one of those / has no usable name.
 *
 * The namespace is derived the way the commands themselves derive it
 * (createkek.c / createdek.c): an explicit schema when the name is qualified,
 * otherwise the active creation namespace. Deriving it here, instead of looking
 * the new object up by name afterwards, is not just cheaper -- it is required: a
 * row inserted by the statement being processed is invisible to a scan in the
 * same command (cmin == curcid), so a post-hoc lookup of a freshly created KEK
 * or DEK would always come up empty.
 */
static bool
TeccKeyStmtTargetIdentity(Node *parsetree, Oid *catalogId, Oid *namespaceId,
						  char **objectName)
{
	List	   *names = TeccKeyStmtNames(parsetree);
	char	   *schemaName = NULL;
	char	   *bareName = NULL;

	*objectName = NULL;

	if (names == NIL)
	{
		return false;
	}

	if (IsA(parsetree, CreateDekStmt))
	{
		*catalogId = HgDekRelationId;
	}
	else if (IsA(parsetree, CreateKekStmt) || IsA(parsetree, AlterKekStmt))
	{
		*catalogId = HgKekRelationId;
	}
	else
	{
		return false;
	}

	DeconstructQualifiedName(names, &schemaName, &bareName);

	if (bareName == NULL)
	{
		return false;
	}

	/*
	 * DeconstructQualifiedName 返回的是**指向语法树里 String 节点内部的指针**
	 * （见 catalog/namespace.c:2858 的 strVal 用法），不是副本。本函数把名字交给
	 * 调用方记配方、还要在 address 回调里再取一次，因此必须复制一份：
	 * 直接 pfree 语法树内存会造成 use-after-free（实测表现为语法树里的
	 * names 被后来的分配内容顶替，报出 "KEK \"default\" does not exist"）。
	 */
	*objectName = pstrdup(bareName);

	if (schemaName != NULL)
	{
		Oid			explicitNamespace = LookupExplicitNamespace(schemaName, true);

		if (!OidIsValid(explicitNamespace))
		{
			pfree(*objectName);
			*objectName = NULL;
			return false;
		}

		*namespaceId = explicitNamespace;
	}
	else
	{
		*namespaceId = GetActiveCreationNamespace();

		if (!OidIsValid(*namespaceId))
		{
			pfree(*objectName);
			*objectName = NULL;
			return false;
		}
	}

	return true;
}


/*
 * TeccKeyOidByNameInNamespace resolves a KEK/DEK name inside one namespace.
 *
 * Deliberately scans with systable_beginscan(..., snapshot = NULL, ...): that
 * selects the *catalog* snapshot (access/index/genam.c:406), which
 * CommandCounterIncrement() invalidates -- and ProcessUtilitySlow performs such a
 * CCI before our postprocess runs (other/tcop/utility.c:1091), so the scan sees a
 * row inserted by the current command.
 *
 * The search_path-based helpers (get_kek_oid_ext / get_dek_oid_ext) must NOT be
 * used here: they scan with GetActiveSnapshot(), i.e. the *query* snapshot, which
 * no CCI refreshes, so right after CREATE KEK/DEK they cannot resolve the object
 * that this very command just created.
 */
static Oid
TeccKeyOidByNameInNamespace(Oid catalogId, Oid namespaceId, const char *objectName)
{
	Relation	rel;
	Oid			indexId;
	AttrNumber	nameAttNo;
	AttrNumber	namespaceAttNo;
	ScanKeyData	scanKey[2];
	SysScanDesc	scan;
	HeapTuple	tuple;
	Oid			objectId = InvalidOid;

	Assert(catalogId == HgKekRelationId || catalogId == HgDekRelationId);

	/*
	 * 两个 catalog 的 name 唯一索引都是 (name, namespace)，但**列号不同**：
	 * hg_kek 是 (oid, kek_name, key_namespace, …)，hg_dek 是
	 * (oid, dek_name, column_key_distributed_id, kek_id, key_namespace, …)
	 * ⇒ namespace 分别是 3 与 5。用错列号会让 systable_beginscan 报
	 * "column is not in index"（实测）。
	 */
	if (catalogId == HgKekRelationId)
	{
		indexId = HgKekKekNameIndexId;
		nameAttNo = Anum_hg_kek_kek_name;
		namespaceAttNo = Anum_hg_kek_key_namespace;
	}
	else
	{
		indexId = HgDekDekNameIndexId;
		nameAttNo = Anum_hg_dek_dek_name;
		namespaceAttNo = Anum_hg_dek_key_namespace;
	}

	rel = table_open(catalogId, AccessShareLock);

	ScanKeyInit(&scanKey[0], nameAttNo, BTEqualStrategyNumber,
				F_NAMEEQ, CStringGetDatum(objectName));
	ScanKeyInit(&scanKey[1], namespaceAttNo, BTEqualStrategyNumber,
				F_OIDEQ, ObjectIdGetDatum(namespaceId));

	scan = systable_beginscan(rel, indexId, true, NULL, 2, scanKey);

	tuple = systable_getnext(scan);

	if (HeapTupleIsValid(tuple))
	{
		if (catalogId == HgKekRelationId)
		{
			objectId = ((Form_hg_kek) GETSTRUCT(tuple))->oid;
		}
		else
		{
			objectId = ((Form_hg_dek) GETSTRUCT(tuple))->oid;
		}
	}

	systable_endscan(scan);
	table_close(rel, AccessShareLock);

	return objectId;
}


/*
 * TeccKeyStmtAddress returns the ObjectAddress of the KEK/DEK that a CREATE
 * KEK / ALTER KEK / CREATE DEK statement targets, or NIL when the statement is
 * not one of those / has no usable name.
 *
 * This is the `address` callback Citus's object framework calls to find out what
 * a statement acted on -- it is what lets CreateKekStmtObjectAddress satisfy
 * `markDistributed` (the ObjectAddress is what gets written into pg_dist_object).
 * It must therefore accept both a pre-execution and a post-execution call; the
 * identity is derived from the statement itself (not from a search_path lookup),
 * so it works either way.
 */
static List *
TeccKeyStmtAddress(Node *node, Oid expectedCatalogId, bool missing_ok)
{
	Oid			catalogId = InvalidOid;
	Oid			namespaceId = InvalidOid;
	char	   *objectName = NULL;
	Oid			objectId;
	ObjectAddress *address;

	if (!TeccKeyStmtTargetIdentity(node, &catalogId, &namespaceId, &objectName) ||
		catalogId != expectedCatalogId)
	{
		return NIL;
	}

	objectId = TeccKeyOidByNameInNamespace(catalogId, namespaceId, objectName);

	if (!OidIsValid(objectId))
	{
		if (!missing_ok)
		{
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_OBJECT),
					 errmsg("%s \"%s\" does not exist",
							catalogId == HgKekRelationId ? "KEK" : "DEK", objectName)));
		}

		pfree(objectName);
		return NIL;
	}

	pfree(objectName);

	address = palloc0(sizeof(ObjectAddress));
	ObjectAddressSet(*address, catalogId, objectId);

	return list_make1(address);
}


/*
 * CreateKekStmtObjectAddress / CreateDekStmtObjectAddress are the `address`
 * callbacks registered for CREATE KEK and CREATE DEK in Citus's object
 * framework (see distribute_object_ops.c).
 */
List *
CreateKekStmtObjectAddress(Node *node, bool missing_ok, bool isPostprocess)
{
	return TeccKeyStmtAddress(node, HgKekRelationId, missing_ok);
}


List *
CreateDekStmtObjectAddress(Node *node, bool missing_ok, bool isPostprocess)
{
	return TeccKeyStmtAddress(node, HgDekRelationId, missing_ok);
}


/*
 * TeccKeyStmtNames returns the (possibly schema-qualified) name list the
 * statement targets, or NIL when the statement has none.
 */
static List *
TeccKeyStmtNames(Node *parsetree)
{
	if (IsA(parsetree, CreateKekStmt))
	{
		return ((CreateKekStmt *) parsetree)->names;
	}

	if (IsA(parsetree, AlterKekStmt))
	{
		return ((AlterKekStmt *) parsetree)->names;
	}

	if (IsA(parsetree, CreateDekStmt))
	{
		return ((CreateDekStmt *) parsetree)->names;
	}

	return NIL;
}


/*
 * TeccKeyStatementCommands returns, in replay order, the statements that must be
 * sent to every worker for the TECC key statement currently executing: the
 * statement itself followed, for ALTER KEK, by the material write-back calls the
 * frontend appended to the same query string. Returns NIL when there is nothing
 * to replay.
 *
 * Besides returning them, each command is recorded as part of the recipe of the
 * object it belongs to (hg_tecc_ddl), which is what lets a worker added later
 * catch up. Recording is skipped for DROP: there the recipe rows are removed
 * instead, by the object_access_hook -- that hook also covers cascading deletions
 * (DROP SCHEMA ... CASCADE, DROP OWNED) which never produce a "DROP KEK"
 * statement of their own. See tecc_key_recipe.c.
 */
static List *
TeccKeyStatementCommands(Node *parsetree, const char *queryString,
						 int stmt_location, int stmt_len)
{
	List	   *commands = NIL;
	List	   *seqTracker = NIL;
	char	   *stmtText = NULL;
	char	   *objectName = NULL;
	Oid			catalogId = InvalidOid;
	Oid			namespaceId = InvalidOid;

	if (IsA(parsetree, DropStmt))
	{
		stmtText = TeccKeyStmtText(queryString, stmt_location, stmt_len);

		if (stmtText != NULL)
		{
			commands = lappend(commands, stmtText);
		}

		return commands;
	}

	stmtText = TeccKeyStmtText(queryString, stmt_location, stmt_len);

	if (stmtText != NULL)
	{
		commands = lappend(commands, stmtText);

		if (TeccKeyStmtTargetIdentity(parsetree, &catalogId, &namespaceId,
									  &objectName))
		{
			TeccKeyRecipeAppend(catalogId, namespaceId, objectName, stmtText,
								&seqTracker);

			/*
			 * 方案 B（hgdde 源码承接）：CREATE KEK / CREATE DEK 把 create 命令登记进
			 * 集群通用对象 create provider（hgdde.distributed_object_create_ddl），
			 * 供后续 `hgdde_add_node` 的新节点依赖回放（建壳）之前自动补发 KEK/DEK。
			 * ALTER KEK 由配方（hg_tecc_ddl）承载，不在这里登记。
			 */
			if (IsA(parsetree, CreateKekStmt) || IsA(parsetree, CreateDekStmt))
			{
				if (catalogId == HgKekRelationId)
				{
					TeccKeyProviderUpsert("key encryption key", objectName, stmtText);
				}
				else if (catalogId == HgDekRelationId)
				{
					TeccKeyProviderUpsert("data encryption key", objectName, stmtText);
				}
			}

			pfree(objectName);
		}
	}

	/* ALTER KEK 的材料写回调用（各自登记到自己所属的对象上） */
	commands = list_concat(commands,
						   TeccKeyTrailingRotateCommands(queryString,
														 stmt_location, stmt_len,
														 &seqTracker));

	return commands;
}


/*
 * TeccKeyTrailingRotateCommands returns the rewritten text of the material
 * write-back calls that follow the current statement in the same query string.
 * Returns NIL when there is no tail or the tail holds none of them.
 *
 * Why the tail is where they live: the frontend delivers ALTER KEK and its
 * write-backs as ONE query string (ALTER first, then one
 * "SELECT hg_kek_rotate_*_arg(...);" per rewritten argument), which the server
 * splits into one parse tree per statement. Those are plain SELECTs, so they
 * never reach the utility hook -- Citus only sees utility statements there
 * (ProcessUtility_hook), while a SELECT goes to the planner/executor. Reading
 * them back out of the tail of the same query string is the only way to pick
 * them up, and it is safe: the whole string was already parsed by
 * exec_simple_query before any statement of it ran, so re-parsing the tail
 * cannot introduce a new syntax error.
 *
 * stmt_len excludes the trailing semicolon (updateRawStmtEnd in gram.y), so the
 * tail begins at stmt_len and may start with that semicolon; leading separators
 * and whitespace are skipped. A non-positive stmt_len means the statement has no
 * tail (it is the last one), which is why that case returns NIL rather than
 * "to the end of the string".
 */
static List *
TeccKeyTrailingRotateCommands(const char *queryString, int stmt_location,
							  int stmt_len, List **seqTracker)
{
	List	   *commands = NIL;
	const char *statementStart;
	const char *tailStart;
	char	   *tailText;
	List	   *tailParseTrees;
	ListCell   *lc;

	if (queryString == NULL || stmt_location < 0 || stmt_len <= 0 ||
		(size_t) stmt_location > strlen(queryString))
	{
		return NIL;
	}

	statementStart = queryString + stmt_location;

	if ((size_t) stmt_len >= strlen(statementStart))
	{
		return NIL;
	}

	tailStart = statementStart + stmt_len;

	while (*tailStart == ';' || *tailStart == '\n' || *tailStart == '\r' ||
		   *tailStart == ' ' || *tailStart == '\t')
	{
		tailStart++;
	}

	if (*tailStart == '\0')
	{
		return NIL;
	}

	tailText = pstrdup(tailStart);
	tailParseTrees = pg_parse_query(tailText);

	foreach(lc, tailParseTrees)
	{
		RawStmt    *rawStmt = lfirst_node(RawStmt, lc);
		Oid			objectClass = InvalidOid;
		Oid			namespaceId = InvalidOid;
		char	   *objectName = NULL;
		char	   *rotateText;

		if (!OidIsValid(TeccKeyRotateCallCatalog(rawStmt->stmt)))
		{
			continue;
		}

		rotateText = TeccKeyRotationStmtText(rawStmt->stmt, tailText,
											 rawStmt->stmt_location,
											 rawStmt->stmt_len,
											 &objectClass, &namespaceId,
											 &objectName);
		if (rotateText != NULL)
		{
			commands = lappend(commands, rotateText);

			if (objectName != NULL)
			{
				TeccKeyRecipeAppend(objectClass, namespaceId, objectName,
									rotateText, seqTracker);
				pfree(objectName);
			}
		}
	}

	return commands;
}


/*
 * TeccKeyRotateCallCatalog returns the catalog (hg_dek / hg_kek) that holds the
 * object addressed by the first argument of an ALTER KEK material write-back
 * call, or InvalidOid when the statement is not one of those calls.
 *
 * The frontend appends these calls right after an ALTER KEK (see the file
 * header); they are plain "SELECT func(...)" statements, so IsTeccKeyStmt does
 * not match them and this predicate is what brings them into the propagation
 * channel.  The shape is checked strictly -- anything unexpected must fall back
 * to "not our statement" rather than be rewritten on a guess.
 */
static Oid
TeccKeyRotateCallCatalog(Node *parsetree)
{
	SelectStmt *selectStmt;
	ResTarget  *target;
	FuncCall   *funcCall;
	const char *funcName;

	if (parsetree == NULL || !IsA(parsetree, SelectStmt))
	{
		return InvalidOid;
	}

	selectStmt = castNode(SelectStmt, parsetree);

	/* 只接受光秃秃的 "SELECT f(...)"：带任何附加子句都不是前端的发射形态 */
	if (selectStmt->op != SETOP_NONE ||
		list_length(selectStmt->targetList) != 1 ||
		selectStmt->fromClause != NIL ||
		selectStmt->whereClause != NULL ||
		selectStmt->groupClause != NIL ||
		selectStmt->havingClause != NULL ||
		selectStmt->windowClause != NIL ||
		selectStmt->sortClause != NIL ||
		selectStmt->limitOffset != NULL ||
		selectStmt->limitCount != NULL ||
		selectStmt->lockingClause != NIL ||
		selectStmt->withClause != NULL ||
		selectStmt->valuesLists != NIL)
	{
		return InvalidOid;
	}

	target = linitial_node(ResTarget, selectStmt->targetList);

	if (target->val == NULL || !IsA(target->val, FuncCall))
	{
		return InvalidOid;
	}

	funcCall = castNode(FuncCall, target->val);

	if (funcCall->funcname == NIL ||
		funcCall->agg_order != NIL ||
		funcCall->agg_filter != NULL ||
		funcCall->over != NULL)
	{
		return InvalidOid;
	}

	/*
	 * 只认 pg_catalog 下的同名函数。用户若在自己的模式里造一个同名函数，它的
	 * 实参语义与本模块的假设无关，不能按 DEK/KEK 去解析它的第一个实参。
	 */
	if (list_length(funcCall->funcname) > 1 &&
		strcmp(strVal(linitial(funcCall->funcname)), "pg_catalog") != 0)
	{
		return InvalidOid;
	}

	funcName = strVal(llast(funcCall->funcname));

	if (strcmp(funcName, TECC_ROTATE_DEK_ARG_FUNC) == 0)
	{
		return HgDekRelationId;
	}

	if (strcmp(funcName, TECC_ROTATE_KEK_ARG_FUNC) == 0)
	{
		return HgKekRelationId;
	}

	return InvalidOid;
}


/*
 * TeccKeyRotationShapeError raises for a write-back call we recognised but
 * cannot safely rewrite.
 *
 * Failing closed is deliberate: silently skipping the statement would leave the
 * workers holding key material that diverges from the coordinator -- exactly the
 * failure mode this module exists to remove.  The caller's transaction (which
 * already executed the local ALTER KEK) rolls back, so no half-rotated state
 * survives.
 */
static void
TeccKeyRotationShapeError(const char *detail)
{
	ereport(ERROR,
			(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
			 errmsg("cannot propagate TECC key material write-back statement"),
			 errdetail("%s", detail),
			 errhint("This statement is issued by the TECC frontend as part of "
					 "ALTER KEK and is replayed verbatim on the worker nodes, so "
					 "it must keep its emitted shape (a bare schema-qualified "
					 "object name is looked up from the decimal OID literal).")));
}


/*
 * TeccKeyRotationStmtText rewrites one ALTER KEK material write-back call so it
 * can be replayed on another node, and returns the rewritten text.  Returns
 * NULL when the statement is not such a call.
 *
 * Why a rewrite is needed: the call's first argument is the *local* OID of the
 * DEK/KEK as the coordinator sees it (the frontend learned it from the catalog),
 * and OIDs are node-local -- CREATE KEK/DEK replay allocates a fresh OID on
 * every node (createkek.c / createdek.c both call GetNewOidWithIndex).  So the
 * worker has the same *object* under a different OID, and replaying the call
 * verbatim would address an unrelated object or none at all.  The write-back
 * functions fail closed on a missing OID, so the outcome would be a hard error
 * rather than corruption -- but the rotated material would never arrive.
 *
 * The rewrite replaces that OID with a lookup each node resolves against its own
 * catalog:
 *
 *   (SELECT o.oid FROM pg_catalog.hg_dek o
 *     JOIN pg_catalog.pg_namespace n ON n.oid = o.key_namespace
 *    WHERE o.dek_name = '<name>' AND n.nspname = '<schema>')
 *
 * Name and schema are portable because the same CREATE KEK/DEK text is what
 * produced the object on every node (and both catalogs carry a unique index on
 * (name, namespace), so the lookup is unambiguous).  The remaining arguments are
 * carried over byte for byte, which matters because the third one is a quoted
 * literal whose exact spelling (text hex vs. E'\\x<hex>' bytea) must survive
 * without any re-escaping.
 *
 * The span of the OID literal comes from the parse tree (A_Const.location), not
 * from text pattern matching.
 *
 * On success *objectClass / *namespaceId / *objectName identify the object the
 * call addresses (so the caller can attribute the rewritten command to that
 * object's recipe); *objectName is palloc'd in the caller's context.
 */
static char *
TeccKeyRotationStmtText(Node *parsetree, const char *queryString,
						int stmt_location, int stmt_len,
						Oid *objectClass, Oid *namespaceId, char **objectName)
{
	Oid				catalogId = TeccKeyRotateCallCatalog(parsetree);
	SelectStmt	   *selectStmt;
	FuncCall	   *funcCall;
	Node		   *firstArgument;
	int				argumentLocation;
	const char	   *statementStart;
	size_t			statementLength;
	const char	   *literalStart;
	const char	   *literalEnd;
	unsigned long	objectOid;
	Oid				resolvedNamespaceId = InvalidOid;
	char		   *resolvedObjectName = NULL;
	char		   *schemaName = NULL;
	const char	   *catalogName = NULL;
	const char	   *nameColumn = NULL;
	StringInfoData	buf;

	*objectClass = InvalidOid;
	*namespaceId = InvalidOid;
	*objectName = NULL;

	if (!OidIsValid(catalogId))
	{
		return NULL;
	}

	selectStmt = castNode(SelectStmt, parsetree);
	funcCall = castNode(FuncCall,
						linitial_node(ResTarget, selectStmt->targetList)->val);

	if (list_length(funcCall->args) != 3)
	{
		TeccKeyRotationShapeError("expected exactly three arguments "
								  "(object OID, argument name, new value)");
		return NULL;			/* keep the compiler happy */
	}

	firstArgument = (Node *) linitial(funcCall->args);

	if (!IsA(firstArgument, A_Const))
	{
		TeccKeyRotationShapeError("the first argument is not an OID literal");
		return NULL;
	}

	argumentLocation = ((A_Const *) firstArgument)->location;

	if (queryString == NULL || stmt_location < 0 ||
		(size_t) stmt_location > strlen(queryString))
	{
		TeccKeyRotationShapeError("the statement text is unavailable");
		return NULL;
	}

	statementStart = queryString + stmt_location;
	statementLength = (stmt_len > 0) ? (size_t) stmt_len : strlen(statementStart);

	if (argumentLocation < stmt_location ||
		(size_t) (argumentLocation - stmt_location) >= statementLength)
	{
		TeccKeyRotationShapeError("the OID literal lies outside the statement");
		return NULL;
	}

	/*
	 * 前端用 %u 发射这个 OID，即裸十进制字面量。这里只扫描数字：扫不到数字就
	 * 说明形态已变（例如带引号或带类型转换），一律 fail-closed。
	 */
	literalStart = queryString + argumentLocation;
	literalEnd = literalStart;

	while (*literalEnd >= '0' && *literalEnd <= '9')
	{
		literalEnd++;
	}

	if (literalEnd == literalStart)
	{
		TeccKeyRotationShapeError("the first argument is not a decimal OID literal");
		return NULL;
	}

	objectOid = strtoul(literalStart, NULL, 10);

	if (catalogId == HgDekRelationId)
	{
		catalogName = "hg_dek";
		nameColumn = "dek_name";
	}
	else
	{
		catalogName = "hg_kek";
		nameColumn = "kek_name";
	}

	if (!TeccKeyObjectIdentity(catalogId, (Oid) objectOid, &resolvedNamespaceId,
							   &resolvedObjectName))
	{
		TeccKeyRotationShapeError("the addressed key object does not exist locally");
		return NULL;
	}

	schemaName = get_namespace_name(resolvedNamespaceId);

	if (schemaName == NULL)
	{
		TeccKeyRotationShapeError("the addressed key object has no namespace");
		return NULL;
	}

	*objectClass = catalogId;
	*namespaceId = resolvedNamespaceId;
	*objectName = resolvedObjectName;

	initStringInfo(&buf);

	/* 前半段：语句开头到 OID 字面量之前，逐字节原样 */
	appendBinaryStringInfo(&buf, statementStart, literalStart - statementStart);

	/* OID 字面量替换为"按 (名字, 模式) 解析本节点 OID"的子查询 */
	appendStringInfo(&buf,
					 "(SELECT o.oid FROM pg_catalog.%s o "
					 "JOIN pg_catalog.pg_namespace n ON n.oid = o.key_namespace "
					 "WHERE o.%s = %s AND n.nspname = %s)",
					 catalogName, nameColumn,
					 quote_literal_cstr(resolvedObjectName),
					 quote_literal_cstr(schemaName));

	/* 后半段：OID 字面量之后到语句结尾，逐字节原样（材料的原始写法就靠这里保住） */
	appendBinaryStringInfo(&buf, literalEnd,
						   statementStart + statementLength - literalEnd);

	/* resolvedObjectName 的所有权已交给调用方（用于登记配方），此处只释放模式名 */
	pfree(schemaName);

	return buf.data;
}
