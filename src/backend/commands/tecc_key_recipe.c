/*-------------------------------------------------------------------------
 *
 * tecc_key_recipe.c
 *	  TECC 密钥语句的"可移植重建配方"存储（系统表 hg_tecc_ddl）。
 *
 * 为什么需要这个存储：
 *   W3 的在线传播只把密钥送到"当时已注册"的 worker；**后来加入**的 worker 拿不到
 *   既有 KEK/DEK，于是它上面的加密表 DDL 会失败（DEK 缺失时
 *   record_encrypted_column_metadata 直接 ERROR，见 encrypted_column.c）。补发需要
 *   回答"这条密钥当初是怎么建出来的"，而这份事实在服务端**只存在于收到的语句文本
 *   里**（密钥材料是前端算好注入进文本的，服务端无法反向推导，见 §"存什么"）。
 *   本模块把它落库，使"原文"成为可查询的事实。
 *
 * 存什么 —— 每条语句必须同时满足两条，否则跨节点重放会出错：
 *
 *   1) **按名字寻址，不含本节点 OID**。OID 是节点本地的：CREATE KEK/DEK 在每个
 *      节点各自 GetNewOidWithIndex（createkek.c / createdek.c），跨节点稳定的
 *      身份是 column_key_distributed_id（= hash_any("schema.名字")）。CREATE/
 *      ALTER/DROP 的原文本就按名字引用，可直接存；但 ALTER KEK 轮换时前端注入的
 *      `SELECT hg_kek_rotate_dek_arg(<本节点 OID>, ...)` 必须先换成按（模式, 名字）
 *      的本地解析 —— 该改写由 tecc_key.c 的 TeccKeyRotationStmtText 完成，
 *      本模块只接收改写后的结果。
 *
 *   2) **密钥材料已注入**。前端已把 KEY_STORE / KEY_PATH / ENCRYPTED_VALUE /
 *      PUBLIC_KEY / HE_EC_PRIVKEY 等写进语句文本，照搬即可 ⇒ 各节点落下的是
 *      **同一份**材料，而不是各自再生成一份（否则 HE 常量运算会用错公钥）。
 *
 * 行序即重放序：seq 从 0 起，同一对象的语句按 seq 递增排。因此一个对象的配方就是
 * "建出来 + 后续被改过的每一步"，按序重放即可复现当前状态。
 *
 * 与 DROP 的关系：配方是"重建当前状态所需的语句"，所以对象被删时必须连带清掉，
 *   否则重放会把已删的密钥又建回来。清理挂在 object_access_hook 的 OAT_DROP 上
 *   （TeccKeyRecipeForget，由 CitusObjectAccessHook 调用），而不是放在 DROP 语句的
 *   postprocess 里，理由有两条：
 *     · 级联删除路径（DROP SCHEMA ... CASCADE、DROP OWNED 等）根本不会出现
 *       "DROP KEK" 这条语句，挂在语句上会漏；
 *     · 该钩子在 deleteOneObject 里于真正删除**之前**调用，对象还在，可以用
 *       （object_class, object_id）精确定位，不需要事后按名字+search_path 猜测。
 *
 * ReqID:TECC4.0
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/genam.h"
#include "access/htup_details.h"
#include "access/table.h"
#include "catalog/catalog.h"
#include "catalog/hg_dek.h"
#include "catalog/hg_kek.h"
#include "catalog/hg_tecc_ddl.h"
#include "commands/tecc_key_recipe.h"
#include "lib/stringinfo.h"
#include "nodes/parsenodes.h"
#include "nodes/pg_list.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/rel.h"


static int32 TeccKeyRecipeNextSeq(Relation rel, Oid objectClass, Oid namespaceId,
								  const char *objectName);
static void TeccKeyRecipeDelete(Oid objectClass, Oid namespaceId, const char *objectName);
static int32 TeccKeyRecipeTakeSeq(List **seqTracker, Relation rel,
								  Oid objectClass, Oid namespaceId,
								  const char *objectName);


/*
 * TeccKeyRecipeSeqState
 *		Command-scoped cursor for one object's recipe sequence numbers.
 *
 * A single command may record several statements for the *same* object (ALTER
 * KEK and the public_key write-back it injects both belong to the KEK). The
 * hg_tecc_ddl scan cannot see rows inserted earlier in the same command (no
 * CommandCounterIncrement in between), so the next free seq has to be carried
 * in memory rather than re-derived each time -- otherwise the second insert
 * collides on the unique index (object_class, key_namespace, object_name, seq).
 */
typedef struct TeccKeyRecipeSeqState
{
	Oid			objectClass;
	Oid			namespaceId;
	NameData	objectName;
	int32		nextSeq;		/* next unused seq for this object */
} TeccKeyRecipeSeqState;


/*
 * TeccKeyObjectIdentity resolves a KEK/DEK OID to the object's namespace OID and
 * name.  *objectName is palloc'd in the caller's context.  Returns false when the
 * row is gone (the caller then has nothing to record).
 *
 * GETSTRUCT() is safe here because both catalogs keep name and key_namespace in
 * their fixed-length prefix, ahead of the CATALOG_VARLEN aclitem[] field.
 */
bool
TeccKeyObjectIdentity(Oid catalogId, Oid objectId, Oid *namespaceId, char **objectName)
{
	Relation	rel;
	SysScanDesc scan;
	ScanKeyData	scanKey;
	HeapTuple	tuple;
	bool		found = false;

	Assert(catalogId == HgKekRelationId || catalogId == HgDekRelationId);

	rel = table_open(catalogId, AccessShareLock);

	ScanKeyInit(&scanKey,
				(catalogId == HgKekRelationId) ? Anum_hg_kek_oid : Anum_hg_dek_oid,
				BTEqualStrategyNumber, F_OIDEQ, ObjectIdGetDatum(objectId));

	scan = systable_beginscan(rel,
							  (catalogId == HgKekRelationId) ? HgKekOidIndexId :
							  HgDekOidIndexId,
							  true, NULL, 1, &scanKey);
	tuple = systable_getnext(scan);

	if (HeapTupleIsValid(tuple))
	{
		if (catalogId == HgKekRelationId)
		{
			Form_hg_kek	kekForm = (Form_hg_kek) GETSTRUCT(tuple);

			*namespaceId = kekForm->key_namespace;
			*objectName = pstrdup(NameStr(kekForm->kek_name));
		}
		else
		{
			Form_hg_dek	dekForm = (Form_hg_dek) GETSTRUCT(tuple);

			*namespaceId = dekForm->key_namespace;
			*objectName = pstrdup(NameStr(dekForm->dek_name));
		}

		found = true;
	}

	systable_endscan(scan);
	table_close(rel, AccessShareLock);

	return found;
}


/*
 * TeccKeyRecipeCommands returns, in replay order, the statements that recreate one
 * KEK/DEK on a node that does not have it yet. Returns NIL when the object is
 * unknown (e.g. already dropped) or has no recipe.
 *
 * This is what the Citus dependency machinery asks for when it recreates a
 * distributed object on a worker -- both during the initial CREATE (through
 * GetDependencyCreateDDLCommands) and when a new node joins (through
 * SendDependencyCreationCommands). The recipe, not the catalog state, is the
 * source: the key material only ever existed in the statement text the frontend
 * produced.
 */
List *
TeccKeyRecipeCommands(Oid catalogId, Oid objectId)
{
	Oid			namespaceId = InvalidOid;
	char	   *objectName = NULL;
	Relation	rel;
	ScanKeyData	scanKey[3];
	SysScanDesc	scan;
	HeapTuple	tuple;
	List	   *commands = NIL;

	if (!TeccKeyObjectIdentity(catalogId, objectId, &namespaceId, &objectName))
	{
		return NIL;
	}

	rel = table_open(HgTeccDdlRelationId, AccessShareLock);

	ScanKeyInit(&scanKey[0], Anum_hg_tecc_ddl_object_class,
				BTEqualStrategyNumber, F_OIDEQ, ObjectIdGetDatum(catalogId));
	ScanKeyInit(&scanKey[1], Anum_hg_tecc_ddl_key_namespace,
				BTEqualStrategyNumber, F_OIDEQ, ObjectIdGetDatum(namespaceId));
	ScanKeyInit(&scanKey[2], Anum_hg_tecc_ddl_object_name,
				BTEqualStrategyNumber, F_NAMEEQ,
				DirectFunctionCall1(namein, CStringGetDatum(objectName)));

	/* 索引按 (…, seq) 升序 ⇒ 扫描顺序即重放顺序 */
	scan = systable_beginscan(rel, HgTeccDdlObjectIndexId, true, NULL, 3, scanKey);

	while ((tuple = systable_getnext(scan)) != NULL)
	{
		Datum		commandDatum;
		bool		isNull = false;

		commandDatum = heap_getattr(tuple, Anum_hg_tecc_ddl_command,
									RelationGetDescr(rel), &isNull);

		if (!isNull)
		{
			commands = lappend(commands, TextDatumGetCString(commandDatum));
		}
	}

	systable_endscan(scan);
	table_close(rel, AccessShareLock);

	pfree(objectName);

	return commands;
}


/*
 * TeccKeyRecipeNextSeq returns the next free seq for the object: the existing
 * maximum plus one, or 0 when the object has no rows yet.
 *
 * The scan index is (object_class, key_namespace, object_name, seq) ascending, so
 * the last tuple returned already carries the maximum -- no aggregation needed.
 */
static int32
TeccKeyRecipeNextSeq(Relation rel, Oid objectClass, Oid namespaceId,
					 const char *objectName)
{
	ScanKeyData	scanKey[3];
	SysScanDesc	scan;
	HeapTuple	tuple;
	int32		nextSeq = 0;

	ScanKeyInit(&scanKey[0], Anum_hg_tecc_ddl_object_class,
				BTEqualStrategyNumber, F_OIDEQ, ObjectIdGetDatum(objectClass));
	ScanKeyInit(&scanKey[1], Anum_hg_tecc_ddl_key_namespace,
				BTEqualStrategyNumber, F_OIDEQ, ObjectIdGetDatum(namespaceId));
	ScanKeyInit(&scanKey[2], Anum_hg_tecc_ddl_object_name,
				BTEqualStrategyNumber, F_NAMEEQ,
				DirectFunctionCall1(namein, CStringGetDatum(objectName)));

	scan = systable_beginscan(rel, HgTeccDdlObjectIndexId, true, NULL, 3, scanKey);

	while ((tuple = systable_getnext(scan)) != NULL)
	{
		Form_hg_tecc_ddl row = (Form_hg_tecc_ddl) GETSTRUCT(tuple);

		nextSeq = row->seq + 1;
	}

	systable_endscan(scan);

	return nextSeq;
}


/*
 * TeccKeyRecipeTakeSeq returns the next free seq for one object and advances the
 * command-scoped cursor. The first call for an object seeds the cursor from the
 * catalog (maximum persisted seq plus one); later calls for the same object in
 * the same command just hand out the next value, which is what keeps two
 * statements recorded for one object from colliding on the unique index.
 */
static int32
TeccKeyRecipeTakeSeq(List **seqTracker, Relation rel, Oid objectClass,
					 Oid namespaceId, const char *objectName)
{
	ListCell   *lc;
	TeccKeyRecipeSeqState *state = NULL;

	foreach(lc, *seqTracker)
	{
		TeccKeyRecipeSeqState *candidate = (TeccKeyRecipeSeqState *) lfirst(lc);

		if (candidate->objectClass == objectClass &&
			candidate->namespaceId == namespaceId &&
			strcmp(NameStr(candidate->objectName), objectName) == 0)
		{
			state = candidate;
			break;
		}
	}

	if (state == NULL)
	{
		state = palloc0(sizeof(TeccKeyRecipeSeqState));
		state->objectClass = objectClass;
		state->namespaceId = namespaceId;
		namestrcpy(&state->objectName, objectName);
		state->nextSeq = TeccKeyRecipeNextSeq(rel, objectClass, namespaceId,
											  objectName);

		*seqTracker = lappend(*seqTracker, state);
	}

	return state->nextSeq++;
}


/*
 * TeccKeyRecipeAppend appends one processed statement to its object's recipe.
 *
 * The object is identified by (namespace, name) rather than by OID: the statement
 * text itself is name-addressed, and the identity can be derived from the
 * statement (plus the namespace resolution the CREATE commands themselves use)
 * instead of being looked up after the fact.
 *
 * seqTracker is a caller-owned cursor (initialize to NIL) scoped to the current
 * command; it lets several appends for the same object get increasing seqs even
 * though the catalog scan cannot see the earlier inserts of the same command.
 */
void
TeccKeyRecipeAppend(Oid objectClass, Oid namespaceId, const char *objectName,
					const char *command, List **seqTracker)
{
	Relation	rel;
	Oid			rowOid;
	int32		seq;
	Datum		values[Natts_hg_tecc_ddl];
	bool		nulls[Natts_hg_tecc_ddl];
	HeapTuple	tuple;

	if (command == NULL || command[0] == '\0' || objectName == NULL)
	{
		return;
	}

	Assert(seqTracker != NULL);
	Assert(objectClass == HgKekRelationId || objectClass == HgDekRelationId);
	Assert(OidIsValid(namespaceId));

	rel = table_open(HgTeccDdlRelationId, RowExclusiveLock);

	rowOid = GetNewOidWithIndex(rel, HgTeccDdlOidIndexId, Anum_hg_tecc_ddl_oid);
	seq = TeccKeyRecipeTakeSeq(seqTracker, rel, objectClass, namespaceId,
							   objectName);

	memset(values, 0, sizeof(values));
	memset(nulls, false, sizeof(nulls));

	values[Anum_hg_tecc_ddl_oid - 1] = ObjectIdGetDatum(rowOid);
	values[Anum_hg_tecc_ddl_object_class - 1] = ObjectIdGetDatum(objectClass);
	values[Anum_hg_tecc_ddl_key_namespace - 1] = ObjectIdGetDatum(namespaceId);
	values[Anum_hg_tecc_ddl_object_name - 1] =
		DirectFunctionCall1(namein, CStringGetDatum(objectName));
	values[Anum_hg_tecc_ddl_seq - 1] = Int32GetDatum(seq);
	values[Anum_hg_tecc_ddl_command - 1] = CStringGetTextDatum(command);

	tuple = heap_form_tuple(RelationGetDescr(rel), values, nulls);
	CatalogTupleInsert(rel, tuple);
	heap_freetuple(tuple);

	table_close(rel, RowExclusiveLock);
}


/*
 * TeccKeyRecipeDelete removes every recipe row of one (namespace, name) object.
 */
static void
TeccKeyRecipeDelete(Oid objectClass, Oid namespaceId, const char *objectName)
{
	Relation	rel;
	ScanKeyData	scanKey[3];
	SysScanDesc	scan;
	HeapTuple	tuple;
	List	   *deadTids = NIL;
	ListCell   *lc;

	rel = table_open(HgTeccDdlRelationId, RowExclusiveLock);

	ScanKeyInit(&scanKey[0], Anum_hg_tecc_ddl_object_class,
				BTEqualStrategyNumber, F_OIDEQ, ObjectIdGetDatum(objectClass));
	ScanKeyInit(&scanKey[1], Anum_hg_tecc_ddl_key_namespace,
				BTEqualStrategyNumber, F_OIDEQ, ObjectIdGetDatum(namespaceId));
	ScanKeyInit(&scanKey[2], Anum_hg_tecc_ddl_object_name,
				BTEqualStrategyNumber, F_NAMEEQ,
				DirectFunctionCall1(namein, CStringGetDatum(objectName)));

	scan = systable_beginscan(rel, HgTeccDdlObjectIndexId, true, NULL, 3, scanKey);

	/* 先收集 ctid 再删：避免在同一个扫描里边走边删 */
	while ((tuple = systable_getnext(scan)) != NULL)
	{
		ItemPointer tid = palloc(sizeof(ItemPointerData));

		ItemPointerCopy(&tuple->t_self, tid);
		deadTids = lappend(deadTids, tid);
	}

	systable_endscan(scan);

	foreach(lc, deadTids)
	{
		ItemPointer tid = (ItemPointer) lfirst(lc);

		CatalogTupleDelete(rel, tid);
		pfree(tid);
	}

	list_free(deadTids);

	table_close(rel, RowExclusiveLock);
}

/*
 * TeccKeyRecipeForget drops every recipe row of one KEK/DEK.  Called from the
 * object_access_hook on OAT_DROP, i.e. for every deletion path including the
 * cascading ones.
 *
 * The OID is turned into (namespace, name) here rather than at record time: the
 * hook runs before the object is actually removed, so the row is still there
 * (and unlike at CREATE time it is long since visible).
 */
void
TeccKeyRecipeForget(Oid objectClass, Oid objectId)
{
	Oid			namespaceId = InvalidOid;
	char	   *objectName = NULL;

	if (!TeccKeyObjectIdentity(objectClass, objectId, &namespaceId, &objectName))
	{
		return;
	}

	TeccKeyRecipeDelete(objectClass, namespaceId, objectName);

	pfree(objectName);
}
