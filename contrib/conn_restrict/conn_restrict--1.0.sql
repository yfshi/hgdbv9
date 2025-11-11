/* contrib/conn_restrict/conn_restrict--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION conn_restrict" to load this file. \quit


CREATE TABLE hg_security.conn_restrict_rules(id Oid primary key, filter_type int not null, rule_user text not null, include_mac text, exclude_mac text, include_time text, exclude_time text);

REVOKE ALL on hg_security.conn_restrict_rules FROM PUBLIC;
GRANT INSERT, DELETE, UPDATE ON hg_security.conn_restrict_rules TO syssso;

CREATE or replace FUNCTION hg_security.add_rules(filter_type int, rule_user text, include_mac text, exclude_mac text, include_time text, exclude_time text)
RETURNS void
AS 'MODULE_PATHNAME'
LANGUAGE C VOLATILE PARALLEL SAFE;

alter FUNCTION hg_security.add_rules(filter_type int, rule_user text, include_mac text, exclude_mac text, include_time text, exclude_time text)
owner to syssso;

CREATE or replace FUNCTION hg_security.remove_rules(rule_user text)
RETURNS void
AS 'MODULE_PATHNAME'
LANGUAGE C VOLATILE PARALLEL SAFE;

alter FUNCTION hg_security.remove_rules(rule_user text)
owner to syssso;

CREATE or replace VIEW hg_security.show_rules as select
		a.oid as id,
		b.filter_type as filter_type,
		a.rolname as rule_user,
		case b.include_mac when '*' then NULL else b.include_mac end as include_mac,
		case b.exclude_mac when '*' then NULL else b.exclude_mac end as exclude_mac,
		case b.include_time when '*' then NULL else b.include_time end as include_time,
		case b.exclude_time when '*' then NULL else b.exclude_time end as exclude_time
	from pg_catalog.pg_authid a , hg_security.conn_restrict_rules b
	where a.oid = b.id;

REVOKE ALL on hg_security.show_rules FROM PUBLIC;
GRANT SELECT ON hg_security.show_rules TO syssso;
