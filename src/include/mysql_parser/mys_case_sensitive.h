/*--------------------------------------------------------------------
 * 
 * File: mys_case_sensitive.h
 *
 * Abstract: 
 * 		This file contains hooks declarations for case sensitive of mysql.
 *
 * Authored by qinshiyu@highgo.com, 20230803.
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		src/include/mysql_parser/mys_case_sensitive.h
 *
 * add the file for requirement "SRS-MYSQL-CASE"
 *
 *--------------------------------------------------------------------
 */

#ifndef MYS_CASE_SENSITIVE_H
#define MYS_CASE_SENSITIVE_H

#include "c.h"
#include "postgres.h"

#include "access/htup.h"
#include "catalog/objectaddress.h"
#include "nodes/parsenodes.h"
#include "nodes/pg_list.h"
#include "parser/parse_node.h"
#include "utils/guc.h"
#include "utils/ora_compatible.h"

typedef enum MysLowerCaseMode
{
	MYS_LOWER_SENSITIVE,		/* lower_case_table_names = 0 */
	MYS_LOWER_INSENSITIVE,		/* lower_case_table_names = 1 */
	MYS_LOWER_MIXED,			/* lower_case_table_names = 2 */
	MYS_LOWER_INVALID
} MysLowerCaseMode;


/* Hook for plugins to get control in all mysql case sensitive */
typedef bool (*Mysql_case_switch_hook_type) (void);
typedef bool (*Mysql_case_switch_lower_hook_type) (void);
typedef MysLowerCaseMode (*Mysql_case_switch_createobjlower_hook_type) (void);

/* Hook for plugins to get control in all mysql case sensitive */
typedef char * (*Mysql_casesensitivestr_hook_type) (char *);
typedef void (*Mysql_casesensitivelist_hook_type) (List *);
typedef void (*Mysql_casesensitiverangevar_hook_type) (RangeVar *);

/* Hook for plugins to get control in all mysql case sensitive,when create/alter/drop function,
 * need to modify table pg_myproc.
 */
typedef Oid (*Mysql_casesensitive_get_oid_hook_type) (const char *procedureName);
typedef void (*Mysql_casesensitive_get_obj_nameinfo_hook_type) (ObjectAddress address, char **objname, char **schema);
typedef HeapTuple (*Mysql_casesensitive_get_attName_hook_type) (Oid oid, char *attname);
typedef Oid (*Mysql_casesensitive_get_typoid_hook_type) (ParseState *pstate,
																		const TypeName *typeName,
																		bool temp_ok,
																		char *schemaname,
																		char *typname,
																		bool missing_ok);
/* Begin - BUG#Z00004315/4316 */
typedef bool (*Mysql_check_for_column_name_collision_hook_type) (Relation rel,
																	 const char *colname,
																	 bool if_not_exists,
																	 bool caseflg);
/* End - BUG#Z00004315/4316 */


extern PGDLLIMPORT Mysql_case_switch_lower_hook_type mysql_lower_hook;
extern PGDLLIMPORT Mysql_casesensitivestr_hook_type mysql_string_case_hook;
extern PGDLLIMPORT Mysql_casesensitivelist_hook_type mysql_list_case_hook;
extern PGDLLIMPORT Mysql_casesensitiverangevar_hook_type mysql_rvar_case_hook;
extern PGDLLIMPORT Mysql_casesensitive_get_oid_hook_type mysql_getoid_hook;
extern PGDLLIMPORT Mysql_case_switch_createobjlower_hook_type mysql_objlower_hook;
extern PGDLLIMPORT Mysql_casesensitive_get_obj_nameinfo_hook_type mysql_get_objnameinfo_hook;
extern PGDLLIMPORT Mysql_casesensitive_get_attName_hook_type mysql_get_attName_hook;
extern PGDLLIMPORT Mysql_casesensitive_get_typoid_hook_type mysql_get_typoid_hook;
/* Begin - BUG#Z00004315/4316 */
extern PGDLLIMPORT Mysql_check_for_column_name_collision_hook_type mysql_check_for_column_name_collision;
/* End - BUG#Z00004315/4316 */

extern DefElem *mys_casemakeDefElem(char *name, int location);
extern char *mys_lower_one_string_case(char *str);
extern char *mys_lower_two_string_case(char *str);
extern char *mys_lower_string_case(char *str);
extern void mysql_rvar_case(RangeVar *var);
extern void mys_lower_two_rvarschecma_case(RangeVar *var);
extern void mys_case_DefElem(List *elemlist);
extern char *mys_string_case(char *str);
extern void mys_createtb_rvar_case(RangeVar *var, RangeVar *varInheritorpart);
extern void mys_createtb_rvar_list_case(RangeVar *var, List *list);
extern char *mys_def_elem_case(char *str, Node *node);
extern void mys_lower_one_rvarrelname_case(RangeVar *var);
extern void mys_tbvmftb_lowerlist_case(ObjectType obt, List *list);
extern void mys_schemaandlanguage_list_case(ObjectType obt, List *list);
extern void mys_lower_list_case(List *list);
extern void mys_qualified_name_list_case(List *list);
extern char *mys_reindex_target_multitable_case(ReindexObjectType ridxtpy, char *str);
extern char *mys_altertb_case(RangeVar *var, char *str);
extern Node *mys_specifyobject_case(char *object, Node *str);
extern void mys_insertupdate_deal(List *list);
extern void mys_delete_clause_deal(List *collist,  List *usinglist);

#define MYS_CASE_MAKE_DEFELEN(name, location) \
	mys_casemakeDefElem(name, location)

#define MYS_LOWERONE_STRING_CASE(str) \
	mys_lower_one_string_case(str)

#define MYS_LOWERTWO_STRING_CASE(str) \
	mys_lower_two_string_case(str)

#define MYS_LOWER_STRING_CASE(str) \
	mys_lower_string_case(str);

#define MYS_RVAR_CASE(var) \
	mysql_rvar_case(var)

#define MYS_LOWERTWO_VARSCHECMA_CASE(var) \
	mys_lower_two_rvarschecma_case(var)

#define MYS_CASE_DEFELEM(elemlist) \
	mys_case_DefElem(elemlist)

#define MYS_STRING_CASE(str) \
	mys_string_case(str)

#define MYS_CREATETB_RVAR_CASE(var, varInheritorpart) \
	mys_createtb_rvar_case(var, varInheritorpart)

#define MYS_CREATETB_RVAR_LIST_CASE(var, list) \
	mys_createtb_rvar_list_case(var, list)

#define MYS_LOWER_ONE_RVARRELNAME_CASE(var) \
	mys_lower_one_rvarrelname_case(var)

#define MYS_DEF_ELEM_CASE(str, node) \
	mys_def_elem_case(str, node)

#define MYS_TBVMFTB_LOWERLIST_CASE(obt, list) \
	mys_tbvmftb_lowerlist_case(obt, list)

#define MYS_SCHEMAANDLANGUAGE_LIST_CASE(obt, list) \
	mys_schemaandlanguage_list_case(obt, list)

#define MYS_LOWER_LIST_CASE(list) \
	mys_lower_list_case(list)

#define MYS_QUALIFIED_NAME_LIST_CASE(list) \
	mys_qualified_name_list_case(list)

#define MYS_REINDEX_TARGET_MULTITABLE_CASE(ridxtpy, str) \
	mys_reindex_target_multitable_case(ridxtpy, str)

#define MYS_ALTERTB_CASE(var, str) \
	mys_altertb_case(var, str)

#define MYS_SPECIFYOBJECT_CASE(object, str) \
	mys_specifyobject_case(object, str)

#define MYS_INSERTUPDATE_CASE(list) \
	mys_insertupdate_deal(list)

#define MYS_DELETE_CLAUSE_DEAL_CASE(collist, usinglist) \
	mys_delete_clause_deal(collist, usinglist)

#endif
