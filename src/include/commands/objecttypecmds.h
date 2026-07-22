/*-------------------------------------------------------------------------
 *
 * objecttypecmds.h
 *	  Oracle object type DDL support.
 *
 *-------------------------------------------------------------------------
 */

#ifndef OBJECTTYPECMDS_H
#define OBJECTTYPECMDS_H

#include "catalog/objectaddress.h"
#include "nodes/parsenodes.h"

/* Method kinds for object type methods */
typedef enum ObjectTypeMethodKind
{
	OBJECT_TYPE_METHOD_MEMBER = 1,	/* Instance method (requires SELF) */
	OBJECT_TYPE_METHOD_STATIC,		/* Static method (no SELF) */
	OBJECT_TYPE_METHOD_CONSTRUCTOR	/* Constructor method (returns SELF) */
} ObjectTypeMethodKind;

/*
 * ObjectTypeMethodDecl
 *    Represents a method declaration in an object type.
 */
typedef struct ObjectTypeMethodDecl
{
	ObjectTypeMethodKind kind;		/* Method kind (member/static/constructor) */
	char   *method_name;			/* Method name */
	List   *parameters;				/* List of FunctionParameter */
	TypeName *returnType;			/* Return type (NULL for constructors) */
	bool	is_override;			/* Declared with OVERRIDING */
	bool	is_final;				/* Declared FINAL */
	char   *declsrc;				/* Source of the declaration clause */
	char   *body_src;				/* Source of the method body */
} ObjectTypeMethodDecl;

/*
 * ObjectTypeSpec
 *    Parsed representation of an object type specification.
 */
typedef struct ObjectTypeSpec
{
	char   *type_name;				/* Fully qualified type name */
	List   *attributes;				/* List of TableFuncElement (attributes) */
	List   *methods;				/* List of ObjectTypeMethodDecl */
	bool	is_final;				/* Type-level FINAL / NOT FINAL */
} ObjectTypeSpec;

/*
 * ObjectTypeBody
 *    Parsed representation of an object type body.
 */
typedef struct ObjectTypeBody
{
	char   *type_name;				/* Fully qualified type name */
	List   *methods;				/* List of ObjectTypeMethodDecl with bodies */
} ObjectTypeBody;

extern ObjectAddress CreateObjectType(CreateObjectTypeStmt *stmt);
extern ObjectAddress CreateObjectTypeBody(CreateObjectTypeBodyStmt *stmt);
extern Oid LookupObjectTypeByNames(List *objname, bool missing_ok);
extern Oid LookupObjectTypeTypidByNames(List *objname, bool missing_ok);
extern Oid LookupObjectTypeBodyByNames(List *objname, bool missing_ok);
extern bool ObjectTypeHasConstructorDeclarations(Oid typoid);
extern bool pg_object_type_ownercheck(Oid objoid, Oid roleid);
extern bool pg_object_type_body_ownercheck(Oid bodyoid, Oid roleid);
extern void DropObjectTypeById(Oid objoid);
extern void DropObjectTypeBodyById(Oid bodyoid);
extern void get_object_type_nameinfo(Oid objoid, char **typname, char **schema);
extern void get_object_type_nameinfo_bybodyid(Oid bodyoid, char **typname,
											  char **schema);
extern char *get_object_type_name(Oid objoid);

#endif
