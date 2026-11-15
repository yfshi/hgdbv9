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
#include "catalog/hg_dek.h"
#include "catalog/hg_dek_args.h"
#include "utils/hg_key_utils.h"
#include "commands/keycmd.h"
#include "utils/bytea.h"
#include "utils/tecc_global_mark.h"
#include "access/relation.h"
#include "common/tecc_algorithm/tecc_sm3_internal.h"
#include "common/tecc_algorithm/tecc_sm4_internal.h"

/* tecc_key functions linked via libpgcommon */
extern int security_encrypt_dek_with_hmac(const unsigned char *plain, size_t plain_len,
                                          const unsigned char *kek, size_t kek_len,
                                          unsigned char **encrypted, size_t *encrypted_len);
extern int security_decrypt_dek_with_hmac(const unsigned char *encrypted, size_t encrypted_len,
                                          const unsigned char *kek, size_t kek_len,
                                          unsigned char **plain, size_t *plain_len);
extern void security_secure_erase(unsigned char *buf, size_t len);
extern int tecc_decrypt_kek_from_buffer(const unsigned char *buf, size_t len,
                                         unsigned char **plain, size_t *plain_len);


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

/* ReqID:TECC4.0 - Result of the KEK key_path dedup check */
typedef enum
{
	TECC_KEK_PATH_OK = 0,
	TECC_KEK_PATH_SAME_FULLPATH_OTHER,	/* identical path used by another KEK (same store) */
	TECC_KEK_PATH_SAME_FULLPATH_SELF,	/* identical path belongs to the excluded KEK itself */
	TECC_KEK_PATH_SAME_BASENAME	/* same file name used by another KEK (other dir) */
} TeccKekPathConflict;

/* Return a pointer to the trailing file-name component of a path. */
static const char *
tecc_key_path_basename(const char *path)
{
	const char *p = path + strlen(path);

	while (p > path && *(p - 1) != '/' && *(p - 1) != '\\')
		p--;
	return p;
}

/*
 * tecc_get_kek_key_store - return the KEY_STORE of the given KEK from
 * hg_kek_args (palloc'd in the caller's memory context), or NULL if absent.
 */
static char *
tecc_get_kek_key_store(Oid kek_id)
{
	Relation	rel;
	SysScanDesc scan;
	HeapTuple	tuple;
	char	   *store = NULL;

	rel = table_open(HgKekArgsRelationId, AccessShareLock);
	scan = systable_beginscan(rel, HgKekArgsKekIdIndexId, true, NULL, 0, NULL);
	while ((tuple = systable_getnext(scan)) != NULL)
	{
		Form_hg_kek_args args = (Form_hg_kek_args) GETSTRUCT(tuple);

		if (args->kek_id == kek_id &&
			strcmp(NameStr(args->function_name), "kek_config") == 0 &&
			strcmp(NameStr(args->key), "KEY_STORE") == 0)
		{
			bool		is_null;
			Datum		val;

			val = heap_getattr(tuple, Anum_hg_kek_args_value,
							   RelationGetDescr(rel), &is_null);
			if (!is_null)
			{
				bytea	   *vb = DatumGetByteaP(val);
				int			vlen = VARSIZE_ANY_EXHDR(vb);
				char	   *vs = (char *) palloc(vlen + 1);

				memcpy(vs, VARDATA_ANY(vb), vlen);
				vs[vlen] = '\0';
				store = vs;
			}
			break;
		}
	}
	systable_endscan(scan);
	table_close(rel, AccessShareLock);

	return store;
}

/*
 * tecc_kek_key_path_check - reject another KEK using the same key file.
 *
 * Scans hg_kek_args for existing KEY_PATH rows and reports the first
 * conflict:
 *   - TECC_KEK_PATH_SAME_FULLPATH_OTHER: another KEK (same key_store)
 *     already uses the identical full path;
 *   - TECC_KEK_PATH_SAME_FULLPATH_SELF: the identical path belongs to
 *     exclude_kek_id itself (a same-path ALTER KEK);
 *   - TECC_KEK_PATH_SAME_BASENAME: another KEK uses the same file name
 *     even in a different directory.
 * A store-mismatch full-path row is skipped (different hardware may share
 * a slot string).  If not NULL, *conflict_path_out is set to the existing
 * KEK's key path (palloc'd) for the basename error message.
 */
static TeccKekPathConflict
tecc_kek_key_path_check(const char *key_path_val, const char *key_store,
						Oid exclude_kek_id, const char **conflict_path_out)
{
	Relation	kek_args_rel;
	SysScanDesc args_scan;
	HeapTuple	args_tuple;
	const char *new_basename;
	TeccKekPathConflict result = TECC_KEK_PATH_OK;

	if (key_path_val == NULL)
		return TECC_KEK_PATH_OK;

	if (conflict_path_out != NULL)
		*conflict_path_out = NULL;

	/* Extract basename of the new KEK's key_path */
	new_basename = tecc_key_path_basename(key_path_val);

	kek_args_rel = table_open(HgKekArgsRelationId, AccessShareLock);
	args_scan = systable_beginscan(kek_args_rel, HgKekArgsKekIdIndexId, true,
								   NULL, 0, NULL);
	while ((args_tuple = systable_getnext(args_scan)) != NULL)
	{
		Form_hg_kek_args args_form = (Form_hg_kek_args) GETSTRUCT(args_tuple);

		if (strcmp(NameStr(args_form->function_name), "kek_config") == 0 &&
			strcmp(NameStr(args_form->key), "KEY_PATH") == 0)
		{
			bool		is_null;
			Datum		val_datum;
			bytea	   *val_bytea;
			int			val_len;
			char	   *val_str;

			val_datum = heap_getattr(args_tuple, Anum_hg_kek_args_value,
									 RelationGetDescr(kek_args_rel), &is_null);
			if (is_null)
				continue;
			val_bytea = DatumGetByteaP(val_datum);
			val_len = VARSIZE_ANY_EXHDR(val_bytea);
			val_str = (char *) palloc(val_len + 1);
			memcpy(val_str, VARDATA_ANY(val_bytea), val_len);
			val_str[val_len] = '\0';

			/* Check 1: exact same full path */
			if (strcmp(val_str, key_path_val) == 0)
			{
				/*
				 * Different KEY_STORE types (e.g. hsmkms vs swxakms) talk to
				 * different hardware — the same KEY_PATH (slot number) is
				 * valid for each.  Only reject when the key_store matches.
				 */
				bool		same_store = true;

				if (key_store != NULL)
				{
					Oid			existing_kek_id = args_form->kek_id;
					bool		ks_is_null;
					SysScanDesc ks_scan;
					HeapTuple	ks_tuple;
					bool		store_mismatch = false;

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
							Datum		ks_datum;

							ks_datum = heap_getattr(ks_tuple,
													Anum_hg_kek_args_value,
													RelationGetDescr(kek_args_rel),
													&ks_is_null);
							if (!ks_is_null)
							{
								bytea	   *ks_bytea = DatumGetByteaP(ks_datum);
								int			ks_len = VARSIZE_ANY_EXHDR(ks_bytea);
								char	   *ks_str = (char *) palloc(ks_len + 1);

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
						same_store = false;
				}

				if (same_store)
				{
					if (args_form->kek_id == exclude_kek_id)
						result = TECC_KEK_PATH_SAME_FULLPATH_SELF;
					else
						result = TECC_KEK_PATH_SAME_FULLPATH_OTHER;
					if (conflict_path_out != NULL)
						*conflict_path_out = val_str;
					break;
				}

				/* Different key store, same KEY_PATH is OK */
				pfree(val_str);
				continue;
			}

			/* Check 2: same filename (basename) in a different directory */
			if (strcmp(tecc_key_path_basename(val_str), new_basename) == 0 &&
				args_form->kek_id != exclude_kek_id)
			{
				result = TECC_KEK_PATH_SAME_BASENAME;
				if (conflict_path_out != NULL)
					*conflict_path_out = val_str;
				break;
			}

			pfree(val_str);
		}
	}
	systable_endscan(args_scan);
	table_close(kek_args_rel, AccessShareLock);

	return result;
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

		/*
		 * ReqID:TECC4.0 - Bug 9332: 与 PG 的 RangeVarGetCreationNamespace
		 * （catalog/namespace.c:497）保持一致。
		 *
		 * GetActiveCreationNamespace() 只是把 recomputeNamespacePath() 算出的
		 * activeCreationNamespace 原样返回：当 search_path 里没有任何**存在**
		 * 的模式时它就是 InvalidOid（例如 SET search_path = sch1 之后 DROP
		 * SCHEMA sch1 —— PG 会把不存在的模式从有效路径里剔除，所以
		 * current_schema() 为 NULL，而 GUC 文本仍是 sch1）。此时若不拦住，
		 * KEY 会被建成 key_namespace = 0 的孤儿行：任何查找/授权/DROP 都按
		 * 命名空间解析，谁也不会再看它一眼（DROP KEK 报 "does not exist"），
		 * 而 tecc_kek_key_path_check 又不按命名空间隔离，这个 KEY_PATH
		 * 会被永久占用，用户既删不掉也换不了。
		 *
		 * 报错文案与 CREATE TABLE 等 PG 对象保持一致，便于用户理解。
		 */
		if (!OidIsValid(namespaceId))
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_SCHEMA),
					 errmsg("no schema has been selected to create in")));
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
        else if (strcmp(defname, "key_algo") == 0)
        {
            /* KEY_ALGO. Canonical values sm4/sm2. */
            if (strcmp(value, "sm4") != 0 && strcmp(value, "sm2") != 0)
                ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                        errmsg("key_algo must be 'sm4' or 'sm2'")));
            /* value stored in hg_kek_args via general store path below */
        }
        else if (strcmp(defname, "algorithm") == 0)
        {
            /* Accept algorithm option */
        }
        else if (strcmp(defname, "key_length") == 0)
        {
            /* Accept key_length option */
        }
        else if (strcmp(defname, "public_key") == 0)
        {
            /* PUBLIC_KEY from pg_dump -- stored in hg_kek_args below */
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
     */
    if (key_path_val != NULL)
    {
        const char *conflict_path = NULL;
        TeccKekPathConflict conflict =
            tecc_kek_key_path_check(key_path_val, key_store, InvalidOid,
                                    &conflict_path);

        if (conflict == TECC_KEK_PATH_SAME_FULLPATH_OTHER)
        {
            ereport(ERROR,
                    (errcode(ERRCODE_DUPLICATE_OBJECT),
                     errmsg("KEY_PATH \"%s\" is already used by another KEK",
                            key_path_val),
                     errdetail("Each KEK must use a unique key file.")));
        }
        else if (conflict == TECC_KEK_PATH_SAME_BASENAME)
        {
            ereport(ERROR,
                    (errcode(ERRCODE_DUPLICATE_OBJECT),
                     errmsg("key file name \"%s\" is already used by another KEK at \"%s\"",
                            tecc_key_path_basename(key_path_val),
                            conflict_path != NULL ? conflict_path : ""),
                     errdetail("Each KEK must have a unique file name, even across different directories. "
                               "Use a different file name, or set the TECC_KEK_PATH environment variable "
                               "for cross-platform support.")));
        }
        /* TECC_KEK_PATH_SAME_FULLPATH_SELF cannot occur for CREATE KEK */
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
        if (strcmp(opt->defname, "public_key") == 0)
        {
            /* ReqID:TECC4.0 - public_key 是 65 字节未压缩点的 hex 字符串（前端
             * 注入），直接 byteain("\x<hex>") 得 65 字节二进制；不能用通用路径
             * 的 hex_encode（会双重编码），也不能用 strlen 截断二进制 0 字节。 */
            size_t slen = strlen(opt->value);
            char *hexbuf = palloc(slen + 3);
            hexbuf[0] = '\\';
            hexbuf[1] = 'x';
            memcpy(hexbuf + 2, opt->value, slen);
            hexbuf[slen + 2] = '\0';
            arg_value = (bytea *) DatumGetPointer(
                DirectFunctionCall1(byteain, CStringGetDatum(hexbuf)));
            pfree(hexbuf);
        }
        else
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
     * ReqID:TECC4.0 - 非对称 KEK 的公钥不再由后端读 .pub 文件（key_path 是
     * 客户端路径，后端读文件依赖"客户端服务端同机"）。改为前端 libpq 在
     * CREATE KEK 时获取公钥（localkms 读 .pub / HSM export_public_key），
     * 通过 public_key 参数传递，后端走上面 kek_options 的 public_key 分支
     * 存储到 PUBLIC_KEY。
     */

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
/*
 * AlterKekCmd -- ALTER KEK statement handler
 *
 * Updates KEK key_path in hg_kek_args.
 */
ObjectAddress
AlterKekCmd(AlterKekStmt *stmt)
{
    char       *kek_name = NULL;
    char       *schema_name = NULL;
    Oid         namespaceId;
    Oid         kekId;
    char       *key_path_val = NULL;
    bool        tecc_processed = false;
    ObjectAddress address;
    ListCell   *opt_lc;

    DeconstructQualifiedName(stmt->names, &schema_name, &kek_name);

    if (schema_name != NULL)
        namespaceId = LookupExplicitNamespace(schema_name, false);
    else
        namespaceId = GetActiveCreationNamespace();

    kekId = get_kek_oid_ext(kek_name, namespaceId, false);
    if (!OidIsValid(kekId))
        ereport(ERROR, (errcode(ERRCODE_UNDEFINED_OBJECT),
                        errmsg("KEK \"%s\" does not exist", kek_name)));

    /*
     * ReqID:TECC4.0 - Bug 9255: KEK 只有 USAGE 一种可授予权限
     * （ACL_ALL_RIGHTS_KEK == ACL_USAGE，acldefault(OBJECT_KEK) 给属主的也只有
     * USAGE），ACL_UPDATE 不是合法权限类型 —— GRANT UPDATE ON KEK 会直接报
     * "invalid privilege type UPDATE for key encryption key"，因此按 ACL_UPDATE
     * 检查会对任何主体恒为假，导致属主本人和已 GRANT USAGE 的用户都无法轮换。
     * 与 CREATE DEK 的检查（createdek.c 的 hg_kek_aclmask(..., ACL_USAGE, ...)）、
     * 客户端 KEK 缓存视图 hg_kek_info 的 has_kek_privilege(k.oid, 'USAGE')
     * 统一为 USAGE。
     */
    {
        AclResult aclresult = hg_kek_aclcheck(kekId, GetUserId(), ACL_USAGE);
        if (aclresult != ACLCHECK_OK)
            aclcheck_error(aclresult, OBJECT_KEK, kek_name);
    }

    foreach(opt_lc, stmt->options)
    {
        DefElem    *def = (DefElem *) lfirst(opt_lc);
        const char *defname = def->defname;
        char       *value = defGetString(def);

        if (strcmp(defname, "key_path") == 0)
            key_path_val = value;
        else if (strcmp(defname, "_tecc_processed") == 0)
            tecc_processed = true;
        else if (strcmp(defname, "algorithm") == 0 ||
                 strcmp(defname, "key_store") == 0 ||
                 strcmp(defname, "key_length") == 0 ||
                 strcmp(defname, "key_algo") == 0)
            ;
        else
            ereport(ERROR, (errcode(ERRCODE_SYNTAX_ERROR),
                            errmsg("unrecognized KEK option \"%s\"", defname)));
    }

    if (!tecc_processed)
        ereport(ERROR, (errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
                        errmsg("ALTER KEK requires TECC encryption mode")));

    if (key_path_val == NULL)
        ereport(ERROR, (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
                        errmsg("KEY_PATH is required for ALTER KEK")));

    /*
     * ReqID:TECC4.0 - Same-path and cross-KEK dedup checks.
     *
     * - A new KEY_PATH identical to the current one is a no-op "rotation"
     *   (DEKs would be re-encrypted with the very same key material).
     *   Reject it so the caller is not misled into believing the KEK
     *   actually changed.
     * - A new KEY_PATH already used by another KEK of the same key_store
     *   would make two logical KEKs share one key file (or one HSM slot);
     *   reject it exactly like CREATE KEK does.
     */
    {
        const char *own_store;
        const char *conflict_path = NULL;
        TeccKekPathConflict conflict;

        own_store = tecc_get_kek_key_store(kekId);
        conflict = tecc_kek_key_path_check(key_path_val, own_store, kekId,
                                           &conflict_path);

        if (conflict == TECC_KEK_PATH_SAME_FULLPATH_SELF)
        {
            ereport(ERROR,
                    (errcode(ERRCODE_DUPLICATE_OBJECT),
                     errmsg("KEY_PATH \"%s\" is identical to the current key path",
                            key_path_val),
                     errdetail("Specify a different KEY_PATH to rotate the KEK.")));
        }
        else if (conflict == TECC_KEK_PATH_SAME_FULLPATH_OTHER)
        {
            ereport(ERROR,
                    (errcode(ERRCODE_DUPLICATE_OBJECT),
                     errmsg("KEY_PATH \"%s\" is already used by another KEK",
                            key_path_val),
                     errdetail("Each KEK must use a unique key file.")));
        }
        else if (conflict == TECC_KEK_PATH_SAME_BASENAME)
        {
            ereport(ERROR,
                    (errcode(ERRCODE_DUPLICATE_OBJECT),
                     errmsg("key file name \"%s\" is already used by another KEK at \"%s\"",
                            tecc_key_path_basename(key_path_val),
                            conflict_path != NULL ? conflict_path : ""),
                     errdetail("Each KEK must have a unique file name, even across different directories.")));
        }
    }

    /* Update KEY_PATH in hg_kek_args */
    {
        Relation    rel = table_open(HgKekArgsRelationId, RowExclusiveLock);
        SysScanDesc scan;
        HeapTuple   tuple;
        bool        found = false;

        scan = systable_beginscan(rel, HgKekArgsKekIdIndexId, true, NULL, 0, NULL);
        while ((tuple = systable_getnext(scan)) != NULL)
        {
            Form_hg_kek_args args = (Form_hg_kek_args) GETSTRUCT(tuple);

            if (args->kek_id == kekId &&
                strcmp(NameStr(args->function_name), "kek_config") == 0 &&
                strcmp(NameStr(args->key), "KEY_PATH") == 0)
            {
                size_t  slen = strlen(key_path_val);
                size_t  hexlen = slen * 2 + 3;
                char   *hexbuf = palloc(hexlen);
                bytea  *new_val;

                hexbuf[0] = '\\';
                hexbuf[1] = 'x';
                hex_encode(key_path_val, slen, hexbuf + 2);
                hexbuf[hexlen - 1] = '\0';

                new_val = (bytea *) DatumGetPointer(
                    DirectFunctionCall1(byteain, CStringGetDatum(hexbuf)));
                pfree(hexbuf);

                {
                    Datum   values[Natts_hg_kek_args];
                    bool    nulls[Natts_hg_kek_args];
                    bool    replaces[Natts_hg_kek_args];

                    memset(values, 0, sizeof(values));
                    memset(nulls, false, sizeof(nulls));
                    memset(replaces, false, sizeof(replaces));

                    values[Anum_hg_kek_args_value - 1] = PointerGetDatum(new_val);
                    replaces[Anum_hg_kek_args_value - 1] = true;

                    tuple = heap_modify_tuple(tuple, RelationGetDescr(rel),
                                              values, nulls, replaces);
                    CatalogTupleUpdate(rel, &tuple->t_self, tuple);
                }
                found = true;
                break;
            }
        }
        systable_endscan(scan);

        if (!found)
        {
            size_t  slen = strlen(key_path_val);
            size_t  hexlen = slen * 2 + 3;
            char   *hexbuf = palloc(hexlen);
            bytea  *new_val;

            hexbuf[0] = '\\';
            hexbuf[1] = 'x';
            hex_encode(key_path_val, slen, hexbuf + 2);
            hexbuf[hexlen - 1] = '\0';

            new_val = (bytea *) DatumGetPointer(
                DirectFunctionCall1(byteain, CStringGetDatum(hexbuf)));
            pfree(hexbuf);

            create_kek_args(kekId, "kek_config", "KEY_PATH", new_val);
        }

        table_close(rel, RowExclusiveLock);
    }

    CacheInvalidateRelcacheByRelid(HgKekRelationId);

    ObjectAddressSet(address, HgKekRelationId, kekId);
    return address;
}

/* End - ReqID:TECC4.0 */
