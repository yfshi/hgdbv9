/* contrib/sql_sentinel/sql_sentinel--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION sql_sentinel" to load this file. \quit


CREATE TABLE hg_security.sql_sentinel_sentive (
    schemaname  text NOT NULL,
    funcname  text NOT NULL,
    f_oid OID PRIMARY key,
    CONSTRAINT sentive_schemaname_funcname_key UNIQUE (schemaname, funcname)
);

CREATE FUNCTION hg_security.sql_sentinel_insert_sentive(schemaname text, funcname text)
RETURNS void
AS 'MODULE_PATHNAME', 'sql_sentinel_insert_sentive'
LANGUAGE C STRICT;

CREATE FUNCTION hg_security.sql_sentinel_delete_sentive(schemaname text, funcname text)
RETURNS void
AS 'MODULE_PATHNAME', 'sql_sentinel_delete_sentive'
LANGUAGE C STRICT;

SELECT hg_security.sql_sentinel_insert_sentive('pg_catalog', 'pg_read_file(text)');
SELECT hg_security.sql_sentinel_insert_sentive('pg_catalog', 'pg_read_file(text, bigint, bigint)');
SELECT hg_security.sql_sentinel_insert_sentive('pg_catalog', 'pg_read_file(text, bigint, bigint, boolean)');
SELECT hg_security.sql_sentinel_insert_sentive('pg_catalog', 'pg_read_binary_file(text)');
SELECT hg_security.sql_sentinel_insert_sentive('pg_catalog', 'pg_read_binary_file(text, bigint, bigint)');
SELECT hg_security.sql_sentinel_insert_sentive('pg_catalog', 'pg_read_binary_file(text, bigint, bigint, boolean)');
SELECT hg_security.sql_sentinel_insert_sentive('pg_catalog', 'pg_ls_dir(text)');
SELECT hg_security.sql_sentinel_insert_sentive('pg_catalog', 'pg_ls_dir(text, boolean, boolean)');

SELECT hg_security.sql_sentinel_insert_sentive('pg_catalog', 'lo_import(text)');
SELECT hg_security.sql_sentinel_insert_sentive('pg_catalog', 'lo_import(text, oid)');
SELECT hg_security.sql_sentinel_insert_sentive('pg_catalog', 'pg_sleep(double precision)');
SELECT hg_security.sql_sentinel_insert_sentive('pg_catalog', 'pg_sleep_for(interval)');

SELECT hg_security.sql_sentinel_insert_sentive('pg_catalog','pg_stat_file(text, boolean)');
SELECT hg_security.sql_sentinel_insert_sentive('pg_catalog','pg_stat_file(text)');


ALTER TABLE hg_security.sql_sentinel_sentive OWNER TO syssso;
GRANT SELECT ON hg_security.sqlite_audit_log to syssso_role;
