/*
 * contrib/mys_btree_gist/mys_btree_gist--1.0.sql
 * Gist/GIN index ops
 */

CREATE FUNCTION ivymy.gbtreekey8_in(cstring)
RETURNS ivymy.gbtreekey8
AS 'MODULE_PATHNAME', 'gbtreekey_in'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbtreekey8_out(ivymy.gbtreekey8)
RETURNS cstring
AS 'MODULE_PATHNAME', 'gbtreekey_out'
LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE ivymy.gbtreekey8 (
	INTERNALLENGTH = 8,
	INPUT  = ivymy.gbtreekey8_in,
	OUTPUT = ivymy.gbtreekey8_out
);

CREATE FUNCTION ivymy.gbtreekey16_in(cstring)
RETURNS ivymy.gbtreekey16
AS 'MODULE_PATHNAME', 'gbtreekey_in'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbtreekey16_out(ivymy.gbtreekey16)
RETURNS cstring
AS 'MODULE_PATHNAME', 'gbtreekey_out'
LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE ivymy.gbtreekey16 (
	INTERNALLENGTH = 16,
	INPUT  = ivymy.gbtreekey16_in,
	OUTPUT = ivymy.gbtreekey16_out
);

CREATE FUNCTION ivymy.gbtreekey32_in(cstring)
RETURNS ivymy.gbtreekey32
AS 'MODULE_PATHNAME', 'gbtreekey_in'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION gbtreekey32_out(ivymy.gbtreekey32)
RETURNS cstring
AS 'MODULE_PATHNAME', 'gbtreekey_out'
LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE ivymy.gbtreekey32 (
	INTERNALLENGTH = 32,
	INPUT  = ivymy.gbtreekey32_in,
	OUTPUT = ivymy.gbtreekey32_out
);

CREATE FUNCTION ivymy.gbtreekey_var_in(cstring)
RETURNS ivymy.gbtreekey_var
AS 'MODULE_PATHNAME', 'gbtreekey_in'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbtreekey_var_out(gbtreekey_var)
RETURNS cstring
AS 'MODULE_PATHNAME', 'gbtreekey_out'
LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE ivymy.gbtreekey_var (
	INTERNALLENGTH = VARIABLE,
	INPUT  = ivymy.gbtreekey_var_in,
	OUTPUT = ivymy.gbtreekey_var_out,
	STORAGE = EXTENDED
);

CREATE FUNCTION ivymy.gbt_decompress(internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

--
--
--
-- ivymy.mysyear ops
--
--
--

CREATE FUNCTION ivymy.gbt_year_consistent(internal, ivymy.year, int2, oid, internal)
RETURNS bool
AS 'MODULE_PATHNAME', 'gbt_year_consistent'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_year_union(internal, internal)
RETURNS gbtreekey8
AS 'MODULE_PATHNAME', 'gbt_year_union'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_year_compress(internal)
RETURNS internal
AS 'MODULE_PATHNAME', 'gbt_year_compress'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_year_penalty(internal, internal, internal)
RETURNS internal
AS 'MODULE_PATHNAME', 'gbt_year_penalty'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_year_picksplit(internal, internal)
RETURNS internal
AS 'MODULE_PATHNAME', 'gbt_year_picksplit'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_year_same(ivymy.gbtreekey8, ivymy.gbtreekey8, internal)
RETURNS internal
AS 'MODULE_PATHNAME', 'gbt_year_same'
LANGUAGE C IMMUTABLE STRICT;

-- Create the year operator class
CREATE OPERATOR CLASS gist_mysyear_ops
DEFAULT FOR TYPE ivymy.year USING gist
AS
	OPERATOR	1	ivymy.< (ivymy.year, ivymy.year),
	OPERATOR	2	ivymy.<=(ivymy.year, ivymy.year),
	OPERATOR	3	ivymy.= (ivymy.year, ivymy.year),
	OPERATOR	4	ivymy.>=(ivymy.year, ivymy.year),
	OPERATOR	5	ivymy.> (ivymy.year, ivymy.year),
	FUNCTION	1	ivymy.gbt_year_consistent (internal, ivymy.year, int2, oid, internal),
	FUNCTION	2	ivymy.gbt_year_union (internal, internal),
	FUNCTION	3	ivymy.gbt_year_compress (internal),
	FUNCTION	4	ivymy.gbt_decompress (internal),
	FUNCTION	5	ivymy.gbt_year_penalty (internal, internal, internal),
	FUNCTION	6	ivymy.gbt_year_picksplit (internal, internal),
	FUNCTION	7	ivymy.gbt_year_same (ivymy.gbtreekey8, ivymy.gbtreekey8, internal),
	STORAGE		gbtreekey8;

ALTER OPERATOR FAMILY gist_mysyear_ops USING gist ADD
	OPERATOR	6	ivymy.<> (ivymy.year, ivymy.year);
	
CREATE FUNCTION ivymy.mysdatetime_dist(ivymy.mysdatetime, ivymy.mysdatetime)
RETURNS pg_catalog.interval
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR <-> (
	LEFTARG = ivymy.mysdatetime,
	RIGHTARG = ivymy.mysdatetime,
	PROCEDURE = ivymy.mysdatetime_dist,
	COMMUTATOR = '<->'
);

CREATE FUNCTION ivymy.mysdate_dist(ivymy.mysdate, ivymy.mysdate)
RETURNS int4
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR <-> (
	LEFTARG = ivymy.mysdate,
	RIGHTARG = ivymy.mysdate,
	PROCEDURE = ivymy.mysdate_dist,
	COMMUTATOR = '<->'
);

CREATE FUNCTION ivymy.mystimestamp_dist(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS pg_catalog.interval
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR <-> (
	LEFTARG = ivymy.mystimestamp,
	RIGHTARG = ivymy.mystimestamp,
	PROCEDURE = ivymy.mystimestamp_dist,
	COMMUTATOR = '<->'
);

CREATE FUNCTION ivymy.mystime_dist(ivymy.mystime, ivymy.mystime)
RETURNS pg_catalog.interval
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR <-> (
	LEFTARG = ivymy.mystime,
	RIGHTARG = ivymy.mystime,
	PROCEDURE = ivymy.mystime_dist,
	COMMUTATOR = '<->'
);
--
--
--
-- mysdatetime ops
--
--
--
CREATE FUNCTION ivymy.gbt_mysdatetime_consistent(internal,ivymy.mysdatetime,int2,oid,internal)
RETURNS bool
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_mysdatetime_distance(internal,ivymy.mysdatetime,int2,oid,internal)
RETURNS float8
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_mysdatetime_compress(internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_mysdatetime_fetch(internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_mysdatetime_penalty(internal,internal,internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_mysdatetime_picksplit(internal, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_mysdatetime_union(internal, internal)
RETURNS ivymy.gbtreekey16
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_mysdatetime_same(ivymy.gbtreekey16, ivymy.gbtreekey16, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

-- Create the operator class
CREATE OPERATOR CLASS gist_mysdatetime_ops
DEFAULT FOR TYPE ivymy.mysdatetime USING gist
AS
	OPERATOR	1	<  ,
	OPERATOR	2	<= ,
	OPERATOR	3	=  ,
	OPERATOR	4	>= ,
	OPERATOR	5	>  ,
	FUNCTION	1	ivymy.gbt_mysdatetime_consistent (internal, ivymy.mysdatetime, int2, oid, internal),
	FUNCTION	2	ivymy.gbt_mysdatetime_union (internal, internal),
	FUNCTION	3	ivymy.gbt_mysdatetime_compress (internal),
	FUNCTION	4	ivymy.gbt_decompress (internal),
	FUNCTION	5	ivymy.gbt_mysdatetime_penalty (internal, internal, internal),
	FUNCTION	6	ivymy.gbt_mysdatetime_picksplit (internal, internal),
	FUNCTION	7	ivymy.gbt_mysdatetime_same (ivymy.gbtreekey16, ivymy.gbtreekey16, internal),
	STORAGE 	ivymy.gbtreekey16;

ALTER OPERATOR FAMILY gist_mysdatetime_ops USING gist ADD
	OPERATOR	6	<> (ivymy.mysdatetime, ivymy.mysdatetime) ,
	OPERATOR	15	<-> (ivymy.mysdatetime, ivymy.mysdatetime) FOR ORDER BY pg_catalog.interval_ops ,
	FUNCTION	8 (ivymy.mysdatetime, ivymy.mysdatetime) ivymy.gbt_mysdatetime_distance (internal, ivymy.mysdatetime, int2, oid, internal) ,
	FUNCTION	9 (ivymy.mysdatetime, ivymy.mysdatetime) ivymy.gbt_mysdatetime_fetch (internal) ;


--
--
--
-- ivymy.mysdate ops
--
--
--

CREATE FUNCTION ivymy.gbt_date_consistent(internal,ivymy.mysdate,int2,oid,internal)
RETURNS bool
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_date_distance(internal,ivymy.mysdate,int2,oid,internal)
RETURNS float8
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_date_compress(internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_date_fetch(internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_date_penalty(internal,internal,internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_date_picksplit(internal, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_date_union(internal, internal)
RETURNS gbtreekey8
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_date_same(ivymy.gbtreekey8, ivymy.gbtreekey8, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

-- Create the operator class
CREATE OPERATOR CLASS gist_mysdate_ops
DEFAULT FOR TYPE ivymy.mysdate USING gist
AS
	OPERATOR	1	<  ,
	OPERATOR	2	<= ,
	OPERATOR	3	=  ,
	OPERATOR	4	>= ,
	OPERATOR	5	>  ,
	FUNCTION	1	ivymy.gbt_date_consistent (internal, ivymy.mysdate, int2, oid, internal),
	FUNCTION	2	ivymy.gbt_date_union (internal, internal),
	FUNCTION	3	ivymy.gbt_date_compress (internal),
	FUNCTION	4	ivymy.gbt_decompress (internal),
	FUNCTION	5	ivymy.gbt_date_penalty (internal, internal, internal),
	FUNCTION	6	ivymy.gbt_date_picksplit (internal, internal),
	FUNCTION	7	ivymy.gbt_date_same (ivymy.gbtreekey8, ivymy.gbtreekey8, internal),
	STORAGE		gbtreekey8;

ALTER OPERATOR FAMILY gist_mysdate_ops USING gist ADD
	OPERATOR	6	<> (ivymy.mysdate, ivymy.mysdate) ,
	OPERATOR	15	<-> (ivymy.mysdate, ivymy.mysdate) FOR ORDER BY pg_catalog.integer_ops ,
	FUNCTION	8 (ivymy.mysdate, ivymy.mysdate) ivymy.gbt_date_distance (internal, ivymy.mysdate, int2, oid, internal) ,
	FUNCTION	9 (ivymy.mysdate, ivymy.mysdate) ivymy.gbt_date_fetch (internal) ;


/* timestamp */
--
--
--
-- mystimestamp ops
--
--
--
CREATE FUNCTION ivymy.gbt_mystimestamp_consistent(internal,ivymy.mystimestamp,int2,oid,internal)
RETURNS bool
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_mystimestamp_distance(internal,ivymy.mystimestamp,int2,oid,internal)
RETURNS float8
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_mystimestamp_compress(internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_mystimestamp_fetch(internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_mystimestamp_penalty(internal,internal,internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_mystimestamp_picksplit(internal, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_mystimestamp_union(internal, internal)
RETURNS ivymy.gbtreekey16
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_mystimestamp_same(ivymy.gbtreekey16, ivymy.gbtreekey16, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR CLASS gist_mystimestamp_ops
DEFAULT FOR TYPE ivymy.mystimestamp USING gist
AS
	OPERATOR	1	<  ,
	OPERATOR	2	<= ,
	OPERATOR	3	=  ,
	OPERATOR	4	>= ,
	OPERATOR	5	>  ,
	FUNCTION	1	ivymy.gbt_mystimestamp_consistent (internal, ivymy.mystimestamp, int2, oid, internal),
	FUNCTION	2	ivymy.gbt_mystimestamp_union (internal, internal),
	FUNCTION	3	ivymy.gbt_mystimestamp_compress (internal),
	FUNCTION	4	ivymy.gbt_decompress (internal),
	FUNCTION	5	ivymy.gbt_mystimestamp_penalty (internal, internal, internal),
	FUNCTION	6	ivymy.gbt_mystimestamp_picksplit (internal, internal),
	FUNCTION	7	ivymy.gbt_mystimestamp_same (ivymy.gbtreekey16, ivymy.gbtreekey16, internal),
	STORAGE 	ivymy.gbtreekey16;

ALTER OPERATOR FAMILY gist_mystimestamp_ops USING gist ADD
	OPERATOR	6	<> (ivymy.mystimestamp, ivymy.mystimestamp) ,
	OPERATOR	15	<-> (ivymy.mystimestamp, ivymy.mystimestamp) FOR ORDER BY pg_catalog.interval_ops ,
	FUNCTION	8 (ivymy.mystimestamp, ivymy.mystimestamp) ivymy.gbt_mystimestamp_distance (internal, ivymy.mystimestamp, int2, oid, internal) ,
	FUNCTION	9 (ivymy.mystimestamp, ivymy.mystimestamp) ivymy.gbt_mystimestamp_fetch (internal) ;

/* time */
--
--
-- mystime ops
--
--
--
CREATE FUNCTION ivymy.gbt_mystime_consistent(internal,ivymy.mystime,int2,oid,internal)
RETURNS bool
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_mystime_distance(internal,ivymy.mystime,int2,oid,internal)
RETURNS float8
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_mystime_compress(internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_mystime_fetch(internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_mystime_penalty(internal,internal,internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_mystime_picksplit(internal, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_mystime_union(internal, internal)
RETURNS ivymy.gbtreekey16
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION ivymy.gbt_mystime_same(ivymy.gbtreekey16, ivymy.gbtreekey16, internal)
RETURNS internal
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR CLASS gist_mystime_ops
DEFAULT FOR TYPE ivymy.mystime USING gist
AS
	OPERATOR	1	<  ,
	OPERATOR	2	<= ,
	OPERATOR	3	=  ,
	OPERATOR	4	>= ,
	OPERATOR	5	>  ,
	FUNCTION	1	ivymy.gbt_mystime_consistent (internal, ivymy.mystime, int2, oid, internal),
	FUNCTION	2	ivymy.gbt_mystime_union (internal, internal),
	FUNCTION	3	ivymy.gbt_mystime_compress (internal),
	FUNCTION	4	ivymy.gbt_decompress (internal),
	FUNCTION	5	ivymy.gbt_mystime_penalty (internal, internal, internal),
	FUNCTION	6	ivymy.gbt_mystime_picksplit (internal, internal),
	FUNCTION	7	ivymy.gbt_mystime_same (ivymy.gbtreekey16, ivymy.gbtreekey16, internal),
	STORAGE 	ivymy.gbtreekey16;

ALTER OPERATOR FAMILY gist_mystimestamp_ops USING gist ADD
	OPERATOR	6	<> (ivymy.mystime, ivymy.mystime) ,
	OPERATOR	15	<-> (ivymy.mystime, ivymy.mystime) FOR ORDER BY pg_catalog.interval_ops ,
	FUNCTION	8 (ivymy.mystime, ivymy.mystime) ivymy.gbt_mystime_distance (internal, ivymy.mystime, int2, oid, internal) ,
	FUNCTION	9 (ivymy.mystime, ivymy.mystime) ivymy.gbt_mystime_fetch (internal) ;