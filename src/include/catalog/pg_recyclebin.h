/*-------------------------------------------------------------------------
 *
 * pg_recyclebin.h
 *		define the recyclebin for flashback.
 *
 * Portions Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  src/include/catalog/pg_recyclebin.h
 *
 * add the file for requirement "SRS-RECOVERY-FLASHBACK"
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_RECYCLEBIN_H
#define PG_RECYCLEBIN_H

#include "catalog/genbki.h"
#include "catalog/pg_recyclebin_d.h"
#include "commands/flashback.h"

#define timestamptz Datum

/* ----------------
 *    pg_recyclebin definition.  cpp turns this into
 *    typedef struct FormData_pg_recyclebin
 * ----------------
 */

CATALOG(pg_recyclebin,8813,RecyclebinRelationId) BKI_ROWTYPE_OID(8814,RecyclebinRelation_Rowtype_Id) BKI_SCHEMA_MACRO
{
    Oid oid;

    /* base rb item id, refer to pg_recyclebin.oid */
    Oid rcybaseid;

    /* relation id */
    Oid rcyrelid;

    /* unique recycle object name */
    NameData rcyname;

    /* origin name before truncate or drop */
    NameData rcyoriginname;

    /* operation type: 't' truncate, 'd' drop */
    char rcyoperation;

    /* object type: 0 table, 1 index, 2 toast table */
    int16 rcytype;

    /* drop/truncate timestamp */
    timestamptz rcyrecycletime;

    /* OID of namespace containing this class */
    Oid rcynamespace;

    /* identifier of table space for object */
    Oid rcytablespace;

    /* identifier of physical storage file */
    Oid rcyrelfilenode;

} FormData_pg_recyclebin;

/* ----------------
 *        Form_pg_recyclebin corresponds to a pointer to a tuple with
 *        the format of pg_recyclebin relation.
 * ----------------
 */
typedef FormData_pg_recyclebin *Form_pg_recyclebin;

DECLARE_UNIQUE_INDEX_PKEY(pg_recyclebin_oid_index, 8815, on pg_recyclebin using btree(oid oid_ops));
#define RecyclebinOidIndexId	8815
DECLARE_INDEX(pg_recyclebin_baseid_index, 8816, on pg_recyclebin using btree(rcybaseid oid_ops));
#define RecyclebinBaseidIndexId	8816

/* rcyoperation: drop table, truncate table */
#define RCYOPERATION_DROP 'd'
#define RCYOPERATION_TRUNCATE 't'

#undef timestamptz

extern Oid RecyclebinWrite(RbObjDesc *desc, Oid baseid);
extern void RecyclebinRead(RbObjDesc *desc, HeapTuple rbtup);
extern void RbOperFetch(const char* schemaname, const char* relname, 
        RbObjType type, RbObjDesc *desc, RbOperMode operMode);
extern bool RbFetchAny(RbObjDesc *desc);
extern void RecyclebinDeleteNamespace(Oid nspId);

#endif /* PG_RECYCLEBIN_H */

