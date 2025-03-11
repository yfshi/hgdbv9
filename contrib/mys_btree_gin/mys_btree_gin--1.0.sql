/*
 * contrib/mys_btree_gin/mys_btree_gin--1.0.sql
 * GIN index ops
 */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION mys_btree_gin" to load this file. \quit

CREATE FUNCTION ivymy.gin_btree_consistent(internal, int2, anyelement, int4, internal, internal)
RETURNS bool
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

--
--
--
-- mysyear ops
--
--
--
-- define the GIN support methods

CREATE FUNCTION ivymy.gin_extract_value_mysyear(ivymy.year, internal)
RETURNS internal
AS 'MODULE_PATHNAME', 'gin_extract_value_mysyear'
LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION ivymy.gin_extract_query_mysyear(ivymy.year, internal, int2, internal, internal)
RETURNS internal
AS 'MODULE_PATHNAME', 'gin_extract_query_mysyear'
LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION ivymy.gin_compare_prefix_mysyear(ivymy.year, ivymy.year, int2, internal)
RETURNS int4
AS 'MODULE_PATHNAME', 'gin_compare_prefix_mysyear'
LANGUAGE C STRICT IMMUTABLE;

CREATE OPERATOR CLASS gin_mysyear_ops
DEFAULT FOR TYPE ivymy.year USING gin
AS
	OPERATOR		1		ivymy.< (ivymy.year, ivymy.year),
	OPERATOR		2		ivymy.<=(ivymy.year, ivymy.year),
	OPERATOR		3		ivymy.= (ivymy.year, ivymy.year),
	OPERATOR		4		ivymy.>=(ivymy.year, ivymy.year),
	OPERATOR		5		ivymy.> (ivymy.year, ivymy.year),
	FUNCTION		1		ivymy.year_cmp(ivymy.year, ivymy.year),
	FUNCTION		2		ivymy.gin_extract_value_mysyear(ivymy.year, internal),
	FUNCTION		3		ivymy.gin_extract_query_mysyear(ivymy.year, internal, int2, internal, internal),
	FUNCTION		4		ivymy.gin_btree_consistent(internal, int2, anyelement, int4, internal, internal),
	FUNCTION		5		ivymy.gin_compare_prefix_mysyear(ivymy.year, ivymy.year, int2, internal),
STORAGE		ivymy.year;