/* contrib/diskquota/diskquota--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION diskquota" to load this file. \quit

/*CREATE SCHEMA diskquota;*/

-- Configuration table
create table hg_security.quota_config (
	targetOid oid,
	quotatype int,
	quotalimitMB int8 default 0,
	minlimitMB int8 default 0,
	PRIMARY KEY(targetOid, quotatype)
);

SELECT pg_catalog.pg_extension_config_dump('hg_security.quota_config', '');

CREATE FUNCTION hg_security.set_schema_quota(nspname text, size text)
RETURNS void STRICT
AS 'MODULE_PATHNAME', 'set_schema_quota'
LANGUAGE C;

CREATE FUNCTION hg_security.set_schema_quota(nspname text, size text, min_quota bool)
RETURNS void STRICT
AS 'MODULE_PATHNAME', 'set_schema_quota'
LANGUAGE C;

CREATE FUNCTION hg_security.set_role_quota(username text, size text)
RETURNS void STRICT
AS 'MODULE_PATHNAME', 'set_role_quota'
LANGUAGE C;

CREATE FUNCTION hg_security.set_role_quota(username text, size text, min_quota bool)
RETURNS void STRICT
AS 'MODULE_PATHNAME', 'set_role_quota'
LANGUAGE C;

-- Begin Bug#z8704
CREATE FUNCTION hg_security.delete_role_quota(username text)
RETURNS void STRICT
AS $$
DECLARE
	deleted_count integer;
BEGIN
	DELETE FROM hg_security.quota_config
	WHERE targetoid = (
		SELECT oid FROM pg_roles WHERE rolname = username
	);

	GET DIAGNOSTICS deleted_count = ROW_COUNT;

	IF deleted_count > 0 THEN
		PERFORM hg_security.set_role_quota(username, '0', true);
	END IF;
END;
$$ LANGUAGE plpgsql;

CREATE FUNCTION hg_security.delete_schema_quota(schemaname text)
RETURNS void STRICT
AS $$
DECLARE
	deleted_count integer;
BEGIN
	DELETE FROM hg_security.quota_config
	WHERE targetoid = (
		SELECT oid FROM pg_namespace WHERE nspname = schemaname
	);

	GET DIAGNOSTICS deleted_count = ROW_COUNT;

	IF deleted_count > 0 THEN
		PERFORM hg_security.set_schema_quota(schemaname, '0', true);
	END IF;
END;
$$ LANGUAGE plpgsql;
-- End Bug#z8704

CREATE FUNCTION hg_security.diskquota_start_worker()
RETURNS void STRICT
AS 'MODULE_PATHNAME'
LANGUAGE C;

CREATE VIEW hg_security.show_schema_quota_view AS
	SELECT pg_namespace.nspname as schema_name, pg_class.relnamespace as schema_oid,
		quota.quotalimitMB as quota_in_mb, quota.minlimitMB as min_quota_in_mb,
		sum(pg_total_relation_size(pg_class.oid)) as nspsize_in_bytes
	FROM pg_namespace, pg_class, hg_security.quota_config as quota
	WHERE pg_class.relnamespace = quota.targetoid and
		pg_class.relnamespace = pg_namespace.oid and
		quota.quotatype=0 and pg_class.relkind='r'
	GROUP BY pg_class.relnamespace, pg_namespace.nspname, quota.quotalimitMB, quota.minlimitMB;

CREATE VIEW hg_security.show_role_quota_view AS
	SELECT pg_roles.rolname as role_name, pg_class.relowner as role_oid,
		quota.quotalimitMB as quota_in_mb, quota.minlimitMB as min_quota_in_mb,
		sum(pg_total_relation_size(pg_class.oid)) as rolsize_in_bytes
	FROM pg_roles, pg_class, hg_security.quota_config as quota
	WHERE pg_class.relowner = quota.targetoid and
		pg_class.relowner = pg_roles.oid and
		quota.quotatype=1 and pg_class.relkind='r'
	GROUP BY pg_class.relowner, pg_roles.rolname, quota.quotalimitMB, quota.minlimitMB;

/* show quota_config info */
CREATE VIEW hg_security.diskquota_config_info AS
SELECT
	case 
		WHEN dq.quotatype = 1 THEN 'role'
		WHEN dq.quotatype = 0 THEN 'schema'
		ELSE 'unknown'
	END AS type,
	case 
		WHEN dq.quotatype = 1 THEN r.oid
		WHEN dq.quotatype = 0 THEN n.oid
		ELSE 0
	END AS oid,
	case 
		WHEN dq.quotatype = 1 THEN r.rolname
		WHEN dq.quotatype = 0 THEN n.nspname
		ELSE 'unknown'
	END AS name,
	dq.quotalimitmb AS quotalimitmb,
	dq.minlimitmb AS minlimitmb
FROM
	hg_security.quota_config dq
	LEFT JOIN pg_namespace n ON dq.quotatype = 0 AND dq.targetoid = n.oid
	LEFT JOIN pg_roles r ON dq.quotatype = 1 AND dq.targetoid = r.oid;

/* show all show_schema_quota_view and show_role_quota_view */
CREATE VIEW hg_security.pg_regroup_diskquota_part_info AS
SELECT 
	'role' AS type,
	role_name AS name,
	role_oid AS oid,
	quota_in_mb,
	min_quota_in_mb,
	rolsize_in_bytes AS use_size
from hg_security.show_role_quota_view
	LEFT JOIN hg_security.quota_config dq ON dq.quotatype = 1 AND dq.targetoid = role_oid
UNION ALL
SELECT
 	'schema' AS type,
	schema_name AS name,
	schema_oid AS oid,
	quota_in_mb,
	min_quota_in_mb,
	nspsize_in_bytes AS use_size
FROM hg_security.show_schema_quota_view
	LEFT JOIN hg_security.quota_config dq ON dq.quotatype = 0 AND dq.targetoid = schema_oid;

/* show disk use info */
CREATE VIEW hg_security.pg_regroup_diskquota AS
SELECT
	di.type as type,
	di.oid as oid,
	di.name as name,
	di.quotalimitmb as quotalimitmb,
	di.minlimitmb AS minlimitmb,
	CASE 
		WHEN rg.use_size IS NOT NULL THEN rg.use_size
		ELSE 0
	END AS use_size
	from hg_security.diskquota_config_info  di 
	LEFT JOIN hg_security.pg_regroup_diskquota_part_info rg ON rg.oid = di.oid;

SELECT hg_security.diskquota_start_worker();
DROP FUNCTION hg_security.diskquota_start_worker();

alter view  hg_security.pg_regroup_diskquota owner to sysdba_role;
alter view  hg_security.pg_regroup_diskquota_part_info owner to sysdba_role;

alter view hg_security.diskquota_config_info owner to sysdba_role;

alter view  hg_security.show_role_quota_view owner to sysdba_role;
alter view hg_security.show_schema_quota_view owner to sysdba_role;

GRANT ALL ON FUNCTION hg_security.delete_schema_quota TO sysdba_role;
GRANT ALL ON FUNCTION hg_security.delete_role_quota TO sysdba_role;
GRANT ALL ON FUNCTION hg_security.set_role_quota(text, text) TO sysdba_role;
GRANT ALL ON FUNCTION hg_security.set_role_quota(text, text, bool) TO sysdba_role;
GRANT ALL ON FUNCTION hg_security.set_schema_quota(text, text) TO sysdba_role;
GRANT ALL ON FUNCTION hg_security.set_schema_quota(text, text, bool) TO sysdba_role;

GRANT ALL ON TABLE hg_security.quota_config TO sysdba_role;

