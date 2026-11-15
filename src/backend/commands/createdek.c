/*-------------------------------------------------------------------------
 *
 * createdek.c
 *    Implementation of CREATE DEK command
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/backend/commands/createdek.c
 *
 * ReqID:TECC4.0 - CREATE DEK command implementation
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
#include "common/hashfn.h"
#include "utils/bytea.h"

#include "catalog/hg_dek.h"
#include "catalog/hg_dek_args.h"
#include "catalog/hg_kek.h"
#include "catalog/hg_kek_args.h"
#include "utils/hg_key_utils.h"
#include "commands/keycmd.h"
#include "utils/tecc_global_mark.h"
/* For backend, we need to implement our own key management functions */
/* #include "common/tecc_key/high_level_api.h"  For key management API - frontend only */
/* #include "common/tecc_key/security.h"        For encryption functions - frontend only */

/* Note: generate_and_encrypt_dek removed - TECC encryption mode required at frontend */
/* #include "common/tecc_algorithm/tecc_sm4_rnd.h" */  /* Replaced by frontend processing */
/* #include "port.h" */                                  /* Replaced by frontend processing */
#include <string.h>


/* Begin - ReqID:TECC4.0 */

/*
 * generate_and_encrypt_dek has been removed.
 * CREATE DEK requires TECC encryption mode to be enabled (psql -k on).
 * The frontend processor generates and encrypts the DEK, then sends the
 * SQL with ENCRYPTED_VALUE to the backend for storage.
 */

/*
 * generate_distributed_id
 *    Generate distributed ID for DEK based on namespace and key name
 */
static Oid
generate_distributed_id(Oid namespace_id, const char *key_name)
{
    char       *namespace_name;
    char       *full_name;
    uint32      hash_value;
    Oid         distributed_id;

    /* Get namespace name */
    namespace_name = get_namespace_name(namespace_id);
    if (namespace_name == NULL)
        ereport(ERROR,
                (errcode(ERRCODE_UNDEFINED_SCHEMA),
                 errmsg("namespace with OID %u does not exist", namespace_id)));

    /* Construct fully qualified name: schema.keyname */
    full_name = psprintf("%s.%s", namespace_name, key_name);

    /* Generate hash value using hash_any */
    hash_value = hash_any((const unsigned char *) full_name, strlen(full_name));

    /* Convert to OID */
    distributed_id = ObjectIdGetDatum(hash_value);

    /* Clean up */
    pfree(full_name);
    pfree(namespace_name);

    return distributed_id;
}

/*
 * create_dek_args
 *    Create DEK argument entry
 */
static void
create_dek_args(Oid dekId, const char *function_name, const char *key, const bytea *value)
{
    Relation rel;
    HeapTuple tuple;
    Datum values[Natts_hg_dek_args];
    bool nulls[Natts_hg_dek_args];
    Oid args_oid;
    /* ReqID:TECC4.0 - Upsert: check for existing (dek_id, key) tuple first */
    SysScanDesc scan;
    ScanKeyData skey[2];
    HeapTuple existing;
    bool found = false;

    /* Open hg_dek_args relation */
    rel = table_open(HgDekArgsRelationId, RowExclusiveLock);

    /* Check if (dek_id, key) already exists (e.g. frontend-injected KEY_ALGO
     * vs backend-parsed key_algo) — if so, update value instead of inserting */
    ScanKeyInit(&skey[0], Anum_hg_dek_args_dek_id, BTEqualStrategyNumber, F_OIDEQ,
                ObjectIdGetDatum(dekId));
    ScanKeyInit(&skey[1], Anum_hg_dek_args_key, BTEqualStrategyNumber, F_NAMEEQ,
                CStringGetDatum(key));
    scan = systable_beginscan(rel, HgDekArgsPkeyIndexId, true, NULL, 2, skey);
    existing = systable_getnext(scan);
    if (HeapTupleIsValid(existing))
        found = true;
    systable_endscan(scan);

    if (found)
    {
        /* Update value of existing tuple */
        bool replaces[Natts_hg_dek_args];

        memset(replaces, false, sizeof(replaces));
        values[Anum_hg_dek_args_value - 1] = PointerGetDatum(value);
        replaces[Anum_hg_dek_args_value - 1] = true;

        tuple = heap_modify_tuple(existing, RelationGetDescr(rel),
                                  values, nulls, replaces);
        CatalogTupleUpdate(rel, &existing->t_self, tuple);
        heap_freetuple(tuple);
        table_close(rel, RowExclusiveLock);
        return;
    }

    /* Prepare tuple data */
    memset(values, 0, sizeof(values));
    memset(nulls, false, sizeof(nulls));

    /* oid */
    args_oid = GetNewOidWithIndex(rel, HgDekArgsOidIndexId, Anum_hg_dek_args_oid);
    values[Anum_hg_dek_args_oid - 1] = ObjectIdGetDatum(args_oid);

    /* dek_id */
    values[Anum_hg_dek_args_dek_id - 1] = ObjectIdGetDatum(dekId);

    /* function_name */
    values[Anum_hg_dek_args_function_name - 1] = DirectFunctionCall1(namein,
                                                                   CStringGetDatum(function_name));

    /* key */
    values[Anum_hg_dek_args_key - 1] = DirectFunctionCall1(namein,
                                                          CStringGetDatum(key));

    /* value (bytea) */
    values[Anum_hg_dek_args_value - 1] = PointerGetDatum(value);

    /* Insert the tuple */
    tuple = heap_form_tuple(RelationGetDescr(rel), values, nulls);
    CatalogTupleInsert(rel, tuple);

    /* Record dependency: args depends on DEK */
    {
        ObjectAddress args_addr, dek_addr;
        ObjectAddressSet(args_addr, HgDekArgsRelationId, args_oid);
        ObjectAddressSet(dek_addr, HgDekRelationId, dekId);
        recordDependencyOn(&args_addr, &dek_addr, DEPENDENCY_AUTO);
    }

    /* Clean up */
    heap_freetuple(tuple);
    table_close(rel, RowExclusiveLock);
}

/*
 * CreateDek
 *    Main entry point for CREATE DEK command
 */
ObjectAddress
CreateDek(CreateDekStmt *stmt)
{
    char *dek_name = NULL;
    char *schema_name = NULL;
    bool if_not_exists = stmt->if_not_exists;
    List *options = stmt->options;
    Oid namespaceId;
    Oid ownerId;
    Relation rel;
    HeapTuple tuple;
    Datum values[Natts_hg_dek];
    bool nulls[Natts_hg_dek];
    bool replaces[Natts_hg_dek];
    Oid dekId;
    ObjectAddress address;
    ListCell *option;
    ListCell *opt_lc = NULL;  /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    Oid existing_dek_id;
    char *kek_name = NULL;
    char *kek_schema_name = NULL;
    char *encrypted_value = NULL;  /* Added: encrypted DEK from frontend */
    int32 key_length = 256;
    bool key_length_explicit = false;  /* Track if KEY_LENGTH was provided in SQL options */
    Oid kekId = InvalidOid;
    Oid distributed_id;
    char *encrypted_dek_hex = NULL;  /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    size_t encrypted_dek_len = 0;    /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    bool tecc_processed = false;     /* ReqID:TECC4.0 - Set by frontend when TECC mode is enabled */

    /* Store options for later insertion into hg_dek_args */
    typedef struct {
        char *defname;
        char *value;
    } DekOption;
    List *dek_options = NIL;
    DekOption *opt = NULL;  /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    char key_length_str[16];  /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
    /* Variables for option insertion loop */
    bytea *arg_value = NULL;  /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */

	/* End - ReqID:TECC4.0 */

	/* Deconstruct qualified name to extract schema and DEK name */
	DeconstructQualifiedName(stmt->names, &schema_name, &dek_name);

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

		/*
		 * ReqID:TECC4.0 - Bug 9332: 与 CreateKek / PG 的
		 * RangeVarGetCreationNamespace（catalog/namespace.c:497）一致。
		 *
		 * 本处原先没有守卫，DEK 靠 generate_distributed_id() 里的
		 * get_namespace_name(0) == NULL 偶然报
		 * "namespace with OID 0 does not exist" 才没落库 —— 那是个实现细节
		 * 外泄的报错，语义上并不表示"没有可创建的模式"。补上守卫后与 KEK
		 * 路径、以及 PG 原生对象（CREATE TABLE）的报错统一。
		 */
		if (!OidIsValid(namespaceId))
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_SCHEMA),
					 errmsg("no schema has been selected to create in")));
	}

    /* Get current user as owner */
    ownerId = GetUserId();

    /* Check schema CREATE permission */
    {
        AclResult aclresult = pg_namespace_aclcheck(namespaceId, ownerId, ACL_CREATE);
        if (aclresult != ACLCHECK_OK)
            aclcheck_error(aclresult, OBJECT_SCHEMA, get_namespace_name(namespaceId));
    }

    /* Check if DEK already exists in the target namespace */
    existing_dek_id = get_dek_oid_ext(dek_name, namespaceId, true);
    if (OidIsValid(existing_dek_id))
    {
        if (if_not_exists)
        {
            ereport(NOTICE,
                    (errmsg("DEK \"%s\" already exists, skipping",
                            dek_name)));
            return InvalidObjectAddress;
        }
        else
        {
            ereport(ERROR,
                    (errcode(ERRCODE_DUPLICATE_OBJECT),
                     errmsg("DEK \"%s\" already exists", dek_name)));
        }
    }

    /* Parse options */
    foreach(option, options)
    {
        DefElem *defel = (DefElem *) lfirst(option);
        const char *defname = defel->defname;
        char *value = defGetString(defel);

        elog(DEBUG1, "CreateDek: parsing option '%s' = '%.64s'", defname, value ? value : "(null)");
        if (strcmp(defname, "kek") == 0)
        {
            kek_name = value;
        }
        else if (strcmp(defname, "support_algo") == 0)
        {
            /* DEK不应该记录算法类型，算法类型只在加密字段中记录（如'det,ope'等）
             * 如果用户提供了support_algo参数，应该报错
             */
            ereport(ERROR,
                    (errcode(ERRCODE_SYNTAX_ERROR),
                     errmsg("DEK option \"support_algo\" is not allowed for CREATE DEK"),
                     errdetail("DEK does not record algorithm types. Algorithm types are specified when creating encrypted columns (e.g., 'det,ope').")));
        }
        else if (strcmp(defname, "key_length") == 0)
        {
            key_length = atoi(value);
            key_length_explicit = true;
        }
        else if (strcmp(defname, "encrypted_value") == 0)
        {
            /* ENCRYPTED_VALUE parameter contains pre-encrypted DEK from frontend */
            encrypted_value = value;
            elog(DEBUG1, "CreateDek: received encrypted_value from frontend, first 32 chars: %.32s", encrypted_value);
        }
        else if (strcmp(defname, "key_algo") == 0)
        {
            /* KEY_ALGO for DEKs (sm4/he-add/he-mul). Stored in hg_dek_args. */
        }
        else if (strcmp(defname, "he_ec_pubkey") == 0 ||
                 strcmp(defname, "he_ec_privkey") == 0 ||
                 strcmp(defname, "he_eg_pubkey") == 0 ||
                 strcmp(defname, "he_eg_privkey") == 0)
        {
            /* ReqID:TECC4.0 - HE key pair material.
             * he_ec_pubkey/he_ec_privkey = EC-ElGamal (add),
             * he_eg_pubkey/he_eg_privkey = FF ElGamal (mul).
             * Public keys plaintext hex; private keys KEK-encrypted hex.
             * Stored in hg_dek_args; used by client encrypt/decrypt and
             * server homomorphic ops. */
        }
        else if (strcmp(defname, "_tecc_processed") == 0)
        {
            /* ReqID:TECC4.0 - Internal marker injected by frontend TECC processor */
            tecc_processed = true;
            /* Don't store this internal marker in hg_dek_args */
            continue;
        }
        else
        {
            ereport(ERROR,
                    (errcode(ERRCODE_SYNTAX_ERROR),
                     errmsg("unrecognized DEK option \"%s\"", defname)));
        }

        /* Store option for later insertion into hg_dek_args */
        opt = (DekOption *) palloc(sizeof(DekOption));  /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
        opt->defname = pstrdup(defname);
        opt->value = pstrdup(value);
        /*
         * ReqID:TECC4.0 - Normalize the KEK reference before storing:
         * keep only the unqualified name (split at the first dot, same
         * rule as the lookup logic below).  Storing the qualified string
         * would make pg_dump prepend the KEK's catalog namespace again
         * and emit "schema.schema.kek", which cannot be restored.
         */
        if (strcmp(defname, "kek") == 0 && value != NULL)
        {
            const char *dotp = strchr(value, '.');

            opt->value = pstrdup(dotp != NULL ? dotp + 1 : value);
        }
        else
            opt->value = pstrdup(value);
        dek_options = lappend(dek_options, opt);
    }

    /* DEK不应该记录算法类型，算法类型只在加密字段中记录（如'det,ope'等）
     * 因此不需要设置默认的algorithm_name参数
     */

    if (key_length == 256 && !key_length_explicit)  /* 默认值，可能用户未提供 */
    {
        snprintf(key_length_str, sizeof(key_length_str), "%d", key_length);
        opt = (DekOption *) palloc(sizeof(DekOption));  /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
        opt->defname = pstrdup("key_length");
        opt->value = pstrdup(key_length_str);
        dek_options = lappend(dek_options, opt);
    }

    /*
     * ReqID:TECC4.0 - Reject DEK creation without frontend TECC processing.
     * The frontend injects _TECC_PROCESSED when TECC encryption mode is
     * enabled (-C on). Without it, the encrypted DEK value hasn't been
     * generated by the frontend.
     */
    if (!tecc_processed)
    {
        ereport(ERROR,
                (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                 errmsg("CREATE DEK requires TECC encryption mode to be enabled"),
                 errdetail("Use psql -k on or enable encryption mode to create DEKs.")));
    }

    /* Validate required options */
    if (kek_name == NULL)
    {
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                 errmsg("KEK option is required for CREATE DEK")));
    }

    /*
     * Parse KEK name to support schema-qualified names (e.g., "schema.kek_name").
     * If the value contains a dot, split into schema name and KEK name,
     * then look up KEK by explicit namespace. Otherwise use search_path resolution.
     */
    {
        char *dot;
        char *kek_name_buf = pstrdup(kek_name);

        dot = strchr(kek_name_buf, '.');
        if (dot != NULL)
        {
            Oid kek_namespace_id;

            *dot = '\0';
            kek_schema_name = kek_name_buf;
            kek_name = dot + 1;

            /* Resolve schema name to OID */
            kek_namespace_id = LookupExplicitNamespace(kek_schema_name, false);

            /* Look up KEK in the specified namespace */
            kekId = get_kek_oid_ext(kek_name, kek_namespace_id, false);

            elog(DEBUG1, "CreateDek: schema-qualified KEK '%s.%s', namespace OID %u, kek OID %u",
                 kek_schema_name, kek_name, kek_namespace_id, kekId);
        }
        else
        {
            /* Unqualified name: use search_path resolution */
            pfree(kek_name_buf);

            kekId = get_kek_oid(kek_name, false);

            elog(DEBUG1, "CreateDek: unqualified KEK '%s', search_path lookup returned OID %u",
                 kek_name, kekId);
        }
    }

    if (!OidIsValid(kekId))
    {
        ereport(ERROR,
                (errcode(ERRCODE_UNDEFINED_OBJECT),
                 errmsg("KEK \"%s\" does not exist", kek_name)));
    }

    /* Check permission on KEK */
    if ((hg_kek_aclmask(kekId, ownerId, ACL_USAGE, ACLMASK_ANY) & ACL_USAGE) == 0)
    {
        aclcheck_error(ACLCHECK_NO_PRIV, OBJECT_KEK, kek_name);
    }

    /* Get encrypted DEK from frontend.
     * The frontend TECC processor generates the DEK, encrypts it with the KEK,
     * and rewrites the SQL with ENCRYPTED_VALUE. If the value is missing,
     * the frontend was unable to encrypt the DEK (e.g. KEK cache miss). */
    if (encrypted_value == NULL)
    {
        ereport(ERROR,
                (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                 errmsg("ENCRYPTED_VALUE is missing, CREATE DEK requires the frontend to provide an encrypted DEK key")));
    }

    /* Use pre-encrypted DEK from frontend */
    encrypted_dek_hex = pstrdup(encrypted_value);
    encrypted_dek_len = strlen(encrypted_dek_hex);
    elog(DEBUG1, "CreateDek: using pre-encrypted DEK from frontend, length=%zu", encrypted_dek_len);

    /* Open hg_dek relation */
    rel = table_open(HgDekRelationId, RowExclusiveLock);

    /* Prepare tuple data */
    memset(values, 0, sizeof(values));
    memset(nulls, false, sizeof(nulls));
    memset(replaces, false, sizeof(replaces));

    /* dek_name */
    values[Anum_hg_dek_dek_name - 1] = DirectFunctionCall1(namein,
                                                          CStringGetDatum(dek_name));

    /* kek_id */
    values[Anum_hg_dek_kek_id - 1] = ObjectIdGetDatum(kekId);

    /* key_namespace */
    values[Anum_hg_dek_key_namespace - 1] = ObjectIdGetDatum(namespaceId);

    /* key_owner */
    values[Anum_hg_dek_key_owner - 1] = ObjectIdGetDatum(ownerId);

    /* key_acl - default ACL */
    values[Anum_hg_dek_key_acl - 1] = PointerGetDatum(acldefault(OBJECT_DEK, ownerId));
    nulls[Anum_hg_dek_key_acl - 1] = false;

    /* column_key_distributed_id - generate distributed ID */
    distributed_id = generate_distributed_id(namespaceId, dek_name);
    values[Anum_hg_dek_column_key_distributed_id - 1] = ObjectIdGetDatum(distributed_id);

    /* create_date */
    values[Anum_hg_dek_create_date - 1] = TimestampTzGetDatum(GetCurrentTimestamp());

    /* Get new OID and insert the tuple */
    dekId = GetNewOidWithIndex(rel, HgDekOidIndexId,
                               Anum_hg_dek_oid);

    elog(DEBUG1, "CreateDek: got OID %u, setting in values array", dekId);

    /* Set OID in values array */
    values[Anum_hg_dek_oid - 1] = ObjectIdGetDatum(dekId);

    /* Note: For CATALOG tables, OID is automatically managed by CatalogTupleInsert */
    tuple = heap_form_tuple(RelationGetDescr(rel), values, nulls);

    CatalogTupleInsert(rel, tuple);

    /* Mark TECC metadata as updated */
    /* tecc_mark_dek_updated(); */

    /* Create dependencies */
    ObjectAddressSet(address, HgDekRelationId, dekId);

    /* Add dependency on namespace */
    recordDependencyOn(&address,
                       &(ObjectAddress){NamespaceRelationId, namespaceId},
                       DEPENDENCY_NORMAL);

    /* Add dependency on owner */
    recordDependencyOnOwner(HgDekRelationId, dekId, ownerId);

    /* Add dependency on KEK */
    elog(DEBUG1, "CreateDek: adding dependency from DEK %u to KEK %u with type DEPENDENCY_NORMAL", dekId, kekId);
    recordDependencyOn(&address,
                       &(ObjectAddress){HgKekRelationId, kekId},
                       DEPENDENCY_NORMAL);

    /* Note: Default ACL is already set in the tuple via acldefault() */

    /* Close relation */
    heap_freetuple(tuple);
    table_close(rel, RowExclusiveLock);

    /* Insert stored options into hg_dek_args table */
    {
        int opt_idx = 0;
        foreach(opt_lc, dek_options)
        {
            opt = (DekOption *) lfirst(opt_lc);  /* ReqID:TECC4.0 - Fix C90 mixed declaration warning */
            arg_value = (bytea *) DatumGetPointer(DirectFunctionCall1(byteain, CStringGetDatum(opt->value)));
            create_dek_args(dekId, "dek_config", opt->defname, arg_value);
            pfree(opt->defname);
            pfree(opt->value);
            pfree(opt);
            opt_idx++;
        }
    }
    list_free(dek_options);

    /* 清理从前端传入的加密DEK值 */
    if (encrypted_dek_hex != NULL)
    {
        pfree(encrypted_dek_hex);
    }

    /* Invalidate caches */
    CacheInvalidateRelcacheByRelid(HgDekRelationId);

    /* Return object address */
    return address;
}

/* End - ReqID:TECC4.0 */