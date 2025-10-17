/*-------------------------------------------------------------------------
 *
 * dml.c
 *	  Routines to handle mandatory access control
 *
 * Authored by wangjie@highgo.com, 20250917.
 *
 * Copyright:
 * Copyright (c) 2009-2025, HighGo Software Co.,Ltd. All right reserved.
 *
 * IDENTIFICATION
 *	  contrib/hg_sysmac/dml.c
 *
 *-------------------------------------------------------------------------
 */

/* Begin - ReqID: SRS-SECURITY-LMAC */

#include "postgres.h"

#include "access/htup_details.h"
#include "access/sysattr.h"
#include "access/tupdesc.h"
#include "catalog/pg_authid.h"
#include "catalog/catalog.h"
#include "catalog/dependency.h"
#include "catalog/heap.h"
#include "catalog/pg_attribute.h"
#include "catalog/pg_class.h"
#include "catalog/pg_inherits.h"
#include "catalog/pg_seclabel.h"
#include "catalog/pg_shseclabel.h"
#include "commands/seclabel.h"
#include "commands/tablecmds.h"
#include "executor/executor.h"
#include "miscadmin.h"
#include "nodes/bitmapset.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"

#include "hg_sysmac.h"
#include "se_features_interface.h"


/*
 * fixup_whole_row_references
 *
 * When user references a whole-row Var, it is equivalent to referencing
 * all the user columns (not system columns). So, we need to fix up the
 * given bitmapset, if it contains a whole-row reference.
 */
static Bitmapset *
fixup_whole_row_references(Oid relOid, Bitmapset *columns)
{
	Bitmapset  *result;
	HeapTuple	tuple;
	AttrNumber	natts;
	AttrNumber	attno;
	int			index;

	/* if no whole-row references, nothing to do */
	index = InvalidAttrNumber - FirstLowInvalidHeapAttributeNumber;
	if (!bms_is_member(index, columns))
		return columns;

	/* obtain number of attributes */
	tuple = SearchSysCache1(RELOID, ObjectIdGetDatum(relOid));
	if (!HeapTupleIsValid(tuple))
		elog(ERROR, "cache lookup failed for relation %u", relOid);
	natts = ((Form_pg_class) GETSTRUCT(tuple))->relnatts;
	ReleaseSysCache(tuple);

	/* remove bit 0 from column set, add in all the non-dropped columns */
	result = bms_copy(columns);
	result = bms_del_member(result, index);

	for (attno = 1; attno <= natts; attno++)
	{
		tuple = SearchSysCache2(ATTNUM,
								ObjectIdGetDatum(relOid),
								Int16GetDatum(attno));
		if (!HeapTupleIsValid(tuple))
			continue;			/* unexpected case, should we error? */

		if (!((Form_pg_attribute) GETSTRUCT(tuple))->attisdropped)
		{
			index = attno - FirstLowInvalidHeapAttributeNumber;
			result = bms_add_member(result, index);
		}

		ReleaseSysCache(tuple);
	}
	return result;
}

/*
 * fixup_inherited_columns
 *
 * When user is querying on a table with children, it implicitly accesses
 * child tables also. So, we also need to check security label of child
 * tables and columns, but here is no guarantee attribute numbers are
 * same between the parent and children.
 * It returns a bitmapset which contains attribute number of the child
 * table based on the given bitmapset of the parent.
 */
static Bitmapset *
fixup_inherited_columns(Oid parentId, Oid childId, Bitmapset *columns)
{
	Bitmapset  *result = NULL;
	int			index;

	/*
	 * obviously, no need to do anything here
	 */
	if (parentId == childId)
		return columns;

	index = -1;
	while ((index = bms_next_member(columns, index)) >= 0)
	{
		/* bit numbers are offset by FirstLowInvalidHeapAttributeNumber */
		AttrNumber	attno = index + FirstLowInvalidHeapAttributeNumber;
		char	   *attname;

		/*
		 * whole-row-reference shall be fixed-up later
		 */
		if (attno == InvalidAttrNumber)
		{
			result = bms_add_member(result, index);
			continue;
		}

		attname = get_attname(parentId, attno, false);
		attno = get_attnum(childId, attname);
		if (attno == InvalidAttrNumber)
			elog(ERROR, "cache lookup failed for attribute %s of relation %u",
				 attname, childId);

		result = bms_add_member(result,
								attno - FirstLowInvalidHeapAttributeNumber);

		pfree(attname);
	}

	return result;
}


/*
 * check_relation_privileges
 *
 * It actually checks required permissions on a certain relation
 * and its columns.
 *
 * (MAC) table-level rules:
 *    - SELECT: user_seclabel >= table_seclabel
 *    - UPDATE: user_seclabel = table_seclabel
 *    - DELETE: user_seclabel = table_seclabel
 *    - INSERT: user_seclabel <= table_seclabel
 *
 * (MAC) column-level rules:
 *    - SELECT: user_seclabel >= column_seclabel
 *    - UPDATE: user_seclabel = column_seclabel
 *    - INSERT: user_seclabel <= column_seclabel
 */
static bool
check_relation_privileges(Oid relOid,
						  Bitmapset *selected,
						  Bitmapset *inserted,
						  Bitmapset *updated,
						  uint32 required,
						  bool abort_on_violation)
{
	ObjectAddress	object;
	char		*audit_name;
	char		*raw_label_s;
	char		*raw_label_o;
	SecLabel	*seclabel_s = NULL;
	SecLabel	*seclabel_o = NULL;
	Bitmapset	*columns;
	int			index;
	bool		result = true;
	char		relkind = get_rel_relkind(relOid);
	volatile hg_sysmac_param *hg_sysmac = (volatile hg_sysmac_param *) hg_sysmac_g;

	if (hg_sysmac == NULL)
	{
		elog(WARNING, "The 'hg_sysmac' plugin was not loaded correctly. Please check the configuration.");
		return true;
	}

	/* if global is off, do nothing */
	if (!hg_sysmac->global_on)
		return true;

	if ((raw_label_s = get_obj_seclabel_internal(AuthIdRelationId, GetUserId(), 0,true)))
		seclabel_s = string_to_seclabel(raw_label_s, NULL, true);
	if ((raw_label_o = get_obj_seclabel_internal(RelationRelationId, relOid, 0,true)))
		seclabel_o = string_to_seclabel(raw_label_o, NULL, true);
	else
		return true;/*fast path if no label on target*/

	/*
	 * Check type
	 */
	switch (relkind)
	{
		case RELKIND_RELATION:
		case RELKIND_PARTITIONED_TABLE:
		case RELKIND_SEQUENCE:
		case RELKIND_VIEW:
		/*supported type*/
			break;

		default:
			elog(ERROR, "relation type %c not supported", relkind);
			break;
	}

	if (hg_sysmac->global_table_level)
	{
		if (required & ACL_SELECT)
		{
			if (seclabel_compare(seclabel_s, seclabel_o) == LABEL_UNCOMPARABLE)
			{
				result = false;
				if (abort_on_violation)
				{
					if (seclabel_s)
						pfree(seclabel_s);
					if (seclabel_o)
						pfree(seclabel_o);
					elog(ERROR, "permission deined: violate select rules on %s",
								 get_rel_name(relOid));
				}
			}
		}
		if (required & ACL_UPDATE)
		{
			if (seclabel_compare(seclabel_s, seclabel_o) != LABEL_EQUAL)
			{
				result = false;
				if (abort_on_violation)
				{
					if (seclabel_s)
						pfree(seclabel_s);
					if (seclabel_o)
						pfree(seclabel_o);
					elog(ERROR, "permission deined: violate update rules on %s",
								 get_rel_name(relOid));
				}
			}
		}
		if (required & ACL_INSERT)
		{
			if (seclabel_compare(seclabel_o, seclabel_s) == LABEL_UNCOMPARABLE)
			{
				result = false;
				if (abort_on_violation)
				{
					if (seclabel_s)
						pfree(seclabel_s);
					if (seclabel_o)
						pfree(seclabel_o);
					elog(ERROR, "permission deined: violate insert rules on %s",
								 get_rel_name(relOid));
				}
			}
		}
		if (required & ACL_DELETE)
		{
			if (seclabel_compare(seclabel_s, seclabel_o) != LABEL_EQUAL)
			{
				result = false;
				if (abort_on_violation)
				{
					if (seclabel_s)
						pfree(seclabel_s);
					if (seclabel_o)
						pfree(seclabel_o);
					elog(ERROR, "permission deined: violate delete rules on %s",
								 get_rel_name(relOid));
				}
			}
		}

		/*do column level*/
		if (hg_sysmac->global_column_level)
		{
			/*
			* Only columns owned by relations shall be checked
			*/
			if (relkind != RELKIND_RELATION && relkind != RELKIND_PARTITIONED_TABLE)
				return true;

			/*
			* Check permissions on the columns
			*/
			selected = fixup_whole_row_references(relOid, selected);
			inserted = fixup_whole_row_references(relOid, inserted);
			updated = fixup_whole_row_references(relOid, updated);
			columns = bms_union(selected, bms_union(inserted, updated));

			while ((index = bms_first_member(columns)) >= 0)
			{
				char *raw_lable_o_col;
				SecLabel *seclabel_o_col;
				AttrNumber	attnum;

				attnum = index + FirstLowInvalidHeapAttributeNumber;

				raw_lable_o_col = get_obj_seclabel_internal(RelationRelationId, relOid, attnum, true);

				/*fast path if not label on target column*/
				if (!raw_lable_o_col)
					continue;
				else
					seclabel_o_col = string_to_seclabel(raw_lable_o_col, NULL, true);

				object.classId = RelationRelationId;
				object.objectId = relOid;
				object.objectSubId = attnum;
				audit_name = getObjectDescription(&object, false);

				if (bms_is_member(index, selected))
				{
					/* focus only on column label */
					if (seclabel_compare(seclabel_s, seclabel_o_col) == LABEL_UNCOMPARABLE)
					{
						result = false;
						if (abort_on_violation)
						{
							if (seclabel_s)
								pfree(seclabel_s);
							if (seclabel_o)
								pfree(seclabel_o);
							elog(ERROR, "permission deined: violate select rules on %s",
										 audit_name);
						}
					}
				}
				if (bms_is_member(index, inserted))
				{
					if (seclabel_compare(seclabel_o_col, seclabel_s) == LABEL_UNCOMPARABLE)
					{
						result = false;
						if (abort_on_violation)
						{
							if (seclabel_s)
								pfree(seclabel_s);
							if (seclabel_o)
								pfree(seclabel_o);
							elog(ERROR, "permission deined: violate insert rules on %s",
										 audit_name);
						}
					}
				}
				if (bms_is_member(index, updated))
				{
					if (seclabel_compare(seclabel_s, seclabel_o_col) != LABEL_EQUAL)
					{
						result = false;
						if (abort_on_violation)
						{
							if (seclabel_s)
								pfree(seclabel_s);
							if (seclabel_o)
								pfree(seclabel_o);
							elog(ERROR, "permission deined: violate update rules on %s",
										 audit_name);
						}
					}
				}

				pfree(audit_name);
				bms_free(seclabel_o_col->catagory);
				pfree(seclabel_o_col);
			}
		}
	}

	if (seclabel_s)
		pfree(seclabel_s);
	if (seclabel_o)
		pfree(seclabel_o);

	return result;
}

/*
 * Enforce table-level or column-level mandatory access
 * control on the table being queried.
 */
bool
sysmac_dml_privileges(List *rangeTabls, bool abort_on_violation)
{
	ListCell   *lr;
	volatile hg_sysmac_param *hg_sysmac = (volatile hg_sysmac_param *) hg_sysmac_g;

	if (hg_sysmac == NULL)
	{
		elog(WARNING, "The 'hg_sysmac' plugin was not loaded correctly. Please check the configuration.");
		return true;
	}

	/* if global is off, do nothing */
	if (!hg_sysmac->global_on)
		return true;

	foreach(lr, rangeTabls)
	{
		RangeTblEntry *rte = lfirst(lr);
		uint32		required = rte->requiredPerms;
		List	   *tableIds;
		ListCell   *li;

		/* Only regular relations shall be checked */
		if (rte->rtekind != RTE_RELATION)
			continue;

		/* Skip, if nothing to be checked */
		if (rte->requiredPerms == 0)
			continue;

		/* skip all relations created before first start */
		if (rte->relid < FirstNormalObjectId)
		{
			if (rte->relid == SecLabelRelationId || rte->relid == SharedSecLabelRelationId)
			{
				if (!IS_SYSSSO(GetUserId()))
					ereport(ERROR,
							(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
							errmsg("permission denied"),
							errdetail("Only member of syssso_role can query seclabel.")));
			}
			continue;
		}

		/*
		 * If this RangeTblEntry is also supposed to reference inherited
		 * tables, we need to check security label of the child tables. So, we
		 * expand rte->relid into list of OIDs of inheritance hierarchy, then
		 * checker routine will be invoked for each relations.
		 */
		if (!rte->inh)
			tableIds = list_make1_oid(rte->relid);
		else
			tableIds = find_all_inheritors(rte->relid, NoLock, NULL);

		foreach(li, tableIds)
		{
			Oid			tableOid = lfirst_oid(li);
			Bitmapset  *selectedCols;
			Bitmapset  *insertedCols;
			Bitmapset  *updatedCols;

			/*
			 * child table has different attribute numbers, so we need to fix
			 * up them.
			 */
			selectedCols = fixup_inherited_columns(rte->relid, tableOid,
												   rte->selectedCols);
			insertedCols = fixup_inherited_columns(rte->relid, tableOid,
												   rte->insertedCols);
			updatedCols = fixup_inherited_columns(rte->relid, tableOid,
												  rte->updatedCols);

			/*
			 * check permissions on individual tables
			 */
			if (!check_relation_privileges(tableOid,
										   selectedCols,
										   insertedCols,
										   updatedCols,
										   required, abort_on_violation))
				return false;
		}
		list_free(tableIds);
	}
	return true;
}

/* End - ReqID: SRS-SECURITY-LMAC */
