/* ------------------------------------------------ 
* 
* File: pg_secmon_threshold.h
*
* Abstract: 
* Definition of Security Monitoring Threshold
* Configuration Item System Table
*
* Authored by wanghaoyan@highgo.com 20250206.
*
* Copyright:
* Copyright (c) 2009-2025, HighGo Software Co.,Ltd. 
* All rights reserved .   
*
* Identification:
*	   src/include/catalog/pg_secmon_threshold.h  
*-------------------------------------------------
*/	

#ifndef PG_SECMON_THRESHOLD_H
#define PG_SECMON_THRESHOLD_H

#include "catalog/genbki.h"
#include "catalog/pg_secmon_threshold_d.h"

/* ----------------
 *		pg_audit_config definition.  cpp turns this into
 *		typedef struct FormData_pg_secmon_threshold
 * ----------------
 */
CATALOG(pg_secmon_threshold,6235,AuditThresholdRelationId)
{
	Oid			throid;
	NameData	thrname;
	Oid			ruleoid;
	int32		interval;
	int32		level;
	bool		enable;
	int32		maxtimes;
	int32		nsteps;
	NameData	basename BKI_FORCE_NULL;
	NameData	basevalue BKI_FORCE_NULL;

	/* variable-length fields start here, but we allow direct access to indkey */
	oidvector	ruleseq BKI_LOOKUP(pg_type) BKI_FORCE_NOT_NULL;
#ifdef CATALOG_VARLEN
	int2vector	impt_columns;
	timestamptz starttime;
	timestamptz impt_starttime;
	timestamptz impt_endtime;
#endif
	
} FormData_pg_secmon_threshold;


/* ----------------
 *		FormData_pg_secmon_rule corresponds to a pointer to a tuple with
 *		the format of pg_secmon_threshold relation.
 * ----------------
 */
typedef FormData_pg_secmon_threshold *Form_pg_secmon_threshold;

DECLARE_UNIQUE_INDEX(pg_secmon_threshold_throid_index, 6236, on pg_secmon_threshold using btree(throid oid_ops));
#define AuditThresholdIndexId 6236
DECLARE_UNIQUE_INDEX(pg_secmon_threshold_ruleoid_thrname_ruleseq_index, 6237, on pg_secmon_threshold using btree(ruleoid oid_ops, thrname name_ops, ruleseq oidvector_ops));
#define AuditRuleoidThrnameRuleseqIndexId 6237

#endif							/* PG_SECMON_THRESHOLD_H */

