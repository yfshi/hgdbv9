/* contrib/hg_rolstatus/hg_rolstatus--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION hg_rolstatus" to load this file. \quit

/* create function: set role status  */
CREATE OR REPLACE FUNCTION hg_security.set_role_status(role_name text, status bool)
RETURNS BOOLEAN
AS 'MODULE_PATHNAME', 'set_role_status'
LANGUAGE C;

ALTER FUNCTION hg_security.set_role_status(role_name text, status bool) owner to syssso_role;

/* create function: get role status  */
CREATE OR REPLACE FUNCTION hg_security.get_role_status(role_name text)
RETURNS BOOLEAN
AS 'MODULE_PATHNAME', 'get_role_status'
LANGUAGE C;

ALTER FUNCTION hg_security.get_role_status(role_name text) owner to syssso_role;

-- create view
CREATE VIEW hg_security.role_status AS
SELECT
	a.oid as roleid,
	a.rolname as rolname,
	COALESCE(r.rolstatus,true) as rolstatus
	from pg_authid a
	left join
	pg_role_status r on a.oid = r.roleid
	where a.rolcanlogin = false;