#include "postgres.h"

#include "replication/output_plugin.h"
#include "replication/logical.h"
#include "access/relation.h"
#include "access/htup_details.h"
#include "miscadmin.h"
#include "utils/builtins.h"
#include "utils/datum.h"
#include "utils/guc.h"
#include "utils/hsearch.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/rel.h"
#include "utils/relcache.h"
#include "catalog/pg_type.h"
#include "catalog/pg_index.h"
#include "parser/parse_coerce.h"
#include "utils/typcache.h"
#include "funcapi.h"

PG_MODULE_MAGIC;

PGDLLEXPORT void _PG_output_plugin_init(OutputPluginCallbacks *cb);

static void flashback_begin_cb(LogicalDecodingContext *ctx,
							   ReorderBufferTXN *txn);
static void flashback_commit_cb(LogicalDecodingContext *ctx,
								ReorderBufferTXN *txn, XLogRecPtr commit_lsn);
static void flashback_startup_cb(LogicalDecodingContext *ctx,
								 OutputPluginOptions *opt, bool is_init);
static void flashback_change_cb(LogicalDecodingContext *ctx,
								ReorderBufferTXN *txn, Relation relation,
								ReorderBufferChange *change);
static int	flashback_get_locator_attnums(Relation relation, TupleDesc desc,
									 AttrNumber *attnums, int max_attnums);
static void flashback_wire_begin_record(StringInfo out, char op,
										int64 commit_time, const char *nspname,
										const char *relname, const char *ownername);
static void flashback_wire_add_string(StringInfo out, const char *val);
static void flashback_wire_add_int64(StringInfo out, int64 val);
static void flashback_append_wire_locator_array(StringInfo out, HeapTuple tuple,
												TupleDesc desc,
												AttrNumber *attnums,
												int nattnums);
static void flashback_append_wire_rowid(StringInfo out,
										const char *rowid_literal);
static int flashback_append_wire_cols(StringInfo out, HeapTuple oldtuple,
									  HeapTuple newtuple, TupleDesc desc,
									  bool only_changed);
static char *flashback_get_tuple_value_literal(HeapTuple tuple, TupleDesc desc,
											   int attnum);
static char *flashback_get_value_literal(Datum value, Oid typid, bool isnull);
static Datum makeRowIdTupleHeader(Oid tableoid, Datum rowid);
static const char *flashback_get_include_tables(void);
static bool flashback_table_name_matches_filter(const char *filter,
												const char *nspname,
												const char *relname);
static void flashback_mark_txn(TransactionId xid);
static bool flashback_txn_marked(TransactionId xid, bool remove);

typedef struct FlashbackTypeCacheEntry
{
	Oid			typid;
	Oid			typoutput;
	bool		typisvarlena;
	bool		quote_literal;
} FlashbackTypeCacheEntry;

typedef struct FlashbackRelCacheEntry
{
	Oid			relid;
	TupleDesc	desc;
	Oid			namespace_id;
	Oid			owner_id;
	Oid			pkindex;
	bool		has_rowid;
	int			natts;
	int			locator_natts;
	AttrNumber *locator_attnums;
	char	   *nspname;
	char	   *relname;
	char	   *ownername;
	char	   *filter_config;
	bool		included;
} FlashbackRelCacheEntry;

typedef struct FlashbackTxnFilterEntry
{
	TransactionId xid;
} FlashbackTxnFilterEntry;

static HTAB *flashback_type_cache = NULL;
static HTAB *flashback_rel_cache = NULL;
static HTAB *flashback_txn_filter_hash = NULL;
static FlashbackTypeCacheEntry *flashback_get_type_cache_entry(Oid typid);
static FlashbackRelCacheEntry *flashback_get_rel_cache_entry(Relation relation);

void
_PG_output_plugin_init(OutputPluginCallbacks *cb)
{
	/*
	 * Register logical decoding callbacks.
	 * We implement begin/commit for visibility and change for payload output.
	 */
	cb->begin_cb = flashback_begin_cb;
	cb->commit_cb = flashback_commit_cb;
	cb->startup_cb = flashback_startup_cb;
	cb->change_cb = flashback_change_cb;
}

/* Use textual output so pg_logical_slot_get_changes can consume it. */
static void
flashback_startup_cb(LogicalDecodingContext *ctx, OutputPluginOptions *opt,
					 bool is_init)
{
	/*
	 * Use textual output so pg_logical_slot_get_changes can consume it.
	 * This keeps decoding simple and avoids binary output handling.
	 */
	opt->output_type = OUTPUT_PLUGIN_TEXTUAL_OUTPUT;
}

static void
flashback_begin_cb(LogicalDecodingContext *ctx, ReorderBufferTXN *txn)
{
	/* Keep begin callback silent; commit is emitted only for captured xids. */
}

static void
flashback_commit_cb(LogicalDecodingContext *ctx, ReorderBufferTXN *txn,
					XLogRecPtr commit_lsn)
{
	if (!flashback_txn_marked(txn->xid, true))
		return;

	/* Emit commit marker so consumer flushes only xid groups with payload. */
	OutputPluginPrepareWrite(ctx, true);
	resetStringInfo(ctx->out);
	flashback_wire_begin_record(ctx->out, 'C', (int64) txn->commit_time,
								"", "", "");
	flashback_wire_add_int64(ctx->out, 0);
	flashback_wire_add_int64(ctx->out, 0);
	OutputPluginWrite(ctx, true);
}

static const char *
flashback_get_include_tables(void)
{
	const char *include_tables;

	include_tables = GetConfigOption("hg_flashback_tx.include_tables", true, false);
	if (include_tables == NULL)
		return "";
	return include_tables;
}

static bool
flashback_table_name_matches_filter(const char *filter, const char *nspname,
									const char *relname)
{
	char	   *raw;
	char	   *token;
	char		qualified[NAMEDATALEN * 2 + 2];

	if (filter == NULL || filter[0] == '\0')
		return true;
	if (nspname == NULL || relname == NULL)
		return false;

	snprintf(qualified, sizeof(qualified), "%s.%s", nspname, relname);
	raw = pstrdup(filter);

	token = strtok(raw, ", \t\r\n");
	while (token != NULL)
	{
		if (strcmp(token, qualified) == 0 ||
			(strchr(token, '.') == NULL && strcmp(token, relname) == 0))
		{
			pfree(raw);
			return true;
		}
		token = strtok(NULL, ", \t\r\n");
	}

	pfree(raw);
	return false;
}

static void
flashback_mark_txn(TransactionId xid)
{
	FlashbackTxnFilterEntry *entry;
	bool		found;

	if (!TransactionIdIsValid(xid))
		return;

	if (flashback_txn_filter_hash == NULL)
	{
		HASHCTL		ctl;

		memset(&ctl, 0, sizeof(ctl));
		ctl.keysize = sizeof(TransactionId);
		ctl.entrysize = sizeof(FlashbackTxnFilterEntry);
		ctl.hcxt = TopMemoryContext;
		flashback_txn_filter_hash = hash_create("flashback_decoder xid filter",
												128, &ctl,
												HASH_ELEM | HASH_BLOBS | HASH_CONTEXT);
	}

	entry = hash_search(flashback_txn_filter_hash, &xid, HASH_ENTER, &found);
	if (!found)
		entry->xid = xid;
}

static bool
flashback_txn_marked(TransactionId xid, bool remove)
{
	bool		found;

	if (!TransactionIdIsValid(xid) || flashback_txn_filter_hash == NULL)
		return false;

	(void) hash_search(flashback_txn_filter_hash, &xid,
					   remove ? HASH_REMOVE : HASH_FIND, &found);
	return found;
}

static int
flashback_get_locator_attnums(Relation relation, TupleDesc desc,
							  AttrNumber *attnums, int max_attnums)
{
	Oid			pkindex;
	Relation	idxrel;
	Form_pg_index idx;
	int			i;
	int			nkeys;
	int			natts;

	pkindex = RelationGetPrimaryKeyIndex(relation);
	if (OidIsValid(pkindex))
	{
		idxrel = relation_open(pkindex, AccessShareLock);
		if (idxrel->rd_index != NULL)
		{
			idx = idxrel->rd_index;
			nkeys = idx->indnkeyatts;
			if (nkeys > 0 && nkeys <= max_attnums)
			{
				for (i = 0; i < nkeys; i++)
				{
					AttrNumber	attnum = idx->indkey.values[i];

					if (attnum <= 0)
						break;
					attnums[i] = attnum;
				}
				if (i == nkeys)
				{
					relation_close(idxrel, AccessShareLock);
					return nkeys;
				}
			}
		}
		relation_close(idxrel, AccessShareLock);
	}

	/* No primary key: fall back to all visible columns as row locator. */
	natts = desc->natts;
	if (natts <= 0 || natts > max_attnums)
		return 0;
	nkeys = 0;
	for (i = 0; i < natts; i++)
	{
		Form_pg_attribute attr = TupleDescAttr(desc, i);

		if (attr->attisdropped)
			continue;
		attnums[nkeys++] = (AttrNumber) (i + 1);
	}
	return nkeys;
}

static FlashbackTypeCacheEntry *
flashback_get_type_cache_entry(Oid typid)
{
	FlashbackTypeCacheEntry *entry;
	bool		found;

	if (flashback_type_cache == NULL)
	{
		HASHCTL		ctl;

		memset(&ctl, 0, sizeof(ctl));
		ctl.keysize = sizeof(Oid);
		ctl.entrysize = sizeof(FlashbackTypeCacheEntry);
		ctl.hcxt = TopMemoryContext;
		flashback_type_cache = hash_create("flashback_decoder type cache",
										   128, &ctl,
										   HASH_ELEM | HASH_BLOBS | HASH_CONTEXT);
	}

	entry = hash_search(flashback_type_cache, &typid, HASH_ENTER, &found);
	if (!found)
	{
		Oid			base_typid;
		TYPCATEGORY category;

		entry->typid = typid;
		base_typid = getBaseType(typid);
		getTypeOutputInfo(typid, &entry->typoutput, &entry->typisvarlena);
		category = TypeCategory(base_typid);
		entry->quote_literal = (category != TYPCATEGORY_NUMERIC);
	}

	return entry;
}

static FlashbackRelCacheEntry *
flashback_get_rel_cache_entry(Relation relation)
{
	FlashbackRelCacheEntry *entry;
	TupleDesc	desc = RelationGetDescr(relation);
	Oid			relid = RelationGetRelid(relation);
	Oid			namespace_id = RelationGetNamespace(relation);
	Oid			owner_id = RelationGetForm(relation)->relowner;
	Oid			pkindex = RelationGetPrimaryKeyIndex(relation);
	bool		has_rowid = relation->rd_rel->relhasrowid;
	const char *filter_config = flashback_get_include_tables();
	bool		found;
	bool		stale;
	MemoryContext oldcxt;

	if (flashback_rel_cache == NULL)
	{
		HASHCTL		ctl;

		memset(&ctl, 0, sizeof(ctl));
		ctl.keysize = sizeof(Oid);
		ctl.entrysize = sizeof(FlashbackRelCacheEntry);
		ctl.hcxt = TopMemoryContext;
		flashback_rel_cache = hash_create("flashback_decoder relation cache",
										  128, &ctl,
										  HASH_ELEM | HASH_BLOBS | HASH_CONTEXT);
	}

	entry = hash_search(flashback_rel_cache, &relid, HASH_ENTER, &found);
	stale = !found ||
		entry->desc != desc ||
		entry->namespace_id != namespace_id ||
		entry->owner_id != owner_id ||
		entry->pkindex != pkindex ||
		entry->has_rowid != has_rowid ||
		entry->natts != desc->natts ||
		entry->filter_config == NULL ||
		strcmp(entry->filter_config, filter_config) != 0;

	if (!stale)
		return entry;

	if (found)
	{
		if (entry->locator_attnums)
			pfree(entry->locator_attnums);
		if (entry->nspname)
			pfree(entry->nspname);
		if (entry->relname)
			pfree(entry->relname);
		if (entry->ownername)
			pfree(entry->ownername);
		if (entry->filter_config)
			pfree(entry->filter_config);
	}

	oldcxt = MemoryContextSwitchTo(TopMemoryContext);
	memset(entry, 0, sizeof(FlashbackRelCacheEntry));
	entry->relid = relid;
	entry->desc = desc;
	entry->namespace_id = namespace_id;
	entry->owner_id = owner_id;
	entry->pkindex = pkindex;
	entry->has_rowid = has_rowid;
	entry->natts = desc->natts;
	entry->nspname = pstrdup(get_namespace_name(namespace_id));
	entry->relname = pstrdup(RelationGetRelationName(relation));
	entry->ownername = pstrdup(GetUserNameFromId(owner_id, false));
	entry->filter_config = pstrdup(filter_config);
	entry->included = flashback_table_name_matches_filter(filter_config,
														  entry->nspname,
														  entry->relname);

	if (!has_rowid)
	{
		entry->locator_attnums = palloc(sizeof(AttrNumber) * desc->natts);
		entry->locator_natts = flashback_get_locator_attnums(relation, desc,
															 entry->locator_attnums,
															 desc->natts);
	}
	MemoryContextSwitchTo(oldcxt);

	return entry;
}

static void
flashback_wire_add_string(StringInfo out, const char *val)
{
	size_t		len;

	if (val == NULL)
		val = "";
	len = strlen(val);
	appendStringInfo(out, "%lu:", (unsigned long) len);
	appendBinaryStringInfo(out, val, len);
}

static void
flashback_wire_add_int64(StringInfo out, int64 val)
{
	char		buf[64];

	snprintf(buf, sizeof(buf), "%lld", (long long) val);
	flashback_wire_add_string(out, buf);
}

static void
flashback_wire_begin_record(StringInfo out, char op, int64 commit_time,
							const char *nspname, const char *relname,
							const char *ownername)
{
	char		opbuf[2];

	opbuf[0] = op;
	opbuf[1] = '\0';
	appendStringInfoString(out, "FB1");
	flashback_wire_add_string(out, opbuf);
	flashback_wire_add_int64(out, commit_time);
	flashback_wire_add_string(out, nspname);
	flashback_wire_add_string(out, relname);
	flashback_wire_add_string(out, ownername);
}

static void
flashback_append_wire_locator_array(StringInfo out, HeapTuple tuple,
									TupleDesc desc, AttrNumber *attnums,
									int nattnums)
{
	int			i;

	flashback_wire_add_int64(out, nattnums);
	for (i = 0; i < nattnums; i++)
	{
		const char *colname = NameStr(TupleDescAttr(desc, attnums[i] - 1)->attname);
		char	   *val = flashback_get_tuple_value_literal(tuple, desc, attnums[i]);

		flashback_wire_add_string(out, colname);
		flashback_wire_add_string(out, val);
		pfree(val);
	}
}

static void
flashback_append_wire_rowid(StringInfo out, const char *rowid_literal)
{
	flashback_wire_add_int64(out, 1);
	flashback_wire_add_string(out, "rowid");
	flashback_wire_add_string(out, rowid_literal);
}

static bool
flashback_attr_changed(Form_pg_attribute attr, Datum oldval, bool oldnull,
					   Datum newval, bool newnull)
{
	if (oldnull != newnull)
		return true;
	if (oldnull)
		return false;

	/*
	 * This is an image comparison. For toasted values it may report "changed"
	 * more often than logically necessary, which preserves correctness and only
	 * reduces the size-saving opportunity for those values.
	 */
	return !datumIsEqual(oldval, newval, attr->attbyval, attr->attlen);
}

static int
flashback_build_wire_cols(StringInfo cols, HeapTuple oldtuple,
						  HeapTuple newtuple, TupleDesc desc,
						  bool only_changed)
{
	Datum	   *values;
	Datum	   *newvalues = NULL;
	bool	   *nulls;
	bool	   *newnulls = NULL;
	int			natts;
	int			i;
	int			count = 0;

	natts = desc->natts;
	values = palloc(sizeof(Datum) * natts);
	nulls = palloc(sizeof(bool) * natts);
	heap_deform_tuple(oldtuple, desc, values, nulls);
	if (only_changed && newtuple != NULL)
	{
		newvalues = palloc(sizeof(Datum) * natts);
		newnulls = palloc(sizeof(bool) * natts);
		heap_deform_tuple(newtuple, desc, newvalues, newnulls);
	}

	for (i = 0; i < natts; i++)
	{
		Form_pg_attribute attr = TupleDescAttr(desc, i);
		const char *colname;
		char	   *val;

		if (attr->attisdropped)
			continue;
		if (only_changed && newtuple != NULL &&
			!flashback_attr_changed(attr, values[i], nulls[i],
									newvalues[i], newnulls[i]))
			continue;

		colname = NameStr(attr->attname);
		val = flashback_get_value_literal(values[i], attr->atttypid, nulls[i]);

		flashback_wire_add_string(cols, colname);
		flashback_wire_add_string(cols, val);
		pfree(val);
		count++;
	}

	pfree(values);
	pfree(nulls);
	if (newvalues)
		pfree(newvalues);
	if (newnulls)
		pfree(newnulls);
	return count;
}

static int
flashback_append_wire_cols(StringInfo out, HeapTuple oldtuple,
						   HeapTuple newtuple, TupleDesc desc,
						   bool only_changed)
{
	StringInfoData cols;
	int			count;

	initStringInfo(&cols);
	count = flashback_build_wire_cols(&cols, oldtuple, newtuple, desc,
									  only_changed);
	if (only_changed && count == 0)
	{
		resetStringInfo(&cols);
		count = flashback_build_wire_cols(&cols, oldtuple, NULL, desc, false);
	}

	flashback_wire_add_int64(out, count);
	appendBinaryStringInfo(out, cols.data, cols.len);
	pfree(cols.data);
	return count;
}

static char *
flashback_get_tuple_value_literal(HeapTuple tuple, TupleDesc desc, int attnum)
{
	Datum		value;
	bool		isnull;
	Oid			typid;

	value = heap_getattr(tuple, attnum, desc, &isnull);
	if (isnull)
		return pstrdup("NULL");

	typid = TupleDescAttr(desc, attnum - 1)->atttypid;
	return flashback_get_value_literal(value, typid, false);
}

/*
 * Get value literal without quotes for numeric types.
 * Uses type category to determine if quotes are needed.
 * Handles domain types and user-defined types based on numeric types.
 */
static char *
flashback_get_value_literal(Datum value, Oid typid, bool isnull)
{
	char	   *outstr;
	FlashbackTypeCacheEntry *type_entry;

	if (isnull)
		return pstrdup("NULL");

	type_entry = flashback_get_type_cache_entry(typid);
	outstr = OidOutputFunctionCall(type_entry->typoutput, value);

	/* Numeric category doesn't need quotes (includes int2/4/8, float4/8, numeric, etc.) */
	if (!type_entry->quote_literal)
		return outstr;

	/* Other types need quotes; free temporary text representation after quoting. */
	{
		char	   *quoted = quote_literal_cstr(outstr);

		pfree(outstr);
		return quoted;
	}
}

static void
flashback_change_cb(LogicalDecodingContext *ctx, ReorderBufferTXN *txn,
					Relation relation, ReorderBufferChange *change)
{
	const char *relname;
	const char *nspname;
	const char *ownername;
	AttrNumber  *locator_attnums;
	ReorderBufferTupleBuf *newtuple;
	ReorderBufferTupleBuf *oldtuple;
	TupleDesc	desc;
	bool		has_rowid;
	char	   *rowid_literal = NULL;
	int			pk_natts = 0;
	FlashbackRelCacheEntry *relcache;

	relcache = flashback_get_rel_cache_entry(relation);
	relname = relcache->relname;
	nspname = relcache->nspname;
	ownername = relcache->ownername;

	/*
	 * Avoid decoding changes generated by internal schemas.
	 * This prevents recursive feedback into the slot.
	 */
	if (nspname != NULL &&
		(strcmp(nspname, "hg_flashback_tx") == 0 ||
		 strcmp(nspname, "hg_security") == 0 ||
		 strcmp(nspname, "pg_catalog") == 0 ||
		 strcmp(nspname, "information_schema") == 0))
		return;

	if (!relcache->included)
		return;

	newtuple = change->data.tp.newtuple;
	oldtuple = change->data.tp.oldtuple;
	desc = RelationGetDescr(relation);

	/* use rowid as locator if it exist */
	has_rowid = relcache->has_rowid;
	if (has_rowid)
	{
		Datum	rowid;
		Datum 	rowidTup;
		FlashbackTypeCacheEntry *rowid_type;
		char   *rowid_output_str;

		if (oldtuple == NULL && newtuple == NULL)
			elog(ERROR, "flashback_decoder: both oldtuple and newtuple unavailable in %s.%s",
				nspname, relname);

		if (oldtuple)
			rowid = HeapTupleGetRowId(&oldtuple->tuple);
		else
			rowid = HeapTupleGetRowId(&newtuple->tuple);
		rowidTup = makeRowIdTupleHeader(RelationGetRelid(relation), rowid);
		rowid_type = flashback_get_type_cache_entry(ROWIDOID);
		rowid_output_str = OidOutputFunctionCall(rowid_type->typoutput, rowidTup);
		rowid_literal = quote_literal_cstr(rowid_output_str);
		pfree(rowid_output_str);
		pfree(DatumGetPointer(rowidTup));

		locator_attnums = NULL;
	}
	else
	{
		locator_attnums = relcache->locator_attnums;
		pk_natts = relcache->locator_natts;

		if (pk_natts <= 0)
		{
			elog(LOG, "flashback_decoder: skip %s.%s without usable locator columns",
				nspname, relname);
			return;
		}
	}

	/*
	 * Build one textual record per change.
	 * Output format:
	 * operation nsp rel owner pk_count [pk_col pk_val ...] commit_time [colcount col val ...]
	 * commit_time is in epoch milliseconds, 0 if not available.
	 * Downstream consumer (hg_flashback_tx worker) parses this to construct
	 * undo SQL without re-running decoder logic.
	 *
	 * PrepareWrite is intentionally placed after early-return validations to keep
	 * each prepare/write pair balanced.
	 */
	flashback_mark_txn(txn->xid);
	OutputPluginPrepareWrite(ctx, true);
	resetStringInfo(ctx->out);
	/*
	 * "pk_count + locator fields" is a stable wire contract consumed by
	 * hg_flashback_tx_process_raw_messages(). "pk" here means "row locator":
	 * primary key columns if present, otherwise all visible columns.
	 */

	if (change->action == REORDER_BUFFER_CHANGE_INSERT && newtuple != NULL)
	{
		flashback_wire_begin_record(ctx->out, 'I', (int64) txn->commit_time,
									nspname, relname, ownername);
		if (has_rowid)
			flashback_append_wire_rowid(ctx->out, rowid_literal);
		else
			flashback_append_wire_locator_array(ctx->out, &newtuple->tuple, desc,
												locator_attnums, pk_natts);
		flashback_wire_add_int64(ctx->out, 0);
	}
	else if (change->action == REORDER_BUFFER_CHANGE_UPDATE &&
			 newtuple != NULL)
	{
		/*
		 * UPDATE: emit primary key value and old column values.
		 * We serialize the old tuple so undo can restore prior values.
		 * If oldtuple is not available (e.g. only replica identity logged),
		 * fall back to newtuple to keep output format stable.
		 */
		if (oldtuple == NULL)
		{
			elog(LOG, "flashback_decoder: %s.%s UPDATE - oldtuple unavailable, using newtuple values (set REPLICA IDENTITY FULL for accurate undo)",
					 nspname, relname);
			oldtuple = newtuple;
		}
		flashback_wire_begin_record(ctx->out, 'U', (int64) txn->commit_time,
									nspname, relname, ownername);
		if (has_rowid)
			flashback_append_wire_rowid(ctx->out, rowid_literal);
		else
			flashback_append_wire_locator_array(ctx->out, &newtuple->tuple, desc,
												locator_attnums, pk_natts);
		flashback_append_wire_cols(ctx->out, &oldtuple->tuple, &newtuple->tuple,
								   desc, true);
	}
	else if (change->action == REORDER_BUFFER_CHANGE_DELETE &&
			 oldtuple != NULL)
	{
		/*
		 * DELETE: emit primary key value and old column values for re-insert undo.
		 */
		flashback_wire_begin_record(ctx->out, 'D', (int64) txn->commit_time,
									nspname, relname, ownername);
		if (has_rowid)
			flashback_append_wire_rowid(ctx->out, rowid_literal);
		else
			flashback_append_wire_locator_array(ctx->out, &oldtuple->tuple, desc,
												locator_attnums, pk_natts);
		flashback_append_wire_cols(ctx->out, &oldtuple->tuple, NULL, desc, false);
	}
	else
	{
		flashback_wire_begin_record(ctx->out, 'X', (int64) txn->commit_time,
									nspname, relname, ownername);
		flashback_wire_add_int64(ctx->out, 0);
		flashback_wire_add_int64(ctx->out, 0);
	}

	OutputPluginWrite(ctx, true);
	if (rowid_literal)
		pfree(rowid_literal);
}

/* copied from heap_getsysattr */
static Datum
makeRowIdTupleHeader(Oid tableoid, Datum rowid)
{
	HeapTupleHeader ret;
	TupleDesc	rd_tupdesc;
	HeapTuple	rd_tuple;
	Datum	   *values;
	bool	   *nulls;

	rd_tupdesc = lookup_rowtype_tupdesc(ROWIDOID, -1);
	values = (Datum *) palloc(rd_tupdesc->natts * sizeof(Datum));
	nulls = (bool *) palloc(rd_tupdesc->natts * sizeof(bool));

	values[0] = tableoid;
	values[1] = rowid;
	nulls[0] = false;
	nulls[1] = false;

	rd_tuple = heap_form_tuple(rd_tupdesc, values, nulls);

	ret = (HeapTupleHeader) palloc(rd_tuple->t_len);
	memcpy(ret, rd_tuple->t_data, rd_tuple->t_len);

	heap_freetuple(rd_tuple);
	pfree(values);
	pfree(nulls);
	ReleaseTupleDesc(rd_tupdesc);

	return HeapTupleHeaderGetDatum(ret);
}
