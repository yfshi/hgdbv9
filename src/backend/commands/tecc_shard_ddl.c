/*-------------------------------------------------------------------------
 *
 * tecc_shard_ddl.c
 *	  Supply ENCRYPTED WITH clauses to shard DDL that arrives without them.
 *
 * ReqID:TECC4.0
 *
 * 背景（W2）：
 *	  分布式插件（Citus / hgdde）在 worker 上建分片表时，发下去的是**按元组描述符
 *	  反解析**出来的 CREATE TABLE / ALTER TABLE 文本。ENCRYPTED WITH 子句只存在
 *	  hg_encrypted_columns 里，pg_attribute 不保存它，所以反解析出来的列定义
 *	  只剩裸的 *_enc 类型；worker 用本 fork 的语法重新解析后，DefineRelation()
 *	  看到 ColumnDef.encrypted_with 为空，直接报
 *	      ERROR: encrypted data type "int4_enc" requires ENCRYPTED WITH clause
 *	  表就建不出来（W2 的原始缺口）。
 *
 * 主路径不在这里：
 *	  W2 的主路径是 **libpq 侧**（`TECC_MODE_DDL_ONLY`）—— 协调节点把 DDL 发出去
 *	  之前，由 TECC 的前端处理在文本里补齐子句。见
 *	  plan/2026-10-28-hgdde分布式适配可行性分析.md §0.3/§0.4。
 *
 * 本文件是**第二道防线**（纵深防御）：
 *	  libpq 那条路遇到它覆盖不到的形态（例如某种没被识别出来的下发方式）时，
 *	  worker 落地这一侧再从**本地**元数据重建子句。它只处理**分片表**
 *	  （`<逻辑表>_<shardid>`）—— 那种情况下 worker 上逻辑表已经就位、元数据可查
 *	  （插件是"先发逻辑表 DDL、再发分片 DDL"）。逻辑表自己那条 DDL 在 worker 上
 *	  没有任何本地元数据可依，只能靠 libpq 那条主路径。
 *
 * 为什么必须改解析树而不是写目录：
 *	  报错判据是解析树里的 ColumnDef.encrypted_with（tablecmds.c 的
 *	  DefineRelation() 与 ATExecAddColumn()），与目录内容无关；仅预写目录照样报错。
 *
 * 分片识别方式（不依赖插件内部函数，因而 Citus 与 hgdde 通用）：
 *	  1. 语句里的表名形如 <任意>_<十进制数字>；
 *	  2. 该数字在本节点的 pg_catalog.pg_dist_shard 里存在，取其 logicalrelid；
 *	  3. logicalrelid 的名字 + "_" + 数字 与语句里的表名逐字相等（模式名也要等）。
 *	  单机库没有 pg_dist_shard，第 2 步直接返回，钩子退化为一次 strrchr。
 *
 * 已知边界：
 *	  - 只补子句，不动 COLLATE（本 fork 对非 OPE 加密列禁 COLLATE，插件若给非 OPE
 *	    列发了 COLLATE 仍按原规则报错，不静默改写）；
 *	  - 只覆盖 CREATE TABLE 与 ALTER TABLE ... ADD COLUMN 两种会产生加密列元数据的
 *	    形态；分片搬迁/分裂、CREATE TABLE ... LIKE、分区表另行评估。
 *
 * Portions Copyright (c) 2026, HighGo Software Co.,Ltd.
 * Author: <wangxp@highgo.com>
 *
 * IDENTIFICATION
 *	  src/backend/commands/tecc_shard_ddl.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/genam.h"
#include "access/htup_details.h"
#include "access/table.h"
#include "catalog/namespace.h"
#include "catalog/pg_attribute.h"
#include "catalog/pg_namespace_d.h"
#include "commands/encrypted_column.h"
#include "commands/tecc_shard_ddl.h"
#include "nodes/makefuncs.h"
#include "nodes/parsenodes.h"
#include "parser/parse_type.h"
#include "tcop/utility.h"
#include "utils/lsyscache.h"
#include "utils/rel.h"
#include "utils/syscache.h"

/* Begin - ReqID:TECC4.0 */

static ProcessUtility_hook_type prev_ProcessUtility = NULL;

static void tecc_inject_shard_clauses(Node *utilityStmt);
static void tecc_inject_one_column(Oid logical_relid, ColumnDef *column);
static Oid	tecc_shard_logical_relid(RangeVar *relation);
static bool tecc_parse_shard_suffix(const char *relname, uint64 *shard_id);
static bool tecc_type_is_encrypted(TypeName *typeName);

/*
 * tecc_shard_ddl_ProcessUtility
 *		ProcessUtility 钩子：补子句之后再走原有链条。
 *
 *		必须挂在链条最前面：Citus 的 multi_ProcessUtility 在分片 DDL 上会做
 *		自己的预处理，子句要在那之前就在解析树里。
 */
static void
tecc_shard_ddl_ProcessUtility(PlannedStmt *pstmt,
							  const char *queryString,
							  bool readOnlyTree,
							  ProcessUtilityContext context,
							  ParamListInfo params,
							  QueryEnvironment *queryEnv,
							  DestReceiver *dest,
							  QueryCompletion *qc)
{
	/*
	 * readOnlyTree 为真表示调用方要求不改动这棵树（可能被复用）。这种情况下
	 * 先拷贝一份再改，避免污染调用方持有的解析树。
	 */
	if (pstmt->utilityStmt != NULL && readOnlyTree)
	{
		PlannedStmt *copy = copyObject(pstmt);

		tecc_inject_shard_clauses(copy->utilityStmt);
		if (prev_ProcessUtility)
			prev_ProcessUtility(copy, queryString, false, context,
								params, queryEnv, dest, qc);
		else
			standard_ProcessUtility(copy, queryString, false, context,
									params, queryEnv, dest, qc);
		return;
	}

	if (pstmt->utilityStmt != NULL)
		tecc_inject_shard_clauses(pstmt->utilityStmt);

	if (prev_ProcessUtility)
		prev_ProcessUtility(pstmt, queryString, readOnlyTree, context,
							params, queryEnv, dest, qc);
	else
		standard_ProcessUtility(pstmt, queryString, readOnlyTree, context,
								params, queryEnv, dest, qc);
}

/*
 * tecc_inject_shard_clauses
 *		按语句类型分派。只处理建表与加列这两条会产生加密列元数据的形式。
 */
static void
tecc_inject_shard_clauses(Node *utilityStmt)
{
	Oid			logical_relid;

	if (utilityStmt == NULL)
		return;

	if (IsA(utilityStmt, CreateStmt))
	{
		CreateStmt *stmt = (CreateStmt *) utilityStmt;
		ListCell   *lc;

		logical_relid = tecc_shard_logical_relid(stmt->relation);
		if (!OidIsValid(logical_relid))
			return;

		foreach(lc, stmt->tableElts)
		{
			Node	   *elt = (Node *) lfirst(lc);

			if (IsA(elt, ColumnDef))
				tecc_inject_one_column(logical_relid, (ColumnDef *) elt);
		}
	}
	else if (IsA(utilityStmt, AlterTableStmt))
	{
		AlterTableStmt *stmt = (AlterTableStmt *) utilityStmt;
		ListCell   *lc;

		logical_relid = tecc_shard_logical_relid(stmt->relation);
		if (!OidIsValid(logical_relid))
			return;

		foreach(lc, stmt->cmds)
		{
			AlterTableCmd *cmd = (AlterTableCmd *) lfirst(lc);

			if (cmd->subtype == AT_AddColumn && cmd->def != NULL &&
				IsA(cmd->def, ColumnDef))
				tecc_inject_one_column(logical_relid, (ColumnDef *) cmd->def);
		}
	}
}

/*
 * tecc_inject_one_column
 *		把一列的 ENCRYPTED WITH 选项从逻辑表的本地元数据搬到解析树上。
 *
 *		搬不动就什么都不做：标准流程随后会给出 "requires ENCRYPTED WITH
 *		clause" 的报错，这正是我们想要的失败关闭形态（不猜 DEK、不猜算法）。
 */
static void
tecc_inject_one_column(Oid logical_relid, ColumnDef *column)
{
	HeapTuple	atttup;
	Form_pg_attribute attform;
	List	   *options;

	if (column == NULL || column->colname == NULL)
		return;
	/* 插件（或 libpq 主路径）已经带了子句就以它为准，不覆盖 */
	if (column->encrypted_with != NIL)
		return;
	if (!tecc_type_is_encrypted(column->typeName))
		return;

	atttup = SearchSysCache2(ATTNAME,
							 ObjectIdGetDatum(logical_relid),
							 CStringGetDatum(column->colname));
	if (!HeapTupleIsValid(atttup))
		return;
	attform = (Form_pg_attribute) GETSTRUCT(atttup);
	if (attform->attisdropped)
	{
		ReleaseSysCache(atttup);
		return;
	}

	options = hg_get_encrypted_column_options(logical_relid, attform->attnum);
	ReleaseSysCache(atttup);

	if (options == NIL)
		return;

	column->encrypted_with = options;
}

/*
 * tecc_shard_logical_relid
 *		判断语句里的表是不是本节点的分片表；是则返回其逻辑表 OID。
 *
 *		不认领的情形（返回 InvalidOid）一律保持原有报错，绝不静默放过。
 */
static Oid
tecc_shard_logical_relid(RangeVar *relation)
{
	uint64		shard_id;
	Oid			dist_shard_relid;
	Oid			logical_relid = InvalidOid;
	Relation	shard_rel;
	TupleDesc	desc;
	int			shard_id_attno = -1;
	int			logical_attno = -1;
	SysScanDesc scan;
	HeapTuple	tup;
	int			i;

	if (relation == NULL || relation->relname == NULL)
		return InvalidOid;
	if (!tecc_parse_shard_suffix(relation->relname, &shard_id))
		return InvalidOid;

	/*
	 * 单机库（没有分布式插件）里没有 pg_dist_shard：直接退出，不引入任何
	 * 额外依赖。
	 */
	dist_shard_relid = get_relname_relid("pg_dist_shard", PG_CATALOG_NAMESPACE);
	if (!OidIsValid(dist_shard_relid))
		return InvalidOid;

	/*
	 * 按列名定位，不硬编码 attnum：pg_dist_shard 是插件的表，列顺序由插件决定，
	 * hgdde 的目录表只承诺"同名同签名"，不承诺物理列位置。
	 */
	shard_rel = table_open(dist_shard_relid, AccessShareLock);
	desc = RelationGetDescr(shard_rel);
	for (i = 0; i < desc->natts; i++)
	{
		Form_pg_attribute att = TupleDescAttr(desc, i);

		if (att->attisdropped)
			continue;
		if (strcmp(NameStr(att->attname), "shardid") == 0)
			shard_id_attno = i + 1;
		else if (strcmp(NameStr(att->attname), "logicalrelid") == 0)
			logical_attno = i + 1;
	}

	if (shard_id_attno > 0 && logical_attno > 0)
	{
		scan = systable_beginscan(shard_rel, InvalidOid, false, NULL, 0, NULL);
		while (HeapTupleIsValid(tup = systable_getnext(scan)))
		{
			bool		isnull = false;
			Datum		value;
			int64		row_shard_id;
			Oid			row_logical_relid;

			value = heap_getattr(tup, shard_id_attno, desc, &isnull);
			if (isnull)
				continue;
			row_shard_id = DatumGetInt64(value);
			if (row_shard_id < 0 || (uint64) row_shard_id != shard_id)
				continue;

			value = heap_getattr(tup, logical_attno, desc, &isnull);
			if (isnull)
				break;
			row_logical_relid = DatumGetObjectId(value);
			if (OidIsValid(row_logical_relid))
				logical_relid = row_logical_relid;
			break;
		}
		systable_endscan(scan);
	}
	table_close(shard_rel, AccessShareLock);

	if (!OidIsValid(logical_relid))
		return InvalidOid;

	/*
	 * 名字自校验：<逻辑表名>_<shardid> 必须与语句里的名字逐字相等，且（语句带
	 * 模式名时）模式也要对得上。挡掉"恰好同名但不是分片"与"分片在别的模式"这类
	 * 误认领，避免把别人的表改写掉。
	 */
	{
		char	   *logical_name = get_rel_name(logical_relid);
		char	   *expected;

		if (logical_name == NULL)
			return InvalidOid;

		expected = psprintf("%s_" UINT64_FORMAT, logical_name, shard_id);
		if (strcmp(expected, relation->relname) != 0)
			return InvalidOid;
		pfree(expected);
		pfree(logical_name);

		if (relation->schemaname != NULL)
		{
			Oid			rel_nsp = get_rel_namespace(logical_relid);
			char	   *rel_nspname = get_namespace_name(rel_nsp);
			bool		same = (rel_nspname != NULL &&
								strcmp(rel_nspname, relation->schemaname) == 0);

			if (rel_nspname != NULL)
				pfree(rel_nspname);
			if (!same)
				return InvalidOid;
		}
	}

	return logical_relid;
}

/*
 * tecc_parse_shard_suffix
 *		取 <前缀>_<十进制数字> 里的数字。不合法返回 false。
 */
static bool
tecc_parse_shard_suffix(const char *relname, uint64 *shard_id)
{
	const char *suffix;
	char	   *endptr = NULL;
	unsigned long long value;

	suffix = strrchr(relname, '_');
	if (suffix == NULL || suffix == relname)
		return false;
	suffix++;
	if (*suffix < '0' || *suffix > '9')
		return false;

	errno = 0;
	value = strtoull(suffix, &endptr, 10);
	if (errno != 0 || endptr == NULL || *endptr != '\0')
		return false;

	*shard_id = (uint64) value;
	return true;
}

/*
 * tecc_type_is_encrypted
 *		列类型是否为密态类型。走 LookupTypeName（missing_ok）而不是比字符串：
 *		带模式名限定、带引号的写法都能正确解析，未知类型也不会在这里报错。
 */
static bool
tecc_type_is_encrypted(TypeName *typeName)
{
	Type		typtup;
	Oid			typoid;

	if (typeName == NULL)
		return false;

	typtup = LookupTypeName(NULL, typeName, NULL, true);
	if (typtup == NULL)
		return false;

	typoid = typeTypeId(typtup);
	ReleaseSysCache(typtup);

	return is_encrypted_data_type(typoid);
}

/*
 * tecc_install_shard_ddl_hooks
 *		挂在钩子链最前面（postinit 里每个后端各挂一次）。
 */
void
tecc_install_shard_ddl_hooks(void)
{
	prev_ProcessUtility = ProcessUtility_hook;
	ProcessUtility_hook = tecc_shard_ddl_ProcessUtility;
}

/* End - ReqID:TECC4.0 */