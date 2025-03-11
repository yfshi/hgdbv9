/***************************************************************
 *
 * ivymy.year(4) type support
 *
 ***************************************************************/

-- CREATE YEAR'S BASIC FUNCTION
CREATE OR REPLACE FUNCTION ivymy.year_in(cstring)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'year_in'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.year_out(ivymy.year)
RETURNS cstring
AS 'MODULE_PATHNAME', 'year_out'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.year_send(ivymy.year)
RETURNS bytea
AS 'MODULE_PATHNAME', 'year_send'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.year_recv(internal)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'year_recv'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.yeartypmodin(cstring[])
RETURNS integer
AS 'MODULE_PATHNAME', 'yeartypmodin'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.yeartypmodout(integer)
RETURNS cstring
AS 'MODULE_PATHNAME', 'yeartypmodout'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- Register the basic I/O for 'year' in pg_type.
UPDATE pg_type
SET typinput=(SELECT oid FROM pg_proc WHERE proname='year_in')
WHERE oid = 9295;

UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='year_out')
WHERE oid = 9295;

UPDATE pg_type
SET typreceive=(SELECT oid FROM pg_proc WHERE proname='year_recv')
WHERE oid = 9295;

UPDATE pg_type
SET typsend=(SELECT oid FROM pg_proc WHERE proname='year_send')
WHERE oid = 9295;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='yeartypmodin')
WHERE oid = 9295;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='yeartypmodout')
WHERE oid = 9295;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='yeartypmodin')
WHERE oid = 9296;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='yeartypmodout')
WHERE oid = 9296;

-- CREATE YEAR'S COMPARATION FUNCTION
CREATE OR REPLACE FUNCTION ivymy.year_eq(ivymy.year, ivymy.year)
RETURNS boolean
AS 'MODULE_PATHNAME', 'year_eq'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.year_ne(ivymy.year, ivymy.year)
RETURNS boolean
AS 'MODULE_PATHNAME', 'year_ne'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.year_le(ivymy.year, ivymy.year)
RETURNS boolean
AS 'MODULE_PATHNAME', 'year_le'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.year_lt(ivymy.year, ivymy.year)
RETURNS boolean
AS 'MODULE_PATHNAME', 'year_lt'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.year_ge(ivymy.year, ivymy.year)
RETURNS boolean
AS 'MODULE_PATHNAME', 'year_ge'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.year_gt(ivymy.year, ivymy.year)
RETURNS boolean
AS 'MODULE_PATHNAME', 'year_gt'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.year_seq(ivymy.year, ivymy.year)
RETURNS boolean
AS 'MODULE_PATHNAME', 'year_seq'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- CREATE OPERATOR
CREATE OPERATOR ivymy.=(
	LEFTARG		= ivymy.year,
	RIGHTARG	= ivymy.year,
	COMMUTATOR  = =,
	NEGATOR     = <>,
	PROCEDURE	= ivymy.year_eq,
	RESTRICT	= eqsel,
	JOIN		= eqjoinsel,
	MERGES,HASHES
);

CREATE OPERATOR ivymy.<>(
	LEFTARG		= ivymy.year,
	RIGHTARG	= ivymy.year,
	COMMUTATOR  = <>,
	NEGATOR    	= =,
	PROCEDURE	= ivymy.year_ne,
	RESTRICT	= neqsel,
	JOIN		= neqjoinsel
);

CREATE OPERATOR ivymy.<=(
	LEFTARG		= ivymy.year,
	RIGHTARG	= ivymy.year,
	COMMUTATOR	= >=,
	NEGATOR		= >,
	PROCEDURE	= ivymy.year_le,
	RESTRICT	= scalarlesel,
	JOIN		= scalarlejoinsel
);

CREATE OPERATOR ivymy.<(
	LEFTARG		= ivymy.year,
	RIGHTARG	= ivymy.year,
	COMMUTATOR	= >,
	NEGATOR		= >=,
	PROCEDURE	= ivymy.year_lt,
	RESTRICT	= scalarltsel,
	JOIN		= scalarltjoinsel
);

CREATE OPERATOR ivymy.>=(
	LEFTARG		= ivymy.year,
	RIGHTARG	= ivymy.year,
	COMMUTATOR	= <=,
	NEGATOR		= <,
	PROCEDURE	= ivymy.year_ge,
	RESTRICT	= scalargesel,
	JOIN		= scalargejoinsel
);

CREATE OPERATOR ivymy.>(
	LEFTARG 	= ivymy.year,
	RIGHTARG	= ivymy.year,
	COMMUTATOR	= <,
	NEGATOR		= <=,
	PROCEDURE	= ivymy.year_gt,
	RESTRICT	= scalargtsel,
	JOIN		= scalargtjoinsel
);

CREATE OPERATOR ivymy.<=>(
	LEFTARG 	= ivymy.year,
	RIGHTARG	= ivymy.year,
	PROCEDURE	= ivymy.year_seq
);

-- CREATE YEAR'S B-TREE SUPPORT FUNCTION
CREATE OR REPLACE FUNCTION ivymy.year_cmp(ivymy.year, ivymy.year)
RETURNS integer
AS 'MODULE_PATHNAME', 'year_cmp'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.year_sortsupport(internal)
RETURNS void
AS 'MODULE_PATHNAME', 'year_sortsupport'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR CLASS year_ops
	DEFAULT FOR TYPE year USING btree AS
		OPERATOR	1	ivymy.< (year, year),
		OPERATOR	2	ivymy.<=(year, year),
		OPERATOR	3	ivymy.= (year, year),
		OPERATOR	4	ivymy.>=(year, year),
		OPERATOR	5	ivymy.> (year, year),
		FUNCTION	1	ivymy.year_cmp(year, year),
		FUNCTION	2	ivymy.year_sortsupport(internal);

-- CREATE YEAR'S HASH SUPPORT FUNCTION
CREATE FUNCTION ivymy.year_hash(ivymy.year)
RETURNS INT4
AS 'MODULE_PATHNAME', 'year_hash'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR CLASS year_ops
	DEFAULT FOR TYPE year USING hash AS
		OPERATOR	1	ivymy.= (ivymy.year, ivymy.year),
		FUNCTION	1	ivymy.year_hash(ivymy.year);

-- CREATE YEAR'S CAST FUNCTION
-- To year
CREATE OR REPLACE FUNCTION ivymy.year(ivymy.year, integer)
RETURNS ivymy.YEAR
AS 'MODULE_PATHNAME', 'year_scale'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE CAST(year as year)
WITH FUNCTION ivymy.year(ivymy.year, integer) AS IMPLICIT;

CREATE OR REPLACE FUNCTION ivymy.int4_year(INT4)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'int4_year'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE CAST(INT4 AS year)
WITH FUNCTION ivymy.int4_year(INT4) AS IMPLICIT;

CREATE OR REPLACE FUNCTION ivymy.timestamptz2year(timestamptz)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'timestamptz_year'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE CAST(timestamptz AS year)
WITH FUNCTION ivymy.timestamptz2year(timestamptz) AS IMPLICIT;

CREATE OR REPLACE FUNCTION ivymy.numeric2year(numeric)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'numeric_year'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE CAST(numeric AS year)
WITH FUNCTION ivymy.numeric2year(numeric) AS IMPLICIT;

CREATE OR REPLACE FUNCTION ivymy.date2year(DATE)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'date_year'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE CAST (DATE AS year)
WITH FUNCTION ivymy.date2year(DATE) AS IMPLICIT;

CREATE OR REPLACE FUNCTION ivymy.time2year(TIME)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'time_year'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE CAST (TIME as year)
WITH FUNCTION ivymy.time2year(TIME) AS IMPLICIT;

CREATE OR REPLACE FUNCTION ivymy.timetz2year(timetz)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'timetz_year'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE CAST (timetz as year)
WITH FUNCTION ivymy.timetz2year(timetz) AS IMPLICIT;

CREATE OR REPLACE FUNCTION ivymy.char2year(CHAR)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'char_year'
LANGUAGE C VOLATILE STRICT PARALLEL SAFE;

CREATE CAST (CHAR AS YEAR)
WITH FUNCTION ivymy.char2year (CHAR) AS IMPLICIT;

CREATE OR REPLACE FUNCTION ivymy.bigint2year(IN num BIGINT)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'bigint_year'
LANGUAGE C VOLATILE STRICT PARALLEL SAFE;

CREATE CAST (BIGINT AS year)
WITH FUNCTION ivymy.bigint2year (BIGINT) AS IMPLICIT;

-- CREATE YEAR'S MAX, MIN FUNCTION
CREATE OR REPLACE FUNCTION ivymy.year_larger(ivymy.year, ivymy.year)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'year_larger'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.year_smaller(ivymy.year, ivymy.year)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'year_smaller'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

-- CREATE AGGREGATE pg_catalog.max(year)
CREATE AGGREGATE ivymy.max(year)
(
	sfunc		= ivymy.year_larger,
	stype		= ivymy.year,
	combinefunc	= ivymy.year_larger,
	sortop 		= >,
	parallel	= safe
);

-- CREATE AGGREGATE pg_catalog.min(year)
CREATE AGGREGATE ivymy.min(year)
(
	sfunc		= ivymy.year_smaller,
	stype		= ivymy.year,
	combinefunc	= ivymy.year_smaller,
	sortop		= <,
	parallel	= safe
);

-- year vs pg_catalog.int4
CREATE FUNCTION ivymy.year_eq_int(ivymy.year, pg_catalog.int4)
RETURNS bool
AS 'MODULE_PATHNAME', 'year_eq_int'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION ivymy.year_ne_int(ivymy.year, pg_catalog.int4)
RETURNS bool
AS 'MODULE_PATHNAME', 'year_ne_int'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION ivymy.year_lt_int(ivymy.year, pg_catalog.int4)
RETURNS bool
AS 'MODULE_PATHNAME', 'year_lt_int'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION ivymy.year_le_int(ivymy.year, pg_catalog.int4)
RETURNS bool
AS 'MODULE_PATHNAME', 'year_le_int'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION ivymy.year_gt_int(ivymy.year, pg_catalog.int4)
RETURNS bool
AS 'MODULE_PATHNAME', 'year_gt_int'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION ivymy.year_ge_int(ivymy.year, pg_catalog.int4)
RETURNS bool
AS 'MODULE_PATHNAME', 'year_ge_int'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ivymy.= (
    LEFTARG    = ivymy.year,
    RIGHTARG   = pg_catalog.int4,
    COMMUTATOR = =,
    NEGATOR    = <>,
    PROCEDURE  = ivymy.year_eq_int,
    RESTRICT   = eqsel,
    JOIN       = eqjoinsel
);

CREATE OPERATOR ivymy.<> (
    LEFTARG    = ivymy.year,
    RIGHTARG   = pg_catalog.int4,
    NEGATOR    = =,
    COMMUTATOR = <>,
    PROCEDURE  = ivymy.year_ne_int,
    RESTRICT   = neqsel,
    JOIN       = neqjoinsel
);

CREATE OPERATOR ivymy.< (
    LEFTARG    = ivymy.year,
    RIGHTARG   = pg_catalog.int4,
    NEGATOR    = >=,
    COMMUTATOR = >,
    PROCEDURE  = ivymy.year_lt_int,
    RESTRICT   = scalarltsel,
    JOIN       = scalarltjoinsel
);

CREATE OPERATOR ivymy.<= (
    LEFTARG    = ivymy.year,
    RIGHTARG   = pg_catalog.int4,
    NEGATOR    = >,
    COMMUTATOR = >=,
    PROCEDURE  = ivymy.year_le_int,
    RESTRICT   = scalarlesel,
    JOIN       = scalarlejoinsel
);

CREATE OPERATOR ivymy.> (
    LEFTARG    = ivymy.year,
    RIGHTARG   = pg_catalog.int4,
    NEGATOR    = <=,
    COMMUTATOR = <,
    PROCEDURE  = ivymy.year_gt_int,
    RESTRICT   = scalargtsel,
    JOIN       = scalargtjoinsel
);

CREATE OPERATOR ivymy.>= (
    LEFTARG    = ivymy.year,
    RIGHTARG   = pg_catalog.int4,
    NEGATOR    = <,
    COMMUTATOR = <=,
    PROCEDURE  = ivymy.year_ge_int,
    RESTRICT   = scalargesel,
    JOIN       = scalargejoinsel
);

CREATE OPERATOR ivymy.+ (
    LEFTARG     = ivymy.year,
    RIGHTARG    = pg_catalog.int4,
    PROCEDURE   = ivymy.year_add_int
);

CREATE OPERATOR ivymy.- (
    LEFTARG     = ivymy.year,
    RIGHTARG    = pg_catalog.int4,
    PROCEDURE   = ivymy.year_sub_int
);

CREATE OPERATOR ivymy.* (
    LEFTARG     = ivymy.year,
    RIGHTARG    = pg_catalog.int4,
    PROCEDURE   = ivymy.year_multi_int
);

CREATE OPERATOR ivymy./ (
    LEFTARG     = ivymy.year,
    RIGHTARG    = pg_catalog.int4,
    PROCEDURE   = ivymy.year_div_int
);

-- pg_catalog.int4 vs year
CREATE FUNCTION ivymy.int_eq_year(pg_catalog.int4, ivymy.year)
RETURNS bool
AS 'MODULE_PATHNAME', 'int_eq_year'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION ivymy.int_ne_year(pg_catalog.int4, ivymy.year)
RETURNS bool
AS 'MODULE_PATHNAME', 'int_ne_year'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION ivymy.int_lt_year(pg_catalog.int4, ivymy.year)
RETURNS bool
AS 'MODULE_PATHNAME', 'int_lt_year'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION ivymy.int_le_year(pg_catalog.int4, ivymy.year)
RETURNS bool
AS 'MODULE_PATHNAME', 'int_le_year'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION ivymy.int_gt_year(pg_catalog.int4, ivymy.year)
RETURNS bool
AS 'MODULE_PATHNAME', 'int_gt_year'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION ivymy.int_ge_year(pg_catalog.int4, ivymy.year)
RETURNS bool
AS 'MODULE_PATHNAME', 'int_ge_year'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ivymy.= (
    LEFTARG    = pg_catalog.int4,
    RIGHTARG   = ivymy.year,
    COMMUTATOR = =,
    NEGATOR    = <>,
    PROCEDURE  = ivymy.int_eq_year,
    RESTRICT   = eqsel,
    JOIN       = eqjoinsel
);

CREATE OPERATOR ivymy.<> (
    LEFTARG    = pg_catalog.int4,
    RIGHTARG   = ivymy.year,
    NEGATOR    = =,
    COMMUTATOR = <>,
    PROCEDURE  = ivymy.int_ne_year,
    RESTRICT   = neqsel,
    JOIN       = neqjoinsel
);

CREATE OPERATOR ivymy.< (
    LEFTARG    = pg_catalog.int4,
    RIGHTARG   = ivymy.year,
    NEGATOR    = >=,
    COMMUTATOR = >,
    PROCEDURE  = ivymy.int_lt_year,
    RESTRICT   = scalarltsel,
    JOIN       = scalarltjoinsel
);

CREATE OPERATOR ivymy.<= (
    LEFTARG    = pg_catalog.int4,
    RIGHTARG   = ivymy.year,
    NEGATOR    = >,
    COMMUTATOR = >=,
    PROCEDURE  = ivymy.int_le_year,
    RESTRICT   = scalarlesel,
    JOIN       = scalarlejoinsel
);

CREATE OPERATOR ivymy.> (
    LEFTARG    = pg_catalog.int4,
    RIGHTARG   = ivymy.year,
    NEGATOR    = <=,
    COMMUTATOR = <,
    PROCEDURE  = ivymy.int_gt_year,
    RESTRICT   = scalargtsel,
    JOIN       = scalargtjoinsel
);

CREATE OPERATOR ivymy.>= (
    LEFTARG    = pg_catalog.int4,
    RIGHTARG   = ivymy.year,
    NEGATOR    = <,
    COMMUTATOR = <=,
    PROCEDURE  = ivymy.int_ge_year,
    RESTRICT   = scalargesel,
    JOIN       = scalargejoinsel
);

CREATE OPERATOR ivymy.+ (
    LEFTARG     = pg_catalog.int4,
    RIGHTARG    = ivymy.year,
    PROCEDURE   = ivymy.int_add_year
);

CREATE OPERATOR ivymy.- (
    LEFTARG     = pg_catalog.int4,
    RIGHTARG    = ivymy.year,
    PROCEDURE   = ivymy.int_sub_year
);

CREATE OPERATOR ivymy.* (
    LEFTARG     = pg_catalog.int4,
    RIGHTARG    = ivymy.year,
    PROCEDURE   = ivymy.int_multi_year
);

CREATE OPERATOR ivymy./ (
    LEFTARG     = pg_catalog.int4,
    RIGHTARG    = ivymy.year,
    PROCEDURE   = ivymy.int_div_year
);
