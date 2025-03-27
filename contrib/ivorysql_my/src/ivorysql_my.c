/*------------------------------------------------------------------
 *
 * File: ivorysql_my.c
 *
 * Abstract:
 *      Ivorysql_my main entrypoint.
 *
 * Authored by qinshiyu@highgo.com, 20230511.
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved.
 *
 * Identification:
 *      contrib/ivorysql_my/src/ivorysql_my.c
 *
 *------------------------------------------------------------------
 */
#include "postgres.h"
#include "charset/ctype.h"
#include "commands/collationcmds.h"
#include "fmgr.h"
#include "mysql_parser/mys_case_sensitive.h"
#include "include/ivorysql_my.h"
#include "include/casesensitive.h"
#include "utils/varlena.h"

/* Only include it once in any C file */
PG_MODULE_MAGIC;

/* Begin - ReqID:SRS-MYSQL-CASE */
/* Saved hook value in case of unload */
static Mysql_case_switch_hook_type pre_mysql_switch_hook = NULL;
static Mysql_case_switch_lower_hook_type pre_mysql_lower_hook = NULL;
static Mysql_casesensitivestr_hook_type pre_mysql_string_case_hook = NULL;
static Mysql_casesensitivelist_hook_type pre_mysql_list_case_hook = NULL;
static Mysql_casesensitiverangevar_hook_type premysql_rvar_case_hook = NULL;

static Mysql_casesensitive_get_oid_hook_type premysql_getoid_hook = NULL;
static Mysql_case_switch_createobjlower_hook_type premysql_objlower_hook = NULL;
static Mysql_casesensitive_get_obj_nameinfo_hook_type premysql_get_objnameinfo_hook = NULL;
static Mysql_casesensitive_get_attName_hook_type premysql_get_attName_hook = NULL;
static Mysql_casesensitive_get_typoid_hook_type premysql_get_typoid_hook = NULL;
/* End - ReqID:SRS-MYSQL-CASE */

void _PG_init(void);
void _PG_fini(void);

/* Begin - Req: SRS-MYSQL-CHARSET */
static mysql_hash_hook_type 				pre_mysql_hash_hook;
static mysql_strcmp_hook_type 				pre_mysql_strcmp_hook;
static import_mysql_collation_hook_type 	pre_import_mysql_collation_hook;
/* End - Req: SRS-MYSQL-CHARSET */

/*
 * Module initialization function.
 */
void
_PG_init(void)
{
	/* Begin - ReqID:SRS-MYSQL-CASE */
	pre_mysql_switch_hook = mysql_switch_hook;
	mysql_switch_hook = pg_compatible_mysql_switch;
	pre_mysql_lower_hook = mysql_lower_hook;
	mysql_lower_hook = pg_compatible_mysql_lower;
	pre_mysql_string_case_hook = mysql_string_case_hook;
	mysql_string_case_hook = pg_compatible_mysql_case;
	pre_mysql_list_case_hook = mysql_list_case_hook;
	mysql_list_case_hook = pg_compatible_mysql_list_case;
	premysql_rvar_case_hook = mysql_rvar_case_hook;
	mysql_rvar_case_hook = pg_compatible_mysql_rangvar_case;
	premysql_getoid_hook = mysql_getoid_hook;
	mysql_getoid_hook = mysql_get_namespace_oid;
	premysql_objlower_hook = mysql_objlower_hook;
	mysql_objlower_hook = pg_compatible_mysql_ojblower;
	premysql_get_objnameinfo_hook = mysql_get_objnameinfo_hook;
	mysql_get_objnameinfo_hook = get_obj_nameinfo_bybodyid;
	premysql_get_attName_hook = mysql_get_attName_hook;
	mysql_get_attName_hook = mysql_SearchAttName;
	premysql_get_typoid_hook = mysql_get_typoid_hook;
	mysql_get_typoid_hook = pg_compatible_mysql_get_typoid;
	/* End - ReqID:SRS-MYSQL-CASE */

	/* Beign - Req:SRS-MYSQL-CHARSET */
	pre_import_mysql_collation_hook = import_mysql_collation_hook;
	import_mysql_collation_hook = do_import_mysql_collation;

	pre_mysql_strcmp_hook = mysql_strcoll_hook;
	mysql_strcoll_hook = do_mysql_strcoll;
	
	pre_mysql_hash_hook = mysql_hash_hook;
	mysql_hash_hook = do_mysql_hash;
	/* End - Req:SRS-MYSQL-CHARSET */
}

/*
 * Module finalization function.
 *
 * Note that _PG_fini will only be called during an unload of the module, 
 * not during process termination. Presently, unloads are disabled and
 * will never occur, but this may change in the future.
 */
void
_PG_fini(void)
{
	/* Begin - ReqID:SRS-MYSQL-CASE */
	mysql_switch_hook = pre_mysql_switch_hook;
	mysql_lower_hook = pre_mysql_lower_hook;
	mysql_string_case_hook = pre_mysql_string_case_hook;
	mysql_list_case_hook = pre_mysql_list_case_hook;
	mysql_rvar_case_hook = premysql_rvar_case_hook;
	mysql_getoid_hook = premysql_getoid_hook;
	mysql_objlower_hook = premysql_objlower_hook;
	mysql_get_objnameinfo_hook = premysql_get_objnameinfo_hook;
	mysql_get_attName_hook = premysql_get_attName_hook;
	mysql_get_typoid_hook = premysql_get_typoid_hook;
	/* End - ReqID:SRS-MYSQL-CASE */

	/* Beign - Req:SRS-MYSQL-CHARSET */
	mysql_hash_hook = pre_mysql_hash_hook;
	mysql_strcoll_hook = pre_mysql_strcmp_hook;
	import_mysql_collation_hook = pre_import_mysql_collation_hook;
	/* End - Req:SRS-MYSQL-CHARSET */
}
