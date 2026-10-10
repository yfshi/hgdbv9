\echo Use "CREATE EXTENSION hg_ddos_guard" to load this file. \quit

CREATE FUNCTION hg_security.hg_ddos_unlock_login(text, name)
RETURNS boolean AS 'MODULE_PATHNAME', 'hg_ddos_unlock_login'
LANGUAGE C STRICT PARALLEL UNSAFE;

CREATE FUNCTION hg_security.hg_ddos_unlock_login_ip(text, text)
RETURNS boolean AS 'MODULE_PATHNAME', 'hg_ddos_unlock_login_ip'
LANGUAGE C STRICT PARALLEL UNSAFE;

CREATE FUNCTION hg_security.hg_ddos_unlock_sql(text, name)
RETURNS boolean AS 'MODULE_PATHNAME', 'hg_ddos_unlock_sql'
LANGUAGE C STRICT PARALLEL UNSAFE;

CREATE FUNCTION hg_security.hg_ddos_login_status(
	OUT client_ip text, OUT username text, OUT userid oid, OUT database_oid oid,
	OUT trigger_count bigint,
	OUT locked boolean, OUT lock_start timestamptz, OUT lock_until timestamptz)
RETURNS SETOF record AS 'MODULE_PATHNAME', 'hg_ddos_login_status'
LANGUAGE C STABLE PARALLEL RESTRICTED;

CREATE FUNCTION hg_security.hg_ddos_login_ip_status(
	OUT client_ip text, OUT username text, OUT userid oid, OUT database_oid oid,
	OUT request_count bigint,
	OUT locked boolean, OUT lock_start timestamptz, OUT lock_until timestamptz)
RETURNS SETOF record AS 'MODULE_PATHNAME', 'hg_ddos_login_ip_status'
LANGUAGE C STABLE PARALLEL RESTRICTED;

CREATE FUNCTION hg_security.hg_ddos_sql_status(
	OUT client_ip text, OUT username text, OUT userid oid, OUT database_oid oid,
	OUT request_count bigint,
	OUT locked boolean, OUT lock_start timestamptz, OUT lock_until timestamptz)
RETURNS SETOF record AS 'MODULE_PATHNAME', 'hg_ddos_sql_status'
LANGUAGE C STABLE PARALLEL RESTRICTED;

CREATE VIEW hg_security.hg_ddos_login_status_view AS
SELECT client_ip,
	   username,
	   userid,
	   trigger_count,
	   locked,
	   lock_start,
	   lock_until
FROM hg_security.hg_ddos_login_status();

CREATE VIEW hg_security.hg_ddos_login_ip_status_view AS
SELECT client_ip,
	   request_count AS locked_user_count,
	   lock_start,
	   lock_until
FROM hg_security.hg_ddos_login_ip_status();

CREATE VIEW hg_security.hg_ddos_sql_status_view AS
SELECT status.client_ip,
	   status.username,
	   status.userid,
	   status.request_count,
	   status.locked,
	   status.lock_start,
	   status.lock_until
FROM hg_security.hg_ddos_sql_status() AS status
WHERE status.database_oid = (SELECT current_database.oid
							 FROM pg_catalog.pg_database AS current_database
							 WHERE current_database.datname = pg_catalog.current_database());

CREATE FUNCTION hg_security.hg_ddos_stats(
	OUT login_hash_entries bigint, OUT sql_hash_entries bigint,
	OUT login_reject_total bigint, OUT sql_reject_total bigint)
RETURNS record AS 'MODULE_PATHNAME', 'hg_ddos_stats'
LANGUAGE C STABLE PARALLEL RESTRICTED;

CREATE VIEW hg_security.hg_ddos_stats_view AS
SELECT login_hash_entries,
	   sql_hash_entries,
	   login_reject_total,
	   sql_reject_total
FROM hg_security.hg_ddos_stats();

REVOKE ALL ON FUNCTION hg_security.hg_ddos_unlock_login(text, name) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.hg_ddos_unlock_login_ip(text, text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.hg_ddos_unlock_sql(text, name) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.hg_ddos_login_ip_status()FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.hg_ddos_login_status() FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.hg_ddos_sql_status() FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.hg_ddos_stats() FROM PUBLIC;

GRANT ALL ON FUNCTION hg_security.hg_ddos_unlock_login(text, name) to syssso_role;
GRANT ALL ON FUNCTION hg_security.hg_ddos_unlock_login_ip(text, text) to syssso_role;
GRANT ALL ON FUNCTION hg_security.hg_ddos_unlock_sql(text, name) to syssso_role;
GRANT ALL ON FUNCTION hg_security.hg_ddos_login_ip_status() to syssso_role;
GRANT ALL ON FUNCTION hg_security.hg_ddos_login_status() to syssso_role;
GRANT ALL ON FUNCTION hg_security.hg_ddos_sql_status() to syssso_role;
GRANT ALL ON FUNCTION hg_security.hg_ddos_stats() to syssso_role;

GRANT select on hg_security.hg_ddos_login_ip_status_view to public;
ALTER VIEW hg_security.hg_ddos_login_ip_status_view OWNER TO syssso_role;
GRANT select on hg_security.hg_ddos_login_status_view to public;
ALTER VIEW hg_security.hg_ddos_login_status_view OWNER TO syssso_role;
GRANT select on hg_security.hg_ddos_sql_status_view to public;
ALTER VIEW hg_security.hg_ddos_sql_status_view OWNER TO syssso_role;
GRANT select on hg_security.hg_ddos_stats_view to public;
ALTER VIEW hg_security.hg_ddos_stats_view OWNER TO syssso_role;
