/*-------------------------------------------------------------------------
 *
 * objecttypecmds.c
 *    Oracle object type command support.
 *
 * This module handles CREATE TYPE / CREATE TYPE BODY for Oracle-compatible
 * object types, including method declaration/validation and dependency
 * tracking between type specifications and their bodies.
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "access/xact.h"
#include "access/htup_details.h"
#include "access/table.h"
#include "catalog/catalog.h"
#include "catalog/dependency.h"
#include "catalog/indexing.h"
#include "catalog/namespace.h"
#include "catalog/objectaccess.h"
#include "catalog/pg_namespace.h"
#include "catalog/pg_language.h"
#include "catalog/pg_object_type.h"
#include "catalog/pg_object_type_body.h"
#include "catalog/pg_type.h"
#include "commands/extension.h"
#include "commands/objecttypecmds.h"
#include "commands/proclang.h"
#include "commands/typecmds.h"
#include "funcapi.h"
#include "miscadmin.h"
#include "nodes/makefuncs.h"
#include "parser/parse_type.h"
#include "utils/acl.h"
#include "utils/builtins.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"
#include "utils/objecttypecache.h"
#include "utils/packagecache.h"
#include "utils/rel.h"
#include "utils/syscache.h"
#include "utils/typcache.h"

static ObjectTypeSpec *ParseObjectTypeSpec(const char *typname,
									 const char *objsrc);
static ObjectTypeBody *ParseObjectTypeBody(const char *typname,
									 const char *bodysrc);
static Oid CreateBaseCompositeType(const char *typname,
							  Oid namespaceId,
							  List *attributes);
static Oid GetObjectTypeByTypid(Oid typoid, bool missing_ok);
static Oid GetObjectTypeTypid(Oid objoid);
static Oid GetObjectTypeParentTypid(Oid objoid);
static Oid GetObjectTypeParentObjectId(Oid objoid);
static bool GetObjectTypeIsFinal(Oid objoid);
static List *BuildObjectTypeAttributeLayout(Oid parenttypid,
										List *attributes);
static void DropShellObjectType(Oid objoid, Oid typoid);
static void ValidateObjectTypeInheritance(const char *typname,
									  const char *schemaName,
									  Oid typoid,
									  ObjectTypeSpec *spec,
									  Oid parentobjoid);
static void ValidateObjectTypeInheritedNameConflicts(const char *typname,
												 ObjectTypeSpec *spec,
												 Oid parentobjoid);
static void ValidateObjectTypeMethodDeclarations(const char *typname,
											 const char *schemaName,
											 Oid typoid,
											 ObjectTypeSpec *spec);
static void ValidateObjectTypeBody(const char *typname,
							   const char *schemaName,
							   Oid typoid,
							   const char *objsrc,
							   const char *bodysrc);
static bool ObjectTypeMethodSignatureEquals(ObjectTypeMethodDecl *spec,
									 ObjectTypeMethodDecl *body,
									 const char *typname,
									 const char *schemaName,
									 Oid typoid);
static bool ObjectTypeMethodTypeEquals(TypeName *left,
								   TypeName *right,
								   const char *typname,
								   const char *schemaName,
								   Oid typoid);
static bool ResolveObjectTypeMethodType(TypeName *typeName,
									const char *typname,
									const char *schemaName,
									Oid typoid,
									Oid *typeid,
									int32 *typmod);
static const char *GetObjectTypeMethodKindName(ObjectTypeMethodDecl *method);
static char *GetObjectTypeMethodDisplayName(ObjectTypeMethodDecl *method);
static ObjectAddress CreateObjectType_internal(const char *typname,
										   Oid namespaceId,
										   Oid ownerId,
										   List *parentname,
										   const char *objsrc,
										   bool replace);
static ObjectAddress CreateObjectTypeBody_internal(const char *typname,
												   Oid typoid,
												   const char *bodysrc,
												   bool replace);

/*
 * CreateObjectType
 *    Main entry point for CREATE TYPE statement.
 *    Handles both new type creation and OR REPLACE scenarios.
 */
ObjectAddress
CreateObjectType(CreateObjectTypeStmt *stmt)
{
	char	   *schemaname;
	char	   *typname;
	Oid			namespaceId;
	Oid			ownerId = GetUserId();
	AclResult	aclresult;

	/* Resolve namespace and type name from qualified name */
	DeconstructQualifiedName(stmt->objname, &schemaname, &typname);
	if (schemaname)
		namespaceId = LookupExplicitNamespace(schemaname, false);
	else
		namespaceId = QualifiedNameGetCreationNamespace(stmt->objname, &typname);

	aclresult = pg_namespace_aclcheck(namespaceId, ownerId, ACL_CREATE);
	if (aclresult != ACLCHECK_OK)
		aclcheck_error(aclresult, OBJECT_SCHEMA, get_namespace_name(namespaceId));

	return CreateObjectType_internal(typname, namespaceId, ownerId,
									 stmt->parentname,
									 stmt->objsrc,
									 stmt->replace);
}

/*
 * CreateObjectTypeBody
 *    Main entry point for CREATE TYPE BODY statement.
 *    Validates that the type specification exists before creating body.
 */
ObjectAddress
CreateObjectTypeBody(CreateObjectTypeBodyStmt *stmt)
{
	char	   *schemaname;
	char	   *typname;
	Oid			typoid;

	/* Resolve namespace and type name from qualified name */
	DeconstructQualifiedName(stmt->objname, &schemaname, &typname);
	if (schemaname)
	{
		Oid namespaceId = LookupExplicitNamespace(schemaname, false);
		typoid = GetSysCacheOid2(TYPENAMENSP, Anum_pg_type_oid,
								 CStringGetDatum(typname),
								 ObjectIdGetDatum(namespaceId));
	}
	else
		typoid = TypenameGetTypidExtended(typname, true);

	if (!OidIsValid(typoid))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("object type \"%s\" does not exist", typname)));

	return CreateObjectTypeBody_internal(typname, typoid,
										 stmt->bodysrc, stmt->replace);
}

/*
 * LookupObjectTypeTypidByNames
 *    Look up the pg_type OID for an object type given its (possibly qualified)
 *    name. Returns InvalidOid if not found and missing_ok is true.
 */
Oid
LookupObjectTypeTypidByNames(List *objname, bool missing_ok)
{
	Oid			objoid;
	HeapTuple	objtup;
	Oid			typoid;

	/* First get the object type OID, then resolve its underlying type OID */
	objoid = LookupObjectTypeByNames(objname, missing_ok);
	if (!OidIsValid(objoid))
		return InvalidOid;

	objtup = SearchSysCache1(OBJTYPEOID, ObjectIdGetDatum(objoid));
	if (!HeapTupleIsValid(objtup))
	{
		if (missing_ok)
			return InvalidOid;
		elog(ERROR, "cache lookup failed for object type %u", objoid);
	}

	typoid = ((Form_pg_object_type) GETSTRUCT(objtup))->typid;
	ReleaseSysCache(objtup);

	return typoid;
}

bool
ObjectTypeHasConstructorDeclarations(Oid typoid)
{
	Oid				objoid;
	HeapTuple		objtup;
	Datum			srcdatum;
	bool			isnull;
	char		   *objsrc;
	char		   *typname;
	ObjectTypeSpec *spec;
	ListCell	   *lc;
	bool			has_constructor = false;

	objoid = GetSysCacheOid1(OBJTYPETYPOID, Anum_pg_object_type_oid,
							 ObjectIdGetDatum(typoid));
	if (!OidIsValid(objoid))
		return false;

	objtup = SearchSysCache1(OBJTYPEOID, ObjectIdGetDatum(objoid));
	if (!HeapTupleIsValid(objtup))
		elog(ERROR, "cache lookup failed for object type %u", objoid);

	srcdatum = SysCacheGetAttr(OBJTYPEOID, objtup,
							   Anum_pg_object_type_objsrc, &isnull);
	objsrc = isnull ? NULL : TextDatumGetCString(srcdatum);
	if (objsrc == NULL)
	{
		ReleaseSysCache(objtup);
		return false;
	}

	typname = pstrdup(get_typename(typoid));
	spec = ParseObjectTypeSpec(typname, objsrc);

	foreach(lc, spec->methods)
	{
		ObjectTypeMethodDecl *method = ((ObjectTypeMethodDecl *) lfirst(lc));

		if (method->kind == OBJECT_TYPE_METHOD_CONSTRUCTOR)
		{
			has_constructor = true;
			break;
		}
	}

	ReleaseSysCache(objtup);
	return has_constructor;
}

/*
 * CreateObjectType_internal
 *    Internal implementation for creating/updating an object type specification.
 *    Parses the type spec, creates the base composite type, and stores metadata
 *    in pg_object_type catalog.
 */
static ObjectAddress
CreateObjectType_internal(const char *typname, Oid namespaceId, Oid ownerId,
						  List *parentname, const char *objsrc, bool replace)
{
	ObjectTypeSpec  *spec;
	Relation		rel;
	TupleDesc		tupDesc;
	HeapTuple		tup;
	HeapTuple		oldtup = NULL;
	Datum			values[Natts_pg_object_type];
	bool			nulls[Natts_pg_object_type];
	bool			replaces[Natts_pg_object_type];
	ObjectAddress	myself,
					referenced;
	ObjectAddresses *addrs;
	Oid				oldtypoid;
	Oid				parenttypid = InvalidOid;
	Oid				parentobjoid = InvalidOid;
	Oid				typoid;
	Oid				objoid;
	int				i;
	bool			is_update = false;

	/* Check if a type with this name already exists in the namespace */
	oldtypoid = GetSysCacheOid2(TYPENAMENSP, Anum_pg_type_oid,
								CStringGetDatum(typname),
								ObjectIdGetDatum(namespaceId));
	if (OidIsValid(oldtypoid))
		oldtup = SearchSysCache1(OBJTYPETYPOID, ObjectIdGetDatum(oldtypoid));

	/* Parse the object type specification source to extract attributes and methods */
	spec = ParseObjectTypeSpec(typname, objsrc);

	if (parentname != NIL)
	{
		parentobjoid = LookupObjectTypeByNames(parentname, false);
		parenttypid = LookupObjectTypeTypidByNames(parentname, false);

		if (GetObjectTypeIsFinal(parentobjoid))
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
					 errmsg("cannot create subtype \"%s\" under FINAL type \"%s\"",
							typname, NameListToString(parentname))));
	}

	if (OidIsValid(parentobjoid))
		ValidateObjectTypeInheritedNameConflicts(typname, spec, parentobjoid);

	/* Handle OR REPLACE case: validate ownership and check for no-op changes */
	if (HeapTupleIsValid(oldtup))
	{
		Form_pg_object_type oldform = (Form_pg_object_type) GETSTRUCT(oldtup);
		Datum srcdatum;
		bool isnull;
		char *oldsrc;

		if (!replace)
			ereport(ERROR,
					(errcode(ERRCODE_DUPLICATE_OBJECT),
					 errmsg("object type \"%s\" already exists", typname)));

		if (!pg_object_type_ownercheck(oldform->oid, ownerId))
			aclcheck_error(ACLCHECK_NOT_OWNER, OBJECT_OBJECT_TYPE, typname);

		/* Compare old and new source to detect no-op updates */
		srcdatum = SysCacheGetAttr(OBJTYPEOID, oldtup,
								   Anum_pg_object_type_objsrc, &isnull);
		oldsrc = isnull ? NULL : TextDatumGetCString(srcdatum);
		if ((oldsrc == NULL && objsrc == NULL) ||
			(oldsrc != NULL && objsrc != NULL && strcmp(oldsrc, objsrc) == 0))
		{
			myself.classId = ObjectTypeRelationId;
			myself.objectId = oldform->oid;
			myself.objectSubId = 0;
			if (oldsrc)
				pfree(oldsrc);
			ReleaseSysCache(oldtup);
			return myself;
		}
		if (oldsrc)
			pfree(oldsrc);

		if (get_typtype(oldform->typid) == TYPTYPE_COMPOSITE)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("CREATE OR REPLACE TYPE is not supported after composite type creation")));

		DropShellObjectType(oldform->oid, oldform->typid);
	}
	else
	{
		/* Check for conflict with non-object types */
		if (OidIsValid(oldtypoid))
			ereport(ERROR,
					(errcode(ERRCODE_DUPLICATE_OBJECT),
					 errmsg("type \"%s\" already exists", typname)));
	}

	/* Create the underlying composite type for attribute storage */
	typoid = CreateBaseCompositeType(typname, namespaceId,
									 BuildObjectTypeAttributeLayout(parenttypid,
																 spec->attributes));
	ValidateObjectTypeMethodDeclarations(typname,
										 get_namespace_name(namespaceId),
										 typoid,
										 spec);
	ValidateObjectTypeInheritance(typname,
								  get_namespace_name(namespaceId),
								  typoid,
								  spec,
								  parentobjoid);

	/* Open pg_object_type catalog and prepare tuple values */
	rel = table_open(ObjectTypeRelationId, RowExclusiveLock);
	tupDesc = RelationGetDescr(rel);

	for (i = 0; i < Natts_pg_object_type; ++i)
	{
		values[i] = (Datum) 0;
		nulls[i] = false;
		replaces[i] = false;
	}

	values[Anum_pg_object_type_typid - 1] = ObjectIdGetDatum(typoid);
	values[Anum_pg_object_type_parenttypid - 1] = ObjectIdGetDatum(parenttypid);
	values[Anum_pg_object_type_objnamespace - 1] = ObjectIdGetDatum(namespaceId);
	values[Anum_pg_object_type_objowner - 1] = ObjectIdGetDatum(ownerId);
	values[Anum_pg_object_type_is_final - 1] = BoolGetDatum(spec->is_final);
	values[Anum_pg_object_type_is_valid - 1] = BoolGetDatum(true);
	if (objsrc)
		values[Anum_pg_object_type_objsrc - 1] = CStringGetTextDatum(objsrc);
	else
		nulls[Anum_pg_object_type_objsrc - 1] = true;

	/* Update existing tuple or insert new one */
	if (HeapTupleIsValid(oldtup))
	{
		replaces[Anum_pg_object_type_typid - 1] = true;
		replaces[Anum_pg_object_type_parenttypid - 1] = true;
		replaces[Anum_pg_object_type_is_final - 1] = true;
		replaces[Anum_pg_object_type_objsrc - 1] = true;
		replaces[Anum_pg_object_type_is_valid - 1] = true;
		tup = heap_modify_tuple(oldtup, tupDesc, values, nulls, replaces);
		CatalogTupleUpdate(rel, &tup->t_self, tup);
		ReleaseSysCache(oldtup);
		is_update = true;
	}
	else
	{
		Oid newOid = GetNewOidWithIndex(rel, ObjectTypeObjectIndexId,
										Anum_pg_object_type_oid);

		values[Anum_pg_object_type_oid - 1] = ObjectIdGetDatum(newOid);
		tup = heap_form_tuple(tupDesc, values, nulls);
		CatalogTupleInsert(rel, tup);
	}

	/* Get the assigned OID and set up object address for dependency tracking */
	objoid = ((Form_pg_object_type) GETSTRUCT(tup))->oid;
	ObjectAddressSet(myself, ObjectTypeRelationId, objoid);

	/* Clean up old dependencies on update */
	if (is_update)
		deleteDependencyRecordsFor(ObjectTypeRelationId, objoid, true);

	/*
	 * The pg_type entry is the user-visible owner of the object type.  Keep
	 * pg_object_type as its internal metadata so that DROP TYPE removes the
	 * metadata (and its AUTO-dependent body) without requiring CASCADE.
	 */
	ObjectAddressSet(referenced, TypeRelationId, typoid);
	recordDependencyOn(&myself, &referenced, DEPENDENCY_INTERNAL);

	/* Record normal dependencies of the object type metadata. */
	addrs = new_object_addresses();
	ObjectAddressSet(referenced, NamespaceRelationId, namespaceId);
	add_exact_object_address(&referenced, addrs);
	ObjectAddressSet(referenced, LanguageRelationId,
					 get_language_oid("plisql", false));
	add_exact_object_address(&referenced, addrs);
	if (OidIsValid(parentobjoid))
	{
		ObjectAddressSet(referenced, ObjectTypeRelationId, parentobjoid);
		add_exact_object_address(&referenced, addrs);
	}
	record_object_address_dependencies(&myself, addrs, DEPENDENCY_NORMAL);
	free_object_addresses(addrs);

	if (!is_update)
		recordDependencyOnOwner(ObjectTypeRelationId, objoid, ownerId);

	recordDependencyOnCurrentExtension(&myself, is_update);
	InvokeObjectPostCreateHook(ObjectTypeRelationId, objoid, 0);

	heap_freetuple(tup);
	table_close(rel, RowExclusiveLock);

	return myself;
}

/*
 * ParseObjectTypeSpec
 *    Invoke the PL/iSQL parser to extract attributes and methods from
 *    the object type specification source string.
 */
static ObjectTypeSpec *
ParseObjectTypeSpec(const char *typname, const char *objsrc)
{
	plisql_internel_funcs_init();

	if (plisql_internal_funcs.object_type_parse_spec == NULL)
		elog(ERROR, "object type spec parser is not registered");

	return plisql_internal_funcs.object_type_parse_spec(typname, objsrc);
}

/*
 * ParseObjectTypeBody
 *    Invoke the PL/iSQL parser to extract method implementations from
 *    the object type body source string.
 */
static ObjectTypeBody *
ParseObjectTypeBody(const char *typname, const char *bodysrc)
{
	plisql_internel_funcs_init();

	if (plisql_internal_funcs.object_type_parse_body == NULL)
		elog(ERROR, "object type body parser is not registered");

	return plisql_internal_funcs.object_type_parse_body(typname, bodysrc);
}

static Oid
GetObjectTypeByTypid(Oid typoid, bool missing_ok)
{
	Oid			objoid;

	objoid = GetSysCacheOid1(OBJTYPETYPOID, Anum_pg_object_type_oid,
							 ObjectIdGetDatum(typoid));
	if (!OidIsValid(objoid) && !missing_ok)
		elog(ERROR, "object type for base type %u does not exist", typoid);

	return objoid;
}

static Oid
GetObjectTypeTypid(Oid objoid)
{
	HeapTuple	objtup;
	Oid			typoid;

	objtup = SearchSysCache1(OBJTYPEOID, ObjectIdGetDatum(objoid));
	if (!HeapTupleIsValid(objtup))
		elog(ERROR, "cache lookup failed for object type %u", objoid);

	typoid = ((Form_pg_object_type) GETSTRUCT(objtup))->typid;
	ReleaseSysCache(objtup);

	return typoid;
}

static Oid
GetObjectTypeParentTypid(Oid objoid)
{
	HeapTuple	tup;
	Oid			parenttypid;

	tup = SearchSysCache1(OBJTYPEOID, ObjectIdGetDatum(objoid));
	if (!HeapTupleIsValid(tup))
		elog(ERROR, "cache lookup failed for object type %u", objoid);

	parenttypid = ((Form_pg_object_type) GETSTRUCT(tup))->parenttypid;
	ReleaseSysCache(tup);

	return parenttypid;
}

static Oid
GetObjectTypeParentObjectId(Oid objoid)
{
	Oid			parenttypid;

	parenttypid = GetObjectTypeParentTypid(objoid);
	if (!OidIsValid(parenttypid))
		return InvalidOid;

	return GetObjectTypeByTypid(parenttypid, false);
}

static bool
GetObjectTypeIsFinal(Oid objoid)
{
	HeapTuple	tup;
	bool		is_final;

	tup = SearchSysCache1(OBJTYPEOID, ObjectIdGetDatum(objoid));
	if (!HeapTupleIsValid(tup))
		elog(ERROR, "cache lookup failed for object type %u", objoid);

	is_final = ((Form_pg_object_type) GETSTRUCT(tup))->is_final;
	ReleaseSysCache(tup);

	return is_final;
}

static List *
BuildObjectTypeAttributeLayout(Oid parenttypid, List *attributes)
{
	List	   *merged = NIL;

	if (OidIsValid(parenttypid))
	{
		TupleDesc	tupdesc;
		int			i;

		tupdesc = lookup_rowtype_tupdesc(parenttypid, -1);
		for (i = 0; i < tupdesc->natts; i++)
		{
			Form_pg_attribute attr = TupleDescAttr(tupdesc, i);

			if (attr->attisdropped)
				continue;

			merged = lappend(merged,
							 makeColumnDef(pstrdup(NameStr(attr->attname)),
										   attr->atttypid,
										   attr->atttypmod,
										   attr->attcollation));
		}
		ReleaseTupleDesc(tupdesc);
	}

	if (attributes != NIL)
		merged = list_concat(merged, copyObject(attributes));

	return merged;
}

/*
 * CreateBaseCompositeType
 *    Create the underlying composite type to store object type attributes.
 *    This provides the storage structure for object type instances.
 */
static Oid
CreateBaseCompositeType(const char *typname, Oid namespaceId, List *attributes)
{
	ObjectAddress address;
	RangeVar   *typevar;

	typevar = makeRangeVar(get_namespace_name(namespaceId), pstrdup(typname), -1);
	address = DefineCompositeType(typevar, attributes);

	return address.objectId;
}

/*
 * DropShellObjectType
 *    Remove a shell object type during OR REPLACE operation.
 *    Cleans up dependencies and drops the underlying composite type.
 */
static void
DropShellObjectType(Oid objoid, Oid typoid)
{
	ObjectAddress shelladdr;

	deleteDependencyRecordsFor(ObjectTypeRelationId, objoid, true);
	ObjectAddressSet(shelladdr, TypeRelationId, typoid);
	performDeletion(&shelladdr, DROP_RESTRICT, 0);
}

static void
ValidateObjectTypeMethodDeclarations(const char *typname,
									 const char *schemaName,
									 Oid typoid,
									 ObjectTypeSpec *spec)
{
	ListCell   *outer_lc;

	foreach(outer_lc, spec->methods)
	{
		ObjectTypeMethodDecl *outer_method =
			((ObjectTypeMethodDecl *) lfirst(outer_lc));
		ListCell   *inner_lc = lnext(spec->methods, outer_lc);

		for (; inner_lc != NULL; inner_lc = lnext(spec->methods, inner_lc))
		{
			ObjectTypeMethodDecl *inner_method =
				((ObjectTypeMethodDecl *) lfirst(inner_lc));

			if (ObjectTypeMethodSignatureEquals(outer_method,
												inner_method,
												typname,
												schemaName,
												typoid))
				ereport(ERROR,
						(errcode(ERRCODE_DUPLICATE_FUNCTION),
						 errmsg("duplicate signature for method %s in object type \"%s\"",
								GetObjectTypeMethodDisplayName(outer_method),
								typname)));
		}
	}
}

static void
ValidateObjectTypeInheritance(const char *typname, const char *schemaName,
							  Oid typoid, ObjectTypeSpec *spec,
							  Oid parentobjoid)
{
	ListCell   *lc;

	if (!OidIsValid(parentobjoid))
		return;

	foreach(lc, spec->methods)
	{
		ObjectTypeMethodDecl *child_method = ((ObjectTypeMethodDecl *) lfirst(lc));
		Oid			current = parentobjoid;
		bool		same_name_seen = false;
		bool		exact_match_seen = false;
		bool		final_match_seen = false;
		bool		cross_kind_seen = false;
		const char *conflict_kind = NULL;

		if (child_method->kind == OBJECT_TYPE_METHOD_CONSTRUCTOR)
		{
			if (child_method->is_override)
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_FUNCTION_DEFINITION),
						 errmsg("constructor \"%s\" in object type \"%s\" cannot be declared OVERRIDING",
								child_method->method_name, typname)));
			continue;
		}

		if (child_method->kind == OBJECT_TYPE_METHOD_STATIC &&
			child_method->is_override)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_FUNCTION_DEFINITION),
					 errmsg("static method %s in object type \"%s\" cannot be declared OVERRIDING",
							GetObjectTypeMethodDisplayName(child_method),
							typname)));

		while (OidIsValid(current) && !exact_match_seen)
		{
			HeapTuple	objtup;
			Datum		srcdatum;
			bool		isnull;
			char	   *ancestor_src;
			char	   *ancestor_name = NULL;
			char	   *ancestor_schema = NULL;
			ObjectTypeSpec *ancestor_spec;
			ListCell   *ancestor_lc;

			objtup = SearchSysCache1(OBJTYPEOID, ObjectIdGetDatum(current));
			if (!HeapTupleIsValid(objtup))
				elog(ERROR, "cache lookup failed for object type %u", current);

			srcdatum = SysCacheGetAttr(OBJTYPEOID, objtup,
									   Anum_pg_object_type_objsrc, &isnull);
			ancestor_src = isnull ? NULL : TextDatumGetCString(srcdatum);
			get_object_type_nameinfo(current, &ancestor_name, &ancestor_schema);
			ancestor_spec = ParseObjectTypeSpec(ancestor_name, ancestor_src);

			foreach(ancestor_lc, ancestor_spec->methods)
			{
				ObjectTypeMethodDecl *ancestor_method;

				ancestor_method = ((ObjectTypeMethodDecl *) lfirst(ancestor_lc));
				if (strcmp(ancestor_method->method_name, child_method->method_name) != 0)
					continue;

				if (ancestor_method->kind != child_method->kind)
				{
					cross_kind_seen = true;
					conflict_kind = GetObjectTypeMethodKindName(ancestor_method);
					break;
				}

				if (ancestor_method->kind == child_method->kind)
				{
					same_name_seen = true;
					if (ObjectTypeMethodSignatureEquals(ancestor_method,
													   child_method,
													   typname,
													   schemaName,
													   typoid))
					{
						exact_match_seen = true;
						final_match_seen = ancestor_method->is_final;
						break;
					}
				}
			}

			ReleaseSysCache(objtup);
			if (cross_kind_seen)
				break;
			current = GetObjectTypeParentObjectId(current);
		}

		if (cross_kind_seen)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_FUNCTION_DEFINITION),
					 errmsg("method %s in object type \"%s\" conflicts with inherited %s method of the same name",
							GetObjectTypeMethodDisplayName(child_method),
							typname,
							conflict_kind)));

		if (child_method->kind == OBJECT_TYPE_METHOD_STATIC && exact_match_seen)
			continue;

		if (child_method->is_override)
		{
			if (!exact_match_seen)
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_FUNCTION_DEFINITION),
						 errmsg("method %s in object type \"%s\" is declared OVERRIDING but no compatible inherited method exists",
								GetObjectTypeMethodDisplayName(child_method), typname)));

			if (final_match_seen)
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_FUNCTION_DEFINITION),
						 errmsg("method %s in object type \"%s\" cannot override a FINAL inherited method",
								GetObjectTypeMethodDisplayName(child_method), typname)));
		}
		else if (exact_match_seen)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_FUNCTION_DEFINITION),
					 errmsg("method %s in object type \"%s\" must be declared OVERRIDING",
							GetObjectTypeMethodDisplayName(child_method), typname)));
		else if (same_name_seen)
		{
			/* Overloading is allowed; nothing else to validate here. */
		}
	}
}

static void
ValidateObjectTypeInheritedNameConflicts(const char *typname,
										 ObjectTypeSpec *spec,
										 Oid parentobjoid)
{
	Oid			parenttypid;
	TupleDesc	tupdesc;
	ListCell   *lc;
	Oid			current;

	if (!OidIsValid(parentobjoid))
		return;

	parenttypid = GetObjectTypeTypid(parentobjoid);
	tupdesc = lookup_rowtype_tupdesc(parenttypid, -1);

	foreach(lc, spec->attributes)
	{
		ColumnDef  *attr = ((ColumnDef *) lfirst(lc));
		int			i;

		for (i = 0; i < tupdesc->natts; i++)
		{
			Form_pg_attribute inherited_attr = TupleDescAttr(tupdesc, i);

			if (inherited_attr->attisdropped)
				continue;

			if (strcmp(attr->colname, NameStr(inherited_attr->attname)) == 0)
				ereport(ERROR,
						(errcode(ERRCODE_DUPLICATE_COLUMN),
						 errmsg("attribute \"%s\" in object type \"%s\" conflicts with inherited attribute of the same name",
								attr->colname,
								typname)));
		}
	}

	foreach(lc, spec->methods)
	{
		ObjectTypeMethodDecl *method = ((ObjectTypeMethodDecl *) lfirst(lc));
		int			i;

		if (method->kind == OBJECT_TYPE_METHOD_CONSTRUCTOR)
			continue;

		for (i = 0; i < tupdesc->natts; i++)
		{
			Form_pg_attribute inherited_attr = TupleDescAttr(tupdesc, i);

			if (inherited_attr->attisdropped)
				continue;

			if (strcmp(method->method_name, NameStr(inherited_attr->attname)) == 0)
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_FUNCTION_DEFINITION),
						 errmsg("method %s in object type \"%s\" conflicts with inherited attribute of the same name",
								GetObjectTypeMethodDisplayName(method),
								typname)));
		}
	}

	ReleaseTupleDesc(tupdesc);

	for (current = parentobjoid; OidIsValid(current);
		 current = GetObjectTypeParentObjectId(current))
	{
		HeapTuple		objtup;
		Datum			srcdatum;
		bool			isnull;
		char		   *ancestor_src;
		char		   *ancestor_name = NULL;
		char		   *ancestor_schema = NULL;
		ObjectTypeSpec *ancestor_spec;
		ListCell	   *ancestor_lc;

		objtup = SearchSysCache1(OBJTYPEOID, ObjectIdGetDatum(current));
		if (!HeapTupleIsValid(objtup))
			elog(ERROR, "cache lookup failed for object type %u", current);

		srcdatum = SysCacheGetAttr(OBJTYPEOID, objtup,
								   Anum_pg_object_type_objsrc, &isnull);
		ancestor_src = isnull ? NULL : TextDatumGetCString(srcdatum);
		get_object_type_nameinfo(current, &ancestor_name, &ancestor_schema);
		ancestor_spec = ParseObjectTypeSpec(ancestor_name, ancestor_src);

		foreach(ancestor_lc, ancestor_spec->methods)
		{
			ObjectTypeMethodDecl *ancestor_method;
			ListCell   *attr_lc;

			ancestor_method = ((ObjectTypeMethodDecl *) lfirst(ancestor_lc));
			if (ancestor_method->kind == OBJECT_TYPE_METHOD_CONSTRUCTOR)
				continue;

			foreach(attr_lc, spec->attributes)
			{
				ColumnDef  *attr = ((ColumnDef *) lfirst(attr_lc));

				if (strcmp(attr->colname, ancestor_method->method_name) == 0)
					ereport(ERROR,
							(errcode(ERRCODE_DUPLICATE_COLUMN),
							 errmsg("attribute \"%s\" in object type \"%s\" conflicts with inherited method of the same name",
									attr->colname,
									typname)));
			}
		}

		ReleaseSysCache(objtup);
	}
}

/*
 * ValidateObjectTypeBody
 *    Validate that the object type body matches its specification.
 *    Each body method must have a corresponding declaration in the spec
 *    with matching signature (name, kind, parameters, return type).
 */
static void
ValidateObjectTypeBody(const char *typname, const char *schemaName,
					   Oid typoid, const char *objsrc, const char *bodysrc)
{
	ObjectTypeSpec *spec;
	ObjectTypeBody *body;
	List	   *matched = NIL;
	ListCell   *lc;

	/* Parse both spec and body to compare method declarations */
	spec = ParseObjectTypeSpec(typname, objsrc);
	body = ParseObjectTypeBody(typname, bodysrc);

	/* Match each body method against spec declarations */
	foreach(lc, body->methods)
	{
		ObjectTypeMethodDecl *body_method = ((ObjectTypeMethodDecl *) lfirst(lc));
		ObjectTypeMethodDecl *exact_match = NULL;
		bool		same_name_seen = false;
		ListCell   *spec_lc;

		foreach(spec_lc, spec->methods)
		{
			ObjectTypeMethodDecl *spec_method;

			spec_method = ((ObjectTypeMethodDecl *) lfirst(spec_lc));
			if (strcmp(spec_method->method_name, body_method->method_name) == 0 &&
				spec_method->kind == body_method->kind)
			{
				same_name_seen = true;
				/* Check if signatures match (parameters and return type) */
				if (ObjectTypeMethodSignatureEquals(spec_method, body_method,
												 typname, schemaName, typoid) &&
					spec_method->is_override == body_method->is_override &&
					spec_method->is_final == body_method->is_final)
				{
					exact_match = spec_method;
					break;
				}
			}
		}

		/* Report errors for unmatched or mismatched methods */
		if (exact_match == NULL)
		{
			char *display_name = GetObjectTypeMethodDisplayName(body_method);

			if (same_name_seen)
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_FUNCTION_DEFINITION),
						 errmsg("method %s in object type body \"%s\" does not match any declaration in the specification",
								display_name, typname)));

			ereport(ERROR,
					(errcode(ERRCODE_INVALID_FUNCTION_DEFINITION),
					 errmsg("method %s in object type body \"%s\" is not declared in the specification",
							display_name, typname)));
		}

		/* Detect duplicate implementations of the same spec method */
		if (list_member_ptr(matched, exact_match))
		{
			char *display_name = GetObjectTypeMethodDisplayName(body_method);

			ereport(ERROR,
					(errcode(ERRCODE_DUPLICATE_FUNCTION),
					 errmsg("duplicate implementation for method %s in object type body \"%s\"",
							display_name, typname)));
		}

		matched = lappend(matched, exact_match);
	}

	/* Verify all spec methods have implementations in the body */
	foreach(lc, spec->methods)
	{
		ObjectTypeMethodDecl *spec_method = ((ObjectTypeMethodDecl *) lfirst(lc));

		if (!list_member_ptr(matched, spec_method))
		{
			char *display_name = GetObjectTypeMethodDisplayName(spec_method);

			ereport(ERROR,
					(errcode(ERRCODE_INVALID_FUNCTION_DEFINITION),
					 errmsg("method %s declared in object type \"%s\" has no implementation in the body",
							display_name, typname)));
		}
	}
}

/*
 * ObjectTypeMethodSignatureEquals
 *    Compare two method declarations for signature equality.
 *    Checks method name, kind (MEMBER/STATIC/CONSTRUCTOR), parameter
 *    count/modes/types, and return type.
 */
static bool
ObjectTypeMethodSignatureEquals(ObjectTypeMethodDecl *spec,
								ObjectTypeMethodDecl *body,
								const char *typname,
								const char *schemaName,
								Oid typoid)
{
	ListCell   *spec_lc;
	ListCell   *body_lc;

	/* Compare method name */
	if (strcmp(spec->method_name, body->method_name) != 0)
		return false;

	/* Compare method kind (MEMBER/STATIC/CONSTRUCTOR) */
	if (spec->kind != body->kind)
		return false;

	/* Compare parameter count */
	if (list_length(spec->parameters) != list_length(body->parameters))
		return false;

	/* Compare return type presence (both have or neither has) */
	if ((spec->returnType == NULL) != (body->returnType == NULL))
		return false;

	/* Compare each parameter's mode and type */
	forboth(spec_lc, spec->parameters, body_lc, body->parameters)
	{
		FunctionParameter *spec_param;
		FunctionParameter *body_param;

		spec_param = ((FunctionParameter *) lfirst(spec_lc));
		body_param = ((FunctionParameter *) lfirst(body_lc));

		if (spec_param->mode != body_param->mode)
			return false;

		if (!ObjectTypeMethodTypeEquals(spec_param->argType, body_param->argType,
									 typname, schemaName, typoid))
			return false;
	}

	/* For procedures, no return type to compare */
	if (spec->returnType == NULL)
		return true;

	/* Compare return types */
	return ObjectTypeMethodTypeEquals(spec->returnType, body->returnType,
								  typname, schemaName, typoid);
}

/*
 * ObjectTypeMethodTypeEquals
 *    Compare two type names for equality, handling SELF type references.
 *    First attempts OID resolution (to handle SELF), falls back to
 *    string comparison if resolution fails.
 */
static bool
ObjectTypeMethodTypeEquals(TypeName *left, TypeName *right,
						   const char *typname, const char *schemaName,
						   Oid typoid)
{
	Oid			leftid = InvalidOid;
	Oid			rightid = InvalidOid;
	int32		leftmod = -1;
	int32		rightmod = -1;

	if (left == NULL || right == NULL)
		return left == right;

	/* Try OID resolution first (handles SELF type references) */
	if (ResolveObjectTypeMethodType(left, typname, schemaName, typoid,
									&leftid, &leftmod) &&
		ResolveObjectTypeMethodType(right, typname, schemaName, typoid,
									&rightid, &rightmod))
		return leftid == rightid && leftmod == rightmod;

	/* Fall back to string comparison if resolution fails */
	return strcmp(TypeNameToString(left), TypeNameToString(right)) == 0;
}

/*
 * ResolveObjectTypeMethodType
 *    Resolve a type name to OID, handling SELF references specially.
 *    If the type name matches the object type being defined (unqualified
 *    or qualified with same schema), return the object's type OID.
 */
static bool
ResolveObjectTypeMethodType(TypeName *typeName, const char *typname,
							const char *schemaName, Oid typoid,
							Oid *typeid, int32 *typmod)
{
	char	   *rawtype;

	*typeid = InvalidOid;
	*typmod = -1;

	/* Check if this is a SELF reference to the object type being defined */
	if (typeName != NULL &&
		typeName->names != NIL &&
		typeName->typeOid == InvalidOid &&
		typeName->pct_type == false &&
		typeName->setof == false &&
		typeName->arrayBounds == NIL)
	{
		char	   *resolved_schema = NULL;
		char	   *resolved_type = NULL;

		DeconstructQualifiedName(typeName->names, &resolved_schema, &resolved_type);
		if (resolved_type != NULL &&
			strcmp(resolved_type, typname) == 0 &&
			((resolved_schema == NULL) ||
			 (schemaName != NULL && strcmp(resolved_schema, schemaName) == 0)))
		{
			*typeid = typoid;
			*typmod = -1;
			return true;
		}
	}

	/* Fall back to standard type resolution */
	rawtype = TypeNameToString(typeName);
	parseTypeString(rawtype, typeid, typmod, true);
	return OidIsValid(*typeid);
}

static const char *
GetObjectTypeMethodKindName(ObjectTypeMethodDecl *method)
{
	if (method->kind == OBJECT_TYPE_METHOD_MEMBER)
		return "MEMBER";
	if (method->kind == OBJECT_TYPE_METHOD_STATIC)
		return "STATIC";

	return "CONSTRUCTOR";
}

/*
 * GetObjectTypeMethodDisplayName
 *    Format a method declaration for display in error messages.
 *    Returns strings like "MEMBER FUNCTION foo" or "CONSTRUCTOR FUNCTION bar".
 */
static char *
GetObjectTypeMethodDisplayName(ObjectTypeMethodDecl *method)
{
	const char *kind;

	/* Determine method kind string based on kind and return type presence */
	if (method->kind == OBJECT_TYPE_METHOD_MEMBER)
		kind = (method->returnType == NULL) ?
			"MEMBER PROCEDURE" : "MEMBER FUNCTION";
	else if (method->kind == OBJECT_TYPE_METHOD_STATIC)
		kind = (method->returnType == NULL) ?
			"STATIC PROCEDURE" : "STATIC FUNCTION";
	else
		kind = "CONSTRUCTOR FUNCTION";

	return psprintf("%s %s", kind, quote_identifier(method->method_name));
}

/*
 * CreateObjectTypeBody_internal
 *    Internal implementation for creating/updating an object type body.
 *    Validates body against specification, then stores in pg_object_type_body.
 */
static ObjectAddress
CreateObjectTypeBody_internal(const char *typname, Oid typoid,
							  const char *bodysrc, bool replace)
{
	Relation		rel;
	TupleDesc		tupDesc;
	HeapTuple		tup;
	HeapTuple		oldtup;
	HeapTuple		objtup;
	Datum			values[Natts_pg_object_type_body];
	bool			nulls[Natts_pg_object_type_body];
	bool			replaces[Natts_pg_object_type_body];
	ObjectAddress	myself,
					referenced;
	Form_pg_object_type objform;
	Oid				objoid;
	Oid				bodyoid;
	Datum			srcdatum;
	bool			isnull;
	char		   *objsrc;
	char		   *schemaName;
	int				i;
	bool			is_update = false;

	/* Look up the object type by its type OID */
	objoid = GetSysCacheOid1(OBJTYPETYPOID, Anum_pg_object_type_oid,
							 ObjectIdGetDatum(typoid));
	if (!OidIsValid(objoid))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("object type \"%s\" does not exist", typname)));

	/* Verify ownership permission */
	if (!pg_object_type_ownercheck(objoid, GetUserId()))
		aclcheck_error(ACLCHECK_NOT_OWNER, OBJECT_OBJECT_TYPE_BODY, typname);

	/* Get the object type tuple and extract specification source */
	objtup = SearchSysCache1(OBJTYPEOID, ObjectIdGetDatum(objoid));
	if (!HeapTupleIsValid(objtup))
		elog(ERROR, "cache lookup failed for object type %u", objoid);

	objform = (Form_pg_object_type) GETSTRUCT(objtup);
	schemaName = get_namespace_name(objform->objnamespace);
	srcdatum = SysCacheGetAttr(OBJTYPEOID, objtup,
							   Anum_pg_object_type_objsrc, &isnull);
	objsrc = isnull ? NULL : TextDatumGetCString(srcdatum);
	if (objsrc == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
				 errmsg("object type \"%s\" has no specification source", typname)));

	/* Open pg_object_type_body catalog and prepare tuple values */
	rel = table_open(ObjectTypeBodyRelationId, RowExclusiveLock);
	tupDesc = RelationGetDescr(rel);

	for (i = 0; i < Natts_pg_object_type_body; ++i)
	{
		values[i] = (Datum) 0;
		nulls[i] = false;
		replaces[i] = true;
	}

	values[Anum_pg_object_type_body_objtypoid - 1] = ObjectIdGetDatum(objoid);
	/* Mark as invalid until validation passes */
	values[Anum_pg_object_type_body_is_valid - 1] = BoolGetDatum(false);
	if (bodysrc)
		values[Anum_pg_object_type_body_bodysrc - 1] = CStringGetTextDatum(bodysrc);
	else
		nulls[Anum_pg_object_type_body_bodysrc - 1] = true;

	/* Check for existing body and handle OR REPLACE */
	oldtup = SearchSysCache1(OBJTYPEBODYOBJTYPOID, ObjectIdGetDatum(objoid));
	if (HeapTupleIsValid(oldtup))
	{
		Datum bodydatum;
		char *oldsrc;
		Form_pg_object_type_body oldform;

		if (!replace)
			ereport(ERROR,
					(errcode(ERRCODE_DUPLICATE_OBJECT),
					 errmsg("object type body \"%s\" already exists", typname)));

		oldform = (Form_pg_object_type_body) GETSTRUCT(oldtup);
		bodydatum = SysCacheGetAttr(OBJTYPEBODYOID, oldtup,
									Anum_pg_object_type_body_bodysrc, &isnull);
		oldsrc = isnull ? NULL : TextDatumGetCString(bodydatum);
		/* Check for no-op update (same source) */
		if ((oldsrc == NULL && bodysrc == NULL) ||
			(oldsrc != NULL && bodysrc != NULL && strcmp(oldsrc, bodysrc) == 0))
		{
			if (oldform->is_valid)
			{
				myself.classId = ObjectTypeBodyRelationId;
				myself.objectId = oldform->oid;
				myself.objectSubId = 0;
				if (oldsrc)
					pfree(oldsrc);
				ReleaseSysCache(oldtup);
				ReleaseSysCache(objtup);
				table_close(rel, RowExclusiveLock);
				return myself;
			}
		}
		if (oldsrc)
			pfree(oldsrc);
	}

	/* Validate body against specification before marking valid */
	ValidateObjectTypeBody(typname, schemaName, typoid, objsrc, bodysrc);
	values[Anum_pg_object_type_body_is_valid - 1] = BoolGetDatum(true);

	/* Update existing tuple or insert new one */
	if (HeapTupleIsValid(oldtup))
	{
		replaces[Anum_pg_object_type_body_objtypoid - 1] = false;
		replaces[Anum_pg_object_type_body_oid - 1] = false;
		tup = heap_modify_tuple(oldtup, tupDesc, values, nulls, replaces);
		CatalogTupleUpdate(rel, &tup->t_self, tup);
		ReleaseSysCache(oldtup);
		is_update = true;
	}
	else
	{
		Oid newOid = GetNewOidWithIndex(rel, ObjectTypeBodyObjectIndexId,
										Anum_pg_object_type_body_oid);

		values[Anum_pg_object_type_body_oid - 1] = ObjectIdGetDatum(newOid);
		tup = heap_form_tuple(tupDesc, values, nulls);
		CatalogTupleInsert(rel, tup);
	}

	ReleaseSysCache(objtup);

	/* Get the assigned OID and set up object address */
	bodyoid = ((Form_pg_object_type_body) GETSTRUCT(tup))->oid;
	ObjectAddressSet(myself, ObjectTypeBodyRelationId, bodyoid);

	/* Clean up old dependencies on update */
	if (is_update)
		deleteDependencyRecordsFor(ObjectTypeBodyRelationId, bodyoid, true);

	/* Record dependency on the object type specification (AUTO for cascade drop) */
	ObjectAddressSet(referenced, ObjectTypeRelationId, objoid);
	recordDependencyOn(&myself, &referenced, DEPENDENCY_AUTO);
	recordDependencyOnCurrentExtension(&myself, is_update);
	InvokeObjectPostCreateHook(ObjectTypeBodyRelationId, bodyoid, 0);

	CommandCounterIncrement();
	if (check_function_bodies)
		ObjectTypeValidate(bodyoid, true);

	heap_freetuple(tup);
	table_close(rel, RowExclusiveLock);

	return myself;
}

/*
 * LookupObjectTypeByNames
 *    Look up an object type OID given its (possibly qualified) name.
 *    Returns InvalidOid if not found and missing_ok is true.
 */
Oid
LookupObjectTypeByNames(List *objname, bool missing_ok)
{
	char   *schemaname;
	char   *typname;
	Oid		namespaceId;
	Oid		typoid;
	Oid		objoid;

	if (missing_ok && list_length(objname) > 2)
		return InvalidOid;

	/* Resolve namespace and type name from qualified name */
	DeconstructQualifiedName(objname, &schemaname, &typname);
	if (schemaname)
	{
		namespaceId = LookupExplicitNamespace(schemaname, missing_ok);
		if (!OidIsValid(namespaceId))
			return InvalidOid;
		typoid = GetSysCacheOid2(TYPENAMENSP, Anum_pg_type_oid,
								 CStringGetDatum(typname),
								 ObjectIdGetDatum(namespaceId));
	}
	else
		typoid = TypenameGetTypidExtended(typname, true);
	if (!OidIsValid(typoid))
	{
		if (missing_ok)
			return InvalidOid;
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("object type \"%s\" does not exist", typname)));
	}

	objoid = GetSysCacheOid1(OBJTYPETYPOID, Anum_pg_object_type_oid,
							 ObjectIdGetDatum(typoid));
	if (!OidIsValid(objoid) && !missing_ok)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("object type \"%s\" does not exist", typname)));

	return objoid;
}

/*
 * LookupObjectTypeBodyByNames
 *    Look up an object type body OID given the type's (possibly qualified) name.
 *    Returns InvalidOid if type or body not found and missing_ok is true.
 */
Oid
LookupObjectTypeBodyByNames(List *objname, bool missing_ok)
{
	Oid objoid;
	Oid bodyoid;

	/* First look up the object type, then find its body */
	objoid = LookupObjectTypeByNames(objname, missing_ok);
	if (!OidIsValid(objoid))
		return InvalidOid;

	bodyoid = GetSysCacheOid1(OBJTYPEBODYOBJTYPOID, Anum_pg_object_type_body_oid,
							  ObjectIdGetDatum(objoid));
	if (!OidIsValid(bodyoid) && !missing_ok)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("object type body \"%s\" does not exist",
						NameListToString(objname))));

	return bodyoid;
}

/*
 * pg_object_type_ownercheck
 *    Check if the given role is the owner of the specified object type.
 *    Superusers are always considered owners.
 */
bool
pg_object_type_ownercheck(Oid objoid, Oid roleid)
{
	HeapTuple	tuple;
	Oid			ownerId;

	if (superuser_arg(roleid))
		return true;

	tuple = SearchSysCache1(OBJTYPEOID, ObjectIdGetDatum(objoid));
	if (!HeapTupleIsValid(tuple))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("object type with OID %u does not exist", objoid)));

	ownerId = ((Form_pg_object_type) GETSTRUCT(tuple))->objowner;
	ReleaseSysCache(tuple);

	return has_privs_of_role(roleid, ownerId);
}

/*
 * pg_object_type_body_ownercheck
 *    Check if the given role is the owner of the specified object type body.
 *    Ownership is determined by the parent object type. Superusers are always
 *    considered owners.
 */
bool
pg_object_type_body_ownercheck(Oid bodyoid, Oid roleid)
{
	HeapTuple	tuple;
	Oid			objoid;

	if (superuser_arg(roleid))
		return true;

	tuple = SearchSysCache1(OBJTYPEBODYOID, ObjectIdGetDatum(bodyoid));
	if (!HeapTupleIsValid(tuple))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("object type body with OID %u does not exist", bodyoid)));

	objoid = ((Form_pg_object_type_body) GETSTRUCT(tuple))->objtypoid;
	ReleaseSysCache(tuple);

	return pg_object_type_ownercheck(objoid, roleid);
}

/*
 * DropObjectTypeById
 *    Delete an object type tuple from pg_object_type catalog.
 *    Called during DROP TYPE execution.
 */
void
DropObjectTypeById(Oid objoid)
{
	Relation rel;
	HeapTuple tup;

	rel = table_open(ObjectTypeRelationId, RowExclusiveLock);
	tup = SearchSysCache1(OBJTYPEOID, ObjectIdGetDatum(objoid));
	if (!HeapTupleIsValid(tup))
		elog(ERROR, "cache lookup failed for object type %u", objoid);

	CatalogTupleDelete(rel, &tup->t_self);
	ReleaseSysCache(tup);
	table_close(rel, RowExclusiveLock);
}

/*
 * DropObjectTypeBodyById
 *    Delete an object type body tuple from pg_object_type_body catalog.
 *    Called during DROP TYPE BODY execution.
 */
void
DropObjectTypeBodyById(Oid bodyoid)
{
	Relation rel;
	HeapTuple tup;

	rel = table_open(ObjectTypeBodyRelationId, RowExclusiveLock);
	tup = SearchSysCache1(OBJTYPEBODYOID, ObjectIdGetDatum(bodyoid));
	if (!HeapTupleIsValid(tup))
		elog(ERROR, "cache lookup failed for object type body %u", bodyoid);

	CatalogTupleDelete(rel, &tup->t_self);
	ReleaseSysCache(tup);
	table_close(rel, RowExclusiveLock);
}

/*
 * get_object_type_nameinfo
 *    Retrieve the type name and schema name for an object type.
 *    Either output parameter can be NULL if that value is not needed.
 */
void
get_object_type_nameinfo(Oid objoid, char **typname, char **schema)
{
	HeapTuple			tup;
	Form_pg_object_type form;

	if (typname)
		*typname = NULL;
	if (schema)
		*schema = NULL;

	tup = SearchSysCache1(OBJTYPEOID, ObjectIdGetDatum(objoid));
	if (!HeapTupleIsValid(tup))
		elog(ERROR, "cache lookup failed for object type %u", objoid);

	form = (Form_pg_object_type) GETSTRUCT(tup);
	if (schema)
		*schema = get_namespace_name(form->objnamespace);
	if (typname)
		*typname = pstrdup(get_typename(form->typid));

	ReleaseSysCache(tup);
}

/*
 * get_object_type_nameinfo_bybodyid
 *    Retrieve the type name and schema name given an object type body OID.
 *    Looks up the parent object type first, then retrieves its name info.
 */
void
get_object_type_nameinfo_bybodyid(Oid bodyoid, char **typname, char **schema)
{
	HeapTuple tup;
	Oid objoid;

	if (typname)
		*typname = NULL;
	if (schema)
		*schema = NULL;

	tup = SearchSysCache1(OBJTYPEBODYOID, ObjectIdGetDatum(bodyoid));
	if (!HeapTupleIsValid(tup))
		elog(ERROR, "cache lookup failed for object type body %u", bodyoid);

	objoid = ((Form_pg_object_type_body) GETSTRUCT(tup))->objtypoid;
	ReleaseSysCache(tup);

	get_object_type_nameinfo(objoid, typname, schema);
}

/*
 * get_object_type_name
 *    Convenience function to retrieve just the type name for an object type.
 *    Returns a palloc'd string or NULL on error.
 */
char *
get_object_type_name(Oid objoid)
{
	char *typname = NULL;

	get_object_type_nameinfo(objoid, &typname, NULL);
	return typname;
}
