/*-------------------------------------------------------------------------
 *
 * flashback.c
 *		Implement the functionality of flashback table
 *
 * Note that the entire flashback feature is divided into two parts:
 * flashback query and flashback table (there are also flashback transaction
 * and flashback database, among others). This file only contains the code
 * implementation for parts other than flashback query.
 *
 *
 * Portions Copyright (c) 2025, IvorySQL-2.0
 *
 * IDENTIFICATION
 *	  src/backend/commands/flashback.c
 *
 * add the file for requirement "SRS-RECOVERY-FLASHBACK"
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "nodes/parsenodes.h"
#include "utils/relcache.h"
#include "utils/snapshot.h"
#include "storage/lockdefs.h"
#include "access/table.h"
#include "utils/acl.h"
#include "utils/rel.h"
#include "access/xact.h"
#include "miscadmin.h"
#include "commands/flashback.h"
#include "utils/timestamp.h"
#include "utils/snapmgr.h"
#include "access/tableam.h"
#include "catalog/pg_recyclebin.h"
#include "utils/lsyscache.h"
#include "storage/lmgr.h"
#include "access/skey.h"
#include "access/genam.h"
#include "utils/inval.h"
#include "utils/fmgroids.h"
#include "utils/syscache.h"
#include "utils/builtins.h"
#include "catalog/indexing.h"
#include "access/xact.h"
#include "access/heapam.h"
#include "catalog/heap.h"
#include "storage/predicate.h"
#include "catalog/index.h"
#include "pgstat.h"
#include "parser/pg_object_hook.h"
#include "utils/ora_compatible.h"
#include "catalog/objectaccess.h"
#include "commands/tablecmds.h"
#include "catalog/catalog.h"
#include "catalog/storage.h"
#include "catalog/pg_namespace.h"
#include "catalog/pg_trigger.h"
#include "executor/executor.h"

/*
 * Delete delta tuples that inserted after the snapshot.
 */
static void
table_delete_delta(Relation rel, Snapshot snapshot)
{
    TableScanDesc scandesc;
    TupleTableSlot *slot;

    snapshot->snapshot_type = SNAPSHOT_DELTA;
    scandesc = table_beginscan(rel, snapshot, 0, NULL);

    slot = table_slot_create(rel, NULL);
    while (table_scan_getnextslot(scandesc, ForwardScanDirection, slot))
    {
        simple_table_tuple_delete(rel, &slot->tts_tid, snapshot);
    }

    ExecDropSingleTupleTableSlot(slot);
    table_endscan(scandesc);
}

/*
 * Insert lost tuples that inserted before the snapshot
 * but deleted after it.
 */
static void
table_insert_lost(Relation rel, Snapshot snapshot)
{
    TableScanDesc scandesc;
    TupleTableSlot *slot;
    ResultRelInfo *reulstRelinfo = makeNode(ResultRelInfo);
    EState *estate = CreateExecutorState();
    RangeTblEntry *rte;

    snapshot->snapshot_type = SNAPSHOT_LOST;
    scandesc = table_beginscan(rel, snapshot, 0, NULL);

    slot = table_slot_create(rel, NULL);

    /* Struct for index insert. */
    estate->es_relations = (Relation *)palloc(sizeof(Relation));
    estate->es_relations[0] = rel;
    rte = makeNode(RangeTblEntry);
	rte->rtekind = RTE_RELATION;
	rte->relid = RelationGetRelid(rel);
	rte->relkind = rel->rd_rel->relkind;
	rte->rellockmode = AccessShareLock;
    ExecInitRangeTable(estate, list_make1(rte));
    ExecInitResultRelation(estate, reulstRelinfo, 1);
    ExecOpenIndices(reulstRelinfo, false);

    while (table_scan_getnextslot(scandesc, ForwardScanDirection, slot))
    {
        simple_table_tuple_insert(rel, slot);
        ExecInsertIndexTuples(reulstRelinfo, slot, estate, false, false, NULL, NULL);
    }

    ExecCloseResultRelations(estate);
    FreeExecutorState(estate);

    ExecDropSingleTupleTableSlot(slot);
    table_endscan(scandesc);
}

/*
 * Restore the data in the table to the state at a specified time point.
 */
void FlashbackToTimepoint(FlashbackStmt *stmt)
{
    Relation rel;
    Snapshot snapshot;

    rel = table_openrv(stmt->relation, AccessExclusiveLock);

    if (!pg_class_ownercheck(RelationGetRelid(rel), GetUserId()))
    {
        aclcheck_error(ACLCHECK_NOT_OWNER, OBJECT_TABLE, RelationGetRelationName(rel));
    }

    if (rel->rd_rel->relkind != RELKIND_RELATION ||
        (rel->rd_rel->relpersistence != RELPERSISTENCE_PERMANENT &&
         rel->rd_rel->relpersistence != RELPERSISTENCE_UNLOGGED))
        elog(ERROR, "Flashback only support simple table");

    /*
     * Construct the snapshot based on the transaction status
     * at the corresponding timestamp.
     */
    snapshot = TimestampGetSnapshot(DatumGetTimestampTz(((Const *)stmt->timepoint)->constvalue));

    /*
     * Delete delta tuples that inserted after restore point.
     */
    table_delete_delta(rel, snapshot);

    /*
     * Insert lost tuples that inserted before restore point
     * but deleted after restore point.
     */
    table_insert_lost(rel, snapshot);

    table_close(rel, NoLock);

    /*
     * CommandCounterIncrement here to ensure that preceding changes are all
     * visible to the next command.
     */
    CommandCounterIncrement();

    return;
}

/*
 * After enabling the recycle bin feature, the DROP and TRUNCATE operations do not
 * delete the physical files, which can easily lead to space bloat.
 * Therefore, added a parameters for control.
 */
bool EnableRecycleBin()
{
    return (DB_ORACLE == database_mode) && enable_flashback_recycle_bin;
}

RbObjType
RbGetObjType(Oid nspId, char relKind)
{
    RbObjType type = RB_OBJ_TABLE;

    switch (relKind)
    {
    case RELKIND_INDEX:
        type = IsToastNamespace(nspId) ? RB_OBJ_TOAST_INDEX : RB_OBJ_INDEX;
        break;
    case RELKIND_RELATION:
        type = RB_OBJ_TABLE;
        break;
    case RELKIND_TOASTVALUE:
        type = RB_OBJ_TOAST;
        break;
    default:
        elog(ERROR, "unrecognized relation kind %c", relKind);
        break;
    }

    return type;
}

static void
RbCheckPermission(RbObjDesc *desc, RbOperMode operMode)
{
    /* Allow restore to either table owner or schema owner */
    if (!pg_class_ownercheck(desc->relid, desc->authid) &&
        !pg_namespace_ownercheck(desc->nspace, desc->authid))
    {
        aclcheck_error(ACLCHECK_NOT_OWNER, OBJECT_TABLE, desc->name);
        return;
    }

    if (operMode == RB_OPER_RESTORE_TRUNCATE)
    {
        AclResult aclTruncateResult = pg_class_aclcheck(desc->relid, desc->authid, ACL_TRUNCATE);
        if (aclTruncateResult != ACLCHECK_OK)
        {
            aclcheck_error(aclTruncateResult, OBJECT_TABLE, desc->name);
        }
    }
}

/*
 * Lock the entries in the system table pg_recyclebin.
 */
static void
RbLockItem(RbObjDesc *desc)
{
    Relation rbRel;
    SysScanDesc sd;
    ScanKeyData skey[1];
    HeapTuple tup;

    /* 1. Try to lock rb item in AccessExclusiveLock */
    if (!ConditionalLockDatabaseObject(RecyclebinRelationId, desc->id, 0, AccessExclusiveLock))
    {
        elog(ERROR, "could not obtain lock on recycle object '%s'", desc->name);
    }

    /*
     * 2. Now that we have the lock, probe to see if the rb item really
     *    exists or not.
     */
    AcceptInvalidationMessages();

    rbRel = table_open(RecyclebinRelationId, AccessShareLock);

    ScanKeyInit(&skey[0], Anum_pg_recyclebin_oid, BTEqualStrategyNumber,
                F_OIDEQ, ObjectIdGetDatum(desc->id));

    sd = systable_beginscan(rbRel, RecyclebinOidIndexId, true, NULL, 1, skey);
    if ((tup = systable_getnext(sd)) == NULL)
    {
        UnlockDatabaseObject(RecyclebinRelationId, desc->id, 0, AccessExclusiveLock);
        systable_endscan(sd);
        table_close(rbRel, AccessShareLock);
        elog(ERROR, "recycle object id %u does not exist", desc->id);
    }

    systable_endscan(sd);
    table_close(rbRel, AccessShareLock);

    return;
}

static void
RbUnlockItem(RbObjDesc *desc)
{
    UnlockDatabaseObject(RecyclebinRelationId, desc->id, 0, AccessExclusiveLock);
}

static void
RbLockRelationImpl(Oid relid)
{
    /*
     * Lock failed may due to concurrently purge/timecapsule/DQL
     * on recycle object, or access on normal relation.
     */
    if (!ConditionalLockRelationOid(relid, AccessExclusiveLock))
    {
        elog(ERROR, "could not obtain lock on relation \"%u\"", relid);
    }

    /*
     * Now that we have the lock, probe to see if the relation
     * really exists or not.
     */
    AcceptInvalidationMessages();
    if (!SearchSysCacheExists1(RELOID, ObjectIdGetDatum(relid)))
    {
        /* Clean already held locks if error return. */
        UnlockRelationOid(relid, AccessExclusiveLock);
        elog(ERROR, "relation \"%u\" does not exist", relid);
    }
}

/*
 * Lock the objects that need to be flashed back.
 */
static void
RbLockRelation(RbObjDesc *desc)
{
    Oid heapOid = InvalidOid;

    /* Lock heap relation for index first */
    if (desc->type == RB_OBJ_INDEX || desc->type == RB_OBJ_TOAST_INDEX)
    {
        heapOid = IndexGetRelation(desc->relid, true);
        if (!OidIsValid(heapOid))
        {
            elog(ERROR, "relation %u does not exist", desc->relid);
        }
        RbLockRelationImpl(heapOid);
    }

    /* Use TRY-CATCH block to clean locks already held if error. */
    PG_TRY();
    {
        /* Lock relation self */
        RbLockRelationImpl(desc->relid);
    }
    PG_CATCH();
    {
        if (desc->type == RB_OBJ_INDEX || desc->type == RB_OBJ_TOAST_INDEX)
        {
            UnlockRelationOid(heapOid, AccessExclusiveLock);
        }
        PG_RE_THROW();
    }
    PG_END_TRY();
}

/*
 * Prepare for rb operations - purge, restore :
 *     check permission, lock objects
 */
static void
RbOperPrep(RbObjDesc *desc, RbOperMode operMode)
{
    bool needLockRelation = false;

    /*
     * 1. Check permission.
     */
    RbCheckPermission(desc, operMode);

    /*
     * 2. Acquire lock on rb item, avoid concurrently purge, restore.
     */
    RbLockItem(desc);

    /*
     * 3. Acquire lock on relation, avoid concurrently DQL.
     *    Notice: ignore this step when we purge truncated relation
     *            as base relation may not exists.
     */
    needLockRelation = !(operMode == RB_OPER_PURGE && desc->operation == RCYOPERATION_TRUNCATE);
    if (needLockRelation)
    {
        /* Use TRY-CATCH block to clean locks already held if error. */
        PG_TRY();
        {
            RbLockRelation(desc);
        }
        PG_CATCH();
        {
            RbUnlockItem(desc);
            PG_RE_THROW();
        }
        PG_END_TRY();
    }
}

/*
 * Delete the records of the object and it's dependent objects from the recycle bin.
 *
 * In the database kernel, references to indexes, TOAST tables,
 * and similar objects are all made using OIDs rather than names.
 * Therefore, we do not need to actually "restore" all these dependencies.
 */
static void
RbDeleteBaseid(Oid baseid, char optype)
{
    Relation rbRel;
    SysScanDesc sd;
    ScanKeyData skey[1];
    HeapTuple tup;

    rbRel = table_open(RecyclebinRelationId, RowExclusiveLock);

    ScanKeyInit(&skey[0], Anum_pg_recyclebin_rcybaseid, BTEqualStrategyNumber, F_OIDEQ, ObjectIdGetDatum(baseid));
    sd = systable_beginscan(rbRel, RecyclebinBaseidIndexId, true, NULL, 1, skey);
    while (HeapTupleIsValid(tup = systable_getnext(sd)))
    {
        Form_pg_recyclebin rbForm = (Form_pg_recyclebin)GETSTRUCT(tup);
        if (rbForm->rcyoperation == optype)
            simple_heap_delete(rbRel, &tup->t_self);
    }

    systable_endscan(sd);
    table_close(rbRel, RowExclusiveLock);

    /*
     * CommandCounterIncrement here to ensure that preceding changes are all
     * visible to the next deletion step.
     */
    CommandCounterIncrement();
}

/*
 * Modify the name of the class object.
 */
void 
RbRenameClass(RbObjDesc *baseDesc, ObjectAddress *object, char relKind, const char *newName)
{
    Relation rel;
    HeapTuple tup;
    HeapTuple newtup;
    Datum values[Natts_pg_class] = {0};
    bool nulls[Natts_pg_class] = {false};
    bool replaces[Natts_pg_class] = {false};
    Oid relid = object->objectId;
    const char *objName = NULL;

    /* Handle other dependent objects outside the base table. */
    if (object->classId != RelationRelationId ||
        object->objectId != baseDesc->relid ||
        object->objectSubId != 0)
    {
        RbObjDesc desc;
        /* Deletion lock already accquired before single object drop. */
        rel = relation_open(object->objectId, NoLock);

        RbDescInit(rel, &desc, RCYOPERATION_DROP, RbGetObjType(RelationGetNamespace(rel), relKind));
        relation_close(rel, NoLock);

        RecyclebinWrite(&desc, baseDesc->baseid);
        objName = desc.name;
    }
    else
    {
        objName = newName;
    }

    replaces[Anum_pg_class_relname - 1] = true;
    values[Anum_pg_class_relname - 1] = DirectFunctionCall1(namein, CStringGetDatum(objName));

    rel = table_open(RelationRelationId, RowExclusiveLock);
    tup = SearchSysCache1(RELOID, ObjectIdGetDatum(relid));
    if (!HeapTupleIsValid(tup))
    {
        elog(ERROR, "cache lookup failed for relation %u", relid);
    }

    newtup = heap_modify_tuple(tup, RelationGetDescr(rel), values, nulls, replaces);
    CatalogTupleUpdate(rel, &newtup->t_self, newtup);
    ReleaseSysCache(tup);

    heap_freetuple(newtup);
    table_close(rel, RowExclusiveLock);
}

/*
 * Initialize the information that needs to be recorded in the pg_recyclebin.
 */
void 
RbDescInit(Relation rel, RbObjDesc *desc, char operType, RbObjType objType)
{
    /* Notice: desc->id will be assigned by invoker later. */
    desc->relid = RelationGetRelid(rel);

    pg_snprintf(desc->name, NAMEDATALEN, "BIN$%X%X%X$%lX==$0",
                MyDatabaseId, RelationRelationId, desc->relid, (uint64)GetXLogInsertRecPtr());
    (void)namestrcpy((Name)desc->originname, RelationGetRelationName(rel));

    desc->operation = operType;
    desc->type = objType;
    desc->recycletime = GetCurrentTimestamp();
    desc->nspace = RelationGetNamespace(rel);
    desc->tspace = rel->rd_rel->reltablespace;
    desc->relfilenode = rel->rd_rel->relfilenode;
    desc->authid = GetUserId();
}

/*
 * Restore the object to its state before the drop operation.
 */
void 
FlashbackBeforeDrop(FlashbackStmt *stmt)
{
    RbObjDesc baseDesc;
    Relation rel;
    Relation rbRel;
    ScanKeyData skey[1];
    SysScanDesc sd;
    HeapTuple tup;
    HeapTuple reltup;
    HeapTuple newreltup;
    Datum values[Natts_pg_class] = {0};
    bool nulls[Natts_pg_class] = {false};
    bool replaces[Natts_pg_class] = {false};

    /* 1. Fetch the latest available recycle object. */
    RbOperFetch(stmt->relation->schemaname, stmt->relation->relname,
                RB_OBJ_TABLE, &baseDesc, RB_OPER_RESTORE_DROP);

    /* 2. Do restore. */
    rbRel = table_open(RecyclebinRelationId, RowExclusiveLock);
    rel = table_open(RelationRelationId, RowExclusiveLock);
    ScanKeyInit(&skey[0], Anum_pg_recyclebin_rcybaseid, BTEqualStrategyNumber,
                F_OIDEQ, ObjectIdGetDatum(baseDesc.baseid));
    sd = systable_beginscan(rbRel, RecyclebinBaseidIndexId, true, NULL, 1, skey);
    while (HeapTupleIsValid(tup = systable_getnext(sd)))
    {
        RbObjDesc desc;

        RecyclebinRead(&desc, tup);

        /* Lock recycle object and base relation. */
        RbOperPrep(&desc, RB_OPER_RESTORE_DROP);

        replaces[Anum_pg_class_relname - 1] = true;
        if (desc.type == RB_OBJ_TABLE && stmt->new_relname != NULL)
            values[Anum_pg_class_relname - 1] = DirectFunctionCall1(namein, CStringGetDatum(stmt->new_relname));
        else
            values[Anum_pg_class_relname - 1] = DirectFunctionCall1(namein, CStringGetDatum(desc.originname));

        reltup = SearchSysCache1(RELOID, ObjectIdGetDatum(desc.relid));
        if (!HeapTupleIsValid(reltup))
        {
            elog(ERROR, "cache lookup failed for relation %u", desc.relid);
        }
        
        newreltup = heap_modify_tuple(reltup, RelationGetDescr(rel), values, nulls, replaces);
        CatalogTupleUpdate(rel, &newreltup->t_self, newreltup);

        ReleaseSysCache(reltup);
        heap_freetuple(newreltup);
    }
    systable_endscan(sd);

    table_close(rel, RowExclusiveLock);
    table_close(rbRel, RowExclusiveLock);

    /*
     * CommandCounterIncrement here to ensure that preceding changes are all
     * visible to the next deletion step.
     */
    CommandCounterIncrement();

    /* 4. Delete from recyclebin. */
    RbDeleteBaseid(baseDesc.baseid, RCYOPERATION_DROP);

    return;
}

/*
 * seqScan table pg_trigger and find exist truncate trigger.
 */
static bool 
RbJudgeHaveTrigger(Oid tgOid)
{
    HeapTuple tup;
    Relation relTrig;
    ScanKeyData skey[1];
    SysScanDesc sd;
    uint16 tgType;

    relTrig = table_open(TriggerRelationId, AccessShareLock);
    ScanKeyInit(&skey[0], Anum_pg_trigger_tgrelid, BTEqualStrategyNumber,
                F_OIDEQ, ObjectIdGetDatum(tgOid));
    sd = systable_beginscan(relTrig, InvalidOid, false, NULL, 1, skey);
    if ((tup = systable_getnext(sd)) != NULL)
    {
        tgType = ((Form_pg_trigger)GETSTRUCT(tup))->tgtype;
        if (TRIGGER_FOR_TRUNCATE(tgType))
        {
            systable_endscan(sd);
            table_close(relTrig, AccessShareLock);
            return true;
        }
    }
    systable_endscan(sd);
    table_close(relTrig, AccessShareLock);
    return false;
}

/*
 * Generate a new filenode for the table and delete the old one.
 */
static void
RbRelationSetNewRelfilenode(Relation relation, char persistence)
{
    Oid newrelfilenode = InvalidOid;
    Relation pg_class = NULL;
    HeapTuple tuple = NULL;
    Form_pg_class classform;
    MultiXactId minmulti = InvalidMultiXactId;
    TransactionId freezeXid = InvalidTransactionId;
    RelFileNode newrnode;

    /* Allocate a new relfilenode */
    newrelfilenode = GetNewRelFileNode(relation->rd_rel->reltablespace, NULL, persistence);

    memset(&classform, 0, sizeof(classform));

    /*
     * Get a writable copy of the pg_class tuple for the given relation.
     */
    pg_class = table_open(RelationRelationId, RowExclusiveLock);

    tuple = SearchSysCacheCopy1(RELOID,
                                ObjectIdGetDatum(RelationGetRelid(relation)));
    if (!HeapTupleIsValid(tuple))
        elog(ERROR, "could not find tuple for relation %u",
             RelationGetRelid(relation));
    classform = (Form_pg_class)GETSTRUCT(tuple);

    /*
     * NOTE: if the relation was created in this transaction, it will now be
     * present in the pending-delete list twice, once with atCommit true and
     * once with atCommit false.  Hence, it will be physically deleted at end
     * of xact in either case (and the other entry will be ignored by
     * smgrDoPendingDeletes, so no error will occur).  We could instead remove
     * the existing list entry and delete the physical file immediately, but
     * for now I'll keep the logic simple.
     */

    RelationCloseSmgr(relation);

    newrnode = relation->rd_node;
    newrnode.relNode = newrelfilenode;
    table_relation_set_new_filenode(relation, &newrnode,
                                    persistence,
                                    &freezeXid, &minmulti);

    /* Normal case, update the pg_class entry */
    classform->relfilenode = newrelfilenode;
    classform->relpages = 0; /* it's empty until further notice */
    classform->reltuples = -1;
    classform->relallvisible = 0;
    classform->relfrozenxid = freezeXid;
    classform->relminmxid = minmulti;
    classform->relpersistence = persistence;

    CatalogTupleUpdate(pg_class, &tuple->t_self, tuple);

    heap_freetuple(tuple);
    table_close(pg_class, RowExclusiveLock);

    /*
     * Make the pg_class row change or relation map change visible.  This will
     * cause the relcache entry to get updated, too.
     */
    CommandCounterIncrement();

    RelationAssumeNewRelfilenode(relation);
}

/*
 * Move the old filenode to the recycle bin and generate a new filenode for the table.
 */
void 
RbTruncate(List *explicit_rels, List *relids, List *relids_logged,
                DropBehavior behavior, bool restart_seqs)
{
    Relation rel = (Relation)linitial(explicit_rels);
    Oid relid;
    Oid toastRelid;
    RbObjDesc baseDesc;
    ReindexParams newparams = {0};

    /*
     * 1. Open relation in AccessExclusiveLock, and check permission, etc.
     */
    relid = RelationGetRelid(rel);
    rel = table_open(relid, AccessExclusiveLock);

    if (list_length(explicit_rels) != 1 ||
        restart_seqs == true ||
        rel->rd_rel->relkind != RELKIND_RELATION ||
        (rel->rd_rel->relpersistence != RELPERSISTENCE_PERMANENT &&
         rel->rd_rel->relpersistence != RELPERSISTENCE_UNLOGGED))
    {
        ExecuteTruncateGuts(explicit_rels, relids, relids_logged,
                            behavior, restart_seqs);
        table_close(rel, AccessExclusiveLock);
        return;
    }

    if (RbJudgeHaveTrigger(relid))
    {
        elog(NOTICE, "Not support truncate triggers where use recyclebin features, "
                     "requires manual truncate target table");
    }

    heap_truncate_check_FKs(explicit_rels, false);

    /*
     * This effectively deletes all rows in the table, and may be done
     * in a serializable transaction.  In that case we must record a
     * rw-conflict in to this transaction from each transaction
     * holding a predicate lock on the table.
     */
    CheckTableForSerializableConflictIn(rel);

    /*
     * 2. Create a new empty storage file for the relation, and assign it
     * as the relfilenode value, and record the old relfilenode to recyclebin.
     */
    RbDescInit(rel, &baseDesc, RCYOPERATION_TRUNCATE, RB_OBJ_TABLE);
    baseDesc.id = baseDesc.baseid = RecyclebinWrite(&baseDesc, InvalidOid);
    RbRelationSetNewRelfilenode(rel, rel->rd_rel->relpersistence);

    /*
     * 3. The same for the toast table, if any.
     */
    toastRelid = rel->rd_rel->reltoastrelid;
    if (OidIsValid(toastRelid))
    {
        RbObjDesc   desc;
        Relation    relToast;

        relToast = relation_open(toastRelid, AccessExclusiveLock);
        RbDescInit(relToast, &desc, RCYOPERATION_TRUNCATE, RB_OBJ_TOAST);
        RecyclebinWrite(&desc, baseDesc.baseid);
        RbRelationSetNewRelfilenode(relToast, relToast->rd_rel->relpersistence);
        table_close(relToast, NoLock);
    }

    /*
     * 4. Reconstruct the indexes to match, and we're done.
     */
    (void)reindex_relation(relid, REINDEX_REL_PROCESS_TOAST, &newparams);

    /*
     * 5. Report stat, and clean.
     */

    /* report truncate to PgStatCollector */
    pgstat_count_truncate(rel);

    /* Record time of truancate relation. */
    if (pg_object_alter_obj_hook && compatible_db == ORA_PARSER)
    {
        ObjectAddress address;
        PgObjectInfo objinf;

        ObjectAddressSet(address, RelationRelationId, relid);
        objinf = pg_object_get_ori_infor_hook(address);
        pg_object_alter_obj_hook(address,
                                 objinf->oriname,
                                 objinf->orischema,
                                 objinf->orioption,
                                 NULL);
    }

    table_close(rel, NoLock);
}

/*
 * Replace the table's filenode with the old version saved in the recycle bin.
 */
static void 
RbSwapRelfilenode(Relation rbRel, HeapTuple rbTup)
{
    RelFileNode rnode;
    Relation relRel;
    HeapTuple relTup;
    Form_pg_class relTupForm;
    HeapTuple newTup;
    RbObjDesc desc;
    Datum values[Natts_pg_class] = {0};
    bool nulls[Natts_pg_class] = {false};
    bool replaces[Natts_pg_class] = {false};

    RecyclebinRead(&desc, rbTup);
    relRel = table_open(RelationRelationId, RowExclusiveLock);
    relTup = SearchSysCacheCopy1(RELOID, ObjectIdGetDatum(desc.relid));

    if (!HeapTupleIsValid(relTup))
    {
        elog(ERROR, "cache lookup failed for relation %u", desc.relid);
    }

    /* 1. delete old filenode */
    relTupForm = (Form_pg_class) GETSTRUCT(relTup);
    rnode.spcNode = relTupForm->reltablespace;
    rnode.dbNode = MyDatabaseId;
    rnode.relNode = relTupForm->relfilenode;
    FilenodeAppendIntoPendingList(&rnode);

    /* 2. Update pg_class */
    replaces[Anum_pg_class_relfilenode - 1] = true;
    values[Anum_pg_class_relfilenode - 1] = ObjectIdGetDatum(desc.relfilenode);
    replaces[Anum_pg_class_reltablespace - 1] = true;
    values[Anum_pg_class_reltablespace - 1] = ObjectIdGetDatum(desc.tspace);

    newTup = heap_modify_tuple(relTup, RelationGetDescr(relRel), values, nulls, replaces);
    CatalogTupleUpdate(relRel, &newTup->t_self, newTup);
    heap_freetuple(newTup);
    heap_freetuple(relTup);

    table_close(relRel, RowExclusiveLock);

    /* 3. Delete from pg_recyclebin */
    CatalogTupleDelete(rbRel, &rbTup->t_self);

    /*
     * Make the pg_class row change or relation map change visible.  This will
     * cause the relcache entry to get updated, too.
     */
    CommandCounterIncrement();

    return;
}

/*
 * Restore the object to its state before the truncate operation.
 */
void 
FlashbackBeforeTruncate(FlashbackStmt *stmt)
{
    RbObjDesc baseDesc;
    Relation rbRel;
    SysScanDesc sd;
    ScanKeyData skey[1];
    HeapTuple tup;
    Relation rel;
    ReindexParams newparams = {0};

    rel = table_openrv(stmt->relation, AccessExclusiveLock);

    if (rel->rd_rel->relkind != RELKIND_RELATION ||
        rel->rd_rel->relpersistence != RELPERSISTENCE_PERMANENT)
        elog(ERROR, "Flashback only support simple table");

    /* 1. Fetch the latest available recycle object. */
    RbOperFetch(stmt->relation->schemaname, stmt->relation->relname,
                RB_OBJ_TABLE, &baseDesc, RB_OPER_RESTORE_TRUNCATE);

    /* 2. Do restore. */
    rbRel = table_open(RecyclebinRelationId, RowExclusiveLock);
    ScanKeyInit(&skey[0], Anum_pg_recyclebin_rcybaseid, BTEqualStrategyNumber,
                F_OIDEQ, ObjectIdGetDatum(baseDesc.baseid));
    sd = systable_beginscan(rbRel, RecyclebinBaseidIndexId, true, NULL, 1, skey);
    while (HeapTupleIsValid(tup = systable_getnext(sd)))
    {
        RbObjDesc desc;

        RecyclebinRead(&desc, tup);
        /* Lock recycle object and base relation. */
        RbOperPrep(&desc, RB_OPER_RESTORE_TRUNCATE);

        RbSwapRelfilenode(rbRel, tup);
    }
    systable_endscan(sd);

    /*
     * 3. Reconstruct the indexes to match, and we're done.
     */
    (void)reindex_relation(RelationGetRelid(rel), REINDEX_REL_PROCESS_TOAST, &newparams);

    table_close(rbRel, RowExclusiveLock);
    table_close(rel, NoLock);

    /*
     * CommandCounterIncrement here to ensure that preceding changes are all
     * visible to the next deletion step.
     */
    CommandCounterIncrement();

    return;
}

/* Purge the last DROP operation data for a table and its related objects. */
static void 
RbDoPurgeObjectDrop(RbObjDesc *desc)
{
    ObjectAddresses *objects;
    ObjectAddress obj;

    /* Lock and validate each relation; build a list of object addresses */
    objects = new_object_addresses();

    obj.classId = RelationRelationId;
    obj.objectId = desc->relid;
    obj.objectSubId = 0;
    add_exact_object_address(&obj, objects);

    /* really delete */
    performMultipleDeletions(objects, DROP_CASCADE, 0);

    /* delete items from pg_recyclebin */
    RbDeleteBaseid(desc->baseid, RCYOPERATION_DROP);

    free_object_addresses(objects);
    return;
}

/* Purge the last TRUNCATE operation data for a table and its related objects. */
static void 
RbDoPurgeObjectTruncate(RbObjDesc *desc)
{
    Relation rbRel;
    SysScanDesc sd;
    ScanKeyData skey[1];
    HeapTuple tup;

    rbRel = table_open(RecyclebinRelationId, RowExclusiveLock);

    ScanKeyInit(&skey[0], Anum_pg_recyclebin_rcybaseid, BTEqualStrategyNumber,
                F_OIDEQ, ObjectIdGetDatum(desc->baseid));
    sd = systable_beginscan(rbRel, RecyclebinBaseidIndexId, true, NULL, 1, skey);
    while (HeapTupleIsValid(tup = systable_getnext(sd)))
    {
        Form_pg_recyclebin rbForm = (Form_pg_recyclebin)GETSTRUCT(tup);
        RelFileNode rnode;

        rnode.spcNode = rbForm->rcytablespace;
        rnode.dbNode = MyDatabaseId;
        rnode.relNode = rbForm->rcyrelfilenode;

        /*
         * Schedule unlinking of the old storage at transaction commit.
         */
        FilenodeAppendIntoPendingList(&rnode);

        simple_heap_delete(rbRel, &tup->t_self);
    }

    systable_endscan(sd);
    table_close(rbRel, RowExclusiveLock);

    /*
     * CommandCounterIncrement here to ensure that preceding changes are all
     * visible to the next deletion step.
     */
    CommandCounterIncrement();
    return;
}

static void 
RbDoPurgeObject(RbObjDesc *desc)
{
    if (desc->operation == RCYOPERATION_DROP)
    {
        RbDoPurgeObjectDrop(desc);
    }
    else
    {
        RbDoPurgeObjectTruncate(desc);
    }
    return;
}

/*
 * Purge the specified tables and their associated objects from the recyclebin.
 */
void 
PurgeObject(RangeVar *rv)
{
    RbObjDesc baseDesc;

    /* 1. Fetch the purge object. */
    RbOperFetch(rv->schemaname, rv->relname,
                RB_OBJ_TABLE, &baseDesc, RB_OPER_PURGE);

    /* 2. Lock recycle object and base relation. */
    RbOperPrep(&baseDesc, RB_OPER_PURGE);

    /* 3. Do purge. */
    RbDoPurgeObject(&baseDesc);

    return;
}

/*
 * Purge all objects from the recyclebin.
 */
void 
PurgeAllObjects()
{
    RbObjDesc baseDesc;
    Oid userId = GetUserId();

    /* superuser only */
    if (!superuser_arg(userId))
        elog(ERROR, "Only superuser can do purge recyclebin operation.");

    /* 1. Fetch the purge object. */
    while (RbFetchAny(&baseDesc))
    {

        /* 2. Lock recycle object and base relation. */
        baseDesc.authid = userId;
        RbOperPrep(&baseDesc, RB_OPER_PURGE);

        /* 3. Do purge. */
        RbDoPurgeObject(&baseDesc);
    }
    return;
}
