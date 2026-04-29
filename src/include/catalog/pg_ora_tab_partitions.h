/*------------------------------------------------------------------------------------
 *
 * File:pg_ora_tab_partitions.h
 *
 * Abstract:
 *		definition of the Oracle "partitions & subpartitions" system catalog
 *		(pg_ora_tab_partitions)
 *
 *
 * ReqID:SRS-SQL-PARTITION-TABLE
 *
 * Authored by zhenmingyang@highgo.com,20251021.
 *
 * Copyright:
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  src/include/catalog/pg_ora_tab_partitions.h
 *
 *------------------------------------------------------------------------------------
 */
#ifndef PG_ORA_TAB_PARTITIONS_H
#define PG_ORA_TAB_PARTITIONS_H

#include "catalog/genbki.h"
#include "catalog/pg_ora_tab_partitions_d.h"

/* ----------------
 *		pg_ora_tab_partitions definition.  cpp turns this into
 *		typedef struct FormData_pg_ora_tab_partitions
 * ----------------
 */
CATALOG(pg_ora_tab_partitions,4810,OraTabPartitionsId)
{
	/* root */
	Oid			partroot BKI_LOOKUP(pg_class);

	/* parent of (sub)partitions */
	Oid			partparent BKI_LOOKUP(pg_class);

	/* oid of (sub)partitions */
	Oid			part BKI_LOOKUP(pg_class);

	/* name of (sub)partitions */
	NameData	partname;

	/* partitioning level, partitions 1, subpartitions 2 */
	int16		partlevel;

	/* auto generated interval partitions? */
	bool		autogen BKI_DEFAULT(f);
} FormData_pg_ora_tab_partitions;

/* ----------------
 *		Form_pg_ora_tab_partitions corresponds to a pointer to a tuple with
 *		the format of pg_ora_tab_partitions relation.
 * ----------------
 */
typedef FormData_pg_ora_tab_partitions *Form_pg_ora_tab_partitions;

DECLARE_UNIQUE_INDEX_PKEY(pg_ora_tab_partitions_part_index, 4811, on pg_ora_tab_partitions using btree(part oid_ops));
#define OraTabPartitionsIdPartIndexId			 4811

DECLARE_UNIQUE_INDEX(pg_ora_tab_partitions_partroot_partname, 4812, on pg_ora_tab_partitions using btree(partroot oid_ops, partname name_ops));
#define OraTabPartitionsPartNameId  4812

#endif							/* PG_ORA_TAB_PARTITIONS_H */
