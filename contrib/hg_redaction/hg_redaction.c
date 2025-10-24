/*-------------------------------------------------------------------------
 *
 * File: hg_redaction.c
 *
 * Abstract: support data redaction.
 *
 * Copyright:
 * Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *			/contrib/hg_redaction/hg_redaction.c
 *
 * ReqID:SRS-SECURITY-DATADESENSITIZATION
 *
 *-------------------------------------------------------------------------
 */
#include "hg_redaction.h"
#include "tcop/utility.h"

PG_MODULE_MAGIC;

post_parse_analyze_hook_type hg_prev_redaction_parse_analyze = NULL;
ProcessUtilityRedaction_hook_type hg_prev_redaction_ProcessUtility = false;

/*---- Function declarations ----*/
void		_PG_init(void);
void		_PG_fini(void);

static CreateRedactionPolicyStmt *
_copyCreateRedactionPolicystmt(const CreateRedactionPolicyStmt *from);
static  AlterRedactionPolicyStmt *
_copyAlterRedactionPolicystmt(const AlterRedactionPolicyStmt *from);
static DropRedactionPolicyStmt *
_copyDropRedactionPolicystmt(const DropRedactionPolicyStmt *from);
static void * Redaction_pbeop(const void *from);
static RedactionPolicyColumnStmt *
_copyRedactionPolicyColumnStmt(const RedactionPolicyColumnStmt *from);


/*
 * Module load callback
 */
void
_PG_init(void)
{
	hg_prev_redaction_parse_analyze = post_parse_analyze_hook;
	post_parse_analyze_hook = hg_handle_redaction_parse_analyze;
	redact_parse_analyze_hook = redactionTransformDelTargetList;

	hg_prev_redaction_ProcessUtility = ProcessrdUtility_hook;
	ProcessrdUtility_hook = hg_redact_ProcessUtility;

	/* there is support to alter table */
	get_object_description_hook = redaction_getObjectDescription;
	do_deletion_hook = redaction_do_deletion;
	alterobjectnamespaceoid_hook = Redaction_AlterObjectNamespace_oid;
	getobjecttypedescription_hook = Redaction_GetObjectTypeDescription;
	getobjectidentityparts_hook = Redaction_getObjectIdentityParts;
	atexecaltercolumntype_hook_type_hook = Redaction_ATExecAlterTable;
	get_object_impl_hook = Redaction_pbeop;
	alterrelationnamespaceinternal_hook = Redaction_AlterRelationNamespaceInternal;
	do_drop_table_redaction = judge_redaction_tb;
}

/*
 * Module unload callback
 */
void
_PG_fini(void)
{
	post_parse_analyze_hook = hg_prev_redaction_parse_analyze;
	ProcessrdUtility_hook = hg_prev_redaction_ProcessUtility;
}

/*
 * Redaction_pbeop -- implementation of copyObject(); 
 * see nodes/nodes.h
 *
 * Create a copy of a Node tree or list.  This is 
 * a "deep" copy: all substructure is copied too,
 * recursively, there support redaction policy create
 * alter and drop.RedactionPolicyColumnStmt main function
 * is record relevant information.
 */
static void *
Redaction_pbeop(const void *from)
{
	void	   *retval = NULL;

	if (from == NULL)
		return NULL;

	switch (nodeTag(from))
	{
		case T_CreateRedactionPolicyStmt:
		{
				retval = _copyCreateRedactionPolicystmt(from);
				break;
		}
		case T_AlterRedactionPolicyStmt:
		{
				retval = _copyAlterRedactionPolicystmt(from);
				break;
		}
		case T_DropRedactionPolicyStmt:
		{
				retval = _copyDropRedactionPolicystmt(from);
				break;
		}
		case T_RedactionPolicyColumnStmt:
		{
			retval = _copyRedactionPolicyColumnStmt(from);
			break;
		}
		default:
			break;
	}

	return retval;
}

/*
 * _copyCreateRedactionPolicystmt
 *
 *
 * NOTE: copy create redaction policy information.
 */
static CreateRedactionPolicyStmt *
_copyCreateRedactionPolicystmt(const CreateRedactionPolicyStmt *from)
{
	CreateRedactionPolicyStmt *newnode = makeNode(CreateRedactionPolicyStmt);

	COPY_REDACT_STRING_FIELD(policy_name);
	COPY_REDACT_NODE_FIELD(table_name);
	COPY_REDACT_NODE_FIELD(add_columns);
	COPY_REDACT_NODE_FIELD(options);
	COPY_REDACT_SCALAR_FIELD(policy_enabled);
	return newnode;
}

/*
 * _copyAlterRedactionPolicystmt
 *
 *
 * NOTE: copy alter redaction policy information.
 */
static  AlterRedactionPolicyStmt *
_copyAlterRedactionPolicystmt(const AlterRedactionPolicyStmt *from)
{
	AlterRedactionPolicyStmt *newnode = makeNode(AlterRedactionPolicyStmt);

	COPY_REDACT_SCALAR_FIELD(stmt_type);
	COPY_REDACT_STRING_FIELD(policy_name);
	COPY_REDACT_STRING_FIELD(policy_newname);
	COPY_REDACT_NODE_FIELD(table_name);
	COPY_REDACT_NODE_FIELD(options);
	COPY_REDACT_NODE_FIELD(colname_list);
	COPY_REDACT_NODE_FIELD(add_columns);
	COPY_REDACT_STRING_FIELD(procname);
	COPY_REDACT_SCALAR_FIELD(policy_enabled);
	return newnode;
}

/*
 * _copyDropRedactionPolicystmt
 *
 *
 * NOTE: copy drop redaction policy information.
 */
static DropRedactionPolicyStmt *
_copyDropRedactionPolicystmt(const DropRedactionPolicyStmt *from)
{
	DropRedactionPolicyStmt *newnode = makeNode(DropRedactionPolicyStmt);

	COPY_REDACT_SCALAR_FIELD(if_exists);
	COPY_REDACT_NODE_FIELD(policyname_list);
	COPY_REDACT_NODE_FIELD(table_name);
	return newnode;
}

/*
 * _copyRedactionPolicyColumnStmt
 *
 *
 * NOTE: copy redaction policy column information.
 */
static RedactionPolicyColumnStmt *
_copyRedactionPolicyColumnStmt(const RedactionPolicyColumnStmt *from)
{
	RedactionPolicyColumnStmt *newnode = makeNode(RedactionPolicyColumnStmt);

	COPY_REDACT_NODE_FIELD(colname_list);
	COPY_REDACT_SCALAR_FIELD(optype);
	COPY_REDACT_NODE_FIELD(withops_list);
	return newnode;
}
