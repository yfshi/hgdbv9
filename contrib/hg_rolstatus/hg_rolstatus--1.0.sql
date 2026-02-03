/* contrib/hg_rolstatus/hg_rolstatus--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION hg_rolstatus" to load this file. \quit

/* create table: flag role rolstatus  */
DO $$ 
BEGIN
	IF NOT EXISTS (
		SELECT 1 FROM pg_class WHERE relname = 'roles_status'
	) THEN
		-- If the rolstatus rel does not exist, create table 
		EXECUTE '
			CREATE TABLE hg_security.roles_status(
			roleoid   Oid PRIMARY KEY,
			rolstatus bool
			);
			grant select on hg_security.roles_status to public;
		';
	ELSE
		RAISE EXCEPTION ' hg_security.roles_status already exists ';
	END IF;
END $$;

/* create function: set role status  */
CREATE OR REPLACE FUNCTION hg_security.set_role_status(role_name text, status bool)
RETURNS BOOLEAN AS $$
DECLARE
	rolstatus_table_exists BOOLEAN;
	role_oid oid;
	isuser bool;
BEGIN
	SELECT oid, rolcanlogin INTO role_oid,isuser FROM pg_roles WHERE rolname = role_name;

	-- check : not set user
	IF isuser THEN
		RAISE WARNING 'Only roles can be set, users cannot!';
		RETURN FALSE;
	END IF;
	--check set role
	IF (hg_security.is_syssso(role_oid)  OR
	    hg_security.is_syssao(role_oid)  OR
	    hg_security.is_sysdba(role_oid)
	    )
	THEN
		RAISE WARNING 'Can not set syssso syssao sysdba user and superuser!';
		RETURN FALSE;
	END IF;

	--check user permissions
	IF NOT hg_security.is_syssso(get_current_userid()) THEN
		RAISE WARNING 'The user[%] has not set permissions, need syssso user or superuser!', current_user;
		RETURN FALSE;
	END IF;

	SELECT EXISTS (
		SELECT 1 FROM pg_class WHERE relname = 'roles_status'
	) INTO rolstatus_table_exists;

	IF NOT rolstatus_table_exists THEN
		RAISE WARNING ' roles_status relation does not exists ';
		RETURN FALSE;
	END IF;

	INSERT INTO hg_security.roles_status(roleoid, rolstatus) VALUES(role_oid, status)
	ON CONFLICT(roleoid)
	DO UPDATE SET rolstatus = EXCLUDED.rolstatus;

	RETURN TRUE;
END;
$$ LANGUAGE plpgsql;

ALTER FUNCTION hg_security.set_role_status(role_name text, status bool) owner to syssso_role;

/* create function: get role status  */
CREATE OR REPLACE FUNCTION hg_security.get_role_status(role_name text)
RETURNS BOOLEAN AS $$
DECLARE
	rolstatus_table_exists BOOLEAN;
	role_oid oid;
	rol_status bool;
	isuser bool;
BEGIN
	SELECT oid, rolcanlogin INTO role_oid,isuser FROM pg_roles WHERE rolname = role_name;

	-- check : not set user
	IF isuser THEN
		RAISE EXCEPTION 'Only roles can be get, users cannot!';
	END IF;

	--check user permissions
	IF NOT hg_security.is_syssso(get_current_userid()) THEN
		RAISE EXCEPTION 'The user[%] has not set permissions, need syssso user or superuser!', current_user;
	END IF;

	SELECT EXISTS (
		SELECT 1 FROM pg_class WHERE relname = 'roles_status'
	) INTO rolstatus_table_exists;

	IF NOT rolstatus_table_exists THEN
		RAISE EXCEPTION ' roles_status relation does not exists ';
	END IF;

	SELECT rolstatus INTO rol_status FROM hg_security.roles_status WHERE roleoid = role_oid;

	IF NOT FOUND THEN 
		RETURN TRUE;
	ELSE
		RETURN rol_status;
	END IF;
END;
$$ LANGUAGE plpgsql;

ALTER FUNCTION hg_security.get_role_status(role_name text) owner to syssso_role;
