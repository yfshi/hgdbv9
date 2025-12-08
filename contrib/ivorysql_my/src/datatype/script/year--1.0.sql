/***************************************************************
 *
 * MySQL year type support
 *
***************************************************************/
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
	DEFAULT FOR TYPE ivymy.year USING btree AS
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
	DEFAULT FOR TYPE ivymy.year USING hash AS
		OPERATOR	1	ivymy.= (ivymy.year, ivymy.year),
		FUNCTION	1	ivymy.year_hash(ivymy.year);

---------- Compatible MySQL Cast ----------
CREATE OR REPLACE FUNCTION ivymy.year(ivymy.year, integer)
RETURNS ivymy.YEAR
AS 'MODULE_PATHNAME', 'year_scale'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE CAST(year as year)
WITH FUNCTION ivymy.year(ivymy.year, integer) AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.int4_year(pg_catalog.int4)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'int4_year'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE CAST(pg_catalog.int4 AS year)
WITH FUNCTION ivymy.int4_year(pg_catalog.int4) AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.timestamptz2year(pg_catalog.timestamptz)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'timestamptz_year'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE CAST(pg_catalog.timestamptz AS year)
WITH FUNCTION ivymy.timestamptz2year(pg_catalog.timestamptz) AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.numeric2year(pg_catalog.numeric)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'numeric_year'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE CAST(pg_catalog.numeric AS year)
WITH FUNCTION ivymy.numeric2year(pg_catalog.numeric) AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.timetz2year(pg_catalog.timetz)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'timetz_year'
LANGUAGE C STABLE STRICT PARALLEL SAFE;

CREATE CAST (pg_catalog.timetz as year)
WITH FUNCTION ivymy.timetz2year(pg_catalog.timetz) AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.char2year(char)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'char_year'
LANGUAGE C VOLATILE STRICT PARALLEL SAFE;

CREATE CAST (char AS YEAR)
WITH FUNCTION ivymy.char2year (char) AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.bigint2year(IN num BIGINT)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'bigint_year'
LANGUAGE C VOLATILE STRICT PARALLEL SAFE;

CREATE CAST (BIGINT AS year)
WITH FUNCTION ivymy.bigint2year (BIGINT) AS ASSIGNMENT;

CREATE CAST (year AS int)  WITH INOUT AS IMPLICIT;

-- CREATE YEAR'S MAX, MIN FUNCTION
CREATE OR REPLACE FUNCTION ivymy.year_larger(ivymy.year, ivymy.year)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'year_larger'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OR REPLACE FUNCTION ivymy.year_smaller(ivymy.year, ivymy.year)
RETURNS ivymy.year
AS 'MODULE_PATHNAME', 'year_smaller'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE AGGREGATE ivymy.max(year)
(
	sfunc		= ivymy.year_larger,
	stype		= ivymy.year,
	combinefunc	= ivymy.year_larger,
	sortop 		= >,
	parallel	= safe
);

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

CREATE FUNCTION ivymy.year_add_int(ivymy.year, pg_catalog.int4)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME', 'year_add_int'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION ivymy.year_sub_int(ivymy.year, pg_catalog.int4)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME', 'year_sub_int'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION ivymy.year_multi_int(ivymy.year, pg_catalog.int4)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME', 'year_multi_int'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION ivymy.year_div_int(ivymy.year, pg_catalog.int4)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME', 'year_div_int'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

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

CREATE FUNCTION ivymy.int_add_year(pg_catalog.int4, ivymy.year)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME', 'int_add_year'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION ivymy.int_sub_year(pg_catalog.int4, ivymy.year)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME', 'int_sub_year'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION ivymy.int_multi_year(pg_catalog.int4, ivymy.year)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME', 'int_multi_year'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE FUNCTION ivymy.int_div_year(pg_catalog.int4, ivymy.year)
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME', 'int_div_year'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

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

-- ivymy.year vs pg_catalog.numeric
CREATE FUNCTION ivymy.year_eq_num(ivymy.year, pg_catalog.numeric)
RETURNS bool
AS 'MODULE_PATHNAME', 'year_eq_num'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ivymy.= (
    LEFTARG    = ivymy.year,
    RIGHTARG   = pg_catalog.numeric,
    COMMUTATOR = =,
    PROCEDURE  = ivymy.year_eq_num,
    RESTRICT   = eqsel,
    JOIN       = eqjoinsel
);

-- pg_catalog.numeric vs ivymy.year
CREATE FUNCTION ivymy.num_eq_year(pg_catalog.numeric, ivymy.year)
RETURNS bool
AS 'MODULE_PATHNAME', 'num_eq_year'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ivymy.= (
    LEFTARG    = pg_catalog.numeric,
    RIGHTARG   = ivymy.year,
    COMMUTATOR = =,
    PROCEDURE  = ivymy.num_eq_year,
    RESTRICT   = eqsel,
    JOIN       = eqjoinsel
);

-- ivymy.year vs pg_catalog.timestamptz
CREATE OR REPLACE FUNCTION ivymy.year_eq_timestamptz(ivymy.year, pg_catalog.timestamptz)
RETURNS boolean
AS 'MODULE_PATHNAME', 'year_eq_timestamptz'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ivymy.=(
	LEFTARG		= ivymy.year,
	RIGHTARG	= pg_catalog.timestamptz,
	COMMUTATOR  = =,
	PROCEDURE	= ivymy.year_eq_timestamptz,
	RESTRICT	= eqsel,
	JOIN		= eqjoinsel
);

-- ivymy.timestamptz vs pg_catalog.year
CREATE OR REPLACE FUNCTION ivymy.timestamptz_eq_year(pg_catalog.timestamptz, ivymy.year)
RETURNS boolean
AS 'MODULE_PATHNAME', 'timestamptz_eq_year'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ivymy.=(
	LEFTARG		= pg_catalog.timestamptz,
	RIGHTARG	= ivymy.year,
	COMMUTATOR  = =,
	PROCEDURE	= ivymy.timestamptz_eq_year,
	RESTRICT	= eqsel,
	JOIN		= eqjoinsel
);

-- ivymy.year vs pg_catalog.bigint
CREATE OR REPLACE FUNCTION ivymy.year_lt_bigint(ivymy.year, bigint)
RETURNS boolean
AS 'MODULE_PATHNAME', 'year_lt_bigint'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ivymy.<(
    LEFTARG    = ivymy.year,
    RIGHTARG   = bigint,
    COMMUTATOR = >,
    PROCEDURE  = ivymy.year_lt_bigint,
    RESTRICT   = scalarltsel,
    JOIN       = scalarltjoinsel
);

CREATE OR REPLACE FUNCTION ivymy.year_gt_bigint(ivymy.year, bigint)
RETURNS boolean
AS 'MODULE_PATHNAME', 'year_gt_bigint'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ivymy.>(
    LEFTARG    = ivymy.year,
    RIGHTARG   = bigint,
    COMMUTATOR = <,
    PROCEDURE  = ivymy.year_gt_bigint,
    RESTRICT   = scalarltsel,
    JOIN       = scalarltjoinsel
);

-- pg_catalog.bigint vs ivymy.year
CREATE OR REPLACE FUNCTION ivymy.bigint_lt_year(bigint, ivymy.year)
RETURNS boolean
AS 'MODULE_PATHNAME', 'bigint_lt_year'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ivymy.<(
    LEFTARG    = bigint,
    RIGHTARG   = ivymy.year,
    COMMUTATOR = >,
    PROCEDURE  = ivymy.bigint_lt_year,
    RESTRICT   = scalarltsel,
    JOIN       = scalarltjoinsel
);

CREATE OR REPLACE FUNCTION ivymy.bigint_gt_year(bigint, ivymy.year)
RETURNS boolean
AS 'MODULE_PATHNAME', 'bigint_gt_year'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ivymy.>(
    LEFTARG    = bigint,
    RIGHTARG   = ivymy.year,
    COMMUTATOR = <,
    PROCEDURE  = ivymy.bigint_gt_year,
    RESTRICT   = scalarltsel,
    JOIN       = scalarltjoinsel
);

-- Fix Bug#Z5263 begin
CREATE  OR REPLACE FUNCTION ivymy.year_like(ivymy.year, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','year_text_like'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ~~ (
	procedure = ivymy.year_like,
	leftarg   = ivymy.year,
	rightarg  = pg_catalog.text
);

CREATE  OR REPLACE FUNCTION ivymy.year_notlike(ivymy.year, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','year_text_notlike'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR !~~ (
	procedure = ivymy.year_notlike,
	leftarg   = ivymy.year,
	rightarg  = pg_catalog.text
);

CREATE  OR REPLACE FUNCTION ivymy.year_like(ivymy.year, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','year_int_like'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR ~~ (
	procedure = ivymy.year_like,
	leftarg   = ivymy.year,
	rightarg  = pg_catalog.int4
);

CREATE  OR REPLACE FUNCTION ivymy.year_notlike(ivymy.year, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','year_int_notlike'
LANGUAGE C IMMUTABLE STRICT PARALLEL SAFE;

CREATE OPERATOR !~~ (
	procedure = ivymy.year_notlike,
	leftarg   = ivymy.year,
	rightarg  = pg_catalog.int4
);
-- Fix Bug#Z5263 end

CREATE FUNCTION ivymy.year_mod(int4, year)
RETURNS uint8
AS 'select $1 mod $2::int4'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
CREATE FUNCTION ivymy.year_mod(year, int4)
RETURNS uint8
AS 'select $1::int4 mod $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.year_mod(int8, year)
RETURNS uint8
AS 'select $1 mod $2::int8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
CREATE FUNCTION ivymy.year_mod(year, int8)
RETURNS uint8
AS 'select $1::int8 mod $2'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
