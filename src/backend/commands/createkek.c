/*-------------------------------------------------------------------------
 *
 * createkek.c
 *    Implementation of CREATE KEK command
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/backend/commands/createkek.c
 *
 * ReqID:TECC4.0 - CREATE KEK command implementation
 * Authored by <wangxp@highgo.com>, 20260306.
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/genam.h"
#include "access/heapam.h"
#include "access/htup.h"
#include "access/htup_details.h"
#include "access/tableam.h"
#include "access/xact.h"
#include "catalog/catalog.h"
#include "catalog/dependency.h"
#include "catalog/indexing.h"
#include "catalog/namespace.h"
#include "catalog/objectaccess.h"
#include "catalog/pg_authid.h"
#include "catalog/pg_namespace.h"
#include "catalog/pg_type.h"
#include "utils/acl.h"
#include "utils/inval.h"
#include "commands/defrem.h"
#include "miscadmin.h"
#include "nodes/makefuncs.h"
#include "parser/parse_type.h"
#include "storage/lmgr.h"
#include "utils/acl.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "utils/rel.h"
#include "utils/syscache.h"
#include "utils/timestamp.h"
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "catalog/hg_kek.h"
#include "catalog/hg_kek_args.h"
#include "utils/hg_key_utils.h"
#include "commands/keycmd.h"
#include "utils/bytea.h"
#include "utils/tecc_global_mark.h"
#include "access/relation.h"
#include "common/tecc_algorithm/tecc_sm3_internal.h"
#include "common/tecc_algorithm/tecc_sm4_internal.h"


/* Begin - ReqID:TECC4.0 */

/*
 * create_kek_args
 *    Create KEK argument entry
 */
static void
create_kek_args(Oid kekId, const char *function_name, const char *key, const bytea *value)
{
    Relation rel;
    HeapTuple tuple;
    Datum values[Natts_hg_kek_args];
    bool nulls[Natts_hg_kek_args];
    Oid args_oid;

    /* Open hg_kek_args relation */
    rel = table_open(HgKekArgsRelationId, RowExclusiveLock);

    /* Prepare tuple data */
    memset(values, 0, sizeof(values));
    memset(nulls, false, sizeof(nulls));

    /* oid */
    args_oid = GetNewOidWithIndex(rel, HgKekArgsOidIndexId, Anum_hg_kek_args_oid);
    values[Anum_hg_kek_args_oid - 1] = ObjectIdGetDatum(args_oid);

    /* kek_id */
    values[Anum_hg_kek_args_kek_id - 1] = ObjectIdGetDatum(kekId);

    /* function_name */
    values[Anum_hg_kek_args_function_name - 1] = DirectFunctionCall1(namein,
                                                                   CStringGetDatum(function_name));

    /* key */
    values[Anum_hg_kek_args_key - 1] = DirectFunctionCall1(namein,
                                                          CStringGetDatum(key));

    /* value (bytea) */
    values[Anum_hg_kek_args_value - 1] = PointerGetDatum(value);

    /* Insert the tuple */
    tuple = heap_form_tuple(RelationGetDescr(rel), values, nulls);
    CatalogTupleInsert(rel, tuple);

    /* Record dependency: args depends on KEK */
    {
        ObjectAddress args_addr, kek_addr;
        ObjectAddressSet(args_addr, HgKekArgsRelationId, args_oid);
        ObjectAddressSet(kek_addr, HgKekRelationId, kekId);
        recordDependencyOn(&args_addr, &kek_addr, DEPENDENCY_AUTO);
    }

    /* Clean up */
    heap_freetuple(tuple);
    table_close(rel, RowExclusiveLock);
}

/*
 * CreateKek
 *    Main entry point for CREATE KEK command
 */
ObjectAddress
CreateKek(CreateKekStmt *stmt)
{
    char *kek_name = NULL;
    char *schema_name = NULL;
    bool if_not_exists = stmt->if_not_exists;
    List *options = stmt->options;
    Oid namespaceId;
    Oid ownerId;
    Relation rel;
    HeapTuple tuple;
    Datum values[Natts_hg_kek];
    bool nulls[Natts_hg_kek];
    bool replaces[Natts_hg_kek];
    Oid kekId;
    ObjectAddress address;
    ListCell *option;
    ListCell *opt_lc = NULL;  /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    /* Parse options */
    char *key_store = NULL;
    char *key_path_val = NULL;  /* KEY_PATH value for dedup check and encryption */
    char *kek_file_checksum = NULL;  /* SM3 checksum of key file, injected by frontend */
    bool tecc_processed = false;  /* ReqID:TECC4.0 - Set by frontend when TECC mode is enabled */
    Oid existing_kek_id;

    /* Store options for later insertion into hg_kek_args */
    typedef struct {
        char *defname;
        char *value;
    } KekOption;
    List *kek_options = NIL;
    KekOption *opt = NULL;  /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    /* Variables for option insertion loop */
    bytea *arg_value = NULL;  /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    char *upper_name = NULL;  /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    char *p = NULL;  /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */

	/* Deconstruct qualified name to extract schema and KEK name */
	DeconstructQualifiedName(stmt->names, &schema_name, &kek_name);

	if (schema_name != NULL)
	{
		/* Use the specified schema */
		namespaceId = LookupExplicitNamespace(schema_name, if_not_exists);
		if (!OidIsValid(namespaceId))
			return InvalidObjectAddress;
	}
	else
	{
		/* Use current active creation namespace */
		namespaceId = GetActiveCreationNamespace();
	}
	elog(DEBUG1, "CreateKek: using namespace OID %u for KEK '%s'", namespaceId, kek_name);

    /* Get current user as owner */
    ownerId = GetUserId();

    /* Check schema CREATE permission */
    {
        AclResult aclresult = pg_namespace_aclcheck(namespaceId, ownerId, ACL_CREATE);
        if (aclresult != ACLCHECK_OK)
            aclcheck_error(aclresult, OBJECT_SCHEMA, get_namespace_name(namespaceId));
    }

    /* Check if KEK already exists in the target namespace */
    existing_kek_id = get_kek_oid_ext(kek_name, namespaceId, true);
    elog(DEBUG1, "CreateKek: get_kek_oid returned OID %u (valid=%d)", existing_kek_id, OidIsValid(existing_kek_id));
    if (OidIsValid(existing_kek_id))
    {
        if (if_not_exists)
        {
            ereport(NOTICE,
                    (errmsg("KEK \"%s\" already exists, skipping",
                            kek_name)));
            return InvalidObjectAddress;
        }
        else
        {
            ereport(ERROR,
                    (errcode(ERRCODE_DUPLICATE_OBJECT),
                     errmsg("KEK \"%s\" already exists", kek_name)));
        }
    }

    foreach(option, options)
    {
        DefElem *defel = (DefElem *) lfirst(option);
        const char *defname = defel->defname;
        char *value = defGetString(defel);

        if (strcmp(defname, "key_store") == 0)
        {
            key_store = value;
        }
        /* ReqID:TECC4.0 - Accept all options for storage in hg_kek_args */
        else if (strcmp(defname, "key_path") == 0)
        {
            key_path_val = value;
        }
        else if (strcmp(defname, "algorithm") == 0)
        {
            /* Accept algorithm option */
        }
        else if (strcmp(defname, "key_length") == 0)
        {
            /* Accept key_length option */
        }
        else if (strcmp(defname, "_tecc_processed") == 0)
        {
            /* Internal marker injected by frontend TECC processor.
             * Indicates that the frontend processed this KEK statement
             * with proper TECC encryption mode enabled. */
            tecc_processed = true;
            /* Don't store this internal marker in hg_kek_args */
            continue;
        }
        else if (strcmp(defname, "_kek_file_checksum") == 0)
        {
            /* SM3 checksum of key file contents, injected by frontend.
             * Used for encrypted _KEK_META storage. */
            kek_file_checksum = value;
            /* Don't store this internal marker in hg_kek_args */
            continue;
        }
        else
        {
            ereport(ERROR,
                    (errcode(ERRCODE_SYNTAX_ERROR),
                     errmsg("unrecognized KEK option \"%s\"", defname)));
        }

        /* Store option for later insertion into hg_kek_args */
        opt = (KekOption *) palloc(sizeof(KekOption));  /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
        opt->defname = pstrdup(defname);
        opt->value = pstrdup(value);
        kek_options = lappend(kek_options, opt);
    }

    /* Validate required options */
    if (key_store == NULL)
    {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("KEY_STORE option is required for CREATE KEK")));
    }

    /*
     * ReqID:TECC4.0 - Reject KEK creation without frontend TECC processing.
     * The frontend injects _TECC_PROCESSED when TECC encryption mode is
     * enabled (-C on). Without it, KMS key management hasn't occurred.
     */
    if (!tecc_processed)
    {
        ereport(ERROR,
                (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                 errmsg("CREATE KEK requires TECC encryption mode to be enabled"),
                 errdetail("Use psql -k on or enable encryption mode to create KEKs.")));
    }

    /*
     * Dedup check: reject if another KEK already uses the same KEY_PATH
     * (identical full path), or the same key filename (basename) even if
     * the directory differs.  The latter prevents ambiguity when files
     * with the same name exist in different directories — it also avoids
     * collisions when a cross-platform KEK directory (TECC_KEK_PATH) is
     * in use.
     *
     * Scan hg_kek_args for existing rows with function_name='kek_config'
     * and key='KEY_PATH'.
     */
    if (key_path_val != NULL)
    {
        Relation kek_args_rel;
        SysScanDesc args_scan;
        HeapTuple args_tuple;
        const char *new_basename;

        /* Extract basename of the new KEK's key_path */
        {
            const char *p = key_path_val + strlen(key_path_val);
            while (p > key_path_val && *(p - 1) != '/' && *(p - 1) != '\\')
                p--;
            new_basename = p;
        }

        kek_args_rel = table_open(HgKekArgsRelationId, AccessShareLock);
        args_scan = systable_beginscan(kek_args_rel, HgKekArgsKekIdIndexId, true,
                                       NULL, 0, NULL);
        while ((args_tuple = systable_getnext(args_scan)) != NULL)
        {
            Form_hg_kek_args args_form = (Form_hg_kek_args) GETSTRUCT(args_tuple);

            if (strcmp(NameStr(args_form->function_name), "kek_config") == 0 &&
                strcmp(NameStr(args_form->key), "KEY_PATH") == 0)
            {
                bool is_null;
                Datum val_datum = heap_getattr(args_tuple, Anum_hg_kek_args_value,
                                               RelationGetDescr(kek_args_rel), &is_null);
                if (!is_null)
                {
                    bytea *val_bytea = DatumGetByteaP(val_datum);
                    int val_len = VARSIZE_ANY_EXHDR(val_bytea);
                    char *val_str = (char *) palloc(val_len + 1);
                    memcpy(val_str, VARDATA_ANY(val_bytea), val_len);
                    val_str[val_len] = '\0';

                    /* Check 1: exact same full path */
                    if (strcmp(val_str, key_path_val) == 0)
                    {
                        /*
                         * Different KEY_STORE types (e.g. hsmkms vs swxakms)
                         * talk to different hardware — the same KEY_PATH
                         * (slot number) is valid for each.  Only reject when
                         * the key_store also matches.
                         */
                        if (key_store != NULL)
                        {
                            /* Look up the existing KEK's KEY_STORE */
                            Oid existing_kek_id = args_form->kek_id;
                            bool ks_is_null;
                            Datum ks_datum;
                            SysScanDesc ks_scan;
                            HeapTuple ks_tuple;
                            bool store_mismatch = false;

                            ks_scan = systable_beginscan(kek_args_rel,
                                                         HgKekArgsKekIdIndexId, true,
                                                         NULL, 0, NULL);
                            while ((ks_tuple = systable_getnext(ks_scan)) != NULL)
                            {
                                Form_hg_kek_args ks_form = (Form_hg_kek_args) GETSTRUCT(ks_tuple);
                                if (ks_form->kek_id == existing_kek_id &&
                                    strcmp(NameStr(ks_form->function_name), "kek_config") == 0 &&
                                    strcmp(NameStr(ks_form->key), "KEY_STORE") == 0)
                                {
                                    ks_datum = heap_getattr(ks_tuple,
                                                             Anum_hg_kek_args_value,
                                                             RelationGetDescr(kek_args_rel),
                                                             &ks_is_null);
                                    if (!ks_is_null)
                                    {
                                        bytea *ks_bytea = DatumGetByteaP(ks_datum);
                                        int ks_len = VARSIZE_ANY_EXHDR(ks_bytea);
                                        char *ks_str = (char *) palloc(ks_len + 1);
                                        memcpy(ks_str, VARDATA_ANY(ks_bytea), ks_len);
                                        ks_str[ks_len] = '\0';

                                        if (strcmp(ks_str, key_store) != 0)
                                            store_mismatch = true;
                                        pfree(ks_str);
                                    }
                                    break;
                                }
                            }
                            systable_endscan(ks_scan);

                            if (store_mismatch)
                            {
                                /* Different key store, same KEY_PATH is OK */
                                pfree(val_str);
                                continue;
                            }
                        }

                        systable_endscan(args_scan);
                        table_close(kek_args_rel, AccessShareLock);
                        ereport(ERROR,
                                (errcode(ERRCODE_DUPLICATE_OBJECT),
                                 errmsg("KEY_PATH \"%s\" is already used by another KEK",
                                        key_path_val),
                                 errdetail("Each KEK must use a unique key file.")));
                    }

                    /* Check 2: same filename (basename) in a different directory */
                    {
                        const char *existing_basename;
                        const char *ep = val_str + strlen(val_str);
                        while (ep > val_str && *(ep - 1) != '/' && *(ep - 1) != '\\')
                            ep--;
                        existing_basename = ep;

                        if (strcmp(existing_basename, new_basename) == 0)
                        {
                            systable_endscan(args_scan);
                            table_close(kek_args_rel, AccessShareLock);
                            ereport(ERROR,
                                    (errcode(ERRCODE_DUPLICATE_OBJECT),
                                     errmsg("key file name \"%s\" is already used by another KEK at \"%s\"",
                                            new_basename, val_str),
                                     errdetail("Each KEK must have a unique file name, even across different directories. "
                                               "Use a different file name, or set the TECC_KEK_PATH environment variable "
                                               "for cross-platform support.")));
                        }
                    }

                    pfree(val_str);
                }
            }
        }
        systable_endscan(args_scan);
        table_close(kek_args_rel, AccessShareLock);
    }

    /* Open hg_kek relation */
    rel = table_open(HgKekRelationId, RowExclusiveLock);

    /* Prepare tuple data */
    memset(values, 0, sizeof(values));
    memset(nulls, false, sizeof(nulls));
    memset(replaces, false, sizeof(replaces));

    /* kek_name */
    values[Anum_hg_kek_kek_name - 1] = DirectFunctionCall1(namein,
                                                          CStringGetDatum(kek_name));

    /* key_namespace */
    values[Anum_hg_kek_key_namespace - 1] = ObjectIdGetDatum(namespaceId);

    /* key_owner */
    values[Anum_hg_kek_key_owner - 1] = ObjectIdGetDatum(ownerId);

    /* key_acl - default ACL */
    values[Anum_hg_kek_key_acl - 1] = PointerGetDatum(acldefault(OBJECT_KEK, ownerId));
    nulls[Anum_hg_kek_key_acl - 1] = false;

    /* create_date */
    values[Anum_hg_kek_create_date - 1] = TimestampTzGetDatum(GetCurrentTimestamp());

    /* Get new OID and insert the tuple */
    kekId = GetNewOidWithIndex(rel, HgKekOidIndexId,
                               Anum_hg_kek_oid);
    /* Mark TECC metadata as updated */
    /* tecc_mark_kek_updated(); */

    elog(DEBUG1, "CreateKek: got new OID %u (valid=%d)", kekId, OidIsValid(kekId));

    /* Set OID in values array */
    values[Anum_hg_kek_oid - 1] = ObjectIdGetDatum(kekId);

    /* Note: For CATALOG tables, OID is automatically managed by CatalogTupleInsert */
    tuple = heap_form_tuple(RelationGetDescr(rel), values, nulls);
    CatalogTupleInsert(rel, tuple);

    /* Create dependencies */
    ObjectAddressSet(address, HgKekRelationId, kekId);

    /* Add dependency on namespace */
    recordDependencyOn(&address,
                       &(ObjectAddress){NamespaceRelationId, namespaceId},
                       DEPENDENCY_NORMAL);

    /* Add dependency on owner */
    recordDependencyOnOwner(HgKekRelationId, kekId, ownerId);

    /* Note: Default ACL is already set in the tuple via acldefault() */

    /* Close relation */
    heap_freetuple(tuple);
    table_close(rel, RowExclusiveLock);

    /* Insert stored options into hg_kek_args table */
    foreach(opt_lc, kek_options)
    {
        opt = (KekOption *) lfirst(opt_lc);  /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
        {
            /* String values (like file paths) contain backslashes that byteain
             * would misinterpret.  Hex-encode the raw string bytes so byteain
             * sees a valid "\x...." input. */
            size_t slen = strlen(opt->value);
            size_t hexlen = slen * 2 + 3;  /* "\x" + hex + NUL */
            char *hexbuf = palloc(hexlen);
            hexbuf[0] = '\\';
            hexbuf[1] = 'x';
            hex_encode(opt->value, slen, hexbuf + 2);
            hexbuf[hexlen - 1] = '\0';
            arg_value = (bytea *) DatumGetPointer(
                DirectFunctionCall1(byteain, CStringGetDatum(hexbuf)));
            pfree(hexbuf);
        }

        /* Convert parameter name to uppercase for consistency with CREATE DEK */
        upper_name = pstrdup(opt->defname);
        for (p = upper_name; *p; ++p)
            *p = toupper((unsigned char)*p);

        create_kek_args(kekId, "kek_config", upper_name, arg_value);
        pfree(upper_name);
        pfree(opt->defname);
        pfree(opt->value);
        pfree(opt);
    }
    list_free(kek_options);

    /*
     * ReqID:TECC4.0 - Encrypt KEK metadata as _KEK_META.
     *
     * Construct plaintext "key_store:key_path:file_checksum", encrypt with
     * SM4-CBC using a fixed server-side key, and store SM3(ciphertext) ||
     * ciphertext in hg_kek_args for integrity verification on DROP.
     */
    if (key_store != NULL && key_path_val != NULL && kek_file_checksum != NULL)
    {
        char plaintext[1024];
        int pt_len;
        int padded_len;
        unsigned char *padded = NULL;
        unsigned char *ciphertext = NULL;
        unsigned char iv[16];
        unsigned char digest[32];
        bytea *meta_value;
        unsigned char *meta_bytes;
        int meta_len;
        TeccSM4Context *sm4_ctx;
        static const unsigned char enc_key[16] = {
            0x2b, 0x7e, 0x15, 0x16, 0x28, 0xae, 0xd2, 0xa6,
            0xab, 0xf7, 0xcf, 0x9c, 0x4e, 0x8d, 0x3a, 0x1f
        };

        pt_len = snprintf(plaintext, sizeof(plaintext), "%s:%s:%s",
                          key_store, key_path_val, kek_file_checksum);
        if (pt_len > 0 && pt_len < (int)sizeof(plaintext))
        {
            /* PKCS7 padding to SM4 block size */
            padded_len = ((pt_len / TECC_SM4_BLOCK_SIZE) + 1) * TECC_SM4_BLOCK_SIZE;
            padded = (unsigned char *) palloc(padded_len);
            memcpy(padded, plaintext, pt_len);
            memset(padded + pt_len, padded_len - pt_len, padded_len - pt_len);

            ciphertext = (unsigned char *) palloc(padded_len);
            memset(iv, 0, sizeof(iv));

            sm4_ctx = tecc_sm4_context_new();
            if (sm4_ctx != NULL &&
                tecc_sm4_set_key_encrypt(sm4_ctx, enc_key) == 1)
            {
                tecc_sm4_cbc_encrypt(sm4_ctx, iv, padded, padded_len, ciphertext);

                /* Compute SM3 of ciphertext for integrity */
                tecc_sm3_hash(ciphertext, padded_len, digest);

                /* Build bytea: SM3(ciphertext)[32] || ciphertext[padded_len] */
                meta_len = 32 + padded_len;
                meta_bytes = (unsigned char *) palloc(VARHDRSZ + meta_len);
                SET_VARSIZE(meta_bytes, VARHDRSZ + meta_len);
                memcpy(VARDATA(meta_bytes), digest, 32);
                memcpy(VARDATA(meta_bytes) + 32, ciphertext, padded_len);

                meta_value = (bytea *) meta_bytes;
                create_kek_args(kekId, "kek_config", "_KEK_META", meta_value);
                pfree(meta_bytes);
            }
            else
            {
                elog(WARNING, "Failed to initialize SM4 context for _KEK_META encryption");
            }

            if (sm4_ctx != NULL)
                tecc_sm4_context_free(sm4_ctx);
            pfree(ciphertext);
            pfree(padded);
        }
    }

    /* Invalidate caches */
    CacheInvalidateRelcacheByRelid(HgKekRelationId);

    /* Return object address */
    elog(DEBUG1, "CreateKek: returning object address {classId=%u, objectId=%u}",
         address.classId, address.objectId);
    return address;
}

/* End - ReqID:TECC4.0 */