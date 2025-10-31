/*-------------------------------------------------------------------------
 *
 * File: conn_restrict.h
 *
 * Abstract: support connect restrict, include mac address and logon time.
 *
 * Copyright:
 * Copyright (c) 2009-2023, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *	  		/contrib/conn_restrict/conn_restrict.h
 *
 *-------------------------------------------------------------------------
 */
/* Begin ReqID: SRS-CLIENT-AUTH */

#ifndef CONN_RESTRICT_H
#define CONN_RESTRICT_H

#include "postgres.h"

#include "access/htup_details.h"
#include "access/heapam.h"
#include "executor/spi.h"
#include "fmgr.h"
#include "funcapi.h"
#include "lib/stringinfo.h"
#include "miscadmin.h"

#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/rel.h"
#include "utils/inet.h"
#include "utils/lsyscache.h"
#include "utils/fmgroids.h"
#include "utils/restrict.h"
#if PG_VERSION_NUM >= 120000
#include "utils/snapmgr.h"
#else
#include "utils/tqual.h"
#endif

#include "access/table.h"
#include "access/skey.h"
#include "access/genam.h"

#include "catalog/pg_class.h"
#include "catalog/pg_namespace.h"
#include "catalog/pg_index.h"
#include "catalog/pg_extension.h"
#include "catalog/indexing.h"
#include "utils/varlena.h"
#include "catalog/pg_authid.h"
#include "utils/timestamp.h"
#include "se_features_interface.h"
#include "utils/restrict.h"

PG_MODULE_MAGIC;

/* define */
#define HGS_RULE_TABLE_NAME "conn_restrict_rules"
#define Natts_connect_restrict_rules 7

typedef enum
{
	rulemac_wmatch,
	rulemac_bmatch,
	rulemac_outmatch
} match_macflag;

typedef enum
{
	ruletime_wmatch,
	ruletime_bmatch,
	ruletime_outmatch
}
match_timeflag;

#endif
/* End ReqID: SRS-CLIENT-AUTH */
