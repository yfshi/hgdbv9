#include "postgres.h"

#include "replication/output_plugin.h"
#include "replication/logical.h"
#include "access/relation.h"
#include "access/htup_details.h"
#include "miscadmin.h"
#include "utils/builtins.h"
#include "utils/json.h"
#include "utils/lsyscache.h"
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
static void flashback_json_add_string_field(StringInfo out, bool *first,
											const char *key, const char *val);
static void flashback_json_add_int64_field(StringInfo out, bool *first,
										   const char *key, int64 val);
static void flashback_append_json_locator_array(StringInfo out, const char *key,
												HeapTuple tuple, TupleDesc desc,
												AttrNumber *attnums, int nattnums);
static void flashback_append_json_rowid(StringInfo out, const char *key,
										const char *rowid_literal);
static void flashback_append_json_all_cols_array(StringInfo out, const char *key,
												 HeapTuple tuple, TupleDesc desc);
static char *flashback_get_tuple_value_literal(HeapTuple tuple, TupleDesc desc,
											   int attnum);
static char *flashback_get_value_literal(Datum value, Oid typid, bool isnull);
static Datum makeRowIdTupleHeader(Oid tableoid, Datum rowid);

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
	bool		first = true;

	/* Emit explicit begin marker for downstream xid-grouping logic. */
	OutputPluginPrepareWrite(ctx, true);
	resetStringInfo(ctx->out);
	appendStringInfoChar(ctx->out, '{');
	flashback_json_add_string_field(ctx->out, &first, "op", "B");
	flashback_json_add_int64_field(ctx->out, &first, "xid", (int64) txn->xid);
	flashback_json_add_int64_field(ctx->out, &first, "first_lsn",
								   (int64) txn->first_lsn);
	appendStringInfoChar(ctx->out, '}');
	OutputPluginWrite(ctx, true);
}

static void
flashback_commit_cb(LogicalDecodingContext *ctx, ReorderBufferTXN *txn,
					XLogRecPtr commit_lsn)
{
	bool		first = true;

	/* Emit explicit commit marker so consumer flushes only committed xid groups. */
	OutputPluginPrepareWrite(ctx, true);
	resetStringInfo(ctx->out);
	appendStringInfoChar(ctx->out, '{');
	flashback_json_add_string_field(ctx->out, &first, "op", "C");
	flashback_json_add_int64_field(ctx->out, &first, "xid", (int64) txn->xid);
	flashback_json_add_int64_field(ctx->out, &first, "commit_lsn",
								   (int64) commit_lsn);
	flashback_json_add_int64_field(ctx->out, &first, "end_lsn",
								   (int64) txn->end_lsn);
	flashback_json_add_int64_field(ctx->out, &first, "commit_time",
								   (int64) txn->commit_time);
	appendStringInfoChar(ctx->out, '}');
	OutputPluginWrite(ctx, true);
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

static void
flashback_json_add_string_field(StringInfo out, bool *first, const char *key,
								const char *val)
{
	if (!*first)
		appendStringInfoChar(out, ',');
	*first = false;
	escape_json(out, key);
	appendStringInfoChar(out, ':');
	escape_json(out, val ? val : "");
}

static void
flashback_json_add_int64_field(StringInfo out, bool *first, const char *key,
							   int64 val)
{
	if (!*first)
		appendStringInfoChar(out, ',');
	*first = false;
	escape_json(out, key);
	appendStringInfo(out, ":%lld", (long long) val);
}

static void
flashback_append_json_locator_array(StringInfo out, const char *key,
									HeapTuple tuple, TupleDesc desc,
									AttrNumber *attnums, int nattnums)
{
	int			i;

	appendStringInfoChar(out, ',');
	escape_json(out, key);
	appendStringInfoString(out, ":[");
	for (i = 0; i < nattnums; i++)
	{
		const char *colname = NameStr(TupleDescAttr(desc, attnums[i] - 1)->attname);
		char	   *val = flashback_get_tuple_value_literal(tuple, desc, attnums[i]);

		if (i > 0)
			appendStringInfoChar(out, ',');
		appendStringInfoChar(out, '{');
		escape_json(out, "name");
		appendStringInfoChar(out, ':');
		escape_json(out, colname);
		appendStringInfoChar(out, ',');
		escape_json(out, "val");
		appendStringInfoChar(out, ':');
		escape_json(out, val);
		appendStringInfoChar(out, '}');
		pfree(val);
	}
	appendStringInfoChar(out, ']');
}

static void
flashback_append_json_rowid(StringInfo out, const char *key,
							const char *rowid_literal)
{
	appendStringInfoChar(out, ',');
	escape_json(out, key);
	appendStringInfoString(out, ":[");
	appendStringInfoChar(out, '{');
	escape_json(out, "name");
	appendStringInfoChar(out, ':');
	escape_json(out, "rowid");
	appendStringInfoChar(out, ',');
	escape_json(out, "val");
	appendStringInfoChar(out, ':');
	escape_json(out, rowid_literal);
	appendStringInfoChar(out, '}');
	appendStringInfoChar(out, ']');
}

static void
flashback_append_json_all_cols_array(StringInfo out, const char *key,
									 HeapTuple tuple, TupleDesc desc)
{
	Datum	   *values;
	bool	   *nulls;
	int			natts;
	int			i;
	bool		first = true;

	natts = desc->natts;
	values = palloc(sizeof(Datum) * natts);
	nulls = palloc(sizeof(bool) * natts);
	heap_deform_tuple(tuple, desc, values, nulls);

	appendStringInfoChar(out, ',');
	escape_json(out, key);
	appendStringInfoString(out, ":[");
	for (i = 0; i < natts; i++)
	{
		Form_pg_attribute attr = TupleDescAttr(desc, i);
		const char *colname;
		const char *val;

		if (attr->attisdropped)
			continue;

		colname = NameStr(attr->attname);
		if (nulls[i])
			val = "NULL";
		else
			val = flashback_get_value_literal(values[i], attr->atttypid, false);

		if (!first)
			appendStringInfoChar(out, ',');
		first = false;
		appendStringInfoChar(out, '{');
		escape_json(out, "name");
		appendStringInfoChar(out, ':');
		escape_json(out, colname);
		appendStringInfoChar(out, ',');
		escape_json(out, "val");
		appendStringInfoChar(out, ':');
		escape_json(out, val);
		appendStringInfoChar(out, '}');
	}
	appendStringInfoChar(out, ']');

	pfree(values);
	pfree(nulls);
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
	Oid			typoutput;
	bool		typisvarlena;
	char	   *outstr;
	Oid			base_typid;
	TYPCATEGORY category;

	if (isnull)
		return pstrdup("NULL");

	/* Get base type for domain types */
	base_typid = getBaseType(typid);

	getTypeOutputInfo(typid, &typoutput, &typisvarlena);
	outstr = OidOutputFunctionCall(typoutput, value);

	/* Use type category to determine if quotes are needed */
	category = TypeCategory(base_typid);

	/* Numeric category doesn't need quotes (includes int2/4/8, float4/8, numeric, etc.) */
	if (category == TYPCATEGORY_NUMERIC)
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
	int			natts;
	bool		has_rowid;
	char	   *rowid_literal;
	int			pk_natts;
	bool		first = true;

	relname = RelationGetRelationName(relation);
	nspname = get_namespace_name(RelationGetNamespace(relation));
	ownername = GetUserNameFromId(RelationGetForm(relation)->relowner, false);

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

	newtuple = change->data.tp.newtuple;
	oldtuple = change->data.tp.oldtuple;
	desc = RelationGetDescr(relation);
	natts = desc->natts;

	/* use rowid as locator if it exist */
	has_rowid = relation->rd_rel->relhasrowid;
	if (has_rowid)
	{
		Datum	rowid;
		Datum 	rowidTup;
		Oid		rowid_output;
		bool	rowid_isvalena;

		if (oldtuple == NULL && newtuple == NULL)
			elog(ERROR, "flashback_decoder: both oldtuple and newtuple unavailable in %s.%s",
				nspname, relname);

		if (oldtuple)
			rowid = HeapTupleGetRowId(&oldtuple->tuple);
		else
			rowid = HeapTupleGetRowId(&newtuple->tuple);
		rowidTup = makeRowIdTupleHeader(RelationGetRelid(relation), rowid);
		getTypeOutputInfo(ROWIDOID, &rowid_output, &rowid_isvalena);
		rowid_literal = quote_literal_cstr(OidOutputFunctionCall(rowid_output, rowidTup));

		locator_attnums = NULL;
	}
	else
	{
		locator_attnums = palloc(sizeof(AttrNumber) * natts);
		pk_natts = flashback_get_locator_attnums(relation, desc, locator_attnums, natts);

		if (pk_natts <= 0)
		{
			elog(LOG, "flashback_decoder: skip %s.%s without usable locator columns",
				nspname, relname);
			pfree(locator_attnums);
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
	OutputPluginPrepareWrite(ctx, true);
	resetStringInfo(ctx->out);
	/*
	 * "pk_count + locator fields" is a stable wire contract consumed by
	 * hg_flashback_tx_process_raw_messages(). "pk" here means "row locator":
	 * primary key columns if present, otherwise all visible columns.
	 */

	if (change->action == REORDER_BUFFER_CHANGE_INSERT && newtuple != NULL)
	{
		appendStringInfoChar(ctx->out, '{');
		flashback_json_add_string_field(ctx->out, &first, "op", "I");
		flashback_json_add_string_field(ctx->out, &first, "nsp", nspname);
		flashback_json_add_string_field(ctx->out, &first, "rel", relname);
		flashback_json_add_string_field(ctx->out, &first, "owner", ownername);
		flashback_json_add_int64_field(ctx->out, &first, "commit_time",
									   (int64) txn->commit_time);
		if (has_rowid)
			flashback_append_json_rowid(ctx->out, "locator", rowid_literal);
		else
			flashback_append_json_locator_array(ctx->out, "locator",
												&newtuple->tuple, desc,
												locator_attnums, pk_natts);
		appendStringInfoString(ctx->out, ",\"cols\":[]}");
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
		appendStringInfoChar(ctx->out, '{');
		flashback_json_add_string_field(ctx->out, &first, "op", "U");
		flashback_json_add_string_field(ctx->out, &first, "nsp", nspname);
		flashback_json_add_string_field(ctx->out, &first, "rel", relname);
		flashback_json_add_string_field(ctx->out, &first, "owner", ownername);
		flashback_json_add_int64_field(ctx->out, &first, "commit_time",
									   (int64) txn->commit_time);
		if (has_rowid)
			flashback_append_json_rowid(ctx->out, "locator", rowid_literal);
		else
			flashback_append_json_locator_array(ctx->out, "locator",
												&newtuple->tuple, desc,
												locator_attnums, pk_natts);
		flashback_append_json_all_cols_array(ctx->out, "cols",
											 &oldtuple->tuple, desc);
		appendStringInfoChar(ctx->out, '}');
	}
	else if (change->action == REORDER_BUFFER_CHANGE_DELETE &&
			 oldtuple != NULL)
	{
		/*
		 * DELETE: emit primary key value and old column values for re-insert undo.
		 */
		appendStringInfoChar(ctx->out, '{');
		flashback_json_add_string_field(ctx->out, &first, "op", "D");
		flashback_json_add_string_field(ctx->out, &first, "nsp", nspname);
		flashback_json_add_string_field(ctx->out, &first, "rel", relname);
		flashback_json_add_string_field(ctx->out, &first, "owner", ownername);
		flashback_json_add_int64_field(ctx->out, &first, "commit_time",
									   (int64) txn->commit_time);
		if (has_rowid)
			flashback_append_json_rowid(ctx->out, "locator", rowid_literal);
		else
			flashback_append_json_locator_array(ctx->out, "locator",
												&oldtuple->tuple, desc,
												locator_attnums, pk_natts);
		flashback_append_json_all_cols_array(ctx->out, "cols",
											 &oldtuple->tuple, desc);
		appendStringInfoChar(ctx->out, '}');
	}
	else
	{
		appendStringInfoChar(ctx->out, '{');
		flashback_json_add_string_field(ctx->out, &first, "op", "X");
		flashback_json_add_string_field(ctx->out, &first, "nsp", nspname);
		flashback_json_add_string_field(ctx->out, &first, "rel", relname);
		appendStringInfoChar(ctx->out, '}');
	}

	OutputPluginWrite(ctx, true);
	if (locator_attnums)
		pfree(locator_attnums);
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
