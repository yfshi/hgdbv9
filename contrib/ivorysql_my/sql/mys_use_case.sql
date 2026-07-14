SET mysql.create_database_mode=schema;
CREATE SCHEMA TEST1;
USE TEST1;
SELECT  current_schema();
DROP SCHEMA TEST1;

CREATE DATABASE TEST1;
USE TEST1;
SELECT  current_schema();
DROP SCHEMA TEST1;


CREATE DATABASE IF NOT EXISTS TEST2;

USE TEST2;

SELECT  current_schema();

CREATE DATABASE IF NOT EXISTS TEST2;

DROP SCHEMA TEST2;

CREATE SCHEMA IF NOT EXISTS TEST2;

USE TEST2;

SELECT  current_schema();

CREATE SCHEMA IF NOT EXISTS TEST2;

DROP SCHEMA TEST2;
/*CHARACTER & SET COLLATE test  */
CREATE SCHEMA  TEST4  CHARACTER SET = "charactertest";

USE TEST4;

SELECT  current_schema();

DROP SCHEMA TEST4;

CREATE SCHEMA  TEST41  COLLATE  = "collatetest";

USE TEST41;

SELECT  current_schema();

DROP SCHEMA TEST41;

CREATE DATABASE  TEST5  CHARACTER SET = "charactertest";
USE TEST5;

SELECT  current_schema();

DROP SCHEMA TEST5;

CREATE DATABASE  TEST5  COLLATE  = "collatetest";

USE TEST5;

SELECT  current_schema();

DROP SCHEMA TEST5;
/* use object test */
CREATE SCHEMA TEST6;
CREATE TABLE TEST6.t(a int);
INSERT INTO TEST6.t VALUES (1);

CREATE SCHEMA TEST7;
CREATE TABLE TEST7.t(a int);
INSERT INTO TEST7.t VALUES (2);

USE TEST6;
SELECT a FROM t;

USE TEST7;
SELECT a FROM t;

DROP SCHEMA TEST6 CASCADE;
DROP SCHEMA TEST7 CASCADE;
/*use function test*/
CREATE FUNCTION pg_catalog.sche_f(a int)
    RETURNS INT
    RETURN 1;

CREATE SCHEMA SCTEST8;

CREATE FUNCTION SCTEST8.sche_f(a int)
    RETURNS INT
    RETURN 2;

USE  SCTEST8;
SELECT sche_f(0);

--BUG#1444
use aa;
create database aa;
use aa;
select current_schema();
drop schema aa;


create schema aa charset utf8;
drop schema aa;

create schema aa character set utf8;
drop schema aa;

create schema aa default charset utf8;
drop schema aa;

create schema aa default character set utf8;
drop schema aa;

create schema aa collate utf8_bin;
drop schema aa;

create schema aa default collate utf8_bin;
drop schema aa;

create schema aa charset utf8 collate utf8_bin charset utf8 collate utf8_bin;
drop schema aa;

create schema aa default charset utf8 default collate utf8_bin default charset utf8 default collate utf8_bin;
drop schema aa;
reset mysql.create_database_mode;