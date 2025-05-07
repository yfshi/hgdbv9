/* Begin - ReqID:SRS-SECURITY-RESOURCEMANAGEMENT */
/*-------------------------------------------------------------------------
 *
 * cgroup-ops-v1.h
 *	  GPDB resource group definitions.
 *
 * Copyright (c) 2017 VMware, Inc. or its affiliates.
 *
 *
 * Authored by zhangyilin@highgo.com
 * Portions Copyright (c) 2009-2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	    src/include/utils/cgroup-ops-v1.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef RES_GROUP_OPS_V1_H
#define RES_GROUP_OPS_V1_H

#include "utils/cgroup.h"

extern CGroupOpsRoutine *get_group_routine_v1(void);
extern CGroupSystemInfo *get_cgroup_sysinfo_v1(void);

#endif   /* RES_GROUP_OPS_V1_H */
/* End - ReqID:SRS-SECURITY-RESOURCEMANAGEMENT */
