/*-------------------------------------------------------------------------
 *
 * hg_metadata_views.sql
 *    TECC metadata system views for client-side caching support
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * ReqID:TECC4.0 - Metadata views implementation
 * Authored by: Claude Code
 * Copyright (c) 2026, HighGo Software Co.,Ltd. All rights reserved.
 *
 *-------------------------------------------------------------------------
 */

/*
 * KEK Information View
 *
 * Provides complete KEK information including parameters, with permission check.
 * This view joins hg_kek table with hg_kek_args parameter table.
 */
CREATE VIEW hg_kek_info AS
SELECT
    k.oid AS kek_oid,
    k.kek_name,
    n.nspname AS key_namespace_name,
    k.key_namespace,
    k.key_owner AS key_owner_oid,
    r.rolname AS key_owner_name,
    k.key_acl,
    k.create_date,
    ka.function_name,
    ka.key AS arg_key,
    ka.value AS arg_value
FROM hg_kek k
JOIN pg_namespace n ON k.key_namespace = n.oid
JOIN pg_authid r ON k.key_owner = r.oid
LEFT JOIN hg_kek_args ka ON k.oid = ka.kek_id AND ka.function_name = 'kek_config'
WHERE has_kek_privilege(k.oid, 'USAGE');

/*
 * DEK Information View
 *
 * Provides complete DEK information including algorithm parameters, with permission check.
 */
CREATE VIEW hg_dek_info AS
SELECT
    d.oid AS dek_oid,
    d.dek_name,
    d.column_key_distributed_id,
    k.oid AS kek_oid,
    k.kek_name,
    n.nspname AS key_namespace_name,
    d.key_namespace,
    d.key_owner AS key_owner_oid,
    r.rolname AS key_owner_name,
    d.key_acl,
    d.create_date,
    da.function_name,
    da.key AS arg_key,
    da.value AS arg_value
FROM hg_dek d
JOIN hg_kek k ON d.kek_id = k.oid
JOIN pg_namespace n ON d.key_namespace = n.oid
JOIN pg_authid r ON d.key_owner = r.oid
LEFT JOIN hg_dek_args da ON d.oid = da.dek_id AND da.function_name = 'dek_config'
WHERE has_dek_privilege(d.oid, 'USAGE');

/*
 * Encrypted Columns Information View
 *
 * Provides complete encrypted column information.
 */
CREATE VIEW hg_encrypted_columns_info AS
SELECT
    ec.oid AS column_oid,
    ec.table_oid,
    ec.column_attnum,
    ec.column_name,
    ec.data_type_original_oid,
    ec.data_type_original_mod,
    d.dek_name,
    ec.encryption_type,
    CASE
        WHEN ec.collation_oid = 100 THEN (SELECT datcollate FROM pg_database WHERE datname = current_database())
        ELSE col.collcollate
    END AS collation_locale,
    ec.column_key_id,
    ec.create_date
FROM hg_encrypted_columns ec
JOIN hg_dek d ON ec.column_key_id = d.oid
LEFT JOIN pg_collation col ON ec.collation_oid = col.oid
WHERE EXISTS (
    SELECT 1 FROM hg_dek d2
    WHERE d2.oid = ec.column_key_id
);

-- /*
--  * Metadata Version Table
--  *
--  * Tracks metadata changes to support client-side cache invalidation.
--  * Automatically updated via triggers when KEK/DEK/Column metadata changes.
--  */
-- CREATE TABLE hg_metadata_version (
--     version_id BIGINT PRIMARY KEY,
--     last_modified TIMESTAMPTZ NOT NULL DEFAULT CURRENT_TIMESTAMP,
--     change_type TEXT NOT NULL,  -- 'KEK'/'DEK'/'COLUMN'
--     change_count INTEGER NOT NULL DEFAULT 0
-- );

-- /*
--  * Get Current Metadata Version Function
--  *
--  * Returns the current version ID from hg_metadata_version table.
--  * Clients can use this to check if their cache is stale.
--  */
-- CREATE OR REPLACE FUNCTION hg_get_metadata_version()
-- RETURNS BIGINT AS $$
-- DECLARE
--     current_version BIGINT;
-- BEGIN
--     SELECT COALESCE(MAX(version_id), 0) INTO current_version
--     FROM hg_metadata_version;
--     RETURN current_version;
-- END;
-- $$ LANGUAGE plpgsql;

-- /*
--  * Metadata Version Update Trigger Function
--  *
--  * Called by triggers when metadata changes to increment the version.
--  */
-- CREATE OR REPLACE FUNCTION hg_update_metadata_version()
-- RETURNS TRIGGER AS $$
-- BEGIN
--     INSERT INTO hg_metadata_version (version_id, last_modified, change_type, change_count)
--     VALUES (
--         (SELECT COALESCE(MAX(version_id), 0) + 1 FROM hg_metadata_version),
--         CURRENT_TIMESTAMP,
--         TG_ARGV[0],  -- Change type passed as trigger argument
--         1
--     );
--     RETURN NULL;
-- END;
-- $$ LANGUAGE plpgsql;

-- /*
--  * Triggers for Metadata Version Updates
--  *
--  * These triggers automatically update the version table when metadata changes.
--  */

-- -- KEK change trigger
-- CREATE TRIGGER hg_kek_version_trigger
-- AFTER INSERT OR UPDATE OR DELETE ON hg_kek
-- FOR EACH STATEMENT EXECUTE FUNCTION hg_update_metadata_version('KEK');

-- -- DEK change trigger
-- CREATE TRIGGER hg_dek_version_trigger
-- AFTER INSERT OR UPDATE OR DELETE ON hg_dek
-- FOR EACH STATEMENT EXECUTE FUNCTION hg_update_metadata_version('DEK');

-- -- Encrypted columns change trigger
-- CREATE TRIGGER hg_encrypted_columns_version_trigger
-- AFTER INSERT OR UPDATE OR DELETE ON hg_encrypted_columns
-- FOR EACH STATEMENT EXECUTE FUNCTION hg_update_metadata_version('COLUMN');

-- /*
--  * Get All Metadata Function
--  *
--  * Batch query function that returns all metadata in a single result set.
--  * This is optimized for client-side cache initialization.
--  */
-- CREATE OR REPLACE FUNCTION hg_get_all_metadata()
-- RETURNS TABLE(
--     metadata_type TEXT,
--     kek_oid OID,
--     dek_oid OID,
--     column_oid OID,
--     key_name NAME,
--     key_namespace OID,
--     key_owner OID,
--     key_acl ACLITEM[],
--     column_key_distributed_id OID,
--     kek_id OID,
--     table_oid OID,
--     column_attnum INT2,
--     column_name NAME,
--     data_type_original_oid OID,
--     data_type_original_mod INT4,
--     column_key_id OID,
--     encryption_type INT4,
--     collation_oid OID,
--     function_name NAME,
--     param_key NAME,
--     param_value BYTEA,
--     create_date TIMESTAMPTZ
-- ) AS $$
-- BEGIN
--     -- Return KEK information
--     FOR kek_oid, key_name, key_namespace, key_owner, key_acl, create_date,
--         function_name, param_key, param_value IN
--         SELECT k.oid, k.key_name, k.key_namespace, k.key_owner, k.key_acl,
--                k.create_date, ka.function_name, ka.key, ka.value
--         FROM hg_kek k
--         LEFT JOIN hg_kek_args ka ON k.oid = ka.kek_id
--     LOOP
--         RETURN NEXT ROW('KEK', kek_oid, NULL, NULL, key_name, key_namespace,
--                        key_owner, key_acl, NULL, NULL, NULL, NULL, NULL,
--                        NULL, NULL, NULL, NULL, NULL, function_name, param_key,
--                        param_value, create_date);
--     END LOOP;

--     -- Return DEK information
--     FOR dek_oid, key_name, column_key_distributed_id, kek_id, key_namespace,
--         key_owner, key_acl, create_date, function_name, param_key, param_value IN
--         SELECT d.oid, d.key_name, d.column_key_distributed_id, d.kek_id,
--                d.key_namespace, d.key_owner, d.key_acl, d.create_date,
--                da.function_name, da.key, da.value
--         FROM hg_dek d
--         LEFT JOIN hg_dek_args da ON d.oid = da.dek_id
--     LOOP
--         RETURN NEXT ROW('DEK', NULL, dek_oid, NULL, key_name, key_namespace,
--                        key_owner, key_acl, column_key_distributed_id, kek_id,
--                        NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
--                        function_name, param_key, param_value, create_date);
--     END LOOP;

--     -- Return encrypted column information
--     FOR column_oid, table_oid, column_attnum, column_name,
--         data_type_original_oid, data_type_original_mod, column_key_id,
--         encryption_type, collation_oid, create_date IN
--         SELECT ec.oid, ec.table_oid, ec.column_attnum, ec.column_name,
--                ec.data_type_original_oid, ec.data_type_original_mod,
--                ec.column_key_id, ec.encryption_type, ec.collation_oid, ec.create_date
--         FROM hg_encrypted_columns ec
--         WHERE EXISTS (
--             SELECT 1 FROM hg_dek d
--             WHERE d.oid = ec.column_key_id
--         )
--     LOOP
--         RETURN NEXT ROW('COLUMN', NULL, NULL, column_oid, NULL, NULL,
--                        NULL, NULL, NULL, NULL, table_oid, column_attnum,
--                        column_name, data_type_original_oid, data_type_original_mod,
--                        column_key_id, encryption_type, collation_oid, NULL, NULL, NULL,
--                        create_date);
--     END LOOP;
-- END;
-- $$ LANGUAGE plpgsql SECURITY DEFINER;

-- /*
--  * Get Metadata Changes Since Version Function
--  *
--  * Returns metadata changes since a specified version.
--  * This supports incremental cache updates on the client side.
--  */
-- CREATE OR REPLACE FUNCTION hg_get_metadata_changes(since_version BIGINT)
-- RETURNS TABLE(
--     metadata_type TEXT,
--     change_action TEXT,  -- 'INSERT', 'UPDATE', 'DELETE'
--     kek_oid OID,
--     dek_oid OID,
--     column_oid OID,
--     key_name NAME,
--     key_namespace OID,
--     key_owner OID,
--     key_acl ACLITEM[],
--     column_key_distributed_id OID,
--     kek_id OID,
--     table_oid OID,
--     column_attnum INT2,
--     column_name NAME,
--     data_type_original_oid OID,
--     data_type_original_mod INT4,
--     column_key_id OID,
--     encryption_type INT4,
--     collation_oid OID,
--     function_name NAME,
--     param_key NAME,
--     param_value BYTEA,
--     create_date TIMESTAMPTZ
-- ) AS $$
-- BEGIN
--     -- Note: This is a simplified implementation. In a production system,
--     -- this would query a detailed change log table rather than the current state.
--     -- For now, if the version has changed, return all metadata.
--     IF hg_get_metadata_version() > since_version THEN
--         RETURN QUERY SELECT 'KEK'::TEXT, 'REFRESH'::TEXT,
--             kek_oid, dek_oid, column_oid, key_name, key_namespace, key_owner, key_acl,
--             column_key_distributed_id, kek_id, table_oid, column_attnum, column_name,
--             data_type_original_oid, data_type_original_mod, column_key_id, encryption_type,
--             collation_oid, function_name, param_key, param_value, create_date
--         FROM hg_get_all_metadata();
--     END IF;
--     -- If version hasn't changed, return empty result set
--     RETURN;
-- END;
-- $$ LANGUAGE plpgsql;

-- /*
--  * Notification Support
--  *
--  * Listen/Notify channel for real-time metadata change notifications.
--  * Clients can LISTEN to 'hg_metadata_changes' channel to receive notifications.
--  */

-- -- Function to send notifications on metadata changes
-- CREATE OR REPLACE FUNCTION hg_notify_metadata_change()
-- RETURNS TRIGGER AS $$
-- BEGIN
--     PERFORM pg_notify('hg_metadata_changes',
--         json_build_object(
--             'version', hg_get_metadata_version(),
--             'change_type', TG_ARGV[0],
--             'change_time', CURRENT_TIMESTAMP
--         )::text
--     );
--     RETURN NULL;
-- END;
-- $$ LANGUAGE plpgsql;

-- -- Triggers for metadata change notifications
-- CREATE TRIGGER hg_kek_notify_trigger
-- AFTER INSERT OR UPDATE OR DELETE ON hg_kek
-- FOR EACH STATEMENT EXECUTE FUNCTION hg_notify_metadata_change('KEK');

-- CREATE TRIGGER hg_dek_notify_trigger
-- AFTER INSERT OR UPDATE OR DELETE ON hg_dek
-- FOR EACH STATEMENT EXECUTE FUNCTION hg_notify_metadata_change('DEK');

-- CREATE TRIGGER hg_encrypted_columns_notify_trigger
-- AFTER INSERT OR UPDATE OR DELETE ON hg_encrypted_columns
-- FOR EACH STATEMENT EXECUTE FUNCTION hg_notify_metadata_change('COLUMN');