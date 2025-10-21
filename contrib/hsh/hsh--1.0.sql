/* contrib/hsh/hsh--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION hsh" to load this file. \quit

-- Register the function.
CREATE FUNCTION hsh_ringbuf_raw(
	OUT ts timestamptz,
	OUT pid int,
	OUT client_addr text,
	OUT app_name text,
	OUT dbid oid,
	OUT db_name text,
	OUT userid oid,
	OUT user_name text,
	OUT event_type text,
	OUT event text,
	OUT query_id bigint,
	OUT cmd_type text,
	OUT phase text,
	OUT backend_type text,
	OUT state text,
	OUT is_sample bool,
	OUT activity_text text
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C VOLATILE STRICT;

-- create the history data table
CREATE TABLE hsh_history_data(
	ts timestamptz,
	pid int,
	client_addr text,
	app_name text,
	dbid oid,
	db_name text,
	userid oid,
	username text,
	event_type text,
	event text,
	query_id bigint,
	cmd_type text,
	phase text,
	backend_type text,
	state text,
	is_sample bool,
	activity_text text
);

-- create the statments related saved table
CREATE TABLE hsh_statements
(
	query_id bigint,
	query	text,
	constraint pk_hsh_statements primary key(query_id)
);


CREATE VIEW V$ACTIVE_SESSION_HISTORY AS
	SELECT
		s.ts,
		s.pid,
		s.client_addr,
		s.app_name,
		s.dbid,
		d.datname as db_name,
		s.userid,
		u.rolname AS user_name,
		s.event_type,
		s.event,
		s.query_id,
		s.cmd_type,
		s.phase,
		s.backend_type,
		s.state,
		s.is_sample,
		s.activity_text
	FROM ((hsh_ringbuf_raw() s(ts, pid, client_addr, app_name, dbid, db_name, userid, user_name, event_type, event, query_id, cmd_type, phase, backend_type, state, is_sample, activity_text)
		LEFT JOIN pg_catalog.pg_database d ON (s.dbid = d.oid))
		LEFT JOIN pg_catalog.pg_roles u ON (s.userid = u.oid));

CREATE FUNCTION hsh_report_to_file(timestamptz, timestamptz, text)
RETURNS bool
AS 'MODULE_PATHNAME', 'hsh_report_to_file'
LANGUAGE C VOLATILE STRICT;

CREATE FUNCTION hsh_report_timespec(timestamptz, timestamptz)
RETURNS text
AS 'MODULE_PATHNAME', 'hsh_report_timespec'
LANGUAGE C VOLATILE STRICT;
