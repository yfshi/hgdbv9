/* contrib/hg_antitamper/hg_antitamper--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION hg_antitamper" to load this file. \quit

/* create table: flag schema antitamper  */
DO $$ 
BEGIN
	IF NOT EXISTS (
		SELECT 1 FROM pg_class WHERE relname = 'schema_antitamper'
	) THEN
		-- If the tablespace does not exist, do not specify the tablespace when creating the table (use the default tablespace)
		EXECUTE '
			CREATE TABLE hg_security.schema_antitamper(
			schemaoid  Oid,
			antitamper bool
			);
			GRANT SELECT on hg_security.schema_antitamper to public;
			GRANT all on hg_security.schema_antitamper to syssso_role;
		';
	END IF;
END $$;

/* create global history table */
DO $$ 
BEGIN
	IF NOT EXISTS (
		SELECT 1 FROM pg_class WHERE relname = 'hg_global_antitamper'
	) THEN
		-- If the tablespace does not exist, do not specify the tablespace when creating the table (use the default tablespace)
		EXECUTE '
			CREATE SEQUENCE hg_security.antitamper_block as bigint;
			GRANT all on sequence hg_security.antitamper_block to syssso_role;
			GRANT all on sequence hg_security.antitamper_block to public;

			CREATE TABLE hg_security.hg_global_antitamper(
				blocknum int8 default nextval(''hg_security.antitamper_block'') primary key,
				username name,
				dbname name,
				schemaoid Oid,
				reloid Oid,
				insertrelhash int8,
				deleterelhash int8,
				starttime timestamp without time zone,
				sqlcommand text
			);
			GRANT select,insert on hg_security.hg_global_antitamper to public;
			GRANT all ON hg_security.hg_global_antitamper to syssso_role;
		';
	END IF;
END $$;

DO $$ 
BEGIN
	IF NOT EXISTS (
		SELECT 1 FROM pg_class WHERE relname = 'hg_global_antitamper_backup'
	) THEN
		-- If the tablespace does not exist, do not specify the tablespace when creating the table (use the default tablespace)
		EXECUTE '
			CREATE TABLE hg_security.hg_global_antitamper_backup(
				blocknum int,
				username name,
				dbname name,
				schemaoid Oid,
				reloid Oid,
				insertrelhash int8,
				deleterelhash int8,
				starttime timestamp without time zone,
				sqlcommand text
			);
			GRANT SELECT on hg_security.hg_global_antitamper_backup to public;
			GRANT all ON hg_security.hg_global_antitamper_backup to syssso_role;
		';
	END IF;
END $$;

CREATE OR REPLACE FUNCTION hg_security.check_schema_is_antitamper(schema_oid OID)
RETURNS BOOLEAN AS $$
DECLARE
	bc_table_exists BOOLEAN;
	value_exists BOOLEAN;
BEGIN
	SELECT EXISTS (
		SELECT 1
		FROM pg_class
		WHERE relname = 'schema_antitamper'
	) INTO bc_table_exists;

	IF NOT bc_table_exists THEN
		RETURN FALSE;
	END IF;

	SELECT EXISTS (
		SELECT 1
		FROM hg_security.schema_antitamper c
		WHERE c.schemaoid = schema_oid
	) INTO value_exists;

	IF NOT value_exists THEN
		RETURN FALSE;
	END IF;

	RETURN TRUE;
END;
$$ LANGUAGE plpgsql;

ALTER FUNCTION hg_security.check_schema_is_antitamper(schema_oid OID) owner to syssso_role;
GRANT EXECUTE on FUNCTION hg_security.check_schema_is_antitamper(schema_oid OID) to public;

/* calc hash value function */
CREATE OR REPLACE FUNCTION hg_security.calculate_row_hash(
	table_name regclass,
	row_data anyelement,
	exclude_columns text[] DEFAULT ARRAY['__hg_hash__']  -- exclude hash column
)
RETURNS int8
LANGUAGE plpgsql
STABLE
AS $$
DECLARE
	col_list text;
	query text;
	result int8;
BEGIN
	-- get table column and exclude system column and hash column
	WITH column_info AS (
		SELECT attname,
				format(
					'(hashtext(COALESCE(%I::text, ''''))::bigint & x''FFFFFFFF''::bigint)::text',
					attname
				) as hash_expr
		FROM pg_attribute
		WHERE attrelid = table_name
			AND attnum > 0
			AND NOT attisdropped
			AND attname != ALL(exclude_columns)
		ORDER BY attnum
	)
	SELECT string_agg(hash_expr, ' || ')
	INTO col_list
	FROM column_info;

	-- not column return null
	IF col_list IS NULL THEN
		RETURN NULL;
	END IF;

	-- construting the query :
	--     1.calculate the hash value for each column
	--     2.concatrnate them and calculate the final hash value
	query := format(
		'SELECT (hashtext(%s::text)::bigint & x''FFFFFFFF''::bigint) FROM (SELECT $1.*) t',
		rtrim(col_list, ' || ')
	);

	EXECUTE query INTO result USING row_data;

	RETURN result;
END;
$$;

ALTER FUNCTION hg_security.calculate_row_hash(
	table_name regclass,
	row_data anyelement,
	exclude_columns text[] )
owner to syssso_role;

GRANT EXECUTE on FUNCTION hg_security.calculate_row_hash(
	table_name regclass,
	row_data anyelement,
	exclude_columns text[] ) to public;

-- create trigger function
CREATE OR REPLACE FUNCTION hg_security.set_generic_hash()
RETURNS TRIGGER
LANGUAGE plpgsql
AS $$
DECLARE
	exclude_cols text[] := ARRAY['__hg_hash__'];  -- exclude hash column
	hash_del_val BIGINT := NULL;
	hash_ins_val BIGINT := NULL;
	current_dbname VARCHAR(100);
	current_username VARCHAR(100);
	current_schemaname VARCHAR(100);
	current_tablename VARCHAR(100);
	current_tableoid OID;
	schema_oid OID;
	hist_table_name text;
	global_hist_table_name text;
	sqlcommand text;
	current_table regclass;
	partitionroot regclass;
	partitionrootrelname text;
	nextblocknum BIGINT;
BEGIN
	-- get schema and table info
	current_dbname := current_database();
	current_username := current_user;
	current_schemaname := TG_TABLE_SCHEMA;
	current_tablename := TG_TABLE_NAME;
	current_tableoid := TG_RELID;

	SELECT c.relnamespace INTO schema_oid
	FROM pg_class c
	WHERE c.oid = current_tableoid;

	-- UPDATE/DELETE calc old hash value
	IF TG_OP = 'UPDATE' or TG_OP = 'DELETE' THEN
		hash_del_val := hg_security.calculate_row_hash(current_tableoid, OLD, exclude_cols);
	END IF;

	-- UPDATE/INSERT calc new hash value
	IF TG_OP = 'UPDATE' or TG_OP = 'INSERT' THEN
		hash_ins_val := hg_security.calculate_row_hash(current_tableoid, NEW, exclude_cols);
		-- set __hg_hash__ value
		NEW.__hg_hash__ := hash_ins_val;
	END IF;

	-- insert table history antitamper value
	hist_table_name := 'hist_' || current_tableoid;

	EXECUTE format(
		'SELECT COALESCE(MAX(rec_num), 0)+1 FROM hg_security.%I',
		hist_table_name) INTO nextblocknum;

	EXECUTE format(
		'INSERT INTO hg_security.%I (rec_num, hash_ins, hash_del) VALUES ($1, $2, $3)',
		hist_table_name)
		USING nextblocknum, hash_ins_val, hash_del_val
		;

	-- insert partition root table history antitamper value
	current_table = (current_schemaname || '.' || current_tablename)::regclass;
	select pg_partition_root(current_table) into partitionroot;
	IF partitionroot IS NOT NULL THEN
		select relname into partitionrootrelname from pg_class c join pg_namespace n on c.relnamespace = n.oid
			where c.oid = partitionroot;
		hist_table_name := 'hist_' || partitionroot::OID;
		EXECUTE format(
			'SELECT COALESCE(MAX(rec_num), 0)+1 FROM hg_security.%I',
			hist_table_name) INTO nextblocknum;
		EXECUTE format(
			'INSERT INTO hg_security.%I (rec_num, hash_ins, hash_del) VALUES ($1, $2, $3)',
			hist_table_name)
			USING nextblocknum, hash_ins_val, hash_del_val
			;
		current_tablename := partitionrootrelname;
		current_tableoid := partitionroot::oid;
	END IF;

	select query into sqlcommand from pg_stat_activity where pid = pg_backend_pid();
	-- insert global antitamper value
	INSERT INTO hg_security.hg_global_antitamper (
		username ,
		dbname,
		schemaoid,
		reloid,
		starttime,
		insertrelhash,
		deleterelhash,
		sqlcommand
	) VALUES (
		current_username,
		current_dbname,
		schema_oid,
		current_tableoid,
		CURRENT_TIMESTAMP::timestamp without time zone,
		hash_ins_val,
		hash_del_val,
		sqlcommand
	);

	IF TG_OP = 'DELETE' THEN
		RETURN OLD;
	END IF;
	RETURN NEW;
END;
$$;

ALTER FUNCTION hg_security.set_generic_hash() owner to syssso_role;
GRANT EXECUTE ON FUNCTION hg_security.set_generic_hash to PUBLIC;

-- get table hash diff value
CREATE OR REPLACE FUNCTION
hg_security.get_history_hash_diff_internal(
    hist_table_oid oid
)
RETURNS BIGINT
AS 'MODULE_PATHNAME', 'get_history_hash_diff_internal'
LANGUAGE C
STRICT
STABLE
PARALLEL UNSAFE;

ALTER FUNCTION hg_security.get_history_hash_diff_internal(
	hist_table_oid oid
)
owner to syssso_role;

-- insert table hash diff value
CREATE OR REPLACE FUNCTION
hg_security.append_history_hash_diff_internal(
    hist_table_oid oid,
    diff_value BIGINT
)
RETURNS BIGINT
AS 'MODULE_PATHNAME', 'append_history_hash_diff_internal'
LANGUAGE C
STRICT
VOLATILE
PARALLEL UNSAFE;

ALTER FUNCTION hg_security.append_history_hash_diff_internal(
    hist_table_oid oid,
    diff_value BIGINT
)
owner to syssso_role;

-- get table hash value
CREATE OR REPLACE FUNCTION hg_security.get_table_hash_sum(
	table_name text,
	schema_name text DEFAULT 'public',
	exclude_columns text[] DEFAULT ARRAY['__hg_hash__']
)
RETURNS BIGINT
AS 'MODULE_PATHNAME', 'get_table_hash_sum'
LANGUAGE C
STRICT
STABLE
PARALLEL UNSAFE;

ALTER FUNCTION hg_security.get_table_hash_sum(
	table_name text,
	schema_name text,
	exclude_columns text[])
owner to syssso_role;

-- antitamper_hist_check: is the sum of hash values in the user table consistent
--    with the sum of insert - delete in the history table
CREATE OR REPLACE FUNCTION hg_security.antitamper_hist_check(
	schema_name text,
	table_name text
)
RETURNS BOOLEAN
LANGUAGE plpgsql
AS $$
DECLARE
	table_hash_sum BIGINT;
	aaaa_insert_sum BIGINT;
	aaaa_delete_sum BIGINT;
	aaaa_diff BIGINT;
	full_table_name text;
	table_oid oid;
	schema_oid oid;
	hist_table_name text;
	hist_table_oid oid;
	query text;
	opt_val text;
	se_feature_mode_val text;
	is_superuser_val text;
BEGIN
	--check user permissions
	se_feature_mode_val = current_setting('se_feature.mode', true);
	is_superuser_val = current_setting('is_superuser', true);
	IF se_feature_mode_val IN('on', 'true', '1') THEN
		IF NOT hg_security.is_syssso(get_current_userid()) THEN
			RAISE EXCEPTION 'permission denied for function antitamper_hist_check';
		END IF;
	ELSE
		IF NOT is_superuser_val IN('on', 'true', '1') THEN
			RAISE EXCEPTION 'permission denied for function antitamper_hist_check';
		END IF;
	END IF;

	-- check function para
	IF schema_name IS NULL OR schema_name = '' OR table_name IS NULL  OR table_name = '' THEN
		RAISE EXCEPTION 'Parameter schema_name and table_name cannot be NULL';
	END IF;

	-- check table exists
	SELECT c.oid, c.relnamespace INTO table_oid, schema_oid
	FROM pg_class c
	JOIN pg_namespace n ON c.relnamespace = n.oid
	WHERE n.nspname = schema_name AND c.relname = table_name;

	IF table_oid IS NULL THEN
		RAISE EXCEPTION ' %.% does not exists ', schema_name, table_name;
	END IF;

	-- check is antitamper table
	SELECT option_value INTO opt_val FROM pg_options_to_table(
		(SELECT reloptions FROM pg_class WHERE oid = table_oid)
	) as opt WHERE opt.option_name = 'antitamper';

	IF opt_val is null or lower(opt_val) not in('true', 'on', 'yes', '1') THEN
		RAISE EXCEPTION ' %.% is not antitamper table.', schema_name, table_name;
	END IF;

	hist_table_name := 'hist_' || table_oid;

	-- 1. sum of hash values in user table
	table_hash_sum := hg_security.get_table_hash_sum(table_name, schema_name, ARRAY['__hg_hash__']);

	-- 2. sum of insert hash values in history table
	-- 3. sum of delete hash values in history table
	-- 4. insert - delete
	SELECT c.oid INTO hist_table_oid FROM pg_catalog.pg_class c
			JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace
		WHERE n.nspname = 'hg_security' AND c.relname = hist_table_name
			AND c.relkind IN ('r', 'p');
	IF hist_table_oid IS NULL THEN
		RAISE EXCEPTION
			'history table hg_security.% does not exist', hist_table_name;
	END IF;

	aaaa_diff := hg_security.get_history_hash_diff_internal(hist_table_oid);

	-- 5. check
	--RAISE NOTICE 'table: % hash values: %, history insert values: %, delete values: %, diff values: %',
	--	table_name, table_hash_sum, aaaa_insert_sum, aaaa_delete_sum, aaaa_diff;

	RETURN table_hash_sum = aaaa_diff;
END;
$$;

ALTER FUNCTION hg_security.antitamper_hist_check(
	schema_name text ,
	table_name text
)
owner to syssso_role;

-- antitamper_hist_repair: comparative value and add a column of difference
CREATE OR REPLACE FUNCTION hg_security.antitamper_hist_repair(
	schema_name text,
	table_name text
)
RETURNS BOOLEAN
LANGUAGE plpgsql
AS $$
DECLARE
	table_hash_sum BIGINT;
	aaaa_insert_sum BIGINT;
	aaaa_delete_sum BIGINT;
	aaaa_diff BIGINT;
	diff_value BIGINT;
	full_table_name text;
	table_oid oid;
	schema_oid oid;
	hist_table_name text;
	hist_table_oid oid;
	query text;
	nextblocknum BIGINT;
	opt_val text;
	se_feature_mode_val text;
	is_superuser_val text;
BEGIN
	--check user permissions
	se_feature_mode_val = current_setting('se_feature.mode', true);
	is_superuser_val = current_setting('is_superuser', true);
	IF se_feature_mode_val IN('on', 'true', '1') THEN
		IF NOT hg_security.is_syssso(get_current_userid()) THEN
			RAISE EXCEPTION 'permission denied for function antitamper_hist_repair';
		END IF;
	ELSE
		IF NOT is_superuser_val IN('on', 'true', '1') THEN
			RAISE EXCEPTION 'permission denied for function antitamper_hist_repair';
		END IF;
	END IF;

	-- check function para
	IF schema_name IS NULL OR schema_name = '' OR table_name IS NULL  OR table_name = '' THEN
		RAISE EXCEPTION 'Parameter schema_name and table_name cannot be NULL';
	END IF;

	-- check table exists
	SELECT c.oid, c.relnamespace INTO table_oid, schema_oid
	FROM pg_class c
	JOIN pg_namespace n ON c.relnamespace = n.oid
	WHERE n.nspname = schema_name
	  AND c.relname = table_name;

	IF table_oid IS NULL THEN
		RAISE EXCEPTION ' %.% does not exists ', schema_name, table_name;
	END IF;

	-- check is antitamper table
	SELECT option_value INTO opt_val FROM pg_options_to_table(
		(SELECT reloptions FROM pg_class WHERE oid = table_oid)
	) as opt WHERE opt.option_name = 'antitamper';

	IF opt_val is null or lower(opt_val) not in('true', 'on', 'yes', '1') THEN
		RAISE EXCEPTION ' %.% is not antitamper table.', schema_name, table_name;
	END IF;

	hist_table_name := 'hist_' || table_oid;

	-- 1. sum of hash values in user table
	table_hash_sum := hg_security.get_table_hash_sum(table_name, schema_name, ARRAY['__hg_hash__']);

	-- 2. sum of insert hash values in history table
	-- 3. sum of delete hash values in history table
	-- 4. insert - delete
	SELECT c.oid INTO hist_table_oid FROM pg_catalog.pg_class c
			JOIN pg_catalog.pg_namespace n ON n.oid = c.relnamespace
		WHERE n.nspname = 'hg_security' AND c.relname = hist_table_name
			AND c.relkind IN ('r', 'p');
	IF hist_table_oid IS NULL THEN
		RAISE EXCEPTION
			'history table hg_security.% does not exist', hist_table_name;
	END IF;

	aaaa_diff := hg_security.get_history_hash_diff_internal(hist_table_oid);

	-- 5. check
	--RAISE NOTICE 'table: % hash values: %, history insert values: %, delete values: %, diff values: %',
	--	table_name, table_hash_sum, aaaa_insert_sum, aaaa_delete_sum, aaaa_diff;

	diff_value = table_hash_sum - aaaa_diff;

	-- 6. insert difference column
	IF diff_value <> 0 THEN
		nextblocknum = hg_security.append_history_hash_diff_internal(hist_table_oid, diff_value);
	END IF;

	RETURN TRUE;
END;
$$;

ALTER FUNCTION hg_security.antitamper_hist_repair(
	schema_name text ,
	table_name text
)
owner to syssso_role;

-- antitamper_gchain_check:  is the sum of hash values in the user table consistent
--    with the sum of insert - delete in the global table
CREATE OR REPLACE FUNCTION hg_security.antitamper_gchain_check(
	schema_name text,
	table_name text
)
RETURNS BOOLEAN
LANGUAGE plpgsql
AS $$
DECLARE
	table_hash_sum BIGINT;
	aaaa_insert_sum BIGINT;
	aaaa_delete_sum BIGINT;
	aaaa_diff BIGINT;
	full_table_name text;
	table_oid oid;
	query text;
	opt_val text;
	se_feature_mode_val text;
	is_superuser_val text;
BEGIN
	--check user permissions
	se_feature_mode_val = current_setting('se_feature.mode', true);
	is_superuser_val = current_setting('is_superuser', true);
	IF se_feature_mode_val IN('on', 'true', '1') THEN
		IF NOT hg_security.is_syssso(get_current_userid()) THEN
			RAISE EXCEPTION 'permission denied for function antitamper_gchain_check';
		END IF;
	ELSE
		IF NOT is_superuser_val IN('on', 'true', '1') THEN
			RAISE EXCEPTION 'permission denied for function antitamper_gchain_check';
		END IF;
	END IF;

	-- check function para
	IF schema_name IS NULL OR schema_name = '' OR table_name IS NULL  OR table_name = '' THEN
		RAISE EXCEPTION 'Parameter schema_name and table_name cannot be NULL';
	END IF;

	-- check table exists
	SELECT c.oid INTO table_oid
	FROM pg_class c
	JOIN pg_namespace n ON c.relnamespace = n.oid
	WHERE n.nspname = schema_name
	  AND c.relname = table_name;

	IF table_oid IS NULL THEN
		RAISE EXCEPTION ' %.% does not exists ', schema_name, table_name;
	END IF;

	-- check is antitamper table
	SELECT option_value INTO opt_val FROM pg_options_to_table(
		(SELECT reloptions FROM pg_class WHERE oid = table_oid)
	) as opt WHERE opt.option_name = 'antitamper';

	IF opt_val is null or lower(opt_val) not in('true', 'on', 'yes', '1') THEN
		RAISE EXCEPTION ' %.% is not antitamper table.', schema_name, table_name;
	END IF;

	-- 1. sum of hash values in user table
	table_hash_sum := hg_security.get_table_hash_sum(table_name, schema_name, ARRAY['__hg_hash__']);

	-- 2. sum of insert hash values in global table
	query := format(
		'SELECT COALESCE(SUM(insertrelhash)::BIGINT, 0)
			FROM hg_security.hg_global_antitamper
			WHERE insertrelhash IS NOT NULL AND reloid = %s',
		 table_oid
	);
	EXECUTE query INTO aaaa_insert_sum;

	-- 3. sum of delete hash values in global table
	query := format(
		'SELECT COALESCE(SUM(deleterelhash)::BIGINT, 0)
			FROM hg_security.hg_global_antitamper
			WHERE deleterelhash IS NOT NULL AND reloid = %s',
		table_oid
	);
	EXECUTE query INTO aaaa_delete_sum;

	-- 4. insert - delete
	aaaa_diff := aaaa_insert_sum - aaaa_delete_sum;

	-- 5. check
	--RAISE NOTICE 'table: % hash values: %, history insert values: %, delete values: %, diff values: %',
	--	table_name, table_hash_sum, aaaa_insert_sum, aaaa_delete_sum, aaaa_diff;

	RETURN table_hash_sum = aaaa_diff;
END;
$$;

ALTER FUNCTION hg_security.antitamper_gchain_check(
	schema_name text ,
	table_name text
)
owner to syssso_role;

-- antitamper_gchain_repair: comparative value and add a column of difference
CREATE OR REPLACE FUNCTION hg_security.antitamper_gchain_repair(
	schema_name text,
	table_name text
)
RETURNS BOOLEAN
LANGUAGE plpgsql
AS $$
DECLARE
	table_hash_sum BIGINT;
	aaaa_insert_sum BIGINT;
	aaaa_delete_sum BIGINT;
	aaaa_diff BIGINT;
	diff_value BIGINT;
	full_table_name text;
	schema_oid oid;
	table_oid oid;
	query text;
	opt_val text;
	se_feature_mode_val text;
	is_superuser_val text;
BEGIN
	--check user permissions
	se_feature_mode_val = current_setting('se_feature.mode', true);
	is_superuser_val = current_setting('is_superuser', true);
	IF se_feature_mode_val IN('on', 'true', '1') THEN
		IF NOT hg_security.is_syssso(get_current_userid()) THEN
			RAISE EXCEPTION 'permission denied for function antitamper_gchain_repair';
		END IF;
	ELSE
		IF NOT is_superuser_val IN('on', 'true', '1') THEN
			RAISE EXCEPTION 'permission denied for function antitamper_gchain_repair';
		END IF;
	END IF;

	-- check function para
	IF schema_name IS NULL OR schema_name = '' OR table_name IS NULL  OR table_name = '' THEN
		RAISE EXCEPTION 'Parameter schema_name and table_name cannot be NULL';
	END IF;

	-- check table exists
	SELECT c.oid ,c.relnamespace INTO table_oid, schema_oid
	FROM pg_class c
	JOIN pg_namespace n ON c.relnamespace = n.oid
	WHERE n.nspname = schema_name AND c.relname = table_name;

	IF table_oid IS NULL THEN
		RAISE EXCEPTION ' %.% does not exists ', schema_name, table_name;
	END IF;

	-- check is antitamper table
	SELECT option_value INTO opt_val FROM pg_options_to_table(
		(SELECT reloptions FROM pg_class WHERE oid = table_oid)
	) as opt WHERE opt.option_name = 'antitamper';

	IF opt_val is null or lower(opt_val) not in('true', 'on', 'yes', '1') THEN
		RAISE EXCEPTION ' %.% is not antitamper table.', schema_name, table_name;
	END IF;

	-- 1. sum of hash values in user table
	table_hash_sum := hg_security.get_table_hash_sum(table_name, schema_name, ARRAY['__hg_hash__']);

	-- 2. sum of insert hash values in global table
	query := format(
		'SELECT COALESCE(SUM(insertrelhash)::BIGINT, 0)
			FROM hg_security.hg_global_antitamper
			WHERE insertrelhash IS NOT NULL AND reloid = %s',
		table_oid
	);
	EXECUTE query INTO aaaa_insert_sum;

	-- 3. sum of delete hash values in global table
	query := format(
		'SELECT COALESCE(SUM(deleterelhash)::BIGINT, 0)
			FROM hg_security.hg_global_antitamper
			WHERE deleterelhash IS NOT NULL AND reloid = %s',
		table_oid
	);
	EXECUTE query INTO aaaa_delete_sum;

	-- 4. insert - delete
	aaaa_diff := aaaa_insert_sum - aaaa_delete_sum;

	-- 5. check
	--RAISE NOTICE 'table: % hash values: %, history insert values: %, delete values: %, diff values: %',
	--	table_name, table_hash_sum, aaaa_insert_sum, aaaa_delete_sum, aaaa_diff;

	diff_value = table_hash_sum - aaaa_diff;

	-- 6. insert difference column
	IF diff_value != 0 THEN
		IF diff_value > 0 THEN
			-- positive numbers:insert
			INSERT INTO hg_security.hg_global_antitamper (
					username ,
					dbname,
					schemaoid,
					reloid,
					starttime,
					insertrelhash,
					deleterelhash,
					sqlcommand
				) VALUES (
					current_user,
					current_database(),
					schema_oid,
					table_oid,
					CURRENT_TIMESTAMP::timestamp without time zone,
					ABS(diff_value),
					NULL,
					'REPAIR INSERT'
				);
			--RAISE NOTICE 'diff_value =insertrelhash[%] ',diff_value;
		ELSE
			-- negative numbers:delete
			INSERT INTO hg_security.hg_global_antitamper (
					username ,
					dbname,
					schemaoid,
					reloid,
					starttime,
					insertrelhash,
					deleterelhash,
					sqlcommand
				) VALUES (
					current_user,
					current_database(),
					schema_oid,
					table_oid,
					CURRENT_TIMESTAMP::timestamp without time zone,
					NULL,
					ABS(diff_value),
					'REPAIR DELETE'
				);
			--RAISE NOTICE 'diff_value =deleterelhash[%] ',ABS(diff_value);
		END IF;
	ELSE
		--RAISE NOTICE 'data is consistent, no supplementation required';
	END IF;

	RETURN TRUE;
END;
$$;

ALTER FUNCTION hg_security.antitamper_gchain_repair(
	schema_name text,
	table_name text
)
owner to syssso_role;

-- antitamper_gchain_archive
CREATE OR REPLACE FUNCTION hg_security.antitamper_gchain_archive(
	schema_name text,
	table_name text
)
RETURNS BOOLEAN
LANGUAGE plpgsql
AS $$
DECLARE
	table_hash_sum BIGINT;
	aaaa_insert_sum BIGINT;
	aaaa_delete_sum BIGINT;
	aaaa_diff BIGINT;
	diff_value BIGINT;
	full_table_name text;
	table_oid oid;
	schema_oid oid;
	query text;
	opt_val text;
	se_feature_mode_val text;
	is_superuser_val text;
BEGIN
	--check user permissions
	se_feature_mode_val = current_setting('se_feature.mode', true);
	is_superuser_val = current_setting('is_superuser', true);
	IF se_feature_mode_val IN('on', 'true', '1') THEN
		IF NOT hg_security.is_syssso(get_current_userid()) THEN
			RAISE EXCEPTION 'permission denied for function antitamper_gchain_archive';
		END IF;
	ELSE
		IF NOT is_superuser_val IN('on', 'true', '1') THEN
			RAISE EXCEPTION 'permission denied for function antitamper_gchain_archive';
		END IF;
	END IF;

	-- check function para
	IF schema_name IS NULL OR schema_name = '' OR table_name IS NULL  OR table_name = '' THEN
		RAISE EXCEPTION 'Parameter schema_name and table_name cannot be NULL';
	END IF;

	-- check table exists
	SELECT c.oid, c.relnamespace INTO table_oid, schema_oid
	FROM pg_class c
	JOIN pg_namespace n ON c.relnamespace = n.oid
	WHERE n.nspname = schema_name
	  AND c.relname = table_name;

	IF table_oid IS NULL THEN
		RAISE EXCEPTION ' %.% does not exists ', schema_name, table_name;
	END IF;

	-- check is antitamper table
	SELECT option_value INTO opt_val FROM pg_options_to_table(
		(SELECT reloptions FROM pg_class WHERE oid = table_oid)
	) as opt WHERE opt.option_name = 'antitamper';

	IF opt_val is null or lower(opt_val) not in('true', 'on', 'yes', '1') THEN
		RAISE EXCEPTION ' %.% is not antitamper table.', schema_name, table_name;
	END IF;

	-- 1. sum of hash values in user table
	table_hash_sum := hg_security.get_table_hash_sum(table_name, schema_name, ARRAY['__hg_hash__']);

	-- 2. sum of insert hash values in global table
	query := format(
		'SELECT COALESCE(SUM(insertrelhash)::BIGINT, 0)
			FROM hg_security.hg_global_antitamper
			WHERE insertrelhash IS NOT NULL AND reloid = %s',
		table_oid
	);
	EXECUTE query INTO aaaa_insert_sum;

	-- 3. sum of delete hash values in global table
	query := format(
		'SELECT COALESCE(SUM(deleterelhash)::BIGINT, 0)
			FROM hg_security.hg_global_antitamper
			WHERE deleterelhash IS NOT NULL AND reloid = %s',
		table_oid
	);
	EXECUTE query INTO aaaa_delete_sum;

	-- 4. insert bak global antitamper value
	insert into hg_security.hg_global_antitamper_backup
		select * from hg_security.hg_global_antitamper
			where reloid = table_oid;

	delete from hg_security.hg_global_antitamper
		where reloid = table_oid;

	-- 5. insert global antitamper value
	INSERT INTO hg_security.hg_global_antitamper (
		username ,
		dbname,
		schemaoid,
		reloid,
		starttime,
		insertrelhash,
		deleterelhash,
		sqlcommand
	) VALUES (
		current_user,
		current_database(),
		schema_oid,
		table_oid,
		CURRENT_TIMESTAMP::timestamp without time zone,
		aaaa_insert_sum,
		aaaa_delete_sum,
		'archive'
	);
	RETURN TRUE;
END;
$$;

ALTER FUNCTION hg_security.antitamper_gchain_archive(
	schema_name text,
	table_name text
)
owner to syssso_role;

-- antitamper view
-- schema view : show all schema
create view hg_security.antitamper_class as
	select
	oid,
	relname,
	relnamespace,
	relowner,
	'hist_' || oid::text as hist_table_name
	from pg_class
	where reloptions is not null
	and ('antitamper=true' = any(reloptions) 
		or 'antitamper=on' = any(reloptions) 
		or 'antitamper=1' = any(reloptions));

-- table view : show antitamper table
create view hg_security.antitamper_namespace as
	select
	n.oid,
	n.nspname,
	n.nspowner,
	n.nspacl,
	case
		when a.schemaoid is not null and a.antitamper = true then true
		else false
	end as isantitamper
	from pg_namespace n
	left join hg_security.schema_antitamper a
	on n.oid = a.schemaoid;
