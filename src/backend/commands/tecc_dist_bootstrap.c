/*-------------------------------------------------------------------------
 *
 * tecc_dist_bootstrap.c
 *	  Catch up the rest of the cluster after a topology change.
 *
 * ReqID:TECC4.0
 *
 * 要解决的问题：分布式插件的"新节点加入"与"表刚被分发"这两条路径，会把**它自己
 * 认识的对象**（表、角色、模式、类型…）在目标节点上重建出来，但 TECC 的两类事实
 * 它不认识：
 *
 *   ① KEK/DEK —— 密钥元数据在 hg_kek/hg_dek 里，密钥材料只存在于 hg_tecc_ddl 记录的
 *      "已处理语句原文"里。新节点没有它，上面任何加密分片 DDL 都会失败；
 *   ② 表/列 sysmac 标记 —— 存在 pg_seclabel 里，不属于 pg_dist_* 元数据。新节点没有它，
 *      表级/列级强制访问检查会按"标记未设置"静默放行（fail-open）。
 *
 * 为什么不能沿用插件通道：标记这件事原本靠"在重建对象的 DDL 里附加 SECURITY
 * LABEL"（dependencies.c + metadata_sync.c）实现，需要改插件源码。按与 hgdde 团队的
 * 约定，hgdde 不提供 C 层 API，所以必须改到 SQL 层 —— 于是这里在**触发语句成功
 * 执行之后**主动补发一遍。
 *
 * 分工（重要，改的时候别把两边搞反）：
 *   · KEK/DEK 的**主通道**在插件里（把密钥登记为分布式对象，插件在 add_node 内部、
 *     建壳表**之前**重放配方）。本文件只在**插件管不了**时才补发 —— 即集群里有密钥、
 *     却因为没有登记而不会被插件重放的情形；登记了还补发会与插件并发重放，实测把
 *     add_node 挂死（见 tecc_bootstrap_run 的说明）。
 *   · 表/列标记**只有**本文件这一条通道（插件不认识它们）。
 *
 * 触发方式（为什么不改插件）：两件事在 SQL 层都表现为**一次普通函数调用**
 * （`citus_add_node(...)`、`create_distributed_table(...)`），不经过 utility hook。
 * 因此用 post_parse_analyze_hook 认出"这条查询里调用了这些函数"，再在
 * ExecutorEnd（顶层语句真正跑完之后、事务仍在）里补发 —— 只有这样做才能保证
 * "对象已经在目标节点上建好、再补标记/密钥"，顺序不会反。
 *
 * 幂等性：补发脚本自身是幂等的（密钥配方逐条容忍 already exists、标记是覆盖写），
 * 因此不需要"哪些节点是新的"这类信息，也就不必依赖插件的内部节点状态。
 *
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd.
 * Author: <wangxp@highgo.com>
 *
 * IDENTIFICATION
 *	  src/backend/commands/tecc_dist_bootstrap.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/genam.h"
#include "access/htup_details.h"
#include "access/table.h"
#include "catalog/namespace.h"
#include "catalog/pg_namespace.h"
#include "catalog/pg_proc.h"
#include "executor/executor.h"
#include "miscadmin.h"
#include "nodes/nodeFuncs.h"
#include "nodes/pg_list.h"
#include "parser/analyze.h"
#include "utils/lsyscache.h"
#include "utils/fmgroids.h"
#include "utils/rel.h"
#include "utils/snapmgr.h"
#include "access/xact.h"

#include "commands/tecc_dist_bootstrap.h"
#include "commands/tecc_dist_replay.h"
#include "commands/tecc_key_propagation.h"
#include "commands/tecc_mac_distribution.h"

/* 补发的内容（按"要补的东西"分档；NODE_JOIN 是 DISTRIBUTE 的超集） */
typedef enum TeccBootstrapKind
{
	TECC_BOOTSTRAP_NONE = 0,
	TECC_BOOTSTRAP_DISTRIBUTE,	/* 表刚被分发：只需补标记 */
	TECC_BOOTSTRAP_NODE_JOIN	/* 节点刚加入：密钥 + 标记 */
} TeccBootstrapKind;

/*
 * 触发函数。两组的区别只是"要补什么"。
 *
 * 节点相关的几个名字是同一件事的不同入口：citus_add_node / citus_activate_node 是
 * Citus 9.5 之后的名字，master_* 是旧名（仍保留）。只按名字解析、不按签名，因此
 * 参数个数的演化（nodegroup/noderole/nodecluster 默认值）不影响识别。
 */
static const char *const tecc_bootstrap_node_join_functions[] = {
	"citus_add_node",
	"citus_add_inactive_node",
	"citus_activate_node",
	"hgdde_add_node",
	"hgdde_add_inactive_node",
	"hgdde_activate_node",
	"master_add_node",
	"master_add_inactive_node",
	"master_activate_node",
	NULL
};

static const char *const tecc_bootstrap_distribute_functions[] = {
	"create_distributed_table",
	"create_distributed_table_concurrently",
	"create_reference_table",
	"create_citus_local_table",
	"create_hgdde_local_table",
	"master_create_distributed_table",
	NULL
};

#define TECC_BOOTSTRAP_MAX_FUNCS	32

static post_parse_analyze_hook_type prev_tecc_bootstrap_analyze_hook = NULL;
static ExecutorStart_hook_type prev_tecc_bootstrap_executor_start = NULL;
static ExecutorEnd_hook_type prev_tecc_bootstrap_executor_end = NULL;

/* 本查询是否触发了补发（post_parse_analyze 判定、ExecutorStart 消费） */
static TeccBootstrapKind tecc_bootstrap_detected = TECC_BOOTSTRAP_NONE;

/* 顶层语句的待办（失败的语句走不到 ExecutorEnd，故下一轮开头无条件覆盖） */
static TeccBootstrapKind tecc_bootstrap_pending = TECC_BOOTSTRAP_NONE;

/* 上一句是"加节点"：下一句再补一次标记同步（见 ExecutorStart 的说明） */
static bool tecc_bootstrap_sync_pending = false;

/*
 * NODE_JOIN 需要在事务 pre-commit 处再补一次 MAC 标记。
 *
 * 为什么不能只靠 ExecutorEnd 里的 TeccMacSyncLabels()：分布式插件把 psql 里的
 * 顶层调用包在带连接设置的包装语句里（本集群实测：`SELECT pg_catalog.current_setting(
 * 'application_name')` 出现在 `SELECT hgdde_add_node(...)` 的执行链里），导致
 * ExecutorEnd 在 hgdde 内部 SPI 的最外层深度 0 执行器上触发 —— 那一刻新节点还
 * 没有 INSERT 进 pg_dist_node，run_command_on_workers 只把推送发给**已登记**的
 * 节点，新节点于是没有 hg_sysmac_labels（2026-11-05 起 w10/w11 复现两次）。
 * pre-commit 在 commit 记录写入之前执行、事务仍然打开 —— 此时所有执行器都结束、
 * pg_dist_node 的新行在本事务的快照里可见，是对新节点补发标记的可靠时机。
 * 失败不能回滚（/ 刷屏），所以只 WARNING。
 */
static bool tecc_bootstrap_mac_commit_pending = false;

static void tecc_bootstrap_xact_callback(XactEvent event, void *arg);

/* 执行器嵌套层数：只有回到 0（顶层语句结束）才补发 */
static int	tecc_bootstrap_executor_depth = 0;

/* 补发进行中：防止它自己发出的 SPI 语句再次触发补发 */
static bool tecc_bootstrap_running = false;

static void tecc_bootstrap_post_parse_analyze(ParseState *pstate, Query *query,
											 JumbleState *jstate);
static void tecc_bootstrap_ExecutorStart(QueryDesc *queryDesc, int eflags);
static void tecc_bootstrap_ExecutorEnd(QueryDesc *queryDesc);
static bool tecc_bootstrap_walker(Node *node, void *context);
static bool tecc_bootstrap_funcs_available(void);
static void tecc_bootstrap_resolve_functions(void);
static TeccBootstrapKind tecc_bootstrap_classify(Oid funcid);
static void tecc_bootstrap_run(TeccBootstrapKind kind);

/*
 * 触发函数的 OID 缓存。全部解析不到时每次都重试（扩展可能在会话中途才 CREATE）。
 */
static Oid	tecc_bootstrap_func_oids[TECC_BOOTSTRAP_MAX_FUNCS];
static TeccBootstrapKind tecc_bootstrap_func_kinds[TECC_BOOTSTRAP_MAX_FUNCS];
static int	tecc_bootstrap_func_count = 0;


/*
 * tecc_install_dist_bootstrap_hooks
 *		安装三个钩子。必须链到已有的钩子（Citus 在 shared_preload 阶段装过
 *		post_parse_analyze 的是它自己的；本库里 tecc_dist_guard.c 也装了一个）。
 */
void
tecc_install_dist_bootstrap_hooks(void)
{
	prev_tecc_bootstrap_analyze_hook = post_parse_analyze_hook;
	post_parse_analyze_hook = tecc_bootstrap_post_parse_analyze;

	prev_tecc_bootstrap_executor_start = ExecutorStart_hook;
	ExecutorStart_hook = tecc_bootstrap_ExecutorStart;

	prev_tecc_bootstrap_executor_end = ExecutorEnd_hook;
	ExecutorEnd_hook = tecc_bootstrap_ExecutorEnd;

	RegisterXactCallback(tecc_bootstrap_xact_callback, NULL);
}


/*
 * tecc_bootstrap_xact_callback
 *		事务结束时复位所有"当前语句"状态。
 *
 * 为什么必须有：executor_depth 是"顶层语句"的判据，而语句**出错**时
 * ExecutorStart 已经加过、配对的 ExecutorEnd 永远不会来 —— 计数就此泄漏，之后
 * 这个后端里的钩子会一直以为"还在执行嵌套语句"，从此再不补发（实测：MAC 用例里
 * 先故意触发几次权限错误，后面加节点就再也不触发同步了）。
 *
 * sync_pending 刻意不复位：它的语义正是"跨事务再补一次"。
 */
static void
tecc_bootstrap_xact_callback(XactEvent event, void *arg)
{
	switch (event)
	{
		case XACT_EVENT_PRE_COMMIT:
		case XACT_EVENT_PARALLEL_PRE_COMMIT:
			/* 新节点现在已在 pg_dist_node 中（同一事务内），可以安全补发标记 */
			if (tecc_bootstrap_mac_commit_pending)
			{
				tecc_bootstrap_mac_commit_pending = false;

				PG_TRY();
				{
					bool		pushed = false;

					CommandCounterIncrement();

					if (!ActiveSnapshotSet())
					{
						PushActiveSnapshot(GetTransactionSnapshot());
						pushed = true;
					}

					if (TeccDistIsCoordinator() &&
						tecc_mac_sync_labels_hook != NULL)
					{
						tecc_mac_sync_labels_hook();
					}

					if (pushed)
					{
						PopActiveSnapshot();
					}
				}
				PG_CATCH();
				{
					FlushErrorState();

					if (ActiveSnapshotSet())
					{
						PopActiveSnapshot();
					}

					elog(WARNING, "tecc: could not synchronize mandatory "
						 "access control labels for a newly added node");
				}
				PG_END_TRY();
			}
			break;
		case XACT_EVENT_COMMIT:
		case XACT_EVENT_PARALLEL_COMMIT:
		case XACT_EVENT_ABORT:
		case XACT_EVENT_PARALLEL_ABORT:
		case XACT_EVENT_PREPARE:
			tecc_bootstrap_mac_commit_pending = false;
			tecc_bootstrap_detected = TECC_BOOTSTRAP_NONE;
			tecc_bootstrap_pending = TECC_BOOTSTRAP_NONE;
			tecc_bootstrap_executor_depth = 0;
			tecc_bootstrap_running = false;
			break;
		default:
			break;
	}
}


/*
 * tecc_bootstrap_post_parse_analyze
 *		认出"这条查询调用了拓扑变化的入口函数"。
 *
 * 只看普通 SELECT：这些函数都通过 SELECT 调用，utility 语句不会命中（而且
 * create_distributed_table 之类的 SQL 包装函数的实现本身也可能被别的语句间接调用，
 * 那种情况下它自己的 SPI 查询会各走一遍本函数，判定照样成立）。
 */
static void
tecc_bootstrap_post_parse_analyze(ParseState *pstate, Query *query,
								 JumbleState *jstate)
{
	TeccBootstrapKind kind = TECC_BOOTSTRAP_NONE;

	if (prev_tecc_bootstrap_analyze_hook != NULL)
	{
		prev_tecc_bootstrap_analyze_hook(pstate, query, jstate);
	}

	if (query == NULL || query->utilityStmt != NULL ||
		query->commandType != CMD_SELECT)
	{
		return;
	}

	if (!tecc_bootstrap_funcs_available())
	{
		return;
	}

	(void) query_tree_walker(query, tecc_bootstrap_walker, (void *) &kind, 0);

	if (kind != TECC_BOOTSTRAP_NONE)
	{
		tecc_bootstrap_detected = kind;
		elog(DEBUG1, "tecc: topology change detected (kind %d)", (int) kind);
	}
}


/*
 * tecc_bootstrap_walker
 *		在表达式树里找触发函数。
 *
 * 命中即返回 true 中断遍历 —— 同一棵查询树里出现多个触发函数时，先命中的那个
 * 就够了：节点的加入与分发都在同一条语句里发生时，NODE_JOIN 档已经包含标记补发。
 */
static bool
tecc_bootstrap_walker(Node *node, void *context)
{
	TeccBootstrapKind *kind = (TeccBootstrapKind *) context;

	if (node == NULL)
	{
		return false;
	}

	if (IsA(node, FuncExpr))
	{
		FuncExpr   *funcExpr = castNode(FuncExpr, node);
		TeccBootstrapKind found = tecc_bootstrap_classify(funcExpr->funcid);

		if (found != TECC_BOOTSTRAP_NONE)
		{
			if (found > *kind)
			{
				*kind = found;
			}
			return true;
		}
	}

	return expression_tree_walker(node, tecc_bootstrap_walker, context);
}


/*
 * tecc_bootstrap_ExecutorStart
 *		顶层语句开始时把"本查询的检测结果"落实成待办。
 *
 * 一定要在顶层覆盖而不是简单置位：带 ON_ERROR 的语句失败时不会走 ExecutorEnd，
 * 残留的待办会在下一条毫不相干的查询结束时误触发补发。
 */
static void
tecc_bootstrap_ExecutorStart(QueryDesc *queryDesc, int eflags)
{
	if (tecc_bootstrap_executor_depth == 0 && !tecc_bootstrap_running)
	{
		if (tecc_bootstrap_detected != TECC_BOOTSTRAP_NONE)
		{
			tecc_bootstrap_pending = tecc_bootstrap_detected;
		}
		else if (tecc_bootstrap_sync_pending)
		{
			/*
			 * 上一句刚加过节点：那一句结束时做的同步可能"看不见"新节点 ——
			 * 分布式插件把 pg_dist_node 缓存在后端里，新加入的节点要等事务提交后
			 * 才出现在 run_command_on_workers 的目标列表里（实测：085 的新节点在
			 * 加节点那一刻没收到标记表，下一句才收到）。所以再补一次，此时新节点
			 * 一定已经可枚举。
			 */
			tecc_bootstrap_pending = TECC_BOOTSTRAP_DISTRIBUTE;
			tecc_bootstrap_sync_pending = false;
		}
	}

	tecc_bootstrap_detected = TECC_BOOTSTRAP_NONE;
	tecc_bootstrap_executor_depth++;

	if (prev_tecc_bootstrap_executor_start != NULL)
	{
		prev_tecc_bootstrap_executor_start(queryDesc, eflags);
	}
	else
	{
		standard_ExecutorStart(queryDesc, eflags);
	}
}


/*
 * tecc_bootstrap_ExecutorEnd
 *		顶层语句跑完之后补发。
 *
 * 位置的两点考量：
 *   · 必须在**前一个** ExecutorEnd 之后：那时目标节点上的对象才真正建好；
 *   · 必须还在事务里：补发失败要能把整条语句（含刚加入的节点、刚分发的表）一起
 *     回滚，而不是留下一个"节点在、密钥/标记不在"的现场。
 */
static void
tecc_bootstrap_ExecutorEnd(QueryDesc *queryDesc)
{
	if (prev_tecc_bootstrap_executor_end != NULL)
	{
		prev_tecc_bootstrap_executor_end(queryDesc);
	}
	else
	{
		standard_ExecutorEnd(queryDesc);
	}

	if (tecc_bootstrap_executor_depth > 0)
	{
		tecc_bootstrap_executor_depth--;
	}

	if (tecc_bootstrap_executor_depth == 0 &&
		tecc_bootstrap_pending != TECC_BOOTSTRAP_NONE &&
		!tecc_bootstrap_running)
	{
		TeccBootstrapKind kind = tecc_bootstrap_pending;


		tecc_bootstrap_pending = TECC_BOOTSTRAP_NONE;

		tecc_bootstrap_run(kind);
	}
}


/*
 * tecc_bootstrap_run
 *		真正补发。
 *
 * NODE_JOIN 档先补密钥再补标记：标记只依赖表存在，而密钥是加密分片 DDL 的前提，
 * 顺序反过来会让新节点在"标记已经打上、密钥还没有"的中间态下承载失败的加密 DDL。
 */
static void
tecc_bootstrap_run(TeccBootstrapKind kind)
{
	bool		pushedSnapshot = false;

	tecc_bootstrap_running = true;

	/*
	 * 先把刚执行完的那条语句的改动推到可见：补发要读的正是它刚写下的东西
	 * （pg_dist_partition 里新分发的那一行、hg_tecc_ddl 里新记的配方）。不推这一下，
	 * 本命令自己插入的行在快照里还看不见 —— 实测：刚分发完的表**不在**重放清单里，
	 * 标记于是永远到不了 worker。
	 */
	CommandCounterIncrement();

	/*
	 * 取一个"能看到刚才那次 CCI"的快照。三种情况：
	 *   · 没有活动快照（ExecutorEnd 在 PortalDrop 里跑，Portal 自己的快照已经弹掉、
	 *     ActivePortal 也恢复成 NULL，此时任何 SPI 都会以
	 *     "cannot execute SQL without an outer snapshot or portal" 失败）→ 压一个；
	 *   · 有活动快照且是"私有"的（未被登记）→ 就地推进它的 command id；
	 *   · 有活动快照但**被登记过**（插件在自己的 DDL 里 RegisterSnapshot 了事务快照，
	 *     例如 CREATE EXTENSION 里的 create_reference_table）→ 不能就地改
	 *     （UpdateActiveSnapshotCommandId 会断言 regd_count == 0，实测在
	 *     CREATE EXTENSION conn_restrict 上直接 abort），改为压一份拷贝上去。
	 */
	if (!ActiveSnapshotSet())
	{
		PushActiveSnapshot(GetTransactionSnapshot());
		pushedSnapshot = true;
	}
	else if (GetActiveSnapshot()->regd_count == 0)
	{
		UpdateActiveSnapshotCommandId();
	}
	else
	{
		PushCopiedSnapshot(GetTransactionSnapshot());
		pushedSnapshot = true;
	}

	PG_TRY();
	{
		/*
		 * 先密钥、再表/列标记（顺序理由见本函数开头的说明）。
		 *
		 * 密钥这一档是**补强**，只在插件管不了这件事时才出手：插件把 KEK/DEK 登记为
		 * 分布式对象时，它会在 add_node **内部**、建壳表之前重放配方，此时我们再补发
		 * 一遍会与它并发落在新节点的同一张密钥表上 —— 实测把 citus_add_node 挂死。
		 * "登记与否"用 pg_dist_object 判（TeccKeyRegisteredAsDistributed），不靠猜。
		 *
		 * 补的是这个缺口：集群里只有密钥、还没有任何密态分布式表时加节点，插件没有
		 * 任何壳表要建，也就不需要重放任何对象，密钥于是不会顺带送达，新节点会
		 * **静默地**缺密钥，直到以后第一次建加密分布式表才炸（故障点与原因相隔很远）。
		 *
		 * 它救不了"已有密态分布式表"那种情形：本函数跑在语句成功之后，而那时依赖
		 * DEK 的 CREATE TABLE ... ENCRYPTED WITH 早就执行过（实测新节点缺 DEK →
		 * 建壳表失败 → 整个 add_node 回滚）。
		 *
		 * 幂等与失败语义：配方重放对每条语句独立容忍 already exists / already absent
		 * （见 TeccKeyReplayAllOnWorkers），已有节点不受影响；真正的失败会原样抛错，
		 * 连带把刚加入的节点一起回滚 —— 不留半途状态。
		 *
		 * 表/列标记不受上面的顺序限制：它们的载体 public.hg_sysmac_labels 是普通表，
		 * 每个节点早就有一份，推送不依赖任何分布式对象。重算并整体推给每个 worker ——
		 * 表刚被分发时把分发前打的标记补登进来，新节点加入时顺带自愈历史遗留。
		 */
		if (kind == TECC_BOOTSTRAP_NODE_JOIN &&
			!TeccKeyRegisteredAsDistributed())
		{
			TeccKeyReplayAllOnWorkers();
		}

		if (tecc_mac_sync_labels_hook != NULL)
			tecc_mac_sync_labels_hook();

		if (kind == TECC_BOOTSTRAP_NODE_JOIN)
		{
			tecc_bootstrap_sync_pending = true;
			tecc_bootstrap_mac_commit_pending = true;
		}

		tecc_bootstrap_running = false;
	}
	PG_CATCH();
	{
		tecc_bootstrap_running = false;
		PG_RE_THROW();
	}
	PG_END_TRY();

	if (pushedSnapshot)
	{
		PopActiveSnapshot();
	}
}


/*
 * 触发函数的 OID 缓存。全部解析不到时每次都重试（扩展可能在会话中途才 CREATE）。
 */
static bool
tecc_bootstrap_funcs_available(void)
{
	if (tecc_bootstrap_func_count > 0)
	{
		return true;
	}

	tecc_bootstrap_resolve_functions();

	return tecc_bootstrap_func_count > 0;
}


static void
tecc_bootstrap_resolve_functions(void)
{
	static bool resolved = false;
	Relation	procrel;
	int			group;
	int			i;

	if (resolved)
	{
		return;
	}

	tecc_bootstrap_func_count = 0;

	/*
	 * 按**名字**扫 pg_proc 收集 OID，而不是按签名查找：这些函数的参数里有插件
	 * 自定义类型（例如 create_distributed_table 的 citus.distribution_type），
	 * 按签名解析既脆（hgdde 的类型名可能不同），又容易漏掉重载。
	 */
	procrel = table_open(ProcedureRelationId, AccessShareLock);

	for (group = 0; group < 2; group++)
	{
		const char *const *names = (group == 0)
			? tecc_bootstrap_node_join_functions
			: tecc_bootstrap_distribute_functions;
		TeccBootstrapKind kind = (group == 0)
			? TECC_BOOTSTRAP_NODE_JOIN
			: TECC_BOOTSTRAP_DISTRIBUTE;

		for (i = 0; names[i] != NULL; i++)
		{
			ScanKeyData key;
			SysScanDesc scan;

			ScanKeyInit(&key, Anum_pg_proc_proname,
						BTEqualStrategyNumber, F_NAMEEQ,
						CStringGetDatum(names[i]));

			scan = systable_beginscan(procrel, ProcedureNameArgsNspIndexId, true,
									  NULL, 1, &key);

			for (;;)
			{
				HeapTuple	tuple = systable_getnext(scan);
				Form_pg_proc form;

				if (!HeapTupleIsValid(tuple))
				{
					break;
				}

				form = (Form_pg_proc) GETSTRUCT(tuple);

				if (form->pronamespace != PG_CATALOG_NAMESPACE)
				{
					continue;
				}

				if (tecc_bootstrap_func_count >= TECC_BOOTSTRAP_MAX_FUNCS)
				{
					break;
				}

				tecc_bootstrap_func_oids[tecc_bootstrap_func_count] = form->oid;
				tecc_bootstrap_func_kinds[tecc_bootstrap_func_count] = kind;
				tecc_bootstrap_func_count++;
			}

			systable_endscan(scan);
		}
	}

	table_close(procrel, AccessShareLock);

	if (tecc_bootstrap_func_count > 0)
	{
		resolved = true;
	}
}


static TeccBootstrapKind
tecc_bootstrap_classify(Oid funcid)
{
	int			i;

	for (i = 0; i < tecc_bootstrap_func_count; i++)
	{
		if (tecc_bootstrap_func_oids[i] == funcid)
		{
			return tecc_bootstrap_func_kinds[i];
		}
	}

	return TECC_BOOTSTRAP_NONE;
}
/*-------------------------------------------------------------------------
 * hg_sysmac 未编入本树（标记强访以 contrib/hg_sysmac 预加载扩展形式交付），
 * 因此这里的调用缝保持 NULL：拓扑变化后不会触发标记补发，等价于未安装标记强访。
 * 定义放在本文件，避免为了一个函数指针引入整份 MAC 核心代码。
 *-------------------------------------------------------------------------
 */
tecc_mac_sync_labels_hook_type tecc_mac_sync_labels_hook = NULL;
