/* ------------------------------------------------ 
* 
* File: pg_audit_config.h
*
* Abstract: 
* Definition of audit configuration item system table
*
* Authored by wanghaoyan@highgo.com 20240401.
*
* Copyright:
* Copyright (c) 2009-2025, HighGo Software Co.,Ltd. 
* All rights reserved .   
*
* Identification:
*	   src/include/catalog/pg_audit_config.h  
*-------------------------------------------------
*/	

#ifndef PG_AUDIT_CONFIG_H
#define PG_AUDIT_CONFIG_H

#include "catalog/genbki.h"
#include "catalog/pg_audit_config_d.h"

/* ----------------
 *		pg_audit_config definition.  cpp turns this into
 *		typedef struct FormData_pg_audit_config
 * ----------------
 */
CATALOG(pg_audit_config,6229,AuditConfigRelationId)
{
	Oid			confid;
	NameData	actionname;
	Oid			auditoroid;
	NameData	objoid;
	Oid			classoid;
	NameData	objtype;
	char		audittype;
	char		auditmode;
	bool		enable;
		
} FormData_pg_audit_config;


/* ----------------
 *		Form_pg_audit_config corresponds to a pointer to a tuple with
 *		the format of pg_audit_config relation.
 * ----------------
 */
typedef FormData_pg_audit_config *Form_pg_audit_config;

DECLARE_UNIQUE_INDEX(pg_audit_config_confid_index, 6230, on pg_audit_config using btree(confid oid_ops));
#define AuditConfigconfIdIndexId 6230
DECLARE_UNIQUE_INDEX(pg_audit_config_actionname_objoid_classoid_type_index, 6231, on pg_audit_config using btree(actionname name_ops, objoid name_ops, classoid oid_ops, audittype char_ops));
#define AuditConfigActionnameAudittypeIndexId 6231
DECLARE_UNIQUE_INDEX(pg_audit_config_enable_index, 6238, on pg_audit_config using btree(enable bool_ops, confid oid_ops));
#define AuditEnableIdIndexId 6238

#endif							/* PG_AUDIT_CONFIG_H */

