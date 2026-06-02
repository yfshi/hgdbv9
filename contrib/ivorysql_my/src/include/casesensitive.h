/*--------------------------------------------------------------------
 * 
 * File: casesensitive.h
 *
 * Abstract: 
 * 		This file contains extern declarations for ivorysql_my itself.
 *
 * Authored by qinshiyu@highgo.com, 20230803.
 *
 * Copyright:
 * Copyright (c) 2023, HighGo Software Co.,Ltd. All rights reserved. 
 *
 * Identification:
 *		contrib/ivorysql_my/src/include/casesensitive.h
 *
 * add the file for requirement "SRS-MYSQL-CASE"
 *
 *--------------------------------------------------------------------
 */
#ifndef IVORYSQL_MYS_CASE_H_
#define IVORYSQL_MYS_CASE_H_

#include "postgres.h"
#include "access/htup.h"
#include "nodes/pg_list.h"
#include "nodes/parsenodes.h"
#include "mysql_parser/mys_case_sensitive.h"
#include "catalog/objectaddress.h"
#include "parser/parse_node.h"


extern bool pg_compatible_mysql_lower(void);
extern MysLowerCaseMode pg_compatible_mysql_ojblower(void);


extern char *pg_compatible_mysql_case(char *str);
extern void pg_compatible_mysql_list_case(List *list);
extern void pg_compatible_mysql_rangvar_case(RangeVar *relation);
extern Oid mysql_get_namespace_oid(const char *nspname);
extern void get_obj_nameinfo_bybodyid(ObjectAddress address, char **objname, char **schema);
extern HeapTuple mysql_SearchAttName(Oid relid, char *attname);
extern Oid pg_compatible_mysql_get_typoid(ParseState *pstate, const TypeName *typeName, bool temp_ok, 
										  char *schemaname, char *typname, bool missing_ok);
/* Begin - BUG#Z00004315/4316 */
extern bool mys_check_for_column_name_collision(Relation rel,
															 const char *colname,
															 bool if_not_exists,
/* End - BUG#Z00004315/4316 */															 bool caseflg);

#endif	/* IVORYSQL_MYS_CASE_H_ */
