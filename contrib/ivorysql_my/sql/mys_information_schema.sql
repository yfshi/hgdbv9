SET mysql.enable_mysql_information_schema TO on;

------------------------------------------------------------
-- 1. Test for information_schema.table_privileges
------------------------------------------------------------
CREATE SCHEMA test_schema;
CREATE TABLE test_schema.tbl_priv (id SERIAL PRIMARY KEY, val TEXT);
CREATE ROLE test_user LOGIN PASSWORD '123';
GRANT SELECT, UPDATE ON test_schema.tbl_priv TO test_user;

SELECT grantee, table_catalog, table_schema, table_name, privilege_type FROM
information_schema.table_privileges
WHERE table_schema = 'test_schema' AND table_name = 'tbl_priv' AND grantee like 'test_user%'
ORDER BY grantee, privilege_type;
DROP TABLE test_schema.tbl_priv;

------------------------------------------------------------
-- 2. Test for information_schema.columns
------------------------------------------------------------
CREATE TABLE test_schema.sample_table (
    id SERIAL PRIMARY KEY,
    code VARCHAR(10) NOT NULL UNIQUE,
    ttl INTEGER DEFAULT 3600,
    created_at TIMESTAMP NOT NULL DEFAULT now(),
    full_name TEXT GENERATED ALWAYS AS (code || '_x') STORED,
    description TEXT
);
COMMENT ON COLUMN test_schema.sample_table.description IS 'test';
GRANT SELECT(id),UPDATE(description) ON test_schema.sample_table TO test_user;
SET ROLE test_user;
SELECT * FROM information_schema.columns
WHERE table_schema = 'test_schema' AND table_name = 'sample_table'
ORDER BY ordinal_position;
RESET ROLE;
DROP TABLE test_schema.sample_table;

-- # Begin - BUG#8916

DROP TABLE IF EXISTS test_schema.test_all_types;

CREATE TABLE test_schema.test_all_types
(
    /* signed integer */
    c_tiny          TINYINT,
    c_tiny_m        TINYINT(2),

    c_small         SMALLINT,
    c_small_m       SMALLINT(4),

    c_medium        MEDIUMINT,
    c_medium_m      MEDIUMINT(6),

    c_int           INT,
    c_int_m         INT(7),

    c_big           BIGINT,
    c_big_m         BIGINT(12),

    /* unsigned integer uint1/uint2/uint3/uint4/uint8 */
    c_uint1         TINYINT UNSIGNED,
    c_uint1_m       TINYINT(2) UNSIGNED,

    c_uint2         SMALLINT UNSIGNED,
    c_uint2_m       SMALLINT(4) UNSIGNED,

    c_uint3         MEDIUMINT UNSIGNED,
    c_uint3_m       MEDIUMINT(6) UNSIGNED,

    c_uint4         INT UNSIGNED,
    c_uint4_m       INT(7) UNSIGNED,

    c_uint8         BIGINT UNSIGNED,
    c_uint8_m       BIGINT(12) UNSIGNED,

    /* bit */
    c_bit           BIT,
    c_bit_m         BIT(8),

    /* character */
    c_char          CHAR(10),
    c_varchar       VARCHAR(100),

    /* exact numeric */
    c_decimal       DECIMAL,
    c_decimal_ps    DECIMAL(10,2),

    /* approximate numeric */
    c_float         FLOAT,
    c_double        DOUBLE,

    /* date/time */
    c_date          DATE,
    c_datetime      DATETIME,
    c_datetime_fsp  DATETIME(3),
    c_timestamp     TIMESTAMP,
    c_timestamp_fsp TIMESTAMP(6),
    c_time          TIME,
    c_time_fsp      TIME(4),

    /* year */
    c_year          YEAR,
    c_year4         YEAR(4),

    /* enum */
    c_enum          ENUM('a','b','c')
);

SELECT
    COLUMN_NAME,
    DATA_TYPE,
    COLUMN_TYPE
FROM INFORMATION_SCHEMA.COLUMNS
WHERE TABLE_SCHEMA = 'test_schema'
    AND TABLE_NAME = 'test_all_types'
ORDER BY ORDINAL_POSITION;

drop table test_schema.test_all_types;
-- # End - BUG#8916
------------------------------------------------------------
-- 3. Test for information_schema.key_column_usage
------------------------------------------------------------
CREATE TABLE test_schema.ref_tbl (id INTEGER PRIMARY KEY, name TEXT);
CREATE TABLE test_schema.main_tbl (
    col1 INTEGER PRIMARY KEY,
    col2 TEXT UNIQUE,
    col3 INTEGER,
    FOREIGN KEY (col3) REFERENCES test_schema.ref_tbl(id)
);

SELECT constraint_catalog, constraint_schema, constraint_name,
       table_catalog, table_schema, table_name, column_name,
       ordinal_position, position_in_unique_constraint,
       referenced_table_schema, referenced_table_name, referenced_column_name
FROM information_schema.key_column_usage
WHERE table_schema = 'test_schema'
ORDER BY constraint_name, ordinal_position, table_name;
DROP TABLE test_schema.main_tbl cascade;
DROP TABLE test_schema.ref_tbl cascade;

------------------------------------------------------------
-- 4. Test for ivymy.tables
------------------------------------------------------------
CREATE SCHEMA test_schema_ivymy;
CREATE TABLE test_schema_ivymy.base_tbl (id SERIAL PRIMARY KEY, val TEXT);
COMMENT ON TABLE test_schema_ivymy.base_tbl IS 'Base Table Test';
CREATE VIEW test_schema_ivymy.vw_tbl AS SELECT id, val FROM test_schema_ivymy.base_tbl;

CREATE TABLE test_schema_ivymy.part_tbl (id INT, note TEXT) PARTITION BY RANGE (id);
CREATE TABLE test_schema_ivymy.part_tbl_p1 PARTITION OF test_schema_ivymy.part_tbl
    FOR VALUES FROM (1) TO (100);

SELECT table_schema, table_name, table_type, engine, table_rows,
       avg_row_length, data_length, max_data_length, index_length,
       data_free, auto_increment, table_comment
FROM information_schema.tables
WHERE table_schema = 'test_schema_ivymy'
ORDER BY table_name;

-- test auto_increment
CREATE TABLE test_schema_ivymy.t1(
c1 INT DEFAULT 12 COMMENT 'column1',
c2 INT NULL COMMENT 'column2',
c3 INT NOT NULL COMMENT 'column3',
c4 VARCHAR(255) CHARACTER SET utf8 NOT NULL DEFAULT 'a',
c5 VARCHAR(255) COLLATE utf8_unicode_ci NULL DEFAULT 'b',
c6 VARCHAR(255),
c7 int auto_increment PRIMARY KEY
)auto_increment = 77 COLLATE latin1_bin COMMENT 'dddd' ;
\d+ test_schema_ivymy.t1
select obj_description(oid) from pg_class where relname = 't1';
SHOW CREATE TABLE test_schema_ivymy.t1;
SELECT table_schema, table_name, table_type, engine, auto_increment, table_comment
FROM information_schema.tables WHERE table_schema = 'test_schema_ivymy' and table_name = 't1' ORDER BY table_name;
drop table test_schema_ivymy.t1;

CREATE TABLE test_schema_ivymy.test_show (
id INT NOT NULL AUTO_INCREMENT,
name VARCHAR(50),
PRIMARY KEY (id)
)AUTO_INCREMENT=9;
SHOW CREATE TABLE test_schema_ivymy.test_show;
ALTER TABLE test_schema_ivymy.test_show AUTO_INCREMENT = 0;
SHOW CREATE TABLE test_schema_ivymy.test_show;
SELECT table_schema, table_name, table_type, engine, auto_increment, table_comment
FROM information_schema.tables WHERE table_schema = 'test_schema_ivymy' and table_name = 'test_show' ORDER BY table_name;
INSERT INTO test_schema_ivymy.test_show (name) VALUES ('test1');
SHOW CREATE TABLE test_schema_ivymy.test_show;
SELECT table_schema, table_name, table_type, engine, auto_increment, table_comment
FROM information_schema.tables WHERE table_schema = 'test_schema_ivymy' and table_name = 'test_show' ORDER BY table_name;
ALTER TABLE test_schema_ivymy.test_show AUTO_INCREMENT = 0;
SHOW CREATE TABLE test_schema_ivymy.test_show;
SELECT table_schema, table_name, table_type, engine, auto_increment, table_comment
FROM information_schema.tables WHERE table_schema = 'test_schema_ivymy' and table_name = 'test_show' ORDER BY table_name;
INSERT INTO test_schema_ivymy.test_show (name) VALUES ('test2');
SHOW CREATE TABLE test_schema_ivymy.test_show;
SELECT table_schema, table_name, table_type, engine, auto_increment, table_comment
FROM information_schema.tables WHERE table_schema = 'test_schema_ivymy' and table_name = 'test_show' ORDER BY table_name;
ALTER TABLE test_schema_ivymy.test_show AUTO_INCREMENT = 5;
SHOW CREATE TABLE test_schema_ivymy.test_show;
SELECT table_schema, table_name, table_type, engine, auto_increment, table_comment
FROM information_schema.tables WHERE table_schema = 'test_schema_ivymy' and table_name = 'test_show' ORDER BY table_name;
INSERT INTO test_schema_ivymy.test_show (name) VALUES ('test3');
SHOW CREATE TABLE test_schema_ivymy.test_show;
SELECT table_schema, table_name, table_type, engine, auto_increment, table_comment
FROM information_schema.tables WHERE table_schema = 'test_schema_ivymy' and table_name = 'test_show' ORDER BY table_name;
ALTER TABLE test_schema_ivymy.test_show AUTO_INCREMENT = 100;
SHOW CREATE TABLE test_schema_ivymy.test_show;
SELECT table_schema, table_name, table_type, engine, auto_increment, table_comment
FROM information_schema.tables WHERE table_schema = 'test_schema_ivymy' and table_name = 'test_show' ORDER BY table_name;
INSERT INTO test_schema_ivymy.test_show (name) VALUES ('test4');
SHOW CREATE TABLE test_schema_ivymy.test_show;
SELECT table_schema, table_name, table_type, engine, auto_increment, table_comment
FROM information_schema.tables WHERE table_schema = 'test_schema_ivymy' and table_name = 'test_show' ORDER BY table_name;
SELECT * FROM test_schema_ivymy.test_show order by id;
drop TABLE test_schema_ivymy.test_show;

CREATE TABLE test_schema_ivymy.test_show (
id INT NOT NULL AUTO_INCREMENT,
name VARCHAR(50),
PRIMARY KEY (id)
)AUTO_INCREMENT=9;
SHOW CREATE TABLE test_schema_ivymy.test_show;
SELECT table_schema, table_name, table_type, engine, auto_increment, table_comment
FROM information_schema.tables WHERE table_schema = 'test_schema_ivymy' and table_name = 'test_show' ORDER BY table_name;
INSERT INTO test_schema_ivymy.test_show (name) VALUES ('test1');
SHOW CREATE TABLE test_schema_ivymy.test_show;
SELECT table_schema, table_name, table_type, engine, auto_increment, table_comment
FROM information_schema.tables WHERE table_schema = 'test_schema_ivymy' and table_name = 'test_show' ORDER BY table_name;
ALTER TABLE test_schema_ivymy.test_show AUTO_INCREMENT = 0;
SHOW CREATE TABLE test_schema_ivymy.test_show;
SELECT table_schema, table_name, table_type, engine, auto_increment, table_comment
FROM information_schema.tables WHERE table_schema = 'test_schema_ivymy' and table_name = 'test_show' ORDER BY table_name;
INSERT INTO test_schema_ivymy.test_show (name) VALUES ('test2');
SHOW CREATE TABLE test_schema_ivymy.test_show;
SELECT table_schema, table_name, table_type, engine, auto_increment, table_comment
FROM information_schema.tables WHERE table_schema = 'test_schema_ivymy' and table_name = 'test_show' ORDER BY table_name;
ALTER TABLE test_schema_ivymy.test_show AUTO_INCREMENT = 5;
SHOW CREATE TABLE test_schema_ivymy.test_show;
SELECT table_schema, table_name, table_type, engine, auto_increment, table_comment
FROM information_schema.tables WHERE table_schema = 'test_schema_ivymy' and table_name = 'test_show' ORDER BY table_name;
INSERT INTO test_schema_ivymy.test_show (name) VALUES ('test3');
SHOW CREATE TABLE test_schema_ivymy.test_show;
SELECT table_schema, table_name, table_type, engine, auto_increment, table_comment
FROM information_schema.tables WHERE table_schema = 'test_schema_ivymy' and table_name = 'test_show' ORDER BY table_name;
ALTER TABLE test_schema_ivymy.test_show AUTO_INCREMENT = 100;
SHOW CREATE TABLE test_schema_ivymy.test_show;
SELECT table_schema, table_name, table_type, engine, auto_increment, table_comment
FROM information_schema.tables WHERE table_schema = 'test_schema_ivymy' and table_name = 'test_show' ORDER BY table_name;
INSERT INTO test_schema_ivymy.test_show (name) VALUES ('test4');
SHOW CREATE TABLE test_schema_ivymy.test_show;
SELECT table_schema, table_name, table_type, engine, auto_increment, table_comment
FROM information_schema.tables WHERE table_schema = 'test_schema_ivymy' and table_name = 'test_show' ORDER BY table_name;
SELECT * FROM test_schema_ivymy.test_show order by id;
drop TABLE test_schema_ivymy.test_show;

------------------------------------------------------------
-- 5. Test for information_schema.partitions
------------------------------------------------------------
CREATE TABLE test_schema.list_part (region text, val int)
PARTITION BY LIST (region);
CREATE TABLE test_schema.list_part_asia PARTITION OF test_schema.list_part
    FOR VALUES IN ('ASIA') PARTITION BY HASH (val);
CREATE TABLE test_schema.list_part_asia_0 PARTITION OF test_schema.list_part_asia
    FOR VALUES WITH (modulus 2, remainder 0);
CREATE TABLE test_schema.list_part_asia_1 PARTITION OF test_schema.list_part_asia
    FOR VALUES WITH (modulus 2, remainder 1) PARTITION BY HASH (val);
CREATE TABLE test_schema.list_part_europe PARTITION OF test_schema.list_part
    FOR VALUES IN ('EUROPE');

SELECT table_schema, table_name, partition_name, subpartition_name,
       partition_method, partition_expression, subpartition_method, subpartition_expression, partition_description,
       table_rows, avg_row_length, data_length, index_length, partition_comment
FROM information_schema.partitions
WHERE table_schema = 'test_schema'
ORDER BY table_name, partition_name, subpartition_name;

------------------------------------------------------------
-- 6. Test for information_schema.column_privileges
------------------------------------------------------------
CREATE TABLE test_schema.test_table (id INT, name TEXT);
CREATE ROLE user_a LOGIN;
CREATE ROLE user_b LOGIN;
GRANT SELECT (id), UPDATE (name) ON test_schema.test_table TO user_a WITH GRANT OPTION;
GRANT SELECT (name) ON test_schema.test_table TO user_b;

SELECT grantee, table_schema, table_name, column_name, privilege_type, is_grantable
FROM information_schema.column_privileges
WHERE table_schema = 'test_schema' AND table_name = 'test_table' AND grantee like '%user_%'
order by is_grantable , column_name;

------------------------------------------------------------
-- 7. Test for information_schema.views
------------------------------------------------------------
CREATE SCHEMA test_schema_views;
CREATE TABLE test_schema_views.base_tbl (id int, val text);
CREATE VIEW test_schema_views.simple_view AS SELECT id, val FROM test_schema_views.base_tbl;
CREATE VIEW test_schema_views.chk_view AS
SELECT id, val FROM test_schema_views.base_tbl WHERE id > 0 WITH LOCAL CHECK OPTION;

SELECT table_schema, table_name, view_definition, check_option, is_updatable
FROM information_schema.views
WHERE table_schema = 'test_schema_views'
ORDER BY table_name;

------------------------------------------------------------
-- 8. Test for information_schema.routines
------------------------------------------------------------
CREATE SCHEMA test_schema_routines;
CREATE FUNCTION test_schema_routines.add_one(x int)
RETURNS int LANGUAGE sql IMMUTABLE AS $$ SELECT x + 1; $$;
CREATE PROCEDURE test_schema_routines.do_nothing() LANGUAGE sql AS $$ SELECT 1; $$;

SELECT routine_schema, routine_name, routine_type, data_type,
       routine_definition, is_deterministic, sql_data_access
FROM information_schema.routines
WHERE routine_schema = 'test_schema_routines'
ORDER BY routine_name;

------------------------------------------------------------
-- 9. Test for information_schema.triggers
------------------------------------------------------------
CREATE SCHEMA test_schema_triggers;
CREATE TABLE test_schema_triggers.trig_tbl (id int, val text);
CREATE FUNCTION test_schema_triggers.trig_func()
RETURNS trigger LANGUAGE plpgsql AS $$
BEGIN
  NEW.val := upper(NEW.val);
  RETURN NEW;
END;$$;
CREATE TRIGGER before_ins BEFORE INSERT ON test_schema_triggers.trig_tbl
FOR EACH ROW EXECUTE FUNCTION test_schema_triggers.trig_func();

SELECT trigger_schema, trigger_name, event_object_table, event_manipulation,
       action_timing, action_orientation, action_statement
FROM information_schema.triggers
WHERE trigger_schema = 'test_schema_triggers';

------------------------------------------------------------
-- 10. Test for information_schema.table_constraints
------------------------------------------------------------
CREATE SCHEMA test_schema_constraints;
CREATE TABLE test_schema_constraints.cons_tbl (
    id INT PRIMARY KEY,
    code TEXT UNIQUE,
    val INT NOT NULL,
    ref_id INT,
    CONSTRAINT fk_ref FOREIGN KEY (ref_id) REFERENCES test_schema_constraints.cons_tbl(id)
);

SELECT constraint_schema, table_schema,
       table_name, constraint_type
FROM information_schema.table_constraints
WHERE table_schema = 'test_schema_constraints'
ORDER BY constraint_type;

------------------------------------------------------------
-- 11. Test for information_schema.schemata
------------------------------------------------------------
CREATE SCHEMA test_schema_schemata;

SELECT catalog_name, schema_name, default_character_set_name
FROM information_schema.schemata
WHERE schema_name = 'test_schema_schemata';

------------------------------------------------------------
-- 12. Test for information_schema.schema_privileges
------------------------------------------------------------
CREATE SCHEMA test_schema_schema_privs;
CREATE ROLE schema_user LOGIN;
GRANT USAGE ON SCHEMA test_schema_schema_privs TO schema_user;
GRANT CREATE ON SCHEMA test_schema_schema_privs TO schema_user;

SELECT grantee, table_catalog, table_schema, privilege_type, is_grantable
FROM information_schema.schema_privileges
WHERE table_schema = 'test_schema_schema_privs' AND grantee like '%schema_user%'
ORDER BY privilege_type;

------------------------------------------------------------
-- 13. Test for information_schema.statistics
------------------------------------------------------------
CREATE SCHEMA test_schema_stats;
CREATE TABLE test_schema_stats.stats_tbl (
    id SERIAL PRIMARY KEY,
    name TEXT NOT NULL,
    code VARCHAR(100)
);
CREATE UNIQUE INDEX idx_stats_name ON test_schema_stats.stats_tbl(name);
CREATE INDEX idx_stats_code_prefix ON test_schema_stats.stats_tbl((left(code, 10)));

SELECT table_schema, table_name, index_name, seq_in_index,
       column_name, "collation", cardinality, sub_part, index_type
FROM information_schema.statistics
WHERE table_schema = 'test_schema_stats'
ORDER BY index_name, seq_in_index;

------------------------------------------------------------
-- 14. Test for mysql.user
------------------------------------------------------------
CREATE USER testuser WITH CONNECTION LIMIT 20;
SELECT * FROM mysql.user WHERE "User" = 'testuser';

------------------------------------------------------------
-- 15. Test for mys_information_schema.keywords
------------------------------------------------------------
SELECT "WORD", "RESERVED"
FROM information_schema.keywords
WHERE "WORD" IN ('SELECT', 'FROM', 'COLUMN', 'SHOW')
ORDER BY "WORD";

------------------------------------------------------------
-- 16. Test for mys_information_schema.referential_constraints
------------------------------------------------------------
CREATE SCHEMA test_schema_refcons;
CREATE TABLE test_schema_refcons.parent (
    id INT PRIMARY KEY,
    email VARCHAR(100),
    CONSTRAINT uk_email UNIQUE (email)
);
CREATE TABLE test_schema_refcons.child (
    id INT PRIMARY KEY,
    parent_id INT,
    user_email VARCHAR(100),
    CONSTRAINT fk_parent FOREIGN KEY (parent_id)
        REFERENCES test_schema_refcons.parent(id)
        ON UPDATE CASCADE ON DELETE SET NULL,
    CONSTRAINT fk_uemail FOREIGN KEY (user_email)
        REFERENCES test_schema_refcons.parent(email)
);
SELECT *
FROM information_schema.referential_constraints
WHERE "CONSTRAINT_SCHEMA" = 'test_schema_refcons'
ORDER BY "CONSTRAINT_NAME";

DROP SCHEMA test_schema_refcons CASCADE;

------------------------------------------------------------
-- 17. Test for mys_information_schema.parameters
------------------------------------------------------------
CREATE SCHEMA test_schema_params;

CREATE OR REPLACE FUNCTION test_schema_params.f_all_types(
    p_char        CHAR(10),
    p_varchar     VARCHAR(20),
    p_binary      BINARY(8),
    p_varbinary   VARBINARY(16),
    p_tinytext    TINYTEXT,
    p_text        TEXT,
    p_mediumtext  MEDIUMTEXT,
    p_longtext    LONGTEXT,
    p_tinyblob    TINYBLOB,
    p_blob        BLOB,
    p_mediumblob  MEDIUMBLOB,
    p_longblob    LONGBLOB,
    p_enum        ENUM('a','b','c'),
    p_json        JSON,
    p_tinyint     TINYINT,
    p_smallint    SMALLINT,
    p_int         INT,
    p_bigint      BIGINT,
    p_decimal     DECIMAL(10,2),
    p_float       FLOAT,
    p_double      DOUBLE,
    p_date        DATE,
    p_datetime    DATETIME,
    p_timestamp   TIMESTAMP,
    p_time        TIME,
    p_year        YEAR,
    p_bit         BIT(8),
    p_point       POINT
)
RETURNS INT
LANGUAGE sql
AS $$
    SELECT 1;
$$;

SELECT "SPECIFIC_CATALOG", "SPECIFIC_SCHEMA", "SPECIFIC_NAME", "ORDINAL_POSITION",
       "PARAMETER_MODE", "PARAMETER_NAME", "DATA_TYPE",
       "CHARACTER_MAXIMUM_LENGTH", "CHARACTER_OCTET_LENGTH", "NUMERIC_PRECISION", 
	   "NUMERIC_SCALE", "DATETIME_PRECISION", "DTD_IDENTIFIER", "ROUTINE_TYPE"
FROM information_schema.parameters
WHERE "SPECIFIC_SCHEMA" = 'test_schema_params'
  AND "SPECIFIC_NAME"   = 'f_all_types'
ORDER BY "ORDINAL_POSITION";

SELECT
    "PARAMETER_NAME",
    "CHARACTER_SET_NAME" = pg_catalog.getdatabaseencoding() AS charset_matches_db,
    "COLLATION_NAME" =
    (
        SELECT "Default collation"
        FROM show_mysql_charsets()
        WHERE lower("Charset") =
              lower(pg_catalog.getdatabaseencoding())
    ) AS has_collation
FROM information_schema.parameters
WHERE "SPECIFIC_SCHEMA" = 'test_schema_params'
  AND"SPECIFIC_NAME" = 'f_all_types'
ORDER BY "ORDINAL_POSITION";

-- Test PROCEDURE: verify IN/OUT/INOUT modes and no ORDINAL_POSITION=0 row
CREATE OR REPLACE PROCEDURE test_schema_params.p_modes(
    IN    p_in_int       INT,
    IN    p_in_varchar   VARCHAR(50),
    OUT   p_out_text     TEXT,
    OUT   p_out_decimal  DECIMAL(10,2),
    INOUT p_inout_bigint BIGINT
)
LANGUAGE plpgsql
AS $$
BEGIN
    p_out_text := 'done';
    p_out_decimal := 0;
END;
$$;

SELECT "SPECIFIC_CATALOG", "SPECIFIC_SCHEMA", "SPECIFIC_NAME", "ORDINAL_POSITION",
       "PARAMETER_MODE", "PARAMETER_NAME", "DATA_TYPE",
       "CHARACTER_MAXIMUM_LENGTH", "CHARACTER_OCTET_LENGTH", "NUMERIC_PRECISION", 
	   "NUMERIC_SCALE", "DATETIME_PRECISION", "DTD_IDENTIFIER", "ROUTINE_TYPE"
FROM information_schema.parameters
WHERE "SPECIFIC_SCHEMA" = 'test_schema_params'
  AND "SPECIFIC_NAME"   = 'p_modes'
ORDER BY "ORDINAL_POSITION";

SET mysql.enable_mysql_information_schema TO off;

------------------------------------------------------------
-- clean
------------------------------------------------------------
DROP SCHEMA test_schema CASCADE;
DROP SCHEMA test_schema_ivymy CASCADE;
DROP SCHEMA test_schema_views CASCADE;
DROP SCHEMA test_schema_routines CASCADE;
DROP SCHEMA test_schema_triggers CASCADE;
DROP SCHEMA test_schema_constraints CASCADE;
DROP SCHEMA test_schema_schemata CASCADE;
DROP SCHEMA test_schema_schema_privs CASCADE;
DROP SCHEMA test_schema_stats CASCADE;
DROP ROLE IF EXISTS test_user;
DROP ROLE IF EXISTS user_a;
DROP ROLE IF EXISTS user_b;
DROP ROLE IF EXISTS schema_user;
DROP ROLE IF EXISTS testuser;
DROP SCHEMA test_schema_params CASCADE;
