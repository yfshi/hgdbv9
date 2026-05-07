/*-------------------------------------------------------------------------
 *
 * parse_type.h
 *		handle type operations for parser
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/parser/parse_type.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PARSE_TYPE_H
#define PARSE_TYPE_H

#include "access/htup.h"
#include "parser/parse_node.h"

/* Begin - ReqID:SRS-MYSQL-ENUMTYPE */
/* mysql schema.enum('xxx') is not supported, so the list length is 1 */
#define typename_is_enum(T) \
			((T && T->names && (list_length(T->names)==1) && strcmp(strVal(llast(T->names)), "enum") == 0))
/* End - ReqID:SRS-MYSQL-ENUMTYPE */

typedef HeapTuple Type;

extern Type LookupTypeName(ParseState *pstate, const TypeName *typeName,
						   int32 *typmod_p, bool missing_ok);
extern Type LookupTypeNameExtended(ParseState *pstate,
								   const TypeName *typeName, int32 *typmod_p,
								   bool temp_ok, bool missing_ok);
extern Oid	LookupTypeNameOid(ParseState *pstate, const TypeName *typeName,
							  bool missing_ok);
extern Type typenameType(ParseState *pstate, const TypeName *typeName,
						 int32 *typmod_p);
extern Oid	typenameTypeId(ParseState *pstate, const TypeName *typeName);
extern void typenameTypeIdAndMod(ParseState *pstate, const TypeName *typeName,
								 Oid *typeid_p, int32 *typmod_p);

extern char *TypeNameToString(const TypeName *typeName);
/* Begin - ReqID:SRS-SQL-PACKAGE */
extern char *TypeNameToQuoteString(const TypeName *typeName);
/* End - ReqID:SRS-SQL-PACKAGE */
extern char *TypeNameListToString(List *typenames);

extern Oid	LookupCollation(ParseState *pstate, List *collnames, int location);
extern Oid	GetColumnDefCollation(ParseState *pstate, ColumnDef *coldef, Oid typeOid);

extern Type typeidType(Oid id);

extern Oid	typeTypeId(Type tp);
extern int16 typeLen(Type t);
extern bool typeByVal(Type t);
extern char *typeTypeName(Type t);
extern Oid	typeTypeRelid(Type typ);
extern Oid	typeTypeCollation(Type typ);
extern Datum stringTypeDatum(Type tp, char *string, int32 atttypmod);

extern Oid	typeidTypeRelid(Oid type_id);
extern Oid	typeOrDomainTypeRelid(Oid type_id);

extern TypeName *typeStringToTypeName(const char *str);
extern void parseTypeString(const char *str, Oid *typeid_p, int32 *typmod_p, bool missing_ok);

/* true if typeid is composite, or domain over composite, but not RECORD */
#define ISCOMPLEX(typeid) (typeOrDomainTypeRelid(typeid) != InvalidOid)

/* Begin - ReqID:SRS-PLSQL-TYPE */
extern Type LookupOraTypeName(ParseState *pstate, const TypeName *typeName,
			   int32 *typmod_p, bool missing_ok);
extern Type LookupOraTypeNameExtended(ParseState *pstate,
					   const TypeName *typeName, int32 *typmod_p,
					   bool temp_ok, bool missing_ok);
/* End - ReqID:SRS-PLSQL-TYPE */
/* Begin - ReqID:SRS-SQL-SYNONYM */

typedef Type (*SynnameGettype_hook_type)(ParseState *pstate, const TypeName *typeName, int32 *typmod_p, List **synoids, bool missing_ok);
extern PGDLLIMPORT SynnameGettype_hook_type SynnameGettype_hook;
/* End - ReqID:SRS-SQL-SYNONYM */

/* Begin - SRS-MYSQL-HEX-LITERAL */
extern Datum stringTypeDatum_mysql(Type tp, char *string, int32 atttypmod, int64 flag, bool *isnull);
/* End - SRS-MYSQL-HEX-LITERAL */
#endif							/* PARSE_TYPE_H */
