-- NOTE: when test this script, you must make sure that there is no diskquota launcher
-- process or diskquota worker process. i.e. `ps -ef | grep postgres | grep "\[diskquota]" | grep -v grep | wc -l`
-- returns 0
CREATE DATABASE dbx0 ;
CREATE DATABASE dbx1 ;
CREATE DATABASE dbx2 ;
CREATE DATABASE dbx3 ;
CREATE DATABASE dbx4 ;
CREATE DATABASE dbx5 ;
CREATE DATABASE dbx6 ;
CREATE DATABASE dbx7 ;


show max_worker_processes;


\c dbx0
CREATE EXTENSION diskquota;
CREATE SCHEMA SX;
CREATE TABLE SX.a(i int);
SELECT hg_security.set_schema_quota('SX', '1MB');
INSERT INTO SX.a values(generate_series(0, 100000));
INSERT INTO SX.a values(generate_series(0, 10));
DROP TABLE SX.a;

\c dbx1
CREATE EXTENSION diskquota;
CREATE SCHEMA SX;
CREATE TABLE SX.a(i int);
SELECT hg_security.set_schema_quota('SX', '1MB');
INSERT INTO SX.a values(generate_series(0, 100000));
INSERT INTO SX.a values(generate_series(0, 10));
DROP TABLE SX.a;

\c dbx2
CREATE EXTENSION diskquota;
CREATE SCHEMA SX;
CREATE TABLE SX.a(i int);
SELECT hg_security.set_schema_quota('SX', '1MB');
INSERT INTO SX.a values(generate_series(0, 100000));
INSERT INTO SX.a values(generate_series(0, 10));
DROP TABLE SX.a;

\c dbx3
CREATE EXTENSION diskquota;
CREATE SCHEMA SX;
CREATE TABLE SX.a(i int);
SELECT hg_security.set_schema_quota('SX', '1MB');
INSERT INTO SX.a values(generate_series(0, 100000));
INSERT INTO SX.a values(generate_series(0, 10));
DROP TABLE SX.a;

\c dbx4
CREATE EXTENSION diskquota;
CREATE SCHEMA SX;
CREATE TABLE SX.a(i int);
SELECT hg_security.set_schema_quota('SX', '1MB');
INSERT INTO SX.a values(generate_series(0, 100000));
INSERT INTO SX.a values(generate_series(0, 10));
DROP TABLE SX.a;

\c dbx5
CREATE EXTENSION diskquota;
CREATE SCHEMA SX;
CREATE TABLE SX.a(i int);
SELECT hg_security.set_schema_quota('SX', '1MB');
INSERT INTO SX.a values(generate_series(0, 100000));
INSERT INTO SX.a values(generate_series(0, 10));
DROP TABLE SX.a;

\c dbx6
CREATE EXTENSION diskquota;
CREATE SCHEMA SX;
CREATE TABLE SX.a(i int);
SELECT hg_security.set_schema_quota('SX', '1MB');
INSERT INTO SX.a values(generate_series(0, 100000));
INSERT INTO SX.a values(generate_series(0, 10));
DROP TABLE SX.a;

\c dbx7
CREATE EXTENSION diskquota;
CREATE SCHEMA SX;
CREATE TABLE SX.a(i int);
SELECT hg_security.set_schema_quota('SX', '1MB');
INSERT INTO SX.a values(generate_series(0, 100000));
INSERT INTO SX.a values(generate_series(0, 10));
DROP TABLE SX.a;




\c dbx0
DROP EXTENSION diskquota;

\c dbx1
DROP EXTENSION diskquota;

\c dbx2
DROP EXTENSION diskquota;

\c dbx3
DROP EXTENSION diskquota;

\c dbx4
DROP EXTENSION diskquota;

\c dbx5
DROP EXTENSION diskquota;

\c dbx6
DROP EXTENSION diskquota;

\c dbx7
DROP EXTENSION diskquota;



\c postgres

DROP DATABASE dbx0 ;
DROP DATABASE dbx1 ;
DROP DATABASE dbx2 ;
DROP DATABASE dbx3 ;
DROP DATABASE dbx4 ;
DROP DATABASE dbx5 ;
DROP DATABASE dbx6 ;
DROP DATABASE dbx7 ;

