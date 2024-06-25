/*-------------------------------------------------------------------------
 *
 * pg_object_to_relcache.c
 *	  MySQL case routines
 *
 * Portions Copyright (c) 2024, Highgo MySQL Compatible Development Group
 *
 * Authored by qinshiyu@highgo.com, 20240614.
 *
 * IDENTIFICATION
 *	  contrib/ivorysql_my/src/pg_object/pg_object_to_relcache.c
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

#include "postgres.h"
#include "c.h"

#include "access/genam.h"
#include "access/xact.h"
#include "access/table.h"
#include "access/heapam.h"
#include "catalog/dependency.h"
#include "catalog/objectaccess.h"
#include "catalog/pg_object.h"
#include "catalog/pg_proc.h"
#include "catalog/indexing.h"
#include "catalog/namespace.h"
#include "catalog/pg_namespace.h"
#include "catalog/pg_database.h"
#include "catalog/pg_attribute.h"
#include "utils/guc.h"
#include "utils/builtins.h"
#include "utils/catcache.h"
#include "utils/ora_compatible.h"
#include "utils/syscache.h"
#include "utils/timestamp.h"
#include "utils/rel.h"
#include "utils/lsyscache.h"
#include "utils/jsonb.h"
#include "utils/fmgroids.h"
#include "nodes/parsenodes.h"
#include "nodes/pg_list.h"
#include "nodes/value.h"
#include "parser/scansup.h"
#include "pg_object_interface.h"
#include "parser/pg_object_hook.h"

static void relattrfill(Relation objrel, Relation relation, HeapTuple tup, RelAttrFillOption opr);

/* RelationBuildColumnAttr
 * Put the column information of the table into the rel cache.
 */
void RelationBuildColumnAttr(Relation relation)
{
	Relation		rel;
	HeapTuple		tup;
	HeapTuple		pg_object_tuple;
	SysScanDesc		pg_object_scan;
	ScanKeyData		skey[3];
	int		relattrsnum = 0;

	/* If the oid of the table is invalid, return directly*/
	if (relation == NULL)
		return;
	if (RelationGetRelid(relation) < FirstGenbkiObjectId)
		return;

	/* check whether the user table has records in the
	 * pg_object system table. If so, write the records
	 * in pg_object to the cache of the table. If not,
	 * return directly.
	 */

	tup = SearchSysCache3(OBJECTREFCLASSIDOIDSUBID,
						  ObjectIdGetDatum(RelationRelationId),
						  ObjectIdGetDatum(RelationGetRelid(relation)),
						  Int32GetDatum(0));

	if (!HeapTupleIsValid(tup))
	{
		relation->relattrinfo = NULL;
		return;
	}

	rel = table_open(ObjectRelationId, AccessShareLock);
	relattrsnum = RelationGetNumberOfAttributes(relation);

	relation->relattrinfo = 
		(RelinffromObject *)palloc(sizeof(RelinffromObject));
	relation->relattrinfo->relinfo =
		(ObjectAttribute *)palloc(sizeof(ObjectAttribute));
	relation->relattrinfo->attrinfo =
		(ObjectAttribute *)palloc(relattrsnum * sizeof(ObjectAttribute));
	relattrfill(rel, relation, tup, OBJECT_FILL_REL);

	/*
	 * Form a scan key that selects only user attributes (attnum > 0).
	 * (Eliminating system attribute rows at the index level is lots faster
	 * than fetching them.)
	 */
	ScanKeyInit(&skey[0],
				Anum_pg_object_refclassid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(AttributeRelationId));
	ScanKeyInit(&skey[1],
				Anum_pg_object_obj_oid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(RelationGetRelid(relation)));
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

	/* Start scanning column information in the
	 * system table pg_object,If found, record
	 * it in the table cache.
	 */

	while (HeapTupleIsValid(pg_object_tuple = systable_getnext(pg_object_scan)))
	{
		relattrfill(rel, relation, pg_object_tuple, OBJECT_FILL_ATTR);
	}
	systable_endscan(pg_object_scan);

	table_close(rel, AccessShareLock);

}

/* relattrfill
 * Store the table information and column
 * information recorded in the pg_object
 * system table into the corresponding user
 * table cache.
 */
static void relattrfill(Relation objrel, Relation relation, HeapTuple tup, RelAttrFillOption opr)
{
	int				i;
	Datum			val;
	Name			sname;
	Form_pg_object	objecttp;
	bool			isNull;
	int				relattrsnum = 0;
	char		   *namestr = NULL;
	char			compatible_mode = 'p';

	relattrsnum = RelationGetNumberOfAttributes(relation);

	if (opr == OBJECT_FILL_REL)
	{
		/* Table information filling */
		val = heap_getattr(tup,
						   Anum_pg_object_objsubid,
						   RelationGetDescr(objrel),
						   &isNull);
		if (!isNull)
			relation->relattrinfo->relinfo->objSubid = DatumGetInt32(val);

		val =heap_getattr(tup,
						  Anum_pg_object_objname,
						  RelationGetDescr(objrel),
						  &isNull);
		if (!isNull)
		{
			namestr = NULL;
			sname = DatumGetName(val);
			namestr = pstrdup(NameStr(*sname));
			namestrcpy(&(relation->relattrinfo->relinfo->objname), namestr);
		}

		val = heap_getattr(tup,
						   Anum_pg_object_objkind,
						   RelationGetDescr(objrel),
						   &isNull);
		if (!isNull)
			relation->relattrinfo->relinfo->objkind = DatumGetChar(val);

		val = heap_getattr(tup,
						   Anum_pg_object_compatible_mode,
						   RelationGetDescr(objrel),
						   &isNull);
		if (!isNull)
			compatible_mode = DatumGetChar(val);

		relation->relattrinfo->relinfo->compatible_mode = compatible_mode;

		val = heap_getattr(tup,
						   Anum_pg_object_option,
						   RelationGetDescr(objrel),
						   &isNull);
		if (!isNull)
		{
			relation->relattrinfo->relinfo->optionval = val;
		}
		else
			relation->relattrinfo->relinfo->optionval = (Datum)0;

		/* Column information initialization */
		for (i = 0; i < relattrsnum; i++)
		{
			relation->relattrinfo->attrinfo[i].objSubid = relation->rd_att->attrs[i].attnum;
			namestrcpy(&(relation->relattrinfo->attrinfo[i].objname),
						pstrdup(NameStr(relation->rd_att->attrs[i].attname)));
			relation->relattrinfo->attrinfo[i].objkind = 'c';
			relation->relattrinfo->attrinfo[i].compatible_mode = compatible_mode;
			relation->relattrinfo->attrinfo[i].optionval = (Datum)0;
		}
		ReleaseSysCache(tup);
	}
	else if (opr == OBJECT_FILL_ATTR)
	{
		/* Column information filling */
		int			attnum;

		objecttp = (Form_pg_object) GETSTRUCT(tup);
		attnum = objecttp->objsubid;
		compatible_mode = 'p';

		/* The order scanned by pg_object may be
		 * inconsistent with the order scanned by
		 * pg_attribute. In order to be consistent
		 * with the order scanned by pg_attribute,
		 * do the following processing.
		 */
		for (i = 0; i < relattrsnum; i++)
		{
			if (relation->relattrinfo->attrinfo[i].objSubid == attnum) /* modify attr*/
			{
				val = heap_getattr(tup,
								   Anum_pg_object_compatible_mode,
								   RelationGetDescr(objrel),
								   &isNull);
				if (!isNull)
					compatible_mode = DatumGetChar(val);

				relation->relattrinfo->attrinfo[i].compatible_mode = compatible_mode;

				val = heap_getattr(tup,
								   Anum_pg_object_option,
								   RelationGetDescr(objrel),
								   &isNull);
				if (!isNull)
				{
					relation->relattrinfo->attrinfo[i].optionval = val;
					break;
				}
			}
		}
	}
	else
		return;
}

