/* ------------------------------------------------ 
* 
* File: pg_secmon_rule.h
*
* Abstract: 
* Definition of Security Monitoring Configuration 
* Item System Table
*
* Authored by wanghaoyan@highgo.com 20250206.
*
* Copyright:
* Copyright (c) 2009-2025, HighGo Software Co.,Ltd. 
* All rights reserved .   
*
* Identification:
*	   src/include/catalog/pg_secmon_rule.h  
*-------------------------------------------------
*/	

#ifndef PG_SECMON_RULE_H
#define PG_SECMON_RULE_H

#include "catalog/genbki.h"
#include "catalog/pg_secmon_rule_d.h"

/* ----------------
 *		pg_audit_config definition.  cpp turns this into
 *		typedef struct FormData_pg_secmon_rule
 * ----------------
 */
CATALOG(pg_secmon_rule,6232,AuditRuleRelationId)
{
	Oid			ruleoid;
	NameData	rulename;
	Oid			confid;
	char		monmode;
	bool		enable;
		
} FormData_pg_secmon_rule;


/* ----------------
 *		FormData_pg_secmon_rule corresponds to a pointer to a tuple with
 *		the format of pg_secmon_rule relation.
 * ----------------
 */
typedef FormData_pg_secmon_rule *Form_pg_secmon_rule;

DECLARE_UNIQUE_INDEX(pg_secmon_rule_ruleoid_index, 6233, on pg_secmon_rule using btree(ruleoid oid_ops));
#define AuditRuleIndexId 6233
DECLARE_UNIQUE_INDEX(pg_secmon_rule_confid_rulename_index, 6234, on pg_secmon_rule using btree(confid oid_ops, rulename name_ops));
#define AuditConfidRulenameIndexId 6234

#endif							/* PG_SECMON_RULE_H */

