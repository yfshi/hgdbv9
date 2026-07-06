/***************************************************************
 *
 * MySQL date type support
 *
***************************************************************/
CREATE FUNCTION ivymy.mysdate_in(pg_catalog.cstring)
RETURNS ivymy.mysdate
AS 'MODULE_PATHNAME','mysdate_in'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mysdate_out(ivymy.mysdate)
RETURNS CSTRING
AS 'MODULE_PATHNAME','mysdate_out'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.mysdate_recv(internal)
RETURNS ivymy.mysdate
AS 'MODULE_PATHNAME','mysdate_recv'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysdate_send(ivymy.mysdate)
RETURNS bytea
AS 'MODULE_PATHNAME','mysdate_send'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

-- Register the basic I/O for 'mysdate' in pg_type.
UPDATE pg_type
SET typinput=(SELECT oid FROM pg_proc WHERE proname='mysdate_in')
WHERE oid = 9281;

UPDATE pg_type
SET typoutput=(SELECT oid FROM pg_proc WHERE proname='mysdate_out')
WHERE oid = 9281;

UPDATE pg_type
SET typreceive=(SELECT oid FROM pg_proc WHERE proname='mysdate_recv')
WHERE oid = 9281;

UPDATE pg_type
SET typsend=(SELECT oid FROM pg_proc WHERE proname='mysdate_send')
WHERE oid = 9281;


/* CREATE OPERATOR */
----function eq
CREATE FUNCTION ivymy.mysdate_eq(ivymy.mysdate, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_eq'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function ne
CREATE FUNCTION ivymy.mysdate_ne(ivymy.mysdate, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_ne'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function lt
CREATE FUNCTION ivymy.mysdate_lt(ivymy.mysdate, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_lt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function gt
CREATE FUNCTION ivymy.mysdate_gt(ivymy.mysdate, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_gt'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

--function le
CREATE FUNCTION ivymy.mysdate_le(ivymy.mysdate, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_le'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

---function ge
CREATE FUNCTION ivymy.mysdate_ge(ivymy.mysdate, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_ge'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

----function seq
CREATE FUNCTION ivymy.mysdate_seq(ivymy.mysdate, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_seq'
LANGUAGE C
PARALLEL SAFE
IMMUTABLE
LEAKPROOF;
--  "bit_count"
CREATE FUNCTION ivymy.bit_count(ivymy.mysdate)
RETURNS pg_catalog.int8
AS 'MODULE_PATHNAME','mysdate_bit_count'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

----function or
CREATE FUNCTION ivymy.mysdate_cat(ivymy.mysdate, ivymy.mysdate)
RETURNS varchar
AS 'select concat($1::varchar, $2::varchar);'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
----operator =
CREATE OPERATOR = (
	procedure = ivymy.mysdate_eq,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate,
	commutator = =,
	negator = <>,
	restrict = eqsel,
	join = eqjoinsel,
	hashes,
	merges
);

----operator <>
CREATE OPERATOR <> (
	procedure = ivymy.mysdate_ne,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate,
	commutator = <>,
	negator = =,
	restrict = neqsel,
	join = neqjoinsel
);

--operator >
CREATE OPERATOR > (
	procedure = ivymy.mysdate_gt,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate,
	commutator = <,
	negator = <=,
	restrict = scalargtsel,
	join = scalargtjoinsel
);

----operator <
CREATE OPERATOR < (
	procedure = ivymy.mysdate_lt,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate,
	commutator = >,
	negator = >=,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator <=
CREATE OPERATOR <= (
	procedure = ivymy.mysdate_le,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate,
	commutator = >=,
	negator = >,
	restrict = scalarltsel,
	join = scalarltjoinsel
);

--operator >=
CREATE OPERATOR >= (
	procedure = ivymy.mysdate_ge,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate,
	commutator = <=,
	negator = <,
	restrict = scalargtsel,
	join = scalargtjoinsel
);


--operator <=>
CREATE OPERATOR <=> (
	procedure = ivymy.mysdate_seq,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate
);
--operator ||
CREATE OPERATOR || (
	procedure = ivymy.mysdate_cat,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate
);

-- eq: date = numeric
CREATE FUNCTION ivymy.mysdate_eq(ivymy.mysdate, numeric)
RETURNS bool
AS 'select $1::int8::numeric = $2::numeric;'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE OPERATOR ivymy.= (
	procedure = ivymy.mysdate_eq,
	leftarg = ivymy.mysdate,
	rightarg = numeric
);
----operator ~~ like
CREATE  OR REPLACE FUNCTION ivymy.mysdate_like(ivymy.mysdate, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate2_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdate_like(ivymy.mysdate, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_text_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdate_like(ivymy.mysdate, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_int8_like'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdate_notlike(ivymy.mysdate, ivymy.mysdate)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate2_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdate_notlike(ivymy.mysdate, pg_catalog.text)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_text_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE  OR REPLACE FUNCTION ivymy.mysdate_notlike(ivymy.mysdate, pg_catalog.int8)
RETURNS boolean
AS 'MODULE_PATHNAME','mysdate_int8_notlike'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR ~~ (
	procedure = ivymy.mysdate_like,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mysdate_like,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.text
);
CREATE OPERATOR ~~ (
	procedure = ivymy.mysdate_like,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int8
);

CREATE OPERATOR !~~ (
	procedure = ivymy.mysdate_notlike,
	leftarg = ivymy.mysdate,
	rightarg = ivymy.mysdate
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mysdate_notlike,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.text
);
CREATE OPERATOR !~~ (
	procedure = ivymy.mysdate_notlike,
	leftarg = ivymy.mysdate,
	rightarg = pg_catalog.int8
);

/*
 * B-tree Index Support
 * Create Operator family 'mysdatetime_ops' for type 'mysdate/mysdatetime/mystimestamp.
 */
/* B-tree index support procedure */
CREATE FUNCTION ivymy.mysdate_cmp(ivymy.mysdate, ivymy.mysdate)
RETURNS integer
AS 'MODULE_PATHNAME','mysdate_cmp'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE FUNCTION ivymy.mysdate_sortsupport(internal)
RETURNS void
AS 'date_sortsupport'
LANGUAGE internal
PARALLEL SAFE
STRICT
IMMUTABLE;

/* Create Operator family 'mysdatetime_ops' */
CREATE OPERATOR FAMILY ivymy.mysdatetime_ops USING btree;

/* CREATE OPERATOR CLASS for 'oradate' */
CREATE OPERATOR CLASS ivymy.mysdate_ops
	DEFAULT FOR TYPE ivymy.mysdate USING btree FAMILY ivymy.mysdatetime_ops AS
		OPERATOR		1		< (ivymy.mysdate, ivymy.mysdate),
		OPERATOR		2		<= (ivymy.mysdate, ivymy.mysdate),
		OPERATOR		3		= (ivymy.mysdate, ivymy.mysdate),
		OPERATOR		4		>= (ivymy.mysdate, ivymy.mysdate),
		OPERATOR		5		> (ivymy.mysdate, ivymy.mysdate),
		FUNCTION		1		ivymy.mysdate_cmp(ivymy.mysdate, ivymy.mysdate),
		FUNCTION		2		ivymy.mysdate_sortsupport(internal);

/* HASH index support */
CREATE FUNCTION ivymy.mysdate_hash(ivymy.mysdate)
RETURNS integer
AS 'MODULE_PATHNAME','mysdate_hash'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE FUNCTION ivymy.mysdate_hash_extended(ivymy.mysdate, bigint)
RETURNS bigint
AS 'MODULE_PATHNAME','mys_date_hash_extended'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;


CREATE OPERATOR CLASS ivymy.mysdate_ops
	DEFAULT FOR TYPE ivymy.mysdate USING hash AS
		OPERATOR		1		= (ivymy.mysdate, ivymy.mysdate),
		FUNCTION		1		ivymy.mysdate_hash(ivymy.mysdate),
		FUNCTION		2		ivymy.mysdate_hash_extended(ivymy.mysdate, bigint);

GRANT USAGE ON SCHEMA ivymy TO PUBLIC;

--
-- CREATE AGGREGATE 
--
/* max */
CREATE FUNCTION ivymy.mysdate_larger(ivymy.mysdate, ivymy.mysdate)
RETURNS ivymy.mysdate
AS 'MODULE_PATHNAME', 'mysdate_larger'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.max(ivymy.mysdate) (
  SFUNC = ivymy.mysdate_larger,
  STYPE = ivymy.mysdate,
  SORTOP = >,
  PARALLEL = SAFE
);

/* min */
CREATE FUNCTION ivymy.mysdate_smaller(ivymy.mysdate, ivymy.mysdate)
RETURNS ivymy.mysdate
AS 'MODULE_PATHNAME', 'mysdate_smaller'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;

CREATE AGGREGATE ivymy.min(ivymy.mysdate) (
  SFUNC = ivymy.mysdate_smaller,
  STYPE = ivymy.mysdate,
  SORTOP = <,
  PARALLEL = SAFE
);

CREATE FUNCTION ivymy.int_mysdate(pg_catalog.int8, int, bool, int8)
RETURNS ivymy.mysdate
AS 'MODULE_PATHNAME','int_mysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mysdate(pg_catalog.int4, int, bool, int8)
RETURNS ivymy.mysdate
AS 'MODULE_PATHNAME','int_mysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.int_mysdate(pg_catalog.int2, int, bool, int8)
RETURNS ivymy.mysdate
AS 'MODULE_PATHNAME','int_mysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE FUNCTION ivymy.numeric_mysdate(pg_catalog.numeric)
RETURNS ivymy.mysdate
AS 'MODULE_PATHNAME','numeric_mysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;

CREATE CAST (pg_catalog.int8 AS ivymy.mysdate) WITH FUNCTION ivymy.int_mysdate(pg_catalog.int8, int, bool, int8) AS ASSIGNMENT;
CREATE CAST (pg_catalog.int4 AS ivymy.mysdate) WITH FUNCTION ivymy.int_mysdate(pg_catalog.int4, int, bool, int8) AS ASSIGNMENT;
CREATE CAST (pg_catalog.int2 AS ivymy.mysdate) WITH FUNCTION ivymy.int_mysdate(pg_catalog.int2, int, bool, int8) AS ASSIGNMENT;
CREATE CAST (pg_catalog.numeric AS ivymy.mysdate) WITH FUNCTION ivymy.numeric_mysdate(pg_catalog.numeric) AS ASSIGNMENT;

CREATE FUNCTION ivymy.date_mysdate(pg_catalog.date)
RETURNS ivymy.mysdate
AS 'MODULE_PATHNAME','date_mysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (pg_catalog.date AS ivymy.mysdate) WITH FUNCTION ivymy.date_mysdate(pg_catalog.date) AS ASSIGNMENT;

CREATE FUNCTION ivymy.timestamp_mysdate(pg_catalog.timestamp)
RETURNS ivymy.mysdate
AS 'MODULE_PATHNAME','timestamp_mysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (pg_catalog.timestamp AS ivymy.mysdate) WITH FUNCTION ivymy.timestamp_mysdate(pg_catalog.timestamp) AS ASSIGNMENT;

CREATE FUNCTION ivymy.timestamptz_mysdate(pg_catalog.timestamptz)
RETURNS ivymy.mysdate
AS 'MODULE_PATHNAME','timestamptz_mysdate'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (pg_catalog.timestamptz AS ivymy.mysdate) WITH FUNCTION ivymy.timestamptz_mysdate(pg_catalog.timestamptz) AS ASSIGNMENT;

CREATE FUNCTION ivymy.mysdate_boolean(ivymy.mysdate) 
RETURNS boolean 
AS 'MODULE_PATHNAME','mysdate_boolean'
LANGUAGE C
PARALLEL SAFE
STRICT
IMMUTABLE;
CREATE CAST (ivymy.mysdate AS boolean) WITH FUNCTION ivymy.mysdate_boolean(ivymy.mysdate) AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.mysdate_float4(ivymy.mysdate)
RETURNS float4
AS 'select $1::int8::float4'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
CREATE CAST (ivymy.mysdate as float4) WITH FUNCTION mysdate_float4(ivymy.mysdate) AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.mysdate_float8(ivymy.mysdate)
RETURNS float8
AS 'select $1::int8::float8'
LANGUAGE SQL
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;
CREATE CAST (ivymy.mysdate as float8) WITH FUNCTION mysdate_float8(ivymy.mysdate) AS ASSIGNMENT;

CREATE OR REPLACE FUNCTION ivymy.mysdate_int8(ivymy.mysdate)
RETURNS int8
AS 'MODULE_PATHNAME', 'mysdate_int8'
LANGUAGE C 
STABLE
STRICT
PARALLEL SAFE;
CREATE CAST (ivymy.mysdate as bigint) WITH FUNCTION mysdate_int8(ivymy.mysdate) AS ASSIGNMENT;

CREATE CAST (ivymy.mysdate as int) WITHOUT FUNCTION AS IMPLICIT;

CREATE FUNCTION ivymy.mysdate_numeric(ivymy.mysdate)
RETURNS pg_catalog.numeric
AS 'MODULE_PATHNAME','mysdate_numeric'
LANGUAGE C
PARALLEL SAFE
STRICT
STABLE;
CREATE CAST (ivymy.mysdate AS pg_catalog.numeric) WITH FUNCTION ivymy.mysdate_numeric(ivymy.mysdate) AS ASSIGNMENT;

/* Begin bug#Z8459 */
CREATE OR REPLACE FUNCTION ivymy.mysdate_minus_interval(ivymy.mysdate, pg_catalog.interval)
RETURNS ivymy.mysdatetime
AS $$ SELECT (($1::text::pg_catalog.date) - $2)::ivymy.mysdatetime $$
LANGUAGE sql
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;

CREATE OPERATOR - (
       procedure = ivymy.mysdate_minus_interval,
       leftarg = ivymy.mysdate,
       rightarg = pg_catalog.interval
);

CREATE OR REPLACE FUNCTION ivymy.mysdate_plus_interval(ivymy.mysdate, pg_catalog.interval)
RETURNS ivymy.mysdatetime
AS $$ SELECT (($1::text::pg_catalog.date) + $2)::ivymy.mysdatetime $$
LANGUAGE sql
PARALLEL SAFE
STRICT
IMMUTABLE
LEAKPROOF;


CREATE OPERATOR + (
       procedure = ivymy.mysdate_plus_interval,
       leftarg = ivymy.mysdate,
       rightarg = pg_catalog.interval
);
/* End bug#Z8459 */