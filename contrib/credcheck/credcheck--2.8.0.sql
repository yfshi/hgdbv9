-- credcheck extension for PostgreSQL
-- Copyright (c) 2021-2023 MigOps Inc
-- Copyright (c) 2023 Gilles Darold
-- Copyright (c) 2024 HexaCluster Corp

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION credcheck" to load this file. \quit

CREATE SCHEMA credcheck;

----
-- Remove all entries from password history.
-- Returns the number of entries removed.
----
CREATE FUNCTION hg_security.pg_password_history_reset( )
RETURNS integer
AS 'MODULE_PATHNAME'
LANGUAGE C VOLATILE;

----
-- Remove entries of the specified user from password history.
-- Returns the number of entries removed.
----
CREATE FUNCTION hg_security.pg_password_history_reset( IN username name )
RETURNS integer
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT VOLATILE;

----
-- Look at password history entries
----
CREATE FUNCTION hg_security.pg_password_history (
	OUT userid Oid,
	OUT rolename name,
	OUT password_date timestamp with time zone,
	OUT password_hash text
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT VOLATILE;

-- Register a view on the function for ease of use.
CREATE VIEW hg_security.pg_password_history AS
  SELECT * FROM hg_security.pg_password_history();

----
-- Change password creation timestamp for all entries of the specified
-- user in the password history. Proposed for testing purpose only.
-- Returns the number of entries changed.
----
CREATE FUNCTION hg_security.pg_password_history_timestamp( IN username name, IN new_timestamp timestamp with time zone)
RETURNS integer
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT VOLATILE;


-- Don't want this to be available to non-superusers.
---REVOKE ALL ON FUNCTION hg_security.pg_password_history_reset() FROM PUBLIC;
---REVOKE ALL ON FUNCTION hg_security.pg_password_history_reset(name) FROM PUBLIC;
---REVOKE ALL ON FUNCTION hg_security.pg_password_history_timestamp(name, timestamp with time zone) FROM PUBLIC;

----
-- Remove all entries from authent failure cache.
-- Returns the number of entries removed.
----
CREATE FUNCTION hg_security.pg_banned_role_reset( )
RETURNS integer
AS 'MODULE_PATHNAME'
LANGUAGE C VOLATILE;

----
-- Remove entries of the specified user from authent failure cache.
-- Returns the number of entries removed.
----
CREATE FUNCTION hg_security.pg_banned_role_reset( IN username name )
RETURNS integer
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT VOLATILE;

----
-- Look at authent failure cache entries
----
CREATE FUNCTION hg_security.pg_banned_role (
	OUT roleid Oid,
	OUT username Name,
	OUT failure_count real,
	OUT banned_date text
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT VOLATILE;

-- Register a view on the function for ease of use.
CREATE VIEW hg_security.pg_banned_role AS
  SELECT * FROM hg_security.pg_banned_role();


-- Don't want this to be available to non-superusers.
---REVOKE ALL ON FUNCTION hg_security.pg_banned_role_reset() FROM PUBLIC;
---REVOKE ALL ON FUNCTION hg_security.pg_banned_role_reset(name) FROM PUBLIC;

-- Use a function to display the login message.
CREATE FUNCTION hg_security.login_message_display(IN username name DEFAULT NULL)
RETURNS void
AS 'MODULE_PATHNAME', 'login_message_display_user'
LANGUAGE C;

-- User expiration functions
CREATE FUNCTION hg_security.user_expiration_info(
	IN username Name DEFAULT NULL,
	OUT userid Oid,
	OUT rolename Name,
	OUT user_validity interval
)
RETURNS SETOF record
AS 'MODULE_PATHNAME'
LANGUAGE C VOLATILE;

-- Register a view on the function for ease of use.
CREATE VIEW hg_security.user_expiration_info AS
  SELECT * FROM hg_security.user_expiration_info();

ALTER FUNCTION hg_security.pg_password_history_reset() OWNER TO syssso;
GRANT ALL ON FUNCTION hg_security.pg_password_history_reset() TO syssso_role;

ALTER FUNCTION hg_security.pg_password_history_reset(name) OWNER TO syssso;
GRANT ALL ON FUNCTION hg_security.pg_password_history_reset(name) TO syssso_role;

ALTER FUNCTION hg_security.pg_password_history OWNER TO syssso;
GRANT ALL ON FUNCTION hg_security.pg_password_history TO syssso_role;
ALTER VIEW hg_security.pg_password_history OWNER TO syssso;
GRANT ALL ON hg_security.pg_password_history TO syssso_role;

ALTER FUNCTION hg_security.pg_banned_role_reset(name) OWNER TO syssso;
GRANT ALL ON FUNCTION hg_security.pg_banned_role_reset(name) TO syssso_role;

ALTER FUNCTION hg_security.pg_banned_role_reset() OWNER TO syssso;
GRANT ALL ON FUNCTION hg_security.pg_banned_role_reset() TO syssso_role;
ALTER VIEW hg_security.pg_banned_role OWNER TO syssso;
GRANT ALL ON hg_security.pg_banned_role TO syssso_role;

ALTER FUNCTION hg_security.pg_banned_role OWNER TO syssso;
GRANT ALL ON FUNCTION hg_security.pg_banned_role TO syssso_role;
ALTER FUNCTION hg_security.login_message_display OWNER TO syssso;
GRANT ALL ON FUNCTION hg_security.login_message_display TO syssso_role;

GRANT EXECUTE ON FUNCTION hg_security.login_message_display TO PUBLIC;

ALTER FUNCTION hg_security.user_expiration_info OWNER TO syssso;
GRANT ALL ON FUNCTION hg_security.user_expiration_info TO syssso_role;
ALTER VIEW hg_security.user_expiration_info OWNER TO syssso;
GRANT ALL ON hg_security.user_expiration_info TO syssso_role;

GRANT EXECUTE ON FUNCTION hg_security.user_expiration_info TO PUBLIC;
GRANT SELECT ON hg_security.user_expiration_info TO PUBLIC;

