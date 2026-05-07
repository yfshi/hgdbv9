/***************************************************************
 *
 * MySQL timestamps type support
 *
***************************************************************/
CREATE FUNCTION ivymy.mystimestamps_in(pg_catalog.cstring)
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','mystimestamps_in'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mystimestamps_out(ivymy.mystimestamps)
RETURNS CSTRING
AS 'MODULE_PATHNAME','mystimestamps_out'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mystimestamps_recv(internal)
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','mystimestamps_recv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystimestamps_send(ivymy.mystimestamps)
RETURNS bytea
AS 'MODULE_PATHNAME','mystimestamps_send'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystimestamps_typmodin(pg_catalog.cstring[])
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','mystimestamps_typmodin'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystimestamps_typmodout(pg_catalog.int4)
RETURNS pg_catalog.cstring
AS 'MODULE_PATHNAME','mystimestamps_typmodout'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystimestamps_scale(ivymy.mysdatetime, pg_catalog.int4)
RETURNS ivymy.mysdatetime
AS 'MODULE_PATHNAME','mystimestamps_scale'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


-- Register the basic I/O for 'mystimestamps' in pg_type.
UPDATE pg_type
SET typinput=(SELECT oid FROM pg_proc WHERE proname='mystimestamps_in')
WHERE oid = 9289;

UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='mystimestamps_out')
WHERE oid = 9289;

UPDATE pg_type
SET typreceive=(SELECT oid FROM pg_proc WHERE proname='mystimestamps_recv')
WHERE oid = 9289;

UPDATE pg_type
SET typsend=(SELECT oid FROM pg_proc WHERE proname='mystimestamps_send')
WHERE oid = 9289;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='mystimestamps_typmodin')
WHERE oid = 9289;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='mystimestamps_typmodout')
WHERE oid = 9289;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='mystimestamps_typmodin')
WHERE oid = 9290;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='mystimestamps_typmodout')
WHERE oid = 9290;

/* CREATE OPERATOR */
----function eq
CREATE FUNCTION ivymy.mystimestamps_eq(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_eq'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ne
CREATE FUNCTION ivymy.mystimestamps_ne(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_ne'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function lt
CREATE FUNCTION ivymy.mystimestamps_lt(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_lt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.mystimestamps_gt(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_gt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--function le
CREATE FUNCTION ivymy.mystimestamps_le(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_le'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.mystimestamps_ge(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_ge'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.mystimestamps_seq(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function ~
CREATE FUNCTION ivymy.mystimestamps_nega(ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_negatimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ^
CREATE FUNCTION ivymy.mystimestamps_bitxor(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_bitxortimestamp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitxor(ivymy.mystimestamps, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_bitxorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitxor(ivymy.mystimestamps, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_bitxorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitxor(ivymy.mystimestamps, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_bitxorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitxor(ivymy.mystimestamps, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_bitxornum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitxor(pg_catalog.int2, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_bitxormystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitxor(pg_catalog.int4, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_bitxormystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitxor(pg_catalog.int8, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_bitxormystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_bitxor(pg_catalog.numeric, ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_bitxormystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--  "bit_count"
CREATE FUNCTION ivymy.bit_count(ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_bit_count'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

----function or
CREATE FUNCTION ivymy.mystimestamps_cat(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS varchar
AS 'select concat($1::varchar, $2::varchar);'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----operator invert !
CREATE FUNCTION ivymy.mystimestamps_invert(ivymy.mystimestamps)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimestamps_invertmystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----operator =
CREATE OPERATOR = (
	procedure = ivymy.mystimestamps_eq,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

----operator <>
CREATE OPERATOR <> (
	procedure = ivymy.mystimestamps_ne,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

--operator >
CREATE OPERATOR > (
	procedure = ivymy.mystimestamps_gt,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR < (
	procedure = ivymy.mystimestamps_lt,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator <=
CREATE OPERATOR <= (
	procedure = ivymy.mystimestamps_le,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator >=
CREATE OPERATOR >= (
	procedure = ivymy.mystimestamps_ge,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

--operator <=>
CREATE OPERATOR <=> (
	procedure = ivymy.mystimestamps_seq,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps
);

--operator ~
CREATE OPERATOR ~ (
	procedure = ivymy.mystimestamps_nega,
	rightarg = ivymy.mystimestamps
);

--operator ^
CREATE OPERATOR ^ (
	procedure = ivymy.mystimestamps_bitxor,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimestamps_bitxor,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimestamps_bitxor,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimestamps_bitxor,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimestamps_bitxor,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimestamps_bitxor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimestamps_bitxor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimestamps_bitxor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimestamps
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimestamps_bitxor,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimestamps
);

--operator ||
CREATE OPERATOR || (
	procedure = ivymy.mystimestamps_cat,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps
);

--operator !
CREATE OPERATOR ! (
	procedure = ivymy.mystimestamps_invert,
	rightarg = ivymy.mystimestamps
);

----operator ~~ like
CREATE  OR REPLACE FUNCTION ivymy.mystimestamps_like(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps2_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimestamps_like(ivymy.mystimestamps, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_text_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimestamps_like(ivymy.mystimestamps, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_int8_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimestamps_like(ivymy.mystimestamps, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_numeric_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimestamps_notlike(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps2_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimestamps_notlike(ivymy.mystimestamps, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_text_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimestamps_notlike(ivymy.mystimestamps, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_int8_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimestamps_notlike(ivymy.mystimestamps, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimestamps_numeric_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR ~~ (
	procedure = ivymy.mystimestamps_like,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mystimestamps_like,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.text
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mystimestamps_like,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ~~ (
	procedure = ivymy.mystimestamps_like,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mystimestamps_notlike,
	leftarg = ivymy.mystimestamps,
	rightarg = ivymy.mystimestamps
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mystimestamps_notlike,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.text
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mystimestamps_notlike,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.int8
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mystimestamps_notlike,
	leftarg = ivymy.mystimestamps,
	rightarg = pg_catalog.numeric
);
/*
 * B-tree Index Support
 * Create Operator family 'mystimestampstime_ops' for type 'mystimestamps/mystimestampstime/mystimestamps.
 */
/* B-tree index support procedure */
CREATE FUNCTION ivymy.mystimestamps_cmp(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS integer
AS 'MODULE_PATHNAME','mystimestamps_cmp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimestamps_sortsupport(internal)
RETURNS void
AS 'numeric_sortsupport'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

/* Create Operator family 'mystimestamps_ops' */
CREATE OPERATOR FAMILY ivymy.mystimestampstime_ops USING btree;

/* CREATE OPERATOR CLASS for 'oratimestamp' */
CREATE OPERATOR CLASS ivymy.mystimestamps_ops
	DEFAULT FOR TYPE ivymy.mystimestamps USING btree FAMILY ivymy.mystimestampstime_ops AS
		OPERATOR		1		< (ivymy.mystimestamps, ivymy.mystimestamps),
		OPERATOR		2		<= (ivymy.mystimestamps, ivymy.mystimestamps),
		OPERATOR		3		= (ivymy.mystimestamps, ivymy.mystimestamps),
		OPERATOR		4		>= (ivymy.mystimestamps, ivymy.mystimestamps),
		OPERATOR		5		> (ivymy.mystimestamps, ivymy.mystimestamps),
		FUNCTION		1		ivymy.mystimestamps_cmp(ivymy.mystimestamps, ivymy.mystimestamps),
		FUNCTION		2		ivymy.mystimestamps_sortsupport(internal);

/* HASH index support */
CREATE FUNCTION ivymy.mystimestamps_hash(ivymy.mystimestamps)
RETURNS integer
AS 'MODULE_PATHNAME','mystimestamps_hash'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystimestamps_hash_extended(ivymy.mystimestamps, bigint)
RETURNS bigint
AS 'MODULE_PATHNAME','mystimestamps_hash_extended'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


CREATE OPERATOR CLASS ivymy.mystimestamps_ops
	DEFAULT FOR TYPE ivymy.mystimestamps USING hash AS
		OPERATOR		1		= (ivymy.mystimestamps, ivymy.mystimestamps),
		FUNCTION		1		ivymy.mystimestamps_hash(ivymy.mystimestamps),
		FUNCTION		2		ivymy.mystimestamps_hash_extended(ivymy.mystimestamps, bigint);

GRANT USAGE ON SCHEMA ivymy TO PUBLIC;

--
-- CREATE AGGREGATE 
--
/* max */
CREATE FUNCTION ivymy.mystimestamps_larger(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME', 'mysdatetimes_larger'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.max(ivymy.mystimestamps) (
  SFUNC = ivymy.mystimestamps_larger,
  STYPE = ivymy.mystimestamps,
  SORTOP = >,
  PARALLEL = SAFE
);

/* min */
CREATE FUNCTION ivymy.mystimestamps_smaller(ivymy.mystimestamps, ivymy.mystimestamps)
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME', 'mysdatetimes_smaller'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.min(ivymy.mystimestamps) (
  SFUNC = ivymy.mystimestamps_smaller,
  STYPE = ivymy.mystimestamps,
  SORTOP = <,
  PARALLEL = SAFE
);

---------- Compatible MySQL Cast ----------
CREATE FUNCTION ivymy.int_mystimestamps(pg_catalog.int8, int, bool, int8)
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','int_mystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mystimestamps(pg_catalog.int4, int, bool, int8)
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','int_mystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mystimestamps(pg_catalog.int2, int, bool, int8)
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','int_mystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.numeric_mystimestamps(pg_catalog.numeric, int, bool, int8)
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','numeric_mystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.int8 AS ivymy.mystimestamps)
WITH FUNCTION ivymy.int_mystimestamps(pg_catalog.int8, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.int4 AS ivymy.mystimestamps)
WITH FUNCTION ivymy.int_mystimestamps(pg_catalog.int4, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.int2 AS ivymy.mystimestamps)
WITH FUNCTION ivymy.int_mystimestamps(pg_catalog.int2, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.numeric AS ivymy.mystimestamps)
WITH FUNCTION ivymy.numeric_mystimestamps(pg_catalog.numeric, int, bool, int8)
AS ASSIGNMENT;

/* CREATE CAST */
---------- Compatible Postgresql Cast ----------
CREATE CAST (ivymy.mystimestamps AS pg_catalog.timestamp)
WITH INOUT
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgdate_mystimestamps(pg_catalog.date, integer)
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','pgdate_mystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.date AS ivymy.mystimestamps)
WITH FUNCTION ivymy.pgdate_mystimestamps(pg_catalog.date, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgtimestamp_mystimestamps(pg_catalog.timestamp, integer)
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','pgtimestamp_mystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.timestamp AS ivymy.mystimestamps)
WITH FUNCTION ivymy.pgtimestamp_mystimestamps(pg_catalog.timestamp, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgtimestamptz_mystimestamps(pg_catalog.timestamptz, integer)
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','pgtimestamptz_mystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.timestamptz AS ivymy.mystimestamps)
WITH FUNCTION ivymy.pgtimestamptz_mystimestamps(pg_catalog.timestamptz, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgtime_mystimestamps(pg_catalog.time, integer)
RETURNS ivymy.mystimestamps
AS 'MODULE_PATHNAME','pgtime_mystimestamps'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.time AS ivymy.mystimestamps)
WITH FUNCTION ivymy.pgtime_mystimestamps(pg_catalog.time, integer)
AS ASSIGNMENT;

--timestamp to boolean
CREATE FUNCTION ivymy.mystimestamps_boolean(ivymy.mystimestamps) 
RETURNS boolean 
AS 'MODULE_PATHNAME','mystimestamps_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.mystimestamps AS boolean)
WITH FUNCTION ivymy.mystimestamps_boolean(ivymy.mystimestamps)
AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.mystimestamps_float4(ivymy.mystimestamps)
RETURNS float4
AS 'select $1::numeric::float4'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
CREATE CAST (ivymy.mystimestamps as float4) WITH FUNCTION mystimestamps_float4(ivymy.mystimestamps) AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.mystimestamps_float8(ivymy.mystimestamps)
RETURNS float8
AS 'select $1::numeric::float8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
CREATE CAST (ivymy.mystimestamps as float8) WITH FUNCTION mystimestamps_float8(ivymy.mystimestamps) AS ASSIGNMENT;

-- to int8
CREATE OR REPLACE FUNCTION ivymy.mystimestamps_int8(ivymy.mystimestamps)
RETURNS int8
AS 'MODULE_PATHNAME', 'mysdatetimes_int8'
LANGUAGE C 
STABLE
STRICT
PARALLEL SAFE;
CREATE CAST (ivymy.mystimestamps as bigint)
WITH FUNCTION mystimestamps_int8(ivymy.mystimestamps)
AS ASSIGNMENT;

-- to numeric
CREATE CAST (ivymy.mystimestamps AS pg_catalog.numeric) WITHOUT FUNCTION AS IMPLICIT;
-- End - ReqID:SRS-MYSQL-TIMESTAMPTYPE 