create or replace function dbuser_function() RETURNS void as $$
declare
    name text;
begin
	select into name 10::regrole;
    execute format('set session authorization %s;',name);
	return;
end; $$ language plpgsql;
/
--
-- ALTER xxx OWNER TO
--
--

-- databaseuser

CREATE USER regress_database_test_user;

set session authorization regress_database_test_user;

CREATE TABLE regtest_user_table_2 (a int, b text);
CREATE TABLE regtest_user_ptable_2 (o int, p text) PARTITION BY RANGE (o);
CREATE TABLE regtest_user_ptable_2_ones PARTITION OF regtest_user_ptable_2 FOR VALUES FROM ('0') TO ('10');
CREATE SEQUENCE regtest_user_seq_2;
CREATE VIEW regtest_user_view_2 AS SELECT * FROM regtest_user_table_2 WHERE a > 0;
CREATE FUNCTION regtest_user_func_2() RETURNS integer AS $$SELECT 4$$ language sql;
/

CREATE AGGREGATE regtest_user_agg_2(int2) (SFUNC = int2_sum, STYPE = int8);

CREATE CONVERSION regtest_user_conv_2 FOR 'LATIN1' TO 'UTF8' FROM iso8859_1_to_utf8;
CREATE TABLE regtest_user_t2 (a INTEGER, b INTEGER);
CREATE STATISTICS regtest_user_stat_2 ON a, b FROM regtest_user_t2;


set ivorysql.compatible_mode to pg;
show ivorysql.compatible_mode;
select dbuser_function();
-- instanceuser
CREATE USER regress_instance_test_user;

set session authorization regress_instance_test_user;


CREATE TABLE regtest_user_table_1 (a int, b text);
CREATE TABLE regtest_user_ptable_1 (o int, p text) PARTITION BY RANGE (o);
CREATE TABLE regtest_user_ptable_1_ones PARTITION OF regtest_user_ptable_1 FOR VALUES FROM ('0') TO ('10');
CREATE SEQUENCE regtest_user_seq_1;
CREATE VIEW regtest_user_view_1 AS SELECT * FROM regtest_user_table_1 WHERE a > 0;
CREATE FUNCTION regtest_user_func_1() RETURNS integer AS $$SELECT 4$$ language sql;
/

CREATE AGGREGATE regtest_user_agg_1(int2) (SFUNC = int2_sum, STYPE = int8);

CREATE CONVERSION regtest_user_conv_1 FOR 'LATIN1' TO 'UTF8' FROM iso8859_1_to_utf8;
CREATE TABLE regtest_user_t1 (a INTEGER, b INTEGER);
CREATE STATISTICS regtest_user_stat_1 ON a, b FROM regtest_user_t1;

--
--  error test
-- 1 
ALTER TABLE regtest_user_table_1 OWNER TO regress_database_test_user;
ALTER TABLE regtest_user_ptable_1 OWNER TO regress_database_test_user;
ALTER TABLE regtest_user_ptable_1_ones OWNER TO regress_database_test_user;
ALTER SEQUENCE regtest_user_seq_1 OWNER TO regress_database_test_user;
ALTER VIEW regtest_user_view_1 OWNER TO regress_database_test_user;
ALTER FUNCTION regtest_user_func_1() OWNER TO regress_database_test_user;
ALTER AGGREGATE regtest_user_agg_1(int2) OWNER TO regress_database_test_user;
ALTER CONVERSION regtest_user_conv_1 OWNER TO regress_database_test_user;
ALTER STATISTICS regtest_user_stat_1 OWNER TO regress_database_test_user; 

--
--  success test
-- 2 
ALTER TABLE regtest_user_table_1 OWNER TO regress_instance_test_user;
ALTER TABLE regtest_user_ptable_1 OWNER TO regress_instance_test_user;
ALTER TABLE regtest_user_ptable_1_ones OWNER TO regress_instance_test_user;
ALTER SEQUENCE regtest_user_seq_1 OWNER TO regress_instance_test_user;
ALTER VIEW regtest_user_view_1 OWNER TO regress_instance_test_user;
ALTER FUNCTION regtest_user_func_1() OWNER TO regress_instance_test_user;
ALTER AGGREGATE regtest_user_agg_1(int2) OWNER TO regress_instance_test_user;
ALTER CONVERSION regtest_user_conv_1 OWNER TO regress_instance_test_user;
ALTER STATISTICS regtest_user_stat_1 OWNER TO regress_instance_test_user; 

--
--  error test
-- 3 
ALTER TABLE regtest_user_table_1 OWNER TO system;
ALTER TABLE regtest_user_ptable_1 OWNER TO system;
ALTER TABLE regtest_user_ptable_1_ones OWNER TO system;
ALTER SEQUENCE regtest_user_seq_1 OWNER TO system;
ALTER VIEW regtest_user_view_1 OWNER TO system;
ALTER FUNCTION regtest_user_func_1() OWNER TO system;
ALTER AGGREGATE regtest_user_agg_1(int2) OWNER TO system;
ALTER CONVERSION regtest_user_conv_1 OWNER TO system;
ALTER STATISTICS regtest_user_stat_1 OWNER TO system; 

--
--  error test
-- 4
ALTER TABLE regtest_user_table_1 OWNER TO databaseuser regress_database_test_user;
ALTER TABLE regtest_user_ptable_1 OWNER TO databaseuser regress_database_test_user;
ALTER TABLE regtest_user_ptable_1_ones OWNER TO databaseuser regress_database_test_user;
ALTER SEQUENCE regtest_user_seq_1 OWNER TO databaseuser regress_database_test_user;
ALTER VIEW regtest_user_view_1 OWNER TO databaseuser regress_database_test_user;
ALTER FUNCTION regtest_user_func_1() OWNER TO databaseuser regress_database_test_user;
ALTER AGGREGATE regtest_user_agg_1(int2) OWNER TO databaseuser regress_database_test_user;
ALTER CONVERSION regtest_user_conv_1 OWNER TO databaseuser regress_database_test_user;
ALTER STATISTICS regtest_user_stat_1 OWNER TO databaseuser regress_database_test_user; 


--
--  success test
-- 5 
ALTER TABLE regtest_user_table_1 OWNER TO regress_instance_test_user;
ALTER TABLE regtest_user_ptable_1 OWNER TO regress_instance_test_user;
ALTER TABLE regtest_user_ptable_1_ones OWNER TO regress_instance_test_user;
ALTER SEQUENCE regtest_user_seq_1 OWNER TO regress_instance_test_user;
ALTER VIEW regtest_user_view_1 OWNER TO regress_instance_test_user;
ALTER FUNCTION regtest_user_func_1() OWNER TO regress_instance_test_user;
ALTER AGGREGATE regtest_user_agg_1(int2) OWNER TO regress_instance_test_user;
ALTER CONVERSION regtest_user_conv_1 OWNER TO regress_instance_test_user;
ALTER STATISTICS regtest_user_stat_1 OWNER TO regress_instance_test_user; 

--
--  error test
-- 6
ALTER TABLE regtest_user_table_1 OWNER TO databaseuser system;
ALTER TABLE regtest_user_ptable_1 OWNER TO databaseuser system;
ALTER TABLE regtest_user_ptable_1_ones OWNER TO databaseuser system;
ALTER SEQUENCE regtest_user_seq_1 OWNER TO databaseuser system;
ALTER VIEW regtest_user_view_1 OWNER TO databaseuser system;
ALTER FUNCTION regtest_user_func_1() OWNER TO databaseuser system;
ALTER AGGREGATE regtest_user_agg_1(int2) OWNER TO databaseuser system;
ALTER CONVERSION regtest_user_conv_1 OWNER TO databaseuser system;
ALTER STATISTICS regtest_user_stat_1 OWNER TO databaseuser system; 

--
--  error test
-- 7
ALTER TABLE regtest_user_table_1 OWNER TO instanceuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_1 OWNER TO instanceuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_1_ones OWNER TO instanceuser regress_instance_test_user;
ALTER SEQUENCE regtest_user_seq_1 OWNER TO instanceuser regress_instance_test_user;
ALTER VIEW regtest_user_view_1 OWNER TO instanceuser regress_instance_test_user;
ALTER FUNCTION regtest_user_func_1() OWNER TO instanceuser regress_instance_test_user;
ALTER AGGREGATE regtest_user_agg_1(int2) OWNER TO instanceuser regress_instance_test_user;
ALTER CONVERSION regtest_user_conv_1 OWNER TO instanceuser regress_instance_test_user;
ALTER STATISTICS regtest_user_stat_1 OWNER TO instanceuser regress_instance_test_user; 

--
--  error test
-- 8
ALTER TABLE regtest_user_table_1 OWNER TO databaseuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_1 OWNER TO databaseuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_1_ones OWNER TO databaseuser regress_instance_test_user;
ALTER SEQUENCE regtest_user_seq_1 OWNER TO databaseuser regress_instance_test_user;
ALTER VIEW regtest_user_view_1 OWNER TO databaseuser regress_instance_test_user;
ALTER FUNCTION regtest_user_func_1() OWNER TO databaseuser regress_instance_test_user;
ALTER AGGREGATE regtest_user_agg_1(int2) OWNER TO databaseuser regress_instance_test_user;
ALTER CONVERSION regtest_user_conv_1 OWNER TO databaseuser regress_instance_test_user;
ALTER STATISTICS regtest_user_stat_1 OWNER TO databaseuser regress_instance_test_user; 


--
set ivorysql.grant_usertype to oracle;
show ivorysql.grant_usertype ;

--
--  error test
-- 9
ALTER TABLE regtest_user_table_1 OWNER TO system;
ALTER TABLE regtest_user_ptable_1 OWNER TO system;
ALTER TABLE regtest_user_ptable_1_ones OWNER TO system;
ALTER SEQUENCE regtest_user_seq_1 OWNER TO system;
ALTER VIEW regtest_user_view_1 OWNER TO system;
ALTER FUNCTION regtest_user_func_1() OWNER TO system;
ALTER AGGREGATE regtest_user_agg_1(int2) OWNER TO system;
ALTER CONVERSION regtest_user_conv_1 OWNER TO system;
ALTER STATISTICS regtest_user_stat_1 OWNER TO system; 

--
--  error test
-- 10
ALTER TABLE regtest_user_table_1 OWNER TO databaseuser system;
ALTER TABLE regtest_user_ptable_1 OWNER TO databaseuser system;
ALTER TABLE regtest_user_ptable_1_ones OWNER TO databaseuser system;
ALTER SEQUENCE regtest_user_seq_1 OWNER TO databaseuser system;
ALTER VIEW regtest_user_view_1 OWNER TO databaseuser system;
ALTER FUNCTION regtest_user_func_1() OWNER TO databaseuser system;
ALTER AGGREGATE regtest_user_agg_1(int2) OWNER TO databaseuser system;
ALTER CONVERSION regtest_user_conv_1 OWNER TO databaseuser system;
ALTER STATISTICS regtest_user_stat_1 OWNER TO databaseuser system; 


--
--  error test
-- 11
ALTER TABLE regtest_user_table_1 OWNER TO regress_database_test_user;
ALTER TABLE regtest_user_ptable_1 OWNER TO regress_database_test_user;
ALTER TABLE regtest_user_ptable_1_ones OWNER TO regress_database_test_user;
ALTER SEQUENCE regtest_user_seq_1 OWNER TO regress_database_test_user;
ALTER VIEW regtest_user_view_1 OWNER TO regress_database_test_user;
ALTER FUNCTION regtest_user_func_1() OWNER TO regress_database_test_user;
ALTER AGGREGATE regtest_user_agg_1(int2) OWNER TO regress_database_test_user;
ALTER CONVERSION regtest_user_conv_1 OWNER TO regress_database_test_user;
ALTER STATISTICS regtest_user_stat_1 OWNER TO regress_database_test_user; 

--
--  error test
-- 12
ALTER TABLE regtest_user_table_1 OWNER TO databaseuser regress_database_test_user;
ALTER TABLE regtest_user_ptable_1 OWNER TO databaseuser regress_database_test_user;
ALTER TABLE regtest_user_ptable_1_ones OWNER TO databaseuser regress_database_test_user;
ALTER SEQUENCE regtest_user_seq_1 OWNER TO databaseuser regress_database_test_user;
ALTER VIEW regtest_user_view_1 OWNER TO databaseuser regress_database_test_user;
ALTER FUNCTION regtest_user_func_1() OWNER TO databaseuser regress_database_test_user;
ALTER AGGREGATE regtest_user_agg_1(int2) OWNER TO databaseuser regress_database_test_user;
ALTER CONVERSION regtest_user_conv_1 OWNER TO databaseuser regress_database_test_user;
ALTER STATISTICS regtest_user_stat_1 OWNER TO databaseuser regress_database_test_user; 

--
--  error test
-- 13
ALTER SCHEMA regtest_schema_1 OWNER TO instanceuser regress_instance_test_user;
ALTER TABLE regtest_user_table_1 OWNER TO instanceuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_1 OWNER TO instanceuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_1_ones OWNER TO instanceuser regress_instance_test_user;
ALTER SEQUENCE regtest_user_seq_1 OWNER TO instanceuser regress_instance_test_user;
ALTER VIEW regtest_user_view_1 OWNER TO instanceuser regress_instance_test_user;
ALTER FUNCTION regtest_user_func_1() OWNER TO instanceuser regress_instance_test_user;
ALTER AGGREGATE regtest_user_agg_1(int2) OWNER TO instanceuser regress_instance_test_user;
ALTER CONVERSION regtest_user_conv_1 OWNER TO instanceuser regress_instance_test_user;
ALTER STATISTICS regtest_user_stat_1 OWNER TO instanceuser regress_instance_test_user; 


--
--  success test
-- 14 
ALTER TABLE regtest_user_table_1 OWNER TO regress_instance_test_user;
ALTER TABLE regtest_user_ptable_1 OWNER TO regress_instance_test_user;
ALTER TABLE regtest_user_ptable_1_ones OWNER TO regress_instance_test_user;
ALTER SEQUENCE regtest_user_seq_1 OWNER TO regress_instance_test_user;
ALTER VIEW regtest_user_view_1 OWNER TO regress_instance_test_user;
ALTER FUNCTION regtest_user_func_1() OWNER TO regress_instance_test_user;
ALTER AGGREGATE regtest_user_agg_1(int2) OWNER TO regress_instance_test_user;
ALTER CONVERSION regtest_user_conv_1 OWNER TO regress_instance_test_user;
ALTER STATISTICS regtest_user_stat_1 OWNER TO regress_instance_test_user; 

--
--  error test
-- 15 
ALTER TABLE regtest_user_table_1 OWNER TO databaseuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_1 OWNER TO databaseuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_1_ones OWNER TO databaseuser regress_instance_test_user;
ALTER SEQUENCE regtest_user_seq_1 OWNER TO databaseuser regress_instance_test_user;
ALTER VIEW regtest_user_view_1 OWNER TO databaseuser regress_instance_test_user;
ALTER FUNCTION regtest_user_func_1() OWNER TO databaseuser regress_instance_test_user;
ALTER AGGREGATE regtest_user_agg_1(int2) OWNER TO databaseuser regress_instance_test_user;
ALTER CONVERSION regtest_user_conv_1 OWNER TO databaseuser regress_instance_test_user;
ALTER STATISTICS regtest_user_stat_1 OWNER TO databaseuser regress_instance_test_user; 


set ivorysql.compatible_mode to oracle;
show ivorysql.compatible_mode;
set session authorization regress_database_test_user;

--
--  error test
-- 16
ALTER TABLE regtest_user_table_1 OWNER TO regress_database_test_user;
ALTER TABLE regtest_user_ptable_1 OWNER TO regress_database_test_user;
ALTER TABLE regtest_user_ptable_1_ones OWNER TO regress_database_test_user;
ALTER SEQUENCE regtest_user_seq_1 OWNER TO regress_database_test_user;
ALTER VIEW regtest_user_view_1 OWNER TO regress_database_test_user;
ALTER FUNCTION regtest_user_func_1() OWNER TO regress_database_test_user;
ALTER AGGREGATE regtest_user_agg_1(int2) OWNER TO regress_database_test_user;
ALTER CONVERSION regtest_user_conv_1 OWNER TO regress_database_test_user;
ALTER STATISTICS regtest_user_stat_1 OWNER TO regress_database_test_user; 

--
--  error test
-- 17
ALTER TABLE regtest_user_table_1 OWNER TO system;
ALTER TABLE regtest_user_ptable_1 OWNER TO system;
ALTER TABLE regtest_user_ptable_1_ones OWNER TO system;
ALTER SEQUENCE regtest_user_seq_1 OWNER TO system;
ALTER VIEW regtest_user_view_1 OWNER TO system;
ALTER FUNCTION regtest_user_func_1() OWNER TO system;
ALTER AGGREGATE regtest_user_agg_1(int2) OWNER TO system;
ALTER CONVERSION regtest_user_conv_1 OWNER TO system;
ALTER STATISTICS regtest_user_stat_1 OWNER TO system; 

--
--  error test
-- 18 
ALTER TABLE regtest_user_table_1 OWNER TO databaseuser regress_database_test_user;
ALTER TABLE regtest_user_ptable_1 OWNER TO databaseuser regress_database_test_user;
ALTER TABLE regtest_user_ptable_1_ones OWNER TO databaseuser regress_database_test_user;
ALTER SEQUENCE regtest_user_seq_1 OWNER TO databaseuser regress_database_test_user;
ALTER VIEW regtest_user_view_1 OWNER TO databaseuser regress_database_test_user;
ALTER FUNCTION regtest_user_func_1() OWNER TO databaseuser regress_database_test_user;
ALTER AGGREGATE regtest_user_agg_1(int2) OWNER TO databaseuser regress_database_test_user;
ALTER CONVERSION regtest_user_conv_1 OWNER TO databaseuser regress_database_test_user;
ALTER STATISTICS regtest_user_stat_1 OWNER TO databaseuser regress_database_test_user; 

--
--  error test
-- 19
ALTER TABLE regtest_user_table_1 OWNER TO regress_instance_test_user;
ALTER TABLE regtest_user_ptable_1 OWNER TO regress_instance_test_user;
ALTER TABLE regtest_user_ptable_1_ones OWNER TO regress_instance_test_user;
ALTER SEQUENCE regtest_user_seq_1 OWNER TO regress_instance_test_user;
ALTER VIEW regtest_user_view_1 OWNER TO regress_instance_test_user;
ALTER FUNCTION regtest_user_func_1() OWNER TO regress_instance_test_user;
ALTER AGGREGATE regtest_user_agg_1(int2) OWNER TO regress_instance_test_user;
ALTER CONVERSION regtest_user_conv_1 OWNER TO regress_instance_test_user;
ALTER STATISTICS regtest_user_stat_1 OWNER TO regress_instance_test_user; 

--
--  error test
-- 20
ALTER TABLE regtest_user_table_1 OWNER TO databaseuser system;
ALTER TABLE regtest_user_ptable_1 OWNER TO databaseuser system;
ALTER TABLE regtest_user_ptable_1_ones OWNER TO databaseuser system;
ALTER SEQUENCE regtest_user_seq_1 OWNER TO databaseuser system;
ALTER VIEW regtest_user_view_1 OWNER TO databaseuser system;
ALTER FUNCTION regtest_user_func_1() OWNER TO databaseuser system;
ALTER AGGREGATE regtest_user_agg_1(int2) OWNER TO databaseuser system;
ALTER CONVERSION regtest_user_conv_1 OWNER TO databaseuser system;
ALTER STATISTICS regtest_user_stat_1 OWNER TO databaseuser system; 

--
--  error test
-- 21
ALTER TABLE regtest_user_table_1 OWNER TO instanceuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_1 OWNER TO instanceuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_1_ones OWNER TO instanceuser regress_instance_test_user;
ALTER SEQUENCE regtest_user_seq_1 OWNER TO instanceuser regress_instance_test_user;
ALTER VIEW regtest_user_view_1 OWNER TO instanceuser regress_instance_test_user;
ALTER FUNCTION regtest_user_func_1() OWNER TO instanceuser regress_instance_test_user;
ALTER AGGREGATE regtest_user_agg_1(int2) OWNER TO instanceuser regress_instance_test_user;
ALTER CONVERSION regtest_user_conv_1 OWNER TO instanceuser regress_instance_test_user;
ALTER STATISTICS regtest_user_stat_1 OWNER TO instanceuser regress_instance_test_user; 

--
--  error test
-- 22
ALTER TABLE regtest_user_table_1 OWNER TO databaseuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_1 OWNER TO databaseuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_1_ones OWNER TO databaseuser regress_instance_test_user;
ALTER SEQUENCE regtest_user_seq_1 OWNER TO databaseuser regress_instance_test_user;
ALTER VIEW regtest_user_view_1 OWNER TO databaseuser regress_instance_test_user;
ALTER FUNCTION regtest_user_func_1() OWNER TO databaseuser regress_instance_test_user;
ALTER AGGREGATE regtest_user_agg_1(int2) OWNER TO databaseuser regress_instance_test_user;
ALTER CONVERSION regtest_user_conv_1 OWNER TO databaseuser regress_instance_test_user;
ALTER STATISTICS regtest_user_stat_1 OWNER TO databaseuser regress_instance_test_user; 


--
set ivorysql.grant_usertype to oracle;
show ivorysql.grant_usertype ;

--
--  error test
-- 23
ALTER TABLE regtest_user_table_1 OWNER TO system;
ALTER TABLE regtest_user_ptable_1 OWNER TO system;
ALTER TABLE regtest_user_ptable_1_ones OWNER TO system;
ALTER SEQUENCE regtest_user_seq_1 OWNER TO system;
ALTER VIEW regtest_user_view_1 OWNER TO system;
ALTER FUNCTION regtest_user_func_1() OWNER TO system;
ALTER AGGREGATE regtest_user_agg_1(int2) OWNER TO system;
ALTER CONVERSION regtest_user_conv_1 OWNER TO system;
ALTER STATISTICS regtest_user_stat_1 OWNER TO system; 

--
--  error test
-- 24
ALTER TABLE regtest_user_table_1 OWNER TO databaseuser system;
ALTER TABLE regtest_user_ptable_1 OWNER TO databaseuser system;
ALTER TABLE regtest_user_ptable_1_ones OWNER TO databaseuser system;
ALTER SEQUENCE regtest_user_seq_1 OWNER TO databaseuser system;
ALTER VIEW regtest_user_view_1 OWNER TO databaseuser system;
ALTER FUNCTION regtest_user_func_1() OWNER TO databaseuser system;
ALTER AGGREGATE regtest_user_agg_1(int2) OWNER TO databaseuser system;
ALTER CONVERSION regtest_user_conv_1 OWNER TO databaseuser system;
ALTER STATISTICS regtest_user_stat_1 OWNER TO databaseuser system; 


--
--  error test
-- 25
ALTER TABLE regtest_user_table_1 OWNER TO regress_database_test_user;
ALTER TABLE regtest_user_ptable_1 OWNER TO regress_database_test_user;
ALTER TABLE regtest_user_ptable_1_ones OWNER TO regress_database_test_user;
ALTER SEQUENCE regtest_user_seq_1 OWNER TO regress_database_test_user;
ALTER VIEW regtest_user_view_1 OWNER TO regress_database_test_user;
ALTER FUNCTION regtest_user_func_1() OWNER TO regress_database_test_user;
ALTER AGGREGATE regtest_user_agg_1(int2) OWNER TO regress_database_test_user;
ALTER CONVERSION regtest_user_conv_1 OWNER TO regress_database_test_user;
ALTER STATISTICS regtest_user_stat_1 OWNER TO regress_database_test_user; 

--
--  error test
--26
ALTER TABLE regtest_user_table_1 OWNER TO databaseuser regress_database_test_user;
ALTER TABLE regtest_user_ptable_1 OWNER TO databaseuser regress_database_test_user;
ALTER TABLE regtest_user_ptable_1_ones OWNER TO databaseuser regress_database_test_user;
ALTER SEQUENCE regtest_user_seq_1 OWNER TO databaseuser regress_database_test_user;
ALTER VIEW regtest_user_view_1 OWNER TO databaseuser regress_database_test_user;
ALTER FUNCTION regtest_user_func_1() OWNER TO databaseuser regress_database_test_user;
ALTER AGGREGATE regtest_user_agg_1(int2) OWNER TO databaseuser regress_database_test_user;
ALTER CONVERSION regtest_user_conv_1 OWNER TO databaseuser regress_database_test_user;
ALTER STATISTICS regtest_user_stat_1 OWNER TO databaseuser regress_database_test_user; 

--
--  error test
-- 27
ALTER SCHEMA regtest_schema_1 OWNER TO instanceuser regress_instance_test_user;
ALTER TABLE regtest_user_table_1 OWNER TO instanceuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_1 OWNER TO instanceuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_1_ones OWNER TO instanceuser regress_instance_test_user;
ALTER SEQUENCE regtest_user_seq_1 OWNER TO instanceuser regress_instance_test_user;
ALTER VIEW regtest_user_view_1 OWNER TO instanceuser regress_instance_test_user;
ALTER FUNCTION regtest_user_func_1() OWNER TO instanceuser regress_instance_test_user;
ALTER AGGREGATE regtest_user_agg_1(int2) OWNER TO instanceuser regress_instance_test_user;
ALTER CONVERSION regtest_user_conv_1 OWNER TO instanceuser regress_instance_test_user;
ALTER STATISTICS regtest_user_stat_1 OWNER TO instanceuser regress_instance_test_user; 


--
--  error test
--28
ALTER TABLE regtest_user_table_1 OWNER TO regress_instance_test_user;
ALTER TABLE regtest_user_ptable_1 OWNER TO regress_instance_test_user;
ALTER TABLE regtest_user_ptable_1_ones OWNER TO regress_instance_test_user;
ALTER SEQUENCE regtest_user_seq_1 OWNER TO regress_instance_test_user;
ALTER VIEW regtest_user_view_1 OWNER TO regress_instance_test_user;
ALTER FUNCTION regtest_user_func_1() OWNER TO regress_instance_test_user;
ALTER AGGREGATE regtest_user_agg_1(int2) OWNER TO regress_instance_test_user;
ALTER CONVERSION regtest_user_conv_1 OWNER TO regress_instance_test_user;
ALTER STATISTICS regtest_user_stat_1 OWNER TO regress_instance_test_user; 

--
--  error test
--29
ALTER TABLE regtest_user_table_1 OWNER TO databaseuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_1 OWNER TO databaseuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_1_ones OWNER TO databaseuser regress_instance_test_user;
ALTER SEQUENCE regtest_user_seq_1 OWNER TO databaseuser regress_instance_test_user;
ALTER VIEW regtest_user_view_1 OWNER TO databaseuser regress_instance_test_user;
ALTER FUNCTION regtest_user_func_1() OWNER TO databaseuser regress_instance_test_user;
ALTER AGGREGATE regtest_user_agg_1(int2) OWNER TO databaseuser regress_instance_test_user;
ALTER CONVERSION regtest_user_conv_1 OWNER TO databaseuser regress_instance_test_user;
ALTER STATISTICS regtest_user_stat_1 OWNER TO databaseuser regress_instance_test_user; 

--
set ivorysql.grant_usertype to pg;
show ivorysql.grant_usertype ;

--

set ivorysql.compatible_mode to pg;
show ivorysql.compatible_mode;
set session authorization regress_instance_test_user;

DROP TABLE regtest_user_ptable_1;
DROP SEQUENCE regtest_user_seq_1;
DROP VIEW regtest_user_view_1;
DROP FUNCTION regtest_user_func_1;
DROP AGGREGATE regtest_user_agg_1(int2);
DROP STATISTICS regtest_user_stat_1;
DROP CONVERSION regtest_user_conv_1;
DROP TABLE regtest_user_t1;
DROP TABLE regtest_user_table_1;

set ivorysql.compatible_mode to oracle;
show ivorysql.compatible_mode;
set session authorization regress_database_test_user;


---------------------------------------------------------------------------------------------

--
--  success test
-- 1 
ALTER TABLE regtest_user_table_2 OWNER TO regress_database_test_user;
ALTER TABLE regtest_user_ptable_2 OWNER TO regress_database_test_user;
ALTER TABLE regtest_user_ptable_2_ones OWNER TO regress_database_test_user;
ALTER SEQUENCE regtest_user_seq_2 OWNER TO regress_database_test_user;
ALTER VIEW regtest_user_view_2 OWNER TO regress_database_test_user;
ALTER FUNCTION regtest_user_func_2() OWNER TO regress_database_test_user;
ALTER AGGREGATE regtest_user_agg_2(int2) OWNER TO regress_database_test_user;
ALTER CONVERSION regtest_user_conv_2 OWNER TO regress_database_test_user;
ALTER STATISTICS regtest_user_stat_2 OWNER TO regress_database_test_user; 

--
--  error test
-- 2 
ALTER TABLE regtest_user_table_2 OWNER TO regress_instance_test_user;
ALTER TABLE regtest_user_ptable_2 OWNER TO regress_instance_test_user;
ALTER TABLE regtest_user_ptable_2_ones OWNER TO regress_instance_test_user;
ALTER SEQUENCE regtest_user_seq_2 OWNER TO regress_instance_test_user;
ALTER VIEW regtest_user_view_2 OWNER TO regress_instance_test_user;
ALTER FUNCTION regtest_user_func_2() OWNER TO regress_instance_test_user;
ALTER AGGREGATE regtest_user_agg_2(int2) OWNER TO regress_instance_test_user;
ALTER CONVERSION regtest_user_conv_2 OWNER TO regress_instance_test_user;
ALTER STATISTICS regtest_user_stat_2 OWNER TO regress_instance_test_user; 

--
--  error test
-- 3 
ALTER TABLE regtest_user_table_2 OWNER TO system;
ALTER TABLE regtest_user_ptable_2 OWNER TO system;
ALTER TABLE regtest_user_ptable_2_ones OWNER TO system;
ALTER SEQUENCE regtest_user_seq_2 OWNER TO system;
ALTER VIEW regtest_user_view_2 OWNER TO system;
ALTER FUNCTION regtest_user_func_2() OWNER TO system;
ALTER AGGREGATE regtest_user_agg_2(int2) OWNER TO system;
ALTER CONVERSION regtest_user_conv_2 OWNER TO system;
ALTER STATISTICS regtest_user_stat_2 OWNER TO system; 

--
--  error test
-- 4
ALTER TABLE regtest_user_table_2 OWNER TO databaseuser regress_database_test_user;
ALTER TABLE regtest_user_ptable_2 OWNER TO databaseuser regress_database_test_user;
ALTER TABLE regtest_user_ptable_2_ones OWNER TO databaseuser regress_database_test_user;
ALTER SEQUENCE regtest_user_seq_2 OWNER TO databaseuser regress_database_test_user;
ALTER VIEW regtest_user_view_2 OWNER TO databaseuser regress_database_test_user;
ALTER FUNCTION regtest_user_func_2() OWNER TO databaseuser regress_database_test_user;
ALTER AGGREGATE regtest_user_agg_2(int2) OWNER TO databaseuser regress_database_test_user;
ALTER CONVERSION regtest_user_conv_2 OWNER TO databaseuser regress_database_test_user;
ALTER STATISTICS regtest_user_stat_2 OWNER TO databaseuser regress_database_test_user; 


--
--  error test
-- 5 
ALTER TABLE regtest_user_table_2 OWNER TO regress_instance_test_user;
ALTER TABLE regtest_user_ptable_2 OWNER TO regress_instance_test_user;
ALTER TABLE regtest_user_ptable_2_ones OWNER TO regress_instance_test_user;
ALTER SEQUENCE regtest_user_seq_2 OWNER TO regress_instance_test_user;
ALTER VIEW regtest_user_view_2 OWNER TO regress_instance_test_user;
ALTER FUNCTION regtest_user_func_2() OWNER TO regress_instance_test_user;
ALTER AGGREGATE regtest_user_agg_2(int2) OWNER TO regress_instance_test_user;
ALTER CONVERSION regtest_user_conv_2 OWNER TO regress_instance_test_user;
ALTER STATISTICS regtest_user_stat_2 OWNER TO regress_instance_test_user; 

--
--  error test
-- 6
ALTER TABLE regtest_user_table_2 OWNER TO databaseuser system;
ALTER TABLE regtest_user_ptable_2 OWNER TO databaseuser system;
ALTER TABLE regtest_user_ptable_2_ones OWNER TO databaseuser system;
ALTER SEQUENCE regtest_user_seq_2 OWNER TO databaseuser system;
ALTER VIEW regtest_user_view_2 OWNER TO databaseuser system;
ALTER FUNCTION regtest_user_func_2() OWNER TO databaseuser system;
ALTER AGGREGATE regtest_user_agg_2(int2) OWNER TO databaseuser system;
ALTER CONVERSION regtest_user_conv_2 OWNER TO databaseuser system;
ALTER STATISTICS regtest_user_stat_2 OWNER TO databaseuser system; 

--
--  error test
-- 7
ALTER TABLE regtest_user_table_2 OWNER TO instanceuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_2 OWNER TO instanceuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_2_ones OWNER TO instanceuser regress_instance_test_user;
ALTER SEQUENCE regtest_user_seq_2 OWNER TO instanceuser regress_instance_test_user;
ALTER VIEW regtest_user_view_2 OWNER TO instanceuser regress_instance_test_user;
ALTER FUNCTION regtest_user_func_2() OWNER TO instanceuser regress_instance_test_user;
ALTER AGGREGATE regtest_user_agg_2(int2) OWNER TO instanceuser regress_instance_test_user;
ALTER CONVERSION regtest_user_conv_2 OWNER TO instanceuser regress_instance_test_user;
ALTER STATISTICS regtest_user_stat_2 OWNER TO instanceuser regress_instance_test_user; 

--
--  error test
-- 8
ALTER TABLE regtest_user_table_2 OWNER TO databaseuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_2 OWNER TO databaseuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_2_ones OWNER TO databaseuser regress_instance_test_user;
ALTER SEQUENCE regtest_user_seq_2 OWNER TO databaseuser regress_instance_test_user;
ALTER VIEW regtest_user_view_2 OWNER TO databaseuser regress_instance_test_user;
ALTER FUNCTION regtest_user_func_2() OWNER TO databaseuser regress_instance_test_user;
ALTER AGGREGATE regtest_user_agg_2(int2) OWNER TO databaseuser regress_instance_test_user;
ALTER CONVERSION regtest_user_conv_2 OWNER TO databaseuser regress_instance_test_user;
ALTER STATISTICS regtest_user_stat_2 OWNER TO databaseuser regress_instance_test_user; 


--
set ivorysql.grant_usertype to oracle;
show ivorysql.grant_usertype ;

--
--  error test
-- 9
ALTER TABLE regtest_user_table_2 OWNER TO system;
ALTER TABLE regtest_user_ptable_2 OWNER TO system;
ALTER TABLE regtest_user_ptable_2_ones OWNER TO system;
ALTER SEQUENCE regtest_user_seq_2 OWNER TO system;
ALTER VIEW regtest_user_view_2 OWNER TO system;
ALTER FUNCTION regtest_user_func_2() OWNER TO system;
ALTER AGGREGATE regtest_user_agg_2(int2) OWNER TO system;
ALTER CONVERSION regtest_user_conv_2 OWNER TO system;
ALTER STATISTICS regtest_user_stat_2 OWNER TO system; 

--
--  error test
-- 10
ALTER TABLE regtest_user_table_2 OWNER TO databaseuser system;
ALTER TABLE regtest_user_ptable_2 OWNER TO databaseuser system;
ALTER TABLE regtest_user_ptable_2_ones OWNER TO databaseuser system;
ALTER SEQUENCE regtest_user_seq_2 OWNER TO databaseuser system;
ALTER VIEW regtest_user_view_2 OWNER TO databaseuser system;
ALTER FUNCTION regtest_user_func_2() OWNER TO databaseuser system;
ALTER AGGREGATE regtest_user_agg_2(int2) OWNER TO databaseuser system;
ALTER CONVERSION regtest_user_conv_2 OWNER TO databaseuser system;
ALTER STATISTICS regtest_user_stat_2 OWNER TO databaseuser system; 


--
--  success test
-- 11
ALTER TABLE regtest_user_table_2 OWNER TO regress_database_test_user;
ALTER TABLE regtest_user_ptable_2 OWNER TO regress_database_test_user;
ALTER TABLE regtest_user_ptable_2_ones OWNER TO regress_database_test_user;
ALTER SEQUENCE regtest_user_seq_2 OWNER TO regress_database_test_user;
ALTER VIEW regtest_user_view_2 OWNER TO regress_database_test_user;
ALTER FUNCTION regtest_user_func_2() OWNER TO regress_database_test_user;
ALTER AGGREGATE regtest_user_agg_2(int2) OWNER TO regress_database_test_user;
ALTER CONVERSION regtest_user_conv_2 OWNER TO regress_database_test_user;
ALTER STATISTICS regtest_user_stat_2 OWNER TO regress_database_test_user; 

--
--  success test
-- 12
ALTER TABLE regtest_user_table_2 OWNER TO databaseuser regress_database_test_user;
ALTER TABLE regtest_user_ptable_2 OWNER TO databaseuser regress_database_test_user;
ALTER TABLE regtest_user_ptable_2_ones OWNER TO databaseuser regress_database_test_user;
ALTER SEQUENCE regtest_user_seq_2 OWNER TO databaseuser regress_database_test_user;
ALTER VIEW regtest_user_view_2 OWNER TO databaseuser regress_database_test_user;
ALTER FUNCTION regtest_user_func_2() OWNER TO databaseuser regress_database_test_user;
ALTER AGGREGATE regtest_user_agg_2(int2) OWNER TO databaseuser regress_database_test_user;
ALTER CONVERSION regtest_user_conv_2 OWNER TO databaseuser regress_database_test_user;
ALTER STATISTICS regtest_user_stat_2 OWNER TO databaseuser regress_database_test_user; 

--
--  error test
-- 13
ALTER TABLE regtest_user_table_2 OWNER TO instanceuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_2 OWNER TO instanceuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_2_ones OWNER TO instanceuser regress_instance_test_user;
ALTER SEQUENCE regtest_user_seq_2 OWNER TO instanceuser regress_instance_test_user;
ALTER VIEW regtest_user_view_2 OWNER TO instanceuser regress_instance_test_user;
ALTER FUNCTION regtest_user_func_2() OWNER TO instanceuser regress_instance_test_user;
ALTER AGGREGATE regtest_user_agg_2(int2) OWNER TO instanceuser regress_instance_test_user;
ALTER CONVERSION regtest_user_conv_2 OWNER TO instanceuser regress_instance_test_user;
ALTER STATISTICS regtest_user_stat_2 OWNER TO instanceuser regress_instance_test_user; 


--
--  error test
-- 14 
ALTER TABLE regtest_user_table_2 OWNER TO regress_instance_test_user;
ALTER TABLE regtest_user_ptable_2 OWNER TO regress_instance_test_user;
ALTER TABLE regtest_user_ptable_2_ones OWNER TO regress_instance_test_user;
ALTER SEQUENCE regtest_user_seq_2 OWNER TO regress_instance_test_user;
ALTER VIEW regtest_user_view_2 OWNER TO regress_instance_test_user;
ALTER FUNCTION regtest_user_func_2() OWNER TO regress_instance_test_user;
ALTER AGGREGATE regtest_user_agg_2(int2) OWNER TO regress_instance_test_user;
ALTER CONVERSION regtest_user_conv_2 OWNER TO regress_instance_test_user;
ALTER STATISTICS regtest_user_stat_2 OWNER TO regress_instance_test_user; 
--
--  error test
-- 15 
ALTER TABLE regtest_user_table_2 OWNER TO databaseuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_2 OWNER TO databaseuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_2_ones OWNER TO databaseuser regress_instance_test_user;
ALTER SEQUENCE regtest_user_seq_2 OWNER TO databaseuser regress_instance_test_user;
ALTER VIEW regtest_user_view_2 OWNER TO databaseuser regress_instance_test_user;
ALTER FUNCTION regtest_user_func_2() OWNER TO databaseuser regress_instance_test_user;
ALTER AGGREGATE regtest_user_agg_2(int2) OWNER TO databaseuser regress_instance_test_user;
ALTER CONVERSION regtest_user_conv_2 OWNER TO databaseuser regress_instance_test_user;
ALTER STATISTICS regtest_user_stat_2 OWNER TO databaseuser regress_instance_test_user; 


set ivorysql.compatible_mode to oracle;
show ivorysql.compatible_mode;
set session authorization regress_database_test_user;


--
--  success test
-- 16
ALTER TABLE regtest_user_table_2 OWNER TO regress_database_test_user;
ALTER TABLE regtest_user_ptable_2 OWNER TO regress_database_test_user;
ALTER TABLE regtest_user_ptable_2_ones OWNER TO regress_database_test_user;
ALTER SEQUENCE regtest_user_seq_2 OWNER TO regress_database_test_user;
ALTER VIEW regtest_user_view_2 OWNER TO regress_database_test_user;
ALTER FUNCTION regtest_user_func_2() OWNER TO regress_database_test_user;
ALTER AGGREGATE regtest_user_agg_2(int2) OWNER TO regress_database_test_user;
ALTER CONVERSION regtest_user_conv_2 OWNER TO regress_database_test_user;
ALTER STATISTICS regtest_user_stat_2 OWNER TO regress_database_test_user; 

--
--  error test
-- 17
ALTER TABLE regtest_user_table_2 OWNER TO system;
ALTER TABLE regtest_user_ptable_2 OWNER TO system;
ALTER TABLE regtest_user_ptable_2_ones OWNER TO system;
ALTER SEQUENCE regtest_user_seq_2 OWNER TO system;
ALTER VIEW regtest_user_view_2 OWNER TO system;
ALTER FUNCTION regtest_user_func_2() OWNER TO system;
ALTER AGGREGATE regtest_user_agg_2(int2) OWNER TO system;
ALTER CONVERSION regtest_user_conv_2 OWNER TO system;
ALTER STATISTICS regtest_user_stat_2 OWNER TO system; 

--
--  success test
-- 18 
ALTER TABLE regtest_user_table_2 OWNER TO databaseuser regress_database_test_user;
ALTER TABLE regtest_user_ptable_2 OWNER TO databaseuser regress_database_test_user;
ALTER TABLE regtest_user_ptable_2_ones OWNER TO databaseuser regress_database_test_user;
ALTER SEQUENCE regtest_user_seq_2 OWNER TO databaseuser regress_database_test_user;
ALTER VIEW regtest_user_view_2 OWNER TO databaseuser regress_database_test_user;
ALTER FUNCTION regtest_user_func_2() OWNER TO databaseuser regress_database_test_user;
ALTER AGGREGATE regtest_user_agg_2(int2) OWNER TO databaseuser regress_database_test_user;
ALTER CONVERSION regtest_user_conv_2 OWNER TO databaseuser regress_database_test_user;
ALTER STATISTICS regtest_user_stat_2 OWNER TO databaseuser regress_database_test_user; 

--
--  error test
-- 19
ALTER TABLE regtest_user_table_2 OWNER TO regress_instance_test_user;
ALTER TABLE regtest_user_ptable_2 OWNER TO regress_instance_test_user;
ALTER TABLE regtest_user_ptable_2_ones OWNER TO regress_instance_test_user;
ALTER SEQUENCE regtest_user_seq_2 OWNER TO regress_instance_test_user;
ALTER VIEW regtest_user_view_2 OWNER TO regress_instance_test_user;
ALTER FUNCTION regtest_user_func_2() OWNER TO regress_instance_test_user;
ALTER AGGREGATE regtest_user_agg_2(int2) OWNER TO regress_instance_test_user;
ALTER CONVERSION regtest_user_conv_2 OWNER TO regress_instance_test_user;
ALTER STATISTICS regtest_user_stat_2 OWNER TO regress_instance_test_user; 

--
--  error test
-- 20
ALTER TABLE regtest_user_table_2 OWNER TO databaseuser system;
ALTER TABLE regtest_user_ptable_2 OWNER TO databaseuser system;
ALTER TABLE regtest_user_ptable_2_ones OWNER TO databaseuser system;
ALTER SEQUENCE regtest_user_seq_2 OWNER TO databaseuser system;
ALTER VIEW regtest_user_view_2 OWNER TO databaseuser system;
ALTER FUNCTION regtest_user_func_2() OWNER TO databaseuser system;
ALTER AGGREGATE regtest_user_agg_2(int2) OWNER TO databaseuser system;
ALTER CONVERSION regtest_user_conv_2 OWNER TO databaseuser system;
ALTER STATISTICS regtest_user_stat_2 OWNER TO databaseuser system; 

--
--  error test
-- 21
ALTER TABLE regtest_user_table_2 OWNER TO instanceuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_2 OWNER TO instanceuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_2_ones OWNER TO instanceuser regress_instance_test_user;
ALTER SEQUENCE regtest_user_seq_2 OWNER TO instanceuser regress_instance_test_user;
ALTER VIEW regtest_user_view_2 OWNER TO instanceuser regress_instance_test_user;
ALTER FUNCTION regtest_user_func_2() OWNER TO instanceuser regress_instance_test_user;
ALTER AGGREGATE regtest_user_agg_2(int2) OWNER TO instanceuser regress_instance_test_user;
ALTER CONVERSION regtest_user_conv_2 OWNER TO instanceuser regress_instance_test_user;
ALTER STATISTICS regtest_user_stat_2 OWNER TO instanceuser regress_instance_test_user; 

--
--  error test
-- 22
ALTER TABLE regtest_user_table_2 OWNER TO databaseuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_2 OWNER TO databaseuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_2_ones OWNER TO databaseuser regress_instance_test_user;
ALTER SEQUENCE regtest_user_seq_2 OWNER TO databaseuser regress_instance_test_user;
ALTER VIEW regtest_user_view_2 OWNER TO databaseuser regress_instance_test_user;
ALTER FUNCTION regtest_user_func_2() OWNER TO databaseuser regress_instance_test_user;
ALTER AGGREGATE regtest_user_agg_2(int2) OWNER TO databaseuser regress_instance_test_user;
ALTER CONVERSION regtest_user_conv_2 OWNER TO databaseuser regress_instance_test_user;
ALTER STATISTICS regtest_user_stat_2 OWNER TO databaseuser regress_instance_test_user; 


--
set ivorysql.grant_usertype to oracle;
show ivorysql.grant_usertype ;

--
--  error test
-- 23
ALTER TABLE regtest_user_table_2 OWNER TO system;
ALTER TABLE regtest_user_ptable_2 OWNER TO system;
ALTER TABLE regtest_user_ptable_2_ones OWNER TO system;
ALTER SEQUENCE regtest_user_seq_2 OWNER TO system;
ALTER VIEW regtest_user_view_2 OWNER TO system;
ALTER FUNCTION regtest_user_func_2() OWNER TO system;
ALTER AGGREGATE regtest_user_agg_2(int2) OWNER TO system;
ALTER CONVERSION regtest_user_conv_2 OWNER TO system;
ALTER STATISTICS regtest_user_stat_2 OWNER TO system; 

--
--  error test
-- 24
ALTER TABLE regtest_user_table_2 OWNER TO databaseuser system;
ALTER TABLE regtest_user_ptable_2 OWNER TO databaseuser system;
ALTER TABLE regtest_user_ptable_2_ones OWNER TO databaseuser system;
ALTER SEQUENCE regtest_user_seq_2 OWNER TO databaseuser system;
ALTER VIEW regtest_user_view_2 OWNER TO databaseuser system;
ALTER FUNCTION regtest_user_func_2() OWNER TO databaseuser system;
ALTER AGGREGATE regtest_user_agg_2(int2) OWNER TO databaseuser system;
ALTER CONVERSION regtest_user_conv_2 OWNER TO databaseuser system;
ALTER STATISTICS regtest_user_stat_2 OWNER TO databaseuser system; 


--
--  success test
-- 25
ALTER TABLE regtest_user_table_2 OWNER TO regress_database_test_user;
ALTER TABLE regtest_user_ptable_2 OWNER TO regress_database_test_user;
ALTER TABLE regtest_user_ptable_2_ones OWNER TO regress_database_test_user;
ALTER SEQUENCE regtest_user_seq_2 OWNER TO regress_database_test_user;
ALTER VIEW regtest_user_view_2 OWNER TO regress_database_test_user;
ALTER FUNCTION regtest_user_func_2() OWNER TO regress_database_test_user;
ALTER AGGREGATE regtest_user_agg_2(int2) OWNER TO regress_database_test_user;
ALTER CONVERSION regtest_user_conv_2 OWNER TO regress_database_test_user;
ALTER STATISTICS regtest_user_stat_2 OWNER TO regress_database_test_user; 

--
--  success test
--26
ALTER TABLE regtest_user_table_2 OWNER TO databaseuser regress_database_test_user;
ALTER TABLE regtest_user_ptable_2 OWNER TO databaseuser regress_database_test_user;
ALTER TABLE regtest_user_ptable_2_ones OWNER TO databaseuser regress_database_test_user;
ALTER SEQUENCE regtest_user_seq_2 OWNER TO databaseuser regress_database_test_user;
ALTER VIEW regtest_user_view_2 OWNER TO databaseuser regress_database_test_user;
ALTER FUNCTION regtest_user_func_2() OWNER TO databaseuser regress_database_test_user;
ALTER AGGREGATE regtest_user_agg_2(int2) OWNER TO databaseuser regress_database_test_user;
ALTER CONVERSION regtest_user_conv_2 OWNER TO databaseuser regress_database_test_user;
ALTER STATISTICS regtest_user_stat_2 OWNER TO databaseuser regress_database_test_user; 

--
--  error test
-- 27
ALTER TABLE regtest_user_table_2 OWNER TO instanceuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_2 OWNER TO instanceuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_2_ones OWNER TO instanceuser regress_instance_test_user;
ALTER SEQUENCE regtest_user_seq_2 OWNER TO instanceuser regress_instance_test_user;
ALTER VIEW regtest_user_view_2 OWNER TO instanceuser regress_instance_test_user;
ALTER FUNCTION regtest_user_func_2() OWNER TO instanceuser regress_instance_test_user;
ALTER AGGREGATE regtest_user_agg_2(int2) OWNER TO instanceuser regress_instance_test_user;
ALTER CONVERSION regtest_user_conv_2 OWNER TO instanceuser regress_instance_test_user;
ALTER STATISTICS regtest_user_stat_2 OWNER TO instanceuser regress_instance_test_user; 


--
--  error test
--28
ALTER TABLE regtest_user_table_2 OWNER TO regress_instance_test_user;
ALTER TABLE regtest_user_ptable_2 OWNER TO regress_instance_test_user;
ALTER TABLE regtest_user_ptable_2_ones OWNER TO regress_instance_test_user;
ALTER SEQUENCE regtest_user_seq_2 OWNER TO regress_instance_test_user;
ALTER VIEW regtest_user_view_2 OWNER TO regress_instance_test_user;
ALTER FUNCTION regtest_user_func_2() OWNER TO regress_instance_test_user;
ALTER AGGREGATE regtest_user_agg_2(int2) OWNER TO regress_instance_test_user;
ALTER CONVERSION regtest_user_conv_2 OWNER TO regress_instance_test_user;
ALTER STATISTICS regtest_user_stat_2 OWNER TO regress_instance_test_user; 

--
--  error test
--29
ALTER TABLE regtest_user_table_2 OWNER TO databaseuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_2 OWNER TO databaseuser regress_instance_test_user;
ALTER TABLE regtest_user_ptable_2_ones OWNER TO databaseuser regress_instance_test_user;
ALTER SEQUENCE regtest_user_seq_2 OWNER TO databaseuser regress_instance_test_user;
ALTER VIEW regtest_user_view_2 OWNER TO databaseuser regress_instance_test_user;
ALTER FUNCTION regtest_user_func_2() OWNER TO databaseuser regress_instance_test_user;
ALTER AGGREGATE regtest_user_agg_2(int2) OWNER TO databaseuser regress_instance_test_user;
ALTER CONVERSION regtest_user_conv_2 OWNER TO databaseuser regress_instance_test_user;
ALTER STATISTICS regtest_user_stat_2 OWNER TO databaseuser regress_instance_test_user; 

--
set ivorysql.grant_usertype to pg;
show ivorysql.grant_usertype ;

--
DROP TABLE regtest_user_ptable_2;
DROP SEQUENCE regtest_user_seq_2;
DROP VIEW regtest_user_view_2;
DROP FUNCTION regtest_user_func_2;
DROP STATISTICS regtest_user_stat_2;
DROP AGGREGATE regtest_user_agg_2(int2);
DROP CONVERSION regtest_user_conv_2;
DROP TABLE regtest_user_t2;
DROP TABLE regtest_user_table_2;
--
-- GRANT xxx ON xxx TO xxx
-- 
-- databaseuser 
set session authorization system;

CREATE USER databaseu;
CREATE GROUP regress_database_group;
set session authorization databaseu;
CREATE TABLE regress_user_table_2 (col1 serial, col2 int check (col2 > 0));
CREATE SEQUENCE regress_user_seq_2;

CREATE SEQUENCE regress_user_seq2_2;
CREATE TYPE regress_user_type_2 AS (col1 int);
CREATE FUNCTION regress_user_func_2 () RETURNS int
AS 'SELECT 1;' LANGUAGE SQL;
/
CREATE AGGREGATE regress_user_agg_2(int2)
(SFUNC = int2_sum, STYPE = int8);


set ivorysql.compatible_mode to pg;
show ivorysql.compatible_mode;
select dbuser_function();
-- instanceuser
CREATE USER instanceu;
CREATE GROUP regress_instance_group;

set session authorization instanceu;


CREATE TABLE regress_user_table_1 (col1 serial, col2 int check (col2 > 0));
CREATE SEQUENCE regress_user_seq_1;

CREATE SEQUENCE regress_user_seq2_1;
CREATE TYPE regress_user_type_1 AS (col1 int);
CREATE FUNCTION regress_user_func_1 () RETURNS int
AS 'SELECT 1;' LANGUAGE SQL;
/
CREATE AGGREGATE regress_user_agg_1(int2)
(SFUNC = int2_sum, STYPE = int8);


--
--  error test
-- 30
GRANT SELECT ON regress_user_table_1 TO databaseu;
GRANT SELECT(col2) ON regress_user_table_1 TO databaseu;
GRANT SELECT,INSERT ON regress_user_table_1 TO databaseu;
GRANT DELETE ON regress_user_table_1 TO GROUP regress_database_group;

REVOKE SELECT(col2) ON regress_user_table_1 FROM databaseu;
GRANT ALL ON regress_user_seq_1 TO databaseu;

GRANT ALL ON regress_user_seq2_1 TO databaseu;
GRANT ALL ON TYPE regress_user_type_1 TO databaseu;
GRANT EXECUTE ON FUNCTION regress_user_func_1() TO databaseu;
GRANT EXECUTE ON FUNCTION regress_user_agg_1(int2) TO databaseu;


--
--  success test
-- 31
--
GRANT SELECT ON regress_user_table_1 TO instanceu;
GRANT SELECT(col2) ON regress_user_table_1 TO instanceu;
GRANT SELECT,INSERT ON regress_user_table_1 TO instanceu;
GRANT DELETE ON regress_user_table_1 TO GROUP regress_instance_group;

REVOKE SELECT(col2) ON regress_user_table_1 FROM instanceu;

GRANT ALL ON SEQUENCE regress_user_seq_1 TO pg_signal_backend;
GRANT ALL ON regress_user_seq_1 TO instanceu;

GRANT ALL ON regress_user_seq2_1 TO instanceu;
GRANT ALL ON TYPE regress_user_type_1 TO instanceu;
GRANT EXECUTE ON FUNCTION regress_user_func_1() TO instanceu;
GRANT EXECUTE ON FUNCTION regress_user_agg_1(int2) TO instanceu;


--
--  error test
-- 32
GRANT SELECT ON regress_user_table_1 TO databaseuser databaseu;
GRANT SELECT(col2) ON regress_user_table_1 TO databaseuser databaseu;
GRANT SELECT,INSERT ON regress_user_table_1 TO databaseuser databaseu;
GRANT DELETE ON regress_user_table_1 TO GROUP databaseuser regress_database_group;

REVOKE SELECT(col2) ON regress_user_table_1 FROM databaseuser databaseu;
GRANT ALL ON regress_user_seq_1 TO databaseuser databaseu;

GRANT ALL ON regress_user_seq2_1 TO databaseuser databaseu;
GRANT ALL ON TYPE regress_user_type_1 TO databaseuser databaseu;
GRANT EXECUTE ON FUNCTION regress_user_func_1() TO databaseuser databaseu;
GRANT EXECUTE ON FUNCTION regress_user_agg_1(int2) TO databaseuser databaseu;



--
--  error test
-- 33
--
GRANT SELECT ON regress_user_table_1 TO instanceuser instanceu;
GRANT SELECT(col2) ON regress_user_table_1 TO instanceuser instanceu;
GRANT SELECT,INSERT ON regress_user_table_1 TO instanceuser instanceu;
GRANT DELETE ON regress_user_table_1 TO GROUP instanceuser regress_instance_group;

REVOKE SELECT(col2) ON regress_user_table_1 FROM instanceuser instanceu;

GRANT ALL ON SEQUENCE regress_user_seq_1 TO instanceuser pg_signal_backend;
GRANT ALL ON regress_user_seq_1 TO instanceuser instanceu;

GRANT ALL ON regress_user_seq2_1 TO instanceuser instanceu;
GRANT ALL ON TYPE regress_user_type_1 TO instanceuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_func_1() TO instanceuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_agg_1(int2) TO instanceuser instanceu;


--
--  error test
-- 34
GRANT SELECT ON regress_user_table_1 TO databaseuser instanceu;
GRANT SELECT(col2) ON regress_user_table_1 TO databaseuser instanceu;
GRANT SELECT,INSERT ON regress_user_table_1 TO databaseuser instanceu;
GRANT DELETE ON regress_user_table_1 TO GROUP databaseuser regress_instance_group;

REVOKE SELECT(col2) ON regress_user_table_1 FROM databaseuser instanceu;

GRANT ALL ON SEQUENCE regress_user_seq_1 TO databaseuser pg_signal_backend;
GRANT ALL ON regress_user_seq_1 TO databaseuser instanceu;

GRANT ALL ON regress_user_seq2_1 TO databaseuser instanceu;
GRANT ALL ON TYPE regress_user_type_1 TO databaseuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_func_1() TO databaseuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_agg_1(int2) TO databaseuser instanceu;


--
--  error test
-- 35
--
GRANT SELECT ON regress_user_table_1 TO databaseuser system;
GRANT SELECT(col2) ON regress_user_table_1 TO databaseuser system;
GRANT SELECT,INSERT ON regress_user_table_1 TO databaseuser system;
GRANT DELETE ON regress_user_table_1 TO GROUP databaseuser regress_database_group;

REVOKE SELECT(col2) ON regress_user_table_1 FROM databaseuser system;

GRANT ALL ON SEQUENCE regress_user_seq_1 TO databaseuser system;
GRANT ALL ON regress_user_seq_1 TO databaseuser system;

GRANT ALL ON regress_user_seq2_1 TO databaseuser system;
GRANT ALL ON TYPE regress_user_type_1 TO databaseuser system;
GRANT EXECUTE ON FUNCTION regress_user_func_1() TO databaseuser system;
GRANT EXECUTE ON FUNCTION regress_user_agg_1(int2) TO databaseuser system;


--
set ivorysql.grant_usertype to oracle;
show ivorysql.grant_usertype ;

--
--  error test
-- 36

GRANT SELECT ON regress_user_table_1 TO databaseuser system;
GRANT SELECT(col2) ON regress_user_table_1 TO databaseuser system;
GRANT SELECT,INSERT ON regress_user_table_1 TO databaseuser system;
GRANT DELETE ON regress_user_table_1 TO GROUP databaseuser regress_database_group;

REVOKE SELECT(col2) ON regress_user_table_1 FROM databaseuser system;

GRANT ALL ON SEQUENCE regress_user_seq_1 TO databaseuser system;
GRANT ALL ON regress_user_seq_1 TO databaseuser system;

GRANT ALL ON regress_user_seq2_1 TO databaseuser system;
GRANT ALL ON TYPE regress_user_type_1 TO databaseuser system;
GRANT EXECUTE ON FUNCTION regress_user_func_1() TO databaseuser system;
GRANT EXECUTE ON FUNCTION regress_user_agg_1(int2) TO databaseuser system;



--
--  error test
-- 37
GRANT SELECT ON regress_user_table_1 TO instanceuser instanceu;
GRANT SELECT(col2) ON regress_user_table_1 TO instanceuser instanceu;
GRANT SELECT,INSERT ON regress_user_table_1 TO instanceuser instanceu;
GRANT DELETE ON regress_user_table_1 TO GROUP instanceuser regress_instance_group;

REVOKE SELECT(col2) ON regress_user_table_1 FROM instanceuser instanceu;

GRANT ALL ON SEQUENCE regress_user_seq_1 TO instanceuser pg_signal_backend;
GRANT ALL ON regress_user_seq_1 TO instanceuser instanceu;

GRANT ALL ON regress_user_seq2_1 TO instanceuser instanceu;
GRANT ALL ON TYPE regress_user_type_1 TO instanceuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_func_1() TO instanceuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_agg_1(int2) TO instanceuser instanceu;



--
--  error test
--38
GRANT SELECT ON regress_user_table_1 TO databaseuser databaseu;
GRANT SELECT(col2) ON regress_user_table_1 TO databaseuser databaseu;
GRANT SELECT,INSERT ON regress_user_table_1 TO databaseuser databaseu;
GRANT DELETE ON regress_user_table_1 TO GROUP databaseuser regress_database_group;

REVOKE SELECT(col2) ON regress_user_table_1 FROM databaseuser databaseu;

GRANT ALL ON SEQUENCE regress_user_seq_1 TO databaseuser databaseu;
GRANT ALL ON regress_user_seq_1 TO databaseuser databaseu;

GRANT ALL ON regress_user_seq2_1 TO databaseuser databaseu;
GRANT ALL ON TYPE regress_user_type_1 TO databaseuser databaseu;
GRANT EXECUTE ON FUNCTION regress_user_func_1() TO databaseuser databaseu;
GRANT EXECUTE ON FUNCTION regress_user_agg_1(int2) TO databaseuser databaseu;



--
--  error test
--39
--
GRANT SELECT ON regress_user_table_1 TO databaseuser instanceu;
GRANT SELECT(col2) ON regress_user_table_1 TO databaseuser instanceu;
GRANT SELECT,INSERT ON regress_user_table_1 TO databaseuser instanceu;
GRANT DELETE ON regress_user_table_1 TO GROUP databaseuser regress_database_group;

REVOKE SELECT(col2) ON regress_user_table_1 FROM databaseuser instanceu;

GRANT ALL ON SEQUENCE regress_user_seq_1 TO databaseuser instanceu;
GRANT ALL ON regress_user_seq_1 TO databaseuser instanceu;

GRANT ALL ON regress_user_seq2_1 TO databaseuser instanceu;
GRANT ALL ON TYPE regress_user_type_1 TO databaseuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_func_1() TO databaseuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_agg_1(int2) TO databaseuser instanceu;


--
--  success test
--  40
--
GRANT SELECT ON regress_user_table_1 TO instanceu;
GRANT SELECT(col2) ON regress_user_table_1 TO instanceu;
GRANT SELECT,INSERT ON regress_user_table_1 TO instanceu;
GRANT DELETE ON regress_user_table_1 TO GROUP regress_instance_group;

REVOKE SELECT(col2) ON regress_user_table_1 FROM instanceu;

GRANT ALL ON SEQUENCE regress_user_seq_1 TO pg_signal_backend;
GRANT ALL ON regress_user_seq_1 TO instanceu;

GRANT ALL ON regress_user_seq2_1 TO instanceu;
GRANT ALL ON TYPE regress_user_type_1 TO instanceu;
GRANT EXECUTE ON FUNCTION regress_user_func_1() TO instanceu;
GRANT EXECUTE ON FUNCTION regress_user_agg_1(int2) TO instanceu;



set ivorysql.compatible_mode to oracle;
show ivorysql.compatible_mode;
set session authorization databaseu;

--
--  error test
-- 41
GRANT SELECT ON regress_user_table_1 TO databaseu;
GRANT SELECT(col2) ON regress_user_table_1 TO databaseu;
GRANT SELECT,INSERT ON regress_user_table_1 TO databaseu;
GRANT DELETE ON regress_user_table_1 TO GROUP regress_database_group;

REVOKE SELECT(col2) ON regress_user_table_1 FROM databaseu;
GRANT ALL ON regress_user_seq_1 TO databaseu;

GRANT ALL ON regress_user_seq2_1 TO databaseu;
GRANT ALL ON TYPE regress_user_type_1 TO databaseu;
GRANT EXECUTE ON FUNCTION regress_user_func_1() TO databaseu;
GRANT EXECUTE ON FUNCTION regress_user_agg_1(int2) TO databaseu;


--
--  error test
-- 42
--
GRANT SELECT ON regress_user_table_1 TO instanceu;
GRANT SELECT(col2) ON regress_user_table_1 TO instanceu;
GRANT SELECT,INSERT ON regress_user_table_1 TO instanceu;
GRANT DELETE ON regress_user_table_1 TO GROUP regress_instance_group;

REVOKE SELECT(col2) ON regress_user_table_1 FROM instanceu;

GRANT ALL ON SEQUENCE regress_user_seq_1 TO pg_signal_backend;
GRANT ALL ON regress_user_seq_1 TO instanceu;

GRANT ALL ON regress_user_seq2_1 TO instanceu;
GRANT ALL ON TYPE regress_user_type_1 TO instanceu;
GRANT EXECUTE ON FUNCTION regress_user_func_1() TO instanceu;
GRANT EXECUTE ON FUNCTION regress_user_agg_1(int2) TO instanceu;


--
--  error test
-- 43
GRANT SELECT ON regress_user_table_1 TO databaseuser databaseu;
GRANT SELECT(col2) ON regress_user_table_1 TO databaseuser databaseu;
GRANT SELECT,INSERT ON regress_user_table_1 TO databaseuser databaseu;
GRANT DELETE ON regress_user_table_1 TO GROUP databaseuser regress_database_group;

REVOKE SELECT(col2) ON regress_user_table_1 FROM databaseuser databaseu;
GRANT ALL ON regress_user_seq_1 TO databaseuser databaseu;

GRANT ALL ON regress_user_seq2_1 TO databaseuser databaseu;
GRANT ALL ON TYPE regress_user_type_1 TO databaseuser databaseu;
GRANT EXECUTE ON FUNCTION regress_user_func_1() TO databaseuser databaseu;
GRANT EXECUTE ON FUNCTION regress_user_agg_1(int2) TO databaseuser databaseu;



--
--  error test
-- 44 
--
GRANT SELECT ON regress_user_table_1 TO instanceuser instanceu;
GRANT SELECT(col2) ON regress_user_table_1 TO instanceuser instanceu;
GRANT SELECT,INSERT ON regress_user_table_1 TO instanceuser instanceu;
GRANT DELETE ON regress_user_table_1 TO GROUP instanceuser regress_instance_group;

REVOKE SELECT(col2) ON regress_user_table_1 FROM instanceuser instanceu;

GRANT ALL ON SEQUENCE regress_user_seq_1 TO instanceuser pg_signal_backend;
GRANT ALL ON regress_user_seq_1 TO instanceuser instanceu;

GRANT ALL ON regress_user_seq2_1 TO instanceuser instanceu;
GRANT ALL ON TYPE regress_user_type_1 TO instanceuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_func_1() TO instanceuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_agg_1(int2) TO instanceuser instanceu;


--
--  error test
-- 45
GRANT SELECT ON regress_user_table_1 TO databaseuser instanceu;
GRANT SELECT(col2) ON regress_user_table_1 TO databaseuser instanceu;
GRANT SELECT,INSERT ON regress_user_table_1 TO databaseuser instanceu;
GRANT DELETE ON regress_user_table_1 TO GROUP databaseuser regress_instance_group;

REVOKE SELECT(col2) ON regress_user_table_1 FROM databaseuser instanceu;

GRANT ALL ON SEQUENCE regress_user_seq_1 TO databaseuser pg_signal_backend;
GRANT ALL ON regress_user_seq_1 TO databaseuser instanceu;

GRANT ALL ON regress_user_seq2_1 TO databaseuser instanceu;
GRANT ALL ON TYPE regress_user_type_1 TO databaseuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_func_1() TO databaseuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_agg_1(int2) TO databaseuser instanceu;


--
--  error test
-- 46
--
GRANT SELECT ON regress_user_table_1 TO databaseuser system;
GRANT SELECT(col2) ON regress_user_table_1 TO databaseuser system;
GRANT SELECT,INSERT ON regress_user_table_1 TO databaseuser system;
GRANT DELETE ON regress_user_table_1 TO GROUP databaseuser regress_database_group;

REVOKE SELECT(col2) ON regress_user_table_1 FROM databaseuser system;

GRANT ALL ON SEQUENCE regress_user_seq_1 TO databaseuser system;
GRANT ALL ON regress_user_seq_1 TO databaseuser system;

GRANT ALL ON regress_user_seq2_1 TO databaseuser system;
GRANT ALL ON TYPE regress_user_type_1 TO databaseuser system;
GRANT EXECUTE ON FUNCTION regress_user_func_1() TO databaseuser system;
GRANT EXECUTE ON FUNCTION regress_user_agg_1(int2) TO databaseuser system;


--
set ivorysql.grant_usertype to oracle;
show ivorysql.grant_usertype ;

--
--  error test
-- 47

GRANT SELECT ON regress_user_table_1 TO databaseuser system;
GRANT SELECT(col2) ON regress_user_table_1 TO databaseuser system;
GRANT SELECT,INSERT ON regress_user_table_1 TO databaseuser system;
GRANT DELETE ON regress_user_table_1 TO GROUP databaseuser regress_database_group;

REVOKE SELECT(col2) ON regress_user_table_1 FROM databaseuser system;

GRANT ALL ON SEQUENCE regress_user_seq_1 TO databaseuser system;
GRANT ALL ON regress_user_seq_1 TO databaseuser system;

GRANT ALL ON regress_user_seq2_1 TO databaseuser system;
GRANT ALL ON TYPE regress_user_type_1 TO databaseuser system;
GRANT EXECUTE ON FUNCTION regress_user_func_1() TO databaseuser system;
GRANT EXECUTE ON FUNCTION regress_user_agg_1(int2) TO databaseuser system;



--
--  error test
-- 48
GRANT SELECT ON regress_user_table_1 TO instanceuser instanceu;
GRANT SELECT(col2) ON regress_user_table_1 TO instanceuser instanceu;
GRANT SELECT,INSERT ON regress_user_table_1 TO instanceuser instanceu;
GRANT DELETE ON regress_user_table_1 TO GROUP instanceuser regress_instance_group;

REVOKE SELECT(col2) ON regress_user_table_1 FROM instanceuser instanceu;

GRANT ALL ON SEQUENCE regress_user_seq_1 TO instanceuser pg_signal_backend;
GRANT ALL ON regress_user_seq_1 TO instanceuser instanceu;

GRANT ALL ON regress_user_seq2_1 TO instanceuser instanceu;
GRANT ALL ON TYPE regress_user_type_1 TO instanceuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_func_1() TO instanceuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_agg_1(int2) TO instanceuser instanceu;



--
--  error test
-- 49
GRANT SELECT ON regress_user_table_1 TO databaseuser databaseu;
GRANT SELECT(col2) ON regress_user_table_1 TO databaseuser databaseu;
GRANT SELECT,INSERT ON regress_user_table_1 TO databaseuser databaseu;
GRANT DELETE ON regress_user_table_1 TO GROUP databaseuser regress_database_group;

REVOKE SELECT(col2) ON regress_user_table_1 FROM databaseuser databaseu;

GRANT ALL ON SEQUENCE regress_user_seq_1 TO databaseuser databaseu;
GRANT ALL ON regress_user_seq_1 TO databaseuser databaseu;

GRANT ALL ON regress_user_seq2_1 TO databaseuser databaseu;
GRANT ALL ON TYPE regress_user_type_1 TO databaseuser databaseu;
GRANT EXECUTE ON FUNCTION regress_user_func_1() TO databaseuser databaseu;
GRANT EXECUTE ON FUNCTION regress_user_agg_1(int2) TO databaseuser regress_database_test_ro


--
--  error test
-- 50
--
GRANT SELECT ON regress_user_table_1 TO databaseuser instanceu;
GRANT SELECT(col2) ON regress_user_table_1 TO databaseuser instanceu;
GRANT SELECT,INSERT ON regress_user_table_1 TO databaseuser instanceu;
GRANT DELETE ON regress_user_table_1 TO GROUP databaseuser regress_database_group;

REVOKE SELECT(col2) ON regress_user_table_1 FROM databaseuser instanceu;

GRANT ALL ON SEQUENCE regress_user_seq_1 TO databaseuser instanceu;
GRANT ALL ON regress_user_seq_1 TO databaseuser instanceu;

GRANT ALL ON regress_user_seq2_1 TO databaseuser instanceu;
GRANT ALL ON TYPE regress_user_type_1 TO databaseuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_func_1() TO databaseuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_agg_1(int2) TO databaseuser instanceu;


--
--  error test
-- 51
--
GRANT SELECT ON regress_user_table_1 TO instanceu;
GRANT SELECT(col2) ON regress_user_table_1 TO instanceu;
GRANT SELECT,INSERT ON regress_user_table_1 TO instanceu;
GRANT DELETE ON regress_user_table_1 TO GROUP regress_instance_group;

REVOKE SELECT(col2) ON regress_user_table_1 FROM instanceu;

GRANT ALL ON SEQUENCE regress_user_seq_1 TO pg_signal_backend;
GRANT ALL ON regress_user_seq_1 TO instanceu;

GRANT ALL ON regress_user_seq2_1 TO instanceu;
GRANT ALL ON TYPE regress_user_type_1 TO instanceu;
GRANT EXECUTE ON FUNCTION regress_user_func_1() TO instanceu;
GRANT EXECUTE ON FUNCTION regress_user_agg_1(int2) TO instanceu;


--
set ivorysql.grant_usertype to pg;
show ivorysql.grant_usertype ;

--

set ivorysql.compatible_mode to pg;
show ivorysql.compatible_mode;
set session authorization instanceu;

DROP TABLE regress_user_table_1;
DROP SEQUENCE regress_user_seq_1;
DROP SEQUENCE regress_user_seq2_1;
DROP TYPE regress_user_type_1;
DROP FUNCTION regress_user_func_1;
DROP AGGREGATE regress_user_agg_1(int2);

set ivorysql.compatible_mode to oracle;
show ivorysql.compatible_mode;
set session authorization databaseu;

-----------------------------------------------------------

--
--  success test
-- 30
GRANT SELECT ON regress_user_table_2 TO databaseu;
GRANT SELECT(col2) ON regress_user_table_2 TO databaseu;
GRANT SELECT,INSERT ON regress_user_table_2 TO databaseu;
GRANT DELETE ON regress_user_table_2 TO GROUP regress_database_group;

REVOKE SELECT(col2) ON regress_user_table_2 FROM databaseu;
GRANT ALL ON regress_user_seq_2 TO databaseu;

GRANT ALL ON regress_user_seq2_2 TO databaseu;
GRANT ALL ON TYPE regress_user_type_2 TO databaseu;
GRANT EXECUTE ON FUNCTION regress_user_func_2() TO databaseu;
GRANT EXECUTE ON FUNCTION regress_user_agg_2(int2) TO databaseu;


--
--  error test
-- 31
--
GRANT SELECT ON regress_user_table_2 TO instanceu;
GRANT SELECT(col2) ON regress_user_table_2 TO instanceu;
GRANT SELECT,INSERT ON regress_user_table_2 TO instanceu;
GRANT DELETE ON regress_user_table_2 TO GROUP regress_instance_group;

REVOKE SELECT(col2) ON regress_user_table_2 FROM instanceu;

GRANT ALL ON SEQUENCE regress_user_seq_2 TO pg_signal_backend;
GRANT ALL ON regress_user_seq_2 TO instanceu;

GRANT ALL ON regress_user_seq2_2 TO instanceu;
GRANT ALL ON TYPE regress_user_type_2 TO instanceu;
GRANT EXECUTE ON FUNCTION regress_user_func_2() TO instanceu;
GRANT EXECUTE ON FUNCTION regress_user_agg_2(int2) TO instanceu;


--
--  error test
-- 32

GRANT SELECT ON regress_user_table_2 TO databaseuser databaseu;
GRANT SELECT(col2) ON regress_user_table_2 TO databaseuser databaseu;
GRANT SELECT,INSERT ON regress_user_table_2 TO databaseuser databaseu;
GRANT DELETE ON regress_user_table_2 TO GROUP databaseuser regress_database_group;

REVOKE SELECT(col2) ON regress_user_table_2 FROM databaseuser databaseu;
GRANT ALL ON regress_user_seq_2 TO databaseuser databaseu;

GRANT ALL ON regress_user_seq2_2 TO databaseuser databaseu;
GRANT ALL ON TYPE regress_user_type_2 TO databaseuser databaseu;
GRANT EXECUTE ON FUNCTION regress_user_func_2() TO databaseuser databaseu;
GRANT EXECUTE ON FUNCTION regress_user_agg_2(int2) TO databaseuser databaseu;



--
--  error test
-- 33
--
GRANT SELECT ON regress_user_table_2 TO instanceuser instanceu;
GRANT SELECT(col2) ON regress_user_table_2 TO instanceuser instanceu;
GRANT SELECT,INSERT ON regress_user_table_2 TO instanceuser instanceu;
GRANT DELETE ON regress_user_table_2 TO GROUP instanceuser regress_instance_group;

REVOKE SELECT(col2) ON regress_user_table_2 FROM instanceuser instanceu;

GRANT ALL ON SEQUENCE regress_user_seq_2 TO instanceuser pg_signal_backend;
GRANT ALL ON regress_user_seq_2 TO instanceuser instanceu;

GRANT ALL ON regress_user_seq2_2 TO instanceuser instanceu;
GRANT ALL ON TYPE regress_user_type_2 TO instanceuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_func_2() TO instanceuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_agg_2(int2) TO instanceuser instanceu;


--
--  error test
-- 34
GRANT SELECT ON regress_user_table_2 TO databaseuser instanceu;
GRANT SELECT(col2) ON regress_user_table_2 TO databaseuser instanceu;
GRANT SELECT,INSERT ON regress_user_table_2 TO databaseuser instanceu;
GRANT DELETE ON regress_user_table_2 TO GROUP databaseuser regress_instance_group;

REVOKE SELECT(col2) ON regress_user_table_2 FROM databaseuser instanceu;

GRANT ALL ON SEQUENCE regress_user_seq_2 TO databaseuser pg_signal_backend;
GRANT ALL ON regress_user_seq_2 TO databaseuser instanceu;

GRANT ALL ON regress_user_seq2_2 TO databaseuser instanceu;
GRANT ALL ON TYPE regress_user_type_2 TO databaseuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_func_2() TO databaseuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_agg_2(int2) TO databaseuser instanceu;


--
--  error test
-- 35
--
GRANT SELECT ON regress_user_table_2 TO databaseuser system;
GRANT SELECT(col2) ON regress_user_table_2 TO databaseuser system;
GRANT SELECT,INSERT ON regress_user_table_2 TO databaseuser system;
GRANT DELETE ON regress_user_table_2 TO GROUP databaseuser regress_database_group;

REVOKE SELECT(col2) ON regress_user_table_2 FROM databaseuser system;

GRANT ALL ON SEQUENCE regress_user_seq_2 TO databaseuser system;
GRANT ALL ON regress_user_seq_2 TO databaseuser system;

GRANT ALL ON regress_user_seq2_2 TO databaseuser system;
GRANT ALL ON TYPE regress_user_type_2 TO databaseuser system;
GRANT EXECUTE ON FUNCTION regress_user_func_2() TO databaseuser system;
GRANT EXECUTE ON FUNCTION regress_user_agg_2(int2) TO databaseuser system;


--
set ivorysql.grant_usertype to oracle;
show ivorysql.grant_usertype ;

--
--  success test
-- 36
-- 
GRANT SELECT ON regress_user_table_2 TO databaseuser system;
GRANT SELECT(col2) ON regress_user_table_2 TO databaseuser system;
GRANT SELECT,INSERT ON regress_user_table_2 TO databaseuser system;
GRANT DELETE ON regress_user_table_2 TO GROUP databaseuser regress_database_group;

REVOKE SELECT(col2) ON regress_user_table_2 FROM databaseuser system;

GRANT ALL ON SEQUENCE regress_user_seq_2 TO databaseuser system;
GRANT ALL ON regress_user_seq_2 TO databaseuser system;

GRANT ALL ON regress_user_seq2_2 TO databaseuser system;
GRANT ALL ON TYPE regress_user_type_2 TO databaseuser system;
GRANT EXECUTE ON FUNCTION regress_user_func_2() TO databaseuser system;
GRANT EXECUTE ON FUNCTION regress_user_agg_2(int2) TO databaseuser system;



--
--  success test
-- 37
GRANT SELECT ON regress_user_table_2 TO instanceuser instanceu;
GRANT SELECT(col2) ON regress_user_table_2 TO instanceuser instanceu;
GRANT SELECT,INSERT ON regress_user_table_2 TO instanceuser instanceu;
GRANT DELETE ON regress_user_table_2 TO GROUP instanceuser regress_instance_group;

REVOKE SELECT(col2) ON regress_user_table_2 FROM instanceuser instanceu;

GRANT ALL ON SEQUENCE regress_user_seq_2 TO instanceuser pg_signal_backend;
GRANT ALL ON regress_user_seq_2 TO instanceuser instanceu;

GRANT ALL ON regress_user_seq2_2 TO instanceuser instanceu;
GRANT ALL ON TYPE regress_user_type_2 TO instanceuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_func_2() TO instanceuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_agg_2(int2) TO instanceuser instanceu;



--
--  success test
--38
GRANT SELECT ON regress_user_table_2 TO databaseuser databaseu;
GRANT SELECT(col2) ON regress_user_table_2 TO databaseuser databaseu;
GRANT SELECT,INSERT ON regress_user_table_2 TO databaseuser databaseu;
GRANT DELETE ON regress_user_table_2 TO GROUP databaseuser regress_database_group;

REVOKE SELECT(col2) ON regress_user_table_2 FROM databaseuser databaseu;

GRANT ALL ON SEQUENCE regress_user_seq_2 TO databaseuser databaseu;
GRANT ALL ON regress_user_seq_2 TO databaseuser databaseu;

GRANT ALL ON regress_user_seq2_2 TO databaseuser databaseu;
GRANT ALL ON TYPE regress_user_type_2 TO databaseuser databaseu;
GRANT EXECUTE ON FUNCTION regress_user_func_2() TO databaseuser databaseu;
GRANT EXECUTE ON FUNCTION regress_user_agg_2(int2) TO databaseuser databaseu;



--
--  error test
--39
--
GRANT SELECT ON regress_user_table_2 TO databaseuser instanceu;
GRANT SELECT(col2) ON regress_user_table_2 TO databaseuser instanceu;
GRANT SELECT,INSERT ON regress_user_table_2 TO databaseuser instanceu;
GRANT DELETE ON regress_user_table_2 TO GROUP databaseuser regress_database_group;

REVOKE SELECT(col2) ON regress_user_table_2 FROM databaseuser instanceu;

GRANT ALL ON SEQUENCE regress_user_seq_2 TO databaseuser instanceu;
GRANT ALL ON regress_user_seq_2 TO databaseuser instanceu;

GRANT ALL ON regress_user_seq2_2 TO databaseuser instanceu;
GRANT ALL ON TYPE regress_user_type_2 TO databaseuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_func_2() TO databaseuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_agg_2(int2) TO databaseuser instanceu;


--
--  error test
--  40
--
GRANT SELECT ON regress_user_table_2 TO instanceu;
GRANT SELECT(col2) ON regress_user_table_2 TO instanceu;
GRANT SELECT,INSERT ON regress_user_table_2 TO instanceu;
GRANT DELETE ON regress_user_table_2 TO GROUP regress_instance_group;

REVOKE SELECT(col2) ON regress_user_table_2 FROM instanceu;

GRANT ALL ON SEQUENCE regress_user_seq_2 TO pg_signal_backend;
GRANT ALL ON regress_user_seq_2 TO instanceu;

GRANT ALL ON regress_user_seq2_2 TO instanceu;
GRANT ALL ON TYPE regress_user_type_2 TO instanceu;
GRANT EXECUTE ON FUNCTION regress_user_func_2() TO instanceu;
GRANT EXECUTE ON FUNCTION regress_user_agg_2(int2) TO instanceu;



set ivorysql.compatible_mode to oracle;
show ivorysql.compatible_mode;
set session authorization databaseu;

--
--  success test
-- 41
GRANT SELECT ON regress_user_table_2 TO databaseu;
GRANT SELECT(col2) ON regress_user_table_2 TO databaseu;
GRANT SELECT,INSERT ON regress_user_table_2 TO databaseu;
GRANT DELETE ON regress_user_table_2 TO GROUP regress_database_group;

REVOKE SELECT(col2) ON regress_user_table_2 FROM databaseu;
GRANT ALL ON regress_user_seq_2 TO databaseu;

GRANT ALL ON regress_user_seq2_2 TO databaseu;
GRANT ALL ON TYPE regress_user_type_2 TO databaseu;
GRANT EXECUTE ON FUNCTION regress_user_func_2() TO databaseu;
GRANT EXECUTE ON FUNCTION regress_user_agg_2(int2) TO databaseu;


--
--  error test
-- 42
--
GRANT SELECT ON regress_user_table_2 TO instanceu;
GRANT SELECT(col2) ON regress_user_table_2 TO instanceu;
GRANT SELECT,INSERT ON regress_user_table_2 TO instanceu;
GRANT DELETE ON regress_user_table_2 TO GROUP regress_instance_group;

REVOKE SELECT(col2) ON regress_user_table_2 FROM instanceu;

GRANT ALL ON SEQUENCE regress_user_seq_2 TO pg_signal_backend;
GRANT ALL ON regress_user_seq_2 TO instanceu;

GRANT ALL ON regress_user_seq2_2 TO instanceu;
GRANT ALL ON TYPE regress_user_type_2 TO instanceu;
GRANT EXECUTE ON FUNCTION regress_user_func_2() TO instanceu;
GRANT EXECUTE ON FUNCTION regress_user_agg_2(int2) TO instanceu;


--
--  success test
-- 43
GRANT SELECT ON regress_user_table_2 TO databaseuser databaseu;
GRANT SELECT(col2) ON regress_user_table_2 TO databaseuser databaseu;
GRANT SELECT,INSERT ON regress_user_table_2 TO databaseuser databaseu;
GRANT DELETE ON regress_user_table_2 TO GROUP databaseuser regress_database_group;

REVOKE SELECT(col2) ON regress_user_table_2 FROM databaseuser databaseu;
GRANT ALL ON regress_user_seq_2 TO databaseuser databaseu;

GRANT ALL ON regress_user_seq2_2 TO databaseuser databaseu;
GRANT ALL ON TYPE regress_user_type_2 TO databaseuser databaseu;
GRANT EXECUTE ON FUNCTION regress_user_func_2() TO databaseuser databaseu;
GRANT EXECUTE ON FUNCTION regress_user_agg_2(int2) TO databaseuser databaseu;



--
--  success test
-- 44 
--
GRANT SELECT ON regress_user_table_2 TO instanceuser instanceu;
GRANT SELECT(col2) ON regress_user_table_2 TO instanceuser instanceu;
GRANT SELECT,INSERT ON regress_user_table_2 TO instanceuser instanceu;
GRANT DELETE ON regress_user_table_2 TO GROUP instanceuser regress_instance_group;

REVOKE SELECT(col2) ON regress_user_table_2 FROM instanceuser instanceu;

GRANT ALL ON SEQUENCE regress_user_seq_2 TO instanceuser pg_signal_backend;
GRANT ALL ON regress_user_seq_2 TO instanceuser instanceu;

GRANT ALL ON regress_user_seq2_2 TO instanceuser instanceu;
GRANT ALL ON TYPE regress_user_type_2 TO instanceuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_func_2() TO instanceuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_agg_2(int2) TO instanceuser instanceu;


--
--  error test
-- 45
GRANT SELECT ON regress_user_table_2 TO databaseuser instanceu;
GRANT SELECT(col2) ON regress_user_table_2 TO databaseuser instanceu;
GRANT SELECT,INSERT ON regress_user_table_2 TO databaseuser instanceu;
GRANT DELETE ON regress_user_table_2 TO GROUP databaseuser regress_instance_group;

REVOKE SELECT(col2) ON regress_user_table_2 FROM databaseuser instanceu;

GRANT ALL ON SEQUENCE regress_user_seq_2 TO databaseuser pg_signal_backend;
GRANT ALL ON regress_user_seq_2 TO databaseuser instanceu;

GRANT ALL ON regress_user_seq2_2 TO databaseuser instanceu;
GRANT ALL ON TYPE regress_user_type_2 TO databaseuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_func_2() TO databaseuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_agg_2(int2) TO databaseuser instanceu;


--
--  success test
-- 46
--
GRANT SELECT ON regress_user_table_2 TO databaseuser system;
GRANT SELECT(col2) ON regress_user_table_2 TO databaseuser system;
GRANT SELECT,INSERT ON regress_user_table_2 TO databaseuser system;
GRANT DELETE ON regress_user_table_2 TO GROUP databaseuser regress_database_group;

REVOKE SELECT(col2) ON regress_user_table_2 FROM databaseuser system;

GRANT ALL ON SEQUENCE regress_user_seq_2 TO databaseuser system;
GRANT ALL ON regress_user_seq_2 TO databaseuser system;

GRANT ALL ON regress_user_seq2_2 TO databaseuser system;
GRANT ALL ON TYPE regress_user_type_2 TO databaseuser system;
GRANT EXECUTE ON FUNCTION regress_user_func_2() TO databaseuser system;
GRANT EXECUTE ON FUNCTION regress_user_agg_2(int2) TO databaseuser system;


--
set ivorysql.grant_usertype to oracle;
show ivorysql.grant_usertype ;

--
--  success test
-- 47

GRANT SELECT ON regress_user_table_2 TO databaseuser system;
GRANT SELECT(col2) ON regress_user_table_2 TO databaseuser system;
GRANT SELECT,INSERT ON regress_user_table_2 TO databaseuser system;
GRANT DELETE ON regress_user_table_2 TO GROUP databaseuser regress_database_group;

REVOKE SELECT(col2) ON regress_user_table_2 FROM databaseuser system;

GRANT ALL ON SEQUENCE regress_user_seq_2 TO databaseuser system;
GRANT ALL ON regress_user_seq_2 TO databaseuser system;

GRANT ALL ON regress_user_seq2_2 TO databaseuser system;
GRANT ALL ON TYPE regress_user_type_2 TO databaseuser system;
GRANT EXECUTE ON FUNCTION regress_user_func_2() TO databaseuser system;
GRANT EXECUTE ON FUNCTION regress_user_agg_2(int2) TO databaseuser system;



--
--  success test
-- 48
GRANT SELECT ON regress_user_table_2 TO instanceuser instanceu;
GRANT SELECT(col2) ON regress_user_table_2 TO instanceuser instanceu;
GRANT SELECT,INSERT ON regress_user_table_2 TO instanceuser instanceu;
GRANT DELETE ON regress_user_table_2 TO GROUP instanceuser regress_instance_group;

REVOKE SELECT(col2) ON regress_user_table_2 FROM instanceuser instanceu;

GRANT ALL ON SEQUENCE regress_user_seq_2 TO instanceuser pg_signal_backend;
GRANT ALL ON regress_user_seq_2 TO instanceuser instanceu;

GRANT ALL ON regress_user_seq2_2 TO instanceuser instanceu;
GRANT ALL ON TYPE regress_user_type_2 TO instanceuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_func_2() TO instanceuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_agg_2(int2) TO instanceuser instanceu;



--
--  success test
-- 49
GRANT SELECT ON regress_user_table_2 TO databaseuser databaseu;
GRANT SELECT(col2) ON regress_user_table_2 TO databaseuser databaseu;
GRANT SELECT,INSERT ON regress_user_table_2 TO databaseuser databaseu;
GRANT DELETE ON regress_user_table_2 TO GROUP databaseuser regress_database_group;

REVOKE SELECT(col2) ON regress_user_table_2 FROM databaseuser databaseu;

GRANT ALL ON SEQUENCE regress_user_seq_2 TO databaseuser databaseu;
GRANT ALL ON regress_user_seq_2 TO databaseuser databaseu;

GRANT ALL ON regress_user_seq2_2 TO databaseuser databaseu;
GRANT ALL ON TYPE regress_user_type_2 TO databaseuser databaseu;
GRANT EXECUTE ON FUNCTION regress_user_func_2() TO databaseuser databaseu;
GRANT EXECUTE ON FUNCTION regress_user_agg_2(int2) TO databaseuser databaseu;



--
--  error test
-- 50
--
GRANT SELECT ON regress_user_table_2 TO databaseuser instanceu;
GRANT SELECT(col2) ON regress_user_table_2 TO databaseuser instanceu;
GRANT SELECT,INSERT ON regress_user_table_2 TO databaseuser instanceu;
GRANT DELETE ON regress_user_table_2 TO GROUP databaseuser regress_database_group;

REVOKE SELECT(col2) ON regress_user_table_2 FROM databaseuser instanceu;

GRANT ALL ON SEQUENCE regress_user_seq_2 TO databaseuser instanceu;
GRANT ALL ON regress_user_seq_2 TO databaseuser instanceu;

GRANT ALL ON regress_user_seq2_2 TO databaseuser instanceu;
GRANT ALL ON TYPE regress_user_type_2 TO databaseuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_func_2() TO databaseuser instanceu;
GRANT EXECUTE ON FUNCTION regress_user_agg_2(int2) TO databaseuser instanceu;


--
--  error test
-- 51
--
GRANT SELECT ON regress_user_table_2 TO instanceu;
GRANT SELECT(col2) ON regress_user_table_2 TO instanceu;
GRANT SELECT,INSERT ON regress_user_table_2 TO instanceu;
GRANT DELETE ON regress_user_table_2 TO GROUP regress_instance_group;

REVOKE SELECT(col2) ON regress_user_table_2 FROM instanceu;

GRANT ALL ON SEQUENCE regress_user_seq_2 TO pg_signal_backend;
GRANT ALL ON regress_user_seq_2 TO instanceu;

GRANT ALL ON regress_user_seq2_2 TO instanceu;
GRANT ALL ON TYPE regress_user_type_2 TO instanceu;
GRANT EXECUTE ON FUNCTION regress_user_func_2() TO instanceu;
GRANT EXECUTE ON FUNCTION regress_user_agg_2(int2) TO instanceu;


--
set ivorysql.grant_usertype to pg;
show ivorysql.grant_usertype ;

--
DROP TABLE regress_user_table_2;
DROP SEQUENCE regress_user_seq_2;
DROP SEQUENCE regress_user_seq2_2;
DROP TYPE regress_user_type_2;
DROP FUNCTION regress_user_func_2;
DROP AGGREGATE regress_user_agg_2(int2);

--
-- GRANT xxx TO xxx
--
-- databaseuser
set session authorization system;
CREATE USER regress_databaseuser;
CREATE GROUP regress_databasegroup;
set session authorization regress_databaseuser;

set ivorysql.compatible_mode to pg;
show ivorysql.compatible_mode;
select dbuser_function();
-- instanceuser
CREATE USER regress_instanceuser;
CREATE GROUP regress_instancegroup;

--set session authorization regress_instanceuser;

--
--  error test
-- 52
GRANT regress_databasegroup TO regress_databaseuser WITH ADMIN OPTION;
GRANT pg_database_owner TO regress_databaseuser;
GRANT regress_databaseuser TO pg_database_owner;

REVOKE regress_databasegroup FROM regress_databaseuser;

--
--  success test
-- 53
GRANT regress_instancegroup TO regress_instanceuser WITH ADMIN OPTION;      -- success 
GRANT pg_database_owner TO regress_instanceuser;        --  error 
GRANT regress_instanceuser TO pg_database_owner;        --  error 

REVOKE regress_instancegroup FROM regress_instanceuser;

--
--  error test
-- 54
GRANT databaseuser regress_instancegroup TO databaseuser regress_instanceuser WITH ADMIN OPTION;
GRANT databaseuser pg_database_owner TO databaseuser regress_instanceuser;
GRANT databaseuser regress_instanceuser TO databaseuser pg_database_owner;

--
--  error test
-- 55
GRANT instanceuser regress_instancegroup TO instanceuser regress_instanceuser WITH ADMIN OPTION;
GRANT instanceuser pg_database_owner TO instanceuser regress_instanceuser;
GRANT instanceuser regress_instanceuser TO instanceuser pg_database_owner;

--
--  error test
-- 56
GRANT databaseuser regress_databasegroup TO databaseuser regress_databaseuser WITH ADMIN OPTION;
GRANT instanceuser pg_database_owner TO databaseuser regress_databaseuser;
GRANT databaseuser regress_databaseuser TO instanceuser pg_database_owner;



set ivorysql.grant_usertype to oracle;
show ivorysql.grant_usertype ;

--
--  error test
-- 57
GRANT regress_databasegroup TO regress_databaseuser WITH ADMIN OPTION ;
GRANT pg_database_owner TO regress_databaseuser;
GRANT regress_databaseuser TO pg_database_owner;

REVOKE regress_databasegroup FROM regress_databaseuser;

--
--  error test
-- 58
GRANT instanceuser regress_instancegroup TO instanceuser regress_instanceuser WITH ADMIN OPTION;
GRANT instanceuser pg_database_owner TO instanceuser regress_instanceuser;
GRANT instanceuser regress_instanceuser TO instanceuser pg_database_owner;

--
--  error test
-- 59
GRANT databaseuser regress_databasegroup TO databaseuser regress_databaseuser WITH ADMIN OPTION;
GRANT instanceuser pg_database_owner TO databaseuser regress_databaseuser;
GRANT databaseuser regress_databaseuser TO instanceuser pg_database_owner;

--
--  error test
-- 60
GRANT databaseuser regress_instancegroup TO databaseuser regress_instanceuser WITH ADMIN OPTION; 
GRANT databaseuser pg_database_owner TO databaseuser regress_instanceuser;
GRANT databaseuser regress_instanceuser TO databaseuser pg_database_owner;

--
--  sucess test
-- 61
GRANT regress_instancegroup TO regress_instanceuser WITH ADMIN OPTION;    --  sucess
GRANT pg_database_owner TO regress_instanceuser;        --  error
GRANT regress_instanceuser TO pg_database_owner;        --  error

REVOKE regress_instancegroup FROM regress_instanceuser;


set ivorysql.compatible_mode to oracle;
show ivorysql.compatible_mode;
set session authorization system;

--
--  sucess test
-- 62
GRANT regress_databasegroup TO regress_databaseuser WITH ADMIN OPTION;      --  sucess
GRANT pg_database_owner TO regress_databaseuser;        --  error
GRANT regress_databaseuser TO pg_database_owner;        --  error

REVOKE regress_databasegroup FROM regress_databaseuser;

--
--  error test
-- 63
GRANT regress_instancegroup TO regress_instanceuser WITH ADMIN OPTION;
GRANT pg_database_owner TO regress_instanceuser;
GRANT regress_instanceuser TO pg_database_owner;

REVOKE regress_instancegroup FROM regress_instanceuser;

--
--  error test
-- 64
GRANT databaseuser regress_instancegroup TO databaseuser regress_instanceuser WITH ADMIN OPTION;
GRANT databaseuser pg_database_owner TO databaseuser regress_instanceuser;
GRANT databaseuser regress_instanceuser TO databaseuser pg_database_owner;

--
--  error test
-- 65 
GRANT instanceuser regress_instancegroup TO instanceuser regress_instanceuser WITH ADMIN OPTION;
GRANT instanceuser pg_database_owner TO instanceuser regress_instanceuser;
GRANT instanceuser regress_instanceuser TO instanceuser pg_database_owner;

--
--  error test
-- 66
GRANT databaseuser regress_databasegroup TO databaseuser regress_databaseuser WITH ADMIN OPTION;
GRANT instanceuser pg_database_owner TO databaseuser regress_databaseuser;
GRANT databaseuser regress_databaseuser TO instanceuser pg_database_owner;


set ivorysql.grant_usertype to oracle;
show ivorysql.grant_usertype ;

--
--  success test
-- 67
GRANT regress_databasegroup TO regress_databaseuser WITH ADMIN OPTION;      --  success
GRANT pg_database_owner TO regress_databaseuser;        --  error
GRANT regress_databaseuser TO pg_database_owner;        --  error 

REVOKE regress_databasegroup FROM regress_databaseuser;

--
--  error test
-- 68
GRANT instanceuser regress_instancegroup TO instanceuser regress_instanceuser WITH ADMIN OPTION;      --  success
GRANT instanceuser pg_database_owner TO instanceuser regress_instanceuser;        --  error
GRANT instanceuser regress_instanceuser TO instanceuser pg_database_owner;        --  error

REVOKE instanceuser regress_instancegroup FROM instanceuser regress_instanceuser;

--
--  error test
-- 69
GRANT databaseuser regress_databasegroup TO databaseuser regress_databaseuser WITH ADMIN OPTION;      --  success
GRANT instanceuser pg_database_owner TO databaseuser regress_databaseuser;        --  error
GRANT databaseuser regress_databaseuser TO instanceuser pg_database_owner;        --  error

REVOKE databaseuser regress_databasegroup FROM databaseuser regress_databaseuser;

--
--  error test
-- 70
GRANT databaseuser regress_instancegroup TO databaseuser regress_instanceuser WITH ADMIN OPTION;
GRANT databaseuser pg_database_owner TO databaseuser regress_instanceuser;
GRANT databaseuser regress_instanceuser TO databaseuser pg_database_owner;


--
--  error test
-- 71
GRANT regress_instancegroup TO regress_instanceuser WITH ADMIN OPTION;
GRANT pg_database_owner TO regress_instanceuser;
GRANT regress_instanceuser TO pg_database_owner;

REVOKE regress_instancegroup FROM regress_instanceuser;

set ivorysql.grant_usertype to pg;
show ivorysql.grant_usertype ;


--
-- ALTER DEFAULT PRIVILEGES GRANT xxx ON xxx TO xxx
-- 

set ivorysql.compatible_mode to pg;
show ivorysql.compatible_mode;
set session authorization regress_instanceuser;

--
--  error test
-- 72
ALTER DEFAULT PRIVILEGES GRANT ALL ON FUNCTIONS TO regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SCHEMAS TO regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SEQUENCES TO regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TABLES TO regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TYPES TO regress_databaseuser;

ALTER DEFAULT PRIVILEGES REVOKE ALL ON FUNCTIONS FROM regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SCHEMAS FROM regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SEQUENCES FROM regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TABLES FROM regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TYPES FROM regress_databaseuser;

--
--  success test
-- 73 
BEGIN;
ALTER DEFAULT PRIVILEGES GRANT ALL ON FUNCTIONS TO regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SCHEMAS TO regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SEQUENCES TO regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TABLES TO regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TYPES TO regress_instanceuser;

ALTER DEFAULT PRIVILEGES REVOKE ALL ON FUNCTIONS FROM regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SCHEMAS FROM regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SEQUENCES FROM regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TABLES FROM regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TYPES FROM regress_instanceuser;
ROLLBACK;
--
--  error test
-- 74
ALTER DEFAULT PRIVILEGES GRANT ALL ON FUNCTIONS TO databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SCHEMAS TO databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SEQUENCES TO databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TABLES TO databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TYPES TO databaseuser regress_databaseuser;

ALTER DEFAULT PRIVILEGES REVOKE ALL ON FUNCTIONS FROM databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SCHEMAS FROM databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SEQUENCES FROM databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TABLES FROM databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TYPES FROM databaseuser regress_databaseuser;

--
--  error test
-- 75
ALTER DEFAULT PRIVILEGES GRANT ALL ON FUNCTIONS TO instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SCHEMAS TO instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SEQUENCES TO instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TABLES TO instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TYPES TO instanceuser regress_instanceuser;

ALTER DEFAULT PRIVILEGES REVOKE ALL ON FUNCTIONS FROM instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SCHEMAS FROM instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SEQUENCES FROM instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TABLES FROM instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TYPES FROM instanceuser regress_instanceuser;

set ivorysql.grant_usertype to oracle;
show ivorysql.grant_usertype ;

--
--  error test
-- 76
ALTER DEFAULT PRIVILEGES GRANT ALL ON FUNCTIONS TO regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SCHEMAS TO regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SEQUENCES TO regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TABLES TO regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TYPES TO regress_databaseuser;

ALTER DEFAULT PRIVILEGES REVOKE ALL ON FUNCTIONS FROM regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SCHEMAS FROM regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SEQUENCES FROM regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TABLES FROM regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TYPES FROM regress_databaseuser;

--
--  success test
-- 77
BEGIN;
ALTER DEFAULT PRIVILEGES GRANT ALL ON FUNCTIONS TO regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SCHEMAS TO regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SEQUENCES TO regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TABLES TO regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TYPES TO regress_instanceuser;

ALTER DEFAULT PRIVILEGES REVOKE ALL ON FUNCTIONS FROM regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SCHEMAS FROM regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SEQUENCES FROM regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TABLES FROM regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TYPES FROM regress_instanceuser;
ROLLBACK;
--
--  error test
-- 78
ALTER DEFAULT PRIVILEGES GRANT ALL ON FUNCTIONS TO databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SCHEMAS TO databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SEQUENCES TO databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TABLES TO databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TYPES TO databaseuser regress_databaseuser;

ALTER DEFAULT PRIVILEGES REVOKE ALL ON FUNCTIONS FROM databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SCHEMAS FROM databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SEQUENCES FROM databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TABLES FROM databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TYPES FROM databaseuser regress_databaseuser;

--
--  error test
-- 79
ALTER DEFAULT PRIVILEGES GRANT ALL ON FUNCTIONS TO instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SCHEMAS TO instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SEQUENCES TO instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TABLES TO instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TYPES TO instanceuser regress_instanceuser;

ALTER DEFAULT PRIVILEGES REVOKE ALL ON FUNCTIONS FROM instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SCHEMAS FROM instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SEQUENCES FROM instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TABLES FROM instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TYPES FROM instanceuser regress_instanceuser;

set ivorysql.compatible_mode to oracle;
show ivorysql.compatible_mode;
set session authorization regress_databaseuser;

--
--  success test
-- 80
BEGIN;
ALTER DEFAULT PRIVILEGES GRANT ALL ON FUNCTIONS TO regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SCHEMAS TO regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SEQUENCES TO regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TABLES TO regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TYPES TO regress_databaseuser;

ALTER DEFAULT PRIVILEGES REVOKE ALL ON FUNCTIONS FROM regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SCHEMAS FROM regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SEQUENCES FROM regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TABLES FROM regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TYPES FROM regress_databaseuser;
ROLLBACK;
--
--  error test
-- 81
ALTER DEFAULT PRIVILEGES GRANT ALL ON FUNCTIONS TO regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SCHEMAS TO regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SEQUENCES TO regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TABLES TO regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TYPES TO regress_instanceuser;

ALTER DEFAULT PRIVILEGES REVOKE ALL ON FUNCTIONS FROM regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SCHEMAS FROM regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SEQUENCES FROM regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TABLES FROM regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TYPES FROM regress_instanceuser;

--
--  error test
-- 82
ALTER DEFAULT PRIVILEGES GRANT ALL ON FUNCTIONS TO databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SCHEMAS TO databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SEQUENCES TO databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TABLES TO databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TYPES TO databaseuser regress_databaseuser;

ALTER DEFAULT PRIVILEGES REVOKE ALL ON FUNCTIONS FROM databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SCHEMAS FROM databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SEQUENCES FROM databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TABLES FROM databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TYPES FROM databaseuser regress_databaseuser;

--
--  error test
-- 83 
ALTER DEFAULT PRIVILEGES GRANT ALL ON FUNCTIONS TO instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SCHEMAS TO instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SEQUENCES TO instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TABLES TO instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TYPES TO instanceuser regress_instanceuser;

ALTER DEFAULT PRIVILEGES REVOKE ALL ON FUNCTIONS FROM instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SCHEMAS FROM instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SEQUENCES FROM instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TABLES FROM instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TYPES FROM instanceuser regress_instanceuser;

set ivorysql.grant_usertype to oracle;
show ivorysql.grant_usertype ;

--
--  success test
-- 84
BEGIN;
ALTER DEFAULT PRIVILEGES GRANT ALL ON FUNCTIONS TO regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SCHEMAS TO regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SEQUENCES TO regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TABLES TO regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TYPES TO regress_databaseuser;

ALTER DEFAULT PRIVILEGES REVOKE ALL ON FUNCTIONS FROM regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SCHEMAS FROM regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SEQUENCES FROM regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TABLES FROM regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TYPES FROM regress_databaseuser;
ROLLBACK;

--
--  error test
-- 85
ALTER DEFAULT PRIVILEGES GRANT ALL ON FUNCTIONS TO regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SCHEMAS TO regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SEQUENCES TO regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TABLES TO regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TYPES TO regress_instanceuser;

ALTER DEFAULT PRIVILEGES REVOKE ALL ON FUNCTIONS FROM regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SCHEMAS FROM regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SEQUENCES FROM regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TABLES FROM regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TYPES FROM regress_instanceuser;

--
--  success test
-- 86
BEGIN;
ALTER DEFAULT PRIVILEGES GRANT ALL ON FUNCTIONS TO databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SCHEMAS TO databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SEQUENCES TO databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TABLES TO databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TYPES TO databaseuser regress_databaseuser;

ALTER DEFAULT PRIVILEGES REVOKE ALL ON FUNCTIONS FROM databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SCHEMAS FROM databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SEQUENCES FROM databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TABLES FROM databaseuser regress_databaseuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TYPES FROM databaseuser regress_databaseuser;
ROLLBACK;

--
--  success test
-- 87
BEGIN;
ALTER DEFAULT PRIVILEGES GRANT ALL ON FUNCTIONS TO instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SCHEMAS TO instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON SEQUENCES TO instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TABLES TO instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES GRANT ALL ON TYPES TO instanceuser regress_instanceuser;

ALTER DEFAULT PRIVILEGES REVOKE ALL ON FUNCTIONS FROM instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SCHEMAS FROM instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON SEQUENCES FROM instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TABLES FROM instanceuser regress_instanceuser;
ALTER DEFAULT PRIVILEGES REVOKE ALL ON TYPES FROM instanceuser regress_instanceuser;
ROLLBACK;

set ivorysql.grant_usertype to pg;


set ivorysql.compatible_mode to pg;
show ivorysql.compatible_mode;
select dbuser_function();

drop user regress_instanceuser;
drop user instanceu;
drop user regress_instance_test_user;
drop role regress_instancegroup;
drop role regress_instance_group;


set ivorysql.compatible_mode to oracle;
show ivorysql.compatible_mode;
set session authorization system;

drop user regress_databaseuser;
drop user databaseu;
drop user regress_database_test_user;
drop role regress_databasegroup;
drop role regress_database_group;

