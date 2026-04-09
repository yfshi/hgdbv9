/*------------------------------------------------------------------------------------
 *
 * File: ora_partition.h
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
 *		src/include/partitioning/ora_partition.h
 *
 *------------------------------------------------------------------------------------
 */
#ifndef ORA_PARTITION_H
#define ORA_PARTITION_H


#define GEN_AUTO_PART_QUERY	"(auto generated interval or automatic partition)"

/*
 * transform
 */
extern List *transformOraclePartitionSpec(OraPartitionSpec *ora_partspec, RangeVar *root);
extern List *transformOracleModifyPartitionStmt(AlterTableStmt *atstmt);
extern List *derivePostgresPartitionStmts(char strategy, List *part_desc, RangeVar *part_parent,
									PartitionSpec *subpart_key, OraSubPartitionTemplateSpec *subpart_template,
									char *parent_part_name, int part_key_cnt, bool is_subpart, int hash_part_quantity,
									bool is_automatic, bool is_interval, List *lower_bound, HTAB *part_duplicate_check,
									HTAB *relname_duplicate_check, int ora_partname_seqno, int *final_seqno);
extern void checkOracleSubpartitionDesc(const OraSubPartitionTemplateSpec *desc, char strategy, int keycnt, bool istempldate);
extern char *make_system_partname_unique(HTAB *partnamehash, bool is_subpartition, int start_seqno, int *end_seqno);
extern void FetchOraclePartNameToHash(Oid rootid, HTAB *desthash);
extern Oid ResolvePartitionExtenedName(Relation rel, List *forexpr);
extern PartitionKey BuildPartitionKeyFromArray(TupleDesc tupdesc,
									const char strategy,
									const int16 partnatts,
									const AttrNumber *attrs,
									const List *partexprs,
									const Oid *opclass,
									const Oid *collation);
extern PartitionKey GetOracleSubpartitionKey(Relation rootrel);

/*
 * catalog
 */
extern void
StoreOraclePartitionedTable(Oid relationId,
									OraPartitionSpec *desc,
									char subpartstrat,
									int16 subpartnatts,
									AttrNumber *subpartattrs,
									List *subpartexprs,
									Oid *subpartopclass,
									Oid *subpartcollation);
extern void UpdateOraclePartitionedTableSeqno(Oid rootid, int32 seqno);
extern void UpdateOraclePartitionedTableTemplate(Oid rootid, OraSubPartitionTemplateSpec *temp);
extern void UpdateOraclePartitionedTableInterval(Oid rootid, PartitionInterval *interval_expr);
extern void RemoveOraclePartitionedTable(Oid relid);
extern void StoreOraclePartitions(Oid partid, Oid parentid, Oid rootpartid,
										  char *partname, int16 partlevel, bool autogen);
extern void RemoveOraclePartitions(Oid relid);
extern void StorePartitionKeyFromSibling(Relation rel, Oid sibling);
extern Oid update_next_range_partition_bound(Oid relid, Oid parentOid, bool uselower);

/*
 * misc
 */
extern void checkOraclePartitionTree(List *objects);
extern void PreventOraclePartitions(Oid relid);

#endif							/* ORA_PARTITION_H */
