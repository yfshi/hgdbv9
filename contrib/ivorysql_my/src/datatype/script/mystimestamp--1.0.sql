
-- Begin - ReqID:SRS-MYSQL-TIMESTAMPTYPE 
/***************************************************************
 *
 * MySQL timestamp type support
 *
***************************************************************/
CREATE FUNCTION ivymy.mystimestamp_in(pg_catalog.cstring)
RETURNS ivymy.mystimestamp
AS 'MODULE_PATHNAME','mystimestamp_in'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mystimestamp_out(ivymy.mystimestamp)
RETURNS CSTRING
AS 'MODULE_PATHNAME','mystimestamp_out'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mystimestamp_recv(internal)
RETURNS ivymy.mystimestamp
AS 'MODULE_PATHNAME','mystimestamp_recv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystimestamp_send(ivymy.mystimestamp)
RETURNS bytea
AS 'MODULE_PATHNAME','mystimestamp_send'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

-- Register the basic I/O for 'mystimestamp' in pg_type.
UPDATE pg_type
SET typinput=(SELECT oid FROM pg_proc WHERE proname='mystimestamp_in')
WHERE oid = 9287;

UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='mystimestamp_out')
WHERE oid = 9287;

UPDATE pg_type
SET typreceive=(SELECT oid FROM pg_proc WHERE proname='mystimestamp_recv')
WHERE oid = 9287;

UPDATE pg_type
SET typsend=(SELECT oid FROM pg_proc WHERE proname='mystimestamp_send')
WHERE oid = 9287;

/* CREATE OPERATOR */
----function eq
CREATE FUNCTION ivymy.mystimestamp_eq(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_eq'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ne
CREATE FUNCTION ivymy.mystimestamp_ne(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_ne'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function lt
CREATE FUNCTION ivymy.mystimestamp_lt(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_lt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.mystimestamp_gt(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_gt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--function le
CREATE FUNCTION ivymy.mystimestamp_le(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_le'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.mystimestamp_ge(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_ge'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.mystimestamp_seq(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function add
CREATE FUNCTION ivymy.mystimestamp_add(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_addtimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function sub
CREATE FUNCTION ivymy.mystimestamp_sub(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_subtimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function multiply
CREATE FUNCTION ivymy.mystimestamp_mul(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_multimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function divide
CREATE FUNCTION ivymy.mystimestamp_div(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS float8
AS 'MODULE_PATHNAME','mystimestamp_divtimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function exact division
CREATE FUNCTION ivymy.mys_div(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_exact_divtimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function mod
CREATE FUNCTION ivymy.mystimestamp_mod(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_modtimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function &
CREATE FUNCTION ivymy.mystimestamp_bitand(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_bitandtimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function |
CREATE FUNCTION ivymy.mystimestamp_bitor(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_bitortimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ~
CREATE FUNCTION ivymy.mystimestamp_nega(ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_negatimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ^
CREATE FUNCTION ivymy.mystimestamp_bitxor(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_bitxortimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function >>
CREATE FUNCTION ivymy.mystimestamp_rightmov(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_rightmovtimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function <<
CREATE FUNCTION ivymy.mystimestamp_leftmov(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_leftmovtimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--  "bit_count"
CREATE FUNCTION ivymy.bit_count(ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_bit_count'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

----function &&
CREATE FUNCTION ivymy.mystimestamp_and(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_andtimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

-- || ,string cat ,need "set sql_mode = 'pipes_as_concat';"
CREATE FUNCTION ivymy.mystimestamp_cat(mystimestamp, mystimestamp)
RETURNS varchar
AS 'select concat($1::varchar, $2::varchar);'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE;

----operator invert !
CREATE FUNCTION ivymy.mystimestamp_invert(ivymy.mystimestamp)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamp_invertmystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--operator xor or and

----operator =
CREATE OPERATOR = (
	procedure = ivymy.mystimestamp_eq,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

----operator <>
CREATE OPERATOR <> (
	procedure = ivymy.mystimestamp_ne,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

--operator >
CREATE OPERATOR > (
	procedure = ivymy.mystimestamp_gt,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR < (
	procedure = ivymy.mystimestamp_lt,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator <=
CREATE OPERATOR <= (
	procedure = ivymy.mystimestamp_le,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator >=
CREATE OPERATOR >= (
	procedure = ivymy.mystimestamp_ge,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

--operator <=>
CREATE OPERATOR <=> (
	procedure = ivymy.mystimestamp_seq,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp
);

--operator +
CREATE OPERATOR + (
	procedure = ivymy.mystimestamp_add,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp,
	commutator = +
);

--operator -
CREATE OPERATOR - (
	procedure = ivymy.mystimestamp_sub,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp
);

--operator *
CREATE OPERATOR * (
	procedure = ivymy.mystimestamp_mul,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp
);

--operator /
CREATE OPERATOR / (
	procedure = ivymy.mystimestamp_div,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp
);

--operator //
CREATE OPERATOR // (
	procedure = ivymy.mys_div,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp
);

--operator %
CREATE OPERATOR % (
	procedure = ivymy.mystimestamp_mod,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp
);

--operator &
CREATE OPERATOR & (
	procedure = ivymy.mystimestamp_bitand,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp
);

--operator |
CREATE OPERATOR | (
	procedure = ivymy.mystimestamp_bitor,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp
);

--operator ~
CREATE OPERATOR ~ (
	procedure = ivymy.mystimestamp_nega,
	rightarg = ivymy.mystimestamp
);


--operator ^
CREATE OPERATOR ^ (
	procedure = ivymy.mystimestamp_bitxor,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp
);

--operator >>
CREATE OPERATOR >> (
	procedure = ivymy.mystimestamp_rightmov,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp
);

--operator <<
CREATE OPERATOR << (
	procedure = ivymy.mystimestamp_leftmov,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp
);

--operator &&
CREATE OPERATOR && (
	procedure = ivymy.mystimestamp_and,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp
);

--operator ||
CREATE OPERATOR ivymy.|| (
	procedure = ivymy.mystimestamp_cat,
	leftarg = mystimestamp,
	rightarg = mystimestamp,
	commutator = ||
);

--operator !
CREATE OPERATOR ! (
	procedure = ivymy.mystimestamp_invert,
	rightarg = ivymy.mystimestamp
);

----operator ~~ like
CREATE  OR REPLACE FUNCTION ivymy.mystimestamp_like(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp2_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimestamp_like(ivymy.mystimestamp, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_text_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimestamp_like(ivymy.mystimestamp, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_int8_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimestamp_notlike(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp2_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimestamp_notlike(ivymy.mystimestamp, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_text_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimestamp_notlike(ivymy.mystimestamp, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamp_int8_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR ~~ (
	procedure = ivymy.mystimestamp_like,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mystimestamp_like,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.text
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mystimestamp_like,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mystimestamp_notlike,
	leftarg = ivymy.mystimestamp,
	rightarg = ivymy.mystimestamp
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mystimestamp_notlike,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.text
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mystimestamp_notlike,
	leftarg = ivymy.mystimestamp,
	rightarg = pg_catalog.int8
);
/*
 * B-tree Index Support
 * Create Operator family 'mystimestamptime_ops' for type 'mystimestamp/mystimestamptime/mystimestamp.
 */
/* B-tree index support procedure */
CREATE FUNCTION ivymy.mystimestamp_cmp(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS integer
AS 'MODULE_PATHNAME','mystimestamp_cmp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamp_sortsupport(internal)
RETURNS void
AS 'date_sortsupport'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

/* Create Operator family 'mystimestamp_ops' */
CREATE OPERATOR FAMILY ivymy.mystimestamptime_ops USING btree;

/* CREATE OPERATOR CLASS for 'mystimestamp' */
CREATE OPERATOR CLASS ivymy.mystimestamp_ops
	DEFAULT FOR TYPE ivymy.mystimestamp USING btree FAMILY ivymy.mystimestamptime_ops AS
		OPERATOR		1		< (ivymy.mystimestamp, ivymy.mystimestamp),
		OPERATOR		2		<= (ivymy.mystimestamp, ivymy.mystimestamp),
		OPERATOR		3		= (ivymy.mystimestamp, ivymy.mystimestamp),
		OPERATOR		4		>= (ivymy.mystimestamp, ivymy.mystimestamp),
		OPERATOR		5		> (ivymy.mystimestamp, ivymy.mystimestamp),
		FUNCTION		1		ivymy.mystimestamp_cmp(ivymy.mystimestamp, ivymy.mystimestamp),
		FUNCTION		2		ivymy.mystimestamp_sortsupport(internal);

/* HASH index support */
CREATE FUNCTION ivymy.mystimestamp_hash(ivymy.mystimestamp)
RETURNS integer
AS 'MODULE_PATHNAME','mystimestamp_hash'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystimestamp_hash_extended(ivymy.mystimestamp, bigint)
RETURNS bigint
AS 'MODULE_PATHNAME','mystimestamp_hash_extended'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


CREATE OPERATOR CLASS ivymy.mystimestamp_ops
	DEFAULT FOR TYPE ivymy.mystimestamp USING hash AS
		OPERATOR		1		= (ivymy.mystimestamp, ivymy.mystimestamp),
		FUNCTION		1		ivymy.mystimestamp_hash(ivymy.mystimestamp),
		FUNCTION		2		ivymy.mystimestamp_hash_extended(ivymy.mystimestamp, bigint);

GRANT USAGE ON SCHEMA ivymy TO PUBLIC;

--
-- CREATE AGGREGATE 
--
/* max */
CREATE FUNCTION ivymy.mystimestamp_larger(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS ivymy.mystimestamp
AS 'MODULE_PATHNAME', 'mysdatetime_larger'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.max(ivymy.mystimestamp) (
  SFUNC = ivymy.mystimestamp_larger,
  STYPE = ivymy.mystimestamp,
  SORTOP = >,
  PARALLEL = SAFE
);

/* min */
CREATE FUNCTION ivymy.mystimestamp_smaller(ivymy.mystimestamp, ivymy.mystimestamp)
RETURNS ivymy.mystimestamp
AS 'MODULE_PATHNAME', 'mysdatetime_smaller'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.min(ivymy.mystimestamp) (
  SFUNC = ivymy.mystimestamp_smaller,
  STYPE = ivymy.mystimestamp,
  SORTOP = <,
  PARALLEL = SAFE
);

---------- Compatible MySQL Cast ----------
CREATE FUNCTION ivymy.int_mystimestamp(pg_catalog.int8, int, bool, int8)
RETURNS ivymy.mystimestamp
AS 'MODULE_PATHNAME','int_mystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mystimestamp(pg_catalog.int4, int, bool, int8)
RETURNS ivymy.mystimestamp
AS 'MODULE_PATHNAME','int_mystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mystimestamp(pg_catalog.int2, int, bool, int8)
RETURNS ivymy.mystimestamp
AS 'MODULE_PATHNAME','int_mystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.numeric_mystimestamp(pg_catalog.numeric, int, bool, int8)
RETURNS ivymy.mystimestamp
AS 'MODULE_PATHNAME','numeric_mystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.int8 AS ivymy.mystimestamp)
WITH FUNCTION ivymy.int_mystimestamp(pg_catalog.int8, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.int4 AS ivymy.mystimestamp)
WITH FUNCTION ivymy.int_mystimestamp(pg_catalog.int4, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.int2 AS ivymy.mystimestamp)
WITH FUNCTION ivymy.int_mystimestamp(pg_catalog.int2, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.numeric AS ivymy.mystimestamp)
WITH FUNCTION ivymy.numeric_mystimestamp(pg_catalog.numeric, int, bool, int8)
AS ASSIGNMENT;

/* CREATE CAST */
---------- Compatible Postgresql Cast ----------
CREATE CAST (ivymy.mystimestamp AS pg_catalog.timestamp)
WITH INOUT
AS ASSIGNMENT;

CREATE CAST (pg_catalog.timestamp AS ivymy.mystimestamp)
WITH INOUT
AS ASSIGNMENT;

CREATE FUNCTION ivymy.timestamptz_mystimestamp(pg_catalog.timestamptz)
RETURNS ivymy.mystimestamp
AS 'MODULE_PATHNAME','timestamptz_mystimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.timestamptz AS ivymy.mystimestamp)
WITH FUNCTION ivymy.timestamptz_mystimestamp(pg_catalog.timestamptz)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.date AS ivymy.mystimestamp)
WITH INOUT
AS ASSIGNMENT;

--timestamp to boolean
CREATE FUNCTION ivymy.mystimestamp_boolean(ivymy.mystimestamp) 
RETURNS boolean 
AS 'MODULE_PATHNAME','mystimestamp_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mystimestamp AS boolean)
WITH FUNCTION ivymy.mystimestamp_boolean(ivymy.mystimestamp)
AS ASSIGNMENT;

-- to int8
--CREATE OR REPLACE FUNCTION ivymy.mystimestamp_int8(ivymy.mystimestamp)
--RETURNS int8
--AS 'MODULE_PATHNAME', 'mysdatetime_int8'
--LANGUAGE C 
--STABLE
--STRICT
--PARALLEL SAFE;
--CREATE CAST (ivymy.mystimestamp as bigint) WITH FUNCTION mystimestamp_int8(ivymy.mystimestamp) AS IMPLICIT;
---- to numeric
--CREATE FUNCTION ivymy.mystimestamp_numeric(ivymy.mystimestamp)
--RETURNS pg_catalog.numeric
--AS 'MODULE_PATHNAME','mysdatetime_numeric'
--LANGUAGE C
--PARALLEL SAFE
--STRICT
--STABLE;
--CREATE CAST (ivymy.mystimestamp AS pg_catalog.numeric) WITH FUNCTION ivymy.mystimestamp_numeric(ivymy.mystimestamp) AS ASSIGNMENT;

-- to int8
CREATE OR REPLACE FUNCTION ivymy.mystimestamp_int8(ivymy.mystimestamp)
RETURNS int8
AS 'MODULE_PATHNAME', 'mysdatetime_int8'
LANGUAGE C 
STABLE
STRICT
PARALLEL SAFE;
CREATE CAST (ivymy.mystimestamp as bigint) WITH FUNCTION mystimestamp_int8(ivymy.mystimestamp) AS IMPLICIT;

---- to numeric
CREATE FUNCTION ivymy.mystimestamp_numeric(ivymy.mystimestamp)
RETURNS numeric
AS 'MODULE_PATHNAME','mysdatetime_numeric'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (ivymy.mystimestamp AS pg_catalog.numeric)
WITH FUNCTION ivymy.mystimestamp_numeric(ivymy.mystimestamp)
AS ASSIGNMENT;