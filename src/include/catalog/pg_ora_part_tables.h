/*------------------------------------------------------------------------------------
 *
 * File:pg_ora_part_tables.h
 *
 * Abstract:
 *		definition of the Oracle "partitioned table" system catalog(pg_ora_part_tables)
 *
 *
 * ReqID:SRS-SQL-PARTITION-TABLE
 *
 * Authored by zhenmingyang@highgo.com,20251020.
 *
 * Copyright:
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  src/include/catalog/pg_ora_part_tables.h
 *
 *------------------------------------------------------------------------------------
 */
#ifndef PG_ORA_PART_TABLES_H
#define PG_ORA_PART_TABLES_H

#include "catalog/genbki.h"
#include "catalog/pg_ora_part_tables_d.h"

/* ----------------
 *		pg_ora_part_tables definition.  cpp turns this into
 *		typedef struct FormData_pg_ora_part_tables
 * ----------------
 */
CATALOG(pg_ora_part_tables,4800,OraPartitionedRelationId)
{
	/* partitioned table oid */
	Oid			partrelid BKI_LOOKUP(pg_class);

	/* is a automatic list partition? */
	bool		automatic BKI_DEFAULT(f);

	/* is a composite partition? */
	bool		composite BKI_DEFAULT(f);

	/* serial number used by sys_p and sys_subp */
	int32		seqno;

	/*
	 * In normal case, both the partition strategy and the subpartition
	 * strategy are stored in the pg_partitioned_table system table.
	 * However, in order to support pg_dump, a multi-step Oracle partition
	 * creation syntax is allowed in the pg_dump environment.
	 */
	char		subpartstrat;
	int16		subpartnatts;

	/*
	 * variable-length fields start here, but we allow direct access to
	 * partattrs via the C struct.  That's because the first variable-length
	 * field of a heap tuple can be reliably accessed using its C struct
	 * offset, as previous fields are all non-nullable fixed-length fields.
	 */
	int2vector	subpartattrs;

#ifdef CATALOG_VARLEN
	oidvector	subpartclass;
	oidvector	subpartcollation;
	pg_node_tree subpartexprs;

	/* interval expr of range partition */
	pg_node_tree interval_expr BKI_DEFAULT(_null_);

	/* subpartition template if any */
	pg_node_tree subpart_template BKI_DEFAULT(_null_);
#endif
} FormData_pg_ora_part_tables;

/* ----------------
 *		Form_pg_ora_part_tables corresponds to a pointer to a tuple with
 *		the format of pg_ora_part_tables relation.
 * ----------------
 */
typedef FormData_pg_ora_part_tables *Form_pg_ora_part_tables;

DECLARE_TOAST(pg_ora_part_tables, 4802, 4803);

DECLARE_UNIQUE_INDEX_PKEY(pg_ora_part_tables_partrelid_index, 4801, on pg_ora_part_tables using btree(partrelid oid_ops));
#define OraPartitionedRelidIndexId			 4801

#endif							/* PG_ORA_PART_TABLES_H */
