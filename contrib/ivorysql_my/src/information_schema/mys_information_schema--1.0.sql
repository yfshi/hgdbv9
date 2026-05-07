/*
 * MySQL Information Schema
 *
 * contrib/ivorysql_my/src/information_schema/mys_information_schema--1.0.sql
 */

--
-- Function: mys_information_schema.mys_format_type (internal)
-- Maps a PostgreSQL type OID to a MySQL-compatible DATA_TYPE string.
--
CREATE OR REPLACE FUNCTION mys_information_schema.mys_format_type(type_oid oid)
RETURNS varchar(64)
LANGUAGE sql 
STABLE 
PARALLEL SAFE
RETURNS NULL ON NULL INPUT
AS $$
	SELECT CASE
		WHEN t.typname = 'mysbit'  THEN 'bit'
		WHEN t.typname = 'bpchar'  THEN 'char'
		WHEN t.typname = 'float8'  THEN 'double'
		WHEN t.typname = 'float4'  THEN 'float'
		WHEN t.typname = 'int4'    THEN 'int'
		WHEN t.typname = 'numeric' THEN 'decimal'
		WHEN t.typname = 'varchar' THEN 'varchar'
		WHEN t.typname = 'year'    THEN 'year'
		WHEN t.typtype = 'e'       THEN 'enum'
		ELSE format_type(type_oid, null)
	END
	FROM pg_type t WHERE t.oid = type_oid
$$;

--
-- View: mys_information_schema.tables
--

CREATE OR REPLACE VIEW mys_information_schema.tables AS
SELECT
	'def'::varchar(512) AS "TABLE_CATALOG",                                    -- Name of the database catalog (def)
	n.nspname::varchar(64) AS "TABLE_SCHEMA",                                  -- Name of the schema containing the table
	c.relname::varchar(64) AS "TABLE_NAME",                                    -- Table name

	CASE
		WHEN n.nspname = 'information_schema' THEN 'SYSTEM VIEW'               -- System views from information_schema
		WHEN c.relkind = 'r' THEN 'BASE TABLE'                                 -- Ordinary table
		WHEN c.relkind = 'p' THEN 'BASE TABLE'                                 -- Partitioned table
		WHEN c.relkind = 'v' THEN 'VIEW'                                       -- View
		WHEN c.relkind = 'm' THEN 'VIEW'                                       -- Materialized view
		WHEN c.relkind = 'f' THEN 'FOREIGN TABLE'                              -- Foreign table
		ELSE NULL
	END::varchar(64) AS "TABLE_TYPE",

	CASE
		WHEN c.relkind = 'v' THEN NULL /* Bug - #z5989 */
		ELSE 'InnoDB'
	END::varchar(64) AS "ENGINE",                                              -- Storage engine (access method), e.g. heap

	CAST(NULL AS unsigned) AS "VERSION",                                       -- Version info (not supported in PostgreSQL)

	CASE
		WHEN c.relkind = 'v' THEN NULL /* Bug - #z5989 */
		ELSE 'Dynamic'
	END::varchar(10) AS "ROW_FORMAT",                                          -- Row storage format description

	CAST(
		CASE
			WHEN c.relkind = 'v' THEN NULL /* Bug - #z5989 */
			ELSE n_live_tup
		END AS unsigned) AS "TABLE_ROWS",                                       -- Estimated number of live rows (from pg_stat_user_tables)

	CAST(
		CASE
			WHEN c.relkind = 'v' THEN NULL /* Bug - #z5989 */
			ELSE
				CASE (us.n_tup_ins - us.n_tup_del)                              -- Average row length (estimated)
					WHEN 0 THEN 0
					ELSE (pg_catalog.pg_total_relation_size(c.oid::regclass) / (us.n_tup_ins - us.n_tup_del))
				END
		END AS unsigned) AS "AVG_ROW_LENGTH",

	CAST(
		CASE
			WHEN c.relkind = 'v' THEN NULL /* Bug - #z5989 */
			ELSE pg_catalog.pg_total_relation_size(c.oid::regclass)
		END AS unsigned) AS "DATA_LENGTH",                                     -- Total size of the table including all indexes and TOAST data

	CAST(CASE c.relkind
			WHEN 'r' THEN 0
			ELSE NULL
		END AS unsigned) AS "MAX_DATA_LENGTH",                                 -- Maximum data length (not supported)

	CAST(
		CASE
			WHEN c.relkind = 'v' THEN NULL /* Bug - #z5989 */
			ELSE pg_catalog.pg_indexes_size(c.oid::regclass)
		END AS unsigned) AS "INDEX_LENGTH",                                    -- Total size of all indexes attached to the table

	CAST(CASE
			WHEN c.relkind = 'v' THEN NULL /* Bug - #z5989 */
			ELSE '16384'
		END AS unsigned) AS "DATA_FREE",                                       -- Estimated free space in tablespace (not precise)

	CAST(
		CASE
			WHEN c.relkind = 'v' THEN NULL /* Bug - #z5989 */
			ELSE
				(SELECT CASE MYSSERIES                                         -- AUTO_INCREMENT value (for identity/serial columns)
					WHEN NULL THEN NULL
					ELSE
						(SELECT ivymy.show_next_autoinc(c.oid))
					END
				FROM (SELECT pg_catalog.pg_get_serial_sequence(c.relname, at.attname) AS seriesname
					FROM pg_attribute at
					WHERE at.attrelid = c.oid AND (at.attidentity = 'a' or at.attidentity = 'd' or at.attidentity = 'm')) AS MYSSERIES)
			END AS unsigned) AS "AUTO_INCREMENT",

	CASE
		WHEN c.relkind = 'v' THEN NULL /* Bug - #z5989 */
		ELSE ob.created
	END::mysdatetime AS "CREATE_TIME",                                  -- Table creation timestamp (from pg_object)

	CASE
		WHEN c.relkind = 'v' THEN NULL /* Bug - #z5989 */
		ELSE ob.modified
	END::mysdatetime AS "UPDATE_TIME",                                  -- Table last modification timestamp (from pg_object)

	NULL::mysdatetime AS "CHECK_TIME",                                  -- Last check time (not supported)

	CASE
		WHEN c.relkind = 'v' THEN NULL /* Bug - #z5989 */
		ELSE (SELECT d.datcollate
				FROM pg_catalog.pg_database d
				  WHERE d.datname = current_database())
		END::varchar(32) AS "TABLE_COLLATION", -- Default collation of the table

	CAST(NULL AS unsigned) AS "CHECKSUM",                                -- Table checksum (not supported)

	CASE c.relispartition                                                -- Additional create options (e.g. partitioned)
		WHEN TRUE THEN 'partitioned'
		ELSE NULL
	END::varchar(255) AS "CREATE_OPTIONS",

	CASE
		WHEN c.relkind = 'v' THEN 'VIEW' /* Bug - #z5989 */
		ELSE pg_catalog.obj_description(c.oid)
	END::varchar(2048) AS "TABLE_COMMENT"                                -- Table comment/description
FROM pg_catalog.pg_class c
	LEFT JOIN pg_catalog.pg_object ob ON c.oid = ob.obj_oid
	LEFT JOIN pg_catalog.pg_namespace n ON c.relnamespace = n.oid
	LEFT JOIN pg_catalog.pg_stat_user_tables us ON c.oid = us.relid
WHERE
	c.relpersistence != 't' AND /* Bug - #z5989 */
	c.relkind IN ('r', 'v', 'm', 'p', 'f')                              -- Tables, views, materialized views, partitioned tables, foreign tables
ORDER BY c.oid;		-- bug#Z5983


GRANT SELECT ON mys_information_schema.tables TO PUBLIC;

--
-- View: mys_information_schema.table_privileges
--

CREATE OR REPLACE VIEW mys_information_schema.table_privileges AS
SELECT
	-- The name of the grantee (role) in the format 'rolename@localhost'
	concat(quote_ident(pg_roles.rolname), '@', 'localhost')::varchar(81) AS "GRANTEE",

	-- Name of the database catalog (always 'def')
	'def'::varchar(512) AS "TABLE_CATALOG",

	-- Name of the schema that contains the table
	nc.nspname::varchar(64) AS "TABLE_SCHEMA",

	-- Name of the table
	c.relname::varchar(64) AS "TABLE_NAME",

	-- Type of privilege granted to the role (one of SELECT, INSERT, UPDATE, etc.)
	unnest(string_to_array(privs.priv, ','))::varchar(64) AS "PRIVILEGE_TYPE",

	-- Whether the privilege is grantable to other users (simulated using pg_has_role with USAGE)
	CASE
		WHEN pg_has_role(pg_roles.oid, 'USAGE') THEN 'YES'
		ELSE 'NO'
	END::varchar(3) AS "IS_GRANTABLE"

FROM
	-- All relations (tables, views, etc.)
	pg_class c

	-- Join with namespaces (schemas)
	JOIN pg_namespace nc ON nc.oid = c.relnamespace

	-- Cross join with all database roles
	JOIN pg_roles ON true

	-- LATERAL join to extract privilege types the role has on the table
	JOIN LATERAL (
		SELECT array_to_string(array[
			CASE WHEN has_table_privilege(pg_roles.rolname, c.oid, 'SELECT') THEN 'SELECT' END,
			CASE WHEN has_table_privilege(pg_roles.rolname, c.oid, 'INSERT') THEN 'INSERT' END,
			CASE WHEN has_table_privilege(pg_roles.rolname, c.oid, 'UPDATE') THEN 'UPDATE' END,
			CASE WHEN has_table_privilege(pg_roles.rolname, c.oid, 'DELETE') THEN 'DELETE' END,
			CASE WHEN has_table_privilege(pg_roles.rolname, c.oid, 'TRUNCATE') THEN 'TRUNCATE' END,
			CASE WHEN has_table_privilege(pg_roles.rolname, c.oid, 'REFERENCES') THEN 'REFERENCES' END,
			CASE WHEN has_table_privilege(pg_roles.rolname, c.oid, 'TRIGGER') THEN 'TRIGGER' END
		], ',') AS priv
	) AS privs ON privs.priv IS NOT NULL

-- Only include valid relation kinds (regular tables, partitioned tables, views, foreign tables, materialized views)
WHERE c.relkind IN ('r', 'p', 'v', 'f', 'm');

GRANT SELECT ON mys_information_schema.table_privileges TO PUBLIC;

--
-- View: mys_information_schema.partitions /* Bug - #z5987 */
--

CREATE OR REPLACE VIEW mys_information_schema.partitions AS
WITH RECURSIVE partition_tree AS (
	-- Find all root partitioned tables (top level)
	SELECT
		c.oid AS root_oid,
		c.relname AS root_name,
		n.nspname AS root_schema,
		c.oid AS partition_oid,
		c.relname AS partition_name,
		NULL::oid AS parent_oid,
		NULL::name AS parent_name,
		0 AS level,
		c.relname AS path,
		TRUE AS is_root
	FROM pg_class c
	JOIN pg_namespace n ON n.oid = c.relnamespace
	WHERE c.relkind = 'p'  -- Partitioned table
		AND n.nspname NOT IN ('pg_catalog', 'information_schema')

	UNION ALL

	-- Recursively find all child partitions
	SELECT
		pt.root_oid,
		pt.root_name,
		pt.root_schema,
		c.oid AS partition_oid,
		c.relname AS partition_name,
		pt.partition_oid AS parent_oid,
		pt.partition_name AS parent_name,
		pt.level + 1 AS level,
		pt.path || '.' || c.relname AS path,
		FALSE AS is_root
	FROM partition_tree pt
	JOIN pg_inherits i ON i.inhparent = pt.partition_oid
	JOIN pg_class c ON c.oid = i.inhrelid
	WHERE c.relkind IN ('p', 'r')  -- Partitioned table or regular table
),
partition_relationships AS (
	-- Find direct subpartitions for each partition
	SELECT
		pt.partition_oid,
		pt.partition_name,
		pt.root_oid,
		pt.root_name,
		pt.root_schema,
		pt.level,
		pt.path,
		pt.is_root,
		pt.parent_oid,
		child.oid AS subpartition_oid,
		child.relname AS subpartition_name,
		child_ns.nspname AS subpartition_schema
	FROM partition_tree pt
	LEFT JOIN pg_inherits i ON i.inhparent = pt.partition_oid
	LEFT JOIN pg_class child ON child.oid = i.inhrelid
	LEFT JOIN pg_namespace child_ns ON child_ns.oid = child.relnamespace
),
partition_info AS (
	SELECT
		pr.partition_oid,
		pr.partition_name,
		pr.root_oid,
		pr.root_name,
		pr.root_schema,
		pr.level,
		pr.path,
		pr.is_root,
		pr.parent_oid,
		pr.subpartition_oid,
		pr.subpartition_name,
		pr.subpartition_schema,
		-- Get partition method from the actual partitioned table
		CASE
			WHEN pt_root.partstrat IS NOT NULL THEN
				CASE pt_root.partstrat
					WHEN 'r' THEN 'RANGE'
					WHEN 'l' THEN 'LIST'
					WHEN 'h' THEN 'HASH'
					ELSE NULL
				END
			ELSE NULL
		END AS root_partition_method,
		-- Get partition expression from root partitioned table
		CASE
			WHEN pt_root.partstrat IS NOT NULL THEN
				pg_get_partkeydef(pr.root_oid)
			ELSE NULL
		END AS root_partition_expression,
		-- Get current partition method (if this partition is itself partitioned)
		CASE
			WHEN pt_current.partstrat IS NOT NULL THEN
				CASE pt_current.partstrat
					WHEN 'r' THEN 'RANGE'
					WHEN 'l' THEN 'LIST'
					WHEN 'h' THEN 'HASH'
					ELSE NULL
				END
			ELSE NULL
		END AS current_partition_method,
		-- Get current partition expression
		CASE
			WHEN pt_current.partstrat IS NOT NULL THEN
				pg_get_partkeydef(pr.partition_oid)
			ELSE NULL
		END AS current_partition_expression,
		-- Get subpartition method from the parent partition (if parent is partitioned)
		CASE
			WHEN pr.subpartition_oid IS NOT NULL AND pt_current.partstrat IS NOT NULL THEN
				CASE pt_current.partstrat
					WHEN 'r' THEN 'RANGE'
					WHEN 'l' THEN 'LIST'
					WHEN 'h' THEN 'HASH'
					ELSE NULL
				END
			ELSE NULL
		END AS subpartition_method,
		-- Get subpartition expression from the parent partition
		CASE
			WHEN pr.subpartition_oid IS NOT NULL AND pt_current.partstrat IS NOT NULL THEN
				pg_get_partkeydef(pr.partition_oid)
			ELSE NULL
		END AS subpartition_expression,
		-- Store the parent's partition method for correct inheritance
		CASE
			WHEN pr.parent_oid IS NOT NULL THEN
				(SELECT
					CASE pt_parent.partstrat
						WHEN 'r' THEN 'RANGE'
						WHEN 'l' THEN 'LIST'
						WHEN 'h' THEN 'HASH'
						ELSE NULL
					END
				FROM pg_partitioned_table pt_parent
				WHERE pt_parent.partrelid = pr.parent_oid)
			ELSE NULL
		END AS parent_partition_method,
		-- Store the parent's partition expression for correct inheritance
		CASE
			WHEN pr.parent_oid IS NOT NULL THEN
				pg_get_partkeydef(pr.parent_oid)
			ELSE NULL
		END AS parent_partition_expression
	FROM partition_relationships pr
	-- Root partition info
	LEFT JOIN pg_partitioned_table pt_root ON pt_root.partrelid = pr.root_oid
	-- Current partition info (to get subpartition info)
	LEFT JOIN pg_partitioned_table pt_current ON pt_current.partrelid = pr.partition_oid
),
filtered_partitions AS (
	SELECT *
	FROM partition_info pi
	-- Exclude rows where partition_name and table_name are the same.
	WHERE pi.partition_name != pi.root_name
		AND pi.root_oid IN (
			SELECT c.oid
			  FROM pg_class c
			  JOIN pg_namespace n ON n.oid = c.relnamespace
			  WHERE c.relkind = 'p'
			AND n.nspname NOT IN ('pg_catalog', 'information_schema', 'mys_information_schema')
			AND NOT EXISTS (
				SELECT 1 FROM pg_inherits WHERE inhrelid = c.oid
			)
		)
)
SELECT
	-- Database name (def)
	'def'::varchar(512) AS "TABLE_CATALOG",

	-- Schema name containing the root partitioned table
	fp.root_schema::varchar(64) AS "TABLE_SCHEMA",

	-- Name of the root partitioned table (top-level table name)
	fp.root_name::varchar(64) AS "TABLE_NAME",

	-- Current partition name
	fp.partition_name::varchar(64) AS "PARTITION_NAME",

	-- Direct subpartition name (if exists)
	fp.subpartition_name::varchar(64) AS "SUBPARTITION_NAME",

	-- Partition ordinal position (compatible with MySQL)
	CAST(ROW_NUMBER() OVER (
		PARTITION BY fp.root_oid
		ORDER BY fp.path
	) AS unsigned) AS "PARTITION_ORDINAL_POSITION",

	-- Subpartition ordinal position (compatible with MySQL)
	CAST(
	CASE
		WHEN fp.subpartition_name IS NOT NULL THEN
			ROW_NUMBER() OVER (
				PARTITION BY fp.root_oid, fp.partition_oid
				ORDER BY fp.subpartition_name
			)
		ELSE NULL
	END AS unsigned) AS "SUBPARTITION_ORDINAL_POSITION",

	-- Partitioning method - use correct inheritance chain
	CASE
		-- For leaf partitions that are not themselves partitioned, use parent's method
		WHEN fp.current_partition_method IS NULL AND fp.parent_partition_method IS NOT NULL
			THEN fp.parent_partition_method
		-- Otherwise use current or root method
		ELSE COALESCE(fp.current_partition_method, fp.root_partition_method)
	END::varchar(18) AS "PARTITION_METHOD",

	-- Subpartitioning method - from parent partition
	fp.subpartition_method::varchar(12) AS "SUBPARTITION_METHOD",

	-- Expression defining the partition key - use correct inheritance chain
	CASE
		-- For leaf partitions that are not themselves partitioned, use parent's expression
		WHEN fp.current_partition_method IS NULL AND fp.parent_partition_expression IS NOT NULL
			THEN fp.parent_partition_expression
		-- For partitions that are themselves partitioned tables
		WHEN fp.current_partition_method IS NOT NULL THEN fp.current_partition_expression
		-- For leaf partitions, use root partition expression
		ELSE fp.root_partition_expression
	END::longtext AS "PARTITION_EXPRESSION",

	-- Subpartition expression - from parent partition
	fp.subpartition_expression::longtext AS "SUBPARTITION_EXPRESSION",

	-- Description of partition bounds
	CASE
		WHEN fp.is_root THEN NULL  -- Root table has no partition bounds
		ELSE pg_catalog.pg_get_expr(c.relpartbound, c.oid)
	END::longtext AS "PARTITION_DESCRIPTION",

	-- Estimated number of rows
	CAST(GREATEST(c.reltuples, 0)::bigint AS unsigned) AS "TABLE_ROWS",

	-- Average row length (bytes per row, estimated)
	CAST(
		CASE
			WHEN c.reltuples > 0
				THEN (c.relpages * current_setting('block_size')::integer / c.reltuples)
			ELSE NULL
		END::bigint AS unsigned) AS "AVG_ROW_LENGTH",

	-- Total size of partition (data + TOAST + FSM + VM)
	CAST(pg_total_relation_size(c.oid) AS unsigned) AS "DATA_LENGTH",

	-- Maximum data length (unsupported, always NULL)
		CAST(NULL AS unsigned) AS "MAX_DATA_LENGTH",

	-- Index size - compatible with MySQL INDEX_LENGTH
	CAST(pg_indexes_size(c.oid) AS unsigned) AS "INDEX_LENGTH",

	-- Free space (unsupported, always NULL)
	CAST(NULL AS unsigned) AS "DATA_FREE",

	-- Creation time (from pg_object)
	po.created::mysdatetime AS "CREATE_TIME",

	-- Last modification time (from pg_object)
	po.modified::mysdatetime AS "UPDATE_TIME",

	-- Check time (unsupported, always NULL)
	NULL::mysdatetime AS "CHECK_TIME",

	-- Checksum (unsupported, always NULL)
	CAST(NULL AS unsigned) AS "CHECKSUM",

	-- Partition comment
	COALESCE(
		(SELECT description FROM pg_description
		 WHERE objoid = c.oid AND classoid = 'pg_class'::regclass LIMIT 1),
		''
	)::varchar(80) AS "PARTITION_COMMENT",

	-- Node group (always 'default')
	'default'::varchar(12) AS "NODEGROUP",

	-- Tablespace name
	ts.spcname::varchar(64) AS "TABLESPACE_NAME"

FROM filtered_partitions fp
-- Current partition details
JOIN pg_class c ON c.oid = fp.partition_oid
-- Tablespace information
LEFT JOIN pg_tablespace ts ON c.reltablespace = ts.oid
-- Created time and update time
LEFT JOIN pg_catalog.pg_object po ON po.obj_oid = c.oid

ORDER BY fp.root_schema, fp.root_name, fp.path, fp.subpartition_name;


GRANT SELECT ON mys_information_schema.partitions TO PUBLIC;

--
-- View: mys_information_schema.column_privileges
--

CREATE OR REPLACE VIEW mys_information_schema.column_privileges AS
SELECT
	-- Name of the user to whom the privilege is granted
	concat('''', grantee.rolname, '''@''localhost''')::varchar(81) AS "GRANTEE",

	-- Database in which the table resides (always 'def')
	'def'::varchar(512) AS "TABLE_CATALOG",

	-- Name of the schema containing the table
	nc.nspname::varchar(64) AS "TABLE_SCHEMA",

	-- Name of the table containing the column
	x.relname::varchar(64) AS "TABLE_NAME",

	-- Name of the column on which the privilege is granted
	x.attname::varchar(64) AS "COLUMN_NAME",

	-- Type of privilege (e.g., SELECT, INSERT, UPDATE, REFERENCES)
	x.prtype::varchar(64) AS "PRIVILEGE_TYPE",

	-- Whether the privilege is grantable (YES or NO)
	CASE
		WHEN pg_has_role(x.grantee, x.relowner, 'USAGE') OR x.grantable THEN 'YES'
		ELSE 'NO'
	END::varchar(3) AS "IS_GRANTABLE"

FROM (
	-- Privileges inherited from relation-level ACLs
	SELECT
		pr_c.grantee,
		a.attname,
		pr_c.relname,
		pr_c.relnamespace,
		pr_c.prtype,
		pr_c.grantable,
		pr_c.relowner
	FROM (
		SELECT
			c.oid,
			c.relname,
			c.relnamespace,
			c.relowner,
			(aclexplode(COALESCE(c.relacl, acldefault('r', c.relowner)))).grantee,
			(aclexplode(COALESCE(c.relacl, acldefault('r', c.relowner)))).privilege_type AS prtype,
			(aclexplode(COALESCE(c.relacl, acldefault('r', c.relowner)))).is_grantable AS grantable
		FROM pg_class c
		WHERE c.relkind IN ('r', 'v', 'f', 'p')
	) pr_c
	JOIN pg_attribute a ON a.attrelid = pr_c.oid AND a.attnum > 0 AND NOT a.attisdropped

	UNION

	-- Privileges granted directly to columns (column-level ACLs)
	SELECT
		pr_a.grantee,
		pr_a.attname,
		c.relname,
		c.relnamespace,
		pr_a.prtype,
		pr_a.grantable,
		c.relowner
	FROM (
		SELECT
			a.attrelid,
			a.attname,
			(aclexplode(COALESCE(a.attacl, acldefault('c', c.relowner)))).grantee,
			(aclexplode(COALESCE(a.attacl, acldefault('c', c.relowner)))).privilege_type AS prtype,
			(aclexplode(COALESCE(a.attacl, acldefault('c', c.relowner)))).is_grantable AS grantable
		FROM pg_attribute a
		JOIN pg_class c ON a.attrelid = c.oid
		WHERE a.attnum > 0 AND NOT a.attisdropped
	) pr_a
	JOIN pg_class c ON pr_a.attrelid = c.oid AND c.relkind IN ('r', 'v', 'f', 'p')
) x
JOIN pg_namespace nc ON x.relnamespace = nc.oid

-- Map grantee OIDs to user names (including PUBLIC pseudo-role)
JOIN (
	SELECT oid, rolname FROM pg_authid
	UNION ALL
	SELECT 0::oid AS oid, 'PUBLIC'::name
) grantee ON x.grantee = grantee.oid

-- Filter to MySQL-compatible privilege types
WHERE x.prtype IN ('INSERT', 'SELECT', 'UPDATE', 'REFERENCES')
	AND (pg_has_role(grantee.oid, 'USAGE') OR grantee.rolname = 'PUBLIC');

GRANT SELECT ON mys_information_schema.column_privileges TO PUBLIC;

--
-- View: mys_information_schema.columns
--

CREATE OR REPLACE VIEW mys_information_schema.columns AS
SELECT
	'def'::varchar(512) AS "TABLE_CATALOG",                                    -- Name of the database catalog (def)
	nc.nspname::varchar(64) AS "TABLE_SCHEMA",                                 -- Name of the schema containing the table
	c.relname::varchar(64) AS "TABLE_NAME",                                    -- Table name
	a.attname::varchar(64) AS "COLUMN_NAME",                                   -- Column name
	CAST(a.attnum AS unsigned) AS "ORDINAL_POSITION",                          -- Position of column in table
	pg_get_expr(ad.adbin, ad.adrelid)::longtext AS "COLUMN_DEFAULT",           -- Column default value, NULL if explicitly DEFAULT NULL or no default defined
	CASE WHEN a.attnotnull THEN 'NO' ELSE 'YES' END::varchar(3) AS "IS_NULLABLE",  -- Whether column allows NULL

	-- DATA_TYPE logic compatible with MySQL conventions
	CASE
		WHEN t.typtype = 'd' THEN
			t.typname    /* Bug - #z5966 #z5964 */
		ELSE
			format_type(a.atttypid, NULL)
	END::varchar(64) AS "DATA_TYPE",

	-- Max character length for character types
	CAST(information_schema._pg_char_max_length(
		information_schema._pg_truetypid(a.*, t.*),
		information_schema._pg_truetypmod(a.*, t.*)
	) AS unsigned) AS "CHARACTER_MAXIMUM_LENGTH",

	-- Max byte length
	CAST(information_schema._pg_char_octet_length(
		information_schema._pg_truetypid(a.*, t.*),
		information_schema._pg_truetypmod(a.*, t.*)
	) AS unsigned) AS "CHARACTER_OCTET_LENGTH",

	-- Precision for numeric types
	CAST(information_schema._pg_numeric_precision(
		information_schema._pg_truetypid(a.*, t.*),
		information_schema._pg_truetypmod(a.*, t.*)
	) AS unsigned) AS "NUMERIC_PRECISION",

	-- Scale for numeric types
	CAST(information_schema._pg_numeric_scale(
		information_schema._pg_truetypid(a.*, t.*),
		information_schema._pg_truetypmod(a.*, t.*)
		) AS unsigned) AS "NUMERIC_SCALE",

	-- Precision for datetime types
	CAST(information_schema._pg_datetime_precision(
		information_schema._pg_truetypid(a.*, t.*),
		information_schema._pg_truetypmod(a.*, t.*)
		) AS unsigned) AS "DATETIME_PRECISION",

	-- Character set name
	pg_catalog.getdatabaseencoding()::varchar(32) AS "CHARACTER_SET_NAME",

	-- Collation name
	CASE WHEN co.collname IS NOT NULL THEN co.collname ELSE NULL END::varchar(32) AS "COLLATION_NAME",

	-- COLUMN_TYPE: complete type definition
	format_type(a.atttypid, a.atttypmod)::longtext AS "COLUMN_TYPE",

	-- COLUMN_KEY: PRI, UNI, MUL
	CASE
		WHEN pk.conname IS NOT NULL THEN 'PRI'
		WHEN uq.conname IS NOT NULL THEN 'UNI'
		WHEN idx.indexrelid IS NOT NULL AND idx.indisunique THEN 'UNI' /* BUG#Z6767 */
		WHEN idx.indexrelid IS NOT NULL THEN 'MUL'
		ELSE NULL
	END::varchar(3) AS "COLUMN_KEY",

	-- EXTRA: auto_increment, generated, etc.
	trim(
		both ' ' from (concat(
		CASE WHEN a.attidentity IN ('a','d','m') THEN 'auto_increment ' ELSE '' END,
		CASE WHEN a.attgenerated <> '' THEN 'STORED GENERATED ' ELSE '' END)
		)
		)::varchar(30) AS "EXTRA",

	-- PRIVILEGES: user privileges on this column
	(SELECT string_agg("PRIVILEGE_TYPE", ',' ORDER BY "PRIVILEGE_TYPE")
		FROM mys_information_schema.column_privileges cp
		WHERE cp."TABLE_SCHEMA" = nc.nspname
		AND cp."TABLE_NAME" = c.relname
		AND cp."COLUMN_NAME" = a.attname
		AND cp."GRANTEE" = concat(quote_literal(current_user), '@', quote_literal('localhost'))
	)::varchar(80) AS "PRIVILEGES",

	-- COLUMN_COMMENT: column-level comment
	col_description(a.attrelid, a.attnum)::varchar(1024) AS "COLUMN_COMMENT",

	-- GENERATION_EXPRESSION: if generated column
	CASE WHEN a.attgenerated <> '' THEN pg_get_expr(ad.adbin, ad.adrelid) ELSE NULL END::longtext AS "GENERATION_EXPRESSION"

FROM pg_attribute a
JOIN pg_class c ON a.attrelid = c.oid AND c.relpersistence != 't' /* Bug - #z5986 */
JOIN pg_namespace nc ON c.relnamespace = nc.oid
JOIN pg_type t ON a.atttypid = t.oid
LEFT JOIN pg_type bt ON t.typtype='d' AND t.typbasetype=bt.oid
LEFT JOIN pg_namespace nbt ON bt.typnamespace=nbt.oid
LEFT JOIN pg_namespace nt ON t.typnamespace=nt.oid
LEFT JOIN pg_attrdef ad ON a.attrelid=ad.adrelid AND a.attnum=ad.adnum
LEFT JOIN pg_collation co ON a.attcollation=co.oid AND (co.collname<>'default' OR co.collnamespace<>(SELECT oid FROM pg_namespace WHERE nspname='pg_catalog'))
-- PRIMARY KEY info
LEFT JOIN (
	SELECT conrelid, unnest(conkey) AS colnum, conname
	FROM pg_constraint WHERE contype='p'
) pk ON pk.conrelid=c.oid AND pk.colnum=a.attnum
-- UNIQUE KEY info
LEFT JOIN (
	SELECT conrelid, unnest(conkey) AS colnum, conname
	FROM pg_constraint WHERE contype='u'
) uq ON uq.conrelid=c.oid AND uq.colnum=a.attnum
-- MUL index info
LEFT JOIN LATERAL (
	SELECT i.indexrelid, i.indisunique /* BUG#Z6767 */
	FROM pg_index i
	WHERE i.indrelid=c.oid AND a.attnum = any(i.indkey) /* BUG#Z6281*/
	LIMIT 1
) idx ON true

WHERE a.attnum > 0
	AND NOT a.attisdropped
	AND c.relkind IN ('r','p','v','f')  -- Only regular, partitioned, view, foreign tables
	AND NOT pg_is_other_temp_schema(nc.oid)
	AND (pg_has_role(c.relowner, 'USAGE') OR has_column_privilege(c.oid, a.attnum, 'SELECT, INSERT, UPDATE, REFERENCES'))
	ORDER BY "ORDINAL_POSITION" ASC; /* Bug - #z5984 #z5985 */

GRANT SELECT ON mys_information_schema.columns TO PUBLIC;

--
-- View: mys_information_schema.key_column_usage
--

CREATE OR REPLACE VIEW mys_information_schema.key_column_usage AS
SELECT
	-- Name of the catalog containing the constraint (always current database)
	'def'::varchar(512) AS "CONSTRAINT_CATALOG",

	-- Name of the schema containing the constraint
	ss.nc_nspname::varchar(64) AS "CONSTRAINT_SCHEMA",

	-- Name of the constraint (if primary key, value is `PRIMARY`)
	CASE
		WHEN ss.contype = 'p'::"char" THEN 'PRIMARY'
		ELSE ss.conname
	END::varchar(64) AS "CONSTRAINT_NAME",

	-- Name of the catalog containing the table (always current database)
	'def'::varchar(512) AS "TABLE_CATALOG",

	-- Name of the schema containing the table
	ss.nr_nspname::varchar(64) AS "TABLE_SCHEMA",

	-- Name of the table that the constraint applies to
	ss.relname::varchar(64) AS "TABLE_NAME",

	-- Name of the column involved in the constraint
	a.attname::varchar(64) AS "COLUMN_NAME",

	-- Column`s position within the constraint (1-based)
	(ss.x).n::bigint(10) AS "ORDINAL_POSITION",

	-- For foreign keys, the column`s ordinal position in the referenced unique constraint; NULL otherwise
	CASE
		WHEN ss.contype = 'f'::"char" THEN
			information_schema._pg_index_position(ss.conindid, ss.confkey[(ss.x).n])
		ELSE NULL::integer
	END::bigint(10) AS "POSITION_IN_UNIQUE_CONSTRAINT",

	-- Schema name of the referenced table for foreign keys; NULL otherwise
	CASE
		WHEN ss.contype = 'f'::"char" THEN
			(SELECT nspname FROM pg_namespace WHERE oid = ss.confrelid_nsp)
		ELSE NULL
	END::varchar(64) AS "REFERENCED_TABLE_SCHEMA",

	-- Name of the referenced table for foreign keys; NULL otherwise
	CASE
		WHEN ss.contype = 'f'::"char" THEN
			(SELECT relname FROM pg_class WHERE oid = ss.confrelid)
		ELSE NULL
	END::varchar(64) AS "REFERENCED_TABLE_NAME",

	-- Name of the referenced column for foreign keys; NULL otherwise
	CASE
		WHEN ss.contype = 'f'::"char" THEN
			(SELECT attname FROM pg_attribute
			WHERE attrelid = ss.confrelid AND attnum = ss.confkey[(ss.x).n])
		ELSE NULL
	END::varchar(64) AS "REFERENCED_COLUMN_NAME"

FROM
	pg_attribute a
JOIN (
	-- Extract constraint and related metadata
	SELECT
		r.oid AS roid,
		r.relname,
		r.relowner,
		nc.nspname AS nc_nspname,
		nr.nspname AS nr_nspname,
		c.oid AS coid,
		c.conname,
		c.contype,
		c.conindid,
		c.confkey,
		c.confrelid,

		-- Namespace of the referenced table
		(SELECT relnamespace FROM pg_class WHERE oid = c.confrelid) AS confrelid_nsp,

		-- Expand the constraint key array to get column indexes
		information_schema._pg_expandarray(c.conkey) AS x
	FROM
		pg_namespace nr,
		pg_class r,
		pg_namespace nc,
		pg_constraint c
	WHERE
		nr.oid = r.relnamespace
		AND r.oid = c.conrelid
		AND nc.oid = c.connamespace
		AND c.contype = ANY (ARRAY['p'::"char", 'u'::"char", 'f'::"char"])  -- Primary, Unique, Foreign
		AND r.relkind = ANY (ARRAY['r'::"char", 'p'::"char"])              -- Regular or partitioned tables
		AND NOT pg_is_other_temp_schema(nr.oid)
) ss ON ss.roid = a.attrelid AND a.attnum = (ss.x).x AND NOT a.attisdropped

-- Filter rows based on role usage or column privileges
WHERE
	pg_has_role(ss.relowner, 'USAGE'::text)
	OR has_column_privilege(ss.roid, a.attnum, 'SELECT, INSERT, UPDATE, REFERENCES'::text);

GRANT SELECT ON mys_information_schema.key_column_usage TO PUBLIC;

--
-- View: mys_information_schema.views
--

CREATE OR REPLACE VIEW mys_information_schema.views AS
SELECT
	-- Name of the catalog (always the current database)
	'def'::varchar(512) AS "TABLE_CATALOG",

	-- Name of the schema containing the view
	nc.nspname::varchar(64) AS "TABLE_SCHEMA",

	-- Name of the view
	c.relname::varchar(64) AS "TABLE_NAME",

	-- SQL definition of the view (NULL if no privilege)
	CASE
		WHEN pg_has_role(c.relowner, 'USAGE'::text)
			 OR has_table_privilege(c.oid, 'SELECT'::text) THEN pg_get_viewdef(c.oid, true)
		ELSE NULL::text
	END::longtext AS "VIEW_DEFINITION",

	-- Check option: CASCADED, LOCAL, or NONE
	'NONE'::varchar(8) AS "CHECK_OPTION",

	-- Indicates if the view is updatable (YES or NO)
	CASE
		WHEN (pg_relation_is_updatable(c.oid::regclass, false) & 20) = 20 THEN 'YES'::text
		ELSE 'NO'::text
	END::varchar(3) AS "IS_UPDATABLE",

	-- MySQL compatibility: view definer
	concat((pg_get_userbyid(c.relowner), '@', 'localhost'))::varchar(93) AS "DEFINER",

	-- MySQL compatibility: security type
	'DEFINER'::varchar(7) AS "SECURITY_TYPE",

	-- MySQL compatibility: character set used during creation
	pg_catalog.getdatabaseencoding()::varchar(32) AS "CHARACTER_SET_CLIENT",

	-- MySQL compatibility: collation used during creation
	d.datcollate::varchar(32) AS "COLLATION_CONNECTION"

FROM pg_namespace nc
JOIN pg_class c ON c.relnamespace = nc.oid
JOIN pg_database d ON d.datname = current_database()
WHERE c.relkind = 'v'::"char"
	AND NOT pg_is_other_temp_schema(nc.oid)
	AND (
		pg_has_role(c.relowner, 'USAGE'::text)
		OR has_table_privilege(c.oid, 'SELECT, INSERT, UPDATE, DELETE, TRUNCATE, REFERENCES, TRIGGER'::text)
		OR has_any_column_privilege(c.oid, 'SELECT, INSERT, UPDATE, REFERENCES'::text)
	);

GRANT SELECT ON mys_information_schema.views TO PUBLIC;

--
-- View: mys_information_schema.routines
--

CREATE OR REPLACE VIEW mys_information_schema.routines AS
SELECT
	-- Specific name of the routine (same as routine name in PostgreSQL)
	p.proname::varchar(64) AS "SPECIFIC_NAME",

	-- Catalog name (always 'def' for compatibility with MySQL)
	'def'::varchar(512) AS "ROUTINE_CATALOG",

	-- Schema (namespace) of the routine
	n.nspname::varchar(64) AS "ROUTINE_SCHEMA",

	-- Name of the routine
	p.proname::varchar(64) AS "ROUTINE_NAME",

	-- Type of routine: FUNCTION or PROCEDURE
	CASE p.prokind
		WHEN 'p' THEN 'PROCEDURE'
		WHEN 'f' THEN 'FUNCTION'
		ELSE NULL
	END::varchar(9) AS "ROUTINE_TYPE",

	-- Return type for functions; NULL for procedures
	CASE
		WHEN p.prokind = 'f' THEN format_type(p.prorettype, NULL)
		ELSE NULL
	END::varchar(64) AS "DATA_TYPE",

	-- Max character length (for string return types only)
	CASE
		WHEN p.prokind = 'f' AND t.typcategory = 'S' THEN NULLIF(t.typlen, -1)
		ELSE NULL
	END::integer AS "CHARACTER_MAXIMUM_LENGTH",

	-- Max octet length (same as above)
	CASE
		WHEN p.prokind = 'f' AND t.typcategory = 'S' THEN NULLIF(t.typlen, -1)
		ELSE NULL
	END::integer AS "CHARACTER_OCTET_LENGTH",

	-- Character set name of return type (if string)
	CASE
		WHEN p.prokind = 'f' AND t.typcategory = 'S' THEN pg_encoding_to_char(d.encoding)
		ELSE NULL
	END::varchar(64) AS "CHARACTER_SET_NAME",

	-- Collation of the return type (if applicable)
	CASE
		WHEN p.prokind = 'f' AND t.typcategory = 'S' THEN c.collname
		ELSE NULL
	END::varchar(64) AS "COLLATION_NAME",

	-- Numeric precision/scale/date precision (unsupported, set NULL)
	CAST(NULL AS unsigned) AS "NUMERIC_PRECISION",
	NULL::integer AS "NUMERIC_SCALE",
	CAST(NULL AS unsigned) AS "DATETIME_PRECISION",

	-- DTD identifier (return type full descriptor)
	CASE
		WHEN p.prokind = 'f' THEN format_type(p.prorettype, NULL)
		ELSE NULL
	END::longtext AS "DTD_IDENTIFIER",

	-- Always SQL body
	'SQL'::varchar(8) AS "ROUTINE_BODY",

	-- Routine definition (pg_get_functiondef)
	pg_get_functiondef(p.oid) AS "ROUTINE_DEFINITION",

	-- External references (not applicable in PostgreSQL)
	NULL::varchar(64) AS "EXTERNAL_NAME",
	NULL::varchar(64) AS "EXTERNAL_LANGUAGE",

	-- Always SQL parameter style
	'SQL'::varchar(8) AS "PARAMETER_STYLE",

	-- Whether function is deterministic (based on provolatile)
	CASE
		WHEN p.provolatile = 'i' THEN 'YES'
		ELSE 'NO'
	END::varchar(3) AS "IS_DETERMINISTIC",

	-- SQL data access
	'MODIFIES SQL DATA'::varchar(64) AS "SQL_DATA_ACCESS",

	-- SQL path (not tracked in PostgreSQL)
	NULL::varchar(64) AS "SQL_PATH",

	-- Security type (based on prosecdef)
	CASE
		WHEN p.prosecdef = TRUE THEN 'DEFINER'
		WHEN p.prosecdef = FALSE THEN 'INVOKER'
		ELSE NULL
	END::varchar(7) AS "SECURITY_TYPE",

	-- Creation time (from pg_object metadata if available)
	COALESCE(o.created::mysdatetime, 0::mysdatetime) AS "CREATED",

	-- Last altered time (fallback to created if missing)
	COALESCE(o.modified, o.created)::mysdatetime AS "LAST_ALTERED",

	-- SQL mode (default initial value)
	(
		SELECT reset_val FROM pg_settings WHERE name = 'sql_mode'
	)::varchar(8192) AS "SQL_MODE",

	-- Function comment or empty string
	COALESCE(obj_description(p.oid, 'pg_proc'), '')::longtext AS "ROUTINE_COMMENT",

	-- Definer (not available in PostgreSQL)
	concat(r.rolname, '@', 'localhost')::varchar(93) AS "DEFINER",

	-- Client character set (not available)
	NULL::varchar(32) AS "CHARACTER_SET_CLIENT",

	-- Connection collation (not available)
	NULL::varchar(32) AS "COLLATION_CONNECTION",

	-- Database default collation
	d.datcollate::varchar(32) AS "DATABASE_COLLATION"

FROM pg_proc p
JOIN pg_namespace n ON p.pronamespace = n.oid
JOIN pg_type t ON p.prorettype = t.oid
LEFT JOIN pg_collation c ON t.typcollation = c.oid
LEFT JOIN pg_database d ON d.datname = current_database()
LEFT JOIN pg_object o ON o.obj_oid = p.oid AND o.refclassid = 'pg_proc'::regclass
JOIN pg_roles r ON r.oid = p.proowner
WHERE n.nspname NOT IN ('pg_catalog', 'information_schema')
	AND p.prokind IN ('f', 'p');

GRANT SELECT ON mys_information_schema.routines TO PUBLIC;

--
-- View: mys_information_schema.triggers
--

CREATE OR REPLACE VIEW mys_information_schema.triggers AS
SELECT
	-- Name of the catalog (always 'def')
	'def'::varchar(512) AS "TRIGGER_CATALOG",

	-- Name of the schema containing the trigger
	n.nspname::varchar(64) AS "TRIGGER_SCHEMA",

	-- Name of the trigger
	t.tgname::varchar(64) AS "TRIGGER_NAME",

	-- Trigger event: INSERT, UPDATE, DELETE
	em.text::varchar(6) AS "EVENT_MANIPULATION",

	-- Name of the catalog containing the table on which the trigger acts
	'def'::varchar(512) AS "EVENT_OBJECT_CATALOG",

	-- Name of the schema containing the table on which the trigger acts
	n.nspname::varchar(64) AS "EVENT_OBJECT_SCHEMA",

	-- Name of the table on which the trigger acts
	c.relname::varchar(64) AS "EVENT_OBJECT_TABLE",

	-- Order in which the trigger action is executed (within event, table, timing, level)
	rank() OVER (
		PARTITION BY n.nspname, c.relname, em.num, (t.tgtype::integer & 1), (t.tgtype::integer & 66)
		ORDER BY t.tgname
	)::bigint(4) AS "ACTION_ORDER",

	-- Trigger condition (WHEN clause), NULL if no permission
	CASE
		WHEN pg_has_role(c.relowner, 'USAGE')
		THEN (regexp_match(pg_get_triggerdef(t.oid), '.{35,} WHEN \\((.+)\\) EXECUTE FUNCTION'))[1]
		ELSE NULL
	END::longtext AS "ACTION_CONDITION",

	-- Trigger action body (function call after EXECUTE FUNCTION)
	SUBSTRING(
		pg_get_triggerdef(t.oid)
		FROM POSITION(('EXECUTE FUNCTION'::text) IN (SUBSTRING(pg_get_triggerdef(t.oid) FROM 48))) OPERATOR(pg_catalog.+) 47
	)::longtext AS "ACTION_STATEMENT",

	-- Trigger orientation: ROW or STATEMENT
	CASE t.tgtype::integer & 1
		WHEN 1 THEN 'ROW'
		ELSE 'STATEMENT'
	END::varchar(9) AS "ACTION_ORIENTATION",

	-- Trigger timing: BEFORE, AFTER, INSTEAD OF
	CASE t.tgtype::integer & 66
		WHEN 2 THEN 'BEFORE'
		WHEN 64 THEN 'INSTEAD OF'
		ELSE 'AFTER'
	END::varchar(6) AS "ACTION_TIMING",

	-- Transition table for OLD rows
	t.tgoldtable::varchar(64) AS "ACTION_REFERENCE_OLD_TABLE",

	-- Transition table for NEW rows
	t.tgnewtable::varchar(64) AS "ACTION_REFERENCE_NEW_TABLE",

	-- Transition variable for OLD row
	'OLD'::varchar(3) AS "ACTION_REFERENCE_OLD_ROW",

	-- Transition variable for NEW row
	'NEW'::varchar(3) AS "ACTION_REFERENCE_NEW_ROW",

	-- Trigger creation timestamp (from pg_object)
	o.created::mysdatetimes(2) AS "CREATED",

	-- SQL mode (default initial value)
	(
		SELECT reset_val FROM pg_settings WHERE name = 'sql_mode'
	)::varchar(8192) AS "SQL_MODE",

	-- MySQL-specific: DEFINER, not tracked by default in PostgreSQL
	concat(pg_get_userbyid(c.relowner), '@', 'localhost')::varchar(93) AS "DEFINER",

	-- MySQL-specific: character set of client during creation
	NULL::varchar(32) AS "CHARACTER_SET_CLIENT",

	-- MySQL-specific: collation of connection during creation
	NULL::varchar(32) AS "COLLATION_CONNECTION",

	-- Default collation of the database
	(SELECT datcollate FROM pg_database WHERE datname = current_database())::varchar(32) AS "DATABASE_COLLATION"

FROM
	pg_namespace n
JOIN
	pg_class c ON c.relnamespace = n.oid
JOIN
	pg_trigger t ON t.tgrelid = c.oid
JOIN
	(VALUES (4, 'INSERT'), (8, 'DELETE'), (16, 'UPDATE')) em(num, text)
	  ON (t.tgtype & em.num) <> 0
LEFT JOIN
	pg_object o
	  ON o.obj_oid	   = t.oid
	 AND o.refclassid  = 'pg_trigger'::regclass::oid
	 AND o.schemaoid   = n.oid
	 AND o.objsubid   = 0
WHERE
	NOT t.tgisinternal
	AND NOT pg_is_other_temp_schema(n.oid)
	AND (
		pg_has_role(c.relowner, 'USAGE')
		OR has_table_privilege(c.oid, 'INSERT, UPDATE, DELETE, TRUNCATE, REFERENCES, TRIGGER')
		OR has_any_column_privilege(c.oid, 'INSERT, UPDATE, REFERENCES')
	);

GRANT SELECT ON mys_information_schema.triggers TO PUBLIC;

--
-- View: mys_information_schema.table_constraints
--

CREATE OR REPLACE VIEW mys_information_schema.table_constraints AS
SELECT
	-- Catalog name (always 'def')
	'def'::varchar(512) AS "CONSTRAINT_CATALOG",

	-- Schema that owns the constraint
	nc.nspname::varchar(64) AS "CONSTRAINT_SCHEMA",

	-- Constraint name
	c.conname::varchar(64) AS "CONSTRAINT_NAME",

	-- Catalog name for the table
	current_database()::varchar(64) AS "TABLE_CATALOG",

	-- Schema that owns the table
	nr.nspname::varchar(64) AS "TABLE_SCHEMA",

	-- Table name
	r.relname::varchar(64) AS "TABLE_NAME",

	-- Type of constraint (CHECK, FOREIGN KEY, PRIMARY KEY, UNIQUE)
	CASE c.contype
		WHEN 'c'::"char" THEN 'CHECK'::text
		WHEN 'f'::"char" THEN 'FOREIGN KEY'::text
		WHEN 'p'::"char" THEN 'PRIMARY KEY'::text
		WHEN 'u'::"char" THEN 'UNIQUE'::text
		ELSE NULL::text
	END::varchar(64) AS "CONSTRAINT_TYPE"
FROM
	pg_namespace nc,
	pg_namespace nr,
	pg_constraint c,
	pg_class r
WHERE
	nc.oid = c.connamespace
	AND nr.oid = r.relnamespace
	AND c.conrelid = r.oid
	AND (c.contype <> ALL (ARRAY['t'::"char", 'x'::"char"]))
	AND (r.relkind = ANY (ARRAY['r'::"char", 'p'::"char"]))
	AND NOT pg_is_other_temp_schema(nr.oid)
	AND (
		pg_has_role(r.relowner, 'USAGE'::text)
		OR has_table_privilege(r.oid, 'INSERT, UPDATE, DELETE, TRUNCATE, REFERENCES, TRIGGER'::text)
		OR has_any_column_privilege(r.oid, 'INSERT, UPDATE, REFERENCES'::text)
	)

UNION ALL

SELECT
	-- Catalog name (current database)
	'def'::varchar(512) AS "CONSTRAINT_CATALOG",

	-- Schema that owns the table
	nr.nspname::varchar(64) AS "CONSTRAINT_SCHEMA",

	-- Generated constraint name for NOT NULL constraints
	concat(nr.oid::text, '_', r.oid::text, '_', a.attnum::text, '_not_null')::varchar(64) AS "CONSTRAINT_NAME",

	-- Catalog name for the table
	current_database()::varchar(64) AS "TABLE_CATALOG",

	-- Schema that owns the table
	nr.nspname::varchar(64) AS "TABLE_SCHEMA",

	-- Table name
	r.relname::varchar(64) AS "TABLE_NAME",

	-- Constraint type fixed as CHECK for NOT NULL constraints
	'CHECK'::character varying::varchar(64) AS "CONSTRAINT_TYPE"
FROM
	pg_namespace nr,
	pg_class r,
	pg_attribute a
WHERE
	nr.oid = r.relnamespace
	AND r.oid = a.attrelid
	AND a.attnotnull
	AND a.attnum > 0
	AND NOT a.attisdropped
	AND (r.relkind = ANY (ARRAY['r'::"char", 'p'::"char"]))
	AND NOT pg_is_other_temp_schema(nr.oid)
	AND (
		pg_has_role(r.relowner, 'USAGE'::text)
		OR has_table_privilege(r.oid, 'INSERT, UPDATE, DELETE, TRUNCATE, REFERENCES, TRIGGER'::text)
		OR has_any_column_privilege(r.oid, 'INSERT, UPDATE, REFERENCES'::text)
	);

GRANT SELECT ON mys_information_schema.table_constraints TO PUBLIC;

--
-- View: - mys_information_schema.schemata
--

CREATE OR REPLACE VIEW mys_information_schema.schemata AS
SELECT
	-- Catalog name ('def')
	'def'::varchar(512) AS "CATALOG_NAME",

	-- Schema (namespace) name
	n.nspname::varchar(64) AS "SCHEMA_NAME",

	-- Default character set name (encoding of the current database)
	pg_encoding_to_char(d.encoding)::varchar(32) AS "DEFAULT_CHARACTER_SET_NAME",

	-- Default collation name of the current database
	d.datcollate::varchar(32) AS "DEFAULT_COLLATION_NAME",

	-- SQL path, always NULL in PostgreSQL (not supported)
	NULL::varchar(512) AS "SQL_PATH"
FROM
	pg_namespace n,
	pg_database d
WHERE
	d.datname = current_database();	/* Bug - #z6081 */ -- Limit to the current database only

GRANT SELECT ON mys_information_schema.schemata TO PUBLIC;

--
-- View: mys_information_schema.schema_privileges
--

CREATE OR REPLACE VIEW mys_information_schema.schema_privileges AS
SELECT
	-- Grantee in format 'rolename@localhost'
	concat('''', usr.rolname, '''@''localhost''')::varchar(81) AS "GRANTEE",

	-- Catalog name ('def')
	'def'::varchar(512) AS "TABLE_CATALOG",

	-- Schema name
	n.nspname::varchar(64) AS "TABLE_SCHEMA",

	-- Privilege type: CREATE or USAGE
	p.privilege_type::varchar(64) AS "PRIVILEGE_TYPE",

	-- Whether the privilege is grantable by the grantee
	CASE
		WHEN pg_has_role(usr.oid, n.nspowner, 'USAGE') THEN
			CASE WHEN has_schema_privilege(usr.oid, n.oid, concat(privilege_type, ' WITH GRANT OPTION')) THEN 'YES'
				 ELSE 'NO'
			END
		ELSE 'NO'
	END::varchar(3) AS "IS_GRANTABLE"
FROM
	pg_namespace n
JOIN
	pg_roles usr ON TRUE
JOIN (
	VALUES
		('CREATE'),
		('USAGE')
) AS p(privilege_type)
ON has_schema_privilege(usr.oid, n.oid, p.privilege_type)
WHERE
	LEFT(n.nspname , 3) <> 'pg_'
	AND n.nspname <> 'information_schema';

GRANT SELECT ON mys_information_schema.schema_privileges TO PUBLIC;

--
-- View: mys_information_schema.tablespaces
--

CREATE OR REPLACE VIEW mys_information_schema.tablespaces AS
SELECT
	''::varchar(64) AS "TABLESPACE_NAME",
	''::varchar(64) AS "ENGINE",
	NULL::varchar(64) AS "TABLESPACE_TYPE",
	NULL::varchar(64) AS "LOGFILE_GROUP_NAME",
	cast(0 AS unsigned) AS "EXTENT_SIZE",
	cast(0 AS unsigned) AS "AUTOEXTEND_SIZE",
	cast(0 AS unsigned) AS "MAXIMUM_SIZE",
	cast(0 AS unsigned) AS "NODEGROUP_ID",
	''::varchar(2048) AS "TABLESPACE_COMMENT"
LIMIT 0;

GRANT SELECT ON mys_information_schema.tablespaces TO PUBLIC;

--
-- View: mys_information_schema.statistics
--

CREATE OR REPLACE VIEW mys_information_schema.statistics AS
SELECT
	'def'::varchar(512) AS "TABLE_CATALOG",
	n.nspname::varchar(64) AS "TABLE_SCHEMA",							  -- Schema name of the table
	t.relname::varchar(64) AS "TABLE_NAME",								  -- Table name
	CASE WHEN i.indisunique THEN 0 ELSE 1 END::bigint(1) AS "NON_UNIQUE", -- Indicates if the index is non-unique (0 = unique, 1 = non-unique)
	n.nspname::varchar(64) AS "INDEX_SCHEMA",							  -- Schema of the index, usually same as table schema
	ci.relname::varchar(64) AS "INDEX_NAME",							  -- Index name
	k.n AS "SEQ_IN_INDEX",												  -- Column sequence number within the index (starting from 1)
	-- Begin - BUG#ZZ5974
	CASE WHEN a.attname IS NOT NULL THEN
		a.attname::varchar(64)							  -- Column name in the index; NULL if expression index
	ELSE
		CASE WHEN am.amname = 'gist' THEN
			mys_information_schema.mys_get_gistindex_column(i.indexrelid)
			-- Handle PostgreSQL native expression indexes like "left"(name, 100)
			WHEN pg_get_indexdef(i.indexrelid, k.n::integer, true) ~* '"?left"?\\s*\\(.*,\\s*\\d+\\s*\\)' THEN
				(regexp_match(pg_get_indexdef(i.indexrelid, k.n::integer, true), '"left"\\s*\\(\\s*([^,:\\s")]+)'))[1]::varchar(64)
		ELSE
			a.attname::varchar(64)
		END
	END AS "COLUMN_NAME",
	-- End - BUG#ZZ5974
	CASE (k.opt & 1) WHEN 1 THEN 'D' ELSE 'A' END::varchar(1) AS "COLLATION",
	CASE /* Bug - #z5988 */
		WHEN s.n_distinct IS NULL THEN 0
		WHEN s.n_distinct <= 0 THEN 0
		ELSE s.n_distinct
	END::bigint(21) AS "CARDINALITY",					  -- Estimated cardinality (number of distinct values) from pg_stats.n_distinct

	CASE
	  WHEN a.attname IS NOT NULL THEN NULL
	  ELSE
		CASE
		  -- Handle MySQL-compatible prefix indexes, format like (100::name)
		  WHEN pg_get_indexdef(ci.oid, k.n::integer, true) ~ '^\\(\\d+::name\\)$' THEN
			(regexp_match(pg_get_indexdef(ci.oid, k.n::integer, true), '^\\((\\d+)::name\\)$'))[1]::int
		  -- Handle PostgreSQL native expression indexes like "left"(name, 100)
		  WHEN pg_get_indexdef(ci.oid, k.n::integer, true) ~* '"?left"?\\s*\\(.*,\\s*\\d+\\s*\\)' THEN
			(regexp_match(pg_get_indexdef(ci.oid, k.n::integer, true), '"?left"?\\s*\\(.*,\\s*(\\d+)\\s*\\)'))[1]::int
		  ELSE NULL
		END
	END::bigint(3) AS "SUB_PART", 											   -- Prefix length extracted from index expression; NULL for normal columns

	NULL::varchar(10) AS "PACKED",											   -- MySQL-specific field, not supported in PostgreSQL, always NULL
	CASE WHEN a.attnotnull THEN 'NO' ELSE 'YES' END::varchar(3) AS "NULLABLE", -- Whether the column is nullable ('NO' if NOT NULL, else 'YES')
	am.amname::varchar(16) AS "INDEX_TYPE",									   -- Index method/type, e.g. 'btree', 'hash'
	NULL::varchar(16) AS "COMMENT", 										   -- Index column comment, not supported in PostgreSQL, always NULL
	d.description::varchar(1024) AS "INDEX_COMMENT"									   -- Index comment, not supported in PostgreSQL, always NULL

FROM
	pg_class t											  -- Table relation
JOIN pg_namespace n ON t.relnamespace = n.oid			  -- Table schema
JOIN pg_index i ON t.oid = i.indrelid AND t.relpersistence != 't'/* Bug - #z5988 */  -- Table-to-index relationship
JOIN pg_class ci ON ci.oid = i.indexrelid				  -- Index relation
JOIN pg_am am ON ci.relam = am.oid						  -- Index access method
JOIN LATERAL unnest(i.indkey, i.indoption) WITH ORDINALITY AS k(attnum, opt, n) ON true   -- Unnest index keys with order
LEFT JOIN pg_description d ON ci.oid = d.objoid
LEFT JOIN pg_attribute a ON a.attrelid = t.oid AND a.attnum = k.attnum	-- Index column attributes
LEFT JOIN pg_stats s ON s.schemaname = n.nspname AND s.tablename = t.relname AND s.attname = a.attname	-- Statistics for cardinality

WHERE
	t.relkind = 'r' OR t.relkind = 'p'					  -- Only ordinary tables --BUG#Z5991,add OR t.relkind = 'p'
	AND NOT i.indisprimary; 							  -- Exclude primary key indexes (MySQL does not show these in statistics)

GRANT SELECT ON mys_information_schema.statistics TO PUBLIC;

--
-- View: mys_information_schema.collations
--

CREATE OR REPLACE VIEW mys_information_schema.collations AS
SELECT
	"Collation"::varchar(32)            AS "COLLATION_NAME",
	"Charset"::varchar(32)              AS "CHARACTER_SET_NAME",
	"Id"::bigint(11)                    AS "ID",
	"Default"::varchar(3)               AS "IS_DEFAULT",
	"Compiled"::varchar(3)              AS "IS_COMPILED",
	"Sortlen"::bigint(3)                AS "SORTLEN"
FROM show_mysql_collations();

GRANT SELECT ON mys_information_schema.collations TO PUBLIC;

-- View: mys_information_schema.character_sets

CREATE OR REPLACE VIEW mys_information_schema.character_sets AS
SELECT
	"Charset"::varchar(32)              AS "CHARACTER_SET_NAME",
	"Default collation"::varchar(32)    AS "DEFAULT_COLLATE_NAME",
	"Description"::varchar(60)          AS "DESCRIPTION",
	"Maxlen"::bigint(3)                 AS "MAXLEN"
FROM show_mysql_charsets();

GRANT SELECT ON mys_information_schema.character_sets TO PUBLIC;

-- View: mys_information_schema.processlist

CREATE OR REPLACE VIEW mys_information_schema.processlist AS
SELECT
	CAST(p.pid AS unsigned) AS "ID",

	-- User field handling, distinguish system users
	CASE
		WHEN p.usename IS NULL THEN 'system user'
		ELSE p.usename::text
	END::varchar(32) AS "USER",

	-- Host information
	CASE
		WHEN p.client_addr IS NOT NULL THEN
			COALESCE(p.client_hostname, p.client_addr::text) || ':' || p.client_port::text
		WHEN p.usename IS NULL THEN ''  -- System users have no host
		ELSE 'localhost'
	END::varchar(64) AS "HOST",

	-- Database
	NULL::varchar(64) AS "DB",

	-- More precise command type mapping
	CASE
		WHEN p.backend_type != 'client backend' THEN 'Daemon'
		WHEN p.state = 'active' THEN 'Query'
		WHEN p.state = 'idle' THEN 'Sleep'
		WHEN p.state = 'idle in transaction' THEN 'Sleep'
		WHEN p.state = 'idle in transaction (aborted)' THEN 'Sleep'
		WHEN p.state = 'fastpath function call' THEN 'Query'
		ELSE 'Daemon'
	END::varchar(16) AS "COMMAND",

	-- Time calculation
	CASE
		WHEN p.state = 'active' AND p.backend_type = 'client backend' THEN
			EXTRACT(EPOCH FROM (now() - p.query_start))::integer
		WHEN p.state LIKE 'idle%' AND p.backend_type = 'client backend' THEN
			EXTRACT(EPOCH FROM (now() - p.state_change))::integer
		ELSE
			0::integer
			--EXTRACT(EPOCH FROM (now() - p.backend_start))::integer
	END::int(7) AS "TIME",

	-- State
	p.state::varchar(64) AS "STATE",

	-- Query information
	CASE
		WHEN p.state = 'active' THEN p.query
		ELSE NULL
	END::longtext AS "INFO"
FROM
	pg_stat_activity p
ORDER BY
	CASE WHEN p.pid = pg_backend_pid() THEN 0 ELSE 1 END,
	p.query_start DESC NULLS LAST;

GRANT SELECT ON mys_information_schema.processlist TO PUBLIC;


CREATE VIEW mys_information_schema.global_variables AS SELECT name AS "VARIABLE_NAME", setting AS "VARIABLE_VALUE" FROM pg_settings;
GRANT SELECT ON mys_information_schema.global_variables TO PUBLIC;

CREATE VIEW mys_information_schema.session_variables AS SELECT name AS "VARIABLE_NAME", setting AS "VARIABLE_VALUE" FROM pg_settings;
GRANT SELECT ON mys_information_schema.session_variables TO PUBLIC;

/* Begin - #Z5936 */
-- Create the `mysql` schema if it does not exist
CREATE SCHEMA IF NOT EXISTS mysql;
CREATE type mysql.enum_mysql_user as enum('N','Y');
CREATE type mysql.enum_mysql_user_ssl_type as enum('','ANY','X509','SPECIFIED');

-- Create or replace the `mysql.user` view to simulate MySQL 5.7 `mysql.user` table
CREATE OR REPLACE VIEW mysql.user AS
SELECT
    -- Host column in MySQL; using '%' as default since PostgreSQL has no host concept
    '%'::char(60) AS "Host",

    -- User name (PostgreSQL role name)
    r.rolname::char(32) AS "User",

    -- ===== Simulated privileges (Y/N) =====
    -- SELECT privilege: not mapped in this simplified view, default 'N'
    'N'::mysql.enum_mysql_user AS "Select_priv",

    -- Table-level privileges: not mapped, default 'N'
    'N'::mysql.enum_mysql_user AS "Insert_priv",
    'N'::mysql.enum_mysql_user AS "Update_priv",
    'N'::mysql.enum_mysql_user AS "Delete_priv",

    -- CREATE privilege: not mapped, default 'N'
    'N'::mysql.enum_mysql_user AS "Create_priv",

    -- DROP privilege: not available at global level, default 'N'
    'N'::mysql.enum_mysql_user AS "Drop_priv",

    -- Administrative privileges not supported in PostgreSQL
    'N'::mysql.enum_mysql_user AS "Reload_priv",
    'N'::mysql.enum_mysql_user AS "Shutdown_priv",
    'N'::mysql.enum_mysql_user AS "Process_priv",
    'N'::mysql.enum_mysql_user AS "File_priv",

    -- GRANT privilege: not mapped, default 'N'
    'N'::mysql.enum_mysql_user AS "Grant_priv",

    -- Other object-level privileges: default 'N'
    'N'::mysql.enum_mysql_user AS "References_priv",
    'N'::mysql.enum_mysql_user AS "Index_priv",
    'N'::mysql.enum_mysql_user AS "Alter_priv",
    'N'::mysql.enum_mysql_user AS "Show_db_priv",

    -- SUPER privilege: corresponds to PostgreSQL superuser
    CASE WHEN r.rolsuper THEN 'Y' ELSE 'N' END AS "Super_priv",

    -- Temporary and table-level privileges: default 'N'
    'N'::mysql.enum_mysql_user AS "Create_tmp_table_priv",
    'N'::mysql.enum_mysql_user AS "Lock_tables_priv",
    'N'::mysql.enum_mysql_user AS "Execute_priv",
    'N'::mysql.enum_mysql_user AS "Repl_slave_priv",
    'N'::mysql.enum_mysql_user AS "Repl_client_priv",
    'N'::mysql.enum_mysql_user AS "Create_view_priv",
    'N'::mysql.enum_mysql_user AS "Show_view_priv",
    'N'::mysql.enum_mysql_user AS "Create_routine_priv",
    'N'::mysql.enum_mysql_user AS "Alter_routine_priv",
    'N'::mysql.enum_mysql_user AS "Create_user_priv",
    'N'::mysql.enum_mysql_user AS "Event_priv",
    'N'::mysql.enum_mysql_user AS "Trigger_priv",

    -- CREATE TABLESPACE privilege: PostgreSQL 14 does not have equivalent field
    'N'::mysql.enum_mysql_user AS "Create_tablespace_priv",

    -- ===== SSL-related fields (PostgreSQL does not track SSL per role) =====
    ''::mysql.enum_mysql_user_ssl_type AS "ssl_type",
    --''::blob AS "ssl_cipher",
    --''::blob AS "x509_issuer",
    --''::text AS "x509_subject",

    -- ===== Resource limits =====
    0::integer AS "max_questions",    -- MySQL-specific, not available in PostgreSQL
    0::integer AS "max_updates",      -- MySQL-specific, not available in PostgreSQL
    0::integer AS "max_connections",  -- MySQL-specific, not available in PostgreSQL

    -- Max concurrent connections for this user; mapped from PostgreSQL rolconnlimit
    CASE WHEN r.rolconnlimit > 0 THEN r.rolconnlimit ELSE 0 END AS "max_user_connections",

    -- ===== Plugin and authentication =====
    ''::text AS plugin,  -- MySQL authentication plugin; PostgreSQL does not have equivalent
    COALESCE(r.rolpassword::text, '') AS "authentication_string",  -- Role password

    -- ===== Password and account status =====
    -- password_expired: 'Y' if rolvaliduntil < now(), else 'N'
    CASE
        WHEN r.rolvaliduntil IS NULL THEN 'N'
        WHEN r.rolvaliduntil::pg_catalog.timestamp < now()::pg_catalog.timestamp THEN 'Y'
        ELSE 'N'
    END AS "password_expired",

    -- PostgreSQL does not track last password change; using NULL
    NULL::timestamp with time zone AS "password_last_changed",

    -- password_lifetime: remaining days until rolvaliduntil; NULL if no expiry
    CASE
        WHEN r.rolvaliduntil IS NULL THEN NULL
        ELSE GREATEST(0, FLOOR(EXTRACT(EPOCH FROM (r.rolvaliduntil::pg_catalog.timestamp - now()::pg_catalog.timestamp)) / 86400))
    END AS "password_lifetime",

    -- account_locked: 'Y' if rolcanlogin is FALSE, else 'N'
    CASE WHEN r.rolcanlogin THEN 'N' ELSE 'Y' END AS "account_locked"

FROM pg_authid r;

/* End - #Z5936 */

/* Begin - SRS-MYSQL-PROTOCOL */
--
-- View: mys_information_schema.plugins
--
CREATE TABLE mys_information_schema.plugins (
	"PLUGIN_NAME" 				varchar(64) DEFAULT '',
	"PLUGIN_VERSION" 			varchar(20) DEFAULT '',
	"PLUGIN_STATUS" 			varchar(10) DEFAULT '',
	"PLUGIN_TYPE"				varchar(80) DEFAULT '',
	"PLUGIN_TYPE_VERSION"		varchar(20) DEFAULT '',
	"PLUGIN_LIBRARY"			varchar(64),
	"PLUGIN_LIBRARY_VERSION"	varchar(20),
	"PLUGIN_AUTHOR"				varchar(64),
	"PLUGIN_DESCRIPTION"		longtext,
	"PLUGIN_LICENSE"			varchar(80),
	"LOAD_OPTION"				varchar(64)  DEFAULT ''
);

/* Begin - bug#6322 */
GRANT SELECT ON mys_information_schema.plugins TO PUBLIC;
/* End - bug#6322 */

INSERT INTO mys_information_schema.plugins VALUES ('mysql_native_password', '1.1', 'ACTIVE', 'AUTHENTICATION', '1.1', NULL, NULL, '', '', 'GPL', '');
INSERT INTO mys_information_schema.plugins VALUES ('caching_sha2_password', '1.1', 'ACTIVE', 'AUTHENTICATION', '1.1', NULL, NULL, '', '', 'GPL', '');

--
-- View: mys_information_schema.engines
--
CREATE TABLE mys_information_schema.engines (
	"ENGINE" 			varchar(64) DEFAULT '',
	"SUPPORT" 			varchar(8) DEFAULT '',
	"COMMENT" 			varchar(80) DEFAULT '',
	"TRANSACTIONS"		varchar(3),
	"XA"				varchar(3),
	"SAVEPOINTS"		varchar(3)
);

/* Begin - bug#6322 */
GRANT SELECT ON mys_information_schema.engines TO PUBLIC;
/* End - bug#6322 */

INSERT INTO mys_information_schema.engines VALUES ('PERFORMANCE_SCHEMA', 'YES', 'Performance Schema', 'NO', 'NO', 'NO');
/* End - SRS-MYSQL-PROTOCOL */

/* Begin - #Z5943 */

--
-- View: mys_information_schema.events
--

CREATE OR REPLACE VIEW mys_information_schema.events AS
SELECT
	''::varchar(64) AS "EVENT_CATALOG",
	''::varchar(64) AS "EVENT_SCHEMA",
	''::varchar(64) AS "EVENT_NAME",
	''::varchar(93) AS "DEFINER",
	''::varchar(64) AS "TIME_ZONE",
	''::varchar(8) AS "EVENT_BODY",
	NULL::longtext AS "EVENT_DEFINITION",
	''::varchar(9) AS "EVENT_TYPE",
	NULL::mysdatetime AS "EXECUTE_AT",
	NULL::varchar(256) AS "INTERVAL_VALUE",
	NULL::varchar(18) AS "INTERVAL_FIELD",
	''::varchar(8192) AS "SQL_MODE",
	NULL::mysdatetime AS "STARTS",
	NULL::mysdatetime AS "ENDS",
	''::varchar(18) AS "STATUS",
	''::varchar(12) AS "ON_COMPLETION",
	0::mysdatetime AS "CREATED",
	0::mysdatetime AS "LAST_ALTERED",
	NULL::mysdatetime AS "LAST_EXECUTED",
	''::varchar(64) AS "EVENT_COMMENT",
	0::bigint(10) AS "ORIGINATOR",
	''::varchar(32) AS "CHARACTER_SET_CLIENT",
	''::varchar(32) AS "COLLATION_CONNECTION",
	''::varchar(32) AS "DATABASE_COLLATION"
LIMIT 0;

GRANT SELECT ON mys_information_schema.events TO PUBLIC;

--
-- View: mys_information_schema.keywords
--
CREATE OR REPLACE VIEW mys_information_schema.keywords AS
SELECT	
	-- keyword text(uppercase)
	upper(word)::varchar(128) AS "WORD",

	-- reserved flag
	(catcode = 'R')::int AS "RESERVED"

FROM pg_get_keywords();

GRANT SELECT ON mys_information_schema.keywords TO PUBLIC;

--
-- View: mys_information_schema.referential_constraints
--

CREATE OR REPLACE VIEW mys_information_schema.referential_constraints AS
SELECT
	-- Catalog of the foreign key constraint (always 'def')
	'def'::varchar(512) AS "CONSTRAINT_CATALOG",

	-- Schema of the foreign key constraint
	nc.nspname::varchar(64) AS "CONSTRAINT_SCHEMA",

	-- Name of the foreign key constraint
	c.conname::varchar(64) AS "CONSTRAINT_NAME",

	-- Catalog of the referenced unique/primary key constraint (always 'def')
	'def'::varchar(512) AS "UNIQUE_CONSTRAINT_CATALOG",

	-- Schema of the referenced unique/primary key constraint
	nr.nspname::varchar(64) AS "UNIQUE_CONSTRAINT_SCHEMA",

	-- Name of the referenced constraint: 'PRIMARY' for primary keys, else the constraint name
	CASE uc.contype
		WHEN 'p'::"char" THEN 'PRIMARY'
		ELSE uc.conname
	END::varchar(64) AS "UNIQUE_CONSTRAINT_NAME",

	-- Match strategy: FULL, PARTIAL, or NONE
	CASE c.confmatchtype
		WHEN 'f'::"char" THEN 'FULL'
		WHEN 'p'::"char" THEN 'PARTIAL'
		ELSE 'NONE'
	END::varchar(64) AS "MATCH_OPTION",

	-- ON UPDATE rule
	CASE c.confupdtype
		WHEN 'a'::"char" THEN 'NO ACTION'
		WHEN 'r'::"char" THEN 'RESTRICT'
		WHEN 'c'::"char" THEN 'CASCADE'
		WHEN 'n'::"char" THEN 'SET NULL'
		WHEN 'd'::"char" THEN 'SET DEFAULT'
		ELSE 'NO ACTION'
	END::varchar(64) AS "UPDATE_RULE",

	-- ON DELETE rule
	CASE c.confdeltype
		WHEN 'a'::"char" THEN 'NO ACTION'
		WHEN 'r'::"char" THEN 'RESTRICT'
		WHEN 'c'::"char" THEN 'CASCADE'
		WHEN 'n'::"char" THEN 'SET NULL'
		WHEN 'd'::"char" THEN 'SET DEFAULT'
		ELSE 'NO ACTION'
	END::varchar(64) AS "DELETE_RULE",

	-- Table that holds the foreign key
	r.relname::varchar(64) AS "TABLE_NAME",

	-- Referenced table
	rr.relname::varchar(64) AS "REFERENCED_TABLE_NAME"

FROM
	pg_constraint c
JOIN pg_namespace nc ON nc.oid = c.connamespace
JOIN pg_class r ON r.oid = c.conrelid
JOIN pg_class rr ON rr.oid = c.confrelid
JOIN pg_namespace nr ON nr.oid = rr.relnamespace
-- Find the unique/primary key constraint on the referenced table via its supporting index
LEFT JOIN pg_constraint uc ON uc.conindid = c.conindid
	AND uc.conrelid = c.confrelid
	AND uc.contype = ANY (ARRAY['p'::"char", 'u'::"char"])
WHERE
	c.contype = 'f'::"char"
	AND r.relkind = ANY (ARRAY['r'::"char", 'p'::"char"])
	AND NOT pg_is_other_temp_schema(nc.oid)
	AND (
		pg_has_role(r.relowner, 'USAGE'::text)
		OR has_table_privilege(r.oid, 'INSERT, UPDATE, DELETE, TRUNCATE, REFERENCES, TRIGGER'::text)
		OR has_any_column_privilege(r.oid, 'INSERT, UPDATE, REFERENCES'::text)
	);

GRANT SELECT ON mys_information_schema.referential_constraints TO PUBLIC;

--
-- View: mys_information_schema.parameters
--
CREATE OR REPLACE VIEW mys_information_schema.parameters AS
-- Return values for functions (ORDINAL_POSITION = 0)
SELECT
	-- Catalog name (MySQL-compatible constant: always 'def')
	'def'::varchar(512)                                                       AS "SPECIFIC_CATALOG",

	-- Schema containing the routine
	n.nspname::varchar(64)                                                    AS "SPECIFIC_SCHEMA",

	-- Specific (routine) name
	p.proname::varchar(64)                                                    AS "SPECIFIC_NAME",

	-- 0 indicates return value (not an input/output parameter)
	0::int                                                                    AS "ORDINAL_POSITION",

	-- NULL for return value (no IN/OUT/INOUT)
	NULL::varchar(5)                                                          AS "PARAMETER_MODE",

	-- Return value has no parameter name
	NULL::varchar(64)                                                         AS "PARAMETER_NAME",

	-- Data type of the function parameter
	mys_information_schema.mys_format_type(t.oid)::varchar(64)                AS "DATA_TYPE",

	-- Max character length (for string types only)
	CASE
	    WHEN t.typname IN ('bpchar', 'binary') THEN 255
	    WHEN t.typname IN ('varchar', 'varbinary') THEN 65535
	    WHEN t.typname IN ('tinytext', 'tinyblob') THEN 255
	    WHEN t.typname IN ('text', 'myblob') THEN 65535
	    WHEN t.typname IN ('mediumtext', 'mediumblob') THEN 16777215
	    WHEN t.typname IN ('longtext', 'longblob') THEN 2147483647
		WHEN t.typtype = 'e' THEN 765
	    ELSE NULL
	END::int                                                                  AS "CHARACTER_MAXIMUM_LENGTH",

	-- Max byte length
	CASE
	    WHEN t.typname IN ('bpchar', 'binary') THEN 255
	    WHEN t.typname IN ('varchar', 'varbinary') THEN 65535
	    WHEN t.typname IN ('tinytext', 'tinyblob') THEN 255
	    WHEN t.typname IN ('text', 'myblob') THEN 65535
	    WHEN t.typname IN ('mediumtext', 'mediumblob') THEN 16777215
	    WHEN t.typname IN ('longtext', 'longblob') THEN 2147483647
		WHEN t.typtype = 'e' THEN 2295
	    ELSE NULL
	END::int                                                                  AS "CHARACTER_OCTET_LENGTH",

	-- Precision for numeric types
	CAST(CASE
	    WHEN t.typname IN ('tinyint', 'uint1') THEN 3
	    WHEN t.typname IN ('int2', 'uint2') THEN 5
	    WHEN t.typname IN ('mediumint', 'uint3') THEN 7
	    WHEN t.typname IN ('int4', 'uint4') THEN 10
	    WHEN t.typname = 'int8' THEN 19
	    WHEN t.typname = 'uint8' THEN 20
	    WHEN t.typname = 'float4' THEN 12
	    WHEN t.typname = 'float8' THEN 22
	    WHEN t.typname = 'numeric' THEN 65
		WHEN t.typname = 'mysbit' THEN 64
	    ELSE NULL
	END AS UNSIGNED)                                                          AS "NUMERIC_PRECISION",

	-- Scale for numeric types
	CASE
	    WHEN t.typname IN ('tinyint','int2','mediumint','int4','int8','uint1','uint2','uint3','uint4','uint8') THEN 0
	    WHEN t.typname = 'numeric' THEN 30
	    ELSE NULL
	END::int                                                                  AS "NUMERIC_SCALE",

	-- Precision for datetime types
	CAST(CASE
	    WHEN t.typname IN ('mystime', 'mystimes',
	                       'mysdatetime', 'mysdatetimes',
	                       'mystimestamp', 'mystimestamps') THEN 6
	    ELSE NULL
	END AS UNSIGNED)                                                          AS "DATETIME_PRECISION",

	-- Character set name
	CASE WHEN t.typcategory IN('S', 'E') AND t.typname <> 'varbinary'
		 THEN pg_catalog.getdatabaseencoding()
	     ELSE NULL
	END::varchar(64)                                                          AS "CHARACTER_SET_NAME",

	-- Collation name
	CASE WHEN t.typcategory IN('S', 'E') AND t.typname <> 'varbinary'
		 THEN
	         (SELECT "Default collation" FROM show_mysql_charsets()
	          WHERE lower("Charset") = lower(pg_catalog.getdatabaseencoding()))
	     ELSE NULL
	END::varchar(64)                                                          AS "COLLATION_NAME",

	-- Full parameter data_type descriptor
	mys_information_schema.mys_format_type(t.oid)::varchar(64)                AS "DTD_IDENTIFIER",

	-- Type of routine: FUNCTION or PROCEDURE
	'FUNCTION'::varchar(9)                                                    AS "ROUTINE_TYPE"
FROM pg_proc p
JOIN pg_namespace n ON n.oid = p.pronamespace
JOIN pg_type t ON t.oid = p.prorettype
JOIN pg_namespace nt ON nt.oid = t.typnamespace
LEFT JOIN pg_collation co ON co.oid = t.typcollation
WHERE p.prokind = 'f'
  AND p.prorettype <> 2278
  AND (pg_has_role(p.proowner, 'USAGE') OR has_function_privilege(p.oid, 'EXECUTE'))

UNION ALL

-- Regular parameters (ORDINAL_POSITION >= 1)
SELECT
	-- Catalog name (MySQL-compatible constant: always 'def')
	'def'::varchar(512)                                                       AS "SPECIFIC_CATALOG",

	-- Schema containing the routine
	ss.n_nspname::varchar(64)                                                 AS "SPECIFIC_SCHEMA",

	-- Specific (routine) name
	ss.proname::varchar(64)                                                   AS "SPECIFIC_NAME",

	-- Parameter ordinal position (1-based)
	CAST(ss.n AS int)                                                         AS "ORDINAL_POSITION",

	-- Parameter direction: IN, OUT, or INOUT
	CASE WHEN ss.proargmodes IS NULL THEN 'IN'
	     WHEN ss.proargmodes[ss.n] = 'i' THEN 'IN'
	     WHEN ss.proargmodes[ss.n] = 'o' THEN 'OUT'
	     WHEN ss.proargmodes[ss.n] = 'b' THEN 'INOUT'
	     WHEN ss.proargmodes[ss.n] = 'v' THEN 'IN'
	     WHEN ss.proargmodes[ss.n] = 't' THEN 'OUT'
	END::varchar(5)                                                           AS "PARAMETER_MODE",

	-- Parameter name (NULL if unnamed)
	NULLIF(ss.proargnames[ss.n], '')::varchar(64)                             AS "PARAMETER_NAME",

	-- Data type of the parameter
	mys_information_schema.mys_format_type(t.oid)::varchar(64)                AS "DATA_TYPE",

	-- Max character length (for string types only)
	CASE
	    WHEN t.typname IN ('bpchar', 'binary') THEN 255
	    WHEN t.typname IN ('varchar', 'varbinary') THEN 65535
	    WHEN t.typname IN ('tinytext', 'tinyblob') THEN 255
	    WHEN t.typname IN ('text', 'myblob') THEN 65535
	    WHEN t.typname IN ('mediumtext', 'mediumblob') THEN 16777215
	    WHEN t.typname IN ('longtext', 'longblob') THEN 2147483647
		WHEN t.typtype = 'e' THEN 765
	    ELSE NULL
	END::int                                                                  AS "CHARACTER_MAXIMUM_LENGTH",

	-- Max byte length
	CASE
	    WHEN t.typname IN ('bpchar', 'binary') THEN 255
	    WHEN t.typname IN ('varchar', 'varbinary') THEN 65535
	    WHEN t.typname IN ('tinytext', 'tinyblob') THEN 255
	    WHEN t.typname IN ('text', 'myblob') THEN 65535
	    WHEN t.typname IN ('mediumtext', 'mediumblob') THEN 16777215
	    WHEN t.typname IN ('longtext', 'longblob') THEN 2147483647
		WHEN t.typtype = 'e' THEN 2295
	    ELSE NULL
	END::int                                                                  AS "CHARACTER_OCTET_LENGTH",

	-- Precision for numeric types
	CAST(CASE
	    WHEN t.typname IN ('tinyint', 'uint1') THEN 3
	    WHEN t.typname IN ('int2', 'uint2') THEN 5
	    WHEN t.typname IN ('mediumint', 'uint3') THEN 7
	    WHEN t.typname IN ('int4', 'uint4') THEN 10
	    WHEN t.typname = 'int8' THEN 19
	    WHEN t.typname = 'uint8' THEN 20
	    WHEN t.typname = 'float4' THEN 12
	    WHEN t.typname = 'float8' THEN 22
	    WHEN t.typname = 'numeric' THEN 65
		WHEN t.typname = 'mysbit' THEN 64
	    ELSE NULL
	END AS UNSIGNED)                                                          AS "NUMERIC_PRECISION",

	-- Scale for numeric types
	CASE
	    WHEN t.typname IN ('tinyint','int2','mediumint','int4','int8','uint1','uint2','uint3','uint4','uint8') THEN 0
	    WHEN t.typname = 'numeric' THEN 30
	    ELSE NULL
	END::int                                                                  AS "NUMERIC_SCALE",

	-- Precision for datetime types
	CAST(CASE
	    WHEN t.typname IN ('mystime', 'mystimes',
	                       'mysdatetime', 'mysdatetimes',
	                       'mystimestamp', 'mystimestamps') THEN 6
	    ELSE NULL
	END AS UNSIGNED)                                                          AS "DATETIME_PRECISION",

	-- Character set name
	CASE WHEN t.typcategory IN('S', 'E') AND t.typname <> 'varbinary'
		 THEN pg_catalog.getdatabaseencoding()
	     ELSE NULL
	END::varchar(64)                                                          AS "CHARACTER_SET_NAME",

	-- Collation name
	CASE WHEN t.typcategory IN('S', 'E') AND t.typname <> 'varbinary'
		 THEN
	         (SELECT "Default collation" FROM show_mysql_charsets()
	          WHERE lower("Charset") = lower(pg_catalog.getdatabaseencoding()))
	     ELSE NULL
	END::varchar(64)                                                          AS "COLLATION_NAME",

	-- Full parameter data_type descriptor
	mys_information_schema.mys_format_type(t.oid)::varchar(64)                AS "DTD_IDENTIFIER",

	-- Type of routine: FUNCTION or PROCEDURE
	CASE ss.prokind
	    WHEN 'f' THEN 'FUNCTION'
	    WHEN 'p' THEN 'PROCEDURE'
	    ELSE NULL
	END::varchar(9)                                                           AS "ROUTINE_TYPE"
FROM pg_type t
JOIN pg_namespace nt ON nt.oid = t.typnamespace
LEFT JOIN pg_collation co ON co.oid = t.typcollation
JOIN (
	SELECT n.nspname AS n_nspname, p.proname, p.oid AS p_oid, p.proowner,
	       p.proargnames, p.proargmodes, p.prokind,
	       u.typoid, u.n::integer AS n
	FROM pg_namespace n, pg_proc p,
	     unnest(coalesce(p.proallargtypes, p.proargtypes::oid[])) WITH ORDINALITY AS u(typoid, n)
	WHERE n.oid = p.pronamespace
	      AND (pg_has_role(p.proowner, 'USAGE') OR has_function_privilege(p.oid, 'EXECUTE'))
) AS ss ON t.oid = ss.typoid;

GRANT SELECT ON mys_information_schema.parameters TO PUBLIC;

--
-- View: performance_schema.global_status
--

CREATE OR REPLACE VIEW performance_schema.global_status AS
SELECT variable_name, variable_value
FROM (
	VALUES

	-- Aborted connections (client connection lost)
	('Aborted_clients', (SELECT COALESCE(SUM(sessions_abandoned), 0) FROM pg_stat_database)::text),

	-- Failed connections (fatal error or terminated by administrator)
	('Aborted_connects', (SELECT COALESCE(SUM(sessions_fatal + sessions_killed), 0) FROM pg_stat_database)::text),

	-- Total number of connections
	('Connections', (SELECT sessions FROM pg_stat_database WHERE datname = current_database())::text),

	-- Number of temporary files
	('Created_tmp_file', (SELECT COALESCE(SUM(temp_files), 0) FROM pg_stat_database)::text),

	-- Size of temporary files (in bytes)
	('Created_tmp_disk_tables', (SELECT COALESCE(SUM(temp_bytes), 0) FROM pg_stat_database)::text),

	-- Number of queries
	('Queries', (SELECT sum(xact_commit+xact_rollback) FROM pg_stat_database)::text),

	-- Current number of connections
	('Threads_connected', (SELECT numbackends FROM pg_stat_database WHERE datname = current_database())::text),

	-- Current number of active connections
	('Threads_running', (SELECT count(*) FROM pg_stat_activity WHERE state='active')::text),

	-- Runtime since PostgreSQL server start
	('Uptime', EXTRACT(EPOCH FROM (clock_timestamp() - pg_postmaster_start_time()))::bigint::text)
) AS t(variable_name, variable_value);

GRANT SELECT ON performance_schema.global_status TO PUBLIC;

--
-- View: performance_schema.session_status
--

CREATE OR REPLACE VIEW performance_schema.session_status AS
SELECT variable_name, variable_value
FROM (
	VALUES

	-- Aborted connections (client connection lost)
	('Aborted_clients', (SELECT COALESCE(SUM(sessions_abandoned), 0) FROM pg_stat_database)::text),

	-- Failed connections (fatal error or terminated by administrator)
	('Aborted_connects', (SELECT COALESCE(SUM(sessions_fatal + sessions_killed), 0) FROM pg_stat_database)::text),

	-- Total number of connections
	('Connections', (SELECT sessions FROM pg_stat_database WHERE datname = current_database())::text),

	-- Number of temporary files
	('Created_tmp_file', (SELECT COALESCE(SUM(temp_files), 0) FROM pg_stat_database)::text),

	-- Size of temporary files (in bytes)
	('Created_tmp_disk_tables', (SELECT COALESCE(SUM(temp_bytes), 0) FROM pg_stat_database)::text),

	-- Number of queries
	('Queries', (SELECT sum(xact_commit+xact_rollback) FROM pg_stat_database)::text),

	-- Current number of connections
	('Threads_connected', (SELECT numbackends FROM pg_stat_database WHERE datname = current_database())::text),

	-- Current number of active connections
	('Threads_running', (SELECT count(*) FROM pg_stat_activity WHERE state='active')::text),

	-- Runtime since PostgreSQL server start
	('Uptime', EXTRACT(EPOCH FROM (clock_timestamp() - pg_postmaster_start_time()))::bigint::text)
) AS t(variable_name, variable_value);

GRANT SELECT ON performance_schema.session_status TO PUBLIC;

/* End - #Z5943 */
