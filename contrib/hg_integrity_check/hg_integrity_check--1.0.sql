/* contrib/hg_integrity_check/hg_integrity_check--1.0.sql */

-- complain if script is sourced IN psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION hg_integrity_check" to load this file. \quit

/* create function: check all wal file md5value, and return false/true */
CREATE OR REPLACE FUNCTION hg_security.wal_integrity_check()
RETURNS BOOLEAN AS $$
DECLARE
	check_failed BOOLEAN;
	is_null BOOLEAN;
	error_filename text;
	archive_mode_val text;
	archive_command_val text;
	wal_integrity_enable text;
	rec record;
	se_feature_mode_val text;
	is_superuser_val text;
BEGIN
	--check user permissions
	se_feature_mode_val = current_setting('se_feature.mode', true);
	is_superuser_val = current_setting('is_superuser', true);
	IF se_feature_mode_val IN('on', 'true', '1') THEN
		IF NOT hg_security.is_syssso(get_current_userid()) THEN
			RAISE EXCEPTION 'permission denied';
		END IF;
	ELSE
		IF NOT is_superuser_val IN('on', 'true', '1') THEN
			RAISE EXCEPTION 'permission denied';
		END IF;
	END IF;

	wal_integrity_enable = current_setting('hg_wal_integrity.enable', true);
	IF wal_integrity_enable IS NULL OR wal_integrity_enable NOT IN('on', 'true', '1') THEN
		RAISE EXCEPTION 'Please enable guc hg_wal_integrity.enable.';
	END IF;

	--check guc para
	archive_mode_val = current_setting('archive_mode', true);
	archive_command_val = current_setting('archive_command', true);
	IF archive_mode_val IS NULL OR archive_mode_val NOT IN('on', 'true', '1') OR
		archive_command_val IS NULL OR archive_command_val = '' or archive_command_val = '(disabled)' THEN
		RAISE warning 'Please enable guc parameters: archive_mode and archive_command. AND trigger wal archive.';
	END IF;

	/* pg_wal_md5_verify exec failed */
	BEGIN
		create temp table tmp_check on commit drop as select * from hg_security.pg_wal_md5_verify();
	EXCEPTION WHEN OTHERS THEN
		RAISE warning 'Wal check info is null, Please trigger wal archive.';
		RETURN true;
	END;

	/* pg_wal_md5_verify return null */
	select exists (select 1 from tmp_check) into is_null;
	IF NOT is_null THEN
		RAISE warning 'Wal check info is null, Please trigger wal archive.';
		return true;
	END IF;

	/* check wal file md5,if false, RAISE filename and info */
	FOR rec IN select is_match,wal_filename,info from tmp_check loop
		IF rec.is_match = false THEN
			RAISE warning 'filename [%] check failed :[%].', rec.wal_filename,rec.info;
			check_failed = true;
		END IF;
	END loop;

	IF check_failed THEN
		return false;
	ELSE
		return true;
	END IF;
END;
$$ LANGUAGE plpgsql;

ALTER FUNCTION hg_security.wal_integrity_check() owner to syssso_role;

/* create function: check every wal file md5value */
CREATE OR REPLACE FUNCTION hg_security.pg_wal_md5_verify(record_file TEXT DEFAULT NULL)
RETURNS TABLE (
	wal_filename TEXT,
	expected_md5 TEXT,
	actual_md5 TEXT,
	is_match BOOLEAN,
	info TEXT
)
AS 'MODULE_PATHNAME', 'pg_wal_md5_verify'
LANGUAGE C;
ALTER FUNCTION hg_security.pg_wal_md5_verify(record_file TEXT) owner to syssso_role;

CREATE OR REPLACE FUNCTION hg_security.pg_wal_init_checklog()
RETURNS boolean
AS 'MODULE_PATHNAME', 'pg_wal_init_checklog'
LANGUAGE C;

select hg_security.pg_wal_init_checklog();
ALTER FUNCTION hg_security.pg_wal_init_checklog() owner to syssso_role;