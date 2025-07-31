/* src/test/modules/worker_spi/worker_spi--1.0.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION worker_pg_pdr" to load this file. \quit

CREATE FUNCTION worker_spi_launch(pg_catalog.int4)
RETURNS pg_catalog.int4 STRICT
AS 'MODULE_PATHNAME'
LANGUAGE C;

CREATE OR REPLACE FUNCTION pg_catalog.pg_pdr_new_snap()
RETURNS boolean 
AS 'MODULE_PATHNAME', 'pg_pdr_new_snap'
LANGUAGE C STRICT;
