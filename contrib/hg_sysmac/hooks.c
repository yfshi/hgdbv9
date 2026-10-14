/*-------------------------------------------------------------------------
 *
 * hooks.c
 *	  Routines to handle mandatory access control
 *
 * Authored by wangjie@highgo.com, 20250917.
 *
 * Copyright:
 * Copyright (c) 2009-2025, HighGo Software Co.,Ltd. All right reserved.
 *
 * IDENTIFICATION
 *	  contrib/hg_sysmac/hooks.c
 *
 *-------------------------------------------------------------------------
 */

/* Begin - ReqID: SRS-SECURITY-LMAC */

#include "postgres.h"

#include "catalog/pg_authid.h"
#include "catalog/pg_class.h"
#include "catalog/objectaccess.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_seclabel.h"
#include "catalog/pg_shseclabel.h"
#include "catalog/namespace.h"
#include "commands/seclabel.h"
#include "commands/sequence.h"
#include "executor/executor.h"
#include "parser/parse_func.h"
#include "miscadmin.h"
#include "parser/parse_func.h"
#include "storage/ipc.h"
#include "storage/shmem.h"
#include "tcop/utility.h"
#include "utils/acl.h"
#include "utils/builtins.h"
#include "utils/catcache.h"
#include "utils/guc.h"
#include "utils/rls.h"
#include "utils/syscache.h"
#include "utils/lsyscache.h"

#include "hg_sysmac.h"
#include "se_features_interface.h"

PG_MODULE_MAGIC;

void		_PG_init(void);

/* Begin - bug#z5940 */
bool global_on = false;
bool global_table_level = false;
bool global_column_level = false;
bool global_row_level = true;
/* End - bug#z5940 */

/*
 * Saved hook entries (if stacked)
 */
static ExecutorCheckPerms_hook_type next_exec_check_perms_hook = NULL;
static ProcessUtility_hook_type next_ProcessUtility_hook = NULL;
static SequenceAccess_hook_type next_sequence_access_hook = NULL; 	/* Bug#z8537 */


static bool
is_sysmac_function(Oid funcoid)
{
	#define		PROCESSLIST_COLUMN_NUMS 7
	char	*funcname = NULL;
	int		i;
	Oid		nspoid;
	char	funclist[PROCESSLIST_COLUMN_NUMS][32] =
							{"get_user_seclabel",   "set_user_seclabel",
							 "get_table_seclabel",  "set_table_seclabel",
							 "get_column_seclabel", "set_column_seclabel",
							 "apply_row_policy"};

	nspoid = get_namespace_oid("hg_security", true);
	if (!OidIsValid(nspoid))
		return false;

	if (get_func_namespace(funcoid) != nspoid)
		return false;

	funcname = get_func_name(funcoid);
	if (!funcname)
		return false;

	for (i = 0; i < PROCESSLIST_COLUMN_NUMS; i++)
	{
		if (pg_strcasecmp(funcname, funclist[i]) == 0)
			return true;
	}

	return false;
}

static bool
sysmac_exec_check_perms(List *rangeTabls, bool abort)
{
	/*
	 * If security provider is stacking and one of them replied 'false' at
	 * least, we don't need to check any more.
	 */
	if (next_exec_check_perms_hook &&
		!(*next_exec_check_perms_hook) (rangeTabls, abort))
		return false;

	if (!sysmac_dml_privileges(rangeTabls, abort))
		return false;

	return true;
}

/*
 * sysmac_utility_command
 *
 * It tries to rough-grained control on utility commands; some of them can
 * break whole of the things if nefarious user would use.
 */
static void
sysmac_utility_command(PlannedStmt *pstmt,
						const char *queryString,
						bool readOnlyTree,
						ProcessUtilityContext context,
						ParamListInfo params,
						QueryEnvironment *queryEnv,
						DestReceiver *dest,
						QueryCompletion *qc)
{
	Node	   *parsetree = pstmt->utilityStmt;
	GrantStmt  *action;
	ListCell   *cell;

	if (next_ProcessUtility_hook)
		(*next_ProcessUtility_hook) (pstmt, queryString, readOnlyTree,
									 context, params, queryEnv,
									 dest, qc);
	else
		standard_ProcessUtility(pstmt, queryString, readOnlyTree,
								context, params, queryEnv,
								dest, qc);

	if (!IsUnderPostmaster || !global_on)
		return;

	switch (nodeTag(parsetree))
	{
		case T_GrantStmt:
			action = (GrantStmt *) parsetree;

			/*
			 * We do not allow granting permissions on hg_sysmac
			 * functions and tables to regular users.
			 */
			foreach(cell, action->grantees)
			{
				RoleSpec   *grantee = (RoleSpec *) lfirst(cell);
				Oid			grantee_uid;

				switch (grantee->roletype)
				{
					case ROLESPEC_PUBLIC:
						grantee_uid = ACL_ID_PUBLIC;
						break;
					default:
						grantee_uid = get_rolespec_oid(grantee, false);
						break;
				}

				if (!IS_SYSSSO(grantee_uid) && !superuser_arg(grantee_uid))
				{
					/* Begin - Bug#z9020 */
					if (action->objtype == OBJECT_TABLE)
					{
						/*
						 * Security Restriction - Granting ANY privilege on system catalog tables is strictly prohibited.
						 * - Do NOT grant ANY privilege (e.g., SELECT, INSERT, UPDATE, DELETE, ALL, etc.) on
						 *   ALL TABLES IN SCHEMA pg_catalog to any ordinary user.
						 * - Do NOT grant ANY privilege on the individual tables pg_seclabel or pg_shseclabel
						 *   to any ordinary user.
						 */
						if (action->targtype == ACL_TARGET_OBJECT)
						{
							foreach(cell, action->objects)
							{
								RangeVar   *relvar = (RangeVar *) lfirst(cell);
								Oid			relOid;

								relOid = RangeVarGetRelid(relvar, NoLock, true);
								if (relOid == SharedSecLabelRelationId ||
									relOid == SecLabelRelationId)
								{
									ereport(ERROR,
											(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
											errmsg("permission denied"),
											errdetail("Cannot grant sysmac-related `%s` to regular users.", relvar->relname)));
								}
							}
						}
						else if (action->targtype == ACL_TARGET_ALL_IN_SCHEMA)
						{
							foreach(cell, action->objects)
							{
								char	   *nspname = strVal(lfirst(cell));

								if (strcmp(nspname, "pg_catalog") == 0)
									ereport(ERROR,
											(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
											errmsg("permission denied"),
											errdetail("Cannot grant pg_catalog tables to regular users.")));
							}
						}
					}
					else if (action->objtype == OBJECT_FUNCTION)
					{
						/*
						 * Security Restriction - Granting EXECUTE privilege on security-critical functions is strictly prohibited.
						 * - Do NOT grant EXECUTE on ALL FUNCTIONS IN SCHEMA hg_security to any ordinary user.
						 * - Do NOT grant EXECUTE on any individual "hg_sysmac" functions within the hg_security
						 *   schema to any ordinary user.
						 */
						if (action->targtype == ACL_TARGET_OBJECT)
						{
							foreach(cell, action->objects)
							{
								ObjectWithArgs *func = (ObjectWithArgs *) lfirst(cell);
								Oid			funcid;

								funcid = LookupFuncWithArgs(OBJECT_FUNCTION, func, false);
								if (is_sysmac_function(funcid))
								{
									ereport(ERROR,
											(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
											errmsg("permission denied"),
											errdetail("Cannot grant sysmac-related functions to regular users.")));
								}
							}
						}
						else if (action->targtype == ACL_TARGET_ALL_IN_SCHEMA)
						{
							foreach(cell, action->objects)
							{
								char	   *nspname = strVal(lfirst(cell));

								if (strcmp(nspname, "hg_security") == 0)
									ereport(ERROR,
											(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
											errmsg("permission denied"),
											errdetail("Cannot grant sysmac-schema hg_security functions to regular users.")));
							}
						}
					}
					/* End - Bug#z9020 */
				}
			}
			break;

		default:
			break;
	}
}

/* Begin - Bug#z8537 */
static void
sysmac_sequence_access(Oid seqid, AclMode required)
{
	char	   *raw_label_s;
	char	   *raw_label_o;
	char	   *sequence_name;
	const char *access_name;
	SecLabel   *seclabel_s;
	SecLabel   *seclabel_o;
	bool		allowed = false;

	if (next_sequence_access_hook)
		next_sequence_access_hook(seqid, required);

	/* Sequence labels are governed by the existing table-level switch. */
	if (!global_on || !global_table_level)
		return;

	raw_label_o = get_obj_seclabel_internal(RelationRelationId, seqid, 0,
											true);
	/* Keep the existing policy convention: unlabeled objects are unrestricted. */
	if (raw_label_o == NULL)
		return;

	raw_label_s = get_obj_seclabel_internal(AuthIdRelationId, GetUserId(), 0,
											true);
	seclabel_s = raw_label_s == NULL ? NULL :
		string_to_seclabel(raw_label_s, NULL, true);
	seclabel_o = string_to_seclabel(raw_label_o, NULL, true);

	/* Advancing and explicitly setting a sequence both modify its state. */
	if (required & ACL_UPDATE)
	{
		access_name = "update";
		allowed = seclabel_s != NULL &&
			seclabel_compare(seclabel_s, seclabel_o) == LABEL_EQUAL;
	}
	/* currval() and lastval() only disclose the current sequence value. */
	else if (required & ACL_SELECT)
	{
		access_name = "select";
		allowed = seclabel_s != NULL &&
			seclabel_compare(seclabel_s, seclabel_o) != LABEL_UNCOMPARABLE;
	}
	else
	{
		free_converted_seclabel(seclabel_s);
		free_converted_seclabel(seclabel_o);
		if (raw_label_s != NULL)
			pfree(raw_label_s);
		pfree(raw_label_o);
		elog(ERROR, "unsupported sequence access mode: %u", required);
	}

	if (!allowed)
	{
		sequence_name = get_rel_name(seqid);
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("permission denied: violate %s rules on sequence %s",
						access_name,
						sequence_name ? sequence_name : "<unknown>")));
	}

	free_converted_seclabel(seclabel_s);
	free_converted_seclabel(seclabel_o);
	if (raw_label_s != NULL)
		pfree(raw_label_s);
	pfree(raw_label_o);
}
/* End - Bug#z8537 */

bool
is_security_roles()
{
	if (se_feature_mode) /* Bug - #5937 #z6015 #z6056 #z6058 */
	{
		if (!IS_SYSSSO(GetUserId()))
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					errmsg("permission denied"),
					errdetail("Only member of syssso_role can set and query seclabel.")));
	}
	else /* Begin - Bug #5937 #z6015 #z6056 #z6058 */
	{
		if (!superuser())
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					errmsg("permission denied"),
					errdetail("Only member of superuser can set and query seclabel.")));
	} /* End - Bug #5937 #z6015 #z6056 #z6058 */

	return true;
}

#ifdef ENABLE_COMPATIBLE_AK

/* Start obsolete GUC compatibility. */

/* Obsolete GUC compatibility placeholders. */
static char *obsolete_enable = NULL;
static char *obsolete_default_label = NULL;
static char *obsolete_labelmode = NULL;
static char *obsolete_label_impexp = NULL;

static bool
reject_obsolete_guc(GucSource source, const char *name, const char *hint)
{
	/* Registration validates the built-in default before accepting settings. */
	if (source == PGC_S_DEFAULT)
		return true;

	GUC_check_errmsg("parameter \"%s\" is no longer supported", name);
	GUC_check_errhint("%s", hint);
	return false;
}

static bool
check_obsolete_enable(char **newval, void **extra, GucSource source)
{
	return reject_obsolete_guc(source, "hg_sysmac.enable",
							   "This parameter has been renamed to "
							   "\"hg_sysmac.global_on\". Use the new name instead.");
}

static bool
check_obsolete_default_label(char **newval, void **extra, GucSource source)
{
	return reject_obsolete_guc(source, "hg_sysmac.default_label",
							   "This parameter has been removed. "
							   "Remove it from your configuration.");
}

static bool
check_obsolete_labelmode(char **newval, void **extra, GucSource source)
{
	return reject_obsolete_guc(source, "hg_sysmac.labelmode",
							   "This parameter has been replaced by "
							   "\"hg_sysmac.global_table_level\" and "
							   "\"hg_sysmac.global_row_level\". "
							   "Set the two parameters separately.");
}

static bool
check_obsolete_label_impexp(char **newval, void **extra, GucSource source)
{
	return reject_obsolete_guc(source, "hg_sysmac.label_impexp",
							   "This parameter has been removed. "
							   "Remove it from your configuration.");
}

static void
register_obsolete_guc(const char *name, char **storage,
					  GucStringCheckHook check_hook)
{
	/* Accept any input type for diagnostics, but reject explicit assignments. */
	DefineCustomStringVariable(name,
							   "Obsolete hg_sysmac parameter.",
							   NULL,
							   storage,
							   NULL,
							   PGC_USERSET,
							   GUC_NO_SHOW_ALL | GUC_NOT_IN_SAMPLE | GUC_NO_RESET_ALL,
							   check_hook,
							   NULL,
							   NULL);
}

static void
register_obsolete_gucs(void)
{
	register_obsolete_guc("hg_sysmac.enable", &obsolete_enable,
						  check_obsolete_enable);
	register_obsolete_guc("hg_sysmac.default_label", &obsolete_default_label,
						  check_obsolete_default_label);
	register_obsolete_guc("hg_sysmac.labelmode", &obsolete_labelmode,
						  check_obsolete_labelmode);
	register_obsolete_guc("hg_sysmac.label_impexp", &obsolete_label_impexp,
						  check_obsolete_label_impexp);
}
/* End obsolete GUC compatibility. */

/* Obsolete function diagnostics. */
static undefined_function_hook_type prev_undefined_function_hook = NULL;

static void
sysmac_undefined_function(const char *funcname, int nargs, Oid *argtypes)
{
	static const struct
	{
		const char *name;
		const char *replacement;
	} obsolete_functions[] = {
		{"mac_remove_user_policy", NULL},
		{"mac_remove_table_policy", NULL},
		{"mac_create_label_policy", NULL},
		{"mac_alter_label_policy", NULL},
		{"mac_drop_label_policy", NULL},
		{"mac_set_label_policy", NULL},
		{"mac_label_to_char", NULL},
		{"mac_char_to_label", NULL},
		{"mac_apply_user_policy", "set_user_seclabel"},
		{"mac_apply_table_policy", "set_table_seclabel"}
	};
	size_t		i;
	const char *message = NULL;
	const char *hint = NULL;

	if (funcname)
	{
		/* Match the parsed final name, including schema-qualified calls. */
		for (i = 0; i < lengthof(obsolete_functions); i++)
		{
			if (strcmp(funcname, obsolete_functions[i].name) == 0)
			{
				message = psprintf("function \"%s\" is no longer supported", funcname);
				if (obsolete_functions[i].replacement != NULL)
					hint = psprintf("Use \"%s\" instead.",
									 obsolete_functions[i].replacement);
				else
					hint = "This function has been removed. Remove it from your SQL.";
				break;
			}
		}

		if (message)
			ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_FUNCTION),
				 errmsg_internal("%s", message),
				 hint ? errhint("%s", hint) : 0));
	}

	if (prev_undefined_function_hook)
		prev_undefined_function_hook(funcname, nargs, argtypes);
}
/* End obsolete function diagnostics. */
#endif

void
_PG_init(void)
{
#ifdef ENABLE_COMPATIBLE_AK
	/* Keep obsolete names recognizable without changing shared state. */
	register_obsolete_gucs();
#endif

	DefineCustomBoolVariable("hg_sysmac.global_on",
							 "Turn on/off global_on",
							 NULL,
							 &global_on, /* Bug - #z5940 */
							 false,
							 PGC_SIGHUP,
							 GUC_NOT_IN_SAMPLE,
							 NULL,
							 NULL,
							 NULL);

	DefineCustomBoolVariable("hg_sysmac.global_table_level",
							 "Turn on/off global_table_level",
							 NULL,
							 &global_table_level, /* Bug - #z5940 */
							 false,
							 PGC_SIGHUP,
							 GUC_NOT_IN_SAMPLE,
							 NULL,
							 NULL,
							 NULL);

	DefineCustomBoolVariable("hg_sysmac.global_column_level",
							 "Turn on/off global_column_level",
							 NULL,
							 &global_column_level, /* Bug - #z5940 */
							 false,
							 PGC_SIGHUP,
							 GUC_NOT_IN_SAMPLE,
							 NULL,
							 NULL,
							 NULL);

	DefineCustomBoolVariable("hg_sysmac.global_row_level",
							 "Turn on/off global_row_level",
							 NULL,
							 &global_row_level, /* Bug - #z5940 */
							 true,
							 PGC_SIGHUP,
							 GUC_NOT_IN_SAMPLE,
							 NULL,
							 NULL,
							 NULL);

	/* Security label provider hook */
	register_label_provider(SYSMAC_LABEL_TAG,
							check_relabel);

	/* DML permission check */
	next_exec_check_perms_hook = ExecutorCheckPerms_hook;
	ExecutorCheckPerms_hook = sysmac_exec_check_perms;

	/* ProcessUtility hook */
	next_ProcessUtility_hook = ProcessUtility_hook;
	ProcessUtility_hook = sysmac_utility_command;

	/* Begin - Bug#z8537 */
	/* Sequence value permission check */
	next_sequence_access_hook = sequence_access_hook;
	sequence_access_hook = sysmac_sequence_access;
	/* End - Bug#z8537 */

#ifdef ENABLE_COMPATIBLE_AK
	/* Customize diagnostics only after ordinary function resolution fails. */
	prev_undefined_function_hook = undefined_function_hook;
	undefined_function_hook = sysmac_undefined_function;
#endif
}

/* End - ReqID: SRS-SECURITY-LMAC */
