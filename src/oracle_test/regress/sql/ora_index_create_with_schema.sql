--
-- Tests for cross-schema index creation
--

-- Create test schemas
CREATE SCHEMA test_index_schema;
CREATE SCHEMA test_index_schema_2;

-- Create a test user
CREATE USER test_index_user;

-- Grant create privileges on test_index_schema_2 to test user
GRANT USAGE, CREATE ON SCHEMA test_index_schema_2 TO test_index_user;

--
-- Permission check test
--
SET ROLE test_index_user;

-- Create a test table in public schema
CREATE TABLE public.test_table (
    id serial PRIMARY KEY,
    name text,
    value int
);

-- Insert some test data
INSERT INTO public.test_table (name, value)
SELECT 'name_' || i, i
FROM generate_series(1, 100) i;

-- Try to create index without permission on test_index_schema - should fail
CREATE INDEX test_index_schema.idx_test_perm_fail ON public.test_table (id);

-- Create index with permission on test_index_schema_2 - should succeed
CREATE INDEX test_index_schema_2.idx_test_perm_ok ON public.test_table (id);
CREATE INDEX test_index_schema_2.idx_test_perm_ok_2 ON public.test_table (id);

-- Verify indexes
SELECT n.nspname AS schema_name, c.relname AS index_name
FROM pg_class c
JOIN pg_namespace n ON c.relnamespace = n.oid
WHERE c.relname IN ('idx_test_perm_ok', 'idx_test_perm_ok_2')
ORDER BY index_name, schema_name;

-- Drop index
DROP INDEX idx_test_perm_ok;	-- ERROR
SET search_path TO 'test_index_schema_2';
DROP INDEX idx_test_perm_ok;	-- OK
RESET search_path;

DROP INDEX test_index_schema_2.idx_test_perm_ok_2;

-- Retrun null, index already be droped.
SELECT n.nspname AS schema_name, c.relname AS index_name
FROM pg_class c
JOIN pg_namespace n ON c.relnamespace = n.oid
WHERE c.relname IN ('idx_test_perm_ok', 'idx_test_perm_ok_2')
ORDER BY index_name, schema_name;

--
-- Create and using index
-- 
-- Create index in same schema as table (default behavior)
CREATE INDEX idx_test_table ON public.test_table (name);

-- Auto-generated index name
CREATE INDEX ON public.test_table (name, value);

-- Create index in different schema
CREATE INDEX test_index_schema_2.idx_test_table ON public.test_table (value);

-- Verify schema name of indexes
SELECT n.nspname AS schema_name, c.relname AS index_name
FROM pg_class c
JOIN pg_namespace n ON c.relnamespace = n.oid
JOIN pg_index i ON c.oid = i.indexrelid
WHERE i.indrelid = 'public.test_table'::regclass
ORDER BY index_name, schema_name;

-- IF NOT EXISTS
CREATE INDEX test_index_schema_2.idx_if_not_exists ON public.test_table (id, name);
CREATE INDEX IF NOT EXISTS test_index_schema_2.idx_if_not_exists ON public.test_table (id, name);
DROP INDEX test_index_schema_2.idx_if_not_exists;

-- has schema name within explain output when verbose
SET enable_seqscan = off;
SET enable_bitmapscan = off;

EXPLAIN (VERBOSE, COSTS OFF) SELECT * FROM public.test_table WHERE name = 'name_50';
EXPLAIN (VERBOSE, COSTS OFF) SELECT * FROM public.test_table WHERE value = 50;

RESET enable_seqscan;
RESET enable_bitmapscan;

-- Temporary table should not allow schema specification
CREATE TEMP TABLE temp_test_table (id int, data text);
CREATE INDEX test_index_schema_2.idx_temp_test ON temp_test_table (id);	-- error
DROP TABLE temp_test_table;

--
-- Partitioned table
--
CREATE TABLE public.partitioned_table_index_test(
    id int,
    created_at date,
    region text
) PARTITION BY RANGE (created_at);

-- Create 1-level partitions
CREATE TABLE public.partitioned_table_index_test_2023 PARTITION OF public.partitioned_table_index_test
    FOR VALUES FROM ('2023-01-01') TO ('2024-01-01')
    PARTITION BY LIST (region);

CREATE TABLE public.partitioned_table_index_test_2024 PARTITION OF public.partitioned_table_index_test
    FOR VALUES FROM ('2024-01-01') TO ('2025-01-01')
    PARTITION BY LIST (region);

-- Create 2-level partitions for 2023
CREATE TABLE public.partitioned_table_index_test_2023_north PARTITION OF public.partitioned_table_index_test_2023
    FOR VALUES IN ('north');

CREATE TABLE public.partitioned_table_index_test_2023_south PARTITION OF public.partitioned_table_index_test_2023
    FOR VALUES IN ('south');

-- Create 2-level partitions for 2024
CREATE TABLE public.partitioned_table_index_test_2024_north PARTITION OF public.partitioned_table_index_test_2024
    FOR VALUES IN ('north');

CREATE TABLE public.partitioned_table_index_test_2024_south PARTITION OF public.partitioned_table_index_test_2024
    FOR VALUES IN ('south');

-- Create index with specified schema for parent and not recurse to all children 
CREATE INDEX test_index_schema_2.idx_partitioned_created_at ON public.partitioned_table_index_test (created_at);

-- Verify parent index is in specified schema
SELECT n.nspname AS schema_name, c.relname AS index_name
FROM pg_class c
JOIN pg_namespace n ON c.relnamespace = n.oid
WHERE c.relname = 'idx_partitioned_created_at';

-- Verify all child indexes in public
SELECT n.nspname AS schema_name, c.relname AS index_name
FROM pg_class c
JOIN pg_namespace n ON c.relnamespace = n.oid
JOIN pg_index i ON c.oid = i.indexrelid
WHERE i.indrelid IN (
    SELECT c.oid FROM pg_class c
    WHERE c.relname LIKE 'partitioned_table_index_test_202%'
)
ORDER BY index_name, schema_name;

-- Cleanup partitioned table
DROP TABLE public.partitioned_table_index_test CASCADE;

--
-- REINDEX
--
REINDEX INDEX test_index_schema_2.idx_test_table;
REINDEX INDEX public.idx_test_table;
REINDEX INDEX idx_test_table;	-- OK, depend on search_path
REINDEX TABLE public.test_table;

--
-- ALTER INDEX
--
SELECT n.nspname AS schema_name, c.relname AS index_name
FROM pg_class c
JOIN pg_namespace n ON c.relnamespace = n.oid
JOIN pg_index i ON c.oid = i.indexrelid
WHERE i.indrelid = 'public.test_table'::regclass
ORDER BY index_name, schema_name;

ALTER INDEX test_index_schema_2.idx_test_table RENAME TO idx_test_table_new_1;
ALTER INDEX public.idx_test_table RENAME TO idx_test_table_new_2;
ALTER INDEX public.test_table_name_value_idx RENAME TO idx_test_table_new_3;

SELECT n.nspname AS schema_name, c.relname AS index_name
FROM pg_class c
JOIN pg_namespace n ON c.relnamespace = n.oid
JOIN pg_index i ON c.oid = i.indexrelid
WHERE i.indrelid = 'public.test_table'::regclass
ORDER BY index_name, schema_name;

DROP TABLE public.test_table CASCADE;

--
-- ALTER TABLE ADD CONSTRAINT USING INDEX
--
CREATE TABLE public.test_table_2 (
    id serial NOT NULL,
    name text NOT NULL,
    value int,
    code text NOT NULL,
    description text NOT NULL
);

CREATE UNIQUE INDEX idx_pk_public ON public.test_table_2 (id);
CREATE UNIQUE INDEX idx_uniq_public ON public.test_table_2 (name);
CREATE UNIQUE INDEX test_index_schema_2.idx_pk_schema ON public.test_table_2 (code);
CREATE UNIQUE INDEX test_index_schema_2.idx_uniq_schema ON public.test_table_2 (description);

-- without schema qualification
ALTER TABLE public.test_table_2 ADD CONSTRAINT pk_test_table_public PRIMARY KEY USING INDEX idx_pk_public;
ALTER TABLE public.test_table_2 ADD CONSTRAINT uniq_test_table_public UNIQUE USING INDEX idx_uniq_public;

-- verify constraints in pg_constraint
SELECT conname, contype
FROM pg_constraint
WHERE conname IN ('pk_test_table_public', 'uniq_test_table_public')
ORDER BY conname;

-- verify indexes in pg_class
SELECT c.relname, n.nspname AS schemaname
FROM pg_class c
JOIN pg_namespace n ON c.relnamespace = n.oid
JOIN pg_index i ON c.oid = i.indexrelid
WHERE i.indrelid = 'public.test_table_2'::regclass
ORDER BY c.relname, n.nspname;

-- with schema qualification
ALTER TABLE public.test_table_2 DROP CONSTRAINT pk_test_table_public;

ALTER TABLE public.test_table_2 ADD CONSTRAINT pk_test_table_schema PRIMARY KEY USING INDEX test_index_schema_2.idx_pk_schema;
ALTER TABLE public.test_table_2 ADD CONSTRAINT uniq_test_table_schema UNIQUE USING INDEX test_index_schema_2.idx_uniq_schema;

-- verify constraints in pg_constraint
SELECT conname, contype
FROM pg_constraint
WHERE conname IN ('pk_test_table_schema', 'uniq_test_table_schema', 'uniq_test_table_public')
ORDER BY conname;

-- verify indexes in pg_class
SELECT c.relname, n.nspname AS schemaname
FROM pg_class c
JOIN pg_namespace n ON c.relnamespace = n.oid
JOIN pg_index i ON c.oid = i.indexrelid
WHERE i.indrelid = 'public.test_table_2'::regclass
ORDER BY c.relname, n.nspname;

-- drop constraints
ALTER TABLE public.test_table_2 DROP CONSTRAINT pk_test_table_schema;
ALTER TABLE public.test_table_2 DROP CONSTRAINT uniq_test_table_schema;
ALTER TABLE public.test_table_2 DROP CONSTRAINT uniq_test_table_public;

--
-- CLUSTER
--
CREATE UNIQUE INDEX idx_test_cluster_ok ON public.test_table_2 (id);
CREATE UNIQUE INDEX test_index_schema_2.idx_test_cluster_schema ON public.test_table_2 (name);

INSERT INTO public.test_table_2 (name, value, code, description)
SELECT 'cluster_' || i, i, 'code_' || i, 'desc_' || i FROM generate_series(1, 100) i;

-- without schema qualification
CLUSTER public.test_table_2 USING idx_test_cluster_ok;
SELECT c.relname, i.indisclustered
FROM pg_class c
JOIN pg_index i ON c.oid = i.indexrelid
WHERE c.relname IN ('idx_test_cluster_ok', 'idx_test_cluster_schema')
ORDER BY c.relname;

-- with schema qualification
CLUSTER public.test_table_2 USING test_index_schema_2.idx_test_cluster_schema;
SELECT c.relname, i.indisclustered
FROM pg_class c
JOIN pg_index i ON c.oid = i.indexrelid
WHERE c.relname IN ('idx_test_cluster_ok', 'idx_test_cluster_schema')
ORDER BY c.relname;

-- old syntax
CLUSTER idx_test_cluster_ok ON public.test_table_2;
SELECT c.relname, i.indisclustered
FROM pg_class c
JOIN pg_index i ON c.oid = i.indexrelid
WHERE c.relname IN ('idx_test_cluster_ok', 'idx_test_cluster_schema')
ORDER BY c.relname;

CLUSTER test_index_schema_2.idx_test_cluster_schema ON public.test_table_2;
SELECT c.relname, i.indisclustered
FROM pg_class c
JOIN pg_index i ON c.oid = i.indexrelid
WHERE c.relname IN ('idx_test_cluster_ok', 'idx_test_cluster_schema')
ORDER BY c.relname;

--
-- ALTER TABLE CLUSTER ON
--
ALTER TABLE public.test_table_2 CLUSTER ON idx_test_cluster_ok;
SELECT c.relname, i.indisclustered
FROM pg_class c
JOIN pg_index i ON c.oid = i.indexrelid
WHERE c.relname IN ('idx_test_cluster_ok', 'idx_test_cluster_schema')
ORDER BY c.relname;

ALTER TABLE public.test_table_2 CLUSTER ON test_index_schema_2.idx_test_cluster_schema;
SELECT c.relname, i.indisclustered
FROM pg_class c
JOIN pg_index i ON c.oid = i.indexrelid
WHERE c.relname IN ('idx_test_cluster_ok', 'idx_test_cluster_schema')
ORDER BY c.relname;

-- clean up cluster set
ALTER TABLE public.test_table_2 SET WITHOUT CLUSTER;
SELECT c.relname, i.indisclustered
FROM pg_class c
JOIN pg_index i ON c.oid = i.indexrelid
WHERE c.relname IN ('idx_test_cluster_ok', 'idx_test_cluster_schema')
ORDER BY c.relname;

--
-- ALTER TABLE REPLICA IDENTITY USING INDEX
--
-- without schema qualification
ALTER TABLE public.test_table_2 REPLICA IDENTITY USING INDEX idx_test_cluster_ok;
SELECT relname, relreplident
FROM pg_class
WHERE relname = 'test_table_2';

-- with schema qualification
ALTER TABLE public.test_table_2 REPLICA IDENTITY USING INDEX test_index_schema_2.idx_test_cluster_schema;
SELECT relname, relreplident
FROM pg_class
WHERE relname = 'test_table_2';

-- reset replica identity
ALTER TABLE public.test_table_2 REPLICA IDENTITY DEFAULT;
SELECT relname, relreplident
FROM pg_class
WHERE relname = 'test_table_2';

--
-- ALTER TABLE SET SCHEMA no longer change the indexes schema,
-- you can using ALTER INDEX SET SCHEMA to do it.
--
SELECT n.nspname AS schema_name, c.relname AS index_name
FROM pg_class c
JOIN pg_namespace n ON c.relnamespace = n.oid
JOIN pg_index i ON c.oid = i.indexrelid
WHERE i.indrelid = 'public.test_table_2'::regclass
ORDER BY index_name, schema_name;

ALTER TABLE public.test_table_2 SET SCHEMA test_index_schema_2;

-- the schema of index no change
SELECT n.nspname AS schema_name, c.relname AS index_name
FROM pg_class c
JOIN pg_namespace n ON c.relnamespace = n.oid
JOIN pg_index i ON c.oid = i.indexrelid
WHERE i.indrelid = 'test_index_schema_2.test_table_2'::regclass
ORDER BY index_name, schema_name;

ALTER INDEX idx_test_cluster_ok SET SCHEMA test_index_schema_2;
ALTER INDEX test_index_schema_2.idx_test_cluster_schema SET SCHEMA test_index_schema_2;

SELECT n.nspname AS schema_name, c.relname AS index_name
FROM pg_class c
JOIN pg_namespace n ON c.relnamespace = n.oid
JOIN pg_index i ON c.oid = i.indexrelid
WHERE i.indrelid = 'test_index_schema_2.test_table_2'::regclass
ORDER BY index_name, schema_name;

-- index is useable
SET enable_seqscan = off;
SET enable_bitmapscan = off;

EXPLAIN (VERBOSE, COSTS OFF) SELECT * FROM test_index_schema_2.test_table_2 WHERE name = 'cluster_50';
EXPLAIN (VERBOSE, COSTS OFF) SELECT * FROM test_index_schema_2.test_table_2 WHERE id = 50;

RESET enable_seqscan;
RESET enable_bitmapscan;

-- IF EXISTS
ALTER INDEX test_index_schema_2.idx_not_exist SET SCHEMA test_index_schema_2;
ALTER INDEX IF EXISTS test_index_schema_2.idx_not_exist SET SCHEMA test_index_schema_2;

--
-- pg_get_indexdef
--
SELECT pg_get_indexdef('test_index_schema_2.idx_test_cluster_ok'::regclass);
SELECT pg_get_indexdef('test_index_schema_2.idx_test_cluster_schema'::regclass);
SELECT pg_get_indexdef('test_index_schema_2.idx_test_cluster_ok'::regclass, 0, true);
SELECT pg_get_indexdef('test_index_schema_2.idx_test_cluster_schema'::regclass, 0, true);

--
-- CREATE TABLE LIKE also include index schema.
--
CREATE TABLE public.test_table_3(
    id int primary key,
    name text
);

CREATE INDEX same_idxname ON public.test_table_3 (id);
CREATE INDEX test_index_schema_2.same_idxname ON public.test_table_3 (name);
CREATE TABLE public.test_table_4 (LIKE public.test_table_3 INCLUDING INDEXES);

SELECT n.nspname AS schema_name, c.relname AS index_name
FROM pg_class c
JOIN pg_namespace n ON c.relnamespace = n.oid
JOIN pg_index i ON c.oid = i.indexrelid
WHERE i.indrelid = 'public.test_table_3'::regclass
ORDER BY index_name, schema_name;

SELECT n.nspname AS schema_name, c.relname AS index_name
FROM pg_class c
JOIN pg_namespace n ON c.relnamespace = n.oid
JOIN pg_index i ON c.oid = i.indexrelid
WHERE i.indrelid = 'public.test_table_4'::regclass
ORDER BY index_name, schema_name;

DROP TABLE public.test_table_3 CASCADE;
DROP TABLE public.test_table_4 CASCADE;

--
-- cleanup
--
DROP TABLE test_index_schema_2.test_table_2 CASCADE;

DECLARE
    name text;
BEGIN
    select into name 10::regrole;
    execute format('set session authorization %s;', name);
END;
/

DROP SCHEMA test_index_schema CASCADE;
DROP SCHEMA test_index_schema_2 CASCADE;
DROP USER test_index_user;