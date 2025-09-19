DO $$
DECLARE
	new_table_name TEXT;
BEGIN
	new_table_name := 'pg_audit_log_' || TO_CHAR(CURRENT_DATE, 'YYYYMMDD');
	EXECUTE format('ALTER TABLE public.pg_audit_log RENAME TO %I', new_table_name);
END;
$$;

DO $$ 
BEGIN
    IF EXISTS (
        SELECT 1 FROM pg_tablespace WHERE spcname = 'audtblspc'
    ) THEN
       -- If a tablespace exists, create a table and specify this tablespace
        EXECUTE '
            CREATE TABLE public.pg_audit_log(
			confid oid, 
			sessionoid integer,
			useroid oid,
			username text,
			roleoid oid,
			rolename text,
			audittime timestamp(6) with time zone,
			result "char",
			namespaceid oid,
			namespacename text,
			objectoid oid,
			objectname text,
			colname text,
			classid oid,
			objecttype text,
			audittype "char",
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
            CREATE TABLE public.pg_audit_log(
			confid oid, 
			sessionoid integer,
			useroid oid,
			username text,
			roleoid oid,
			rolename text,
			audittime timestamp(6) with time zone,
			result "char",
			namespaceid oid,
			namespacename text,
			objectoid oid,
			objectname text,
			colname text,
			classid oid,
			objecttype text,
			audittype "char",
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
DECLARE
	new_table TEXT;
	new_str TEXT;
BEGIN
	new_table := 'pg_audit_log_' || TO_CHAR(CURRENT_DATE, 'YYYYMMDD');
	new_str := CURRENT_DATE + INTERVAL '1 day';
	EXECUTE format('ALTER TABLE public.pg_audit_log ATTACH PARTITION %I FOR VALUES FROM (MINVALUE) TO (%L);', new_table, new_str);
END;
$$;