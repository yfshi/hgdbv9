/***************************************************************
 *
 * MySQL time type support
 *
***************************************************************/
CREATE FUNCTION ivymy.mystime_in(pg_catalog.cstring)
RETURNS ivymy.mystime
AS 'MODULE_PATHNAME','mystime_in'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mystime_out(ivymy.mystime)
RETURNS CSTRING
AS 'MODULE_PATHNAME','mystime_out'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mystime_recv(internal)
RETURNS ivymy.mystime
AS 'MODULE_PATHNAME','mystime_recv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystime_send(ivymy.mystime)
RETURNS bytea
AS 'MODULE_PATHNAME','mystime_send'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

-- Register the basic I/O for 'mystime' in pg_type.
UPDATE pg_type
SET typinput=(SELECT oid FROM pg_proc WHERE proname='mystime_in')
WHERE oid = 9291;

UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='mystime_out')
WHERE oid = 9291;

UPDATE pg_type
SET typreceive=(SELECT oid FROM pg_proc WHERE proname='mystime_recv')
WHERE oid = 9291;

UPDATE pg_type
SET typsend=(SELECT oid FROM pg_proc WHERE proname='mystime_send')
WHERE oid = 9291;

CREATE FUNCTION ivymy.int_mystime(pg_catalog.int8, int, bool, int8)
RETURNS ivymy.mystime
AS 'MODULE_PATHNAME','int_mystime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mystime(pg_catalog.int4, int, bool, int8)
RETURNS ivymy.mystime
AS 'MODULE_PATHNAME','int_mystime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mystime(pg_catalog.int2, int, bool, int8)
RETURNS ivymy.mystime
AS 'MODULE_PATHNAME','int_mystime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.numeric_mystime(pg_catalog.numeric, int, bool, int8)
RETURNS ivymy.mystime
AS 'MODULE_PATHNAME','numeric_mystime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.int8 AS ivymy.mystime)
WITH FUNCTION ivymy.int_mystime(pg_catalog.int8, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.int4 AS ivymy.mystime)
WITH FUNCTION ivymy.int_mystime(pg_catalog.int4, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.int2 AS ivymy.mystime)
WITH FUNCTION ivymy.int_mystime(pg_catalog.int2, int, bool, int8)
AS ASSIGNMENT;

CREATE CAST (pg_catalog.numeric AS ivymy.mystime)
WITH FUNCTION ivymy.numeric_mystime(pg_catalog.numeric, int, bool, int8)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.mystime_numeric(ivymy.mystime)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mystime_numeric'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (ivymy.mystime AS pg_catalog.numeric)
WITH FUNCTION ivymy.mystime_numeric(ivymy.mystime)
AS ASSIGNMENT;


CREATE FUNCTION ivymy.pgdate_mystime(pg_catalog.date)
RETURNS ivymy.mystime
AS 'MODULE_PATHNAME','pgdate_mystime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.date AS ivymy.mystime)
WITH FUNCTION ivymy.pgdate_mystime(pg_catalog.date)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgtimestamp_mystime(pg_catalog.timestamp)
RETURNS ivymy.mystime
AS 'MODULE_PATHNAME','pgtimestamp_mystime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.timestamp AS ivymy.mystime)
WITH FUNCTION ivymy.pgtimestamp_mystime(pg_catalog.timestamp)
AS ASSIGNMENT;

CREATE FUNCTION ivymy.pgtimestamptz_mystime(pg_catalog.timestamptz)
RETURNS ivymy.mystime
AS 'MODULE_PATHNAME','pgtimestamptz_mystime'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.timestamptz AS ivymy.mystime)
WITH FUNCTION ivymy.pgtimestamptz_mystime(pg_catalog.timestamptz)
AS ASSIGNMENT;


----function eq
CREATE FUNCTION ivymy.mystime_eq(ivymy.mystime, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_eq'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ne
CREATE FUNCTION ivymy.mystime_ne(ivymy.mystime, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_ne'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


----function lt
CREATE FUNCTION ivymy.mystime_lt(ivymy.mystime, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_lt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.mystime_gt(ivymy.mystime, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_gt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--function le
CREATE FUNCTION ivymy.mystime_le(ivymy.mystime, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_le'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.mystime_ge(ivymy.mystime, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_ge'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.mystime_seq(ivymy.mystime, ivymy.mystime)
RETURNS boolean
AS 'MODULE_PATHNAME','mystime_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;

----function ~
CREATE FUNCTION ivymy.mystime_nega(ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_neg'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--  "bit_count"
CREATE FUNCTION ivymy.bit_count(ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_bit_count'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

----function or
CREATE FUNCTION ivymy.mystime_cat(ivymy.mystime, ivymy.mystime)
RETURNS varchar
AS 'select concat($1::varchar, $2::varchar);'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----operator invert !
CREATE FUNCTION ivymy.mystime_invert(ivymy.mystime)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mystime_invert'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--time to boolean
CREATE FUNCTION ivymy.mystime_boolean(ivymy.mystime) 
RETURNS boolean 
AS 'MODULE_PATHNAME','mystime_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE CAST (ivymy.mystime AS boolean)
WITH FUNCTION ivymy.mystime_boolean(ivymy.mystime)
AS ASSIGNMENT;

----operator =
CREATE OPERATOR = (
	procedure = ivymy.mystime_eq,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

----operator <>
CREATE OPERATOR <> (
	procedure = ivymy.mystime_ne,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

--operator >
CREATE OPERATOR > (
	procedure = ivymy.mystime_gt,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR < (
	procedure = ivymy.mystime_lt,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator <=
CREATE OPERATOR <= (
	procedure = ivymy.mystime_le,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator >=
CREATE OPERATOR >= (
	procedure = ivymy.mystime_ge,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

--operator <=>
CREATE OPERATOR <=> (
	procedure = ivymy.mystime_seq,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);

--operator ~
CREATE OPERATOR ~ (
	procedure = ivymy.mystime_nega,
	rightarg = ivymy.mystime
);

--operator ||
CREATE OPERATOR || (
	procedure = ivymy.mystime_cat,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);

--operator !
CREATE OPERATOR ! (
	procedure = ivymy.mystime_invert,
	rightarg = ivymy.mystime
);

----operator ~~ like
CREATE  OR REPLACE FUNCTION ivymy.mystime_like(ivymy.mystime, pg_catalog.text)
RETURNS boolean
AS 'select $1::text ~~ $2::text'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystime_like(ivymy.mystime, ivymy.mystime)
RETURNS boolean
AS 'select $1::text ~~ $2::text'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystime_like(ivymy.mystime, pg_catalog.int4)
RETURNS boolean
AS 'select $1::text ~~ $2::text'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystime_like(ivymy.mystime, pg_catalog.int8)
RETURNS boolean
AS 'select $1::text ~~ $2::text'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystime_like(ivymy.mystime, ivymy.mystimes)
RETURNS boolean
AS 'select $1::text ~~ $2::text'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

-- not like !~~
CREATE  OR REPLACE FUNCTION ivymy.mystime_notlike(ivymy.mystime, pg_catalog.text)
RETURNS boolean
AS 'select $1::text !~~ $2::text'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystime_notlike(ivymy.mystime, ivymy.mystime)
RETURNS boolean
AS 'select $1::text !~~ $2::text'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystime_notlike(ivymy.mystime, pg_catalog.int4)
RETURNS boolean
AS 'select $1::text !~~ $2::text'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mystime_notlike(ivymy.mystime, pg_catalog.int8)
RETURNS boolean
AS 'select $1::text !~~ $2::text'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR ~~ (
	procedure = ivymy.mystime_like,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mystime_like,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.text
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mystime_like,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int4
);

CREATE OPERATOR ~~ (
	procedure = ivymy.mystime_like,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR ~~ (
	procedure = ivymy.mystime_like,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystimes
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mystime_notlike,
	leftarg = ivymy.mystime,
	rightarg = ivymy.mystime
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mystime_notlike,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.text
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mystime_notlike,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int4
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mystime_notlike,
	leftarg = ivymy.mystime,
	rightarg = pg_catalog.int8
);

CREATE FUNCTION ivymy.mystime_cmp(ivymy.mystime, ivymy.mystime)
RETURNS integer
AS 'MODULE_PATHNAME','mystime_cmp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mystime_sortsupport(internal)
RETURNS void
AS 'btint8sortsupport'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR FAMILY ivymy.mystimetime_ops USING btree;

CREATE OPERATOR CLASS ivymy.mystime_ops
	DEFAULT FOR TYPE ivymy.mystime USING btree FAMILY ivymy.mystimetime_ops AS
		OPERATOR		1		< (ivymy.mystime, ivymy.mystime),
		OPERATOR		2		<= (ivymy.mystime, ivymy.mystime),
		OPERATOR		3		= (ivymy.mystime, ivymy.mystime),
		OPERATOR		4		>= (ivymy.mystime, ivymy.mystime),
		OPERATOR		5		> (ivymy.mystime, ivymy.mystime),
		FUNCTION		1		ivymy.mystime_cmp(ivymy.mystime, ivymy.mystime),
		FUNCTION		2		ivymy.mystime_sortsupport(internal);

CREATE FUNCTION ivymy.mystime_hash(ivymy.mystime)
RETURNS integer
AS 'MODULE_PATHNAME','mystime_hash'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mystime_hash_extended(ivymy.mystime, bigint)
RETURNS bigint
AS 'MODULE_PATHNAME','mystime_hash_extended'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR CLASS ivymy.mystime_ops
	DEFAULT FOR TYPE ivymy.mystime USING hash AS
		OPERATOR		1		= (ivymy.mystime, ivymy.mystime),
		FUNCTION		1		ivymy.mystime_hash(ivymy.mystime),
		FUNCTION		2		ivymy.mystime_hash_extended(ivymy.mystime, bigint);

GRANT USAGE ON SCHEMA ivymy TO PUBLIC;


--
-- CREATE AGGREGATE 
--
/* max */
CREATE FUNCTION ivymy.mystime_larger(ivymy.mystime, ivymy.mystime)
RETURNS ivymy.mystime
AS 'MODULE_PATHNAME', 'mysdatetime_larger'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.max(ivymy.mystime) (
  SFUNC = ivymy.mystime_larger,
  STYPE = ivymy.mystime,
  SORTOP = >,
  PARALLEL = SAFE
);

/* min */
CREATE FUNCTION ivymy.mystime_smaller(ivymy.mystime, ivymy.mystime)
RETURNS ivymy.mystime
AS 'MODULE_PATHNAME', 'mysdatetime_smaller'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.min(ivymy.mystime) (
  SFUNC = ivymy.mystime_smaller,
  STYPE = ivymy.mystime,
  SORTOP = <,
  PARALLEL = SAFE
);


CREATE OR REPLACE FUNCTION ivymy.mystime_float4(ivymy.mystime)
RETURNS float4
AS 'select $1::int8::float4'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
CREATE CAST (ivymy.mystime as float4) WITH FUNCTION mystime_float4(ivymy.mystime) AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.mystime_float8(ivymy.mystime)
RETURNS float8
AS 'select $1::int8::float8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
CREATE CAST (ivymy.mystime as float8) WITH FUNCTION mystime_float8(ivymy.mystime) AS ASSIGNMENT;

CREATE CAST (ivymy.mystime as bigint) WITHOUT FUNCTION AS IMPLICIT;
