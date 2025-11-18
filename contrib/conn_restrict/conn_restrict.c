/*-------------------------------------------------------------------------
 *
 * File: conn_restrict.c
 *
 * Abstract: support connect restrict, include mac address and logon time.
 *
 * Copyright:
 * Copyright (c) 2009-2023, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *	  		/contrib/conn_restrict/conn_restrict.c
 *
 *-------------------------------------------------------------------------
 */
/* Begin ReqID: SRS-CLIENT-AUTH */
#include "postgres.h"

#include "conn_restrict.h"

/* GUC */
extern bool enable_macandtime;

/* hook */
static check_connrules_hook_type prev_check_rules_hook = NULL;

/* static function declaration */
extern void _PG_init(void);
extern void _PG_fini(void);

static void CheckMacandTimeRules(char* username, char* mac_address);

static void DeleteRestrictRulesByuseroid(Oid           useroid);

static bool DeleteRestrictRulesByusername(char* username);

static void CheckRulesParameters(int fitertype, char* username, char* macaddress, char* outmacaddress, char* allowtime, char* outallowtime);

static void SearchRestrictRules(Oid useroid, char* username);

static void LookupRerstrictRules(char* username, char* mac_address);

static Timestamp timestamptz2timestamp(TimestampTz timestamp);

static int CheckMacInRules(char* macaddress, char* rulemac, char* outrulemac);

static bool CheckMacInRuleslist(char* macaddress, char* rulemac);

static int CheckTimeInRules(Timestamp logintime, char* allow_time, char* outallow_time);

static bool CheckTimeInRuleslist(Timestamp logintime, char* allow_time);

static bool checkTimeformat(char* allow_time);
static bool checkMacformat(char* macaddress);

PG_FUNCTION_INFO_V1(add_rules);
PG_FUNCTION_INFO_V1(remove_rules);

/*
 * Module initialization function
 */
void _PG_init(void)
{
	/* add guc value, bool default false */
	DefineCustomBoolVariable("conn_restrict.enable",
						 "enable restrict or disable",
						 NULL,
						 &enable_macandtime,
						 false,
						 PGC_SUSET,
						 GUC_SUPERUSER_ONLY,
						 NULL,
						 NULL,
						 NULL);
	prev_check_rules_hook = check_rules_hook;
	check_rules_hook = CheckMacandTimeRules;
	elog(DEBUG3, "conn_restrict _PG_init complete, check_rules_hook: %p", check_rules_hook);
}

void _PG_fini(void)
{
	check_rules_hook = prev_check_rules_hook;
	enable_macandtime = false;
	elog(DEBUG3, "conn_restrict _PG_fini complete, check_rules_hook: %p", check_rules_hook);
}

/*
 * checkTimeformat
 * check the time format.
 * return bool
 */
static bool
checkTimeformat(char* allow_time)
{
	text		   *tmpstartime;
	text		   *tmpendtime;
	char		   *tmptime = NULL;
	char		   *tmstime = NULL;
	char		   *tmetime = NULL;
	int				len = 0;
	int				num = 0;
	char		   *starttime = NULL;
	char		   *endtime = NULL;
	Timestamp		startm;
	Timestamp		endtm;
	Timestamp		curtime;
	bool			startflag = false;
	bool			endflag = false;

	/* get the time period,split according to "to". */
	tmptime = strstr(allow_time, "to");
	len = strlen(allow_time);
	num = strlen(tmptime);
	starttime = palloc0(sizeof(char) * (len - num + 1));
	endtime = palloc0(sizeof(char) * len);

	memcpy(starttime, allow_time, len - num);
	memcpy(endtime, tmptime + 2, num - 2);

	/* remove the first and last spaces. */
	tmpstartime = DatumGetTextPP(DirectFunctionCall1(ltrim1,
									PointerGetDatum(cstring_to_text(starttime))));
	tmpstartime = DatumGetTextPP(DirectFunctionCall1(rtrim1,
									PointerGetDatum(tmpstartime)));

	tmpendtime = DatumGetTextPP(DirectFunctionCall1(ltrim1,
									PointerGetDatum(cstring_to_text(endtime))));

	tmpendtime = DatumGetTextPP(DirectFunctionCall1(rtrim1,
									PointerGetDatum(tmpendtime)));

	tmstime = text_to_cstring(tmpstartime);
	tmetime = text_to_cstring(tmpendtime);
	curtime = timestamptz2timestamp(GetCurrentTimestamp());

	/* check startime format. */
	if(strstr(tmstime, ":"))
		startflag = true;
	/* check endtime format. */
	if(strstr(tmetime, ":"))
		endflag = true;

	if(strlen(tmstime) > 8 )
	{
		if(strlen(tmetime) <= 8 )
			elog(ERROR,"Time period is incorrect!");
	}

	if(strlen(tmstime) <= 8 )
	{
		if(strlen(tmetime) > 8 )
			elog(ERROR,"Time period is incorrect!");
	}

	/* convert a time string to a timestamp type. */
	if(strlen(tmstime) > 8 || !startflag)
	/* if startime format is YYYY-MM-DD HH24:MI:SS it can be converted directly. */
		startm = DirectFunctionCall3(timestamp_in,
									CStringGetDatum(tmstime),
									ObjectIdGetDatum(InvalidOid),
									Int32GetDatum(-1));
	else
	{
		/* if the time format is HH24:MI:SS,we need to get the year,month,and day. */
		StringInfoData		buf;
		char			   *nowtime = DatumGetCString(DirectFunctionCall1(timestamp_out,
										   TimestampGetDatum(curtime)));
		char			   *pretime = palloc0(sizeof(char) * 11);

		/* get the year,month,and day. */
		memcpy(pretime, nowtime, 10);
		
		initStringInfo(&buf);

		appendStringInfo(&buf, "%s %s",
					 pretime, tmstime);

		startm = DirectFunctionCall3(timestamp_in,
									CStringGetDatum(buf.data),
									ObjectIdGetDatum(InvalidOid),
									Int32GetDatum(-1));
		pfree(pretime);

	}
	/* if endtime format is YYYY-MM-DD HH24:MI:SS it can be converted directly. */
	if(strlen(tmetime) > 8 || !endflag)
		endtm = DirectFunctionCall3(timestamp_in,
									CStringGetDatum(tmetime),
									ObjectIdGetDatum(InvalidOid),
									Int32GetDatum(-1));
	else
	{
		StringInfoData		buf1;
		char			   *nowtime1 = DatumGetCString(DirectFunctionCall1(timestamp_out,
										   TimestampGetDatum(curtime)));
		char			   *pretime1 = palloc0(sizeof(char) * 11);

		/* get the year,month,and day. */
		memcpy(pretime1, nowtime1, 10);
		
		initStringInfo(&buf1);

		appendStringInfo(&buf1, "%s %s",
					 pretime1, tmetime);

		endtm = DirectFunctionCall3(timestamp_in,
									CStringGetDatum(buf1.data),
									ObjectIdGetDatum(InvalidOid),
									Int32GetDatum(-1));
		pfree(pretime1);

	}

	pfree(starttime);
	pfree(endtime);

	if(startm >= endtm)
		elog(ERROR,"Time period is incorrect!");

	if(!strcmp(tmstime, "") || !strcmp(tmetime, ""))
		return false;
	return true;
}

/*
 * checkMacformat
 * check the mac address format.
 * return bool
 */
static bool
checkMacformat(char* macaddress)
{
	List		   *rulemaclist = NIL;
	ListCell	   *cell = NULL;
	int				num = 0;

	/* Parse string into list of identifiers */
	if(!SplitIdentifierString(macaddress, ',', &rulemaclist))
		elog(ERROR, "invalid list syntax");
	foreach(cell, rulemaclist)
	{
		char   *mac = lfirst(cell);
		if(strlen(mac) != 17)
			return false;
		while(*mac)
		{
			if(*mac == ':')
				num++;
			mac++;
		}
		if(num != 5)
			return false;
		else
			num = 0;
	}
	return true;
}


/* 
 * CheckRulesParmeter
 * check the input parmerters of the function.
 * return void
 */
static void
CheckRulesParameters(int fitertype, char* username, char* macaddress, char* outmacaddress, char* allowtime, char* outallowtime)
{
	/* check the filter_type, it can be only be 0 or 1. */
	if(!(fitertype == 0 || fitertype == 1))
			elog(ERROR, "The rule type %d is error,the correct value is 0 or 1.", fitertype);

	/* check the username, no longer than 255. */
	if(strlen(username) > 256)
		elog(ERROR, "The username length is more than 255!");

	/* check the mac and time parameters. */
	if(!strcmp(macaddress, "*") && !strcmp(outmacaddress, "*") &&
	   !strcmp(allowtime, "*") && !strcmp(outallowtime, "*"))
		elog(ERROR, "Rule_mac,outrule_mac,allow_time,outallow_time parameters cannot be null at the same time!");
	else
	{
		/* check the mac address,restricted and unrestricted cannot be consistent. */
		if(!strcmp(macaddress, outmacaddress))
		{
			if(!(!strcmp(macaddress, "*") && !strcmp(outmacaddress, "*")))
				elog(ERROR, "The mac address is same in blacklist and whitlist!");
		}

		/* check mac address format. */
		if(strcmp(macaddress, "*"))
		{
			if(strstr(macaddress, "-"))
				elog(ERROR, "The rule_mac format is error,please enter mac addresses separted by commas!");
			else
			{
				if(!checkMacformat(macaddress))
					elog(ERROR, "The rule_mac format is error!");
			}
		}
		if(strcmp(outmacaddress, "*"))
		{
			if(strstr(outmacaddress, "-"))
				elog(ERROR, "The outrule_mac format is error,please enter mac addresses separted by commas!");
			else
			{
				if(!checkMacformat(outmacaddress))
					elog(ERROR, "The outrule_mac format is error!");
			}
		}

		/* check time periods, restricted and unrestricted cannot be consistent. */
		if(!strcmp(allowtime, outallowtime))
		{
			if(!(!strcmp(allowtime, "*") && !strcmp(outallowtime, "*")))
				elog(ERROR, "The time interval is same in blacklist and whitlist!");
		}

		/* check time format. */
		if(strcmp(allowtime, "*"))
		{
			if(!strstr(allowtime, "to"))
				elog(ERROR, "The allowtime format is error, must be separated by 'to'!");
			else
			{
				if(!checkTimeformat(allowtime))
					elog(ERROR, "The allowtime format is error!");
			}
		}
		if(strcmp(outallowtime, "*"))
		{
			if(!strstr(outallowtime, "to"))
				elog(ERROR, "The outallowtime format is error, must be separated by 'to'!");
			else
			{
				if(!checkTimeformat(outallowtime))
					elog(ERROR, "The outallowtime format is error!");
			}
		}
	}
}

/*
 * SearchRestrictRules
 * lookup whether the user's rule exists.
 * return void
 */
static void
SearchRestrictRules(Oid useroid, char *username)
{
	HeapTuple		tup;
	Relation		rel;
	ScanKeyData		scanKey[1];
	TableScanDesc	scandesc;

	/* open table conn_restrict_rules */
	rel = table_open(get_relname_relid(HGS_RULE_TABLE_NAME, SECURITY_SCHEMA_NAMESPACE), AccessShareLock);

	/* Through the user oid,look up records in the table. */
	ScanKeyInit(&scanKey[0],
				1,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(useroid));

	scandesc = table_beginscan(rel, SnapshotSelf, 1, scanKey);

	tup = heap_getnext(scandesc, ForwardScanDirection);
	if(HeapTupleIsValid(tup))
	{
		elog(ERROR, "The rule under this user %s already exists!", username);
	}

	table_endscan(scandesc);
	table_close(rel, AccessShareLock);
}

/*
 * add_rules
 * add rules to record to the onn_restrict_rules table.
 */
Datum
add_rules(PG_FUNCTION_ARGS)
{
	text	   *arg1;
	text	   *arg2;
	text	   *arg3;
	text	   *arg4;
	text	   *arg5;
	Datum		values[Natts_connect_restrict_rules];
	bool		nulls[Natts_connect_restrict_rules];
	HeapTuple	tup;
	Relation	rel;
	Oid			useroid;
	char	   *username = NULL;
	char	   *macaddress = NULL;
	char	   *outmacaddress = NULL;
	char	   *allowtime = NULL;
	char	   *outallowtime = NULL;
	int32		arg0 = 0; /* Bug #6248 */

	/* Is syssso user*/
	if(!IS_SYSSSO(GetUserId()))
		ereport(ERROR,
			(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				errmsg("permission denied "),
				errdetail("member of syssso_role "
						  "can add rules")));

	/* get the function parameters. */
	/* Bug #6248 */
	if(!fcinfo->args[0].isnull)
		arg0 = PG_GETARG_INT32(0);
	else
		elog(ERROR,"filter type can't be null!");

	if(!fcinfo->args[1].isnull)
		arg1 = PG_GETARG_TEXT_PP(1);
	else
		elog(ERROR,"user name not null!");
	if(!fcinfo->args[2].isnull)
		arg2 = PG_GETARG_TEXT_PP(2);
	else
		arg2 = cstring_to_text("*");
	if(!fcinfo->args[3].isnull)
		arg3 = PG_GETARG_TEXT_PP(3);
	else
		arg3 = cstring_to_text("*");

	if(!fcinfo->args[4].isnull)
		arg4 = PG_GETARG_TEXT_PP(4);
	else
		arg4 = cstring_to_text("*");

	if(!fcinfo->args[5].isnull)
		arg5 = PG_GETARG_TEXT_PP(5);
	else
		arg5 = cstring_to_text("*");

	username = text_to_cstring(arg1);
	macaddress = text_to_cstring(arg2);
	outmacaddress = text_to_cstring(arg3);
	allowtime = text_to_cstring(arg4);
	outallowtime = text_to_cstring(arg5);

	/* first check the parmeter,include filter_type,username. */
	CheckRulesParameters(arg0, username, macaddress, outmacaddress, allowtime, outallowtime);

	/*get user oid */
	useroid = get_role_oid(username, false);

	/* secend check the new rule  in conn_restrict_rules through the user oid. */
	SearchRestrictRules(useroid, username);

	/* open table conn_restrict_rules */
	rel = table_open(get_relname_relid(HGS_RULE_TABLE_NAME, SECURITY_SCHEMA_NAMESPACE), RowExclusiveLock);

	/* zero-clear */
	memset(values, 0, sizeof(values));
	memset(nulls, 0, sizeof(nulls));

	values[0] = ObjectIdGetDatum(useroid);
	
	values[1] = Int32GetDatum(arg0);
	values[2] = PointerGetDatum(arg1);
	values[3] = PointerGetDatum(arg2);
	values[4] = PointerGetDatum(arg3);
	values[5] = PointerGetDatum(arg4);
	values[6] = PointerGetDatum(arg5);

	/* insert the rule attribute into the table. */
	tup = heap_form_tuple(RelationGetDescr(rel), values, nulls);
	simple_heap_insert(rel, tup);
	table_close(rel, RowExclusiveLock);

	PG_RETURN_BOOL(1);
}

/*
 * DeleteRestrictRulesByuseroid
 * delete restrict rules by useroid.
 * return void.
 */
static void
DeleteRestrictRulesByuseroid(Oid useroid)
{
	TableScanDesc	scandesc;
	ScanKeyData		scanKey[1];
	HeapTuple		tup;
	Relation		rel;

	rel = table_open(get_relname_relid(HGS_RULE_TABLE_NAME, SECURITY_SCHEMA_NAMESPACE), RowExclusiveLock);
	ScanKeyInit(&scanKey[0],
				1,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(useroid));

	scandesc = table_beginscan(rel, SnapshotSelf, 1, scanKey);
	tup = heap_getnext(scandesc, ForwardScanDirection);
	if(HeapTupleIsValid(tup))
	{
		simple_heap_delete(rel, &tup->t_self);
	}
	else
		elog(ERROR, "The rule under this user cannot be found, please check it!");
	table_endscan(scandesc);
	table_close(rel, RowExclusiveLock);
}

/*
 * DeleteRestrictRulesByusername
 * delete restrict rules by username.
 * return void.
 */
static bool
DeleteRestrictRulesByusername(char* username)
{
	TableScanDesc	scandesc;
	ScanKeyData		scanKey[1];
	HeapTuple		tup;
	Relation		rel;
	bool			flag = false;

	rel = table_open(get_relname_relid(HGS_RULE_TABLE_NAME, SECURITY_SCHEMA_NAMESPACE), RowExclusiveLock);
	ScanKeyInit(&scanKey[0],
				3,
				BTEqualStrategyNumber, F_TEXTEQ,
				CStringGetTextDatum(username));

	scandesc = table_beginscan(rel, SnapshotSelf, 1, scanKey);
	tup = heap_getnext(scandesc, ForwardScanDirection);
	while (HeapTupleIsValid(tup))
	{
		flag = true;
		simple_heap_delete(rel, &tup->t_self);
		tup = heap_getnext(scandesc, ForwardScanDirection);
	}

	table_endscan(scandesc);
	table_close(rel, RowExclusiveLock);
	return flag;
}

/*
 * remove_rules
 * remove a rules,delete a recode from the conn_restict_rules table.
 * return void.
 */
Datum
remove_rules(PG_FUNCTION_ARGS)
{
	text	   *arg;
	char	   *username;
	Oid			userid;

	/* Is syssso user*/
	if(!IS_SYSSSO(GetUserId()))
		ereport(ERROR,
			(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				errmsg("permission denied "),
				errdetail("member of syssso_role "
						  "can remove rules")));

	if(!fcinfo->args[0].isnull)
		arg = PG_GETARG_TEXT_PP(0);
	
	else
		/* If the user name is null, report an error! */
		elog(ERROR,"username not null!");

	/* remove the rule,by username,if not find,then through the user oid. */
	username = text_to_cstring(arg);
	if(!DeleteRestrictRulesByusername(username))
	{
		userid = get_role_oid(username, false);
		if(userid != InvalidOid)
			DeleteRestrictRulesByuseroid(userid);
		else
			elog(ERROR, "The rule under this user cannot be found, please check it!");
	}

	PG_RETURN_BOOL(1);
}

/*
 * CheckMacInRuleslist
 * check whether the mac address of the current client is in the rule.
 * return bool.
 */
static bool
CheckMacInRuleslist(char* macaddress, char* rulemac)
{
	List		   *rulemaclist = NIL;
	ListCell	   *cell = NULL;

	/* Parse string into list of identifiers */
	if(!SplitIdentifierString(rulemac, ',', &rulemaclist))
		elog(ERROR, "invalid list syntax");
	foreach(cell, rulemaclist)
	{
		char   *mac = lfirst(cell);
		if(!pg_strcasecmp(macaddress, mac))
		{
			return true;
		}
	}
	return false;
}

/*
 * CheckMacInRules
 * check if mac address is restricted or unrestricted in the rule.
 * return int.
 */
static int
CheckMacInRules(char* macaddress, char* rulemac, char* outrulemac)
{
	bool	include_flag = false;
	bool	exclude_flag = false;
	match_macflag result_flag = rulemac_notmatch;

	if (!macaddress || *macaddress == '\0')
		return rulemac_notmatch;

	include_flag = !strcmp(rulemac, "*") || CheckMacInRuleslist(macaddress, rulemac);
	exclude_flag = strcmp(outrulemac, "*") && CheckMacInRuleslist(macaddress, outrulemac);

	if (include_flag) {
		result_flag = rulemac_include_match;
		if (exclude_flag)
			result_flag = rulemac_both_match;
	} else if (exclude_flag)
		result_flag = rulemac_exclude_match;
	else
		result_flag = rulemac_notmatch;
	elog(DEBUG3, "CheckMacInRules, include_mac:%s, exclude_mac:%s, include_flag:%d, exclude_flag:%d, result_flag: %d",
					rulemac, outrulemac, include_flag, exclude_flag, result_flag);
	return result_flag;
}

/*
 * CheckTimeInRuleslist
 * check whether the logon time of the current client is in the rule.
 * return bool.
 */
static bool
CheckTimeInRuleslist(Timestamp logintime, char* allow_time)
{
	Timestamp		startm;
	Timestamp		endtm;
	bool			result = false;
	bool			startflag = false;
	bool			endflag = false;
	text		   *tmpstartime;
	text		   *tmpendtime;
	char		   *tmptime = NULL;
	int				len = 0;
	int				num = 0;
	char		   *starttime = NULL;
	char		   *endtime = NULL;
	char		   *tmpstime = NULL;
	char		   *tmpetime = NULL;

	if (!allow_time)
		return false;
	/* get the time period,split according to "to". */
	tmptime = strstr(allow_time, "to");
	if (!tmptime)
		elog(ERROR, "The time span:%s is not in correct format, should contain 'to'", allow_time);
	len = strlen(allow_time);
	num = strlen(tmptime);
	starttime = palloc0(sizeof(char) * (len - num + 1));
	endtime = palloc0(sizeof(char) * len);

	memcpy(starttime, allow_time, len - num);
	memcpy(endtime, tmptime + 2, num - 2);

	/* remove the first and last spaces. */
	tmpstartime = DatumGetTextPP(DirectFunctionCall1(ltrim1,
									PointerGetDatum(cstring_to_text(starttime))));
	tmpstartime = DatumGetTextPP(DirectFunctionCall1(rtrim1,
									PointerGetDatum(tmpstartime)));

	tmpendtime = DatumGetTextPP(DirectFunctionCall1(ltrim1,
									PointerGetDatum(cstring_to_text(endtime))));

	tmpendtime = DatumGetTextPP(DirectFunctionCall1(rtrim1,
									PointerGetDatum(tmpendtime)));

	/* check startime format. */
	tmpstime = text_to_cstring(tmpstartime);
	if(strstr(tmpstime, ":"))
		startflag = true;

	/* check endtime format. */
	tmpetime = text_to_cstring(tmpendtime);
	if(strstr(tmpetime, ":"))
		endflag = true;
	
	/* convert a time string to a timestamp type. */
	if(strlen(tmpstime) > 8 || !startflag)
	/* if startime format is YYYY-MM-DD HH24:MI:SS it can be converted directly. */
		startm = DirectFunctionCall3(timestamp_in,
									CStringGetDatum(text_to_cstring(tmpstartime)),
									ObjectIdGetDatum(InvalidOid),
									Int32GetDatum(-1));
	else
	{
		/* if the time format is HH24:MI:SS,we need to get the year,month,and day. */
		StringInfoData		buf;
		char			   *nowtime = DatumGetCString(DirectFunctionCall1(timestamp_out,
										   TimestampGetDatum(logintime)));
		char			   *pretime = palloc0(sizeof(char) * 11);

		/* get the year,month,and day. */
		memcpy(pretime, nowtime, 10);

		initStringInfo(&buf);

		appendStringInfo(&buf, "%s %s",
					 pretime, tmpstime);

		startm = DirectFunctionCall3(timestamp_in,
									CStringGetDatum(buf.data),
									ObjectIdGetDatum(InvalidOid),
									Int32GetDatum(-1));
		pfree(pretime);

	}
	/* if endtime format is YYYY-MM-DD HH24:MI:SS it can be converted directly. */
	if(strlen(tmpetime) > 8 || !endflag)
		endtm = DirectFunctionCall3(timestamp_in,
									CStringGetDatum(text_to_cstring(tmpendtime)),
									ObjectIdGetDatum(InvalidOid),
									Int32GetDatum(-1));
	else
	{
		StringInfoData		buf1;
		char			   *nowtime1 = DatumGetCString(DirectFunctionCall1(timestamp_out,
										   TimestampGetDatum(logintime)));
		char			   *pretime1 = palloc0(sizeof(char) * 11);

		/* get the year,month,and day. */
		memcpy(pretime1, nowtime1, 10);
		
		initStringInfo(&buf1);

		appendStringInfo(&buf1, "%s %s",
					 pretime1, tmpetime);

		endtm = DirectFunctionCall3(timestamp_in,
									CStringGetDatum(buf1.data),
									ObjectIdGetDatum(InvalidOid),
									Int32GetDatum(-1));
		pfree(pretime1);

	}

	/* check if the logon time is in the time preiod. */
	if(logintime - startm >= 0 && endtm - logintime >= 0)
		result = true;
	pfree(starttime);
	pfree(endtime);
	return result;
}

/* 
 * CheckTimeInRules
 * check the logon time,whether it is in restricted or unrestricted time.
 * return int.
 */
static int
CheckTimeInRules(Timestamp logintime, char* allow_time, char* outallow_time)
{
	bool			include_flag = false;
	bool			exclude_flag = false;
	match_timeflag  result_flag = ruletime_notmatch;

	/* First,check whether the login time is within the restricted time period. */
	include_flag = !strcmp(allow_time, "*") || CheckTimeInRuleslist(logintime, allow_time);
	/* Second,check wherther the login time is in an unrestricted time period. */
	exclude_flag = strcmp(outallow_time, "*") && CheckTimeInRuleslist(logintime, outallow_time);
	if(include_flag) {
		result_flag = ruletime_include_match;
		if(exclude_flag)
			result_flag = ruletime_both_match;
	} else if (exclude_flag)
			result_flag = ruletime_exclude_match;
	else
		result_flag = ruletime_notmatch;
	elog(DEBUG3, "CheckTimeInRules, include_time:%s, exclude_time:%s, include_flag:%d, exclude_flag:%d, result_flag: %d",
					allow_time, outallow_time, include_flag, exclude_flag, result_flag);
	return result_flag;
}

/*
 * LookupRerstrictRules
 * get the information in the rule by user name.
 * return void.
 */
static void
LookupRerstrictRules(char* username, char* mac_address)
{
	HeapTuple		tup;
	Relation		rel;
	TableScanDesc	scandesc;
	ScanKeyData		scanKey[1];
	bool			isNull;
	Datum			filtertype;
	Datum			rulemac;
	Datum			outrulemac;
	Datum			allowtime;
	Datum			outallowtime;
	int32			filter_type;
//	char		   *cur_time = NULL;
	char		   *rule_mac;
	char		   *outrule_mac;
	char		   *allow_time;
	char		   *outallow_time;
	match_macflag	macflag = rulemac_notmatch;
	match_timeflag	timeflag = ruletime_notmatch;
	Timestamp		curtime;
	Oid				useroid;
	Oid 			relid = InvalidOid;
	elog(DEBUG3, "Entering LookupRerstrictRules, username:%s, mac_address:%s", username, mac_address);

	/*get user oid */
	useroid = get_role_oid(username, false);

	/* open table restrict_rules */
	relid = get_relname_relid(HGS_RULE_TABLE_NAME, SECURITY_SCHEMA_NAMESPACE);
	if (relid == InvalidOid) {
		elog(WARNING, "Table:%s doesn't exist, check if extension conn_restrict is created", HGS_RULE_TABLE_NAME);
		return;
	}
	else
		rel = table_open(relid, AccessShareLock);

	ScanKeyInit(&scanKey[0], 1,
					BTEqualStrategyNumber, F_OIDEQ,	ObjectIdGetDatum(useroid));

	scandesc = table_beginscan(rel, SnapshotSelf, 1, scanKey);
	tup = heap_getnext(scandesc, ForwardScanDirection);
	if(HeapTupleIsValid(tup))
	{
		/* get specific information,including rule type,restricted mac address and time period,
		 * unrestricted mac address and time period.
		 */
		filtertype = heap_getattr(tup, 2, RelationGetDescr(rel), &isNull);
		filter_type = DatumGetInt32(filtertype);
		rulemac = heap_getattr(tup, 4, RelationGetDescr(rel), &isNull);
		rule_mac = text_to_cstring(DatumGetTextPP(rulemac));
		outrulemac = heap_getattr(tup, 5, RelationGetDescr(rel), &isNull);
		outrule_mac = text_to_cstring(DatumGetTextPP(outrulemac));
		allowtime = heap_getattr(tup, 6, RelationGetDescr(rel), &isNull);
		allow_time = text_to_cstring(DatumGetTextPP(allowtime));
		outallowtime = heap_getattr(tup, 7, RelationGetDescr(rel), &isNull);
		outallow_time = text_to_cstring(DatumGetTextPP(outallowtime));

		elog(DEBUG3, "Checking connect rules: filter_type:%d, rule_mac:%s, outrule_mac:%s, allow_time:%s, outallow_time:%s", 
				filter_type, rule_mac, outrule_mac, allow_time, outallow_time);
		/* if the type is blacklist. */
		if(filter_type == 1)
		{
			/* Check macaddress. */
			macflag = CheckMacInRules(mac_address, pstrdup(rule_mac), pstrdup(outrule_mac));
			/* Check time. */
			curtime = timestamptz2timestamp(GetCurrentTimestamp());
			timeflag = CheckTimeInRules(curtime, pstrdup(allow_time), pstrdup(outallow_time));

			if (macflag == rulemac_include_match)
					elog(ERROR, "Mac address in restricted!");
			if (timeflag == ruletime_include_match)
					elog(ERROR, "Login time in restricted!");
		}
		/* if the type is whitelist. */
		else if (filter_type == 0)
		{
			/* Check macaddress. */
			macflag = CheckMacInRules(mac_address, pstrdup(rule_mac), pstrdup(outrule_mac));
			/* Check time. */
			curtime = timestamptz2timestamp(GetCurrentTimestamp());
			timeflag = CheckTimeInRules(curtime, pstrdup(allow_time), pstrdup(outallow_time));

			/* get mac and time result */
			if (macflag != rulemac_include_match)
					elog(ERROR, "Mac address in restricted!");
			if (timeflag != ruletime_include_match)
					elog(ERROR, "Login time in restricted!");
		}
		else
			elog(ERROR, "filter type: %d is not correct.", filter_type);
	}

	table_endscan(scandesc);
	table_close(rel, AccessShareLock);
	elog(DEBUG3, "Exiting LookupRerstrictRules");
}

/*
 * timestamptz2timestamp
 * type conversion,timestamptz->timestamp.
 */
static Timestamp
timestamptz2timestamp(TimestampTz timestamp)
{
	Timestamp	result;
	struct pg_tm tt,
			   *tm = &tt;
	fsec_t		fsec;
	int			tz;

	if (TIMESTAMP_NOT_FINITE(timestamp))
		result = timestamp;
	else
	{
		if (timestamp2tm(timestamp, &tz, tm, &fsec, NULL, NULL) != 0)
			ereport(ERROR,
					(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
					 errmsg("timestamp out of range")));
		if (tm2timestamp(tm, fsec, NULL, &result) != 0)
			ereport(ERROR,
					(errcode(ERRCODE_DATETIME_VALUE_OUT_OF_RANGE),
					 errmsg("timestamp out of range")));
	}
	return result;
}

/* 
 * CheckMacandTimeRules
 * check whether the mac address and logon time of ther current client are in the rule.
 * return void.
 */
static void 
CheckMacandTimeRules(char* username, char* mac_address)
{
	elog(DEBUG3, "Entering CheckMacandTimeRules hook, enable_macandtime:%d", enable_macandtime);
	if(enable_macandtime)
		LookupRerstrictRules(username, mac_address);
	return;
}

/* End ReqID: SRS-CLIENT-AUTH */
