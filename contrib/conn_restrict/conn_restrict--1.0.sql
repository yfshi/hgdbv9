/* contrib/conn_restrict/conn_restrict--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION conn_restrict" to load this file. \quit


CREATE TABLE hg_security.conn_restrict_rules(id Oid primary key, filter_type int not null, rule_user text not null, rule_mac text, outrule_mac text, allow_time text, outallow_time text);

GRANT SELECT ON hg_security.conn_restrict_rules TO public;

CREATE or replace FUNCTION hg_security.add_rules(filter_type int, rule_user text, rule_mac text, outerule_mac text, allow_time text, outallow_time text)
RETURNS void
AS 'MODULE_PATHNAME'
LANGUAGE C VOLATILE PARALLEL SAFE;

alter FUNCTION hg_security.add_rules(filter_type int, rule_user text, rule_mac text, outerule_mac text, allow_time text, outallow_time text)
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
		case b.rule_mac when '*' then NULL else b.rule_mac end as rule_mac,
		case b.outrule_mac when '*' then NULL else b.outrule_mac end as outrule_mac,
		case b.allow_time when '*' then NULL else b.allow_time end as allow_time,
		case b.outallow_time when '*' then NULL else b.outallow_time end as outallow_time
	from pg_catalog.pg_authid a , hg_security.conn_restrict_rules b
	where a.oid = b.id;
GRANT SELECT ON hg_security.show_rules TO public;
