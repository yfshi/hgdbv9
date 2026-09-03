/* contrib/sql_firewall/sql_firewall--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION sql_firewall" to load this file. \quit

CREATE TABLE hg_security.sql_firewall_rec
(
	userid oid NOT NULL,
	queryid bigint NOT NULL,
	query text NOT NULL,
	original_query text NOT NULL,
	calls bigint NOT NULL DEFAULT 0,
	encoding text NOT NULL,
	statement_type text NOT NULL,
	warning_count bigint NOT NULL DEFAULT 0,
	error_count bigint NOT NULL DEFAULT 0
);

CREATE UNIQUE INDEX sql_firewall_rec_key_idx
ON hg_security.sql_firewall_rec(userid, queryid, statement_type);

GRANT SELECT ON hg_security.sql_firewall_rec TO PUBLIC;

CREATE TABLE hg_security.sql_firewall_stat_rec
(
	sql_warning bigint NOT NULL DEFAULT 0,
	sql_error bigint NOT NULL DEFAULT 0
);

INSERT INTO hg_security.sql_firewall_stat_rec VALUES (0, 0);

GRANT SELECT ON hg_security.sql_firewall_stat_rec TO PUBLIC;

CREATE TABLE hg_security.sql_high_risk_rec
(
	statement text NOT NULL,
	description text NOT NULL,
	warning_count bigint NOT NULL DEFAULT 0,
	error_count bigint NOT NULL DEFAULT 0
);

CREATE UNIQUE INDEX sql_high_risk_rec_statement_idx
ON hg_security.sql_high_risk_rec(statement);

GRANT SELECT ON hg_security.sql_high_risk_rec TO PUBLIC;

-- Register functions.
CREATE FUNCTION hg_security.sql_firewall_reset()
RETURNS void
AS 'MODULE_PATHNAME'
LANGUAGE C;

CREATE FUNCTION hg_security.sql_firewall_statements(IN showtext boolean,
    OUT userid oid,
    OUT queryid bigint,
    OUT query text,
    OUT statement_type text,
    OUT calls int8,
    OUT warning_count int8,
    OUT error_count int8
)
RETURNS SETOF record
AS 'MODULE_PATHNAME', 'sql_firewall_statements'
LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION hg_security.sql_firewall_white_list(IN showtext boolean,
    OUT userid oid,
    OUT queryid bigint,
    OUT query text,
    OUT statement_type text,
    OUT calls int8
)
RETURNS SETOF record
AS 'MODULE_PATHNAME', 'sql_firewall_white_list'
LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION hg_security.sql_firewall_black_list(IN showtext boolean,
    OUT userid oid,
    OUT queryid bigint,
    OUT query text,
    OUT statement_type text,
    OUT calls int8,
    OUT warning_count int8,
    OUT error_count int8
)
RETURNS SETOF record
AS 'MODULE_PATHNAME', 'sql_firewall_black_list'
LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION hg_security.sql_firewall_stat_warning_count()
RETURNS int8
AS 'MODULE_PATHNAME'
LANGUAGE C;

CREATE FUNCTION hg_security.sql_firewall_stat_error_count()
RETURNS int8
AS 'MODULE_PATHNAME'
LANGUAGE C;

CREATE FUNCTION hg_security.sql_firewall_stat_reset()
RETURNS void
AS 'MODULE_PATHNAME'
LANGUAGE C;

-- Register a view on the function for ease of use.
CREATE VIEW hg_security.sql_firewall_statements AS
  SELECT * FROM hg_security.sql_firewall_statements(true);

GRANT SELECT ON hg_security.sql_firewall_statements TO PUBLIC;

CREATE VIEW hg_security.sql_firewall_white_list AS
  SELECT * FROM hg_security.sql_firewall_white_list(true);

GRANT SELECT ON hg_security.sql_firewall_white_list TO PUBLIC;

CREATE VIEW hg_security.sql_firewall_black_list AS
  SELECT * FROM hg_security.sql_firewall_black_list(true);

GRANT SELECT ON hg_security.sql_firewall_black_list TO PUBLIC;

CREATE VIEW hg_security.sql_firewall_stat AS
  SELECT hg_security.sql_firewall_stat_warning_count() AS sql_warning,
         hg_security.sql_firewall_stat_error_count() AS sql_error;

GRANT SELECT ON hg_security.sql_firewall_stat TO PUBLIC;

-- Export/import firewall rules to/from the file.
CREATE FUNCTION hg_security.sql_firewall_export_rule(text)
RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C;

CREATE FUNCTION hg_security.sql_firewall_import_rule(text)
RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C;

CREATE FUNCTION hg_security.import_white_list(text)
RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C;

CREATE FUNCTION hg_security.import_white_list_for_user(name, text)
RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C;

CREATE FUNCTION hg_security.import_white_list_batch(text)
RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C;

CREATE FUNCTION hg_security.import_white_list_batch_for_user(name, text)
RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C;

CREATE FUNCTION hg_security.export_white_list(text)
RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C;

CREATE FUNCTION hg_security.export_white_list_for_user(name, text)
RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C;

CREATE FUNCTION hg_security.export_white_list_sql(text)
RETURNS boolean
AS 'MODULE_PATHNAME', 'export_white_list_sql'
LANGUAGE C;

CREATE FUNCTION hg_security.export_white_list_sql_for_user(name, text)
RETURNS boolean
AS 'MODULE_PATHNAME', 'export_white_list_sql_for_user'
LANGUAGE C;

CREATE FUNCTION hg_security.delete_all_white_list()
RETURNS boolean
AS 'MODULE_PATHNAME', 'delete_all_white_list'
LANGUAGE C;

CREATE FUNCTION hg_security.delete_all_white_list_for_user(name)
RETURNS boolean
AS 'MODULE_PATHNAME', 'delete_all_white_list_for_user'
LANGUAGE C;

CREATE FUNCTION hg_security.delete_white_list(text)
RETURNS boolean
AS 'MODULE_PATHNAME', 'delete_white_list'
LANGUAGE C;

CREATE FUNCTION hg_security.delete_white_list_for_user(name, text)
RETURNS boolean
AS 'MODULE_PATHNAME', 'delete_white_list_for_user'
LANGUAGE C;

CREATE FUNCTION hg_security.delete_white_list_batch(text)
RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C;

CREATE FUNCTION hg_security.delete_white_list_batch_for_user(name, text)
RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C;

CREATE FUNCTION hg_security.import_black_list(text)
RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C;

CREATE FUNCTION hg_security.import_black_list_for_user(name, text)
RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C;

CREATE FUNCTION hg_security.import_black_list_batch(text)
RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C;

CREATE FUNCTION hg_security.import_black_list_batch_for_user(name, text)
RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C;

CREATE FUNCTION hg_security.export_black_list(text)
RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C;

CREATE FUNCTION hg_security.export_black_list_for_user(name, text)
RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C;

CREATE FUNCTION hg_security.export_black_list_sql(text)
RETURNS boolean
AS 'MODULE_PATHNAME', 'export_black_list_sql'
LANGUAGE C;

CREATE FUNCTION hg_security.export_black_list_sql_for_user(name, text)
RETURNS boolean
AS 'MODULE_PATHNAME', 'export_black_list_sql_for_user'
LANGUAGE C;

CREATE FUNCTION hg_security.export_all_sql(text)
RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C;

CREATE FUNCTION hg_security.dispaly_single_sql(bigint)
RETURNS SETOF text
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT STABLE;

CREATE FUNCTION hg_security.import_high_risk_statement(text)
RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT VOLATILE;

CREATE FUNCTION hg_security.remove_high_risk_statement(text)
RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT VOLATILE;

SELECT hg_security.import_high_risk_statement('unsafeupdate');
SELECT hg_security.import_high_risk_statement('unsafedelete');

CREATE FUNCTION hg_security.delete_all_black_list()
RETURNS boolean
AS 'MODULE_PATHNAME', 'delete_all_black_list'
LANGUAGE C;

CREATE FUNCTION hg_security.delete_all_black_list_for_user(name)
RETURNS boolean
AS 'MODULE_PATHNAME', 'delete_all_black_list_for_user'
LANGUAGE C;

CREATE FUNCTION hg_security.delete_black_list(text)
RETURNS boolean
AS 'MODULE_PATHNAME', 'delete_black_list'
LANGUAGE C;

CREATE FUNCTION hg_security.delete_black_list_for_user(name, text)
RETURNS boolean
AS 'MODULE_PATHNAME', 'delete_black_list_for_user'
LANGUAGE C;

CREATE FUNCTION hg_security.delete_black_list_batch(text)
RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C;

CREATE FUNCTION hg_security.delete_black_list_batch_for_user(name, text)
RETURNS boolean
AS 'MODULE_PATHNAME'
LANGUAGE C;

-- Don't want this to be available to non-superusers.
REVOKE ALL ON FUNCTION hg_security.sql_firewall_reset() FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.sql_firewall_stat_reset() FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.sql_firewall_export_rule(text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.sql_firewall_import_rule(text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.import_white_list(text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.import_white_list_for_user(name, text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.import_white_list_batch(text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.import_white_list_batch_for_user(name, text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.export_white_list(text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.export_white_list_for_user(name, text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.export_white_list_sql(text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.export_white_list_sql_for_user(name, text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.delete_all_white_list() FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.delete_all_white_list_for_user(name) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.delete_white_list(text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.delete_white_list_for_user(name, text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.delete_white_list_batch(text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.delete_white_list_batch_for_user(name, text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.import_black_list(text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.import_black_list_for_user(name, text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.import_black_list_batch(text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.import_black_list_batch_for_user(name, text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.export_black_list(text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.export_black_list_for_user(name, text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.export_black_list_sql(text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.export_black_list_sql_for_user(name, text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.export_all_sql(text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.import_high_risk_statement(text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.remove_high_risk_statement(text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.delete_all_black_list() FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.delete_all_black_list_for_user(name) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.delete_black_list(text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.delete_black_list_for_user(name, text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.delete_black_list_batch(text) FROM PUBLIC;
REVOKE ALL ON FUNCTION hg_security.delete_black_list_batch_for_user(name, text) FROM PUBLIC;

GRANT ALL ON FUNCTION  hg_security.sql_firewall_reset() to syssso_role;
GRANT ALL ON FUNCTION  hg_security.sql_firewall_stat_reset() to syssso_role;
GRANT ALL ON FUNCTION  hg_security.sql_firewall_export_rule(text) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.sql_firewall_import_rule(text) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.import_white_list(text) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.import_white_list_for_user(name, text) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.import_white_list_batch(text) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.import_white_list_batch_for_user(name, text) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.export_white_list(text) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.export_white_list_for_user(name, text) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.export_white_list_sql(text) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.export_white_list_sql_for_user(name, text) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.delete_all_white_list() to syssso_role;
GRANT ALL ON FUNCTION  hg_security.delete_all_white_list_for_user(name) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.delete_white_list(text) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.delete_white_list_for_user(name, text) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.delete_white_list_batch(text) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.delete_white_list_batch_for_user(name, text) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.import_black_list(text) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.import_black_list_for_user(name, text) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.import_black_list_batch(text) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.import_black_list_batch_for_user(name, text) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.export_black_list(text) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.export_black_list_for_user(name, text) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.export_black_list_sql(text) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.export_black_list_sql_for_user(name, text) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.export_all_sql(text) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.import_high_risk_statement(text) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.remove_high_risk_statement(text) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.delete_all_black_list() to syssso_role;
GRANT ALL ON FUNCTION  hg_security.delete_all_black_list_for_user(name) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.delete_black_list(text) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.delete_black_list_for_user(name, text) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.delete_black_list_batch(text) to syssso_role;
GRANT ALL ON FUNCTION  hg_security.delete_black_list_batch_for_user(name, text) to syssso_role;
