/*-------------------------------------------------------------------------
 *
 * File: hg_redaction.h
 *
 * Abstract: hg's support data redaction.
 *
 * Copyright:
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *			/contrib/hg_redaction/hg_redaction.h
 *
 * ReqID:SRS-SECURITY-DATADESENSITIZATION
 *
 *-------------------------------------------------------------------------
 */

#ifndef HG_REDACTION_H
#define HG_REDACTION_H

#include "postgres.h"
#include "se_features_interface.h"

#include "access/genam.h"
#include "access/heapam.h"
#include "access/htup_details.h"
#include "access/table.h"
#include "access/tableam.h"
#include "access/relation.h"
#include "access/reloptions.h"
#include "access/relscan.h"
#include "access/skey.h"
#include "catalog/catalog.h"
#include "catalog/dependency.h"
#include "tcop/utility.h"
#include "catalog/indexing.h"
#include "catalog/namespace.h"
#include "catalog/pg_namespace.h"
#include "catalog/pg_class.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_type.h"
#include "catalog/objectaccess.h"
#include "catalog/pg_authid.h"
#include "catalog/dependency.h"
#include "miscadmin.h"
#include "nodes/primnodes.h"
#include "parser/parse_node.h"
#include "parser/parse_coerce.h"
#include "parser/parsetree.h"
#include "utils/acl.h"
#include "utils/catcache.h"
#include "utils/fmgroids.h"
#include "utils/fmgrprotos.h"
#include "utils/lsyscache.h"
#include "utils/rel.h"
#include "utils/syscache.h"
#include "nodes/parsenodes.h"
#include "parser/parse_node.h"
#include "parser/analyze.h"
#include "libpq/libpq-be.h"
#include "catalog/pg_operator.h"
#include "utils/snapmgr.h"
#include "utils/builtins.h"
#include "utils/varlena.h"
#include "catalog/objectaddress.h"


#define CATALOG_RELID		10
#define VIEW_OR_MATVIEW_RELID		11
#define NO_FOUND_RTE_OR_COLUMN		12


#define BUFFSIZE		256
#define GETNEWOID_LOG_THRESHOLD 1000000
#define GETNEWOID_LOG_MAX_INTERVAL 128000000


typedef struct RedactionOptions
{
	int32		vl_len_;		/* varlena header (do not touch directly!) */
	int			application_offset;
	int			user_offset;
	int			session_user_offset;
	int			ip_offset;
} RedactionOptions;


/*
 *  hg_redaction_column tbale of data redaction, column position define 
 */
#define Anum_hg_redaction_column_polid 1
#define Anum_hg_redaction_column_polname 2
#define Anum_hg_redaction_column_tabrelid 3
#define Anum_hg_redaction_column_attname 4
#define Anum_hg_redaction_column_procid 5
#define Anum_hg_redaction_column_procname 6
#define Anum_hg_redaction_column_polscope 7
#define Anum_hg_redaction_column_polexception 8


#define Anum_hg_redaction_policy_polid 1
#define Anum_hg_redaction_policy_polname 2
#define Anum_hg_redaction_policy_tabrelid 3
#define Anum_hg_redaction_policy_schema 4
#define Anum_hg_redaction_policy_tabname 5
#define Anum_hg_redaction_policy_polpermissive 6
#define Anum_hg_redaction_policy_poloptions 7

#define Natts_hg_redaction_policy 7
#define Natts_hg_redaction_column 8

#define hg_redaciton_policy_oid_index "hg_redaciton_policy_oid_index"
#define hg_redaciton_table_oid_index "hg_redaciton_table_oid_index"
#define hg_redaciton_policy_tabrelid_polname_index "hg_redaciton_policy_tabrelid_polname_index"
#define hg_redaciton_column_polname_attname_index "hg_redaciton_column_polname_attname_index"
#define hg_redaciton_column_polid_index "hg_redaciton_column_polid_index"
#define hg_redaciton_column_tabrelid_attname_index "hg_redaciton_column_tabrelid_attname_index"

typedef struct VarParserValue
{
	Oid table_oid;
	char col_name[NAMEDATALEN];
} VarParserValue;

typedef enum{
	M_UNKNOWN,
	REDACT_ALL,
	REDACT_BANKCARD,
	REDACT_EMAILNAME,
	REDACT_EMAILFULL,
	REDACT_DIGITS,
	REDACT_SHUFFLE,
	REDACT_RANDOM,
	REDACT_IDCARD,
	REDACT_NAME,
	REDACT_PHONE,
	REDACT_CASH
}MaskBehaviour;

typedef struct MaskingFuncsInfo
{
	const char*		func;
	MaskBehaviour	type;
} MaskingFuncsInfo;

static const MaskingFuncsInfo masking_funcs_infos[] =
{
	{"redact_all", REDACT_ALL},
	{"redact_bankcard", REDACT_BANKCARD},
	{"redact_emailname", REDACT_EMAILNAME},
	{"redact_emailfull", REDACT_EMAILFULL},
	{"redact_digits", REDACT_DIGITS},
	{"redact_shuffle", REDACT_SHUFFLE},
	{"redact_random", REDACT_RANDOM},
	{"redact_idcard", REDACT_IDCARD},
	{"redact_name", REDACT_NAME},
	{"redact_phone", REDACT_PHONE},
	{"redact_cash", REDACT_CASH},
	{NULL, M_UNKNOWN}
};

extern post_parse_analyze_hook_type hg_prev_redaction_parse_analyze;
extern ProcessUtilityRedaction_hook_type hg_prev_redaction_ProcessUtility;
extern Get_object_description_hook_type hg_prev_Objectclassget;

#define COPY_REDACT_SCALAR_FIELD(fldname) \
	(newnode->fldname = from->fldname)

/* Copy a field that is a pointer to some kind of Node or Node tree */
#define COPY_REDACT_NODE_FIELD(fldname) \
	(newnode->fldname = copyObjectImpl(from->fldname))

/* Copy a field that is a pointer to a C string, or perhaps NULL */
#define COPY_REDACT_STRING_FIELD(fldname) \
	(newnode->fldname = from->fldname ? pstrdup(from->fldname) : (char *) NULL)


extern bool handle_redaction_data(ParseState *pstate, Query *query);
extern bool handle_redaction_data(ParseState *pstate, Query *query);
extern Oid get_policytb_oid(void);
extern Oid get_policycoltb_oid(void);
extern Oid get_policytbindex_oid(char *indexschema, char *indexneam);
extern bool hg_redact_ProcessUtility(PlannedStmt *pstmt, const char *queryString,
									 bool readOnlyTree,
									 ProcessUtilityContext context,
									 ParamListInfo params, QueryEnvironment *queryEnv,
									 DestReceiver *dest, QueryCompletion *qc);
extern void hg_handle_redaction_parse_analyze(ParseState *pstate, Query *query, JumbleState *jstate);
extern bool check_redaction_func(Oid functionId, List *funcargs);
extern int get_function_behavious(const char *func_name);
extern List *redaction_func_args(char *funcname);
extern char * redaction_getObjectDescription(const ObjectAddress *object);
extern bool redaction_do_deletion(const ObjectAddress *object);
extern void RemoveRedactionPolicyById(Oid policy_id);
extern char *Redacton_policyoid_get_policyname(Oid policy_id, Oid reloid, char *attname);
extern bool Redaction_AlterObjectNamespace_oid(const ObjectAddress *object);
extern char *Redaction_GetObjectTypeDescription(const ObjectAddress *object);
extern char *Redaction_getObjectIdentityParts(const ObjectAddress *object, List **objname);
extern void Redaction_ATExecAlterTable(Oid tableoid, char *colName, int type);
extern List *redatcion_untransformRelOptions(Datum options);
extern void Redaction_AlterRelationNamespaceInternal(Oid relOid, Oid oldNspOid, Oid newNspOid);
extern Oid get_procid_from_procname(char *procname);
extern bool judge_redaction_tb(Oid oid);
extern void redactionTransformDelTargetList(ParseState *pstate, Query *query);
#endif

