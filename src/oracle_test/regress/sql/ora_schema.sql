-- test for oracle schema
SET ivorysql.enable_ora_user_schema = on;

--
-- user management
--

-- create user
CREATE SCHEMA sc_u2;
CREATE USER sc_u1;
CREATE USER sc_u2; -- error
CREATE TABLE sc_u1.t1(id int);
\dn sc_u1

-- alter user
ALTER USER sc_u1 RENAME TO sc_u11;
\dn sc_u11
\dn sc_u1

-- drop user
DROP USER sc_u11; --error
DROP SCHEMA sc_u1 CASCADE;
DROP USER sc_u11;

-- clean
DROP SCHEMA sc_u2;


---
--- schema management
---

CREATE USER sc_u1;
CREATE SCHEMA AUTHORIZATION sc_u1 CREATE TABLE t1(id int) CREATE VIEW vt1 AS SELECT * FROM t1;
DROP SCHEMA sc_u1 CASCADE;
CREATE SCHEMA AUTHORIZATION sc_u1 CREATE TABLE t1(id int) CREATE VIEW vt1 AS SELECT * FROM t1;

-- clean
DROP SCHEMA sc_u1 CASCADE;
DROP USER sc_u1;


--
-- object management
--

CREATE USER sc_pub SUPERUSER;
DROP SCHEMA sc_pub;
CREATE USER sc_u1;
SET SESSION AUTHORIZATION sc_pub;
SET ivorysql.enable_ora_user_schema = on;

-- create table
CREATE TABLE obj_t1(id int, s int);
CREATE TABLE sc_u1.obj_t1(id int, s int);

-- create sequence
CREATE TABLE sc_u1.obj_t2(id int, s serial);
CREATE SEQUENCE obj_seq1 OWNED BY obj_t1.s;
CREATE SEQUENCE obj_seq1 OWNED BY sc_u1.obj_t1.s;
CREATE SEQUENCE sc_u1.obj_seq1;

-- create view
CREATE VIEW obj_v1 AS SELECT * FROM sc_u1.obj_t1;
CREATE VIEW sc_u1.obj_v1 AS SELECT * FROM sc_u1.obj_t1;
CREATE FORCE VIEW obj_fv1 AS SELECT * FROM sc_u1.xxx;
CREATE FORCE VIEW sc_u1.obj_fv1 AS SELECT * FROM xxx;
CREATE MATERIALIZED VIEW sc_mv1 AS SELECT * FROM sc_u1.obj_t1;
CREATE MATERIALIZED VIEW sc_u1.sc_mv1 AS SELECT * FROM obj_t1;

-- create index
CREATE INDEX obj_t1_index ON obj_t1(id);
CREATE INDEX obj_t1_index ON sc_u1.obj_t1(id);

-- check relation object
SELECT relnamespace::regnamespace,relname,relowner::regrole FROM pg_class WHERE relname LIKE 'obj\_%' ESCAPE '\' ORDER BY relnamespace,relname;

-- create type
CREATE TYPE obj_type1;
CREATE TYPE sc_u1.obj_type1;
CREATE TYPE obj_type2 AS ENUM ('red', 'orange', 'yellow', 'green', 'blue', 'purple');
CREATE TYPE sc_u1.obj_type2 AS ENUM ('red', 'orange', 'yellow', 'green', 'blue', 'purple');
CREATE TYPE obj_type3 AS RANGE (subtype=text);
CREATE TYPE sc_u1.obj_type3 AS RANGE (subtype=text);
CREATE DOMAIN obj_type4 int4;
CREATE DOMAIN sc_u1.obj_type4 int4;
CREATE TYPE obj_type5 AS (a int, b text);
CREATE TYPE sc_u1.obj_type5 AS (a int, b text);

-- check type object
SELECT typnamespace::regnamespace,typname,typowner::regrole FROM pg_type WHERE typname LIKE 'obj\_type%' ESCAPE '\' ORDER BY typnamespace,typname;

-- create function
CREATE FUNCTION obj_func1() RETURNS void LANGUAGE SQL AS 'SELECT 1';
/
CREATE FUNCTION sc_u1.obj_func1() RETURNS void LANGUAGE SQL AS 'SELECT 1';
/

-- create procedure
CREATE PROCEDURE obj_func2()  LANGUAGE SQL AS 'SELECT 1';
/
CREATE PROCEDURE sc_u1.obj_func2() LANGUAGE SQL AS 'SELECT 1';
/

-- create window function
CREATE OR REPLACE FUNCTION obj_func3(a int) RETURNS int LANGUAGE SQL WINDOW AS 'SELECT $1';
/
CREATE OR REPLACE FUNCTION sc_u1.obj_func3(a int) RETURNS int LANGUAGE SQL WINDOW AS 'SELECT $1';
/

-- create aggregate
CREATE AGGREGATE obj_func4 (sfunc1 = int4pl, basetype = int4, stype1 = int4, initcond1 = '0');
CREATE AGGREGATE sc_u1.obj_func4 (sfunc1 = int4pl, basetype = int4, stype1 = int4, initcond1 = '0');

-- check function object
SELECT pronamespace::regnamespace,proname,proowner::regrole,prokind FROM pg_proc WHERE proname LIKE 'obj\_func%' ESCAPE '\' ORDER BY pronamespace,proname;

-- create oprator
CREATE OPERATOR @@@@@@@ (rightarg = int8, procedure = factorial);
CREATE OPERATOR sc_u1.@@@@@@@ (rightarg = int8, procedure = factorial);
CREATE OPERATOR FAMILY obj_opf1 USING hash;
CREATE OPERATOR FAMILY sc_u1.obj_opf1 USING hash;
CREATE OPERATOR CLASS obj_opc1 FOR TYPE uuid USING hash AS STORAGE uuid;
CREATE OPERATOR CLASS sc_u1.obj_opc1 FOR TYPE uuid USING hash AS STORAGE uuid;

-- check operator object
SELECT oprnamespace::regnamespace,oprname,oprowner::regrole FROM pg_operator WHERE oprname='@@@@@@@' ORDER BY oprnamespace,oprname;
SELECT opfnamespace::regnamespace,opfname,opfowner::regrole FROM pg_opfamily WHERE opfname='obj_opf1' ORDER BY opfnamespace,opfname;
SELECT opcnamespace::regnamespace,opcname,opcowner::regrole FROM pg_opclass WHERE opcname='obj_opc1' ORDER BY opcnamespace,opcname;

-- create package
CREATE PACKAGE obj_pack1 AS
END;
/
CREATE PACKAGE BODY obj_pack1 AS
END;
/
CREATE PACKAGE sc_u1.obj_pack1 AS
END;
/
CREATE PACKAGE BODY sc_u1.obj_pack1 AS
END;
/

-- check package object
SELECT pkgnamespace::regnamespace,pkgname,pkgowner::regrole FROM pg_package WHERE pkgname='obj_pack1' ORDER BY pkgnamespace,pkgname;

-- ceate text search object
CREATE TEXT SEARCH CONFIGURATION obj_tsc1 (PARSER = default);
CREATE TEXT SEARCH CONFIGURATION sc_u1.obj_tsc1 (PARSER = default);
CREATE TEXT SEARCH DICTIONARY obj_tsd1 (TEMPLATE = simple);
CREATE TEXT SEARCH DICTIONARY sc_u1.obj_tsd1 (TEMPLATE = simple);

-- check text search object
SELECT cfgnamespace::regnamespace,cfgname,cfgowner::regrole FROM pg_ts_config WHERE cfgname='obj_tsc1' ORDER BY cfgnamespace,cfgname;
SELECT dictnamespace::regnamespace,dictname,dictowner::regrole FROM pg_ts_dict WHERE dictname='obj_tsd1' ORDER BY dictnamespace,dictname;

-- ceate collation
CREATE COLLATION obj_coll1 (LOCALE = 'C');
CREATE COLLATION sc_u1.obj_coll1 (LOCALE = 'C');
SELECT collnamespace::regnamespace,collname,collowner::regrole FROM pg_collation WHERE collname='obj_coll1' ORDER BY collnamespace,collname;

-- create conversion
CREATE CONVERSION obj_conv1 FOR 'LATIN1' TO 'UTF8' FROM iso8859_1_to_utf8;
CREATE CONVERSION sc_u1.obj_conv1 FOR 'LATIN1' TO 'UTF8' FROM iso8859_1_to_utf8;
SELECT connamespace::regnamespace,conname,conowner::regrole FROM pg_conversion WHERE conname='obj_conv1' ORDER BY connamespace,conname;

-- create extension
--CREATE EXTENSION test_bloomfilter with schema sc_u1;
--SELECT extnamespace::regnamespace,extname FROM pg_extension WHERE extname='test_bloomfilter';
--DROP EXTENSION test_bloomfilter;
--CREATE EXTENSION test_bloomfilter with schema public;
--SELECT extnamespace::regnamespace,extname FROM pg_extension WHERE extname='test_bloomfilter';

-- create statisitc ext
CREATE STATISTICS obj_stat1 ON ( id + 1 ) FROM obj_t1;
SELECT stxnamespace::regnamespace,stxname,stxowner::regrole FROM pg_statistic_ext WHERE stxname='obj_stat1' ORDER BY stxnamespace,stxname;
DROP STATISTICS obj_stat1;
CREATE STATISTICS sc_u1.obj_stat1 ON ( id + 1 ) FROM obj_t1;
SELECT stxnamespace::regnamespace,stxname,stxowner::regrole FROM pg_statistic_ext WHERE stxname='obj_stat1' ORDER BY stxnamespace,stxname;

-- temp object
CREATE TEMP TABLE obj_t_t1(id int, s serial);
CREATE TEMP VIEW obj_t_v1 AS SELECT * FROM obj_t1;
CREATE TEMP SEQUENCE obj_t_seq1;
SELECT relname FROM pg_class
    WHERE relname LIKE 'obj\_t\_%' ESCAPE '\'
    AND relnamespace IN (SELECT oid FROM pg_namespace WHERE nspname LIKE 'pg_temp%')
    ORDER BY relname;

-- clean
RESET SESSION AUTHORIZATION;
DROP OWNED BY sc_u1 CASCADE;
DROP USER sc_u1;
DROP OWNED BY sc_pub CASCADE;
DROP USER sc_pub;
