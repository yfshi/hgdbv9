/*-------------------------------------------------------------------------
 *
 * dropdek.c
 *    Implementation of DROP DEK command
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * ReqID:TECC4.0 - DROP DEK command implementation
 * Authored by <wangxp@highgo.com>, 20260306.
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *    src/backend/commands/dropdek.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/genam.h"
#include "access/htup_details.h"
#include "access/stratnum.h"
#include "access/table.h"
#include "access/xact.h"
#include "catalog/dependency.h"
#include "catalog/namespace.h"
#include "catalog/objectaddress.h"
#include "catalog/pg_namespace.h"
#include "commands/dbcommands.h"
#include "commands/defrem.h"
#include "miscadmin.h"
#include "nodes/makefuncs.h"
#include "parser/parse_type.h"
#include "utils/acl.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/inval.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"

#include "catalog/hg_dek.h"
#include "catalog/hg_dek_args.h"
#include "catalog/hg_encrypted_columns.h"
#include "catalog/pg_namespace.h"
#include "utils/hg_key_utils.h"
#include "commands/keycmd.h"

/* Begin - ReqID:TECC4.0 */

/*
 * DropDek
 *    Handle DROP DEK command
 *
 * Based on openGauss drop_column_settings() function
 */
void
DropDek(DropStmt *stmt)
{
    char       *dek_name = NULL;
    ListCell   *cell;

    /* End - ReqID:TECC4.0 */

    foreach(cell, stmt->objects)
    {
        Oid            dek_oid = InvalidOid;
        Oid            namespace_id = InvalidOid;
        char           *schema_name = NULL;
        List           *namespace_list = NIL;
        ListCell       *nslc;
        ScanKeyData    key[1];
        SysScanDesc    scan;
        ObjectAddress  dek_addr;
        Relation       dek_rel;
        HeapTuple      tup;
        Form_hg_dek    dek_form;

        /* Deconstruct qualified name */
        DeconstructQualifiedName((List *) lfirst(cell), &schema_name, &dek_name);

        if (schema_name != NULL)
        {
            /* Use explicit namespace */
            namespace_id = LookupExplicitNamespace(schema_name, stmt->missing_ok);
            if (!OidIsValid(namespace_id))
            {
                if (stmt->missing_ok)
                {
                    ereport(NOTICE,
                            (errmsg("data encryption key \"%s\" does not exist, skipping",
                                    dek_name)));
                    continue;
                }
                else
                {
                    ereport(ERROR,
                            (errcode(ERRCODE_UNDEFINED_SCHEMA),
                             errmsg("schema \"%s\" does not exist", schema_name)));
                }
            }
            namespace_list = list_make1_oid(namespace_id);
        }
        else
        {
            /* Search through all namespaces in the search path */
            namespace_list = fetch_search_path(true);
        }

        /* Search for DEK in namespace list */
        foreach(nslc, namespace_list)
        {
            namespace_id = lfirst_oid(nslc);
            dek_oid = get_dek_oid_ext(dek_name, namespace_id, true);
            if (OidIsValid(dek_oid))
            {
                /* Check we have creation rights in target namespace */
                AclResult    aclresult = pg_namespace_aclcheck(namespace_id,
                                                               GetUserId(),
                                                               ACL_CREATE);
                if (aclresult != ACLCHECK_OK)
                {
                    aclcheck_error(aclresult, OBJECT_SCHEMA,
                                   get_namespace_name(namespace_id));
                }
                break;
            }
        }

        list_free(namespace_list);

        /* Check if DEK exists */
        if (!OidIsValid(dek_oid))
        {
            if (!stmt->missing_ok)
            {
                ereport(ERROR,
                        (errcode(ERRCODE_UNDEFINED_OBJECT),
                         errmsg("data encryption key \"%s\" does not exist",
                                dek_name)));
            }
            else
            {
                ereport(NOTICE,
                        (errmsg("data encryption key \"%s\" does not exist, skipping",
                                dek_name)));
                continue;
            }
        }

        /* Get the DEK tuple */
        dek_rel = table_open(HgDekRelationId, RowExclusiveLock);
        tup = SearchSysCache1(HGDEKOID, ObjectIdGetDatum(dek_oid));

        if (HeapTupleIsValid(tup))
        {
            dek_form = (Form_hg_dek) GETSTRUCT(tup);
            elog(DEBUG1, "DropDek: found tuple with oid=%u, dek_name='%s'",
                 dek_form->oid, NameStr(dek_form->dek_name));
        }
        if (!HeapTupleIsValid(tup))
        {
            table_close(dek_rel, RowExclusiveLock);
            if (!stmt->missing_ok)
            {
                ereport(ERROR,
                        (errcode(ERRCODE_UNDEFINED_OBJECT),
                         errmsg("data encryption key \"%s\" does not exist",
                                dek_name)));
            }
            else
            {
                ereport(NOTICE,
                        (errmsg("data encryption key \"%s\" does not exist, skipping",
                                dek_name)));
                continue;
            }
        }

        /* Check permissions */
        dek_form = (Form_hg_dek) GETSTRUCT(tup);

        /* Owner can drop, superuser can drop. ReqID:TECC4.0 - USAGE privilege is NOT sufficient for DROP */
        /*Begin: ReqId: TECC-v4.0*/
        if (!pg_dek_ownercheck(dek_oid, GetUserId()) &&
            !superuser())
        /*End: ReqId: TECC-v4.0*/
        {
            aclcheck_error(ACLCHECK_NOT_OWNER, OBJECT_DEK, dek_name);
        }

        /* Check if DEK is being used by any encrypted columns */
        /* ReqID:TECC4.0 - Even with CASCADE, dropping a DEK used by encrypted
         * columns is not allowed. Only DROP TABLE/ALTER TABLE DROP COLUMN
         * can clean up encrypted column records. */
        {
            Relation    enc_col_rel;
            ScanKeyData skey[1];
            SysScanDesc escan;
            HeapTuple   etup;

            enc_col_rel = table_open(HgEncryptedColumnsRelationId,
                                     AccessShareLock);
            ScanKeyInit(&skey[0],
                        Anum_hg_encrypted_columns_column_key_id,
                        BTEqualStrategyNumber, F_OIDEQ,
                        ObjectIdGetDatum(dek_oid));
            escan = systable_beginscan(enc_col_rel,
                                       HgEncryptedColumnsColumnKeyIdIndexId,
                                       true, NULL, 1, skey);
            etup = systable_getnext(escan);
            if (HeapTupleIsValid(etup))
            {
                Form_hg_encrypted_columns enc_form =
                    (Form_hg_encrypted_columns) GETSTRUCT(etup);
                systable_endscan(escan);
                table_close(enc_col_rel, AccessShareLock);

                ereport(ERROR,
                        (errcode(ERRCODE_DEPENDENT_OBJECTS_STILL_EXIST),
                         errmsg("cannot drop data encryption key \"%s\" "
                                "because encrypted columns depend on it",
                                dek_name),
                         errdetail("Encrypted column \"%s\" in table OID %u "
                                   "uses this DEK.",
                                   NameStr(enc_form->column_name),
                                   enc_form->table_oid)));
            }
            systable_endscan(escan);
            table_close(enc_col_rel, AccessShareLock);
        }

        /* ReqID:TECC4.0 - 同态 DEK 的绑定不在 column_key_id 上，而在
         * hg_encrypted_column_args 的 he_add_dek/he_mul_dek 条目里，上面的
         * 占用检查看不到。纯同态列的 column_key_id 是空的，于是列还引用着密钥、
         * 密钥却能删掉；重建同名 DEK 时分布式身份标识由"模式名.密钥名"哈希得出、
         * 与原来完全相同，写入点的跨 DEK 校验会照样通过，遗留密文却已解不开。
         * 因此这里必须一并拦住。 */
        {
            Oid            he_table_oid = InvalidOid;
            char          *he_column_name = NULL;
            const char    *he_arg_key = NULL;

            if (find_he_dek_bound_column(dek_form->column_key_distributed_id,
                                         &he_table_oid, &he_column_name,
                                         &he_arg_key))
            {
                ereport(ERROR,
                        (errcode(ERRCODE_DEPENDENT_OBJECTS_STILL_EXIST),
                         errmsg("cannot drop data encryption key \"%s\" "
                                "because encrypted columns depend on it",
                                dek_name),
                         errdetail("Encrypted column \"%s\" in table OID %u "
                                   "uses this DEK as its %s key.",
                                   he_column_name, he_table_oid, he_arg_key)));
            }
            if (he_column_name != NULL)
                pfree(he_column_name);
        }

        /* Create object address for dependency tracking */
        dek_addr.classId = HgDekRelationId;
        dek_addr.objectId = dek_oid;
        dek_addr.objectSubId = 0;

        /* Perform deletion with dependency checking */
        performDeletion(&dek_addr, stmt->behavior, 0);

        /* Invalidate cache to ensure changes are visible */
        CacheInvalidateRelcacheByRelid(HgDekRelationId);

        /* Free tuple based on its source */
			ReleaseSysCache(tup);
        table_close(dek_rel, RowExclusiveLock);
    }
}

/* End - ReqID:TECC4.0 */