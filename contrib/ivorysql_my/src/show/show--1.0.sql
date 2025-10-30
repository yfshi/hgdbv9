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
