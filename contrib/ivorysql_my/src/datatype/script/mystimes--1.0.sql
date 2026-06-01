-- Begin - ReqID:SRS-MYSQL-TIMETYPE 
/***************************************************************
 *
 * MySQL times type support
 *
***************************************************************/
CREATE FUNCTION ivymy.mystimes_in(pg_catalog.cstring)
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','mystimes_in'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mystimes_out(ivymy.mystimes)
RETURNS CSTRING
AS 'MODULE_PATHNAME','mystimes_out'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mystimes_recv(internal)
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','mystimes_recv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystimes_send(ivymy.mystimes)
RETURNS bytea
AS 'MODULE_PATHNAME','mystimes_send'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystimes_typmodin(pg_catalog.cstring[])
RETURNS pg_catalog.int4
AS 'MODULE_PATHNAME','mystimes_typmodin'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystimes_typmodout(pg_catalog.int4)
RETURNS pg_catalog.cstring
AS 'MODULE_PATHNAME','mystimes_typmodout'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystimes_scale(ivymy.mysdatetime, pg_catalog.int4)
RETURNS ivymy.mysdatetime
AS 'MODULE_PATHNAME','mystimes_scale'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


-- Register the basic I/O for 'mystimes' in pg_type.
UPDATE pg_type
SET typinput=(SELECT oid FROM pg_proc WHERE proname='mystimes_in')
WHERE oid = 9293;

UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='mystimes_out')
WHERE oid = 9293;

UPDATE pg_type
SET typreceive=(SELECT oid FROM pg_proc WHERE proname='mystimes_recv')
WHERE oid = 9293;

UPDATE pg_type
SET typsend=(SELECT oid FROM pg_proc WHERE proname='mystimes_send')
WHERE oid = 9293;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='mystimes_typmodin')
WHERE oid = 9293;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='mystimes_typmodout')
WHERE oid = 9293;

UPDATE pg_type
SET typmodin=(SELECT oid FROM pg_proc WHERE proname='mystimes_typmodin')
WHERE oid = 9294;

UPDATE pg_type
SET typmodout=(SELECT oid FROM pg_proc WHERE proname='mystimes_typmodout')
WHERE oid = 9294;

/* CREATE OPERATOR */
----function eq
CREATE FUNCTION ivymy.mystimes_eq(ivymy.mystimes, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_eq'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ne
CREATE FUNCTION ivymy.mystimes_ne(ivymy.mystimes, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_ne'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function lt
CREATE FUNCTION ivymy.mystimes_lt(ivymy.mystimes, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_lt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.mystimes_gt(ivymy.mystimes, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_gt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--function le
CREATE FUNCTION ivymy.mystimes_le(ivymy.mystimes, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_le'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.mystimes_ge(ivymy.mystimes, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_ge'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.mystimes_seq(ivymy.mystimes, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function ~
CREATE FUNCTION ivymy.mystimes_nega(ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_negatimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ^
CREATE FUNCTION ivymy.mystimes_bitxor(ivymy.mystimes, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_bitxordatetime'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitxor(ivymy.mystimes, pg_catalog.int2)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_bitxorint2'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitxor(ivymy.mystimes, pg_catalog.int4)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_bitxorint4'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitxor(ivymy.mystimes, pg_catalog.int8)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_bitxorint8'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitxor(ivymy.mystimes, pg_catalog.numeric)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_bitxornum'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitxor(pg_catalog.int2, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int2_bitxormystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitxor(pg_catalog.int4, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int4_bitxormystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitxor(pg_catalog.int8, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','int8_bitxormystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_bitxor(pg_catalog.numeric, ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','num_bitxormystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--  "bit_count"
CREATE FUNCTION ivymy.bit_count(ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_bit_count'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

----function or
CREATE FUNCTION ivymy.mystimes_cat(ivymy.mystimes, ivymy.mystimes)
RETURNS varchar
AS 'select concat($1::varchar, $2::varchar);'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----operator invert !
CREATE FUNCTION ivymy.mystimes_invert(ivymy.mystimes)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystimes_invertmystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----operator =
CREATE OPERATOR = (
	procedure = ivymy.mystimes_eq,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

----operator <>
CREATE OPERATOR <> (
	procedure = ivymy.mystimes_ne,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

--operator >
CREATE OPERATOR > (
	procedure = ivymy.mystimes_gt,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR < (
	procedure = ivymy.mystimes_lt,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator <=
CREATE OPERATOR <= (
	procedure = ivymy.mystimes_le,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator >=
CREATE OPERATOR >= (
	procedure = ivymy.mystimes_ge,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

--operator <=>
CREATE OPERATOR <=> (
	procedure = ivymy.mystimes_seq,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes
);

--operator ~
CREATE OPERATOR ~ (
	procedure = ivymy.mystimes_nega,
	rightarg = ivymy.mystimes
);

--operator ^
CREATE OPERATOR ^ (
	procedure = ivymy.mystimes_bitxor,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimes_bitxor,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int2
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimes_bitxor,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimes_bitxor,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimes_bitxor,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimes_bitxor,
	leftarg = pg_catalog.int2,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimes_bitxor,
	leftarg = pg_catalog.int4,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimes_bitxor,
	leftarg = pg_catalog.int8,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR ^ (
	procedure = ivymy.mystimes_bitxor,
	leftarg = pg_catalog.numeric,
	rightarg = ivymy.mystimes
);

--operator ||
CREATE OPERATOR || (
	procedure = ivymy.mystimes_cat,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes
);

--operator !
CREATE OPERATOR ! (
	procedure = ivymy.mystimes_invert,
	rightarg = ivymy.mystimes
);

----operator ~~ like
CREATE  OR REPLACE FUNCTION ivymy.mystimes_like(ivymy.mystimes, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes2_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimes_like(ivymy.mystimes, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_text_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimes_like(ivymy.mystimes, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_int4_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimes_like(ivymy.mystimes, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_int8_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimes_like(ivymy.mystimes, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_numeric_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimes_notlike(ivymy.mystimes, ivymy.mystimes)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes2_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimes_notlike(ivymy.mystimes, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_text_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimes_notlike(ivymy.mystimes, pg_catalog.int4)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_int4_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimes_notlike(ivymy.mystimes, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_int8_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystimes_notlike(ivymy.mystimes, pg_catalog.numeric)
RETURNS boolean
AS 'MODULE_PATHNAME','mystimes_numeric_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR ~~ (
	procedure = ivymy.mystimes_like,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mystimes_like,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.text
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mystimes_like,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int4
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mystimes_like,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ~~ (
	procedure = ivymy.mystimes_like,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.numeric
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mystimes_notlike,
	leftarg = ivymy.mystimes,
	rightarg = ivymy.mystimes
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mystimes_notlike,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.text
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mystimes_notlike,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int4
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mystimes_notlike,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.int8
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mystimes_notlike,
	leftarg = ivymy.mystimes,
	rightarg = pg_catalog.numeric
);
/*
 * B-tree Index Support
 * Create Operator family 'mystimestime_ops' for type 'mystimes/mystimestime/mystimes.
 */
/* B-tree index support procedure */
CREATE FUNCTION ivymy.mystimes_cmp(ivymy.mystimes, ivymy.mystimes)
RETURNS integer
AS 'MODULE_PATHNAME','mystimes_cmp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystimes_sortsupport(internal)
RETURNS void
AS 'numeric_sortsupport'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

/* Create Operator family 'mystimes_ops' */
CREATE OPERATOR FAMILY ivymy.mystimestime_ops USING btree;

/* CREATE OPERATOR CLASS for 'oradatetime' */
CREATE OPERATOR CLASS ivymy.mystimes_ops
	DEFAULT FOR TYPE ivymy.mystimes USING btree FAMILY ivymy.mystimestime_ops AS
		OPERATOR		1		< (ivymy.mystimes, ivymy.mystimes),
		OPERATOR		2		<= (ivymy.mystimes, ivymy.mystimes),
		OPERATOR		3		= (ivymy.mystimes, ivymy.mystimes),
		OPERATOR		4		>= (ivymy.mystimes, ivymy.mystimes),
		OPERATOR		5		> (ivymy.mystimes, ivymy.mystimes),
		FUNCTION		1		ivymy.mystimes_cmp(ivymy.mystimes, ivymy.mystimes),
		FUNCTION		2		ivymy.mystimes_sortsupport(internal);

/* HASH index support */
CREATE FUNCTION ivymy.mystimes_hash(ivymy.mystimes)
RETURNS integer
AS 'MODULE_PATHNAME','mystimes_hash'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystimes_hash_extended(ivymy.mystimes, bigint)
RETURNS bigint
AS 'MODULE_PATHNAME','mystimes_hash_extended'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


CREATE OPERATOR CLASS ivymy.mystimes_ops
	DEFAULT FOR TYPE ivymy.mystimes USING hash AS
		OPERATOR		1		= (ivymy.mystimes, ivymy.mystimes),
		FUNCTION		1		ivymy.mystimes_hash(ivymy.mystimes),
		FUNCTION		2		ivymy.mystimes_hash_extended(ivymy.mystimes, bigint);

GRANT USAGE ON SCHEMA ivymy TO PUBLIC;

--
-- CREATE AGGREGATE 
--
/* max */
CREATE FUNCTION ivymy.mystimes_larger(ivymy.mystimes, ivymy.mystimes)
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME', 'mysdatetimes_larger'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.max(ivymy.mystimes) (
  SFUNC = ivymy.mystimes_larger,
  STYPE = ivymy.mystimes,
  SORTOP = >,
  PARALLEL = SAFE
);

/* min */
CREATE FUNCTION ivymy.mystimes_smaller(ivymy.mystimes, ivymy.mystimes)
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME', 'mysdatetimes_smaller'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.min(ivymy.mystimes) (
  SFUNC = ivymy.mystimes_smaller,
  STYPE = ivymy.mystimes,
  SORTOP = <,
  PARALLEL = SAFE
);

---------- Compatible MySQL Cast ----------
CREATE FUNCTION ivymy.int_mystimes(pg_catalog.int8, integer, bool, int8)
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','int_mystime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mystimes(pg_catalog.int4, integer, bool, int8)
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','int_mystime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mystimes(pg_catalog.int2, integer, bool, int8)
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','int_mystime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.numeric_mystimes(pg_catalog.numeric, integer, bool, int8)
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','numeric_mystime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.int8 AS ivymy.mystimes)
WITH FUNCTION ivymy.int_mystimes(pg_catalog.int8, integer, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.int4 AS ivymy.mystimes)
WITH FUNCTION ivymy.int_mystimes(pg_catalog.int4, integer, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.int2 AS ivymy.mystimes)
WITH FUNCTION ivymy.int_mystimes(pg_catalog.int2, integer, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.numeric AS ivymy.mystimes)
WITH FUNCTION ivymy.numeric_mystimes(pg_catalog.numeric, integer, bool, int8)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgdate_mystimes(pg_catalog.date, integer)
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','pgdate_mystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.date AS ivymy.mystimes)
WITH FUNCTION ivymy.pgdate_mystimes(pg_catalog.date, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgtimestamp_mystimes(pg_catalog.timestamp, integer)
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','pgtimestamp_mystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.timestamp AS ivymy.mystimes)
WITH FUNCTION ivymy.pgtimestamp_mystimes(pg_catalog.timestamp, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgtimestamptz_mystimes(pg_catalog.timestamptz, integer)
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','pgtimestamptz_mystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.timestamptz AS ivymy.mystimes)
WITH FUNCTION ivymy.pgtimestamptz_mystimes(pg_catalog.timestamptz, integer)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgtime_mystimes(pg_catalog.time, integer)
RETURNS ivymy.mystimes
AS 'MODULE_PATHNAME','pgtime_mystimes'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (pg_catalog.time AS ivymy.mystimes)
WITH FUNCTION ivymy.pgtime_mystimes(pg_catalog.time, integer)
AS ASSIGNMENT;

--datetime to boolean
CREATE FUNCTION ivymy.mystimes_boolean(ivymy.mystimes) 
RETURNS boolean 
AS 'MODULE_PATHNAME','mystimes_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.mystimes AS boolean)
WITH FUNCTION ivymy.mystimes_boolean(ivymy.mystimes)
AS ASSIGNMENT;


CREATE OR REPLACE FUNCTION ivymy.mystimes_float4(ivymy.mystimes)
RETURNS float4
AS 'select $1::numeric::float4'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
CREATE CAST (ivymy.mystimes as float4) WITH FUNCTION mystimes_float4(ivymy.mystimes) AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.mystimes_float8(ivymy.mystimes)
RETURNS float8
AS 'select $1::numeric::float8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
CREATE CAST (ivymy.mystimes as float8) WITH FUNCTION mystimes_float8(ivymy.mystimes) AS ASSIGNMENT;

-- to int8
CREATE OR REPLACE FUNCTION ivymy.mystimes_int8(ivymy.mystimes)
RETURNS int8
AS 'MODULE_PATHNAME', 'mystimes_int8'
LANGUAGE C 
STABLE
STRICT
PARALLEL SAFE;
CREATE CAST (ivymy.mystimes as bigint)
WITH FUNCTION mystimes_int8(ivymy.mystimes)
AS ASSIGNMENT;

-- to numeric
CREATE CAST (ivymy.mystimes AS pg_catalog.numeric) WITHOUT FUNCTION AS IMPLICIT;

CREATE CAST (ivymy.mystimes AS ivymy.mystime)
WITH INOUT
AS ASSIGNMENT;
-- End - ReqID:SRS-MYSQL-TIMETYPE 


