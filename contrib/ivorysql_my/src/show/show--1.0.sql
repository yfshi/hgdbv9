/*
 * MySQL show command
 *
 * contrib/ivorysql_my/src/show/show--1.0.sql
 *
 * add the file for requirement "SRS-MYSQL-SHOW"
 *
 */

--
--show charset
--
CREATE TYPE mys_show_charset_results AS ("Charset" VARCHAR,
                                         "Description" VARCHAR,
                                         "Default collation" VARCHAR,
                                         "Maxlen" INT8);

CREATE FUNCTION pg_catalog.show_mysql_charsets()
RETURNS setof mys_show_charset_results
AS 'MODULE_PATHNAME','show_mysql_charsets'
LANGUAGE C
PARALLEL SAFE;

CREATE TYPE mys_show_collation_results AS ("Collation" VARCHAR,
                                           "Charset" VARCHAR,
                                           "Id" INT8,
                                           "Default" VARCHAR,
                                           "Compiled" VARCHAR,
                                           "Sortlen" INT8);

CREATE FUNCTION pg_catalog.show_mysql_collations()
RETURNS setof mys_show_collation_results
AS 'MODULE_PATHNAME','show_mysql_collations'
LANGUAGE C
PARALLEL SAFE;

--
-- show create database
--
CREATE FUNCTION ivymy.mys_show_create_database(
	IN databasename NAME,
	IN hasexist BOOL,
	OUT Database TEXT,
	OUT "Create Database" TEXT)
RETURNS SETOF record
AS 'MODULE_PATHNAME', 'mys_show_create_database'
LANGUAGE C PARALLEL SAFE;

--
-- show create table
--
CREATE FUNCTION ivymy.mys_show_create_table(Oid)
RETURNS text
AS 'MODULE_PATHNAME', 'mys_show_create_table'
LANGUAGE C
STRICT PARALLEL SAFE IMMUTABLE;

--
-- show privileges
--
CREATE FUNCTION ivymy.mys_show_privileges(
	OUT "Privilege" TEXT,
	OUT "Context" TEXT,
	OUT "Comment" TEXT)
RETURNS SETOF record
AS 'MODULE_PATHNAME', 'mys_show_privileges'
LANGUAGE C PARALLEL SAFE;

--
-- show warnings
--
CREATE FUNCTION ivymy.mys_show_warnings(
	OUT "Level" TEXT,
	OUT "Code" TEXT,
	OUT "Message" TEXT)
RETURNS SETOF record
AS 'MODULE_PATHNAME', 'mys_show_warnings'
LANGUAGE C PARALLEL SAFE;

-- Begin bug#Z5956
--
-- View: mys_information_schema.showcolumn_privileges
--

CREATE OR REPLACE VIEW mys_information_schema.showcolumn_privileges AS
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

GRANT SELECT ON mys_information_schema.showcolumn_privileges TO PUBLIC;
--
-- View: mys_information_schema.showcolumns
--
CREATE OR REPLACE VIEW mys_information_schema.showcolumns AS
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
			format_type(t.typbasetype, NULL)
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
		FROM mys_information_schema.showcolumn_privileges cp
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
JOIN pg_class c ON a.attrelid = c.oid
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
	SELECT i.indexrelid
	FROM pg_index i
	WHERE i.indrelid=c.oid AND a.attnum = any(i.indkey) /* BUG#Z6281*/
	LIMIT 1
) idx ON true

WHERE a.attnum > 0
	AND NOT a.attisdropped
	AND c.relkind IN ('r','p','v','f')  -- Only regular, partitioned, view, foreign tables
	AND NOT pg_is_other_temp_schema(nc.oid)
	AND (pg_has_role(c.relowner, 'USAGE') OR has_column_privilege(c.oid, a.attnum, 'SELECT, INSERT, UPDATE, REFERENCES'));

GRANT SELECT ON mys_information_schema.showcolumns TO PUBLIC;
-- End bug#Z5956

-- Begin bug#Z5968
--
-- View: mys_information_schema.tables
--
CREATE OR REPLACE VIEW mys_information_schema.showtables AS
SELECT
	'def'::varchar(512) AS "TABLE_CATALOG",                                    -- Name of the database catalog (def)
	n.nspname::varchar(64) AS "TABLE_SCHEMA",                                  -- Name of the schema containing the table
	c.relname::varchar(64) AS "TABLE_NAME",                                    -- Table name

	CASE WHEN c.relkind = 'v' THEN
		NULL
	ELSE
		CASE
			WHEN n.nspname = 'information_schema' THEN 'SYSTEM VIEW'             -- System views from information_schema
			WHEN c.relkind = 'r' THEN 'BASE TABLE'                               -- Ordinary table
			WHEN c.relkind = 'p' THEN 'BASE TABLE'                               -- Partitioned table
			WHEN c.relkind = 'v' THEN 'VIEW'                                     -- View
			WHEN c.relkind = 'm' THEN 'VIEW'                                     -- Materialized view
			WHEN c.relkind = 'f' THEN 'FOREIGN TABLE'                            -- Foreign table
			ELSE NULL
		END::varchar(64)
	END AS "TABLE_TYPE",

	CASE WHEN c.relkind = 'v' THEN
		NULL
	ELSE
		'InnoDB'::varchar(64)
	END AS "ENGINE",                                       -- Storage engine (access method), e.g. heap

	CAST(NULL AS unsigned) AS "VERSION",                                     -- Version info (not supported in PostgreSQL)

	CASE WHEN c.relkind = 'v' THEN
		NULL
	ELSE
		'Dynamic'::varchar(10)
	END AS "ROW_FORMAT",                                  -- Row storage format description

	CASE WHEN c.relkind = 'v' THEN
		NULL
	ELSE
		CAST(n_live_tup AS unsigned)
	END AS "TABLE_ROWS",                            -- Estimated number of live rows (from pg_stat_user_tables)

	CASE WHEN c.relkind = 'v' THEN
		NULL
	ELSE
		CAST(CASE (us.n_tup_ins - us.n_tup_del)                                  -- Average row length (estimated)
		WHEN 0 THEN 0
		ELSE (pg_catalog.pg_total_relation_size(c.oid::regclass) / (us.n_tup_ins - us.n_tup_del))
		END AS unsigned)
	END AS "AVG_ROW_LENGTH",

	CASE WHEN c.relkind = 'v' THEN
		NULL
	ELSE
		CAST(pg_catalog.pg_total_relation_size(c.oid::regclass) AS unsigned)
	END AS "DATA_LENGTH",    -- Total size of the table including all indexes and TOAST data

	CASE WHEN c.relkind = 'v' THEN
		NULL
	ELSE
		CAST(CASE c.relkind
			WHEN 'r' THEN 0
			ELSE NULL
		END AS unsigned)
	END AS "MAX_DATA_LENGTH",                                              -- Maximum data length (not supported)

	CASE WHEN c.relkind = 'v' THEN
		NULL
	ELSE
		CAST(pg_catalog.pg_indexes_size(c.oid::regclass) AS unsigned)
	END AS "INDEX_LENGTH",        -- Total size of all indexes attached to the table

	CASE WHEN c.relkind = 'v' THEN
		NULL
	ELSE
		CAST('16384' AS unsigned)
	END AS "DATA_FREE",                                               -- Estimated free space in tablespace (not precise)

	CASE WHEN c.relkind = 'v' THEN
		NULL
	ELSE
		CAST(
		(SELECT CASE MYSSERIES                                                -- AUTO_INCREMENT value (for identity/serial columns)
			WHEN NULL THEN NULL
			ELSE
				(SELECT ivymy.show_next_autoinc(c.oid))
			END
		FROM (SELECT pg_catalog.pg_get_serial_sequence(c.relname, at.attname) AS seriesname
			FROM pg_attribute at
			WHERE at.attrelid = c.oid AND (at.attidentity = 'a' or at.attidentity = 'd' or at.attidentity = 'm')) AS MYSSERIES) AS unsigned)
	END AS "AUTO_INCREMENT",

	CASE WHEN c.relkind = 'v' THEN
		NULL
	ELSE
		ob.created::mysdatetime
	END AS "CREATE_TIME",                                 -- Table creation timestamp (from pg_object)

	CASE WHEN c.relkind = 'v' THEN
		NULL
	ELSE
		ob.modified::mysdatetime
	END AS "UPDATE_TIME",                                -- Table last modification timestamp (from pg_object)

	NULL::mysdatetime AS "CHECK_TIME",                                        -- Last check time (not supported)

	CASE WHEN c.relkind = 'v' THEN
		NULL
	ELSE
		(SELECT d.datcollate
			FROM pg_catalog.pg_database d
		  WHERE d.datname = current_database())::varchar(32)
	END AS "TABLE_COLLATION", -- Default collation of the table

	CAST(NULL AS unsigned) AS "CHECKSUM",                                -- Table checksum (not supported)

	CASE c.relispartition                                                -- Additional create options (e.g. partitioned)
		WHEN TRUE THEN 'partitioned'
		ELSE NULL
	END::varchar(255) AS "CREATE_OPTIONS",

	CASE WHEN c.relkind = 'v' THEN
		'VIEW'::varchar(2048)
	ELSE
		pg_catalog.obj_description(c.oid)::varchar(2048)
	END AS "TABLE_COMMENT"  -- Table comment/description

FROM pg_catalog.pg_class c
	LEFT JOIN pg_catalog.pg_object ob ON c.oid = ob.obj_oid
	LEFT JOIN pg_catalog.pg_namespace n ON c.relnamespace = n.oid
	LEFT JOIN pg_catalog.pg_stat_user_tables us ON c.oid = us.relid
WHERE
	c.relkind IN ('r', 'v', 'm', 'p', 'f')                              -- Tables, views, materialized views, partitioned tables, foreign tables
	AND c.relpersistence <> 't'                                         -- /* Bug - #z6441 */
ORDER BY c.oid;		-- bug#Z5983

GRANT SELECT ON mys_information_schema.showtables TO PUBLIC;

-- End bug#Z5968

-- Begin bug#Z5973

-- Begin - BUG#ZZ5974
CREATE FUNCTION mys_information_schema.mys_get_gistindex_column(indexoid Oid)
RETURNS text
AS 'MODULE_PATHNAME','mys_get_gistindex_column'
LANGUAGE C
PARALLEL SAFE STRICT;
-- End - BUG#ZZ5974

--
-- View: mys_information_schema.statistics
--
CREATE OR REPLACE VIEW mys_information_schema.showstatistics AS
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
			WHEN pg_get_indexdef(i.indexrelid, k.n::integer, true) ~* '"?left"?\s*\(.*,\s*\d+\s*\)' THEN
				(regexp_match(pg_get_indexdef(i.indexrelid, k.n::integer, true), '"left"\s*\(\s*([^,:\s")]+)'))[1]::varchar(64)
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
		  WHEN pg_get_indexdef(ci.oid, k.n::integer, true) ~ '^\(\d+::name\)$' THEN
			(regexp_match(pg_get_indexdef(ci.oid, k.n::integer, true), '^\((\d+)::name\)$'))[1]::int
		  -- Handle PostgreSQL native expression indexes like "left"(name, 100)
		  WHEN pg_get_indexdef(ci.oid, k.n::integer, true) ~* '"?left"?\s*\(.*,\s*\d+\s*\)' THEN
			(regexp_match(pg_get_indexdef(ci.oid, k.n::integer, true), '"?left"?\s*\(.*,\s*(\d+)\s*\)'))[1]::int
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
JOIN pg_index i ON t.oid = i.indrelid					  -- Table-to-index relationship
JOIN pg_class ci ON ci.oid = i.indexrelid				  -- Index relation
JOIN pg_am am ON ci.relam = am.oid						  -- Index access method
JOIN LATERAL unnest(i.indkey, i.indoption) WITH ORDINALITY AS k(attnum, opt, n) ON true   -- Unnest index keys with order
LEFT JOIN pg_description d ON ci.oid = d.objoid
LEFT JOIN pg_attribute a ON a.attrelid = t.oid AND a.attnum = k.attnum	-- Index column attributes
LEFT JOIN pg_stats s ON s.schemaname = n.nspname AND s.tablename = t.relname AND s.attname = a.attname	-- Statistics for cardinality

WHERE
	t.relkind = 'r' OR t.relkind = 'p'					  -- Only ordinary tables --BUG#Z5991,add OR t.relkind = 'p'
	AND NOT i.indisprimary							  -- Exclude primary key indexes (MySQL does not show these in statistics)
ORDER BY i.indexrelid;								  -- Eliminate the uncertain regression error

GRANT SELECT ON mys_information_schema.showstatistics TO PUBLIC;
-- End bug#Z5973

-- Begin - BUG#6420
CREATE FUNCTION mys_information_schema.mysql_get_indexdef_columns(indexoid Oid)
RETURNS text
AS 'MODULE_PATHNAME','mysql_get_indexdef_columns'
LANGUAGE C
PARALLEL SAFE STRICT;
-- End - BUG#6420
