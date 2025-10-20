/*-------------------------------------------------------------------------
 *
 * pg_object.c
 *	  MySQL case routines
 *
 * Portions Copyright (c) 2024, Highgo MySQL Compatible Development Group
 *
 * Authored by qinshiyu@highgo.com, 20240410.
 *
 * IDENTIFICATION
 *	  contrib/ivorysql_my/src/pg_object/pg_object.c
 *
 * NOTES
 *	  These routines allow all to perform
 *
 *	  see contrib/pg_object/pg_object_inertface.h for a list of the cache IDs
 *
 * add the file for requirement "SRS-SQL-Object"
 *
 *-------------------------------------------------------------------------
 */

#include "c.h"
#include "postgres.h"

#include "access/heapam.h"
#include "access/genam.h"
#include "access/table.h"
#include "access/xact.h"
#include "catalog/dependency.h"
#include "catalog/indexing.h"
#include "catalog/namespace.h"
#include "catalog/objectaccess.h"
#include "catalog/pg_attribute.h"
#include "catalog/pg_database.h"
#include "catalog/pg_namespace.h"
#include "catalog/pg_object.h"
#include "catalog/pg_proc.h"
#include "nodes/parsenodes.h"
#include "nodes/pg_list.h"
#include "nodes/value.h"
#include "parser/pg_object_hook.h"
#include "parser/scansup.h"
#include "pg_object_interface.h"
#include "utils/builtins.h"
#include "utils/catcache.h"
#include "utils/fmgroids.h"
#include "utils/guc.h"
#include "utils/jsonb.h"
#include "utils/lsyscache.h"
#include "utils/ora_compatible.h"
#include "utils/rel.h"
#include "utils/syscache.h"
#include "utils/timestamp.h"

static Timestamp timestamptz2timestamp(TimestampTz timestamp);

/* timestamptz2timestamp()
 * Get the current time, which is used as the creation time
 * or modification time of the pg object.
 *
 * return a timestamp
 */
static Timestamp timestamptz2timestamp(TimestampTz timestamp)
{
	Timestamp		result;
	struct pg_tm	tt,
				   *tm = &tt;
	fsec_t			fsec;
	int				tz;

	if (TIMESTAMP_NOT_FINITE(timestamp))
		result = timestamp;
	else
	{
		if (timestamp2tm(timestamp, &tz, tm, &fsec, NULL, NULL) != 0)
			ereport(ERROR,
					(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
					 errmsg("timestamp out of range")));
		if (tm2timestamp(tm, 0, NULL, &result) != 0)
			ereport(ERROR,
					(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
					 errmsg("timestamp out of range")));
	}
	return result;
}


/* pg_object_createobj
 *
 * In oracle or mysql mode, when creating an object,
 * store the relevant object in the system table pg_object.
 *
 */
void pg_object_createobj(char *objname, Oid objnsp, bool replace, char objkind,
								ObjectAddress address, Jsonb *attr, Timestamp *createtime)
{
	Relation		rel;
	HeapTuple		oldtuple;
	HeapTuple		newtup;
	bool			nulls[Natts_pg_object];
	Datum			values[Natts_pg_object];
	bool			replaces[Natts_pg_object];
	NameData		dobjname;
	TupleDesc		tupDesc;
	Timestamp		tms;
	char			compatible_mode;

	if (!enable_pg_object || (objname == NULL))
		return;

	/* Initialize inserted values */
	MemSet(values, 0, sizeof(values));
	MemSet(nulls, false, sizeof(nulls));
	MemSet(replaces, true, sizeof(replaces));

	if (attr == NULL)
		nulls[Anum_pg_object_option - 1] = true;

	/* Objects created in that compatibility mode */
	if (compatible_db == ORA_PARSER)
		compatible_mode = 'o';
	/* Begin - ReqID:SRS-MYSQL-CASE */
	else if (compatible_db == MYS_PARSER)
		compatible_mode = 'm';
	/* End - ReqID:SRS-MYSQL-CASE */
	else
		compatible_mode = 'p';

	namestrcpy(&dobjname, pstrdup(objname));

	/* Creation or modification time */
	if (createtime == NULL)
		tms = timestamptz2timestamp(GetCurrentStatementStartTimestamp());
	else
		tms = *createtime;

	rel = table_open(ObjectRelationId, RowExclusiveLock);
	tupDesc = RelationGetDescr(rel);

	/* First check whether the object already exists */
	oldtuple = SearchSysCache3(OBJECTREFCLASSIDOIDSUBID,
							   ObjectIdGetDatum(address.classId),
							   ObjectIdGetDatum(address.objectId),
							   Int32GetDatum(address.objectSubId));
	/* if the object exists, update is executed,
	 * if it does not exist, insert is executed.
	 */
	if (HeapTupleIsValid(oldtuple))
	{
		/* NOTICE: replace means that there is replace
		 * in the syntax of creating an object.
		 */
		if (!replace)
		{
			/* Begin - BUG#Z00004347 */
			Form_pg_object object_struct = (Form_pg_object) GETSTRUCT(oldtuple);

			ereport(ERROR,
					(errcode(ERRCODE_DUPLICATE_FUNCTION),
					 errmsg("originobject \"%s\" already exists, oid  = %d; newobject \"%s\", oid  = %d. \n",
							object_struct->objname.data, object_struct->obj_oid,
							objname, address.objectId)));
			/* End - BUG#Z00004347 */
		}
		else
		{
			/* fill in the values and then update */
			values[Anum_pg_object_obj_oid - 1] = ObjectIdGetDatum(address.objectId);
			values[Anum_pg_object_objsubid - 1] = Int32GetDatum(address.objectSubId);
			values[Anum_pg_object_objname - 1] = NameGetDatum(&dobjname);
			values[Anum_pg_object_refclassid - 1] = ObjectIdGetDatum(address.classId);
			values[Anum_pg_object_schemaoid - 1] = ObjectIdGetDatum(objnsp);
			values[Anum_pg_object_objkind - 1] = CharGetDatum(objkind);
			values[Anum_pg_object_modified - 1] = TimestampGetDatum(tms);
			if (attr)
				values[Anum_pg_object_option - 1] = PointerGetDatum(attr);
			else
				nulls[Anum_pg_object_option - 1] = true;

			replaces[Anum_pg_object_created - 1] = false;
			values[Anum_pg_object_compatible_mode - 1] = CharGetDatum(compatible_mode);
			nulls[Anum_pg_object_compatible_mode - 1] = false;

			newtup = heap_modify_tuple(oldtuple, tupDesc, values, nulls, replaces);
			CatalogTupleUpdate(rel, &newtup->t_self, newtup);

			ReleaseSysCache(oldtuple);
		}
	}
	else
	{
		/* fill in the values and then insert */
		values[Anum_pg_object_obj_oid - 1] = ObjectIdGetDatum(address.objectId);
		values[Anum_pg_object_objsubid - 1] = Int32GetDatum(address.objectSubId);
		values[Anum_pg_object_objname - 1] = NameGetDatum(&dobjname);
		values[Anum_pg_object_refclassid - 1] = ObjectIdGetDatum(address.classId);
		values[Anum_pg_object_schemaoid - 1] = ObjectIdGetDatum(objnsp);
		values[Anum_pg_object_objkind - 1] = CharGetDatum(objkind);
		values[Anum_pg_object_created - 1] = TimestampGetDatum(tms);
		values[Anum_pg_object_modified - 1] = TimestampGetDatum(tms);
		if (attr)
			values[Anum_pg_object_option - 1] = PointerGetDatum(attr);
		else
			nulls[Anum_pg_object_option - 1] = true;

		values[Anum_pg_object_compatible_mode - 1] = CharGetDatum(compatible_mode);
		nulls[Anum_pg_object_compatible_mode - 1] = false;

		newtup = heap_form_tuple(tupDesc, values, nulls);
		CatalogTupleInsert(rel, newtup);
	}

	table_close(rel, RowExclusiveLock);
}

/* pg_compatible_mysql_alterobj
 *
 * modify object name, schema and option,
 * It is necessary to know the oid of these objects,
 * which do not include columns.
 */
void pg_object_alterobj(ObjectAddress address, char *newname, Oid newschema, Jsonb *attr, Timestamp *modifiedtime)
{
	Relation	rel;
	bool		nulls[Natts_pg_object];
	Datum		values[Natts_pg_object];
	bool		replaces[Natts_pg_object];
	HeapTuple	oldtup;
	HeapTuple	newtup;
	Timestamp	tms;
	NameData	newprocname;

	oldtup = SearchSysCache3(OBJECTREFCLASSIDOIDSUBID,
							 ObjectIdGetDatum(address.classId),
							 ObjectIdGetDatum(address.objectId),
							 Int32GetDatum(address.objectSubId));

	if (!HeapTupleIsValid(oldtup))
		return;

	/* If the object does not exist, return directly */
	rel = table_open(ObjectRelationId, RowExclusiveLock);

	/* update time */
	if (modifiedtime == NULL)
		tms = timestamptz2timestamp(GetCurrentStatementStartTimestamp());
	else
		tms = *modifiedtime;

	MemSet(values, 0, sizeof(values));
	MemSet(nulls, false, sizeof(nulls));
	MemSet(replaces, false, sizeof(replaces));

	/* rename the object */
	if (newname != NULL)
	{
		namestrcpy(&newprocname, pstrdup(newname));

		values[Anum_pg_object_objname - 1] = NameGetDatum(&newprocname); /* BUG#Z00004347 */
		replaces[Anum_pg_object_objname - 1] = true;
	}

	/* modify the schema of the object */
	values[Anum_pg_object_schemaoid - 1] = ObjectIdGetDatum(newschema);
	replaces[Anum_pg_object_schemaoid - 1] = true;

	/* modify the option of the object */
	if (attr != NULL)
	{
		values[Anum_pg_object_option - 1] = PointerGetDatum(attr);
		replaces[Anum_pg_object_option - 1] = true;
	}
	else
	{
		nulls[Anum_pg_object_option - 1] = true;
		replaces[Anum_pg_object_option - 1] = true;
	}

	/* modify update time */
	values[Anum_pg_object_modified - 1] = TimestampGetDatum(tms);
	replaces[Anum_pg_object_modified - 1] = true;

	newtup = heap_modify_tuple(oldtup, RelationGetDescr(rel),
							   values, nulls, replaces);

	/* Perform actual update */
	CatalogTupleUpdate(rel, &oldtup->t_self, newtup);

	heap_freetuple(newtup);
	ReleaseSysCache(oldtup);

	/* If the column exists, the schema of
	 * the column is also updated.
	 */
	if (address.classId == RelationRelationId)
	{
		/* Modify the schema of the column. In fact, the column does not
		 * have a schema. When modifying the schema of the table, it must
		 * be modified synchronously.
		 */
		SysScanDesc		pg_object_scan;
		ScanKeyData		skey[3];

		MemSet(values, 0, sizeof(values));
		MemSet(nulls, false, sizeof(nulls));
		MemSet(replaces, false, sizeof(replaces));

		ScanKeyInit(&skey[0],
					Anum_pg_object_refclassid,
					BTEqualStrategyNumber, F_OIDEQ,
					ObjectIdGetDatum(AttributeRelationId));
		ScanKeyInit(&skey[1],
					Anum_pg_object_obj_oid,
					BTEqualStrategyNumber, F_OIDEQ,
					ObjectIdGetDatum(address.objectId));
		ScanKeyInit(&skey[2],
					Anum_pg_object_objsubid,
					BTGreaterStrategyNumber, F_INT4GT,
					Int32GetDatum(0));

		pg_object_scan = systable_beginscan(rel,
											ObjectRefidOidSubidIndexId,
											true,
											NULL,
											3,
											skey);
		while (HeapTupleIsValid(oldtup = systable_getnext(pg_object_scan)))
		{
			values[Anum_pg_object_schemaoid - 1] = ObjectIdGetDatum(newschema);
			replaces[Anum_pg_object_schemaoid - 1] = true;
			newtup = heap_modify_tuple(oldtup, RelationGetDescr(rel),
									   values, nulls, replaces);

			CatalogTupleUpdate(rel, &oldtup->t_self, newtup);
			
			heap_freetuple(newtup);
		}
		/* Clean up after the scan */
		systable_endscan(pg_object_scan);
	}

	table_close(rel, RowExclusiveLock);
}

/* pg_object_drop
 * Delete objects based on oid and objsubid.
 */
void pg_object_drop(ObjectAddress address)
{
	Relation	rel;
	HeapTuple	tup;

	tup = SearchSysCache3(OBJECTREFCLASSIDOIDSUBID,
						  ObjectIdGetDatum(address.classId),
						  ObjectIdGetDatum(address.objectId),
						  Int32GetDatum(address.objectSubId));

	if (!HeapTupleIsValid(tup))
		return;

	rel = table_open(ObjectRelationId, RowExclusiveLock);

	CatalogTupleDelete(rel, &tup ->t_self);
	ReleaseSysCache(tup);

	/* If it is related to the table, then find out whether
	 * the column needs to be deleted.
	 */
	if (address.classId == RelationRelationId)
	{
		SysScanDesc scan;
		ScanKeyData key[3];
		HeapTuple	atttup;

		/* Delete all the matching tuples */
		ScanKeyInit(&key[0],
					Anum_pg_object_refclassid,
					BTEqualStrategyNumber, F_OIDEQ,
					ObjectIdGetDatum(AttributeRelationId));
		ScanKeyInit(&key[1],
					Anum_pg_object_obj_oid,
					BTEqualStrategyNumber,F_OIDEQ,
					ObjectIdGetDatum(address.objectId));
		ScanKeyInit(&key[2],
					Anum_pg_object_objsubid,
					BTGreaterStrategyNumber, F_INT4GT,
					Int32GetDatum(0));

		scan = systable_beginscan(rel,
								  ObjectRefidOidSubidIndexId,
								  true,
								  NULL,
								  3,
								  key);

		while (HeapTupleIsValid(atttup = systable_getnext(scan)))
		{
			CatalogTupleDelete(rel, &atttup->t_self);
		}

		/* Clean up after the scan */
		systable_endscan(scan);
	}

	table_close(rel, RowExclusiveLock);
}

/* get_object_origin_info
 * Get the record information of the system table pg_object.
 */
PgObjectInfo get_object_origin_info(ObjectAddress address)
{
	HeapTuple	tup;
	Datum		val;
	bool		isNull;
	PgObjectInfo objinfo;
	Form_pg_object struct_object; /* BUG#Z00004347 */

	tup = SearchSysCache3(OBJECTREFCLASSIDOIDSUBID,
						  ObjectIdGetDatum(address.classId),
						  ObjectIdGetDatum(address.objectId),
						  Int32GetDatum(address.objectSubId));
	if (!HeapTupleIsValid(tup))
		return NULL;

	objinfo = (pg_object_ori_infor *)palloc(sizeof(pg_object_ori_infor));

	/* Begin - BUG#Z00004347 */
	struct_object = (Form_pg_object) GETSTRUCT(tup);

	/* get object name */
	objinfo->oriname = pstrdup(struct_object->objname.data);

	/* get object schema */
	objinfo->orischema = struct_object->schemaoid;

	/* get object kinkd */
	objinfo->orikind = struct_object->objkind;

	/* get object compatible_mod */
	objinfo->oricompatible_mode = struct_object->compatible_mode;

	/* get object modify time */
	{
		objinfo->orimodifytime = palloc(sizeof(Timestamp));
		*objinfo->orimodifytime = struct_object->modified;
	}

	/* get object option */
	val = SysCacheGetAttr(OBJECTREFCLASSIDOIDSUBID, tup,
						  Anum_pg_object_option,
						  &isNull);

	/* The option column may be NULL, so it is necessary to judge NULL */
	if (isNull)
		objinfo->orioption = NULL;
	else
		objinfo->orioption = DatumGetJsonbPCopy(val);
	/* End - BUG#Z00004347 */

	ReleaseSysCache(tup);

	return objinfo;
}
