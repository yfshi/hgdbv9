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
--
--
--
-- mysdate ops
--
--
--
-- define the GIN support methods
CREATE FUNCTION ivymy.gin_extract_value_mysdate(ivymy.mysdate, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION ivymy.gin_compare_prefix_mysdate(ivymy.mysdate, ivymy.mysdate, int2, internal)
RETURNS int4
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION ivymy.gin_extract_query_mysdate(ivymy.mysdate, internal, int2, internal, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE OPERATOR CLASS gin_mysdate_ops
DEFAULT FOR TYPE ivymy.mysdate USING gin
AS
	OPERATOR		1		<,
	OPERATOR		2		<=,
	OPERATOR		3		=,
	OPERATOR		4		>=,
	OPERATOR		5		>,
	FUNCTION		1		ivymy.mysdate_cmp(ivymy.mysdate, ivymy.mysdate),
	FUNCTION		2		ivymy.gin_extract_value_mysdate(ivymy.mysdate, internal),
	FUNCTION		3		ivymy.gin_extract_query_mysdate(ivymy.mysdate, internal, int2, internal, internal),
	FUNCTION		4		ivymy.gin_btree_consistent(internal, int2, anyelement, int4, internal, internal),
	FUNCTION		5		ivymy.gin_compare_prefix_mysdate(ivymy.mysdate, ivymy.mysdate, int2, internal),
STORAGE		ivymy.mysdate;

--
--
--
-- mysdatetime ops
--
--
--
-- define the GIN support methods

CREATE FUNCTION ivymy.gin_extract_value_mysdatetime(ivymy.mysdatetime, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION ivymy.gin_compare_prefix_mysdatetime(ivymy.mysdatetime, ivymy.mysdatetime, int2, internal)
RETURNS int4
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION ivymy.gin_extract_query_mysdatetime(ivymy.mysdatetime, internal, int2, internal, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE OPERATOR CLASS gin_mysdatetime_ops
DEFAULT FOR TYPE ivymy.mysdatetime USING gin
AS
	OPERATOR		1		<,
	OPERATOR		2		<=,
	OPERATOR		3		=,
	OPERATOR		4		>=,
	OPERATOR		5		>,
	FUNCTION		1		ivymy.mysdatetime_cmp(ivymy.mysdatetime, ivymy.mysdatetime),
	FUNCTION		2		ivymy.gin_extract_value_mysdatetime(ivymy.mysdatetime, internal),
	FUNCTION		3		ivymy.gin_extract_query_mysdatetime(ivymy.mysdatetime, internal, int2, internal, internal),
	FUNCTION		4		ivymy.gin_btree_consistent(internal, int2, anyelement, int4, internal, internal),
	FUNCTION		5		ivymy.gin_compare_prefix_mysdatetime(ivymy.mysdatetime, ivymy.mysdatetime, int2, internal),
STORAGE		ivymy.mysdatetime;


/* timestamp */
--
--
--
-- mystimestamp ops
--
--
--
-- define the GIN support methods
CREATE FUNCTION ivymy.gin_extract_value_mystimestamp(ivymy.mystimestamp, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION ivymy.gin_compare_prefix_mystimestamp(ivymy.mystimestamp, ivymy.mystimestamp, int2, internal)
RETURNS int4
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION ivymy.gin_extract_query_mystimestamp(ivymy.mystimestamp, internal, int2, internal, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE OPERATOR CLASS gin_mystimestamp_ops
DEFAULT FOR TYPE ivymy.mystimestamp USING gin
AS
	OPERATOR		1		<,
	OPERATOR		2		<=,
	OPERATOR		3		=,
	OPERATOR		4		>=,
	OPERATOR		5		>,
	FUNCTION		1		ivymy.mystimestamp_cmp(ivymy.mystimestamp, ivymy.mystimestamp),
	FUNCTION		2		ivymy.gin_extract_value_mystimestamp(ivymy.mystimestamp, internal),
	FUNCTION		3		ivymy.gin_extract_query_mystimestamp(ivymy.mystimestamp, internal, int2, internal, internal),
	FUNCTION		4		ivymy.gin_btree_consistent(internal, int2, anyelement, int4, internal, internal),
	FUNCTION		5		ivymy.gin_compare_prefix_mystimestamp(ivymy.mystimestamp, ivymy.mystimestamp, int2, internal),
STORAGE		ivymy.mystimestamp;

/* time */
--
--
--
-- mystime ops
--
--
--
-- define the GIN support methods
CREATE FUNCTION ivymy.gin_extract_value_mystime(ivymy.mystime, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION ivymy.gin_compare_prefix_mystime(ivymy.mystime, ivymy.mystime, int2, internal)
RETURNS int4
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION ivymy.gin_extract_query_mystime(ivymy.mystime, internal, int2, internal, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C STRICT IMMUTABLE;

CREATE OPERATOR CLASS gin_mystime_ops
DEFAULT FOR TYPE ivymy.mystime USING gin
AS
	OPERATOR		1		<,
	OPERATOR		2		<=,
	OPERATOR		3		=,
	OPERATOR		4		>=,
	OPERATOR		5		>,
	FUNCTION		1		ivymy.mystime_cmp(ivymy.mystime, ivymy.mystime),
	FUNCTION		2		ivymy.gin_extract_value_mystime(ivymy.mystime, internal),
	FUNCTION		3		ivymy.gin_extract_query_mystime(ivymy.mystime, internal, int2, internal, internal),
	FUNCTION		4		ivymy.gin_btree_consistent(internal, int2, anyelement, int4, internal, internal),
	FUNCTION		5		ivymy.gin_compare_prefix_mystime(ivymy.mystime, ivymy.mystime, int2, internal),
STORAGE		ivymy.mystime;