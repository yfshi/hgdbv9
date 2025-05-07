/* Begin - ReqID:SRS-SECURITY-RESOURCEMANAGEMENT */
/*-------------------------------------------------------------------------
 *
 * resgroupcmds.h
 *	  Commands for manipulating resource group.
 *
 * Portions Copyright (c) 2006-2017, Greenplum inc.
 * Portions Copyright (c) 2012-Present VMware, Inc. or its affiliates.
 *
 * Authored by zhangyilin@highgo.com
 * Portions Copyright (c) 2009-2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 * 		src/include/commands/resgroupcmds.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef RESGROUPCMDS_H
#define RESGROUPCMDS_H

#include "nodes/parsenodes.h"
#include "storage/lock.h"
#include "utils/resgroup.h"
#include "utils/relcache.h"

extern void CreateResourceGroup(CreateResourceGroupStmt *stmt);
extern void DropResourceGroup(DropResourceGroupStmt *stmt);
extern void AlterResourceGroup(AlterResourceGroupStmt *stmt);

/* catalog access function */
extern Oid get_resgroup_oid(const char *name, bool missing_ok);
extern char *GetResGroupNameForId(Oid oid);
extern Oid GetResGroupIdForRole(Oid roleid);
extern Oid getResGroupIdByPgCapability(Oid roleid);
extern void GetResGroupCapabilities(Relation rel,
									Oid groupId,
									ResGroupCaps *resgroupCaps);
extern void ResGroupCheckForRole(Oid groupId);
extern bool createDefaultResgroupGroup(char *default_resgroupName, Oid dboid);
extern void dropDefaultResourceGroup(char *default_resgroupName);
extern Oid getResGroupIdByPgResgroup(char *default_resgroupName);

#endif   /* RESGROUPCMDS_H */
/* End - ReqID:SRS-SECURITY-RESOURCEMANAGEMENT */
