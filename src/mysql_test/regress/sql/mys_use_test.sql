SET mysql.create_database_mode=schema;
CREATE SCHEMA test1;
USE test1;
SELECT  current_schema();
DROP SCHEMA test1;

CREATE DATABASE test1;
USE test1;
SELECT  current_schema();
DROP SCHEMA test1;


CREATE DATABASE IF NOT EXISTS test2;

USE test2;

SELECT  current_schema();

CREATE DATABASE IF NOT EXISTS test2;

DROP SCHEMA test2;

CREATE SCHEMA IF NOT EXISTS test2;

USE test2;

SELECT  current_schema();

CREATE SCHEMA IF NOT EXISTS test2;

DROP SCHEMA test2;
/*CHARACTER & SET COLLATE test  */
CREATE SCHEMA  test4  CHARACTER SET = "charactertest";

USE test4;

SELECT  current_schema();

DROP SCHEMA test4;

CREATE SCHEMA  test41  COLLATE  = "collatetest";

USE test41;

SELECT  current_schema();

DROP SCHEMA test41;

CREATE DATABASE  test5  CHARACTER SET = "charactertest";
USE test5;

SELECT  current_schema();

DROP SCHEMA test5;

CREATE DATABASE  test5  COLLATE  = "collatetest";

USE test5;

SELECT  current_schema();

DROP SCHEMA test5;
/* use object test */
CREATE SCHEMA test6;
CREATE TABLE test6.t(a int);
INSERT INTO test6.t VALUES (1);

CREATE SCHEMA test7;
CREATE TABLE test7.t(a int);
INSERT INTO test7.t VALUES (2);

USE test6;
SELECT a FROM t;

USE test7;
SELECT a FROM t;

DROP SCHEMA test6 CASCADE;
DROP SCHEMA test7 CASCADE;
/*use function test*/
CREATE FUNCTION pg_catalog.sche_f(a int)
    RETURNS INT
    RETURN 1;

CREATE SCHEMA sctest8;

CREATE FUNCTION sctest8.sche_f(a int)
    RETURNS INT
    RETURN 2;

USE  sctest8;
SELECT sche_f(0);
reset mysql.create_database_mode;
