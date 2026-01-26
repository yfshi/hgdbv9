/*-------------------------------------------------------------------------
 *
 * schemacmds.h
 *	  prototypes for schemacmds.c.
 *
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/commands/schemacmds.h
 *
 *-------------------------------------------------------------------------
 */

#ifndef SCHEMACMDS_H
#define SCHEMACMDS_H

#include "catalog/objectaddress.h"
#include "nodes/parsenodes.h"

extern Oid	CreateSchemaCommand(CreateSchemaStmt *parsetree,
								const char *queryString,
								int stmt_location, int stmt_len);

extern ObjectAddress RenameSchema(const char *oldname, const char *newname);
extern ObjectAddress AlterSchemaOwner(const char *name, Oid newOwnerId);
extern void AlterSchemaOwner_oid(Oid schemaOid, Oid newOwnerId);

/* Begin - ReqID: SRS-SECURITY-ANTITAMPERING */
typedef bool (*SchemaSetAntiTamperRel_type)(uint32 schemaoid, bool isantitamper);
extern PGDLLIMPORT SchemaSetAntiTamperRel_type SchemaSetAntiTamperRel;

typedef bool (*SchemaGetAntiTamperFlag_type)(uint32 schemaoid);
extern PGDLLIMPORT SchemaGetAntiTamperFlag_type SchemaGetAntiTamperFlag;

extern bool IsAntiTamperNameSpace(Oid nspOid);
/* End - ReqID: SRS-SECURITY-ANTITAMPERING */

#endif							/* SCHEMACMDS_H */
