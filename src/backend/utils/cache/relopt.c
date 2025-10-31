/*-------------------------------------------------------------------------
 *
 * relopt.h
 *	  POSTGRES options functions definition for each kind of relation.
 *
 * Portions Copyright (c) 2025, HighGo Software Co.,Ltd. All rights reserved.
 *
 * src/include/utils/relopt.h
 *
 * add the file for requirement "SRS-SECURITY-TDE"
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/nbtree.h"
#include "access/hash.h"
#include "access/gist_private.h"
#include "access/gin_private.h"
#include "access/spgist_private.h"
#include "access/brin.h"
#include "catalog/pg_am.h"
#include "utils/rel.h"

#define BTIsEncrypt(relation)               \
    ((relation)->rd_options &&              \
    ((BTOptions *) (relation)->rd_options)->encrypt)

#define HashIsEncrypt(relation)             \
    ((relation)->rd_options &&              \
    ((HashOptions *) (relation)->rd_options)->encrypt)

#define GistIsEncrypt(relation)             \
    ((relation)->rd_options &&              \
    ((GiSTOptions *) (relation)->rd_options)->encrypt)

#define GinIsEncrypt(relation)              \
    ((relation)->rd_options &&              \
    ((GinOptions *) (relation)->rd_options)->encrypt)

#define SpGistIsEncrypt(relation)           \
    ((relation)->rd_options &&              \
    ((SpGistOptions *) (relation)->rd_options)->encrypt)

#define BrinIsEncrypt(relation)             \
    ((relation)->rd_options &&              \
    ((BrinOptions *) (relation)->rd_options)->encrypt)

#define HeapIsEncrypted(relation)           \
	((relation)->rd_options &&				\
	((StdRdOptions *) (relation)->rd_options)->encrypt)

inline bool
RelationIsEncrypted(Relation rel)
{
    bool    res = false;

    if (!FileEncryptionEnabled || !rel || !rel->rd_rel)
        return false;

    if (rel->rd_rel->relkind == RELKIND_RELATION ||
        rel->rd_rel->relkind == RELKIND_TOASTVALUE ||
        rel->rd_rel->relkind == RELKIND_MATVIEW)
        res = HeapIsEncrypted(rel);
    else if (rel->rd_rel->relkind == RELKIND_INDEX)
    {
        switch (rel->rd_rel->relam)
        {
            case BTREE_AM_OID:
                res = BTIsEncrypt(rel);
                break;
            case HASH_AM_OID:
                res = HashIsEncrypt(rel);
                break;
            case GIST_AM_OID:
                res = GistIsEncrypt(rel);
                break;
            case GIN_AM_OID:
                res = GinIsEncrypt(rel);
                break;
            case SPGIST_AM_OID:
                res = SpGistIsEncrypt(rel);
                break;
            case BRIN_AM_OID:
                res = BrinIsEncrypt(rel);
                break;
            default:
                /* false */
              break;
        }
    }

    return res;
}
