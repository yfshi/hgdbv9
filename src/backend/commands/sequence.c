/*-------------------------------------------------------------------------
 *
 * sequence.c
 *	  PostgreSQL sequences support code.
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/commands/sequence.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"
#include <math.h>	/* ReqID:SRS-SQL-SEQ */

#include "access/bufmask.h"
#include "access/htup_details.h"
#include "access/multixact.h"
#include "access/relation.h"
#include "access/table.h"
#include "access/transam.h"
#include "access/xact.h"
#include "access/xlog.h"
#include "access/xloginsert.h"
#include "access/xlogutils.h"
#include "catalog/dependency.h"
#include "catalog/indexing.h"
#include "catalog/namespace.h"
#include "catalog/objectaccess.h"
#include "catalog/pg_sequence.h"
#include "catalog/pg_type.h"
#include "catalog/storage.h"
#include "catalog/storage_gtt.h"
#include "catalog/storage_xlog.h"
#include "commands/defrem.h"
#include "commands/sequence.h"
#include "commands/tablecmds.h"
#include "funcapi.h"
#include "miscadmin.h"
#include "nodes/makefuncs.h"
#include "parser/parse_expr.h"
#include "parser/parse_type.h"
#include "storage/lmgr.h"
#include "storage/proc.h"
#include "storage/smgr.h"
#include "utils/acl.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/resowner.h"
#include "utils/syscache.h"
#include "utils/varlena.h"
/* Begin - BUG#Z026 */
#include "utils/guc.h"
#include "utils/ora_compatible.h"
/* End - BUG#Z026 */
/* Begin - ReqID:SRS-SQL-SEQ */
#include "catalog/pg_namespace_d.h"
#include "common/fe_memutils.h"
/* End - ReqID:SRS-SQL-SEQ */
/* Begin - ReqID:SRS-MYSQL-CASE */
#include "mysql_parser/mys_case_sensitive.h"
/* End - ReqID:SRS-MYSQL-CASE */
/* Begin - BUG#00004709 */
#include "pg_contrib_audit.h"
/* End - BUG#00004709 */
#include "utils/guc.h"
#include "utils/numeric.h"

/*Begin - Bug#z6158*/
#include "se_features_interface.h"
/*End - Bug#z6158*/

/*
 * We don't want to log each fetching of a value from a sequence,
 * so we pre-log a few fetches in advance. In the event of
 * crash we can lose (skip over) as many values as we pre-logged.
 */
#define SEQ_LOG_VALS	32

/*
 * The "special area" of a sequence's buffer page looks like this.
 */
#define SEQ_MAGIC	  0x1717

typedef struct sequence_magic
{
	uint32		magic;
} sequence_magic;

/*
 * We store a SeqTable item for every sequence we have touched in the current
 * session.  This is needed to hold onto nextval/currval state.  (We can't
 * rely on the relcache, since it's only, well, a cache, and may decide to
 * discard entries.)
 */
typedef struct SeqTableData
{
	Oid			relid;			/* pg_class OID of this sequence (hash key) */
	Oid			filenode;		/* last seen relfilenode of this sequence */
	LocalTransactionId lxid;	/* xact in which we last did a seq op */
	bool		last_valid;		/* do we have a valid "last" value? */
	int64		last;			/* value last returned by nextval */
	int64		cached;			/* last value already cached for nextval */
	/* if last != cached, we have not used up all the cached values */
	int64		increment;		/* copy of sequence's increment field */
	/* note that increment is zero until we first do nextval_internal() */
	int64		scale_value;	/* for scale option */
} SeqTableData;

typedef SeqTableData *SeqTable;

static HTAB *seqhashtab = NULL; /* hash table for SeqTable items */
/* Begin - ReqID:SRS-SQL-SEQ */
int64	session_id = 0;
int32	scale_value = 0;
/* End - ReqID:SRS-SQL-SEQ */

/*
 * last_used_seq is updated by nextval() to point to the last used
 * sequence.
 */
static SeqTableData *last_used_seq = NULL;
static uint64 mys_g_last_seqval = 0; /* SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */

static void fill_seq_with_data(Relation rel, HeapTuple tuple);
static void fill_seq_fork_with_data(Relation rel, HeapTuple tuple, ForkNumber forkNum);
static Relation lock_and_open_sequence(SeqTable seq);
static void create_seq_hashtable(void);
static void init_sequence(Oid relid, SeqTable *p_elm, Relation *p_rel);
static Form_pg_sequence_data read_seq_tuple(Relation rel,
											Buffer *buf, HeapTuple seqdatatuple);
static void init_params(ParseState *pstate, List *options, bool for_identity,
						bool isInit, char seq_type,	/* ReqID:SRS-SQL-IDENTCOL */
						Form_pg_sequence seqform,
						Form_pg_sequence_data seqdataform,
						bool *need_seq_rewrite,
						List **owned_by, SeqTable seqelm);	/* ReqID:SRS-SQL-SEQ */
static void do_setval(Oid relid, int64 next, bool iscalled);
static void process_owned_by(Relation seqrel, List *owned_by, bool for_identity);
/* Begin - ReqID:SRS-SQL-SEQ */
static bool seq_is_scale(int16 seq_flag);
static bool seq_is_extend(int16 seq_flag);
static bool seq_is_session(int16 seq_flag);
/* End - ReqID:SRS-SQL-SEQ */

/* Begin - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
static AttrNumber get_rel_auto_increment_attr(Oid tableoid);

static uint64 compute_current_autoinc(uint64 nr);
static uint64 compute_next_autoinc(uint64 current, uint64 step, /*!< in: AUTOINC increment step */
							uint64 offset, uint64 max_value);		  /*!< in: max value for type */
static void compute_cur_next_value(uint64 lastautoinc, uint64 *curautoinc, uint64 *nextautoinc);
static void set_auto_increment(SeqTable elm, Relation seqrel, bool displayinsert, uint64 autoincvalue, bool logit);

Datum autoinc_nextval(PG_FUNCTION_ARGS);
/* End - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
/*
 * DefineSequence
 *				Creates a new sequence relation
 */
ObjectAddress
DefineSequence(ParseState *pstate, CreateSeqStmt *seq)
{
	FormData_pg_sequence seqform;
	FormData_pg_sequence_data seqdataform;
	bool		need_seq_rewrite;
	List	   *owned_by;
	CreateStmt *stmt = makeNode(CreateStmt);
	Oid			seqoid;
	ObjectAddress address;
	Relation	rel;
	HeapTuple	tuple;
	TupleDesc	tupDesc;
	Datum		value[SEQ_COL_LASTCOL];
	bool		null[SEQ_COL_LASTCOL];
	Datum		pgs_values[Natts_pg_sequence];
	bool		pgs_nulls[Natts_pg_sequence];
	int			i;

	/*
	 * If if_not_exists was given and a relation with the same name already
	 * exists, bail out. (Note: we needn't check this when not if_not_exists,
	 * because DefineRelation will complain anyway.)
	 */
	if (seq->if_not_exists)
	{
		RangeVarGetAndCheckCreationNamespace(seq->sequence, NoLock, &seqoid);
		if (OidIsValid(seqoid))
		{
			/*
			 * If we are in an extension script, insist that the pre-existing
			 * object be a member of the extension, to avoid security risks.
			 */
			ObjectAddressSet(address, RelationRelationId, seqoid);
			checkMembershipInCurrentExtension(&address);

			/* OK to skip */
			ereport(NOTICE,
					(errcode(ERRCODE_DUPLICATE_TABLE),
					 errmsg("relation \"%s\" already exists, skipping",
							seq->sequence->relname)));
			return InvalidObjectAddress;
		}
	}

	/* Begin - ReqID:SRS-SQL-GTT */
	if (seq->sequence->relpersistence == RELPERSISTENCE_GLOBAL_TEMP)
	{
		seq->sequence->relpersistence = RELPERSISTENCE_PERMANENT;
	}
	/* End - ReqID:SRS-SQL-GTT */

	/* Check and set all option values */
	init_params(pstate, seq->options, seq->for_identity, true, seq->seq_type,	/* ReqID:SRS-SQL-IDENTCOL */
				&seqform, &seqdataform,
				&need_seq_rewrite, &owned_by, NULL);	/* ReqID:SRS-SQL-SEQ */

	/*
	 * Create relation (and fill value[] and null[] for the tuple)
	 */
	stmt->tableElts = NIL;
	for (i = SEQ_COL_FIRSTCOL; i <= SEQ_COL_LASTCOL; i++)
	{
		ColumnDef  *coldef = makeNode(ColumnDef);

		coldef->inhcount = 0;
		coldef->is_local = true;
		coldef->is_not_null = true;
		coldef->is_from_type = false;
		coldef->storage = 0;
		coldef->raw_default = NULL;
		coldef->cooked_default = NULL;
		coldef->collClause = NULL;
		coldef->collOid = InvalidOid;
		coldef->constraints = NIL;
		coldef->location = -1;
		coldef->isvisible = true;	/* ReqID:SRS-SQL-INVISIBLECOL */

		null[i - 1] = false;

		switch (i)
		{
			case SEQ_COL_LASTVAL:
				coldef->typeName = makeTypeNameFromOid(INT8OID, -1);
				coldef->colname = "last_value";
				value[i - 1] = Int64GetDatumFast(seqdataform.last_value);
				break;
			case SEQ_COL_LOG:
				coldef->typeName = makeTypeNameFromOid(INT8OID, -1);
				coldef->colname = "log_cnt";
				value[i - 1] = Int64GetDatum((int64) 0);
				break;
			case SEQ_COL_CALLED:
				coldef->typeName = makeTypeNameFromOid(BOOLOID, -1);
				coldef->colname = "is_called";
				value[i - 1] = BoolGetDatum(false);
				break;
			/* Begin - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
			case SEQ_COL_NEXTVAL:
				coldef->typeName = makeTypeNameFromOid(INT8OID, -1);
				coldef->colname = "next_value";
				value[i - 1] = Int64GetDatum((int64)0);
				break;
			/* End - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
			}
		stmt->tableElts = lappend(stmt->tableElts, coldef);
	}

	stmt->relation = seq->sequence;
	stmt->inhRelations = NIL;
	stmt->constraints = NIL;
	stmt->options = seq->options;	/* ReqID:SRS-CMD-PGDUMP */
	stmt->oncommit = ONCOMMIT_NOOP;
	stmt->tablespacename = NULL;
	stmt->if_not_exists = seq->if_not_exists;
	stmt->with_rowid_seq = seq->with_rowid;

	address = DefineRelation(stmt, RELKIND_SEQUENCE, seq->ownerId, NULL, NULL);
	seqoid = address.objectId;
	Assert(seqoid != InvalidOid);

	rel = table_open(seqoid, AccessExclusiveLock);
	tupDesc = RelationGetDescr(rel);

	/* now initialize the sequence's data */
	tuple = heap_form_tuple(tupDesc, value, null);
	fill_seq_with_data(rel, tuple);

	/* process OWNED BY if given */
	if (owned_by)
		process_owned_by(rel, owned_by, seq->for_identity);

	table_close(rel, NoLock);

	/* fill in pg_sequence */
	rel = table_open(SequenceRelationId, RowExclusiveLock);
	tupDesc = RelationGetDescr(rel);

	memset(pgs_nulls, 0, sizeof(pgs_nulls));

	pgs_values[Anum_pg_sequence_seqrelid - 1] = ObjectIdGetDatum(seqoid);
	pgs_values[Anum_pg_sequence_seqtypid - 1] = ObjectIdGetDatum(seqform.seqtypid);
	pgs_values[Anum_pg_sequence_seqstart - 1] = Int64GetDatumFast(seqform.seqstart);
	pgs_values[Anum_pg_sequence_seqincrement - 1] = Int64GetDatumFast(seqform.seqincrement);
	pgs_values[Anum_pg_sequence_seqmax - 1] = Int64GetDatumFast(seqform.seqmax);
	pgs_values[Anum_pg_sequence_seqmin - 1] = Int64GetDatumFast(seqform.seqmin);
	pgs_values[Anum_pg_sequence_seqcache - 1] = Int64GetDatumFast(seqform.seqcache);
	pgs_values[Anum_pg_sequence_seqcycle - 1] = BoolGetDatum(seqform.seqcycle);
	pgs_values[Anum_pg_sequence_flags - 1] = Int16GetDatum(seqform.flags);	/* ReqID:SRS-SQL-SEQ */

	tuple = heap_form_tuple(tupDesc, pgs_values, pgs_nulls);
	CatalogTupleInsert(rel, tuple);

	heap_freetuple(tuple);
	table_close(rel, RowExclusiveLock);

	return address;
}

/*
 * Reset a sequence to its initial value.
 *
 * The change is made transactionally, so that on failure of the current
 * transaction, the sequence will be restored to its previous state.
 * We do that by creating a whole new relfilenode for the sequence; so this
 * works much like the rewriting forms of ALTER TABLE.
 *
 * Caller is assumed to have acquired AccessExclusiveLock on the sequence,
 * which must not be released until end of transaction.  Caller is also
 * responsible for permissions checking.
 */
void
ResetSequence(Oid seq_relid)
{
	Relation	seq_rel;
	SeqTable	elm;
	Form_pg_sequence_data seq;
	Buffer		buf;
	HeapTupleData seqdatatuple;
	HeapTuple	tuple;
	HeapTuple pgstuple;
	Form_pg_sequence pgsform;
	int64		startv;

	/*
	 * Read the old sequence.  This does a bit more work than really
	 * necessary, but it's simple, and we do want to double-check that it's
	 * indeed a sequence.
	 */
	init_sequence(seq_relid, &elm, &seq_rel);
	(void) read_seq_tuple(seq_rel, &buf, &seqdatatuple);

	pgstuple = SearchSysCache1(SEQRELID, ObjectIdGetDatum(seq_relid));
	if (!HeapTupleIsValid(pgstuple))
		elog(ERROR, "cache lookup failed for sequence %u", seq_relid);
	pgsform = (Form_pg_sequence) GETSTRUCT(pgstuple);
	startv = pgsform->seqstart;
	ReleaseSysCache(pgstuple);

	/*
	 * Copy the existing sequence tuple.
	 */
	tuple = heap_copytuple(&seqdatatuple);

	/* Now we're done with the old page */
	UnlockReleaseBuffer(buf);

	/*
	 * Modify the copied tuple to execute the restart (compare the RESTART
	 * action in AlterSequence)
	 */
	seq = (Form_pg_sequence_data) GETSTRUCT(tuple);
	seq->last_value = startv;
	seq->is_called = false;
	seq->log_cnt = 0;
	seq->next_value = 0;/* SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */

	/*
	 * Create a new storage file for the sequence.
	 */
	RelationSetNewRelfilenode(seq_rel, seq_rel->rd_rel->relpersistence);

	/*
	 * Ensure sequence's relfrozenxid is at 0, since it won't contain any
	 * unfrozen XIDs.  Same with relminmxid, since a sequence will never
	 * contain multixacts.
	 */
	Assert(seq_rel->rd_rel->relfrozenxid == InvalidTransactionId);
	Assert(seq_rel->rd_rel->relminmxid == InvalidMultiXactId);

	/*
	 * Insert the modified tuple into the new storage file.
	 */
	fill_seq_with_data(seq_rel, tuple);

	/* Clear local cache so that we don't think we have cached numbers */
	/* Note that we do not change the currval() state */
	elm->cached = elm->last;

	relation_close(seq_rel, NoLock);
}

/*
 * Initialize a sequence's relation with the specified tuple as content
 *
 * This handles unlogged sequences by writing to both the main and the init
 * fork as necessary.
 */
static void
fill_seq_with_data(Relation rel, HeapTuple tuple)
{
	fill_seq_fork_with_data(rel, tuple, MAIN_FORKNUM);

	if (rel->rd_rel->relpersistence == RELPERSISTENCE_UNLOGGED)
	{
		SMgrRelation srel;

		srel = smgropen(rel->rd_node, InvalidBackendId);
		smgrcreate(srel, INIT_FORKNUM, false);
		log_smgrcreate(&rel->rd_node, INIT_FORKNUM);
		fill_seq_fork_with_data(rel, tuple, INIT_FORKNUM);
		FlushRelationBuffers(rel);
		smgrclose(srel);
	}
}

/*
 * Initialize a sequence's relation fork with the specified tuple as content
 */
static void
fill_seq_fork_with_data(Relation rel, HeapTuple tuple, ForkNumber forkNum)
{
	Buffer		buf;
	Page		page;
	sequence_magic *sm;
	OffsetNumber offnum;

	/* Initialize first page of relation with special magic number */

	buf = ReadBufferExtended(rel, forkNum, P_NEW, RBM_NORMAL, NULL);
	Assert(BufferGetBlockNumber(buf) == 0);

	page = BufferGetPage(buf);

	PageInit(page, BufferGetPageSize(buf), sizeof(sequence_magic));
	sm = (sequence_magic *) PageGetSpecialPointer(page);
	sm->magic = SEQ_MAGIC;

	/* Now insert sequence tuple */

	LockBuffer(buf, BUFFER_LOCK_EXCLUSIVE);

	/*
	 * Since VACUUM does not process sequences, we have to force the tuple to
	 * have xmin = FrozenTransactionId now.  Otherwise it would become
	 * invisible to SELECTs after 2G transactions.  It is okay to do this
	 * because if the current transaction aborts, no other xact will ever
	 * examine the sequence tuple anyway.
	 */
	HeapTupleHeaderSetXmin(tuple->t_data, FrozenTransactionId);
	HeapTupleHeaderSetXminFrozen(tuple->t_data);
	HeapTupleHeaderSetCmin(tuple->t_data, FirstCommandId);
	HeapTupleHeaderSetXmax(tuple->t_data, InvalidTransactionId);
	tuple->t_data->t_infomask |= HEAP_XMAX_INVALID;
	ItemPointerSet(&tuple->t_data->t_ctid, 0, FirstOffsetNumber);

	/* check the comment above nextval_internal()'s equivalent call. */
	if (RelationNeedsWAL(rel))
		GetTopTransactionId();

	START_CRIT_SECTION();

	MarkBufferDirty(buf);

	offnum = PageAddItem(page, (Item) tuple->t_data, tuple->t_len,
						 InvalidOffsetNumber, false, false);
	if (offnum != FirstOffsetNumber)
		elog(ERROR, "failed to add sequence tuple to page");

	/* XLOG stuff */
	if (RelationNeedsWAL(rel) || forkNum == INIT_FORKNUM)
	{
		xl_seq_rec	xlrec;
		XLogRecPtr	recptr;

		XLogBeginInsert();
		XLogRegisterBuffer(0, buf, REGBUF_WILL_INIT);

		xlrec.node = rel->rd_node;

		XLogRegisterData((char *) &xlrec, sizeof(xl_seq_rec));
		XLogRegisterData((char *) tuple->t_data, tuple->t_len);

		recptr = XLogInsert(RM_SEQ_ID, XLOG_SEQ_LOG);

		PageSetLSN(page, recptr);
	}

	END_CRIT_SECTION();

	UnlockReleaseBuffer(buf);
}

/*
 * AlterSequence
 *
 * Modify the definition of a sequence relation
 */
ObjectAddress
AlterSequence(ParseState *pstate, AlterSeqStmt *stmt)
{
	Oid			relid;
	SeqTable	elm;
	Relation	seqrel;
	Buffer		buf;
	HeapTupleData datatuple;
	Form_pg_sequence seqform;
	Form_pg_sequence_data newdataform;
	bool		need_seq_rewrite;
	List	   *owned_by;
	ObjectAddress address;
	Relation	rel;
	HeapTuple	seqtuple;
	HeapTuple	newdatatuple;

	/* Open and lock sequence, and check for ownership along the way. */
	relid = RangeVarGetRelidExtended(stmt->sequence,
									 ShareRowExclusiveLock,
									 stmt->missing_ok ? RVR_MISSING_OK : 0,
									 RangeVarCallbackOwnsRelation,
									 NULL);
	if (relid == InvalidOid)
	{
		ereport(NOTICE,
				(errmsg("relation \"%s\" does not exist, skipping",
						stmt->sequence->relname)));
		return InvalidObjectAddress;
	}

	init_sequence(relid, &elm, &seqrel);

	if (RELATION_IS_GLOBAL_TEMP(seqrel))
	{
		if (is_other_backend_use_gtt(RelationGetRelid(seqrel)))
			ereport(ERROR,
				(errcode(ERRCODE_DEPENDENT_OBJECTS_STILL_EXIST),
				 errmsg("cannot alter global temporary sequence %s when other backend attached it.",
						RelationGetRelationName(seqrel))));
	}

	rel = table_open(SequenceRelationId, RowExclusiveLock);
	seqtuple = SearchSysCacheCopy1(SEQRELID,
								   ObjectIdGetDatum(relid));
	if (!HeapTupleIsValid(seqtuple))
		elog(ERROR, "cache lookup failed for sequence %u",
			 relid);

	seqform = (Form_pg_sequence) GETSTRUCT(seqtuple);

	/* lock page's buffer and read tuple into new sequence structure */
	(void) read_seq_tuple(seqrel, &buf, &datatuple);

	/* copy the existing sequence data tuple, so it can be modified locally */
	newdatatuple = heap_copytuple(&datatuple);
	newdataform = (Form_pg_sequence_data) GETSTRUCT(newdatatuple);

	UnlockReleaseBuffer(buf);

	/* Check and set new values */
	init_params(pstate, stmt->options, stmt->for_identity, false, 0,	/* ReqID:SRS-SQL-IDENTCOL */
				seqform, newdataform,
				&need_seq_rewrite, &owned_by, elm);	/* ReqID:SRS-SQL-SEQ */

	/* Clear local cache so that we don't think we have cached numbers */
	/* Note that we do not change the currval() state */
	elm->cached = elm->last;

	/* If needed, rewrite the sequence relation itself */
	if (need_seq_rewrite)
	{
		/* check the comment above nextval_internal()'s equivalent call. */
		if (RelationNeedsWAL(seqrel))
			GetTopTransactionId();

		/*
		 * Create a new storage file for the sequence, making the state
		 * changes transactional.
		 */
		RelationSetNewRelfilenode(seqrel, seqrel->rd_rel->relpersistence);

		/*
		 * Ensure sequence's relfrozenxid is at 0, since it won't contain any
		 * unfrozen XIDs.  Same with relminmxid, since a sequence will never
		 * contain multixacts.
		 */
		Assert(seqrel->rd_rel->relfrozenxid == InvalidTransactionId);
		Assert(seqrel->rd_rel->relminmxid == InvalidMultiXactId);

		/*
		 * Insert the modified tuple into the new storage file.
		 */
		fill_seq_with_data(seqrel, newdatatuple);
	}

	/* process OWNED BY if given */
	if (owned_by)
		process_owned_by(seqrel, owned_by, stmt->for_identity);

	/* update the pg_sequence tuple (we could skip this in some cases...) */
	CatalogTupleUpdate(rel, &seqtuple->t_self, seqtuple);

	InvokeObjectPostAlterHook(RelationRelationId, relid, 0);

	ObjectAddressSet(address, RelationRelationId, relid);

	table_close(rel, RowExclusiveLock);
	relation_close(seqrel, NoLock);

	return address;
}

void
SequenceChangePersistence(Oid relid, char newrelpersistence)
{
	SeqTable	elm;
	Relation	seqrel;
	Buffer		buf;
	HeapTupleData seqdatatuple;

	init_sequence(relid, &elm, &seqrel);

	/* check the comment above nextval_internal()'s equivalent call. */
	if (RelationNeedsWAL(seqrel))
		GetTopTransactionId();

	(void) read_seq_tuple(seqrel, &buf, &seqdatatuple);
	RelationSetNewRelfilenode(seqrel, newrelpersistence);
	fill_seq_with_data(seqrel, &seqdatatuple);
	UnlockReleaseBuffer(buf);

	relation_close(seqrel, NoLock);
}

void
DeleteSequenceTuple(Oid relid)
{
	Relation	rel;
	HeapTuple	tuple;

	rel = table_open(SequenceRelationId, RowExclusiveLock);

	tuple = SearchSysCache1(SEQRELID, ObjectIdGetDatum(relid));
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "cache lookup failed for sequence %u", relid);

	CatalogTupleDelete(rel, &tuple->t_self);

	ReleaseSysCache(tuple);
	table_close(rel, RowExclusiveLock);
}

/*
 * Note: nextval with a text argument is no longer exported as a pg_proc
 * entry, but we keep it around to ease porting of C code that may have
 * called the function directly.
 */
Datum
nextval(PG_FUNCTION_ARGS)
{
	text	   *seqin = PG_GETARG_TEXT_PP(0);
	RangeVar   *sequence;
	Oid			relid;

	sequence = makeRangeVarFromNameList(textToQualifiedNameList(seqin));

	/*
	 * XXX: This is not safe in the presence of concurrent DDL, but acquiring
	 * a lock here is more expensive than letting nextval_internal do it,
	 * since the latter maintains a cache that keeps us from hitting the lock
	 * manager more than once per transaction.  It's not clear whether the
	 * performance penalty is material in practice, but for now, we do it this
	 * way.
	 */
	relid = RangeVarGetRelid(sequence, NoLock, false);

	PG_RETURN_INT64(nextval_internal(relid, true));
}

Datum
nextval_oid(PG_FUNCTION_ARGS)
{
	Oid			relid = PG_GETARG_OID(0);

	PG_RETURN_INT64(nextval_internal(relid, true));
}

/* Begin - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
/*
 * Note: autoinc_nextval with a text argument is no longer exported as a pg_proc
 * entry, but we keep it around to ease porting of C code that may have
 * called the function directly.
 */
Datum
autoinc_nextval(PG_FUNCTION_ARGS)
{
	text	   *seqin = PG_GETARG_TEXT_PP(0);
	RangeVar   *sequence;
	Oid			relid;

	sequence = makeRangeVarFromNameList(textToQualifiedNameList(seqin));

	/*
	 * XXX: This is not safe in the presence of concurrent DDL, but acquiring
	 * a lock here is more expensive than letting nextval_internal do it,
	 * since the latter maintains a cache that keeps us from hitting the lock
	 * manager more than once per transaction.  It's not clear whether the
	 * performance penalty is material in practice, but for now, we do it this
	 * way.
	 */
	relid = RangeVarGetRelid(sequence, NoLock, false);

	PG_RETURN_INT64(nextval_autoincrement(relid, true));
}

Datum
autoinc_nextval_oid(PG_FUNCTION_ARGS)
{
	Oid			relid = PG_GETARG_OID(0);

	PG_RETURN_INT64(nextval_autoincrement(relid, true));
}

int64
nextval_autoincrement(Oid relid, bool check_permissions)
{
	SeqTable elm;
	Relation seqrel;
	HeapTuple pgstuple;
	bool logit = false;

	pgstuple = SearchSysCache1(SEQRELID, ObjectIdGetDatum(relid));
	if (!HeapTupleIsValid(pgstuple))
		elog(ERROR, "cache lookup failed for sequence %u", relid);

	/* open and lock sequence */
	init_sequence(relid, &elm, &seqrel);

	if (check_permissions &&
		pg_class_aclcheck(relid, GetUserId(),
						  ACL_USAGE | ACL_UPDATE) != ACLCHECK_OK)
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("permission denied for auto_increment %s",
						RelationGetRelationName(seqrel))));

	set_auto_increment(elm, seqrel, false, 0, logit);
	relation_close(seqrel, NoLock);
	ReleaseSysCache(pgstuple);
	last_used_seq = elm;
	return elm->last;
}
/* End - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */

int64
nextval_internal(Oid relid, bool check_permissions)
{
	SeqTable	elm;
	Relation	seqrel;
	Buffer		buf;
	Page		page;
	HeapTuple	pgstuple;
	Form_pg_sequence pgsform;
	HeapTupleData seqdatatuple;
	Form_pg_sequence_data seq;
	int64		incby,
				maxv,
				minv,
				cache,
				log,
				fetch,
				last;
	int64		result,
				next,
				rescnt = 0;
	bool		cycle;
	bool		logit = false;
	/* Begin - ReqID:SRS-SQL-SEQ */
	bool		isScale = false;
	bool		isExtend = false;
	bool		isSession = false;
	int 		maxvalue_bits;
	int			minvalue_bits;
	char		maxstr[MAXINT8LEN + 1];

	pgstuple = SearchSysCache1(SEQRELID, ObjectIdGetDatum(relid));
	if (!HeapTupleIsValid(pgstuple))
		elog(ERROR, "cache lookup failed for sequence %u", relid);
	pgsform = (Form_pg_sequence) GETSTRUCT(pgstuple);
	isScale = seq_is_scale(pgsform->flags);
	isExtend = seq_is_extend(pgsform->flags);
	isSession = seq_is_session(pgsform->flags);
	maxvalue_bits = pg_lltoa((int64)Abs(pgsform->seqmax), maxstr);
	minvalue_bits = pg_lltoa((int64)Abs(pgsform->seqmin), maxstr);
	maxvalue_bits = minvalue_bits > maxvalue_bits ? minvalue_bits : maxvalue_bits;

	/* open and lock sequence */
	init_sequence(relid, &elm, &seqrel);

	if(isSession)
	{
		if (!elm->last_valid)
		{
			seq = read_seq_tuple(seqrel, &buf, &seqdatatuple);
			elm->last = seq->last_value;
			elm->last_valid = true;

			relation_close(seqrel, NoLock);
			ReleaseSysCache(pgstuple);
			UnlockReleaseBuffer(buf);

			return elm->last;
		}

		elm->increment = pgsform->seqincrement;
		result = elm->last + elm->increment;
		if(result > pgsform->seqmax && !pgsform->seqcycle)
			elog(ERROR, "sequence %s.nextval exceeds MAXVALUE and cannot be instantiated",get_rel_name(relid));
		else if(result < pgsform->seqmin && !pgsform->seqcycle)
			elog(ERROR, "sequence %s.nextval goes below MINVALUE and cannot be instantiated",get_rel_name(relid));
		else if(result > pgsform->seqmax && pgsform->seqcycle && pgsform->seqincrement > 0)
			elm->last = pgsform->seqmin;
		else if(result < pgsform->seqmin && pgsform->seqcycle && pgsform->seqincrement < 0)
			elm->last = pgsform->seqmax;
		else
			elm->last += elm->increment;

		relation_close(seqrel, NoLock);
		ReleaseSysCache(pgstuple);

		return elm->last;
	}
	/* End - ReqID:SRS-SQL-SEQ */

	/* Begin - Bug#z6158*/
	/* if se_feature.mode is on, we call our check, otherwise call orginal check */
	if (check_permissions && 
		((se_feature_mode && use_sequence_check_hook) ?
			(!use_sequence_check_hook(elm->relid, GetUserId(),
									ACL_USAGE | ACL_UPDATE)) :
			(pg_class_aclcheck(elm->relid, GetUserId(),
								ACL_USAGE | ACL_UPDATE) != ACLCHECK_OK)))
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("permission denied for sequence %s",
						RelationGetRelationName(seqrel))));
	/* End - Bug#z6158*/

	/* read-only transactions may only modify temp sequences */
	if (!RELATION_IS_TEMP_ON_CURRENT_SESSION(seqrel))
		PreventCommandIfReadOnly("nextval()");

	/*
	 * Forbid this during parallel operation because, to make it work, the
	 * cooperating backends would need to share the backend-local cached
	 * sequence information.  Currently, we don't support that.
	 */
	PreventCommandIfParallelMode("nextval()");

	if (elm->last != elm->cached)	/* some numbers were cached */
	{
		Assert(elm->last_valid);
		Assert(elm->increment != 0);
		elm->last += elm->increment;
		relation_close(seqrel, NoLock);
		/* Begin - ReqID:SRS-SQL-SEQ */
		ReleaseSysCache(pgstuple);
		if (isScale)
		{
			int64	instanc_id;
			int64	sessionid;
			char	str[MAXINT8LEN + 1];
			int		len;

			/* Check whether the scale value is cached */
			if (!elm->scale_value)
			{
				if (!scale_value)
				{
					instanc_id = GetSystemIdentifier();
					instanc_id = instanc_id%100 + 100;
					if (!session_id)
						session_id = get_sessionid();
					sessionid = session_id;
					sessionid = sessionid%1000;
					last_used_seq = elm;
					if (seq_scale_fixed)
						scale_value = 199999;
					else
						scale_value = instanc_id * pow(10, 3) + sessionid;
				}

				if (isExtend)
				{
					if (elm->last >= 0)
						elm->scale_value = (int64)(scale_value * (int64)pow(10, maxvalue_bits));
					else
						elm->scale_value = -(int64)(scale_value * (int64)pow(10, maxvalue_bits));
				}
				else
				{
					if(elm->last >= 0)
						elm->scale_value = scale_value * (int64)pow(10, maxvalue_bits - 6);
					else
						elm->scale_value = -(scale_value * (int64)pow(10, maxvalue_bits - 6));
				}
			}

			if (isExtend)
			{
				if (elm->last > pgsform->seqmax && !pgsform->seqcycle)
					elog(ERROR, "sequence %s.nextval exceeds MAXVALUE and cannot be instantiated",get_rel_name(relid));
				else if(elm->last < pgsform->seqmin && !pgsform->seqcycle)
					elog(ERROR, "sequence %s.nextval goes below MINVALUE and cannot be instantiated",get_rel_name(relid));
				else if(elm->last > pgsform->seqmax && pgsform->seqcycle && pgsform->seqincrement > 0)
					elm->last = pgsform->seqmin;
				else if(elm->last < pgsform->seqmin && pgsform->seqcycle && pgsform->seqincrement < 0)
					elm->last = pgsform->seqmax;

				result = elm->scale_value + elm->last;
				return result;
			}
			else
			{
				if (elm->last > pgsform->seqmax && !pgsform->seqcycle)
					elog(ERROR, "sequence %s.nextval exceeds MAXVALUE and cannot be instantiated",get_rel_name(relid));
				else if(elm->last < pgsform->seqmin && !pgsform->seqcycle)
					elog(ERROR, "sequence %s.nextval goes below MINVALUE and cannot be instantiated",get_rel_name(relid));
				else if(elm->last > pgsform->seqmax && pgsform->seqcycle && pgsform->seqincrement > 0)
					elm->last = pgsform->seqmin;
				else if(elm->last < pgsform->seqmin && pgsform->seqcycle && pgsform->seqincrement < 0)
					elm->last = pgsform->seqmax;

				len = pg_lltoa((int64)Abs(elm->last), str);
				if (maxvalue_bits - len - 6 < 0)
					ereport(ERROR,
						(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
						errmsg("NEXTVAL cannot be instantiated for %s. Widen the sequence by %d digits or alter sequence with SCALE EXTEND",
								RelationGetRelationName(seqrel), 6 + len - maxvalue_bits)));

				result = elm->scale_value + elm->last;
				return result;
			}
		}
		/* End - ReqID:SRS-SQL-SEQ */
		last_used_seq = elm;
		return elm->last;
	}

	incby = pgsform->seqincrement;
	maxv = pgsform->seqmax;
	minv = pgsform->seqmin;
	cache = pgsform->seqcache;
	cycle = pgsform->seqcycle;
	ReleaseSysCache(pgstuple);

	/* lock page' buffer and read tuple */
	seq = read_seq_tuple(seqrel, &buf, &seqdatatuple);
	page = BufferGetPage(buf);

	elm->increment = incby;
	last = next = result = seq->last_value;
	fetch = cache;
	log = seq->log_cnt;

	/* Begin - BUG#Z026 */
	if(result > maxv && compatible_mode == ORA_MODE)
		elog(ERROR, "sequence %s.nextval exceeds MAXVALUE and cannot be instantiated",get_rel_name(relid));
	/* End - BUG#Z026 */

	if (!seq->is_called)
	{
		rescnt++;				/* return last_value if not is_called */
		fetch--;
	}

	/*
	 * Decide whether we should emit a WAL log record.  If so, force up the
	 * fetch count to grab SEQ_LOG_VALS more values than we actually need to
	 * cache.  (These will then be usable without logging.)
	 *
	 * If this is the first nextval after a checkpoint, we must force a new
	 * WAL record to be written anyway, else replay starting from the
	 * checkpoint would fail to advance the sequence past the logged values.
	 * In this case we may as well fetch extra values.
	 */
	if (log < fetch || !seq->is_called)
	{
		/* forced log to satisfy local demand for values */
		fetch = log = fetch + SEQ_LOG_VALS;
		logit = true;
	}
	else
	{
		XLogRecPtr	redoptr = GetRedoRecPtr();

		if (PageGetLSN(page) <= redoptr)
		{
			/* last update of seq was before checkpoint */
			fetch = log = fetch + SEQ_LOG_VALS;
			logit = true;
		}
	}

	while (fetch)				/* try to fetch cache [+ log ] numbers */
	{
		/*
		 * Check MAXVALUE for ascending sequences and MINVALUE for descending
		 * sequences
		 */
		if (incby > 0)
		{
			/* ascending sequence */
			if ((maxv >= 0 && next > maxv - incby) ||
				(maxv < 0 && next + incby > maxv))
			{
				if (rescnt > 0)
					break;		/* stop fetching */
				if (!cycle)
				{
					char		buf[100];

					snprintf(buf, sizeof(buf), INT64_FORMAT, maxv);
					ereport(ERROR,
							(errcode(ERRCODE_SEQUENCE_GENERATOR_LIMIT_EXCEEDED),
							 errmsg("nextval: reached maximum value of sequence \"%s\" (%s)",
									RelationGetRelationName(seqrel), buf)));
				}
				next = minv;
			}
			else
				next += incby;
		}
		else
		{
			/* descending sequence */
			if ((minv < 0 && next < minv - incby) ||
				(minv >= 0 && next + incby < minv))
			{
				if (rescnt > 0)
					break;		/* stop fetching */
				if (!cycle)
				{
					char		buf[100];

					snprintf(buf, sizeof(buf), INT64_FORMAT, minv);
					ereport(ERROR,
							(errcode(ERRCODE_SEQUENCE_GENERATOR_LIMIT_EXCEEDED),
							 errmsg("nextval: reached minimum value of sequence \"%s\" (%s)",
									RelationGetRelationName(seqrel), buf)));
				}
				next = maxv;
			}
			else
				next += incby;
		}
		fetch--;
		if (rescnt < cache)
		{
			log--;
			rescnt++;
			last = next;
			if (rescnt == 1)	/* if it's first result - */
				result = next;	/* it's what to return */
		}
	}

	log -= fetch;				/* adjust for any unfetched numbers */
	Assert(log >= 0);

	/* save info in local cache */
	elm->last = result;			/* last returned number */
	elm->cached = last;			/* last fetched number */
	elm->last_valid = true;

	last_used_seq = elm;

	/*
	 * If something needs to be WAL logged, acquire an xid, so this
	 * transaction's commit will trigger a WAL flush and wait for syncrep.
	 * It's sufficient to ensure the toplevel transaction has an xid, no need
	 * to assign xids subxacts, that'll already trigger an appropriate wait.
	 * (Have to do that here, so we're outside the critical section)
	 */
	if (logit && RelationNeedsWAL(seqrel))
		GetTopTransactionId();

	/* ready to change the on-disk (or really, in-buffer) tuple */
	START_CRIT_SECTION();

	/*
	 * We must mark the buffer dirty before doing XLogInsert(); see notes in
	 * SyncOneBuffer().  However, we don't apply the desired changes just yet.
	 * This looks like a violation of the buffer update protocol, but it is in
	 * fact safe because we hold exclusive lock on the buffer.  Any other
	 * process, including a checkpoint, that tries to examine the buffer
	 * contents will block until we release the lock, and then will see the
	 * final state that we install below.
	 */
	MarkBufferDirty(buf);

	/* XLOG stuff */
	if (logit && RelationNeedsWAL(seqrel))
	{
		xl_seq_rec	xlrec;
		XLogRecPtr	recptr;

		/*
		 * We don't log the current state of the tuple, but rather the state
		 * as it would appear after "log" more fetches.  This lets us skip
		 * that many future WAL records, at the cost that we lose those
		 * sequence values if we crash.
		 */
		XLogBeginInsert();
		XLogRegisterBuffer(0, buf, REGBUF_WILL_INIT);

		/* set values that will be saved in xlog */
		seq->last_value = next;
		seq->is_called = true;
		seq->log_cnt = 0;

		xlrec.node = seqrel->rd_node;

		XLogRegisterData((char *) &xlrec, sizeof(xl_seq_rec));
		XLogRegisterData((char *) seqdatatuple.t_data, seqdatatuple.t_len);

		recptr = XLogInsert(RM_SEQ_ID, XLOG_SEQ_LOG);

		PageSetLSN(page, recptr);
	}

	/* Now update sequence tuple to the intended final state */
	seq->last_value = last;		/* last fetched number */
	seq->is_called = true;
	seq->log_cnt = log;			/* how much is logged */

	END_CRIT_SECTION();

	UnlockReleaseBuffer(buf);

	relation_close(seqrel, NoLock);

	/* Begin - ReqID:SRS-SQL-SEQ */
	if (isScale)
	{
		int64	instanc_id;
		int64	sessionid;
		char	str[MAXINT8LEN + 1];
		int		len;

		/* Check whether the scale value is cached */
		if (!elm->scale_value)
		{
			if (!scale_value)
			{
				instanc_id = GetSystemIdentifier();
				instanc_id = instanc_id%100 + 100;
				if (!session_id)
					session_id = get_sessionid();
				sessionid = session_id;
				sessionid = sessionid%1000;
				if (seq_scale_fixed)
					scale_value = 199999;
				else
					scale_value = instanc_id * pow(10, 3) + sessionid;
			}

			if (isExtend)
			{
				if(result >= 0)
					elm->scale_value = (int64)(scale_value * (int64)pow(10, maxvalue_bits));
				else
					elm->scale_value = -(int64)(scale_value * (int64)pow(10, maxvalue_bits));
			}
			else
			{
				if(result >= 0)
					elm->scale_value = (int64)(scale_value * (int64)pow(10, maxvalue_bits - 6));
				else
					elm->scale_value = -(int64)(scale_value * (int64)pow(10, maxvalue_bits - 6));
			}
		}
		if (isExtend)
		{
			result = elm->scale_value + elm->last;
			return result;
		}
		else
		{
			len = pg_lltoa((int64)Abs(result), str);
			if (len > maxvalue_bits)
				elog(ERROR, "result is bigger than maxvalue");
			if (maxvalue_bits - len - 6 < 0)
				ereport(ERROR,
					(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
					errmsg("NEXTVAL cannot be instantiated for %s. Widen the sequence by %d digits or alter sequence with SCALE EXTEND",
							RelationGetRelationName(seqrel), 6 + len - maxvalue_bits)));

			result = elm->scale_value + elm->last;
			return result;
		}
	}
	/* End - ReqID:SRS-SQL-SEQ */
	return result;
}

/* Begin - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
static AttrNumber
get_rel_auto_increment_attr(Oid tableoid)
{
	Relation rel;
	TupleDesc tupleDesc;
	AttrNumber natts;
	AttrNumber autoincAttnum = 0;

	rel = table_open(tableoid, AccessShareLock);
	tupleDesc = RelationGetDescr(rel);
	natts = tupleDesc->natts;

	for (autoincAttnum = 1; autoincAttnum <= natts; autoincAttnum++)
	{
		Form_pg_attribute attr = TupleDescAttr(tupleDesc, autoincAttnum - 1);
		if (attr->attidentity == ATTRIBUTE_IDENTITY_AUTO_INCREMENT)
		{
			break;
		}
	}
	table_close(rel, AccessShareLock);

	/* set invalid attnum */
	if (autoincAttnum > natts)
	{
		autoincAttnum = -1;
	}
	return autoincAttnum;
}
/* set next auto_increment value*/
void
set_auto_nextval(Oid tableoid, uint64 nextval)
{
	HeapTupleData seqdatatuple;
	Relation seqrel;
	Buffer buf;
	Form_pg_sequence_data seq;
	Oid seqoid;

	seqoid = getIdentityAutoIncrement(tableoid, get_rel_auto_increment_attr(tableoid), false);

	seqrel = relation_open(seqoid, RowExclusiveLock);
	seq = read_seq_tuple(seqrel, &buf, &seqdatatuple);
	seq->next_value = seq->next_value > nextval ? seq->next_value : nextval;
	/* Bug - #z6163 */
	/* when modifying to insert values, table without inserted date can modifying initial values;
	 * while tables with inserted data can not be set to values less than the last inseted value.
	 */
	if (!nextval && !seq->next_value)
	{
		seq->last_value = 1;
	}
	relation_close(seqrel, RowExclusiveLock);
	UnlockReleaseBuffer(buf);

	return;
}

/* set auto_increment value to seqtable
 * displayinsert: insert (autoincvalue)
 * autoincvalue:  
 * logit       : set wal
 * temptable   : insert temp table
 */
static void
set_auto_increment(SeqTable elm, Relation seqrel, bool displayinsert, uint64 autoincvalue, bool logit)
{
	Buffer buf;
	Page page;
	HeapTupleData seqdatatuple;
	Form_pg_sequence_data seq;
	uint64 nextautoinc = 0;
	uint64 curautoinc = 0;
	uint64 lastautoinc = 0;

	/* lock page' buffer and read tuple */
	seq = read_seq_tuple(seqrel, &buf, &seqdatatuple);
	page = BufferGetPage(buf);

	if (displayinsert)
	{
		/* when displaying the inserted data, select the larger value of 
		 * the inserted value and the sequence inserted value
		 */
		if (autoincvalue && (autoincvalue < seq->last_value || autoincvalue < 0))
		{
			lastautoinc = autoincvalue;
			UnlockReleaseBuffer(buf);
			return;
		}
		lastautoinc = seq->next_value > seq->last_value ? seq->next_value : seq->last_value;
		lastautoinc = autoincvalue > lastautoinc ? autoincvalue : lastautoinc;
	}
	else
	{
		/* nextval valid insert  nextval */
		lastautoinc = seq->next_value > seq->last_value ? seq->next_value : seq->last_value;
	}

	compute_cur_next_value(lastautoinc, &curautoinc, &nextautoinc);

	START_CRIT_SECTION();
	MarkBufferDirty(buf);

	/* XLOG stuff */
	if (logit && RelationNeedsWAL(seqrel))
	{
		xl_seq_rec xlrec;
		XLogRecPtr recptr;

		/*
		 * We don't log the current state of the tuple, but rather the state
		 * as it would appear after "log" more fetches.  This lets us skip
		 * that many future WAL records, at the cost that we lose those
		 * sequence values if we crash.
		 */
		XLogBeginInsert();
		XLogRegisterBuffer(0, buf, REGBUF_WILL_INIT);

		/* set values that will be saved in xlog */
		seq->last_value = curautoinc;
		seq->is_called = true;
		seq->log_cnt = 0;

		xlrec.node = seqrel->rd_node;

		XLogRegisterData((char *)&xlrec, sizeof(xl_seq_rec));
		XLogRegisterData((char *)seqdatatuple.t_data, seqdatatuple.t_len);

		recptr = XLogInsert(RM_SEQ_ID, XLOG_SEQ_LOG);

		PageSetLSN(page, recptr);
	}

	elm->last_valid = true;
	elm->last = curautoinc;

	/* Now update sequence tuple to the intended final state */
	seq->last_value = seq->last_value > curautoinc ? seq->last_value : curautoinc; /* last fetched number */
	seq->next_value = nextautoinc;
	seq->is_called = true;
	seq->log_cnt = seq->log_cnt; /* how much is logged */

	END_CRIT_SECTION();
	UnlockReleaseBuffer(buf);

	/* Bug - #z6162 */
	/* when display the insertion of non-empty and non-zero values
	 * the insertion of lastvalues is not recorded
	 */
	if (!(autoincvalue && displayinsert))
	{
		mys_g_last_seqval = (uint64)seq->last_value;
	}
	return;
}

/* comppute current insert auto_increment value*/
static uint64
compute_current_autoinc(uint64 nr)
{
	const uint64 save_nr = nr;

	if (auto_increment_increment == 1)
	{
		nr = nr + 1; // optimization of the formula below
	}
	else
	{
		/* auto_increment_increment auto_increment_offset
		 * compute auto increment values : 
		 *      the auto_increment_offset plus the multiple of the auto_increment_increment
		 * for example: 
		 *      auto_increment_increment = 10  auto_increment_offset = 3
		 *      the valid values are 3 , 13, 23, 33 ...
		 */
		nr = (((nr + auto_increment_increment - auto_increment_offset)) / (uint64)auto_increment_increment);
		nr = (nr * (uint64)auto_increment_increment + auto_increment_offset);
	}

	if (unlikely(nr <= save_nr))
	{
		return UINT64_MAX;
	}

	return nr;
}

/* comppute next insert auto_increment value
 * compute_next_autoinc
 *  uint64	current: Current value 
 *	uint64	step: AUTOINC increment step 
 *	uint64	offset: AUTOINC offset 
 *	uint64	max_value: max value for type 
 */
static uint64
compute_next_autoinc(uint64 current, uint64 step, uint64 offset, uint64 max_value)
{
	uint64 next_value;

	/* According to MySQL documentation, if the offset is greater than
	the step then the offset is ignored. */
	if (offset > step)
	{
		offset = 0;
	}

	/* Check for overflow. Current can be > max_value if the value is
	in reality a negative value. */
	if (step >= max_value || offset > max_value || current >= max_value )
	{
		next_value = max_value;
	}
	else
	{
		uint64 free = max_value - current;

		if (free < offset || free - offset <= step)
		{
			next_value = max_value;
		}
		else
		{
			next_value = 0;
		}
	}

	if (next_value == 0)
	{
		uint64 next;

		if (current > offset)
		{
			next = (current - offset) / step;
		}
		else
		{
			next = (offset - current) / step;
		}
		next_value = next * step;

		/* Check for overflow */
		if (max_value - next_value >= step)
		{
			next_value += step;

			if (max_value - next_value >= offset)
			{
				next_value += offset;
			}
			else
			{
				next_value = max_value;
			}
		}
		else
		{
			next_value = max_value;
		}
	}

	return (next_value);
}

/* comppute insert and next insert auto_increment value*/
static void
compute_cur_next_value(uint64 lastautoinc, uint64 *curautoinc, uint64 *nextautoinc)
{
	if (auto_increment_increment > 1 && lastautoinc < UINT64_MAX)
	{
		uint64 diff = UINT64_MAX - lastautoinc;
		/* Check for overflow */
		if (auto_increment_increment <= diff)
		{
			uint64 prev_auto_inc = lastautoinc;
			lastautoinc = ((lastautoinc - 1) + auto_increment_increment - auto_increment_offset) / auto_increment_increment;
			lastautoinc = lastautoinc * auto_increment_increment + auto_increment_offset;

			/* If autoinc exceeds the col_max_value then reset
			to old autoinc value. Because in case of non-strict
			sql mode, boundary value is not considered as error. */
			if (lastautoinc >= UINT64_MAX)
			{
				lastautoinc = prev_auto_inc;
			}
		}
	}

	*curautoinc = compute_current_autoinc(lastautoinc - 1);
	*nextautoinc = compute_next_autoinc(lastautoinc, auto_increment_increment, auto_increment_offset, UINT64_MAX);

	/* Bug - #z6164 */
	/* when the next calculated inserted value is consistent
	 * with the current inserted value, the next inserted value is recalculated
	 */
	if (*curautoinc == *nextautoinc)
	{
		*nextautoinc = compute_next_autoinc(*nextautoinc, auto_increment_increment, auto_increment_offset, UINT64_MAX);
	}
	return;
}

/* get show create table auto_increment value */
uint64
get_show_autoinc(Oid tableoid)
{
	HeapTupleData seqdatatuple;
	Relation seqrel;
	Buffer buf;
	Form_pg_sequence_data seq;
	Oid seqoid;
	uint64 result = 0;
	AttrNumber attnum = get_rel_auto_increment_attr(tableoid);

	if (attnum == -1)
	{
		return 0;
	}

	seqoid = getIdentityAutoIncrement(tableoid, attnum, false);
	seqrel = relation_open(seqoid, AccessShareLock);
	seq = read_seq_tuple(seqrel, &buf, &seqdatatuple);
	result = seq->next_value ? seq->next_value : seq->last_value;
	relation_close(seqrel, AccessShareLock);
	UnlockReleaseBuffer(buf);

	return result;
}
/* End - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */

Datum
currval_oid(PG_FUNCTION_ARGS)
{
	Oid			relid = PG_GETARG_OID(0);
	int64		result;
	SeqTable	elm;
	Relation	seqrel;
	/* Begin - ReqID:SRS-SQL-SEQ */
	HeapTuple	pgstuple;
	Form_pg_sequence pgsform;
	bool		isScale = false;
	bool		isExtend = false;
	bool		isSession = false;
	int			maxvalue_bits;
	int			minvalue_bits;
	char		maxstr[MAXINT8LEN + 1];
	Buffer		buf;
	Form_pg_sequence_data	seq;
	HeapTupleData	seqdatatuple;
	/* End - ReqID:SRS-SQL-SEQ */

	/* open and lock sequence */
	init_sequence(relid, &elm, &seqrel);

	/* Begin - ReqID:SRS-SQL-SEQ */
	pgstuple = SearchSysCache1(SEQRELID, ObjectIdGetDatum(relid));
	if (!HeapTupleIsValid(pgstuple))
		elog(ERROR, "cache lookup failed for sequence %u", relid);
	pgsform = (Form_pg_sequence) GETSTRUCT(pgstuple);
	isSession = seq_is_session(pgsform->flags);
	maxvalue_bits = pg_lltoa((int64)Abs(pgsform->seqmax), maxstr);
	minvalue_bits = pg_lltoa((int64)Abs(pgsform->seqmin), maxstr);
	maxvalue_bits = minvalue_bits > maxvalue_bits ? minvalue_bits : maxvalue_bits;
	isScale = seq_is_scale(pgsform->flags);
	isExtend = seq_is_extend(pgsform->flags);

	seq = read_seq_tuple(seqrel, &buf, &seqdatatuple);

	if (isSession)
	{
		if (!elm->last_valid)
			ereport(ERROR,
					(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
					errmsg("currval of sequence \"%s\" is not yet defined in this session",
							RelationGetRelationName(seqrel))));
		relation_close(seqrel, NoLock);
		ReleaseSysCache(pgstuple);
		UnlockReleaseBuffer(buf);

		PG_RETURN_INT64(elm->last);
	}
	ReleaseSysCache(pgstuple);
	/* End - ReqID:SRS-SQL-SEQ */

	/* Begin - Bug#z6158*/
	/* if se_feature.mode is on, we call our check, otherwise call orginal check */
	if ((se_feature_mode && use_sequence_check_hook)?
		(!use_sequence_check_hook(elm->relid, GetUserId(),
								  ACL_SELECT | ACL_USAGE)) :
		(pg_class_aclcheck(elm->relid, GetUserId(),
						   ACL_SELECT | ACL_USAGE) != ACLCHECK_OK))
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("permission denied for sequence %s",
						RelationGetRelationName(seqrel))));
	/* End - Bug#z6158*/

	if (!elm->last_valid)
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("currval of sequence \"%s\" is not yet defined in this session",
						RelationGetRelationName(seqrel))));

	result = elm->last;

	relation_close(seqrel, NoLock);
	UnlockReleaseBuffer(buf);	/* ReqID:SRS-SQL-SEQ */

	/* Begin - ReqID:SRS-SQL-SEQ */
	if (isScale && seq->is_called)
	{
		int64	instanc_id;
		int64	sessionid;

		/* Check whether the scale value is cached */
		if (!elm->scale_value)
		{
			if (!scale_value)
			{
				instanc_id = GetSystemIdentifier();
				instanc_id = instanc_id%100 + 100;
				sessionid = session_id%1000;
				if (!session_id)
					session_id = get_sessionid();

				if (seq_scale_fixed)
					scale_value = 199999;
				else
					scale_value = instanc_id * pow(10, 3) + sessionid;
			}

			if (isExtend)
			{
				if (result >= 0)
					elm->scale_value = (int64)(scale_value * (int64)pow(10, maxvalue_bits));
				else
					elm->scale_value = -(int64)(scale_value * (int64)pow(10, maxvalue_bits));
			}
			else
			{
				if (result >= 0)
					elm->scale_value = (int64)(scale_value * (int64)pow(10, maxvalue_bits - 6));
				else
					elm->scale_value = -(int64)(scale_value * (int64)pow(10, maxvalue_bits - 6));
			}
		}
		if (isExtend)
		{
			result = elm->scale_value + result;

			PG_RETURN_INT64(result);
		}
		else
		{
			result = elm->scale_value + result;

			PG_RETURN_INT64(result);
		}
	}

	/* End - ReqID:SRS-SQL-SEQ */
	PG_RETURN_INT64(result);
}

Datum
lastval(PG_FUNCTION_ARGS)
{
	Relation	seqrel;
	int64		result;

	if (last_used_seq == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("lastval is not yet defined in this session")));

	/* Someone may have dropped the sequence since the last nextval() */
	if (!SearchSysCacheExists1(RELOID, ObjectIdGetDatum(last_used_seq->relid)))
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("lastval is not yet defined in this session")));

	seqrel = lock_and_open_sequence(last_used_seq);

	/* nextval() must have already been called for this sequence */
	Assert(last_used_seq->last_valid);

	/* Begin - Bug#z6158*/
	/* if se_feature.mode is on, we call our check, otherwise call orginal check */
	if ((se_feature_mode && use_sequence_check_hook)?
		(!use_sequence_check_hook(last_used_seq->relid, GetUserId(),
										ACL_SELECT | ACL_USAGE)) :
		(pg_class_aclcheck(last_used_seq->relid, GetUserId(),
						   ACL_SELECT | ACL_USAGE) != ACLCHECK_OK))
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("permission denied for sequence %s",
						RelationGetRelationName(seqrel))));
	/* End - Bug#z6158*/

	result = last_used_seq->last;
	relation_close(seqrel, NoLock);

	PG_RETURN_INT64(result);
}
/* Begin - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
Datum
mys_get_lastval(PG_FUNCTION_ARGS)
{
	PG_RETURN_UINT64(mys_g_last_seqval);
}

Datum
mys_set_lastval(PG_FUNCTION_ARGS)
{
	uint64 result = 0;
	if (PG_ARGISNULL(0))
	{
		mys_g_last_seqval = 0;
		PG_RETURN_NULL();
	}

	result = PG_GETARG_INT64(0);
	mys_g_last_seqval = (uint64)result;
	PG_RETURN_UINT64(result);
}

/* mys_set_insertval
 * mys_set_insertval params: 
 * reloid : table oid
 * attnum : table auto_increment col num
 * colnotnull : auto_increment is not null flag
 * valuenullordefault :
 * 1 : insert into .. (null)    :AUTO_INCREMENT_INSERT_NULL       (1)
 * 2 : insert into .. (default) :AUTO_INCREMENT_INSERT_DEFAULT    (2)
 * 3 : insert into .. (0)       :AUTO_INCREMENT_INSERT_ZERO       (3)
 * value : insert auto_increment value
 */
Datum
mys_set_insertval(PG_FUNCTION_ARGS)
{
	SeqTable elm;
	Relation seqrel;
	Oid reloid = PG_GETARG_OID(0);
	int16 attnum = PG_GETARG_INT16(1);
	bool colnotnull = PG_GETARG_BOOL(2);
	int32 valuenullordefault = PG_GETARG_INT32(3);
	uint64 value = 0;
	Numeric valuenum = PG_GETARG_NUMERIC(4);
	bool isreturnnull = false;
	Oid seqoid = InvalidOid;

	if (DatumGetBool(DirectFunctionCall2(numeric_gt, NumericGetDatum(valuenum),
										 NumericGetDatum(uint64_to_numeric(PG_UINT64_MAX)))))
	{
		ereport(ERROR, (errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE), errmsg("Failed to read auto-increment value from storage engine")));
	}
	else if (DatumGetBool(DirectFunctionCall2(numeric_gt, NumericGetDatum(valuenum),
											  NumericGetDatum(uint64_to_numeric(PG_INT64_MAX)))))
	{
		ereport(WARNING, (errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE), errmsg("auto_increment max is 9223372036854775807")));
		value = 9223372036854775807;
	}
	else
	{
		value = DatumGetInt64(DirectFunctionCall1(numeric_int8, NumericGetDatum(valuenum)));
		if ((int64)value < 0)
			PG_RETURN_INT64(value);
	}

	seqoid = getIdentityAutoIncrement(reloid, attnum, false);

	/* insert value: (NULL) . (default) . (0) 
	*  null col:
	* ---------------------------------------------------------------------------
	* | sql_mode          MODE_NO_AUTO_VALUE_ON_ZERO       default_sql_mode
	* | (NULL)                   NULL                           NULL
	* | (default)                NULL                           NULL
	* | (0)                       0                            lastval ++
	* ---------------------------------------------------------------------------
	* ===========================================================================
	*  not null col:
	* ---------------------------------------------------------------------------
	* | sql_mode          MODE_NO_AUTO_VALUE_ON_ZERO       default_sql_mode
	* | (NULL)                   NULL                          lastval ++
	* | (default)                 0                            lastval ++
	* | (0)                       0                            lastval ++
	* ---------------------------------------------------------------------------
	*/
	if (AUTO_INCREMENT_VALUE_ZERO == value)
	{
		if (has_sql_mode(MODE_NO_AUTO_VALUE_ON_ZERO))
		{
			if (colnotnull)
			{
				if (valuenullordefault == AUTO_INCREMENT_INSERT_DEFAULT || 
					valuenullordefault == AUTO_INCREMENT_INSERT_ZERO)
				{
					PG_RETURN_UINT64(AUTO_INCREMENT_VALUE_ZERO);
				}
			}
			else
			{
				if (valuenullordefault == AUTO_INCREMENT_INSERT_NULL || 
					valuenullordefault == AUTO_INCREMENT_INSERT_DEFAULT)
				{
					PG_RETURN_NULL();
				}
				if (valuenullordefault == AUTO_INCREMENT_INSERT_ZERO)
				{
					PG_RETURN_UINT64(AUTO_INCREMENT_VALUE_ZERO);
				}
			}
		}
		else
		{
			if (!colnotnull)
			{
				if (valuenullordefault == AUTO_INCREMENT_INSERT_NULL || 
					valuenullordefault == AUTO_INCREMENT_INSERT_DEFAULT)
				{
					isreturnnull = true;
				}
			}
		}
	}

	/* open and lock sequence */
	init_sequence(seqoid, &elm, &seqrel);
	set_auto_increment(elm, seqrel, true, value, false);
	relation_close(seqrel, NoLock);
	last_used_seq = elm;
	if (value == AUTO_INCREMENT_VALUE_ZERO)
	{
		value = elm->last;
	}

	if (isreturnnull)
		PG_RETURN_NULL();
	PG_RETURN_INT64(value);
}
/* End - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
/*
 * Main internal procedure that handles 2 & 3 arg forms of SETVAL.
 *
 * Note that the 3 arg version (which sets the is_called flag) is
 * only for use in pg_dump, and setting the is_called flag may not
 * work if multiple users are attached to the database and referencing
 * the sequence (unlikely if pg_dump is restoring it).
 *
 * It is necessary to have the 3 arg version so that pg_dump can
 * restore the state of a sequence exactly during data-only restores -
 * it is the only way to clear the is_called flag in an existing
 * sequence.
 */
static void
do_setval(Oid relid, int64 next, bool iscalled)
{
	SeqTable	elm;
	Relation	seqrel;
	Buffer		buf;
	HeapTupleData seqdatatuple;
	Form_pg_sequence_data seq;
	HeapTuple	pgstuple;
	Form_pg_sequence pgsform;
	int64		maxv,
				minv;

	/* open and lock sequence */
	init_sequence(relid, &elm, &seqrel);

	/* Begin - Bug#z6158*/
	/* if se_feature.mode is on, we call our check, otherwise call orginal check */
	if ((se_feature_mode && use_sequence_check_hook) ?
		(!use_sequence_check_hook(elm->relid, GetUserId(), ACL_UPDATE)) :
		(pg_class_aclcheck(elm->relid, GetUserId(), ACL_UPDATE) != ACLCHECK_OK))
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("permission denied for sequence %s",
						RelationGetRelationName(seqrel))));
	/* End - Bug#z6158*/

	pgstuple = SearchSysCache1(SEQRELID, ObjectIdGetDatum(relid));
	if (!HeapTupleIsValid(pgstuple))
		elog(ERROR, "cache lookup failed for sequence %u", relid);
	pgsform = (Form_pg_sequence) GETSTRUCT(pgstuple);
	maxv = pgsform->seqmax;
	minv = pgsform->seqmin;
	ReleaseSysCache(pgstuple);

	/* read-only transactions may only modify temp sequences */
	if (!RELATION_IS_TEMP_ON_CURRENT_SESSION(seqrel))
		PreventCommandIfReadOnly("setval()");

	/*
	 * Forbid this during parallel operation because, to make it work, the
	 * cooperating backends would need to share the backend-local cached
	 * sequence information.  Currently, we don't support that.
	 */
	PreventCommandIfParallelMode("setval()");

	/* lock page' buffer and read tuple */
	seq = read_seq_tuple(seqrel, &buf, &seqdatatuple);

	if ((next < minv) || (next > maxv))
	{
		char		bufv[100],
					bufm[100],
					bufx[100];

		snprintf(bufv, sizeof(bufv), INT64_FORMAT, next);
		snprintf(bufm, sizeof(bufm), INT64_FORMAT, minv);
		snprintf(bufx, sizeof(bufx), INT64_FORMAT, maxv);
		ereport(ERROR,
				(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
				 errmsg("setval: value %s is out of bounds for sequence \"%s\" (%s..%s)",
						bufv, RelationGetRelationName(seqrel),
						bufm, bufx)));
	}

	/* Set the currval() state only if iscalled = true */
	if (iscalled)
	{
		elm->last = next;		/* last returned number */
		elm->last_valid = true;
	}

	/* In any case, forget any future cached numbers */
	elm->cached = elm->last;

	/* check the comment above nextval_internal()'s equivalent call. */
	if (RelationNeedsWAL(seqrel))
		GetTopTransactionId();

	/* ready to change the on-disk (or really, in-buffer) tuple */
	START_CRIT_SECTION();

	seq->last_value = next;		/* last fetched number */
	seq->is_called = iscalled;
	seq->log_cnt = 0;
	/* Begin - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
	if (compatible_mode == MYS_MODE)
	{
		seq->next_value = compute_current_autoinc(next);
	}
	else
	{
		seq->next_value = next + 1;
	}
	/* End - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */

	MarkBufferDirty(buf);

	/* XLOG stuff */
	if (RelationNeedsWAL(seqrel))
	{
		xl_seq_rec	xlrec;
		XLogRecPtr	recptr;
		Page		page = BufferGetPage(buf);

		XLogBeginInsert();
		XLogRegisterBuffer(0, buf, REGBUF_WILL_INIT);

		xlrec.node = seqrel->rd_node;
		XLogRegisterData((char *) &xlrec, sizeof(xl_seq_rec));
		XLogRegisterData((char *) seqdatatuple.t_data, seqdatatuple.t_len);

		recptr = XLogInsert(RM_SEQ_ID, XLOG_SEQ_LOG);

		PageSetLSN(page, recptr);
	}

	END_CRIT_SECTION();

	UnlockReleaseBuffer(buf);

	relation_close(seqrel, NoLock);
}

/*
 * Implement the 2 arg setval procedure.
 * See do_setval for discussion.
 */
Datum
setval_oid(PG_FUNCTION_ARGS)
{
	Oid			relid = PG_GETARG_OID(0);
	int64		next = PG_GETARG_INT64(1);

	do_setval(relid, next, true);

	PG_RETURN_INT64(next);
}

/*
 * Implement the 3 arg setval procedure.
 * See do_setval for discussion.
 */
Datum
setval3_oid(PG_FUNCTION_ARGS)
{
	Oid			relid = PG_GETARG_OID(0);
	int64		next = PG_GETARG_INT64(1);
	bool		iscalled = PG_GETARG_BOOL(2);

	do_setval(relid, next, iscalled);

	PG_RETURN_INT64(next);
}


/*
 * Open the sequence and acquire lock if needed
 *
 * If we haven't touched the sequence already in this transaction,
 * we need to acquire a lock.  We arrange for the lock to
 * be owned by the top transaction, so that we don't need to do it
 * more than once per xact.
 */
static Relation
lock_and_open_sequence(SeqTable seq)
{
	LocalTransactionId thislxid = MyProc->lxid;

	/* Get the lock if not already held in this xact */
	if (seq->lxid != thislxid)
	{
		ResourceOwner currentOwner;

		currentOwner = CurrentResourceOwner;
		CurrentResourceOwner = TopTransactionResourceOwner;

		LockRelationOid(seq->relid, RowExclusiveLock);

		CurrentResourceOwner = currentOwner;

		/* Flag that we have a lock in the current xact */
		seq->lxid = thislxid;
	}

	/* We now know we have the lock, and can safely open the rel */
	return relation_open(seq->relid, NoLock);
}

/*
 * Creates the hash table for storing sequence data
 */
static void
create_seq_hashtable(void)
{
	HASHCTL		ctl;

	ctl.keysize = sizeof(Oid);
	ctl.entrysize = sizeof(SeqTableData);

	seqhashtab = hash_create("Sequence values", 16, &ctl,
							 HASH_ELEM | HASH_BLOBS);
}

/*
 * Given a relation OID, open and lock the sequence.  p_elm and p_rel are
 * output parameters.
 */
static void
init_sequence(Oid relid, SeqTable *p_elm, Relation *p_rel)
{
	SeqTable	elm;
	Relation	seqrel;
	bool		found;

	/* Find or create a hash table entry for this sequence */
	if (seqhashtab == NULL)
		create_seq_hashtable();

	elm = (SeqTable) hash_search(seqhashtab, &relid, HASH_ENTER, &found);

	/*
	 * Initialize the new hash table entry if it did not exist already.
	 *
	 * NOTE: seqhashtab entries are stored for the life of a backend (unless
	 * explicitly discarded with DISCARD). If the sequence itself is deleted
	 * then the entry becomes wasted memory, but it's small enough that this
	 * should not matter.
	 */
	if (!found)
	{
		/* relid already filled in */
		elm->filenode = InvalidOid;
		elm->lxid = InvalidLocalTransactionId;
		elm->last_valid = false;
		elm->last = elm->cached = 0;
		elm->scale_value = 0;
	}

	/*
	 * Open the sequence relation.
	 */
	seqrel = lock_and_open_sequence(elm);

	if (seqrel->rd_rel->relkind != RELKIND_SEQUENCE)
		ereport(ERROR,
				(errcode(ERRCODE_WRONG_OBJECT_TYPE),
				 errmsg("\"%s\" is not a sequence",
						RelationGetRelationName(seqrel))));

	/*
	 * If the sequence has been transactionally replaced since we last saw it,
	 * discard any cached-but-unissued values.  We do not touch the currval()
	 * state, however.
	 */
	if (seqrel->rd_rel->relfilenode != elm->filenode)
	{
		elm->filenode = seqrel->rd_rel->relfilenode;
		elm->cached = elm->last;
	}
	
	/* Begin - BUG#00004709 */
	if(audit_object_hook)
	{
		(*audit_object_hook)(relid,seqrel);
	}
	/* End - BUG#00004709 */

	/* Return results */
	*p_elm = elm;
	*p_rel = seqrel;
}


/*
 * Given an opened sequence relation, lock the page buffer and find the tuple
 *
 * *buf receives the reference to the pinned-and-ex-locked buffer
 * *seqdatatuple receives the reference to the sequence tuple proper
 *		(this arg should point to a local variable of type HeapTupleData)
 *
 * Function's return value points to the data payload of the tuple
 */
static Form_pg_sequence_data
read_seq_tuple(Relation rel, Buffer *buf, HeapTuple seqdatatuple)
{
	Page		page;
	ItemId		lp;
	sequence_magic *sm;
	Form_pg_sequence_data seq;

	*buf = ReadBuffer(rel, 0);
	LockBuffer(*buf, BUFFER_LOCK_EXCLUSIVE);

	page = BufferGetPage(*buf);
	sm = (sequence_magic *) PageGetSpecialPointer(page);

	if (sm->magic != SEQ_MAGIC)
		elog(ERROR, "bad magic number in sequence \"%s\": %08X",
			 RelationGetRelationName(rel), sm->magic);

	lp = PageGetItemId(page, FirstOffsetNumber);
	Assert(ItemIdIsNormal(lp));

	/* Note we currently only bother to set these two fields of *seqdatatuple */
	seqdatatuple->t_data = (HeapTupleHeader) PageGetItem(page, lp);
	seqdatatuple->t_len = ItemIdGetLength(lp);

	/*
	 * Previous releases of Postgres neglected to prevent SELECT FOR UPDATE on
	 * a sequence, which would leave a non-frozen XID in the sequence tuple's
	 * xmax, which eventually leads to clog access failures or worse. If we
	 * see this has happened, clean up after it.  We treat this like a hint
	 * bit update, ie, don't bother to WAL-log it, since we can certainly do
	 * this again if the update gets lost.
	 */
	Assert(!(seqdatatuple->t_data->t_infomask & HEAP_XMAX_IS_MULTI));
	if (HeapTupleHeaderGetRawXmax(seqdatatuple->t_data) != InvalidTransactionId)
	{
		HeapTupleHeaderSetXmax(seqdatatuple->t_data, InvalidTransactionId);
		seqdatatuple->t_data->t_infomask &= ~HEAP_XMAX_COMMITTED;
		seqdatatuple->t_data->t_infomask |= HEAP_XMAX_INVALID;
		MarkBufferDirtyHint(*buf, true);
	}

	seq = (Form_pg_sequence_data) GETSTRUCT(seqdatatuple);

	return seq;
}

/*
 * init_params: process the options list of CREATE or ALTER SEQUENCE, and
 * store the values into appropriate fields of seqform, for changes that go
 * into the pg_sequence catalog, and fields of seqdataform for changes to the
 * sequence relation itself.  Set *need_seq_rewrite to true if we changed any
 * parameters that require rewriting the sequence's relation (interesting for
 * ALTER SEQUENCE).  Also set *owned_by to any OWNED BY option, or to NIL if
 * there is none.
 *
 * If isInit is true, fill any unspecified options with default values;
 * otherwise, do not change existing options that aren't explicitly overridden.
 *
 * Note: we force a sequence rewrite whenever we change parameters that affect
 * generation of future sequence values, even if the seqdataform per se is not
 * changed.  This allows ALTER SEQUENCE to behave transactionally.  Currently,
 * the only option that doesn't cause that is OWNED BY.  It's *necessary* for
 * ALTER SEQUENCE OWNED BY to not rewrite the sequence, because that would
 * break pg_upgrade by causing unwanted changes in the sequence's relfilenode.
 */
static void
init_params(ParseState *pstate, List *options, bool for_identity,
			bool isInit, char seq_type,	/* ReqID:SRS-SQL-IDENTCOL */
			Form_pg_sequence seqform,
			Form_pg_sequence_data seqdataform,
			bool *need_seq_rewrite,
			List **owned_by, SeqTable seqelm)	/* ReqID:SRS-SQL-SEQ */
{
	DefElem    *as_type = NULL;
	DefElem    *start_value = NULL;
	DefElem    *restart_value = NULL;
	DefElem    *increment_by = NULL;
	DefElem    *max_value = NULL;
	DefElem    *min_value = NULL;
	DefElem    *cache_value = NULL;
	DefElem    *is_cycled = NULL;
	/* Begin - ReqID:SRS-SQL-SEQ */
	DefElem	   *scale = NULL;
	DefElem	   *session = NULL;
	/* End - ReqID:SRS-SQL-SEQ */
	ListCell   *option;
	bool		reset_max_value = false;
	bool		reset_min_value = false;
	int			ordercnt = 0;	/* ReqID:SRS-SQL-IDENTCOL */
	/* Begin - ReqID:SRS-SQL-SEQ */
	int			keepcnt = 0;
	int			shardcnt = 0;
	bool		scale_flag = false;
	bool		session_flag = false;
	bool		global_flag = false;
	bool		extend_flag = false;
	bool		noscale_flag = false;
	bool		no_cache_flag = false;
	int64		old_maxvalue;
	/* End - ReqID:SRS-SQL-SEQ */

	*need_seq_rewrite = false;
	*owned_by = NIL;

	foreach(option, options)
	{
		DefElem    *defel = (DefElem *) lfirst(option);

		if (strcmp(defel->defname, "as") == 0)
		{
			if (as_type)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options"),
						 parser_errposition(pstate, defel->location)));
			as_type = defel;
			*need_seq_rewrite = true;
		}
		else if (strcmp(defel->defname, "increment") == 0)
		{
			if (increment_by)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options"),
						 parser_errposition(pstate, defel->location)));
			increment_by = defel;
			*need_seq_rewrite = true;
		}
		else if (strcmp(defel->defname, "start") == 0)
		{
			if (start_value)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options"),
						 parser_errposition(pstate, defel->location)));
			start_value = defel;
			*need_seq_rewrite = true;
		}
		else if (strcmp(defel->defname, "restart") == 0)
		{
			if (restart_value)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options"),
						 parser_errposition(pstate, defel->location)));
			restart_value = defel;
			/* Begin - ReqID:SRS-SQL-SEQ */
			if(isInit && compatible_mode == ORA_MODE)
				elog(ERROR, "CREATE SEQUENCE cannot have a RESTART clause");
			if(!isInit && restart_value->arg != NULL && compatible_mode == ORA_MODE)
				elog(ERROR, "SQL command not properly ended");
			/* End - ReqID:SRS-SQL-SEQ */
			*need_seq_rewrite = true;
		}
		else if (strcmp(defel->defname, "maxvalue") == 0)
		{
			if (max_value)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options"),
						 parser_errposition(pstate, defel->location)));
			max_value = defel;
			*need_seq_rewrite = true;
		}
		else if (strcmp(defel->defname, "minvalue") == 0)
		{
			if (min_value)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options"),
						 parser_errposition(pstate, defel->location)));
			min_value = defel;
			*need_seq_rewrite = true;
		}
		else if (strcmp(defel->defname, "cache") == 0)
		{
			if (cache_value)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options"),
						 parser_errposition(pstate, defel->location)));
			cache_value = defel;
			*need_seq_rewrite = true;
		}
		/* Begin - ReqID:SRS-SQL-SEQ */
		else if (strcmp(defel->defname, "nocache") == 0)
		{
			if (cache_value)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options"),
						 parser_errposition(pstate, defel->location)));
			no_cache_flag = true;
			*need_seq_rewrite = true;
		}
		/* End - ReqID:SRS-SQL-SEQ */
		else if (strcmp(defel->defname, "cycle") == 0)
		{
			if (is_cycled)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options"),
						 parser_errposition(pstate, defel->location)));
			is_cycled = defel;
			*need_seq_rewrite = true;
		}
		else if (strcmp(defel->defname, "owned_by") == 0)
		{
			if (*owned_by)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options"),
						 parser_errposition(pstate, defel->location)));
			*owned_by = defGetQualifiedName(defel);
		}
		else if (strcmp(defel->defname, "sequence_name") == 0)
		{
			/*
			 * The parser allows this, but it is only for identity columns, in
			 * which case it is filtered out in parse_utilcmd.c.  We only get
			 * here if someone puts it into a CREATE SEQUENCE.
			 */
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("invalid sequence option SEQUENCE NAME"),
					 parser_errposition(pstate, defel->location)));
		}
		/* Begin - ReqID:SRS-SQL-IDENTCOL */
		else if(strcmp(defel->defname, "order") == 0 || strcmp(defel->defname, "noorder") == 0)
		{
			ordercnt++;
			if(ordercnt > 1)
			{
				ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("dumplicate or conflicting ORDER/NOORDER specifications"),
					 parser_errposition(pstate, defel->location)));
			}
		}
		/* End - ReqID:SRS-SQL-IDENTCOL */
		/* Begin - ReqID:SRS-SQL-SEQ */
		else if(strcmp(defel->defname, "scale_extend") == 0)
		{
			if (scale)
			{
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						errmsg("duplicate or conflicting SCALE or EXTEND specifications"),
						parser_errposition(pstate, defel->location)));
			}
			scale = defel;
			scale_flag = true;
			extend_flag = true;
		}
		else if(strcmp(defel->defname, "scale_noextend") == 0)
		{
			if (scale)
			{
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						errmsg("duplicate or conflicting SCALE or EXTEND specifications"),
						parser_errposition(pstate, defel->location)));
			}
			scale = defel;
			scale_flag = true;
			extend_flag = false;
		}
		else if(strcmp(defel->defname, "noscale") == 0)
		{
			if (scale)
			{
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						errmsg("duplicate or conflicting SCALE or EXTEND specifications"),
						parser_errposition(pstate, defel->location)));
			}
			noscale_flag = true;
		}
		else if(strcmp(defel->defname, "session") == 0)
		{
			if (session || global_flag)
			{
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						errmsg("duplicate SESSION or GLOBAL specifications"),
						parser_errposition(pstate, defel->location)));
			}
			session_flag = true;
		}
		else if(strcmp(defel->defname, "global") == 0)
		{
			if (session || session_flag)
			{
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						errmsg("duplicate SESSION or GLOBAL specifications"),
						parser_errposition(pstate, defel->location)));
			}
			global_flag = true;
		}
		else if(strcmp(defel->defname, "keep") == 0 || strcmp(defel->defname, "nokeep") == 0)
		{
			keepcnt++;
			if(keepcnt > 1)
			{
				ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("dumplicate or conflicting KEEP/NOKEEP specifications"),
					 parser_errposition(pstate, defel->location)));
			}
		}
		else if(strcmp(defel->defname, "shard_extend") == 0 || strcmp(defel->defname, "shard_noextend") == 0
			|| strcmp(defel->defname, "noshard") == 0)
		{
			shardcnt++;
			if(keepcnt > 1)
			{
				ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("dumplicate or conflicting SHARD EXTEND/NOEXTEND specifications"),
					 parser_errposition(pstate, defel->location)));
			}
		}
		/* End - ReqID:SRS-SQL-SEQ */
		else
			elog(ERROR, "option \"%s\" not recognized",
				 defel->defname);
	}

	/*
	 * We must reset log_cnt when isInit or when changing any parameters that
	 * would affect future nextval allocations.
	 */
	if (isInit)
		seqdataform->log_cnt = 0;

	/* Begin BUG#Z250 */
	if (start_value && !isInit && !restart_value &&
			compatible_mode == ORA_MODE && !for_identity)
		elog(ERROR, "cannot alter starting sequence number");
	/* End BUG#Z250 */

	/* AS type */
	if (as_type != NULL)
	{
		Oid			newtypid = typenameTypeId(pstate, defGetTypeName(as_type));
		/* Begin - ReqID:SRS-SQL-IDENTCOL */
		if (seq_type == ATTRIBUTE_IDENTITY_ALWAYS || seq_type == ATTRIBUTE_IDENTITY_BY_DEFAULT || compatible_mode == PG_MODE
						|| compatible_mode == MYS_MODE)	/* Bug#m0000070 */	/* ReqID:SRS-MISC-MYSQL-FRAME */
		{
			if (newtypid != INT2OID &&
				newtypid != INT4OID &&
				newtypid != INT8OID && compatible_mode != MYS_MODE)
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						 for_identity
						 ? errmsg("identity column type must be smallint, integer, or bigint")
						 : errmsg("sequence type must be smallint, integer, or bigint")));
			/* Begin - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
			if (!type_is_numeric(newtypid) && compatible_mode == MYS_MODE)
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						 for_identity
							 ? errmsg("identity column type must be smallint, integer, bigint, float4, or float8")
							 : errmsg("sequence type must be smallint, integer, bigint, float4, or float8")));
			/* End - SRS-MYSQL-CREATE-TABLE-AUTO-INCREMENT */
		}
		else if(seq_type == ATTRIBUTE_IDENTITY_DEFAULT_ON_NULL || seq_type == ATTRIBUTE_ORA_IDENTITY_ALWAYS
						|| seq_type == ATTRIBUTE_ORA_IDENTITY_BY_DEFAULT || compatible_mode == ORA_MODE)	/* Bug#m0000070 */
		{
			if (newtypid != INT2OID &&
				newtypid != INT4OID &&
				newtypid != INT8OID &&
				newtypid != FLOAT4OID	&&
				newtypid != FLOAT8OID	&&
				newtypid != NUMBEROID	&&
				newtypid != NUMERICOID)
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						 for_identity
						 ? errmsg("identity column type must be a numeric type")
						 : errmsg("sequence type must be a numeric type")));
		}
		/* End - ReqID:SRS-SQL-IDENTCOL */

		if (!isInit)
		{
			/*
			 * When changing type and the old sequence min/max values were the
			 * min/max of the old type, adjust sequence min/max values to
			 * min/max of new type.  (Otherwise, the user chose explicit
			 * min/max values, which we'll leave alone.)
			 */
			if ((seqform->seqtypid == INT2OID && seqform->seqmax == PG_INT16_MAX) ||
				(seqform->seqtypid == INT4OID && seqform->seqmax == PG_INT32_MAX) ||
				/* Begin - Bug#m0000070 */
				(seqform->seqtypid == INT8OID && seqform->seqmax == PG_INT64_MAX) ||
				(seqform->seqtypid == FLOAT4OID && seqform->seqmax == PG_INT32_MAX) ||
				(seqform->seqtypid == FLOAT8OID && seqform->seqmax == PG_INT64_MAX) ||
				(seqform->seqtypid == NUMBEROID && seqform->seqmax == PG_INT64_MAX))
				/* End - Bug#m0000070 */
				reset_max_value = true;
			if ((seqform->seqtypid == INT2OID && seqform->seqmin == PG_INT16_MIN) ||
				(seqform->seqtypid == INT4OID && seqform->seqmin == PG_INT32_MIN) ||
				/* Begin - Bug#m0000070 */
				(seqform->seqtypid == INT8OID && seqform->seqmin == PG_INT64_MIN) ||
				(seqform->seqtypid == FLOAT4OID && seqform->seqmin == PG_INT32_MAX) ||
				(seqform->seqtypid == FLOAT8OID && seqform->seqmin == PG_INT64_MAX) ||
				(seqform->seqtypid == NUMBEROID && seqform->seqmin == PG_INT64_MIN))
				/* End - Bug#m0000070 */
				reset_min_value = true;
		}

		seqform->seqtypid = newtypid;
	}
	else if (isInit)
	{
		seqform->seqtypid = INT8OID;
	}

	/* INCREMENT BY */
	if (increment_by != NULL)
	{
		/* Begin - BUG#Z165 */
		if (compatible_mode == ORA_MODE)
		{
			PG_TRY();
			{
				seqform->seqincrement = defGetInt64(increment_by);
				if (seqform->seqincrement == 0)
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
							errmsg("INCREMENT must not be zero")));
				seqdataform->log_cnt = 0;
			}
			PG_CATCH();
			{
				int		errcod;
				char   *intval;

				errcod = geterrcode();
				if(errcod == ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE)
				{
					if(internal_warning)	/* BUG#Z230 */
						ereport(WARNING,
								(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
								errmsg("Increment value is out of range for data type bigint")));
					intval = strVal(increment_by->arg);
					if(*intval == '-')
						seqform->seqincrement = PG_INT64_MIN;
					else
						seqform->seqincrement = PG_INT64_MAX;
					seqdataform->log_cnt = 0;
				}
				else if (errcod == ERRCODE_INVALID_TEXT_REPRESENTATION || errcod == ERRCODE_INVALID_PARAMETER_VALUE)
				{
					PG_RE_THROW();
				}
				FlushErrorState();
			}
			PG_END_TRY();
		}
		else
		{
			seqform->seqincrement = defGetInt64(increment_by);
			if (seqform->seqincrement == 0)
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						errmsg("INCREMENT must not be zero")));
			seqdataform->log_cnt = 0;
		}
		/* End - BUG#Z165 */
	}
	else if (isInit)
	{
		seqform->seqincrement = 1;
	}

	/* CYCLE */
	if (is_cycled != NULL)
	{
		seqform->seqcycle = intVal(is_cycled->arg);
		Assert(BoolIsValid(seqform->seqcycle));
		seqdataform->log_cnt = 0;
	}
	else if (isInit)
	{
		seqform->seqcycle = false;
	}

	old_maxvalue = seqform->seqmax;	/* ReqID:SRS-SQL-SEQ */
	/* MAXVALUE (null arg means NO MAXVALUE) */
	if (max_value != NULL && max_value->arg)
	{
		/* Begin - BUG#Z165 */
		if (compatible_mode == ORA_MODE)
		{
			/* Begin - BUG#Z028 */
			PG_TRY();
			{
				seqform->seqmax = defGetInt64(max_value);
				seqdataform->log_cnt = 0;
			}
			PG_CATCH();
			{
				int		errcod;
				char   *intval;

				errcod = geterrcode();
				if(errcod == ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE)
				{
					if(internal_warning)	/* BUG#Z230 */
						ereport(WARNING,
								(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
								errmsg("Maxvalue is out of range for data type bigint")));
					intval = strVal(max_value->arg);
					if(*intval == '-')
						seqform->seqmax = PG_INT64_MIN;
					else
						seqform->seqmax = PG_INT64_MAX;
					seqdataform->log_cnt = 0;
				}
				else if (errcod == ERRCODE_INVALID_TEXT_REPRESENTATION)
				{
					PG_RE_THROW();
				}
				FlushErrorState();
			}
			PG_END_TRY();
			/* End - BUG#Z028 */
		}
		else
		{
			seqform->seqmax = defGetInt64(max_value);
			seqdataform->log_cnt = 0;
		}
		/* End - BUG#Z165 */
	}
	else if (isInit || max_value != NULL || reset_max_value)
	{
		if (seqform->seqincrement > 0 || reset_max_value)
		{
			/* ascending seq */
			if (seqform->seqtypid == INT2OID)
				seqform->seqmax = PG_INT16_MAX;
			else if (seqform->seqtypid == INT4OID)
				seqform->seqmax = PG_INT32_MAX;
			else
				seqform->seqmax = PG_INT64_MAX;
		}
		else
			seqform->seqmax = -1;	/* descending seq */
		seqdataform->log_cnt = 0;
	}

	if ((seqform->seqtypid == INT2OID && (seqform->seqmax < PG_INT16_MIN || seqform->seqmax > PG_INT16_MAX))
		|| (seqform->seqtypid == INT4OID && (seqform->seqmax < PG_INT32_MIN || seqform->seqmax > PG_INT32_MAX))
		|| (seqform->seqtypid == INT8OID && (seqform->seqmax < PG_INT64_MIN || seqform->seqmax > PG_INT64_MAX)))
	{
		char		bufx[100];

		snprintf(bufx, sizeof(bufx), INT64_FORMAT, seqform->seqmax);

		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("MAXVALUE (%s) is out of range for sequence data type %s",
						bufx, format_type_be(seqform->seqtypid))));
	}

	/* MINVALUE (null arg means NO MINVALUE) */
	if (min_value != NULL && min_value->arg)
	{
		/* Begin - BUG#Z165 */
		if (compatible_mode == ORA_MODE)
		{
			PG_TRY();
			{
				seqform->seqmin = defGetInt64(min_value);
				seqdataform->log_cnt = 0;
			}
			PG_CATCH();
			{
				int		errcod;
				char   *intval;

				errcod = geterrcode();
				if (errcod == ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE)
				{
					if(internal_warning)	/* BUG#Z230 */
						ereport(WARNING,
								(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
								errmsg("Minvalue is out of range for data type bigint")));
					intval = strVal(min_value->arg);	/* ReqID:SRS-SQL-SEQ */
					if (*intval == '-')
						seqform->seqmin = PG_INT64_MIN;
					else
						seqform->seqmin = PG_INT64_MAX;
					seqdataform->log_cnt = 0;
				}
				else if (errcod == ERRCODE_INVALID_TEXT_REPRESENTATION)
				{
					PG_RE_THROW();
				}
				FlushErrorState();
			}
			PG_END_TRY();
		}
		else
		{
			seqform->seqmin = defGetInt64(min_value);
			seqdataform->log_cnt = 0;
		}
		/* End - BUG#Z165 */
	}
	else if (isInit || min_value != NULL || reset_min_value)
	{
		if (seqform->seqincrement < 0 || reset_min_value)
		{
			/* descending seq */
			if (seqform->seqtypid == INT2OID)
				seqform->seqmin = PG_INT16_MIN;
			else if (seqform->seqtypid == INT4OID)
				seqform->seqmin = PG_INT32_MIN;
			else
				seqform->seqmin = PG_INT64_MIN;
		}
		else
			seqform->seqmin = 1;	/* ascending seq */
		seqdataform->log_cnt = 0;
	}

	if ((seqform->seqtypid == INT2OID && (seqform->seqmin < PG_INT16_MIN || seqform->seqmin > PG_INT16_MAX))
		|| (seqform->seqtypid == INT4OID && (seqform->seqmin < PG_INT32_MIN || seqform->seqmin > PG_INT32_MAX))
		|| (seqform->seqtypid == INT8OID && (seqform->seqmin < PG_INT64_MIN || seqform->seqmin > PG_INT64_MAX)))
	{
		char		bufm[100];

		snprintf(bufm, sizeof(bufm), INT64_FORMAT, seqform->seqmin);

		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("MINVALUE (%s) is out of range for sequence data type %s",
						bufm, format_type_be(seqform->seqtypid))));
	}

	/* crosscheck min/max */
	if (seqform->seqmin >= seqform->seqmax)
	{
		char		bufm[100],
					bufx[100];

		snprintf(bufm, sizeof(bufm), INT64_FORMAT, seqform->seqmin);
		snprintf(bufx, sizeof(bufx), INT64_FORMAT, seqform->seqmax);
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("MINVALUE (%s) must be less than MAXVALUE (%s)",
						bufm, bufx)));
	}

	/* START WITH */
	if (start_value != NULL)
	{
		/* Begin - BUG#Z024 */
		if (compatible_mode == ORA_MODE)
		{
			/* Begin - BUG#Z028 */
			PG_TRY();
			{
				seqform->seqstart = defGetInt64(start_value);
				seqdataform->log_cnt = 0;
				seqdataform->last_value = seqform->seqstart;
				seqdataform->is_called = false;
			}
			PG_CATCH();
			{
				int		errcod;
				char   *intval;	/* BUG#Z165 */

				errcod = geterrcode();
				if(errcod == ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE)
				{
					if(internal_warning)	/* BUG#Z230 */
						ereport(WARNING,
								(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
								errmsg("Start value is out of range for data type bigint")));
					/* Begin - BUG#Z165 */
					intval = strVal(start_value->arg);
					if(*intval == '-')
						seqform->seqstart = PG_INT64_MIN;
					else
						seqform->seqstart = PG_INT64_MAX;
					seqdataform->log_cnt = 0;
					seqdataform->last_value = seqform->seqstart;
					seqdataform->is_called = false;
					/* End - BUG#Z165 */
				}
				else if (errcod == ERRCODE_INVALID_TEXT_REPRESENTATION)
				{
					PG_RE_THROW();
				}
				FlushErrorState();
			}
			PG_END_TRY();
			/* End - BUG#Z028 */
		}
		else
		/* End - BUG#Z024 */
			seqform->seqstart = defGetInt64(start_value);
	}
	else if (isInit)
	{
		if (seqform->seqincrement > 0)
			seqform->seqstart = seqform->seqmin;	/* ascending seq */
		else
			seqform->seqstart = seqform->seqmax;	/* descending seq */
	}

	/* crosscheck START */
	if (seqform->seqstart < seqform->seqmin)
	{
		char		bufs[100],
					bufm[100];

		/* Bug - #z6160 */
		if (compatible_mode == MYS_MODE && seq_type == ATTRIBUTE_IDENTITY_AUTO_INCREMENT)
		{
			seqform->seqstart = seqform->seqmin;
		}
		else
		{
			snprintf(bufs, sizeof(bufs), INT64_FORMAT, seqform->seqstart);
			snprintf(bufm, sizeof(bufm), INT64_FORMAT, seqform->seqmin);
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("START value (%s) cannot be less than MINVALUE (%s)",
							bufs, bufm)));
		}
	}
	if (seqform->seqstart > seqform->seqmax && (compatible_mode == PG_MODE || compatible_mode == MYS_MODE || !for_identity))	/* BUG#Z026 */ /* ReqID:SRS-SQL-SEQ */	/* ReqID:SRS-MISC-MYSQL-FRAME */
	{
		char		bufs[100],
					bufm[100];

		/* Bug - #z6160 */
		if (compatible_mode == MYS_MODE && seq_type == ATTRIBUTE_IDENTITY_AUTO_INCREMENT)
		{
			seqform->seqstart = seqform->seqmax;
		}
		else
		{
			snprintf(bufs, sizeof(bufs), INT64_FORMAT, seqform->seqstart);
			snprintf(bufm, sizeof(bufm), INT64_FORMAT, seqform->seqmax);
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("START value (%s) cannot be greater than MAXVALUE (%s)",
							bufs, bufm)));
		}
	}

	/* Begin - ReqID:SRS-SQL-SEQ */
	if (isInit)
		seqform->flags = 0;

	if (scale_flag)
	{
		seqform->flags |= SCALE_FLAG;
		if (extend_flag)
		{
			/* Change sequence maxvavalue/minvalue */
			seqform->flags |= EXTEND_FLAG;
			if(seqform->seqmax > PG_SCALE_MAX && seqform->seqincrement > 0)
			{
				/* Begin BUG#Z230 */
				if(internal_warning)	/* BUG#Z230 */
					ereport(WARNING,
								(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
								errmsg("Scale extend value is greater than 9999999999999")));
				/* End BUG#Z230 */
				seqform->seqmax = PG_SCALE_MAX;
			}
			else if(seqform->seqmin < PG_SCALE_MIN&& seqform->seqincrement < 0)
			{
				/* Begin BUG#Z230 */
				if(internal_warning)
					ereport(WARNING,
								(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
								errmsg("Scale extend value is less than -9999999999999")));
				/* End BUG#Z230 */
				seqform->seqmin = PG_SCALE_MIN;
			}
		}
		else if (!isInit && !extend_flag)
		{
			seqform->flags &= ~(EXTEND_FLAG);
			if(seqform->seqmax == PG_SCALE_MAX && seqform->seqincrement > 0)
				seqform->seqmax = PG_INT64_MAX;
			else if(seqform->seqmin < PG_SCALE_MIN && seqform->seqincrement < 0)
				seqform->seqmin = PG_SCALE_MIN;
		}
	}
	else if (noscale_flag && isInit)
	{
		seqform->flags &= ~(SCALE_FLAG);
	}
	else if (noscale_flag && !isInit)
	{
		char		maxstr[MAXINT8LEN + 1];
		int 		maxvalue_bits;

		maxvalue_bits = pg_lltoa(old_maxvalue, maxstr);
		if (seq_is_scale(seqform->flags))
			seqform->flags &= ~(SCALE_FLAG);
		if (seqdataform->is_called)
		{
			if (seq_is_extend(seqform->flags))
			{
				if (seqdataform->last_value >= 0 && !seqelm)
					seqdataform->last_value = 199999 * (int64)pow(10, maxvalue_bits) + seqdataform->last_value;
				else if (seqdataform->last_value >= 0 && seqelm && seqelm->last != seqelm->cached)
					seqdataform->last_value = 199999 * (int64)pow(10, maxvalue_bits) + seqelm->last;
				else if (seqdataform->last_value < 0 && !seqelm)
					seqdataform->last_value = -199999 * (int64)pow(10, maxvalue_bits) + seqdataform->last_value;
				else if (seqdataform->last_value < 0 && seqelm && seqelm->last != seqelm->cached)
					seqdataform->last_value = -199999 * (int64)pow(10, maxvalue_bits) + seqelm->last;
				seqdataform->is_called = true;
				seqdataform->log_cnt = 0;
			}
			else
			{
				if (seqdataform->last_value >= 0 && !seqelm)
					seqdataform->last_value = 199999 * (int64)pow(10, maxvalue_bits - 6) + seqdataform->last_value;
				else if (seqdataform->last_value >= 0 && seqelm && seqelm->last != seqelm->cached)
					seqdataform->last_value = 199999 * (int64)pow(10, maxvalue_bits - 6) + seqelm->last;
				else if (seqdataform->last_value < 0 && !seqelm)
					seqdataform->last_value = -199999 * (int64)pow(10, maxvalue_bits - 6) + seqdataform->last_value;
				else if (seqdataform->last_value < 0 && seqelm && seqelm->last != seqelm->cached)
					seqdataform->last_value = -199999 * (int64)pow(10, maxvalue_bits - 6) + seqelm->last;
				seqdataform->is_called = true;
				seqdataform->log_cnt = 0;
			}
		}
	}

	if (session_flag)
	{
		int64	instanc_id;
		int64	sessionid;
		int 		maxvalue_bits;
		int			minvalue_bits;
		char		maxstr[MAXINT8LEN + 1];

		seqform->flags |= SESSION_FLAG;
		if (!isInit && seq_is_scale(seqform->flags))
		{
			maxvalue_bits = pg_lltoa((int64)Abs(seqform->seqmax), maxstr);
			minvalue_bits = pg_lltoa((int64)Abs(seqform->seqmin), maxstr);
			maxvalue_bits = minvalue_bits > maxvalue_bits ? minvalue_bits : maxvalue_bits;
			if (!seqelm->scale_value)
			{
				instanc_id = GetSystemIdentifier();
				instanc_id = instanc_id%100 + 100;
				if (!session_id)
					session_id = get_sessionid();
				sessionid = session_id;
				sessionid = sessionid%1000;
				last_used_seq = seqelm;
				if (seq_scale_fixed)
					scale_value = 199999;
				else
					scale_value = instanc_id * pow(10, 3) + sessionid;
			}

			if (seq_is_extend(seqform->flags))
			{
				if (seqelm->last >= 0)
					seqelm->scale_value = (int64)(scale_value * (int64)pow(10, maxvalue_bits));
				else
					seqelm->scale_value = -(int64)(scale_value * (int64)pow(10, maxvalue_bits));
			}
			else
			{
				if(seqelm->last >= 0)
					seqelm->scale_value = scale_value * (int64)pow(10, maxvalue_bits - 6);
				else
					seqelm->scale_value = -(scale_value * (int64)pow(10, maxvalue_bits - 6));
			}
			seqelm->last += seqelm->scale_value;
		}
	}
	else if(global_flag)
	{
		if (seq_is_session(seqform->flags))
			seqdataform->is_called = false;
		seqform->flags &= ~(SESSION_FLAG);
	}
	/* End - ReqID:SRS-SQL-SEQ */

	/* RESTART [WITH] */
	if (restart_value != NULL)
	{
		if (restart_value->arg != NULL)
			seqdataform->last_value = defGetInt64(restart_value);
		/* Begin - ReqID:SRS-SQL-SEQ */
		else if (!restart_value->arg && compatible_mode == PG_MODE)
			seqdataform->last_value = seqform->seqstart;
		else if (!restart_value->arg && compatible_mode == ORA_MODE && seqform->seqincrement > 0 && !start_value)
			seqdataform->last_value = seqform->seqmin;
		else if (!restart_value->arg && compatible_mode == ORA_MODE && seqform->seqincrement < 0 && !start_value)
			seqdataform->last_value = seqform->seqmax;
		else if (start_value && compatible_mode == ORA_MODE)
			seqdataform->last_value = defGetInt64(start_value);
		/* End - ReqID:SRS-SQL-SEQ */
		/* Begin - ReqID:SRS-MISC-MYSQL-FRAME */
		else if (!restart_value->arg && compatible_mode == MYS_MODE)
			seqdataform->last_value = seqform->seqstart;
		/* End - ReqID:SRS-MISC-MYSQL-FRAME */
		seqdataform->is_called = false;
		seqdataform->log_cnt = 0;
	}
	else if (isInit)
	{
		seqdataform->last_value = seqform->seqstart;
		seqdataform->is_called = false;
	}

	/* crosscheck RESTART (or current value, if changing MIN/MAX) */
	if (seqdataform->last_value < seqform->seqmin)
	{
		char		bufs[100],
					bufm[100];

		snprintf(bufs, sizeof(bufs), INT64_FORMAT, seqdataform->last_value);
		snprintf(bufm, sizeof(bufm), INT64_FORMAT, seqform->seqmin);
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("RESTART value (%s) cannot be less than MINVALUE (%s)",
						bufs, bufm)));
	}
	if (seqdataform->last_value > seqform->seqmax && compatible_mode == PG_MODE)	/* BUG#Z026 */
	{
		char		bufs[100],
					bufm[100];

		snprintf(bufs, sizeof(bufs), INT64_FORMAT, seqdataform->last_value);
		snprintf(bufm, sizeof(bufm), INT64_FORMAT, seqform->seqmax);
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("RESTART value (%s) cannot be greater than MAXVALUE (%s)",
						bufs, bufm)));
	}
	/* Begin - ReqID:SRS-MISC-MYSQL-FRAME */
	else if (seqdataform->last_value > seqform->seqmax && compatible_mode == MYS_MODE)
	{
		char		bufs[100],
					bufm[100];

		snprintf(bufs, sizeof(bufs), INT64_FORMAT, seqdataform->last_value);
		snprintf(bufm, sizeof(bufm), INT64_FORMAT, seqform->seqmax);
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("RESTART value (%s) cannot be greater than MAXVALUE (%s)",
						bufs, bufm)));
	}
	/* End - ReqID:SRS-MISC-MYSQL-FRAME */

	/* CACHE */
	if (cache_value != NULL)
	{
		/* Begin - BUG#Z165 */
		if (compatible_mode == ORA_MODE)
		{
			PG_TRY();
			{
				seqform->seqcache = defGetInt64(cache_value);
				if (seqform->seqcache <= 1)	/* ReqID:SRS-SQL-SEQ */
				{
					char		buf[100];

					snprintf(buf, sizeof(buf), INT64_FORMAT, seqform->seqcache);

					/* For create sequence command */
					if (seq_type == 0)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("the number of values to CACHE must be greater than 1")));

					/* For generated identity */
					if ((seq_type == ATTRIBUTE_IDENTITY_DEFAULT_ON_NULL || seq_type == ATTRIBUTE_ORA_IDENTITY_ALWAYS
						|| seq_type == ATTRIBUTE_ORA_IDENTITY_BY_DEFAULT) && !no_cache_flag) /* ReqID:SRS-SQL-GTT */
					{
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								 errmsg("the number of values to CACHE must be greater than 1")));	/* ReqID:SRS-SQL-SEQ */
					}
				}
				seqdataform->log_cnt = 0;
			}
			PG_CATCH();
			{
				int		errcod;
				char   *intval;

				errcod = geterrcode();
				if (errcod == ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE)
				{
					if(internal_warning)	/* BUG#Z230 */
						ereport(WARNING,
								(errcode(ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE),
								errmsg("Cache value is out of range for data type bigint")));
					intval = strVal(cache_value->arg);
					if(*intval == '-')
						seqform->seqcache = PG_INT64_MIN;
					else
						seqform->seqcache = PG_INT64_MAX;
					if (seqform->seqcache <= 0)
					{
						char		buf[100];

						snprintf(buf, sizeof(buf), INT64_FORMAT, seqform->seqcache);
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
								errmsg("CACHE (%s) must be greater than zero",
										buf)));
					}
					seqdataform->log_cnt = 0;
				}
				else if (errcod == ERRCODE_INVALID_TEXT_REPRESENTATION || errcod == ERRCODE_INVALID_PARAMETER_VALUE)
				{
					PG_RE_THROW();
				}
				FlushErrorState();
			}
			PG_END_TRY();
		}
		else
		{
			seqform->seqcache = defGetInt64(cache_value);
			if (seqform->seqcache <= 0)
			{
				char		buf[100];

				snprintf(buf, sizeof(buf), INT64_FORMAT, seqform->seqcache);
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						errmsg("CACHE (%s) must be greater than zero",
								buf)));
			}
			seqdataform->log_cnt = 0;
		}
		/* End - BUG#Z165 */
	}
	else if (isInit)
	{
		/* Begin - BUG#Z025*/
		/* Set default cache size */
		if ((seq_type == ATTRIBUTE_IDENTITY_DEFAULT_ON_NULL || seq_type == ATTRIBUTE_ORA_IDENTITY_ALWAYS
						|| seq_type == ATTRIBUTE_ORA_IDENTITY_BY_DEFAULT) && !no_cache_flag) /* ReqID:SRS-SQL-GTT */
			seqform->seqcache = 20;
		/* End - BUG#Z025*/
		/* Begin - ReqID:SRS-SQL-SEQ */
		else if (compatible_mode == ORA_MODE && !no_cache_flag)
			seqform->seqcache = 20;
		/* End - ReqID:SRS-SQL-SEQ */
		else
			seqform->seqcache = 1;
	}
}

/*
 * Process an OWNED BY option for CREATE/ALTER SEQUENCE
 *
 * Ownership permissions on the sequence are already checked,
 * but if we are establishing a new owned-by dependency, we must
 * enforce that the referenced table has the same owner and namespace
 * as the sequence.
 */
static void
process_owned_by(Relation seqrel, List *owned_by, bool for_identity)
{
	DependencyType deptype;
	int			nnames;
	Relation	tablerel;
	AttrNumber	attnum = InvalidAttrNumber;
	char		*seqname; /* ReqID:SRS-SQL-ROWID */

	deptype = for_identity ? DEPENDENCY_INTERNAL : DEPENDENCY_AUTO;

	nnames = list_length(owned_by);

	seqname = RelationGetRelationName(seqrel);

	Assert(nnames > 0);
	if (nnames == 1)
	{
		/* Must be OWNED BY NONE */
		if (strcmp(strVal(linitial(owned_by)), "none") != 0)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("invalid OWNED BY option"),
					 errhint("Specify OWNED BY table.column or OWNED BY NONE.")));
		tablerel = NULL;
		attnum = 0;
	}
	else
	{
		List	   *relname;
		char	   *attrname;
		RangeVar   *rel;

		/* Separate relname and attr name */
		relname = list_truncate(list_copy(owned_by), nnames - 1);
		attrname = strVal(llast(owned_by));

		/* Open and lock rel to ensure it won't go away meanwhile */
		rel = makeRangeVarFromNameList(relname);
		/* Begin - ReqID:SRS-MYSQL-CASE */
		if (mysql_objlower_hook && mysql_string_case_hook && mysql_rvar_case_hook)
		{
			if (lower_zero != mysql_objlower_hook() && rel != NULL)
				mysql_rvar_case_hook(rel);
		}
		/* End - ReqID:SRS-MYSQL-CASE */
		tablerel = relation_openrv(rel, AccessShareLock);

		/* Must be a regular or foreign table */
		if (!(tablerel->rd_rel->relkind == RELKIND_RELATION ||
			  tablerel->rd_rel->relkind == RELKIND_FOREIGN_TABLE ||
			  tablerel->rd_rel->relkind == RELKIND_VIEW ||
			  tablerel->rd_rel->relkind == RELKIND_PARTITIONED_TABLE))
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("referenced relation \"%s\" is not a table or foreign table",
							RelationGetRelationName(tablerel))));

		/* We insist on same owner and schema */
		if (seqrel->rd_rel->relowner != tablerel->rd_rel->relowner)
			ereport(ERROR,
					(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
					 errmsg("sequence must have same owner as table it is linked to")));
		if (RelationGetNamespace(seqrel) != RelationGetNamespace(tablerel))
			ereport(ERROR,
					(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
					 errmsg("sequence must be in same schema as table it is linked to")));

		/* Now, fetch the attribute number from the system cache */
		if (compatible_mode == ORA_MODE && strcmp(seqname, attrname) != 0)
		{
			attnum = get_attnum(RelationGetRelid(tablerel), attrname);
			if (attnum == InvalidAttrNumber)
				ereport(ERROR,
						(errcode(ERRCODE_UNDEFINED_COLUMN),
						 errmsg("column \"%s\" of relation \"%s\" does not exist",
								attrname, RelationGetRelationName(tablerel))));
		}
		/* Begin - ReqID:SRS-MISC-MYSQL-FRAME */
		else if (compatible_mode == MYS_MODE) /* BUG#Z5121 */
		{
			attnum = get_attnum(RelationGetRelid(tablerel), attrname);
			if (attnum == InvalidAttrNumber)
				ereport(ERROR,
						(errcode(ERRCODE_UNDEFINED_COLUMN),
						 errmsg("column \"%s\" of relation \"%s\" does not exist",
								attrname, RelationGetRelationName(tablerel))));
		}
		/* End - ReqID:SRS-MISC-MYSQL-FRAME */
		else if (compatible_mode == PG_MODE)
		{
			attnum = get_attnum(RelationGetRelid(tablerel), attrname);
			if (attnum == InvalidAttrNumber)
				ereport(ERROR,
						(errcode(ERRCODE_UNDEFINED_COLUMN),
						 errmsg("column \"%s\" of relation \"%s\" does not exist",
								attrname, RelationGetRelationName(tablerel))));
		}
	}

	/*
	 * Catch user explicitly running OWNED BY on identity sequence.
	 */
	if (deptype == DEPENDENCY_AUTO)
	{
		Oid			tableId;
		int32		colId;

		if (sequenceIsOwned(RelationGetRelid(seqrel), DEPENDENCY_INTERNAL, &tableId, &colId))
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("cannot change ownership of identity sequence"),
					 errdetail("Sequence \"%s\" is linked to table \"%s\".",
							   RelationGetRelationName(seqrel),
							   get_rel_name(tableId))));
	}

	/*
	 * OK, we are ready to update pg_depend.  First remove any existing
	 * dependencies for the sequence, then optionally add a new one.
	 */
	deleteDependencyRecordsForClass(RelationRelationId, RelationGetRelid(seqrel),
									RelationRelationId, deptype);

	if (tablerel)
	{
		ObjectAddress refobject,
					depobject;

		refobject.classId = RelationRelationId;
		refobject.objectId = RelationGetRelid(tablerel);
		refobject.objectSubId = attnum;
		depobject.classId = RelationRelationId;
		depobject.objectId = RelationGetRelid(seqrel);
		depobject.objectSubId = 0;
		recordDependencyOn(&depobject, &refobject, deptype);
	}

	/* Done, but hold lock until commit */
	if (tablerel)
		relation_close(tablerel, NoLock);
}


/*
 * Return sequence parameters in a list of the form created by the parser.
 */
List *
sequence_options(Oid relid)
{
	HeapTuple	pgstuple;
	Form_pg_sequence pgsform;
	List	   *options = NIL;

	pgstuple = SearchSysCache1(SEQRELID, relid);
	if (!HeapTupleIsValid(pgstuple))
		elog(ERROR, "cache lookup failed for sequence %u", relid);
	pgsform = (Form_pg_sequence) GETSTRUCT(pgstuple);

	/* Use makeFloat() for 64-bit integers, like gram.y does. */
	options = lappend(options,
					  makeDefElem("cache", (Node *) makeFloat(psprintf(INT64_FORMAT, pgsform->seqcache)), -1));
	options = lappend(options,
					  makeDefElem("cycle", (Node *) makeInteger(pgsform->seqcycle), -1));
	options = lappend(options,
					  makeDefElem("increment", (Node *) makeFloat(psprintf(INT64_FORMAT, pgsform->seqincrement)), -1));
	options = lappend(options,
					  makeDefElem("maxvalue", (Node *) makeFloat(psprintf(INT64_FORMAT, pgsform->seqmax)), -1));
	options = lappend(options,
					  makeDefElem("minvalue", (Node *) makeFloat(psprintf(INT64_FORMAT, pgsform->seqmin)), -1));
	options = lappend(options,
					  makeDefElem("start", (Node *) makeFloat(psprintf(INT64_FORMAT, pgsform->seqstart)), -1));

	ReleaseSysCache(pgstuple);

	return options;
}

/*
 * Return sequence parameters (formerly for use by information schema)
 */
Datum
pg_sequence_parameters(PG_FUNCTION_ARGS)
{
	Oid			relid = PG_GETARG_OID(0);
	TupleDesc	tupdesc;
	Datum		values[7];
	bool		isnull[7];
	HeapTuple	pgstuple;
	Form_pg_sequence pgsform;

	if (pg_class_aclcheck(relid, GetUserId(), ACL_SELECT | ACL_UPDATE | ACL_USAGE) != ACLCHECK_OK)
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("permission denied for sequence %s",
						get_rel_name(relid))));

	tupdesc = CreateTemplateTupleDesc(7, false);
	TupleDescInitEntry(tupdesc, (AttrNumber) 1, "start_value",
					   INT8OID, -1, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 2, "minimum_value",
					   INT8OID, -1, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 3, "maximum_value",
					   INT8OID, -1, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 4, "increment",
					   INT8OID, -1, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 5, "cycle_option",
					   BOOLOID, -1, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 6, "cache_size",
					   INT8OID, -1, 0);
	TupleDescInitEntry(tupdesc, (AttrNumber) 7, "data_type",
					   OIDOID, -1, 0);

	BlessTupleDesc(tupdesc);

	memset(isnull, 0, sizeof(isnull));

	pgstuple = SearchSysCache1(SEQRELID, relid);
	if (!HeapTupleIsValid(pgstuple))
		elog(ERROR, "cache lookup failed for sequence %u", relid);
	pgsform = (Form_pg_sequence) GETSTRUCT(pgstuple);

	values[0] = Int64GetDatum(pgsform->seqstart);
	values[1] = Int64GetDatum(pgsform->seqmin);
	values[2] = Int64GetDatum(pgsform->seqmax);
	values[3] = Int64GetDatum(pgsform->seqincrement);
	values[4] = BoolGetDatum(pgsform->seqcycle);
	values[5] = Int64GetDatum(pgsform->seqcache);
	values[6] = ObjectIdGetDatum(pgsform->seqtypid);

	ReleaseSysCache(pgstuple);

	return HeapTupleGetDatum(heap_form_tuple(tupdesc, values, isnull));
}

/*
 * Return the last value from the sequence
 *
 * Note: This has a completely different meaning than lastval().
 */
Datum
pg_sequence_last_value(PG_FUNCTION_ARGS)
{
	Oid			relid = PG_GETARG_OID(0);
	SeqTable	elm;
	Relation	seqrel;
	bool		is_called = false;
	int64		result = 0;

	/* open and lock sequence */
	init_sequence(relid, &elm, &seqrel);

	if (pg_class_aclcheck(relid, GetUserId(), ACL_SELECT | ACL_USAGE) != ACLCHECK_OK)
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("permission denied for sequence %s",
						RelationGetRelationName(seqrel))));

	/*
	 * We return NULL for other sessions' temporary sequences.  The
	 * pg_sequences system view already filters those out, but this offers a
	 * defense against ERRORs in case someone invokes this function directly.
	 */
	if (!RELATION_IS_OTHER_TEMP(seqrel))
	{
		Buffer		buf;
		HeapTupleData seqtuple;
		Form_pg_sequence_data seq;

		seq = read_seq_tuple(seqrel, &buf, &seqtuple);

		is_called = seq->is_called;
		result = seq->last_value;

		UnlockReleaseBuffer(buf);
	}
	relation_close(seqrel, NoLock);

	if (is_called)
		PG_RETURN_INT64(result);
	else
		PG_RETURN_NULL();
}


void
seq_redo(XLogReaderState *record)
{
	XLogRecPtr	lsn = record->EndRecPtr;
	uint8		info = XLogRecGetInfo(record) & ~XLR_INFO_MASK;
	Buffer		buffer;
	Page		page;
	Page		localpage;
	char	   *item;
	Size		itemsz;
	xl_seq_rec *xlrec = (xl_seq_rec *) XLogRecGetData(record);
	sequence_magic *sm;

	if (info != XLOG_SEQ_LOG)
		elog(PANIC, "seq_redo: unknown op code %u", info);

	buffer = XLogInitBufferForRedo(record, 0);
	page = (Page) BufferGetPage(buffer);

	/*
	 * We always reinit the page.  However, since this WAL record type is also
	 * used for updating sequences, it's possible that a hot-standby backend
	 * is examining the page concurrently; so we mustn't transiently trash the
	 * buffer.  The solution is to build the correct new page contents in
	 * local workspace and then memcpy into the buffer.  Then only bytes that
	 * are supposed to change will change, even transiently. We must palloc
	 * the local page for alignment reasons.
	 */
	localpage = (Page) palloc(BufferGetPageSize(buffer));

	PageInit(localpage, BufferGetPageSize(buffer), sizeof(sequence_magic));
	sm = (sequence_magic *) PageGetSpecialPointer(localpage);
	sm->magic = SEQ_MAGIC;

	item = (char *) xlrec + sizeof(xl_seq_rec);
	itemsz = XLogRecGetDataLen(record) - sizeof(xl_seq_rec);

	if (PageAddItem(localpage, (Item) item, itemsz,
					FirstOffsetNumber, false, false) == InvalidOffsetNumber)
		elog(PANIC, "seq_redo: failed to add item to page");

	PageSetLSN(localpage, lsn);

	memcpy(page, localpage, BufferGetPageSize(buffer));
	MarkBufferDirty(buffer);
	UnlockReleaseBuffer(buffer);

	pfree(localpage);
}

/*
 * Flush cached sequence information.
 */
void
ResetSequenceCaches(void)
{
	if (seqhashtab)
	{
		hash_destroy(seqhashtab);
		seqhashtab = NULL;
	}

	last_used_seq = NULL;
}

/*
 * Mask a Sequence page before performing consistency checks on it.
 */
void
seq_mask(char *page, BlockNumber blkno)
{
	mask_page_lsn_and_checksum(page);

	mask_unused_space(page);
}

/* Begin - ReqID:SRS-SQL-SEQ */
static bool
seq_is_scale(int16 seq_flag)
{
	if ((seq_flag & SCALE_FLAG) == SCALE_FLAG)
		return true;
	return false;
}

static bool
seq_is_extend(int16 seq_flag)
{
	if ((seq_flag & EXTEND_FLAG) == EXTEND_FLAG)
		return true;
	return false;
}

static bool
seq_is_session(int16 seq_flag)
{
	if ((seq_flag & SESSION_FLAG) == SESSION_FLAG)
		return true;
	return false;
}

int64
get_sessionid(void)
{
	Oid		session_seq_oid;
	SeqTable	elm;
	Relation	seqrel;
	int64		result;

	session_seq_oid = get_relname_relid("userenv_sessionid_sequence", PG_SYS_NAMESPACE);
	init_sequence(session_seq_oid, &elm, &seqrel);
	if (pg_class_aclcheck(elm->relid, GetUserId(),
						  ACL_SELECT | ACL_USAGE) != ACLCHECK_OK)
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("permission denied for sequence %s",
						RelationGetRelationName(seqrel))));
	relation_close(seqrel, NoLock);
	if (!elm->last_valid)
		PG_RETURN_INT64(nextval_internal(session_seq_oid, true));
	result = elm->last;
	PG_RETURN_INT64(result);
}
/* End - ReqID:SRS-SQL-SEQ */
