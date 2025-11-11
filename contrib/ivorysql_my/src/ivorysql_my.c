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

#include "auth.h"
#include "charset/ctype.h"
#include "commands/collationcmds.h"
#include "common/int.h"
#include "include/casesensitive.h"
#include "include/set.h"
#include "include/server.h"
#include "utils/guc.h"
#include "utils/varlena.h"
#include "mysql_parser/mys_set_hook.h"
#include "utils/float.h"


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

/* Begin - ReqID:SRS-MYSQL-NUMBERICTYPE-INT2 */
static mysql_int2_datatype_in_hook_type pre_mysql_int2_datatype_in_hook = NULL;
/* End - ReqID:SRS-MYSQL-NUMBERICTYPE-INT2 */

/* Begin - ReqID:SRS-MYSQL-NUMBERICTYPE-INT4 */
static mysql_int4_datatype_in_hook_type pre_mysql_int4_datatype_in_hook = NULL;
/* End - ReqID:SRS-MYSQL-NUMBERICTYPE-INT4 */

/* Begin - SRS-MYSQL-HEX-LITERAL */
static mysql_int8_datatype_in_hook_type pre_mysql_int8_datatype_in_hook = NULL;
/* End - SRS-MYSQL-HEX-LITERAL */

/* Begin - bug#z5590 */
static mysql_float4_datatype_in_hook_type  pre_mysql_float4_datatype_in_hook = NULL;
static mysql_float8_datatype_in_hook_type  pre_mysql_float8_datatype_in_hook = NULL;
static mysql_numeric_datatype_in_hook_type pre_mysql_numeric_datatype_in_hook = NULL;
/* End - bug#z5590 */

/* Begin - BUG#Z00004315/4316 */
Mysql_check_for_column_name_collision_hook_type pre_mysql_check_for_column_name_collision = NULL;
/* End - BUG#Z00004315/4316 */

/* Begin BUG#5048 5070 */
static mysql_hexstring_parse_hook_type pre_mysql_hexstring_parse_hook = NULL;
/* End BUG#5048 5070 */

/* Begin - ReqID:SRS-MYSQL-SET */
static uservariable_hash_search_hook_type pre_uservariable_hash_search_hook = NULL;
static SetUserVarInternal_hook_type pre_setuservarinternal_hook = NULL;

/* End - ReqID:SRS-MYSQL-SET */

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
	/* Begin - Req: SRS-MYSQL-PROTOCOL */
	register_mysql_listener();
    register_mysql_auth();
	/* End - Req: SRS-MYSQL-PROTOCOL */

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

	/* Begin - ReqID:SRS-MYSQL-NUMBERICTYPE-INT2 */
	pre_mysql_int2_datatype_in_hook = mysql_int2_datatype_in_hook;
	mysql_int2_datatype_in_hook = my_atoi_int16;
	/* End - ReqID:SRS-MYSQL-NUMBERICTYPE-INT2*/

	/* Begin - ReqID:SRS-MYSQL-NUMBERICTYPE-INT4 */
	pre_mysql_int4_datatype_in_hook = mysql_int4_datatype_in_hook;
	mysql_int4_datatype_in_hook = my_atoi_int32;
	/* End - ReqID:SRS-MYSQL-NUMBERICTYPE-INT4 */

	/* Begin - SRS-MYSQL-HEX-LITERAL */
	pre_mysql_int8_datatype_in_hook = mysql_int8_datatype_in_hook;
	mysql_int8_datatype_in_hook = my_atoi_int64;
	/* End - SRS-MYSQL-HEX-LITERAL */
	
	/* Begin - bug#z5590 */
	pre_mysql_float4_datatype_in_hook = mysql_float4_datatype_in_hook;
	mysql_float4_datatype_in_hook = my_atof;

	pre_mysql_float8_datatype_in_hook = mysql_float8_datatype_in_hook;
	mysql_float8_datatype_in_hook = my_atod;

	pre_mysql_numeric_datatype_in_hook = mysql_numeric_datatype_in_hook;
	mysql_numeric_datatype_in_hook = my_aton;
	/* End - bug#z5590 */

	pre_mysql_check_for_column_name_collision = mysql_check_for_column_name_collision;

	/* Begin - BUG#Z00004315/4316 */
	mysql_check_for_column_name_collision = mys_check_for_column_name_collision;
	/* End - BUG#Z00004315/4316 */

	/* Begin BUG#5048 5070 */
	pre_mysql_hexstring_parse_hook = mysql_hexstring_parse_hook;
	mysql_hexstring_parse_hook = parse_hexstring;
	/* End BUG#5048 5070 */

	/* Begin - ReqID:SRS-MYSQL-SET */
	pre_uservariable_hash_search_hook = uservariable_hash_search_hook;
	uservariable_hash_search_hook = userVariable_Hash_Search;
	pre_setuservarinternal_hook = setuservarinternal_hook;
	setuservarinternal_hook = mysSetUserVarInternal;
	/* End - ReqID:SRS-MYSQL-SET */
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

	/* Begin - ReqID:SRS-MYSQL-NUMBERICTYPE-INT2 */
	mysql_int2_datatype_in_hook = pre_mysql_int2_datatype_in_hook;
	/* End - ReqID:SRS-MYSQL-NUMBERICTYPE-INT2 */

	/* Begin - ReqID:SRS-MYSQL-NUMBERICTYPE-INT4 */
	mysql_int4_datatype_in_hook = pre_mysql_int4_datatype_in_hook;
	/* End - ReqID:SRS-MYSQL-NUMBERICTYPE-INT4 */

	/* Begin - SRS-MYSQL-HEX-LITERAL */
	mysql_int8_datatype_in_hook = pre_mysql_int8_datatype_in_hook;
	/* End - SRS-MYSQL-HEX-LITERAL */

	/* Begin - bug#z5590 */
	mysql_float4_datatype_in_hook = pre_mysql_float4_datatype_in_hook;
	mysql_float8_datatype_in_hook = pre_mysql_float8_datatype_in_hook;
	mysql_numeric_datatype_in_hook = pre_mysql_numeric_datatype_in_hook;
	/* End - bug#z5590 */

	/* Begin - BUG#Z00004315/4316 */
	mysql_check_for_column_name_collision = pre_mysql_check_for_column_name_collision;
	/* End - BUG#Z00004315/4316 */

	/* Begin BUG#5048 5070 */
	mysql_hexstring_parse_hook = pre_mysql_hexstring_parse_hook;
	/* End BUG#5048 5070 */

	/* Begin - ReqID:SRS-MYSQL-SET */
	uservariable_hash_search_hook = pre_uservariable_hash_search_hook;
	setuservarinternal_hook = pre_setuservarinternal_hook;
	/* End - ReqID:SRS-MYSQL-SET */

    /* Begin - Req: SRS-MYSQL-PROTOCOL */
	unregister_mysql_listener();
	/* End - Req: SRS-MYSQL-PROTOCOL */
}
