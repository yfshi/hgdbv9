#include "postgres.h"

#include "fmgr.h"
#include "miscadmin.h"
#include "storage/proc.h"
#include "utils/backend_status.h"
#include "utils/builtins.h"
#include "utils/guc.h"

#include "restricted_dba.h"

PG_FUNCTION_INFO_V1(is_sysdba);
Datum
is_sysdba(PG_FUNCTION_ARGS)
{
	Oid		role_id = PG_GETARG_OID(0);
	bool	result = IS_SYSDBA(role_id);

	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(is_syssso);
Datum
is_syssso(PG_FUNCTION_ARGS)
{
	Oid		role_id = PG_GETARG_OID(0);
	bool	result = IS_SYSSSO(role_id);

	PG_RETURN_BOOL(result);
}

PG_FUNCTION_INFO_V1(is_syssao);
Datum
is_syssao(PG_FUNCTION_ARGS)
{
	Oid		role_id = PG_GETARG_OID(0);
	bool	result = IS_SYSSAO(role_id);

	PG_RETURN_BOOL(result);
}

static bool get_se_param(void)
{
	bool	changed = false;
	int		i;

	LWLockAcquire(se_param_ctl->lock, LW_EXCLUSIVE);
	for(i = 0; i < MAX_SE_PARAMS_ENTRIES; i++)
	{
		SeParamsEntry *entry = &se_param_ctl->entries[i];
		if(entry->is_valid > 0 && 
		   entry->target_backend_pid == MyProcPid && 
		   entry->target_db_id == MyDatabaseId)
		{
			if(entry->is_valid & SE_FEATURE_ON)
				se_feature_mode = entry->infos & SE_FEATURE_ON;
			if(entry->is_valid & RESTRICTED_DBA)
				restricted_dba = entry->infos & RESTRICTED_DBA;
			entry->is_valid = 0;
			changed = true;
			break;
		}
	}
	LWLockRelease(se_param_ctl->lock);

	if(!se_feature_mode && restricted_dba)
		ereport(WARNING,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("se_feature.mode is currently off, restricted_dba is invalid")));

	return changed;
}

PG_FUNCTION_INFO_V1(check_se_param_change);
Datum
check_se_param_change(PG_FUNCTION_ARGS)
{
	bool changed = get_se_param();
	char *result;

	if (changed)
		result = "got new se_feature params";
	else
		result = "already updated, nothing changed";

	PG_RETURN_TEXT_P(cstring_to_text(result));
}

static bool 
set_se_param(Oid user_id, char *param_name, bool value)
{
	bool	changed = false;
	int		i;
	int		first_invalid = -1;
	int32	infos = 0;
	SeParamsEntry *entry;
	int		num_backends = pgstat_fetch_stat_numbackends();
	int		beid;
	int		pid = -1;
	

	for (beid = 1; beid <= num_backends; beid++)
	{
		PgBackendStatus *beentry = pgstat_fetch_stat_beentry(beid);

		if (beentry && 
			beentry->st_databaseid == MyDatabaseId &&
			beentry->st_userid == user_id &&
			beentry->st_backendType == B_BACKEND)
		{
			pid = beentry->st_procpid;
			break;
		}	
	}

	if (pid <= 0)
		elog(ERROR,"no active backend of user %d",user_id);

	if (pg_strcasecmp(param_name, "se_feature.mode") == 0)
		infos = SE_FEATURE_ON;
	else if (pg_strcasecmp(param_name, "se_feature.restricted_dba") == 0)
		infos = RESTRICTED_DBA;
	else
		elog(ERROR, "Invalid parameter setting %s", param_name);




	LWLockAcquire(se_param_ctl->lock, LW_EXCLUSIVE);
	for(i = 0; i < MAX_SE_PARAMS_ENTRIES; i++)
	{
		
		entry = &se_param_ctl->entries[i];

		if(entry->is_valid > 0)
		{
			if(entry->target_db_id == MyDatabaseId && 
			   entry->target_backend_pid == pid
			   )
			{
				if(value)
					entry->infos |= infos; 
				else
					entry->infos &= ~infos;

				entry->is_valid |= infos;
				changed = true;
				break;
			}
		}
		else
		{
			if (first_invalid < 0)
				first_invalid = i;
		}
	}

	if (!changed && first_invalid >= 0)
	{
		entry = &se_param_ctl->entries[first_invalid];

		entry->target_db_id = MyDatabaseId;
		entry->target_backend_pid = pid;
		entry->infos = 0;
		if(value)
			entry->infos |= infos; 
		else
			entry->infos &= ~infos;

		entry->is_valid |= infos;
		changed = true;
	}
	LWLockRelease(se_param_ctl->lock);

	return changed;
}

PG_FUNCTION_INFO_V1(set_backend_se_param);
Datum
set_backend_se_param(PG_FUNCTION_ARGS)
{
	int		usre_id = PG_GETARG_OID(0);
	char   *param_name = text_to_cstring(PG_GETARG_TEXT_P(1));
	char   *result;
	bool	value = PG_GETARG_BOOL(2);

	if (!IS_SYSSSO(GetUserId()))
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("Only syssso users can set this value")));
	
	if(set_se_param(usre_id, param_name, value))
		result = "parameters set";
	else
		result = "No empty slot, check message or clean all of them";

	PG_RETURN_TEXT_P(cstring_to_text(result));
}

static void reset_se_param(void)
{
	int		i;

	LWLockAcquire(se_param_ctl->lock, LW_EXCLUSIVE);
	for(i = 0; i < MAX_SE_PARAMS_ENTRIES; i++)
	{
		SeParamsEntry *entry = &se_param_ctl->entries[i];
		
		entry->is_valid = 0;
	}
	LWLockRelease(se_param_ctl->lock);

}

PG_FUNCTION_INFO_V1(reset_all_se_param);
Datum
reset_all_se_param(PG_FUNCTION_ARGS)
{
	reset_se_param();

	PG_RETURN_TEXT_P(cstring_to_text("All reset"));
}
