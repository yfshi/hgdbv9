/*-------------------------------------------------------------------------
 *
 * tecc_dist_guard.c
 *	  Reject encrypted columns as the distribution column of a distributed table.
 *
 * ReqID:TECC4.0
 *
 * 为什么需要它：
 *	  分布式插件在协调节点上对**分布键**求 hash 来决定目标分片，它假定这一列是
 *	  明文；而 TECC 的密态列在服务端只有密文，DET/OPE 提供的是"密文比较"，与
 *	  "按明文值定位分片"不是一回事。插件自己**会接受** `_enc` 类型做分布键
 *	  （`_enc` 类型有默认 btree/hash opclass），不拦就会变成"能建表、但剪枝结果
 *	  错"—— 比直接报错难查得多。所以必须显式拒绝。
 *
 * 为什么拦在 `post_parse_analyze_hook`：
 *	  这个判断原本在插件内部（`EnsureValidDistributionColumn()`）。按"外围适配"
 *	  的要求不能改插件，而 `create_distributed_table()` 是**函数调用**，不是
 *	  utility 语句，`ProcessUtility_hook` 看不到；`fmgr_hook` 又只提供"钩子私有
 *	  槽"、拿不到实参。`post_parse_analyze_hook` 拿到的是分析后的 `Query`，
 *	  参数已经折叠成 `Const`，可以直接读出（表 OID, 列名）。
 *
 * 判据失败时怎么办：
 *	  只有参数是字面量常量时才能判定。若参数不是常量（极少见：`create_distributed_table`
 *	  的实参都是字面量），按"**无法判定就失败关闭**"的底线直接报错，并提示改用字面量
 *	  —— 宁可挡住一个奇怪写法，也不放一个"剪枝会算错"的分布式表进来。
 *
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd.
 * Author: <wangxp@highgo.com>
 *
 * IDENTIFICATION
 *	  src/backend/commands/tecc_dist_guard.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/genam.h"
#include "access/htup_details.h"
#include "access/table.h"
#include "catalog/namespace.h"
#include "catalog/pg_namespace_d.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_type_d.h"
#include "commands/encrypted_column.h"
#include "commands/tecc_dist_guard.h"
#include "miscadmin.h"
#include "nodes/nodeFuncs.h"
#include "nodes/parsenodes.h"
#include "nodes/pg_list.h"
#include "parser/analyze.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "utils/regproc.h"
#include "utils/syscache.h"

/* Begin - ReqID:TECC4.0 */

static post_parse_analyze_hook_type prev_post_parse_analyze_hook = NULL;

/* 目标函数的 OID（首次使用时解析并缓存；可能有多个重载） */
#define TECC_DIST_FUNC_MAX 4
static Oid	tecc_dist_func_oids[TECC_DIST_FUNC_MAX];
static int	tecc_dist_func_count = 0;
static bool tecc_dist_funcs_resolved = false;

static void tecc_dist_guard_post_parse_analyze(ParseState *pstate, Query *query,
											   JumbleState *jstate);
static void tecc_dist_guard_resolve_funcs(void);
static void tecc_dist_guard_check_funcexpr(FuncExpr *funcExpr, const char *fname);

/*
 * tecc_dist_guard_resolve_funcs
 *		收集 `pg_catalog.create_distributed_table` 与 `..._concurrently` 的 OID。
 *
 *		按**名字**收集而不是按签名：这两个函数的第三个参数是插件自定义的
 *		`citus.distribution_type` 枚举（hgdde 的名字可能不同），按签名查找太脆。
 *		插件未安装时收集不到任何 OID，钩子随即退化为纯透传。
 */
static void
tecc_dist_guard_resolve_funcs(void)
{
	static const char *const names[] = {
		"create_distributed_table",
		"create_distributed_table_concurrently"
	};
	Relation	procrel;
	SysScanDesc scan;
	ScanKeyData key;
	HeapTuple	tup;
	int			i;

	if (tecc_dist_funcs_resolved)
		return;

	tecc_dist_funcs_resolved = true;

	procrel = table_open(ProcedureRelationId, AccessShareLock);

	for (i = 0; i < (int) lengthof(names) &&
		 tecc_dist_func_count < TECC_DIST_FUNC_MAX; i++)
	{
		ScanKeyInit(&key,
					Anum_pg_proc_proname,
					BTEqualStrategyNumber, F_NAMEEQ,
					CStringGetDatum(names[i]));

		scan = systable_beginscan(procrel, ProcedureNameArgsNspIndexId, true,
								  NULL, 1, &key);

		while (HeapTupleIsValid(tup = systable_getnext(scan)))
		{
			Form_pg_proc form = (Form_pg_proc) GETSTRUCT(tup);

			if (form->pronamespace != PG_CATALOG_NAMESPACE)
				continue;
			if (tecc_dist_func_count >= TECC_DIST_FUNC_MAX)
				break;

			tecc_dist_func_oids[tecc_dist_func_count++] = form->oid;
		}

		systable_endscan(scan);
	}

	table_close(procrel, AccessShareLock);
}

/*
 * tecc_dist_guard_is_watched
 *		该函数 OID 是否是我们关心的两个 API 之一。
 */
static bool
tecc_dist_guard_is_watched(Oid funcid)
{
	int			i;

	if (!OidIsValid(funcid))
		return false;

	for (i = 0; i < tecc_dist_func_count; i++)
	{
		if (tecc_dist_func_oids[i] == funcid)
			return true;
	}

	return false;
}

/*
 * tecc_dist_guard_check_funcexpr
 *		若该 FuncExpr 是"把某列声明为分布键"，则校验该列是否为密态类型。
 */
static void
tecc_dist_guard_check_funcexpr(FuncExpr *funcExpr, const char *fname)
{
	Node	   *relarg;
	Node	   *colarg;
	Const	   *relconst;
	Const	   *colconst;
	Oid			relid = InvalidOid;
	char	   *column_name;
	Oid			atttype;

	if (list_length(funcExpr->args) < 2)
		return;

	relarg = (Node *) linitial(funcExpr->args);
	colarg = (Node *) lsecond(funcExpr->args);

	/*
	 * 表名实参既可能是 regclass 也可能是 text，见下面解析处的说明。
	 */
	relconst = (IsA(relarg, Const) &&
				(((Const *) relarg)->consttype == REGCLASSOID ||
				 ((Const *) relarg)->consttype == TEXTOID)) ?
		(Const *) relarg : NULL;
	colconst = (IsA(colarg, Const) && ((Const *) colarg)->consttype == TEXTOID) ?
		(Const *) colarg : NULL;

	/*
	 * 实参不是字面量常量：判定不了。按"无法判定就失败关闭"处理，并给出改法
	 * —— 放过去的代价是"能建表但剪枝算错"，那比报错危险得多。
	 */
	if (relconst == NULL || colconst == NULL ||
		relconst->constisnull || colconst->constisnull)
	{
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("%s() must be called with literal table and column names",
						fname),
				 errhint("TECC has to verify that the distribution column is not an "
						 "encrypted column, which requires literal arguments.")));
		return;
	}

	if (relconst->consttype == REGCLASSOID)
	{
		relid = DatumGetObjectId(relconst->constvalue);
	}
	else
	{
		/*
		 * hgdde 在自由 PG 上也多提供了一个 `(text, text, ...)` 重载（为 IvorySQL
		 * oracle 兼容模式下的标识符大小写服务）。字面量调用在类型解析阶段会优先
		 * 落到 text 版本（text 是字符串类的 preferred type），所以这不是边角路径：
		 * `create_distributed_table('t', 'id')` 在 hgdde 上命中的就是它。
		 * 按 regclassin 的语义把名字解析成 OID（与插件 text 版入口一致）。
		 *
		 * 解析不到（表还不存在）时不在这里报错：真正的调用会给出插件自己的
		 * 报错，比这里重复一遍更准确；也不会因此放过任何**存在**的表。
		 */
		char	   *relname = TextDatumGetCString(relconst->constvalue);
		RangeVar   *rv;

		rv = makeRangeVarFromNameList(stringToQualifiedNameList(relname));
		relid = RangeVarGetRelid(rv, AccessShareLock, true);
	}

	if (!OidIsValid(relid))
		return;

	column_name = TextDatumGetCString(colconst->constvalue);

	atttype = get_atttype(relid, get_attnum(relid, column_name));
	if (OidIsValid(atttype) && is_encrypted_data_type(atttype))
	{
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("cannot use encrypted column \"%s\" as the "
						"distribution column of relation \"%s\"",
						column_name, get_rel_name(relid)),
				 errhint("Distribution columns must be plaintext columns.")));
	}
}

/*
 * tecc_dist_guard_post_parse_analyze
 *		post_parse_analyze 钩子：只看最外层目标列表里的函数调用。
 *
 *		只扫 `query->targetList`（不递归 subquery）是刻意的：这类调用只会出现在
 *		语句最外层，而钩子对**每条**语句都会跑，必须保持廉价。
 */
static void
tecc_dist_guard_post_parse_analyze(ParseState *pstate, Query *query,
								   JumbleState *jstate)
{
	if (prev_post_parse_analyze_hook)
		prev_post_parse_analyze_hook(pstate, query, jstate);

	if (query == NULL || query->commandType != CMD_SELECT ||
		query->utilityStmt != NULL)
		return;

	tecc_dist_guard_resolve_funcs();

	if (tecc_dist_func_count == 0)
		return;

	{
		ListCell   *lc;

		foreach(lc, query->targetList)
		{
			TargetEntry *tle = (TargetEntry *) lfirst(lc);
			Node	   *expr;

			if (tle->resjunk)
				continue;

			expr = (Node *) tle->expr;
			if (expr == NULL || !IsA(expr, FuncExpr))
				continue;

			if (tecc_dist_guard_is_watched(((FuncExpr *) expr)->funcid))
				tecc_dist_guard_check_funcexpr((FuncExpr *) expr,
											   "create_distributed_table");
		}
	}
}

/*
 * tecc_install_dist_guard_hooks
 *		postinit 里每个后端各挂一次（链在已有钩子之后，不抢占）。
 */
void
tecc_install_dist_guard_hooks(void)
{
	prev_post_parse_analyze_hook = post_parse_analyze_hook;
	post_parse_analyze_hook = tecc_dist_guard_post_parse_analyze;
}

/* End - ReqID:TECC4.0 */