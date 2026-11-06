/*-------------------------------------------------------------------------
 *
 * keycmd.h
 *    Declarations for KEK and DEK management commands
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/commands/keycmd.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef KEYCMD_H
#define KEYCMD_H

#include "postgres.h"
#include "nodes/parsenodes.h"
#include "catalog/objectaddress.h"

/*-------------------------------------------------------------------------
 * Encryption algorithm bitmask definitions
 *-------------------------------------------------------------------------
 */

/* Encryption algorithm bitmask definitions */
#define HG_ENCRYPTION_RND    0x01    /* 随机加密算法 */
#define HG_ENCRYPTION_DET    0x02    /* 确定性加密算法 */
#define HG_ENCRYPTION_OPE    0x04    /* 保序加密算法 */
#define HG_ENCRYPTION_SEARCH 0x08    /* 可搜索加密算法 */

/* Common algorithm combination macros */
#define HG_ENCRYPTION_RND_ONLY        HG_ENCRYPTION_RND                    /* 0x01 */
#define HG_ENCRYPTION_RND_DET         (HG_ENCRYPTION_RND | HG_ENCRYPTION_DET)     /* 0x03 */
#define HG_ENCRYPTION_RND_OPE         (HG_ENCRYPTION_RND | HG_ENCRYPTION_OPE)     /* 0x05 */
#define HG_ENCRYPTION_RND_SEARCH      (HG_ENCRYPTION_RND | HG_ENCRYPTION_SEARCH)  /* 0x09 */
#define HG_ENCRYPTION_RND_DET_OPE     (HG_ENCRYPTION_RND | HG_ENCRYPTION_DET | HG_ENCRYPTION_OPE) /* 0x07 */
#define HG_ENCRYPTION_ALL             (HG_ENCRYPTION_RND | HG_ENCRYPTION_DET | \
                                       HG_ENCRYPTION_OPE | HG_ENCRYPTION_SEARCH)  /* 0x0F */

/* Algorithm check macros */
#define HG_IS_ENCRYPTION_RND(type)    ((type) & HG_ENCRYPTION_RND)
#define HG_IS_ENCRYPTION_DET(type)    ((type) & HG_ENCRYPTION_DET)
#define HG_IS_ENCRYPTION_OPE(type)    ((type) & HG_ENCRYPTION_OPE)
#define HG_IS_ENCRYPTION_SEARCH(type) ((type) & HG_ENCRYPTION_SEARCH)

/* Validate encryption type (must contain RND, and be a valid combination) */
#define HG_IS_VALID_ENCRYPTION_TYPE(type) \
    (((type) & HG_ENCRYPTION_RND) && ((type) <= HG_ENCRYPTION_ALL) && \
     ((type) != 0) && ((type) != HG_ENCRYPTION_DET) && \
     ((type) != HG_ENCRYPTION_OPE) && ((type) != HG_ENCRYPTION_SEARCH) && \
     ((type) != (HG_ENCRYPTION_DET | HG_ENCRYPTION_OPE)) && \
     ((type) != (HG_ENCRYPTION_DET | HG_ENCRYPTION_SEARCH)) && \
     ((type) != (HG_ENCRYPTION_OPE | HG_ENCRYPTION_SEARCH)) && \
     ((type) != (HG_ENCRYPTION_DET | HG_ENCRYPTION_OPE | HG_ENCRYPTION_SEARCH)))


/* CREATE KEK command handling function */
extern ObjectAddress CreateKek(CreateKekStmt *stmt);

/* Algorithm parameter structure */
typedef struct DekAlgorithmParam
{
    char       *algorithm;         /* Algorithm name: rnd/det/ope/search */
    List       *params;            /* Algorithm-specific parameters */
} DekAlgorithmParam;

/* CREATE DEK command handling function */
extern ObjectAddress CreateDek(CreateDekStmt *stmt);

/*-------------------------------------------------------------------------
 * DROP KEK command
 *-------------------------------------------------------------------------
 */

/* DROP KEK statement structure */
typedef struct DropKekStmt
{
    NodeTag     type;
    List       *key_names;         /* KEK name list */
    char       *schema_name;       /* Schema name (optional) */
    DropBehavior behavior;         /* DROP RESTRICT/CASCADE */
    bool        missing_ok;        /* IF EXISTS option */
} DropKekStmt;

/* DROP KEK command handling function */
extern void DropKek(DropStmt *stmt);

/*-------------------------------------------------------------------------
 * DROP DEK command
 *-------------------------------------------------------------------------
 */

/* DROP DEK statement structure */
typedef struct DropDekStmt
{
    NodeTag     type;
    List       *key_names;         /* DEK name list */
    char       *schema_name;       /* Schema name (optional) */
    DropBehavior behavior;         /* DROP RESTRICT/CASCADE */
    bool        missing_ok;        /* IF EXISTS option */
} DropDekStmt;

/* DROP DEK command handling function */
extern void DropDek(DropStmt *stmt);

/*-------------------------------------------------------------------------
 * ALTER commands (for future use)
 *-------------------------------------------------------------------------
 */

/* ALTER KEK command (placeholder) */
extern ObjectAddress AlterKek(AlterTableStmt *stmt);

/* ALTER DEK command (placeholder) */
extern ObjectAddress AlterDek(AlterTableStmt *stmt);

#endif /* KEYCMD_H */