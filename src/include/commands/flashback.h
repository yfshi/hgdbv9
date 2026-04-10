/*-------------------------------------------------------------------------
 *
 * flashback.c
 *		Implement the functionality of flashback table
 *
 * Portions Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *	  src/include/commands/flashback.h
 *
 * add the file for requirement "SRS-RECOVERY-FLASHBACK"
 *
 *-------------------------------------------------------------------------
 */
#ifndef FLASHBACK_H
#define FLASHBACK_H

#include "parser/parse_node.h"
#include "datatype/timestamp.h"
#include "catalog/objectaddress.h"

typedef enum RbObjType {
    RB_OBJ_TABLE = 0,
    RB_OBJ_INDEX = 1,
    RB_OBJ_TOAST = 2,
    RB_OBJ_TOAST_INDEX = 3,
} RbObjType;

typedef struct RbObjDesc {
    Oid id;
    Oid baseid;
    Oid relid;
    char name[NAMEDATALEN];
    char originname[NAMEDATALEN];
    char operation;
    RbObjType type;
    TimestampTz recycletime;
    Oid nspace;
    Oid tspace;
    Oid relfilenode;
    Oid authid;
} RbObjDesc;

typedef enum RbOperMode {
    RB_OPER_PURGE = 0,
    RB_OPER_RESTORE_DROP = 1,
    RB_OPER_RESTORE_TRUNCATE = 2,
} RbOperMode;

extern void FlashbackToTimepoint(FlashbackStmt *stmt, int nth);
extern bool EnableRecycleBin(void);
extern RbObjType RbGetObjType(Oid nspId, char relKind);
extern void RbRenameClass(RbObjDesc *baseDesc, ObjectAddress *object, char relKind, const char *newName);
extern void RbDescInit(Relation rel, RbObjDesc *desc, char operType, RbObjType objType);
extern void FlashbackBeforeDrop(FlashbackStmt *stmt, int nth);
extern void RbTruncate(List *explicit_rels, List *relids, List *relids_logged, 
                            DropBehavior behavior, bool restart_seqs);
extern void FlashbackBeforeTruncate(FlashbackStmt *stmt, int nth);

extern void PurgeObject(RangeVar *rv);
extern void PurgeAllObjects(void);

#endif  /* FLASHBACK_H */