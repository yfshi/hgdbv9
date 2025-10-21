\echo Use "CREATE EXTENSION pg_audit" to load this file. \quit

--LOAD 'pg_audit';

DO $$ 
BEGIN
    IF EXISTS (
        SELECT 1 FROM pg_tablespace WHERE spcname = 'audtblspc'
    ) THEN
       -- If a tablespace exists, create a table and specify this tablespace
        EXECUTE '
            CREATE TABLE hg_secsch.pg_audit_log(
			confid oid, 
			sessionoid integer,
			useroid oid,
			username text,
			roleoid oid,
			rolename text,
			audittime timestamp(6) with time zone,
			result text,
			namespaceid oid,
			namespacename text,
			objectoid oid,
			objectname text,
			colname text,
			classid oid,
			objecttype text,
			audittype text,
			commandid integer,
			actionname text,
			clientport text,
			clientname text,
			clientip text,
			clientmac text,
			clientappname text,
			sqltext text,
			errtext text,
			authmethod text,
			privlevel text
			)  PARTITION BY RANGE(audittime) TABLESPACE audtblspc ;
        ';
    ELSE
        -- If the tablespace does not exist, do not specify the tablespace when creating the table (use the default tablespace)
        EXECUTE '
            CREATE TABLE hg_secsch.pg_audit_log(
			confid oid, 
			sessionoid integer,
			useroid oid,
			username text,
			roleoid oid,
			rolename text,
			audittime timestamp(6) with time zone,
			result text,
			namespaceid oid,
			namespacename text,
			objectoid oid,
			objectname text,
			colname text,
			classid oid,
			objecttype text,
			audittype text,
			commandid integer,
			actionname text,
			clientport text,
			clientname text,
			clientip text,
			clientmac text,
			clientappname text,
			sqltext text,
			errtext text,
			authmethod text,
			privlevel text) PARTITION BY RANGE(audittime);
        ';
    END IF;
END $$;


DO $$ 
BEGIN
    IF EXISTS (
        SELECT 1 FROM pg_tablespace WHERE spcname = 'audtblspc'
    ) THEN
       -- If a tablespace exists, create a table and specify this tablespace
        EXECUTE '
            CREATE TABLE hg_secsch.pg_secmon_history(
            		audittime timestamp with time zone,
			ruleoid oid,
			confid oid,
			actionname text,
			session integer,
			commandid integer,
			rulename text
			)TABLESPACE audtblspc;
        ';
    ELSE
        -- If the tablespace does not exist, do not specify the tablespace when creating the table (use the default tablespace)
        EXECUTE '
            CREATE TABLE hg_secsch.pg_secmon_history(
            		audittime timestamp with time zone,
			ruleoid oid,
			confid oid,
			actionname text,
			session integer,
			commandid integer,
			rulename text);
        ';
    END IF;
END $$;

CREATE INDEX pg_secmon_history_audttime_ruleoid_idx ON hg_secsch.pg_secmon_history USING btree (audittime, ruleoid);

DO $$ 
BEGIN
    IF EXISTS (
        SELECT 1 FROM pg_tablespace WHERE spcname = 'audtblspc'
    ) THEN
        -- If a tablespace exists, create a table and specify this tablespace
        EXECUTE '
            CREATE TABLE hg_secsch.pg_secmon_thrhis(
			throid oid,
			confid oid,
			session integer,
			audittime timestamp(6) with time zone,
			commandid integer,
			thrname name,
			ruleoid oid,
			interval integer,
			starttime timestamp(6) with time zone,
			level integer,
			maxtimes integer,
			ipmt_starttime timestamp(6) with time zone,
			ipmt_endtime timestamp(6) with time zone,
			nsteps integer,
			ruleseq oidvector,
			basename text,
			basevalue text,
			ipmt_cloumns int2vector
			)TABLESPACE audtblspc;
        ';
    ELSE
        -- If the tablespace does not exist, do not specify the tablespace when creating the table (use the default tablespace)
        EXECUTE '
            CREATE TABLE hg_secsch.pg_secmon_thrhis(
			throid oid,
			confid oid,
			session integer,
			audittime timestamp(6) with time zone,
			commandid integer,
			thrname name,
			ruleoid oid,
			interval integer,
			starttime timestamp(6) with time zone,
			level smallint,
			maxtimes integer,
			ipmt_starttime timestamp(6) with time zone,
			ipmt_endtime timestamp(6) with time zone,
			nsteps integer,
			ruleseq oidvector,
			basename text,
			basevalue text,
			ipmt_cloumns int2vector);
        ';
    END IF;
END $$;

ALTER TABLE hg_secsch.pg_audit_log OWNER TO syssao;
GRANT SELECT ON hg_secsch.pg_audit_log TO syssso;

ALTER TABLE hg_secsch.pg_secmon_history OWNER TO syssao;
GRANT SELECT ON hg_secsch.pg_secmon_history to syssao_role;

ALTER TABLE hg_secsch.pg_secmon_thrhis OWNER TO syssao;
GRANT SELECT ON hg_secsch.pg_secmon_thrhis to syssao_role;

CREATE FUNCTION hg_secsch.current_is_syssao()
RETURNS boolean
AS 'MODULE_PATHNAME', 'current_is_syssao'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION hg_secsch.current_is_syssso()
RETURNS boolean
AS 'MODULE_PATHNAME', 'current_is_syssso'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION hg_secsch.current_is_superuser()
RETURNS boolean
AS 'MODULE_PATHNAME', 'current_is_superuser'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;


CREATE OR REPLACE FUNCTION hg_secsch.pg_clean_auditlog()
RETURNS BOOLEAN AS $$
DECLARE
	mode BOOLEAN;
    v_sql1 TEXT;
    v_sql2 TEXT;
    v_sql3 TEXT;
    issao  BOOLEAN;
    issuper  BOOLEAN;
    v_row_count BIGINT;
BEGIN
	SELECT into mode current_setting('se_feature.mode');
    v_sql1 := format('SELECT hg_secsch.current_is_syssao()');
    v_sql2 := format('SELECT hg_secsch.current_is_superuser()');
    v_sql3 := format('WITH deleted_rows AS (DELETE FROM hg_secsch.pg_audit_log RETURNING *) SELECT count(*) FROM deleted_rows');

    IF mode = TRUE THEN
    	EXECUTE v_sql1 INTO issao;

	    IF issao = FALSE THEN
			RAISE EXCEPTION USING
			ERRCODE = 'insufficient_privilege',
			MESSAGE = 'permission denied, only role=syssao_role can run this function';
			RETURN FALSE;
		ELSE
	    	EXECUTE v_sql3 INTO v_row_count;

		    IF v_row_count > 0 THEN
		        RETURN TRUE;
		    ELSE
		        RETURN FALSE;
		    END IF;
		END IF;
	ELSIF mode = FALSE THEN
		EXECUTE v_sql2 INTO issuper;

		IF issuper = FALSE THEN
    		RAISE EXCEPTION USING
			ERRCODE = 'insufficient_privilege',
			MESSAGE = 'permission denied, only superuser can run this function';
			RETURN FALSE;
		ELSE
	    	EXECUTE v_sql3 INTO v_row_count;

		    IF v_row_count > 0 THEN
		        RETURN TRUE;
		    ELSE
		        RETURN FALSE;
		    END IF;
		END IF;
    END IF;

--EXCEPTION WHEN OTHERS THEN

   -- RETURN FALSE;
END;
$$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION hg_secsch.pg_clean_audit_config()
RETURNS BOOLEAN AS $$
DECLARE
	mode BOOLEAN;
    v_sql1 TEXT;
    v_sql2 TEXT;
    v_sql3 TEXT;
    issao  BOOLEAN;
    issuper  BOOLEAN;
    v_row_count BIGINT;
BEGIN
	SELECT into mode current_setting('se_feature.mode');
    v_sql1 := format('SELECT hg_secsch.current_is_syssao()');
    v_sql2 := format('SELECT hg_secsch.current_is_superuser()');
    v_sql3 := format('WITH deleted_rows AS (DELETE FROM pg_audit_config RETURNING *) SELECT count(*) FROM deleted_rows');

   IF mode = TRUE THEN
    	EXECUTE v_sql1 INTO issao;

	    IF issao = FALSE THEN
			RAISE EXCEPTION USING
			ERRCODE = 'insufficient_privilege',
			MESSAGE = 'permission denied, only role=syssao_role can run this function';
			RETURN FALSE;
		ELSE
	    	EXECUTE v_sql3 INTO v_row_count;

		    IF v_row_count > 0 THEN
		        RETURN TRUE;
		    ELSE
		        RETURN FALSE;
		    END IF;
		END IF;
	ELSIF mode = FALSE THEN
		EXECUTE v_sql2 INTO issuper;

		IF issuper = FALSE THEN
    		RAISE EXCEPTION USING
			ERRCODE = 'insufficient_privilege',
			MESSAGE = 'permission denied, only superuser can run this function';
			RETURN FALSE;
		ELSE
	    	EXECUTE v_sql3 INTO v_row_count;

		    IF v_row_count > 0 THEN
		        RETURN TRUE;
		    ELSE
		        RETURN FALSE;
		    END IF;
		END IF;
    END IF;
--EXCEPTION WHEN OTHERS THEN
  --  RETURN FALSE;
END;
$$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION hg_secsch.pg_delete_auditlog(timestamp_to_delete timestamp)
RETURNS BOOLEAN AS $$
DECLARE
    retentiondays integer;
    can_delete_time timestamp;
    mode BOOLEAN;
    v_sql1 TEXT;
    v_sql2 TEXT;
    v_sql3 TEXT;
    issao  BOOLEAN;
    issuper  BOOLEAN;
    v_row_count BIGINT;
BEGIN
	SELECT into mode current_setting('se_feature.mode');
	SELECT into retentiondays current_setting('audit.retentiondays');
	v_sql1 := format('SELECT hg_secsch.current_is_syssao()');
    v_sql2 := format('SELECT hg_secsch.current_is_superuser()');

	IF mode = TRUE THEN
		EXECUTE v_sql1 INTO issao;

	    IF issao = FALSE THEN
			RAISE EXCEPTION USING
			ERRCODE = 'insufficient_privilege',
			MESSAGE = 'permission denied, only role=syssao_role can run this function';
			RETURN FALSE;
		ELSE
			SELECT into can_delete_time
		    now() -  INTERVAL '1' day * retentiondays;
		 
		    IF can_delete_time >= timestamp_to_delete THEN
		        v_sql3 := format('WITH deleted_rows AS (DELETE FROM hg_secsch.pg_audit_log WHERE audittime <= %L RETURNING *) SELECT count(*) FROM deleted_rows',
		        				timestamp_to_delete);
		    ELSE
		        v_sql3 := format('WITH deleted_rows AS (DELETE FROM hg_secsch.pg_audit_log WHERE audittime <= %L RETURNING *) SELECT count(*) FROM deleted_rows', 
		        				can_delete_time);
		    END IF;
		    
		    EXECUTE v_sql3 INTO v_row_count;
		    IF v_row_count > 0 THEN
		        RETURN TRUE;
		    ELSE
		        RETURN FALSE;
		    END IF;
		END IF;
	ELSIF mode = FALSE THEN
		EXECUTE v_sql2 INTO issuper;

		IF issuper = FALSE THEN
    		RAISE EXCEPTION USING
			ERRCODE = 'insufficient_privilege',
			MESSAGE = 'permission denied, only superuser can run this function';
			RETURN FALSE;
		ELSE
		    SELECT into can_delete_time
		    now() -  INTERVAL '1' day * retentiondays;
		 
		    IF can_delete_time >= timestamp_to_delete THEN
		        v_sql3 := format('WITH deleted_rows AS (DELETE FROM hg_secsch.pg_audit_log WHERE audittime <= %L RETURNING *) SELECT count(*) FROM deleted_rows',
		        				timestamp_to_delete);
		    ELSE
		        v_sql3 := format('WITH deleted_rows AS (DELETE FROM hg_secsch.pg_audit_log WHERE audittime <= %L RETURNING *) SELECT count(*) FROM deleted_rows', 
		        				can_delete_time);
		    END IF;
		    
		    EXECUTE v_sql3 INTO v_row_count;
		    IF v_row_count > 0 THEN
		        RETURN TRUE;
		    ELSE
		        RETURN FALSE;
		    END IF;
		END IF;
	END IF;

--EXCEPTION WHEN OTHERS THEN
  --  RETURN FALSE;
END;
$$ LANGUAGE plpgsql;


CREATE OR REPLACE FUNCTION hg_secsch.pg_delete_audit_config(
    dconfid Oid
) RETURNS BOOLEAN AS $$
DECLARE
	mode BOOLEAN;
    v_sql1 TEXT;
    v_sql2 TEXT;
    v_sql3 TEXT;
    issao  BOOLEAN;
    issuper  BOOLEAN;
    v_row_count BIGINT;
BEGIN
	SELECT into mode current_setting('se_feature.mode');
    v_sql1 := format('SELECT hg_secsch.current_is_syssao()');
    v_sql2 := format('SELECT hg_secsch.current_is_superuser()');

    IF mode = TRUE THEN
		EXECUTE v_sql1 INTO issao;

	    IF issao = FALSE THEN
			RAISE EXCEPTION USING
			ERRCODE = 'insufficient_privilege',
			MESSAGE = 'permission denied, only role=syssao_role can run this function';
			RETURN FALSE;
		ELSE
		    v_sql3 := format('WITH deleted_rows AS (DELETE FROM pg_audit_config WHERE confid = %L RETURNING *) SELECT count(*) FROM deleted_rows', 
		                    dconfid);

		    EXECUTE v_sql3 INTO v_row_count;

		    IF v_row_count > 0 THEN
		        RETURN TRUE;
		    ELSE
		        RETURN FALSE;
		    END IF;
		END IF;
	ELSIF mode = FALSE THEN
		EXECUTE v_sql2 INTO issuper;

		IF issuper = FALSE THEN
    		RAISE EXCEPTION USING
			ERRCODE = 'insufficient_privilege',
			MESSAGE = 'permission denied, only superuser can run this function';
			RETURN FALSE;
		ELSE
			v_sql3 := format('WITH deleted_rows AS (DELETE FROM pg_audit_config WHERE confid = %L RETURNING *) SELECT count(*) FROM deleted_rows', 
		                    dconfid);

		    EXECUTE v_sql3 INTO v_row_count;

		    IF v_row_count > 0 THEN
		        RETURN TRUE;
		    ELSE
		        RETURN FALSE;
		    END IF;
		END IF;
	END IF;

--EXCEPTION WHEN OTHERS THEN

  --  RETURN FALSE;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION hg_secsch.pg_clean_secmon_config()
RETURNS BOOLEAN AS $$
DECLARE
	mode BOOLEAN;
    v_sql1 TEXT;
    v_sql2 TEXT;
    v_sql3 TEXT;
    issao  BOOLEAN;
    issuper  BOOLEAN;
    v_row_count BIGINT;
BEGIN
	SELECT into mode current_setting('se_feature.mode');
    v_sql1 := format('SELECT hg_secsch.current_is_syssao()');
    v_sql2 := format('SELECT hg_secsch.current_is_superuser()');
    v_sql3 := format('WITH deleted_rules AS (DELETE FROM pg_secmon_rule RETURNING 1), deleted_thresholds AS (DELETE FROM pg_secmon_threshold RETURNING 1) SELECT (SELECT COUNT(*) FROM deleted_rules) + (SELECT COUNT(*) FROM deleted_thresholds) AS deleted_rows');

   IF mode = TRUE THEN
    	EXECUTE v_sql1 INTO issao;

	    IF issao = FALSE THEN
	    RAISE EXCEPTION USING
			ERRCODE = 'insufficient_privilege',
			MESSAGE = 'permission denied, only role=syssao_role can run this function';
			RETURN FALSE;
		ELSE
	    	EXECUTE v_sql3 INTO v_row_count;

		    IF v_row_count > 0 THEN
		        RETURN TRUE;
		    ELSE
		        RETURN FALSE;
		    END IF;
		END IF;
	ELSIF mode = FALSE THEN
		EXECUTE v_sql2 INTO issuper;

		IF issuper = FALSE THEN
		RAISE EXCEPTION USING
			ERRCODE = 'insufficient_privilege',
			MESSAGE = 'permission denied, only superuser can run this function';
			RETURN FALSE;
		ELSE
	    	EXECUTE v_sql3 INTO v_row_count;

		    IF v_row_count > 0 THEN
		        RETURN TRUE;
		    ELSE
		        RETURN FALSE;
		    END IF;
		END IF;
    END IF;
--EXCEPTION WHEN OTHERS THEN
  --  RETURN FALSE;
END;
$$ LANGUAGE plpgsql;

CREATE OR REPLACE FUNCTION hg_secsch.pg_clean_secmon_history()
RETURNS BOOLEAN AS $$
DECLARE
	mode BOOLEAN;
    v_sql1 TEXT;
    v_sql2 TEXT;
    v_sql3 TEXT;
    issao  BOOLEAN;
    issuper  BOOLEAN;
    v_row_count BIGINT;
BEGIN
	SELECT into mode current_setting('se_feature.mode');
    v_sql1 := format('SELECT hg_secsch.current_is_syssao()');
    v_sql2 := format('SELECT hg_secsch.current_is_superuser()');
   	v_sql3 := format('WITH deleted_rules AS (DELETE FROM hg_secsch.pg_secmon_history RETURNING 1), deleted_thresholds AS (DELETE FROM hg_secsch.pg_secmon_thrhis RETURNING 1) SELECT (SELECT COUNT(*) FROM deleted_rules) + (SELECT COUNT(*) FROM deleted_thresholds) AS deleted_rows');

    IF mode = TRUE THEN
    	EXECUTE v_sql1 INTO issao;

	    IF issao = FALSE THEN
	    RAISE EXCEPTION USING
			ERRCODE = 'insufficient_privilege',
			MESSAGE = 'permission denied, only role=syssao_role can run this function';
			RETURN FALSE;
		ELSE
	    	EXECUTE v_sql3 INTO v_row_count;

		    IF v_row_count > 0 THEN
		        RETURN TRUE;
		    ELSE
		        RETURN FALSE;
		    END IF;
		END IF;
	ELSIF mode = FALSE THEN
		EXECUTE v_sql2 INTO issuper;

		IF issuper = FALSE THEN
		RAISE EXCEPTION USING
			ERRCODE = 'insufficient_privilege',
			MESSAGE = 'permission denied, only superuser can run this function';
			RETURN FALSE;
		ELSE
	    	EXECUTE v_sql3 INTO v_row_count;

		    IF v_row_count > 0 THEN
		        RETURN TRUE;
		    ELSE
		        RETURN FALSE;
		    END IF;
		END IF;
    END IF;

--EXCEPTION WHEN OTHERS THEN

   -- RETURN FALSE;
END;
$$ LANGUAGE plpgsql;

CREATE FUNCTION pg_catalog.pg_audit_ddl_command_end()
		RETURNS  event_trigger
		AS 'MODULE_PATHNAME', 'pg_audit_ddl_command_end'
LANGUAGE C IMMUTABLE PARALLEL SAFE STRICT;

CREATE FUNCTION pg_catalog.pg_audit_sql_drop()
		RETURNS  event_trigger
		AS 'MODULE_PATHNAME', 'pg_audit_sql_drop'
LANGUAGE C STRICT;

CREATE EVENT TRIGGER pg_audit_ddl_command_end ON ddl_command_end EXECUTE FUNCTION pg_catalog.pg_audit_ddl_command_end();
ALTER EVENT TRIGGER pg_audit_ddl_command_end ENABLE ALWAYS;

CREATE EVENT TRIGGER pg_audit_sql_drop ON sql_drop EXECUTE FUNCTION pg_catalog.pg_audit_sql_drop();
ALTER EVENT TRIGGER pg_audit_sql_drop ENABLE ALWAYS;


CREATE FUNCTION hg_secsch.pg_audit_file_archive( )
		RETURNS  bool
		AS 'MODULE_PATHNAME', 'pg_audit_file_archive'
LANGUAGE C STRICT;

CREATE OR REPLACE FUNCTION hg_secsch.copy_file_to_remot_srv(text, text)
RETURNS void
AS 'MODULE_PATHNAME','copy_file_to_remot_srv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION hg_secsch.pg_audit_privilege(statement_opts text)
		RETURNS  text
		AS 'MODULE_PATHNAME', 'pg_audit_privilege'
LANGUAGE C STRICT;

CREATE FUNCTION hg_secsch.pg_audit_privilege(text, text)
		RETURNS  text
		AS 'MODULE_PATHNAME', 'pg_audit_privilege'
LANGUAGE C STRICT;

CREATE FUNCTION hg_secsch.pg_noaudit_privilege(statement_opts text)
		RETURNS  text
		AS 'MODULE_PATHNAME', 'pg_noaudit_privilege'
LANGUAGE C STRICT;

CREATE FUNCTION hg_secsch.pg_noaudit_privilege(text, text)
		RETURNS  text
		AS 'MODULE_PATHNAME', 'pg_noaudit_privilege'
LANGUAGE C STRICT;

CREATE FUNCTION hg_secsch.pg_noaudit_privilege_byoid(confid Oid)
		RETURNS  text
		AS 'MODULE_PATHNAME', 'pg_noaudit_privilege_byoid'
LANGUAGE C STRICT;


CREATE FUNCTION hg_secsch.pg_audit_stmt(statement_opts text)
		RETURNS  text
		AS 'MODULE_PATHNAME', 'pg_audit_stmt'
LANGUAGE C STRICT;

CREATE FUNCTION hg_secsch.pg_audit_stmt(text, text)
		RETURNS  text
		AS 'MODULE_PATHNAME', 'pg_audit_stmt'
LANGUAGE C STRICT;


CREATE FUNCTION hg_secsch.pg_noaudit_stmt(statement_opts text)
		RETURNS  text
		AS 'MODULE_PATHNAME', 'pg_noaudit_stmt'
LANGUAGE C STRICT;

CREATE FUNCTION hg_secsch.pg_noaudit_stmt(text, text)
		RETURNS  text
		AS 'MODULE_PATHNAME', 'pg_noaudit_stmt'
LANGUAGE C STRICT;

CREATE FUNCTION hg_secsch.pg_noaudit_stmt_byoid(confid Oid)
		RETURNS  text
		AS 'MODULE_PATHNAME', 'pg_noaudit_stmt_byoid'
LANGUAGE C STRICT;


CREATE FUNCTION hg_secsch.pg_audit_user(statement_opts text, username text)
		RETURNS  text
		AS 'MODULE_PATHNAME', 'pg_audit_user'
LANGUAGE C STRICT;

CREATE FUNCTION hg_secsch.pg_audit_user(text, text, text)
		RETURNS  text
		AS 'MODULE_PATHNAME', 'pg_audit_user'
LANGUAGE C STRICT;

CREATE FUNCTION hg_secsch.pg_audit_all_user(statement_opts text)
		RETURNS  text
		AS 'MODULE_PATHNAME', 'pg_audit_all_user'
LANGUAGE C STRICT;

CREATE FUNCTION hg_secsch.pg_audit_all_user(text, text)
		RETURNS  text
		AS 'MODULE_PATHNAME', 'pg_audit_all_user'
LANGUAGE C STRICT;


CREATE FUNCTION hg_secsch.pg_noaudit_user(statement_opts text, username text)
		RETURNS  text
		AS 'MODULE_PATHNAME', 'pg_noaudit_user'
LANGUAGE C STRICT;

CREATE FUNCTION hg_secsch.pg_noaudit_user(text, text, text)
		RETURNS  text
		AS 'MODULE_PATHNAME', 'pg_noaudit_user'
LANGUAGE C STRICT;

CREATE FUNCTION hg_secsch.pg_noaudit_all_user(statement_opts text)
		RETURNS  text
		AS 'MODULE_PATHNAME', 'pg_noaudit_all_user'
LANGUAGE C STRICT;

CREATE FUNCTION hg_secsch.pg_noaudit_all_user(text, text)
		RETURNS  text
		AS 'MODULE_PATHNAME', 'pg_noaudit_all_user'
LANGUAGE C STRICT;


CREATE FUNCTION hg_secsch.pg_noaudit_user_byoid(confid Oid)
		RETURNS  text
		AS 'MODULE_PATHNAME', 'pg_noaudit_user_byoid'
LANGUAGE C STRICT;

CREATE FUNCTION hg_secsch.pg_audit_object(statement text, opts text, object text)
		RETURNS  text
		AS 'MODULE_PATHNAME', 'pg_audit_object'
LANGUAGE C STRICT;

CREATE FUNCTION hg_secsch.pg_audit_object(statement text, opts text, object text, schema text)
		RETURNS  text
		AS 'MODULE_PATHNAME', 'pg_audit_object'
LANGUAGE C STRICT;

CREATE FUNCTION hg_secsch.pg_audit_object(statement text, opts text, object text, schema text, whenever text)
		RETURNS  text
		AS 'MODULE_PATHNAME', 'pg_audit_object'
LANGUAGE C STRICT;


CREATE FUNCTION hg_secsch.pg_noaudit_object(statement text, opts text, object text)
		RETURNS  text
		AS 'MODULE_PATHNAME', 'pg_noaudit_object'
LANGUAGE C STRICT;

CREATE FUNCTION hg_secsch.pg_noaudit_object(statement text, opts text, object text, schema text)
		RETURNS  text
		AS 'MODULE_PATHNAME', 'pg_noaudit_object'
LANGUAGE C STRICT;

CREATE FUNCTION hg_secsch.pg_noaudit_object(statement text, opts text, object text, schema text, whenever text)
		RETURNS  text
		AS 'MODULE_PATHNAME', 'pg_noaudit_object'
LANGUAGE C STRICT;

CREATE FUNCTION hg_secsch.pg_noaudit_object_byoid(confid Oid)
		RETURNS  text
		AS 'MODULE_PATHNAME', 'pg_noaudit_object_byoid'
LANGUAGE C STRICT;

CREATE FUNCTION hg_secsch.pg_create_secmon_rule(rulename text, confid Oid, monmode text, enable BOOLEAN)
		RETURNS  Oid
		AS 'MODULE_PATHNAME', 'pg_create_secmon_rule'
LANGUAGE C STRICT;


CREATE FUNCTION hg_secsch.pg_alter_secmon_rule(ruleoid Oid, rulename text, monmode text, enable BOOLEAN)
		RETURNS  Oid
		AS 'MODULE_PATHNAME', 'pg_alter_secmon_rule'
LANGUAGE C STRICT;

CREATE FUNCTION hg_secsch.pg_alter_secmon_rule(ruleoid Oid, rulename text, monmode text)
		RETURNS  Oid
		AS 'MODULE_PATHNAME', 'pg_alter_secmon_rule'
LANGUAGE C STRICT;

CREATE FUNCTION hg_secsch.pg_alter_secmon_rule(ruleoid Oid, rulename text)
		RETURNS  Oid
		AS 'MODULE_PATHNAME', 'pg_alter_secmon_rule'
LANGUAGE C STRICT;


CREATE FUNCTION hg_secsch.pg_drop_secmon_rule(ruleoid Oid)
		RETURNS  Oid
		AS 'MODULE_PATHNAME', 'pg_drop_secmon_rule'
LANGUAGE C STRICT;

CREATE FUNCTION hg_secsch.pg_create_secmon_threshold(
thrname text, 
secmonseq oid[], 
interverl int, 
maxtimes int, 
basename text, 
basevalue text, 
starttime timestamp with time zone, 
endtime timestamp with time zone, 
columns int2vector, 
level int, 
enabled BOOLEAN)
		RETURNS  Oid
		AS 'MODULE_PATHNAME', 'pg_create_secmon_threshold'
LANGUAGE C;

CREATE FUNCTION hg_secsch.pg_alter_secmon_threshold(
throid Oid, 
thrname text, 
interverl int, 
maxtimes int, 
basename text, 
basevalue text, 
starttime timestamp with time zone, 
endtime timestamp with time zone, 
columns int2vector, 
level int, 
enabled BOOLEAN)
		RETURNS  Oid
		AS 'MODULE_PATHNAME', 'pg_alter_secmon_threshold'
LANGUAGE C;


CREATE FUNCTION hg_secsch.pg_drop_secmon_threshold(throid Oid)
		RETURNS  Oid
		AS 'MODULE_PATHNAME', 'pg_drop_secmon_threshold'
LANGUAGE C STRICT;

CREATE FUNCTION hg_secsch.audit_show_diskinfo()
	RETURNS  TABLE (
		disk_size        text,
		disk_used         text,
		"disk_used%"         text,
		space_limit text,
		table_used  text,
		avail text,
		"table_used%" text,
		alarm	text
		)
	AS 'MODULE_PATHNAME', 'audit_show_diskinfo'
LANGUAGE C STRICT;

CREATE FUNCTION hg_secsch.pg_audit_switch_primary(int)
		RETURNS  bool
		AS 'MODULE_PATHNAME', 'pg_audit_switch_primary'
LANGUAGE C STRICT;

create view hg_secsch.show_audit_diskinfo as select * from hg_secsch.audit_show_diskinfo();
alter view  hg_secsch.show_audit_diskinfo owner to  syssao; --BUG Z4802
GRANT SELECT ON hg_secsch.show_audit_diskinfo to syssao_role;

GRANT ALL ON TABLE hg_secsch.pg_track_settings_list TO PUBLIC;
GRANT ALL ON TABLE hg_secsch.pg_track_settings_history TO PUBLIC;
GRANT ALL ON TABLE hg_secsch.pg_track_db_role_settings_list TO PUBLIC;
GRANT ALL ON TABLE hg_secsch.pg_track_db_role_settings_history TO PUBLIC;
GRANT ALL ON TABLE hg_secsch.pg_reboot TO PUBLIC;

GRANT SELECT ON hg_secsch.pg_audit_log to syssao_role;
GRANT SELECT ON hg_secsch.pg_audit_log to syssso_role; -- /* Bug#z4284 */

-- /* Begin - BUG#00004708 */
GRANT ALL ON SCHEMA pgagent to syssao;
GRANT ALL ON TABLE pgagent.pga_jobagent to syssao;
GRANT ALL ON TABLE pgagent.pga_job to syssao;
GRANT ALL ON TABLE pgagent.pga_joblog to syssao;
GRANT ALL ON TABLE pgagent.pga_jobstep to syssao;
GRANT ALL ON TABLE pgagent.pga_schedule to syssao;
GRANT ALL ON TABLE pgagent.pga_jobclass to syssao;
GRANT ALL ON TABLE pgagent.pga_jobsteplog to syssao;
GRANT ALL ON SEQUENCE pgagent.pga_joblog_jlgid_seq TO syssao;
GRANT ALL ON SEQUENCE pgagent.pga_jobsteplog_jslid_seq TO syssao;
GRANT ALL ON SEQUENCE pgagent.pga_job_jobid_seq TO syssao;
GRANT ALL ON SEQUENCE pgagent.pga_schedule_jscid_seq TO syssao;
GRANT ALL ON SEQUENCE pgagent.pga_jobstep_jstid_seq TO syssao;
-- /* End - BUG#00004708 */

GRANT ALL ON FUNCTION sqlite_fdw_handler TO syssao;
GRANT ALL ON FUNCTION sqlite_fdw_handler TO syssao_role;

ALTER FUNCTION hg_secsch.current_is_syssao OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.current_is_syssao TO syssao_role;

ALTER FUNCTION hg_secsch.current_is_syssso OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.current_is_syssso TO syssao_role;

ALTER FUNCTION hg_secsch.current_is_superuser OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.current_is_superuser TO syssao_role;

ALTER FUNCTION hg_secsch.pg_clean_auditlog OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_clean_auditlog TO syssao_role;

ALTER FUNCTION hg_secsch.pg_clean_audit_config OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_clean_audit_config TO syssao_role;

ALTER FUNCTION hg_secsch.pg_delete_auditlog OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_delete_auditlog TO syssao_role;

ALTER FUNCTION hg_secsch.pg_delete_audit_config OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_delete_audit_config TO syssao_role;

ALTER FUNCTION hg_secsch.pg_clean_secmon_config OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_clean_secmon_config TO syssao_role;

ALTER FUNCTION hg_secsch.pg_clean_secmon_history OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_clean_secmon_history TO syssao_role;

ALTER FUNCTION hg_secsch.pg_audit_file_archive OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_audit_file_archive TO syssao_role;

ALTER FUNCTION hg_secsch.pg_audit_privilege(statement_opts text) OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_audit_privilege(statement_opts text) TO syssao_role;

ALTER FUNCTION hg_secsch.pg_audit_privilege(text, text) OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_audit_privilege(text, text) TO syssao_role;

ALTER FUNCTION hg_secsch.pg_noaudit_privilege(statement_opts text) OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_noaudit_privilege(statement_opts text) TO syssao_role;

ALTER FUNCTION hg_secsch.pg_noaudit_privilege(text, text) OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_noaudit_privilege(text, text) TO syssao_role;

ALTER FUNCTION hg_secsch.pg_noaudit_privilege_byoid OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_noaudit_privilege_byoid TO syssao_role;

ALTER FUNCTION hg_secsch.pg_audit_stmt(statement_opts text) OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_audit_stmt(statement_opts text) TO syssao_role;

ALTER FUNCTION hg_secsch.pg_audit_stmt(text, text) OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_audit_stmt(text, text) TO syssao_role;

ALTER FUNCTION hg_secsch.pg_noaudit_stmt(statement_opts text) OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_noaudit_stmt(statement_opts text) TO syssao_role;

ALTER FUNCTION hg_secsch.pg_noaudit_stmt(text, text) OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_noaudit_stmt(text, text) TO syssao_role;

ALTER FUNCTION hg_secsch.pg_noaudit_stmt_byoid OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_noaudit_stmt_byoid TO syssao_role;

ALTER FUNCTION hg_secsch.pg_audit_user(statement_opts text, username text) OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_audit_user(statement_opts text, username text) TO syssao_role;

ALTER FUNCTION hg_secsch.pg_audit_user(text, text, text) OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_audit_user(text, text, text) TO syssao_role;

ALTER FUNCTION hg_secsch.pg_audit_all_user(statement_opts text) OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_audit_all_user(statement_opts text) TO syssao_role;

ALTER FUNCTION hg_secsch.pg_audit_all_user(text, text) OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_audit_all_user(text, text) TO syssao_role;

ALTER FUNCTION hg_secsch.pg_noaudit_user(statement_opts text, username text) OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_noaudit_user(statement_opts text, username text) TO syssao_role;

ALTER FUNCTION hg_secsch.pg_noaudit_user(text, text, text) OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_noaudit_user(text, text, text) TO syssao_role;

ALTER FUNCTION hg_secsch.pg_noaudit_all_user(statement_opts text) OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_noaudit_all_user(statement_opts text) TO syssao_role;

ALTER FUNCTION hg_secsch.pg_noaudit_all_user(text, text) OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_noaudit_all_user(text, text) TO syssao_role;

ALTER FUNCTION hg_secsch.pg_noaudit_user_byoid OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_noaudit_user_byoid TO syssao_role;

ALTER FUNCTION hg_secsch.pg_audit_object(statement text, opts text, object text) OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_audit_object(statement text, opts text, object text) TO syssao_role;

ALTER FUNCTION hg_secsch.pg_audit_object(statement text, opts text, object text, schema text) OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_audit_object(statement text, opts text, object text, schema text) TO syssao_role;

ALTER FUNCTION hg_secsch.pg_audit_object(statement text, opts text, object text, schema text, whenever text) OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_audit_object(statement text, opts text, object text, schema text, whenever text) TO syssao_role;

ALTER FUNCTION hg_secsch.pg_noaudit_object(statement text, opts text, object text) OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_noaudit_object(statement text, opts text, object text) TO syssao_role;

ALTER FUNCTION hg_secsch.pg_noaudit_object(statement text, opts text, object text, schema text) OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_noaudit_object(statement text, opts text, object text, schema text) TO syssao_role;

ALTER FUNCTION hg_secsch.pg_noaudit_object(statement text, opts text, object text, schema text, whenever text) OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_noaudit_object(statement text, opts text, object text, schema text, whenever text) TO syssao_role;

ALTER FUNCTION hg_secsch.pg_noaudit_object_byoid OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_noaudit_object_byoid TO syssao_role;

ALTER FUNCTION hg_secsch.pg_create_secmon_rule OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_create_secmon_rule TO syssao_role;

ALTER FUNCTION hg_secsch.pg_alter_secmon_rule(ruleoid Oid, rulename text, monmode text, enable BOOLEAN) OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_alter_secmon_rule(ruleoid Oid, rulename text, monmode text, enable BOOLEAN) TO syssao_role;

ALTER FUNCTION hg_secsch.pg_alter_secmon_rule(ruleoid Oid, rulename text, monmode text) OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_alter_secmon_rule(ruleoid Oid, rulename text, monmode text) TO syssao_role;

ALTER FUNCTION hg_secsch.pg_alter_secmon_rule(ruleoid Oid, rulename text) OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_alter_secmon_rule(ruleoid Oid, rulename text) TO syssao_role;

ALTER FUNCTION hg_secsch.pg_drop_secmon_rule OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_drop_secmon_rule TO syssao_role;

ALTER FUNCTION hg_secsch.pg_create_secmon_threshold OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_create_secmon_threshold TO syssao_role;

ALTER FUNCTION hg_secsch.pg_alter_secmon_threshold OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_alter_secmon_threshold TO syssao_role;

ALTER FUNCTION hg_secsch.pg_drop_secmon_threshold OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.pg_drop_secmon_threshold TO syssao_role;

ALTER FUNCTION hg_secsch.audit_show_diskinfo OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.audit_show_diskinfo TO syssao_role;

ALTER FUNCTION hg_secsch.copy_file_to_remot_srv OWNER TO syssao;
GRANT ALL ON FUNCTION hg_secsch.copy_file_to_remot_srv TO syssao_role;

CREATE SERVER IF NOT EXISTS sqlite_server 
FOREIGN DATA WRAPPER sqlite_fdw
OPTIONS (database 'pgaudit/sqlite/sqlite.db');

CREATE FOREIGN TABLE hg_secsch.sqlite_audit_log(
	confid oid,
	sessionoid integer,
	useroid oid,
	username text,
	roleoid oid,
	rolename text,
	audittime text,
	result text,
	namespaceid oid,
	namespacename text,
	objectoid oid,
	objectname text,
	colname text,
	classid oid,
	objecttype text,
	audittype text,
	commandid integer,
	actionname text,
	clientport text,
	clientname text,
	clientip text,
	clientmac text,
	clientappname text,
	sqltext text,
	errtext text,
	authmethod text,
	privlevel text)
	SERVER sqlite_server
	OPTIONS (table 'pg_sqlite_audit_log');

ALTER TABLE hg_secsch.sqlite_audit_log OWNER TO syssao;
GRANT SELECT ON hg_secsch.sqlite_audit_log TO syssso;

GRANT SELECT ON hg_secsch.sqlite_audit_log to syssao_role;
GRANT SELECT ON hg_secsch.sqlite_audit_log to syssso_role;