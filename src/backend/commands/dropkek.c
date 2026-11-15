/*-------------------------------------------------------------------------
 *
 * dropkek.c
 *    Implementation of DROP KEK command
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * ReqID:TECC4.0 - DROP KEK command implementation
 * Authored by <wangxp@highgo.com>, 20260306.
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 * IDENTIFICATION
 *    src/backend/commands/dropkek.c
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
#include "utils/lsyscache.h"
#include "utils/rel.h"
#include "utils/syscache.h"

#include "catalog/hg_kek.h"
#include "catalog/hg_kek_args.h"
#include "catalog/hg_dek.h"
#include "catalog/hg_encrypted_columns.h"
#include "catalog/pg_namespace.h"
#include "utils/hg_key_utils.h"
#include "commands/keycmd.h"

/* Begin - ReqID:TECC4.0 */

/*
 * check_dek_has_encrypted_columns - Check if a DEK has any encrypted columns depending on it
 */
static bool
check_dek_has_encrypted_columns(Oid dek_oid)
{
    Relation    enc_col_rel;
    ScanKeyData key[1];
    SysScanDesc scan;
    HeapTuple   tuple;
    bool        has_columns = false;

    elog(DEBUG1, "DROP KEK: Checking encrypted columns for DEK OID %u", dek_oid);

    /* Open hg_encrypted_columns relation */
    enc_col_rel = table_open(HgEncryptedColumnsRelationId, AccessShareLock);

    /* Setup scan key: column_key_id = dek_oid */
    ScanKeyInit(&key[0],
                Anum_hg_encrypted_columns_column_key_id,
                BTEqualStrategyNumber, F_OIDEQ,
                ObjectIdGetDatum(dek_oid));

    scan = systable_beginscan(enc_col_rel, HgEncryptedColumnsColumnKeyIdIndexId, true,
                              NULL, 1, key);

    /* Check if any encrypted columns exist for this DEK */
    tuple = systable_getnext(scan);
    if (HeapTupleIsValid(tuple))
    {
        Form_hg_encrypted_columns enc_form = (Form_hg_encrypted_columns) GETSTRUCT(tuple);
        has_columns = true;
        elog(DEBUG1, "DROP KEK: Found encrypted column OID %u for DEK OID %u (table OID: %u, column: %s)",
             enc_form->oid, dek_oid, enc_form->table_oid, NameStr(enc_form->column_name));
    }
    else
    {
        elog(DEBUG1, "DROP KEK: No encrypted columns found for DEK OID %u", dek_oid);
    }

    systable_endscan(scan);
    table_close(enc_col_rel, AccessShareLock);

    /* ReqID:TECC4.0 - 同态 DEK 的绑定不在 column_key_id 上，而在
     * hg_encrypted_column_args 的 he_add_dek/he_mul_dek 条目里，纯同态列只
     * 有这条检查能发现。否则 DROP KEK ... CASCADE 会连带删掉仍被列引用的
     * 同态 DEK，留下指向不存在密钥的绑定。 */
    if (!has_columns)
    {
        Oid            dek_dist_id = get_dek_dist_id(dek_oid);
        Oid            he_table_oid = InvalidOid;
        char          *he_column_name = NULL;
        const char    *he_arg_key = NULL;

        if (find_he_dek_bound_column(dek_dist_id, &he_table_oid,
                                     &he_column_name, &he_arg_key))
        {
            has_columns = true;
            elog(DEBUG1, "DROP KEK: DEK OID %u is bound as %s key by column \"%s\" (table OID %u)",
                 dek_oid, he_arg_key, he_column_name, he_table_oid);
        }
        if (he_column_name != NULL)
            pfree(he_column_name);
    }

    return has_columns;
}

/*
 * drop_kek
 *    Handle DROP KEK command
 *
 * Based on openGauss drop_global_settings() function
 */
void
DropKek(DropStmt *stmt)
{
    char       *kek_name = NULL;
    ListCell   *cell;
    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning - declare variables at function scope beginning */
    Relation    dek_rel = NULL;
    ScanKeyData dek_key[1];
    SysScanDesc dek_scan = NULL;
    HeapTuple   dek_tuple = NULL;
    bool        has_dek_deps = false;
    List       *dependent_deks = NIL;
    ListCell   *lc = NULL;

    /* Begin - ReqID:TECC4.0 - Add debug logging */
    elog(DEBUG1, "DropKek: called with behavior=%d, missing_ok=%d", stmt->behavior, stmt->missing_ok);
    /* End - ReqID:TECC4.0 */

    foreach(cell, stmt->objects)
    {
        /* Reset per-iteration state variables */
        has_dek_deps = false;
        dependent_deks = NIL;

        Oid            kek_oid = InvalidOid;
        Oid            namespace_id = InvalidOid;
        char           *schema_name = NULL;
        List           *namespace_list = NIL;
        ListCell       *nslc;
        ObjectAddress  kek_addr;
        Relation       kek_rel;
        HeapTuple      tup;
        Form_hg_kek    kek_form;

        /* Deconstruct qualified name */
        DeconstructQualifiedName((List *) lfirst(cell), &schema_name, &kek_name);

        if (schema_name != NULL)
        {
            /* Use explicit namespace */
            namespace_id = LookupExplicitNamespace(schema_name, stmt->missing_ok);
            if (!OidIsValid(namespace_id))
            {
                if (stmt->missing_ok)
                {
                    ereport(NOTICE,
                            (errmsg("key encryption key \"%s\" does not exist, skipping",
                                    kek_name)));
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

        /* Search for KEK in namespace list */
        foreach(nslc, namespace_list)
        {
            namespace_id = lfirst_oid(nslc);
            kek_oid = get_kek_oid_ext(kek_name, namespace_id, true);  /* missing_ok=true for search */
            if (OidIsValid(kek_oid))
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

        /* Check if KEK exists */
        if (!OidIsValid(kek_oid))
        {
            if (!stmt->missing_ok)
            {
                ereport(ERROR,
                        (errcode(ERRCODE_UNDEFINED_OBJECT),
                         errmsg("key encryption key \"%s\" does not exist",
                                kek_name)));
            }
            else
            {
                ereport(NOTICE,
                        (errmsg("key encryption key \"%s\" does not exist, skipping",
                                kek_name)));
                continue;
            }
        }

        /* Get the KEK tuple */
        kek_rel = table_open(HgKekRelationId, RowExclusiveLock);
        tup = SearchSysCache1(HGKEKOID, ObjectIdGetDatum(kek_oid));

        if (HeapTupleIsValid(tup))
        {
            kek_form = (Form_hg_kek) GETSTRUCT(tup);
            elog(DEBUG1, "drop_kek: found tuple with oid=%u, kek_name='%s'",
                 kek_form->oid, NameStr(kek_form->kek_name));
        }


        if (!HeapTupleIsValid(tup))
        {
            table_close(kek_rel, RowExclusiveLock);
            if (!stmt->missing_ok)
            {
                ereport(ERROR,
                        (errcode(ERRCODE_UNDEFINED_OBJECT),
                         errmsg("key encryption key \"%s\" does not exist",
                                kek_name)));
            }
            else
            {
                ereport(NOTICE,
                        (errmsg("key encryption key \"%s\" does not exist, skipping",
                                kek_name)));
                continue;
            }
        }

        /* Check permissions */
        kek_form = (Form_hg_kek) GETSTRUCT(tup);

        /* Owner can drop, superuser can drop. ReqID:TECC4.0 - USAGE privilege is NOT sufficient for DROP */
        /*Begin: ReqId: TECC-v4.0*/
        if (!pg_kek_ownercheck(kek_oid, GetUserId()) &&
            !superuser())
        /*End: ReqId: TECC-v4.0*/
        {
            aclcheck_error(ACLCHECK_NOT_OWNER, OBJECT_KEK, kek_name);
        }

        /* Create object address for dependency tracking */
        kek_addr.classId = HgKekRelationId;
        kek_addr.objectId = kek_oid;
        kek_addr.objectSubId = 0;

        /* Always check for encrypted column dependencies - even with CASCADE */
        /* First, check if any DEKs depend on this KEK */
        /* ReqID:TECC4.0 - Variables declared at function scope beginning to fix C90 warning */

        dek_rel = table_open(HgDekRelationId, AccessShareLock);

        ScanKeyInit(&dek_key[0],
                    Anum_hg_dek_kek_id,
                    BTEqualStrategyNumber, F_OIDEQ,
                    ObjectIdGetDatum(kek_oid));

        dek_scan = systable_beginscan(dek_rel, HgDekKekIdIndexId, true,
                                      NULL, 1, dek_key);

        while ((dek_tuple = systable_getnext(dek_scan)) != NULL)
        {
            Form_hg_dek dek_form = (Form_hg_dek) GETSTRUCT(dek_tuple);
            has_dek_deps = true;
            dependent_deks = lappend_oid(dependent_deks, dek_form->oid);
            elog(DEBUG1, "DROP KEK: Found dependent DEK OID %u (name: %s) on KEK OID %u",
                 dek_form->oid, NameStr(dek_form->dek_name), kek_oid);
        }

        systable_endscan(dek_scan);
        table_close(dek_rel, AccessShareLock);

        /* If there are DEK dependencies, check if any have encrypted columns */
        if (has_dek_deps)
        {
            foreach(lc, dependent_deks)
            {
                Oid dek_oid = lfirst_oid(lc);
                if (check_dek_has_encrypted_columns(dek_oid))
                {
                    /* Found encrypted column dependency - always fail, even with CASCADE */
                    ereport(ERROR,
                            (errcode(ERRCODE_DEPENDENT_OBJECTS_STILL_EXIST),
                             errmsg("cannot drop key encryption key \"%s\" because encrypted columns depend on it",
                                    kek_name)));
                }
            }

            /* If using CASCADE and no encrypted column dependencies, allow deletion */
            if (stmt->behavior == DROP_CASCADE)
            {
            }
            else
            {
                /* Not using CASCADE and there are DEK dependencies */
                ereport(ERROR,
                        (errcode(ERRCODE_DEPENDENT_OBJECTS_STILL_EXIST),
                         errmsg("cannot drop key encryption key \"%s\" because other objects depend on it",
                                kek_name),
                         errhint("Use DROP KEK ... CASCADE to drop the dependent objects too.")));
            }
        }
        list_free(dependent_deks);

        /* Perform deletion with dependency checking */
        performDeletion(&kek_addr, stmt->behavior, 0);

		ReleaseSysCache(tup);
        table_close(kek_rel, RowExclusiveLock);
    }
}

/* End - ReqID:TECC4.0 */