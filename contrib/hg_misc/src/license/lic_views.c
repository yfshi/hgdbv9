/*------------------------------------------------------------------
 *
 * File: lic_views.c
 *
 * Abstract:
 *		Create views of license info. 
 *		Display the version of the Highgo database.
 *
 * Copyright:
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *		contrib/hg_misc/src/license/lic_views.c
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

#define LIC_VERSION_16		16

/*---- Function declarations ----*/

PG_FUNCTION_INFO_V1(get_lic_baseinfo);
PG_FUNCTION_INFO_V1(get_lic_funcinfo);
PG_FUNCTION_INFO_V1(highgo_version);
/* Begin - ReqID:SRS-MISC-LICENSE-MysqlDatabases */
PG_FUNCTION_INFO_V1(get_lic_extinfo); 
/* End - ReqID:SRS-MISC-LICENSE-MysqlDatabases */


Datum highgo_version(PG_FUNCTION_ARGS)
{
	/* Begin Fix Bug#M0000525 */
	const char *hgdb_version;
	char *curr_encoding = NULL;

	hg_set_is_with_title(false);

	curr_encoding = (char *)hg_get_encoding();
	hg_set_encoding(GetConfigOptionByName("server_encoding", NULL, true));

	hgdb_version = hg_get_highgoVersion();

	hg_set_encoding(curr_encoding);
	free(curr_encoding);

	PG_RETURN_TEXT_P(cstring_to_text(hgdb_version));
	/* End Fix Bug#M0000525 */
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

/* Begin - Reqid:SRS-MISC-LICENSE-TEMPLATE*/
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
	int  funclist_size = hg_get_productTemplateLen();
	int  idx;
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

	curr_encoding = (char *)hg_get_encoding();
	hg_set_encoding(GetConfigOptionByName("server_encoding", NULL, true));

	hg_set_productFunclistIsChinese(is_chinese);

	for (idx = 0; idx < funclist_size; idx++)
	{
		Datum		values[LIC_FUNC_VIEWS_COLS];
		bool		nulls[LIC_FUNC_VIEWS_COLS];
		int			i = 0;
		const char * classify = NULL;
		const char * name = NULL;
		const char * status = NULL;
		uint32_t 	func_id;

		memset(values, 0, sizeof(values));
		memset(nulls, 0, sizeof(nulls));

		/*
		 * The license function template is upward compatible. 
		 * For example, the 1.5 version license library is compatible with the 1.6 version feature templates. 
		 * Note: The newly added features in version 1.6 cannot be displayed in the 1.5 version library, but can 
		 * be displayed normally in the 1.6 version license library.
		 */
		func_id = hg_get_productFuncId(idx);
		if (func_id >= hg_get_productMaxFuncId() && !hg_get_is_classifyTitle(idx))
			continue;

		/* Let id start from 1 instead of 0 */
		values[i++] = UInt32GetDatum((idx+1));

		if (hg_get_is_classifyTitle(idx))
		{
			classify = hg_get_productFuncName(idx);
			values[i++] = CStringGetTextDatum(classify);
			nulls[i++]  = true;
			nulls[i++]  = true;
		}
		else
		{
			name   = hg_get_productFuncName(idx);
			status = hg_get_productFuncCodeStatus(idx);

			nulls[i++]  = true;
			values[i++] = CStringGetTextDatum(name);
			values[i++] = CStringGetTextDatum(status);
		}

		tuplestore_putvalues(tupstore, tupdesc, values, nulls);
	}

	tuplestore_donestoring(tupstore);
	
	hg_set_encoding(curr_encoding);
	free(curr_encoding);
	
	return (Datum)0;
}
/* End - Reqid:SRS-MISC-LICENSE-TEMPLATE*/

/* Begin - ReqID:SRS-MISC-LICENSE-MysqlDatabases */
/*
 * Get license extended info from license module. Depends on encoding show
 * Chinese version or English version.
 */
Datum
get_lic_extinfo(PG_FUNCTION_ARGS)
{
	bool is_chinese = false;
	char result_info[1024];
	char *encoding = "UTF8";
	char *utf8_region = NULL;
	char *curr_encoding = NULL;

	if (!hg_get_extended_fields(LICENSE_VERSION, 0))
		PG_RETURN_NULL();

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
	hg_set_encoding(GetConfigOptionByName("server_encoding", NULL, true));

	sprintf(result_info, "%s\n", hg_get_extended_fields(LICENSE_VERSION, 0));

	hg_set_encoding(curr_encoding);
	free(curr_encoding);

	return CStringGetTextDatum(result_info);
}
/* End - ReqID:SRS-MISC-LICENSE-MysqlDatabases */