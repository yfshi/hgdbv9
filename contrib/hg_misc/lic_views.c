/*------------------------------------------------------------------
 *
 * File: lic_views.c
 *
 * Abstract:
 * 	  Create views of license info.
 *
 * Copyright:
 * Copyright (c) 2022, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *		contrib/hg_misc/lic_views.c
 *
 * Requirement description:
 *		ReqID:SRS-MISC-LICENSE
 
 *------------------------------------------------------------------
 */
#include "postgres.h"

#include <math.h>
#include <sys/stat.h>
#include <unistd.h>

#include "funcapi.h"
#include "miscadmin.h"
#include "utils/builtins.h"
#include "utils/memutils.h"
#include "utils/guc.h"

/* this file must be compiled with license module */
#ifdef USE_LICENSE
#include "license.h"
#endif

/*
 * The number of columns in view v_lic_funcinfo. Currently we have id,
 * name, description and enabled. If we change the defination of
 * v_lic_funcinfo we must change the following macro too.
 */
#define LIC_FUNC_VIEWS_COLS  4

/*---- Function declarations ----*/

void		_PG_init(void);
void		_PG_fini(void);

PG_FUNCTION_INFO_V1(get_lic_baseinfo);
PG_FUNCTION_INFO_V1(get_lic_funcinfo);



/*
 * Module load callback
 */
void
_PG_init(void)
{
	/*add code for init*/
}

/*
 * Module unload callback
 */
void
_PG_fini(void)
{
	/*add code for fini */
}

/*
 * Get license base info from license module. Depends on encoding show
 * Chinese version or English version.
 */
Datum
get_lic_baseinfo(PG_FUNCTION_ARGS)
{
	bool is_chinese = false;
	char result_info[1024];
	char *encoding = "UTF8";
	char *utf8_region = NULL;
	char *curr_encoding = NULL;

	encoding = GetConfigOptionByName("client_encoding", NULL, true);
	/* output the Chinese version if specified*/
	if (pg_strncasecmp(encoding, "GBK", strlen("GBK")) == 0 ||
		pg_strncasecmp(encoding, "EUC_CN", strlen("EUC_CN")) == 0 ||
		pg_strncasecmp(encoding, "GB18030", strlen("GB18030")) == 0 ||
		(pg_strncasecmp(encoding, "UTF8", strlen("UTF8")) == 0 && \
			(utf8_region = getenv("LANG")) && \
			pg_strncasecmp(utf8_region, "zh_CN", strlen("zh_CN")) == 0))
	{
		is_chinese = true;
	}

	/* Build result string here, output the Chinese version if specified */
	hg_set_is_with_title(true);
	hg_set_is_chinese_title(is_chinese);
	curr_encoding = (char *)hg_get_encoding();
	hg_set_encoding(GetConfigOptionByName("server_encoding", NULL, true)); // Fix bug#M0000278

	sprintf(result_info, "%s\n%s\n%s\n%s\n%s\n%s\n%s\n" \
						 "%s\n%s\n%s\n%s\n%s\n%s\n%s\n" \
						 "%s\n%s\n",
							hg_get_licver(), hg_get_licSerial(), hg_get_license_status(), hg_get_userInfo(), hg_get_authType(),
							hg_get_usedType(), hg_get_createTime(), hg_get_finalTime(), hg_get_highgoVersion(),
							hg_get_projectInfo(), hg_get_maxConnection(), hg_get_maxCpuCores(), hg_get_defaultDBname(),
							hg_get_defaultDBuser(), hg_get_defaultDBport(), hg_get_hardware_code());
	hg_set_encoding(curr_encoding);
	free(curr_encoding);
	
	return CStringGetTextDatum(result_info);
}

/*
 * Get function list from license and return them as a record, So it can
 * be used in view v_lic_funcinfo.
 */
Datum
get_lic_funcinfo(PG_FUNCTION_ARGS)
{
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
	TupleDesc	tupdesc;
	Tuplestorestate *tupstore;
	MemoryContext per_query_ctx;
	MemoryContext oldcontext;
	int funclist_size = hg_get_enumLen();
	int idx;
	bool is_chinese = false;
	char *encoding = "UTF8";
	char *utf8_region = NULL;
	char *curr_encoding = NULL;

	encoding = GetConfigOptionByName("client_encoding", NULL, true);
	/* output the Chinese version if specified*/
	if (pg_strncasecmp(encoding, "GBK", strlen("GBK")) == 0 ||
		pg_strncasecmp(encoding, "EUC_CN", strlen("EUC_CN")) == 0 ||
		pg_strncasecmp(encoding, "GB18030", strlen("GB18030")) == 0 ||
		(pg_strncasecmp(encoding, "UTF8", strlen("UTF8")) == 0 && \
			(utf8_region = getenv("LANG")) && \
			pg_strncasecmp(utf8_region, "zh_CN", strlen("zh_CN")) == 0))
	{
		is_chinese = true;
	}

	/* check to see if caller supports us returning a tuplestore */
	if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("set-valued function called in context that cannot accept a set")));
	if (!(rsinfo->allowedModes & SFRM_Materialize))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("materialize mode required, but it is not " \
						"allowed in this context")));

	/* Switch into long-lived context to construct returned data structures */
	per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
	oldcontext = MemoryContextSwitchTo(per_query_ctx);

	/* Build a tuple descriptor for our result type */
	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");

	tupstore = tuplestore_begin_heap(true, false, work_mem);
	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = tupstore;
	rsinfo->setDesc = tupdesc;

	MemoryContextSwitchTo(oldcontext);
	// Begin - bug#M0000278
	curr_encoding = (char *)hg_get_encoding();
	hg_set_encoding(GetConfigOptionByName("server_encoding", NULL, true));
	// End - bug#M0000278
	for (idx = 0; idx < funclist_size; idx++)
	{
		Datum		values[LIC_FUNC_VIEWS_COLS];
		bool		nulls[LIC_FUNC_VIEWS_COLS];
		int			i = 0;
		const char		*name = NULL;
		const char		*desc = NULL;

		memset(values, 0, sizeof(values));
		memset(nulls, 0, sizeof(nulls));

		/* Set coresponding version based on encoding */
		if(is_chinese)
		{
			name = hg_get_productFuncNameCh(idx);
			desc = hg_get_productFuncDescCh(idx);
		}
		else
		{
			name = hg_get_productFuncNameEn(idx);
			desc = hg_get_productFuncDescEn(idx);
		}

		/* Let id start from 1 instead of 0 */
		values[i++] = UInt32GetDatum((idx+1));
		values[i++] = CStringGetTextDatum(name);
		values[i++] = CStringGetTextDatum(desc);
		values[i++] = BoolGetDatum(hg_get_productBitMapByCode(idx));

		tuplestore_putvalues(tupstore, tupdesc, values, nulls);
	}

	tuplestore_donestoring(tupstore);
	// Begin - bug#M0000278
	hg_set_encoding(curr_encoding);
	free(curr_encoding);
	// End - bug#M0000278
	return (Datum)0;
}
