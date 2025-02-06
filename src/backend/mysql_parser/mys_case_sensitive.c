/*--------------------------------------------------------------------
 * 
 * File: mys_case_sensitive.c
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
 *		src/backend/mysql_parser/mys_case_sensitive.c
 *
 * add the file for requirement "SRS-MYSQL-CASE"
 *
 *--------------------------------------------------------------------
 */

#include "postgres.h"
#include "nodes/makefuncs.h"
#include "nodes/parsenodes.h"
#include "mysql_parser/mys_case_sensitive.h"
#include "mysql_parser/mys_scanner.h"


Mysql_case_switch_hook_type mysql_switch_hook = NULL;
Mysql_case_switch_lower_hook_type mysql_lower_hook = NULL;
Mysql_casesensitivestr_hook_type mysql_string_case_hook = NULL;
Mysql_casesensitivelist_hook_type mysql_list_case_hook = NULL;
Mysql_casesensitiverangevar_hook_type mysql_rvar_case_hook = NULL;
Mysql_casesensitive_get_oid_hook_type mysql_getoid_hook = NULL;
Mysql_case_switch_createobjlower_hook_type mysql_objlower_hook = NULL;
Mysql_casesensitive_get_obj_nameinfo_hook_type mysql_get_objnameinfo_hook = NULL;
Mysql_casesensitive_get_attName_hook_type mysql_get_attName_hook = NULL;
Mysql_casesensitive_get_typoid_hook_type mysql_get_typoid_hook = NULL;

/*
 * mys_casemakeDefElem
 *
 * We handle identifiers that aren't parser keywords with
 * the following special-case codes, to avoid bloating the
 * size of the main parser.
 */
DefElem *mys_casemakeDefElem(char *name, int location)
{
	DefElem		   *res = NULL;

	if (mysql_switch_hook && mysql_switch_hook())
	{
		if (pg_strcasecmp(name, "superuser") == 0)
			res = makeDefElem("superuser", (Node *)makeInteger(true), location);
		else if (pg_strcasecmp(name, "nosuperuser") == 0)
			res = makeDefElem("superuser", (Node *)makeInteger(false), location);
		else if (pg_strcasecmp(name, "createrole") == 0)
			res = makeDefElem("createrole", (Node *)makeInteger(true), location);
		else if (pg_strcasecmp(name, "nocreaterole") == 0)
			res = makeDefElem("createrole", (Node *)makeInteger(false), location);
		else if (pg_strcasecmp(name, "replication") == 0)
			res = makeDefElem("isreplication", (Node *)makeInteger(true), location);
		else if (pg_strcasecmp(name, "noreplication") == 0)
			res = makeDefElem("isreplication", (Node *)makeInteger(false), location);
		else if (pg_strcasecmp(name, "createdb") == 0)
			res = makeDefElem("createdb", (Node *)makeInteger(true), location);
		else if (pg_strcasecmp(name, "nocreatedb") == 0)
			res = makeDefElem("createdb", (Node *)makeInteger(false), location);
		else if (pg_strcasecmp(name, "login") == 0)
			res = makeDefElem("canlogin", (Node *)makeInteger(true), location);
		else if (pg_strcasecmp(name, "nologin") == 0)
			res = makeDefElem("canlogin", (Node *)makeInteger(false), location);
		else if (pg_strcasecmp(name, "bypassrls") == 0)
			res = makeDefElem("bypassrls", (Node *)makeInteger(true), location);
		else if (pg_strcasecmp(name, "nobypassrls") == 0)
			res = makeDefElem("bypassrls", (Node *)makeInteger(false), location);
		else if (pg_strcasecmp(name, "noinherit") == 0)
		{
			/*
			 * Note that INHERIT is a keyword, so it's handled by main parser, but
			 * NOINHERIT is handled here.
			 */
			res = makeDefElem("inherit", (Node *)makeInteger(false), location);
		}
	}
	else
	{
		if (strcmp(name, "superuser") == 0)
			res = makeDefElem("superuser", (Node *)makeInteger(true), location);
		else if (strcmp(name, "nosuperuser") == 0)
			res = makeDefElem("superuser", (Node *)makeInteger(false), location);
		else if (strcmp(name, "createrole") == 0)
			res = makeDefElem("createrole", (Node *)makeInteger(true), location);
		else if (strcmp(name, "nocreaterole") == 0)
			res = makeDefElem("createrole", (Node *)makeInteger(false), location);
		else if (strcmp(name, "replication") == 0)
			res = makeDefElem("isreplication", (Node *)makeInteger(true), location);
		else if (strcmp(name, "noreplication") == 0)
			res = makeDefElem("isreplication", (Node *)makeInteger(false), location);
		else if (strcmp(name, "createdb") == 0)
			res = makeDefElem("createdb", (Node *)makeInteger(true), location);
		else if (strcmp(name, "nocreatedb") == 0)
			res = makeDefElem("createdb", (Node *)makeInteger(false), location);
		else if (strcmp(name, "login") == 0)
			res = makeDefElem("canlogin", (Node *)makeInteger(true), location);
		else if (strcmp(name, "nologin") == 0)
			res = makeDefElem("canlogin", (Node *)makeInteger(false), location);
		else if (strcmp(name, "bypassrls") == 0)
			res = makeDefElem("bypassrls", (Node *)makeInteger(true), location);
		else if (strcmp(name, "nobypassrls") == 0)
			res = makeDefElem("bypassrls", (Node *)makeInteger(false), location);
		else if (strcmp(name, "noinherit") == 0)
		{
			/*
			 * Note that INHERIT is a keyword, so it's handled by main parser, but
			 * NOINHERIT is handled here.
			 */
			res = makeDefElem("inherit", (Node *)makeInteger(false), location);
		}
	}
	
	return res;
}

/*
 * mys_lower_one_string_case
 *
 * When lower_case_table_names = 1, call this function
 * to convert the string to lower case
 */
char *mys_lower_one_string_case(char *str)
{
	char	   *newstr = str;

	if (mysql_objlower_hook && mysql_string_case_hook)
	{
		if (lower_one == mysql_objlower_hook())
			newstr = mysql_string_case_hook(str);
	}

	return newstr;
}

/*
 * mys_lower_two_string_case
 *
 * When lower_case_table_names = 2, call this function
 * to convert the string to lower case
 */
char *mys_lower_two_string_case(char *str)
{
	char	   *newstr = str;

	if (mysql_objlower_hook && mysql_string_case_hook)
	{
		if (lower_two == mysql_objlower_hook())
			newstr = mysql_string_case_hook(str);
	}

	return newstr;
}

/*
 * mys_lower_string_case
 *
 * When lower_case_table_names = 1 or 2, call this function
 * to convert the string to lower case
 */
char *mys_lower_string_case(char *str)
{
	char	   *newstr = str;

	if (mysql_lower_hook && mysql_string_case_hook && mysql_lower_hook())
	{
		newstr = mysql_string_case_hook(str);
	}

	return newstr;
}

/*
 * mysql_rvar_case
 *
 * When lower_case_table_names = 1 or 2,
 * this function is called to convert the
 * string in the structure RangeVar to lower case.
 */
void mysql_rvar_case(RangeVar *var)
{
	if (mysql_objlower_hook && mysql_string_case_hook && mysql_rvar_case_hook)
	{
		if (lower_one == mysql_objlower_hook() && var != NULL)
			var->relname = mysql_string_case_hook(var->relname);

		if (lower_two == mysql_objlower_hook() && var != NULL)
			mysql_rvar_case_hook(var);
	}
}

/*
 * mys_lower_two_rvarschecma_case
 *
 * When lower_case_table_names = 2,
 * this function is called to convert the
 * string in the structure RangeVar to lower case.
 */
void mys_lower_two_rvarschecma_case(RangeVar *var)
{
	if (mysql_objlower_hook && mysql_string_case_hook && var != NULL)
	{
		if (lower_two == mysql_objlower_hook())
			var->schemaname = mysql_string_case_hook(var->schemaname);
	}
}

/*
 * mys_case_DefElem
 *
 * When lower_case_table_names = 1 or 2,
 * this function is called to convert the
 * string to lower case.
 */
void mys_case_DefElem(List *elemlist)
{
	if (mysql_switch_hook && mysql_string_case_hook && mysql_switch_hook())
	{
		ListCell	*cell;

		foreach (cell, elemlist)
		{
			if (nodeTag(lfirst(cell)) == T_DefElem)
			{
				DefElem *res = lfirst(cell);
				res->defname = mysql_string_case_hook(res->defname);
			}
		}
	}
}

/*
 * mys_string_case
 *
 * When lower_case_table_names = 1 or 2,
 * this function is called to convert the
 * string to lower case.
 */
char *mys_string_case(char *str)
{
	char	   *newstr = str;

	if (mysql_switch_hook && mysql_string_case_hook && mysql_switch_hook())
	{
		newstr = mysql_string_case_hook(str);
	}

	return newstr;
}

/*
 * mys_createtb_rvar_case
 *
 * When lower_case_table_names = 2,
 * this function is called to convert the
 * string in the structure RangeVar to lower case.
 */
void mys_createtb_rvar_case(RangeVar *var, RangeVar *varInheritorpart)
{
	if (mysql_objlower_hook && mysql_string_case_hook && mysql_list_case_hook)
	{
		if (lower_one == mysql_objlower_hook())
		{
			if (var != NULL)
				var->relname = mysql_string_case_hook(var->relname);
			if (varInheritorpart != NULL)
				mysql_rvar_case_hook(varInheritorpart);
		}
		if (lower_two == mysql_objlower_hook())
		{
			if (varInheritorpart != NULL)
				mysql_rvar_case_hook(varInheritorpart);
		}
	}
}

/*
 * mys_createtb_rvar_list_case
 *
 * When lower_case_table_names = 1 or 2,
 * this function is called to convert the
 * string in the structure RangeVar or list to lower case.
 */
void mys_createtb_rvar_list_case(RangeVar *var, List *list)
{
	if (mysql_objlower_hook && mysql_string_case_hook && mysql_list_case_hook)
	{
		if (lower_one == mysql_objlower_hook())
		{
			if (var != NULL)
				var->relname = mysql_string_case_hook(var->relname);
			if (list != NIL)
				mysql_list_case_hook(list);
		}
		if (lower_two == mysql_objlower_hook())
		{
			if (list != NIL)
				mysql_list_case_hook(list);
		}
	}
}

/*
 * mys_lower_one_rvarrelname_case
 *
 * When lower_case_table_names = 1,
 * this function is called to convert the
 * string in the structure RangeVar to lower case.
 */
void mys_lower_one_rvarrelname_case(RangeVar *var)
{
	if (mysql_objlower_hook && mysql_string_case_hook && var != NULL)
	{
		if (lower_one == mysql_objlower_hook())
			var->relname = mysql_string_case_hook(var->relname);
	}
}

/*
 * mys_def_elem_case
 *
 * When MySQL case is turned on, this function is
 * called to convert the string lower case.
 */
char *mys_def_elem_case(char *str, Node *node)
{
	char	   *newstr = str;

	if (mysql_switch_hook && mysql_string_case_hook && mysql_switch_hook())
	{
		newstr = mysql_string_case_hook(str);

		if(node != NULL && nodeTag(node) == T_TypeName)
		{
			TypeName *n = (TypeName *)node;

			if (list_length(n->names) == 1 && (!pg_strcasecmp(strVal(llast(n->names)), "SAFE") ||
				!pg_strcasecmp(strVal(llast(n->names)), "RESTRICTED") || !pg_strcasecmp(strVal(llast(n->names)), "UNSAFE")))
					strVal(llast(n->names)) = mysql_string_case_hook(strVal(llast(n->names)));
		}
	}

	return newstr;
}

/*
 * mys_tbvmftb_lowerlist_case
 *
 * When MySQL case is turned on, this function is
 * called to convert the string to lower case.
 */
void mys_tbvmftb_lowerlist_case(ObjectType obt, List *list)
{
	if (mysql_lower_hook && mysql_list_case_hook && mysql_lower_hook())
	{
		if (obt == OBJECT_TABLE || obt == OBJECT_VIEW || obt == OBJECT_MATVIEW ||
			obt == OBJECT_FOREIGN_TABLE || obt == OBJECT_SEQUENCE)
		{
			ListCell *cell;

			foreach(cell, list)
			{
				if (IsA(lfirst(cell), List))
					mysql_list_case_hook((List *)lfirst(cell));
			}
		}
	}
}

/*
 * mys_schemaandlanguage_list_case
 *
 * When MySQL case is turned on, this function is
 * called to convert the string in list to lower case.
 */
void mys_schemaandlanguage_list_case(ObjectType obt, List *list)
{
	if (mysql_lower_hook && mysql_list_case_hook && mysql_lower_hook())
	{
		if (obt == OBJECT_SCHEMA)
			mysql_list_case_hook(list);
	}
	if (mysql_switch_hook && mysql_switch_hook())
	{
		if (obt == OBJECT_LANGUAGE)
		{
			if (strVal(llast(list)))
				strVal(llast(list)) = mysql_string_case_hook(strVal(llast(list)));
		}
	}
}

/*
 * mys_lower_list_case
 *
 * When MySQL case is turned on, this function is
 * called to convert the string in list to lower case.
 */
void mys_lower_list_case(List *list)
{
	if (mysql_lower_hook && mysql_list_case_hook && mysql_lower_hook())
	{
		if (list != NIL)
			mysql_list_case_hook(list);
	}
}

/*
 * mys_qualified_name_list_case
 *
 * When lower_case_table_names = 1 or 2, this function is
 * called to convert the string in the structure RangeVar
 * to lower case.
 */
void mys_qualified_name_list_case(List *list)
{
	if (mysql_objlower_hook && mysql_string_case_hook && mysql_rvar_case_hook)
	{
		ListCell *cell;
		RangeVar *r;

		foreach(cell, list)
		{
			if (IsA(lfirst(cell), RangeVar))
			{
				r = (RangeVar *)lfirst(cell);
				if (r != NULL)
				{
					if (lower_one == mysql_objlower_hook())
						((RangeVar *)lfirst(cell))->relname = mysql_string_case_hook(r->relname);
					if (lower_two == mysql_objlower_hook())
						mysql_rvar_case_hook((RangeVar *)lfirst(cell));
				}
			}
		}
	}
}

/*
 * mys_reindex_target_multitable_case
 *
 * When lower_case_table_names = 1 or 2, this function
 * is called to convert the string to lower case.
 */
char *mys_reindex_target_multitable_case(ReindexObjectType ridxtpy, char *str)
{
	char	   *newstr = str;

	if (mysql_objlower_hook && mysql_string_case_hook)
	{
		if (ridxtpy == REINDEX_OBJECT_SCHEMA && (str != NULL))
		{
			if (lower_one == mysql_objlower_hook() || lower_two == mysql_objlower_hook())
				newstr = mysql_string_case_hook(str);
		}
	}

	return newstr;
}

/*
 * mys_altertb_case
 *
 * When lower_case_table_names = 1 or 2, this function is called
 * to convert the string in the structure RangeVar to lower case.
 */
char *mys_altertb_case(RangeVar *var, char *str)
{
	char	   *newstr = str;

	if (mysql_objlower_hook && mysql_string_case_hook )
	{
		if (lower_one == mysql_objlower_hook())
		{
			if (var != NULL)
				var->relname = mysql_string_case_hook(var->relname);

			newstr = mysql_string_case_hook(str);
		}
		if (lower_two == mysql_objlower_hook() && var != NULL)
		{
			mysql_rvar_case_hook(var);
		}
	}

	return newstr;
}

/*
 * mys_specifyobject_case
 *
 * When lower_case_table_names = 1 or 2, this function
 * is called to convert the string to lower case.
 */
Node *mys_specifyobject_case(char *object, Node *str)
{
	if ((lower_one == mysql_objlower_hook() || lower_two == mysql_objlower_hook()) &&
											(object != NULL) && (str != NULL))
	{
		if (!pg_strcasecmp(object, "schema_name") || !pg_strcasecmp(object, "table_name"))
		{
			if (nodeTag(str) == T_String)
				 return (Node *)makeString(mysql_string_case_hook(strVal(str)));
		}
	}

	return str;
}

/*
 * mys_insertupdate_deal
 *
 * When lower_case_table_names = 1 or 2, this function is called
 * to convert the string in the structure RangeVar to lower case.
 */
void mys_insertupdate_deal(List *list)
{
	ListCell *cell;
	ResTarget *res;
	obj_lower lowernum = lower_invalid;

	if (mysql_objlower_hook && mysql_string_case_hook)
		lowernum = mysql_objlower_hook();
	if (lower_one == lowernum || lower_two == lowernum)
	{
		foreach(cell, list)
		{
			if (IsA(lfirst(cell), ResTarget))
			{
				res = (ResTarget *)lfirst(cell);
				if (res->indirection != NIL)
				{
					res->name = mysql_string_case_hook(res->name);
					if (list_length(res->indirection) == 2)
					{
						if (IsA(linitial(res->indirection), String))
							strVal(linitial(res->indirection)) = 
											mysql_string_case_hook(strVal(linitial(res->indirection)));
					}
				}
			}
		}
	}
}

/*
 * mys_delete_clause_deal
 *
 * When lower_case_table_names = 1 or 2, this function is called
 * to convert the string in the structure ColumnRef or RangeVar
 * to lower case.
 */
void mys_delete_clause_deal(List *collist,  List *usinglist)
{
	ColumnRef *cref = NULL;
	RangeVar *rvar = NULL;
	obj_lower lowernum = lower_invalid;
	ListCell *tbcell;
	ListCell *ucell;

	if (mysql_objlower_hook && mysql_string_case_hook)
		lowernum = mysql_objlower_hook();
	if (lower_one != lowernum && lower_two != lowernum)
		return;

	foreach(tbcell, collist)
	{
		if (IsA(lfirst(tbcell), ColumnRef))
		{
			ListCell *tbnamecell;

			cref = (ColumnRef *)lfirst(tbcell);
			foreach(tbnamecell, cref->fields)
			{
				if (IsA(lfirst(tbnamecell), String))
				{
					strVal(lfirst(tbnamecell)) = 
						mysql_string_case_hook(strVal(lfirst(tbnamecell)));
				}
			}
		}
	}

	foreach(ucell, usinglist)
	{
		if (IsA(lfirst(ucell), RangeVar))
		{
			rvar = (RangeVar *)lfirst(ucell);
			rvar->schemaname = mysql_string_case_hook(rvar->schemaname);
			rvar->relname = mysql_string_case_hook(rvar->relname);
		}
	}
}