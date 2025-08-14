/* contrib/ora_synonym/ora_synonym--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION ora_synonym" to load this file. \quit

--CREATE FUNCTION get_synonymversion()
--RETURNS text
--AS 'MODULE_PATHNAME', 'get_synonymversion'
--LANGUAGE C STRICT PARALLEL RESTRICTED;

CREATE OR REPLACE VIEW SYS.ALL_SYNONYMS AS
    SELECT synowner::regrole AS OWNER,
            synname AS SYNONYM_NAME,
            table_namespace AS TABLE_OWNER,
            objname AS TABLE_NAME,
            db_link AS DB_LINK,
            '' AS ORIGIN_CON_ID
    FROM PG_SYNONYM;

GRANT SELECT ON SYS.ALL_SYNONYMS TO PUBLIC;

CREATE OR REPLACE VIEW SYS.USER_SYNONYMS AS
    SELECT synowner::regrole AS OWNER,
            synname AS SYNONYM_NAME,
            table_namespace AS TABLE_OWNER,
            objname AS TABLE_NAME,
            db_link AS DB_LINK,
            '' AS ORIGIN_CON_ID
    FROM PG_SYNONYM WHERE pg_get_userbyid(synowner) = CURRENT_USER;

GRANT SELECT ON SYS.USER_SYNONYMS TO PUBLIC;

CREATE OR REPLACE VIEW SYS.DBA_SYNONYMS AS
    SELECT synowner::regrole AS OWNER,
            synname AS SYNONYM_NAME,
            table_namespace AS TABLE_OWNER,
            objname AS TABLE_NAME,
            db_link AS DB_LINK,
            '' AS ORIGIN_CON_ID
    FROM PG_SYNONYM;

GRANT SELECT ON SYS.DBA_SYNONYMS TO PUBLIC;