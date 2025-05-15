
/*****************************************************************
 *
 * Test DBLINK feature
 *
 ****************************************************************/

--add enrol11 user
CREATE USER enrol11 WITH superuser;

set ivorysql.compatible_mode to pg;
show ivorysql.compatible_mode;

create database tdblnk;

set ivorysql.compatible_mode to oracle;
show ivorysql.compatible_mode;

\c tdblnk enrol11
create schema enrol11;

ALTER USER enrol11 with password 'ivy123';

CREATE TABLE it1(c1 int, c2 varchar2(10));
CREATE TABLE it2(b1 int, b2 int);
CREATE TABLE it3(a varchar2(20), b number);
CREATE TABLE it4(x number, y varchar2(10));
CREATE TABLE it5(m varchar2(10), n number(5), e blob);


INSERT INTO it1 VALUES (11, 'DBLINK'), (1, 'DATATYPE'), (3, 'PACKAGE'), (7, 'SEQUENCE');
INSERT INTO it2 VALUES (1, 1001), (12, 1003), (5, 7), (3, 1005);
INSERT INTO it3 VALUES ('zzz', 26), ('ccc', 3), ('ddd', 4);
INSERT INTO it4 VALUES (100, 'One'), (10, 'Text'), (200, 'Postgres');
INSERT INTO it5(m, n) VALUES ('shared', 99), ('private', 909), ('public', 999);


-- create view
CREATE VIEW vit1 AS SELECT * FROM it1 WHERE c1 < 10;

-- create materialized view
CREATE MATERIALIZED VIEW mivt2 AS SELECT * FROM it2;


\c regression

CREATE TABLE lptb(a int);
INSERT INTO lptb VALUES (1), (100), (1000);

-- create a public DBLINK
show ivorysql.default_dblink_fdw; -- postgres_fdw is default fdw

--
-- Fixed User
--
-- create public dblink
DO $d$
    BEGIN
        EXECUTE $$ create public database link ivyfuplnk
			connect to enrol11 identified by "ivy123"
        	using '(DESCRIPTION =
            (ADDRESS = (PROTOCOL = TCP)(PORT = $$||current_setting('ivorysql.port')||$$))
            (CONNECT_DATA =
                (SERVER = DEDICATED)
                (SERVICE_NAME = tdblnk)
            ))'$$;
    END;
$d$;

-- create private dblink
DO $d$
    BEGIN
        EXECUTE $$ create database link ivyfurlnk
			connect to enrol11 identified by "ivy123"
        	using '(DESCRIPTION =
            (ADDRESS = (PROTOCOL = TCP)(PORT = $$||current_setting('ivorysql.port')||$$))
            (CONNECT_DATA =
                (SERVER = DEDICATED)
                (SERVICE_NAME = tdblnk)
            ))'$$;
    END;
$d$;


--
-- CURRENT_USER
--
DO $d$
    BEGIN
        EXECUTE $$ create public database link ivycurrent
        	connect to CURRENT_USER
        	using '(DESCRIPTION =
            (ADDRESS = (PROTOCOL = TCP)(PORT = $$||current_setting('ivorysql.port')||$$))
            (CONNECT_DATA =
                (SERVER = DEDICATED)
                (SERVICE_NAME = tdblnk)
            ))'$$;
    END;
$d$;

-- must be create a user mapping for current_user DBLINK before using this link
create user mapping for public
 server ivycurrent options (user 'enrol11', password 'ivy123');


--
-- Connected User
--
DO $d$
    BEGIN
        EXECUTE $$ create public database link ivyconned
        	using '(DESCRIPTION =
            (ADDRESS = (PROTOCOL = TCP)(PORT = $$||current_setting('ivorysql.port')||$$))
            (CONNECT_DATA =
                (SERVER = DEDICATED)
                (SERVICE_NAME = tdblnk)
            ))'$$;
    END;
$d$;

-- must be create a user mapping for Connected User DBLINK before using this link
create user mapping for public
 server ivyconned options (user 'enrol11', password 'ivy123');


--
-- Shared DBLINK
--
DO $d$
    BEGIN
        EXECUTE $$ create shared public database link spglink
		connect to enrol11 identified by "ivy123"
		AUTHENTICATED BY enrol11 IDENTIFIED BY "ivy123"
        	using '(DESCRIPTION =
            (ADDRESS = (PROTOCOL = TCP)(PORT = $$||current_setting('ivorysql.port')||$$))
            (CONNECT_DATA =
                (SERVER = DEDICATED)
                (SERVICE_NAME = tdblnk)
            ))'$$;
    END;
$d$;

/*Begin - BUG#Z1391 */
CREATE OR REPLACE PROCEDURE ins_from_pglnk IS
BEGIN
    INSERT INTO it1@ivyfuplnk SELECT * FROM it1@ivyfuplnk;
END;
/
/*End - BUG#Z1391 */


-- access remote table it1, it2, it3, it4, it5 by DBLINKs
select * from it1@ivyfuplnk;
select * from it2@ivyfurlnk;
select * from it3@ivycurrent;
select * from it4@ivyconned;

select * from it5@spglink; -- error, open_links default is 4


-- access remote view
select * from vit1@ivyfuplnk;

-- access remote materialized view
select * from mivt2@ivyfuplnk;

-- test alter session close command
alter session close database link ivyfuplnk;

-- create local view base on dblink
create view vt1 as select c2 from it1@ivyfuplnk;

-- access local view vt1
select * from vt1;


-- where conditions
--select * from it1@ivyfuplnk where it1.c2@ivyfuplnk = 'PACKAGE';
select * from it1@ivyfuplnk where c2 = 'PACKAGE';

-- inner join two remote tables f2 and f3.
select * from it1@ivyfuplnk A inner join it2@ivyfuplnk B on A.c1 = B.b2;


-- update remote table by public dblink
update it1@ivyfuplnk set c2 = 'Itest' where c1 = 1;
select * from it1@ivyfuplnk;

-- delete remote table by public dblink
delete from it1@ivyfuplnk where c1 = 1;
select * from it1@ivyfuplnk;

/*Begin - BUG#Z1391 */
call ins_from_pglnk();
select * from it1@ivyfuplnk;
drop procedure ins_from_pglnk;
/*End - BUG#Z1391 */

-- test alter database link command
select umoptions from pg_user_mappings where srvname = 'ivyfurlnk';

alter database link ivyfurlnk connect to enrol11 identified by ivorysqlpro;

select umoptions from pg_user_mappings where srvname = 'ivyfurlnk';


-- drop link
drop database link ivyfurlnk;
drop public database link ivyfuplnk;
drop public database link ivycurrent;
drop public database link ivyconned;

-- access the shared DBLINK
select * from it5@spglink;

drop public database link spglink;


-- test global_names GUC
show global_names;

alter system set global_names = true;

show global_names;

DO $d$
    BEGIN
        EXECUTE $$ create public database link glolnk
			connect to enrol11 identified by "ivy123"
        	using '(DESCRIPTION =
            (ADDRESS = (PROTOCOL = TCP)(PORT = $$||current_setting('ivorysql.port')||$$))
            (CONNECT_DATA =
                (SERVER = DEDICATED)
                (SERVICE_NAME = tdblnk)
            ))'$$;
    END;
$d$;

select * from it1@glolnk; -- error


DO $d$
    BEGIN
        EXECUTE $$ create public database link tdblnk
			connect to enrol11 identified by "ivy123"
        	using '(DESCRIPTION =
            (ADDRESS = (PROTOCOL = TCP)(PORT = $$||current_setting('ivorysql.port')||$$))
            (CONNECT_DATA =
                (SERVER = DEDICATED)
                (SERVICE_NAME = tdblnk)
            ))'$$;
    END;
$d$;

select * from it1@tdblnk;

drop public database link glolnk;
drop public database link tdblnk;

show global_names;

alter system set global_names = false;

show global_names;


-- Test Global Name as a Loopback Database Link
select * from lptb@regression;


select current_user;

drop view vt1;
--drop extension postgres_fdw;
drop table lptb;


\c tdblnk enrol11
drop view vit1;
drop MATERIALIZED view mivt2;
drop table it1;
drop table it2;
drop table it3;
drop table it4;
drop table it5;
drop schema enrol11;


\c regression

set ivorysql.compatible_mode to pg;
show ivorysql.compatible_mode;
drop database tdblnk;
set ivorysql.compatible_mode to oracle;