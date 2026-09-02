/*------------------------------------------------------------------------------------
 *
 * File: ora_partition.c
 *
 * Abstract:
 *		Support routines for Oracle-compatible partitioned tables.
 *
 * ReqID:SRS-SQL-PARTITION-TABLE
 *
 * Authored by zhenmingyang@highgo.com,20251117.
 *
 * Copyright:
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *		src/backend/partitioning/ora_partition.c
 *
 *------------------------------------------------------------------------------------
 */



#include "postgres.h"

#include "access/genam.h"
#include "access/hash.h"
#include "access/htup_details.h"
#include "access/nbtree.h"
#include "access/relation.h"
#include "access/table.h"
#include "access/xact.h"
#include "catalog/dependency.h"
#include "catalog/indexing.h"
#include "catalog/namespace.h"
#include "catalog/objectaddress.h"
#include "catalog/partition.h"
#include "catalog/pg_collation.h"
#include "catalog/pg_inherits.h"
#include "catalog/pg_opclass.h"
#include "catalog/pg_ora_part_tables.h"
#include "catalog/pg_ora_tab_partitions.h"
#include "catalog/pg_partitioned_table.h"
#include "commands/tablespace.h"
#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"
#include "nodes/nodes.h"
#include "optimizer/optimizer.h"
#include "parser/parse_coerce.h"
#include "parser/parse_collate.h"
#include "parser/parse_expr.h"
#include "parser/parse_relation.h"
#include "parser/parse_utilcmd.h"
#include "partitioning/ora_partition.h"
#include "partitioning/partbounds.h"
#include "partitioning/partdesc.h"
#include "storage/lmgr.h"
#include "utils/builtins.h"
#include "utils/catcache.h"
#include "utils/fmgroids.h"
#include "utils/guc.h"
#include "utils/inval.h"
#include "utils/lsyscache.h"
#include "utils/partcache.h"
#include "utils/syscache.h"

static HTAB *InitPartNameDuplicateCheckHashTable(void);
static HTAB *InitPartRelnameDuplicateCheckHashTable(void);
static char *make_part_relname_unique(char *part_relname, Oid namespaceid, HTAB *partrelhash);
static List *transform_partition_extended_names(Relation rel, List *pnames, bool check_duplicate);
static List *transform_subpartition_extended_names(Relation rel, List *subpnames, List **parentlist, bool check_duplicate);


/* ----------------------------------------------------------------
 *					catalog interface
 * ----------------------------------------------------------------
 */
/*
 * StoreOraclePartitionedTable
 *		Store Oracle object-level partition information
 */
void
StoreOraclePartitionedTable(Oid relationId,
									OraPartitionSpec *desc,
									char subpartstrat,
									int16 subpartnatts,
									AttrNumber *subpartattrs,
									List *subpartexprs,
									Oid *subpartopclass,
									Oid *subpartcollation)
{
	Datum		values[Natts_pg_ora_part_tables];
	bool		nulls[Natts_pg_ora_part_tables];
	bool		iscomposite = false;
	Datum		subpart_templdate_datum;
	Relation	pg_ora_part_tables;
	HeapTuple	tuple;

	/*
	 * sanity checks
	 */
	AssertArg(OidIsValid(relationId));

	/* Convert the subpartition template (if any) to a text datum */
	if (desc->sub_part_template)
	{
		char	   *templateString;

		templateString = nodeToString(desc->sub_part_template);
		subpart_templdate_datum = CStringGetTextDatum(templateString);
		pfree(templateString);
	}
	else
		subpart_templdate_datum = (Datum) 0;

	pg_ora_part_tables = table_open(OraPartitionedRelationId, RowExclusiveLock);

	if (desc->sub_partiton_spec)
		iscomposite = true;

	MemSet(nulls, false, sizeof(nulls));

	if (iscomposite)
	{
		int2vector	*subpartattrs_vec;
		oidvector	*subpartopclass_vec;
		oidvector	*subpartcollation_vec;
		Datum		subpartexprDatum;

		/* Copy the subpartition attribute numbers, opclass OIDs into arrays */
		subpartattrs_vec = buildint2vector(subpartattrs, subpartnatts);
		subpartopclass_vec = buildoidvector(subpartopclass, subpartnatts);
		subpartcollation_vec = buildoidvector(subpartcollation, subpartnatts);

		/* Convert the expressions (if any) to a text datum */
		if (subpartexprs)
		{
			char	   *exprString;

			exprString = nodeToString(subpartexprs);
			subpartexprDatum = CStringGetTextDatum(exprString);
			pfree(exprString);
		}
		else
			subpartexprDatum = (Datum) 0;

		if (!subpartexprDatum)
			nulls[Anum_pg_ora_part_tables_subpartexprs - 1] = true;

		values[Anum_pg_ora_part_tables_subpartstrat - 1] = CharGetDatum(subpartstrat);
		values[Anum_pg_ora_part_tables_subpartnatts - 1] = Int16GetDatum(subpartnatts);
		values[Anum_pg_ora_part_tables_subpartattrs - 1] = PointerGetDatum(subpartattrs_vec);
		values[Anum_pg_ora_part_tables_subpartclass - 1] = PointerGetDatum(subpartopclass_vec);
		values[Anum_pg_ora_part_tables_subpartcollation - 1] = PointerGetDatum(subpartcollation_vec);
		values[Anum_pg_ora_part_tables_subpartexprs - 1] = subpartexprDatum;
	}
	else
	{
		values[Anum_pg_ora_part_tables_subpartstrat - 1] = CharGetDatum(subpartstrat);
		values[Anum_pg_ora_part_tables_subpartnatts - 1] = Int16GetDatum(subpartnatts);
		nulls[Anum_pg_ora_part_tables_subpartattrs - 1] = true;
		nulls[Anum_pg_ora_part_tables_subpartclass - 1] = true;
		nulls[Anum_pg_ora_part_tables_subpartcollation - 1] = true;
		nulls[Anum_pg_ora_part_tables_subpartexprs - 1] = true;
	}

	if (!subpart_templdate_datum)
		nulls[Anum_pg_ora_part_tables_subpart_template - 1] = true;

	/* interval partition expressions will be filled later */
	nulls[Anum_pg_ora_part_tables_interval_expr - 1] = true;

	values[Anum_pg_ora_part_tables_partrelid - 1] = ObjectIdGetDatum(relationId);
	values[Anum_pg_ora_part_tables_automatic - 1] = BoolGetDatum(desc->automatic);
	values[Anum_pg_ora_part_tables_composite - 1] = BoolGetDatum(iscomposite);
	values[Anum_pg_ora_part_tables_seqno - 1] = Int32GetDatum(1);	/* update later */
	values[Anum_pg_ora_part_tables_subpart_template - 1] = subpart_templdate_datum;

	tuple = heap_form_tuple(RelationGetDescr(pg_ora_part_tables), values, nulls);

	CatalogTupleInsert(pg_ora_part_tables, tuple);
	table_close(pg_ora_part_tables, RowExclusiveLock);
}

/*
 *	UpdateOraclePartitionedTableSeqno
 *		Update the seqno of pg_ora_part_tables for an Oracle partitioned table.
 */
void
UpdateOraclePartitionedTableSeqno(Oid rootid, int32 seqno)
{
	HeapTuple	tuple;
	Relation	pg_ora_part_tables;
	Form_pg_ora_part_tables part_table_form;

	pg_ora_part_tables = table_open(OraPartitionedRelationId, RowExclusiveLock);

	tuple = SearchSysCacheCopy1(ORAPARTRELID, ObjectIdGetDatum(rootid));

	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "cache lookup failed for oracle partitioned table of relation %u",
			 rootid);

	part_table_form = (Form_pg_ora_part_tables) GETSTRUCT(tuple);
	part_table_form->seqno = seqno;
	CatalogTupleUpdate(pg_ora_part_tables, &tuple->t_self, tuple);

	heap_freetuple(tuple);
	table_close(pg_ora_part_tables, RowExclusiveLock);
	CommandCounterIncrement();
}

/*
 *	UpdateOraclePartitionedTableTemplate
 *		Update the subpartition template of pg_ora_part_tables for an Oracle partitioned table.
 */
void
UpdateOraclePartitionedTableTemplate(Oid rootid, OraSubPartitionTemplateSpec *subp_template)
{
	HeapTuple	tuple;
	HeapTuple	newtuple;
	Relation	pg_ora_part_tables;
	bool		nulls[Natts_pg_ora_part_tables];
	Datum		values[Natts_pg_ora_part_tables];
	bool		replaces[Natts_pg_ora_part_tables];

	MemSet(values, 0, sizeof(values));
	MemSet(nulls, false, sizeof(nulls));
	MemSet(replaces, false, sizeof(replaces));

	pg_ora_part_tables = table_open(OraPartitionedRelationId, RowExclusiveLock);

	tuple = SearchSysCacheCopy1(ORAPARTRELID, ObjectIdGetDatum(rootid));

	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "cache lookup failed for oracle partitioned table of relation %u",
			 rootid);

	if (subp_template)
	{
		values[Anum_pg_ora_part_tables_subpart_template - 1] = CStringGetTextDatum(nodeToString(subp_template));
		nulls[Anum_pg_ora_part_tables_subpart_template - 1] = false;
		replaces[Anum_pg_ora_part_tables_subpart_template - 1] = true;
		newtuple = heap_modify_tuple(tuple, RelationGetDescr(pg_ora_part_tables),
									 values, nulls, replaces);
	}
	else
	{
		nulls[Anum_pg_ora_part_tables_subpart_template - 1] = true;
		replaces[Anum_pg_ora_part_tables_subpart_template - 1] = true;
		newtuple = heap_modify_tuple(tuple, RelationGetDescr(pg_ora_part_tables),
									 values, nulls, replaces);
	}

	CatalogTupleUpdate(pg_ora_part_tables, &newtuple->t_self, newtuple);
	heap_freetuple(tuple);
	table_close(pg_ora_part_tables, RowExclusiveLock);
	CommandCounterIncrement();

	/*
	 * TODO:
	 * If the sub-partition template is placed in the partition descriptor
	 * in the future, then CacheInvalidateRelcache must be called here.
	 */
}

/*
 *	UpdateOraclePartitionedTableInterval
 *		Update the interval expression of pg_ora_part_tables for an Oracle partitioned table.
 *
 * This is worker routines for StorePartitionInterval, so CacheInvalidateRelcache and
 * CommandCounterIncrement will be done in StorePartitionInterval.
 */
void
UpdateOraclePartitionedTableInterval(Oid rootid, PartitionInterval *interval_expr)
{
	HeapTuple	tuple;
	HeapTuple	newtuple;
	Relation	pg_ora_part_tables;
	bool		nulls[Natts_pg_ora_part_tables];
	Datum		values[Natts_pg_ora_part_tables];
	bool		replaces[Natts_pg_ora_part_tables];

	MemSet(values, 0, sizeof(values));
	MemSet(nulls, false, sizeof(nulls));
	MemSet(replaces, false, sizeof(replaces));

	pg_ora_part_tables = table_open(OraPartitionedRelationId, RowExclusiveLock);

	tuple = SearchSysCacheCopy1(ORAPARTRELID, ObjectIdGetDatum(rootid));

	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "cache lookup failed for oracle partitioned table of relation %u",
			 rootid);

	if (interval_expr)
	{
		values[Anum_pg_ora_part_tables_interval_expr - 1] = CStringGetTextDatum(nodeToString(interval_expr));
		nulls[Anum_pg_ora_part_tables_interval_expr - 1] = false;
		replaces[Anum_pg_ora_part_tables_interval_expr - 1] = true;
		newtuple = heap_modify_tuple(tuple, RelationGetDescr(pg_ora_part_tables),
									 values, nulls, replaces);
	}
	else
	{
		nulls[Anum_pg_ora_part_tables_interval_expr - 1] = true;
		replaces[Anum_pg_ora_part_tables_interval_expr - 1] = true;
		newtuple = heap_modify_tuple(tuple, RelationGetDescr(pg_ora_part_tables),
									 values, nulls, replaces);
	}

	CatalogTupleUpdate(pg_ora_part_tables, &newtuple->t_self, newtuple);
	heap_freetuple(tuple);
	table_close(pg_ora_part_tables, RowExclusiveLock);
}

/*
 *	RemoveOraclePartitionedTable
 *		Remove pg_ora_part_tables entry for a an Oracle partitioned table.
 */
void
RemoveOraclePartitionedTable(Oid relid)
{
	Relation	rel;
	HeapTuple	tuple;

	rel = table_open(OraPartitionedRelationId, RowExclusiveLock);

	tuple = SearchSysCache1(ORAPARTRELID, ObjectIdGetDatum(relid));
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "cache lookup failed for oracle partitioned table of relation %u", relid);

	CatalogTupleDelete(rel, &tuple->t_self);

	ReleaseSysCache(tuple);
	table_close(rel, RowExclusiveLock);
}

/*
 * StoreOraclePartitions
 *		Store Oracle partitions and subpartitions information.
 */
void
StoreOraclePartitions(Oid partid, Oid parentid, Oid rootpartid,
							  char *partname, int16 partlevel, bool autogen)
{
	Datum		values[Natts_pg_ora_tab_partitions];
	bool		nulls[Natts_pg_ora_tab_partitions];
	Relation	pg_ora_tab_partitions;
	HeapTuple	tuple;
	NameData	part_name;

	/*
	 * sanity checks
	 */
	AssertArg(OidIsValid(partid) && OidIsValid(parentid));

	pg_ora_tab_partitions = table_open(OraTabPartitionsId, RowExclusiveLock);
	MemSet(nulls, false, sizeof(nulls));

	namestrcpy(&part_name, partname);
	values[Anum_pg_ora_tab_partitions_partroot - 1] = ObjectIdGetDatum(rootpartid);
	values[Anum_pg_ora_tab_partitions_partparent - 1] = ObjectIdGetDatum(parentid);
	values[Anum_pg_ora_tab_partitions_part - 1] = ObjectIdGetDatum(partid);
	values[Anum_pg_ora_tab_partitions_partname - 1] = NameGetDatum(&part_name);
	values[Anum_pg_ora_tab_partitions_partlevel - 1] = Int16GetDatum(partlevel);
	values[Anum_pg_ora_tab_partitions_autogen - 1] = BoolGetDatum(autogen);

	tuple = heap_form_tuple(RelationGetDescr(pg_ora_tab_partitions), values, nulls);

	CatalogTupleInsert(pg_ora_tab_partitions, tuple);
	table_close(pg_ora_tab_partitions, RowExclusiveLock);

	CommandCounterIncrement();
}

/*
 *	RemoveOraclePartitions
 *		Remove pg_ora_tab_partitions entry for an Oracle partitioned table.
 */
void
RemoveOraclePartitions(Oid relid)
{
	Relation	rel;
	HeapTuple	tuple;

	rel = table_open(OraTabPartitionsId, RowExclusiveLock);

	tuple = SearchSysCache1(ORATABPARTID, ObjectIdGetDatum(relid));
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "cache lookup failed for oracle (sub)partitione of relation %u", relid);

	CatalogTupleDelete(rel, &tuple->t_self);

	ReleaseSysCache(tuple);
	table_close(rel, RowExclusiveLock);
}

/*
 * StorePartitionKey
 *		Store information about the partition key into the catalog
 *		for automatic partitioning. except for partrelid and partdefid,
 *		all other fields were copied from the sibling partition.
 */
void
StorePartitionKeyFromSibling(Relation rel, Oid sibling)
{
	int			i;
	HeapTuple	sibling_tup;
	HeapTuple	new_tup;
	Relation	pg_partitioned_table;
	Datum		values[Natts_pg_partitioned_table];
	bool		nulls[Natts_pg_partitioned_table];
	Datum		datum;
	bool		isnull;
	int			partnatts = 0;
	AttrNumber	partattrs[PARTITION_MAX_KEYS];
	Oid			partopclass[PARTITION_MAX_KEYS];
	Oid			partcollation[PARTITION_MAX_KEYS];
	List		*partexprs = NIL;
	ObjectAddress	myself;
	ObjectAddress	referenced;
	ObjectAddresses	*addrs;

	Assert(rel->rd_rel->relkind == RELKIND_PARTITIONED_TABLE);

	sibling_tup = SearchSysCacheCopy1(PARTRELID, ObjectIdGetDatum(sibling));
	if (!HeapTupleIsValid(sibling_tup))
		elog(ERROR, "cache lookup failed for sibling of relation %u",
			 RelationGetRelid(rel));

	/*
	 * Copy data from sibling partition.
	 * To record dependencies, we also need to additionally deparser the value of Datum.
	 * Since the information about the sibling partitions has already entered the catalog,
	 * it means that everything is correct. We don't need to spend any more effort checking
	 * whether each field is null or whether the value is valid (such as whether opclass
	 * already exists).
	 */
	pg_partitioned_table = table_open(PartitionedRelationId, RowExclusiveLock);
	MemSet(nulls, false, sizeof(nulls));

	/* partstrat */
	datum = heap_getattr(sibling_tup, Anum_pg_partitioned_table_partstrat,
						 RelationGetDescr(pg_partitioned_table), &isnull);
	if (isnull)
		nulls[Anum_pg_partitioned_table_partstrat - 1] = true;
	else
		values[Anum_pg_partitioned_table_partstrat - 1] = datum;

	/* partnatts */
	datum = heap_getattr(sibling_tup, Anum_pg_partitioned_table_partnatts,
						 RelationGetDescr(pg_partitioned_table), &isnull);
	if (isnull)
		nulls[Anum_pg_partitioned_table_partnatts - 1] = true;
	else
	{
		values[Anum_pg_partitioned_table_partnatts - 1] = datum;
		partnatts = DatumGetInt16(datum);
	}

	/* partattrs */
	datum = heap_getattr(sibling_tup, Anum_pg_partitioned_table_partattrs,
						 RelationGetDescr(pg_partitioned_table), &isnull);
	if (isnull)
		nulls[Anum_pg_partitioned_table_partattrs - 1] = true;
	else
	{
		int2vector	*partattrs_vec;

		values[Anum_pg_partitioned_table_partattrs - 1] = datum;
		partattrs_vec = (int2vector *) DatumGetPointer(datum);
		for (i = 0; i < partnatts; i++)
			partattrs[i] = partattrs_vec->values[i];
	}

	/* partclass */
	datum = heap_getattr(sibling_tup, Anum_pg_partitioned_table_partclass,
						 RelationGetDescr(pg_partitioned_table), &isnull);
	if (isnull)
		nulls[Anum_pg_partitioned_table_partclass - 1] = true;
	else
	{
		oidvector	*partopclass_vec;

		values[Anum_pg_partitioned_table_partclass - 1] = datum;
		partopclass_vec = (oidvector *) DatumGetPointer(datum);
		for (i = 0; i < partnatts; i++)
			partopclass[i] = partopclass_vec->values[i];
	}

	/* partcollation */
	datum = heap_getattr(sibling_tup, Anum_pg_partitioned_table_partcollation,
						 RelationGetDescr(pg_partitioned_table), &isnull);
	if (isnull)
		nulls[Anum_pg_partitioned_table_partcollation - 1] = true;
	else
	{
		oidvector	*partcollation_vec;

		values[Anum_pg_partitioned_table_partcollation - 1] = datum;
		partcollation_vec = (oidvector *) DatumGetPointer(datum);
		for (i = 0; i < partnatts; i++)
			partcollation[i] = partcollation_vec->values[i];
	}

	/* partexprs */
	datum = heap_getattr(sibling_tup, Anum_pg_partitioned_table_partexprs,
						 RelationGetDescr(pg_partitioned_table), &isnull);
	if (isnull)
		nulls[Anum_pg_partitioned_table_partexprs - 1] = true;
	else
	{
		char	*str;

		values[Anum_pg_partitioned_table_partexprs - 1] = datum;
		str = TextDatumGetCString(datum);
		partexprs = castNode(List, stringToNode(str));
		pfree(str);
	}

	values[Anum_pg_partitioned_table_partrelid - 1] = ObjectIdGetDatum(RelationGetRelid(rel));
	values[Anum_pg_partitioned_table_partdefid - 1] = ObjectIdGetDatum(InvalidOid);

	new_tup = heap_form_tuple(RelationGetDescr(pg_partitioned_table), values, nulls);

	CatalogTupleInsert(pg_partitioned_table, new_tup);
	table_close(pg_partitioned_table, RowExclusiveLock);
	heap_freetuple(sibling_tup);

	/* Mark this relation as dependent on a few things as follows */
	addrs = new_object_addresses();
	ObjectAddressSet(myself, RelationRelationId, RelationGetRelid(rel));

	/* Operator class and collation per key column */
	for (i = 0; i < partnatts; i++)
	{
		ObjectAddressSet(referenced, OperatorClassRelationId, partopclass[i]);
		add_exact_object_address(&referenced, addrs);

		/* The default collation is pinned, so don't bother recording it */
		if (OidIsValid(partcollation[i]) &&
			partcollation[i] != DEFAULT_COLLATION_OID)
		{
			ObjectAddressSet(referenced, CollationRelationId, partcollation[i]);
			add_exact_object_address(&referenced, addrs);
		}
	}

	record_object_address_dependencies(&myself, addrs, DEPENDENCY_NORMAL);
	free_object_addresses(addrs);

	/*
	 * The partitioning columns are made internally dependent on the table,
	 * because we cannot drop any of them without dropping the whole table.
	 * (ATExecDropColumn independently enforces that, but it's not bulletproof
	 * so we need the dependencies too.)
	 */
	for (i = 0; i < partnatts; i++)
	{
		if (partattrs[i] == 0)
			continue;			/* ignore expressions here */

		ObjectAddressSubSet(referenced, RelationRelationId,
							RelationGetRelid(rel), partattrs[i]);
		recordDependencyOn(&referenced, &myself, DEPENDENCY_INTERNAL);
	}

	/*
	 * Also consider anything mentioned in partition expressions.  External
	 * references (e.g. functions) get NORMAL dependencies.  Table columns
	 * mentioned in the expressions are handled the same as plain partitioning
	 * columns, i.e. they become internally dependent on the whole table.
	 */
	if (partexprs)
		recordDependencyOnSingleRelExpr(&myself,
										(Node *) partexprs,
										RelationGetRelid(rel),
										DEPENDENCY_NORMAL,
										DEPENDENCY_INTERNAL,
										true /* reverse the self-deps */ );

	/*
	 * We must invalidate the relcache so that the next
	 * CommandCounterIncrement() will cause the same to be rebuilt using the
	 * information in just created catalog entry.
	 */
	CacheInvalidateRelcache(rel);
}

/*
 * update_next_range_partition_bound
 *
 * When drop a partition, if it is a range partitioning
 * strategy and there is a higher partition, then update
 * the lower bound value of the higher partition to the
 * value of lower bound of droped partition.
 *
 * If ALTER TABLE SET INTERVAL statement we need eliminate
 * gasp then update the lower bound value of the higher
 * partition to the value of upper bound of last partition.
 */
Oid
update_next_range_partition_bound(Oid relid, Oid parentOid, bool uselower)
{
	Oid			higherOid = InvalidOid;

	if (get_partitioning_strategy(parentOid) == PARTITION_STRATEGY_RANGE)
	{
		int			i;
		Relation	parentrel;
		PartitionDesc	parentdesc;

		/* caller has acquired the lock */
		parentrel = table_open(parentOid, NoLock);
		parentdesc = RelationGetPartitionDesc(parentrel, false);

		/* get oid of higher partition */
		for (i = 0; i < parentdesc->nparts; i++)
		{
			if (parentdesc->oids[i] == relid &&
				i < parentdesc->nparts - 1)
			{
				/* Oracle range partitioned table do not have a default partition */
				higherOid = parentdesc->oids[i + 1];
				break;
			}
		}

		if (higherOid)
		{
			Relation	pg_class;
			HeapTuple	tuple;
			HeapTuple	newtuple;
			List		*lowerb = NIL;
			PartitionBoundSpec	*boundspec = NULL;
			Datum		new_val[Natts_pg_class];
			bool		new_null[Natts_pg_class];
			bool		new_repl[Natts_pg_class];

			/* must lock it since it's bound will be changed */
			LockRelationOid(higherOid, AccessExclusiveLock);

			pg_class = table_open(RelationRelationId, RowExclusiveLock);

			/*
			 * Oracle partitioned tables do not support
			 * attach and detach, so this will succeed.
			 */
			tuple = SearchSysCacheCopy1(RELOID, ObjectIdGetDatum(relid));
			if (HeapTupleIsValid(tuple))
			{
				Datum		datum;
				bool		isnull;

				datum = SysCacheGetAttr(RELOID, tuple,
										Anum_pg_class_relpartbound,
										&isnull);
				if (!isnull)
					boundspec = stringToNode(TextDatumGetCString(datum));
				heap_freetuple(tuple);
			}
			else
				elog(ERROR, "cache lookup failed for partition bound of relation %u", relid);

			/* Sanity checks */
			if (!boundspec)
				elog(ERROR, "missing relpartbound for relation %u", relid);
			if (!IsA(boundspec, PartitionBoundSpec))
				elog(ERROR, "invalid relpartbound for relation %u", relid);

			if (uselower)
				lowerb = boundspec->lowerdatums;
			else
				lowerb = boundspec->upperdatums;

			/* get bounds of higher partition */
			tuple = SearchSysCacheCopy1(RELOID, ObjectIdGetDatum(higherOid));
			if (HeapTupleIsValid(tuple))
			{
				Datum		datum;
				bool		isnull;

				datum = SysCacheGetAttr(RELOID, tuple,
										Anum_pg_class_relpartbound,
										&isnull);
				if (!isnull)
					boundspec = stringToNode(TextDatumGetCString(datum));
				else
					elog(ERROR, "missing relpartbound for relation %u", higherOid);
			}
			else
				elog(ERROR, "cache lookup failed for partition bound of relation %u", higherOid);

			/* Sanity checks */
			if (!IsA(boundspec, PartitionBoundSpec))
				elog(ERROR, "invalid relpartbound for relation %u", higherOid);

			/* do update */
			boundspec->lowerdatums = lowerb;

			memset(new_val, 0, sizeof(new_val));
			memset(new_null, false, sizeof(new_null));
			memset(new_repl, false, sizeof(new_repl));

			new_val[Anum_pg_class_relpartbound - 1] = CStringGetTextDatum(nodeToString(boundspec));
			new_null[Anum_pg_class_relpartbound - 1] = false;
			new_repl[Anum_pg_class_relpartbound - 1] = true;

			newtuple = heap_modify_tuple(tuple, RelationGetDescr(pg_class),
										 new_val, new_null, new_repl);

			CatalogTupleUpdate(pg_class, &newtuple->t_self, newtuple);

			heap_freetuple(tuple);
			heap_freetuple(newtuple);
			table_close(pg_class, RowExclusiveLock);
		}

		table_close(parentrel, NoLock);
	}

	return higherOid;
}


/* ----------------------------------------------------------------
 *					parse analysis interface
 * ----------------------------------------------------------------
 */
/*
 *	transformOraclePartitionSpec
 *		Derive the CreateStmt(s) for partitions and subpartitions.
 */
List *
transformOraclePartitionSpec(OraPartitionSpec *ora_partspec, RangeVar *parent)
{
	List		*result = NIL;
	CreateStmt	*lcstmt;
	int			final_seqno;
	HTAB		*part_duplicate_check = NULL;
	HTAB		*part_relname_duplicate_check = NULL;

	part_duplicate_check = InitPartNameDuplicateCheckHashTable();
	part_relname_duplicate_check = InitPartRelnameDuplicateCheckHashTable();

	switch (ora_partspec->partition_strategy)
	{
		case PARTITION_RANGE:
		case PARTITION_LIST:
		case PARTITION_HASH:
			{
				char	part_strategy;

				/* Parse partition strategy name */
				if (pg_strcasecmp(ora_partspec->root_partiton_spec->strategy, "hash") == 0)
					part_strategy = PARTITION_STRATEGY_HASH;
				else if (pg_strcasecmp(ora_partspec->root_partiton_spec->strategy, "list") == 0)
					part_strategy = PARTITION_STRATEGY_LIST;
				else if (pg_strcasecmp(ora_partspec->root_partiton_spec->strategy, "range") == 0)
					part_strategy = PARTITION_STRATEGY_RANGE;
				else
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
							 errmsg("unrecognized partitioning strategy \"%s\"",
									ora_partspec->root_partiton_spec->strategy)));

				result = derivePostgresPartitionStmts(part_strategy,
													  ora_partspec->partiton_desc,
													  parent,
													  NULL, /* simple partition */
													  NULL,
													  NULL,
													  list_length(ora_partspec->root_partiton_spec->partParams),
													  false,
													  ora_partspec->hash_partition_quantity,
													  ora_partspec->automatic,
													  ora_partspec->part_interval != NULL,
													  NIL,
													  part_duplicate_check,
													  part_relname_duplicate_check,
													  1,
													  &final_seqno);
			}
			break;

		case PARTITION_COMPOSITE_RANGE:
		case PARTITION_COMPOSITE_LIST:
		case PARTITION_COMPOSITE_HASH:
			{
				List		*part_stmt_list = NIL;
				List		*subpart_stmt_list = NIL;
				char		part_strategy;
				char		subpart_strategy;
				int			mid_seqno;
				ListCell	*lc;
				ListCell	*lc2;

				Assert(ora_partspec->root_partiton_spec);
				Assert(ora_partspec->sub_partiton_spec);

				/* Parse (sub)partition strategy name */
				if (pg_strcasecmp(ora_partspec->root_partiton_spec->strategy, "hash") == 0)
					part_strategy = PARTITION_STRATEGY_HASH;
				else if (pg_strcasecmp(ora_partspec->root_partiton_spec->strategy, "list") == 0)
					part_strategy = PARTITION_STRATEGY_LIST;
				else if (pg_strcasecmp(ora_partspec->root_partiton_spec->strategy, "range") == 0)
					part_strategy = PARTITION_STRATEGY_RANGE;
				else
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
							 errmsg("unrecognized partitioning strategy \"%s\"",
									ora_partspec->root_partiton_spec->strategy)));

				if (pg_strcasecmp(ora_partspec->sub_partiton_spec->strategy, "hash") == 0)
					subpart_strategy = PARTITION_STRATEGY_HASH;
				else if (pg_strcasecmp(ora_partspec->sub_partiton_spec->strategy, "list") == 0)
					subpart_strategy = PARTITION_STRATEGY_LIST;
				else if (pg_strcasecmp(ora_partspec->sub_partiton_spec->strategy, "range") == 0)
					subpart_strategy = PARTITION_STRATEGY_RANGE;
				else
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
							 errmsg("unrecognized partitioning strategy \"%s\"",
									ora_partspec->sub_partiton_spec->strategy)));

				/* 1-level partition */
				part_stmt_list = derivePostgresPartitionStmts(part_strategy,
														ora_partspec->partiton_desc,
														parent,
														ora_partspec->sub_partiton_spec,
														NULL,
														NULL,
														list_length(ora_partspec->root_partiton_spec->partParams),
														false,
														ora_partspec->hash_partition_quantity,
														ora_partspec->automatic,
														ora_partspec->part_interval != NULL,
														NIL,
														part_duplicate_check,
														part_relname_duplicate_check,
														1,
														&mid_seqno);

				result = list_concat(result, part_stmt_list);

				checkOracleSubpartitionDesc(ora_partspec->sub_part_template,
											subpart_strategy,
											list_length(ora_partspec->sub_partiton_spec->partParams),
											true);

				/* PARTITIONS clause of a HASH composite partition is handled separately */
				if (ora_partspec->partiton_desc)
				{
					if (unlikely(list_length(ora_partspec->partiton_desc) != list_length(part_stmt_list)))
						elog(ERROR, "Partition internal error");	/* shouldn't happen */

					forboth(lc, ora_partspec->partiton_desc, lc2, part_stmt_list)
					{
						OraPartitionBoundSpec	*part = (OraPartitionBoundSpec *) lfirst(lc);
						CreateStmt	*stmt = (CreateStmt *) lfirst(lc2);

						checkOracleSubpartitionDesc(part->subpart_bound,
													subpart_strategy,
													list_length(ora_partspec->sub_partiton_spec->partParams),
													false);

						/* 2-level subpartition */
						subpart_stmt_list = derivePostgresPartitionStmts(subpart_strategy,
																 part->subpart_bound ? part->subpart_bound->subpartitons_desc : NULL,
																 stmt->relation,
																 NULL,
																 ora_partspec->sub_part_template,
																 stmt->ora_partsup->partname,
																 list_length(ora_partspec->sub_partiton_spec->partParams),
																 true,
																 part->subpart_bound ? part->subpart_bound->hash_subpartition_quantity : 0,
																 false,
																 false,
																 NIL,
																 part_duplicate_check,
																 part_relname_duplicate_check,
																 mid_seqno,
																 &final_seqno);

						result = list_concat(result, subpart_stmt_list);
						mid_seqno = final_seqno;
					}
				}
				else
				{
					/* PARTITIONS clause of HASH composite partition */
					if (ora_partspec->hash_partition_quantity != list_length(part_stmt_list))
						elog(ERROR, "Partition internal error");	/* shouldn't happen */

					foreach(lc2, part_stmt_list)
					{
						CreateStmt	*stmt = (CreateStmt *) lfirst(lc2);

						/* 2-level subpartition */
						subpart_stmt_list = derivePostgresPartitionStmts(subpart_strategy,
																 NULL,
																 stmt->relation,
																 NULL,
																 ora_partspec->sub_part_template,
																 stmt->ora_partsup->partname,
																 list_length(ora_partspec->sub_partiton_spec->partParams),
																 true,
																 0,
																 false,
																 false,
																 NIL,
																 part_duplicate_check,
																 part_relname_duplicate_check,
																 mid_seqno,
																 &final_seqno);

						result = list_concat(result, subpart_stmt_list);
						mid_seqno = final_seqno;
					}
				}

				break;
			}
	}

	/*
	 * Use the last statement to update the latest value of
	 * the sequence currently used by the partitioned table.
	 */
	lcstmt = (CreateStmt *)llast(result);
	lcstmt->ora_partsup->seqno = final_seqno;

	/*
	 * If an INTERVAL clause exists, append an ALTER TABLE to convert
	 * the range partitioned table into an interval partitioned table.
	 */
	if ((ora_partspec->partition_strategy == PARTITION_RANGE ||
		 ora_partspec->partition_strategy == PARTITION_COMPOSITE_RANGE) &&
		ora_partspec->part_interval)
	{
		AlterTableStmt	*alterstmt = makeNode(AlterTableStmt);
		AlterTableCmd	*sub = makeNode(AlterTableCmd);

		sub->subtype = AT_SetInterval;
		sub->def = (Node *) ora_partspec->part_interval;

		alterstmt->relation = parent;
		alterstmt->cmds = list_make1(sub);
		alterstmt->objtype = OBJECT_TABLE;
		alterstmt->missing_ok = false;

		result = lappend(result, alterstmt);
		ora_partspec->part_interval = NULL;	/* create root partition without interval clause */
	}

	/* clean up */
	hash_destroy(part_duplicate_check);
	hash_destroy(part_relname_duplicate_check);

	return result;
}

/*
 *	derivePostgresPartitionStmts
 *		Worker for derive the CreateStmt of (sub)partitions.
 */
List *
derivePostgresPartitionStmts(char strategy, List *part_desc, RangeVar *part_parent,
									PartitionSpec *subpart_key, OraSubPartitionTemplateSpec *subpart_template,
									char *parent_part_name, int part_key_cnt, bool is_subpart, int hash_part_quantity,
									bool is_automatic, bool is_interval, List *lower_bound, HTAB *part_duplicate_check,
									HTAB *relname_duplicate_check, int ora_partname_seqno, int *final_seqno)
{
	List		*result = NIL;
	ListCell	*lc;
	bool		using_subpart_template = false;

	Assert(part_duplicate_check);
	Assert(relname_duplicate_check);

	switch (strategy)
	{
		case PARTITION_STRATEGY_RANGE:
			{
				List		*part_lowerdatums = NIL;
				int			i;

				if (lower_bound)
				{
					if (list_length(lower_bound) == part_key_cnt)
						part_lowerdatums = lower_bound;
					else
						elog(ERROR, "An error occurred while generating the range partition boundary.");
				}
				else
				{
					/*
					 * Constructs the lower bound of the first range partition.
					 * the lower bound of the first range partition all are MINVALUE,
					 * and the lower bound of subsequent range partitions is the upper
					 * bound of the previous partition.
					 */
					for (i = 0; i < part_key_cnt; i++)
					{
						ColumnRef	*first_part_lower_b;

						first_part_lower_b = makeNode(ColumnRef);
						first_part_lower_b->fields = list_make1(makeString("minvalue"));
						first_part_lower_b->location = -1;

						part_lowerdatums = lappend(part_lowerdatums, first_part_lower_b);
					}
				}

				/*
				 * If you omit both subpartition_template and the range_subpartition_desc,
				 * then the database creates a single MAXVALUE subpartition.
				 */
				if (is_subpart && part_desc == NULL)
				{
					if (subpart_template)
					{
						part_desc = subpart_template->subpartitons_desc;
						using_subpart_template = true;
					}
					else
					{
						List		*single_part_upperdatums = NIL;
						char		*single_part_schema;
						char		*single_part_rel_name;
						char		*single_part_name;
						RangeVar	*single_part_relvar;
						PartitionBoundSpec	*single_part_bound;
						CreateStmt	*part_stmt = makeNode(CreateStmt);

						for (i = 0; i < part_key_cnt; i++)
						{
							ColumnRef	*upperdatum;

							upperdatum = makeNode(ColumnRef);
							upperdatum->fields = list_make1(makeString("maxvalue"));
							upperdatum->location = -1;

							single_part_upperdatums = lappend(single_part_upperdatums, upperdatum);
						}

						single_part_name = make_system_partname_unique(part_duplicate_check,
																		true,
																		ora_partname_seqno,
																		&ora_partname_seqno);

						single_part_schema = pstrdup(part_parent->schemaname);
						single_part_rel_name = psprintf("%s_%s", part_parent->relname, single_part_name);
						single_part_rel_name = make_part_relname_unique(single_part_rel_name,
																		get_namespace_oid(single_part_schema, false),
																		relname_duplicate_check);

						single_part_relvar = makeRangeVar(single_part_schema, single_part_rel_name, -1);
						single_part_bound = makeNode(PartitionBoundSpec);
						single_part_bound->strategy = PARTITION_STRATEGY_RANGE;
						single_part_bound->is_default = false;
						single_part_bound->lowerdatums = part_lowerdatums;
						single_part_bound->upperdatums = single_part_upperdatums;
						single_part_bound->location = -1;

						part_stmt->relation = single_part_relvar;
						part_stmt->tableElts = NIL;
						part_stmt->inhRelations = list_make1(copyObject(part_parent));
						part_stmt->partbound = single_part_bound;
						part_stmt->ora_partsup = makeOraPartSup(single_part_name, 2, 0, false);
						part_stmt->partspec = NULL;
						part_stmt->oncommit = ONCOMMIT_NOOP;
						part_stmt->tablespacename = NULL;
						part_stmt->if_not_exists = false;

						result = list_make1(part_stmt);

						if (final_seqno)
							*final_seqno = ora_partname_seqno;

						return result;

					}
				}

				foreach(lc, part_desc)
				{
					char		*part_schema;
					char		*part_rel_name;
					char		*part_name;
					RangeVar	*part_relation;
					CreateStmt	*part_stmt = makeNode(CreateStmt);
					PartitionBoundSpec		*part_bound;
					OraPartitionBoundSpec	*part = (OraPartitionBoundSpec *) lfirst(lc);

					/*
					 * ora_gram.y can ensures that the partition strategy is consistent
					 * when is CreateStmt, this mainly used in ALTER TABLE ADD PARTITION.
					 */
					if (part->bound->strategy != strategy)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_TABLE_DEFINITION),
								 errmsg("partition description clause is inconsistent with the partition strategy")));

					/*
					 * ORA-14020:
					 * this physical attribute may not be specified for a table partition.
					 *
					 * Oracle's error message is not very friendly. Perhaps it is a common
					 * error in many Oracle scenarios. We add a hint to help users pinpoint
					 * the cause of this error.
					 */
					if (subpart_key == NULL && part->subpart_bound)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_TABLE_DEFINITION),
								 errmsg("this physical attribute may not be specified for a table partition"),
								 errhint("Not a composite partition but subpartition description specified.")));

					/*
					 * ORA-14017 && ORA-14018
					 * The number of boundary values for a RANGE (sub)partition
					 * must be equal to the number of (sub)partition keys.
					 */
					if (part_key_cnt > list_length(part->bound->upperdatums))
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_TABLE_DEFINITION),
								 errmsg("partition bound list contains too few elements")));
					else if (part_key_cnt < list_length(part->bound->upperdatums))
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_TABLE_DEFINITION),
								 errmsg("partition bound list contains too many elements")));

					/* ORA-14761 */
					if (is_interval)
					{
						Node	*expr = linitial(part->bound->upperdatums);	/* interval partition only one part key */

						if (IsA(expr, ColumnRef) ||
							IsA(expr, ColumnRefOrFuncCall))
						{
							ColumnRef  *cref = NULL;
							char	   *cname = NULL;

							if (IsA(expr, ColumnRef))
								cref = (ColumnRef *) expr;
							else
								cref = ((ColumnRefOrFuncCall *) expr)->cref;

							if (list_length(cref->fields) == 1 &&
								IsA(linitial(cref->fields), String))
								cname = strVal(linitial(cref->fields));

							if (cname && strcmp("maxvalue", cname) == 0)
							{
								ereport(ERROR,
										(errcode(ERRCODE_INVALID_TABLE_DEFINITION),
										 errmsg("MAXVALUE partition cannot be specified for INTERVAL partitioned objects")));
							}
						}
					}

					/*
					 * Partitions of different Oracle partitioned tables can have the same name,
					 * Our partitions cannot have the same name in the same schema, so we add a
					 * prefix to the partition name using partitioned table name.
					 *
					 * In Oracle, only partitioned table can have schema, but partitions cannot,
					 * therefore, we have a convention here:
					 * partitions and partitioned table are located in the same schema.
					 */
					part_schema = pstrdup(part_parent->schemaname);
					if (part->partition_name)
					{
						part_rel_name = psprintf("%s_%s", part_parent->relname, part->partition_name);
						if (using_subpart_template)
							part_name = psprintf("%s_%s", parent_part_name, part->partition_name);
						else
							part_name = pstrdup(part->partition_name);
					}
					else
					{
						part_name = make_system_partname_unique(part_duplicate_check,
																is_subpart,
																ora_partname_seqno,
																&ora_partname_seqno);

						part_rel_name = psprintf("%s_%s", part_parent->relname, part_name);
					}

					part_rel_name = make_part_relname_unique(part_rel_name,
															 get_namespace_oid(part_schema, false),
															 relname_duplicate_check);

					/* SYS_Pn and SYS_SUBPn already inserted hashtab by make_system_partname_unique */
					if (part_duplicate_check &&
						part->partition_name)
					{
						char	*pname;
						bool	found;

						pname = hash_search(part_duplicate_check, part_name,
											HASH_ENTER,
											&found);
						if (found)
							ereport(ERROR,
									(errcode(ERRCODE_DUPLICATE_TABLE),
									 errmsg(is_subpart ?
									 "duplicate subpartition name \"%s\"" :
									 "duplicate partition name \"%s\"", pname)));
					}

					part_relation = makeRangeVar(part_schema, part_rel_name, -1);
					part_bound = copyObject(part->bound);
					part_bound->lowerdatums = part_lowerdatums;	/* Update partition lower bound */

					part_stmt->relation = part_relation;
					part_stmt->tableElts = NIL;
					part_stmt->inhRelations = list_make1(copyObject(part_parent));
					part_stmt->partbound = part_bound;
					part_stmt->partspec = copyObject(subpart_key);
					part_stmt->ora_partsup = makeOraPartSup(part_name, is_subpart ? 2 : 1, 0, false);
					part_stmt->oncommit = ONCOMMIT_NOOP;
					part_stmt->tablespacename = part->tablespace_name ? pstrdup(part->tablespace_name) : NULL;
					part_stmt->if_not_exists = false;

					/* lower bound of next partition */
					part_lowerdatums = list_copy_deep(part->bound->upperdatums);
					result = lappend(result, part_stmt);
				}
				break;
			}

		case PARTITION_STRATEGY_LIST:
			{
				/*
				 * If you omit both subpartition_template and the list_subpartition_desc,
				 * then the database creates a single DEFAULT subpartition.
				 */
				if (is_subpart && part_desc == NULL)
				{
					if (subpart_template)
					{
						part_desc = subpart_template->subpartitons_desc;
						using_subpart_template = true;
					}
					else
					{
						char		*single_part_schema;
						char		*single_part_rel_name;
						char		*single_part_name;
						RangeVar	*single_part_relvar;
						PartitionBoundSpec	*single_part_bound;
						CreateStmt	*part_stmt = makeNode(CreateStmt);

						single_part_name = make_system_partname_unique(part_duplicate_check,
																		true,
																		ora_partname_seqno,
																		&ora_partname_seqno);

						single_part_schema = pstrdup(part_parent->schemaname);
						single_part_rel_name = psprintf("%s_%s", part_parent->relname, single_part_name);
						single_part_rel_name = make_part_relname_unique(single_part_rel_name,
																		get_namespace_oid(single_part_schema, false),
																		relname_duplicate_check);

						single_part_relvar = makeRangeVar(single_part_schema, single_part_rel_name, -1);
						single_part_bound = makeNode(PartitionBoundSpec);
						single_part_bound->strategy = PARTITION_STRATEGY_LIST;
						single_part_bound->is_default = true;
						single_part_bound->location = -1;

						part_stmt->relation = single_part_relvar;
						part_stmt->tableElts = NIL;
						part_stmt->inhRelations = list_make1(copyObject(part_parent));
						part_stmt->partbound = single_part_bound;
						part_stmt->partspec = NULL;
						part_stmt->ora_partsup = makeOraPartSup(single_part_name, 2, 0, false);
						part_stmt->oncommit = ONCOMMIT_NOOP;
						part_stmt->tablespacename = NULL;
						part_stmt->if_not_exists = false;

						result = list_make1(part_stmt);

						if (final_seqno)
							*final_seqno = ora_partname_seqno;

						return result;

					}
				}

				foreach(lc, part_desc)
				{
					char		*part_schema;
					char		*part_rel_name;
					char		*part_name;
					RangeVar	*part_relvar;
					CreateStmt	*part_stmt = makeNode(CreateStmt);
					PartitionBoundSpec		*part_bound;
					OraPartitionBoundSpec	*part = (OraPartitionBoundSpec *) lfirst(lc);

					if (part->bound->strategy != strategy)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_TABLE_DEFINITION),
								 errmsg("partition description clause is inconsistent with the partition strategy")));

					/* ORA-14020 */
					if (subpart_key == NULL && part->subpart_bound)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_TABLE_DEFINITION),
								 errmsg("this physical attribute may not be specified for a table partition"),
								 errhint("Not a composite partition but subpartition description specified.")));

					/* ORA-14318:DEFAULT partition must be last partition specified */
					if (part->bound->is_default && lnext(part_desc, lc))
						elog(ERROR, "DEFAULT partition must be last partition specified");

					/* ORA-14851 */
					if (is_automatic && part->bound->is_default)
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_TABLE_DEFINITION),
								 errmsg("DEFAULT [sub]partition cannot be specified for AUTOLIST [sub]partitioned objects")));

					/*
					 * ORA-14018
					 * The number of boundary values in a LIST (sub)partition must be greater than
					 * or equal to the number of (sub)partition keys, except for the DEFAULT partition.
					 */
					if (!part->bound->is_default &&
						part_key_cnt > list_length(part->bound->listdatums))
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_TABLE_DEFINITION),
								 errmsg("partition bound list contains too few elements")));

					part_schema = pstrdup(part_parent->schemaname);
					if (part->partition_name)
					{
						part_rel_name = psprintf("%s_%s", part_parent->relname, part->partition_name);
						if (using_subpart_template)
							part_name = psprintf("%s_%s", parent_part_name, part->partition_name);
						else
							part_name = pstrdup(part->partition_name);
					}
					else
					{
						part_name = make_system_partname_unique(part_duplicate_check,
																is_subpart,
																ora_partname_seqno,
																&ora_partname_seqno);

						part_rel_name = psprintf("%s_%s", part_parent->relname, part_name);
					}

					part_rel_name = make_part_relname_unique(part_rel_name,
															 get_namespace_oid(part_schema, false),
															 relname_duplicate_check);

					/*
					 * ORA-14013: duplicate partition name
					 * ORA-14159: duplicate subpartition name
					 */
					if (part_duplicate_check &&
						part->partition_name)
					{
						char	*pname;
						bool	found;

						pname = hash_search(part_duplicate_check, part_name,
											HASH_ENTER,
											&found);
						if (found)
							ereport(ERROR,
									(errcode(ERRCODE_DUPLICATE_TABLE),
									 errmsg(is_subpart ?
									 "duplicate subpartition name \"%s\"" :
									 "duplicate partition name \"%s\"", pname)));
					}

					part_relvar = makeRangeVar(part_schema, part_rel_name, -1);
					part_bound = copyObject(part->bound);

					part_stmt->relation = part_relvar;
					part_stmt->tableElts = NIL;
					part_stmt->inhRelations = list_make1(copyObject(part_parent));
					part_stmt->partbound = part_bound;
					part_stmt->partspec = copyObject(subpart_key);
					part_stmt->ora_partsup = makeOraPartSup(part_name, is_subpart ? 2 : 1, 0, false);
					part_stmt->oncommit = ONCOMMIT_NOOP;
					part_stmt->tablespacename = part->tablespace_name ? pstrdup(part->tablespace_name) : NULL;
					part_stmt->if_not_exists = false;

					result = lappend(result, part_stmt);
				}
				break;
			}

		case PARTITION_STRATEGY_HASH:
			{
				int		modulus;
				int		i;

				/*
				 * If you omit both subpartition_template and the individual_hash_subparts and
				 * SUBPARTITIONS integer, then the database creates a single hash subpartition.
				 */
				if (is_subpart &&
					part_desc == NULL &&
					hash_part_quantity == 0)
				{
					if (subpart_template)
					{
						part_desc = subpart_template->subpartitons_desc;
						hash_part_quantity = subpart_template->hash_subpartition_quantity;
						using_subpart_template = true;
					}
					else
					{
						char		*single_part_schema;
						char		*single_part_rel_name;
						char		*single_part_name;
						RangeVar	*single_part_relvar;
						PartitionBoundSpec	*single_part_bound;
						CreateStmt	*part_stmt = makeNode(CreateStmt);

						single_part_name = make_system_partname_unique(part_duplicate_check,
																		true,
																		ora_partname_seqno,
																		&ora_partname_seqno);

						single_part_schema = pstrdup(part_parent->schemaname);
						single_part_rel_name = psprintf("%s_%s", part_parent->relname, single_part_name);
						single_part_rel_name = make_part_relname_unique(single_part_rel_name,
																		get_namespace_oid(single_part_schema, false),
																		relname_duplicate_check);

						single_part_relvar = makeRangeVar(single_part_schema, single_part_rel_name, -1);
						single_part_bound = makeNode(PartitionBoundSpec);
						single_part_bound->strategy = PARTITION_STRATEGY_HASH;
						single_part_bound->modulus = 1;
						single_part_bound->remainder = 0;
						single_part_bound->location = -1;

						part_stmt->relation = single_part_relvar;
						part_stmt->tableElts = NIL;
						part_stmt->inhRelations = list_make1(copyObject(part_parent));
						part_stmt->partbound = single_part_bound;
						part_stmt->partspec = NULL;
						part_stmt->ora_partsup = makeOraPartSup(single_part_name, 2, 0, false);
						part_stmt->oncommit = ONCOMMIT_NOOP;
						part_stmt->tablespacename = NULL;
						part_stmt->if_not_exists = false;

						result = list_make1(part_stmt);

						if (final_seqno)
							*final_seqno = ora_partname_seqno;

						return result;

					}
				}

				if (hash_part_quantity > 0)
					modulus = hash_part_quantity;
				else
					modulus = list_length(part_desc);

				for (i = 0; i < modulus; i++)
				{
					char		*part_schema;
					char		*part_rel_name;
					char		*part_name;
					RangeVar	*part_relvar;
					CreateStmt	*part_stmt = makeNode(CreateStmt);
					PartitionBoundSpec		*part_bound;
					OraPartitionBoundSpec	*part = NULL;

					part_schema = pstrdup(part_parent->schemaname);

					if (hash_part_quantity > 0)
					{
						part_name = make_system_partname_unique(part_duplicate_check,
																is_subpart,
																ora_partname_seqno,
																&ora_partname_seqno);

						part_rel_name = psprintf("%s_%s", part_parent->relname, part_name);
					}
					else
					{
						part = list_nth_node(OraPartitionBoundSpec, part_desc, i);

						/* ORA-14020 */
						if (subpart_key == NULL && part->subpart_bound)
							ereport(ERROR,
									(errcode(ERRCODE_INVALID_TABLE_DEFINITION),
									 errmsg("this physical attribute may not be specified for a table partition"),
									 errhint("Not a composite partition but subpartition description specified.")));

						if (part->partition_name)
						{
							part_rel_name = psprintf("%s_%s", part_parent->relname, part->partition_name);
							if (using_subpart_template)
								part_name = psprintf("%s_%s", parent_part_name, part->partition_name);
							else
								part_name = pstrdup(part->partition_name);
						}
						else
						{
							part_name = make_system_partname_unique(part_duplicate_check,
																	is_subpart,
																	ora_partname_seqno,
																	&ora_partname_seqno);

							part_rel_name = psprintf("%s_%s", part_parent->relname, part_name);
						}
					}

					part_rel_name = make_part_relname_unique(part_rel_name,
															 get_namespace_oid(part_schema, false),
															 relname_duplicate_check);

					/*
					 * ORA-14013: duplicate partition name
					 * ORA-14159: duplicate subpartition name
					 */
					if (part_duplicate_check &&
						part &&
						part->partition_name)
					{
						char	*pname;
						bool	found;

						pname = hash_search(part_duplicate_check, part_name,
											HASH_ENTER,
											&found);
						if (found)
							ereport(ERROR,
									(errcode(ERRCODE_DUPLICATE_TABLE),
									 errmsg(is_subpart ?
									 "duplicate subpartition name \"%s\"" :
									 "duplicate partition name \"%s\"", pname)));
					}

					part_bound = makeNode(PartitionBoundSpec);
					part_bound->strategy = PARTITION_STRATEGY_HASH;
					part_bound->modulus = modulus;
					part_bound->remainder = i;
					part_bound->location = -1;

					part_relvar = makeRangeVar(part_schema, part_rel_name, -1);
					part_stmt->relation = part_relvar;
					part_stmt->tableElts = NIL;
					part_stmt->inhRelations = list_make1(copyObject(part_parent));
					part_stmt->partbound = part_bound;
					part_stmt->partspec = copyObject(subpart_key);
					part_stmt->ora_partsup = makeOraPartSup(part_name, is_subpart ? 2 : 1, 0, false);
					part_stmt->oncommit = ONCOMMIT_NOOP;
					/* TODO: TABLESPACE come from STORE IN clause */
					part_stmt->tablespacename = (part && part->tablespace_name) ? pstrdup(part->tablespace_name) : NULL;
					part_stmt->if_not_exists = false;

					result = lappend(result, part_stmt);
				}
				break;
			}
	}

	if (final_seqno)
		*final_seqno = ora_partname_seqno;

	return result;
}

/*
 *	checkOracleSubpartitionDesc
 *		Check the subpartition description or subpartition template.
 */
void
checkOracleSubpartitionDesc(const OraSubPartitionTemplateSpec *desc, char strategy, int keycnt, bool istempldate)
{
	ListCell   *lc;
	HTAB		*subp_template_htab = NULL;
	HASHCTL		hash_ctl;

	if (!desc)
		return;

	/* hash subpartition by quantity */
	if (desc->hash_subpartition_quantity > 0 &&
		desc->subpartitons_desc == NIL &&
		strategy != PARTITION_STRATEGY_HASH)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg(istempldate ?
				 "syntax error in subpartition template" :
				 "syntax error in subpartition description")));

	hash_ctl.keysize = NAMEDATALEN;
	hash_ctl.entrysize = NAMEDATALEN;
	hash_ctl.hcxt = CurrentMemoryContext;
	subp_template_htab = hash_create("subpartition template name check", 16,
									 &hash_ctl,
									 HASH_ELEM | HASH_STRINGS);

	foreach(lc, desc->subpartitons_desc)
	{
		char	*pname;
		bool	found;

		OraPartitionBoundSpec	*sub_part = (OraPartitionBoundSpec *) lfirst(lc);

		/*
		 * ORA-14605: Name missing for subpartition/lob segment in template.
		 * the subpartition name must be specified in the subpartition,
		 * template and subpartition names cannot be repeated.
		 */
		if (istempldate && sub_part->partition_name == NULL)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_NAME),
					 errmsg("Name missing for subpartition in template")));

		if (sub_part->partition_name)
		{
			pname = hash_search(subp_template_htab, sub_part->partition_name,
								HASH_ENTER,
								&found);
			if (found)
				ereport(ERROR,
						(errcode(ERRCODE_DUPLICATE_TABLE),
						 errmsg("duplicate subpartition name \"%s\"", pname)));
		}

		switch (strategy)
		{
			case PARTITION_STRATEGY_RANGE:
				if (sub_part->bound == NULL ||
					sub_part->bound->strategy != strategy)
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg(istempldate ?
							 "syntax error in subpartition template" :
							 "syntax error in subpartition description")));

				/* ORA-14017 && ORA-14018 */
				if (keycnt > list_length(sub_part->bound->upperdatums))
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_TABLE_DEFINITION),
							 errmsg("partition bound list contains too few elements")));
				else if (keycnt < list_length(sub_part->bound->upperdatums))
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_TABLE_DEFINITION),
							 errmsg("partition bound list contains too many elements")));
				break;

			case PARTITION_STRATEGY_LIST:
				if (sub_part->bound == NULL ||
					sub_part->bound->strategy != strategy)
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg(istempldate ?
							 "syntax error in subpartition template" :
							 "syntax error in subpartition description")));

				/* ORA-14318:DEFAULT partition must be last partition specified */
				if (sub_part->bound->is_default && lnext(desc->subpartitons_desc, lc))
					elog(ERROR, "DEFAULT partition must be last partition specified");

				/* ORA-14017 */
				if (!sub_part->bound->is_default &&
					keycnt > list_length(sub_part->bound->listdatums))
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_TABLE_DEFINITION),
							 errmsg("partition bound list contains too few elements")));
				break;

			case PARTITION_STRATEGY_HASH:
				/* Hash partitions have no boundary values, so sub_part->bound must be NULL */
				if (sub_part->bound)
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg(istempldate ?
							 "syntax error in subpartition template" :
							 "syntax error in subpartition description")));
				break;
		}

		/* verify tablespace */
		if (sub_part->tablespace_name &&
			!OidIsValid(get_tablespace_oid(sub_part->tablespace_name, true)))
			ereport(ERROR,
					(errcode(ERRCODE_DUPLICATE_OBJECT),
					 errmsg("tablespace \"%s\" does not exist",
							sub_part->tablespace_name)));
	}

	/* htab clean up */
	hash_destroy(subp_template_htab);
}

/*
 *	make_system_partname_unique
 *		Ensure that the internally generated partition name SYS_Pn is unique.
 */
char *
make_system_partname_unique(HTAB *partnamehash, bool is_subpartition, int start_seqno, int *end_seqno)
{
	char	*basename = psprintf(is_subpartition ? "sys_subp" : "sys_p");
	char	*partname = psprintf("%s%d", basename, start_seqno);
	char	*hash_entry;

	for (;;)
	{
		char	*tmp;

		hash_entry = hash_search(partnamehash, partname,
								 HASH_FIND, NULL);

		/* everything is ok */
		if (hash_entry == NULL)
			break;

		/* seqno increment */
		tmp = psprintf("%s%d", basename, ++start_seqno);
		pfree(partname);
		partname = tmp;
	}

	if (start_seqno < 0 || strlen(partname) >= NAMEDATALEN)
		elog(ERROR, "partition/subpartition name is too long, please adjust it");

	pfree(basename);

	/* just insert it */
	(void) hash_search(partnamehash, partname, HASH_ENTER, NULL);

	if (end_seqno)
		*end_seqno = start_seqno + 1;	/* next available seqno */

	return partname;
}

/*
 *	FetchOraclePartNameToHash
 *		Retrieves the partition names of all partitions and subpartitions of the
 *		Oracle partitioned table and populates them into the desthash, the caller
 *		ensures that the hash table is not NULL.
 */
void
FetchOraclePartNameToHash(Oid rootid, HTAB *desthash)
{
	CatCList   *catlist;
	int			i;

	Assert(desthash);

	catlist = SearchSysCacheList1(ORATABPARTPARTNAME, ObjectIdGetDatum(rootid));

	for (i = 0; i < catlist->n_members; i++)
	{
		HeapTuple	tuple = &catlist->members[i]->tuple;
		Form_pg_ora_tab_partitions tabpartform = (Form_pg_ora_tab_partitions) GETSTRUCT(tuple);

		/* insert part name into hash */
		(void) hash_search(desthash, pstrdup(NameStr(tabpartform->partname)), HASH_ENTER, NULL);
	}

	ReleaseCatCacheList(catlist);
}

/*
 * transformOracleModifyPartitionStmt
 *	  parse analysis for ALTER TABLE
 *
 * Returns a List of utility commands to be done in sequence.
 */
List *
transformOracleModifyPartitionStmt(AlterTableStmt *atstmt)
{
	List		*result = NIL;
	Oid			relid;
	Relation	rel;
	LOCKMODE	lockmode;
	AlterTableCmd	*cmd;

	/*
	 * Ensure this is true in ora_gram.y, if you
	 * modify ora_gram.y, modify this accordingly.
	 */
	Assert(list_length(atstmt->cmds) == 1);
	cmd = (AlterTableCmd *) linitial(atstmt->cmds);

	switch (cmd->subtype)
	{
		case AT_SetTableSpace:
		case AT_AddSubPartitions:
		case AT_DropSubPartitions:
		case AT_RenamePartition:
		case AT_RenameSubPartition:
		case AT_TruncatePartition:
		case AT_TruncateSubPartition:
		case AT_MovePartition:
		case AT_MoveSubPartition:
		case AT_SetPartitionAutomatic:
		case AT_SetSubPartitionTemplate:
		case AT_AddPartitions:
		case AT_DropPartitions:
			lockmode = AccessExclusiveLock;
			break;

		default:
			elog(ERROR, "the MODIFY clause for Oracle partition is not yet supported");
			break;
	}

	rel = relation_openrv(atstmt->relation, lockmode);
	relid = RelationGetRelid(rel);

	/*
	 * ORA-14501: object is not partitioned
	 */
	if (!is_oracle_partitioned_table(relid))
		ereport(ERROR,
				(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
				 errmsg("object is not a oracle partitioned table")));

	switch (cmd->subtype)
	{
		case AT_SetTableSpace:
			{
				if (cmd->partextname)
				{
					Oid		partid;

					/* modify the default tablespace of partition */
					Assert(cmd->partextname->partition || cmd->partextname->forexpr);

					/* ORA-14253: table is not partitioned by composite partition method */
					if (!is_ora_composite_partitioning(relid))
						ereport(ERROR,
								(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
								 errmsg("table is not partitioned by composite partition method")));

					/*
					 * the actual modification is to the partition tablespace,
					 * not the partitioned table. Therefore, we update the
					 * AlterTableStmt->relation field to reflect the partition.
					 */
					if (cmd->partextname->partition)
						partid = ora_get_partid_from_partname(cmd->partextname->partition, relid);
					else
						partid = ResolvePartitionExtenedName(rel, cmd->partextname->forexpr);

					/* ORA-02149: Specified partition does not exist */
					if (!OidIsValid(partid))
						ereport(ERROR,
								(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
								 errmsg("Specified partition does not exist")));

					LockRelationOid(partid, AccessExclusiveLock);
					atstmt->relation = makeRangeVar(get_namespace_name(get_rel_namespace(partid)),
													get_rel_name(partid),
													-1);
					result = lappend(result, atstmt);
				}
				else
				{
					/* modify the default tablespace of partitioned table */
					result = lappend(result, atstmt);
				}
			}
			break;

		case AT_AddPartitions:
			{
				int			seqno;
				int			final_seqno;
				bool		is_composite;
				RangeVar	*parentrv;
				List		*lower_bound = NIL;
				CreateStmt	*lcstmt;
				PartitionDesc	partdesc = RelationGetPartitionDesc(rel, false);
				PartitionKey	key = RelationGetPartitionKey(rel);
				char			strategy = get_partition_strategy(key);
				int				partnatts = get_partition_natts(key);
				HTAB			*part_duplicate_check = NULL;
				HTAB			*part_relname_duplicate_check = NULL;

				switch (strategy)
				{
					case PARTITION_STRATEGY_RANGE:
						{
							HeapTuple	tuple;
							Oid			maxbound;

							/*
							 * ORA-14760:
							 * ADD PARTITION is not permitted on Interval partitioned objects.
							 */
							if (partdesc->intervalinfo)
								ereport(ERROR,
										(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
										 errmsg("ADD PARTITION is not permitted on Interval partitioned objects")));

							/* Oracle partitioned tables must have at least one child */
							if (partdesc->nparts < 1 || !OidIsValid(partdesc->oids[partdesc->nparts - 1]))
								ereport(ERROR,
										(errcode(ERRCODE_INVALID_TABLE_DEFINITION),
										 errmsg("Oracle-compatible partitioned table \"%s\" must have at least one partition or subpartition",
												RelationGetRelationName(rel))));

							maxbound = partdesc->oids[partdesc->nparts - 1];

							/*
							 * Oracle partitioned tables do not support DETACH and ATTACH,
							 * so fetching the tuple directly from the catcache.
							 */
							tuple = SearchSysCache1(RELOID, maxbound);
							if (HeapTupleIsValid(tuple))
							{
								PartitionBoundSpec	*boundspec = NULL;
								Datum		datum;
								bool		isnull;

								datum = SysCacheGetAttr(RELOID, tuple,
														Anum_pg_class_relpartbound,
														&isnull);
								if (!isnull)
								{
									boundspec = stringToNode(TextDatumGetCString(datum));
									Assert(!boundspec->is_default);
									lower_bound = copyObject(boundspec->upperdatums);
								}
								else
									/* Oracle partitioned tables no longer support detach and attach. Has this changed? */
									elog(ERROR, "the partition boundary is null");
								ReleaseSysCache(tuple);
							}
							else
								elog(ERROR, "failed to retrieve the maximum boundary of the range partition");
						}
						break;

					case PARTITION_STRATEGY_LIST:
						{
							/* ORA-14323: cannot add partition when DEFAULT partition exists */
							if (OidIsValid(get_default_partition_oid(relid)))
								ereport(ERROR,
										(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
										 errmsg("cannot add partition when DEFAULT partition exists")));
						}
						break;

					case PARTITION_STRATEGY_HASH:
						ereport(ERROR,
								(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
								 errmsg("this operation is not currently supported for hash partitioning.")));
						break;
				}

				part_duplicate_check = InitPartNameDuplicateCheckHashTable();
				part_relname_duplicate_check = InitPartRelnameDuplicateCheckHashTable();

				FetchOraclePartNameToHash(relid, part_duplicate_check);

				is_composite = is_ora_composite_partitioning(relid);
				seqno = ora_get_partitioned_table_seqno(relid);
				parentrv = makeRangeVar(get_namespace_name(RelationGetNamespace(rel)), RelationGetRelationName(rel), -1);

				if (!is_composite)
				{
					/* simple partition */
					result = derivePostgresPartitionStmts(strategy,			/* strategy */
														  cmd->part_desc,	/* partition desc */
														  parentrv,			/* parent Rangevar */
														  NULL,				/* subpartition key */
														  NULL,				/* subpartition template */
														  NULL,				/* partition name of parent */
														  partnatts,		/* partition nattrs */
														  false,			/* is subpartition? */
														  0,				/* quantity for hash partition */
														  false,			/* is automatic? */
														  false,			/* is interval? */
														  lower_bound,		/* lowerbound of fisrt partition for range */
														  part_duplicate_check,
														  part_relname_duplicate_check,
														  seqno,			/* start seqno*/
														  &final_seqno);	/* end seqno */

					/* final, update seqno */
					lcstmt = (CreateStmt *)llast(result);
					lcstmt->ora_partsup->seqno = final_seqno;

				}
				else
				{
					/* composite partition */
					List		*part_stmt_list = NIL;
					List		*subpart_stmt_list = NIL;
					int			mid_seqno;
					Oid			siblingId;
					char		subpart_strategy;
					int			subpart_partnatts;
					ListCell	*lc;
					ListCell	*lc2;
					PartitionSpec *new_part_spec;
					OraSubPartitionTemplateSpec	*sub_part_template;

					/* Oracle partitioned tables must have at least one child */
					if (partdesc->nparts < 1 || !OidIsValid(partdesc->oids[0]))
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_TABLE_DEFINITION),
								 errmsg("Oracle-compatible partitioned table \"%s\" must have at least one partition or subpartition",
										RelationGetRelationName(rel))));

					siblingId = partdesc->oids[0];

					subpart_strategy = get_partitioning_strategy(siblingId);
					subpart_partnatts = get_partitioning_partnatts(siblingId);

					/* make a dummy PartitionSpec */
					new_part_spec = makeNode(PartitionSpec);
					new_part_spec->strategy = "dummy";
					new_part_spec->partParams = NIL;
					new_part_spec->sibling = siblingId;
					new_part_spec->location = -1;

					/* 1-level partition */
					part_stmt_list = derivePostgresPartitionStmts(strategy,	/* strategy */
														  cmd->part_desc,	/* partition desc */
														  parentrv,			/* parent Rangevar */
														  new_part_spec,	/* subpartition key */
														  NULL,				/* subpartition template */
														  NULL,				/* partition name of parent */
														  partnatts,		/* partition nattrs */
														  false,			/* is subpartition? */
														  0,				/* quantity for hash partition */
														  false,			/* is automatic? */
														  false,			/* is interval? */
														  lower_bound,		/* lowerbound of fisrt partition for range */
														  part_duplicate_check,
														  part_relname_duplicate_check,
														  seqno,			/* start seqno*/
														  &mid_seqno);		/* end seqno */

					result = list_concat(result, part_stmt_list);

					/* fetch subpartition template if any */
					sub_part_template = ora_get_subpartition_template(relid);

					if (list_length(cmd->part_desc) != list_length(part_stmt_list))
						elog(ERROR, "Partition internal error");	/* shouldn't happen */

					forboth(lc, cmd->part_desc, lc2, part_stmt_list)
					{
						OraPartitionBoundSpec	*part = (OraPartitionBoundSpec *) lfirst(lc);
						CreateStmt	*stmt = (CreateStmt *) lfirst(lc2);

						checkOracleSubpartitionDesc(part->subpart_bound,
													subpart_strategy,
													subpart_partnatts,
													false);

						/* 2-level subpartition */
						subpart_stmt_list = derivePostgresPartitionStmts(subpart_strategy,	/* strategy */
															 part->subpart_bound ? part->subpart_bound->subpartitons_desc : NULL, /* partition desc */
															 stmt->relation,				/* parent Rangevar */
															 NULL,							/* subpartition key */
															 sub_part_template,				/* subpartition template */
															 stmt->ora_partsup->partname,	/* partition name of parent */
															 subpart_partnatts,				/* partition nattrs */
															 true,							/* is subpartition? */
															 part->subpart_bound ? part->subpart_bound->hash_subpartition_quantity : 0,	/* quantity for hash partition */
															 false,							/* is automatic? */
															 false,							/* is interval? */
															 NIL,							/* lowerbound of fisrt partition for range */
															 part_duplicate_check,
															 part_relname_duplicate_check,
															 mid_seqno,						/* start seqno*/
															 &final_seqno);					/* end seqno */

						result = list_concat(result, subpart_stmt_list);
						mid_seqno = final_seqno;
					}

					/* final, update seqno */
					lcstmt = (CreateStmt *)llast(result);
					lcstmt->ora_partsup->seqno = final_seqno;
				}

				/* clean up */
				hash_destroy(part_duplicate_check);
				hash_destroy(part_relname_duplicate_check);
			}
			break;

		case AT_DropPartitions:
			{
				List	*do_del_relids;
				List	*children;
				int		del_relids_num;
				int		children_num;
				char	strategy;

				/*
				 * ORA-14255: table is not partitioned by range, list,
				 * composite range, or composite list method.
				 */
				strategy = get_partitioning_strategy(relid);
				if (strategy == PARTITION_STRATEGY_HASH)
					ereport(ERROR,
							(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
							 errmsg("table is not partitioned by range, list, composite range, or composite list method")));

				do_del_relids = transform_partition_extended_names(rel, cmd->partitions, true);
				del_relids_num = list_length(do_del_relids);

				children = find_inheritance_children(relid, NoLock);
				children_num = list_length(children);

				/* ORA-14083: cannot drop the only partition of a partitioned table */
				if (children_num == 1)
					ereport(ERROR,
							(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
							 errmsg("cannot drop the only partition of a partitioned table")));

				if (del_relids_num > children_num || children_num < 1)
					elog(ERROR, "Oracle partition table hierarchy is corrupted"); /* shouldn't happen */
				else if (del_relids_num == children_num)
					/* ORA-14331: cannot drop all [sub]partitions of a partitioned table */
					ereport(ERROR,
							(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
							 errmsg("cannot drop all [sub]partitions of a partitioned table")));
				else
				{
					DropStmt	*dstmt = makeNode(DropStmt);
					List		*objects = NIL;
					ListCell	*lc;

					/* make DropStmt */
					dstmt->removeType = OBJECT_TABLE;
					dstmt->missing_ok = false;
					dstmt->behavior = DROP_RESTRICT;
					dstmt->concurrent = false;
					dstmt->nocheckpart = true;

					foreach(lc, do_del_relids)
					{
						Oid			drelid = lfirst_oid(lc);
						List		*ditem;

						/* lock each dropping relation */
						LockRelationOid(drelid, AccessExclusiveLock);
						ditem = list_make2(makeString(get_namespace_name(get_rel_namespace(drelid))),
											makeString(get_rel_name(drelid)));

						objects = lappend(objects, ditem);
					}

					dstmt->objects = objects;

					result = lappend(result, dstmt);
				}
			}
			break;

		case AT_AddSubPartitions:
			{
				Oid				partid;
				Relation		partrel;
				RangeVar		*partrv;
				PartitionDesc	partdesc;
				PartitionKey	key;
				char			strategy;
				int				partnatts;
				int				seqno;
				int				final_seqno;
				CreateStmt		*lcstmt;
				List			*lower_bound = NIL;
				HTAB			*part_duplicate_check = NULL;
				HTAB			*part_relname_duplicate_check = NULL;
				OraSubPartitionTemplateSpec	*subdesc;

				Assert(cmd->part_desc);

				/* ORA-14253: table is not partitioned by composite partition method */
				if (!is_ora_composite_partitioning(relid))
					ereport(ERROR,
							(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
							 errmsg("table is not partitioned by composite partition method")));

				if (cmd->partextname->partition)
					partid = ora_get_partid_from_partname(cmd->partextname->partition, relid);
				else
					partid = ResolvePartitionExtenedName(rel, cmd->partextname->forexpr);

				/* ORA-02149: Specified partition does not exist */
				if (!OidIsValid(partid))
					ereport(ERROR,
							(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
							 errmsg("Specified partition does not exist")));

				partrv = makeRangeVar(get_namespace_name(get_rel_namespace(partid)),
										get_rel_name(partid),
										-1);

				partrel = table_open(partid, AccessExclusiveLock);

				partdesc = RelationGetPartitionDesc(partrel, false);
				key = RelationGetPartitionKey(partrel);
				strategy = get_partition_strategy(key);
				partnatts = get_partition_natts(key);

				switch (strategy)
				{
					case PARTITION_STRATEGY_RANGE:
						{
							HeapTuple	tuple;
							Oid			maxbound;

							maxbound = partdesc->oids[partdesc->nparts - 1];

							/*
							 * Oracle partitioned tables do not support DETACH and ATTACH,
							 * so fetching the tuple directly from the catcache.
							 */
							tuple = SearchSysCache1(RELOID, maxbound);
							if (HeapTupleIsValid(tuple))
							{
								PartitionBoundSpec	*boundspec = NULL;
								Datum		datum;
								bool		isnull;

								datum = SysCacheGetAttr(RELOID, tuple,
														Anum_pg_class_relpartbound,
														&isnull);
								if (!isnull)
								{
									boundspec = stringToNode(TextDatumGetCString(datum));
									Assert(!boundspec->is_default);
									lower_bound = copyObject(boundspec->upperdatums);
								}
								else
									/* Oracle partitioned tables no longer support detach and attach. has this changed? */
									elog(ERROR, "the subpartition boundary is null");
								ReleaseSysCache(tuple);
							}
							else
								elog(ERROR, "failed to retrieve the maximum boundary of the range subpartition");
						}
						break;

					case PARTITION_STRATEGY_LIST:
						{
							/* ORA-14621:: cannot add subpartition when DEFAULT subpartition exists */
							if (OidIsValid(get_default_partition_oid(partid)))
								ereport(ERROR,
										(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
										 errmsg("cannot add subpartition when DEFAULT subpartition exists")));
						}
						break;

					case PARTITION_STRATEGY_HASH:
						ereport(ERROR,
								(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
								 errmsg("this operation is not currently supported for hash partitioning.")));
						break;
				}

				relation_close(partrel, NoLock);

				part_duplicate_check = InitPartNameDuplicateCheckHashTable();
				part_relname_duplicate_check = InitPartRelnameDuplicateCheckHashTable();

				FetchOraclePartNameToHash(relid, part_duplicate_check);
				seqno = ora_get_partitioned_table_seqno(relid);

				subdesc = makeNode(OraSubPartitionTemplateSpec);
				subdesc->subpartitons_desc = cmd->part_desc;

				checkOracleSubpartitionDesc(subdesc, strategy, partnatts, false);

				/* derive subpartition stmts */
				result = derivePostgresPartitionStmts(strategy,						/* strategy */
													 cmd->part_desc,				/* partition desc */
													 partrv,						/* parent Rangevar */
													 NULL,							/* subpartition key */
													 NULL,							/* subpartition template */
													 NULL,							/* partition name of parent */
													 partnatts,						/* partition nattrs */
													 true,							/* is subpartition? */
													 0,								/* quantity for hash partition */
													 false,							/* is automatic? */
													 false,							/* is interval? */
													 lower_bound,					/* lowerbound of fisrt partition for range */
													 part_duplicate_check,			/* htab for partition name check */
													 part_relname_duplicate_check,	/* htab for partition relname check */
													 seqno,							/* start seqno*/
													 &final_seqno);					/* end seqno */

				/* update seqno */
				lcstmt = (CreateStmt *)llast(result);
				lcstmt->ora_partsup->seqno = final_seqno;

				/* clean up */
				hash_destroy(part_duplicate_check);
				hash_destroy(part_relname_duplicate_check);
			}
			break;

		case AT_DropSubPartitions:
			{
				List	*do_del_relids;
				List	*children;
				List	*subpparentlist = NIL;
				Oid		subpparentid;
				int		del_relids_num;
				int		children_num;

				/* ORA-14253: table is not partitioned by composite partition method */
				if (!is_ora_composite_partitioning(relid))
					ereport(ERROR,
							(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
							 errmsg("table is not partitioned by composite partition method")));
				else
				{
					char			subpart_strategy;
					PartitionDesc	pdesc = RelationGetPartitionDesc(rel, false);

					/* Oracle partitioned tables must have at least one child */
					if (pdesc->nparts < 1 || !OidIsValid(pdesc->oids[0]))
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_TABLE_DEFINITION),
								 errmsg("Oracle-compatible partitioned table \"%s\" must have at least one partition or subpartition",
										RelationGetRelationName(rel))));

					subpart_strategy = get_partitioning_strategy(pdesc->oids[0]);

					/* ORA-14206: table is not subpartitioned by List or Range methods */
					if (subpart_strategy == PARTITION_STRATEGY_HASH)
						ereport(ERROR,
								(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
								 errmsg("table is not subpartitioned by List or Range methods")));
				}

				do_del_relids = transform_subpartition_extended_names(rel, cmd->partitions, &subpparentlist, true);

				/* ORA-14333: subpartition names specified are across multiple partitions */
				if (list_length(subpparentlist) != 1)
					ereport(ERROR,
							(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
							 errmsg("subpartition names specified are across multiple partitions")));

				subpparentid = linitial_oid(subpparentlist);
				LockRelationOid(subpparentid, AccessExclusiveLock);

				del_relids_num = list_length(do_del_relids);

				children = find_inheritance_children(subpparentid, NoLock);
				children_num = list_length(children);

				/* ORA-14629: cannot drop the only subpartition of a partition */
				if (del_relids_num >= children_num)
					ereport(ERROR,
							(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
							 errmsg("cannot drop the only subpartition of a partition")));
				else
				{
					DropStmt	*dstmt = makeNode(DropStmt);
					List		*objects = NIL;
					ListCell	*lc;

					/* make DropStmt */
					dstmt->removeType = OBJECT_TABLE;
					dstmt->missing_ok = false;
					dstmt->behavior = DROP_RESTRICT;
					dstmt->concurrent = false;
					dstmt->nocheckpart = true;

					foreach(lc, do_del_relids)
					{
						Oid			drelid = lfirst_oid(lc);
						List		*ditem;

						/* lock each dropping relation */
						LockRelationOid(drelid, AccessExclusiveLock);
						ditem = list_make2(makeString(get_namespace_name(get_rel_namespace(drelid))),
											makeString(get_rel_name(drelid)));

						objects = lappend(objects, ditem);
					}

					dstmt->objects = objects;

					result = lappend(result, dstmt);
				}
			}
			break;

		case AT_RenamePartition:
			{
				Oid		partid;
				HTAB	*part_duplicate_check = NULL;

				Assert(cmd->partextname);
				if (cmd->partextname->partition)
					partid = ora_get_partid_from_partname(cmd->partextname->partition, relid);
				else
					partid = ResolvePartitionExtenedName(rel, cmd->partextname->forexpr);

				/* ORA-02149: Specified partition does not exist */
				if (!OidIsValid(partid))
					ereport(ERROR,
							(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
							 errmsg("Specified partition does not exist")));

				part_duplicate_check = InitPartNameDuplicateCheckHashTable();
				FetchOraclePartNameToHash(relid, part_duplicate_check);

				/*
				 * ORA-14082: New partition name must differ from that
				 * of any other partition or subpartition of the object.
				 */
				if (hash_search(part_duplicate_check,
								cmd->name,
								HASH_FIND,
								NULL))
					ereport(ERROR,
							(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
							 errmsg("New partition name must differ from that of "
									"any other partition or subpartition of the object")));

				/* everything is ok, lock it and update cmd->part_oid */
				LockRelationOid(partid, AccessExclusiveLock);
				cmd->part_oid = partid;
				result = lappend(result, atstmt);
				hash_destroy(part_duplicate_check);
			}
			break;

		case AT_RenameSubPartition:
			{
				Oid		subpartid;
				Oid		parentoid;
				HTAB	*part_duplicate_check = NULL;

				Assert(cmd->partextname);

				/* ORA-14253: table is not partitioned by composite partition method */
				if (!is_ora_composite_partitioning(relid))
					ereport(ERROR,
							(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
							 errmsg("table is not partitioned by composite partition method")));

				if (cmd->partextname->partition)
				{
					subpartid = ora_get_subpartid_from_partname(cmd->partextname->partition, relid);

					/* ORA-14251: Specified subpartition does not exist */
					if (!OidIsValid(subpartid))
						ereport(ERROR,
								(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
								 errmsg("Specified subpartition does not exist")));

					parentoid = ora_get_subpartition_parent(subpartid);
					Assert(OidIsValid(parentoid));
					LockRelationOid(parentoid, AccessShareLock);
				}
				else
				{
					int		pnattrs;
					int		subpatrs;
					List	*plist = NIL;
					List	*subplist = NIL;
					Relation	parent_rel;
					PartitionDesc	pdesc = RelationGetPartitionDesc(rel, false);

					/* Oracle partitioned tables must have at least one child */
					if (pdesc->nparts < 1 || !OidIsValid(pdesc->oids[0]))
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_TABLE_DEFINITION),
								 errmsg("Oracle-compatible partitioned table \"%s\" must have at least one partition or subpartition",
										RelationGetRelationName(rel))));

					pnattrs = get_partitioning_partnatts(RelationGetRelid(rel));
					subpatrs = get_partitioning_partnatts(pdesc->oids[0]);

					/* ORA-14092: number of expressions is not equal to the number of partitioning columns */
					if ((pnattrs + subpatrs) != list_length(cmd->partextname->forexpr))
						ereport(ERROR,
								(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
								 errmsg("number of expressions is not equal to the number of partitioning columns")));

					plist = list_copy_head(cmd->partextname->forexpr, pnattrs);
					subplist = list_copy_tail(cmd->partextname->forexpr, pnattrs);

					parentoid = ResolvePartitionExtenedName(rel, plist);
					Assert(OidIsValid(parentoid));

					parent_rel = table_open(parentoid, AccessShareLock);
					subpartid = ResolvePartitionExtenedName(parent_rel, subplist);
					table_close(parent_rel, NoLock);

					/* ORA-14251: Specified subpartition does not exist */
					if (!OidIsValid(subpartid))
						ereport(ERROR,
								(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
								 errmsg("Specified subpartition does not exist")));
				}

				part_duplicate_check = InitPartNameDuplicateCheckHashTable();
				FetchOraclePartNameToHash(relid, part_duplicate_check);

				/*
				 * ORA-14263: New subpartition name must differ from that
				 * of any other partition or subpartition of the object.
				 */
				if (hash_search(part_duplicate_check,
								cmd->name,
								HASH_FIND,
								NULL))
					ereport(ERROR,
							(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
							 errmsg("New subpartition name must differ from that of "
									"any other partition or subpartition of the object")));

				/* everything is ok, lock it and update cmd->part_oid */
				LockRelationOid(subpartid, AccessExclusiveLock);
				cmd->part_oid = subpartid;
				result = lappend(result, atstmt);
				hash_destroy(part_duplicate_check);
			}
			break;

		case AT_TruncatePartition:
			{
				List			*truncate_relids;
				TruncateStmt	*tstmt = makeNode(TruncateStmt);
				List			*relations = NIL;
				ListCell		*lc;

				truncate_relids = transform_partition_extended_names(rel, cmd->partitions, false);

				/* make TruncateStmt */
				tstmt->restart_seqs = false;
				tstmt->behavior = DROP_RESTRICT;

				foreach(lc, truncate_relids)
				{
					Oid			truncate_relid = lfirst_oid(lc);
					RangeVar	*truncate_rv;

					/* lock each truncate relation */
					LockRelationOid(truncate_relid, AccessExclusiveLock);
					truncate_rv = makeRangeVar(get_namespace_name(get_rel_namespace(truncate_relid)),
												get_rel_name(truncate_relid),
												-1);

					truncate_rv->inh = true;
					truncate_rv->alias = NULL;
					relations = lappend(relations, truncate_rv);
				}

				tstmt->relations = relations;

				result = lappend(result, tstmt);
			}
			break;

		case AT_TruncateSubPartition:
			{
				List			*truncate_relids;
				TruncateStmt	*tstmt = makeNode(TruncateStmt);
				List			*relations = NIL;
				ListCell		*lc;

				/* ORA-14253: table is not partitioned by composite partition method */
				if (!is_ora_composite_partitioning(relid))
					ereport(ERROR,
							(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
							 errmsg("table is not partitioned by composite partition method")));

				truncate_relids = transform_subpartition_extended_names(rel, cmd->partitions, NULL, false);

				/* make TruncateStmt */
				tstmt->restart_seqs = false;
				tstmt->behavior = DROP_RESTRICT;

				foreach(lc, truncate_relids)
				{
					Oid			truncate_relid = lfirst_oid(lc);
					RangeVar	*truncate_rv;

					/* lock each truncate relation */
					LockRelationOid(truncate_relid, AccessExclusiveLock);
					truncate_rv = makeRangeVar(get_namespace_name(get_rel_namespace(truncate_relid)),
												get_rel_name(truncate_relid),
												-1);

					truncate_rv->inh = true;
					truncate_rv->alias = NULL;
					relations = lappend(relations, truncate_rv);
				}

				tstmt->relations = relations;

				result = lappend(result, tstmt);
			}
			break;

		case AT_MovePartition:
			{
				Oid		partid;

				Assert(cmd->partextname);

				/* ORA-14257: cannot move a partition which is a composite partition */
				if (is_ora_composite_partitioning(relid))
					ereport(ERROR,
							(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
							 errmsg("cannot move a partition which is a composite partition")));

				if (cmd->partextname->partition)
					partid = ora_get_partid_from_partname(cmd->partextname->partition, relid);
				else
					partid = ResolvePartitionExtenedName(rel, cmd->partextname->forexpr);

				/* ORA-02149: Specified partition does not exist */
				if (!OidIsValid(partid))
					ereport(ERROR,
							(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
							 errmsg("Specified partition does not exist")));

				/* must be leaf node has storage */
				if (get_rel_relkind(partid) == RELKIND_PARTITIONED_TABLE)
					ereport(ERROR,
							(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
							 errmsg("partition internal error, the oracle partition tree was corrupted")));

				LockRelationOid(partid, AccessExclusiveLock);
				cmd->subtype = AT_SetTableSpace;
				atstmt->relation = makeRangeVar(get_namespace_name(get_rel_namespace(partid)),
												get_rel_name(partid),
												-1);

				result = lappend(result, atstmt);
			}
			break;

		case AT_MoveSubPartition:
			{
				Oid		subpartid;
				Oid		parentoid;

				Assert(cmd->partextname);

				/* ORA-14253: table is not partitioned by composite partition method */
				if (!is_ora_composite_partitioning(relid))
					ereport(ERROR,
							(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
							 errmsg("table is not partitioned by composite partition method")));

				if (cmd->partextname->partition)
				{
					subpartid = ora_get_subpartid_from_partname(cmd->partextname->partition, relid);

					/* ORA-14251: Specified subpartition does not exist */
					if (!OidIsValid(subpartid))
						ereport(ERROR,
								(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
								 errmsg("Specified subpartition does not exist")));

					parentoid = ora_get_subpartition_parent(subpartid);
					Assert(OidIsValid(parentoid));
					LockRelationOid(parentoid, AccessShareLock);
				}
				else
				{
					int		pnattrs;
					int		subpatrs;
					List	*plist = NIL;
					List	*subplist = NIL;
					Relation	parent_rel;
					PartitionDesc	pdesc = RelationGetPartitionDesc(rel, false);

					/* Oracle partitioned tables must have at least one child */
					if (pdesc->nparts < 1 || !OidIsValid(pdesc->oids[0]))
						ereport(ERROR,
								(errcode(ERRCODE_INVALID_TABLE_DEFINITION),
								 errmsg("Oracle-compatible partitioned table \"%s\" must have at least one partition or subpartition",
										RelationGetRelationName(rel))));

					pnattrs = get_partitioning_partnatts(RelationGetRelid(rel));
					subpatrs = get_partitioning_partnatts(pdesc->oids[0]);

					/* ORA-14092: number of expressions is not equal to the number of partitioning columns */
					if ((pnattrs + subpatrs) != list_length(cmd->partextname->forexpr))
						ereport(ERROR,
								(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
								 errmsg("number of expressions is not equal to the number of partitioning columns")));

					plist = list_copy_head(cmd->partextname->forexpr, pnattrs);
					subplist = list_copy_tail(cmd->partextname->forexpr, pnattrs);

					parentoid = ResolvePartitionExtenedName(rel, plist);
					Assert(OidIsValid(parentoid));

					parent_rel = table_open(parentoid, AccessShareLock);
					subpartid = ResolvePartitionExtenedName(parent_rel, subplist);
					table_close(parent_rel, NoLock);

					/* ORA-14251: Specified subpartition does not exist */
					if (!OidIsValid(subpartid))
						ereport(ERROR,
								(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
								 errmsg("Specified subpartition does not exist")));
				}

				/* must be leaf node has storage */
				if (get_rel_relkind(subpartid) == RELKIND_PARTITIONED_TABLE)
					ereport(ERROR,
							(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
							 errmsg("partition internal error, the oracle partition tree was corrupted")));

				LockRelationOid(subpartid, AccessExclusiveLock);
				cmd->subtype = AT_SetTableSpace;
				atstmt->relation = makeRangeVar(get_namespace_name(get_rel_namespace(subpartid)),
												get_rel_name(subpartid),
												-1);

				result = lappend(result, atstmt);
			}
			break;

		/* No special transform is needed */
		case AT_SetPartitionAutomatic:
		case AT_SetSubPartitionTemplate:
			result = lappend(result, atstmt);
			break;

		default:

			/*
			 * Currently, we shouldn't actually get here for subcommand
			 * types that don't require transformation; but if we do, just
			 * emit them unchanged.
			 */
			elog(ERROR, "the MODIFY clause for Oracle partition is not yet supported");
			break;
	}

	/* hold lock until commit avoid concurrent DDL drop of root table */
	relation_close(rel, NoLock);

	return result;
}

/*
 *  transformColumnDefinition
 *		transform a List of OraPartExtentedName and returns
 *		a list of OIDs for partition relation.
 *
 * rel is the parent of the partition to be transformed.
 * pnames is the partition extended name list to be transformed.
 * check_duplicate indicates whether to check for duplicate partition names,
 * if it is true and a duplicate partition name exists, an error is thrown.
 */
static List *
transform_partition_extended_names(Relation rel, List *pnames, bool check_duplicate)
{
	List		*result = NIL;
	ListCell	*lc;
	bool		is_name;
	HTAB		*htab = NULL;

	if (check_duplicate)
	{
		HASHCTL		hash_ctl;

		hash_ctl.keysize = sizeof(Oid);
		hash_ctl.entrysize = sizeof(Oid);
		hash_ctl.hcxt = CurrentMemoryContext;
		htab = hash_create("partition extended name duplicate check",
						32,
						&hash_ctl,
						HASH_ELEM | HASH_BLOBS | HASH_CONTEXT);
	}

	if ((linitial_node(OraPartExtentedName, pnames))->partition)
		is_name = true;
	else
		is_name = false;

	foreach(lc, pnames)
	{
		Oid		partid;
		bool	found;
		OraPartExtentedName	*partextname = (OraPartExtentedName *) lfirst(lc);

		/* ORA-14764: Partitions cannot be truncated, dropped or merged with mixed syntax */
		if((partextname->partition && !is_name) ||
			(partextname->forexpr && is_name))
			ereport(ERROR,
					(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
					 errmsg("Partitions cannot be truncated, dropped or merged with mixed syntax")));

		if (partextname->partition)
			partid = ora_get_partid_from_partname(partextname->partition, RelationGetRelid(rel));
		else
			partid = ResolvePartitionExtenedName(rel, partextname->forexpr);

		/* ORA-02149: Specified partition does not exist */
		if (!OidIsValid(partid))
			ereport(ERROR,
					(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
					 errmsg("Specified partition does not exist")));

		if (check_duplicate && htab)
		{
			/* ORA-14332: duplicate partition or subpartition names specified */
			(void) hash_search(htab, &partid, HASH_ENTER, &found);
			if (found)
				ereport(ERROR,
						(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
						 errmsg("duplicate partition or subpartition names specified")));
		}

		result = list_append_unique_oid(result, partid);
	}

	if (check_duplicate)
		hash_destroy(htab);

	return result;
}

/*
 *  transform_subpartition_extended_names
 *		transform a List of OraPartExtentedName and returns
 *		a list of OIDs for subpartition relation.
 *
 * rel is the root relation of the subpartition to be transformed.
 * subpnames is the subpartition extended name list to be transformed.
 * parentlist returns a list of parent OIDs for subpnames.
 * check_duplicate indicates whether to check for duplicate subpartition names,
 * if it is true and a duplicate subpartition name exists, an error is thrown.
 */
static List *
transform_subpartition_extended_names(Relation rel, List *subpnames, List **parentlist, bool check_duplicate)
{
	List		*result = NIL;
	ListCell	*lc;
	bool		is_name;
	HTAB		*htab = NULL;
	int			part_kyes = 0;
	int			subpart_kyes = 0;

	if (check_duplicate)
	{
		HASHCTL	hash_ctl;

		hash_ctl.keysize = sizeof(Oid);
		hash_ctl.entrysize = sizeof(Oid);
		hash_ctl.hcxt = CurrentMemoryContext;
		htab = hash_create("subpartition extended name duplicate check",
						32,
						&hash_ctl,
						HASH_ELEM | HASH_BLOBS | HASH_CONTEXT);
	}

	if ((linitial_node(OraPartExtentedName, subpnames))->partition)
		is_name = true;
	else
		is_name = false;

	/* if a FOR clause exists, get the total number of partition keys */
	if (!is_name)
	{
		PartitionDesc	pdesc = RelationGetPartitionDesc(rel, false);

		/* Oracle partitioned tables must have at least one child */
		if (pdesc->nparts < 1 || !OidIsValid(pdesc->oids[0]))
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_TABLE_DEFINITION),
					 errmsg("Oracle-compatible partitioned table \"%s\" must have at least one partition or subpartition",
							RelationGetRelationName(rel))));

		part_kyes = get_partitioning_partnatts(RelationGetRelid(rel));
		subpart_kyes = get_partitioning_partnatts(pdesc->oids[0]);
	}

	foreach(lc, subpnames)
	{
		Oid		subpartid;
		Oid		subpartparentid;
		bool	found;
		OraPartExtentedName	*partextname = (OraPartExtentedName *) lfirst(lc);

		/* ORA-14764: Partitions cannot be truncated, dropped or merged with mixed syntax */
		if((partextname->partition && !is_name) ||
			(partextname->forexpr && is_name))
			ereport(ERROR,
					(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
					 errmsg("Partitions cannot be truncated, dropped or merged with mixed syntax")));

		if (partextname->partition)
		{
			subpartid = ora_get_subpartid_from_partname(partextname->partition, RelationGetRelid(rel));

			/* ORA-14251:: Specified subpartition does not exist */
			if (!OidIsValid(subpartid))
				ereport(ERROR,
						(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
						 errmsg("Specified subpartition does not exist")));

			subpartparentid = ora_get_subpartition_parent(subpartid);
			Assert(OidIsValid(subpartparentid));

			if (parentlist)
				*parentlist = list_append_unique_oid(*parentlist, subpartparentid);
		}
		else
		{
			List		*plist = NIL;
			List		*subplist = NIL;
			Relation	subpart_parent_rel;

			/* ORA-14092: number of expressions is not equal to the number of partitioning columns */
			if ((part_kyes + subpart_kyes) != list_length(partextname->forexpr))
				ereport(ERROR,
						(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
						 errmsg("number of expressions is not equal to the number of partitioning columns")));

			plist = list_copy_head(partextname->forexpr, part_kyes);
			subplist = list_copy_tail(partextname->forexpr, part_kyes);

			subpartparentid = ResolvePartitionExtenedName(rel, plist);
			Assert(OidIsValid(subpartparentid));

			if (parentlist)
				*parentlist = list_append_unique_oid(*parentlist, subpartparentid);

			subpart_parent_rel = table_open(subpartparentid, AccessShareLock);
			subpartid = ResolvePartitionExtenedName(subpart_parent_rel, subplist);
			table_close(subpart_parent_rel, AccessShareLock);

			/* ORA-14251: Specified subpartition does not exist */
			if (!OidIsValid(subpartid))
				ereport(ERROR,
						(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
						 errmsg("Specified subpartition does not exist")));
		}

		if (check_duplicate && htab)
		{
			/* ORA-14332: duplicate partition or subpartition names specified */
			(void) hash_search(htab, &subpartid, HASH_ENTER, &found);

			if (found)
				ereport(ERROR,
						(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
						 errmsg("duplicate partition or subpartition names specified")));
		}

		result = list_append_unique_oid(result, subpartid);
	}

	if (check_duplicate)
		hash_destroy(htab);

	return result;
}

/*
 *  InitPartNameDuplicateCheckHashTable
 *		Initialize (sub)partition name duplicate check hash table.
 */
static HTAB *
InitPartNameDuplicateCheckHashTable(void)
{
	HASHCTL		hash_ctl;

	hash_ctl.keysize = NAMEDATALEN;
	hash_ctl.entrysize = NAMEDATALEN;
	return hash_create("partition name check", 32, &hash_ctl,
						HASH_ELEM | HASH_STRINGS);
}

/*
 *  InitPartRelnameDuplicateCheckHashTable
 *		Initialize (sub)partition relation name duplicate check hash table.
 */
static HTAB *
InitPartRelnameDuplicateCheckHashTable(void)
{
	HASHCTL		hash_ctl;

	hash_ctl.keysize = NAMEDATALEN;
	hash_ctl.entrysize = NAMEDATALEN;
	return hash_create("partition relation name check", 32,
						&hash_ctl, HASH_ELEM | HASH_STRINGS);
}

/*
 *  make_part_relname_unique
 *		Ensure that the relation name of (sub)partition is unique.
 */
static char *
make_part_relname_unique(char *part_relname, Oid namespaceid, HTAB *partrelhash)
{
	int		pass = 0;
	char	*basename = pstrdup(part_relname);
	char	*relname = part_relname;
	char	*hash_entry;

	for (;;)
	{
		char	*tmp;

		hash_entry = hash_search(partrelhash, relname,
								 HASH_FIND, NULL);

		/* everything is ok */
		if (!OidIsValid(get_relname_relid(relname, namespaceid)) &&
			hash_entry == NULL)
			break;

		/*
		 * relation already exists or the name is already used
		 * by another partition or subpartition.
		 */
		tmp = psprintf("%s_%d", basename, ++pass);
		pfree(relname);
		relname = tmp;
	}

	if (pass < 0 || strlen(relname) >= NAMEDATALEN)
		elog(ERROR, "partition/subpartition relation name is too long, please adjust it");

	pfree(basename);

	/* insert */
	(void) hash_search(partrelhash, relname, HASH_ENTER, NULL);

	return relname;
}

/*
 *  ResolvePartitionExtenedName
 *		Parsing partition name in FOR clause form.
 */
Oid
ResolvePartitionExtenedName(Relation rel, List *forexpr)
{
	ListCell		*lc;
	PartitionKey	key = RelationGetPartitionKey(rel);
	PartitionDesc	partdesc = RelationGetPartitionDesc(rel, true);
	Datum			values[PARTITION_MAX_KEYS];
	bool			isnull[PARTITION_MAX_KEYS];
	int				i = 0;
	int				part_index = -1;
	int				bound_offset;
	PartitionBoundInfo	boundinfo = partdesc->boundinfo;

	Assert(forexpr);

	/*
	 * ORA-14092:
	 * number of expressions is not equal to the number of partitioning columns.
	 */
	if (key->partnatts != list_length(forexpr))
		ereport(ERROR,
				(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
				 errmsg("number of expressions is not equal to the number of partitioning columns")));

	foreach(lc, forexpr)
	{
		Node	*expr = lfirst(lc);
		Node	*value;
		Oid		coltype;
		int32	coltypmod;
		Oid		partcollation;
		ParseState	*pstate;
		ParseNamespaceItem	*nsitem;

		coltype = get_partition_col_typid(key, i);
		coltypmod = get_partition_col_typmod(key, i);
		partcollation = get_partition_col_collation(key, i);

		pstate = make_parsestate(NULL);
		nsitem = addRangeTableEntryForRelation(pstate, rel, AccessShareLock, NULL, false, false);
		addNSItemToQuery(pstate, nsitem, false, true, true);

		value = transformExpr(pstate, expr, EXPR_KIND_PARTITION_BOUND);

		/*
		 * ORA-14093:
		 * data type of expression incompatible with that of partitioning column.
		 */
		value = coerce_to_target_type(pstate,
									  value, exprType(value),
									  coltype,
									  coltypmod,
									  COERCION_ASSIGNMENT,
									  COERCE_IMPLICIT_CAST,
									  -1);

		if (value == NULL)
			ereport(ERROR,
					(errcode(ERRCODE_DATATYPE_MISMATCH),
					 errmsg("data type of expression incompatible with that of partitioning column")));

		if (!IsA(value, Const))
		{
			assign_expr_collations(pstate, value);
			value = (Node *) expression_planner((Expr *) value);
			value = (Node *) evaluate_expr((Expr *) value, coltype, coltypmod, partcollation);

			/* ORA-14755: Invalid partition specification for FOR VALUES clause. */
			if (!IsA(value, Const))
				ereport(ERROR,
						(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
						 errmsg("Invalid partition specification for FOR VALUES clause")));
		}
		else
			((Const *) value)->constcollid = partcollation;
		((Const *) value)->location = exprLocation(expr);

		if (((Const *) value)->constisnull)
		{
			values[i] = (Datum) 0;
			isnull[i] = true;
		}
		else
		{
			values[i] = ((Const *) value)->constvalue;
			isnull[i] = false;
		}

		i++;
	}

	switch (key->strategy)
	{
		case PARTITION_STRATEGY_HASH:
			{
				uint64		rowHash;

				rowHash = compute_partition_hash_value(key->partnatts,
													   key->partsupfunc,
													   key->partcollation,
													   values, isnull);

				part_index = boundinfo->indexes[rowHash % boundinfo->nindexes];
			}
			break;

		case PARTITION_STRATEGY_LIST:
			if (isnull[0])
			{
				if (partition_bound_accepts_nulls(boundinfo))
					part_index = boundinfo->null_index;
			}
			else
			{
				bool		equal = false;

				bound_offset = partition_list_bsearch(key->partsupfunc,
													  key->partcollation,
													  boundinfo,
													  values[0], &equal);
				if (bound_offset >= 0 && equal)
					part_index = boundinfo->indexes[bound_offset];
			}
			break;

		case PARTITION_STRATEGY_RANGE:
			{
				bool		equal = false,
							range_partkey_has_null = false;
				int			i;

				/*
				 * No range includes NULL, so this will be accepted by the
				 * default partition if there is one, and otherwise rejected.
				 */
				for (i = 0; i < key->partnatts; i++)
				{
					if (isnull[i])
					{
						range_partkey_has_null = true;
						break;
					}
				}

				if (!range_partkey_has_null)
				{
					bound_offset = partition_range_datum_bsearch(key->partsupfunc,
																 key->partcollation,
																 boundinfo,
																 key->partnatts,
																 values,
																 &equal);

					/*
					 * The bound at bound_offset is less than or equal to the
					 * tuple value, so the bound at offset+1 is the upper
					 * bound of the partition we're looking for, if there
					 * actually exists one.
					 */
					part_index = boundinfo->indexes[bound_offset + 1];
				}
				else
				{
					bound_offset = partition_range_datum_bsearch_nullable(key->partsupfunc,
																		  key->partcollation,
																		  boundinfo,
																		  key->partnatts,
																		  values,
																		  isnull,
																		  &equal);

					part_index = boundinfo->indexes[bound_offset + 1];
				}
			}
			break;

		default:
			elog(ERROR, "unexpected partition strategy: %d",
				 (int) key->strategy);
	}

	/*
	 * part_index < 0 means we failed to find a partition of this parent. Use
	 * the default partition, if there is one.
	 */
	if (part_index < 0)
		part_index = boundinfo->default_index;

	/*
	 * ORA-14702:
	 * the partition number is invalid or out-of-range.
	 */
	if (part_index < 0)
		ereport(ERROR,
				(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
				 errmsg("the partition number is invalid or out-of-range")));

	return partdesc->oids[part_index];
}

/*
 *  checkOraclePartitionTree
 *		When using the DROP command to drop a (sub)partition, check
 *		if it violates Oracle's requirements for partitioned tables.
 *
 * When directly drop a partition or subpartition of an Oracle partitioned table,
 * we must avoid violating the Oracle partitioned table convention: at least one
 * partition or subpartition must exist. It is recommended to use ALTER TABLE DROP
 * to drop the partition or subpartition of the Oracle partitioned table.
 * Perhaps we shouldn't allow DROP to be executed directly on partitions and
 * subpartitions of Oracle partitioned tables.
 */
void
checkOraclePartitionTree(List *objects)
{
	int			i;
	List		*partitionlist = NIL;
	List		*parentlist = NIL;
	ListCell	*lc;

	/* no work if no objects */
	if (list_length(objects) == 0)
		return;

	foreach(lc, objects)
	{
		Form_pg_ora_tab_partitions	pg_ora_tab_partitions;
		HeapTuple	tuple;
		Oid			relid = lfirst_oid(lc);

		tuple = SearchSysCache1(ORATABPARTID, ObjectIdGetDatum(relid));

		if (HeapTupleIsValid(tuple))
		{
			pg_ora_tab_partitions = (Form_pg_ora_tab_partitions) GETSTRUCT(tuple);

			parentlist = list_append_unique_oid(parentlist, pg_ora_tab_partitions->partparent);
			partitionlist = lappend_oid(partitionlist, relid);

			ReleaseSysCache(tuple);
		}
	}

	/* no work if no oracle (sub)partitions */
	if (list_length(partitionlist) == 0 ||
		list_length(parentlist) == 0)
		return;
	else
	{
		typedef struct CheckOraPart
		{
			Oid		partid;
			int32	nchild;
		} CheckOraPart;

		CheckOraPart	*checklist = (CheckOraPart *) palloc(sizeof(CheckOraPart) * list_length(parentlist));

		i = 0;
		foreach(lc, parentlist)
		{
			Oid			parentid = lfirst_oid(lc);
			Relation	pg_ora_tab_partitions;
			ScanKeyData	key[1];
			SysScanDesc	scan;
			int32		cnt = 0;

			LockRelationOid(parentid, AccessShareLock);

			pg_ora_tab_partitions = table_open(OraTabPartitionsId, AccessShareLock);

			ScanKeyInit(&key[0],
						Anum_pg_ora_tab_partitions_partparent,
						BTEqualStrategyNumber, F_OIDEQ,
						ObjectIdGetDatum(parentid));

			scan = systable_beginscan(pg_ora_tab_partitions, InvalidOid, false,
									  NULL, 1, key);

			while (systable_getnext(scan))
				cnt++;

			systable_endscan(scan);
			table_close(pg_ora_tab_partitions, AccessShareLock);

			checklist[i].partid = parentid;
			checklist[i++].nchild = cnt;
		}

		for (i = 0; i < list_length(parentlist); i++)
		{
			/* ORA-14083: cannot drop the only partition of a partitioned table */
			if (checklist[i].nchild == 1)
				ereport(ERROR,
						(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
						 errmsg("cannot drop the only partition of a partitioned table")));
		}

		foreach(lc, partitionlist)
		{
			Form_pg_ora_tab_partitions	pg_ora_tab_partitions;
			HeapTuple	tuple;
			Oid			parent;
			Oid			partid = lfirst_oid(lc);

			tuple = SearchSysCache1(ORATABPARTID, ObjectIdGetDatum(partid));

			if (!HeapTupleIsValid(tuple))
				elog(ERROR, "cache lookup failed for oracle (sub)partitione %u", partid);

			pg_ora_tab_partitions = (Form_pg_ora_tab_partitions) GETSTRUCT(tuple);
			parent = pg_ora_tab_partitions->partparent;

			for (i = 0; i < list_length(parentlist); i++)
				if (checklist[i].partid == parent)
					checklist[i].nchild--;

			ReleaseSysCache(tuple);
		}

		for (i = 0; i < list_length(parentlist); i++)
		{
			/* ORA-14331: cannot drop all [sub]partitions of a partitioned table */
			if (checklist[i].nchild <= 0)
				ereport(ERROR,
						(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
						 errmsg("cannot drop all [sub]partitions of a partitioned table")));
		}
	}
}

/*
 * PreventOraclePartitions
 *		Prevent commands from being executed on Oracle partitioned table.
 *
 * Some commands that are not allowed to be executed on Oracle partitioned table, include
 *	1) ALTER TABLE ATTACH
 *	2) ALTER TABLE DETACH
 *	3) CREATE TABLE PARTITION OF
 *
 * Attach and detach operations can disrupt the partition tree structure of Oracle partitions.
 * Oracle requires at least one partition or subpartition to exist, and the detach operation
 * may result in zero child partitions or subpartitions. Attach may attach a partition with a
 * diff partitioning strategy. for safety, we prohibit performing attach and detach operations
 * on Oracle partitioned tables. If you wish to perform this action, please use ALTER TABLE to
 * add a new partition or subpartition.
 */
void
PreventOraclePartitions(Oid relid)
{
	if (!OidIsValid(relid))
		return;

	if (is_oracle_partitioned_table(relid) ||
		is_oracle_partition(relid) ||
		is_oracle_subpartition(relid))
		ereport(ERROR,
				(errcode(ERRCODE_AMBIGUOUS_PARAMETER),
				 errmsg("Oracle partition table do not support this command")));
}

/*
 * BuildPartitionKeyFromArray
 *		Build partition key data from the specified arrays
 *
 * It is a variant of RelationBuildPartitionKey, used when the partition
 * Relation does not yet exist. Currently, it is mainly used to provide
 * support when transforming sub-partition template.
 */
PartitionKey
BuildPartitionKeyFromArray(TupleDesc tupdesc,
									const char strategy,
									const int16 partnatts,
									const AttrNumber *attrs,
									const List *partexprs,
									const Oid *opclass,
									const Oid *collation)
{
	int				i;
	PartitionKey	key = NULL;
	ListCell		*partexprs_item;
	int16			procnum;

	key = (PartitionKey) MemoryContextAllocZero(CurrentMemoryContext,
												sizeof(PartitionKeyData));

	key->strategy = strategy;
	key->partnatts = partnatts;

	if (partexprs)
	{
		Node	*expr;

		expr = eval_const_expressions(NULL, (Node *) partexprs);
		fix_opfuncids(expr);
		key->partexprs = (List *) copyObject(expr);
	}

	key->partattrs = (AttrNumber *) palloc0(key->partnatts * sizeof(AttrNumber));
	key->partopfamily = (Oid *) palloc0(key->partnatts * sizeof(Oid));
	key->partopcintype = (Oid *) palloc0(key->partnatts * sizeof(Oid));
	key->partsupfunc = (FmgrInfo *) palloc0(key->partnatts * sizeof(FmgrInfo));

	key->partcollation = (Oid *) palloc0(key->partnatts * sizeof(Oid));
	key->parttypid = (Oid *) palloc0(key->partnatts * sizeof(Oid));
	key->parttypmod = (int32 *) palloc0(key->partnatts * sizeof(int32));
	key->parttyplen = (int16 *) palloc0(key->partnatts * sizeof(int16));
	key->parttypbyval = (bool *) palloc0(key->partnatts * sizeof(bool));
	key->parttypalign = (char *) palloc0(key->partnatts * sizeof(char));
	key->parttypcoll = (Oid *) palloc0(key->partnatts * sizeof(Oid));

	/* determine support function number to search for */
	procnum = (key->strategy == PARTITION_STRATEGY_HASH) ?
		HASHEXTENDED_PROC : BTORDER_PROC;

	/* Copy partattrs and fill other per-attribute info */
	memcpy(key->partattrs, attrs, key->partnatts * sizeof(int16));
	partexprs_item = list_head(key->partexprs);

	for (i = 0; i < key->partnatts; i++)
	{
		AttrNumber	attno = key->partattrs[i];
		HeapTuple	opclasstup;
		Oid			funcid;
		Form_pg_opclass	opclassform;

		/* Collect opfamily information */
		opclasstup = SearchSysCache1(CLAOID,
									 ObjectIdGetDatum(opclass[i]));
		if (!HeapTupleIsValid(opclasstup))
			elog(ERROR, "cache lookup failed for opclass %u", opclass[i]);

		opclassform = (Form_pg_opclass) GETSTRUCT(opclasstup);
		key->partopfamily[i] = opclassform->opcfamily;
		key->partopcintype[i] = opclassform->opcintype;

		/* Get a support function for the specified opfamily and datatypes */
		funcid = get_opfamily_proc(opclassform->opcfamily,
								   opclassform->opcintype,
								   opclassform->opcintype,
								   procnum);
		if (!OidIsValid(funcid))
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
					 errmsg("operator class \"%s\" of access method %s is missing support function %d for type %s",
							NameStr(opclassform->opcname),
							(key->strategy == PARTITION_STRATEGY_HASH) ?
							"hash" : "btree",
							procnum,
							format_type_be(opclassform->opcintype))));

		fmgr_info_cxt(funcid, &key->partsupfunc[i], CurrentMemoryContext);

		/* Collation */
		key->partcollation[i] = collation[i];

		/* Collect type information */
		if (attno != 0)
		{
			Form_pg_attribute att = TupleDescAttr(tupdesc, attno - 1);

			key->parttypid[i] = att->atttypid;
			key->parttypmod[i] = att->atttypmod;
			key->parttypcoll[i] = att->attcollation;
		}
		else
		{
			if (partexprs_item == NULL)
				elog(ERROR, "wrong number of partition key expressions");

			key->parttypid[i] = exprType(lfirst(partexprs_item));
			key->parttypmod[i] = exprTypmod(lfirst(partexprs_item));
			key->parttypcoll[i] = exprCollation(lfirst(partexprs_item));

			partexprs_item = lnext(key->partexprs, partexprs_item);
		}

		get_typlenbyvalalign(key->parttypid[i],
							 &key->parttyplen[i],
							 &key->parttypbyval[i],
							 &key->parttypalign[i]);

		ReleaseSysCache(opclasstup);
	}

	return key;
}

/*
 *  GetOracleSubpartitionKey
 *		Get the subpartition Key of an Oracle partitioned table.
 *
 * Returns NULL if it is a simple partition, the caller must
 * ensure that this is an Oracle-compatible partitioned table.
 */
PartitionKey
GetOracleSubpartitionKey(Relation rootrel)
{
	HeapTuple	tuple;
	Datum		datum;
	bool		isnull;
	char		subpartstrat;
	int16		subpartnatts;
	oidvector	*subpartclass;
	oidvector	*subpartcollation;
	List		*subpartexprs = NIL;
	PartitionKey	result = NULL;
	Form_pg_ora_part_tables form;

	tuple = SearchSysCache1(ORAPARTRELID, ObjectIdGetDatum(RelationGetRelid(rootrel)));
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "cache lookup failed for oracle partitioned table of %u",
				RelationGetRelid(rootrel));

	form = (Form_pg_ora_part_tables) GETSTRUCT(tuple);

	if (!form->composite)
	{
		ReleaseSysCache(tuple);
		return NULL;	/* simple partition */
	}

	subpartstrat = form->subpartstrat;
	subpartnatts = form->subpartnatts;

	/* subpartclass */
	datum = SysCacheGetAttr(ORAPARTRELID, tuple,
							Anum_pg_ora_part_tables_subpartclass, &isnull);

	if (isnull)
		elog(ERROR, "the subpartclass is null of oracle partitioned table %u",
				RelationGetRelid(rootrel));

	subpartclass = (oidvector *) DatumGetPointer(datum);

	/* subpartcollation */
	datum = SysCacheGetAttr(ORAPARTRELID, tuple,
							Anum_pg_ora_part_tables_subpartcollation, &isnull);

	if (isnull)
		elog(ERROR, "the subpartcollation is null of oracle partitioned table %u",
				RelationGetRelid(rootrel));

	subpartcollation = (oidvector *) DatumGetPointer(datum);

	/* subpartexprs */
	datum = SysCacheGetAttr(ORAPARTRELID, tuple,
							Anum_pg_ora_part_tables_subpartexprs, &isnull);

	if (!isnull)
		subpartexprs = castNode(List, stringToNode(TextDatumGetCString(datum)));

	result = BuildPartitionKeyFromArray(rootrel->rd_att,
										subpartstrat,
										subpartnatts,
										form->subpartattrs.values,
										subpartexprs,
										subpartclass->values,
										subpartcollation->values);

	/* Clean up */
	ReleaseSysCache(tuple);

	return result;
}

/* ----------------------------------------------------------------
 *					export interface
 * ----------------------------------------------------------------
 */

/*
 *  ora_def_subpartition_count
 *		Get the default number of subpartitions for a composite partition table.
 *
 * If it is an Oracle composite partitioned table, the default number of subpartitions is
 * returned. If a subpartition template exists, the number of subpartitions in the subpartition
 * template is returned. Otherwise, the default number of subpartitions is 1. That is, by default
 * the RANGE partition strategy create a MAXVALUE subpartition, the LIST partition strategy create
 * a DEFAULT subpartition, and the HASH partition strategy create one hash partition.
 */
Datum
ora_def_subpartition_count(PG_FUNCTION_ARGS)
{
	Oid		partid = PG_GETARG_OID(0);
	int		defsubpartcnt = 0;
	OraSubPartitionTemplateSpec	*temp = NULL;

	/*
	 * If it is not an Oracle partitioned table or is a Oracle
	 * simple partitioned table, we return 0.
	 */
	if (!is_oracle_partitioned_table(partid) ||
		!is_ora_composite_partitioning(partid))
		PG_RETURN_INT32(0);

	temp = ora_get_subpartition_template(partid);
	if (temp)
		defsubpartcnt =
			temp->subpartitons_desc ? 
			list_length(temp->subpartitons_desc) : temp->hash_subpartition_quantity;
	else
		defsubpartcnt = 1;

	PG_RETURN_INT32(defsubpartcnt);
}

/*
 * ora_def_subpartition_count
 *		Get the (sub)partition location of an Oracle partitioned table.
 */
Datum
ora_get_partition_position(PG_FUNCTION_ARGS)
{
	Oid				parentid = PG_GETARG_OID(0);
	Oid				childid = PG_GETARG_OID(1);
	char			parent_strategy;
	int				nchild = 0;
	int				i = -1;
	Oid				*allchild;
	Relation		parentrel;
	PartitionDesc	parentdesc;

	if (get_rel_relkind(parentid) != RELKIND_PARTITIONED_TABLE)
		PG_RETURN_NULL();

	parentrel = table_open(parentid, AccessShareLock);
	parentdesc = RelationGetPartitionDesc(parentrel, true);

	if (parentdesc->nparts <= 0)
	{
		table_close(parentrel, AccessShareLock);
		PG_RETURN_NULL();
	}
	else
	{
		nchild = parentdesc->nparts;
		allchild = (Oid *) palloc(nchild * sizeof(Oid));
		memcpy(allchild, parentdesc->oids, nchild * sizeof(Oid));
	}

	table_close(parentrel, AccessShareLock);

	parent_strategy = get_partitioning_strategy(parentid);
	switch (parent_strategy)
	{
		case PARTITION_STRATEGY_RANGE:
			/*
			 * The range partitions are ordered according to their
			 * partition boundary values; therefore, the partition
			 * description has already done all the work.
			 */
			for (i = 0; i < nchild; i++)
				if (childid == allchild[i])
					break;
			break;

		case PARTITION_STRATEGY_LIST:
		case PARTITION_STRATEGY_HASH:
			/* sort them by oid */
			if (nchild > 1)
				qsort(allchild, nchild, sizeof(Oid), oid_cmp);

			for (i = 0; i < nchild; i++)
				if (childid == allchild[i])
					break;
			break;

		default:
			elog(ERROR, "unexpected partition strategy: %d",
				 (int) parent_strategy);
			break;
	}

	pfree(allchild);

	if (i >= 0 && i < nchild)
		PG_RETURN_INT32(i + 1);
	else
		PG_RETURN_NULL();
}
